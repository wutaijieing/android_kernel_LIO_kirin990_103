/*
 * dubai_lp_stats.c
 *
 * monitor device status file
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
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
#include <linux/platform_drivers/dubai_lp_stats.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/suspend.h>
#include <securec.h>
#include <lpmcu_runtime.h>
#include <m3_rdr_ddr_map.h>
#include <ddr_vote_statistic_struct.h>
#include <platform_include/basicplatform/linux/ipc_msg.h>
#include <linux/platform_drivers/dpm_hwmon_user.h>

#define CLIENT_BUTT	10
#define LPM3_VOTE_TRANS_MAX	1024
static unsigned int g_lpm3_vote_trans_size;
struct dubai_transmit_t *g_lpm3_vote_trans_data;

#define ID_LEN	1 /* 1 word */
#define DDR_ACK_OK	0xff

#define DDR_VOTE_NUM	19
#define DDR_VOTE_DATA_MAX	19200
static unsigned int g_ddr_vote_num;
static unsigned int g_ddr_vote_data_size;
struct dubai_transmit_t *g_ddr_vote_data;

#ifdef CONFIG_LOWPM_DDR_STATE
#define DDR_STATE_NUM	13
#define DDR_STATE_DATA_MAX	19200
static unsigned int g_ddr_state_num;
static unsigned int g_ddr_state_data_size;
struct dubai_transmit_t *g_ddr_state_data;
#endif

static const char *g_lpm3_vote_client[CLIENT_BUTT] = {
	"acpu",
	"ccpu",
	"hifi",
	"iomcu",
	"lpmcu",
	"hisee",
	"codec",
	"asp",
	"ccpu_nr",
	"gpu",
};

static struct ddr_vote_ip g_ddr_vote_table[DDR_VOTE_NUM] = {
	{ LIT_CLUSTER_ID, "lit_cluster" },
	{ BIG_CLUSTER_ID, "big_cluster" },
	{ GPU_ID, "gpu" },
	{ IOMCU_ID, "iomcu" },
	{ MODEM_ID, "modem" },
	{ TEMPERTURE_ID, "temperture" },
	{ MID_CLUSTER_ID, "mid_cluster" },
	{ L3_CLUSTER_ID, "l3_cluster" },
	{ CPU_MONITOR_ID, "cpu_monitor"},
	{ AP_SR_ID, "ap_sr" },
	{ PERI_VOLT_ID, "peri_volt" },
	{ MODEM_5G_ID, "modem_5g" },
	{ FLOOR_VOTE_HW_AP_ID, "hw_ap" },
	{ FLOOR_VOTE_HW_HIFI_ID, "hw_hifi" },
	{ FLOOR_VOTE_HW_IOMCU_ID, "hw_iomcu" },
	{ FLOOR_VOTE_HW_LIT_LAT_ID, "hw_lit_lat" },
	{ FLOOR_VOTE_HW_BIG_LAT_ID, "hw_big_lat" },
	{ FLOOR_VOTE_HW_MID_LAT_ID, "hw_mid_lat" },
	{ FLOOR_VOTE_HW_LATSTAT_ID, "hw_latstat" },
};

#ifdef CONFIG_LOWPM_DDR_STATE
static struct ddr_state_ip g_ddr_lp_stats_table[DDR_STATE_NUM] = {
	{ DDR_PD_RANK0_CNT, "ddr_pd_rank0_cnt" },
	{ DDR_PD_RANK1_CNT, "ddr_pd_rank1_cnt" },
	{ DDR_ACT_PD_RANK0_CNT, "ddr_act_pd_rank0_cnt" },
	{ DDR_ACT_PD_RANK1_CNT, "ddr_act_pd_rank1_cnt" },
	{ DDR_ACT_NPD_RANK0_CNT, "ddr_act_npd_rank0_cnt" },
	{ DDR_ACT_NPD_RANK1_CNT, "ddr_act_npd_rank1_cnt" },
	{ DDR_ASREF_RANK0_CNT, "ddr_asref_rank0_cnt" },
	{ DDR_ASREF_RANK1_CNT, "ddr_asref_rank1_cnt" },
	{ DDR_PD_RANK0_NUM, "ddr_pd_rank0_num"},
	{ DDR_PD_RANK1_NUM, "ddr_pd_rank1_num" },
	{ DDR_ASREF_RANK0_NUM, "ddr_asref_rank0_num" },
	{ DDR_ASREF_RANK1_NUM, "ddr_asref_rank1_num" },
	{ DDR_LP_NUM, "ddr_lp_num" },
};
#endif

