/*
 * asp_codec.c -- asp codec driver
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

#include "asp_codec.h"

#include <linux/module.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/pm_runtime.h>
#include <linux/io.h>
#include <linux/of.h>
#include <sound/pcm.h>
#include <sound/soc-dapm.h>
#include <soc_crgperiph_interface.h>
#include <linux/version.h>
#include <linux/pinctrl/consumer.h>

#include "audio_log.h"
#include "asp_codec_regs.h"
#include "asp_codec_kcontrol.h"
#include "asp_codec_single_kcontrol.h"
#include "asp_codec_route.h"
#include "asp_codec_single_route.h"
#include "asp_codec_switch_widget.h"
#include "asp_codec_single_switch_widget.h"
#include "asp_codec_path_widget.h"
#include "asp_codec_resource_widget.h"
#ifdef CONFIG_SND_SOC_CODEC_DEBUG
#include "codec_debug.h"
#include "asp_codec_debug.h"
#endif

/*lint -e548 -e429*/
#ifdef CONFIG_AUDIO_DEBUG
#define LOG_TAG "asp_codec"
#else
#define LOG_TAG "Analog_less_v1"
#endif
#define MAX_APS_CLK_COUNT 0x2
#define AUTOSUSPEND_DELAY_TIME 200

#define ASP_CODEC_NAME "asp-codec"

#ifdef CONFIG_SND_SOC_CODEC_DEBUG
static struct hicodec_dump_reg_entry asp_codec_dump_table[] = {
	{ "SOC CODEC", DBG_SOCCODEC_START_ADDR, DBG_SOCCODEC_END_ADDR, 4 },
};

static struct hicodec_dump_reg_info asp_codec_dump_info = {
	.entry = asp_codec_dump_table,
	.count = ARRAY_SIZE(asp_codec_dump_table),
};
#endif

static struct snd_soc_component *soc_codec = NULL;

static void pop_process(void)
{
	/* set audio down pga -120db */
	asp_codec_reg_update(AUDIO_L_DN_PGA_CTRL_REG,
		MAX_VAL_ON_BIT(8) << AUDIO_L_DN_PGA_GAIN_OFFSET,
		0x88U << AUDIO_L_DN_PGA_GAIN_OFFSET);
	asp_codec_reg_update(AUDIO_R_DN_PGA_CTRL_REG,
		MAX_VAL_ON_BIT(8) << AUDIO_R_DN_PGA_GAIN_OFFSET,
		0x88U << AUDIO_R_DN_PGA_GAIN_OFFSET);

	/* set codec3 down pga -120db */
	asp_codec_reg_update(CODEC3_L_DN_PGA_CTRL_REG,
		MAX_VAL_ON_BIT(8) << CODEC3_L_DN_PGA_GAIN_OFFSET,
		0x88U << CODEC3_L_DN_PGA_GAIN_OFFSET);
	asp_codec_reg_update(CODEC3_R_DN_PGA_CTRL_REG,
		MAX_VAL_ON_BIT(8) << CODEC3_R_DN_PGA_GAIN_OFFSET,
		0x88U << CODEC3_R_DN_PGA_GAIN_OFFSET);

	/* set dacl mixer2 pga -12db */
	asp_codec_reg_update(DACL_MIXER4_CTRL0_REG,
		MAX_VAL_ON_BIT(2) << DACL_MIXER4_GAIN1_OFFSET,
		0x0 << DACL_MIXER4_GAIN1_OFFSET);
	asp_codec_reg_update(DACL_MIXER4_CTRL0_REG,
		MAX_VAL_ON_BIT(2) << DACL_MIXER4_GAIN2_OFFSET,
		0x0 << DACL_MIXER4_GAIN2_OFFSET);
	asp_codec_reg_update(DACL_MIXER4_CTRL0_REG,
		MAX_VAL_ON_BIT(2) << DACL_MIXER4_GAIN3_OFFSET,
		0x0 << DACL_MIXER4_GAIN3_OFFSET);
	asp_codec_reg_update(DACL_MIXER4_CTRL0_REG,
		MAX_VAL_ON_BIT(2) << DACL_MIXER4_GAIN4_OFFSET,
		0x0 << DACL_MIXER4_GAIN4_OFFSET);

	/* set dacr mixer2 pga -12db */
	asp_codec_reg_update(DACR_MIXER4_CTRL0_REG,
		MAX_VAL_ON_BIT(2) << DACR_MIXER4_GAIN1_OFFSET,
		0x0 << DACR_MIXER4_GAIN1_OFFSET);
	asp_codec_reg_update(DACL_MIXER4_CTRL0_REG,
		MAX_VAL_ON_BIT(2) << DACR_MIXER4_GAIN2_OFFSET,
		0x0 << DACR_MIXER4_GAIN2_OFFSET);
	asp_codec_reg_update(DACL_MIXER4_CTRL0_REG,
		MAX_VAL_ON_BIT(2) << DACR_MIXER4_GAIN3_OFFSET,
		0x0 << DACR_MIXER4_GAIN3_OFFSET);
	asp_codec_reg_update(DACL_MIXER4_CTRL0_REG,
		MAX_VAL_ON_BIT(2) << DACR_MIXER4_GAIN4_OFFSET,
		0x0 << DACR_MIXER4_GAIN4_OFFSET);
}

