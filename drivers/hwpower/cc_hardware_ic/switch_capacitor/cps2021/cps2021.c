// SPDX-License-Identifier: GPL-2.0
/*
 * cps2021.c
 *
 * cps2021 driver
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

#include "cps2021.h"
#include "cps2021_i2c.h"
#include "cps2021_scp.h"
#include "cps2021_ufcs.h"
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_pinctrl.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_device_id.h>

#define HWLOG_TAG cps2021_chg
HWLOG_REGIST();

#define BUF_LEN                              80
#define CPS2021_PAGE0_NUM                    0x12
#define CPS2021_PAGE1_NUM                    0x07
#define CPS2021_PAGE0_BASE                   CPS2021_CONTROL1_REG
#define CPS2021_PAGE1_BASE                   CPS2021_SCP_CTL_REG
#define CPS2021_DBG_VAL_SIZE                 6

static void cps2021_dump_register(struct cps2021_device_info *di)
{
	u8 i;
	u8 val = 0;

	for (i = CPS2021_CONTROL1_REG; i <= CPS2021_SCP_STIMER_REG; i++) {
		if ((i == CPS2021_FLAG1_REG) || (i == CPS2021_FLAG2_REG) ||
			(i == CPS2021_FLAG3_REG) || (i == CPS2021_SCP_ISR1_REG) ||
			(i == CPS2021_SCP_ISR2_REG))
			continue;

		if (cps2021_read_byte(di, i, &val))
			hwlog_err("dump_register read fail\n");
		hwlog_info("reg[%x]=0x%x\n", i, val);
	}
}

static int cps2021_reg_reset(void *dev_data)
{
	int ret;
	u8 reg = 0;
	struct cps2021_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	ret = cps2021_write_mask(di, CPS2021_CONTROL1_REG,
		CPS2021_CONTROL1_RESET_MASK, CPS2021_CONTROL1_RESET_SHIFT,
		CPS2021_CONTROL1_RST_ENABLE);
	if (ret)
		return -EIO;

	power_usleep(DT_USLEEP_1MS); /* wait soft reset ready, min:500us */
	ret = cps2021_read_byte(di, CPS2021_CONTROL1_REG, &reg);
	if (ret)
		return -EIO;

	hwlog_info("reg_reset[%x]=0x%x\n", CPS2021_CONTROL1_REG, reg);
	return 0;
}

static int cps2021_discharge(int enable, void *dev_data)
{
	return 0;
}

static int cps2021_is_device_close(void *dev_data)
{
	u8 reg = 0;
	u8 value;
	int ret;
	struct cps2021_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	ret = cps2021_read_byte(di, CPS2021_CONTROL1_REG, &reg);
	if (ret)
		return -EIO;

	value = (reg & CPS2021_CONTROL1_OPMODE_MASK) >> CPS2021_CONTROL1_OPMODE_SHIFT;
	if ((value < CPS2021_CONTROL1_FBYPASSMODE) || (value > CPS2021_CONTROL1_FCHGPUMPMODE))
		return -EIO;

	return 0;
}

static int cps2021_get_device_id(void *dev_data)
{
	u8 part_info = 0;
	int ret;
	static bool first_rd = true;
	struct cps2021_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	if (!first_rd)
		return di->device_id;

	first_rd = false;
	ret = cps2021_read_byte(di, CPS2021_DEVICE_INFO_REG, &part_info);
	if (ret) {
		first_rd = true;
		hwlog_err("get_device_id read fail\n");
		return -EIO;
	}
	hwlog_info("get_device_id[%x]=0x%x\n", CPS2021_DEVICE_INFO_REG, part_info);

	part_info = part_info & CPS2021_DEVICE_INFO_DEVID_MASK;
	switch (part_info) {
	case CPS2021_DEVICE_ID_CPS2021:
		di->device_id = SWITCHCAP_CPS2021;
		break;
	default:
		di->device_id = DC_DEVICE_ID_END;
		hwlog_err("switchcap get dev_id fail\n");
		break;
	}

	hwlog_info("device_id=0x%x\n", di->device_id);

	return di->device_id;
}

static int cps2021_get_vbat_mv(void *dev_data)
{
	u16 data = 0;
	int voltage, ret;
	struct cps2021_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	ret = cps2021_read_word(di, CPS2021_VBATADC_H_REG, &data);
	if (ret)
		return -EIO;

	voltage = (int)(s16)data * CPS2021_ADC_RESOLUTION_2;
	hwlog_info("VBAT_ADC=0x%x, vbat=%d\n", data, voltage);

	return voltage;
}

static int cps2021_get_ibat_ma(int *ibat, void *dev_data)
{
	u16 data = 0;
	int curr;
	struct cps2021_device_info *di = dev_data;

	if (!ibat || !di || (di->sense_r_actual == 0))
		return -ENODEV;

	if (cps2021_read_word(di, CPS2021_IBATADC_H_REG, &data))
		return -EIO;

	curr = (int)(s16)data * CPS2021_ADC_RESOLUTION_2;
	*ibat = curr * di->sense_r_config / di->sense_r_actual;
	hwlog_info("IBAT_ADC=0x%x, ibat=%d\n", data, *ibat);

	return 0;
}

static int cps2021_get_ibus_ma(int *ibus, void *dev_data)
{
	u16 data = 0;
	struct cps2021_device_info *di = dev_data;

	if (!ibus || !di)
		return -ENODEV;

	if (cps2021_read_word(di, CPS2021_IBUSADC_H_REG, &data))
		return -EIO;

	*ibus = (int)(s16)data * CPS2021_ADC_RESOLUTION_2;
	hwlog_info("IBUS_ADC=0x%x, ibus=%d\n", data, *ibus);

	return 0;
}

int cps2021_get_vbus_mv(int *vbus, void *dev_data)
{
	u16 data = 0;
	struct cps2021_device_info *di = dev_data;

	if (!vbus || !di)
		return -ENODEV;

	if (cps2021_read_word(di, CPS2021_VBUSADC_H_REG, &data))
		return -EIO;

	*vbus = (int)(s16)data * CPS2021_ADC_RESOLUTION_4;
	hwlog_info("VBUS_ADC=0x%x, vbus=%d\n", data, *vbus);

	return 0;
}

static int cps2021_get_device_temp(int *temp, void *dev_data)
{
	u8 reg = 0;
	int ret;
	struct cps2021_device_info *di = dev_data;

	if (!temp || !di)
		return -ENODEV;

	ret = cps2021_read_byte(di, CPS2021_TDIEADC_REG, &reg);
	if (ret)
		return -EIO;

	*temp = reg - CPS2021_ADC_STANDARD_TDIE;
	hwlog_info("TDIE_ADC=0x%x\n, tdie=%d", reg, *temp);

	return 0;
}

