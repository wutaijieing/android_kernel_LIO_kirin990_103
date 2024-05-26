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

#include <linux/types.h>
#include "hisi_isr_offline.h"
#include "hisi_disp_composer.h"

void hisi_offline_handle_second_irq(struct hisi_disp_isr *isr_ctrl, char __iomem *dpu_base, uint32_t isr_s1)
{
	uint32_t isr_wch0_state = 0;
	uint32_t isr_wch1_state = 0;
	uint32_t isr_wch2_state = 0;
	char __iomem *wch0_ints_addr;
	char __iomem *wch0_msk_addr;
	char __iomem *wch1_ints_addr;
	char __iomem *wch1_msk_addr;
	char __iomem *wch2_ints_addr;
	char __iomem *wch2_msk_addr;

	disp_pr_info("enter+++");
	if (isr_s1 & DPU_WCH0_NS_INT) {
		wch0_ints_addr = DPU_GLB_NS_OFFLINE0_TO_GIC_O_ADDR(dpu_base + DSS_GLB0_OFFSET);
		wch0_msk_addr = DPU_GLB_NS_OFFLINE0_TO_GIC_MSK_ADDR(dpu_base + DSS_GLB0_OFFSET);

		isr_wch0_state = inp32(wch0_ints_addr);
		outp32(wch0_ints_addr, isr_wch0_state);
		isr_wch0_state &= ~((uint32_t)inp32(wch0_msk_addr));

		hisi_disp_isr_notify_listener(isr_ctrl, IRQ_UNMASK_OFFLINE0, isr_wch0_state);
	}

	if (isr_s1 & DPU_WCH1_NS_INT) {
		disp_pr_info("isr_s1 = %d", isr_s1);
		wch1_ints_addr = DPU_GLB_NS_OFFLINE1_TO_GIC_O_ADDR(dpu_base + DSS_GLB0_OFFSET);
		wch1_msk_addr = DPU_GLB_NS_OFFLINE1_TO_GIC_MSK_ADDR(dpu_base + DSS_GLB0_OFFSET);

		isr_wch1_state = inp32(wch1_ints_addr);
		outp32(wch1_ints_addr, isr_wch1_state);
		isr_wch1_state &= ~((uint32_t)inp32(wch1_msk_addr));

		hisi_disp_isr_notify_listener(isr_ctrl, IRQ_UNMASK_OFFLINE0, isr_wch1_state);
	}

	if (isr_s1 & DPU_WCH2_NS_INT) {
		wch2_ints_addr = DPU_GLB_NS_OFFLINE2_TO_GIC_O_ADDR(dpu_base + DSS_GLB0_OFFSET);
		wch2_msk_addr = DPU_GLB_NS_OFFLINE2_TO_GIC_MSK_ADDR(dpu_base + DSS_GLB0_OFFSET);

		isr_wch2_state = inp32(wch2_ints_addr);
		outp32(wch2_ints_addr, isr_wch2_state);
		isr_wch2_state &= ~((uint32_t)inp32(wch2_msk_addr));

		hisi_disp_isr_notify_listener(isr_ctrl, IRQ_UNMASK_OFFLINE0, isr_wch2_state);
	}
	disp_pr_info("exit---");
}

static uint32_t g_test_wch_frm_cnt = 0;
irqreturn_t hisi_offline_handle_irq(int irq, void *ptr)
{
	struct hisi_composer_device *comp_dev = (struct hisi_composer_device *)ptr;
	struct hisi_disp_isr *isr_ctrl = NULL;
	uint32_t isr_off0_glb_state = 0;
	uint32_t isr2 = 0;
	char __iomem *dpu_base = comp_dev->ov_data.ip_base_addrs[DISP_IP_BASE_DPU];

	if (!comp_dev)
		return IRQ_NONE;

	char __iomem *off0_ints_addr = DPU_GLB_NS_OFFLINE0_TO_GIC_O_ADDR(dpu_base + DSS_GLB0_OFFSET);
	char __iomem *off0_mask_addr = DPU_GLB_NS_OFFLINE0_TO_GIC_MSK_ADDR(dpu_base + DSS_GLB0_OFFSET);
	uint32_t i = 0;
	g_test_wch_frm_cnt++;

	isr_off0_glb_state = inp32(off0_ints_addr);
	disp_pr_info("irq:%d, isr_off0_glb_state = 0x%x", irq, isr_off0_glb_state);
	outp32(off0_ints_addr, isr_off0_glb_state);
	isr_off0_glb_state &= ~((uint32_t)inp32(off0_mask_addr));

	/* TODO: read other isr status */

	for (i = 0; i < COMP_ISR_MAX; i++) {
		if (irq == comp_dev->ov_data.isr_ctrl[i].irq_no)
			isr_ctrl = &comp_dev->ov_data.isr_ctrl[i];
	}

	if (!isr_ctrl) {
		disp_pr_err("isr_ctrl is null, irq =%d", irq);
		return IRQ_HANDLED;
	}

	if (isr_off0_glb_state & DPU_WCH1_NS_INT) {
		disp_pr_info("isr_s1 = 0x%x", isr_off0_glb_state);
		isr2 = inp32(DPU_GLB_WCH1_NS_INT_O_ADDR(dpu_base + DSS_GLB0_OFFSET));
		outp32(DPU_GLB_WCH1_NS_INT_O_ADDR(dpu_base + DSS_GLB0_OFFSET), isr2);
		disp_pr_info("isr_s2 = 0x%x", isr2);
		if (isr2 & BIT(13))
			disp_pr_info("wch_frm_start, wch_frm_cnt = %u", g_test_wch_frm_cnt);
		if (isr2 & BIT(11))
			disp_pr_info("wch_frm_end, wch_frm_cnt = %u", g_test_wch_frm_cnt);
	}
	return IRQ_HANDLED;
}

