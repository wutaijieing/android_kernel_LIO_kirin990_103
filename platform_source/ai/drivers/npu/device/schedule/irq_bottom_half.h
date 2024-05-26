/*
 * irq_bottom_half.h
 *
 * about irq bottom half
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
#ifndef _IRQ_BOTTOM_HALF_H
#define _IRQ_BOTTOM_HALF_H

#include <linux/types.h>
#include <linux/workqueue.h>

#include "hwts_interrupt.h"

#define irq_handler_index(irq_type, event_type) (((irq_type) * IRQ_TYPE_MAX_EVENT_NUM) + (event_type))

#define MAX_WORK_NUM 128
#define MAX_IRQ_NUM irq_handler_index(HWTS_IRQ_TYPE_MAX, 0)

struct work_holder {
	struct work_struct work;
	uint64_t channel_bitmap;
	uint8_t irq_type;
	uint8_t event_type;
};

typedef void (*irq_bh_handler_t)(uint8_t event_type, uint64_t channel_bitmap);

struct irq_bottom_half {
	struct workqueue_struct *wq;
	struct work_holder work_list[MAX_WORK_NUM];
	irq_bh_handler_t handler[MAX_IRQ_NUM];
	unsigned int work_index;
};

int irq_bh_init(void);

void irq_bh_deinit(void);

int irq_bh_register_handler(uint8_t irq_type, uint8_t event_type, irq_bh_handler_t handler);

int irq_bh_unregister_handler(uint8_t irq_type, uint8_t event_type);

void irq_bh_trigger(uint8_t irq_type, uint8_t event_type, uint64_t channel_bitmap);

void irq_bh_run(struct work_struct *work);

#endif /* _IRQ_BOTTOM_HALF_H */
