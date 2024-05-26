/*
 * slimbus_codec_dai.c -- slimbus codec dai driver
 *
 * Copyright (c) 2020 Huawei Technologies Co., Ltd.
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

#include <linux/of.h>
#include <linux/timer.h>
#include <linux/platform_drivers/da_combine/da_combine_v5.h>
#include <linux/platform_drivers/da_combine/da_combine_v5_regs.h>
#include <linux/platform_drivers/da_combine/da_combine_v5_type.h>

#include "codec_dai.h"
#include "audio_log.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "slimbus_codec_dai"

#define FS_SLIM_OFFSET 4
#define DAI_LINK_CODEC_DAI_NAME_AUDIO  "da_combine_v5-audio-dai"
#define DAI_LINK_CODEC_DAI_NAME_BT     "da_combine_v5-bt-dai"

struct snd_soc_dai_ops slimbus_codec_audio_dai_ops = {
	.hw_params = codec_dai_hw_params,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	.mute_stream = codec_dai_mute_stream,
#else
	.digital_mute = codec_dai_digital_mute,
#endif
};

struct snd_soc_dai_ops slimbus_codec_bt_dai_ops = {
	.hw_params = codec_dai_hw_params,
};

static void slimbus_codec_dai_init(struct snd_soc_component *codec)
{
	struct da_combine_v5_platform_data *priv = snd_soc_component_get_drvdata(codec);

	/* io share */
	snd_soc_component_write_adapter(codec, IOS_MF_CTRL3_REG, 0x2);
	snd_soc_component_write_adapter(codec, IOS_MF_CTRL4_REG, 0x2);
	snd_soc_component_write_adapter(codec, IOS_IOM_CTRL7_REG, 0x114);
	snd_soc_component_write_adapter(codec, IOS_IOM_CTRL8_REG, 0x115);
	da_combine_update_bits(codec, IOS_IOM_CTRL7_REG, MASK_ON_BIT(DS_LEN, DS_OFFSET),
		priv->cdc_ctrl->data_cdc_drv << DS_OFFSET);
	da_combine_update_bits(codec, IOS_IOM_CTRL8_REG, MASK_ON_BIT(DS_LEN, DS_OFFSET),
		priv->cdc_ctrl->clk_cdc_drv << DS_OFFSET);

	da_combine_update_bits(codec, CFG_CLK_CTRL_REG, MASK_ON_BIT(INTF_SLIM_CLK_EN_LEN, INTF_SLIM_CLK_EN_OFFSET),
		BIT(INTF_SLIM_CLK_EN_OFFSET));
	da_combine_update_bits(codec, SC_CODEC_MUX_CTRL0_REG, MASK_ON_BIT(SLIM_SW_SEL_LEN, SLIM_SW_SEL_OFFSET), 0x0);
	snd_soc_component_write_adapter(codec, SLIM_CTRL3_REG, 0xBF);
	snd_soc_component_write_adapter(codec, SLIM_CTRL0_REG, 0x6);
	da_combine_update_bits(codec, SLIM_CTRL1_REG, MASK_ON_BIT(SLIM_SWIRE_DIV_EN_LEN, SLIM_SWIRE_DIV_EN_OFFSET),
		BIT(SLIM_SWIRE_DIV_EN_OFFSET));
	da_combine_update_bits(codec, SLIM_CTRL1_REG, MASK_ON_BIT(SLIM_CLK_EN_LEN, SLIM_CLK_EN_OFFSET),
		BIT(SLIM_CLK_EN_OFFSET));

	/* slimbus up1&2 port fs config */
	snd_soc_component_write_adapter(codec, SC_FS_SLIM_CTRL_3_REG, 0x44);
	/* slimbus up3&4 port fs config */
	snd_soc_component_write_adapter(codec, SC_FS_SLIM_CTRL_4_REG, 0x44);
	/* slimbus up7&8 port fs config */
	snd_soc_component_write_adapter(codec, SC_FS_SLIM_CTRL_6_REG, 0x44);
	/* slimbus up9&10 port fs config */
	snd_soc_component_write_adapter(codec, SC_FS_SLIM_CTRL_7_REG, 0x44);

	/* slimbus dn1&dn2 port fs config */
	snd_soc_component_write_adapter(codec, SC_FS_SLIM_CTRL_0_REG, 0x44);
	/* slimbus dn5&dn6 port fs config */
	snd_soc_component_write_adapter(codec, SC_FS_SLIM_CTRL_2_REG, 0x44);

	AUDIO_LOGI("slimbus codec dai init");
}

static const struct port_config slimbus_port_cfg[] = {
	{ PORT_D1_D2, SC_FS_SLIM_CTRL_0_REG },
	{ PORT_D3_D4, SC_FS_SLIM_CTRL_1_REG },
	{ PORT_D5_D6, SC_FS_SLIM_CTRL_2_REG },
	{ PORT_U1_U2, SC_FS_SLIM_CTRL_3_REG },
	{ PORT_U3_U4, SC_FS_SLIM_CTRL_4_REG },
	{ PORT_U5_U6, SC_FS_SLIM_CTRL_5_REG },
	{ PORT_U7_U8, SC_FS_SLIM_CTRL_6_REG },
	{ PORT_U9,    SC_FS_SLIM_CTRL_7_REG },
	{ PORT_U10,   SC_FS_SLIM_CTRL_7_REG },
};

