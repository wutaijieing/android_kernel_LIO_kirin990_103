// SPDX-License-Identifier: GPL-2.0
/*
 * mt5785_cali.c
 *
 * mt5785 cali driver
 *
 * Copyright (c) 2022-2022 Huawei Technologies Co., Ltd.
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

#include "mt5785.h"
#include <chipset_common/hwpower/hardware_ic/wireless_ic_icali.h>
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_algorithm.h>
#include <chipset_common/hwpower/charger/charger_common_interface.h>

#define HWLOG_TAG wireless_mt5785_cali
HWLOG_REGIST();

#define MT5785_CALI_SAMPLE_TIME           5
#define MT5785_CALI_ICHIP                 20
#define MT5785_CALI_GET_READ_FLAG_RETRY   10
#define MT5785_CALI_PARA_GAIN             10000
#define MT5785_CALI_PARA_OFFSET           0
#define MT5785_CALI_VOUT_POWERON_DIFF     300
#define MT5785_CALI_VRECT_POWERON_DIFF    100
#define MT5785_CALI_EN_LDO                1
#define MT5785_CALI_DIS_LDO               0
#define MT5785_CALI_CHK_MTP_WR_RETRY      10
#define MT5785_CALI_GAIN_MULTIPLE         10000
#define MT5785_CALI_VOUT_SET              4400
#define MT5785_CALI_WIRE_ON_VOLT          4600

static int mt5785_cali_set_ldo(int value, struct mt5785_dev_info *di)
{
	int ret;

	hwlog_info("[set_ldo] value=%d\n", value);

	if (value)
		ret = mt5785_write_word_mask(di, MT5785_CALI_CMD_ADDR,
			MT5785_CALI_CMD_ENLDO_MASK, MT5785_CALI_CMD_ENLDO_SHIFT, MT5785_CALI_CMD_EN);
	else
		ret = mt5785_write_word_mask(di, MT5785_CALI_CMD_ADDR,
			MT5785_CALI_CMD_DISLDO_MASK, MT5785_CALI_CMD_DISLDO_SHIFT, MT5785_CALI_CMD_EN);

	power_msleep(DT_MSLEEP_50MS, 0, NULL);
	return ret;
}

static int mt5785_cali_get_vout(struct mt5785_dev_info *di, int *vout)
{
	return mt5785_read_word(di, MT5785_CALI_VOUT_ADDR, (u16 *)vout);
}

static int mt5785_cali_get_vrect(struct mt5785_dev_info *di, int *vrect)
{
	return mt5785_read_word(di, MT5785_CALI_VRECT_ADDR, (u16 *)vrect);
}

static int mt5785_cali_set_vout(struct mt5785_dev_info *di, int vout)
{
	int ret;

	ret = mt5785_write_word(di, MT5785_CALI_VOUT_SET_ADDR, (u16)vout);
	ret += mt5785_write_word_mask(di, MT5785_CALI_CMD_ADDR,
		MT5785_CALI_CMD_VOUTSET_MASK, MT5785_CALI_CMD_VOUTSET_SHIFT,
		MT5785_CALI_CMD_EN);
	return ret;
}

static int mt5785_cali_set_read_cmd(struct mt5785_dev_info *di)
{
	int i = 0;
	int ret;
	u16 val = 0;

	ret = mt5785_write_word_mask(di, MT5785_CALI_CMD_ADDR,
		MT5785_CALI_CMD_READ_IOUT_MASK, MT5785_CALI_CMD_READ_IOUT_SHIFT,
		MT5785_CALI_CMD_EN);
	if (ret) {
		hwlog_err("set_read_cmd: write failed\n");
		return -EIO;
	}

	do {
		ret = mt5785_read_word(di, MT5785_CALI_CMD_ADDR, &val);
		i++;
		if ((ret == 0) && (val == 0))
			return 0;
		power_usleep(DT_USLEEP_1MS);
	} while (i < MT5785_CALI_GET_READ_FLAG_RETRY);

	hwlog_err("set_read_cmd: failed val=0x%x ret=%d\n", val, ret);
	return -EIO;
}

static int mt5785_cali_get_current(int *value, void *dev_data)
{
	int ret;
	struct mt5785_dev_info *di = dev_data;

	if (!di || !value)
		return -ENODEV;

	ret = mt5785_cali_set_read_cmd(di);
	ret += mt5785_read_word(di, MT5785_CALI_IOUT_ADDR, (u16 *)value);
	return ret;
}

static int mt5785_cali_check_mtp_write(struct mt5785_dev_info *di)
{
	int ret;
	int i;
	u16 val = 0;
	u32 cnt = 0;

	for (i = 0; i < MT5785_CALI_CHK_MTP_WR_RETRY; i++) {
		power_msleep(DT_MSLEEP_50MS, 0, NULL);
		mt5785_read_dword(di, MT5785_CALI_CNT_ADDR, &cnt); /* read interrupt cnt */
		hwlog_info("check_mtp_write: cnt=%u\n", cnt);
		ret = mt5785_read_word(di, MT5785_CALI_STATUS_ADDR, &val);
		if (ret) {
			hwlog_err("check_mtp_write: read failed\n");
			return -EIO;
		}

		if (val & MT5785_CALI_STATUS_SUCC)
			return 0;
		else if (val & MT5785_CALI_STATUS_FAIL)
			return -EIO;
	}

	hwlog_err("check_mtp_write: failed\n");
	return -EIO;
}

