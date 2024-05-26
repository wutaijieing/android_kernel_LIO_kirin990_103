/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include "dpu_smmu_base.h"
#include "dpu_fb_struct.h"
#include "dpu_fb.h"
#include "dpu_iommu.h"

void dpu_smmu_init(const char __iomem *smmu_base, dss_smmu_t *s_smmu)
{
	void_unused(smmu_base);
	if (!s_smmu)
		return;

	memset(s_smmu, 0, sizeof(*s_smmu));
}

static void dpu_smmu_scr_config(char __iomem *smmu_base)
{
	set_reg(smmu_base + SMMU_SCR, 0x0, 1, 0);
	set_reg(smmu_base + SMMU_SCR, 0x1, 8, 20);
	set_reg(smmu_base + SMMU_SCR, g_dss_smmu_outstanding - 1, 4, 16);
	set_reg(smmu_base + SMMU_SCR, 0x7, 3, 3);
	set_reg(smmu_base + SMMU_LP_CTRL, 0x1, 1, 0);

	/* Long Descriptor */
	set_reg(smmu_base + SMMU_CB_TTBCR, 0x1, 1, 0);
}

static void dpu_smmu_rwerr_addr_config(char __iomem *smmu_base,
	bool is_mdc)
{
	uint64_t smmu_rwerraddr_phys;

	if (!g_smmu_rwerraddr_virt) {
		set_reg(smmu_base + SMMU_ERR_RDADDR, 0x7FF00000, 32, 0);
		set_reg(smmu_base + SMMU_ERR_WRADDR, 0x7FFF0000, 32, 0);
		return;
	}

	smmu_rwerraddr_phys = virt_to_phys(g_smmu_rwerraddr_virt);
	set_reg(smmu_base + SMMU_ERR_RDADDR,
		(uint32_t)(smmu_rwerraddr_phys & 0xFFFFFFFF), 32, 0);
	set_reg(smmu_base + SMMU_ERR_WRADDR,
		(uint32_t)(smmu_rwerraddr_phys & 0xFFFFFFFF), 32, 0);

	if (!is_mdc) {

		set_reg(smmu_base + SMMU_ADDR_MSB,
			(uint32_t)((smmu_rwerraddr_phys >> 32) & 0xFFFF), 16, 0);
		set_reg(smmu_base + SMMU_ADDR_MSB,
			(uint32_t)((smmu_rwerraddr_phys >> 32) & 0xFFFF), 16, 16);
	}
}

static void dpu_smmu_cmdlist_config(char __iomem *smmu_base)
{
	uint32_t mmu_bypass_value;
	int idx0, idx1, idx2;
	uint32_t cmdlist_rld0, cmdlist_rld1, cmdlist_rld2;


	idx0 = 36; /* debug stream id */
	idx1 = 37; /* cmd unsec stream */
	idx2 = 38; /* cmd sec stream id */
	mmu_bypass_value = 0x1D;
	cmdlist_rld0 = 0xFFFFFFFF;
	cmdlist_rld1 = 0xFFFFFFCF;

	cmdlist_rld2 = 0x00FFFFFF;

	set_reg(smmu_base + SMMU_RLD_EN0_NS, cmdlist_rld0, 32, 0);
	set_reg(smmu_base + SMMU_RLD_EN1_NS, cmdlist_rld1, 32, 0);
	set_reg(smmu_base + SMMU_RLD_EN2_NS, cmdlist_rld2, 32, 0);
	set_reg(smmu_base + SMMU_SMRx_NS + idx0 * 0x4, mmu_bypass_value, 32, 0);
	set_reg(smmu_base + SMMU_SMRx_NS + idx1 * 0x4, mmu_bypass_value, 32, 0);
	set_reg(smmu_base + SMMU_SMRx_NS + idx2 * 0x4, mmu_bypass_value, 32, 0);
}

static void dpu_smmu_ttbr_config(char __iomem *smmu_base, bool is_mdc)
{
	uint32_t fama_ptw_msb;
	void_unused(is_mdc);

	set_reg(smmu_base + SMMU_CB_TTBR0,
		(uint32_t)dpu_smmu_domain_get_ttbr(), 32, 0);

	fama_ptw_msb = (dpu_smmu_domain_get_ttbr() >> 32) & 0x7F;
	set_reg(smmu_base + SMMU_FAMA_CTRL0, 0x80, 14, 0);
	set_reg(smmu_base + SMMU_FAMA_CTRL1, fama_ptw_msb, 7, 0);
}

