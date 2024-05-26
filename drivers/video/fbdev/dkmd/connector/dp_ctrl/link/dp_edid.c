/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#include "dp_edid.h"
#include "dp_dtd_helper.h"

static const uint8_t edid_v1_other_descriptor_flag[2] = {0x00, 0x00};

static struct list_head *dptx_hdmi_list;
struct dptx_hdmi_vic {
	struct list_head list_node;
	uint32_t vic_id;
};
static uint8_t g_hdmi_vic_len;
static uint8_t g_hdmi_vic_real_len;

int parse_edid(struct dp_ctrl *dptx, uint16_t len)
{
	int16_t i, ext_block_num;
	int ret;
	uint8_t *edid_t = NULL;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL!\n");

	edid_t = dptx->edid;

	/* Check if data has any error */
	dpu_check_and_return(!edid_t, -EINVAL, err, "[DP] Raw Data is invalid!(NULL error)!\n");

	if (((len / EDID_LENGTH) > 5) || ((len % EDID_LENGTH) != 0) || (len < EDID_LENGTH)) {
		dpu_pr_err("[DP] Raw Data length is invalid(not the size of (128 x N , N = [1-5]) uint8_t!");
		return -EINVAL;
	}

	/* Parse the EDID main part, check how many(count as ' ext_block_num ') Extension blocks there are to follow. */
	ext_block_num = parse_main(dptx);
	if ((ext_block_num > 0) && (len == (uint16_t)((ext_block_num + 1) * EDID_LENGTH))) {
		dptx->edid_info.audio.ext_acount = 0;
		dptx->edid_info.audio.ext_speaker = 0;
		dptx->edid_info.audio.basic_audio = 0;
		dptx->edid_info.audio.spec = NULL;
		/* Parse all Extension blocks */
		for (i = 0; i < ext_block_num; i++) {
			ret = parse_extension(dptx, edid_t + (EDID_LENGTH * (i + 1)));
			if (ret != 0) {
				dpu_pr_err(
					"[DP] Extension Parsing failed!Only returning the Main Part of this EDID!\n");
				break;
			}
		}
	} else if (ext_block_num < 0) {
		dpu_pr_err("[DP] Error occurred while parsing, returning with NULL!");
		return -EINVAL;
	}

	for (i = 0; i < len;) {
		if (!(i % 16)) {
			printk(KERN_INFO "EDID [%04x]:  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
				i, dptx->edid[i], dptx->edid[i + 1], dptx->edid[i + 2], dptx->edid[i + 3],
				dptx->edid[i + 4], dptx->edid[i + 5], dptx->edid[i + 6], dptx->edid[i + 7],
				dptx->edid[i + 8], dptx->edid[i + 9], dptx->edid[i + 10], dptx->edid[i + 11],
				dptx->edid[i + 12], dptx->edid[i + 13], dptx->edid[i + 14], dptx->edid[i + 15]);
		}

		i += 16;

		if (i == 128)
			printk(KERN_INFO "<<<-------------------------------------------------------------->>>\n");
	}

	return 0;
}

int check_main_edid(uint8_t *edid)
{
	uint32_t checksum = 0;
	int32_t i;

	dpu_check_and_return((edid == NULL), -EINVAL, err, "[DP] edid is NULL!\n");
	/* Verify 0 checksum */
	for (i = 0; i < EDID_LENGTH; i++)
		checksum += edid[i];
	if (checksum & 0xFF) {
		dpu_pr_err("[DP] EDID checksum failed - data is corrupt. Continuing anyway.\n");
		return -EINVAL;
	}

	/* Verify Header */
	for (i = 0; i < EDID_HEADER_END + 1; i++) {
		if (edid[i] != edid_v1_header[i]) {
			dpu_pr_info("[DP] first uint8_ts don't match EDID version 1 header\n");
			return -EINVAL;
		}
	}

	return 0;
}

static int check_factory_info(uint8_t *edid)
{
	uint8_t i;
	uint8_t data_tmp;

	data_tmp = 0;

	dpu_check_and_return((edid == NULL), -EINVAL, err, "[DP] edid is NULL!\n");

	/* For hiding custom device info */
	for (i = EDID_FACTORY_START; i <= EDID_FACTORY_END; i++)
		data_tmp += edid[i];
	memset(&edid[EDID_FACTORY_START], 0x0, (EDID_FACTORY_END - EDID_FACTORY_START + 1));
	edid[EDID_FACTORY_START] = data_tmp; // For EDID checksum, we need reserve the sum for blocks.

	return 0;
}