static int mt5785_cali_save_para(int a, int b, struct mt5785_dev_info *di)
{
	int ret;
	u16 a_read = 0;
	s16 b_read = 0;

	hwlog_info("[set_current_gain] a:%d, b:%d\n", a, b);

	ret = mt5785_write_word(di, MT5785_CALI_OFFSET_ADDR, (u16)b);
	ret += mt5785_write_word(di, MT5785_CALI_GAIN_ADDR, (u16)a);
	ret += mt5785_write_word_mask(di, MT5785_CALI_CMD_ADDR,
		MT5785_CALI_CMD_WRITE_MTP_MASK, MT5785_CALI_CMD_WRITE_MTP_SHIFT, MT5785_CALI_CMD_EN);
	if (ret) {
		hwlog_err("set_current_gain: set gain fail\n");
		return ret;
	}

	ret = mt5785_cali_check_mtp_write(di);
	if (ret)
		return ret;

	(void)mt5785_read_word(di, MT5785_CALI_GAIN_ADDR, &a_read);
	(void)mt5785_read_word(di, MT5785_CALI_OFFSET_ADDR, (u16 *)&b_read);
	hwlog_info("[set_current_gain] read back:a=%d, b=%d\n", a_read, b_read);
	di->cali_a = a;
	di->cali_b = b;
	return 0;
}

static int mt5785_cali_reset_para(struct mt5785_dev_info *di)
{
	return mt5785_cali_save_para(MT5785_CALI_PARA_GAIN,
		MT5785_CALI_PARA_OFFSET, di);
}

static int mt5785_cali_reset_chip(struct mt5785_dev_info *di)
{
	int ret;

	ret = mt5785_write_word_mask(di, MT5785_CALI_CMD_ADDR,
		MT5785_CALI_CMD_RESET_MASK, MT5785_CALI_CMD_RESET_SHIFT, MT5785_CALI_CMD_EN);
	if (ret) {
		hwlog_err("reset_chip: write cmd fail\n");
		return ret;
	}

	ret = mt5785_cali_check_mtp_write(di);
	ret += mt5785_cali_reset_para(di);
	return ret;
}

static int mt5785_cali_load_bootloader(struct mt5785_dev_info *di)
{
	int ret;

	ret = mt5785_fw_load_bootloader_cali(di);
	if (ret) {
		hwlog_err("load_bootloader: failed\n");
		return ret;
	}
	hwlog_info("[load_bootloader] succ\n");
	return 0;
}

static int mt5785_cali_get_current_gain(int *value, void *dev_data)
{
	struct mt5785_dev_info *di = dev_data;

	if (!di || !value)
		return -ENODEV;

	*value = di->cali_a;
	return 0;
}

static int mt5785_cali_get_current_offset(int *value, void *dev_data)
{
	struct mt5785_dev_info *di = dev_data;

	if (!di || !value)
		return -ENODEV;

	*value = di->cali_b;
	return 0;
}

static bool mt5785_cali_is_poweron(struct mt5785_dev_info *di)
{
	int vrect = 0;
	int vout = 0;
	int ret;

	ret = mt5785_cali_set_ldo(MT5785_CALI_DIS_LDO, di);
	if (ret) {
		hwlog_err("is_poweron: set ldo disable failed\n");
		return false;
	}

	ret = mt5785_cali_set_read_cmd(di);
	ret = mt5785_cali_get_vout(di, &vout);
	ret += mt5785_cali_get_vrect(di, &vrect);

	hwlog_info("[is_poweron] vout=%d, vrect=%d\n", vout, vrect);
	if (ret) {
		hwlog_err("is_poweron: read failed\n");
		return false;
	}

	if (vout - vrect > MT5785_CALI_VOUT_POWERON_DIFF ||
		abs(vout - vrect) <= MT5785_CALI_VRECT_POWERON_DIFF) {
		hwlog_info("[is_poweron] poweron\n");
		ret = mt5785_cali_set_ldo(MT5785_CALI_EN_LDO, di);
		if (ret) {
			hwlog_err("is_poweron: open ldo failed\n");
			return false;
		}
		return true;
	}

	return false;
}

