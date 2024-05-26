/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2019. All rights reserved.
 * Description:  mas block dump
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

#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/module.h>
#include <platform_include/basicplatform/linux/rdr_platform.h>
#include <linux/types.h>

#include "blk.h"
#include "blk-mq.h"
#include "blk-mq-tag.h"
#include "mas_blk_dump_interface.h"
#include "mas_blk_latency_interface.h"

static LIST_HEAD(dump_list);
static DEFINE_SPINLOCK(dump_list_lock);

#define MAS_BLK_DUMP_BUF_LEN 512UL
static int mas_blk_io_type_op(u64 op, char *type_buf, int len)
{
	if (len < 1)
		return 0;

	switch (op) {
	case REQ_OP_READ:
		type_buf[0] = 'R';
		break;
	case REQ_OP_WRITE:
		type_buf[0] = 'W';
		break;
	case REQ_OP_DISCARD:
		type_buf[0] = 'D';
		break;
	case REQ_OP_SECURE_ERASE:
		type_buf[0] = 'E';
		break;
	case REQ_OP_WRITE_SAME:
		type_buf[0] = 'T';
		break;
	case REQ_OP_FLUSH:
		type_buf[0] = 'F';
		break;
	default:
		pr_warn("%s: Unknown OP %ld!\n", __func__, op);
		return 0;
	}

	return 1;
}

static int mas_blk_io_type_flag(u64 flag, char *type_buf, int len)
{
	int offset = 0;

	if ((flag & REQ_FUA) && offset < len) {
		type_buf[offset] = 'A';
		offset++;
	}
	if ((flag & REQ_SYNC) && offset < len) {
		type_buf[offset] = 'S';
		offset++;
	}
	if ((flag & REQ_META) && offset < len) {
		type_buf[offset] = 'M';
		offset++;
	}
	if ((flag & REQ_PREFLUSH) && offset < len) {
		type_buf[offset] = 'F';
		offset++;
	}

	return offset;
}

static int mas_blk_io_type_priv_flag_v5(unsigned long flag, u64 mas_flag, char *type_buf, u32 len)
{
	int offset = 0;
	unsigned long priv_flag = mas_flag;

	if ((flag & REQ_FG) && offset < (int)len) {
		type_buf[offset] = 'H';
		offset++;
	}
	if ((priv_flag & REQ_CP) && offset < (int)len) {
		type_buf[offset] = 'C';
		offset++;
	}
	if ((priv_flag & REQ_TZ) && offset < (int)len) {
		type_buf[offset] = 'Z';
		offset++;
	}
	if ((priv_flag & REQ_VIP) && offset < (int)len) {
		type_buf[offset] = 'V';
		offset++;
	}
	if ((priv_flag & REQ_TT_RB) && offset < (int)len) {
		type_buf[offset] = 'T';
		offset++;
	}

	return offset;
}

