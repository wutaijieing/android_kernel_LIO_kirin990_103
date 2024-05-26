/*
 * drv_tele_mntn_node.c
 *
 * tele mntn module
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2015-2020. All rights reserved.
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

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/syslog.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/semaphore.h>
#include <linux/completion.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/debugfs.h>
#include <linux/of_platform.h>
#include <linux/rdr_lpm3.h>
#include <linux/jiffies.h>
#include <linux/wait.h>
#include <linux/version.h>
#include <securec.h>
#include <drv_tele_mntn_platform.h>
#include <drv_tele_mntn_gut.h>
/* ms */
#define TELE_MNTN_NODE_READ_WAIT_TIMEOUT	(msecs_to_jiffies(2000))
#define TELE_MNTN_NODE_READ_WAIT_FOREVER	MAX_SCHEDULE_TIMEOUT
#define TELE_MNTN_NODE_READ_WAIT_CONDITION_TRUE	0x11 /* wait */
#define TELE_MNTN_NODE_READ_WAIT_CONDITION_FALSE	0x10 /* no wait */
#define TELE_MNTN_WAKE_UP_ACORE_IPC_CMD \
	((8 << 24) | (17 << 16) | (3 << 8) | (7 << 0))
#define NO_SEQFILE	0
#define TELE_MNTN_SW_BUF_SIZE	32
#define TELE_MNTN_SW_BITS_LIMIT	64
#define PD_STATE_SUBSYS_VOTE_MASK	0xFF
#define PD_STATE_SUBSYS_VOTE_OFFSET	8
#define PD_STATE_RESET_COUNT_MASK	0xFF
#define PD_STATE_SUBSYS_AP_OFFSET	8
#define PD_STATE_SUBSYS_M7_OFFSET	9
#define PD_STATE_SUBSYS_HIFI_OFFSET	9
#define TELE_MNTN_PU_STATE_BIT_MASK	1
#define PD_STATE_CURRENT_STATE_OFFSET	16
#define PU_WAKE_MBX_MODE_MASK	0x40

