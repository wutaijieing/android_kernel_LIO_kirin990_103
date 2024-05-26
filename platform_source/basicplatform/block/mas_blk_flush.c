/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2019. All rights reserved.
 * Description:  mas block flush reduce core
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
#include <linux/module.h>
#include <linux/bio.h>
#include <linux/delay.h>
#include <linux/blkdev.h>
#include <linux/gfp.h>
#include <linux/blk-mq.h>
#include <trace/events/block.h>
#include <platform_include/basicplatform/linux/powerkey_event.h>
#include <platform_include/basicplatform/linux/rdr_platform.h>

#include "blk.h"
#include "blk-mq.h"
#include "mas_blk_flush_interface.h"

#define MAS_BLK_ASYNC_FLUSH_DELAY_MS 20U

static LIST_HEAD(all_flush_q_list);
static DEFINE_SPINLOCK(all_flush_q_lock);
static struct workqueue_struct *blk_async_flush_workqueue;

static LIST_HEAD(all_object_list);
static DEFINE_SPINLOCK(all_object_lock);

void mas_blk_flush_work_fn(const struct work_struct *work)
{
	struct blk_queue_cust *member =
		container_of(work, struct blk_queue_cust, flush_work.work);
	struct request_queue *q =
		container_of(member, struct request_queue, mas_queue);
	struct block_device *bdev = bdget_disk(q->mas_queue.queue_disk, 0);

	if (bdev) {
		int ret = blkdev_get(bdev, FMODE_WRITE, NULL);
		if (ret) {
			pr_err("%s: blkdev_get fail!\n", __func__);
			return;
		}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
		ret = blkdev_issue_flush(bdev, GFP_KERNEL);
#else
		ret = blkdev_issue_flush(bdev, GFP_KERNEL, NULL);
#endif
		if (ret)
			pr_err("%s: blkdev_issue_flush fail!\n", __func__);
		blkdev_put(bdev, FMODE_WRITE);
	}
}

/*
 * This function is used to judge if the bio should be dispatch async or not
 */
bool mas_blk_flush_async_dispatch(
	struct request_queue *q, const struct bio *bio)
{
	struct blk_queue_cust *mas_queue = &q->mas_queue;
#ifdef CONFIG_MAS_BLK_FTRACE_ENABLE
	char log[MAS_IO_TRACE_LEN];
#endif
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	if (blk_queue_query_unistore_enable(q) &&
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
		(!(bio->mas_bi_opf & (REQ_VIP))) && (!bio->bi_iter.bi_size))
#else
		(!(bio->bi_opf & (REQ_VIP))) && (!bio->bi_iter.bi_size))
#endif
		return false;
#endif
	if (unlikely(!mas_queue->flush_optimize))
		return false;

	if (likely(!(bio->bi_opf & REQ_PREFLUSH) || bio->bi_iter.bi_size))
		goto out;

#ifdef CONFIG_MAS_BLK_FTRACE_ENABLE
	(void)snprintf(log, MAS_IO_TRACE_LEN, "flush fua: write_after_flush = %d ",
			atomic_read(&mas_queue->write_after_flush));
	trace_mas_io(__func__, log, MAS_IO_TRACE_LEN);
#endif

	if (bio->mas_bio.bi_async_flush) {
		cancel_delayed_work(&mas_queue->flush_work);
		queue_delayed_work(blk_async_flush_workqueue,
			&mas_queue->flush_work,
			msecs_to_jiffies(MAS_BLK_ASYNC_FLUSH_DELAY_MS));
#ifdef CONFIG_MAS_BLK_FTRACE_ENABLE
		trace_mas_io(__func__, "flush async", MAS_IO_TRACE_LEN);
#endif
		return true;
	}

	if (unlikely(!atomic_read(&mas_queue->write_after_flush))) {
#ifdef CONFIG_MAS_BLK_FTRACE_ENABLE
		trace_mas_io(__func__, "write_after_flush = 0, flush skip",
			MAS_IO_TRACE_LEN);
#endif
		return true;
	}
