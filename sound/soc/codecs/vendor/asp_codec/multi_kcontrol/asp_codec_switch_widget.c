/*
 * asp_codec_switch_widget.c -- asp codec switch widget driver
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

#include "asp_codec_switch_widget.h"

#include <linux/delay.h>

#include "audio_log.h"
#include "asp_codec_regs.h"
#include "asp_codec.h"

#ifdef CONFIG_AUDIO_DEBUG
#define LOG_TAG "asp_codec"
#else
#define LOG_TAG "Analog_less_v1"
#endif

/*lint -e570*/
static void vo_up_l_set_default_config(void)
{
	asp_codec_reg_update(FS_CTRL4_REG,
		MAX_VAL_ON_BIT(2) << FS_VOICE_UP_L_SRCDN_DIN_OFFSET,
		0x1 << FS_VOICE_UP_L_SRCDN_DIN_OFFSET);
	asp_codec_reg_update(FS_CTRL4_REG,
		MAX_VAL_ON_BIT(3) << FS_VOICE_UP_L_SRCDN_DOUT_OFFSET,
		0x2 << FS_VOICE_UP_L_SRCDN_DOUT_OFFSET);
	asp_codec_reg_update(SRCDN_CTRL0_REG,
		MAX_VAL_ON_BIT(3) << VOICE_L_UP_SRCDN_SRC_MODE_OFFSET,
		0x0 << VOICE_L_UP_SRCDN_SRC_MODE_OFFSET);
	asp_codec_reg_update(FS_CTRL2_REG,
		MAX_VAL_ON_BIT(3) << FS_VOICE_L_UP_PGA_OFFSET,
		0x2 << FS_VOICE_L_UP_PGA_OFFSET);
	asp_codec_reg_update(FS_CTRL1_REG,
		MAX_VAL_ON_BIT(3) << FS_VOICE_L_UP_AFIFO_OFFSET,
		0x2 << FS_VOICE_L_UP_AFIFO_OFFSET);
}

static int vo_up_l_8k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_reg_update(FS_CTRL4_REG,
			MAX_VAL_ON_BIT(2) << FS_VOICE_UP_L_SRCDN_DIN_OFFSET,
			0x1 << FS_VOICE_UP_L_SRCDN_DIN_OFFSET);
		asp_codec_reg_update(FS_CTRL4_REG,
			MAX_VAL_ON_BIT(3) << FS_VOICE_UP_L_SRCDN_DOUT_OFFSET,
			0x0 << FS_VOICE_UP_L_SRCDN_DOUT_OFFSET);
		asp_codec_reg_update(SRCDN_CTRL0_REG,
			MAX_VAL_ON_BIT(3) << VOICE_L_UP_SRCDN_SRC_MODE_OFFSET,
			0x1 << VOICE_L_UP_SRCDN_SRC_MODE_OFFSET);
		asp_codec_reg_update(FS_CTRL2_REG,
			MAX_VAL_ON_BIT(3) << FS_VOICE_L_UP_PGA_OFFSET,
			0x0 << FS_VOICE_L_UP_PGA_OFFSET);
		asp_codec_reg_update(FS_CTRL1_REG,
			MAX_VAL_ON_BIT(3) << FS_VOICE_L_UP_AFIFO_OFFSET,
			0x0 << FS_VOICE_L_UP_AFIFO_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		vo_up_l_set_default_config();
		break;
	default:
		break;
	}

	return 0;
}

static int vo_up_l_16k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_reg_update(FS_CTRL4_REG,
			MAX_VAL_ON_BIT(2) << FS_VOICE_UP_L_SRCDN_DIN_OFFSET,
			0x1 << FS_VOICE_UP_L_SRCDN_DIN_OFFSET);
		asp_codec_reg_update(FS_CTRL4_REG,
			MAX_VAL_ON_BIT(3) << FS_VOICE_UP_L_SRCDN_DOUT_OFFSET,
			0x1 << FS_VOICE_UP_L_SRCDN_DOUT_OFFSET);
		asp_codec_reg_update(SRCDN_CTRL0_REG,
			MAX_VAL_ON_BIT(3) << VOICE_L_UP_SRCDN_SRC_MODE_OFFSET,
			0x2 << VOICE_L_UP_SRCDN_SRC_MODE_OFFSET);
		asp_codec_reg_update(FS_CTRL2_REG,
			MAX_VAL_ON_BIT(3) << FS_VOICE_L_UP_PGA_OFFSET,
			0x1 << FS_VOICE_L_UP_PGA_OFFSET);
		asp_codec_reg_update(FS_CTRL1_REG,
			MAX_VAL_ON_BIT(3) << FS_VOICE_L_UP_AFIFO_OFFSET,
			0x1 << FS_VOICE_L_UP_AFIFO_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		vo_up_l_set_default_config();
		break;
	default:
		break;
	}

	return 0;
}

static int vo_up_l_32k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
	case SND_SOC_DAPM_POST_PMD:
		/* default config is same as 32k config */
		vo_up_l_set_default_config();
		break;
	default:
		break;
	}

	return 0;
}

int vo_up_l_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_reg_update(FS_CTRL4_REG,
			MAX_VAL_ON_BIT(2) << FS_VOICE_UP_L_SRCDN_DIN_OFFSET,
			0x1 << FS_VOICE_UP_L_SRCDN_DIN_OFFSET);
		asp_codec_reg_update(FS_CTRL4_REG,
			MAX_VAL_ON_BIT(3) << FS_VOICE_UP_L_SRCDN_DOUT_OFFSET,
			0x3 << FS_VOICE_UP_L_SRCDN_DOUT_OFFSET);
		asp_codec_reg_update(SRCDN_CTRL0_REG,
			MAX_VAL_ON_BIT(3) << VOICE_L_UP_SRCDN_SRC_MODE_OFFSET,
			0x3 << VOICE_L_UP_SRCDN_SRC_MODE_OFFSET);
		asp_codec_reg_update(FS_CTRL2_REG,
			MAX_VAL_ON_BIT(3) << FS_VOICE_L_UP_PGA_OFFSET,
			0x3 << FS_VOICE_L_UP_PGA_OFFSET);
		asp_codec_reg_update(FS_CTRL1_REG,
			MAX_VAL_ON_BIT(3) << FS_VOICE_L_UP_AFIFO_OFFSET,
			0x3 << FS_VOICE_L_UP_AFIFO_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		vo_up_l_set_default_config();
		break;
	default:
		break;
	}

	return 0;
}

static void vo_up_r_set_default_config(void)
{
	asp_codec_reg_update(FS_CTRL4_REG,
		MAX_VAL_ON_BIT(2) << FS_VOICE_UP_R_SRCDN_DIN_OFFSET,
		0x1 << FS_VOICE_UP_R_SRCDN_DIN_OFFSET);
	asp_codec_reg_update(FS_CTRL4_REG,
		MAX_VAL_ON_BIT(3) << FS_VOICE_UP_R_SRCDN_DOUT_OFFSET,
		0x2 << FS_VOICE_UP_R_SRCDN_DOUT_OFFSET);
	asp_codec_reg_update(SRCDN_CTRL0_REG,
		MAX_VAL_ON_BIT(3) << VOICE_R_UP_SRCDN_SRC_MODE_OFFSET,
		0x0 << VOICE_R_UP_SRCDN_SRC_MODE_OFFSET);
	asp_codec_reg_update(FS_CTRL2_REG,
		MAX_VAL_ON_BIT(3) << FS_VOICE_R_UP_PGA_OFFSET,
		0x2 << FS_VOICE_R_UP_PGA_OFFSET);
	asp_codec_reg_update(FS_CTRL1_REG,
		MAX_VAL_ON_BIT(3) << FS_VOICE_R_UP_AFIFO_OFFSET,
		0x2 << FS_VOICE_R_UP_AFIFO_OFFSET);
}

/*lint -e648*/
static int vo_up_r_8k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_reg_update(FS_CTRL4_REG,
			MAX_VAL_ON_BIT(2) << FS_VOICE_UP_R_SRCDN_DIN_OFFSET,
			0x1 << FS_VOICE_UP_R_SRCDN_DIN_OFFSET);
		asp_codec_reg_update(FS_CTRL4_REG,
			MAX_VAL_ON_BIT(3) << FS_VOICE_UP_R_SRCDN_DOUT_OFFSET,
			0x0 << FS_VOICE_UP_R_SRCDN_DOUT_OFFSET);
		asp_codec_reg_update(SRCDN_CTRL0_REG,
			MAX_VAL_ON_BIT(3) << VOICE_R_UP_SRCDN_SRC_MODE_OFFSET,
			0x1 << VOICE_R_UP_SRCDN_SRC_MODE_OFFSET);
		asp_codec_reg_update(FS_CTRL2_REG,
			MAX_VAL_ON_BIT(3) << FS_VOICE_R_UP_PGA_OFFSET,
			0x0 << FS_VOICE_R_UP_PGA_OFFSET);
		asp_codec_reg_update(FS_CTRL1_REG,
			MAX_VAL_ON_BIT(3) << FS_VOICE_R_UP_AFIFO_OFFSET,
			0x0 << FS_VOICE_R_UP_AFIFO_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		vo_up_r_set_default_config();
		break;
	default:
		break;
	}

	return 0;
}

