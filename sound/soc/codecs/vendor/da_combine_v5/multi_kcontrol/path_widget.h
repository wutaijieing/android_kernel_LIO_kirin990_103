/*
 * path_widget.h -- da combine v5 codec driver
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

#ifndef __DA_COMBINE_V5_PATH_WIDGET_H__
#define __DA_COMBINE_V5_PATH_WIDGET_H__

#include <sound/soc.h>

#include "linux/platform_drivers/da_combine/da_combine_v5_type.h"

enum headset_path {
	HEADSET_LEFT = 0x1,
	HEADSET_RIGHT
};

int da_combine_v5_add_path_widgets(struct snd_soc_component *codec, bool single_kcontrol);
void da_combine_v5_set_classH_config(struct snd_soc_component *codec,
	enum classH_state classH_state_cfg);
void da_combine_v5_headphone_pop_on(struct snd_soc_component *codec);
void da_combine_v5_headphone_pop_off(struct snd_soc_component *codec);
void headphone_pop_on_delay_work(struct work_struct *work);
void dacl_post_mixer_enable(struct snd_soc_component *codec);
void dacl_post_mixer_disable(struct snd_soc_component *codec);
void dacr_post_mixer_enable(struct snd_soc_component *codec);
void dacr_post_mixer_disable(struct snd_soc_component *codec);
void dacl_pga_mux_enable(struct snd_soc_component *codec);
void dacl_pga_mux_disable(struct snd_soc_component *codec);
void dacr_pga_mux_enable(struct snd_soc_component *codec);
void dacr_pga_mux_disable(struct snd_soc_component *codec);

#endif

