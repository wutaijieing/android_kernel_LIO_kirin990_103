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

#include <securec.h>
#include "dkmd_log.h"

#include "hdmitx_infoframe.h"
#include "hdmitx_core_config.h"

#define HDMITX_IF_CHECKSUM_OFFSET 3
#define HDMITX_AVI_IF_VER_2       2
#define HDMITX_AVI_IF_VER_3       3
#define HDMITX_AVI_IF_VER_4       4

static u8 hdmi_infoframe_checksum(const u8 *ptr, u32 size)
{
	u8 check_sum = 0;
	u32 i;
	u8 ret;

	/* count checksum */
	for (i = 0; i < size; i++)
		check_sum += ptr[i];

	ret = (u8)(0x100 - check_sum);
	return ret;
}

static void hdmi_infoframe_set_checksum(void *buffer, u32 size)
{
	u8 *ptr = buffer;

	ptr[HDMITX_IF_CHECKSUM_OFFSET] = hdmi_infoframe_checksum(buffer, size);
}

s32 drv_hdmitx_avi_infoframe_init(struct hdmitx_avi_infoframe *frame)
{
	if (frame == NULL) {
		dpu_pr_err("null ptr!\n");
		return -1;
	}

	if (memset_s(frame, sizeof(*frame), 0, sizeof(*frame)) != EOK) {
		dpu_pr_err("memset_s err\n");
		return -1;
	}

	frame->type = HDMITX_INFOFRAME_TYPE_AVI;
	frame->version = HDMITX_AVI_IF_VER_2;
	frame->length = HDMITX_AVI_INFOFRAME_SIZE;

	return 0;
}

static s32 hdmitx_avi_infoframe_check(const struct hdmitx_avi_infoframe *frame)
{
	if (frame->type != HDMITX_INFOFRAME_TYPE_AVI ||
		(frame->version != HDMITX_AVI_IF_VER_2 &&
		frame->version != HDMITX_AVI_IF_VER_3 &&
		frame->version != HDMITX_AVI_IF_VER_4) ||
		frame->length != HDMITX_AVI_INFOFRAME_SIZE)
		return -1;

	if (frame->picture_aspect > HDMITX_PICTURE_ASPECT_256_135)
		return -1;

	return 0;
}

static void drv_hdmitx_avi_infoframe_pack_remain(const struct hdmitx_avi_infoframe *frame, u8 *ptr)
{
	*ptr++ = frame->video_code;
	*ptr++ = (((u8)frame->ycc_quantization_range & 0x3) << 6) | /* bit[7:6] is ycc_quantization_range */
			 (((u8)frame->content_type & 0x3) << 4) | /* bit[5:4] is content_type */
			 (frame->pixel_repeat & 0xf);
	*ptr++ = frame->top_bar & 0xff;
	*ptr++ = (frame->top_bar >> 8) & 0xff; /* high 8bits */
	*ptr++ = frame->bottom_bar & 0xff;
	*ptr++ = (frame->bottom_bar >> 8) & 0xff; /* high 8bits */
	*ptr++ = frame->left_bar & 0xff;
	*ptr++ = (frame->left_bar >> 8) & 0xff; /* high 8bits */
	*ptr++ = frame->right_bar & 0xff;
	*ptr++ = (frame->right_bar >> 8) & 0xff; /* high 8bits */
}

s32 drv_hdmitx_avi_infoframe_pack(const struct hdmitx_avi_infoframe *frame, void *buffer, u32 size)
{
	u8 *ptr = buffer;
	u8 tmp;
	u32 length;
	s32 ret;

	if (frame == NULL || buffer == NULL) {
		dpu_pr_err("null ptr!\n");
		return -1;
	}

	ret = hdmitx_avi_infoframe_check(frame);
	if (ret) {
		dpu_pr_err("avi check err\n");
		return ret;
	}

	length = HDMITX_INFOFRAME_HEADER_SIZE + frame->length;
	if (size < length) {
		dpu_pr_err("avi packet over size\n");
		return -1;
	}

	if (memset_s(buffer, size, 0, size) != EOK) {
		dpu_pr_err("avi memset_s err\n");
		return -1;
	}

	*ptr++ = frame->type;
	*ptr++ = frame->version;
	*ptr++ = frame->length;
	*ptr++ = 0; /* checksum */

	/* Data byte 1, bit[6:5] is colorspace, bit[1:0] is scan mode */
	tmp = (((u8)frame->colorspace & 0x3) << 5) | ((u8)frame->scan_mode & 0x3);
	/* Data byte 1, bit 4 has to be set if we provide the active format aspect ratio */
	if ((u8)frame->active_aspect & 0xf)
		tmp |= (1 << 4); /* bit 4 is set to 1 */

	/* Bit 3 and 2 indicate if we transmit horizontal/vertical bar data */
	if (frame->top_bar || frame->bottom_bar)
		tmp |= (1 << 3); /* bit 3 is set to 1 */

	if (frame->left_bar || frame->right_bar)
		tmp |= (1 << 2); /* bit 2 is set to 1 */

	*ptr++ = tmp;

	/* bit[7:6] is colorimetry, bit[5:4] is picture aspect */
	*ptr++ = (((u8)frame->colorimetry & 0x3) << 6) | (((u8)frame->picture_aspect & 0x3) << 4) |
			((u8)frame->active_aspect & 0xf);

	/* bit[6:4] is extended_colorimetry, bit[3:2] is quantization_range */
	tmp = (((u8)frame->extended_colorimetry & 0x7) << 4) | (((u8)frame->quantization_range & 0x3) << 2) |
			((u8)frame->nups & 0x3);
	if (frame->itc)
		tmp |= 1 << 7; /* bit7 is itc */

	*ptr++ = tmp;

	drv_hdmitx_avi_infoframe_pack_remain(frame, ptr);

	hdmi_infoframe_set_checksum(buffer, length);

	return length;
}

