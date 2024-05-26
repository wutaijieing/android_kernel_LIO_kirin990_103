// SPDX-License-Identifier: GPL-2.0
/*
 * aw32280.c
 *
 * aw32280 driver
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

#include "aw32280.h"
#include <linux/delay.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <chipset_common/hwpower/common_module/power_i2c.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/direct_charge/direct_charge_device_id.h>

#define HWLOG_TAG aw32280
HWLOG_REGIST();

static int aw32280_write_byte(struct aw32280_device_info *di, u16 reg, u8 value)
{
	if (!di || (di->chip_already_init == 0)) {
		hwlog_err("chip not init\n");
		return -ENODEV;
	}

	return power_i2c_u16_write_byte(di->client, reg, value);
}

static int aw32280_read_byte(struct aw32280_device_info *di, u16 reg, u8 *value)
{
	if (!di || (di->chip_already_init == 0)) {
		hwlog_err("chip not init\n");
		return -ENODEV;
	}

	return power_i2c_u16_read_byte(di->client, reg, value);
}

static int aw32280_read_word(struct aw32280_device_info *di, u16 reg, u16 *value)
{
	if (!di || (di->chip_already_init == 0)) {
		hwlog_err("chip not init\n");
		return -ENODEV;
	}

	return power_i2c_u16_read_word(di->client, reg, value, false);
}

static int aw32280_write_mask(struct aw32280_device_info *di, u16 reg,
	u8 mask, u8 shift, u8 value)
{
	int ret;
	u8 val = 0;

	ret = aw32280_read_byte(di, reg, &val);
	if (ret < 0)
		return ret;

	val &= ~mask;
	val |= ((value << shift) & mask);

	return aw32280_write_byte(di, reg, val);
}

static u16 aw32280_dump_map[] = {
	AW32280_SC_SC_EN_REG,
	AW32280_SC_SC_MODE_REG,
	AW32280_SC_PRO_TOP_CFG_4_REG,
	AW32280_SC_PRO_TOP_CFG_5_REG,
	AW32280_SC_PRO_TOP_CFG_8_REG,
	AW32280_SC_VOUT_OVP_REG,
};

static void aw32280_dump_register(struct aw32280_device_info *di)
{
	int i;
	u8 val;

	for (i = 0; i < ARRAY_SIZE(aw32280_dump_map); i++) {
		val = 0;
		if (!aw32280_read_byte(di, aw32280_dump_map[i], &val))
			hwlog_info("reg[0x%x]=0x%x", aw32280_dump_map[i], val);
	}
}

static int aw32280_reg_reset(struct aw32280_device_info *di)
{
	return gpio_direction_output(di->gpio_reset, 0);
}

static int aw32280_charger_irq_clear(struct aw32280_device_info *di)
{
	int ret;
	int i;
	u8 irq_flag = 0;
	u8 val;

	ret = aw32280_read_byte(di, AW32280_IRQ_FLAG_REG, &irq_flag);
	for (i = 0; i < AW32280_IRQ_FLAG_REG_NUM; i++) {
		val = 0;
		ret += aw32280_read_byte(di, AW32280_IRQ_FLAG_REG + 1 + i, &val);
		if (val) {
			hwlog_err("%s, irq_flag 0x%x, irq_flag0x%x = 0x%x\n",
				__func__, irq_flag, AW32280_IRQ_FLAG_REG + 1 + i, val);
			ret += aw32280_write_byte(di, AW32280_IRQ_FLAG_REG + 1 + i, 0xff);
		}
	}

	return ret;
}

static int aw32280_adc_channel_cfg(struct aw32280_device_info *di)
{
	int ret;

	ret = aw32280_write_mask(di, AW32280_HKADC_CTRL1_REG, AW32280_HKADC_SEQ_LOOP_MSK,
		AW32280_HKADC_SEQ_LOOP_SHIFT, AW32280_HKADC_SEQ_LOOP_ENABLE);
	ret += aw32280_write_mask(di, AW32280_HKADC_CTRL1_REG, AW32280_SC_HKADC_AVG_TIMES_MASK,
		AW32280_SC_HKADC_AVG_TIMES_SHIFT, AW32280_SC_HKADC_AVG_TIMES_INIT);
	ret += aw32280_write_byte(di, AW32280_HKADC_SEQ_CH_H_REG, AW32280_HKADC_SEQ_CH_H_INIT);
	ret += aw32280_write_byte(di, AW32280_HKADC_SEQ_CH_L_REG, AW32280_HKADC_SEQ_CH_L_INIT);

	return ret;
}

static int aw32280_adc_enable(int enable, void *dev_data)
{
	int ret;
	u8 value = enable ? AW32280_SC_HKADC_ENABLE : AW32280_SC_HKADC_DISABLE;
	struct aw32280_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	ret = aw32280_write_mask(di, AW32280_HKADC_EN_REG, AW32280_SC_HKADC_EN_MASK,
		AW32280_SC_HKADC_EN_SHIFT, value);
	ret += aw32280_write_mask(di, AW32280_HKADC_START_REG, AW32280_HKADC_START_MASK,
		AW32280_HKADC_START_SHIFT, value);
	hwlog_info("adc enable=%d ret=%d\n", enable, ret);

	return ret;
}

static bool aw32280_is_adc_disabled(struct aw32280_device_info *di)
{
	u8 reg = 0;
	int ret;

	ret = aw32280_read_byte(di, AW32280_HKADC_EN_REG, &reg);
	if (ret || !(reg & AW32280_SC_HKADC_EN_MASK))
		return true;

	return false;
}

static int aw32280_discharge(int enable, void *dev_data)
{
	return 0;
}

static int aw32280_is_device_close(void *dev_data)
{
	u8 val = 0;
	struct aw32280_device_info *di = dev_data;

	if (!di)
		return 1;

	if (aw32280_read_byte(di, AW32280_SC_SC_EN_REG, &val))
		return 1;

	if (val & AW32280_SC_SC_EN)
		return 0;

	return 1;
}

static int aw32280_get_device_id(void *dev_data)
{
	int i;
	u8 chip_id[AW32280_CHIP_ID_REG_NUM] = { 0 };
	struct aw32280_device_info *di = dev_data;

	if (!di)
		return AW32280_DEVICE_ID_GET_FAIL;

	if (di->get_id_time == AW32280_USED)
		return di->device_id;

	di->get_id_time = AW32280_USED;

	for (i = 0; i < AW32280_CHIP_ID_REG_NUM; i++) {
		if (aw32280_read_byte(di, AW32280_CHIP_ID_0_REG + i, &chip_id[i])) {
			di->get_id_time = AW32280_NOT_USED;
			return AW32280_DEVICE_ID_GET_FAIL;
		}
	}
	if ((chip_id[0] != AW32280_CHIP_ID_0) ||
		(chip_id[1] != AW32280_CHIP_ID_1) ||
		(chip_id[2] != AW32280_CHIP_ID_2) ||
		(chip_id[3] != AW32280_CHIP_ID_3) ||
		(chip_id[4] != AW32280_CHIP_ID_4) ||
		((chip_id[5] != AW32280_CHIP_ID_5_1) && (chip_id[5] != AW32280_CHIP_ID_5_2))) {
		di->get_id_time = AW32280_NOT_USED;
		return AW32280_DEVICE_ID_GET_FAIL;
	}

	di->device_id = SWITCHCAP_AW32280;
	hwlog_info("device_id: 0x%x\n", di->device_id);

	return di->device_id;
}

static int aw32280_get_vbat_mv(void *dev_data)
{
	u16 data = 0;
	int ret;
	struct aw32280_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	if (aw32280_is_adc_disabled(di))
		return 0;

	ret = aw32280_write_mask(di, AW32280_HKADC_RD_SEQ_REG,
		AW32280_SC_HKADC_RD_REQ_MASK, AW32280_SC_HKADC_RD_REQ_SHIFT, AW32280_SC_HKADC_RD_REQ);
	ret += aw32280_read_word(di, AW32280_VBAT1_ADC_L_REG, &data);
	if (ret)
		return ret;

	hwlog_info("VBAT_ADC=0x%x\n", data);

	return (int)data;
}

static int aw32280_get_ibat_ma(int *ibat, void *dev_data)
{
	int ret;
	u16 data = 0;
	int ibat_ori;
	struct aw32280_device_info *di = dev_data;

	if (!ibat || !di)
		return -ENODEV;

	if (aw32280_is_adc_disabled(di)) {
		*ibat = 0;
		return 0;
	}

	ret = aw32280_write_mask(di, AW32280_HKADC_RD_SEQ_REG,
		AW32280_SC_HKADC_RD_REQ_MASK, AW32280_SC_HKADC_RD_REQ_SHIFT, AW32280_SC_HKADC_RD_REQ);
	ret += aw32280_read_word(di, AW32280_IBAT1_ADC_L_REG, &data);
	if (ret)
		return ret;

	ibat_ori = (int)data * AW80322_IBAT_ADC_STEP / AW80322_BASE_RATIO_UNIT;
	*ibat = ibat_ori * di->sense_r_config;
	*ibat /= di->sense_r_actual;
	hwlog_info("IBAT_ADC=0x%x ibat_ori=%d ibat=%d\n", data, ibat_ori, *ibat);

	return 0;
}

static int aw32280_get_ibus_ma(int *ibus, void *dev_data)
{
	u16 data = 0;
	int ret;
	struct aw32280_device_info *di = dev_data;

	if (!di || !ibus)
		return -ENODEV;

	if (aw32280_is_adc_disabled(di)) {
		*ibus = 0;
		return 0;
	}

	ret = aw32280_write_mask(di, AW32280_HKADC_RD_SEQ_REG,
		AW32280_SC_HKADC_RD_REQ_MASK, AW32280_SC_HKADC_RD_REQ_SHIFT, AW32280_SC_HKADC_RD_REQ);
	ret += aw32280_read_word(di, AW32280_IBUS_ADC_L_REG, &data);
	if (ret)
		return ret;

	hwlog_info("IBUS_ADC=0x%x\n", data);
	*ibus = (int)data;

	return 0;
}

static int aw32280_get_vbus_mv(int *vbus, void *dev_data)
{
	int ret;
	u16 data = 0;
	struct aw32280_device_info *di = dev_data;

	if (!di || !vbus)
		return -ENODEV;

	if (aw32280_is_adc_disabled(di)) {
		*vbus = 0;
		return 0;
	}

	ret = aw32280_write_mask(di, AW32280_HKADC_RD_SEQ_REG,
		AW32280_SC_HKADC_RD_REQ_MASK, AW32280_SC_HKADC_RD_REQ_SHIFT, AW32280_SC_HKADC_RD_REQ);
	ret += aw32280_read_word(di, AW32280_VBUS_ADC_L_REG, &data);
	if (ret)
		return ret;

	hwlog_info("VBUS_ADC=0x%x\n", data);
	*vbus = (int)data;

	return 0;
}

static int aw32280_get_device_temp(int *temp, void *dev_data)
{
	s16 data = 0;
	int ret;
	struct aw32280_device_info *di = dev_data;

	if (!temp || !di)
		return -ENODEV;

	if (aw32280_is_adc_disabled(di)) {
		*temp = 0;
		return 0;
	}

	ret = aw32280_write_mask(di, AW32280_HKADC_RD_SEQ_REG,
		AW32280_SC_HKADC_RD_REQ_MASK, AW32280_SC_HKADC_RD_REQ_SHIFT, AW32280_SC_HKADC_RD_REQ);
	ret += aw32280_read_word(di, AW32280_TDIE_ADC_L_REG, &data);
	if (ret)
		return ret;
	*temp = ((long long)AW80322_TEMP_ADC_MAX * AW80322_BASE_RATIO_UNIT - (long long)data * AW80322_TEMP_ADC_DATA_COEF)
		/ AW80322_BASE_RATIO_UNIT / AW80322_TEMP_ADC_UNIT;

	hwlog_info("TDIE_ADC=0x%x temp=%d\n", data, *temp);

	return 0;
}

static int aw32280_get_vusb_mv(int *vusb, void *dev_data)
{
	int ret;
	u16 data = 0;
	struct aw32280_device_info *di = dev_data;

	if (!vusb || !di)
		return -ENODEV;

	if (aw32280_is_adc_disabled(di)) {
		*vusb = 0;
		return 0;
	}

	ret = aw32280_write_mask(di, AW32280_HKADC_RD_SEQ_REG,
		AW32280_SC_HKADC_RD_REQ_MASK, AW32280_SC_HKADC_RD_REQ_SHIFT, AW32280_SC_HKADC_RD_REQ);
	ret += aw32280_read_word(di, AW32280_VUSB_ADC_L_REG, &data);
	if (ret)
		return ret;

	hwlog_info("VUSB_ADC=0x%x\n", data);
	*vusb = (int)data;

	return 0;
}

static int aw32280_get_vout_mv(int *vout, void *dev_data)
{
	int ret;
	u16 data = 0;
	struct aw32280_device_info *di = dev_data;

	if (!vout || !di)
		return -ENODEV;

	if (aw32280_is_adc_disabled(di)) {
		*vout = 0;
		return 0;
	}
	ret = aw32280_write_mask(di, AW32280_HKADC_RD_SEQ_REG,
		AW32280_SC_HKADC_RD_REQ_MASK, AW32280_SC_HKADC_RD_REQ_SHIFT, AW32280_SC_HKADC_RD_REQ);
	ret += aw32280_read_word(di, AW32280_VOUT1_ADC_L_REG, &data);
	if (ret)
		return ret;

	hwlog_info("VOUT_ADC=0x%x\n", data);
	*vout = (int)data;

	return 0;
}

static int aw32280_get_register_head(char *buffer, int size, void *dev_data)
{
	struct aw32280_device_info *di = dev_data;

	if (!buffer || !di)
		return -ENODEV;

	snprintf(buffer, size,
		"dev        mode   Ibus   Vbus   Ibat   Vusb   Vout   Vbat   Temp");

	return 0;
}

static int aw32280_value_dump(char *buffer, int size, void *dev_data)
{
	int vbat;
	int ibus = 0;
	int vbus = 0;
	int ibat = 0;
	int vusb = 0;
	int vout = 0;
	int temp = 0;
	u8 val = 0;
	char buff[AW32280_BUF_LEN] = { 0 };
	struct aw32280_device_info *di = dev_data;

	if (!buffer || !di)
		return -ENODEV;

	vbat = aw32280_get_vbat_mv(dev_data);
	(void)aw32280_get_ibus_ma(&ibus, dev_data);
	(void)aw32280_get_vbus_mv(&vbus, dev_data);
	(void)aw32280_get_ibat_ma(&ibat, dev_data);
	(void)aw32280_get_vusb_mv(&vusb, dev_data);
	(void)aw32280_get_vout_mv(&vout, dev_data);
	(void)aw32280_get_device_temp(&temp, dev_data);
	(void)aw32280_read_byte(di, AW32280_SC_SC_MODE_REG, &val);

	snprintf(buff, sizeof(buff), "%s ", di->name);
	strncat(buffer, buff, strlen(buff));

	if (aw32280_is_device_close(dev_data))
		snprintf(buff, sizeof(buff), "%s", "   OFF    ");
	else if (((val & AW32280_SC_SC_MODE_MASK) >> AW32280_SC_SC_MODE_SHIFT) ==
		AW32280_CHG_FBYPASS_MODE)
		snprintf(buff, sizeof(buff), "%s", "   LVC    ");
	else if (((val & AW32280_SC_SC_MODE_MASK) >> AW32280_SC_SC_MODE_SHIFT) ==
		AW32280_CHG_F21SC_MODE)
		snprintf(buff, sizeof(buff), "%s", "   SC     ");
	else if (((val & AW32280_SC_SC_MODE_MASK) >> AW32280_SC_SC_MODE_SHIFT) ==
		AW32280_CHG_F41SC_MODE)
		snprintf(buff, sizeof(buff), "%s", "   SC4    ");

	strncat(buffer, buff, strlen(buff));
	snprintf(buff, sizeof(buff), "%-7d%-7d%-7d%-7d%-7d%-7d%-7d",
		ibus, vbus, ibat, vusb, vout, vbat, temp);
	strncat(buffer, buff, strlen(buff));

	return 0;
}

static int aw32280_config_watchdog_ms(int time, void *dev_data)
{
	u8 val;
	u8 enable = 0;
	u8 reg = 0;
	int ret;
	struct aw32280_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	if (time > AW32280_WTD_CONFIG_TIMING_2000MS)
		val = AW32280_WDT_CONFIG_5S;
	else if (time > AW32280_WTD_CONFIG_TIMING_1000MS)
		val = AW32280_WDT_CONFIG_2S;
	else if (time > AW32280_WTD_CONFIG_TIMING_500MS)
		val = AW32280_WDT_CONFIG_1S;
	else
		val = AW32280_WDT_CONFIG_500MS;

	if (time > 0)
		enable = 1;

	ret = aw32280_write_mask(di, AW32280_WDT_CONFIG_REG,
		AW32280_WDT_CONFIG_MASK, AW32280_WDT_CONFIG_SHIFT, val);
	ret += aw32280_read_byte(di, AW32280_WDT_CONFIG_REG, &reg);
	ret += aw32280_write_mask(di, AW32280_SC_WDT_EN_REG,
		AW32280_SC_WDT_EN_MASK, AW32280_SC_WDT_EN_SHIFT, enable);
	if (ret)
		return ret;

	hwlog_info("config_watchdog_ms [%x]=0x%x\n", AW32280_WDT_CONFIG_REG, reg);

	return 0;
}

static int aw32280_reset_watchdog_execute(void *dev_data)
{
	struct aw32280_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	return aw32280_write_mask(di, AW32280_WDT_SOFT_RESET_REG,
		AW32280_WD_RST_N_MASK, AW32280_WD_RST_N_SHIFT, AW32280_WD_RST_N_RESET);
}

static int aw32280_en_cfg(struct aw32280_device_info *di, int enable)
{
	int ret;
	u8 chg_en = enable ? AW32280_SC_SC_EN : AW32280_SC_SC_DISABLE;

	ret = gpio_direction_output(di->gpio_enable, chg_en);
	if (enable) {
		ret += aw32280_write_mask(di, AW32280_USB_OVP_CFG_1_REG, AW32280_USB_OVP_DA_SC_PD_MASK,
			AW32280_USB_OVP_DA_SC_PD_SHIFT, AW32280_USB_OVP_DA_SC_PD_ENABLE);
		msleep(400); /* sleep 400ms for cap discharger */
		ret += aw32280_write_mask(di, AW32280_USB_OVP_CFG_1_REG, AW32280_USB_OVP_DA_SC_PD_MASK,
			AW32280_USB_OVP_DA_SC_PD_SHIFT, AW32280_USB_OVP_DA_SC_PD_DISABLE);
	}

	ret += aw32280_write_mask(di, AW32280_SC_SC_EN_REG, AW32280_SC_SC_EN_MASK,
		AW32280_SC_SC_EN_SHIFT, chg_en);
	if (enable) {
		msleep(80); /* sleep 80ms prevent current backward */
		ret += aw32280_write_mask(di, AW32280_SC_PRO_TOP_CFG_0_REG, AW32280_DA_SC_IBUS_RCP_EN_MASK,
			AW32280_DA_SC_IBUS_RCP_EN_SHIFT, AW32280_DA_SC_IBUS_RCP_EN_ENABLE);
	}

	return ret;
}

