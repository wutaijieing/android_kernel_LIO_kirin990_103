/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2021. All rights reserved.
 * Description: mas block core framework
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
#include <linux/gfp.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <platform_include/basicplatform/linux/powerkey_event.h>
#include <trace/events/block.h>
#include <linux/types.h>

#include "blk.h"
#include "mas_blk_core_interface.h"
#include "mas_blk_flush_interface.h"
#include "mas_blk_iosched_interface.h"
#ifdef CONFIG_MMC_MQ_CQ_HCI
#include "mas_blk_iosched_mmc_interface.h"
#endif
#include "dsm_block.h"

#define BLK_USR_CTRL_SPEED_LIMIT_MAGIC 290714
#define BLK_USR_CTRL_SPEED_LIMIT_PROTECT_DURATION_MS 3600000
#define BLK_IO_SIZE_BOUNDARY (128 << 10)

struct blk_dev_lld *mas_blk_get_lld(struct request_queue *q)
{
	if (likely(q->mq_ops))
		return (q->tag_set ? &q->tag_set->lld_func : &q->lld_func);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	return &q->lld_func;
#else
	return (q->tag_set ? &q->tag_set->lld_func : &q->lld_func);
#endif
}

struct request_queue *mas_blk_get_queue_by_lld(struct blk_dev_lld *lld)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0))
	struct blk_queue_tag *tag = NULL;
#endif
	struct blk_mq_tag_set *tag_set = NULL;
	switch (lld->type) {
	case BLK_LLD_QUEUE_BASE:
		return (struct request_queue *)(lld->data);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0))
	case BLK_LLD_QUEUE_TAG_BASE:
		tag = (struct blk_queue_tag *)(lld->data);
		return list_last_entry(
				&tag->tag_list, struct request_queue, tag_set_list);
#endif
	case BLK_LLD_TAGSET_BASE:
		tag_set = (struct blk_mq_tag_set *)(lld->data);
		return list_last_entry(
			&tag_set->tag_list, struct request_queue, tag_set_list);
	default:
		mas_blk_rdr_panic("Unknown lld type");
		return NULL;
	}
	return NULL;
}

void blk_generic_freeze(
	const void *freeze_obj, enum blk_freeze_obj_type type, bool freeze);
static void mas_blk_freeze_lld(const struct blk_dev_lld *lld, bool freeze)
{
	struct request_queue *q = NULL;
	struct blk_mq_tag_set *tag_set = NULL;

	switch (lld->type) {
	case BLK_LLD_QUEUE_BASE:
		q = (struct request_queue *)(lld->data);
		if (!q->mq_ops)
			break;

		if (freeze)
			blk_mq_freeze_queue(q);
		else
			blk_mq_unfreeze_queue(q);
		break;
	case BLK_LLD_QUEUE_TAG_BASE:
		break;
	case BLK_LLD_TAGSET_BASE:
		tag_set = (struct blk_mq_tag_set *)(lld->data);
		list_for_each_entry(q, &tag_set->tag_list, tag_set_list)
			blk_generic_freeze(q, BLK_QUEUE, freeze);

		break;
	default:
		mas_blk_rdr_panic("Unknown lld type");
		break;
	}
}

static void blk_dio_do_vl(int target_v, ktime_t dio_start, int dio_page_count)
{
	long long target_lat;
	long long real_lat;
	const unsigned int rate = 3906; /* Calc (page_count >> 8) * 1000000 */

	if (!target_v)
		return;

	/* Calc the lat @target_v */
	target_lat = ((long long)dio_page_count * rate) / target_v;
	real_lat = ktime_us_delta(ktime_get(), dio_start);
	if (real_lat < 0)
		return;
	/* Substract the actual lat */
	target_lat -= real_lat;

	if (target_lat > 0)
		udelay(target_lat);
}

void blk_dio_ck(struct gendisk *target_disk, ktime_t dio_start,
	int dio_op, int dio_page_count)
{
	if (unlikely((!target_disk) || (!target_disk->queue)))
		return;

	if (dio_op == READ)
		blk_dio_do_vl(mas_blk_get_lld(target_disk->queue)->sqr_v,
			dio_start, dio_page_count);
	else if (dio_op == WRITE)
		blk_dio_do_vl(mas_blk_get_lld(target_disk->queue)->sqw_v,
			dio_start, dio_page_count);
}

