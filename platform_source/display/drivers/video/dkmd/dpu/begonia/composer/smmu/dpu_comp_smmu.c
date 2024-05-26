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

#include "dkmd_log.h"

#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#if defined(CONFIG_DRMDRIVER)
#include <platform_include/display/linux/dpu_drmdriver.h>
#endif
#include <soc_crgperiph_interface.h>
#include <soc_smmuv3_tbu_interface.h>
#include <dpu/soc_dpu_define.h>
#include <linux/mm_iommu.h>
#include "dkmd_chrdev.h"
#include "res_mgr.h"
#include "dpu_comp_mgr.h"
#include "smmu/dpu_comp_smmu.h"
#include "config/dpu_comp_config_utils.h"
#include "cmdlist_interface.h"

static int32_t g_tbu_sr_refcount;
static int32_t g_tbu0_cnt_refcount;
static int32_t g_tbu1_cnt_refcount;

typedef bool (*smmu_event_cmp)(uint32_t value, uint32_t expect_value);

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

	/* request event config */
	set_reg(SOC_SMMUv3_TBU_SMMU_TBU_CR_ADDR(smmu_base), check_value, 8, 8);
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
		dpu_pr_warn("smmu ack timeout, smmu=0x%x event=%d smmu_tbu_crack=0x%x check_value=0x%x!",
			smmu_base, event, smmu_tbu_crack, check_value);
	}
}

void dpu_comp_smmuv3_on(struct composer_manager *comp_mgr, struct dpu_composer *dpu_comp)
{
	char __iomem *tbu_base = NULL;

	mutex_lock(&comp_mgr->tbu_sr_lock);
	if (g_tbu_sr_refcount == 0) {
		/* cmdlist select tbu0 config stream bypass, so offline need connect tbu0 */
		set_reg(DPU_DBCU_CMDLIST_AXI_SEL_ADDR(comp_mgr->dpu_base + DPU_DBCU_OFFSET), 0x0, 2, 0);
		set_reg(DPU_DBCU_MMU_ID_ATTR_NS_56_ADDR(comp_mgr->dpu_base + DPU_DBCU_OFFSET), 0x3F, 32, 0);
		set_reg(DPU_DBCU_AIF_CMD_RELOAD_ADDR(comp_mgr->dpu_base + DPU_DBCU_OFFSET), 0x1, 1, 0);
	}

	if (g_tbu0_cnt_refcount == 0) {
	#if defined(CONFIG_DRMDRIVER)
		configure_dss_service_security(DSS_SMMU_INIT, 0, dpu_comp->conn_info->base.pipe_sw_itfch_idx);
	#endif
		tbu_base = comp_mgr->dpu_base + DPU_SMMU_OFFSET;
		dpu_smmu_event_request(tbu_base, dpu_smmu_connect_event_cmp, TBU_CONNECT, DPU_TBU0_DTI_NUMS);
	}

	tbu_base = dpu_comp_get_tbu1_base(comp_mgr->dpu_base);
	if (is_offline_panel(&dpu_comp->conn_info->base) && tbu_base) {
		if (g_tbu1_cnt_refcount == 0) {
		#if defined(CONFIG_DRMDRIVER)
			configure_dss_service_security(DSS_SMMU_INIT, 0, dpu_comp->conn_info->base.pipe_sw_itfch_idx);
		#endif
			dpu_smmu_event_request(tbu_base, dpu_smmu_connect_event_cmp, TBU_CONNECT, DPU_TBU1_DTI_NUMS);
		}
		g_tbu1_cnt_refcount++;
	} else {
		g_tbu0_cnt_refcount++;
	}
	if (is_offline_panel(&dpu_comp->conn_info->base))
		dpu_comp_wch_axi_sel_set_reg(comp_mgr->dpu_base + DPU_DBCU_OFFSET);

	g_tbu_sr_refcount++;
	dpu_pr_info("tbu_sr_refcount=%d, tbu0_cnt_refcount=%d, g_tbu1_cnt_refcount=%d",
		g_tbu_sr_refcount, g_tbu0_cnt_refcount, g_tbu1_cnt_refcount);

	mutex_unlock(&comp_mgr->tbu_sr_lock);
}

