/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2019. All rights reserved.
 * Description: mas block busy idle framework
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
#include <linux/blkdev.h>
#include <linux/gfp.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <trace/events/block.h>
#include <linux/types.h>

#include "blk.h"
#include "mas_blk_busy_idle_interface.h"
#include "mas_blk_busy_idle_notify.h"

#define BLK_IO_IDLE_AVOID_JITTER_TIME 5
#define BUSYIDLE_HANDLE_MAX_MS 1000U
#define BUSYIDLE_WARNING_MS 10ULL

static struct workqueue_struct *blk_idle_notify_workqueue;

#ifdef CONFIG_MAS_UNISTORE_PRESERVE
static struct workqueue_struct *blk_fg_io_idle_notify_workqueue;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
void mas_blk_busyidle_handler_latency_check_timer_expire
	(struct timer_list *timer)
{
	struct blk_busyidle_nb *nb = (struct blk_busyidle_nb *)from_timer(nb,
		timer, handler_dur_check_timer);

	pr_err("%s: %s process time is more than %u ms\n", __func__,
		nb->event_node.subscriber, BUSYIDLE_HANDLE_MAX_MS);
}
#else
void mas_blk_busyidle_handler_latency_check_timer_expire(unsigned long data)
{
	struct blk_busyidle_nb *nb = (struct blk_busyidle_nb *)(uintptr_t)data;

	pr_err("%s: %s process time is more than %u ms\n", __func__,
		nb->event_node.subscriber, BUSYIDLE_HANDLE_MAX_MS);
}
#endif

#ifdef CONFIG_MAS_UNISTORE_PRESERVE
static enum blk_busyidle_callback_ret handle_fg_idle_event(
	const struct blk_busyidle_event_node *node, struct blk_busyidle_nb *nb,
	int *handler_result)
{
	enum blk_busyidle_callback_ret ret = BUSYIDLE_NO_IO;
	struct blk_dev_lld *lld = node->lld;

	if ((lld->features & BLK_LLD_UFS_UNISTORE_EN) &&
			(nb->last_fg_io_state == BLK_FG_IO_BUSY)) {
		ret = node->busyidle_callback(
			(struct blk_busyidle_event_node *)node,
			BLK_FG_IDLE_NOTIFY);
		nb->last_fg_io_state = BLK_FG_IO_IDLE;
		if (ret == BUSYIDLE_IO_TRIGGERED)
			*handler_result = NOTIFY_STOP;
	}

	return ret;
}

static enum blk_busyidle_callback_ret handle_fg_busy_event(
	const struct blk_busyidle_event_node *node, struct blk_busyidle_nb *nb)
{
	enum blk_busyidle_callback_ret ret = BUSYIDLE_NO_IO;
	struct blk_dev_lld *lld = node->lld;

	if ((lld->features & BLK_LLD_UFS_UNISTORE_EN) &&
			(nb->last_fg_io_state == BLK_FG_IO_IDLE)) {
		ret = node->busyidle_callback(
			(struct blk_busyidle_event_node *)node,
			BLK_FG_BUSY_NOTIFY);
		nb->last_fg_io_state = BLK_FG_IO_BUSY;
	}

	return ret;
}
#endif

static enum blk_busyidle_callback_ret handle_busyidle_event(
	unsigned long event, struct blk_busyidle_nb *nb,
	const struct blk_busyidle_event_node *node, int *handler_result)
{
	enum blk_busyidle_callback_ret ret = BUSYIDLE_NO_IO;

	switch (event) {
	case BLK_IDLE_NOTIFY:
		if (likely(nb->last_state == BLK_IO_BUSY)) {
			ret = node->busyidle_callback(
				(struct blk_busyidle_event_node *)node,
				BLK_IDLE_NOTIFY);
			nb->last_state = BLK_IO_IDLE;
			if (ret == BUSYIDLE_IO_TRIGGERED)
				*handler_result = NOTIFY_STOP;
		}
		break;
	case BLK_BUSY_NOTIFY:
		if (likely(nb->last_state == BLK_IO_IDLE)) {
			ret = node->busyidle_callback(
				(struct blk_busyidle_event_node *)node,
				BLK_BUSY_NOTIFY);
			nb->last_state = BLK_IO_BUSY;
		}
		break;
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	case BLK_FG_IDLE_NOTIFY:
		ret = handle_fg_idle_event(node, nb, handler_result);
		break;
	case BLK_FG_BUSY_NOTIFY:
		ret = handle_fg_busy_event(node, nb);
		break;
#endif
	default:
		mas_blk_rdr_panic("Unknown busy idle event!");
		break;
	}

	return ret;
}

