/*
 * npu_io.h
 *
 * about npu io
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
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
#ifndef __NPU_IO_H
#define __NPU_IO_H
#include "npu_platform_register.h"
#include "npu_adapter.h"
#include "npu_platform.h"

#ifndef read_uint32
#define read_uint32(uw_value, addr) \
	((uw_value) = *((volatile u32 *)((uintptr_t)(addr))))
#endif
#ifndef read_uint64
#define read_uint64(ull_value, addr) \
	((ull_value) = *((volatile u64 *)((uintptr_t)(addr))))
#endif
#ifndef write_uint32
#define write_uint32(uw_value, addr) \
	(*((volatile u32 *)((uintptr_t)(addr))) = (uw_value))
#endif
#ifndef write_uint64
#define write_uint64(ull_value, addr) \
	(*((volatile u64 *)((uintptr_t)(addr))) = (ull_value))
#endif

static inline void update_reg32(u64 addr, u32 value, u32 mask)
{
	u32 reg_val;

	read_uint32(reg_val, addr);
	reg_val = (reg_val & (~mask)) | (value & mask);
	write_uint32(reg_val, addr);
}

static inline void update_reg64(u64 addr, u64 value, u64 mask)
{
	u64 reg_val;

	read_uint64(reg_val, addr);
	reg_val = (reg_val & (~mask)) | (value & mask);
	write_uint64(reg_val, addr);
}

#endif
