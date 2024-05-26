// SPDX-License-Identifier: GPL-2.0
/*
 * wireless_rx_ui.c
 *
 * ui handler for wireless charge
 *
 * Copyright (c) 2020-2022 Huawei Technologies Co., Ltd.
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

#include <chipset_common/hwpower/wireless_charge/wireless_rx_ui.h>
#include <chipset_common/hwpower/wireless_charge/wireless_trx_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_acc.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_pctrl.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_status.h>
#include <chipset_common/hwpower/protocol/wireless_protocol.h>
#include <chipset_common/hwpower/common_module/power_ui_ne.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <huawei_platform/hwpower/wireless_charge/wireless_rx_platform.h>

#ifdef CONFIG_HUAWEI_CHARGER_AP
#include <huawei_platform/power/huawei_charger_uevent.h>
#endif /* CONFIG_HUAWEI_CHARGER_AP */

#include <linux/kernel.h>
#include <linux/slab.h>

#define HWLOG_TAG wireless_rx_ui
HWLOG_REGIST();

struct wlrx_max_pwr_revise {
	int unit_low;
	int unit_high;
	int remainder;
	int carry;
};

/*
 * tx_max_pwr only supports powers of integer multiples of 5
 * if 0 <= pwr <= 2, takes 0
 * if 3 <= pwr <= 6, takes 5
 * if 7 <= pwr <= 9, carry takes 1
 */
static struct wlrx_max_pwr_revise g_revise_tbl[] = {
	{ 0, 2, 0, 0 },
	{ 3, 6, 5, 0 },
	{ 7, 9, 0, 1 },
};
static int g_exceptional_pwr_tbl[] = { 27000 };

struct wlrx_ui_icon_flag {
	bool normal_charge;
	bool fast_charge;
	bool super_charge;
};

struct wlrx_ui_dev {
	struct wlrx_ui_para *ui_para;
	unsigned int curr_icon_type;
	struct wlrx_ui_icon_flag icon_flag;
};

static struct wlrx_ui_dev *g_rx_ui_di[WLTRX_DRV_MAX];

static struct wlrx_ui_dev *wlrx_ui_get_di(unsigned int drv_type)
{
	if (!wltrx_is_drv_type_valid(drv_type)) {
		hwlog_err("get_di: drv_type=%u err\n", drv_type);
		return NULL;
	}

	return g_rx_ui_di[drv_type];
}

static bool wlrx_ui_is_exceptional_pwr(int tx_max_pwr_mv)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_exceptional_pwr_tbl); i++) {
		if (tx_max_pwr_mv == g_exceptional_pwr_tbl[i])
			return true;
	}

	return false;
}

static int wlrx_revise_ui_max_pwr(int tx_max_pwr_w)
{
	unsigned int i;
	int carry = 0;
	int remainder;
	int quotient;

	quotient = tx_max_pwr_w / POWER_BASE_DEC;
	remainder = tx_max_pwr_w % POWER_BASE_DEC;

	for (i = 0; i < ARRAY_SIZE(g_revise_tbl); i++) {
		if ((remainder >= g_revise_tbl[i].unit_low) &&
			(remainder <= g_revise_tbl[i].unit_high)) {
			remainder = g_revise_tbl[i].remainder;
			carry = g_revise_tbl[i].carry;
			break;
		}
	}

	return ((quotient + carry) * POWER_BASE_DEC + remainder) * POWER_MW_PER_W;
}

static void wlrx_ui_send_max_pwr_evt(struct wlrx_ui_para *ui_para)
{
	int cur_pwr;
	int tx_max_pwr_mw;

	if ((ui_para->ui_max_pwr <= 0) || (ui_para->product_max_pwr <= 0) ||
		(ui_para->tx_max_pwr < ui_para->ui_max_pwr))
		return;

	tx_max_pwr_mw = ui_para->tx_max_pwr;
	if (!wlrx_ui_is_exceptional_pwr(tx_max_pwr_mw))
		tx_max_pwr_mw = wlrx_revise_ui_max_pwr(tx_max_pwr_mw / POWER_MW_PER_W);

	hwlog_info("[send_max_pwr] revised_tx_pwr=%d ui_pwr=%d product_pwr=%d\n",
		tx_max_pwr_mw, ui_para->ui_max_pwr, ui_para->product_max_pwr);

	cur_pwr = (tx_max_pwr_mw < ui_para->product_max_pwr) ?
		tx_max_pwr_mw : ui_para->product_max_pwr;
	power_ui_event_notify(POWER_UI_NE_MAX_POWER, &cur_pwr);
}

static void wlrx_ui_send_soc_decimal_evt(struct wlrx_ui_para *ui_para)
{
	int cur_pwr;

	cur_pwr = (ui_para->tx_max_pwr < ui_para->product_max_pwr) ?
		ui_para->tx_max_pwr : ui_para->product_max_pwr;
	hwlog_info("[send_soc_decimal] tx_pwr=%d product_pwr=%d\n",
		ui_para->tx_max_pwr, ui_para->product_max_pwr);
	power_event_bnc_notify(POWER_BNT_SOC_DECIMAL, POWER_NE_SOC_DECIMAL_WL_DC, &cur_pwr);
}

