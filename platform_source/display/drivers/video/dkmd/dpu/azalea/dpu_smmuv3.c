/* Copyright (c) 2019-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#include <linux/mm_iommu.h>
#include <linux/platform_drivers/mm_svm.h>

#include "dpu_smmuv3.h"
#include "overlay/dpu_overlay_utils.h"
#include "dpu_iommu.h"

static bool g_smmuv3_inited = false;
static int g_tbu_sr_refcount;
static int g_tbu0_cnt_refcount;
static bool offline_tbu1_conected = false;
static struct mutex g_tbu_sr_lock;

typedef bool (*smmu_event_cmp)(uint32_t value, uint32_t expect_value);

struct ssid_info dpu_ssid_info[] = {
	{0, 2}, // fb0
	{3, 5}, // fb1
};

bool dpu_smmu_connect_event_cmp(uint32_t value, uint32_t expect_value)
{
	if (((value & 0x3) != 0x3) || (((value >> 8) & 0x7f) < expect_value))
		return false;

	return true;
}

bool dpu_smmu_disconnect_event_cmp(uint32_t value, uint32_t expect_value)
{
	void_unused(expect_value);
	if ((value & 0x3) != 0x1)
		return false;

	return true;
}

static void dpu_smmu_event_request(char __iomem *smmu_base,
	smmu_event_cmp cmp_func, enum smmu_event event, uint32_t check_value)
{
	uint32_t smmu_tbu_crack;
	uint32_t delay_count = 0;

	dpu_check_and_no_retval(!smmu_base, ERR, "smmu_base is nullptr!\n");
	/* request event config */
	set_reg(SOC_SMMUv3_TBU_SMMU_TBU_CR_ADDR(smmu_base), event, 1, 0);

	/* check ack */
	do {
		smmu_tbu_crack = inp32(SOC_SMMUv3_TBU_SMMU_TBU_CRACK_ADDR(smmu_base));
		if (cmp_func(smmu_tbu_crack, check_value))
			break;

		udelay(1);
		delay_count++;
	} while (delay_count < SMMU_TIMEOUT);

	if (delay_count == SMMU_TIMEOUT) {
		mm_smmu_dump_tbu_status(__hdss_get_dev());
		DPU_FB_ERR("smmu=0x%x event=%d smmu_tbu_crack=0x%x check_value=0x%x!\n", /*lint !e559*/
			smmu_base, event, smmu_tbu_crack, check_value);
	}
}

/* invalidate tlb before online play */
void dpu_smmuv3_invalidate_tlb(struct dpu_fb_data_type *dpufd)
{
	uint32_t ssid_idx;
	struct ssid_info fb_ssid_info;

	if (dpufd->index >= ARRAY_SIZE(dpu_ssid_info)) {
		DPU_FB_DEBUG("fb%u no need flush tlb\n", dpufd->index);
		return;
	}

	fb_ssid_info = dpu_ssid_info[dpufd->index];
	for (ssid_idx = fb_ssid_info.start_idx; ssid_idx <= fb_ssid_info.end_idx; ssid_idx++)
		mm_iommu_dev_flush_tlb(__hdss_get_dev(), ssid_idx);
}

void dpu_smmuv3_on(struct dpu_fb_data_type *dpufd)
{
	char __iomem *tbu_base = NULL;
	int ret;

	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is nullptr!\n");

	mutex_lock(&g_tbu_sr_lock);
	if (g_tbu_sr_refcount == 0) {
		ret = regulator_enable(dpufd->smmu_tcu_regulator->consumer);
		if (ret)
			DPU_FB_ERR("fb%u smmu tcu regulator_enable failed, error=%d!\n", dpufd->index, ret);

		/* cmdlist select tbu1 config stream bypass */
		set_reg(dpufd->dss_base + DSS_VBIF0_AIF + AIF_CH_CTL_CMD, 0x1, 1, 0);
		set_reg(dpufd->dss_base + DSS_VBIF0_AIF + MMU_ID_ATTR_NS_13, 0x3F, 32, 0);
		set_reg(dpufd->dss_base + VBIF0_MIF_OFFSET + AIF_CMD_RELOAD, 0x1, 1, 0);
	}

	if (dpufd->index == AUXILIARY_PANEL_IDX) {
		if (g_tbu0_cnt_refcount == 0) {
			dpufb_atf_config_security(DSS_SMMU_INIT, 0, OFFLINE_COMPOSE_MODE);
			tbu_base = dpufd->dss_base + DSS_SMMU1_OFFSET;
			dpu_smmu_event_request(tbu_base, dpu_smmu_connect_event_cmp,
				TBU_CONNECT, DSS_TBU1_DTI_NUMS);
		}
		offline_tbu1_conected = true;
	} else {
		if (g_tbu0_cnt_refcount == 0) {
			dpufb_atf_config_security(DSS_SMMU_INIT, 0, ONLINE_COMPOSE_MODE);
			tbu_base = dpufd->dss_base + DSS_SMMU_OFFSET;
			dpu_smmu_event_request(tbu_base, dpu_smmu_connect_event_cmp,
				TBU_CONNECT, DSS_TBU0_DTI_NUMS);

			dpufb_atf_config_security(DSS_SMMU_INIT, 0, OFFLINE_COMPOSE_MODE);
			tbu_base = dpufd->dss_base + DSS_SMMU1_OFFSET;
			dpu_smmu_event_request(tbu_base, dpu_smmu_connect_event_cmp,
				TBU_CONNECT, DSS_TBU1_DTI_NUMS);
		}
		g_tbu0_cnt_refcount++;
	}

	g_tbu_sr_refcount++;

	DPU_FB_DEBUG("fb%u tbu_sr_refcount=%d, tbu0_cnt_refcount=%d, offline_tbu1_conected=%d",
		dpufd->index, g_tbu_sr_refcount, g_tbu0_cnt_refcount, offline_tbu1_conected);

	mutex_unlock(&g_tbu_sr_lock);
}