int parse_main(struct dp_ctrl *dptx)
{
	int16_t i;
	int ret;
	uint8_t *block = NULL;
	uint8_t *edid_t = NULL;
	struct edid_video *vid_info = NULL;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL!\n");
	dpu_check_and_return(!dptx->edid, -EINVAL, err, "[DP] dptx->edid is NULL!\n");

	dptx->edid_info.video.dptx_timing_list = NULL;
	dptx->edid_info.video.ext_timing = NULL;
	dptx->edid_info.video.dp_monitor_descriptor = NULL;
	dptx->edid_info.video.ext_vcount = 0;
	edid_t = dptx->edid;
	/* Initialize some fields */
	vid_info = &(dptx->edid_info.video);
	vid_info->main_vcount = 0;
	vid_info->max_pixel_clock = 0;

	if (check_main_edid(edid_t)) {
		dpu_pr_err("[DP] The main edid block is wrong");
		return -EINVAL;
	}

	check_factory_info(edid_t);

	/* Check EDID version (usually 1.3) */
	dpu_pr_info("[DP] EDID version %d revision %d", (int)edid_t[EDID_STRUCT_VERSION],
		(int)edid_t[EDID_STRUCT_REVISION]);

	/* Check Display Image Size(Physical) */
	vid_info->max_himage_size = (uint16_t)edid_t[H_MAX_IMAGE_SIZE];
	vid_info->max_vimage_size = (uint16_t)edid_t[V_MAX_IMAGE_SIZE];
	/* Alloc the name memory */
	if (vid_info->dp_monitor_descriptor == NULL) {
		vid_info->dp_monitor_descriptor =
			(char *)kzalloc(MONTIOR_NAME_DESCRIPTION_SIZE * sizeof(char), GFP_KERNEL);
		if (vid_info->dp_monitor_descriptor == NULL) {
			dpu_pr_err("[DP] dp_monitor_descriptor memory alloc fail\n");
			return -EINVAL;
		}
	}
	memset(vid_info->dp_monitor_descriptor, 0, MONTIOR_NAME_DESCRIPTION_SIZE);
	/* Parse the EDID Detailed Timing Descriptor */
	block = edid_t + DETAILED_TIMING_DESCRIPTIONS_START;
	/* EDID main part has a total of four Descriptor Block */
	for (i = 0; i < DETAILED_TIMING_DESCRIPTION_COUNT; i++, block += DETAILED_TIMING_DESCRIPTION_SIZE) {
		switch (block_type(block)) {
		case DETAILED_TIMING_BLOCK:
			ret = parse_timing_description(dptx, block);
			if (ret != 0) {
				dpu_pr_info("[DP] Timing Description Parsing failed!\n");
				return ret;
			}
			break;
		case MONITOR_LIMITS:
			ret = parse_monitor_limits(dptx, block);
			if (ret != 0) {
				dpu_pr_info("[DP] Parsing the monitor limit is failed!\n");
				return ret;
			}
			break;
		case MONITOR_NAME:
			ret = parse_monitor_name(dptx, block, DETAILED_TIMING_DESCRIPTION_SIZE);
			if (ret != 0) {
				dpu_pr_err("[DP] The monitor name parsing failed.\n");
				return ret;
			}
			break;
		default:
			break;
		}
	}

	dpu_pr_info("[DP] Extensions to follow:\t%d\n", (int)edid_t[EXTENSION_FLAG]);
	/* Return the number of following extension blocks */
	return (int)edid_t[EXTENSION_FLAG];
}

int check_exten_edid(uint8_t *exten)
{
	uint32_t i, checksum;

	checksum = 0;

	dpu_check_and_return(!exten, -EINVAL, err, "[DP] exten is NULL!\n");

	for (i = 0; i < EDID_LENGTH; i++)
		checksum += exten[i];
	if (checksum & 0xFF) {
		dpu_pr_err("[DP] Extension Data checksum failed - data is corrupt. Continuing anyway.\n");
		return -EINVAL;
	}
	/* Check Extension Tag */
	/* ( Header Tag stored in the first uint8_t ) */
	if (exten[0] != EXTENSION_HEADER) {
		dpu_pr_err("[DP] Not CEA-EDID Timing Extension, Extension-Parsing will not continue!\n");
		return -EINVAL;
	}
	return 0;
}

int parse_extension_timing_description(struct dp_ctrl *dptx, uint8_t *dtd_block, uint32_t dtd_begin, uint16_t dtd_total)
{
	uint32_t i;
	int ret;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL!\n");
	dpu_check_and_return(!dtd_block, -EINVAL, err, "[DP] dtd_block is NULL!\n");

	if ((dtd_begin + 1 + dtd_total * DETAILED_TIMING_DESCRIPTION_SIZE) > EDID_LENGTH) {
		dpu_pr_err("[DP] The dtd total number 0x%x is out of the limit\n", dtd_total);
		return -EINVAL;
	}

	for (i = 0; i < dtd_total; i++, dtd_block += DETAILED_TIMING_DESCRIPTION_SIZE) {
		switch (block_type(dtd_block)) {
		case DETAILED_TIMING_BLOCK:
			ret = parse_timing_description(dptx, dtd_block);
			if (ret != 0) {
				dpu_pr_err("[DP] Timing Description Parsing failed!");
				return ret;
			}
			break;
		case MONITOR_LIMITS:
			parse_monitor_limits(dptx, dtd_block);
			break;
		default:
			break;
		}
	}
	return 0;
}

