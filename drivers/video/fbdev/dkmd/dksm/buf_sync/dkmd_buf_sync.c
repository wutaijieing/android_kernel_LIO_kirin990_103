/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "dkmd_log.h"

#include <linux/dma-buf.h>
#include <linux/module.h>

#include "dkmd_buf_sync.h"

static bool buf_sync_is_signaled(struct dkmd_timeline_listener *listener, uint64_t tl_val)
{
	bool ret = tl_val > listener->pt_value;

	if (g_debug_fence_timeline)
		dpu_pr_info("signal=%d tl_val=%llu, listener=0x%pK, listener->pt_value=%llu",
					ret, tl_val, listener, listener->pt_value);

	return ret;
}

static int32_t buf_sync_handle_signal(struct dkmd_timeline_listener *listener)
{
	struct dkmd_dma_buf *layer_buf = (struct dkmd_dma_buf *)listener->listener_data;

	if (layer_buf->share_fd < 0) {
		dpu_pr_warn("listener = 0x%pK, release share_fd = %d, buf_handle = 0x%pK",
					listener, layer_buf->share_fd, layer_buf->buf_handle);
		return -1;
	}
	if (g_debug_fence_timeline)
		dpu_pr_info("listener = 0x%pK, release share_fd = %d, buf_handle = 0x%pK",
			listener, layer_buf->share_fd, layer_buf->buf_handle);

	dma_buf_put(layer_buf->buf_handle);
	layer_buf->buf_handle = NULL;
	layer_buf->share_fd = -1;

	return 0;
}

static const char *buf_sync_handle_get_name(struct dkmd_timeline_listener *listener)
{
	struct dkmd_dma_buf *layer_buf = (struct dkmd_dma_buf *)listener->listener_data;

	return layer_buf->name;
}

static void buf_sync_handle_release(struct dkmd_timeline_listener *listener)
{
	if (listener->listener_data) {
		kfree(listener->listener_data);
		listener->listener_data = NULL;
	}
}

static struct dkmd_timeline_listener_ops g_layer_buf_listener_ops = {
	.get_listener_name = buf_sync_handle_get_name,
	.is_signaled = buf_sync_is_signaled,
	.handle_signal = buf_sync_handle_signal,
	.release = buf_sync_handle_release,
};

int32_t dkmd_buf_sync_lock_dma_buf(struct dkmd_timeline *timeline, int32_t share_fd)
{
	struct dma_buf *buf_handle = NULL;
	struct dkmd_timeline_listener *listener = NULL;
	struct dkmd_dma_buf *layer_dma_buf = NULL;

	dpu_assert(timeline == NULL);
	if (share_fd < 0)
		return 0;

	buf_handle = dma_buf_get(share_fd);
	if (IS_ERR_OR_NULL(buf_handle)) {
		dpu_pr_err("get dma buf fail, share_fd=%d", share_fd);
		return -1;
	}

	layer_dma_buf = (struct dkmd_dma_buf *)kmalloc(sizeof(*layer_dma_buf), GFP_KERNEL);
	if (!layer_dma_buf) {
		dpu_pr_err("layer_dma_buf alloc failed!");
		dma_buf_put(buf_handle);
		return -1;
	}
	layer_dma_buf->share_fd = share_fd;
	layer_dma_buf->buf_handle = buf_handle;
	snprintf(layer_dma_buf->name, DKMD_SYNC_NAME_SIZE, "buf_shared_fd_%u", share_fd);

	listener = dkmd_timeline_alloc_listener(&g_layer_buf_listener_ops, layer_dma_buf,
		dkmd_timeline_get_next_value(timeline) + 1);
	if (!listener) {
		dpu_pr_err("alloc layer buf listener fail, share_fd=%d", share_fd);
		dma_buf_put(buf_handle);
		kfree(layer_dma_buf);
		return -1;
	}
	dkmd_timeline_add_listener(timeline, listener);

	if (g_debug_fence_timeline)
		dpu_pr_info("create listener_node=0x%pK, %s, buf_handle=0x%pK", listener, layer_dma_buf->name, buf_handle);

	return 0;
}
EXPORT_SYMBOL(dkmd_buf_sync_lock_dma_buf);

MODULE_LICENSE("GPL");