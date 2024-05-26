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

#ifndef HDMIRX_HDCP_H
#define HDMIRX_HDCP_H

#include "hdmirx_struct.h"

int hdmirx_hdcp_init(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_start_hdcp(struct hdmirx_ctrl_st *hdmirx);

#endif