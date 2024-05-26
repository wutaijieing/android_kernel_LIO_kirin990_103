/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2013-2021. All rights reserved.
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

#ifndef __HDMITX_HDR_H__
#define __HDMITX_HDR_H__

#define INFOFRAME_PACKET_TYPE_HDR 0x07
#define MAX_INFOFRAME_LENGTH 27
#define INFOFRAME_REGISTER_SIZE 32
#define INFOFRAME_DATA_SIZE 8
#define DATA_NUM_PER_REG (INFOFRAME_REGISTER_SIZE / INFOFRAME_DATA_SIZE)
#define HDR_INFOFRAME_VERSION 0x01
#define HDR_INFOFRAME_LENGTH 26
#define HDR_INFOFRAME_EOTF_BYTE_NUM 0
#define HDR_INFOFRAME_SMPTE_ST_2084 2
#define STATIC_MATADATA_TYPE_1 0
#define HDR_INFOFRAME_METADATA_ID_BYTE_NUM 1
#define HDR_INFOFRAME_DISP_PRI_X_0_LSB 2
#define HDR_INFOFRAME_DISP_PRI_X_0_MSB 3
#define HDR_INFOFRAME_DISP_PRI_Y_0_LSB 4
#define HDR_INFOFRAME_DISP_PRI_Y_0_MSB 5
#define HDR_INFOFRAME_DISP_PRI_X_1_LSB 6
#define HDR_INFOFRAME_DISP_PRI_X_1_MSB 7
#define HDR_INFOFRAME_DISP_PRI_Y_1_LSB 8
#define HDR_INFOFRAME_DISP_PRI_Y_1_MSB 9
#define HDR_INFOFRAME_DISP_PRI_X_2_LSB 10
#define HDR_INFOFRAME_DISP_PRI_X_2_MSB 11
#define HDR_INFOFRAME_DISP_PRI_Y_2_LSB 12
#define HDR_INFOFRAME_DISP_PRI_Y_2_MSB 13
#define HDR_INFOFRAME_WHITE_POINT_X_LSB 14
#define HDR_INFOFRAME_WHITE_POINT_X_MSB 15
#define HDR_INFOFRAME_WHITE_POINT_Y_LSB 16
#define HDR_INFOFRAME_WHITE_POINT_Y_MSB 17
#define HDR_INFOFRAME_MAX_LUMI_LSB 18
#define HDR_INFOFRAME_MAX_LUMI_MSB 19
#define HDR_INFOFRAME_MIN_LUMI_LSB 20
#define HDR_INFOFRAME_MIN_LUMI_MSB 21
#define HDR_INFOFRAME_MAX_LIGHT_LEVEL_LSB 22
#define HDR_INFOFRAME_MAX_LIGHT_LEVEL_MSB 23
#define HDR_INFOFRAME_MAX_AVERAGE_LEVEL_LSB 24
#define HDR_INFOFRAME_MAX_AVERAGE_LEVEL_MSB 25


struct hdr_infoframe {
	uint8_t type_code;
	uint8_t version_number;
	uint8_t length;
	uint8_t data[HDR_INFOFRAME_LENGTH];
};

struct hdr_metadata {
	uint32_t electro_optical_transfer_function;
	uint32_t static_metadata_descriptor_id;
	uint32_t red_primary_x;
	uint32_t red_primary_y;
	uint32_t green_primary_x;
	uint32_t green_primary_y;
	uint32_t blue_primary_x;
	uint32_t blue_primary_y;
	uint32_t white_point_x;
	uint32_t white_point_y;
	uint32_t max_display_mastering_luminance;
	uint32_t min_display_mastering_luminance;
	uint32_t max_content_light_level;
	uint32_t max_frame_average_light_level;
};


#endif
