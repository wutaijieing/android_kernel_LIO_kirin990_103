/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: header file for msg initial draft
 * Create: 2020-04-10
 */

#ifndef HISP350_MSG_H_INCLUDED
#define HISP350_MSG_H_INCLUDED

#include <platform_include/isp/linux/hisp_remoteproc.h>
#include "hisp350/hisp350_msg_common.h"
#include "hisp350/hisp350_msg_dmap.h"
#include "hisp350/hisp350_msg_subcmd.h"
#include "hisp350/hisp350_msg_online_api.h"
#include "hisp350/hisp350_msg_offline_api.h"
#include "hisp350/hisp350_msg_warp_tnr.h"
#include "hisp350/hisp350_msg_peridevice.h"
#include "hisp350/hisp350_msg_ispnn_api.h"

struct hi_list_head {
	struct hi_list_head *next, *prev;
};

typedef enum {
	/* Request items. */
	COMMAND_QUERY_CAPABILITY = 0x1000,
	COMMAND_ACQUIRE_CAMERA,
	COMMAND_RELEASE_CAMERA,
	COMMAND_USECASE_CONFIG,
	COMMAND_GET_OTP,
	COMMAND_REQUEST,
	COMMAND_MAP_BUFFER,
	COMMAND_UNMAP_BUFFER,
	COMMAND_CALIBRATION_DATA,
	COMMAND_TEST_CASE_INTERFACE,
	COMMAND_FLUSH,
	COMMAND_EXTEND_SET,
	COMMAND_EXTEND_GET,
	COMMAND_INV_TLB,
	COMMAND_QUERY_OIS_UPDATE,
	COMMAND_OIS_UPDATE,
	COMMAND_QUERY_LASER,
	COMMAND_ACQUIRE_LASER,
	COMMAND_RELEASE_LASER,
	COMMAND_STREAM_ON,
	COMMAND_STREAM_OFF,
	COMMAND_WARP_REQUEST,
	COMMAND_ARSR_REQUEST,
	COMMAND_DMAP_MAP_REQUEST,
	COMMAND_DMAP_UNMAP_REQUEST,
	COMMAND_DMAP_FORMAT_REQUEST,
	COMMAND_DMAP_REQUEST,
	COMMAND_DMAP_FLUSH_REQUEST,
	COMMAND_MEM_POOL_INIT_REQUEST,
	COMMAND_MEM_POOL_DEINIT_REQUEST,
	COMMAND_ISP_CPU_POWER_OFF_REQUEST,
	COMMAND_DYNAMIC_MAP_BUFFER,
	COMMAND_DYNAMIC_UNMAP_BUFFER,
	COMMAND_RAW2YUV_MAP_BUFFER,
	COMMAND_RAW2YUV_START,
	COMMAND_RAW2YUV_REQUEST,
	COMMAND_RAW2RGB_REQUEST,
	COMMAND_RGB2YUV_REQUEST,
	COMMAND_RAW2YUV_STOP,
	COMMAND_RAW2YUV_UNMAP_BUFFER,
	/* end add */
	COMMAND_DMAP_OFFLINE_MAP_REQUEST,
	COMMAND_DMAP_OFFLINE_UNMAP_REQUEST,

	COMMAND_PQ_SETTING_CONFIG,
	COMMAND_FBDRAW_REQUEST,
	COMMAND_DEVICE_CTL,

	COMMAND_RELEASE_I2C,
	COMMAND_BATCH_REQUEST,
	COMMAND_CONNECT_CAMERA,

	/* req for ispnn */
	COMMAND_CREATE_ISPNN_MODEL,
	COMMAND_EXTEND_ISPNN_BUFFER,
	COMMAND_ENABLE_ISPNN_MODEL,
	COMMAND_DISABLE_ISPNN_MODEL,
	COMMAND_DESTROY_ISPNN_MODEL,

	/* Response items. */
	QUERY_CAPABILITY_RESPONSE = 0x2000,
	ACQUIRE_CAMERA_RESPONSE,
	RELEASE_CAMERA_RESPONSE,
	USECASE_CONFIG_RESPONSE,
	GET_OTP_RESPONSE,
	REQUEST_RESPONSE,
	MAP_BUFFER_RESPONSE,
	UNMAP_BUFFER_RESPONSE,
	CALIBRATION_DATA_RESPONSE,
	TEST_CASE_RESPONSE,
	FLUSH_RESPONSE,
	EXTEND_SET_RESPONSE,
	EXTEND_GET_RESPONSE,
	INV_TLB_RESPONSE,
	QUERY_OIS_UPDATE_RESPONSE,
	OIS_UPDATE_RESPONSE,
	QUERY_LASER_RESPONSE,
	ACQUIRE_LASER_RESPONSE,
	RELEASE_LASER_RESPONSE,
	STREAM_ON_RESPONSE,
	STREAM_OFF_RESPONSE,
	WARP_REQUEST_RESPONSE,
	ARSR_REQUEST_RESPONSE,
	DMAP_MAP_RESPONSE,
	DMAP_UNMAP_RESPONSE,
	DMAP_FORMAT_RESPONSE,
	DMAP_REQUEST_RESPONSE,
	DMAP_FLUSH_RESPONSE,
	MEM_POOL_INIT_RESPONSE,
	MEM_POOL_DEINIT_RESPONSE,
	ISP_CPU_POWER_OFF_RESPONSE,
	DYNAMIC_MAP_BUFFER_RESPONSE,
	DYNAMIC_UNMAP_BUFFER_RESPONSE,
	RAW2YUV_MAP_BUFFER_RESPONSE,
	RAW2YUV_START_RESPONSE,
	RAW2YUV_REQUEST_RESPONSE,
	RAW2RGB_REQUEST_RESPONSE,
	RGB2YUV_REQUEST_RESPONSE,
	RAW2YUV_STOP_RESPONSE,
	RAW2YUV_UNMAP_BUFFER_RESPONSE,
	/* for rawmfnr bm3d raw processing */
	/* end add */
	DMAP_OFFLINE_MAP_RESPONSE,
	DMAP_OFFLINE_UNMAP_RESPONSE,

	PQ_SETTING_CONFIG_RESPONSE,
	FBDRAW_REQUEST_RESPONSE,
	DEVICE_CTL_RESPONSE,
	RELEASE_I2C_RESPONSE,
	BATCH_REQUEST_RESPONSE,
	CONNECT_CAMERA_RESPONCE,

	/* responce for ispnn */
	CREATE_ISPNN_MODEL_RESPONCE,
	EXTEND_ISPNN_BUFFER_RESPONCE,
	ENABLE_ISPNN_MODEL_RESPONCE,
	DISABLE_ISPNN_MODEL_RESPONCE,
	DESTROY_ISPNN_MODEL_RESPONCE,
	COMMAND_DRBR_REQUEST, // delete later
	DRBR_REQUEST_RESPONSE, // delete later

	COMMAND_IPP_SRC_REQUEST,
	IPP_SRC_RESPONSE,

	/* Event items sent to AP. */
	MSG_EVENT_SENT = 0x3000,
} api_id_e;