s32 drv_hdmitx_vendor_infoframe_init(struct hdmitx_vendor_infoframe *frame)
{
	if (frame == NULL) {
		dpu_pr_warn("null ptr!\n");
		return -1;
	}

	if (memset_s(frame, sizeof(*frame), 0, sizeof(*frame)) != EOK) {
		dpu_pr_warn("memset_s err\n");
		return -1;
	}

	frame->type = HDMITX_INFOFRAME_TYPE_VENDOR;
	frame->version = 1;
	frame->oui = HDMITX_IEEE_ID;
	/* 0 is a valid value for s3d_struct, so we use a special "not set" value */
	frame->s3d_struct = HDMITX_3D_STRUCTURE_INVALID;
	frame->length = HDMITX_VENDOR_INFOFRAME_SIZE_OF_NONE;

	return 0;
}

static s32 hdmitx_vendor_infoframe_check(const struct hdmitx_vendor_infoframe *frame)
{
	if (frame->type != HDMITX_INFOFRAME_TYPE_VENDOR ||
		frame->version != 1 ||
		frame->oui != HDMITX_IEEE_ID)
		return -1;

	if (frame->length == HDMITX_VENDOR_INFOFRAME_SIZE_OF_DOLBYV0)
		return 0;

	if (frame->s3d_struct != HDMITX_3D_STRUCTURE_INVALID) {
		if (frame->length != HDMITX_VENDOR_INFOFRAME_SIZE_OF_3D)
			return -1;
	} else if (frame->vic != 0) {
		if (frame->length != HDMITX_VENDOR_INFOFRAME_SIZE_OF_4K)
			return -1;
	} else {
		if (frame->length != HDMITX_VENDOR_INFOFRAME_SIZE_OF_NONE)
			return -1;
	}

	return 0;
}

s32 drv_hdmitx_vendor_infoframe_pack(const struct hdmitx_vendor_infoframe *frame,
	void *buffer, u32 size)
{
	u8 *ptr = buffer;
	u32 length;
	s32 ret;

	if (frame == NULL || buffer == NULL) {
		dpu_pr_err("null ptr!\n");
		return -1;
	}

	ret = hdmitx_vendor_infoframe_check(frame);
	if (ret) {
		dpu_pr_err("VSIF check err\n");
		return ret;
	}

	length = HDMITX_INFOFRAME_HEADER_SIZE + frame->length;
	if (size < length) {
		dpu_pr_err("VSIF over size\n");
		return -1;
	}

	if (memset_s(buffer, size, 0, size) != EOK) {
		dpu_pr_err("vsif memset_s err\n");
		return -1;
	}

	*ptr++ = frame->type;
	*ptr++ = frame->version;
	*ptr++ = frame->length;
	*ptr++ = 0; /* checksum */

	/* HDMITX OUI */
	*ptr++ = 0x03;
	*ptr++ = 0x0c;
	*ptr++ = 0x00;

	if (frame->s3d_struct != HDMITX_3D_STRUCTURE_INVALID) {
		*ptr++ = 0x2 << 5; /* bit[7:5] is video format */
		*ptr++ = ((u8)frame->s3d_struct & 0xf) << 4; /* bit[7:4] is s3d_struct */
		if (frame->s3d_struct >= HDMITX_3D_STRUCTURE_SIDE_BY_SIDE_HALF)
			*ptr++ = (frame->s3d_ext_data & 0xf) << 4; /* bit[7:4] is s3d_ext_data */
	} else if (frame->vic) {
		*ptr++ = 0x1 << 5; /* bit[7:5] is video format */
		*ptr++ = frame->vic;
	}

	hdmi_infoframe_set_checksum(buffer, length);

	return length;
}