static int send_ipc_2_ddr(union ipc_data *msg, union ipc_data *ack)
{
	int ret;

	(msg->cmd_mix).cmd = CMD_INQUIRY;
	(msg->cmd_mix).cmd_obj = OBJ_DDR;
	(msg->cmd_mix).cmd_src = OBJ_AP;

	ret = RPROC_SYNC_SEND(IPC_ACPU_LPM3_MBX_2, (mbox_msg_t *)msg,
			      DDR_VOTE_MSG_LEN,
			      (mbox_msg_t *)ack, DDR_VOTE_ACK_LEN);
	if (ret != OK)
		pr_err("%s send data err:0x%x 0x%x\n",
		       __func__, (msg->data)[0], (msg->data)[1]);

	return ret;
}

static int update_ddr_vote_num(void)
{
	union ipc_data msg = {{0}};
	union ipc_data ack = {{0}};
	int ret;

	(msg.cmd_mix).cmd_type = TYPE_DDR_VOTE_NUMS;
	ret = send_ipc_2_ddr(&msg, &ack);
	if (ret != OK) {
		pr_err("%s fail ret = %d\n", __func__, ret);
		return -EINVAL;
	}

	g_ddr_vote_num = ack.data[1];

	return OK;
}

int ioctrl_get_ddr_vote_num(void __user *argp)
{
	if (argp == NULL || update_ddr_vote_num() != OK)
		return -EFAULT;
	if (copy_to_user(argp, &g_ddr_vote_num, sizeof(g_ddr_vote_num)) != 0) {
		pr_err("update_ddr_vote_num failed!\n");
		return -EFAULT;
	}

	return OK;
}

static int get_ddr_vote_id(unsigned int id_offset)
{
	u32 id, logic_id;

	logic_id = readl(M3_RDR_DDR_VOTE_TIME_ADDR + sizeof(u32) * id_offset);
	for (id = 0; id < ARRAY_SIZE(g_ddr_vote_table); id++) {
		if (g_ddr_vote_table[id].logic_id == (int)logic_id)
			break;
	}
	if (id == ARRAY_SIZE(g_ddr_vote_table))
		return -EINVAL;

	return (int)id;
}

/*
 * notify_ddr_save_data - send ipc to ddr save data
 * Return OK if save data successful or -EFAULT if fail
 */
static int notify_ddr_save_data(unsigned char cmd_type)
{
	union ipc_data msg = {{0}};
	union ipc_data ack = {{0}};
	unsigned int ddr_ack;
	int ret;

	(msg.cmd_mix).cmd_type = cmd_type;
	ret = send_ipc_2_ddr(&msg, &ack);
	if (ret != OK)
		return -EFAULT;
	ddr_ack = ack.data[1];
	if (ddr_ack != DDR_ACK_OK) {
		pr_err("%s wrong ack\n", __func__);
		return -EFAULT;
	}
	return OK;
}

static int get_ddr_vote_data(struct ddr_vote *ddr_vote_data)
{
	u32 i, freq, offset, id_offset, ddr_freq_num;
	int logic_id, ret;

	ret = notify_ddr_save_data(TYPE_DDR_VOTE_TIME);
	if (ret != OK)
		return -EFAULT;
	/*
	 * ddr vote data save in M3_RDR_DDR_VOTE_TIME_ADDR
	 * and the data as follow:
	 * id, freq0, freq1, freq2, freq3, freq4...
	 * id, freq0, freq1, freq2, freq3, freq4...
	 * etc...
	 */
	ddr_freq_num = MAX_DDR_FREQ_NUM;
	for (i = 0; i < g_ddr_vote_num; i++) {
		id_offset = i * (ID_LEN + ddr_freq_num);
		logic_id = get_ddr_vote_id(id_offset);
		if (logic_id < 0)
			continue;
		for (freq = 0; freq < ddr_freq_num; freq++) {
			offset = i * ddr_freq_num + freq;
			if (offset >= g_ddr_vote_data_size) {
				pr_err("%s offset = %u, overflow!\n", __func__, offset);
				return -EFAULT;
			}
			ddr_vote_data[offset].freq = (int)freq;
			ddr_vote_data[offset].duratiton =
				(int64_t)readl(M3_RDR_DDR_VOTE_TIME_ADDR + sizeof(u32) * (id_offset + ID_LEN + freq));
			ret = strncpy_s(ddr_vote_data[offset].name, VOTE_IP_NAME_SIZE,
					g_ddr_vote_table[logic_id].name, VOTE_IP_NAME_SIZE - 1);
			if (ret != EOK) {
				pr_err("copy g_ddr_vote_table %u error\n", i);
				return ret;
			}
		}
	}

	return OK;
}