void blk_mq_tagset_vl_setup(
	struct blk_mq_tag_set *tag_set, u64 device_capacity)
{
	int sqw_bw;
	int sqr_bw;

	if (device_capacity < DEVICE_CAPACITY_128_G) {
		sqw_bw = BLK_MID_SQW_BW;
		sqr_bw = BLK_MAX_SQR_BW;
	} else {
		sqw_bw = BLK_MAX_SQW_BW;
		sqr_bw = BLK_MAX_SQR_BW;
	}
	tag_set->lld_func.sqr_v = sqr_bw;
	tag_set->lld_func.sqw_v = sqw_bw;
}

void blk_mq_tagset_vl_setup_ufs_3_1(
	struct blk_mq_tag_set *tag_set, u64 device_capacity)
{
	int sqw_bw = 0; /* 0: no restriction */
	int sqr_bw = 0;

	if (tag_set == NULL)
		return;

	if (device_capacity <= DEVICE_CAPACITY_256_G) {
		sqw_bw = BLK_MAX_SQW_BW_3_1;
		sqr_bw = BLK_MAX_SQR_BW_3_1;
	}
	tag_set->lld_func.sqr_v = sqr_bw;
	tag_set->lld_func.sqw_v = sqw_bw;
}


static void mas_blk_io_speed_adj_func(const struct request *req)
{
	if (blk_rq_is_passthrough((struct request *)req) || !req->__data_len)
		return;

	if (req_op(req) == REQ_OP_WRITE) {
		if (req->q->mas_queue.rw_l &&
			(req->__data_len <= BLK_IO_SIZE_BOUNDARY))
			udelay(req->q->mas_queue.rw_l);
		if (req->q->mas_queue.sw_l &&
			(req->__data_len > BLK_IO_SIZE_BOUNDARY))
			udelay(req->q->mas_queue.sw_l);
	}
	if (req_op(req) == REQ_OP_READ) {
		if (req->q->mas_queue.rr_l &&
			(req->__data_len <= BLK_IO_SIZE_BOUNDARY))
			udelay(req->q->mas_queue.rr_l);
		if (req->q->mas_queue.sr_l &&
			(req->__data_len > BLK_IO_SIZE_BOUNDARY))
			udelay(req->q->mas_queue.sr_l);
	}
}

/*
 * This interface will be called to trigger the speed limitation mode
 */
void mas_blk_queue_usr_ctrl_set(struct request_queue *q)
{
	if (q->mas_queue.usr_ctrl_n == BLK_USR_CTRL_SPEED_LIMIT_MAGIC) {
		q->mas_queue.rw_l = ONE_HUNDRED;
		mod_timer(&q->mas_queue.limit_setting_protect_timer,
			jiffies + msecs_to_jiffies(
			BLK_USR_CTRL_SPEED_LIMIT_PROTECT_DURATION_MS));
	} else {
		q->mas_queue.rw_l = 0;
		del_timer_sync(&q->mas_queue.limit_setting_protect_timer);
	}
}

void mas_blk_queue_usr_ctrl_recovery_timer_expire(unsigned long data)
{
	struct request_queue *q = (struct request_queue *)(uintptr_t)data;

	q->mas_queue.usr_ctrl_n = 0;
	q->mas_queue.rw_l = 0;
}

static inline void mas_blk_io_speed_adj_deinit(struct request_queue *q)
{
	del_timer_sync(&q->mas_queue.limit_setting_protect_timer);
}

static void mas_blk_io_speed_adj_init(struct request_queue *q)
{
	q->mas_queue.sr_l = 0;
	q->mas_queue.sw_l = 0;
	q->mas_queue.rr_l = 0;
	q->mas_queue.rw_l = 0;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
	timer_setup(&q->mas_queue.limit_setting_protect_timer,
		_cfi_mas_blk_queue_usr_ctrl_recovery_timer_expire, 0);
#else
	init_timer(&q->mas_queue.limit_setting_protect_timer);
	q->mas_queue.limit_setting_protect_timer.function =
		_cfi_mas_blk_queue_usr_ctrl_recovery_timer_expire;
	q->mas_queue.limit_setting_protect_timer.data = (uintptr_t)q;
#endif
}