static void config_gpio_pinctrl(void)
{
	struct snd_soc_component *codec = asp_codec_get_codec();

	if (codec == NULL) {
		AUDIO_LOGE("parameter is null");
		return;
	}

	AUDIO_LOGI("set gpio pinctrl");

	/* i2s1 DI */
	asp_codec_reg_write(codec, IOC_IOMG_095, 0x1);
	/* i2s1 DO */
	asp_codec_reg_write(codec, IOC_IOMG_096, 0x1);
	/* i2s1 XCLK */
	asp_codec_reg_write(codec, IOC_IOMG_097, 0x1);
	/* i2s1 XFS */
	asp_codec_reg_write(codec, IOC_IOMG_098, 0x1);

	/* i2s3 DI */
	asp_codec_reg_write(codec, IOC_IOMG_099, 0x1);
	/* i2s3 DO */
	asp_codec_reg_write(codec, IOC_IOMG_100, 0x1);
	/* i2s3 XCLK */
	asp_codec_reg_write(codec, IOC_IOMG_101, 0x1);
	/* i2s3 XFS */
	asp_codec_reg_write(codec, IOC_IOMG_102, 0x1);

	/* DMIC0 CLK */
	asp_codec_reg_write(codec, IOC_IOMG_SE_22, 0x4);
	/* DMIC0 DIN */
	asp_codec_reg_write(codec, IOC_IOMG_SE_23, 0x4);

	/* DMIC1 CLK */
	asp_codec_reg_write(codec, IOC_IOMG_SE_24, 0x4);
	/* DMIC1 DIN */
	asp_codec_reg_write(codec, IOC_IOMG_SE_25, 0x4);
}

static int asp_codec_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	return 0;
}

static int asp_codec_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	return 0;
}

struct snd_soc_dai_ops asp_codec_dai_ops = {
	.startup   = asp_codec_startup,
	.hw_params = asp_codec_hw_params,
};

struct snd_soc_dai_driver asp_codec_dai[] = {
	{
		.name = "asp-codec-dai",
		.playback = {
			.stream_name = "Playback",
			.channels_min = PB_MIN_CHANNELS,
			.channels_max = PB_MAX_CHANNELS,
			.rates = PCM_RATES,
			.formats = PCM_FORMATS
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = CP_MIN_CHANNELS,
			.channels_max = CP_MAX_CHANNELS,
			.rates = PCM_RATES,
			.formats = PCM_FORMATS
		},
		.ops = &asp_codec_dai_ops,
	},
};

static void reset_asp_codec(struct snd_soc_component *codec)
{
	asp_codec_reg_write(codec, R_RST_CTRLEN, BIT(RST_EN_CODEC_N));
	asp_codec_reg_write(codec, R_RST_CTRLDIS, BIT(RST_DISEN_CODEC_N));
	asp_codec_reg_write(codec, R_GATE_EN, BIT(GT_CODEC_CLK));
	asp_codec_set_reg_bits(R_CODEC_DMA_SEL, 1 << CODEC_DMA_SEL);
}

