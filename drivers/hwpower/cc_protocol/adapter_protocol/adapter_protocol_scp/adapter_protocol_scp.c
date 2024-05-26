// SPDX-License-Identifier: GPL-2.0
/*
 * adapter_protocol_scp.c
 *
 * scp protocol driver
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <chipset_common/hwpower/protocol/adapter_protocol.h>
#include <chipset_common/hwpower/protocol/adapter_protocol_scp.h>
#include <chipset_common/hwpower/protocol/adapter_protocol_scp_auth.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG scp_protocol
HWLOG_REGIST();

static struct hwscp_dev *g_hwscp_dev;

static const struct adapter_protocol_device_data g_hwscp_dev_data[] = {
	{ PROTOCOL_DEVICE_ID_FSA9685, "fsa9685" },
	{ PROTOCOL_DEVICE_ID_RT8979, "rt8979" },
	{ PROTOCOL_DEVICE_ID_SCHARGER_V300, "scharger_v300" },
	{ PROTOCOL_DEVICE_ID_SCHARGER_V600, "scharger_v600" },
	{ PROTOCOL_DEVICE_ID_FUSB3601, "fusb3601" },
	{ PROTOCOL_DEVICE_ID_SM5450, "sm5450" },
	{ PROTOCOL_DEVICE_ID_HL7139, "hl7139" },
	{ PROTOCOL_DEVICE_ID_SC8545, "sc8545" },
	{ PROTOCOL_DEVICE_ID_SC8562, "sc8562" },
	{ PROTOCOL_DEVICE_ID_SC8546, "sc8546" },
	{ PROTOCOL_DEVICE_ID_CPS2021, "cps2021" },
	{ PROTOCOL_DEVICE_ID_SC200X, "sc200x" },
	{ PROTOCOL_DEVICE_ID_STM32G031, "stm32g031" },
	{ PROTOCOL_DEVICE_ID_HC32L110, "hc32l110" },
};

/* power: 10^n */
static int g_hwscp_ten_power[] = {
	[0] = 1,
	[1] = 10,
	[2] = 100,
	[3] = 1000,
};

static int hwscp_get_device_id(const char *str)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_hwscp_dev_data); i++) {
		if (!strncmp(str, g_hwscp_dev_data[i].name,
			strlen(str)))
			return g_hwscp_dev_data[i].id;
	}

	return -EPERM;
}

static struct hwscp_dev *hwscp_get_dev(void)
{
	if (!g_hwscp_dev) {
		hwlog_err("g_hwscp_dev is null\n");
		return NULL;
	}

	return g_hwscp_dev;
}

static struct hwscp_ops *hwscp_get_ops(void)
{
	if (!g_hwscp_dev || !g_hwscp_dev->p_ops) {
		hwlog_err("g_hwscp_dev or p_ops is null\n");
		return NULL;
	}

	return g_hwscp_dev->p_ops;
}

int hwscp_ops_register(struct hwscp_ops *ops)
{
	int dev_id;

	if (!g_hwscp_dev || !ops || !ops->chip_name) {
		hwlog_err("g_hwscp_dev or ops or chip_name is null\n");
		return -EPERM;
	}

	dev_id = hwscp_get_device_id(ops->chip_name);
	if (dev_id < 0) {
		hwlog_err("%s ops register fail\n", ops->chip_name);
		return -EPERM;
	}

	g_hwscp_dev->p_ops = ops;
	g_hwscp_dev->dev_id = dev_id;

	hwlog_info("%d:%s ops register ok\n", dev_id, ops->chip_name);
	return 0;
}

static int hwscp_check_trans_num(int num)
{
	/* num must be less equal than 16 */
	if ((num >= BYTE_ONE) && (num <= BYTE_SIXTEEN)) {
		/* num must be 1 or even numbers */
		if ((num > 1) && (num % 2 == 1))
			return -EPERM;

		return 0;
	}

	return -EPERM;
}

static int hwscp_get_rw_error_flag(void)
{
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev)
		return -EPERM;

	return l_dev->info.rw_error_flag;
}

static void hwscp_set_rw_error_flag(int flag)
{
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev)
		return;

	hwlog_info("set_rw_error_flag: %d\n", flag);
	l_dev->info.rw_error_flag = flag;
}

int hwscp_get_reg80_rw_error_flag(void)
{
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev)
		return 0;

	return l_dev->info.reg80_rw_error_flag;
}

static void hwscp_set_reg80_rw_error_flag(int flag)
{
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev)
		return;

	hwlog_info("set_reg80_rw_error_flag: %d\n", flag);
	l_dev->info.reg80_rw_error_flag = flag;
}

static int hwscp_get_reg7e_rw_error_flag(void)
{
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev)
		return 0;

	return l_dev->info.reg7e_rw_error_flag;
}

static void hwscp_set_reg7e_rw_error_flag(int flag)
{
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev)
		return;

	hwlog_info("set_reg7e_rw_error_flag: %d\n", flag);
	l_dev->info.reg7e_rw_error_flag = flag;
}

static int hwscp_reg_read(int reg, int *val, int num)
{
	int ret;
	struct hwscp_ops *l_ops = NULL;

	if (hwscp_get_rw_error_flag())
		return -EPERM;

	l_ops = hwscp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->reg_read) {
		hwlog_err("reg_read is null\n");
		return -EPERM;
	}

	if (hwscp_check_trans_num(num)) {
		hwlog_err("invalid num=%d\n", num);
		return -EPERM;
	}

	ret = l_ops->reg_read(reg, val, num, l_ops->dev_data);
	if (ret < 0) {
		if ((reg != HWSCP_ADP_TYPE0) &&
			(reg != HWSCP_POWER_CURVE_NUM))
			hwscp_set_rw_error_flag(RW_ERROR_FLAG);

		hwlog_err("reg 0x%x read fail\n", reg);
		return -EPERM;
	}

	return 0;
}

/* adapter must support multi read */
static int hwscp_reg_multi_read(int reg, int *val, int num)
{
	int i;
	int ret;
	u8 value[BYTE_SIXTEEN] = { 0 };
	struct hwscp_ops *l_ops = NULL;

	if (hwscp_get_rw_error_flag())
		return -EPERM;

	l_ops = hwscp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->reg_multi_read) {
		hwlog_info("not support reg_multi_read\n");
		return hwscp_reg_read(reg, val, num);
	}

	if (hwscp_check_trans_num(num)) {
		hwlog_err("invalid num=%d\n", num);
		return -EPERM;
	}

	ret = l_ops->reg_multi_read((u8)reg, value, (u8)num, l_ops->dev_data);
	if (ret) {
		if (reg != HWSCP_ADP_TYPE0)
			hwscp_set_rw_error_flag(RW_ERROR_FLAG);
		hwlog_err("reg 0x%x multi read fail\n", reg);
		return -EPERM;
	}

	for (i = 0; i < num; i++)
		val[i] = value[i];

	return 0;
}

static int hwscp_reg_write(int reg, const int *val, int num)
{
	int ret;
	struct hwscp_ops *l_ops = NULL;

	if (hwscp_get_rw_error_flag())
		return -EPERM;

	l_ops = hwscp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->reg_write) {
		hwlog_err("reg_write is null\n");
		return -EPERM;
	}

	if (hwscp_check_trans_num(num)) {
		hwlog_err("invalid num=%d\n", num);
		return -EPERM;
	}

	ret = l_ops->reg_write(reg, val, num, l_ops->dev_data);
	if (ret < 0) {
		if (reg != HWSCP_ADP_TYPE0)
			hwscp_set_rw_error_flag(RW_ERROR_FLAG);

		hwlog_err("reg 0x%x write fail\n", reg);
		return -EPERM;
	}

	return 0;
}

static int hwscp_detect_adapter(void)
{
	struct hwscp_ops *l_ops = NULL;

	l_ops = hwscp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->detect_adapter) {
		hwlog_err("detect_adapter is null\n");
		return -EPERM;
	}

	hwlog_info("detect_adapter\n");

	return l_ops->detect_adapter(l_ops->dev_data);
}

static int hwscp_soft_reset_master(void)
{
	struct hwscp_ops *l_ops = NULL;

	l_ops = hwscp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->soft_reset_master) {
		hwlog_err("soft_reset_master is null\n");
		return -EPERM;
	}

	hwlog_info("soft_reset_master\n");

	return l_ops->soft_reset_master(l_ops->dev_data);
}

static int hwscp_soft_reset_slave(void)
{
	struct hwscp_ops *l_ops = NULL;

	l_ops = hwscp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->soft_reset_slave) {
		hwlog_err("soft_reset_slave is null\n");
		return -EPERM;
	}

	hwlog_info("soft_reset_slave\n");

	return l_ops->soft_reset_slave(l_ops->dev_data);
}

static int hwscp_process_pre_init(void)
{
	struct hwscp_ops *l_ops = NULL;

	l_ops = hwscp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->pre_init)
		return 0;

	hwlog_info("process_pre_init\n");

	return l_ops->pre_init(l_ops->dev_data);
}

