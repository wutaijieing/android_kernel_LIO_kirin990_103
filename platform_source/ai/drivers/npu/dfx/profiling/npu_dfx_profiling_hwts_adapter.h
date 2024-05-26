/*
 * npu_dfx_profiling_hwts_adapter.h
 *
 * about npu dfx profiling
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef _PROF_DRV_DEV_HWTS_ADAPT_H_
#define _PROF_DRV_DEV_HWTS_ADAPT_H_

#include <linux/cdev.h>
#include "npu_user_common.h"
#include "npu_model_description.h"
#include "npu_platform.h"
#include "npu_iova.h"
#include "npu_profiling.h"

/* profiling reserved memory size is 4k;
 * head size = 4K; TSCPU DATA size  256k;
 * AICORE DATA SIZE 2M
 */
#define PROF_TSCPU_DATA_SIZE            0x40000
#define PROF_HWTS_LOG_SIZE            0x200000
#define PROF_HWTS_PROFILING_SIZE           0x200000

#define PROF_HWTS_LOG_FORMAT_SIZE       64
#define PROF_HWTS_PROFILING_FORMAT_SIZE       128

struct prof_buff_desc {
	union {
		struct profiling_buff_manager manager;
		char data[PROF_HEAD_MANAGER_SIZE];
	} head;
};

#endif /* _PROF_DRV_DEV_H_ */