int parse_extension(struct dp_ctrl *dptx, uint8_t *exten)
{
	int ret;
	uint8_t *dtd_block = NULL;
	uint8_t *cea_block = NULL;
	uint8_t dtd_start_byte = 0;
	uint8_t cea_data_block_collection = 0;
	uint16_t dtd_total = 0;
	struct edid_video *vid_info = NULL;
	struct edid_audio *aud_info = NULL;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL!\n");
	dpu_check_and_return(!exten, -EINVAL, err, "[DP] exten is NULL!\n");

	vid_info = &(dptx->edid_info.video);
	aud_info = &(dptx->edid_info.audio);

	ret = check_exten_edid(exten);
	if (ret) {
		dpu_pr_err("[DP] The check_exten_edid failed.\n");
		return ret;
	}
	/*
	 * Get uint8_t number (decimal) within this block where the 18-uint8_t DTDs begin.
	 * ( Number data stored in the third uint8_t )
	 */

	if (exten[2] == 0x00) {
		dpu_pr_info("[DP] There are no DTDs present in this extension block and no non-DTD data.\n");
		return -EINVAL;
	} else if (exten[2] == 0x04) {
		dtd_start_byte = 0x04;
	} else {
		cea_data_block_collection = 0x04;
		dtd_start_byte = exten[2];
	}
	/*
	 * Get Number of Native DTDs present, other Version 2+ information
	 * DTD Total stored in the fourth uint8_t )
	 */
	if (aud_info->basic_audio != 1)
		aud_info->basic_audio = (0x40 & exten[3]) >> 6;
	dp_imonitor_set_param(DP_PARAM_BASIC_AUDIO, &(aud_info->basic_audio));

	dtd_total = lower_nibble(exten[3]);
	// Parse DTD in Extension
	dtd_block = exten + dtd_start_byte;
	if (dtd_total != (EDID_LENGTH - dtd_start_byte - 1) / DETAILED_TIMING_DESCRIPTION_SIZE) {
		dpu_pr_info("[DP] The number of native DTDs is not equal the size\n");
		dtd_total = (EDID_LENGTH - dtd_start_byte - 1) / DETAILED_TIMING_DESCRIPTION_SIZE;
	}

	ret = parse_extension_timing_description(dptx, dtd_block, dtd_start_byte, dtd_total);
	if (ret) {
		dpu_pr_err("[DP] Parse the extension block timing information fail.\n");
		return ret;
	}
	/* Parse CEA Data Block Collection */
	if (cea_data_block_collection == 0x04) {
		cea_block = exten + cea_data_block_collection;
		ret = parse_cea_data_block(dptx, cea_block, dtd_start_byte);
		if (ret != 0) {
			dpu_pr_err("[DP] CEA data block Parsing failed!\n");
			return ret;
		}
	}

	return 0;
}
/* lint -e429 */
int parse_timing_description(struct dp_ctrl *dptx, uint8_t *dtd)
{
	struct edid_video *vid_info = NULL;
	struct timing_info *node = NULL;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL!\n");
	dpu_check_and_return(!dtd, -EINVAL, err, "[DP] dtd is NULL!\n");

	vid_info = &(dptx->edid_info.video);

	if (vid_info->dptx_timing_list == NULL) {
		vid_info->dptx_timing_list = kzalloc(sizeof(struct list_head), GFP_KERNEL);
		if (vid_info->dptx_timing_list == NULL) {
			dpu_pr_err("[DP] vid_info->dptx_timing_list is NULL");
			return -EINVAL;
		}
		INIT_LIST_HEAD(vid_info->dptx_timing_list);

		vid_info->main_vcount = 0;
		/* Initial Max Value */
		vid_info->max_hpixels = H_ACTIVE;
		vid_info->max_vpixels = V_ACTIVE;
		vid_info->max_pixel_clock = PIXEL_CLOCK;
	}

	/* Get Max Value by comparing all values */
	if ((vid_info->max_hpixels < H_ACTIVE) && (vid_info->max_vpixels <= V_ACTIVE) &&
		(vid_info->max_pixel_clock < PIXEL_CLOCK)) {
		vid_info->max_hpixels = H_ACTIVE;
		vid_info->max_vpixels = V_ACTIVE;
		vid_info->max_pixel_clock = PIXEL_CLOCK;
	}

	node = kzalloc(sizeof(struct timing_info), GFP_KERNEL);
	if (node != NULL) {
		node->hactive_pixels = H_ACTIVE;
		node->hblanking = H_BLANKING;
		node->hsync_offset = H_SYNC_OFFSET;
		node->hsync_pulse_width = H_SYNC_WIDTH;
		node->hborder = H_BORDER;
		node->hsize = H_SIZE;

		node->vactive_pixels = V_ACTIVE;
		node->vblanking = V_BLANKING;
		node->vsync_offset = V_SYNC_OFFSET;
		node->vsync_pulse_width = V_SYNC_WIDTH;
		node->vborder = V_BORDER;
		node->vsize = V_SIZE;

		node->pixel_clock = PIXEL_CLOCK;

		node->input_type = INPUT_TYPE; // need to modify later
		node->interlaced = INTERLACED;
		node->vsync_polarity = V_SYNC_POLARITY;
		node->hsync_polarity = H_SYNC_POLARITY;
		node->sync_scheme = SYNC_SCHEME;
		node->scheme_detail = SCHEME_DETAIL;

		vid_info->main_vcount += 1;
		list_add_tail(&node->list_node, vid_info->dptx_timing_list);
	} else {
		dpu_pr_err("[DP] kzalloc struct dptx_hdmi_vic fail!\n");
		return -EINVAL;
	}

	dpu_pr_info("[DP] The timinginfo %d: hactive_pixels is %d, vactive_pixels is %d, pixel clock=%llu\n",
		vid_info->main_vcount, node->hactive_pixels, node->vactive_pixels, node->pixel_clock);

	return 0;
}

