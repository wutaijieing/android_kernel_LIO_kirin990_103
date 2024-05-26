/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/dma-buf.h>
#include <linux/types.h>

#include "hisi_layer_buf.h"

uint32_t g_disp_buf_sync_offset = 0;

static int hisi_layerbuf_lock_img(struct list_head *buf_list, struct layer_img *img)
{
	struct dma_buf *buf_handle = NULL;
	struct disp_layer_buf *layer_buf_node;

	buf_handle = dma_buf_get(img->shared_fd);
	if (IS_ERR_OR_NULL(buf_handle)) {
		disp_pr_err("get dma buf fail, shard_fd=%d", img->shared_fd);
		return -1;
	}

	layer_buf_node = (struct disp_layer_buf *)kzalloc(sizeof(*layer_buf_node), GFP_KERNEL);
	if (!layer_buf_node)
		return -1;

	layer_buf_node->shared_fd = img->shared_fd;
	layer_buf_node->buf_handle = buf_handle;

	/* TODO: init others */

	list_add_tail(&layer_buf_node->list_node, buf_list);
	return 0;
}

int hisi_layerbuf_lock_pre_buf(struct list_head *pre_buf_list, struct hisi_display_frame *frame)
{
	uint32_t i;
	struct layer_img *layer_img;
	// return; // ybr
	disp_pr_debug(" ++++ ");
	for (i = 0; i < frame->pre_pipeline_count; i++) {
		layer_img = &(frame->pre_pipelines[i].src_layer.img);

		if (layer_img->shared_fd < 0)
			continue;

		hisi_layerbuf_lock_img(pre_buf_list, layer_img);

		/* TODO: other debug info */
		disp_buf_pr_debug("lock shard_fd = %d", layer_img->shared_fd);
	}
	disp_pr_debug(" ---- ");
	return 0;
}

int hisi_layerbuf_lock_post_wb_buf(struct list_head *wb_buf_list, struct hisi_display_frame *frame)
{
	uint32_t i;
	struct layer_img *layer_img;

	/* TODO: SMMU tlb flush */

	for (i = 0; i < frame->post_offline_count; i++) {
		layer_img = &(frame->post_offline_pipelines[i].offline_info.wb_layer.dst);

		if (layer_img->shared_fd < 0)
			continue;

		hisi_layerbuf_lock_img(wb_buf_list, layer_img);

		/* TODO: other debug info */
		disp_buf_pr_info("lock wb layer shard_fd = %d", layer_img->shared_fd);
	}

	return 0;
}

void hisi_layerbuf_unlock_buf(struct list_head *buf_list)
{
	struct disp_layer_buf *_node_ = NULL;
	struct disp_layer_buf *layer_buf_node = NULL;
	// return; // ybr
	list_for_each_entry_safe(layer_buf_node, _node_, buf_list, list_node) {
		list_del(&layer_buf_node->list_node);

		/* TODO: other debug info */
		disp_buf_pr_debug("unlock shard_fd = %d", layer_buf_node->shared_fd);

		if (!layer_buf_node->buf_handle) {
			/* TODO: flush smmu */
			dma_buf_put(layer_buf_node->buf_handle);
			layer_buf_node->buf_handle = NULL;
		}

		kfree(layer_buf_node);
	}
}

static bool hisi_layerbuf_is_signaled(struct hisi_timeline_listener *listener, uint32_t tl_val)
{
	disp_pr_info("tl_val=%u, listener->pt_value = %u", tl_val, listener->pt_value);
	return tl_val > listener->pt_value;
}

static int hisi_layerbuf_handle_signal(struct hisi_timeline_listener *listener)
{
	struct disp_layer_buf *layer_buf = (struct disp_layer_buf *)listener->listener_data;
	disp_pr_info(" ++++ ");
	disp_buf_pr_info("unlock shard_fd = %d", layer_buf->shared_fd);

	dma_buf_put(layer_buf->buf_handle);
	return 0;
}

static void hisi_layerbuf_release(struct hisi_timeline_listener *listener)
{
	if (listener->listener_data) {
		kfree(listener->listener_data);
		listener->listener_data = NULL;
	}
}

/* TODO: implement those functions */
static struct hisi_timeline_listener_ops g_layer_buf_listener_ops = {
	.get_listener_name = NULL,
	.enable_signaling = NULL,
	.disable_signaling = NULL,
	.is_signaled = hisi_layerbuf_is_signaled,
	.handle_signal = hisi_layerbuf_handle_signal,
	.release = hisi_layerbuf_release,
};

void hisi_layerbuf_move_buf_to_tl(struct list_head *buf_list, struct hisi_disp_timeline *timeline)
{
	struct disp_layer_buf *_node_ = NULL;
	struct disp_layer_buf *layer_buf_node = NULL;
	struct hisi_timeline_listener *listener = NULL;

	list_for_each_entry_safe(layer_buf_node, _node_, buf_list, list_node) {
		list_del(&layer_buf_node->list_node);

		listener = hisi_timeline_alloc_listener(&g_layer_buf_listener_ops, layer_buf_node, timeline->pt_value + g_disp_buf_sync_offset + 1);
		if (!listener) {
			kfree(layer_buf_node);
			continue;
		}

		hisi_timeline_add_listener(timeline, listener);
	}

	list_del_init(buf_list);
}

