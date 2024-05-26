// SPDX-License-Identifier: GPL-2.0
/*
 * smt_mode_gov.c
 *
 * smt mode governor
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

#include <linux/cpuhotplug.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/kthread.h>
#include <linux/percpu.h>
#include <linux/math64.h>
#include <linux/version.h>
#include <linux/sched/rt.h>
#include <linux/capability.h>
#include <linux/arch_topology.h>
#include <uapi/linux/sched/types.h>
#include <linux/core_ctl.h>
#include <securec.h>

#define CREATE_TRACE_POINTS
#include <trace/events/smt.h>

#include "sched.h"
#include "smt_mode_gov.h"


static DEFINE_PER_CPU(struct smt_cpu_state*, smt_data);

void set_smt_cpu_req(int cpu, unsigned int req)
{
	struct smt_cpu_state *cpu_data = per_cpu(smt_data, cpu);

	if (cpu_data)
		cpu_data->req_cpus = req;
}

unsigned int get_smt_cpu_req(int cpu)
{
	struct smt_cpu_state *cpu_data = per_cpu(smt_data, cpu);

	if (cpu_data)
		return cpu_data->req_cpus;

	return 0;
}

/* called with lock held */
static void update_target_mode(struct smt_cpu_state *data)
{
	unsigned int curr = data->curr_mode;
	unsigned int target = SMT_MODE;

	if (data->user_req_mode != UINT_MAX) {
		target = data->user_req_mode;
		goto update;
	}

	if ((cpumask_weight(&data->smt_mask) -
	     cpumask_weight(&data->idle_mask) -
	     cpumask_weight(&data->offline_mask)) <= 1)
		target = ST_MODE;

update:
	if (target != curr) {
		data->target_mode = target;
		data->need_update = true;
	}
}

static void do_update_mode(struct smt_cpu_state *data)
{
	int i;

	trace_smt_update_mode(&data->smt_mask,
			      data->curr_mode, data->target_mode,
			      data->cpu_max_cap[data->target_mode]);

	for_each_cpu(i, &data->smt_mask)
		topology_set_cpu_scale(i, data->cpu_max_cap[data->target_mode]);

	update_cpus_capacity(&data->smt_mask);
	data->curr_mode = data->target_mode;
	data->nr_switch++;
}

static int update_mode(void *data)
{
	struct smt_cpu_state *cpu_data = data;

	while (1) {
		set_current_state(TASK_INTERRUPTIBLE);
		write_lock(&cpu_data->state_lock);
		if (!cpu_data->need_update) {
			write_unlock(&cpu_data->state_lock);
			schedule();
			if (kthread_should_stop())
				break;
			write_lock(&cpu_data->state_lock);
		}
		set_current_state(TASK_RUNNING);
		update_target_mode(cpu_data);

		if (cpu_data->need_update) {
			cpu_data->need_update = false;
			do_update_mode(cpu_data);
		}

		write_unlock(&cpu_data->state_lock);
	}

	return 0;
}

static int cpuhp_smt_online(unsigned int cpu)
{
	bool need_update = false;
	struct smt_cpu_state *cpu_data = per_cpu(smt_data, cpu);

	if (!cpu_data)
		return 0;

	write_lock(&cpu_data->state_lock);
	cpumask_clear_cpu(cpu, &cpu_data->offline_mask);

	update_target_mode(cpu_data);
	need_update = cpu_data->need_update;
	write_unlock(&cpu_data->state_lock);

	if (need_update)
		wake_up_process(cpu_data->update_thread);

	return 0;
}

static int cpuhp_smt_offline(unsigned int cpu)
{
	bool need_update = false;
	struct smt_cpu_state *cpu_data = per_cpu(smt_data, cpu);

	if (!cpu_data)
		return 0;

	write_lock(&cpu_data->state_lock);
	cpumask_set_cpu(cpu, &cpu_data->offline_mask);

	update_target_mode(cpu_data);
	need_update = cpu_data->need_update;
	write_unlock(&cpu_data->state_lock);

	if (need_update)
		wake_up_process(cpu_data->update_thread);

	return 0;
}

