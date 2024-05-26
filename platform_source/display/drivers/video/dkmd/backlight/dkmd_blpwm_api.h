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

#ifndef __DKMD_BLPWM_API_H__
#define __DKMD_BLPWM_API_H__

struct blpwm_dev_ops {
	int (*set_backlight)(void *bl_ctrl, uint32_t bkl_lvl);
	int (*on)(void *data);
};

#ifdef CONFIG_DKMD_BACKLIGHT
void dkmd_blpwm_register_bl_device(const char *dev_name, struct blpwm_dev_ops *ops);
#else
#define dkmd_blpwm_register_bl_device(dev_name, ops)
#endif

#endif
