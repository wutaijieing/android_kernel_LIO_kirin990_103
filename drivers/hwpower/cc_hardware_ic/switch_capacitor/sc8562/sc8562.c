// SPDX-License-Identifier: GPL-2.0
/*
 * sc8562.c
 *
 * sc8562 driver
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

#include "sc8562.h"
#include "sc8562_i2c.h"
#include "sc8562_scp.h"
#include <linux/delay.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <chipset_common/hwpower/common_module/power_pinctrl.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_device_id.h>

#define HWLOG_TAG sc8562_chg
HWLOG_REGIST();

static void sc8562_dump_register(struct sc8562_device_info *di)
{
	u8 i;
	u8 val = 0;

	for (i = SC8562_VBAT_OVP_REG; i <= SC8562_PMID2OUT_UVP_REG; i++) {
		if (!sc8562_read_byte(di, i, &val))
			hwlog_info("reg[0x%x]=0x%x", i, val);
	}
}

static int sc8562_discharge(int enable, void *dev_data)
{
	return 0;
}

static int sc8562_is_device_close(void *dev_data)
{
	u8 val = 0;
	struct sc8562_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	if (sc8562_read_byte(di, SC8562_CONVERTER_STATE_REG, &val))
		return -EIO;

	if (val & SC8562_CP_SWITCHING_STAT_MASK)
		return 0;

	return 1; /* 1:ic is closed */
}

static int sc8562_get_device_id(void *dev_data)
{
	u8 part_info = 0;
	int ret;
	struct sc8562_device_info *di = dev_data;

	if (!di)
		return SC8562_DEVICE_ID_GET_FAIL;

	if (di->get_id_time == SC8562_USED)
		return di->device_id;

	di->get_id_time = SC8562_USED;
	ret = sc8562_read_byte(di, SC8562_DEVICE_ID_REG, &part_info);
	if (ret) {
		di->get_id_time = SC8562_NOT_USED;
		hwlog_err("get_device_id read failed\n");
		return SC8562_DEVICE_ID_GET_FAIL;
	}

	if (part_info == SC8562_DEVICE_ID_SC8562)
		di->device_id = SWITCHCAP_SC8562;
	else
		di->device_id = SC8562_DEVICE_ID_GET_FAIL;

	hwlog_info("get_device_id [%x]=0x%x, device_id: 0x%x\n",
		SC8562_DEVICE_ID_REG, part_info, di->device_id);

	return di->device_id;
}

static int sc8562_get_vbat_mv(void *dev_data)
{
	s16 data = 0;
	int vbat;
	struct sc8562_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	if (sc8562_read_word(di, SC8562_VBAT_ADC1_REG, &data))
		return -EIO;

	/* VBAT ADC LBS: 1.25mV */
	vbat = (int)data * 125 / 100;

	hwlog_info("VBAT_ADC=0x%x, vbat=%d\n", data, vbat);

	return vbat;
}

static int sc8562_get_ibat_ma(int *ibat, void *dev_data)
{
	s16 data = 0;
	int ibat_ori;
	struct sc8562_device_info *di = dev_data;

	if (!ibat || !di)
		return -EPERM;

	if (sc8562_read_word(di, SC8562_IBAT_ADC1_REG, &data))
		return -EIO;

	/* IBAT ADC LBS: 3.125mA */
	ibat_ori = (int)data * 3125 / 1000;
	*ibat = ibat_ori * di->sense_r_config;
	*ibat /= di->sense_r_actual;

	hwlog_info("IBAT_ADC=0x%x ibat_ori=%d ibat=%d\n", data, ibat_ori, *ibat);

	return 0;
}

static int sc8562_get_ibus_ma(int *ibus, void *dev_data)
{
	s16 data = 0;
	struct sc8562_device_info *di = dev_data;

	if (!di || !ibus)
		return -EPERM;

	if (sc8562_read_word(di, SC8562_IBUS_ADC1_REG, &data))
		return -EIO;

	/* IBUS ADC LBS: 1.5625mA */
	*ibus = (int)data * 6400 / 4096;

	hwlog_info("IBUS_ADC=0x%x, ibus=%d\n", data, *ibus);

	return 0;
}

int sc8562_get_vbus_mv(int *vbus, void *dev_data)
{
	s16 data = 0;
	struct sc8562_device_info *di = dev_data;

	if (!di || !vbus)
		return -EPERM;

	if (sc8562_read_word(di, SC8562_VBUS_ADC1_REG, &data))
		return -EIO;

	/* VBUS ADC LBS: 6.25mV */
	*vbus = (int)data * 6250 / 1000;

	hwlog_info("VBUS_ADC=0x%x, vbus=%d\n", data, *vbus);

	return 0;
}

static int sc8562_get_vusb_mv(int *vusb, void *dev_data)
{
	s16 data = 0;
	struct sc8562_device_info *di = dev_data;

	if (!vusb || !di)
		return -EPERM;

	if (sc8562_read_word(di, SC8562_VUSB_ADC1_REG, &data))
		return -EIO;

	/* VUSB_ADC LSB: 6.25mV */
	*vusb = (int)data * 625 / 100;

	hwlog_info("VUSB_ADC=0x%x, vusb=%d\n", data, *vusb);

	return 0;
}

static int sc8562_get_device_temp(int *temp, void *dev_data)
{
	s16 data = 0;
	struct sc8562_device_info *di = dev_data;

	if (!temp || !di)
		return -EPERM;

	if (sc8562_read_word(di, SC8562_TDIE_ADC1_REG, &data))
		return -EIO;

	/* TDIE_ADC LSB: 0.5C */
	*temp = (int)data * 5 / 10;

	hwlog_info("TDIE_ADC=0x%x temp=%d\n", data, *temp);

	return 0;
}

static int sc8562_get_vout_mv(int *vout, void *dev_data)
{
	s16 data = 0;
	struct sc8562_device_info *di = dev_data;

	if (!vout || !di)
		return -EPERM;

	if (sc8562_read_word(di, SC8562_VOUT_ADC1_REG, &data))
		return -EIO;

	/* VOUT_ADC LSB: 1.25mV */
	*vout = (int)data * 125 / 100;

	hwlog_info("VOUT_ADC=0x%x, vout=%d\n", data, *vout);

	return 0;
}

