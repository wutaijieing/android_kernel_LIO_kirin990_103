/*
 * asp_codec_route.c -- asp codec route driver
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

#include "asp_codec_route.h"

#include "audio_log.h"

#ifdef CONFIG_AUDIO_DEBUG
#define LOG_TAG "asp_codec"
#else
#define LOG_TAG "Analog_less_v1"
#endif

#define SUPPLY_ROUTE \
	{ "PDM_MIC12_INPUT",     NULL, "DMIC1_SUPPLY" }, \
	{ "PDM_MIC34_INPUT",     NULL, "DMIC2_SUPPLY" }, \
	{ "AU_INPUT",            NULL, "AU_IN_SUPPLY" }, \
	{ "MDM5G_DN_INPUT",      NULL, "MDM5G_DN_L_SUPPLY" }, \
	{ "MDM5G_DN_INPUT",      NULL, "MDM5G_DN_R_SUPPLY" }, \
	{ "S1_RX_INPUT",         NULL, "S1_SUPPLY" }, \
	{ "S1_RX_INPUT",         NULL, "S1_RX_SUPPLY" }, \
	{ "S1_TX_OUTPUT",        NULL, "S1_SUPPLY" }, \
	{ "S2_RX_INPUT",         NULL, "S2_SUPPLY" }, \
	{ "S2_RX_INPUT",         NULL, "S2_RX_SUPPLY" }, \
	{ "S2_TX_OUTPUT",        NULL, "S2_SUPPLY" }, \
	{ "S3_RX_INPUT",         NULL, "S3_SUPPLY" }, \
	{ "S3_RX_INPUT",         NULL, "S3_RX_SUPPLY" }, \
	{ "S3_SAMPLE_INPUT",     NULL, "S3_SUPPLY" }, \
	{ "S3_SAMPLE_INPUT",     NULL, "S3_RX_SUPPLY" }, \
	{ "S4_RX_INPUT",         NULL, "S4_SUPPLY" }, \
	{ "S4_RX_INPUT",         NULL, "S4_RX_SUPPLY" }, \
	{ "AUDIO_DN_FIFO_INPUT", NULL, "PLL" } \

#define DN_PGA_ROUTE \
	{ "AUDIO_DN_L_PGA",  NULL, "AUDIO_DN_L_SWITCH" }, \
	{ "AUDIO_DN_R_PGA",  NULL, "AUDIO_DN_R_SWITCH" }, \
	{ "CODEC3_DN_L_PGA", NULL, "CODEC3_DN_L_SWITCH" }, \
	{ "CODEC3_DN_R_PGA", NULL, "CODEC3_DN_R_SWITCH" }, \
	{ "I2S2_RX_L_PGA",   NULL, "S2_RX_L_SWITCH" }, \
	{ "I2S2_RX_R_PGA",   NULL, "S2_RX_R_SWITCH" }, \
	{ "MDM5G_UP_L_PGA",  NULL, "MDM5G_UP_L_MUX" }, \
	{ "MDM5G_UP_R_PGA",  NULL, "MDM5G_UP_R_MUX" }, \
	{ "SIDETONE_PGA",    NULL, "SIDETONE_PGA_MUX" }, \
	{ "ADC1_0P5_PGA",    NULL, "ADC1" }, \
	{ "MIC3_UP_PGA",     NULL, "MIC3_MUX" }, \
	{ "MIC4_UP_PGA",     NULL, "MIC4_MUX" } \

#define ADC_MUX_ROUTE \
	{ "ADC1_MUX", "DMIC1", "CIC_ADC1_SWITCH" }, \
	{ "ADC1_MUX", "AU",    "AU_INPUT" }, \
	{ "ADC2_MUX", "DMIC1", "CIC_ADC2_SWITCH" }, \
	{ "ADC2_MUX", "AU",    "AU_INPUT" }, \
	{ "ADC3_MUX", "DMIC2", "CIC_ADC3_SWITCH" }, \
	{ "ADC3_MUX", "AU",    "AU_INPUT" }, \
	{ "ADC4_MUX", "DMIC2", "CIC_ADC4_SWITCH" }, \
	{ "ADC4_MUX", "AU",    "AU_INPUT" } \

#define ADC_ROUTE \
	{ "ADC1", NULL, "ADC1_MUX" }, \
	{ "ADC2", NULL, "ADC2_MUX" }, \
	{ "ADC3", NULL, "ADC3_MUX" }, \
	{ "ADC4", NULL, "ADC4_MUX" } \

#define UP_MUX_ROUTE \
	{ "AUDIO_UP_L_MUX",     "MIC1_ADC",    "ADC1" }, \
	{ "AUDIO_UP_L_MUX",     "MIC2_ADC",    "ADC2" }, \
	{ "AUDIO_UP_L_MUX",     "I2S2_RX_L",   "I2S2_RX_L_PGA" }, \
	{ "AUDIO_UP_L_MUX",     "MIXER4_L",    "DAC_L_MIXER4" }, \
	{ "AUDIO_UP_L_MUX",     "MIXER2_S_L",  "DACL_S_MIXER2" }, \
	{ "AUDIO_UP_L_MUX",     "AUDIO_DN_L",  "AUDIO_DN_L_PGA" }, \
	{ "AUDIO_UP_R_MUX",     "MIC1_ADC",    "ADC1" }, \
	{ "AUDIO_UP_R_MUX",     "MIC2_ADC",    "ADC2" }, \
	{ "AUDIO_UP_R_MUX",     "I2S2_RX_R",   "I2S2_RX_R_PGA" }, \
	{ "AUDIO_UP_R_MUX",     "MIXER4_R",    "DAC_R_MIXER4" }, \
	{ "AUDIO_UP_R_MUX",     "MIXER2_S_R",  "DACR_S_MIXER2" }, \
	{ "AUDIO_UP_R_MUX",     "AUDIO_DN_R",  "AUDIO_DN_R_PGA" }, \
	{ "VOICE_UP_L_MUX",     "MIC1_ADC",    "ADC1" }, \
	{ "VOICE_UP_L_MUX",     "MIC2_ADC",    "ADC2" }, \
	{ "VOICE_UP_L_MUX",     "I2S2_RX_L",   "I2S2_RX_L_PGA" }, \
	{ "VOICE_UP_L_MUX",     "MIXER4_L",    "DAC_L_MIXER4" }, \
	{ "VOICE_UP_L_MUX",     "CODEC3_DN_L", "CODEC3_DN_L_PGA" }, \
	{ "VOICE_UP_R_MUX",     "MIC1_ADC",    "ADC1" }, \
	{ "VOICE_UP_R_MUX",     "MIC2_ADC",    "ADC2" }, \
	{ "VOICE_UP_R_MUX",     "I2S2_RX_R",   "I2S2_RX_R_PGA" }, \
	{ "VOICE_UP_R_MUX",     "MIXER4_R",    "DAC_R_MIXER4" }, \
	{ "VOICE_UP_R_MUX",     "CODEC3_DN_R", "CODEC3_DN_R_SWITCH" }, \
	{ "MIC3_MUX",           "MIC3_ADC",    "ADC3" }, \
	{ "MIC3_MUX",           "MIC4_ADC",    "ADC4" }, \
	{ "MIC3_MUX",           "I2S3_RX_L",   "S3_RX_INPUT" }, \
	{ "MIC4_MUX",           "MIC3_ADC",    "ADC3" }, \
	{ "MIC4_MUX",           "MIC4_ADC",    "ADC4" }, \
	{ "MIC4_MUX",           "I2S3_RX_R",   "S3_RX_INPUT" }, \
	{ "MIC5_MUX",           "MIC1_ADC",    "ADC1" }, \
	{ "MIC5_MUX",           "MIC2_ADC",    "ADC2" }, \
	{ "MIC5_MUX",           "MIC3_ADC",    "ADC3" }, \
	{ "MIC5_MUX",           "MIC4_ADC",    "ADC4" }, \
	{ "MDM5G_UP_L_MUX",     "MIXER4_L",    "DAC_L_MIXER4" }, \
	{ "MDM5G_UP_L_MUX",     "CODEC3_DN_L", "CODEC3_DN_L_PGA" }, \
	{ "MDM5G_UP_L_MUX",     "MIXER2_S_L",  "DACL_S_MIXER2" }, \
	{ "MDM5G_UP_L_MUX",     "ULTR_DN_L",   "ULTR_DN_96K_SWITCH" }, \
	{ "MDM5G_UP_L_MUX",     "ULTR_DN_L",   "ULTR_DN_48K_SWITCH" }, \
	{ "MDM5G_UP_L_MUX",     "I2S3_RX_L",   "S3_RX_INPUT" }, \
	{ "MDM5G_UP_R_MUX",     "MIXER4_R",    "DAC_R_MIXER4" }, \
	{ "MDM5G_UP_R_MUX",     "CODEC3_DN_R", "CODEC3_DN_R_PGA" }, \
	{ "MDM5G_UP_R_MUX",     "MIXER2_S_R",  "DACR_S_MIXER2" }, \
	{ "MDM5G_UP_R_MUX",     "ULTR_DN_R",   "ULTR_DN_96K_SWITCH" }, \
	{ "MDM5G_UP_R_MUX",     "ULTR_DN_R",   "ULTR_DN_48K_SWITCH" }, \
	{ "MDM5G_UP_R_MUX",     "I2S3_RX_R",   "S3_RX_INPUT" }, \
	{ "MDM5G_UP_FIFOL_MUX", "MDM5G_UP_L",  "MDM5G_UP_L_PGA" }, \
	{ "MDM5G_UP_FIFOL_MUX", "S4_RX_L",     "S4_RX_INPUT" }, \
	{ "MDM5G_UP_FIFOR_MUX", "MDM5G_UP_R",  "MDM5G_UP_R_PGA" }, \
	{ "MDM5G_UP_FIFOR_MUX", "S4_RX_R",     "S4_RX_INPUT" }, \
	{ "MDM5G_UP_FIFOR_MUX", "S4_RX_R",     "S4_RX_SWITCH" }, \
	{ "MDM5G_UP_FIFOL_MUX", "S4_RX_L",     "S4_RX_SWITCH" } \

#define UP_SWITCH_ROUTE \
	{ "DMIC1_IN_SWITCH",       "ENABLE", "PDM_MIC12_INPUT" }, \
	{ "DMIC2_IN_SWITCH",       "ENABLE", "PDM_MIC34_INPUT" }, \
	{ "CIC_ADC1_SWITCH",       "ENABLE", "DMIC1_IN_SWITCH" }, \
	{ "CIC_ADC2_SWITCH",       "ENABLE", "DMIC1_IN_SWITCH" }, \
	{ "CIC_ADC3_SWITCH",       "ENABLE", "DMIC2_IN_SWITCH" }, \
	{ "CIC_ADC4_SWITCH",       "ENABLE", "DMIC2_IN_SWITCH" }, \
	{ "AUDIO_UP_L_O_SWITCH",   "ENABLE", "AUDIO_UP_L_MUX" }, \
	{ "AUDIO_UP_R_O_SWITCH",   "ENABLE", "AUDIO_UP_R_MUX" }, \
	{ "MIC3_O_SWITCH",         "ENABLE", "MIC3_MUX" }, \
	{ "MIC34_96K_SWITCH",      "ENABLE", "MIC3_MUX" }, \
	{ "MIC3_O_SWITCH",         "ENABLE", "MIC3_UP_PGA" }, \
	{ "MIC4_O_SWITCH",         "ENABLE", "MIC4_MUX" }, \
	{ "MIC34_96K_SWITCH",      "ENABLE", "MIC4_MUX" }, \
	{ "MIC4_O_SWITCH",         "ENABLE", "MIC4_UP_PGA" }, \
	{ "VOICE_UP_L_O_SWITCH",   "ENABLE", "VOICE_UP_L_MUX" }, \
	{ "VOICE_UP_R_O_SWITCH",   "ENABLE", "VOICE_UP_R_MUX" }, \
	{ "AUDIO_4MIC_O_SWITCH",   "ENABLE", "AUDIO_MIC_INPUT" }, \
	{ "AUDIO_2MIC_O_SWITCH",   "ENABLE", "AUDIO_MIC_INPUT" }, \
	{ "VOICE_2MIC_O_SWITCH",   "ENABLE", "VOICE_MIC_INPUT" }, \
	{ "VOICE_4MIC_O_SWITCH",   "ENABLE", "VOICE_MIC_INPUT" }, \
	{ "MDM5G_UP_L_96K_SWITCH", "ENABLE", "MDM5G_UP_FIFOL_MUX" }, \
	{ "MDM5G_UP_L_48K_SWITCH", "ENABLE", "MDM5G_UP_FIFOL_MUX" }, \
	{ "MDM5G_UP_R_96K_SWITCH", "ENABLE", "MDM5G_UP_FIFOR_MUX" }, \
	{ "MDM5G_UP_R_48K_SWITCH", "ENABLE", "MDM5G_UP_FIFOR_MUX" }, \
	{ "ULTR_UP_O_SWITCH",      "ENABLE", "MIC5_MUX" }, \
	{ "MIC3_32K_SWITCH",       "ENABLE", "MIC3_INPUT" }, \
	{ "MIC3_48K_SWITCH",       "ENABLE", "MIC3_INPUT" }, \
	{ "MIC4_32K_SWITCH",       "ENABLE", "MIC4_INPUT" }, \
	{ "MIC4_48K_SWITCH",       "ENABLE", "MIC4_INPUT" }, \
	{ "VOICE_UP_L_8K_SWITCH",  "ENABLE", "VOICE_UP_INPUT" }, \
	{ "VOICE_UP_L_16K_SWITCH", "ENABLE", "VOICE_UP_INPUT" }, \
	{ "VOICE_UP_L_32K_SWITCH", "ENABLE", "VOICE_UP_INPUT" }, \
	{ "VOICE_UP_L_48K_SWITCH", "ENABLE", "VOICE_UP_INPUT" }, \
	{ "VOICE_UP_R_8K_SWITCH",  "ENABLE", "VOICE_UP_INPUT" }, \
	{ "VOICE_UP_R_16K_SWITCH", "ENABLE", "VOICE_UP_INPUT" }, \
	{ "VOICE_UP_R_32K_SWITCH", "ENABLE", "VOICE_UP_INPUT" }, \
	{ "VOICE_UP_R_48K_SWITCH", "ENABLE", "VOICE_UP_INPUT" }, \
	{ "S3_44K1_SWITCH",        "ENABLE", "S3_SAMPLE_INPUT" }, \
	{ "S3_48K_SWITCH",         "ENABLE", "S3_SAMPLE_INPUT" }, \
	{ "S3_96K_SWITCH",         "ENABLE", "S3_SAMPLE_INPUT" }, \
	{ "S3_192K_SWITCH",        "ENABLE", "S3_SAMPLE_INPUT" }, \
	{ "S4_RX_SWITCH",          "ENABLE", "S4_RX_INPUT" } \

#define DN_MIXER_ROUTE \
	{ "DAC_L_MIXER4",  "AUDIO_DN_L",   "AUDIO_DN_L_PGA" }, \
	{ "DAC_R_MIXER4",  "AUDIO_DN_R",   "AUDIO_DN_R_PGA" }, \
	{ "DAC_L_MIXER4",  "SIDETONE",     "SIDETONE_PGA" }, \
	{ "DAC_R_MIXER4",  "SIDETONE",     "SIDETONE_PGA" }, \
	{ "DACL_S_MIXER2", "MIXER4_SRC_L", "DAC_L_MIXER4" }, \
	{ "DACL_S_MIXER2", "ULTR_DN_L",    "ULTR_DN_96K_SWITCH" }, \
	{ "DACL_S_MIXER2", "ULTR_DN_L",    "ULTR_DN_48K_SWITCH" }, \
	{ "DACR_S_MIXER2", "MIXER4_SRC_R", "DAC_R_MIXER4" }, \
	{ "DACR_S_MIXER2", "ULTR_DN_R",    "ULTR_DN_96K_SWITCH" }, \
	{ "DACR_S_MIXER2", "ULTR_DN_R",    "ULTR_DN_48K_SWITCH" } \

#define DN_MUX_ROUTE \
	{ "S1_TX_L_MUX",     "MIXER2_S_L",        "DACL_S_MIXER2" }, \
	{ "S1_TX_R_MUX",     "MIXER2_S_R",        "DACR_S_MIXER2" }, \
	{ "S1_TX_L_MUX",     "AU_UP_L",           "AUDIO_UP_L_MUX" }, \
	{ "S1_TX_R_MUX",     "AU_UP_R",           "AUDIO_UP_R_MUX" }, \
	{ "S1_TDM_TX_L_MUX", "AUDIO_DN_L",        "AUDIO_DN_L_PGA" }, \
	{ "S1_TDM_TX_R_MUX", "AUDIO_DN_R",        "AUDIO_DN_R_PGA" }, \
	{ "S2_TX_R_SEL_MUX", "I2S2_TX_R_BTMATCH", "DAC_R_MIXER4" }, \
	{ "BT_TX_SRCDN_MUX", "MIXER4_L",          "DAC_L_MIXER4" }, \
	{ "BM_DACL_AU_MUX",  "DAC_M_L",           "DACL_M_SWITCH" }, \
	{ "SIDETONE_PGA_MUX","MIC1_ADC",          "ADC1_0P5_PGA" } \

#define DN_SWITCH_ROUTE \
	{ "CODEC3_DN_L_SWITCH",    "ENABLE", "CODEC3_DN_FIFO_INPUT" }, \
	{ "CODEC3_DN_R_SWITCH",    "ENABLE", "CODEC3_DN_FIFO_INPUT" }, \
	{ "CODEC3_DN_44K1_SWITCH", "ENABLE", "CODEC3_DN_INPUT" }, \
	{ "CODEC3_DN_48K_SWITCH",  "ENABLE", "CODEC3_DN_INPUT" }, \
	{ "CODEC3_DN_96K_SWITCH",  "ENABLE", "CODEC3_DN_INPUT" }, \
	{ "CODEC3_DN_192K_SWITCH", "ENABLE", "CODEC3_DN_INPUT" }, \
	{ "AUDIO_DN_L_SWITCH",     "ENABLE", "AUDIO_DN_FIFO_INPUT" }, \
	{ "AUDIO_DN_R_SWITCH",     "ENABLE", "AUDIO_DN_FIFO_INPUT" }, \
	{ "AUDIO_DN_48K_SWITCH",   "ENABLE", "AUDIO_DN_INPUT" }, \
	{ "S3_TX_SWITCH",          "ENABLE", "CODEC3_DN_L_PGA" }, \
	{ "S3_TX_SWITCH",          "ENABLE", "CODEC3_DN_R_PGA" }, \
	{ "ULTR_DN_96K_SWITCH",    "ENABLE", "ULTR_DN_INPUT" }, \
	{ "ULTR_DN_48K_SWITCH",    "ENABLE", "ULTR_DN_INPUT" }, \
	{ "CARKIT_48K_SWITCH",     "ENABLE", "CARKIT_INPUT" }, \
	{ "DACL_M_SWITCH",         "ENABLE", "DACL_S_MIXER2" } \

#define I2S_SWITCH_ROUTE \
	{ "S1_TX_I2S_SWITCH",  "ENABLE", "S1_TX_L_MUX" }, \
	{ "S1_TX_I2S_SWITCH",  "ENABLE", "S1_TX_R_MUX" }, \
	{ "IV_2PA_48K_SWITCH", "ENABLE", "S1_RX_INPUT" }, \
	{ "IV_4PA_SWITCH",     "ENABLE", "S1_RX_INPUT" }, \
	{ "S1_TX_TDM_SWITCH",  "ENABLE", "CODEC3_DN_L_PGA" }, \
	{ "S1_TX_TDM_SWITCH",  "ENABLE", "CODEC3_DN_R_PGA" }, \
	{ "S1_TX_TDM_SWITCH",  "ENABLE", "S1_TDM_TX_L_MUX" }, \
	{ "S1_TX_TDM_SWITCH",  "ENABLE", "S1_TDM_TX_R_MUX" }, \
	{ "S2_TX_L_SWITCH",    "ENABLE", "BT_TX_SRCDN_MUX" }, \
	{ "S2_TX_R_SWITCH",    "ENABLE", "S2_TX_R_SEL_MUX" }, \
	{ "S2_RX_L_SWITCH",    "ENABLE", "S2_RX_INPUT" }, \
	{ "S2_RX_R_SWITCH",    "ENABLE", "S2_RX_INPUT" }, \
	{ "S4_TX_SWITCH",      "ENABLE", "MDM5G_DN_INPUT" }, \
	{ "MIC34_BT_O_SWITCH", "ENABLE", "MIC34_BT_INPUT" } \

#define UP_OUT_ROUTE \
	{ "ULTR_UP_OUT_DRV",        NULL, "ULTR_UP_O_SWITCH" }, \
	{ "MIC3_OUTPUT",            NULL, "MIC3_32K_SWITCH" }, \
	{ "MIC3_OUTPUT",            NULL, "MIC3_48K_SWITCH" }, \
	{ "MIC4_OUTPUT",            NULL, "MIC4_32K_SWITCH" }, \
	{ "MIC4_OUTPUT",            NULL, "MIC4_48K_SWITCH" }, \
	{ "VOICE_UP_OUTPUT",        NULL, "VOICE_UP_L_16K_SWITCH" }, \
	{ "VOICE_UP_OUTPUT",        NULL, "VOICE_UP_L_32K_SWITCH" }, \
	{ "VOICE_UP_OUTPUT",        NULL, "VOICE_UP_L_48K_SWITCH" }, \
	{ "VOICE_UP_OUTPUT",        NULL, "VOICE_UP_R_16K_SWITCH" }, \
	{ "VOICE_UP_OUTPUT",        NULL, "VOICE_UP_R_32K_SWITCH" }, \
	{ "VOICE_UP_OUTPUT",        NULL, "VOICE_UP_R_48K_SWITCH" }, \
	{ "AUDIO_FIFO_OUTPUT",      NULL, "AUDIO_UP_L_O_SWITCH" }, \
	{ "AUDIO_FIFO_OUTPUT",      NULL, "AUDIO_UP_R_O_SWITCH" }, \
	{ "MIC3_FIFO_OUTPUT",       NULL, "MIC3_O_SWITCH" }, \
	{ "MIC4_FIFO_OUTPUT",       NULL, "MIC4_O_SWITCH" }, \
	{ "MIC3_FIFO_OUTPUT",       NULL, "MIC34_96K_SWITCH" }, \
	{ "MIC4_FIFO_OUTPUT",       NULL, "MIC34_96K_SWITCH" }, \
	{ "VOICE_FIFO_OUTPUT",      NULL, "VOICE_UP_L_O_SWITCH" }, \
	{ "VOICE_FIFO_OUTPUT",      NULL, "VOICE_UP_R_O_SWITCH" }, \
	{ "AUDIO_MIC_OUTPUT",       NULL, "AUDIO_2MIC_O_SWITCH" }, \
	{ "AUDIO_MIC_OUTPUT",       NULL, "AUDIO_4MIC_O_SWITCH" }, \
	{ "VOICE_MIC_OUTPUT",       NULL, "VOICE_2MIC_O_SWITCH" }, \
	{ "VOICE_MIC_OUTPUT",       NULL, "VOICE_4MIC_O_SWITCH" }, \
	{ "MDM5G_UP_OUTPUT",        NULL, "MDM5G_UP_L_96K_SWITCH" }, \
	{ "MDM5G_UP_OUTPUT",        NULL, "MDM5G_UP_L_48K_SWITCH" }, \
	{ "MDM5G_UP_OUTPUT",        NULL, "MDM5G_UP_R_96K_SWITCH" }, \
	{ "MDM5G_UP_OUTPUT",        NULL, "MDM5G_UP_R_48K_SWITCH" }, \
	{ "ULTR_UP_OUTPUT",         NULL, "ULTR_UP_OUT_DRV" }, \
	{ "SMARTPA_UP_FIFO_OUTPUT", NULL, "IV_2PA_48K_SWITCH" }, \
	{ "SMARTPA_UP_FIFO_OUTPUT", NULL, "IV_4PA_SWITCH" }, \
	{ "MIC34_BT_OUTPUT",        NULL, "MIC34_BT_O_SWITCH" } \

#define DN_OUT_ROUTE \
	{ "AUDIO_OUTPUT",  NULL, "AUDIO_DN_48K_SWITCH" }, \
	{ "CODEC3_OUTPUT", NULL, "CODEC3_DN_44K1_SWITCH" }, \
	{ "CODEC3_OUTPUT", NULL, "CODEC3_DN_48K_SWITCH" }, \
	{ "CODEC3_OUTPUT", NULL, "CODEC3_DN_96K_SWITCH" }, \
	{ "CODEC3_OUTPUT", NULL, "CODEC3_DN_192K_SWITCH" }, \
	{ "CARKIT_OUTPUT", NULL, "CARKIT_48K_SWITCH" }, \
	{ "AU_INPUT",      NULL, "BM_DACL_AU_MUX" } \

#define I2S_OUT_ROUTE \
	{ "S1_TX_DRV",        NULL, "S1_TX_I2S_SWITCH" }, \
	{ "S1_TX_DRV",        NULL, "S1_TX_TDM_SWITCH" }, \
	{ "S2_TX_DRV",        NULL, "S2_TX_L_SWITCH" }, \
	{ "S2_TX_DRV",        NULL, "S2_TX_R_SWITCH" }, \
	{ "S3_TX_DRV",        NULL, "S3_TX_SWITCH" }, \
	{ "S1_TX_OUTPUT",     NULL, "S1_TX_DRV" }, \
	{ "S2_TX_OUTPUT",     NULL, "S2_TX_DRV" }, \
	{ "S3_SAMPLE_OUTPUT", NULL, "S3_44K1_SWITCH" }, \
	{ "S3_SAMPLE_OUTPUT", NULL, "S3_48K_SWITCH" }, \
	{ "S3_SAMPLE_OUTPUT", NULL, "S3_96K_SWITCH" }, \
	{ "S3_SAMPLE_OUTPUT", NULL, "S3_192K_SWITCH" }, \
	{ "S4_TX_DRV",        NULL, "S4_TX_SWITCH" }, \
	{ "S3_TX_OUTPUT",     NULL, "S3_TX_DRV" }, \
	{ "S3_TX_OUTPUT",     NULL, "S3_SUPPLY" }, \
	{ "S4_TX_OUTPUT",     NULL, "S4_TX_DRV" }, \
	{ "S4_TX_OUTPUT",     NULL, "S4_SUPPLY" } \

static const struct snd_soc_dapm_route route_map[] = {
	SUPPLY_ROUTE,
	DN_PGA_ROUTE,
	ADC_MUX_ROUTE,
	ADC_ROUTE,
	UP_MUX_ROUTE,
	UP_SWITCH_ROUTE,
	DN_MIXER_ROUTE,
	DN_MUX_ROUTE,
	DN_SWITCH_ROUTE,
	I2S_SWITCH_ROUTE,
	UP_OUT_ROUTE,
	DN_OUT_ROUTE,
	I2S_OUT_ROUTE,
};

int asp_codec_add_routes(struct snd_soc_dapm_context *dapm)
{
	if (dapm == NULL) {
		AUDIO_LOGE("input dapm is null");
		return -EINVAL;
	}

	return snd_soc_dapm_add_routes(dapm, route_map, ARRAY_SIZE(route_map));
}

