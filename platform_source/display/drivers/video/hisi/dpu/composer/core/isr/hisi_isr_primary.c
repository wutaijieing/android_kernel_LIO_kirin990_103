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

#include "hisi_fb.h"
#include <linux/types.h>
#include "hisi_isr_primary.h"
#include "hisi_disp_composer.h"
#include "vsync/hisi_disp_vactive.h"
#include "hisi_disp_config.h"
#include "hisi_fb_vsync.h"
#include "../../operators/dpp/hisi_operator_dpp.h"
#include "../../operators/hisi_operators_manager.h"
#include "../../../effect/display_effect_alsc.h"

extern struct hisi_fb_info *hfb_info_list[HISI_FB_MAX_FBI_LIST];

static void hisi_te_vsync_handle(struct hisi_composer_device *comp_dev, uint32_t isr_glb_state)
{
	struct hisi_fb_info *hfb_info = hfb_info_list[PRIMARY_PANEL_IDX];//platform_get_drvdata(comp_dev->pdev);
	if (!hfb_info) {
		disp_pr_err("hfb_info is NULL\n");
		return;
	}

	if (hfb_info->index != PRIMARY_PANEL_IDX) {
		return;
	}

	if (isr_glb_state & DSI_INT_LCD_TE0) {
		//disp_pr_info("fb%d, hfb_info:%p, isr handle:%p\n", hfb_info->index, hfb_info, hfb_info->fb_priv_ops.vsync_ops.vsync_isr_handler);
		//disp_pr_info("%d, te handle\n", isr_glb_state);
		if (hfb_info->fb_priv_ops.vsync_ops.vsync_isr_handler) {
			hfb_info->fb_priv_ops.vsync_ops.vsync_isr_handler(hfb_info);
		}
	}
}

static void alsc_store_data(uint32_t isr_glb_state, struct hisi_composer_device *comp_dev)
{
	struct hisi_comp_operator *dpp_op = NULL;
	union operator_id id_dpp;
	if (g_noise_got == false) {
		if (isr_glb_state & BIT_VACTIVE0_END) {
			if (g_reg_inited == false)
				return;
			id_dpp.id = 0;
			id_dpp.info.type = COMP_OPS_DPP;
			id_dpp.info.idx= 0;
			dpp_op = hisi_disp_get_operator(id_dpp);
			hisi_alsc_store_data((struct hisi_operator_dpp *)dpp_op, BIT_VACTIVE0_END,
				comp_dev->ov_data.ip_base_addrs[DISP_IP_BASE_DPU]);
		}
	}
}

irqreturn_t hisi_primary_handle_irq(int irq, void *ptr)
{
	struct hisi_composer_device *comp_dev = (struct hisi_composer_device *)ptr;
	struct hisi_disp_isr *isr_ctrl = NULL;
	char __iomem *dpu_base_addr = hisi_config_get_ip_base(DISP_IP_BASE_DPU);
	uint32_t isr_glb_state = 0;
	uint32_t isr_dsi_state = 0;
	uint32_t isr_dpp_state = 0;
	uint32_t i = 0;

	if (!comp_dev)
		return IRQ_NONE;

#ifndef CONFIG_DKMD_DPU_V720
	hisi_vactive0_set_event(1);
#endif

	/* TODO: read isr status */
	isr_glb_state = inp32(comp_dev->ov_data.ip_base_addrs[DISP_IP_BASE_DPU] + 0x248);
	outp32(comp_dev->ov_data.ip_base_addrs[DISP_IP_BASE_DPU] + 0x248, isr_glb_state);

	if (isr_glb_state & DSI_INT_UNDER_FLOW)
		disp_pr_err("underflow, isr_glb_state =%d", isr_glb_state);

	hisi_te_vsync_handle(comp_dev, isr_glb_state);
#ifdef CONFIG_DPP_ALSC
	alsc_store_data(isr_glb_state, comp_dev);
#endif
	isr_glb_state &= ~((uint32_t)inp32(comp_dev->ov_data.ip_base_addrs[DISP_IP_BASE_DPU] + 0x024C));


	for (i = 0; i < COMP_ISR_MAX; i++) {
		if (irq == comp_dev->ov_data.isr_ctrl[i].irq_no)
			isr_ctrl = &comp_dev->ov_data.isr_ctrl[i];
	}

	if (!isr_ctrl) {
		disp_pr_err("isr_ctrl is null, irq =%d", irq);
		return IRQ_HANDLED;
	}

	hisi_disp_isr_notify_listener(isr_ctrl, IRQ_UNMASK_GLB, isr_glb_state);
	// hisi_disp_isr_notify_listener(isr_ctrl, IRQ_UNMASK_MODULE_1, isr_dsi_state);
	// hisi_disp_isr_notify_listener(isr_ctrl, IRQ_UNMASK_MODULE_2, isr_dsi_state);
	// hisi_disp_isr_notify_listener(isr_ctrl, IRQ_UNMASK_MODULE_3, isr_dsi_state);

	return IRQ_HANDLED;
}

static void hisi_interrupt_mask(char __iomem *dpu_base)
{
	uint32_t mask = ~0;

	disp_pr_info("++++");
	outp32(DPU_DSI_CPU_ITF_INT_MSK_ADDR(dpu_base + DPU_MIPI_DSI0_OFFSET), mask);
}

static void hisi_interrupt_unmask(char __iomem *dpu_base, void *ptr)
{
	struct hisi_composer_data *ov_data = (struct hisi_composer_data *)ptr;
	uint32_t unmask;
	uint32_t vsync_mask;

	disp_pr_info(" ++++ ");

	vsync_mask = hisi_disp_isr_get_vsync_bit(ov_data->fix_panel_info->type);
	disp_pr_info("vsync_mask =%d", vsync_mask);

	unmask = ~(DSI_INT_UNDER_FLOW | DSI_INT_VACT0_START | vsync_mask);
	outp32(DPU_DSI_CPU_ITF_INT_MSK_ADDR(dpu_base + DPU_MIPI_DSI0_OFFSET), unmask);
}

static void hisi_interrupt_clear(char __iomem *dpu_base)
{
	disp_pr_info(" ++++ ");
	outp32(dpu_base + 0x248, 0);
}

static struct hisi_disp_isr_ops g_primary_isr_ops = {
	.handle_irq = hisi_primary_handle_irq,
	.interrupt_mask = hisi_interrupt_mask,
	.interrupt_clear = hisi_interrupt_clear,
	.interrupt_unmask = hisi_interrupt_unmask,
};

void hisi_primary_isr_init(struct hisi_disp_isr *isr, const uint32_t irq_no,
		uint32_t *irq_state, uint32_t state_count, const char *irq_name, void *parent)
{
	uint32_t i;

	disp_pr_info(" ++++ ");
	if (!irq_state)
		return;

	disp_pr_info("enter irq =%d,irq_unmask 0x%x", irq_no, *irq_state);

	memcpy(isr->irq_states, irq_state, state_count * sizeof(uint32_t));
	for (i = 0; i < state_count; i++)
		disp_pr_info("isr->irq_states[%d] =0x%x", i, isr->irq_states[i]);

	isr->irq_name = irq_name;
	isr->irq_no = irq_no;
	isr->parent = parent;
	isr->isr_ops = &g_primary_isr_ops;
	disp_pr_info(" irq_name:%s, irq_no:%d", irq_name, irq_no);
	hisi_disp_isr_init_state_lists(isr);

	hisi_disp_isr_setup(isr);
}

