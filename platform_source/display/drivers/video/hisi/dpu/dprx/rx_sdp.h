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

#ifndef __RX_SDP_H__
#define __RX_SDP_H__
#include <linux/types.h>

#include "drm_dsc.h"

#define SDP_COMMON_HEADER_SIZE 4
#define SDP_COMMON_PAYLOAD_SIZE 32
#define PPS_SIZE 128

#define SDP_HB_FIFO_SIZE 32 /* unit: 1 HB(48bit), HB fifo size: 32*48bit */
#define SDP_DB_FIFO_SIZE 512 /* uint: byte, DB fifo size: 32*128bit */
#define SDP_ONE_DB_SIZE 16 /* unit: byte, in DB fifo, 1 pcs is 16 byte */

extern struct drm_dsc_config *dsc_cfg;

enum sdp_type {
	DISPLAYPORT_RESERVED1_SDP = 0,
	AUDIO_TIMESTAMP_SDP = 1,
	AUDIO_STREAM_SDP = 2,
	DISPLAYPORT_RESERVED2_SDP = 3,
	EXTENSION_SDP = 4,
	AUDIO_COPY_MANAGEMENT_SDP = 5,
	ISRC_SDP = 6,
	VSC_SDP = 7,
	/* CAMERA_GENERIC_SDP 0-7: 0x8-0xF, */
	PPS_SDP = 0x10,
	/* DISPLAYPORT_RESERVED3_SDP: 0x11-0x1F */
	VSC_EXT_VESA_SDP = 0x20,
	VSC_EXT_CTA_SDP = 0x21,
	ADAPTIVE_SYNC_SDP = 0x22,
	/* DISPLAYPORT_RESERVED4_SDP: 0x23-0x7F */
	VENDOR_SPECIFIC_INFOFRAME_SDP = 0x81,
	AVI_INFOFRAME_SDP = 0x82,
	SPD_INFOFRAME_SDP = 0x83, /* SOURCE_PRODUCT_DESCRIPTION */
	AUDIO_INFOFRAME_SDP = 0x84,
	MPEG_SOURCE_INFOFRAME_SDP = 0x85,
	NTSC_VBI_INFOFRAME_SDP = 0x86,
	DYNAMIC_RANGE_AND_MASTERING_INFOFRAME_SDP = 0x87,
};

typedef union
{
	unsigned int value;
	struct
	{
		unsigned int hb0: 8;
		unsigned int hb1: 8;
		unsigned int hb2: 8;
		unsigned int hb3: 8;
	} headers;
} dprx_sdp_header;

struct sdp_hb_fifo_data {
	uint32_t data_len; /* unit: 16byte */
	dprx_sdp_header header;
};

struct rx_sdp_raw_data {
	uint32_t hb_data_len;
	uint32_t db_data_len;
	struct sdp_hb_fifo_data hb_fifo_data[SDP_HB_FIFO_SIZE];
	uint8_t db_fifo_data[SDP_DB_FIFO_SIZE];
};

/** Video_Stream_Configuration SDP
 *
 * software just
 */
struct vsc_sdp {
	uint8_t header[SDP_COMMON_HEADER_SIZE];
	uint8_t payload[SDP_COMMON_PAYLOAD_SIZE];
};

/**
 * 0: Traditional gamma - SDR Luminance Range: the maximum encoded luminance is typically mastered to 100 cd/m2
 * 1: Traditional gamma - HDR Luminance Range: the maximum encoded luminance is understood to be the maximum luminance
 * of the Sink device
 * 2: SMPTE ST 2084
 * 3: Hybrid Log-Gamma (HLG) based on ITU-R BT.2100-0
 * 4-7: Reserved
 */
typedef enum {
	DRM_TRADITIONAL_GAMMA_SDR = 0,
	DRM_TRADITIONAL_GAMMA_HDR,
	DRM_SMPTE_ST_2084,
	DRM_HLG,
} drm_payload_eotf;

/**
 * 0: Static Metadata Type 1
 * 1-7: Reserved
 */
typedef enum {
	DRM_STATIC_METADATA_TYPE1 = 0,
} drm_static_metadata_desc_id;

/**
 * Dynamic Range and Mastering InfoFrame: Can be used for HDR static metadata
 */
struct drm_infoframe_sdp {
	drm_payload_eotf eotf;
	drm_static_metadata_desc_id descriptor_id;

	struct {
		uint16_t x,y;
	} disp_primaries[3]; /* RGB */

	struct {
		uint16_t x,y;
	} white_point;

	uint16_t max_disp_mastering_luminance;
	uint16_t min_disp_mastering_luminance;
	uint16_t max_content_light_level;
	uint16_t max_frame_average_light_level;
};

#define DSC_RC_RANGE_LENGTH 15
#define DSC_THRESH_LENGTH 14

/**
 * DSC PPS Table
 */
struct pps_sdp {
	uint8_t pps[PPS_SIZE];
};

void rx_sdp_init(struct pps_sdp *pps_table);
void dprx_sdp_wq_handler(struct work_struct *work);
#endif