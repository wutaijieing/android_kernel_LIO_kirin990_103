/*
 * soundtrigger_pcm_drv_comm.c
 *
 * soundtrigger_pcm_drv_comm is a kernel driver common operation which is used to manager dma
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
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

#include "soundtrigger_pcm_drv_comm.h"

#include "audio_log.h"
#include "soundtrigger_pcm_drv.h"
#include "dsp_misc.h"
#include "dsp_utils.h"
#include "asp_dma.h"

#define LOG_TAG "soundtrigger_comm"

void stop_dma(int32_t pcm_channel, const struct soundtrigger_pcm *dma_drv_info)
{
	uint32_t i;
	uint32_t dma_port_num;
	uint32_t dma_channel;
	const struct soundtrigger_pcm_info *pcm_info = NULL;

	if (dma_drv_info == NULL)
		return;

	pcm_info = &(dma_drv_info->pcm_info[pcm_channel]);

	dma_port_num = ARRAY_SIZE(pcm_info->channel);
	for (i = 0; i < dma_port_num; i++) {
		dma_channel = pcm_info->channel[i];
		asp_dma_stop((unsigned short)dma_channel);
	}
}

void dump_dma_addr_info(const struct soundtrigger_pcm *dma_drv_info)
{
	uint32_t pcm_num;
	uint32_t dma_port_num;
	uint32_t buff_num;
	uint32_t i, j, k;
	const struct soundtrigger_pcm_info *pcm_info = NULL;

	if (dma_drv_info == NULL)
		return;

	AUDIO_LOGI("dma config soundtrigger addr: %pK", (void *)CODEC_DSP_SOUNDTRIGGER_BASE_ADDR);

	pcm_num = ARRAY_SIZE(dma_drv_info->pcm_info);
	for (i = 0; i < pcm_num; i++) {
		pcm_info = &(dma_drv_info->pcm_info[i]);
		dma_port_num = ARRAY_SIZE(pcm_info->channel);
		buff_num = ARRAY_SIZE(pcm_info->dma_cfg[0]);
		for (j = 0; j < dma_port_num; j++) {
			for (k = 0; k < buff_num; k++) {
				AUDIO_LOGI("dma soundtrigger info: dma num: %u, buffer num: %u", j, k);

				AUDIO_LOGI("buffer physical addr: %pK, buffer: %pK, lli dma physical addr: %pK, dma cfg: %pK",
					pcm_info->buffer_phy_addr[j][k],
					pcm_info->buffer[j][k],
					pcm_info->lli_dma_phy_addr[j][k],
					pcm_info->dma_cfg[j][k]);

				AUDIO_LOGI("a count: %u, src addr: 0x%pK, dest addr: 0x%pK, config: %u",
					pcm_info->dma_cfg[j][k]->a_count,
					(const void *)(uintptr_t)(pcm_info->dma_cfg[j][k]->src_addr),
					(const void *)(uintptr_t)(pcm_info->dma_cfg[j][k]->des_addr),
					pcm_info->dma_cfg[j][k]->config);
			}
		}
	}
}

