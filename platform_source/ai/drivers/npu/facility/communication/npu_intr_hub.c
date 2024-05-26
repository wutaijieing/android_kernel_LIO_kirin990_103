/*
 * npu_intr_hub.c
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
#include "npu_intr_hub.h"

#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/list.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/interrupt.h>

#include "npu_common.h"
#include "npu_log.h"
#include "npu_platform.h"
#include "npu_shm.h"
#include "npu_adapter.h"
#include "npu_ddr_map.h"

static struct npu_intr_hub_irq_desc g_npu_intr_hub_proc_map[NPU_INTR_HUB_L2_0_COUNT] __cacheline_aligned_in_smp = {
	[0 ... NPU_INTR_HUB_L2_0_COUNT-1] = {
		.handler= npu_intr_hub_handle_bad_irq,
		.name = "Not Registed",
	}
};

struct npu_intr_hub_irq_desc *npu_intr_hub_irq_to_desc(unsigned int irq, struct npu_intr_hub *hub)
{
	cond_return_error(hub == NULL, NULL, "hub is NULL\n");
	return (irq < hub->map_size) ? hub->proc_map + irq : NULL;
}

int npu_intr_hub_request_irq(unsigned int irq, irq_handler_t handler,
	const char *name, void *dev, struct npu_intr_hub *hub)
{
	struct npu_intr_hub_irq_desc *desc = NULL;
	if((hub == NULL) || (irq >= hub->map_size)) {
		npu_drv_err("regist error, irq: %u is larger than the COUNT",irq);
		return -1;
	}
	desc = npu_intr_hub_irq_to_desc(irq, hub);
	cond_return_error(desc == NULL, -1, "desc is NULL\n");
	desc->irq = irq;
	desc->handler = handler;
	desc->name = name;
	desc->dev = dev;
	desc->hub = hub;
	hub->registed_count++;
	npu_drv_warn("regist irq: %u name:%s \n", irq, desc->name);
	return 0;
}

void npu_intr_hub_irq_scheduler(struct npu_intr_hub *intr_hub)
{
	unsigned long irq;
	struct npu_intr_hub_irq_desc *desc = NULL;

	while (intr_hub->reg_bitmap) {
		irq = find_first_bit(&intr_hub->reg_bitmap, intr_hub->map_size);
		bitmap_clear(&intr_hub->reg_bitmap, irq, 1);
		desc = npu_intr_hub_irq_to_desc(irq, intr_hub);
		cond_return_void(desc == NULL, "desc is NULL\n");
		desc->handler(irq, desc->dev);
	}
}

irqreturn_t npu_intr_hub_handle_bad_irq(int irq, void *data)
{
	npu_drv_err("irq: %d not registed\n", irq);
	return IRQ_NONE;
}

irqreturn_t npu_intr_hub_irq_handler(int irq, void *data)
{
	/* top half process */
	struct npu_intr_hub *intr_hub = (struct npu_intr_hub *)data;

	intr_hub->rcv_count++;
	intr_hub->reg_bitmap = (unsigned long)readl(
		(const volatile void *)(uintptr_t)SOC_INTR_HUB_L2_INTR_STATUS_S_n_ADDR((u64)(uintptr_t)intr_hub->base_vaddr, 0));
	npu_intr_hub_irq_scheduler(intr_hub);

	return IRQ_HANDLED;
}

int npu_intr_hub_irq_init(struct npu_platform_info *plat_info)
{
	int ret;
	struct npu_intr_hub* intr_hub = &plat_info->intr_hub;
	npu_drv_warn("intr hub irq init, irq_intr_hub=%d\n", plat_info->dts_info.irq_intr_hub);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	intr_hub->base_vaddr = ioremap(SOC_ACPU_intr_hub_npu_BASE_ADDR, SIZE_4KB);
#else
	intr_hub->base_vaddr = ioremap_nocache(SOC_ACPU_intr_hub_npu_BASE_ADDR, SIZE_4KB);
#endif
	if (intr_hub->base_vaddr == NULL) {
		npu_drv_err("ioremap_nocache intr_hub_npu failed\n");
		return -1;
	}

	ret = request_irq(plat_info->dts_info.irq_intr_hub, npu_intr_hub_irq_handler,
		IRQF_TRIGGER_NONE, "npu_intr_hub_irq", intr_hub);
	if (ret != 0) {
		npu_drv_err("request intr_hub_irq_handler failed, ret:%d\n", ret);
		return -1;
	}
	intr_hub->reg_bitmap = 0;
	intr_hub->proc_map = g_npu_intr_hub_proc_map;
	intr_hub->map_size = NPU_INTR_HUB_L2_0_COUNT;
	intr_hub->rcv_count = 0;
	intr_hub->registed_count = 0;

	return 0;
}

