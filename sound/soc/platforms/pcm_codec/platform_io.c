/*
 * platform_io.c
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

#include "platform_io.h"

#include <sound/pcm.h>
#include <linux/of.h>

#include "audio_log.h"
#ifdef CONFIG_PLATFORM_SLIMBUS
#include "slimbus.h"
#endif

#ifdef CONFIG_AUDIO_DEBUG
#define LOG_TAG "pcm_codec"
#else
#define LOG_TAG "audio_pcm"
#endif
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#ifndef CONFIG_AUDIO_COMMON_IMAGE
#ifndef SOC_ACPU_ASP_CODEC_BASE_ADDR
#define SOC_ACPU_ASP_CODEC_BASE_ADDR 0xFA552000
#endif
#else
#define SOC_ACPU_ASP_CODEC_BASE_ADDR 0
#endif

#define DSP_MUSIC_DATA_SIZE        (0x132000 + 0x32000)

#define DSP_MUSIC_DATA_LOCATION    (HIFI_UNSEC_BASE_ADDR)
#define PCM_DATA_LOCATION          (DSP_MUSIC_DATA_LOCATION + DSP_MUSIC_DATA_SIZE)

#ifndef CONFIG_AUDIO_COMMON_IMAGE
#define SLIMBUS_BASE_ADDR          (SOC_ACPU_SLIMBUS_BASE_ADDR)
#else
#define SLIMBUS_BASE_ADDR          0
#endif

/* 48k 20ms 8bytes 2buf(AB) */
#define DMA_BUF_SIZE (48 * 20 * 8 * 2 * PCM_PORTS_MAX)
#define DMA_LLI_SIZE 1024

#define DMA_BUF_BASE_ADDR (PCM_DATA_LOCATION)
#define DMA_LLI_BASE_ADDR (DMA_BUF_BASE_ADDR + (DMA_BUF_SIZE * PCM_DEVICE_TOTAL * PCM_STREAM_MAX))

struct io_bus {
	struct pipe pipes[PIPE_CNT];
};

static struct io_bus g_io_bus[IO_CNT];

static struct pcm_dma_buf_config  platform_dma_buf_config[PCM_DEVICE_TOTAL][PCM_STREAM_MAX] = {{{0}}};

static struct pcm_dma_buf_config  platform_dma_lli_config[PCM_DEVICE_TOTAL][PCM_STREAM_MAX] = {{{0}}};

static unsigned int slimbus_port_config[PIPE_CNT][PCM_PORTS_MAX] = {
	{2, 3, 12, 13, DMA_CHANNEL_MAX},  /* PIPE_AUDIO_UL_PORT_NUM */
	{0, 1, DMA_CHANNEL_MAX},          /* PIPE_AUDIO_DL_PORT_NUM */
	{2, 3, 12, 13, 14},               /* PIPE_AUDIO_UL_EC_PORT_NUM */
	{10, 11, DMA_CHANNEL_MAX},        /* PIPE_BT_UL */
	{8, 9, DMA_CHANNEL_MAX},          /* PIPE_BT_DL */
	{DMA_CHANNEL_MAX},
};

static struct dma_config slimbus_dma_config[DMA_CHANNEL_MAX] = {
	{SLIMBUS_BASE_ADDR + 0x1000, 0x83322007, 5},  /* dport0, d1 */
	{SLIMBUS_BASE_ADDR + 0x1040, 0x83322017, 6},  /* dport1, d2 */
	{SLIMBUS_BASE_ADDR + 0x1080, 0x43322027, 7},  /* dport2, u1 */
	{SLIMBUS_BASE_ADDR + 0x10c0, 0x43322037, 8},  /* dport3, u2 */
	{SLIMBUS_BASE_ADDR + 0x1100, 0x83322047, 12}, /* dport4, d3 */
	{SLIMBUS_BASE_ADDR + 0x1140, 0x83322057, 15}, /* dport5, d4 */
	{SLIMBUS_BASE_ADDR + 0x1180, 0x43322067, 14}, /* dport6, u9 */
	{SLIMBUS_BASE_ADDR + 0x11c0, 0x43322077, 15}, /* dport7, u10 */
	{SLIMBUS_BASE_ADDR + 0x1200, 0x83322087, 1},  /* dport8, d5 */
	{SLIMBUS_BASE_ADDR + 0x1240, 0x83322097, 2},  /* dport9, d6 */
	{SLIMBUS_BASE_ADDR + 0x1280, 0x433220a7, 3},  /* dport10, u5 */
	{SLIMBUS_BASE_ADDR + 0x12c0, 0x433220b7, 4},  /* dport11, u6 */
	{SLIMBUS_BASE_ADDR + 0x1300, 0x433220c7, 9},  /* dport12, u3 */
	{SLIMBUS_BASE_ADDR + 0x1340, 0x433220d7, 11}, /* dport13, u4 */
	{SLIMBUS_BASE_ADDR + 0x1380, 0x433220e7, 0},  /* dport14, u7, voice pa */
	{SLIMBUS_BASE_ADDR + 0x13c0, 0x433220f7, 13}, /* dport15, u8, audio pa */
};

