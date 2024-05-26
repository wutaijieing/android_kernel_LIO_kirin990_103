// SPDX-License-Identifier: GPL-2.0
/*
 * wireless_tx_cap.c
 *
 * tx capability for wireless reverse charging
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

#include <chipset_common/hwpower/common_module/power_printk.h>
#include <huawei_platform/hwpower/common_module/power_platform.h>
#include <chipset_common/hwpower/wireless_charge/wireless_trx_ic_intf.h>
#include <chipset_common/hwpower/wireless_charge/wireless_tx_dts.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_status.h>
#include <chipset_common/hwpower/wireless_charge/wireless_acc_types.h>
#include <chipset_common/hwpower/wireless_charge/wireless_tx_cap.h>
#include <chipset_common/hwpower/wireless_charge/wireless_tx_client.h>
#include <chipset_common/hwpower/wireless_charge/wireless_tx_pwr_src.h>
#include <chipset_common/hwpower/wireless_charge/wireless_tx_pwr_ctrl.h>
#include <chipset_common/hwpower/protocol/wireless_protocol.h>

#define HWLOG_TAG wireless_tx_cap
HWLOG_REGIST();

struct wltx_cap_dts {
	int mode_id;
	int cap_cfg_level;
	struct wltx_cap *cap_cfg;
	int fsk_ic_id;
};

struct wltx_cap_dev {
	unsigned int cur_id;
	unsigned int exp_id;
	struct wltx_cap_dts *dts;
};

static struct wltx_cap_dev *g_tx_cap_di[WLTRX_DRV_MAX];

static struct wltx_cap_dev *wltx_cap_get_di(unsigned int drv_type)
{
	if (!wltrx_is_drv_type_valid(drv_type)) {
		hwlog_err("get_di: drv_type=%u err\n", drv_type);
		return NULL;
	}

	return g_tx_cap_di[drv_type];
}

static unsigned int wltx_cap_get_ic_type(unsigned int drv_type)
{
	if (drv_type == WLTRX_DRV_MAIN)
		return WLTRX_IC_MAIN;
	return WLTRX_IC_AUX;
}

u8 wltx_cap_get_tx_type(unsigned int drv_type)
{
	struct wltx_cap_dev *di = wltx_cap_get_di(drv_type);

	if (!di || !di->dts || (di->cur_id >= di->dts->cap_cfg_level))
		return -EINVAL;

	return di->dts->cap_cfg[di->cur_id].type;
}

void wltx_cap_set_exp_id(unsigned int drv_type, unsigned int exp_id)
{
	struct wltx_cap_dev *di = wltx_cap_get_di(drv_type);

	if (!di)
		return;

	di->exp_id = exp_id;
	hwlog_info("[set_exp_id] id=%u\n", di->exp_id);
}

unsigned int wltx_cap_get_mode_id(unsigned int drv_type)
{
	struct wltx_cap_dev *di = wltx_cap_get_di(drv_type);

	if (!di || !di->dts)
		return WLTX_DFLT_CAP;

	return di->dts->mode_id;
}

unsigned int wltx_cap_get_cur_id(unsigned int drv_type)
{
	struct wltx_cap_dev *di = wltx_cap_get_di(drv_type);

	if (!di)
		return WLTX_DFLT_CAP;

	return di->cur_id;
}

void wltx_cap_set_cur_id(unsigned int drv_type, unsigned int cur_id)
{
	struct wltx_cap_dev *di = wltx_cap_get_di(drv_type);

	if (!di)
		return;

	di->cur_id = cur_id;
	hwlog_info("[set_cur_id] id=%u\n", di->cur_id);
}

static void wltx_cap_set_type(u8 *tx_cap, u8 type)
{
	if ((type != WLACC_ADP_OTG_A) && (type != WLACC_ADP_OTG_B)) {
		tx_cap[WLTX_CAP_TYPE] = type;
	} else {
		if (wlrx_get_wired_channel_state() == WIRED_CHANNEL_ON)
			tx_cap[WLTX_CAP_TYPE] = WLACC_ADP_OTG_B;
		else
			tx_cap[WLTX_CAP_TYPE] = WLACC_ADP_OTG_A;
	}

	hwlog_info("[set_type] type:0x%x\n", tx_cap[WLTX_CAP_TYPE]);
}

static void wltx_cap_set_vmax(u8 *tx_cap, int vmax)
{
	/* 100mV in unit according to private qi protocol */
	tx_cap[WLTX_CAP_VMAX] = (u8)(vmax / 100);

	hwlog_info("[set_vmax] vmax:%dmV\n", vmax);
}

