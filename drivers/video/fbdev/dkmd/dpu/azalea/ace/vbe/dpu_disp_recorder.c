/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
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

#include "dpu_fb_struct.h"
#include "dpu_fb_def.h"
#include "dpu_disp_recorder.h"

/* disp check period-->1000ms */
#define DISP_CHECK_TIME_PERIOD 1000

void enable_disp_recorder(struct disp_state_recorder *disp_recorder)
{
	dpu_check_and_no_retval(!disp_recorder, ERR, "disp_recorder is null\n");

	hrtimer_start(&disp_recorder->disp_timer, ktime_set(DISP_CHECK_TIME_PERIOD / 1000,
		(DISP_CHECK_TIME_PERIOD % 1000) * 1000000), HRTIMER_MODE_REL);
}

void disable_disp_recorder(struct disp_state_recorder *disp_recorder)
{
	dpu_check_and_no_retval(!disp_recorder, ERR, "disp_recorder is null\n");

	hrtimer_cancel(&disp_recorder->disp_timer);
}

static enum hrtimer_restart dump_disp_state(struct hrtimer *timer)
{
	struct disp_state_recorder *disp_recorder = NULL;
	struct dpu_fb_data_type *dpufd = NULL;
	static uint32_t frame_cnt = 0;
	static uint32_t rda_cnt = 0;
	static uint32_t non_rda_cnt = 0;

	disp_recorder = container_of(timer, struct disp_state_recorder, disp_timer);
	dpu_check_and_return(!disp_recorder, HRTIMER_NORESTART, ERR, "disp_recorder is null\n");

	dpufd = (struct dpu_fb_data_type *)disp_recorder->par;
	dpu_check_and_return(!dpufd, HRTIMER_NORESTART, ERR, "dpufd is null\n");

	if (is_support_disp_merge(dpufd)) {
		DPU_FB_INFO("fb%u rda has %u frames, non rda has %u frames\n",
			dpufd->index, dpufd->merger_mgr.rda_frame_no - rda_cnt,
			dpufd->merger_mgr.non_rda_overlay.frame_no - non_rda_cnt);
		rda_cnt = dpufd->merger_mgr.rda_frame_no;
		non_rda_cnt = dpufd->merger_mgr.non_rda_overlay.frame_no;
	} else {
		DPU_FB_INFO("fb%u has %u frames\n", dpufd->index, dpufd->frame_count - frame_cnt);
		frame_cnt = dpufd->frame_count;
	}

	enable_disp_recorder(disp_recorder);

	return HRTIMER_NORESTART;
}

void init_disp_recorder(struct disp_state_recorder *disp_recorder, void *data)
{
	dpu_check_and_no_retval(!disp_recorder, ERR, "disp_recorder is null\n");

	DPU_FB_INFO("+\n");
	hrtimer_init(&disp_recorder->disp_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	disp_recorder->disp_timer.function = dump_disp_state;
	disp_recorder->par = data;
	DPU_FB_INFO("-\n");
}
