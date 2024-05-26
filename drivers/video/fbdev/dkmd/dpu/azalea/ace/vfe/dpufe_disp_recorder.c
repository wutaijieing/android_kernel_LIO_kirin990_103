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

#include "dpufe.h"
#include "dpufe_dbg.h"
#include "dpufe_disp_recorder.h"
#include "dpufe_panel_def.h"

/* disp check period-->1000ms */
#define DISP_CHECK_TIME_PERIOD 1000

void enable_disp_recorder(struct disp_state_recorder *disp_recorder)
{
	dpufe_check_and_no_retval(!disp_recorder, "disp_recorder is null\n");

	hrtimer_start(&disp_recorder->disp_timer, ktime_set(DISP_CHECK_TIME_PERIOD / 1000,
		(DISP_CHECK_TIME_PERIOD % 1000) * 1000000), HRTIMER_MODE_REL);
}

void disable_disp_recorder(struct disp_state_recorder *disp_recorder)
{
	dpufe_check_and_no_retval(!disp_recorder, "disp_recorder is null\n");

	hrtimer_cancel(&disp_recorder->disp_timer);
}

static enum hrtimer_restart dump_disp_state(struct hrtimer *timer)
{
	struct disp_state_recorder *disp_recorder = NULL;
	struct dpufe_data_type *dfd = NULL;
	static uint32_t frame_count[MAX_DISP_CHN_NUM];
	uint32_t fb_idx;

	disp_recorder = container_of(timer, struct disp_state_recorder, disp_timer);
	dpufe_check_and_return(!disp_recorder, HRTIMER_NORESTART, "disp_recorder is null\n");

	dfd = (struct dpufe_data_type *)disp_recorder->par;
	dpufe_check_and_return(!dfd, HRTIMER_NORESTART, "dfd is null\n");

	fb_idx = dfd->index;
	dpufe_check_and_return(fb_idx >= MAX_DISP_CHN_NUM, HRTIMER_NORESTART, "invalid fb%u", fb_idx);

	DPUFE_INFO("fb%u has %u frames\n", fb_idx, dfd->frame_count - frame_count[fb_idx]);
	frame_count[fb_idx] = dfd->frame_count;

	enable_disp_recorder(disp_recorder);

	return HRTIMER_NORESTART;
}

void init_disp_recorder(struct disp_state_recorder *disp_recorder, void *data)
{
	dpufe_check_and_no_retval(!disp_recorder, "disp_recorder is null\n");

	DPUFE_INFO("+\n");
	hrtimer_init(&disp_recorder->disp_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	disp_recorder->disp_timer.function = dump_disp_state;
	disp_recorder->par = data;
	DPUFE_INFO("-\n");
}
