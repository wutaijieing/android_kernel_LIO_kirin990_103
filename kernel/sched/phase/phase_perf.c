/*
 * SPDX-License-Identifier: GPL-2.0
 *
 * phase_perf.c
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

#include <linux/kernel.h>
#include <linux/perf_event.h>
#include <linux/percpu-defs.h>
#include <linux/slab.h>
#include <linux/stop_machine.h>
#include <linux/phase.h>
#include "phase_perf.h"

#define EVENT_TYPE_FLEXIBLE		0
#define EVENT_TYPE_PINNED		1

int *phase_perf_pevents = NULL;
int *phase_perf_fevents = NULL;

static DEFINE_PER_CPU(__typeof__(struct perf_event *)[PHASE_PEVENT_NUM], cpu_phase_perf_events_pinned);
static DEFINE_PER_CPU(__typeof__(struct perf_event *)[PHASE_FEVENT_NUM], cpu_phase_perf_events_flexible);

/* Helpers for phase perf event */
static inline struct perf_event *perf_event_of_cpu(int cpu, int index, int pinned)
{
	if (pinned)
		return per_cpu(cpu_phase_perf_events_pinned, cpu)[index];
	else
		return per_cpu(cpu_phase_perf_events_flexible, cpu)[index];
}

static inline struct perf_event **perf_events_of_cpu(int cpu, int pinned)
{
	if (pinned)
		return per_cpu(cpu_phase_perf_events_pinned, cpu);
	else
		return per_cpu(cpu_phase_perf_events_flexible, cpu);
}

/* Helpers for cpu counters */
static inline u64 read_cpu_counter(int cpu, int index, int pinned)
{
	struct perf_event *event = perf_event_of_cpu(cpu, index, pinned);

	if (!event || !event->pmu)
		return 0;

	return perf_event_local_pmu_read(event);
}

static struct perf_event_attr *alloc_attr(int event_id, int pinned)
{
	struct perf_event_attr *attr = NULL;

	attr = kzalloc(sizeof(struct perf_event_attr), GFP_KERNEL);
	if (!attr)
		return ERR_PTR(-ENOMEM);

	attr->type = PERF_TYPE_RAW;
	attr->config = (u64)event_id;
	attr->size = sizeof(struct perf_event_attr);
	attr->pinned = (u64)pinned;
	attr->disabled = 1;
	attr->exclude_hv = sysctl_phase_exclude_hv;
	attr->exclude_idle = sysctl_phase_exclude_idle;
	attr->exclude_kernel = sysctl_phase_exclude_kernel;

	return attr;
}

static int create_cpu_counter(int cpu, int event_id, int index, int pinned)
{
	struct perf_event_attr *attr = NULL;
	struct perf_event **events = perf_events_of_cpu(cpu, pinned);
	struct perf_event *event = NULL;

	attr = alloc_attr(event_id, pinned);
	if (IS_ERR(attr))
		return PTR_ERR(attr);

	event = perf_event_create_kernel_counter(attr, cpu, NULL, NULL, NULL);
	if (IS_ERR(event)) {
		pr_err("unable to create perf event (cpu:%i-type:%d-pinned:%d-config:0x%llx) : %ld",
		       cpu, attr->type, attr->pinned, attr->config, PTR_ERR(event));
		kfree(attr);
		return PTR_ERR(event);
	}

	events[index] = event;
	perf_event_enable(events[index]);
	if (pinned && event->hw.idx == -1) {
		pr_err("pinned event unable to get onto hardware, perf event (cpu:%i-type:%d-config:0x%llx)",
		       cpu, attr->type, attr->config);
		kfree(attr);
		return -EINVAL;
	}
	pr_debug("create perf_event (cpu:%i-idx:%d-type:%d-pinned:%d-exclude_hv:%d"
		 "-exclude_idle:%d-exclude_kernel:%d-config:0x%llx-addr:%llx)",
		 event->cpu, event->hw.idx,
		 event->attr.type, event->attr.pinned, event->attr.exclude_hv,
		 event->attr.exclude_idle, event->attr.exclude_kernel,
		 event->attr.config, event);

	kfree(attr);
	return 0;
}

static int create_cpu_group_counters(int cpu, int *event_ids, int num, int *index, int pinned)
{
	int err;
	int idx = *index;
	int event_id;
	struct perf_event *event = NULL;
	struct perf_event **events = perf_events_of_cpu(cpu, pinned);
	struct perf_event_attr *attr = NULL;

	/* Create group leader first */
	err = create_cpu_counter(cpu, event_ids[idx], idx, pinned);
	if (err) {
		pr_err("create phase perf group leader failed\n");
		return err;
	}

	/* Then the group members */
	for (idx = idx + 1; idx < num; idx++) {
		event_id = event_ids[idx];
		if (event_id == PHASE_EVENT_GROUP_TERMINATOR)
			break;

		attr = alloc_attr(event_id, pinned);
		if (IS_ERR(attr))
			return PTR_ERR(attr);

		event = perf_event_create_kernel_group_counter(attr, cpu, NULL, events[*index], NULL, NULL);
		if (IS_ERR(event)) {
			pr_err("unable to create perf event (cpu:%i-type:%d-pinned:%d-config:0x%llx) : %ld\n",
			       cpu, attr->type, attr->pinned, attr->config, PTR_ERR(event));
			kfree(attr);
			return PTR_ERR(event);
		}

		events[idx] = event;
		perf_event_enable(event);
		pr_debug("create perf_event (cpu:%i-idx:%d-type:%d-pinned:%d-exclude_hv:%d"
			 "-exclude_idle:%d-exclude_kernel:%d-config:0x%llx-leader:%llx)",
			 event->cpu, event->hw.idx,
			 event->attr.type, event->attr.pinned, event->attr.exclude_hv,
			 event->attr.exclude_idle, event->attr.exclude_kernel,
			 event->attr.config, event->group_leader);
		kfree(attr);
	}

	*index = idx;
	return 0;
}