static int vo_up_r_16k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_reg_update(FS_CTRL4_REG,
			MAX_VAL_ON_BIT(2) << FS_VOICE_UP_R_SRCDN_DIN_OFFSET,
			0x1 << FS_VOICE_UP_R_SRCDN_DIN_OFFSET);
		asp_codec_reg_update(FS_CTRL4_REG,
			MAX_VAL_ON_BIT(3) << FS_VOICE_UP_R_SRCDN_DOUT_OFFSET,
			0x1 << FS_VOICE_UP_R_SRCDN_DOUT_OFFSET);
		asp_codec_reg_update(SRCDN_CTRL0_REG,
			MAX_VAL_ON_BIT(3) << VOICE_R_UP_SRCDN_SRC_MODE_OFFSET,
			0x2 << VOICE_R_UP_SRCDN_SRC_MODE_OFFSET);
		asp_codec_reg_update(FS_CTRL2_REG,
			MAX_VAL_ON_BIT(3) << FS_VOICE_R_UP_PGA_OFFSET,
			0x1 << FS_VOICE_R_UP_PGA_OFFSET);
		asp_codec_reg_update(FS_CTRL1_REG,
			MAX_VAL_ON_BIT(3) << FS_VOICE_R_UP_AFIFO_OFFSET,
			0x1 << FS_VOICE_R_UP_AFIFO_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		vo_up_r_set_default_config();
		break;
	default:
		break;
	}

	return 0;
}

static int vo_up_r_32k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
	case SND_SOC_DAPM_POST_PMD:
		/* default config is same as 32k config */
		vo_up_r_set_default_config();
		break;
	default:
		break;
	}

	return 0;
}

int vo_up_r_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_reg_update(FS_CTRL4_REG,
			MAX_VAL_ON_BIT(2) << FS_VOICE_UP_R_SRCDN_DIN_OFFSET,
			0x1 << FS_VOICE_UP_R_SRCDN_DIN_OFFSET);
		asp_codec_reg_update(FS_CTRL4_REG,
			MAX_VAL_ON_BIT(3) << FS_VOICE_UP_R_SRCDN_DOUT_OFFSET,
			0x3 << FS_VOICE_UP_R_SRCDN_DOUT_OFFSET);
		asp_codec_reg_update(SRCDN_CTRL0_REG,
			MAX_VAL_ON_BIT(3) << VOICE_R_UP_SRCDN_SRC_MODE_OFFSET,
			0x3 << VOICE_R_UP_SRCDN_SRC_MODE_OFFSET);
		asp_codec_reg_update(FS_CTRL2_REG,
			MAX_VAL_ON_BIT(3) << FS_VOICE_R_UP_PGA_OFFSET,
			0x3 << FS_VOICE_R_UP_PGA_OFFSET);
		asp_codec_reg_update(FS_CTRL1_REG,
			MAX_VAL_ON_BIT(3) << FS_VOICE_R_UP_AFIFO_OFFSET,
			0x3 << FS_VOICE_R_UP_AFIFO_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		vo_up_r_set_default_config();
		break;
	default:
		break;
	}

	return 0;
}

static void mic3_set_default_config(void)
{
	asp_codec_reg_update(FS_CTRL5_REG,
		MAX_VAL_ON_BIT(3) << FS_MIC3_SRCDN_DIN_OFFSET,
		0x4 << FS_MIC3_SRCDN_DIN_OFFSET);
	asp_codec_reg_update(FS_CTRL5_REG,
		MAX_VAL_ON_BIT(3) << FS_MIC3_SRCDN_DOUT_OFFSET,
		0x2 << FS_MIC3_SRCDN_DOUT_OFFSET);
	asp_codec_reg_update(SRCDN_CTRL0_REG,
		MAX_VAL_ON_BIT(3) << MIC3_UP_SRCDN_SRC_MODE_OFFSET,
		0x0 << MIC3_UP_SRCDN_SRC_MODE_OFFSET);
	asp_codec_reg_update(FS_CTRL2_REG,
		MAX_VAL_ON_BIT(3) << FS_MIC3_UP_PGA_OFFSET,
		0x2 << FS_MIC3_UP_PGA_OFFSET);
	asp_codec_reg_update(FS_CTRL1_REG,
		MAX_VAL_ON_BIT(3) << FS_MIC3_UP_AFIFO_OFFSET,
		0x2 << FS_MIC3_UP_AFIFO_OFFSET);
}

static int mic3_32k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
	case SND_SOC_DAPM_POST_PMD:
		/* default config is same as 32k config */
		mic3_set_default_config();
		break;
	default:
		break;
	}

	return 0;
}

int mic3_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_reg_update(FS_CTRL5_REG,
			MAX_VAL_ON_BIT(3) << FS_MIC3_SRCDN_DIN_OFFSET,
			0x4 << FS_MIC3_SRCDN_DIN_OFFSET);
		asp_codec_reg_update(FS_CTRL5_REG,
			MAX_VAL_ON_BIT(3) << FS_MIC3_SRCDN_DOUT_OFFSET,
			0x3 << FS_MIC3_SRCDN_DOUT_OFFSET);
		asp_codec_reg_update(SRCDN_CTRL0_REG,
			MAX_VAL_ON_BIT(3) << MIC3_UP_SRCDN_SRC_MODE_OFFSET,
			0x3 << MIC3_UP_SRCDN_SRC_MODE_OFFSET);
		asp_codec_reg_update(FS_CTRL2_REG,
			MAX_VAL_ON_BIT(3) << FS_MIC3_UP_PGA_OFFSET,
			0x3 << FS_MIC3_UP_PGA_OFFSET);
		asp_codec_reg_update(FS_CTRL1_REG,
			MAX_VAL_ON_BIT(3) << FS_MIC3_UP_AFIFO_OFFSET,
			0x3 << FS_MIC3_UP_AFIFO_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mic3_set_default_config();
		break;
	default:
		break;
	}

	return 0;
}

static void mic4_set_default_config(void)
{
	asp_codec_reg_update(FS_CTRL5_REG,
		MAX_VAL_ON_BIT(3) << FS_MIC4_SRCDN_DIN_OFFSET,
		0x4 << FS_MIC4_SRCDN_DIN_OFFSET);
	asp_codec_reg_update(FS_CTRL5_REG,
		MAX_VAL_ON_BIT(3) << FS_MIC4_SRCDN_DOUT_OFFSET,
		0x2 << FS_MIC4_SRCDN_DOUT_OFFSET);
	asp_codec_reg_update(SRCDN_CTRL0_REG,
		MAX_VAL_ON_BIT(3) << MIC4_UP_SRCDN_SRC_MODE_OFFSET,
		0x0 << MIC4_UP_SRCDN_SRC_MODE_OFFSET);
	asp_codec_reg_update(FS_CTRL2_REG,
		MAX_VAL_ON_BIT(3) << FS_MIC4_UP_PGA_OFFSET,
		0x2 << FS_MIC4_UP_PGA_OFFSET);
	asp_codec_reg_update(FS_CTRL1_REG,
		MAX_VAL_ON_BIT(3) << FS_MIC4_UP_AFIFO_OFFSET,
		0x2 << FS_MIC4_UP_AFIFO_OFFSET);
}

static int mic4_32k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
	case SND_SOC_DAPM_POST_PMD:
		/* default config is same as 32k config */
		mic4_set_default_config();
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

