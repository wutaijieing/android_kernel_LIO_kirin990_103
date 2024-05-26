/*
 * vcodec_vdec_plat_v600.c
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

#include "vcodec_vdec_plat.h"

static bool bypass_vdec_by_soc_spec(void)
{
	return false;
}

static bool need_cfg_tbu_max_tok_trans(void)
{
	return true;
}

static vdec_plat_ops g_vdec_plat_v600_ops = {
	.vcodec_vdec_device_bypass = bypass_vdec_by_soc_spec,
	.smmu_need_cfg_max_tok_trans = need_cfg_tbu_max_tok_trans,
};

vdec_plat_ops *get_vdec_plat_ops(void)
{
	return &g_vdec_plat_v600_ops;
}

