/*
 * asp_codec.h -- asp codec driver
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

#ifndef __ASP_CODEC_H__
#define __ASP_CODEC_H__

#include "asp_codec_utils.h"

enum virtual_reg_offset {
	VIR0_REG_OFFSET = 0x0,
	VIR1_REG_OFFSET,
	VIR2_REG_OFFSET,
	VIR_REG_OFFSET_BUTT,
};

/* virtual reg */
#define CODEC_VIR0_REG_ADDR ((PAGE_VIRCODEC) + (VIR0_REG_OFFSET))
#define VOICE_UP_L_8K_BIT   0
#define VOICE_UP_L_32K_BIT  1
#define VOICE_UP_L_48K_BIT  2
#define VOICE_UP_R_8K_BIT   3
#define VOICE_UP_R_32K_BIT  4
#define VOICE_UP_R_48K_BIT  5
#define MIC3_32K_BIT        6
#define MIC3_48K_BIT        7
#define MIC4_32K_BIT        8
#define MIC4_48K_BIT        9
#define DMIC1_IN_BIT        10
#define DMIC2_IN_BIT        11
#define S3_44K1_IN_BIT      12
#define S3_48K_IN_BIT       13
#define S3_96K_IN_BIT       14
#define S3_192K_IN_BIT      15
#define AUDIO_UP_L_O_BIT    16
#define AUDIO_UP_R_O_BIT    17
#define VOICE_UP_L_O_BIT    18
#define VOICE_UP_R_O_BIT    19
#define MIC3_O_BIT          20
#define MIC4_O_BIT          21
#define AUDIO_4MIC_O_BIT    22
#define VOICE_4MIC_O_BIT    23
#define ULTR_UP_O_BIT       24
#define CIC_MIC1_BIT        25
#define CIC_MIC2_BIT        26
#define CIC_MIC3_BIT        27
#define CIC_MIC4_BIT        28
#define CIC_MIC5_BIT        29
#define MIC34_96K_BIT       30

#define CODEC_VIR1_REG_ADDR   ((PAGE_VIRCODEC) + (VIR1_REG_OFFSET))
#define CODEC3_DN_L_IN_BIT    0
#define CODEC3_DN_R_IN_BIT    1
#define CODEC3_DN_44K1_IN_BIT 2
#define CODEC3_DN_48K_IN_BIT  3
#define CODEC3_DN_96K_IN_BIT  4
#define CODEC3_DN_192K_IN_BIT 5
#define AUDIO_DN_L_IN_BIT     6
#define AUDIO_DN_R_IN_BIT     7
#define AUDIO_DN_PLAY48K_BIT  8
#define S1_TX_TDM_BIT         9
#define S1_TX_I2S_BIT         10
#define IV_2PA_BIT            11
#define IV_4PA_BIT            12
#define S2_TX_L_BIT           13
#define S2_TX_R_BIT           14
#define S2_RX_L_BIT           15
#define S2_RX_R_BIT           16
#define S3_TX_BIT             17
#define S4_TX_BIT             18
#define ULTR_DN_96K_BIT       19
#define CARKIT_BIT            20
#define MIC34_BT_O_BIT        21
#define VOICE_UP_L_16K_BIT    22
#define VOICE_UP_R_16K_BIT    23
#define AUDIO_UP_O_BIT        24
#define VOICE_UP_O_BIT        25
#define ULTR_DN_48K_BIT       26
#define MDM5G_UP_L_96K_BIT    27
#define MDM5G_UP_R_96K_BIT    28
#define MDM5G_UP_L_48K_BIT    29
#define MDM5G_UP_R_48K_BIT    30
#define MASTER_PLAYBACK_BIT   31

#define CODEC_VIR2_REG_ADDR   ((PAGE_VIRCODEC) + (VIR2_REG_OFFSET))
#define CAR_BT_UPLINK         0
#define CAR_BT_DLLINK         1
#define CAPTURE_BIT           2

#define PB_MIN_CHANNELS       2
#define PB_MAX_CHANNELS       12
#define CP_MIN_CHANNELS       1
#define CP_MAX_CHANNELS       30
#define PCM_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE\
				| SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S32_LE)
#define PCM_RATES   SNDRV_PCM_RATE_8000_384000

#define CLEAR_FIFO_DELAY_LEN_MS      1

enum asp_codec_platform_type {
	FPGA_PLATFORM = 0,
	UDP_PLATFORM,
	PLATFORM_CNT,
};

enum track_state {
	TRACK_FREE = 0,
	TRACK_STARTUP = 1,
};

enum port_config_type {
	PORT_CODECLESS,
	PORT_CODEC,
	PORT_CNT,
};

struct asp_codec_reg_page {
	unsigned int page_tag;
	unsigned int page_reg_begin;
	unsigned int page_reg_end;
	const char *page_name;
};

struct asp_codec_priv {
	struct snd_soc_component *codec;
	bool have_dapm;
	bool asp_pd;
	spinlock_t lock;
	enum asp_codec_platform_type platform_type;
	unsigned int v_codec_reg[VIR_REG_OFFSET_BUTT];

	struct regulator *asp_regulator;
	struct clk *asp_subsys_clk;
	struct clk *asp_49m_clk;
	struct pinctrl *pctrl;
	struct pinctrl_state *pin_default;
	struct pinctrl_state *pin_idle;
	enum track_state voiceup_state;
	enum track_state audioup_4mic_state;
	bool pm_runtime_support;
	enum port_config_type port_cfg_type;
};

struct snd_soc_component *asp_codec_get_codec(void);
#endif