static int mas_blk_io_type_priv_flag(u64 flag, char *type_buf, u32 len)
{
	int offset = 0;

	if ((flag & REQ_FG) && offset < (int)len) {
		type_buf[offset] = 'H';
		offset++;
	}
	if ((flag & REQ_CP) && offset < (int)len) {
		type_buf[offset] = 'C';
		offset++;
	}
	if ((flag & REQ_TZ) && offset < (int)len) {
		type_buf[offset] = 'Z';
		offset++;
	}
	if ((flag & REQ_VIP) && offset < (int)len) {
		type_buf[offset] = 'V';
		offset++;
	}
	if ((flag & REQ_TT_RB) && offset < (int)len) {
		type_buf[offset] = 'T';
		offset++;
	}

	return offset;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
static char *mas_blk_io_type(u64 op, unsigned long flag, u64 mas_flag, char *type_buf, int len)
#else
static char *mas_blk_io_type(u64 op, u64 flag, char *type_buf, int len)
#endif
{
	int offset = 0;

	offset += mas_blk_io_type_op(op, type_buf, len);
	offset += mas_blk_io_type_flag(flag, type_buf + offset, len - offset);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	offset += mas_blk_io_type_priv_flag_v5(
		flag, mas_flag, type_buf + offset, len - offset);
#else
	offset += mas_blk_io_type_priv_flag(
			flag, type_buf + offset, len - offset);
#endif
	type_buf[offset] = '\0';

	return type_buf;
}

#ifdef CONFIG_MAS_UNISTORE_PRESERVE
static int mas_blk_io_type_flag_unistore(u64 flag, char *type_buf, int len)
{
	int offset = 0;
	if ((flag & MAS_REQ_LBA) && offset < len) {
		type_buf[offset] = 'L';
		offset++;
	}
	return offset;
}

static char *mas_blk_io_type_unistore(
	u64 op, u64 flag, u64 flag_unis, char *type_buf, int len)
{
	int offset = 0;

	offset += mas_blk_io_type_op(op, type_buf, len);
	offset += mas_blk_io_type_flag(flag, type_buf + offset, len - offset);
	offset += mas_blk_io_type_flag_unistore(
		flag_unis, type_buf + offset, len - offset);
	offset += mas_blk_io_type_priv_flag_v5(
		flag, flag_unis, type_buf + offset, len - offset);
	type_buf[offset] = '\0';

	return type_buf;
}

static unsigned int mas_blk_dump_request_unist(
	struct request *rq, char *log, int len)
{
	int count = 0;
	unsigned int mas_sec_size;
	unsigned int mas_pu_size;

	if (!blk_queue_query_unistore_enable(rq->q))
		return count;

	mas_sec_size = mas_blk_get_sec_size(rq->q);
	mas_pu_size = mas_blk_get_pu_size(rq->q);
	if (!mas_sec_size || !mas_pu_size)
		return count;

	count += snprintf(log, len,
		"make_nr:%u, order:%u, pu:0x%llx, offset:0x%llx, "
		"lba:0x%llx - 0x%llx. stream type: %d\n",
		rq->mas_req.make_req_nr, rq->mas_req.protocol_nr,
		(blk_rq_pos(rq) >> SECTION_SECTOR) / mas_pu_size,
		(blk_rq_pos(rq) >> SECTION_SECTOR) % mas_pu_size,
		(blk_rq_pos(rq) >> SECTION_SECTOR) / mas_sec_size,
		(blk_rq_pos(rq) >> SECTION_SECTOR) % mas_sec_size,
		rq->mas_req.stream_type);
	return count;
}

static unsigned int mas_blk_dump_bio_unist(
	const struct bio *bio, char *log, int len)
{
	int count = 0;
	unsigned int mas_sec_size = 0;
	struct request_queue *q = bio->bi_disk->queue;

	if (!blk_queue_query_unistore_enable(q))
		return count;

	if (bio->mas_bio.io_req)
		return count;

	mas_sec_size = mas_blk_get_sec_size(q);
	if (!mas_sec_size)
		return count;

	count += snprintf(log, len,
		"\nlba:0x%llx - 0x%llx. stream type: %d, mas_bi_opf 0x%llx",
		(bio->bi_iter.bi_sector >> SECTION_SECTOR) / mas_sec_size,
		(bio->bi_iter.bi_sector >> SECTION_SECTOR) % mas_sec_size,
		bio->mas_bio.stream_type,
		bio->mas_bi_opf);
	return count;
}
#endif

static char *mas_blk_io_type_rq(
	struct request *rq, char *type_buf, int len)
{
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	return mas_blk_io_type_unistore(
		req_op(rq), rq->cmd_flags, rq->mas_cmd_flags, type_buf, len);
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	return mas_blk_io_type(
		req_op(rq), rq->cmd_flags, rq->mas_cmd_flags, type_buf, len);
#else
	return mas_blk_io_type(
			req_op(rq), rq->cmd_flags, type_buf, len);
#endif
#endif
}

static char *get_cmp_type(struct request *rq)
{
	if (blk_rq_is_scsi(rq))
		return "TYPE_BLOCK_PC";
	else if (blk_rq_is_private(rq))
		return "TYPE_DRV_PRIV";
	else
		return "TYPE_FS";
}

/*
 * This function is used to dump the request info
 */
void mas_blk_dump_request(struct request *rq, enum blk_dump_scene s)
{
	int i;
	char log[MAS_BLK_DUMP_BUF_LEN];
	char *cmp_type = NULL;
	unsigned int count = 0;
	char io_type[IO_BUF_LEN];
	char *type = mas_blk_io_type_rq(
		rq, io_type, sizeof(io_type));
	char *prefix = mas_blk_prefix_str(s);
	cmp_type = get_cmp_type(rq);

	count += snprintf(log, MAS_BLK_DUMP_BUF_LEN,
			"rq: 0x%pK, type:%s len:%u %s requeue_reason: %d tag: %d ",
			rq, type, rq->__data_len, cmp_type,
			rq->mas_req.requeue_reason, rq->tag);

	count += snprintf(log + count, MAS_BLK_DUMP_BUF_LEN - count,
			"rq_hoq_flag: %llx rq_cp_flag: %llx ",
			req_hoq(rq), req_cp(rq));

	if (rq->mas_req.dispatch_task) {
#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
		count += snprintf(log + count, MAS_BLK_DUMP_BUF_LEN - count,
			"disp_pid: %d %s, task: 0x%pK\n",
			(int)rq->mas_req.task_pid, rq->mas_req.task_comm,
			rq->mas_req.dispatch_task);
#else
		count += snprintf(log + count, MAS_BLK_DUMP_BUF_LEN - count,
			"disp_pid: %d %s\n",
			(int)rq->mas_req.task_pid, rq->mas_req.task_comm);
#endif
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
		count += mas_blk_dump_request_unist(
			rq, log + count, MAS_BLK_DUMP_BUF_LEN - count);
#endif
	} else {
		count += snprintf(log + count, MAS_BLK_DUMP_BUF_LEN - count,
			"\n");
	}

	for (i = 0; i < REQ_PROC_STAGE_MAX; i++) {
		if (!rq->mas_req.req_stage_ktime[i])
			continue;
		count += snprintf(log + count,
			(size_t)(MAS_BLK_DUMP_BUF_LEN - count),
			"%s: %lld ", req_stage_cfg[i].stage_name,
			rq->mas_req.req_stage_ktime[i]);
	}

	log[MAS_BLK_DUMP_BUF_LEN - 1] = 0;
	pr_err("%s: %s\n", prefix, log);
}

/*
 * This function is used to dump the bio info
 */
void mas_blk_dump_bio(const struct bio *bio, enum blk_dump_scene s)
{
	int i;
	char log[MAS_BLK_DUMP_BUF_LEN];
	int count = 0;
	struct blk_bio_cust *mas_bio = (struct blk_bio_cust *)&bio->mas_bio;
	char io_type[IO_BUF_LEN];
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	char *type = mas_blk_io_type(
		bio_op(bio), bio->bi_opf, bio->mas_bi_opf, io_type, sizeof(io_type));
#else
	char *type = mas_blk_io_type(bio_op(bio), bio->bi_opf, io_type, sizeof(io_type));
#endif
	char *prefix = mas_blk_prefix_str(s);

	count += snprintf(log, MAS_BLK_DUMP_BUF_LEN,
			"bio: 0x%pK, type:%s len:%u ",
			bio, type, bio->bi_iter.bi_size);

	if (mas_bio->dispatch_task) {
		count += snprintf(log + count, MAS_BLK_DUMP_BUF_LEN - count,
			"disp_pid: %d %s",
			mas_bio->task_pid, mas_bio->task_comm);
#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
		count += snprintf(log + count, MAS_BLK_DUMP_BUF_LEN - count,
			", task: 0x%pK", mas_bio->dispatch_task);
#endif
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
		count += mas_blk_dump_bio_unist(
			bio, log + count, MAS_BLK_DUMP_BUF_LEN - count);
#endif
	}
	count += snprintf(log + count, MAS_BLK_DUMP_BUF_LEN - count, "\n");

	for (i = 0; i < BIO_PROC_STAGE_MAX; i++) {
		if (!(mas_bio->bio_stage_ktime[i]))
			continue;
		count += snprintf(log + count, MAS_BLK_DUMP_BUF_LEN - count,
			"<%s:%lld> ", bio_stage_cfg[i].stage_name,
			mas_bio->bio_stage_ktime[i]);
	}
	log[count] = '\0';
	pr_err("%s %s,current: %lld\n", prefix, log, ktime_get());

	if (bio->mas_bio.io_req)
		mas_blk_dump_request(bio->mas_bio.io_req, s);
}

static int __mas_blk_dump_printf(char *buf, int len, const char *str)
{
	int offset = 0;

	if (buf && len > 0)
		offset += snprintf(buf, len, "%s", str);
	else
		pr_err("%s", str);

	return offset;
}

static int mas_blk_q_dump_header(
	struct request_queue *q, char *buf, unsigned long len)
{
	unsigned char tmp_str[MAS_BLK_DUMP_BUF_LEN];

	(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "Queue Status:\n");
	return __mas_blk_dump_printf(buf, len, tmp_str);
}

static int mas_blk_q_dump_opt_features(
	struct request_queue *q, char *buf, unsigned long len)
{
	unsigned char tmp_str[MAS_BLK_DUMP_BUF_LEN];
	int offset = 0;

	if (q->mas_queue.io_latency_enable)
		snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN,
			"\tIO Latency Warning: enable\tThreshold(ms): %u\n",
			q->mas_queue.io_lat_warning_thresh);
	else
		snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN,
			"\tIO Latency Warning: disable\n");
	offset += __mas_blk_dump_printf(buf, len, tmp_str);

	(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "\tFlush Reduce: %s\n",
		q->mas_queue.flush_optimize ? "enable" : "disable");
	offset += __mas_blk_dump_printf(buf + offset, len - offset, tmp_str);

