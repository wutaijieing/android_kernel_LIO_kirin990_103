/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: header file for online api initial draft
 * Create: 2019-01-11
 */

#ifndef HISP350_MSG_ONLINE_API_H_INCLUDED
#define HISP350_MSG_ONLINE_API_H_INCLUDED

#include "hisp350_msg_common.h"

#define API_COUNT_FIRST 32
#define API_COUNT_SECOND 256
#define DMD_INFO_SIZE 32
#define EVENT_PARAMS_LEN 400

typedef struct _msg_req_query_capability_t {
	unsigned int cam_id;
	unsigned int csi_index;
	unsigned int i2c_index;
	unsigned int i2c_pad_type;
	char product_name[NAME_LEN];
	char sensor_name[NAME_LEN];
	unsigned int input_settings_buffer;
} msg_req_query_capability_t;

typedef struct _msg_ack_query_capability_t {
	unsigned int cam_id;
	char product_name[NAME_LEN];
	char sensor_name[NAME_LEN];
	unsigned int output_metadata_buffer;
	int status;
	int version;
} msg_ack_query_capability_t;

typedef struct _msg_req_release_i2c_t {
	unsigned int cam_id;
} msg_req_release_i2c_t;

typedef struct _msg_ack_release_i2c_t {
	unsigned int cam_id;
	int status;
} msg_ack_release_i2c_t;

typedef struct _msg_req_acquire_camera_t {
	unsigned int cam_id;
	unsigned int csi_index;
	unsigned int i2c_index;
	unsigned int i2c_pad_type;
	struct hisp_phy_info_t   phy_info;
	char sensor_name[NAME_LEN];
	char product_name[NAME_LEN];
	char extend_name[NAME_LEN];
	unsigned int input_otp_buffer;
	unsigned int input_calib_buffer;
	unsigned int buffer_size;
	unsigned int info_buffer;
	unsigned int info_count;
	unsigned int factory_calib_buffer;
	int ir_topology_type;
	unsigned int capacity;
} msg_req_acquire_camera_t;

typedef struct _msg_ack_acquire_camera_t {
	unsigned int cam_id;
	char sensor_name[NAME_LEN];
} msg_ack_acquire_camera_t;

typedef struct _msg_req_release_camera_t {
	unsigned int cam_id;
} msg_req_release_camera_t;

typedef struct _msg_ack_release_camera_t {
	unsigned int cam_id;
} msg_ack_release_camera_t;

typedef enum {
	SENSOR_NORMAL_BAYER = 0,
	SENSOR_NORMAL_BINNING = 1,

	SENSOR_QUADRA_2X2,
	SENSOR_QUADRA_2X2_REMOSAIC,
	SENSOR_QUADRA_2X2_BINNING_BAYER,

	SENSOR_QUADRA_4X4,
	SENSOR_QUADRA_4X4_REMOSAIC,
	SENSOR_QUADRA_4X4_BINNING_QUAD_2X2,
	SENSOR_QUADRA_4X4_BINNING_QUAD_2X2_REMOSAIC,
	SENSOR_QUADRA_4X4_BINNING_BAYER,
} cfa_mode_e; // color filter array mode

typedef enum {
	SENSOR_NORMAL_EXPO,
	SENSOR_QUADRA_HDR,
	SENSOR_STAGGER_HDR = 4,
} exposure_type_e;

typedef enum {
	SENSOR_FREQ_GEAR_0,
	SENSOR_FREQ_GEAR_1,
} freq_gear_type_e;

typedef enum {
	SENSOR_SIZE_PREFERENCE_DEFAULT,
	SENSOR_SIZE_PREFERENCE_SMALL,
	SENSOR_SIZE_PREFERENCE_LARGE,
	SENSOR_SIZE_PREFERENCE_FIXED,
} size_preference_e;

typedef enum {
	SENSOR_T_LINE_MODE_DEFAULT,
	SENSOR_T_LINE_MODE_1,
} t_line_mode_e;

