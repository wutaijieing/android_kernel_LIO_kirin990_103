/*
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

#ifndef __HDMITX_INFOFRAME_H__
#define __HDMITX_INFOFRAME_H__

#include <linux/types.h>
#include "hdmitx_ctrl.h"

enum hdmitx_infoframe_type {
	HDMITX_INFOFRAME_TYPE_VENDOR = 0x81,
	HDMITX_INFOFRAME_TYPE_AVI = 0x82,
	HDMITX_INFOFRAME_TYPE_SPD = 0x83,
	HDMITX_INFOFRAME_TYPE_AUDIO = 0x84,
	HDMITX_INFOFRAME_TYPE_DRM = 0X87
};

enum hdmitx_packet_type {
	HDMITX_PACKET_TYPE_NULL = 0x00,
	HDMITX_PACKET_TYPE_AUDIO_CLOCK_REGEN = 0x01,
	HDMITX_PACKET_TYPE_AUDIO_SAMPLE = 0x02,
	HDMITX_PACKET_TYPE_GENERAL_CONTROL = 0x03,
	HDMITX_PACKET_TYPE_ACP = 0x04,
	HDMITX_PACKET_TYPE_ISRC1 = 0x05,
	HDMITX_PACKET_TYPE_ISRC2 = 0x06,
	HDMITX_PACKET_TYPE_ONE_BIT_AUDIO_SAMPLE = 0x07,
	HDMITX_PACKET_TYPE_DST_AUDIO = 0x08,
	HDMITX_PACKET_TYPE_HBR_AUDIO_STREAM = 0x09,
	HDMITX_PACKET_TYPE_GAMUT_METADATA = 0x0a,
	HDMITX_PACKET_TYPE_EXTENDED_METADATA = 0x7f
};

#define HDMITX_IEEE_ID              0x000c03
#define HDMITX_FORUM_IEEE_ID        0xc45dd8
#define HDMITX_DOLBY_IEEE_ID        0x00d046
#define HDMITX_HDR10_PLUS_IEEE_ID   0x90848b
#define HDMITX_CUVA_IEEE_ID         0x047503
#define HDMITX_INFOFRAME_HEADER_SIZE 4
#define HDMITX_AVI_INFOFRAME_SIZE    13
#define HDMITX_SPD_INFOFRAME_SIZE    25
#define HDMITX_AUDIO_INFOFRAME_SIZE  10
#define HDMITX_DRM_INFOFRAME_SIZE    26
#define HDMITX_VENDOR_INFOFRAME_SIZE_OF_NONE 4
#define HDMITX_VENDOR_INFOFRAME_SIZE_OF_3D 7
#define HDMITX_VENDOR_INFOFRAME_SIZE_OF_4K 5
#define HDMITX_VENDOR_INFOFRAME_SIZE_OF_DOLBYV0 0x18

#define hdmitx_infoframe_size(type) (HDMITX_INFOFRAME_HEADER_SIZE + HDMITX_##type##_INFOFRAME_SIZE)

#define HDMITX_PACKET_SIZE 31

struct hdmitx_any_infoframe {
	enum hdmitx_infoframe_type type;
	u8 version;
	u8 length;
};

enum hdmitx_colorspace {
	HDMITX_COLORSPACE_RGB,
	HDMITX_COLORSPACE_YUV422,
	HDMITX_COLORSPACE_YUV444,
	HDMITX_COLORSPACE_YUV420,
	HDMITX_COLORSPACE_RESERVED4,
	HDMITX_COLORSPACE_RESERVED5,
	HDMITX_COLORSPACE_RESERVED6,
	HDMITX_COLORSPACE_IDO_DEFINED
};

enum hdmitx_scan_mode {
	HDMITX_SCAN_MODE_NONE,
	HDMITX_SCAN_MODE_OVERSCAN,
	HDMITX_SCAN_MODE_UNDERSCAN,
	HDMITX_SCAN_MODE_RESERVED
};

enum hdmitx_colorimetry {
	HDMITX_COLORIMETRY_NONE,
	HDMITX_COLORIMETRY_ITU_601,
	HDMITX_COLORIMETRY_ITU_709,
	HDMITX_COLORIMETRY_EXTENDED
};

enum hdmitx_picture_aspect {
	HDMITX_PICTURE_ASPECT_NONE,
	HDMITX_PICTURE_ASPECT_4_3,
	HDMITX_PICTURE_ASPECT_16_9,
	HDMITX_PICTURE_ASPECT_64_27,
	HDMITX_PICTURE_ASPECT_256_135,
	HDMITX_PICTURE_ASPECT_RESERVED
};

enum hdmitx_active_aspect {
	HDMITX_ACTIVE_ASPECT_16_9_TOP = 2,
	HDMITX_ACTIVE_ASPECT_14_9_TOP = 3,
	HDMITX_ACTIVE_ASPECT_16_9_CENTER = 4,
	HDMITX_ACTIVE_ASPECT_PICTURE = 8,
	HDMITX_ACTIVE_ASPECT_4_3 = 9,
	HDMITX_ACTIVE_ASPECT_16_9 = 10,
	HDMITX_ACTIVE_ASPECT_14_9 = 11,
	HDMITX_ACTIVE_ASPECT_4_3_SP_14_9 = 13,
	HDMITX_ACTIVE_ASPECT_16_9_SP_14_9 = 14,
	HDMITX_ACTIVE_ASPECT_16_9_SP_4_3 = 15
};

enum hdmitx_extended_colorimetry {
	HDMITX_EXTENDED_COLORIMETRY_XV_YCC_601,
	HDMITX_EXTENDED_COLORIMETRY_XV_YCC_709,
	HDMITX_EXTENDED_COLORIMETRY_S_YCC_601,
	HDMITX_EXTENDED_COLORIMETRY_OPYCC_601,
	HDMITX_EXTENDED_COLORIMETRY_OPRGB,

	/* The following EC values are only defined in CEA-861-F. */
	HDMITX_EXTENDED_COLORIMETRY_BT2020_CONST_LUM,
	HDMITX_EXTENDED_COLORIMETRY_BT2020,
	HDMITX_EXTENDED_COLORIMETRY_RESERVED
};

