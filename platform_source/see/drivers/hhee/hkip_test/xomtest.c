/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: XOM TEST
 * Date: 2020/05/04
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/debugfs.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <securec.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
#include <asm/compiler.h>
#endif
#include "harness.h"
#include "hkip.h"


#if (KERNEL_VERSION(4, 18, 0) > LINUX_VERSION_CODE)
#define vm_area_alloc(mm)                                                      \
({                                                                             \
	struct vm_area_struct *vma;                                            \
                                                                               \
	vma = kmem_cache_alloc(vm_area_cachep, GFP_KERNEL);                    \
	if (vma != NULL) {                                                     \
		*vma = (struct vm_area_struct){};                              \
		vma->vm_mm = mm;                                               \
		INIT_LIST_HEAD(&vma->anon_vma_chain);                          \
	}                                                                      \
	vma;                                                                   \
})

#define vm_area_free(vma) (kmem_cache_free(vm_area_cachep, vma))
#endif

/* System registers */
#define DO_SYSREGS_EL1(f)                                                      \
	f(sctlr) f(ttbr0) f(ttbr1) f(tcr) f(esr) f(far) f(afsr0) f(afsr1)      \
		f(mair) f(amair) f(contextidr) f(vbar)

struct hkip_sysreg_ops {
	u64 (*get)(void);
	void (*set)(u64);
};

