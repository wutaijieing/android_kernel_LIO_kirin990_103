// SPDX-License-Identifier: GPL-2.0
/*
 * adapter_protocol_ufcs.c
 *
 * ufcs protocol driver
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

#include "adapter_protocol_ufcs_base.h"
#include "adapter_protocol_ufcs_interface.h"
#include "adapter_protocol_ufcs_handle.h"
#include <chipset_common/hwpower/protocol/adapter_protocol_ufcs_auth.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG ufcs_protocol
HWLOG_REGIST();

static struct hwufcs_dev *g_hwufcs_dev;

static struct hwufcs_dev *hwufcs_get_dev(void)
{
	if (!g_hwufcs_dev) {
		hwlog_err("g_hwufcs_dev is null\n");
		return NULL;
	}

	return g_hwufcs_dev;
}

static int hwufcs_get_output_capabilities(struct hwufcs_dev *l_dev)
{
	int ret;

	if (!l_dev)
		return -EPERM;

	if (l_dev->info.outout_capabilities_rd_flag == HAS_READ_FLAG)
		return 0;

	ret = hwufcs_request_communication(true);
	if (ret)
		return -EPERM;

	ret = hwufcs_send_control_msg(HWUFCS_CTL_MSG_GET_OUTPUT_CAPABILITIES, true);
	if (ret)
		goto end;

	l_dev->info.output_mode = 0;
	ret = hwufcs_receive_output_capabilities_data_msg(
		&l_dev->info.cap[0], &l_dev->info.mode_quantity);
	if (ret)
		goto end;

	l_dev->info.outout_capabilities_rd_flag = HAS_READ_FLAG;

	/* init output_mode & volt & curr */
	l_dev->info.output_mode = 1;
	l_dev->info.output_volt = l_dev->info.cap[0].max_volt;
	l_dev->info.output_curr = l_dev->info.cap[0].max_curr;

end:
	(void)hwufcs_request_communication(false);
	return ret;
}

static int hwufcs_get_source_info(struct hwufcs_source_info_data *p)
{
	int ret;

	/* delay 20ms to avoid send msg densely */
	power_usleep(DT_USLEEP_20MS);

	ret = hwufcs_request_communication(true);
	if (ret)
		return -EPERM;

	ret = hwufcs_send_control_msg(HWUFCS_CTL_MSG_GET_SOURCE_INFO, true);
	if (ret)
		goto end;

	ret = hwufcs_receive_source_info_data_msg(p);

end:
	(void)hwufcs_request_communication(false);
	return ret;
}

static int hwufcs_get_dev_info(void)
{
	struct hwufcs_dev *l_dev = hwufcs_get_dev();
	int ret;

	if (!l_dev)
		return -EPERM;

	if (l_dev->info.dev_info_rd_flag == HAS_READ_FLAG)
		return 0;

	ret = hwufcs_request_communication(true);
	if (ret)
		return -EPERM;

	ret = hwufcs_send_control_msg(HWUFCS_CTL_MSG_GET_DEVICE_INFO, true);
	if (ret)
		goto end;

	ret = hwufcs_receive_dev_info_data_msg(&l_dev->info.dev_info);
	if (ret)
		goto end;

	l_dev->info.dev_info_rd_flag = HAS_READ_FLAG;

end:
	(void)hwufcs_request_communication(false);
	return ret;
}

static int hwufcs_send_dev_info(void)
{
	struct hwufcs_dev *l_dev = hwufcs_get_dev();
	int ret;

	if (!l_dev)
		return -EPERM;

	ret = hwufcs_receive_control_msg(HWUFCS_CTL_MSG_GET_DEVICE_INFO, false);
	if (ret)
		return -EPERM;

	ret = hwufcs_send_device_information_data_msg(&l_dev->info.dev_info);
	if (ret)
		return -EPERM;

	return 0;
}

static int hwufcs_config_watchdog(struct hwufcs_wtg_data *p)
{
	int ret;

	ret = hwufcs_request_communication(true);
	if (ret)
		return -EPERM;

	ret = hwufcs_send_config_watchdog_data_msg(p);
	if (ret)
		goto end;

	ret = hwufcs_receive_control_msg(HWUFCS_CTL_MSG_ACCEPT, false);

end:
	(void)hwufcs_request_communication(false);
	return ret;
}

