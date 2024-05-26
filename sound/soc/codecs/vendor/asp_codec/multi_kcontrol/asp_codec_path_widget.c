/*
 * asp_codec_path_widget.c -- asp codec path widget define
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

#include"asp_codec_path_widget.h"

#include "audio_log.h"
#include "asp_codec_regs.h"
#include "asp_codec.h"

#ifdef CONFIG_AUDIO_DEBUG
#define LOG_TAG "asp_codec"
#else
#define LOG_TAG "Analog_less_v1"
#endif

#define INPUT_WIDGET \
	SND_SOC_DAPM_INPUT("AUDIO_DN_FIFO_INPUT"), \
	SND_SOC_DAPM_INPUT("AUDIO_DN_INPUT"), \
	SND_SOC_DAPM_INPUT("CODEC3_DN_FIFO_INPUT"), \
	SND_SOC_DAPM_INPUT("CODEC3_DN_INPUT"), \
	SND_SOC_DAPM_INPUT("ULTR_DN_INPUT"), \
	SND_SOC_DAPM_INPUT("PDM_MIC12_INPUT"), \
	SND_SOC_DAPM_INPUT("PDM_MIC34_INPUT"), \
	SND_SOC_DAPM_INPUT("MIC3_INPUT"), \
	SND_SOC_DAPM_INPUT("MIC4_INPUT"), \
	SND_SOC_DAPM_INPUT("VOICE_UP_INPUT"), \
	SND_SOC_DAPM_INPUT("AU_INPUT"), \
	SND_SOC_DAPM_INPUT("MDM5G_DN_INPUT"), \
	SND_SOC_DAPM_INPUT("S1_RX_INPUT"), \
	SND_SOC_DAPM_INPUT("S2_RX_INPUT"), \
	SND_SOC_DAPM_INPUT("S3_RX_INPUT"), \
	SND_SOC_DAPM_INPUT("S3_SAMPLE_INPUT"), \
	SND_SOC_DAPM_INPUT("S4_RX_INPUT"), \
	SND_SOC_DAPM_INPUT("CARKIT_INPUT"), \
	SND_SOC_DAPM_INPUT("MIC34_BT_INPUT"), \
	SND_SOC_DAPM_INPUT("AUDIO_MIC_INPUT"), \
	SND_SOC_DAPM_INPUT("VOICE_MIC_INPUT") \

#define OUTPUT_WIDGET \
	SND_SOC_DAPM_OUTPUT("AUDIO_FIFO_OUTPUT"), \
	SND_SOC_DAPM_OUTPUT("AUDIO_OUTPUT"), \
	SND_SOC_DAPM_OUTPUT("CODEC3_OUTPUT"), \
	SND_SOC_DAPM_OUTPUT("MIC3_OUTPUT"), \
	SND_SOC_DAPM_OUTPUT("MIC3_FIFO_OUTPUT"), \
	SND_SOC_DAPM_OUTPUT("MIC4_OUTPUT"), \
	SND_SOC_DAPM_OUTPUT("MIC4_FIFO_OUTPUT"), \
	SND_SOC_DAPM_OUTPUT("VOICE_UP_OUTPUT"), \
	SND_SOC_DAPM_OUTPUT("VOICE_FIFO_OUTPUT"), \
	SND_SOC_DAPM_OUTPUT("MDM5G_UP_OUTPUT"), \
	SND_SOC_DAPM_OUTPUT("ULTR_UP_OUTPUT"), \
	SND_SOC_DAPM_OUTPUT("S1_TX_OUTPUT"), \
	SND_SOC_DAPM_OUTPUT("S2_TX_OUTPUT"), \
	SND_SOC_DAPM_OUTPUT("S3_TX_OUTPUT"), \
	SND_SOC_DAPM_OUTPUT("S3_SAMPLE_OUTPUT"), \
	SND_SOC_DAPM_OUTPUT("S4_TX_OUTPUT"), \
	SND_SOC_DAPM_OUTPUT("SMARTPA_UP_FIFO_OUTPUT"), \
	SND_SOC_DAPM_OUTPUT("CARKIT_OUTPUT"), \
	SND_SOC_DAPM_OUTPUT("MIC34_BT_OUTPUT"), \
	SND_SOC_DAPM_OUTPUT("AUDIO_MIC_OUTPUT"), \
	SND_SOC_DAPM_OUTPUT("VOICE_MIC_OUTPUT") \

#define OUT_DRV_WIDGET \
	SND_SOC_DAPM_OUT_DRV("ULTR_UP_OUT_DRV", \
		CODEC_CLK_EN0_REG, INFR_L_UP_AFIFO_CLKEN_OFFSET, 0, NULL, 0), \
	SND_SOC_DAPM_OUT_DRV("S1_TX_DRV", \
		I2S1_CTRL_REG, I2S1_IF_TX_ENA_OFFSET, 0, NULL, 0), \
	SND_SOC_DAPM_OUT_DRV("S2_TX_DRV", \
		I2S2_PCM_CTRL_REG, I2S2_IF_TX_ENA_OFFSET, 0, NULL, 0), \
	SND_SOC_DAPM_OUT_DRV("S3_TX_DRV", \
		I2S3_PCM_CTRL_REG, I2S3_IF_TX_ENA_OFFSET, 0, NULL, 0), \
	SND_SOC_DAPM_OUT_DRV("S4_TX_DRV", \
		I2S4_CTRL_REG, I2S4_IF_TX_ENA_OFFSET, 0, NULL, 0) \

/* mixer widget */
static const struct snd_kcontrol_new dac_l_mixer4_controls[] = {
	SOC_DAPM_SINGLE("I2S2_RX_L", DACL_MIXER4_CTRL0_REG,
		DACL_MIXER4_IN1_MUTE_OFFSET, 1, 1),
	SOC_DAPM_SINGLE("CODEC3_DN_L", DACL_MIXER4_CTRL0_REG,
		DACL_MIXER4_IN2_MUTE_OFFSET, 1, 1),
	SOC_DAPM_SINGLE("AUDIO_DN_L", DACL_MIXER4_CTRL0_REG,
		DACL_MIXER4_IN3_MUTE_OFFSET, 1, 1),
	SOC_DAPM_SINGLE("SIDETONE", DACL_MIXER4_CTRL0_REG,
		DACL_MIXER4_IN4_MUTE_OFFSET, 1, 1),
};

