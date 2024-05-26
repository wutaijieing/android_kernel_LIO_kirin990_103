/*
 * npu_sink_sqe_fmt.h
 *
 * about npu hwts sqe fmt
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
#ifndef _NPU_SINK_SQE_FMT_H_
#define _NPU_SINK_SQE_FMT_H_

#include <linux/types.h>
#include "npu_user_common.h"
#include "npu_soc_defines.h"


int npu_format_hwts_sqe(void *hwts_sq_addr, void *stream_buf_addr,
	u64 ts_sq_len, npu_model_desc_info_t *model_desc_info);

int npu_hwts_query_sqe_info(u64 sqe);

#endif
