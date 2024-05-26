/*
 * asp_codec_single_switch_widget.c -- asp codec single switch widget driver
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

#include "asp_codec_single_switch_widget.h"

#include <linux/of.h>
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
static void bt_s22voiceup_config(void)
{
	asp_codec_clr_reg_bits(VOICE_R_UP_PGA_CTRL_REG,
		BIT(VOICE_R_UP_PGA_BYPASS_OFFSET));
	asp_codec_clr_reg_bits(VOICE_L_UP_PGA_CTRL_REG,
		BIT(VOICE_L_UP_PGA_BYPASS_OFFSET));
	asp_codec_reg_update(CODEC_DIN_MUX0_REG,
		MASK_ON_BIT(VO_UP_L_DIN_SEL_LEN, VO_UP_L_DIN_SEL_OFFSET),
		0x2 << VO_UP_L_DIN_SEL_OFFSET);
	asp_codec_reg_update(CODEC_DIN_MUX0_REG,
		MASK_ON_BIT(VO_UP_R_DIN_SEL_LEN, VO_UP_R_DIN_SEL_OFFSET),
		0x2 << VO_UP_R_DIN_SEL_OFFSET);
}

static void bt_s2_up_config(void)
{
	asp_codec_reg_update(CODEC_CLK_EN2_REG,
		MASK_ON_BIT(I2S2_PCM_CLKEN_LEN, I2S2_PCM_CLKEN_OFFSET),
		BIT(I2S2_PCM_CLKEN_OFFSET));
	asp_codec_reg_update(CODEC_CLK_EN1_REG,
		MASK_ON_BIT(I2S2_RX_L_PGA_CLKEN_LEN, I2S2_RX_L_PGA_CLKEN_OFFSET),
		BIT(I2S2_RX_L_PGA_CLKEN_OFFSET));
	asp_codec_reg_update(I2S2_PCM_CTRL_REG,
		MASK_ON_BIT(I2S2_RX_CLK_SEL_LEN, I2S2_RX_CLK_SEL_OFFSET),
		BIT(I2S2_RX_CLK_SEL_OFFSET));
	asp_codec_reg_update(I2S2_PCM_CTRL_REG,
		MASK_ON_BIT(I2S2_TX_CLK_SEL_LEN, I2S2_TX_CLK_SEL_OFFSET),
		BIT(I2S2_TX_CLK_SEL_OFFSET));
	asp_codec_reg_update(FS_CTRL3_REG,
		MASK_ON_BIT(FS_I2S2_RX_L_PGA_LEN, FS_I2S2_RX_L_PGA_OFFSET),
		0x1 << FS_I2S2_RX_L_PGA_OFFSET);
	asp_codec_reg_update(CODEC_CLK_EN1_REG,
		MASK_ON_BIT(I2S2_RX_L_SRCUP_CLKEN_LEN, I2S2_RX_L_SRCUP_CLKEN_OFFSET),
		BIT(I2S2_RX_L_SRCUP_CLKEN_OFFSET));
	asp_codec_reg_update(FS_CTRL3_REG,
		MASK_ON_BIT(FS_I2S2_RX_L_SRCUP_DIN_LEN, FS_I2S2_RX_L_SRCUP_DIN_OFFSET),
		0x1 << FS_I2S2_RX_L_SRCUP_DIN_OFFSET);
	asp_codec_reg_update(FS_CTRL3_REG,
		MASK_ON_BIT(FS_I2S2_RX_L_SRCUP_DOUT_LEN, FS_I2S2_RX_L_SRCUP_DOUT_OFFSET),
		0x4 << FS_I2S2_RX_L_SRCUP_DOUT_OFFSET);
	asp_codec_reg_update(SRCUP_CTRL_REG,
		MASK_ON_BIT(I2S2_RX_L_SRCUP_SRC_MODE_LEN, I2S2_RX_L_SRCUP_SRC_MODE_OFFSET),
		0x5 << I2S2_RX_L_SRCUP_SRC_MODE_OFFSET);
	asp_codec_reg_update(I2S2_PCM_CTRL_REG,
		MASK_ON_BIT(I2S2_FRAME_MODE_LEN, I2S2_FRAME_MODE_OFFSET),
		BIT(I2S2_FRAME_MODE_OFFSET));
	asp_codec_reg_update(I2S2_PCM_CTRL_REG,
		MASK_ON_BIT(I2S2_FUNC_MODE_LEN, I2S2_FUNC_MODE_OFFSET),
		0x4 << I2S2_FUNC_MODE_OFFSET);
	asp_codec_reg_update(I2S2_PCM_CTRL_REG,
		MASK_ON_BIT(FS_I2S2_LEN, FS_I2S2_OFFSET),
		0x1 << FS_I2S2_OFFSET);
	asp_codec_reg_update(I2S2_PCM_CTRL_REG,
		MASK_ON_BIT(I2S2_CODEC_IO_WORDLENGTH_LEN, I2S2_CODEC_IO_WORDLENGTH_OFFSET),
		0x0 << I2S2_CODEC_IO_WORDLENGTH_OFFSET);
}


static int bt_playback_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		vo_up_l_48k_power_event(w, kcontrol, event);
		vo_up_r_48k_power_event(w, kcontrol, event);
		voice_2mic_power_event(w, kcontrol, event);
		bt_s22voiceup_config();
		bt_s2_up_config();
		break;
	case SND_SOC_DAPM_POST_PMD:
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int master_playback_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	audio_dn_l_power_event(w, kcontrol, event);
	audio_dn_r_power_event(w, kcontrol, event);
	codec3_dn_l_power_event(w, kcontrol, event);
	codec3_dn_r_power_event(w, kcontrol, event);
	s1_tx_tdm_power_event(w, kcontrol, event);
	iv_4pa_power_event(w, kcontrol, event);
	bt_playback_power_event(w, kcontrol, event);
	mdm5g_dn_r_48k_power_event(w, kcontrol, event);
	mdm5g_dn_l_48k_power_event(w, kcontrol, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			BIT(AUDIO_DN_L_PGA_CLKEN_OFFSET));
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			BIT(AUDIO_DN_R_PGA_CLKEN_OFFSET));
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			BIT(CODEC3_DN_L_PGA_CLKEN_OFFSET));
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			BIT(CODEC3_DN_R_PGA_CLKEN_OFFSET));
		udelay(CLEAR_FIFO_DELAY_LEN_MS);
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			BIT(AUDIO_DN_L_PGA_CLKEN_OFFSET));
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			BIT(AUDIO_DN_R_PGA_CLKEN_OFFSET));
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			BIT(CODEC3_DN_L_PGA_CLKEN_OFFSET));
		asp_codec_set_reg_bits(CODEC_CLK_EN0_REG,
			BIT(CODEC3_DN_R_PGA_CLKEN_OFFSET));
		asp_codec_reg_update(FS_CTRL7_REG,
			MASK_ON_BIT(FS_S1_TDM_LEN, FS_S1_TDM_OFFSET),
			0x3 << FS_S1_TDM_OFFSET);
		asp_codec_reg_update(I2S1_CTRL_REG,
			MASK_ON_BIT(FS_I2S1_LEN, FS_I2S1_OFFSET),
			0x3 << FS_I2S1_OFFSET);
		asp_codec_reg_update(CODEC_DIN_MUX1_REG,
			MASK_ON_BIT(BM_I2S1_TDM_TX_L_DIN_SEL_LEN, BM_I2S1_TDM_TX_L_DIN_SEL_OFFSET),
			0x0 << BM_I2S1_TDM_TX_L_DIN_SEL_OFFSET);
		asp_codec_reg_update(CODEC_DIN_MUX1_REG,
			MASK_ON_BIT(BM_I2S1_TDM_TX_R_DIN_SEL_LEN, BM_I2S1_TDM_TX_R_DIN_SEL_OFFSET),
			0x0 << BM_I2S1_TDM_TX_R_DIN_SEL_OFFSET);
		asp_codec_set_reg_bits(I2S4_CTRL_REG,
			BIT(I2S4_IF_TX_ENA_OFFSET));
		break;
	case SND_SOC_DAPM_POST_PMD:
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			BIT(AUDIO_DN_L_PGA_CLKEN_OFFSET));
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			BIT(AUDIO_DN_R_PGA_CLKEN_OFFSET));
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			BIT(CODEC3_DN_L_PGA_CLKEN_OFFSET));
		asp_codec_clr_reg_bits(CODEC_CLK_EN0_REG,
			BIT(CODEC3_DN_R_PGA_CLKEN_OFFSET));

		asp_codec_reg_update(FS_CTRL7_REG,
			MASK_ON_BIT(FS_S1_TDM_LEN, FS_S1_TDM_OFFSET),
			0x3 << FS_S1_TDM_OFFSET);
		asp_codec_reg_update(I2S1_CTRL_REG,
			MASK_ON_BIT(FS_I2S1_LEN, FS_I2S1_OFFSET),
			0x3 << FS_I2S1_OFFSET);
		asp_codec_reg_update(CODEC_DIN_MUX1_REG,
			MASK_ON_BIT(BM_I2S1_TDM_TX_L_DIN_SEL_LEN, BM_I2S1_TDM_TX_L_DIN_SEL_OFFSET),
			0x0 << BM_I2S1_TDM_TX_L_DIN_SEL_OFFSET);
		asp_codec_reg_update(CODEC_DIN_MUX1_REG,
			MASK_ON_BIT(BM_I2S1_TDM_TX_R_DIN_SEL_LEN, BM_I2S1_TDM_TX_R_DIN_SEL_OFFSET),
			0x0 << BM_I2S1_TDM_TX_R_DIN_SEL_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static void bt_ultr2dacl_config(void)
{
	asp_codec_reg_update(DACL_S_MIXER2_CTRL_REG,
		MASK_ON_BIT(DACL_S_MIXER2_IN1_MUTE_LEN, DACL_S_MIXER2_IN1_MUTE_OFFSET),
		BIT(DACL_S_MIXER2_IN1_MUTE_OFFSET));
	asp_codec_reg_update(DACL_S_MIXER2_CTRL_REG,
		MASK_ON_BIT(DACL_S_MIXER2_IN2_MUTE_LEN, DACL_S_MIXER2_IN2_MUTE_OFFSET),
		0x0 << DACL_S_MIXER2_IN2_MUTE_OFFSET);
	asp_codec_reg_update(CODEC_CLK_EN2_REG,
		MASK_ON_BIT(DACL_S_MIXER2_CLKEN_LEN, DACL_S_MIXER2_CLKEN_OFFSET),
		BIT(DACL_S_MIXER2_CLKEN_OFFSET));
	asp_codec_reg_update(FS_CTRL6_REG,
		MASK_ON_BIT(FS_DACLS_MIXER2_LEN, FS_DACLS_MIXER2_OFFSET),
		0x0 << FS_DACLS_MIXER2_OFFSET);
	asp_codec_reg_update(DACL_S_MIXER2_CTRL_REG,
		MASK_ON_BIT(DACL_S_MIXER2_GAIN2_LEN, DACL_S_MIXER2_GAIN2_OFFSET),
		0x3 << DACL_S_MIXER2_GAIN2_OFFSET);
	asp_codec_reg_update(CODEC_DIN_MUX1_REG,
		MASK_ON_BIT(BM_DACL_AU_DIN_SEL_LEN, BM_DACL_AU_DIN_SEL_OFFSET),
		0x0 << BM_DACL_AU_DIN_SEL_OFFSET);
	asp_codec_reg_update(CODEC_CLK_EN2_REG,
		MASK_ON_BIT(DACL_M_CLKEN_LEN, DACL_M_CLKEN_OFFSET),
		BIT(DACL_M_CLKEN_OFFSET));
}

static void bt_adc2s2_config(void)
{
	asp_codec_reg_update(CODEC_DIN_MUX0_REG,
		MASK_ON_BIT(SIDETONE_PGA_DIN_SEL_LEN, SIDETONE_PGA_DIN_SEL_OFFSET),
		0x0 << SIDETONE_PGA_DIN_SEL_OFFSET);
	asp_codec_reg_update(CODEC_CLK_EN1_REG,
		MASK_ON_BIT(SIDETONE_PGA_CLKEN_LEN, SIDETONE_PGA_CLKEN_OFFSET),
		BIT(SIDETONE_PGA_CLKEN_OFFSET));
	asp_codec_reg_update(CODEC_CLK_EN2_REG,
		MASK_ON_BIT(DACL_MIXER4_CLKEN_LEN, DACL_MIXER4_CLKEN_OFFSET),
		BIT(DACL_MIXER4_CLKEN_OFFSET));
	asp_codec_reg_update(DACL_MIXER4_CTRL0_REG,
		MASK_ON_BIT(DACL_MIXER4_IN4_MUTE_LEN, DACL_MIXER4_IN4_MUTE_OFFSET),
		0x0 << DACL_MIXER4_IN4_MUTE_OFFSET);
	asp_codec_reg_update(DACL_MIXER4_CTRL0_REG,
		MASK_ON_BIT(DACL_MIXER4_IN3_MUTE_LEN, DACL_MIXER4_IN3_MUTE_OFFSET),
		BIT(DACL_MIXER4_IN3_MUTE_OFFSET));
	asp_codec_reg_update(DACL_MIXER4_CTRL0_REG,
		MASK_ON_BIT(DACL_MIXER4_GAIN4_LEN, DACL_MIXER4_GAIN4_OFFSET),
		0x3 << DACL_MIXER4_GAIN4_OFFSET);
	asp_codec_reg_update(FS_CTRL6_REG,
		MASK_ON_BIT(FS_DACL_MIXER4_LEN, FS_DACL_MIXER4_OFFSET),
		0x0 << FS_DACL_MIXER4_OFFSET);
}

static void bt_dacl2adc_config(void)
{
	asp_codec_reg_update(CODEC_CLK_EN2_REG,
		MASK_ON_BIT(AU_49152K_CLKEN_LEN, AU_49152K_CLKEN_OFFSET),
		BIT(AU_49152K_CLKEN_OFFSET));
	asp_codec_reg_update(AU_CTRL_REG,
		MASK_ON_BIT(AU_S2P_LOOP_LEN, AU_S2P_LOOP_OFFSET),
		0x1 << AU_S2P_LOOP_OFFSET);
	asp_codec_reg_update(CODEC_DIN_MUX0_REG,
		MASK_ON_BIT(ADC1_DIN_SEL_LEN, ADC1_DIN_SEL_OFFSET),
		0x0 << ADC1_DIN_SEL_OFFSET);
	asp_codec_reg_update(CODEC_CLK_EN2_REG,
		MASK_ON_BIT(ADC1_CLKEN_LEN, ADC1_CLKEN_OFFSET),
		BIT(ADC1_CLKEN_OFFSET));
	asp_codec_reg_update(CODEC_CLK_EN1_REG,
		MASK_ON_BIT(ADC1_0P5_PGA_CLKEN_LEN, ADC1_0P5_PGA_CLKEN_OFFSET),
		BIT(ADC1_0P5_PGA_CLKEN_OFFSET));
	asp_codec_reg_update(CODEC_CLK_EN2_REG,
		MASK_ON_BIT(DMIC1_CLKEN_LEN, DMIC1_CLKEN_OFFSET),
		BIT(DMIC1_CLKEN_OFFSET));
	asp_codec_reg_update(ADC1_UP_PGA_CTRL_REG,
		MASK_ON_BIT(ADC1_UP_PGA_BYPASS_LEN, ADC1_UP_PGA_BYPASS_OFFSET),
		BIT(ADC1_UP_PGA_BYPASS_OFFSET));
}

static void bt_s2_dl_config(void)
{
	asp_codec_reg_update(CODEC_DIN_MUX1_REG,
		MASK_ON_BIT(BT_TX_SRCDN_DIN_SEL_LEN, BT_TX_SRCDN_DIN_SEL_OFFSET),
		BIT(BT_TX_SRCDN_DIN_SEL_OFFSET));
	asp_codec_reg_update(I2S2_PCM_CTRL_REG,
		MASK_ON_BIT(I2S2_FUNC_MODE_LEN, I2S2_FUNC_MODE_OFFSET),
		0x2 << I2S2_FUNC_MODE_OFFSET);
	asp_codec_reg_update(I2S2_PCM_CTRL_REG,
		MASK_ON_BIT(I2S2_IF_TX_ENA_LEN, I2S2_IF_TX_ENA_OFFSET),
		BIT(I2S2_IF_TX_ENA_OFFSET));
	asp_codec_reg_update(I2S2_PCM_CTRL_REG,
		MASK_ON_BIT(I2S2_IF_RX_ENA_LEN, I2S2_IF_RX_ENA_OFFSET),
		BIT(I2S2_IF_RX_ENA_OFFSET));
	asp_codec_reg_update(I2S2_PCM_CTRL_REG,
		MASK_ON_BIT(I2S2_FRAME_MODE_LEN, I2S2_FRAME_MODE_OFFSET),
		0x0 << I2S2_FRAME_MODE_OFFSET);
	asp_codec_reg_update(I2S2_PCM_CTRL_REG,
		MASK_ON_BIT(I2S2_CODEC_IO_WORDLENGTH_LEN, I2S2_CODEC_IO_WORDLENGTH_OFFSET),
		0x0 << I2S2_CODEC_IO_WORDLENGTH_OFFSET);
	asp_codec_reg_update(CODEC_DIN_MUX0_REG,
		MASK_ON_BIT(I2S2_TX_R_SEL_LEN, I2S2_TX_R_SEL_OFFSET),
		BIT(I2S2_TX_R_SEL_OFFSET));
	asp_codec_reg_update(CODEC_CLK_EN2_REG,
		MASK_ON_BIT(I2S2_TX_L_SRCDN_CLKEN_LEN, I2S2_TX_L_SRCDN_CLKEN_OFFSET),
		BIT(I2S2_TX_L_SRCDN_CLKEN_OFFSET));
	asp_codec_reg_update(FS_CTRL6_REG,
		MASK_ON_BIT(FS_I2S2_TX_L_SRCDN_DIN_LEN, FS_I2S2_TX_L_SRCDN_DIN_OFFSET),
		0x0 << FS_I2S2_TX_L_SRCDN_DIN_OFFSET);
	asp_codec_reg_update(FS_CTRL6_REG,
		MASK_ON_BIT(FS_I2S2_TX_L_SRCDN_DOUT_LEN, FS_I2S2_TX_L_SRCDN_DOUT_OFFSET),
		0x1 << FS_I2S2_TX_L_SRCDN_DOUT_OFFSET);
	asp_codec_reg_update(SRCDN_CTRL1_REG,
		MASK_ON_BIT(I2S2_TX_L_SRCDN_SRC_MODE_LEN, I2S2_TX_L_SRCDN_SRC_MODE_OFFSET),
		0x0 << I2S2_TX_L_SRCDN_SRC_MODE_OFFSET);
	asp_codec_reg_update(I2S2_PCM_CTRL_REG,
		MASK_ON_BIT(FS_I2S2_LEN, FS_I2S2_OFFSET),
		0x1 << FS_I2S2_OFFSET);
}

static int bt_capture_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	ultr_dn_48k_power_event(w, kcontrol, event);

	switch(event){
	case SND_SOC_DAPM_PRE_PMU:
		bt_ultr2dacl_config();
		bt_dacl2adc_config();
		bt_adc2s2_config();
		bt_s2_dl_config();
		break;
	case SND_SOC_DAPM_POST_PMD:
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int s3_capture_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	switch(event){
	case SND_SOC_DAPM_PRE_PMU:
		s3_48k_power_event(w, kcontrol, event);
		mic3_48k_power_event(w, kcontrol, event);
		mic4_48k_power_event(w, kcontrol, event);
		mic34_bt_power_event(w, kcontrol, event);
		asp_codec_reg_update(CODEC_DIN_MUX0_REG,
			MASK_ON_BIT(MIC3_DIN_SEL_LEN, MIC3_DIN_SEL_OFFSET),
			0x2 << MIC3_DIN_SEL_OFFSET);
		asp_codec_reg_update(CODEC_DIN_MUX0_REG,
			MASK_ON_BIT(MIC4_DIN_SEL_LEN, MIC4_DIN_SEL_OFFSET),
			0x2 << MIC4_DIN_SEL_OFFSET);
		asp_codec_reg_update(I2S3_PCM_CTRL_REG,
			MASK_ON_BIT(I2S3_IF_RX_ENA_LEN, I2S3_IF_RX_ENA_OFFSET),
			0x1 << I2S3_IF_RX_ENA_OFFSET);
		asp_codec_reg_update(CODEC_CLK_EN0_REG,
			MASK_ON_BIT(MIC3_UP_PGA_CLKEN_LEN, MIC3_UP_PGA_CLKEN_OFFSET),
			0x1 << MIC3_UP_PGA_CLKEN_OFFSET);
		asp_codec_reg_update(CODEC_CLK_EN1_REG,
			MASK_ON_BIT(MIC4_UP_PGA_CLKEN_LEN, MIC4_UP_PGA_CLKEN_OFFSET),
			0x1 << MIC4_UP_PGA_CLKEN_OFFSET);
		asp_codec_reg_update(CODEC_CLK_EN2_REG,
			MASK_ON_BIT(I2S3_PCM_CLKEN_LEN, I2S3_PCM_CLKEN_OFFSET),
			0x1 << I2S3_PCM_CLKEN_OFFSET);
		asp_codec_reg_update(MIC3_UP_PGA_CTRL_REG,
			MASK_ON_BIT(MIC3_UP_PGA_GAIN_LEN, MIC3_UP_PGA_GAIN_OFFSET),
			0x0 << MIC3_UP_PGA_GAIN_OFFSET);
		asp_codec_reg_update(MIC4_UP_PGA_CTRL_REG,
			MASK_ON_BIT(MIC4_UP_PGA_GAIN_LEN, MIC4_UP_PGA_GAIN_OFFSET),
			0x0 << MIC4_UP_PGA_GAIN_OFFSET);
		break;
	case SND_SOC_DAPM_POST_PMD:
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static int capture_power_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	mdm5g_up_l_48k_power_event(w, kcontrol, event);
	mdm5g_up_r_48k_power_event(w, kcontrol, event);
	bt_capture_power_event(w, kcontrol, event);
	s3_capture_power_event(w, kcontrol, event);

	switch (event) {
	case SND_SOC_DAPM_PRE_PMU:
		asp_codec_reg_update(CODEC_DIN_MUX1_REG,
			MASK_ON_BIT(MDM_5G_L_AFIFO_DIN_SEL_LEN, MDM_5G_L_AFIFO_DIN_SEL_OFFSET),
			BIT(MDM_5G_L_AFIFO_DIN_SEL_OFFSET));
		asp_codec_reg_update(CODEC_DIN_MUX1_REG,
			MASK_ON_BIT(MDM_5G_R_AFIFO_DIN_SEL_LEN, MDM_5G_R_AFIFO_DIN_SEL_OFFSET),
			BIT(MDM_5G_R_AFIFO_DIN_SEL_OFFSET));
		asp_codec_clr_reg_bits(CODEC_CLK_EN2_REG,
			BIT(I2S1_TDM_CLKEN_OFFSET));
		udelay(CLEAR_FIFO_DELAY_LEN_MS);
		asp_codec_set_reg_bits(CODEC_CLK_EN2_REG,
			BIT(I2S1_TDM_CLKEN_OFFSET));
		asp_codec_reg_update(I2S4_CTRL_REG,
			MASK_ON_BIT(I2S4_RX_CLK_SEL_LEN, I2S4_RX_CLK_SEL_OFFSET),
			0x1 << I2S4_RX_CLK_SEL_OFFSET);
		asp_codec_reg_update(I2S4_CTRL_REG,
			MASK_ON_BIT(I2S4_TX_CLK_SEL_LEN, I2S4_TX_CLK_SEL_OFFSET),
			0x1 << I2S4_TX_CLK_SEL_OFFSET);
		asp_codec_reg_update(I2S4_CTRL_REG,
			MASK_ON_BIT(I2S4_FUNC_MODE_LEN, I2S4_FUNC_MODE_OFFSET),
			0x0 << I2S4_FUNC_MODE_OFFSET);
		asp_codec_reg_update(I2S4_CTRL_REG,
			MASK_ON_BIT(I2S4_MST_SLV_LEN, I2S4_MST_SLV_OFFSET),
			0x0 << I2S4_MST_SLV_OFFSET);
		asp_codec_reg_update(I2S4_CTRL_REG,
			MASK_ON_BIT(FS_I2S4_LEN, FS_I2S4_OFFSET),
			0x3 << FS_I2S4_OFFSET);
		asp_codec_reg_update(I2S4_CTRL_REG,
			MASK_ON_BIT(I2S4_CODEC_IO_WORDLENGTH_LEN, I2S4_CODEC_IO_WORDLENGTH_OFFSET),
			0x3 << I2S4_CODEC_IO_WORDLENGTH_OFFSET);
		asp_codec_reg_update(I2S4_CTRL_REG,
			MASK_ON_BIT(I2S4_IF_RX_ENA_LEN, I2S4_IF_RX_ENA_OFFSET),
			BIT(I2S4_IF_RX_ENA_OFFSET));
		break;
	case SND_SOC_DAPM_POST_PMD:
		asp_codec_reg_update(CODEC_DIN_MUX1_REG,
			MASK_ON_BIT(MDM_5G_L_AFIFO_DIN_SEL_LEN, MDM_5G_L_AFIFO_DIN_SEL_OFFSET),
			0x0 << MDM_5G_L_AFIFO_DIN_SEL_OFFSET);
		asp_codec_reg_update(CODEC_DIN_MUX1_REG,
			MASK_ON_BIT(MDM_5G_R_AFIFO_DIN_SEL_LEN, MDM_5G_R_AFIFO_DIN_SEL_OFFSET),
			0x0 << MDM_5G_R_AFIFO_DIN_SEL_OFFSET);
		asp_codec_clr_reg_bits(CODEC_CLK_EN2_REG,
			BIT(I2S1_TDM_CLKEN_OFFSET));
		asp_codec_reg_update(I2S4_CTRL_REG,
			MASK_ON_BIT(I2S4_RX_CLK_SEL_LEN, I2S4_RX_CLK_SEL_OFFSET),
			0x0 << I2S4_RX_CLK_SEL_OFFSET);
		asp_codec_reg_update(I2S4_CTRL_REG,
			MASK_ON_BIT(I2S4_FUNC_MODE_LEN, I2S4_FUNC_MODE_OFFSET),
			0x0 << I2S4_FUNC_MODE_OFFSET);
		asp_codec_reg_update(I2S4_CTRL_REG,
			MASK_ON_BIT(I2S4_MST_SLV_LEN, I2S4_MST_SLV_OFFSET),
			0x0 << I2S4_MST_SLV_OFFSET);
		asp_codec_reg_update(I2S4_CTRL_REG,
			MASK_ON_BIT(FS_I2S4_LEN, FS_I2S4_OFFSET),
			0x5 << FS_I2S4_OFFSET);
		asp_codec_reg_update(I2S4_CTRL_REG,
			MASK_ON_BIT(I2S4_CODEC_IO_WORDLENGTH_LEN, I2S4_CODEC_IO_WORDLENGTH_OFFSET),
			0x3 << I2S4_CODEC_IO_WORDLENGTH_OFFSET);
		asp_codec_reg_update(I2S4_CTRL_REG,
			MASK_ON_BIT(I2S4_IF_RX_ENA_LEN, I2S4_IF_RX_ENA_OFFSET),
			0x0 << I2S4_IF_RX_ENA_OFFSET);
		break;
	default:
		AUDIO_LOGW("power event err:%d", event);
		break;
	}

	return 0;
}

static const struct snd_kcontrol_new dapm_master_playback_switch_controls =
	SOC_DAPM_SINGLE("Switch", CODEC_VIR1_REG_ADDR, MASTER_PLAYBACK_BIT, 1, 0);
static const struct snd_kcontrol_new dapm_capture_switch_controls =
	SOC_DAPM_SINGLE("Switch", CODEC_VIR2_REG_ADDR, CAPTURE_BIT, 1, 0);


#define SWITCH_WIDGET \
	SND_SOC_DAPM_SWITCH_E("Master Playback", SND_SOC_NOPM, 0, 0, \
		&dapm_master_playback_switch_controls, master_playback_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)), \
	SND_SOC_DAPM_SWITCH_E("Master Capture", SND_SOC_NOPM, 0, 0, \
		&dapm_capture_switch_controls, capture_power_event, \
		(SND_SOC_DAPM_PRE_PMU | SND_SOC_DAPM_POST_PMD)) \
/*lint +e570*/

static const struct snd_soc_dapm_widget switch_widgets[] = { SWITCH_WIDGET };

int asp_codec_add_single_switch_widgets(struct snd_soc_dapm_context *dapm)
{
	if (dapm == NULL) {
		AUDIO_LOGE("dapm is null");
		return -EINVAL;
	}

	return snd_soc_dapm_new_controls(dapm, switch_widgets,
		ARRAY_SIZE(switch_widgets));
}

