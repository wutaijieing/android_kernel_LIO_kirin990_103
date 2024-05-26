// SPDX-License-Identifier: GPL-2.0-only
/*
 *  linux/drivers/cpufreq/cpufreq_performance.c
 *
 *  Copyright (C) 2002 - 2003 Dominik Brodowski <linux@brodo.de>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/cpufreq.h>
#include <linux/init.h>
#include <linux/module.h>
#ifdef CONFIG_ARCH_PLATFORM
#include <platform_include/basicplatform/linux/dfx_cmdline_parse.h>
#include <linux/topology.h>
#endif

static void cpufreq_gov_performance_limits(struct cpufreq_policy *policy)
{
#ifdef CONFIG_ARCH_PLATFORM
	unsigned int utarget;

	if ((get_low_battery_flag() == 1) &&
	    topology_physical_package_id(policy->cpu) > 0)
		utarget = policy->min;
	else
		utarget = policy->max;

	pr_debug("%s utarget=%d\n", __func__, utarget);

	__cpufreq_driver_target(policy, utarget, CPUFREQ_RELATION_H);
#else
	pr_debug("setting to %u kHz\n", policy->max);
	__cpufreq_driver_target(policy, policy->max, CPUFREQ_RELATION_H);
#endif
}

#ifdef CONFIG_ARCH_PLATFORM
static void cpufreq_gov_performance_plat_exit(struct cpufreq_policy *policy)
{
	set_low_battery_flag(0);
}
#endif

static struct cpufreq_governor cpufreq_gov_performance = {
	.name		= "performance",
	.owner		= THIS_MODULE,
	.flags		= CPUFREQ_GOV_STRICT_TARGET,
	.limits		= cpufreq_gov_performance_limits,
#ifdef CONFIG_ARCH_PLATFORM
	.exit		= cpufreq_gov_performance_plat_exit,
#endif
};

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_PERFORMANCE
struct cpufreq_governor *cpufreq_default_governor(void)
{
	return &cpufreq_gov_performance;
}
#endif
#ifndef CONFIG_CPU_FREQ_GOV_PERFORMANCE_MODULE
struct cpufreq_governor *cpufreq_fallback_governor(void)
{
	return &cpufreq_gov_performance;
}
#endif

MODULE_AUTHOR("Dominik Brodowski <linux@brodo.de>");
MODULE_DESCRIPTION("CPUfreq policy governor 'performance'");
MODULE_LICENSE("GPL");

cpufreq_governor_init(cpufreq_gov_performance);
cpufreq_governor_exit(cpufreq_gov_performance);
