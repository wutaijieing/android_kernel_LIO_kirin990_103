// SPDX-License-Identifier: GPL-2.0
/*
 * bq25970.c
 *
 * bq25970 driver
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

#include "bq25970.h"
#include <huawei_platform/hwpower/common_module/power_platform.h>
#include <chipset_common/hwpower/common_module/power_algorithm.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <chipset_common/hwpower/common_module/power_pinctrl.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_i2c.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_device_id.h>

#define HWLOG_TAG bq25970
HWLOG_REGIST();

static int bq25970_write_byte(struct bq25970_device_info *di, u8 reg, u8 value)
{
	if (!di || (di->chip_already_init == 0)) {
		hwlog_err("chip not init\n");
		return -EIO;
	}

	return power_i2c_u8_write_byte(di->client, reg, value);
}

static int bq25970_read_byte(struct bq25970_device_info *di, u8 reg, u8 *value)
{
	if (!di || (di->chip_already_init == 0)) {
		hwlog_err("chip not init\n");
		return -EIO;
	}

	return power_i2c_u8_read_byte(di->client, reg, value);
}

static int bq25970_read_word(struct bq25970_device_info *di, u8 reg, u16 *value)
{
	u8 hi_data = 0;
	u8 lo_data = 0;

	if (!di || (di->chip_already_init == 0)) {
		hwlog_err("chip not init\n");
		return -EIO;
	}

	if (di->device_id == SWITCHCAP_NU2105) {
		if (power_i2c_u8_read_byte(di->client, reg, &hi_data))
			return -EPERM;

		if (power_i2c_u8_read_byte(di->client, reg + 1, &lo_data))
			return -EPERM;

		*value = (hi_data << BQ2597X_LENTH_OF_BYTE) | lo_data;
	} else {
		if (power_i2c_u8_read_word(di->client, reg, value, true))
			return -EPERM;
	}

	return 0;
}

static int bq25970_write_mask(struct bq25970_device_info *di,
	u8 reg, u8 mask, u8 shift, u8 value)
{
	int ret;
	u8 val = 0;

	ret = bq25970_read_byte(di, reg, &val);
	if (ret < 0)
		return ret;

	val &= ~mask;
	val |= ((value << shift) & mask);

	return bq25970_write_byte(di, reg, val);
}

static void bq25970_dump_register(struct bq25970_device_info *di)
{
	u8 i;
	int ret;
	u8 val = 0;

	if (!di)
		return;

	for (i = 0; i <= BQ2597X_DEGLITCH_REG; ++i) {
		ret = bq25970_read_byte(di, i, &val);
		if (ret)
			hwlog_err("dump_register read fail\n");

		hwlog_info("reg [%x]=0x%x\n", i, val);
	}
}

static int bq25970_reg_reset(struct bq25970_device_info *di)
{
	int ret;
	u8 reg = 0;

	ret = bq25970_write_mask(di, BQ2597X_CONTROL_REG,
		BQ2597X_REG_RST_MASK, BQ2597X_REG_RST_SHIFT,
		BQ2597X_REG_RST_ENABLE);
	if (ret)
		return -EPERM;

	ret = bq25970_read_byte(di, BQ2597X_CONTROL_REG, &reg);
	if (ret)
		return -EPERM;

	hwlog_info("reg_reset [%x]=0x%x\n", BQ2597X_CONTROL_REG, reg);
	return 0;
}

static int bq25970_fault_clear(struct bq25970_device_info *di)
{
	int ret;
	u8 reg = 0;

	ret = bq25970_read_byte(di, BQ2597X_FLT_FLAG_REG, &reg);
	if (ret)
		return -EPERM;

	hwlog_info("fault_flag [%x]=0x%x\n", BQ2597X_FLT_FLAG_REG, reg);
	return 0;
}

static int bq25970_charge_enable(int enable, void *dev_data)
{
	int ret;
	u8 reg = 0;
	u8 value = enable ? 0x1 : 0x0;
	struct bq25970_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	ret = bq25970_write_mask(di, BQ2597X_CHRG_CTL_REG,
		BQ2597X_CHARGE_EN_MASK, BQ2597X_CHARGE_EN_SHIFT,
		value);
	if (ret)
		return -EPERM;

	ret = bq25970_read_byte(di, BQ2597X_CHRG_CTL_REG, &reg);
	if (ret)
		return -EPERM;

	hwlog_info("charge_enable [%x]=0x%x\n", BQ2597X_CHRG_CTL_REG, reg);

	if ((reg & BQ2597X_CHARGE_EN_MASK) !=
		((value << BQ2597X_CHARGE_EN_SHIFT) & BQ2597X_CHARGE_EN_MASK)) {
		hwlog_info("bq25970_charge_enable fail\n");
		bq25970_dump_register(di);
	}
	return 0;
}

static int bq25970_enable_adc(int enable, void *dev_data)
{
	int ret;
	u8 reg = 0;
	u8 value = enable ? 0x1 : 0x0;
	struct bq25970_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	ret = bq25970_write_mask(di, BQ2597X_ADC_CTRL_REG,
		BQ2597X_ADC_CTRL_EN_MASK, BQ2597X_ADC_CTRL_EN_SHIFT,
		value);
	if (ret)
		return -EPERM;

	ret = bq25970_read_byte(di, BQ2597X_ADC_CTRL_REG, &reg);
	if (ret)
		return -EPERM;

	hwlog_info("adc_enable [%x]=0x%x\n", BQ2597X_ADC_CTRL_REG, reg);
	return 0;
}

static bool bq25970_is_adc_disabled(struct bq25970_device_info *di)
{
	u8 reg = 0;
	int ret;

	ret = bq25970_read_byte(di, BQ2597X_ADC_CTRL_REG, &reg);
	if (ret || !(reg & BQ2597X_ADC_CTRL_EN_MASK))
		return true;

	return false;
}

static int bq25970_discharge(int enable, void *dev_data)
{
	int ret;
	u8 reg = 0;
	u8 value = enable ? 0x1 : 0x0;
	struct bq25970_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	if ((di->device_id == SWITCHCAP_HL1530) ||
		(di->device_id == SWITCHCAP_SYH69637))
		return 0;

	ret = bq25970_write_mask(di, BQ2597X_CONTROL_REG,
		BQ2597X_VBUS_PD_EN_MASK, BQ2597X_VBUS_PD_EN_SHIFT,
		value);
	if (ret)
		return -EPERM;

	ret = bq25970_read_byte(di, BQ2597X_CONTROL_REG, &reg);
	if (ret)
		return -EPERM;

	hwlog_info("discharge [%x]=0x%x\n", BQ2597X_CONTROL_REG, reg);
	return 0;
}

static int bq25970_is_device_close(void *dev_data)
{
	u8 reg = 0;
	int ret;
	struct bq25970_device_info *di = dev_data;

	if (!di)
		return 1;

	ret = bq25970_read_byte(di, BQ2597X_CONVERTER_STATE_REG, &reg);
	if (ret)
		return 1;

	if (reg & BQ2597X_WD_TIMEOUT_FLAG_MASK)
		return 0;

	return 1;
}

static int bq25970_get_device_id(void *dev_data)
{
	u8 part_info = 0;
	int ret;
	struct bq25970_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	if (di->get_id_time == BQ2597X_USED)
		return di->device_id;

	di->get_id_time = BQ2597X_USED;
	ret = bq25970_read_byte(di, BQ2597X_PART_INFO_REG, &part_info);
	if (ret && di->i2c_recovery) {
		hwlog_err("i2c read fail, try to read other address\n");
		di->client->addr = di->i2c_recovery_addr;
		ret = bq25970_read_byte(di, BQ2597X_PART_INFO_REG, &part_info);
	}
	if (ret) {
		di->get_id_time = BQ2597X_NOT_USED;
		hwlog_err("get_device_id read fail\n");
		return -EPERM;
	}
	hwlog_info("get_device_id [%x]=0x%x\n",
		BQ2597X_PART_INFO_REG, part_info);

	part_info = part_info & BQ2597X_DEVICE_ID_MASK;
	switch (part_info) {
	case BQ2597X_DEVICE_ID_BQ25970:
		di->device_id = SWITCHCAP_BQ25970;
		break;
	case BQ2597X_DEVICE_ID_SC8551:
		di->device_id = SWITCHCAP_SC8551;
		break;
	case BQ2597X_DEVICE_ID_SC8551A:
		di->device_id = SWITCHCAP_SC8551A;
		break;
	case BQ2597X_DEVICE_ID_HL1530:
		di->device_id = SWITCHCAP_HL1530;
		break;
	case BQ2597X_DEVICE_ID_HL7130:
		di->device_id = SWITCHCAP_HL7130;
		break;
	case BQ2597X_DEVICE_ID_SYH69637:
		di->device_id = SWITCHCAP_SYH69637;
		break;
	case BQ2597X_DEVICE_ID_NU2105:
		di->device_id = SWITCHCAP_NU2105;
		break;
	default:
		di->device_id = DC_DEVICE_ID_END;
		hwlog_err("device id not match\n");
		break;
	}

	return di->device_id;
}

static int bq25970_get_vbat_mv(void *dev_data)
{
	u16 data = 0;
	int ret;
	int vbat;
	int vbat2;
	int result;
	struct bq25970_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	if (bq25970_is_adc_disabled(di))
		return 0;

	ret = bq25970_read_word(di, BQ2597X_VBAT_ADC1_REG, &data);
	if (ret)
		return -EPERM;

	hwlog_info("VBAT_ADC=0x%x\n", data);

	vbat = (int)(s16)data;

	ret = bq25970_read_word(di, BQ2597X_VBAT_ADC1_REG, &data);
	if (ret)
		return -EPERM;

	hwlog_info("VBAT_ADC=0x%x\n", data);

	vbat2 = (int)(s16)data;

	result = vbat < vbat2 ? vbat : vbat2;

	if ((di->device_id == SWITCHCAP_SC8551) || (di->device_id == SWITCHCAP_SC8551A))
		return result * SC8551_VBAT_ADC_STEP / SC8551_BASE_RATIO_UNIT;
	if (di->device_id == SWITCHCAP_SYH69637)
		return result * SYH69637_VBAT_ADC_STEP / SYH69637_BASE_RATIO_UNIT;

	return result;
}

static int bq25970_get_ibat_ma(int *ibat, void *dev_data)
{
	int ret;
	u16 data = 0;
	u16 val;
	int flag;
	struct bq25970_device_info *di = dev_data;

	if (!ibat || !di)
		return -EPERM;

	if (bq25970_is_adc_disabled(di)) {
		*ibat = 0;
		return 0;
	}

	ret = bq25970_read_word(di, BQ2597X_IBAT_ADC1_REG, &data);
	if (ret)
		return -EPERM;

	hwlog_info("IBAT_ADC=0x%x\n", data);

	if ((di->device_id == SWITCHCAP_SC8551) || (di->device_id == SWITCHCAP_SC8551A)) {
		*ibat = (int)(s16)data * SC8551_IBAT_ADC_STEP /
			SC8551_BASE_RATIO_UNIT * di->sense_r_config;
	} else if (di->device_id == SWITCHCAP_SYH69637) {
		flag = (data & (BQ2597X_IBATADC_SIGN_MASK << BQ2597X_LENTH_OF_BYTE)) ?
			BQ2597X_IBATADC_NEGATIVE : BQ2597X_IBATADC_POSITIVE;
		val = (int)(s16)(data & (~(BQ2597X_IBATADC_SIGN_MASK << BQ2597X_LENTH_OF_BYTE)));
		*ibat = flag * val * SYH69637_IBAT_ADC_STEP /
			SYH69637_BASE_RATIO_UNIT * di->sense_r_config;
	} else {
		*ibat = (int)(s16)data * di->sense_r_config;
	}
	*ibat /= di->sense_r_actual;

	return 0;
}

static int bq25970_get_ibus_ma(int *ibus, void *dev_data)
{
	u16 data = 0;
	int ret;
	struct bq25970_device_info *di = dev_data;

	if (!di || !ibus)
		return -EPERM;

	if (bq25970_is_adc_disabled(di)) {
		*ibus = 0;
		return 0;
	}

	ret = bq25970_read_word(di, BQ2597X_IBUS_ADC1_REG, &data);
	if (ret)
		return -EPERM;

	hwlog_info("IBUS_ADC=0x%x\n", data);

	if ((di->device_id == SWITCHCAP_SC8551) || (di->device_id == SWITCHCAP_SC8551A))
		*ibus = (int)(s16)data * SC8551_IBUS_ADC_STEP / SC8551_BASE_RATIO_UNIT;
	else if (di->device_id == SWITCHCAP_SYH69637)
		*ibus = (int)(s16)data * SYH69637_IBUS_ADC_STEP / SYH69637_BASE_RATIO_UNIT;
	else
		*ibus = (int)(s16)data;

	return 0;
}

static int bq25970_get_vbus_mv(int *vbus, void *dev_data)
{
	int ret;
	u16 data = 0;
	struct bq25970_device_info *di = dev_data;

	if (!di || !vbus)
		return -EPERM;

	if (bq25970_is_adc_disabled(di)) {
		*vbus = 0;
		return 0;
	}

	ret = bq25970_read_word(di, BQ2597X_VBUS_ADC1_REG, &data);
	if (ret)
		return -EPERM;

	hwlog_info("VBUS_ADC=0x%x\n", data);

	if ((di->device_id == SWITCHCAP_SC8551) || (di->device_id == SWITCHCAP_SC8551A))
		*vbus = (int)(s16)data * SC8551_VBUS_ADC_STEP / SC8551_BASE_RATIO_UNIT;
	else
		*vbus = (int)(s16)data;

	return 0;
}

static int bq25970_get_tsbus_percentage(struct bq25970_device_info *di,
	long *tsbus_per)
{
	int ret;
	u16 data = 0;
	u16 adc_value;

	ret = bq25970_read_word(di, BQ2597X_TSBUS_ADC1_REG, &data);
	if (ret)
		return -EPERM;

	if (bq25970_is_adc_disabled(di)) {
		*tsbus_per = 0;
		return 0;
	}

	hwlog_info("TSBUS_ADC=0x%x\n", data);

	adc_value = data & (((BQ2597X_TDIE_ADC1_SIGN_MASK | BQ2597X_TDIE_ADC1_MASK) <<
		BQ2597X_LENTH_OF_BYTE) | BQ2597X_LOW_BYTE_INIT);
	if (di->device_id == SWITCHCAP_SYH69637)
		*tsbus_per = (long)((s16)adc_value * SYH69637_TSBUS_ADC_STEP);
	else
		*tsbus_per = (long)((s16)adc_value * BQ2597X_TSBUS_ADC_STEP);

	return 0;
}

static int bq25970_get_adc_raw_data(struct bq25970_device_info *di,
	int adc_channel)
{
	int adc_value;
	struct adc_comp_data comp_data = { 0 };

	adc_value = power_platform_get_adc_sample(adc_channel);
	if (adc_value < 0)
		return -EPERM;

	comp_data.adc_accuracy = di->adc_accuracy;
	comp_data.adc_v_ref = di->adc_v_ref;
	comp_data.v_pullup = di->v_pullup;
	comp_data.r_pullup = di->r_pullup;
	comp_data.r_comp = di->r_comp;

	return power_get_adc_compensation_value(adc_value, &comp_data);
}

static int bq25970_get_raw_data(int adc_channel, long *data, void *dev_data)
{
	int ret;
	long tsbus_per = 0;
	long r_temp;
	struct bq25970_device_info *di = dev_data;

	if (!di || !data)
		return -EPERM;

	if (adc_channel) {
		*data = (long)bq25970_get_adc_raw_data(di, adc_channel);
		if (*data < 0)
			return -EPERM;

		return 0;
	}

	ret = bq25970_get_tsbus_percentage(di, &tsbus_per);
	if (ret)
		return -EPERM;

	/*
	 * Rt = 1 / ((1 / Rntc) + (1 / Rlow))
	 * Vtsbus / Vout =  Rt / (Rhigh + Rt)
	 * r_temp = (tsbus_per * tsbus_high_r_kohm) / (BQ2597X_TSBUS_PER_MAX - tsbus_per)
	 * data = (r_temp * tsbus_low_r_kohm) / (tsbus_low_r_kohm - r_temp)
	 */
	r_temp = ((BQ2597X_TSBUS_PER_MAX * di->tsbus_low_r_kohm) -
		tsbus_per * (di->tsbus_low_r_kohm + di->tsbus_high_r_kohm));
	if (r_temp <= 0) {
		hwlog_err("get tsbus ntc resistor failed\n");
		return -EPERM;
	}

	*data = ((di->tsbus_high_r_kohm * di->tsbus_low_r_kohm * tsbus_per) /
		r_temp * BQ2597X_RESISTORS_KILO);

	return 0;
}

