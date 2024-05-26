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

#include <linux/uaccess.h>
#include <linux/time.h>

#include "dpufe_dbg.h"
#include "dpufe_mmbuf_manager.h"
#include "dpufe_panel_def.h"
#include "dpufe_ov_utils.h"

struct dpufe_queue g_dpu_dbg_queue[DBG_Q_NUM];

int dpufe_overlay_get_ov_data_from_user(struct dpufe_data_type *dfd,
	dss_overlay_t *pov_req, const void __user *argp)
{
	dss_overlay_block_t *pov_h_block_infos = NULL;
	int ret;

	dpufe_check_and_return(!dfd, -EINVAL, "dfd is NULL!\n");
	dpufe_check_and_return(!pov_req, -EINVAL, "pov_req is NULL!\n");
	dpufe_check_and_return(!argp, -EINVAL, "user data is invalid!\n");

	pov_h_block_infos = dfd->ov_block_infos;

	ret = copy_from_user(pov_req, argp, sizeof(dss_overlay_t));
	if (ret) {
		DPUFE_ERR("fb%d, copy_from_user failed!\n", dfd->index);
		return -EINVAL;
	}

	pov_req->release_fence = -1;
	pov_req->retire_fence = -1;

	if ((pov_req->ov_block_nums <= 0) || (pov_req->ov_block_nums > DPU_OV_BLOCK_NUMS)) {
		DPUFE_ERR("fb%d, ov_block_nums(%d) is out of range!\n",
			dfd->index, pov_req->ov_block_nums);
		return -EINVAL;
	}

	if ((dss_overlay_block_t *)(uintptr_t)pov_req->ov_block_infos_ptr == NULL) {
		DPUFE_ERR("pov_req->ov_block_infos_ptr is NULL\n");
		return -EINVAL;
	}
	ret = copy_from_user(pov_h_block_infos, (dss_overlay_block_t *)(uintptr_t)pov_req->ov_block_infos_ptr,
		pov_req->ov_block_nums * sizeof(dss_overlay_block_t));
	if (ret) {
		DPUFE_ERR("fb%d, dss_overlay_block_t copy_from_user failed!\n", dfd->index);
		return -EINVAL;
	}

	pov_req->ov_block_infos_ptr = (uint64_t)(uintptr_t)pov_h_block_infos;

	return ret;
}

int dpufe_overlay_init(struct dpufe_data_type *dfd)
{
	int i;

	dpufe_check_and_return(dfd == NULL, -EINVAL, "dfd is NULL\n");

	dfd->frame_count = 0;
	dfd->online_play_count = 0;
	dfd->ref_cnt = 0;
	dfd->panel_power_on = false;

	dpufe_mmbuf_init(dfd);
	memset(&dfd->ov_req, 0, sizeof(dss_overlay_t));
	dfd->ov_req.release_fence = -1;
	dfd->ov_req.retire_fence = -1;
	memset(&dfd->ov_req_prev, 0, sizeof(dss_overlay_t));
	dfd->ov_req_prev.release_fence = -1;
	dfd->ov_req_prev.retire_fence = -1;
#if defined(CONFIG_ASYNCHRONOUS_PLAY)
	dfd->asynchronous_play_flag = 1;
	init_waitqueue_head(&dfd->asynchronous_play_wq);
#endif

	for (i = 0; i < DBG_Q_NUM; i++)
		dpufe_queue_init(&g_dpu_dbg_queue[i], DBG_QUEUE_CAP);

	init_disp_recorder(&dfd->disp_recorder, (void *)dfd);

	return 0;
}

void dpufe_dbg_queue_free(void)
{
	int i;

	for (i = 0; i < DBG_Q_NUM; i++)
		dpufe_queue_deinit(&g_dpu_dbg_queue[i]);
}

static void dump_dbg_queue(struct dpufe_queue *q, const char *queue_name)
{
	int i;
	ktime_t cur_ts;
	ktime_t pre_ts = 0;

	for (i = 0; i < q->capacity; i++) {
		cur_ts = q->buf[(i + q->header) % q->capacity];
		DPUFE_INFO("name[%s]:ts[%d]:%llu, diff:%llu\n", queue_name, i,
			ktime_to_us(cur_ts), i == 0 ? 0 : ktime_to_us(cur_ts) - ktime_to_us(pre_ts));
		pre_ts = cur_ts;
	}
}

void dump_dbg_queues(int index)
{
	const char *queue_names[DBG_Q_NUM] = {
		"dpu0_play",
		"dpu0_vsync",
		"dpu1_play",
		"dpu1_vsync"
	};
	if (index == PRIMARY_PANEL_IDX) {
		dump_dbg_queue(&g_dpu_dbg_queue[DBG_DPU0_PLAY], queue_names[DBG_DPU0_PLAY]);
		dump_dbg_queue(&g_dpu_dbg_queue[DBG_DPU0_VSYNC], queue_names[DBG_DPU0_VSYNC]);
	} else if (index == EXTERNAL_PANEL_IDX) {
		dump_dbg_queue(&g_dpu_dbg_queue[DBG_DPU1_PLAY], queue_names[DBG_DPU1_PLAY]);
		dump_dbg_queue(&g_dpu_dbg_queue[DBG_DPU1_VSYNC], queue_names[DBG_DPU1_VSYNC]);
	}
}

static void empty_dbg_queue(struct dpufe_queue *q)
{
	int i;

	for (i = 0; i < q->capacity; i++) {
		if (q->buf)
			q->buf[(i + q->header) % q->capacity] = NULL;
	}
}

void empty_dbg_queues(uint32_t index)
{
	if (index == PRIMARY_PANEL_IDX) {
		empty_dbg_queue(&g_dpu_dbg_queue[DBG_DPU0_PLAY]);
		empty_dbg_queue(&g_dpu_dbg_queue[DBG_DPU0_VSYNC]);
	} else if (index == EXTERNAL_PANEL_IDX) {
		empty_dbg_queue(&g_dpu_dbg_queue[DBG_DPU1_PLAY]);
		empty_dbg_queue(&g_dpu_dbg_queue[DBG_DPU1_VSYNC]);
	}
}