enum hdmitx_quantization_range {
	HDMITX_QUANTIZATION_RANGE_DEFAULT,
	HDMITX_QUANTIZATION_RANGE_LIMITED,
	HDMITX_QUANTIZATION_RANGE_FULL,
	HDMITX_QUANTIZATION_RANGE_RESERVED
};

/* non-uniform picture scaling */
enum hdmitx_nups {
	HDMITX_NUPS_UNKNOWN,
	HDMITX_NUPS_HORIZONTAL,
	HDMITX_NUPS_VERTICAL,
	HDMITX_NUPS_BOTH
};

enum hdmitx_ycc_quantization_range {
	HDMITX_YCC_QUANTIZATION_RANGE_LIMITED,
	HDMITX_YCC_QUANTIZATION_RANGE_FULL
};

enum hdmitx_content_type {
	HDMITX_CONTENT_TYPE_GRAPHICS,
	HDMITX_CONTENT_TYPE_PHOTO,
	HDMITX_CONTENT_TYPE_CINEMA,
	HDMITX_CONTENT_TYPE_GAME
};

struct hdmitx_avi_infoframe {
	enum hdmitx_infoframe_type type;
	u8 version;
	u8 length;
	enum hdmitx_colorspace colorspace;
	enum hdmitx_scan_mode scan_mode;
	enum hdmitx_colorimetry colorimetry;
	enum hdmitx_picture_aspect picture_aspect;
	enum hdmitx_active_aspect active_aspect;
	bool itc;
	enum hdmitx_extended_colorimetry extended_colorimetry;
	enum hdmitx_quantization_range quantization_range;
	enum hdmitx_nups nups;
	u8 video_code;
	enum hdmitx_ycc_quantization_range ycc_quantization_range;
	enum hdmitx_content_type content_type;
	u8 pixel_repeat;
	u16 top_bar;
	u16 bottom_bar;
	u16 left_bar;
	u16 right_bar;
};

