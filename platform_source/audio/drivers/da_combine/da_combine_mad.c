/*
 * da_combine_mad.c
 *
 * da_combine mad driver
 *
 * Copyright (c) 2019-2020 Huawei Technologies Co., Ltd.
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

#include <linux/platform_drivers/da_combine/da_combine_mad.h>

#include <linux/platform_drivers/da_combine_dsp/da_combine_dsp_misc.h>
#include <om_debug.h>

#include "audio_log.h"

#define LOG_TAG "DA_combine_mad"

enum pcm_hook_status {
	PCM_HOOK_START,
	PCM_HOOK_STOP
};

struct mad_data {
	struct da_combine_irq *irq;
	enum pcm_hook_status pcm_hook_flag;
};

static struct mad_data *g_mad_data;

static irqreturn_t mad_trigger_handler(int irq, void *data)
{
	if (g_mad_data == NULL)
		return IRQ_HANDLED;

	if (g_mad_data->pcm_hook_flag == PCM_HOOK_START) {
		AUDIO_LOGI("wake up hook already started");
		return IRQ_HANDLED;
	}

	da_combine_wakeup_pcm_hook_start();
	g_mad_data->pcm_hook_flag = PCM_HOOK_START;

	return IRQ_HANDLED;
}

void da_combine_hook_pcm_handle(void)
{
	if (g_mad_data == NULL) {
		AUDIO_LOGE("mad data is null");
		return;
	}

	if (g_mad_data->pcm_hook_flag == PCM_HOOK_START) {
		da_combine_wakeup_pcm_hook_stop();
		g_mad_data->pcm_hook_flag = PCM_HOOK_STOP;
		AUDIO_LOGI("wake up hook stopped");
	}
}

void da_combine_mad_request_irq(void)
{
	int ret;

	if (g_mad_data == NULL) {
		AUDIO_LOGE("mad data is null");
		return;
	}

	ret = da_combine_irq_request_irq(g_mad_data->irq, IRQ_MAD,
		mad_trigger_handler, "mad_triger", g_mad_data);
	if (ret < 0)
		AUDIO_LOGE("request irq for mad triger err");
}

void da_combine_mad_free_irq(void)
{
	if (g_mad_data == NULL) {
		AUDIO_LOGE("mad data is null");
		return;
	}

	if (g_mad_data->irq != NULL)
		da_combine_irq_free_irq(g_mad_data->irq, IRQ_MAD, g_mad_data);
}

int da_combine_mad_init(struct da_combine_irq *irq)
{
	AUDIO_LOGI("enter");

	if (irq == NULL) {
		AUDIO_LOGE("irq is null");
		return -EINVAL;
	}

	g_mad_data = kzalloc(sizeof(*g_mad_data), GFP_KERNEL);
	if (g_mad_data == NULL) {
		AUDIO_LOGE("cannot allocate mad data");
		return -ENOMEM;
	}

	g_mad_data->irq = irq;
	g_mad_data->pcm_hook_flag = PCM_HOOK_STOP;

	AUDIO_LOGI("ok");

	return 0;
}

void da_combine_mad_deinit(void)
{
	if (g_mad_data == NULL) {
		AUDIO_LOGE("mad data is null");
		return;
	}

	kfree(g_mad_data);
	g_mad_data = NULL;
}

