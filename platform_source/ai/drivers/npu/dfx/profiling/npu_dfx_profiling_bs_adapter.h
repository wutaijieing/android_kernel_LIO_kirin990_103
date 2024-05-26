/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * npu_dfx_profiling_bs_adapter.h
 *
 * about npu dfx profiling
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
#ifndef _PROF_DRV_DEV_BS_ADAPT_H_
#define _PROF_DRV_DEV_BS_ADAPT_H_

#include <linux/cdev.h>
#include "npu_user_common.h"
#include "npu_profiling.h"

/* profiling reserved memory size is 1M;
* head size = 4K; TSCPU DATA size  256k;
* AICORE DATA SIZE 508k
*/
#define PROF_TSCPU_DATA_SIZE            0x40000
#define PROF_AICORE_DATA_SIZE           (0x80000 - 0x1000)

struct prof_buff_desc {
	union {
		struct profiling_buff_manager manager;
		char data[PROF_HEAD_MANAGER_SIZE];
	} head;
	volatile char tscpu_data[PROF_TSCPU_DATA_SIZE];
	volatile char aicore_data[PROF_AICORE_DATA_SIZE];
};

#endif /* _PROF_DRV_DEV_V100_H_ */
