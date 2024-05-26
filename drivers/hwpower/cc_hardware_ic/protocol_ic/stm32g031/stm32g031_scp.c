// SPDX-License-Identifier: GPL-2.0
/*
 * stm32g031_protocol.c
 *
 * stm32g031 protocol driver
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

#include "stm32g031_scp.h"
#include "stm32g031_i2c.h"
#include "stm32g031_fw.h"
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/protocol/adapter_protocol.h>
#include <chipset_common/hwpower/protocol/adapter_protocol_scp.h>
#include <chipset_common/hwpower/protocol/adapter_protocol_fcp.h>

#define HWLOG_TAG stm32g031_scp
HWLOG_REGIST();

static u32 g_stm32g031_scp_error_flag;

static int stm32g031_scp_wdt_reset_by_sw(void *dev_data)
{
	int ret;
	struct stm32g031_device_info *di = dev_data;

	ret = stm32g031_write_byte(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_WDT_RESET);
	ret += stm32g031_write_byte(di, STM32G031_SCP_STIMER_REG, STM32G031_SCP_STIMER_WDT_RESET);
	ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_0_REG, STM32G031_RT_BUFFER_0_WDT_RESET);
	ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_1_REG, STM32G031_RT_BUFFER_1_WDT_RESET);
	ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_2_REG, STM32G031_RT_BUFFER_2_WDT_RESET);
	ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_3_REG, STM32G031_RT_BUFFER_3_WDT_RESET);
	ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_4_REG, STM32G031_RT_BUFFER_4_WDT_RESET);
	ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_5_REG, STM32G031_RT_BUFFER_5_WDT_RESET);
	ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_6_REG, STM32G031_RT_BUFFER_6_WDT_RESET);
	ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_7_REG, STM32G031_RT_BUFFER_7_WDT_RESET);
	ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_8_REG, STM32G031_RT_BUFFER_8_WDT_RESET);
	ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_9_REG, STM32G031_RT_BUFFER_9_WDT_RESET);
	ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_10_REG, STM32G031_RT_BUFFER_10_WDT_RESET);

	return ret;
}

static int stm32g031_scp_cmd_transfer_check(void *dev_data)
{
	u8 reg_val0 = 0;
	u8 reg_val1 = 0;
	u8 reg_val2 = 0;
	int i = 0;
	int ret;
	struct stm32g031_device_info *di = dev_data;

	do {
		power_msleep(DT_MSLEEP_10MS, 0, NULL); /* wait 10ms for each cycle */
		ret = stm32g031_read_byte(di, STM32G031_SCP_CTL_REG, &reg_val0);
		ret += stm32g031_read_byte(di, STM32G031_SCP_ISR1_REG, &reg_val1);
		ret += stm32g031_read_byte(di, STM32G031_SCP_ISR2_REG, &reg_val2);
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
			if (((reg_val2 & STM32G031_SCP_ISR2_ACK_MASK) &&
				(reg_val2 & STM32G031_SCP_ISR2_CMD_CPL_MASK)) &&
				!(reg_val1 & (STM32G031_SCP_ISR1_ACK_CRCRX_MASK |
				STM32G031_SCP_ISR1_ACK_PARRX_MASK |
				STM32G031_SCP_ISR1_ERR_ACK_L_MASK))) {
				return 0;
			} else if (reg_val1 & (STM32G031_SCP_ISR1_ACK_CRCRX_MASK |
				STM32G031_SCP_ISR1_ENABLE_HAND_NO_RESPOND_MASK)) {
				hwlog_err("scp transfer fail, slave status changed: ISR1 = 0x%x, ISR2 = 0x%x\n",
					reg_val1, reg_val2);
				return -EPERM;
			} else if (reg_val2 & STM32G031_SCP_ISR2_NACK_MASK) {
				hwlog_err("scp transfer fail, slave nack: ISR1 = 0x%x, ISR2 = 0x%x\n",
					reg_val1, reg_val2);
				return -EPERM;
			} else if (reg_val1 & (STM32G031_SCP_ISR1_ACK_CRCRX_MASK |
				STM32G031_SCP_ISR1_ACK_PARRX_MASK |
				STM32G031_SCP_ISR1_TRANS_HAND_NO_RESPOND_MASK)) {
				hwlog_err("scp transfer fail, CRCRX_PARRX_ERROR: ISR1 = 0x%x, ISR2 = 0x%x\n",
					reg_val1, reg_val2);
				return -EPERM;
			}
			hwlog_err("scp transfer fail, ISR1 = 0x%x, ISR2 = 0x%x, index = %d\n",
				reg_val1, reg_val2, i);
		}
		i++;
	} while (i < STM32G031_SCP_ACK_RETRY_CYCLE);

	hwlog_err("scp adapter transfer time out\n");
	return -EPERM;
}

