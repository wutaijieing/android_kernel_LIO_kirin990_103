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
#include <soc_dte_define.h>
#include <linux/delay.h>
#if defined(CONFIG_DRMDRIVER)
#include <platform_include/display/linux/dpu_drmdriver.h>
#endif
#include "hisi_disp_iommu.h"
#include "hisi_disp_gadgets.h"
#include "hisi_disp_config.h"
#include "hisi_disp_smmu.h"

static bool g_smmuv3_inited = false;
static int g_tbu_sr_refcount;
static int g_tbu0_cnt_refcount;
static bool offline_tbu1_conected = false;
static bool offline_tbu0_conected = false;
static struct mutex g_tbu_sr_lock;

typedef bool (*smmu_event_cmp)(uint32_t value, uint32_t expect_value);

bool hisi_smmu_connect_event_cmp(uint32_t value, uint32_t expect_value)
{
	if (((value & 0x3) != 0x3) || (((value >> 8) & 0x7f) < expect_value))
		return false;

	return true;
}

bool hisi_smmu_disconnect_event_cmp(uint32_t value, uint32_t expect_value)
{
	void_unused(expect_value);
	if ((value & 0x3) != 0x1)
		return false;

	return true;
}

static void hisi_smmu_event_request(char __iomem *smmu_base,
	smmu_event_cmp cmp_func, enum smmu_event event, uint32_t check_value)
{
	uint32_t smmu_tbu_crack;
	uint32_t delay_count = 0;

	/* request event config */
	dpu_set_reg(SOC_SMMUv3_TBU_SMMU_TBU_CR_ADDR(smmu_base), check_value, 8, 8);
	dpu_set_reg(SOC_SMMUv3_TBU_SMMU_TBU_CR_ADDR(smmu_base), event, 1, 0);

	/* check ack */
	do {
		smmu_tbu_crack = inp32(SOC_SMMUv3_TBU_SMMU_TBU_CRACK_ADDR(smmu_base));
		if (cmp_func(smmu_tbu_crack, check_value))
			break;

		udelay(1);
		delay_count++;
	} while (delay_count < SMMU_TIMEOUT);

	if (delay_count == SMMU_TIMEOUT) {
		disp_pr_info("smmu=0x%x event=%d smmu_tbu_crack=0x%x check_value=0x%x!\n", /*lint !e559*/
			smmu_base, event, smmu_tbu_crack, check_value);
	}
}

void hisi_dss_smmuv3_on(char __iomem *dpu_base, uint32_t scene_id)
{
	char __iomem *tbu_base = NULL;
	int ret = 0;

	mutex_lock(&g_tbu_sr_lock);
	if (g_tbu_sr_refcount == 0) {
		ret = regulator_enable(g_dpu_config.regulators[PM_REGULATOR_SMMU].consumer);
		if (ret)
			disp_pr_info("smmu tcu regulator_enable failed, error=%d!\n", ret);

		/* cmdlist select tbu0 config stream bypass */
		dpu_set_reg(DPU_DBCU_CMDLIST_AXI_SEL_ADDR(dpu_base + DSS_DBCU_OFFSET), 0, 1, 0);
		dpu_set_reg(DPU_DBCU_MMU_ID_ATTR_NS_56_ADDR(dpu_base + DSS_DBCU_OFFSET), 0x3F, 32, 0);
		dpu_set_reg(DPU_DBCU_AIF_CMD_RELOAD_ADDR(dpu_base + DSS_DBCU_OFFSET), 0x1, 1, 0);
	}

	if (scene_id > DPU_SCENE_ONLINE_3) {
		if (g_tbu0_cnt_refcount == 0) {
		#if defined(CONFIG_DRMDRIVER)
			// tbu1 is not available, skip this configure
			configure_dss_service_security(DSS_SMMU_INIT, 0, OFFLINE_COMPOSE_MODE);
		#endif
		disp_pr_info("offline smmu connect");
#ifdef CONFIG_DKMD_DPU_V720
			tbu_base = dpu_base + DSS_SMMU_OFFSET;
			hisi_smmu_event_request(tbu_base, hisi_smmu_connect_event_cmp,
				TBU_CONNECT, DSS_TBU0_DTI_NUMS);
			offline_tbu0_conected = true;
#else
			tbu_base = dpu_base + DSS_SMMU1_OFFSET;
			hisi_smmu_event_request(tbu_base, hisi_smmu_connect_event_cmp,
				TBU_CONNECT, DSS_TBU1_DTI_NUMS);
			offline_tbu1_conected = true;
#endif
		}
	} else {
		if (g_tbu0_cnt_refcount == 0) {
		#if defined(CONFIG_DRMDRIVER)
			configure_dss_service_security(DSS_SMMU_INIT, 0, ONLINE_COMPOSE_MODE);
		#endif
			tbu_base = dpu_base + DSS_SMMU_OFFSET;
			hisi_smmu_event_request(tbu_base, hisi_smmu_connect_event_cmp,
				TBU_CONNECT, DSS_TBU0_DTI_NUMS);

		#if defined(CONFIG_DRMDRIVER)
			// tbu1 is not available, skip this configure
			/* configure_dss_service_security(DSS_SMMU_INIT, 0, OFFLINE_COMPOSE_MODE); */
		#endif
#ifndef CONFIG_DKMD_DPU_V720
			tbu_base = dpu_base + DSS_SMMU1_OFFSET;
			hisi_smmu_event_request(tbu_base, hisi_smmu_connect_event_cmp,
				TBU_CONNECT, DSS_TBU1_DTI_NUMS);
#endif
		}
	}
	g_tbu0_cnt_refcount++;
	g_tbu_sr_refcount++;
	disp_pr_info("tbu_sr_refcount=%d, tbu0_cnt_refcount=%d, offline_tbu0_conected=%d, offline_tbu1_conected=%d",
		g_tbu_sr_refcount, g_tbu0_cnt_refcount, offline_tbu0_conected, offline_tbu1_conected);

	mutex_unlock(&g_tbu_sr_lock);
}