static int aw32280_config_vbat_ovp_threshold(struct aw32280_device_info *di,
	int ovp_th)
{
	u8 vbat1;
	u8 vbat2 = 0;
	int ret;

	if (ovp_th < AW32280_FWD_VBAT_OVP_MIN)
		ovp_th = AW32280_FWD_VBAT_OVP_MIN;

	if (ovp_th > AW32280_FWD_VBAT_OVP_MAX)
		ovp_th = AW32280_FWD_VBAT_OVP_MAX;

	vbat1 = (u8)((ovp_th - AW32280_FWD_VBAT_OVP_MIN) /
		AW32280_FWD_VBAT_OVP_STEP);
	ret = aw32280_write_mask(di, AW32280_FWD_VBAT1_OVP_SEL_REG,
		AW32280_FWD_VBAT_OVP_MASK, AW32280_FWD_VBAT_OVP_SHIFT, vbat1);
	ret += aw32280_write_mask(di, AW32280_FWD_VBAT2_OVP_SEL_REG,
		AW32280_FWD_VBAT_OVP_MASK, AW32280_FWD_VBAT_OVP_SHIFT, vbat1);
	vbat1 = 0;
	ret += aw32280_read_byte(di, AW32280_FWD_VBAT1_OVP_SEL_REG, &vbat1);
	ret += aw32280_read_byte(di, AW32280_FWD_VBAT2_OVP_SEL_REG, &vbat2);
	if (ret)
		return ret;

	hwlog_info("config_vbat1_ovp_threshold_mv [%x]=0x%x\n",
		AW32280_FWD_VBAT1_OVP_SEL_REG, vbat1);
	hwlog_info("config_vbat2_ovp_threshold_mv [%x]=0x%x\n",
		AW32280_FWD_VBAT2_OVP_SEL_REG, vbat2);

	return 0;
}

