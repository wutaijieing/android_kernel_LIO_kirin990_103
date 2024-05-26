// SPDX-License-Identifier: GPL-2.0
/*
 * sched_cluster.h
 *
 * sched topology data structure and interfaces
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
#ifndef __SCHED_CLUSTER_H
#define __SCHED_CLUSTER_H

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/list_sort.h>
#include <linux/notifier.h>

struct sched_cluster {
	struct list_head list;
	struct cpumask cpus;
	int id;
	int max_possible_capacity;
	unsigned int cur_freq, max_freq, min_freq;
	unsigned int capacity_margin;
	unsigned int sd_capacity_margin;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,0))
	struct notifier_block nb_min;
	struct notifier_block nb_max;
#endif
	bool freq_init_done;
};

extern struct list_head cluster_head;
extern int num_clusters;
extern struct sched_cluster init_cluster;

extern void update_cluster_topology(void);
extern void init_clusters(void);

/* Iterate in increasing order of cluster max possible capacity */
#define for_each_sched_cluster(cluster) \
	list_for_each_entry(cluster, &cluster_head, list)

#define for_each_sched_cluster_reverse(cluster) \
	list_for_each_entry_reverse(cluster, &cluster_head, list)

#define min_cap_cluster()	\
	list_first_entry(&cluster_head, struct sched_cluster, list)
#define max_cap_cluster()	\
	list_last_entry(&cluster_head, struct sched_cluster, list)

#endif