static const struct rate_reg_config slimbus_fs_cfg[] = {
	{ SAMPLE_RATE_8K,   FS_VALUE_8K   << FS_SLIM_OFFSET, FS_VALUE_8K },
	{ SAMPLE_RATE_16K,  FS_VALUE_16K  << FS_SLIM_OFFSET, FS_VALUE_16K },
	{ SAMPLE_RATE_32K,  FS_VALUE_32K  << FS_SLIM_OFFSET, FS_VALUE_32K },
	{ SAMPLE_RATE_48K,  FS_VALUE_48K  << FS_SLIM_OFFSET, FS_VALUE_48K },
	{ SAMPLE_RATE_96K,  FS_VALUE_96K  << FS_SLIM_OFFSET, FS_VALUE_96K },
	{ SAMPLE_RATE_192K, FS_VALUE_192K << FS_SLIM_OFFSET, FS_VALUE_192K },
	{ SAMPLE_RATE_44K1, FS_VALUE_44K1 << FS_SLIM_OFFSET, FS_VALUE_44K1 },
	{ SAMPLE_RATE_88K2, FS_VALUE_88K2 << FS_SLIM_OFFSET, FS_VALUE_88K2 },
	{ SAMPLE_RATE_176K4, FS_VALUE_176K4 << FS_SLIM_OFFSET, FS_VALUE_176K4 }
};

static void set_slimbus_fifo_for_44k1(struct snd_soc_component *codec)
{
	da_combine_update_bits(codec, SLIM_SYNC_CTRL2_REG,
		MASK_ON_BIT(SLIM_SYNC_FIFO_AEMPTY_TH_LEN, SLIM_SYNC_FIFO_AEMPTY_TH_OFFSET),
		0x2c);
	da_combine_update_bits(codec, SLIM_SYNC_CTRL1_REG,
		MASK_ON_BIT(SLIM_SYNC_FIFO_AF0_TH_LEN, SLIM_SYNC_FIFO_AF0_TH_OFFSET),
		0x3c);
}

static void set_slimbus_fifo_for_default(struct snd_soc_component *codec)
{
	da_combine_update_bits(codec, SLIM_SYNC_CTRL2_REG,
		MASK_ON_BIT(SLIM_SYNC_FIFO_AEMPTY_TH_LEN, SLIM_SYNC_FIFO_AEMPTY_TH_OFFSET),
		0x1);
	da_combine_update_bits(codec, SLIM_SYNC_CTRL1_REG,
		MASK_ON_BIT(SLIM_SYNC_FIFO_AF0_TH_LEN, SLIM_SYNC_FIFO_AF0_TH_OFFSET),
		0x18);
}

static int slimbus_codec_dai_config_rate(struct snd_soc_component *codec,
	struct codec_dai_sample_rate_params *hwparms)
{
	unsigned int i;
	unsigned int reg;

	if (hwparms->rate % SAMPLE_RATE_44K1 == 0)
		set_slimbus_fifo_for_44k1(codec);
	else
		set_slimbus_fifo_for_default(codec);

	for (i = 0; i < ARRAY_SIZE(slimbus_port_cfg); i++) {
		if (hwparms->port == slimbus_port_cfg[i].port) {
			reg = slimbus_port_cfg[i].reg;
			break;
		}
	}

	if (i >= ARRAY_SIZE(slimbus_port_cfg)) {
		AUDIO_LOGE("port invalid argument");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(slimbus_fs_cfg); i++)
		if (hwparms->rate == slimbus_fs_cfg[i].rate)
			break;

	if (i >= ARRAY_SIZE(slimbus_fs_cfg)) {
		AUDIO_LOGE("config sample rate invalid argument");
		return -EINVAL;
	}

	if (hwparms->port == PORT_U9)
		da_combine_update_bits(codec, reg, MASK_ON_BIT(FS_SLIM_UP9_LEN, FS_SLIM_UP9_OFFSET), slimbus_fs_cfg[i].dn_bit_val);
	else if (hwparms->port == PORT_U10)
		da_combine_update_bits(codec, reg, MASK_ON_BIT(FS_SLIM_UP10_LEN, FS_SLIM_UP10_OFFSET), slimbus_fs_cfg[i].up_bit_val);
	else
		snd_soc_component_write_adapter(codec, reg, slimbus_fs_cfg[i].up_bit_val | slimbus_fs_cfg[i].dn_bit_val);

	return 0;
}

static struct snd_soc_dai_driver slimbus_codec_dai_driver[] = {
	{
		.name = DAI_LINK_CODEC_DAI_NAME_AUDIO,
		.playback = {
			.stream_name = "Playback",
			.channels_min = 2,
			.channels_max = 4,
			.rates = DA_COMBINE_V5_RATES,
			.formats = DA_COMBINE_V5_FORMATS },
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 13,
			.rates = DA_COMBINE_V5_RATES,
			.formats = DA_COMBINE_V5_FORMATS },
		.ops = &slimbus_codec_audio_dai_ops,
		.probe = codec_dai_probe,
	},
	{
		.name = DAI_LINK_CODEC_DAI_NAME_BT,
		.playback = {
			.stream_name = "Playback",
			.channels_min = 2,
			.channels_max = 2,
			.rates = DA_COMBINE_V5_RATES,
			.formats = DA_COMBINE_V5_FORMATS },
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = DA_COMBINE_V5_RATES,
			.formats = DA_COMBINE_V5_FORMATS },
		.ops = &slimbus_codec_bt_dai_ops,
	},
};

static struct da_combine_v5_codec_dai slimbus_codec_dai = {
	.dai_driver = slimbus_codec_dai_driver,
	.dai_nums = ARRAY_SIZE(slimbus_codec_dai_driver),
	.config_sample_rate = slimbus_codec_dai_config_rate,
	.codec_dai_init = slimbus_codec_dai_init,
};

struct da_combine_v5_codec_dai *get_slimbus_codec_dai(void)
{
	return &slimbus_codec_dai;
}