static void hdmitx_avi_infoframe_config(struct hdmitx_soft_status *soft_status,
	struct hdmitx_avi_infoframe *frame)
{
	struct hdmitx_avi_data *avi_data = NULL;
	struct dpu_video_info *info = NULL;
	u32 colorimetry;
	u32 extended_colorimetry;

	info = &soft_status->info;
	avi_data = &soft_status->info.avi_data;
	colorimetry = avi_data->color.colorimetry & 0xf;
	extended_colorimetry = (avi_data->color.colorimetry >> 4) & 0xf; /* The upper 4 bits is extended colorimetry. */

	frame->pixel_repeat = avi_data->pixel_repeat;
	frame->video_code = (avi_data->vic > 255) ? 0 : (avi_data->vic & 0xff); /* vic > 255 is not CEA timing. */

	/*
	 * The video code must be 0 in the avi infoframe, when the timing
	 * is HDMITX_3840X2160P24_16_9(93), HDMITX_3840X2160P25_16_9(94),
	 * HDMITX_3840X2160P30_16_9(95), HDMITX_4096X2160P24_256_135(98) at 2D mode.
	 */
	if ((frame->video_code == VIC_3840X2160P24_16_9 ||
		frame->video_code == VIC_3840X2160P25_16_9 ||
		frame->video_code == VIC_3840X2160P30_16_9 ||
		frame->video_code == VIC_4096X2160P24_256_135) &&
		info->mode_3d == HDMITX_3D_NONE)
		frame->video_code = 0;

	frame->picture_aspect = avi_data->picture_aspect_ratio;
	frame->content_type = avi_data->it_content_type;
	frame->itc = avi_data->it_content_valid;
	frame->active_aspect = avi_data->active_aspect_ratio;
	frame->scan_mode = avi_data->scan_info;
	frame->colorspace = avi_data->color.color_format;
	frame->colorimetry = colorimetry;
	frame->extended_colorimetry = extended_colorimetry;
	frame->quantization_range = avi_data->color.rgb_quantization;
	frame->ycc_quantization_range = avi_data->color.ycc_quantization;
	frame->top_bar = avi_data->top_bar;
	frame->bottom_bar = avi_data->bottom_bar;
	frame->left_bar = avi_data->left_bar;
	frame->right_bar = avi_data->right_bar;
	frame->nups = avi_data->picture_scal;

	/* Y2 = 1 or vic >= 128, version shall use 3 */
	if (frame->video_code > VIC_5120X2160P100_64_27) {
		frame->version = 3; /* Avi infoframe version need be 3, when vic > 127. */
	} else if (frame->colorimetry == HDMITX_COLORIMETRY_EXTENDED &&
		frame->extended_colorimetry == HDMITX_EXTENDED_COLORIMETRY_RESERVED) {
		frame->version = 4; /* Avi infoframe version need be 4, when extended colorimetry is reserved. */
	}
}

static s32 hdmitx_set_avi_infoframe(struct hdmitx_ctrl *hdmitx)
{
	struct hdmitx_avi_infoframe frame;
	u8 buffer[HDMITX_PACKET_SIZE] = {0};
	ssize_t err;

	err = drv_hdmitx_avi_infoframe_init(&frame);
	if (err < 0) {
		dpu_pr_warn("Failed to setup avi infoframe: %zd\n", err);
		return err;
	}

	hdmitx_avi_infoframe_config(&hdmitx->soft_status, &frame);

	err = drv_hdmitx_avi_infoframe_pack(&frame, buffer, sizeof(buffer));
	if (err < 0) {
		dpu_pr_warn("Failed to pack AVI infoframe: %zd\n", err);
		return err;
	}

	dpu_pr_info("avi infoframe : %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		/* 0,1,2,3:header of avi infoframe */
		buffer[0], buffer[1], buffer[2], buffer[3],
		/* 4,5,6,7,8,9:avi infoframe body */
		buffer[4], buffer[5], buffer[6], buffer[7], buffer[8], buffer[9],
		/* 10,11,12,13,14,15:avi infoframe body */
		buffer[10], buffer[11], buffer[12], buffer[13], buffer[14], buffer[15]);

	core_hw_send_info_frame(hdmitx->base, buffer, sizeof(buffer));
	return 0;
}