static const struct snd_kcontrol_new dac_r_mixer4_controls[] = {
	SOC_DAPM_SINGLE("I2S2_RX_R", DACR_MIXER4_CTRL0_REG,
		DACR_MIXER4_IN1_MUTE_OFFSET, 1, 1),
	SOC_DAPM_SINGLE("CODEC3_DN_R", DACR_MIXER4_CTRL0_REG,
		DACR_MIXER4_IN2_MUTE_OFFSET, 1, 1),
	SOC_DAPM_SINGLE("AUDIO_DN_R", DACR_MIXER4_CTRL0_REG,
		DACR_MIXER4_IN3_MUTE_OFFSET, 1, 1),
	SOC_DAPM_SINGLE("SIDETONE", DACR_MIXER4_CTRL0_REG,
		DACR_MIXER4_IN4_MUTE_OFFSET, 1, 1),
};

static const struct snd_kcontrol_new dacl_s_mixer2_controls[] = {
	SOC_DAPM_SINGLE("MIXER4_SRC_L", DACL_S_MIXER2_CTRL_REG,
		DACL_S_MIXER2_IN1_MUTE_OFFSET, 1, 1),
	SOC_DAPM_SINGLE("ULTR_DN_L", DACL_S_MIXER2_CTRL_REG,
		DACL_S_MIXER2_IN2_MUTE_OFFSET, 1, 1),
};

