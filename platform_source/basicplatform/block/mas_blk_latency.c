/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2019. All rights reserved.
 * Description: block io latency framework
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

#include <linux/backing-dev.h>
#include <linux/bio.h>
#include <linux/blk-cgroup.h>
#include <linux/blk-mq.h>
#include <linux/blkdev.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/fault-inject.h>
#include <linux/highmem.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kernel_stat.h>
#include <linux/list_sort.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/ratelimit.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/swap.h>
#include <linux/task_io_accounting_ops.h>
#include <linux/writeback.h>
#include <trace/events/block.h>
#include <linux/types.h>
#include <linux/version.h>
#if defined(CONFIG_HW_ZEROHUNG) && defined(CONFIG_DETECT_HUAWEI_HUNG_TASK)
#include <chipset_common/hwzrhung/zrhung_wp_io.h>
#endif

#include "blk.h"
#ifdef CONFIG_HUAWEI_DSM
#include "dsm_block.h"
#endif
#include "mas_blk_latency_interface.h"

#define LATENCY_WARNING_DEF_MS 2000
#define LATENCY_LOG_BUF_SIZE 500UL
#define NORMAL_TIMEOUT_MS 1000ul
#define IN_IRQ_TIMEOUT_MS 10ul
#define BIO_UDELAY_UNIT_US 10ul
#ifdef CONFIG_HP_CORE
#define FAULT_OUT_TIMEOUT_MS 100ul
#define BATCH_OUT_TIMEOUT_MS 500ul
#endif

static spinlock_t latency_log_protect_lock;

static void mas_blk_latency_bio_submit_func(struct bio *bio);
static void mas_blk_latency_bio_endio_func(struct bio *bio);
static void mas_blk_latency_req_init_func(struct request *req);
static void mas_blk_latency_req_free_func(struct request *req);

const struct bio_delay_stage_config bio_stage_cfg[BIO_PROC_STAGE_MAX] = {
	{ "BIO_SUBMIT", mas_blk_latency_bio_submit_func },
	{ "BIO_END", mas_blk_latency_bio_endio_func },
	{ "BIO_ENTER", NULL },
	{ "BIO_MAKE_REQ", NULL },
	{ "BIO_WBT_DONE", NULL },
};

const struct req_delay_stage_config req_stage_cfg[REQ_PROC_STAGE_MAX] = {
	{ "REQ_INIT", mas_blk_latency_req_init_func },
	{ "FSEQ_PREFLUSH", NULL },
	{ "FSEQ_DATA", NULL },
	{ "FSEQ_POSTFLUSH", NULL },
	{ "FSEQ_DONE", NULL },
	{ "REQ_RQ_START", NULL },
	{ "REQ_MQ_ADDTO_PLUGLIST", NULL },
	{ "REQ_MQ_FLUSH_PLUGLIST", NULL },
	{ "REQ_MQ_ADDTO_SYNC_LIST", NULL },
	{ "REQ_MQ_ADDTO_ASYNC_LIST", NULL },
	{ "REQ_MQ_QUEUE_ERR", NULL },
	{ "REQ_MQ_QUEUE_RQ", NULL },
	{ "REQ_MQ_QUEUE_SCSI_OUT", NULL },
	{ "REQ_MQ_QUEUE_SCSI_TARGET", NULL },
	{ "REQ_MQ_QUEUE_SCSI_HOST", NULL },
	{ "REQ_MQ_QUEUE_SCSI_RECOVERY", NULL },
	{ "REQ_MQ_QUEUE_SCSI_HBB", NULL },
	{ "REQ_MQ_QUEUE_SCSI_CAN", NULL },
	{ "REQ_MQ_QUEUE_SCSI_SELF_BLOCKED", NULL },
	{ "REQ_MQ_HOLD_START", NULL },
	{ "REQ_MQ_HOLD_FAIL", NULL },
	{ "REQ_MQ_HOLD_SUCCESS", NULL },
	{ "REQ_SQ_REQUEUE", NULL },
	{ "REQ_SCSI_SFT_IRQ_DONE", NULL },
	{ "REQ_SCSI_SFT_IRQ_DONE_UNISTORE", NULL },
	{ "REQ_SCSI_SFT_IRQ_INSERT", NULL },
	{ "REQ_SCSI_SFT_IRQ_FINISH_REQUEUE", NULL },
	{ "REQ_SCSI_SFT_IRQ_EH_ADD", NULL },
	{ "REQ_SCSI_EH_INC", NULL },
	{ "REQ_SCSI_EH_STU", NULL },
	{ "REQ_SCSI_EH_DEVICE", NULL },
	{ "REQ_SCSI_EH_TARGET", NULL },
	{ "REQ_SCSI_EH_BUS", NULL },
	{ "REQ_SCSI_EH_HOST", NULL },
	{ "REQ_SCSI_EH_OFFLINE", NULL },
	{ "REQ_SCSI_EH_FLUSH", NULL },
	{ "REQ_UFS_EH_START", NULL },
	{ "REQ_UFS_EH_END", NULL },
	{ "REQ_RQ_UPDATE", NULL },
	{ "REQ_RQ_FREE",	mas_blk_latency_req_free_func },
};