static int bq25970_is_tsbat_disabled(void *dev_data)
{
	u8 reg = 0;
	int ret;
	struct bq25970_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	ret = bq25970_read_byte(di, BQ2597X_CHRG_CTL_REG, &reg);
	if (ret)
		return -EPERM;

	hwlog_info("is_tsbat_disabled [%x]=0x%x\n", BQ2597X_CHRG_CTL_REG, reg);

	if ((di->device_id == SWITCHCAP_HL1530) || (di->device_id == SWITCHCAP_HL7130))
		return (reg & BQ2597X_TSBAT_DIS_MASK) ? -1 : 0;
	return (reg & BQ2597X_TSBAT_DIS_MASK) ? 0 : -1;
}

static int bq25970_get_device_temp(int *temp, void *dev_data)
{
	u16 data = 0;
	int ret;
	struct bq25970_device_info *di = dev_data;

	if (!temp || !di)
		return -EPERM;

	if (bq25970_is_adc_disabled(di)) {
		*temp = 0;
		return 0;
	}

	ret = bq25970_read_word(di, BQ2597X_TDIE_ADC1_REG, &data);
	if (ret)
		return -EPERM;

	hwlog_info("TDIE_ADC=0x%x\n", data);

	if (data & (BQ2597X_TDIE_ADC1_SIGN_MASK << BQ2597X_LENTH_OF_BYTE))
		data |= BQ2597X_TDIE_ADC1_RESERVE_MASK << BQ2597X_LENTH_OF_BYTE;
	else
		data &= ((BQ2597X_TDIE_ADC1_MASK <<
			BQ2597X_LENTH_OF_BYTE) | BQ2597X_LOW_BYTE_INIT);

	*temp = (int)((s16)data / BQ2597X_TDIE_SCALE) + BQ2597X_TDIE_DEFAULT;

	return 0;
}

