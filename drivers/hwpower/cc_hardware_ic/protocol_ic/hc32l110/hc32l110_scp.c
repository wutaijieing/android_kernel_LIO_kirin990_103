// SPDX-License-Identifier: GPL-2.0
/*
 * hc32l110_protocol.c
 *
 * hc32l110 protocol driver
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

#include "hc32l110_scp.h"
#include "hc32l110_i2c.h"
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/protocol/adapter_protocol.h>
#include <chipset_common/hwpower/protocol/adapter_protocol_scp.h>
#include <chipset_common/hwpower/protocol/adapter_protocol_fcp.h>

#define HWLOG_TAG hc32l110
HWLOG_REGIST();

static u32 g_hc32l110_scp_error_flag;

static int hc32l110_scp_wdt_reset_by_sw(void *dev_data)
{
	int ret;
	struct hc32l110_device_info *di = dev_data;

	ret = hc32l110_write_byte(di, HC32L110_SCP_STIMER_REG, HC32L110_SCP_STIMER_WDT_RESET);
	ret += hc32l110_write_byte(di, HC32L110_RT_BUFFER_0_REG, HC32L110_RT_BUFFER_0_WDT_RESET);
	ret += hc32l110_write_byte(di, HC32L110_RT_BUFFER_1_REG, HC32L110_RT_BUFFER_1_WDT_RESET);
	ret += hc32l110_write_byte(di, HC32L110_RT_BUFFER_2_REG, HC32L110_RT_BUFFER_2_WDT_RESET);
	ret += hc32l110_write_byte(di, HC32L110_RT_BUFFER_3_REG, HC32L110_RT_BUFFER_3_WDT_RESET);
	ret += hc32l110_write_byte(di, HC32L110_RT_BUFFER_4_REG, HC32L110_RT_BUFFER_4_WDT_RESET);
	ret += hc32l110_write_byte(di, HC32L110_RT_BUFFER_5_REG, HC32L110_RT_BUFFER_5_WDT_RESET);
	ret += hc32l110_write_byte(di, HC32L110_RT_BUFFER_6_REG, HC32L110_RT_BUFFER_6_WDT_RESET);
	ret += hc32l110_write_byte(di, HC32L110_RT_BUFFER_7_REG, HC32L110_RT_BUFFER_7_WDT_RESET);
	ret += hc32l110_write_byte(di, HC32L110_RT_BUFFER_8_REG, HC32L110_RT_BUFFER_8_WDT_RESET);
	ret += hc32l110_write_byte(di, HC32L110_RT_BUFFER_9_REG, HC32L110_RT_BUFFER_9_WDT_RESET);
	ret += hc32l110_write_byte(di, HC32L110_RT_BUFFER_10_REG, HC32L110_RT_BUFFER_10_WDT_RESET);

	return ret;
}

static int hc32l110_scp_cmd_transfer_check(void *dev_data)
{
	u8 reg_val0 = 0;
	u8 reg_val1 = 0;
	u8 reg_val2 = 0;
	int i = 0;
	int ret;
	struct hc32l110_device_info *di = dev_data;

	do {
		power_msleep(DT_MSLEEP_50MS, 0, NULL); /* wait 50ms for each cycle */
		ret = hc32l110_read_byte(di, HC32L110_SCP_CTL_REG, &reg_val0);
		ret += hc32l110_read_byte(di, HC32L110_SCP_ISR1_REG, &reg_val1);
		ret += hc32l110_read_byte(di, HC32L110_SCP_ISR2_REG, &reg_val2);
		if (ret) {
			hwlog_err("reg read failed\n");
			break;
		}
		hwlog_info("reg 0x1b=0x%x, 0x1c=0x%x, 0x1d=0x%x, scp_isr_backup[0] = 0x%x, scp_isr_backup[1] = 0x%x\n",
			reg_val0, reg_val1, reg_val2, di->scp_isr_backup[0], di->scp_isr_backup[1]);
		/* interrupt work can hook the interrupt value first. so it is necessily to do backup via isr_backup */
		reg_val1 |= di->scp_isr_backup[0];
		reg_val2 |= di->scp_isr_backup[1];
		if (reg_val1 || reg_val2) {
			if (((reg_val2 & HC32L110_SCP_ISR2_ACK_MASK) &&
				(reg_val2 & HC32L110_SCP_ISR2_CMD_CPL_MASK)) &&
				!(reg_val1 & (HC32L110_SCP_ISR1_ACK_CRCRX_MASK |
				HC32L110_SCP_ISR1_ACK_PARRX_MASK |
				HC32L110_SCP_ISR1_ERR_ACK_L_MASK))) {
				return 0;
			} else if (reg_val1 & (HC32L110_SCP_ISR1_ACK_CRCRX_MASK |
				HC32L110_SCP_ISR1_ENABLE_HAND_NO_RESPOND_MASK)) {
				hwlog_err("scp transfer fail, slave status changed: ISR1 = 0x%x, ISR2 = 0x%x\n",
					reg_val1, reg_val2);
				return -EPERM;
			} else if (reg_val2 & HC32L110_SCP_ISR2_NACK_MASK) {
				hwlog_err("scp transfer fail, slave nack: ISR1 = 0x%x, ISR2 = 0x%x\n",
					reg_val1, reg_val2);
				return -EPERM;
			} else if (reg_val1 & (HC32L110_SCP_ISR1_ACK_CRCRX_MASK |
				HC32L110_SCP_ISR1_ACK_PARRX_MASK |
				HC32L110_SCP_ISR1_TRANS_HAND_NO_RESPOND_MASK)) {
				hwlog_err("scp transfer fail, CRCRX_PARRX_ERROR: ISR1 = 0x%x, ISR2 = 0x%x\n",
					reg_val1, reg_val2);
				return -EPERM;
			}
			hwlog_err("scp transfer fail, ISR1 = 0x%x, ISR2 = 0x%x, index = %d\n",
				reg_val1, reg_val2, i);
		}
		i++;
	} while (i < HC32L110_SCP_ACK_RETRY_CYCLE);

	hwlog_err("scp adapter transfer time out\n");
	return -EPERM;
}