int mas_blk_busyidle_notify_handler(
	const struct notifier_block *nb, unsigned long event, const void *v)
{
	int handler_result = NOTIFY_DONE;
	ktime_t start_ktime;
	struct blk_busyidle_nb *busyidle_nb =
		container_of(nb, struct blk_busyidle_nb, busyidle_nb);
	struct blk_busyidle_event_node *node = &(busyidle_nb->event_node);

	if (unlikely(!node->busyidle_callback)) {
		pr_err("%s: %s NULL callback\n", __func__, node->subscriber);
#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
		mas_blk_rdr_panic(NO_EXTRA_MSG);
#else
		return NOTIFY_DONE;
#endif
	}

	start_ktime = ktime_get();
	mod_timer(&busyidle_nb->handler_dur_check_timer,
		jiffies + msecs_to_jiffies(BUSYIDLE_HANDLE_MAX_MS));

	if (handle_busyidle_event(event, busyidle_nb, node, &handler_result) ==
		BUSYIDLE_ERR) {
		pr_err("%s: %s error!\n", __func__, node->subscriber);
#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
		dump_stack();
#endif
	}

	del_timer_sync(&busyidle_nb->handler_dur_check_timer);
	if (unlikely(ktime_after(ktime_get(),
		    ktime_add_ms(start_ktime, BUSYIDLE_WARNING_MS))))
		pr_err("%s: %s busy idle callback cost %lld ms\n", __func__,
			node->subscriber,
			ktime_to_ms(ktime_sub(ktime_get(), start_ktime)));

	return handler_result;
}

static struct blk_busyidle_nb *busyidle_find_subscribed_node(
	const struct blk_idle_state *blk_idle,
	const struct blk_busyidle_event_node *node)
{
	struct blk_busyidle_nb *p = NULL;

	list_for_each_entry(p, &blk_idle->subscribed_list, subscribed_node)
		if (!strcmp(node->subscriber, p->event_node.subscriber)
			&& node->busyidle_callback ==
			p->event_node.busyidle_callback)
			return p;

	return NULL;
}

static bool is_valid_event_node(
	const struct blk_busyidle_event_node *event_node)
{
	if (!event_node->busyidle_callback) {
		pr_emerg("%s: notifier_callback is NULL\n", __func__);
		return false;
	}

	if (strlen(event_node->subscriber) < 1 ||
		strlen(event_node->subscriber) >= SUBSCRIBER_NAME_LEN) {
		pr_emerg("%s: Invalid subscriber\n", __func__);
		return false;
	}

	return true;
}

static void setup_busy_idle_notify_nb(struct blk_busyidle_nb *nb)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
	timer_setup(&nb->handler_dur_check_timer,
	__cfi_mas_blk_busyidle_handler_latency_check_timer_expire, 0);
#else
	setup_timer(&nb->handler_dur_check_timer,
		__cfi_mas_blk_busyidle_handler_latency_check_timer_expire,
		(uintptr_t)nb);
#endif
	nb->busyidle_nb.notifier_call = __cfi_mas_blk_busyidle_notify_handler;
	nb->busyidle_nb.priority = 0;
	nb->last_state = BLK_IO_IDLE;
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	nb->last_fg_io_state = BLK_FG_IO_IDLE;
#endif
}

static int mas_blk_busyidle_event_register(
	const struct blk_dev_lld *lld,
	const struct blk_busyidle_event_node *event_node)
{
#if !defined(CONFIG_MAS_DEBUG_FS) && !defined(CONFIG_MAS_BLK_DEBUG)
	int ret = 0;
#endif
	struct blk_busyidle_nb *nb = NULL;
	struct blk_idle_state *blk_idle =
		(struct blk_idle_state *)&lld->blk_idle;

	nb = kzalloc(sizeof(struct blk_busyidle_nb), GFP_KERNEL);
	if (!nb)
		return -ENOMEM;

	memcpy(&nb->event_node, event_node, sizeof(nb->event_node));
	if (!is_valid_event_node(&nb->event_node)) {
#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
		mas_blk_rdr_panic("Invalid event node!");
#else
		dump_stack();
		ret = -EINVAL;
		goto free_notify_nb;
#endif
	}

	mutex_lock(&blk_idle->io_count_mutex);
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	if (lld->features & BLK_LLD_UFS_UNISTORE_EN)
		mutex_lock(&blk_idle->fg_io_count_mutex);
#endif
	if (busyidle_find_subscribed_node(blk_idle, &nb->event_node)) {
		pr_err("%s: %s has been registered already!\n",
			__func__, nb->event_node.subscriber);
#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
		mas_blk_rdr_panic(NO_EXTRA_MSG);
#else
		dump_stack();
		ret = -EPERM;
		goto unlock;
#endif
	}

	setup_busy_idle_notify_nb(nb);
	list_add_tail(&nb->subscribed_node, &blk_idle->subscribed_list);
	blocking_notifier_chain_register(&blk_idle->nh, &nb->busyidle_nb);
	pr_err("%s registered to busy idle\n", nb->event_node.subscriber);

#if !defined(CONFIG_MAS_DEBUG_FS) && !defined(CONFIG_MAS_BLK_DEBUG)
unlock:
#endif
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	if (lld->features & BLK_LLD_UFS_UNISTORE_EN)
		mutex_unlock(&blk_idle->fg_io_count_mutex);
#endif
	mutex_unlock(&blk_idle->io_count_mutex);
#if !defined(CONFIG_MAS_DEBUG_FS) && !defined(CONFIG_MAS_BLK_DEBUG)
free_notify_nb:
	if (ret && nb)
		kfree(nb);
	return ret;
#else
	return 0;
#endif
}

