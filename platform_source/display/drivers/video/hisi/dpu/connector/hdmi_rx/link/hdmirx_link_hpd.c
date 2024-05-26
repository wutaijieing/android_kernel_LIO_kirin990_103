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

#include "hdmirx_link_hpd.h"
#include "hdmirx_common.h"

int hdmirx_link_hpd_set(struct hdmirx_ctrl_st *hdmirx, bool high)
{
	if (high) {
		disp_pr_info("[hdmirx]hpd high");
		set_reg(hdmirx->hdmirx_sysctrl_base + 0x014, 1, 1, 0); // should follow powr 5v

		set_reg(hdmirx->hdmirx_aon_base + 0x1090, 0, 1, 1);
		set_reg(hdmirx->hdmirx_aon_base + 0x1090, 1, 1, 0);
	} else {
		disp_pr_info("[hdmirx]hpd low");
		set_reg(hdmirx->hdmirx_aon_base + 0x1090, 0, 1, 1);
		set_reg(hdmirx->hdmirx_aon_base + 0x1090, 0, 1, 0);

		set_reg(hdmirx->hdmirx_sysctrl_base + 0x014, 0, 1, 0); // should follow powr 5v
	}

	return 0;
}