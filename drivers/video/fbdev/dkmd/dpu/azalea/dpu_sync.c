/* Copyright (c) Huawei Technologies Co., Ltd. 2015-2021. All rights reserved.
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

#include "dpu_sync.h"
#include "dpu_fb.h"

#define DPU_SYNC_NAME_SIZE 64
#define DPU_SYNC_DRIVER_NAME "dpu"

#if defined(CONFIG_SYNC_FILE)
/*
 * to_dpu_fence - get dpu dss fence from fence base object
 * @fence: Pointer to fence base object
 */
static struct dpu_fence *to_dpu_fence(struct dma_fence *fence)
{
	return container_of(fence, struct dpu_fence, base);
}

/*
 * to_dpu_timeline - get dpu dss timeline from fence base object
 * @fence: Pointer to fence base object
 */
static struct dpu_timeline *to_dpu_timeline(struct dma_fence *fence)
{
	return container_of(fence->lock, struct dpu_timeline, lock);
}

/*
 * dpu_free_timeline - Free the given timeline object
 * @kref: Pointer to timeline kref object.
 */
static void dpu_free_timeline(struct kref *kref)
{
	struct dpu_timeline *tl =
		container_of(kref, struct dpu_timeline, kref);

	kfree(tl);
}

/*
 * dpu_put_timeline - Put the given timeline object
 * @tl: Pointer to timeline object.
 */
static void dpu_put_timeline(struct dpu_timeline *tl)
{
	if (!tl) {
		DPU_FB_ERR("invalid parameters\n");
		return;
	}

	kref_put(&tl->kref, dpu_free_timeline);
}

/*
 * dpu_get_timeline - Get the given timeline object
 * @tl: Pointer to timeline object.
 */
static void dpu_get_timeline(struct dpu_timeline *tl)
{
	if (!tl) {
		DPU_FB_ERR("invalid parameters\n");
		return;
	}

	kref_get(&tl->kref);
}

static const char *dpu_fence_get_driver_name(struct dma_fence *fence)
{
	return DPU_SYNC_DRIVER_NAME;
}

static const char *dpu_fence_get_timeline_name(struct dma_fence *fence)
{
	struct dpu_timeline *tl = to_dpu_timeline(fence);

	return tl->name;
}

static bool dpu_fence_enable_signaling(struct dma_fence *fence)
{
	return true;
}

static bool dpu_fence_signaled(struct dma_fence *fence)
{
	struct dpu_timeline *tl = to_dpu_timeline(fence);
	bool status;

	status = ((s32) (tl->value - fence->seqno)) >= 0;
	return status;
}

static void dpu_fence_release(struct dma_fence *fence)
{
	struct dpu_fence *f = to_dpu_fence(fence);
	struct dpu_timeline *tl = to_dpu_timeline(fence);
	unsigned long flags = 0;

	if (g_debug_fence_timeline)
		DPU_FB_INFO("release for fence %s\n", f->name);
	spin_lock_irqsave(&tl->list_lock, flags);
	if (!list_empty(&f->fence_list))
		list_del(&f->fence_list);
	spin_unlock_irqrestore(&tl->list_lock, flags);
	dpu_put_timeline(to_dpu_timeline(fence));
	kfree_rcu(f, base.rcu); /*lint !e514*/
}

static void dpu_fence_value_str(struct dma_fence *fence, char *str, int size)
{
	snprintf(str, size, "%u", fence->seqno);
}

static void dpu_fence_timeline_value_str(struct dma_fence *fence, char *str,
		int size)
{
	struct dpu_timeline *tl = to_dpu_timeline(fence);

	snprintf(str, size, "%u", tl->value);
}

static struct dma_fence_ops dpu_fence_ops = {
	.get_driver_name = dpu_fence_get_driver_name,
	.get_timeline_name = dpu_fence_get_timeline_name,
	.enable_signaling = dpu_fence_enable_signaling,
	.signaled = dpu_fence_signaled,
	.wait = dma_fence_default_wait,
	.release = dpu_fence_release,
	.fence_value_str = dpu_fence_value_str,
	.timeline_value_str = dpu_fence_timeline_value_str,
};

/*
 * dpu_create_timeline - Create timeline object with the given name
 * @name: Pointer to name character string.
 */
struct dpu_timeline *dpu_create_timeline(const char *name)
{
	struct dpu_timeline *tl = NULL;

	if (!name) {
		DPU_FB_ERR("invalid parameters\n");
		return NULL;
	}

	tl = kzalloc(sizeof(struct dpu_timeline), GFP_KERNEL);
	if (!tl)
		return NULL;

