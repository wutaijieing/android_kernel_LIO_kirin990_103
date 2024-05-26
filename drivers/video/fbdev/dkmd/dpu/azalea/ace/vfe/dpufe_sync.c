/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include <linux/fs.h>
#include <linux/file.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/sync_file.h>

#include "dpufe_sync.h"
#include "dpufe_dbg.h"

#define DPUFE_SYNC_DRIVER_NAME "dpu"

static struct dpufe_timeline *to_dpufe_timeline(struct dma_fence *fence)
{
	return container_of(fence->lock, struct dpufe_timeline, lock);
}

static struct dpufe_fence *to_dpufe_fence(struct dma_fence *fence)
{
	return container_of(fence, struct dpufe_fence, base);
}

static const char *dpufe_fence_get_driver_name(struct dma_fence *fence)
{
	return DPUFE_SYNC_DRIVER_NAME;
}

static const char *dpufe_fence_get_timeline_name(struct dma_fence *fence)
{
	struct dpufe_timeline *tl = to_dpufe_timeline(fence);

	return tl->name;
}

static bool dpufe_fence_enable_signaling(struct dma_fence *fence)
{
	return true;
}

static bool dpufe_fence_signaled(struct dma_fence *fence)
{
	struct dpufe_timeline *tl = to_dpufe_timeline(fence);

	return (((s32) (tl->value - fence->seqno)) >= 0);
}

static void dpufe_free_timeline(struct kref *kref)
{
	struct dpufe_timeline *tl = container_of(kref, struct dpufe_timeline, kref);

	kfree(tl);
}

static void dpufe_put_timeline(struct dpufe_timeline *tl)
{
	if (!tl) {
		DPUFE_ERR("invalid parameters\n");
		return;
	}

	kref_put(&tl->kref, dpufe_free_timeline);
}

static void dpufe_fence_release(struct dma_fence *fence)
{
	struct dpufe_fence *f = to_dpufe_fence(fence);
	struct dpufe_timeline *tl = to_dpufe_timeline(fence);
	unsigned long flags = 0;

	if (g_dpufe_debug_fence_timeline)
		DPUFE_INFO("release for fence %s\n", f->name);
	spin_lock_irqsave(&tl->list_lock, flags);
	if (!list_empty(&f->fence_list))
		list_del(&f->fence_list);
	spin_unlock_irqrestore(&tl->list_lock, flags);
	dpufe_put_timeline(to_dpufe_timeline(fence));
	kfree_rcu(f, base.rcu); /*lint !e514*/
}

static void dpufe_fence_value_str(struct dma_fence *fence, char *str, int size)
{
	(void)snprintf(str, size, "%u", fence->seqno);
}

static void dpufe_fence_timeline_value_str(struct dma_fence *fence, char *str,
		int size)
{
	struct dpufe_timeline *tl = to_dpufe_timeline(fence);

	(void)snprintf(str, size, "%u", tl->value);
}

static struct dma_fence_ops dpufe_fence_ops = {
	.get_driver_name = dpufe_fence_get_driver_name,
	.get_timeline_name = dpufe_fence_get_timeline_name,
	.enable_signaling = dpufe_fence_enable_signaling,
	.signaled = dpufe_fence_signaled,
	.wait = dma_fence_default_wait,
	.release = dpufe_fence_release,
	.fence_value_str = dpufe_fence_value_str,
	.timeline_value_str = dpufe_fence_timeline_value_str,
};

struct dpufe_timeline *dpufe_create_timeline(const char *name)
{
	struct dpufe_timeline *tl = NULL;

	if (!name) {
		DPUFE_ERR("invalid parameters\n");
		return NULL;
	}

	tl = kzalloc(sizeof(struct dpufe_timeline), GFP_KERNEL);
	if (!tl)
		return NULL;

	kref_init(&tl->kref);
	(void)snprintf(tl->name, sizeof(tl->name), "%s", name);
	spin_lock_init(&tl->lock);
	spin_lock_init(&tl->list_lock);
	tl->context = dma_fence_context_alloc(1);

	INIT_LIST_HEAD(&tl->fence_list_head);

	return tl;
}

static void dpufe_get_timeline(struct dpufe_timeline *tl)
{
	if (!tl) {
		DPUFE_ERR("invalid parameters\n");
		return;
	}

	kref_get(&tl->kref);
}

void dpufe_destroy_timeline(struct dpufe_timeline *tl)
{
	if (!tl) {
		DPUFE_ERR("invalid parameters\n");
		return;
	}

	kref_put(&tl->kref, dpufe_free_timeline);
}

static bool dpufe_update_fence_list(struct dpufe_timeline *tl, struct list_head *local_list_head)
{
	struct dpufe_fence *f = NULL;
	struct dpufe_fence *next = NULL;
	bool need_inc_timeline = false;

	spin_lock(&tl->list_lock);
	if (list_empty(&tl->fence_list_head)) {
		DPUFE_DEBUG("fence list is empty\n");
		tl->value += 1;
		spin_unlock(&tl->list_lock);
		return need_inc_timeline;
	}

	list_for_each_entry_safe(f, next, &tl->fence_list_head, fence_list)
		list_move(&f->fence_list, local_list_head);
	spin_unlock(&tl->list_lock);

	need_inc_timeline = true;

	return need_inc_timeline;
}

static void dpufe_update_tl_value(struct dpufe_timeline *tl, int increment, unsigned long flags)
{
	s32 val;

	spin_lock_irqsave(&tl->lock, flags);
	val = tl->next_value - tl->value;
	if (val < increment)
		DPUFE_INFO("timeline->value didnot update, val:%d, inc:%d, tl->value:%d!\n",
			val, increment, tl->value);
	tl->value += increment;

	spin_unlock_irqrestore(&tl->lock, flags);
}

