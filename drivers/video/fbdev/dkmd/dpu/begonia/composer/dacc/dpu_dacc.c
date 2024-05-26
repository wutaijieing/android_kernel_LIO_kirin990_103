/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <dpu/soc_dpu_define.h>
#include "dpu_comp_config_utils.h"
#include "peri/dkmd_peri.h"
#include "dpu_dacc.h"
#include "dpu_cmdlist.h"
#if defined(CONFIG_DRMDRIVER)
#include <platform_include/display/linux/dpu_drmdriver.h>
#endif

#define DACC_CRG_CTL_CLEAR_TIMEOUT_THR (17) // 16ms

void dpu_dacc_load(void)
{
#if defined(CONFIG_DRMDRIVER)
	/* DSS_CH_DEFAULT_SEC_CONFIG: send cmd to bl31 for dacc load */
	configure_dss_service_security(DSS_CH_DEFAULT_SEC_CONFIG, 0, 0);
#endif
}

void dpu_dacc_config_scene(char __iomem *dpu_base, uint32_t scene_id, bool enable_cmdlist)
{
	char __iomem *dmc_base = dpu_base + DACC_OFFSET + DMC_OFFSET;

	set_reg(SOC_DACC_CLK_SEL_REG_ADDR(dmc_base, scene_id), 1, 1, 0);
	set_reg(SOC_DACC_CLK_EN_REG_ADDR(dmc_base, scene_id), 1, 1, 0);
	set_reg(SOC_DACC_CTL_CFG_MODE0_ADDR(dmc_base, scene_id), (uint32_t)enable_cmdlist, 1, 0);

	if (is_offline_scene(scene_id))
		set_reg(SOC_DACC_CTL_SW_START_REG_ADDR(dmc_base, scene_id), 1, 1, 0);
	else
		set_reg(SOC_DACC_CTL_ST_FRM_SEL0_REG_ADDR(dmc_base, scene_id), 0, 1, 0);
}

uint32_t dpu_dacc_handle_clear(char __iomem *dpu_base, uint32_t scene_id)
{
	uint32_t ret = 0;
	char __iomem *module_base = dpu_base + DACC_OFFSET + DMC_OFFSET;

	outp32(SOC_DACC_CLR_RISCV_START_ADDR(module_base, scene_id), 1);
	dpu_pr_info("dacc clear for scene_id=%d, CLR_START_REG=%#x!",
		scene_id, inp32(SOC_DACC_CLR_START_REG_ADDR(module_base, scene_id)));

	/* 3. wait dacc exec clear, check dacc_crg clear_timeout_status_reg */
	if (!check_addr_status_is_valid(SOC_DACC_CLR_TIMEOUT_STATUS_REG_ADDR(module_base, scene_id),
		0b11, 1000, DACC_CRG_CTL_CLEAR_TIMEOUT_THR)) {
		dpu_pr_info("dacc clear timeout, need reset dpu! "
			"MCLEAR_CTRL=%#x MCLEAR_CLEAR_ST=%#x MCLEAR_CLEAR_IP_ST=%#x CLR_START_REG=%#x",
			inp32(DPU_GLB_MCLEAR_CTRL_ADDR(dpu_base + DPU_GLB0_OFFSET)),
			inp32(DPU_GLB_MCLEAR_CLEAR_ST_ADDR(dpu_base + DPU_GLB0_OFFSET)),
			inp32(DPU_GLB_MCLEAR_CLEAR_IP_ST_ADDR(dpu_base + DPU_GLB0_OFFSET)),
			inp32(SOC_DACC_CLR_START_REG_ADDR(module_base, scene_id)));
		ret = BIT(scene_id);
	}
	outp32(SOC_DACC_CLR_TIMEOUT_STATUS_REG_ADDR(module_base, scene_id), 0b11);
	outp32(SOC_DACC_CLR_START_REG_ADDR(module_base, scene_id), 0);

	return ret;
}
