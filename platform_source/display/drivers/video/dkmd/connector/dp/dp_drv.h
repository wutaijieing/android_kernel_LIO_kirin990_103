/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef __DP_DRV_H__
#define __DP_DRV_H__

#include "dkmd_peri.h"
#include "dp_ctrl.h"

#define DEV_NAME_DP "gfx_dp"
#define DTS_COMP_DP "dkmd,gfx_dp"

struct dp_private {
	struct dkmd_connector_info connector_info;

	char __iomem *hidptx_base;
	char __iomem *hsdt1_crg_base;
	char __iomem *combo_phy_base;

	struct clk *clk_dpctrl_pixel;

	/* other dp information */
	struct dp_ctrl dp;

	struct platform_device *device;
};

extern struct platform_device *g_dkmd_dp_devive;
static inline struct dp_private *to_dp_private(struct dkmd_connector_info *info)
{
	return container_of(info, struct dp_private, connector_info);
}

#endif
