// SPDX-License-Identifier: GPL-2.0
/*
 * ovp_switch.c
 *
 * ovp switch to control wired channel
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
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG ovp_chsw
HWLOG_REGIST();

struct ovp_chsw_gpio_client {
	unsigned int path;
	const char *name;
};

static struct ovp_chsw_gpio_client g_gpio_types[] = {
	{ WIRED_CHANNEL_BUCK, "buck_gpio_en" },
	{ WIRED_CHANNEL_LVC, "lvc_gpio_en" },
	{ WIRED_CHANNEL_LVC_MOS, "lvc_mos_gpio_en" },
	{ WIRED_CHANNEL_SC, "sc_gpio_en" },
	{ WIRED_CHANNEL_SC_AUX, "sc_aux_gpio_en" },
	{ WIRED_CHANNEL_SC4, "sc4_gpio_en" },
};

struct ovp_chsw_gpio_para {
	int gpio_count;
	int gpio_num[WIRED_CHANNEL_ALL];
	int gpio_restraint[WIRED_CHANNEL_ALL];
	int gpio_path[WIRED_CHANNEL_ALL];
	int gpio_en_status[WIRED_CHANNEL_ALL];
	unsigned int gpio_status[WIRED_CHANNEL_ALL];
};

struct ovp_chsw_info {
	struct device *dev;
	u32 chsw_by_ovp;
	u32 gpio_low_by_set_input;
	int initialized;
	struct ovp_chsw_gpio_para para;
};

static struct ovp_chsw_info *g_ovp_chsw;

static int ovp_chsw_get_wired_channel(int channel_type)
{
	int i;
	struct ovp_chsw_gpio_para *para = NULL;

	if (!g_ovp_chsw || (channel_type < WIRED_CHANNEL_BEGIN) || (channel_type >= WIRED_CHANNEL_END))
		return WIRED_CHANNEL_RESTORE;

	para = &g_ovp_chsw->para;
	if (channel_type == WIRED_CHANNEL_ALL) {
		for (i = 0; i < para->gpio_count; i++) {
			if (para->gpio_num[i] <= 0)
				continue;
			if (para->gpio_status[i] == WIRED_CHANNEL_RESTORE)
				return WIRED_CHANNEL_RESTORE;
		}
		return WIRED_CHANNEL_CUTOFF;
	}

	for (i = 0; i < para->gpio_count; i++) {
		if (para->gpio_num[i] <= 0)
			continue;
		if (channel_type == para->gpio_path[i]) {
			if (para->gpio_status[i] == WIRED_CHANNEL_RESTORE)
				return WIRED_CHANNEL_RESTORE;
			return WIRED_CHANNEL_CUTOFF;
		}
	}

	return WIRED_CHANNEL_CUTOFF;
}

static int ovp_chsw_configure_gpio_status(int index, unsigned int value)
{
	int ret;
	struct ovp_chsw_gpio_para *para = NULL;

	if (!g_ovp_chsw)
		return 0;

	para = &g_ovp_chsw->para;
	if ((para->gpio_restraint[index] >= 0) && (value == WIRED_CHANNEL_CUTOFF)) {
		if (ovp_chsw_get_wired_channel(para->gpio_restraint[index]) != value) {
			hwlog_info("set %d cutoff fail, because %d need it keep on\n", para->gpio_path[index],
				para->gpio_restraint[index]);
			return 0;
		}
	}

	if (g_ovp_chsw->gpio_low_by_set_input && (value == WIRED_CHANNEL_RESTORE))
		ret = gpio_direction_input(para->gpio_num[index]); /* restore */
	else if (para->gpio_en_status[index])
		ret = gpio_direction_output(para->gpio_num[index], !value); /* restore or cutoff */
	else
		ret = gpio_direction_output(para->gpio_num[index], value); /* restore or cutoff */

	if (ret == 0)
		para->gpio_status[index] = value;
	return ret;
}

static int ovp_chsw_set_other_wired_channel(int channel_type, int flag)
{
	int i;
	unsigned int gpio_num = 0;
	struct ovp_chsw_gpio_para *para = NULL;

	if (!g_ovp_chsw || !g_ovp_chsw->initialized) {
		hwlog_err("ovp channel switch not initialized\n");
		return -ENODEV;
	}

	if ((channel_type < WIRED_CHANNEL_BEGIN) || (channel_type >= WIRED_CHANNEL_ALL))
		return 0;

	para = &g_ovp_chsw->para;
	for (i = 0; i < para->gpio_count; i++) {
		if (channel_type == para->gpio_path[i]) {
			gpio_num = para->gpio_num[i];
			break;
		}
	}

	for (i = 0; i < para->gpio_count; i++) {
		if ((channel_type == para->gpio_path[i]) || (para->gpio_num[i] == gpio_num))
			if ((flag == WIRED_CHANNEL_CUTOFF) || (para->gpio_restraint[i] < 0))
				continue;

		if (para->gpio_num[i] <= 0)
			continue;

		hwlog_info("%d switch set %s\n", para->gpio_path[i],
			(flag == WIRED_CHANNEL_CUTOFF) ? "off" : "on");
		if (ovp_chsw_configure_gpio_status(i, flag) < 0)
			return -EPERM;
	}

	return 0;
}