s32 drv_hdmitx_avi_infoframe_init(struct hdmitx_avi_infoframe *frame);
s32 drv_hdmitx_avi_infoframe_pack(const struct hdmitx_avi_infoframe *frame, void *buffer, u32 size);
s32 drv_hdmitx_avi_infoframe_unpack(struct hdmitx_avi_infoframe *frame,
	const void *buffer, u32 size);

enum hdmitx_3d_structure {
	HDMITX_3D_STRUCTURE_INVALID = -1,
	HDMITX_3D_STRUCTURE_FRAME_PACKING = 0,
	HDMITX_3D_STRUCTURE_FIELD_ALTERNATIVE,
	HDMITX_3D_STRUCTURE_LINE_ALTERNATIVE,
	HDMITX_3D_STRUCTURE_SIDE_BY_SIDE_FULL,
	HDMITX_3D_STRUCTURE_L_DEPTH,
	HDMITX_3D_STRUCTURE_L_DEPTH_GFX_GFX_DEPTH,
	HDMITX_3D_STRUCTURE_TOP_AND_BOTTOM,
	HDMITX_3D_STRUCTURE_SIDE_BY_SIDE_HALF = 8,
};

struct hdmitx_vendor_infoframe {
	enum hdmitx_infoframe_type type;
	u8 version;
	u8 length;
	u32 oui;
	u8 vic;
	enum hdmitx_3d_structure s3d_struct;
	u32 s3d_ext_data;
};

s32 drv_hdmitx_vendor_infoframe_init(struct hdmitx_vendor_infoframe *frame);
s32 drv_hdmitx_vendor_infoframe_pack(const struct hdmitx_vendor_infoframe *frame,
	void *buffer, u32 size);
s32 drv_hdmitx_vendor_infoframe_unpack(struct hdmitx_vendor_infoframe *frame,
	const void *buffer, u32 size);

struct hdmitx_3d_infoframe {
	enum hdmitx_3d_structure s3d_struct;
	u32 s3d_ext_data;
	bool additional_info_present;
	bool disparity_data_present;
	bool meta_present;
	bool dual_view;
	u8 view_dependency;
	u8 preferred_2d_view;
	u8 disparity_data_version;
	u8 disparity_data_length;
	u8 disparity_data[10]; /* 10: len of 3d_disparity_data */
	u8 ext_data;
	u8 metadata_type;
	u8 metadata_length;
	u8 metadata[10]; /* 10: len of 3d_metadata */
};

struct hdmitx_forum_vendor_infoframe {
	enum hdmitx_infoframe_type type;
	u8 version;
	u8 length;
	u32 oui;
	u8 hf_version;
	u8 ccpbc;
	bool allm;
	bool s3d_valid;
	struct hdmitx_3d_infoframe s3d_infoframe;
}; /* for hdmi2.0 */

s32 drv_hdmitx_forum_vendor_infoframe_init(struct hdmitx_forum_vendor_infoframe *frame);
s32 drv_hdmitx_forum_vendor_infoframe_pack(const struct hdmitx_forum_vendor_infoframe *frame,
	void *buffer, u32 size);
s32 drv_hdmitx_forum_vendor_infoframe_unpack(struct hdmitx_forum_vendor_infoframe *frame,
	const void *buffer, u32 size);

union hdmitx_vendor_any_infoframe {
	struct {
		enum hdmitx_infoframe_type type;
		u8 version;
		u8 length;
		u32 oui;
	} any;
	struct hdmitx_vendor_infoframe hdmi;             /* for hdmi1.4 or dolby vision v2 */
	struct hdmitx_forum_vendor_infoframe hdmi_forum; /* for hdmi2.0 */
};

s32 hdmitx_set_infoframe(struct hdmitx_ctrl *hdmitx);
void hdmitx_init_infoframe(struct hdmitx_ctrl *hdmitx);

#endif /* __HDMITX_INFOFRAME_H__ */
