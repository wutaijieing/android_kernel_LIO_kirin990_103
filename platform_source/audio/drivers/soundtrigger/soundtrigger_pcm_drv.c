/*
 * soundtrigger_pcm_drv.c
 *
 * soundtrigger pcm driver
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2013-2020. All rights reserved.
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

#include "soundtrigger_pcm_drv.h"

#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/miscdevice.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/version.h>
#include <sound/dmaengine_pcm.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>

#include "audio_log.h"
#include "dsp_misc.h"
#include "dsp_utils.h"
#include "asp_dma.h"
#include "soundtrigger_event.h"
#include "soundtrigger_socdsp_mailbox.h"
#include "soundtrigger_socdsp_pcm.h"
#include "soundtrigger_ring_buffer.h"

#define LOG_TAG "soundtrigger"

#define DRV_NAME "soundtrigger_dma_drv"
#define COMP_SOUNDTRIGGER_PCM_DRV_NAME "hisilicon,soundtrigger_pcm_drv"
#define SOUNDTRIGGER_HWLOCK_ID 5
#define MAX_MSG_SIZE 1024

#define SOUNDTRIGGER_CMD_DMA_OPEN  _IO('S', 0x1)
#define SOUNDTRIGGER_CMD_DMA_CLOSE  _IO('S', 0x2)
#define SOUNDTRIGGER_CMD_DMA_READY  _IO('S', 0x3)

#define int_to_addr(low, high) \
	(void *)(uintptr_t)((unsigned long long)(low) | \
	((unsigned long long)(high) << 32))

#ifndef UNUSED_PARAMETER
#define UNUSED_PARAMETER(x) (void)(x)
#endif

static struct soundtrigger_pcm *g_pcm_obj;
static struct soundtrigger_pcm_ops *g_ex_codec_ops;

void set_pcm_ops(struct soundtrigger_pcm_ops *ops)
{
	g_ex_codec_ops = ops;
}

static int get_input_param(unsigned int usr_para_size, const void __user *usr_para_addr,
	 unsigned int *krn_para_size, void **krn_para_addr)
{
	int ret = 0;
	void *para_in = NULL;
	unsigned int para_size_in;

	if (usr_para_addr == NULL) {
		AUDIO_LOGE("usr para addr is null no user data");
		return -EINVAL;
	}

	if ((usr_para_size == 0) || (usr_para_size > MAX_MSG_SIZE)) {
		AUDIO_LOGE("usr buffer size:%u out of range", usr_para_size);
		return -EINVAL;
	}

	para_size_in = roundup(usr_para_size, 4);

	para_in = kzalloc(para_size_in, GFP_KERNEL);
	if (para_in == NULL) {
		AUDIO_LOGE("kzalloc para in fail");
		return -ENOMEM;
	}

	if (copy_from_user(para_in, usr_para_addr, usr_para_size)) {
		AUDIO_LOGE("copy from user fail");
		ret = -EINVAL;
		goto ERR;
	}

	*krn_para_size = para_size_in;
	*krn_para_addr = para_in;

	return ret;

ERR:
	if (para_in != NULL) {
		kfree(para_in);
		para_in = NULL;
	}

	OUT_FUNCTION;

	return ret;
}

static int32_t soundtrigger_pcm_init(struct soundtrigger_pcm_info *pcm_info,
	enum codec_dsp_type dsp_type, uint32_t pcm_channel, uint64_t *cfg_addr)
{
	uint32_t i;
	uint32_t j;
	struct dma_config cfg;

	if (g_ex_codec_ops == NULL)
		return 0;

	for (i = 0; i < ARRAY_SIZE(pcm_info->channel); i++) {
		g_ex_codec_ops->get_dma_cfg(&cfg, dsp_type, pcm_channel, i);

		pcm_info->channel[i] = cfg.channel;
		for (j = 0; j < ARRAY_SIZE(pcm_info->dma_cfg[0]); j++) {
			/* remap the soundtrigger address */
			pcm_info->buffer_phy_addr[i][j] = (void *)(uintptr_t)(*cfg_addr);
			pcm_info->buffer[i][j] = ioremap_wc((phys_addr_t)(*cfg_addr),
				pcm_info->buffer_size);
			if (!pcm_info->buffer[i][j]) {
				AUDIO_LOGE("remap buffer failed");
				return -ENOMEM;
			}

			memset(pcm_info->buffer[i][j], 0, pcm_info->buffer_size);
			*cfg_addr += pcm_info->buffer_size;

			pcm_info->lli_dma_phy_addr[i][j] = (void *)(uintptr_t)(*cfg_addr);
			pcm_info->dma_cfg[i][j] = ioremap_wc((phys_addr_t)(*cfg_addr),
				sizeof(*pcm_info->dma_cfg[i][j]));
			if (!pcm_info->dma_cfg[i][j]) {
				AUDIO_LOGE("remap dma config failed");
				return -ENOMEM;
			}

			memset(pcm_info->dma_cfg[i][j], 0, sizeof(*pcm_info->dma_cfg[i][j]));
			*cfg_addr += sizeof(*pcm_info->dma_cfg[i][j]);

			/* set dma config */
			pcm_info->dma_cfg[i][j]->config = cfg.config;
			pcm_info->dma_cfg[i][j]->src_addr = cfg.port;
			pcm_info->dma_cfg[i][j]->a_count = pcm_info->buffer_size;
		}
	}

	return 0;
}

