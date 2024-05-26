/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/delay.h>

#include "dkmd_log.h"
#include "hdmitx_ctrl_reg.h"
#include "hdmitx_video_path_reg.h"
#include "hdmitx_reg.h"
#include "hdmitx_aon_reg.h"
#include "hdmitx_ctrl.h"
#include "hdmitx_infoframe.h"
#include "hdmitx_ddc_config.h"

#define INFOFRAME_PKT_SIZE 31
#define REG_TP_MEMCTRL 0x3C
#define REG_SP_MEMCTRL 0x40
#define REG_AUD_ACR_CTRL 0x1040
#define REG_ACR_CTS_INIT_MASK (0xf << 0)

struct infoframe_offset {
	u32 head;
	u32 pkt_l;
	u32 pkt_h;
	u32 cfg;
};

static void core_get_infoframe(const u8 *buffer, struct infoframe_offset *offset)
{
	switch (*buffer) {
	case HDMITX_INFOFRAME_TYPE_AVI:
		offset->head = REG_AVI_PKT_HEADER;
		offset->pkt_l = REG_AVI_SUB_PKT0_L;
		offset->pkt_h = REG_AVI_SUB_PKT0_H;
		offset->cfg = REG_CEA_AVI_CFG;
		break;

	case HDMITX_INFOFRAME_TYPE_AUDIO:
		offset->head = REG_AIF_PKT_HEADER;
		offset->pkt_l = REG_AIF_SUB_PKT0_L;
		offset->pkt_h = REG_AIF_SUB_PKT0_H;
		offset->cfg = REG_CEA_AUD_CFG;
		break;

	case HDMITX_INFOFRAME_TYPE_VENDOR:
		offset->head = REG_VSIF_PKT_HEADER;
		offset->pkt_l = REG_VSIF_SUB_PKT0_L;
		offset->pkt_h = REG_VSIF_SUB_PKT0_H;
		offset->cfg = REG_CEA_VSIF_CFG;
		break;
	default:
		dpu_pr_err("do not support infoframe type");
	}
}

static void core_hw_write_info_frame(void *hdmi_reg_base, const u8 *buffer,
	u8 len, const struct infoframe_offset *offset)
{
	const u8 *ptr = buffer;
	u32 i;
	u32 data;

	/* Write PKT header */
	data = reg_sub_pkt_hb0(*ptr++);
	data |= reg_sub_pkt_hb1(*ptr++);
	data |= reg_sub_pkt_hb2(*ptr++);
	hdmi_writel(hdmi_reg_base, offset->head, data);

	/* Write PKT body */
	for (i = 0; i < MAX_SUB_PKT_NUM; i++) {
		data = reg_sub_pktx_pb0(*ptr++);
		data |= reg_sub_pktx_pb1(*ptr++);
		data |= reg_sub_pktx_pb2(*ptr++);
		data |= reg_sub_pktx_pb3(*ptr++);
		/* the address offset of two subpackages is 8bytes */
		hdmi_writel(hdmi_reg_base, offset->pkt_l + i * 8, data);
		data = reg_sub_pktx_pb4(*ptr++);
		data |= reg_sub_pktx_pb5(*ptr++);
		data |= reg_sub_pktx_pb6(*ptr++);
		/* the address offset of two subpackages is 8bytes */
		hdmi_writel(hdmi_reg_base, offset->pkt_h + i * 8, data);
	}
}

void core_hw_send_info_frame(void *hdmi_reg_base, const u8 *buffer, u8 len)
{
	const u8 *ptr = buffer;
	struct infoframe_offset offset = {0};

	if (buffer == NULL || hdmi_reg_base == NULL) {
		dpu_pr_err("ptr null err.\n");
		return;
	}
	if (len < INFOFRAME_PKT_SIZE) {
		dpu_pr_err("buffer size less than send buffer size \n");
		return;
	}

	core_get_infoframe(ptr, &offset);
	core_hw_write_info_frame(hdmi_reg_base, buffer, len, &offset);

	/* Enable PKT send */
	hdmi_clrset(hdmi_reg_base, offset.cfg,
		REG_EN_M | REG_RPT_EN_M, reg_en(1) | reg_rpt_en(1));
}


void core_set_color_format(void *hdmi_reg_base, int in_color_format)
{
	bool is_yuv_format = false;

	if (in_color_format != RGB444) {
		dpu_pr_info("color format is not RGB!");
		is_yuv_format = true;
	}
	hdmi_clrset(hdmi_reg_base, REG_CSC_DITHER_DFIR_EN,
		REG_VDP2HDMI_YUV_MODE_M, reg_vdp2hdmi_yuv_mode((u8)is_yuv_format));
}

