/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: header file for dmap initial draft
 * Create: 2021-04-11
 */

#ifndef HISP350_MSG_DMAP_H_INCLUDED
#define HISP350_MSG_DMAP_H_INCLUDED

typedef enum _map_pool_usage_e {
	MAP_POOL_USAGE_FW = 0,
	MAP_POOL_USAGE_ISP_FW,
	MAP_POOL_USAGE_ISP,
	MAP_POOL_USAGE_MAX,
} map_pool_usage_e;

typedef enum {
	DMAP_DGEN_DISP,
	DMAP_DGEN_CONF,
	DMAP_LEFT_VERT,
	DMAP_DOPT_HORZ_DISP,
	DMAP_DOPT_VERT_DISP,
	DMAP_DGEN_DEPTH,
	DMAP_DOPT_DEPTH,
	DMAP_DOPT_XMAP,
	DMAP_DOPT_YMAP,
	DMAP_DOPT_CONF,
	DMAP_DOPT_SPARSE_DISP,
	DMAP_DOPT_SPARSE_DEPTH,
	DMAP_DGEN_LEFT_DS,
	DMAP_DGEN_RIGHT_DS,
	DMAP_DGEN_STAT,
	DMAP_MAX_OUTPUT,
} dmap_output_e;

typedef struct _map_pool_desc_t {
	unsigned int start_addr;
	unsigned int ion_iova;
	unsigned int size;
	unsigned int usage;
} map_pool_desc_t;

typedef struct _dmap_output_info_t {
	unsigned int width;
	unsigned int height;
	unsigned int stride;
} dmap_output_info_t;

typedef struct _msg_req_dmap_format_request_t {
	unsigned int dgen_output_bit;
	unsigned int dopt_output_bit;
	dmap_output_info_t output_info[DMAP_MAX_OUTPUT];
	unsigned int direction;
	unsigned int expansion;
	unsigned int dgen_input_format;
	unsigned int dopt_input_format;
	unsigned int req_switch_mask;
} msg_req_dmap_format_t;

typedef struct _msg_ack_dmap_format_request_t {
	unsigned int status;
} msg_ack_dmap_format_t;

typedef struct _msg_req_dgen_request_t {
	unsigned int base_img;
	unsigned int frame_number;
	unsigned int req_type;
	unsigned int disp_direction;
	unsigned int dmap_crop_x;
	unsigned int dmap_crop_y;
	unsigned int dmap_crop_width;
	unsigned int dmap_crop_height;
	unsigned int input_left_buffer;
	unsigned int input_right_buffer;
	unsigned int output_left_raster;
	unsigned int output_disp_raster;
	unsigned int output_conf_raster;
	unsigned int output_disp_fw_addr;
	unsigned int output_conf_fw_addr;
} msg_req_dgen_request_t;

typedef struct _msg_ack_dgen_request_t {
	unsigned int output_disp_buffer;
	unsigned int output_conf_buffer;
	unsigned int output_left_raster;
	unsigned int input_left_buffer;
	unsigned int input_right_buffer;
	unsigned int base_img;
	unsigned int frame_number;
	unsigned int req_type;
	unsigned int out_stride;
	unsigned int timestampL;
	unsigned int timestampH;
	unsigned int status;
} msg_ack_dgen_request_t;

typedef struct _msg_req_dopt_request_t {
	unsigned int base_img;
	unsigned int frame_number;
	unsigned int req_type;
	unsigned int disp_direction;
	unsigned int dmap_crop_x;
	unsigned int dmap_crop_y;
	unsigned int dmap_crop_width;
	unsigned int dmap_crop_height;
	unsigned int input_horz_left_image;
	unsigned int input_vert_left_image;
	unsigned int input_raster_disp_buffer;
	unsigned int input_last_left_buffer;
	unsigned int input_last_disp_buffer;
	unsigned int output_disp_buffer;
} msg_req_dopt_request_t;

typedef struct _msg_ack_dopt_request_t {
	unsigned int output_disp_buffer;
	unsigned int input_horz_left_image;
	unsigned int input_vert_left_image;
	unsigned int input_raster_disp_buffer;
	unsigned int input_last_left_buffer;
	unsigned int input_last_disp_buffer;
	unsigned int base_img;
	unsigned int frame_number;
	unsigned int req_type;
	unsigned int out_stride;
	unsigned int timestampL;
	unsigned int timestampH;
	unsigned int status;
} msg_ack_dopt_request_t;

typedef struct _msg_req_drbr_request_t {
	unsigned int base_img;
	unsigned int frame_number;
	unsigned int req_type;
	unsigned int dmap_crop_x;
	unsigned int dmap_crop_y;
	unsigned int dmap_crop_width;
	unsigned int dmap_crop_height;
	unsigned int input_buffer;
	unsigned int output_buffer;
	unsigned int image_width;
	unsigned int image_height;
	unsigned int data_type;
	unsigned int mode;
	unsigned int read_flip;
	unsigned int write_flip;
	unsigned int rotation;
	unsigned int rub_dist;
	unsigned int b2r_expansion;
} msg_req_drbr_request_t;