static int aw32280_config_ibat_sns_res(struct aw32280_device_info *di)
{
	u8 res_config;
	int ret;

	if (di->sense_r_config == SENSE_R_2_MOHM)
		res_config = AW32280_IBAT_SNS_RES_2MOHM;
	else
		res_config = AW32280_IBAT_SNS_RES_1MOHM;

	ret = aw32280_write_byte(di, AW32280_IBAT1_RES_PLACE_REG,
		AW32280_IBAT_RES_LOWSIDE);
	ret += aw32280_write_byte(di, AW32280_IBAT2_RES_PLACE_REG,
		AW32280_IBAT_RES_LOWSIDE);
	ret += aw32280_write_mask(di, AW32280_SC_PRO_TOP_CFG_6_REG,
		AW32280_IBAT_RES_SEL_MASK, AW32280_IBAT_RES_SEL_SHIFT, res_config);

	return ret;
}

static int aw32280_opt_regs_init(struct aw32280_device_info *di)
{
	int ret;

	ret = aw32280_write_byte(di, AW32280_SC_DET_TOP_CFG_2_REG,
		AW32280_DET_TOP_CFG_2_REG_INIT);
	ret += aw32280_write_byte(di, AW32280_USB_OVP_CFG_2_REG,
		AW32280_USB_OVP_CFG_2_REG_INIT);
	ret += aw32280_write_byte(di, AW32280_PSW_OVP_CFG_2_REG,
		AW32280_PSW_OVP_CFG_2_REG_INIT);
	ret += aw32280_write_byte(di, AW32280_SC_PRO_TOP_CFG_7_REG,
		AW32280_PRO_TOP_CFG_7_REG_INIT);
	ret += aw32280_write_byte(di, AW32280_SC_PRO_TOP_CFG_8_REG,
		AW32280_PRO_TOP_CFG_8_REG_INIT);
	ret += aw32280_write_byte(di, AW32280_SC_DET_TOP_CFG_4_REG,
		AW32280_DET_TOP_CFG_4_REG_INIT);
	ret += aw32280_write_byte(di, AW32280_SC_DET_TOP_CFG_5_REG,
		AW32280_DET_TOP_CFG_5_REG_INIT);
	ret += aw32280_config_vbat_ovp_threshold(di, AW80322_FWD_VBAT_OVP_INIT);
	ret += aw32280_write_byte(di, AW32280_SC_VOUT_OVP_REG,
		AW32280_SC_VOUT_OVP_REG_INIT);
	ret += aw32280_write_byte(di, AW32280_SC_TOP_CFG_8_REG,
		AW32280_SC_TOP_CFG_8_REG_INIT);
	ret += aw32280_write_byte(di, AW32280_SC_TOP_CFG_7_REG,
		AW32280_SC_TOP_CFG_7_REG_INIT);
	ret += aw32280_write_byte(di, AW22803_SC_PRO_TOP_CFG_2_REG,
		AW22803_SC_PRO_TOP_CFG_2_REG_INIT);
	ret += aw32280_write_byte(di, AW32280_SC_PRO_TOP_CFG_0_REG,
		AW32280_SC_PRO_TOP_CFG_0_REG_INIT);
	ret += aw32280_write_byte(di, AW32280_SC_PRO_TOP_CFG_5_REG,
		AW32280_SC_PRO_TOP_CFG_5_REG_INIT);
	ret += aw32280_write_byte(di, AW32280_SC_PRO_TOP_CFG_4_REG,
		AW32280_SC_PRO_TOP_CFG_4_REG_INIT);
	ret += aw32280_write_byte(di, AW32280_SC_PRO_TOP_CFG_3_REG,
		AW32280_SC_PRO_TOP_CFG_3_REG_INIT);
	ret += aw32280_write_byte(di, AW32280_SC_DET_TOP_CFG_0_REG,
		AW32280_SC_DET_TOP_CFG_0_REG_INIT);
	ret += aw32280_write_byte(di, AW32280_USB_OVP_CFG_0_REG,
		AW32280_USB_OVP_CFG_0_REG_INIT);
	ret += aw32280_write_byte(di, AW32280_FWD_VBAT1_OVP_SEL_REG,
		AW32280_FWD_VBAT1_OVP_SEL_REG_INIT);
	ret += aw32280_write_byte(di, AW32280_SC_PRO_TOP_CFG_12_REG,
		AW32280_SC_PRO_TOP_CFG_12_REG_INIT);
	ret += aw32280_config_ibat_sns_res(di);

	return ret;
}