static int sc8562_config_watchdog_ms(int time, void *dev_data)
{
	u8 val;
	u8 reg = 0;
	int ret;
	struct sc8562_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	if (time >= SC8562_WD_TMR_TIMING_30000MS)
		val = SC8562_WD_TMR_30000MS;
	else if (time >= SC8562_WD_TMR_TIMING_5000MS)
		val = SC8562_WD_TMR_5000MS;
	else if (time >= SC8562_WD_TMR_TIMING_1000MS)
		val = SC8562_WD_TMR_1000MS;
	else if (time >= SC8562_WD_TMR_TIMING_500MS)
		val = SC8562_WD_TMR_500MS;
	else if (time >= SC8562_WD_TMR_TIMING_200MS)
		val = SC8562_WD_TMR_200MS;
	else
		val = SC8562_WD_TMR_TIMING_DIS;

	ret = sc8562_write_mask(di, SC8562_CTRL3_REG, SC8562_CTRL3_WD_TIMEOUT_MASK,
		SC8562_CTRL3_WD_TIMEOUT_SHIFT, val);
	ret += sc8562_read_byte(di, SC8562_CTRL3_REG, &reg);
	if (ret)
		return -EIO;

	hwlog_info("config_watchdog_ms [%x]=0x%x\n", SC8562_CTRL3_REG, reg);
	return 0;
}

static int sc8562_kick_watchdog_ms(void *dev_data)
{
	return 0;
}

static int sc8562_config_vbat_ovp_th_mv(struct sc8562_device_info *di, int ovp_th)
{
	u8 vbat;
	int ret;

	if (ovp_th < SC8562_VBAT_OVP_MIN)
		ovp_th = SC8562_VBAT_OVP_MIN;

	if (ovp_th > SC8562_VBAT_OVP_MAX)
		ovp_th = SC8562_VBAT_OVP_MAX;

	vbat = (u8)((ovp_th - SC8562_VBAT_OVP_MIN) / SC8562_VBAT_OVP_STEP);
	ret = sc8562_write_mask(di, SC8562_VBAT_OVP_REG, SC8562_VBAT_OVP_TH_MASK,
		SC8562_VBAT_OVP_TH_SHIFT, vbat);
	ret += sc8562_read_byte(di, SC8562_VBAT_OVP_REG, &vbat);
	if (ret)
		return -EIO;

	hwlog_info("config_vbat_ovp_threshold_mv [%x]=0x%x\n",
		SC8562_VBAT_OVP_REG, vbat);

	return 0;
}

static int sc8562_config_ibat_ocp_th_ma(struct sc8562_device_info *di, int ocp_th)
{
	u8 value;
	int ret;

	if (ocp_th < SC8562_IBAT_OCP_MIN)
		ocp_th = SC8562_IBAT_OCP_MIN;

	if (ocp_th > SC8562_IBAT_OCP_MAX)
		ocp_th = SC8562_IBAT_OCP_MAX;

	value = (u8)((ocp_th - SC8562_IBAT_OCP_MIN) / SC8562_IBAT_OCP_STEP);
	ret = sc8562_write_mask(di, SC8562_IBAT_OCP_REG, SC8562_IBAT_OCP_TH_MASK,
		SC8562_IBAT_OCP_TH_SHIFT, value);
	ret += sc8562_read_byte(di, SC8562_IBAT_OCP_REG, &value);
	if (ret)
		return -EIO;

	hwlog_info("config_ibat_ocp_threshold_ma [%x]=0x%x\n",
		SC8562_IBAT_OCP_REG, value);
	return 0;
}

int sc8562_config_vusb_ovp_th_mv(struct sc8562_device_info *di, int ovp_th)
{
	u8 value;
	int ret;

	if (ovp_th < SC8562_VUSB_OVP_MIN) {
		ovp_th = SC8562_VUSB_OVP_DEF;
		value = SC8562_VUSB_OVP_DEF_VAL;
	} else if (ovp_th > SC8562_VUSB_OVP_MAX) {
		ovp_th = SC8562_VUSB_OVP_MAX;
		value = (u8)((ovp_th - SC8562_VUSB_OVP_MIN) / SC8562_VUSB_OVP_STEP);
	} else {
		value = (u8)((ovp_th - SC8562_VUSB_OVP_MIN) / SC8562_VUSB_OVP_STEP);
	}
	ret = sc8562_write_mask(di, SC8562_VUSB_OVP_REG, SC8562_VUSB_OVP_TH_MASK,
		SC8562_VUSB_OVP_TH_SHIFT, value);
	ret += sc8562_read_byte(di, SC8562_VUSB_OVP_REG, &value);
	if (ret)
		return -EIO;

	hwlog_info("config_vusb_ovp_threshold_mv [%x]=0x%x\n",
		SC8562_VUSB_OVP_REG, value);
	return 0;
}

int sc8562_config_vout_ovp_th_mv(struct sc8562_device_info *di, int ovp_th)
{
	u8 value;
	int ret;

	if (ovp_th < SC8562_VOUT_OVP_MIN)
		ovp_th = SC8562_VOUT_OVP_MIN;

	if (ovp_th > SC8562_VOUT_OVP_MAX)
		ovp_th = SC8562_VOUT_OVP_MAX;

	value = (u8)((ovp_th - SC8562_VOUT_OVP_MIN) / SC8562_VOUT_OVP_STEP);
	ret = sc8562_write_mask(di, SC8562_VOUT_VBUS_OVP_REG, SC8562_VOUT_OVP_TH_MASK,
		SC8562_VOUT_OVP_TH_SHIFT, value);
	ret += sc8562_read_byte(di, SC8562_VOUT_VBUS_OVP_REG, &value);
	if (ret)
		return -EIO;

	hwlog_info("config_vout_ovp_threshold_mv [%x]=0x%x\n",
		SC8562_VOUT_VBUS_OVP_REG, value);
	return 0;
}