static void asp_reg_init(struct snd_soc_component *codec)
{
	reset_asp_codec(codec);

	/* ADC PGA BYPASS */
	asp_codec_set_reg_bits(ADC1_UP_PGA_CTRL_REG,
		1 << ADC1_UP_PGA_BYPASS_OFFSET);
	asp_codec_set_reg_bits(ADC2_UP_PGA_CTRL_REG,
		1 << ADC2_UP_PGA_BYPASS_OFFSET);
	asp_codec_set_reg_bits(ADC3_UP_PGA_CTRL_REG,
		1 << ADC3_UP_PGA_BYPASS_OFFSET);
	asp_codec_set_reg_bits(ADC4_UP_PGA_CTRL_REG,
		1 << ADC4_UP_PGA_BYPASS_OFFSET);
	/* set i2s2 frq 8k */
	asp_codec_reg_update(I2S2_PCM_CTRL_REG,
		MAX_VAL_ON_BIT(3) << FS_I2S2_OFFSET,
		0x0 << FS_I2S2_OFFSET);
}

static void set_platform_type(struct snd_soc_component *codec)
{
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(codec);
	struct device_node *np = codec->dev->of_node;
	unsigned int val = 0;

	if (!of_property_read_u32(np, "udp_io_config", &val)) {
		AUDIO_LOGI("udp io config is %d", val);

		if (val == 1)
			priv->platform_type = UDP_PLATFORM;
		else
			priv->platform_type = FPGA_PLATFORM;
	} else {
		AUDIO_LOGI("no configed platform type, set fpga");
		priv->platform_type = FPGA_PLATFORM;
	}
}

static int asp_resource_init(struct snd_soc_component *codec)
{
	int ret;
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(codec);

	priv->asp_regulator = devm_regulator_get(codec->dev, "asp");
	if (IS_ERR_OR_NULL(priv->asp_regulator)) {
		ret = PTR_ERR(priv->asp_regulator);
		AUDIO_LOGE("get asp regulators err:%d", ret);
		goto get_asp_regulator_err;
	}

	priv->asp_subsys_clk = devm_clk_get(codec->dev, "clk_asp_subsys");
	if (IS_ERR_OR_NULL(priv->asp_subsys_clk)) {
		ret = PTR_ERR(priv->asp_subsys_clk);
		AUDIO_LOGE("get clk asp subsys err:%d", ret);
		goto get_asp_subsys_clk_err;
	}

	priv->asp_49m_clk = devm_clk_get(codec->dev, "clk_asp_codec");
	if (IS_ERR_OR_NULL(priv->asp_49m_clk)) {
		ret = PTR_ERR(priv->asp_49m_clk);
		AUDIO_LOGE("get clk err:%d", ret);
		goto get_asp_49m_clk_err;
	}

	AUDIO_LOGI("asp resource init ok");

	return 0;

get_asp_49m_clk_err:
	priv->asp_49m_clk = NULL;
get_asp_subsys_clk_err:
	priv->asp_subsys_clk = NULL;
get_asp_regulator_err:
	priv->asp_regulator = NULL;

	return ret;
}

static void asp_resource_deinit(struct snd_soc_component *codec)
{
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(codec);

	priv->asp_regulator = NULL;
	priv->asp_subsys_clk = NULL;
	priv->asp_49m_clk = NULL;

	AUDIO_LOGI("asp resource deinit ok");
}

static int asp_regulator_enable(struct snd_soc_component *codec)
{
	int ret;
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(codec);

	ret = regulator_enable(priv->asp_regulator);
	if (ret) {
		AUDIO_LOGE("asp regulator enable failed, ret:%d", ret);
		return ret;
	}

	AUDIO_LOGI("asp regulator enable done");

	return ret;
}

static void asp_regulator_disable(struct snd_soc_component *codec)
{
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(codec);

	if (!IS_ERR_OR_NULL(priv->asp_regulator)) {
		if (regulator_disable(priv->asp_regulator)) {
			AUDIO_LOGE("can not disable asp regulator");
			return;
		}

		AUDIO_LOGI("asp regulator disable ok");
	}
}

