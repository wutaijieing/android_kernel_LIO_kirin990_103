/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/types.h>
#include <linux/time64.h>
#include <linux/errno.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <linux/timekeeping.h>
#include "hisi_disp_vactive.h"
#include "hisi_disp_debug.h"

static struct hisi_fb_vactive g_hisi_vactive_flag;

void hisifb_get_timestamp(struct timespec64 *ts)
{
	ktime_get_ts64(ts);
}

uint32_t hisifb_timestamp_diff(struct timespec64 *lasttime, struct timespec64 *curtime)
{
	uint32_t ret;

	ret = (curtime->tv_nsec >= lasttime->tv_nsec) ?
		((curtime->tv_nsec - lasttime->tv_nsec) / 1000) :
		1000000 - ((lasttime->tv_nsec - curtime->tv_nsec) / 1000);  /* 1s */

	return ret;
}

void hisi_vactive0_set_event(uint8_t val)
{
	g_hisi_vactive_flag.vactive0_start_flag = val;
}

void hisi_vactive0_event_increase(void)
{
	g_hisi_vactive_flag.vactive0_start_flag++;
	wake_up_interruptible_all(&(g_hisi_vactive_flag.vactive0_start_wq));
}

int hisi_vactive0_wait_event(uint32_t condition_type, uint32_t *time_diff)
{
	int ret;
	struct timespec64 tv0;
	struct timespec64 tv1;
	int times = 0;
	//uint32_t timeout_interval =
	//	(g_fpga_flag == 0) ? DSS_COMPOSER_TIMEOUT_THRESHOLD_ASIC : DSS_COMPOSER_TIMEOUT_THRESHOLD_FPGA;
	uint32_t timeout_interval = DSS_COMPOSER_TIMEOUT_THRESHOLD_FPGA;
	uint32_t prev_vactive0_start = g_hisi_vactive_flag.vactive0_start_flag;

	hisifb_get_timestamp(&tv0);
	while (times < 50) {  /* wait times */
		if (condition_type)
			ret = wait_event_interruptible_timeout(g_hisi_vactive_flag.vactive0_start_wq, g_hisi_vactive_flag.vactive0_start_flag,
				msecs_to_jiffies(timeout_interval));
		else
			ret = wait_event_interruptible_timeout(g_hisi_vactive_flag.vactive0_start_wq,
				((prev_vactive0_start == 0) || (prev_vactive0_start != g_hisi_vactive_flag.vactive0_start_flag)),
				msecs_to_jiffies(timeout_interval));
		disp_pr_debug(" ret:%d", ret);
		if (ret != -ERESTARTSYS)
			break;
		
		times++;
		disp_pr_debug(" times:%d", times);
		mdelay(10);  /* delay 10ms */
	}

	hisifb_get_timestamp(&tv1);
	*time_diff = hisifb_timestamp_diff(&tv0, &tv1);
	disp_pr_debug(" ---- time_diff:%u, ret:%d", *time_diff, ret);
	return ret;
}

void hisi_vactive0_init()
{
	init_waitqueue_head(&g_hisi_vactive_flag.vactive0_start_wq);
	g_hisi_vactive_flag.vactive0_start_flag = 0;
}

static int hisi_vactive0_isr_notify(struct notifier_block *self, unsigned long action, void *data)
{
	hisi_vactive0_set_event(1);
#ifdef CONFIG_DKMD_DPU_V720
	wake_up_interruptible_all(&(g_hisi_vactive_flag.vactive0_start_wq));
#endif
	return 0;
}

static struct notifier_block vactive0_isr_notifier = {
	.notifier_call = hisi_vactive0_isr_notify,
};

struct notifier_block* get_vactive0_isr_notifier()
{
	return &vactive0_isr_notifier;
}
