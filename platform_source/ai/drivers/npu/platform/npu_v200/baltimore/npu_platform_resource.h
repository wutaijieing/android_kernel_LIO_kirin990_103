/*
 * npu_platform_resource.h
 *
 * about npu platform resource
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
#ifndef __NPU_PLATFORM_RESOURCE_H
#define __NPU_PLATFORM_RESOURCE_H

#define NPU_MAX_MODEL_ID        384
#define NPU_MAX_PROCESS_MODEL_NUM       (NPU_MAX_MODEL_ID)
#define NPU_MAX_PROCESS_SQ_NUM  80

#define NPU_PLAT_AICORE_MAX     2

#define NPU_AIC_SINK_SHORT_STREAM_SUM   352
#define NPU_AIC_SINK_LONG_STREAM_SUM    32
#define NPU_AIC_NON_SINK_STREAM_SUM     (NPU_MAX_MODEL_ID + 64)

#define NPU_MAX_SINK_LONG_STREAM_ID     (NPU_AIC_SINK_LONG_STREAM_SUM)
#define NPU_MAX_SINK_SHORT_STREAM_ID    (NPU_AIC_SINK_SHORT_STREAM_SUM)
#define NPU_MAX_SINK_STREAM_ID           (NPU_AIC_SINK_SHORT_STREAM_SUM + NPU_AIC_SINK_LONG_STREAM_SUM)
#define NPU_MAX_NON_SINK_STREAM_ID      (NPU_AIC_NON_SINK_STREAM_SUM)
#define NPU_MAX_STREAM_ID               (NPU_AIC_NON_SINK_STREAM_SUM + \
	NPU_AIC_SINK_LONG_STREAM_SUM + NPU_AIC_SINK_SHORT_STREAM_SUM)

#define NPU_MAX_HWTS_SQ_NUM           384
#define NPU_MAX_HWTS_CQ_NUM           20

#define NPU_PLAT_SYSCACHE_SIZE  0x800000 /* 8M */

#define NPU_NS_PROF_SIZE        0x1000
#endif
