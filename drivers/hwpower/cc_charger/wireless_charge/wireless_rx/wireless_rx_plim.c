// SPDX-License-Identifier: GPL-2.0
/*
 * wireless_rx_plim.c
 *
 * power limit for wireless charging
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

#include <chipset_common/hwpower/wireless_charge/wireless_rx_plim.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_pctrl.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <linux/slab.h>

#define HWLOG_TAG wireless_rx_plim
HWLOG_REGIST();

enum wlrx_plim_info {
	PLIM_SRC_ID,
	PLIM_SRC_NAME,
	PLIM_NEED_RST,
	PLIM_VTX,
	PLIM_VRX,
	PLIM_IRX,
	PLIM_INFO_TOTAL,
};

struct wlrx_plim  {
	int src_id;
	const char *src_name;
	bool need_rst; /* reset para when recharging */
	int vtx; /* mV */
	int vrx; /* mV */
	int irx; /* mA */
};

static struct wlrx_plim g_plim_tbl[WLRX_PLIM_SRC_END] = {
	{ WLRX_PLIM_SRC_OTG,        "otg",        false, 5000,  5500,  1000 },
	{ WLRX_PLIM_SRC_RPP,        "rpp",        true,  12000, 12000, 1300 },
	{ WLRX_PLIM_SRC_FAN,        "fan",        true,  9000,  9900,  1250 },
	{ WLRX_PLIM_SRC_VOUT_ERR,   "vout_err",   true,  9000,  9900,  1250 },
	{ WLRX_PLIM_SRC_TX_BST_ERR, "tx_bst_err", true,  5000,  5500,  1000 },
	{ WLRX_PLIM_SRC_KB,         "keyboard",   true,  9000,  9900,  1100 },
	{ WLRX_PLIM_SRC_THERMAL,    "thermal",    false, 9000,  9900,  1250 },
};

struct wlrx_plim_dev {
	struct wlrx_plim *tbl;
	unsigned long cur_src;
};

static struct wlrx_plim_dev *g_rx_plim_di[WLTRX_DRV_MAX];

static struct wlrx_plim_dev *wlrx_plim_get_di(unsigned int drv_type)
{
	if (!wltrx_is_drv_type_valid(drv_type)) {
		hwlog_err("get_di: drv_type=%u err\n", drv_type);
		return NULL;
	}

	return g_rx_plim_di[drv_type];
}

unsigned int wlrx_plim_get_src(unsigned int drv_type)
{
	struct wlrx_plim_dev *di = wlrx_plim_get_di(drv_type);

	if (!di)
		return 0;

	return (unsigned int)di->cur_src;
}

void wlrx_plim_set_src(unsigned int drv_type, unsigned int src_id)
{
	struct wlrx_plim_dev *di = wlrx_plim_get_di(drv_type);

	if (!di || (src_id < WLRX_PLIM_SRC_BEGIN) || (src_id >= WLRX_PLIM_SRC_END))
		return;

	if ((di->tbl[src_id].vtx <= 0) || (di->tbl[src_id].vrx <= 0) || (di->tbl[src_id].irx <= 0))
		return;

	if (test_bit(src_id, &di->cur_src))
		return;
	set_bit(src_id, &di->cur_src);
	if (src_id != di->tbl[src_id].src_id)
		return;
	hwlog_info("[set_src] %s\n", di->tbl[src_id].src_name);
}
EXPORT_SYMBOL(wlrx_plim_set_src);

void wlrx_plim_clear_src(unsigned int drv_type, unsigned int src_id)
{
	struct wlrx_plim_dev *di = wlrx_plim_get_di(drv_type);

	if (!di || (src_id < WLRX_PLIM_SRC_BEGIN) || (src_id >= WLRX_PLIM_SRC_END))
		return;

	if ((di->tbl[src_id].vtx <= 0) || (di->tbl[src_id].vrx <= 0) || (di->tbl[src_id].irx <= 0))
		return;

	if (!test_bit(src_id, &di->cur_src))
		return;
	clear_bit(src_id, &di->cur_src);
	if (src_id != di->tbl[src_id].src_id)
		return;
	hwlog_info("[clear_src] %s\n", di->tbl[src_id].src_name);
}
EXPORT_SYMBOL(wlrx_plim_clear_src);

void wlrx_plim_reset_src(unsigned int drv_type)
{
	int i;
	struct wlrx_plim_dev *di = wlrx_plim_get_di(drv_type);

	if (!di)
		return;

	for (i = WLRX_PLIM_SRC_BEGIN; i < WLRX_PLIM_SRC_END; i++) {
		if (di->tbl[i].need_rst)
			clear_bit(i, &di->cur_src);
	}
}

void wlrx_plim_update_pctrl(unsigned int drv_type, struct wlrx_pctrl *pctrl)
{
	int i;
	struct wlrx_plim_dev *di = wlrx_plim_get_di(drv_type);

	if (!di || !pctrl)
		return;

	for (i = WLRX_PLIM_SRC_BEGIN; i < WLRX_PLIM_SRC_END; i++) {
		if (!test_bit(i, &di->cur_src))
			continue;
		pctrl->vtx = min(pctrl->vtx, di->tbl[i].vtx);
		pctrl->vrx = min(pctrl->vrx, di->tbl[i].vrx);
		pctrl->irx = min(pctrl->irx, di->tbl[i].irx);
	}
}

