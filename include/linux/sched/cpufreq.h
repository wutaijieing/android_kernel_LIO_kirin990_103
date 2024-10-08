/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_SCHED_CPUFREQ_H
#define _LINUX_SCHED_CPUFREQ_H

#include <linux/types.h>

/*
 * Interface between cpufreq drivers and the scheduler:
 */

#define SCHED_CPUFREQ_IOWAIT	(1U << 0)
#define SCHED_CPUFREQ_WALT	(1U << 1)

#ifdef CONFIG_CPU_FREQ
struct cpufreq_policy;

struct update_util_data {
       void (*func)(struct update_util_data *data, u64 time, unsigned int flags);
};

void cpufreq_add_update_util_hook(int cpu, struct update_util_data *data,
                       void (*func)(struct update_util_data *data, u64 time,
				    unsigned int flags));
void cpufreq_remove_update_util_hook(int cpu);
bool cpufreq_this_cpu_can_update(struct cpufreq_policy *policy);

static inline unsigned long map_util_freq(unsigned long util,
					unsigned long freq, unsigned long cap)
{
	return (freq + (freq >> 2)) * util / cap;
}
#endif /* CONFIG_CPU_FREQ */

#define SUGOV_START	0
#define SUGOV_STOP	1
#define SUGOV_ACTIVE	2

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
int sugov_register_notifier(struct notifier_block *nb);
int sugov_unregister_notifier(struct notifier_block *nb);
#else
static inline int sugov_register_notifier(struct notifier_block *nb)
{
	return 0;
}
static inline int sugov_unregister_notifier(struct notifier_block *nb)
{
	return 0;
}
#endif /* CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT */

#endif /* _LINUX_SCHED_CPUFREQ_H */