int dpu_smmuv3_off(struct dpu_fb_data_type *dpufd)
{
	char __iomem *tbu_base = NULL;
	int ret = 0;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is nullptr\n");
	mutex_lock(&g_tbu_sr_lock);
	g_tbu_sr_refcount--;

	if (dpufd->index == AUXILIARY_PANEL_IDX) {
		if (g_tbu0_cnt_refcount == 0) {
			tbu_base = dpufd->dss_base + DSS_SMMU1_OFFSET;
			dpu_smmu_event_request(tbu_base, dpu_smmu_disconnect_event_cmp, TBU_DISCONNECT, 0);
		}
		offline_tbu1_conected = false;
	} else {
		g_tbu0_cnt_refcount--;
		if (g_tbu0_cnt_refcount == 0) {
			/* keep the connection if tbu0 is used by fb2 cmdlist */
			tbu_base = dpufd->dss_base + DSS_SMMU_OFFSET;
			dpu_smmu_event_request(tbu_base, dpu_smmu_disconnect_event_cmp, TBU_DISCONNECT, 0);
			if (!offline_tbu1_conected) {
				tbu_base = dpufd->dss_base + DSS_SMMU1_OFFSET;
				dpu_smmu_event_request(tbu_base, dpu_smmu_disconnect_event_cmp, TBU_DISCONNECT, 0);
			}
		}
	}

	if (g_tbu_sr_refcount == 0) {
		ret = regulator_disable(dpufd->smmu_tcu_regulator->consumer);
		if (ret)
			DPU_FB_ERR("fb%u smmu tcu regulator_disable failed, error=%d!\n", dpufd->index, ret);
	}
	DPU_FB_DEBUG("fb%u tbu_sr_refcount=%d, tbu0_cnt_refcount=%d, offline_tbu1_conected=%d",
		dpufd->index, g_tbu_sr_refcount, g_tbu0_cnt_refcount, offline_tbu1_conected);

	mutex_unlock(&g_tbu_sr_lock);

	return ret;
}

void dpu_mdc_smmu_on(struct dpu_fb_data_type *dpufd)
{
	int ret;
	int i = 0;
	char __iomem *addr_base = NULL;

	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is nullptr!\n");

	ret = regulator_enable(dpufd->smmu_tcu_regulator->consumer);
	if (ret)
		DPU_FB_ERR("smmu tcu regulator_enable failed, error=%d!\n", ret);

	addr_base = dpufd->media_common_base + VBIF0_SMMU_OFFSET;
	set_reg(SOC_SMMUv3_TBU_SMMU_TBU_CR_ADDR(addr_base), 0x10100F03, 32, 0);
	dpu_smmu_event_request(addr_base, dpu_smmu_connect_event_cmp,
		TBU_CONNECT, DSS_TBU1_DTI_NUMS);

	/* enable mdc tbu pref, num=33 was reference chip manual */
	for (; i < MDC_SWID_NUMS; i++)
		set_reg(SOC_SMMUv3_TBU_SMMU_TBU_SWID_CFG_ADDR(addr_base, i), 1, 1, 31);
}

int dpu_mdc_smmu_off(struct dpu_fb_data_type *dpufd)
{
	int ret;
	char __iomem *addr_base = NULL;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is null!\n");

	addr_base = dpufd->media_common_base + VBIF0_SMMU_OFFSET;
	dpu_smmu_event_request(addr_base, dpu_smmu_disconnect_event_cmp, TBU_DISCONNECT, 0);

	ret = regulator_disable(dpufd->smmu_tcu_regulator->consumer);
	if (ret)
		DPU_FB_ERR("smmu tcu regulator_disable failed, error=%d!\n", ret);

	return ret;
}