static int release_cpu_counter(int cpu, int event_id, int index, int pinned)
{
	struct perf_event **events = perf_events_of_cpu(cpu, pinned);
	struct perf_event *event = NULL;

	event = events[index];
	if (!event)
		return 0;

	pr_debug("release perf_event (cpu:%i-idx:%d-type:%d-pinned:%d-exclude_hv:%d"
		 "-exclude_idle:%d-exclude_kernel:%d-config:0x%llx)\n",
		 event->cpu, event->hw.idx,
		 event->attr.type, event->attr.pinned, event->attr.exclude_hv,
		 event->attr.exclude_idle, event->attr.exclude_kernel,
		 event->attr.config);

	perf_event_release_kernel(event);
	events[index] = NULL;

	return 0;
}

static int release_cpu_group_counters(int cpu, int *event_ids, int num, int *index, int pinned)
{
	int idx = *index;
	int event_id;

	/* release group member events */
	for (idx = idx + 1; idx < num; idx++) {
		event_id = event_ids[idx];
		if (event_id == PHASE_EVENT_GROUP_TERMINATOR)
			break;
		release_cpu_counter(cpu, event_id, idx, pinned);
	}
	/* release group leader event */
	release_cpu_counter(cpu, event_id, *index, pinned);

	*index = idx;
	return 0;
}

/* Helpers for phase perf */
static int do_pevents(int (*fn)(int, int, int, int), int cpu)
{
	int index;
	int err;

	for_each_phase_pevents(index, phase_perf_pevents) {
		err = fn(cpu, phase_perf_pevents[index], index, EVENT_TYPE_PINNED);
		if (err)
			return err;
	}

	return 0;
}

static int do_fevents(int (*fn)(int, int*, int, int*, int), int cpu)
{
	int index;
	int err;

	for_each_phase_fevents(index, phase_perf_fevents) {
		err = fn(cpu, phase_perf_fevents, PHASE_FEVENT_NUM, &index, EVENT_TYPE_FLEXIBLE);
		if (err)
			return err;
	}

	return 0;
}

static int __phase_perf_create(void *args)
{
	int err;
	int cpu = raw_smp_processor_id();

	/* create pinned events */
	err = do_pevents(create_cpu_counter, cpu);
	if (err) {
		pr_err("create pinned events failed\n");
		do_pevents(release_cpu_counter, cpu);
		return err;
	}

	/* create flexible event groups */
	err = do_fevents(create_cpu_group_counters, cpu);
	if (err) {
		pr_err("create flexible events failed\n");
		do_fevents(release_cpu_group_counters, cpu);
		return err;
	}

	pr_info("CPU%d phase class event create success\n", cpu);
	return 0;
}

int phase_perf_create(int *pevents, int *fevents, const struct cpumask *cpus)
{
	phase_perf_pevents = pevents;
	phase_perf_fevents = fevents;
	return stop_machine(__phase_perf_create, NULL, cpus);
}

static int __phase_perf_release(void *args)
{
	int cpu = raw_smp_processor_id();

	/* release pinned events */
	do_pevents(release_cpu_counter, cpu);

	/* release flexible event groups */
	do_fevents(release_cpu_group_counters, cpu);

	pr_info("CPU%d phase class event release success\n", cpu);
	return 0;
}

void phase_perf_release(const struct cpumask *cpus)
{
	stop_machine(__phase_perf_release, NULL, cpus);
	phase_perf_pevents = NULL;
	phase_perf_fevents = NULL;
}

int *phase_perf_pinned_events(void)
{
	return phase_perf_pevents;
}

int *phase_perf_flexible_events(void)
{
	return phase_perf_fevents;
}

void phase_perf_read_events(int cpu, u64 *pdata, u64 *fdata)
{
	int event_id;
	int index;

	for_each_phase_pevents(index, phase_perf_pevents)
		pdata[index] = read_cpu_counter(cpu, index, EVENT_TYPE_PINNED);

	for_each_phase_fevents(index, phase_perf_fevents) {
		event_id = phase_perf_fevents[index];
		if (event_id == PHASE_EVENT_GROUP_TERMINATOR)
			continue;
		fdata[index] = read_cpu_counter(cpu, index, EVENT_TYPE_FLEXIBLE);
	}
}
