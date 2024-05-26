// SPDX-License-Identifier: GPL-2.0
/*
 * adapter_protocol_fcp.c
 *
 * fcp protocol driver
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
#include <chipset_common/hwpower/protocol/adapter_protocol_fcp.h>
#include <chipset_common/hwpower/protocol/adapter_protocol_scp.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG fcp_protocol
HWLOG_REGIST();

static struct hwfcp_dev *g_hwfcp_dev;

static const struct adapter_protocol_device_data g_hwfcp_dev_data[] = {
	{ PROTOCOL_DEVICE_ID_FSA9685, "fsa9685" },
	{ PROTOCOL_DEVICE_ID_RT8979, "rt8979" },
	{ PROTOCOL_DEVICE_ID_SCHARGER_V300, "scharger_v300" },
	{ PROTOCOL_DEVICE_ID_SCHARGER_V600, "scharger_v600" },
	{ PROTOCOL_DEVICE_ID_FUSB3601, "fusb3601" },
	{ PROTOCOL_DEVICE_ID_BQ2560X, "bq2560x" },
	{ PROTOCOL_DEVICE_ID_RT9466, "rt9466" },
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

static int hwfcp_get_device_id(const char *str)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_hwfcp_dev_data); i++) {
		if (!strncmp(str, g_hwfcp_dev_data[i].name, strlen(str)))
			return g_hwfcp_dev_data[i].id;
	}

	return -EPERM;
}

static struct hwfcp_dev *hwfcp_get_dev(void)
{
	if (!g_hwfcp_dev) {
		hwlog_err("g_hwfcp_dev is null\n");
		return NULL;
	}

	return g_hwfcp_dev;
}

static struct hwfcp_ops *hwfcp_get_ops(void)
{
	if (!g_hwfcp_dev || !g_hwfcp_dev->p_ops) {
		hwlog_err("g_hwfcp_dev or p_ops is null\n");
		return NULL;
	}

	return g_hwfcp_dev->p_ops;
}

int hwfcp_ops_register(struct hwfcp_ops *ops)
{
	int dev_id;

	if (!g_hwfcp_dev || !ops || !ops->chip_name) {
		hwlog_err("g_hwfcp_dev or ops or chip_name is null\n");
		return -EPERM;
	}

	dev_id = hwfcp_get_device_id(ops->chip_name);
	if (dev_id < 0) {
		hwlog_err("%s ops register fail\n", ops->chip_name);
		return -EPERM;
	}

	g_hwfcp_dev->p_ops = ops;
	g_hwfcp_dev->dev_id = dev_id;

	hwlog_info("%d:%s ops register ok\n", dev_id, ops->chip_name);
	return 0;
}

static int hwfcp_check_trans_num(int num)
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

static int hwfcp_get_rw_error_flag(void)
{
	struct hwfcp_dev *l_dev = NULL;

	l_dev = hwfcp_get_dev();
	if (!l_dev)
		return -EPERM;

	return l_dev->info.rw_error_flag;
}

static void hwfcp_set_rw_error_flag(int flag)
{
	struct hwfcp_dev *l_dev = NULL;

	l_dev = hwfcp_get_dev();
	if (!l_dev)
		return;

	hwlog_info("set_rw_error_flag: %d\n", flag);
	l_dev->info.rw_error_flag = flag;
}

static int hwfcp_reg_read(int reg, int *val, int num)
{
	int ret;
	struct hwfcp_ops *l_ops = NULL;

	if (hwfcp_get_rw_error_flag())
		return -EPERM;

	l_ops = hwfcp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->reg_read) {
		hwlog_err("reg_read is null\n");
		return -EPERM;
	}

	if (hwfcp_check_trans_num(num)) {
		hwlog_err("invalid num=%d\n", num);
		return -EPERM;
	}

	ret = l_ops->reg_read(reg, val, num, l_ops->dev_data);
	if (ret < 0) {
		if (reg != HWFCP_ADP_TYPE1)
			hwfcp_set_rw_error_flag(RW_ERROR_FLAG);

		hwlog_err("reg 0x%x read fail\n", reg);
		return -EPERM;
	}

	return 0;
}

static int hwfcp_reg_write(int reg, const int *val, int num)
{
	int ret;
	struct hwfcp_ops *l_ops = NULL;

	if (hwfcp_get_rw_error_flag())
		return -EPERM;

	l_ops = hwfcp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->reg_write) {
		hwlog_err("reg_write is null\n");
		return -EPERM;
	}

	if (hwfcp_check_trans_num(num)) {
		hwlog_err("invalid num=%d\n", num);
		return -EPERM;
	}

	ret = l_ops->reg_write(reg, val, num, l_ops->dev_data);
	if (ret < 0) {
		if (reg != HWFCP_ADP_TYPE1)
			hwfcp_set_rw_error_flag(RW_ERROR_FLAG);

		hwlog_err("reg 0x%x write fail\n", reg);
		return -EPERM;
	}

	return 0;
}

static int hwfcp_detect_adapter(void)
{
	struct hwfcp_ops *l_ops = NULL;

	l_ops = hwfcp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->detect_adapter) {
		hwlog_err("detect_adapter is null\n");
		return -EPERM;
	}

	hwlog_info("detect_adapter\n");

	return l_ops->detect_adapter(l_ops->dev_data);
}

static int hwfcp_soft_reset_master(void)
{
	struct hwfcp_ops *l_ops = NULL;

	l_ops = hwfcp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->soft_reset_master) {
		hwlog_err("soft_reset_master is null\n");
		return -EPERM;
	}

	hwlog_info("soft_reset_master\n");

	return l_ops->soft_reset_master(l_ops->dev_data);
}

static int hwfcp_soft_reset_slave(void)
{
	struct hwfcp_ops *l_ops = NULL;

	l_ops = hwfcp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->soft_reset_slave) {
		hwlog_err("soft_reset_slave is null\n");
		return -EPERM;
	}

	hwlog_info("soft_reset_slave\n");

	return l_ops->soft_reset_slave(l_ops->dev_data);
}

static int hwfcp_get_master_status(void)
{
	struct hwfcp_ops *l_ops = NULL;

	l_ops = hwfcp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->get_master_status)
		return 0;

	hwlog_info("get_master_status\n");

	return l_ops->get_master_status(l_ops->dev_data);
}

static int hwfcp_stop_charging_config(void)
{
	struct hwfcp_ops *l_ops = NULL;

	l_ops = hwfcp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->stop_charging_config)
		return 0;

	hwlog_info("stop_charging_config\n");

	return l_ops->stop_charging_config(l_ops->dev_data);
}

static bool hwfcp_is_accp_charger_type(void)
{
	struct hwfcp_ops *l_ops = NULL;

	l_ops = hwfcp_get_ops();
	if (!l_ops)
		return false;

	if (!l_ops->is_accp_charger_type)
		return false;

	hwlog_info("is_accp_charger_type\n");

	return (bool)l_ops->is_accp_charger_type(l_ops->dev_data);
}

#ifdef POWER_MODULE_DEBUG_FUNCTION
static int hwfcp_process_pre_init(void)
{
	struct hwfcp_ops *l_ops = NULL;

	l_ops = hwfcp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->pre_init)
		return 0;

	hwlog_info("process_pre_init\n");

	return l_ops->pre_init(l_ops->dev_data);
}

static int hwfcp_process_post_init(void)
{
	struct hwfcp_ops *l_ops = NULL;

	l_ops = hwfcp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->post_init)
		return 0;

	hwlog_info("process_post_init\n");

	return l_ops->post_init(l_ops->dev_data);
}
#endif /* POWER_MODULE_DEBUG_FUNCTION */

