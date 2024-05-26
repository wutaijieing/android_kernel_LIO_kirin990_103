/*
 * lowpm_log.h
 *
 * log func for lowpm
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
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

#ifndef __HEAD_LOWPM_LOG__
#define __HEAD_LOWPM_LOG__

#include <linux/printk.h>
#include <linux/seq_file.h>

#define NO_SEQFILE		0
#define lp_msg(seq_file, fmt, args ...) \
	do { \
		if (seq_file == NO_SEQFILE) \
			pr_info(fmt"\n", ##args); \
		else \
			seq_printf(seq_file, fmt"\n", ##args); \
	} while (0)
#define lp_msg_cont(seq_file, fmt, args ...) \
	do { \
		if (seq_file == NO_SEQFILE) \
			pr_cont(fmt, ##args); \
		else \
			seq_printf(seq_file, fmt, ##args); \
	} while (0)

#define lowpm_fmt(fmt) "lowpm: %s " fmt "\n"

#define lp_debug(fmt, ...) pr_debug(lowpm_fmt(fmt), __func__, ##__VA_ARGS__)
#define lp_info(fmt, ...) pr_info(lowpm_fmt(fmt), __func__, ##__VA_ARGS__)
#define lp_notice(fmt, ...) pr_notice(lowpm_fmt(fmt), __func__, ##__VA_ARGS__)
#define lp_err(fmt, ...) pr_err(lowpm_fmt(fmt), __func__, ##__VA_ARGS__)
#define lp_crit(fmt, ...) pr_crit("%s " fmt "\n", __func__, ##__VA_ARGS__)

#endif /* __HEAD_LOWPM_LOG__ */