int hisi_dss_smmuv3_off(char __iomem *dpu_base, uint32_t scene_id)
{
	char __iomem *tbu_base = NULL;
	int ret = 0;

	mutex_lock(&g_tbu_sr_lock);
	g_tbu_sr_refcount--;

	if (scene_id > DPU_SCENE_ONLINE_3) {
		if (g_tbu0_cnt_refcount == 0) {
			tbu_base = dpu_base + DSS_SMMU1_OFFSET;
			hisi_smmu_event_request(tbu_base, hisi_smmu_disconnect_event_cmp, TBU_DISCONNECT, 0);
		}
#ifdef CONFIG_DKMD_DPU_V720
		offline_tbu0_conected = false;
#else
		offline_tbu1_conected = false;
#endif
	} else {
		g_tbu0_cnt_refcount--;
		if (g_tbu0_cnt_refcount == 0) {
			/* keep the connection if tbu0 is used by fb2 cmdlist */
			tbu_base = dpu_base + DSS_SMMU_OFFSET;
			hisi_smmu_event_request(tbu_base, hisi_smmu_disconnect_event_cmp, TBU_DISCONNECT, 0);

#ifndef CONFIG_DKMD_DPU_V720
			tbu_base = dpu_base + DSS_SMMU1_OFFSET;
			hisi_smmu_event_request(tbu_base, hisi_smmu_disconnect_event_cmp, TBU_DISCONNECT, 0);
#endif
		}
	}

	if (g_tbu_sr_refcount == 0) {
		ret = regulator_disable(g_dpu_config.regulators[PM_REGULATOR_SMMU].consumer);
		if (ret)
			disp_pr_info("smmu tcu regulator_disable failed, error=%d!\n", ret);
	}
	disp_pr_info("tbu_sr_refcount=%d, tbu0_cnt_refcount=%d, offline_tbu0_conected=%d, offline_tbu1_conected=%d",
		g_tbu_sr_refcount, g_tbu0_cnt_refcount, offline_tbu0_conected, offline_tbu1_conected);

	mutex_unlock(&g_tbu_sr_lock);

	return ret;
}