static int hwfcp_process_pre_exit(void)
{
	struct hwfcp_ops *l_ops = NULL;

	l_ops = hwfcp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->pre_exit)
		return 0;

	hwlog_info("process_pre_exit\n");

	return l_ops->pre_exit(l_ops->dev_data);
}

static int hwfcp_process_post_exit(void)
{
	struct hwfcp_ops *l_ops = NULL;

	l_ops = hwfcp_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->post_exit)
		return 0;

	hwlog_info("process_post_exit\n");

	return l_ops->post_exit(l_ops->dev_data);
}

static int hwfcp_set_default_param(void)
{
	struct hwfcp_dev *l_dev = NULL;

	l_dev = hwfcp_get_dev();
	if (!l_dev)
		return -EPERM;

	memset(&l_dev->info, 0, sizeof(l_dev->info));

	return 0;
}

static int hwfcp_get_vendor_id(int *id)
{
	int value[BYTE_ONE] = { 0 };
	struct hwfcp_dev *l_dev = NULL;

	l_dev = hwfcp_get_dev();
	if (!l_dev || !id)
		return -EPERM;

	if (l_dev->info.vid_rd_flag == HAS_READ_FLAG) {
		*id = l_dev->info.vid;
		hwlog_info("get_vendor_id_a: 0x%x\n", *id);
		return 0;
	}

	/* reg: 0x04 */
	if (hwfcp_reg_read(HWFCP_ID_OUT0, value, BYTE_ONE))
		return -EPERM;

	*id = value[0];
	l_dev->info.vid_rd_flag = HAS_READ_FLAG;
	l_dev->info.vid = value[0];

	hwlog_info("get_vendor_id_f: 0x%x\n", *id);
	return 0;
}