void mas_blk_bio_clone_fast(
	struct bio *bio, struct bio *bio_src)
{
	bio->mas_bi_opf = bio_src->mas_bi_opf;
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	bio->mas_bio.stream_type = bio_src->mas_bio.stream_type;
	bio->mas_bio.cp_tag = bio_src->mas_bio.cp_tag;
	bio->mas_bio.data_ino = bio_src->mas_bio.data_ino;
	bio->mas_bio.data_idx = bio_src->mas_bio.data_idx;
	bio->mas_bio.fsync_ind = bio_src->mas_bio.fsync_ind;
	bio->mas_bio.slc_mode = bio_src->mas_bio.slc_mode;
#endif
}

void mas_blk_set_data_flag(struct request_queue *q,
	struct blk_mq_alloc_data *data, struct bio *bio, unsigned int op)
{
	data->io_flag = op;
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	if (blk_queue_query_unistore_enable(q)) {
		data->bio = bio;
		data->mas_io_flag &= ~MAS_REQ_LBA;
		if (mas_blk_match_expected_lba(q, bio))
			data->mas_io_flag |= MAS_REQ_LBA;
	}
#endif
}

/*
 * This interface will be called when the split bio will be run
 */
void mas_blk_bio_queue_split(
	struct request_queue *q, struct bio **bio, struct bio *split)
{
	mas_blk_busyidle_check_bio_endio(*bio);
	mas_blk_busyidle_check_bio(q, split);
}

/*
 * This interface is used to judge if the bio can be merge or not
 */
bool mas_blk_bio_merge_allow(const struct request *rq, const struct bio *bio)
{
#ifdef CONFIG_F2FS_TURBO_ZONE
	if (!!req_tz(rq) != !!(bio->flags & IO_TZ_MARK))
		return false;
#endif
	return true;
}

/*
 * This interface will be called after a bio has been merged.
 */
void mas_blk_bio_merge_done(
	const struct request_queue *q, const struct request *req,
	const struct request *next)
{
	mas_blk_latency_for_merge(req, next);
}

/*
 * This interface will be called in blk_account_io_completion().
 */
int mas_blk_account_io_completion(
	const struct request *req, unsigned int bytes)
{
	struct blk_dev_lld *lld = NULL;

	if (!req)
		return -1;

	lld = mas_blk_get_lld(req->q);
	if (lld) {
		if (req_op(req) == REQ_OP_DISCARD)
			lld->discard_len += bytes;

		if (req_op(req) == REQ_OP_WRITE)
			lld->write_len += bytes;
	}

	return 0;
}

#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
struct mas_process_io_prio {
	char comm[TASK_COMM_LEN];
	unsigned int set;
	unsigned int clear;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	unsigned int mas_set;
#endif
};

static const struct mas_process_io_prio process_io_prio_array[] = {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	{ "fio_vip", REQ_FG | REQ_SYNC, REQ_VIP, 0 },
	{ "fio_fg", REQ_FG | REQ_SYNC, 0 , 0 },
	{ "fio_sync", REQ_SYNC, 0 , 0 },
	{ "fio_async", 0, 0, REQ_SYNC },
#else
	{ "fio_vip", REQ_FG | REQ_VIP | REQ_SYNC, 0 },
	{ "fio_fg", REQ_FG | REQ_SYNC, 0 },
	{ "fio_sync", REQ_SYNC, 0 },
	{ "fio_async", 0, REQ_SYNC },
#endif
};

ssize_t mas_queue_io_prio_sim_show(const struct request_queue *q, char *page)
{
	return snprintf(page, PAGE_SIZE, "%d\n", q->mas_queue.io_prio_sim);
}

ssize_t mas_queue_io_prio_sim_store(
	struct request_queue *q, const char *page, size_t count)
{
	ssize_t ret;
	unsigned long val;

	ret = queue_var_store(&val, page, count);
	if (ret >= 0 && val)
		q->mas_queue.io_prio_sim = 1;
	else
		q->mas_queue.io_prio_sim = 0;

	return (ssize_t)count;
}
ssize_t mas_queue_io_flush_time_debug_show(struct request_queue *q, char *page)
{
	int ret;
	struct block_device *bdev = bdget_disk(q->mas_queue.queue_disk, 0);
	ktime_t start_time;
	ktime_t end_time;
	int flush_optimize = q->mas_queue.flush_optimize;

	if (!bdev) {
		pr_err("bdev is NULL!\n");
		return -EINVAL;
	}

	/* close flush reduce */
	if (flush_optimize)
		q->mas_queue.flush_optimize = 0;

	start_time = ktime_get();
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	ret = blkdev_issue_flush(bdev, GFP_KERNEL);
#else
	ret = blkdev_issue_flush(bdev, GFP_KERNEL, NULL);
#endif
	if (ret)
		pr_err("%s: blkdev_issue_flush fail!\n", __func__);
	end_time = ktime_get();
	/* test end, open flush reduce */
	q->mas_queue.flush_optimize = flush_optimize;

	return snprintf(page, PAGE_SIZE, "flush_request_time = %lld\n", end_time - start_time);
}