static int ovp_chsw_set_gpio(struct ovp_chsw_gpio_para *para, int channel_type, int flag)
{
	int i;

	if (!para || (channel_type < WIRED_CHANNEL_BEGIN) || (channel_type >= WIRED_CHANNEL_ALL))
		return 0;

	if (ovp_chsw_get_wired_channel(channel_type) == flag) {
		hwlog_info("%d is already set %s\n", channel_type,
			(flag == WIRED_CHANNEL_CUTOFF) ? "off" : "on");
		return 0;
	}

	for (i = 0; i < para->gpio_count; i++) {
		if (para->gpio_num[i] <= 0)
			continue;

		if ((channel_type == para->gpio_path[i]) ||
			((flag == WIRED_CHANNEL_RESTORE) && (channel_type == para->gpio_restraint[i]))) {
			hwlog_info("%d switch set %s\n", para->gpio_path[i],
				(flag == WIRED_CHANNEL_CUTOFF) ? "off" : "on");
			if (ovp_chsw_configure_gpio_status(i, flag))
				return -EPERM;
		}
	}

	return 0;
}

static int ovp_chsw_set_wired_channel(int channel_type, int flag)
{
	int i;

	if (!g_ovp_chsw || !g_ovp_chsw->initialized) {
		hwlog_err("ovp channel switch not initialized\n");
		return -ENODEV;
	}

	if ((channel_type < WIRED_CHANNEL_BEGIN) || (channel_type >= WIRED_CHANNEL_END))
		return 0;

	if (channel_type != WIRED_CHANNEL_ALL)
		return ovp_chsw_set_gpio(&g_ovp_chsw->para, channel_type, flag);

	for (i = 0; i < g_ovp_chsw->para.gpio_count; i++)
		if (ovp_chsw_set_gpio(&g_ovp_chsw->para, g_ovp_chsw->para.gpio_path[i], flag))
			return -ENODEV;

	return 0;
}

static struct wired_chsw_device_ops ovp_chsw_ops = {
	.get_wired_channel = ovp_chsw_get_wired_channel,
	.set_wired_channel = ovp_chsw_set_wired_channel,
	.set_other_wired_channel = ovp_chsw_set_other_wired_channel,
};

static void ovp_chsw_gpio_free(struct ovp_chsw_gpio_para *para)
{
	int i;

	for (i = 0; i < para->gpio_count; i++) {
		if (gpio_is_valid(para->gpio_num[i]))
			gpio_free(para->gpio_num[i]);
	}
}

static void ovp_chsw_parse_dts(struct device_node *np, struct ovp_chsw_info *di)
{
	(void)power_dts_read_u32_compatible(power_dts_tag(HWLOG_TAG),
		"huawei,wired_channel_switch", "use_ovp_cutoff_wired_channel",
		&di->chsw_by_ovp, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"gpio_low_by_set_input", &di->gpio_low_by_set_input, 1);
}

static int ovp_chsw_get_wired_channel_index(const char *name)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_gpio_types); i++) {
		if (!strcmp(g_gpio_types[i].name, name))
			return i;
	}

	return -EINVAL;
}

static int ovp_chsw_get_gpio_para(struct device_node *np,
	struct ovp_chsw_gpio_para *para)
{
	int i, index;
	const char *gpio_tag = NULL;
	const char *gpio_restraints = NULL;

	para->gpio_count = of_gpio_count(np);
	if (para->gpio_count < 0) {
		hwlog_err("gpios doesn't exist\n");
		if (!power_gpio_request(np, "gpio_ovp_chsw_en", "buck_gpio_en",
			&para->gpio_num[0])) {
			para->gpio_count = 1;
			para->gpio_path[0] = WIRED_CHANNEL_BUCK;
			para->gpio_restraint[0] = -1;
			para->gpio_status[0] = WIRED_CHANNEL_CUTOFF;
			return 0;
		}
		goto err_out;
	}

