// SPDX-License-Identifier: GPL-2.0
/*
 * sc8546.c
 *
 * sc8546 direct charge driver
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

#include "sc8546.h"
#include "sc8546_i2c.h"
#include "sc8546_scp.h"
#include "sc8546_ufcs.h"
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_pinctrl.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_device_id.h>

#define HWLOG_TAG sc8546_chg
HWLOG_REGIST();

static void sc8546_dump_register(struct sc8546_device_info *di)
{
	u8 i;
	u8 flag = 0;

	for (i = SC8546_VBATREG_REG; i <= SC8546_PMID2OUT_OVP_UVP_REG; i++) {
		sc8546_read_byte(di, i, &flag);
		hwlog_info("reg[0x%x]=0x%x\n", i, flag);
	}
}

static int sc8546_discharge(int enable, void *dev_data)
{
	return 0;
}

static int sc8546_is_device_close(void *dev_data)
{
	u8 reg = 0;
	int ret;
	struct sc8546_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	ret = sc8546_read_byte(di, SC8546_CONVERTER_STATE_REG, &reg);
	if (ret)
		return -EIO;

	if (reg & SC8546_CP_SWITCHING_STAT_MASK)
		return 0;

	return 1; /* 1:ic is closed */
}

static int sc8546_get_device_id(void *dev_data)
{
	u8 dev_id = 0;
	int ret;
	struct sc8546_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	if (di->get_id_time == SC8546_USED)
		return di->device_id;

	di->get_id_time = SC8546_USED;
	ret = sc8546_read_byte(di, SC8546_DEVICE_ID_REG, &dev_id);
	if (ret && di->i2c_recovery) {
		hwlog_err("i2c read fail, try to read other address\n");
		di->client->addr = di->i2c_recovery_addr;
		ret = sc8546_read_byte(di, SC8546_DEVICE_ID_REG, &dev_id);
	}
	if (ret) {
		di->get_id_time = SC8546_NOT_USED;
		hwlog_err("get_device_id read failed\n");
		return -EIO;
	}

	if (dev_id == SC8546_DEVICE_ID_VALUE)
		di->device_id = SWITCHCAP_SC8546;
	else
		di->device_id = DC_DEVICE_ID_END;

	hwlog_info("get_device_id [%x]=0x%x, device_id: 0x%x\n",
		SC8546_DEVICE_ID_REG, dev_id, di->device_id);
	return di->device_id;
}

static int sc8546_get_vbat_mv(void *dev_data)
{
	s16 data = 0;
	int ret, vbat;
	struct sc8546_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	ret = sc8546_read_word(di, SC8546_VBAT_ADC1_REG, &data);
	if (ret)
		return -EIO;

	/* VBAT ADC LSB: 1.25mV */
	vbat = (int)data * 125 / 100;

	hwlog_info("VBAT_ADC=0x%x, vbat=%d\n", data, vbat);
	return vbat;
}

static int sc8546_get_ibat_ma(int *ibat, void *dev_data)
{
	int ret;
	s16 data = 0;
	struct sc8546_device_info *di = dev_data;

	if (!di || !ibat)
		return -EPERM;

	ret = sc8546_read_word(di, SC8546_IBAT_ADC1_REG, &data);
	if (ret)
		return -EIO;

	/* IBAT ADC LSB: 3.125mA */
	*ibat = ((int)data * 3125 / 1000) * di->sense_r_config;
	*ibat /= di->sense_r_actual;

	hwlog_info("IBAT_ADC=0x%x, ibat=%d\n", data, *ibat);
	return 0;
}

int sc8546_get_vbus_mv(int *vbus, void *dev_data)
{
	int ret;
	s16 data = 0;
	struct sc8546_device_info *di = dev_data;

	if (!di || !vbus)
		return -EPERM;

	ret = sc8546_read_word(di, SC8546_VBUS_ADC1_REG, &data);
	if (ret)
		return -EIO;

	/* VBUS ADC LSB: 3.75mV */
	*vbus = (int)data * 375 / 100;

	hwlog_info("VBUS_ADC=0x%x, vbus=%d\n", data, *vbus);
	return 0;
}

static int sc8546_get_ibus_ma(int *ibus, void *dev_data)
{
	s16 data = 0;
	int ret;
	struct sc8546_device_info *di = dev_data;

	if (!di || !ibus)
		return -EPERM;

	ret = sc8546_read_word(di, SC8546_IBUS_ADC1_REG, &data);
	if (ret)
		return -EIO;

	/* IBUS ADC LSB: 1.875mA */
	*ibus = (int)data * 1875 / 1000;

	hwlog_info("IBUS_ADC=0x%x, ibus=%d\n", data, *ibus);
	return 0;
}

static int sc8546_get_vusb_mv(int *vusb, void *dev_data)
{
	int ret;
	s16 data = 0;
	struct sc8546_device_info *di = dev_data;

	if (!di || !vusb)
			return -EPERM;

	ret = sc8546_read_word(di, SC8546_VAC_ADC1_REG, &data);
	if (ret)
		return -EIO;

	/* VUSB ADC LSB: 5mV */
	*vusb = (int)data * 5;

	hwlog_info("VUSB_ADC=0x%x, vusb=%d\n", data, *vusb);
	return 0;
}

