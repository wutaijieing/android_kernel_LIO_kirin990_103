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
#include <linux/delay.h>

#include "dpufe_async_play.h"
#include "dpufe_dbg.h"
#include "dpufe_enum.h"
#include "dpufe_ov_play.h"
#include "dpufe_sync.h"
#include "dpufe_fb_buf_sync.h"
#include "dpufe_ov_utils.h"
#include "dpufe_pan_display.h"

#define MAX_WAIT_ASYNC_TIMES 25

static int dpufe_ov_async_play_finish(struct dpufe_data_type *dfd, void __user *argp, struct list_head *lock_list)
{
	uint64_t ov_block_infos_ptr;
	dss_overlay_t *pov_req = &(dfd->ov_req);

	dfd->frame_count++;
	ov_block_infos_ptr = pov_req->ov_block_infos_ptr; /* protect pov_req->ov_block_infos_ptr */
	pov_req->ov_block_infos_ptr = (uint64_t)0; /* clear ov_block_infos_ptr */

	if (copy_to_user((struct dss_overlay_t __user *)argp, pov_req, sizeof(dss_overlay_t))) {
		DPUFE_ERR("fb%d, copy_to_user failed.\n", dfd->index);
		return -EFAULT;
	}

	pov_req->ov_block_infos_ptr = ov_block_infos_ptr;

	dpufe_append_layer_list(dfd, lock_list);

	return 0;
}

static int dpufe_ov_async_play_commit(struct dpufe_data_type *dfd)
{
	int ret;

	ret = dpufe_play_pack_data(dfd, dfd->disp_data);
	if (ret) {
		DPUFE_ERR("pack data fail\n");
		return ret;
	}

	ret = dpufe_play_send_info(dfd, dfd->disp_data);
	if (ret) {
		DPUFE_ERR("send info fail\n");
		return ret;
	}
	return 0;
}

int dpufe_ov_async_play(struct dpufe_data_type *dfd, void __user *argp)
{
	dss_overlay_t *pov_req = NULL;
	int ret;
	struct list_head lock_list;
	timeval_compatible tv_begin;

	dpufe_check_and_return(!dfd, -EINVAL, "dfd is nullptr!\n");
	dpufe_check_and_return(!argp, -EINVAL, "argp is nullptr!\n");

	dfd->online_play_count++;
	if (dfd->online_play_count <= DSS_ONLINE_PLAY_PRINT_COUNT)
		DPUFE_INFO("[online_play_count = %d] fb%d dpufe_ov_async_play.\n", dfd->online_play_count, dfd->index);

	DPUFE_DEBUG("fb[%u] async play\n", dfd->index);

	if (g_dpufe_debug_ldi_underflow) {
		if (g_dpufe_err_status & (DSS_PDP_LDI_UNDERFLOW | DSS_SDP_LDI_UNDERFLOW)) {
			mdelay(DPU_COMPOSER_HOLD_TIME);
			return 0;
		}
	}

	if (dfd->evs_enable)
		return 0;

	if (g_dpufe_debug_fence_timeline)
		DPUFE_INFO("fb[%u] async play+\n", dfd->index);

	pov_req = &(dfd->ov_req);
	INIT_LIST_HEAD(&lock_list);

	dpufe_trace_ts_begin(&tv_begin);
	ret = dpufe_overlay_get_ov_data_from_user(dfd, pov_req, argp);
	if (ret != 0) {
		DPUFE_ERR("fb[%u], get_ov_data_from_user failed! ret=%d\n", dfd->index, ret);
		return -EINVAL;
	}

	down(&dfd->blank_sem);
	dpufe_layerbuf_lock(dfd, pov_req, &lock_list);

	ret = dpufe_ov_async_play_commit(dfd);
	if (ret != 0) {
		DPUFE_ERR("fb[%u] async play commit fail\n", dfd->index);
		up(&dfd->blank_sem);
		return -1;
	}

	dpufe_ov_async_play_finish(dfd, argp, &lock_list);
	up(&dfd->blank_sem);
	dpufe_trace_ts_end(&tv_begin, g_dpufe_debug_time_threshold, "fb[%u] config ov async play timediff=", dfd->index);
	dpufe_queue_push_tail(&g_dpu_dbg_queue[DBG_DPU0_PLAY], ktime_get());

	if (g_dpufe_debug_fence_timeline)
		DPUFE_INFO("fb[%u] async play-\n", dfd->index);

	return 0;
}

void wait_last_frame_start_working(struct dpufe_data_type *dfd)
{
	int ret;
	timeval_compatible tv_begin;
	int times = 0;
	uint32_t timeout_interval = g_dpufe_debug_wait_asy_vactive0_thr;

	dpufe_check_and_no_retval(!dfd, "dfd is null!\n");

	dpufe_trace_ts_begin(&tv_begin);

	if (dfd->asynchronous_play_flag == 0) {
		while (true) {
			ret = wait_event_interruptible_timeout(dfd->asynchronous_play_wq,
				dfd->asynchronous_play_flag, msecs_to_jiffies(timeout_interval));
			if ((ret == -ERESTARTSYS) && (times++ < MAX_WAIT_ASYNC_TIMES))
				mdelay(2); /* ms */
			else
				break;
		}

		if (ret <= 0) {
			DPUFE_ERR("fb%u wait time out, resync fence!\n", dfd->index);
			dpufe_reset_all_buf_fence(dfd);
			dump_dbg_queues(dfd->index);
		}
	}
	dfd->asynchronous_play_flag = 0;
	dpufe_trace_ts_end(&tv_begin, g_dpufe_debug_time_threshold, "fb%u wait async play time=", dfd->index);
}

int dpufe_get_release_and_retire_fence(struct dpufe_data_type *dfd, void __user *argp)
{
	int ret;
	struct dss_fence fence_fd;

	dpufe_check_and_return((!dfd) || (!argp), -EINVAL, "NULL Pointer!\n");
	dpufe_check_and_return(!dfd->panel_power_on, -EINVAL, "fb%d, panel is power off!\n", dfd->index);

	DPUFE_DEBUG("fb%u+\n", dfd->index);
	if (g_dpufe_debug_fence_timeline)
		DPUFE_INFO("fb[%u] get fence +\n", dfd->index);

	fence_fd.release_fence = -1;
	fence_fd.retire_fence = -1;

	if (dfd->evs_enable)
		return 0;

	wait_last_frame_start_working(dfd);

	ret = dpufe_buf_sync_create_fence(dfd, &fence_fd.release_fence, &fence_fd.retire_fence);
	if (ret != 0) {
		DPUFE_INFO("fb[%u], dpu_create_fence failed!\n", dfd->index);
		return -1;
	}

	if (copy_to_user((struct dss_fence __user *)argp, &fence_fd, sizeof(struct dss_fence))) {
		DPUFE_ERR("fb[%u], copy_to_user failed.\n", dfd->index);
		dpufe_buf_sync_close_fence(&fence_fd.release_fence, &fence_fd.retire_fence);
		ret = -EFAULT;
	}

	if (g_dpufe_debug_fence_timeline)
		DPUFE_INFO("fb[%u] get fence -\n", dfd->index);
	DPUFE_DEBUG("fb%u-\n", dfd->index);

	return ret;
}

void dpufe_resume_async_state(struct dpufe_data_type *dfd)
{
	dfd->asynchronous_play_flag = 1;
}
