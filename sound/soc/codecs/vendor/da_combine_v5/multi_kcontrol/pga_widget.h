/*
 * pga_widget.h -- da combine v5 codec driver
 *
 * Copyright (c) 2018 Huawei Technologies Co., Ltd.
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

#ifndef __DA_COMBINE_V5_PGA_WIDGET_H__
#define __DA_COMBINE_V5_PGA_WIDGET_H__

#include "linux/platform_drivers/da_combine/da_combine_v5_type.h"

int da_combine_v5_add_pga_widgets(struct snd_soc_component *codec, bool single_kcontrol);

#endif