static int sc8546_get_vout_mv(int *vout, void *dev_data)
{
	int ret;
	s16 data = 0;
	struct sc8546_device_info *di = dev_data;

	if (!di || !vout)
			return -EPERM;

	ret = sc8546_read_word(di, SC8546_VOUT_ADC1_REG, &data);
	if (ret)
		return -EIO;

	/* VUOT ADC LSB: 1.25mV */
	*vout = (int)data * 125 / 100;

	hwlog_info("VUSB_OUT=0x%x, vout=%d\n", data, *vout);
	return 0;
}

static int sc8546_get_device_temp(int *temp, void *dev_data)
{
	s16 data = 0;
	int ret;
	struct sc8546_device_info *di = dev_data;

	if (!di || !temp)
		return -EPERM;

	ret = sc8546_read_word(di, SC8546_TDIE_ADC1_REG, &data);
	if (ret)
		return -EIO;

	/* TDIE_ADC LSB: 0.5C */
	*temp = (int)data * 5 / 10;

	hwlog_info("TDIE_ADC=0x%x, temp=%d\n", data, *temp);
	return 0;
}

static int sc8546_config_watchdog_ms(int time, void *dev_data)
{
	int ret;
	u8 val;
	u8 reg = 0;
	struct sc8546_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	if (time >= SC8546_WTD_CONFIG_TIMING_30000MS)
		val = SC8546_WTD_SET_30000MS;
	else if (time >= SC8546_WTD_CONFIG_TIMING_5000MS)
		val = SC8546_WTD_SET_5000MS;
	else if (time >= SC8546_WTD_CONFIG_TIMING_1000MS)
		val = SC8546_WTD_SET_1000MS;
	else if (time >= SC8546_WTD_CONFIG_TIMING_500MS)
		val = SC8546_WTD_SET_500MS;
	else if (time >= SC8546_WTD_CONFIG_TIMING_200MS)
		val = SC8546_WTD_SET_200MS;
	else
		val = SC8546_WTD_CONFIG_TIMING_DIS;

	ret = sc8546_write_mask(di, SC8546_CTRL3_REG, SC8546_WTD_TIMEOUT_MASK,
		SC8546_WTD_TIMEOUT_SHIFT, val);
	ret += sc8546_read_byte(di, SC8546_CTRL3_REG, &reg);
	if (ret)
		return -EIO;

	hwlog_info("config_watchdog_ms [%x]=0x%x\n", SC8546_CTRL3_REG, reg);
	return 0;
}

static int sc8546_kick_watchdog_ms(void *dev_data)
{
	return 0;
}

static int sc8546_config_vbat_ovp_th_mv(struct sc8546_device_info *di, int ovp_th)
{
	u8 value;
	int ret;

	if (ovp_th < SC8546_VBAT_OVP_BASE)
		ovp_th = SC8546_VBAT_OVP_BASE;

	if (ovp_th > SC8546_VBAT_OVP_MAX)
		ovp_th = SC8546_VBAT_OVP_MAX;

	value = (u8)((ovp_th - SC8546_VBAT_OVP_BASE) / SC8546_VBAT_OVP_STEP);
	ret = sc8546_write_mask(di, SC8546_VBAT_OVP_REG, SC8546_VBAT_OVP_MASK,
		SC8546_VBAT_OVP_SHIFT, value);
	ret += sc8546_read_byte(di, SC8546_VBAT_OVP_REG, &value);
	if (ret)
		return -EIO;

	hwlog_info("config_vbat_ovp_threshold_mv [%x]=0x%x\n",
		SC8546_VBAT_OVP_REG, value);
	return 0;
}

static int sc8546_config_ibat_ocp_th_ma(struct sc8546_device_info *di, int ocp_th)
{
	u8 value;
	int ret;

	if (ocp_th < SC8546_IBAT_OCP_BASE)
		ocp_th = SC8546_IBAT_OCP_BASE;

	if (ocp_th > SC8546_IBAT_OCP_MAX)
		ocp_th = SC8546_IBAT_OCP_MAX;

	value = (u8)((ocp_th - SC8546_IBAT_OCP_BASE) / SC8546_IBAT_OCP_STEP);
	ret = sc8546_write_mask(di, SC8546_IBAT_OCP_REG, SC8546_IBAT_OCP_MASK,
		SC8546_IBAT_OCP_SHIFT, value);
	ret += sc8546_read_byte(di, SC8546_IBAT_OCP_REG, &value);
	if (ret)
		return -EIO;

	hwlog_info("config_ibat_ocp_threshold_ma [%x]=0x%x\n",
		SC8546_IBAT_OCP_REG, value);
	return 0;
}

int sc8546_config_vac_ovp_th_mv(struct sc8546_device_info *di, int ovp_th)
{
	u8 value;
	int ret;

	if (ovp_th < SC8546_VAC_OVP_BASE)
		ovp_th = SC8546_VAC_OVP_DEFAULT;

	if (ovp_th > SC8546_VAC_OVP_MAX)
		ovp_th = SC8546_VAC_OVP_MAX;

	value = (u8)((ovp_th - SC8546_VAC_OVP_BASE) / SC8546_VAC_OVP_STEP);
	ret = sc8546_write_mask(di, SC8546_VAC_OVP_REG, SC8546_VAC_OVP_MASK,
		SC8546_VAC_OVP_SHIFT, value);
	ret += sc8546_read_byte(di, SC8546_VAC_OVP_REG, &value);
	if (ret)
		return -EIO;

	hwlog_info("config_ac_ovp_threshold_mv [%x]=0x%x\n",
		SC8546_VAC_OVP_REG, value);
	return 0;
}

