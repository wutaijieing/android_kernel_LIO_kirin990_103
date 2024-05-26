/*
 * hwts_interrupt_handler.c
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
#include "hwts_interrupt_handler.h"

#include <linux/bitmap.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include "irq_bottom_half.h"
#include "npu_log.h"
#include "soc_npu_hwts_interface.h"

// run in irq top half
irqreturn_t hwts_irq_handler(int irq, void *data)
{
	uint64_t reg_val;
	unsigned int event_type;
	uint64_t channel_bitmap;
	struct hwts_irq *hwts_irq_ctx = (struct hwts_irq *)data;
	for (reg_val = readq(hwts_irq_ctx->reg_addr); reg_val != 0; bitmap_clear(&reg_val, event_type, 1)) {
		event_type = find_first_bit(&reg_val, IRQ_TYPE_MAX_EVENT_NUM);
		if (hwts_irq_ctx->event_reg_addr[event_type] != 0) {
			channel_bitmap = readq(hwts_irq_ctx->event_reg_addr[event_type]);
			writeq(channel_bitmap, hwts_irq_ctx->event_reg_addr[event_type]);
			irq_bh_trigger(hwts_irq_ctx->irq_type, event_type, channel_bitmap);
		}
	}

	return IRQ_HANDLED;
}

void hwts_irq_get_ctx(uint64_t hwts_base, uint32_t hwts_irq_num, struct hwts_irq *hwts_irq_ctx)
{
	uint32_t idx;

	hwts_irq_ctx->irq_type = HWTS_IRQ_NORMAL;
	hwts_irq_ctx->irq_num = hwts_irq_num;
	hwts_irq_ctx->reg_addr = SOC_NPU_HWTS_HWTS_L1_NORMAL_NS_INTERRUPT_ADDR(hwts_base);

	for (idx = 0; idx < IRQ_TYPE_MAX_EVENT_NUM; idx++)
		hwts_irq_ctx->event_reg_addr[idx] = 0;

	hwts_irq_ctx->event_reg_addr[HWTS_SQ_DONE_NS] =
		SOC_NPU_HWTS_HWTS_L2_NORMAL_SQ_DONE_NS_INTERRUPT_ADDR(hwts_base, 0);

	hwts_irq_ctx->event_reg_addr[HWTS_CQE_WRITTEN_NS] =
		SOC_NPU_HWTS_HWTS_L2_NORMAL_CQE_WRITTEN_NS_INTERRUPT_ADDR(hwts_base, 0);

	return;
}