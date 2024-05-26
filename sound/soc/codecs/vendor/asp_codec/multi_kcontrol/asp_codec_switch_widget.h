/*
 * asp_codec_switch_widget.h -- asp codec switch widget driver
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

#ifndef __ASP_CODEC_SWITCH_WIDGET_H__
#define __ASP_CODEC_SWITCH_WIDGET_H__
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

int vo_up_l_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event);
int vo_up_r_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event);
int voice_2mic_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event);
int audio_dn_l_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event);
int audio_dn_r_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event);
int codec3_dn_l_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event);
int codec3_dn_r_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event);
int s1_tx_tdm_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event);
int iv_4pa_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event);
int s3_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event);
int mic3_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event);
int mic4_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event);
int mic34_bt_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event);
int mdm5g_up_l_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event);
int mdm5g_up_r_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event);
int mdm5g_dn_l_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event);
int mdm5g_dn_r_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event);

int ultr_dn_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event);

int asp_codec_add_switch_widgets(struct snd_soc_dapm_context *dapm);
#endif