static const struct snd_kcontrol_new dacr_s_mixer2_controls[] = {
	SOC_DAPM_SINGLE("MIXER4_SRC_R", DACR_S_MIXER2_CTRL_REG,
		DACR_S_MIXER2_IN1_MUTE_OFFSET, 1, 1),
	SOC_DAPM_SINGLE("ULTR_DN_R", DACR_S_MIXER2_CTRL_REG,
		DACR_S_MIXER2_IN2_MUTE_OFFSET, 1, 1),
};

static const struct snd_kcontrol_new i2s2_tx_mixer2_controls[] = {
	SOC_DAPM_SINGLE("MIXER4_L", I2S2_TX_MIXER2_CTRL_REG,
		I2S2_TX_MIXER2_IN1_MUTE_OFFSET, 1, 1),
	SOC_DAPM_SINGLE("MIXER4_R", I2S2_TX_MIXER2_CTRL_REG,
		I2S2_TX_MIXER2_IN2_MUTE_OFFSET, 1, 1),
};

#define MIXER_WIDGET \
	SND_SOC_DAPM_MIXER("DAC_L_MIXER4", CODEC_CLK_EN2_REG, \
		DACL_MIXER4_CLKEN_OFFSET, 0, dac_l_mixer4_controls, \
		ARRAY_SIZE(dac_l_mixer4_controls)), \
	SND_SOC_DAPM_MIXER("DAC_R_MIXER4", CODEC_CLK_EN2_REG, \
		DACR_MIXER4_CLKEN_OFFSET, 0, dac_r_mixer4_controls, \
		ARRAY_SIZE(dac_r_mixer4_controls)), \
	SND_SOC_DAPM_MIXER("DACL_S_MIXER2", CODEC_CLK_EN2_REG, \
		DACL_S_MIXER2_CLKEN_OFFSET, 0, dacl_s_mixer2_controls, \
		ARRAY_SIZE(dacl_s_mixer2_controls)), \
	SND_SOC_DAPM_MIXER("DACR_S_MIXER2", CODEC_CLK_EN2_REG, \
		DACR_S_MIXER2_CLKEN_OFFSET, 0, dacr_s_mixer2_controls, \
		ARRAY_SIZE(dacr_s_mixer2_controls)), \
	SND_SOC_DAPM_MIXER("I2S2_TX_MIXER2", CODEC_CLK_EN2_REG, \
		I2S2_TX_MIXER2_CLKEN_OFFSET, 0, i2s2_tx_mixer2_controls, \
		ARRAY_SIZE(i2s2_tx_mixer2_controls)) \

/* mux widget */
static const char * const au_up_l_mux_texts[] = {
	"MIC1_ADC",
	"MIC2_ADC",
	"I2S2_RX_L",
	"MIXER4_L",
	"MIXER2_S_L",
	"AUDIO_DN_L",
};

static const struct soc_enum au_up_l_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX0_REG, AU_UP_L_DIN_SEL_OFFSET,
		ARRAY_SIZE(au_up_l_mux_texts), au_up_l_mux_texts);
static const struct snd_kcontrol_new dapm_au_up_l_mux_controls =
	SOC_DAPM_ENUM("Mux", au_up_l_mux_enum);

static const char * const au_up_r_mux_texts[] = {
	"MIC1_ADC",
	"MIC2_ADC",
	"I2S2_RX_R",
	"MIXER4_R",
	"MIXER2_S_R",
	"AUDIO_DN_R",
};

static const struct soc_enum au_up_r_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX0_REG, AU_UP_R_DIN_SEL_OFFSET,
		ARRAY_SIZE(au_up_r_mux_texts), au_up_r_mux_texts);

static const struct snd_kcontrol_new dapm_au_up_r_mux_controls =
	SOC_DAPM_ENUM("Mux", au_up_r_mux_enum);

static const char * const mic3_mux_texts[] = {
	"MIC3_ADC",
	"MIC4_ADC",
	"I2S3_RX_L",
};