static int hwscp_process_post_init(void)
{
	struct hwscp_ops *l_ops = NULL;

	l_ops = hwscp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->post_init)
		return 0;

	hwlog_info("process_post_init\n");

	return l_ops->post_init(l_ops->dev_data);
}

static int hwscp_process_pre_exit(void)
{
	struct hwscp_ops *l_ops = NULL;

	l_ops = hwscp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->pre_exit)
		return 0;

	hwlog_info("process_pre_exit\n");

	return l_ops->pre_exit(l_ops->dev_data);
}

static int hwscp_process_post_exit(void)
{
	struct hwscp_ops *l_ops = NULL;

	l_ops = hwscp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->post_exit)
		return 0;

	hwlog_info("process_post_exit\n");

	return l_ops->post_exit(l_ops->dev_data);
}

static int hwscp_set_default_param(void)
{
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev)
		return -EPERM;

	memset(&l_dev->info, 0, sizeof(l_dev->info));
	memset(l_dev->encrypt_random_host, 0,
		sizeof(l_dev->encrypt_random_host));
	memset(l_dev->encrypt_random_slave, 0,
		sizeof(l_dev->encrypt_random_slave));
	memset(l_dev->encrypt_hash_slave, 0,
		sizeof(l_dev->encrypt_hash_slave));

	return 0;
}

static int hwscp_get_vendor_id(int *id)
{
	int value[BYTE_TWO] = { 0 };
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev || !id)
		return -EPERM;

	if (l_dev->info.vid_rd_flag == HAS_READ_FLAG) {
		*id = (l_dev->info.vid_h << POWER_BITS_PER_BYTE) |
			l_dev->info.vid_l;
		hwlog_info("get_vendor_id_a: 0x%x\n", *id);
		return 0;
	}

	/* reg: 0x82 & 0x83 */
	if (hwscp_reg_read(HWSCP_VENDOR_ID_HBYTE, value, BYTE_TWO))
		return -EPERM;

	*id = (value[0] << POWER_BITS_PER_BYTE) | value[1];
	l_dev->info.vid_rd_flag = HAS_READ_FLAG;
	l_dev->info.vid_h = value[0];
	l_dev->info.vid_l = value[1];

	hwlog_info("get_vendor_id_f: 0x%x\n", *id);
	return 0;
}

static int hwscp_get_module_id(int *id)
{
	int value[BYTE_TWO] = { 0 };
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev || !id)
		return -EPERM;

	if (l_dev->info.mid_rd_flag == HAS_READ_FLAG) {
		*id = (l_dev->info.mid_h << POWER_BITS_PER_BYTE) |
			l_dev->info.mid_l;
		hwlog_info("get_module_id_a: 0x%x\n", *id);
		return 0;
	}

	/* reg: 0x84 & 0x85 */
	if (hwscp_reg_read(HWSCP_MODULE_ID_HBYTE, value, BYTE_TWO))
		return -EPERM;

	*id = (value[0] << POWER_BITS_PER_BYTE) | value[1];
	l_dev->info.mid_rd_flag = HAS_READ_FLAG;
	l_dev->info.mid_h = value[0];
	l_dev->info.mid_l = value[1];

	hwlog_info("get_module_id_f: 0x%x\n", *id);
	return 0;
}

static int hwscp_get_serial_no_id(int *id)
{
	int value[BYTE_TWO] = { 0 };
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev || !id)
		return -EPERM;

	if (l_dev->info.serial_rd_flag == HAS_READ_FLAG) {
		*id = (l_dev->info.serial_h << POWER_BITS_PER_BYTE) |
			l_dev->info.serial_l;
		hwlog_info("get_serial_no_id_a: 0x%x\n", *id);
		return 0;
	}

	/* reg: 0x86 & 0x87 */
	if (hwscp_reg_read(HWSCP_SERIAL_NO_HBYTE, value, BYTE_TWO))
		return -EPERM;

	*id = (value[0] << POWER_BITS_PER_BYTE) | value[1];
	l_dev->info.serial_rd_flag = HAS_READ_FLAG;
	l_dev->info.serial_h = value[0];
	l_dev->info.serial_l = value[1];

	hwlog_info("get_serial_no_id_f: 0x%x\n", *id);
	return 0;
}

static int hwscp_get_chip_id(int *id)
{
	int value[BYTE_ONE] = { 0 };
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev || !id)
		return -EPERM;

	if (l_dev->info.chip_id_rd_flag == HAS_READ_FLAG) {
		*id = l_dev->info.chip_id;
		hwlog_info("get_chip_id_a: 0x%x\n", *id);
		return 0;
	}

	/* reg: 0x88 */
	if (hwscp_reg_read(HWSCP_CHIP_ID, value, BYTE_ONE))
		return -EPERM;

	*id = value[0];
	l_dev->info.chip_id_rd_flag = HAS_READ_FLAG;
	l_dev->info.chip_id = value[0];

	hwlog_info("get_chip_id_f: 0x%x\n", *id);
	return 0;
}

static int hwscp_get_hw_version_id(int *id)
{
	int value[BYTE_ONE] = { 0 };
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev || !id)
		return -EPERM;

	if (l_dev->info.hwver_rd_flag == HAS_READ_FLAG) {
		*id = l_dev->info.hwver;
		hwlog_info("get_hw_version_id_a: %x\n", *id);
		return 0;
	}

	/* reg: 0x89 */
	if (hwscp_reg_read(HWSCP_HWVER, value, BYTE_ONE))
		return -EPERM;

	*id = value[0];
	l_dev->info.hwver_rd_flag = HAS_READ_FLAG;
	l_dev->info.hwver = value[0];

	hwlog_info("get_hw_version_id_f: 0x%x\n", *id);
	return 0;
}

static int hwscp_get_fw_version_id(int *id)
{
	int value[BYTE_TWO] = { 0 };
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev || !id)
		return -EPERM;

	if (l_dev->info.fwver_rd_flag == HAS_READ_FLAG) {
		*id = (l_dev->info.fwver_h << POWER_BITS_PER_BYTE) |
			l_dev->info.fwver_l;
		hwlog_info("get_fw_version_id_a: 0x%x\n", *id);
		return 0;
	}

	/* reg: 0x8a & 0x8b */
	if (hwscp_reg_read(HWSCP_FWVER_HBYTE, value, BYTE_TWO))
		return -EPERM;

	*id = (value[0] << POWER_BITS_PER_BYTE) | value[1];
	l_dev->info.fwver_rd_flag = HAS_READ_FLAG;
	l_dev->info.fwver_h = value[0];
	l_dev->info.fwver_l = value[1];

	hwlog_info("get_fw_version_id_f: 0x%x\n", *id);
	return 0;
}