static int hwufcs_set_default_state(void)
{
	int ret;

	ret = hwufcs_request_communication(true);
	if (ret)
		return -EPERM;

	ret = hwufcs_send_control_msg(HWUFCS_CTL_MSG_EXIT_HWUFCS_MODE, true);
	ret += hwufcs_request_communication(false);

	return ret;
}

static int hwufcs_set_default_param(void)
{
	struct hwufcs_dev *l_dev = hwufcs_get_dev();

	if (!l_dev)
		return -EPERM;

	(void)hwufcs_request_communication(false);
	memset(&l_dev->info, 0, sizeof(l_dev->info));
	return 0;
}

static int hwufcs_detect_adapter_support_mode(int *mode)
{
	struct hwufcs_dev *l_dev = hwufcs_get_dev();
	int ret;
	u8 cap_index;
	struct hwufcs_wtg_data wtg = { 0 };

	if (!l_dev || !mode)
		return ADAPTER_DETECT_FAIL;

	/* set all parameter to default state */
	hwufcs_set_default_param();
	l_dev->info.detect_finish_flag = 1; /* has detect adapter */
	l_dev->info.support_mode = ADAPTER_SUPPORT_UNDEFINED;

	/* protocol handshark: detect ufcs adapter */
	ret = hwufcs_detect_adapter();
	if (ret == HWUFCS_DETECT_FAIL) {
		hwlog_err("ufcs adapter detect fail\n");
		return ADAPTER_DETECT_FAIL;
	}

	ret = hwufcs_request_communication(true);
	if (ret)
		return -EPERM;

	ret = hwufcs_send_control_msg(HWUFCS_CTL_MSG_PING, true);
	(void)hwufcs_request_communication(false);
	if (ret)
		return ADAPTER_DETECT_FAIL;

	hwlog_info("ufcs adapter ping succ\n");

	hwufcs_config_watchdog(&wtg); /* disable wdt func */
	if (hwufcs_get_output_capabilities(l_dev))
		return ADAPTER_DETECT_FAIL;

	cap_index = l_dev->info.mode_quantity - HWUFCS_REQ_BASE_OUTPUT_MODE;
	if (l_dev->info.cap[cap_index].max_volt >= HWUFCS_REQ_LVC_OUTPUT_VOLT)
		*mode |= ADAPTER_SUPPORT_LVC;
	if (l_dev->info.cap[cap_index].max_volt >= HWUFCS_REQ_SC_OUTPUT_VOLT)
		*mode |= ADAPTER_SUPPORT_SC;
	if (l_dev->info.cap[cap_index].max_volt >= HWUFCS_REQ_SC4_OUTPUT_VOLT)
		*mode |= ADAPTER_SUPPORT_SC4;

	l_dev->info.support_mode = *mode;
	return ADAPTER_DETECT_SUCC;
}

static int hwufcs_get_support_mode(int *mode)
{
	struct hwufcs_dev *l_dev = hwufcs_get_dev();

	if (!l_dev || !mode)
		return -EPERM;

	if (l_dev->info.detect_finish_flag)
		*mode = l_dev->info.support_mode;
	else
		hwufcs_detect_adapter_support_mode(mode);

	hwlog_info("support_mode: %d\n", *mode);
	return 0;
}

static int hwufcs_set_init_data(struct adapter_init_data *data)
{
	struct hwufcs_wtg_data wtg;

	wtg.time = data->watchdog_timer * HWUFCS_WTG_UNIT_TIME;
	if (hwufcs_config_watchdog(&wtg))
		return -EPERM;

	hwlog_info("set_init_data\n");
	return 0;
}

static int hwufcs_soft_reset_slave(void)
{
	hwlog_info("soft_reset_slave\n");
	return 0;
}

static int hwufcs_get_inside_temp(int *temp)
{
	struct hwufcs_source_info_data data;

	if (!temp)
		return -EPERM;

	if (hwufcs_get_source_info(&data))
		return -EPERM;

	*temp = data.dev_temp;

	hwlog_info("get_inside_temp: %d\n", *temp);
	return 0;
}