static int aw32280_config_switching_frequency(int data, struct aw32280_device_info *di)
{
	u8 freq;
	int ret;

	if (data < AW32280_SW_FREQ_MIN)
		data = AW32280_SW_FREQ_MIN;

	if (data > AW32280_SW_FREQ_MAX)
		data = AW32280_SW_FREQ_MAX;
	freq = (u8)((data - AW32280_SW_FREQ_MIN) / AW32280_SW_FREQ_STEP);
	ret = aw32280_write_mask(di, AW32280_SC_TOP_CFG_4_REG,
		AW32280_SC_FREQ_SEL_MASK, AW32280_SC_FREQ_SEL_SHIFT, freq);
	if (ret)
		return -ENODEV;
	hwlog_info("aw32280 config_switching_frequency [%x]=0x%x\n",
		AW32280_SC_TOP_CFG_4_REG, freq);

	return 0;
}

static int aw32280_charge_mode_enable(int enable, void *dev_data, u8 mode)
{
	int ret;
	u8 mode_reg = 0;
	u8 chg_en_reg = 0;
	u8 usb_ovp_val = 0;
	u8 psw_ovp_val = 0;
	struct aw32280_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	if (mode == AW32280_CHG_F41SC_MODE) {
		usb_ovp_val = AW32280_USB_OVP_F41SC_VAL;
		psw_ovp_val = AW32280_PSW_OVP_F41SC_VAL;
	} else if (mode == AW32280_CHG_F21SC_MODE) {
		usb_ovp_val = AW32280_USB_OVP_F21SC_VAL;
		psw_ovp_val = AW32280_PSW_OVP_F21SC_VAL;
	} else if (mode == AW32280_CHG_FBYPASS_MODE) {
		usb_ovp_val = AW32280_USB_OVP_FBPSC_VAL;
		psw_ovp_val = AW32280_PSW_OVP_FBPSC_VAL;
	}
	ret = aw32280_write_mask(di, AW32280_USB_OVP_CFG_2_REG,
		AW32280_USB_OVP_MASK, AW32280_USB_OVP_SHIFT, usb_ovp_val);
	ret += aw32280_write_mask(di, AW32280_PSW_OVP_CFG_2_REG,
		AW32280_PSW_OVP_MASK, AW32280_PSW_OVP_SHIFT, psw_ovp_val);
	ret += aw32280_write_mask(di, AW32280_SC_SC_MODE_REG,
		AW32280_SC_SC_MODE_MASK, AW32280_SC_SC_MODE_SHIFT, mode);
	ret += aw32280_en_cfg(di, enable);
	ret += aw32280_read_byte(di, AW32280_SC_SC_MODE_REG, &mode_reg);
	ret += aw32280_read_byte(di, AW32280_SC_SC_EN_REG, &chg_en_reg);
	if (ret)
		return ret;

	di->charge_mode = mode;
	hwlog_info("ic_role=%d, charge_enable [0x%x]=0x%x,[0x%x]=0x%x\n",
		di->ic_role, AW32280_SC_SC_MODE_REG, mode_reg,
		AW32280_SC_SC_EN_REG, chg_en_reg);

	return 0;
}