static int asp_clk_enable(struct snd_soc_component *codec)
{
	int ret;
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(codec);

	ret = clk_prepare_enable(priv->asp_subsys_clk);
	if (ret) {
		AUDIO_LOGE("asp subsys clk enable failed, ret:%d", ret);
		return ret;
	}

	ret = clk_prepare_enable(priv->asp_49m_clk);
	if (ret) {
		AUDIO_LOGE("asp 49m clk enable failed, ret:%d", ret);
		clk_disable_unprepare(priv->asp_subsys_clk);
		return ret;
	}

	AUDIO_LOGI("asp clk enable ok");

	return ret;
}

static void asp_clk_disable(struct snd_soc_component *codec)
{
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(codec);

	if (!IS_ERR_OR_NULL(priv->asp_49m_clk)) {
		clk_disable_unprepare(priv->asp_49m_clk);
		AUDIO_LOGI("asp 49m clk disable ok");
	}

	if (!IS_ERR_OR_NULL(priv->asp_subsys_clk)) {
		clk_disable_unprepare(priv->asp_subsys_clk);
		AUDIO_LOGI("asp subsys clk disable ok");
	}
}

static int asp_resource_enable(struct snd_soc_component *codec)
{
	int ret;

	ret = asp_regulator_enable(codec);
	if (ret) {
		AUDIO_LOGE("asp regulator enable failed, ret:%d", ret);
		return ret;
	}

	ret = asp_clk_enable(codec);
	if (ret) {
		AUDIO_LOGE("asp clk enable failed, ret:%d", ret);
		asp_regulator_disable(codec);
		return ret;
	}

	AUDIO_LOGI("asp resource enable ok");

	return 0;
}

static void asp_resource_disable(struct snd_soc_component *codec)
{
	asp_clk_disable(codec);
	asp_regulator_disable(codec);
}

static int codec_pinctrl_init(struct snd_soc_component *codec)
{
	int ret;
	struct device *dev = codec->dev;
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(codec);

	/* set gpio I2S1 & I2S2 & DMIC1 & DMIC2 */
	priv->pctrl = pinctrl_get(dev);
	if (IS_ERR_OR_NULL(priv->pctrl)) {
		priv->pctrl = NULL;
		AUDIO_LOGE("can not get pinctrl");
		return -EFAULT;
	}

	priv->pin_default = pinctrl_lookup_state(priv->pctrl,
		PINCTRL_STATE_DEFAULT);
	if (IS_ERR_OR_NULL(priv->pin_default)) {
		AUDIO_LOGE("can not get default state, errno:%li",
			PTR_ERR(priv->pin_default));
		goto pinctrl_operation_err;
	}

	priv->pin_idle = pinctrl_lookup_state(priv->pctrl, PINCTRL_STATE_IDLE);
	if (IS_ERR_OR_NULL(priv->pin_idle)) {
		AUDIO_LOGE("can not get idle state, errno:%li",
			PTR_ERR(priv->pin_idle));
		goto pinctrl_operation_err;
	}

	ret = pinctrl_select_state(priv->pctrl, priv->pin_default);
	if (ret) {
		AUDIO_LOGE("set pinctrl state failed, ret:%d", ret);
		goto pinctrl_operation_err;
	}

	AUDIO_LOGI("pinctrl init ok");

	return 0;

pinctrl_operation_err:
	priv->pin_idle = NULL;
	priv->pin_default = NULL;
	devm_pinctrl_put(priv->pctrl);
	priv->pctrl = NULL;

	return -EFAULT;
}

static void codec_pinctrl_deinit(struct snd_soc_component *codec)
{
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(codec);

	devm_pinctrl_put(priv->pctrl);
	priv->pctrl = NULL;
	priv->pin_idle = NULL;
	priv->pin_default = NULL;

	AUDIO_LOGI("pinctrl deinit ok");
}

static void codec_pm_runtime_init(struct snd_soc_component *codec)
{
	struct device_node *np = codec->dev->of_node;
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(codec);

	if (of_property_read_bool(np, "pm_runtime_support"))
		priv->pm_runtime_support = true;
	else
		priv->pm_runtime_support = false;

	AUDIO_LOGI("pm runtime support:%d", priv->pm_runtime_support);

	if (priv->pm_runtime_support) {
		pm_runtime_use_autosuspend(codec->dev);
		pm_runtime_set_autosuspend_delay(codec->dev,
			AUTOSUSPEND_DELAY_TIME);
		pm_runtime_set_active(codec->dev);
		pm_runtime_enable(codec->dev);
	}

	AUDIO_LOGI("pm runtrime init ok");
}