#endif /* CONFIG_MAS_DEBUG_FS */

/*
 * This interface will be called when a bio is submitted.
 */
int mas_blk_generic_make_request_check(struct bio *bio)
{
	struct request_queue *q = bio->bi_disk->queue;

#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
	if (unlikely(q->mas_queue.io_prio_sim)) {
		unsigned int i;
		char tcomm[TASK_COMM_LEN];

		get_task_comm(tcomm, current); /*lint !e514 */
		tcomm[TASK_COMM_LEN - 1] = 0;
		for (i = 0; i < ARRAY_SIZE(process_io_prio_array); i++)
			if (strstr(tcomm, process_io_prio_array[i].comm)) {
				bio->bi_opf |= process_io_prio_array[i].set;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
				bio->mas_bi_opf |= process_io_prio_array[i].mas_set;
#endif
				bio->bi_opf &= ~process_io_prio_array[i].clear;
				break;
			}
	}
#endif
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	mas_blk_bio_set_opf_unistore(bio);
#endif
	mas_blk_latency_bio_check(bio, BIO_PROC_STAGE_SUBMIT);
	if (unlikely(mas_blk_flush_async_dispatch(q, bio))) {
		bio->bi_status = BLK_STS_OK;
		bio_endio(bio);
		return 1;
	}
	mas_blk_busyidle_check_bio(q, bio);

	return 0;
}

/*
 * This interface will be called when a bio will be passed into io scheduler.
 */
void mas_blk_generic_make_request(const struct bio *bio)
{
	mas_blk_latency_bio_check(
		(struct bio *)bio, BIO_PROC_STAGE_GENERIC_MAKE_REQ);
}

/*
 * This interface will be called when process enable plug.
 */
void mas_blk_start_plug(struct blk_plug *plug)
{
	INIT_LIST_HEAD(&plug->mas_blk_list);
	plug->flush_plug_list_fn = NULL;
}

/*
 * This interface will be called when the plug should be flushed.
 */
void mas_blk_flush_plug_list(const struct blk_plug *plug, bool from_schedule)
{
	if (likely(!list_empty(&plug->mas_blk_list) &&
		    plug->flush_plug_list_fn))
		plug->flush_plug_list_fn(
			(struct blk_plug *)plug, from_schedule);
}

/*
 * This interface will be called when the bio is ended.
 */
void mas_blk_bio_endio(const struct bio *bio)
{
	mas_blk_busyidle_check_bio_endio((struct bio *)bio);
	mas_blk_latency_bio_check((struct bio *)bio, BIO_PROC_STAGE_ENDBIO);
}

/*
 * This interface will be called before the bio will be freed.
 */
void mas_blk_bio_free(const struct bio *bio)
{
	if (unlikely(bio->mas_bio.io_in_count & MAS_IO_IN_COUNT_SET)) {
#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
		pr_emerg("free counted bio, bio=0x%lx\n",
			(uintptr_t)bio);
		mas_blk_rdr_panic(NO_EXTRA_MSG);
#else
		mas_blk_busyidle_check_bio_endio((struct bio *)bio);
#endif
	}
}

/*
 * This interface will be called when a request will be configed according to a
 */
void mas_blk_request_init_from_bio(struct request *req, struct bio *bio)
{
	mas_blk_latency_req_check(req, REQ_PROC_STAGE_INIT_FROM_BIO);
	req->mas_req.requeue_reason = REQ_REQUEUE_IO_NO_REQUEUE;
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	mas_blk_request_init_from_bio_unistore(req, bio);
#endif
	bio->mas_bio.io_req = req;

#ifdef CONFIG_F2FS_TURBO_ZONE
	if (bio->flags & IO_TZ_MARK)
		req->mas_req.mas_featrue_flag |= CUST_REQ_TURBO_ZONE;
#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
	if ((bio->flags & IO_TZ_MARK) && bio_op(bio) == REQ_OP_WRITE)
		req->q->mas_queue.tz_write_bytes += bio->bi_iter.bi_size;
#endif
#endif /* CONFIG_F2FS_TURBO_ZONE */
}

