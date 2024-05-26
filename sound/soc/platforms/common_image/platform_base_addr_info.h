/*
 * platform_base_addr_info.h
 *
 * get platform base addr info from dts
 *
 * Copyright (c) 2020 Huawei Technologies Co., Ltd.
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

#ifndef __PLATFORM_BASE_ADDR_INFO__
#define __PLATFORM_BASE_ADDR_INFO__

#include <linux/types.h>
#include <linux/device.h>

#ifndef CONFIG_AUDIO_COMMON_IMAGE
#include "audio/asp_register.h"
#ifdef CONFIG_PRODUCT_CDC_ACE
#include <global_ddr_map_cdc_ace.h>
#else
#include <global_ddr_map.h>
#endif
#endif

enum platform_mem_type {
	PLT_HIFI_MEM = 0,
	PLT_SECRAM_MEM,
	PLT_DMAC_MEM,
	PLT_WATCHDOG_MEM,
	PLT_ASP_CFG_MEM,
	PLT_ASP_CODEC_MEM,
	PLT_SLIMBUS_MEM,
	PLT_SCTRL_MEM,
	PLT_IOC_MEM,
	PLT_SOUNDWIRE_MEM,
#ifdef CONFIG_AUDIO_SOC_SHARED_IRQ
	PLT_INT_HUB_AO_MEM,
#endif
	PLT_MEM_MAX,
};

uint32_t get_platform_mem_base_addr(enum platform_mem_type mem_type);
uint32_t get_platform_mem_size(enum platform_mem_type mem_type);
uint32_t get_phy_addr(uint32_t addr, enum platform_mem_type mem_type);

#endif