static int aw32280_lvc_charge_enable(int enable, void *dev_data)
{
	return aw32280_charge_mode_enable(enable, dev_data, AW32280_CHG_FBYPASS_MODE);
}

static int aw32280_sc_charge_enable(int enable, void *dev_data)
{
	return aw32280_charge_mode_enable(enable, dev_data, AW32280_CHG_F21SC_MODE);
}

static int aw32280_sc_41_charge_enable(int enable, void *dev_data)
{
	return aw32280_charge_mode_enable(enable, dev_data, AW32280_CHG_F41SC_MODE);
}

static int aw32280_reg_init(struct aw32280_device_info *di)
{
	int ret;

	ret = aw32280_en_cfg(di, 0);
	ret += aw32280_opt_regs_init(di);
	ret += aw32280_adc_channel_cfg(di);
	ret += aw32280_adc_enable(1, di);
	ret += aw32280_charger_irq_clear(di);
	if (ret)
		hwlog_err("reg_init fail %d\n", ret);

	return ret;
}

static int aw32280_lvc_charge_init(void *dev_data)
{
	int ret;
	struct aw32280_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	ret = aw32280_reg_init(di);
	ret += aw32280_config_switching_frequency(AW32280_SW_FREQ_MIN, di);
	if (ret)
		return -EINVAL;

	hwlog_info("ic_role=%d device_id=%d init\n", di->ic_role, di->device_id);

	di->init_finish_flag = AW32280_INIT_FINISH;

	return 0;
}