#ifdef CONFIG_SCSI_UFS_INLINE_CRYPTO
	(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "\tInline Crypto: %s\n",
		q->inline_crypt_support ? "enable" : "disable");
#endif
	offset += __mas_blk_dump_printf(buf + offset, len - offset, tmp_str);
	return offset;
}

int mas_blk_dump_queue_status2(
	struct request_queue *q, char *buf, int len)
{
	int offset = 0;

	offset += mas_blk_q_dump_header(q, buf, len);
	offset += mas_blk_q_dump_opt_features(q, buf + offset, len - offset);
	if (buf)
		buf[offset] = '\0';

	return offset;
}

/*
 * This function is used to dump request queue status
 */
void mas_blk_dump_queue_status(
	const struct request_queue *q, enum blk_dump_scene s)
{
	char *prefix = mas_blk_prefix_str(s);
	unsigned int inflight[2];

	if (!q->mas_queue.queue_disk)
		return;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	blk_mq_in_flight_rw((struct request_queue *)q, &q->mas_queue.queue_disk->part0, inflight);
#else
	inflight[0] = atomic_read(&q->mas_queue.queue_disk->part0.in_flight[0]);
	inflight[1] = atomic_read(&q->mas_queue.queue_disk->part0.in_flight[1]);
#endif

	pr_err("%s: bdev %s, ptable %s, r_inflt: %d w_inflt: %d",
		prefix, q->mas_queue.queue_disk->disk_name,
		q->mas_queue.blk_part_tbl_exist ? "exist" : "none",
		inflight[0],
		inflight[1]);

#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
	pr_err("q: 0x%pK", q);
#endif

	if (s == BLK_DUMP_PANIC)
		mas_blk_dump_queue_status2((struct request_queue *)q, NULL, 0);

	if (q->mas_queue_ops && q->mas_queue_ops->blk_status_dump_fn)
		q->mas_queue_ops->blk_status_dump_fn(
			(struct request_queue *)q, s);
}