int parse_timing_description_by_vesaid(struct edid_video *vid_info, uint8_t vesa_id)
{
	struct dtd mdtd;
	struct timing_info *node = NULL;

	dpu_check_and_return(!vid_info, -EINVAL, err, "[DP] vid_info is NULL!\n");

	if (!convert_code_to_dtd(&mdtd, vesa_id, 60000, VCEA)) {
		dpu_pr_info("[DP] Invalid video mode value %d\n",
						vesa_id);
		return -EINVAL;
	}

	if (mdtd.interlaced) {
		dpu_pr_info("[DP] Don't Support interlace mode %d\n",
						vesa_id);
		return -EINVAL;
	}

	if (vid_info->dptx_timing_list == NULL) {
		vid_info->dptx_timing_list = kzalloc(sizeof(struct list_head), GFP_KERNEL);
		if (vid_info->dptx_timing_list == NULL) {
			dpu_pr_err("[DP] vid_info->dptx_timing_list is NULL");
			return -EINVAL;
		}
		INIT_LIST_HEAD(vid_info->dptx_timing_list);

		vid_info->main_vcount = 0;
		/* Initial Max Value */
		vid_info->max_hpixels = mdtd.h_active;
		vid_info->max_vpixels = mdtd.v_active;
		vid_info->max_pixel_clock = mdtd.pixel_clock / 10;
	}

	/* Get Max Value by comparing all values */
	if ((mdtd.h_active > vid_info->max_hpixels) && (mdtd.v_active >= vid_info->max_vpixels) &&
		((mdtd.pixel_clock / 10) > vid_info->max_pixel_clock)) {
		vid_info->max_hpixels = mdtd.h_active;
		vid_info->max_vpixels = mdtd.v_active;
		vid_info->max_pixel_clock = mdtd.pixel_clock / 10;
	}

	node = kzalloc(sizeof(struct timing_info), GFP_KERNEL);
	if (node != NULL) {
		node->hactive_pixels = mdtd.h_active;
		node->hblanking = mdtd.h_blanking;
		node->hsync_offset = mdtd.h_sync_offset;
		node->hsync_pulse_width = mdtd.h_sync_pulse_width;
		node->hsize = mdtd.h_image_size;

		node->vactive_pixels = mdtd.v_active;
		node->vblanking = mdtd.v_blanking;
		node->vsync_offset = mdtd.v_sync_offset;
		node->vsync_pulse_width = mdtd.v_sync_pulse_width;
		node->vsize = mdtd.v_image_size;

		node->pixel_clock = mdtd.pixel_clock / 10; // Edid pixel clock is 1/10 of dtd filled timing.
		node->interlaced = mdtd.interlaced;
		node->vsync_polarity = mdtd.v_sync_polarity;
		node->hsync_polarity = mdtd.h_sync_polarity;

		vid_info->main_vcount += 1;
		list_add_tail(&node->list_node, vid_info->dptx_timing_list);
	} else {
		dpu_pr_err("[DP] kzalloc struct dptx_hdmi_vic fail!\n");
		return -EINVAL;
	}

	dpu_pr_info("[DP] The timinginfo %d: hactive_pixels is %d, vactive_pixels is %d, pixel clock=%llu\n",
		vid_info->main_vcount, node->hactive_pixels, node->vactive_pixels, node->pixel_clock);

	return 0;
}

int parse_hdmi_vic_id(uint8_t vic_id)
{
	struct dptx_hdmi_vic *node = NULL;

	if (g_hdmi_vic_real_len >= g_hdmi_vic_len) {
		dpu_pr_err("[DP] The g_hdmi_vic_real_len is more than g_hdmi_vic_len.\n");
		return -EINVAL;
	}

	if (dptx_hdmi_list == NULL) {
		dptx_hdmi_list = kzalloc(sizeof(struct list_head), GFP_KERNEL);
		if (dptx_hdmi_list == NULL) {
			dpu_pr_err("[DP] dptx_hdmi_list is NULL");
			return -EINVAL;
		}
		INIT_LIST_HEAD(dptx_hdmi_list);
	}

	node = kzalloc(sizeof(struct dptx_hdmi_vic), GFP_KERNEL);
	if (node != NULL) {
		node->vic_id = vic_id;
		list_add_tail(&node->list_node, dptx_hdmi_list);
		g_hdmi_vic_real_len++;
	} else {
		dpu_pr_err("[DP] kzalloc struct dptx_hdmi_vic fail!\n");
	}

	return 0;
}
/*lint +e429*/
int parse_audio_spec_info(struct edid_audio *aud_info, struct edid_audio_info *spec_info, uint8_t *cea_data_block)
{
	if ((EXTEN_AUDIO_FORMAT <= 8) && (EXTEN_AUDIO_FORMAT >= 1)) {
		dpu_check_and_return((!aud_info || !spec_info), -EINVAL, err, "[DP] aud_info or spec_info is NULL!\n");
		/* Set up SAD fields */
		spec_info->format = EXTEN_AUDIO_FORMAT;
		spec_info->channels =  EXTEN_AUDIO_MAX_CHANNELS;
		spec_info->sampling = EXTEN_AUDIO_SAMPLING;
		if (EXTEN_AUDIO_FORMAT == 1)
			spec_info->bitrate = EXTEN_AUDIO_LPCM_BIT;
		else
			spec_info->bitrate = EXTEN_AUDIO_BITRATE;
		aud_info->ext_acount += 1;

		dpu_pr_info(
			"[DP] parse audio spec success. format(0x%x), channels(0x%x), sampling(0x%x), bitrate(0x%x)\n",
			spec_info->format, spec_info->channels, spec_info->sampling, spec_info->bitrate);
	}

	return 0;
}

