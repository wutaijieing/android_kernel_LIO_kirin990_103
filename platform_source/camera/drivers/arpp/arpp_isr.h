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
#ifndef _ARPP_ISR_H_
#define _ARPP_ISR_H_

#include <linux/types.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include "platform.h"

/* mailbox8(normal) DATA0 bit mask */
#define MBX_NORM_DATA0_INIT_CMPL        (1U << 0)
#define MBX_NORM_DATA0_LBA_CMPL         (1U << 1)
#define MBX_NORM_DATA0_CLR              (1U << 2)
#define MBX_NORM_DATA0_POWER_DOWN       (1U << 3)

/* mailbox9(error) DATA0 bit mask */
#define MBX_ERR_DATA0_PARAM_INVLD       (1U << 0)
#define MBX_ERR_DATA0_AXI_EXCP          (1U << 1)
#define MBX_ERR_DATA0_HWACC_EXCP        (1U << 2)
#define MBX_ERR_DATA0_FLOAT_EXCP        (1U << 3)
#define MBX_ERR_DATA0_ACPU_COMM_TO      (1U << 4)
#define MBX_ERR_DATA0_FSM_STAT_EXCP     (1U << 5)
#define MBX_ERR_DATA0_FSM_DEADLOCK      (1U << 6)
#define MBX_ERR_DATA0_ASC0_EXCP         (1U << 7)
#define MBX_ERR_DATA0_ASC1_EXCP         (1U << 8)
#define MBX_ERR_DATA0_ASC2_EXCP         (1U << 9)
#define MBX_ERR_DATA0_INTR_BUS0_EXCP    (1U << 10)
#define MBX_ERR_DATA0_INTR_BUS1_EXCP    (1U << 11)

struct arpp_ahc {
	wait_queue_head_t hwacc_init_cmpl_wq;
	uint32_t hwacc_init_cmpl_flag;

	wait_queue_head_t hwacc_pd_req_wq;
	uint32_t hwacc_pd_req_flag;

	wait_queue_head_t lba_done_wq;
	uint32_t lba_done_flag;

	struct arpp_device *arpp_dev;
	uint32_t lba_error_flag;
	int (*hwacc_reset_intr)(void);
};

enum ahc_firq_num {
	COMPLETION_FIRQ_NUM = 8,
	EXCEPTION_FIRQ_NUM = 9,
	OTHER_FIRQ_NUM = 10
};

int register_irqs(struct arpp_ahc *ahc_info);
void unregister_irqs(struct arpp_ahc *ahc_info);
int enable_irqs(struct arpp_ahc *ahc_info);
void disable_irqs(struct arpp_ahc *ahc_info);
void disable_irqs_nosync(struct arpp_ahc *ahc_info);
int wait_hwacc_init_completed(struct arpp_ahc *ahc_info);
void send_hwacc_init_completed_signal(struct arpp_ahc *ahc_info);
int wait_hwacc_pd_request(struct arpp_ahc *ahc_info);
void send_hwacc_pd_request_signal(struct arpp_ahc *ahc_info);
int wait_lba_calculation_completed(struct arpp_ahc *ahc_info);
void send_lba_calculation_completed_signal(struct arpp_ahc *ahc_info);
uint32_t read_mailbox_info(struct arpp_device *arpp_dev, uint32_t channel_id);

#endif /* _ARPP_ISR_H_ */