static int mas_blk_lld_dump_latency(
	struct blk_dev_lld *lld, char *buf, int len)
{
	unsigned char tmp_str[MAS_BLK_DUMP_BUF_LEN];

	if (lld->features & BLK_LLD_LATENCY_SUPPORT)
		snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN,
			"\tLatency Warning:enable\tThreshold(ms):%u\n",
			lld->latency_warning_threshold_ms);
	else
		snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN,
			"\tLatency Warning: disable\n");
	return __mas_blk_dump_printf(buf, len, tmp_str);
}

static int mas_blk_lld_dump_ioscheduler(
	struct blk_dev_lld *lld, char *buf, int len)
{
	unsigned char tmp_str[MAS_BLK_DUMP_BUF_LEN];
	int offset = 0;

	(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "\tIO Scheduler: ");
	offset += __mas_blk_dump_printf(buf, len, tmp_str);

	if (lld->features & BLK_LLD_IOSCHED_UFS_MQ)
		(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "MAS MQ\n");
	else
		(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "Default Scheduler\n");
	offset += __mas_blk_dump_printf(buf + offset, len - offset, tmp_str);

	return offset;
}

static int mas_blk_lld_dump_busyidle(
	struct blk_dev_lld *lld, char *buf, int len)
{
	unsigned char tmp_str[MAS_BLK_DUMP_BUF_LEN];
	int offset = 0;

	(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "\tBusy/Idle Notifier: %s\t",
		(lld->features & BLK_LLD_BUSYIDLE_SUPPORT) ? "enable"
							     : "disable");
	offset += __mas_blk_dump_printf(buf, len, tmp_str);

	switch (lld->blk_idle.idle_state) {
	case BLK_IO_BUSY:
		(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "\tState: BUSY\t");
		break;
	case BLK_IO_IDLE:
		(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "\tState: IDLE\t");
		break;
	default:
		return offset;
	}
	offset += __mas_blk_dump_printf(buf + offset, len - offset, tmp_str);

	(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "\tIO Count: %d\n",
		atomic_read(&lld->blk_idle.io_count));
	offset += __mas_blk_dump_printf(buf + offset, len - offset, tmp_str);

	return offset;
}

