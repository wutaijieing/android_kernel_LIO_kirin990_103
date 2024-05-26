/*
 * trace_utils.c
 *
 * trace utils
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

#include "trace_utils.h"
#include <linux/version.h>
#include <huawei_platform/log/hw_log.h>

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG trace_utils
HWLOG_REGIST();

static bool g_real_log = 1;

void trace_get_time(struct tm *ptm)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
	struct timespec64 tv;
#else
	struct timeval tv;
#endif

	memset(&tv, 0, sizeof(tv));
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
	ktime_get_real_ts64(&tv);
	time64_to_tm(tv.tv_sec, 0, ptm);
#else
	do_gettimeofday(&tv);
	time_to_tm(tv.tv_sec, 0, ptm);
#endif
}

void trace_real_log_set(bool real)
{
	g_real_log = real;
}

bool trace_real_log(void)
{
	return g_real_log;
}