int mic4_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_reg_update(FS_CTRL5_REG,
			MAX_VAL_ON_BIT(3) << FS_MIC4_SRCDN_DIN_OFFSET,
			0x4 << FS_MIC4_SRCDN_DIN_OFFSET);
		asp_codec_reg_update(FS_CTRL5_REG,
			MAX_VAL_ON_BIT(3) << FS_MIC4_SRCDN_DOUT_OFFSET,
			0x3 << FS_MIC4_SRCDN_DOUT_OFFSET);
		asp_codec_reg_update(SRCDN_CTRL0_REG,
			MAX_VAL_ON_BIT(3) << MIC4_UP_SRCDN_SRC_MODE_OFFSET,
			0x3 << MIC4_UP_SRCDN_SRC_MODE_OFFSET);
		asp_codec_reg_update(FS_CTRL2_REG,
			MAX_VAL_ON_BIT(3) << FS_MIC4_UP_PGA_OFFSET,
			0x3 << FS_MIC4_UP_PGA_OFFSET);
		asp_codec_reg_update(FS_CTRL1_REG,
			MAX_VAL_ON_BIT(3) << FS_MIC4_UP_AFIFO_OFFSET,
			0x3 << FS_MIC4_UP_AFIFO_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mic4_set_default_config();
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int mic34_96k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_reg_update(FS_CTRL2_REG,
			MAX_VAL_ON_BIT(FS_MIC4_UP_PGA_LEN) << FS_MIC4_UP_PGA_OFFSET |
			MAX_VAL_ON_BIT(FS_MIC3_UP_PGA_LEN) << FS_MIC3_UP_PGA_OFFSET,
			0x4 << FS_MIC4_UP_PGA_OFFSET | 0x4 << FS_MIC3_UP_PGA_OFFSET);
		asp_codec_reg_update(FS_CTRL1_REG,
			MAX_VAL_ON_BIT(FS_MIC4_UP_AFIFO_LEN) << FS_MIC4_UP_AFIFO_OFFSET |
			MAX_VAL_ON_BIT(FS_MIC3_UP_AFIFO_LEN) << FS_MIC3_UP_AFIFO_OFFSET,
			0x4 << FS_MIC4_UP_AFIFO_OFFSET | 0x4 << FS_MIC3_UP_AFIFO_OFFSET);
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << MIC3_UP_AFIFO_CLKEN_OFFSET | 0x1 << MIC4_UP_AFIFO_CLKEN_OFFSET |
			0x1 << MIC3_UP_PGA_CLKEN_OFFSET);
		asp_codec_set_reg_bits(CODEC_CLK_EN1_REG, 0x1 << MIC4_UP_PGA_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << MIC3_UP_AFIFO_CLKEN_OFFSET | 0x1 << MIC4_UP_AFIFO_CLKEN_OFFSET |
			0x1 << MIC3_UP_PGA_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG, 0x1 << MIC4_UP_PGA_CLKEN_OFFSET);
		mic3_set_default_config();
		mic4_set_default_config();
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int s3_44k1_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_set_reg_bits(CODEC_SW_RST_N_REG,
			0x1 << SLV_MODE_441_OFFSET);
		asp_codec_reg_update(I2S3_PCM_CTRL_REG,
			MAX_VAL_ON_BIT(3) << FS_I2S3_OFFSET,
			0x4 << FS_I2S3_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		asp_codec_clr_reg_bits(CODEC_SW_RST_N_REG,
			0x1 << SLV_MODE_441_OFFSET);
		asp_codec_reg_update(I2S3_PCM_CTRL_REG,
			MAX_VAL_ON_BIT(3) << FS_I2S3_OFFSET,
			0x6 << FS_I2S3_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

int s3_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_reg_update(I2S3_PCM_CTRL_REG,
			MAX_VAL_ON_BIT(3) << FS_I2S3_OFFSET,
			0x4 << FS_I2S3_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		asp_codec_reg_update(I2S3_PCM_CTRL_REG,
			MAX_VAL_ON_BIT(3) << FS_I2S3_OFFSET,
			0x6 << FS_I2S3_OFFSET);
		break;
	default:
		AUDIO_LOGW("s3 48k power event err:%d", event);
		break;
	}

	return 0;
}

static int s3_96k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_reg_update(I2S3_PCM_CTRL_REG,
			MAX_VAL_ON_BIT(3) << FS_I2S3_OFFSET,
			0x3 << FS_I2S3_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		asp_codec_reg_update(I2S3_PCM_CTRL_REG,
			MAX_VAL_ON_BIT(3) << FS_I2S3_OFFSET,
			0x6 << FS_I2S3_OFFSET);
		break;
	default:
		AUDIO_LOGW("s3 96k power event err:%d", event);
		break;
	}

	return 0;
}

static int s3_192k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_reg_update(I2S3_PCM_CTRL_REG,
			MAX_VAL_ON_BIT(3) << FS_I2S3_OFFSET,
			0x5 << FS_I2S3_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
			asp_codec_reg_update(I2S3_PCM_CTRL_REG,
			MAX_VAL_ON_BIT(3) << FS_I2S3_OFFSET,
			0x6 << FS_I2S3_OFFSET);
		break;
	default:
		AUDIO_LOGW("s3 192k power event err:%d", event);
		break;
	}

	return 0;
}

static void audio_up_l_set_48k(void)
{
	asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << AUDIO_UP_L_PGA_CLKEN_OFFSET);
	asp_codec_set_reg_bits(CODEC_CLK_EN1_REG,
		0x1 << AUDIO_UP_L_SRCDN_CLKEN_OFFSET);
	asp_codec_reg_update(FS_CTRL4_REG,
		MAX_VAL_ON_BIT(2) << FS_AUDIO_UP_L_SRCDN_DIN_OFFSET,
		0x1 << FS_AUDIO_UP_L_SRCDN_DIN_OFFSET);
	asp_codec_clr_reg_bits(FS_CTRL4_REG,
		0x3 << FS_AUDIO_UP_L_SRCDN_DOUT_OFFSET);
	asp_codec_clr_reg_bits(FS_CTRL2_REG,
		0x3 << FS_AUDIO_L_UP_PGA_OFFSET);
	asp_codec_clr_reg_bits(FS_CTRL1_REG,
		0x3 << FS_AUDIO_L_UPLINK_OFFSET);
}

static void audio_up_r_set_48k(void)
{
	asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << AUDIO_UP_R_PGA_CLKEN_OFFSET);
	asp_codec_set_reg_bits(CODEC_CLK_EN1_REG,
		0x1 << AUDIO_UP_R_SRCDN_CLKEN_OFFSET);
	asp_codec_reg_update(FS_CTRL4_REG,
		MAX_VAL_ON_BIT(2) << FS_AUDIO_UP_R_SRCDN_DIN_OFFSET,
		0x1 << FS_AUDIO_UP_R_SRCDN_DIN_OFFSET);
	asp_codec_clr_reg_bits(FS_CTRL4_REG,
		0x3 << FS_AUDIO_UP_R_SRCDN_DOUT_OFFSET);
	asp_codec_clr_reg_bits(FS_CTRL2_REG,
		0x3 << FS_AUDIO_R_UP_PGA_OFFSET);
	asp_codec_clr_reg_bits(FS_CTRL1_REG,
		0x3 << FS_AUDIO_R_UPLINK_OFFSET);
}

static int audio_2mic_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *codec = asp_codec_get_codec();
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		audio_up_l_set_48k();
		/* fifo clk disable */
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << AUDIO_L_UP_AFIFO_CLKEN_OFFSET);
		udelay(CLEAR_FIFO_DELAY_LEN_MS);
		/* fifo clk enable */
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << AUDIO_L_UP_AFIFO_CLKEN_OFFSET);
		audio_up_r_set_48k();
		/* fifo clk disable */
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << AUDIO_R_UP_AFIFO_CLKEN_OFFSET);
		udelay(CLEAR_FIFO_DELAY_LEN_MS);
		/* fifo clk enable */
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << AUDIO_R_UP_AFIFO_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		if (priv->audioup_4mic_state == TRACK_FREE) {
			/* clear fifo */
			asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
				0x1 << AUDIO_L_UP_AFIFO_CLKEN_OFFSET);
			asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
				0x1 << AUDIO_UP_L_PGA_CLKEN_OFFSET);
			asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
				0x1 << AUDIO_UP_L_SRCDN_CLKEN_OFFSET);

			/* clear fifo */
			asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
				0x1 << AUDIO_R_UP_AFIFO_CLKEN_OFFSET);
			asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
				0x1 << AUDIO_UP_R_PGA_CLKEN_OFFSET);
			asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
				0x1 << AUDIO_UP_R_SRCDN_CLKEN_OFFSET);
		}
		break;
	default:
		AUDIO_LOGW("audio 2mic power event err:%d", event);
		break;
	}

	return 0;
}

int voice_2mic_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << VOICE_UP_L_PGA_CLKEN_OFFSET);
		asp_codec_set_reg_bits(CODEC_CLK_EN1_REG,
			0x1 << VOICE_UP_L_SRCDN_CLKEN_OFFSET);
		/* fifo clk disable */
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << VOICE_L_UP_AFIFO_CLKEN_OFFSET);
		udelay(CLEAR_FIFO_DELAY_LEN_MS);
		/* fifo clk enable */
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << VOICE_L_UP_AFIFO_CLKEN_OFFSET);
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << VOICE_UP_R_PGA_CLKEN_OFFSET);
		asp_codec_set_reg_bits(CODEC_CLK_EN1_REG,
			0x1 << VOICE_UP_R_SRCDN_CLKEN_OFFSET);
		/* fifo clk disable */
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << VOICE_R_UP_AFIFO_CLKEN_OFFSET);
		udelay(CLEAR_FIFO_DELAY_LEN_MS);
		/* fifo clk enable */
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << VOICE_R_UP_AFIFO_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* clear fifo */
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << VOICE_L_UP_AFIFO_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
			0x1 << VOICE_UP_L_SRCDN_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << VOICE_UP_L_PGA_CLKEN_OFFSET);
		/* clear fifo */
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << VOICE_R_UP_AFIFO_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
			0x1 << VOICE_UP_R_SRCDN_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << VOICE_UP_R_PGA_CLKEN_OFFSET);
		break;
	default:
		AUDIO_LOGW("voice 2mic power event err:%d", event);
		break;
	}

	return 0;
}

static void mic3_fifo_clear(void)
{
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << MIC3_UP_AFIFO_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
		0x1 << MIC3_UP_SRCDN_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << MIC3_UP_PGA_CLKEN_OFFSET);
}

static void mic4_fifo_clear(void)
{
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << MIC4_UP_AFIFO_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
		0x1 << MIC4_UP_SRCDN_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
		0x1 << MIC4_UP_PGA_CLKEN_OFFSET);
}

