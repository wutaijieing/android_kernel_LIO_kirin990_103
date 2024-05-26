// SPDX-License-Identifier: GPL-2.0-only
/*
 * plat_cpumask.c
 *
 * platform cpumask interfaces
 *
 * Copyright (c) 2015-2021 Huawei Technologies Co., Ltd.
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

#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/init.h>
#include <linux/node.h>
#include <linux/nodemask.h>
#include <linux/of.h>
#include <linux/smp.h>
#include <linux/string.h>
#include <asm/smp_plat.h>
#include <asm/sysreg.h>
#include <asm/cpufeature.h>


static const char * const little_cores[] = {
	"arm,cortex-a53",
	NULL,
};

static bool is_little_cpu(struct device_node *cn)
{
	const char * const *lc;

	for (lc = little_cores; *lc != NULL; lc++)
		if (of_device_is_compatible(cn, *lc))
			return true;

	return false;
}

void __init arch_get_fast_and_slow_cpus(struct cpumask *fast,
					struct cpumask *slow)
{
	struct device_node *cn = NULL;
	int cpu;

	cpumask_clear(fast);
	cpumask_clear(slow);

	/*
	 * Else, parse device tree for little cores.
	 */
	while ((cn = of_find_node_by_type(cn, "cpu"))) {
		const u32 *mpidr;
		int len;

		mpidr = of_get_property(cn, "reg", &len);
		if (!mpidr || len != 8) {
			pr_err("%s missing reg property\n", cn->full_name);
			continue;
		}

		cpu = get_logical_index(be32_to_cpup(mpidr+1));
		if (cpu == -EINVAL) {
			pr_err("couldn't get logical index for mpidr %x\n",
							be32_to_cpup(mpidr+1));
			break;
		}

		if (is_little_cpu(cn))
			cpumask_set_cpu(cpu, slow);
		else
			cpumask_set_cpu(cpu, fast);
	}

	if (!cpumask_empty(fast) && !cpumask_empty(slow))
		return;

	/*
	 * We didn't find both big and little cores so let's call all cores
	 * fast as this will keep the system running, with all cores being
	 * treated equal.
	 */
	cpumask_setall(fast);
	cpumask_clear(slow);
}

struct cpumask slow_cpu_mask;
struct cpumask fast_cpu_mask;

void get_fast_cpus(struct cpumask *cpumask)
{
	if (cpumask)
		cpumask_copy(cpumask, &fast_cpu_mask);
}
EXPORT_SYMBOL(get_fast_cpus);

void get_slow_cpus(struct cpumask *cpumask)
{
	if (cpumask)
		cpumask_copy(cpumask, &slow_cpu_mask);
}
EXPORT_SYMBOL(get_slow_cpus);

int test_fast_cpu(int cpu)
{
	return cpumask_test_cpu(cpu, &fast_cpu_mask);
}
EXPORT_SYMBOL(test_fast_cpu);

int test_slow_cpu(int cpu)
{
	return cpumask_test_cpu(cpu, &slow_cpu_mask);
}
EXPORT_SYMBOL(test_slow_cpu);

#ifdef CONFIG_32BIT_COMPAT
static struct cpumask compat_cpu_mask;

void get_compat_cpus(struct cpumask *cpumask)
{
	if (cpumask)
		cpumask_copy(cpumask, &compat_cpu_mask);
}

int test_compat_cpu(int cpu)
{
	return cpumask_test_cpu(cpu, &compat_cpu_mask);
}

static bool is_current_compat_cpu(void)
{
	u64 reg_id_aa64pfr0 = read_sysreg_s(SYS_ID_AA64PFR0_EL1);

	return id_aa64pfr0_32bit_el0(reg_id_aa64pfr0);
}

void update_compat_masks(int cpu)
{
	if (is_current_compat_cpu())
		cpumask_set_cpu(cpu, &compat_cpu_mask);
}

int get_online_compat_cpu_num(void)
{
	struct cpumask online_compat_cpus_mask;

	cpumask_and(&online_compat_cpus_mask, cpu_online_mask, &compat_cpu_mask);
	return cpumask_weight(&online_compat_cpus_mask);
}

const struct cpumask *task_cpu_possible_mask(struct task_struct *p)
{
	if (!test_tsk_thread_flag(p, TIF_32BIT))
		return cpu_possible_mask;

	return &compat_cpu_mask;
}
#endif

static int __init plat_init_cpumasks(void)
{
	arch_get_fast_and_slow_cpus(&fast_cpu_mask, &slow_cpu_mask);
	return 0;
}
core_initcall(plat_init_cpumasks);
