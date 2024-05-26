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

#include "hdmirx_phy.h"
#include <linux/delay.h>
#include "hdmirx_common.h"

int hdmirx_phy_fgpa_reset(struct hdmirx_ctrl_st *hdmirx)
{
	set_reg(hdmirx->hdmirx_sysctrl_base + 0x084, 0x3ff, 32, 0);
	mdelay(500);
	set_reg(hdmirx->hdmirx_sysctrl_base + 0x084, 0x3b8, 32, 0);

	disp_pr_info("[hdmirx]phy init 0x3b8.\n");

	return 0;
}

int hdmirx_phy_asic_reset(struct hdmirx_ctrl_st *hdmirx)
{
	return 0;
}

int hdmirx_phy_init(struct hdmirx_ctrl_st *hdmirx)
{
	if (1) {
		hdmirx_phy_fgpa_reset(hdmirx);
	} else {
		hdmirx_phy_asic_reset(hdmirx);
	}

	return 0;
}