/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2019. All rights reserved.
 * Description:  mas block function test
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#define pr_fmt(fmt) "[BLK-IO]" fmt

#include <linux/bio.h>
#include <linux/blk-mq.h>
#include <linux/blkdev.h>
#include <linux/delay.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/suspend.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

#include "blk.h"

#define TEST_BUF_LEN (1024UL * 1024UL)

#define MAS_BLK_FT_PWR_OFF 0
#define MAS_BLK_FT_PANIC 1

#define MAS_BLK_FT_SUSPEND_CASE_1 1
#define MAS_BLK_FT_SUSPEND_CASE_2 2

#define MAS_BLK_FT_BG_IO_CASE_1 1
#define MAS_BLK_FT_BG_IO_CASE_2 2

enum mas_blk_apd_mode {
	MAS_BLK_APD_INTR_EN = 1,
	MAS_BLK_APD_INTR_DISABLE
};

static spinlock_t blk_rq_sim_mode_lock;
static spinlock_t blk_rq_requeue_lock;
static wait_queue_head_t mq_req_sim_tst_wq;
static enum blk_ft_rq_sim_mode blk_rq_sim_mode = BLK_FT_RQ_SIM_NONE;
static int blk_rq_sim_ongoing;
static struct delayed_work mq_requeue_work;
static struct request *mq_requeue_rq;

static void mas_blk_ft_mq_requeue_work(struct work_struct *work)
{
	int ret = 1;
	unsigned long flags;
	struct request *req = NULL;
	struct blk_mq_hw_ctx *hctx = NULL;
	struct blk_mq_queue_data bd;

	spin_lock_irqsave(&blk_rq_requeue_lock, flags);
	req = mq_requeue_rq;
	mq_requeue_rq = NULL;
	spin_unlock_irqrestore(&blk_rq_requeue_lock, flags);
	if (!req) {
		pr_err("%s req is NULL\n", __func__);
		return;
	}

	pr_err("%s dispatch rq: 0x%pK in normal routine!\n", __func__, req);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	hctx = blk_mq_map_queue(req->q, req->cmd_flags, req->mq_ctx);
#else
	hctx = blk_mq_map_queue(req->q, (int)req->mq_ctx->cpu);
#endif
	bd.rq = req;
	while (ret)
		ret = req->q->mq_ops->queue_rq(hctx, &bd);
}

static void mas_blk_ft_rq_sim_start(struct request *rq)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	set_bit(REQ_ATOM_STARTED, &rq->__deadline);
	clear_bit(REQ_ATOM_COMPLETE, &rq->__deadline);
#else
	set_bit(REQ_ATOM_STARTED, &rq->atomic_flags);
	clear_bit(REQ_ATOM_COMPLETE, &rq->atomic_flags);
#endif
	blk_add_timer(rq);
	rq->timeout = 0;
}

bool mas_blk_ft_mq_queue_rq_redirection(
	struct request *rq, struct request_queue *q)
{
	unsigned long flags;
	enum blk_ft_rq_sim_mode *sim_mode = &rq->mas_req.simulate_mode;

	/* Send request in the normal routine */
	if (likely(!blk_rq_sim_mode))
		return false;

	spin_lock_irqsave(&blk_rq_sim_mode_lock, flags);
	*sim_mode = blk_rq_sim_mode;
	blk_rq_sim_mode = BLK_FT_RQ_SIM_NONE;
	spin_unlock_irqrestore(&blk_rq_sim_mode_lock, flags);

	pr_err("%s simulate_mode: %d rq: 0x%pK\n", __func__, *sim_mode, rq);
	switch (*sim_mode) {
	case BLK_FT_RQ_SIM_EXPECT_REQUEUE:
	case BLK_FT_RQ_SIM_RESET_TIMER:
		pr_err("%s break dispatch on purpose!\n", __func__);
		rq->timeout = 1;
		break;
	case BLK_FT_RQ_SIM_TIMEOUT_HANDLED:
		pr_err("%s continue dispatching ...\n", __func__);
		return false;
	default:
		break;
	}
	mas_blk_ft_rq_sim_start(rq);

	return true;
}

