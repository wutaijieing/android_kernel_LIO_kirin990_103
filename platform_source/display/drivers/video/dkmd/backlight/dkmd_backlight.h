/** @file
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
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
#ifndef DKMD_BACKLIGHT_H
#define DKMD_BACKLIGHT_H

#include <linux/platform_device.h>
#include "dkmd_backlight_common.h"

#ifdef CONFIG_DKMD_BACKLIGHT
void dkmd_backlight_register_led(struct platform_device *pdev, struct dkmd_bl_ctrl *bl_ctrl);
void dkmd_backlight_unregister_led(struct dkmd_bl_ctrl *bl_ctrl);
void dkmd_backlight_init_bl_ctrl(uint32_t bl_type, struct dkmd_bl_ctrl *bl_ctrl);
void dkmd_backlight_deinit_bl_ctrl(struct dkmd_bl_ctrl *bl_ctrl);
void dkmd_backlight_update(struct dkmd_bl_ctrl *bl_ctrl, bool enforce);
void dkmd_backlight_cancel(struct dkmd_bl_ctrl *bl_ctrl);
#else
#define dkmd_backlight_register_led(pdev, bl_ctrl)
#define dkmd_backlight_unregister_led(bl_ctrl)
#define dkmd_backlight_init_bl_ctrl(bl_type, bl_ctrl)
#define dkmd_backlight_deinit_bl_ctrl(bl_ctrl)
#define dkmd_backlight_update(bl_ctrl, enforce)
#define dkmd_backlight_cancel(bl_ctrl)
#endif

#endif /* DKMD_BACKLIGHT_H */
