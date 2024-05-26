/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/arm-smccc.h>
#include <linux/delay.h>

#include <soc_dte_define.h>
#include <dpu/soc_dpu_dacc_img_v900.h>

#include "hisi_dacc_im_array.h"
#include "hisi_dacc_dm_array.h"
#include "hisi_disp_gadgets.h"
#include "hisi_operator_tool.h"
#include "hisi_disp.h"

#if defined(CONFIG_DRMDRIVER)
#include <platform_include/display/linux/dpu_drmdriver.h>
#endif

enum DACCFW_LOAD_FLAG {
	DPU_LOAD_DACCFW_FROM_BL2,
	DPU_LOAD_DACCFW_FROM_BL31,
	DPU_LOAD_DACCFW_FROM_KERNEL,
};

static uint32_t g_daccfw_load_flag = DPU_LOAD_DACCFW_FROM_BL31;

#define BL2_DISPLAY_DACC_LOAD 0xC60001B4
#define DACC_CRG_CTL_CLEAR_TIMEOUT_THR 17

uint32_t dpu_set_load_daccfw_flag(uint32_t flag)
{
	disp_pr_info("set flag to %d\n", flag);
	g_daccfw_load_flag = flag;

	return 0;
}
EXPORT_SYMBOL_GPL(dpu_set_load_daccfw_flag);

void hisi_disp_dacc_scene_config(unsigned scene_id, bool enable_cmdlist, char __iomem *dpu_base,
	struct hisi_display_frame *frame)
{
	char __iomem *dmc_base = dpu_base + DACC_OFFSET + DMC_OFFSET;

	dpu_set_reg(SOC_DACC_CLK_SEL_REG_ADDR(dmc_base, scene_id), 1, 1, 0);
	dpu_set_reg(SOC_DACC_CLK_EN_REG_ADDR(dmc_base, scene_id), 1, 1, 0);
	dpu_set_reg(SOC_DACC_CTL_CFG_MODE0_ADDR(dmc_base, scene_id), (uint32_t)enable_cmdlist, 1, 0);

	if (is_offline_scene(scene_id)) {
		if (frame->pre_pipelines[0].src_layer.src_type == LAYER_SRC_TYPE_LOCAL)
			dpu_set_reg(SOC_DACC_CTL_SW_START_REG_ADDR(dmc_base, scene_id), 0x1, 1, 0);
		else
			disp_pr_info("RX DSD not need config ctl_sw_start.\n");
	} else {
		 dpu_set_reg(SOC_DACC_CTL_ST_FRM_SEL0_REG_ADDR(dmc_base, scene_id), 0, 1, 0);
	}
}

static void dpu_load_daccfw_from_bl2(void)
{
	struct arm_smccc_res res = {0};

	disp_pr_info("load daccfw from BL2\n");

	arm_smccc_smc(BL2_DISPLAY_DACC_LOAD, 0, 0, 0, 0, 0, 0, 0, &res);
}

static void dpu_load_daccfw_from_bl31(void)
{
	disp_pr_info("load daccfw from BL31\n");

#if defined(CONFIG_DRMDRIVER)
	/* DSS_CH_DEFAULT_SEC_CONFIG: send cmd to bl31 for dacc load */
	configure_dss_service_security(DSS_CH_DEFAULT_SEC_CONFIG, 0, 0);
#endif
}

static void dpu_load_daccfw_from_kernel(char __iomem *dpu_base)
{
	int i;
	disp_pr_info("load daccfw from kernel\n");

	for (i = 0; i < dacc_rv32im_v900_bin_len / 4; i++)
		outp32(dpu_base + DACC_OFFSET + DACC_IM_OFFSET + i * 4, *((uint32_t*)(dacc_rv32im_v900_bin + i * 4)));
}