#ifdef CONFIG_MAS_UNISTORE_PRESERVE
static int mas_blk_lld_dump_fgio_busyidle(
	struct blk_dev_lld *lld, char *buf, int len)
{
	unsigned char tmp_str[MAS_BLK_DUMP_BUF_LEN];
	int offset = 0;

	(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "\tFGIO/Busy/Idle Notifier: %s\t",
		(lld->features & BLK_LLD_BUSYIDLE_SUPPORT) ? "enable"
							     : "disable");
	offset += __mas_blk_dump_printf(buf, len, tmp_str);

	switch (lld->blk_idle.fg_io_idle_state) {
	case BLK_FG_IO_BUSY:
		(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "\tFG State: BUSY\t");
		break;
	case BLK_FG_IO_IDLE:
		(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "\tFG State: IDLE\t");
		break;
	default:
		return offset;
	}
	offset += __mas_blk_dump_printf(buf + offset, len - offset, tmp_str);

	(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "\tFG IO Count: %d\n",
		atomic_read(&lld->blk_idle.fg_io_count));
	offset += __mas_blk_dump_printf(buf + offset, len - offset, tmp_str);

	return offset;
}

static int mas_blk_lld_dump_opt_feature_unist(
	struct blk_dev_lld *lld, char *buf, int len, char *tmp_str)
{
	int offset = 0;

	if (!(lld->features & BLK_LLD_UFS_UNISTORE_EN))
		return offset;

	if (!lld->mas_sec_size) {
		pr_err("%s, section size error: %u\n", __func__, lld->mas_sec_size);
		return offset;
	}

	(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "Write Order: %s Order Num: %u"
		"LAST STREAM TYPE: %u, fsync ind: %s\n",
		(lld->features & BLK_LLD_UFS_UNISTORE_EN) ? "enable" : "disable",
		lld->write_num, lld->last_stream_type, lld->fsync_ind ? "true" : "false");
	offset += __mas_blk_dump_printf(buf, len, tmp_str);

	(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "expected_lba: "
		"0x%llx - 0x%llx, 0x%llx - 0x%llx, 0x%llx - 0x%llx, 0x%llx - 0x%llx\n",
		lld->expected_lba[BLK_STREAM_COLD_NODE - 1] / (lld->mas_sec_size),
		lld->expected_lba[BLK_STREAM_COLD_NODE - 1] % (lld->mas_sec_size),
		lld->expected_lba[BLK_STREAM_COLD_DATA - 1] / (lld->mas_sec_size),
		lld->expected_lba[BLK_STREAM_COLD_DATA - 1] % (lld->mas_sec_size),
		lld->expected_lba[BLK_STREAM_HOT_NODE - 1] / (lld->mas_sec_size),
		lld->expected_lba[BLK_STREAM_HOT_NODE - 1] % (lld->mas_sec_size),
		lld->expected_lba[BLK_STREAM_HOT_DATA - 1] / (lld->mas_sec_size),
		lld->expected_lba[BLK_STREAM_HOT_DATA - 1] % (lld->mas_sec_size));
	offset += __mas_blk_dump_printf(buf + offset, len - offset, tmp_str);

	(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN,
		"expected_pu: 0x%llx, current_pu_size: 0x%llx\n",
		lld->expected_pu[BLK_STREAM_HOT_DATA - 1],
		lld->current_pu_size[BLK_STREAM_HOT_DATA - 1]);
	offset += __mas_blk_dump_printf(buf + offset, len - offset, tmp_str);

	(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "expected_lba_time: "
		"%llu, %llu, %llu, %llu\n",
		lld->expected_refresh_time[BLK_STREAM_COLD_NODE - 1],
		lld->expected_refresh_time[BLK_STREAM_COLD_DATA - 1],
		lld->expected_refresh_time[BLK_STREAM_HOT_NODE - 1],
		lld->expected_refresh_time[BLK_STREAM_HOT_DATA - 1]);
	offset += __mas_blk_dump_printf(buf + offset, len - offset, tmp_str);

	(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "old_section: "
		"0x%llx, 0x%llx, 0x%llx, 0x%llx\n",
		lld->old_section[BLK_STREAM_COLD_NODE - 1],
		lld->old_section[BLK_STREAM_COLD_DATA - 1],
		lld->old_section[BLK_STREAM_HOT_NODE - 1],
		lld->old_section[BLK_STREAM_HOT_DATA - 1]);
	offset += __mas_blk_dump_printf(buf + offset, len - offset, tmp_str);

	return offset;
}

