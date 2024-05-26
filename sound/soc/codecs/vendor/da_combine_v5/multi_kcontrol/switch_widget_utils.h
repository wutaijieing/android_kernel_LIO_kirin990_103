/*
 * switch_widget_utils.h -- da combine v5 codec driver
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

#ifndef __DA_COMBINE_V5_SWITCH_WIDGET_UTILS_H__
#define __DA_COMBINE_V5_SWITCH_WIDGET_UTILS_H__

#include <sound/soc.h>

#include "linux/platform_drivers/da_combine/da_combine_v5_type.h"


int play_config_power_event(unsigned int rate, struct snd_soc_component *codec, int event);
int audioup_2mic_enable(struct snd_soc_component *codec);
int audioup_2mic_disable(struct snd_soc_component *codec);
int audioup_4mic_enable(struct snd_soc_component *codec);
int audioup_4mic_disable(struct snd_soc_component *codec);
void ec_switch_enable(struct snd_soc_component *codec);
void ec_switch_disable(struct snd_soc_component *codec);
void iv_2pa_switch_enable(struct snd_soc_component *codec);
void iv_2pa_switch_disable(struct snd_soc_component *codec);
void hp_high_level_enable(struct snd_soc_component *codec);
void hp_high_level_disable(struct snd_soc_component *codec);
void iv_dspif_switch_enable(struct snd_soc_component *codec);
void iv_dspif_switch_disable(struct snd_soc_component *codec);
int voice_codec_bus_active(struct snd_soc_component *codec,
	struct da_combine_v5_platform_data *platform_data);
int voice_codec_bus_deactive(struct snd_soc_component *codec,
	struct da_combine_v5_platform_data *platform_data);
void u12_select_pga(struct snd_soc_component *codec);
void u12_select_dspif(struct snd_soc_component *codec);
void u56_select_dspif(struct snd_soc_component *codec);
void u56_select_pga(struct snd_soc_component *codec);
void u10_select_mic_src(struct snd_soc_component *codec);
void u10_select_dspif(struct snd_soc_component *codec);
void up_src_pre_pmu(struct snd_soc_component *codec, unsigned int channel,
	unsigned int sample_rate, unsigned int sample_rate_reg_offset, unsigned int src_mode);
void up_src_post_pmu(struct snd_soc_component *codec, unsigned int channel);
void up_src_pre_pmu_v2(struct snd_soc_component *codec, unsigned int channel,
	unsigned int sample_rate, unsigned int sample_rate_reg_offset, unsigned int src_mode);
void up_src_post_pmu_v2(struct snd_soc_component *codec, unsigned int channel);
void update_callswitch_state(int event, struct da_combine_v5_platform_data *platform_data);
int voice_v2_power_event(struct snd_soc_dapm_widget *w,
	 int event, uint32_t rate);
void u9_select_dspif(struct snd_soc_component *codec);
void u9_select_pga(struct snd_soc_component *codec);

#endif