static bool wltx_cap_need_revise_imax(void)
{
	enum wltx_pwr_type pwr_type;
	const char *src_name = NULL;

	pwr_type = wltx_get_pwr_type();
	src_name = wltx_get_pwr_src_name(wltx_get_cur_pwr_src());
	if ((pwr_type == WL_TX_PWR_5VBST_VBUS_OTG_CP) && !strstr(src_name, "CP"))
		return true;
	if ((pwr_type == WL_TX_PWR_VBUS_OTG2) && (wltx_get_cur_pwr_src() == PWR_SRC_VBUS))
		return true;

	return false;
}

static void wltx_cap_set_imax(u8 *tx_cap, int imax)
{
	if (wltx_cap_need_revise_imax())
		imax = 500; /* 500mA imax for 5vbst/vbus/otg,otg/vbus2 pwr_src */

	/* 100mA in unit according to private qi protocol */
	tx_cap[WLTX_CAP_IMAX] = (u8)(imax / 100);

	hwlog_info("[set_imax] imax:%dmA\n", imax);
}

static void wltx_cap_set_attr(u8 *tx_cap, u8 attr)
{
	tx_cap[WLTX_CAP_ATTR] = attr;

	hwlog_info("[set_attr] attr:0x%x\n", attr);
}

static void wltx_set_tx_cap(u8 *tx_cap, struct wltx_cap *cfg_cap)
{
	wltx_cap_set_type(tx_cap, cfg_cap->type);
	wltx_cap_set_vmax(tx_cap, cfg_cap->vout);
	wltx_cap_set_imax(tx_cap, cfg_cap->iout);
	wltx_cap_set_attr(tx_cap, cfg_cap->attr);
}

static void wltx_cap_set_high_pwr_id(unsigned int drv_type, int charger_type)
{
	if (charger_type == WLTX_SC_HI_PWR_CHARGER)
		wltx_cap_set_exp_id(drv_type, WLTX_HIGH_PWR_CAP);
	else if (charger_type == WLTX_SC_HI_PWR2_CHARGER)
		wltx_cap_set_exp_id(drv_type, WLTX_HIGH_PWR2_CAP);
	else
		wltx_cap_set_exp_id(drv_type, WLTX_DFLT_CAP);
}

void wltx_send_tx_cap(unsigned int drv_type)
{
	unsigned int cap_id;
	u8 tx_cap[WLTX_CAP_END] = { WLACC_ADP_OTG_A, 0 };
	struct wltx_cap_dev *di = wltx_cap_get_di(drv_type);

	if (!di || !di->dts)
		return;

	cap_id = di->exp_id;
	if ((cap_id >= WLTX_TOTAL_CAP) || (cap_id >= di->dts->cap_cfg_level))
		return;

	wltx_set_tx_cap(tx_cap, &di->dts->cap_cfg[cap_id]);
	hwlog_info("[send_tx_cap] type=0x%x vmax=0x%x imax=0x%x attr=0x%x\n",
		tx_cap[WLTX_CAP_TYPE], tx_cap[WLTX_CAP_VMAX],
		tx_cap[WLTX_CAP_IMAX], tx_cap[WLTX_CAP_ATTR]);

	(void)wireless_send_tx_capability(wltx_cap_get_ic_type(drv_type),
		WIRELESS_PROTOCOL_QI, tx_cap, WLTX_CAP_END);
	wltx_cap_set_cur_id(drv_type, di->exp_id);
}

static void wltx_cap_check_high_pwr_id(unsigned int drv_type)
{
	int soc;
	struct wltx_pwr_ctrl_info *info = NULL;
	struct wltx_dts *dts = wltx_get_dts();

	if (!dts)
		goto set_dflt_id;
	soc = power_platform_get_battery_ui_capacity();
	if (soc < dts->high_pwr_soc)
		goto set_dflt_id;
	if ((dts->pwr_type != WL_TX_PWR_5VBST_VBUS_OTG_CP) &&
		(dts->pwr_type != WL_TX_PWR_VBUS_OTG2)) {
		wltx_cap_set_exp_id(drv_type, WLTX_HIGH_PWR_CAP);
		return;
	}
	info = wltx_get_pwr_ctrl_info();
	if (!info)
		goto set_dflt_id;

	wltx_cap_set_high_pwr_id(drv_type, info->charger_type);
	return;

set_dflt_id:
	wltx_cap_set_exp_id(drv_type, WLTX_DFLT_CAP);
}

