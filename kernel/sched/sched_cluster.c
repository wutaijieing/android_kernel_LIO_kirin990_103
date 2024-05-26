// SPDX-License-Identifier: GPL-2.0
/*
 * sched_cluster.c
 *
 * sched topology
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

#include <linux/version.h>
#include <linux/cpufreq.h>
#include "sched.h"

struct list_head cluster_head;
int num_clusters;

struct sched_cluster init_cluster = {
	.list = LIST_HEAD_INIT(init_cluster.list),
	.id = 0,
	.max_possible_capacity = SCHED_CAPACITY_SCALE,
	.cur_freq = 1,
	.max_freq = 1,
	.min_freq = 1,
	.capacity_margin = 1280,
	.sd_capacity_margin = 1280,
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
static int cpufreq_update_min(struct notifier_block *nb, unsigned long freq,
			      void *data);
static int cpufreq_update_max(struct notifier_block *nb, unsigned long freq,
			      void *data);
#endif

static void move_list(struct list_head *dst, struct list_head *src,
		      bool sync_rcu)
{
	struct list_head *first = src->next;
	struct list_head *last = src->prev;

	if (sync_rcu) {
		INIT_LIST_HEAD_RCU(src);
		synchronize_rcu();
	}

	first->prev = dst;
	dst->prev = last;
	last->next = dst;

	/* Ensure list sanity before making the head visible to all CPUs. */
	smp_mb();
	dst->next = first;
}

static void insert_cluster(struct sched_cluster *cluster,
			   struct list_head *head)
{
	struct sched_cluster *tmp = NULL;
	struct list_head *iter = head;

	list_for_each_entry(tmp, head, list) {
		if (cluster->id < tmp->id)
			break;
		iter = &tmp->list;
	}

	list_add(&cluster->list, iter);
}

static struct sched_cluster *alloc_new_cluster(const struct cpumask *cpus)
{
	struct sched_cluster *cluster =
		kzalloc(sizeof(struct sched_cluster), GFP_ATOMIC);

	if (!cluster) {
		pr_warn("Cluster allocation failed. Possible bad scheduling\n");
		return NULL;
	}

	INIT_LIST_HEAD(&cluster->list);
	cluster->id = topology_physical_package_id(cpumask_first(cpus));
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	cluster->max_possible_capacity = arch_scale_cpu_capacity(cpumask_first(cpus));
	cluster->nb_min.notifier_call = cpufreq_update_min;
	cluster->nb_max.notifier_call = cpufreq_update_max;
#else
	cluster->max_possible_capacity = arch_scale_cpu_capacity(NULL, cpumask_first(cpus));
#endif
	cluster->cur_freq = 1;
	cluster->max_freq = 1;
	cluster->min_freq = 1;
	cluster->freq_init_done = false;
	cluster->capacity_margin = capacity_margin;
	cluster->sd_capacity_margin = sd_capacity_margin;
	cluster->cpus = *cpus;

	return cluster;
}

static void add_cluster(const struct cpumask *cpus, struct list_head *head)
{
	struct sched_cluster *cluster = alloc_new_cluster(cpus);
	int i;

	if (!cluster)
		return;

	for_each_cpu(i, cpus)
		cpu_rq(i)->cluster = cluster;

	insert_cluster(cluster, head);
	num_clusters++;
}

void update_cluster_topology(void)
{
	struct cpumask cpus = *cpu_possible_mask;
	const struct cpumask *cluster_cpus = NULL;
	struct list_head new_head;
	int i;

	INIT_LIST_HEAD(&new_head);

	for_each_cpu(i, &cpus) {
		cluster_cpus = cpu_coregroup_mask(i);
		cpumask_andnot(&cpus, &cpus, cluster_cpus);
		add_cluster(cluster_cpus, &new_head);
	}

	/*
	 * Ensure cluster ids are visible to all CPUs before making
	 * cluster_head visible.
	 */
	move_list(&cluster_head, &new_head, false);
}