static int bq25970_get_vusb_mv(int *vusb, void *dev_data)
{
	int ret;
	u16 data = 0;
	struct bq25970_device_info *di = dev_data;

	if (!vusb || !di)
		return -EPERM;

	if (bq25970_is_adc_disabled(di)) {
		*vusb = 0;
		return 0;
	}

	ret = bq25970_read_word(di, BQ2597X_VAC_ADC1_REG, &data);
	if (ret)
		return -EPERM;

	hwlog_info("VAC_ADC=0x%x\n", data);

	if ((di->device_id == SWITCHCAP_SC8551) || (di->device_id == SWITCHCAP_SC8551A))
		*vusb = (int)(s16)data * SC8551_VAC_ADC_STEP;
	else
		*vusb = (int)(s16)data;

	return 0;
}

static int bq25970_get_vout_mv(int *vout, void *dev_data)
{
	int ret;
	u16 data = 0;
	struct bq25970_device_info *di = dev_data;

	if (!vout || !di)
		return -EPERM;

	if (bq25970_is_adc_disabled(di)) {
		*vout = 0;
		return 0;
	}

	ret = bq25970_read_word(di, BQ2597X_VOUT_ADC1_REG, &data);
	if (ret)
		return -EPERM;

	hwlog_info("VOUT_ADC=0x%x\n", data);

	if ((di->device_id == SWITCHCAP_SC8551) || (di->device_id == SWITCHCAP_SC8551A))
		*vout = (int)(s16)data * SC8551_VOUT_ADC_STEP / SC8551_BASE_RATIO_UNIT;
	else if (di->device_id == SWITCHCAP_SYH69637)
		*vout = (int)(s16)data * SYH69637_VOUT_ADC_STEP / SYH69637_BASE_RATIO_UNIT;
	else
		*vout = (int)(s16)data;

	return 0;
}

static int bq25970_get_register_head(char *buffer, int size, void *dev_data)
{
	struct bq25970_device_info *di = dev_data;

	if (!buffer || !di)
		return -EPERM;

	snprintf(buffer, size,
		"dev        Ibus   Vbus   Ibat   Vusb   Vout   Vbat   Temp");

	return 0;
}

static int bq25970_value_dump(char *buffer, int size, void *dev_data)
{
	int ibus = 0;
	int vbus = 0;
	int ibat = 0;
	int vusb = 0;
	int vout = 0;
	int temp = 0;
	struct bq25970_device_info *di = dev_data;

	if (!buffer || !di)
		return -EPERM;

	bq25970_get_ibus_ma(&ibus, dev_data);
	bq25970_get_vbus_mv(&vbus, dev_data);
	bq25970_get_ibat_ma(&ibat, dev_data);
	bq25970_get_vusb_mv(&vusb, dev_data);
	bq25970_get_vout_mv(&vout, dev_data);
	bq25970_get_device_temp(&temp, dev_data);

	snprintf(buffer, size,
		"%s %-7d%-7d%-7d%-7d%-7d%-7d%-7d",
		di->name, ibus, vbus, ibat, vusb, vout,
		bq25970_get_vbat_mv(dev_data), temp);

	return 0;
}

static int bq25970_config_watchdog_ms(int time, void *dev_data)
{
	u8 val;
	u8 reg;
	int ret;
	struct bq25970_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	if (time >= BQ2597X_WTD_CONFIG_TIMING_30000MS)
		val = BQ2597X_WTD_SET_30000MS;
	else if (time >= BQ2597X_WTD_CONFIG_TIMING_5000MS)
		val = BQ2597X_WTD_SET_30000MS;
	else if (time >= BQ2597X_WTD_CONFIG_TIMING_1000MS)
		val = BQ2597X_WTD_SET_30000MS;
	else
		val = BQ2597X_WTD_SET_30000MS;

	ret = bq25970_write_mask(di, BQ2597X_CONTROL_REG,
		BQ2597X_WATCHDOG_CONFIG_MASK, BQ2597X_WATCHDOG_CONFIG_SHIFT,
		val);
	if (ret)
		return -EPERM;

	ret = bq25970_read_byte(di, BQ2597X_CONTROL_REG, &reg);
	if (ret)
		return -EPERM;

	hwlog_info("config_watchdog_ms [%x]=0x%x\n", BQ2597X_CONTROL_REG, reg);

	return 0;
}

static int bq25970_kick_watchdog_ms(void *dev_data)
{
	return 0;
}

static u8 bq25970_select_vbat_ovp_threshold(struct bq25970_device_info *di,
	int ovp_threshold)
{
	if ((di->device_id == SWITCHCAP_HL1530) || (di->device_id == SWITCHCAP_HL7130)) {
		if (ovp_threshold < HL1530_BAT_OVP_BASE)
			ovp_threshold = HL1530_BAT_OVP_BASE;
		if (ovp_threshold > HL1530_BAT_OVP_MAX)
			ovp_threshold = HL1530_BAT_OVP_MAX;

		return (u8)((ovp_threshold - HL1530_BAT_OVP_BASE) /
			BQ2597X_BAT_OVP_STEP);
	} else {
		if (ovp_threshold < BQ2597X_BAT_OVP_BASE_3500MV)
			ovp_threshold = BQ2597X_BAT_OVP_BASE_3500MV;
		if (ovp_threshold > BQ2597X_BAT_OVP_MAX_5075MV)
			ovp_threshold = BQ2597X_BAT_OVP_MAX_5075MV;

		return (u8)((ovp_threshold - BQ2597X_BAT_OVP_BASE_3500MV) /
			BQ2597X_BAT_OVP_STEP);
	}
}

static int bq25970_config_vbat_ovp_threshold_mv(struct bq25970_device_info *di,
	int ovp_threshold)
{
	u8 value;
	int ret;

	value = bq25970_select_vbat_ovp_threshold(di, ovp_threshold);
	ret = bq25970_write_mask(di, BQ2597X_BAT_OVP_REG,
		BQ2597X_BAT_OVP_MASK, BQ2597X_BAT_OVP_SHIFT,
		value);
	if (ret)
		return -EPERM;

	hwlog_info("config_vbat_ovp_threshold_mv [%x]=0x%x\n",
		BQ2597X_BAT_OVP_REG, value);

	return 0;
}

static int bq25970_config_ibat_ocp_threshold_ma(struct bq25970_device_info *di,
	int ocp_threshold)
{
	u8 value;
	int ret;

	if (ocp_threshold < BQ2597X_BAT_OCP_BASE_2000MA)
		ocp_threshold = BQ2597X_BAT_OCP_BASE_2000MA;

	if (ocp_threshold > BQ2597X_BAT_OCP_MAX_14700MA)
		ocp_threshold = BQ2597X_BAT_OCP_MAX_14700MA;

	value = (u8)((ocp_threshold - BQ2597X_BAT_OCP_BASE_2000MA) /
		BQ2597X_BAT_OCP_STEP);
	ret = bq25970_write_mask(di, BQ2597X_BAT_OCP_REG,
		BQ2597X_BAT_OCP_MASK, BQ2597X_BAT_OCP_SHIFT,
		value);
	if (ret)
		return -EPERM;

	hwlog_info("config_ibat_ocp_threshold_ma [%x]=0x%x\n",
		BQ2597X_BAT_OCP_REG, value);

	return 0;
}

static u8 bq25970_select_ac_ovp_threshold(struct bq25970_device_info *di,
	int ovp_threshold)
{
	if ((di->device_id == SWITCHCAP_HL1530) ||
		(di->device_id == SWITCHCAP_SYH69637) || (di->device_id == SWITCHCAP_HL7130)) {
		if (ovp_threshold == HL1530_AC_OVP_BASE_MIN)
			ovp_threshold = BQ2597X_AC_OVP_MAX_18000MV;
		else if (ovp_threshold < BQ2597X_AC_OVP_BASE_11000MV)
			ovp_threshold = BQ2597X_AC_OVP_BASE_11000MV;
		else if (ovp_threshold > HL1530_AC_OVP_BASE_MAX)
			ovp_threshold = HL1530_AC_OVP_BASE_MAX;
	} else {
		if (ovp_threshold < BQ2597X_AC_OVP_BASE_11000MV)
			ovp_threshold = BQ2597X_AC_OVP_BASE_11000MV;
		else if (ovp_threshold > BQ2597X_AC_OVP_MAX_18000MV)
			ovp_threshold = BQ2597X_AC_OVP_MAX_18000MV;
	}

	return (u8)((ovp_threshold - BQ2597X_AC_OVP_BASE_11000MV) /
		BQ2597X_AC_OVP_STEP);
}

