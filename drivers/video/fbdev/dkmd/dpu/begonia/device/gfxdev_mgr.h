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

#ifndef DPU_GFX_DEVICE_MGR_H
#define DPU_GFX_DEVICE_MGR_H

#include <linux/types.h>
#include "dkmd_comp.h"

enum {
	FBDEV_ARCH,
	DRMDEV_ARCH
};

void gfxdev_blank_power_on(struct composer *comp);
void gfxdev_blank_power_off(struct composer *comp);
int32_t device_mgr_create_gfxdev(struct composer *comp);
void device_mgr_destroy_gfxdev(struct composer *comp);
void device_mgr_shutdown_gfxdev(struct composer *comp);
void device_mgr_register_comp(struct composer *comp);
void device_mgr_primary_frame_refresh(struct composer *comp, char *trigger);

struct composer *get_comp_from_device(struct device *dev);

#endif