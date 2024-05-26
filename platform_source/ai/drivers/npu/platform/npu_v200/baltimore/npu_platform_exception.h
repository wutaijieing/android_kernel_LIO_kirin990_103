/*
 * npu_platform_exception.h
 *
 * about npu proc hwts exception
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
#ifndef __NPU_PLATFORM_EXCEPTION_H
#define __NPU_PLATFORM_EXCEPTION_H

#include "npu_platform_register.h"
#include "npu_adapter.h"

#define NPU_HWTS_MAX_AICORE_POOL_NUM 4
#define NPU_HWTS_MAX_SDMA_POOL_NUM 4

void npu_exception_powerdown(struct npu_dev_ctx *dev_ctx, struct hwts_exception_report_info *report);

void npu_exception_report_proc(
	struct npu_dev_ctx *dev_ctx, struct hwts_exception_report_info *report);

#endif
