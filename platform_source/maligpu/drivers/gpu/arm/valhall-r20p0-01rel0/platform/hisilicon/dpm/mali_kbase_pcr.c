/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: This file describe HISI GPU DPM&PCR feature.
 * Author: Huawei Technologies Co., Ltd.
 * Create: 2019-10-1
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include "mali_kbase_pcr.h"
#include "mali_kbase_dpm.h"
#include "mali_kbase_config_platform.h"

static void pcr_bypass_config(struct kbase_device *kbdev, bool flag_for_bypass)
{
	if (flag_for_bypass) {
		/* top bypass config (only for mali_borr) */
		if (PCR_CFG_BYPASS_TOP_ADDR(
			kbdev->hisi_dev_data.dpm_ctrl.peri_pcr_reg) != NULL) {
			writel(PCR_BYPASS, PCR_CFG_BYPASS_TOP_ADDR(
				kbdev->hisi_dev_data.dpm_ctrl.peri_pcr_reg));
			writel(PCR_BYPASS_ENABLE, PCR_CTRL_TOP_ADDR(
				kbdev->hisi_dev_data.dpm_ctrl.peri_pcr_reg));
		}
		/* core bypass config */
		writel(PCR_BYPASS, PCR_CFG_BYPASS_CORE_ADDR(
			kbdev->hisi_dev_data.dpm_ctrl.dpmreg +
				PCR_GPU_TOP_OFFSET));
		writel(PCR_BYPASS_ENABLE, PCR_CTRL_CORE_ADDR(
			kbdev->hisi_dev_data.dpm_ctrl.dpmreg +
				PCR_GPU_TOP_OFFSET));
	} else {
		/* top no bypass config (only for mali_borr) */
		if (PCR_CFG_BYPASS_TOP_ADDR(
			kbdev->hisi_dev_data.dpm_ctrl.peri_pcr_reg) != NULL) {
			writel(PCR_NO_BYPASS_ENABLE, PCR_CTRL_TOP_ADDR(
				kbdev->hisi_dev_data.dpm_ctrl.peri_pcr_reg));
			writel(PCR_NO_BYPASS, PCR_CFG_BYPASS_TOP_ADDR(
				kbdev->hisi_dev_data.dpm_ctrl.peri_pcr_reg));
		}
		/* core no bypass config */
		writel(PCR_NO_BYPASS_ENABLE, PCR_CTRL_CORE_ADDR(
			kbdev->hisi_dev_data.dpm_ctrl.dpmreg +
				PCR_GPU_TOP_OFFSET));
		writel(PCR_NO_BYPASS, PCR_CFG_BYPASS_CORE_ADDR(
			kbdev->hisi_dev_data.dpm_ctrl.dpmreg +
				PCR_GPU_TOP_OFFSET));
	}
}

