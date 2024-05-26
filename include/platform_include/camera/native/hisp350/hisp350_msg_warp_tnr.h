/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: header file for warp tnr initial draft
 * Create: 2019-01-11
 */

#ifndef HISP350_MSG_WARP_TNR_H_INCLUDED
#define HISP350_MSG_WARP_TNR_H_INCLUDED

#include "hisp350_msg_common.h"

#define WARP_COUNT_FIRST 2
#define WARP_COUNT_SECOND 3
#define WARP_COUNT_THIRD 64
#define YNR_NUM 53

typedef enum {
	MODE_EIS_PRE = 0,
	MODE_EIS_VID,
	MODE_EIS_MULTI,
	MODE_DMAP,
	MODE_EIS_4K_60FPS,
	MODE_MAX,
} warp_request_mode_e;

typedef struct warp_output_info {
	unsigned int		isHFBC;
	stream_info_t	   output_info;
} warp_output_info_t;

typedef struct eis_tnr_info {
	unsigned int ae_gain;
	int lv;
	unsigned int chromatix_addr;
	unsigned int mv_buffer_addr;
	unsigned int lf_mv_buf_offset;
	unsigned int global_offset_dx;
	unsigned int global_offset_dy;
	unsigned int prev_buffer_addr;
	unsigned int curr_buffer_addr;
	isp_crop_region_info_t prev_crop_region;
	isp_crop_region_info_t curr_crop_region;
	stream_info_t prev_stream;
	stream_info_t cur_stream;
	unsigned int status;
	unsigned int chromatixVaild;
	unsigned short lsc_gain_53_to_tnr[YNR_NUM];
	unsigned int cgrid_info_buffer_lf;
	unsigned int tnr_mode;
	unsigned int extra_buf_offset;
	unsigned int stream_buf_offset;
	unsigned int prev_statae_addr;
	unsigned int curr_statae_addr;
} eis_tnr_info_t;

typedef enum {
	WARP_OUTPUT_DEFAULT = 0,
	WARP_OUTPUT_ARSR_ENABLE,
	WARP_OUTPUT_ARSR_DISABLE,
	WARP_OUTPUT_MAX,
} warp_output_module_e;

typedef struct _msg_req_warp_request_t {
	unsigned int cam_id;
	unsigned int frame_number;
	stream_info_t input_stream_info;
	stream_info_t warp_output_stream_info;
	unsigned int grid_enable;
	unsigned int grid_order;
	unsigned int cgrid_info_buffer;
	warp_request_mode_e mode;
	warp_output_info_t output_stream_info[WARP_COUNT_FIRST];
	eis_tnr_info_t tnr_info;
	unsigned int aip_enable;
	warp_output_module_e arsr_ctrl_mode;
} msg_req_warp_request_t;

typedef struct _msg_ack_warp_request_t {
	unsigned int cam_id;
	unsigned int frame_number;
	stream_info_t input_stream_info;
	unsigned int status;
	warp_request_mode_e mode;
	warp_output_info_t output_stream_info[WARP_COUNT_FIRST];
} msg_ack_warp_request_t;

#endif /* HISP350_MSG_WARP_TNR_H_INCLUDED */