static int hwufcs_get_port_temp(int *temp)
{
	struct hwufcs_source_info_data data;

	if (!temp)
		return -EPERM;

	if (hwufcs_get_source_info(&data))
		return -EPERM;

	*temp = data.port_temp;

	hwlog_info("get_port_temp: %d\n", *temp);
	return 0;
}

static int hwufcs_get_chip_vendor_id(int *id)
{
	struct hwufcs_dev *l_dev = hwufcs_get_dev();

	if (!l_dev || !id)
		return -EPERM;

	if (hwufcs_get_dev_info())
		return -EPERM;

	*id = l_dev->info.dev_info.chip_vid;

	hwlog_info("get_chip_vendor_id: %d\n", *id);
	return 0;
}

static int hwufcs_get_chip_id(int *id)
{
	struct hwufcs_dev *l_dev = hwufcs_get_dev();

	if (!l_dev || !id)
		return -EPERM;

	if (hwufcs_get_dev_info())
		return -EPERM;

	*id = l_dev->info.dev_info.manu_vid;

	hwlog_info("get_chip_id_f: 0x%x\n", *id);
	return 0;
}

static int hwufcs_get_hw_version_id(int *id)
{
	struct hwufcs_dev *l_dev = hwufcs_get_dev();

	if (!l_dev || !id)
		return -EPERM;

	if (hwufcs_get_dev_info())
		return -EPERM;

	*id = l_dev->info.dev_info.hw_ver;

	hwlog_info("get_hw_version_id_f: 0x%x\n", *id);
	return 0;
}

static int hwufcs_get_sw_version_id(int *id)
{
	struct hwufcs_dev *l_dev = hwufcs_get_dev();

	if (!l_dev || !id)
		return -EPERM;

	if (hwufcs_get_dev_info())
		return -EPERM;

	*id = l_dev->info.dev_info.sw_ver;

	hwlog_info("get_sw_version_id_f: 0x%x\n", *id);
	return 0;
}

static int hwufcs_get_adp_type(int *type)
{
	if (!type)
		return -EPERM;

	*type = ADAPTER_TYPE_UNKNOWN;
	return 0;
}

static int hwufcs_check_output_mode(struct hwufcs_device_info *info, int volt, int curr)
{
	int i;

	for (i = 0; i < HWUFCS_CAP_MAX_OUTPUT_MODE; i++) {
		if ((volt > info->cap[i].min_volt) &&
			(volt <= info->cap[i].max_volt) &&
			(curr <= info->cap[i].max_curr)) {
			info->output_mode = info->cap[i].output_mode;
			return 0;
		}
	}

	info->output_mode = HWUFCS_REQ_BASE_OUTPUT_MODE;
	hwlog_err("choose mode fail, request volt=%d, curr=%d\n", volt, curr);
	return -EPERM;
}

static int hwufcs_set_output_voltage(int volt)
{
	struct hwufcs_dev *l_dev = hwufcs_get_dev();
	struct hwufcs_request_data req;
	u8 cap_index;
	int ret, i;

	if (!l_dev)
		return -EPERM;

	ret = hwufcs_check_output_mode(&l_dev->info, volt, l_dev->info.output_curr);
	if (ret)
		return -EPERM;

	/* save current output voltage */
	l_dev->info.output_volt = volt;

	cap_index = l_dev->info.output_mode - HWUFCS_REQ_BASE_OUTPUT_MODE;
	req.output_mode = l_dev->info.output_mode;
	req.output_curr = l_dev->info.output_curr;
	req.output_volt = volt;

	/* delay 20ms to avoid send msg densely */
	power_usleep(DT_USLEEP_20MS);

	ret = hwufcs_request_communication(true);
	if (ret)
		return -EPERM;

	ret = hwufcs_send_request_data_msg(&req);
	if (ret)
		goto end;

	ret = hwufcs_receive_control_msg(HWUFCS_CTL_MSG_ACCEPT, false);
	if (ret)
		goto end;

	for (i = 0; i < HWUFCS_POWER_READY_RETRY; i++) {
		ret = hwufcs_receive_control_msg(HWUFCS_CTL_MSG_POWER_READY, false);
		if (!ret) {
			hwlog_info("set_output_voltage: %d\n", volt);
			break;
		}
	}

end:
	(void)hwufcs_request_communication(false);
	return ret;
}

