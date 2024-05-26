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

#include "hdmirx_video_hdr.h"
#include "hdmirx_common.h"
#include <linux/delay.h>

static hdmirx_hdr_ctx g_hdmirx_hdr_ctx;

static hdmirx_hdr_ctx *hdmirx_hdr_get_ctx(void)
{
	return &g_hdmirx_hdr_ctx;
}

static ext_drv_hdr_type hdmirx_hdr_get_hdr10_type(const hdmirx_pkt *pkt)
{
	if (pkt->pkt_received) {
		uint32_t tmp;
		tmp = (pkt->pkt_buffer[IF_HEADER_LENGTH] & 0x7);
		if (tmp == 0) {
			return EXT_DRV_HDR_TYPE_SDR;
		} else if ((tmp == 1) || (tmp == 2)) { /* 2: check HDR10 type */
			return EXT_DRV_HDR_TYPE_HDR10;
		} else if (tmp == 3) { /* 3: check HLG type */
			return EXT_DRV_HDR_TYPE_HLG;
		} else {
			return EXT_DRV_HDR_TYPE_MAX;
		}
	} else {
		return EXT_DRV_HDR_TYPE_MAX;
	}
}

void hdmirx_hdr_set_hdr_type(const hdmirx_pkt *pkt)
{
	hdmirx_hdr_ctx *hdr_ctx = NULL;
	uint32_t tmp;

	hdr_ctx = hdmirx_hdr_get_ctx();

	if (!pkt->pkt_received) {
		hdr_ctx->hdr_type = EXT_DRV_HDR_TYPE_MAX;
		return;
	}
	if (pkt->pkt_type == HDMIRX_PACKET_HDR) {
		hdr_ctx->hdr_type = hdmirx_hdr_get_hdr10_type(pkt);
		disp_pr_info("hdr_ctx->hdr_type 0x%x.\n", hdr_ctx->hdr_type);
	} else if (pkt->pkt_type == HDMIRX_PACKET_HFVSI_HDR) {
		tmp = pkt->pkt_buffer[IF_HEADER_LENGTH + 3]; /* 3: get dolby type from head 3st byte */
		tmp = (tmp >> 1) & 0x1;
		if (tmp == 1) {
			hdr_ctx->hdr_type = EXT_DRV_HDR_TYPE_DOLBYVISION;
		} else {
			hdr_ctx->hdr_type = EXT_DRV_HDR_TYPE_SDR;
		}
	}
}

bool hdmirx_hdr_type_is_hdr10(void)
{
	hdmirx_hdr_ctx *hdr_ctx = NULL;

	hdr_ctx = hdmirx_hdr_get_ctx();

	disp_pr_info("hdr_ctx->hdr_type 0x%x.\n", hdr_ctx->hdr_type);

	return (hdr_ctx->hdr_type == EXT_DRV_HDR_TYPE_HDR10);
}