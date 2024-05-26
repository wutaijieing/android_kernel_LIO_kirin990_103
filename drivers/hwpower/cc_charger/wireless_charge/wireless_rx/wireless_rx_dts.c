// SPDX-License-Identifier: GPL-2.0
/*
 * wireless_rx_dts.c
 *
 * parse dts for wireless charging
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

#include <linux/slab.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_dts.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_pctrl.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_pmode.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_interfere.h>

#define HWLOG_TAG wireless_rx_dts
HWLOG_REGIST();

/* for vmode_para */
#define WLRX_VMODE_CFG_ROW             5
#define WLRX_VMODE_CFG_COL             2

static struct wlrx_dts *g_wlrx_dts;

struct wlrx_dts *wlrx_get_dts(void)
{
	return g_wlrx_dts;
}

static int wlrx_parse_basic_cfg(const struct device_node *np, struct wlrx_dts *dts)
{
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"hvc_need_5vbst", (u32 *)&dts->hvc_need_5vbst, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"bst5v_ignore_vbus_only", (u32 *)&dts->bst5v_ignore_vbus_only, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"rx_vout_err_ratio", (u32 *)&dts->rx_vout_err_ratio, 81); /* default_ratio:81% */
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"rx_iout_min", (u32 *)&dts->rx_imin, 150); /* default_imin:150mA */
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"rx_iout_step", (u32 *)&dts->rx_istep, 100); /* default_istep:100mA */
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"pmax", &dts->pmax, 20); /* default_pmax:20*2=40w */
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"sample_delay_time", &dts->sample_delay_time, 3000); /* default_delay:3s */

	return 0;
}

static int wlrx_parse_vmode_cfg(const struct device_node *np, struct wlrx_dts *dts)
{
	int i, len, size;

	len = power_dts_read_u32_count(power_dts_tag(HWLOG_TAG), np,
		"volt_mode", WLRX_VMODE_CFG_ROW, WLRX_VMODE_CFG_COL);
	if (len <= 0)
		return -EINVAL;

	size = sizeof(*dts->vmode_cfg) * (len / WLRX_VMODE_CFG_COL);
	dts->vmode_cfg = kzalloc(size, GFP_KERNEL);
	if (!dts->vmode_cfg)
		return -ENOMEM;

	if (power_dts_read_u32_array(power_dts_tag(HWLOG_TAG), np,
		"volt_mode", (u32 *)dts->vmode_cfg, len))
		return -EINVAL;

	dts->vmode_cfg_level = len / WLRX_VMODE_CFG_COL;
	for (i = 0; i < dts->vmode_cfg_level; i++)
		hwlog_info("vmode[%d], id: %u vtx: %-5d\n",
			i, dts->vmode_cfg[i].id, dts->vmode_cfg[i].vtx);

	return 0;
}

void wlrx_kfree_dts(void)
{
	if (!g_wlrx_dts)
		return;

	kfree(g_wlrx_dts->vmode_cfg);
	kfree(g_wlrx_dts);
	g_wlrx_dts = NULL;
}

int wlrx_parse_dts(const struct device_node *np)
{
	int ret;
	struct wlrx_dts *dts = NULL;

	if (!np)
		return -EINVAL;

	dts = kzalloc(sizeof(*dts), GFP_KERNEL);
	if (!dts)
		return -ENOMEM;

	g_wlrx_dts = dts;
	ret = wlrx_parse_basic_cfg(np, dts);
	if (ret)
		goto exit;

	ret = wlrx_parse_vmode_cfg(np, dts);
	if (ret)
		goto exit;

	return 0;

exit:
	wlrx_kfree_dts();
	return -EINVAL;
}
