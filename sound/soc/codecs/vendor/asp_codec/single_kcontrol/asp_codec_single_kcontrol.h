/*
 * asp_codec_single_kcontrol.h -- asp codec single kcontrol driver
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2018-2019. All rights reserved.
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

#ifndef __ASP_CODEC_SINGLE_KCONTROL_H__
#define __ASP_CODEC_SINGLE_KCONTROL_H__
#include <sound/soc.h>

int asp_codec_add_single_kcontrols(struct snd_soc_component *codec);
#endif

