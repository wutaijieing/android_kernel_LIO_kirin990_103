/*
 * asp_codec_utils.c -- asp codec utils driver
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

#include "asp_codec_utils.h"
#include <linux/types.h>

#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/pm_runtime.h>
#include <linux/version.h>

#ifdef CONFIG_SND_SOC_CODEC_DEBUG
#include "codec_debug.h"
#endif
#include "audio_log.h"
#include "asp_codec.h"

#ifdef CONFIG_AUDIO_DEBUG
#define LOG_TAG "asp_codec"
#else
#define LOG_TAG "Analog_less_v1"
#endif

#define PAGE_TYPE_MASK    0xFF000
#define PAGE_VALUE_MASK   0x00000FFF
#define INVALID_REG_VALUE 0xFFFFFFFF

enum register_resource {
	SOCCODEC_REG = 0,
	ASPCFG_REG,
	AOIOC_REG,
	IOC_REG,
	REG_CNT
};

static void __iomem *reg_base_addr[REG_CNT] = { NULL };

static const struct asp_codec_reg_page reg_page_array[] = {
	{ PAGE_SOCCODEC, SOCCODEC_START_OFFSET, SOCCODEC_END_OFFSET,
		"PAGE_SOCCODEC" },
	{ PAGE_ASPCFG, ASPCFG_START_OFFSET, ASPCFG_END_OFFSET, "PAGE_ASPCFG" },
	{ PAGE_AO_IOC, AOIOC_START_OFFSET, AOIOC_END_OFFSET, "PAGE_AO_IOC" },
	{ PAGE_IOC, IOC_START_OFFSET, IOC_END_OFFSET, "PAGE_IOC" },
	{ PAGE_VIRCODEC, VIRCODEC_START_OFFSET, VIRCODEC_END_OFFSET,
		"PAGE_VIRCODEC" },
};

static bool check_reg_value_valid(struct asp_codec_priv *priv,
	unsigned int reg_type, unsigned int reg_value)
{
	bool is_valid = false;
	size_t i;
	size_t array_size = ARRAY_SIZE(reg_page_array);

	for (i = 0; i < array_size; i++) {
		if ((reg_type == reg_page_array[i].page_tag) &&
			((reg_value >= reg_page_array[i].page_reg_begin) &&
			(reg_value <= reg_page_array[i].page_reg_end))) {
			is_valid = true;
			break;
		}
	}

	if (is_valid) {
		if ((reg_type == PAGE_ASPCFG || reg_type == PAGE_SOCCODEC) &&
			priv->asp_pd) {
			AUDIO_LOGE("asp power down");
			is_valid = false;
		}
	} else {
		AUDIO_LOGE("reg type:0x%x, reg value:0x%x is invalid",
			reg_type, reg_value);
	}

	return is_valid;
}

static void runtime_info_print(struct asp_codec_priv *priv)
{
	struct device *dev = NULL;

	if (priv->codec == NULL) {
		AUDIO_LOGE("parameter is null");
		return;
	}

	dev = priv->codec->dev;
	if (dev == NULL) {
		AUDIO_LOGE("dev is null");
		return;
	}

	AUDIO_LOGD("suspend usage count:%d child count:%d",
		atomic_read(&(dev->power.usage_count)),
		atomic_read(&(dev->power.child_count)));
	AUDIO_LOGD("runtime status:0x%x disable depth:%d",
		dev->power.runtime_status, dev->power.disable_depth);
	AUDIO_LOGD("asp subsys clk:%d, asp 49m clk:%d",
		clk_get_enable_count(priv->asp_subsys_clk),
		clk_get_enable_count(priv->asp_49m_clk));
}

static int runtime_get_sync(struct asp_codec_priv *priv,
	unsigned int reg_type)
{
	struct device *dev = NULL;
	int pm_ret = 0;

	IN_FUNCTION;

	if (priv->codec == NULL) {
		AUDIO_LOGE("parameter is null");
		return -EINVAL;
	}

	if (priv->codec->dev == NULL) {
		AUDIO_LOGE("dev is null");
		return -EINVAL;
	}

	dev = priv->codec->dev;

	if (priv->pm_runtime_support &&
		(reg_type == PAGE_SOCCODEC || reg_type == PAGE_ASPCFG)) {
		pm_ret = pm_runtime_get_sync(dev);
		AUDIO_LOGD("get pm resume ret:%d", pm_ret);
		if (pm_ret < 0) {
			AUDIO_LOGE("pm resume error, ret:%d", pm_ret);
			return pm_ret;
		}

		runtime_info_print(priv);
	}

	OUT_FUNCTION;

	return pm_ret;
}

static void runtime_put_sync(struct asp_codec_priv *priv,
	unsigned int reg_type)
{
	struct device *dev = NULL;

	IN_FUNCTION;

	if (priv->codec == NULL) {
		AUDIO_LOGE("parameter is null");
		return;
	}

	if (priv->codec->dev == NULL) {
		AUDIO_LOGE("dev is null");
		return;
	}

	dev = priv->codec->dev;

	if (priv->pm_runtime_support &&
		(reg_type == PAGE_SOCCODEC || reg_type == PAGE_ASPCFG)) {
		pm_runtime_mark_last_busy(dev);
		pm_runtime_put_autosuspend(dev);
		runtime_info_print(priv);
	}

	OUT_FUNCTION;
}

static unsigned int _asp_codec_reg_read(struct asp_codec_priv *priv,
	unsigned int reg)
{
	unsigned int ret = 0;
	unsigned long flags;
	unsigned int reg_type = reg & PAGE_TYPE_MASK;
	unsigned int reg_value = reg & PAGE_VALUE_MASK;

	if (!check_reg_value_valid(priv, reg_type, reg_value)) {
		AUDIO_LOGE("invalid reg:0x%pK", (void *)(uintptr_t)reg);
		return INVALID_REG_VALUE;
	}

	if (runtime_get_sync(priv, reg_type) < 0) {
		AUDIO_LOGE("runtime resume failed");
		return INVALID_REG_VALUE;
	}

	spin_lock_irqsave(&priv->lock, flags);

	switch (reg_type) {
	case PAGE_SOCCODEC:
		ret = readl(reg_base_addr[SOCCODEC_REG] + reg_value);
		break;
	case PAGE_ASPCFG:
		ret = readl(reg_base_addr[ASPCFG_REG] + reg_value);
		break;
	case PAGE_AO_IOC:
		ret = readl(reg_base_addr[AOIOC_REG] + reg_value);
		break;
	case PAGE_IOC:
		if (priv->platform_type == FPGA_PLATFORM) {
			ret = readl(reg_base_addr[IOC_REG] + reg_value);
		} else {
			AUDIO_LOGE("platform type is not available");
			ret = INVALID_REG_VALUE;
		}
		break;
	case PAGE_VIRCODEC:
		ret = priv->v_codec_reg[reg_value];
		break;
	default:
		AUDIO_LOGE("reg = 0x%pK", (void *)(uintptr_t)reg);
		ret = INVALID_REG_VALUE;
		break;
	}

	spin_unlock_irqrestore(&priv->lock, flags);

	runtime_put_sync(priv, reg_type);

	return ret;
}

static void _asp_codec_reg_write(struct asp_codec_priv *priv,
	unsigned int reg, unsigned int value)
{
	unsigned long flags;
	unsigned int reg_type = reg & PAGE_TYPE_MASK;
	unsigned int reg_value = reg & PAGE_VALUE_MASK;

	if (!check_reg_value_valid(priv, reg_type, reg_value)) {
		AUDIO_LOGE("invalid reg:0x%pK, value:%d",
			(void *)(uintptr_t)reg, value);
		return;
	}

	if (runtime_get_sync(priv, reg_type) < 0) {
		AUDIO_LOGE("runtime resume failed");
		return;
	}

	spin_lock_irqsave(&priv->lock, flags);

#ifdef CONFIG_SND_SOC_CODEC_DEBUG
	hicodec_debug_reg_rw_cache(reg, value, HICODEC_DEBUG_FLAG_WRITE);
#endif

	switch (reg_type) {
	case PAGE_SOCCODEC:
		writel(value, reg_base_addr[SOCCODEC_REG] + reg_value);
		break;
	case PAGE_ASPCFG:
		writel(value, reg_base_addr[ASPCFG_REG] + reg_value);
		break;
	case PAGE_AO_IOC:
		writel(value, reg_base_addr[AOIOC_REG] + reg_value);
		break;
	case PAGE_IOC:
		if (priv->platform_type == FPGA_PLATFORM)
			writel(value,
				reg_base_addr[IOC_REG] + reg_value);
		else
			AUDIO_LOGE("platform type is not available");
		break;
	case PAGE_VIRCODEC:
		priv->v_codec_reg[reg_value] = value;
		break;
	default:
		AUDIO_LOGE("reg = 0x%pK, value:0x%x",
			(void *)(uintptr_t)reg, value);
		break;
	}
	spin_unlock_irqrestore(&priv->lock, flags);

	runtime_put_sync(priv, reg_type);
}

int asp_codec_base_addr_map(struct platform_device *pdev)
{
	struct resource *res = NULL;
	size_t i;
	size_t array_size = ARRAY_SIZE(reg_base_addr);

	IN_FUNCTION;

	if (pdev == NULL) {
		AUDIO_LOGE("pdev is null");
		return -EINVAL;
	}

	for (i = 0; i < array_size; i++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (res == NULL) {
			AUDIO_LOGE("platform get resource %zu err", i);
			return -ENOENT;
		}

		reg_base_addr[i] = (char * __force)(ioremap(res->start,
			resource_size(res)));
		if (reg_base_addr[i] == NULL) {
			AUDIO_LOGE("cannot map register memory");
			return -ENOMEM;
		}
	}

	OUT_FUNCTION;

	return 0;
}

void asp_codec_base_addr_unmap(void)
{
	size_t i;
	size_t array_size = ARRAY_SIZE(reg_base_addr);

	IN_FUNCTION;
	for (i = 0; i < array_size; i++) {
		if (reg_base_addr[i]) {
			iounmap(reg_base_addr[i]);
			reg_base_addr[i] = NULL;
		}
	}
	OUT_FUNCTION;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0))
unsigned int asp_codec_reg_read_by_codec(struct snd_soc_codec *codec,
	unsigned int reg)
{
	return asp_codec_reg_read(&codec->component, reg);
}

int asp_codec_reg_write_by_codec(struct snd_soc_codec *codec, unsigned int reg,
		unsigned int value)
{
	return asp_codec_reg_write(&codec->component, reg, value);
}
#endif

unsigned int asp_codec_reg_read(struct snd_soc_component *codec,
	unsigned int reg)
{
	unsigned int ret;
	struct asp_codec_priv *priv = NULL;

	IN_FUNCTION;

	if (codec == NULL) {
		AUDIO_LOGE("parameter is null");
		return INVALID_REG_VALUE;
	}

	priv = snd_soc_component_get_drvdata(codec);
	if (priv == NULL) {
		AUDIO_LOGE("priv is null");
		return INVALID_REG_VALUE;
	}

	ret = _asp_codec_reg_read(priv, reg);
	if (ret == INVALID_REG_VALUE)
		AUDIO_LOGW("reg 0x%pK read value 0x%x maybe invalid",
			(void *)(uintptr_t)reg, ret);

	return ret;
}

int asp_codec_reg_write(struct snd_soc_component *codec, unsigned int reg,
	unsigned int value)
{
	struct asp_codec_priv *priv = NULL;

	if (codec == NULL) {
		AUDIO_LOGE("parameter is null");
		return -EINVAL;
	}

	priv = snd_soc_component_get_drvdata(codec);
	if (priv == NULL) {
		AUDIO_LOGE("priv is null");
		return -EINVAL;
	}

	_asp_codec_reg_write(priv, reg, value);

	return 0;
}

int asp_codec_reg_update(unsigned int reg, unsigned int mask,
	unsigned int value)
{
	int change;
	unsigned int data_old, data_new;
	struct snd_soc_component *codec = asp_codec_get_codec();

	if (codec == NULL) {
		AUDIO_LOGE("parameter is null");
		return -EINVAL;
	}

	data_old = asp_codec_reg_read(codec, reg);
	data_new = (data_old & ~mask) | (value & mask);
	change = data_old != data_new;
	if (change) {
		if (asp_codec_reg_write(codec, reg, data_new) != 0) {
			AUDIO_LOGE("reg write failed");
			return -EINVAL;
		}
	}

	return change;
}

void asp_codec_set_reg_bits(unsigned int reg, unsigned int value)
{
	unsigned int val;
	struct snd_soc_component *codec = asp_codec_get_codec();

	if (codec == NULL) {
		AUDIO_LOGE("parameter is null");
		return;
	}

	val = asp_codec_reg_read(codec, reg) | (value);
	if (val != INVALID_REG_VALUE) {
		if (asp_codec_reg_write(codec, reg, val) != 0)
			AUDIO_LOGE("reg write failed");
	} else {
		AUDIO_LOGE("reg value read error:0x%x", val);
	}
}

void asp_codec_clr_reg_bits(unsigned int reg, unsigned int value)
{
	unsigned int val;
	struct snd_soc_component *codec = asp_codec_get_codec();

	if (codec == NULL) {
		AUDIO_LOGE("parameter is null");
		return;
	}

	val = asp_codec_reg_read(codec, reg) & ~(value);
	if (val != INVALID_REG_VALUE) {
		if (asp_codec_reg_write(codec, reg, val) != 0)
			AUDIO_LOGE("reg write failed");
	} else {
		AUDIO_LOGE("reg value read error:0x%x", val);
	}
}

