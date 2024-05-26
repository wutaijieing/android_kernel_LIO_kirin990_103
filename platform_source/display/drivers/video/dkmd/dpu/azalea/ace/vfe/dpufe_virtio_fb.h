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

#ifndef DPUFE_VIRTIO_FB_H
#define DPUFE_VIRTIO_FB_H

#include "../include/dpu_communi_def.h"

int get_registered_fb_nums(void);
int vfb_send_blank_ctl(uint32_t fb_id, int blank_mode);
int vfb_send_buffer(struct dpu_core_disp_data *ov_info);

#endif
