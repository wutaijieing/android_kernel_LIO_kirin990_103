/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef HISI_DP_CORE_H
#define HISI_DP_CORE_H

#include <linux/platform_device.h>
#include <linux/types.h>

#include "hisi_connector_utils.h"
#include "hisi_dp_struct.h"

struct hisi_dp_info {
	struct hisi_connector_info base;

	/* other dp information */
	struct dp_ctrl dp;
};

void hisi_dp_init_component(struct hisi_connector_device *connector);
void hisi_dp_init_slaver(struct hisi_connector_device *dp_dev, struct hisi_connector_component *out_slaver);

#endif /* HISI_DP_CORE_H */
