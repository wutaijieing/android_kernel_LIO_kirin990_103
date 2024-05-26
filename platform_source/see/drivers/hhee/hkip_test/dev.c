/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
 * Description: hkip test driver
 * Create : 2017/12/6
 */

#define pr_fmt(fmt) "HKIPT: " fmt

#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/irqdomain.h>
#include <linux/of_irq.h>
#include <asm/uaccess.h>
#include <linux/io.h>
#include "hkip.h"

int hkip_get_monitorlog_info(u64 *base, u64 *size)
{
	*base = *size = 0;
	hkip_hvc_2(HKIP_MONITORLOG_INFO, base, size, 0, 0, 0);

	if (*base == 0 || *size == 0) {
		pr_err("Monitor buffer not available base:%llx size:%llx\n",
		       *base, *size);
		return -ENODEV;
	}

	/* this would have to be removed when HKIP aligns size properly */
	*size &= ~(PAGE_SIZE - 1);

	DBG("log area start at PA %llx and has size %llx\n", *base, *size);
	return 0;
}

bool el1_addr_invalid(u64 addr, bool is_write)
{
	u64 par;

	if (is_write)
		asm volatile("at s1e1w, %0" : : "r"(addr));
	else
		asm volatile("at s1e1r, %0" : : "r"(addr));
	isb();

	par = read_sysreg(par_el1);
	if (unlikely(par & 1)) {
		pr_err("Translation failed. PAR_EL1: 0x%016llx\n", par);
		return true; /* Translation failed */
	}

	return false;
}

static bool el1_addr_range_invalid(u64 addr, size_t count, bool is_write)
{
	u64 va;

	for (va = addr & PAGE_MASK; va < addr + count; va += PAGE_SIZE) {
		if (el1_addr_invalid(va, is_write)) {
			pr_err("Inaccessible kernel address 0x%016llx\n", va);
			return 1;
		}
	}
	return 0;
}

int map_physical_area(struct vm_struct **varea, phys_addr_t pa, size_t size,
		      pgprot_t prot)
{
	uintptr_t addr;
	unsigned long offset = pa & ~PAGE_MASK;
	struct vm_struct *area = NULL;
	int ret;

	pa &= PAGE_MASK;
	size = PAGE_ALIGN(size + offset);

	area = get_vm_area(size, VM_IOREMAP);
	if (!area) {
		pr_err("error finding place to map memory\n");
		return -ENOMEM;
	}

	addr = (uintptr_t)area->addr;
	ret = ioremap_page_range(addr, addr + size, pa, PAGE_KERNEL);
	if (!ret) {
		*varea = area;
		return 0;
	}
	pr_err("failed to map memory\n");
	free_vm_area(area);
	return ret;
}

static size_t weakened_copy_to_user(void __user *to, const void *from,
				    size_t count)
{
	size_t left = count;
	while (left > 0) {
		char buf;

		buf = *(char *)from;
		from++;
		if (put_user(buf, (char __user *)(uintptr_t __user *)(to++)))
			break;
		left--;
	}
	return count - left;
}

static size_t weakened_copy_from_user(void *to, const void __user *from,
				      size_t count)
{
	size_t left = count;

	while (left > 0) {
		char buf;

		if (get_user(buf, (const char __user *)(uintptr_t __user *)(from++)))
			break;
		*(char *)to = buf;
		to++;
		left--;
	}
	return count - left;
}

int nr_interrupts = 0;

static int hkip_open(struct inode *inode, struct file *filep)
{
	filep->f_mode |= FMODE_UNSIGNED_OFFSET;
	return 0;
}

static loff_t hkip_llseek(struct file *file, loff_t offset, int whence)
{
	switch (whence) {
	case SEEK_SET:
		offset = vfs_setpos(file, offset, offset);
		break;
	case SEEK_CUR:
		spin_lock(&file->f_lock);
		offset = vfs_setpos(file, file->f_pos + offset,
				    file->f_pos + offset);
		spin_unlock(&file->f_lock);
		break;
	default:
		return -EINVAL;
	}
	return offset;
}

static ssize_t hkip_virt_read(struct file *file, char __user *to, size_t count,
			      loff_t *ppos)
{
	uintptr_t from = *ppos;

	if (el1_addr_range_invalid(from, count, 0))
		return -EFAULT;

	count = weakened_copy_to_user(to, (void *)from, count);
	if (!count) {
		pr_err("%s: Write access to userspace failed\n", __FUNCTION__);
		return -EFAULT;
	}
	*ppos += count;
	return count;
}

static ssize_t hkip_virt_write(struct file *file, const char __user *from,
			       size_t count, loff_t *ppos)
{
	uintptr_t to = *ppos;

	if (el1_addr_range_invalid(to, count, 1))
		return -EFAULT;

	count = weakened_copy_from_user((void *)to, from, count);
	if (!count) {
		pr_err("%s: Read access to userspace failed\n", __FUNCTION__);
		return -EFAULT;
	}
	*ppos += count;
	return count;
}