int sc8562_config_vbus_ovp_th_mv(struct sc8562_device_info *di, int ovp_th, int mode)
{
	u8 value;
	int ret;

	switch (mode) {
	case SC8562_CHG_FBYPASS_MODE:
		if (ovp_th < SC8562_VBUS_OVP_FBPSC_MIN)
			ovp_th = SC8562_VBUS_OVP_FBPSC_MIN;

		if (ovp_th > SC8562_VBUS_OVP_FBPSC_MAX)
			ovp_th = SC8562_VBUS_OVP_FBPSC_MAX;

		value = (u8)((ovp_th - SC8562_VBUS_OVP_FBPSC_MIN) / SC8562_VBUS_OVP_FBPSC_STEP);
		break;
	case SC8562_CHG_F21SC_MODE:
		if (ovp_th < SC8562_VBUS_OVP_F21SC_MIN)
			ovp_th = SC8562_VBUS_OVP_F21SC_MIN;

		if (ovp_th > SC8562_VBUS_OVP_F21SC_MAX)
			ovp_th = SC8562_VBUS_OVP_F21SC_MAX;

		value = (u8)((ovp_th - SC8562_VBUS_OVP_F21SC_MIN) / SC8562_VBUS_OVP_F21SC_STEP);
		break;
	case SC8562_CHG_F41SC_MODE:
		if (ovp_th < SC8562_VBUS_OVP_F41SC_MIN)
			ovp_th = SC8562_VBUS_OVP_F41SC_MIN;

		if (ovp_th > SC8562_VBUS_OVP_F41SC_MAX)
			ovp_th = SC8562_VBUS_OVP_F41SC_MAX;

		value = (u8)((ovp_th - SC8562_VBUS_OVP_F41SC_MIN) / SC8562_VBUS_OVP_F41SC_STEP);
		break;
	default:
		if (ovp_th < SC8562_VBUS_OVP_F21SC_MIN)
			ovp_th = SC8562_VBUS_OVP_F21SC_MIN;

		if (ovp_th > SC8562_VBUS_OVP_F21SC_MAX)
			ovp_th = SC8562_VBUS_OVP_F21SC_MAX;

		value = (u8)((ovp_th - SC8562_VBUS_OVP_F21SC_MIN) / SC8562_VBUS_OVP_F21SC_STEP);
		break;
	}

	ret = sc8562_write_mask(di, SC8562_VOUT_VBUS_OVP_REG, SC8562_VBUS_OVP_TH_MASK,
		SC8562_VBUS_OVP_TH_SHIFT, value);
	ret += sc8562_read_byte(di, SC8562_VOUT_VBUS_OVP_REG, &value);
	if (ret)
		return -EIO;

	hwlog_info("config_vbus_ovp_threshole_mv [%x]=0x%x\n",
		SC8562_VOUT_VBUS_OVP_REG, value);
	return 0;
}

static int sc8562_config_ibus_ocp_th_ma(struct sc8562_device_info *di, int ocp_th)
{
	u8 value;
	int ret;

	if (ocp_th < SC8562_IBUS_OCP_MIN)
		ocp_th = SC8562_IBUS_OCP_MIN;

	if (ocp_th > SC8562_IBUS_OCP_MAX)
		ocp_th = SC8562_IBUS_OCP_MAX;

	value = (u8)((ocp_th - SC8562_IBUS_OCP_MIN) / SC8562_IBUS_OCP_STEP);
	ret = sc8562_write_mask(di, SC8562_IBUS_OCP_REG,
		SC8562_IBUS_OCP_TH_MASK, SC8562_IBUS_OCP_TH_SHIFT, value);
	ret += sc8562_read_byte(di, SC8562_IBUS_OCP_REG, &value);
	if (ret)
		return -EIO;

	hwlog_info("config_ibus_ocp_threshold_ma [%x]=0x%x\n",
		SC8562_IBUS_OCP_REG, value);
	return 0;
}

static int sc8562_config_switching_frequency(int data, struct sc8562_device_info *di)
{
	u8 freq;
	int freq_shift, ret;

	if (data < SC8562_SW_FREQ_MIN)
		data = SC8562_SW_FREQ_MIN;

	if (data > SC8562_SW_FREQ_MAX)
		data = SC8562_SW_FREQ_MAX;

	freq = (u8)((data - SC8562_SW_FREQ_MIN) / SC8562_SW_FREQ_STEP);
	freq_shift = SC8562_SW_FREQ_SHIFT_NORMAL;
	ret = sc8562_write_mask(di, SC8562_CTRL2_REG, SC8562_CTRL2_FSW_SET_MASK,
		SC8562_CTRL2_FSW_SET_SHIFT, freq);
	ret += sc8562_write_mask(di, SC8562_CTRL2_REG, SC8562_CTRL2_FREQ_DITHER_MASK,
		SC8562_CTRL2_FREQ_DITHER_SHIFT, freq_shift);
	if (ret)
		return -EIO;

	hwlog_info("config_switching_frequency [%x]=0x%x\n",
		SC8562_CTRL2_REG, freq);
	hwlog_info("config_adjustable_switching_frequency [%x]=0x%x\n",
		SC8562_CTRL2_REG, freq_shift);

	return 0;
}

static int sc8562_config_ibat_sns_res(struct sc8562_device_info *di)
{
	u8 res_config;
	int ret;

	if (di->sense_r_config == SENSE_R_1_MOHM)
		res_config = SC8562_IBAT_SNS_RES_1MOHM;
	else
		res_config = SC8562_IBAT_SNS_RES_2MOHM;

	ret = sc8562_write_mask(di, SC8562_CTRL4_REG, SC8562_CTRL4_SET_IBAT_SNS_RES_MASK,
		SC8562_CTRL4_SET_IBAT_SNS_RES_SHIFT, res_config);
	if (ret)
		return -EIO;

	hwlog_info("congfig_ibat_sns_res=%d\n", di->sense_r_config);
	return 0;
}

static int sc8562_threshold_reg_init(struct sc8562_device_info *di, u8 mode)
{
	int ret, vbus_ovp, vusb_ovp, ibat_ocp, ibus_ocp;

	if (mode == SC8562_CHG_FBYPASS_MODE) {
		vbus_ovp = SC8562_VBUS_OVP_FBPSC_INIT;
		vusb_ovp = SC8562_VUSB_OVP_DEF;
		ibat_ocp = SC8562_IBAT_OCP_FBPSC_INIT;
		ibus_ocp = SC8562_IBUS_OCP_FBPSC_INIT;
	} else if (mode == SC8562_CHG_F21SC_MODE) {
		vbus_ovp = SC8562_VBUS_OVP_F21SC_INIT;
		vusb_ovp = SC8562_VUSB_OVP_F21SC_INIT;
		ibat_ocp = SC8562_IBAT_OCP_F21SC_INIT;
		ibus_ocp = SC8562_IBUS_OCP_F21SC_INIT;
	} else if (mode == SC8562_CHG_F41SC_MODE) {
		vbus_ovp = SC8562_VBUS_OVP_F41SC_INIT;
		vusb_ovp = SC8562_VUSB_OVP_F41SC_INIT;
		ibat_ocp = SC8562_IBAT_OCP_F41SC_INIT;
		ibus_ocp = SC8562_IBUS_OCP_F41SC_INIT;
	} else {
		vbus_ovp = SC8562_VBUS_OVP_F21SC_INIT;
		vusb_ovp = SC8562_VUSB_OVP_F21SC_INIT;
		ibat_ocp = SC8562_IBAT_OCP_F21SC_INIT;
		ibus_ocp = SC8562_IBUS_OCP_F21SC_INIT;
	}

	ret = sc8562_config_vusb_ovp_th_mv(di, vusb_ovp);
	ret += sc8562_config_vout_ovp_th_mv(di, SC8562_VOUT_OVP_INIT);
	ret += sc8562_config_vbus_ovp_th_mv(di, vbus_ovp, mode);
	ret += sc8562_config_ibus_ocp_th_ma(di, ibus_ocp);
	ret += sc8562_config_vbat_ovp_th_mv(di, SC8562_VBAT_OVP_INIT);
	ret += sc8562_config_ibat_ocp_th_ma(di, ibat_ocp);
	if (ret)
		hwlog_err("protect threshold init failed\n");

	di->charge_mode = mode;
	return ret;
}