static int hwfcp_get_voltage_capabilities(int *cap)
{
	int value[BYTE_ONE] = { 0 };
	struct hwfcp_dev *l_dev = NULL;

	l_dev = hwfcp_get_dev();
	if (!l_dev)
		return -EPERM;

	if (l_dev->info.volt_cap_rd_flag == HAS_READ_FLAG) {
		*cap = l_dev->info.volt_cap;
		hwlog_info("get_voltage_capabilities_a: %d\n", *cap);
		return 0;
	}

	/* reg: 0x21 */
	if (hwfcp_reg_read(HWFCP_DISCRETE_CAPABILOTIES0, value, BYTE_ONE))
		return -EPERM;

	/* only support three out voltage cap(5v 9v 12v) */
	if (value[0] > HWFCP_CAPABILOTIES_5V_9V_12V) {
		hwlog_err("invalid voltage_capabilities=%d\n", value[0]);
		return -EPERM;
	}

	*cap = value[0];
	l_dev->info.volt_cap_rd_flag = HAS_READ_FLAG;
	l_dev->info.volt_cap = value[0];

	hwlog_info("get_voltage_capabilities_f: %d\n", *cap);
	return 0;
}

static int hwfcp_get_max_voltage(int *volt)
{
	struct hwfcp_dev *l_dev = NULL;
	int value[BYTE_ONE] = { 0 };
	int cap;

	l_dev = hwfcp_get_dev();
	if (!l_dev || !volt)
		return -EPERM;

	if (l_dev->info.max_volt_rd_flag == HAS_READ_FLAG) {
		*volt = l_dev->info.max_volt;
		hwlog_info("get_max_voltage_a: %d\n", *volt);
		return 0;
	}

	if (hwfcp_get_voltage_capabilities(&cap))
		return -EPERM;

	/* reg: 0x3n */
	if (hwfcp_reg_read(hwfcp_output_v_reg(cap), value, BYTE_ONE))
		return -EPERM;

	*volt = (value[0] * HWFCP_OUTPUT_V_STEP);
	l_dev->info.max_volt_rd_flag = HAS_READ_FLAG;
	l_dev->info.max_volt = *volt;

	hwlog_info("get_max_voltage_f: %d\n", *volt);
	return 0;
}

static int hwfcp_get_max_power(int *power)
{
	int value[BYTE_ONE] = { 0 };
	struct hwfcp_dev *l_dev = NULL;

	l_dev = hwfcp_get_dev();
	if (!l_dev)
		return -EPERM;

	if (l_dev->info.max_pwr_rd_flag == HAS_READ_FLAG) {
		*power = l_dev->info.max_pwr;
		hwlog_info("get_max_power_a: %d\n", *power);
		return 0;
	}

	/* reg: 0x22 */
	if (hwfcp_reg_read(HWFCP_MAX_PWR, value, BYTE_ONE))
		return -EPERM;

	*power = (value[0] * HWFCP_MAX_PWR_STEP);
	l_dev->info.max_pwr_rd_flag = HAS_READ_FLAG;
	l_dev->info.max_pwr = *power;

	hwlog_info("get_max_power_f: %d\n", *power);
	return 0;
}

#ifdef CONFIG_DIRECT_CHARGER
static int hwfcp_detect_adapter_support_mode_by_0x80(void)
{
	int value[BYTE_ONE] = { 0 };

	/* reg: 0x80 */
	if (hwfcp_reg_read(HWFCP_ADP_TYPE1, value, BYTE_ONE)) {
		hwlog_err("read adp_type1(0x80) fail\n");
		return HWFCP_DETECT_SUCC;
	}

	hwlog_info("adp_type1[%x]=%x\n", HWFCP_ADP_TYPE1, value[0]);
	return HWFCP_DETECT_OTHER;
}
#endif /* CONFIG_DIRECT_CHARGER */