void hisi_offline_interrupt_clear(char __iomem *dpu_base)
{
	uint32_t clear = ~0;

	outp32(DPU_GLB_NS_OFFLINE0_TO_GIC_O_ADDR(dpu_base + DSS_GLB0_OFFSET), clear);
}

void hisi_offline_interrupt_mask(char __iomem *dpu_base)
{
	uint32_t mask = ~0;

	outp32(DPU_GLB_NS_OFFLINE0_TO_GIC_MSK_ADDR(dpu_base + DSS_GLB0_OFFSET), mask);
	outp32(DPU_GLB_NS_OFFLINE1_TO_GIC_MSK_ADDR(dpu_base + DSS_GLB0_OFFSET), mask);
	outp32(DPU_GLB_NS_OFFLINE2_TO_GIC_MSK_ADDR(dpu_base + DSS_GLB0_OFFSET), mask);
	outp32(DPU_GLB_NS_DP0_TO_IOMCU_MSK_ADDR(dpu_base + DSS_GLB0_OFFSET), mask);
	outp32(DPU_GLB_NS_DP1_TO_IOMCU_MSK_ADDR(dpu_base + DSS_GLB0_OFFSET), mask);
}

void hisi_offline_interrupt_unmask(char __iomem *dpu_base, void *ptr)
{
	struct hisi_composer_data *ov_data = ptr;
	uint32_t unmask = ~0;

	unmask &= ~(BIT(19) | DPU_WCH1_NS_INT);
	outp32(DPU_GLB_NS_OFFLINE0_TO_GIC_MSK_ADDR(dpu_base + DSS_GLB0_OFFSET), unmask);

	outp32(DPU_GLB_WCH1_NS_INT_MSK_ADDR(dpu_base + DSS_GLB0_OFFSET), 0);
}

static struct hisi_disp_isr_ops g_offline_isr_ops = {
	.handle_irq = hisi_offline_handle_irq,
	.interrupt_mask = hisi_offline_interrupt_mask,
	.interrupt_unmask = hisi_offline_interrupt_unmask,
	.interrupt_clear = hisi_offline_interrupt_clear,
};

static int hisi_offline_isr_notify(struct notifier_block *self, unsigned long action, void *data)
{
	struct hisi_composer_device *dev = (struct hisi_composer_device *)data;

	disp_pr_info("action = %d wb_flag = %d", action, dev->ov_data.offline_isr_info->wb_flag[WB_TYPE_WCH1]);
	if (action & WCH_FRM_END_INTS) {
		if (dev->ov_data.offline_isr_info->wb_flag[WB_TYPE_WCH1] == 1) {
			dev->ov_data.offline_isr_info->wb_done[WB_TYPE_WCH1] = 1;
			wake_up_interruptible_all(&(dev->ov_data.offline_isr_info->wb_wq[WB_TYPE_WCH1]));
		}
	}

	//TODO: wch0/wch2

	return 0;
}

static struct notifier_block offline_isr_notifier = {
	.notifier_call = hisi_offline_isr_notify,
};

void hisi_offline_isr_init(struct hisi_disp_isr *isr, const uint32_t irq_no,
		uint32_t *irq_state, uint32_t state_count, const char *irq_name, void *parent)
{
	uint32_t irq_mask = 0;
	if (!irq_state)
		return;

	disp_pr_info("enter+++ irq_no = %d", irq_no);
	memcpy(isr->irq_states, irq_state, state_count * sizeof(uint32_t));

	isr->irq_name = irq_name;
	isr->irq_no = irq_no;
	isr->parent = parent;
	isr->isr_ops = &g_offline_isr_ops;

	hisi_disp_isr_init_state_lists(isr);

	hisi_disp_isr_setup(isr);

	irq_mask = (DPU_WCH1_NS_INT | BIT(19));
	hisi_disp_isr_register_listener(isr, &offline_isr_notifier, IRQ_UNMASK_OFFLINE0, irq_state[IRQ_UNMASK_OFFLINE0]);

	disp_pr_info("exit---");
}

