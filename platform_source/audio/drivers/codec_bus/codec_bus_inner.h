/*
 * codec_bus_innner.h -- codec bus inner header
 *
 * Copyright (c) 2012-2021 Huawei Technologies Co., Ltd.
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
#ifndef __CODEC_BUS_INNER_H__
#define __CODEC_BUS_INNER_H__

#include <linux/types.h>

struct codec_bus_ops {
	int (*runtime_get)(void);
	void (*runtime_put)(void);
	int (*switch_framer)(enum framer_type framer_type);
	int (*enum_device)(void);
	enum framer_type (*get_framer_type)(void);
	int (*activate)(const char* scene_name, struct scene_param *params);
	int (*deactivate)(const char* scene_name, struct scene_param *params);
};

bool is_pm_runtime_support(void);
bool is_bus_active(void);
void set_bus_active(bool active);
void register_ops(struct codec_bus_ops *bus_ops);
int get_irq_id(void);
#endif
