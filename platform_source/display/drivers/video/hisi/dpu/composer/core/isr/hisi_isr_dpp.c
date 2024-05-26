/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 * GNU General Public License for more details.
 *
 */
#include <securec.h>
#include "hisi_isr_dpp.h"
#include "hisi_disp_config.h"
#include "hisi_disp_dpp.h"
#include "hisi_disp_gadgets.h"
#include "soc_dte_interface.h"
#include <linux/types.h>

void dpu_effect_hiace_trigger_wq(struct hisi_dpp_data *dpp,
	bool is_idle_display) {
	// Check whether the system enters the idle state. If no, need queue work
	// if yes, clear the current hiace interrupt and no need queue work
	if (!is_idle_display) {
		queue_work(dpp->hiace_end_wq, &dpp->hiace_end_work);
		disp_pr_info("queue_work");
	} else {
		/* clear hiace interrupt */
		dpu_set_reg(DPU_HIACE_ACE_INT_STAT_ADDR(dpp->dpu_base + DSS_HIACE_OFFSET),
		0x1, 1, 0);
	}
}

void hisi_dpp0_interrupt_clear(char __iomem *dpu_base) {
	uint32_t clear = ~0;
	disp_pr_info("dpp ++++ ");

	if (!dpu_base) {
		disp_pr_err("dpu_base is null");
		return;
	}

	/* glb/dpp/hiace int */
	dpu_set_reg(DPU_GLB_NS_MDP_TO_GIC_O_ADDR(dpu_base + DSS_GLB0_OFFSET), clear,
	  1, 4);
	dpu_set_reg(DPU_DPP_INTS_ADDR(dpu_base + DSS_DPP0_OFFSET), clear, 1, 0);
	dpu_set_reg(DPU_HIACE_ACE_INT_STAT_ADDR(dpu_base + DSS_HIACE_OFFSET), clear,
	  1, 0);
}

irqreturn_t hisi_dpp_handle_irq(int irq, void *ptr) {
	struct hisi_disp_dpp *dpp_dev = NULL;
	struct hisi_dpp_data *dpp_data = NULL;
	struct hisi_disp_isr *isr_ctrl = NULL;
	char __iomem *dpu_base = NULL;
	uint32_t isr_dpp0_ns_state = 0;
	uint32_t isr_dpp0_state = 0;
	uint32_t isr_ace_state = 0;

	if (!ptr) {
		disp_pr_err("ptr is null");
		return IRQ_NONE;
	}

	dpp_dev = (struct hisi_disp_dpp *)ptr;
	if (!dpp_dev)
		return IRQ_NONE;

	dpp_data = &dpp_dev->dpp_data;
	dpu_base = dpp_data->dpu_base;

	isr_dpp0_ns_state =
			inp32(DPU_GLB_NS_MDP_TO_GIC_O_ADDR(dpu_base + DSS_GLB0_OFFSET));

	disp_pr_info(" isr_dpp0_ns_state = 0x%x, dpu_base %p\n",
							 isr_dpp0_ns_state, dpu_base);
	outp32(DPU_GLB_NS_MDP_TO_GIC_O_ADDR(dpu_base + DSS_GLB0_OFFSET), isr_dpp0_ns_state);

	isr_dpp0_state = inp32(DPU_DPP_INTS_ADDR(dpu_base + DSS_DPP0_OFFSET));

	outp32(DPU_DPP_INTS_ADDR(dpu_base + DSS_DPP0_OFFSET), isr_dpp0_state);

	isr_ace_state =
			inp32(DPU_HIACE_ACE_INT_STAT_ADDR(dpu_base + DSS_HIACE_OFFSET));
	disp_pr_info(" isr_ace_state = 0x%x\n", isr_ace_state);
	outp32(DPU_HIACE_ACE_INT_STAT_ADDR(dpu_base + DSS_HIACE_OFFSET),
				 isr_ace_state);

	if (irq == dpp_data->isr_ctrl[COMP_ISR_PRE].irq_no)
		isr_ctrl = &dpp_data->isr_ctrl[COMP_ISR_PRE];

	if (!isr_ctrl) {
		disp_pr_err("isr_ctrl is null, dpp_data->isr_ctrl[COMP_ISR_PRE].irq_no = "
		"%d, irq =%d",
		dpp_data->isr_ctrl[COMP_ISR_PRE].irq_no, irq);
		return IRQ_HANDLED;
	}
	disp_pr_info(" success dpp_data->isr_ctrl[COMP_ISR_PRE].irq_no = %d, irq "
	"=%d, isr_ace_state = 0x%x\n",
	dpp_data->isr_ctrl[COMP_ISR_PRE].irq_no, irq, isr_ace_state);

	if (isr_ace_state & BIT_HIACE_IND) {
		if (dpp_data->hiace_end_wq)
			dpu_effect_hiace_trigger_wq(dpp_data, false);
	}
	return IRQ_HANDLED;
}

