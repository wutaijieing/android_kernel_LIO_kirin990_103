/* Copyright (c) 2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef HDMIRX_CTRL_IRQ_H
#define HDMIRX_CTRL_IRQ_H

#include "hdmirx_struct.h"
#include <linux/interrupt.h>

enum {
	HDMIRX_IRQ_RAM_PACKET = 0,
	HDMIRX_IRQ_VIDEO,
	HDMIRX_IRQ_MAX
};

int hdmirx_ctrl_irq_setup(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_ctrl_irq_enable(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_ctrl_irq_disable(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_ctrl_irq_config_init(struct device_node *np, struct hdmirx_ctrl_st *hdmirx);

#endif