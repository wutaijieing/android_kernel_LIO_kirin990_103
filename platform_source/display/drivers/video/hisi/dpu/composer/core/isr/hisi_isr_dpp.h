/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef HISI_DISP_ISR_PRIMARY_H
#define HISI_DISP_ISR_PRIMARY_H

#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/types.h>
#include <linux/of.h>
#include "hisi_isr_utils.h"

struct hiace_work_info {
	wait_queue_head_t hiace_wq;
	struct workqueue_struct *hiace_end_wq;
	struct work_struct hiace_end_work;
	uint32_t hiace_done;
	uint32_t hiace_start_flag;
};
#define BIT_HIACE_IND   BIT(0)

void hisi_dpp_isr_init(struct hisi_disp_isr *isr, const uint32_t irq_no,
		uint32_t *irq_state, uint32_t state_count, const char *irq_name, void *parent);

void hisi_hiace_isr_init(void);

#endif /* HISI_DISP_ISR_PRIMARY_H */