int sc8546_config_vbus_ovp_th_mv(struct sc8546_device_info *di, int ovp_th)
{
	u8 value;
	int ret;

	if (ovp_th < SC8546_VBUS_OVP_BASE)
		ovp_th = SC8546_VBUS_OVP_BASE;

	if (ovp_th > SC8546_VBUS_OVP_MAX)
		ovp_th = SC8546_VBUS_OVP_MAX;

	value = (u8)((ovp_th - SC8546_VBUS_OVP_BASE) / SC8546_VBUS_OVP_STEP);
	ret = sc8546_write_mask(di, SC8546_VBUS_OVP_REG, SC8546_VBUS_OVP_MASK,
		SC8546_VBUS_OVP_SHIFT, value);
	ret += sc8546_read_byte(di, SC8546_VBUS_OVP_REG, &value);
	if (ret)
		return -EIO;

	hwlog_info("config_vbus_ovp_threshole_mv [%x]=0x%x\n",
		SC8546_VBUS_OVP_REG, value);
	return 0;
}

static int sc8546_config_ibus_ocp_th_ma(struct sc8546_device_info *di, int ocp_th)
{
	u8 value;
	int ret;

	if (ocp_th < SC8546_IBUS_OCP_BASE)
		ocp_th = SC8546_IBUS_OCP_BASE;

	if (ocp_th > SC8546_IBUS_OCP_MAX)
		ocp_th = SC8546_IBUS_OCP_MAX;

	value = (u8)((ocp_th - SC8546_IBUS_OCP_BASE) / SC8546_IBUS_OCP_STEP);
	ret = sc8546_write_mask(di, SC8546_IBUS_OCP_UCP_REG,
		SC8546_IBUS_OCP_MASK, SC8546_IBUS_OCP_SHIFT, value);
	ret += sc8546_read_byte(di, SC8546_IBUS_OCP_UCP_REG, &value);
	if (ret)
		return -EIO;

	hwlog_info("config_ibus_ocp_threshold_ma [%x]=0x%x\n",
		SC8546_IBUS_OCP_UCP_REG, value);
	return 0;
}

static int sc8546_config_sw_freq_shift(int shift, struct sc8546_device_info *di)
{
	u8 value;
	int ret;

	switch (shift) {
	case SC8546_SW_FREQ_SHIFT_P_P10:
	case SC8546_SW_FREQ_SHIFT_M_P10:
	case SC8546_SW_FREQ_SHIFT_MP_P10:
		value = (u8)shift;
		break;
	default:
		value = SC8546_SW_FREQ_SHIFT_NORMAL;
		break;
	}

	ret = sc8546_write_mask(di, SC8546_CTRL1_REG, SC8546_FREQ_SHIFT_MASK,
		SC8546_FREQ_SHIFT_SHIFT, value);
	ret += sc8546_read_byte(di, SC8546_CTRL1_REG, &value);
	if (ret)
		return -EIO;

	hwlog_info("config_adjustable_switching_frequency [%x]=0x%x\n",
		SC8546_CTRL1_REG, value);
	return 0;
}

static int sc8546_config_sw_freq(int freq, struct sc8546_device_info *di)
{
	u8 value;
	int ret;

	if (freq <= SC8546_SW_FREQ_MIN)
		freq = SC8546_SW_FREQ_MIN;
	if (freq >= SC8546_SW_FREQ_MAX)
		freq = SC8546_SW_FREQ_MAX;

	value = (u8)((freq - SC8546_SW_FREQ_MIN) / SC8546_SW_FREQ_STEP);

	ret = sc8546_write_mask(di, SC8546_CTRL1_REG, SC8546_SW_FREQ_MASK,
		SC8546_SW_FREQ_SHIFT, value);
	ret += sc8546_read_byte(di, SC8546_CTRL1_REG, &value);
	if (ret)
		return -EIO;

	hwlog_info("config_switching_frequency [%x]=0x%x\n",
		SC8546_CTRL1_REG, value);
	return 0;
}

static int sc8546_congfig_ibat_sns_res(struct sc8546_device_info *di)
{
	int ret;
	u8 value;

	if (di->sense_r_config == SENSE_R_5_MOHM)
		value = SC8546_IBAT_SNS_RES_5MOHM;
	else
		value = SC8546_IBAT_SNS_RES_2MOHM;

	ret = sc8546_write_mask(di, SC8546_CTRL2_REG,
		SC8546_SET_IBAT_SNS_RES_MASK,
		SC8546_SET_IBAT_SNS_RES_SHIFT, value);
	if (ret)
		return -EIO;

	hwlog_info("congfig_ibat_sns_res=%d\n", di->sense_r_config);
	return 0;
}

static int sc8546_threshold_reg_init(struct sc8546_device_info *di, int mode)
{
	int ret, ibus_ocp;

	if (mode == SC8546_CHG_MODE_CHGPUMP)
		ibus_ocp = SC8546_SC_IBUS_OCP_TH_INIT;
	else
		ibus_ocp = SC8546_LVC_IBUS_OCP_TH_INIT;

	ret = sc8546_config_vac_ovp_th_mv(di, SC8546_VAC_OVP_TH_INIT);
	ret += sc8546_config_vbus_ovp_th_mv(di, SC8546_VBUS_OVP_TH_INIT);
	ret += sc8546_config_ibus_ocp_th_ma(di, ibus_ocp);
	ret += sc8546_config_vbat_ovp_th_mv(di, SC8546_VBAT_OVP_TH_INIT);
	ret += sc8546_config_ibat_ocp_th_ma(di, SC8546_IBAT_OCP_TH_INIT);
	if (ret)
		hwlog_info("protect threshold init failed\n");

	return ret;
}

