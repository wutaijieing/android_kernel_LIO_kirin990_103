/*
 * npu_aicore_plat.h
 *
 * about npu aicore plat
 *
 * Copyright (c) 2019-2020 Huawei Technologies Co., Ltd.
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
#ifndef __NPU_AICORE_PLAT_H
#define __NPU_AICORE_PLAT_H

#include "npu_platform_register.h"
#include "npu_adapter.h"
#include "npu_platform.h"

#define NPU_AICORE_DBG_WAIT_LOOP_MAX 1000

enum aicore_memtype_debug_mode {
	LOC_MEM_FIRST = 0,
	LOC_MEM_GPR = 0,
	LOC_MEM_L0A = 1,
	LOC_MEM_L0B = 2,
	LOC_MEM_L0C = 3,
	LOC_MEM_UB = 4, /* unified buffer */
	LOC_MEM_L1 = 5,
	LOC_MEM_OUT_MEM = 6,
	LOC_MEM_IFU_CACHE_LINE = 7,
	LOC_MEM_MTE = 8,
	LOC_MEM_SCALAR_BUFFER = 9,
	LOC_MEM_DATA_CACHE = 10,
	LOC_MEM_LAST,
};

struct aicore_exception_info {
	u64 ifu_start;
	u64 aic_error;
	u64 aic_error1;
	u64 aic_error2;
	u64 aic_error_mask;
	u64 aic_core_int;
};

struct aicore_exception_error_info {
	u64 biu_err_info;
	u64 ccu_err_info;
	u64 cube_err_info;
	u64 mte_err_info;
	u64 vec_err_info;
	u64 mte_ecc_1bit_err;
	u64 vec_cube_ecc_1bit_err;
};

struct aicore_exception_dbg_info {
	u64 ifu_current;
	u64 general_register[32];
};

int npu_aicore_query_exception_info(
	u8 aic_id, struct aicore_exception_info *aic_info);

int npu_aicore_query_exception_error_info(
	u8 aic_id, struct aicore_exception_error_info *aic_info);

int npu_aicore_query_exception_dbg_info(
	u8 aic_id, struct aicore_exception_dbg_info *aic_info);

int npu_aicore_query_ub_flowtable(
	u8 aic_id, u64 query_addr, u64 *buffer, u32 buffer_len);

#endif