static int hwscp_check_adp_type(int mode, int value)
{
	if (mode & ADAPTER_SUPPORT_LVC) {
		switch (value) {
		/* check 5v4.5a */
		case HWSCP_ADP_B_TYPE1_25W_IWATT:
		case HWSCP_ADP_B_TYPE1_25W_RICH1:
		case HWSCP_ADP_B_TYPE1_25W_RICH2:
			return ADAPTER_TYPE_5V4P5A;
		case HWSCP_ADP_B_TYPE1_22P5W_BANK:
			return ADAPTER_TYPE_5V4P5A_BANK;
		default:
			break;
		}
	}

	if ((mode & ADAPTER_SUPPORT_LVC) && (mode & ADAPTER_SUPPORT_SC)) {
		switch (value) {
		/* check 10v4a */
		case HWSCP_ADP_B_TYPE1_40W:
		case HWSCP_ADP_B_TYPE1_40W_1:
		case HWSCP_ADP_B_TYPE1_40W_2:
		case HWSCP_ADP_B_TYPE1_NR_40W:
		case HWSCP_ADP_B_TYPE1_NR_40W_1:
		case HWSCP_ADP_B_TYPE1_NR_40W_2:
		case HWSCP_ADP_B_TYPE1_NR_40W_3:
		case HWSCP_ADP_B_TYPE1_PS_C_40W:
		case HWSCP_ADP_B_TYPE1_PS_C_40W_1:
		case HWSCP_ADP_B_TYPE1_PS_A_40W:
			return ADAPTER_TYPE_10V4A;
		/* check 20v3.25a-max */
		case HWSCP_ADP_B_TYPE1_65W_MAX:
			return ADAPTER_TYPE_20V3P25A_MAX;
		/* check 10v4a-bank */
		case HWSCP_ADP_B_TYPE1_40W_BANK:
			return ADAPTER_TYPE_10V4A_BANK;
		/* check 10v4a-car */
		case HWSCP_ADP_B_TYPE1_40W_CAR:
			return ADAPTER_TYPE_10V4A_CAR;
		/* check 11v6a */
		case HWSCP_ADP_B_TYPE1_66W:
		case HWSCP_ADP_B_TYPE1_MJR_66W:
		case HWSCP_ADP_B_TYPE1_MJR_66W_1:
			return ADAPTER_TYPE_11V6A;
		case HWSCP_ADP_B_TYPE1_66W_CAR:
		case HWSCP_ADP_B_TYPE1_66W_CAR_1:
			return ADAPTER_TYPE_11V6A_CAR;
		case HWSCP_ADP_B_TYPE1_66W_BANK:
			return ADAPTER_TYPE_11V6A_BANK;
		case HWSCP_ADP_B_TYPE1_QTR_C_60W:
			return ADAPTER_TYPE_QTR_C_20V3A;
		case HWSCP_ADP_B_TYPE1_QTR_C_40W:
			return ADAPTER_TYPE_QTR_C_10V4A;
		case HWSCP_ADP_B_TYPE1_QTR_A_40W:
			return ADAPTER_TYPE_QTR_A_10V4A;
		case HWSCP_ADP_B_TYPE1_QTR_A_22P5W:
			return ADAPTER_TYPE_QTR_A_10V2P25A;
		case HWSCP_ADP_B_TYPE1_HPR_A_22P5W:
			return ADAPTER_TYPE_HPR_A_10V2P25A;
		case HWSCP_ADP_B_TYPE1_JLR_135W:
			return ADAPTER_TYPE_JLR_20V6P7A;
		case HWSCP_ADP_B_TYPE1_YLR_100W_CAR:
			return ADAPTER_TYPE_YLR_20V5A_CAR;
		default:
			break;
		}
	}

	if (mode & ADAPTER_SUPPORT_SC) {
		switch (value) {
		/* check 10v2a */
		case HWSCP_ADP_B_TYPE1_20W:
			return ADAPTER_TYPE_10V2A;
		/* check 20v3.25a */
		case HWSCP_ADP_B_TYPE1_65W:
		case HWSCP_ADP_B_TYPE1_65W_1:
		case HWSCP_ADP_B_TYPE1_65W_2:
		case HWSCP_ADP_B_TYPE1_XR_65W_PC:
		case HWSCP_ADP_B_TYPE1_PS_C_65W:
			return ADAPTER_TYPE_20V3P25A;
		/* check 10v2.25a */
		case HWSCP_ADP_B_TYPE1_22P5W:
		case HWSCP_ADP_B_TYPE1_PS_C_22P5W:
		case HWSCP_ADP_B_TYPE1_PS_A_22P5W:
		case HWSCP_ADP_B_TYPE1_HPR_A_22P5W_1:
			return ADAPTER_TYPE_10V2P25A;
		/* check 10v2.25a-bank */
		case HWSCP_ADP_B_TYPE1_22P5W_BANK_1:
			return ADAPTER_TYPE_10V2P25A_BANK;
		/* check 10v2.25a-car */
		case HWSCP_ADP_B_TYPE1_22P5W_CAR:
		case HWSCP_ADP_B_TYPE1_22P5W_CAR_1:
			return ADAPTER_TYPE_10V2P25A_CAR;
		case HWSCP_ADP_B_TYPE1_HPR_C_66W:
		case HWSCP_ADP_B_TYPE1_HPR_C_66W_1:
			return ADAPTER_TYPE_HPR_C_11V6A;
		case HWSCP_ADP_B_TYPE1_HPR_C_40W:
		case HWSCP_ADP_B_TYPE1_HPR_C_40W_1:
			return ADAPTER_TYPE_HPR_C_10V4A;
		case HWSCP_ADP_B_TYPE1_HPR_A_66W:
		case HWSCP_ADP_B_TYPE1_HPR_A_66W_1:
			return ADAPTER_TYPE_HPR_A_11V6A;
		case HWSCP_ADP_B_TYPE1_HPR_A_40W:
		case HWSCP_ADP_B_TYPE1_HPR_A_40W_1:
			return ADAPTER_TYPE_HPR_A_10V4A;
		case HWSCP_ADP_B_TYPE1_HHR_90W:
		case HWSCP_ADP_B_TYPE1_HHR_90W_1:
			return ADAPTER_TYPE_HHR_20V4P5A;
		case HWSCP_ADP_B_TYPE1_FCR_66W:
		case HWSCP_ADP_B_TYPE1_FCR_66W_1:
			return ADAPTER_TYPE_FCR_C_11V6A;
		default:
			break;
		}
	}

	return ADAPTER_TYPE_UNKNOWN;
}

static int hwscp_get_adp_type(int *type)
{
	int mode;
	int value[BYTE_ONE] = { 0 };
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev || !type)
		return -EPERM;

	if (l_dev->info.adp_type_rd_flag == HAS_READ_FLAG) {
		*type = l_dev->info.adp_type;
		hwlog_info("get_adp_type_a: %d\n", *type);
		return 0;
	}

	/* reg: 0x8d */
	if (hwscp_reg_read(HWSCP_ADP_B_TYPE1, value, BYTE_ONE))
		return -EPERM;

	mode = l_dev->info.support_mode;
	*type = hwscp_check_adp_type(mode, value[0]);
	l_dev->info.adp_type = *type;
	l_dev->info.adp_type_rd_flag = HAS_READ_FLAG;

	hwlog_info("get_adp_type_f: %d,%d\n", value[0], *type);
	return 0;
}

static int hwscp_get_adp_code(int *code)
{
	int value[BYTE_ONE] = { 0 };
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev || !code)
		return -EPERM;

	if (l_dev->info.adp_code_rd_flag == HAS_READ_FLAG) {
		*code = l_dev->info.adp_code;
		hwlog_info("get_adp_code_a: %d\n", *code);
		return 0;
	}

	/* reg: 0x8d */
	if (hwscp_reg_read(HWSCP_ADP_B_TYPE1, value, BYTE_ONE))
		return -EPERM;

	l_dev->info.adp_code = value[0];
	*code = value[0];
	l_dev->info.adp_code_rd_flag = HAS_READ_FLAG;

	hwlog_info("get_adp_code_f: %d\n", value[0]);
	return 0;
}

static int hwscp_get_power_drop_info(int *drop_flag, int *drop_ratio)
{
	int value[BYTE_ONE] = { 0 };
	int value_a;
	int value_b;

	/* reg: 0xa5 */
	if (hwscp_reg_read(HWSCP_SSTS, value, BYTE_ONE))
		return -EPERM;

	value_a = (value[0] & HWSCP_SSTS_DPARTO_MASK) >> HWSCP_SSTS_DPARTO_SHIFT;
	value_b = (value[0] & HWSCP_SSTS_DROP_MASK) >> HWSCP_SSTS_DROP_SHIFT;
	*drop_flag = value_b;
	*drop_ratio = value_a;

	hwlog_info("get_power_drop_info: %d,%d\n", *drop_flag, *drop_ratio);
	return 0;
}

static int hwscp_get_min_voltage(int *volt)
{
	int value[BYTE_ONE] = { 0 };
	int value_a;
	int value_b;
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev || !volt)
		return -EPERM;

	if (l_dev->info.min_volt_rd_flag == HAS_READ_FLAG) {
		*volt = l_dev->info.min_volt;
		hwlog_info("get_min_voltage_a: %d\n", *volt);
		return 0;
	}

	/* reg: 0x92 */
	if (hwscp_reg_read(HWSCP_MIN_VOUT, value, BYTE_ONE))
		return -EPERM;

	hwlog_info("min_vout[%x]=%x\n", HWSCP_MIN_VOUT, value[0]);

	value_a = (value[0] & HWSCP_VOUT_A_MASK) >> HWSCP_VOUT_A_SHIFT;
	value_b = (value[0] & HWSCP_VOUT_B_MASK) >> HWSCP_VOUT_B_SHIFT;

	if (value_a < HWSCP_VOUT_A_0 || value_a > HWSCP_VOUT_A_3) {
		hwlog_err("invalid value_a=%d\n", value_a);
		return -EPERM;
	}

	*volt = g_hwscp_ten_power[value_a] * value_b;
	l_dev->info.min_volt_rd_flag = HAS_READ_FLAG;
	l_dev->info.min_volt = *volt;

	hwlog_info("get_min_voltage_f: %d\n", *volt);
	return 0;
}

static int hwscp_get_max_voltage(int *volt)
{
	int value[BYTE_ONE] = { 0 };
	int value_a;
	int value_b;
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev || !volt)
		return -EPERM;

	if (l_dev->info.max_volt_rd_flag == HAS_READ_FLAG) {
		*volt = l_dev->info.max_volt;
		hwlog_info("get_max_voltage_a: %d\n", *volt);
		return 0;
	}

	/* reg: 0x93 */
	if (hwscp_reg_read(HWSCP_MAX_VOUT, value, BYTE_ONE))
		return -EPERM;

	hwlog_info("max_vout[%x]=%x\n", HWSCP_MAX_VOUT, value[0]);

	value_a = (value[0] & HWSCP_VOUT_A_MASK) >> HWSCP_VOUT_A_SHIFT;
	value_b = (value[0] & HWSCP_VOUT_B_MASK) >> HWSCP_VOUT_B_SHIFT;

	if (value_a < HWSCP_VOUT_A_0 || value_a > HWSCP_VOUT_A_3) {
		hwlog_err("invalid value_a=%d\n", value_a);
		return -EPERM;
	}

	*volt = g_hwscp_ten_power[value_a] * value_b;
	l_dev->info.max_volt_rd_flag = HAS_READ_FLAG;
	l_dev->info.max_volt = *volt;

	hwlog_info("get_max_voltage_f: %d\n", *volt);
	return 0;
}

