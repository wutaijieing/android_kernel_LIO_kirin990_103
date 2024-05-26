/*
 * monitor_ap_m3_ipc.c
 *
 * monitor ap ipc
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

#include "monitor_ap_m3_ipc.h"

static unsigned int g_ipc_cnt;
u32 g_dubai_ddr_info[DDR_INFO_LEN][MAX_DDR_FREQ_NUM];
char g_ddr_info_name[DDR_INFO_LEN][STR_SIZE] = {
	{"DDR_FREQ_TIME"},
	{"DDR_FREQ_FLUX"},
};

static int g_ddr_freq_num;

static int send_ipc(unsigned char cmd_type)
{
	int i;
	int index;
	int max_freq_num;
	int ret = 0;
	union ipc_data msg = { {0} };
	union ipc_data ack = { {0} };

	if (!(cmd_type == TYPE_DDR_TIME || cmd_type == TYPE_DDR_FLUX))
		return -EINVAL;

	/* send ipc */
	(msg.cmd_mix).cmd_type = cmd_type;
	(msg.cmd_mix).cmd = CMD_INQUIRY;
	(msg.cmd_mix).cmd_obj = OBJ_DDR;
	(msg.cmd_mix).cmd_src = OBJ_AP;
	(msg.cmd_mix).cmd_para[0] = (unsigned char)g_ipc_cnt;

	ret = RPROC_SYNC_SEND(IPC_ACPU_LPM3_MBX_2, (mbox_msg_t *)&msg, MSG_LEN,
			      (mbox_msg_t *)&ack, ACK_LEN);
	if (ret != 0) {
		pr_err("%s send data err:[0x%x][0x%x]\n",
		       __func__, msg.data[0], msg.data[1]);
		return ret;
	}

	/* check ACK */
	if (ack.data[0] != msg.data[0]) {
		pr_err("%s recv data err:[0x%x][0x%x]\n",
		       __func__, msg.data[0], ack.data[0]);
		return -EINVAL;
	}

	index = (int)(g_ipc_cnt * (MAX_MAIL_SIZE - 1));
	max_freq_num = ((unsigned int)g_ddr_freq_num - ((MAX_MAIL_SIZE - 1) * g_ipc_cnt)) /
		       (MAX_MAIL_SIZE - 1) > 0 ?
		       MAX_MAIL_SIZE - 1 : g_ddr_freq_num % (MAX_MAIL_SIZE - 1);

	switch (cmd_type) {
	case TYPE_DDR_TIME:
		for (i = 0; i < max_freq_num &&
		     (i + index) < (int)g_ddr_freq_num; ++i)
			g_dubai_ddr_info[0][i + index] = ack.data[i + 1];
		break;
	case TYPE_DDR_FLUX:
		for (i = 0; i < max_freq_num &&
		     (i + index) < (int)g_ddr_freq_num; ++i)
			g_dubai_ddr_info[1][i + index] = ack.data[i + 1];
		break;
	default:
		pr_err("%s unknown cmd %d\n", __func__, cmd_type);
		return -EINVAL;
	}

	return OK;
}

int get_ddr_freq_num(void)
{
	int ret;
	union ipc_data msg = { {0} };
	union ipc_data ack = { {0} };

	msg.cmd_mix.cmd_type = TYPE_FREQ;
	msg.cmd_mix.cmd = CMD_INQUIRY;
	msg.cmd_mix.cmd_obj = OBJ_DDR;
	msg.cmd_mix.cmd_src = OBJ_AP;
	msg.cmd_mix.cmd_para[0] = 0;

	ret = RPROC_SYNC_SEND(IPC_ACPU_LPM3_MBX_2, (mbox_msg_t *)&msg, MSG_LEN,
			      (mbox_msg_t *)&ack, ACK_LEN);
	if (ret != OK) {
		pr_err("%s send data err:0x%x 0x%x, ret:%d\n",
		       __func__, msg.data[0], msg.data[1], ret);
		return -EINVAL;
	}

	if (ack.data[0] != msg.data[0]) {
		pr_err("%s recv data err:0x%x 0x%x\n", __func__, msg.data[0], ack.data[0]);
		return -EINVAL;
	}

	if (ack.data[1] > MAX_DDR_FREQ_NUM) {
		pr_err("%s ddr freq num over flow, max:%d, curr:%d\n", __func__,
		       MAX_DDR_FREQ_NUM, ack.data[1]);
		return -EINVAL;
	}

	return ack.data[1];
}

int monitor_ipc_send(void)
{
	int ret;
	unsigned int max_ipc_cnt;

	g_ipc_cnt = 0;
	g_ddr_freq_num = get_ddr_freq_num();
	if (g_ddr_freq_num == -EINVAL) {
		pr_err("%s freq num is invalid\n", __func__);
		return -EINVAL;
	}

	max_ipc_cnt = ((unsigned int)g_ddr_freq_num / (MAX_MAIL_SIZE - 1) + 1);
	while (g_ipc_cnt < max_ipc_cnt) {
		ret = send_ipc(TYPE_DDR_TIME);
		if (ret != 0)
			return ret;

		ret = send_ipc(TYPE_DDR_FLUX);
		if (ret != 0)
			return ret;
		++g_ipc_cnt;
	}
	g_ipc_cnt = 0;

	return OK;
}
