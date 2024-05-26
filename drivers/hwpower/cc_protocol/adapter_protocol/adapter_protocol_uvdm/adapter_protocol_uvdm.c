// SPDX-License-Identifier: GPL-2.0
/*
 * adapter_protocol_uvdm.c
 *
 * uvdm protocol driver
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
#include <chipset_common/hwpower/protocol/adapter_protocol_uvdm.h>
#include <chipset_common/hwpower/protocol/adapter_protocol_uvdm_auth.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG uvdm_protocol
HWLOG_REGIST();

static struct hwuvdm_dev *g_hwuvdm_dev;

static const struct adapter_protocol_device_data g_hwuvdm_dev_data[] = {
	{ PROTOCOL_DEVICE_ID_SCHARGER_V600, "scharger_v600" },
};

static struct hwuvdm_dev *hwuvdm_get_dev(void)
{
	if (!g_hwuvdm_dev) {
		hwlog_err("g_hwuvdm_dev is null\n");
		return NULL;
	}

	return g_hwuvdm_dev;
}

static struct hwuvdm_ops *hwuvdm_get_ops(void)
{
	if (!g_hwuvdm_dev || !g_hwuvdm_dev->p_ops) {
		hwlog_err("g_hwuvdm_dev or p_ops is null\n");
		return NULL;
	}

	return g_hwuvdm_dev->p_ops;
}

static int hwuvdm_get_device_id(const char *str)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_hwuvdm_dev_data); i++) {
		if (!strncmp(str, g_hwuvdm_dev_data[i].name, strlen(str)))
			return g_hwuvdm_dev_data[i].id;
	}

	return -EPERM;
}

int hwuvdm_ops_register(struct hwuvdm_ops *ops)
{
	int dev_id;

	if (!g_hwuvdm_dev || !ops || !ops->chip_name) {
		hwlog_err("g_hwuvdm_dev or ops or chip_name is null\n");
		return -EPERM;
	}

	dev_id = hwuvdm_get_device_id(ops->chip_name);
	if (dev_id < 0) {
		hwlog_err("%s ops register fail\n", ops->chip_name);
		return -EPERM;
	}

	g_hwuvdm_dev->p_ops = ops;
	g_hwuvdm_dev->dev_id = dev_id;

	hwlog_info("%d:%s ops register ok\n", dev_id, ops->chip_name);
	return 0;
}

static u32 hwuvdm_package_header_data(u32 cmd)
{
	return (HWUVDM_CMD_DIRECTION_INITIAL << HWUVDM_HDR_SHIFT_CMD_DIRECTTION) |
		(cmd << HWUVDM_HDR_SHIFT_CMD) |
		(HWUVDM_FUNCTION_DC_CTRL << HWUVDM_HDR_SHIFT_FUNCTION) |
		(HWUVDM_VERSION << HWUVDM_HDR_SHIFT_VERSION) |
		(HWUVDM_VDM_TYPE << HWUVDM_HDR_SHIFT_VDM_TYPE) |
		(HWUVDM_VID << HWUVDM_HDR_SHIFT_VID);
}

static int hwuvdm_send_data(u32 *data, u8 cnt, bool wait_rsp)
{
	int i;
	struct hwuvdm_ops *l_ops = NULL;

	l_ops = hwuvdm_get_ops();
	if (!l_ops)
		return -EPERM;

	if (!l_ops->send_data) {
		hwlog_err("send_data is null\n");
		return -EPERM;
	}

	if ((cnt <= HWUVDM_VDOS_COUNT_BEGIN) || (cnt >= HWUVDM_VDOS_COUNT_END)) {
		hwlog_err("invalid vdos count=%d\n", cnt);
		return -EPERM;
	}

	for (i = 0; i < cnt; i++)
		hwlog_info("data[%d] = %2x\n", i, data[i]);

	l_ops->send_data(data, cnt, wait_rsp, l_ops->dev_data);
	return 0;
}

int hwuvdm_handle_vdo_data(u32 *data, int len, bool response,
	unsigned int retrys)
{
	struct hwuvdm_dev *l_dev = hwuvdm_get_dev();
	int ret;
	unsigned int retry_count = 0;

	if (!l_dev)
		return -EPERM;

RETRY:
	/* false: need response */
	ret = hwuvdm_send_data(data, len, false);
	if (ret)
		return -EPERM;

	if (!response)
		return 0;

	ret = wait_for_completion_timeout(&l_dev->rsp_comp,
		msecs_to_jiffies(HWUVDM_WAIT_RESPONSE_TIME));
	reinit_completion(&l_dev->rsp_comp);
	if (!ret) {
		hwlog_err("wait response timeout\n");
		if (retry_count++ < retrys)
			goto RETRY;

		return -EPERM;
	}

	return 0;
}

