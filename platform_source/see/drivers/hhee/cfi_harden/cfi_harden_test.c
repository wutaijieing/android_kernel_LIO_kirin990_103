/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: CFI harden test program.
 * Create: 2020/10/19
 */

#include <asm/memory.h>
#include <asm/set_memory.h>
#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/stringify.h>
#include <linux/sysfs.h>
#include <linux/version.h>
#include <platform_include/see/prmem.h>

#define MODULE "cfi harden test:"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
typedef u16 shadow_t;

struct cfi_shadow {
	/* Page index for the beginning of the shadow */
	unsigned long base;
	/* An array of __cfi_check locations (as indices to the shadow) */
	shadow_t shadow[1];
} __packed;

/*
 * The shadow covers ~128M from the beginning of the module region. If
 * the region is larger, we fall back to __module_address for the rest.
 */
#define __SHADOW_RANGE          (_UL(SZ_128M) >> PAGE_SHIFT)

/* The in-memory size of struct cfi_shadow, always at least one page */
#define __SHADOW_PAGES          ((__SHADOW_RANGE * sizeof(shadow_t)) >> PAGE_SHIFT)
#define SHADOW_PAGES            max(1UL, __SHADOW_PAGES)
#define SHADOW_SIZE             (SHADOW_PAGES << PAGE_SHIFT)
#else
struct shadow_range {
	/* Module address range */
	unsigned long mod_min_addr;
	unsigned long mod_max_addr;
	/* Module page range */
	unsigned long min_page;
	unsigned long max_page;
};

#define SHADOW_ORDER    1
#define SHADOW_PAGES    (1 << SHADOW_ORDER)
#define SHADOW_SIZE \
	((SHADOW_PAGES * PAGE_SIZE - sizeof(struct shadow_range)) / sizeof(u16))
#define SHADOW_INVALID  0xFFFF

struct cfi_shadow {
	/* Page range covered by the shadow */
	struct shadow_range r;
	/* Page offsets to __cfi_check functions in modules */
	u16 shadow[SHADOW_SIZE];
};
#endif

typedef void (*mod_func)(int);

enum test_cases {
	TEST_CFI_HARDEN,
	TEST_MAX,
};

static struct kobj_attribute *g_cfi_attrs[TEST_MAX + 1] = {[TEST_MAX] = NULL, };
static unsigned long g_func1_addr;
static unsigned long g_func2_addr;
static struct module *g_cfi_mod;
static struct cfi_shadow *g_cfi_shadow;
static mod_func g_mod_ptr;

void do_not_check(void)
{
	pr_info("%s do not check!!!\n", MODULE);
}

static ssize_t about_cfi_shadow_show(struct kobject *kobj,
				     struct kobj_attribute *attribute,
				     char *buf)
{
	uintptr_t *cfi_shadow_addr = NULL;

	g_func1_addr = kallsyms_lookup_name("test_cfi_func1");
	if (!g_func1_addr) {
		pr_err("%s kallsyms lookup func1 err!\n", MODULE);
		return 0;
	}
	pr_info("%s %s func1 addr: 0x%lx\n", MODULE, __func__, g_func1_addr);

	g_func2_addr = kallsyms_lookup_name("test_cfi_func2");
	if (!g_func2_addr) {
		pr_err("%s kallsyms lookup func2 err!\n", MODULE);
		return 0;
	}
	pr_info("%s %s func2 addr: 0x%lx\n", MODULE, __func__, g_func2_addr);

	preempt_disable();
	g_cfi_mod = __module_address(g_func1_addr);
	preempt_enable();
	pr_info("%s %s cfi mod: 0x%lx, cfi_mod->name: %s\n",
		MODULE, __func__, (uintptr_t)g_cfi_mod, g_cfi_mod->name);

	cfi_shadow_addr = (uintptr_t *)(uintptr_t)kallsyms_lookup_name("cfi_shadow");
	if (!cfi_shadow_addr) {
		pr_err("%s kallsyms lookup cfi_shadow err!\n", MODULE);
		return 0;
	}

	g_cfi_shadow = (struct cfi_shadow *)(*cfi_shadow_addr);
	if (!g_cfi_shadow) {
		pr_err("%s get cfi_shadow err!\n", MODULE);
		return 0;
	}

	set_memory_rw((uintptr_t)g_cfi_shadow, SHADOW_PAGES);
	pr_info("%s %s cfi_shadow_addr: 0x%lx, cfi_shadow: 0x%lx\n", MODULE,
		__func__, (uintptr_t)cfi_shadow_addr, (uintptr_t)g_cfi_shadow);

	return snprintf(buf, PAGE_SIZE, "Passed\n");
}