static const struct soc_enum mic3_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX0_REG, MIC3_DIN_SEL_OFFSET,
		ARRAY_SIZE(mic3_mux_texts), mic3_mux_texts);

static const struct snd_kcontrol_new dapm_mic3_mux_controls =
	SOC_DAPM_ENUM("Mux", mic3_mux_enum);

static const char * const mic4_mux_texts[] = {
	"MIC3_ADC",
	"MIC4_ADC",
	"I2S3_RX_R",
};

static const struct soc_enum mic4_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX0_REG, MIC4_DIN_SEL_OFFSET,
		ARRAY_SIZE(mic4_mux_texts), mic4_mux_texts);

static const struct snd_kcontrol_new dapm_mic4_mux_controls =
	SOC_DAPM_ENUM("Mux", mic4_mux_enum);

static const char * const mic5_mux_texts[] = {
	"MIC1_ADC",
	"MIC2_ADC",
	"MIC3_ADC",
	"MIC4_ADC",
	"MIC5_ADC",
	"DAC_M_L",
	"DAC_M_R",
	"SIDETONE",
};

static const struct soc_enum mic5_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX0_REG, MIC5_DIN_SEL_OFFSET,
		ARRAY_SIZE(mic5_mux_texts), mic5_mux_texts);

static const struct snd_kcontrol_new dapm_mic5_mux_controls =
	SOC_DAPM_ENUM("Mux", mic5_mux_enum);

static const char * const vo_up_l_mux_texts[] = {
	"MIC1_ADC",
	"MIC2_ADC",
	"I2S2_RX_L",
	"MIXER4_L",
	"RESERVED",
	"CODEC3_DN_L",
};

static const struct soc_enum vo_up_l_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX0_REG, VO_UP_L_DIN_SEL_OFFSET,
		ARRAY_SIZE(vo_up_l_mux_texts), vo_up_l_mux_texts);

static const struct snd_kcontrol_new dapm_vo_up_l_mux_controls =
	SOC_DAPM_ENUM("Mux", vo_up_l_mux_enum);

static const char * const vo_up_r_mux_texts[] = {
	"MIC1_ADC",
	"MIC2_ADC",
	"I2S2_RX_R",
	"MIXER4_R",
	"RESERVED",
	"CODEC3_DN_R",
};
static const struct soc_enum vo_up_r_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX0_REG, VO_UP_R_DIN_SEL_OFFSET,
		ARRAY_SIZE(vo_up_r_mux_texts), vo_up_r_mux_texts);

static const struct snd_kcontrol_new dapm_vo_up_r_mux_controls =
	SOC_DAPM_ENUM("Mux", vo_up_r_mux_enum);

static const char * const adc1_mux_texts[] = {
	"AU",
	"DMIC1",
};

static const struct soc_enum adc1_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX0_REG, ADC1_DIN_SEL_OFFSET,
		ARRAY_SIZE(adc1_mux_texts), adc1_mux_texts);

static const struct snd_kcontrol_new dapm_adc1_mux_controls =
	SOC_DAPM_ENUM("Mux", adc1_mux_enum);

static const char * const adc2_mux_texts[] = {
	"AU",
	"DMIC1",
};

static const struct soc_enum adc2_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX0_REG, ADC2_DIN_SEL_OFFSET,
		ARRAY_SIZE(adc2_mux_texts), adc2_mux_texts);

static const struct snd_kcontrol_new dapm_adc2_mux_controls =
	SOC_DAPM_ENUM("Mux", adc2_mux_enum);

static const char * const adc3_mux_texts[] = {
	"AU",
	"DMIC2",
};

static const struct soc_enum adc3_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX0_REG, ADC3_DIN_SEL_OFFSET,
		ARRAY_SIZE(adc3_mux_texts), adc3_mux_texts);

static const struct snd_kcontrol_new dapm_adc3_mux_controls =
	SOC_DAPM_ENUM("Mux", adc3_mux_enum);