void dpu_comp_smmuv3_off(struct composer_manager *comp_mgr, struct dpu_composer *dpu_comp)
{
	char __iomem *tbu_base = NULL;

	mutex_lock(&comp_mgr->tbu_sr_lock);
	g_tbu_sr_refcount--;
	tbu_base = dpu_comp_get_tbu1_base(comp_mgr->dpu_base);
	if (is_offline_panel(&dpu_comp->conn_info->base) && tbu_base) {
		g_tbu1_cnt_refcount--;
		if (g_tbu1_cnt_refcount == 0)
			dpu_smmu_event_request(tbu_base, dpu_smmu_disconnect_event_cmp, TBU_DISCONNECT, 0);

		if (g_tbu0_cnt_refcount == 0) {
			tbu_base = comp_mgr->dpu_base + DPU_SMMU_OFFSET;
			dpu_smmu_event_request(tbu_base, dpu_smmu_disconnect_event_cmp, TBU_DISCONNECT, 0);
		}
	} else {
		g_tbu0_cnt_refcount--;
		/* keep the connection if tbu0 is used by fb2 cmdlist */
		if ((g_tbu0_cnt_refcount == 0) && (g_tbu1_cnt_refcount == 0)) {
			tbu_base = comp_mgr->dpu_base + DPU_SMMU_OFFSET;
			dpu_smmu_event_request(tbu_base, dpu_smmu_disconnect_event_cmp, TBU_DISCONNECT, 0);
		}
	}
	dpu_pr_info("tbu_sr_refcount=%d, tbu0_cnt_refcount=%d, g_tbu1_cnt_refcount=%d",
		g_tbu_sr_refcount, g_tbu0_cnt_refcount, g_tbu1_cnt_refcount);

	mutex_unlock(&comp_mgr->tbu_sr_lock);
}

void dpu_comp_smmu_ch_set_reg(uint32_t reg_cmdlist_id, uint32_t scene_id, uint32_t frame_index)
{
	uint32_t i = 0;
	soc_dss_mmu_id_attr target;
	struct swid_info *info = NULL;

	for (i = 0; i < ARRAY_SIZE(g_sdma_swid_tlb_info); i++) {
		if (scene_id == g_sdma_swid_tlb_info[i].scene_id ) {
			info = &g_sdma_swid_tlb_info[i];
			break;
		}
	}

	if (info == NULL) {
		dpu_pr_err("invalid scene_id=%u", scene_id);
		return;
	}

	target.value = 0;
	target.reg.ch_sid = 0x1;
	target.reg.ch_ssidv = 1;
	target.reg.ch_ssid = dpu_comp_get_ssid(frame_index) + scene_id * DPU_SSID_MAX_NUM;
	for (i = info->swid_reg_offset_start; i <= info->swid_reg_offset_end; i++) {
		dkmd_set_reg(scene_id, reg_cmdlist_id, DPU_DBCU_MMU_ID_ATTR_NS_0_ADDR(DPU_DBCU_OFFSET) + 0x4 * i, target.value);
		dpu_pr_debug("offset[%#x] scene_id=%u config mmu_id=0x%x", 0x4 * i, scene_id, target.value);
	}

	dkmd_set_reg(scene_id, reg_cmdlist_id, DPU_DBCU_AIF_CMD_RELOAD_ADDR(DPU_DBCU_OFFSET), 1);
}

void dpu_comp_smmu_offline_tlb_flush(uint32_t scene_id, uint32_t block_num)
{
	uint32_t i = 0;
	uint32_t mmu_id = scene_id * DPU_SSID_MAX_NUM;

	for (; i < block_num; ++i) {
		mm_iommu_dev_flush_tlb(dpu_res_get_device(), mmu_id + i);
		dpu_pr_debug("scene_id=%u block_num=%u flush mmu_id=0x%x", scene_id, block_num, mmu_id + i);
	}
}

