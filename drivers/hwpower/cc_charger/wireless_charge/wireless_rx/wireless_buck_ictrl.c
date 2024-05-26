// SPDX-License-Identifier: GPL-2.0
/*
 * wireless_buck_ictrl.c
 *
 * current control of wireless buck charging
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

#include <chipset_common/hwpower/wireless_charge/wireless_buck_ictrl.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_ic_intf.h>
#include <huawei_platform/hwpower/wireless_charge/wireless_rx_platform.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <linux/of.h>
#include <linux/slab.h>

#define HWLOG_TAG wireless_buck_ictrl
HWLOG_REGIST();

#define WLRX_BUCK_ICTRL_CFG_ROW        15
#define WLRX_BUCK_ICTRL_CFG_COL        3

#define WLRX_BUCK_DFLT_IIN             100 /* mA */

struct wlrx_ictrl_para {
	int imin;
	int imax;
	int iset;
};

struct wlrx_ictrl_dts {
	int icfg_level;
	struct wlrx_ictrl_para *icfg;
};

struct wlrx_ictrl_dev {
	struct wlrx_ictrl_dts *dts;
	int iin_charger;
	int iin_step;
	int iin_delay;
	int iin_now;
	bool iset_commplete;
};

static struct wlrx_ictrl_dev *g_rx_ictrl_di[WLTRX_DRV_MAX];

static struct wlrx_ictrl_dev *wlrx_ictrl_get_di(unsigned int drv_type)
{
	if (!wltrx_is_drv_type_valid(drv_type)) {
		hwlog_err("get_di: drv_type=%u err\n", drv_type);
		return NULL;
	}

	return g_rx_ictrl_di[drv_type];
}

static void wlrx_buck_set_iin_step_by_step(struct wlrx_ictrl_dev *di, int iin_ma)
{
	int ret;
	int iin_tmp = di->iin_now;

	di->iset_commplete = false;
	if (iin_ma > di->iin_now) {
		do {
			ret = wlrx_buck_set_dev_iin(iin_tmp);
			if (ret)
				hwlog_err("set_iin_step_by_step: failed\n");
			di->iin_now = iin_tmp;
			iin_tmp += di->iin_step;
			if (!di->iset_commplete)
				(void)power_msleep(di->iin_delay, 0, NULL);
		} while ((iin_tmp < iin_ma) && !di->iset_commplete &&
			di->iin_delay && di->iin_step);
	}
	if (iin_ma < di->iin_now) {
		do {
			ret = wlrx_buck_set_dev_iin(iin_tmp);
			if (ret)
				hwlog_err("set_iin_step_by_step: failed\n");
			di->iin_now = iin_tmp;
			iin_tmp -= di->iin_step;
			if (!di->iset_commplete)
				(void)power_msleep(di->iin_delay, 0, NULL);
		} while ((iin_tmp > iin_ma) && !di->iset_commplete &&
			di->iin_delay && di->iin_step);
	}
	if (!di->iset_commplete) {
		di->iin_now = iin_ma;
		ret = wlrx_buck_set_dev_iin(iin_ma);
		if (ret)
			hwlog_err("set_iin_step_by_step: failed\n");
		di->iset_commplete = true;
	}
}

static void wlrx_buck_set_iin(struct wlrx_ictrl_dev *di, int iin_ma)
{
	if ((iin_ma <= 0) || (di->iin_delay <= 0) || (di->iin_step <= 0)) {
		hwlog_err("set_iin: iin=%d delay=%d step=%d\n",
			iin_ma, di->iin_delay, di->iin_step);
		return;
	}

	if (!di->iset_commplete) {
		di->iset_commplete = true;
		/* delay double time for completion */
		(void)power_msleep(2 * di->iin_delay, 0, NULL);
	}
	wlrx_buck_set_iin_step_by_step(di, iin_ma);
}

void wlrx_buck_set_iin_for_rx(unsigned int drv_type, int iin_ma)
{
	struct wlrx_ictrl_dev *di = wlrx_ictrl_get_di(drv_type);

	if (!di)
		return;

	if ((di->iin_charger > 0) && (iin_ma > di->iin_charger))
		iin_ma = di->iin_charger;

	wlrx_buck_set_iin(di, iin_ma);
}

