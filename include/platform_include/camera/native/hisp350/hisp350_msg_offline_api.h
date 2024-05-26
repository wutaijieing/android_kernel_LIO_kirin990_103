/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: header file for offline api initial draft
 * Create: 2019-04-11
 */

#ifndef HISP350_MSG_OFFLINE_API_H_INCLUDED
#define HISP350_MSG_OFFLINE_API_H_INCLUDED

#include "hisp350_msg_common.h"

typedef struct _msg_req_request_offline_t {
	unsigned int cam_id;
	unsigned int num_targets;
	unsigned int target_map;
	unsigned int frame_number;
	unsigned int buf[MAX_STREAM_NUM];
	unsigned int input_setting_buffer;
	unsigned int output_metadata_buffer;
} msg_req_request_offline_t;

typedef struct _msg_ack_request_offline_t {
	unsigned int cam_id;
	unsigned int num_targets;
	unsigned int target_map;
	unsigned int frame_number;
	stream_info_t stream_info[MAX_STREAM_NUM];
	unsigned int input_setting_buffer;
	unsigned int output_metadata_buffer;
	unsigned int timestampL;
	unsigned int timestampH;
	unsigned int status;
} msg_ack_request_offline_t;

typedef struct _msg_req_request_offline_raw2rgb_t {
	unsigned int cam_id;
	unsigned int num_targets;
	unsigned int target_map;
	unsigned int frame_number;
	unsigned int buf[MAX_STREAM_NUM];
	unsigned int input_setting_buffer;
	unsigned int output_metadata_buffer;
} msg_req_request_offline_raw2rgb_t;

typedef struct _msg_req_request_offline_rgb2yuv_t {
	unsigned int cam_id;
	unsigned int num_targets;
	unsigned int target_map;
	unsigned int frame_number;
	unsigned int buf[MAX_STREAM_NUM];
	unsigned int input_setting_buffer;
	unsigned int output_metadata_buffer;
} msg_req_request_offline_rgb2yuv_t;

typedef struct _msg_ack_raw_proc_request_t {
	unsigned int cam_id;
	unsigned int num_targets;
	unsigned int target_map;
	unsigned int frame_number;
	stream_info_t stream_info[MAX_STREAM_NUM];
	unsigned int input_setting_buffer;
	unsigned int output_metadata_buffer;
	unsigned int timestampL;
	unsigned int timestampH;
	unsigned int status;
} msg_ack_raw_proc_request_t;

typedef struct _msg_req_map_buffer_offline_t {
	unsigned int cam_id;
	unsigned int pool_count;
	map_pool_desc_t map_pool[MAP_POOL_USAGE_MAX];
} msg_req_map_buffer_offline_t;

typedef struct _msg_ack_map_buffer_offline_t {
	unsigned int cam_id;
	int status;
} msg_ack_map_buffer_offline_t;

typedef struct _msg_req_raw_proc_map_buffer_t {
	unsigned int cam_id;
	unsigned int pool_count;
	map_pool_desc_t map_pool[MAP_POOL_USAGE_MAX];
} msg_req_raw_proc_map_buffer_t;

typedef struct _msg_ack_raw_proc_map_buffer_t {
	unsigned int cam_id;
	int status;
} msg_ack_raw_proc_map_buffer_t;

typedef struct _msg_req_unmap_buffer_offline_t {
	unsigned int cam_id;
	unsigned int buffer;
} msg_req_unmap_buffer_offline_t;

typedef struct _msg_ack_unmap_buffer_offline_t {
	unsigned int cam_id;
	int status;
} msg_ack_unmap_buffer_offline_t;

typedef struct _msg_req_raw_proc_unmap_buffer_t {
	unsigned int cam_id;
	unsigned int buffer;
} msg_req_raw_proc_unmap_buffer_t;

typedef struct _msg_ack_raw_proc_unmap_buffer_t {
	unsigned int cam_id;
	int status;
} msg_ack_raw_proc_unmap_buffer_t;

typedef struct _msg_req_raw2yuv_start_t {
	unsigned int cam_id;
	unsigned int csi_index;
	unsigned int i2c_index;
	char		 sensor_name[NAME_LEN];
	char		 product_name[NAME_LEN];
	unsigned int input_calib_buffer;
	raw2yuv_req_mode_e raw2yuv_mode;
} msg_req_raw2yuv_start_t;

typedef struct _msg_ack_raw2yuv_start_t {
	unsigned int cam_id;
} msg_ack_raw2yuv_start_t;

typedef struct _msg_req_raw2yuv_stop_t {
	unsigned int cam_id;
} msg_req_raw2yuv_stop_t;

typedef struct _msg_ack_raw2yuv_stop_t {
	unsigned int cam_id;
} msg_ack_raw2yuv_stop_t;

typedef struct _msg_req_raw_proc_start_t {
	unsigned int cam_id;
	unsigned int csi_index;
	unsigned int i2c_index;
	char		 sensor_name[NAME_LEN];
	char		 product_name[NAME_LEN];
	unsigned int input_calib_buffer;
} msg_req_raw_proc_start_t;

typedef struct _msg_ack_raw_proc_start_t {
	unsigned int cam_id;
} msg_ack_raw_proc_start_t;

typedef struct _msg_req_raw_proc_stop_t {
	unsigned int cam_id;
} msg_req_raw_proc_stop_t;

typedef struct _msg_ack_raw_proc_stop_t {
	unsigned int cam_id;
} msg_ack_raw_proc_stop_t;

#endif /* HISP350_MSG_OFFLINE_API_H_INCLUDED */