static const char * const adc4_mux_texts[] = {
	"AU",
	"DMIC2",
};

static const struct soc_enum adc4_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX0_REG, ADC4_DIN_SEL_OFFSET,
		ARRAY_SIZE(adc4_mux_texts), adc4_mux_texts);

static const struct snd_kcontrol_new dapm_adc4_mux_controls =
	SOC_DAPM_ENUM("Mux", adc4_mux_enum);

static const char * const mdm5g_up_l_mux_texts[] = {
	"MIXER4_L",
	"DAC_M_L",
	"CODEC3_DN_L",
	"MIXER2_S_L",
	"ULTR_DN_L",
	"I2S3_RX_L",
};

static const struct soc_enum mdm5g_up_l_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX1_REG, MDM_5G_UP_L_DIN_SEL_OFFSET,
		ARRAY_SIZE(mdm5g_up_l_mux_texts), mdm5g_up_l_mux_texts);

static const struct snd_kcontrol_new dapm_mdm5g_up_l_mux_controls =
	SOC_DAPM_ENUM("Mux", mdm5g_up_l_mux_enum);

static const char * const mdm5g_up_fifol_mux_texts[] = {
	"MDM5G_UP_L",
	"S4_RX_L",
};

static const struct soc_enum mdm5g_up_fifol_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX1_REG, MDM_5G_L_AFIFO_DIN_SEL_OFFSET,
		ARRAY_SIZE(mdm5g_up_fifol_mux_texts), mdm5g_up_fifol_mux_texts);

static const struct snd_kcontrol_new dapm_mdm5g_up_fifol_mux_controls =
	SOC_DAPM_ENUM("Mux", mdm5g_up_fifol_mux_enum);

static const char * const mdm5g_up_r_mux_texts[] = {
	"MIXER4_R",
	"DAC_M_R",
	"CODEC3_DN_R",
	"MIXER2_S_R",
	"ULTR_DN_R",
	"I2S3_RX_R",
};

static const struct soc_enum mdm5g_up_r_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX1_REG, MDM_5G_UP_R_DIN_SEL_OFFSET,
		ARRAY_SIZE(mdm5g_up_r_mux_texts), mdm5g_up_r_mux_texts);

static const struct snd_kcontrol_new dapm_mdm5g_up_r_mux_controls =
	SOC_DAPM_ENUM("Mux", mdm5g_up_r_mux_enum);

static const char * const mdm5g_up_fifor_mux_texts[] = {
	"MDM5G_UP_R",
	"S4_RX_R",
};

static const struct soc_enum mdm5g_up_fifor_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX1_REG, MDM_5G_R_AFIFO_DIN_SEL_OFFSET,
		ARRAY_SIZE(mdm5g_up_fifor_mux_texts), mdm5g_up_fifor_mux_texts);

static const struct snd_kcontrol_new dapm_mdm5g_up_fifor_mux_controls =
	SOC_DAPM_ENUM("Mux", mdm5g_up_fifor_mux_enum);

static const char * const i2s1_tx_l_mux_texts[] = {
	"AU_UP_L",
	"MIXER2_S_L",
	"MIXER4_SRC_L",
	"MAD_TO_CODEC_DATA",
};

static const struct soc_enum i2s1_tx_l_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX0_REG, I2S1_TX_L_SEL_OFFSET,
		ARRAY_SIZE(i2s1_tx_l_mux_texts), i2s1_tx_l_mux_texts);

static const struct snd_kcontrol_new dapm_i2s1_tx_l_mux_controls =
	SOC_DAPM_ENUM("Mux", i2s1_tx_l_mux_enum);

static const char * const i2s1_tx_r_mux_texts[] = {
	"AU_UP_R",
	"MIXER2_S_R",
	"MIXER4_SRC_R",
};

static const struct soc_enum i2s1_tx_r_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX0_REG, I2S1_TX_R_SEL_OFFSET,
		ARRAY_SIZE(i2s1_tx_r_mux_texts), i2s1_tx_r_mux_texts);