int mic34_bt_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << MIC3_UP_AFIFO_CLKEN_OFFSET);
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << MIC4_UP_AFIFO_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* clear fifo */
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << MIC3_UP_AFIFO_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << MIC4_UP_AFIFO_CLKEN_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static void audio_4mic_dapm_pre_pmu_proc(enum track_state state)
{
	/* reset route */
	if (state == TRACK_FREE)
		asp_codec_set_reg_bits(CODEC_SW_RST_N_REG,
			0x1 << RST_4MIC_AUDIO_ACCESS_IRQ_OFFSET);

	audio_up_l_set_48k();
	audio_up_r_set_48k();

	if (state == TRACK_FREE) {
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << MIC3_UP_PGA_CLKEN_OFFSET);
		asp_codec_set_reg_bits(CODEC_CLK_EN1_REG,
			0x1 << MIC3_UP_SRCDN_CLKEN_OFFSET);
		asp_codec_set_reg_bits(CODEC_CLK_EN1_REG,
			0x1 << MIC4_UP_PGA_CLKEN_OFFSET);
		asp_codec_set_reg_bits(CODEC_CLK_EN1_REG,
			0x1 << MIC4_UP_SRCDN_CLKEN_OFFSET);
	}

	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << AUDIO_L_UP_AFIFO_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << AUDIO_R_UP_AFIFO_CLKEN_OFFSET);

	if (state == TRACK_FREE) {
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << MIC3_UP_AFIFO_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << MIC4_UP_AFIFO_CLKEN_OFFSET);
	}

	udelay(CLEAR_FIFO_DELAY_LEN_MS);
	asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << AUDIO_L_UP_AFIFO_CLKEN_OFFSET);
	asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << AUDIO_R_UP_AFIFO_CLKEN_OFFSET);

	if (state == TRACK_FREE) {
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << MIC3_UP_AFIFO_CLKEN_OFFSET);
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << MIC4_UP_AFIFO_CLKEN_OFFSET);

		/* unreset route */
		asp_codec_clr_reg_bits(CODEC_SW_RST_N_REG,
			0x1 << RST_4MIC_AUDIO_ACCESS_IRQ_OFFSET);
	}
}

static void audio_4mic_dapm_post_pmd_proc(enum track_state state)
{
	/* clear fifo */
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << AUDIO_L_UP_AFIFO_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << AUDIO_UP_L_PGA_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
		0x1 << AUDIO_UP_L_SRCDN_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << AUDIO_R_UP_AFIFO_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << AUDIO_UP_R_PGA_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
		0x1 << AUDIO_UP_R_SRCDN_CLKEN_OFFSET);

	if (state == TRACK_FREE) {
		mic3_fifo_clear();
		mic4_fifo_clear();
	}
}

static int audio_4mic_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *codec = asp_codec_get_codec();
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		audio_4mic_dapm_pre_pmu_proc(priv->voiceup_state);
		priv->audioup_4mic_state = TRACK_STARTUP;
		break;
	case SND_SOC_DAPM_POST_PMD:
		audio_4mic_dapm_post_pmd_proc(priv->voiceup_state);
		priv->audioup_4mic_state = TRACK_FREE;
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static void voice_4mic_dapm_pre_pmu_proc(void)
{
	/* reset route */
	asp_codec_set_reg_bits(CODEC_SW_RST_N_REG,
		0x1 << RST_4MIC_VOICE_ACCESS_IRQ_OFFSET);

	asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << VOICE_UP_L_PGA_CLKEN_OFFSET);
	asp_codec_set_reg_bits(CODEC_CLK_EN1_REG,
		0x1 << VOICE_UP_L_SRCDN_CLKEN_OFFSET);
	asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << VOICE_UP_R_PGA_CLKEN_OFFSET);
	asp_codec_set_reg_bits(CODEC_CLK_EN1_REG,
		0x1 << VOICE_UP_R_SRCDN_CLKEN_OFFSET);
	asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << MIC3_UP_PGA_CLKEN_OFFSET);
	asp_codec_set_reg_bits(CODEC_CLK_EN1_REG,
		0x1 << MIC3_UP_SRCDN_CLKEN_OFFSET);
	asp_codec_set_reg_bits(CODEC_CLK_EN1_REG,
		0x1 << MIC4_UP_PGA_CLKEN_OFFSET);
	asp_codec_set_reg_bits(CODEC_CLK_EN1_REG,
		0x1 << MIC4_UP_SRCDN_CLKEN_OFFSET);

	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << VOICE_L_UP_AFIFO_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << VOICE_R_UP_AFIFO_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << MIC3_UP_AFIFO_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << MIC4_UP_AFIFO_CLKEN_OFFSET);
	udelay(CLEAR_FIFO_DELAY_LEN_MS);
	asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << VOICE_L_UP_AFIFO_CLKEN_OFFSET);
	asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << VOICE_R_UP_AFIFO_CLKEN_OFFSET);
	asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << MIC3_UP_AFIFO_CLKEN_OFFSET);
	asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << MIC4_UP_AFIFO_CLKEN_OFFSET);

	/* unreset route */
	asp_codec_clr_reg_bits(CODEC_SW_RST_N_REG,
		0x1 << RST_4MIC_VOICE_ACCESS_IRQ_OFFSET);
}

static void voice_4mic_dapm_post_pmd_proc(void)
{
	/* clear fifo */
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << VOICE_L_UP_AFIFO_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
		0x1 << VOICE_UP_L_SRCDN_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << VOICE_UP_L_PGA_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << VOICE_R_UP_AFIFO_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
		0x1 << VOICE_UP_R_SRCDN_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << VOICE_UP_R_PGA_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << MIC3_UP_AFIFO_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN1_REG,
		0x1 << MIC3_UP_SRCDN_CLKEN_OFFSET);
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		0x1 << MIC3_UP_PGA_CLKEN_OFFSET);
	mic4_fifo_clear();
}

static int voice_4mic_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *codec = asp_codec_get_codec();
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(codec);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		voice_4mic_dapm_pre_pmu_proc();
		priv->voiceup_state = TRACK_STARTUP;
		break;
	case SND_SOC_DAPM_POST_PMD:
		voice_4mic_dapm_post_pmd_proc();
		priv->voiceup_state = TRACK_FREE;
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static void mdm5g_up_l_pre_pmu_proc()
{
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		BIT(MDM_5G_L_UP_AFIFO_CLKEN_OFFSET));
	udelay(CLEAR_FIFO_DELAY_LEN_MS);
	asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
		BIT(MDM_5G_L_UP_AFIFO_CLKEN_OFFSET));
}

static void mdm5g_up_l_post_pmd_proc()
{
	/* clear fifo */
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		BIT(MDM_5G_L_UP_AFIFO_CLKEN_OFFSET));
	asp_codec_reg_update(FS_CTRL1_REG,
		MASK_ON_BIT(FS_MDM_5G_L_UP_AFIFO_LEN, FS_MDM_5G_L_UP_AFIFO_OFFSET),
		0x5 << FS_MDM_5G_L_UP_AFIFO_OFFSET);
}
static int mdm5g_up_l_96k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		mdm5g_up_l_pre_pmu_proc();
		asp_codec_reg_update(FS_CTRL1_REG,
			MASK_ON_BIT(FS_MDM_5G_L_UP_AFIFO_LEN, FS_MDM_5G_L_UP_AFIFO_OFFSET),
			0x5 << FS_MDM_5G_L_UP_AFIFO_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mdm5g_up_l_post_pmd_proc();
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

int mdm5g_up_l_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		mdm5g_up_l_pre_pmu_proc();
		asp_codec_reg_update(FS_CTRL1_REG,
				MASK_ON_BIT(FS_MDM_5G_L_UP_AFIFO_LEN,
					FS_MDM_5G_L_UP_AFIFO_OFFSET),
				0x3 << FS_MDM_5G_L_UP_AFIFO_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mdm5g_up_l_post_pmd_proc();
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static void mdm5g_up_r_pre_pmu_proc()
{
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		BIT(MDM_5G_R_UP_AFIFO_CLKEN_OFFSET));
	udelay(CLEAR_FIFO_DELAY_LEN_MS);
	asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
		BIT(MDM_5G_R_UP_AFIFO_CLKEN_OFFSET));
}

static void mdm5g_up_r_post_pmd_proc()
{
	/* clear fifo */
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		BIT(MDM_5G_R_UP_AFIFO_CLKEN_OFFSET));
	asp_codec_reg_update(FS_CTRL1_REG,
		MASK_ON_BIT(3, FS_MDM_5G_R_UP_AFIFO_OFFSET),
		0x5 << FS_MDM_5G_R_UP_AFIFO_OFFSET);
}

static int mdm5g_up_r_96k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		mdm5g_up_r_pre_pmu_proc();
		asp_codec_reg_update(FS_CTRL1_REG,
			MASK_ON_BIT(FS_MDM_5G_R_UP_AFIFO_LEN, FS_MDM_5G_R_UP_AFIFO_OFFSET),
			0x5 << FS_MDM_5G_R_UP_AFIFO_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mdm5g_up_r_post_pmd_proc();
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

int mdm5g_up_r_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		mdm5g_up_r_pre_pmu_proc();
		asp_codec_reg_update(FS_CTRL1_REG,
			MASK_ON_BIT(FS_MDM_5G_R_UP_AFIFO_LEN, FS_MDM_5G_R_UP_AFIFO_OFFSET),
			0x3 << FS_MDM_5G_R_UP_AFIFO_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mdm5g_up_r_post_pmd_proc();
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static void mdm5g_dn_r_pre_pmu_proc()
{
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		BIT(MDM_5G_R_DN_AFIFO_CLKEN_OFFSET));
	udelay(CLEAR_FIFO_DELAY_LEN_MS);
	asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
		BIT(MDM_5G_R_DN_AFIFO_CLKEN_OFFSET));
}