static ssize_t hkip_phys_read(struct file *file, char __user *to, size_t count,
			      loff_t *ppos)
{
	ssize_t ret;
	struct vm_struct *area = NULL;
	unsigned offset = (((unsigned)*ppos) & ~PAGE_MASK);

	ret = map_physical_area(&area, *ppos, count, PAGE_KERNEL);
	if (ret)
		return ret;

	count = weakened_copy_to_user(to, area->addr + offset, count);
	if (!count) {
		ret = -EFAULT;
		pr_err("%s: Write access to userspace failed\n", __FUNCTION__);
		goto out;
	}
	*ppos += count;
	ret = (ssize_t)count;
out:
	free_vm_area(area);
	return ret;
}

static ssize_t hkip_phys_write(struct file *file, const char __user *from,
			       size_t count, loff_t *ppos)
{
	ssize_t ret;
	struct vm_struct *area = NULL;
	unsigned offset = (((unsigned)*ppos) & ~PAGE_MASK);

	ret = map_physical_area(&area, *ppos, count, PAGE_KERNEL);
	if (ret)
		return ret;

	count = weakened_copy_from_user(area->addr + offset, from, count);
	if (!count) {
		ret = -EFAULT;
		pr_err("%s: Read access to userspace failed\n", __FUNCTION__);
		goto out;
	}

	*ppos += count;
	ret = (ssize_t)count;
out:
	free_vm_area(area);
	return ret;
}

static int hkip_virt_mmap(struct file *filep, struct vm_area_struct *vma)
{
	size_t size;
	unsigned long pfn;
	uintptr_t *pgaddr = NULL;
	struct vm_struct *area = NULL;

	if (vma->vm_end < vma->vm_start)
		return -EINVAL;

	size = vma->vm_end - vma->vm_start;
	pgaddr = (uintptr_t *)(uintptr_t)((vma->vm_pgoff << PAGE_SHIFT) |
					  HKIP_VADDR_RESMASK);
	if (is_vmalloc_addr(pgaddr)) {
		area = find_vm_area(pgaddr);
		if (!area) {
			pr_err("vm area not found\n");
			goto skip;
		}

		if (!(area->flags & VM_USERMAP)) {
			pr_err("no permission to map to userspace\n");
			goto skip;
		}

		return remap_vmalloc_range(vma, pgaddr, 0);
	}
skip:
	if (virt_addr_valid((uintptr_t)pgaddr) || 1) {
		pfn = virt_to_pfn((uintptr_t)pgaddr);
	} else {
		pr_err("bad address\n");
		return -1;
	}

	if (!valid_mmap_phys_addr_range(pfn, size))
		return -EINVAL;

	return remap_pfn_range(vma, vma->vm_start, pfn,
			       size, vma->vm_page_prot);
}

static int hkip_phys_mmap(struct file *filep, struct vm_area_struct *vma)
{
	size_t size;
	unsigned long pfn;

	if (vma->vm_end < vma->vm_start)
		return -EINVAL;

	size = vma->vm_end - vma->vm_start;
	if (!valid_mmap_phys_addr_range(vma->vm_pgoff, size))
		return -EINVAL;

	pfn = vma->vm_pgoff;
	return remap_pfn_range(vma, vma->vm_start, pfn,
			       size, vma->vm_page_prot);
}

const struct file_operations hkip_virt_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = hkip_unlocked_ioctl,
	.open = hkip_open,
	.llseek = hkip_llseek,
	.read = hkip_virt_read,
	.write = hkip_virt_write,
	.mmap = hkip_virt_mmap,
};

const struct file_operations hkip_phys_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = hkip_unlocked_ioctl,
	.open = hkip_open,
	.llseek = hkip_llseek,
	.read = hkip_phys_read,
	.write = hkip_phys_write,
	.mmap = hkip_phys_mmap,
};

static struct miscdevice hkip_dev_phy = { MISC_DYNAMIC_MINOR, "hkip_phys",
					  &hkip_phys_fops };
static struct miscdevice hkip_dev_virt = { MISC_DYNAMIC_MINOR, "hkip_virt",
					   &hkip_virt_fops };

static int hkip_notify(unsigned int len, void *buf)
{
	nr_interrupts += 1;
	return 0;
}

void hkip_cleanup(void);
int __init hkip_init(void)
{
	int ret = -ENOTTY;

	if (HHEE_DISABLE == hhee_check_enable()) {
		pr_err("[%s]hhee is disabled\n", __func__);
		return 0;
	}
	ret = hhee_msg_register_handler(HHEE_MSG_ID_HKIP_TEST, hkip_notify);
	if (ret)
		pr_err("hkip register msg handler failed\n");

	ret = misc_register(&hkip_dev_phy);
	if (ret != 0) {
		pr_err("failed to register device\n");
		goto out;
	}

	ret = misc_register(&hkip_dev_virt);
	if (ret != 0) {
		pr_err("failed to register device\n");
		goto out;
	}

	pr_info("driver Loaded\n");
	return 0;

out:
	hkip_cleanup();
	pr_err("failed to load driver\n");
	return ret;
}

void hkip_cleanup(void)
{
	if (hkip_dev_phy.list.prev)
		misc_deregister(&hkip_dev_phy);
	if (hkip_dev_virt.list.prev)
		misc_deregister(&hkip_dev_virt);
}

module_init(hkip_init);
module_exit(hkip_cleanup);
MODULE_LICENSE("GPL");