static void stm32g031_scp_protocol_restart(void *dev_data)
{
	u8 reg_val = 0;
	int ret;
	int i;
	struct stm32g031_device_info *di = dev_data;

	mutex_lock(&di->scp_detect_lock);

	/* detect scp charger, wait for ping succ */
	for (i = 0; i < STM32G031_SCP_RESTART_TIME; i++) {
		power_usleep(DT_USLEEP_10MS); /* wait 10ms for each cycle */
		ret = stm32g031_read_byte(di, STM32G031_SCP_STATUS_REG, &reg_val);
		if (ret) {
			hwlog_err("read det attach err, ret: %d\n", ret);
			continue;
		}

		if (reg_val & STM32G031_SCP_STATUS_ENABLE_HAND_SUCCESS_MASK)
			break;
	}

	if (i == STM32G031_SCP_RESTART_TIME) {
		hwlog_err("wait for slave fail\n");
		mutex_unlock(&di->scp_detect_lock);
		return;
	}
	mutex_unlock(&di->scp_detect_lock);
	hwlog_info("disable and enable scp protocol accp status is 0x%x\n", reg_val);
}

static int stm32g031_scp_adapter_reg_read(u8 *val, u8 reg, void *dev_data)
{
	int ret;
	int i;
	u8 reg_val1 = 0;
	u8 reg_val2 = 0;
	struct stm32g031_device_info *di = dev_data;

	mutex_lock(&di->accp_adapter_reg_lock);
	hwlog_info("CMD = 0x%x, REG = 0x%x\n", STM32G031_SCP_CMD_SBRRD, reg);

	for (i = 0; i < STM32G031_SCP_RETRY_TIME; i++) {
		/* init */
		stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_SNDCMD_MASK,
			STM32G031_SCP_CTL_SNDCMD_SHIFT, STM32G031_SCP_CTL_SNDCMD_RESET);
		/* before send cmd, clear isr interrupt registers */
		ret = stm32g031_read_byte(di, STM32G031_SCP_ISR1_REG, &reg_val1);
		ret += stm32g031_read_byte(di, STM32G031_SCP_ISR2_REG, &reg_val2);
		ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_0_REG, STM32G031_SCP_CMD_SBRRD);
		ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_1_REG, reg);
		ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_2_REG, 1);
		/* initial scp_isr_backup[0],[1] due to catching the missing isr by interrupt_work */
		di->scp_isr_backup[0] = 0;
		di->scp_isr_backup[1] = 0;
		ret += stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_SNDCMD_MASK,
			STM32G031_SCP_CTL_SNDCMD_SHIFT, STM32G031_SCP_CTL_SNDCMD_START);
		if (ret) {
			hwlog_err("write error, ret is %d\n", ret);
			/* manual init */
			stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_SNDCMD_MASK,
				STM32G031_SCP_CTL_SNDCMD_SHIFT, STM32G031_SCP_CTL_SNDCMD_RESET);
			mutex_unlock(&di->accp_adapter_reg_lock);
			return -EPERM;
		}

		/* check cmd transfer success or fail */
		if (stm32g031_scp_cmd_transfer_check(di) == 0) {
			/* recived data from adapter */
			ret = stm32g031_read_byte(di, STM32G031_RT_BUFFER_12_REG, val);
			break;
		}

		stm32g031_scp_protocol_restart(di);
	}
	if (i >= STM32G031_SCP_RETRY_TIME) {
		hwlog_err("ack error, retry %d times\n", i);
		ret = -1;
	}
	/* manual init */
	stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_SNDCMD_MASK,
		STM32G031_SCP_CTL_SNDCMD_SHIFT, STM32G031_SCP_CTL_SNDCMD_RESET);
	power_usleep(DT_USLEEP_10MS); /* wait 10ms for operate effective */
	mutex_unlock(&di->accp_adapter_reg_lock);

	return ret;
}