static void hc32l110_scp_protocol_restart(void *dev_data)
{
	u8 reg_val = 0;
	int ret;
	int i;
	struct hc32l110_device_info *di = dev_data;

	mutex_lock(&di->scp_detect_lock);

	/* detect scp charger, wait for ping succ */
	for (i = 0; i < HC32L110_SCP_RESTART_TIME; i++) {
		power_usleep(DT_USLEEP_10MS); /* wait 10ms for each cycle */
		ret = hc32l110_read_byte(di, HC32L110_SCP_STATUS_REG, &reg_val);
		if (ret) {
			hwlog_err("read det attach err, ret: %d\n", ret);
			continue;
		}

		if (reg_val & HC32L110_SCP_STATUS_ENABLE_HAND_SUCCESS_MASK)
			break;
	}

	if (i == HC32L110_SCP_RESTART_TIME) {
		hwlog_err("wait for slave fail\n");
		mutex_unlock(&di->scp_detect_lock);
		return;
	}
	mutex_unlock(&di->scp_detect_lock);
	hwlog_info("disable and enable scp protocol accp status is 0x%x\n", reg_val);
}

static int hc32l110_scp_adapter_reg_read(u8 *val, u8 reg, void *dev_data)
{
	int ret;
	int i;
	u8 reg_val1 = 0;
	u8 reg_val2 = 0;
	struct hc32l110_device_info *di = dev_data;

	mutex_lock(&di->accp_adapter_reg_lock);
	hwlog_info("CMD = 0x%x, REG = 0x%x\n", HC32L110_SCP_CMD_SBRRD, reg);

	for (i = 0; i < HC32L110_SCP_RETRY_TIME; i++) {
		/* init */
		hc32l110_write_mask(di, HC32L110_SCP_CTL_REG, HC32L110_SCP_CTL_SNDCMD_MASK,
			HC32L110_SCP_CTL_SNDCMD_SHIFT, HC32L110_SCP_CTL_SNDCMD_RESET);
		/* before send cmd, clear isr interrupt registers */
		ret = hc32l110_read_byte(di, HC32L110_SCP_ISR1_REG, &reg_val1);
		ret += hc32l110_read_byte(di, HC32L110_SCP_ISR2_REG, &reg_val2);
		ret += hc32l110_write_byte(di, HC32L110_RT_BUFFER_0_REG, HC32L110_SCP_CMD_SBRRD);
		ret += hc32l110_write_byte(di, HC32L110_RT_BUFFER_1_REG, reg);
		ret += hc32l110_write_byte(di, HC32L110_RT_BUFFER_2_REG, 1);
		/* initial scp_isr_backup[0],[1] due to catching the missing isr by interrupt_work */
		di->scp_isr_backup[0] = 0;
		di->scp_isr_backup[1] = 0;
		ret += hc32l110_write_mask(di, HC32L110_SCP_CTL_REG, HC32L110_SCP_CTL_SNDCMD_MASK,
			HC32L110_SCP_CTL_SNDCMD_SHIFT, HC32L110_SCP_CTL_SNDCMD_START);
		if (ret) {
			hwlog_err("write error, ret is %d\n", ret);
			/* manual init */
			hc32l110_write_mask(di, HC32L110_SCP_CTL_REG, HC32L110_SCP_CTL_SNDCMD_MASK,
				HC32L110_SCP_CTL_SNDCMD_SHIFT, HC32L110_SCP_CTL_SNDCMD_RESET);
			mutex_unlock(&di->accp_adapter_reg_lock);
			return -EPERM;
		}

		/* check cmd transfer success or fail */
		if (hc32l110_scp_cmd_transfer_check(di) == 0) {
			/* recived data from adapter */
			ret = hc32l110_read_byte(di, HC32L110_RT_BUFFER_12_REG, val);
			break;
		}

		hc32l110_scp_protocol_restart(di);
	}
	if (i >= HC32L110_SCP_RETRY_TIME) {
		hwlog_err("ack error, retry %d times\n", i);
		ret = -1;
	}
	/* manual init */
	hc32l110_write_mask(di, HC32L110_SCP_CTL_REG, HC32L110_SCP_CTL_SNDCMD_MASK,
		HC32L110_SCP_CTL_SNDCMD_SHIFT, HC32L110_SCP_CTL_SNDCMD_RESET);
	power_usleep(DT_USLEEP_10MS); /* wait 10ms for operate effective */
	mutex_unlock(&di->accp_adapter_reg_lock);

	return ret;
}