static int bq25970_config_ac_ovp_threshold_mv(struct bq25970_device_info *di,
	int ovp_threshold)
{
	u8 value;
	int ret;

	value = bq25970_select_ac_ovp_threshold(di, ovp_threshold);
	ret = bq25970_write_mask(di, BQ2597X_AC_OVP_REG,
		BQ2597X_AC_OVP_MASK, BQ2597X_AC_OVP_SHIFT,
		value);
	if (ret)
		return -EPERM;

	hwlog_info("config_ac_ovp_threshold_mv [%x]=0x%x\n",
		BQ2597X_AC_OVP_REG, value);

	return 0;
}

static int bq25970_config_vbus_ovp_threshold_mv(struct bq25970_device_info *di,
	int ovp_threshold)
{
	u8 value;
	int ret;

	if (ovp_threshold < BQ2597X_BUS_OVP_BASE_6000MV)
		ovp_threshold = BQ2597X_BUS_OVP_BASE_6000MV;

	if (ovp_threshold > BQ2597X_BUS_OVP_MAX_12350MV)
		ovp_threshold = BQ2597X_BUS_OVP_MAX_12350MV;

	value = (u8)((ovp_threshold - BQ2597X_BUS_OVP_BASE_6000MV) /
		BQ2597X_BUS_OVP_STEP);
	ret = bq25970_write_mask(di, BQ2597X_BUS_OVP_REG,
		BQ2597X_BUS_OVP_MASK, BQ2597X_BUS_OVP_SHIFT,
		value);
	if (ret)
		return -EPERM;

	hwlog_info("config_vbus_ovp_threshold_mv [%x]=0x%x\n",
		BQ2597X_BUS_OVP_REG, value);

	return 0;
}

static int bq25970_config_ibus_ocp_threshold_ma(struct bq25970_device_info *di,
	int ocp_threshold)
{
	u8 value;
	int ret;

	if (ocp_threshold < BQ2597X_BUS_OCP_BASE_1000MA)
		ocp_threshold = BQ2597X_BUS_OCP_BASE_1000MA;

	if (ocp_threshold > BQ2597X_BUS_OCP_MAX_4750MA)
		ocp_threshold = BQ2597X_BUS_OCP_MAX_4750MA;

	value = (u8)((ocp_threshold - BQ2597X_BUS_OCP_BASE_1000MA) /
		BQ2597X_BUS_OCP_STEP);
	ret = bq25970_write_mask(di, BQ2597X_BUS_OCP_UCP_REG,
		BQ2597X_BUS_OCP_MASK, BQ2597X_BUS_OCP_SHIFT,
		value);
	if (ret)
		return -EPERM;

	hwlog_info("config_ibus_ocp_threshold_ma [%x]=0x%x\n",
		BQ2597X_BUS_OCP_UCP_REG, value);

	return 0;
}

static void bq25970_get_sc8551_frequency(int data, int *freq, int *freq_shift)
{
	switch (data) {
	case BQ2597X_SW_FREQ_300KHZ:
		*freq = SC8551_FSW_SET_SW_FREQ_300KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_NORMAL;
		break;
	case BQ2597X_SW_FREQ_350KHZ:
		*freq = SC8551_FSW_SET_SW_FREQ_350KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_NORMAL;
		break;
	case BQ2597X_SW_FREQ_400KHZ:
		*freq = SC8551_FSW_SET_SW_FREQ_400KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_NORMAL;
		break;
	case BQ2597X_SW_FREQ_450KHZ:
		*freq = SC8551_FSW_SET_SW_FREQ_450KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_NORMAL;
		break;
	case BQ2597X_SW_FREQ_500KHZ:
		*freq = SC8551_FSW_SET_SW_FREQ_500KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_NORMAL;
		break;
	case BQ2597X_SW_FREQ_550KHZ:
		*freq = SC8551_FSW_SET_SW_FREQ_550KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_NORMAL;
		break;
	case BQ2597X_SW_FREQ_600KHZ:
		*freq = SC8551_FSW_SET_SW_FREQ_600KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_NORMAL;
		break;
	case BQ2597X_SW_FREQ_675KHZ:
		*freq = SC8551_FSW_SET_SW_FREQ_750KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_M_P10;
		break;
	case BQ2597X_SW_FREQ_750KHZ:
		*freq = SC8551_FSW_SET_SW_FREQ_750KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_NORMAL;
		break;
	case BQ2597X_SW_FREQ_825KHZ:
		*freq = SC8551_FSW_SET_SW_FREQ_750KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_P_P10;
		break;
	default:
		*freq = SC8551_FSW_SET_SW_FREQ_500KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_P_P10;
		break;
	}
}

static void bq25970_get_sc_frequency(int data, int *freq, int *freq_shift)
{
	switch (data) {
	case BQ2597X_SW_FREQ_187P5KHZ:
		*freq = BQ2597X_FSW_SET_SW_FREQ_187P5KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_NORMAL;
		break;
	case BQ2597X_SW_FREQ_250KHZ:
		*freq = BQ2597X_FSW_SET_SW_FREQ_250KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_NORMAL;
		break;
	case BQ2597X_SW_FREQ_300KHZ:
		*freq = BQ2597X_FSW_SET_SW_FREQ_300KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_NORMAL;
		break;
	case BQ2597X_SW_FREQ_375KHZ:
		*freq = BQ2597X_FSW_SET_SW_FREQ_375KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_NORMAL;
		break;
	case BQ2597X_SW_FREQ_450KHZ:
		*freq = BQ2597X_FSW_SET_SW_FREQ_500KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_M_P10;
		break;
	case BQ2597X_SW_FREQ_500KHZ:
		*freq = BQ2597X_FSW_SET_SW_FREQ_500KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_NORMAL;
		break;
	case BQ2597X_SW_FREQ_550KHZ:
		*freq = BQ2597X_FSW_SET_SW_FREQ_500KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_P_P10;
		break;
	case BQ2597X_SW_FREQ_675KHZ:
		*freq = BQ2597X_FSW_SET_SW_FREQ_750KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_M_P10;
		break;
	case BQ2597X_SW_FREQ_750KHZ:
		*freq = BQ2597X_FSW_SET_SW_FREQ_750KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_NORMAL;
		break;
	case BQ2597X_SW_FREQ_825KHZ:
		*freq = BQ2597X_FSW_SET_SW_FREQ_750KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_P_P10;
		break;
	default:
		*freq = BQ2597X_FSW_SET_SW_FREQ_500KHZ;
		*freq_shift = BQ2597X_SW_FREQ_SHIFT_P_P10;
		break;
	}
}

static int bq25970_config_switching_frequency(struct bq25970_device_info *di,
	int data)
{
	int freq = 0;
	int freq_shift = 0;
	int ret;

	if ((di->device_id == SWITCHCAP_SC8551) || (di->device_id == SWITCHCAP_SC8551A))
		bq25970_get_sc8551_frequency(data, &freq, &freq_shift);
	else
		bq25970_get_sc_frequency(data, &freq, &freq_shift);

	if (di->set_freq_flag) {
		freq = BQ2597X_FSW_SET_SW_FREQ_750KHZ;
		freq_shift = BQ2597X_SW_FREQ_SHIFT_M_P10;
	}

	ret = bq25970_write_mask(di, BQ2597X_CONTROL_REG,
		BQ2597X_FSW_SET_MASK, BQ2597X_FSW_SET_SHIFT,
		freq);
	if (ret)
		return -EPERM;

	ret = bq25970_write_mask(di, BQ2597X_CHRG_CTL_REG,
		BQ2597X_FREQ_SHIFT_MASK, BQ2597X_FREQ_SHIFT_SHIFT,
		freq_shift);
	if (ret)
		return -EPERM;

	hwlog_info("config_switching_frequency [%x]=0x%x, [%x]=0x%x\n",
		BQ2597X_CONTROL_REG, freq, BQ2597X_CHRG_CTL_REG, freq_shift);

	return 0;
}

static int bq25970_config_ibat_sns_res(struct bq25970_device_info *di, int data)
{
	int res_config;

	if (data == SENSE_R_2_MOHM)
		res_config = BQ2597X_IBAT_SNS_RES_2MOHM;
	else
		res_config = BQ2597X_IBAT_SNS_RES_5MOHM;

	return bq25970_write_mask(di, BQ2597X_PULSE_MODE_REG,
		BQ2597X_IBAT_SNS_RES_MASK, BQ2597X_IBAT_SNS_RES_SHIFT,
		res_config);
}