static void soundtrigger_pcm_uinit(struct soundtrigger_pcm_info *pcm_info)
{
	uint32_t i;
	uint32_t j;

	for (i = 0; i < ARRAY_SIZE(pcm_info->channel); i++) {
		for (j = 0; j < ARRAY_SIZE(pcm_info->dma_cfg[0]); j++) {
			if (pcm_info->buffer[i][j]) {
				iounmap(pcm_info->buffer[i][j]);
				pcm_info->buffer[i][j] = NULL;
			}

			if (pcm_info->dma_cfg[i][j]) {
				iounmap(pcm_info->dma_cfg[i][j]);
				pcm_info->dma_cfg[i][j] = NULL;
			}
		}
	}
}

static void soundtrigger_pcm_adjust(struct soundtrigger_pcm_info *pcm_info)
{
	uint32_t swap_buf_num;
	uint32_t next_addr;
	uint32_t real_pos;
	uint32_t i;
	uint32_t j;

	swap_buf_num = ARRAY_SIZE(pcm_info->dma_cfg[0]);
	for (i = 0; i < ARRAY_SIZE(pcm_info->channel); i++) {
		for (j = 0; j < swap_buf_num; j++) {
			real_pos = (j + 1) % swap_buf_num;
			next_addr = (uint32_t)(uintptr_t)pcm_info->lli_dma_phy_addr[i][real_pos];
			pcm_info->dma_cfg[i][j]->des_addr =
				(uint32_t)(uintptr_t)pcm_info->buffer_phy_addr[i][j];
			pcm_info->dma_cfg[i][j]->lli = drv_dma_lli_link(next_addr);
		}
	}
}

static void clear_dma_config(struct soundtrigger_pcm *obj)
{
	uint32_t i;
	struct soundtrigger_pcm_info *pcm_info = NULL;

	AUDIO_LOGI("dma config clear");

	for (i = 0; i < ARRAY_SIZE(obj->pcm_info); i++) {
		pcm_info = &(obj->pcm_info[i]);
		soundtrigger_pcm_uinit(pcm_info);
	}
}

static int32_t set_dma_config(struct soundtrigger_pcm *obj)
{
	int32_t ret;
	uint32_t i;
	struct soundtrigger_pcm_info *pcm_info = NULL;
	enum codec_dsp_type dsp_type = obj->type;
	uint64_t cfg_addr = get_phy_addr(CODEC_DSP_SOUNDTRIGGER_BASE_ADDR, PLT_HIFI_MEM);

	if (g_ex_codec_ops == NULL)
		return 0;

	for (i = 0; i < ARRAY_SIZE(obj->pcm_info); i++) {
		pcm_info = &(obj->pcm_info[i]);

		g_ex_codec_ops->get_pcm_cfg(&(pcm_info->pcm_cfg), dsp_type, i);

		pcm_info->buffer_size = pcm_info->pcm_cfg.frame_len * pcm_info->pcm_cfg.byte_count;

		ret = soundtrigger_pcm_init(pcm_info, dsp_type, i, &cfg_addr);
		if (ret != 0) {
			AUDIO_LOGE("soundtrigger pcm init error, ret: %d", ret);
			clear_dma_config(obj);
			return ret;
		}

		soundtrigger_pcm_adjust(pcm_info);
	}

	if (g_ex_codec_ops->dump_dma_info != NULL)
		g_ex_codec_ops->dump_dma_info(obj);

	AUDIO_LOGI("success");

	return 0;
}

static bool is_irq_valid(unsigned short int_type)
{
	if (int_type != ASP_DMA_INT_TYPE_TC1 && int_type != ASP_DMA_INT_TYPE_TC2) {
		AUDIO_LOGE("dma irq is invalid, interrupt type: %d", int_type);
		return false;
	}

	return true;
}

static int32_t soundtrigger_pcm_irq_handler(unsigned short int_type,
	unsigned long para, unsigned int dma_channel)
{
	struct soundtrigger_pcm *obj = g_pcm_obj;

	if (obj == NULL) {
		AUDIO_LOGE("pcm_obj is null");
		return -EINVAL;;
	}

	if (!is_irq_valid(int_type))
		return -EINVAL;

	switch (dma_channel) {
	case DMA_FAST_LEFT_CH_NUM:
		(obj->fast_tran_info_left.irq_count)++;
		obj->dma_int_fast_left_flag = 1;
		if (!queue_delayed_work(obj->workqueue.dma_irq_handle_wq,
			&obj->workqueue.dma_fast_left_work, msecs_to_jiffies(0)))
			AUDIO_LOGE("fast left lost msg");
		return 0;

	case DMA_FAST_RIGHT_CH_NUM:
		if (g_ex_codec_ops != NULL && g_ex_codec_ops->dmac_irq_handle != NULL) {
			g_ex_codec_ops->dmac_irq_handle(obj);
			queue_delayed_work(obj->workqueue.dma_irq_handle_wq,
					&obj->workqueue.dma_fast_right_work, msecs_to_jiffies(0));
		}
		return 0;

	case DMA_NORMAL_LEFT_CH_NUM:
		(obj->normal_tran_info.irq_count_left)++;
		obj->dma_int_nomal_flag = 1;
		queue_delayed_work(obj->workqueue.dma_irq_handle_wq,
			&obj->workqueue.dma_normal_left_work, msecs_to_jiffies(0));
		return 0;

	case DMA_NORMAL_RIGHT_CH_NUM:
		(obj->normal_tran_info.irq_count_right)++;
		queue_delayed_work(obj->workqueue.dma_irq_handle_wq,
			&obj->workqueue.dma_normal_right_work, msecs_to_jiffies(0));
		return 0;

	default:
		AUDIO_LOGE("dma interrupt error, dma channel: %u", dma_channel);
		return -EINVAL;
	}

	return 0;
}