/*
 * This interface will be called when dm device pass a cloned request to block
 * layer.
 */
void mas_blk_insert_cloned_request(
	const struct request_queue *q, const struct request *rq)
{
	mas_blk_busyidle_check_request_bio(q, rq);
}

/*
 * This interface will be called when non-fs request will be executed.
 */
void mas_blk_request_execute_nowait(
	const struct request_queue *q, const struct request *rq,
	rq_end_io_fn *done)
{
	mas_blk_busyidle_check_execute_request(q, (struct request *)rq, done);
}

/*
 * This interface will be called when request will be inited in MQ.
 */
void mas_blk_mq_rq_ctx_init(
	struct request_queue *q, struct blk_mq_ctx *ctx, struct request *rq)
{
	if (q->mas_queue_ops && q->mas_queue_ops->mq_req_init_fn)
		q->mas_queue_ops->mq_req_init_fn(q, ctx, rq);
}

/*
 * This interface will be called when request will be passed to driver in MQ.
 */
void mas_blk_mq_request_start(const struct request *req)
{
	mas_blk_latency_req_check((struct request *)req, REQ_PROC_STAGE_START);
	if (unlikely(req->q->mas_queue.usr_ctrl_n ==
		     BLK_USR_CTRL_SPEED_LIMIT_MAGIC))
		mas_blk_io_speed_adj_func(req);
}

/*
 * This interface will be called when request will be passed to driver.
 */
void mas_blk_request_start(const struct request *req)
{
	mas_blk_latency_req_check((struct request *)req, REQ_PROC_STAGE_START);
}

/*
 * This interface will be called when request will be requeue.
 */
void mas_blk_requeue_request(const struct request *rq)
{
	mas_blk_latency_req_check(
		(struct request *)rq, REQ_PROC_STAGE_SQ_REQUEUE);
}

/*
 * This interface will be called when request has been returned by driver.
 */
void mas_blk_request_update(
	const struct request *req, blk_status_t error, unsigned int nr_bytes)
{
	struct blk_dev_lld *lld = mas_blk_get_lld(req->q);

	if (req_op(req) == REQ_OP_DISCARD)
		lld->discard_len += nr_bytes;

	if (req_op(req) == REQ_OP_WRITE)
		lld->write_len += nr_bytes;

	if (req_op(req) == REQ_OP_FLUSH)
		mas_blk_flush_update(req, error);

	mas_blk_latency_req_check(
		(struct request *)req, REQ_PROC_STAGE_UPDATE);

#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	mas_blk_req_update_unistore((struct request *)req, error, nr_bytes);
#endif
}

/*
 * This interface will be called when request is releaseing in MQ.
 */
void mas_blk_mq_request_free(struct request *rq)
{
	rq->mas_req.make_req_nr = 0;
	rq->mas_req.protocol_nr = 0;
	mas_blk_latency_req_check(rq, REQ_PROC_STAGE_FREE);
}

/*
 * This interface will be called when request is releaseing in SQ.
 */
void mas_blk_request_put(struct request *req)
{
	req->mas_req.make_req_nr = 0;
	req->mas_req.protocol_nr = 0;
	mas_blk_latency_req_check(req, REQ_PROC_STAGE_FREE);
}

static void mas_blk_dev_lld_init(
	struct blk_dev_lld *blk_lld, enum blk_lld_base type, const void *data)
{
#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
	if (blk_lld->init_magic == BLK_LLD_INIT_MAGIC)
		mas_blk_rdr_panic("blk_lld has been inited already!");
#endif
	blk_lld->init_magic = BLK_LLD_INIT_MAGIC;
	blk_lld->write_num = 0;
	spin_lock_init(&blk_lld->write_num_lock);
	blk_lld->make_req_nr = 1;
	spin_lock_init(&blk_lld->make_req_nr_lock);
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	mas_blk_dev_lld_init_unistore(blk_lld);
#endif
	INIT_LIST_HEAD(&blk_lld->lld_list);
	blk_lld->type = type;
	blk_lld->data = (void *)data;
	if (mas_blk_busyidle_state_init(&blk_lld->blk_idle))
		blk_lld->features &= ~BLK_LLD_BUSYIDLE_SUPPORT;
	if (blk_lld->features & BLK_LLD_IOSCHED_UFS_HW_IDLE)
		blk_lld->blk_idle.idle_intr_support = 1;
#ifdef CONFIG_MAS_MQ_USING_CP
	blk_lld->features |= BLK_LLD_UFS_CP_EN;
#endif
#ifndef CONFIG_MAS_ORDER_PRESERVE
	blk_lld->features &= ~BLK_LLD_UFS_ORDER_EN;
	blk_lld->dev_order_en = false;
#endif