static s32 hdmitx_set_vendor_specific_infoframe(struct hdmitx_ctrl *hdmitx)
{
	struct hdmitx_vendor_infoframe frame;
	struct hdmitx_avi_data *avi_data = NULL;
	u8 buffer[HDMITX_PACKET_SIZE] = {0};
	ssize_t err;
	struct dpu_video_info *info = NULL;

	info = &hdmitx->soft_status.info;
	avi_data = &hdmitx->soft_status.info.avi_data;
	err = drv_hdmitx_vendor_infoframe_init(&frame);
	if (err < 0) {
		dpu_pr_warn("Failed to setup vendor infoframe: %zd\n", err);
		return err;
	}

	if (info->mode_3d == HDMITX_3D_NONE) {
		if (avi_data->vic == VIC_3840X2160P30_16_9) {
			frame.vic = 1; /* hdmi vic is 1 */
		} else if (avi_data->vic == VIC_3840X2160P25_16_9) {
			frame.vic = 2; /* hdmi vic is 2 */
		} else if (avi_data->vic == VIC_3840X2160P24_16_9) {
			frame.vic = 3; /* hdmi vic is 3 */
		} else if (avi_data->vic == VIC_4096X2160P24_256_135) {
			frame.vic = 4; /* hdmi vic is 4 */
		}
	}

	frame.s3d_struct = (info->mode_3d == HDMITX_3D_NONE) ?
		HDMITX_3D_STRUCTURE_INVALID : info->mode_3d;

	if (frame.vic) {
		frame.length = 5; /* hdmi vic is not zero, the length must be 5. */
	} else if (frame.s3d_struct != HDMITX_3D_STRUCTURE_INVALID) {
		frame.length = 7; /* 3d struct is not none, the length must be 7. */
	}

	err = drv_hdmitx_vendor_infoframe_pack(&frame, buffer, sizeof(buffer));
	if (err < 0) {
		dpu_pr_warn("Failed to pack vendor infoframe: %zd\n", err);
		return err;
	}

	dpu_pr_info("vendor infoframe : %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
		/* 0,1,2,3:header of vendor infoframe */
		buffer[0], buffer[1], buffer[2], buffer[3],
		/* 4,5,6,7,8,9,10:vendor infoframe body */
		buffer[4], buffer[5], buffer[6], buffer[7], buffer[8], buffer[9], buffer[10]);

	core_hw_send_info_frame(hdmitx->base, buffer, sizeof(buffer));
	return 0;
}

s32 hdmitx_set_infoframe(struct hdmitx_ctrl *hdmitx)
{
	s32 ret;

	if (hdmitx == NULL) {
		dpu_pr_err("Input params is NULL pointer\n");
		return -1;
	}

	ret = hdmitx_set_avi_infoframe(hdmitx);
	if (ret) {
		dpu_pr_warn("Send avi infoframe fail %d\n", ret);
		return ret;
	}

	ret = hdmitx_set_vendor_specific_infoframe(hdmitx);
	if (ret) {
		dpu_pr_warn("Send vendor specific infoframe fail %d\n", ret);
		return ret;
	}

	return ret;
}

static void hdmitx_avi_data_init(struct hdmitx_display_mode *select_mode, struct dpu_video_info *info)
{
	struct hdmitx_avi_data *avi = &info->avi_data;
	struct hdmi_detail *detail = &select_mode->detail;

	avi->vic = detail->vic;

	avi->scan_info = HDMITX_SCAN_MODE_NONE; // 0
	avi->bar_present = HDMITX_BAR_DATA_NO_PRESENT; // 0
	avi->active_aspect_present = false;
	avi->color.color_format = HDMITX_COLOR_FORMAT_RGB; // 0

	avi->active_aspect_ratio = HDMITX_ACT_ASP_RATIO_ATSC_SAME_PIC; // 1000
	avi->picture_aspect_ratio = detail->pic_asp_ratio;
	avi->color.colorimetry = HDMITX_COLORIMETRY_NO_DATA; // 0

	avi->it_content_valid = false;
	avi->color.rgb_quantization = HDMITX_RGB_QUANTIZEION_DEFAULT; // 0
	avi->color.ycc_quantization = HDMITX_RGB_QUANTIZEION_DEFAULT; // 0
	avi->picture_scal = HDMITX_NUPS_UNKNOWN; // 0

	avi->it_content_type = 0;
	avi->pixel_repeat = 0;

	avi->top_bar = 0;
	avi->bottom_bar = 0;
	avi->left_bar = 0;
	avi->right_bar = 0;
}

void hdmitx_init_infoframe(struct hdmitx_ctrl *hdmitx)
{
	struct hdmitx_display_mode *select_mode = &hdmitx->select_mode;
	struct dpu_video_info *info = &hdmitx->soft_status.info;

	hdmitx_avi_data_init(select_mode, info);
	info->mode_3d = HDMITX_3D_NONE;
}