#ifdef CONFIG_MAS_BLK_FTRACE_ENABLE
	trace_mas_io(__func__, "flush fua issue", MAS_IO_TRACE_LEN);
#endif

out:
	if (bio_op(bio) == REQ_OP_WRITE)
		atomic_inc(&mas_queue->write_after_flush);

	return false;
}

void mas_blk_flush_update(const struct request *req, int error)
{
	if (likely(!error))
		atomic_set(&req->q->mas_queue.write_after_flush, 0);
}

/*
 * This function is used to send the emergency flush before system power down
 */
void blk_power_off_flush(int emergency)
{
	struct blk_queue_cust *mas_queue = NULL;
	struct request_queue *q = NULL;
	struct blk_dev_lld *blk_lld = NULL;
	unsigned long flag;

	pr_err("%s: emergency = %d\n", __func__, emergency);
	spin_lock_irqsave(&all_flush_q_lock, flag);
	list_for_each_entry(mas_queue, &all_flush_q_list, flush_queue_node) {
		struct gendisk *queue_disk = NULL;

		if (!mas_queue->blk_part_tbl_exist)
			continue;
		q = (struct request_queue *)container_of(
			mas_queue, struct request_queue, mas_queue);
		blk_lld = mas_blk_get_lld(q);
		queue_disk = q->mas_queue.queue_disk;
		if (blk_lld && blk_lld->flush_fn) {
			if (queue_disk)
				pr_err("%s: emergency flush on %s\n",
					__func__, queue_disk->disk_name);
			blk_lld->flush_fn(q);
			if (queue_disk)
				pr_err("%s: emergency flush on %s done!\n",
					__func__, queue_disk->disk_name);
		}
	}
	spin_unlock_irqrestore(&all_flush_q_lock, flag);
	pr_err("%s: done!\n", __func__);
}

void mas_blk_flush_list_register(struct list_head *lld_list)
{
	unsigned long flag;

	spin_lock_irqsave(&all_object_lock, flag);
	list_add(lld_list, &all_object_list);
	spin_unlock_irqrestore(&all_object_lock, flag);
}

void mas_blk_flush_list_unregister(struct list_head *lld_list)
{
	unsigned long flag;

	spin_lock_irqsave(&all_object_lock, flag);
	list_del_init(lld_list);
	spin_unlock_irqrestore(&all_object_lock, flag);
}

void mas_blk_order_panic_wait_flush(void)
{
	unsigned long flags;
	unsigned int sleep_count = 200;
	bool need_flush = false;
	struct blk_dev_lld *blk_lld = NULL;

	pr_err("<%s> start\n", __func__);
	while (sleep_count) {
		mdelay(1);
		spin_lock_irqsave(&all_object_lock, flags);
		list_for_each_entry(blk_lld, &all_object_list, lld_list) {
			if (blk_lld->type != BLK_LLD_TAGSET_BASE) {
				pr_err("<%s> type = %d\n",
					__func__, blk_lld->type);
				continue;
			}

			if (!(blk_lld->features & BLK_LLD_UFS_ORDER_EN))
				continue;
			need_flush = need_flush ||
				ufs_order_panic_wait_datasync_handle(blk_lld);
		}
		spin_unlock_irqrestore(&all_object_lock, flags);
		if (need_flush) {
			blk_power_off_flush(1);
			pr_err("<%s> normal end\n", __func__);
			return;
		}
		sleep_count--;
	}
	blk_power_off_flush(1);
	pr_err("<%s> timeout end\n", __func__);
}

