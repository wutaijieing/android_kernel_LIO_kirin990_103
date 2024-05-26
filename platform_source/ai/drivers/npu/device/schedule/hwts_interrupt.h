/*
 * hwts_interrupt.h
 *
 * about hwts interrupt
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef _HWTS_INTERRUPT_H
#define _HWTS_INTERRUPT_H

#include <linux/types.h>

enum hwts_irq_type {
	HWTS_IRQ_NORMAL = 0,
	HWTS_IRQ_ERROR,
	HWTS_IRQ_TYPE_MAX,
};

enum hwts_normal_interrupt {
	HWTS_SQE_DONE_NS = 0x00,
	HWTS_PRE_PAUSED_NS = 0x01,
	HWTS_POST_PAUSED_NS = 0x02,
	HWTS_CQ_FULL_NS = 0x03,
	HWTS_TASK_PAUSED_NS = 0x04,
	HWTS_RESERVED = 0x05,
	HWTS_L2_BUF_SWAP_IN_NS = 0x06,
	HWTS_L2_BUF_SWAP_OUT_NS = 0x07,
	HWTS_SQ_DONE_NS = 0x08,
	HWTS_CQE_WRITTEN_NS = 0x09,
	HWTS_NORMAL_IRQ_MAX = 0x0a,
};

#define IRQ_TYPE_MAX_EVENT_NUM 32

struct hwts_irq {
	uintptr_t reg_addr;
	uintptr_t event_reg_addr[IRQ_TYPE_MAX_EVENT_NUM];
	uint32_t irq_num;
	uint8_t irq_type;
};

int hwts_irq_init(void);

void hwts_irq_deinit(void);

#endif /* _HWTS_INTERRUPT_H */
