/*
 * da_combine_vad.c
 *
 * codec vad driver
 *
 * Copyright (c) 2014-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/platform_drivers/da_combine/da_combine_vad.h>

#include <linux/mutex.h>
#include <sound/soc.h>
#include <linux/jiffies.h>
#include <linux/device.h>
#include <linux/pm_wakeup.h>
#include <linux/version.h>
#include <linux/platform_drivers/da_combine/da_combine_irq.h>
#include <da_combine_algo_interface.h>
#include <linux/platform_drivers/da_combine/da_combine_regs.h>
#include <linux/platform_drivers/da_combine/da_combine_mad.h>

#include "audio_log.h"
#include "soundtrigger_event.h"

#define LOG_TAG "DA_combine_vad"

#define DA_COMBINE_V2_DSP_VAD_CMD (BASE_ADDR_PAGE_CFG + 0x73)

struct da_combine_vad_platform_data {
	struct snd_soc_component *codec;
	struct da_combine_irq *irq;
	struct wakeup_source *soundtrigger_wake_lock;
	bool fast_mode_enable;
};

static struct da_combine_vad_platform_data *g_vad_data;

static irqreturn_t da_combine_sound_trigger_handler(int irq, void *data)
{
	struct da_combine_vad_platform_data *pdata = data;

	__pm_wakeup_event(pdata->soundtrigger_wake_lock, 1000);

	/* clr VAD INTR */
	snd_soc_component_write(pdata->codec, DA_COMBINE_VAD_INT_SET, 0);
	snd_soc_component_write(pdata->codec, DA_COMBINE_REG_IRQ_1, BIT(DA_COMBINE_VAD_BIT));

	AUDIO_LOGI("soundtrigger event: %d, fast mode enable: %d",
		AUDIO_SOUNDTRIGGER_TRIGGER, g_vad_data->fast_mode_enable);

#ifdef CONFIG_SOUND_TRIGGER
	if (g_vad_data->fast_mode_enable == false)
		hw_soundtrigger_event_input(AUDIO_SOUNDTRIGGER_TRIGGER);
	else
		hw_soundtrigger_event_uevent(AUDIO_SOUNDTRIGGER_TRIGGER);
#endif

#ifdef ENABLE_DA_COMBINE_CODEC_DEBUG
	da_combine_hook_pcm_handle();
#endif

	return IRQ_HANDLED;
}

int da_combine_fast_mode_set(bool enable)
{
	if (g_vad_data == NULL) {
		AUDIO_LOGE("vad data is null");
		return -EINVAL;
	}

	g_vad_data->fast_mode_enable = enable;

	return 0;
}

int da_combine_vad_init(struct snd_soc_component *codec, struct da_combine_irq *irq)
{
	int ret;

	AUDIO_LOGI("enter");

	if (codec == NULL || codec->dev == NULL || codec->dev->of_node == NULL ||
		irq == NULL) {
		AUDIO_LOGE("param is NULL");
		return -EINVAL;
	}

	if (of_property_read_bool(codec->dev->of_node, "dsp_soundtrigger_disable")) {
		AUDIO_LOGI("do not need to init vad");
		return 0;
	}

	g_vad_data = kzalloc(sizeof(*g_vad_data), GFP_KERNEL);
	if (g_vad_data == NULL) {
		AUDIO_LOGE("cannot allocate vad platform data");
		return -ENOMEM;
	}

	g_vad_data->codec = codec;
	g_vad_data->irq = irq;
	g_vad_data->fast_mode_enable = false;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	g_vad_data->soundtrigger_wake_lock = wakeup_source_register(NULL,
		"da_combine_v2-soundtrigger");
#else
	g_vad_data->soundtrigger_wake_lock = wakeup_source_register("da_combine_v2-soundtrigger");
#endif
	if (g_vad_data->soundtrigger_wake_lock == NULL) {
		AUDIO_LOGE("request wakeup source failed");
		kfree(g_vad_data);
		g_vad_data = NULL;
		return -ENOMEM;
	}

	ret = da_combine_irq_request_irq(g_vad_data->irq, IRQ_VAD,
		da_combine_sound_trigger_handler, "sound_triger", g_vad_data);
	if (ret < 0) {
		AUDIO_LOGE("request irq for soundtrigger err");
		wakeup_source_unregister(g_vad_data->soundtrigger_wake_lock);
		kfree(g_vad_data);
		g_vad_data = NULL;
	}

	AUDIO_LOGI("exit");

	return ret;
}

int da_combine_vad_deinit(struct device_node *node)
{
	if (node == NULL || g_vad_data == NULL) {
		AUDIO_LOGE("param is NULL");
		return -EINVAL;
	}

	if (of_property_read_bool(node, "dsp_soundtrigger_disable")) {
		AUDIO_LOGI("dsp soundtrigger disable, not need deinit vad");
		return 0;
	}

	if (g_vad_data->irq != NULL)
		da_combine_irq_free_irq(g_vad_data->irq, IRQ_VAD, g_vad_data);

	wakeup_source_unregister(g_vad_data->soundtrigger_wake_lock);

	kfree(g_vad_data);
	g_vad_data = NULL;

	return 0;
}

