// SPDX-License-Identifier: GPL-2.0
/*
 * wired_channel_manager.c
 *
 * wired_channel_manager to control wired channel
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

#include <linux/init.h>
#include <linux/module.h>
#include <chipset_common/hwpower/hardware_channel/wired_channel_switch.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_dts.h>

#define HWLOG_TAG wired_ch_manager
HWLOG_REGIST();

#define WDCM_FORCED        1
#define WDCM_UNFORCED      0
#define WDCM_CFG_LEN       2

static struct {
	enum wdcm_client client;
	const char *name;
} const g_wdcm_client[] = {
	{ WDCM_CLIENT_WLS,    "WDCM_CLIENT_WLS" },
	{ WDCM_CLIENT_WIRED,  "WDCM_CLIENT_WIRED" },
	{ WDCM_CLIENT_OTG,    "WDCM_CLIENT_OTG" },
	{ WDCM_CLIENT_TX_OTG, "WDCM_CLIENT_TX_OTG" }
};

struct wdcm_client_info {
	int state;
	bool force_flag;
};

struct wdcm_cfg {
	int channel_state;
	int force_flag;
};

struct wdcm_dev {
	struct device *dev;
	struct wdcm_client_info client_info[WDCM_CLIENT_END];
};

/* wdcm_client: WDCM_CLIENT_WLS */
struct wdcm_cfg g_wdcm_attr0[] = {
	[WDCM_DEV_OFF] = { WIRED_CHANNEL_CUTOFF, WDCM_UNFORCED },
	[WDCM_DEV_ON] = { WIRED_CHANNEL_CUTOFF, WDCM_UNFORCED }
};

/* wdcm_client: WDCM_CLIENT_WIRED */
struct wdcm_cfg g_wdcm_attr1[] = {
	[WDCM_DEV_OFF] = { WIRED_CHANNEL_RESTORE, WDCM_UNFORCED },
	[WDCM_DEV_ON] = { WIRED_CHANNEL_RESTORE, WDCM_FORCED }
};

/* wdcm_client: WDCM_CLIENT_OTG */
struct wdcm_cfg g_wdcm_attr2[] = {
	[WDCM_DEV_OFF] = { WIRED_CHANNEL_RESTORE, WDCM_UNFORCED },
	[WDCM_DEV_ON] = { WIRED_CHANNEL_CUTOFF,  WDCM_UNFORCED }
};

/* wdcm_client: WDCM_CLIENT_TX_OTG */
struct wdcm_cfg g_wdcm_attr3[] = {
	[WDCM_DEV_OFF] = { WIRED_CHANNEL_RESTORE, WDCM_UNFORCED },
	[WDCM_DEV_ON] = { WIRED_CHANNEL_CUTOFF,  WDCM_UNFORCED }
};

static struct {
	enum wdcm_client client;
	struct wdcm_cfg *wdcm_cfg;
} const g_wdcm_ctrl[] = {
	{ WDCM_CLIENT_WLS,    g_wdcm_attr0 },
	{ WDCM_CLIENT_WIRED,  g_wdcm_attr1 },
	{ WDCM_CLIENT_OTG,    g_wdcm_attr2 },
	{ WDCM_CLIENT_TX_OTG, g_wdcm_attr3 }
};

static struct wdcm_dev *g_wdcm_dev;

bool wdcm_dev_exist(void)
{
	return g_wdcm_dev != NULL;
}

static const char *wdcm_get_client_name(enum wdcm_client client)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(g_wdcm_client); i++) {
		if (client == g_wdcm_client[i].client)
			return g_wdcm_client[i].name;
	}
	return "NA";
}

static int wdcm_vote_buck_channel_state(struct wdcm_dev *l_dev)
{
	int client;

	for (client = WDCM_CLIENT_BEGIN; client < WDCM_CLIENT_END; client++) {
		if (l_dev->client_info[client].force_flag)
			return l_dev->client_info[client].state;
	}
	for (client = WDCM_CLIENT_BEGIN; client < WDCM_CLIENT_END; client++) {
		if (l_dev->client_info[client].state == WIRED_CHANNEL_CUTOFF)
			return l_dev->client_info[client].state;
	}
	return WIRED_CHANNEL_RESTORE;
}

void wdcm_set_buck_channel_state(int client, int client_state)
{
	int voted_state;
	struct wdcm_dev *l_dev = g_wdcm_dev;

	if (!l_dev || (client < WDCM_CLIENT_BEGIN) || (client >= WDCM_CLIENT_END) ||
		((client_state != WDCM_DEV_ON) && (client_state != WDCM_DEV_OFF)))
		return;

	l_dev->client_info[client].state = g_wdcm_ctrl[client].wdcm_cfg[client_state].channel_state;
	l_dev->client_info[client].force_flag = g_wdcm_ctrl[client].wdcm_cfg[client_state].force_flag;

	if (l_dev->client_info[client].force_flag)
		voted_state = l_dev->client_info[client].state;
	else
		voted_state = wdcm_vote_buck_channel_state(l_dev);
	hwlog_info("[set_buck_channel] client:%s original_state %s, voted_state %s\n",
		wdcm_get_client_name(client),
		(l_dev->client_info[client].state == WIRED_CHANNEL_RESTORE) ? "on" : "off",
		(voted_state == WIRED_CHANNEL_RESTORE) ? "on" : "off");
	wired_chsw_set_wired_channel(WIRED_CHANNEL_BUCK, voted_state);
}