static int sc8562_lvc_charge_enable(int enable, void *dev_data)
{
	int ret;
	u8 ctrl1_reg = 0;
	u8 ctrl4_reg = 0;
	u8 chg_en;
	struct sc8562_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	chg_en = enable ? SC8562_CTRL1_CP_ENABLE : SC8562_CTRL1_CP_DISABLE;
	ret = sc8562_write_mask(di, SC8562_CTRL1_REG, SC8562_CTRL1_CP_EN_MASK,
		SC8562_CTRL1_CP_EN_SHIFT, SC8562_CTRL1_CP_DISABLE);
	ret += sc8562_write_mask(di, SC8562_CTRL4_REG, SC8562_CTRL4_MODE_MASK,
		SC8562_CTRL4_MODE_SHIFT, SC8562_CHG_FBYPASS_MODE);
	ret += sc8562_write_byte(di, SC8562_IBUS_UCP_REG, SC8562_IBUS_UCP_DIS_MASK_DISABLE);
	ret += sc8562_write_mask(di, SC8562_CTRL1_REG, SC8562_CTRL1_CP_EN_MASK,
		SC8562_CTRL1_CP_EN_SHIFT, chg_en);
	ret += sc8562_read_byte(di, SC8562_CTRL1_REG, &ctrl1_reg);
	ret += sc8562_read_byte(di, SC8562_CTRL4_REG, &ctrl4_reg);

	if (ctrl1_reg)
		ret += sc8562_write_byte(di, SC8562_IBUS_UCP_REG, SC8562_IBUS_UCP_REG_INIT);
	if (ret)
		return -EIO;

	hwlog_info("ic_role=%d, lvc_charge_enable [%x]=0x%x, [%x]=0x%x\n",
		di->ic_role, SC8562_CTRL1_REG, ctrl1_reg,
		SC8562_CTRL4_REG, ctrl4_reg);
	return 0;
}

static int sc8562_sc_charge_enable(int enable, void *dev_data)
{
	int ret;
	u8 ctrl1_reg = 0;
	u8 ctrl4_reg = 0;
	u8 chg_en;
	struct sc8562_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	chg_en = enable ? SC8562_CTRL1_CP_ENABLE : SC8562_CTRL1_CP_DISABLE;
	ret = sc8562_write_mask(di, SC8562_CTRL1_REG, SC8562_CTRL1_CP_EN_MASK,
		SC8562_CTRL1_CP_EN_SHIFT, SC8562_CTRL1_CP_DISABLE);
	ret += sc8562_write_mask(di, SC8562_CTRL4_REG, SC8562_CTRL4_MODE_MASK,
		SC8562_CTRL4_MODE_SHIFT, SC8562_CHG_F21SC_MODE);
	ret += sc8562_write_byte(di, SC8562_IBUS_UCP_REG, SC8562_IBUS_UCP_DIS_MASK_DISABLE);
	ret += sc8562_write_mask(di, SC8562_CTRL1_REG, SC8562_CTRL1_CP_EN_MASK,
		SC8562_CTRL1_CP_EN_SHIFT, chg_en);
	ret += sc8562_read_byte(di, SC8562_CTRL1_REG, &ctrl1_reg);
	ret += sc8562_read_byte(di, SC8562_CTRL4_REG, &ctrl4_reg);

	if (ctrl1_reg)
		ret += sc8562_write_byte(di, SC8562_IBUS_UCP_REG, SC8562_IBUS_UCP_REG_INIT);
	if (ret)
		return -EIO;

	hwlog_info("ic_role=%d, sc_charge_enable [%x]=0x%x, [%x]=0x%x\n",
		di->ic_role, SC8562_CTRL1_REG, ctrl1_reg,
		SC8562_CTRL4_REG, ctrl4_reg);
	return 0;
}

static int sc8562_sc4_charge_enable(int enable, void *dev_data)
{
	int ret;
	u8 ctrl1_reg = 0;
	u8 ctrl4_reg = 0;
	u8 chg_en;
	struct sc8562_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	chg_en = enable ? SC8562_CTRL1_CP_ENABLE : SC8562_CTRL1_CP_DISABLE;
	ret = sc8562_write_mask(di, SC8562_CTRL1_REG, SC8562_CTRL1_CP_EN_MASK,
		SC8562_CTRL1_CP_EN_SHIFT, SC8562_CTRL1_CP_DISABLE);
	ret += sc8562_write_mask(di, SC8562_CTRL4_REG, SC8562_CTRL4_MODE_MASK,
		SC8562_CTRL4_MODE_SHIFT, SC8562_CHG_F41SC_MODE);
	ret += sc8562_write_byte(di, SC8562_IBUS_UCP_REG, SC8562_IBUS_UCP_DIS_MASK_DISABLE);
	ret += sc8562_write_mask(di, SC8562_CTRL1_REG, SC8562_CTRL1_CP_EN_MASK,
		SC8562_CTRL1_CP_EN_SHIFT, chg_en);
	ret += sc8562_read_byte(di, SC8562_CTRL1_REG, &ctrl1_reg);
	ret += sc8562_read_byte(di, SC8562_CTRL4_REG, &ctrl4_reg);

	if (ctrl1_reg)
		ret += sc8562_write_byte(di, SC8562_IBUS_UCP_REG, SC8562_IBUS_UCP_REG_INIT);
	if (ret)
		return -EIO;

	hwlog_info("ic_role=%d, sc4_charge_enable [%x]=0x%x, [%x]=0x%x\n",
		di->ic_role, SC8562_CTRL1_REG, ctrl1_reg,
		SC8562_CTRL4_REG, ctrl4_reg);
	return 0;
}