static int mas_blk_busyidle_event_unregister(
	const struct blk_busyidle_event_node *event_node)
{
	int ret = 0;
	struct blk_busyidle_nb *nb = NULL;
	struct blk_dev_lld *lld = event_node->lld;
	struct blk_idle_state *blk_idle = &lld->blk_idle;

	mutex_lock(&blk_idle->io_count_mutex);
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	if (lld->features & BLK_LLD_UFS_UNISTORE_EN)
		mutex_lock(&blk_idle->fg_io_count_mutex);
#endif
	nb = busyidle_find_subscribed_node(blk_idle, event_node);
	if (!nb) {
		pr_err("Trying to unregister an unregistered event_node!\n");
		dump_stack();
		ret = -EINVAL;
		goto out;
	}

	blocking_notifier_chain_unregister(&blk_idle->nh, &nb->busyidle_nb);
	pr_err("%s unregistered from busy idle module 0x%pK\n",
		nb->event_node.subscriber,
		&(nb->busyidle_nb));
	list_del_init(&nb->subscribed_node);
	kfree(nb);

out:
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	if (lld->features & BLK_LLD_UFS_UNISTORE_EN)
		mutex_unlock(&blk_idle->fg_io_count_mutex);
#endif
	mutex_unlock(&blk_idle->io_count_mutex);
	return ret;
}

static void blk_busy_to_idle(struct blk_idle_state *blk_idle)
{
	blk_idle->idle_state = BLK_IO_IDLE;
#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
	blk_idle->total_idle_count++;
	blk_idle->last_idle_ktime = ktime_get();
	if (blk_idle->last_busy_ktime)
		blk_idle->total_busy_ktime =
			ktime_add(blk_idle->total_busy_ktime,
				ktime_sub(blk_idle->last_idle_ktime,
					blk_idle->last_busy_ktime));
#endif
	blocking_notifier_call_chain(&blk_idle->nh,
		(unsigned long)BLK_IDLE_NOTIFY, NULL);
}

#ifdef CONFIG_MAS_UNISTORE_PRESERVE
static void mas_blk_fg_io_idle_notify_work(struct work_struct *work)
{
	struct blk_idle_state *blk_idle = (struct blk_idle_state *)container_of(
		(struct delayed_work *)work, struct blk_idle_state,
		fg_io_idle_notify_worker);

	struct blk_dev_lld *blk_lld =
		container_of(blk_idle, struct blk_dev_lld, blk_idle);
	struct request_queue *q = mas_blk_get_queue_by_lld(blk_lld);

	mutex_lock(&blk_idle->fg_io_count_mutex);
	if ((atomic_read(&blk_idle->fg_io_count) == 0) &&
		(blk_idle->fg_io_idle_state != BLK_FG_IO_IDLE)) {
		blk_idle->fg_io_idle_state = BLK_FG_IO_IDLE;
		blocking_notifier_call_chain(&blk_idle->nh,
			(unsigned long)BLK_FG_IDLE_NOTIFY, (void *)q);
	}
	mutex_unlock(&blk_idle->fg_io_count_mutex);
}

void blk_lld_fg_io_idle_notify(struct blk_dev_lld *lld)
{
	if (likely(lld->init_magic == BLK_LLD_INIT_MAGIC))
		queue_delayed_work(blk_fg_io_idle_notify_workqueue,
			&lld->blk_idle.fg_io_idle_notify_worker,
			msecs_to_jiffies(lld->blk_idle.fg_io_idle_notify_delay_ms));
}

static void blk_fg_io_idle_count(
	struct request_queue *q, struct blk_dev_lld *lld)
{
	if (unlikely(!(lld->features & BLK_LLD_BUSYIDLE_SUPPORT)))
		return;

	if (unlikely(!(lld->features & BLK_LLD_UFS_UNISTORE_EN)))
		return;

	if (unlikely(atomic_read(&lld->blk_idle.fg_io_count) == 0)) {
		pr_err("<%s> io_count has been zero\n", __func__);
#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
		rdr_syserr_process_for_ap(
			(u32)MODID_AP_S_PANIC_STORAGE, 0ull, 0ull);
#else
		return;
#endif
	}
	if (unlikely(atomic_dec_return(&lld->blk_idle.fg_io_count) == 0))
		blk_lld_fg_io_idle_notify(lld);
}

static void blk_fg_io_busy_count(
	struct request_queue *q, struct blk_dev_lld *lld)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0))
	bool unlock = false;