void core_set_color_depth(void *hdmi_reg_base, int in_color_depth)
{
	bool deep_color_en = false;
	u8 pack_mode = 0;

	switch (in_color_depth) {
	case HDMITX_BPC_24:
		break;
	case HDMITX_BPC_30:
		pack_mode = 1;
		deep_color_en = true;
		break;
	case HDMITX_BPC_36:
		pack_mode = 2; /* pack mode need set to 2. */
		deep_color_en = true;
		break;
	default:
		dpu_pr_err("Not support this output color depth: %d!\n", in_color_depth);
	}

	/* set tmds pack mode */
	hdmi_clrset(hdmi_reg_base, REG_TX_PACK_FIFO_CTRL,
		REG_TMDS_PACK_MODE_M, reg_pack_mode(pack_mode));

	/* Enable/disable PKT send, and enable/disable avmute in phase */
	hdmi_clrset(hdmi_reg_base, REG_AVMIXER_CONFIG,
		REG_DC_PKT_EN_M | REG_AVMUTE_IN_PHASE_M,
		reg_dc_pkt_en((u8)deep_color_en) | reg_avmute_in_phase((u8)deep_color_en));
}

void core_set_hdmi_mode(void *hdmi_reg_base, bool is_hdmi_mode)
{
	hdmi_clrset(hdmi_reg_base, REG_AVMIXER_CONFIG,
		REG_HDMI_MODE_M, reg_hdmi_mode((u8)is_hdmi_mode));
}

void core_set_vdp_packet(void *hdmi_reg_base, bool enabe)
{
	/* Reset vdp packet path */
	hdmi_clr(hdmi_reg_base, REG_TX_METADATA_CTRL_ARST_REQ,
		REG_TX_METADATA_ARST_REQ_M);

	/* enable vdp packet path */
	hdmi_clrset(hdmi_reg_base, REG_TX_METADATA_CTRL, REG_TXMETA_VDP_PATH_EN_M,
		reg_txmeta_vdp_path_en((u8)enabe));

	/* enable vdp avi path */
	hdmi_clrset(hdmi_reg_base, REG_TX_METADATA_CTRL,
		REG_TXMETA_VDP_AVI_EN_M, reg_txmeta_vdp_avi_en((u8)enabe));
}

void core_send_av_mute(void *hdmi_reg_base)
{
	dpu_check_and_no_retval(!hdmi_reg_base, err, "null ptr\n");

	/* set av mute */
	hdmi_clrset(hdmi_reg_base, REG_CP_PKT_AVMUTE,
		REG_CP_CLR_AVMUTE_M | REG_CP_SET_AVMUTE_M,
		reg_cp_clr_avmute(0) | reg_cp_set_avmute(1));

	/* Enable PKT send. set mute send 17 times, clr mute always send */
	hdmi_clrset(hdmi_reg_base, REG_CEA_CP_CFG,
		REG_EN_M | REG_RPT_EN_M | REG_RPT_CNT_M,
		reg_en(1) | reg_rpt_en(0) | reg_rpt_cnt(17));
}

void core_send_av_unmute(void *hdmi_reg_base)
{
	dpu_check_and_no_retval(!hdmi_reg_base, err, "null ptr\n");

	/* set av unmute */
	hdmi_clrset(hdmi_reg_base, REG_CP_PKT_AVMUTE,
		REG_CP_CLR_AVMUTE_M | REG_CP_SET_AVMUTE_M,
		reg_cp_clr_avmute(1) | reg_cp_set_avmute(0));

	/* Enable PKT send */
	hdmi_clrset(hdmi_reg_base, REG_CEA_CP_CFG,
		REG_EN_M | REG_RPT_EN_M,
		reg_en(1) | reg_rpt_en(1));
}

void core_enable_memory(void *hdmi_reg_base)
{
	hdmi_writel(hdmi_reg_base, REG_TP_MEMCTRL, 0x50);
	hdmi_writel(hdmi_reg_base, REG_SP_MEMCTRL, 0x5858);
}

void core_disable_memory(void *hdmi_reg_base)
{
	hdmi_writel(hdmi_reg_base, REG_TP_MEMCTRL, 0x54);
	hdmi_writel(hdmi_reg_base, REG_SP_MEMCTRL, 0x585c);
}

void core_reset_pwd(void *hdmi_reg_base, bool enable)
{
	hdmi_clrset(hdmi_reg_base, REG_TX_PWD_RST_CTRL, REG_TX_PWD_SRST_REQ_M, reg_tx_pwd_srst_req((u8)enable));
}

void core_set_audio_acr(void *hdmi_reg_base)
{
	hdmi_clrset(hdmi_reg_base, REG_AUD_ACR_CTRL, REG_ACR_CTS_INIT_MASK, 0x4);
}