static int hwfcp_detect_adapter_support_mode(int *mode)
{
	int ret;
	struct hwfcp_dev *l_dev = NULL;

	l_dev = hwfcp_get_dev();
	if (!l_dev || !mode)
		return ADAPTER_DETECT_OTHER;

	/* set all parameter to default state */
	hwfcp_set_default_param();

	l_dev->info.support_mode = ADAPTER_SUPPORT_UNDEFINED;

	/*
	 * if 0x80 has read failed(-1) when detect scp adapter,
	 * we think it is a fcp adapter at this case.
	 * in order to reduce the detection time of fcp adapter.
	 */
	if (hwscp_get_reg80_rw_error_flag()) {
		hwlog_info("no need continue, reg80 already read fail\n");
		goto end_detect;
	}

	/* protocol handshark: detect fcp adapter */
	ret = hwfcp_detect_adapter();
	if (ret == HWFCP_DETECT_OTHER) {
		hwlog_err("fcp adapter detect other\n");
		return ADAPTER_DETECT_OTHER;
	}
	if (ret == HWFCP_DETECT_FAIL) {
		hwlog_err("fcp adapter detect fail\n");
		return ADAPTER_DETECT_FAIL;
	}

#ifdef CONFIG_DIRECT_CHARGER
	/*
	 * if the product does not support scp,
	 * we think it is a fcp adapter at this case.
	 */
	if (hwscp_get_protocol_register_state()) {
		hwlog_info("no need continue, scp protocol not support\n");
		goto end_detect;
	}

	/* adapter type detect: 0x80 */
	ret = hwfcp_detect_adapter_support_mode_by_0x80();
	if (ret == HWFCP_DETECT_OTHER) {
		hwlog_info("fcp adapter type_b detect other(judge by 0x80)\n");
		return ADAPTER_DETECT_OTHER;
	}
#endif /* CONFIG_DIRECT_CHARGER */

end_detect:
	*mode = ADAPTER_SUPPORT_HV;
	l_dev->info.support_mode = ADAPTER_SUPPORT_HV;
	hwlog_info("detect_adapter_type success\n");
	return ADAPTER_DETECT_SUCC;
}

static int hwfcp_get_support_mode(int *mode)
{
	struct hwfcp_dev *l_dev = NULL;

	l_dev = hwfcp_get_dev();
	if (!l_dev || !mode)
		return -EPERM;

	*mode = l_dev->info.support_mode;

	hwlog_info("get_support_mode\n");
	return 0;
}

static int hwfcp_get_device_info(struct adapter_device_info *info)
{
	if (!info)
		return -EPERM;

	if (hwfcp_get_vendor_id(&info->vendor_id))
		return -EPERM;

	if (hwfcp_get_max_voltage(&info->max_volt))
		return -EPERM;

	if (hwfcp_get_voltage_capabilities(&info->volt_cap))
		return -EPERM;

	if (hwfcp_get_max_power(&info->max_pwr))
		return -EPERM;

	hwlog_info("get_device_info\n");
	return 0;
}

static int hwfcp_set_output_voltage(int volt)
{
	int value1[BYTE_ONE] = { 0 };
	int value2[BYTE_ONE] = { 0 };
	int value3[BYTE_ONE] = { 0 };
	int vendor_id = 0;
	struct hwfcp_dev *l_dev = NULL;

	l_dev = hwfcp_get_dev();
	if (!l_dev)
		return -EPERM;

	/* read vid for identify huawei adapter */
	if (hwfcp_get_vendor_id(&vendor_id))
		return -EPERM;

	value1[0] = (volt / HWFCP_VOUT_STEP);
	value3[0] = HWFCP_VOUT_CONFIG_ENABLE;

	/* reg: 0x2c */
	if (hwfcp_reg_write(HWFCP_VOUT_CONFIG, value1, BYTE_ONE))
		return -EPERM;

	/* reg: 0x2c */
	if (hwfcp_reg_read(HWFCP_VOUT_CONFIG, value2, BYTE_ONE))
		return -EPERM;

	/* bq2560x & rt9466 should ignore this case */
	if ((l_dev->dev_id != PROTOCOL_DEVICE_ID_BQ2560X) &&
		(l_dev->dev_id != PROTOCOL_DEVICE_ID_RT9466)) {
		if (value1[0] != value2[0]) {
			hwlog_err("output voltage config fail, reg[0x%x]=%d\n",
				HWFCP_VOUT_CONFIG, value2[0]);
			return -EPERM;
		}
	}

	/* reg: 0x2b */
	if (hwfcp_reg_write(HWFCP_OUTPUT_CONTROL, value3, BYTE_ONE))
		return -EPERM;

	hwlog_info("set_output_voltage: %d\n", volt);
	return 0;
}