static int sc8562_reg_reset(struct sc8562_device_info *di)
{
	int ret;
	u8 ctrl4_reg = 0;

	ret = sc8562_write_mask(di, SC8562_CTRL4_REG,
		SC8562_CTRL4_REG_RST_MASK, SC8562_CTRL4_REG_RST_SHIFT,
		SC8562_REG_RST_ENABLE);
	if (ret)
		return -EIO;

	power_usleep(DT_USLEEP_1MS);
	ret = sc8562_read_byte(di, SC8562_CTRL4_REG, &ctrl4_reg);
	if (ret)
		return -EIO;

	di->charge_mode = SC8562_CHG_F21SC_MODE;
	hwlog_info("reg_reset [%x]=0x%x\n", SC8562_CTRL4_REG, ctrl4_reg);
	return 0;
}

static int sc8562_chip_init(void *dev_data)
{
	return 0;
}

static int sc8562_reg_init(struct sc8562_device_info *di)
{
	int ret;

	ret = sc8562_config_watchdog_ms(SC8562_WD_TMR_TIMING_5000MS, di);
	ret += sc8562_config_ibat_sns_res(di);
	ret += sc8562_write_byte(di, SC8562_INT_MASK_REG,
		SC8562_INT_MASK_REG_INIT);
	ret += sc8562_write_mask(di, SC8562_ADC_CTRL_REG,
		SC8562_ADC_CTRL_ADC_EN_MASK, SC8562_ADC_CTRL_ADC_EN_SHIFT, SC8562_ADC_ENABLE);
	ret += sc8562_write_mask(di, SC8562_ADC_CTRL_REG,
		SC8562_ADC_CTRL_ADC_RATE_MASK, SC8562_ADC_CTRL_ADC_RATE_SHIFT, SC8562_ADC_DISABLE);
	ret += sc8562_write_mask(di, SC8562_DPDM_CTRL2_REG,
		SC8562_DP_BUFF_EN_MASK, SC8562_DP_BUFF_EN_SHIFT, SC8562_DP_BUFF_EN_ENABLE);
	ret += sc8562_write_byte(di, SC8562_SCP_FLAG_MASK_REG,
		SC8562_SCP_FLAG_MASK_REG_INIT);
	ret += gpio_direction_output(di->gpio_enable, SC8562_GPIO_ENABLE);
	ret += sc8562_write_mask(di, SC8562_FUN_DIS_REG,
		SC8562_TSBAT_FLT_DIS_MASK, SC8562_TSBAT_FLT_DIS_SHIFT, SC8562_TSBAT_FLT_DIS_DISABLE);
	ret += sc8562_write_mask(di, SC8562_CTRL3_REG,
		SC8562_CTRL3_SS_TIMEOUT_MASK, SC8562_CTRL3_SS_TIMEOUT_SHIFT, SC8562_SS_TIMEOUT_DISABLE);
	if (ret)
		hwlog_err("reg_init failed %d\n", ret);

	return ret;
}

static int sc8562_lvc_charge_init(void *dev_data)
{
	int ret;
	struct sc8562_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	ret = sc8562_reg_init(di);
	ret += sc8562_config_switching_frequency(SC8562_SW_FREQ_MIN, di);
	if (ret)
		return -EIO;

	di->init_finish_flag = SC8562_INIT_FINISH;

	return 0;
}

static int sc8562_sc_charge_init(void *dev_data)
{
	int ret;
	struct sc8562_device_info *di = dev_data;


	if (!di)
		return -EPERM;

	ret = sc8562_reg_init(di);
	ret += sc8562_config_switching_frequency(SC8562_SW_FREQ_MIN, di);
	if (ret)
		return -EIO;

	di->init_finish_flag = SC8562_INIT_FINISH;

	return 0;
}

static int sc8562_sc4_charge_init(void *dev_data)
{
	int ret;
	struct sc8562_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	ret = sc8562_reg_init(di);
	ret += sc8562_config_switching_frequency(SC8562_SW_FREQ_750KHZ, di);
	if (ret)
		return -EIO;

	di->init_finish_flag = SC8562_INIT_FINISH;

	return 0;
}

static int sc8562_reg_reset_and_init(void *dev_data)
{
	int ret;
	struct sc8562_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	ret = sc8562_reg_reset(di);
	ret += sc8562_reg_init(di);
	ret += sc8562_threshold_reg_init(di, SC8562_CHG_F21SC_MODE);

	return ret;
}

static int sc8562_charge_exit(void *dev_data)
{
	int ret;
	struct sc8562_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	ret = sc8562_sc_charge_enable(SC8562_SWITCHCAP_DISABLE, di);
	di->fcp_support = false;
	di->init_finish_flag = false;
	di->int_notify_enable_flag = false;

	return ret;
}

static int sc8562_batinfo_exit(void *dev_data)
{
	return 0;
}

static int sc8562_batinfo_init(void *dev_data)
{
	struct sc8562_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	return sc8562_chip_init(di);
}

static int sc8562_lvc_set_threshold(int enable, void *dev_data)
{
	struct sc8562_device_info *di = dev_data;
	u8 mode = enable ? SC8562_CHG_FBYPASS_MODE : SC8562_CHG_F21SC_MODE;

	if (!di)
		return -EPERM;

	if (sc8562_threshold_reg_init(di, mode))
		return -EIO;

	return 0;
}

static int sc8562_sc_set_threshold(int enable, void *dev_data)
{
	struct sc8562_device_info *di = dev_data;
	u8 mode = SC8562_CHG_F21SC_MODE;

	if (!di)
		return -EPERM;

	if (sc8562_threshold_reg_init(di, mode))
		return -EIO;

	return 0;
}

static int sc8562_sc4_set_threshold(int enable, void *dev_data)
{
	struct sc8562_device_info *di = dev_data;
	u8 mode = enable ? SC8562_CHG_F41SC_MODE : SC8562_CHG_F21SC_MODE;

	if (!di)
		return -EPERM;

	if (sc8562_threshold_reg_init(di, mode))
		return -EIO;

	return 0;
}

static int sc8562_get_register_head(char *buffer, int size, void *dev_data)
{
	struct sc8562_device_info *di = dev_data;

	if (!buffer || !di)
		return -EPERM;

	snprintf(buffer, size,
		"dev        mode   Ibus   Vbus   Ibat   Vusb   Vout   Vbat   Temp");

	return 0;
}