void wlrx_buck_set_iin_for_charger(unsigned int drv_type, int iin_ma)
{
	int rx_ilim;
	struct wlrx_ictrl_dev *di = wlrx_ictrl_get_di(drv_type);

	if (!di)
		return;

	di->iin_charger = iin_ma;
	rx_ilim = wireless_charge_get_rx_iout_limit();
	iin_ma = min(di->iin_charger, rx_ilim);
	wlrx_buck_set_iin(di, iin_ma);
}

int wlrx_buck_get_iset(unsigned int drv_type)
{
	struct wlrx_ictrl_dev *di = wlrx_ictrl_get_di(drv_type);

	if (!di)
		return 0;

	return di->iin_now;
}

void wlrx_buck_set_iin_prop(unsigned int drv_type, int iin_step, int iin_delay, int iset)
{
	struct wlrx_ictrl_dev *di = wlrx_ictrl_get_di(drv_type);

	if (!di)
		return;

	di->iin_delay = iin_delay;
	di->iin_step = iin_step;
	di->iin_now = WLRX_BUCK_DFLT_IIN;

	if ((iset > 0) && ((iin_step <= 0) || (iin_delay <= 0)))
		wlrx_buck_set_dev_iin(iset);
	hwlog_info("[set_iin_prop] delay=%d step=%d iset=%d\n",
		di->iin_delay, di->iin_step, iset);
}

void wlrx_buck_update_ictrl_para(unsigned int drv_type, int *iset)
{
	int i;
	int rx_iavg = 0;
	struct wlrx_ictrl_dev *di = wlrx_ictrl_get_di(drv_type);

	if (!di || !iset)
		return;

	wlrx_ic_get_iavg(wltrx_get_dflt_ic_type(drv_type), &rx_iavg);
	for (i = 0; i < di->dts->icfg_level; i++) {
		if ((rx_iavg >= di->dts->icfg[i].imin) &&
			(rx_iavg < di->dts->icfg[i].imax)) {
			*iset = di->dts->icfg[i].iset;
			break;
		}
	}
}

static void wlrx_buck_ictrl_parse_icfg(const struct device_node *np, struct wlrx_ictrl_dts *dts)
{
	int i, len, level;

	len = power_dts_read_u32_count(power_dts_tag(HWLOG_TAG), np,
		"rx_iout_ctrl_para", WLRX_BUCK_ICTRL_CFG_ROW, WLRX_BUCK_ICTRL_CFG_COL);
	if (len <= 0)
		return;

	level = len / WLRX_BUCK_ICTRL_CFG_COL;
	dts->icfg = kcalloc(level, sizeof(*dts->icfg), GFP_KERNEL);
	if (!dts->icfg)
		return;

	if (power_dts_read_u32_array(power_dts_tag(HWLOG_TAG), np,
		"rx_iout_ctrl_para", (u32 *)dts->icfg, len)) {
		kfree(dts->icfg);
		return;
	}

	dts->icfg_level = level;
	for (i = 0; i < dts->icfg_level; i++)
		hwlog_info("[ictrl_cfg][%d] imin:%-4d imax:%-4d iset:%-4d\n",
			i, dts->icfg[i].imin, dts->icfg[i].imax, dts->icfg[i].iset);
}

static int wlrx_buck_ictrl_parse_dts(const struct device_node *np, struct wlrx_ictrl_dts **dts)
{
	*dts = kzalloc(sizeof(**dts), GFP_KERNEL);
	if (!*dts)
		return -ENOMEM;

	wlrx_buck_ictrl_parse_icfg(np, *dts);
	return 0;
}

static void wlrx_buck_ictrl_kfree_dev(struct wlrx_ictrl_dev *di)
{
	if (!di)
		return;

	if (di->dts) {
		kfree(di->dts->icfg);
		kfree(di->dts);
	}

	kfree(di);
}

int wlrx_buck_ictrl_init(unsigned int drv_type, struct device *dev)
{
	int ret;
	struct wlrx_ictrl_dev *di = NULL;

	if (!dev || !wltrx_is_drv_type_valid(drv_type))
		return -ENODEV;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	ret = wlrx_buck_ictrl_parse_dts(dev->of_node, &di->dts);
	if (ret)
		goto exit;

	g_rx_ictrl_di[drv_type] = di;
	return 0;

exit:
	wlrx_buck_ictrl_kfree_dev(di);
	return ret;
}

void wlrx_buck_ictrl_deinit(unsigned int drv_type)
{
	if (!wltrx_is_drv_type_valid(drv_type))
		return;

	wlrx_buck_ictrl_kfree_dev(g_rx_ictrl_di[drv_type]);
	g_rx_ictrl_di[drv_type] = NULL;
}
