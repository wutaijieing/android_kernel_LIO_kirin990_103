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
#include <dpu/soc_dpu_define.h>
#include "dpu_dacc.h"
#include "dpu_config_utils.h"
#include "dpu_comp_smmu.h"
#include "dpu_comp_online.h"
#include "dpu_comp_offline.h"
#include "dpu_comp_mgr.h"
#include "dpu_comp_init.h"

static int32_t media_regulator_enable(struct composer_manager *comp_mgr)
{
	int32_t ret = 0;

	ret = regulator_enable(comp_mgr->media1_subsys_regulator);
	if (ret)
		dpu_pr_err("regulator enable media1_subsys failed, error=%d!", ret);

	ret = regulator_enable(comp_mgr->vivobus_regulator);
	if (ret)
		dpu_pr_err("regulator enable vivobus failed, error=%d!", ret);

	return ret;
}

static int32_t media_regulator_disable(struct composer_manager *comp_mgr)
{
	int32_t ret = 0;

	ret = regulator_disable(comp_mgr->vivobus_regulator);
	if (ret)
		dpu_pr_err("regulator disable vivobus failed, error=%d!", ret);

	ret = regulator_disable(comp_mgr->media1_subsys_regulator);
	if (ret)
		dpu_pr_err("regulator disable media1_subsys failed, error=%d!", ret);

	return ret;
}

static int32_t composer_regulator_enable(struct composer_manager *comp_mgr)
{
	int32_t ret = 0;

	dpu_pr_info("v740 +");

	if (!IS_ERR_OR_NULL(comp_mgr->dsisubsys_regulator)) {
		ret = regulator_enable(comp_mgr->dsisubsys_regulator);
		if (ret)
			dpu_pr_err("dsisubsys_regulator enable failed, error=%d!", ret);
	}

	if (!IS_ERR_OR_NULL(comp_mgr->dpsubsys_regulator)) {
		ret = regulator_enable(comp_mgr->dpsubsys_regulator);
		if (ret)
			dpu_pr_err("dpsubsys_regulator enable failed, error=%d!", ret);
	}

	if (!IS_ERR_OR_NULL(comp_mgr->dsssubsys_regulator)) {
		ret = regulator_enable(comp_mgr->dsssubsys_regulator);
		if (ret)
			dpu_pr_err("dsssubsys_regulator enable failed, error=%d!", ret);
	}

	return ret;
}

static int32_t composer_regulator_disable(struct composer_manager *comp_mgr)
{
	int32_t ret = 0;

	if (!IS_ERR_OR_NULL(comp_mgr->dsssubsys_regulator)) {
		ret = regulator_disable(comp_mgr->dsssubsys_regulator);
		if (ret)
			dpu_pr_err("dsssubsys_regulator disable failed, error=%d!", ret);
	}

	if (!IS_ERR_OR_NULL(comp_mgr->dpsubsys_regulator)) {
		ret = regulator_enable(comp_mgr->dpsubsys_regulator);
		if (ret)
			dpu_pr_err("dpsubsys_regulator enable failed, error=%d!", ret);
	}

	if (!IS_ERR_OR_NULL(comp_mgr->dsisubsys_regulator)) {
		ret = regulator_enable(comp_mgr->dsisubsys_regulator);
		if (ret)
			dpu_pr_err("dsisubsys_regulator enable failed, error=%d!", ret);
	}

	return 0;
}

static void composer_dpu_power_on(struct composer_manager *comp_mgr)
{
	dpu_enable_core_clock();
	dpu_legacy_inter_frame_dvfs_vote(1);
	composer_regulator_enable(comp_mgr);

	/* Cmdlist preparation, need to use the CPU configuration */
	set_reg(comp_mgr->dpu_base + DPU_GLB_PM_CTRL_ADDR(DPU_GLB0_OFFSET), 0x0401A00F, 32, 0);

	/* FIXME: stub sel scene_id=3, sdma=0  would be deleted */
	set_reg(DPU_GLB_SDMA_CTRL0_ADDR(comp_mgr->dpu_base + DPU_GLB0_OFFSET, 0), 1, 1, 12);
	set_reg(DPU_DBCU_MIF_CTRL_SMARTDMA_0_ADDR(comp_mgr->dpu_base + DPU_DBCU_OFFSET), 1, 1, 0);

	set_reg(DPU_GLB_SDMA_DBG_RESERVED0_ADDR(comp_mgr->dpu_base + DPU_GLB0_OFFSET, 0), 1, 1, 20);
	set_reg(DPU_GLB_SDMA_DBG_RESERVED0_ADDR(comp_mgr->dpu_base + DPU_GLB0_OFFSET, 1), 1, 1, 20);
	set_reg(DPU_GLB_SDMA_DBG_RESERVED0_ADDR(comp_mgr->dpu_base + DPU_GLB0_OFFSET, 2), 1, 1, 20);
	set_reg(DPU_GLB_SDMA_DBG_RESERVED0_ADDR(comp_mgr->dpu_base + DPU_GLB0_OFFSET, 3), 1, 1, 20);
}