static struct dma_config codecless_dma_config[DMA_CHANNEL_MAX] = {
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x200, 0x43322007, 12},  /* port0, smartpa12 */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x204, 0x43322017, 15},  /* port1, smartpa34 */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x208, 0x83322027, 6},   /* port2, codec3 dn */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x20c, 0x83322037, 5},   /* port3, audio dn */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x210, 0x83322047, 8},   /* port4, utlr dn */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x214, 0x83322057, 0},   /* port5, infr dn */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x218, 0x43322067, 7},   /* port6, audio up */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x21c, 0x43322077, 3},   /* port7, voice up */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x220, 0x43322087, 1},   /* port8, mic 34 up */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x224, 0x43322097, 9},   /* port9, mdm up */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x228, 0x833220a7, 2},   /* port10, mdm dn */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x22c, 0x433220b7, 4},   /* port11, infr up l */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x230, 0x433220c7, 10},  /* port12, infr up r */
};

static unsigned int codecless_port_config[PIPE_CNT][PCM_PORTS_MAX] = {
	{9, 0, 1, DMA_CHANNEL_MAX},     /* PIPE_AUDIO_UL_PORT_NUM */
	{2, 3, 10, DMA_CHANNEL_MAX},    /* PIPE_AUDIO_DL_PORT_NUM */
	{9, 0, 1, DMA_CHANNEL_MAX},     /* PIPE_AUDIO_UL_EC_PORT_NUM */
	{7, DMA_CHANNEL_MAX},           /* PCM_PORTS_EXMODEM_UL bt ul */
	{4, DMA_CHANNEL_MAX},           /* PCM_PORTS_EXMODEM_DL bt dl */
	{8, DMA_CHANNEL_MAX},           /* PCM_PORTS_FM_UL fm ul */
};

static struct dma_config codec_dma_config[DMA_CHANNEL_MAX] = {
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x200, 0x43322007, 12},  /* port0, smartpa12 */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x204, 0x43322017, 15},  /* port1, smartpa34 */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x208, 0x83322027, 6},   /* port2, codec3 dn */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x20c, 0x83322037, 5},   /* port3, audio dn */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x210, 0x83322047, 8},   /* port4, utlr dn */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x214, 0x83322057, 0},   /* port5, infr dn */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x218, 0x43322067, 7},   /* port6, audio up */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x21c, 0x43322077, 3},   /* port7, voice up */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x220, 0x43322087, 1},   /* port8, mic 34 up */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x224, 0x43322097, 9},   /* port9, mdm up */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x228, 0x833220a7, 2},   /* port10, mdm dn */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x22c, 0x433220b7, 4},   /* port11, infr up l */
	{SOC_ACPU_ASP_CODEC_BASE_ADDR + 0x230, 0x433220c7, 10},  /* port12, infr up r */
};

static unsigned int codec_port_config[PIPE_CNT][PCM_PORTS_MAX] = {
	{9, 8, DMA_CHANNEL_MAX},     /* PIPE_AUDIO_UL_PORT_NUM */
	{2, 3, 10, DMA_CHANNEL_MAX}, /* PIPE_AUDIO_DL_PORT_NUM */
	{9, 8, DMA_CHANNEL_MAX},     /* PIPE_AUDIO_UL_EC_PORT_NUM */
	{7, DMA_CHANNEL_MAX},        /* PCM_PORTS_EXMODEM_UL bt ul */
	{4, DMA_CHANNEL_MAX},        /* PCM_PORTS_EXMODEM_DL bt dl */
	{0, 1, DMA_CHANNEL_MAX},     /* PCM_PORTS_FM_UL fm ul */
};

bool is_device_valid(int device)
{
	if (device < 0 || device >= PCM_DEVICE_TOTAL)
		return false;
	return true;
}

bool is_stream_valid(int stream)
{
	if (stream < 0 || stream >= PCM_STREAM_MAX)
		return false;
	return true;
}