static int cps2021_set_nwatchdog(int disable, void *dev_data)
{
	int ret;
	u8 reg = 0;
	u8 value;
	struct cps2021_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	value = disable ? CPS2021_NWDT_DISABLE : CPS2021_NWDT_ENABLE;
	ret = cps2021_write_mask(di, CPS2021_CONTROL1_REG, CPS2021_CONTROL1_NEN_WATCHDOG_MASK,
		CPS2021_CONTROL1_NEN_WATCHDOG_SHIFT, value);
	if (ret)
		return -EIO;

	ret = cps2021_read_byte(di, CPS2021_CONTROL1_REG, &reg);
	if (ret)
		return -EIO;

	hwlog_info("watchdog[%x]=0x%x\n", CPS2021_CONTROL1_REG, reg);
	return 0;
}

static int cps2021_config_watchdog_ms(int time, void *dev_data)
{
	u8 val;
	int ret;
	u8 reg = 0;
	struct cps2021_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	/* 0,1,2,3,4,5,6,7 represents different wdt gears */
	if (time >= CPS2021_WTD_CONFIG_TIMING_80000MS)
		val = 7;
	else if (time >= CPS2021_WTD_CONFIG_TIMING_40000MS)
		val = 6;
	else if (time >= CPS2021_WTD_CONFIG_TIMING_20000MS)
		val = 5;
	else if (time >= CPS2021_WTD_CONFIG_TIMING_10000MS)
		val = 4;
	else if (time >= CPS2021_WTD_CONFIG_TIMING_5000MS)
		val = 3;
	else if (time >= CPS2021_WTD_CONFIG_TIMING_2000MS)
		val = 2;
	else if (time >= CPS2021_WTD_CONFIG_TIMING_1000MS)
		val = 1;
	else
		val = 0;

	ret = cps2021_write_mask(di, CPS2021_CONTROL1_REG, CPS2021_CONTROL1_WATCHDOG_REF_MASK,
		CPS2021_CONTROL1_WATCHDOG_REF_SHIFT, val);
	if (ret)
		return -EIO;

	ret = cps2021_read_byte(di, CPS2021_CONTROL1_REG, &reg);
	if (ret)
		return -EIO;

	hwlog_info("config_watchdog_ms[%x]=0x%x\n", CPS2021_CONTROL1_REG, reg);
	return 0;
}

static int cps2021_kick_watchdog_ms(void *dev_data)
{
	return 0;
}

static int cps2021_config_rlt_uvp_ref(int rltuvp_rate, struct cps2021_device_info *di)
{
	u8 value;
	int ret;
	u8 reg = 0;

	value = (rltuvp_rate >= CPS2021_RLTVUVP_REF_1P04) ?
		CPS2021_RLTVUVP_REF_1P04 : rltuvp_rate;
	ret = cps2021_write_mask(di, CPS2021_CONTROL3_REG, CPS2021_CONTROL3_RLTVUVP_REF_MASK,
		CPS2021_CONTROL3_RLTVUVP_REF_SHIFT, value);
	if (ret)
		return -EIO;

	ret = cps2021_read_byte(di, CPS2021_CONTROL3_REG, &reg);
	if (ret)
		return -EIO;

	hwlog_info("config_rlt_uvp_ref[%x]=0x%x\n", CPS2021_CONTROL3_REG, reg);
	return 0;
}

static int cps2021_config_rlt_ovp_ref(int rltovp_rate, struct cps2021_device_info *di)
{
	u8 value;
	int ret;
	u8 reg = 0;

	value = (rltovp_rate >= CPS2021_RLTVOVP_REF_1P25) ?
		CPS2021_RLTVOVP_REF_1P25 : rltovp_rate;
	ret = cps2021_write_mask(di, CPS2021_CONTROL3_REG, CPS2021_CONTROL3_RLTVOVP_REF_MASK,
		CPS2021_CONTROL3_RLTVOVP_REF_SHIFT, value);
	if (ret)
		return -EIO;

	ret = cps2021_read_byte(di, CPS2021_CONTROL3_REG, &reg);
	if (ret)
		return -EIO;

	hwlog_info("config_rlt_ovp_ref[%x]=0x%x\n", CPS2021_CONTROL3_REG, reg);
	return 0;
}

static int cps2021_config_vbat_ovp_ref_mv(int ovp_threshold,
	struct cps2021_device_info *di)
{
	u8 value;
	int ret;
	u8 reg = 0;

	if (ovp_threshold < CPS2021_VBAT_OVP_BASE)
		ovp_threshold = CPS2021_VBAT_OVP_BASE;

	if (ovp_threshold > CPS2021_VBAT_OVP_MAX)
		ovp_threshold = CPS2021_VBAT_OVP_MAX;

	value = (u8)((ovp_threshold - CPS2021_VBAT_OVP_BASE) /
			CPS2021_VBAT_OVP_STEP);

	ret = cps2021_write_mask(di, CPS2021_VBATOVP_REG, CPS2021_VBATOVP_VBATOVP_REF_MASK,
		CPS2021_VBATOVP_VBATOVP_REF_SHIFT, value);
	if (ret)
		return -EIO;

	ret = cps2021_read_byte(di, CPS2021_VBATOVP_REG, &reg);
	if (ret)
		return -EIO;

	hwlog_info("config_vbat_ovp_ref_mv[%x]=0x%x\n", CPS2021_VBATOVP_REG, reg);
	return 0;
}

static int cps2021_config_ibat_ocp_ref_ma(int ocp_threshold,
	struct cps2021_device_info *di)
{
	u8 value;
	int ret;
	u8 reg = 0;

	if (ocp_threshold < CPS2021_IBAT_OCP_BASE)
		ocp_threshold = CPS2021_IBAT_OCP_BASE;

	if (ocp_threshold > CPS2021_IBAT_OCP_MAX)
		ocp_threshold = CPS2021_IBAT_OCP_MAX;

	value = (u8)((ocp_threshold - CPS2021_IBAT_OCP_BASE) /
		CPS2021_IBAT_OCP_STEP);
	ret = cps2021_write_mask(di, CPS2021_IBATOCP_REG, CPS2021_IBATOCP_IBATOCP_REF_MASK,
		CPS2021_IBATOCP_IBATOCP_REF_SHIFT, value);
	if (ret)
		return -EIO;

	ret = cps2021_read_byte(di, CPS2021_IBATOCP_REG, &reg);
	if (ret)
		return -EIO;

	hwlog_info("config_ibat_ocp_ref_ma[%x]=0x%x\n", CPS2021_IBATOCP_REG, reg);
	return 0;
}

int cps2021_config_vbuscon_ovp_ref_mv(int ovp_threshold,
	struct cps2021_device_info *di)
{
	u8 value;
	int ret;
	u8 reg = 0;

