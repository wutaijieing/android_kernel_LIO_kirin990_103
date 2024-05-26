/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
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

#include <linux/spinlock.h>
#include <linux/wait.h>
#include <securec.h>

#include "dpu_frame_buffer_mgr.h"
#include "../dpu_fb_def.h"
#include "dpu_iommu.h"

#define BUFFER_NUM 3
#define WAIT_BUF_AVAILABLE_TIMEOUT 300 /* ms */
#define WAIT_TIMES_NUM 50
#define TIME_EACH_WAIT 10 /* ms */
#define TIMELINE_OF_BUF_USED 2

struct buf_timeline {
	uint32_t value;
	bool is_using;
	struct dma_buf *dma_buf_handle[MAX_DSS_SRC_NUM];
	uint32_t layer_num;
};

static struct buf_timeline g_buf_tl[BUFFER_NUM];
static bool has_new_frame_flush;
static spinlock_t buf_lock;
static wait_queue_head_t wait_buf_available;


static void dpu_add_dma_buf_handle(struct disp_merger_mgr *merger_mgr,
	struct buf_timeline *buf_tl)
{
	uint32_t i;
	struct block_info * block_info = NULL;

	block_info = &(merger_mgr->rda_overlay.ov_block_infos[0]);
	for (i = 0; i < block_info->layer_nums; i++)
		buf_tl->dma_buf_handle[i] = block_info->layer_infos[i].buf;
	buf_tl->layer_num = block_info->layer_nums;
}

static void dpu_unlock_dma_buf_handle(struct buf_timeline *buf_tl)
{
	uint32_t i;

	for (i = 0; i < buf_tl->layer_num; i++) {
		if (buf_tl->dma_buf_handle[i] != NULL)
			dpu_put_dmabuf(buf_tl->dma_buf_handle[i]);
	}
}

void wait_buffer_available(struct disp_merger_mgr *merger_mgr)
{
	uint32_t buf_idx;
	struct timeval tv_start;
	int times = 0;
	unsigned long flags = 0;
	long ret;

	DPU_FB_DEBUG("[BUF_MGR] +\n");
	dpu_check_and_no_retval(unlikely(!merger_mgr), ERR, "merger_mgr is null\n");

	if (merger_mgr->rda_frame_no < BUFFER_NUM)
		return;

	buf_idx = merger_mgr->rda_frame_no % BUFFER_NUM;
	DPU_FB_DEBUG("[BUF_MGR] frame_no:%u wait next buf:%u\n", merger_mgr->rda_frame_no, buf_idx);

	dpu_trace_ts_begin(&tv_start);

	while (true) {
		ret = wait_event_interruptible_timeout(wait_buf_available, g_buf_tl[buf_idx].value >= TIMELINE_OF_BUF_USED,
			msecs_to_jiffies(WAIT_BUF_AVAILABLE_TIMEOUT));
		if ((ret == -ERESTARTSYS) && (times++ < WAIT_TIMES_NUM))
			mdelay(TIME_EACH_WAIT);
		else
			break;
	}

	dpu_unlock_dma_buf_handle(&(g_buf_tl[buf_idx]));
	spin_lock_irqsave(&buf_lock, flags);
	g_buf_tl[buf_idx].value = 0;
	g_buf_tl[buf_idx].is_using = false;
	spin_unlock_irqrestore(&buf_lock, flags);

	dpu_trace_ts_end(&tv_start, 20000, "wait_buffer_available");

	DPU_FB_DEBUG("[BUF_MGR] -\n");
}

void dpu_lock_buf_sync(struct disp_merger_mgr *merger_mgr)
{
	uint32_t buf_idx;
	unsigned long flags = 0;

	dpu_check_and_no_retval(unlikely(!merger_mgr), ERR, "merger_mgr is null\n");
	buf_idx = (merger_mgr->rda_frame_no - 1) % BUFFER_NUM;

	spin_lock_irqsave(&buf_lock, flags);
	g_buf_tl[buf_idx].value = 0;
	g_buf_tl[buf_idx].is_using = true;
	dpu_add_dma_buf_handle(merger_mgr, &(g_buf_tl[buf_idx]));
	has_new_frame_flush = true;
	DPU_FB_DEBUG("[BUF_MGR] frame_no:%u buf:%u tl:%u\n",
		merger_mgr->rda_frame_no, buf_idx, g_buf_tl[buf_idx].value);
	spin_unlock_irqrestore(&buf_lock, flags);
}

void dpu_unlock_buf_sync(void)
{
	uint32_t buf_idx;

	spin_lock(&buf_lock);

	if (has_new_frame_flush == false) {
		spin_unlock(&buf_lock);
		return;
	}

	for (buf_idx = 0; buf_idx < BUFFER_NUM; buf_idx++) {
		if (g_buf_tl[buf_idx].is_using == false)
			continue;
		g_buf_tl[buf_idx].value++;
		if (g_buf_tl[buf_idx].value >= 2)
			wake_up_interruptible_all(&wait_buf_available);
		DPU_FB_DEBUG("[BUF_MGR] buf:%u tl:%u\n", buf_idx, g_buf_tl[buf_idx].value);
	}

	has_new_frame_flush = false;
	spin_unlock(&buf_lock);
}

void dpu_init_buf_timeline(void)
{
	memset_s(g_buf_tl, sizeof(g_buf_tl), 0, sizeof(g_buf_tl));
}

void init_buf_sync_mgr(void)
{
	static bool is_inited = false;

	DPU_FB_INFO("[BUF_MGR] +\n");

	if (is_inited)
		return;

	dpu_init_buf_timeline();
	init_waitqueue_head(&wait_buf_available);
	spin_lock_init(&buf_lock);
	is_inited = true;

	DPU_FB_INFO("[BUF_MGR] -\n");
}