static int hc32l110_scp_adapter_reg_write(u8 val, u8 reg, void *dev_data)
{
	int ret;
	int i;
	u8 reg_val1 = 0;
	u8 reg_val2 = 0;
	struct hc32l110_device_info *di = dev_data;

	mutex_lock(&di->accp_adapter_reg_lock);
	hwlog_info("CMD = 0x%x, REG = 0x%x, val = 0x%x\n", HC32L110_SCP_CMD_SBRWR, reg, val);

	for (i = 0; i < HC32L110_SCP_RETRY_TIME; i++) {
		/* init */
		hc32l110_write_mask(di, HC32L110_SCP_CTL_REG, HC32L110_SCP_CTL_SNDCMD_MASK,
			HC32L110_SCP_CTL_SNDCMD_SHIFT, HC32L110_SCP_CTL_SNDCMD_RESET);
		/* before send cmd, clear accp interrupt registers */
		ret = hc32l110_read_byte(di, HC32L110_SCP_ISR1_REG, &reg_val1);
		ret += hc32l110_read_byte(di, HC32L110_SCP_ISR2_REG, &reg_val2);
		ret += hc32l110_write_byte(di, HC32L110_RT_BUFFER_0_REG, HC32L110_SCP_CMD_SBRWR);
		ret += hc32l110_write_byte(di, HC32L110_RT_BUFFER_1_REG, reg);
		ret += hc32l110_write_byte(di, HC32L110_RT_BUFFER_2_REG, 1);
		ret += hc32l110_write_byte(di, HC32L110_RT_BUFFER_3_REG, val);
		/* initial scp_isr_backup[0],[1] */
		di->scp_isr_backup[0] = 0;
		di->scp_isr_backup[1] = 0;
		ret += hc32l110_write_mask(di, HC32L110_SCP_CTL_REG, HC32L110_SCP_CTL_SNDCMD_MASK,
			HC32L110_SCP_CTL_SNDCMD_SHIFT, HC32L110_SCP_CTL_SNDCMD_START);
		if (ret) {
			hwlog_err("write error, ret is %d\n", ret);
			/* manual init */
			hc32l110_write_mask(di, HC32L110_SCP_CTL_REG, HC32L110_SCP_CTL_SNDCMD_MASK,
				HC32L110_SCP_CTL_SNDCMD_SHIFT, HC32L110_SCP_CTL_SNDCMD_RESET);
			mutex_unlock(&di->accp_adapter_reg_lock);
			return -EPERM;
		}

		/* check cmd transfer success or fail */
		if (hc32l110_scp_cmd_transfer_check(di) == 0)
			break;

		hc32l110_scp_protocol_restart(di);
	}
	if (i >= HC32L110_SCP_RETRY_TIME) {
		hwlog_err("ack error, retry %d times\n", i);
		ret = -1;
	}
	/* manual init */
	hc32l110_write_mask(di, HC32L110_SCP_CTL_REG, HC32L110_SCP_CTL_SNDCMD_MASK,
		HC32L110_SCP_CTL_SNDCMD_SHIFT, HC32L110_SCP_CTL_SNDCMD_RESET);
	power_usleep(DT_USLEEP_10MS); /* wait 10ms for operate effective */
	mutex_unlock(&di->accp_adapter_reg_lock);

	return ret;
}

