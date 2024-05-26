/*
 * onsys_avs.c
 *
 * onsys avs module
 *
 * Copyright (c) 2021-2022 Huawei Technologies Co., Ltd.
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
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_drivers/platform_qos.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>
#include <platform_include/basicplatform/linux/ipc_msg.h>
#include <linux/slab.h>

static int set_onsys_avs_ctrl_cmd(unsigned int cmd)
{
	unsigned int *msg = NULL;
	int rproc_id = IPC_ACPU_LPM3_MBX_1;
	int ret;

	msg = kzalloc(sizeof(u32) * MAX_MAIL_SIZE, GFP_KERNEL);
	if (msg == NULL) {
		pr_err("%s: cannot allocate msg space\n", __func__);
		return -ENOMEM;
	}
	msg[0] = IPC_CMD(OBJ_AP, OBJ_LITTLE_CLUSTER, CMD_SETTING, TYPE_PWC);
	msg[1] = cmd;
	ret = RPROC_ASYNC_SEND((rproc_id_t)rproc_id, (mbox_msg_t *)msg,
			       MAX_MAIL_SIZE);
	if (ret != 0)
		pr_err("%s, line %d, send error\n", __func__, __LINE__);

	kfree(msg);
	msg = NULL;

	return ret;
}

static int onsys_avs_ctrl_cmd_notify(struct notifier_block *nb,
				   unsigned long val, void *v)
{
	int ret;

	ret = set_onsys_avs_ctrl_cmd((unsigned int)val);
	if (ret != 0)
		return NOTIFY_BAD;

	return NOTIFY_OK;
}

static struct notifier_block onsys_avs_ctrl_notifier = {
	.notifier_call = onsys_avs_ctrl_cmd_notify,
};

static __init int onsys_avs_ctrl_init(void)
{
	int ret;

	ret = platform_qos_add_notifier(PLATFORM_QOS_ONSYS_AVS_LEVEL,
					&onsys_avs_ctrl_notifier);
	if (ret != 0)
		pr_err("onsys_avs_ctrl init fail%d\n", ret);

	return ret;
}
module_init(onsys_avs_ctrl_init);