	kref_init(&tl->kref);
	snprintf(tl->name, sizeof(tl->name), "%s", name);
	spin_lock_init(&tl->lock);
	spin_lock_init(&tl->list_lock);
	tl->context = dma_fence_context_alloc(1);

	INIT_LIST_HEAD(&tl->fence_list_head);

	return tl;
}

/*
 * dpu_destroy_timeline - Destroy the given timeline object
 * @tl: Pointer to timeline object.
 */
void dpu_destroy_timeline(struct dpu_timeline *tl)
{
	dpu_put_timeline(tl);
}

/*
 * dpu_inc_timeline_locked - Increment timeline by given amount
 * @tl: Pointer to timeline object.
 * @increment: the amount to increase the timeline by.
 */
static int dpu_inc_timeline_locked(struct dpu_timeline *tl,
		int increment)
{
	struct dpu_fence *f = NULL;
	struct dpu_fence *next = NULL;
	s32 val;
	bool is_signaled = false;
	struct list_head local_list_head;
	unsigned long flags = 0;

	INIT_LIST_HEAD(&local_list_head);

	spin_lock(&tl->list_lock);
	if (list_empty(&tl->fence_list_head)) {
		DPU_FB_DEBUG("fence list is empty\n");
		tl->value += 1;
		spin_unlock(&tl->list_lock);
		return 0;
	}

	list_for_each_entry_safe(f, next, &tl->fence_list_head, fence_list)
		list_move(&f->fence_list, &local_list_head);
	spin_unlock(&tl->list_lock);

	spin_lock_irqsave(&tl->lock, flags);
	val = tl->next_value - tl->value;
	if (val < increment)
		DPU_FB_INFO("timeline->value didnot update, val:%d, inc:%d, tl->value:%d!\n",
			val, increment, tl->value);
	tl->value += increment;

	spin_unlock_irqrestore(&tl->lock, flags);

	list_for_each_entry_safe(f, next, &local_list_head, fence_list) {
		spin_lock_irqsave(&tl->lock, flags);
		is_signaled = dma_fence_is_signaled_locked(&f->base);
		spin_unlock_irqrestore(&tl->lock, flags);
		if (is_signaled) {
			if (g_debug_fence_timeline)
				DPU_FB_INFO("%s signaled\n", f->name);
			list_del_init(&f->fence_list);
			dma_fence_put(&f->base);
		} else {
			spin_lock(&tl->list_lock);
			list_move(&f->fence_list, &tl->fence_list_head);
			spin_unlock(&tl->list_lock);
		}
	}

	return 0;
}

/*
 * dpu_resync_timeline - Resync timeline to last committed value
 * @tl: Pointer to timeline object.
 */
void dpu_resync_timeline(struct dpu_timeline *tl)
{
	s32 val;

	if (!tl) {
		pr_err("invalid parameters\n");
		return;
	}

	val = tl->next_value - tl->value;
	if (val > 0) {
		if (g_debug_fence_timeline)
			DPU_FB_INFO("flush %s:%d TL(Nxt %d , Crnt %d)\n", tl->name, val,
				tl->next_value, tl->value);
		dpu_inc_timeline_locked(tl, val);
	}
}

/*
 * dpu_get_sync_fence - Create fence object from the given timeline
 * @tl: Pointer to timeline object
 * @timestamp: Pointer to timestamp of the returned fence. Null if not required.
 * Return: pointer fence created on give time line.
 */
struct dpu_fence *dpu_get_sync_fence(
		struct dpu_timeline *tl, const char *fence_name,
		u32 *timestamp, int value)
{
	struct dpu_fence *f = NULL;
	unsigned long flags = 0;

	if (!tl) {
		DPU_FB_ERR("invalid parameters\n");
		return NULL;
	}

	f = kzalloc(sizeof(struct dpu_fence), GFP_KERNEL);
	if (!f)
		return NULL;

	INIT_LIST_HEAD(&f->fence_list);
	spin_lock_irqsave(&tl->lock, flags);
	tl->next_value = (u32)value;
	dma_fence_init(&f->base, &dpu_fence_ops, &tl->lock, tl->context, value);
	dpu_get_timeline(tl);
	spin_unlock_irqrestore(&tl->lock, flags);

	spin_lock_irqsave(&tl->list_lock, flags);
	list_add_tail(&f->fence_list, &tl->fence_list_head);
	spin_unlock_irqrestore(&tl->list_lock, flags);
	snprintf(f->name, sizeof(f->name), "%s_%u", fence_name, value);

	if (timestamp)
		*timestamp = value;

	if (g_debug_fence_timeline)
		DPU_FB_INFO("fence created at val=%u tl->name=%s tl->value=%d tl->next_value=%d\n",
			value, tl->name, tl->value, tl->next_value);

	return f;
}

