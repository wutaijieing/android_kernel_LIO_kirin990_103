/*
 * hwts_interrupt_handler.h
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

#ifndef _HWTS_INTERRUPT_HANDLER_H
#define _HWTS_INTERRUPT_HANDLER_H

#include <linux/interrupt.h>
#include "hwts_interrupt.h"

irqreturn_t hwts_irq_handler(int irq, void *data);
void hwts_irq_get_ctx(uint64_t hwts_base, uint32_t hwts_normal_irq_num, struct hwts_irq *hwts_normal_irq_ctx);
void hwts_error_irq_get_ctx(uint64_t hwts_base, uint32_t hwts_error_irq_num, struct hwts_irq *hwts_error_irq_ctx);
#endif