	for (i = 0; i < para->gpio_count; i++) {
		if (power_dts_read_string_index(power_dts_tag(HWLOG_TAG),
			np, "gpio_types", i, &gpio_tag))
			goto err_out;

		index = ovp_chsw_get_wired_channel_index(gpio_tag);
		if (index < 0)
			goto err_out;

		para->gpio_num[i] = of_get_gpio(np, i);
		para->gpio_path[i] = index;
		hwlog_info("gpio_types=%s, gpio_num=%d\n", gpio_tag, para->gpio_num[i]);
		if (!gpio_is_valid(para->gpio_num[i])) {
			hwlog_err("gpio %d is not valid\n", para->gpio_num[i]);
			goto err_out;
		}

		if (gpio_request(para->gpio_num[i], gpio_tag))
			hwlog_err("gpio %d request fail\n", para->gpio_num[i]);

		para->gpio_en_status[i] = 0;
		power_dts_read_u32_index(power_dts_tag(HWLOG_TAG),
			np, "gpio_en_status", i, &para->gpio_en_status[i]);

		para->gpio_restraint[i] = -1;
		if (power_dts_read_string_index(power_dts_tag(HWLOG_TAG),
			np, "gpio_restraints", i, &gpio_restraints))
			continue;

		index = ovp_chsw_get_wired_channel_index(gpio_restraints);
		if (index >= 0) {
			para->gpio_restraint[i] = index;
			hwlog_info("gpio_types=%s deponds on %s\n", gpio_restraints, gpio_tag);
		}
	}
	return 0;

err_out:
	para->gpio_count = 0;
	return -EINVAL;
}

static int ovp_chsw_gpio_init(struct device_node *np, struct ovp_chsw_info *di)
{
	if (ovp_chsw_get_gpio_para(np, &di->para))
		return -EPERM;

	di->initialized = 1;

	if (ovp_chsw_set_wired_channel(WIRED_CHANNEL_BUCK, WIRED_CHANNEL_RESTORE)) {
		hwlog_err("gpio set buck_path restore fail\n");
		ovp_chsw_gpio_free(&di->para);
		return -EPERM;
	}

	if (ovp_chsw_set_other_wired_channel(WIRED_CHANNEL_BUCK, WIRED_CHANNEL_CUTOFF)) {
		hwlog_err("set other wired channel(except buck_path) cutoff fail\n");
		ovp_chsw_gpio_free(&di->para);
		return -EPERM;
	}

	return 0;
}

static int ovp_chsw_probe(struct platform_device *pdev)
{
	int ret;
	struct ovp_chsw_info *di = NULL;
	struct device_node *np = NULL;

	if (!pdev || !pdev->dev.of_node)
		return -ENODEV;

	di = devm_kzalloc(&pdev->dev, sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	di->dev = &pdev->dev;
	np = pdev->dev.of_node;
	ovp_chsw_parse_dts(np, di);
	g_ovp_chsw = di;

	if (di->chsw_by_ovp) {
		ret = ovp_chsw_gpio_init(np, di);
		if (ret)
			goto err_out;

		ret = wired_chsw_ops_register(&ovp_chsw_ops);
		if (ret) {
			ovp_chsw_gpio_free(&di->para);
			goto err_out;
		}
	}

	platform_set_drvdata(pdev, di);
	return 0;
err_out:
	devm_kfree(&pdev->dev, di);
	g_ovp_chsw = NULL;
	return ret;
}

static int ovp_chsw_remove(struct platform_device *pdev)
{
	struct ovp_chsw_info *di = platform_get_drvdata(pdev);

	if (!di)
		return -ENODEV;

	ovp_chsw_gpio_free(&di->para);
	devm_kfree(&pdev->dev, di);
	platform_set_drvdata(pdev, NULL);
	g_ovp_chsw = NULL;
	return 0;
}

static const struct of_device_id ovp_chsw_match_table[] = {
	{
		.compatible = "huawei,ovp_channel_switch",
		.data = NULL,
	},
	{},
};

static struct platform_driver ovp_chsw_driver = {
	.probe = ovp_chsw_probe,
	.remove = ovp_chsw_remove,
	.driver = {
		.name = "huawei,ovp_channel_switch",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ovp_chsw_match_table),
	},
};

static int __init ovp_chsw_init(void)
{
	return platform_driver_register(&ovp_chsw_driver);
}

static void __exit ovp_chsw_exit(void)
{
	platform_driver_unregister(&ovp_chsw_driver);
}

fs_initcall_sync(ovp_chsw_init);
module_exit(ovp_chsw_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ovp switch module driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
