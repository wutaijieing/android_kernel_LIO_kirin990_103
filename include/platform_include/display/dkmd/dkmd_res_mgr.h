/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#ifndef DKMD_RES_MGR_H
#define DKMD_RES_MGR_H

#include <linux/types.h>
#include <linux/bitops.h>

#define DPU_HW_TYPE_FPGA 0
#define DPU_HW_TYPE_ASIC 1
#define DPU_SOC_TYPE_ES  0
#define DPU_SOC_TYPE_CS  1

#define RES_IOCTL_MAGIC 'R'

enum {
	RES_IOCTL_INIT_OPRS = 0x10,
	RES_IOCTL_REQUEST_OPR = 0x11,
	RES_IOCTL_REQUEST_OPR_ASYNC = 0x12,
	RES_IOCTL_RELEASE_OPR = 0x13,
	RES_IOCTL_GET_CONFIG = 0x14,
	RES_IOCTL_MAP_IOVA = 0x15,
	RES_IOCTL_UNMAP_IOVA  = 0x16,
	RES_IOCTL_REGISTER_TYPES = 0x17,
	RES_IOCTL_REQUEST_SCENE_ID = 0x18,
	RES_IOCTL_RELEASE_SCENE_ID = 0x19,
	RES_IOCTL_GET_VSCREEN_INFO = 0x20,
	RES_IOCTL_GET_DISP_VERSION = 0x21,
	RES_IOCTL_NORMAL_DVFS = 0x22,
	RES_IOCTL_FRAME_INTRA_DVFS = 0x23,
	RES_IOCTL_CMD_MAX,
};

// RES_OPERATORS
#define RES_INIT_OPR         _IOW(RES_IOCTL_MAGIC, RES_IOCTL_INIT_OPRS, struct res_opr_type)
#define RES_REQUEST_OPR      _IOW(RES_IOCTL_MAGIC, RES_IOCTL_REQUEST_OPR, struct res_opr_info)
#define RES_REQUEST_OPR_ASYNC _IOW(RES_IOCTL_MAGIC, RES_IOCTL_REQUEST_OPR_ASYNC, struct res_opr_info)
#define RES_RELEASE_OPR      _IOW(RES_IOCTL_MAGIC, RES_IOCTL_RELEASE_OPR, uint32_t)

// RES_GR_DEV
#define RES_MAP_IOVA         _IOWR(RES_IOCTL_MAGIC, RES_IOCTL_MAP_IOVA, struct res_dma_buf)
#define RES_UNMAP_IOVA       _IOWR(RES_IOCTL_MAGIC, RES_IOCTL_UNMAP_IOVA, struct res_dma_buf)
#define RES_GET_VSCREEN_INFO _IOWR(RES_IOCTL_MAGIC, RES_IOCTL_GET_VSCREEN_INFO, struct screen_info)
#define RES_GET_DISP_VERSION _IOWR(RES_IOCTL_MAGIC, RES_IOCTL_GET_DISP_VERSION, uint64_t)

#define RES_REGISTER_TYPES   _IOWR(RES_IOCTL_MAGIC, RES_IOCTL_REGISTER_TYPES, uint64_t)     // enum res_types

#define RES_REQUEST_SCENE_ID _IOWR(RES_IOCTL_MAGIC, RES_IOCTL_REQUEST_SCENE_ID, struct scene_id_info)
#define RES_RELEASE_SCENE_ID _IOWR(RES_IOCTL_MAGIC, RES_IOCTL_RELEASE_SCENE_ID, struct scene_id_info)

enum res_types {
	RES_OPERATORS = BIT(0),
	RES_DVFS = BIT(1),
	RES_GR_DEV = BIT(2), // for gralloc
	RES_SCENE_ID = BIT(3),
};

enum scene_type {
	SCENE_TYPE_ONLINE = 0,
	SCENE_TYPE_OFFLINE,
	SCENE_TYPE_MAX,
};

enum dpu_version_code {
	DPU_ACCEL_DPUV700 = 0x8000,
	DPU_ACCEL_DPUV160 = 0x8001,
	DPU_ACCEL_DPUV740 = 0x8002,
	DPU_ACCEL_DPUV741 = 0x8003,
};

enum scene_user_type {
	SCENE_USER_NONE = 0,
	SCENE_USER_HDM = 1,
	SCENE_USER_MDC = 2,
	SCENE_USER_EFFECT = 3,
	SCENE_USER_MAX
};

struct res_opr_info {
	int32_t scene_id;
	uint32_t scene_type;
	int32_t opr_id;
	int32_t result;
};

struct res_opr_type {
	uint32_t opr_type;
	uint32_t opr_count;
	uint64_t opr_id_ptr;
};

struct res_dma_buf {
	int share_fd;
	uint64_t iova;
	uint64_t size;
};

struct screen_info {
	uint32_t xres;
	uint32_t yres;
};

union dpu_version {
	uint64_t value;
	struct {
		uint64_t version   : 32;     /* serial number */
		uint64_t reserved1 : 16;
		uint64_t hw_type   : 1;      /* fpga or asic */
		uint64_t soc_type  : 1;     /* es or cs */
		uint64_t reserved2 : 14;
	} info;
};

struct scene_id_user {
	enum scene_user_type user_type;
	enum scene_type scene_type;
};

struct scene_id_info {
	struct scene_id_user user;
	int32_t scene_id;
	int32_t result; // 0 is succ, not 0 is fail
};

#endif