#endif
	struct blk_idle_state *blk_idle = &lld->blk_idle;

	if (unlikely(!(lld->features & BLK_LLD_BUSYIDLE_SUPPORT)))
		return;

	if (unlikely(!(lld->features & BLK_LLD_UFS_UNISTORE_EN)))
		return;

	if (likely(atomic_read(&blk_idle->fg_io_count)))
		goto inc_fg_iocount;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0))
	if (preempt_count())
		unlock = mas_blk_lld_unlock(lld);
#endif

	mutex_lock(&blk_idle->fg_io_count_mutex);
	cancel_delayed_work(&blk_idle->fg_io_idle_notify_worker);
	if (blk_idle->fg_io_idle_state == BLK_FG_IO_IDLE) {
		blk_idle->fg_io_idle_state = BLK_FG_IO_BUSY;
		blocking_notifier_call_chain(
			&blk_idle->nh, BLK_FG_BUSY_NOTIFY, (void *)q);
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0))
	if (unlock)
		mas_blk_lld_lock(lld);
#endif

	mutex_unlock(&blk_idle->fg_io_count_mutex);
inc_fg_iocount:
	atomic_inc(&blk_idle->fg_io_count);
}
#endif /* CONFIG_MAS_UNISTORE_PRESERVE */

void mas_blk_idle_notify_work(const struct work_struct *work)
{
	struct blk_idle_state *blk_idle = (struct blk_idle_state *)container_of(
		(struct delayed_work *)work, struct blk_idle_state,
		idle_notify_worker);

	if (blk_idle->idle_intr_support)
		atomic_set(&blk_idle->io_count, 0);

	mutex_lock(&blk_idle->io_count_mutex);
	if (!atomic_read(&blk_idle->io_count) &&
		(blk_idle->idle_state != BLK_IO_IDLE))
		blk_busy_to_idle(blk_idle);
	mutex_unlock(&blk_idle->io_count_mutex);
}

void blk_lld_idle_notify(const struct blk_dev_lld *lld)
{
	if (likely(lld->init_magic == BLK_LLD_INIT_MAGIC))
		queue_delayed_work(blk_idle_notify_workqueue,
			(struct delayed_work *)&lld->blk_idle
				.idle_notify_worker,
			msecs_to_jiffies(lld->blk_idle.idle_notify_delay_ms));
}

#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
static void blk_busyidle_update_dur(struct blk_idle_state *blk_idle)
{
	s64 blk_idle_dur_ms;

	if (!blk_idle->last_idle_ktime || !blk_idle->last_busy_ktime)
		return;

	blk_idle_dur_ms = ktime_to_ms(blk_idle->last_busy_ktime) -
			  ktime_to_ms(blk_idle->last_idle_ktime);
	if (blk_idle_dur_ms > blk_idle->max_idle_dur)
		blk_idle->max_idle_dur = blk_idle_dur_ms;

	if (blk_idle_dur_ms < BLK_IDLE_100MS)
		blk_idle->blk_idle_dur[BLK_IDLE_DUR_IDX_100MS]++;
	else if (blk_idle_dur_ms < BLK_IDLE_500MS)
		blk_idle->blk_idle_dur[BLK_IDLE_DUR_IDX_500MS]++;
	else if (blk_idle_dur_ms < BLK_IDLE_1000MS)
		blk_idle->blk_idle_dur[BLK_IDLE_DUR_IDX_1000MS]++;
	else if (blk_idle_dur_ms < BLK_IDLE_2000MS)
		blk_idle->blk_idle_dur[BLK_IDLE_DUR_IDX_2000MS]++;
	else if (blk_idle_dur_ms < BLK_IDLE_4000MS)
		blk_idle->blk_idle_dur[BLK_IDLE_DUR_IDX_4000MS]++;
	else if (blk_idle_dur_ms < BLK_IDLE_6000MS)
		blk_idle->blk_idle_dur[BLK_IDLE_DUR_IDX_6000MS]++;
	else if (blk_idle_dur_ms < BLK_IDLE_8000MS)
		blk_idle->blk_idle_dur[BLK_IDLE_DUR_IDX_8000MS]++;
	else if (blk_idle_dur_ms < BLK_IDLE_10000MS)
		blk_idle->blk_idle_dur[BLK_IDLE_DUR_IDX_10000MS]++;
	else
		blk_idle->blk_idle_dur[BLK_IDLE_DUR_IDX_FOR_AGES]++;
}

ssize_t mas_queue_busyidle_enable_store(
	const struct request_queue *q, const char *page, size_t count)
{
	ssize_t ret;
	unsigned long val;

	ret = queue_var_store(&val, page, count);
	if ((ret >= 0) && val) {
		pr_err("%s: busy idle enable\n", __func__);
		blk_queue_busyidle_enable(q, 1);
	} else {
		pr_err("%s: busy idle disable\n", __func__);
		blk_queue_busyidle_enable(q, 0);
	}
	return ret;
}