typedef enum {
	SENSOR_VBLANK_MODE_DEFAULT,
	SENSOR_VBLANK_MODE_SHORT,
} vblank_mode_e;

typedef enum {
	SENSOR_GAIN_MODE_DEFAULT,
	SENSOR_GAIN_MODE_HIGH, // supernight
} gain_mode_e;

typedef struct _sensor_configuration_t {
	unsigned short fps; // adapt to max fps of the appropriate interval given by sensor driver

	unsigned char data_bit_depth;   // 10bits, 12bits, etc.
	unsigned char cfa_mode;

	unsigned char exposure_type;
	unsigned char crop_type;
	unsigned char mirror_mode;
	unsigned char reseverd;

	union {
		unsigned int extend_configs;
		struct {
			unsigned int freq_gear_mode : 1; // freq_gear_type_e
			unsigned int size_preference : 2; // size_preference_e
			unsigned int t_line_mode : 2; // t_line_mode_e, portait_mode
			unsigned int vblank_mode : 2; // vblank_mode_e
			unsigned int gain_mode : 2; // gain_mode_e
			unsigned int reseverdbits : 23;
		};
	};
} sensor_configuration_t;

typedef enum {
	ISP_SEAMLESS_OFF,
	ISP_SEAMLESS_ON,
} seamless_mode_e;

typedef enum {
	ISP_STATISTICS_OFF,
	ISP_STATISTICS_ON,
} hdr_statistics_mode_e;

typedef enum {
	ISP_PHOTO_MODE,
	ISP_VIDEO_MODE,
} scheduling_mode_e;

typedef struct _isp_configuration_t {
	unsigned short min_fps;
	unsigned short max_fps;

	unsigned char seamless_mode;
	unsigned char hdr_statistics_mode;
	unsigned char scheduling_mode;
	unsigned char reserved;
} isp_configuration_t;

typedef struct _stream_config_t {
	unsigned short type;
	unsigned short width;
	unsigned short height;
	unsigned short stride;
	unsigned short format;
	unsigned short secure;
} stream_config_t;

typedef struct _msg_req_usecase_config_t {
	unsigned int cam_id;
	unsigned int extension;
	unsigned int stream_nr;
	stream_config_t stream_cfg[MAX_STREAM_NUM];
	char time[API_COUNT_FIRST];
	sensor_configuration_t sensor_cfg;
	isp_configuration_t isp_cfg;
} msg_req_usecase_config_t;

typedef struct _msg_ack_usecase_config_t {
	unsigned int cam_id;
	int status;
	unsigned int sensor_width;
	unsigned int sensor_height;
	unsigned int sensor_pixel_width;
	unsigned int sensor_pixel_height;
} msg_ack_usecase_config_t;

typedef struct _msg_req_pq_setting_config_t {
	unsigned int cam_id;
	unsigned int mode;
	char scene[API_COUNT_SECOND];
	unsigned int pq_setting;
	unsigned int pq_setting_size;
} msg_req_pq_setting_config_t;

typedef struct _msg_ack_pq_setting_config_t {
	unsigned int cam_id;
	unsigned int mode;
	unsigned int pq_setting;
	unsigned int pq_setting_size;
	int status;
} msg_ack_pq_setting_config_t;

typedef struct _msg_req_stream_on_t {
	unsigned int cam_id;
} msg_req_stream_on_t;

typedef struct _msg_req_stream_off_t {
	unsigned int cam_id;
	unsigned int is_hotplug;
} msg_req_stream_off_t;

typedef struct _msg_ack_stream_on_t {
	unsigned int cam_id;
	int status;
} msg_ack_stream_on_t;

typedef struct _msg_ack_stream_off_t {
	unsigned int cam_id;
	int status;
} msg_ack_stream_off_t;