void mas_blk_panic_flush(void)
{
	unsigned long flags;
	struct blk_dev_lld *blk_lld = NULL;
	bool order_enable = false;

	blk_power_off_flush(0);

	spin_lock_irqsave(&all_object_lock, flags);
	list_for_each_entry(blk_lld, &all_object_list, lld_list) {
		if (blk_lld->type != BLK_LLD_TAGSET_BASE) {
			pr_err("<%s> type = %d\n",
				__func__, blk_lld->type);
			continue;
		}

		if (blk_lld->features & BLK_LLD_UFS_ORDER_EN) {
			order_enable = true;
			ufs_order_panic_datasync_handle(blk_lld);
		}
	}
	spin_unlock_irqrestore(&all_object_lock, flags);

	if (order_enable)
		mas_blk_order_panic_wait_flush();
}

static void mas_blk_order_poweroff_proc(void)
{
	unsigned long flags;
	struct blk_dev_lld *blk_lld = NULL;
	struct blk_mq_tag_set *tag_set = NULL;

	spin_lock_irqsave(&all_object_lock, flags);
	list_for_each_entry(blk_lld, &all_object_list, lld_list) {
		if (blk_lld->type != BLK_LLD_TAGSET_BASE) {
			pr_err("<%s> type = %d\n",
				__func__, blk_lld->type);
			continue;
		}

		if (!(blk_lld->features & BLK_LLD_UFS_ORDER_EN))
			continue;
		tag_set = (struct blk_mq_tag_set *)(blk_lld->data);
		if (tag_set && tag_set->mas_tagset_ops &&
				tag_set->mas_tagset_ops->tagset_power_off_proc_fn)
			tag_set->mas_tagset_ops->tagset_power_off_proc_fn(blk_lld);
	}
	spin_unlock_irqrestore(&all_object_lock, flags);
}

int mas_blk_poweroff_flush_notifier_call(
	const struct notifier_block *powerkey_nb, unsigned long event,
	const void *data)
{
	if (event == PRESS_KEY_6S)
		blk_power_off_flush(1);
	else if (event == PRESS_KEY_1S)
		mas_blk_order_poweroff_proc();

	return 0;
}

static struct notifier_block mas_blk_poweroff_flush_nb = {
	.notifier_call = __cfi_mas_blk_poweroff_flush_notifier_call,
	.priority = 0,
};

int __init mas_blk_flush_init(void)
{
	return powerkey_register_notifier(&mas_blk_poweroff_flush_nb);
}

module_init(__cfi_mas_blk_flush_init);

static void mas_blk_flush_reduced_queue_register(struct request_queue *q)
{
	unsigned long flag;

	spin_lock_irqsave(&all_flush_q_lock, flag);
	list_add(&q->mas_queue.flush_queue_node, &all_flush_q_list);
	spin_unlock_irqrestore(&all_flush_q_lock, flag);
}

void mas_blk_flush_reduced_queue_unregister(struct request_queue *q)
{
	unsigned long flag;
	struct blk_queue_cust *mas_queue = &q->mas_queue;
	struct list_head *flush_queue_node = &mas_queue->flush_queue_node;

	if (mas_queue->flush_optimize) {
		if (!flush_queue_node->next || !flush_queue_node->prev) {
			mas_blk_rdr_panic("Invalid flush_queue_node!");
			return;
		}
		spin_lock_irqsave(&all_flush_q_lock, flag);
		list_del(flush_queue_node);
		spin_unlock_irqrestore(&all_flush_q_lock, flag);
	}
}

/*
 * This function is used to enable the register the emergency flush interface to
 * the MQ tagset
 */
void blk_mq_tagset_direct_flush_register(
	struct blk_mq_tag_set *tag_set, blk_direct_flush_fn func)
{
	if (!tag_set) {
		pr_err("%s: tag_set is NULL!\n", __func__);
		return;
	}

	tag_set->lld_func.flush_fn = func;
}

