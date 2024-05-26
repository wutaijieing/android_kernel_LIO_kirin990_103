/*
 * Copyright (C) 2019-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/sched.h>

#include "arpp_isr.h"
#include "arpp_log.h"
#include "arpp_common_define.h"

/* each channel id needs 64 bytes */
#define GET_MBX_OFFSET_BY_CHAN(id)  (id << 6)

/* AHC reg offset */
#define MBX_DATA0                   0x0020
#define MBX_MODE                    0x0010
#define MBX_ICLR                    0x0018

/* AHC mbx mode */
#define STATUS_FREE                 0x11
#define STATUS_SRC                  0x21
#define STATUS_DST                  0x41
#define STATUS_ACK                  0x81

/* AHC bit map */
#define DEST_IPC_FUNC0_MASK         (1U << 0)
#define DEST_IPC_FUNC1_MASK         (1U << 1)
#define DEST_IPC_ACPU_MASK          (1U << 2)
#define DEST_IPC_ISP_MASK           (1U << 3)
#define DEST_IPC_IVP0_MASK          (1U << 4)
#define DEST_IPC_IVP1_MASK          (1U << 5)
#define DEST_IPC_IPP_MASK           (1U << 6)

#define WAIT_HWACC_INI_SECS         (500)
#define WAIT_LBA_CALC_SECS          (1000)
#define WAIT_POWER_DOWN_SECS        (500)

uint32_t g_error_irq_bits[] = {
	MBX_ERR_DATA0_PARAM_INVLD,
	MBX_ERR_DATA0_AXI_EXCP,
	MBX_ERR_DATA0_HWACC_EXCP,
	MBX_ERR_DATA0_FLOAT_EXCP,
	MBX_ERR_DATA0_ACPU_COMM_TO,
	MBX_ERR_DATA0_FSM_STAT_EXCP,
	MBX_ERR_DATA0_FSM_DEADLOCK,
	MBX_ERR_DATA0_ASC0_EXCP,
	MBX_ERR_DATA0_ASC1_EXCP,
	MBX_ERR_DATA0_ASC2_EXCP,
	MBX_ERR_DATA0_INTR_BUS0_EXCP,
	MBX_ERR_DATA0_INTR_BUS1_EXCP,
};

static void clear_irq_force(struct arpp_ahc *ahc_info, uint32_t channel_id)
{
	uint32_t mbx_mode;
	struct arpp_iomem *io_info = NULL;
	char __iomem *ahc_ns_base = NULL;
	char __iomem *hwacc0_cfg_base = NULL;

	if (ahc_info == NULL) {
		hiar_loge("ahc_info is NULL");
		return;
	}

	if (ahc_info->arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return;
	}

	io_info = &ahc_info->arpp_dev->io_info;
	ahc_ns_base = io_info->ahc_ns_base;
	if (ahc_ns_base == NULL) {
		hiar_loge("ahc_ns_base is NULL");
		return;
	}

	hwacc0_cfg_base = io_info->hwacc0_cfg_base;
	if (hwacc0_cfg_base == NULL) {
		hiar_loge("hwacc0_cfg_base is NULL");
		return;
	}

	mbx_mode = readl(ahc_ns_base + MBX_MODE +
		GET_MBX_OFFSET_BY_CHAN(channel_id));
	hiar_logi("mbx_mode is %x", mbx_mode);

	if (mbx_mode != STATUS_ACK)
		return;

	hiar_logi("force to set irq reset");
	writel(HWACC_CFG0_EN0_PRV_RST, hwacc0_cfg_base + CRTL_EN0);
	mbx_mode = readl(ahc_ns_base + MBX_MODE +
		GET_MBX_OFFSET_BY_CHAN(channel_id));
	if (mbx_mode == STATUS_ACK)
		hiar_loge("irq is still unclear");
}

static void clear_irq(struct arpp_ahc *ahc_info, uint32_t channel_id)
{
	struct arpp_iomem *io_info = NULL;
	char __iomem *ahc_ns_base = NULL;

	if (ahc_info == NULL) {
		hiar_loge("ahc_info is NULL");
		return;
	}

	if (ahc_info->arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return;
	}

	io_info = &ahc_info->arpp_dev->io_info;
	ahc_ns_base = io_info->ahc_ns_base;
	if (ahc_ns_base == NULL) {
		hiar_loge("ahc_ns_base is NULL");
		return;
	}

	if (channel_id < COMPLETION_FIRQ_NUM ||
		channel_id > OTHER_FIRQ_NUM) {
		hiar_loge("ahc id is invalid[%d]", channel_id);
		return;
	}

	writel(DEST_IPC_ACPU_MASK, ahc_ns_base +
		MBX_ICLR + GET_MBX_OFFSET_BY_CHAN(channel_id));

	clear_irq_force(ahc_info, channel_id);
}

