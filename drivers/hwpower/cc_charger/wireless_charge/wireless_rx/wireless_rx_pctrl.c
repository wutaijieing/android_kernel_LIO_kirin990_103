// SPDX-License-Identifier: GPL-2.0
/*
 * wireless_rx_pctrl.c
 *
 * power(vtx,vrx,irx) control of wireless charging
 *
 * Copyright (c) 2020-2021 Huawei Technologies Co., Ltd.
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

#include <chipset_common/hwpower/wireless_charge/wireless_rx_pctrl.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_module.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_plim.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_interfere.h>
#include <huawei_platform/hwpower/common_module/power_platform.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <linux/kernel.h>
#include <linux/of.h>

#define HWLOG_TAG wireless_rx_pctrl
HWLOG_REGIST();

/* for product_para */
#define WLRX_PCTRL_PRODUCT_CFG_LEN           3

/* for segment_para */
#define WLRX_PCTRL_SOC_CFG_ROW               5
#define WLRX_PCTRL_SOC_CFG_COL               5

struct wlrx_soc_para {
	int soc_min;
	int soc_max;
	int vtx;
	int vrx;
	int irx;
};

struct wlrx_pctrl_dts {
	struct wlrx_pctrl *product_pcfg;
	int soc_pcfg_level;
	struct wlrx_soc_para *soc_pcfg;
	int product_max_pwr;
};

struct wlrx_pctrl_dev {
	struct wlrx_pctrl_dts *dts;
};

static struct wlrx_pctrl_dev *g_rx_pctrl_di[WLTRX_DRV_MAX];

static struct wlrx_pctrl_dev *wlrx_pctrl_get_di(unsigned int drv_type)
{
	if (!wltrx_is_drv_type_valid(drv_type)) {
		hwlog_err("get_di: drv_type=%u err\n", drv_type);
		return NULL;
	}

	return g_rx_pctrl_di[drv_type];
}

struct wlrx_pctrl *wlrx_pctrl_get_product_cfg(unsigned int drv_type)
{
	struct wlrx_pctrl_dev *di = wlrx_pctrl_get_di(drv_type);

	if (!di || !di->dts)
		return NULL;

	return di->dts->product_pcfg;
}

static void wlrx_pctrl_update_soc_para(struct wlrx_pctrl_dev *di, struct wlrx_pctrl *pctrl)
{
	int i, soc;

	soc = power_platform_get_battery_ui_capacity();
	for (i = 0; i < di->dts->soc_pcfg_level; i++) {
		if ((soc < di->dts->soc_pcfg[i].soc_min) ||
			(soc > di->dts->soc_pcfg[i].soc_max))
			continue;
		pctrl->vtx = min(di->dts->soc_pcfg[i].vtx, pctrl->vtx);
		pctrl->vrx = min(di->dts->soc_pcfg[i].vrx, pctrl->vrx);
		pctrl->irx = min(di->dts->soc_pcfg[i].irx, pctrl->irx);
		break;
	}
}

static void wlrx_pctrl_update_product_para(struct wlrx_pctrl_dev *di, struct wlrx_pctrl *pctrl)
{
	pctrl->vtx = di->dts->product_pcfg->vtx;
	pctrl->vrx = di->dts->product_pcfg->vrx;
	pctrl->irx = di->dts->product_pcfg->irx;
}

void wlrx_pctrl_update_para(unsigned int drv_type, struct wlrx_pctrl *pctrl)
{
	struct wlrx_pctrl_dev *di = wlrx_pctrl_get_di(drv_type);

	if (!di || !pctrl)
		return;

	wlrx_pctrl_update_product_para(di, pctrl);
	wlrx_pctrl_update_soc_para(di, pctrl);
	wlrx_plim_update_pctrl(drv_type, pctrl);
	wlrx_intfr_update_pctrl(drv_type, pctrl);
}

int wlrx_pctrl_get_product_pmax(unsigned int drv_type)
{
	struct wlrx_pctrl_dev *di = wlrx_pctrl_get_di(drv_type);

	if (!di || !di->dts)
		return 0;

	return di->dts->product_max_pwr;
}

