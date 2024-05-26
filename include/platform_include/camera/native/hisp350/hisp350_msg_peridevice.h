/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: header file for peridevice initial draft
 * Create: 2019-01-11
 */

#ifndef HISP350_MSG_PERIDEVICE_H_INCLUDED
#define HISP350_MSG_PERIDEVICE_H_INCLUDED

#include "hisp350_msg_common.h"

#define PER_COUNT_FIRST 32
#define MAX_OIS_NUM 5
#define NAME_LENGTH 32
#define DMD_MSG_LENGTH 64
#define DMD_NUMBERS 3

typedef struct _msg_req_query_laser_t {
	unsigned int i2c_index;
	char product_name[NAME_LEN];
	char name[NAME_LEN];
} msg_req_query_laser_t;

typedef struct _laser_spad_t {
	unsigned int ref_spad_count;
	unsigned char is_aperture_spads;
}laser_spad_t;

typedef struct _laser_dmax_t {
	unsigned int dmax_range;
	unsigned int dmax_rate;
}laser_dmax_t;

typedef struct _msg_ack_query_laser_t {
	char name[NAME_LEN];
	unsigned char revision;
	int status;
	laser_spad_t spad;
} msg_ack_query_laser_t;

typedef struct _laser_fov_t {
	float x;
	float y;
	float width;
	float height;
	float angle;
}laser_fov_t;

typedef struct _msg_req_acquire_laser_t {
	unsigned int i2c_index;
	char product_name[NAME_LEN];
	char name[NAME_LEN];
	int offset;
	int xtalk;
	laser_fov_t fov_info;
	laser_spad_t spad;
	laser_dmax_t dmax;
} msg_req_acquire_laser_t;

typedef struct _msg_ack_acquire_laser_t {
	char name[NAME_LEN];
	unsigned char revision;
	int status;
} msg_ack_acquire_laser_t;

typedef struct _msg_req_release_laser_t {
	unsigned int i2c_index;
} msg_req_release_laser_t;

typedef struct _msg_ack_release_laser_t {
	unsigned int i2c_index;
} msg_ack_release_laser_t;

typedef struct _msg_req_query_ois_update_t {
	unsigned int cam_id;
	char sensor_name[PER_COUNT_FIRST];
} msg_req_query_ois_update_t;

typedef struct _msg_ack_query_ois_update_t {
	unsigned int cam_id;
	int status;
} msg_ack_query_ois_update_t;

typedef struct _msg_req_ois_update_t {
	unsigned int cam_id;
	char sensor_name[PER_COUNT_FIRST];
	unsigned int input_ois_buffer;
	unsigned int input_ois_buffer_size;
} msg_req_ois_update_t;

typedef struct _msg_ack_ois_update_t {
	unsigned int cam_id;
	int status;
} msg_ack_ois_update_t;

typedef struct _isp_device_ctl_dot_t {
	unsigned int data;
} isp_device_ctl_dot_t;

typedef struct _isp_device_ctl_cam_t {
	unsigned int cam_id;
	unsigned int cam_addr;
	unsigned int cci_id;
	unsigned int ctl_dev;
	unsigned int rw_pad;
	unsigned int cam_reg;
	unsigned int reg_val;
	unsigned int reg_bit;
	unsigned int val_bit;
} isp_device_ctl_cam_t;

typedef struct _isp_device_ctl_ois_t {
	char dev_name[MAX_OIS_NUM][NAME_LENGTH];
	unsigned int ois_num;
	unsigned int addr;
	unsigned int data;
} isp_device_ctl_ois_t;

typedef struct _dmd_msg_t {
	unsigned int dmd_id;
	unsigned char msg[DMD_MSG_LENGTH];
} dmd_msg_t;

typedef struct _dmd_msg_array_t {
	unsigned int real_length;
	dmd_msg_t dmd_array[DMD_NUMBERS];
} dmd_msg_array_t;

typedef struct _isp_device_ctl_dump_info_t {
	unsigned int size;
	unsigned int info;
} isp_device_ctl_dump_info_t;

typedef struct _isp_device_ctl_t {
	unsigned int msg;
	union {
		isp_device_ctl_dot_t dot_ctl;
		isp_device_ctl_cam_t cam_ctl;
		isp_device_ctl_ois_t ois_ctl;
		dmd_msg_array_t dmd_array;
		isp_device_ctl_dump_info_t info;
	} u;
} isp_device_ctl_t;

typedef struct _msg_req_device_ctl_t {
	unsigned int dev_type;
	char dev_name[NAME_LENGTH];
	unsigned int i2c_index;
	isp_device_ctl_t ctl;
	int ack;
} msg_req_device_ctl_t;

typedef struct _msg_ack_device_ctl_t {
	unsigned int dev_type;
	char dev_name[NAME_LENGTH];
	unsigned int i2c_index;
	isp_device_ctl_t ctl;
	int ack;
} msg_ack_device_ctl_t;

#endif /* HISP350_MSG_PERIDEVICE_H_INCLUDED */