	if (ovp_threshold < CPS2021_VBUSCON_OVP_BASE)
		ovp_threshold = CPS2021_VBUSCON_OVP_BASE;

	if (ovp_threshold > CPS2021_VBUSCON_OVP_MAX)
		ovp_threshold = CPS2021_VBUSCON_OVP_MAX;

	value = (u8)((ovp_threshold - CPS2021_VBUSCON_OVP_BASE) /
		CPS2021_VBUSCON_OVP_STEP);
	ret = cps2021_write_mask(di, CPS2021_VBUSCON_OVP_REG, CPS2021_VBUSCON_OVP_VBUSCON_OVP_REF_MASK,
		CPS2021_VBUSCON_OVP_VBUSCON_OVP_REF_SHIFT, value);
	if (ret)
		return -EIO;

	ret = cps2021_read_byte(di, CPS2021_VBUSCON_OVP_REG, &reg);
	if (ret)
		return -EIO;

	hwlog_info("config_ac_ovp_threshold_mv[%x]=0x%x\n", CPS2021_VBUSCON_OVP_REG, reg);
	return 0;
}

int cps2021_config_vbus_ovp_ref_mv(int ovp_threshold, struct cps2021_device_info *di)
{
	u8 value;
	int ret;
	u8 reg = 0;

	if (ovp_threshold < CPS2021_VBUS_OVP_BASE)
		ovp_threshold = CPS2021_VBUS_OVP_BASE;

	if (ovp_threshold > CPS2021_VBUS_OVP_MAX)
		ovp_threshold = CPS2021_VBUS_OVP_MAX;

	value = (u8)((ovp_threshold - CPS2021_VBUS_OVP_BASE) /
		CPS2021_VBUS_OVP_STEP);
	ret = cps2021_write_mask(di, CPS2021_VBUSOVP_REG, CPS2021_VBUSOVP_VBUSOVP_REF_MASK,
		CPS2021_VBUSOVP_VBUSOVP_REF_SHIFT, value);
	if (ret)
		return -EIO;

	ret = cps2021_read_byte(di, CPS2021_VBUSOVP_REG, &reg);
	if (ret)
		return -EIO;

	hwlog_info("config_vbus_ovp_ref_mv[%x]=0x%x\n", CPS2021_VBUSOVP_REG, reg);
	return 0;
}

static int cps2021_config_ibus_ucp_ref_ma(int ucp_threshold, struct cps2021_device_info *di)
{
	u8 value = 0;
	u8 ucp_th_value, pre_opmode;
	int ret;
	u8 reg = 0;

	if (ucp_threshold <= CPS2021_BUS_UCP_BASE_300MA_150MA) {
		ucp_threshold = CPS2021_BUS_UCP_BASE_300MA_150MA;
		ucp_th_value = 0;
	} else if (ucp_threshold <= CPS2021_BUS_UCP_BASE_500MA_250MA) {
		ucp_threshold = CPS2021_BUS_UCP_BASE_500MA_250MA;
		ucp_th_value = 1;
	} else {
		ucp_threshold = CPS2021_BUS_UCP_BASE_300MA_150MA;
		ucp_th_value = 0;
	}

	/* 1. save previous op mode */
	ret = cps2021_read_byte(di, CPS2021_CONTROL1_REG, &value);
	if (ret)
		return -EIO;
	pre_opmode = (value & CPS2021_CONTROL1_OPMODE_MASK) >> CPS2021_CONTROL1_OPMODE_SHIFT;

	/* 2. set op mode to init mode */
	ret = cps2021_write_mask(di, CPS2021_CONTROL1_REG,
		CPS2021_CONTROL1_OPMODE_MASK, CPS2021_CONTROL1_OPMODE_SHIFT,
		CPS2021_CONTROL1_OFFMODE);
	if (ret)
		return -EIO;

	/* 3. set IBUS_UCP reg */
	ret = cps2021_write_mask(di, CPS2021_IBUS_OCP_UCP_REG, CPS2021_IBUS_OCP_UCP_IBUSUCP_REF_MASK,
		CPS2021_IBUS_OCP_UCP_IBUSUCP_REF_SHIFT, ucp_th_value);
	if (ret)
		return -EIO;

	/* 4. set op mode to pre mode */
	ret = cps2021_write_mask(di, CPS2021_CONTROL1_REG, CPS2021_CONTROL1_OPMODE_MASK,
		CPS2021_CONTROL1_OPMODE_SHIFT, pre_opmode);
	if (ret)
		return -EIO;

	ret = cps2021_read_byte(di, CPS2021_IBUS_OCP_UCP_REG, &reg);
	if (ret)
		return -EIO;

	hwlog_info("config_ibus_ucp_threshold_ma[%x]=0x%x\n", CPS2021_IBUS_OCP_UCP_REG, reg);
	return 0;
}

static int cps2021_config_ibus_ocp_ref_ma(int ocp_threshold, int chg_mode,
	struct cps2021_device_info *di)
{
	u8 value;
	int ret, min_th, max_th;
	u8 reg = 0;

	if (chg_mode == CPS2021_CHG_MODE_BYPASS) {
		min_th = CPS2021_IBUS_OCP_BP_BASE;
		max_th = CPS2021_IBUS_OCP_BP_MAX;
	} else if (chg_mode == CPS2021_CHG_MODE_CHGPUMP) {
		min_th = CPS2021_IBUS_OCP_CP_BASE;
		max_th = CPS2021_IBUS_OCP_CP_MAX;
	} else {
		hwlog_err("chg mode error, chg_mode=%d\n", chg_mode);
		return -EPERM;
	}

	ocp_threshold = (ocp_threshold < min_th) ? min_th : ocp_threshold;
	ocp_threshold = (ocp_threshold > max_th) ? max_th : ocp_threshold;
	value = (u8)((ocp_threshold - min_th) / CPS2021_IBUS_OCP_STEP);
	ret = cps2021_write_mask(di, CPS2021_IBUS_OCP_UCP_REG,
		CPS2021_IBUS_OCP_UCP_IBUSOCP_REF_MASK,
		CPS2021_IBUS_OCP_UCP_IBUSOCP_REF_SHIFT, value);
	if (ret)
		return -EIO;

	ret = cps2021_read_byte(di, CPS2021_IBUS_OCP_UCP_REG, &reg);
	if (ret)
		return -EIO;

	hwlog_info("config_ibus_ocp_threshold_ma[%x]=0x%x\n",
		CPS2021_IBUS_OCP_UCP_REG, reg);
	return 0;
}