	blk_lld->sqr_v = 0;
	blk_lld->sqw_v = 0;
}

/*
 * This interface will be called after block device partition table check done.
 */
void mas_blk_check_partition_done(struct gendisk *disk, bool has_part_tbl)
{
	disk->queue->mas_queue.blk_part_tbl_exist = has_part_tbl;
}

/*
 * This interface will be called when init request queue.
 */
void mas_blk_allocated_queue_init(struct request_queue *q)
{
	INIT_LIST_HEAD(&q->mas_queue.dump_list);
	q->mas_queue.blk_part_tbl_exist = false;
	mas_blk_dev_lld_init(&q->lld_func, BLK_LLD_QUEUE_BASE, (void *)q);
}

static void mas_blk_queue_init_common(struct request_queue *q)
{
	mas_blk_dump_register_queue(q);
	mas_blk_queue_latency_init(q);
	mas_blk_queue_async_flush_init(q);
	mas_blk_io_speed_adj_init(q);
}

/*
 * This interface will be called when SQ init request queue.
 */
void mas_blk_sq_init_allocated_queue(const struct request_queue *q)
{
	mas_blk_queue_init_common((struct request_queue *)q);
}

/*
 * This interface will be called when MQ init request queue.
 */
void mas_blk_mq_init_allocated_queue(struct request_queue *q)
{
	unsigned int i;
	struct blk_mq_tag_set *set = q->tag_set;

	for (i = 0; i < set->nr_hw_queues; i++) {
		if (!set->tags[i])
			continue;
		set->tags[i]->set = set;
	}

	mas_blk_queue_init_common(q);

	q->mas_queue_ops =
		set->mas_tagset_ops ? set->mas_tagset_ops->queue_ops : NULL;
	if (q->mas_queue_ops && q->mas_queue_ops->mq_iosched_init_fn) {
		int ret = q->mas_queue_ops->mq_iosched_init_fn(q);
		if (unlikely(ret)) {
			pr_err("%s: mq_iosched_init_fn failed. err = %d\n",
				__func__, ret);
			mas_blk_rdr_panic(NO_EXTRA_MSG);
		}
	}
}

/*
 * This interface will be called when MQ start to release request queue.
 */
void mas_blk_mq_free_queue(const struct request_queue *q)
{
	if (q->mas_queue_ops && q->mas_queue_ops->mq_iosched_exit_fn)
		q->mas_queue_ops->mq_iosched_exit_fn(
			(struct request_queue *)q);
}

/*
 * This interface will be called when request queue release.
 */
