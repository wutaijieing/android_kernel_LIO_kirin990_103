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

#include <linux/delay.h>
#include <linux/list.h>
#include <linux/slab.h>
#include "res_mgr.h"
#include "dkmd_object.h"
#include "dkmd_dpu.h"
#include "dvfs.h"

#define DVFS_DESCENDING_COUNT 2
#define PERF_INIT_VALUE 0xFFFFFFFF
struct dpu_dvfs *g_dvfs_mgr = NULL;

static bool need_vote_freq(uint32_t perf_level)
{
	/* for first vote */
	if (unlikely(g_dvfs_mgr->voted_level == PERF_INIT_VALUE))
		return true;

	dpu_pr_debug("perf_level=%u voted_level=%u delay_cnt=%u is_perf_falling=%d",
		perf_level, g_dvfs_mgr->voted_level, g_dvfs_mgr->delay_cnt, g_dvfs_mgr->is_perf_falling);
	if (perf_level > g_dvfs_mgr->voted_level) {
		g_dvfs_mgr->delay_cnt = DVFS_DESCENDING_COUNT;
		g_dvfs_mgr->is_perf_falling = false;
		return true;
	}

	if (perf_level == g_dvfs_mgr->voted_level) {
		if (g_dvfs_mgr->is_perf_falling)
			--g_dvfs_mgr->delay_cnt;
	} else {
		g_dvfs_mgr->is_perf_falling = true;
		--g_dvfs_mgr->delay_cnt;
	}

	if (g_dvfs_mgr->delay_cnt == 0) {
		g_dvfs_mgr->delay_cnt = DVFS_DESCENDING_COUNT;
		g_dvfs_mgr->is_perf_falling = false;
		return true;
	}

	return false;
}

int dpu_dvfs_intra_frame_vote(struct intra_frame_dvfs_info *info)
{
	if (unlikely(g_dvfs_mgr == NULL))
		return 0;

	if (!is_dpu_dvfs_enable())
		return 0;

	down(&g_dvfs_mgr->sem);

	if (need_vote_freq(info->perf_level)) {
		g_dvfs_mgr->voted_level = info->perf_level;
		dpu_legacy_inter_frame_dvfs_vote(info->perf_level);
	}

	up(&g_dvfs_mgr->sem);

	return 0;
}

void dpu_dvfs_reset_dvfs_info(void)
{
	if (unlikely(g_dvfs_mgr == NULL))
		return;

	if (!is_dpu_dvfs_enable())
		return;

	down(&g_dvfs_mgr->sem);

	g_dvfs_mgr->voted_level = PERF_INIT_VALUE;
	g_dvfs_mgr->delay_cnt = DVFS_DESCENDING_COUNT;
	g_dvfs_mgr->is_perf_falling = false;

	up(&g_dvfs_mgr->sem);
}

static void* dvfs_init(struct dpu_res_data *rm_data)
{
	struct dpu_dvfs *dvfs_mgr = NULL;

	dpu_check_and_return(!rm_data, NULL, err, "res_data is nullptr!");
	dvfs_mgr = (struct dpu_dvfs *)kzalloc(sizeof(*dvfs_mgr), GFP_KERNEL);
	dpu_check_and_return(!dvfs_mgr, NULL, err, "dvfs_mgr alloc failed!");

	sema_init(&dvfs_mgr->sem, 1);
	dvfs_mgr->voted_level = PERF_INIT_VALUE;
	dvfs_mgr->delay_cnt = DVFS_DESCENDING_COUNT;
	dvfs_mgr->is_perf_falling = false;
	g_dvfs_mgr = dvfs_mgr;

	return dvfs_mgr;
}

static void dvfs_deinit(void *dvfs)
{
	struct dpu_dvfs *dvfs_mgr = (struct dpu_dvfs *)dvfs;

	dpu_check_and_no_retval(!dvfs_mgr, err, "dvfs_mgr is nullptr!");
	kfree(dvfs_mgr);
	g_dvfs_mgr = NULL;
	dvfs = NULL;
}

void dpu_res_register_dvfs(struct list_head *res_head)
{
	struct dpu_res_resouce_node *dvfs_node = NULL;

	dvfs_node = kzalloc(sizeof(*dvfs_node), GFP_KERNEL);
	if (unlikely(!dvfs_node))
		return;

	dvfs_node->res_type = RES_DVFS;
	atomic_set(&dvfs_node->res_ref_cnt, 0);
	dvfs_node->init = dvfs_init;
	dvfs_node->deinit = dvfs_deinit;
	dvfs_node->ioctl = NULL;

	list_add_tail(&dvfs_node->list_node, res_head);
}