typedef struct _msg_req_get_otp_t {
	unsigned int cam_id;
	char sensor_name[NAME_LEN];
	unsigned int input_otp_buffer;
	unsigned int buffer_size;
} msg_req_get_otp_t;

typedef struct _msg_ack_get_otp_t {
	unsigned int cam_id;
	char sensor_name[NAME_LEN];
	unsigned int output_otp_buffer;
	unsigned int buffer_size;
	int status;
} msg_ack_get_otp_t;

typedef struct _msg_req_request_t {
	unsigned int cam_id;
	unsigned int num_targets;
	unsigned int target_map;
	unsigned int frame_number;
	unsigned int buf[MAX_STREAM_NUM];
	unsigned int input_setting_buffer;
	unsigned int output_metadata_buffer;
	unsigned int output_cap_info_buffer;
	unsigned int input_cfgtab_buffer;
	unsigned int output_cfgtab_buffer;
	unsigned int request_mode; // 0:invalid, 1<<0:binning request, 1<<2:quad request
	unsigned int extra_buf_offset[MAX_STREAM_NUM];
} msg_req_request_t;

typedef struct _msg_ack_request_t {
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
	unsigned int output_cap_info_buffer;
} msg_ack_request_t;

typedef struct _msg_batch_req_request_t {
	unsigned int cam_id;
	unsigned int count;
	unsigned int batch_frame_num;
	unsigned int requests_buf;
	unsigned int requests_buf_size;
} msg_batch_req_request_t;

typedef struct _msg_batch_ack_request_t {
	unsigned int cam_id;
	unsigned int count;
	unsigned int batch_frame_num;
	unsigned int acks_buf;
	unsigned int acks_buf_size;
	unsigned int timestampL;
	unsigned int timestampH;
	unsigned int system_couter_rate;
} msg_batch_ack_request_t;

typedef struct _msg_req_fbdraw_request_t {
	unsigned int  cam_id;
	unsigned int  frame_number;
	stream_info_t input_stream_info;
	unsigned int input_raw_type; // raw_type_e
	stream_info_t output_stream_info;
	unsigned int output_raw_type; // raw_type_e
	unsigned int input_setting_buffer;
	unsigned int crop_enable; // 0: disable 1: enable
	isp_crop_region_info_t crop_region;
} msg_req_fbdraw_request_t;

typedef struct _msg_ack_fbdraw_request_t {
	unsigned int  cam_id;
	unsigned int  frame_number;
	unsigned int  status;
	stream_info_t output_stream_info;
} msg_ack_fbdraw_request_t;

typedef struct _msg_req_map_buffer_t {
	unsigned int cam_id;
	unsigned int pool_count;
	map_pool_desc_t map_pool[MAP_POOL_USAGE_MAX];
} msg_req_map_buffer_t;

typedef struct _msg_ack_map_buffer_t {
	unsigned int cam_id;
	int status;
} msg_ack_map_buffer_t;

typedef struct _msg_req_unmap_buffer_t {
	unsigned int cam_id;
	unsigned int buffer;
} msg_req_unmap_buffer_t;

typedef struct _msg_ack_unmap_buffer_t {
	unsigned int cam_id;
	int status;
} msg_ack_unmap_buffer_t;

typedef struct _map_resolution_t {
	unsigned int width;
	unsigned int height;
} map_pipe_resolution_t;

typedef enum _map_pool_resolution_e {
	MAP_POOL_RESOLUTION_RAW = 0,
	MAP_POOL_RESOLUTION_YUV,
	MAP_POOL_RESOLUTION_MAX
} map_pool_resolution_e;

typedef struct _msg_req_dynamic_map_buffer_t {
	unsigned int cam_id;
	unsigned int pool_count;
	map_pipe_resolution_t pipe_resolution[MAP_POOL_RESOLUTION_MAX];
	map_pool_desc_t map_pool[MAP_POOL_USAGE_MAX];
} msg_req_dynamic_map_buffer_t;

