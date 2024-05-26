/*
 * irq_bottom_half.c
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
#include "irq_bottom_half.h"
#include "npu_log.h"

struct irq_bottom_half g_irq_bh;

int irq_bh_init(void)
{
	int i;

	g_irq_bh.wq = create_workqueue("npu_irq_bottom_half");
	cond_return_error(g_irq_bh.wq == NULL, -1, "create_workqueue error");

	g_irq_bh.work_index = 0;
	for (i = 0; i < MAX_WORK_NUM; i++)
		INIT_WORK(&g_irq_bh.work_list[i].work, irq_bh_run);

	for (i = 0; i < MAX_IRQ_NUM; i++)
		g_irq_bh.handler[i] = NULL;

	return 0;
}

void irq_bh_deinit(void)
{
	destroy_workqueue(g_irq_bh.wq);
}

int irq_bh_register_handler(uint8_t irq_type, uint8_t event_type, irq_bh_handler_t handler)
{
	uint16_t handler_index;

	cond_return_error(irq_type >= HWTS_IRQ_TYPE_MAX, -1, "irq_type %u error", irq_type);
	cond_return_error(event_type >= IRQ_TYPE_MAX_EVENT_NUM, -1, "event_type %u error", event_type);
	cond_return_error(handler == NULL, -1, "handler is nullptr");

	handler_index = irq_handler_index(irq_type, event_type);
	npu_drv_info("irq_type = %u, event_type = %u, handler_index = %u", irq_type, event_type, handler_index);
	g_irq_bh.handler[handler_index] = handler;

	return 0;
}

int irq_bh_unregister_handler(uint8_t irq_type, uint8_t event_type)
{
	uint16_t handler_index;

	cond_return_error(irq_type >= HWTS_IRQ_TYPE_MAX, -1, "irq_type %u error", irq_type);
	cond_return_error(event_type >= IRQ_TYPE_MAX_EVENT_NUM, -1, "event_type %u error", event_type);

	handler_index = irq_handler_index(irq_type, event_type);
	g_irq_bh.handler[handler_index] = NULL;

	return 0;
}

// run in irq top half
void irq_bh_trigger(uint8_t irq_type, uint8_t event_type, uint64_t channel_bitmap)
{
	g_irq_bh.work_list[g_irq_bh.work_index].irq_type = irq_type;
	g_irq_bh.work_list[g_irq_bh.work_index].event_type = event_type;
	g_irq_bh.work_list[g_irq_bh.work_index].channel_bitmap = channel_bitmap;

	queue_work(g_irq_bh.wq, &g_irq_bh.work_list[g_irq_bh.work_index].work);
	g_irq_bh.work_index = (g_irq_bh.work_index + 1) % MAX_WORK_NUM;
}

// run in irq bottom half
void irq_bh_run(struct work_struct *work)
{
	uint16_t handler_index;
	struct work_holder *cur_work_holder = NULL;

	cur_work_holder = container_of(work, struct work_holder, work);

	cond_return_void(cur_work_holder->irq_type >= HWTS_IRQ_TYPE_MAX,
		"irq_type %u error", cur_work_holder->irq_type);
	cond_return_void(cur_work_holder->event_type >= IRQ_TYPE_MAX_EVENT_NUM,
		"event_type %u error", cur_work_holder->event_type);

	handler_index = irq_handler_index(cur_work_holder->irq_type, cur_work_holder->event_type);
	if (g_irq_bh.handler[handler_index] != NULL) {
		npu_drv_info("running irq %u event %u handler", cur_work_holder->irq_type, cur_work_holder->event_type);
		g_irq_bh.handler[handler_index](cur_work_holder->event_type, cur_work_holder->channel_bitmap);
	}
}
