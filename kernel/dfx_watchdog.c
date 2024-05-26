/*
 * huawei_watchdog.c
 *
 * implement the ioctl for user space to get memory usage information,
 * and also provider control command
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

#include <linux/workqueue.h>
#include <securec.h>
#include <chipset_common/hwzrhung/zrhung.h>

#define HIVIEW_REPORT_THRESHOLD 3
#define HIVIEW_SOFTLOCKUP_MSG_LEN 100
#define BBOX_HUNGINFO "zrhung send event lockup"

static struct work_struct  hiview_report_work;
static char hiview_msg[HIVIEW_SOFTLOCKUP_MSG_LEN];
static DEFINE_PER_CPU(int, hiview_hang_cnt);
static DEFINE_PER_CPU(bool, hiview_hang_stat);

static void dfx_watchdog_report_work_func(struct work_struct *work)
{
	int ret;

	ret = zrhung_send_event(ZRHUNG_WP_SOFT_LOCKUP, "", hiview_msg);
	if (ret <= 0)
		pr_err("hiview: softlockup msg upload fail\n");
	else
		pr_info("hiview: softlockup msg upload success\n");
}

void dfx_watchdog_check_hung(int duration)
{
	int cnt;
	int ret;

	if (__this_cpu_read(hiview_hang_stat))
		return;

	cnt = __this_cpu_read(hiview_hang_cnt) + 1;
	__this_cpu_write(hiview_hang_cnt, cnt);
	ret = snprintf_s(hiview_msg, HIVIEW_SOFTLOCKUP_MSG_LEN,
					 HIVIEW_SOFTLOCKUP_MSG_LEN - 1,
					 "softlockup: CPU#%d stuck for %d times![%s:%d]\n",
					 smp_processor_id(), cnt, current->comm,
					 task_pid_nr(current));
	if (ret <= 0) {
		pr_err("hiview: softlock msg format fail!");
	} else {
		pr_info("hiview: msg = %s\n", hiview_msg);
		if (cnt >= HIVIEW_REPORT_THRESHOLD) {
			schedule_work_on(0, &hiview_report_work);
			__this_cpu_write(hiview_hang_stat, true);
		}
	}
}

int dfx_watchdog_lockup_init(void)
{
	__this_cpu_write(hiview_hang_cnt, 0);
	__this_cpu_write(hiview_hang_stat, false);
	return 0;
}

void dfx_watchdog_lockup_init_work(void)
{
	INIT_WORK(&hiview_report_work, dfx_watchdog_report_work_func);
	__this_cpu_write(hiview_hang_cnt, 0);
	__this_cpu_write(hiview_hang_stat, false);
}