static void cps2021_get_freq_parameters(int data, int *freq, int *freq_shift)
{
	switch (data) {
	case CPS2021_SW_FREQ_450KHZ:
		*freq = CPS2021_FSW_SET_SW_FREQ_500KHZ;
		*freq_shift = CPS2021_SW_FREQ_SHIFT_M_P10;
		break;
	case CPS2021_SW_FREQ_500KHZ:
		*freq = CPS2021_FSW_SET_SW_FREQ_500KHZ;
		*freq_shift = CPS2021_SW_FREQ_SHIFT_NORMAL;
		break;
	case CPS2021_SW_FREQ_550KHZ:
		*freq = CPS2021_FSW_SET_SW_FREQ_500KHZ;
		*freq_shift = CPS2021_SW_FREQ_SHIFT_P_P10;
		break;
	case CPS2021_SW_FREQ_675KHZ:
		*freq = CPS2021_FSW_SET_SW_FREQ_750KHZ;
		*freq_shift = CPS2021_SW_FREQ_SHIFT_M_P10;
		break;
	case CPS2021_SW_FREQ_750KHZ:
		*freq = CPS2021_FSW_SET_SW_FREQ_750KHZ;
		*freq_shift = CPS2021_SW_FREQ_SHIFT_NORMAL;
		break;
	case CPS2021_SW_FREQ_825KHZ:
		*freq = CPS2021_FSW_SET_SW_FREQ_750KHZ;
		*freq_shift = CPS2021_SW_FREQ_SHIFT_P_P10;
		break;
	case CPS2021_SW_FREQ_1000KHZ:
		*freq = CPS2021_FSW_SET_SW_FREQ_1000KHZ;
		*freq_shift = CPS2021_SW_FREQ_SHIFT_NORMAL;
		break;
	case CPS2021_SW_FREQ_1250KHZ:
		*freq = CPS2021_FSW_SET_SW_FREQ_1250KHZ;
		*freq_shift = CPS2021_SW_FREQ_SHIFT_NORMAL;
		break;
	case CPS2021_SW_FREQ_1500KHZ:
		*freq = CPS2021_FSW_SET_SW_FREQ_1500KHZ;
		*freq_shift = CPS2021_SW_FREQ_SHIFT_NORMAL;
		break;
	default:
		*freq = CPS2021_FSW_SET_SW_FREQ_500KHZ;
		*freq_shift = CPS2021_SW_FREQ_SHIFT_NORMAL;
		break;
	}
}

static int cps2021_config_switching_frequency(int data, struct cps2021_device_info *di)
{
	int freq = 0;
	int freq_shift = 0;
	int ret;
	u8 reg = 0;

	cps2021_get_freq_parameters(data, &freq, &freq_shift);
	ret = cps2021_write_mask(di, CPS2021_CONTROL2_REG, CPS2021_CONTROL2_FREQ_MASK,
		CPS2021_CONTROL2_FREQ_SHIFT, freq);
	if (ret)
		return -EIO;

	ret = cps2021_write_mask(di, CPS2021_CONTROL2_REG, CPS2021_CONTROL2_ADJUST_FREQ_MASK,
		CPS2021_CONTROL2_ADJUST_FREQ_SHIFT, freq_shift);
	if (ret)
		return -EIO;

	ret = cps2021_read_byte(di, CPS2021_CONTROL2_REG, &reg);
	if (ret)
		return -EIO;

	hwlog_info("config_switching_frequency[%x]=0x%x\n",
		CPS2021_CONTROL2_REG, reg);

	return 0;
}

static void cps2021_common_opt_regs(struct cps2021_device_info *di)
{
	cps2021_config_vbuscon_ovp_ref_mv(CPS2021_VBUSCON_OVP_REF_INIT, di);
	cps2021_config_vbus_ovp_ref_mv(CPS2021_VBUS_OVP_REF_INIT, di);
	cps2021_config_ibus_ocp_ref_ma(CPS2021_CP_IBUS_OCP_REF_INIT,
		CPS2021_CHG_MODE_CHGPUMP, di);
}

static void cps2021_lvc_opt_regs(struct cps2021_device_info *di)
{
	cps2021_config_vbuscon_ovp_ref_mv(CPS2021_BP_VBUSCON_OVP_REF_INIT, di);
	cps2021_config_vbus_ovp_ref_mv(CPS2021_BP_VBUS_OVP_REF_INIT, di);
	cps2021_config_ibus_ocp_ref_ma(CPS2021_BP_IBUS_OCP_REF_INIT,
		CPS2021_CHG_MODE_BYPASS, di);
}

static void cps2021_sc_opt_regs(struct cps2021_device_info *di)
{
	cps2021_config_vbuscon_ovp_ref_mv(CPS2021_VBUSCON_OVP_REF_INIT, di);
	cps2021_config_vbus_ovp_ref_mv(CPS2021_VBUS_OVP_REF_INIT, di);
	cps2021_config_ibus_ocp_ref_ma(CPS2021_CP_IBUS_OCP_REF_INIT,
		CPS2021_CHG_MODE_CHGPUMP, di);
}

static int cps2021_threshold_reg_init(struct cps2021_device_info *di)
{
	int ret;

	cps2021_common_opt_regs(di);
	ret = cps2021_config_rlt_ovp_ref(CPS2021_RLTVOVP_REF_1P25, di);
	ret += cps2021_config_rlt_uvp_ref(CPS2021_RLTVUVP_REF_1P01, di);
	ret += cps2021_config_vbat_ovp_ref_mv(CPS2021_VBAT_OVP_REF_INIT, di);
	ret += cps2021_config_ibat_ocp_ref_ma(CPS2021_IBAT_OCP_REF_INIT, di);
	ret += cps2021_config_ibus_ucp_ref_ma(CPS2021_BUS_UCP_BASE_300MA_150MA, di);
	if (ret)
		hwlog_err("protect threshold init fail\n");

	return ret;
}

static int cps2021_lvc_charge_enable(int enable, void *dev_data)
{
	int ret;
	u8 reg = 0;
	u8 value;
	struct cps2021_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	value = enable ? CPS2021_CONTROL1_FBYPASSMODE : CPS2021_CONTROL1_OFFMODE;
	if (value == CPS2021_CONTROL1_FBYPASSMODE)
		cps2021_lvc_opt_regs(di);

	/* 1. off mode */
	ret = cps2021_write_mask(di, CPS2021_CONTROL1_REG, CPS2021_CONTROL1_OPMODE_MASK,
		CPS2021_CONTROL1_OPMODE_SHIFT, CPS2021_CONTROL1_OFFMODE);
	if (ret)
		return -EIO;

	/* 2. enable lvc mode */
	ret = cps2021_write_mask(di, CPS2021_CONTROL1_REG, CPS2021_CONTROL1_OPMODE_MASK,
		CPS2021_CONTROL1_OPMODE_SHIFT, value);
	if (ret)
		return -EIO;

	if (value == CPS2021_CONTROL1_OFFMODE)
		cps2021_common_opt_regs(di);

	ret = cps2021_read_byte(di, CPS2021_CONTROL1_REG, &reg);
	if (ret)
		return -EIO;

	hwlog_info("charge_enable[%x]=0x%x\n", CPS2021_CONTROL1_REG, reg);
	return 0;
}

