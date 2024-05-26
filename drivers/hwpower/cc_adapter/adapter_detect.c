// SPDX-License-Identifier: GPL-2.0
/*
 * adapter_detect.c
 *
 * detect of adapter driver
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

#include <chipset_common/hwpower/adapter/adapter_detect.h>
#include <chipset_common/hwpower/common_module/power_cmdline.h>
#include <chipset_common/hwpower/common_module/power_debug.h>
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <chipset_common/hwpower/common_module/power_interface.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG adapter_detect
HWLOG_REGIST();

static struct adapter_detect_dev *g_adapter_detect_dev;
static int g_sysfs_prot_type_table[SYSFS_PROTOCOL_END] = {
	[SYSFS_PROTOCOL_SCP] = ADAPTER_PROTOCOL_SCP,
	[SYSFS_PROTOCOL_UFCS] = ADAPTER_PROTOCOL_UFCS,
};

static struct adapter_detect_dev *adapter_detect_get_dev(void)
{
	if (!g_adapter_detect_dev) {
		hwlog_err("g_adapter_detect_dev is null\n");
		return NULL;
	}

	return g_adapter_detect_dev;
}

static void adapter_detect_set_plugged_state(bool state)
{
	struct adapter_detect_dev *l_dev = adapter_detect_get_dev();

	if (!l_dev)
		return;

	mutex_lock(&l_dev->plugged_lock);
	l_dev->plugged_state = state;
	mutex_unlock(&l_dev->plugged_lock);
	hwlog_info("set plugged_state=%d\n", state);
}

static bool adapter_detect_get_plugged_state(void)
{
	struct adapter_detect_dev *l_dev = adapter_detect_get_dev();
	bool state = false;

	if (!l_dev)
		return state;

	mutex_lock(&l_dev->plugged_lock);
	state = l_dev->plugged_state;
	mutex_unlock(&l_dev->plugged_lock);

	return state;
}

static void adapter_detect_set_ping_times(unsigned int times)
{
	struct adapter_detect_dev *l_dev = adapter_detect_get_dev();

	if (!l_dev)
		return;

	mutex_lock(&l_dev->ping_lock);
	l_dev->ping_times = times;
	mutex_unlock(&l_dev->ping_lock);
	hwlog_info("set ping_times=%u\n", times);
}

static int adapter_detect_set_prot_type(unsigned int val)
{
	struct adapter_detect_dev *l_dev = adapter_detect_get_dev();

	if (!power_cmdline_is_factory_mode())
		return 0;

	if (!l_dev) {
		hwlog_err("l_dev is null\n");
		return -EPERM;
	}

	l_dev->sysfs_prot_type = val;
	l_dev->init_prot_type_flag = false;
	hwlog_info("set sysfs prot type=%d\n", l_dev->sysfs_prot_type);
	return 0;
}

static int adapter_detect_get_prot_type(unsigned int *val)
{
	struct adapter_detect_dev *l_dev = adapter_detect_get_dev();

	if (!l_dev) {
		hwlog_err("l_dev is null\n");
		return -EPERM;
	}

	*val = l_dev->init_prot_type;
	return 0;
}

static bool adapter_detect_check_sysfs_prot_type(struct adapter_detect_dev *l_dev)
{
	if (!power_cmdline_is_factory_mode())
		return false;

	if ((l_dev->sysfs_prot_type <= SYSFS_PROTOCOL_BEGIN) ||
		(l_dev->sysfs_prot_type >= SYSFS_PROTOCOL_END))
		return false;

	l_dev->init_prot_type = BIT(g_sysfs_prot_type_table[l_dev->sysfs_prot_type]);
	hwlog_info("set init prot type=%d\n", l_dev->init_prot_type);
	return true;
}

void adapter_detect_init_protocol_type(void)
{
	struct adapter_detect_dev *l_dev = adapter_detect_get_dev();
	int i;

	if (!l_dev)
		return;

	if (adapter_detect_check_sysfs_prot_type(l_dev))
		return;

	if (l_dev->init_prot_type_flag)
		return;

	for (i = ADAPTER_PROTOCOL_BEGIN; i < ADAPTER_PROTOCOL_END; i++) {
		if (!adapter_get_protocol_register_state(i))
			l_dev->init_prot_type |= (unsigned int)BIT(i);
	}
	l_dev->init_prot_type_flag = true;

	hwlog_info("init prot_type=0x%x\n", l_dev->init_prot_type);
}

bool adapter_detect_check_protocol_type(unsigned int prot_type)
{
	struct adapter_detect_dev *l_dev = adapter_detect_get_dev();

	if (!l_dev)
		return false;

	return (l_dev->init_prot_type & BIT(prot_type)) ? true : false;
}

static void adapter_detect_set_runtime_protocol_type(unsigned int prot_type)
{
	struct adapter_detect_dev *l_dev = adapter_detect_get_dev();

	if (!l_dev)
		return;

	l_dev->rt_prot_type = prot_type;
	hwlog_info("set rt_prot_type=%x\n", prot_type);
}

unsigned int adapter_detect_get_runtime_protocol_type(void)
{
	struct adapter_detect_dev *l_dev = adapter_detect_get_dev();

	if (!l_dev)
		return 0;

	hwlog_info("get rt_prot_type=%x\n", l_dev->rt_prot_type);
	return l_dev->rt_prot_type;
}

int adapter_detect_ping_ufcs_type(int *adp_mode)
{
	int ret;
	int prot_type = ADAPTER_PROTOCOL_UFCS;
	struct adapter_detect_dev *l_dev = adapter_detect_get_dev();

	if (!l_dev)
		return -EPERM;

	if (l_dev->ufcs_ping_fail_flag)
		return ADAPTER_DETECT_FAIL;

	ret = adapter_detect_adapter_support_mode(prot_type, adp_mode);
	l_dev->ufcs_ping_fail_flag = (ret != ADAPTER_DETECT_SUCC);
	return ret;
}

int adapter_detect_ping_scp_type(int *adp_mode)
{
	int ret;
	int i;
	int retry = ADAPTER_DETECT_RETRY_TIMES;
	int prot_type = ADAPTER_PROTOCOL_SCP;

	ret = adapter_detect_adapter_support_mode(prot_type, adp_mode);
	if (ret != ADAPTER_DETECT_FAIL)
		return ret;

	for (i = 0; (i < retry) && (ret == ADAPTER_DETECT_FAIL); ++i) {
		/* check if the adapter has been plugged out */
		if (adapter_detect_get_plugged_state()) {
			hwlog_info("adapter plugged out, stop detect\n");
			return ADAPTER_DETECT_FAIL;
		}

		/* soft reset adapter */
		if (adapter_soft_reset_slave(prot_type)) {
			hwlog_err("soft reset adapter failed\n");
			break;
		}

		ret = adapter_detect_adapter_support_mode(prot_type, adp_mode);
	}

	if (ret == ADAPTER_DETECT_FAIL) {
		/* check if the adapter has been plugged out */
		if (adapter_detect_get_plugged_state()) {
			hwlog_info("adapter plugged out, stop detect\n");
			return ADAPTER_DETECT_FAIL;
		}

		/* soft reset protocol master */
		if (adapter_soft_reset_master(prot_type)) {
			hwlog_err("soft reset master failed\n");
			return ADAPTER_DETECT_FAIL;
		}

		(void)power_msleep(DT_MSLEEP_2S, 0, NULL);
		ret = adapter_detect_adapter_support_mode(prot_type, adp_mode);
	}

	return ret;
}