static int hwscp_get_min_current(int *cur)
{
	int value[BYTE_ONE] = { 0 };
	int value_a;
	int value_b;
	int drop_flag;
	int drop_ratio;
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev || !cur)
		return -EPERM;

	if (l_dev->info.min_cur_rd_flag == HAS_READ_FLAG) {
		*cur = l_dev->info.min_cur;
		hwlog_info("get_min_current_a: %d\n", *cur);
		return 0;
	}

	if (hwscp_get_power_drop_info(&drop_flag, &drop_ratio))
		return -EPERM;

	/* reg: 0x94 */
	if (hwscp_reg_read(HWSCP_MIN_IOUT, value, BYTE_ONE))
		return -EPERM;

	hwlog_info("min_iout[%x]=%x\n", HWSCP_MIN_IOUT, value[0]);

	value_a = (value[0] & HWSCP_IOUT_A_MASK) >> HWSCP_IOUT_A_SHIFT;
	value_b = (value[0] & HWSCP_IOUT_B_MASK) >> HWSCP_IOUT_B_SHIFT;

	if (value_a < HWSCP_IOUT_A_0 || value_a > HWSCP_IOUT_A_3) {
		hwlog_err("invalid value_a=%d\n", value_a);
		return -EPERM;
	}

	if (drop_flag == HWSCP_DROP_ENABLE)
		*cur = g_hwscp_ten_power[value_a] * value_b * drop_ratio / HWSCP_DROP_FACTOR;
	else
		*cur = g_hwscp_ten_power[value_a] * value_b;

	l_dev->info.min_cur_rd_flag = HAS_READ_FLAG;
	l_dev->info.min_cur = *cur;

	hwlog_info("get_min_current_f: %d\n", *cur);
	return 0;
}

static int hwscp_get_max_current_accuracy_err(int *ierr)
{
	int value_a;
	int value_b;
	int value[BYTE_ONE] = { 0 };
	struct hwscp_dev *l_dev = hwscp_get_dev();

	if (!l_dev || !ierr)
		 return -EPERM;

	*ierr = 0;

	if (hwscp_reg_read(HWSCP_MAX_IEER, value, BYTE_ONE))
		return 0;

	value_a = (value[0] & HWSCP_IERR_A_MASK) >> HWSCP_IERR_A_SHIFT;
	value_b = (value[0] & HWSCP_IERR_B_MASK) >> HWSCP_IOUT_B_SHIFT;
	*ierr = g_hwscp_ten_power[value_a] * value_b;

	l_dev->info.max_ierr = *ierr;

	hwlog_info("get_value[0]: %d, get_max_ierr: %d\n", value[0], *ierr);
	return 0;
}

static int hwscp_get_extend_current(void)
{
	int value[BYTE_ONE] = { 0 };

	if (!hwscp_reg_read(HWSCP_MAX_IOUT_EXTEND, value, BYTE_ONE))
		return g_hwscp_ten_power[1] * value[0]; /* 1: 10^1 */

	return 0;
}

static int hwscp_get_max_current(int *cur)
{
	int value[BYTE_ONE] = { 0 };
	int value_a;
	int value_b;
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev || !cur)
		return -EPERM;

	if (l_dev->info.max_cur_rd_flag == HAS_READ_FLAG) {
		*cur = l_dev->info.max_cur;
		hwlog_info("get_max_current_a: %d\n", *cur);
		return 0;
	}

	/* reg: 0x95 */
	if (hwscp_reg_read(HWSCP_MAX_IOUT, value, BYTE_ONE))
		return -EPERM;

	hwlog_info("max_iout[%x]=%x\n", HWSCP_MAX_IOUT, value[0]);

	value_a = (value[0] & HWSCP_IOUT_A_MASK) >> HWSCP_IOUT_A_SHIFT;
	value_b = (value[0] & HWSCP_IOUT_B_MASK) >> HWSCP_IOUT_B_SHIFT;

	if (value_a < HWSCP_IOUT_A_0 ||
		value_a > HWSCP_IOUT_A_3) {
		hwlog_err("invalid value_a=%d\n", value_a);
		return -EPERM;
	}

	*cur = g_hwscp_ten_power[value_a] * value_b;
	if (*cur >= HWSCP_MAX_IOUT_EXTEND_TH)
		*cur += hwscp_get_extend_current();

	l_dev->info.max_cur_rd_flag = HAS_READ_FLAG;
	l_dev->info.max_cur = *cur;

	hwlog_info("get_max_current_f: %d\n", *cur);
	return 0;
}

static int hwscp_get_power_drop_current(int *cur)
{
	int drop_flag;
	int drop_ratio;

	if (!cur)
		return -EPERM;

	if (hwscp_get_power_drop_info(&drop_flag, &drop_ratio))
		return -EPERM;

	if ((drop_flag == HWSCP_DROP_ENABLE) && (drop_ratio > 0)) {
		*cur = *cur * drop_ratio / HWSCP_DROP_FACTOR;
		hwlog_info("get power_drop_current=%d\n", *cur);
	}
	return 0;
}

static int hwscp_get_power_curve_num(int *num)
{
	int value[BYTE_ONE] = { 0 };

	/* check reg 0x7e read result */
	if (hwscp_get_reg7e_rw_error_flag())
		return -EPERM;

	/* reg: 0x8f */
	if (hwscp_reg_read(HWSCP_POWER_CURVE_NUM, value, BYTE_ONE))
		return -EPERM;

	*num = value[0];
	hwlog_info("get power_curve_num=%d\n", value[0]);
	return 0;
}

static int hwscp_get_power_curve(int *val, unsigned int size)
{
	unsigned int i;
	int num;
	int value[HWSCP_POWER_CURVE_SIZE] = { 0 };

	if (!val || (size > HWSCP_POWER_CURVE_SIZE))
		return -EPERM;

	num = (size < BYTE_SIXTEEN) ? size : BYTE_SIXTEEN;
	if (hwscp_reg_multi_read(HWSCP_POWER_CURVE_BASE0, value, num))
		return -EPERM;

	if (size > BYTE_SIXTEEN) {
		num = size - BYTE_SIXTEEN;
		if (hwscp_reg_multi_read(HWSCP_POWER_CURVE_BASE1, &value[BYTE_SIXTEEN], num))
			return -EPERM;
	}

	for (i = 0; i < size; i++) {
		if (i % 2 == 0)
			val[i] = value[i] * HWSCP_POWER_CURVE_VOLT_STEP;
		else
			val[i] = value[i] * HWSCP_POWER_CURVE_CUR_STEP;
	}

	return 0;
}

static void hwscp_detect_undetach_cable(unsigned int value)
{
	struct hwscp_dev *l_dev = hwscp_get_dev();

	if (!l_dev)
		return;

	l_dev->info.is_undetach_cable = value & HWSCP_UNDETACH_CABLE;
}

static void hwscp_detect_port_type(unsigned int value)
{
	struct hwscp_dev *l_dev = hwscp_get_dev();

	if (!l_dev)
		return;

	l_dev->info.port_type = value & HWSCP_ADP_TYPE0_PORT_MASK;
}

static unsigned int hwscp_is_cport_adp(void)
{
	struct hwscp_dev *l_dev = hwscp_get_dev();

	if (!l_dev)
		return false;

	return l_dev->info.port_type == HWSCP_ADP_PORT_C;
}

static int hwscp_detect_adapter_support_mode_by_0x7e(int *mode)
{
	int value[BYTE_ONE] = { 0 };
	int ret = SCP_DETECT_FAIL;

	hwscp_set_reg7e_rw_error_flag(NO_RW_ERROR_FLAG);

	/* reg: 0x7e */
	if (hwscp_reg_read(HWSCP_ADP_TYPE0, value, BYTE_ONE)) {
		hwscp_set_reg7e_rw_error_flag(RW_ERROR_FLAG);
		hwlog_err("read adp_type0(0x7e) fail\n");
		return ret;
	}

	hwlog_info("adp_type0[%x]=%x\n", HWSCP_ADP_TYPE0, value[0]);

	if (value[0] & HWSCP_ADP_TYPE0_AB_MASK) {
		/* b type: regulable high voltage adapter */
		if (value[0] & HWSCP_ADP_TYPE0_B_SC_MASK) {
			*mode |= ADAPTER_SUPPORT_SC;
			ret = SCP_DETECT_SUCC;
		}

		/* b type: regulable low voltage adapter */
		if (!(value[0] & HWSCP_ADP_TYPE0_B_LVC_MASK)) {
			*mode |= ADAPTER_SUPPORT_LVC;
			ret = SCP_DETECT_SUCC;
		}

		/* detect undetachable cable */
		hwscp_detect_undetach_cable(value[0]);

		/* detect interface type */
		hwscp_detect_port_type(value[0]);

		hwlog_info("scp type_b detected(0x7e), support mode: 0x%x\n",
			*mode);
	}

	return ret;
}