void composer_dpu_power_on_sub(struct dpu_composer *dpu_comp)
{
	struct dkmd_connector_info *pinfo = dpu_comp->conn_info;
	struct composer_manager *comp_mgr = dpu_comp->comp_mgr;

	dpu_comp_smmuv3_on(comp_mgr, dpu_comp);

	dpu_comp_init(dpu_comp);

	(void)pipeline_next_ops_handle(pinfo->conn_device, pinfo, INIT_DSC, pinfo);
	(void)pipeline_next_ops_handle(pinfo->conn_device, pinfo, INIT_SPR, NULL);
	(void)pipeline_next_on(pinfo->conn_device, pinfo);
	(void)pipeline_next_ops_handle(pinfo->conn_device, pinfo, ENABLE_ISR, &dpu_comp->isr_ctrl);
}

static void composer_dpu_power_off(struct composer_manager *comp_mgr)
{
	composer_regulator_disable(comp_mgr);
	dpu_disable_core_clock();
}

void composer_dpu_power_off_sub(struct dpu_composer *dpu_comp)
{
	struct dkmd_connector_info *pinfo = dpu_comp->conn_info;
	struct composer_manager *comp_mgr = dpu_comp->comp_mgr;

	(void)pipeline_next_ops_handle(pinfo->conn_device, pinfo, DISABLE_ISR, &dpu_comp->isr_ctrl);
	(void)pipeline_next_off(pinfo->conn_device, pinfo);

	dpu_comp_smmuv3_off(comp_mgr, dpu_comp);
}

int32_t composer_manager_set_fastboot(struct composer *comp)
{
	struct dpu_composer *dpu_comp = to_dpu_composer(comp);
	struct dkmd_connector_info *pinfo = dpu_comp->conn_info;
	struct composer_manager *comp_mgr = dpu_comp->comp_mgr;

	media_regulator_enable(comp_mgr);
	composer_dpu_power_on(comp_mgr);
	dpu_comp_smmuv3_on(comp_mgr, dpu_comp);
	dpu_comp_init(dpu_comp);

	(void)pipeline_next_ops_handle(pinfo->conn_device, pinfo, SET_FASTBOOT, (void *)&comp->fastboot_display_enabled);
	(void)pipeline_next_ops_handle(pinfo->conn_device, pinfo, ENABLE_ISR, &dpu_comp->isr_ctrl);
	comp_mgr->power_status.refcount.value[comp->index]++;

	return comp->fastboot_display_enabled;
}

int32_t composer_manager_on(struct composer *comp)
{
	struct dpu_composer *dpu_comp = to_dpu_composer(comp);
	struct composer_manager *comp_mgr = dpu_comp->comp_mgr;

	dpu_pr_info("power_status=0x%llx enter", comp_mgr->power_status.status);

	dpu_comp_active_vsync(dpu_comp);
	if (comp_mgr->power_status.refcount.value[comp->index] == 0) {
		/* public function need use public data as interface parameter, such as 'comp_mgr' */
		down(&dpu_comp->comp_mgr->power_sem);
		if (comp_mgr->power_status.status == 0) {
			media_regulator_enable(dpu_comp->comp_mgr);
			composer_dpu_power_on(dpu_comp->comp_mgr);
			dpu_dacc_load();
		}
		comp_mgr->power_status.refcount.value[comp->index]++;
		up(&dpu_comp->comp_mgr->power_sem);

		composer_present_data_setup(dpu_comp, false);
		(dpu_comp);
	} else {
		comp_mgr->power_status.refcount.value[comp->index]++;
		dpu_pr_warn("%s set too many powerup!", comp->base.name);
	}
	dpu_comp_deactive_vsync(dpu_comp);
	dpu_pr_info("power_status=0x%llx exit", comp_mgr->power_status.status);

	return 0;
}

