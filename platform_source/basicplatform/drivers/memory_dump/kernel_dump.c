/*
 * kernel_dump.c
 *
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <asm/pgtable.h>
#include <linux/mm_types.h>
#include <linux/memblock.h>
#include <linux/percpu.h>
#include <linux/printk.h>
#include <asm/memory.h>
#include "kernel_dump.h"
#include <platform_include/basicplatform/linux/mntn_dump.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/rculist.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "securec.h"
#include <platform_include/basicplatform/linux/pr_log.h>
#include <platform_include/basicplatform/linux/util.h>

#define PR_LOG_TAG MEMORY_DUMP_TAG

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
#define KERNELDUMP_TEXT_OFFSET 0x00080000
#else
#define KERNELDUMP_TEXT_OFFSET 0x00
#endif

/* the max size of mod->core_size is 4M */
#define MODULE_MAX_CORE_SIZE (4 * 1024 * 1024)

extern int pcpu_base_size;
extern struct mm_struct init_mm;
extern struct page *mem_map;
extern char _text[];
extern char _end[];
#ifdef CONFIG_RANDOMIZE_BASE
extern s64 phystart_addr;
#endif
#ifdef CONFIG_KERNELDUMP_KO_DBG
extern struct list_head modules;
#endif

#ifdef CONFIG_DFX_KERNELDUMP
struct reserved_mem {
	u64 base;
	u64 size;
} g_kdump_reserve_mem[KDUMP_RESERVED_MAX] = {{0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}};
#endif

struct kernel_dump_cb *g_kdump_cb;
static struct table_extra {
	u64 extra_mem_phy_base;
	u64 extra_mem_virt_base;
	u64 extra_mem_size;
} g_tbl_extra_mem[MAX_EXTRA_MEM] = {{0, 0} };
static unsigned int extra_index;
static DEFINE_RAW_SPINLOCK(g_kdump_lock);

