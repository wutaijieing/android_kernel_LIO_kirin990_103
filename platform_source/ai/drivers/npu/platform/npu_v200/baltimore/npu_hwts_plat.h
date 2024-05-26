/*
 * npu_hwts_plat.h
 *
 * about npu hwts plat
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
#ifndef __NPU_HWTS_PLAT_H
#define __NPU_HWTS_PLAT_H

#include "npu_platform_register.h"
#include "npu_adapter.h"
#include "npu_platform.h"
#include "npu_common.h"

#define TEST_AICORE_SHIFT 0x14  // aicore 1 base addr: aicore 0 + 1 << 0x14

void hwts_profiling_init(u8 devid);

struct hwts_interrupt_info {
	u8 aic_batch_mode_timeout_man;
	u8 aic_task_runtime_limit_exp;
	u8 wait_task_limit_exp;
	u64 l1_normal_ns_interrupt;
	u64 l1_debug_ns_interrupt;
	u64 l1_error_ns_interrupt;
	u64 l1_dfx_interrupt;
};

struct sq_exception_info {
	u16 sq_head;
	u16 sq_length;
	u16 sq_tail;
	u8 aic_own_bitmap;
	u64 sqcq_fsm_state;
};

u64 npu_hwts_get_base_addr(void);

int npu_hwts_query_aicore_pool_status(
	u8 index, u8 pool_sec, u8 *reg_val);

int npu_hwts_query_sdma_pool_status(
	u8 index, u8 pool_sec, u8 *reg_val);

int npu_hwts_query_bus_error_type(u8 *reg_val);

int npu_hwts_query_bus_error_id(u8 *reg_val);

int npu_hwts_query_sw_status(u16 channel_id, u32 *reg_val);

int npu_hwts_query_sq_head(u16 channel_id, u16 *reg_val);

int npu_hwts_query_sqe_type(u16 channel_id, u8 *reg_val);

int npu_hwts_query_aic_own_bitmap(u16 channel_id, u8 *reg_val);

int npu_hwts_query_aic_exception_bitmap(u16 channel_id, u8 *reg_val);

int npu_hwts_query_aic_trap_bitmap(u16 channel_id, u8 *reg_val);

int npu_hwts_query_sdma_own_state(u16 channel_id, u8 *reg_val);

int npu_hwts_query_aic_task_config(void);

int npu_hwts_query_sdma_task_config(void);

int npu_hwts_query_interrupt_info(
	struct hwts_interrupt_info *interrupt_info);

int npu_hwts_query_sq_info(
	u16 channel_id, struct sq_exception_info *sq_info);

int npu_hwts_query_ispnn_info(u16 channel_id);

#endif
