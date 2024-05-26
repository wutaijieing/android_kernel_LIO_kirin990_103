// SPDX-License-Identifier: GPL-2.0-only
/*
 * time_in_state.h
 *
 * header file for time_in_state
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
 */

#ifndef __TIME_IN_STATE_H__
#define __TIME_IN_STATE_H__

#include <linux/cpumask.h>

#ifdef CONFIG_FREQ_STATS_COUNTING_IDLE
void time_in_state_update_freq(struct cpumask *cpus,
			       unsigned int new_freq_index);
void time_in_state_update_idle(int cpu, unsigned int new_idle_index);
int lpcpu_time_in_freq_get(int cpu, u64 *freqs, int freqs_len);

#else /* !CONFIG_FREQ_STATS_COUNTING_IDLE */

static inline void time_in_state_update_freq(struct cpumask *cpus,
					     unsigned int new_freq_index)
{
}

static inline void time_in_state_update_idle(int cpu, unsigned int new_idle_index)
{
}

static inline int lpcpu_time_in_freq_get(int cpu, u64 *freqs, int freqs_len)
{
	return 0;
}
#endif /* CONFIG_FREQ_STATS_COUNTING_IDLE */

#endif /* __TIME_IN_STATE_H__ */
