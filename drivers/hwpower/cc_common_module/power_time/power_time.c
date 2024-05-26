// SPDX-License-Identifier: GPL-2.0
/*
 * power_time.c
 *
 * time for power module
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
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

#include <linux/rtc.h>
#include <linux/time64.h>
#include <linux/timekeeping.h>
#include <linux/version.h>
#include <chipset_common/hwpower/common_module/power_common_macro.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG power_time
HWLOG_REGIST();

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
struct timespec64 power_get_current_kernel_time(void)
{
	struct timespec64 ts64 = { 0 };

	ktime_get_coarse_real_ts64(&ts64);
	return ts64;
}

struct timespec64 power_get_current_kernel_time64(void)
{
	struct timespec64 ts64 = { 0 };

	ktime_get_coarse_real_ts64(&ts64);
	return ts64;
}

void power_get_timeofday(struct timespec64 *tv)
{
	struct timespec64 now = { 0 };

	if (!tv)
		return;

	ktime_get_real_ts64(&now);
	tv->tv_sec = now.tv_sec;
	tv->tv_nsec = now.tv_nsec;
}

time64_t power_get_monotonic_boottime(void)
{
	struct timespec64 ts64 = { 0 };

	ktime_get_boottime_ts64(&ts64);
	hwlog_info("tv_sec:%ld\n", (long)ts64.tv_sec);
	return ts64.tv_sec;
}

#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 18, 0))
struct timespec power_get_current_kernel_time(void)
{
	struct timespec64 ts64 = { 0 };

	ktime_get_coarse_real_ts64(&ts64);
	return timespec64_to_timespec(ts64);
}

struct timespec64 power_get_current_kernel_time64(void)
{
	struct timespec64 ts64 = { 0 };

	ktime_get_coarse_real_ts64(&ts64);
	return ts64;
}

void power_get_timeofday(struct timeval *tv)
{
	struct timespec64 now = { 0 };

	if (!tv)
		return;

	ktime_get_real_ts64(&now);
	tv->tv_sec = now.tv_sec;
	tv->tv_usec = now.tv_nsec / 1000;
}

time64_t power_get_monotonic_boottime(void)
{
	struct timespec64 ts64 = { 0 };

	ktime_get_boottime_ts64(&ts64);
	hwlog_info("tv_sec:%ld\n", (long)ts64.tv_sec);
	return ts64.tv_sec;
}

#else
struct timespec power_get_current_kernel_time(void)
{
	struct timespec64 now = current_kernel_time64();

	return timespec64_to_timespec(now);
}

struct timespec64 power_get_current_kernel_time64(void)
{
	return current_kernel_time64();
}

void power_get_timeofday(struct timeval *tv)
{
	if (!tv)
		return;

	do_gettimeofday(tv);
}

time64_t power_get_monotonic_boottime(void)
{
	struct timespec64 ts64 = { 0 };

	get_monotonic_boottime64(&ts64);
	hwlog_info("tv_sec:%ld\n", (long)ts64.tv_sec);
	return ts64.tv_sec;
}
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(4, 18, 0) */
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
void power_get_local_time(struct timespec64 *time, struct rtc_time *tm)
{
	if (!time || !tm)
		return;

	time->tv_sec -= sys_tz.tz_minuteswest * POWER_SEC_PER_MIN;
	rtc_time64_to_tm(time->tv_sec, tm);
}
#else
void power_get_local_time(struct timeval *time, struct rtc_time *tm)
{
	if (!time || !tm)
		return;

	time->tv_sec -= sys_tz.tz_minuteswest * POWER_SEC_PER_MIN;
	rtc_time_to_tm(time->tv_sec, tm);
}
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) */

bool power_is_within_time_interval(u8 start_hour, u8 start_min, u8 end_hour, u8 end_min)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
	struct timespec64 tv = { 0 };
#else
	struct timeval tv = { 0 };
#endif
	struct rtc_time tm = { 0 };
	unsigned int current_time;
	unsigned int start_time;
	unsigned int end_time;

	power_get_timeofday(&tv); /* seconds since 1970-01-01 00:00:00 */
	power_get_local_time(&tv, &tm);

	current_time = tm.tm_hour * POWER_MIN_PER_HOUR + tm.tm_min;
	start_time = start_hour * POWER_MIN_PER_HOUR + start_min;
	end_time = end_hour * POWER_MIN_PER_HOUR + end_min;

	/* example: 2:30 - 5:30 */
	if ((start_time <= end_time) &&
		(current_time >= start_time) && (current_time <= end_time))
		return true;

	/* example: 21:30 - 5:30 */
	if ((start_time > end_time) &&
		((current_time >= start_time) || (current_time <= end_time)))
		return true;

	return false;
}
