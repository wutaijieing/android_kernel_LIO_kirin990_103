/* SPDX-License-Identifier: GPL-2.0 */
/*
 * power_time.h
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

#ifndef _POWER_TIME_H_
#define _POWER_TIME_H_

#include <linux/rtc.h>
#include <linux/time64.h>
#include <linux/timekeeping.h>
#include <linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#define TIME_T_MAX	(time64_t)((1UL << ((sizeof(time64_t) << 3) - 1)) - 1)

struct timespec64 power_get_current_kernel_time(void);
void power_get_timeofday(struct timespec64 *tv);
void power_get_local_time(struct timespec64 *time, struct rtc_time *tm);
#else
struct timespec power_get_current_kernel_time(void);
void power_get_timeofday(struct timeval *tv);
void power_get_local_time(struct timeval *time, struct rtc_time *tm);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0) */

struct timespec64 power_get_current_kernel_time64(void);
time64_t power_get_monotonic_boottime(void);
bool power_is_within_time_interval(u8 start_hour, u8 start_min, u8 end_hour, u8 end_min);

#endif /* _POWER_TIME_H_ */