int parse_extension_audio_tag(struct edid_audio *aud_info, uint8_t *cea_data_block, uint8_t temp_length)
{
	uint8_t i, xa;
	void *temp_ptr = NULL;

	dpu_check_and_return(!aud_info, -EINVAL, err, "[DP] aud_info is NULL!\n");
	dpu_check_and_return(!cea_data_block, -EINVAL, err, "[DP] cea_data_block is NULL!\n");
	dpu_check_and_return((temp_length < 1), -EINVAL, err, "[DP] The input param temp_length is wrong!\n");

	for (i = 0; i < (temp_length) / 3; i++) {
		xa = aud_info->ext_acount;

		if (xa == 0) {
			/* Initial audio part */
			if (aud_info->spec != NULL) {
				dpu_pr_err("[DP] The spec of audio is error.\n");
				return -EINVAL;
			}
			aud_info->spec = kzalloc(sizeof(struct edid_audio_info), GFP_KERNEL);
			if (aud_info->spec == NULL) {
				dpu_pr_err("[DP] malloc audio Spec part failed!\n");
				return -EINVAL;
			}
		} else {
			/* Add memory as needed with error handling */
			temp_ptr = kzalloc((xa + 1) * sizeof(struct edid_audio_info), GFP_KERNEL);
			if (temp_ptr == NULL) {
				dpu_pr_err("[DP] Realloc audio Spec part failed!\n");
				return -EINVAL;
			}
			if (aud_info->spec == NULL) {
				dpu_pr_err("[DP] The spec is NULL.\n");
				kfree(temp_ptr);
				return -EINVAL;
			}
			memcpy(temp_ptr, aud_info->spec, xa * sizeof(struct edid_audio_info));
			kfree(aud_info->spec);
			aud_info->spec = NULL;
			aud_info->spec = temp_ptr;
		}
		if (parse_audio_spec_info(aud_info, &(aud_info->spec[xa]), cea_data_block)) {
			dpu_pr_err("[DP] parse the audio spec info fail.\n");
			return -EINVAL;
		}

		cea_data_block += 3;
	}
	return 0;
}

int parse_extension_video_tag(struct edid_video *vid_info, uint8_t *cea_data_block, uint8_t length)
{
	uint8_t i;

	dpu_check_and_return(!vid_info, -EINVAL, err, "[DP] vid_info is NULL!\n");
	dpu_check_and_return(!cea_data_block, -EINVAL, err, "[DP] cea_data_block is NULL!\n");
	dpu_check_and_return((length < 1), -EINVAL, err, "[DP] The input param length is wrong!\n");

	vid_info->ext_timing = kzalloc(length * sizeof(struct ext_timing), GFP_KERNEL);
	if (vid_info->ext_timing == NULL) {
		dpu_pr_err("[DP] ext_timing memory alloc fail\n");
		return -EINVAL;
	}
	memset(vid_info->ext_timing, 0x0, length * sizeof(struct ext_timing));
	vid_info->ext_vcount = 0;

	for (i = 0; i < length; i++) {
		if (EXTEN_VIDEO_CODE != 0) {
			/* Set up SVD fields */
			vid_info->ext_timing[i].ext_format_code = EXTEN_VIDEO_CODE;
			vid_info->ext_vcount += 1;

			parse_timing_description_by_vesaid(vid_info, vid_info->ext_timing[i].ext_format_code);
		}
		cea_data_block += 1;
	}
	return 0;
}

static void parse_extension_vsdb_after_latency(uint8_t *cea_data_block, uint8_t length,
	uint8_t pos_after_latency)
{
	uint8_t i;
	bool b3dpresent = false;

	dpu_check_and_no_retval(!cea_data_block, err, "[DP] cea_data_block is NULL!\n");

	for (i = pos_after_latency; i < length; i++) {
		if (i == pos_after_latency) {
			b3dpresent = (cea_data_block[i] & 0x80) >> 7;
		} else if (i == pos_after_latency + 1) {
			g_hdmi_vic_len = (cea_data_block[i] & 0xE0) >> 5;
			if (g_hdmi_vic_len == 0) {
				dpu_pr_info("[DP] This EDID don't include HDMI additional video format (2).\n");
				return;
			}
			g_hdmi_vic_real_len = 0;
		} else if (i <= pos_after_latency + 1 + g_hdmi_vic_len) {
			parse_hdmi_vic_id(cea_data_block[i]);
		} else {
			return;
		}
	}
}

