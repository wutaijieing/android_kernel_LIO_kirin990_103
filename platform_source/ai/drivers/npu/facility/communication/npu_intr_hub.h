/*
 * npu_intr_hub.h
 *
 * about npu intr_hub for interrupt
 *
 * Copyright (c) 2012-2019 Huawei Technologies Co., Ltd.
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
#ifndef __NPU_INTR_HUB_H
#define __NPU_INTR_HUB_H
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/list.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/sched.h>

#include "npu_common.h"
#include "npu_proc_ctx.h"
#include "npu_soc_defines.h"

struct npu_intr_hub_irq_desc {
	unsigned int irq;
	irq_handler_t handler;
	void *dev;
	unsigned int flags;
	const char *name;
	struct npu_intr_hub *hub;
};

struct npu_intr_hub {
	void __iomem *base_vaddr;
	unsigned long reg_bitmap;
	struct npu_intr_hub_irq_desc *proc_map;
	unsigned int map_size;
	unsigned int registed_count;
	unsigned int rcv_count; // just for count
};

int npu_intr_hub_irq_init(struct npu_platform_info *platform_info);

int npu_intr_hub_request_irq(unsigned int irq, irq_handler_t handler,
	const char *name, void *dev, struct npu_intr_hub *hub);

irqreturn_t npu_intr_hub_handle_bad_irq(int irq, void *data);

irqreturn_t npu_intr_hub_irq_handler(int irq, void *data);

void npu_intr_hub_irq_scheduler(struct npu_intr_hub *intr_hub);

#endif