static void mas_blk_dump_unistore_section_info(struct blk_dev_lld *lld)
{
	unsigned int stream;
	unsigned long flags;
	struct unistore_section_info *first_section_info = NULL;
	int locked;

	for (stream = 0; stream < BLK_ORDER_STREAM_NUM; stream++) {
		locked = spin_trylock_irqsave(&lld->expected_lba_lock[stream], flags);
		if (!locked) {
			pr_err("stream %u, trylock fail\n", stream + 1);
			continue;
		}

		if (list_empty_careful(&lld->section_list[stream])) {
			pr_err("stream %u, section list is empty\n", stream + 1);
		} else {
			first_section_info = list_first_entry(&lld->section_list[stream],
				struct unistore_section_info, section_list);
			pr_err("stream %u, first 0x%llx, next 0x%llx, rcv flag %d\n",
				stream + 1,
				first_section_info->section_id,
				first_section_info->next_section_id,
				first_section_info->rcv_io_complete_flag);
		}
		spin_unlock_irqrestore(&lld->expected_lba_lock[stream], flags);
	}
}

static void mas_blk_dump_unistore_buf_bio(struct request_queue *q)
{
	unsigned char stream;
	unsigned long flags;
	struct bio *pos = NULL;
	struct blk_dev_lld *lld = mas_blk_get_lld(q);
	if (!lld || !(lld->mas_sec_size) ||
		!(lld->features & BLK_LLD_UFS_UNISTORE_EN))
		return;

	pr_err("%s, dump buf bio start\n", __func__);
	for (stream = 1; stream <= BLK_ORDER_STREAM_NUM; stream++) {
		spin_lock_irqsave(&lld->buf_bio_list_lock[stream], flags);
		if (list_empty_careful(&lld->buf_bio_list[stream]))
			pr_err("recovery_dump, stream: %u, buffer list is empty\n", stream);
		else
			list_for_each_entry(pos, &lld->buf_bio_list[stream], buf_bio_list_node)
				pr_err("recovery_dump, stream: %u, bio_nr: %u, "
					"bio_lba: 0x%llx - 0x%llx, len: %u\n",
					stream, pos->bio_nr,
					(pos->bi_iter.bi_sector >> SECTION_SECTOR) /
						(lld->mas_sec_size),
					(pos->bi_iter.bi_sector >> SECTION_SECTOR) %
						(lld->mas_sec_size),
					pos->bi_iter.bi_done / BLKSIZE);
		spin_unlock_irqrestore(&lld->buf_bio_list_lock[stream], flags);
	}
	pr_err("%s, dump buf bio end\n", __func__);
}

static void mas_blk_dump_unistore_info(struct blk_dev_lld *lld)
{
	if (!lld || !(lld->mas_sec_size) || !(lld->mas_pu_size) ||
		!(lld->features & BLK_LLD_UFS_UNISTORE_EN))
		return;

	pr_err("expected_lba: 0x%llx - 0x%llx, 0x%llx - 0x%llx, "
			"0x%llx - 0x%llx, 0x%llx - 0x%llx\n",
		lld->expected_lba[BLK_STREAM_COLD_NODE - 1] / (lld->mas_sec_size),
		lld->expected_lba[BLK_STREAM_COLD_NODE - 1] % (lld->mas_sec_size),
		lld->expected_lba[BLK_STREAM_COLD_DATA - 1] / (lld->mas_sec_size),
		lld->expected_lba[BLK_STREAM_COLD_DATA - 1] % (lld->mas_sec_size),
		lld->expected_lba[BLK_STREAM_HOT_NODE - 1] / (lld->mas_sec_size),
		lld->expected_lba[BLK_STREAM_HOT_NODE - 1] % (lld->mas_sec_size),
		lld->expected_lba[BLK_STREAM_HOT_DATA - 1] / (lld->mas_sec_size),
		lld->expected_lba[BLK_STREAM_HOT_DATA - 1] % (lld->mas_sec_size));

	pr_err("expected_pu: 0x%llx (0x%llx - 0x%llx)",
		lld->expected_pu[BLK_STREAM_HOT_DATA - 1],
		lld->expected_pu[BLK_STREAM_HOT_DATA - 1] * lld->mas_pu_size / lld->mas_sec_size,
		lld->expected_pu[BLK_STREAM_HOT_DATA - 1] * lld->mas_pu_size % lld->mas_sec_size);
	pr_err("current_pu_size: 0x%llx", lld->current_pu_size[BLK_STREAM_HOT_DATA - 1]);

	pr_err("old_section: 0x%llx, 0x%llx, 0x%llx, 0x%llx\n",
		lld->old_section[BLK_STREAM_COLD_NODE - 1],
		lld->old_section[BLK_STREAM_COLD_DATA - 1],
		lld->old_section[BLK_STREAM_HOT_NODE - 1],
		lld->old_section[BLK_STREAM_HOT_DATA - 1]);

	pr_err("reset_count: %d\n", atomic_read(&lld->reset_cnt));
	pr_err("recovery_flag: %d\n", atomic_read(&lld->recovery_flag));
	pr_err("recovery_pwron_flag: %d\n", atomic_read(&lld->recovery_pwron_flag));
	pr_err("recovery_pwron_inprocess_flag: %d\n",
		atomic_read(&lld->recovery_pwron_inprocess_flag));
	pr_err("max_recovery_size: 0x%llx\n", lld->max_recovery_size);
	pr_err("async_submit_bio_fail: %d\n", atomic_read(&lld->async_fail_cnt));

	mas_blk_dump_unistore_section_info(lld);
}
#endif