static int update_isolated_cpu(struct notifier_block *nb,
			       unsigned long val, void *data)
{
	int cpu = *(int *)data;
	struct smt_cpu_state *cpu_data = per_cpu(smt_data, cpu);

	if (!cpu_data)
		return 0;

	write_lock(&cpu_data->state_lock);
	cpumask_and(&cpu_data->isolated_mask,
		    &cpu_data->smt_mask, cpu_isolated_mask);
	write_unlock(&cpu_data->state_lock);

	return NOTIFY_OK;
}

static struct notifier_block isolation_update_nb = {
	.notifier_call = update_isolated_cpu,
};

static int update_idle_cpu(struct notifier_block *nb,
			   unsigned long action, void *data)
{
	bool need_update = false;
	int cpu = smp_processor_id();
	struct smt_cpu_state *cpu_data = per_cpu(smt_data, cpu);

	if (!cpu_data)
		return NOTIFY_OK;

	write_lock(&cpu_data->state_lock);
	switch (action) {
	case IDLE_START:
		cpumask_set_cpu(cpu, &cpu_data->idle_mask);
		update_target_mode(cpu_data);
		need_update = cpu_data->need_update;
		break;
	case IDLE_END:
		cpumask_clear_cpu(cpu, &cpu_data->idle_mask);
		update_target_mode(cpu_data);
		need_update = cpu_data->need_update;
		break;
	default:
		break;
	}
	write_unlock(&cpu_data->state_lock);

	if (need_update)
		wake_up_process(cpu_data->update_thread);

	return NOTIFY_OK;
}

static struct notifier_block idle_update_nb = {
	.notifier_call = update_idle_cpu,
};

/* ========================= sysfs interface =========================== */
static ssize_t show_status(struct device *dev,
			   struct device_attribute *attr,
			   char *buf)
{
	ssize_t count = 0;
	struct smt_cpu_state *cpu_data = per_cpu(smt_data, dev->id);

	if (!cpu_data)
		return snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "\n");

	read_lock(&cpu_data->state_lock);
	count += snprintf_s(buf + count, PAGE_SIZE - count, PAGE_SIZE - count - 1,
			  "all:%*pbl\n", cpumask_pr_args(&cpu_data->smt_mask));
	count += snprintf_s(buf + count, PAGE_SIZE - count, PAGE_SIZE - count - 1,
			  "isolated:%*pbl\n", cpumask_pr_args(&cpu_data->isolated_mask));
	count += snprintf_s(buf + count, PAGE_SIZE - count, PAGE_SIZE - count - 1,
			  "idle:%*pbl\n", cpumask_pr_args(&cpu_data->idle_mask));
	count += snprintf_s(buf + count, PAGE_SIZE - count, PAGE_SIZE - count - 1,
			  "offline:%*pbl\n", cpumask_pr_args(&cpu_data->offline_mask));
	count += snprintf_s(buf + count, PAGE_SIZE - count, PAGE_SIZE - count - 1,
			  "curr_mode:%s\n", (cpu_data->curr_mode == ST_MODE) ? "ST" : "MT");
	count += snprintf_s(buf + count, PAGE_SIZE - count, PAGE_SIZE - count - 1,
			  "user_req_mode:%s\n",
			  (cpu_data->user_req_mode == ST_MODE) ?
				"ST" : ((cpu_data->user_req_mode == SMT_MODE) ? "MT" : "NONE"));
	count += snprintf_s(buf + count, PAGE_SIZE - count, PAGE_SIZE - count - 1,
			  "target_mode:%s\n", (cpu_data->target_mode == ST_MODE) ? "ST" : "MT");
	count += snprintf_s(buf + count, PAGE_SIZE - count, PAGE_SIZE - count - 1,
			  "req_cpus:%u\n", cpu_data->req_cpus);
	count += snprintf_s(buf + count, PAGE_SIZE - count, PAGE_SIZE - count - 1,
			  "nr_switch:%u\n", cpu_data->nr_switch);
	read_unlock(&cpu_data->state_lock);

	return count;
}