static int hwufcs_set_output_current(int cur)
{
	struct hwufcs_dev *l_dev = hwufcs_get_dev();
	struct hwufcs_request_data req;
	u8 cap_index;
	int ret, i;

	if (!l_dev)
		return -EPERM;

	ret = hwufcs_check_output_mode(&l_dev->info, l_dev->info.output_volt, cur);
	if (ret)
		return -EPERM;

	/* save current output current */
	l_dev->info.output_curr = cur;

	cap_index = l_dev->info.output_mode - HWUFCS_REQ_BASE_OUTPUT_MODE;
	req.output_mode = l_dev->info.output_mode;
	req.output_curr = cur;
	req.output_volt = l_dev->info.output_volt;

	/* delay 20ms to avoid send msg densely */
	power_usleep(DT_USLEEP_20MS);

	ret = hwufcs_request_communication(true);
	if (ret)
		return -EPERM;

	ret = hwufcs_send_request_data_msg(&req);
	if (ret)
		goto end;

	ret = hwufcs_receive_control_msg(HWUFCS_CTL_MSG_ACCEPT, false);
	if (ret)
		goto end;

	for (i = 0; i < HWUFCS_POWER_READY_RETRY; i++) {
		ret = hwufcs_receive_control_msg(HWUFCS_CTL_MSG_POWER_READY, false);
		if (!ret) {
			hwlog_info("set_output_current: %d\n", cur);
			break;
		}
	}

end:
	(void)hwufcs_request_communication(false);
	return ret;
}

static int hwufcs_get_output_voltage(int *volt)
{
	struct hwufcs_source_info_data data;

	if (!volt)
		return -EPERM;

	if (hwufcs_get_source_info(&data))
		return -EPERM;

	*volt = data.output_volt;

	hwlog_info("get_output_voltage: %d\n", *volt);
	return 0;
}

static int hwufcs_get_output_current(int *cur)
{
	struct hwufcs_source_info_data data;

	if (!cur)
		return -EPERM;

	if (hwufcs_get_source_info(&data))
		return -EPERM;

	*cur = data.output_curr;

	hwlog_info("get_output_current: %d\n", *cur);
	return 0;
}

static int hwufcs_get_output_current_set(int *cur)
{
	struct hwufcs_dev *l_dev = hwufcs_get_dev();

	if (!l_dev || !cur)
		return -EPERM;

	*cur = l_dev->info.output_curr;

	hwlog_info("get_output_current_set: %d\n", *cur);
	return 0;
}

static int hwufcs_get_power_curve_num(int *num)
{
	struct hwufcs_dev *l_dev = hwufcs_get_dev();

	if (!l_dev || !num)
		return -EPERM;

	if (hwufcs_get_output_capabilities(l_dev))
		return -EPERM;

	*num = l_dev->info.mode_quantity;
	hwlog_info("get power_curve_num=%d\n", l_dev->info.mode_quantity);
	return 0;
}

static int hwufcs_get_power_curve(int *val, unsigned int size)
{
	unsigned int i;
	struct hwufcs_dev *l_dev = hwufcs_get_dev();

	if (!val || (size > HWUFCS_POWER_CURVE_SIZE))
		return -EPERM;

	if (hwufcs_get_output_capabilities(l_dev))
		return -EPERM;

	/* sync output cap to power curve: 0 max volt; 1 max curr */
	for (i = 0; i < size; i++) {
		if (i % 2 == 0)
			val[i] = l_dev->info.cap[i / 2].max_volt;
		else
			val[i] = l_dev->info.cap[i / 2].max_curr;
	}

	return 0;
}

static int hwufcs_get_min_voltage(int *volt)
{
	struct hwufcs_dev *l_dev = hwufcs_get_dev();

	if (!l_dev || !volt)
		return -EPERM;

	if (hwufcs_get_output_capabilities(l_dev))
		return -EPERM;

	if (l_dev->info.mode_quantity < HWUFCS_REQ_BASE_OUTPUT_MODE)
		return -EPERM;

	*volt = l_dev->info.cap[0].min_volt;
	hwlog_info("get_min_voltage: %d\n", *volt);
	return 0;
}