static void mdm5g_dn_r_post_pmd_proc()
{
	/* clear fifo */
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		BIT(MDM_5G_R_DN_AFIFO_CLKEN_OFFSET));
	asp_codec_reg_update(FS_CTRL1_REG,
		MASK_ON_BIT(3, FS_MDM_5G_R_DN_AFIFO_OFFSET),
		0x3 << FS_MDM_5G_R_DN_AFIFO_OFFSET);
}

int mdm5g_dn_r_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		mdm5g_dn_r_pre_pmu_proc();
		asp_codec_reg_update(FS_CTRL1_REG,
			MASK_ON_BIT(FS_MDM_5G_R_DN_AFIFO_LEN, FS_MDM_5G_R_DN_AFIFO_OFFSET),
			0x3 << FS_MDM_5G_R_DN_AFIFO_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mdm5g_dn_r_post_pmd_proc();
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static void mdm5g_dn_l_pre_pmu_proc()
{
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		BIT(MDM_5G_L_DN_AFIFO_CLKEN_OFFSET));
	udelay(CLEAR_FIFO_DELAY_LEN_MS);
	asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
		BIT(MDM_5G_L_DN_AFIFO_CLKEN_OFFSET));
}

static void mdm5g_dn_l_post_pmd_proc()
{
	/* clear fifo */
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		BIT(MDM_5G_L_DN_AFIFO_CLKEN_OFFSET));
	asp_codec_reg_update(FS_CTRL1_REG,
		MASK_ON_BIT(3, FS_MDM_5G_L_DN_AFIFO_OFFSET),
		0x3 << FS_MDM_5G_L_DN_AFIFO_OFFSET);
}

int mdm5g_dn_l_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		mdm5g_dn_l_pre_pmu_proc();
		asp_codec_reg_update(FS_CTRL1_REG,
			MASK_ON_BIT(FS_MDM_5G_L_DN_AFIFO_LEN, FS_MDM_5G_L_DN_AFIFO_OFFSET),
			0x3 << FS_MDM_5G_L_DN_AFIFO_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		mdm5g_dn_l_post_pmd_proc();
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int ultr_up_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << INFR_L_UP_AFIFO_CLKEN_OFFSET);
		udelay(CLEAR_FIFO_DELAY_LEN_MS);
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << INFR_L_UP_AFIFO_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* clear fifo */
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << INFR_L_UP_AFIFO_CLKEN_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

int audio_dn_l_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << AUDIO_L_DN_AFIFO_CLKEN_OFFSET);
		udelay(CLEAR_FIFO_DELAY_LEN_MS);
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << AUDIO_L_DN_AFIFO_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << AUDIO_L_DN_AFIFO_CLKEN_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

int audio_dn_r_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << AUDIO_R_DN_AFIFO_CLKEN_OFFSET);
		udelay(CLEAR_FIFO_DELAY_LEN_MS);
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << AUDIO_R_DN_AFIFO_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << AUDIO_R_DN_AFIFO_CLKEN_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int audio_dn_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
	case SND_SOC_DAPM_POST_PMD:
		/* default config is same as 48k config */
		asp_codec_reg_update(FS_CTRL0_REG,
			MAX_VAL_ON_BIT(2) << FS_AUDIO_L_DN_AFIFO_OFFSET,
			0x0 << FS_AUDIO_L_DN_AFIFO_OFFSET);
		asp_codec_reg_update(FS_CTRL0_REG,
			MAX_VAL_ON_BIT(2) << FS_AUDIO_R_DN_AFIFO_OFFSET,
			0x0 << FS_AUDIO_R_DN_AFIFO_OFFSET);
		asp_codec_reg_update(FS_CTRL2_REG,
			MAX_VAL_ON_BIT(2) << FS_AUDIO_L_DN_PGA_OFFSET,
			0x0 << FS_AUDIO_L_DN_PGA_OFFSET);
		asp_codec_reg_update(FS_CTRL2_REG,
			MAX_VAL_ON_BIT(2) << FS_AUDIO_R_DN_PGA_OFFSET,
			0x0 << FS_AUDIO_R_DN_PGA_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

int codec3_dn_l_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << CODEC3_L_DN_AFIFO_CLKEN_OFFSET);
		udelay(CLEAR_FIFO_DELAY_LEN_MS);
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << CODEC3_L_DN_AFIFO_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << CODEC3_L_DN_AFIFO_CLKEN_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

