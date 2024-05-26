// SPDX-License-Identifier: GPL-2.0
/*
 * wireless_rx_pmode.c
 *
 * power mode for wireless charging
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

#include <chipset_common/hwpower/wireless_charge/wireless_rx_pmode.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_module.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_acc.h>
#include <chipset_common/hwpower/wireless_charge/wireless_rx_pctrl.h>
#include <huawei_platform/hwpower/wireless_charge/wireless_rx_platform.h>
#include <chipset_common/hwpower/wireless_charge/wireless_dc_check.h>
#include <chipset_common/hwpower/battery/battery_temp.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_dts.h>

#define HWLOG_TAG wireless_rx_pmode
HWLOG_REGIST();

struct wlrx_pmode_dts {
	int pcfg_level;
	struct wlrx_pmode *pcfg;
};

struct wlrx_pmode_dev {
	int curr_pid;
	struct wlrx_pmode_dts *dts;
};

static struct wlrx_pmode_dev *g_rx_pmode_di[WLTRX_DRV_MAX];

static struct wlrx_pmode_dev *wlrx_pmode_get_di(unsigned int drv_type)
{
	if (!wltrx_is_drv_type_valid(drv_type)) {
		hwlog_err("get_di: drv_type=%u err\n", drv_type);
		return NULL;
	}

	return g_rx_pmode_di[drv_type];
}

int wlrx_pmode_get_curr_pid(unsigned int drv_type)
{
	struct wlrx_pmode_dev *di = wlrx_pmode_get_di(drv_type);

	if (!di)
		return 0;

	return di->curr_pid;
}

void wlrx_pmode_set_curr_pid(unsigned int drv_type, int pid)
{
	struct wlrx_pmode_dev *di = wlrx_pmode_get_di(drv_type);

	if (!di || !di->dts || (pid < 0) || (pid >= di->dts->pcfg_level))
		return;

	di->curr_pid = pid;
	hwlog_info("[set_curr_pid] pid=%d\n", pid);
}

bool wlrx_pmode_is_pid_valid(unsigned int drv_type, int pid)
{
	struct wlrx_pmode_dev *di = wlrx_pmode_get_di(drv_type);

	if (!di || !di->dts || (pid < 0) || (pid >= di->dts->pcfg_level))
		return false;

	return true;
}

int wlrx_pmode_get_exp_pid(unsigned int drv_type, int pid)
{
	struct wlrx_pmode_dev *di = wlrx_pmode_get_di(drv_type);

	if (!di || !di->dts || !di->dts->pcfg ||
		(pid < 0) || (pid >= di->dts->pcfg_level))
		return 0;

	return di->dts->pcfg[pid].expect_mode;
}

struct wlrx_pmode *wlrx_pmode_get_pcfg_by_pid(unsigned int drv_type, int pid)
{
	struct wlrx_pmode_dev *di = wlrx_pmode_get_di(drv_type);

	if (!di || !di->dts || !di->dts->pcfg ||
		(pid < 0) || (pid >= di->dts->pcfg_level))
		return NULL;

	return &di->dts->pcfg[pid];
}

struct wlrx_pmode *wlrx_pmode_get_curr_pcfg(unsigned int drv_type)
{
	struct wlrx_pmode_dev *di = wlrx_pmode_get_di(drv_type);

	if (!di)
		return NULL;

	return wlrx_pmode_get_pcfg_by_pid(drv_type, di->curr_pid);
}

int wlrx_pmode_get_pcfg_level(unsigned int drv_type)
{
	struct wlrx_pmode_dev *di = wlrx_pmode_get_di(drv_type);

	if (!di || !di->dts)
		return 0;

	return di->dts->pcfg_level;
}

int wlrx_pmode_get_pid_by_name(unsigned int drv_type, const char *mode_name)
{
	int pid;
	struct wlrx_pmode_dev *di = wlrx_pmode_get_di(drv_type);

	if (!mode_name || !di || !di->dts || !di->dts->pcfg)
		return 0;

	for (pid = 0; pid < di->dts->pcfg_level; pid++) {
		if (!strncmp(mode_name, di->dts->pcfg[pid].name,
			strlen(di->dts->pcfg[pid].name)))
			return pid;
	}

	return 0;
}

static bool wlrx_pmode_quick_judge(unsigned int drv_type, struct wlrx_pmode *pcfg)
{
	struct wlprot_acc_cap *acc_cap = wlrx_acc_get_cap(drv_type);
	struct wlrx_pctrl *product_pcfg = wlrx_pctrl_get_product_cfg(drv_type);

	if (!acc_cap || !product_pcfg)
		return false;

	if ((acc_cap->vmax < pcfg->vtx_min) || (product_pcfg->vtx < pcfg->vtx) ||
		(product_pcfg->vrx < pcfg->vrx) || (product_pcfg->irx < pcfg->irx))
		return false;
	if (acc_cap->vmax * acc_cap->imax < pcfg->vtx_min * pcfg->itx_min)
		return false;

	return true;
}

static bool wlrx_pmode_normal_judge(unsigned int drv_type, struct wlrx_pmode *pcfg)
{
	if ((pcfg->auth >= 0) && !wlrx_acc_auth_succ(drv_type))
		return false;

	return true;
}

static bool wlrx_pmode_final_judge(unsigned int drv_type, struct wlrx_pmode *pcfg)
{
	int tbatt = 0;
	struct wlprot_acc_cap *acc_cap = wlrx_acc_get_cap(drv_type);

	if (!acc_cap)
		return false;
	/* if acc not support 12V, 12V pmode will judge fail */
	if (!acc_cap->support_12v && (pcfg->vtx == 12000))
		return false;

	bat_temp_get_temperature(BAT_TEMP_MIXED, &tbatt);
	if ((pcfg->tbatt >= 0) && (tbatt >= pcfg->tbatt))
		return false;

	return true;
}

