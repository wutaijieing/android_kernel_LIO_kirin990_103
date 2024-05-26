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

#ifndef HISI_MIPI_CORE_H
#define HISI_MIPI_CORE_H

#include <linux/platform_device.h>
#include <linux/types.h>

#include "hisi_connector_utils.h"

typedef void (*mipi_init_component_cb)(struct hisi_connector_device *mipi_dev, struct hisi_connector_component *out);

struct hisi_mipi_info {
	struct hisi_connector_info base;

	/* other mipi information */
};

void hisi_mipi_init_component(struct hisi_connector_device *connector);
void hisi_mipi_init_slaver(struct hisi_connector_device *mipi_dev, struct hisi_connector_component *out_slaver);


#endif /* HISI_MIPI_CORE_H */
