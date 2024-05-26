// SPDX-License-Identifier: GPL-2.0
/*
 * adapter_protocol_ufcs_handle.c
 *
 * ufcs protocol irq event handle
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

#include "adapter_protocol_ufcs_handle.h"
#include "adapter_protocol_ufcs_base.h"
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG ufcs_protocol_handle
HWLOG_REGIST();

static void hwufcs_handle_ping(struct hwufcs_package_data *pkt)
{
	hwlog_info("handle ping msg\n");
}

static void hwufcs_handle_soft_reset(struct hwufcs_package_data *pkt)
{
	hwlog_info("handle soft_reset msg\n");
}

static void hwufcs_handle_get_sink_info(struct hwufcs_package_data *pkt)
{
	hwlog_info("handle get_sink_info msg\n");
}

static void hwufcs_handle_get_device_info(struct hwufcs_package_data *pkt)
{
	hwlog_info("handle get_device_info msg\n");
}

static void hwufcs_handle_get_error_info(struct hwufcs_package_data *pkt)
{
	hwlog_info("handle get_error_info msg\n");
}

static void hwufcs_handle_detect_cable_info(struct hwufcs_package_data *pkt)
{
	hwlog_info("handle detect_cable_info msg\n");
}

static void hwufcs_handle_start_cable_detect(struct hwufcs_package_data *pkt)
{
	hwlog_info("handle start_cable_detect msg\n");
}

static void hwufcs_handle_end_cable_detect(struct hwufcs_package_data *pkt)
{
	hwlog_info("handle end_cable_detect msg\n");
}

static void hwufcs_handle_exit_ufcs_mode(struct hwufcs_package_data *pkt)
{
	hwlog_info("handle exit_ufcs_mode msg\n");
}

static void hwufcs_handle_output_capabilities(struct hwufcs_package_data *pkt)
{
	hwlog_info("handle output_capabilities msg\n");
}

static void hwufcs_handle_error_info(struct hwufcs_package_data *pkt)
{
	hwlog_info("handle error_info msg\n");
}

static void hwufcs_handle_test_request(struct hwufcs_package_data *pkt)
{
	hwlog_info("handle test_request msg\n");
}

static void hwufcs_handle_ctrl_msg(struct hwufcs_package_data *pkt)
{
	if (!pkt) {
		hwlog_err("msg is null\n");
		return;
	}

	switch (pkt->cmd) {
	case HWUFCS_CTL_MSG_PING:
		hwufcs_handle_ping(pkt);
		break;
	case HWUFCS_CTL_MSG_SOFT_RESET:
		hwufcs_handle_soft_reset(pkt);
		break;
	case HWUFCS_CTL_MSG_GET_SINK_INFO:
		hwufcs_handle_get_sink_info(pkt);
		break;
	case HWUFCS_CTL_MSG_GET_DEVICE_INFO:
		hwufcs_handle_get_device_info(pkt);
		break;
	case HWUFCS_CTL_MSG_GET_ERROR_INFO:
		hwufcs_handle_get_error_info(pkt);
		break;
	case HWUFCS_CTL_MSG_DETECT_CABLE_INFO:
		hwufcs_handle_detect_cable_info(pkt);
		break;
	case HWUFCS_CTL_MSG_START_CABLE_DETECT:
		hwufcs_handle_start_cable_detect(pkt);
		break;
	case HWUFCS_CTL_MSG_END_CABLE_DETECT:
		hwufcs_handle_end_cable_detect(pkt);
		break;
	case HWUFCS_CTL_MSG_EXIT_HWUFCS_MODE:
		hwufcs_handle_exit_ufcs_mode(pkt);
		break;
	default:
		hwlog_err("ctrl cmd invalid\n");
		break;
	}
}

static void hwufcs_handle_data_msg(struct hwufcs_package_data *pkt)
{
	if (!pkt) {
		hwlog_err("msg is null\n");
		return;
	}

	switch (pkt->cmd) {
	case HWUFCS_DATA_MSG_OUTPUT_CAPABILITIES:
		hwufcs_handle_output_capabilities(pkt);
		break;
	case HWUFCS_DATA_MSG_ERROR_INFO:
		hwufcs_handle_error_info(pkt);
		break;
	case HWUFCS_DATA_MSG_TEST_REQUEST:
		hwufcs_handle_test_request(pkt);
		break;
	default:
		hwlog_err("data cmd invalid\n");
		break;
	}
}

static void hwufcs_handle_vendor_define_msg(struct hwufcs_package_data *pkt)
{
	if (!pkt) {
		hwlog_err("msg is null\n");
		return;
	}

	hwlog_info("handle vendor_define_msg\n");
}

static void hwufcs_handle_msg(struct hwufcs_dev *l_dev)
{
	struct hwufcs_package_data pkt = { 0 };
	int ret;

	ret = hwufcs_request_communication(true);
	if (ret)
		return;

	if (hwufcs_receive_msg(&pkt, false)) {
		hwlog_err("receive msg err\n");
		goto end;
	}

	switch (pkt.msg_type) {
	case HWUFCS_MSG_TYPE_CONTROL:
		hwufcs_handle_ctrl_msg(&pkt);
		break;
	case HWUFCS_MSG_TYPE_DATA:
		hwufcs_handle_data_msg(&pkt);
		break;
	case HWUFCS_MSG_TYPE_VENDOR_DEFINED:
		hwufcs_handle_vendor_define_msg(&pkt);
		break;
	default:
		hwlog_err("msg type invalid\n");
		break;
	}

end:
	hwufcs_request_communication(false);
}

int hwufcs_notifier_call(struct notifier_block *nb,
	unsigned long event, void *data)
{
	struct hwufcs_dev *di = container_of(nb, struct hwufcs_dev, event_nb);

	if (!di)
		return NOTIFY_OK;

	switch (event) {
	case POWER_NE_UFCS_REC_UNSOLICITED_DATA:
		hwlog_info("receive unsolicited msg\n");
		hwufcs_handle_msg(di);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}