static int stm32g031_scp_adapter_reg_multi_read(u8 reg, u8 *val, u8 num, void *dev_data)
{
	int ret;
	int i;
	u8 reg_val1 = 0;
	u8 reg_val2 = 0;
	u8 *reg_val = val;
	u8 data_len = (num < STM32G031_SCP_DATA_LEN) ? num : STM32G031_SCP_DATA_LEN;
	struct stm32g031_device_info *di = dev_data;

	if (!di || !val) {
		hwlog_err("di or val is null\n");
		return -EPERM;
	}

	mutex_lock(&di->accp_adapter_reg_lock);
	hwlog_info("CMD = 0x%x, REG = 0x%x, Num = 0x%x\n",
		STM32G031_SCP_CMD_MBRRD, reg, data_len);

	for (i = 0; i < STM32G031_SCP_RETRY_TIME; i++) {
		/* init */
		stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_SNDCMD_MASK,
			STM32G031_SCP_CTL_SNDCMD_SHIFT, STM32G031_SCP_CTL_SNDCMD_RESET);
		/* before send cmd, clear isr interrupt registers */
		ret = stm32g031_read_byte(di, STM32G031_SCP_ISR1_REG, &reg_val1);
		ret += stm32g031_read_byte(di, STM32G031_SCP_ISR2_REG, &reg_val2);
		ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_0_REG, STM32G031_SCP_CMD_MBRRD);
		ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_1_REG, reg);
		ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_2_REG, data_len);
		/* initial scp_isr_backup[0],[1] due to catching the missing isr by interrupt_work */
		di->scp_isr_backup[0] = 0;
		di->scp_isr_backup[1] = 0;
		ret += stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_SNDCMD_MASK,
			STM32G031_SCP_CTL_SNDCMD_SHIFT, STM32G031_SCP_CTL_SNDCMD_START);
		if (ret) {
			hwlog_err("write error, ret is %d\n", ret);
			/* manual init */
			stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_SNDCMD_MASK,
				STM32G031_SCP_CTL_SNDCMD_SHIFT, STM32G031_SCP_CTL_SNDCMD_RESET);
			mutex_unlock(&di->accp_adapter_reg_lock);
			return -EPERM;
		}

		/* check cmd transfer success or fail */
		if (stm32g031_scp_cmd_transfer_check(di) == 0) {
			/* recived data from adapter */
			ret = stm32g031_read_block(di, STM32G031_RT_BUFFER_12_REG, reg_val, data_len);
			break;
		}

		stm32g031_scp_protocol_restart(di);
	}
	if (i >= STM32G031_SCP_RETRY_TIME) {
		hwlog_err("ack error, retry %d times\n", i);
		ret = -1;
	}
	/* manual init */
	stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_SNDCMD_MASK,
		STM32G031_SCP_CTL_SNDCMD_SHIFT, STM32G031_SCP_CTL_SNDCMD_RESET);
	power_usleep(DT_USLEEP_10MS); /* wait 10ms for operate effective */
	mutex_unlock(&di->accp_adapter_reg_lock);

	num -= data_len;
	/* max is STM32G031_SCP_DATA_LEN. remaining data is read in below */
	if (num) {
		reg_val += data_len;
		reg += data_len;
		ret = stm32g031_scp_adapter_reg_multi_read(reg, reg_val, num, di);
		if (ret) {
			/* manual init */
			stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_SNDCMD_MASK,
				STM32G031_SCP_CTL_SNDCMD_SHIFT, STM32G031_SCP_CTL_SNDCMD_RESET);
			return -EPERM;
		}
	}
	/* manual init */
	stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_SNDCMD_MASK,
		STM32G031_SCP_CTL_SNDCMD_SHIFT, STM32G031_SCP_CTL_SNDCMD_RESET);
	power_usleep(DT_USLEEP_10MS); /* wait 10ms for operate effective */

	return ret;
}