static int hwfcp_get_output_current(int *cur)
{
	int volt = 1;
	int max_power = 0;

	if (!cur)
		return -EPERM;

	if (hwfcp_get_max_voltage(&volt))
		return -EPERM;

	if (hwfcp_get_max_power(&max_power))
		return -EPERM;

	if (volt == 0) {
		hwlog_err("volt is zero\n");
		return -EPERM;
	}

	/* mw = 1000ma * mv */
	*cur = (max_power / volt) * 1000;

	hwlog_info("get_output_current: %d\n", *cur);
	return 0;
}

static int hwfcp_get_slave_status(void)
{
	int value[BYTE_ONE] = { 0 };

	/* reg: 0x28 */
	if (hwfcp_reg_read(HWFCP_ADAPTER_STATUS, value, BYTE_ONE))
		return -EPERM;

	hwlog_info("get_slave_status: %d\n", value[0]);

	if ((value[0] & HWFCP_UVP_MASK) == HWFCP_UVP_MASK)
		return ADAPTER_OUTPUT_UVP;

	if ((value[0] & HWFCP_OVP_MASK) == HWFCP_OVP_MASK)
		return ADAPTER_OUTPUT_OVP;

	if ((value[0] & HWFCP_OCP_MASK) == HWFCP_OCP_MASK)
		return ADAPTER_OUTPUT_OCP;

	if ((value[0] & HWFCP_OTP_MASK) == HWFCP_OTP_MASK)
		return ADAPTER_OUTPUT_OTP;

	return ADAPTER_OUTPUT_NORMAL;
}

static int hwfcp_set_default_state(void)
{
	struct hwfcp_dev *l_dev = NULL;

	l_dev = hwfcp_get_dev();
	if (!l_dev)
		return -EPERM;

	/* process non protocol flow */
	if (hwfcp_process_pre_exit())
		return -EPERM;

	/* process non protocol flow */
	if (hwfcp_process_post_exit())
		return -EPERM;

	hwlog_info("set_default_state ok\n");
	return 0;
}

static int hwfcp_get_protocol_register_state(void)
{
	struct hwfcp_dev *l_dev = NULL;

	l_dev = hwfcp_get_dev();
	if (!l_dev)
		return -EPERM;

	if ((l_dev->dev_id >= PROTOCOL_DEVICE_ID_BEGIN) &&
		(l_dev->dev_id < PROTOCOL_DEVICE_ID_END))
		return 0;

	hwlog_info("get_protocol_register_state fail\n");
	return -EPERM;
}

static struct adapter_protocol_ops adapter_protocol_hwfcp_ops = {
	.type_name = "hw_fcp",
	.soft_reset_master = hwfcp_soft_reset_master,
	.soft_reset_slave = hwfcp_soft_reset_slave,
	.get_master_status = hwfcp_get_master_status,
	.stop_charging_config = hwfcp_stop_charging_config,
	.detect_adapter_support_mode = hwfcp_detect_adapter_support_mode,
	.get_support_mode = hwfcp_get_support_mode,
	.get_device_info = hwfcp_get_device_info,
	.set_output_voltage = hwfcp_set_output_voltage,
	.get_output_current = hwfcp_get_output_current,
	.get_max_voltage = hwfcp_get_max_voltage,
	.get_slave_status = hwfcp_get_slave_status,
	.set_default_state = hwfcp_set_default_state,
	.set_default_param = hwfcp_set_default_param,
	.get_protocol_register_state = hwfcp_get_protocol_register_state,
	.is_accp_charger_type = hwfcp_is_accp_charger_type,
};

static int __init hwfcp_init(void)
{
	int ret;
	struct hwfcp_dev *l_dev = NULL;

	l_dev = kzalloc(sizeof(*l_dev), GFP_KERNEL);
	if (!l_dev)
		return -ENOMEM;

	g_hwfcp_dev = l_dev;
	l_dev->dev_id = PROTOCOL_DEVICE_ID_END;

	ret = adapter_protocol_ops_register(&adapter_protocol_hwfcp_ops);
	if (ret)
		goto fail_register_ops;

	return 0;

fail_register_ops:
	kfree(l_dev);
	g_hwfcp_dev = NULL;
	return ret;
}

static void __exit hwfcp_exit(void)
{
	if (!g_hwfcp_dev)
		return;

	kfree(g_hwfcp_dev);
	g_hwfcp_dev = NULL;
}

subsys_initcall_sync(hwfcp_init);
module_exit(hwfcp_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("fcp protocol driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