static void start_dma(struct soundtrigger_pcm *obj)
{
	uint32_t pcm_num;
	uint32_t dma_channel;
	uint32_t dma_port_num;
	uint32_t i;
	uint32_t j;
	struct soundtrigger_pcm_info *pcm_info = NULL;

	pcm_num = ARRAY_SIZE(obj->pcm_info);
	for (i = 0; i < pcm_num; i++) {
		pcm_info = &(obj->pcm_info[i]);
		dma_port_num = ARRAY_SIZE(pcm_info->channel);
		for (j = 0; j < dma_port_num; j++) {
			dma_channel = pcm_info->channel[j];
			asp_dma_config((unsigned short)dma_channel,
				pcm_info->dma_cfg[j][0], soundtrigger_pcm_irq_handler, 0);
			asp_dma_start((unsigned short)dma_channel, pcm_info->dma_cfg[j][0]);
		}
	}
}

static void fast_info_init(struct fast_tran_info *fast_info)
{
	memset(fast_info->fast_buffer, 0, sizeof(uint16_t) * FAST_BUFFER_SIZE);

	fast_info->fast_read_complete_flag = READ_NOT_COMPLETE;
	fast_info->dma_tran_count = 0;
	fast_info->fast_complete_flag = FAST_TRAN_NOT_COMPLETE;
	fast_info->fast_head_frame_word = FRAME_MAGIC_WORD;

	fast_info->fast_head_frame_size = FAST_FRAME_SIZE;
	fast_info->dma_tran_total_count = FAST_TRAN_COUNT;

	fast_info->fast_frame_find_flag = FRAME_NOT_FIND;
	fast_info->irq_count = 0;
	fast_info->read_count = 0;
	fast_info->fast_head_word_count = 0;
}

static void fast_info_deinit(struct fast_tran_info *fast_info)
{
	memset(fast_info->fast_buffer, 0, sizeof(uint16_t) * FAST_BUFFER_SIZE);

	fast_info->irq_count = 0;
	fast_info->read_count = 0;
	fast_info->dma_tran_count = 0;
	fast_info->dma_tran_total_count = 0;
}

static int32_t normal_info_init(struct normal_tran_info *normal_info,
	const struct soundtrigger_pcm *obj)
{
	normal_info->normal_buffer = st_ring_buffer_init(RINGBUF_FRAME_SIZE, RINGBUF_FRAME_COUNT);
	if (normal_info->normal_buffer == NULL) {
		AUDIO_LOGE("ring buffer init failed");
		return -ENOMEM;
	}

	normal_info->normal_head_frame_word = FRAME_MAGIC_WORD;
	if (obj->type == CODEC_DA_COMBINE_V3) {
		normal_info->normal_head_frame_size = DA_COMBINE_V3_NORMAL_FRAME_SIZE;
	}
#ifdef CONFIG_SND_SOC_DA_COMBINE_V5
	else if (obj->type == CODEC_DA_COMBINE_V5) {
		normal_info->normal_head_frame_size = DA_COMBINE_V5_NORMAL_FRAME_SIZE;
	}
#endif
#ifdef CONFIG_SND_SOC_THIRD_CODEC
	else if (obj->type == CODEC_THIRD_CODEC) {
		normal_info->normal_head_frame_size = THIRD_CODEC_NORMAL_FRAME_SIZE;
	}
#endif
	else {
		normal_info->normal_buffer->deinit(normal_info->normal_buffer);
		normal_info->normal_buffer = NULL;
		AUDIO_LOGE("device type is err: %d", obj->type);
		return -EINVAL;
	}

	normal_info->normal_frame_find_flag = FRAME_NOT_FIND;
	normal_info->normal_first_frame_read_flag = READ_NOT_COMPLETE;
	normal_info->normal_tran_count = 0;
	normal_info->irq_count_left = 0;
	normal_info->irq_count_right = 0;
	normal_info->read_count_left = 0;
	normal_info->read_count_right = 0;
	normal_info->normal_head_word_count = 0;

	return 0;
}

static void normal_info_deinit(struct normal_tran_info *normal_info)
{
	normal_info->irq_count_left = 0;
	normal_info->irq_count_right = 0;
	normal_info->read_count_left = 0;
	normal_info->read_count_right = 0;
	normal_info->normal_tran_count = 0;

	if (normal_info->normal_buffer) {
		normal_info->normal_buffer->deinit(normal_info->normal_buffer);
		normal_info->normal_buffer = NULL;
	}
}

static int32_t check_fast_info(const struct fast_tran_info *fast_info)
{
	if (fast_info->fast_complete_flag == FAST_TRAN_COMPLETE) {
		AUDIO_LOGE("fast transmit complete");
		return -EAGAIN;
	}

	if (fast_info->read_count >= fast_info->irq_count) {
		AUDIO_LOGE("read count %u out of range irq count %u error",
			fast_info->read_count, fast_info->irq_count);
		return -EAGAIN;
	}

	if (fast_info->read_count >= FAST_CHANNEL_TIMEOUT_READ_COUNT) {
		if (g_ex_codec_ops != NULL && g_ex_codec_ops->stop_dma != NULL)
			g_ex_codec_ops->stop_dma(g_pcm_obj);

		AUDIO_LOGE("dma fast channel timeout");
		return -EAGAIN;
	}

	return 0;
}