static int mas_blk_lld_dump_opt_feature(
	struct blk_dev_lld *lld, char *buf, int len)
{
	unsigned char tmp_str[MAS_BLK_DUMP_BUF_LEN];
	int offset = 0;

	(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "\tFlush Reduce: %s\n",
		 (lld->features & BLK_LLD_FLUSH_REDUCE_SUPPORT) ? "enable" :
								"disable");
	offset += __mas_blk_dump_printf(buf, len, tmp_str);

	(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "\tUFS command priority: %s\n",
		(lld->features & BLK_LLD_UFS_CP_EN) ? "enable" : "disable");
	offset += __mas_blk_dump_printf(buf + offset, len - offset, tmp_str);

	(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "\tUFS order preserving: %s\n",
		(lld->features & BLK_LLD_UFS_ORDER_EN) ? "enable" : "disable");
	offset += __mas_blk_dump_printf(buf + offset, len - offset, tmp_str);

#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	offset += mas_blk_lld_dump_opt_feature_unist(
		lld, buf + offset, len - offset, tmp_str);
#endif

	return offset;
}

static int mas_blk_lld_dump_header(
	struct blk_dev_lld *lld, char *buf, int len)
{
	unsigned char tmp_str[MAS_BLK_DUMP_BUF_LEN];
	int offset = 0;

	(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "LLD Status:\n");
	offset += __mas_blk_dump_printf(buf, len, tmp_str);

	(void)snprintf(tmp_str, MAS_BLK_DUMP_BUF_LEN, "\tPanic Dump: %s\n",
		(lld->features & BLK_LLD_DUMP_SUPPORT) ? "enable" : "disable");
	// cppcheck-suppress *
	offset += __mas_blk_dump_printf(buf + offset, len - offset, tmp_str);

	return offset;
}

/*
 * This function is used to dump low level driver object status
 */
int mas_blk_dump_lld_status(struct blk_dev_lld *lld, char *buf, int len)
{
	int offset = 0;

	offset += mas_blk_lld_dump_header(lld, buf, len);
	// cppcheck-suppress *
	offset += mas_blk_lld_dump_latency(lld, buf + offset, len - offset);
	// cppcheck-suppress *
	offset += mas_blk_lld_dump_busyidle(lld, buf + offset, len - offset);
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	if (lld->features & BLK_LLD_UFS_UNISTORE_EN)
		// cppcheck-suppress *
		offset += mas_blk_lld_dump_fgio_busyidle(lld, buf + offset, len - offset);
#endif
	offset += mas_blk_lld_dump_ioscheduler(
		lld, buf + offset, len - offset);
	offset += mas_blk_lld_dump_opt_feature(
		lld, buf + offset, len - offset);
	if (buf)
		buf[offset] = '\0';

	return offset;
}

static void mas_blk_dump_counted_io(
	struct blk_dev_lld *lld, struct request_queue *q,
	enum blk_dump_scene s)
{
	if (!list_empty(&lld->blk_idle.bio_list)) {
		struct bio *pos = NULL;

		pr_err("bio_list:\n");
		list_for_each_entry(pos, &lld->blk_idle.bio_list, cnt_list) {
			if (pos->mas_bio.q == q)
				mas_blk_dump_bio(pos, s);
		}
	}
	if (!list_empty(&lld->blk_idle.req_list)) {
		struct request *pos = NULL;

		pr_err("req_list:\n");
		list_for_each_entry(pos, &lld->blk_idle.req_list, cnt_list) {
			if (pos->q == q)
				mas_blk_dump_request(pos, s);
		}
	}
}

static void mas_blk_dump_queue(
	struct request_queue *q, enum blk_dump_scene s)
{
	struct blk_dev_lld *lld = mas_blk_get_lld(q);