static int hc32l110_fcp_master_reset(void *dev_data)
{
	struct hc32l110_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -EPERM;
	}

	return hc32l110_scp_wdt_reset_by_sw(di);
}

static int hc32l110_fcp_adapter_reset(void *dev_data)
{
	int ret;
	struct hc32l110_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -EPERM;
	}

	ret = hc32l110_write_mask(di, HC32L110_SCP_CTL_REG, HC32L110_SCP_CTL_MSTR_RST_MASK,
		HC32L110_SCP_CTL_MSTR_RST_SHIFT, 1);
	power_usleep(DT_USLEEP_20MS); /* wait 20ms for operate effective */
	if (ret) {
		hwlog_err("fcp adapter reset error\n");
		return hc32l110_scp_wdt_reset_by_sw(di);
	}

	return ret;
}

static int hc32l110_fcp_read_switch_status(void *dev_data)
{
	return 0;
}

static int hc32l110_is_fcp_charger_type(void *dev_data)
{
	u8 reg_val = 0;
	int ret;
	struct hc32l110_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -EPERM;
	}

	if (!hc32l110_is_support_fcp(di))
		return 0;

	ret = hc32l110_read_byte(di, HC32L110_SCP_STATUS_REG, &reg_val);
	if (ret)
		return 0;

	if (reg_val & HC32L110_SCP_STATUS_ENABLE_HAND_SUCCESS_MASK)
		return 1;

	return 0;
}