static void codec_pm_runtime_deinit(struct snd_soc_component *codec)
{
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(codec);

	if (priv->pm_runtime_support) {
		pm_runtime_resume(codec->dev);
		pm_runtime_disable(codec->dev);
		pm_runtime_set_suspended(codec->dev);
	}

	AUDIO_LOGI("pm runtrime deinit ok");
}

static int codec_resource_init(struct snd_soc_component *codec)
{
	int ret = 0;
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(codec);

	set_platform_type(codec);

	ret = asp_resource_init(codec);
	if (ret) {
		AUDIO_LOGE("asp resource init error, ret:%d", ret);
		return ret;
	}

	ret = asp_resource_enable(codec);
	if (ret) {
		AUDIO_LOGE("asp resource enable error, ret:%d", ret);
		goto asp_resource_enable_err;
	}

	if (priv->platform_type == FPGA_PLATFORM) {
		config_gpio_pinctrl();
	} else {
		ret = codec_pinctrl_init(codec);
		if (ret) {
			AUDIO_LOGE("pinctrl init error, ret:%d", ret);
			goto pinctrl_init_err;
		}
	}

	codec_pm_runtime_init(codec);
	asp_reg_init(codec);
	AUDIO_LOGI("resource init ok");

	return 0;

pinctrl_init_err:
	asp_resource_disable(codec);
asp_resource_enable_err:
	asp_resource_deinit(codec);

	return ret;
}

static void codec_resource_deinit(struct snd_soc_component *codec)
{
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(codec);

	codec_pm_runtime_deinit(codec);

	if (priv->platform_type != FPGA_PLATFORM)
		codec_pinctrl_deinit(codec);

	asp_resource_disable(codec);
	asp_resource_deinit(codec);

	AUDIO_LOGI("resource deinit ok");
}

/*lint -e429*/
static int codec_priv_init(struct snd_soc_component *codec)
{
	struct asp_codec_priv *priv = NULL;
	struct device *dev = codec->dev;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (priv == NULL) {
		AUDIO_LOGE("priv devm kzalloc failed");
		return -ENOMEM;
	}

	snd_soc_component_set_drvdata(codec, priv);

	soc_codec = codec;
	priv->codec = codec;
	priv->v_codec_reg[0] = 0;
	priv->v_codec_reg[1] = 0;
	priv->voiceup_state = TRACK_FREE;
	priv->audioup_4mic_state = TRACK_FREE;
	spin_lock_init(&priv->lock);

	AUDIO_LOGI("priv init ok");

	return 0;
}
/*lint +e429*/

static void codec_priv_deinit(struct snd_soc_component *codec)
{
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(codec);

	if (priv != NULL)
		snd_soc_component_set_drvdata(codec, NULL);

	soc_codec = NULL;

	AUDIO_LOGI("priv deinit ok");
}

static int add_all_widgets(bool single_mode, struct snd_soc_dapm_context *dapm)
{
	if (single_mode) {
		if (asp_codec_add_single_switch_widgets(dapm) != 0) {
			AUDIO_LOGE("add single switch widgets failed");
			return -EINVAL;
		}
	} else {
		if (asp_codec_add_switch_widgets(dapm) != 0) {
			AUDIO_LOGE("add switch widgets failed");
			return -EINVAL;
		}
	}

	if (asp_codec_add_resource_widgets(dapm) != 0) {
		AUDIO_LOGE("add resource widgets failed");
		return -EINVAL;
	}

	if (asp_codec_add_path_widgets(dapm) != 0) {
		AUDIO_LOGE("add path widgets failed");
		return -EINVAL;
	}

	return 0;
}

static int add_all_kcontrols(bool single_mode, struct snd_soc_component *codec)
{
	if (single_mode) {
		if (asp_codec_add_single_kcontrols(codec) != 0) {
			AUDIO_LOGE("add single kcontrols failed");
			return -EINVAL;
		}
	} else {
		if (asp_codec_add_kcontrols(codec) != 0) {
			AUDIO_LOGE("add kcontrols failed");
			return -EINVAL;
		}
	}

	return 0;
}

