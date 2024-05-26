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

#ifndef HDMIRX_INIT_H
#define HDMIRX_INIT_H

#include "hdmirx_struct.h"

enum {
	HDMIRX_AON_BASE_ID = 0,
	HDMIRX_PWD_BASE_ID,
	HDMIRX_SYSCTRL_BASE_ID,
	HDMIRX_HSDT1_CRG_BASE_ID,
	HDMIRX_HSDT1_SYSCTRL_BASE_ID,
	HDMIRX_IOC_BASE_ID,
	HDMIRX_HI_GPIO14_BASE_ID,
	HDMIRX_BASE_ADDR_MAX,
};

void hdmirx_display_dss_ready(void);
void hdmirx_display_hdmi_ready(void);
uint32_t hdmirx_get_layer_fmt(void);

#endif