static int hwuvdm_set_output_voltage(int volt)
{
	u32 data[HWUVDM_VDOS_COUNT_TWO] = { 0 };

	/* data[0]: header */
	data[0] = hwuvdm_package_header_data(HWUVDM_CMD_SET_VOLTAGE);
	/* data[1]: voltage */
	data[1] = volt / HWUVDM_VOLTAGE_UNIT;

	return hwuvdm_handle_vdo_data(data, HWUVDM_VDOS_COUNT_TWO, false, 0);
}

static int hwuvdm_get_output_voltage(int *volt)
{
	u32 data[HWUVDM_VDOS_COUNT_ONE] = { 0 };
	int ret;
	struct hwuvdm_dev *l_dev = hwuvdm_get_dev();

	if (!l_dev || !volt)
		return -EPERM;

	/* data[0]: header */
	data[0] = hwuvdm_package_header_data(HWUVDM_CMD_GET_VOLTAGE);

	ret = hwuvdm_handle_vdo_data(data, HWUVDM_VDOS_COUNT_ONE, true,
		HWUVDM_RETRY_TIMES);
	if (ret)
		return -EPERM;

	*volt = l_dev->info.volt;
	return 0;
}

static int hwuvdm_get_adp_type(int *type)
{
	int ret;
	struct hwuvdm_dev *l_dev = hwuvdm_get_dev();

	if (!l_dev || !type)
		return -EPERM;

	ret = wait_for_completion_timeout(&l_dev->report_type_comp,
		msecs_to_jiffies(HWUVDM_WAIT_RESPONSE_TIME));
	reinit_completion(&l_dev->report_type_comp);
	if (!ret) {
		hwlog_err("get_adp_type wait response timeout\n");
		return -EPERM;
	}

	*type = l_dev->info.power_type;
	return 0;
}

static int hwuvdm_set_encrypt_index(u8 *data, int index)
{
	if (!data)
		return -EPERM;

	data[HWUVDM_KEY_INDEX_OFFSET] = index;

	hwlog_info("set_encrypt_index: %d\n", index);
	return 0;
}

static int hwuvdm_set_random_num(u8 *data, int start, int end)
{
	int i;

	if (!data)
		return -EPERM;

	for (i = start; i <= end; i++)
		get_random_bytes(&data[i], sizeof(u8));

	return 0;
}

static int hwuvdm_send_random_num(u8 *random)
{
	u32 data[HWUVDM_VDOS_COUNT_THREE] = { 0 };

	if (!random)
		return -EPERM;

	/* data[0]: header */
	data[0] = hwuvdm_package_header_data(HWUVDM_CMD_SEND_RANDOM);
	/* data[1],data[2]: random value */
	data[1] = (random[HWUVDM_VDO_BYTE_ZERO] <<
		(HWUVDM_VDO_BYTE_ZERO * HWUVDM_RANDOM_HASH_SHIFT)) |
		(random[HWUVDM_VDO_BYTE_ONE] <<
		(HWUVDM_VDO_BYTE_ONE * HWUVDM_RANDOM_HASH_SHIFT)) |
		(random[HWUVDM_VDO_BYTE_TWO] <<
		(HWUVDM_VDO_BYTE_TWO * HWUVDM_RANDOM_HASH_SHIFT)) |
		(random[HWUVDM_VDO_BYTE_THREE] <<
		(HWUVDM_VDO_BYTE_THREE * HWUVDM_RANDOM_HASH_SHIFT));
	data[2] = (random[HWUVDM_VDO_BYTE_FOUR] <<
		(HWUVDM_VDO_BYTE_ZERO * HWUVDM_RANDOM_HASH_SHIFT)) |
		(random[HWUVDM_VDO_BYTE_FIVE] <<
		(HWUVDM_VDO_BYTE_ONE * HWUVDM_RANDOM_HASH_SHIFT)) |
		(random[HWUVDM_VDO_BYTE_SIX] <<
		(HWUVDM_VDO_BYTE_TWO * HWUVDM_RANDOM_HASH_SHIFT)) |
		(random[HWUVDM_VDO_BYTE_SEVEN] <<
		(HWUVDM_VDO_BYTE_THREE * HWUVDM_RANDOM_HASH_SHIFT));

	return hwuvdm_handle_vdo_data(data, HWUVDM_VDOS_COUNT_THREE, true,
		HWUVDM_RETRY_TIMES);
}