static void parse_hdmi_vsdb_timing_list(struct edid_video *vid_info)
{
#ifdef SUPPORT_HDMI_VSDB_BLOCK
	struct dptx_hdmi_vic *hdmi_vic_node = NULL;
	struct dptx_hdmi_vic *_node_ = NULL;
#endif

	dpu_check_and_no_retval(!vid_info, err, "[DP] vid_info is NULL!\n");
	dpu_check_and_no_retval(!dptx_hdmi_list, err, "[DP] dptx_hdmi_list is NULL!\n");

#ifdef SUPPORT_HDMI_VSDB_BLOCK
	list_for_each_entry_safe(hdmi_vic_node, _node_, dptx_hdmi_list, list_node) {
		switch (hdmi_vic_node->vic_id) {
		case 1:
			parse_timing_description_by_vesaid(vid_info, 95);
			break;
		case 2:
			parse_timing_description_by_vesaid(vid_info, 94);
			break;
		case 3:
			parse_timing_description_by_vesaid(vid_info, 93);
			break;
		case 4:
			parse_timing_description_by_vesaid(vid_info, 98);
			break;
		default:
			dpu_pr_err("[DP] hdmi_vic_node is illegal!\n");
			break;
		}
		list_del(&hdmi_vic_node->list_node);
		kfree(hdmi_vic_node);
	}
#endif
	kfree(dptx_hdmi_list);
	dptx_hdmi_list = NULL;
}

int parse_extension_vsdb_tag(struct edid_video *vid_info, uint8_t *cea_data_block, uint8_t length)
{
	uint8_t i;
	uint32_t ieee_flag;
	uint32_t hdmi_cec_port;
	uint8_t max_tmds_clock;
	uint8_t latency_fields;
	uint8_t interlaced_latency_fields;
	uint8_t hdmi_video_present;
	uint8_t vesa_id;
	bool support_ai = false;

	vesa_id = 0;
	g_hdmi_vic_real_len = 0;
	g_hdmi_vic_len = 0;
	latency_fields = 0;
	interlaced_latency_fields = 0;

	dpu_check_and_return(!vid_info, -EINVAL, err, "[DP] vid_info is NULL!\n");
	dpu_check_and_return(!cea_data_block, -EINVAL, err, "[DP] cea_data_block is NULL!\n");
	dpu_check_and_return((length < 5), -EINVAL, err, "[DP] VSDB length isn't correct!\n");

	ieee_flag = 0;
	ieee_flag = (cea_data_block[0]) | (cea_data_block[1] << 8) | (cea_data_block[2] << 16);

	if (ieee_flag != 0x000c03) {
		dpu_pr_info("[DP] This block isn't belong to HDMI block: %x.\n", ieee_flag);
		return 0;
	}

	hdmi_cec_port = ((cea_data_block[3] << 8) | (cea_data_block[4]));

	for (i = 5; i < min(length, (uint8_t)8); i++) {
		switch (i) {
		case 5:
			support_ai = (cea_data_block[i] & 0x80) >> 7;
			break;
		case 6:
			max_tmds_clock = cea_data_block[i];
			break;
		case 7:
			latency_fields = (cea_data_block[i] & 0x80) >> 7;
			interlaced_latency_fields = (cea_data_block[i] & 0x40) >> 6;
			hdmi_video_present = (cea_data_block[i] & 0x20) >> 5;
			if (hdmi_video_present == 0) {
				dpu_pr_info("[DP] This EDID don't include HDMI additional video format (1).\n");
				return 0;
			}
			break;
		default:
			break;
		}
	}

	if (2 * LATENCY_PRESENT + 8 < length)
		parse_extension_vsdb_after_latency(cea_data_block, length, 2 * LATENCY_PRESENT + 8);

	if (g_hdmi_vic_len == 0)
		return 0;
	dpu_pr_info("[DP] vic_id real length =%d , vic length=%d !\n", g_hdmi_vic_real_len, g_hdmi_vic_len);

	parse_hdmi_vsdb_timing_list(vid_info);

	return 0;
}