/*
 * dpu_inc_timeline - Increment timeline by given amount
 * @tl: Pointer to timeline object.
 * @increment: the amount to increase the timeline by.
 */
int dpu_inc_timeline(struct dpu_timeline *tl, int increment)
{
	int rc;

	if (!tl) {
		DPU_FB_ERR("invalid parameters\n");
		return -EINVAL;
	}

	rc = dpu_inc_timeline_locked(tl, increment);
	return rc;
}

/*
 * dpu_get_timeline_commit_ts - Return commit tick of given timeline
 * @tl: Pointer to timeline object.
 */
u32 dpu_get_timeline_commit_ts(struct dpu_timeline *tl)
{
	if (!tl) {
		DPU_FB_ERR("invalid parameters\n");
		return 0;
	}

	return tl->next_value;
}

/*
 * dpu_get_timeline_retire_ts - Return retire tick of given timeline
 * @tl: Pointer to timeline object.
 */
u32 dpu_get_timeline_retire_ts(struct dpu_timeline *tl)
{
	if (!tl) {
		DPU_FB_ERR("invalid parameters\n");
		return 0;
	}

	return tl->value;
}

/*
 * dpu_put_sync_fence - Destroy given fence object
 * @fence: Pointer to fence object.
 */
void dpu_put_sync_fence(struct dpu_fence *fence)
{
	if (!fence) {
		DPU_FB_ERR("invalid parameters\n");
		return;
	}
	dma_fence_put((struct dma_fence *) fence);
}

/*
 * dpu_wait_sync_fence - Wait until fence signal or timeout
 * @fence: Pointer to fence object.
 * @timeout: maximum wait time, in msec, for fence to signal.
 */
int dpu_wait_sync_fence(struct dpu_fence *fence,
		long timeout)
{
	struct dma_fence *input_fence = NULL;
	int rc;

	if (!fence) {
		DPU_FB_ERR("invalid parameters\n");
		return -EINVAL;
	}
	input_fence = (struct dma_fence *) fence;
	rc = dma_fence_wait_timeout(input_fence, false, msecs_to_jiffies(timeout));
	if (rc > 0) {
		if (g_debug_fence_timeline)
			DPU_FB_INFO(
				"drv:%s timeline:%s seqno:%d\n",
				input_fence->ops->get_driver_name(input_fence),
				input_fence->ops->get_timeline_name(input_fence),
				input_fence->seqno);
		rc = 0;
	} else if (rc == 0) {
		DPU_FB_ERR(
			"drv:%s timeline:%s seqno:%d.\n",
			input_fence->ops->get_driver_name(input_fence),
			input_fence->ops->get_timeline_name(input_fence),
			input_fence->seqno);
		rc = -ETIMEDOUT;
	}

	return rc;
}

/*
 * dpu_get_fd_sync_fence - Get fence object of given file descriptor
 * @fd: File description of fence object.
 */
struct dpu_fence *dpu_get_fd_sync_fence(int fd)
{
	return (struct dpu_fence *) sync_file_get_fence(fd);
}

/*
 * dpu_get_sync_fence_fd - Get file descriptor of given fence object
 * @fence: Pointer to fence object.
 * Return: File descriptor on success, or error code on error
 */
int dpu_get_sync_fence_fd(struct dpu_fence *fence)
{
	int fd;
	struct sync_file *sync_file = NULL;

	if (!fence) {
		DPU_FB_ERR("invalid parameters\n");
		return -EINVAL;
	}

	fd = get_unused_fd_flags(O_CLOEXEC);
	if (fd < 0) {
		DPU_FB_ERR("fail to get unused fd\n");
		return fd;
	}

	sync_file = sync_file_create((struct dma_fence *) fence);
	if (!sync_file) {
		put_unused_fd(fd);
		DPU_FB_ERR("failed to create sync file\n");
		return -ENOMEM;
	}

	fd_install(fd, sync_file->file);

	return fd;
}

/*
 * dpu_get_sync_fence_name - get given fence object name
 * @fence: Pointer to fence object.
 * Return: fence name
 */
const char *dpu_get_sync_fence_name(struct dpu_fence *fence)
{
	if (!fence) {
		DPU_FB_ERR("invalid parameters\n");
		return NULL;
	}

	return fence->name;
}

#endif