#ifdef CONFIG_HP_CORE
static inline bool mas_blk_bio_is_hp(const struct bio *bio)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	return (bio->mas_bi_opf & (REQ_FAULT_OUT | REQ_BATCH_OUT)) ? true : false;
#else
	return (bio->bi_opf & (REQ_FAULT_OUT | REQ_BATCH_OUT)) ? true : false;
#endif
}

static inline unsigned long hp_bio_timueout_threshold(
	const struct bio *bio)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	return (bio->mas_bi_opf & REQ_FAULT_OUT) ? FAULT_OUT_TIMEOUT_MS :
		BATCH_OUT_TIMEOUT_MS;
#else
	return (bio->bi_opf & REQ_FAULT_OUT) ? FAULT_OUT_TIMEOUT_MS :
		BATCH_OUT_TIMEOUT_MS;
#endif
}
#endif

static unsigned long bio_timeout_threshold(
	const struct bio *bio, const struct request_queue *q)
{
#ifdef CONFIG_HP_CORE
	if (mas_blk_bio_is_hp(bio))
		return hp_bio_timueout_threshold(bio);
#endif

#ifdef CONFIG_MAS_MQ_USING_CP
	if (mas_blk_bio_is_cp(bio))
		return MSEC_PER_SEC;
#endif

	return q->mas_queue.io_lat_warning_thresh;
}

static void mas_blk_setup_latency_tiemr(
	struct bio *bio, const struct request_queue *q)
{
	int target_cpu;
	int next_cpu;
	struct blk_bio_cust *mas_bio = NULL;

	mas_bio = &bio->mas_bio;
	mas_bio->latency_timer_running = 1;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
	timer_setup(&mas_bio->latency_expire_timer,
		__mas_blk_latency_check_timer_expire, 0);
#else
	init_timer(&mas_bio->latency_expire_timer);
	mas_bio->latency_expire_timer.data = (uintptr_t)bio;
	mas_bio->latency_expire_timer.function =
		__mas_blk_latency_check_timer_expire;
#endif
	mas_bio->latency_expire_timer.expires =
		jiffies + msecs_to_jiffies(bio_timeout_threshold(bio, q));
	/* latency_expire_timer will access bio, get it first */
	bio_get(bio);
	next_cpu = (raw_smp_processor_id() + 1) % num_possible_cpus();
	if (cpu_online(next_cpu))
		target_cpu = next_cpu;
	else
		for_each_online_cpu(target_cpu) /*lint !e574 */
			if (target_cpu != raw_smp_processor_id())
				break;
	if ((unsigned int)target_cpu >= num_possible_cpus())
		target_cpu = raw_smp_processor_id();
	add_timer_on(&mas_bio->latency_expire_timer, target_cpu);
	mas_bio->dispatch_task = current;
	mas_bio->task_pid = current->pid;
	mas_bio->task_tgid = current->tgid;
	memcpy(mas_bio->task_comm, current->comm, sizeof(mas_bio->task_comm));
}