static int32_t soundtrigger_pcm_open(struct st_fast_status *fast_status)
{
	struct soundtrigger_pcm *obj = g_pcm_obj;
	struct fast_tran_info *fast_info_left = &obj->fast_tran_info_left;
	struct normal_tran_info *normal_info = &obj->normal_tran_info;
	int32_t ret;

	if (obj->is_open) {
		AUDIO_LOGE("soundtrigger pcm drv is already opened");
		return -EAGAIN;
	}

	AUDIO_LOGI("enter");

	fast_info_init(fast_info_left);
	fast_info_init(&obj->fast_tran_info_right);

	ret = normal_info_init(normal_info, obj);
	if (ret != 0)
		return ret;

	obj->fm_status = fast_status->fm_status;

	if (obj->dma_alloc_flag == 0) {
		ret = set_dma_config(obj);
		if (ret != 0) {
			normal_info_deinit(normal_info);

			AUDIO_LOGE("set dma config failed, err: %d", ret);
			return ret;
		}

		obj->dma_alloc_flag = 1;
	}

	obj->dma_int_fast_left_flag = 0;

	if (g_ex_codec_ops != NULL && g_ex_codec_ops->open != NULL)
		g_ex_codec_ops->open(obj);

	obj->dma_int_nomal_flag = 0;

	AUDIO_LOGI("soundtrigger_pcm open aspclk: %d, --",
		clk_get_enable_count(obj->asp_subsys_clk));

	ret = clk_prepare_enable(obj->asp_subsys_clk);
	if (ret != 0) {
		clear_dma_config(obj);
		normal_info_deinit(normal_info);

		AUDIO_LOGE("clk prepare enable failed, ret: %d", ret);
		return ret;
	}

	AUDIO_LOGI("soundtrigger_pcm open aspclk: %d, ++",
		clk_get_enable_count(obj->asp_subsys_clk));

	obj->is_open = true;

	return 0;
}

static int32_t soundtrigger_pcm_start(struct st_fast_status *fast_status)
{
	struct soundtrigger_pcm *obj = g_pcm_obj;
	int32_t ret;

	AUDIO_LOGI("dma start");

	if (obj == NULL) {
		AUDIO_LOGE("soundtrigger pcm_obj is null");
		return -ENOENT;
	}

	if (!obj->is_open) {
		AUDIO_LOGE("soundtrigger dma drv is not open");
		return -EAGAIN;
	}

	if (obj->is_dma_enable) {
		AUDIO_LOGE("soundtrigger dma drv is already enabled");
		return -EAGAIN;
	}

	if (g_ex_codec_ops != NULL && g_ex_codec_ops->start_dma != NULL) {
		ret = g_ex_codec_ops->start_dma(obj);
		if (ret != 0)
			return ret;
	}
	__pm_stay_awake(obj->wake_lock);

	if (!queue_delayed_work(obj->workqueue.dma_close_wq,
		&obj->workqueue.close_dma_timeout_work,
		msecs_to_jiffies(TIMEOUT_CLOSE_DMA_MS)))
		AUDIO_LOGE("close dma timeout lost msg");

	start_dma(obj);
	obj->is_dma_enable = 1;

	return 0;
}

static int32_t dma_info_deinit(struct soundtrigger_pcm *obj)
{
	int32_t ret;
	struct fast_tran_info *fast_info_left = &obj->fast_tran_info_left;
	struct normal_tran_info *normal_info = &obj->normal_tran_info;

	obj->is_open = false;

	if (obj->is_dma_enable) {
		if (g_ex_codec_ops != NULL && g_ex_codec_ops->deinit_dma != NULL)
			g_ex_codec_ops->deinit_dma(obj);

		obj->is_dma_enable = 0;
	}

	if (obj->is_codec_bus_enable && g_ex_codec_ops != NULL && g_ex_codec_ops->deinit_dma_info != NULL) {
		ret = g_ex_codec_ops->deinit_dma_info(obj);
		if (ret != 0)
			AUDIO_LOGW("deinit dma failed");
	}

	cancel_delayed_work(&obj->workqueue.dma_fast_left_work);
	cancel_delayed_work(&obj->workqueue.dma_fast_right_work);
	cancel_delayed_work(&obj->workqueue.dma_normal_left_work);
	cancel_delayed_work(&obj->workqueue.dma_normal_right_work);
	flush_workqueue(obj->workqueue.dma_irq_handle_wq);

	fast_info_deinit(fast_info_left);
	fast_info_deinit(&obj->fast_tran_info_right);
	normal_info_deinit(normal_info);

	return 0;
}

static int32_t soundtrigger_pcm_close(void)
{
	int32_t ret;
	struct soundtrigger_pcm *obj = g_pcm_obj;

	AUDIO_LOGI("dma close");

	if (obj == NULL)
		return -ENOENT;

	if (!obj->is_open) {
		if (obj->wake_lock->active)
			__pm_relax(obj->wake_lock);

		AUDIO_LOGE("soundtrigger dma drv is not open");
		return -EAGAIN;
	}

	ret = dma_info_deinit(obj);
	if (ret != 0)
		AUDIO_LOGW("dma close info deinit error, ret: %d", ret);

	if (obj->wake_lock->active)
		__pm_relax(obj->wake_lock);

	AUDIO_LOGI("soundtrigger dma close asp clk: %d, ++",
		clk_get_enable_count(obj->asp_subsys_clk));

	clk_disable_unprepare(obj->asp_subsys_clk);

	AUDIO_LOGI("soundtrigger dma close asp clk: %d, --",
		clk_get_enable_count(obj->asp_subsys_clk));

	return 0;
}

static int32_t dma_fops_open(struct inode *finode, struct file *fd)
{
	if (g_pcm_obj == NULL)
		return -ENOENT;

	return 0;
}

static int32_t dma_fops_release(struct inode *finode, struct file *fd)
{
	if (g_pcm_obj == NULL)
		return -ENOENT;

	return 0;
}