int32_t composer_manager_off(struct composer *comp)
{
	struct dpu_composer *dpu_comp = to_dpu_composer(comp);
	struct composer_manager *comp_mgr = dpu_comp->comp_mgr;

	dpu_pr_info("power_status=0x%llx enter", comp_mgr->power_status.status);

	dpu_comp_active_vsync(dpu_comp);
	if (comp_mgr->power_status.refcount.value[comp->index] == 1) {
		composer_dpu_power_off_sub(dpu_comp);
		/* public function need use public data as interface parameter, such as 'comp_mgr' */
		down(&dpu_comp->comp_mgr->power_sem);
		comp_mgr->power_status.refcount.value[comp->index]--;
		if (comp_mgr->power_status.status == 0) {
			/* abort idle thread handle */
			mutex_lock(&comp_mgr->idle_lock);
			comp_mgr->idle_enable = false;
			comp_mgr->idle_func_flag = 0;
			mutex_unlock(&comp_mgr->idle_lock);

			composer_dpu_power_off(comp_mgr);
			media_regulator_disable(comp_mgr);
		}
		up(&dpu_comp->comp_mgr->power_sem);

		composer_present_data_release(dpu_comp);
	} else {
		comp_mgr->power_status.refcount.value[comp->index]--;
		dpu_pr_warn("%s set too many powerup!", comp->base.name);
	}
	dpu_comp_deactive_vsync(dpu_comp);
	dpu_pr_info("power_status=0x%llx exit", comp_mgr->power_status.status);

	return 0;
}

static void composer_manager_underflow_reset(struct dpu_composer *dpu_comp)
{
	struct dkmd_isr *isr_ctrl = NULL;
	struct dkmd_connector_info *pinfo = dpu_comp->conn_info;
	struct composer_manager *comp_mgr = dpu_comp->comp_mgr;

	(void)pipeline_next_ops_handle(pinfo->conn_device, pinfo, DISABLE_ISR, &dpu_comp->isr_ctrl);
	(void)pipeline_next_off(pinfo->conn_device, pinfo);
	dpu_comp_smmuv3_reset_off(comp_mgr, dpu_comp);

	/* MUST disable all irq before hardware reset
	 * if primary abnormal, but extern is still in the interrupt,
	 * access register will be hang dead
	 */
	list_for_each_entry(isr_ctrl, &comp_mgr->isr_list, list_node) {
		/* only handle other registration  */
		if ((isr_ctrl->irq_no != dpu_comp->isr_ctrl.irq_no) && isr_ctrl->handle_func)
			isr_ctrl->handle_func(isr_ctrl, DKMD_ISR_DISABLE_NO_REF);
	}

	composer_dpu_power_off(comp_mgr);
}

static void composer_manager_underflow_recovery(struct dpu_composer *dpu_comp)
{
	struct dkmd_connector_info *pinfo = dpu_comp->conn_info;
	struct composer_manager *comp_mgr = dpu_comp->comp_mgr;
	struct dkmd_isr *isr_ctrl = NULL;

	composer_dpu_power_on(dpu_comp->comp_mgr);
	dpu_dacc_load();

	list_for_each_entry(isr_ctrl, &comp_mgr->isr_list, list_node) {
		/* only handle other registration  */
		if ((isr_ctrl->irq_no != dpu_comp->isr_ctrl.irq_no) && isr_ctrl->handle_func)
			isr_ctrl->handle_func(isr_ctrl, DKMD_ISR_ENABLE_NO_REF);
	}

	dpu_comp_smmuv3_recovery_on(comp_mgr, dpu_comp);

	dpu_comp_init(dpu_comp);

	(void)pipeline_next_ops_handle(pinfo->conn_device, pinfo, INIT_DSC, pinfo);
	(void)pipeline_next_ops_handle(pinfo->conn_device, pinfo, INIT_SPR, NULL);
	(void)pipeline_next_on(pinfo->conn_device, pinfo);
	(void)pipeline_next_ops_handle(pinfo->conn_device, pinfo, ENABLE_ISR, &dpu_comp->isr_ctrl);
}

void composer_manager_reset_hardware(struct dpu_composer *dpu_comp)
{
	struct comp_online_present *present = NULL;
	struct composer_manager *comp_mgr = dpu_comp->comp_mgr;

	down(&comp_mgr->power_sem);
	if (!comp_mgr->power_status.refcount.value[dpu_comp->comp.index]) {
		up(&comp_mgr->power_sem);
		dpu_pr_warn("comp.index=%d is power off", dpu_comp->comp.index);
		return;
	}

	composer_manager_underflow_reset(dpu_comp);
	composer_manager_underflow_recovery(dpu_comp);
	dpu_pr_info("reset hardware finished!");
	/* Notify the other module for processing */
	comp_mgr->hardware_reseted |= BIT(dpu_comp->comp.index);

	if (!is_offline_panel(&dpu_comp->conn_info->base)) {
		present = (struct comp_online_present *)dpu_comp->present_data;
		present->vactive_start_flag = 1;
	}
	up(&comp_mgr->power_sem);
}