int adapter_detect_ping_fcp_type(int *adp_mode)
{
	int prot_type = ADAPTER_PROTOCOL_FCP;

	return adapter_detect_adapter_support_mode(prot_type, adp_mode);
}

static struct adapter_detect_priority_data adapter_detect_priority_data[] = {
	{ ADAPTER_PROTOCOL_UFCS, "ufcs", adapter_detect_ping_ufcs_type, false },
	{ ADAPTER_PROTOCOL_SCP, "scp", adapter_detect_ping_scp_type, true },
};

unsigned int adapter_detect_ping_protocol_type(void)
{
	struct adapter_detect_priority_data *data = adapter_detect_priority_data;
	int data_size = ARRAY_SIZE(adapter_detect_priority_data);
	unsigned int prot_type;
	unsigned int rt_prot_type;
	int adp_mode, i, ret;

	/* return default protocol type when adapter has been plugged out */
	if (adapter_detect_get_plugged_state())
		return 0;

	adapter_detect_init_protocol_type();
	adapter_detect_set_runtime_protocol_type(0);
	rt_prot_type = 0;

	for (i = 0; i < data_size; i++) {
		prot_type = data[i].prot_type;
		if (!adapter_detect_check_protocol_type(prot_type)) {
			hwlog_info("ignore prot_type=%u,%s\n",
				data[i].prot_type, data[i].prot_name);
			continue;
		}

		adp_mode = ADAPTER_SUPPORT_UNDEFINED;
		ret = data[i].ping_cb(&adp_mode);
		hwlog_info("ping prot_type=%u,%s compatible=%d adp_mode=%d ret=%d\n",
			data[i].prot_type, data[i].prot_name, data[i].compatible,
			adp_mode, ret);

		if (ret != ADAPTER_DETECT_SUCC)
			continue;

		if (ret == ADAPTER_DETECT_SUCC) {
			rt_prot_type |= (unsigned int)BIT(prot_type);
			if (!data[i].compatible)
				break;
		}
	}

	adapter_detect_set_runtime_protocol_type(rt_prot_type);

	return rt_prot_type;
}