static ssize_t dma_get_max_read_len(enum codec_dsp_type codec_type,
	size_t *max_read_len, size_t count)
{
	if (codec_type == CODEC_DA_COMBINE_V3) {
		*max_read_len = (RINGBUF_SIZE > DA_COMBINE_V3_NORMAL_FRAME_SIZE) ?
			RINGBUF_SIZE : DA_COMBINE_V3_NORMAL_FRAME_SIZE;
	}
#ifdef CONFIG_SND_SOC_DA_COMBINE_V5
	else if (codec_type == CODEC_DA_COMBINE_V5) {
		*max_read_len = (RINGBUF_SIZE > DA_COMBINE_V5_NORMAL_FRAME_SIZE) ?
			RINGBUF_SIZE : DA_COMBINE_V5_NORMAL_FRAME_SIZE;
	}
#endif
#ifdef CONFIG_SND_SOC_THIRD_CODEC
	else if (codec_type == CODEC_THIRD_CODEC) {
		*max_read_len = (RINGBUF_SIZE > THIRD_CODEC_NORMAL_FRAME_SIZE) ?
			RINGBUF_SIZE : THIRD_CODEC_NORMAL_FRAME_SIZE;
	}
#endif
	else {
		AUDIO_LOGE("type is invalid, codec type: %d", codec_type);
		return -EINVAL;
	}

	if (count < *max_read_len) {
		AUDIO_LOGE("user buffer too short, need %zu", *max_read_len);
		return -EINVAL;
	}

	return 0;
}

static ssize_t read_normal_data(struct normal_tran_info *normal_info,
	char __user *buffer)
{
	uint16_t *pcm_buf = NULL;
	static uint16_t static_buffer[RINGBUF_FRAME_LEN];
	int32_t rest_bytes;
	uint32_t vaild_buf_len;

	if ((normal_info->normal_buffer == NULL) ||
		(normal_info->normal_buffer->empty(normal_info->normal_buffer)))
		return -EFAULT;

	if (normal_info->normal_start_addr > RINGBUF_FRAME_LEN) {
		AUDIO_LOGE("normal start addr error: %u", normal_info->normal_start_addr);
		return -EINVAL;
	}

	pcm_buf = kzalloc(RINGBUF_FRAME_SIZE, GFP_KERNEL);
	if (pcm_buf == NULL) {
		AUDIO_LOGE("pcm buffer kzalloc failed");
		return -ENOMEM;
	}

	normal_info->normal_buffer->get(normal_info->normal_buffer, pcm_buf, RINGBUF_FRAME_SIZE);
	vaild_buf_len = RINGBUF_FRAME_LEN - normal_info->normal_start_addr;

	if (normal_info->normal_first_frame_read_flag == READ_NOT_COMPLETE) {
		normal_info->normal_first_frame_read_flag = READ_COMPLETE;
		memcpy(static_buffer, pcm_buf + normal_info->normal_start_addr,
			vaild_buf_len * VALID_BYTE_COUNT_EACH_SAMPLE_POINT);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		kfree_sensitive(pcm_buf);
#else
		kzfree(pcm_buf);
#endif
		return -EINVAL;
	}

	memcpy(static_buffer + vaild_buf_len, pcm_buf,
		normal_info->normal_start_addr * VALID_BYTE_COUNT_EACH_SAMPLE_POINT);

	rest_bytes = copy_to_user(buffer, static_buffer, RINGBUF_FRAME_SIZE);
	memcpy(static_buffer, pcm_buf + normal_info->normal_start_addr,
		vaild_buf_len * VALID_BYTE_COUNT_EACH_SAMPLE_POINT);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
		kfree_sensitive(pcm_buf);
#else
		kzfree(pcm_buf);
#endif
	if (rest_bytes != 0) {
		AUDIO_LOGE("copy to user failed, rest bytes: %d", rest_bytes);
		return -EINVAL;
	}

	return 0;
}

static int32_t check_dma_read_info(struct soundtrigger_pcm *obj,
	char __user *buf)
{
	if (buf == NULL) {
		AUDIO_LOGE("buffer is invalid");
		return -EINVAL;
	}

	if (obj == NULL) {
		AUDIO_LOGE("pcm obj is null");
		return -EINVAL;
	}

	if (!obj->is_open) {
		AUDIO_LOGE("soundtrigger dma aleady closed");
		return -EINVAL;
	}

	return 0;
}

static ssize_t dma_fops_read(struct file *file,
	char __user *buffer, size_t count, loff_t *f_ops)
{
	size_t max_read_len = 0;
	ssize_t ret;
	struct soundtrigger_pcm *obj = g_pcm_obj;
	struct fast_tran_info *fast_info_left = &obj->fast_tran_info_left;
	struct fast_tran_info *fast_info_right = &obj->fast_tran_info_right;
	struct normal_tran_info *normal_info = NULL;

	ret = check_dma_read_info(obj, buffer);
	if (ret != 0) {
		AUDIO_LOGE("check dma read info error, ret: %zd", ret);
		return ret;
	}

	ret = dma_get_max_read_len(obj->type, &max_read_len, count);
	if (ret < 0) {
		AUDIO_LOGE("get max read len error, ret: %zd", ret);
		return ret;
	}
	if (g_ex_codec_ops != NULL && g_ex_codec_ops->check_fast_complete_flag != NULL) {
		ret = g_ex_codec_ops->check_fast_complete_flag(obj);
		if (ret != 0) {
			AUDIO_LOGE("check fast complete flag error, ret: %zd", ret);
			return ret;
		}
	}
	if (g_ex_codec_ops != NULL && g_ex_codec_ops->set_dma_int_flag != NULL)
		g_ex_codec_ops->set_dma_int_flag(obj);

	if (fast_info_left->fast_read_complete_flag == READ_NOT_COMPLETE) {
		if (g_ex_codec_ops != NULL && g_ex_codec_ops->read_data != NULL) {
			ret = g_ex_codec_ops->read_data(fast_info_left, fast_info_right, buffer, max_read_len);
			if (ret == 0)
				return max_read_len;
		}
	} else {
		normal_info = &(obj->normal_tran_info);
		ret = read_normal_data(normal_info, buffer);
		if (ret == 0)
			return RINGBUF_FRAME_SIZE;
	}

	return -EINVAL;
}

