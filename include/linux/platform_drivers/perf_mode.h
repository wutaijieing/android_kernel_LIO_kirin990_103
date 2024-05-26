/*
 * perf_mode.h
 *
 * perf mode header file
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

#ifndef _PERF_MODE_H
#define _PERF_MODE_H

enum {
	PERF_MODE_0 = 0,
	PERF_MODE_1,
	PERF_MODE_2,
	MAX_PERF_MODE,
};

#ifdef CONFIG_PERF_MODE
int perf_mode_register_notifier(struct notifier_block *nb);
int perf_mode_unregister_notifier(struct notifier_block *nb);
#else
static inline
int perf_mode_register_notifier(struct notifier_block *nb)
{
	return -EINVAL;
}
static inline
int perf_mode_unregister_notifier(struct notifier_block *nb)
{
	return -EINVAL;
}
#endif
#endif /* _PERF_MODE_H */
