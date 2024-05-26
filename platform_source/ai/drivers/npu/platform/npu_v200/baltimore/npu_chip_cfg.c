/*
 * npu_chip_cfg.c
 *
 * about chip config
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
 */

#include <linux/io.h>
#include <npu_shm_config.h>
#include "npu_log.h"
#include "npu_common.h"
#include "npu_platform.h"
#include "npu_atf_subsys.h"
#include "npu_adapter.h"

#define LIMITED_SPEC_VALUE    6

static u32 s_aicore_disable_map = UINT8_MAX;
static u32 s_platform_specification = UINT8_MAX;

static int npu_plat_get_chip_cfg(void)
{
	int ret = 0;
	struct npu_chip_cfg *chip_cfg = NULL;
	struct npu_platform_info *plat_info = NULL;
	struct npu_mem_desc *npu_cfg_mem = NULL;
	npu_shm_cfg *shm_cfg = NULL;

	plat_info = npu_plat_get_info();
	if (plat_info == NULL) {
		npu_drv_err("get platform info error\n");
		return -1;
	}

	npu_cfg_mem = plat_info->resmem_info.npu_cfg_buf;
	npu_drv_debug("chip_cfg_mem.base = 0x%pK, chip_cfg_mem.len = 0x%x\n",
		npu_cfg_mem->base, npu_cfg_mem->len);

	shm_cfg = (npu_shm_cfg *)ioremap_wc(npu_cfg_mem->base,
		npu_cfg_mem->len);
	cond_return_error(shm_cfg == NULL, -EINVAL,
		"ioremap shm_cfg error, size:0x%x\n", npu_cfg_mem->len);
	chip_cfg = &(shm_cfg->cfg.chip_cfg);
	cond_goto_error(chip_cfg->valid_magic != NPU_DDR_CONFIG_VALID_MAGIC,
		COMPLETE, ret, -EINVAL,
		"va_npu_config valid_magic:0x%x is not valid\n",
		chip_cfg->valid_magic);
	s_aicore_disable_map = chip_cfg->aicore_disable_bitmap;
	s_platform_specification = chip_cfg->platform_specification;
	npu_drv_warn("aicore disable map : %u platform specification : %u\n",
		s_aicore_disable_map, s_platform_specification);

COMPLETE:
	iounmap((void *)chip_cfg);
	return ret;
}

/*
 * return value : 1 disable core; 0 not disable core
 */
int npu_plat_aicore_get_disable_status(u32 core_id)
{
	int ret;
	int aicore_disable;

	if (s_aicore_disable_map == UINT8_MAX) {
		ret = npu_plat_get_chip_cfg();
		if (ret != 0)
			return 0;
	}

	aicore_disable = npu_bitmap_get(s_aicore_disable_map, core_id);

	return aicore_disable;
}

/*
 * return value: 1 limited chip, 0 full specification chip
 */
static bool npu_plat_is_limited_chip(void)
{
	int ret;

	if (s_platform_specification == UINT8_MAX) {
		ret = npu_plat_get_chip_cfg();
		if (ret != 0)
			return false;
	}

	if (s_platform_specification == (u32)LIMITED_SPEC_VALUE)
		return true;

	return false;
}

/*
 * return value : 1 support ispnn; 0 not support ispnn
 */
bool npu_plat_is_support_ispnn(void)
{
	return !npu_plat_is_limited_chip();
}

/*
 * return value : 1 support sc; 0 not support sc
 */
bool npu_plat_is_support_sc(void)
{
	return !npu_plat_is_limited_chip();
}

/*
 * return value : NPU_NON_BYPASS, NPU_BYPASS
 */
int npu_plat_bypass_status(void)
{
	int ret;
	u32 index;
	struct device_node *soc_spec_np = NULL;
	const char *soc_spec = NULL;
	static int bypass_status = NPU_INIT_BYPASS;
	int tmp_status = NPU_NON_BYPASS;
	static const char *npu_bypass_soc_spec[] = {
		"pc", "pc-normal", "lsd", "lsd-normal", "sd", "sd-normal",
	};

	if (bypass_status != NPU_INIT_BYPASS)
		return bypass_status;

	soc_spec_np = of_find_compatible_node(NULL, NULL, "hisilicon, soc_spec");
	if (soc_spec_np == NULL) {
		npu_drv_err("find compatible node fail\n");
		return bypass_status;
	}

	ret = of_property_read_string(soc_spec_np, "soc_spec_set", &soc_spec);
	if (ret < 0) {
		npu_drv_err("read property string fail\n");
		return bypass_status;
	}

	npu_drv_warn("soc spec is %s\n", soc_spec);

	for (index = 0; index < ARRAY_SIZE(npu_bypass_soc_spec); index++) {
		if (!strcmp(soc_spec, npu_bypass_soc_spec[index])) {
			tmp_status = NPU_BYPASS;
			break;
		}
	}

	bypass_status = tmp_status;

	return bypass_status;
}