static int bq25970_config_ibus_low_deglitch(struct bq25970_device_info *di, int data)
{
	int ibus_config;

	if (data == IBUS_DEGLITCH_5MS)
		ibus_config = BQ2597X_IBUS_LOW_DEGLITCH_5MS;
	else
		ibus_config = BQ2597X_IBUS_LOW_DEGLITCH_10US;

	if (di->device_id == SWITCHCAP_HL7130)
		ibus_config = BQ2597X_IBUS_LOW_DEGLITCH_5MS;

	return bq25970_write_mask(di, BQ2597X_DEGLITCH_REG,
		BQ2597X_IBUS_LOW_DEGLITCH_MASK, BQ2597X_IBUS_LOW_DEGLITCH_SHIFT,
		ibus_config);
}

static int bq25970_config_ucp_rise_threshold(struct bq25970_device_info *di, int data)
{
	int ucp_config;

	if (data == UCP_RISE_500MA)
		ucp_config = BQ2597X_UCP_RISE_500MA;
	else
		ucp_config = BQ2597X_UCP_RISE_300MA;

	return bq25970_write_mask(di, BQ2597X_PULSE_MODE_REG,
		BQ2597X_UCP_RISE_MASK, BQ2597X_UCP_RISE_SHIFT,
		ucp_config);
}

static void bq25970_close_vbat_protection(struct bq25970_device_info *di)
{
	bq25970_write_mask(di, BQ2597X_BAT_OVP_REG,
		BQ2597X_BAT_OVP_DIS_MASK, BQ2597X_BAT_OVP_DIS_SHIFT, 0);
	bq25970_write_mask(di, BQ2597X_FLT_MASK_REG,
		BQ2597X_BAT_OVP_FLT_MASK_MASK, BQ2597X_BAT_OVP_FLT_MASK_SHIFT, 1);
}

static int bq25970_get_config_freq(struct bq25970_device_info *di)
{
	int i;
	int freq = di->switching_frequency;

	if ((di->device_id == SWITCHCAP_SC8551) || (di->device_id == SWITCHCAP_SC8551A))
		freq = BQ2597X_SW_FREQ_500KHZ;
	else if (di->device_id == SWITCHCAP_NU2105)
		freq = BQ2597X_SW_FREQ_450KHZ;

	for (i = 0; i < di->freq_para_size; i++) {
		if (dc_get_device_id(di->freq_para[i].sc_name) == di->device_id) {
			freq = di->freq_para[i].freq_value;
			break;
		}
	}

	return freq;
}

static int bq25970_reg_lvc_init(struct bq25970_device_info *di)
{
	int ret;

	ret = bq25970_write_byte(di, BQ2597X_CONTROL_REG,
		BQ2597X_CONTROL_REG_INIT);
	if ((di->device_id == SWITCHCAP_SC8551) || (di->device_id == SWITCHCAP_SC8551A)) {
		ret += bq25970_write_byte(di, BQ2597X_ADC_ACCU_REG,
			BQ2597X_ADC_ACCU_REG_INIT);
		ret += bq25970_write_mask(di, SC8551_CHARGE_MODE_REG,
			SC8551_CHARGE_MODE_MASK, SC8551_CHARGE_MODE_SHIFT,
			SC8551_CHARGE_MODE_LVC);
	} else if (di->device_id == SWITCHCAP_NU2105) {
		ret += bq25970_write_mask(di, BQ2597X_PULSE_MODE_REG,
			NU2105_SS_TIMEOUT_SET_MASK, NU2105_SS_TIMEOUT_SET_SHIFT,
			NU2105_SS_TIMEOUT_SET_100MS);
		ret += bq25970_write_mask(di, NU2105_CHRG_MODE_REG,
			NU2105_CHRG_MODE_MASK, NU2105_CHRG_MODE_SHIFT,
			NU2105_CHRG_MODE_REG_LVC);
	}

	ret += bq25970_write_mask(di, BQ2597X_BAT_OVP_ALM_REG,
		BQ2597X_BAT_OVP_ALM_DIS_MASK, BQ2597X_BAT_OVP_ALM_DIS_SHIFT,
		BQ2597X_ALM_DISABLE);
	ret += bq25970_write_mask(di, BQ2597X_BAT_OCP_ALM_REG,
		BQ2597X_BAT_OCP_ALM_DIS_MASK, BQ2597X_BAT_OCP_ALM_DIS_SHIFT,
		BQ2597X_ALM_DISABLE);
	ret += bq25970_write_mask(di, BQ2597X_BAT_UCP_ALM_REG,
		BQ2597X_BAT_UCP_ALM_DIS_MASK, BQ2597X_BAT_UCP_ALM_DIS_SHIFT,
		BQ2597X_ALM_DISABLE);
	ret += bq25970_write_mask(di, BQ2597X_BUS_OVP_ALM_REG,
		BQ2597X_BUS_OVP_ALM_DIS_MASK, BQ2597X_BUS_OVP_ALM_DIS_SHIFT,
		BQ2597X_ALM_DISABLE);
	ret += bq25970_write_mask(di, BQ2597X_BUS_OCP_ALM_REG,
		BQ2597X_BUS_OCP_ALM_DIS_MASK, BQ2597X_BUS_OCP_ALM_DIS_SHIFT,
		BQ2597X_ALM_DISABLE);
	ret += bq25970_config_vbat_ovp_threshold_mv(di, BQ2597X_VBAT_OVP_THRESHOLD_INIT);
	ret += bq25970_config_ibat_ocp_threshold_ma(di, BQ2597X_IBAT_OCP_THRESHOLD_INIT);
	ret += bq25970_config_ac_ovp_threshold_mv(di, BQ2597X_AC_OVP_THRESHOLD_INIT);
	ret += bq25970_config_vbus_ovp_threshold_mv(di, BQ2597X_VBUS_OVP_THRESHOLD_INIT);
	ret += bq25970_config_ibus_ocp_threshold_ma(di, BQ2597X_IBUS_OCP_THRESHOLD_INIT);
	ret += bq25970_config_switching_frequency(di, bq25970_get_config_freq(di));
	ret += bq25970_config_ibat_sns_res(di, di->sense_r_config);
	ret += bq25970_write_mask(di, BQ2597X_DEGLITCH_REG,
		BQ2597X_VBUS_LOW_DEGLITCH_MASK, BQ2597X_VBUS_LOW_DEGLITCH_SHIFT,
		BQ2597X_VBUS_LOW_DEGLITCH_10MS);
	ret += bq25970_config_ibus_low_deglitch(di, di->ibus_low_deglitch);
	ret += bq25970_config_ucp_rise_threshold(di, di->ucp_rise_threshold);

	if (di->close_vbat_protection)
		bq25970_close_vbat_protection(di);

	return ret;
}

