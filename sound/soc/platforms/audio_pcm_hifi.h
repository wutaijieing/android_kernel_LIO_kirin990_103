/*
 * audio_pcm_hifi.h
 *
 * ALSA SoC PCM HIFI driver
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2013-2021. All rights reserved.
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

#ifndef __AUIDO_PCM_HIFI_H__
#define __AUIDO_PCM_HIFI_H__

#include <linux/spinlock.h>

#include "dsp_misc.h"
#include "audio_common_msg_id.h"

#define HI_CHN_COMMON   \
	unsigned short  msg_type;   \
	unsigned short  pcm_device; \
	unsigned short  pcm_mode;   \
	unsigned short  reserved;


#define INT_TO_ADDR(low, high) ((void *) (uintptr_t)((unsigned long long)(low) | ((unsigned long long)(high) << 32)))
#define GET_LOW32(x) (unsigned int)(((unsigned long long)(unsigned long)(x)) & 0xffffffffULL)
#define GET_HIG32(x) (unsigned int)((((unsigned long long)(unsigned long)(x)) >> 32) & 0xffffffffULL)

/* custom hwdep iface for pcm, add to asound.h later */
#define SND_HWDEP_IFACE_PCM (0x30)

/* pcm ioctl cmds */
#define AUIDO_PCM_IOCTL_MMAP_SHARED_FD    _IOWR('H', 0x01, struct audio_pcm_mmap_fd)

enum audio_pcm_status {
	STATUS_STOP = 0,
	STATUS_RUNNING = 1,
	STATUS_STOPPING = 2
};

enum audio_pcm_formats {
	PCM_FORMAT_S16_LE = 0,
	PCM_FORMAT_S24_LE_LA,       /* left alignment */
	PCM_FORMAT_S24_LE_RA,       /* right alignment */
	PCM_FORMAT_S32_LE,
	PCM_FORMAT_S24_3LE,
	PCM_FORMAT_MAX,
};

enum hifi_chn_msg_type {
	HI_CHN_MSG_PCM_OPEN             = ID_AP_AUDIO_PCM_OPEN_REQ,
	HI_CHN_MSG_PCM_CLOSE            = ID_AP_AUDIO_PCM_CLOSE_REQ,
	HI_CHN_MSG_PCM_HW_PARAMS        = ID_AP_AUDIO_PCM_HW_PARA_REQ,
	HI_CHN_MSG_PCM_HW_FREE          = ID_AP_AUDIO_PCM_HW_FREE_REQ,
	HI_CHN_MSG_PCM_PREPARE          = ID_AP_AUDIO_PCM_PREPARE_REQ,
	HI_CHN_MSG_PCM_TRIGGER          = ID_AP_AUDIO_PCM_TRIGGER_REQ,
	HI_CHN_MSG_PCM_POINTER          = ID_AP_AUDIO_PCM_POINTER_REQ,
	/* transport data */
	HI_CHN_MSG_PCM_SET_BUF          = ID_AP_AUDIO_PCM_SET_BUF_CMD,
	/* HFI data transfer completed */
	HI_CHN_MSG_PCM_PERIOD_ELAPSED   = ID_AUDIO_AP_PCM_PERIOD_ELAPSED_CMD,
	HI_CHN_MSG_PCM_PERIOD_STOP      = ID_AUDIO_AP_PCM_TRIGGER_CNF,
	HI_CHN_MSG_PCM_XRUN             = ID_AUDIO_AP_PCM_XRUN,
};

enum irq_rt {
	/* IRQ Not Handled as Other problem */
	IRQ_NH_OTHERS   = -5,
	/* IRQ Not Handled as Mailbox problem */
	IRQ_NH_MB       = -4,
	/* IRQ Not Handled as pcm MODE problem */
	IRQ_NH_MODE     = -3,
	/* IRQ Not Handled as TYPE problem */
	IRQ_NH_TYPE     = -2,
	/* IRQ Not Handled */
	IRQ_NH          = -1,
	/* IRQ HanDleD */
	IRQ_HDD         = 0,
	/* IRQ HanDleD related to PoinTeR */
	IRQ_HDD_PTR     = 1,
	/* IRQ HanDleD related to STATUS */
	IRQ_HDD_STATUS,
	/* IRQ HanDleD related to SIZE */
	IRQ_HDD_SIZE,
	/* IRQ HanDleD related to PoinTeR of Substream */
	IRQ_HDD_PTRS,
	/* IRQ HanDleD Error */
	IRQ_HDD_ERROR,
};

#define PCM_DMA_BUF_MMAP_MAX_SIZE (48 * 5 * 4 * 2 * 16)
#define PCM_DMA_BUF_PLAYBACK_LEN    (0x00030000)
#define PCM_DMA_BUF_0_PLAYBACK_BASE (PCM_PLAY_BUFF_LOCATION)