ssize_t mas_queue_busyidle_statistic_reset_store(
	const struct request_queue *q, const char *page, size_t count)
{
	ssize_t ret;
	unsigned long val;
	struct blk_dev_lld *lld = mas_blk_get_lld((struct request_queue *)q);

	ret = queue_var_store(&val, page, count);
	if ((ret >= 0) && val) {
		pr_err("%s: io busy idle statistic result reset!\n", __func__);
		lld->blk_idle.total_busy_ktime =
			lld->blk_idle.total_idle_ktime = ktime_set(0, 0);
		lld->blk_idle.total_idle_count = 0ULL;
		memset(lld->blk_idle.blk_idle_dur, 0,
			sizeof(lld->blk_idle.blk_idle_dur));
		lld->blk_idle.max_idle_dur = 0;
	}
	return ret;
}

/* this is for debug purpose and page is long enough for now */
ssize_t mas_queue_busyidle_statistic_show(
	const struct request_queue *q, char *page, unsigned long len)
{
	unsigned long offset = 0;
	struct blk_dev_lld *lld = mas_blk_get_lld((struct request_queue *)q);
	struct blk_idle_state *blk_idle = &(lld->blk_idle);

	offset += snprintf(page, len,
		"Total Busy Time: %lld ms  Total Idle Time: %lld ms\r\n",
		ktime_to_ms(lld->blk_idle.total_busy_ktime),
		ktime_to_ms(lld->blk_idle.total_idle_ktime));
	offset += snprintf(page + offset, (size_t)(len - offset),
		"Total Idle Count: %llu \r\n", lld->blk_idle.total_idle_count);
	offset += snprintf(page + offset, (size_t)(len - offset),
		"block idle interval statistic:\n");
	offset += snprintf(page + offset, (size_t)(len - offset),
		"max_idle_duration: %lldms\n", blk_idle->max_idle_dur);
	offset += snprintf(page + offset, (size_t)(len - offset),
		"less than 100ms: %lld\n",
		blk_idle->blk_idle_dur[BLK_IDLE_DUR_IDX_100MS]);
	offset += snprintf(page + offset, (size_t)(len - offset),
		"less than 500ms: %lld\n",
		blk_idle->blk_idle_dur[BLK_IDLE_DUR_IDX_500MS]);
	offset += snprintf(page + offset, (size_t)(len - offset),
		"less than 1s: %lld\n",
		blk_idle->blk_idle_dur[BLK_IDLE_DUR_IDX_1000MS]);
	offset += snprintf(page + offset, (size_t)(len - offset),
		"less than 2s: %lld\n",
		blk_idle->blk_idle_dur[BLK_IDLE_DUR_IDX_2000MS]);
	offset += snprintf(page + offset, (size_t)(len - offset),
		"less than 4s: %lld\n",
		blk_idle->blk_idle_dur[BLK_IDLE_DUR_IDX_4000MS]);
	offset += snprintf(page + offset, (size_t)(len - offset),
		"less than 6s: %lld\n",
		blk_idle->blk_idle_dur[BLK_IDLE_DUR_IDX_6000MS]);
	offset += snprintf(page + offset, (size_t)(len - offset),
		"less than 8s: %lld\n",
		blk_idle->blk_idle_dur[BLK_IDLE_DUR_IDX_8000MS]);
	offset += snprintf(page + offset, (size_t)(len - offset),
		"less than 10s: %lld\n",
		blk_idle->blk_idle_dur[BLK_IDLE_DUR_IDX_10000MS]);
	offset += snprintf(page + offset, (size_t)(len - offset),
		"more than 10s: %lld\n",
		blk_idle->blk_idle_dur[BLK_IDLE_DUR_IDX_FOR_AGES]);
	offset += snprintf(page + offset, (size_t)(len - offset),
		"idle total counts: %llu\n", blk_idle->total_idle_count);
	return (ssize_t)offset;
}

ssize_t mas_queue_hw_idle_enable_show(
	const struct request_queue *q, char *page, unsigned long len)
{
	struct blk_dev_lld *lld = mas_blk_get_lld((struct request_queue *)q);

	return snprintf(page, len, "%s\n",
		lld->blk_idle.idle_intr_support ? "enabled" : "disabled");
}

ssize_t mas_queue_idle_state_show(
	const struct request_queue *q, char *page, unsigned long len)
{
	struct blk_dev_lld *lld = mas_blk_get_lld((struct request_queue *)q);
	struct blk_idle_state *blk_idle = &(lld->blk_idle);
	ssize_t n = 0;

	n += snprintf(page, (size_t)len, "idle_state: %s\n",
		(blk_idle->idle_state == BLK_IO_IDLE) ? "idle" : "busy");
	n += snprintf(page + n, (size_t)(len - n), "io_count: %d\n",
		atomic_read(&blk_idle->io_count));
	return n;
}
#endif /* CONFIG_MAS_DEBUG_FS */

