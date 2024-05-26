/*
 * da_separate_mbhc_custom.h -- da_separate mbhc custom driver
 *
 * Copyright (c) 2020 Hisilicon Technologies Co., Ltd.
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
#ifndef __DA_SEPARATE_MBHC_CUSTOM_H__
#define __DA_SEPARATE_MBHC_CUSTOM_H__

#include "da_separate_mbhc.h"

void da_separate_hs_mic_mute(struct snd_soc_component *codec);
bool da_separate_get_mic_mute_status(void);
void da_separate_mbhc_custom_init(struct device_node *np);

#endif

