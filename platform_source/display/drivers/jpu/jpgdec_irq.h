/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2021. All rights reserved.
 *
 * jpgdec irq
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef JPGDEC_IRQ_H
#define JPGDEC_IRQ_H

#include <linux/of.h>
#include "jpu.h"

int jpgdec_get_irq(struct jpu_data_type *jpu_device, struct device_node *np);
int jpgdec_request_irq(struct jpu_data_type *jpu_device);
int jpgdec_enable_irq(struct jpu_data_type *jpu_device);
void jpgdec_disable_irq(struct jpu_data_type *jpu_device);
void jpgdec_free_irq(struct jpu_data_type *jpu_device);

#endif /* JPGDEC_IRQ_H */
