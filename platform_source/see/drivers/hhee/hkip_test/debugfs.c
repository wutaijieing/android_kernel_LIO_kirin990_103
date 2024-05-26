/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
 * Description: HKIP DEBUGFS
 * Create : 2017/9/2
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/debugfs.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
#include <asm/compiler.h>
#endif
#include <linux/mm.h>
#include <linux/lsm_hooks.h>
#include "hkip.h"

extern int nr_interrupts;

#define DEFINE_SYSREG_EL1_ATTRIBUTE(name)                                      \
static int sysreg_##name##_el1_get(void *data, u64 *val)                       \
{                                                                              \
	u64 r;                                                                 \
	asm volatile("mrs %0, " #name "_el1\n" : "=r"(r));                     \
	*val = r;                                                              \
	return 0;                                                              \
}                                                                              \
                                                                               \
static int sysreg_##name##_el1_set(void *data, u64 val)                        \
{                                                                              \
	asm volatile("msr " #name "_el1, %0\n" : : "r"(val));                  \
	return 0;                                                              \
}                                                                              \
DEFINE_SIMPLE_ATTRIBUTE(fops_sysreg_##name##_el1,                              \
			sysreg_##name##_el1_get,                               \
			sysreg_##name##_el1_set, "0x%016llx\n")

#define DEFINE_SYSREG_EL1_ATTRIBUTE_RO(name)                                   \
static int sysreg_##name##_el1_get(void *data, u64 *val)                       \
{                                                                              \
	u64 r;                                                                 \
	asm volatile("mrs %0, " #name "_el1\n" : "=r"(r));                     \
	*val = r;                                                              \
	return 0;                                                              \
}                                                                              \
                                                                               \
DEFINE_SIMPLE_ATTRIBUTE(fops_sysreg_##name##_el1,                              \
			sysreg_##name##_el1_get, NULL, "0x%016llx\n")

DEFINE_SYSREG_EL1_ATTRIBUTE(mair);
DEFINE_SYSREG_EL1_ATTRIBUTE(sctlr);
DEFINE_SYSREG_EL1_ATTRIBUTE(tcr);
DEFINE_SYSREG_EL1_ATTRIBUTE(ttbr0);
DEFINE_SYSREG_EL1_ATTRIBUTE(ttbr1);
DEFINE_SYSREG_EL1_ATTRIBUTE(esr);
DEFINE_SYSREG_EL1_ATTRIBUTE(far);
DEFINE_SYSREG_EL1_ATTRIBUTE(contextidr);
DEFINE_SYSREG_EL1_ATTRIBUTE(afsr0);
DEFINE_SYSREG_EL1_ATTRIBUTE(afsr1);
DEFINE_SYSREG_EL1_ATTRIBUTE(amair);
DEFINE_SYSREG_EL1_ATTRIBUTE_RO(id_aa64mmfr1);

static u64 hkip_get_version(void)
{
	u64 major = HKIP_HVC_VERSION;
	u64 minor = 0;

	hkip_hvc_2(HKIP_HVC_VERSION, &major, &minor, 0, 0, 0);
	return (major << 32) | (minor & 0xFFFFFFFF);
}

static int hvreg_get(void *data, u64 *val)
{
	uintptr_t hvc_id = (uintptr_t)data;

	if (hvc_id == HKIP_HVC_VERSION)
		*val = hkip_get_version();
	else if (hvc_id == HKIP_MONITORLOG_RESET_COUNTERS)
		hkip_clean_counters();
	else
		*val = hkip_hvc(hvc_id, 0, 0, 0, 0, 0);
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(fops_hvreg, hvreg_get, NULL, "0x%016llx\n");

static struct dentry *__init debugfs_create_hvreg(const char *name,
						  struct dentry *parent,
						  uintptr_t fid)
{
	return debugfs_create_file(name, 0400, parent, (void *)fid,
				   &fops_hvreg);
}

enum kconf {
	KCONF_AFDBM = 1,
};

static int kconf_get(void *data, u64 *val)
{
	enum kconf conf = (enum kconf)(uintptr_t)data;

	switch (conf) {
	case KCONF_AFDBM:
#ifdef CONFIG_ARM64_HW_AFDBM
		*val = 1;
#else
		*val = 0;
#endif
		break;
	default:
		return 1;
	}
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_kconf, kconf_get, NULL, "0x%016llx\n");

static struct dentry *__init debugfs_create_conf_file(const char *name,
						      struct dentry *parent,
						      enum kconf conf)
{
	return debugfs_create_file(name, 0400, parent, (void *)conf,
				   &fops_kconf);
}

static int hvc_set(void *data, u64 val)
{
	uint64_t tmp;

	printk(KERN_ALERT "HVC invoke fn: 0x%016llx\n", val);
	tmp = hkip_hvc(val, 0, 0, 0, 0, 0);
	printk(KERN_ALERT "HVC fn(0x%016llx) = 0x%016llx\n", val, tmp);
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(fops_hvc, NULL, hvc_set, "0x%016llx\n");

static int get_interrupt_count(void *data, u64 *val)
{
	*val = nr_interrupts;
	nr_interrupts = 0;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_nr_interrupts, get_interrupt_count, NULL,
			"0x%016llx\n");

static u64 el1_addr;

static int el1_addr_get(void *data, u64 *val)
{
	*val = el1_addr;
	return 0;
}

static int el1_addr_set(void *data, u64 val)
{
	/* check if address is actually a valid kernel address */
	el1_addr = val;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_el1_addr, el1_addr_get, el1_addr_set, "0x%016llx\n");

static int el1_get_doubleword(void *data, u64 *val)
{
	if (el1_addr_invalid(el1_addr, 0)) {
		pr_err("Invalid kernel address\n");
		return -EINVAL;
	}

	*val = *((u64 *)(uintptr_t)el1_addr);
	return 0;
}

static int el1_addr_set_doubleword(void *data, u64 val)
{
	if (el1_addr_invalid(el1_addr, 1)) {
		pr_err("Invalid kernel address\n");
		return -EINVAL;
	}

	*((u64 *)(uintptr_t)el1_addr) = val;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_el1_doubleword_access, el1_get_doubleword,
			el1_addr_set_doubleword, "0x%016llx\n");

static int el1_var_addr_get(void *data, u64 *val)
{
	*val = (u64)(uintptr_t)&el1_addr;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_el1_var_addr, el1_var_addr_get, NULL, "0x%016llx\n");

static u64 __attribute__((aligned(1024), used)) el1_sandbox[1024];

static int el1_sandbox_addr_get(void *data, u64 *val)
{
	*val = (u64)(uintptr_t)el1_sandbox;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_el1_sandbox_addr, el1_sandbox_addr_get, NULL, "0x%016llx\n");

static int el1_func_addr_get(void *data, u64 *val)
{
	*val = (u64)(uintptr_t)&el1_text_sandbox;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_el1_func_addr, el1_func_addr_get, NULL, "0x%016llx\n");

static int el1_rosym_addr_get(void *data, u64 *val)
{
	*val = (u64)(uintptr_t)linux_banner;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_el1_rosym_addr, el1_rosym_addr_get, NULL, "0x%016llx\n");

extern struct security_hook_list hkipt_lsm_hooks[];
static int lsm_hook_addr_get(void *data, u64 *val)
{
	*val = (u64)(uintptr_t)hkipt_lsm_hooks;
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(fops_lsm_hook_addr, lsm_hook_addr_get, NULL, "0x%016llx\n");

#define PAR_PA(x) ((((x) & ((1ULL << 48) - 1)) >> 12) << 12)

static u64 el1_va;
static int el1_ipa_read(void *data, u64 *val)
{
	u64 par;

	asm volatile("at s1e1r, %0" : : "r"(el1_va));

	isb();

	par = read_sysreg(par_el1);
	if (unlikely(par & 1)) {
		pr_err("Translation failed. PAR_EL1: 0x%016llx\n", par);
		return -EINVAL; /* Translation failed */
	}

	*val = PAR_PA(par) | (el1_va & ((1 << 12) - 1));
	return 0;
}

static int el1_va_set(void *data, u64 val)
{
	el1_va = val;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_el1_ipa, el1_ipa_read, el1_va_set, "0x%016llx\n");

static int id_get(void *data, u64 *val)
{
	u32 id_offset = (u32)(uintptr_t)data;
	const struct cred *cred = current_cred();

	if (!cred)
		return -ENODEV;

	*val = *(u32 *)(((u8 *)cred) + id_offset);
	return 0;
}

static int id_set(void *data, u64 val)
{
	u32 id_offset = (u32)(uintptr_t)data;
	const struct cred *cred = (struct cred *)current_cred();

	if (val > UINT_MAX)
		return -EINVAL;

	if (!cred)
		return -ENODEV;

	*(u32 *)(((u8 *)cred) + id_offset) = val;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_id, id_get, id_set, "%llu\n");

static int addr_limit_get(void *data, u64 *val)
{
	mm_segment_t limit = current_thread_info()->addr_limit;

	*val = limit == KERNEL_DS ? 1 : 0;
	return 0;
}

static int addr_limit_set(void *data, u64 val)
{
	mm_segment_t *limit = &current_thread_info()->addr_limit;

	*limit = val ? KERNEL_DS : USER_DS;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_addr_limit, addr_limit_get, addr_limit_set, "%llu\n");

static int noop_get(void *data, u64 *val)
{
	return 0;
}

static int noop_set(void *data, u64 val)
{
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(fops_noop, noop_get, noop_set, "0x%016llx\n");

static struct dentry *__init debugfs_create_id(const char *name,
					       struct dentry *parent,
					       uintptr_t offset)
{
	return debugfs_create_file(name, 0666, parent, (void *)offset,
				   &fops_id);
}
static int debugfs_create_reg_file(struct dentry *dir)
{
	if (debugfs_create_file("mair_el1", 0600, dir, NULL,
				&fops_sysreg_mair_el1) == NULL)
		return -1;
	if (debugfs_create_file("sctlr_el1", 0600, dir, NULL,
				&fops_sysreg_sctlr_el1) == NULL)
		return -1;
	if (debugfs_create_file("tcr_el1", 0600, dir, NULL,
				&fops_sysreg_tcr_el1) == NULL)
		return -1;
	if (debugfs_create_file("ttbr0_el1", 0600, dir, NULL,
				&fops_sysreg_ttbr0_el1) == NULL)
		return -1;
	if (debugfs_create_file("ttbr1_el1", 0600, dir, NULL,
				&fops_sysreg_ttbr1_el1) == NULL)
		return -1;
	if (debugfs_create_file("esr_el1", 0600, dir, NULL,
				&fops_sysreg_esr_el1) == NULL)
		return -1;
	if (debugfs_create_file("far_el1", 0600, dir, NULL,
				&fops_sysreg_far_el1) == NULL)
		return -1;
	if (debugfs_create_file("contextidr_el1", 0600, dir, NULL,
				&fops_sysreg_contextidr_el1) == NULL)
		return -1;
	if (debugfs_create_file("afsr0_el1", 0600, dir, NULL,
				&fops_sysreg_afsr0_el1) == NULL)
		return -1;
	if (debugfs_create_file("afsr1_el1", 0600, dir, NULL,
				&fops_sysreg_afsr1_el1) == NULL)
		return -1;
	if (debugfs_create_file("amair_el1", 0600, dir, NULL,
				&fops_sysreg_amair_el1) == NULL)
		return -1;
	if (debugfs_create_file("id_aa64mmfr1_el1", 0600, dir, NULL,
				&fops_sysreg_id_aa64mmfr1_el1) == NULL)
		return -1;
	return 0;
}

static int debugfs_create_hvc_hvreg(struct dentry *dir)
{
	if (debugfs_create_hvreg("hcr_el2", dir, HKIP_HCR_EL2) == NULL)
		return -1;
	if (debugfs_create_hvreg("vttbr_el2", dir, HKIP_VTTBR_EL2) == NULL)
		return -1;
	if (debugfs_create_hvreg("version", dir, HKIP_HVC_VERSION) == NULL)
		return -1;
	if (debugfs_create_hvreg("notify", dir, HKIP_HVC_NOTIFY) == NULL)
		return -1;
	if (debugfs_create_hvreg("permissive", dir, HKIP_PERMISSIVE) == NULL)
		return -1;
	if (debugfs_create_hvreg("reset_event_counters", dir,
				 HKIP_MONITORLOG_RESET_COUNTERS) == NULL)
		return -1;
	if (debugfs_create_hvreg("el2_func_addr", dir,
				 HKIP_RETURN_FUNCTION_ADDRESS) == NULL)
		return -1;
	if (debugfs_create_hvreg("el2_var_addr", dir,
				 HKIP_RETURN_VAR_ADDRESS) == NULL)
		return -1;
	if (debugfs_create_hvreg("unknown_hvc", dir, HKIP_UNKNOWN_HVC) == NULL)
		return -1;
	return 0;
}

static int debugfs_create_el1_file(struct dentry *dir)
{
	if (debugfs_create_file("hvc", 0600, dir, NULL, &fops_hvc) == NULL)
		return -1;

	if (debugfs_create_file("interrupt_count", 0400, dir, NULL,
				&fops_nr_interrupts) == NULL)
		return -1;

	if (debugfs_create_file("el1_addr", 0600, dir, NULL, &fops_el1_addr) ==
	    NULL)
		return -1;

	if (debugfs_create_file("el1_doubleword", 0600, dir, NULL,
				&fops_el1_doubleword_access) == NULL)
		return -1;

	if (debugfs_create_file("el1_var_addr", 0400, dir, NULL,
				&fops_el1_var_addr) == NULL)
		return -1;

	if (debugfs_create_file("el1_sandbox_addr", 0400, dir, NULL,
				&fops_el1_sandbox_addr) == NULL)
		return -1;

	if (debugfs_create_file("el1_func_addr", 0400, dir, NULL,
				&fops_el1_func_addr) == NULL)
		return -1;

	if (debugfs_create_file("el1_rosym_addr", 0400, dir, NULL,
				&fops_el1_rosym_addr) == NULL)
		return -1;

	if (debugfs_create_file("hkipt_lsm_hook_addr", 0400, dir, NULL,
				&fops_lsm_hook_addr) == NULL)
		return -1;

	if (debugfs_create_file("el1_ipa", 0600, dir, NULL, &fops_el1_ipa) ==
	    NULL)
		return -1;

	return 0;
}

static int debugfs_create_acc_file(struct dentry *dir)
{
	if (debugfs_create_file("acc_u", 0600, dir, NULL, &fops_noop) == NULL)
		return -1;
	if (debugfs_create_file("acc_g", 0060, dir, NULL, &fops_noop) == NULL)
		return -1;
	if (debugfs_create_file("acc_ug", 0660, dir, NULL, &fops_noop) == NULL)
		return -1;
#ifdef CONFIG_HKIP_ADDR_LIMIT_PROTECTION
	if (debugfs_create_file("addr_limit", 0600, dir, NULL,
				&fops_addr_limit) == NULL)
		return -1;
#endif
	return 0;
}

static int debugfs_create_test_id(struct dentry *dir)
{
	if (debugfs_create_id("uid", dir, offsetof(struct cred, uid)) == NULL)
		return -1;
	if (debugfs_create_id("gid", dir, offsetof(struct cred, gid)) == NULL)
		return -1;
	if (debugfs_create_id("suid", dir, offsetof(struct cred, suid)) == NULL)
		return -1;
	if (debugfs_create_id("sgid", dir, offsetof(struct cred, sgid)) == NULL)
		return -1;
	if (debugfs_create_id("euid", dir, offsetof(struct cred, euid)) == NULL)
		return -1;
	if (debugfs_create_id("egid", dir, offsetof(struct cred, egid)) == NULL)
		return -1;
	if (debugfs_create_id("fsuid", dir, offsetof(struct cred, fsuid)) == NULL)
		return -1;
	if (debugfs_create_id("fsgid", dir, offsetof(struct cred, fsgid)) == NULL)
		return -1;
	return 0;
}

static struct dentry *__init hkip_init_debugfs(void)
{
	struct dentry *dir = NULL;
	struct dentry *conf_dir;

	if (HHEE_DISABLE == hhee_check_enable())
		return NULL;

	dir = debugfs_create_dir("hkip", NULL);
	if (dir == NULL)
		return NULL;

	conf_dir = debugfs_create_dir("kconfig", dir);
	if (conf_dir == NULL)
		goto error;

	if (debugfs_create_reg_file(dir))
		goto error;

	if (debugfs_create_hvc_hvreg(dir))
		goto error;

	if (debugfs_create_el1_file(dir))
		goto error;

	if (debugfs_create_acc_file(dir))
		goto error;

	if (debugfs_create_test_id(dir))
		goto error;

	if (debugfs_create_conf_file("afdbm", conf_dir, KCONF_AFDBM) == NULL)
		goto error;
	return dir;

error:
	debugfs_remove_recursive(dir);
	printk(KERN_ERR "HKIP debugfs init failed\n");
	return NULL;
}

struct dentry *debug_dir;

int __init hkip_test_init(void)
{
	debug_dir = hkip_init_debugfs();
	if (debug_dir == NULL)
		return -ENOMEM;
	return 0;
}

void __exit hkip_test_exit(void)
{
	debugfs_remove_recursive(debug_dir);
}

module_init(hkip_test_init);
module_exit(hkip_test_exit);
MODULE_LICENSE("GPL");
