/*
 * platform_io.h
 *
 * platform io config for pcm codec
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

#ifndef __PLATFORM_IO_H__
#define __PLATFORM_IO_H__

#include <linux/mutex.h>
#include <linux/types.h>

#include "platform_base_addr_info.h"

#define PCM_PLAY_BUFF_SIZE     0x100000

#ifndef CONFIG_AUDIO_COMMON_IMAGE
#define HIFI_UNSEC_BASE_ADDR   (RESERVED_HIFI_DATA_PHYMEM_BASE)
#else
#define HIFI_UNSEC_BASE_ADDR   0
#endif

#define ASP_FAMA_PHY_ADDR_DIFF 0

#define PCM_PORTS_MAX 5
#define DMA_CHANNEL_MAX 16
#define PLAYBACK_PORTS_NUM_MAX 4
#define RECORD_PORTS_NUM_MAX 8
#define PCM_STREAM_MAX 2
#define MAX_CHANNEL_NUM 8
#define MAX_PERIODS_COUNT 4
#define EC_CHANNEL_CNT 2
#define RECORD_EC_CHANNEL_CNT 6
#define RECORD_CHANNEL_CNT_MAX 4
#define PLAYBACK_CHANNEL_CNT_MAX 6
#define SLIMBUS_DMA_BYTES 4
#define INTERLACED_DMA_BYTES 8

enum pcm_device {
	PCM_DEVICE_NORMAL = 0,
	PCM_DEVICE_BT,
	PCM_DEVICE_FM,
	PCM_DEVICE_TOTAL
};

enum pipe_type {
	PIPE_AUDIO_UL,
	PIPE_AUDIO_DL,
	PIPE_AUDIO_UL_EC,
	PIPE_BT_UL,
	PIPE_BT_DL,
	PIPE_FM_UL,
	PIPE_CNT,
};

enum pcm_swap_type {
	PCM_SWAP_BUFF_A = 0,
	PCM_SWAP_BUFF_B,
	PCM_SWAP_BUFF_CNT,
};

enum io_type {
	IO_I2S,
	IO_FIFO,
	IO_FIFO_EXCODEC,
	IO_FIFO_INTERLACED,
	IO_USB,
	IO_SLIMBUS,
	IO_SLIMBUS_CONCURRENCY_HEADSET,
	IO_CNT,
};

enum port_config_type {
	PORT_CODECLESS,
	PORT_CODEC,
	PORT_CNT,
};
struct pcm_ports {
	enum pipe_type pipe_type_cfg; /* device ports table index */
	unsigned int ports_cnt; /* ports number */
};

struct dma_config {
	unsigned int port; /* port addr */
	unsigned int config; /* dma config */
	unsigned int channel; /* dma channel */
};

struct pipe {
	unsigned int ports_cnt;
	const struct dma_config *ports[PCM_PORTS_MAX];
};

struct pcm_dma_buf_config {
	u64 phy_addr;
	u64 virtual_addr;
	u64 len;
};

bool is_device_valid(int device);
bool is_stream_valid(int stream);

struct pcm_dma_buf_config *get_dma_buf_cfg(int device, int stream);
struct pcm_dma_buf_config *get_lli_buf_cfg(int device, int stream);

const struct pipe *get_pipe(enum pipe_type pipe_type_cfg, enum io_type io_type_cfg);
int platform_io_init(enum port_config_type port_cfg);
unsigned int get_dma_port_count(unsigned int channels,
	enum io_type io_type_cfg, enum pipe_type pipe_type_cfg);
enum pipe_type get_pipe_type(int device, int stream, enum io_type io_type_cfg);

#ifdef CONFIG_PLATFORM_SLIMBUS
void clear_fifo(int device, int stream, enum io_type io_type_cfg);
#endif

#endif