static void wlrx_ui_prepare_ui_para(unsigned int drv_type, struct wlrx_ui_para *ui_para)
{
	struct wlprot_acc_cap *acc_cap = wlrx_acc_get_cap(drv_type);

	if (!acc_cap)
		return;

	ui_para->product_max_pwr = wlrx_pctrl_get_product_pmax(drv_type);
	ui_para->tx_max_pwr = acc_cap->vmax / POWER_MV_PER_V *
		acc_cap->imax * WLRX_ACC_TX_PWR_RATIO / POWER_PERCENT;
}

void wlrx_ui_reset_icon_type(unsigned int drv_type)
{
	struct wlrx_ui_dev *di = wlrx_ui_get_di(drv_type);

	if (!di)
		return;
	di->curr_icon_type = 0;
}

enum wlrx_ui_icon_type wlrx_ui_get_icon_type(unsigned int drv_type)
{
	struct wlrx_ui_dev *di = wlrx_ui_get_di(drv_type);

	if (!di)
		return WLRX_UI_NORMAL_CHARGE;

	if (di->icon_flag.fast_charge)
		return WLRX_UI_FAST_CHARGE;
	if (di->icon_flag.super_charge)
		return WLRX_UI_SUPER_CHARGE;
	return WLRX_UI_NORMAL_CHARGE;
}

void wlrx_ui_reset_icon_flag(unsigned int drv_type, int icon_type)
{
	struct wlrx_ui_dev *di = wlrx_ui_get_di(drv_type);

	if (!di)
		return;

	switch (icon_type) {
	case WLRX_UI_NORMAL_CHARGE:
		di->icon_flag.normal_charge = false;
		break;
	case WLRX_UI_FAST_CHARGE:
		di->icon_flag.fast_charge = false;
		break;
	case WLRX_UI_SUPER_CHARGE:
		di->icon_flag.super_charge = false;
		break;
	default:
		hwlog_err("reset_icon_flag: unknown icon_type\n");
		break;
	}
}

void wlrx_ui_send_charge_uevent(unsigned int drv_type, unsigned int icon_type)
{
	int icon = ICON_TYPE_WIRELESS_NORMAL;
	struct wlrx_ui_dev *di = wlrx_ui_get_di(drv_type);

	if (!di || !di->ui_para)
		return;

	di->icon_flag.normal_charge = false;
	di->icon_flag.fast_charge = false;
	di->icon_flag.super_charge = false;
	switch (icon_type) {
	case WLRX_UI_NORMAL_CHARGE:
		di->icon_flag.normal_charge = true;
		icon = ICON_TYPE_WIRELESS_NORMAL;
		break;
	case WLRX_UI_FAST_CHARGE:
		di->icon_flag.fast_charge = true;
		icon = ICON_TYPE_WIRELESS_QUICK;
		break;
	case WLRX_UI_SUPER_CHARGE:
		di->icon_flag.super_charge = true;
		icon = ICON_TYPE_WIRELESS_SUPER;
		break;
	default:
		hwlog_err("send_charge_uevent: unknown icon_type\n");
		break;
	}

	power_event_bnc_notify(POWER_BNT_WLC, POWER_NE_WLC_ICON_TYPE, &icon_type);
	hwlog_info("[send_charge_uevent] cur type=%u, last type=%u\n",
		icon_type, di->curr_icon_type);
	if (di->curr_icon_type ^ icon_type) {
		wireless_connect_send_icon_uevent(icon);
		if (di->icon_flag.super_charge) {
			wlrx_ui_prepare_ui_para(drv_type, di->ui_para);
			wlrx_ui_send_soc_decimal_evt(di->ui_para);
			wlrx_ui_send_max_pwr_evt(di->ui_para);
		}
	}

	di->curr_icon_type = icon_type;
}

static void wlrx_ui_kfree_dev(struct wlrx_ui_dev *di)
{
	if (!di)
		return;

	kfree(di->ui_para);
	kfree(di);
}

static int wlrx_ui_parse_dts(const struct device_node *np, struct wlrx_ui_para **dts)
{
	*dts = kzalloc(sizeof(**dts), GFP_KERNEL);
	if (!*dts)
		return -ENOMEM;

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"ui_max_pwr", (u32 *)&(*dts)->ui_max_pwr, 0);

	return 0;
}

int wlrx_ui_init(unsigned int drv_type, struct device *dev)
{
	int ret;
	struct wlrx_ui_dev *di = NULL;

	if (!dev || !wltrx_is_drv_type_valid(drv_type))
		return -EINVAL;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	ret = wlrx_ui_parse_dts(dev->of_node, &di->ui_para);
	if (ret)
		goto exit;

	g_rx_ui_di[drv_type] = di;
	return 0;

exit:
	wlrx_ui_kfree_dev(di);
	return ret;
}

void wlrx_ui_deinit(unsigned int drv_type)
{
	if (!wltrx_is_drv_type_valid(drv_type))
		return;

	wlrx_ui_kfree_dev(g_rx_ui_di[drv_type]);
	g_rx_ui_di[drv_type] = NULL;
}