int dpu_smmu_ch_config(struct dpu_fb_data_type *dpufd,
	dss_layer_t *layer, dss_wb_layer_t *wb_layer)
{
	dss_img_t *img = NULL;
	dss_smmu_t *smmu = NULL;
	int chn_idx;
	uint32_t ssid;
	soc_dss_mmu_id_attr taget;

	dpu_check_and_return(!dpufd, -EINVAL, ERR, "dpufd is null!\n");

	if (wb_layer) {
		img = &(wb_layer->dst);
		chn_idx = wb_layer->chn_idx;
	} else if (layer) {
		img = &(layer->img);
		chn_idx = layer->chn_idx;
	} else {
		DPU_FB_ERR("layer or wb_layer is NULL");
		return -EINVAL;
	}

	smmu = &(dpufd->dss_module.smmu);
	dpufd->dss_module.smmu_used = 1;

	taget.value = 0;
	taget.reg.ch_sid = img->mmu_enable ? 0x1 : 0x3F;
	taget.reg.ch_ssidv = 1;

	ssid = dpu_get_ssid(dpufd->ov_req.frame_no);
	if (dpufd->index == PRIMARY_PANEL_IDX)
		taget.reg.ch_ssid = ssid + DPU_PRIMARY_DSS_SSID_OFFSET;
	else if (dpufd->index == EXTERNAL_PANEL_IDX)
		taget.reg.ch_ssid = ssid + DPU_EXTERNAL_DSS_SSID_OFFSET;
	else
		taget.reg.ch_ssid = DPU_OFFLINE_SSID;

	smmu->ch_mmu_attr[chn_idx].value = taget.value;
	DPU_FB_DEBUG("config ch_mmu_attr[%d].value=0x%x",
		chn_idx, smmu->ch_mmu_attr[chn_idx].value);

	return 0;
}

void dpu_smmu_ch_set_reg(struct dpu_fb_data_type *dpufd,
	char __iomem *smmu_base, dss_smmu_t *s_smmu, int chn_idx)
{
	char __iomem *addr = NULL;

	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is nullptr!\n");

	if (chn_idx == DSS_RCHN_V2)
		addr = dpufd->dss_base + DSS_VBIF0_AIF + MMU_ID_ATTR_NS_11;
	else
		addr = dpufd->dss_base + DSS_VBIF0_AIF + MMU_ID_ATTR_NS_0 + chn_idx * 0x4;

	if (dpufd->index == MEDIACOMMON_PANEL_IDX)
		addr = dpufd->media_common_base + DSS_VBIF0_AIF + MMU_ID_ATTR_NS_0 + chn_idx * 0x4;

	dpufd->set_reg(dpufd, addr, s_smmu->ch_mmu_attr[chn_idx].value, 32, 0);

	DPU_FB_DEBUG("config ch_mmu_attr[%d].value=0x%x",
		chn_idx, s_smmu->ch_mmu_attr[chn_idx].value);
}

/* for dssv600 es chip bugfix pref lock clean fail */
void dpu_mmu_lock_clear(struct dpu_fb_data_type *dpufd)
{
	char __iomem *addr = NULL;

	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is nullptr!\n");

	if (dpufd->index != AUXILIARY_PANEL_IDX) {
		/* set tbu0 lock clear */
		addr = dpufd->dss_base + DSS_SMMU_OFFSET;
		set_reg(SOC_SMMUv3_TBU_SMMU_TBU_CR_ADDR(addr), 0x3, 2, 1);
		outp32(SOC_SMMUv3_TBU_SMMU_TBU_LOCK_CLR_ADDR(addr), 0x1);
		set_reg(SOC_SMMUv3_TBU_SMMU_TBU_CR_ADDR(addr), 0x1, 2, 1);
	}

	/* set tbu1 lock clear */
	addr = dpufd->dss_base + DSS_SMMU1_OFFSET;
	set_reg(SOC_SMMUv3_TBU_SMMU_TBU_CR_ADDR(addr), 0x3, 2, 1);
	outp32(SOC_SMMUv3_TBU_SMMU_TBU_LOCK_CLR_ADDR(addr), 0x1);
	set_reg(SOC_SMMUv3_TBU_SMMU_TBU_CR_ADDR(addr), 0x1, 2, 1);
}

void dpu_mmu_tlb_flush(struct dpu_fb_data_type *dpufd, uint32_t frame_no)
{
	uint32_t ssid;

	dpu_check_and_no_retval(!dpufd, ERR, "dpufd is nullptr!\n");
	ssid = dpu_get_ssid(frame_no);

	if (dpufd->index == PRIMARY_PANEL_IDX)
		ssid = ssid + DPU_PRIMARY_DSS_SSID_OFFSET;
	else if (dpufd->index == EXTERNAL_PANEL_IDX)
		ssid = ssid + DPU_EXTERNAL_DSS_SSID_OFFSET;
	else
		ssid = DPU_OFFLINE_SSID;

	mm_iommu_dev_flush_tlb(__hdss_get_dev(), ssid);
}

void dpu_smmuv3_deinit(void)
{
	if (g_smmuv3_inited) {
		g_smmuv3_inited = false;
		mutex_destroy(&g_tbu_sr_lock);
	}
}

void dpu_smmuv3_init(void)
{
	if (!g_smmuv3_inited) {
		mutex_init(&g_tbu_sr_lock);
		g_smmuv3_inited = true;
	}
}