	if (lld->dump_fn) {
		mas_blk_dump_lld_status(lld, NULL, 0);
		pr_err("bio_count = %llu req_count = %llu\n",
			lld->blk_idle.bio_count, lld->blk_idle.req_count);
		lld->dump_fn(q, BLK_DUMP_PANIC);
		lld->dump_fn = NULL;
	}

	mas_blk_dump_queue_status(q, BLK_DUMP_PANIC);

#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	mas_blk_dump_unistore_info(lld);
#endif

	mas_blk_dump_counted_io(lld, q, s);
	if (q->mas_queue_ops && q->mas_queue_ops->blk_req_dump_fn)
		q->mas_queue_ops->blk_req_dump_fn(q, s);
}

/*
 * ecall mas_blk_dump() to test blk panic dump functionality
 * don't change this function name or you'll need to correct
 * the ST case too.
 */
#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
void mas_blk_panic_dump(void)
#else
static void mas_blk_panic_dump(void)
#endif
{
	struct request_queue *q = NULL;
	struct blk_queue_cust *mas_queue = NULL;

	list_for_each_entry(mas_queue, &dump_list, dump_list) {
		q = (struct request_queue *)container_of(
			mas_queue, struct request_queue, mas_queue);
		if (q->queuedata)
			mas_blk_dump_queue(q, BLK_DUMP_PANIC);
	}

#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	if (q)
		mas_blk_dump_unistore_buf_bio(q);
#endif
}

int mas_blk_panic_notify(
	struct notifier_block *nb, unsigned long event, void *buf)
{
	pr_err("%s: ++ current = %lld\n", __func__, ktime_get());
	mas_blk_panic_dump();
	pr_err("%s: --\n", __func__);
	return 0;
}

#ifdef CONFIG_MAS_UNISTORE_PRESERVE
void mas_blk_dump_unistore(
	struct request_queue *q, unsigned char *prefix)
{
	if (!blk_queue_query_unistore_enable(q))
		return;

	pr_err("%s - %s\n", __func__, prefix);
	mas_blk_panic_dump();
#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
	mas_blk_recovery_debug_off();
#endif
}
#endif

/*
 * This function is used to add the request queue into the dump list
 */
void mas_blk_dump_register_queue(struct request_queue *q)
{
	struct blk_dev_lld *blk_lld = mas_blk_get_lld(q);

	if (blk_lld->features & BLK_LLD_DUMP_SUPPORT) {
		if (!list_empty(&q->mas_queue.dump_list))
			return;
		spin_lock(&dump_list_lock);
		list_add_tail(&q->mas_queue.dump_list, &dump_list);
		spin_unlock(&dump_list_lock);
	}
}

/*
 * This function is used to remove the request queue from the dump list
 */
void mas_blk_dump_unregister_queue(struct request_queue *q)
{
	struct request_queue *q_node = NULL;
	struct blk_queue_cust *mas_queue = NULL;
	struct blk_queue_cust *mas_queue_backup = NULL;

	spin_lock(&dump_list_lock);
	list_for_each_entry_safe(
		mas_queue, mas_queue_backup, &dump_list, dump_list) {
		q_node = (struct request_queue *)container_of(
			mas_queue, struct request_queue, mas_queue);
		if (q_node == q) {
			list_del(&q_node->mas_queue.dump_list);
			break;
		}
	}
	spin_unlock(&dump_list_lock);
}

static struct notifier_block mas_blk_panic_nb = {
	.notifier_call = __cfi_mas_blk_panic_notify,
	.priority = 0,
};

void __init mas_blk_dump_init(void)
{
	atomic_notifier_chain_register(&panic_notifier_list,
		&mas_blk_panic_nb);
}

/*
 * This function is used to enable the dump function on MQ tagset
 */
void blk_mq_tagset_dump_register(
	struct blk_mq_tag_set *tag_set, lld_dump_status_fn func)
{
	struct blk_dev_lld *blk_lld = &tag_set->lld_func;

	blk_lld->dump_fn = func;
	blk_lld->features |= BLK_LLD_DUMP_SUPPORT;
}

/*
 * This function is used to enable the dump function on request queue
 */
void blk_queue_dump_register(struct request_queue *q, lld_dump_status_fn func)
{
	struct blk_dev_lld *blk_lld = mas_blk_get_lld(q);

	blk_lld->dump_fn = func;
	blk_lld->features |= BLK_LLD_DUMP_SUPPORT;
	mas_blk_dump_register_queue(q);
}