static int sc8546_lvc_charge_enable(int enable, void *dev_data)
{
	int ret = 0;
	u8 ctrl1_reg = 0;
	u8 ctrl3_reg = 0;
	u8 chg_en;
	struct sc8546_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	chg_en = enable ? SC8546_CHG_CTRL_ENABLE : SC8546_CHG_CTRL_DISABLE;
	if (chg_en == SC8546_CHG_CTRL_ENABLE)
		ret = sc8546_threshold_reg_init(di, SC8546_CHG_MODE_BYPASS);

	ret += sc8546_write_mask(di, SC8546_CTRL1_REG, SC8546_CHG_CTRL_MASK,
		SC8546_CHG_CTRL_SHIFT, SC8546_CHG_CTRL_DISABLE);
	ret += sc8546_write_mask(di, SC8546_CTRL3_REG, SC8546_CHG_MODE_MASK,
		SC8546_CHG_MODE_SHIFT, SC8546_CHG_MODE_BYPASS);
	ret += sc8546_write_mask(di, SC8546_CTRL1_REG, SC8546_CHG_CTRL_MASK,
		SC8546_CHG_CTRL_SHIFT, chg_en);

	if (chg_en == SC8546_CHG_CTRL_DISABLE)
		ret += sc8546_threshold_reg_init(di, SC8546_CHG_MODE_BUCK);

	ret += sc8546_read_byte(di, SC8546_CTRL1_REG, &ctrl1_reg);
	ret += sc8546_read_byte(di, SC8546_CTRL3_REG, &ctrl3_reg);
	if (ret)
		return -EIO;

	hwlog_info("ic_role=%d, charge_enable [%x]=0x%x, [%x]=0x%x\n",
		di->ic_role, SC8546_CTRL1_REG, ctrl1_reg,
		SC8546_CTRL3_REG, ctrl3_reg);
	return 0;
}

static int sc8546_sc_charge_enable(int enable, void *dev_data)
{
	int ret = 0;
	u8 ctrl1_reg = 0;
	u8 ctrl3_reg = 0;
	u8 chg_en;
	struct sc8546_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	chg_en = enable ? SC8546_CHG_CTRL_ENABLE : SC8546_CHG_CTRL_DISABLE;
	if (chg_en == SC8546_CHG_CTRL_ENABLE)
		ret = sc8546_threshold_reg_init(di, SC8546_CHG_MODE_CHGPUMP);

	ret += sc8546_write_mask(di, SC8546_CTRL1_REG, SC8546_CHG_CTRL_MASK,
		SC8546_CHG_CTRL_SHIFT, SC8546_CHG_CTRL_DISABLE);
	ret += sc8546_write_mask(di, SC8546_CTRL3_REG, SC8546_CHG_MODE_MASK,
		SC8546_CHG_MODE_SHIFT, SC8546_CHG_MODE_CHGPUMP);
	ret += sc8546_write_mask(di, SC8546_CTRL1_REG, SC8546_CHG_CTRL_MASK,
		SC8546_CHG_CTRL_SHIFT, chg_en);

	if (chg_en == SC8546_CHG_CTRL_DISABLE)
		ret += sc8546_threshold_reg_init(di, SC8546_CHG_MODE_BUCK);

	ret += sc8546_read_byte(di, SC8546_CTRL1_REG, &ctrl1_reg);
	ret += sc8546_read_byte(di, SC8546_CTRL3_REG, &ctrl3_reg);
	if (ret)
		return -EIO;

	hwlog_info("ic_role=%d, charge_enable [%x]=0x%x, [%x]=0x%x\n",
		di->ic_role, SC8546_CTRL1_REG, ctrl1_reg,
		SC8546_CTRL3_REG, ctrl3_reg);
	return 0;
}

static int sc8546_reg_reset(struct sc8546_device_info *di)
{
	int ret;
	u8 ctrl1_reg = 0;

	ret = sc8546_write_mask(di, SC8546_CTRL1_REG,
		SC8546_REG_RST_MASK, SC8546_REG_RST_SHIFT,
		SC8546_REG_RST_ENABLE);
	if (ret)
		return -EIO;

	power_usleep(DT_USLEEP_1MS);
	ret = sc8546_read_byte(di, SC8546_CTRL1_REG, &ctrl1_reg);
	ret += sc8546_config_vac_ovp_th_mv(di, SC8546_VAC_OVP_TH_INIT);
	if (ret)
		return -EIO;

	hwlog_info("reg_reset [%x]=0x%x\n", SC8546_CTRL1_REG, ctrl1_reg);
	return 0;
}

static int sc8546_chip_init(void *dev_data)
{
	return 0;
}