static ssize_t show_req_mode(struct device *dev,
			     struct device_attribute *attr,
			     char *buf)
{
	struct smt_cpu_state *cpu_data = per_cpu(smt_data, dev->id);
	unsigned int user_req_mode;

	if (!cpu_data)
		return snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "\n");

	read_lock(&cpu_data->state_lock);
	user_req_mode = cpu_data->user_req_mode;
	read_unlock(&cpu_data->state_lock);

	return snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "%u\n", user_req_mode);
}

static ssize_t store_req_mode(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t count)
{
	struct smt_cpu_state *cpu_data = per_cpu(smt_data, dev->id);
	unsigned int mode;
	bool need_update = false;
	int err;

	if (!cpu_data)
		return count;

	err = kstrtouint(buf, 0, &mode);
	if (err)
		return err;

	write_lock(&cpu_data->state_lock);
	cpu_data->user_req_mode = mode;

	update_target_mode(cpu_data);
	need_update = cpu_data->need_update;
	write_unlock(&cpu_data->state_lock);

	if (need_update)
		wake_up_process(cpu_data->update_thread);

	return count;
}

static DEVICE_ATTR(status, 0440, show_status, NULL);
static DEVICE_ATTR(req_mode, 0660, show_req_mode, store_req_mode);

static struct attribute *smt_attrs[] = {
	&dev_attr_status.attr,
	&dev_attr_req_mode.attr,
	NULL
};

static struct attribute_group smt_attr_group = {
	.attrs = smt_attrs,
	.name = "smt",
};

int smt_add_interface(struct device *dev)
{
	return sysfs_create_group(&dev->kobj, &smt_attr_group);
}

static int cpu_smt_data_init(int cpu)
{
	int i;
	struct smt_cpu_state *cpu_data = NULL;
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };

	if (per_cpu(smt_data, cpu))
		return 0;

	cpu_data = kzalloc(sizeof(struct smt_cpu_state), GFP_KERNEL);
	if (IS_ERR_OR_NULL(cpu_data)) {
		pr_err("%s: alloc smt data for CPU%d err\n", cpu);
		return -ENOMEM;
	}

	rwlock_init(&cpu_data->state_lock);
	cpu_data->curr_mode = ST_MODE; /* ST_MODE default */
	cpu_data->user_req_mode = UINT_MAX; /* no req default */
	cpumask_copy(&cpu_data->smt_mask, cpu_smt_mask(cpu));
	cpumask_clear(&cpu_data->offline_mask);
	cpumask_clear(&cpu_data->isolated_mask);
	cpumask_clear(&cpu_data->idle_mask);

	/* initialize max cpu cap in ST&MT modes */
	cpu_data->cpu_max_cap[ST_MODE] = topology_get_cpu_scale(NULL, cpu);
	cpu_data->cpu_max_cap[SMT_MODE] = (cpu_data->cpu_max_cap[ST_MODE] * 660) >> 10; /* ~65% */

	cpu_data->update_thread = kthread_run(update_mode, (void *)cpu_data,
					       "smt/%u", cpu);
	if (IS_ERR(cpu_data->update_thread)) {
		pr_err("%s: thread create err\n", __func__);
		return PTR_ERR(cpu_data->update_thread);
	}

	sched_setscheduler_nocheck(cpu_data->update_thread, SCHED_FIFO,
				   &param);

	for_each_cpu(i, cpu_smt_mask(cpu))
		per_cpu(smt_data, i) = cpu_data;
}

static int smt_mode_init(void)
{
	int cpu, ret;
	struct device *cpu_dev = NULL;

	for_each_possible_cpu(cpu) {
		if (cpumask_weight(cpu_smt_mask(cpu)) < 2)
			continue;

		ret = cpu_smt_data_init(cpu);
		if (ret)
			return ret;

		cpu_dev = get_cpu_device(cpu);
		ret = smt_add_interface(cpu_dev);
		if (ret)
			return ret;
	}

	/* register notifiers */
	ret = cpuhp_setup_state_nocalls_cpuslocked(CPUHP_AP_ONLINE_DYN,
						   "sched_smt:online",
						   cpuhp_smt_online,
						   cpuhp_smt_offline);
	if (ret < 0)
		return ret;

	core_ctl_notifier_register(&isolation_update_nb);
	idle_notifier_register(&idle_update_nb);
}
late_initcall(smt_mode_init);