bool mas_blk_ft_mq_complete_rq_redirection(struct request *rq, bool succ_done)
{
	unsigned long flags;
	enum blk_ft_rq_sim_mode *sim_mode = &rq->mas_req.simulate_mode;

	switch (*sim_mode) {
	case BLK_FT_RQ_SIM_TIMEOUT_HANDLED:
		if (!succ_done)
			break;

		pr_err("%s temporary skip complete rq: 0x%pK\n", __func__, rq);
		mas_blk_ft_rq_sim_start(rq);
		return true;
	default:
		break;
	}

	if (likely(!(*sim_mode)))
		return false;

	pr_err("%s mode: %d rq: 0x%pK\n", __func__, *sim_mode, rq);
	spin_lock_irqsave(&blk_rq_sim_mode_lock, flags);
	*sim_mode = 0;
	blk_rq_sim_ongoing = 0;
	spin_unlock_irqrestore(&blk_rq_sim_mode_lock, flags);
	wake_up_all(&mq_req_sim_tst_wq);

	return false;
}

bool mas_blk_ft_mq_rq_timeout_redirection(
	struct request *rq, enum blk_eh_timer_return *ret)
{
	unsigned long flags;

	switch (rq->mas_req.simulate_mode) {
	/* Do nothing, expect block layer requeue */
	case BLK_FT_RQ_SIM_EXPECT_REQUEUE:
		pr_err("%s EXPECT_REQUEUE rq: 0x%pK\n", __func__, rq);
		*ret = BLK_EH_REQUEUE;
		return true;
	/* Trigger a worker to dispatch the origin req */
	case BLK_FT_RQ_SIM_RESET_TIMER:
		pr_err("%s RESET_TIMER rq: 0x%pK\n", __func__, rq);
		spin_lock_irqsave(&blk_rq_requeue_lock, flags);
		mq_requeue_rq = rq;
		kblockd_schedule_delayed_work(&mq_requeue_work, 0UL);
		spin_unlock_irqrestore(&blk_rq_requeue_lock, flags);
		*ret = BLK_EH_RESET_TIMER;
		return true;
	case BLK_FT_RQ_SIM_TIMEOUT_HANDLED:
		pr_err("%s TIMEOUT_HANDLED rq: 0x%pK\n", __func__, rq);
		*ret = BLK_EH_HANDLED;
		return true;
	default:
		break;
	}

	return false;
}

ssize_t mas_queue_timeout_tst_enable_store(
	struct request_queue *q, const char *page, size_t count)
{
	ssize_t ret;
	unsigned long val;

	ret = queue_var_store(&val, page, count);
	if (ret < 0) {
		pr_err("%s Invalid value\n", __func__);
		return (ssize_t)count;
	}

	spin_lock_init(&blk_rq_sim_mode_lock);
	spin_lock_init(&blk_rq_requeue_lock);
	init_waitqueue_head(&mq_req_sim_tst_wq);
	INIT_DELAYED_WORK(&mq_requeue_work, mas_blk_ft_mq_requeue_work);
	blk_rq_sim_mode = val;
	blk_rq_sim_ongoing = 1;
	pr_err("%s mode:  %d\n", __func__, blk_rq_sim_mode);
	wait_event(mq_req_sim_tst_wq, !blk_rq_sim_ongoing);

	return (ssize_t)count;
}

/*
 * Below is for IO Latency function Test
 */
#define IO_LATENCY_TST_FILE "/data/io_latency.ft"
#define TST_DATA_PATTERN 0x5A
ssize_t mas_queue_io_latency_tst_enable_store(
	struct request_queue *q, const char *page, size_t count)
{
	char *io_buf = NULL;
	long fd = -1;
	long ret;
	mm_segment_t oldfs;

	if (strncmp(page, "auto", sizeof("auto"))) {
		pr_err("%s Invalid value\n", __func__);
		return (ssize_t)count;
	}

	pr_err("%s: start io latency auto test!\n", __func__);
	oldfs = get_fs();
	set_fs(get_ds());
	blk_queue_latency_warning_set(q, 1);

	io_buf = vmalloc(TEST_BUF_LEN);
	if (!io_buf)
		goto exit;

	memset(io_buf, TST_DATA_PATTERN, TEST_BUF_LEN);