static int hwscp_detect_adapter_support_mode_by_0x80(int *mode)
{
	int value_a[BYTE_ONE] = { 0 };
	int value_b[BYTE_ONE] = { 0 };

	hwscp_set_reg80_rw_error_flag(NO_RW_ERROR_FLAG);

	/* reg: 0x80 */
	if (hwscp_reg_read(HWSCP_ADP_TYPE1, value_a, BYTE_ONE)) {
		hwscp_set_reg80_rw_error_flag(RW_ERROR_FLAG);
		hwlog_err("read adp_type1(0x80) fail\n");
		return SCP_DETECT_OTHER;
	}

	hwlog_info("adp_type1[%x]=%x\n", HWSCP_ADP_TYPE1, value_a[0]);

	if (value_a[0] & HWSCP_ADP_TYPE1_B_MASK) {
		hwlog_info("scp type_b detected(0x80)\n");

		/* reg: 0x81 */
		if (hwscp_reg_read(HWSCP_B_ADP_TYPE, value_b, BYTE_ONE)) {
			hwlog_err("read b_adp_type(0x81) fail\n");
			return SCP_DETECT_OTHER;
		}

		hwlog_info("b_adp_type[%x]=%x\n",
			HWSCP_B_ADP_TYPE, value_b[0]);

		if (value_b[0] == HWSCP_B_ADP_TYPE_B_MASK) {
			*mode |= ADAPTER_SUPPORT_LVC;
			hwlog_info("scp type_b detected(0x81), support mode: 0x%x\n",
				*mode);

			return SCP_DETECT_SUCC;
		}
	}

	return SCP_DETECT_OTHER;
}

static int hwscp_detect_adapter_support_mode(int *mode)
{
	int ret;
	int support_mode = 0;
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev || !mode)
		return ADAPTER_DETECT_OTHER;

	/* set all parameter to default state */
	hwscp_set_default_param();
	l_dev->info.detect_finish_flag = 1; /* has detect adapter */
	l_dev->info.support_mode = ADAPTER_SUPPORT_UNDEFINED;

	/* protocol handshark: detect scp adapter */
	ret = hwscp_detect_adapter();
	if (ret == SCP_DETECT_OTHER) {
		hwlog_err("scp adapter detect other\n");
		return ADAPTER_DETECT_OTHER;
	}
	if (ret == SCP_DETECT_FAIL) {
		hwlog_err("scp adapter detect fail\n");
		return ADAPTER_DETECT_FAIL;
	}

	/* adapter type detect: 0x7e */
	ret = hwscp_detect_adapter_support_mode_by_0x7e(&support_mode);
	if (ret == SCP_DETECT_SUCC) {
		*mode = support_mode;
		l_dev->info.support_mode = support_mode;

		hwlog_info("scp adapter type_b detect success(judge by 0x7e)\n");
		return ADAPTER_DETECT_SUCC;
	}

	/* adapter type detect: 0x80 */
	ret = hwscp_detect_adapter_support_mode_by_0x80(&support_mode);
	if (ret == SCP_DETECT_SUCC) {
		*mode = support_mode;
		l_dev->info.support_mode = support_mode;

		hwlog_info("scp adapter type_b detect success(judge by 0x80)\n");
		return ADAPTER_DETECT_SUCC;
	}

	hwlog_err("detect_adapter_type fail\n");
	return ADAPTER_DETECT_OTHER;
}

static int hwscp_get_support_mode(int *mode)
{
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev || !mode)
		return -EPERM;

	if (l_dev->info.detect_finish_flag)
		*mode = l_dev->info.support_mode;
	else
		hwscp_detect_adapter_support_mode(mode);

	hwlog_info("support_mode: %d\n", *mode);
	return 0;
}

static int hwscp_get_device_info(struct adapter_device_info *info)
{
	if (!info)
		return -EPERM;

	if (hwscp_get_vendor_id(&info->vendor_id))
		return -EPERM;

	if (hwscp_get_module_id(&info->module_id))
		return -EPERM;

	if (hwscp_get_serial_no_id(&info->serial_no))
		return -EPERM;

	if (hwscp_get_chip_id(&info->chip_id))
		return -EPERM;

	if (hwscp_get_hw_version_id(&info->hwver))
		return -EPERM;

	if (hwscp_get_fw_version_id(&info->fwver))
		return -EPERM;

	if (hwscp_get_min_voltage(&info->min_volt))
		return -EPERM;

	if (hwscp_get_max_voltage(&info->max_volt))
		return -EPERM;

	if (hwscp_get_min_current(&info->min_cur))
		return -EPERM;

	if (hwscp_get_max_current(&info->max_cur))
		return -EPERM;

	if (hwscp_get_max_current_accuracy_err(&info->max_ierr))
		return -EPERM;

	if (hwscp_get_adp_type(&info->adp_type))
		return -EPERM;

	if (hwscp_get_adp_code(&info->adp_code))
		return -EPERM;

	hwlog_info("get_device_info\n");
	return 0;
}

static int hwscp_get_chip_vendor_id(int *id)
{
	int vendor_id = ADAPTER_CHIP_UNKNOWN;
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev || !id)
		return -EPERM;

	if (hwscp_get_vendor_id(&vendor_id))
		return -EPERM;

	switch (vendor_id) {
	case HWSCP_VENDOR_ID_RICHTEK:
		hwlog_info("adapter chip is richtek\n");
		*id = ADAPTER_CHIP_RICHTEK;
		break;
	case HWSCP_VENDOR_ID_WELTREND:
		hwlog_info("adapter chip is weltrend\n");
		*id = ADAPTER_CHIP_WELTREND;
		break;
	case HWSCP_VENDOR_ID_IWATT:
		hwlog_info("adapter chip is iwatt\n");
		*id = ADAPTER_CHIP_IWATT;
		break;
	default:
		hwlog_err("invalid adaptor chip id\n");
		*id = vendor_id;
		break;
	}

	l_dev->info.chip_vid = *id;

	hwlog_info("get_chip_vendor_id: %d\n", *id);
	return 0;
}

static int hwscp_set_output_mode(int enable)
{
	int value[BYTE_ONE] = { 0 };
	int tmp_value;

	/* reg: 0xa0 */
	if (hwscp_reg_read(HWSCP_CTRL_BYTE0, value, BYTE_ONE))
		return -EPERM;

	tmp_value = enable ? HWSCP_OUTPUT_MODE_ENABLE : HWSCP_OUTPUT_MODE_DISABLE;
	value[0] &= ~HWSCP_OUTPUT_MODE_MASK;
	value[0] |= ((int)(tmp_value << HWSCP_OUTPUT_MODE_SHIFT) & HWSCP_OUTPUT_MODE_MASK);

	/* reg: 0xa0 */
	if (hwscp_reg_write(HWSCP_CTRL_BYTE0, value, BYTE_ONE))
		return -EPERM;

	hwlog_info("set_output_mode: %d,%x\n", enable, value[0]);
	return 0;
}

static int hwscp_set_reset(int enable)
{
	int value[BYTE_ONE] = { 0 };
	int tmp_value;

	/* reg: 0xa0 */
	if (hwscp_reg_read(HWSCP_CTRL_BYTE0, value, BYTE_ONE))
		return -EPERM;

	tmp_value = enable ? HWSCP_RESET_ENABLE : HWSCP_RESET_DISABLE;
	value[0] &= ~HWSCP_RESET_MASK;
	value[0] |= ((int)(tmp_value << HWSCP_RESET_SHIFT) & HWSCP_RESET_MASK);

	/* reg: 0xa0 */
	if (hwscp_reg_write(HWSCP_CTRL_BYTE0, value, BYTE_ONE))
		return -EPERM;

	hwlog_info("set_reset: %d,%x\n", enable, value[0]);
	return 0;
}

static int hwscp_set_output_enable(int enable)
{
	int value[BYTE_ONE] = { 0 };
	int tmp_value;

	if (hwscp_set_output_mode(HWSCP_OUTPUT_MODE_ENABLE))
		return -EPERM;

	/* reg: 0xa0 */
	if (hwscp_reg_read(HWSCP_CTRL_BYTE0, value, BYTE_ONE))
		return -EPERM;

	tmp_value = enable ? HWSCP_OUTPUT_ENABLE : HWSCP_OUTPUT_DISABLE;
	value[0] &= ~HWSCP_OUTPUT_EN_MASK;
	value[0] |= ((int)(tmp_value << HWSCP_OUTPUT_EN_SHIFT) & HWSCP_OUTPUT_EN_MASK);

	/* reg: 0xa0 */
	if (hwscp_reg_write(HWSCP_CTRL_BYTE0, value, BYTE_ONE))
		return -EPERM;

	hwlog_info("set_output_enable: %d,%x\n", enable, value[0]);
	return 0;
}

