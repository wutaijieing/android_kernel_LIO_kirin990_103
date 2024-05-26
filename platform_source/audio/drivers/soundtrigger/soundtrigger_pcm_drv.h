/*
 * soundtrigger_pcm_drv.h
 *
 * soundtrigger_pcm_drv is a kernel driver which is used for soundtrigger
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

#ifndef __SOUNDTRIGGER_PCM_DRV_H__
#define __SOUNDTRIGGER_PCM_DRV_H__

#include <linux/types.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/pm_wakeup.h>
#include <linux/hwspinlock.h>

#include "platform_base_addr_info.h"

/* slimbus register addr */
#ifndef CONFIG_AUDIO_COMMON_IMAGE
#define SLIMBUS_BASE_REG (SOC_ACPU_SLIMBUS_BASE_ADDR)
#else
#define SLIMBUS_BASE_REG 0
#endif
#define SLIMBUS_REG_SIZE 100
#define SLIMBUS_PORT0_REG_0 0x100
#define SLIMBUS_PORT0_REG_1 0x104
#define SLIMBUS_PORT1_REG_0 0x108
#define SLIMBUS_PORT1_REG_1 0x10C
#define SLIMBUS_PORT2_REG_0 0x110
#define SLIMBUS_PORT2_REG_1 0x114
#define SLIMBUS_PORT3_REG_0 0x118
#define SLIMBUS_PORT3_REG_1 0x11C
#define SLIMBUS_PORT4_REG_0 0x120
#define SLIMBUS_PORT4_REG_1 0x124
#define SLIMBUS_PORT5_REG_0 0x128
#define SLIMBUS_PORT5_REG_1 0x12C
#define SLIMBUS_PORT6_REG_0 0x130
#define SLIMBUS_PORT6_REG_1 0x134
#define SLIMBUS_PORT7_REG_0 0x138
#define SLIMBUS_PORT7_REG_1 0x13C
#define SLIMBUS_PORT8_REG_0 0x140
#define SLIMBUS_PORT8_REG_1 0x144
#define SLIMBUS_PORT9_REG_0 0x148
#define SLIMBUS_PORT9_REG_1 0x14C
#define SLIMBUS_PORT10_REG_0 0x150
#define SLIMBUS_PORT10_REG_1 0x154
#define SLIMBUS_PORT11_REG_0 0x158
#define SLIMBUS_PORT11_REG_1 0x15C

#define DA_COMBINE_V5_FAST_L_SLMIMBUS_PORT_OFFSET 0x1180
#define DA_COMBINE_V5_FAST_L_SLMIMBUS_PORT_ADDR      \
	(SLIMBUS_BASE_REG + DA_COMBINE_V5_FAST_L_SLMIMBUS_PORT_OFFSET)
#define DA_COMBINE_V5_FAST_L_DMA_CONF_NUM 0x43322067
#define DA_COMBINE_V5_NORMAL_L_SLMIMBUS_PORT_OFFSET 0x1080
#define DA_COMBINE_V5_NORMAL_L_SLMIMBUS_PORT_ADDR    \
	(SLIMBUS_BASE_REG + DA_COMBINE_V5_NORMAL_L_SLMIMBUS_PORT_OFFSET)
#define DA_COMBINE_V5_NORMAL_L_DMA_CONF_NUM 0x43322027
#define DA_COMBINE_V5_NORMAL_R_SLMIMBUS_PORT_OFFSET 0x10c0
#define DA_COMBINE_V5_NORMAL_R_SLMIMBUS_PORT_ADDR    \
	(SLIMBUS_BASE_REG + DA_COMBINE_V5_NORMAL_R_SLMIMBUS_PORT_OFFSET)
#define DA_COMBINE_V5_NORMAL_R_DMA_CONF_NUM 0x433220b7

#define DA_COMBINE_V3_FAST_L_SLMIMBUS_PORT_OFFSET 0x1280
#define DA_COMBINE_V3_FAST_L_SLMIMBUS_PORT_ADDR      \
	(SLIMBUS_BASE_REG + DA_COMBINE_V3_FAST_L_SLMIMBUS_PORT_OFFSET)
#define DA_COMBINE_V3_FAST_L_DMA_CONF_NUM 0x433220a7
#define DA_COMBINE_V3FAST_R_SLMIMBUS_PORT_OFFSET 0x12c0
#define DA_COMBINE_V3_FAST_R_SLMIMBUS_PORT_ADDR      \
	(SLIMBUS_BASE_REG + DA_COMBINE_V3FAST_R_SLMIMBUS_PORT_OFFSET)
