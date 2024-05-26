/*
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef __DPU_HIHDR_H__
#define __DPU_HIHDR_H__

#include "dpu_fb.h"

void init_hihdr(struct dpu_fb_data_type *dpufd);
void deinit_hihdr(struct dpu_fb_data_type *dpufd);
void dpu_get_hihdr_mean(struct dpu_fb_data_type *dpufd);
void dpu_hihdr_set_reg(struct dpu_fb_data_type *dpufd);
bool dpu_check_hihdr_active(struct dpu_fb_data_type *dpufd);

#endif /*__DPU_HIHDR_H__*/