static void dpu_dump_dacc_regs(char __iomem *dpu_base)
{
	char __iomem *reg_addr;
	uint32_t reg_val[8];
	int i, j;

	reg_addr = dpu_base + DACC_OFFSET + DM_DPP_INITAIL_ST_ADDR + 0x490; // 0x490 for offline scene 0

	disp_pr_info("==================================dump dacc regs=================================\n");
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 8; j++) {
			reg_val[j] = inp32(reg_addr + i * 128 + j * 4);
		}
		disp_pr_info("0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
			reg_val[0], reg_val[1], reg_val[2], reg_val[3], reg_val[4], reg_val[5], reg_val[6], reg_val[7]);
	}
}

static uint32_t dpu_dacc_check_status(char __iomem *dpu_base)
{
	char __iomem *dmc_base;
	uint32_t dacc_int_status;
	uint32_t dacc_dbg_status;
	uint32_t dacc_core_status;
	uint32_t dacc_core_cfg;
	uint32_t dacc_debug_priv;

	dmc_base = dpu_base + DACC_OFFSET + DMC_OFFSET;

	dacc_debug_priv = inp32(SOC_DACC_WLT_SLICE3_STRIDE_ADDR(dmc_base));
	dacc_int_status = inp32(SOC_DACC_SW_EXT_INT_REG_ADDR(dmc_base));
	dacc_dbg_status = inp32(SOC_DACC_DBG_SEL_REG_ADDR(dmc_base));
	dacc_core_status = inp32(SOC_DACC_CORE_STATUS_REG_ADDR(dmc_base));
	dacc_core_cfg = inp32(SOC_DACC_CORE_CONFIG_REG_ADDR(dmc_base));
	disp_pr_info("DACC STATUS: priv_dbg(%08x), sw_int(%02X), dgb(%02X), core_status(%02X), core_cfg(%02X)\n",
		dacc_debug_priv, dacc_int_status, dacc_dbg_status, dacc_core_status, dacc_core_cfg);

	dpu_dump_dacc_regs(dpu_base);

	return (dacc_int_status || dacc_dbg_status);
}

static void dpu_dacc_clear_DM(char __iomem *dpu_base)
{
	uint32_t i;
	char __iomem *dm_base;

	disp_pr_info("------------clear DM space-------------\n");
	dm_base = dpu_base + DACC_OFFSET + DACC_DM_OFFSET;
	for (i = 0; i < 0x9000 / 4; i++)
		dpu_set_reg(dm_base + i * 4, 0, 32, 0);

}

static void dpu_load_daccfw(char __iomem *dpu_base)
{
	uint32_t dacc_state;
	char __iomem *dmc_base;

	dmc_base = dpu_base + DACC_OFFSET + DMC_OFFSET;

	disp_pr_info(" +++++++ \n");
	dacc_state = dpu_dacc_check_status(dpu_base);

	if (dacc_state != 0) {
		disp_pr_info("DACC already on, skip fw loading\n");
		return;
	}

	// FIXME: this is a work-around, avoid smmu-error ocurred during scene switching from
	// normal-online to rx2tx-online.
	dpu_dacc_clear_DM(dpu_base);

	dpu_set_reg(SOC_DACC_CORE_CONFIG_REG_ADDR(dmc_base), 1, 1, 4);

	if (g_daccfw_load_flag == DPU_LOAD_DACCFW_FROM_BL2)
		dpu_load_daccfw_from_bl2();
	else if (g_daccfw_load_flag == DPU_LOAD_DACCFW_FROM_BL31)
		dpu_load_daccfw_from_bl31();
	else
		dpu_load_daccfw_from_kernel(dpu_base);

	dpu_dacc_check_status(dpu_base);

	dpu_set_reg(SOC_DACC_POR_RESET_PC_REG_ADDR(dmc_base), 0, 32, 0);
	dpu_set_reg(SOC_DACC_CORE_CONFIG_REG_ADDR(dmc_base), 0, 1, 4);

	dpu_dacc_check_status(dpu_base);
	disp_pr_info(" ----- \n");
}

void hisi_disp_dacc_init(char __iomem *dpu_base)
{
	disp_pr_info(" +++++++ \n");
	dpu_load_daccfw(dpu_base);
	disp_pr_info(" ----- \n");
}
