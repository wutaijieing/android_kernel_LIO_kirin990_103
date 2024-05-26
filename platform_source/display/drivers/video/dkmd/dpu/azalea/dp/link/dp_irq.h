/*
 * Copyright (c) 2016 Synopsys, Inc.
 *
 * Synopsys DP TX Linux Software Driver and documentation (hereinafter,
 * "Software") is an Unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 */

#ifndef DP_IRQ_H
#define DP_IRQ_H

#include "dpu_dp.h"

struct dp_ctrl;
struct dpu_fb_data_type;

#define MAX_EXT_BLOCKS 3
#define DP_DMD_REPORT_SIZE 900

int handle_hotplug(struct dpu_fb_data_type *dpufd);
int handle_hotunplug(struct dpu_fb_data_type *dpufd);
void dptx_link_params_reset(struct dp_ctrl *dptx);

struct test_dtd {
	uint32_t h_total;
	uint32_t v_total;
	uint32_t h_start;
	uint32_t v_start;
	uint32_t h_sync_width;
	uint32_t v_sync_width;
	uint32_t h_width;
	uint32_t v_width;
	uint32_t h_sync_pol;
	uint32_t v_sync_pol;
	uint32_t refresh_rate;
	enum video_format_type video_format;
	uint8_t vmode;
};

int handle_sink_request(struct dp_ctrl *dptx);
int dptx_calc_fps(struct timing_info *dptx_timing_node, uint32_t *fps);

#endif /* DP_IRQ_H */