int parse_cea_data_block(struct dp_ctrl *dptx, uint8_t *cea_data, uint8_t dtd_start)
{
	uint8_t total_length, block_length;
	uint8_t *cea_data_block = cea_data;
	struct edid_video *vid_info = NULL;
	struct edid_audio *aud_info = NULL;
	/* exTlist *extlist; */
	/* Initialize some fields */

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL!\n");
	dpu_check_and_return(!cea_data, -EINVAL, err, "[DP] cea_data is NULL!\n");
	dpu_check_and_return((dtd_start > (EDID_LENGTH - 1)), -EINVAL, err,
		"[DP] The start of dtd is out of limit!\n");

	total_length = 4;
	vid_info = &(dptx->edid_info.video);
	aud_info = &(dptx->edid_info.audio);
	/* Parse CEA Data Block Collection */
	while (total_length < dtd_start) {
		/* Get length(total number of following uint8_ts of a certain tag) */
		block_length = get_cea_data_block_len(cea_data_block);
		/* Get tag code */
		switch (get_cea_data_block_tag(cea_data_block)) {
		case EXTENSION_AUDIO_TAG:
			/* Block type tag combined with data length takes 1 uint8_t */
			cea_data_block += 1;
			/* Each Short audio Descriptor(SAD) takes 3 uint8_ts */
			if (parse_extension_audio_tag(aud_info, cea_data_block, block_length)) {
				dpu_pr_err("[DP] parse_extension_audio_tag fail.\n");
				return -EINVAL;
			}
			cea_data_block += block_length;
			break;
		case EXTENSION_VIDEO_TAG:
			/* Block type tag combined with data length takes 1 uint8_t */
			cea_data_block += 1;
			/* Each Short video Descriptor(SVD) takes 1 uint8_t */
			if (parse_extension_video_tag(vid_info, cea_data_block, block_length)) {
				dpu_pr_err("[DP] parse_extension_video_tag fail.\n");
				return -EINVAL;
			}
			cea_data_block += block_length;
			break;
		case EXTENSION_VENDOR_TAG:
			cea_data_block += 1;
			if (parse_extension_vsdb_tag(vid_info, cea_data_block, block_length)) {
				dpu_pr_err("[DP] parse_extension_vsdb_tag fail.\n");
				return -EINVAL;
			}
			cea_data_block += block_length;
			break;
		case EXTENSION_SPEAKER_TAG:
			/*
			 * If the extension has Speaker Allocation Data,
			 * there should only be one
			 */
			cea_data_block += 1;
			aud_info->ext_speaker = EXTEN_SPEAKER;
			/* Speaker Allocation Data takes 3 uint8_ts */
			cea_data_block += 3;
			break;
		default:
			cea_data_block += block_length + 1;
			break;
		}
		total_length = total_length + block_length + 1;
	}

	return 0;
}

int block_type(uint8_t *block)
{
	dpu_check_and_return(!block, -EINVAL, err, "[DP] block is NULL!\n");

	if ((block[0] == 0) && (block[1] == 0)) {
		/* Other descriptor */
		if ((block[2] != 0) || (block[4] != 0))
			return UNKNOWN_DESCRIPTOR;
		return block[3];
	}
	/* Detailed timing block */
	return DETAILED_TIMING_BLOCK;
}

int parse_monitor_limits(struct dp_ctrl *dptx, uint8_t *block)
{
	struct edid_video *vid_info = NULL;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL!\n");
	dpu_check_and_return(!block, -EINVAL, err, "[DP] block is NULL!\n");

	vid_info = &(dptx->edid_info.video);
	/* Set up limit fields */
	vid_info->max_hfreq = H_MAX_RATE;
	vid_info->min_hfreq = H_MIN_RATE;
	vid_info->max_vfreq = V_MAX_RATE;
	vid_info->min_vfreq = V_MIN_RATE;
	vid_info->max_pixel_clock = (int)MAX_PIXEL_CLOCK;

	return 0;
}

/*
 * EDID Display Descriptors[7]
 * Bytes	Description
 * 0-1	0, indicates Display Descriptor (cf. Detailed Timing Descriptor).
 * 2	0, reserved
 * 3	Descriptor type. FA-FF currently defined. 00-0F reserved for vendors.
 * 4	0, reserved, except for Display Range Limits Descriptor.
 * 5-17	Defined by descriptor type. If text, code page 437 text,
 * terminated (if less than 13 bytes) with LF and padded with SP.
 */
int parse_monitor_name(struct dp_ctrl *dptx, uint8_t *blockname, uint32_t size)
{
	uint32_t i;
	uint32_t str_start = 5;
	uint32_t str_end = 0;
	uint8_t data_tmp = 0;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL!\n");
	dpu_check_and_return(!blockname, -EINVAL, err, "[DP] blockname is NULL!\n");
	dpu_check_and_return((size != DETAILED_TIMING_DESCRIPTION_SIZE), -EINVAL, err,
		"[DP] The length of monitor name is wrong!\n");

	for (i = str_start; i < (size - 1); i++) {
		if ((blockname[i] == 0x0A) && (blockname[i + 1] == 0x20)) {
			str_end = i;
			break;
		}
	}

	if (str_end == 0)
		str_end = size;

	if (str_end < str_start) {
		dpu_pr_err("[DP] The length of monitor name is wrong\n");
		return -EINVAL;
	}

	if (((str_end - str_start) < MONTIOR_NAME_DESCRIPTION_SIZE) &&
		(dptx->edid_info.video.dp_monitor_descriptor != NULL)) {
		memcpy(dptx->edid_info.video.dp_monitor_descriptor, &(blockname[str_start]), (str_end - str_start));
		dptx->edid_info.video.dp_monitor_descriptor[str_end - str_start] = '\0';

		/* For hiding custom device info */
		for (i = str_start; i < str_end; i++)
			data_tmp += blockname[i];

		memset(&blockname[str_start], 0x0, (str_end - str_start));
		/* For EDID checksum, we need reserve the sum for blocks. */
		blockname[str_start] = data_tmp;
	} else {
		dpu_pr_err("[DP] The length of monitor name is wrong\n");
		return -EINVAL;
	}

	return 0;
}

