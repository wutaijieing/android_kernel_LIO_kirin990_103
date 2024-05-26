/*
 * vcodec_vdec_plat_v700.c
 *
 * This is for vdec platform
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/of.h>
#include "vcodec_vdec_plat.h"
#include "vcodec_vdec_regulator.h"

const char *pg_chip_bypass_vdec[] = {
	"level2_partial_good_modem",
	"level2_partial_good_drv",
	"unknown",
};

static bool bypass_vdec_by_soc_spec(void)
{
	int32_t ret;
	const char *soc_spec = NULL;
	struct device_node *np = of_find_compatible_node(NULL, NULL, "hisilicon, soc_spec");
	unsigned int i;
	if (!np) {
		dprint(PRN_FATAL, "of_find_compatible_node fail or normal type chip\n");
		return false;
	}

	ret = of_property_read_string(np, "soc_spec_set", &soc_spec);
	if (ret) {
		dprint(PRN_FATAL, "of_property_read_string fail\n");
		return false;
	}

	for (i = 0; i < sizeof (pg_chip_bypass_vdec) / sizeof(pg_chip_bypass_vdec[0]); i++) {
		ret = strncmp(soc_spec, pg_chip_bypass_vdec[i], strlen(pg_chip_bypass_vdec[i]));
		if (!ret) {
			dprint(PRN_ALWS, "is pg chip : %s, need bypass vdec\n", pg_chip_bypass_vdec[i]);
			return true;
		}
	}

	dprint(PRN_ALWS, " no need to bypss vdec\n");
	return false;
}

static bool need_cfg_tbu_max_tok_trans(void)
{
	vdec_dts *dts_info = &(vdec_plat_get_entry()->dts_info);
	return dts_info->is_es_plat;
}

static vdec_plat_ops g_vdec_plat_v700_ops = {
	.vcodec_vdec_device_bypass = bypass_vdec_by_soc_spec,
	.smmu_need_cfg_max_tok_trans = need_cfg_tbu_max_tok_trans,
};

vdec_plat_ops *get_vdec_plat_ops(void)
{
	return &g_vdec_plat_v700_ops;
}
