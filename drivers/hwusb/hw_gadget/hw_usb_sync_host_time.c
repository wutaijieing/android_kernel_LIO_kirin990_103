/*
 * hw_usb_sync_host_time.c
 *
 * driver file for usb sync host time
 *
 * Copyright (c) 2019-2021 Huawei Technologies Co., Ltd.
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

#include <chipset_common/hwusb/hw_usb_sync_host_time.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/time.h>
#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/utsname.h>
#include <linux/version.h>
#include <chipset_common/hwpower/common_module/power_cmdline.h>

struct _time {
	int time_bias;
	unsigned short year;
	unsigned short month;
	unsigned short day;
	unsigned short hour;
	unsigned short minute;
	unsigned short second;
	unsigned short millisecond;
	unsigned short weekday;
};

struct hw_flush_pc_time {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct timespec64 tv;
#else
	struct timespec tv;
#endif
	struct delayed_work pc_data_work;
};

static struct hw_flush_pc_time flush_pc_data;

static void hw_usb_flush_pc_time_work(struct work_struct *work)
{
#if defined(CONFIG_RTC_HCTOSYS_DEVICE)
	int rv;
	struct rtc_time new_rtc_tm;
	struct rtc_device *rtc = NULL;
	struct hw_flush_pc_time *pc_data = container_of(work,
		struct hw_flush_pc_time, pc_data_work.work);

	if (!pc_data) {
		pr_info("set RTC time Fail\n");
		return;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	if (do_settimeofday64(&pc_data->tv) < 0)
#else
	if (do_settimeofday(&pc_data->tv) < 0)
#endif
		pr_info("set system time Fail\n");

	rtc = rtc_class_open(CONFIG_RTC_HCTOSYS_DEVICE);
	if (!rtc) {
		pr_info("%s: unable to open rtc device %s\n",
			__FILE__, CONFIG_RTC_HCTOSYS_DEVICE);
		return;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	rtc_time64_to_tm(pc_data->tv.tv_sec, &new_rtc_tm);
#else
	rtc_time_to_tm(pc_data->tv.tv_sec, &new_rtc_tm);
#endif
	rv = rtc_set_time(rtc, &new_rtc_tm);
	if (rv != 0)
		pr_info("set RTC time Fail\n");

	pr_info("set system time ok\n");
#else
	return;
#endif /* CONFIG_RTC_HCTOSYS_DEVICE */
}

void hw_usb_handle_host_time(struct usb_ep *ep, struct usb_request *req)
{
	struct _time *host_time = NULL;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct timespec64 tv_now;
#else
	struct timeval tv_now;
#endif
	struct rtc_time current_gmt_tm;

	if (!req)
		return;

	if (!power_cmdline_is_factory_mode())
		return;

	host_time = (struct _time *)req->buf;

	pr_info("Time Bias:%d minutes\n", host_time->time_bias);
	pr_info("Host Time:[%d:%d:%d %d:%d:%d:%d Weekday:%d]\n",
		host_time->year, host_time->month, host_time->day,
		host_time->hour, host_time->minute, host_time->second,
		host_time->millisecond, host_time->weekday);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	ktime_get_real_ts64(&tv_now);
	rtc_time64_to_tm(tv_now.tv_sec, &current_gmt_tm);
#else
	do_gettimeofday(&tv_now);
	rtc_time_to_tm(tv_now.tv_sec, &current_gmt_tm);
#endif
	pr_info("Device Current Time:[%d:%d:%d %d:%d:%d]\n",
		current_gmt_tm.tm_year, current_gmt_tm.tm_mon, current_gmt_tm.tm_mday,
		current_gmt_tm.tm_hour, current_gmt_tm.tm_min, current_gmt_tm.tm_sec);
	if ((current_gmt_tm.tm_mon != 0) || (current_gmt_tm.tm_mday != 1))
		return;

	flush_pc_data.tv.tv_nsec = NSEC_PER_SEC >> 1;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	flush_pc_data.tv.tv_sec = (unsigned long)mktime64(host_time->year,
			host_time->month,
			host_time->day,
			host_time->hour,
			host_time->minute,
			host_time->second);
#else
	flush_pc_data.tv.tv_sec = (unsigned long)mktime(host_time->year,
			host_time->month,
			host_time->day,
			host_time->hour,
			host_time->minute,
			host_time->second);
#endif

	schedule_delayed_work(&(flush_pc_data.pc_data_work), 0);
}
EXPORT_SYMBOL(hw_usb_handle_host_time);

void hw_usb_sync_host_time_init(void)
{
	pr_info("Enter fuction %s\n", __func__);
	memset(&flush_pc_data, 0, sizeof(struct hw_flush_pc_time));
	INIT_DELAYED_WORK(&(flush_pc_data.pc_data_work),
		hw_usb_flush_pc_time_work);
}
EXPORT_SYMBOL(hw_usb_sync_host_time_init);
