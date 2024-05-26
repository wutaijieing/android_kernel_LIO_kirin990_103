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

#ifndef HDMIRX_CTRL_H
#define HDMIRX_CTRL_H

#include "hdmirx_struct.h"

int hdmirx_ctrl_init(struct hdmirx_ctrl_st *hdmirx);
void hdmirx_ctrl_source_select_hdmi(struct hdmirx_ctrl_st *hdmirx);
void hdmirx_ctrl_set_sys_mute(struct hdmirx_ctrl_st *hdmirx, bool enable);
bool hdmirx_ctrl_get_sys_mute(struct hdmirx_ctrl_st *hdmirx);
void hdmirx_ctrl_mute_irq_mask(struct hdmirx_ctrl_st *hdmirx);
void hdmirx_ctrl_mute_proc(struct hdmirx_ctrl_st *hdmirx);
bool hdmirx_ctrl_is_hdr10(void);
uint32_t hdmirx_ctrl_get_layer_fmt(struct hdmirx_ctrl_st *hdmirx);
void hdmirx_ctrl_yuv422_set(struct hdmirx_ctrl_st *hdmirx, uint32_t enable);

#endif