static int bq25970_reg_init(struct bq25970_device_info *di)
{
	int ret;

	ret = bq25970_write_byte(di, BQ2597X_CONTROL_REG,
		BQ2597X_CONTROL_REG_INIT);
	if (di->device_id == SWITCHCAP_NU2105) {
		ret += bq25970_write_mask(di, BQ2597X_PULSE_MODE_REG,
			NU2105_SS_TIMEOUT_SET_MASK, NU2105_SS_TIMEOUT_SET_SHIFT,
			NU2105_SS_TIMEOUT_SET_100MS);
		ret += bq25970_write_mask(di, NU2105_CHRG_MODE_REG,
			NU2105_CHRG_MODE_MASK, NU2105_CHRG_MODE_SHIFT,
			NU2105_CHRG_MODE_REG_SC);
	}
	if ((di->device_id == SWITCHCAP_HL1530) ||
		(di->device_id == SWITCHCAP_SYH69637) || (di->device_id == SWITCHCAP_HL7130))
		ret += bq25970_write_mask(di, BQ2597X_BUS_OVP_REG,
			HL1530_BUS_PD_EN_MASK, HL1530_BUS_PD_EN_SHIFT,
			BQ2597X_ALM_DISABLE);
	if (di->device_id == SWITCHCAP_HL7130) {
		ret += bq25970_write_byte(di, HL7130_PMID_VOUT_UV_OV_REG,
			HL7130_PMID_VOUT_UV_OV_INIT);
		ret += bq25970_write_byte(di, HL7130_CONTROL2_REG,
			HL7130_CONTROL2_INIT);
		ret += bq25970_write_byte(di, HL7130_ADC_CTL1_REG,
			HL7130_ADC_CTL1_INIT);
	}

	ret += bq25970_write_byte(di, BQ2597X_CHRG_CTL_REG,
		di->chrg_ctl_reg_init);
	ret += bq25970_write_byte(di, BQ2597X_INT_MASK_REG,
		BQ2597X_INT_MASK_REG_INIT);
	ret += bq25970_write_byte(di, BQ2597X_FLT_MASK_REG,
		BQ2597X_FLT_MASK_REG_INIT);

	if (di->device_id == SWITCHCAP_HL7130) {
		ret += bq25970_write_byte(di, BQ2597X_ADC_CTRL_REG,
			HL7130_ADC_CTRL_REG_INIT);
		ret += bq25970_write_byte(di, BQ2597X_ADC_FN_DIS_REG,
			HL7130_ADC_FN_DIS_REG_INIT);
	} else {
		ret += bq25970_write_byte(di, BQ2597X_ADC_CTRL_REG,
			BQ2597X_ADC_CTRL_REG_INIT);
		ret += bq25970_write_byte(di, BQ2597X_ADC_FN_DIS_REG,
			di->adc_fn_reg_init);
	}
	ret += bq25970_write_mask(di, BQ2597X_BAT_OVP_ALM_REG,
		BQ2597X_BAT_OVP_ALM_DIS_MASK, BQ2597X_BAT_OVP_ALM_DIS_SHIFT,
		BQ2597X_ALM_DISABLE);
	ret += bq25970_write_mask(di, BQ2597X_BAT_OCP_ALM_REG,
		BQ2597X_BAT_OCP_ALM_DIS_MASK, BQ2597X_BAT_OCP_ALM_DIS_SHIFT,
		BQ2597X_ALM_DISABLE);
	ret += bq25970_write_mask(di, BQ2597X_BAT_UCP_ALM_REG,
		BQ2597X_BAT_UCP_ALM_DIS_MASK, BQ2597X_BAT_UCP_ALM_DIS_SHIFT,
		BQ2597X_ALM_DISABLE);
	ret += bq25970_write_mask(di, BQ2597X_BUS_OVP_ALM_REG,
		BQ2597X_BUS_OVP_ALM_DIS_MASK, BQ2597X_BUS_OVP_ALM_DIS_SHIFT,
		BQ2597X_ALM_DISABLE);
	ret += bq25970_write_mask(di, BQ2597X_BUS_OCP_ALM_REG,
		BQ2597X_BUS_OCP_ALM_DIS_MASK, BQ2597X_BUS_OCP_ALM_DIS_SHIFT,
		BQ2597X_ALM_DISABLE);
	ret += bq25970_config_vbat_ovp_threshold_mv(di,
		BQ2597X_VBAT_OVP_THRESHOLD_INIT);
	ret += bq25970_config_ibat_ocp_threshold_ma(di,
		BQ2597X_IBAT_OCP_THRESHOLD_INIT);
	ret += bq25970_config_ac_ovp_threshold_mv(di,
		BQ2597X_AC_OVP_THRESHOLD_INIT);
	ret += bq25970_config_vbus_ovp_threshold_mv(di,
		BQ2597X_VBUS_OVP_THRESHOLD_INIT);
	ret += bq25970_config_ibus_ocp_threshold_ma(di,
		BQ2597X_IBUS_OCP_THRESHOLD_INIT);
	ret += bq25970_config_switching_frequency(di, bq25970_get_config_freq(di));
	ret += bq25970_config_ibat_sns_res(di, di->sense_r_config);
	ret += bq25970_write_mask(di, BQ2597X_DEGLITCH_REG,
		BQ2597X_VBUS_LOW_DEGLITCH_MASK, BQ2597X_VBUS_LOW_DEGLITCH_SHIFT,
		BQ2597X_VBUS_LOW_DEGLITCH_10MS);
	ret += bq25970_config_ibus_low_deglitch(di, di->ibus_low_deglitch);
	ret += bq25970_config_ucp_rise_threshold(di, di->ucp_rise_threshold);
	if ((di->device_id == SWITCHCAP_SC8551) || (di->device_id == SWITCHCAP_SC8551A)) {
		ret += bq25970_write_byte(di, BQ2597X_ADC_ACCU_REG,
			BQ2597X_ADC_ACCU_REG_INIT);
		ret += bq25970_write_mask(di, SC8551_CHARGE_MODE_REG,
			SC8551_CHARGE_MODE_MASK, SC8551_CHARGE_MODE_SHIFT,
			SC8551_CHARGE_MODE_SC);
	}
	if (di->device_id == SWITCHCAP_SC8551A)
		ret += bq25970_write_mask(di, SC8551A_ADC_OPEN_REG,
			SC8551A_ADC_IBAT_OPEN_MASK, SC8551A_ADC_IBAT_OPEN_SHIFT, 1);
	if (di->close_vbat_protection)
		bq25970_close_vbat_protection(di);

	return ret;
}

static int bq25970_charge_lvc_init(void *dev_data)
{
	struct bq25970_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	if (bq25970_reg_lvc_init(di))
		return -EPERM;

	hwlog_info("lvc device id is %d\n", di->device_id);

	di->init_finish_flag = BQ2597X_INIT_FINISH;
	return 0;
}

static int bq25970_charge_init(void *dev_data)
{
	struct bq25970_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -EPERM;
	}

	if (bq25970_reg_init(di))
		return -EPERM;

	hwlog_info("device id is %d\n", di->device_id);

	di->init_finish_flag = BQ2597X_INIT_FINISH;
	return 0;
}

static int bq25970_irq_enable(int enable, void *dev_data)
{
	int ret;
	u8 value = enable ? 0x0 : 0x1;
	struct bq25970_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	ret = bq25970_write_mask(di, SC8551_VAC_OVP_REG,
		SC8551_VAC_OVP_MASK, SC8551_VAC_OVP_SHIFT, value);
	ret += bq25970_write_mask(di, BQ2597X_AC_OVP_REG,
		BQ2597X_AC_OVP_MASK_MASK, BQ2597X_AC_OVP_MASK_SHIFT, value);
	ret += bq25970_write_mask(di, SC8551_CHARGE_MODE_REG,
		SC8551_VDR_OVP_MASK, SC8551_VDR_OVP_SHIFT, value);
	hwlog_info("irq enable=%d\n", enable);

	return ret;
}

static int bq25970_charge_exit(void *dev_data)
{
	int ret;
	struct bq25970_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -EPERM;
	}

	ret = bq25970_charge_enable(BQ2597X_SWITCHCAP_DISABLE, dev_data);
	if (di->device_id == SWITCHCAP_NU2105)
		ret += bq25970_write_mask(di, NU2105_CHRG_MODE_REG,
			NU2105_CHRG_MODE_MASK, NU2105_CHRG_MODE_SHIFT,
			NU2105_CHRG_MODE_REG_SC);
	if ((di->device_id == SWITCHCAP_SC8551) || (di->device_id == SWITCHCAP_SC8551A))
		ret += bq25970_write_mask(di, SC8551_CHARGE_MODE_REG,
			SC8551_CHARGE_MODE_MASK, SC8551_CHARGE_MODE_SHIFT,
			SC8551_CHARGE_MODE_SC);
	di->init_finish_flag = BQ2597X_NOT_INIT;
	di->int_notify_enable_flag = BQ2597X_DISABLE_INT_NOTIFY;

	power_usleep(DT_USLEEP_10MS);

	return ret;
}

static int bq25970_batinfo_exit(void *dev_data)
{
	return 0;
}

static int bq25970_batinfo_init(void *dev_data)
{
	return 0;
}

static void bq25970_fault_event_notify(unsigned long event, void *data)
{
	power_event_anc_notify(POWER_ANT_SC_FAULT, event, data);
}

static void bq25970_fault_handle(struct bq25970_device_info *di,
	struct nty_data *data)
{
	int val = 0;
	u8 fault_flag = data->event1;
	u8 ac_protection = data->event2;
	u8 converter_state = data->event3;

	if (ac_protection & BQ2597X_AC_OVP_FLAG_MASK) {
		hwlog_info("AC OVP happened\n");
		bq25970_fault_event_notify(POWER_NE_DC_FAULT_AC_OVP, data);
	} else if (fault_flag & BQ2597X_BAT_OVP_FLT_FLAG_MASK) {
		val = bq25970_get_vbat_mv(di);
		hwlog_info("BAT OVP happened, vbat=%d mv\n", val);
		if (val >= BQ2597X_VBAT_OVP_THRESHOLD_INIT)
			bq25970_fault_event_notify(POWER_NE_DC_FAULT_VBAT_OVP, data);
	} else if (fault_flag & BQ2597X_BAT_OCP_FLT_FLAG_MASK) {
		bq25970_get_ibat_ma(&val, di);
		hwlog_info("BAT OCP happened, ibat=%d ma\n", val);
		if (val >= BQ2597X_IBAT_OCP_THRESHOLD_INIT)
			bq25970_fault_event_notify(POWER_NE_DC_FAULT_IBAT_OCP, data);
	} else if (fault_flag & BQ2597X_BUS_OVP_FLT_FLAG_MASK) {
		bq25970_get_vbus_mv(&val, di);
		hwlog_info("BUS OVP happened, vbus=%d mv\n", val);
		if (val >= BQ2597X_VBUS_OVP_THRESHOLD_INIT)
			bq25970_fault_event_notify(POWER_NE_DC_FAULT_VBUS_OVP, data);
	} else if (fault_flag & BQ2597X_BUS_OCP_FLT_FLAG_MASK) {
		bq25970_get_ibus_ma(&val, di);
		hwlog_info("BUS OCP happened, ibus=%d ma\n", val);
		if (val >= BQ2597X_IBUS_OCP_THRESHOLD_INIT)
			bq25970_fault_event_notify(POWER_NE_DC_FAULT_IBUS_OCP, data);
	} else if (fault_flag & BQ2597X_TSBAT_FLT_FLAG_MASK) {
		hwlog_info("BAT TEMP OTP happened\n");
		bq25970_fault_event_notify(POWER_NE_DC_FAULT_TSBAT_OTP, data);
	} else if (fault_flag & BQ2597X_TSBUS_FLT_FLAG_MASK) {
		hwlog_info("BUS TEMP OTP happened\n");
		bq25970_fault_event_notify(POWER_NE_DC_FAULT_TSBUS_OTP, data);
	} else if (fault_flag & BQ2597X_TDIE_ALM_FLAG_MASK) {
		hwlog_info("DIE TEMP OTP happened\n");
	}