static int stm32g031_scp_adapter_reg_write(u8 val, u8 reg, void *dev_data)
{
	int ret;
	int i;
	u8 reg_val1 = 0;
	u8 reg_val2 = 0;
	struct stm32g031_device_info *di = dev_data;

	mutex_lock(&di->accp_adapter_reg_lock);
	hwlog_info("CMD = 0x%x, REG = 0x%x, val = 0x%x\n", STM32G031_SCP_CMD_SBRWR, reg, val);

	for (i = 0; i < STM32G031_SCP_RETRY_TIME; i++) {
		/* init */
		stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_SNDCMD_MASK,
			STM32G031_SCP_CTL_SNDCMD_SHIFT, STM32G031_SCP_CTL_SNDCMD_RESET);
		/* before send cmd, clear accp interrupt registers */
		ret = stm32g031_read_byte(di, STM32G031_SCP_ISR1_REG, &reg_val1);
		ret += stm32g031_read_byte(di, STM32G031_SCP_ISR2_REG, &reg_val2);
		ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_0_REG, STM32G031_SCP_CMD_SBRWR);
		ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_1_REG, reg);
		ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_2_REG, 1);
		ret += stm32g031_write_byte(di, STM32G031_RT_BUFFER_3_REG, val);
		/* initial scp_isr_backup[0],[1] */
		di->scp_isr_backup[0] = 0;
		di->scp_isr_backup[1] = 0;
		ret += stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_SNDCMD_MASK,
			STM32G031_SCP_CTL_SNDCMD_SHIFT, STM32G031_SCP_CTL_SNDCMD_START);
		if (ret) {
			hwlog_err("write error, ret is %d\n", ret);
			/* manual init */
			stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_SNDCMD_MASK,
				STM32G031_SCP_CTL_SNDCMD_SHIFT, STM32G031_SCP_CTL_SNDCMD_RESET);
			mutex_unlock(&di->accp_adapter_reg_lock);
			return -EPERM;
		}

		/* check cmd transfer success or fail */
		if (stm32g031_scp_cmd_transfer_check(di) == 0)
			break;

		stm32g031_scp_protocol_restart(di);
	}
	if (i >= STM32G031_SCP_RETRY_TIME) {
		hwlog_err("ack error, retry %d times\n", i);
		ret = -1;
	}
	/* manual init */
	stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_SNDCMD_MASK,
		STM32G031_SCP_CTL_SNDCMD_SHIFT, STM32G031_SCP_CTL_SNDCMD_RESET);
	power_usleep(DT_USLEEP_10MS); /* wait 10ms for operate effective */
	mutex_unlock(&di->accp_adapter_reg_lock);

	return ret;
}

static int stm32g031_fcp_master_reset(void *dev_data)
{
	struct stm32g031_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -EPERM;
	}

	return stm32g031_scp_wdt_reset_by_sw(di);
}

static int stm32g031_fcp_adapter_reset(void *dev_data)
{
	int ret;
	struct stm32g031_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -EPERM;
	}

	ret = stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_MSTR_RST_MASK,
		STM32G031_SCP_CTL_MSTR_RST_SHIFT, 1);
	power_usleep(DT_USLEEP_20MS); /* wait 20ms for operate effective */
	if (ret) {
		hwlog_err("fcp adapter reset error\n");
		return stm32g031_scp_wdt_reset_by_sw(di);
	}

	return ret;
}

static int stm32g031_fcp_read_switch_status(void *dev_data)
{
	return 0;
}

static int stm32g031_is_fcp_charger_type(void *dev_data)
{
	u8 reg_val = 0;
	int ret;
	struct stm32g031_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -EPERM;
	}

	if (!stm32g031_is_support_fcp(di))
		return 0;

	ret = stm32g031_read_byte(di, STM32G031_SCP_STATUS_REG, &reg_val);
	if (ret)
		return 0;

	if (reg_val & STM32G031_SCP_STATUS_ENABLE_HAND_SUCCESS_MASK)
		return 1;

	return 0;
}

