/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include "dkmd_blpwm.h"
#include "dkmd_blpwm_api.h"
#include "dkmd_log.h"

#define DKMD_BLPWM_DEVICE_MAX 8
static struct blpwm_device {
	const char *dev_name;
	struct blpwm_dev_ops *dev_ops;
} g_blpwm_device[DKMD_BLPWM_DEVICE_MAX];

uint32_t g_blpwm_dev_count = 0;

static struct blpwm_dev_ops *get_blpwm_dev_ops(const char *dev_name)
{
	uint32_t i;
	struct blpwm_device *dev = NULL;

	if (dev_name == NULL)
		return NULL;

	for (i = 0; i < g_blpwm_dev_count; i++) {
		dev = &g_blpwm_device[i];
		if (dev->dev_name == NULL)
			continue;

		if (strcmp(dev->dev_name, dev_name) == 0)
			return dev->dev_ops;
	}

	return NULL;
}

static int dkmd_blpwm_on(void *data)
{
	struct dkmd_bl_ctrl *bl_ctrl = NULL;
	struct dkmd_bl_ops *bl_ops = NULL;

	if (!data)
		return 0;

	bl_ctrl = (struct dkmd_bl_ctrl *)data;
	bl_ops = &bl_ctrl->bl_ops;

	dpu_pr_debug("blpwm device have not been inited, need re-init");

	dkmd_blpwm_init(bl_ctrl);
	if (bl_ops->set_backlight && bl_ops->on)
		return bl_ops->on(data);

	return 0;
}

static int dkmd_blpwm_set_backlight(struct dkmd_bl_ctrl *bl_ctrl, uint32_t bkl_lvl)
{
	struct blpwm_dev_ops *dev_ops = NULL;

	if (unlikely(bl_ctrl == NULL))
		return -1;

	if (unlikely(bl_ctrl->bl_dev_name == NULL)) {
		dpu_pr_err("bl dev name is null");
		return -1;
	}

	dev_ops = get_blpwm_dev_ops(bl_ctrl->bl_dev_name);
	if (unlikely(dev_ops == NULL)) {
		dpu_pr_err("bl_dev_name %s dev_ops is null", bl_ctrl->bl_dev_name);
		return -1;
	}

	if (likely(dev_ops->set_backlight))
		return dev_ops->set_backlight(NULL, bkl_lvl);

	return 0;
}

void dkmd_blpwm_init(struct dkmd_bl_ctrl *bl_ctrl)
{
	struct blpwm_dev_ops *dev_ops = NULL;
	struct dkmd_bl_ops *bl_ops = NULL;

	if (!bl_ctrl) {
		dpu_pr_err("[backlight] bl_ops is NULL\n");
		return;
	}

	bl_ops = &bl_ctrl->bl_ops;
	dev_ops = get_blpwm_dev_ops(bl_ctrl->bl_dev_name);
	if (dev_ops == NULL) {
		bl_ops->on = dkmd_blpwm_on;
		bl_ops->off = NULL;
		bl_ops->set_backlight = NULL;
		bl_ops->deinit = NULL;

		dpu_pr_debug("blpwm device have not been registed yet");
		return;
	}

	bl_ops->on = dev_ops->on;
	bl_ops->off = NULL;
	bl_ops->set_backlight = dkmd_blpwm_set_backlight;
	bl_ops->deinit = NULL;

	bl_ctrl->private_data = bl_ctrl;
	dpu_pr_info("blpwm device[%s] have registed ok", bl_ctrl->bl_dev_name);
}

void dkmd_blpwm_register_bl_device(const char *dev_name, struct blpwm_dev_ops *ops)
{
	if (ops == NULL || dev_name == NULL) {
		dpu_pr_err("ops or dev_name is null");
		return;
	}

	if (g_blpwm_dev_count >= DKMD_BLPWM_DEVICE_MAX) {
		dpu_pr_err("register bl device %s fail, index is overflow", dev_name);
		return;
	}

	g_blpwm_device[g_blpwm_dev_count].dev_name = dev_name;
	g_blpwm_device[g_blpwm_dev_count].dev_ops = ops;
	++g_blpwm_dev_count;
}
EXPORT_SYMBOL(dkmd_blpwm_register_bl_device);