static int add_all_routes(bool single_mode, struct snd_soc_dapm_context *dapm)
{
	if (single_mode) {
		if (asp_codec_add_single_routes(dapm) != 0) {
			AUDIO_LOGE("add single routes failed");
			return -EINVAL;
		}
	} else {
		if (asp_codec_add_routes(dapm) != 0) {
			AUDIO_LOGE("add routes failed");
			return -EINVAL;
		}
	}

	return 0;
}

static int init_component_driver(struct snd_soc_component *codec,
	struct snd_soc_dapm_context *dapm)
{
	int ret;
	bool is_single_mode = false;
	struct device_node *np = codec->dev->of_node;

	if (of_property_read_bool(np, "single_mode_enable"))
		is_single_mode = true;

	ret = add_all_kcontrols(is_single_mode, codec);
	if (ret) {
		AUDIO_LOGE("add all kcontrols failed, ret:%d", ret);
		return -EINVAL;
	}

	ret = add_all_widgets(is_single_mode, dapm);
	if (ret) {
		AUDIO_LOGE("add all widgets failed, ret:%d", ret);
		return -EINVAL;
	}

	ret = add_all_routes(is_single_mode, dapm);
	if (ret) {
		AUDIO_LOGE("add all routes failed, ret:%d", ret);
		return -EINVAL;
	}

	return ret;
}

static void _read_dts_config(struct snd_soc_component *codec)
{
	struct device_node *node = NULL;
	struct device_node *np = codec->dev->of_node;
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(codec);

	node = of_parse_phandle(np, "asp_dmac_handle", 0);
	if (node) {
		if (of_property_read_u32(node, "port_config_type", &priv->port_cfg_type)) {
			AUDIO_LOGE("get port config type failed, default set Codecless");
			priv->port_cfg_type = PORT_CODECLESS;
		}
	} else {
		AUDIO_LOGE("get asp_dmac_handle failed");
	}
}

static int asp_codec_codec_probe(struct snd_soc_component *codec)
{
	struct snd_soc_dapm_context *dapm = NULL;
	int ret;

	IN_FUNCTION;

	if (codec == NULL) {
		AUDIO_LOGE("parameter is null");
		return -EINVAL;
	}

	dapm = snd_soc_component_get_dapm(codec);
	if (dapm == NULL) {
		AUDIO_LOGE("dapm is null");
		return -EINVAL;
	}

	ret = codec_priv_init(codec);
	if (ret) {
		AUDIO_LOGE("priv init failed, ret:%d", ret);
		goto end;
	}

	ret = codec_resource_init(codec);
	if (ret) {
		AUDIO_LOGE("resource init failed, ret:%d", ret);
		goto resource_init_err;
	}

	ret = snd_soc_dapm_sync(dapm);
	if (ret) {
		AUDIO_LOGE("dapm sync error, errno:%d", ret);
		goto dapm_sync_err;
	}

	_read_dts_config(codec);

	ret = init_component_driver(codec, dapm);
	if (ret) {
		AUDIO_LOGE("init component driver failed, ret:%d", ret);
		goto dapm_sync_err;
	}

#ifdef CONFIG_SND_SOC_CODEC_DEBUG
#ifdef CONFIG_AUDIO_COMMON_IMAGE
	bind_dump_reg_info(&asp_codec_dump_info);
#endif
	ret = hicodec_debug_init(codec, &asp_codec_dump_info);
	if (ret) {
		AUDIO_LOGE("debug init error, errno:%d", ret);
		goto dapm_sync_err;
	}
#endif

	goto end;

dapm_sync_err:
	codec_resource_deinit(codec);
resource_init_err:
	codec_priv_deinit(codec);
end:
	OUT_FUNCTION;

	return ret;
}

