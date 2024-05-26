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

#include "hdmirx_hdcp.h"
#include "hdmirx_common.h"

int hdmirx_hdcp_14_key_load(struct hdmirx_ctrl_st *hdmirx)
{
	return 0;
}

int hdmirx_hdcp_2x_key_load(struct hdmirx_ctrl_st *hdmirx)
{
	return 0;
}

int hdmirx_hdcp_key_init(struct hdmirx_ctrl_st *hdmirx)
{
	hdmirx_hdcp_14_key_load(hdmirx);
	hdmirx_hdcp_2x_key_load(hdmirx);

	return 0;
}

int hdmirx_hdcp_random_number_init(struct hdmirx_ctrl_st *hdmirx)
{
	return 0;
}

int hdmirx_hdcp_mcu_code_load(struct hdmirx_ctrl_st *hdmirx)
{
	return 0;
}

int hdmirx_hdcp_init(struct hdmirx_ctrl_st *hdmirx)
{
	hdmirx_hdcp_key_init(hdmirx);

	hdmirx_hdcp_random_number_init(hdmirx);

	hdmirx_hdcp_mcu_code_load(hdmirx);

	return 0;
}

int hdmirx_start_hdcp(struct hdmirx_ctrl_st *hdmirx)
{
	// start HDCP 1.4
	set_reg(hdmirx->hdmirx_pwd_base + 0x1A038, 0x1, 32, 0);
	set_reg(hdmirx->hdmirx_pwd_base + 0x1A02C, 0x6, 32, 0);

	set_reg(hdmirx->hdmirx_pwd_base + 0x1A108, 0xf02, 32, 0);
	set_reg(hdmirx->hdmirx_pwd_base + 0x1A114, 0x12160210, 32, 0);
	set_reg(hdmirx->hdmirx_pwd_base + 0x1A024, 0x91, 32, 0);

	// start MCU HDCP 2.2
	set_reg(hdmirx->hdmirx_pwd_base + 0x18008, 0x7, 32, 0);
	set_reg(hdmirx->hdmirx_pwd_base + 0x1A02C, 0x6, 32, 0);

	set_reg(hdmirx->hdmirx_pwd_base + 0x00020, 0, 1, 31);
	set_reg(hdmirx->hdmirx_pwd_base + 0x04004, 0x4c84200, 32, 0);
	return 0;
}