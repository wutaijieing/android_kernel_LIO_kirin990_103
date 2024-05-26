/*
 * hisi_cpufreq_req.h
 *
 * hisi cpufreq req head file
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

#ifndef _HISI_CPUFREQ_REQ_H
#define _HISI_CPUFREQ_REQ_H

#ifdef CONFIG_ARCH_PLATFORM
#include <linux/platform_drivers/lpcpu_cpufreq_req.h>
#else
#include <linux/notifier.h>

struct cpufreq_req {
	struct notifier_block nb;
	int cpu;
	unsigned int freq;
};
#endif /* CONFIG_ARCH_PLATFORM */

#if !defined(CONFIG_ARCH_PLATFORM) || !defined(CONFIG_AB_APCP_NEW_INTERFACE)
/* Only supported on old plat */
#ifdef CONFIG_LPCPU_CPUFREQ_DT
extern int hisi_cpufreq_init_req(struct cpufreq_req *req, int cpu);
extern void hisi_cpufreq_update_req(struct cpufreq_req *req,
				     unsigned int freq);
extern void hisi_cpufreq_exit_req(struct cpufreq_req *req);
#else
static inline int hisi_cpufreq_init_req(struct cpufreq_req *req, int cpu)
{
	return 0;
}

static inline void hisi_cpufreq_update_req(struct cpufreq_req *req,
					    unsigned int freq)
{
}

static inline void hisi_cpufreq_exit_req(struct cpufreq_req *req)
{
}
#endif /* CONFIG_LPCPU_CPUFREQ_DT */
#endif /* for old plat */

#endif /* _HISI_CPUFREQ_REQ_H */