// TODO: to fix the unbalanced calls to smmu-on/smmu-off
int hisi_dss_smmuv3_force_off(char __iomem *dpu_base, uint32_t scene_id)
{
	char __iomem *tbu_base = NULL;
	int ret = 0;

	disp_pr_info(" ++++ \n");
	disp_pr_warn(" force smmuv3 off \n");

	mutex_lock(&g_tbu_sr_lock);

	g_tbu_sr_refcount = 0;
	g_tbu0_cnt_refcount = 0;
	offline_tbu1_conected = false;
	offline_tbu0_conected = false;

	tbu_base = dpu_base + DSS_SMMU1_OFFSET;
	hisi_smmu_event_request(tbu_base, hisi_smmu_disconnect_event_cmp, TBU_DISCONNECT, 0);

	ret = regulator_disable(g_dpu_config.regulators[PM_REGULATOR_SMMU].consumer);
	if (ret)
		disp_pr_err("smmu tcu regulator_disable failed, error=%d!\n", ret);

	mutex_unlock(&g_tbu_sr_lock);

	return ret;
}

struct swid_info {
	enum SCENE_ID scene_id;
	uint32_t swid_reg_offset_start;
	uint32_t swid_reg_offset_end;
};

#ifdef CONFIG_DKMD_DPU_V720
static struct swid_info sdma_swid_tlb_info[] = {
	{DPU_SCENE_ONLINE_0,   0,  19},
	{DPU_SCENE_ONLINE_1,   0,  19}, // no use
	{DPU_SCENE_ONLINE_2,   0,  19}, // no use
	{DPU_SCENE_ONLINE_3,   0,  19}, // no use
	{DPU_SCENE_OFFLINE_0, 20,  39},
	{DPU_SCENE_OFFLINE_1, 40,  47},
};
#else
static struct swid_info sdma_swid_tlb_info[] = {
	{DPU_SCENE_ONLINE_0,   0,  19},
	{DPU_SCENE_ONLINE_1,  20,  39},
	{DPU_SCENE_ONLINE_2,  40,  47},
	{DPU_SCENE_ONLINE_3,  48,  55},
	{DPU_SCENE_OFFLINE_0, 60,  67},
	{DPU_SCENE_OFFLINE_1, 68,  75},
	{DPU_SCENE_OFFLINE_2, 76,  76},
};
#endif

void hisi_dss_smmu_ch_set_reg(char __iomem *dpu_base, uint32_t scene_id, uint32_t layer_num)
{
	uint32_t i = 0;
	soc_dss_mmu_id_attr target;
	uint32_t swid_reg_offset_start = 0;
	uint32_t swid_reg_offset_end = 0;

	if (scene_id > ARRAY_SIZE(sdma_swid_tlb_info)) {
		disp_pr_info("invalid scene_id=%u\n", scene_id);
		return;
	}

	target.value = 0;
	target.reg.ch_sid = 0x1;
	target.reg.ch_ssidv = 1;
	if (scene_id > DPU_SCENE_ONLINE_3)
		target.reg.ch_ssid = HISI_OFFLINE_SSID + scene_id;
	else
		target.reg.ch_ssid = HISI_PRIMARY_DSS_SSID_OFFSET + scene_id;

	swid_reg_offset_start = sdma_swid_tlb_info[scene_id].swid_reg_offset_start;
	swid_reg_offset_end = sdma_swid_tlb_info[scene_id].swid_reg_offset_end;

	for (i = swid_reg_offset_start; i <= swid_reg_offset_end; i++) {
		dpu_set_reg(DPU_DBCU_MMU_ID_ATTR_NS_0_ADDR(dpu_base + DSS_DBCU_OFFSET) + 0x4 * i, target.value, 32, 0);
		disp_pr_debug("scene_id=%u config mmu_id=0x%x\n", scene_id, target.value);
	}

	target.reg.ch_ssid = HISI_OFFLINE_SSID + scene_id;
	dpu_set_reg(DPU_DBCU_MMU_ID_ATTR_NS_60_ADDR(dpu_base + DSS_DBCU_OFFSET), target.value, 32, 0);
}

#ifdef CONFIG_DKMD_DPU_V720
static struct swid_info wch_swid_tlb_info[] = {
	{DPU_SCENE_ONLINE_0,  57,  60},
};
#else
static struct swid_info wch_swid_tlb_info[] = {
	{DPU_SCENE_ONLINE_0,  57,  59},
	{DPU_SCENE_OFFLINE_0, 77,  79},
};
#endif

