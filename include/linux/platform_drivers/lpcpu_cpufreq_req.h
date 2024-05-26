/*
 * lpcpu_cpufreq_req.h
 *
 * lpcpu cpufreq req head file
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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

#ifndef _LPCPU_CPUFREQ_REQ_H
#define _LPCPU_CPUFREQ_REQ_H

#include <linux/notifier.h>

struct cpufreq_req {
	struct notifier_block nb;
	int cpu;
	unsigned int freq;
};

#ifdef CONFIG_LPCPU_CPUFREQ_DT
extern int lpcpu_cpufreq_init_req(struct cpufreq_req *req, int cpu);
extern void lpcpu_cpufreq_update_req(struct cpufreq_req *req,
				     unsigned int freq);
extern void lpcpu_cpufreq_exit_req(struct cpufreq_req *req);
#else
static inline int lpcpu_cpufreq_init_req(struct cpufreq_req *req, int cpu)
{
	return 0;
}

static inline void lpcpu_cpufreq_update_req(struct cpufreq_req *req,
					    unsigned int freq)
{
}

static inline void lpcpu_cpufreq_exit_req(struct cpufreq_req *req)
{
}
#endif

#endif /* _LPCPU_CPUFREQ_REQ_H */