struct msg_base;

typedef void (*msg_looper_handler)(struct msg_base*, void*);

typedef struct msg_base {
	struct hi_list_head link;
	msg_looper_handler handler;
	void* user;
} msg_base_t;

typedef struct _isp_msg_t {
	unsigned int message_size;
	unsigned int api_name;
	unsigned int message_id;
	unsigned int message_hash;
	union {
		/* Request items. */
		msg_req_query_capability_t req_query_capability;
		msg_req_acquire_camera_t req_acquire_camera;
		msg_req_release_camera_t req_release_camera;
		msg_req_usecase_config_t req_usecase_config;
		msg_req_stream_on_t req_stream_on;
		msg_req_stream_off_t req_stream_off;
		msg_req_get_otp_t req_get_otp;
		msg_req_request_t req_request;
		msg_req_warp_request_t req_warp_request;
		msg_req_dmap_format_t req_dmap_format;
		msg_req_dmap_request_t req_dmap_request;
		msg_req_dgen_request_t req_dgen_request;
		msg_req_dopt_request_t req_dopt_request;
		msg_req_map_buffer_t req_map_buffer;
		msg_req_unmap_buffer_t req_unmap_buffer;
		msg_req_dynamic_map_buffer_t req_dynamic_map_buffer;
		msg_req_dynamic_unmap_buffer_t req_dynamic_unmap_buffer;
		msg_req_dmap_offline_map_t req_dmap_offline_map;
		msg_req_dmap_offline_unmap_t req_dmap_offline_unmap;
		msg_req_dmap_map_t req_dmap_map;
		msg_req_dmap_unmap_t req_dmap_unmap;
		msg_req_cal_data_t req_cal_data;
		msg_req_flush_t req_flush;
		msg_req_dmap_flush_t req_dmap_flush;
		msg_req_extend_set_t req_extend_set;
		msg_req_extend_get_t req_extend_get;
		msg_req_inv_tlb_t req_inv_tlb;
		msg_req_query_ois_update_t req_query_ois_update;
		msg_req_ois_update_t req_ois_update;
		msg_req_query_laser_t req_query_laser;
		msg_req_acquire_laser_t req_acquire_laser;
		msg_req_release_laser_t req_release_laser;
		msg_req_mem_pool_init_t req_mem_pool_init;
		msg_req_mem_pool_deinit_t req_mem_pool_deinit;
		msg_req_isp_cpu_poweroff_t req_isp_cpu_poweroff;

		msg_req_raw2yuv_start_t req_raw2yuv_start;
		msg_req_raw2yuv_stop_t req_raw2yuv_stop;
		msg_req_request_offline_t req_raw2yuv_req;
		msg_req_request_offline_raw2rgb_t req_raw2rgb_req;
		msg_req_request_offline_rgb2yuv_t req_rgb2yuv_req;
		msg_req_map_buffer_offline_t req_raw2yuv_mapbuffer;
		msg_req_unmap_buffer_offline_t req_raw2yuv_unmapbuffer;
		msg_req_fbdraw_request_t req_fbdraw_request;

		msg_req_pq_setting_config_t req_pq_setting_config;
		msg_req_device_ctl_t req_device_ctl;
		msg_req_release_i2c_t req_release_i2c;
		msg_batch_req_request_t req_batch_request;
		msg_req_connect_camera_t req_connect_camera;

		/* add for ispnn req */
		msg_req_create_ispnn_model_t req_create_ispnn_model;
		msg_req_extend_ispnn_buffer_t req_extend_ispnn_buffer;
		msg_req_enable_ispnn_model_t req_enable_ispnn_model;
		msg_req_disable_ispnn_model_t req_disable_ispnn_model;
		msg_req_destroy_ispnn_model_t req_destroy_ispnn_model;

		/* add for raw processing RAWMFNR and BM3D */
		msg_req_raw_proc_start_t req_rawmfnr_bm3d_start;
		msg_req_raw_proc_stop_t req_rawmfnr_bm3d_stop;
		msg_req_raw_proc_map_buffer_t req_rawmfnr_bm3d_mapbuffer;
		msg_req_raw_proc_unmap_buffer_t req_rawmfnr_bm3d_unmapbuffer;
		/* Response items. */
		msg_ack_query_capability_t ack_query_capability;
		msg_ack_acquire_camera_t ack_require_camera;
		msg_ack_release_camera_t ack_release_camera;
		msg_ack_usecase_config_t ack_usecase_config;
		msg_ack_stream_on_t ack_stream_on;
		msg_ack_stream_off_t ack_stream_off;
		msg_ack_get_otp_t ack_get_otp;
		msg_ack_request_t ack_request;
		msg_ack_warp_request_t ack_warp_request;
		msg_ack_dmap_format_t ack_dmap_format;
		msg_ack_dmap_request_t ack_dmap_request;
		msg_ack_dgen_request_t ack_dgen_request;
		msg_ack_dopt_request_t ack_dopt_request;
		msg_ack_map_buffer_t ack_map_buffer;
		msg_ack_unmap_buffer_t ack_unmap_buffer;
		msg_ack_dynamic_map_buffer_t ack_dynamic_map_buffer;
		msg_ack_dynamic_unmap_buffer_t ack_dynamic_unmap_buffer;
		msg_ack_dmap_offline_map_t ack_dmap_offline_map_buffer;
		msg_ack_dmap_offline_unmap_t ack_dmap_offline_unmap_buffer;
		msg_ack_dmap_map_t ack_dmap_map_buffer;
		msg_ack_dmap_unmap_t ack_dmap_unmap_buffer;
		msg_ack_cal_data_t ack_cal_data;
		msg_ack_flush_t ack_flush;
		msg_ack_dmap_flush_t ack_dmap_flush;
		msg_ack_extend_set_t ack_extend_set;
		msg_ack_extend_get_t ack_extend_get;
		msg_ack_inv_tlb_t ack_inv_tlb;
		msg_ack_query_ois_update_t ack_query_ois_update;
		msg_ack_ois_update_t ack_ois_update;
		msg_ack_query_laser_t ack_query_laser;
		msg_ack_acquire_laser_t ack_require_laser;
		msg_ack_release_laser_t ack_release_laser;
		msg_ack_mem_pool_init_t ack_mem_pool_init;
		msg_ack_mem_pool_deinit_t ack_mem_pool_deinit;
		msg_ack_isp_cpu_poweroff_t ack_isp_cpu_poweroff;

		msg_ack_raw2yuv_start_t ack_raw2yuv_start;
		msg_ack_raw2yuv_stop_t ack_raw2yuv_stop;
		msg_ack_request_offline_t ack_raw2yuv_req;
		msg_ack_request_offline_t ack_raw2rgb_req;
		msg_ack_request_offline_t ack_rgb2yuv_req;
		msg_ack_map_buffer_offline_t ack_raw2yuv_mapbuffer;
		msg_ack_unmap_buffer_offline_t ack_raw2yuv_unmapbuffer;

		msg_ack_pq_setting_config_t ack_pq_setting_config;

		msg_ack_fbdraw_request_t ack_fbdraw_request;
		msg_ack_device_ctl_t ack_device_ctl;
		msg_ack_release_i2c_t ack_release_i2c;
		msg_batch_ack_request_t ack_batch_request;
		msg_ack_connect_camera_t ack_connect_camera;

		/* add for ispnn ack */
		msg_ack_create_ispnn_model_t ack_create_ispnn_model;
		msg_ack_extend_ispnn_buffer_t ack_extend_ispnn_buffer;
		msg_ack_enable_ispnn_model_t ack_enable_ispnn_model;
		msg_ack_disable_ispnn_model_t ack_disable_ispnn_model;
		msg_ack_destroy_ispnn_model_t ack_destroy_ispnn_model;

		msg_req_drbr_request_t req_drbr_request;
		msg_ack_drbr_request_t ack_drbr_request;

		msg_ack_raw_proc_start_t ack_raw_proc_start;
		msg_ack_raw_proc_stop_t ack_raw_proc_stop;
		msg_ack_raw_proc_request_t ack_raw_proc_req;
		msg_ack_raw_proc_map_buffer_t ack_raw_proc_mapbuffer;
		msg_ack_raw_proc_unmap_buffer_t ack_raw_proc_unmapbuffer;

		/* Event items sent to AP. */
		msg_event_sent_t event_sent;
	} u;
	msg_base_t base;
	short token;
	struct rpmsg_ept *ept;
} hisp_msg_t;

void msg_init_req(hisp_msg_t* req, unsigned int api_name, unsigned int msg_id);
void msg_init_ack(hisp_msg_t* req, hisp_msg_t* ack);

#endif /* HISP_MSG_H_INCLUDED */
