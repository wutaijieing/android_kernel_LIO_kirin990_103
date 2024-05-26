/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * phase_perf.h
 *
 * Copyright (c) 2021 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PHASE_PERF_H
#define __PHASE_PERF_H

#include <linux/types.h>
#include <linux/cpumask.h>
#include <asm/phase_perf.h>

#ifdef CONFIG_PHASE_PERF
extern unsigned int sysctl_phase_exclude_hv;
extern unsigned int sysctl_phase_exclude_idle;
extern unsigned int sysctl_phase_exclude_kernel;

int phase_perf_create(int *pevents, int *fevents, const struct cpumask *cpus);
void phase_perf_release(const struct cpumask *cpus);
void phase_perf_read_events(int cpu, u64 *pdata, u64 *fdata);

int *phase_perf_pinned_events(void);
int *phase_perf_flexible_events(void);

#else

static inline int phase_perf_create(int *pevents, int *fevents, const struct cpumask *cpus)
{
	return 0;
}

static inline void phase_perf_release(const struct cpumask *cpus)
{
}

static inline void phase_perf_read_events(int cpu, u64 *pdata, u64 *fdata)
{
}

static inline int *phase_perf_pinned_events(void)
{
	return NULL;
}

static inline int *phase_perf_flexible_events(void)
{
	return NULL;
}
#endif

#endif