static const struct snd_kcontrol_new dapm_i2s1_tx_r_mux_controls =
	SOC_DAPM_ENUM("Mux", i2s1_tx_r_mux_enum);

static const char * const S1_TDM_TX_L_MUX_texts[] = {
	"AUDIO_DN_L",
	"MIXER4_SRC_L",
};

static const struct soc_enum S1_TDM_TX_L_MUX_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX1_REG, BM_I2S1_TDM_TX_L_DIN_SEL_OFFSET,
		ARRAY_SIZE(S1_TDM_TX_L_MUX_texts), S1_TDM_TX_L_MUX_texts);

static const struct snd_kcontrol_new dapm_S1_TDM_TX_L_MUX_controls =
	SOC_DAPM_ENUM("Mux", S1_TDM_TX_L_MUX_enum);

static const char * const S1_TDM_TX_R_MUX_texts[] = {
	"AUDIO_DN_R",
	"MIXER4_SRC_R",
};

static const struct soc_enum S1_TDM_TX_R_MUX_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX1_REG, BM_I2S1_TDM_TX_R_DIN_SEL_OFFSET,
		ARRAY_SIZE(S1_TDM_TX_R_MUX_texts), S1_TDM_TX_R_MUX_texts);

static const struct snd_kcontrol_new dapm_S1_TDM_TX_R_MUX_controls =
	SOC_DAPM_ENUM("Mux", S1_TDM_TX_R_MUX_enum);

static const char * const i2s2_tx_r_sel_mux_texts[] = {
	"I2S2_TX_L_BTMATCH",
	"I2S2_TX_R_BTMATCH",
};

static const struct soc_enum i2s2_tx_r_sel_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX0_REG, I2S2_TX_R_SEL_OFFSET,
		ARRAY_SIZE(i2s2_tx_r_sel_mux_texts), i2s2_tx_r_sel_mux_texts);

static const struct snd_kcontrol_new dapm_i2s2_tx_r_sel_mux_controls =
	SOC_DAPM_ENUM("Mux", i2s2_tx_r_sel_mux_enum);

static const char * const bt_tx_srcdn_mux_texts[] = {
	"I2S2_MIXER2",
	"MIXER4_L",
};

static const struct soc_enum bt_tx_srcdn_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX1_REG, BT_TX_SRCDN_DIN_SEL_OFFSET,
		ARRAY_SIZE(bt_tx_srcdn_mux_texts), bt_tx_srcdn_mux_texts);

static const struct snd_kcontrol_new dapm_bt_tx_srcdn_mux_controls =
	SOC_DAPM_ENUM("Mux", bt_tx_srcdn_mux_enum);

static const char * const bm_dacl_au_mux_texts[] = {
	"DAC_M_L",
	"ULTR_DN_L",
};
static const struct soc_enum bm_dacl_au_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX1_REG, BM_DACL_AU_DIN_SEL_OFFSET,
		ARRAY_SIZE(bm_dacl_au_mux_texts), bm_dacl_au_mux_texts);
static const struct snd_kcontrol_new dapm_bm_dacl_au_mux_controls =
	SOC_DAPM_ENUM("Mux", bm_dacl_au_mux_enum);

static const char * const bm_dacr_au_mux_texts[] = {
	"DAC_M_R",
	"ULTR_DN_R",
};
static const struct soc_enum bm_dacr_au_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX1_REG, BM_DACR_AU_DIN_SEL_OFFSET,
		ARRAY_SIZE(bm_dacr_au_mux_texts), bm_dacr_au_mux_texts);
static const struct snd_kcontrol_new dapm_bm_dacr_au_mux_controls =
	SOC_DAPM_ENUM("Mux", bm_dacr_au_mux_enum);