void hisi_dpp0_interrupt_mask(char __iomem *dpu_base) {
	uint32_t mask = ~0;
	if (!dpu_base) {
		disp_pr_err("ptr is null");
		return;
	}
	disp_pr_info("dpp ++++\n ");

	dpu_set_reg(DPU_GLB_NS_MDP_TO_GIC_MSK_ADDR(dpu_base + DSS_GLB0_OFFSET), mask, 1, 4);
	dpu_set_reg(DPU_DPP_INT_MSK_ADDR(dpu_base + DSS_DPP0_OFFSET), mask, 1, 0);
	dpu_set_reg(DPU_HIACE_ACE_INT_UNMASK_ADDR(dpu_base + DSS_HIACE_OFFSET), 0, 1, 0);
}

void hisi_dpp0_interrupt_unmask(char __iomem *dpu_base, void *ptr) {
	uint32_t unmask = ~0;
	struct hisi_disp_dpp *dpp_device = NULL;

	if (!dpu_base) {
		disp_pr_err("ptr is null");
		return;
	}
	dpp_device = (struct hisi_disp_dpp *)ptr;

	disp_pr_info("+++++");

	/* mask register 0 means permit */
	dpu_set_reg(DPU_DPP_INT_MSK_ADDR(dpu_base + DSS_DPP0_OFFSET), 0, 1, 0);

	/* unmask register 1 means permit */
	unmask &= BIT_HIACE_IND;
	dpu_set_reg(DPU_HIACE_ACE_INT_UNMASK_ADDR(dpu_base + DSS_HIACE_OFFSET),
	unmask, 1, 0);

	dpu_set_reg(DPU_GLB_NS_MDP_TO_GIC_MSK_ADDR(dpu_base + DSS_GLB0_OFFSET), 0, 1,
	4);
}

void hisi_dpp_isr_setup(struct hisi_disp_isr *isr_ctrl) {
	int ret;
	disp_pr_info(" ++++ ");
	if (NULL == isr_ctrl)
		disp_pr_err("isr_ctrl is NULL");

	ret = request_irq(isr_ctrl->irq_no, isr_ctrl->isr_ops->handle_irq, 0,
	isr_ctrl->irq_name, isr_ctrl->parent);
	if (ret) {
		disp_pr_err("setup irq fail %s, isr_ctrl->irq_no %d", isr_ctrl->irq_name,
		isr_ctrl->irq_no);
		return;
	}

	disable_irq(isr_ctrl->irq_no);
	disp_pr_info(" ---- ");
}

static struct hisi_disp_isr_ops g_dpp_isr_ops = {
		.handle_irq = hisi_dpp_handle_irq,
		.interrupt_mask = hisi_dpp0_interrupt_mask,
		.interrupt_clear = hisi_dpp0_interrupt_clear,
		.interrupt_unmask = hisi_dpp0_interrupt_unmask,
};

void hisi_dpp_isr_init(struct hisi_disp_isr *isr, const uint32_t irq_no,
	uint32_t *irq_state, uint32_t state_count,
	const char *irq_name, void *parent) {
	uint32_t i;

	disp_pr_info(" ++++ ");
	if (!irq_state)
		return;

	disp_pr_info("dpp enter irq =%d,irq_unmask 0x%x", irq_no, *irq_state);

	memcpy_s(isr->irq_states, state_count * sizeof(uint32_t),
		irq_state, state_count * sizeof(uint32_t));
	for (i = 0; i < state_count; i++)
		disp_pr_info("isr->irq_states[%d] =0x%x", i, isr->irq_states[i]);

	isr->irq_name = irq_name;
	isr->irq_no = irq_no;
	isr->parent = parent;
	isr->isr_ops = &g_dpp_isr_ops;
	disp_pr_info(" irq_name:%s, irq_no:%d", irq_name, irq_no);

	hisi_dpp_isr_setup(isr);
}