static ssize_t adapter_detect_dbg_info_show(void *dev_data,
	char *buf, size_t size)
{
	struct adapter_detect_dev *l_dev = dev_data;

	if (!l_dev)
		return -EINVAL;

	adapter_detect_init_protocol_type();

	return scnprintf(buf, size, "init_prot_type=0x%x rt_prot_type=0x%x\n",
		l_dev->init_prot_type, l_dev->rt_prot_type);
}

static int adapter_detect_notifier_call(struct notifier_block *nb,
	unsigned long event, void *data)
{
	struct adapter_detect_dev *l_dev = adapter_detect_get_dev();

	if (!l_dev)
		return NOTIFY_OK;

	switch (event) {
	case POWER_NE_USB_DISCONNECT:
		adapter_detect_set_plugged_state(true);
		adapter_detect_set_ping_times(0);
		break;
	case POWER_NE_USB_CONNECT:
		l_dev->ufcs_ping_fail_flag = false;
		adapter_detect_set_plugged_state(false);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static struct power_if_ops adapter_detect_if_ops = {
	.set_adapter_protocol = adapter_detect_set_prot_type,
	.get_adapter_protocol = adapter_detect_get_prot_type,
	.type_name = "adapter_protocol",
};

static int __init adapter_detect_init(void)
{
	struct adapter_detect_dev *l_dev = NULL;

	l_dev = kzalloc(sizeof(*l_dev), GFP_KERNEL);
	if (!l_dev)
		return -ENOMEM;

	l_dev->plugged_nb.notifier_call = adapter_detect_notifier_call;
	(void)power_event_bnc_register(POWER_BNT_CONNECT, &l_dev->plugged_nb);

	mutex_init(&l_dev->plugged_lock);
	mutex_init(&l_dev->ping_lock);
	g_adapter_detect_dev = l_dev;
	power_dbg_ops_register("adapter_detect", "info", l_dev,
		adapter_detect_dbg_info_show, NULL);
	power_if_ops_register(&adapter_detect_if_ops);
	return 0;
}

static void __exit adapter_detect_exit(void)
{
	struct adapter_detect_dev *l_dev = adapter_detect_get_dev();

	if (!l_dev)
		return;

	power_event_bnc_unregister(POWER_BNT_CONNECT, &l_dev->plugged_nb);
	mutex_destroy(&l_dev->plugged_lock);
	mutex_destroy(&l_dev->ping_lock);
	kfree(l_dev);
	g_adapter_detect_dev = NULL;
}

late_initcall(adapter_detect_init);
module_exit(adapter_detect_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("adapter detect driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
