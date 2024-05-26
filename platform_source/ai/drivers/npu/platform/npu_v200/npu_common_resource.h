/*
 * npu_common_resource.h
 *
 * about v200 common resource specification
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
#ifndef __NPU_COMMON_RESOURCE_H
#define __NPU_COMMON_RESOURCE_H
#include "global_ddr_map.h"
#include "npu_model_description.h"
#include "npu_platform_resource.h"

#define NPU_MAX_EVENT_ID                256

// each stream is encoded separately
#define NPU_MAX_SINK_TASK_ID            1024 // normal stream
#define NPU_MAX_SINK_LONG_TASK_ID       8192 // long stream
#define NPU_MAX_TASK_START_ID           32678
#define NPU_MAX_NOTIFY_ID               256

#define NPU_SQ_SLOT_SIZE                64
#define NPU_CQ_SLOT_SIZE                16

#define NPU_MAX_SQ_NUM                  80
#define NPU_MAX_CQ_NUM                  20
#define NPU_MAX_DFX_SQ_NUM              4
#define NPU_MAX_DFX_CQ_NUM              10

#define NPU_DFX_SQ_OFFSET               0x200000

#define NPU_MAX_SQ_DEPTH                256
#define NPU_MAX_CQ_DEPTH                1024
#define NPU_DEV_NUM                     1

#define NPU_PLAT_DOORBELL_STRIDE        4096  /* stride 4KB */

#define NPU_HWTS_CHANNEL_NUM            64

#define NPU_NS_PROFILE_BASE_ADDR \
	(NPU_NS_CTRL_INFO_BASE_ADDR + NPU_PROFILINGL_INFO_OFFSET)

enum npu_dump_region_index {
	NPU_DUMP_REGION_NPU_CRG,
	NPU_DUMP_REGION_MAX
};

#define NPU_PROFILINGL_INFO_OFFSET \
	((NPU_NS_CTRL_INFO_SIZE - NPU_MAILBOX_OCCUPY_SIZE) - NPU_PROFILINGL_INFO_OCCUPY_SIZE)

#define NPU_INFO_OCCUPY_SIZE \
	((NPU_SQ_INFO_OCCUPY_SIZE) + (NPU_CQ_INFO_OCCUPY_SIZE) + \
		(NPU_STREAM_INFO_OCCUPY_SIZE) + (NPU_HWTS_SQ_INFO_OCCUPY_SIZE) + \
		(NPU_HWTS_CQ_INFO_OCCUPY_SIZE) + (NPU_MODEL_INFO_OCCUPY_SIZE))

#endif