static const char * const sidetone_pga_mux_texts[] = {
	"MIC1_ADC",
	"MIC2_ADC",
	"MIC3_ADC",
	"MIC4_ADC",
	"MIC5_ADC",
};
static const struct soc_enum sidetone_pga_mux_enum =
	SOC_ENUM_SINGLE(CODEC_DIN_MUX0_REG, SIDETONE_PGA_DIN_SEL_OFFSET,
		ARRAY_SIZE(sidetone_pga_mux_texts), sidetone_pga_mux_texts);
static const struct snd_kcontrol_new dapm_sidetone_pga_mux_controls =
	SOC_DAPM_ENUM("Mux", sidetone_pga_mux_enum);

#define MUX_WIDGET \
	SND_SOC_DAPM_MUX("AUDIO_UP_L_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_au_up_l_mux_controls), \
	SND_SOC_DAPM_MUX("AUDIO_UP_R_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_au_up_r_mux_controls), \
	SND_SOC_DAPM_MUX("MIC3_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_mic3_mux_controls), \
	SND_SOC_DAPM_MUX("MIC4_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_mic4_mux_controls), \
	SND_SOC_DAPM_MUX("MIC5_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_mic5_mux_controls), \
	SND_SOC_DAPM_MUX("VOICE_UP_L_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_vo_up_l_mux_controls), \
	SND_SOC_DAPM_MUX("VOICE_UP_R_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_vo_up_r_mux_controls), \
	SND_SOC_DAPM_MUX("ADC1_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_adc1_mux_controls), \
	SND_SOC_DAPM_MUX("ADC2_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_adc2_mux_controls), \
	SND_SOC_DAPM_MUX("ADC3_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_adc3_mux_controls), \
	SND_SOC_DAPM_MUX("ADC4_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_adc4_mux_controls), \
	SND_SOC_DAPM_MUX("MDM5G_UP_L_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_mdm5g_up_l_mux_controls), \
	SND_SOC_DAPM_MUX("MDM5G_UP_FIFOL_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_mdm5g_up_fifol_mux_controls), \
	SND_SOC_DAPM_MUX("MDM5G_UP_R_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_mdm5g_up_r_mux_controls), \
	SND_SOC_DAPM_MUX("MDM5G_UP_FIFOR_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_mdm5g_up_fifor_mux_controls), \
	SND_SOC_DAPM_MUX("S1_TX_L_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_i2s1_tx_l_mux_controls), \
	SND_SOC_DAPM_MUX("S1_TX_R_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_i2s1_tx_r_mux_controls), \
	SND_SOC_DAPM_MUX("S1_TDM_TX_L_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_S1_TDM_TX_L_MUX_controls), \
	SND_SOC_DAPM_MUX("S1_TDM_TX_R_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_S1_TDM_TX_R_MUX_controls), \
	SND_SOC_DAPM_MUX("S2_TX_R_SEL_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_i2s2_tx_r_sel_mux_controls), \
	SND_SOC_DAPM_MUX("BT_TX_SRCDN_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_bt_tx_srcdn_mux_controls), \
	SND_SOC_DAPM_MUX("BM_DACL_AU_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_bm_dacl_au_mux_controls), \
	SND_SOC_DAPM_MUX("BM_DACR_AU_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_bm_dacr_au_mux_controls), \
	SND_SOC_DAPM_MUX("SIDETONE_PGA_MUX", \
		SND_SOC_NOPM, 0, 0, &dapm_sidetone_pga_mux_controls) \

static const struct snd_soc_dapm_widget path_widgets[] = {
	INPUT_WIDGET,
	OUTPUT_WIDGET,
	MIXER_WIDGET,
	MUX_WIDGET,
	OUT_DRV_WIDGET,
};

int asp_codec_add_path_widgets(struct snd_soc_dapm_context *dapm)
{
	if (dapm == NULL) {
		AUDIO_LOGE("input dapm is null");
		return -EINVAL;
	}

	return snd_soc_dapm_new_controls(dapm, path_widgets,
		ARRAY_SIZE(path_widgets));
}

