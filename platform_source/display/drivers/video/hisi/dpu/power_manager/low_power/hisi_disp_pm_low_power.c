/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "hisi_disp_pm.h"

static void  hisi_disp_pm_base_set_lp_clk(struct hisi_disp_pm_lowpower *lp_mng)
{
	disp_pr_info("+++");
}

static void  hisi_disp_pm_base_set_lp_memory(struct hisi_disp_pm_lowpower *lp_mng)
{
	disp_pr_info("+++");
}

void hisi_disp_pm_registe_base_lp_cb(struct hisi_disp_pm_lowpower *lp_mng)
{
	lp_mng->set_lp_clk = hisi_disp_pm_base_set_lp_clk;
	lp_mng->set_lp_memory = hisi_disp_pm_base_set_lp_memory; // memory default is low power mode

	hisi_disp_pm_registe_platform_lp_cb(lp_mng);
}