void dpu_smmu_on(struct dpu_fb_data_type *dpufd)
{
	char __iomem *smmu_base = NULL;

	dpu_check_and_no_retval((!dpufd || !dpufd->dss_base), ERR, "null pointer\n");

	smmu_base = dpufd->dss_base + DSS_SMMU_OFFSET;

	dpu_smmu_scr_config(smmu_base);
	dpu_smmu_rwerr_addr_config(smmu_base, false);
	dpu_smmu_cmdlist_config(smmu_base);
	dpu_smmu_ttbr_config(smmu_base, false);
}

void dpu_mdc_smmu_on(struct dpu_fb_data_type *dpufd)
{
	char __iomem *smmu_base = NULL;

	dpu_check_and_no_retval((!dpufd || !dpufd->media_common_base), ERR, "null pointer\n");

	smmu_base = dpufd->media_common_base + VBIF0_SMMU_OFFSET;

	dpu_smmu_scr_config(smmu_base);
	dpu_smmu_rwerr_addr_config(smmu_base, true);
	dpu_smmu_cmdlist_config(smmu_base);
	dpu_smmu_ttbr_config(smmu_base, true);
}

static void dpu_smmu_smrx_config(struct dpu_fb_data_type *dpufd,
	dss_smmu_t *smmu, uint32_t idx, uint32_t mmu_enable)
{
	void_unused(dpufd);
	if (mmu_enable == 0) {
		smmu->smmu_smrx_ns[idx] = set_bits32(smmu->smmu_smrx_ns[idx], 0x1, 1, 0);
		return;
	}

	smmu->smmu_smrx_ns[idx] = set_bits32(smmu->smmu_smrx_ns[idx], 0x0, 1, 0);
	smmu->smmu_smrx_ns[idx] = set_bits32(smmu->smmu_smrx_ns[idx], 0x1, 1, 4);

	smmu->smmu_smrx_ns[idx] = set_bits32(smmu->smmu_smrx_ns[idx], 0x3, 2, 2);
}

int dpu_smmu_ch_config(struct dpu_fb_data_type *dpufd,
	dss_layer_t *layer, dss_wb_layer_t *wb_layer)
{
	int chn_idx;
	uint32_t i, idx;
	dss_img_t *img = NULL;
	dss_smmu_t *smmu = NULL;

	if (!dpufd)
		return -EINVAL;

	if (!wb_layer && !layer) {
		DPU_FB_ERR("layer and wb_layer is null");
		return -EINVAL;
	}

	if (wb_layer) {
		img = &(wb_layer->dst);
		chn_idx = wb_layer->chn_idx;
	} else {
		img = &(layer->img);
		chn_idx = layer->chn_idx;
	}
	dpu_check_and_return(((chn_idx >= DSS_CHN_MAX_DEFINE) || (chn_idx < 0)), -EINVAL, ERR,
		"array index overflow!\n");

	smmu = &(dpufd->dss_module.smmu);
	dpufd->dss_module.smmu_used = 1;
	smmu->smmu_smrx_ns_used[chn_idx] = 1;
	for (i = 0; i < g_dss_chn_sid_num[chn_idx]; i++) {
		idx = g_dss_smmu_smrx_idx[chn_idx] + i;
		if (idx >= SMMU_SID_NUM) {
			DPU_FB_ERR("idx is invalid");
			return -EINVAL;
		}
		dpu_smmu_smrx_config(dpufd, smmu, idx, img->mmu_enable);
	}

	return 0;
}

void dpu_smmu_ch_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *smmu_base, dss_smmu_t *s_smmu, int chn_idx)
{
	uint32_t idx = 0;
	uint32_t i = 0;

	dpu_check_and_no_retval((!dpufd || !smmu_base || !s_smmu), ERR, "null pointer\n");
	dpu_check_and_no_retval(((chn_idx >= DSS_CHN_MAX_DEFINE) || (chn_idx < 0)), ERR, "array index overflow!\n");

	if (s_smmu->smmu_smrx_ns_used[chn_idx] == 0)
		return;

	for (i = 0; i < g_dss_chn_sid_num[chn_idx]; i++) {
		idx = g_dss_smmu_smrx_idx[chn_idx] + i;
		if (idx >= SMMU_SID_NUM) {
			DPU_FB_ERR("idx is invalid");
			return;
		}
		dpufd->set_reg(dpufd, smmu_base + SMMU_SMRx_NS + idx * 0x4,
			s_smmu->smmu_smrx_ns[idx], 32, 0);
	}
}