static int stm32g031_fcp_adapter_detect(struct stm32g031_device_info *di)
{
	u8 reg_val = 0;
	int i;
	int ret;

	mutex_lock(&di->scp_detect_lock);

	ret = stm32g031_fw_set_hw_config(di);
	if (ret && !di->is_low_power_mode) {
		stm32g031_hard_reset(di);
		ret = stm32g031_fw_set_hw_config(di);
	}

	power_usleep(DT_USLEEP_20MS);
	ret += stm32g031_fw_get_hw_config(di);
	if (ret) {
		hwlog_err("set hw config fail\n");
		mutex_unlock(&di->scp_detect_lock);
		return ADAPTER_DETECT_OTHER;
	}

	/* temp */
	ret = stm32g031_read_byte(di, STM32G031_SCP_STATUS_REG, &reg_val);
	if (ret) {
		hwlog_err("read det attach err, ret:%d\n", ret);
		mutex_unlock(&di->scp_detect_lock);
		return -EPERM;
	}

	/* confirm enable hand success status */
	if (reg_val & STM32G031_SCP_STATUS_ENABLE_HAND_SUCCESS_MASK) {
		mutex_unlock(&di->scp_detect_lock);
		hwlog_info("scp adapter detect ok\n");
		return ADAPTER_DETECT_SUCC;
	}

	ret = stm32g031_write_byte(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_DET_VAL);
	if (ret) {
		hwlog_err("SCP enable detect fail, ret is %d\n", ret);
		stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_EN_SCP_MASK,
			STM32G031_SCP_CTL_EN_SCP_SHIFT, 0);
		stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_SCP_DET_EN_MASK,
			STM32G031_SCP_CTL_SCP_DET_EN_SHIFT, 0);
		/* reset scp registers when EN_SCP is changed to 0 */
		stm32g031_scp_wdt_reset_by_sw(di);
		stm32g031_fcp_adapter_reset(di);
		mutex_unlock(&di->scp_detect_lock);
		return -EPERM;
	}

	/* waiting for scp set */
	for (i = 0; i < STM32G031_SCP_DETECT_MAX_COUT; i++) {
		ret = stm32g031_read_byte(di, STM32G031_SCP_STATUS_REG, &reg_val);
		if (ret) {
			hwlog_err("read det attach err, ret:%d\n", ret);
			continue;
		}

		hwlog_info("SCP_STATUS_REG=0x%x\n", reg_val);
		if (reg_val & STM32G031_SCP_STATUS_ENABLE_HAND_SUCCESS_MASK)
			break;
		power_msleep(DT_MSLEEP_200MS, 0, NULL);
	}

	if (i == STM32G031_SCP_DETECT_MAX_COUT) {
		stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_EN_SCP_MASK,
			STM32G031_SCP_CTL_EN_SCP_SHIFT, 0);
		stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_SCP_DET_EN_MASK,
			STM32G031_SCP_CTL_SCP_DET_EN_SHIFT, 0);
		/* reset scp registers when EN_SCP is changed to 0 */
		stm32g031_scp_wdt_reset_by_sw(di);
		stm32g031_fcp_adapter_reset(di);
		hwlog_err("CHG_SCP_ADAPTER_DETECT_OTHER return\n");
		mutex_unlock(&di->scp_detect_lock);
		return ADAPTER_DETECT_OTHER;
	}

	mutex_unlock(&di->scp_detect_lock);
	return ret;
}

static int stm32g031_fcp_stop_charge_config(void *dev_data)
{
	struct stm32g031_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -EPERM;
	}

	stm32g031_fcp_master_reset(di);
	stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_SCP_DET_EN_MASK,
		STM32G031_SCP_CTL_SCP_DET_EN_SHIFT, 0);

	return 0;
}

static int stm32g031_scp_reg_read(u8 *val, u8 reg, void *dev_data)
{
	int ret;

	if (g_stm32g031_scp_error_flag) {
		hwlog_err("scp timeout happened, do not read reg = 0x%x\n", reg);
		return -EPERM;
	}

	ret = stm32g031_scp_adapter_reg_read(val, reg, dev_data);
	if (ret) {
		hwlog_err("error reg = 0x%x\n", reg);
		if (reg != HWSCP_ADP_TYPE0)
			g_stm32g031_scp_error_flag = STM32G031_SCP_IS_ERR;

		return -EPERM;
	}

	return 0;
}

static int stm32g031_scp_reg_write(u8 val, u8 reg, void *dev_data)
{
	int ret;

	if (g_stm32g031_scp_error_flag) {
		hwlog_err("scp timeout happened, do not write reg = 0x%x\n", reg);
		return -EPERM;
	}

	ret = stm32g031_scp_adapter_reg_write(val, reg, dev_data);
	if (ret) {
		hwlog_err("error reg = 0x%x\n", reg);
		g_stm32g031_scp_error_flag = STM32G031_SCP_IS_ERR;
		return -EPERM;
	}

	return 0;
}

static int stm32g031_self_check(void *dev_data)
{
	return 0;
}

static int stm32g031_scp_chip_reset(void *dev_data)
{
	return stm32g031_fcp_master_reset(dev_data);
}

static int stm32g031_scp_reg_read_block(int reg, int *val, int num,
	void *dev_data)
{
	int ret;
	int i;
	u8 data = 0;

	if (!val || !dev_data) {
		hwlog_err("val or dev_data is null\n");
		return -EPERM;
	}

	g_stm32g031_scp_error_flag = STM32G031_SCP_NO_ERR;

	for (i = 0; i < num; i++) {
		ret = stm32g031_scp_reg_read(&data, reg + i, dev_data);
		if (ret) {
			hwlog_err("scp read failed, reg=0x%x\n", reg + i);
			return -EPERM;
		}
		val[i] = data;
	}

	return 0;
}

