/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#ifndef DPU_DISP_MERGE_MGR_H
#define DPU_DISP_MERGE_MGR_H

#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <uapi/linux/time.h>
#include <linux/workqueue.h>

#include "../include/dpu_communi_def.h"
#include "dpu_enum.h"
#include "dpu.h"
#include "../dpu_fb_debug.h"
#include "../dpu_fb_def.h"

struct disp_merge_ops;

struct disp_merger_mgr {
	struct mutex rda_disp_lock; /* to protect merge display for rda */
	struct mutex non_rda_disp_lock; /* to protect merge display for non-rda */
	struct dpu_core_disp_data rda_overlay; /* current rda disp info */
	struct dpu_core_disp_data non_rda_overlay; /* current non-rda disp info */
	struct workqueue_struct *disp_merge_wq;
	struct work_struct disp_merge_work;
	bool is_playing;
	bool is_non_rda_task_consumed;
	bool is_rda_task_consumed;
	wait_queue_head_t wait_consume;
	struct disp_merge_ops *ops;
	int merge_disp_status[MAX_SRC_NUM];
	uint32_t rda_frame_no;
};

struct disp_merger_ctl {
	uint32_t enable;
	uint32_t disp_source;
};

struct disp_merge_ops {
	/* provide an api to close display(only for rda) */
	int (*ctl_enable)(struct disp_merger_mgr *merger_mgr, struct disp_merger_ctl *merger_ctl);

	/* provide an api for user to close display(only for rda) */
	int (*ctl_enable_for_usr)(struct disp_merger_mgr *merger_mgr, void __user *argp);

	/* provide an api for user to commit display task (but won't be executed immediately) */
	int (*commit_task)(struct disp_merger_mgr *merger_mgr, void __user *argp);

	/* initialize merge mgr(init status values, lock and wq) */
	int (*init)(struct disp_merger_mgr *merger_mgr);

	/* deinit merge mgr(delete wq , and so on) */
	void (*deinit)(struct disp_merger_mgr *merger_mgr);

	/* if exists disp merge task, execute it when vsync arrives */
	void (*exec_task)(struct disp_merger_mgr *merger_mgr);

	/* some state need reinit when power down */
	void (*suspend)(struct disp_merger_mgr *merger_mgr);

	void (*resume)(struct disp_merger_mgr *merger_mgr);
};

enum {
	DISP_NONE = -1,
	DISP_CLOSE,
	DISP_OPEN,
};

extern int dpu_load_layer_info(dss_overlay_block_t *ov_block_info, struct block_info *tmp_blk_info,
	uint32_t fb_index);
extern int dpu_online_play_config_locked(struct dpu_fb_data_type *dpufd);
extern void dpu_online_play_post_handle( struct dpu_fb_data_type *dpufd);
extern int dpu_vactive0_start_config(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req);
extern bool dpu_check_vsync_valid(struct dpu_fb_data_type *dpufd);
void register_disp_merger_mgr(struct dpu_fb_data_type *dpufd);
bool is_support_disp_merge(struct dpu_fb_data_type *dpufd);

static inline bool is_from_rda(struct dpu_core_disp_data *disp_data)
{
	return disp_data->ov_block_infos[0].layer_infos[0].disp_source == RDA_DISP;
}

static inline bool need_rda_disp(struct disp_merger_mgr *merger_mgr)
{
	return merger_mgr->merge_disp_status[RDA_DISP] == DISP_OPEN;
}

static inline bool need_non_rda_disp(struct disp_merger_mgr *merger_mgr)
{
	return merger_mgr->merge_disp_status[NON_RDA_DISP] == DISP_OPEN;
}

static inline bool has_disp_merge_task(struct disp_merger_mgr *merger_mgr)
{
	return (merger_mgr->is_rda_task_consumed == false) || (merger_mgr->is_non_rda_task_consumed == false);
}

#endif