static void call_fptr(unsigned long func_addr)
{
	pr_info("%s call_fptr addr 0x%lx\n", MODULE, func_addr);
#ifdef CONFIG_HKIP_CFI_HARDEN
	g_cfi_mod->safe_cfi_area->cfi_check = (cfi_check_fn)&do_not_check;
#else
	g_cfi_mod->cfi_check = (cfi_check_fn)&do_not_check;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	if (g_cfi_shadow) {
		g_cfi_shadow->base = 0;
		pr_err("%s modify g_cfi_shadow->base to 0x%lx\n",
		       MODULE, g_cfi_shadow->base);
	}
#else
	if (g_cfi_shadow) {
		g_cfi_shadow->r.mod_max_addr = 0;
		pr_err("%s modify g_cfi_shadow->r.mod_max_addr to 0x%lx\n",
		       MODULE, g_cfi_shadow->r.mod_max_addr);
	}
#endif

	g_mod_ptr = (mod_func)(uintptr_t)func_addr;
	g_mod_ptr(1);
	pr_err("%s bypass the cfi check!\n", MODULE);
}

static ssize_t about_cfi_shadow_store(struct kobject *kobj,
				      struct kobj_attribute *attribute,
				      const char *buf, size_t count)
{
	unsigned long num;
	unsigned long addr;
	mod_func mod_ptr = NULL;

	sscanf(buf, "%lx", &num);
	/*
	 * echo 1 : normal call
	 * echo 2 : illegal call
	 * echo addr : specify addr call
	 */
	pr_info("%s call num %d\n", MODULE, num);
	if (num == 1) {
		addr = kallsyms_lookup_name("test_cfi_func1");
		if (!addr) {
			pr_err("%s kallsyms lookup func1 err!\n", MODULE);
			return 0;
		}
		mod_ptr = (mod_func)(uintptr_t)addr;
		mod_ptr(0);
		pr_err("%s call normally!\n", MODULE);
	} else if (num == 2) {
		num = kallsyms_lookup_name("test_cfi_func2");
		if (!num) {
			pr_err("%s kallsyms lookup func2 err!\n", MODULE);
			return 0;
		}
		call_fptr(num);
	} else {
		call_fptr(num);
	}

	return count;
}

static void test_cfi_attr(void)
{
	struct kobj_attribute *cfi_attr = NULL;

	cfi_attr = kmalloc(sizeof(struct kobj_attribute), GFP_KERNEL);
	if (!cfi_attr) {
		pr_err("%s %s malloc cfi kobj attr fail.\n", MODULE, __func__);
		return;
	}
	sysfs_attr_init(&(cfi_attr->attr));
	cfi_attr->attr.name =  __stringify(test_cfi_harden);
	cfi_attr->attr.mode = 0666;
	cfi_attr->show = about_cfi_shadow_show;
	cfi_attr->store = about_cfi_shadow_store;
	g_cfi_attrs[TEST_CFI_HARDEN] = cfi_attr;
}

extern struct kobject *prmem_kobj;
static __init int test_cfi_harden_init(void)
{
	int ret;
	struct kobject *prmem_cfi_kobj = NULL;

	prmem_cfi_kobj = kobject_create_and_add("prmem_cfi", prmem_kobj);
	if (!prmem_cfi_kobj) {
		pr_err("%s %s create prmem_cfi fail.\n", MODULE, __func__);
		return -1;
	}
	prmem_cfi_kobj->sd->mode |= 0440;
	test_cfi_attr();

	ret = sysfs_create_files(prmem_cfi_kobj,
				 (const struct attribute **)g_cfi_attrs);
	if (ret) {
		pr_err("%s %s create prmem_cfi sysfs fail\n", MODULE, __func__);
		return -1;
	}

	return 0;
}

late_initcall(test_cfi_harden_init);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
MODULE_DESCRIPTION("Test module for cfi harden feature");
