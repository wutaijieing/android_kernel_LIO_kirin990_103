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

#ifndef __HDMITX_CTRL_DEV_H__
#define __HDMITX_CTRL_DEV_H__

#include "dpu_connector.h"

#if (CONFIG_DKMD_DPU_VERSION >= 740)
void hdmitx_ctrl_default_setup(struct dpu_connector *connector);
#else
static void hdmitx_ctrl_default_setup(struct dpu_connector *connector) {}
#endif

#endif