static void wdcm_parse_cfg_dts(const struct device_node *np,
	const char *prop,  int client, int client_state)
{
	int len;

	len = of_property_count_u32_elems(np, prop);
	if (len != WDCM_CFG_LEN)
		return;

	if (power_dts_read_u32_array(power_dts_tag(HWLOG_TAG), np,
		prop, (u32 *)&g_wdcm_ctrl[client].wdcm_cfg[client_state].channel_state, len))
		hwlog_info("[parse_cfg_dts] %s use default para\n", prop);
}

static int wdcm_parse_dts(const struct device_node *np)
{
	int i;

	wdcm_parse_cfg_dts(np, "wdcm_wls_on_para", WDCM_CLIENT_WLS, WDCM_DEV_ON);
	wdcm_parse_cfg_dts(np, "wdcm_wls_off_para", WDCM_CLIENT_WLS, WDCM_DEV_OFF);
	wdcm_parse_cfg_dts(np, "wdcm_wired_on_para", WDCM_CLIENT_WIRED, WDCM_DEV_ON);
	wdcm_parse_cfg_dts(np, "wdcm_wired_off_para", WDCM_CLIENT_WIRED, WDCM_DEV_OFF);
	wdcm_parse_cfg_dts(np, "wdcm_otg_on_para", WDCM_CLIENT_OTG, WDCM_DEV_ON);
	wdcm_parse_cfg_dts(np, "wdcm_otg_off_para", WDCM_CLIENT_OTG, WDCM_DEV_OFF);
	wdcm_parse_cfg_dts(np, "wdcm_tx_otg_on_para", WDCM_CLIENT_TX_OTG, WDCM_DEV_ON);
	wdcm_parse_cfg_dts(np, "wdcm_tx_otg_off_para", WDCM_CLIENT_TX_OTG, WDCM_DEV_OFF);

	for (i = WDCM_CLIENT_BEGIN; i < WDCM_CLIENT_END; i++)
		hwlog_info("[wdcm_parse_dts] %s: wdcm_dev_on=%s force_flag=%s\t"
			"wdcm_dev_off=%s force_flag=%s\n",
			wdcm_get_client_name(i),
			g_wdcm_ctrl[i].wdcm_cfg[WDCM_DEV_ON].channel_state ? "off" : "on",
			g_wdcm_ctrl[i].wdcm_cfg[WDCM_DEV_ON].force_flag ? "forced" : "unforced",
			g_wdcm_ctrl[i].wdcm_cfg[WDCM_DEV_OFF].channel_state ? "off" : "on",
			g_wdcm_ctrl[i].wdcm_cfg[WDCM_DEV_OFF].force_flag ? "forced" : "unforced");
	return 0;
}

static int wdcm_probe(struct platform_device *pdev)
{
	int ret;
	struct wdcm_dev *l_dev = NULL;
	struct device_node *np = NULL;

	l_dev = kzalloc(sizeof(*l_dev), GFP_KERNEL);
	if (!l_dev)
		return -ENOMEM;

	if (!pdev || !pdev->dev.of_node)
		return -ENODEV;

	g_wdcm_dev = l_dev;
	l_dev->dev = &pdev->dev;
	np = pdev->dev.of_node;
	ret = wdcm_parse_dts(np);
	if (ret) {
		kfree(g_wdcm_dev);
		g_wdcm_dev = NULL;
		return ret;
	}

	platform_set_drvdata(pdev, g_wdcm_dev);
	return 0;
}

static int wdcm_remove(struct platform_device *pdev)
{
	struct ovp_chsw_info *di = platform_get_drvdata(pdev);

	if (!di)
		return -ENODEV;

	kfree(g_wdcm_dev);
	platform_set_drvdata(pdev, NULL);
	g_wdcm_dev = NULL;
	return 0;
}

static struct of_device_id wdcm_match_table[] = {
	{
		.compatible = "huawei,wired_channel_manager",
		.data = NULL,
	},
	{},
};

static struct platform_driver wdcm_driver = {
	.probe = wdcm_probe,
	.remove = wdcm_remove,
	.driver = {
		.name = "huawei,wired_channel_manager",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(wdcm_match_table),
	},
};

static int __init wdcm_init(void)
{
	return platform_driver_register(&wdcm_driver);
}

static void __exit wdcm_exit(void)
{
	platform_driver_unregister(&wdcm_driver);
}

fs_initcall_sync(wdcm_init);
module_exit(wdcm_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("wired channel manager driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