	fd = sys_open(IO_LATENCY_TST_FILE, (O_RDWR | O_CREAT | O_DIRECT), 0);
	if (fd < 0) {
		pr_err("%s: Open %s failed!\n", __func__, IO_LATENCY_TST_FILE);
		goto free_mem;
	}

	ret = sys_write((unsigned int)fd, io_buf, TEST_BUF_LEN);
	if (ret != TEST_BUF_LEN) {
		pr_err("%s: Write %s failed!\n", __func__, IO_LATENCY_TST_FILE);
		goto close_file;
	}

	ret = sys_lseek((unsigned int)fd, TEST_BUF_LEN, SEEK_SET);
	if (ret == -1) {
		pr_err("%s: Seek %s failed!\n", __func__, IO_LATENCY_TST_FILE);
		goto close_file;
	}

	ret = sys_write((unsigned int)fd, io_buf, TEST_BUF_LEN);
	if (ret != TEST_BUF_LEN) {
		pr_err("%s: Write %s failed!\n", __func__, IO_LATENCY_TST_FILE);
		goto close_file;
	}

close_file:
	sys_close((unsigned int)fd);
free_mem:
	vfree(io_buf);
exit:
	blk_queue_latency_warning_set(q, 0);
	set_fs(oldfs);
	return (ssize_t)count;
}

/*
 * Below is for IO Busy & Idle Notify function Test
 */
#define MAS_BUSYIDLE_TEST_NB_NAME "blk_busyidle_notify_test"
static bool busyidle_notify_result_sim;
static unsigned int busyidle_notify_handle_sleep_ms;
static unsigned long busyidle_notify_multi_nb_io_trigger_sim;
static unsigned char busyidle_state = BLK_IO_IDLE;
static unsigned char busyidle_multi_nb_state[] = {
	BLK_IO_IDLE, BLK_IO_IDLE, BLK_IO_IDLE, BLK_IO_IDLE, BLK_IO_IDLE
};
static enum blk_busyidle_callback_ret test_io_idle_notify_handler(
	struct blk_busyidle_event_node *event_node,
	enum blk_idle_notify_state state)
{
	struct blk_idle_state *idle_state =
		container_of(event_node, struct blk_idle_state,
			busy_idle_test_node);
	struct blk_dev_lld *lld =
		container_of(idle_state, struct blk_dev_lld, blk_idle);
	struct request_queue *q =
		(struct request_queue *)event_node->param_data;
	enum blk_busyidle_callback_ret ret = BUSYIDLE_NO_IO;
	struct gendisk *queue_disk = q->mas_queue.queue_disk;

	if (!queue_disk)
		return ret;

	switch (state) {
	case BLK_IDLE_NOTIFY:
		pr_err("%s: BLK_IDLE_NOTIFY\n", queue_disk->disk_name);
		if (busyidle_state != BLK_IO_BUSY)
			mas_blk_rdr_panic(NO_EXTRA_MSG);
		busyidle_state = BLK_IO_IDLE;
		if (lld->blk_idle.idle_state != BLK_IO_IDLE)
			mas_blk_rdr_panic(NO_EXTRA_MSG);
		break;
	case BLK_BUSY_NOTIFY:
		pr_err("%s: BLK_BUSY_NOTIFY\n", queue_disk->disk_name);
		if (busyidle_state != BLK_IO_IDLE)
			mas_blk_rdr_panic(NO_EXTRA_MSG);
		busyidle_state = BLK_IO_BUSY;
		if (lld->blk_idle.idle_state != BLK_IO_BUSY)
			mas_blk_rdr_panic(NO_EXTRA_MSG);
		break;
	}

	if (busyidle_notify_handle_sleep_ms)
		msleep(busyidle_notify_handle_sleep_ms);

	if (busyidle_notify_result_sim)
		ret = BUSYIDLE_ERR;
	else
		ret = BUSYIDLE_NO_IO;

	return ret;
}

static enum blk_busyidle_callback_ret m_nb_test_io_idle_notify_handler(
	struct blk_busyidle_event_node *event_node,
	enum blk_idle_notify_state state)
{
	enum blk_busyidle_callback_ret ret = BUSYIDLE_ERR;
	u64 test_idx = (u64)event_node->param_data;