int release_edid_info(struct dp_ctrl *dptx)
{
	struct timing_info *dptx_timing_node = NULL;
	struct timing_info *_node_ = NULL;

	dpu_check_and_return(!dptx, -EINVAL, err, "[DP] dptx is NULL!\n");

	if (dptx->edid_info.video.dptx_timing_list != NULL) {
		list_for_each_entry_safe(dptx_timing_node, _node_, dptx->edid_info.video.dptx_timing_list, list_node) {
			list_del(&dptx_timing_node->list_node);
			kfree(dptx_timing_node);
		}

		kfree(dptx->edid_info.video.dptx_timing_list);
		dptx->edid_info.video.dptx_timing_list = NULL;
	}

	if (dptx->edid_info.video.ext_timing != NULL) {
		kfree(dptx->edid_info.video.ext_timing);
		dptx->edid_info.video.ext_timing = NULL;
	}

	if (dptx->edid_info.video.dp_monitor_descriptor != NULL) {
		kfree(dptx->edid_info.video.dp_monitor_descriptor);
		dptx->edid_info.video.dp_monitor_descriptor = NULL;
	}

	if (dptx->edid_info.audio.spec != NULL) {
		kfree(dptx->edid_info.audio.spec);
		dptx->edid_info.audio.spec = NULL;
	}

	memset(&(dptx->edid_info), 0, sizeof(struct edid_information));

	return 0;
}

struct vfp_data {
	uint32_t refresh_rate;
	uint16_t v_blanking;
	uint16_t v_sync_offset;
};

struct vary_vfp_data {
	uint8_t code;
	uint8_t valid_count;

	struct vfp_data data[2];
};

struct vary_vfp_data vary_vfp_tlb[] = {
	{  8, 1, {{59940, 22, 4}, {    0,  0, 0}} },
	{  9, 1, {{59940, 22, 4}, {    0,  0, 0}} },
	{ 12, 1, {{60054, 22, 4}, {    0,  0, 0}} },
	{ 13, 1, {{60054, 22, 4}, {    0,  0, 0}} },
	{ 23, 2, {{50080, 24, 2}, {49920, 25, 3}} },
	{ 24, 2, {{50080, 24, 2}, {49920, 25, 3}} },
	{ 27, 2, {{50080, 24, 2}, {49920, 25, 3}} },
	{ 28, 2, {{50080, 24, 2}, {49920, 25, 3}} }
};

static void vary_vfp(struct dtd *dtd_by_code, uint8_t code, uint32_t refresh_rate)
{
	uint8_t i, j;

	for (i = 0; i < ARRAY_SIZE(vary_vfp_tlb); i++) {
		if (code == vary_vfp_tlb[i].code) {
			/* valid_count must be 1 or 2, so < ARRAYSIZE(vary_vfp_tlb[i].data) */
			for (j = 0; j < vary_vfp_tlb[i].valid_count; j++) {
				if (refresh_rate == vary_vfp_tlb[i].data[j].refresh_rate) {
					dtd_by_code->v_blanking = vary_vfp_tlb[i].data[j].v_blanking;
					dtd_by_code->v_sync_offset = vary_vfp_tlb[i].data[j].v_sync_offset;
					return;
				}
			}
			if (j > 0) {
				dtd_by_code->v_blanking = vary_vfp_tlb[i].data[--j].v_blanking + 1;
				dtd_by_code->v_sync_offset = vary_vfp_tlb[i].data[--j].v_sync_offset + 1;
			}
		}
	}
}

bool convert_code_to_dtd(struct dtd *mdtd, uint8_t code, uint32_t refresh_rate, uint8_t video_format)
{
	struct dtd_info {
		uint8_t size;
		const struct dtd *support_modes_dtd;
	} support_dtd_info[] = {
		{ .size = (uint8_t)dtd_array_size(cea_modes_dtd), .support_modes_dtd = cea_modes_dtd },
		{ .size = (uint8_t)dtd_array_size(cvt_modes_dtd), .support_modes_dtd = cvt_modes_dtd },
		{ .size = (uint8_t)dtd_array_size(dmt_modes_dtd), .support_modes_dtd = dmt_modes_dtd }
	}, *handle_dtd = NULL;

	dpu_check_and_return(!mdtd, false, err, "[DP] mdtd is NULL!");
	memset(mdtd, 0, sizeof(*mdtd));

	if (video_format > DMT) {
		dpu_pr_info("[DP] video_format=%d is error", video_format);
		return false;
	}

	handle_dtd = &support_dtd_info[video_format];
	if (code == 0 || code >= handle_dtd->size) {
		dpu_pr_info("[DP] code error, code is %u", code);
		return false;
	}

	*mdtd = handle_dtd->support_modes_dtd[code];
	if (mdtd->pixel_clock == 0) {
		dpu_pr_info("[DP] Empty Timing");
		return false;
	}

	/*
	 * For certain VICs the spec allows the vertical
	 * front porch to vary by one or two lines.
	 */
	vary_vfp(mdtd, code, refresh_rate);

	return true;
}