static int hc32l110_fcp_adapter_detect(struct hc32l110_device_info *di)
{
	u8 reg_val = 0;
	int i;
	int ret;

	mutex_lock(&di->scp_detect_lock);

	/* temp */
	ret = hc32l110_read_byte(di, HC32L110_SCP_STATUS_REG, &reg_val);
	if (ret) {
		hwlog_err("read det attach err, ret:%d\n", ret);
		mutex_unlock(&di->scp_detect_lock);
		return -EPERM;
	}

	/* confirm enable hand success status */
	if (reg_val & HC32L110_SCP_STATUS_ENABLE_HAND_SUCCESS_MASK) {
		mutex_unlock(&di->scp_detect_lock);
		hwlog_info("scp adapter detect ok\n");
		return ADAPTER_DETECT_SUCC;
	}

	ret = hc32l110_write_byte(di, HC32L110_SCP_CTL_REG, HC32L110_SCP_CTL_DET_VAL);
	if (ret) {
		hwlog_err("SCP enable detect fail, ret is %d\n", ret);
		hc32l110_write_mask(di, HC32L110_SCP_CTL_REG, HC32L110_SCP_CTL_EN_SCP_MASK,
			HC32L110_SCP_CTL_EN_SCP_SHIFT, 0);
		hc32l110_write_mask(di, HC32L110_SCP_CTL_REG, HC32L110_SCP_CTL_SCP_DET_EN_MASK,
			HC32L110_SCP_CTL_SCP_DET_EN_SHIFT, 0);
		/* reset scp registers when EN_SCP is changed to 0 */
		hc32l110_scp_wdt_reset_by_sw(di);
		hc32l110_fcp_adapter_reset(di);
		mutex_unlock(&di->scp_detect_lock);
		return -EPERM;
	}

	/* waiting for scp set */
	for (i = 0; i < HC32L110_SCP_DETECT_MAX_COUT; i++) {
		ret = hc32l110_read_byte(di, HC32L110_SCP_STATUS_REG, &reg_val);
		if (ret) {
			hwlog_err("read det attach err, ret:%d\n", ret);
			continue;
		}

		hwlog_info("SCP_STATUS_REG=0x%x\n", reg_val);
		if (reg_val & HC32L110_SCP_STATUS_ENABLE_HAND_SUCCESS_MASK)
			break;
		power_msleep(DT_MSLEEP_200MS, 0, NULL);
	}

	if (i == HC32L110_SCP_DETECT_MAX_COUT) {
		hc32l110_write_mask(di, HC32L110_SCP_CTL_REG, HC32L110_SCP_CTL_EN_SCP_MASK,
			HC32L110_SCP_CTL_EN_SCP_SHIFT, 0);
		hc32l110_write_mask(di, HC32L110_SCP_CTL_REG, HC32L110_SCP_CTL_SCP_DET_EN_MASK,
			HC32L110_SCP_CTL_SCP_DET_EN_SHIFT, 0);
		/* reset scp registers when EN_SCP is changed to 0 */
		hc32l110_scp_wdt_reset_by_sw(di);
		hc32l110_fcp_adapter_reset(di);
		hwlog_err("CHG_SCP_ADAPTER_DETECT_OTHER return\n");
		mutex_unlock(&di->scp_detect_lock);
		return ADAPTER_DETECT_OTHER;
	}

	mutex_unlock(&di->scp_detect_lock);
	return ret;
}

static int hc32l110_fcp_stop_charge_config(void *dev_data)
{
	struct hc32l110_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -EPERM;
	}

	hc32l110_fcp_master_reset(di);
	hc32l110_write_mask(di, HC32L110_SCP_CTL_REG, HC32L110_SCP_CTL_SCP_DET_EN_MASK,
		HC32L110_SCP_CTL_SCP_DET_EN_SHIFT, 0);

	return 0;
}

static int hc32l110_scp_reg_read(u8 *val, u8 reg, void *dev_data)
{
	int ret;

	if (g_hc32l110_scp_error_flag) {
		hwlog_err("scp timeout happened, do not read reg = 0x%x\n", reg);
		return -EPERM;
	}

	ret = hc32l110_scp_adapter_reg_read(val, reg, dev_data);
	if (ret) {
		hwlog_err("error reg = 0x%x\n", reg);
		if (reg != HWSCP_ADP_TYPE0)
			g_hc32l110_scp_error_flag = HC32L110_SCP_IS_ERR;

		return -EPERM;
	}

	return 0;
}

static int hc32l110_scp_reg_write(u8 val, u8 reg, void *dev_data)
{
	int ret;

	if (g_hc32l110_scp_error_flag) {
		hwlog_err("scp timeout happened, do not write reg = 0x%x\n", reg);
		return -EPERM;
	}

	ret = hc32l110_scp_adapter_reg_write(val, reg, dev_data);
	if (ret) {
		hwlog_err("error reg = 0x%x\n", reg);
		g_hc32l110_scp_error_flag = HC32L110_SCP_IS_ERR;
		return -EPERM;
	}

	return 0;
}

static int hc32l110_self_check(void *dev_data)
{
	return 0;
}

static int hc32l110_scp_chip_reset(void *dev_data)
{
	return hc32l110_fcp_master_reset(dev_data);
}

static int hc32l110_scp_reg_read_block(int reg, int *val, int num,
	void *dev_data)
{
	int ret;
	int i;
	u8 data = 0;

	if (!val || !dev_data) {
		hwlog_err("val or dev_data is null\n");
		return -EPERM;
	}

	g_hc32l110_scp_error_flag = HC32L110_SCP_NO_ERR;

	for (i = 0; i < num; i++) {
		ret = hc32l110_scp_reg_read(&data, reg + i, dev_data);
		if (ret) {
			hwlog_err("scp read failed, reg=0x%x\n", reg + i);
			return -EPERM;
		}
		val[i] = data;
	}

	return 0;
}