void gpu_pcr_enable(struct kbase_device *kbdev, bool flag_for_bypass)
{
	/* close gt_clk_gpu_pcr */
	gpu_set_bits(PCR_CRG_PERI_CLKEN_ADDR_MASK,
		PCR_CRG_PERI_CLKDIS_ADDR(kbdev->hisi_dev_data.crgreg));

	/* set clk_gpu_pcr frequency dividing ratio (only for mali_trym) */
	if (PCR_CRG_PERCLKDIV_ADDR(kbdev->hisi_dev_data.crgreg) != NULL)
		gpu_set_bits(PCR_CRG_PERCLKDIV_ADDR_MASK,
			PCR_CRG_PERCLKDIV_ADDR(kbdev->hisi_dev_data.crgreg));

	/* [core] set ip_rst_gpu_pcr/ip_prst_gpupcr */
	gpu_set_bits(PCR_CRG_CORE_RESET_ADDR_MASK,
		PCR_CRG_CORE_DIS_RESET_ADDR(kbdev->hisi_dev_data.crgreg));

	/*
	 * [top] set: ip_prst_peripcr/ip_rst_peripcr bit 17,18
	 * (only for mali_borr).
	 */
	if (PCR_CRG_TOP_DIS_RESET_ADDR(kbdev->hisi_dev_data.crgreg) != NULL)
		gpu_set_bits(PCR_CRG_TOP_RESET_ADDR_MASK,
			PCR_CRG_TOP_DIS_RESET_ADDR(
				kbdev->hisi_dev_data.crgreg));

	/* open gt_clk_gpu_pcr */
	gpu_set_bits(PCR_CRG_PERI_CLKEN_ADDR_MASK,
		PCR_CRG_PERI_CLKEN_ADDR(kbdev->hisi_dev_data.crgreg));

	/* [core] tpram exit shutdown mode:mask-0x0850 */
	gpu_set_bits(PCR_PCTRL_TPRAM_MEM_ADDR_EXIT_SHUTDOWN_MASK,
		PCR_PCTRL_CORE_TPRAM_MEM_ADDR(kbdev->hisi_dev_data.pctrlreg));

	/* [top] tpram exit shutdown mode:mask-0x0850 (only for mali_borr) */
	if (PCR_PCTRL_TOP_TPRAM_MEM_ADDR(kbdev->hisi_dev_data.pctrlreg) != NULL)
		gpu_set_bits(PCR_PCTRL_TPRAM_MEM_ADDR_EXIT_SHUTDOWN_MASK,
			PCR_PCTRL_TOP_TPRAM_MEM_ADDR(
				kbdev->hisi_dev_data.pctrlreg));

	writel(PCR_CFG_PERIOD0_ADDR_MASK,
		PCR_CFG_PERIOD0_ADDR(
			kbdev->hisi_dev_data.dpm_ctrl.dpmreg +
				PCR_GPU_TOP_OFFSET));

	writel(PCR_CFG_PERIOD1_ADDR_MASK,
		PCR_CFG_PERIOD1_ADDR(
			kbdev->hisi_dev_data.dpm_ctrl.dpmreg +
				PCR_GPU_TOP_OFFSET));

	pcr_bypass_config(kbdev, flag_for_bypass);

}
void gpu_pcr_disable(struct kbase_device *kbdev)
{
	/* disable top bypass (only for mali_borr) */
	if (PCR_CFG_BYPASS_TOP_ADDR(
		kbdev->hisi_dev_data.dpm_ctrl.peri_pcr_reg) != NULL) {
		writel(PCR_BYPASS_DISABLE, PCR_CFG_BYPASS_TOP_ADDR(
			kbdev->hisi_dev_data.dpm_ctrl.peri_pcr_reg));
		writel(PCR_DISABLE, PCR_CTRL_TOP_ADDR(
			kbdev->hisi_dev_data.dpm_ctrl.peri_pcr_reg));
	}
	/* disable core bypass */
	writel(PCR_BYPASS_DISABLE,
		PCR_CFG_BYPASS_CORE_ADDR(kbdev->hisi_dev_data.dpm_ctrl.dpmreg +
			PCR_GPU_TOP_OFFSET));
	writel(PCR_DISABLE,
		PCR_CTRL_CORE_ADDR(kbdev->hisi_dev_data.dpm_ctrl.dpmreg +
			PCR_GPU_TOP_OFFSET));

	/* [core] tpram set to shutdown mode:mask-0x0854 */
	gpu_set_bits(PCR_PCTRL_TPRAM_MEM_ADDR_SHUTDOWN_MASK,
		PCR_PCTRL_CORE_TPRAM_MEM_ADDR(kbdev->hisi_dev_data.pctrlreg));

	/* [top] tpram set to shutdown mode:mask-0x0854 (only for mali_borr) */
	if (PCR_PCTRL_TOP_TPRAM_MEM_ADDR(kbdev->hisi_dev_data.pctrlreg) != NULL)
		gpu_set_bits(PCR_PCTRL_TPRAM_MEM_ADDR_SHUTDOWN_MASK,
			PCR_PCTRL_TOP_TPRAM_MEM_ADDR(
				kbdev->hisi_dev_data.pctrlreg));

	/* [core] reset:0x09C ip_rst_gpu_pcr/ip_prst_gpupcr */
	gpu_set_bits(PCR_CRG_CORE_RESET_ADDR_MASK,
		PCR_CRG_CORE_EN_RESET_ADDR(kbdev->hisi_dev_data.crgreg));

	/*
	 * [top] reset:0x078 ip_prst_peripcr/ip_rst_peripcr
	 * (only for mali_borr)
	 */
	if (PCR_CRG_TOP_EN_RESET_ADDR(kbdev->hisi_dev_data.crgreg) != NULL)
		gpu_set_bits(PCR_CRG_TOP_RESET_ADDR_MASK,
			PCR_CRG_TOP_EN_RESET_ADDR(
				kbdev->hisi_dev_data.crgreg));

	gpu_set_bits(PCR_CRG_PERI_CLKEN_ADDR_MASK,
		PCR_CRG_PERI_CLKDIS_ADDR(kbdev->hisi_dev_data.crgreg));
}