#define DEFINE_SYSREG_EL1_ATTRIBUTE(name)                                      \
static u64 sysreg_##name##_el1_get(void)                                       \
{                                                                              \
	u64 r = 0;                                                             \
	asm("mrs %0, " #name "_el1\n" : "=r"(r));                              \
	return r;                                                              \
}                                                                              \
                                                                               \
static void sysreg_##name##_el1_set(u64 val)                                   \
{                                                                              \
	asm volatile("msr " #name "_el1, %0\n" : : "r"(val));                  \
}                                                                              \
                                                                               \
static const struct hkip_sysreg_ops sysreg_##name##_el1 = {                    \
	sysreg_##name##_el1_get,                                               \
	sysreg_##name##_el1_set,                                               \
};

DO_SYSREGS_EL1(DEFINE_SYSREG_EL1_ATTRIBUTE)

static DEFINE_MUTEX(g_access_mutex);

static int hkip_sysreg_get(void *data, u64 *val)
{
	const struct hkip_sysreg_ops *reg = data;

	*val = reg->get();
	return 0;
}

int hkip_sysreg_set(void *data, u64 val)
{
	const struct hkip_sysreg_ops *reg = data;

	reg->set(val);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(hkip_sysreg_fops, hkip_sysreg_get,
			hkip_sysreg_set_wrapper, "%016llx\n");

static int hkip_sysreg_try(void *data, u64 val)
{
	u64 oval;
	int ret;
	unsigned long flags;

	local_irq_save(flags);
	hkip_sysreg_get(data, &oval);
	ret = hkip_sysreg_set_wrapper(data, val);
	hkip_sysreg_set(data, oval);
	local_irq_restore(flags);
	return ret;
}

DEFINE_SIMPLE_ATTRIBUTE(hkip_sysreg_try_fops, NULL, hkip_sysreg_try,
			"%016llx\n");

struct hkip_sysreg_entry {
	const char *name;
	const struct hkip_sysreg_ops *ops;
};

static int __init hkip_debugfs_create_sysreg_files(
	const struct hkip_sysreg_entry *sr, struct dentry *dir)
{
	char namebuf[32];
	struct dentry *e = NULL;

	e = debugfs_create_file(sr->name, 0600, dir, (void *)sr->ops,
				&hkip_sysreg_fops);
	if (!e)
		return -ENOMEM;

	snprintf(namebuf, sizeof(namebuf), "%s-try", sr->name);

	if (debugfs_create_file(namebuf, 0200, dir, (void *)sr->ops,
				&hkip_sysreg_try_fops) == NULL) {
		debugfs_remove(e);
		return -ENOMEM;
	}

	return 0;
}

#define REGISTER_SYSREG_EL1_ATTRIBUTE(name)                                    \
	{ #name "_el1", &sysreg_##name##_el1 },

static const struct hkip_sysreg_entry hkip_sysregs[] __initconst = {
	DO_SYSREGS_EL1(REGISTER_SYSREG_EL1_ATTRIBUTE)
};

/* Memory permission checks */
static int perm_char(int res, int c)
{
	if (res < 0)
		return '?';
	if (res == 0)
		return '-';
	return c;
}

static int hkip_seq_print_perms(struct seq_file *s, void *addr)
{
	seq_putc(s, perm_char(hkip_mem_readable(addr), 'r'));
	seq_putc(s, perm_char(hkip_mem_writable(addr), 'w'));
	seq_putc(s, perm_char(hkip_mem_executable(addr), 'x'));
	seq_putc(s, '/');
	seq_putc(s, perm_char(hkip_mem_user_readable(addr), 'r'));
	seq_putc(s, perm_char(hkip_mem_user_writable(addr), 'w'));
	seq_putc(s, perm_char(hkip_mem_user_executable(addr), 'x'));
	seq_putc(s, '\n');
	return 0;
}

static int hkip_seq_print_perms_text_read(struct seq_file *s, void *addr)
{
	seq_putc(s, perm_char(hkip_mem_readable(addr), 'r'));
	seq_putc(s, '\n');
	return 0;
}

#define HKIP_PROT_CODE (PROT_NORMAL & ~PTE_PXN)
#define HKIP_PROT_DATA ((PROT_NORMAL | PTE_USER) & ~PTE_UXN)

#define XOM_READ_CNT   45

#define DEFINE_HKIP_MEM_FOPS(x)                                                \
static int hkip_mem_##x##_open(struct inode *inode, struct file *file)         \
{                                                                              \
	return single_open(file, hkip_mem_##x##_show,                          \
			   inode->i_private);                                  \
}                                                                              \
                                                                               \
static const struct file_operations hkip_mem_##x##_fops = {                    \
	.owner = THIS_MODULE,                                                  \
	.open = hkip_mem_##x##_open,                                           \
	.read = seq_read,                                                      \
	.llseek = seq_lseek,                                                   \
	.release = single_release,                                             \
}

static int hkip_mem_direct_show(struct seq_file *s, void *data)
{
	int ret = 0;
	uint32_t i;
	void *va = NULL;
	phys_addr_t pa;

	mutex_lock(&g_access_mutex);
	pa = virt_to_phys(s->private);

	for (i = 0; i < XOM_READ_CNT; i++) {
		pa += PAGE_SIZE;
		va = phys_to_virt(pa);
		ret += hkip_seq_print_perms_text_read(s, va);
	}
	mutex_unlock(&g_access_mutex);
	return ret;
}
DEFINE_HKIP_MEM_FOPS(direct);

/* Double mapping permission checks */
static void *hkip_remap_addr(struct vm_struct *area, phys_addr_t pa,
			     pteval_t prot)
{
	uintptr_t va;
	void *addr = NULL;
	int ret;

	area->phys_addr = pa;
	va = (uintptr_t)area->addr;
	addr = area->addr + (pa & ~PAGE_MASK);
	ret = ioremap_page_range(va, va + PAGE_SIZE, pa & PAGE_MASK,
				 __pgprot(prot));

	if (ret) {
		vunmap(addr);
		return NULL;
	}
	return addr;
}

static int hkip_mem_code_show(struct seq_file *s, void *data)
{
	phys_addr_t pa = virt_to_phys(s->private);
	static struct vm_struct *area = NULL;
	static void *addr = NULL;
	int ret;

	mutex_lock(&g_access_mutex);
	if (area && addr)
		goto finish;
	area = get_vm_area(PAGE_SIZE, VM_IOREMAP);

	if (!area) {
		mutex_unlock(&g_access_mutex);
		return -ENOMEM;
	}

	addr = hkip_remap_addr(area, pa, HKIP_PROT_CODE);
finish:
	if (!addr) {
		mutex_unlock(&g_access_mutex);
		return -ENOMEM;
	}
	WARN_ON(addr == s->private);
	ret = hkip_seq_print_perms(s, addr);
	mutex_unlock(&g_access_mutex);
	return ret;
}
DEFINE_HKIP_MEM_FOPS(code);

static int hkip_mem_data_show(struct seq_file *s, void *data)
{
	phys_addr_t pa = virt_to_phys(s->private);
	static struct vm_struct *area = NULL;
	static void *addr = NULL;
	int ret;

	mutex_lock(&g_access_mutex);
	if (area && addr)
		goto finish;
	area = get_vm_area(PAGE_SIZE, VM_IOREMAP);

	if (!area) {
		mutex_unlock(&g_access_mutex);
		return -ENOMEM;
	}
	addr = hkip_remap_addr(area, pa, HKIP_PROT_DATA);
finish:
	if (!addr) {
		mutex_unlock(&g_access_mutex);
		return -ENOMEM;
	}
	WARN_ON(addr == s->private);
	ret = hkip_seq_print_perms(s, addr);
	mutex_unlock(&g_access_mutex);
	return ret;
}
DEFINE_HKIP_MEM_FOPS(data);

/* User mapping permission checks */
static int hkip_mem_user_show(struct seq_file *s, pteval_t prot)
{
#if IS_ENABLED(CONFIG_ARM64_SW_TTBR0_PAN)
	return -EOPNOTSUPP;
#else
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma = NULL;
	phys_addr_t pa = virt_to_phys(s->private);
	unsigned long va;
	void *addr = NULL;
	int ret;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	down_write(&mm->mmap_lock);
#else
	down_write(&mm->mmap_sem);
#endif
	va = get_unmapped_area(NULL, 0, PAGE_SIZE, 0, 0);
	if (IS_ERR_VALUE(va)) {
		ret = -ENOMEM;
		goto out;
	}

	addr = (void *)(uintptr_t)(va + (pa & ~PAGE_MASK));
	ret = -ENOMEM;
	vma = vm_area_alloc(mm);
	if (!vma)
		goto out;

	vma->vm_start = va;
	vma->vm_end = va + PAGE_SIZE;
	vma->vm_flags = VM_READ | VM_WRITE | VM_EXEC;
	vma->vm_page_prot = __pgprot(prot);

	ret = vm_iomap_memory(vma, pa, PAGE_SIZE);

	if (ret == 0)
		ret = hkip_seq_print_perms(s, addr);

	zap_page_range(vma, vma->vm_start, PAGE_SIZE);
	vm_area_free(vma);
out:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	up_write(&mm->mmap_lock);
#else
	up_write(&mm->mmap_sem);
#endif
	return ret;
#endif
}

static int hkip_mem_code_user_show(struct seq_file *s, void *data)
{
	int ret;

	mutex_lock(&g_access_mutex);
	ret = hkip_mem_user_show(s, HKIP_PROT_CODE);
	mutex_unlock(&g_access_mutex);
	return ret;
}
DEFINE_HKIP_MEM_FOPS(code_user);

static int hkip_mem_data_user_show(struct seq_file *s, void *data)
{
	int ret;

	mutex_lock(&g_access_mutex);
	ret = hkip_mem_user_show(s, HKIP_PROT_DATA);
	mutex_unlock(&g_access_mutex);
	return ret;
}
DEFINE_HKIP_MEM_FOPS(data_user);

struct hkip_mem_map {
	const char *format;
	const struct file_operations *fops;
};

static const struct hkip_mem_map hkip_mem_maps[] __initconst = {
	{ "%s", &hkip_mem_direct_fops },
	{ "%s-remap-code", &hkip_mem_code_fops },
	{ "%s-remap-data", &hkip_mem_data_fops },
	{ "%s-remap-code-user", &hkip_mem_code_user_fops },
	{ "%s-remap-data-user", &hkip_mem_data_user_fops },
};

static int __init hkip_debugfs_create_mem_files(const char *name,
						struct dentry *dir, void *addr)
{
	struct dentry *e[ARRAY_SIZE(hkip_mem_maps)];
	int i;

	for (i = 0; i < ARRAY_SIZE(e); i++) {
		char namebuf[32];

		snprintf(namebuf, sizeof(namebuf), hkip_mem_maps[i].format, name);
		e[i] = debugfs_create_file(namebuf, 0400, dir, addr,
					   hkip_mem_maps[i].fops);
		if (e[i] == NULL) {
			while (i > 0)
				debugfs_remove(e[--i]);
			return -ENOMEM;
		}
	}
	return 0;
}

/* Literal read check */
static int hkip_code_literal_get(void *data, u64 *value)
{
	(void)data;
	return hkip_literal_read(value) ? -EACCES : 0;
}

/* Literal read check */
static int hkip_code_literal_user_get(void *data, u64 *value)
{
	(void)data;
	return hkip_literal_read_user(value) ? -EACCES : 0;
}

DEFINE_SIMPLE_ATTRIBUTE(hkip_code_literal_user_fops, hkip_code_literal_user_get,
			NULL, "%016llx\n");

DEFINE_SIMPLE_ATTRIBUTE(hkip_code_literal_fops, hkip_code_literal_get, NULL,
			"%016llx\n");

/* Module registration */

static struct dentry *xomdir;
static u64 *heap_value;

static int __init hkip_debugfs_init(void)
{
	int ret;
	struct dentry *srdir = NULL;
	struct dentry *memdir = NULL;
	size_t i;

	if (hhee_check_enable() == HHEE_DISABLE)
		return 0;

	heap_value = kmalloc(sizeof(*heap_value), GFP_KERNEL);
	if (!heap_value)
		goto error;

	ret = memcpy_s(heap_value, sizeof(*heap_value), hkip_rodata, sizeof(*heap_value));
	if (ret)
		goto error;

	xomdir = debugfs_create_dir("hkip_xom", NULL);
	if (!xomdir)
		goto error;

	srdir = debugfs_create_dir("sysregs", xomdir);
	if (!srdir)
		goto error;

	for (i = 0; i < ARRAY_SIZE(hkip_sysregs); i++)
		if (hkip_debugfs_create_sysreg_files(&hkip_sysregs[i], srdir))
			goto error;

	memdir = debugfs_create_dir("mem", xomdir);
	if (!memdir)
		goto error;

	if (hkip_debugfs_create_mem_files("text", memdir, hkip_text))
		goto error;
	if (hkip_debugfs_create_mem_files("rodata", memdir, hkip_rodata))
		goto error;
	if (hkip_debugfs_create_mem_files("data", memdir, hkip_data))
		goto error;
	if (hkip_debugfs_create_mem_files("heap", memdir, heap_value))
		goto error;
	if (debugfs_create_file("literal", 0400, memdir, NULL,
				&hkip_code_literal_fops) == NULL)
		goto error;
	if (debugfs_create_file("literal-user", 0400, memdir, NULL,
				&hkip_code_literal_user_fops) == NULL)
		goto error;

	return 0;
error:
	kfree(heap_value);
	heap_value = NULL;
	debugfs_remove_recursive(xomdir);
	return -ENOMEM;
}

static void __exit hkip_debugfs_exit(void)
{
	debugfs_remove_recursive(xomdir);
	kfree(heap_value);
	heap_value = NULL;
}

module_init(hkip_debugfs_init);
module_exit(hkip_debugfs_exit);
MODULE_LICENSE("GPL");