static int aw32280_sc_charge_init(void *dev_data)
{
	int ret;
	struct aw32280_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	ret = aw32280_reg_init(di);
	ret += aw32280_config_switching_frequency(AW32280_SW_FREQ_MIN, di);
	if (ret)
		return -EINVAL;

	hwlog_info("ic_role=%d device_id=%d init\n", di->ic_role, di->device_id);

	di->init_finish_flag = AW32280_INIT_FINISH;

	return 0;
}

static int aw32280_sc4_charge_init(void *dev_data)
{
	int ret;
	struct aw32280_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	ret = aw32280_reg_init(di);
	ret += aw32280_config_switching_frequency(AW32280_SW_FREQ_600KHZ, di);
	if (ret)
		return -EINVAL;

	hwlog_info("ic_role=%d device_id=%d init\n", di->ic_role, di->device_id);

	di->init_finish_flag = AW32280_INIT_FINISH;

	return 0;
}

static int aw32280_charge_exit(void *dev_data)
{
	int ret;
	struct aw32280_device_info *di = dev_data;

	if (!di)
		return -ENODEV;

	ret = aw32280_charge_mode_enable(0, dev_data, AW32280_CHG_F41SC_MODE);
	di->init_finish_flag = AW32280_NOT_INIT;
	di->int_notify_enable_flag = AW32280_DISABLE_INT_NOTIFY;
	di->charge_mode = AW32280_CHG_F41SC_MODE;

	return ret;
}

static int aw32280_batinfo_exit(void *dev_data)
{
	return 0;
}

static int aw32280_batinfo_init(void *dev_data)
{
	return 0;
}

static struct dc_ic_ops aw32280_lvc_ops = {
	.dev_name = "aw32280",
	.ic_init = aw32280_lvc_charge_init,
	.ic_exit = aw32280_charge_exit,
	.ic_enable = aw32280_lvc_charge_enable,
	.ic_adc_enable = aw32280_adc_enable,
	.ic_discharge = aw32280_discharge,
	.is_ic_close = aw32280_is_device_close,
	.get_ic_id = aw32280_get_device_id,
	.config_ic_watchdog = aw32280_config_watchdog_ms,
	.kick_ic_watchdog = aw32280_reset_watchdog_execute,
};

static struct dc_ic_ops aw32280_sc_ops = {
	.dev_name = "aw32280",
	.ic_init = aw32280_sc_charge_init,
	.ic_exit = aw32280_charge_exit,
	.ic_enable = aw32280_sc_charge_enable,
	.ic_adc_enable = aw32280_adc_enable,
	.ic_discharge = aw32280_discharge,
	.is_ic_close = aw32280_is_device_close,
	.get_ic_id = aw32280_get_device_id,
	.config_ic_watchdog = aw32280_config_watchdog_ms,
	.kick_ic_watchdog = aw32280_reset_watchdog_execute,
};

static struct dc_ic_ops aw32280_sc4_ops = {
	.dev_name = "aw32280",
	.ic_init = aw32280_sc4_charge_init,
	.ic_exit = aw32280_charge_exit,
	.ic_enable = aw32280_sc_41_charge_enable,
	.ic_adc_enable = aw32280_adc_enable,
	.ic_discharge = aw32280_discharge,
	.is_ic_close = aw32280_is_device_close,
	.get_ic_id = aw32280_get_device_id,
	.config_ic_watchdog = aw32280_config_watchdog_ms,
	.kick_ic_watchdog = aw32280_reset_watchdog_execute,
};

static struct dc_batinfo_ops aw32280_batinfo_ops = {
	.init = aw32280_batinfo_init,
	.exit = aw32280_batinfo_exit,
	.get_bat_btb_voltage = aw32280_get_vbat_mv,
	.get_bat_package_voltage = aw32280_get_vbat_mv,
	.get_vbus_voltage = aw32280_get_vbus_mv,
	.get_bat_current = aw32280_get_ibat_ma,
	.get_ic_ibus = aw32280_get_ibus_ma,
	.get_ic_temp = aw32280_get_device_temp,
	.get_ic_vout = aw32280_get_vout_mv,
	.get_ic_vusb = aw32280_get_vusb_mv,
};

static struct power_log_ops aw32280_log_ops = {
	.dev_name = "aw32280",
	.dump_log_head = aw32280_get_register_head,
	.dump_log_content = aw32280_value_dump,
};

static void aw32280_init_ops_dev_data(struct aw32280_device_info *di)
{
	memcpy(&di->lvc_ops, &aw32280_lvc_ops, sizeof(struct dc_ic_ops));
	di->lvc_ops.dev_data = (void *)di;
	memcpy(&di->sc_ops, &aw32280_sc_ops, sizeof(struct dc_ic_ops));
	di->sc_ops.dev_data = (void *)di;
	memcpy(&di->sc4_ops, &aw32280_sc4_ops, sizeof(struct dc_ic_ops));
	di->sc4_ops.dev_data = (void *)di;
	memcpy(&di->batinfo_ops, &aw32280_batinfo_ops, sizeof(struct dc_batinfo_ops));
	di->batinfo_ops.dev_data = (void *)di;
	memcpy(&di->log_ops, &aw32280_log_ops, sizeof(struct power_log_ops));
	di->log_ops.dev_data = (void *)di;

	if (!di->ic_role) {
		snprintf(di->name, CHIP_DEV_NAME_LEN, "aw32280");
	} else {
		snprintf(di->name, CHIP_DEV_NAME_LEN, "aw32280_%d", di->ic_role);
		di->lvc_ops.dev_name = di->name;
		di->sc_ops.dev_name = di->name;
		di->sc4_ops.dev_name = di->name;
		di->log_ops.dev_name = di->name;
	}
}

