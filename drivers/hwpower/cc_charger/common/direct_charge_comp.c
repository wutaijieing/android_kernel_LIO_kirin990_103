// SPDX-License-Identifier: GPL-2.0
/*
 * direct_charge_comp.c
 *
 * compensation parameter interface for direct charger
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

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/math64.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_supply_application.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_comp.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_ic_manager.h>
#include <huawei_platform/power/direct_charger/direct_charger.h>

#define HWLOG_TAG direct_charge_comp
HWLOG_REGIST();

#define DC_VBAT_COMP_BASE_PARA_MAX    20
#define DC_VBAT_COMP_INDEX_PARA_MAX   5
#define DC_VBAT_COMP_BASE             500
#define DC_VBAT_COMP_INDEX_DEFULT     (-1)
#define DC_VBAT_COMP_IR               BIT(0)
#define DC_VBAT_COMP_LEAK_CUR         BIT(1)

enum dc_ic_vbat_comp_base_info {
	DC_IC_NAME_BASE,
	DC_VBAT_COMP_LEAK_CUR_P,
	DC_VBAT_COMP_LEAK_CUR_N,
	DC_VBAT_COMP_MODE,
	DC_VBAT_COMP_BASE_INFO_TOTAL,
};

enum dc_ic_vbat_comp_index_info {
	DC_IC_ROLE_INDEX,
	DC_VBAT_COMP_INDEX_P,
	DC_VBAT_COMP_INDEX_N,
	DC_VBAT_COMP_INDEX_INFO_TOTAL,
};

struct dc_vbat_comp_base_para {
	char ic_name[DC_IC_NAME_LEN_MAX];
	int leak_cur_p;
	int leak_cur_n;
	int comp_mode;
};

struct dc_vbat_comp_index_para {
	int ic_index;
	int p_index;
	int n_index;
};

struct dc_ic_vbat_comp {
	int p_index;
	int n_index;
	int leak_cur_p;
	int leak_cur_n;
	int leak_cur_comp;
	int comp_mode;
};

struct dc_comp_device {
	struct device *dev;
	struct dc_vbat_comp_base_para ic_para[DC_VBAT_COMP_BASE_PARA_MAX];
	struct dc_vbat_comp_index_para vbat_samp_para[DC_VBAT_COMP_INDEX_PARA_MAX];
	struct dc_ic_vbat_comp ic_vbat_comp[CHARGE_IC_MAX_NUM];
	int ic_para_group_size;
	int vbat_samp_para_group_size;
	int compensation_r;
	int leak_cur_r;
	int compensation_mode;
	int finish_flag;
	bool cal_complete;
};

static struct dc_comp_device *g_dc_comp_di;

void dc_get_vbat_comp_para(const char *ic_name, struct dc_comp_para *info)
{
	int i;

	if (!ic_name || !info || !info->vbat_comp_group_size)
		return;

	for (i = 0; i < info->vbat_comp_group_size; i++) {
		if (!strstr(ic_name, info->vbat_comp_para[i].ic_name))
			continue;

		memcpy(info->vbat_comp, info->vbat_comp_para[i].vbat_comp,
			sizeof(info->vbat_comp));

		hwlog_info("[%d]: ic_name=%s, vbat_comp_main=%d, vbat_comp_aux=%d\n",
			i, info->vbat_comp_para[i].ic_name,
			info->vbat_comp[0], info->vbat_comp[1]);
		return;
	}

	memcpy(info->vbat_comp, info->vbat_comp_para[0].vbat_comp,
		sizeof(info->vbat_comp));
	hwlog_info("ic_name=%s is invalid, vbat_comp_main=%d, vbat_comp_aux=%d\n",
		ic_name, info->vbat_comp[0], info->vbat_comp[1]);
}

static void dc_parse_vbat_comp_para(struct device_node *np,
	struct dc_comp_para *info)
{
	int i, row, col, len, idata;
	const char *tmp_string = NULL;

	len = power_dts_read_count_strings(power_dts_tag(HWLOG_TAG), np,
		"vbat_comp_para", DC_VBAT_COMP_PARA_MAX, DC_VBAT_COMP_TOTAL);
	if (len < 0) {
		info->vbat_comp_group_size = 0;
		return;
	}

	info->vbat_comp_group_size = len / DC_VBAT_COMP_TOTAL;

	for (i = 0; i < len; i++) {
		if (power_dts_read_string_index(power_dts_tag(HWLOG_TAG),
			np, "vbat_comp_para", i, &tmp_string))
			return;

		row = i / DC_VBAT_COMP_TOTAL;
		col = i % DC_VBAT_COMP_TOTAL;
		switch (col) {
		case DC_IC_NAME:
			strncpy(info->vbat_comp_para[row].ic_name,
				tmp_string, DC_IC_NAME_LEN_MAX - 1);
			break;
		case DC_VBAT_COMP_VALUE_MAIN:
			if (kstrtoint(tmp_string, POWER_BASE_DEC, &idata))
				return;
			info->vbat_comp_para[row].vbat_comp[0] = idata;
			break;
		case DC_VBAT_COMP_VALUE_AUX:
			if (kstrtoint(tmp_string, POWER_BASE_DEC, &idata))
				return;
			info->vbat_comp_para[row].vbat_comp[1] = idata;
			break;
		default:
			break;
		}
	}

	for (i = 0; i < info->vbat_comp_group_size; i++)
		hwlog_info("ic_name=%s,vbat_comp_main=%d,vbat_comp_aux=%d\n",
			info->vbat_comp_para[i].ic_name,
			info->vbat_comp_para[i].vbat_comp[0],
			info->vbat_comp_para[i].vbat_comp[1]);
}

void dc_comp_parse_dts(struct device_node *np, struct dc_comp_para *info)
{
	if (!np || !info)
		return;

	dc_parse_vbat_comp_para(np, info);
}

bool dc_use_ic_vbat_comp(void)
{
	struct dc_comp_device *di = g_dc_comp_di;

	if (!di || !di->finish_flag)
		return false;

	return true;
}

static int dc_get_ir_comp(struct dc_comp_device *di)
{
	int ibat;

	ibat = power_supply_app_get_bat_current_now();
	if (ibat <= 0)
		return 0;

	return di->compensation_r * ibat / POWER_UV_PER_MV;
}

static int dc_get_leakage_comp(struct dc_comp_device *di, int p_index, int n_index)
{
	int i;
	int comp = 0;
	int leakage = 0;

	for (i = 0; i < CHARGE_IC_MAX_NUM; i++) {
		if (p_index == di->ic_vbat_comp[i].p_index)
			leakage += di->ic_vbat_comp[i].leak_cur_p;

		if (n_index == di->ic_vbat_comp[i].n_index)
			leakage += di->ic_vbat_comp[i].leak_cur_n;
	}

	comp += (leakage * di->leak_cur_r + DC_VBAT_COMP_BASE) / POWER_UV_PER_MV;
	return comp;
}

static void dc_cal_vbat_comp(struct dc_comp_device *di)
{
	int i;
	int p_index = 0;
	int n_index = 0;

	for (i = 0; i < CHARGE_IC_MAX_NUM; i++) {
		p_index = di->ic_vbat_comp[i].p_index;
		n_index = di->ic_vbat_comp[i].n_index;
		if (di->ic_vbat_comp[i].comp_mode & DC_VBAT_COMP_LEAK_CUR)
			di->ic_vbat_comp[i].leak_cur_comp = dc_get_leakage_comp(di, p_index, n_index);
		else
			di->ic_vbat_comp[i].leak_cur_comp = 0;

		hwlog_info("[%d]p_index=%d,n_index=%d,leak_cur_p=%d,leak_cur_n=%d,\t"
			"leak_cur_comp=%d,comp_mode=%d\n", i,
			di->ic_vbat_comp[i].p_index,
			di->ic_vbat_comp[i].n_index,
			di->ic_vbat_comp[i].leak_cur_p,
			di->ic_vbat_comp[i].leak_cur_n,
			di->ic_vbat_comp[i].leak_cur_comp,
			di->ic_vbat_comp[i].comp_mode);
	}
}

static int dc_get_vbat_comp_info(struct dc_comp_device *di)
{
	int i;
	int j;
	bool ic_status = false;
	const char *name = NULL;
	struct dc_ic_dev_info *ic_info_group = dc_ic_get_ic_info();

	if (!ic_info_group)
		return -EPERM;

	for (i = 0; i < CHARGE_IC_MAX_NUM; i++) {
		if (!ic_info_group[i].flag) {
			di->ic_vbat_comp[i].p_index = DC_VBAT_COMP_INDEX_DEFULT;
			di->ic_vbat_comp[i].n_index = DC_VBAT_COMP_INDEX_DEFULT;
			continue;
		}

		ic_status = true;
		name = dc_get_device_name_without_mode(ic_info_group[i].ic_id);
		if (!name)
			return -EPERM;

		hwlog_info("ic[%d] name is %s\n", i, name);
		for (j = 0; j < di->vbat_samp_para_group_size; j++) {
			if (i != di->vbat_samp_para[j].ic_index)
				continue;

			di->ic_vbat_comp[i].p_index = di->vbat_samp_para[j].p_index;
			di->ic_vbat_comp[i].n_index = di->vbat_samp_para[j].n_index;
			break;
		}

		for (j = 0; j < di->ic_para_group_size; j++) {
			if (!strstr(name, di->ic_para[j].ic_name))
				continue;

			di->ic_vbat_comp[i].leak_cur_p = di->ic_para[j].leak_cur_p;
			di->ic_vbat_comp[i].leak_cur_n = di->ic_para[j].leak_cur_n;
			di->ic_vbat_comp[i].comp_mode = di->ic_para[j].comp_mode;
			break;
		}
	}

	if (!ic_status)
		return -EPERM;

	dc_cal_vbat_comp(di);
	di->cal_complete = true;
	return 0;
}

int dc_get_ic_vbat_comp(int ic_role)
{
	int vbat_comp = 0;
	int final_comp_mode = 0;
	struct dc_comp_device *di = g_dc_comp_di;

	if (!di || !di->finish_flag)
		return 0;

	if (!di->cal_complete && dc_get_vbat_comp_info(di))
		return 0;

	if (ic_role >= CHARGE_IC_MAX_NUM)
		return 0;

	final_comp_mode = di->compensation_mode & di->ic_vbat_comp[ic_role].comp_mode;
	if (final_comp_mode & DC_VBAT_COMP_LEAK_CUR)
		vbat_comp += di->ic_vbat_comp[ic_role].leak_cur_comp;
	else if (final_comp_mode & DC_VBAT_COMP_IR)
		vbat_comp -= dc_get_ir_comp(di);

	hwlog_info("final vbat comp[%d]=%d\n", ic_role, vbat_comp);
	return vbat_comp;
}

static void dc_comp_parse_basic_dts(struct device_node *np, struct dc_comp_device *di)
{
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"compensate_r", (u32 *)&di->compensation_r, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"leakage_r", (u32 *)&di->leak_cur_r, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"compensation_mode", (u32 *)&di->compensation_mode, 0);
}

static int dc_comp_parse_leakage_dts(struct device_node *np, struct dc_comp_device *di)
{
	int i, row, col, len, idata;
	const char *tmp_string = NULL;

	len = power_dts_read_count_strings(power_dts_tag(HWLOG_TAG), np,
		"vbat_comp_ic_para", DC_VBAT_COMP_BASE_PARA_MAX,
		DC_VBAT_COMP_BASE_INFO_TOTAL);
	if (len < 0)
		return -EPERM;

	di->ic_para_group_size = len / DC_VBAT_COMP_BASE_INFO_TOTAL;
	for (i = 0; i < len; i++) {
		if (power_dts_read_string_index(power_dts_tag(HWLOG_TAG),
			np, "vbat_comp_ic_para", i, &tmp_string))
			return -EPERM;

		row = i / DC_VBAT_COMP_BASE_INFO_TOTAL;
		col = i % DC_VBAT_COMP_BASE_INFO_TOTAL;
		switch (col) {
		case DC_IC_NAME_BASE:
			strncpy(di->ic_para[row].ic_name,
				tmp_string, DC_IC_NAME_LEN_MAX - 1);
			break;
		case DC_VBAT_COMP_LEAK_CUR_P:
			if (kstrtoint(tmp_string, POWER_BASE_DEC, &idata))
				return -EPERM;
			di->ic_para[row].leak_cur_p = idata;
			break;
		case DC_VBAT_COMP_LEAK_CUR_N:
			if (kstrtoint(tmp_string, POWER_BASE_DEC, &idata))
				return -EPERM;
			di->ic_para[row].leak_cur_n = idata;
			break;
		case DC_VBAT_COMP_MODE:
			if (kstrtoint(tmp_string, POWER_BASE_DEC, &idata))
				return -EPERM;
			di->ic_para[row].comp_mode = idata;
			break;
		default:
			break;
		}
	}

	for (i = 0; i < di->ic_para_group_size; i++)
		hwlog_info("ic_name=%s,ic_vbat_comp_p=%d,ic_vbat_comp_n=%d\n",
			di->ic_para[i].ic_name,
			di->ic_para[i].leak_cur_p,
			di->ic_para[i].leak_cur_n);

	return 0;
}

static int dc_comp_parse_index_dts(struct device_node *np, struct dc_comp_device *di)
{
	int i, row, col, len;
	int idata[DC_VBAT_COMP_INDEX_PARA_MAX * DC_VBAT_COMP_INDEX_INFO_TOTAL] = { 0 };

	len = power_dts_read_string_array(power_dts_tag(HWLOG_TAG), np,
		"vbat_samp_point_para", idata, DC_VBAT_COMP_INDEX_PARA_MAX,
		DC_VBAT_COMP_INDEX_INFO_TOTAL);
	if (len < 0)
		return -EPERM;

	di->vbat_samp_para_group_size = len / DC_VBAT_COMP_INDEX_INFO_TOTAL;
	for (row = 0; row < di->vbat_samp_para_group_size; row++) {
		col = row * DC_VBAT_COMP_INDEX_INFO_TOTAL + DC_IC_ROLE_INDEX;
		di->vbat_samp_para[row].ic_index = idata[col];
		col = row * DC_VBAT_COMP_INDEX_INFO_TOTAL + DC_VBAT_COMP_INDEX_P;
		di->vbat_samp_para[row].p_index = idata[col];
		col = row * DC_VBAT_COMP_INDEX_INFO_TOTAL + DC_VBAT_COMP_INDEX_N;
		di->vbat_samp_para[row].n_index = idata[col];
	}

	for (i = 0; i < di->vbat_samp_para_group_size; i++)
		hwlog_info("ic_index=%d,p_index=%d,n_index=%d\n",
			di->vbat_samp_para[i].ic_index,
			di->vbat_samp_para[i].p_index,
			di->vbat_samp_para[i].n_index);

	return 0;
}

static int dc_comp_probe(struct platform_device *pdev)
{
	int ret;
	struct dc_comp_device *di = NULL;
	struct device_node *np = NULL;

	if (!pdev || !pdev->dev.of_node)
		return -ENODEV;

	di = devm_kzalloc(&pdev->dev, sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	di->dev = &pdev->dev;
	np = di->dev->of_node;
	di->cal_complete = false;

	ret = dc_comp_parse_leakage_dts(np, di);
	if (ret)
		goto fail_free_mem;

	ret = dc_comp_parse_index_dts(np, di);
	if (ret)
		goto fail_free_mem;

	dc_comp_parse_basic_dts(np, di);
	dc_get_vbat_comp_info(di);

	di->finish_flag = 1;
	platform_set_drvdata(pdev, di);
	g_dc_comp_di = di;
	return 0;

fail_free_mem:
	devm_kfree(&pdev->dev, di);
	g_dc_comp_di = NULL;

	return ret;
}

static int dc_comp_remove(struct platform_device *pdev)
{
	struct dc_comp_device *di = platform_get_drvdata(pdev);

	if (!di)
		return -ENODEV;

	devm_kfree(&pdev->dev, di);
	g_dc_comp_di = NULL;

	return 0;
}

static const struct of_device_id dc_comp_match_table[] = {
	{
		.compatible = "direct_charge_comp",
		.data = NULL,
	},
	{},
};

static struct platform_driver dc_comp_driver = {
	.probe = dc_comp_probe,
	.remove = dc_comp_remove,
	.driver = {
		.name = "direct_charge_comp",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(dc_comp_match_table),
	},
};

static int __init dc_comp_init(void)
{
	return platform_driver_register(&dc_comp_driver);
}

static void __exit dc_comp_exit(void)
{
	platform_driver_unregister(&dc_comp_driver);
}

late_initcall_sync(dc_comp_init);
module_exit(dc_comp_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("direct charger with vbat comp driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