int ioctrl_ddr_vote_data(void __user *argp)
{
	int ret;

	if (argp == NULL || g_ddr_vote_data == NULL || g_ddr_vote_num == 0 ||
	    g_ddr_vote_data_size > DDR_VOTE_DATA_MAX)
		return -EINVAL;

	g_ddr_vote_data->length = g_ddr_vote_data_size;
	ret = get_ddr_vote_data((struct ddr_vote *)(g_ddr_vote_data->data));
	if (ret != OK)
		return -EFAULT;
	if (copy_to_user(argp, g_ddr_vote_data, g_ddr_vote_data_size) != 0) {
		pr_err("%s failed!\n", __func__);
		return -EFAULT;
	}

	return OK;
}

int ioctrl_clr_ddr_vote_data(void __user *argp)
{
	union ipc_data msg = {{0}};
	union ipc_data ack = {{0}};
	int ret;
	unsigned int ddr_ack;

	(void)argp;
	(msg.cmd_mix).cmd_type = TYPE_DDR_VOTE_TIME_CLEAR;
	ret = send_ipc_2_ddr(&msg, &ack);
	if (ret != OK) {
		pr_err("%s failed!\n", __func__);
		return -EINVAL;
	}
	ddr_ack = ack.data[1];
	if (ddr_ack != DDR_ACK_OK) {
		pr_err("%s wrong ack\n", __func__);
		return -EFAULT;
	}

	return OK;
}

#ifdef CONFIG_LOWPM_DDR_STATE
static int update_ddr_state_num(void)
{
	union ipc_data msg = {{0}};
	union ipc_data ack = {{0}};
	int ret;

	(msg.cmd_mix).cmd_type = TYPE_DDR_STATE_NUM;
	ret = send_ipc_2_ddr(&msg, &ack);
	if (ret != OK) {
		pr_err("%s fail ret = %d\n", __func__, ret);
		return -EINVAL;
	}

	g_ddr_state_num = ack.data[1];

	return OK;
}

int ioctrl_get_ddr_state_num(void __user *argp)
{
	if (argp == NULL || update_ddr_state_num() != OK)
		return -EFAULT;
	if (copy_to_user(argp, &g_ddr_state_num, sizeof(g_ddr_state_num)) != 0) {
		pr_err("update_ddr_state_num failed!\n");
		return -EFAULT;
	}
	return OK;
}

static int get_ddr_state_id(unsigned int id_offset)
{
	u32 id, logic_id;

	logic_id = readl(M3_RDR_DDR_LOWPOWER_TIME_ADDR + sizeof(u32) * id_offset);
	for (id = 0; id < ARRAY_SIZE(g_ddr_lp_stats_table); id++) {
		if (g_ddr_lp_stats_table[id].logic_id == (int)logic_id)
			break;
	}
	if (id == ARRAY_SIZE(g_ddr_lp_stats_table))
		return -EINVAL;

	return (int)id;
}