static int hwufcs_get_max_voltage(int *volt)
{
	struct hwufcs_dev *l_dev = hwufcs_get_dev();
	u8 cap_index;

	if (!l_dev || !volt)
		return -EPERM;

	if (hwufcs_get_output_capabilities(l_dev))
		return -EPERM;

	cap_index = l_dev->info.mode_quantity - HWUFCS_REQ_BASE_OUTPUT_MODE;
	*volt = l_dev->info.cap[cap_index].max_volt;

	hwlog_info("get_max_voltage: %d\n", *volt);
	return 0;
}

static int hwufcs_get_min_current(int *cur)
{
	struct hwufcs_dev *l_dev = hwufcs_get_dev();
	u16 min_curr = l_dev->info.cap[0].min_curr;
	int i;

	if (!l_dev || !cur)
		return -EPERM;

	if (hwufcs_get_output_capabilities(l_dev))
		return -EPERM;

	for (i = 1; i < l_dev->info.mode_quantity; i++) {
		if (min_curr > l_dev->info.cap[i].min_curr)
			min_curr = l_dev->info.cap[i].min_curr;
	}

	*cur = min_curr;
	hwlog_info("get_min_current: %d\n", *cur);
	return 0;
}

static int hwufcs_get_max_current(int *cur)
{
	struct hwufcs_dev *l_dev = hwufcs_get_dev();
	u16 max_curr = l_dev->info.cap[0].max_curr;
	int i;

	if (!l_dev || !cur)
		return -EPERM;

	if (hwufcs_get_output_capabilities(l_dev))
		return -EPERM;

	for (i = 1; i < l_dev->info.mode_quantity; i++) {
		if (max_curr < l_dev->info.cap[i].max_curr)
			max_curr = l_dev->info.cap[i].max_curr;
	}

	*cur = max_curr;
	hwlog_info("get_max_current: %d\n", *cur);
	return 0;
}

static int hwufcs_get_power_drop_current(int *cur)
{
	return 0;
}

static int hwufcs_auth_encrypt_start(int key)
{
	struct hwufcs_verify_request_data verify_req;
	struct hwufcs_verify_response_data verify_rsp;
	int ret, i;
	u8 *hash = hwufcs_auth_get_hash_data_header();

	if (!hash)
		return -EPERM;

	/* first: set key index */
	verify_req.encrypt_index = key;

	/* second: host set random num to slave */
	for (i = 0; i < HWUFCS_RANDOM_SIZE; i++)
		get_random_bytes(&verify_req.random[i], sizeof(u8));

	ret = hwufcs_request_communication(true);
	if (ret)
		return -EPERM;

	ret = hwufcs_send_verify_request_data_msg(&verify_req);
	if (ret)
		goto end;

	ret = hwufcs_receive_control_msg(HWUFCS_CTL_MSG_ACCEPT, false);
	if (ret)
		goto end;

	ret = hwufcs_send_dev_info();
	if (ret)
		goto end;

	ret = hwufcs_receive_verify_response_data_msg(&verify_rsp);
	if (ret)
		goto end;

	(void)hwufcs_request_communication(false);

	/* third: copy hash value */
	hwufcs_auth_clean_hash_data();
	for (i = 0; i < HWUFCS_RANDOM_SIZE; i++)
		hash[i] = verify_req.random[i];
	for (i = 0; i < HWUFCS_RANDOM_SIZE; i++)
		hash[i + HWUFCS_RANDOM_SIZE] = verify_rsp.random[i];
	for (i = 0; i < HWUFCS_ENCRYPT_SIZE; i++)
		hash[i + HWUFCS_RANDOM_SIZE * 2] = verify_rsp.encrypt[i];
	hash[HWUFCS_AUTH_HASH_LEN - 1] = (unsigned char)key;

	/* forth: wait hash calculate complete */
	ret = hwufcs_auth_wait_completion();
end:
	hwlog_info("auth_encrypt_start\n");
	(void)hwufcs_request_communication(false);
	hwufcs_auth_clean_hash_data();
	return ret;
}

