/*
 * npu_sdma_plat.h
 *
 * about npu sdma plat
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
#ifndef __NPU_SDMA_PLAT_H
#define __NPU_SDMA_PLAT_H

#include "npu_platform_register.h"
#include "npu_adapter.h"
#include "npu_platform.h"

struct sdma_exception_info {
	u64 s_addr;
	u64 d_addr;
	u8 s_sid;
	u8 s_subsid;
	u8 d_sid;
	u8 d_subsid;
};

int npu_sdma_query_exception_info(
	u8 sdma_channel, struct sdma_exception_info *sdma_info);

#endif