int wait_hwacc_init_completed(struct arpp_ahc *ahc_info)
{
	int ret;
	uint32_t timeout_interval;
	unsigned long interval_in_jiffies;
	uint32_t mbx_data;

	if (ahc_info == NULL) {
		hiar_loge("ahc_info is NULL");
		return -EINVAL;
	}

	timeout_interval = WAIT_HWACC_INI_SECS;
	hiar_logi("start waitting hwacc init irq");
	interval_in_jiffies = msecs_to_jiffies(timeout_interval);
	ret = wait_event_interruptible_timeout(ahc_info->hwacc_init_cmpl_wq,
		ahc_info->hwacc_init_cmpl_flag, interval_in_jiffies);
	ahc_info->hwacc_init_cmpl_flag = 0;
	if (ret == -ERESTARTSYS) {
		hiar_logi("get hwacc init signal");
		return 0;
	} else if (ret > 0) {
		hiar_logi("wait hwacc init ret=%d", ret);
		return 0;
	}

	hiar_logw("wait hwacc init timeout! attempting to query data from mbox");

	mbx_data = read_mailbox_info(ahc_info->arpp_dev, COMPLETION_FIRQ_NUM);
	if (mbx_data & MBX_NORM_DATA0_INIT_CMPL) {
		hiar_logi("query success! hwacc init success!");
		return 0;
	}

	hiar_logw("query failed!");

	return -1;
}

void send_hwacc_init_completed_signal(struct arpp_ahc *ahc_info)
{
	if (ahc_info == NULL) {
		hiar_loge("ahc_info is NULL");
		return;
	}

	ahc_info->hwacc_init_cmpl_flag = 1;
	wake_up_interruptible_all(&ahc_info->hwacc_init_cmpl_wq);

	hiar_logi("send hwacc init completion signal!");
}

int wait_hwacc_pd_request(struct arpp_ahc *ahc_info)
{
	int ret;
	uint32_t timeout_interval;
	unsigned long interval_in_jiffies;

	if (ahc_info == NULL) {
		hiar_loge("ahc_info is NULL");
		return -EINVAL;
	}

	timeout_interval = WAIT_POWER_DOWN_SECS;
	ahc_info->hwacc_pd_req_flag = 0;
	interval_in_jiffies = msecs_to_jiffies(timeout_interval);
	ret = wait_event_interruptible_timeout(ahc_info->hwacc_pd_req_wq,
		ahc_info->hwacc_pd_req_flag, interval_in_jiffies);
	if (ret == -ERESTARTSYS) {
		hiar_logi("get pd req signal");
		return 0;
	} else if (ret > 0) {
		hiar_logi("wait pd req ret=%d", ret);
		return 0;
	}

	hiar_logw("wait pd request timeout!");

	return -1;
}

void send_hwacc_pd_request_signal(struct arpp_ahc *ahc_info)
{
	if (ahc_info == NULL) {
		hiar_loge("ahc_info is NULL");
		return;
	}

	ahc_info->hwacc_pd_req_flag = 1;
	wake_up_interruptible_all(&ahc_info->hwacc_pd_req_wq);

	hiar_logi("send hwacc power down request signal!");
}

int wait_lba_calculation_completed(struct arpp_ahc *ahc_info)
{
	int ret;
	uint32_t timeout_interval;
	unsigned long interval_in_jiffies;

	if (ahc_info == NULL) {
		hiar_loge("ahc_info is NULL");
		return -EINVAL;
	}

	timeout_interval = WAIT_LBA_CALC_SECS;
	ahc_info->lba_done_flag = 0;
	ahc_info->lba_error_flag = 0;
	interval_in_jiffies = msecs_to_jiffies(timeout_interval);
	ret = wait_event_interruptible_timeout(ahc_info->lba_done_wq,
		(ahc_info->lba_done_flag || ahc_info->lba_error_flag),
		interval_in_jiffies);
	if (ret == 0) {
		hiar_logw("wait calculation timeout!");
		return -1;
	}
	/* wake up by lba_done_flag or lba_error_flag */
	if (ret == -ERESTARTSYS && ahc_info->lba_error_flag == 1) {
		hiar_logw("error irq happend!");
		return -1;
	}

	hiar_logi("wait lba ret=%d", jiffies_to_msecs(ret));

	return 0;
}

void send_lba_calculation_completed_signal(struct arpp_ahc *ahc_info)
{
	if (ahc_info == NULL) {
		hiar_loge("ahc_info is NULL");
		return;
	}

	ahc_info->lba_done_flag = 1;
	wake_up_interruptible_all(&ahc_info->lba_done_wq);

	hiar_logi("send lba calculate completion signal!");
}