static void mas_blk_latency_bio_submit_func(struct bio *bio)
{
	int i;
	struct request_queue *q = NULL;

	if (!bio->bi_disk)
		return;

	q = bio->bi_disk->queue;
	if (unlikely(!q || !q->mas_queue.io_latency_enable))
		return;

	if (likely(bio->mas_bio.fs_io_flag != IO_FROM_SUBMIT_BIO_MAGIC))
		mas_blk_setup_latency_tiemr(bio, q);

	bio->mas_bio.fs_io_flag = IO_FROM_SUBMIT_BIO_MAGIC;
	bio->mas_bio.io_req = NULL;
	for (i = 0; i < BIO_PROC_STAGE_MAX; i++)
		bio->mas_bio.bio_stage_ktime[i] = 0;

#ifdef CONFIG_MAS_IO_CLASSIFY_SCHED_TUNE
	if (bio->mas_bio.dispatch_task &&
		schedtune_prefer_idle(bio->mas_bio.dispatch_task))
		bio->bi_opf |= REQ_FG;
	else
		bio->bi_opf &= ~REQ_FG;
#endif
}

static inline bool mas_blk_bio_timeout(
	const struct bio *bio, const struct request_queue *q)
{
	return ktime_after(ktime_get(),
		ktime_add_ms(
			bio->mas_bio.bio_stage_ktime[BIO_PROC_STAGE_SUBMIT],
			bio_timeout_threshold(bio, q)));
}

static void mas_blk_dump_timeouted_bio(
	const struct bio *bio, const struct request_queue *q)
{
	struct blk_dev_lld *lld = mas_blk_get_lld((struct request_queue *)q);
	struct blk_queue_ops *ops = q->mas_queue_ops;

	mas_blk_dump_bio(bio, BLK_DUMP_WARNING);
	mas_blk_dump_queue_status(q, BLK_DUMP_WARNING);
	mas_blk_dump_fs(bio);

	if (lld->dump_fn)
		lld->dump_fn((struct request_queue *)q, BLK_DUMP_WARNING);
	if (ops && ops->blk_status_dump_fn)
		ops->blk_status_dump_fn((struct request_queue *)q, BLK_DUMP_WARNING);
}

/*
 * This enforces a rate limit: not more than 10 kernel messages every 5s.
 */
static DEFINE_RATELIMIT_STATE(latency_endbio_ratelmt_state, 5 * HZ, 10);
static void mas_blk_latency_bio_endio_func(struct bio *bio)
{
	unsigned long timeout = ((in_interrupt() ? IN_IRQ_TIMEOUT_MS :
		NORMAL_TIMEOUT_MS) * USEC_PER_MSEC) / BIO_UDELAY_UNIT_US;
	struct request_queue *q = bio->mas_bio.q;

	if (q && unlikely(mas_blk_bio_timeout(bio, q) &&
		    ___ratelimit(&latency_endbio_ratelmt_state, "bio timeout")))
		mas_blk_dump_timeouted_bio(bio, q);

	bio->mas_bio.fs_io_flag = 0;
	/* latency_expire_timer was deleted successfully */
	if (likely(del_timer(&bio->mas_bio.latency_expire_timer))) {
		bio_put(bio);
	} else {
		while (bio->mas_bio.latency_timer_running && timeout--)
			udelay(BIO_UDELAY_UNIT_US);

		if (unlikely(!timeout) &&
			___ratelimit(&latency_endbio_ratelmt_state,
				"Failed to wait io latency timer"))
			dump_stack();
	}
	bio->mas_bio.latency_timer_running = 0;

	bio->mas_bio.io_req = NULL;
}