static int sc8562_value_dump(char *buffer, int size, void *dev_data)
{
	u8 val = 0;
	char buff[SC8562_BUF_LEN] = { 0 };
	struct sc8562_device_info *di = dev_data;
	struct sc8562_dump_value dv = { 0 };

	if (!buffer || !di)
		return -EPERM;

	dv.vbat = sc8562_get_vbat_mv(dev_data);
	(void)sc8562_get_ibus_ma(&dv.ibus, dev_data);
	(void)sc8562_get_vbus_mv(&dv.vbus, dev_data);
	(void)sc8562_get_ibat_ma(&dv.ibat, dev_data);
	(void)sc8562_get_vusb_mv(&dv.vusb, dev_data);
	(void)sc8562_get_vout_mv(&dv.vout, dev_data);
	(void)sc8562_get_device_temp(&dv.temp, dev_data);
	(void)sc8562_read_byte(di, SC8562_CTRL4_REG, &val);

	snprintf(buff, sizeof(buff), "%s    ", di->name);
	strncat(buffer, buff, strlen(buff));

	if (sc8562_is_device_close(dev_data))
		snprintf(buff, sizeof(buff), "%s", "   OFF    ");
	else if (((val & SC8562_CTRL4_MODE_MASK) >> SC8562_CTRL4_MODE_SHIFT) ==
		SC8562_CHG_FBYPASS_MODE)
		snprintf(buff, sizeof(buff), "%s", "   LVC    ");
	else if (((val & SC8562_CTRL4_MODE_MASK) >> SC8562_CTRL4_MODE_SHIFT) ==
		SC8562_CHG_F21SC_MODE)
		snprintf(buff, sizeof(buff), "%s", "   SC     ");
	else if (((val & SC8562_CTRL4_MODE_MASK) >> SC8562_CTRL4_MODE_SHIFT) ==
		SC8562_CHG_F41SC_MODE)
		snprintf(buff, sizeof(buff), "%s", "   SC4    ");

	strncat(buffer, buff, strlen(buff));
	snprintf(buff, sizeof(buff), "%-7d%-7d%-7d%-7d%-7d%-7d%-7d",
		dv.ibus, dv.vbus, dv.ibat, dv.vusb, dv.vout, dv.vbat, dv.temp);
	strncat(buffer, buff, strlen(buff));

	return 0;
}

static struct dc_ic_ops sc8562_lvc_ops = {
	.dev_name = "sc8562",
	.ic_init = sc8562_lvc_charge_init,
	.ic_exit = sc8562_charge_exit,
	.ic_enable = sc8562_lvc_charge_enable,
	.ic_discharge = sc8562_discharge,
	.is_ic_close = sc8562_is_device_close,
	.get_ic_id = sc8562_get_device_id,
	.config_ic_watchdog = sc8562_config_watchdog_ms,
	.kick_ic_watchdog = sc8562_kick_watchdog_ms,
	.ic_reg_reset_and_init = sc8562_reg_reset_and_init,
	.set_ic_thld = sc8562_lvc_set_threshold,
};

static struct dc_ic_ops sc8562_sc_ops = {
	.dev_name = "sc8562",
	.ic_init = sc8562_sc_charge_init,
	.ic_exit = sc8562_charge_exit,
	.ic_enable = sc8562_sc_charge_enable,
	.ic_discharge = sc8562_discharge,
	.is_ic_close = sc8562_is_device_close,
	.get_ic_id = sc8562_get_device_id,
	.config_ic_watchdog = sc8562_config_watchdog_ms,
	.kick_ic_watchdog = sc8562_kick_watchdog_ms,
	.ic_reg_reset_and_init = sc8562_reg_reset_and_init,
	.set_ic_thld = sc8562_sc_set_threshold,
};

static struct dc_ic_ops sc8562_sc4_ops = {
	.dev_name = "sc8562",
	.ic_init = sc8562_sc4_charge_init,
	.ic_exit = sc8562_charge_exit,
	.ic_enable = sc8562_sc4_charge_enable,
	.ic_discharge = sc8562_discharge,
	.is_ic_close = sc8562_is_device_close,
	.get_ic_id = sc8562_get_device_id,
	.config_ic_watchdog = sc8562_config_watchdog_ms,
	.kick_ic_watchdog = sc8562_kick_watchdog_ms,
	.ic_reg_reset_and_init = sc8562_reg_reset_and_init,
	.set_ic_thld = sc8562_sc4_set_threshold,
};

static struct dc_batinfo_ops sc8562_batinfo_ops = {
	.init = sc8562_batinfo_init,
	.exit = sc8562_batinfo_exit,
	.get_bat_btb_voltage = sc8562_get_vbat_mv,
	.get_bat_package_voltage = sc8562_get_vbat_mv,
	.get_vbus_voltage = sc8562_get_vbus_mv,
	.get_bat_current = sc8562_get_ibat_ma,
	.get_ic_ibus = sc8562_get_ibus_ma,
	.get_ic_temp = sc8562_get_device_temp,
	.get_ic_vout = sc8562_get_vout_mv,
	.get_ic_vusb = sc8562_get_vusb_mv,
};

static struct power_log_ops sc8562_log_ops = {
	.dev_name = "sc8562",
	.dump_log_head = sc8562_get_register_head,
	.dump_log_content = sc8562_value_dump,
};

static void sc8562_init_ops_dev_data(struct sc8562_device_info *di)
{
	memcpy(&di->lvc_ops, &sc8562_lvc_ops, sizeof(struct dc_ic_ops));
	di->lvc_ops.dev_data = (void *)di;
	memcpy(&di->sc_ops, &sc8562_sc_ops, sizeof(struct dc_ic_ops));
	di->sc_ops.dev_data = (void *)di;
	memcpy(&di->sc4_ops, &sc8562_sc4_ops, sizeof(struct dc_ic_ops));
	di->sc4_ops.dev_data = (void *)di;
	memcpy(&di->batinfo_ops, &sc8562_batinfo_ops, sizeof(struct dc_batinfo_ops));
	di->batinfo_ops.dev_data = (void *)di;
	memcpy(&di->log_ops, &sc8562_log_ops, sizeof(struct power_log_ops));
	di->log_ops.dev_data = (void *)di;

	if (!di->ic_role) {
		snprintf(di->name, CHIP_DEV_NAME_LEN, "sc8562");
	} else {
		snprintf(di->name, CHIP_DEV_NAME_LEN, "sc8562_%d", di->ic_role);
		di->lvc_ops.dev_name = di->name;
		di->sc_ops.dev_name = di->name;
		di->sc4_ops.dev_name = di->name;
		di->log_ops.dev_name = di->name;
	}
}