const struct pipe *get_pipe(enum pipe_type pipe_type_cfg, enum io_type io_type_cfg)
{
	if (io_type_cfg >= IO_CNT) {
		AUDIO_LOGE("io type invalid, type: %d", io_type_cfg);
		return NULL;
	}

	if (pipe_type_cfg >= PIPE_CNT) {
		AUDIO_LOGE("pipe type invalid, type: %d", pipe_type_cfg);
		return NULL;
	}

	return &g_io_bus[io_type_cfg].pipes[pipe_type_cfg];
}

#ifdef CONFIG_AUDIO_COMMON_IMAGE
static void config_dma_port(void)
{
	int i;
	uint32_t slimbus_base_addr = get_platform_mem_base_addr(PLT_SLIMBUS_MEM);
	uint32_t asp_codec_base_addr = get_platform_mem_base_addr(PLT_ASP_CODEC_MEM);

	for (i = 0; i < DMA_CHANNEL_MAX; i++) {
		slimbus_dma_config[i].port += slimbus_base_addr;
		codecless_dma_config[i].port += asp_codec_base_addr;
	}
}
#endif

static void set_platform_io(enum io_type io_type_cfg,
	struct dma_config *config,
	unsigned int (*port)[PCM_PORTS_MAX])
{
	unsigned int i, j;

	for (i = 0; i < PIPE_CNT; i++) {
		if (port[i][0] == DMA_CHANNEL_MAX)
			break;

		for (j = 0; j < PCM_PORTS_MAX; j++) {
			if (port[i][j] == DMA_CHANNEL_MAX)
				break;

			g_io_bus[io_type_cfg].pipes[i].ports[j] =
				&config[port[i][j]];
		}
		g_io_bus[io_type_cfg].pipes[i].ports_cnt = j;
	}
}

struct pcm_dma_buf_config *get_dma_buf_cfg(int device, int stream)
{
	if (!is_device_valid(device) || !is_stream_valid(stream))
		return NULL;

	return &platform_dma_buf_config[device][stream];
}

struct pcm_dma_buf_config *get_lli_buf_cfg(int device, int stream)
{
	if (!is_device_valid(device) || !is_stream_valid(stream))
		return NULL;

	return &platform_dma_lli_config[device][stream];
}

static int buffer_init(void)
{
	int i, j;
#ifdef CONFIG_AUDIO_COMMON_IMAGE
	uint32_t phy_base_addr = get_platform_mem_base_addr(PLT_HIFI_MEM);
#endif

	for (i = 0; i < PCM_DEVICE_TOTAL; i++) {
		for (j = 0; j < PCM_STREAM_MAX; j++) {
#ifndef CONFIG_AUDIO_COMMON_IMAGE
			platform_dma_buf_config[i][j].phy_addr = DMA_BUF_BASE_ADDR +
				((i * PCM_STREAM_MAX) + j) * DMA_BUF_SIZE;
			platform_dma_lli_config[i][j].phy_addr = DMA_LLI_BASE_ADDR +
				((i * PCM_STREAM_MAX) + j) * DMA_LLI_SIZE;
#else
			platform_dma_buf_config[i][j].phy_addr = DMA_BUF_BASE_ADDR +
				phy_base_addr + ((i * PCM_STREAM_MAX) + j) * DMA_BUF_SIZE;
			platform_dma_lli_config[i][j].phy_addr = DMA_LLI_BASE_ADDR +
				phy_base_addr + ((i * PCM_STREAM_MAX) + j) * DMA_LLI_SIZE;
#endif
			platform_dma_buf_config[i][j].len = DMA_BUF_SIZE;
			platform_dma_lli_config[i][j].len = DMA_LLI_SIZE;
		}
	}

	if ((PCM_DEVICE_TOTAL * PCM_STREAM_MAX * (DMA_BUF_SIZE + DMA_LLI_SIZE)) >
		PCM_PLAY_BUFF_SIZE) {
		AUDIO_LOGE("buf address is out of range");
		return -1;
	}

	return 0;
}

int platform_io_init(enum port_config_type port_cfg)
{
	unsigned int io_type;

#ifdef CONFIG_AUDIO_COMMON_IMAGE
	config_dma_port();
#endif
	for (io_type = 0; io_type < IO_CNT; io_type++) {
		switch (io_type) {
		case IO_SLIMBUS:
			set_platform_io(io_type, slimbus_dma_config,
				slimbus_port_config);
			break;
		case IO_FIFO_INTERLACED:
			if (port_cfg == PORT_CODEC) {
				AUDIO_LOGI("port config type:%d", port_cfg);
				set_platform_io(io_type, codec_dma_config,
					codec_port_config);
			} else if (port_cfg == PORT_CODECLESS) {
				set_platform_io(io_type, codecless_dma_config,
					codecless_port_config);
			}
			break;
		default:
			break;
		}
	}

	return buffer_init();
}