static void mas_blk_latency_req_init_func(struct request *req)
{
	int i;
	struct blk_req_cust *mas_req = &req->mas_req;

	if (likely(req->q && req->q->mas_queue.io_latency_enable)) {
		mas_req->fs_io_flag = IO_FROM_SUBMIT_BIO_MAGIC;
		for (i = 0; i < REQ_PROC_STAGE_MAX; i++)
			mas_req->req_stage_ktime[i] = 0;
		mas_req->dispatch_task = current;
		mas_req->task_pid = current->pid;
		mas_req->task_tgid = current->tgid;
		memcpy(mas_req->task_comm, current->comm,
			sizeof(mas_req->task_comm));
	} else {
		mas_req->fs_io_flag = 0;
	}
}

static void mas_blk_latency_req_free_func(struct request *req)
{
	req->mas_req.fs_io_flag = 0;
	req->mas_req.dispatch_task = NULL;
	while (req->bio) {
		struct bio *bio = req->bio;

		bio->mas_bio.io_req = NULL;
		req->bio = bio->bi_next;
	}
}

/*
 * This enforces a rate limit: not more than 5 kernel messages
 * every 3s.
 */
static DEFINE_RATELIMIT_STATE(latency_log_ratelmt_state, 3 * HZ, 5);
void mas_blk_latency_check_timer_expire(unsigned long data)
{
	unsigned long flags;
	struct bio *bio = (struct bio *)(uintptr_t)data;
	struct request_queue *q = NULL;
	struct blk_dev_lld *lld = NULL;

	spin_lock_irqsave(&latency_log_protect_lock, flags);
	if (unlikely(bio->mas_bio.fs_io_flag != IO_FROM_SUBMIT_BIO_MAGIC)) {
		pr_err("%s %d Invalid fs_io_flag: 0x%x\n", __func__, __LINE__,
			bio->mas_bio.fs_io_flag);
		goto put_bio;
	}

	if (!bio->bi_disk || !bio->bi_disk->queue) {
		pr_err("%s %d Invalid request queue !\n", __func__, __LINE__);
		goto put_bio;
	}

	q = bio->bi_disk->queue;
	lld = mas_blk_get_lld(q);

	if (!___ratelimit(&latency_log_ratelmt_state, "io latency"))
		goto put_bio;
#ifdef CONFIG_HUAWEI_DSM
#ifdef CONFIG_MAS_MQ_USING_CP
	if (mas_blk_bio_is_cp(bio))
		dsm_block_record(DSM_BLOCK_CP);
	else
#endif
		dsm_block_record(DSM_BLOCK_NORMAL);
#endif

	mas_blk_dump_timeouted_bio(bio, q);

#if defined(CONFIG_HW_ZEROHUNG) && defined(CONFIG_DETECT_HUAWEI_HUNG_TASK)
	iowp_report(
		bio->mas_bio.task_pid, bio->mas_bio.task_tgid, "io latency");
#endif

put_bio:
	spin_unlock_irqrestore(&latency_log_protect_lock, flags);
	bio->mas_bio.latency_timer_running = 0;
	bio_put(bio);
}

/*
 * This function is used to record the bio latency checkpoint
 */
void mas_blk_latency_bio_check(
	struct bio *bio, enum bio_process_stage_enum bio_stage)
{
	if (unlikely((bio_stage != BIO_PROC_STAGE_SUBMIT) &&
		     (bio->mas_bio.fs_io_flag != IO_FROM_SUBMIT_BIO_MAGIC)))
		return;

	if (bio_stage_cfg[bio_stage].function)
		bio_stage_cfg[bio_stage].function(bio);

	bio->mas_bio.bio_stage_ktime[bio_stage] = ktime_get();
}

/*
 * This function is used to record the request latency checkpoint
 */
void mas_blk_latency_req_check(
	struct request *req, enum req_process_stage_enum req_stage)
{
	if (unlikely((req_stage != REQ_PROC_STAGE_INIT_FROM_BIO) &&
		     (req->mas_req.fs_io_flag != IO_FROM_SUBMIT_BIO_MAGIC)))
		return;

	if (likely(req_stage_cfg[req_stage].function))
		req_stage_cfg[req_stage].function(req);

	req->mas_req.req_stage_ktime[req_stage] = ktime_get();
}