static int hwuvdm_get_encrypted_value(void)
{
	u32 data[HWUVDM_VDOS_COUNT_ONE] = { 0 };

	/* data[0]: header */
	data[0] = hwuvdm_package_header_data(HWUVDM_CMD_GET_HASH);

	return hwuvdm_handle_vdo_data(data, HWUVDM_VDOS_COUNT_ONE, true,
		HWUVDM_RETRY_TIMES);
}

static hwuvdm_copy_hash_value(u8 *hash, int len)
{
	int i;
	struct hwuvdm_dev *l_dev = hwuvdm_get_dev();

	for (i = 0; i < HWUVDM_ENCRYPT_RANDOM_LEN; i++)
		hash[i] = l_dev->encrypt_random_value[i];

	for (i = 0; i < HWUVDM_ENCTYPT_HASH_LEN; i++)
		hash[i + HWUVDM_ENCRYPT_RANDOM_LEN] = l_dev->encrypt_hash_value[i];

	return 0;
}

static int hwuvdm_auth_encrypt_start(int key)
{
	struct hwuvdm_dev *l_dev = hwuvdm_get_dev();
	int ret;

	if (!l_dev)
		return -EPERM;

	/* first: set key index */
	if (hwuvdm_set_encrypt_index(l_dev->encrypt_random_value, key))
		return -EPERM;

	/* second: host create random num */
	if (hwuvdm_set_random_num(l_dev->encrypt_random_value,
		HWUVDM_RANDOM_S_OFFSET, HWUVDM_RANDOM_E_OFFESET))
		return -EPERM;

	/* third: host set random num to slave */
	if (hwuvdm_send_random_num(l_dev->encrypt_random_value))
		return -EPERM;

	/* fourth: host get hash num from slave */
	if (hwuvdm_get_encrypted_value())
		return -EPERM;

	/* fifth: copy hash value */
	hwuvdm_auth_clean_hash_data();
	if (hwuvdm_copy_hash_value(hwuvdm_auth_get_hash_data_header(),
		hwuvdm_auth_get_hash_data_size()))
		return -EPERM;

	/* sixth: wait hash calculate complete */
	ret = hwuvdm_auth_wait_completion();
	hwuvdm_auth_clean_hash_data();

	hwlog_info("auth_encrypt_start\n");
	return ret;
}

static int hwuvdm_set_slave_power_mode(int mode)
{
	u32 data[HWUVDM_VDOS_COUNT_TWO] = { 0 };

	/* data[0]: header */
	data[0] = hwuvdm_package_header_data(HWUVDM_CMD_SWITCH_POWER_MODE);
	/* data[1]: power mode */
	data[1] = mode;

	if (mode == HWUVDM_PWR_MODE_BUCK2SC)
		return hwuvdm_handle_vdo_data(data, HWUVDM_VDOS_COUNT_TWO,
			true, HWUVDM_RETRY_TIMES);
	else
		return hwuvdm_handle_vdo_data(data, HWUVDM_VDOS_COUNT_TWO,
			false, 0);
}

/* reduce the voltage of rx when charge done */
static int hwuvdm_set_rx_reduce_voltage(void)
{
	u32 data[HWUVDM_VDOS_COUNT_ONE] = { 0 };

	/* data[0]: header */
	data[0] = hwuvdm_package_header_data(HWUVDM_CMD_SET_RX_REDUCE_VOLTAGE);

	return hwuvdm_handle_vdo_data(data, HWUVDM_VDOS_COUNT_ONE,
		true, HWUVDM_RETRY_TIMES);
}

static int hwuvdm_get_protocol_register_state(void)
{
	struct hwuvdm_dev *l_dev = hwuvdm_get_dev();

	if (!l_dev)
		return -EPERM;

	if ((l_dev->dev_id >= PROTOCOL_DEVICE_ID_BEGIN) &&
		(l_dev->dev_id < PROTOCOL_DEVICE_ID_END))
		return 0;

	hwlog_info("get_protocol_register_state fail\n");
	return -EPERM;
}

static struct adapter_protocol_ops adapter_protocol_hwuvdm_ops = {
	.type_name = "hw_uvdm",
	.set_output_voltage = hwuvdm_set_output_voltage,
	.get_output_voltage = hwuvdm_get_output_voltage,
	.auth_encrypt_start = hwuvdm_auth_encrypt_start,
	.set_slave_power_mode = hwuvdm_set_slave_power_mode,
	.set_rx_reduce_voltage = hwuvdm_set_rx_reduce_voltage,
	.get_adp_type = hwuvdm_get_adp_type,
	.get_protocol_register_state = hwuvdm_get_protocol_register_state,
};

