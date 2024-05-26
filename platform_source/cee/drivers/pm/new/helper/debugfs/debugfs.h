/*
 * debugfs.h
 *
 * debug information of suspend
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

#ifndef __HEAD_LOWPM_DEBUGFS__
#define __HEAD_LOWPM_DEBUGFS__

#include <linux/seq_file.h>
#include <linux/compiler_types.h>

struct lowpm_debugfile {
	const char *name;
	umode_t mode;
	int (*show)(struct seq_file *seq, void *data);
	ssize_t (*store)(struct seq_file *seq, const char __user *buf,
			size_t count, loff_t *ppos);
};

struct lowpm_debugdir {
	const char *dir;
	const struct lowpm_debugfile files[];
};

#ifdef CONFIG_DFX_DEBUG_FS
int lowpm_create_debugfs(const struct lowpm_debugdir *fs);
#else
static inline int lowpm_create_debugfs(const struct lowpm_debugdir *fs) { return 0; }
#endif

#endif /* __HEAD_LOWPM_DEBUGFS__ */
