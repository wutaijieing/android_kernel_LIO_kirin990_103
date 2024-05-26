// SPDX-License-Identifier: GPL-2.0
/*
 * direct_charge_ic.c
 *
 * direct charge ic module
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
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_supply_application.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_ic.h>

#define HWLOG_TAG direct_charge_ic
HWLOG_REGIST();

static struct dc_ic_dev *g_dc_ic_di;
static struct dc_batinfo_ops *g_batinfo_ops[CHARGE_IC_MAX_NUM];
static struct dc_ic_ops *g_sc4_ic_ops[CHARGE_IC_MAX_NUM];
static struct dc_ic_ops *g_sc_ic_ops[CHARGE_IC_MAX_NUM];
static struct dc_ic_ops *g_lvc_ic_ops[CHARGE_IC_MAX_NUM];
static struct dc_ic_mode_para g_default_ic_mode_para;
static struct dc_ic_dev_info g_ic_info_group[CHARGE_IC_MAX_NUM];

struct dc_ic_dev_info *dc_ic_get_ic_info(void)
{
	return g_ic_info_group;
}

static const char *dc_ic_get_ic_mode_string(int mode)
{
	switch (mode) {
	case LVC_MODE:
		return "lvc_mode";
	case SC_MODE:
		return "sc_mode";
	case SC4_MODE:
		return "sc4_mode";
	default:
		return "illegal mode";
	}
}

struct dc_ic_ops *dc_ic_get_ic_ops(int mode, unsigned int index)
{
	if (index >= CHARGE_IC_MAX_NUM)
		return NULL;

	if (g_dc_ic_di && g_dc_ic_di->use_two_stage && mode == SC4_MODE)
		return g_sc_ic_ops[index];

	switch (mode) {
	case LVC_MODE:
		return g_lvc_ic_ops[index];
	case SC_MODE:
		return g_sc_ic_ops[index];
	case SC4_MODE:
		return g_sc4_ic_ops[index];
	default:
		return NULL;
	}
}

struct dc_batinfo_ops *dc_ic_get_battinfo_ops(unsigned int index)
{
	if (index >= CHARGE_IC_MAX_NUM)
		return NULL;

	if (g_batinfo_ops[index] == NULL)
		return g_batinfo_ops[IC_ONE];

	return g_batinfo_ops[index];
}

int dc_ic_ops_register(int mode, unsigned int index, struct dc_ic_ops *ops)
{
	if (!ops || (index >= CHARGE_IC_MAX_NUM))
		return -EPERM;

	switch (mode) {
	case LVC_MODE:
		g_lvc_ic_ops[index] = ops;
		break;
	case SC_MODE:
		g_sc_ic_ops[index] = ops;
		break;
	case SC4_MODE:
		g_sc4_ic_ops[index] = ops;
		break;
	default:
		hwlog_err("mode is error\n");
		return -EPERM;
	}

	hwlog_info("mode %d, type %u ic register ok\n", mode, index);
	return 0;
}

int dc_batinfo_ops_register(unsigned int index, struct dc_batinfo_ops *ops, int id)
{
	if (!ops || (index >= CHARGE_IC_MAX_NUM))
		return -EPERM;

	g_batinfo_ops[index] = ops;
	g_ic_info_group[index].ic_id = id;
	g_ic_info_group[index].flag = 1;

	hwlog_info("type %u batinfo register ok\n", index);
	return 0;
}

static struct dc_ic_mode_para *dc_ic_get_ic_mode_para(int mode)
{
	int i;

	if (!g_dc_ic_di || !g_dc_ic_di->para_flag) {
		g_default_ic_mode_para.ic_para_size = 1;
		return &g_default_ic_mode_para;
	}

	for (i = 0; i < g_dc_ic_di->mode_num; i++) {
		if (!strcmp(g_dc_ic_di->mode_para[i].ic_mode,
			dc_ic_get_ic_mode_string(mode)))
			break;
	}

	if (i >= g_dc_ic_di->mode_num) {
		hwlog_err("mode[%d] is illegal\n", mode);
		return NULL;
	}

	return &g_dc_ic_di->mode_para[i];
}

int dc_ic_get_ic_index(int mode, unsigned int path, unsigned int *index, int len)
{
	int i;
	int ic_num = 0;
	struct dc_ic_mode_para *para = NULL;

	if (!(path & CHARGE_MULTI_IC)) {
		hwlog_err("path[%u] is illegal\n", path);
		return -EPERM;
	}

	para = dc_ic_get_ic_mode_para(mode);
	if (!para)
		return -EPERM;

	for (i = 0; i < para->ic_para_size; i++) {
		if (ic_num >= len)
			return -EPERM;
		if (para->ic_para[i].path_index < 0)
			continue;
		if (BIT(para->ic_para[i].path_index) & path)
			index[ic_num++] = para->ic_para[i].ic_index;
	}

	return ic_num;
}

int dc_ic_get_ic_index_for_ibat(int mode, unsigned int path, unsigned int *index, int len)
{
	int i, j;
	int ic_num = 0;
	struct dc_ic_mode_para *para = NULL;
	bool flag = false;

	if (!(path & CHARGE_MULTI_IC)) {
		hwlog_err("path[%u] is illegal\n", path);
		return -EPERM;
	}

	para = dc_ic_get_ic_mode_para(mode);
	if (!para)
		return -EPERM;

	for (i = 0; i < para->ic_para_size; i++) {
		if (ic_num >= len)
			return -EPERM;
		flag = false;
		for (j = 0; j < para->ic_para_size; j++) {
			if (para->ic_para[j].ibat_sample_point == i) {
				flag = true;
				index[ic_num] = para->ic_para[j].ic_index;
				if (para->ic_para[i].path_index < 0)
					continue;
				if (BIT(para->ic_para[j].path_index) & path)
					break;
			}
		}
		if (flag)
			ic_num++;
	}

	return ic_num;
}

int dc_ic_get_ic_max_ibat(int mode, unsigned int index, int *ibat)
{
	int i;
	struct dc_ic_mode_para *para = NULL;

	if (!ibat)
		return -EINVAL;

	para = dc_ic_get_ic_mode_para(mode);
	if (!para)
		return -EPERM;

	for (i = 0; i < para->ic_para_size; i++) {
		if (para->ic_para[i].ic_index == index) {
			*ibat = para->ic_para[i].max_ibat;
			break;
		}
	}

	return 0;
}

bool dc_ic_get_ibat_from_coul(int *ibat)
{
	if (!ibat || !g_dc_ic_di || !g_dc_ic_di->use_coul_ibat)
		return false;

	*ibat = power_supply_app_get_bat_current_now();
	return true;
}

bool dc_ic_get_vbat_from_coul(int *vbat)
{
	if (!vbat || !g_dc_ic_di || !g_dc_ic_di->use_coul_vbat)
		return false;

	*vbat = power_supply_app_get_bat_voltage_now();
	return true;
}

bool dc_ic_use_two_stage(void)
{
	if (!g_dc_ic_di || !g_dc_ic_di->use_two_stage)
		return false;
	return true;
}

static int dc_ic_parse_ic_para(struct device_node *np, struct dc_ic_dev *di, int index)
{
	int array_len, col, row;
	int idata[CHARGE_IC_MAX_NUM * DC_IC_INFO_TOTAL] = { 0 };
	char *str = di->mode_para[index].ic_para_index;

	array_len = power_dts_read_string_array(power_dts_tag(HWLOG_TAG), np,
		str, idata, CHARGE_IC_MAX_NUM, DC_IC_INFO_TOTAL);
	if (array_len < 0)
		return -EPERM;

	di->mode_para[index].ic_para_size = array_len / DC_IC_INFO_TOTAL;
	for (row = 0; row < array_len / DC_IC_INFO_TOTAL; row++) {
		col = row * DC_IC_INFO_TOTAL + DC_IC_INFO_IC_INDEX;
		di->mode_para[index].ic_para[row].ic_index = idata[col];
		col = row * DC_IC_INFO_TOTAL + DC_IC_INFO_PATH_INDEX;
		di->mode_para[index].ic_para[row].path_index = idata[col];
		col = row * DC_IC_INFO_TOTAL + DC_IC_INFO_MAX_IBAT;
		di->mode_para[index].ic_para[row].max_ibat = idata[col];
		col = row * DC_IC_INFO_TOTAL + DC_IC_INFO_IBAT_POINT;
		di->mode_para[index].ic_para[row].ibat_sample_point = idata[col];
		col = row * DC_IC_INFO_TOTAL + DC_IC_INFO_VBAT_POINT;
		di->mode_para[index].ic_para[row].vbat_sample_point = idata[col];
	}

	for (row = 0; row < di->mode_para[index].ic_para_size; row++)
		hwlog_info("%s[%d]=%d %d %d %d %d\n", str, row,
			di->mode_para[index].ic_para[row].ic_index,
			di->mode_para[index].ic_para[row].path_index,
			di->mode_para[index].ic_para[row].max_ibat,
			di->mode_para[index].ic_para[row].ibat_sample_point,
			di->mode_para[index].ic_para[row].vbat_sample_point);

	return 0;
}

static int dc_ic_parse_ic_para_tab(struct device_node *np, struct dc_ic_dev *di)
{
	int i;

	for (i = 0; i < di->mode_num; i++) {
		if (dc_ic_parse_ic_para(np, di, i))
			return -EPERM;
	}
	return 0;
}

static int dc_ic_parse_mode_para_tab(struct device_node *np,
	struct dc_ic_dev *di)
{
	int array_len, i, row, col;
	const char *str = NULL;

	array_len = power_dts_read_count_strings(power_dts_tag(HWLOG_TAG), np,
		"mode_para", IC_MODE_MAX_NUM, DC_IC_MODE_TOTAL);
	if (array_len <= 0) {
		hwlog_err("mode_para read fail\n");
		goto err_out;
	}

	di->mode_num = array_len / DC_IC_MODE_TOTAL;
	for (i = 0; i < array_len; i++) {
		if (power_dts_read_string_index(power_dts_tag(HWLOG_TAG),
			np, "mode_para", i, &str))
			goto err_out;

		row = i / DC_IC_MODE_TOTAL;
		col = i % DC_IC_MODE_TOTAL;
		switch (col) {
		case DC_IC_MODE_INDEX:
			strncpy(di->mode_para[row].ic_mode,
				str, PARA_NAME_LEN_MAX - 1);
			break;
		case DC_IC_MODE_IC_PARA_NAME:
			strncpy(di->mode_para[row].ic_para_index,
				str, PARA_NAME_LEN_MAX - 1);
			break;
		default:
			break;
		}
	}

	for (i = 0; i < di->mode_num; i++)
		hwlog_info("mode_para[%d] %s %s\n", i, di->mode_para[i].ic_mode,
			di->mode_para[i].ic_para_index);

	return 0;

err_out:
	di->mode_num = 0;
	return -EPERM;
}

static void dc_ic_parse_dts(struct device_node *np, struct dc_ic_dev *di)
{
	di->para_flag = false;
	if (dc_ic_parse_mode_para_tab(np, di))
		return;

	if (dc_ic_parse_ic_para_tab(np, di))
		return;

	di->para_flag = true;

	if (!power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"vbat_en_gpio_output_value", &di->vbat_en_gpio_output_value, 0))
		power_gpio_config_output(np, "vbat_en_gpio", "vbat_en_gpio",
			&di->vbat_en_gpio, di->vbat_en_gpio_output_value);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"use_coul_ibat", &di->use_coul_ibat, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"use_coul_vbat", &di->use_coul_vbat, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"use_two_stage", &di->use_two_stage, 0);
}

static int dc_ic_probe(struct platform_device *pdev)
{
	struct dc_ic_dev *l_dev = NULL;
	struct device_node *np = NULL;

	if (!pdev || !pdev->dev.of_node)
		return -ENODEV;

	l_dev = kzalloc(sizeof(*l_dev), GFP_KERNEL);
	if (!l_dev)
		return -ENOMEM;

	g_dc_ic_di = l_dev;
	np = pdev->dev.of_node;

	dc_ic_parse_dts(np, l_dev);
	platform_set_drvdata(pdev, l_dev);

	return 0;
}

static int dc_ic_remove(struct platform_device *pdev)
{
	struct dc_ic_dev *l_dev = platform_get_drvdata(pdev);

	if (!l_dev)
		return -ENODEV;

	platform_set_drvdata(pdev, NULL);
	kfree(l_dev);
	g_dc_ic_di = NULL;

	return 0;
}

static const struct of_device_id dc_ic_match_table[] = {
	{
		.compatible = "huawei,direct_charge_ic",
		.data = NULL,
	},
	{},
};

static struct platform_driver dc_ic_driver = {
	.probe = dc_ic_probe,
	.remove = dc_ic_remove,
	.driver = {
		.name = "huawei,direct_charge_ic",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(dc_ic_match_table),
	},
};

static int __init dc_ic_init(void)
{
	return platform_driver_register(&dc_ic_driver);
}

static void __exit dc_ic_exit(void)
{
	platform_driver_unregister(&dc_ic_driver);
}

device_initcall_sync(dc_ic_init);
module_exit(dc_ic_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("direct charge ic driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
