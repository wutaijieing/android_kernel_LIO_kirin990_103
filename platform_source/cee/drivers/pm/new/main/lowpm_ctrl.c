/*
 * lowpm_ctrl.c
 *
 * The AP Kernel communicates with LPM3 through IPC and
 * passes some SR control of the application layer.
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
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

#include "lowpm_ctrl.h"
#include "helper/log/lowpm_log.h"

#include <linux/platform_drivers/lpm_ctrl.h>
#include <platform_include/basicplatform/linux/ipc_msg.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>
#include <linux/export.h>
#include <securec.h>
#include <linux/init.h>
#include <linux/kobject.h>

#ifdef CONFIG_SR_SWITCH_32K
#define ACK_BUFFER_LEN		2
#define SWITCH_32K_BUFFER_LEN		20

static int g_lowpm_32k_switch;

static int get_32k_switch_from_lpmcu(void)
{
	rproc_msg_t ack[ACK_BUFFER_LEN] = {0};
	rproc_id_t mbx = IPC_ACPU_LPM3_MBX_2;
	union ipc_data msg;
	int ret;

	msg.cmd_mix.cmd_src = OBJ_AP;
	msg.cmd_mix.cmd_obj = OBJ_AP;
	msg.cmd_mix.cmd = CMD_INQUIRY;
	msg.cmd_mix.cmd_type = TYPE_SR;
	ret = memset_s(msg.cmd_mix.cmd_para, sizeof(msg.cmd_mix.cmd_para),
			0, sizeof(msg.cmd_mix.cmd_para));
	if (ret != EOK) {
		lp_err("memset_s fail[%d]", ret);
		return -LOWPM_IPC_ECOMM;
	}

	ret = RPROC_SYNC_SEND(mbx, (rproc_msg_t *)&msg,
			      MAX_MAIL_SIZE, ack, ACK_BUFFER_LEN);
	if (ret != 0) {
		lp_err("send msg failed err %d", ret);
		return -LOWPM_IPC_ECOMM;
	}

	lp_info("get 32k switch[%u] from lpm3 success", ack[1]);

	return ack[1];
}

static ssize_t lowpm_set_32k_switch(u8 status)
{
	rproc_msg_t ack[ACK_BUFFER_LEN] = {0};
	rproc_id_t mbx = IPC_ACPU_LPM3_MBX_2;
	union ipc_data msg;
	int ret;

	if (status != SUSPEND_NOT_SWITCH_32K && status != SUSPEND_SWITCH_32K) {
		lp_err("invalid para %u", status);
		return -LOWPM_IPC_EINVAL;
	}

	msg.cmd_mix.cmd_src = OBJ_AP;
	msg.cmd_mix.cmd_obj = OBJ_AP;
	msg.cmd_mix.cmd  = CMD_SETTING;
	msg.cmd_mix.cmd_type = TYPE_SR;
	ret = memset_s(msg.cmd_mix.cmd_para, sizeof(msg.cmd_mix.cmd_para),
		       0, sizeof(msg.cmd_mix.cmd_para));
	if (ret != EOK) {
		lp_err("memset_s fail[%d]", ret);
		return -LOWPM_IPC_ECOMM;
	}
	msg.cmd_mix.cmd_para[0] = status;

	ret = RPROC_SYNC_SEND(mbx, (rproc_msg_t *)&msg,
			      MAX_MAIL_SIZE, ack, ACK_BUFFER_LEN);
	if (ret != 0) {
		lp_err("send msg failed err %d", ret);
		return -LOWPM_IPC_ECOMM;
	}

	ret = ack[1];
	if (ret != 0) {
		lp_err("lpm3 failed ack %d", ret);
		return -LOWPM_IPC_EFAILED;
	}

	g_lowpm_32k_switch = status;

	lp_info("set 32k switch[%u] to lpm3 success", status);

	return LOWPM_IPC_SUCCESS;
}

int lowpm_get_32k_switch(void)
{
	return g_lowpm_32k_switch;
}
EXPORT_SYMBOL_GPL(lowpm_get_32k_switch);

static ssize_t switch_32k_show(struct kobject *kobj,
			       struct kobj_attribute *attr, char *buf)
{
	return snprintf_s(buf, SWITCH_32K_BUFFER_LEN * sizeof(char),
			  (SWITCH_32K_BUFFER_LEN - 1) * sizeof(char), "%d\n",
			  get_32k_switch_from_lpmcu());
}

static ssize_t switch_32k_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t n)
{
	u8 status;
	int ret;

	if (kstrtou8(buf, 0, &status) != 0)
		return -EINVAL;

	ret = lowpm_set_32k_switch(status);
	if (ret != 0)
		return ret;

	return n;
}
static struct kobj_attribute switch_32k = __ATTR_RW(switch_32k);
static struct attribute *g_low_pm_attrs[] = {
	&switch_32k.attr,
	NULL
};
ATTRIBUTE_GROUPS(g_low_pm);
#endif

static __init int pm_lpctrl_init(void)
{
#ifdef CONFIG_SR_SWITCH_32K
	int ret;

	g_lowpm_32k_switch = SUSPEND_NOT_SWITCH_32K;
	ret = sysfs_create_groups(power_kobj, g_low_pm_groups);
	if (ret != 0) {
		lp_err("create file failed");
		return ret;
	}
#endif

	lp_notice("success");

	return 0;
}

late_initcall(pm_lpctrl_init);
