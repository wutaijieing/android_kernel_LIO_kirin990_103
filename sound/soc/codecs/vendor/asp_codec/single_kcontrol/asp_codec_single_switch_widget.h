/*
 * asp_codec_single_switch_widget.h -- asp codec single switch widget driver
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2020. All rights reserved.
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

#ifndef __ASP_CODEC_SINGLE_SWITCH_WIDGET_H__
#define __ASP_CODEC_SINGLE_SWITCH_WIDGET_H__
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

int asp_codec_add_single_switch_widgets(struct snd_soc_dapm_context *dapm);
#endif