	if (test_idx >= ARRAY_SIZE(busyidle_multi_nb_state)) {
		pr_err("%s Invalid test_idx: %llu\n", __func__, test_idx);
		return ret;
	}

	switch (state) {
	case BLK_IDLE_NOTIFY:
		pr_err("BLK_IDLE_NOTIFY idx: %llu\n", test_idx);
		if (busyidle_multi_nb_state[test_idx] != BLK_IO_BUSY)
			mas_blk_rdr_panic(NO_EXTRA_MSG);
		busyidle_multi_nb_state[test_idx] = BLK_IO_IDLE;
		break;
	case BLK_BUSY_NOTIFY:
		pr_err("BLK_BUSY_NOTIFY idx: %llu\n", test_idx);
		if (busyidle_multi_nb_state[test_idx] != BLK_IO_IDLE)
			mas_blk_rdr_panic(NO_EXTRA_MSG);
		busyidle_multi_nb_state[test_idx] = BLK_IO_BUSY;
		break;
	default:
		pr_err("Invalid busy idle state\n");
	}
	ret = test_bit(test_idx, &busyidle_notify_multi_nb_io_trigger_sim) ?
						     BUSYIDLE_IO_TRIGGERED :
						     BUSYIDLE_NO_IO;
	pr_err("ret: %d simulate: 0x%lx\n", ret,
		busyidle_notify_multi_nb_io_trigger_sim);

	return ret;
}

ssize_t mas_queue_busyidle_tst_enable_store(
	struct request_queue *q, const char *page, size_t count)
{
	struct blk_dev_lld *lld = mas_blk_get_lld(q);
	ssize_t ret;
	unsigned long val;
	struct blk_busyidle_event_node *event_node = NULL;
	struct gendisk *queue_disk = q->mas_queue.queue_disk;

	if (!queue_disk)
		return (ssize_t)count;

	ret = queue_var_store(&val, page, count);
	if (ret < 0)
		return (ssize_t)count;

	event_node = &lld->blk_idle.busy_idle_test_node;
	if (val) {
		pr_err("%s: subscriber event\n", queue_disk->disk_name);
		strncpy(event_node->subscriber, MAS_BUSYIDLE_TEST_NB_NAME,
				SUBSCRIBER_NAME_LEN);
		event_node->busyidle_callback = test_io_idle_notify_handler;
		event_node->param_data = (void *)q;
		blk_queue_busyidle_event_subscribe(q, event_node);
	} else {
		pr_err("%s: unsubscriber event\n", queue_disk->disk_name);
		blk_queue_busyidle_event_unsubscribe(event_node);
		busyidle_state = BLK_IO_IDLE;
	}
	return (ssize_t)count;
}

ssize_t mas_queue_busyidle_multi_nb_tst_enable_store(
	struct request_queue *q, const char *page, size_t count)
{
	struct blk_dev_lld *lld = mas_blk_get_lld(q);
	u64 i;
	ssize_t ret;
	unsigned long val;
	struct blk_busyidle_event_node *event_node = NULL;

	ret = queue_var_store(&val, page, count);
	if (ret < 0)
		return (ssize_t)count;

	event_node = lld->blk_idle.busy_idle_test_nodes;
	if (val) {
		for (i = 0; i < ARRAY_SIZE(busyidle_multi_nb_state); i++) {
			strncpy(event_node[i].subscriber,
				MAS_BUSYIDLE_TEST_NB_NAME,
				SUBSCRIBER_NAME_LEN);
			event_node[i].busyidle_callback =
				m_nb_test_io_idle_notify_handler;
			event_node[i].param_data = (void *)i;
			blk_queue_busyidle_event_subscribe(q, &event_node[i]);
		}
	} else {
		for (i = 0; i < ARRAY_SIZE(busyidle_multi_nb_state); i++) {
			blk_queue_busyidle_event_unsubscribe(&event_node[i]);
			busyidle_multi_nb_state[i] = BLK_IO_IDLE;
		}
	}
	return (ssize_t)count;
}