/*******************************************************************************
Function:       dataready_write_proc
Description:    write /proc/resize-cb, for get the status of data_partition
Input:          file;buffer;count;data
Output:         NA
Return:         >0:success;other:fail
********************************************************************************/
ssize_t resizecb_write_proc(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{
	ssize_t ret = -EINVAL;
	char tmp;

	/* buffer must be '1' or '0', so count<=2 */
	if (count > RESIZE_FLAG_MAX)
		return ret;

	if (!buffer)
		return ret;

	if (!file)
		return ret;

	if (!data)
		return ret;

	/* should ignore character '\n' */
	if (copy_from_user(&tmp, buffer, sizeof(tmp))) {
		return -EFAULT;
	}

	pr_err("%s():%d:input arg [%c],%d\n", __func__, __LINE__, tmp, tmp);

	if (tmp == '1') {
		g_kdump_cb->resize_flag = SKP_DUMP_RESIZE_SUCCESS_FLAG;
		g_kdump_cb->crc = 0;
		g_kdump_cb->crc = checksum32((u32 *)g_kdump_cb, sizeof(struct kernel_dump_cb));
		pr_err("%s():%d  resize success \n", __func__, __LINE__);
	}
	else if (tmp == '0') {
		g_kdump_cb->resize_flag = SKP_DUMP_RESIZE_FAIL;
		g_kdump_cb->crc = 0;
		g_kdump_cb->crc = checksum32((u32 *)g_kdump_cb, sizeof(struct kernel_dump_cb));
		pr_err("%s():%d  resize fail \n", __func__, __LINE__);
	}
	else
		pr_err("%s():%d:input arg invalid[%c]\n", __func__, __LINE__, tmp);

	return 1;
}

/*******************************************************************************
Function:       dataready_info_show
Description:    show g_dataready_flag
Input:          struct seq_file *m, void *v
Output:         NA
Return:         0:success;other:fail
********************************************************************************/
static int resizecb_info_show(struct seq_file *m, void *v)
{
	pr_err("%x\n", g_kdump_cb->resize_flag);
	return 0;
}

/*******************************************************************************
Function:       dataready_open
Description:    open /proc/data-ready
Input:          inode;file
Output:         NA
Return:         0:success;other:fail
********************************************************************************/
static int resizecb_open(struct inode *inode, struct file *file)
{
	if (!file)
		return -EFAULT;

	return single_open(file, resizecb_info_show, NULL);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
 static const struct proc_ops resizecb_proc_fops = {
	.proc_open    	= resizecb_open,
	.proc_write  	= resizecb_write_proc,
	.proc_read		= seq_read,
	.proc_lseek		= seq_lseek,
	.proc_release	= single_release,
};
#else
static const struct file_operations resizecb_proc_fops = {
	.open    	= resizecb_open,
	.write  	= resizecb_write_proc,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};
#endif

static int add_mem2table(u64 va, u64 pa, u64 size, bool need_crc)
{
	unsigned int i;
	bool crc_check = false;

	if ((pa == 0) || (va == 0) || (size == 0) || (extra_index >= MAX_EXTRA_MEM))
		return -1;

	raw_spin_lock(&g_kdump_lock);
	/* Kernel dump is not inited */
	if (!g_kdump_cb) {
			g_tbl_extra_mem[extra_index].extra_mem_phy_base = pa;
			g_tbl_extra_mem[extra_index].extra_mem_virt_base = va;
			g_tbl_extra_mem[extra_index].extra_mem_size = size;
			extra_index++;
	} else {
		i = extra_index;
		if (i < MAX_EXTRA_MEM) {
			g_kdump_cb->extra_mem_phy_base[i] = pa;
			g_kdump_cb->extra_mem_virt_base[i] = va;
			g_kdump_cb->extra_mem_size[i] = size;
			extra_index += 1;

			crc_check = true;
		} else {
			pr_err("%s: extra memory(nums:%d) is out of range. \r\n", __func__, extra_index);
			goto err;
		}
	}

	if ((need_crc == true) && (crc_check == true)) {
		g_kdump_cb->crc = 0;
		g_kdump_cb->crc = checksum32((u32 *)g_kdump_cb, sizeof(struct kernel_dump_cb));
	}

	raw_spin_unlock(&g_kdump_lock);
	return 0;
err:
	raw_spin_unlock(&g_kdump_lock);
	return -1;
}

int add_extra_table(u64 pa, u64 size)
{
	return add_mem2table((uintptr_t)phys_to_virt(pa), pa, size, true);
}

#ifdef CONFIG_DFX_KERNELDUMP
void kdump_reserved_addr_save(phys_addr_t base, phys_addr_t size)
{
	static int cnt = 0;

	if ((!base) || (!size))
		return;

	g_kdump_reserve_mem[cnt].base = base;
	g_kdump_reserve_mem[cnt].size = size;

	cnt++;
}

void kdump_reserved_flag_set(void)
{
	int i;
	struct page *page = NULL;
	static int cnt;
	u64 _phys;
	u64 page_size = KDUMP_PAGE_SIZE;

	for (i = 0; i < KDUMP_RESERVED_MAX; i++) {
		cnt = 0;
		if (g_kdump_reserve_mem[i].base != 0) {
			for (_phys = g_kdump_reserve_mem[i].base; _phys < (g_kdump_reserve_mem[i].base + g_kdump_reserve_mem[i].size); _phys += page_size) {
				page = phys_to_page((uintptr_t)_phys);
				SetPageMemDump(page);
				cnt++;
			}
		}
	}
}
#endif

#ifdef CONFIG_KERNELDUMP_KO_DBG
/*lint -e429*/
int ko_dump(void)
{
	struct module *mod = NULL;
	unsigned int mod_core_size;
	char *p = NULL;
	int ret = 0;
	bool crc_check = false;

	if (!g_kdump_cb) {
		pr_info("kernel dump is not initd\n");
		return -1;
	}

	list_for_each_entry_rcu(mod, &modules, list) {
		if (mod->state == MODULE_STATE_LIVE) {
			if (mod->core_size > MODULE_MAX_CORE_SIZE) {
				pr_err("mod->core_size out of range!\n");
				ret = -1;
				goto exit;
			}

			p = (char *)kzalloc(mod->core_size, GFP_ATOMIC);
			if (!p) {
				pr_err("kmalloc mem for ko fail!\n");
				ret = -1;
				goto exit;
			}

			mod_core_size = mod->core_size;
			memcpy_s((void *)p, mod_core_size, (void *)mod->module_core, mod->core_size);
			ret = add_mem2table((u64)mod->module_core, (u64)virt_to_phys((void *)p), mod->core_size, false);
			if (ret) {
				pr_err("add ko mem to kerneldump table fail!\n");
				ret = -1;
				goto exit;
			}

			crc_check = true;
		}
	}

exit:
	if (crc_check == true) {
		g_kdump_cb->crc = 0;
		g_kdump_cb->crc = checksum32((u32 *)g_kdump_cb, sizeof(struct kernel_dump_cb));
	}
	return ret;
}
/*lint +e429*/
#endif

static void kernel_dump_printcb(struct memblock_type *print_mb_cb, struct kernel_dump_cb *cb)
{
#ifdef CONFIG_DFX_BB_DEBUG
	unsigned int i;

	pr_info("cb->page is 0x%lx\n", (uintptr_t)(cb->page));
	pr_info("cb->page_shift is 0x%x\n", cb->page_shift);
	pr_info("cb->struct_page_size is 0x%x\n", cb->struct_page_size);
	pr_info("cb->phys_offset   is 0x%llx\n", cb->phys_offset);
	pr_info("cb->kernel_offset is 0x%llx\n", cb->kernel_offset);
	pr_info("cb->page_offset is 0x%llx\n", cb->page_offset);
	pr_info("cb->pfn_offset is 0x%llx\n", cb->pfn_offset);
	pr_info("cb->ttbr is ttbr:%pK\n", (void *)(uintptr_t)cb->ttbr);
	pr_info("cb->mb_cb is 0x%lx\n", (uintptr_t)(cb->mb_cb));
	pr_info("cb->section_size is 0x%llx\n", cb->section_size);
	pr_info("cb->pmd_size is 0x%llx\n", cb->pmd_size);
	pr_info("cb->mbr_size is 0x%llx\n", cb->mbr_size);
	pr_info("cb->page_flags_offset is 0x%x\n", cb->page_flags_offset);
	pr_info("cb->page_mapcount_offset is 0x%x\n", cb->page_mapcount_offset);
	pr_info("cb->page_refcount_offset is 0x%x\n", cb->page_refcount_offset);
	pr_info("cb->mb_need_filter is 0x%x\n", cb->mb_need_filter);
	for (i = 0; i < MAX_PAGE_FLAGS_NUM; i++)
		pr_info("cb->pageflags_map_value[%u] is 0x%x\n", i, cb->pageflags_map_value[i]);
	pr_info("cb->kern_map_offset is 0x%llx\n", (unsigned long long)(cb->kern_map_offset));
	pr_info("print_mb_cb->regions addr is 0x%lx\n", (uintptr_t)(print_mb_cb->regions));
	for (i = 0; i < print_mb_cb->cnt; i++) {
		pr_info("print_mb_cb->regions base is 0x%llx\n", (print_mb_cb->regions + i)->base);
		pr_info("print_mb_cb->regions size is 0x%llx\n", (print_mb_cb->regions + i)->size);
	}
#endif
	return;
}

int kernel_dump_init(void)
{
	unsigned int i, j;
	phys_addr_t mem_ret;
	struct kernel_dump_cb *cb = NULL;
	struct memblock_type *print_mb_cb = NULL;
	struct proc_dir_entry *proc_dir_entry = NULL;

	if (register_mntn_dump(MNTN_DUMP_KERNEL_DUMP, sizeof(struct kernel_dump_cb), (void **)&cb)) {
		pr_err("%s: fail to get reserve memory\r\n", __func__);
		goto err;
	}
#ifdef CONFIG_DFX_BB_DEBUG
	pr_info("%s begin, 0x%llx!\r\n", __func__, (unsigned long long)(uintptr_t)cb);
#endif
	if (EOK != memset_s((void *)cb, sizeof(struct kernel_dump_cb), 0, sizeof(struct kernel_dump_cb)))
		pr_err("[%s:%d]: memset_s err \n]", __func__, __LINE__);

	cb->magic = KERNELDUMP_CB_MAGIC;
	cb->page_shift = PAGE_SHIFT;
	cb->struct_page_size = sizeof(struct page);
#ifdef CONFIG_RANDOMIZE_BASE
	cb->phys_offset = phystart_addr;
	cb->kernel_offset = kimage_vaddr - KERNELDUMP_TEXT_OFFSET;
#else
	cb->phys_offset = PHYS_OFFSET;
	cb->kernel_offset = KIMAGE_VADDR;/*lint !e648*/
#endif
	cb->page_offset = PAGE_OFFSET;/*lint !e648*/
	cb->extra_mem_phy_base[0] = virt_to_phys(_text);
	cb->extra_mem_virt_base[0] = (u64)(uintptr_t)_text;
	cb->extra_mem_size[0] = ALIGN((u64)(uintptr_t)_end - (u64)(uintptr_t)_text, PAGE_SIZE);
	cb->extra_mem_phy_base[1] = virt_to_phys(pcpu_base_addr); /* per cpu info */
	cb->extra_mem_virt_base[1] = (u64)(uintptr_t)pcpu_base_addr; /* per cpu info */
	cb->extra_mem_size[1] = ALIGN((u64)pcpu_base_size, PAGE_SIZE)*CONFIG_NR_CPUS;
	for (i = 2, j = 0; i < MAX_EXTRA_MEM && j < extra_index; i++, j++) {
		cb->extra_mem_phy_base[i] = g_tbl_extra_mem[j].extra_mem_phy_base;
		cb->extra_mem_virt_base[i] = g_tbl_extra_mem[j].extra_mem_virt_base;
		cb->extra_mem_size[i] = g_tbl_extra_mem[j].extra_mem_size;
	}
	extra_index = i;
#ifdef CONFIG_DFX_BB_DEBUG
	pr_info("_text:0x%pK _end:0x%pK\n", _text, _end);
#endif
#ifdef CONFIG_FLATMEM
	cb->page = (uintptr_t)mem_map;
	cb->pfn_offset = PHYS_PFN_OFFSET;
	cb->section_size = 0;
#elif defined CONFIG_SPARSEMEM_VMEMMAP
	cb->page = (uintptr_t)vmemmap;
	cb->pfn_offset = 0;
	cb->pmd_size = PMD_SIZE;
	cb->section_size = 1UL << SECTION_SIZE_BITS;
#else
#error "Configurations other than CONFIG_PLATMEM and CONFIG_SPARSEMEM_VMEMMAP are not supported"
#endif
	#ifdef CONFIG_64BIT
	/* Subtract the base address that TTBR1 maps */
	cb->kern_map_offset = (UL(0xffffffffffffffff) << VA_BITS);/*lint !e648*/
	#else
	cb->kern_map_offset = 0;
	#endif

	cb->ttbr = virt_to_phys(init_mm.pgd);

	cb->mb_cb = (struct memblock_type *)(uintptr_t)virt_to_phys(&memblock.memory);
	print_mb_cb = &memblock.memory;
	cb->mbr_size = sizeof(struct memblock_region);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0))
	cb->text_kaslr_offset = (uintptr_t)_text - (KIMAGE_VADDR + TEXT_OFFSET); /*lint !e648*/
#else
	cb->text_kaslr_offset = (uintptr_t)_text - KIMAGE_VADDR;
#endif
	mem_ret = memblock_start_of_DRAM();
#ifdef CONFIG_DFX_MNTN_PC
	cb->linear_kaslr_offset = __phys_to_virt(mem_ret) - PAGE_OFFSET - mem_ret;/*lint !e648 !e666*/
	cb->kernel_version      = LINUX_VERSION_CODE;
#else
	cb->linear_kaslr_offset = __phys_to_virt(mem_ret) - PAGE_OFFSET;/*lint !e648 !e666*/
#endif
	cb->resize_flag = SKP_DUMP_RESIZE_FAIL; /* init fail status */
	cb->skp_flag    = SKP_DUMP_SKP_FAIL; /* init fail status */
	cb->page_flags_offset = offsetof(struct page, flags);
	cb->page_mapcount_offset = offsetof(struct page, _mapcount);
	cb->page_refcount_offset = offsetof(struct page, _refcount);
	cb->pageflags_map_value[PG_MEMDUMP_INDEX] = PG_memdump;
	cb->pageflags_map_value[PG_LRU_INDEX] = PG_lru;
	cb->pageflags_map_value[PG_RESERVED_INDEX] = PG_reserved;
#ifdef CONFIG_DFX_PAGE_TABLE_DUMP
	cb->pageflags_map_value[PG_KERNELPAGE_INDEX] = PG_kernelpage;
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	cb->mb_need_filter = 1;
#endif
	kernel_dump_printcb(print_mb_cb, cb);
	g_kdump_cb = cb;

	cb->crc = 0;
	cb->crc = checksum32((u32 *)cb, sizeof(struct kernel_dump_cb));

	if (check_mntn_switch(MNTN_KERNEL_DUMP_ENABLE)) {
		pr_err("%s: kdump enable ,proc create \n", __func__);
		proc_dir_entry = proc_create(RESIZE_RESULT_NAME,
			RESIZE_PROC_RIGHT,
			NULL,
			&resizecb_proc_fops);
		if (!proc_dir_entry) {
			pr_err("proc_create RESIZE_RESULT_NAME fail\n");
			return -1;
		}
	}

#ifdef CONFIG_DFX_KERNELDUMP
	kdump_reserved_flag_set();
#endif
	return 0;
err:
	return -1;
}
early_initcall(kernel_dump_init);

