/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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
#ifndef DPU_OVERLAY_UTILS_H
#define DPU_OVERLAY_UTILS_H

#include "../dpu_fb.h"

#include "dump/dpu_dump.h"
#include "tool/dpu_tool.h"
#include "init/dpu_init.h"
#include "handler/dpu_handler.h"
#include "clip/dpu_clip.h"
#include "dfc/dpu_dfc.h"
#include "mctl/dpu_mctl.h"
#include "mif/dpu_mif.h"
#include "ovl/dpu_ovl.h"
#include "rdma/dpu_rdma.h"
#include "scl/dpu_scl.h"
#include "dpu_overlay_utils_platform.h"

#define P3_PROCESS_NEEDED_LAYER (1U << 31)

int dpu_ch_module_set_regs(struct dpu_fb_data_type *dpufd, int32_t mctl_idx,
	int chn_idx, uint32_t wb_type, bool enable_cmdlist);

int dpu_ov_module_set_regs(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req, int ovl_idx,
	struct ov_module_set_regs_flag ov_module_flag);

int dpu_prev_module_set_regs(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req, uint32_t cmdlist_pre_idxs, bool enable_cmdlist, int *use_comm_mmbuf);
bool dpu_check_reg_reload_status(struct dpu_fb_data_type *dpufd);
bool dpu_check_crg_sctrl_status(struct dpu_fb_data_type *dpufd);
void dpu_ldi_underflow_handle_func(struct work_struct *work);
int dpu_overlay_get_ov_data_from_user(struct dpu_fb_data_type *dpufd,
	dss_overlay_t *pov_req, const void __user *argp);
uint64_t dpu_get_vsync_timediff(struct dpu_fb_data_type *dpufd);
int dpu_check_vsync_timediff(struct dpu_fb_data_type *dpufd, dss_overlay_t *pov_req);
bool csc_need_p3_process(uint32_t csc_mode);
bool dpu_is_async_play(struct dpu_fb_data_type *dpufd);

#endif /* DPU_OVERLAY_UTILS_H */
