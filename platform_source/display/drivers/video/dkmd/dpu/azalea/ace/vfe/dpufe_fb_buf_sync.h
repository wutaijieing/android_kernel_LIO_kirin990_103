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

#ifndef DPUFE_FB_BUF_SYNC_H
#define DPUFE_FB_BUF_SYNC_H

#include "dpufe_ov_utils.h"
#include "dpufe_sync.h"
#include "dpufe.h"

struct dpufe_layerbuf {
	struct dma_buf *buffer_handle;
	struct list_head list_node;
	int timeline;

	struct dpufe_fence *fence;
	int32_t shared_fd;
	uint32_t frame_no;
	dss_mmbuf_t mmbuf;
	uint64_t vir_addr;
	int32_t chn_idx;
};

int dpufe_buf_sync_handle(struct dpufe_data_type *dfd, dss_overlay_t *pov_req);
int dpufe_layerbuf_lock(struct dpufe_data_type *dfd, dss_overlay_t *pov_req, struct list_head *plock_list);
void dpufe_layerbuf_unlock(struct dpufe_data_type *dfd, struct list_head *pfree_list);
void dpufe_layerbuf_lock_exception(struct dpufe_data_type *dfd, struct list_head *plock_list);
int dpufe_buf_sync_create_fence(struct dpufe_data_type *dfd, int32_t *release_fence_fd, int32_t *retire_fence_fd);
void dpufe_append_layer_list(struct dpufe_data_type *dfd, struct list_head *plock_list);
void dpufe_layerbuf_flush(struct dpufe_data_type *dfd, struct list_head *plock_list);
void dpufe_reset_all_buf_fence(struct dpufe_data_type *dfd);
void dpufe_resync_timeline_fence(struct dpufe_data_type *dfd);
void dpufe_buf_sync_close_fence(int32_t *release_fence, int32_t *retire_fence);
void dpufe_update_frame_refresh_state(struct dpufe_data_type *dfd);
void dpufe_buf_sync_signal(struct dpufe_data_type *dfd);
void dpufe_buf_sync_register(struct dpufe_data_type *dfd);
void dpufe_buf_sync_unregister(struct dpufe_data_type *dfd);
void dpufe_layerbuf_unlock(struct dpufe_data_type *dfd, struct list_head *pfree_list);

#endif
