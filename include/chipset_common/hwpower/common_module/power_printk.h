/* SPDX-License-Identifier: GPL-2.0 */
/*
 * power_printk.h
 *
 * print output for power module
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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

#ifndef _POWER_PRINTK_H_
#define _POWER_PRINTK_H_

#include <linux/types.h>
#include <huawei_platform/log/hw_log.h>

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif

#define _pm_fmt_tag(TAG, LEVEL) "[" #LEVEL "/" #TAG "]"
#define pm_fmt_tag(TAG, LEVEL) _pm_fmt_tag(TAG, LEVEL)

#define pm_klog_info(fmt, x...) pr_info(pm_fmt_tag(HWLOG_TAG, I)"[pmt_key] " fmt, ##x)
#define pm_klog_warn(fmt, x...) pr_info(pm_fmt_tag(HWLOG_TAG, W)"[pmt_key] " fmt, ##x)
#define pm_klog_err(fmt, x...) pr_info(pm_fmt_tag(HWLOG_TAG, E)"[pmt_key] " fmt, ##x)

void power_print_u8_array(const char *name, const u8 *array, unsigned int row, unsigned int col);

#endif /* _POWER_PRINTK_H_ */