bool wlrx_pmode_judge(unsigned int drv_type, int pid, unsigned int judge_type)
{
	struct wlrx_pmode *pcfg = NULL;

	if (!wlrx_pmode_is_pid_valid(drv_type, pid))
		return false;

	pcfg = wlrx_pmode_get_pcfg_by_pid(drv_type, pid);
	if (!pcfg) {
		hwlog_err("judge: input err\n");
		return false;
	}

	switch (judge_type) {
	case WLDC_PMODE_FINAL_JUDGE:
	case WLRX_PMODE_FINAL_JUDGE:
		if (!wlc_pmode_final_judge(drv_type, pid, pcfg))
			return false;
		if (!wlrx_pmode_final_judge(drv_type, pcfg))
			return false;
		/* fall-through */
	case WLRX_PMODE_NORMAL_JUDGE:
		if (!wlrx_pmode_normal_judge(drv_type, pcfg))
			return false;
		/* fall-through */
	case WLRX_PMODE_QUICK_JUDGE:
		if (!wlrx_pmode_quick_judge(drv_type, pcfg))
			return false;
		break;
	default:
		hwlog_err("judge: type=%u err\n", judge_type);
		return false;
	}

	if ((judge_type == WLRX_PMODE_FINAL_JUDGE) && strstr(pcfg->name, "SC")) {
		if (!wlrx_pmode_dc_judge_crit(pcfg->name, pid))
			return false;
	}

	return true;
}

static int wlrx_pmode_cfg_str2int(const char *str, int *pcfg, int i)
{
	if (kstrtoint(str, 0, &pcfg[(i - 1) % WLRX_PMODE_CFG_COL]))
		return -EINVAL;

	return 0;
}

static int wlrx_pmode_parse_pcfg(const struct device_node *np, struct wlrx_pmode_dts *dts)
{
	int i, len, size;
	const char *tmp_str = NULL;
	struct wlrx_pmode *pcfg = NULL;

	len = power_dts_read_count_strings(power_dts_tag(HWLOG_TAG), np,
		"rx_mode_para", WLRX_PMODE_CFG_ROW, WLRX_PMODE_CFG_COL);
	if (len <= 0)
		return -EINVAL;

	size = sizeof(*dts->pcfg) * (len / WLRX_PMODE_CFG_COL);
	dts->pcfg = kzalloc(size, GFP_KERNEL);
	if (!dts->pcfg)
		return -ENOMEM;

	dts->pcfg_level = len / WLRX_PMODE_CFG_COL;
	for (i = 0; i < len; i++) {
		if (power_dts_read_string_index(power_dts_tag(HWLOG_TAG), np,
			"rx_mode_para", i, &tmp_str))
			return -EINVAL;
		if ((i % WLRX_PMODE_CFG_COL) == 0) { /* 0: pmode name */
			dts->pcfg[i / WLRX_PMODE_CFG_COL].name = tmp_str;
			continue;
		}
		if (wlrx_pmode_cfg_str2int(tmp_str,
			(int *)&dts->pcfg[i / WLRX_PMODE_CFG_COL].vtx_min, i))
			return -EINVAL;
	}
	for (i = 0; i < dts->pcfg_level; i++) {
		pcfg = &dts->pcfg[i];
		hwlog_info("pmode[%d] name:%-4s vtx_min:%-5d itx_min:%-4d\t"
			"vtx:%-5d vrx:%-5d irx:%-4d vrect_lth:%-5d tbatt:%-3d\t"
			"cable:%-2d auth:%-2d icon:%u timeout:%-4d expect_mode:%-2d\n",
			i, pcfg->name, pcfg->vtx_min, pcfg->itx_min,
			pcfg->vtx, pcfg->vrx, pcfg->irx, pcfg->vrect_lth,
			pcfg->tbatt, pcfg->cable, pcfg->auth, pcfg->icon,
			pcfg->timeout, pcfg->expect_mode);
	}
	return 0;
}

static int wlrx_pmode_parse_dts(const struct device_node *np, struct wlrx_pmode_dts **dts)
{
	*dts = kzalloc(sizeof(**dts), GFP_KERNEL);
	if (!*dts)
		return -ENOMEM;

	return wlrx_pmode_parse_pcfg(np, *dts);
}

static void wlrx_pmode_kfree_dev(struct wlrx_pmode_dev *di)
{
	if (!di)
		return;

	if (di->dts) {
		kfree(di->dts->pcfg);
		kfree(di->dts);
	}

	kfree(di);
}

int wlrx_pmode_init(unsigned int drv_type, struct device *dev)
{
	int ret;
	struct wlrx_pmode_dev *di = NULL;

	if (!dev || !wltrx_is_drv_type_valid(drv_type))
		return -EINVAL;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	ret = wlrx_pmode_parse_dts(dev->of_node, &di->dts);
	if (ret)
		goto exit;

	g_rx_pmode_di[drv_type] = di;
	return 0;

exit:
	wlrx_pmode_kfree_dev(di);
	return ret;
}

void wlrx_pmode_deinit(unsigned int drv_type)
{
	if (!wltrx_is_drv_type_valid(drv_type))
		return;

	wlrx_pmode_kfree_dev(g_rx_pmode_di[drv_type]);
	g_rx_pmode_di[drv_type] = NULL;
}
