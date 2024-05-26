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

#ifndef DKMD_ISR_H
#define DKMD_ISR_H

#include <linux/types.h>
#include <linux/notifier.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/atomic.h>

#define ISR_LISTENER_LIST_COUNT   32
#define ISR_NAME_SIZE   64

enum {
	DKMD_ISR_RELEASE = 1,
	DKMD_ISR_DISABLE,
	DKMD_ISR_ENABLE,
	DKMD_ISR_DISABLE_NO_REF,
	DKMD_ISR_ENABLE_NO_REF
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
	WCH_SDMA4_END_INTS = BIT(4),
	WCH_WLT_END_INTS = BIT(8),
	WCH_WLT_ERR_INTS = BIT(9),
	WCH_BLK_END_INTS = BIT(10),
	WCH_FRM_END_INTS = BIT(11),
};

enum {
	DPU_M1_QIC_INTR_ERR_TB_DSS_CFG = BIT(28),
	DPU_M1_QIC_INTR_SAFETY_TB_DSS_CFG = BIT(27),
	DPU_M1_QIC_INTR_SAFETY_ERR_TB_DSS_CFG = BIT(26),
	DPU_M1_QIC_INTR_ERR_IB_DSS_RT0_DATA_RD = BIT(24),
	DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT0_DATA_RD = BIT(23),
	DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT0_DATA_RD = BIT(22),
	DPU_M1_QIC_INTR_ERR_IB_DSS_RT0_DATA_WR = BIT(20),
	DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT0_DATA_WR = BIT(19),
	DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT0_DATA_WR = BIT(18),
	DPU_M1_QIC_INTR_ERR_IB_DSS_RT1_DATA_RD = BIT(16),
	DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT1_DATA_RD = BIT(15),
	DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT1_DATA_RD = BIT(14),
	DPU_M1_QIC_INTR_ERR_IB_DSS_RT2_DATA_RD = BIT(12),
	DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT2_DATA_RD = BIT(11),
	DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT2_DATA_RD = BIT(10),
	DPU_M1_QIC_INTR_ERR_IB_DSS_RT2_DATA_WR = BIT(8),
	DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT2_DATA_WR = BIT(7),
	DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT2_DATA_WR = BIT(6),
	DPU_M1_QIC_INTR_ERR_IB_DSS_RT3_DATA_RD = BIT(4),
	DPU_M1_QIC_INTR_SAFETY_IB_DSS_RT3_DATA_RD = BIT(3),
	DPU_M1_QIC_INTR_SAFETY_ERR_IB_DSS_RT3_DATA_RD = BIT(2),
	DPU_M1_QIC_INTR_ERR_RT_0_RD_REQ = BIT(0),
};

enum {
	DPU_M1_QIC_INTR_ERR_RT_0_RD_RSP = BIT(22),
	DPU_M1_QIC_INTR_ERR_RT_0_REQ = BIT(18),
	DPU_M1_QIC_INTR_ERR_RT_0_RSP = BIT(14),
	DPU_M1_QIC_INTR_ERR_RT_QCP_DSS_CFG_REQ = BIT(10),
	DPU_M1_QIC_INTR_ERR_RT_QCP_DSS_CFG_RSP = BIT(6),
	DPU_M1_QIC_INTR_ERR_TB_QCP_DSS = BIT(2),
	DPU_M1_QIC_INTR_SAFETY_TB_QCP_DSS = BIT(1),
	DPU_M1_QIC_INTR_SAFETY_ERR_TB_QCP_DSS = BIT(0),
};

struct dkmd_isr_listener_node {
	struct list_head list_node;

	void *data;
	uint32_t listen_bit;
	struct raw_notifier_head irq_nofitier;
};

struct dkmd_isr {
	int32_t irq_no;
	char irq_name[ISR_NAME_SIZE];
	void *parent;
	uint32_t unmask;
	atomic_t refcount;
	struct list_head list_node;

	void (*handle_func)(struct dkmd_isr *isr_ctrl, uint32_t handle_event);
	irqreturn_t (*isr_fnc)(int32_t irq, void *ptr);
	struct list_head isr_listener_list[ISR_LISTENER_LIST_COUNT];
};

void dkmd_isr_setup(struct dkmd_isr *isr_ctrl);
int32_t dkmd_isr_notify_listener(struct dkmd_isr *isr_ctrl, uint32_t listen_bit);
int32_t dkmd_isr_unregister_listener(struct dkmd_isr *isr_ctrl, struct notifier_block *nb, uint32_t listen_bit);
int32_t dkmd_isr_register_listener(struct dkmd_isr *isr_ctrl,
	struct notifier_block *nb, uint32_t listen_bit, void *listener_data);

#endif