static int sc8546_reg_init(struct sc8546_device_info *di)
{
	int ret;

	ret = sc8546_config_watchdog_ms(SC8546_WTD_CONFIG_TIMING_5000MS, di);
	ret += sc8546_threshold_reg_init(di, SC8546_CHG_MODE_BUCK);
	ret += sc8546_config_sw_freq(di->dts_sw_freq, di);
	ret += sc8546_config_sw_freq_shift(di->dts_sw_freq_shift, di);
	ret += sc8546_congfig_ibat_sns_res(di);
	ret += sc8546_write_byte(di, SC8546_INT_MASK_REG,
		SC8546_INT_MASK_REG_INIT);
	ret += sc8546_write_mask(di, SC8546_ADCCTRL_REG,
		SC8546_ADC_EN_MASK, SC8546_ADC_EN_SHIFT, 1);
	ret += sc8546_write_mask(di, SC8546_ADCCTRL_REG,
		SC8546_ADC_RATE_MASK, SC8546_ADC_RATE_SHIFT, 0);
	ret += sc8546_write_mask(di, SC8546_DPDM_CTRL2_REG,
		SC8546_DP_BUFF_EN_MASK, SC8546_DP_BUFF_EN_SHIFT, 1);
	ret += sc8546_write_byte(di, SC8546_SCP_FLAG_MASK_REG,
		SC8546_SCP_FLAG_MASK_REG_INIT);
	/* set IBUS_UCP_DEGLITCH 5ms */
	ret += sc8546_write_mask(di, SC8546_IBUS_OCP_UCP_REG,
		SC8546_IBUS_UCP_DEGLITCH_MASK, SC8546_IBUS_UCP_DEGLITCH_SHIFT,
		SC8546_IBUS_UCP_DEGLITCH_5MS);
	/* close ufcs rx_buff busy & ack timeout interrupt */
	ret += sc8546_write_mask(di, SC8546_UFCS_MASK2_REG,
		SC8546_UFCS_MASK2_RX_BUF_BUSY_MASK,
		SC8546_UFCS_MASK2_RX_BUF_BUSY_SHIFT, 1);
	ret += sc8546_write_mask(di, SC8546_UFCS_MASK1_REG,
		SC8546_UFCS_MASK1_ACK_REC_TIMEOUT_MASK,
		SC8546_UFCS_MASK1_ACK_REC_TIMEOUT_SHIFT, 1);
	if (ret)
		hwlog_err("reg_init failed\n");

	return ret;
}

static int sc8546_charge_init(void *dev_data)
{
	struct sc8546_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	if (sc8546_reg_init(di))
		return -EPERM;

	di->init_finish_flag = true;
	return 0;
}

static int sc8546_reg_reset_and_init(void *dev_data)
{
	int ret;
	struct sc8546_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	ret = sc8546_reg_reset(di);
	ret += sc8546_reg_init(di);

	return ret;
}

static int sc8546_charge_exit(void *dev_data)
{
	int ret;
	struct sc8546_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	ret = sc8546_sc_charge_enable(SC8546_SWITCHCAP_DISABLE, di);
	di->fcp_support = false;
	di->init_finish_flag = false;
	di->int_notify_enable_flag = false;

	return ret;
}

static int sc8546_batinfo_exit(void *dev_data)
{
	return 0;
}

static int sc8546_batinfo_init(void *dev_data)
{
	struct sc8546_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	return sc8546_chip_init(di);
}

/* print the register head in charging process */
static int sc8546_register_head(char *buffer, int size, void *dev_data)
{
	struct sc8546_device_info *di = dev_data;

	if (!di || !buffer)
		return -EPERM;

	snprintf(buffer, size,
		"dev        mode   Vbus   Ibus   Vbat   Ibat   Vusb   Vout   Temp");

	return 0;
}

static int sc8546_dump_reg(char *buffer, int size, void *dev_data)
{
	char buff[SC8546_BUF_LEN] = { 0 };
	u8 reg = 0;
	struct sc8546_device_info *di = dev_data;
	struct sc8646_dump_value dv = { 0 };

	if (!di || !buffer)
		return -EPERM;

	dv.vbat = sc8546_get_vbat_mv(di);
	(void)sc8546_get_ibat_ma(&dv.ibat, di);
	(void)sc8546_get_vbus_mv(&dv.vbus, di);
	(void)sc8546_get_ibus_ma(&dv.ibus, di);
	(void)sc8546_get_vusb_mv(&dv.vusb, di);
	(void)sc8546_get_vout_mv(&dv.vout, di);
	(void)sc8546_get_device_temp(&dv.temp, di);
	(void)sc8546_read_byte(di, SC8546_CTRL3_REG, &reg);
	snprintf(buff, sizeof(buff), "%s    ", di->name);
	strncat(buffer, buff, strlen(buff));

	if (sc8546_is_device_close(di))
		snprintf(buff, sizeof(buff), "%s", "OFF    ");
	else if (((reg & SC8546_CHG_MODE_MASK) >> SC8546_CHG_MODE_SHIFT) ==
		SC8546_CHG_MODE_BYPASS)
		snprintf(buff, sizeof(buff), "%s", "LVC    ");
	else if (((reg & SC8546_CHG_MODE_MASK) >> SC8546_CHG_MODE_SHIFT) ==
		SC8546_CHG_MODE_CHGPUMP)
		snprintf(buff, sizeof(buff), "%s", "SC     ");

	strncat(buffer, buff, strlen(buff));
	snprintf(buff, sizeof(buff), "%-7d%-7d%-7d%-7d%-7d%-7d%-7d",
		dv.vbus, dv.ibus, dv.vbat, dv.ibat, dv.vusb, dv.vout, dv.temp);
	strncat(buffer, buff, strlen(buff));

	return 0;
}

