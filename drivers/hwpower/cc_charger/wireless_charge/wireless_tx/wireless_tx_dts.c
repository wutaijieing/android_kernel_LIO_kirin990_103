// SPDX-License-Identifier: GPL-2.0
/*
 * wireless_tx_dts.c
 *
 * parse dts for wireless reverse charging
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

#include <linux/slab.h>
#include <chipset_common/hwpower/wireless_charge/wireless_tx_dts.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/wireless_charge/wireless_tx_pwr_ctrl.h>

#define HWLOG_TAG wireless_tx_dts
HWLOG_REGIST();

/* for basic config */
#define WLTX_HI_PWR_SOC_LTH             90
#define WLTX_HI_PWR_TH_CFG_LEN          4
#define WLTX_HI_PWR_VTH                 5000 /* mV */
#define WLTX_HI_PWR2_VTH                9000 /* mV */
#define WLTX_HI_PWR_ITH                 4000 /* mA */

static struct wltx_dts *g_wltx_dts;

struct wltx_dts *wltx_get_dts(void)
{
	return g_wltx_dts;
}

static int wltx_parse_basic_cfg(const struct device_node *np, struct wltx_dts *dts)
{
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"pwr_type", &dts->pwr_type, WL_TX_PWR_VBUS_OTG);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"vbus_change_type", &dts->vbus_change_type,
		WLTX_VBUS_CHANGED_BY_WIRED_CHSW);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"tx_high_pwr_soc", &dts->high_pwr_soc, WLTX_HI_PWR_SOC_LTH);

	return 0;
}

static int wltx_parse_high_pwr_th(const struct device_node *np, struct wltx_dts *dts)
{
	int size, len;

	size = sizeof(*dts->high_pwr_th_cfg);
	dts->high_pwr_th_cfg = kzalloc(size, GFP_KERNEL);
	if (!dts->high_pwr_th_cfg)
		return -ENOMEM;

	len = of_property_count_u32_elems(np, "wltx_hi_pwr_th");
	if (len != WLTX_HI_PWR_TH_CFG_LEN)
		goto parse_high_pwr_th_err;

	if (power_dts_read_u32_array(power_dts_tag(HWLOG_TAG), np,
		"wltx_hi_pwr_th", (u32 *)dts->high_pwr_th_cfg, len))
		goto parse_high_pwr_th_err;

	goto print_para;

parse_high_pwr_th_err:
	dts->high_pwr_th_cfg->hi_pwr_vth = WLTX_HI_PWR_VTH;
	dts->high_pwr_th_cfg->hi_pwr_ith = WLTX_HI_PWR_ITH;
	dts->high_pwr_th_cfg->hi_pwr2_vth = WLTX_HI_PWR2_VTH;
	dts->high_pwr_th_cfg->hi_pwr2_ith = WLTX_HI_PWR_ITH;


print_para:
	hwlog_info("[parse_high_pwr_th] hi_pwr_vth:%d hi_pwr_ith:%d hi_pwr2_vth:%d hi_pwr2_ith:%d\n",
		dts->high_pwr_th_cfg->hi_pwr_vth, dts->high_pwr_th_cfg->hi_pwr_ith,
		dts->high_pwr_th_cfg->hi_pwr2_vth, dts->high_pwr_th_cfg->hi_pwr2_ith);
	return 0;
}

void wltx_kfree_dts(void)
{
	if (!g_wltx_dts)
		return;

	kfree(g_wltx_dts);
	g_wltx_dts = NULL;
}

int wltx_parse_dts(const struct device_node *np)
{
	int ret;
	struct wltx_dts *dts = NULL;

	if (!np)
		return -EINVAL;

	dts = kzalloc(sizeof(*dts), GFP_KERNEL);
	if (!dts)
		return -ENOMEM;

	g_wltx_dts = dts;
	ret = wltx_parse_basic_cfg(np, dts);
	if (ret)
		goto exit;
	ret = wltx_parse_high_pwr_th(np, dts);
	if (ret)
		goto exit;

	return 0;

exit:
	wltx_kfree_dts();
	return -EINVAL;
}