int codec3_dn_r_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << CODEC3_R_DN_AFIFO_CLKEN_OFFSET);
		udelay(CLEAR_FIFO_DELAY_LEN_MS);
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << CODEC3_R_DN_AFIFO_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << CODEC3_R_DN_AFIFO_CLKEN_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int codec3_dn_44k1_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_set_reg_bits(CODEC_SW_RST_N_REG,
			0x1 << SLV_MODE_441_LEN);

		asp_codec_reg_update(FS_CTRL0_REG,
			MAX_VAL_ON_BIT(3) << FS_CODEC3_L_DN_AFIFO_OFFSET,
			0x3 << FS_CODEC3_L_DN_AFIFO_OFFSET);
		asp_codec_reg_update(FS_CTRL0_REG,
			MAX_VAL_ON_BIT(3) << FS_CODEC3_R_DN_AFIFO_OFFSET,
			0x3 << FS_CODEC3_R_DN_AFIFO_OFFSET);
		asp_codec_reg_update(FS_CTRL2_REG,
			MAX_VAL_ON_BIT(3) << FS_CODEC3_L_DN_PGA_OFFSET,
			0x3 << FS_CODEC3_L_DN_PGA_OFFSET);
		asp_codec_reg_update(FS_CTRL2_REG,
			MAX_VAL_ON_BIT(3) << FS_CODEC3_R_DN_PGA_OFFSET,
			0x3 << FS_CODEC3_R_DN_PGA_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		asp_codec_clr_reg_bits(CODEC_SW_RST_N_REG,
			0x1 << SLV_MODE_441_LEN);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int codec3_dn_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_reg_update(FS_CTRL0_REG,
			MAX_VAL_ON_BIT(3) << FS_CODEC3_L_DN_AFIFO_OFFSET,
			0x3 << FS_CODEC3_L_DN_AFIFO_OFFSET);
		asp_codec_reg_update(FS_CTRL0_REG,
			MAX_VAL_ON_BIT(3) << FS_CODEC3_R_DN_AFIFO_OFFSET,
			0x3 << FS_CODEC3_R_DN_AFIFO_OFFSET);
		asp_codec_reg_update(FS_CTRL2_REG,
			MAX_VAL_ON_BIT(3) << FS_CODEC3_L_DN_PGA_OFFSET,
			0x3 << FS_CODEC3_L_DN_PGA_OFFSET);
		asp_codec_reg_update(FS_CTRL2_REG,
			MAX_VAL_ON_BIT(3) << FS_CODEC3_R_DN_PGA_OFFSET,
			0x3 << FS_CODEC3_R_DN_PGA_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int codec3_dn_96k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_reg_update(FS_CTRL0_REG,
			MAX_VAL_ON_BIT(3) << FS_CODEC3_L_DN_AFIFO_OFFSET,
			0x5 << FS_CODEC3_L_DN_AFIFO_OFFSET);
		asp_codec_reg_update(FS_CTRL0_REG,
			MAX_VAL_ON_BIT(3) << FS_CODEC3_R_DN_AFIFO_OFFSET,
			0x5 << FS_CODEC3_R_DN_AFIFO_OFFSET);
		asp_codec_reg_update(FS_CTRL2_REG,
			MAX_VAL_ON_BIT(3) << FS_CODEC3_L_DN_PGA_OFFSET,
			0x5 << FS_CODEC3_L_DN_PGA_OFFSET);
		asp_codec_reg_update(FS_CTRL2_REG,
			MAX_VAL_ON_BIT(3) << FS_CODEC3_R_DN_PGA_OFFSET,
			0x5 << FS_CODEC3_R_DN_PGA_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int codec3_dn_192k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_reg_update(FS_CTRL0_REG,
			MAX_VAL_ON_BIT(3) << FS_CODEC3_L_DN_AFIFO_OFFSET,
			0x6 << FS_CODEC3_L_DN_AFIFO_OFFSET);
		asp_codec_reg_update(FS_CTRL0_REG,
			MAX_VAL_ON_BIT(3) << FS_CODEC3_R_DN_AFIFO_OFFSET,
			0x6 << FS_CODEC3_R_DN_AFIFO_OFFSET);
		asp_codec_reg_update(FS_CTRL2_REG,
			MAX_VAL_ON_BIT(3) << FS_CODEC3_L_DN_PGA_OFFSET,
			0x6 << FS_CODEC3_L_DN_PGA_OFFSET);
		asp_codec_reg_update(FS_CTRL2_REG,
			MAX_VAL_ON_BIT(3) << FS_CODEC3_R_DN_PGA_OFFSET,
			0x6 << FS_CODEC3_R_DN_PGA_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static void s1_tx_tdm_dapm_pre_pmu_proc(void)
{
	asp_codec_set_reg_bits(I2S1_TDM_CTRL1_REG,
		0x1 << S1_I2S_TDM_MODE_OFFSET);
	asp_codec_set_reg_bits(I2S1_TDM_CTRL0_REG,
		0x1 << S1_TDM_IF_EN_OFFSET);
	/* audio dn fifo fs 48k */
	asp_codec_reg_update(FS_CTRL0_REG,
		MAX_VAL_ON_BIT(2) << FS_AUDIO_L_DN_AFIFO_OFFSET,
		0x0 << FS_AUDIO_L_DN_AFIFO_OFFSET);
	asp_codec_reg_update(FS_CTRL0_REG,
		MAX_VAL_ON_BIT(2) << FS_AUDIO_R_DN_AFIFO_OFFSET,
		0x0 << FS_AUDIO_R_DN_AFIFO_OFFSET);
	/* codec3 dn fifo fs 48k */
	asp_codec_reg_update(FS_CTRL0_REG,
		MAX_VAL_ON_BIT(3) << FS_CODEC3_L_DN_AFIFO_OFFSET,
		0x3 << FS_CODEC3_L_DN_AFIFO_OFFSET);
	asp_codec_reg_update(FS_CTRL0_REG,
		MAX_VAL_ON_BIT(3) << FS_CODEC3_R_DN_AFIFO_OFFSET,
		0x3 << FS_CODEC3_R_DN_AFIFO_OFFSET);
	/* audio PGA FS */
	asp_codec_reg_update(FS_CTRL2_REG,
		MAX_VAL_ON_BIT(2) << FS_AUDIO_L_DN_PGA_OFFSET,
		0x0 << FS_AUDIO_L_DN_PGA_OFFSET);
	asp_codec_reg_update(FS_CTRL2_REG,
		MAX_VAL_ON_BIT(2) << FS_AUDIO_R_DN_PGA_OFFSET,
		0x0 << FS_AUDIO_R_DN_PGA_OFFSET);
	/* codec3 PGA FS */
	asp_codec_reg_update(FS_CTRL2_REG,
		MAX_VAL_ON_BIT(3) << FS_CODEC3_L_DN_PGA_OFFSET,
		0x3 << FS_CODEC3_L_DN_PGA_OFFSET);
	asp_codec_reg_update(FS_CTRL2_REG,
		MAX_VAL_ON_BIT(3) << FS_CODEC3_R_DN_PGA_OFFSET,
		0x3 << FS_CODEC3_R_DN_PGA_OFFSET);
}

static void s1_tx_tdm_dapm_post_pmd_proc(void)
{
	/* audio dn fifo fs 48k */
	asp_codec_reg_update(FS_CTRL0_REG,
		MAX_VAL_ON_BIT(2) << FS_AUDIO_L_DN_AFIFO_OFFSET,
		0x0 << FS_AUDIO_L_DN_AFIFO_OFFSET);
	asp_codec_reg_update(FS_CTRL0_REG,
		MAX_VAL_ON_BIT(2) << FS_AUDIO_R_DN_AFIFO_OFFSET,
		0x0 << FS_AUDIO_R_DN_AFIFO_OFFSET);
	/* codec3 dn fifo fs default */
	asp_codec_reg_update(FS_CTRL0_REG,
		MAX_VAL_ON_BIT(3) << FS_CODEC3_L_DN_AFIFO_OFFSET,
		0x7 << FS_CODEC3_L_DN_AFIFO_OFFSET);
	asp_codec_reg_update(FS_CTRL0_REG,
		MAX_VAL_ON_BIT(3) << FS_CODEC3_R_DN_AFIFO_OFFSET,
		0x7 << FS_CODEC3_R_DN_AFIFO_OFFSET);
	/* audio PGA FS */
	asp_codec_reg_update(FS_CTRL2_REG,
		MAX_VAL_ON_BIT(2) << FS_AUDIO_L_DN_PGA_OFFSET,
		0x0 << FS_AUDIO_L_DN_PGA_OFFSET);
	asp_codec_reg_update(FS_CTRL2_REG,
		MAX_VAL_ON_BIT(2) << FS_AUDIO_R_DN_PGA_OFFSET,
		0x0 << FS_AUDIO_R_DN_PGA_OFFSET);
	/* codec3 PGA FS */
	asp_codec_reg_update(FS_CTRL2_REG,
		MAX_VAL_ON_BIT(3) << FS_CODEC3_L_DN_PGA_OFFSET,
		0x3 << FS_CODEC3_L_DN_PGA_OFFSET);
	asp_codec_reg_update(FS_CTRL2_REG,
		MAX_VAL_ON_BIT(3) << FS_CODEC3_R_DN_PGA_OFFSET,
		0x3 << FS_CODEC3_R_DN_PGA_OFFSET);
	asp_codec_clr_reg_bits(I2S1_TDM_CTRL1_REG,
		0x1 << S1_I2S_TDM_MODE_OFFSET);
	asp_codec_clr_reg_bits(I2S1_TDM_CTRL0_REG,
		0x1 << S1_TDM_IF_EN_OFFSET);
}

int s1_tx_tdm_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		s1_tx_tdm_dapm_pre_pmu_proc();
		break;
	case SND_SOC_DAPM_POST_PMD:
		s1_tx_tdm_dapm_post_pmd_proc();
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int s1_tx_i2s_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_clr_reg_bits(I2S1_TDM_CTRL1_REG,
			0x1 << S1_I2S_TDM_MODE_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		asp_codec_clr_reg_bits(I2S1_TDM_CTRL1_REG,
			0x1 << S1_I2S_TDM_MODE_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int iv_2pa_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << SPA_1_UP_AFIFO_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << SPA_3_UP_AFIFO_CLKEN_OFFSET);
		udelay(CLEAR_FIFO_DELAY_LEN_MS);
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << SPA_1_UP_AFIFO_CLKEN_OFFSET);
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << SPA_3_UP_AFIFO_CLKEN_OFFSET);
		/* smartpa fifo 1/3 fs */
		asp_codec_reg_update(FS_CTRL0_REG,
			MAX_VAL_ON_BIT(3) << FS_SPA_1_UP_AFIFO_OFFSET,
			0x3 << FS_SPA_1_UP_AFIFO_OFFSET);
		asp_codec_reg_update(FS_CTRL0_REG,
			MAX_VAL_ON_BIT(3) << FS_SPA_3_UP_AFIFO_OFFSET,
			0x3 << FS_SPA_3_UP_AFIFO_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << SPA_1_UP_AFIFO_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << SPA_3_UP_AFIFO_CLKEN_OFFSET);
		/* set fifo fs default */
		asp_codec_reg_update(FS_CTRL0_REG,
			MAX_VAL_ON_BIT(3) << FS_SPA_1_UP_AFIFO_OFFSET,
			0x3 << FS_SPA_1_UP_AFIFO_OFFSET);
		asp_codec_reg_update(FS_CTRL0_REG,
			MAX_VAL_ON_BIT(3) << FS_SPA_3_UP_AFIFO_OFFSET,
			0x3 << FS_SPA_3_UP_AFIFO_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static void spa_fifo_fs_set_48k(void)
{
	asp_codec_reg_update(FS_CTRL0_REG,
		MAX_VAL_ON_BIT(3) << FS_SPA_1_UP_AFIFO_OFFSET,
		0x3 << FS_SPA_1_UP_AFIFO_OFFSET);
	asp_codec_reg_update(FS_CTRL0_REG,
		MAX_VAL_ON_BIT(3) << FS_SPA_2_UP_AFIFO_OFFSET,
		0x3 << FS_SPA_2_UP_AFIFO_OFFSET);
	asp_codec_reg_update(FS_CTRL0_REG,
		MAX_VAL_ON_BIT(3) << FS_SPA_3_UP_AFIFO_OFFSET,
		0x3 << FS_SPA_3_UP_AFIFO_OFFSET);
	asp_codec_reg_update(FS_CTRL0_REG,
		MAX_VAL_ON_BIT(3) << FS_SPA_4_UP_AFIFO_OFFSET,
		0x3 << FS_SPA_4_UP_AFIFO_OFFSET);
}

int iv_4pa_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << SPA_1_UP_AFIFO_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << SPA_2_UP_AFIFO_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << SPA_3_UP_AFIFO_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << SPA_4_UP_AFIFO_CLKEN_OFFSET);
		udelay(CLEAR_FIFO_DELAY_LEN_MS);
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << SPA_1_UP_AFIFO_CLKEN_OFFSET);
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << SPA_2_UP_AFIFO_CLKEN_OFFSET);
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << SPA_3_UP_AFIFO_CLKEN_OFFSET);
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << SPA_4_UP_AFIFO_CLKEN_OFFSET);
		spa_fifo_fs_set_48k();
		break;
	case SND_SOC_DAPM_POST_PMD:
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << SPA_1_UP_AFIFO_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << SPA_2_UP_AFIFO_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << SPA_3_UP_AFIFO_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << SPA_4_UP_AFIFO_CLKEN_OFFSET);
		spa_fifo_fs_set_48k();
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int carkit_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		/* audio dn fifo fs */
		asp_codec_reg_update(FS_CTRL0_REG,
			MAX_VAL_ON_BIT(2) << FS_AUDIO_L_DN_AFIFO_OFFSET,
			0x0 << FS_AUDIO_L_DN_AFIFO_OFFSET);
		asp_codec_reg_update(FS_CTRL0_REG,
			MAX_VAL_ON_BIT(2) << FS_AUDIO_R_DN_AFIFO_OFFSET,
			0x0 << FS_AUDIO_R_DN_AFIFO_OFFSET);

		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << AUDIO_L_DN_AFIFO_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << AUDIO_R_DN_AFIFO_CLKEN_OFFSET);
		udelay(CLEAR_FIFO_DELAY_LEN_MS);
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << AUDIO_L_DN_AFIFO_CLKEN_OFFSET);
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << AUDIO_R_DN_AFIFO_CLKEN_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		/* audio dn fifo fs */
		asp_codec_reg_update(FS_CTRL0_REG,
			MAX_VAL_ON_BIT(2) << FS_AUDIO_L_DN_AFIFO_OFFSET,
			0x0 << FS_AUDIO_L_DN_AFIFO_OFFSET);
		asp_codec_reg_update(FS_CTRL0_REG,
			MAX_VAL_ON_BIT(2) << FS_AUDIO_R_DN_AFIFO_OFFSET,
			0x0 << FS_AUDIO_R_DN_AFIFO_OFFSET);

		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << AUDIO_L_DN_AFIFO_CLKEN_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			0x1 << AUDIO_R_DN_AFIFO_CLKEN_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static void ultr_dn_pre_pmu_proc(void)
{
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		BIT(ULTR_L_DN_AFIFO_CLKEN_OFFSET));
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		BIT(ULTR_R_DN_AFIFO_CLKEN_OFFSET));
	udelay(CLEAR_FIFO_DELAY_LEN_MS);
	asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
		BIT(ULTR_L_DN_AFIFO_CLKEN_OFFSET));
	asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
		BIT(ULTR_R_DN_AFIFO_CLKEN_OFFSET));
}