struct audio_pcm_mmap_buf {
	void *buf_addr;
	uint32_t buf_size;
	dma_addr_t phy_addr;
	struct dma_buf *dmabuf;
};

struct pcm_ap_hifi_buf {
	unsigned char *area; /* virtual pointer */
	dma_addr_t addr;     /* physical address */
	size_t bytes;        /* buffer size in bytes */
};

struct audio_pcm_mmap_fd {
	int32_t shared_fd;
	int32_t stream_direction; /* 0-play; 1-capture */
	uint32_t buf_size;
};

struct audio_pcm_mailbox_data {
	struct list_head node;
	unsigned short msg_type; /* msg type: hifi_chn_msg_type */
	unsigned short pcm_mode; /* PLAYBACK or CAPTURE */
	void *substream;         /* address of the channel substream object */
};

/* runtime data, which is used for PLAYBACK or CAPTURE */
struct audio_pcm_runtime_data {
	spinlock_t lock;                     /* protect audio_pcm_runtime_data */
	unsigned int period_next;            /* record which period to fix dma next time */
	unsigned int period_cur;             /* record which period using now */
	unsigned int period_size;            /* DMA SIZE */
	enum audio_pcm_status  status;       /* pcm status running or stop */
	struct snd_pcm_substream *substream;
	long snd_pre_time;
	bool using_thread_flag;              /* indicate stream using thread */
	struct audio_pcm_mmap_buf *mmap_buf; /* share memory info, only used in mmap */
	struct pcm_ap_hifi_buf hifi_buf;     /* record ap-hifi share buffer info, only used in mmap */
	unsigned long frame_counter;         /* record frames processed count, only used in mmap */
	bool is_support_low_power;
};

struct hifi_pcm_config {
	unsigned int channels;
	unsigned int rate;
	unsigned int period_size;
	unsigned int period_count;
	unsigned int format;
};

struct hifi_chn_pcm_open {
	HI_CHN_COMMON
	unsigned short is_support_low_power;
	unsigned short reserved2;
	struct hifi_pcm_config config;
};

struct hifi_chn_pcm_close {
	HI_CHN_COMMON
};

struct hifi_chn_pcm_hw_params {
	HI_CHN_COMMON
	unsigned int    channel_num;
	unsigned int    sample_rate;
	unsigned int    format;
};

struct hifi_chn_pcm_trigger {
	HI_CHN_COMMON
	unsigned short tg_cmd;        /* trigger type, for example SNDRV_PCM_TRIGGER_START */
	unsigned short reserved1;     /* reserved for aligned */
	unsigned int   substream_l32; /* address of the substream object of the channel */
	unsigned int   substream_h32; /* address of the substream object of the channel */
	unsigned int   data_addr;     /* data pointer for DMA data transfer, physical address */
	unsigned int   data_len;      /* byte */
};

struct hifi_channel_set_buffer {
	HI_CHN_COMMON
	/*
	 * data pointer for DMA data transfer, physical address
	 * used as the DMA data transfer source during PLAYBACK
	 * used as the DMA data transfer destination in the case of CAPTURE
	 */
	unsigned int data_addr;
	unsigned int data_len; /* byte */
};

struct hifi_chn_pcm_period_elapsed {
	HI_CHN_COMMON
	unsigned int substream_l32; /* address of the substream object of the channel */
	unsigned int substream_h32; /* address of the substream object of the channel */
	unsigned int msg_timestamp;
	unsigned short frame_num;
	unsigned short discard;
};

#define PCM_STREAM_MAX              2
#define PCM_DEVICE_MAX              10

#define UPDATE_FRAME_NUM_MAX 10
#define RESPOND_MAX_TIME 10
#define PROCESS_MAX_TIME 5

struct pcm_dma_buf_config {
	u64 pcm_dma_buf_base;
	u64 pcm_dma_buf_len;
};

struct cust_priv {
	bool is_cdc_device;
	struct snd_soc_dai_driver *pcm_dai;
	unsigned int  dai_num;
	const struct snd_pcm_hardware* (*get_pcm_hw_config)(int device, int stream);
	const struct pcm_dma_buf_config* (*get_pcm_buf_config)(int device, int stream);
	bool (*is_valid_pcm_device)(int device);
	unsigned int  end_addr;
	unsigned int  dev_normal;
	unsigned int  dev_lowlatency;
	unsigned int  dev_mmap;
	unsigned int  dev_num;
	unsigned int  active;
};

void codec_register_cust_interface(struct cust_priv *cust);

#endif /* __AUIDO_PCM_HIFI_H__ */