static int get_ddr_state_data(struct ddr_state *ddr_state_data)
{
	u32 i, freq, offset, id_offset, ddr_freq_num;
	int logic_id, ret;

	ret = notify_ddr_save_data(TYPE_DDR_STATE_DATA);
	if (ret != OK)
		return -EFAULT;
	/*
	 * ddr vote data save in M3_RDR_DDR_LOWPOWER_TIME_ADDR
	 * and the data as follow:
	 * id, freq0, freq1, freq2, freq3, freq4...
	 * id, freq0, freq1, freq2, freq3, freq4...
	 * etc...
	 */
	ddr_freq_num = MAX_DDR_FREQ_NUM;
	for (i = 0; i < g_ddr_state_num; i++) {
		id_offset = i * (ID_LEN + ddr_freq_num);
		logic_id = get_ddr_state_id(id_offset);
		if (logic_id < 0)
			continue;
		for (freq = 0; freq < ddr_freq_num; freq++) {
			offset = i * ddr_freq_num + freq;
			if (offset >= g_ddr_state_data_size) {
				pr_err("%s offset = %u, overflow!\n", __func__, offset);
				return -EFAULT;
			}
			ddr_state_data[offset].freq = (int)freq;
			ddr_state_data[offset].value =
				(int64_t)readl(M3_RDR_DDR_LOWPOWER_TIME_ADDR + sizeof(u32) * (id_offset + ID_LEN + freq));
			ret = strncpy_s(ddr_state_data[offset].name, VOTE_IP_NAME_SIZE,
					g_ddr_lp_stats_table[logic_id].name, VOTE_IP_NAME_SIZE - 1);
			if (ret != EOK) {
				pr_err("copy ddr lowpower info error %u\n", i);
				return ret;
			}
		}
	}

	return OK;
}

int ioctrl_ddr_state_data(void __user *argp)
{
	int ret;

	if (argp == NULL || g_ddr_state_data == NULL || g_ddr_state_num == 0 ||
	    g_ddr_state_data_size > DDR_STATE_DATA_MAX)
		return -EINVAL;

	g_ddr_state_data->length = g_ddr_state_data_size;
	ret = get_ddr_state_data((struct ddr_state *)(g_ddr_state_data->data));
	if (ret != OK)
		return -EFAULT;
	if (copy_to_user(argp, g_ddr_state_data, g_ddr_state_data_size) != 0) {
		pr_err("%s failed!\n", __func__);
		return -EFAULT;
	}

	return OK;
}
#endif

static void update_lpm3_vote_info_suspend(void)
{
	int client_id;
	void __iomem *runtime_reserved_base = NULL;

	pr_info("%s +\n", __func__);
	if (M3_RDR_SYS_CONTEXT_BASE_ADDR) {
		runtime_reserved_base = M3_RDR_SYS_CONTEXT_RUNTIME_VAR_ADDR;
	} else {
		pr_err("%s: M3_RDR_SYS_CONTEXT_BASE_ADDR is NULL\n", __func__);
		return;
	}

	for (client_id = 0; client_id < CLIENT_BUTT; client_id++)
		writel(0, runtime_reserved_base + CLENT_VOTE_TIME_OFFSET(client_id));

	pr_info("%s -\n", __func__);
}

static void update_lpm3_vote_info_resume(void)
{
	int client_id;
	void __iomem *runtime_reserved_base = NULL;
	struct lpm3_vote *lpm3_vote_data = NULL;
	int ret;

	pr_info("%s +\n", __func__);
	if (g_lpm3_vote_trans_data == NULL)
		return;
	if (M3_RDR_SYS_CONTEXT_BASE_ADDR != 0) {
		runtime_reserved_base = M3_RDR_SYS_CONTEXT_RUNTIME_VAR_ADDR;
	} else{
		pr_err("%s: M3_RDR_SYS_CONTEXT_BASE_ADDR is NULL\n", __func__);
		return;
	}

	lpm3_vote_data = (struct lpm3_vote *)(g_lpm3_vote_trans_data->data);
	for (client_id = 0; client_id < CLIENT_BUTT; client_id++) {
		ret = strncpy_s(lpm3_vote_data[client_id].name, VOTE_IP_NAME_SIZE,
				g_lpm3_vote_client[client_id], VOTE_IP_NAME_SIZE - 1);
		if (ret != EOK) {
			pr_err("lpm3_vote_data %d error.\n", client_id);
			return;
		}
		lpm3_vote_data[client_id].duration +=
			(int64_t)readl(runtime_reserved_base + CLENT_VOTE_TIME_OFFSET(client_id));
		/* should clear here */
		writel(0, runtime_reserved_base + CLENT_VOTE_TIME_OFFSET(client_id));
	}
	pr_info("%s -\n", __func__);
}