static int cps2021_sc_charge_enable(int enable, void *dev_data)
{
	int ret;
	u8 reg = 0;
	u8 value;
	struct cps2021_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	value = enable ? CPS2021_CONTROL1_FCHGPUMPMODE : CPS2021_CONTROL1_OFFMODE;
	if (value == CPS2021_CONTROL1_FCHGPUMPMODE)
		cps2021_sc_opt_regs(di);

	/* 1. off mode */
	ret = cps2021_write_mask(di, CPS2021_CONTROL1_REG, CPS2021_CONTROL1_OPMODE_MASK,
		CPS2021_CONTROL1_OPMODE_SHIFT, CPS2021_CONTROL1_OFFMODE);
	if (ret)
		return -EIO;
	/* 2. enable sc mode */
	ret = cps2021_write_mask(di, CPS2021_CONTROL1_REG, CPS2021_CONTROL1_OPMODE_MASK,
		CPS2021_CONTROL1_OPMODE_SHIFT, value);
	if (ret)
		return -EIO;

	if (value == CPS2021_CONTROL1_OFFMODE)
		cps2021_common_opt_regs(di);

	ret = cps2021_read_byte(di, CPS2021_CONTROL1_REG, &reg);
	if (ret)
		return -EIO;

	hwlog_info("ic_role=%d, charge_enable[%x]=0x%x\n", di->ic_role, CPS2021_CONTROL1_REG, reg);
	return 0;
}

static int cps2021_chip_init(void *dev_data)
{
	return 0;
}

static int cps2021_reg_init(struct cps2021_device_info *di)
{
	int ret;
	u8 reg_val1 = 0;

	/* enable watchdog */
	ret = cps2021_set_nwatchdog(1, di);
	cps2021_threshold_reg_init(di);

	/* enable EN_COMP */
	ret += cps2021_write_mask(di, CPS2021_CONTROL3_REG,
		CPS2021_CONTROL3_EN_COMP_MASK, CPS2021_CONTROL3_EN_COMP_SHIFT, 1);
	(void)cps2021_read_byte(di, CPS2021_CONTROL3_REG, &reg_val1);
	hwlog_info("reg[%x]=0x%x\n", CPS2021_CONTROL3_REG, reg_val1);

	ret += cps2021_config_switching_frequency(di->switching_frequency, di);
	ret += cps2021_write_byte(di, CPS2021_FLAG_MASK1_REG, CPS2021_FLAG_MASK1_INIT_REG);
	ret += cps2021_write_byte(di, CPS2021_FLAG_MASK2_REG, CPS2021_FLAG_MASK2_INIT_REG);
	ret += cps2021_write_byte(di, CPS2021_FLAG_MASK3_REG, CPS2021_FLAG_MASK3_INIT_REG);
	ret += cps2021_write_mask(di, CPS2021_SCP_PING_REG,
		CPS2021_SCP_PING_WAIT_TIME_MASK, CPS2021_SCP_PING_WAIT_TIME_SHIFT, 1);

	/* enable ENADC */
	ret += cps2021_write_mask(di, CPS2021_ADCCTRL_REG,
		CPS2021_ADCCTRL_ENADC_MASK, CPS2021_ADCCTRL_ENADC_SHIFT, 1);

	/* enable automatic VBUS_PD */
	ret += cps2021_write_mask(di, CPS2021_PULLDOWN_REG,
		CPS2021_PULLDOWN_EN_VBUS_PD_MASK, CPS2021_PULLDOWN_EN_VBUS_PD_SHIFT, 0);

	/* disable vbat&ibat regulation function */
	ret += cps2021_write_mask(di, CPS2021_REGULATION_REG,
		CPS2021_REGULATION_EN_IBATREG_MASK, CPS2021_REGULATION_EN_IBATREG_SHIFT, 0);
	ret += cps2021_write_mask(di, CPS2021_REGULATION_REG,
		CPS2021_REGULATION_EN_VBATREG_MASK, CPS2021_REGULATION_EN_VBATREG_SHIFT, 0);

	/* scp interrupt mask init */
	ret += cps2021_write_byte(di, CPS2021_SCP_MASK1_REG, 0xFF);
	ret += cps2021_write_byte(di, CPS2021_SCP_MASK2_REG, 0xFF);

	if (ret) {
		hwlog_err("reg_init fail\n");
		return -EIO;
	}

	return 0;
}

static int cps2021_enable_adc(int enable, void *dev_data)
{
	int ret;
	u8 reg = 0;
	u8 value = enable ? 0x1 : 0x0;
	struct cps2021_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	ret = cps2021_write_mask(di, CPS2021_ADCCTRL_REG, CPS2021_ADCCTRL_ENADC_MASK,
		CPS2021_ADCCTRL_ENADC_SHIFT, value);
	if (ret)
		return -EIO;

	ret = cps2021_read_byte(di, CPS2021_ADCCTRL_REG, &reg);
	if (ret)
		return -EIO;

	hwlog_info("adc_enable[%x]=0x%x\n", CPS2021_ADCCTRL_REG, reg);
	return 0;
}

static int cps2021_charge_init(void *dev_data)
{
	struct cps2021_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	if (cps2021_reg_init(di))
		return -EPERM;

	di->init_finish_flag = CPS2021_INIT_FINISH;
	return 0;
}

static int cps2021_reg_reset_and_init(void *dev_data)
{
	int ret;
	struct cps2021_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	ret = cps2021_reg_reset(di);
	ret += cps2021_reg_init(di);

	return ret;
}

static int cps2021_charge_exit(void *dev_data)
{
	int ret;
	struct cps2021_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	ret = cps2021_sc_charge_enable(CPS2021_SWITCHCAP_DISABLE, di);
	di->init_finish_flag = CPS2021_NOT_INIT;
	di->int_notify_enable_flag = CPS2021_DISABLE_INT_NOTIFY;

	return ret;
}

static int cps2021_batinfo_exit(void *dev_data)
{
	return 0;
}

static int cps2021_batinfo_init(void *dev_data)
{
	struct cps2021_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	return cps2021_chip_init(di);
}

int cps2021_is_support_fcp(void *dev_data)
{
	struct cps2021_device_info *di = dev_data;

	if (di->param_dts.fcp_support != 0) {
		hwlog_info("support fcp charge\n");
		return 0;
	}
	return 1;
}