static void sc8562_ops_register(struct sc8562_device_info *di)
{
	int ret;

	sc8562_init_ops_dev_data(di);

	ret = dc_ic_ops_register(LVC_MODE, di->ic_role, &di->lvc_ops);
	ret += dc_ic_ops_register(SC_MODE, di->ic_role, &di->sc_ops);
	ret += dc_ic_ops_register(SC4_MODE, di->ic_role, &di->sc4_ops);
	ret += dc_batinfo_ops_register(di->ic_role, &di->batinfo_ops, di->device_id);
	if (ret)
		hwlog_err("sysinfo ops register failed\n");

	ret = sc8562_protocol_ops_register(di);
	if (ret)
		hwlog_err("scp or fcp ops register failed\n");

	power_log_ops_register(&di->log_ops);
}

static void sc8562_fault_event_notify(unsigned long event, void *data)
{
	power_event_anc_notify(POWER_ANT_SC4_FAULT, event, data);
}

static void sc8562_interrupt_handle(struct sc8562_device_info *di,
	struct nty_data *data, u8 *flag)
{
	int val = 0;

	if (flag[SC8562_IRQ_VUSB_OVP] & SC8562_VUSB_OVP_FLAG_MASK) {
		hwlog_info("USB OVP happened\n");
		sc8562_fault_event_notify(POWER_NE_DC_FAULT_AC_OVP, data);
	} else if (flag[SC8562_IRQ_VBAT_OVP] & SC8562_VBAT_OVP_FLAG_MASK) {
		val = sc8562_get_vbat_mv(di);
		hwlog_info("BAT OVP happened, vbat=%d\n", val);
		if (val >= SC8562_VBAT_OVP_INIT)
			sc8562_fault_event_notify(POWER_NE_DC_FAULT_VBAT_OVP, data);
	} else if (flag[SC8562_IRQ_IBAT_OCP] & SC8562_IBAT_OCP_FLAG_MASK) {
		sc8562_get_ibat_ma(&val, di);
		hwlog_info("BAT OCP happened, ibat=%d\n", val);
		if (((val >= SC8562_IBAT_OCP_F41SC_INIT) && (di->charge_mode == SC8562_CHG_F41SC_MODE)) ||
			((val >= SC8562_IBAT_OCP_F21SC_INIT) && (di->charge_mode == SC8562_CHG_F21SC_MODE)) ||
			((val >= SC8562_IBAT_OCP_FBPSC_INIT) && (di->charge_mode == SC8562_CHG_FBYPASS_MODE)))
			sc8562_fault_event_notify(POWER_NE_DC_FAULT_IBAT_OCP, data);
	} else if (flag[SC8562_IRQ_FLT_FLAG] & SC8562_VBUS_OVP_FLAG_MASK) {
		sc8562_get_vbus_mv(&val, di);
		hwlog_info("BUS OVP happened, vbus=%d\n", val);
		if (((val >= SC8562_VBUS_OVP_F41SC_INIT) && (di->charge_mode == SC8562_CHG_F41SC_MODE)) ||
			((val >= SC8562_VBUS_OVP_F21SC_INIT) && (di->charge_mode == SC8562_CHG_F21SC_MODE)) ||
			((val >= SC8562_VBUS_OVP_FBPSC_INIT) && (di->charge_mode == SC8562_CHG_FBYPASS_MODE)))
			sc8562_fault_event_notify(POWER_NE_DC_FAULT_VBUS_OVP, data);
	} else if (flag[SC8562_IRQ_IBUS_OCP] & SC8562_IBUS_OCP_FLAG_MASK) {
		sc8562_get_ibus_ma(&val, di);
		hwlog_info("BUS OCP happened, ibus=%d\n", val);
		if (((val >= SC8562_IBUS_OCP_F41SC_INIT) && (di->charge_mode == SC8562_CHG_F41SC_MODE)) ||
			((val >= SC8562_IBUS_OCP_F21SC_INIT) && (di->charge_mode == SC8562_CHG_F21SC_MODE)) ||
			((val >= SC8562_IBUS_OCP_FBPSC_INIT) && (di->charge_mode == SC8562_CHG_FBYPASS_MODE)))
			sc8562_fault_event_notify(POWER_NE_DC_FAULT_IBUS_OCP, data);
	} else if (flag[SC8562_IRQ_FLT_FLAG] & SC8562_VOUT_OVP_FLAG_MASK) {
		hwlog_info("VOUT OVP happened\n");
	}

	if (flag[SC8562_IRQ_SCP_FLAG] & SC8562_TRANS_DONE_FLAG_MASK)
		di->scp_trans_done = true;
}

static void sc8562_interrupt_work(struct work_struct *work)
{
	u8 flag[SC8562_IRQ_END] = { 0 };
	struct sc8562_device_info *di = NULL;
	struct nty_data *data = NULL;

	if (!work)
		return;

	di = container_of(work, struct sc8562_device_info, irq_work);
	if (!di || !di->client) {
		hwlog_err("di is null\n");
		return;
	}

	(void)sc8562_read_byte(di, SC8562_VBAT_OVP_REG, &flag[SC8562_IRQ_VBAT_OVP]);
	(void)sc8562_read_byte(di, SC8562_FLT_FLAG_REG, &flag[SC8562_IRQ_FLT_FLAG]);
	(void)sc8562_read_byte(di, SC8562_VUSB_OVP_REG, &flag[SC8562_IRQ_VUSB_OVP]);
	(void)sc8562_read_byte(di, SC8562_IBUS_OCP_REG, &flag[SC8562_IRQ_IBUS_OCP]);
	(void)sc8562_read_byte(di, SC8562_IBAT_OCP_REG, &flag[SC8562_IRQ_IBAT_OCP]);
	(void)sc8562_read_byte(di, SC8562_SCP_FLAG_MASK_REG, &flag[SC8562_IRQ_SCP_FLAG]);
	(void)sc8562_read_byte(di, SC8562_INT_FLAG_REG, &flag[SC8562_IRQ_INT_FLAG]);

	data = &(di->nty_data);
	data->addr = di->client->addr;

	if (di->int_notify_enable_flag) {
		sc8562_interrupt_handle(di, data, flag);
		sc8562_dump_register(di);
	}

	hwlog_info("FLAG_VBAT_OVP [0x%x]=0x%x, FLAG_FLT_FLAG [0x%x]=0x%x, FLAG_VUSB_OVP [0x%x]=0x%x\n",
		SC8562_VBAT_OVP_REG, flag[SC8562_IRQ_VBAT_OVP], SC8562_FLT_FLAG_REG, flag[SC8562_IRQ_FLT_FLAG],
		SC8562_VUSB_OVP_REG, flag[SC8562_IRQ_VUSB_OVP]);
	hwlog_info("FLAG_IBUS_OCP [0x%x]=0x%x, FLAG_IBAT_OCP [0x%x]=0x%x, FLAG_SCP_FLAG [0x%x]=0x%x\n",
		SC8562_IBUS_OCP_REG, flag[SC8562_IRQ_IBUS_OCP], SC8562_IBAT_OCP_REG, flag[SC8562_IRQ_IBAT_OCP],
		SC8562_SCP_FLAG_MASK_REG, flag[SC8562_IRQ_SCP_FLAG]);
	hwlog_info("FLAG_INT_FLAG [0x%x]=0x%x\n", SC8562_INT_FLAG_REG, flag[SC8562_IRQ_INT_FLAG]);

	enable_irq(di->irq_int);
}

