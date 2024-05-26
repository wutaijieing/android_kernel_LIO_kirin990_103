/* Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#ifndef HIDPTX_DP_IRQ_H
#define HIDPTX_DP_IRQ_H

#include <linux/kernel.h>
#include <linux/interrupt.h>

struct dp_ctrl;

irqreturn_t dptx_irq(int irq, void *dev);
irqreturn_t dptx_threaded_irq(int irq, void *dev);
void dptx_hpd_handler(struct dp_ctrl *dptx, bool plugin, uint8_t dp_lanes);
void dptx_hpd_irq_handler(struct dp_ctrl *dptx);

int handle_test_link_phy_pattern(struct dp_ctrl *dptx);

#endif /* HIDPTX_DP_IRQ_H */