static void hwacc_init_interrupt_handler(struct arpp_ahc *ahc_info)
{
	if (ahc_info == NULL) {
		hiar_loge("ahc_info is NULL");
		return;
	}

	send_hwacc_init_completed_signal(ahc_info);
}

static void lba_completed_interrupt_handler(struct arpp_ahc *ahc_info)
{
	if (ahc_info == NULL) {
		hiar_loge("ahc_info is NULL");
		return;
	}

	send_lba_calculation_completed_signal(ahc_info);
}

static void hwacc_pd_request_interrupt_handler(struct arpp_ahc *ahc_info)
{
	if (ahc_info == NULL) {
		hiar_loge("ahc_info is NULL");
		return;
	}

	send_hwacc_pd_request_signal(ahc_info);
}

static void hwacc_error_interrupt_handler(struct arpp_ahc *ahc_info)
{
	if (ahc_info == NULL) {
		hiar_loge("ahc_info is NULL");
		return;
	}

	/* wake up lba calculation sleep */
	ahc_info->lba_error_flag = 1;
}

irqreturn_t arpp_merged_isr(int irq, void *ptr)
{
	UNUSED(irq);
	UNUSED(ptr);
	return IRQ_HANDLED;
}

irqreturn_t arpp_to_acpu_normal_isr(int irq, void *ptr)
{
	struct arpp_ahc *ahc_info = NULL;
	uint32_t ipc_data;
	UNUSED(irq);

	hiar_logi("+");

	ahc_info = (struct arpp_ahc *)ptr;
	if (ahc_info == NULL) {
		hiar_loge("ahc_info is NULL");
		return IRQ_NONE;
	}

	ipc_data = read_mailbox_info(ahc_info->arpp_dev, COMPLETION_FIRQ_NUM);
	hiar_logi("read MBX(normal) ipc data %x", ipc_data);

	clear_irq(ahc_info, COMPLETION_FIRQ_NUM);

	if (ipc_data & MBX_NORM_DATA0_INIT_CMPL) {
		hwacc_init_interrupt_handler(ahc_info);
	} else if (ipc_data & MBX_NORM_DATA0_LBA_CMPL) {
		lba_completed_interrupt_handler(ahc_info);
	} else if (ipc_data & MBX_NORM_DATA0_CLR) {
		hiar_logi("clear all irq succ");
	} else if (ipc_data & MBX_NORM_DATA0_POWER_DOWN) {
		hwacc_pd_request_interrupt_handler(ahc_info);
		hiar_logi("power down succ");
	} else {
		hiar_logw("hit nothing");
	}

	hiar_logi("-");

	return IRQ_HANDLED;
}

irqreturn_t arpp_to_acpu_error_isr(int irq, void *ptr)
{
	int i;
	uint32_t ipc_data;
	struct arpp_ahc *ahc_info = NULL;
	int err_irq_nums = sizeof(g_error_irq_bits) / sizeof(uint32_t);
	UNUSED(irq);

	hiar_logi("+");

	ahc_info = (struct arpp_ahc *)ptr;
	if (ahc_info == NULL) {
		hiar_loge("ahc_info is NULL");
		return IRQ_NONE;
	}

	ipc_data = read_mailbox_info(ahc_info->arpp_dev, EXCEPTION_FIRQ_NUM);
	hiar_logi("read MBX(error) ipc data %x", ipc_data);

	for (i = 0; i < err_irq_nums; ++i) {
		if ((ipc_data & g_error_irq_bits[i]) != 0) {
			hiar_logi("err irq code=%d", i);
			break;
		}
	}

	hwacc_error_interrupt_handler(ahc_info);

	clear_irq(ahc_info, EXCEPTION_FIRQ_NUM);

	hiar_logi("-");

	return IRQ_HANDLED;
}

static void init_wait_queue(struct arpp_ahc *ahc_info)
{
	if (ahc_info == NULL) {
		hiar_loge("ahc_info is NULL");
		return;
	}

	ahc_info->hwacc_init_cmpl_flag = 0;
	init_waitqueue_head(&ahc_info->hwacc_init_cmpl_wq);

	ahc_info->hwacc_pd_req_flag = 0;
	init_waitqueue_head(&ahc_info->hwacc_pd_req_wq);

	ahc_info->lba_done_flag = 0;
	ahc_info->lba_error_flag = 0;
	init_waitqueue_head(&ahc_info->lba_done_wq);
}

