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

#ifndef HDMIRX_VIDEO_HDR_H
#define HDMIRX_VIDEO_HDR_H

#include "hdmirx_struct.h"
#include "packet/hdmirx_packet.h"

typedef enum {
	EXT_DRV_HDR_TYPE_SDR = 0,
	EXT_DRV_HDR_TYPE_HDR10,
	EXT_DRV_HDR_TYPE_HLG,
	EXT_DRV_HDR_TYPE_CUVA,

	EXT_DRV_HDR_TYPE_JTP_SL_HDR,
	EXT_DRV_HDR_TYPE_HDR10PLUS,
	EXT_DRV_HDR_TYPE_DOLBYVISION,

	EXT_DRV_HDR_TYPE_MAX
} ext_drv_hdr_type;

typedef struct {
	ext_drv_hdr_type               hdr_type;
} hdmirx_hdr_ctx;

void hdmirx_hdr_set_hdr_type(const hdmirx_pkt *pkt);
bool hdmirx_hdr_type_is_hdr10(void);

#endif