static void blk_idle_count(struct blk_dev_lld *lld)
{
	struct blk_idle_state *blk_idle = &lld->blk_idle;

	if (unlikely(!(lld->features & BLK_LLD_BUSYIDLE_SUPPORT)))
		return;

	if (blk_idle->idle_intr_support)
		return;
	if (unlikely(!atomic_read(&lld->blk_idle.io_count))) {
		pr_err("%s: io_count has been zero\n", __func__);
#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
		mas_blk_rdr_panic(NO_EXTRA_MSG);
#else
		return;
#endif
	}
	if (unlikely(!atomic_dec_return(&lld->blk_idle.io_count)))
		blk_lld_idle_notify(lld);
}

static void blk_idle_to_busy(struct blk_idle_state *blk_idle)
{
	blk_idle->idle_state = BLK_IO_BUSY;
#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
	blk_idle->last_busy_ktime = ktime_get();
	blk_busyidle_update_dur(blk_idle);
	if (likely(blk_idle->last_idle_ktime))
		blk_idle->total_idle_ktime =
			ktime_add(blk_idle->total_idle_ktime,
				ktime_sub(blk_idle->last_busy_ktime,
					blk_idle->last_idle_ktime));
#endif

	blocking_notifier_call_chain(&blk_idle->nh, BLK_BUSY_NOTIFY, NULL);
}

static void blk_busy_count(struct blk_dev_lld *lld)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0))
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	bool unlock = false;
#endif
#endif
	struct blk_idle_state *blk_idle = &lld->blk_idle;

	if (unlikely(!(lld->features & BLK_LLD_BUSYIDLE_SUPPORT)))
		return;

	if (likely(atomic_read(&blk_idle->io_count)))
		goto inc_iocount;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0))
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	if (preempt_count())
		unlock = mas_blk_lld_unlock(lld);
#endif
#endif
	mutex_lock(&blk_idle->io_count_mutex);
	cancel_delayed_work(&blk_idle->idle_notify_worker);

	/*
	 * Avoid twice busy event
	 * (The idle work may be canceled)
	 */
	if (blk_idle->idle_state == BLK_IO_IDLE)
		blk_idle_to_busy(blk_idle);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0))
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	if (unlock)
		mas_blk_lld_lock(lld);
#endif
#endif
	mutex_unlock(&blk_idle->io_count_mutex);

inc_iocount:
	atomic_inc(&blk_idle->io_count);
}

static void mas_blk_add_bio_to_counted_list(
	struct blk_dev_lld *lld, struct bio *bio)
{
	unsigned long flags;

	spin_lock_irqsave(&lld->blk_idle.counted_list_lock, flags);
	lld->blk_idle.bio_count++;
	list_add_tail(&bio->cnt_list, &lld->blk_idle.bio_list);
	spin_unlock_irqrestore(&lld->blk_idle.counted_list_lock, flags);
}

static void mas_blk_remove_bio_from_counted_list(
	struct blk_dev_lld *lld, struct bio *bio)
{
	unsigned long flags;

	if (unlikely(!lld->blk_idle.bio_count))
#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
		mas_blk_rdr_panic("bio_count is zero!");
#else
		return;
#endif

	spin_lock_irqsave(&lld->blk_idle.counted_list_lock, flags);
	lld->blk_idle.bio_count--;
	list_del_init(&bio->cnt_list);
	spin_unlock_irqrestore(&lld->blk_idle.counted_list_lock, flags);
}

static void mas_blk_add_rq_to_counted_list(
	struct blk_dev_lld *lld, struct request *rq)
{
	unsigned long flags;

	spin_lock_irqsave(&lld->blk_idle.counted_list_lock, flags);
	lld->blk_idle.req_count++;
	list_add_tail(&rq->cnt_list, &lld->blk_idle.req_list);
	spin_unlock_irqrestore(&lld->blk_idle.counted_list_lock, flags);
}

static void mas_blk_remove_rq_from_counted_list(
	struct blk_dev_lld *lld, struct request *rq)
{
	unsigned long flags;

	if (unlikely(!lld->blk_idle.req_count))
#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
		mas_blk_rdr_panic("req_count is zero!");
#else
		return;
#endif

	spin_lock_irqsave(&lld->blk_idle.counted_list_lock, flags);
	lld->blk_idle.req_count--;
	list_del_init(&rq->cnt_list);
	spin_unlock_irqrestore(&lld->blk_idle.counted_list_lock, flags);
}

#ifdef CONFIG_MAS_UNISTORE_PRESERVE
static void mas_blk_fg_io_check_busy(
	struct request_queue *q, struct bio *bio, struct blk_dev_lld *lld)
{
	if ((lld->features & BLK_LLD_UFS_UNISTORE_EN) && bio->mas_bio.fg_io &&
			(!(bio->mas_bio.io_in_count & MAS_IO_IN_FG_COUNT_SET))) {
		bio->mas_bio.io_in_count |= MAS_IO_IN_FG_COUNT_SET;
		blk_fg_io_busy_count(q, lld);
	}
}
static void mas_blk_fg_io_check_idle(
	struct request_queue *q, struct bio *bio, struct blk_dev_lld *lld)
{
	if ((lld->features & BLK_LLD_UFS_UNISTORE_EN) && bio->mas_bio.fg_io &&
			(bio->mas_bio.io_in_count & MAS_IO_IN_FG_COUNT_SET)) {
		bio->mas_bio.io_in_count &= ~MAS_IO_IN_FG_COUNT_SET;
		blk_fg_io_idle_count(q, lld);
	}
}
#endif