static void wlrx_pctrl_parse_soc_cfg(const struct device_node *np, struct wlrx_pctrl_dts *dts)
{
	int i, len;

	len = power_dts_read_u32_count(power_dts_tag(HWLOG_TAG), np,
		"segment_para", WLRX_PCTRL_SOC_CFG_ROW, WLRX_PCTRL_SOC_CFG_COL);
	if (len <= 0)
		return;

	dts->soc_pcfg = kcalloc(len / WLRX_PCTRL_SOC_CFG_COL, sizeof(*dts->soc_pcfg), GFP_KERNEL);
	if (!dts->soc_pcfg)
		return;

	if (power_dts_read_u32_array(power_dts_tag(HWLOG_TAG), np,
		"segment_para", (u32 *)dts->soc_pcfg, len)) {
		kfree(dts->soc_pcfg);
		return;
	}

	dts->soc_pcfg_level = len / WLRX_PCTRL_SOC_CFG_COL;
	for (i = 0; i < dts->soc_pcfg_level; i++)
		hwlog_info("[soc_pcfg][%d] soc_min:%-3d soc_max:%-3d vtx:%-5d vrx:%-5d irx:%-4d\n",
			i, dts->soc_pcfg[i].soc_min, dts->soc_pcfg[i].soc_max,
			dts->soc_pcfg[i].vtx, dts->soc_pcfg[i].vrx, dts->soc_pcfg[i].irx);
}

static int wlrx_pctrl_parse_product_cfg(const struct device_node *np, struct wlrx_pctrl_dts *dts)
{
	dts->product_pcfg = kzalloc(sizeof(*dts->product_pcfg), GFP_KERNEL);
	if (!dts->product_pcfg)
		return -ENOMEM;

	if (power_dts_read_u32_array(power_dts_tag(HWLOG_TAG), np,
		"product_para", (u32 *)dts->product_pcfg, WLRX_PCTRL_PRODUCT_CFG_LEN))
		return -EINVAL;

	hwlog_info("[product_pcfg] vtx_max:%dmV vrx_max:%dmV irx_max:%dmA\n",
		dts->product_pcfg->vtx, dts->product_pcfg->vrx, dts->product_pcfg->irx);
	return 0;
}

static int wlrx_pctrl_parse_dts(const struct device_node *np, struct wlrx_pctrl_dts **dts)
{
	*dts = kzalloc(sizeof(**dts), GFP_KERNEL);
	if (!*dts)
		return -ENOMEM;

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"product_max_pwr", (u32 *)&(*dts)->product_max_pwr, 0);
	wlrx_pctrl_parse_soc_cfg(np, *dts);

	return wlrx_pctrl_parse_product_cfg(np, *dts);
}

static void wlrx_pctrl_kfree_dev(struct wlrx_pctrl_dev *di)
{
	if (!di)
		return;

	if (di->dts) {
		kfree(di->dts->product_pcfg);
		kfree(di->dts->soc_pcfg);
		kfree(di->dts);
	}

	kfree(di);
}

static int wlrx_pctrl_submodule_init(unsigned int drv_type, struct device *dev)
{
	return wlrx_plim_init(drv_type, dev) || wlrx_intfr_init(drv_type, dev);
}

static void wlrx_pctrl_submodule_deinit(unsigned int drv_type)
{
	wlrx_plim_deinit(drv_type);
	wlrx_intfr_deinit(drv_type);
}

int wlrx_pctrl_init(unsigned int drv_type, struct device *dev)
{
	int ret;
	struct wlrx_pctrl_dev *di = NULL;

	if (!dev || !wltrx_is_drv_type_valid(drv_type))
		return -ENODEV;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	ret = wlrx_pctrl_parse_dts(dev->of_node, &di->dts);
	if (ret)
		goto exit;
	ret = wlrx_pctrl_submodule_init(drv_type, dev);
	if (ret)
		goto exit;

	g_rx_pctrl_di[drv_type] = di;
	return 0;

exit:
	wlrx_pctrl_kfree_dev(di);
	return ret;
}

void wlrx_pctrl_deinit(unsigned int drv_type)
{
	if (!wltrx_is_drv_type_valid(drv_type))
		return;

	wlrx_pctrl_submodule_deinit(drv_type);
	wlrx_pctrl_kfree_dev(g_rx_pctrl_di[drv_type]);
	g_rx_pctrl_di[drv_type] = NULL;
}