static int hwuvdm_check_receive_data(u32 data)
{
	struct hwuvdm_header_data hdr;

	hwlog_info("uvdm_header: 0x%02x\n", data);

	hdr.vdm_type = (data >> HWUVDM_HDR_SHIFT_VDM_TYPE) & HWUVDM_MASK_VDM_TYPE;
	hdr.vid = (data >> HWUVDM_HDR_SHIFT_VID) & HWUVDM_MASK_VID;

	if ((hdr.vdm_type == HWUVDM_VDM_TYPE) && (hdr.vid == HWUVDM_VID))
		return 0;

	hwlog_err("invalid uvdm header\n");
	return -EPERM;
}

static void hwuvdm_report_power_type(u32 data)
{
	struct hwuvdm_dev *l_dev = hwuvdm_get_dev();

	if (!l_dev)
		return;

	l_dev->info.power_type = data & HWUVDM_MASK_POWER_TYPE;
	complete(&l_dev->report_type_comp);
}

static void hwuvdm_send_random(u32 data)
{
	struct hwuvdm_dev *l_dev = hwuvdm_get_dev();

	if (!l_dev)
		return;

	if ((data & HWUVDM_MASK_RESPONSE_STATE) == HWUVDM_RESPONSE_TX_INTERRUPT)
		complete(&l_dev->rsp_comp);
}

static void hwuvdm_switch_power_mode(u32 data)
{
	struct hwuvdm_dev *l_dev = hwuvdm_get_dev();

	if (!l_dev)
		return;

	hwlog_info("switch power mode data = %x\n", data);
	if ((data & HWUVDM_MASK_RESPONSE_STATE) == HWUVDM_RESPONSE_POWER_READY)
		complete(&l_dev->rsp_comp);
}

static void hwuvdm_get_voltage(u32 data)
{
	struct hwuvdm_dev *l_dev = hwuvdm_get_dev();

	if (!l_dev)
		return;

	l_dev->info.volt = (data & HWUVDM_MASK_VOLTAGE) * HWUVDM_VOLTAGE_UNIT;
	complete(&l_dev->rsp_comp);
}

static void hwuvdm_get_hash(u32 *data, int len)
{
	int i, j, k;
	struct hwuvdm_dev *l_dev = hwuvdm_get_dev();

	if (!l_dev)
		return;

	if (len > HWUVDM_VDOS_COUNT_THREE)
		return;

	/* data[1],data[2]: hash values */
	for (i = 0; i < HWUVDM_ENCTYPT_HASH_LEN; i++) {
		k = i / HWUVDM_VDO_BYTE_FOUR + 1; /* k = {1, 2} */
		j = i % HWUVDM_VDO_BYTE_FOUR * HWUVDM_RANDOM_HASH_SHIFT;
		l_dev->encrypt_hash_value[i] = data[k] >> j & HWUVDM_MASK_KEY;
	}
	complete(&l_dev->rsp_comp);
}

static void hwuvdm_report_otg_event(u32 data)
{
	struct hwuvdm_dev *l_dev = hwuvdm_get_dev();

	if (!l_dev)
		return;

	l_dev->info.otg_event = data & HWUVDM_MASK_OTG_EVENT;
	power_event_anc_notify(POWER_ANT_UVDM_FAULT,
		POWER_NE_UVDM_FAULT_OTG, &l_dev->info.otg_event);
}

static void hwuvdm_report_abnormal_event(u32 data)
{
	struct hwuvdm_dev *l_dev = hwuvdm_get_dev();

	if (!l_dev)
		return;

	l_dev->info.abnormal_flag = data & HWUVDM_MASK_REPORT_ABNORMAL;
	power_event_anc_notify(POWER_ANT_UVDM_FAULT,
		POWER_NE_UVDM_FAULT_COVER_ABNORMAL, &l_dev->info.abnormal_flag);
}

static void hwuvdm_handle_receive_dc_crtl_data(u32 *vdo, int len)
{
	u32 cmd;
	u32 vdo_hdr;
	u32 vdo_data0;

	vdo_hdr = vdo[0]; /* vdo[0]: uvdm header */
	vdo_data0 = vdo[1]; /* vdo[1]: the first data of uvdm data */

	cmd = (vdo_hdr >> HWUVDM_HDR_SHIFT_CMD) & HWUVDM_MASK_CMD;
	switch (cmd) {
	case HWUVDM_CMD_REPORT_POWER_TYPE:
		hwuvdm_report_power_type(vdo_data0);
		break;
	case HWUVDM_CMD_REPORT_ABNORMAL:
		hwuvdm_report_abnormal_event(vdo_data0);
		break;
	case HWUVDM_CMD_SEND_RANDOM:
		hwuvdm_send_random(vdo_data0);
		break;
	case HWUVDM_CMD_SWITCH_POWER_MODE:
		hwuvdm_switch_power_mode(vdo_data0);
		break;
	case HWUVDM_CMD_GET_VOLTAGE:
		hwuvdm_get_voltage(vdo_data0);
		break;
	case HWUVDM_CMD_GET_HASH:
		hwuvdm_get_hash(vdo, HWUVDM_VDOS_COUNT_THREE);
		break;
	default:
		break;
	}
}