/*
 * This function is used to get the io count when a bio is submitted to block
 * layer
 */
void mas_blk_busyidle_check_bio(const struct request_queue *q, struct bio *bio)
{
	struct blk_dev_lld *lld = mas_blk_get_lld((struct request_queue *)q);

	bio->mas_bio.q = (struct request_queue *)q;

	if (unlikely(bio->mas_bio.io_in_count & MAS_IO_IN_COUNT_SET))
		return;
	bio->mas_bio.io_in_count |= MAS_IO_IN_COUNT_SET;
	mas_blk_add_bio_to_counted_list(lld, bio);
	blk_busy_count(lld);
#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	mas_blk_fg_io_check_busy((struct request_queue *)q, bio, lld);
#endif
}

/*
 * This function is used to get the io count when a request is inserted directly
 */
bool mas_blk_busyidle_check_request_bio(
	const struct request_queue *q, const struct request *rq)
{
	struct bio *bio = rq->bio;

	if (bio) {
		do {
			mas_blk_busyidle_check_bio(q, bio);
			bio = bio->bi_next;
		} while (bio);
		return true;
	}
	return false;
}

void mas_blk_busyidle_end_rq(const struct request *rq, blk_status_t error)
{
	struct blk_dev_lld *lld = mas_blk_get_lld(rq->q);

	mas_blk_remove_rq_from_counted_list(lld, (struct request *)rq);
	blk_idle_count(lld);

	if (rq->mas_req.uplayer_end_io)
		rq->mas_req.uplayer_end_io((struct request *)rq, error);
}

/*
 * This function is used to relase the io count when non-fs request end
 */
void mas_blk_busyidle_check_execute_request(
	const struct request_queue *q, struct request *rq,
	rq_end_io_fn *done)
{
	if (!mas_blk_busyidle_check_request_bio(q, rq)) {
		struct blk_dev_lld *lld = mas_blk_get_lld(rq->q);

		mas_blk_add_rq_to_counted_list(lld, rq);
		blk_busy_count(lld);
		rq->end_io = __cfi_mas_blk_busyidle_end_rq;
		rq->mas_req.uplayer_end_io = done;
	} else {
		rq->end_io = done;
	}
}

/*
 * This function is used to relase the io count when non-fs request end
 */
void mas_blk_busyidle_check_bio_endio(struct bio *bio)
{
	struct request_queue *q =
		bio->bi_disk ? bio->bi_disk->queue : bio->mas_bio.q;
	struct blk_dev_lld *lld = mas_blk_get_lld(q);

#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	mas_blk_fg_io_check_idle(q, bio, lld);
#endif
	if (unlikely(!(bio->mas_bio.io_in_count & MAS_IO_IN_COUNT_SET)))
		return;

	mas_blk_remove_bio_from_counted_list(lld, bio);

	bio->mas_bio.io_in_count = 0;

	blk_idle_count(lld);
}

/*
 * This function is to subscriber busy idle event from the block device
 */
int blk_busyidle_event_subscribe(
	const struct block_device *bdev,
	const struct blk_busyidle_event_node *event_node)
{
	if (!bdev || !event_node)
		return -EINVAL;

	return blk_queue_busyidle_event_subscribe(
		bdev_get_queue((struct block_device *)bdev), event_node);
}

/*
 * This function is to subscriber busy idle event from the request queue
 */
int blk_queue_busyidle_event_subscribe(
	const struct request_queue *q,
	const struct blk_busyidle_event_node *event_node)
{
	if (!q || !event_node)
		return -EINVAL;

	return blk_lld_busyidle_event_subscribe(
		mas_blk_get_lld((struct request_queue *)q),
		(struct blk_busyidle_event_node *)event_node);
}

/*
 * This function is to subscriber busy idle event from the lld object
 */
int blk_lld_busyidle_event_subscribe(
	const struct blk_dev_lld *lld,
	struct blk_busyidle_event_node *event_node)
{
	if (!lld || !event_node)
		return -EINVAL;

	event_node->lld = (struct blk_dev_lld *)lld;
	return mas_blk_busyidle_event_register(lld, event_node);
}

/*
 * This function is to unsubscriber busy idle event from the block device
 */
int blk_busyidle_event_unsubscribe(
	const struct blk_busyidle_event_node *event_node)
{
	if (!event_node)
		return -EINVAL;

	return mas_blk_busyidle_event_unregister(event_node);
}

/*
 * This function is to unsubscriber busy idle event from the request queue
 */
int blk_queue_busyidle_event_unsubscribe(
	const struct blk_busyidle_event_node *event_node)
{
	if (!event_node)
		return -EINVAL;

	return mas_blk_busyidle_event_unregister(event_node);
}

