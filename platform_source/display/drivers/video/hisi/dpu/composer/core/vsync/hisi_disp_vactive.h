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
#ifndef HISI_DISP_VACTIVE_H
#define HISI_DISP_VACTIVE_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/wait.h>

#define DSS_COMPOSER_TIMEOUT_THRESHOLD_FPGA 10000
#define DSS_COMPOSER_TIMEOUT_THRESHOLD_ASIC 300
#define VIDEO_MODE 0
#define CMD_MODE 1

struct hisi_fb_vactive {
	wait_queue_head_t vactive0_start_wq;
	uint32_t vactive0_start_flag;
	uint32_t vactive0_end_flag;
};

int hisi_vactive0_wait_event(uint32_t condition_type, uint32_t *time_diff);
void hisi_vactive0_init(void);
void hisi_vactive0_set_event(uint8_t val);
void hisi_vactive0_event_increase(void);
struct notifier_block* get_vactive0_isr_notifier(void);
#endif /* HISI_DISP_VACTIVE_H */