ssize_t mas_queue_busyidle_tst_proc_result_simulate_store(
	struct request_queue *q, const char *page, size_t count)
{
	ssize_t ret;
	unsigned long val;

	ret = queue_var_store(&val, page, count);
	if (ret >= 0)
		busyidle_notify_result_sim = val ? true : false;

	pr_err("busyidle_notify_result_sim %d\n", busyidle_notify_result_sim);
	return (ssize_t)count;
}

ssize_t mas_queue_busyidle_tst_proc_latency_simulate_store(
	struct request_queue *q, const char *page, size_t count)
{
	ssize_t ret;
	unsigned long val;

	ret = queue_var_store(&val, page, count);
	if (ret >= 0)
		busyidle_notify_handle_sleep_ms = val;

	pr_err("busyidle_notify_handle_sleep_ms %u\n",
		busyidle_notify_handle_sleep_ms);
	return (ssize_t)count;
}

/*
 * Below is for System Suspend Test
 */
#define STR_SR_TST_FILE "/data/io_sr.ft"
static struct delayed_work bg_io_test_work;
static int bg_io_test_io_case_id;
static char *bg_io_test_buf;
static atomic_t bg_io_test_work_state;
static struct notifier_block blk_bg_io_notif_block;
static void blk_run_sr_test_work(struct work_struct *work)
{
	unsigned long interval_for_next_test;
	long fd = -1;
	long ret = 0;
	mm_segment_t oldfs = get_fs();

	if (!work || !atomic_read(&bg_io_test_work_state))
		return;

	switch (bg_io_test_io_case_id) {
	case MAS_BLK_FT_BG_IO_CASE_1:
		set_fs(get_ds());
		fd = sys_open(STR_SR_TST_FILE, O_RDWR | O_DIRECT, 0);
		if (fd < 0) {
			pr_err("%s: sys_open fail!! fd: %ld\n", __func__, fd);
			goto restore_fs;
		}

		if (bg_io_test_buf)
			ret = sys_write(
				(unsigned int)fd, bg_io_test_buf, TEST_BUF_LEN);
		if (ret == -1) {
			pr_err("%s: sys_write fail!!\n", __func__);
			goto close_file;
		}

		ret = sys_fsync((unsigned int)fd);
		if (ret < 0) {
			pr_emerg("%s: sys_fsync fail!!\n", __func__);
			goto close_file;
		}

		pr_err("%s: success!!\n", __func__);
close_file:
		sys_close((unsigned int)fd);
restore_fs:
		set_fs(oldfs);
		/* delay 2 jiffies for next test */
		interval_for_next_test = 2UL;
		break;
	case MAS_BLK_FT_BG_IO_CASE_2:
		/* delay 10 jiffies for next test */
		interval_for_next_test = 10UL;
		break;
	default:
		pr_err("MQ-APD:WRONG CASE ID\n");
		interval_for_next_test = 0;
		break;
	}
	if (!atomic_read(&bg_io_test_work_state))
		return;

	kblockd_schedule_delayed_work(&bg_io_test_work, interval_for_next_test);
}

static void blk_sr_tst_start(void)
{
	if (atomic_read(&bg_io_test_work_state))
		return;

	if (bg_io_test_io_case_id == MAS_BLK_FT_BG_IO_CASE_1 &&
		bg_io_test_buf) {
		unsigned int i;
		char j = 0;

		for (i = 0; i < TEST_BUF_LEN; i++, j++)
			bg_io_test_buf[i] = j;
	}
	kblockd_schedule_delayed_work(&bg_io_test_work, 0UL);
	atomic_set(&bg_io_test_work_state, 1);
	pr_err("%s: success!!\n", __func__);
}

static void blk_sr_tst_end(void)
{
	atomic_set(&bg_io_test_work_state, 0);
	cancel_delayed_work_sync(&bg_io_test_work);
	pr_err("%s: success!!\n", __func__);
}

static int blk_sr_tst_suspend_notifier(
	struct notifier_block *nb, unsigned long event, void *dummy)
{
	switch (event) {
	case PM_SUSPEND_PREPARE:
		pr_err("%s: PM_SUSPEND_PREPARE\n", __func__);
		blk_sr_tst_start();
		break;
	case PM_POST_SUSPEND:
		pr_err("%s: PM_POST_SUSPEND\n", __func__);
		blk_sr_tst_end();
		break;
	default:
		break;
	}

