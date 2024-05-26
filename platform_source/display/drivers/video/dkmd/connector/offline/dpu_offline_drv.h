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

#ifndef DPU_OFFLINE_DRV_H
#define DPU_OFFLINE_DRV_H

#include <linux/types.h>
#include <linux/wait.h>
#include <linux/platform_device.h>

#include "peri/dkmd_peri.h"

#define DEV_NAME_OFFLINE "gfx_offline"
#define DTS_COMP_OFFLINE "dkmd,gfx_offline"

struct offline_private {
	struct platform_device *pdev;

	struct dkmd_connector_info connector_info;
};

#endif