typedef struct _msg_ack_drbr_request_t {
	unsigned int frame_number;
	unsigned int req_type;
	unsigned int status;
	unsigned int timestampL;
	unsigned int timestampH;
	unsigned int image_width;
	unsigned int image_height;
	unsigned int output_buffer;
	unsigned int bit_num;
	unsigned int rotation;
	unsigned int read_flip;
	unsigned int write_flip;
	unsigned int mode;
} msg_ack_drbr_request_t;

typedef struct _msg_req_dmap_request_t {
	unsigned int base_img;
	unsigned int frame_number;
	unsigned int req_type;
	unsigned int disp_direction;
	unsigned int sparse_enable;
	unsigned int dense_enable;
	unsigned int warp_enable;
	unsigned int rotation_type;
	unsigned int rotation_direction;
	unsigned int dmap_crop_x;
	unsigned int dmap_crop_y;
	unsigned int dmap_crop_width;
	unsigned int dmap_crop_height;
	unsigned int input_dgen_left;
	unsigned int input_dgen_right;
	unsigned int output_dopt_left_ds;
	unsigned int output_dopt_right_ds;
	unsigned int output_dopt_disp_ds;
	unsigned int output_block_disp;
	unsigned int output_hist_stat_addr;
	unsigned int output_sparse_disp;
	unsigned int output_conf_raster;
	unsigned int output_conf_temp;
	unsigned int output_sparse_depth;
	unsigned int output_disp_fw_addr;
	unsigned int output_conf_fw_addr;
	unsigned int input_dopt_left_ds;
	unsigned int input_dopt_right_ds;
	unsigned int input_dopt_disp_ds;
	unsigned int input_block_disp;
	unsigned int input_hist_stat_addr;
	unsigned int input_dopt_horz_left;
	unsigned int input_last_left_buffer;
	unsigned int input_last_disp_buffer;
	unsigned int output_dense_horz_disp;
	unsigned int output_dense_depth;
	unsigned int output_dense_xmap;
	unsigned int output_dense_ymap;
	unsigned int input_warp_sparse_setting_buffer;
	unsigned int input_warp_dense_setting_buffer;
} msg_req_dmap_request_t;

typedef struct _msg_ack_dmap_request_t {
	unsigned int output_sparse_disp;
	unsigned int output_conf_raster;
	unsigned int output_conf_temp;
	unsigned int output_sparse_depth;
	unsigned int input_dgen_left;
	unsigned int input_dgen_right;
	unsigned int output_dense_horz_disp;
	unsigned int output_dense_depth;
	unsigned int output_dense_xmap;
	unsigned int output_dense_ymap;
	unsigned int input_dopt_horz_left;
	unsigned int input_last_left_buffer;
	unsigned int input_last_disp_buffer;
	unsigned int input_warp_sparse_setting_buffer;
	unsigned int input_warp_dense_setting_buffer;
	unsigned int input_dopt_left_ds;
	unsigned int input_dopt_right_ds;
	unsigned int input_dopt_disp_ds;
	unsigned int input_block_disp;
	unsigned int input_hist_stat_addr;
	unsigned int output_dopt_left_ds;
	unsigned int output_dopt_right_ds;
	unsigned int output_dopt_disp_ds;
	unsigned int output_block_disp;
	unsigned int output_hist_stat_addr;
	unsigned int base_img;
	unsigned int frame_number;
	unsigned int req_type;
	unsigned int out_stride;
	unsigned int timestampL;
	unsigned int timestampH;
	unsigned int warp_flag;
	unsigned int status;
} msg_ack_dmap_request_t;

typedef struct _msg_req_dmap_map_t {
	unsigned int width;
	unsigned int height;
	unsigned int mode;
	map_pool_desc_t map_pool[MAP_POOL_USAGE_MAX];
} msg_req_dmap_map_t;

typedef struct _msg_ack_dmap_map_t {
	unsigned int status;
} msg_ack_dmap_map_t;

typedef struct _msg_req_dmap_unmap_t {
	unsigned int unmap_cfg_addr;
	unsigned int unmap_buf_addr;
} msg_req_dmap_unmap_t;

typedef struct _msg_ack_dmap_unmap_t {
	unsigned int status;
} msg_ack_dmap_unmap_t;

typedef struct _msg_req_dmap_offline_map_t {
	unsigned int width;
	unsigned int height;
	unsigned int mode;
	map_pool_desc_t map_pool[MAP_POOL_USAGE_MAX];
} msg_req_dmap_offline_map_t;

typedef struct _msg_ack_dmap_offline_map_t {
	unsigned int status;
} msg_ack_dmap_offline_map_t;

typedef struct _msg_req_dmap_offline_unmap_t {
	unsigned int unmap_isp_fw_addr;
	unsigned int unmap_buf_addr;
} msg_req_dmap_offline_unmap_t;

typedef struct _msg_ack_dmap_offline_unmap_t {
	unsigned int status;
} msg_ack_dmap_offline_unmap_t;

typedef struct _msg_req_dmap_flush_t {
	int flag;
} msg_req_dmap_flush_t;

typedef struct _msg_ack_dmap_flush_t {
	unsigned int status;
} msg_ack_dmap_flush_t;

#endif /* HISP350_MSG_DMAP_H_INCLUDED */