static int stm32g031_scp_reg_write_block(int reg, const int *val, int num,
	void *dev_data)
{
	int ret;
	int i;

	if (!val || !dev_data) {
		hwlog_err("val or dev_data is null\n");
		return -EPERM;
	}

	g_stm32g031_scp_error_flag = STM32G031_SCP_NO_ERR;

	for (i = 0; i < num; i++) {
		ret = stm32g031_scp_reg_write(val[i], reg + i, dev_data);
		if (ret) {
			hwlog_err("scp write failed, reg=0x%x\n", reg + i);
			return -EPERM;
		}
	}

	return 0;
}

static int stm32g031_scp_detect_adapter(void *dev_data)
{
	struct stm32g031_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -EPERM;
	}

	return stm32g031_fcp_adapter_detect(di);
}

static int stm32g031_fcp_reg_read_block(int reg, int *val, int num,
	void *dev_data)
{
	int ret, i;
	u8 data = 0;

	if (!val || !dev_data) {
		hwlog_err("val or dev_data is null\n");
		return -EPERM;
	}

	g_stm32g031_scp_error_flag = STM32G031_SCP_NO_ERR;

	for (i = 0; i < num; i++) {
		ret = stm32g031_scp_reg_read(&data, reg + i, dev_data);
		if (ret) {
			hwlog_err("fcp read failed, reg=0x%x\n", reg + i);
			return -EPERM;
		}
		val[i] = data;
	}
	return 0;
}

static int stm32g031_fcp_reg_write_block(int reg, const int *val, int num,
	void *dev_data)
{
	int ret, i;

	if (!val || !dev_data) {
		hwlog_err("val or dev_data is null\n");
		return -EPERM;
	}

	g_stm32g031_scp_error_flag = STM32G031_SCP_NO_ERR;

	for (i = 0; i < num; i++) {
		ret = stm32g031_scp_reg_write(val[i], reg + i, dev_data);
		if (ret) {
			hwlog_err("fcp write failed, reg=0x%x\n", reg + i);
			return -EPERM;
		}
	}

	return 0;
}

static int stm32g031_fcp_detect_adapter(void *dev_data)
{
	struct stm32g031_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -EPERM;
	}

	return stm32g031_fcp_adapter_detect(di);
}

static int stm32g031_pre_init(void *dev_data)
{
	struct stm32g031_device_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -EPERM;
	}

	return stm32g031_self_check(di);
}

static int stm32g031_scp_adapter_reset(void *dev_data)
{
	return stm32g031_fcp_adapter_reset(dev_data);
}

static struct hwscp_ops stm32g031_hwscp_ops = {
	.chip_name = "stm32g031",
	.reg_read = stm32g031_scp_reg_read_block,
	.reg_write = stm32g031_scp_reg_write_block,
	.reg_multi_read = stm32g031_scp_adapter_reg_multi_read,
	.detect_adapter = stm32g031_scp_detect_adapter,
	.soft_reset_master = stm32g031_scp_chip_reset,
	.soft_reset_slave = stm32g031_scp_adapter_reset,
	.pre_init = stm32g031_pre_init,
};

static struct hwfcp_ops stm32g031_hwfcp_ops = {
	.chip_name = "stm32g031",
	.reg_read = stm32g031_fcp_reg_read_block,
	.reg_write = stm32g031_fcp_reg_write_block,
	.detect_adapter = stm32g031_fcp_detect_adapter,
	.soft_reset_master = stm32g031_fcp_master_reset,
	.soft_reset_slave = stm32g031_fcp_adapter_reset,
	.get_master_status = stm32g031_fcp_read_switch_status,
	.stop_charging_config = stm32g031_fcp_stop_charge_config,
	.is_accp_charger_type = stm32g031_is_fcp_charger_type,
};

int stm32g031_hwscp_register(struct stm32g031_device_info *di)
{
	stm32g031_hwscp_ops.dev_data = (void *)di;
	return hwscp_ops_register(&stm32g031_hwscp_ops);
}

int stm32g031_hwfcp_register(struct stm32g031_device_info *di)
{
	stm32g031_hwfcp_ops.dev_data = (void *)di;
	return hwfcp_ops_register(&stm32g031_hwfcp_ops);
}