static struct dc_ic_ops g_sc8546_lvc_ops = {
	.dev_name = "sc8546",
	.ic_init = sc8546_charge_init,
	.ic_exit = sc8546_charge_exit,
	.ic_enable = sc8546_lvc_charge_enable,
	.ic_discharge = sc8546_discharge,
	.is_ic_close = sc8546_is_device_close,
	.get_ic_id = sc8546_get_device_id,
	.config_ic_watchdog = sc8546_config_watchdog_ms,
	.kick_ic_watchdog = sc8546_kick_watchdog_ms,
	.ic_reg_reset_and_init = sc8546_reg_reset_and_init,
};

static struct dc_ic_ops g_sc8546_sc_ops = {
	.dev_name = "sc8546",
	.ic_init = sc8546_charge_init,
	.ic_exit = sc8546_charge_exit,
	.ic_enable = sc8546_sc_charge_enable,
	.ic_discharge = sc8546_discharge,
	.is_ic_close = sc8546_is_device_close,
	.get_ic_id = sc8546_get_device_id,
	.config_ic_watchdog = sc8546_config_watchdog_ms,
	.kick_ic_watchdog = sc8546_kick_watchdog_ms,
	.ic_reg_reset_and_init = sc8546_reg_reset_and_init,
};

static struct dc_batinfo_ops g_sc8546_batinfo_ops = {
	.init = sc8546_batinfo_init,
	.exit = sc8546_batinfo_exit,
	.get_bat_btb_voltage = sc8546_get_vbat_mv,
	.get_bat_package_voltage = sc8546_get_vbat_mv,
	.get_vbus_voltage = sc8546_get_vbus_mv,
	.get_bat_current = sc8546_get_ibat_ma,
	.get_ic_ibus = sc8546_get_ibus_ma,
	.get_ic_vusb = sc8546_get_vusb_mv,
	.get_ic_vout = sc8546_get_vout_mv,
	.get_ic_temp = sc8546_get_device_temp,
};

static struct power_log_ops g_sc8546_log_ops = {
	.dev_name = "sc8546",
	.dump_log_head = sc8546_register_head,
	.dump_log_content = sc8546_dump_reg,
};

static void sc8546_init_ops_dev_data(struct sc8546_device_info *di)
{
	memcpy(&di->lvc_ops, &g_sc8546_lvc_ops, sizeof(struct dc_ic_ops));
	di->lvc_ops.dev_data = (void *)di;
	memcpy(&di->sc_ops, &g_sc8546_sc_ops, sizeof(struct dc_ic_ops));
	di->sc_ops.dev_data = (void *)di;
	memcpy(&di->batinfo_ops, &g_sc8546_batinfo_ops, sizeof(struct dc_batinfo_ops));
	di->batinfo_ops.dev_data = (void *)di;
	memcpy(&di->log_ops, &g_sc8546_log_ops, sizeof(struct power_log_ops));
	di->log_ops.dev_data = (void *)di;

	if (!di->ic_role) {
		snprintf(di->name, CHIP_DEV_NAME_LEN, "sc8546");
	} else {
		snprintf(di->name, CHIP_DEV_NAME_LEN, "sc8546_%d", di->ic_role);
		di->lvc_ops.dev_name = di->name;
		di->sc_ops.dev_name = di->name;
		di->log_ops.dev_name = di->name;
	}
}

static void sc8546_ops_register(struct sc8546_device_info *di)
{
	int ret;

	sc8546_init_ops_dev_data(di);

	ret = dc_ic_ops_register(LVC_MODE, di->ic_role, &di->lvc_ops);
	ret += dc_ic_ops_register(SC_MODE, di->ic_role, &di->sc_ops);
	ret += dc_batinfo_ops_register(di->ic_role, &di->batinfo_ops, di->device_id);
	if (ret)
		hwlog_err("sysinfo ops register failed\n");

	ret = sc8546_protocol_ops_register(di);
	if (ret)
		hwlog_err("scp or fcp ops register failed\n");

	ret = sc8546_ufcs_ops_register(di);
	if (ret)
		hwlog_err("ufcs ops register failed\n");

	power_log_ops_register(&di->log_ops);
}

static void sc8546_fault_event_notify(unsigned long event, void *data)
{
	power_event_anc_notify(POWER_ANT_SC_FAULT, event, data);
}

static void sc8546_interrupt_handle(struct sc8546_device_info *di,
	struct nty_data *data)
{
	int val = 0;
	u8 flag = data->event1;
	u8 flag1 = data->event2;
	u8 scp_flag = data->event3;

