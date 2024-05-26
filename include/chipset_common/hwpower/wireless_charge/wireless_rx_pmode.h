/* SPDX-License-Identifier: GPL-2.0 */
/*
 * wireless_rx_pmode.h
 *
 * power mode for wireless charging
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
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

#ifndef _WIRELESS_RX_PMODE_H_
#define _WIRELESS_RX_PMODE_H_

#include <chipset_common/hwpower/wireless_charge/wireless_trx_intf.h>
#include <linux/slab.h>

#define WLRX_PMODE_CFG_ROW           8
#define WLRX_PMODE_CFG_COL           13

/* attention: mode type should not be modified */
#define WLRX_SUPP_PMODE_BUCK         BIT(0)
#define WLRX_SUPP_PMODE_SC2          BIT(1)
#define WLRX_SUPP_PMODE_SC4          BIT(2)
#define WLRX_SUPP_PMODE_ALL          0xff

enum wlrx_pmode_judge_type {
	WLRX_PMODE_QUICK_JUDGE = 0, /* quick icon display */
	WLRX_PMODE_NORMAL_JUDGE, /* recorecting icon display */
	WLRX_PMODE_FINAL_JUDGE, /* judging power mode */
	WLDC_PMODE_FINAL_JUDGE, /* wireless direct charging */
};

struct wlrx_pmode {
	const char *name;
	int vtx_min;
	int itx_min;
	int vtx; /* ctrl_para */
	int vrx; /* ctrl_para */
	int irx; /* ctrl_para */
	int vrect_lth;
	int tbatt;
	int cable; /* cable detect type */
	int auth; /* authenticate type */
	unsigned int icon;
	int timeout;
	int expect_mode;
};

struct wlrx_vmode {
	u32 id;
	int vtx;
};

#ifdef CONFIG_WIRELESS_CHARGER
struct wlrx_pmode *wlrx_pmode_get_pcfg_by_pid(unsigned int drv_type, int pid);
struct wlrx_pmode *wlrx_pmode_get_curr_pcfg(unsigned int drv_type);
int wlrx_pmode_get_pcfg_level(unsigned int drv_type);
int wlrx_pmode_get_pid_by_name(unsigned int drv_type, const char *mode_name);
int wlrx_pmode_get_exp_pid(unsigned int drv_type, int pid);
int wlrx_pmode_get_curr_pid(unsigned int drv_type);
void wlrx_pmode_set_curr_pid(unsigned int drv_type, int pid);
bool wlrx_pmode_is_pid_valid(unsigned int drv_type, int pid);
bool wlrx_pmode_judge(unsigned int drv_type, int pid, unsigned int judge_type);
#else
static inline struct wlrx_pmode *wlrx_pmode_get_pcfg_by_pid(unsigned int drv_type, int pid)
{
	return NULL;
}

static inline struct wlrx_pmode *wlrx_pmode_get_curr_pcfg(unsigned int drv_type)
{
	return NULL;
}

static inline int wlrx_pmode_get_pcfg_level(unsigned int drv_type)
{
	return 0;
}

static inline int wlrx_pmode_get_pid_by_name(unsigned int drv_type, const char *mode_name)
{
	return 0;
}

static inline int wlrx_pmode_get_exp_pid(unsigned int drv_type, int pid)
{
	return 0;
}

static inline int wlrx_pmode_get_curr_pid(unsigned int drv_type)
{
	return 0;
}

static inline void wlrx_pmode_set_curr_pid(unsigned int drv_type, int pid)
{
}

static inline bool wlrx_pmode_is_pid_valid(unsigned int drv_type, int pid)
{
	return false;
}

static inline bool wlrx_pmode_judge(unsigned int drv_type, int pid, unsigned int judge_type)
{
	return false;
}
#endif /* CONFIG_WIRELESS_CHARGER */

#endif /* _WIRELESS_RX_PMODE_H_ */
