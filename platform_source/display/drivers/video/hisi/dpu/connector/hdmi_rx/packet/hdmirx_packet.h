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

#ifndef HDMIRX_PACKET_H
#define HDMIRX_PACKET_H

#include "hdmirx_struct.h"
#include "hdmirx_common.h"

#define IF_BUFFER_LENGTH  31
#define IF_HEADER_LENGTH     4
#define IF_LENGTH_INDEX      2

#define IF_MIN_AVI_LENGTH 13 /* the 861C standard defines the length as 13 */
#define IF_MAX_AVI_LENGTH 15 /* maximum allowed by the chip */

typedef enum {
	HDMIRX_COLOR_METRY_NOINFO,
	HDMIRX_COLOR_METRY_ITU601,
	HDMIRX_COLOR_METRY_ITU709,
	HDMIRX_COLOR_METRY_EXTENDED, /* if extended, but unknown */
	HDMIRX_COLOR_METRY_XV601 = 10,
	HDMIRX_COLOR_METRY_XV709,
	HDMIRX_COLOR_METRY_SYCC601,
	HDMIRX_COLOR_METRY_ADOBE_YCC601,
	HDMIRX_COLOR_METRY_ADOBE_RGB,
	HDMIRX_COLOR_METRY_BT2020_YCCBCCRC,
	HDMIRX_COLOR_METRY_BT2020_RGB,
	HDMIRX_COLOR_METRY_BT2020_YCBCR,
	HDMIRX_COLOR_METRY_BUTT
} hdmirx_color_metry;


#define PACKET_BUFFER_LENGTH 35

#define REG_GCPRX_WORD0 0x1060
#define REG_ACPRX_WORD0 0x1080
#define REG_ISRC1RX_WORD0 0x10A0
#define REG_ISRC2RX_WORD0 0x10C0
#define REG_GMPRX_WORD0 0x10E0
#define REG_VSIRX_WORD0 0x1100
#define REG_HF_VSI_3DRX_WORD0 0x1120
#define REG_HF_VSI_HDRRX_WORD0 0x1140
#define REG_AVIRX_WORD0 0x1160
#define REG_SPDRX_WORD0 0x1180
#define REG_AIFRX_WORD0 0x11A0
#define REG_MPEGRX_WORD0 0x11C0
#define REG_HDRRX_WORD0 0x11E0
#define REG_AUD_METARX_WORD0 0x1200
#define REG_UNRECRX_WORD0 0x1220
#define REG_TRASH_CANRX_WORD0 0x1240
#define REG_FVAVRRRX_WORD0 0x1360

#define AVI_VERSION       2

typedef enum {
	HDMIRX_PACKET_GCP,
	HDMIRX_PACKET_ACP,
	HDMIRX_PACKET_ISRC1,
	HDMIRX_PACKET_ISRC2,
	HDMIRX_PACKET_GMP,
	HDMIRX_PACKET_VSI,
	HDMIRX_PACKET_HFVSI_3D,
	HDMIRX_PACKET_HFVSI_HDR,
	HDMIRX_PACKET_AVI,
	HDMIRX_PACKET_SPD,
	HDMIRX_PACKET_AIF,
	HDMIRX_PACKET_MPEG,
	HDMIRX_PACKET_HDR,
	HDMIRX_PACKET_AUD_META,
	HDMIRX_PACKET_UNREC,
	HDMIRX_PACKET_FVAVRR,
	HDMIRX_PACKET_MAX
} hdmirx_packet_type;

typedef enum {
	INTR_CEA_UPDATE_GCP        = 0x1,    /* mask bit 0 */
	INTR_CEA_UPDATE_ACP        = 0x2,    /* mask bit 1 */
	INTR_CEA_UPDATE_ISRC1      = 0x4,    /* mask bit 2 */
	INTR_CEA_UPDATE_ISRC2      = 0x8,    /* mask bit 3 */
	INTR_CEA_UPDATE_GMP        = 0x10,   /* mask bit 4 */
	INTR_CEA_UPDATE_AUD_META   = 0x20,   /* mask bit 5 */
	INTR_CEA_UPDATE_VSI        = 0x40,   /* mask bit 6 */
	INTR_CEA_UPDATE_AVI        = 0x80,   /* mask bit 7 */
	INTR_CEA_UPDATE_SPD        = 0x100,  /* mask bit 8 */
	INTR_CEA_UPDATE_AIF        = 0x200,  /* mask bit 9 */
	INTR_CEA_UPDATE_MPEG       = 0x400,  /* mask bit 10 */
	INTR_CEA_UPDATE_HDR        = 0x800,  /* mask bit 11 */
	INTR_CEA_UPDATE_UNREC      = 0x1000, /* mask bit 12 */
	INTR_CEA_UPDATE_TRASH      = 0x2000, /* mask bit 13 */
	INTR_CEA_UPDATE_HFVSI_3D   = 0x4000, /* mask bit 14 */
	INTR_CEA_UPDATE_HFVSI_HDR  = 0x8000, /* mask bit 15 */
	INTR_CEA_HFVSI_3D_RETRANS  = 0x10000, /* mask bit 16 */
	INTR_CEA_HFVSI_HDR_RETRANS = 0x20000, /* mask bit 17 */
	INTR_CEA_VSI_RETRANS       = 0x40000  /* mask bit 18 */
} hdmirx_packet_intr_mask;

typedef enum {
	ACP_GENERAL_AUDIO = 0,
	ACP_IEC60958,
	ACP_DVD_AUDIO,
	ACP_SUPER_AUDIO_CD,
	ACP_BUTT
} hdmirx_acp_type;

typedef struct {
	bool pkt_received; /* set on any SPD reception (even with incorrect check sum) */
	hdmirx_packet_type pkt_type;
	bool intr_type; /* true : update, false : new */
	uint8_t pkt_buffer[IF_BUFFER_LENGTH]; /* 31: array size */
	uint32_t pkt_len;
	uint32_t version;
} hdmirx_pkt;

typedef struct {
	hdmirx_pkt avi;
	hdmirx_pkt spd;
	hdmirx_pkt vsif_3d;
	hdmirx_pkt vsif_hdr10_plus;
	hdmirx_pkt vsif_dolby;
	hdmirx_pkt hdr;
	hdmirx_pkt vsi;
	hdmirx_pkt aif;
	hdmirx_pkt fvavrr;
	hdmirx_acp_type acp_type;
	uint32_t av_mask; /* RX AV muting sources */
	bool audio_info_frame_received;
} hdmirx_packet_context;

struct hdmirx_to_layer_format {
	hdmirx_color_space color_space;
	hdmirx_input_width bit_width; /* HDMIRX_INPUT_WIDTH_24 */
	uint32_t layer_format;
};

void hdmirx_packet_set_ram_irq_mask(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_packet_ram_depack(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_packet_reg_depack(struct hdmirx_ctrl_st *hdmirx);
void hdmirx_packet_depack_enable(struct hdmirx_ctrl_st *hdmirx);
uint32_t hdmirx_packet_get_color_depth(struct hdmirx_ctrl_st *hdmirx);
hdmirx_color_space hdmirx_packet_avi_get_color_space(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_color_depth_process(struct hdmirx_ctrl_st *hdmirx);
int hdmirx_video_gcp_mute(struct hdmirx_ctrl_st *hdmirx);

#endif