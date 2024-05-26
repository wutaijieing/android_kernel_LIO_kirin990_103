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

static u32 s_aicore_disable_map = 0xff;

static int npu_plat_get_chip_cfg()
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
		"ioremap chip_cfg error, size:0x%x\n", npu_cfg_mem->len);
	chip_cfg = &(shm_cfg->cfg.chip_cfg);
	cond_goto_error(chip_cfg->valid_magic != NPU_DDR_CONFIG_VALID_MAGIC,
		COMPLETE, ret, -EINVAL,
		"va_npu_config valid_magic:0x%x is not valid\n",
		chip_cfg->valid_magic);
	s_aicore_disable_map = chip_cfg->aicore_disable_bitmap;
	npu_drv_warn("aicore disable map: %u\n", s_aicore_disable_map);

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

	if (s_aicore_disable_map == 0xff) {
		ret = npu_plat_get_chip_cfg();
		if (ret != 0)
			return 0;
	}

	aicore_disable = npu_bitmap_get(s_aicore_disable_map, core_id);

	return aicore_disable;
}