static int hwscp_set_dp_delitch(int time)
{
	int value[BYTE_ONE] = { 0 };
	int tmp_value;

	/* reg: 0xa1 */
	if (hwscp_reg_read(HWSCP_CTRL_BYTE1, value, BYTE_ONE))
		return -EPERM;

	tmp_value = time;
	value[0] &= ~HWSCP_DP_DELITCH_MASK;
	value[0] |= ((int)(tmp_value << HWSCP_DP_DELITCH_SHIFT) & HWSCP_DP_DELITCH_MASK);

	/* reg: 0xa1 */
	if (hwscp_reg_write(HWSCP_CTRL_BYTE1, value, BYTE_ONE))
		return -EPERM;

	hwlog_info("set_dp_delitch: %d,%x\n", time, value[0]);
	return 0;
}

static int hwscp_set_watchdog_timer(int second)
{
	int value[BYTE_ONE] = { 0 };
	int tmp_value;

	/* reg: 0xa1 */
	if (hwscp_reg_read(HWSCP_CTRL_BYTE1, value, BYTE_ONE))
		return -EPERM;

	tmp_value = second * HWSCP_WATCHDOG_BITS_UNIT;
	value[0] &= ~HWSCP_WATCHDOG_MASK;
	value[0] |= ((int)(tmp_value << HWSCP_WATCHDOG_SHIFT) & HWSCP_WATCHDOG_MASK);

	/* reg: 0xa1 */
	if (hwscp_reg_write(HWSCP_CTRL_BYTE1, value, BYTE_ONE))
		return -EPERM;

	hwlog_info("set_watchdog_timer: %d,%x\n", second, value[0]);
	return 0;
}

static int hwscp_config_vset_boundary(int vboundary)
{
	int value[BYTE_TWO] = { 0 };
	int tmp_value;

	tmp_value = vboundary / HWSCP_VSET_BOUNDARY_STEP;
	value[0] = (tmp_value >> POWER_BITS_PER_BYTE) & POWER_MASK_BYTE;
	value[1] = tmp_value & POWER_MASK_BYTE;

	/* reg: 0xb0 & 0xb1 */
	if (hwscp_reg_write(HWSCP_VSET_BOUNDARY_HBYTE, value, BYTE_TWO))
		return -EPERM;

	hwlog_info("config_vset_boundary: %d\n", vboundary);
	return 0;
}

static int hwscp_config_iset_boundary(int iboundary)
{
	int value[BYTE_TWO] = { 0 };
	int tmp_value;

	tmp_value = iboundary / HWSCP_ISET_BOUNDARYSTEP;
	value[0] = (tmp_value >> POWER_BITS_PER_BYTE) & POWER_MASK_BYTE;
	value[1] = tmp_value & POWER_MASK_BYTE;

	/* reg: 0xb2 & 0xb3 */
	if (hwscp_reg_write(HWSCP_ISET_BOUNDARY_HBYTE, value, BYTE_TWO))
		return -EPERM;

	hwlog_info("config_iset_boundary: %d\n", iboundary);
	return 0;
}

/* single byte */
static int hwscp_set_output_voltage_s(int volt)
{
	int value[BYTE_ONE] = { 0 };
	int tmp_value;

	tmp_value = volt - HWSCP_VSSET_OFFSET;
	value[0] = tmp_value / HWSCP_VSSET_STEP;

	/* reg: 0xca */
	if (hwscp_reg_write(HWSCP_VSSET, value, BYTE_ONE))
		return -EPERM;

	hwlog_info("set_output_voltage_s: %d\n", volt);
	return 0;
}

/* double byte */
static int hwscp_set_output_voltage_d(int volt)
{
	int value[BYTE_TWO] = { 0 };
	int tmp_value;

	tmp_value = volt / HWSCP_VSET_STEP;
	value[0] = (tmp_value >> POWER_BITS_PER_BYTE) & POWER_MASK_BYTE;
	value[1] = tmp_value & POWER_MASK_BYTE;

	/* reg: 0xb8 & 0xb9 */
	if (hwscp_reg_write(HWSCP_VSET_HBYTE, value, BYTE_TWO))
		return -EPERM;

	hwlog_info("set_output_voltage_d: %d\n", volt);
	return 0;
}

static int hwscp_set_output_voltage(int volt)
{
	if (volt > HWSCP_VSSET_MAX_VOLT)
		return hwscp_set_output_voltage_d(volt);

	return hwscp_set_output_voltage_s(volt);
}

/* single byte */
static int hwscp_get_output_voltage_s(int *volt)
{
	int value[BYTE_ONE] = { 0 };
	int tmp_value;

	/* reg: 0xc8 */
	if (hwscp_reg_read(HWSCP_SREAD_VOUT, value, BYTE_ONE))
		return -EPERM;

	tmp_value = value[0] * HWSCP_SREAD_VOUT_STEP;
	*volt = tmp_value + HWSCP_SREAD_VOUT_OFFSET;

	hwlog_info("get_output_voltage_s: %d\n", *volt);
	return 0;
}

/* double byte */
static int hwscp_get_output_voltage_d(int *volt)
{
	int value[BYTE_TWO] = { 0 };
	int tmp_value;

	/* reg: 0xa8 & 0xa9 */
	if (hwscp_reg_multi_read(HWSCP_READ_VOUT_HBYTE, value, BYTE_TWO))
		return -EPERM;

	tmp_value = (value[0] << POWER_BITS_PER_BYTE) | value[1];
	*volt = tmp_value * HWSCP_READ_VOUT_STEP;

	hwlog_info("get_output_voltage_d: %d\n", *volt);
	return 0;
}

static int hwscp_get_output_voltage(int *volt)
{
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev || !volt)
		return -EPERM;

	if (l_dev->info.support_mode & ADAPTER_SUPPORT_SC)
		return hwscp_get_output_voltage_d(volt);

	return hwscp_get_output_voltage_s(volt);
}

/* single byte */
static int hwscp_set_output_current_s(int cur)
{
	int value[BYTE_ONE] = { 0 };
	struct hwscp_dev *l_dev = hwscp_get_dev();

	if (!l_dev)
		return -EPERM;

	cur += l_dev->info.max_ierr;
	value[0] = cur / HWSCP_ISSET_STEP;

	/* reg: 0xcb */
	if (hwscp_reg_write(HWSCP_ISSET, value, BYTE_ONE))
		return -EPERM;

	hwlog_info("set_output_current_s: %d\n", cur);
	return 0;
}

/* double byte */
#ifdef POWER_MODULE_DEBUG_FUNCTION
static int hwscp_set_output_current_d(int cur)
{
	int value[BYTE_TWO] = { 0 };
	int tmp_value;

	tmp_value = cur / HWSCP_ISET_STEP;
	value[0] = (tmp_value >> POWER_BITS_PER_BYTE) & POWER_MASK_BYTE;
	value[1] = tmp_value & POWER_MASK_BYTE;

	/* reg: 0xba & 0xbb */
	if (hwscp_reg_write(HWSCP_ISET_HBYTE, value, BYTE_TWO))
		return -EPERM;

	hwlog_info("set_output_current_d: %d\n", cur);
	return 0;
}
#endif /* POWER_MODULE_DEBUG_FUNCTION */

static int hwscp_set_output_current(int cur)
{
	return hwscp_set_output_current_s(cur);
}

/* single byte */
static int hwscp_get_output_current_s(int *cur)
{
	int value[BYTE_ONE] = { 0 };

	if (!cur)
		return -EPERM;

	/* reg: 0xc9 */
	if (hwscp_reg_read(HWSCP_SREAD_IOUT, value, BYTE_ONE))
		return -EPERM;

	*cur = value[0] * HWSCP_SREAD_IOUT_STEP;

	hwlog_info("get_output_current_s: %d\n", *cur);
	return 0;
}

/* double byte */
#ifdef POWER_MODULE_DEBUG_FUNCTION
static int hwscp_get_output_current_d(int *cur)
{
	int value[BYTE_TWO] = { 0 };
	int tmp_value;

	if (!cur)
		return -EPERM;

	/* reg: 0xaa & 0xab */
	if (hwscp_reg_read(HWSCP_READ_IOUT_HBYTE, value, BYTE_TWO))
		return -EPERM;

	tmp_value = (value[0] << POWER_BITS_PER_BYTE) | value[1];
	*cur = tmp_value * HWSCP_READ_IOUT_STEP;

	hwlog_info("get_output_current_d: %d\n", *cur);
	return 0;
}
#endif /* POWER_MODULE_DEBUG_FUNCTION */

static int hwscp_get_output_current(int *cur)
{
	return hwscp_get_output_current_s(cur);
}

