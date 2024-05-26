/*
 * platform_base_addr_info.c
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

#include "platform_base_addr_info.h"

#include <linux/of.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/printk.h>
#include <linux/io.h>

#include "audio_log.h"

#define LOG_TAG "common_image"

#define MEM_INFO_CNT 2
#define MEM_BASE_ADDR_INDEX 0
#define MEM_SIZE_INDEX 1

struct platform_mem {
	const char *name;
	uint32_t base_addr;
	uint32_t size;
};

static struct platform_mem g_platform_mems[PLT_MEM_MAX] = {
	{ "hifi_mem",},
	{ "secram_mem",},
	{ "dmac_mem",},
	{ "watchdog_mem",},
	{ "asp_cfg_mem",},
	{ "asp_codec_mem",},
	{ "slimbus_mem",},
	{ "sctrl_mem",},
	{ "ioc_mem",},
	{ "soundwire_mem",},
};

static int32_t check_params(enum platform_mem_type mem_type)
{
	if (mem_type < 0 || mem_type >= PLT_MEM_MAX) {
		AUDIO_LOGE("invalid addr type %d", mem_type);
		return -EINVAL;
	}

	return 0;
}

uint32_t get_platform_mem_base_addr(enum platform_mem_type mem_type)
{
	int32_t ret;

	ret = check_params(mem_type);
	if (ret)
		return 0;

	return g_platform_mems[mem_type].base_addr;
}

uint32_t get_platform_mem_size(enum platform_mem_type mem_type)
{
	int32_t ret;

	ret = check_params(mem_type);
	if (ret)
		return 0;

	return g_platform_mems[mem_type].size;
}

uint32_t get_phy_addr(uint32_t addr, enum platform_mem_type mem_type)
{
	int32_t ret;

	ret = check_params(mem_type);
	if (ret)
		return 0;

#ifndef CONFIG_AUDIO_COMMON_IMAGE
	return addr;
#else
	return g_platform_mems[mem_type].base_addr + addr;
#endif
}

#ifdef CONFIG_AUDIO_COMMON_IMAGE
static int __init platform_mem_info_init(void)
{
	int32_t i;
	struct device_node *root = NULL;
	uint32_t out_values[MEM_INFO_CNT] = {0};
	int32_t ret = -EINVAL;

	root = of_find_node_by_path("/audio_mem_info");
	if (!root) {
		AUDIO_LOGE("dts config error");
		goto out;
	}

	for (i = PLT_HIFI_MEM; i < PLT_MEM_MAX; i++) {
		ret = of_property_read_u32_array(root, g_platform_mems[i].name,
			&out_values[0], MEM_INFO_CNT);
		if (ret) {
			AUDIO_LOGE("property read %s failed", g_platform_mems[i].name);
			goto out;
		}
		g_platform_mems[i].base_addr = out_values[MEM_BASE_ADDR_INDEX];
		g_platform_mems[i].size = out_values[MEM_SIZE_INDEX];
		AUDIO_LOGI("%s property base_addr %#x size %#x", g_platform_mems[i].name,
			g_platform_mems[i].base_addr, g_platform_mems[i].size);
	}

out:
	of_node_put(root);
	return ret;
}

fs_initcall(platform_mem_info_init);
#endif