typedef struct _msg_ack_dynamic_map_buffer_t {
	unsigned int cam_id;
	int status;
} msg_ack_dynamic_map_buffer_t;

typedef struct _msg_req_dynamic_unmap_buffer_t {
	unsigned int cam_id;
	unsigned int buffer;
} msg_req_dynamic_unmap_buffer_t;

typedef struct _msg_ack_dynamic_unmap_buffer_t {
	unsigned int cam_id;
	int status;
} msg_ack_dynamic_unmap_buffer_t;

typedef struct _msg_req_cal_data_t {
	unsigned int cam_id;
	unsigned int buffer_size;
	unsigned int cal_data_buffer;
} msg_req_cal_data_t;

typedef struct _msg_ack_cal_data_t {
	unsigned int cam_id;
	int status;
} msg_ack_cal_data_t;

typedef struct _msg_req_flush_t {
	unsigned int cam_id;
	unsigned int is_hotplug;
} msg_req_flush_t;

typedef struct _msg_ack_flush_t {
	int status;
} msg_ack_flush_t;

typedef struct _msg_req_inv_tlb_t {
	int flag;
} msg_req_inv_tlb_t;

typedef struct _msg_ack_inv_tlb_t {
	int status;
} msg_ack_inv_tlb_t;

typedef struct _msg_req_mem_pool_init_t {
	unsigned int mempool_addr;
	unsigned int mempool_size;
	unsigned int mempool_prot;
}msg_req_mem_pool_init_t;

typedef struct _msg_ack_mem_pool_init_t {
	unsigned int status;
}msg_ack_mem_pool_init_t;

typedef struct _msg_req_mem_pool_deinit_t {
	unsigned int mempool_addr;
	unsigned int mempool_size;
	unsigned int mempool_prot;
}msg_req_mem_pool_deinit_t;

typedef struct _msg_ack_mem_pool_deinit_t {
	unsigned int status;
}msg_ack_mem_pool_deinit_t;

typedef struct _msg_req_isp_cpu_poweroff_t {
	int flag;
} msg_req_isp_cpu_poweroff_t;

typedef struct _msg_ack_isp_cpu_poweroff_t {
	int status;
} msg_ack_isp_cpu_poweroff_t;

typedef enum {
	MOTION_SENSOR_ACCEL = 1,
	MOTION_SENSOR_GYRO = 4,
	MOTION_SENSOR_LINEAR_ACCEL = 10,
} motion_sensor_type_t;

typedef enum {
	DEMOSAIC_ONLINE = 0,
	DEMOSAIC_OFFLINE,
	REMOSAIC_OFFLINE,
	REQUEST_RAW2YUV_MAX,
} raw2yuv_req_mode_e;

typedef enum {
	EVENT_ERR_CODE = 0,
	EVENT_SHUTTER,
	EVENT_INTERRUPT,
	EVENT_FLASH,
	EVENT_AF,
	EVENT_AF_MMI_DEBUG,
	EVENT_AF_DIRECT_TRANS_BASE,
	EVENT_AF_OTP_CALIB_DATA,
	EVENT_AF_SELF_LEARN_DATA,
	EVENT_AF_STAT_INFO,
} event_info_e;

typedef struct _msg_event_sent_t {
	unsigned int cam_id;
	event_info_e event_id;
	unsigned int frame_number;
	unsigned int stream_id;
	unsigned int timestampL;
	unsigned int timestampH;
	char event_params[EVENT_PARAMS_LEN];
} msg_event_sent_t;

typedef struct _msg_req_connect_camera_t {
	unsigned int cam_id;
	unsigned int csi_index;
	unsigned int i2c_index;
	struct hisp_phy_info_t   phy_info;
} msg_req_connect_camera_t;

typedef struct _msg_ack_connect_camera_t {
	unsigned int cam_id;
	int status;
} msg_ack_connect_camera_t;

#endif
/* HISP350_MSG_ONLINE_API_H_INCLUDED */