/* single byte */
static int hwscp_get_output_current_set_s(int *cur)
{
	int value[BYTE_ONE] = { 0 };

	if (!cur)
		return -EPERM;

	/* reg: 0xcb */
	if (hwscp_reg_read(HWSCP_ISSET, value, BYTE_ONE))
		return -EPERM;

	*cur = value[0] * HWSCP_ISSET_STEP;

	hwlog_info("get_output_current_set_s: %d\n", *cur);
	return 0;
}

/* double byte */
#ifdef POWER_MODULE_DEBUG_FUNCTION
static int hwscp_get_output_current_set_d(int *cur)
{
	int value[BYTE_TWO] = { 0 };
	int tmp_value;

	if (!cur)
		return -EPERM;

	/* reg: 0xba & 0xbb */
	if (hwscp_reg_read(HWSCP_ISET_HBYTE, value, BYTE_TWO))
		return -EPERM;

	tmp_value = (value[0] << POWER_BITS_PER_BYTE) | value[1];
	*cur = tmp_value * HWSCP_ISET_STEP;

	hwlog_info("get_output_current_set_d: %d\n", *cur);
	return 0;
}
#endif /* POWER_MODULE_DEBUG_FUNCTION */

static int hwscp_get_output_current_set(int *cur)
{
	return hwscp_get_output_current_set_s(cur);
}

static int hwscp_get_inside_temp(int *temp)
{
	int value[BYTE_ONE] = { 0 };

	if (!temp)
		return -EPERM;

	/* reg: 0xa6 */
	if (hwscp_reg_read(HWSCP_INSIDE_TMP, value, BYTE_ONE))
		return -EPERM;

	*temp = value[0] * HWSCP_INSIDE_TMP_UNIT;

	hwlog_info("get_inside_temp: %d\n", *temp);
	return 0;
}

static int hwscp_get_port_temp(int *temp)
{
	int value[BYTE_ONE] = { 0 };

	if (!temp)
		return -EPERM;

	/* reg: 0xa7 */
	if (hwscp_reg_read(HWSCP_PORT_TMP, value, BYTE_ONE))
		return -EPERM;

	*temp = value[0] * HWSCP_PORT_TMP_UNIT;

	hwlog_info("get_port_temp: %d\n", *temp);
	return 0;
}

static int hwscp_get_port_leakage_cur_flag(int *flag)
{
	int value[BYTE_ONE] = { 0 };

	if (!flag)
		return -EPERM;

	/* reg: 0xa2 */
	if (hwscp_reg_read(HWSCP_STATUS_BYTE0, value, BYTE_ONE))
		return -EPERM;

	if ((value[0] & HWSCP_LEAKAGE_FLAG_MASK) >> HWSCP_LEAKAGE_FLAG_SHIFT)
		*flag = HWSCP_PORT_LEAKAGE;
	else
		*flag = HWSCP_PORT_NOT_LEAKAGE;

	hwlog_info("get_port_leakage_current_flag: %d\n", *flag);
	return 0;
}

static int hwscp_set_encrypt_index(int retrys, int index)
{
	int i;
	int value[BYTE_ONE] = { 0 };

	value[0] = index;

	/* reg: 0xce */
	for (i = 0; i < retrys; i++) {
		if (hwscp_reg_write(HWSCP_KEY_INDEX, value, BYTE_ONE) == 0)
			break;
	}

	if (i >= retrys) {
		hwlog_err("set_encrypt_index failed\n");
		return -EPERM;
	}

	hwlog_info("set_encrypt_index: %d\n", index);
	return 0;
}

static int hwscp_get_encrypt_enable(int retrys, int *flag)
{
	int i;
	int value[BYTE_ONE] = { 0 };

	/* reg: 0xcf */
	for (i = 0; i < retrys; i++) {
		if (hwscp_reg_read(HWSCP_ENCRYPT_INFO, value, BYTE_ONE) == 0)
			break;
	}

	if (i >= retrys) {
		hwlog_err("get_encrypt_enable failed\n");
		return -EPERM;
	}

	if ((value[0] & HWSCP_ENCRYPT_ENABLE_MASK) >> HWSCP_ENCRYPT_ENABLE_SHIFT)
		*flag = HWSCP_ENCRYPT_ENABLE;
	else
		*flag = HWSCP_ENCRYPT_DISABLE;

	hwlog_info("get_encrypt_enable: %d\n", *flag);
	return 0;
}

static int hwscp_get_encrypt_completed(void)
{
	int i;
	int value[BYTE_ONE] = { 0 };

	/* reg: 0xcf */
	for (i = 0; i < RETRY_THREE; i++) {
		if (hwscp_reg_read(HWSCP_ENCRYPT_INFO, value, BYTE_ONE))
			break;

		if ((value[0] & HWSCP_ENCRYPT_COMPLETED_MASK) >> HWSCP_ENCRYPT_COMPLETED_SHIFT) {
			hwlog_info("get_encrypt_completed succ\n");
			return 0;
		}
	}

	hwlog_err("get_encrypt_completed failed\n");
	return -EPERM;
}

static int hwscp_set_random_num(int retrys)
{
	int i;
	int value[BYTE_EIGHT] = { 0 };
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev)
		return -EPERM;

	for (i = 0; i < BYTE_EIGHT; i++) {
		get_random_bytes(&value[i], sizeof(unsigned char));
		l_dev->encrypt_random_host[i] = value[i];
	}

	/* reg: 0xa0 */
	for (i = 0; i < retrys; i++) {
		if (hwscp_reg_write(HWSCP_ENCRYPT_RANDOM_WR_BASE, value, BYTE_EIGHT) == 0)
			break;
	}

	if (i >= retrys) {
		hwlog_err("set_random_num failed\n");
		return -EPERM;
	}

	return 0;
}

static int hwscp_get_random_num(int retrys)
{
	int i;
	int value[BYTE_EIGHT] = { 0 };
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev)
		return -EPERM;

	/* reg: 0xa8 */
	for (i = 0; i < retrys; i++) {
		if (hwscp_reg_read(HWSCP_ENCRYPT_RANDOM_RD_BASE, value, BYTE_EIGHT) == 0)
			break;
	}

	if (i >= retrys) {
		hwlog_err("get_random_num failed\n");
		return -EPERM;
	}

	for (i = 0; i < BYTE_EIGHT; i++)
		l_dev->encrypt_random_slave[i] = value[i];

	return 0;
}

static int hwscp_get_encrypted_value(int retrys)
{
	int i;
	int value[BYTE_SIXTEEN] = { 0 };
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev)
		return -EPERM;

	/* reg: 0xb0 */
	for (i = 0; i < retrys; i++) {
		if (hwscp_reg_read(HWSCP_ENCRYPT_HASH_RD_BASE, value, BYTE_SIXTEEN) == 0)
			break;
	}

	if (i >= retrys) {
		hwlog_err("get_encrypted_value failed\n");
		return -EPERM;
	}

	for (i = 0; i < BYTE_SIXTEEN; i++)
		l_dev->encrypt_hash_slave[i] = value[i];

	return 0;
}

static int hwscp_copy_hash_value(int key, unsigned char *hash, int size)
{
	int i = 0;
	int j;
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev)
		return -EPERM;

	if (size != (BYTE_EIGHT + BYTE_EIGHT + BYTE_SIXTEEN + 1)) {
		hwlog_err("invalid hash_size=%d\n", size);
		return -EPERM;
	}

	for (j = 0; i < BYTE_EIGHT; i++, j++)
		hash[i] = l_dev->encrypt_random_host[j];

	for (j = 0; i < BYTE_EIGHT + BYTE_EIGHT; i++, j++)
		hash[i] = l_dev->encrypt_random_slave[j];

	for (j = 0; i < BYTE_EIGHT + BYTE_EIGHT + BYTE_SIXTEEN; i++, j++)
		hash[i] = l_dev->encrypt_hash_slave[j];

	hash[i] = (unsigned char)key;

	return 0;
}

static int hwscp_auth_encrypt_start(int key)
{
	int ret;
	int encrypted_flag = 0;
	int retry = RETRY_ONE;

	/* first: set key index */
	if (hwscp_set_encrypt_index(retry, key)) {
		ret = -1;
		goto fail_encrypt;
	}

	/* second: get encrypt enable flag */
	if (hwscp_get_encrypt_enable(retry, &encrypted_flag)) {
		ret = -1;
		goto fail_encrypt;
	}

	if (encrypted_flag == HWSCP_ENCRYPT_DISABLE) {
		ret = -1;
		goto fail_encrypt;
	}

	/* third: host set random num to slave */
	if (hwscp_set_random_num(retry)) {
		ret = -1;
		goto fail_encrypt;
	}

	/* fouth: get encrypt completed flag */
	if (hwscp_get_encrypt_completed()) {
		ret = -1;
		goto fail_encrypt;
	}

	/* fifth: host get random num from slave */
	if (hwscp_get_random_num(retry)) {
		ret = -1;
		goto fail_encrypt;
	}

	/* sixth: host get hash num from slave */
	if (hwscp_get_encrypted_value(retry)) {
		ret = -1;
		goto fail_encrypt;
	}

	/* seventh: copy hash value */
	hwscp_auth_clean_hash_data();
	if (hwscp_copy_hash_value(key, hwscp_auth_get_hash_data_header(),
		hwscp_auth_get_hash_data_size())) {
		ret = -1;
		goto fail_encrypt;
	}

	/* eighth: wait hash calculate complete */
	ret = hwscp_auth_wait_completion();

fail_encrypt:
	hwscp_auth_clean_hash_data();
	if (hwscp_set_encrypt_index(retry, HWSCP_KEY_INDEX_RELEASE))
		return -EPERM;

	hwlog_info("auth_encrypt_start\n");
	return ret;
}