static void asp_codec_codec_remove(struct snd_soc_component *codec)
{
	IN_FUNCTION;

	if (codec == NULL) {
		AUDIO_LOGE("parameter is null");
		return;
	}

#ifdef CONFIG_SND_SOC_CODEC_DEBUG
	hicodec_debug_uninit(codec);
#endif

	codec_resource_deinit(codec);
	codec_priv_deinit(codec);

	OUT_FUNCTION;

	return;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
static struct snd_soc_component_driver soc_codec_dev_asp_codec = {
	.name = ASP_CODEC_NAME,
	.probe = asp_codec_codec_probe,
	.remove = asp_codec_codec_remove,
	.read = asp_codec_reg_read,
	.write = asp_codec_reg_write,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	.idle_bias_on = false,
#else
	.idle_bias_off = true
#endif
};
#else
static struct snd_soc_codec_driver soc_codec_dev_asp_codec = {
	.read = asp_codec_reg_read_by_codec,
	.write = asp_codec_reg_write_by_codec,
	.idle_bias_off = true,
	.component_driver.name = ASP_CODEC_NAME,
	.component_driver.probe = asp_codec_codec_probe,
	.component_driver.remove = asp_codec_codec_remove,
};
#endif
static int asp_codec_probe(struct platform_device *pdev)
{
	int ret;

	IN_FUNCTION;

	ret = asp_codec_base_addr_map(pdev);
	if (ret) {
		AUDIO_LOGE("base addr map failed, err code 0x%x", ret);
		asp_codec_base_addr_unmap();
		return ret;
	}

	dev_set_name(&pdev->dev, "asp-codec");
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
	ret = devm_snd_soc_register_component(&pdev->dev, &soc_codec_dev_asp_codec,
		asp_codec_dai, ARRAY_SIZE(asp_codec_dai));
#else
	ret = snd_soc_register_codec(&pdev->dev, &soc_codec_dev_asp_codec,
		asp_codec_dai, ARRAY_SIZE(asp_codec_dai));
#endif
	if (ret) {
		AUDIO_LOGE("regist failed, err code 0x%x", ret);
		asp_codec_base_addr_unmap();
	}

	OUT_FUNCTION;

	return ret;
}

static int asp_codec_remove(struct platform_device *pdev)
{
	IN_FUNCTION;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0))
	snd_soc_unregister_codec(&pdev->dev);
#endif
	asp_codec_base_addr_unmap();

	OUT_FUNCTION;

	return 0;
}

static void asp_codec_shutdown(struct platform_device *dev)
{
	struct asp_codec_priv *priv = NULL;

	IN_FUNCTION;

	if (soc_codec == NULL) {
		AUDIO_LOGE("parameter is null");
		return;
	}

	priv = snd_soc_component_get_drvdata(soc_codec);
	if (priv == NULL) {
		AUDIO_LOGE("priv is null");
		return;
	}

	pop_process();

	OUT_FUNCTION;
}

#ifdef CONFIG_PM
static int asp_codec_runtime_suspend(struct device *dev)
{
	int ret;
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(soc_codec);

	IN_FUNCTION;

	if (priv == NULL) {
		AUDIO_LOGE("get drvdata failed");
		return -EINVAL;
	}

	ret = pinctrl_select_state(priv->pctrl, priv->pin_idle);
	if (ret) {
		AUDIO_LOGE("set pinctrl state failed, ret:%d", ret);
		return ret;
	}

	asp_clk_disable(soc_codec);

	OUT_FUNCTION;

	return 0;
}

static int asp_codec_runtime_resume(struct device *dev)
{
	int ret;
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(soc_codec);

	IN_FUNCTION;

	if (priv == NULL) {
		AUDIO_LOGE("get drvdata failed");
		return -EINVAL;
	}

	ret = asp_clk_enable(soc_codec);
	if (ret) {
		AUDIO_LOGE("can not enable asp resource, ret:%d", ret);
		return ret;
	}

	ret = pinctrl_select_state(priv->pctrl, priv->pin_default);
	if (ret) {
		AUDIO_LOGE("set pinctrl state failed, ret:%d", ret);
		return ret;
	}

	OUT_FUNCTION;

	return 0;
}
#endif