static void aw32280_ops_register(struct aw32280_device_info *di)
{
	int ret;

	aw32280_init_ops_dev_data(di);

	ret = dc_ic_ops_register(LVC_MODE, di->ic_role, &di->lvc_ops);
	ret += dc_ic_ops_register(SC_MODE, di->ic_role, &di->sc_ops);
	ret += dc_ic_ops_register(SC4_MODE, di->ic_role, &di->sc4_ops);
	ret += dc_batinfo_ops_register(di->ic_role, &di->batinfo_ops, di->device_id);
	if (ret) {
		hwlog_err("sysinfo ops register fail\n");
		return;
	}

	ret = power_log_ops_register(&di->log_ops);
	if (ret)
		hwlog_err("thermalzone or power log ops register fail\n");
}

static bool aw32280_vbus_value_check(struct aw32280_device_info *di, int val)
{
	bool ret = false;

	switch (di->charge_mode) {
	case AW32280_CHG_FBYPASS_MODE:
		if (val >= AW32280_VBUS_OVP_FBPSC_MIN)
			ret = true;
		break;
	case AW32280_CHG_F21SC_MODE:
		if (val >= AW32280_VBUS_OVP_F21SC_MIN)
			ret = true;
		break;
	case AW32280_CHG_F41SC_MODE:
		if (val >= AW32280_VBUS_OVP_F41SC_MIN)
			ret = true;
		break;
	default:
		hwlog_err("mode check fail, mode=%u\n", di->charge_mode);
		break;
	}

	return ret;
}

static bool aw32280_ibus_value_check(struct aw32280_device_info *di, int val)
{
	bool ret = false;

	switch (di->charge_mode) {
	case AW32280_CHG_FBYPASS_MODE:
	case AW32280_CHG_F21SC_MODE:
		if (val >= AW32280_IBUS_OCP_F21SC_MIN)
			ret = true;
		break;
	case AW32280_CHG_F41SC_MODE:
		if (val >= AW32280_IBUS_OCP_F41SC_MIN)
			ret = true;
		break;
	default:
		hwlog_err("mode check fail, mode=%u\n", di->charge_mode);
		break;
	}

	return ret;
}

static void aw32280_fault_event_notify(unsigned long event, void *data)
{
	power_event_anc_notify(POWER_ANT_SC4_FAULT, event, data);
}

static void aw32280_fault_handle(struct aw32280_device_info *di,
	struct nty_data *data)
{
	int val = 0;
	u8 flag = data->event1;
	u8 flag1 = data->event2;
	u8 flag2 = data->event3;

	if (flag2 & AW32280_VBAT_OVP_FLAG_MASK) {
		val = aw32280_get_vbat_mv(di);
		hwlog_info("BAT OVP happened, vbat=%d\n", val);
		if (val >= AW80322_FWD_VBAT_OVP_INIT)
			aw32280_fault_event_notify(POWER_NE_DC_FAULT_VBAT_OVP, data);
	} else if (flag1 & AW32280_IBAT_OVP_FLAG_MASK) {
		aw32280_get_ibat_ma(&val, di);
		hwlog_info("BAT OCP happened,ibat=%d\n", val);
		if (val >= AW32280_FWD_IBAT_OVP_INIT)
			aw32280_fault_event_notify(POWER_NE_DC_FAULT_IBAT_OCP, data);
	} else if (flag & AW32280_VBUS_OVP_FLAG_MASK) {
		aw32280_get_vbus_mv(&val, di);
		hwlog_info("BUS OVP happened,vbus=%d\n", val);
		if (aw32280_vbus_value_check(di, val))
			aw32280_fault_event_notify(POWER_NE_DC_FAULT_VBUS_OVP, data);
	} else if (flag2 & AW32280_IBUS_OCP_FLAG_MASK) {
		aw32280_get_ibus_ma(&val, di);
		hwlog_info("BUS OCP happened,ibus=%d\n", val);
		if (aw32280_ibus_value_check(di, val))
			aw32280_fault_event_notify(POWER_NE_DC_FAULT_IBUS_OCP, data);
	} else if (flag & AW32280_VOUT_OVP_FLAG_MASK) {
		hwlog_info("VOUT OVP happened\n");
	}
}

static void aw32280_interrupt_work(struct work_struct *work)
{
	u8 irq_flag2 = 0;
	u8 irq_flag3 = 0;
	u8 irq_flag4 = 0;
	struct aw32280_device_info *di = NULL;
	struct nty_data *data = NULL;

	if (!work)
		return;

	di = container_of(work, struct aw32280_device_info, irq_work);
	if (!di || !di->client) {
		hwlog_err("di is null\n");
		return;
	}

	(void)aw32280_read_byte(di, AW32280_IRQ_FLAG_2_REG, &irq_flag2);
	(void)aw32280_read_byte(di, AW32280_IRQ_FLAG_3_REG, &irq_flag3);
	(void)aw32280_read_byte(di, AW32280_IRQ_FLAG_4_REG, &irq_flag4);

	data = &(di->nty_data);
	data->event1 = irq_flag2;
	data->event2 = irq_flag3;
	data->event3 = irq_flag4;
	data->addr = di->client->addr;

	if (di->int_notify_enable_flag) {
		aw32280_fault_handle(di, data);
		aw32280_dump_register(di);
	}
	if (aw32280_charger_irq_clear(di))
		hwlog_err("irq clear fail\n");

	hwlog_info("FLAG2 [0x%x]=0x%x, FLAG3 [0x%x]=0x%x, FLAG4 [0x%x]=0x%x\n",
		AW32280_IRQ_FLAG_2_REG, irq_flag2, AW32280_IRQ_FLAG_3_REG, irq_flag3,
		AW32280_IRQ_FLAG_4_REG, irq_flag4);

	enable_irq(di->irq_int);
}