#define DA_COMBINE_V3_FAST_R_DMA_CONF_NUM 0x43322077
#define DA_COMBINE_V3_NORMAL_L_SLMIMBUS_PORT_OFFSET 0x1080
#define DA_COMBINE_V3_NORMAL_L_SLMIMBUS_PORT_ADDR    \
	(SLIMBUS_BASE_REG + DA_COMBINE_V3_NORMAL_L_SLMIMBUS_PORT_OFFSET)
#define DA_COMBINE_V3_NORMAL_L_DMA_CONF_NUM 0x43322027
#define DA_COMBINE_V3_NORMAL_R_SLMIMBUS_PORT_OFFSET 0x10c0
#define DA_COMBINE_V3_NORMAL_R_SLMIMBUS_PORT_ADDR    \
	(SLIMBUS_BASE_REG + DA_COMBINE_V3_NORMAL_R_SLMIMBUS_PORT_OFFSET)
#define DA_COMBINE_V3_NORMAL_R_DMA_CONF_NUM 0x433220b7

#define DA_COMBINE_V5_FAST_SOUNDWIRE_PORT_ADDR (SOC_ACPU_SoundWire_BASE_ADDR + 0x400)
#define DA_COMBINE_V5_FAST_SOUNDWIRE_L_DMA_CONF_NUM 0x43322047
#define DA_COMBINE_V5_NORMAL_SOUNDWIRE_PORT_ADDR (SOC_ACPU_SoundWire_BASE_ADDR + 0x0)
#define DA_COMBINE_V5_NORMAL_SOUNDWIRE_L_DMA_CONF_NUM 0x43322007

#define THIRD_CODEC_FAST_L_PORT_ADDR 0xe8052128
#define THIRD_CODEC_FAST_R_PORT_ADDR 0xe805212c
#define THIRD_CODEC_FAST_L_DMA_CONF_NUM 0x43322047
#define THIRD_CODEC_FAST_R_DMA_CONF_NUM 0x43322057

#ifdef CONFIG_SND_SOC_THIRD_CODEC
#define DMA_FAST_LEFT_CH_NUM 12
#define DMA_FAST_RIGHT_CH_NUM 15
#define DMA_NORMAL_LEFT_CH_NUM 2
#define DMA_NORMAL_RIGHT_CH_NUM 3
#else
#ifdef CONFIG_SND_SOC_DA_COMBINE_V5
#define DMA_FAST_LEFT_CH_NUM 14
#define DMA_FAST_RIGHT_CH_NUM 0 /* unused channel */
#define DMA_NORMAL_LEFT_CH_NUM 7
#define DMA_NORMAL_RIGHT_CH_NUM 8
#else
#define DMA_FAST_LEFT_CH_NUM 4
#define DMA_FAST_RIGHT_CH_NUM 8 /* unused channel */
#define DMA_NORMAL_LEFT_CH_NUM 3
#define DMA_NORMAL_RIGHT_CH_NUM 7
#endif
#endif

#ifdef CONFIG_SND_SOC_THIRD_CODEC
#define PCM_SWAP_BUFFER_NUM 2
#else
#define PCM_SWAP_BUFFER_NUM 6
#endif
#define HWLOCK_WAIT_TIME 50

#define TIMEOUT_CLOSE_DMA_MS 60000
#define SAMPLE_RATE_44_TO_16 3
#define STEREO_DMA_PORT_NUM 2
#define FRAME_MAGIC_WORD 0x5A5A

#define FAST_CHANNEL_TIMEOUT_READ_COUNT 200
#define NORMAL_CHANNEL_TIMEOUT_READ_COUNT 200
#define NORMAL_CHANNEL_TIMEOUT_TRAN_COUNT 1000

#ifdef CONFIG_SND_SOC_THIRD_CODEC
#define FAST_FRAME_LEN 2816
#define FAST_TRAN_RATE 48000
#else
#define FAST_FRAME_LEN 1920 /* time: 10ms */
#define FAST_TRAN_RATE 192000
#endif

#ifdef STEREO_TYPE
#define DMA_PORT_NUM 2
#else
#define DMA_PORT_NUM 1
#endif

#define DA_COMBINE_V3_NORMAL_FRAME_LEN 480 /* time: 10ms */
#define DA_COMBINE_V3_NORMAL_TRAN_RATE 48000

#define DA_COMBINE_V5_NORMAL_FRAME_LEN 480 /* time: 10ms */
#define DA_COMBINE_V5_NORMAL_TRAN_RATE 48000

#define THIRD_CODEC_NORMAL_FRAME_LEN 480


#define DA_COMBINE_V3_NORMAL_FRAME_SIZE  \
		(DA_COMBINE_V3_NORMAL_FRAME_LEN * VALID_BYTE_COUNT_EACH_SAMPLE_POINT)
#define DA_COMBINE_V5_NORMAL_FRAME_SIZE  \
		(DA_COMBINE_V5_NORMAL_FRAME_LEN * VALID_BYTE_COUNT_EACH_SAMPLE_POINT)