void mas_blk_cleanup_queue(struct request_queue *q)
{
	mas_blk_dump_unregister_queue(q);
	mas_blk_io_speed_adj_deinit(q);
	mas_blk_flush_reduced_queue_unregister(q);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
/*
 * This interface will be called when SQ tags init.
 */
void mas_blk_allocated_tags_init(struct blk_queue_tag *tags)
{
	mutex_init(&tags->tag_list_lock);
	INIT_LIST_HEAD(&tags->tag_list);
	memset(&tags->lld_func, 0, sizeof(struct blk_dev_lld));
	mas_blk_dev_lld_init(
		&tags->lld_func, BLK_LLD_QUEUE_TAG_BASE, (void *)tags);
}

/*
 * This interface will be called when request queue add to SQ tag pool.
 */
void blk_add_queue_tags(struct blk_queue_tag *tags, struct request_queue *q)
{
	mutex_lock(&tags->tag_list_lock);
	list_add_tail(&q->tag_set_list, &tags->tag_list);
	mutex_unlock(&tags->tag_list_lock);
}
#endif

/*
 * This interface will be called when MQ tagset init.
 */
void mas_blk_mq_allocated_tagset_init(struct blk_mq_tag_set *set)
{
	mas_blk_dev_lld_init(&set->lld_func, BLK_LLD_TAGSET_BASE, (void *)set);

	/*
	 * MAS IO Scheduler Configuration
	 */
	if (set->lld_func.features & BLK_LLD_IOSCHED_UFS_MQ)
		set->mas_tagset_ops = &mas_ufs_blk_tagset_ops;
#ifdef CONFIG_MMC_MQ_CQ_HCI
	else if (set->lld_func.features & BLK_LLD_IOSCHED_MMC_MQ)
		set->mas_tagset_ops = &mas_mmc_blk_tagset_ops;
#endif
	else
		set->mas_tagset_ops = NULL;
}

/*
 * This interface will be called when request queue bind with block disk.
 */
void mas_blk_queue_register(
	struct request_queue *q, const struct gendisk *disk)
{
	q->mas_queue.queue_disk = (struct gendisk *)disk;
}

/*
 * This interface will be called when the whole block module init
 */
int __init mas_blk_dev_init(void)
{
	mas_blk_dump_init();
	mas_blk_latency_init();
	mas_blk_dsm_init();
	return 0;
}

void blk_write_throttle(struct request_queue *queue, int level)
{
	if (queue && queue->mas_queue_ops &&
	    queue->mas_queue_ops->blk_write_throttle_fn)
		queue->mas_queue_ops->blk_write_throttle_fn(queue, level);
}

/*
 * This interface is design for request queue freeze,
 * but only MQ will be in effect currently.
 */
void blk_generic_freeze(
	const void *freeze_obj, enum blk_freeze_obj_type type, bool freeze)
{
	struct request_queue *q = NULL;
	struct blk_dev_lld *lld = NULL;

	if (unlikely(!freeze_obj)) {
		mas_blk_rdr_panic("freeze_obj is NULL!");
		return;
	}

	switch (type) {
	case BLK_LLD:
		lld = (struct blk_dev_lld *)freeze_obj;
		mas_blk_freeze_lld(lld, freeze);
		break;
	case BLK_QUEUE:
		q = (struct request_queue *)freeze_obj;
		if (q->mq_ops) {
			if (freeze)
				blk_mq_freeze_queue(q);
			else
				blk_mq_unfreeze_queue(q);
		}
		break;
	default:
		mas_blk_rdr_panic(NO_EXTRA_MSG);
		break;
	}
}

bool blk_dev_write_order_preserved(struct block_device *bdev)
{
	struct request_queue *q = NULL;
	struct blk_dev_lld *lld = NULL;

	if (unlikely(!bdev || (!bdev->bd_disk) || (!bdev->bd_disk->queue)))
		return false;

	q = bdev->bd_disk->queue;
	lld = mas_blk_get_lld(q);

	return (lld->features & BLK_LLD_UFS_ORDER_EN) ? true : false;
}

#ifdef CONFIG_MAS_ORDER_PRESERVE
static void blk_req_set_flag(struct blk_dev_lld *lld, struct request *req)
{
	if (lld->features & BLK_LLD_UFS_UNISTORE_EN) {
		req->cmd_flags &= ~REQ_META;
		req->cmd_flags &= ~REQ_FG;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		req->mas_cmd_flags &= ~REQ_VIP;
#else
		req->cmd_flags &= ~REQ_VIP;
#endif
		req->cmd_flags |= REQ_SYNC;
	}
}
#endif

void blk_req_set_make_req_nr(struct request *req)
{
	unsigned long flags = 0;
	struct blk_dev_lld *lld = mas_blk_get_lld(req->q);

#ifdef CONFIG_MAS_ORDER_PRESERVE
	blk_req_set_flag(lld, req);
#endif

	if ((lld->features & BLK_LLD_UFS_ORDER_EN) ||
		(lld->features & BLK_LLD_UFS_UNISTORE_EN)) {
		req->mas_req.mas_featrue_flag |= CUST_REQ_ORDER;
		spin_lock_irqsave(&lld->make_req_nr_lock, flags);
		if (unlikely(!lld->make_req_nr))
			lld->make_req_nr++;
		req->mas_req.make_req_nr = lld->make_req_nr++;
		spin_unlock_irqrestore(&lld->make_req_nr_lock, flags);
	}
}

#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
#ifdef CONFIG_MAS_ORDER_PRESERVE
ssize_t mas_queue_order_enabled_show(struct request_queue *q, char *page)
{
	unsigned long offset = 0;
	struct blk_dev_lld *lld = mas_blk_get_lld(q);

	offset += snprintf(page, PAGE_SIZE, "order_enabled: %d\n", /* unsafe_function_ignore: snprintf  */
		(lld->features & BLK_LLD_UFS_ORDER_EN) ? 1 : 0);

	return (ssize_t)offset;
}

ssize_t mas_queue_order_enabled_store(
	struct request_queue *q, const char *page, size_t count)
{
	ssize_t ret;
	unsigned long val;
	struct blk_dev_lld *lld = mas_blk_get_lld(q);

	ret = queue_var_store(&val, page, count);
	if (ret < 0)
		return (ssize_t)count;

	if (!(lld->dev_order_en))
		return (ssize_t)count;

	if (val)
		lld->features |= BLK_LLD_UFS_ORDER_EN;
	else
		lld->features &= ~BLK_LLD_UFS_ORDER_EN;

	return (ssize_t)count;
}

static int order_debug_en;
int mas_blk_order_debug_en(void)
{
	return order_debug_en;
}

ssize_t mas_queue_order_debug_en_show(struct request_queue *q, char *page)
{
	unsigned long offset;

	offset = snprintf(page, PAGE_SIZE, "order_debug_en: %d\n", order_debug_en);

	return (ssize_t)offset;
}

ssize_t mas_queue_order_debug_en_store(
	struct request_queue *q, const char *page, size_t count)
{
	ssize_t ret;
	unsigned long val;

	ret = queue_var_store(&val, page, count);
	if (ret < 0)
		return (ssize_t)count;

	if (val)
		order_debug_en = val;
	else
		order_debug_en = 0;

	return (ssize_t)count;
}
#endif /* CONFIG_MAS_ORDER_PRESERVE */
#endif /* CONFIG_MAS_DEBUG_FS */

unsigned int blk_req_get_order_nr(struct request *req, bool extern_protect)
{
	unsigned int order = 0;
	unsigned long flags;
	struct blk_dev_lld *lld = NULL;

	if (req_cp(req))
		goto out;

	lld = mas_blk_get_lld(req->q);
		if (req->mas_req.protocol_nr) {
		pr_err("old protocol_nr exist! %u\n",
					req->mas_req.protocol_nr);
			return req->mas_req.protocol_nr;
		}
		if (lld->features & BLK_LLD_UFS_ORDER_EN) {
			if (!extern_protect)
				spin_lock_irqsave(&lld->write_num_lock, flags);
			lld->write_num++;
			if (unlikely(!lld->write_num))
				lld->write_num++;
			order = lld->write_num;
			if (!extern_protect)
				spin_unlock_irqrestore(&lld->write_num_lock, flags); /*lint !e644 */
		}

out:
	req->mas_req.protocol_nr = order;
	return order;
}

void blk_queue_order_enable(struct request_queue *q, bool enable)
{
	struct blk_dev_lld *lld = mas_blk_get_lld(q);

	if (enable) {
		lld->features |= BLK_LLD_UFS_ORDER_EN;
		lld->dev_order_en = true;
	} else {
		lld->features &= ~BLK_LLD_UFS_ORDER_EN;
		lld->dev_order_en = false;
	}
}

bool blk_queue_query_order_enable(struct request_queue *q)
{
	struct blk_dev_lld *lld = mas_blk_get_lld(q);
	return ((lld->features & BLK_LLD_UFS_ORDER_EN) ? true : false);
}

void blk_order_nr_reset(struct blk_mq_tag_set *tag_set)
{
	struct blk_dev_lld *blk_lld = NULL;
	if (unlikely(!tag_set))
		return;
	blk_lld = &tag_set->lld_func;
	blk_lld->write_num = 0;
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	mas_blk_order_info_reset(blk_lld);
#endif
}

#ifdef CONFIG_MAS_MQ_USING_CP
void blk_queue_cp_enable(struct request_queue *q, bool enable)
{
	struct blk_dev_lld *lld = mas_blk_get_lld(q);

	if (enable)
		lld->features |= BLK_LLD_UFS_CP_EN;
	else
		lld->features &= ~BLK_LLD_UFS_CP_EN;
}
#endif

ssize_t mas_queue_status_show(
	const struct request_queue *q, char *page, unsigned long len)
{
	struct blk_dev_lld *lld = mas_blk_get_lld((struct request_queue *)q);
	int offset = mas_blk_dump_lld_status(lld, page, (int)len);

	offset += mas_blk_dump_queue_status2(
		(struct request_queue *)q, page + offset, (int)(len - offset));
	return (ssize_t)offset;
}