#define sprint_lpmcu_log(seq_file, fmt, args ...) \
	do { \
		if (seq_file == NO_SEQFILE) \
			pr_info(fmt, ##args); \
		else \
			seq_printf(seq_file, fmt, ##args); \
	} while (0)
static struct notifier_block g_tele_mntn_ipc_msg_nb;
DECLARE_WAIT_QUEUE_HEAD(node_read_wait);
static signed long g_node_read_wait_timeout = TELE_MNTN_NODE_READ_WAIT_FOREVER;
static int g_node_read_wait_condition = TELE_MNTN_NODE_READ_WAIT_CONDITION_TRUE;

/*
 * get the access of modify the rp & wp,
 * nobody can't modify them (means access to the buf) without rw_lock
 */
static struct semaphore g_rw_sem;

static int tele_mntn_node_open(struct inode *inode,
			       struct file *file)
{
	(void)inode;
	(void)file;
	return 0;
}

static int tele_mntn_node_release(struct inode *inode,
				  struct file *file)
{
	(void)inode;
	(void)file;
	return 0;
}

static ssize_t tele_mntn_node_read(struct file *file, char __user *buf,
				   size_t count, loff_t *ppos)
{
	struct tele_mntn_queue log_queue = {0};
	int tm_ret;
	unsigned int size_read;
	bool is_wait = false;
	/* 8 bytes align */
	unsigned int need_read =
		TELE_MNTN_SHORTCUT_ALIGN(count, TELE_MNTN_ALIGN_SIZE);

	(void)file;
	(void)ppos;
	if (need_read == 0 || buf == NULL)
		return 0;
	/* now count > 0 */
	down(&g_rw_sem);
	is_wait =
		g_node_read_wait_condition == TELE_MNTN_NODE_READ_WAIT_CONDITION_FALSE;
	tm_ret = wait_event_interruptible_timeout(node_read_wait,
					       is_wait,
					       g_node_read_wait_timeout);
	if (tm_ret < 0)
		goto copy_error;

	g_node_read_wait_condition = TELE_MNTN_NODE_READ_WAIT_CONDITION_TRUE;

	tm_ret = tele_mntn_get_head(&log_queue);
	if (tm_ret != 0)
		goto copy_error;

	size_read = (log_queue.front >= log_queue.rear) ?
		    (log_queue.front - log_queue.rear) :
		    (log_queue.base + log_queue.length - log_queue.rear);
	size_read = size_read <= need_read ? size_read : need_read;

	if (size_read != 0) {
		tm_ret = (int)copy_to_user(buf, log_queue.rear, size_read);
		if (tm_ret != 0)
			goto copy_error;
		log_queue.rear += size_read;
		if (log_queue.rear >= log_queue.base + log_queue.length)
			log_queue.rear -= log_queue.length;
		tele_mntn_set_head(&log_queue);
	}
	up(&g_rw_sem);
	return size_read;

copy_error:
	up(&g_rw_sem);
	return -EINTR;
}

static const struct file_operations g_proc_tele_mntn_node_operations = {
	.read = tele_mntn_node_read,
	.open = tele_mntn_node_open,
	.release = tele_mntn_node_release,
};

static ssize_t tele_mntn_sw_show(struct file *filp, char __user *buffer,
				 size_t count, loff_t *ppos)
{
	int len, ret;
	char buf[TELE_MNTN_SW_BUF_SIZE];
	unsigned long long sw_value;
	unsigned int tmp_low, tmp_high;

	(void)filp;
	ret = memset_s(buf, sizeof(buf), 0, sizeof(buf));
	if (ret != EOK) {
		pr_err("%s:%d, memset_s err:%d\n",
		       __func__, __LINE__, ret);
		return -EINVAL;
	}
	sw_value = tele_mntn_func_sw_get();
	/* sw_low bit0~31, bit mask : 0xFFFFFFFF */
	tmp_low = (unsigned int)(sw_value & UINT_MAX);
	/* sw_high bit32~63, bit mask : 0xFFFFFFFF */
	tmp_high = (unsigned int)((sw_value >> 32) & UINT_MAX);
	len = snprintf_s(buf, sizeof(buf), sizeof(buf) - 1, "0x%08x%08x\n",
			 tmp_high, tmp_low);
	if (len < 0) {
		pr_err("%s: snprintf_s err\n", __func__);
		return -EINVAL;
	}
	if ((unsigned int)len < count)
		return simple_read_from_buffer(buffer, count, ppos,
					       (void *)buf, len);

	return 0;
}

static ssize_t tele_mntn_sw_on_store(struct file *filp,
				     const char __user *buffer,
				     size_t count,
				     loff_t *ppos)
{
	char buf[TELE_MNTN_SW_BUF_SIZE] = {0};
	unsigned int sw_bit = 0;

	(void)filp;
	(void)ppos;
	if (count >= sizeof(buf)) {
		pr_err("%s: input count larger than buf size\n", __func__);
		return -ENOMEM;
	}

	if (buffer == NULL) {
		pr_err("%s: user buffer is NULL!\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user(buf, buffer, count) != 0) {
		pr_err("%s: copy_from_user from fm is err\n", __func__);
		return -EFAULT;
	}

	if (sscanf_s(buf, "%u", &sw_bit) != 1)
		return -EDOM;

	if (sw_bit < TELE_MNTN_SW_BITS_LIMIT)
		tele_mntn_func_sw_bit_set(sw_bit);
	else if (sw_bit == TELE_MNTN_SW_BITS_LIMIT)
		tele_mntn_func_sw_set(~(0ULL));

	if (sw_bit == TELE_MNTN_NVME_LOGCAT) {
		g_node_read_wait_condition =
			TELE_MNTN_NODE_READ_WAIT_CONDITION_FALSE;
		g_node_read_wait_timeout = (long)TELE_MNTN_NODE_READ_WAIT_TIMEOUT;
		wake_up_interruptible(&node_read_wait);
	}

	return count;
}

static ssize_t tele_mntn_sw_off_store(struct file *filp,
				      const char __user *buffer,
				      size_t count,
				      loff_t *ppos)
{
	char buf[TELE_MNTN_SW_BUF_SIZE] = {0};
	unsigned int sw_bit = 0;

	(void)filp;
	(void)ppos;
	if (count >= sizeof(buf)) {
		pr_err("%s: input count larger than buf size\n", __func__);
		return -ENOMEM;
	}

	if (buffer == NULL) {
		pr_err("%s: user buffer is NULL!\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user(buf, buffer, count) != 0) {
		pr_err("%s: copy_from_user from fm is err\n", __func__);
		return -EFAULT;
	}

	if (sscanf_s(buf, "%u", &sw_bit) != 1)
		return -EDOM;
	if (sw_bit < TELE_MNTN_SW_BITS_LIMIT)
		tele_mntn_func_sw_bit_clr(sw_bit);
	else if (sw_bit == TELE_MNTN_SW_BITS_LIMIT)
		tele_mntn_func_sw_set(0);

	if (sw_bit == TELE_MNTN_NVME_LOGCAT) {
		g_node_read_wait_condition =
			TELE_MNTN_NODE_READ_WAIT_CONDITION_TRUE;
		g_node_read_wait_timeout = TELE_MNTN_NODE_READ_WAIT_FOREVER;
	}

	return count;
}

static void mntn_hifi_pu_dump_show(struct seq_file *s)
{
	u32 vote_tmp, state_tmp, state;
	u32 ap_tmp, hifi_tmp, reset_count;
	struct lpmcu_tele_mntn_stru_s *lp_mntn_ptr = lpmcu_tele_mntn_get();

	if (lp_mntn_ptr == NULL) {
		sprint_lpmcu_log(s, "ERR: M3_RDR_SYS_CONTEXT_BASE_ADDR is null\n");
		return;
	}
	vote_tmp = lp_mntn_ptr->hifi.pu.state >> PD_STATE_SUBSYS_VOTE_OFFSET;
	ap_tmp = lp_mntn_ptr->hifi.pu.state >> PD_STATE_SUBSYS_AP_OFFSET;
	hifi_tmp = lp_mntn_ptr->hifi.pu.state >> PD_STATE_SUBSYS_HIFI_OFFSET;
	reset_count = lp_mntn_ptr->hifi.pu.state & PD_STATE_RESET_COUNT_MASK;
	state_tmp =
		lp_mntn_ptr->hifi.pu.state >> PD_STATE_CURRENT_STATE_OFFSET;
	state = state_tmp & TELE_MNTN_PU_STATE_BIT_MASK;

	sprint_lpmcu_log(s, "asp state: 0x%x\n\t ",
			 lp_mntn_ptr->hifi.pu.state);
	sprint_lpmcu_log(s, "subsys vote 0x%x",
			 vote_tmp & PD_STATE_SUBSYS_VOTE_MASK);
	sprint_lpmcu_log(s, "(ap:%u ",
			 ap_tmp & TELE_MNTN_PU_STATE_BIT_MASK);
	sprint_lpmcu_log(s, "hifi:%u)\n\t ",
			 hifi_tmp & TELE_MNTN_PU_STATE_BIT_MASK);
	sprint_lpmcu_log(s, "hifi reset count %u\n\t ", reset_count);
	sprint_lpmcu_log(s, "hifi current state %s\n",
			 state != 0 ? "on" : "off");
}

static void mntn_hifi_pd_dump_show(struct seq_file *s)
{
	u32 vote_tmp, state_tmp, state;
	u32 ap_tmp, hifi_tmp, reset_count;
	struct lpmcu_tele_mntn_stru_s *lp_mntn_ptr = lpmcu_tele_mntn_get();

	if (lp_mntn_ptr == NULL) {
		sprint_lpmcu_log(s, "ERR: M3_RDR_SYS_CONTEXT_BASE_ADDR is null\n");
		return;
	}
	vote_tmp = lp_mntn_ptr->hifi.pd.state >> PD_STATE_SUBSYS_VOTE_OFFSET;
	ap_tmp = lp_mntn_ptr->hifi.pd.state >> PD_STATE_SUBSYS_AP_OFFSET;
	hifi_tmp = lp_mntn_ptr->hifi.pd.state >> PD_STATE_SUBSYS_HIFI_OFFSET;
	reset_count = lp_mntn_ptr->hifi.pd.state & PD_STATE_RESET_COUNT_MASK;
	state_tmp =
		lp_mntn_ptr->hifi.pd.state >> PD_STATE_CURRENT_STATE_OFFSET;
	state = state_tmp & TELE_MNTN_PU_STATE_BIT_MASK;

	sprint_lpmcu_log(s, "asp state: 0x%x\n\t ",
			 lp_mntn_ptr->hifi.pd.state);
	sprint_lpmcu_log(s, "subsys vote 0x%x",
			 vote_tmp & PD_STATE_SUBSYS_VOTE_MASK);
	sprint_lpmcu_log(s, "(ap:%u ",
			 ap_tmp & TELE_MNTN_PU_STATE_BIT_MASK);
	sprint_lpmcu_log(s, "hifi:%u)\n\t ",
			 hifi_tmp & TELE_MNTN_PU_STATE_BIT_MASK);
	sprint_lpmcu_log(s, "hifi reset count %u\n\t ", reset_count);
	sprint_lpmcu_log(s, "hifi current state %s\n",
			 state != 0 ? "on" : "off");
}

static void mntn_hifi_dump_show(struct seq_file *s)
{
	unsigned char wake_mode_tmp;
	struct lpmcu_tele_mntn_stru_s *lp_mntn_ptr = lpmcu_tele_mntn_get();

	if (lp_mntn_ptr == NULL) {
		sprint_lpmcu_log(s, "ERR: M3_RDR_SYS_CONTEXT_BASE_ADDR is null\n");
		return;
	}
	if (lp_mntn_ptr->hifi.pu.up_cnt > lp_mntn_ptr->hifi.pd.dn_cnt)
		mntn_hifi_pu_dump_show(s);
	else
		mntn_hifi_pd_dump_show(s);
	wake_mode_tmp =
		lp_mntn_ptr->hifi.pu.wake_mbx_mode & PU_WAKE_MBX_MODE_MASK;
	sprint_lpmcu_log(s, "\t powered on count %u\n\t ",
			 lp_mntn_ptr->hifi.pu.up_cnt);
	sprint_lpmcu_log(s, "powered off count %u\n\t ",
			 lp_mntn_ptr->hifi.pd.dn_cnt);
	sprint_lpmcu_log(s, "waked irq %u\n\t\t ",
			 lp_mntn_ptr->hifi.pu.wake_irq);
	sprint_lpmcu_log(s, "waked mailbox %u\n\t\t ",
			 lp_mntn_ptr->hifi.pu.wake_mbx_id);
	sprint_lpmcu_log(s, "waked type %s\n\t\t ",
			 wake_mode_tmp != '\0' ? "receive irq" : "ack irq");
	sprint_lpmcu_log(s, "mailbox data0 0x%x\n",
			 lp_mntn_ptr->hifi.pu.wake_mbx_data0);
}

static void mntn_acore_dump_show(struct seq_file *s)
{
	struct lpmcu_tele_mntn_stru_s *lp_mntn_ptr = lpmcu_tele_mntn_get();

	if (lp_mntn_ptr == NULL) {
		sprint_lpmcu_log(s, "ERR: M3_RDR_SYS_CONTEXT_BASE_ADDR is null\n");
		return;
	}
	sprint_lpmcu_log(s, "acore:\n\t sleep_count %u\n\t ",
			 lp_mntn_ptr->acore.pd_stat.sleep_count);
	sprint_lpmcu_log(s, "wake_count %u\n\t ",
			 lp_mntn_ptr->acore.pu_stat.wake_count);
	sprint_lpmcu_log(s, "wake_src 0x%x\n\t ",
			 lp_mntn_ptr->acore.pu_stat.wake_src);
	sprint_lpmcu_log(s, "pd_slice_time 0x%x\n\t ",
			 lp_mntn_ptr->acore.pd_slice_time);
	sprint_lpmcu_log(s, "pu_slice_time 0x%x\n",
			 lp_mntn_ptr->acore.pu_slice_time);
}

static void mntn_ccore_dump_show(struct seq_file *s)
{
	struct lpmcu_tele_mntn_stru_s *lp_mntn_ptr = lpmcu_tele_mntn_get();

	if (lp_mntn_ptr == NULL) {
		sprint_lpmcu_log(s, "ERR: M3_RDR_SYS_CONTEXT_BASE_ADDR is null\n");
		return;
	}
	sprint_lpmcu_log(s, "ccore:\n\t sleep_count %u\n\t ",
			 lp_mntn_ptr->ccore.pd_stat.sleep_count);
	sprint_lpmcu_log(s, "wake_count %u\n\t ",
			 lp_mntn_ptr->ccore.pu_stat.wake_count);
	sprint_lpmcu_log(s, "wake_src 0x%x\n\t ",
			 lp_mntn_ptr->ccore.pu_stat.wake_src);
	sprint_lpmcu_log(s, "pd_slice_time 0x%x\n\t ",
			 lp_mntn_ptr->ccore.pd_slice_time);
	sprint_lpmcu_log(s, "pu_slice_time 0x%x\n",
			 lp_mntn_ptr->ccore.pu_slice_time);
}

static void mntn_mcu_dump_show(struct seq_file *s)
{
	struct lpmcu_tele_mntn_stru_s *lp_mntn_ptr = lpmcu_tele_mntn_get();

	if (lp_mntn_ptr == NULL) {
		sprint_lpmcu_log(s, "ERR: M3_RDR_SYS_CONTEXT_BASE_ADDR is null\n");
		return;
	}
	sprint_lpmcu_log(s, "mcu_deepSleep:\n\t sleep_count %u\n\t ",
			 lp_mntn_ptr->mcu.info.stat.sleep_count);
	sprint_lpmcu_log(s, "wake_count %u\n\t ",
			 lp_mntn_ptr->mcu.info.stat.wake_count);
	sprint_lpmcu_log(s, "sleep_slice_time 0x%x\n\t ",
			 lp_mntn_ptr->mcu.info.stat.sleep_slice_time);
	sprint_lpmcu_log(s, "wake_slice_time 0x%x\n\t ",
			 lp_mntn_ptr->mcu.info.stat.wake_slice_time);
	sprint_lpmcu_log(s, "wake_src0 0x%x\n\t ",
			 lp_mntn_ptr->mcu.info.stat.wake_src0);
	sprint_lpmcu_log(s, "wake_src1 0x%x\n\t ",
			 lp_mntn_ptr->mcu.info.stat.wake_src1);
	sprint_lpmcu_log(s, "can_sleep_ret 0x%x\n",
			 lp_mntn_ptr->mcu.info.not_sleep_reason.can_sleep_ret);
}

static void mntn_dfs_ddr_dump_show(struct seq_file *s)
{
	struct lpmcu_tele_mntn_stru_s *lp_mntn_ptr = lpmcu_tele_mntn_get();

	if (lp_mntn_ptr == NULL) {
		sprint_lpmcu_log(s, "ERR: M3_RDR_SYS_CONTEXT_BASE_ADDR is null\n");
		return;
	}
	sprint_lpmcu_log(s, "ddr_target:\n\t old_freq %d\n\t ",
			 lp_mntn_ptr->dfs_ddr.target.info.old_freq);
	sprint_lpmcu_log(s, "new_freq %d\n\t ",
			 lp_mntn_ptr->dfs_ddr.target.info.new_freq);
	sprint_lpmcu_log(s, "tag_freq %d\n\t ",
			 lp_mntn_ptr->dfs_ddr.target.info.tag_freq);
	sprint_lpmcu_log(s, "target_slice_time 0x%x\n",
			 lp_mntn_ptr->dfs_ddr.target.target_slice_time);
}

static int tele_mntn_dump_show(struct seq_file *s, void *unused)
{
	int idex;
	struct lpmcu_tele_mntn_stru_s *lp_mntn_ptr = lpmcu_tele_mntn_get();

	(void)unused;
	if (lp_mntn_ptr == NULL) {
		sprint_lpmcu_log(s, "ERR: M3_RDR_SYS_CONTEXT_BASE_ADDR is null\n");
		return 0;
	}

#if defined(CONFIG_LPMCU_BB)
	/* send interrupt to lpm3, update rdr data */
	(void)rdr_lpm3_stat_dump();
#endif
	mntn_acore_dump_show(s);
	mntn_ccore_dump_show(s);
	mntn_mcu_dump_show(s);
	mntn_dfs_ddr_dump_show(s);

	for (idex = 0; idex < PERI_SLEEP_CUR_STATE_NUM; idex++) {
		sprint_lpmcu_log(s, "lpmcu_vote[%d]:\n\t ", idex);
		sprint_lpmcu_log(s, "stat %u\t ",
				 lp_mntn_ptr->vote[idex].vote_stat.stat);
		sprint_lpmcu_log(s, "client_id %u\t ",
				 lp_mntn_ptr->vote[idex].vote_stat.client_id);
		sprint_lpmcu_log(s, "state_id %u\t ",
				 lp_mntn_ptr->vote[idex].vote_stat.state_id);
		sprint_lpmcu_log(s, "vote_map %u\t ",
				 lp_mntn_ptr->vote[idex].vote_stat.vote_map);
		sprint_lpmcu_log(s, "slice_time 0x%x\n",
				 lp_mntn_ptr->vote[idex].slice_time);
	}
	mntn_hifi_dump_show(s);
	return 0;
}

static int tele_mntn_dump_open(struct inode *inode, struct file *file)
{
	return single_open(file, tele_mntn_dump_show, &inode->i_private);
}

static const struct file_operations g_tele_mntn_sw_on_fops = {
	.read = tele_mntn_sw_show,
	.write = tele_mntn_sw_on_store,
};

static const struct file_operations g_tele_mntn_sw_off_fops = {
	.read = tele_mntn_sw_show,
	.write = tele_mntn_sw_off_store,
};

static const struct file_operations g_tele_mntn_dump_fops = {
	.owner = THIS_MODULE,
	.open = tele_mntn_dump_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int tele_mntn_node_debugfs_create(void)
{
#ifdef CONFIG_DFX_DEBUG_FS
	struct dentry *debugfs_root = debugfs_create_dir("telemntnsw", NULL);

	if (debugfs_root == NULL || IS_ERR(debugfs_root))
		return -ENOENT;
	/* mode : S_IRUSR | S_IRGRP | S_IWUSR */
	(void)debugfs_create_file("on", 0640, debugfs_root, NULL,
				  &g_tele_mntn_sw_on_fops);
	/* mode : S_IRUSR | S_IRGRP | S_IWUSR */
	(void)debugfs_create_file("off", 0640, debugfs_root, NULL,
				  &g_tele_mntn_sw_off_fops);
	/* mode : S_IRUSR | S_IRGRP | S_IWUSR */
	(void)debugfs_create_file("dump", 0640, debugfs_root, NULL,
				  &g_tele_mntn_dump_fops);
#endif
	return 0;
}

static int tele_mntn_wake_up_acore_irq(struct notifier_block *nb,
				       unsigned long len, void *msg)
{
	unsigned int *_msg = (unsigned int *)msg;

	(void)nb;
	(void)len;
	if (_msg[0] == TELE_MNTN_WAKE_UP_ACORE_IPC_CMD) {
		g_node_read_wait_condition =
			TELE_MNTN_NODE_READ_WAIT_CONDITION_FALSE;
		pr_err("%s:  receive irq\n", __func__);
	}

	return 0;
}

static int __init proc_tele_mntn_node_init(void)
{
	int err;
	struct proc_dir_entry *entry_tele = NULL;

	tele_mntn_init();
	if (tele_mntn_is_func_on(TELE_MNTN_NVME_LOGCAT))
		g_node_read_wait_timeout = (long)TELE_MNTN_NODE_READ_WAIT_TIMEOUT;
	else
		g_node_read_wait_timeout = TELE_MNTN_NODE_READ_WAIT_FOREVER;
	sema_init(&g_rw_sem, 1);
	/* mode : S_IRUSR | S_IRGRP */
	entry_tele = proc_create("telemntn", 0440, NULL,
				 &g_proc_tele_mntn_node_operations);
	if (entry_tele == NULL) {
		err = -EINVAL;
		pr_err("proc_tele create proc failed!\n");
		return err;
	}

	tele_mntn_node_debugfs_create();
	g_tele_mntn_ipc_msg_nb.next = NULL;
	g_tele_mntn_ipc_msg_nb.notifier_call = tele_mntn_wake_up_acore_irq;
	err = RPROC_MONITOR_REGISTER(IPC_LPM3_ACPU_MBX_1,
				     &g_tele_mntn_ipc_msg_nb);
	if (err != 0) {
		pr_info("proc_tele register err!\n");
		return err;
	}
	pr_info("%s success\n", __func__);
	return 0;
}

static void __exit proc_tele_mntn_node_exit(void)
{
	tele_mntn_exit();
}
module_init(proc_tele_mntn_node_init);
module_exit(proc_tele_mntn_node_exit);