	if (flag1 & SC8546_VAC_OVP_FLAG_MASK) {
		hwlog_info("AC OVP happened\n");
		sc8546_fault_event_notify(POWER_NE_DC_FAULT_AC_OVP, data);
	} else if (flag & SC8546_VBAT_OVP_FLAG_MASK) {
		val = sc8546_get_vbat_mv(di);
		hwlog_info("BAT OVP happened, vbat=%d\n", val);
		if (val >= SC8546_VBAT_OVP_TH_INIT)
			sc8546_fault_event_notify(POWER_NE_DC_FAULT_VBAT_OVP, data);
	} else if (flag & SC8546_IBAT_OCP_FLAG_MASK) {
		sc8546_get_ibat_ma(&val, di);
		hwlog_info("BAT OCP happened, ibat=%d\n", val);
		if (val >= SC8546_IBAT_OCP_TH_INIT)
			sc8546_fault_event_notify(POWER_NE_DC_FAULT_IBAT_OCP, data);
	} else if (flag & SC8546_VBUS_OVP_FLAG_MASK) {
		sc8546_get_vbus_mv(&val, di);
		hwlog_info("BUS OVP happened, vbus=%d\n", val);
		if (val >= SC8546_VBUS_OVP_TH_INIT)
			sc8546_fault_event_notify(POWER_NE_DC_FAULT_VBUS_OVP, data);
	} else if (flag & SC8546_IBUS_OCP_FLAG_MASK) {
		sc8546_get_ibus_ma(&val, di);
		hwlog_info("BUS OCP happened, ibus=%d\n", val);
		if (val >= SC8546_IBUS_OCP_TH_INIT)
			sc8546_fault_event_notify(POWER_NE_DC_FAULT_IBUS_OCP, data);
	} else if (flag & SC8546_VOUT_OVP_FLAG_MASK) {
		hwlog_info("VOUT OVP happened\n");
	}

	if (scp_flag & SC8546_TRANS_DONE_FLAG_MASK)
		di->scp_trans_done = true;
}

static void sc8546_charge_work(struct work_struct *work)
{
	u8 flag[6] = { 0 }; /* 6: six regs */
	struct sc8546_device_info *di = NULL;
	struct nty_data *data = NULL;

	if (!work)
		return;

	di = container_of(work, struct sc8546_device_info, charge_work);
	if (!di || !di->client) {
		hwlog_err("di is null\n");
		return;
	}

	(void)sc8546_read_byte(di, SC8546_INT_FLAG_REG, &flag[0]);
	(void)sc8546_read_byte(di, SC8546_VAC_OVP_REG, &flag[1]);
	(void)sc8546_read_byte(di, SC8546_VDROP_OVP_REG, &flag[2]);
	(void)sc8546_read_byte(di, SC8546_CONVERTER_STATE_REG, &flag[3]);
	(void)sc8546_read_byte(di, SC8546_CTRL3_REG, &flag[4]);
	(void)sc8546_read_byte(di, SC8546_SCP_FLAG_MASK_REG, &flag[5]);

	data = &(di->notify_data);
	data->event1 = flag[0];
	data->event2 = flag[1];
	data->event3 = flag[5];
	data->addr = di->client->addr;

	if (di->int_notify_enable_flag) {
		sc8546_interrupt_handle(di, data);
		sc8546_dump_register(di);
	}

	hwlog_info("reg[0x%x]=0x%x, reg[0x%x]=0x%x, reg[0x%x]=0x%x,\t"
		"reg[0x%x]=0x%x, reg[0x%x]=0x%x, reg[0x%x]=0x%x\n",
		SC8546_INT_FLAG_REG, flag[0], SC8546_VAC_OVP_REG, flag[1],
		SC8546_VDROP_OVP_REG, flag[2], SC8546_CONVERTER_STATE_REG, flag[3],
		SC8546_CTRL3_REG, flag[4], SC8546_SCP_FLAG_MASK_REG, flag[5]);
}

static void sc8546_interrupt_work(struct work_struct *work)
{
	struct sc8546_device_info *di = NULL;
	u8 ufcs_irq[2] = { 0 }; /* 2: two interrupt regs */

	if (!work)
		return;

	di = container_of(work, struct sc8546_device_info, irq_work);
	if (!di || !di->client) {
		hwlog_err("di is null\n");
		return;
	}

	enable_irq(di->irq_int);
	(void)sc8546_read_byte(di, SC8546_UFCS_ISR1_REG, &ufcs_irq[0]);
	(void)sc8546_read_byte(di, SC8546_UFCS_ISR2_REG, &ufcs_irq[1]);

	if (ufcs_irq[0] & SC8546_UFCS_ISR1_DATA_READY_MASK) {
		if (!di->ufcs_communicating_flag)
			power_event_bnc_notify(POWER_BNT_UFCS,
				POWER_NE_UFCS_REC_UNSOLICITED_DATA, NULL);
		sc8546_ufcs_add_msg(di);
	}

	di->ufcs_irq[0] |= ufcs_irq[0];
	di->ufcs_irq[1] |= ufcs_irq[1];
	hwlog_info("UFCS_ISR1 [0x%x]=0x%x, UFCS_ISR2 [0x%x]=0x%x\n",
		SC8546_UFCS_ISR1_REG, ufcs_irq[0], SC8546_UFCS_ISR2_REG, ufcs_irq[1]);

	schedule_work(&di->charge_work);
}

static irqreturn_t sc8546_interrupt(int irq, void *_di)
{
	struct sc8546_device_info *di = _di;

	if (!di)
		return IRQ_HANDLED;

	if (di->init_finish_flag)
		di->int_notify_enable_flag = true;

	hwlog_info("int happened\n");
	disable_irq_nosync(di->irq_int);
	queue_work(system_highpri_wq, &di->irq_work);

	return IRQ_HANDLED;
}

static int sc8546_irq_init(struct sc8546_device_info *di,
	struct device_node *np)
{
	int ret;

	ret = power_gpio_config_interrupt(np,
		"intr_gpio", "sc8546_gpio_int", &di->gpio_int, &di->irq_int);
	if (ret)
		return ret;

