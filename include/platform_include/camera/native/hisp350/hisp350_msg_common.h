/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: header file common
 * Create: 2021-04-11
 */

#ifndef HISP350_MSG_COMMON_H_INCLUDED
#define HISP350_MSG_COMMON_H_INCLUDED

#define MAX_STREAM_NUM 15
#define NAME_LEN 32

typedef struct _stream_info_t {
	unsigned int buffer;
	unsigned int frame_num;
	unsigned short width;
	unsigned short height;
	unsigned short stride;
	unsigned short format;
	unsigned short valid;
} stream_info_t;

typedef struct _isp_crop_region_info_t {
	unsigned int x;
	unsigned int y;
	unsigned int width;
	unsigned int height;
} isp_crop_region_info_t;

#endif /* HISP350_MSG_COMMON_H_INCLUDED */