void dpu_comp_smmu_tlb_flush(uint32_t scene_id, uint32_t frame_index)
{
	uint32_t mmu_id = dpu_comp_get_ssid(frame_index) + scene_id * DPU_SSID_MAX_NUM;

	mm_iommu_dev_flush_tlb(dpu_res_get_device(), mmu_id);
	dpu_pr_debug("scene_id=%u flush mmu_id=0x%x", scene_id, mmu_id);
}

void dpu_comp_smmuv3_recovery_on(struct composer_manager *comp_mgr, struct dpu_composer *dpu_comp)
{
	char __iomem *tbu_base = NULL;

	mutex_lock(&comp_mgr->tbu_sr_lock);

	/* cmdlist select tbu0 config stream bypass, so offline need connect tbu0 */
	set_reg(DPU_DBCU_CMDLIST_AXI_SEL_ADDR(comp_mgr->dpu_base + DPU_DBCU_OFFSET), 0, 1, 0);
	set_reg(DPU_DBCU_MMU_ID_ATTR_NS_56_ADDR(comp_mgr->dpu_base + DPU_DBCU_OFFSET), 0x3F, 32, 0);
	set_reg(DPU_DBCU_AIF_CMD_RELOAD_ADDR(comp_mgr->dpu_base + DPU_DBCU_OFFSET), 0x1, 1, 0);

#if defined(CONFIG_DRMDRIVER)
	configure_dss_service_security(DSS_SMMU_INIT, 0, ONLINE_COMPOSE_MODE);
#endif
	tbu_base = comp_mgr->dpu_base + DPU_SMMU_OFFSET;
	dpu_smmu_event_request(tbu_base, dpu_smmu_connect_event_cmp, TBU_CONNECT, DPU_TBU0_DTI_NUMS);

	if (g_tbu1_cnt_refcount != 0) {
	#if defined(CONFIG_DRMDRIVER)
		configure_dss_service_security(DSS_SMMU_INIT, 0, OFFLINE_COMPOSE_MODE);
	#endif
		tbu_base = dpu_comp_get_tbu1_base(comp_mgr->dpu_base);
		dpu_check_and_no_retval(!tbu_base, err, "tbu_base is null ptr");
		dpu_smmu_event_request(tbu_base, dpu_smmu_connect_event_cmp, TBU_CONNECT, DPU_TBU1_DTI_NUMS);
	}

	if (is_offline_panel(&dpu_comp->conn_info->base))
		dpu_comp_wch_axi_sel_set_reg(comp_mgr->dpu_base + DPU_DBCU_OFFSET);

	dpu_pr_info("tbu_sr_refcount=%d, tbu0_cnt_refcount=%d, g_tbu1_cnt_refcount=%d",
		g_tbu_sr_refcount, g_tbu0_cnt_refcount, g_tbu1_cnt_refcount);

	mutex_unlock(&comp_mgr->tbu_sr_lock);
}

void dpu_comp_smmuv3_reset_off(struct composer_manager *comp_mgr, struct dpu_composer *dpu_comp)
{
	char __iomem *tbu_base = NULL;

	mutex_lock(&comp_mgr->tbu_sr_lock);

	tbu_base = comp_mgr->dpu_base + DPU_SMMU_OFFSET;
	dpu_smmu_event_request(tbu_base, dpu_smmu_disconnect_event_cmp, TBU_DISCONNECT, 0);

	if (g_tbu1_cnt_refcount != 0) {
		tbu_base = dpu_comp_get_tbu1_base(comp_mgr->dpu_base);
		dpu_check_and_no_retval(!tbu_base, err, "tbu_base is null ptr");
		dpu_smmu_event_request(tbu_base, dpu_smmu_disconnect_event_cmp, TBU_DISCONNECT, 0);
	}

	dpu_pr_info("tbu_sr_refcount=%d, tbu0_cnt_refcount=%d, g_tbu1_cnt_refcount=%d",
		g_tbu_sr_refcount, g_tbu0_cnt_refcount, g_tbu1_cnt_refcount);

	mutex_unlock(&comp_mgr->tbu_sr_lock);
}