	ret = request_irq(di->irq_int, sc8546_interrupt,
		IRQF_TRIGGER_FALLING, "sc8546_int_irq", di);
	if (ret) {
		hwlog_err("gpio irq request failed\n");
		di->irq_int = -1;
		gpio_free(di->gpio_int);
		return ret;
	}

	INIT_WORK(&di->irq_work, sc8546_interrupt_work);
	INIT_WORK(&di->charge_work, sc8546_charge_work);
	enable_irq_wake(di->irq_int);
	return 0;
}

static int sc8546_notifier_call(struct notifier_block *nb,
	unsigned long event, void *data)
{
	struct sc8546_device_info *di = container_of(nb, struct sc8546_device_info, event_nb);

	if (!di)
		return NOTIFY_OK;

	switch (event) {
	case POWER_NE_USB_DISCONNECT:
		hwlog_info("reset ic\n");
		(void)sc8546_write_mask(di, SC8546_DPDM_CTRL1_REG,
			SC8546_DPDM_EN_MASK, SC8546_DPDM_EN_SHIFT, 0);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static void sc8546_parse_dts(struct device_node *np,
	struct sc8546_device_info *di)
{
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"switching_frequency", &di->dts_sw_freq, 700); /* default 700khz */
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"switching_frequency_shift", &di->dts_sw_freq_shift, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "scp_support",
		(u32 *)&(di->dts_scp_support), 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "fcp_support",
		(u32 *)&(di->dts_fcp_support), 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "ufcs_support",
		(u32 *)&(di->dts_ufcs_support), 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "ic_role",
		(u32 *)&(di->ic_role), IC_ONE);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"sense_r_config", &di->sense_r_config, SENSE_R_5_MOHM);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"sense_r_actual", &di->sense_r_actual, SENSE_R_5_MOHM);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"i2c_recovery", &di->i2c_recovery, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"i2c_recovery_addr", &di->i2c_recovery_addr, 0);
}

static void sc8546_init_lock_mutex(struct sc8546_device_info *di)
{
	mutex_init(&di->scp_detect_lock);
	mutex_init(&di->ufcs_detect_lock);
	mutex_init(&di->ufcs_node_lock);
	mutex_init(&di->accp_adapter_reg_lock);
}

static void sc8546_destroy_lock_mutex(struct sc8546_device_info *di)
{
	mutex_destroy(&di->scp_detect_lock);
	mutex_destroy(&di->ufcs_detect_lock);
	mutex_destroy(&di->ufcs_node_lock);
	mutex_destroy(&di->accp_adapter_reg_lock);
}

static int sc8546_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret;
	struct sc8546_device_info *di = NULL;
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
	sc8546_parse_dts(np, di);

	ret = sc8546_get_device_id(di);
	if ((ret < 0) || (ret == DC_DEVICE_ID_END))
		goto sc8546_fail_0;

	sc8546_init_lock_mutex(di);

	if (di->dts_ufcs_support) {
		di->event_nb.notifier_call = sc8546_notifier_call;
		if (power_event_bnc_register(POWER_BNT_CONNECT, &di->event_nb))
			goto sc8546_fail_1;
	}

	ret = sc8546_reg_reset(di);
	if (ret)
		goto sc8546_fail_1;

	ret = sc8546_reg_init(di);
	if (ret)
		goto sc8546_fail_1;

	(void)power_pinctrl_config(di->dev, "pinctrl-names", 1); /* 1:pinctrl-names length */
	ret = sc8546_irq_init(di, np);
	if (ret)
		goto sc8546_fail_1;

	sc8546_ops_register(di);
	i2c_set_clientdata(client, di);
	return 0;

sc8546_fail_1:
	sc8546_destroy_lock_mutex(di);
sc8546_fail_0:
	di->chip_already_init = 0;
	devm_kfree(&client->dev, di);

	return ret;
}

static int sc8546_remove(struct i2c_client *client)
{
	struct sc8546_device_info *di = i2c_get_clientdata(client);

	if (!di)
		return -ENODEV;

	sc8546_reg_reset(di);

	if (di->irq_int)
		free_irq(di->irq_int, di);
	if (di->gpio_int)
		gpio_free(di->gpio_int);

	sc8546_ufcs_free_node_list(di);
	sc8546_destroy_lock_mutex(di);
	devm_kfree(&client->dev, di);

	return 0;
}

static void sc8546_shutdown(struct i2c_client *client)
{
	struct sc8546_device_info *di = i2c_get_clientdata(client);

	if (!di)
		return;

	sc8546_reg_reset(di);
}

MODULE_DEVICE_TABLE(i2c, sc8546);
static const struct of_device_id sc8546_of_match[] = {
	{
		.compatible = "sc8546",
		.data = NULL,
	},
	{},
};

static const struct i2c_device_id sc8546_i2c_id[] = {
	{ "sc8546", 0 }, {}
};

static struct i2c_driver sc8546_driver = {
	.probe = sc8546_probe,
	.remove = sc8546_remove,
	.shutdown = sc8546_shutdown,
	.id_table = sc8546_i2c_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "sc8546",
		.of_match_table = of_match_ptr(sc8546_of_match),
	},
};

static int __init sc8546_init(void)
{
	return i2c_add_driver(&sc8546_driver);
}

static void __exit sc8546_exit(void)
{
	i2c_del_driver(&sc8546_driver);
}

module_init(sc8546_init);
module_exit(sc8546_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("sc8546 module driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