void wltx_cap_reset_exp_tx_id(unsigned int drv_type)
{
	struct wltx_cap_dev *di = wltx_cap_get_di(drv_type);

	if (!di || !di->dts)
		return;

	if (wltx_client_is_hall())
		goto set_dflt_id;
	if (di->dts->cap_cfg_level <= 1) /* only default cap, 5v */
		goto set_dflt_id;

	wltx_cap_check_high_pwr_id(drv_type);
	return;

set_dflt_id:
	wltx_cap_set_exp_id(drv_type, WLTX_DFLT_CAP);
}

static int wltx_parse_cap_cfg(const struct device_node *np, struct wltx_cap_dts *dts)
{
	int i, len, size;

	len = power_dts_read_u32_count(power_dts_tag(HWLOG_TAG), np,
		"tx_cap", WLTX_TOTAL_CAP, WLTX_CAP_END);
	if (len <= 0)
		goto tx_cap_null;

	size = sizeof(*dts->cap_cfg) * (len / WLTX_CAP_END);
	dts->cap_cfg = kzalloc(size, GFP_KERNEL);
	if (!dts->cap_cfg)
		return -ENOMEM;
	if (power_dts_read_u32_array(power_dts_tag(HWLOG_TAG), np,
		"tx_cap", (u32 *)dts->cap_cfg, len))
		goto parse_err;

	dts->cap_cfg_level = len / WLTX_CAP_END;

	goto print_para;

parse_err:
	kfree(dts->cap_cfg);
tx_cap_null:
	dts->cap_cfg = kzalloc(sizeof(*dts->cap_cfg), GFP_KERNEL);
	if (!dts->cap_cfg)
		return -ENOMEM;
	dts->mode_id = WLTX_CAP_LOW_PWR;
	dts->cap_cfg_level = 1;
	dts->cap_cfg[0].type = WLACC_ADP_OTG_A;
	dts->cap_cfg[0].vout = 5000; /* 5V */
	dts->cap_cfg[0].iout = 500; /* 500mA */
	dts->cap_cfg[0].attr = 0; /* no attr */
print_para:
	hwlog_info("[cap_cfg] cap_mode=0x%x\n", dts->mode_id);
	for (i = 0; i < dts->cap_cfg_level; i++)
		hwlog_info("[cap_cfg][%d] type:%u vmax:%d imax:%d attr:0x%x\n",
			i, dts->cap_cfg[i].type, dts->cap_cfg[i].vout,
			dts->cap_cfg[i].iout, dts->cap_cfg[i].attr);

	return 0;
}

static int wltx_cap_parse_dts(const struct device_node *np, struct wltx_cap_dts **dts)
{
	int ret;

	*dts = kzalloc(sizeof(**dts), GFP_KERNEL);
	if (!*dts)
		return -ENOMEM;

	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"tx_cap_mode", (u32 *)&(*dts)->mode_id, WLTX_CAP_LOW_PWR);

	ret = wltx_parse_cap_cfg(np, *dts);
	if (ret)
		return ret;

	return 0;
}

static void wltx_cap_kfree_dev(struct wltx_cap_dev *di)
{
	if (!di)
		return;

	if (di->dts) {
		kfree(di->dts->cap_cfg);
		kfree(di->dts);
	}
	kfree(di);
}

int wltx_cap_init(unsigned int drv_type, const struct device_node *np)
{
	int ret;
	struct wltx_cap_dev *di = NULL;

	if (!np || !wltrx_is_drv_type_valid(drv_type))
		return -EINVAL;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	ret = wltx_cap_parse_dts(np, &di->dts);
	if (ret)
		goto exit;

	g_tx_cap_di[drv_type] = di;
	return 0;

exit:
	wltx_cap_kfree_dev(di);
	return ret;
}

void wltx_cap_deinit(unsigned int drv_type)
{
	if (!wltrx_is_drv_type_valid(drv_type))
		return;

	wltx_cap_kfree_dev(g_tx_cap_di[drv_type]);
	g_tx_cap_di[drv_type] = NULL;
}