#define THIRD_CODEC_NORMAL_FRAME_SIZE  \
		(THIRD_CODEC_NORMAL_FRAME_LEN * VALID_BYTE_COUNT_EACH_SAMPLE_POINT)


#define RINGBUF_SIZE 61440

#define FAST_HEAD_FRAME_COUNT_FULL_MAGIC 1
#define FAST_TAIL_FRAME_COUNT_FULL_MAGIC 1
#define NORMAL_HEAD_FRAME_COUNT_FULL_MAGIC 2

#define BYTE_COUNT_EACH_SAMPLE_POINT 4
#define VALID_BYTE_COUNT_EACH_SAMPLE_POINT 2
#define VALID_BYTE_COUNT_FAST 4

#define RINGBUF_FRAME_LEN 160
#define RINGBUF_FRAME_COUNT 200
#define RINGBUF_FRAME_SIZE (RINGBUF_FRAME_LEN * VALID_BYTE_COUNT_EACH_SAMPLE_POINT) /* 160 * 2 = 320 */

#define FAST_FRAME_SIZE (FAST_FRAME_LEN * VALID_BYTE_COUNT_FAST) /* 1920 * 4 = 7680 */
#define FAST_TRAN_COUNT (RINGBUF_SIZE / FAST_FRAME_SIZE + \
	FAST_HEAD_FRAME_COUNT_FULL_MAGIC + FAST_TAIL_FRAME_COUNT_FULL_MAGIC) /* 8 + 2 = 10 */
#define FAST_BUFFER_SIZE (FAST_FRAME_SIZE * FAST_TRAN_COUNT) /* 7680 x 10 = 76800 */

#define drv_dma_lli_link(addr) (((addr) & 0xffffffe0UL) | 0x2UL)

enum codec_dsp_type {
	CODEC_DA_COMBINE_V3 = 0,
	CODEC_DA_COMBINE_V5,
	CODEC_THIRD_CODEC,
	CODEC_DA_COMBINE_MAX
};

enum fast_tran_state {
	FAST_TRAN_NOT_COMPLETE = 0,
	FAST_TRAN_COMPLETE
};

enum read_state {
	READ_NOT_COMPLETE = 0,
	READ_COMPLETE
};

enum find_state {
	FRAME_NOT_FIND = 0,
	FRAME_FIND
};

enum {
	PCM_FAST = 0,
	PCM_FAST_LEFT = 0,
	PCM_FAST_RIGHT = 1,
	PCM_NORMAL = 1,
	PCM_CHL_NUM
};

struct dma_config {
	uint32_t port; /* slimbus port address */
	uint32_t config; /* dma config number */
	uint32_t channel; /* dma channel number */
};

struct soundtrigger_io_sync_param {
	unsigned int para_in_l;
	unsigned int para_in_h;
	unsigned int para_size_in;

	unsigned int para_out_l;
	unsigned int para_out_h;
	unsigned int para_size_out;
};

struct st_fast_status {
	int fm_status;
	int fast_status;
};

struct soundtrigger_pcm_config {
	uint32_t channels; /* stereo: channel = 2, mono: channel = 1 */
	uint32_t rate; /* sampling rate */
	uint32_t frame_len; /* frame length */
	uint32_t byte_count; /* each sampling point contain byte number */
};

struct soundtrigger_pcm_info {
	struct soundtrigger_pcm_config pcm_cfg;
	void *buffer[DMA_PORT_NUM][PCM_SWAP_BUFFER_NUM]; /* swap buffer */
	void *buffer_phy_addr[DMA_PORT_NUM][PCM_SWAP_BUFFER_NUM]; /* swap buffer physical addr */
	uint32_t buffer_size; /* one swap buffer size, each sampling point contains 4 bytes */
	uint32_t channel[DMA_PORT_NUM]; /* dma channel number */
	struct dma_lli_cfg *dma_cfg[DMA_PORT_NUM][PCM_SWAP_BUFFER_NUM];
	void *lli_dma_phy_addr[DMA_PORT_NUM][PCM_SWAP_BUFFER_NUM];
};

struct fast_tran_info {
	uint32_t fast_frame_find_flag; /* flag to decide whether get first frame; before first frame, all input data is unuse */
	uint32_t fast_start_addr; /* address of valid data in the first frame */
	uint16_t fast_head_frame_word; /* word of head frame which is full of unuse data, such as 0x5A5A */
	uint32_t fast_head_frame_size;
	uint32_t fast_head_word_count;
	uint16_t fast_buffer[FAST_BUFFER_SIZE]; /* buffer to store all fast transmit data, including head frame */
	int32_t  dma_tran_count; /* number of valid fast transmission */
	int32_t  dma_tran_total_count; /* total number of fast transmit */
	int32_t  fast_complete_flag; /* fast tansmit complete flag */
	int32_t  fast_read_complete_flag; /* flag to decide whether HAL read cpmlete */
	uint32_t irq_count; /* channel interrupt count */
	uint32_t read_count; /* channel read count */
};