void init_clusters(void)
{
	init_cluster.cpus = *cpu_possible_mask;
	INIT_LIST_HEAD(&cluster_head);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
static int cpufreq_update_min(struct notifier_block *nb, unsigned long freq,
			      void *data)
{
	struct sched_cluster *cluster = container_of(nb, struct sched_cluster, nb_min);

	cluster->min_freq = freq;
	return 0;
}

static int cpufreq_update_max(struct notifier_block *nb, unsigned long freq,
			      void *data)
{
	struct sched_cluster *cluster = container_of(nb, struct sched_cluster, nb_max);

	cluster->max_freq = freq;
	return 0;
}

static int cpufreq_notifier_policy(struct notifier_block *nb,
				   unsigned long val, void *data)
{
	struct cpufreq_policy *policy = (struct cpufreq_policy *)data;
	struct cpumask policy_cluster = *policy->related_cpus;
	struct sched_cluster *cluster = NULL;
	int ret, i, j;

	switch (val) {
	case CPUFREQ_CREATE_POLICY:
		for_each_cpu(i, &policy_cluster) {
			cluster = cpu_rq(i)->cluster;
			cpumask_andnot(&policy_cluster, &policy_cluster,
				       &cluster->cpus);

			if (!cluster->freq_init_done) {
				for_each_cpu(j, &cluster->cpus)
					cpumask_copy(&cpu_rq(j)->freq_domain_cpumask,
						     policy->related_cpus);

				cluster->min_freq = policy->min;
				cluster->max_freq = policy->max;
				cluster->freq_init_done = true;
			}
		}

		ret = freq_qos_add_notifier(&policy->constraints, FREQ_QOS_MIN,
					    &cluster->nb_min);
		if (ret)
			pr_err("Failed to register MIN QoS notifier: %d (%*pbl)\n",
			       ret, cpumask_pr_args(policy->cpus));

		ret = freq_qos_add_notifier(&policy->constraints, FREQ_QOS_MAX,
					    &cluster->nb_max);
		if (ret)
			pr_err("Failed to register MAX QoS notifier: %d (%*pbl)\n",
			       ret, cpumask_pr_args(policy->cpus));

		break;
	case CPUFREQ_REMOVE_POLICY:
		freq_qos_remove_notifier(&policy->constraints, FREQ_QOS_MAX,
					 &cluster->nb_max);
		freq_qos_remove_notifier(&policy->constraints, FREQ_QOS_MIN,
					 &cluster->nb_min);
		ret = 0;
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}
#else
static int cpufreq_notifier_policy(struct notifier_block *nb,
				   unsigned long val, void *data)
{
	struct cpufreq_policy *policy = (struct cpufreq_policy *)data;
	struct cpumask policy_cluster = *policy->related_cpus;
	struct sched_cluster *cluster = NULL;
	int i, j;

	if (val != CPUFREQ_NOTIFY)
		return 0;

	for_each_cpu(i, &policy_cluster) {
		cluster = cpu_rq(i)->cluster;
		cpumask_andnot(&policy_cluster, &policy_cluster,
			       &cluster->cpus);

		cluster->min_freq = policy->min;
		cluster->max_freq = policy->max;

		if (!cluster->freq_init_done) {
			for_each_cpu(j, &cluster->cpus)
				cpumask_copy(&cpu_rq(j)->freq_domain_cpumask,
					     policy->related_cpus);
			cluster->freq_init_done = true;
			continue;
		}
	}

	return 0;
}
#endif

static int cpufreq_notifier_trans(struct notifier_block *nb,
				  unsigned long val, void *data)
{
	struct cpufreq_freqs *freq = (struct cpufreq_freqs *)data;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	unsigned int cpu = freq->policy->cpu;
#else
	unsigned int cpu = freq->cpu;
#endif
	unsigned int new_freq = freq->new;
	struct sched_cluster *cluster = NULL;
	struct cpumask policy_cpus = cpu_rq(cpu)->freq_domain_cpumask;
	int i;

	if (val != CPUFREQ_POSTCHANGE)
		return 0;

	BUG_ON(new_freq == 0);

	for_each_cpu(i, &policy_cpus) {
		cluster = cpu_rq(i)->cluster;

		cluster->cur_freq = new_freq;
		cpumask_andnot(&policy_cpus, &policy_cpus, &cluster->cpus);
	}

	return 0;
}

static struct notifier_block notifier_policy_block = {
	.notifier_call = cpufreq_notifier_policy
};

static struct notifier_block notifier_trans_block = {
	.notifier_call = cpufreq_notifier_trans
};

static int register_sched_callback(void)
{
	int ret;

	ret = cpufreq_register_notifier(&notifier_policy_block,
					CPUFREQ_POLICY_NOTIFIER);
	if (!ret)
		ret = cpufreq_register_notifier(&notifier_trans_block,
						CPUFREQ_TRANSITION_NOTIFIER);

	return ret;
}

/*
 * cpufreq callbacks can be registered at core_initcall or later time.
 * Any registration done prior to that is "forgotten" by cpufreq. See
 * initialization of variable init_cpufreq_transition_notifier_list_called
 * for further information.
 */
core_initcall(register_sched_callback);