int register_irqs(struct arpp_ahc *ahc_info)
{
	int32_t ret;
	struct arpp_device *arpp_dev = NULL;

	hiar_logi("+");

	if (ahc_info == NULL) {
		hiar_loge("ahc_info is NULL");
		return -EINVAL;
	}

	arpp_dev = ahc_info->arpp_dev;
	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return -EINVAL;
	}

	init_wait_queue(ahc_info);

	ret = request_irq(arpp_dev->ahc_normal_irq,
		arpp_to_acpu_normal_isr, 0, "normal_fiq", ahc_info);
	if (ret != 0) {
		hiar_loge("request_irq0 failed %d", ret);
		return ret;
	}
	disable_irq(arpp_dev->ahc_normal_irq);

	ret = request_irq(arpp_dev->ahc_error_irq,
		arpp_to_acpu_error_isr, 0, "error_fiq", ahc_info);
	if (ret != 0) {
		hiar_loge("request_irq1 failed %d", ret);
		goto err_free_normal_irq;
	}
	disable_irq(arpp_dev->ahc_error_irq);

	hiar_logi("-");

	return 0;

err_free_normal_irq:
	free_irq(arpp_dev->ahc_normal_irq, ahc_info);

	return ret;
}

void unregister_irqs(struct arpp_ahc *ahc_info)
{
	struct arpp_device *arpp_dev = NULL;

	hiar_logi("+");

	if (ahc_info == NULL) {
		hiar_loge("ahc_info is NULL");
		return;
	}

	arpp_dev = ahc_info->arpp_dev;
	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return;
	}

	ahc_info->hwacc_init_cmpl_flag = 1;
	ahc_info->hwacc_pd_req_flag = 1;
	ahc_info->lba_done_flag = 1;
	ahc_info->lba_error_flag = 1;

	/* after free_irq, do not set irq to 0 or NULL */
	if (arpp_dev->ahc_normal_irq)
		free_irq(arpp_dev->ahc_normal_irq, ahc_info);

	if (arpp_dev->ahc_error_irq)
		free_irq(arpp_dev->ahc_error_irq, ahc_info);

	hiar_logi("-");
}

int enable_irqs(struct arpp_ahc *ahc_info)
{
	struct arpp_device *arpp_dev = NULL;

	if (ahc_info == NULL) {
		hiar_loge("ahc_info is NULL");
		return -EINVAL;
	}

	arpp_dev = ahc_info->arpp_dev;
	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return -EINVAL;
	}

	if (arpp_dev->ahc_normal_irq != 0)
		enable_irq(arpp_dev->ahc_normal_irq);
	else
		hiar_loge("ahc_normal_irq is 0");

	if (arpp_dev->ahc_error_irq != 0)
		enable_irq(arpp_dev->ahc_error_irq);
	else
		hiar_loge("ahc_error_irq is 0");

	return 0;
}

void disable_irqs(struct arpp_ahc *ahc_info)
{
	struct arpp_device *arpp_dev = NULL;

	if (ahc_info == NULL) {
		hiar_loge("ahc info is NULL");
		return;
	}

	arpp_dev = ahc_info->arpp_dev;
	if (arpp_dev == NULL) {
		hiar_loge("arpp dev is NULL");
		return;
	}

	if (arpp_dev->ahc_normal_irq != 0)
		disable_irq(arpp_dev->ahc_normal_irq);
	else
		hiar_loge("ahc_normal_irq is 0");

	if (arpp_dev->ahc_error_irq != 0)
		disable_irq(arpp_dev->ahc_error_irq);
	else
		hiar_loge("ahc_error_irq is 0");
}

void disable_irqs_nosync(struct arpp_ahc *ahc_info)
{
	struct arpp_device *arpp_dev = NULL;

	if (ahc_info == NULL) {
		hiar_loge("ahc_info is NULL");
		return;
	}

	arpp_dev = ahc_info->arpp_dev;
	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return;
	}

	if (arpp_dev->ahc_normal_irq != 0)
		disable_irq_nosync(arpp_dev->ahc_normal_irq);
	else
		hiar_loge("ahc_normal_irq is 0");

	if (arpp_dev->ahc_error_irq != 0)
		disable_irq_nosync(arpp_dev->ahc_error_irq);
	else
		hiar_loge("ahc_error_irq is 0");
}

uint32_t read_mailbox_info(struct arpp_device *arpp_dev, uint32_t channel_id)
{
	struct arpp_iomem *io_info = NULL;
	char __iomem *ahc_ns_base = NULL;

	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return 0;
	}

	io_info = &arpp_dev->io_info;
	ahc_ns_base = io_info->ahc_ns_base;
	if (ahc_ns_base == NULL) {
		hiar_loge("ahc_ns_base is NULL");
		return 0;
	}

	if (channel_id < COMPLETION_FIRQ_NUM ||
		channel_id > OTHER_FIRQ_NUM) {
		hiar_loge("ahc id is invalid[%d]", channel_id);
		return 0;
	}

	return readl(ahc_ns_base +
		MBX_DATA0 + GET_MBX_OFFSET_BY_CHAN(channel_id));
}