static irqreturn_t sc8562_interrupt(int irq, void *_di)
{
	struct sc8562_device_info *di = _di;

	if (!di)
		return IRQ_HANDLED;

	if (di->init_finish_flag)
		di->int_notify_enable_flag = true;

	hwlog_info("int happened\n");
	disable_irq_nosync(di->irq_int);
	schedule_work(&di->irq_work);

	return IRQ_HANDLED;
}

static int sc8562_irq_init(struct sc8562_device_info *di,
	struct device_node *np)
{
	int ret;

	ret = power_gpio_config_interrupt(np,
		"gpio_int", "sc8562_gpio_int", &di->gpio_int, &di->irq_int);
	if (ret)
		return ret;

	ret = request_irq(di->irq_int, sc8562_interrupt,
		IRQF_TRIGGER_FALLING, "sc8562_int_irq", di);
	if (ret) {
		hwlog_err("gpio irq request failed\n");
		di->irq_int = -1;
		gpio_free(di->gpio_int);
		return ret;
	}

	enable_irq_wake(di->irq_int);
	INIT_WORK(&di->irq_work, sc8562_interrupt_work);

	return 0;
}

static int sc8562_gpio_init(struct sc8562_device_info *di,
	struct device_node *np)
{
	if (power_gpio_config_output(np, "gpio_enable", "sc8562_gpio_enable",
			&di->gpio_enable, SC8562_GPIO_ENABLE))
		return -EPERM;

	if (power_gpio_config_output(np, "gpio_lpm", "sc8562_gpio_lpm",
			&di->gpio_lpm, SC8562_GPIO_ENABLE)) {
		gpio_free(di->gpio_enable);
		return -EPERM;
	}

	return 0;
}

static void sc8562_parse_dts(struct device_node *np,
	struct sc8562_device_info *di)
{
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"switching_frequency", &di->switching_frequency,
		SC8562_SW_FREQ_750KHZ);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "scp_support",
		(u32 *)&(di->dts_scp_support), 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "fcp_support",
		(u32 *)&(di->dts_fcp_support), 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "ic_role",
		(u32 *)&(di->ic_role), IC_ONE);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"sense_r_config", &di->sense_r_config, SENSE_R_1_MOHM);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"sense_r_actual", &di->sense_r_actual, SENSE_R_1_MOHM);
}

static void sc8562_init_lock_mutex(struct sc8562_device_info *di)
{
	mutex_init(&di->scp_detect_lock);
	mutex_init(&di->accp_adapter_reg_lock);
}

static void sc8562_destroy_lock_mutex(struct sc8562_device_info *di)
{
	mutex_destroy(&di->scp_detect_lock);
	mutex_destroy(&di->accp_adapter_reg_lock);
}

static int sc8562_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret;
	struct sc8562_device_info *di = NULL;
	struct device_node *np = NULL;

	if (!client || !client->dev.of_node || !id)
		return -ENODEV;

	di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	di->dev = &client->dev;
	np = di->dev->of_node;
	di->client = client;
	di->chip_already_init = 1;

	ret = sc8562_get_device_id(di);
	if (ret == SC8562_DEVICE_ID_GET_FAIL)
		goto sc8562_fail_0;

	sc8562_parse_dts(np, di);

	ret = sc8562_gpio_init(di, np);
	if (ret)
		goto sc8562_fail_0;

	sc8562_init_lock_mutex(di);

	(void)power_pinctrl_config(di->dev, "pinctrl-names", 1); /* 1:pinctrl-names length */

	ret = sc8562_reg_reset(di);
	if (ret)
		goto sc8562_fail_1;

	ret = sc8562_reg_init(di);
	if (ret)
		goto sc8562_fail_1;

	ret = sc8562_threshold_reg_init(di, SC8562_CHG_F21SC_MODE);
	if (ret)
		goto sc8562_fail_1;

	ret = sc8562_irq_init(di, np);
	if (ret)
		goto sc8562_fail_1;

	sc8562_ops_register(di);
	i2c_set_clientdata(client, di);

	return 0;

sc8562_fail_1:
	gpio_free(di->gpio_enable);
	gpio_free(di->gpio_lpm);
	sc8562_destroy_lock_mutex(di);
sc8562_fail_0:
	di->chip_already_init = 0;
	devm_kfree(&client->dev, di);

	return ret;
}

static int sc8562_remove(struct i2c_client *client)
{
	struct sc8562_device_info *di = i2c_get_clientdata(client);

	if (!di)
		return -EPERM;

	sc8562_reg_reset(di);

	if (di->irq_int)
		free_irq(di->irq_int, di);

	if (di->gpio_int)
		gpio_free(di->gpio_int);

	if (di->gpio_enable)
		gpio_free(di->gpio_enable);
	sc8562_destroy_lock_mutex(di);

	return 0;
}

static void sc8562_shutdown(struct i2c_client *client)
{
	struct sc8562_device_info *di = i2c_get_clientdata(client);

	if (!di)
		return;

	sc8562_reg_reset(di);
}

MODULE_DEVICE_TABLE(i2c, sc8562);
static const struct of_device_id sc8562_of_match[] = {
	{
		.compatible = "sc8562",
		.data = NULL,
	},
	{},
};

static const struct i2c_device_id sc8562_i2c_id[] = {
	{ "sc8562", 0 },
	{}
};

static struct i2c_driver sc8562_driver = {
	.probe = sc8562_probe,
	.remove = sc8562_remove,
	.shutdown = sc8562_shutdown,
	.id_table = sc8562_i2c_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "sc8562",
		.of_match_table = of_match_ptr(sc8562_of_match),
	},
};

static int __init sc8562_init(void)
{
	return i2c_add_driver(&sc8562_driver);
}

static void __exit sc8562_exit(void)
{
	i2c_del_driver(&sc8562_driver);
}

module_init(sc8562_init);
module_exit(sc8562_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("sc8562 module driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
