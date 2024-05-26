/*
 * da_separate_mbhc_custom.c -- da_separate mbhc custom driver
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

#include "da_separate_mbhc_custom.h"
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/version.h>

#include "asoc_adapter.h"

#ifdef CONFIG_SND_SOC_CODEC_DA_SEPARATE_V3
#include "da_separate_utils.h"
#include "da_separate_v3.h"
#endif
#if (defined(CONFIG_SND_SOC_CODEC_DA_SEPARATE_V5)) || (defined(CONFIG_SND_SOC_CODEC_DA_SEPARATE_V6L))
#include "da_separate_utils.h"
#include "da_separate_v5.h"
#endif

#ifdef CONFIG_SND_SOC_CODEC_DA_SEPARATE_V2
#include "da_separate/da_separate_v2/da_separate_v2_utility.h"
#include "da_separate/da_separate_v2/da_separate_v2_pmu_reg_def.h"
#include "da_separate/da_separate_v2/da_separate_v2.h"
#endif

#ifdef CONFIG_SND_SOC_ASP_CODEC_ANA
#include "asp_codec_ana_reg_def.h"
#else
#include "da_separate_reg.h"
#endif

#include "audio_log.h"

#define LOG_TAG "da_separate_mbhc_custom"

#define MIC_SELECT_MASK                0x30
#define HEADSET_MIC_SELECTED           0x20
#define MIC_MUTE_TIME_MS               1300

static bool g_hs_mic_mute;

void da_separate_hs_mic_mute(struct snd_soc_component *codec)
{
	unsigned int hs_mic_select;
	WARN_ON(!codec);

	if (!g_hs_mic_mute) {
		AUDIO_LOGI("headset mic mute not support\n");
		return;
	}

	/* mute uplink ADCL&ADCR reg when headset plug out
	 * and headset-mic is using,
	 * unmute uplink ADCL&ADCR reg after delay 1300ms.
	 */
	hs_mic_select = snd_soc_component_read32(codec,
		CODEC_ANA_RW9_REG) & MIC_SELECT_MASK;
	if (hs_mic_select != HEADSET_MIC_SELECTED)
		return;
#ifdef CONFIG_SND_SOC_CODEC_DA_SEPARATE_V3
	snd_soc_component_update_bits(codec, CODEC_ANA_RW3_REG,
		BIT(ADCL_MUTE_OFFSET) | BIT(ADCR_MUTE_OFFSET),
		BIT(ADCL_MUTE_OFFSET) | BIT(ADCR_MUTE_OFFSET));
	AUDIO_LOGI("mute uplink ADCL&ADCR reg\n");
	msleep(MIC_MUTE_TIME_MS);
	snd_soc_component_update_bits(codec, CODEC_ANA_RW3_REG,
		BIT(ADCL_MUTE_OFFSET) | BIT(ADCR_MUTE_OFFSET), 0);
	AUDIO_LOGI("delay 1300ms unmute uplink ADCL&ADCR reg\n");
#endif
}

bool da_separate_get_mic_mute_status(void)
{
	return g_hs_mic_mute;
}

void da_separate_mbhc_custom_init(struct device_node *np)
{
	WARN_ON(!np);
	g_hs_mic_mute = false;
	if (of_property_read_bool(np, "hs_mic_mute"))
		g_hs_mic_mute = true;
}
