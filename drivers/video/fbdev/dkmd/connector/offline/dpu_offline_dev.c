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

#include "dpu_conn_mgr.h"
#include "dpu_offline_drv.h"

static int32_t dpu_offline_isr_enable(struct dpu_connector *connector, const void *value)
{
	uint32_t mask = ~0;
	struct dkmd_isr *isr_ctrl = (struct dkmd_isr *)value;

	dpu_check_and_return(!isr_ctrl, -1, err, "isr_ctrl is null!");

	/* 1. interrupt mask */
	outp32(DPU_GLB_NS_MDP_TO_GIC_MSK_ADDR(connector->connector_base + DPU_GLB0_OFFSET), mask);
	outp32(DPU_GLB_WCH0_NS_INT_MSK_ADDR(connector->connector_base + DPU_GLB0_OFFSET), mask);
	outp32(DPU_GLB_WCH1_NS_INT_MSK_ADDR(connector->connector_base + DPU_GLB0_OFFSET), mask);
	outp32(DPU_GLB_WCH2_NS_INT_MSK_ADDR(connector->connector_base + DPU_GLB0_OFFSET), mask);

	/* 2. enable irq */
	isr_ctrl->handle_func(isr_ctrl, DKMD_ISR_ENABLE);

	/* 3. interrupt clear */
	outp32(DPU_GLB_NS_MDP_TO_GIC_O_ADDR(connector->connector_base + DPU_GLB0_OFFSET), mask);

	/* 4. interrupt umask */
	outp32(DPU_GLB_NS_MDP_TO_GIC_MSK_ADDR(connector->connector_base + DPU_GLB0_OFFSET), isr_ctrl->unmask);
	outp32(DPU_GLB_WCH0_NS_INT_MSK_ADDR(connector->connector_base + DPU_GLB0_OFFSET), ~(WCH_FRM_END_INTS));
	outp32(DPU_GLB_WCH1_NS_INT_MSK_ADDR(connector->connector_base + DPU_GLB0_OFFSET), ~(WCH_FRM_END_INTS));
	outp32(DPU_GLB_WCH2_NS_INT_MSK_ADDR(connector->connector_base + DPU_GLB0_OFFSET), ~(WCH_FRM_END_INTS));
	return 0;
}

static int32_t dpu_offline_isr_disable(struct dpu_connector *connector, const void *value)
{
	uint32_t mask = ~0;
	struct dkmd_isr *isr_ctrl = (struct dkmd_isr *)value;

	dpu_check_and_return(!isr_ctrl, -1, err, "isr_ctrl is null!");

	/* 1. interrupt mask */
	outp32(DPU_GLB_NS_MDP_TO_GIC_MSK_ADDR(connector->connector_base + DPU_GLB0_OFFSET), mask);
	outp32(DPU_GLB_WCH0_NS_INT_MSK_ADDR(connector->connector_base + DPU_GLB0_OFFSET), mask);
	outp32(DPU_GLB_WCH1_NS_INT_MSK_ADDR(connector->connector_base + DPU_GLB0_OFFSET), mask);
	outp32(DPU_GLB_WCH2_NS_INT_MSK_ADDR(connector->connector_base + DPU_GLB0_OFFSET), mask);

	/* 2. disable irq */
	isr_ctrl->handle_func(isr_ctrl, DKMD_ISR_DISABLE);

	return 0;
}