static int32_t soundtrigger_pcm_fops(uint32_t cmd, struct st_fast_status *fast_status)
{
	int32_t ret = 0;

	if (fast_status == NULL) {
		AUDIO_LOGE("pointer is null");
		return -EINVAL;
	}

	if (g_pcm_obj == NULL) {
		AUDIO_LOGE("pcm_obj is null");
		return -EINVAL;
	}

	if (cmd == SOUNDTRIGGER_CMD_DMA_CLOSE) {
		if (cancel_delayed_work_sync(&g_pcm_obj->workqueue.close_dma_timeout_work))
			AUDIO_LOGI("cancel timeout dma close work success");
	}

	mutex_lock(&g_pcm_obj->ioctl_mutex);
	switch (cmd) {
	case SOUNDTRIGGER_CMD_DMA_READY:
		ret = soundtrigger_pcm_open(fast_status);
		AUDIO_LOGI("dma open, ret[%d]", ret);
		break;
	case SOUNDTRIGGER_CMD_DMA_OPEN:
		ret = soundtrigger_pcm_start(fast_status);
		AUDIO_LOGI("dma start, ret[%d]", ret);
		break;
	case SOUNDTRIGGER_CMD_DMA_CLOSE:
		ret = soundtrigger_pcm_close();
		AUDIO_LOGI("dma close, ret[%d]", ret);
		break;
	default:
		AUDIO_LOGE("invalid value, ret[%d]", ret);
		ret = -ENOTTY;
		break;
	}
	mutex_unlock(&g_pcm_obj->ioctl_mutex);

	return ret;
}

static long dma_fops_ioctl(struct file *fd, uint32_t cmd, uintptr_t arg)
{
	int32_t ret;
	struct st_fast_status *fast_status = NULL;
	struct soundtrigger_io_sync_param param;
	struct da_combine_param_io_buf krn_param;

	if (!(void __user *)arg) {
		AUDIO_LOGE("input error: arg is null");
		return -EINVAL;
	}

	if (copy_from_user(&param, (void __user *)arg, sizeof(param))) {
		AUDIO_LOGE("copy from user failed");
		return -EIO;
	}

	ret = get_input_param(param.para_size_in, int_to_addr(param.para_in_l, param.para_in_h),
		&krn_param.buf_size_in, (void **)&krn_param.buf_in);
	if (ret != 0) {
		AUDIO_LOGE("input error: input param is not valid");
		return -EINVAL;
	}

	fast_status = (struct st_fast_status *)krn_param.buf_in;

	ret = soundtrigger_pcm_fops(cmd, fast_status);
	if (ret != 0)
		AUDIO_LOGW("soundtrigger dma fops error");

	kfree(krn_param.buf_in);

	return ret;
}

static long dma_fops_ioctl32(struct file *fd, uint32_t cmd, unsigned long arg)
{
	void __user *user_arg = (void __user *)compat_ptr(arg);

	return dma_fops_ioctl(fd, cmd, (uintptr_t)user_arg);
}

static void soundtrigger_close_dma(void)
{
	int32_t ret;

	if (g_pcm_obj == NULL)
		return;

	mutex_lock(&g_pcm_obj->ioctl_mutex);
	ret = soundtrigger_pcm_close();
	if (ret != 0)
		AUDIO_LOGW("dma close failed, ret: %d", ret);
	mutex_unlock(&g_pcm_obj->ioctl_mutex);
}

static void dma_fast_left_delaywork(struct work_struct *work)
{
	static uint32_t om_fast_count;
	struct soundtrigger_pcm *obj = NULL;
	struct soundtrigger_pcm_info *pcm_info = NULL;
	struct fast_tran_info *fast_info = NULL;
	int32_t ret;

	om_fast_count++;
	if (om_fast_count == 50) {
		om_fast_count = 0;
		AUDIO_LOGI("fast dma irq come");
	}

	obj = container_of(work, struct soundtrigger_pcm,
		workqueue.dma_fast_left_work.work);
	if (obj == NULL) {
		AUDIO_LOGE("pcm_obj get error");
		return;
	}

	if (!obj->is_open) {
		AUDIO_LOGE("drv is not open, work queue don't process");
		return;
	}

	pcm_info = &obj->pcm_info[PCM_FAST];
	if (pcm_info->buffer_size == 0) {
		AUDIO_LOGE("pcm info buffer size is 0");
		return;
	}

	fast_info = &obj->fast_tran_info_left;
	ret = check_fast_info(fast_info);
	if (ret != 0) {
		AUDIO_LOGE("check fast info error, ret: %d", ret);
		return;
	}
	if (g_ex_codec_ops != NULL && g_ex_codec_ops->proc_fast_trans_buff != NULL)
		g_ex_codec_ops->proc_fast_trans_buff(fast_info, pcm_info, obj);
}

static void dma_fast_right_delaywork(struct work_struct *work)
{
	struct soundtrigger_pcm *obj = NULL;
	static uint32_t om_fast_right_count;

	om_fast_right_count++;
	if (om_fast_right_count == 50) {
		om_fast_right_count = 0;
		AUDIO_LOGI("right fast dma irq come");
	}

	obj = container_of(work, struct soundtrigger_pcm,
		workqueue.dma_fast_right_work.work);

	if (obj == NULL) {
		AUDIO_LOGE("right pcm_obj null");
		return;
	}

	if (!obj->is_open) {
		AUDIO_LOGE("right drv is not open, work queue don't process");
		return;
	}
	if (g_ex_codec_ops != NULL && g_ex_codec_ops->proc_fast_data != NULL)
		g_ex_codec_ops->proc_fast_data(obj);
}