unsigned int get_dma_port_count(unsigned int channels,
	enum io_type io_type_cfg, enum pipe_type pipe_type_cfg)
{
	unsigned int ports_cnt;
	const struct pipe *pipe = get_pipe(pipe_type_cfg, io_type_cfg);
	if (pipe == NULL) {
		AUDIO_LOGE("get pipe failed");
		return 0;
	}

	switch (io_type_cfg) {
	case IO_FIFO_INTERLACED:
		/* interlace use 1dma transfer 2 channel data */
		ports_cnt = channels / 2;
		break;
	case IO_SLIMBUS:
	default:
		ports_cnt = channels;
		break;
	}

	return MIN(pipe->ports_cnt, ports_cnt);
}

static enum pipe_type _slimbus_get_out_pipe_type(int device)
{
	switch (device) {
	case PCM_DEVICE_NORMAL:
		return PIPE_AUDIO_DL;
	case PCM_DEVICE_BT:
		return PIPE_BT_DL;
	default:
		return PIPE_CNT;
	}
}

static enum pipe_type _slimbus_get_in_pipe_type(int device)
{
	switch (device) {
	case PCM_DEVICE_NORMAL:
		return PIPE_AUDIO_UL_EC;
	case PCM_DEVICE_BT:
		return PIPE_BT_UL;
	default:
		return PIPE_CNT;
	}
}

static enum pipe_type _slimbus_get_pipe_type(int device, int stream)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		return _slimbus_get_out_pipe_type(device);
	else if (stream == SNDRV_PCM_STREAM_CAPTURE)
		return _slimbus_get_in_pipe_type(device);
	else
		return PIPE_CNT;
}

static enum pipe_type _interlace_get_out_pipe_type(int device)
{
	switch (device) {
	case PCM_DEVICE_NORMAL:
		return PIPE_AUDIO_DL;
	case PCM_DEVICE_BT:
		return PIPE_BT_DL;
	case PCM_DEVICE_FM:
	default:
		return PIPE_CNT;
	}
}

static enum pipe_type _interlace_get_in_pipe_type(int device)
{
	switch (device) {
	case PCM_DEVICE_NORMAL:
		return PIPE_AUDIO_UL;
	case PCM_DEVICE_BT:
		return PIPE_BT_UL;
	case PCM_DEVICE_FM:
		return PIPE_FM_UL;
	default:
		return PIPE_CNT;
	}
}

static enum pipe_type _interlace_get_pipe_type(int device, int stream)
{
	if (stream == SNDRV_PCM_STREAM_PLAYBACK)
		return _interlace_get_out_pipe_type(device);
	else if (stream == SNDRV_PCM_STREAM_CAPTURE)
		return _interlace_get_in_pipe_type(device);
	else
		return PIPE_CNT;
}

enum pipe_type get_pipe_type(int device, int stream, enum io_type io_type_cfg)
{
	if (!is_device_valid(device) || !is_stream_valid(stream) ||
		io_type_cfg >= IO_CNT)
		return PIPE_CNT;

	switch (io_type_cfg) {
	case IO_SLIMBUS:
		return _slimbus_get_pipe_type(device, stream);
	case IO_FIFO_INTERLACED:
		return _interlace_get_pipe_type(device, stream);
	default:
		return PIPE_CNT;
	}
}

#ifdef CONFIG_PLATFORM_SLIMBUS
static void slimbus_clear_fifo(int device, int stream, enum io_type io_type_cfg)
{
	enum pipe_type pipe;
	int i;

	pipe = get_pipe_type(device, stream, io_type_cfg);
	if (pipe == PIPE_CNT)
		return;

	for (i = 0; i < PCM_PORTS_MAX; i++) {
		if (slimbus_port_config[pipe][i] == DMA_CHANNEL_MAX)
			break;

		slimbus_clear_port_fifo(slimbus_port_config[pipe][i]);
	}
}
#endif

void clear_fifo(int device, int stream, enum io_type io_type_cfg)
{
#ifdef CONFIG_PLATFORM_SLIMBUS
	if (io_type_cfg == IO_SLIMBUS)
		slimbus_clear_fifo(device, stream, io_type_cfg);
#endif
}