static int hwscp_set_usbpd_enable(int enable)
{
	int value[BYTE_ONE] = { 0 };
	int tmp_value;

	if (hwscp_get_reg7e_rw_error_flag() || !hwscp_is_cport_adp()) {
		hwlog_info("disabled pd is not supported\n");
		return 0;
	}

	/* reg: 0xcf */
	if (hwscp_reg_read(HWSCP_USBPD_INFO, value, BYTE_ONE))
		return -EPERM;

	tmp_value = enable ? HWSCP_USBPD_ENABLE : HWSCP_USBPD_DISABLE;
	value[0] &= ~HWSCP_USBPD_ENABLE_MASK;
	value[0] |= ((int)(tmp_value << HWSCP_USBPD_ENABLE_SHIFT) & HWSCP_USBPD_ENABLE_MASK);

	/* reg: 0xcf */
	if (hwscp_reg_write(HWSCP_USBPD_INFO, value, BYTE_ONE))
		return -EPERM;

	hwlog_info("set_usbpd_enable: %d,%x\n", enable, value[0]);
	return 0;
}

static int hwscp_set_default_state(void)
{
	int ret;
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev)
		return -EPERM;

	/* process non protocol flow */
	if (hwscp_process_pre_exit())
		return -EPERM;

	ret = hwscp_set_output_mode(HWSCP_OUTPUT_MODE_DISABLE);

	switch (l_dev->info.chip_vid) {
	case ADAPTER_CHIP_IWATT:
		ret |= hwscp_set_reset(HWSCP_RESET_ENABLE);
		break;
	default:
		hwlog_info("adapter non iwatt chip\n");
		break;
	}

	msleep(RESET_TIME_50MS);

	/* process non protocol flow */
	if (hwscp_process_post_exit())
		return -EPERM;

	if (ret != 0)
		hwlog_info("set_default_state fail\n");
	else
		hwlog_info("set_default_state ok\n");

	return ret;
}

static int hwscp_set_init_data(struct adapter_init_data *data)
{
	int value[BYTE_FOUR] = { 0 };
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev || !data)
		return -EPERM;

	/* process non protocol flow */
	if (hwscp_process_pre_init())
		return -EPERM;

	if (hwscp_set_output_mode(data->scp_mode_enable))
		return -EPERM;

	if (hwscp_config_vset_boundary(data->vset_boundary))
		return -EPERM;

	if (hwscp_config_iset_boundary(data->iset_boundary))
		return -EPERM;

	if (hwscp_set_output_voltage(data->init_voltage))
		return -EPERM;

	/* fix a superswitch bug, set dp_delitch to 5ms for richtek chip */
	if ((l_dev->dev_id == PROTOCOL_DEVICE_ID_FUSB3601) &&
		(l_dev->info.chip_vid == ADAPTER_CHIP_RICHTEK)) {
		if (hwscp_set_dp_delitch(HWSCP_DP_DELITCH_5MS))
			return -EPERM;
	}

	if (hwscp_set_watchdog_timer(data->watchdog_timer))
		return -EPERM;

	/* process non protocol flow */
	if (hwscp_process_post_init())
		return -EPERM;

	if (hwscp_reg_read(HWSCP_CTRL_BYTE0, value, BYTE_FOUR))
		return -EPERM;

	hwlog_info("ctrl_byte0[%x]=%x\n", HWSCP_CTRL_BYTE0, value[0]);
	hwlog_info("ctrl_byte1[%x]=%x\n", HWSCP_CTRL_BYTE1, value[1]);
	hwlog_info("status_byte0[%x]=%x\n", HWSCP_STATUS_BYTE0, value[2]);
	hwlog_info("status_byte1[%x]=%x\n", HWSCP_STATUS_BYTE1, value[3]);

	if (hwscp_reg_read(HWSCP_VSET_BOUNDARY_HBYTE, value, BYTE_FOUR))
		return -EPERM;

	hwlog_info("vset_boundary[%x]=%x\n", HWSCP_VSET_BOUNDARY_HBYTE, value[0]);
	hwlog_info("vset_boundary[%x]=%x\n", HWSCP_VSET_BOUNDARY_LBYTE, value[1]);
	hwlog_info("iset_boundary[%x]=%x\n", HWSCP_ISET_BOUNDARY_HBYTE, value[2]);
	hwlog_info("iset_boundary[%x]=%x\n", HWSCP_ISET_BOUNDARY_LBYTE, value[3]);

	hwlog_info("set_init_data\n");
	return 0;
}

static bool hwscp_is_undetach_cable(void)
{
	struct hwscp_dev *l_dev = hwscp_get_dev();

	if (!l_dev)
		return false;

	return !!l_dev->info.is_undetach_cable;
}

int hwscp_get_protocol_register_state(void)
{
	struct hwscp_dev *l_dev = NULL;

	l_dev = hwscp_get_dev();
	if (!l_dev)
		return -EPERM;

	if ((l_dev->dev_id >= PROTOCOL_DEVICE_ID_BEGIN) &&
		(l_dev->dev_id < PROTOCOL_DEVICE_ID_END))
		return 0;

	hwlog_info("get_protocol_register_state fail\n");
	return -EPERM;
}

static struct adapter_protocol_ops adapter_protocol_scp_ops = {
	.type_name = "hw_scp",
	.soft_reset_master = hwscp_soft_reset_master,
	.soft_reset_slave = hwscp_soft_reset_slave,
	.detect_adapter_support_mode = hwscp_detect_adapter_support_mode,
	.get_support_mode = hwscp_get_support_mode,
	.get_device_info = hwscp_get_device_info,
	.get_chip_vendor_id = hwscp_get_chip_vendor_id,
	.get_chip_serial_num = hwscp_get_serial_no_id,
	.set_output_enable = hwscp_set_output_enable,
	.set_output_mode = hwscp_set_output_mode,
	.set_reset = hwscp_set_reset,
	.set_output_voltage = hwscp_set_output_voltage,
	.get_output_voltage = hwscp_get_output_voltage,
	.set_output_current = hwscp_set_output_current,
	.get_output_current = hwscp_get_output_current,
	.get_output_current_set = hwscp_get_output_current_set,
	.get_min_voltage = hwscp_get_min_voltage,
	.get_max_voltage = hwscp_get_max_voltage,
	.get_min_current = hwscp_get_min_current,
	.get_max_current = hwscp_get_max_current,
	.get_power_drop_current = hwscp_get_power_drop_current,
	.get_power_curve_num = hwscp_get_power_curve_num,
	.get_power_curve = hwscp_get_power_curve,
	.get_inside_temp = hwscp_get_inside_temp,
	.get_port_temp = hwscp_get_port_temp,
	.get_port_leakage_current_flag = hwscp_get_port_leakage_cur_flag,
	.auth_encrypt_start = hwscp_auth_encrypt_start,
	.set_usbpd_enable = hwscp_set_usbpd_enable,
	.set_default_state = hwscp_set_default_state,
	.set_default_param = hwscp_set_default_param,
	.set_init_data = hwscp_set_init_data,
	.get_protocol_register_state = hwscp_get_protocol_register_state,
	.get_adp_type = hwscp_get_adp_type,
	.is_undetach_cable = hwscp_is_undetach_cable,
};

static int __init hwscp_init(void)
{
	int ret;
	struct hwscp_dev *l_dev = NULL;

	l_dev = kzalloc(sizeof(*l_dev), GFP_KERNEL);
	if (!l_dev)
		return -ENOMEM;

	g_hwscp_dev = l_dev;
	l_dev->dev_id = PROTOCOL_DEVICE_ID_END;

	ret = adapter_protocol_ops_register(&adapter_protocol_scp_ops);
	if (ret)
		goto fail_register_ops;

	return 0;

fail_register_ops:
	kfree(l_dev);
	g_hwscp_dev = NULL;
	return ret;
}

static void __exit hwscp_exit(void)
{
	if (!g_hwscp_dev)
		return;

	kfree(g_hwscp_dev);
	g_hwscp_dev = NULL;
}

subsys_initcall_sync(hwscp_init);
module_exit(hwscp_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("scp protocol driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