int ioctrl_get_lpmcu_vote_num(void __user *argp)
{
	unsigned int lpmcu_vote_num;

	if (argp == NULL)
		return -EFAULT;

	lpmcu_vote_num = CLIENT_BUTT;
	if (copy_to_user(argp, &lpmcu_vote_num, sizeof(lpmcu_vote_num)) != 0) {
		pr_err("dpm_get_vote_number failed!\n");
		return -EFAULT;
	}

	return OK;
}

int ioctrl_lpmcu_vote_data(void __user *argp)
{
	if (argp == NULL || g_lpm3_vote_trans_data == NULL ||
	    g_lpm3_vote_trans_size > LPM3_VOTE_TRANS_MAX)
		return -EFAULT;

	g_lpm3_vote_trans_data->length = g_lpm3_vote_trans_size;
	if (copy_to_user(argp, g_lpm3_vote_trans_data, g_lpm3_vote_trans_size) != 0) {
		pr_err("%s failed!\n", __func__);
		return -EFAULT;
	}
	return OK;
}

static void free_vote_time_data(void)
{
	if (g_lpm3_vote_trans_data != NULL) {
		kfree(g_lpm3_vote_trans_data);
		g_lpm3_vote_trans_data = NULL;
	}
	if (g_ddr_vote_data != NULL) {
		kfree(g_ddr_vote_data);
		g_ddr_vote_data = NULL;
	}
#ifdef CONFIG_LOWPM_DDR_STATE
	if (g_ddr_state_data != NULL) {
		kfree(g_ddr_state_data);
		g_ddr_state_data = NULL;
	}
#endif
}

static int lpmcu_vote_callback(struct notifier_block *nb,
			unsigned long action, void *ptr)
{
	switch (action) {
	case PM_SUSPEND_PREPARE:
		update_lpm3_vote_info_suspend();
		break;
	case PM_POST_SUSPEND:
		update_lpm3_vote_info_resume();
		break;
	default:
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block g_lpmcu_vote_notif_block = {
	.notifier_call = lpmcu_vote_callback,
};

static int malloc_vote_time_data(void)
{
	int ret;

	g_lpm3_vote_trans_size = sizeof(struct dubai_transmit_t) +
				 sizeof(struct lpm3_vote) * CLIENT_BUTT;
	g_lpm3_vote_trans_data = (struct dubai_transmit_t *)
				 kzalloc(g_lpm3_vote_trans_size, GFP_KERNEL);
	if (g_lpm3_vote_trans_data == NULL) {
		ret = -ENOMEM;
		goto fail_malloc_mem;
	}

	g_ddr_vote_data_size = sizeof(struct dubai_transmit_t) +
			       sizeof(struct ddr_vote) * DDR_VOTE_NUM * MAX_DDR_FREQ_NUM;
	g_ddr_vote_data = (struct dubai_transmit_t *)kzalloc(g_ddr_vote_data_size, GFP_KERNEL);
	if (g_ddr_vote_data == NULL) {
		ret = -ENOMEM;
		goto fail_malloc_mem;
	}
#ifdef CONFIG_LOWPM_DDR_STATE
	g_ddr_state_data_size = sizeof(struct dubai_transmit_t) +
			       sizeof(struct ddr_state) * DDR_STATE_NUM * MAX_DDR_FREQ_NUM;
	g_ddr_state_data = (struct dubai_transmit_t *)kzalloc(g_ddr_state_data_size, GFP_KERNEL);
	if (g_ddr_state_data == NULL) {
		ret = -ENOMEM;
		goto fail_malloc_mem;
	}
#endif
	return 0;
fail_malloc_mem:
	free_vote_time_data();
	return ret;
}

static int __init lpmcu_vote_init(void)
{
	int ret;

	ret = malloc_vote_time_data();
	if (ret < 0)
		pr_err("%s lpmcu ddr vote_time data malloc failed!\n", __func__);

	register_pm_notifier(&g_lpmcu_vote_notif_block);

	return ret;
}

static void __exit lpmcu_vote_exit(void)
{
	free_vote_time_data();
	unregister_pm_notifier(&g_lpmcu_vote_notif_block);
}

module_init(lpmcu_vote_init);
module_exit(lpmcu_vote_exit);