static int hwufcs_get_device_info(struct adapter_device_info *info)
{
	struct hwufcs_dev *l_dev = hwufcs_get_dev();

	if (!l_dev || !info)
		return -EPERM;

	if (hwufcs_get_chip_id(&info->chip_id))
		return -EPERM;

	if (hwufcs_get_hw_version_id(&info->hwver))
		return -EPERM;

	if (hwufcs_get_sw_version_id(&info->fwver))
		return -EPERM;

	if (hwufcs_get_min_voltage(&info->min_volt))
		return -EPERM;

	if (hwufcs_get_max_voltage(&info->max_volt))
		return -EPERM;

	if (hwufcs_get_min_current(&info->min_cur))
		return -EPERM;

	if (hwufcs_get_max_current(&info->max_cur))
		return -EPERM;

	info->volt_step = l_dev->info.cap[0].volt_step;
	info->curr_step = l_dev->info.cap[0].curr_step;
	info->output_mode = l_dev->info.cap[0].output_mode;

	hwlog_info("get_device_info\n");
	return 0;
}

static int hwufcs_get_protocol_register_state(void)
{
	return hwufcs_check_dev_id();
}

static struct adapter_protocol_ops adapter_protocol_hwufcs_ops = {
	.type_name = "hw_ufcs",
	.detect_adapter_support_mode = hwufcs_detect_adapter_support_mode,
	.get_support_mode = hwufcs_get_support_mode,
	.set_default_state = hwufcs_set_default_state,
	.set_default_param = hwufcs_set_default_param,
	.set_init_data = hwufcs_set_init_data,
	.soft_reset_master = hwufcs_soft_reset_master,
	.soft_reset_slave = hwufcs_soft_reset_slave,
	.get_chip_vendor_id = hwufcs_get_chip_vendor_id,
	.get_adp_type = hwufcs_get_adp_type,
	.get_inside_temp = hwufcs_get_inside_temp,
	.get_port_temp = hwufcs_get_port_temp,
	.set_output_voltage = hwufcs_set_output_voltage,
	.get_output_voltage = hwufcs_get_output_voltage,
	.set_output_current = hwufcs_set_output_current,
	.get_output_current = hwufcs_get_output_current,
	.get_output_current_set = hwufcs_get_output_current_set,
	.get_min_voltage = hwufcs_get_min_voltage,
	.get_max_voltage = hwufcs_get_max_voltage,
	.get_min_current = hwufcs_get_min_current,
	.get_max_current = hwufcs_get_max_current,
	.get_power_drop_current = hwufcs_get_power_drop_current,
	.get_power_curve_num = hwufcs_get_power_curve_num,
	.get_power_curve = hwufcs_get_power_curve,
	.auth_encrypt_start = hwufcs_auth_encrypt_start,
	.get_device_info = hwufcs_get_device_info,
	.get_protocol_register_state = hwufcs_get_protocol_register_state,
};

static int __init hwufcs_init(void)
{
	int ret;
	struct hwufcs_dev *l_dev = NULL;

	l_dev = kzalloc(sizeof(*l_dev), GFP_KERNEL);
	if (!l_dev)
		return -ENOMEM;

	g_hwufcs_dev = l_dev;
	l_dev->event_nb.notifier_call = hwufcs_notifier_call;

	ret = power_event_bnc_register(POWER_BNT_UFCS, &l_dev->event_nb);
	if (ret)
		goto fail_register_ops;

	ret = adapter_protocol_ops_register(&adapter_protocol_hwufcs_ops);
	if (ret)
		goto fail_register_ops;

	hwufcs_init_msg_number_lock();
	return 0;

fail_register_ops:
	kfree(l_dev);
	g_hwufcs_dev = NULL;
	return ret;
}

static void __exit hwufcs_exit(void)
{
	if (!g_hwufcs_dev)
		return;

	hwufcs_destroy_msg_number_lock();
	kfree(g_hwufcs_dev);
	g_hwufcs_dev = NULL;
}

subsys_initcall_sync(hwufcs_init);
module_exit(hwufcs_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ufcs protocol driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
