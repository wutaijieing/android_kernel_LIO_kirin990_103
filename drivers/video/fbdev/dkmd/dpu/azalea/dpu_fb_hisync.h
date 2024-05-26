/* Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
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
#ifndef DPU_FB_HISYNC_H
#define DPU_FB_HISYNC_H

#include "dpu_fb.h"

void dpufb_hisync_disp_sync_enable(struct dpu_fb_data_type *dpufd);
int dpufb_hisync_disp_sync_config(struct dpu_fb_data_type *dpufd);

#endif /* DPU_FB_HISYNC_H */