	if (converter_state & BQ2597X_CONV_OCP_FLAG_MASK) {
		hwlog_info("CONV OCP happened\n");
		bq25970_fault_event_notify(POWER_NE_DC_FAULT_CONV_OCP, data);
	}
}

static void bq25970_interrupt_work(struct work_struct *work)
{
	struct bq25970_device_info *di = NULL;
	struct nty_data *data = NULL;
	u8 converter_state = 0;
	u8 fault_flag = 0;
	u8 ac_protection = 0;
	u8 ibus_ucp = 0;
	int ret;

	if (!work)
		return;

	di = container_of(work, struct bq25970_device_info, irq_work);
	if (!di || !di->client) {
		hwlog_err("di is null\n");
		return;
	}

	data = &(di->nty_data);

	ret = bq25970_read_byte(di, BQ2597X_AC_OVP_REG, &ac_protection);
	ret |= bq25970_read_byte(di, BQ2597X_BUS_OCP_UCP_REG, &ibus_ucp);
	ret |= bq25970_read_byte(di, BQ2597X_FLT_FLAG_REG, &fault_flag);
	ret |= bq25970_read_byte(di, BQ2597X_CONVERTER_STATE_REG,
		&converter_state);
	if (ret)
		hwlog_err("irq_work read fail\n");

	data->event1 = fault_flag;
	data->event2 = ac_protection;
	data->event3 = converter_state;
	data->addr = di->client->addr;

	if (di->int_notify_enable_flag == BQ2597X_ENABLE_INT_NOTIFY) {
		bq25970_fault_handle(di, data);
		bq25970_dump_register(di);
	}

	hwlog_info("ac_ovp_reg [%x]=0x%x\n", BQ2597X_AC_OVP_REG, ac_protection);
	hwlog_info("bus_ocp_ucp_reg [%x]=0x%x\n", BQ2597X_BUS_OCP_UCP_REG,
		ibus_ucp);
	hwlog_info("flt_flag_reg [%x]=0x%x\n", BQ2597X_FLT_FLAG_REG,
		fault_flag);
	hwlog_info("converter_state_reg [%x]=0x%x\n",
		BQ2597X_CONVERTER_STATE_REG, converter_state);

	/* clear irq */
	enable_irq(di->irq_int);
}

static irqreturn_t bq25970_interrupt(int irq, void *_di)
{
	struct bq25970_device_info *di = _di;

	if (!di) {
		hwlog_err("di is null\n");
		return IRQ_HANDLED;
	}

	if (di->chip_already_init == 0)
		hwlog_err("chip not init\n");

	if (di->init_finish_flag == BQ2597X_INIT_FINISH)
		di->int_notify_enable_flag = BQ2597X_ENABLE_INT_NOTIFY;

	hwlog_info("int happened\n");
	disable_irq_nosync(di->irq_int);
	schedule_work(&di->irq_work);
	return IRQ_HANDLED;
}

static int bq25970_irq_init(struct bq25970_device_info *di,
	struct device_node *np)
{
	int ret;

	INIT_WORK(&di->irq_work, bq25970_interrupt_work);

	ret = power_gpio_config_interrupt(np,
		"gpio_int", "bq25970_gpio_int", &di->gpio_int, &di->irq_int);
	if (ret)
		return ret;

	ret = request_irq(di->irq_int, bq25970_interrupt,
		IRQF_TRIGGER_FALLING, "bq25970_int_irq", di);
	if (ret) {
		hwlog_err("gpio irq request fail\n");
		di->irq_int = -1;
		gpio_free(di->gpio_int);
		return ret;
	}

	enable_irq_wake(di->irq_int);
	return 0;
}

static int bq25970_reg_reset_and_init(struct bq25970_device_info *di)
{
	int ret;

	ret = bq25970_reg_reset(di);
	if (ret) {
		hwlog_err("reg reset fail\n");
		return ret;
	}

	ret = bq25970_fault_clear(di);
	if (ret) {
		hwlog_err("fault clear fail\n");
		return ret;
	}

	ret = bq25970_reg_init(di);
	if (ret) {
		hwlog_err("reg init fail\n");
		return ret;
	}

	return 0;
}

static int bq25970_set_freq(int freq, void *dev_data)
{
	struct bq25970_device_info *di = dev_data;

	if (!di)
		return -EPERM;

	hwlog_info("set_freq freq=%d\n", freq);
	di->set_freq_flag = freq;
	if (freq)
		return bq25970_config_switching_frequency(di, BQ2597X_SW_FREQ_675KHZ);

	return bq25970_config_switching_frequency(di, di->switching_frequency);
}

static int bq25970_get_freq(void *dev_data)
{
	struct bq25970_device_info *di = dev_data;
	u8 value = 0;

	if (!di)
		return -EPERM;

	if (bq25970_read_byte(di, BQ2597X_CONTROL_REG, &value))
		return -EPERM;

	return (int)value;
}

static void bq25970_parse_freq_para(struct device_node *np,
	struct bq25970_device_info *di)
{
	const char *tmp_string = NULL;
	int array_len;
	int i;
	int idata = 0;
	int col;
	int row;

	array_len = power_dts_read_count_strings(power_dts_tag(HWLOG_TAG), np,
		"freq_para", BQ2597X_FREQ_PARA_MAX_NUM, BQ2597X_FREQ_PARA_TOTAL);
	if (array_len < 0)
		return;
	di->freq_para_size = array_len / BQ2597X_FREQ_PARA_TOTAL;

	for (i = 0; i < array_len; i++) {
		if (power_dts_read_string_index(power_dts_tag(HWLOG_TAG),
			np, "freq_para", i, &tmp_string)) {
			di->freq_para_size = 0;
			return;
		}

		row = i / BQ2597X_FREQ_PARA_TOTAL;
		col = i % BQ2597X_FREQ_PARA_TOTAL;
		switch (col) {
		case BQ2597X_FREQ_PARA_SC_NAME:
			strncpy(di->freq_para[row].sc_name, tmp_string, CHIP_DEV_NAME_LEN - 1);
			break;
		case BQ2597X_FREQ_PARA_VALUE:
			if (kstrtoint(tmp_string, POWER_BASE_DEC, &idata)) {
				hwlog_err("str to int fail\n");
				di->freq_para_size = 0;
				return;
			}
			di->freq_para[row].freq_value = idata;
			break;
		default:
			break;
		}
	}

	for (i = 0; i < di->freq_para_size; i++)
		hwlog_info("freq_para[%d] %s %d\n", i, di->freq_para[i].sc_name,
			di->freq_para[i].freq_value);

	return;
}

static void bq25970_parse_dts(struct device_node *np,
	struct bq25970_device_info *di)
{
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"tsbus_high_r_kohm", &di->tsbus_high_r_kohm,
		BQ2597X_RESISTORS_100KOHM);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"tsbus_low_r_kohm", &di->tsbus_low_r_kohm,
		BQ2597X_RESISTORS_100KOHM);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"switching_frequency", &di->switching_frequency,
		BQ2597X_SW_FREQ_550KHZ);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"special_freq_shift", &di->special_freq_shift, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"ic_role", &di->ic_role, IC_ONE);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"sense_r_config", &di->sense_r_config, SENSE_R_2_MOHM);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"sense_r_actual", &di->sense_r_actual, SENSE_R_2_MOHM);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"nu2105_check", &di->nu2105_check, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"chrg_ctl_reg_init", &di->chrg_ctl_reg_init,
		BQ2597X_CHRG_CTL_REG_INIT);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"adc_fn_reg_init", &di->adc_fn_reg_init,
		BQ2597X_ADC_FN_DIS_REG_INIT);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"adc_accuracy", &di->adc_accuracy, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"adc_v_ref", &di->adc_v_ref, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"v_pullup", &di->v_pullup, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"r_pullup", &di->r_pullup, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"r_comp", &di->r_comp, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"i2c_recovery", &di->i2c_recovery, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"i2c_recovery_addr", &di->i2c_recovery_addr, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"ibus_low_deglitch", &di->ibus_low_deglitch, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"ucp_rise_threshold", &di->ucp_rise_threshold, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"resume_need_wait_i2c", &di->resume_need_wait_i2c, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"close_vbat_protection", &di->close_vbat_protection, 0);
	bq25970_parse_freq_para(np, di);
}