static void wlrx_plim_para_print(struct wlrx_plim_dev *di)
{
	int i;

	for (i = 0; i < WLRX_PLIM_SRC_END; i++)
		hwlog_info("plim_para[%d] src_id:%d src_name:%s need_rst:%d vtx:%d vrx:%d irx:%d\n",
			i, di->tbl[i].src_id, di->tbl[i].src_name, di->tbl[i].need_rst,
			di->tbl[i].vtx, di->tbl[i].vrx, di->tbl[i].irx);
}

static int wlrx_revise_plim_para(struct wlrx_plim_dev *di, struct wlrx_plim *tbl, int len)
{
	int i, src_id;

	di->tbl = kcalloc(WLRX_PLIM_SRC_END, sizeof(*di->tbl), GFP_KERNEL);
	if (!di->tbl)
		return -ENOMEM;

	memcpy(di->tbl, &g_plim_tbl[0], sizeof(*di->tbl) * WLRX_PLIM_SRC_END);
	for (i = 0; i < len; i++) {
		src_id = tbl[i].src_id;
		if ((src_id < WLRX_PLIM_SRC_BEGIN) || (src_id >= WLRX_PLIM_SRC_END)) {
			kfree(di->tbl);
			return -EINVAL;
		}

		if (di->tbl[src_id].src_name && strncmp(di->tbl[src_id].src_name,
			tbl[i].src_name, strlen(di->tbl[src_id].src_name))) {
			kfree(di->tbl);
			return -EINVAL;
		}

		di->tbl[src_id].src_id = tbl[i].src_id;
		di->tbl[src_id].src_name = tbl[i].src_name;
		di->tbl[src_id].need_rst = tbl[i].need_rst;
		di->tbl[src_id].vtx = tbl[i].vtx;
		di->tbl[src_id].vrx = tbl[i].vrx;
		di->tbl[src_id].irx = tbl[i].irx;
	}

	wlrx_plim_para_print(di);
	return 0;
}

static int wlrx_plim_para_str2int(const char *str, int *plim, int i)
{
	if (kstrtoint(str, 0, &plim[(i - 3) % PLIM_INFO_TOTAL])) /* 3: id, name, need_rst */
		return -EINVAL;

	return 0;
}

static int wlrx_parse_plim_para(const struct device_node *np, struct wlrx_plim_dev *di)
{
	int i, len, row;
	const char *tmp_str = NULL;
	struct wlrx_plim tbl[WLRX_PLIM_SRC_END];

	len = power_dts_read_count_strings(power_dts_tag(HWLOG_TAG), np,
		"plim_para", WLRX_PLIM_SRC_END, PLIM_INFO_TOTAL);
	if (len <= 0) {
		di->tbl = &g_plim_tbl[0];
		wlrx_plim_para_print(di);
		return 0;
	}

	for (i = 0; i < len; i++) {
		if (power_dts_read_string_index(power_dts_tag(HWLOG_TAG), np,
			"plim_para", i, &tmp_str))
			return -EINVAL;

		row = i / PLIM_INFO_TOTAL;
		if ((row < WLRX_PLIM_SRC_BEGIN) || (row >= WLRX_PLIM_SRC_END))
			return -EINVAL;

		if ((i % PLIM_INFO_TOTAL) == PLIM_SRC_ID) {
			if (kstrtoint(tmp_str, POWER_BASE_DEC, &tbl[row].src_id))
				return -EINVAL;
			continue;
		}
		if ((i % PLIM_INFO_TOTAL) == PLIM_SRC_NAME) {
			tbl[row].src_name = tmp_str;
			continue;
		}
		if ((i % PLIM_INFO_TOTAL) == PLIM_NEED_RST) {
			if (kstrtobool(tmp_str, &tbl[row].need_rst))
				return -EINVAL;
			continue;
		}
		if (wlrx_plim_para_str2int(tmp_str, (int *)&tbl[row].vtx, i))
			return -EINVAL;
	}

	return wlrx_revise_plim_para(di, tbl, len / PLIM_INFO_TOTAL);
}

static int wlrx_plim_parse_dts(const struct device_node *np, struct wlrx_plim_dev *di)
{
	return wlrx_parse_plim_para(np, di);
}

static void wlrx_plim_kfree_dev(struct wlrx_plim_dev *di)
{
	if (!di)
		return;

	kfree(di);
}

int wlrx_plim_init(unsigned int drv_type, struct device *dev)
{
	struct wlrx_plim_dev *di = NULL;

	if (!dev || !wltrx_is_drv_type_valid(drv_type))
		return -ENODEV;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	if (wlrx_plim_parse_dts(dev->of_node, di)) {
		kfree(di);
		return -EINVAL;
	}

	g_rx_plim_di[drv_type] = di;
	return 0;
}

void wlrx_plim_deinit(unsigned int drv_type)
{
	if (!wltrx_is_drv_type_valid(drv_type))
		return;

	wlrx_plim_kfree_dev(g_rx_plim_di[drv_type]);
	g_rx_plim_di[drv_type] = NULL;
}