struct normal_tran_info {
	uint32_t normal_frame_find_flag; /* flag to decide whether get first frame; before first frame, all input data is unuse */
	uint32_t normal_start_addr; /* address of valid data in the first frame */
	uint32_t normal_first_frame_read_flag; /* flag to decide whether HAL read first frame */
	uint16_t normal_head_frame_word; /* word of head frame which is full of unuse data, such as 0xA5A5 */
	uint32_t normal_head_frame_size;
	uint32_t normal_head_word_count;
	struct ring_buffer_itef *normal_buffer; /* ringbuffer which stores normal transmit data , including head frame */
	uint32_t normal_tran_count; /* number of valid normal transmission */
	uint32_t irq_count_left; /* left channel interrupt count */
	uint32_t irq_count_right; /* left channel interrupt count */
	uint32_t read_count_left; /* left channel read count */
	uint32_t read_count_right; /* right channel read count */
};

struct soundtrigger_workqueue {
	struct workqueue_struct *dma_irq_handle_wq;
	struct delayed_work dma_fast_left_work;
	struct delayed_work dma_fast_right_work;
	struct delayed_work dma_normal_left_work;
	struct delayed_work dma_normal_right_work;

	struct workqueue_struct *dma_close_wq;
	struct delayed_work close_dma_timeout_work;
};

struct soundtrigger_pcm {
	struct hwspinlock *hwlock;
	spinlock_t lock;
	struct resource *res;
	void __iomem *reg_base_addr;
	struct regulator *asp_ip;
	struct device *dev;
	uint32_t dma_int_fast_left_flag;
	uint32_t dma_int_fast_right_flag;
	uint32_t dma_int_nomal_flag;
	uint32_t dma_alloc_flag;
	bool is_open;
	int32_t fm_status;
	uint32_t is_dma_enable;
	uint32_t is_codec_bus_enable;
	enum codec_dsp_type type;
	struct clk *asp_subsys_clk;
	struct soundtrigger_pcm_info pcm_info[PCM_CHL_NUM];
	struct fast_tran_info fast_tran_info_left;
	struct fast_tran_info fast_tran_info_right;
	struct normal_tran_info normal_tran_info;

	struct soundtrigger_workqueue workqueue;

	struct wakeup_source *wake_lock;
	struct mutex ioctl_mutex;
};

int32_t soundtrigger_set_codec_type(enum codec_dsp_type type);
void soundtrigger_pcm_reset(void);
struct soundtrigger_pcm_ops {
	void (*open)(struct soundtrigger_pcm *dma_drv_info);
	void (*get_dma_cfg)(struct dma_config *cfg, enum codec_dsp_type dsp_type,
			uint32_t pcm_channel, uint32_t offset);
	void (*get_pcm_cfg)(struct soundtrigger_pcm_config *cfg,
			enum codec_dsp_type dsp_type, uint32_t pcm_channel);
	int32_t (*start_dma)(struct soundtrigger_pcm *dma_drv_info);
	int32_t (*check_fast_complete_flag)(const struct soundtrigger_pcm *dma_drv_info);
	void (*set_dma_int_flag)(struct soundtrigger_pcm *dma_drv_info);
	void (*dmac_irq_handle)(struct soundtrigger_pcm *dma_drv_info);
	void (*proc_fast_data)(struct soundtrigger_pcm *dma_drv_info);
	void (*proc_fast_trans_buff)(struct fast_tran_info *fast_info,
			struct soundtrigger_pcm_info *pcm_info,
			struct soundtrigger_pcm *dma_drv_info);
	void (*proc_normal_data)(struct soundtrigger_pcm *dma_drv_info);
	ssize_t (*read_data)(struct fast_tran_info *fast_info_left,
			struct fast_tran_info *fast_info_right,
			char __user *buffer, size_t max_read_len);
	void (*dump_dma_info)(const struct soundtrigger_pcm *dma_drv_info);
	void (*deinit_dma)(struct soundtrigger_pcm *dma_drv_info);
	int32_t (*deinit_dma_info)(struct soundtrigger_pcm *dma_drv_info);
	void (*close_codec_dma)(void);
	void (*stop_dma)(const struct soundtrigger_pcm *dma_drv_info);
	void (*dma_drv_remove)(void);
};

void soundtrigger_pcm_reset(void);
void set_pcm_ops(struct soundtrigger_pcm_ops *ops);
void da_combine_soundtrigger_pcm_close(void);
#endif