static int cps2021_register_head(char *buffer, int size, void *dev_data)
{
	int i, tmp;
	int len = 0;
	char buff[BUF_LEN] = {0};
	const char *half_head = "dev      mode   Ibus   Vbus   Ibat   Vbat   Temp";
	struct cps2021_device_info *di = dev_data;

	if (!di || !buffer)
		return -ENODEV;

	snprintf(buffer, size, half_head);
	len += strlen(half_head);

	memset(buff, 0, sizeof(buff));
	for (i = 0; i < CPS2021_PAGE0_NUM; i++) {
		tmp = CPS2021_PAGE0_BASE + i;
		if ((tmp == CPS2021_FLAG1_REG) || (tmp == CPS2021_FLAG2_REG) ||
			(tmp == CPS2021_FLAG3_REG))
			continue;
		snprintf(buff, sizeof(buff), "R[0x%3x] ", tmp);
		len += strlen(buff);
		if (len < size)
			strncat(buffer, buff, strlen(buff));
	}

	memset(buff, 0, sizeof(buff));
	for (i = 0; i < CPS2021_PAGE1_NUM; i++) {
		tmp = CPS2021_PAGE1_BASE + i;
		if ((tmp == CPS2021_SCP_ISR1_REG) || (tmp == CPS2021_SCP_ISR2_REG))
			continue;
		snprintf(buff, sizeof(buff), "R[0x%3x] ", tmp);
		len += strlen(buff);
		if (len < size)
			strncat(buffer, buff, strlen(buff));
	}

	return 0;
}

static int cps2021_db_value_dump(struct cps2021_device_info *di,
	char *buffer, int size)
{
	char buff[BUF_LEN] = {0};
	int len = 0;
	u8 reg = 0;
	struct cps2021_dump_value dv = { 0 };

	(void)cps2021_get_ibus_ma(&dv.ibus, di);
	(void)cps2021_get_vbus_mv(&dv.vbus, di);
	(void)cps2021_get_ibat_ma(&dv.ibat, di);
	dv.vbat = cps2021_get_vbat_mv(di);
	(void)cps2021_get_device_temp(&dv.temp, di);
	(void)cps2021_read_byte(di, CPS2021_CONTROL1_REG, &reg);

	snprintf(buff, sizeof(buff), "%s  ", di->name);
	len += strlen(buff);
	if (len < size)
		strncat(buffer, buff, strlen(buff));

	if (((reg & CPS2021_CONTROL1_OPMODE_MASK) >> CPS2021_CONTROL1_OPMODE_SHIFT) ==
		CPS2021_CONTROL1_FBYPASSMODE)
		snprintf(buff, sizeof(buff), "%s", "LVC    ");
	else if (((reg & CPS2021_CONTROL1_OPMODE_MASK) >> CPS2021_CONTROL1_OPMODE_SHIFT) ==
		CPS2021_CONTROL1_FCHGPUMPMODE)
		snprintf(buff, sizeof(buff), "%s", "SC     ");
	else
		snprintf(buff, sizeof(buff), "%s", "BUCK   ");

	len += strlen(buff);
	if (len < size)
		strncat(buffer, buff, strlen(buff));

	len += snprintf(buff, sizeof(buff), "%-7d%-7d%-7d%-7d%-7d",
		dv.ibus, dv.vbus, dv.ibat, dv.vbat, dv.temp);
	strncat(buffer, buff, strlen(buff));

	return len;
}

static int cps2021_dump_reg(char *buffer, int size, void *dev_data)
{
	int i, len, tmp;
	u8 reg_val = 0;
	char buff[BUF_LEN] = {0};
	struct cps2021_device_info *di = dev_data;

	if (!di || !buffer)
		return -ENODEV;

	len = snprintf(buffer, size, "%s ", di->name);
	len += cps2021_db_value_dump(di, buff, BUF_LEN);
	if (len < size)
		strncat(buffer, buff, strlen(buff));

	for (i = 0; i < CPS2021_PAGE0_NUM; i++) {
		tmp = CPS2021_PAGE0_BASE + i;
		if ((tmp == CPS2021_FLAG1_REG) || (tmp == CPS2021_FLAG2_REG) ||
			(tmp == CPS2021_FLAG3_REG))
			continue;
		cps2021_read_byte(di, tmp, &reg_val);
		snprintf(buff, sizeof(buff), "0x%-7x", reg_val);
		len += strlen(buff);
		if (len < size)
			strncat(buffer, buff, strlen(buff));
	}
	memset(buff, 0, sizeof(buff));
	for (i = 0; i < CPS2021_PAGE1_NUM; i++) {
		tmp = CPS2021_PAGE1_BASE + i;
		if ((tmp == CPS2021_SCP_ISR1_REG) || (tmp == CPS2021_SCP_ISR2_REG))
			continue;
		cps2021_read_byte(di, tmp, &reg_val);
		snprintf(buff, sizeof(buff), "0x%-7x", reg_val);
		len += strlen(buff);
		if (len < size)
			strncat(buffer, buff, strlen(buff));
	}

	return 0;
}

static struct dc_ic_ops cps2021_lvc_ops = {
	.dev_name = "cps2021",
	.ic_init = cps2021_charge_init,
	.ic_exit = cps2021_charge_exit,
	.ic_enable = cps2021_lvc_charge_enable,
	.ic_discharge = cps2021_discharge,
	.is_ic_close = cps2021_is_device_close,
	.get_ic_id = cps2021_get_device_id,
	.config_ic_watchdog = cps2021_config_watchdog_ms,
	.kick_ic_watchdog = cps2021_kick_watchdog_ms,
	.ic_reg_reset_and_init = cps2021_reg_reset_and_init,
};

static struct dc_ic_ops cps2021_sc_ops = {
	.dev_name = "cps2021",
	.ic_init = cps2021_charge_init,
	.ic_exit = cps2021_charge_exit,
	.ic_enable = cps2021_sc_charge_enable,
	.ic_discharge = cps2021_discharge,
	.is_ic_close = cps2021_is_device_close,
	.get_ic_id = cps2021_get_device_id,
	.config_ic_watchdog = cps2021_config_watchdog_ms,
	.kick_ic_watchdog = cps2021_kick_watchdog_ms,
	.ic_reg_reset_and_init = cps2021_reg_reset_and_init,
};

static struct dc_batinfo_ops cps2021_batinfo_ops = {
	.init = cps2021_batinfo_init,
	.exit = cps2021_batinfo_exit,
	.get_bat_btb_voltage = cps2021_get_vbat_mv,
	.get_bat_package_voltage = cps2021_get_vbat_mv,
	.get_vbus_voltage = cps2021_get_vbus_mv,
	.get_bat_current = cps2021_get_ibat_ma,
	.get_ic_ibus = cps2021_get_ibus_ma,
	.get_ic_temp = cps2021_get_device_temp,
};

