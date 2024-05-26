/*
 * asp_codec_debug.h -- asp codec debug define
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
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

#ifndef __ASP_CODEC_DEBUG_H__
#define __ASP_CODEC_DEBUG_H__

#include "platform_base_addr_info.h"

#ifdef CONFIG_AUDIO_COMMON_IMAGE
#undef SOC_ACPU_ASP_CODEC_BASE_ADDR
#define SOC_ACPU_ASP_CODEC_BASE_ADDR 0
#endif
#include "asp_codec_utils.h"

#define PAGE_SOCCODEC_BASE_ADDR (SOC_ACPU_ASP_CODEC_BASE_ADDR)

#define DBG_SOCCODEC_START_ADDR \
	(PAGE_SOCCODEC_BASE_ADDR + SOCCODEC_START_OFFSET)
#define DBG_SOCCODEC_END_ADDR (PAGE_SOCCODEC_BASE_ADDR + SOCCODEC_END_OFFSET)
#endif