void mas_blk_queue_async_flush_init(struct request_queue *q)
{
	struct blk_dev_lld *blk_lld = mas_blk_get_lld(q);
	struct blk_queue_cust *mas_queue = &q->mas_queue;
	struct list_head *flush_queue_node = &mas_queue->flush_queue_node;

	if (!(blk_lld->features & BLK_LLD_FLUSH_REDUCE_SUPPORT))
		return;

	if (!blk_async_flush_workqueue)
		blk_async_flush_workqueue =
			alloc_workqueue("async_flush", WQ_UNBOUND, 0);

	if (!blk_async_flush_workqueue) {
		pr_err("%s: Failed to alloc workqueue!\n", __func__);
		return;
	}

	atomic_set(&mas_queue->write_after_flush, 0);
	if (!flush_queue_node->next && !flush_queue_node->prev)
		INIT_LIST_HEAD(flush_queue_node);

	INIT_DELAYED_WORK(&mas_queue->flush_work,
		__cfi_mas_blk_flush_work_fn);
	if (blk_lld->flush_fn && list_empty(flush_queue_node))
		mas_blk_flush_reduced_queue_register(q);

	mas_queue->flush_optimize = 1;
}

/*
 * This function is used to allow the flush to dispatch async
 */
void blk_flush_set_async(struct bio *bio)
{
	if (bio)
		bio->mas_bio.bi_async_flush = 1;
}

#ifdef CONFIG_ZODIAC_MMC
/*
 * This function is used to enable the register the emergency flush interface to
 * the request queue
 */
void blk_queue_direct_flush_register(
	struct request_queue *q, blk_direct_flush_fn func)
{
	struct blk_dev_lld *blk_lld = NULL;
	struct list_head *flush_queue_node = NULL;

	if (!q) {
		pr_err("%s: q is NULL!\n", __func__);
		return;
	}

	blk_lld = mas_blk_get_lld(q);
	flush_queue_node = &q->mas_queue.flush_queue_node;
	if (!flush_queue_node->next && !flush_queue_node->prev)
		INIT_LIST_HEAD(flush_queue_node);

	if (func) {
		blk_lld->flush_fn = func;
		mas_blk_flush_reduced_queue_register(q);
	} else {
		blk_lld->flush_fn = NULL;
		mas_blk_flush_reduced_queue_unregister(q);
	}
}

/*
 * This function is used to enable the flush reduce on the request queue
 */
void blk_queue_flush_reduce_config(
	struct request_queue *q, bool flush_reduce_enable)
{
	struct blk_dev_lld *blk_lld = NULL;

	if (!q) {
		pr_err("%s: q is NULL!\n", __func__);
		return;
	}

	blk_lld = mas_blk_get_lld(q);
	if (flush_reduce_enable)
		blk_lld->features |= BLK_LLD_FLUSH_REDUCE_SUPPORT;
	else
		blk_lld->features &= ~BLK_LLD_FLUSH_REDUCE_SUPPORT;

	mas_blk_queue_async_flush_init(q);
}
#endif /* CONFIG_ZODIAC_MMC */

/*
 * This function is used to enable the flush reduce on the MQ tagset
 */
void blk_mq_tagset_flush_reduce_config(
	struct blk_mq_tag_set *tag_set, bool flush_reduce_enable)
{
	if (!tag_set) {
		pr_err("%s: tag_set is NULL!\n", __func__);
		return;
	}

	if (flush_reduce_enable)
		tag_set->lld_func.features |= BLK_LLD_FLUSH_REDUCE_SUPPORT;
	else
		tag_set->lld_func.features &= ~BLK_LLD_FLUSH_REDUCE_SUPPORT;
}

/*
 * This function is for FS module to aware flush reduce function
 */
int blk_flush_async_support(const struct block_device *bi_bdev)
{
	struct request_queue *q = NULL;

	if (!bi_bdev) {
		pr_err("%s: bi_bdev is NULL!\n", __func__);
		return 0;
	}

	q = bdev_get_queue((struct block_device *)bi_bdev);
	if (q)
		return q->mas_queue.flush_optimize;
	else
		return 0;
}