static struct power_log_ops cps2021_log_ops = {
	.dev_name = "cps2021",
	.dump_log_head = cps2021_register_head,
	.dump_log_content = cps2021_dump_reg,
};

static void cps2021_init_ops_dev_data(struct cps2021_device_info *di)
{
	memcpy(&di->lvc_ops, &cps2021_lvc_ops, sizeof(struct dc_ic_ops));
	di->lvc_ops.dev_data = (void *)di;
	memcpy(&di->sc_ops, &cps2021_sc_ops, sizeof(struct dc_ic_ops));
	di->sc_ops.dev_data = (void *)di;
	memcpy(&di->batinfo_ops, &cps2021_batinfo_ops, sizeof(struct dc_batinfo_ops));
	di->batinfo_ops.dev_data = (void *)di;
	memcpy(&di->log_ops, &cps2021_log_ops, sizeof(struct power_log_ops));
	di->log_ops.dev_data = (void *)di;

	if (!di->ic_role) {
		snprintf(di->name, CHIP_DEV_NAME_LEN, "cps2021");
	} else {
		snprintf(di->name, CHIP_DEV_NAME_LEN, "cps2021_%d", di->ic_role);
		di->lvc_ops.dev_name = di->name;
		di->sc_ops.dev_name = di->name;
		di->log_ops.dev_name = di->name;
	}
}

static void cps2021_ops_register(struct cps2021_device_info *di)
{
	int ret;

	cps2021_init_ops_dev_data(di);

	ret = dc_ic_ops_register(LVC_MODE, di->ic_role, &di->lvc_ops);
	ret += dc_ic_ops_register(SC_MODE, di->ic_role, &di->sc_ops);
	ret += dc_batinfo_ops_register(di->ic_role, &di->batinfo_ops, di->device_id);
	if (ret)
		hwlog_err("sysinfo ops register fail\n");

	ret = cps2021_protocol_ops_register(di);
	if (ret)
		hwlog_err("scp or fcp ops register fail\n");

	ret = cps2021_ufcs_ops_register(di);
	if (ret)
		hwlog_err("ufcs ops register fail\n");

	power_log_ops_register(&di->log_ops);
}

static void cps2021_interrupt_handle(struct cps2021_device_info *di,
	struct nty_data *data)
{
	int val = 0;
	u8 flag0 = data->event1;
	u8 flag1 = data->event2;

	if (flag0 & CPS2021_FLAG1_VBUSCON_OVP_FLAG_MASK) {
		hwlog_info("AC OVP happened\n");
		power_event_anc_notify(POWER_ANT_SC_FAULT,
			POWER_NE_DC_FAULT_AC_OVP, data);
	}
	if (flag1 & CPS2021_FLAG2_VBATOVP_FLAG_MASK) {
		hwlog_info("BAT OVP happened\n");
		val = cps2021_get_vbat_mv(di);
		if (val >= CPS2021_VBAT_OVP_REF_INIT) {
			hwlog_info("BAT OVP happened [%d]\n", val);
			power_event_anc_notify(POWER_ANT_SC_FAULT,
				POWER_NE_DC_FAULT_VBAT_OVP, data);
		}
	}
	if (flag1 & CPS2021_FLAG2_IBATOCP_FLAG_MASK) {
		hwlog_info("BAT OCP happened\n");
		cps2021_get_ibat_ma(&val, di);
		if (val >= CPS2021_IBAT_OCP_REF_INIT) {
			hwlog_info("BAT OCP happened [%d]\n", val);
			power_event_anc_notify(POWER_ANT_SC_FAULT,
				POWER_NE_DC_FAULT_IBAT_OCP, data);
		}
	}
	if (flag0 & CPS2021_FLAG1_VBUSOVP_FLAG_MASK) {
		hwlog_info("BUS OVP happened\n");
		cps2021_get_vbus_mv(&val, di);
		if (val >= CPS2021_VBUS_OVP_REF_INIT) {
			hwlog_info("BUS OVP happened [%d]\n", val);
			power_event_anc_notify(POWER_ANT_SC_FAULT,
				POWER_NE_DC_FAULT_VBUS_OVP, data);
		}
	}
	if (flag0 & CPS2021_FLAG1_IBUSOCP_FLAG_MASK) {
		hwlog_info("BUS OCP happened\n");
		cps2021_get_ibus_ma(&val, di);
		if (val >= CPS2021_CP_IBUS_OCP_REF_INIT) {
			hwlog_info("BUS OCP happened [%d]\n", val);
			power_event_anc_notify(POWER_ANT_SC_FAULT,
				POWER_NE_DC_FAULT_IBUS_OCP, data);
		}
	}
	if (flag1 & CPS2021_FLAG2_TSD_FLAG_MASK)
		hwlog_info("DIE TEMP OTP happened\n");
}

static void cps2021_charge_work(struct work_struct *work)
{
	u8 flag[5] = {0}; /* read 5 bytes */
	struct cps2021_device_info *di = NULL;
	struct nty_data *data = NULL;

	if (!work)
		return;

	di = container_of(work, struct cps2021_device_info, charge_work);
	if (!di || !di->client)
		return;

	/* to confirm the interrupt */
	(void)cps2021_read_byte(di, CPS2021_FLAG1_REG, &flag[0]);
	(void)cps2021_read_byte(di, CPS2021_FLAG2_REG, &flag[1]);
	(void)cps2021_read_byte(di, CPS2021_FLAG3_REG, &flag[2]);
	/* to confirm the scp interrupt */
	(void)cps2021_read_byte(di, CPS2021_SCP_ISR1_REG, &flag[3]);
	(void)cps2021_read_byte(di, CPS2021_SCP_ISR2_REG, &flag[4]);

	di->scp_isr_backup[0] |= flag[3];
	di->scp_isr_backup[1] |= flag[4];

	data = &(di->nty_data);
	data->event1 = flag[0];
	data->event2 = flag[1];
	data->event3 = flag[2];
	data->addr = di->client->addr;

	if (di->int_notify_enable_flag == CPS2021_ENABLE_INT_NOTIFY) {
		cps2021_interrupt_handle(di, data);
		cps2021_dump_register(di);
	}

	hwlog_info("FLAG1[%x]=0x%x, FLAG2[%x]=0x%x, FLAG3[%x]=0x%x\n",
		CPS2021_FLAG1_REG, flag[0], CPS2021_FLAG2_REG, flag[1], CPS2021_FLAG3_REG, flag[2]);
	hwlog_info("ISR1[%x]=0x%x, ISR2[%x]=0x%x\n",
		CPS2021_SCP_ISR1_REG, flag[3], CPS2021_SCP_ISR2_REG, flag[4]);
}

