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

#include <drm/drm_of.h>
#include "dpu_drm.h"

static int compare_of(struct device *dev, void *data)
{
	return dev->of_node == data;
}

static int dkmd_drm_bind(struct device *dev)
{
	dpu_pr_info("bind %s for drm!", dev->init_name);

	return 0;
}

static void dkmd_drm_unbind(struct device *dev)
{
	dpu_pr_info("unbind %s for drm!", dev->init_name);
}

static const struct component_master_ops dkmd_drm_ops = {
	.bind = dkmd_drm_bind,
	.unbind = dkmd_drm_unbind,
};

int32_t drm_device_register(struct composer *comp)
{
	struct component_match *match = NULL;
	struct device_node *remote = comp->base.peri_device->dev.of_node;

	dpu_pr_info("register %s for drm!", comp->base.peri_device->name);

	drm_of_component_match_add(&comp->base.peri_device->dev, &match, compare_of, remote);

	return component_master_add_with_match(&comp->base.peri_device->dev, &dkmd_drm_ops, match);
}

void drm_device_unregister(struct composer *comp)
{
	component_master_del(&comp->base.peri_device->dev, &dkmd_drm_ops);
}