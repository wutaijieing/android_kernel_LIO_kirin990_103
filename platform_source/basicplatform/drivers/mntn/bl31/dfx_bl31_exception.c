/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: BL31 exception driver
 */

#include "dfx_bl31_exception.h"
#include <linux/arm-smccc.h>
#include <global_ddr_map.h>
#include <soc_acpu_baseaddr_interface.h>
#include <soc_sctrl_interface.h>
#include <linux/compiler.h>
#include <linux/debugfs.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <platform_include/basicplatform/linux/util.h>
#include <platform_include/basicplatform/linux/rdr_platform.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include "../blackbox/rdr_print.h"

static void *sctrl_base = NULL;

u32 get_bl31_exception_flag(void)
{
	u32 val = 0;

	if (sctrl_base)
		val = readl(SOC_SCTRL_SCBAKDATA5_ADDR(sctrl_base));

	return val;
}

void bl31_panic_ipi_handle(void)
{
	pr_info("bl31 panic handler in kernel\n");
	rdr_syserr_process_for_ap((u32)MODID_AP_S_BL31_PANIC, 0ULL, 0ULL);
}

noinline u64 atfd_dfx_service_bl31_dbg_smc(u64 _function_id, u64 _arg0, u64 _arg1, u64 _arg2)
{
	struct arm_smccc_res res;

	arm_smccc_smc(_function_id, _arg0, _arg1, _arg2, 0, 0, 0, 0, &res);

	return res.a0;
}

#ifdef CONFIG_DFX_DEBUG_FS
static int bl31_panic_debug_show(struct seq_file *s, void *unused)
{
	u64 ret;

	ret = atfd_dfx_service_bl31_dbg_smc((u64)BL31_DEBUG_PANIC_VAL, 0ULL, 0ULL, 0ULL);
	if (!ret)
		return -EPERM;

	return 0;
}

static int bl31_panic_debug_open(struct inode *inode, struct file *file)
{
	if (!inode || !file)
		return -EPERM;

	return single_open(file, bl31_panic_debug_show, &inode->i_private);
}

static const struct file_operations bl31_panic_debug_fops = {
	.owner = THIS_MODULE,
	.open = bl31_panic_debug_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int bl31_create_file(void)
{
	struct dentry *g_bl31_dir = NULL;

	g_bl31_dir = debugfs_create_dir("bl31_mntn", NULL);
	if (g_bl31_dir) {
		if (debugfs_create_file("bl31_panic_trigger", S_IRUGO | S_IWUSR | S_IWGRP, g_bl31_dir,
			NULL, &bl31_panic_debug_fops) == NULL) {
			debugfs_remove_recursive(g_bl31_dir);
			return -EPERM;
		}
	} else {
		return -EPERM;
	}

	return 0;
}
#endif

static int __init dfx_bl31_panic_init(void)
{
	struct rdr_exception_info_s einfo;
	struct device_node *np = NULL;
	uint32_t data[2] = { 0 };
	phys_addr_t bl31_smem_base;
	void *bl31_ctrl_addr = NULL;
	void *bl31_ctrl_addr_phys = NULL;
	s32 ret;

	sctrl_base = (void *)ioremap((phys_addr_t) SOC_ACPU_SCTRL_BASE_ADDR, SCTRL_REG_SIZE);
	if (!sctrl_base) {
		BB_PRINT_ERR("sctrl ioremap faild\n");
		return -EPERM;
	}

	bl31_smem_base = ATF_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE;
	np = of_find_compatible_node(NULL, NULL, "hisilicon, bl31_mntn");
	if (!np) {
		BB_PRINT_ERR("%s: no compatible node found\n", __func__);
		goto iounmap_sctrl;
	}
	/* read bl31 crtl addr ande size from dts , 2 is addr and size count */
	ret = of_property_read_u32_array(np, "dfx,bl31-share-mem", &data[0], 2UL);
	if (ret) {
		BB_PRINT_ERR("%s , get val mem compatible node err\n", __func__);
		goto iounmap_sctrl;
	}
	/* data[0] is bl31 ctrl address , data[1] is reg size */
	bl31_ctrl_addr_phys = (void *)(uintptr_t)(bl31_smem_base + data[0]);
	bl31_ctrl_addr = (void *)ioremap(bl31_smem_base + data[0], (u64)data[1]);
	if (!bl31_ctrl_addr) {
		BB_PRINT_ERR
		    ("%s: %d: allocate memory for bl31_ctrl_addr failed\n",
		     __func__, __LINE__);
		goto iounmap_sctrl;
	}

#ifdef BL31_SWITCH
	if (check_mntn_switch(HIMNTN_BL31)) {
		BB_PRINT_PN("%s, MNTN_BL31 is closed!\n", __func__);
		writel(0x0, bl31_ctrl_addr);
		goto iounmap_bl31_ctrl;
	}
#endif

	/* register rdr exception type */
	memset((void *)&einfo, 0, sizeof(einfo));
	einfo.e_modid = MODID_AP_S_BL31_PANIC;
	einfo.e_modid_end = MODID_AP_S_BL31_PANIC;
	einfo.e_process_priority = RDR_ERR;
	einfo.e_reboot_priority = RDR_REBOOT_NOW;
	einfo.e_notify_core_mask = RDR_AP;
	einfo.e_reset_core_mask = RDR_AP;
	einfo.e_reentrant = (u32) RDR_REENTRANT_DISALLOW;
	einfo.e_exce_type = AP_S_BL31_PANIC;
	einfo.e_from_core = RDR_AP;
	memcpy((void *)einfo.e_from_module, (const void *)"RDR BL31 PANIC", sizeof("RDR BL31 PANIC"));
	memcpy((void *)einfo.e_desc, (const void *)"RDR BL31 PANIC", sizeof("RDR BL31 PANIC"));
	ret = (s32)rdr_register_exception(&einfo);
	if (!ret) {
		BB_PRINT_ERR("register bl31 exception fail\n");
		iounmap(bl31_ctrl_addr);
		goto iounmap_sctrl;
	}

#ifdef CONFIG_DFX_DEBUG_FS
	if (bl31_create_file() != 0)
		BB_PRINT_ERR("create bl31_panic_trigger node faild\n");
#endif

	/* enable bl31 switch:route to kernel */
	writel((u32)0x1, bl31_ctrl_addr);
	iounmap(bl31_ctrl_addr);
	goto succ;
#ifdef BL31_SWITCH
iounmap_bl31_ctrl:
	iounmap(bl31_ctrl_addr);
#endif

iounmap_sctrl:
	iounmap(sctrl_base);
	return -EPERM;
succ:
	return 0;
}

module_init(dfx_bl31_panic_init);
MODULE_LICENSE("GPL");