static void mt5785_check_vbus(struct mt5785_dev_info *di)
{
	int ret;
	int vout = 0;
	int cnt = 10; /* 10 * 100ms */
	int vrect = 0;

	ret = mt5785_cali_set_vout(di, MT5785_CALI_VOUT_SET);
	if (ret) {
		hwlog_err("check_vbus: set vout fail\n");
		return;
	}

	do {
		power_msleep(DT_MSLEEP_100MS, 0, NULL);
		ret = mt5785_cali_set_read_cmd(di);
		ret += mt5785_cali_get_vout(di, &vout);
		ret += mt5785_cali_get_vrect(di, &vrect);
		if (ret) {
			hwlog_err("check_vbus: get vout fail\n");
			return;
		}

		hwlog_info("check_vbus: vout=%d vrect=%d\n", vout, vrect);
		if (vout >= MT5785_CALI_WIRE_ON_VOLT) {
			hwlog_info("check_vbus: wired channel on cnt=%d\n", cnt);
			return;
		}
	} while (--cnt > 0);

	hwlog_err("check_vbus: wired channel off\n");
	return;
}

static void mt5785_cali_exit(void *dev_data)
{
	int ret;
	struct mt5785_dev_info *di = dev_data;

	if (!di) {
		hwlog_err("exit: di invalid\n");
		return;
	}

	wired_chsw_set_wired_channel(WIRED_CHANNEL_ALL, WIRED_CHANNEL_RESTORE);
	mt5785_check_vbus(di);
	charge_set_hiz_enable(HIZ_MODE_DISABLE);
	ret = mt5785_cali_set_ldo(MT5785_CALI_DIS_LDO, di);
	if (ret) {
		hwlog_err("exit: set ldo disable failed\n");
		return;
	}

	hwlog_info("[exit] succ\n");
}

static int mt5785_cali_init(void *dev_data)
{
	int ret;
	struct mt5785_dev_info *di = dev_data;

	if (!di) {
		hwlog_err("init: di invalid\n");
		return -ENODEV;
	}

	charge_set_hiz_enable(HIZ_MODE_ENABLE);
	ret = mt5785_cali_load_bootloader(di);
	if (ret)
		return ret;

	if (!mt5785_cali_is_poweron(di)) {
		hwlog_err("init: poweron failed\n");
		return -EINVAL;
	}

	ret = mt5785_cali_reset_chip(di);
	if (ret) {
		hwlog_err("init: reset_chip failed\n");
		return ret;
	}

	wired_chsw_set_wired_channel(WIRED_CHANNEL_ALL, WIRED_CHANNEL_CUTOFF);
	hwlog_info("[init] succ\n");
	return ret;
}

static int mt5785_cali_get_para(struct wltrx_icali_point point, int *a, int *b)
{
	s64 tmp;

	if (point.x1 - point.x2 == 0) {
		hwlog_err("get_cal_para: param invalid\n");
		return -EINVAL;
	}

	tmp = (s64)(point.y1 - point.y2) * MT5785_CALI_GAIN_MULTIPLE / (point.x1 - point.x2);
	*a = (int)tmp;
	*b = (point.y1 - MT5785_CALI_ICHIP) -
		((*a) * point.x1) / MT5785_CALI_GAIN_MULTIPLE;
	hwlog_info("[get_para] a=%d, b=%d\n", *a, *b);
	return 0;
}

static int mt5785_cali_calibration(struct wltrx_icali_point point, void *dev_data)
{
	int ret;
	int a = 0;
	int b = 0;
	struct mt5785_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	ret = mt5785_cali_get_para(point, &a, &b);
	if (ret)
		return ret;

	ret = mt5785_cali_save_para(a, b, di);
	return ret;
}

static struct wltrx_icali_ops g_wltrx_icali_ops = {
	.init = mt5785_cali_init,
	.exit = mt5785_cali_exit,
	.calibration = mt5785_cali_calibration,
	.get_current = mt5785_cali_get_current,
	.get_current_offset = mt5785_cali_get_current_offset,
	.get_current_gain = mt5785_cali_get_current_gain,
};

int mt5785_cali_ops_register(struct wltrx_ic_ops *ops, struct mt5785_dev_info *di)
{
	if (!ops || !di)
		return -ENODEV;

	ops->cali_ops = kzalloc(sizeof(*(ops->cali_ops)), GFP_KERNEL);
	if (!ops->cali_ops)
		return -ENODEV;

	memcpy(ops->cali_ops, &g_wltrx_icali_ops, sizeof(g_wltrx_icali_ops));
	ops->cali_ops->dev_data = (void *)di;
	return wltrx_icali_ops_register(ops->cali_ops, di->ic_type);
}