void hisi_dss_smmu_wch_set_reg(char __iomem *dpu_base, uint32_t scene_id)
{
	uint32_t i = 0;
	uint32_t idx = 0;
	uint32_t swid_reg_offset_start = 0;
	uint32_t swid_reg_offset_end = 0;
	soc_dss_mmu_id_attr target;

#ifdef CONFIG_DKMD_DPU_V720
	idx = 0;
#else
	idx = (scene_id > DPU_SCENE_ONLINE_3) ? 1 : 0;
#endif
	swid_reg_offset_start = wch_swid_tlb_info[idx].swid_reg_offset_start;
	swid_reg_offset_end = wch_swid_tlb_info[idx].swid_reg_offset_end;

	target.value = 0;
	target.reg.ch_sid = 0x1;
	target.reg.ch_ssidv = 1;
	target.reg.ch_ssid = HISI_OFFLINE_SSID + scene_id;

	for (i = swid_reg_offset_start; i <= swid_reg_offset_end; i++) {
		dpu_set_reg(DPU_DBCU_MMU_ID_ATTR_NS_0_ADDR(dpu_base + DSS_DBCU_OFFSET) + 0x4 * i, target.value, 32, 0);
		disp_pr_debug("scene_id=%u config mmu_id=0x%x\n", scene_id, target.value);
	}
}

void hisi_dss_smmu_tlb_flush(uint32_t scene_id)
{
	if (scene_id > DPU_SCENE_ONLINE_3) {
		mm_iommu_dev_flush_tlb(__hdss_get_dev(), HISI_OFFLINE_SSID + scene_id);
		disp_pr_debug("scene_id=%u flush mmu_id=0x%x\n", scene_id, HISI_OFFLINE_SSID + scene_id);
	} else {
		mm_iommu_dev_flush_tlb(__hdss_get_dev(), HISI_PRIMARY_DSS_SSID_OFFSET + scene_id);
		disp_pr_debug("scene_id=%u flush mmu_id=0x%x\n", scene_id, HISI_PRIMARY_DSS_SSID_OFFSET + scene_id);
	}
}

void hisi_disp_smmu_deinit(char __iomem *dpu_base, uint32_t scene_id)
{
	disp_pr_info(" +++++++ \n");
	if (g_smmuv3_inited) {
		g_smmuv3_inited = false;
		mutex_destroy(&g_tbu_sr_lock);
	}

	hisi_dss_smmuv3_force_off(dpu_base, scene_id);
	disp_pr_info(" ------- \n");
}

void hisi_disp_smmu_init(char __iomem *dpu_base, uint32_t scene_id)
{
	disp_pr_info(" +++++++ \n");
	if (!g_smmuv3_inited) {
		mutex_init(&g_tbu_sr_lock);
		g_smmuv3_inited = true;
	}

	/* FIXME: stub sel scene_id=3, sdma=0 */
	dpu_set_reg(DPU_GLB_SDMA_CTRL0_ADDR(dpu_base + DSS_GLB0_OFFSET, 0), 1, 1, 12); // va_en
	dpu_set_reg(DPU_DBCU_MIF_CTRL_SMARTDMA_0_ADDR(dpu_base + DSS_DBCU_OFFSET), 1, 1, 0); // pref_en_r0

	dpu_set_reg(DPU_GLB_SDMA_CTRL0_ADDR(dpu_base + DSS_GLB0_OFFSET, 1), 1, 1, 12); // va_en
	dpu_set_reg(DPU_DBCU_MIF_CTRL_SMARTDMA_1_ADDR(dpu_base + DSS_DBCU_OFFSET), 1, 1, 0); // pref_en_r1

	dpu_set_reg(DPU_DBCU_MIF_CTRL_WCH1_ADDR(dpu_base + DSS_DBCU_OFFSET), 1, 1, 0); // pref_en_w1
	dpu_set_reg(DPU_DBCU_MIF_CTRL_WCH2_ADDR(dpu_base + DSS_DBCU_OFFSET), 1, 1, 0); // pref_en_w2
	dpu_set_reg(DPU_DBCU_MIF_CTRL_WCH3_ADDR(dpu_base + DSS_DBCU_OFFSET), 0x0, 1, 0); // ld always disable pref_en

	hisi_dss_smmuv3_on(dpu_base, scene_id);

	disp_pr_info(" ------- \n");
}