static irqreturn_t aw32280_interrupt(int irq, void *_di)
{
	struct aw32280_device_info *di = _di;

	if (!di)
		return IRQ_HANDLED;

	if (di->chip_already_init == 0)
		hwlog_err("chip not init\n");

	if (di->init_finish_flag == AW32280_INIT_FINISH)
		di->int_notify_enable_flag = AW32280_ENABLE_INT_NOTIFY;

	hwlog_info("int happened\n");
	disable_irq_nosync(di->irq_int);
	schedule_work(&di->irq_work);

	return IRQ_HANDLED;
}

static int aw32280_irq_init(struct aw32280_device_info *di,
	struct device_node *np)
{
	int ret;

	ret = power_gpio_config_interrupt(np,
		"gpio_int", "aw32280_gpio_int", &di->gpio_int, &di->irq_int);
	if (ret)
		return ret;

	ret = request_irq(di->irq_int, aw32280_interrupt,
		IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND, "aw32280_int_irq", di);
	if (ret) {
		hwlog_err("gpio irq request fail\n");
		di->irq_int = -1;
		gpio_free(di->gpio_int);
		return ret;
	}

	enable_irq_wake(di->irq_int);
	INIT_WORK(&di->irq_work, aw32280_interrupt_work);

	return 0;
}

static int aw32280_gpio_init(struct aw32280_device_info *di,
	struct device_node *np)
{
	if (power_gpio_config_output(np,
		"gpio_reset", "aw32280_gpio_reset", &di->gpio_reset, 1))
		return -EPERM;

	if (power_gpio_config_output(np,
		"gpio_enable", "aw32280_gpio_enable", &di->gpio_enable, 0)) {
		gpio_free(di->gpio_reset);
		return -EPERM;
	}

	(void)power_gpio_config_output(np,
		"gpio_lpm", "aw32280_gpio_lpm", &di->gpio_lpm, 0);

	return 0;
}

static void aw32280_parse_dts(struct device_node *np,
	struct aw32280_device_info *di)
{
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"ic_role", &di->ic_role, IC_ONE);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"slave_mode", &di->slave_mode, AW32280_HOST_MODE);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"parallel_mode", &di->parallel_mode, AW32280_STANDALONE_MODE);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"sense_r_config", &di->sense_r_config, SENSE_R_2_MOHM);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"sense_r_actual", &di->sense_r_actual, SENSE_R_2_MOHM);
}

static int aw32280_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret;
	struct aw32280_device_info *di = NULL;
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

	ret = aw32280_get_device_id(di);
	if (ret == AW32280_DEVICE_ID_GET_FAIL)
		goto aw32280_fail_0;

	aw32280_parse_dts(np, di);

	ret = aw32280_gpio_init(di, np);
	if (ret)
		goto aw32280_fail_0;

	ret = aw32280_irq_init(di, np);
	if (ret)
		goto aw32280_fail_1;

	ret = aw32280_reg_init(di);
	if (ret)
		goto aw32280_fail_2;

	aw32280_ops_register(di);
	i2c_set_clientdata(client, di);

	return 0;

aw32280_fail_2:
	free_irq(di->irq_int, di);
	gpio_free(di->gpio_int);
aw32280_fail_1:
	gpio_free(di->gpio_lpm);
	gpio_free(di->gpio_enable);
	gpio_free(di->gpio_reset);
aw32280_fail_0:
	di->chip_already_init = 0;
	devm_kfree(&client->dev, di);

	return ret;
}

static int aw32280_remove(struct i2c_client *client)
{
	struct aw32280_device_info *di = i2c_get_clientdata(client);

	if (!di)
		return -ENODEV;

	if (di->irq_int)
		free_irq(di->irq_int, di);

	if (di->gpio_int)
		gpio_free(di->gpio_int);

	if (di->gpio_enable)
		gpio_free(di->gpio_enable);

	if (di->gpio_reset)
		gpio_free(di->gpio_reset);

	return 0;
}

static void aw32280_shutdown(struct i2c_client *client)
{
	struct aw32280_device_info *di = i2c_get_clientdata(client);

	if (!di)
		return;

	aw32280_reg_reset(di);
}

#ifdef CONFIG_PM
static int aw32280_i2c_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct aw32280_device_info *di = NULL;

	if (!client)
		return 0;

	di = i2c_get_clientdata(client);
	if (di)
		aw32280_adc_enable(0, di);

	return 0;
}

static int aw32280_i2c_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct aw32280_device_info *di = NULL;

	if (!client)
		return 0;

	di = i2c_get_clientdata(client);
	if (di)
		aw32280_adc_enable(1, di);

	return 0;
}

static const struct dev_pm_ops aw32280_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(aw32280_i2c_suspend, aw32280_i2c_resume)
};
#define AW32280_PM_OPS (&aw32280_pm_ops)
#else
#define AW32280_PM_OPS (NULL)
#endif /* CONFIG_PM */

MODULE_DEVICE_TABLE(i2c, aw32280);
static const struct of_device_id aw32280_of_match[] = {
	{
		.compatible = "aw32280",
		.data = NULL,
	},
	{},
};

static const struct i2c_device_id aw32280_i2c_id[] = {
	{ "aw32280", 0 },
	{}
};

static struct i2c_driver aw32280_driver = {
	.probe = aw32280_probe,
	.remove = aw32280_remove,
	.shutdown = aw32280_shutdown,
	.id_table = aw32280_i2c_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "aw32280",
		.of_match_table = of_match_ptr(aw32280_of_match),
		.pm = AW32280_PM_OPS,
	},
};

static int __init aw32280_init(void)
{
	return i2c_add_driver(&aw32280_driver);
}

static void __exit aw32280_exit(void)
{
	i2c_del_driver(&aw32280_driver);
}

module_init(aw32280_init);
module_exit(aw32280_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("aw32280 module driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
