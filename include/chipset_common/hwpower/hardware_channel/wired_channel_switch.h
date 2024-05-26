/* SPDX-License-Identifier: GPL-2.0 */
/*
 * wired_channel_switch.h
 *
 * wired channel switch
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

#ifndef _WIRED_CHANNEL_SWITCH_H_
#define _WIRED_CHANNEL_SWITCH_H_

#include <linux/errno.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <chipset_common/hwpower/hardware_channel/wired_channel_manager.h>

#define WIRED_CHANNEL_CUTOFF           1
#define WIRED_CHANNEL_RESTORE          0

#define WIRED_REVERSE_CHANNEL_CUTOFF   1
#define WIRED_REVERSE_CHANNEL_RESTORE  0

enum wired_channel_type {
	WIRED_CHANNEL_BEGIN,
	WIRED_CHANNEL_BUCK = WIRED_CHANNEL_BEGIN,
	WIRED_CHANNEL_LVC,
	WIRED_CHANNEL_LVC_MOS,
	WIRED_CHANNEL_SC,
	WIRED_CHANNEL_SC_AUX,
	WIRED_CHANNEL_SC4,
	WIRED_CHANNEL_ALL,
	WIRED_CHANNEL_END,
};

struct wired_chsw_device_ops {
	int (*set_wired_channel)(int channel_type, int state);
	int (*set_other_wired_channel)(int channel_type, int state);
	int (*get_wired_channel)(int channel_type);
	int (*set_wired_reverse_channel)(int state);
};

struct wired_chsw_dts {
	int wired_sw_dflt_on;
};

#ifdef CONFIG_WIRED_CHANNEL_SWITCH
extern const char *wired_chsw_get_channel_type_string(int type);
extern int wired_chsw_ops_register(struct wired_chsw_device_ops *ops);
extern int wired_chsw_set_wired_reverse_channel(int state);
extern int wired_chsw_set_wired_channel(int channel_type, int state);
extern int wired_chsw_set_other_wired_channel(int channel_type, int state);
extern int wired_chsw_get_wired_channel(int channel_type);
extern struct wired_chsw_dts *wired_chsw_get_dts(void);
#else
static inline const char *wired_chsw_get_channel_type_string(int type)
{
	return "illegal channel type";
}

static inline int wired_chsw_ops_register(struct wired_chsw_device_ops *ops)
{
	return -EPERM;
}

static inline int wired_chsw_set_wired_reverse_channel(int state)
{
	return -EPERM;
}

static inline int wired_chsw_set_wired_channel(int channel_type, int state)
{
	return 0;
}

static inline int wired_chsw_set_other_wired_channel(int channel_type, int state)
{
	return 0;
}

static inline int wired_chsw_get_wired_channel(int channel_type)
{
	return WIRED_CHANNEL_RESTORE;
}

static inline struct wired_chsw_dts *wired_chsw_get_dts(void)
{
	return NULL;
}
#endif /* CONFIG_WIRED_CHANNEL_SWITCH */

#endif /* _WIRED_CHANNEL_SWITCH_H_ */
