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

#ifndef DKMD_COMP_H
#define DKMD_COMP_H

#include "dkmd_object.h"
#include "chrdev/dkmd_sysfs.h"

#define DEVICE_COMP_MAX_COUNT (8)

struct dkmd_base_frame;
struct dkmd_network;
struct product_config;

struct composer {
	struct dkmd_object_info base;
	uint32_t index;
	uint32_t comp_frame_index;
	uint32_t fastboot_display_enabled;

	bool power_on; /* sizeof(bool) = 1 */
	uint8_t reserved[3];
	struct semaphore blank_sem;
	void *device_data;

	int32_t (*set_fastboot)(struct composer *comp);
	int32_t (*on)(struct composer *comp);
	int32_t (*off)(struct composer *comp);

	int32_t (*create_fence)(struct composer *comp);
	int32_t (*present)(struct composer *comp, void *frame);

	int32_t (*get_product_config)(struct composer *comp, struct product_config *out_config);
	int32_t (*get_sysfs_attrs)(struct composer *comp, struct dkmd_attr **out_attr);
};

#endif