static void ultr_dn_post_pmd_proc(void)
{
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		BIT(ULTR_L_DN_AFIFO_CLKEN_OFFSET));
	asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
		BIT(ULTR_R_DN_AFIFO_CLKEN_OFFSET));
	/* set ultr fifo fs defaule 96k */
	asp_codec_reg_update(FS_CTRL0_REG,
		MASK_ON_BIT(FS_ULTR_L_DN_AFIFO_LEN, FS_ULTR_L_DN_AFIFO_OFFSET),
		0x1 << FS_ULTR_L_DN_AFIFO_OFFSET);
	asp_codec_reg_update(FS_CTRL0_REG,
		MASK_ON_BIT(FS_ULTR_R_DN_AFIFO_LEN, FS_ULTR_R_DN_AFIFO_OFFSET),
		0x1 << FS_ULTR_R_DN_AFIFO_OFFSET);
}

static int ultr_dn_96k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		ultr_dn_pre_pmu_proc();
		/* ultr fifo fs96k */
		asp_codec_reg_update(FS_CTRL0_REG,
			MASK_ON_BIT(FS_ULTR_L_DN_AFIFO_LEN, FS_ULTR_L_DN_AFIFO_OFFSET),
			0x1 << FS_ULTR_L_DN_AFIFO_OFFSET);
		asp_codec_reg_update(FS_CTRL0_REG,
			MASK_ON_BIT(FS_ULTR_R_DN_AFIFO_LEN, FS_ULTR_R_DN_AFIFO_OFFSET),
			0x1 << FS_ULTR_R_DN_AFIFO_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		ultr_dn_post_pmd_proc();
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

int ultr_dn_48k_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		ultr_dn_pre_pmu_proc();
		/* ultr fifo fs48k */
		asp_codec_reg_update(FS_CTRL0_REG,
			MASK_ON_BIT(FS_ULTR_L_DN_AFIFO_LEN, FS_ULTR_L_DN_AFIFO_OFFSET),
			0x0 << FS_ULTR_L_DN_AFIFO_OFFSET);
		asp_codec_reg_update(FS_CTRL0_REG,
			MASK_ON_BIT(FS_ULTR_R_DN_AFIFO_LEN, FS_ULTR_R_DN_AFIFO_OFFSET),
			0x0 << FS_ULTR_R_DN_AFIFO_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		ultr_dn_post_pmd_proc();
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

/* UP SWITCH */
/* SWITCH - VOICEUP */
static const struct snd_kcontrol_new dapm_vo_up_l_8k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR,
		VOICE_UP_L_8K_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_vo_up_l_16k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR,
		VOICE_UP_L_16K_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_vo_up_l_32k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR,
		VOICE_UP_L_32K_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_vo_up_l_48k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR,
		VOICE_UP_L_48K_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_vo_up_r_8k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR,
		VOICE_UP_R_8K_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_vo_up_r_16k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR,
		VOICE_UP_R_16K_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_vo_up_r_32k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR,
		VOICE_UP_R_32K_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_vo_up_r_48k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR,
		VOICE_UP_R_48K_BIT, 1, 0);
/* SWITCH - MIC34_UP */
static const struct snd_kcontrol_new dapm_mic3_32k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR, MIC3_32K_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_mic3_48k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR, MIC3_48K_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_mic4_32k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR, MIC4_32K_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_mic4_48k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR, MIC4_48K_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_mic34_96k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR, MIC34_96K_BIT, 1, 0);
/* SWITCH - DMIC1_IN */
static const struct snd_kcontrol_new dapm_dmic1_in_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_CLK_EN2_REG, DMIC1_CLKEN_OFFSET, 1, 0);
/* SWITCH - DMIC2_IN */
static const struct snd_kcontrol_new dapm_dmic2_in_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_CLK_EN2_REG, DMIC2_CLKEN_OFFSET, 1, 0);
/* SWITCH - CODEC3_UP_44K1 */
static const struct snd_kcontrol_new dapm_s3_44k1_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR, S3_44K1_IN_BIT, 1, 0);
/* SWITCH - CODEC3_UP_48K */
static const struct snd_kcontrol_new dapm_s3_48k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR, S3_48K_IN_BIT, 1, 0);
/* SWITCH - CODEC3_UP_96K */
static const struct snd_kcontrol_new dapm_s3_96k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR, S3_96K_IN_BIT, 1, 0);
/* SWITCH - CODEC3_UP_192K */
static const struct snd_kcontrol_new dapm_s3_192k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR, S3_192K_IN_BIT, 1, 0);
/* SWITCH - AUDIO_UP */
static const struct snd_kcontrol_new dapm_audio_up_l_o_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR, AUDIO_UP_L_O_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_audio_up_r_o_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR, AUDIO_UP_R_O_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_audio_2mic_o_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, AUDIO_UP_O_BIT, 1, 0);
/* SWITCH - VOICE_UP */
static const struct snd_kcontrol_new dapm_voice_up_l_o_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR, VOICE_UP_L_O_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_voice_up_r_o_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR, VOICE_UP_R_O_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_voice_2mic_o_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, VOICE_UP_O_BIT, 1, 0);
/* SWITCH - MIC34_UP */
static const struct snd_kcontrol_new dapm_mic3_o_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR, MIC3_O_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_mic4_o_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR, MIC4_O_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_audio_4mic_o_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR, AUDIO_4MIC_O_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_voice_4mic_o_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR, VOICE_4MIC_O_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_mic34_bt_o_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, MIC34_BT_O_BIT, 1, 0);
/* SWITCH - MDM5G */

static const struct snd_kcontrol_new dapm_mdm5g_up_l_96k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, MDM5G_UP_L_96K_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_mdm5g_up_l_48k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, MDM5G_UP_L_48K_BIT, 1, 0);

static const struct snd_kcontrol_new dapm_mdm5g_up_r_96k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, MDM5G_UP_R_96K_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_mdm5g_up_r_48k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, MDM5G_UP_R_48K_BIT, 1, 0);
/* SWITCH - ULTR_UP */
static const struct snd_kcontrol_new dapm_ultr_up_o_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR0_REG_ADDR, ULTR_UP_O_BIT, 1, 0);
/* SWITCH -CIC_MIC_UP */
static const struct snd_kcontrol_new dapm_cic_adc1_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_CLK_EN2_REG,
		ADC1_CIC_D16_CLKEN_OFFSET, 1, 0);
static const struct snd_kcontrol_new dapm_cic_adc2_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_CLK_EN2_REG,
		ADC2_CIC_D16_CLKEN_OFFSET, 1, 0);
static const struct snd_kcontrol_new dapm_cic_adc3_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_CLK_EN2_REG,
		ADC3_CIC_D16_CLKEN_OFFSET, 1, 0);
static const struct snd_kcontrol_new dapm_cic_adc4_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_CLK_EN2_REG,
		ADC4_CIC_D16_CLKEN_OFFSET, 1, 0);
static const struct snd_kcontrol_new dapm_cic_adc5_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_CLK_EN2_REG,
		ADC5_CIC_D16_CLKEN_OFFSET, 1, 0);
/* DOWN SWITCH */
static const struct snd_kcontrol_new dapm_codec3_dn_l_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR,
		CODEC3_DN_L_IN_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_codec3_dn_r_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR,
		CODEC3_DN_R_IN_BIT, 1, 0);