static void cps2021_interrupt_work(struct work_struct *work)
{
	struct cps2021_device_info *di = NULL;

	if (!work)
		return;

	di = container_of(work, struct cps2021_device_info, irq_work);
	if (!di || !di->client)
		return;

	schedule_work(&di->ufcs_work);
	schedule_work(&di->charge_work);

	/* clear irq */
	enable_irq(di->irq_int);
}

static irqreturn_t cps2021_interrupt(int irq, void *_di)
{
	struct cps2021_device_info *di = _di;

	if (!di) {
		hwlog_err("di is null\n");
		return IRQ_HANDLED;
	}

	if (di->init_finish_flag == CPS2021_INIT_FINISH)
		di->int_notify_enable_flag = CPS2021_ENABLE_INT_NOTIFY;

	hwlog_info("int happened\n");

	disable_irq_nosync(di->irq_int);
	schedule_work(&di->irq_work);

	return IRQ_HANDLED;
}

static int cps2021_irq_init(struct device_node *np, struct cps2021_device_info *di)
{
	int ret;

	ret = power_gpio_config_interrupt(np, "intr_gpio", "cps2021_gpio_int",
		&di->gpio_int, &di->irq_int);
	if (ret)
		return ret;

	ret = request_irq(di->irq_int, cps2021_interrupt,
		IRQF_TRIGGER_FALLING, "cps2021_int_irq", di);
	if (ret) {
		hwlog_err("gpio irq request fail\n");
		di->irq_int = -1;
		gpio_free(di->gpio_int);
		return ret;
	}

	enable_irq_wake(di->irq_int);
	INIT_WORK(&di->irq_work, cps2021_interrupt_work);
	INIT_WORK(&di->charge_work, cps2021_charge_work);
	INIT_WORK(&di->ufcs_work, cps2021_ufcs_work);
	return 0;
}

static void cps2021_parse_dts(struct device_node *np, struct cps2021_device_info *di)
{
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"switching_frequency", &di->switching_frequency,
		CPS2021_SW_FREQ_550KHZ);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "scp_support",
		(u32 *)&(di->param_dts.scp_support), 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "fcp_support",
		(u32 *)&(di->param_dts.fcp_support), 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "ufcs_support",
		(u32 *)&(di->param_dts.ufcs_support), 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "ic_role",
		(u32 *)&(di->ic_role), IC_ONE);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"sense_r_config", &di->sense_r_config, SENSE_R_5_MOHM);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"sense_r_actual", &di->sense_r_actual, SENSE_R_5_MOHM);
}

static void cps2021_init_lock_mutex(struct cps2021_device_info *di)
{
	mutex_init(&di->scp_detect_lock);
	mutex_init(&di->accp_adapter_reg_lock);
	mutex_init(&di->ufcs_detect_lock);
	mutex_init(&di->ufcs_node_lock);
}

static void cps2021_destroy_lock_mutex(struct cps2021_device_info *di)
{
	mutex_destroy(&di->scp_detect_lock);
	mutex_destroy(&di->accp_adapter_reg_lock);
	mutex_destroy(&di->ufcs_detect_lock);
	mutex_destroy(&di->ufcs_node_lock);
}

static int cps2021_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	struct cps2021_device_info *di = NULL;
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

	ret = cps2021_get_device_id(di);
	if ((ret < 0) || (ret == DC_DEVICE_ID_END))
		goto cps2021_fail_0;

	cps2021_parse_dts(np, di);
	cps2021_init_lock_mutex(di);
	(void)power_pinctrl_config(di->dev, "pinctrl-names", 1); /* 1:pinctrl-names length */

	ret = cps2021_reg_reset(di);
	if (ret)
		goto cps2021_fail_1;

	ret = cps2021_reg_init(di);
	if (ret)
		goto cps2021_fail_1;

	ret = cps2021_irq_init(np, di);
	if (ret)
		goto cps2021_fail_2;

	cps2021_ufcs_init_msg_head(di);
	cps2021_ops_register(di);
	i2c_set_clientdata(client, di);
	return 0;

cps2021_fail_2:
	free_irq(di->irq_int, di);
	gpio_free(di->gpio_int);
cps2021_fail_1:
	cps2021_destroy_lock_mutex(di);
cps2021_fail_0:
	di->chip_already_init = 0;
	devm_kfree(&client->dev, di);

	return ret;
}

static int cps2021_remove(struct i2c_client *client)
{
	struct cps2021_device_info *di = i2c_get_clientdata(client);

	if (!di)
		return -ENODEV;

	cps2021_reg_reset(di);

	if (di->irq_int)
		free_irq(di->irq_int, di);
	if (di->gpio_int)
		gpio_free(di->gpio_int);

	cps2021_ufcs_free_node_list(di);
	cps2021_destroy_lock_mutex(di);

	return 0;
}

static void cps2021_shutdown(struct i2c_client *client)
{
	struct cps2021_device_info *di = i2c_get_clientdata(client);

	if (!di)
		return;

	cps2021_reg_reset(di);
}

#ifdef CONFIG_PM
static int cps2021_i2c_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cps2021_device_info *di = NULL;

	if (!client)
		return 0;

	di = i2c_get_clientdata(client);
	if (di)
		cps2021_enable_adc(0, (void *)di);

	return 0;
}

static int cps2021_i2c_resume(struct device *dev)
{
	return 0;
}

static void cps2021_i2c_complete(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cps2021_device_info *di = NULL;

	if (!client)
		return;

	di = i2c_get_clientdata(client);
	if (!di)
		return;

	cps2021_enable_adc(1, (void *)di);
}

static const struct dev_pm_ops cps2021_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(cps2021_i2c_suspend, cps2021_i2c_resume)
	.complete = cps2021_i2c_complete,
};
#define CPS2021_PM_OPS (&cps2021_pm_ops)
#else
#define CPS2021_PM_OPS (NULL)
#endif /* CONFIG_PM */

MODULE_DEVICE_TABLE(i2c, cps2021);
static const struct of_device_id cps2021_of_match[] = {
	{
		.compatible = "cps2021",
		.data = NULL,
	},
	{},
};

static const struct i2c_device_id cps2021_i2c_id[] = {
	{ "cps2021", 0 }, {}
};

static struct i2c_driver cps2021_driver = {
	.probe = cps2021_probe,
	.remove = cps2021_remove,
	.shutdown = cps2021_shutdown,
	.id_table = cps2021_i2c_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "cps2021",
		.of_match_table = of_match_ptr(cps2021_of_match),
		.pm = CPS2021_PM_OPS,
	},
};

static int __init cps2021_init(void)
{
	return i2c_add_driver(&cps2021_driver);
}

static void __exit cps2021_exit(void)
{
	i2c_del_driver(&cps2021_driver);
}

module_init(cps2021_init);
module_exit(cps2021_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("cps2021 module driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
