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

#ifndef __RX_IRQ_H__
#define __RX_IRQ_H__

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include "dprx.h"

#define DPRX_COLOR_DEPTH_INDEX_MAX 7
#define DPRX_PIXEL_ENC_INDEX_MAX 6
#define ATC_STATUS_TRAIN_DONE 5
#define DPRX_4LANE_ENTER_P3 0x3333
irqreturn_t dprx_irq_handler(int irq, void *dev);
irqreturn_t dprx_threaded_irq_handler(int irq, void *dev);
uint32_t dprx_ctrl_dp2layer_fmt(uint32_t color_space, uint32_t bit_width);
uint32_t  dprx_get_color_space(struct dprx_ctrl *dprx);
#endif