/*
 * This function is to enable the busy idle on the request queue
 */
void blk_queue_busyidle_enable(const struct request_queue *q, int enable)
{
	struct blk_dev_lld *blk_lld = NULL;

	if (!q)
		return;

	blk_lld = mas_blk_get_lld((struct request_queue *)q);

	if (enable)
		blk_lld->features |= BLK_LLD_BUSYIDLE_SUPPORT;
	else
		blk_lld->features &= ~BLK_LLD_BUSYIDLE_SUPPORT;
}

/*
 * This function is to enable the busy idle on the MQ tagset
 */
void blk_mq_tagset_busyidle_enable(struct blk_mq_tag_set *tag_set, int enable)
{
	struct blk_dev_lld *blk_lld = NULL;

	if (!tag_set)
		return;

	blk_lld = &tag_set->lld_func;

	if (enable)
		blk_lld->features |= BLK_LLD_BUSYIDLE_SUPPORT;
	else
		blk_lld->features &= ~BLK_LLD_BUSYIDLE_SUPPORT;
}

/*
 * This function is to enable the hardware idle notify on the MQ tagset
 */
void blk_mq_tagset_hw_idle_notify_enable(
	struct blk_mq_tag_set *tag_set, int enable)
{
	struct blk_dev_lld *blk_lld = NULL;

	if (!tag_set)
		return;

	blk_lld = &tag_set->lld_func;
	blk_lld->blk_idle.idle_intr_support = !!enable;
}

#ifdef CONFIG_MAS_UNISTORE_PRESERVE
static int mas_blk_alloc_fg_io_workqueue(void)
{
	if (!blk_fg_io_idle_notify_workqueue)
		blk_fg_io_idle_notify_workqueue = alloc_workqueue(
			"fg_busy_idle_notify", WQ_MEM_RECLAIM | WQ_HIGHPRI, 0);

	if (!blk_fg_io_idle_notify_workqueue) {
		pr_err("%s: Failed to alloc workqueue!\n", __func__);
		return -ENOMEM;
	}

	return 0;
}

static void mas_blk_fg_io_busyidle_state_init(
	struct blk_idle_state *blk_idle)
{
	blk_idle->fg_io_idle_notify_delay_ms = BLK_IO_IDLE_AVOID_JITTER_TIME;
	INIT_DELAYED_WORK(
		&blk_idle->fg_io_idle_notify_worker, mas_blk_fg_io_idle_notify_work);
	atomic_set(&blk_idle->fg_io_count, 0);
	mutex_init(&blk_idle->fg_io_count_mutex);
	blk_idle->fg_io_idle_state = BLK_FG_IO_IDLE;
}
#endif

int mas_blk_busyidle_state_init(struct blk_idle_state *blk_idle)
{
	if (!blk_idle_notify_workqueue)
		blk_idle_notify_workqueue = alloc_workqueue(
			"busyidle_notify", WQ_MEM_RECLAIM | WQ_HIGHPRI, 0);

	if (!blk_idle_notify_workqueue) {
		pr_err("%s: Failed to alloc workqueue!\n", __func__);
		return -ENOMEM;
	}

#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	if (mas_blk_alloc_fg_io_workqueue())
		return -ENOMEM;
#endif

#if defined(CONFIG_MAS_DEBUG_FS) || defined(CONFIG_MAS_BLK_DEBUG)
	blk_idle->total_busy_ktime = ktime_set(0, 0);
	blk_idle->total_idle_ktime = ktime_set(0, 0);
	blk_idle->last_busy_ktime = ktime_set(0, 0);
	blk_idle->last_idle_ktime = ktime_set(0, 0);
	memset(blk_idle->blk_idle_dur, 0, sizeof(blk_idle->blk_idle_dur));
	blk_idle->max_idle_dur = 0;
	blk_idle->total_idle_count = 0ULL;
#endif

	blk_idle->idle_notify_delay_ms = BLK_IO_IDLE_AVOID_JITTER_TIME;
	INIT_DELAYED_WORK(
		&blk_idle->idle_notify_worker, __cfi_mas_blk_idle_notify_work);
	atomic_set(&blk_idle->io_count, 0);
	mutex_init(&blk_idle->io_count_mutex);
	INIT_LIST_HEAD(&blk_idle->subscribed_list);
	BLOCKING_INIT_NOTIFIER_HEAD(&blk_idle->nh);
	blk_idle->idle_state = BLK_IO_IDLE;

#ifdef CONFIG_MAS_UNISTORE_PRESERVE
	mas_blk_fg_io_busyidle_state_init(blk_idle);
#endif
	blk_idle->bio_count = 0;
	blk_idle->req_count = 0;
	INIT_LIST_HEAD(&blk_idle->bio_list);
	INIT_LIST_HEAD(&blk_idle->req_list);
	spin_lock_init(&blk_idle->counted_list_lock);

	return 0;
}
