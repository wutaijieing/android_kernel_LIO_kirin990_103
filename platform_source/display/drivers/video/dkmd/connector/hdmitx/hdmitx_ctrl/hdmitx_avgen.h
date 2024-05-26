/**
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
 */

#ifndef __HDMITX_AVGEN_H__
#define __HDMITX_AVGEN_H__

#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <securec.h>

#include "hdmitx_ctrl.h"
void hdmitx_frl_video_config(struct hdmitx_ctrl *hdmitx);
void hdmitx_set_mode(struct hdmitx_ctrl *hdmitx);
s32 hdmitx_timing_config(struct hdmitx_ctrl *hdmitx);
s32 hdmitx_timing_config_safe_mode(struct hdmitx_ctrl *hdmitx);

#endif