/*
 * This function is used to immigrate the req info in the bio from origin req
 */
void mas_blk_latency_for_merge(
	const struct request *req, const struct request *next)
{
	struct bio *bio = NULL;

	bio = next->bio;
	while (bio) {
		bio->mas_bio.io_req = (struct request *)req;
		bio = bio->bi_next;
	}
}

void mas_blk_queue_latency_init(struct request_queue *q)
{
	struct blk_dev_lld *blk_lld = mas_blk_get_lld(q);

	if (!blk_lld->latency_warning_threshold_ms)
		blk_lld->latency_warning_threshold_ms = LATENCY_WARNING_DEF_MS;
	q->mas_queue.io_lat_warning_thresh =
		blk_lld->latency_warning_threshold_ms;
	q->mas_queue.io_latency_enable =
		blk_lld->features & BLK_LLD_LATENCY_SUPPORT ? 1 : 0;
}

void __init mas_blk_latency_init(void)
{
	spin_lock_init(&latency_log_protect_lock);
#if defined(CONFIG_HW_ZEROHUNG) && defined(CONFIG_DETECT_HUAWEI_HUNG_TASK)
	iowp_workqueue_init();
#endif
}

/*
 * This interface will be used to enable latency and set the warning threshold
 * on MQ tagset
 */
void blk_mq_tagset_latency_warning_set(
	struct blk_mq_tag_set *tag_set, unsigned int warning_threshold_ms)
{
	tag_set->lld_func.features |= BLK_LLD_LATENCY_SUPPORT;
	tag_set->lld_func.latency_warning_threshold_ms = warning_threshold_ms ?
							 warning_threshold_ms :
							LATENCY_WARNING_DEF_MS;
}

/*
 * This interface will be used to enable latency and set the warning threshold
 * on request queue
 */
void blk_queue_latency_warning_set(
	struct request_queue *q, unsigned int warning_threshold_ms)
{
	struct blk_dev_lld *blk_lld = mas_blk_get_lld(q);

	blk_lld->features |= BLK_LLD_LATENCY_SUPPORT;
	blk_lld->latency_warning_threshold_ms =
		warning_threshold_ms ? warning_threshold_ms
				     : LATENCY_WARNING_DEF_MS;
	q->mas_queue.io_lat_warning_thresh =
		blk_lld->latency_warning_threshold_ms;
	q->mas_queue.io_latency_enable = 1;
}

static void mas_blk_queue_io_latency_check_enable(
	struct request_queue *q, int enable)
{
	struct blk_dev_lld *blk_lld = mas_blk_get_lld(q);

	if (q->mas_queue.io_latency_enable == enable)
		return;

	q->mas_queue.io_latency_enable = enable ? 1 : 0;
	if (enable)
		blk_lld->features |= BLK_LLD_LATENCY_SUPPORT;
	else
		blk_lld->features &= ~BLK_LLD_LATENCY_SUPPORT;
}

static void mas_blk_queue_io_latency_warning_threshold(
	struct request_queue *q, unsigned int warning_threshold_ms)
{
	struct blk_dev_lld *blk_lld = mas_blk_get_lld(q);

	blk_lld->latency_warning_threshold_ms =
		warning_threshold_ms ? warning_threshold_ms
				     : LATENCY_WARNING_DEF_MS;
	q->mas_queue.io_lat_warning_thresh =
		warning_threshold_ms ? warning_threshold_ms
				     : LATENCY_WARNING_DEF_MS;
}

ssize_t mas_queue_io_latency_warning_threshold_store(
	const struct request_queue *q, const char *page, size_t count)
{
	ssize_t ret;
	unsigned long val;

	ret = queue_var_store(&val, page, count);
	if (ret >= 0) {
		pr_err("%s: io latency warning threshold is set to %lu\n",
			__func__, val);
		mas_blk_queue_io_latency_check_enable(
			(struct request_queue *)q, val ? 1 : 0);
		mas_blk_queue_io_latency_warning_threshold(
			(struct request_queue *)q, val);
	}

	return (ssize_t)count;
}