static void dma_normal_left_delaywork(struct work_struct *work)
{
	static uint32_t om_normal_count;
	struct soundtrigger_pcm *obj = NULL;

	om_normal_count++;
	if (om_normal_count == 50) {
		om_normal_count = 0;
		AUDIO_LOGI("normal dma irq come");
	}

	obj = container_of(work, struct soundtrigger_pcm,
		workqueue.dma_normal_left_work.work);
	if (obj == NULL) {
		AUDIO_LOGE("pcm_obj is null");
		return;
	}

	if (!obj->is_open) {
		AUDIO_LOGE("drv is not open, work queue don't process");
		return;
	}

	if (g_ex_codec_ops != NULL && g_ex_codec_ops->proc_normal_data != NULL)
		g_ex_codec_ops->proc_normal_data(obj);
}

static void dma_normal_right_delaywork(struct work_struct *work)
{
	struct soundtrigger_pcm *obj = container_of(work, struct soundtrigger_pcm,
		workqueue.dma_normal_right_work.work);

	if (obj == NULL) {
		AUDIO_LOGE("pcm_obj null");
		return;
	}

	if (!obj->is_open) {
		AUDIO_LOGE("drv is not open, work queue don't process");
		return;
	}

	if (obj->normal_tran_info.read_count_right >=
		obj->normal_tran_info.irq_count_right)
		return;

	obj->normal_tran_info.read_count_right++;
}

static void close_dma_timeout_delaywork(struct work_struct *work)
{
	UNUSED_PARAMETER(work);

	AUDIO_LOGI("enter");

	if (g_ex_codec_ops != NULL && g_ex_codec_ops->close_codec_dma != NULL)
		g_ex_codec_ops->close_codec_dma();

	soundtrigger_close_dma();
}

void soundtrigger_pcm_reset(void)
{
	int32_t ret;
	struct st_fast_status fast_status = {0};

	ret = soundtrigger_pcm_fops(SOUNDTRIGGER_CMD_DMA_CLOSE, &fast_status);
	if (ret != 0)
		AUDIO_LOGW("soundtrigger dma fops error");
}

int32_t soundtrigger_set_codec_type(enum codec_dsp_type type)
{
	if (g_pcm_obj == NULL) {
		AUDIO_LOGE("device not init, use default config");
		return -ENOENT;
	}

	g_pcm_obj->type = type;
	AUDIO_LOGI("dsp type: %d", type);

	return 0;
}

static const struct file_operations g_device_fops = {
	.owner = THIS_MODULE,
	.open = dma_fops_open,
	.release = dma_fops_release,
	.read = dma_fops_read,
	.unlocked_ioctl = dma_fops_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = dma_fops_ioctl32,
#endif
};

static struct miscdevice g_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DRV_NAME,
	.fops = &g_device_fops,
};

static int soundtrigger_create_workqueue(struct soundtrigger_pcm *obj)
{
	obj->workqueue.dma_irq_handle_wq = create_singlethread_workqueue("soundtrigger_dma_irq_handle_wq");
	if (!obj->workqueue.dma_irq_handle_wq) {
		AUDIO_LOGE("create dma irq handle workqueue failed");
		return -ENOMEM;
	}

	obj->workqueue.dma_close_wq = create_singlethread_workqueue("soundtrigger_dma_close_wq");
	if (!obj->workqueue.dma_close_wq) {
		flush_workqueue(obj->workqueue.dma_irq_handle_wq);
		destroy_workqueue(obj->workqueue.dma_irq_handle_wq);
		AUDIO_LOGE("create dma close workqueue failed");
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&obj->workqueue.dma_fast_left_work, dma_fast_left_delaywork);
	INIT_DELAYED_WORK(&obj->workqueue.dma_fast_right_work, dma_fast_right_delaywork);
	INIT_DELAYED_WORK(&obj->workqueue.dma_normal_left_work, dma_normal_left_delaywork);
	INIT_DELAYED_WORK(&obj->workqueue.dma_normal_right_work, dma_normal_right_delaywork);
	INIT_DELAYED_WORK(&obj->workqueue.close_dma_timeout_work, close_dma_timeout_delaywork);

	return 0;
}

static void soundtrigger_destroy_workqueue(struct soundtrigger_pcm *obj)
{
	if (obj->workqueue.dma_irq_handle_wq) {
		cancel_delayed_work(&obj->workqueue.dma_fast_left_work);
		cancel_delayed_work(&obj->workqueue.dma_fast_right_work);
		cancel_delayed_work(&obj->workqueue.dma_normal_left_work);
		cancel_delayed_work(&obj->workqueue.dma_normal_right_work);
		flush_workqueue(obj->workqueue.dma_irq_handle_wq);
		destroy_workqueue(obj->workqueue.dma_irq_handle_wq);
	}

	if (obj->workqueue.dma_irq_handle_wq) {
		cancel_delayed_work(&obj->workqueue.close_dma_timeout_work);
		flush_workqueue(obj->workqueue.dma_irq_handle_wq);
		destroy_workqueue(obj->workqueue.dma_irq_handle_wq);
	}
}

static int devm_remap_base_addr(struct soundtrigger_pcm *info, struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;

	info->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!info->res) {
		AUDIO_LOGE("get resource error");
		return -ENOENT;
	}

	info->reg_base_addr = devm_ioremap(dev, info->res->start, resource_size(info->res));
	if (!info->reg_base_addr) {
		AUDIO_LOGE("reg base addr ioremap failed");
		return -ENOMEM;
	}

	return 0;
}