static int hc32l110_scp_reg_write_block(int reg, const int *val, int num,
	void *dev_data)
{
	int ret;
	int i;

	if (!val || !dev_data) {
		hwlog_err("val or dev_data is null\n");
		return -EPERM;
	}

	g_hc32l110_scp_error_flag = HC32L110_SCP_NO_ERR;

	for (i = 0; i < num; i++) {
		ret = hc32l110_scp_reg_write(val[i], reg + i, dev_data);
		if (ret) {
			hwlog_err("scp write failed, reg=0x%x\n", reg + i);
			return -EPERM;
		}
	}

	return 0;
}

static int hc32l110_scp_detect_adapter(void *dev_data)
{
	struct hc32l110_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -EPERM;
	}

	return hc32l110_fcp_adapter_detect(di);
}

static int hc32l110_fcp_reg_read_block(int reg, int *val, int num,
	void *dev_data)
{
	int ret, i;
	u8 data = 0;

	if (!val || !dev_data) {
		hwlog_err("val or dev_data is null\n");
		return -EPERM;
	}

	g_hc32l110_scp_error_flag = HC32L110_SCP_NO_ERR;

	for (i = 0; i < num; i++) {
		ret = hc32l110_scp_reg_read(&data, reg + i, dev_data);
		if (ret) {
			hwlog_err("fcp read failed, reg=0x%x\n", reg + i);
			return -EPERM;
		}
		val[i] = data;
	}
	return 0;
}

static int hc32l110_fcp_reg_write_block(int reg, const int *val, int num,
	void *dev_data)
{
	int ret, i;

	if (!val || !dev_data) {
		hwlog_err("val or dev_data is null\n");
		return -EPERM;
	}

	g_hc32l110_scp_error_flag = HC32L110_SCP_NO_ERR;

	for (i = 0; i < num; i++) {
		ret = hc32l110_scp_reg_write(val[i], reg + i, dev_data);
		if (ret) {
			hwlog_err("fcp write failed, reg=0x%x\n", reg + i);
			return -EPERM;
		}
	}

	return 0;
}

static int hc32l110_fcp_detect_adapter(void *dev_data)
{
	struct hc32l110_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -EPERM;
	}

	return hc32l110_fcp_adapter_detect(di);
}

static int hc32l110_pre_init(void *dev_data)
{
	struct hc32l110_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -EPERM;
	}

	return hc32l110_self_check(di);
}

static int hc32l110_scp_adapter_reset(void *dev_data)
{
	return hc32l110_fcp_adapter_reset(dev_data);
}

static struct hwscp_ops hc32l110_hwscp_ops = {
	.chip_name = "hc32l110",
	.reg_read = hc32l110_scp_reg_read_block,
	.reg_write = hc32l110_scp_reg_write_block,
	.detect_adapter = hc32l110_scp_detect_adapter,
	.soft_reset_master = hc32l110_scp_chip_reset,
	.soft_reset_slave = hc32l110_scp_adapter_reset,
	.pre_init = hc32l110_pre_init,
};

static struct hwfcp_ops hc32l110_hwfcp_ops = {
	.chip_name = "hc32l110",
	.reg_read = hc32l110_fcp_reg_read_block,
	.reg_write = hc32l110_fcp_reg_write_block,
	.detect_adapter = hc32l110_fcp_detect_adapter,
	.soft_reset_master = hc32l110_fcp_master_reset,
	.soft_reset_slave = hc32l110_fcp_adapter_reset,
	.get_master_status = hc32l110_fcp_read_switch_status,
	.stop_charging_config = hc32l110_fcp_stop_charge_config,
	.is_accp_charger_type = hc32l110_is_fcp_charger_type,
};

int hc32l110_hwscp_register(struct hc32l110_device_info *di)
{
	hc32l110_hwscp_ops.dev_data = (void *)di;
	return hwscp_ops_register(&hc32l110_hwscp_ops);
}

int hc32l110_hwfcp_register(struct hc32l110_device_info *di)
{
	hc32l110_hwfcp_ops.dev_data = (void *)di;
	return hwfcp_ops_register(&hc32l110_hwfcp_ops);
}