/* SWITCH - CODEC3_DN_44K1 */
static const struct snd_kcontrol_new dapm_codec3_dn_44k1_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR,
		CODEC3_DN_44K1_IN_BIT, 1, 0);
/* SWITCH - CODEC3_DN_48K */
static const struct snd_kcontrol_new dapm_codec3_dn_48k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR,
		CODEC3_DN_48K_IN_BIT, 1, 0);
/* SWITCH - CODEC3_DN_96K */
static const struct snd_kcontrol_new dapm_codec3_dn_96k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR,
		CODEC3_DN_96K_IN_BIT, 1, 0);
/* SWITCH - CODEC3_DN_192K */
static const struct snd_kcontrol_new dapm_codec3_dn_192k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE",
		CODEC_VIR1_REG_ADDR, CODEC3_DN_192K_IN_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_audio_dn_l_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, AUDIO_DN_L_IN_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_audio_dn_r_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, AUDIO_DN_R_IN_BIT, 1, 0);
/* SWITCH - AUDIO_DN_48K */
static const struct snd_kcontrol_new dapm_audio_dn_48k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR,
		AUDIO_DN_PLAY48K_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_s1_tx_tdm_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, S1_TX_TDM_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_s1_tx_i2s_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, S1_TX_I2S_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_iv_2pa_48k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, IV_2PA_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_iv_4pa_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, IV_4PA_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_s2_tx_l_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, S2_TX_L_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_s2_tx_r_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, S2_TX_R_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_s2_rx_l_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, S2_RX_L_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_s2_rx_r_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, S2_RX_R_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_s3_tx_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, S3_TX_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_s4_tx_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, S4_TX_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_s4_rx_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_CLK_EN2_REG, I2S1_TDM_CLKEN_OFFSET, 1, 0);
static const struct snd_kcontrol_new dapm_ultr_dn_96k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, ULTR_DN_96K_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_ultr_dn_48k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, ULTR_DN_48K_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_carkit_48k_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_VIR1_REG_ADDR, CARKIT_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_dacl_m_switch_controls =
	SOC_DAPM_SINGLE("ENABLE", CODEC_CLK_EN2_REG, DACL_M_CLKEN_OFFSET, 1, 0);

#define SWITCH_WIDGET \
	SND_SOC_DAPM_SWITCH("DMIC1_IN_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_dmic1_in_switch_controls), \
	SND_SOC_DAPM_SWITCH("DMIC2_IN_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_dmic2_in_switch_controls), \
	SND_SOC_DAPM_SWITCH_E("VOICE_UP_L_8K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_vo_up_l_8k_switch_controls, vo_up_l_8k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("VOICE_UP_L_16K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_vo_up_l_16k_switch_controls, vo_up_l_16k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("VOICE_UP_L_32K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_vo_up_l_32k_switch_controls, vo_up_l_32k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("VOICE_UP_L_48K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_vo_up_l_48k_switch_controls, vo_up_l_48k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("VOICE_UP_R_8K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_vo_up_r_8k_switch_controls, vo_up_r_8k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("VOICE_UP_R_16K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_vo_up_r_16k_switch_controls, vo_up_r_16k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("VOICE_UP_R_32K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_vo_up_r_32k_switch_controls, vo_up_r_32k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("VOICE_UP_R_48K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_vo_up_r_48k_switch_controls, vo_up_r_48k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("MIC3_32K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_mic3_32k_switch_controls, mic3_32k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("MIC3_48K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_mic3_48k_switch_controls, mic3_48k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("MIC4_32K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_mic4_32k_switch_controls, mic4_32k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("MIC4_48K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_mic4_48k_switch_controls, mic4_48k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("MIC34_96K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_mic34_96k_switch_controls, mic34_96k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("S3_44K1_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_s3_44k1_switch_controls, s3_44k1_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("S3_48K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_s3_48k_switch_controls, s3_48k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("S3_96K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_s3_96k_switch_controls, s3_96k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("S3_192K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_s3_192k_switch_controls, s3_192k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH("AUDIO_UP_L_O_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_audio_up_l_o_switch_controls), \
	SND_SOC_DAPM_SWITCH("AUDIO_UP_R_O_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_audio_up_r_o_switch_controls), \
	SND_SOC_DAPM_SWITCH_E("AUDIO_2MIC_O_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_audio_2mic_o_switch_controls, audio_2mic_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH("VOICE_UP_L_O_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_voice_up_l_o_switch_controls), \
	SND_SOC_DAPM_SWITCH("VOICE_UP_R_O_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_voice_up_r_o_switch_controls), \
	SND_SOC_DAPM_SWITCH_E("VOICE_2MIC_O_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_voice_2mic_o_switch_controls, voice_2mic_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH("MIC3_O_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_mic3_o_switch_controls), \
	SND_SOC_DAPM_SWITCH("MIC4_O_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_mic4_o_switch_controls), \
	SND_SOC_DAPM_SWITCH_E("AUDIO_4MIC_O_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_audio_4mic_o_switch_controls, audio_4mic_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("VOICE_4MIC_O_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_voice_4mic_o_switch_controls, voice_4mic_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("MDM5G_UP_L_96K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_mdm5g_up_l_96k_switch_controls, mdm5g_up_l_96k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("MDM5G_UP_L_48K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_mdm5g_up_l_48k_switch_controls, mdm5g_up_l_48k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("MDM5G_UP_R_96K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_mdm5g_up_r_96k_switch_controls, mdm5g_up_r_96k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("MDM5G_UP_R_48K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_mdm5g_up_r_48k_switch_controls, mdm5g_up_r_48k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("ULTR_UP_O_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_ultr_up_o_switch_controls, ultr_up_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("MIC34_BT_O_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_mic34_bt_o_switch_controls, mic34_bt_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH("CIC_ADC1_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_cic_adc1_switch_controls), \
	SND_SOC_DAPM_SWITCH("CIC_ADC2_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_cic_adc2_switch_controls), \
	SND_SOC_DAPM_SWITCH("CIC_ADC3_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_cic_adc3_switch_controls), \
	SND_SOC_DAPM_SWITCH("CIC_ADC4_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_cic_adc4_switch_controls), \
	SND_SOC_DAPM_SWITCH("CIC_ADC5_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_cic_adc5_switch_controls), \
	SND_SOC_DAPM_SWITCH("S2_RX_L_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_s2_rx_l_switch_controls), \
	SND_SOC_DAPM_SWITCH("S2_RX_R_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_s2_rx_r_switch_controls), \
	SND_SOC_DAPM_SWITCH_E("AUDIO_DN_L_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_audio_dn_l_switch_controls, audio_dn_l_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("AUDIO_DN_R_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_audio_dn_r_switch_controls, audio_dn_r_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("AUDIO_DN_48K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_audio_dn_48k_switch_controls, audio_dn_48k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("CODEC3_DN_L_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_codec3_dn_l_switch_controls, codec3_dn_l_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("CODEC3_DN_R_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_codec3_dn_r_switch_controls, codec3_dn_r_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("CODEC3_DN_44K1_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_codec3_dn_44k1_switch_controls, \
		codec3_dn_44k1_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("CODEC3_DN_48K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_codec3_dn_48k_switch_controls, \
		codec3_dn_48k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("CODEC3_DN_96K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_codec3_dn_96k_switch_controls, \
		codec3_dn_96k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("CODEC3_DN_192K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_codec3_dn_192k_switch_controls, \
		codec3_dn_192k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("S1_TX_TDM_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_s1_tx_tdm_switch_controls, s1_tx_tdm_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("S1_TX_I2S_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_s1_tx_i2s_switch_controls, s1_tx_i2s_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("IV_2PA_48K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_iv_2pa_48k_switch_controls, iv_2pa_48k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("IV_4PA_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_iv_4pa_switch_controls, iv_4pa_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("CARKIT_48K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_carkit_48k_switch_controls, carkit_48k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH("S2_TX_L_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_s2_tx_l_switch_controls), \
	SND_SOC_DAPM_SWITCH("S2_TX_R_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_s2_tx_r_switch_controls), \
	SND_SOC_DAPM_SWITCH("S3_TX_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_s3_tx_switch_controls), \
	SND_SOC_DAPM_SWITCH("S4_TX_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_s4_tx_switch_controls), \
	SND_SOC_DAPM_SWITCH("S4_RX_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_s4_rx_switch_controls), \
	SND_SOC_DAPM_SWITCH_E("ULTR_DN_96K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_ultr_dn_96k_switch_controls, ultr_dn_96k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("ULTR_DN_48K_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_ultr_dn_48k_switch_controls, ultr_dn_48k_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH("DACL_M_SWITCH", SND_SOC_NOPM, 0, 0, \
		&dapm_dacl_m_switch_controls) \
/*lint +e570*/

static const struct snd_soc_dapm_widget switch_widgets[] = { SWITCH_WIDGET };

int asp_codec_add_switch_widgets(struct snd_soc_dapm_context *dapm)
{
	if (dapm == NULL) {
		AUDIO_LOGE("input dapm is null");
		return -EINVAL;
	}

	return snd_soc_dapm_new_controls(dapm, switch_widgets,
		ARRAY_SIZE(switch_widgets));
}