static void dpufe_update_fence_signal(struct dpufe_timeline *tl, unsigned long flags, struct dpufe_fence *f)
{
	bool is_signaled = false;

	spin_lock_irqsave(&tl->lock, flags);
	is_signaled = dma_fence_is_signaled_locked(&f->base);
	spin_unlock_irqrestore(&tl->lock, flags);
	if (is_signaled) {
		if (g_dpufe_debug_fence_timeline)
			DPUFE_INFO("%s signaled\n", f->name);
		list_del_init(&f->fence_list);
		dma_fence_put(&f->base);
	} else {
		spin_lock(&tl->list_lock);
		list_move(&f->fence_list, &tl->fence_list_head);
		spin_unlock(&tl->list_lock);
	}
}

static int dpufe_inc_timeline_locked(struct dpufe_timeline *tl, int increment)
{
	struct dpufe_fence *f = NULL;
	struct dpufe_fence *next = NULL;
	struct list_head local_list_head;
	unsigned long flags = 0;
	bool ret = false;

	INIT_LIST_HEAD(&local_list_head);

	ret = dpufe_update_fence_list(tl, &local_list_head);
	if (!ret)
		return ret;

	dpufe_update_tl_value(tl, increment, flags);

	list_for_each_entry_safe(f, next, &local_list_head, fence_list) {
		dpufe_update_fence_signal(tl, flags, f);
	}

	return 0;
}

void dpufe_resync_timeline(struct dpufe_timeline *tl)
{
	s32 val;

	if (!tl) {
		DPUFE_ERR("invalid parameters\n");
		return;
	}

	val = tl->next_value - tl->value;
	if (val > 0) {
		if (g_dpufe_debug_fence_timeline)
			DPUFE_INFO("flush %s:%d TL(Nxt %d , Crnt %d)\n", tl->name, val,
				tl->next_value, tl->value);
		dpufe_inc_timeline_locked(tl, val);
	}
}

struct dpufe_fence *dpufe_get_sync_fence(
	struct dpufe_timeline *tl, const char *fence_name, u32 *timestamp, int value)
{
	struct dpufe_fence *f = NULL;
	unsigned long flags = 0;

	if (!tl) {
		DPUFE_ERR("invalid parameters\n");
		return NULL;
	}

	f = kzalloc(sizeof(struct dpufe_fence), GFP_KERNEL);
	if (!f)
		return NULL;

	INIT_LIST_HEAD(&f->fence_list);
	spin_lock_irqsave(&tl->lock, flags);
	tl->next_value = value;
	dma_fence_init(&f->base, &dpufe_fence_ops, &tl->lock, tl->context, value);
	dpufe_get_timeline(tl);
	spin_unlock_irqrestore(&tl->lock, flags);

	spin_lock_irqsave(&tl->list_lock, flags);
	list_add_tail(&f->fence_list, &tl->fence_list_head);
	spin_unlock_irqrestore(&tl->list_lock, flags);
	(void)snprintf(f->name, sizeof(f->name), "%s_%u", fence_name, value);

	if (timestamp)
		*timestamp = value;

	if (g_dpufe_debug_fence_timeline)
		DPUFE_INFO("fence created at val=%u tl->name=%s tl->value=%d tl->next_value=%d\n",
			value, tl->name, tl->value, tl->next_value);

	return f;
}

int dpufe_inc_timeline(struct dpufe_timeline *tl, int increment)
{
	int rc;

	if (!tl) {
		DPUFE_ERR("invalid parameters\n");
		return -EINVAL;
	}

	rc = dpufe_inc_timeline_locked(tl, increment);
	return rc;
}

int dpufe_wait_sync_fence(struct dpufe_fence *fence, long timeout)
{
	struct dma_fence *input_fence = NULL;
	int rc;

	if (!fence) {
		DPUFE_ERR("invalid parameters\n");
		return -EINVAL;
	}
	input_fence = (struct dma_fence *) fence;
	rc = dma_fence_wait_timeout(input_fence, false, msecs_to_jiffies(timeout));
	if (rc > 0) {
		if (g_dpufe_debug_fence_timeline)
			DPUFE_INFO(
				"drv:%s timeline:%s seqno:%d\n",
				input_fence->ops->get_driver_name(input_fence),
				input_fence->ops->get_timeline_name(input_fence),
				input_fence->seqno);
		rc = 0;
	} else if (rc == 0) {
		DPUFE_ERR(
			"drv:%s timeline:%s seqno:%d.\n",
			input_fence->ops->get_driver_name(input_fence),
			input_fence->ops->get_timeline_name(input_fence),
			input_fence->seqno);
		rc = -ETIMEDOUT;
	}

	return rc;
}

struct dpufe_fence *dpufe_get_fd_sync_fence(int fd)
{
	return (struct dpufe_fence *) sync_file_get_fence(fd);
}

int dpufe_get_sync_fence_fd(struct dpufe_fence *fence)
{
	int fd;
	struct sync_file *sync_file = NULL;

	if (!fence) {
		DPUFE_ERR("invalid parameters\n");
		return -EINVAL;
	}

	fd = get_unused_fd_flags(O_CLOEXEC);
	if (fd < 0) {
		DPUFE_ERR("fail to get unused fd\n");
		return fd;
	}

	sync_file = sync_file_create((struct dma_fence *) fence);
	if (!sync_file) {
		put_unused_fd(fd);
		DPUFE_ERR("failed to create sync file\n");
		return -ENOMEM;
	}

	fd_install(fd, sync_file->file);

	return fd;
}
