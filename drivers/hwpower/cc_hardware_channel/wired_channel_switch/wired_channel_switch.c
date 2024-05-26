// SPDX-License-Identifier: GPL-2.0
/*
 * wired_channel_switch.c
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

#include <chipset_common/hwpower/hardware_channel/wired_channel_switch.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <huawei_platform/hwpower/common_module/power_platform.h>
#include <chipset_common/hwpower/wireless_charge/wireless_tx_pwr_ctrl.h>

#define HWLOG_TAG wired_chsw
HWLOG_REGIST();

static struct wired_chsw_device_ops *g_chsw_ops;
static struct wired_chsw_dts *g_chsw_dts;

static const char * const g_chsw_channel_type_table[WIRED_CHANNEL_END] = {
	[WIRED_CHANNEL_BUCK] = "wired_channel_buck",
	[WIRED_CHANNEL_LVC] = "wired_channel_lvc",
	[WIRED_CHANNEL_LVC_MOS] = "wired_channel_lvc_mos",
	[WIRED_CHANNEL_SC] = "wired_channel_sc",
	[WIRED_CHANNEL_SC_AUX] = "wired_channel_sc_aux",
	[WIRED_CHANNEL_SC4] = "wired_channel_sc4",
	[WIRED_CHANNEL_ALL] = "wired_channel_all",
};

const char *wired_chsw_get_channel_type_string(int type)
{
	if ((type >= WIRED_CHANNEL_BEGIN) && (type < WIRED_CHANNEL_END))
		return g_chsw_channel_type_table[type];

	return "illegal channel type";
}

int wired_chsw_ops_register(struct wired_chsw_device_ops *ops)
{
	if (ops && !g_chsw_ops) {
		g_chsw_ops = ops;
		hwlog_info("wired_chsw ops register ok\n");
		return 0;
	}

	hwlog_err("wired_chsw ops register fail\n");
	return -EPERM;
}

int wired_chsw_get_wired_channel(int channel_type)
{
	int state;

	if (!g_chsw_ops || !g_chsw_ops->get_wired_channel) {
		hwlog_err("g_chsw_ops or get_wired_channel is null\n");
		return WIRED_CHANNEL_RESTORE;
	}

	if ((channel_type < WIRED_CHANNEL_BEGIN) || (channel_type >= WIRED_CHANNEL_END)) {
		hwlog_err("channel_type is illegal\n");
		return WIRED_CHANNEL_RESTORE;
	}

	state = g_chsw_ops->get_wired_channel(channel_type);
	hwlog_info("get wired channel:%s is %s", wired_chsw_get_channel_type_string(channel_type),
		(state == WIRED_CHANNEL_RESTORE) ? "on" : "off");
	return state;
}

struct wired_chsw_dts *wired_chsw_get_dts(void)
{
	return g_chsw_dts;
}

static int wired_chsw_get_other_wired_channel_status(int channel_type)
{
	int i;
	int state = WIRED_CHANNEL_CUTOFF;

	if (channel_type == WIRED_CHANNEL_ALL)
		return WIRED_CHANNEL_CUTOFF;

	for (i = WIRED_CHANNEL_BEGIN; i < WIRED_CHANNEL_ALL; i++) {
		if ((i != channel_type) &&
			(wired_chsw_get_wired_channel(i) == WIRED_CHANNEL_RESTORE)) {
				state = WIRED_CHANNEL_RESTORE;
				break;
			}
	}

	hwlog_info("get other wired channel:%s is %s", wired_chsw_get_channel_type_string(channel_type),
		(state == WIRED_CHANNEL_RESTORE) ? "on" : "off");
	return state;
}

int wired_chsw_set_other_wired_channel(int channel_type, int state)
{
	if (!g_chsw_ops || !g_chsw_ops->set_other_wired_channel) {
		hwlog_err("g_chsw_ops or set_other_wired_channel is null\n");
		return 0;
	}

	if ((channel_type < WIRED_CHANNEL_BEGIN) || (channel_type >= WIRED_CHANNEL_ALL))
		return 0;

	hwlog_info("%s other need set %s\n", wired_chsw_get_channel_type_string(channel_type),
		(state == WIRED_CHANNEL_RESTORE) ? "on" : "off");

	return g_chsw_ops->set_other_wired_channel(channel_type, state);
}

int wired_chsw_set_wired_channel(int channel_type, int state)
{
	int new_state;
	int wltx_vbus_change_type;

	if (!g_chsw_ops || !g_chsw_ops->set_wired_channel) {
		hwlog_err("g_chsw_ops or set_wired_channel is null\n");
		return 0;
	}

	if ((channel_type < WIRED_CHANNEL_BEGIN) || (channel_type >= WIRED_CHANNEL_END))
		return 0;

	wltx_vbus_change_type = wltx_get_vbus_change_type();
	if ((wltx_vbus_change_type == WLTX_VBUS_CHANGED_BY_WIRED_CHSW) &&
		(channel_type != WIRED_CHANNEL_SC)) {
		if (state == WIRED_CHANNEL_RESTORE)
			wireless_tx_cancel_work(PWR_SW_BY_VBUS_ON);
		else if (wired_chsw_get_other_wired_channel_status(channel_type) ==
			WIRED_CHANNEL_CUTOFF)
			wireless_tx_cancel_work(PWR_SW_BY_VBUS_OFF);
	}

	hwlog_info("%s need set %s\n", wired_chsw_get_channel_type_string(channel_type),
		(state == WIRED_CHANNEL_RESTORE) ? "on" : "off");

	if (!g_chsw_ops->set_wired_channel(channel_type, state)) {
		new_state = wired_chsw_get_wired_channel(channel_type);
		hwlog_info("%s is set to %s\n", wired_chsw_get_channel_type_string(channel_type),
			(new_state == WIRED_CHANNEL_RESTORE) ? "on" : "off");
	}

	if ((wltx_vbus_change_type == WLTX_VBUS_CHANGED_BY_WIRED_CHSW) &&
		(channel_type != WIRED_CHANNEL_SC)) {
		if (state == WIRED_CHANNEL_RESTORE)
			wireless_tx_restart_check(PWR_SW_BY_VBUS_ON);
		else if (wired_chsw_get_wired_channel(WIRED_CHANNEL_ALL) == WIRED_CHANNEL_CUTOFF)
			wireless_tx_restart_check(PWR_SW_BY_VBUS_OFF);
	}

	return 0;
}

int wired_chsw_set_wired_reverse_channel(int state)
{
	if (!g_chsw_ops || !g_chsw_ops->set_wired_reverse_channel) {
		hwlog_err("g_chsw_ops or set_wired_reverse_channel is null\n");
		return -EPERM;
	}

	return g_chsw_ops->set_wired_reverse_channel(state);
}

static int wired_chsw_check_ops(void)
{
	if (!g_chsw_ops || !g_chsw_ops->set_wired_channel) {
		hwlog_err("g_chsw_ops is null\n");
		return -EINVAL;
	}

	return 0;
}

static void wired_chsw_parse_dts(struct wired_chsw_dts **dts)
{
	*dts = kzalloc(sizeof(**dts), GFP_KERNEL);
	if (!*dts)
		return;

	(void)power_dts_read_u32_compatible(power_dts_tag(HWLOG_TAG),
		"huawei,wired_channel_switch", "wired_sw_dflt_on",
		&(*dts)->wired_sw_dflt_on, 0);
}

static void wired_chsw_kfree_dts(struct wired_chsw_dts *dts)
{
	if (!dts)
		return;

	kfree(dts);
}

static int wired_chsw_probe(struct platform_device *pdev)
{
	int ret;

	wired_chsw_parse_dts(&g_chsw_dts);
	ret = wired_chsw_check_ops();
	if (ret) {
		wired_chsw_kfree_dts(g_chsw_dts);
		return -EPERM;
	}

	return 0;
}

static const struct of_device_id wired_chsw_match_table[] = {
	{
		.compatible = "huawei,wired_channel_switch",
		.data = NULL,
	},
	{},
};

static struct platform_driver wired_chsw_driver = {
	.probe = wired_chsw_probe,
	.driver = {
		.name = "huawei,wired_channel_switch",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(wired_chsw_match_table),
	},
};

static int __init wired_chsw_init(void)
{
	return platform_driver_register(&wired_chsw_driver);
}

static void __exit wired_chsw_exit(void)
{
	platform_driver_unregister(&wired_chsw_driver);
}

module_init(wired_chsw_init);
module_exit(wired_chsw_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("wired channel switch module driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