static void hwuvdm_handle_receive_pd_crtl_data(u32 *vdo, int len)
{
	u32 cmd;
	u32 vdo_hdr;
	u32 vdo_data0;

	vdo_hdr = vdo[0]; /* vdo[0]: uvdm header */
	vdo_data0 = vdo[1]; /* vdo[1]: the first data of uvdm data */

	cmd = (vdo_hdr >> HWUVDM_HDR_SHIFT_CMD) & HWUVDM_MASK_CMD;
	switch (cmd) {
	case HWUVDM_CMD_REPORT_OTG_EVENT:
		hwuvdm_report_otg_event(vdo_data0);
		break;
	default:
		break;
	}
}

static int hwuvdm_handle_receive_data(void *data)
{
	u32 vdo[HWUVDM_VDOS_COUNT_SEVEN] = { 0 };
	u32 vdo_hdr;
	u32 func;
	int ret;

	if (!data)
		return -EPERM;

	memcpy(vdo, data, sizeof(vdo));
	vdo_hdr = vdo[0]; /* vdo[0]: uvdm header */
	ret = hwuvdm_check_receive_data(vdo_hdr);
	if (ret)
		return -EPERM;

	func = (vdo_hdr >> HWUVDM_HDR_SHIFT_FUNCTION) & HWUVDM_MASK_FUNCTION;
	switch (func) {
	case HWUVDM_FUNCTION_DC_CTRL:
		hwuvdm_handle_receive_dc_crtl_data(vdo, HWUVDM_VDOS_COUNT_THREE);
		break;
	case HWUVDM_FUNCTION_PD_CTRL:
		hwuvdm_handle_receive_pd_crtl_data(vdo, HWUVDM_VDOS_COUNT_THREE);
		break;
	case HWUVDM_FUNCTION_USB_EXT_MODEM:
		power_event_bnc_notify(POWER_BNT_USB_EXT_MODEM,
			POWER_NE_UEM_RECEIVE_UVDM_DATA, vdo);
		break;
	default:
		break;
	}

	return 0;
}

static int hwuvdm_notifier_call(struct notifier_block *nb,
	unsigned long event, void *data)
{
	struct hwuvdm_dev *l_dev = hwuvdm_get_dev();

	if (!l_dev)
		return NOTIFY_OK;

	switch (event) {
	case POWER_NE_UVDM_RECEIVE:
		hwuvdm_handle_receive_data(data);
		return NOTIFY_OK;
	default:
		return NOTIFY_OK;
	}
}

static int __init hwuvdm_init(void)
{
	int ret;
	struct hwuvdm_dev *l_dev = NULL;

	l_dev = kzalloc(sizeof(*l_dev), GFP_KERNEL);
	if (!l_dev)
		return -ENOMEM;

	g_hwuvdm_dev = l_dev;
	l_dev->dev_id = PROTOCOL_DEVICE_ID_END;

	init_completion(&l_dev->rsp_comp);
	init_completion(&l_dev->report_type_comp);
	l_dev->nb.notifier_call = hwuvdm_notifier_call;
	ret = power_event_bnc_register(POWER_BNT_UVDM, &l_dev->nb);
	if (ret)
		goto fail_free_mem;

	ret = adapter_protocol_ops_register(&adapter_protocol_hwuvdm_ops);
	if (ret)
		goto fail_unregister_notifier;

	return 0;

fail_unregister_notifier:
	power_event_bnc_unregister(POWER_BNT_UVDM, &l_dev->nb);
fail_free_mem:
	kfree(l_dev);
	g_hwuvdm_dev = NULL;
	return ret;
}

static void __exit hwuvdm_exit(void)
{
	if (!g_hwuvdm_dev)
		return;

	power_event_bnc_unregister(POWER_BNT_UVDM, &g_hwuvdm_dev->nb);
	kfree(g_hwuvdm_dev);
	g_hwuvdm_dev = NULL;
}

subsys_initcall_sync(hwuvdm_init);
module_exit(hwuvdm_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("uvdm protocol driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