irqreturn_t dpu_offline_isr(int32_t irq, void *ptr)
{
	uint32_t isr1_offline_glb_state = 0;
	uint32_t isr2_offline_state = 0;
	char __iomem *connector_base = NULL;
	struct dpu_connector *connector = NULL;
	struct dkmd_isr *isr_ctrl = (struct dkmd_isr *)ptr;

	dpu_check_and_return(!isr_ctrl, IRQ_NONE, err, "isr_ctrl is null!");

	connector = (struct dpu_connector *)isr_ctrl->parent;
	dpu_check_and_return(!connector, IRQ_NONE, err, "connector is null!");

	connector_base = connector->connector_base;
	/**
	 * @brief What scene decided to read the information according to the configuration of offline compose
	 *
	 */
	isr1_offline_glb_state = inp32(DPU_GLB_NS_MDP_TO_GIC_O_ADDR(connector_base + DPU_GLB0_OFFSET));
	if ((isr1_offline_glb_state & DPU_WCH0_NS_INT) == DPU_WCH0_NS_INT) {
		isr2_offline_state = inp32(DPU_GLB_WCH0_NS_INT_O_ADDR(connector_base + DPU_GLB0_OFFSET));
		outp32(DPU_GLB_WCH0_NS_INT_O_ADDR(connector_base + DPU_GLB0_OFFSET), isr2_offline_state);
	}

	if ((isr1_offline_glb_state & DPU_WCH1_NS_INT) == DPU_WCH1_NS_INT) {
		isr2_offline_state = inp32(DPU_GLB_WCH1_NS_INT_O_ADDR(connector_base + DPU_GLB0_OFFSET));
		outp32(DPU_GLB_WCH1_NS_INT_O_ADDR(connector_base + DPU_GLB0_OFFSET), isr2_offline_state);
	}

	if ((isr1_offline_glb_state & DPU_WCH2_NS_INT) == DPU_WCH2_NS_INT) {
		isr2_offline_state = inp32(DPU_GLB_WCH2_NS_INT_O_ADDR(connector_base + DPU_GLB0_OFFSET));
		outp32(DPU_GLB_WCH2_NS_INT_O_ADDR(connector_base + DPU_GLB0_OFFSET), isr2_offline_state);
	}

	outp32(DPU_GLB_NS_MDP_TO_GIC_O_ADDR(connector_base + DPU_GLB0_OFFSET), isr1_offline_glb_state);
	dpu_pr_info("isr1_offline_glb_state=%#x isr2_offline_state=%#x", isr1_offline_glb_state, isr2_offline_state);
	if ((isr2_offline_state & WCH_FRM_END_INTS) == WCH_FRM_END_INTS)
		dkmd_isr_notify_listener(isr_ctrl, WCH_FRM_END_INTS);

	return IRQ_HANDLED;
}


static int32_t dpu_offline_isr_setup(struct dpu_connector *connector, const void *value)
{
	struct dkmd_isr *isr_ctrl = (struct dkmd_isr *)value;

	(void)snprintf(isr_ctrl->irq_name, sizeof(isr_ctrl->irq_name), "irq_offline_%d", connector->pipe_sw_post_chn_idx);
	isr_ctrl->irq_no = connector->connector_irq;
	isr_ctrl->isr_fnc = dpu_offline_isr;
	isr_ctrl->parent = connector;
	isr_ctrl->unmask = ~(DPU_WCH0_NS_INT | DPU_WCH1_NS_INT | DPU_WCH2_NS_INT);

	return 0;
}

struct connector_ops_handle_data offline_ops_table[] = {
	{ SETUP_ISR, dpu_offline_isr_setup },
	{ ENABLE_ISR, dpu_offline_isr_enable },
	{ DISABLE_ISR, dpu_offline_isr_disable },
};

static int32_t dpu_offline_ops_handle(struct dkmd_connector_info *pinfo, uint32_t ops_cmd_id, void *value)
{
	struct dpu_connector *connector = NULL;

	dpu_check_and_return(!pinfo, -EINVAL, err, "pinfo is NULL!");
	connector = get_primary_connector(pinfo);
	dpu_check_and_return(!connector, -EINVAL, err, "connector is NULL!");

	return dkdm_connector_hanlde_func(offline_ops_table, ARRAY_SIZE(offline_ops_table),
									  ops_cmd_id, connector, value);
}

void dpu_offline_default_setup(struct dpu_connector *connector)
{
	connector->on_func = NULL;
	connector->off_func = NULL;
	connector->ops_handle_func = dpu_offline_ops_handle;
}