static struct dc_ic_ops bq25970_lvc_sysinfo_ops = {
	.dev_name = "bq25970",
	.ic_init = bq25970_charge_lvc_init,
	.ic_exit = bq25970_charge_exit,
	.ic_enable = bq25970_charge_enable,
	.ic_adc_enable = bq25970_enable_adc,
	.ic_discharge = bq25970_discharge,
	.is_ic_close = bq25970_is_device_close,
	.get_ic_id = bq25970_get_device_id,
	.config_ic_watchdog = bq25970_config_watchdog_ms,
	.get_ic_status = bq25970_is_tsbat_disabled,
	.set_ic_freq = bq25970_set_freq,
	.get_ic_freq = bq25970_get_freq,
};

static struct dc_ic_ops bq25970_sysinfo_ops = {
	.dev_name = "bq25970",
	.ic_init = bq25970_charge_init,
	.ic_exit = bq25970_charge_exit,
	.ic_enable = bq25970_charge_enable,
	.irq_enable = bq25970_irq_enable,
	.ic_adc_enable = bq25970_enable_adc,
	.ic_discharge = bq25970_discharge,
	.is_ic_close = bq25970_is_device_close,
	.get_ic_id = bq25970_get_device_id,
	.config_ic_watchdog = bq25970_config_watchdog_ms,
	.kick_ic_watchdog = bq25970_kick_watchdog_ms,
	.get_ic_status = bq25970_is_tsbat_disabled,
	.set_ic_freq = bq25970_set_freq,
	.get_ic_freq = bq25970_get_freq,
};

static struct dc_batinfo_ops bq25970_batinfo_ops = {
	.init = bq25970_batinfo_init,
	.exit = bq25970_batinfo_exit,
	.get_bat_btb_voltage = bq25970_get_vbat_mv,
	.get_bat_package_voltage = bq25970_get_vbat_mv,
	.get_vbus_voltage = bq25970_get_vbus_mv,
	.get_bat_current = bq25970_get_ibat_ma,
	.get_ic_ibus = bq25970_get_ibus_ma,
	.get_ic_temp = bq25970_get_device_temp,
	.get_ic_vusb = bq25970_get_vusb_mv,
	.get_ic_vout = bq25970_get_vout_mv,
};

static struct power_tz_ops bq25970_temp_sensing_ops = {
	.get_raw_data = bq25970_get_raw_data,
};

static struct power_log_ops bq25970_log_ops = {
	.dev_name = "bq25970",
	.dump_log_head = bq25970_get_register_head,
	.dump_log_content = bq25970_value_dump,
};

static void bq25970_init_ops_dev_data(struct bq25970_device_info *di)
{
	memcpy(&di->lvc_ops, &bq25970_lvc_sysinfo_ops, sizeof(struct dc_ic_ops));
	di->lvc_ops.dev_data = (void *)di;
	memcpy(&di->sc_ops, &bq25970_sysinfo_ops, sizeof(struct dc_ic_ops));
	di->sc_ops.dev_data = (void *)di;
	memcpy(&di->batinfo_ops, &bq25970_batinfo_ops, sizeof(struct dc_batinfo_ops));
	di->batinfo_ops.dev_data = (void *)di;
	memcpy(&di->tz_ops, &bq25970_temp_sensing_ops, sizeof(struct power_tz_ops));
	di->tz_ops.dev_data = (void *)di;
	memcpy(&di->log_ops, &bq25970_log_ops, sizeof(struct power_log_ops));
	di->log_ops.dev_data = (void *)di;

	if (!di->ic_role) {
		snprintf(di->name, CHIP_DEV_NAME_LEN, "bq25970");
	} else {
		snprintf(di->name, CHIP_DEV_NAME_LEN, "bq25970_%d", di->ic_role);
		di->lvc_ops.dev_name = di->name;
		di->sc_ops.dev_name = di->name;
		di->log_ops.dev_name = di->name;
	}
}

static void bq25970_ops_register(struct bq25970_device_info *di)
{
	int ret;

	bq25970_init_ops_dev_data(di);
	if ((di->device_id == SWITCHCAP_SC8551) || (di->device_id == SWITCHCAP_NU2105) ||
		(di->device_id == SWITCHCAP_SC8551A)) {
		if (dc_ic_ops_register(LVC_MODE, di->ic_role, &di->lvc_ops)) {
			hwlog_err("lvc ops register fail\n");
			return;
		}
	}

	ret = dc_ic_ops_register(SC_MODE, di->ic_role, &di->sc_ops);
	ret += dc_batinfo_ops_register(di->ic_role, &di->batinfo_ops, di->device_id);
	if (ret) {
		hwlog_err("sysinfo ops register fail\n");
		return;
	}

	ret = power_log_ops_register(&di->log_ops);
	ret += power_tz_ops_register(&di->tz_ops, "bq25970");
	if (ret)
		hwlog_err("thermalzone or power log ops register fail\n");
}

static int bq25970_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret;
	int i;
	u8 reg = 0;
	int retry_times = 3; /* retry 3 times */
	struct bq25970_device_info *di = NULL;
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
	bq25970_parse_dts(np, di);

	for (i = 0; i < retry_times; i++) {
		ret = bq25970_get_device_id(di);
		if (ret >= 0)
			break;
	}
	if ((ret < 0) || (ret == DC_DEVICE_ID_END))
		goto bq25970_fail_0;

	if (di->nu2105_check && (di->device_id == SWITCHCAP_NU2105)) {
		ret = bq25970_read_byte(di, BQ2597X_CHRG_CTL_REG, &reg);
		if (ret || ((reg & BQ2597X_MS_MASK) != 0)) {
			hwlog_err("ms check fail, reg=%x\n", BQ2597X_CHRG_CTL_REG);
			goto bq25970_fail_0;
		}
	}

	ret = bq25970_reg_reset_and_init(di);
	if (ret)
		goto bq25970_fail_0;

	(void)power_pinctrl_config(di->dev, "pinctrl-names", 1); /* 1:pinctrl-names length */
	ret = bq25970_irq_init(di, np);
	if (ret)
		goto bq25970_fail_0;

	bq25970_ops_register(di);
	i2c_set_clientdata(client, di);

	return 0;

bq25970_fail_0:
	di->chip_already_init = 0;
	devm_kfree(&client->dev, di);

	return ret;
}

static int bq25970_remove(struct i2c_client *client)
{
	struct bq25970_device_info *di = i2c_get_clientdata(client);

	if (!di)
		return -ENODEV;

	if (di->irq_int)
		free_irq(di->irq_int, di);

	if (di->gpio_int)
		gpio_free(di->gpio_int);

	return 0;
}

static void bq25970_shutdown(struct i2c_client *client)
{
	struct bq25970_device_info *di = i2c_get_clientdata(client);

	if (!di)
		return;

	if (di->irq_int)
		free_irq(di->irq_int, di);

	if (di->gpio_int)
		gpio_free(di->gpio_int);

	bq25970_reg_reset(di);
	if ((di->device_id == SWITCHCAP_SC8551) || (di->device_id == SWITCHCAP_SC8551A))
		bq25970_enable_adc(0, (void *)di);
}

#ifdef CONFIG_PM
static int bq25970_i2c_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct rt9759_device_info *di = NULL;

	if (!client)
		return 0;

	di = i2c_get_clientdata(client);
	if (di)
		bq25970_enable_adc(0, (void *)di);

	return 0;
}

static int bq25970_i2c_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bq25970_device_info *di = NULL;

	if (!client)
		return 0;

	di = i2c_get_clientdata(client);
	if (!di)
		return 0;

	if (di->resume_need_wait_i2c)
		return 0;

	bq25970_enable_adc(1, (void *)di);
	return 0;
}

static void bq25970_i2c_complete(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bq25970_device_info *di = NULL;

	if (!client)
		return;

	di = i2c_get_clientdata(client);
	if (!di)
		return;

	if (!di->resume_need_wait_i2c)
		return;

	bq25970_enable_adc(1, (void *)di);
}

static const struct dev_pm_ops bq25970_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(bq25970_i2c_suspend, bq25970_i2c_resume)
	.complete = bq25970_i2c_complete,
};
#define BQ25970_PM_OPS (&bq25970_pm_ops)
#else
#define BQ25970_PM_OPS (NULL)
#endif /* CONFIG_PM */

MODULE_DEVICE_TABLE(i2c, bq25970);
static const struct of_device_id bq25970_of_match[] = {
	{
		.compatible = "bq25970",
		.data = NULL,
	},
	{},
};

static const struct i2c_device_id bq25970_i2c_id[] = {
	{ "bq25970", 0 },
	{}
};

static struct i2c_driver bq25970_driver = {
	.probe = bq25970_probe,
	.remove = bq25970_remove,
	.shutdown = bq25970_shutdown,
	.id_table = bq25970_i2c_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "bq25970",
		.of_match_table = of_match_ptr(bq25970_of_match),
		.pm = BQ25970_PM_OPS,
	},
};

static int __init bq25970_init(void)
{
	return i2c_add_driver(&bq25970_driver);
}

static void __exit bq25970_exit(void)
{
	i2c_del_driver(&bq25970_driver);
}

module_init(bq25970_init);
module_exit(bq25970_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("bq25970 module driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