#ifdef CONFIG_PM_SLEEP
static int asp_codec_suspend(struct device *dev)
{
	int ret;
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(soc_codec);

	IN_FUNCTION;

	if (dev == NULL) {
		AUDIO_LOGE("device is null");
		return -EINVAL;
	}

	if (priv == NULL) {
		AUDIO_LOGE("get drvdata failed");
		return -EINVAL;
	}

	if (priv->pm_runtime_support) {
		ret = pm_runtime_get_sync(dev);
		AUDIO_LOGD("pm suspend ret:%d", ret);
		if (ret < 0) {
			AUDIO_LOGE("pm suspend error, ret:%d", ret);
			return ret;
		}
	}

	AUDIO_LOGD("suspend usage count:%d, runtime status:0x%x",
		atomic_read(&(dev->power.usage_count)),
		dev->power.runtime_status);
	AUDIO_LOGD("disable depth:%d, asp subsys clk:%d, 49m clk:%d",
		dev->power.disable_depth,
		clk_get_enable_count(priv->asp_subsys_clk),
		clk_get_enable_count(priv->asp_49m_clk));

	if (!priv->have_dapm) {
		/* set pin to low power mode */
		ret = pinctrl_select_state(priv->pctrl, priv->pin_idle);
		if (ret) {
			AUDIO_LOGE("set pin state failed, ret:%d", ret);
			return ret;
		}

		asp_resource_disable(soc_codec);
		priv->asp_pd = true;

		AUDIO_LOGI("suspend without dapm");
	}

	OUT_FUNCTION;

	return 0;
}

static int asp_codec_resume(struct device *dev)
{
	int ret;
	struct asp_codec_priv *priv = snd_soc_component_get_drvdata(soc_codec);

	IN_FUNCTION;

	if (dev == NULL) {
		AUDIO_LOGE("device is null");
		return -EINVAL;
	}

	if (priv == NULL) {
		AUDIO_LOGE("get drvdata failed");
		return -EINVAL;
	}

	if (priv->asp_pd) {
		ret = asp_resource_enable(soc_codec);
		if (ret) {
			AUDIO_LOGE("asp resource enable failed, ret:%d", ret);
			return ret;
		}

		priv->asp_pd = false;

		asp_reg_init(soc_codec);

		ret = pinctrl_select_state(priv->pctrl, priv->pin_default);
		if (ret) {
			AUDIO_LOGE("set pin state failed, ret:%d", ret);
			return ret;
		}
		AUDIO_LOGI("resume from asp pd");
	}

	if (priv->pm_runtime_support) {
		pm_runtime_mark_last_busy(dev);
		pm_runtime_put_autosuspend(dev);

		pm_runtime_disable(dev);
		pm_runtime_set_active(dev);
		pm_runtime_enable(dev);
	}

	AUDIO_LOGD("resume usage count:%d, status:0x%x",
		atomic_read(&(dev->power.usage_count)),
		dev->power.runtime_status);
	AUDIO_LOGD("disable depth:%d, asp subsys clk:%d, 49m clk:%d",
		dev->power.disable_depth,
		clk_get_enable_count(priv->asp_subsys_clk),
		clk_get_enable_count(priv->asp_49m_clk));

	OUT_FUNCTION;

	return 0;
}
#endif

struct snd_soc_component *asp_codec_get_codec(void)
{
	return soc_codec;
}

const struct dev_pm_ops asp_codec_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(asp_codec_suspend, asp_codec_resume)
	SET_RUNTIME_PM_OPS(asp_codec_runtime_suspend,
		asp_codec_runtime_resume, NULL)
};

static const struct of_device_id asp_codec_match[] = {
	{ .compatible = "hisilicon,asp-codec", },
	{},
};

static struct platform_driver asp_codec_driver = {
	.driver = {
		.name  = "asp-codec",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(asp_codec_match),
		.pm = &asp_codec_pm_ops,
	},
	.probe  = asp_codec_probe,
	.remove = asp_codec_remove,
	.shutdown = asp_codec_shutdown,
};

static int __init asp_analog_less_v1_init(void)
{
	return platform_driver_register(&asp_codec_driver);
}
module_init(asp_analog_less_v1_init);

static void __exit asp_analog_less_v1_exit(void)
{
	IN_FUNCTION;

	platform_driver_unregister(&asp_codec_driver);

	OUT_FUNCTION;
}
module_exit(asp_analog_less_v1_exit);

MODULE_DESCRIPTION("ASoC asp codec driver");
MODULE_LICENSE("GPL");