static int init_resource(struct platform_device *pdev, struct soundtrigger_pcm **out_obj)
{
	struct device *dev = &pdev->dev;
	struct soundtrigger_pcm *obj = NULL;
	int ret;

	obj = devm_kzalloc(dev, sizeof(*obj), GFP_KERNEL);
	if (obj == NULL) {
		AUDIO_LOGE("malloc failed");
		return -ENOMEM;
	}

	ret = devm_remap_base_addr(obj, pdev);
	if (ret != 0)
		return ret;

	obj->asp_ip = devm_regulator_get(dev, "asp-dmac");
	if (IS_ERR(obj->asp_ip)) {
		AUDIO_LOGE("regulator asp dmac failed");
		return -ENOENT;
	}

	obj->asp_subsys_clk = devm_clk_get(dev, "clk_asp_subsys");
	if (IS_ERR(obj->asp_subsys_clk)) {
		ret = PTR_ERR(obj->asp_subsys_clk);
		AUDIO_LOGE("get asp subsys clk failed, ret: %d", ret);
		return ret;
	}

	obj->hwlock = hwspin_lock_request_specific(SOUNDTRIGGER_HWLOCK_ID);
	if (!obj->hwlock) {
		AUDIO_LOGE("get hwspin error");
		return -ENOENT;
	}

	ret = soundtrigger_create_workqueue(obj);
	if (ret != 0) {
		hwspin_lock_free(obj->hwlock);
		return ret;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	obj->wake_lock = wakeup_source_register(dev, "da_combine-soundtrigger");
#else
	obj->wake_lock = wakeup_source_register("da_combine-soundtrigger");
#endif
	if (obj->wake_lock == NULL) {
		AUDIO_LOGE("request wakeup source failed");
		soundtrigger_destroy_workqueue(obj);
		hwspin_lock_free(obj->hwlock);
		return -ENOMEM;
	}

	mutex_init(&obj->ioctl_mutex);
	spin_lock_init(&obj->lock);

	obj->dev = dev;
	obj->is_open = false;
	obj->dma_alloc_flag = 0;
	obj->is_dma_enable = 0;
	obj->is_codec_bus_enable = 0;

	*out_obj = obj;

	return 0;
}

static void deinit_resource(struct soundtrigger_pcm *obj)
{
	mutex_destroy(&obj->ioctl_mutex);
	wakeup_source_unregister(obj->wake_lock);

	soundtrigger_destroy_workqueue(obj);

	if (hwspin_lock_free(obj->hwlock))
		AUDIO_LOGE("free pcm_obj hwlock fail");
}

static int soundtrigger_pcm_drv_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct soundtrigger_pcm *obj = NULL;
	int ret;

	if (dev == NULL) {
		AUDIO_LOGE("dev is null");
		return -EINVAL;
	}

	ret = misc_register(&g_misc_device);
	if (ret != 0) {
		AUDIO_LOGE("misc registe failed, ret:%d", ret);
		return ret;
	}

	ret = init_resource(pdev, &obj);
	if (ret != 0) {
		AUDIO_LOGE("resource init failed, ret:%d", ret);
		goto init_resource_err;
	}

	platform_set_drvdata(pdev, obj);

	if (of_property_read_bool(dev->of_node, "third_codec_support")) {
		obj->type = CODEC_THIRD_CODEC;
	} else {
		ret = soundtrigger_socdsp_pcm_probe(dev);
		if (ret != 0) {
			goto socdsp_pcm_probe_err;
		}
	}

	g_pcm_obj = obj;

	AUDIO_LOGI("success");
	return 0;

socdsp_pcm_probe_err:
	deinit_resource(obj);
	clear_dma_config(obj);

init_resource_err:
	misc_deregister(&g_misc_device);

	return ret;
}

static int32_t soundtrigger_pcm_drv_remove(struct platform_device *pdev)
{
	UNUSED_PARAMETER(pdev);

	if (g_pcm_obj == NULL) {
		AUDIO_LOGE("pcm obj is null");
		return -ENOENT;
	}

	deinit_resource(g_pcm_obj);
	soundtrigger_socdsp_pcm_deinit();

	clear_dma_config(g_pcm_obj);
	g_pcm_obj->dma_alloc_flag = 0;

	misc_deregister(&g_misc_device);

	if (g_ex_codec_ops != NULL && g_ex_codec_ops->dma_drv_remove != NULL)
		g_ex_codec_ops->dma_drv_remove();

	g_pcm_obj = NULL;
	g_ex_codec_ops = NULL;

	return 0;
}

static const struct of_device_id soundtrigger_pcm_match_table[] = {
	{ .compatible = COMP_SOUNDTRIGGER_PCM_DRV_NAME, },
	{},
};
MODULE_DEVICE_TABLE(of, soundtrigger_pcm_match_table);

static struct platform_driver soundtrigger_pcm_driver = {
	.driver = {
		.name = "soundtrigger dma drviver",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(soundtrigger_pcm_match_table),
	},
	.probe = soundtrigger_pcm_drv_probe,
	.remove = soundtrigger_pcm_drv_remove,
};

static int32_t __init soundtrigger_pcm_drv_init(void)
{
	AUDIO_LOGI("enter");
	return platform_driver_register(&soundtrigger_pcm_driver);
}
module_init(soundtrigger_pcm_drv_init);

static void __exit soundtrigger_pcm_drv_exit(void)
{
	AUDIO_LOGI("enter");
	platform_driver_unregister(&soundtrigger_pcm_driver);
}
module_exit(soundtrigger_pcm_drv_exit);

MODULE_DESCRIPTION("Soundtrigger PCM Driver");
MODULE_LICENSE("GPL v2");

