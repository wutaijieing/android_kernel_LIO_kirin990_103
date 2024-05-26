/* SPDX-License-Identifier: GPL-2.0 */
/*
 * battery_model_public.h
 *
 * huawei battery model information
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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

#ifndef _BATTERY_MODEL_PUBLIC_H_
#define _BATTERY_MODEL_PUBLIC_H_

#include <linux/errno.h>

struct bat_model_ops {
	int (*get_vbat_max)(void);
	int (*get_design_fcc)(void);
	int (*get_technology)(void);
	int (*is_removed)(void);
	const char *(*get_brand)(void);
};

enum {
	BAT_MODEL_BAT_CATHODE_TYPE_INVALID = 0,
	BAT_MODEL_BAT_CATHODE_TYPE_GRAPHITE,
	BAT_MODEL_BAT_CATHODE_TYPE_SILICON,
	BAT_MODEL_BAT_CATHODE_TYPE_MAX,
};

#ifdef CONFIG_HUAWEI_BATTERY_MODEL
int bat_model_register_ops(struct bat_model_ops *ops);
int bat_model_id_index(void);
const char *bat_model_name(void);
int bat_model_get_bat_cathode_type(void);
#else
static inline int bat_model_register_ops(struct bat_model_ops *ops)
{
	return -EPERM;
}

static inline int bat_model_id_index(void)
{
	return 0;
}

static inline const char *bat_model_name(void)
{
	return NULL;
}

static inline int bat_model_get_bat_cathode_type(void)
{
	return BAT_MODEL_BAT_CATHODE_TYPE_INVALID;
}

#endif /* CONFIG_HUAWEI_BATTERY_MODEL */

#endif /* _BATTERY_MODEL_PUBLIC_H_ */
