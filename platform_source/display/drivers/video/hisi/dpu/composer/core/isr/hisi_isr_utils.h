/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#ifndef HISI_ISR_UTILS_H
#define HISI_ISR_UTILS_H

#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/notifier.h>
#include <linux/irqreturn.h>

#include "hisi_disp_panel.h"

/* TODO: the enum define need confirm for charlotte, here is second irq status */
enum {
	IRQ_UNMASK_GLB = 0,
	IRQ_UNMASK_OFFLINE0,
	IRQ_UNMASK_OFFLINE1,
	IRQ_UNMASK_OFFLINE2,
	IRQ_UNMASK_MAX,
};

enum {
	DSI_INT_FRM_START = BIT(0),
	DSI_INT_FRM_END = BIT(1),
	DSI_INT_UNDER_FLOW = BIT(2),
	DSI_INT_VFP_END = BIT(3),
	DSI_INT_VSYNC = BIT(4),
	DSI_INT_VBP = BIT(5),
	DSI_INT_VFP = BIT(6),
	DSI_INT_VACT0_START = BIT(7),
	DSI_INT_VACT0_END = BIT(8),
	DSI_INT_VACT1_START = BIT(9),
	DSI_INT_VACT1_END = BIT(10),
	DSI_INT_LCD_TE1 = BIT(11),
	DSI_INT_LCD_TE0 = BIT(12),

	/* TODO: other irq status bits */
};

enum {
	DPU_CMD_NS_INT = BIT(16),
	DPU_DBCU_TBU_RT_NS_INT = BIT(15),
	DPU_DBCU_TBU_SRT_NS_INT = BIT(14),
	DPU_LBUF_CTL_NS_INT = BIT(13),
	DPU_SMART_DMA0_NS_INT = BIT(12),
	DPU_SMART_DMA1_NS_INT = BIT(11),
	DPU_SMART_DMA2_NS_INT = BIT(10),
	DPU_SMART_DMA3_NS_INT = BIT(9),
	DPU_WCH0_NS_INT = BIT(8),
	DPU_WCH1_NS_INT = BIT(7),
	DPU_WCH2_NS_INT = BIT(6),
	DPU_DACC_NS_INT = BIT(5),
	DPU_DPP0_HIACE_NS_INT = BIT(4),
	DPU_HEMCD0_NS_INT = BIT(3),
	DPU_DDE_NS_INT = BIT(2),
	DPU_DPP1_HIACE_NS_INT = BIT(1),
	DPU_HEMCD1_NS_INT = BIT(0),
};

enum {
	DPU_CMD_INT = BIT(16),
	DPU_DBCU_TBU_RT_INT = BIT(15),
	DPU_DBCU_TBU_SRT_INT = BIT(14),
	DPU_LBUF_CTL_INT = BIT(13),
	DPU_SMART_DMA0_INT = BIT(12),
	DPU_SMART_DMA1_INT = BIT(11),
	DPU_SMART_DMA2_INT = BIT(10),
	DPU_SMART_DMA3_INT = BIT(9),
	DPU_WCH0_INT = BIT(8),
	DPU_WCH1_INT = BIT(7),
	DPU_WCH2_INT = BIT(6),
	DPU_DACC_INT = BIT(5),
	DPU_DPP0_HIACE_INT = BIT(4),
	DPU_HEMCD0_INT = BIT(3),
	DPU_DDE_INT = BIT(2),
	DPU_DPP1_HIACE_INT = BIT(1),
	DPU_HEMCD1_INT = BIT(0),
};

enum {
	WCH_SDMA0_INTS = BIT(0),
	WCH_SDMA1_INTS = BIT(1),
	WCH_SDMA2_INTS = BIT(2),
	WCH_SDMA3_INTS = BIT(3),
	WCH_SDMA4_INTS = BIT(4),
	WCH_SDMA5_INTS = BIT(5),
	WCH_SDMA6_INTS = BIT(6),
	WCH_WLT_END_INTS = BIT(8),
	WCH_WLT_ERR_INTS = BIT(9),
	WCH_BLK_END_INTS = BIT(10),
	WCH_FRM_END_INTS = BIT(11),
};

struct hisi_isr_state_node {
	struct list_head list_node;

	uint32_t unmask_bit;
	struct raw_notifier_head irq_nofitier;
};

struct hisi_disp_isr_ops {
	irqreturn_t (*handle_irq)(int irq, void *ptr);
	void (*interrupt_mask)(char __iomem *dpu_base);
	void (*interrupt_clear)(char __iomem *dpu_base);
	void (*interrupt_unmask)(char __iomem *dpu_base, void *ptr);
};

struct hisi_disp_isr {
	uint32_t irq_no;
	const char *irq_name;
	void *parent;
	uint32_t irq_states[IRQ_UNMASK_MAX];
	struct list_head isr_state_lists[IRQ_UNMASK_MAX]; /* struct hisi_isr_state_node */
	struct hisi_disp_isr_ops *isr_ops;
};

void hisi_disp_isr_open(void);
void hisi_disp_isr_close(void);
void hisi_disp_isr_enable(struct hisi_disp_isr *isr_ctrl);
void hisi_disp_isr_disable(struct hisi_disp_isr *isr_ctrl);
void hisi_disp_isr_setup(struct hisi_disp_isr *isr_ctrl);
void hisi_disp_isr_init_state_lists(struct hisi_disp_isr *isr_ctrl);
int hisi_disp_isr_register_listener(struct hisi_disp_isr *isr_ctrl, struct notifier_block *nb,
		uint32_t isr_state, uint32_t unmask_bit);
int hisi_disp_isr_unregister_listener(struct hisi_disp_isr *isr_ctrl, struct notifier_block *nb,
		uint32_t isr_state, uint32_t unmask_bit);
int hisi_disp_isr_notify_listener(struct hisi_disp_isr *isr_ctrl, uint32_t isr_state, uint32_t unmask_bit);

static inline uint32_t hisi_disp_isr_get_vsync_bit(uint32_t type)
{
	return is_mipi_cmd_panel(type) ? DSI_INT_LCD_TE0 : DSI_INT_VSYNC;
}


#endif /* HISI_ISR_UTILS_H */