	if (!nb || dummy)
		pr_err("%s: nb is empty or dummy not empty\n", __func__);

	return 0;
}

static void mas_blk_ft_sr_tst_config(void)
{
	long fd = -1;
	mm_segment_t oldfs = get_fs();

	set_fs(get_ds());
	fd = sys_open(STR_SR_TST_FILE, (O_RDWR | O_CREAT | O_DIRECT), 0);
	if (fd < 0)
		pr_err("%s: sys_open fail!! fd=%ld\n", __func__, fd);
	else
		sys_close((unsigned int)fd);
	set_fs(oldfs);

	INIT_DELAYED_WORK(&bg_io_test_work, blk_run_sr_test_work);
	blk_bg_io_notif_block.notifier_call = blk_sr_tst_suspend_notifier;
	atomic_set(&bg_io_test_work_state, 0);
	register_pm_notifier(&blk_bg_io_notif_block);
}

ssize_t mas_queue_suspend_tst_store(
	struct request_queue *q, const char *page, size_t count)
{
	ssize_t ret;
	unsigned long val;

	ret = queue_var_store(&val, page, count);
	if (ret < 0)
		return (ssize_t)count;

	if (val == MAS_BLK_FT_SUSPEND_CASE_1) {
		if (bg_io_test_io_case_id)
			return (ssize_t)count;

		if (!bg_io_test_buf) {
			bg_io_test_buf = vmalloc(TEST_BUF_LEN);
			if (!bg_io_test_buf)
				return -1;

			mas_blk_ft_sr_tst_config();
		}
	} else if (val == MAS_BLK_FT_SUSPEND_CASE_2) {
		if (bg_io_test_io_case_id)
			return (ssize_t)count;

	} else {
		if (!bg_io_test_io_case_id)
			return (ssize_t)count;

		if (bg_io_test_buf) {
			vfree(bg_io_test_buf);
			bg_io_test_buf = NULL;
		}
	}

	return (ssize_t)count;
}

/*
 * Below is for Abnormal Power Down Test
 */
static unsigned long mas_blk_ft_apd_enter(enum mas_blk_apd_mode mode)
{
	unsigned long irqflags = 0;

	switch (mode) {
	case MAS_BLK_APD_INTR_EN:
		pr_err("%s: Disable interrupt\n", __func__);
		local_irq_save(irqflags);
		break;
	case MAS_BLK_APD_INTR_DISABLE:
		pr_err("%s: Disable preempt\n", __func__);
		preempt_disable();
		break;
	default:
		break;
	}

	return irqflags;
}

void mas_blk_ft_apd_run(int index)
{
	switch (index) {
	case MAS_BLK_FT_PWR_OFF:
		blk_power_off_flush(0);
		break;
	case MAS_BLK_FT_PANIC:
		pr_err("%s: Trigger Panic!\n", __func__);
		mas_blk_rdr_panic(NO_EXTRA_MSG);
		break;
	default:
		break;
	}
}

void mas_blk_ft_apd_exit(enum mas_blk_apd_mode mode, unsigned long irqflags)
{
	switch (mode) {
	case MAS_BLK_APD_INTR_EN:
		pr_err("%s: Enable interrupt\n", __func__);
		local_irq_restore(irqflags);
		break;
	case MAS_BLK_APD_INTR_DISABLE:
		pr_err("%s: Enable preempt\n", __func__);
		preempt_enable();
		break;
	default:
		break;
	}
}

ssize_t mas_queue_apd_tst_enable_store(
	struct request_queue *q, const char *page, size_t count)
{
	ssize_t ret;
	unsigned long val;

	ret = queue_var_store(&val, page, count);
	if (ret >= 0) {
		unsigned long irqflags;
		int mode = val / ONE_HUNDRED;

		irqflags = mas_blk_ft_apd_enter(mode);
		mas_blk_ft_apd_run(val % ONE_HUNDRED);
		mas_blk_ft_apd_exit(mode, irqflags);
	}
	return (ssize_t)count;
}
