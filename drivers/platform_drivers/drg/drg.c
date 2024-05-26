// SPDX-License-Identifier: GPL-2.0-only
/*
 * drg_v2.c
 *
 * function of drg module
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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/notifier.h>
#include <linux/topology.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/cpufreq.h>
#include <linux/devfreq.h>
#include <linux/cpu_pm.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>
#include <linux/topology.h>
#include <linux/of.h>
#include <linux/pm_qos.h>
#include <linux/platform_drivers/perf_mode.h>

#include <securec.h>
#include <linux/drg.h>

#define CPUFREQ_AMP	1000UL
#define DEVFREQ_AMP	1UL
#define KHZ		1000UL
#define INVALID_STATE	(-1)
#define IDLE_UPDATE_SLACK_TIME 20

struct drg_freq_device {
	/* driver pointer of cpufreq device */
	struct cpufreq_policy *policy;
	/* driver pointer of devfreq device */
	struct devfreq *df;
	/* device node for binding drg_device */
	struct device_node *np;
	/* work used to unbind drg_device */
	struct kthread_work destroy_work;
	/* list of all binded master drg_device */
	struct list_head master_list;
	/* list of all binded slave drg_device */
	struct list_head slave_list;
	/* list node for g_drg_cpufreq_list or g_drg_devfreq_list */
	struct list_head node;
	/* amplifer used to convert device input freq to Hz */
	unsigned int amp;
	unsigned long cur_freq;
	/* idle state */
	bool idle;
};

struct drg_rule;

struct freq_qos {
	union {
		struct freq_qos_request cpufreq;
		struct dev_pm_qos_request devfreq;
	} req;
};

struct drg_device {
	/* binded drg_freq_device */
	struct drg_freq_device *freq_dev;
	/* device node parsed from phandle for binding drg freq device */
	struct device_node *np;
	/* the drg rule this device belong to */
	struct drg_rule *ruler;
	/* range divider freq, described as KHz */
	unsigned int *divider;
	/* list node for drg freq device's master list or slave list */
	struct list_head node;
	/* cur state index for master */
	int state;
	/* for freq qos */
	struct freq_qos **max_freq;
	struct freq_qos **min_freq;
};

struct drg_rule {
	struct kobject kobj;
	/* work used to notify slave device the freq range has changed */
	struct kthread_work exec_work;
	/* list node for g_drg_rule_list */
	struct list_head node;
	/* all master belong to this rule */
	struct drg_device *master;
	/* all slave device belong to this rule */
	struct drg_device *slave;
	/* rule name */
	const char *name;
	/* divider number */
	int divider_num;
	/* total number of masters */
	int nr_master;
	/* total number of slaves */
	int nr_slave;
	/* limit slave's minimum freq */
	bool dn_limit;
	/* limit slave's maximum freq */
	bool up_limit;
	/* dynamic switch setting in different scenario, default false */
	bool user_mode_enable;
	bool enable;
};

/* protect following lists */
static DEFINE_MUTEX(drg_list_mutex);
static LIST_HEAD(g_drg_rule_list);
static LIST_HEAD(g_drg_cpufreq_list);
static LIST_HEAD(g_drg_devfreq_list);
/* protect g_drg_idle_update_timer */
static DEFINE_SPINLOCK(idle_state_lock);
static struct timer_list g_drg_idle_update_timer;
static DEFINE_KTHREAD_WORKER(g_drg_worker);
static struct task_struct *g_drg_work_thread;
static bool g_drg_initialized;
static DEFINE_PER_CPU(struct drg_freq_device *, cpufreq_dev);


static struct drg_freq_device *
drg_get_cpufreq_dev_by_policy(struct cpufreq_policy *policy)
{
	struct drg_freq_device *freq_dev = NULL;

	mutex_lock(&drg_list_mutex);
	/* find the freq device of cpufreq */
	list_for_each_entry(freq_dev, &g_drg_cpufreq_list, node) {
		if (freq_dev->policy == policy) {
			mutex_unlock(&drg_list_mutex);
			return freq_dev;
		}
	}

	mutex_unlock(&drg_list_mutex);
	return NULL;
}

static struct drg_freq_device *drg_get_devfreq_dev(struct devfreq *df)
{
	struct drg_freq_device *freq_dev = NULL;

	mutex_lock(&drg_list_mutex);
	/* find the freq device of devfreq */
	list_for_each_entry(freq_dev, &g_drg_devfreq_list, node) {
		if (freq_dev->df == df) {
			mutex_unlock(&drg_list_mutex);
			return freq_dev;
		}
	}

	mutex_unlock(&drg_list_mutex);
	return NULL;
}

static int drg_freq2state(struct drg_rule *ruler,
			  unsigned int *divider, unsigned long freq)
{
	int i;
	int state = 0;

	/* convert freq to freq range index according to divider */
	for (i = 0; i < ruler->divider_num; i++) {
		if (freq >= divider[i])
			state++;
	}

	return state;
}

static int find_proper_upbound(struct drg_freq_device *freq_dev,
			       unsigned long *freq)
{
	struct device *dev = NULL;
	struct dev_pm_opp *opp = NULL;

	if (freq_dev->policy)
		dev = get_cpu_device(freq_dev->policy->cpu);
	else if (freq_dev->df)
		dev = freq_dev->df->dev.parent;
	else
		dev = NULL;

	if (IS_ERR_OR_NULL(dev))
		return -ENODEV;

	opp = dev_pm_opp_find_freq_floor(dev, freq);
	if (IS_ERR(opp))
		return -ENODEV;
	dev_pm_opp_put(opp);

	return 0;
}

static int drg_alloc_max_freq_qos(struct device *dev, struct drg_rule *ruler)
{
	int i, j;
	struct freq_qos *qos = NULL;
	int nr_master = ruler->nr_master;
	int nr_slave = ruler->nr_slave;

	qos = (struct freq_qos *)devm_kzalloc(dev,
			sizeof(struct freq_qos) * nr_master * nr_slave,
			GFP_KERNEL);
	if (!qos)
		return -ENOMEM;

	for (i = 0; i < nr_master; i++) {
		struct drg_device *master = &ruler->master[i];

		master->max_freq = (struct freq_qos **)
					devm_kzalloc(dev,
					sizeof(struct freq_qos *) * nr_slave,
					GFP_KERNEL);

		if (!master->max_freq)
			return -ENOMEM;
	}

	for (i = 0; i < nr_slave; i++) {
		struct drg_device *slave = &ruler->slave[i];

		slave->max_freq = (struct freq_qos **)
					devm_kzalloc(dev,
					sizeof(struct freq_qos *) * nr_master,
					GFP_KERNEL);
		if (!slave->max_freq)
			return -ENOMEM;
	}

	for (i = 0; i < nr_master; i++) {
		struct drg_device *master = &ruler->master[i];

		for (j = 0; j < nr_slave; j++) {
			struct drg_device *slave = &ruler->slave[j];

			master->max_freq[j] = &qos[i * nr_slave + j];
			slave->max_freq[i] = &qos[i * nr_slave + j];
		}
	}
	return 0;
}

static int drg_alloc_min_freq_qos(struct device *dev, struct drg_rule *ruler)
{
	int i, j;
	struct freq_qos *qos = NULL;
	int nr_master = ruler->nr_master;
	int nr_slave = ruler->nr_slave;

	qos = (struct freq_qos *)devm_kzalloc(dev,
			sizeof(struct freq_qos) * nr_master * nr_slave,
			GFP_KERNEL);
	if (!qos)
		return -ENOMEM;

	for (i = 0; i < nr_master; i++) {
		struct drg_device *master = &ruler->master[i];

		master->min_freq = (struct freq_qos **)
					devm_kzalloc(dev,
					sizeof(struct freq_qos *) * nr_slave,
					GFP_KERNEL);
		if (!master->min_freq)
			return -ENOMEM;
	}

	for (i = 0; i < nr_slave; i++) {
		struct drg_device *slave = &ruler->slave[i];

		slave->min_freq = (struct freq_qos **)
					devm_kzalloc(dev,
					sizeof(struct freq_qos *) * nr_master,
					GFP_KERNEL);
		if (!slave->min_freq)
			return -ENOMEM;
	}

	for (i = 0; i < nr_master; i++) {
		struct drg_device *master = &ruler->master[i];

		for (j = 0; j < nr_slave; j++) {
			struct drg_device *slave = &ruler->slave[j];

			master->min_freq[j] = &qos[i * nr_slave + j];
			slave->min_freq[i] = &qos[i * nr_slave + j];
		}
	}

	return 0;
}

static int drg_alloc_freq_qos(struct device *dev, struct drg_rule *ruler)
{
	int ret;

	if (ruler->up_limit) {
		ret = drg_alloc_max_freq_qos(dev, ruler);
		if (ret)
			return ret;
	}

	if (ruler->dn_limit) {
		ret = drg_alloc_min_freq_qos(dev, ruler);
		if (ret)
			return ret;
	}

	return 0;
}

static int drg_init_max_freq_qos(struct freq_qos *max_freq,
				 struct drg_freq_device *freq_dev)
{
	if (freq_dev->policy)
		return freq_qos_add_request(
			&(freq_dev->policy->constraints),
			&(max_freq->req.cpufreq),
			FREQ_QOS_MAX, PM_QOS_MAX_FREQUENCY_DEFAULT_VALUE);

	if (freq_dev->df)
		return dev_pm_qos_add_request(
			freq_dev->df->dev.parent,
			&(max_freq->req.devfreq),
			DEV_PM_QOS_MAX_FREQUENCY,
			PM_QOS_MAX_FREQUENCY_DEFAULT_VALUE);

	return -EINVAL;
}

static int drg_init_min_freq_qos(struct freq_qos *min_freq,
				 struct drg_freq_device *freq_dev)
{
	if (freq_dev->policy)
		return freq_qos_add_request(
			&(freq_dev->policy->constraints),
			&(min_freq->req.cpufreq),
			FREQ_QOS_MIN, PM_QOS_MIN_FREQUENCY_DEFAULT_VALUE);

	if (freq_dev->df)
		return dev_pm_qos_add_request(
			freq_dev->df->dev.parent,
			&(min_freq->req.devfreq),
			DEV_PM_QOS_MIN_FREQUENCY,
			PM_QOS_MIN_FREQUENCY_DEFAULT_VALUE);

	return -EINVAL;

}

static void drg_init_freq_qos(struct drg_freq_device *freq_dev,
			      struct drg_rule *ruler)
{
	struct drg_device *master = NULL;
	struct drg_device *slave = NULL;
	int i;
	int ret = 0;

	list_for_each_entry(master, &freq_dev->master_list, node) {
		if (master->ruler != ruler)
			continue;

		for (i = 0; i < ruler->nr_slave; i++) {
			struct drg_device *slave = &ruler->slave[i];

			if (IS_ERR_OR_NULL(slave->freq_dev))
				continue;

			if (ruler->up_limit)
				ret = drg_init_max_freq_qos(master->max_freq[i], slave->freq_dev);

			if (ruler->dn_limit)
				ret = drg_init_min_freq_qos(master->min_freq[i], slave->freq_dev);

			if (ret < 0)
				pr_err("rule %s qos init fail, m=%s s=%s ret=%d\n",
				       ruler->name, freq_dev->np->full_name,
				       slave->freq_dev->np->full_name, ret);
		}
	}

	list_for_each_entry(slave, &freq_dev->slave_list, node) {
		if (slave->ruler != ruler)
			continue;

		for (i = 0; i < ruler->nr_master; i++) {
			struct drg_device *master = &ruler->master[i];

			if (IS_ERR_OR_NULL(master->freq_dev))
				continue;

			if (ruler->up_limit)
				ret = drg_init_max_freq_qos(slave->max_freq[i], freq_dev);

			if (ruler->dn_limit)
				ret = drg_init_min_freq_qos(slave->min_freq[i], freq_dev);

			if (ret < 0)
				pr_err("rule %s qos init fail, m=%s s=%s ret=%d\n",
				       ruler->name, master->freq_dev->np->full_name,
				       freq_dev->np->full_name, ret);
		}
	}
}

static void drg_remove_freq_qos(struct drg_freq_device *freq_dev)
{
	struct drg_device *drg_dev = NULL;
	struct drg_device *tmp = NULL;
	struct drg_rule *ruler = NULL;
	int i;
	int ret = 0;

	list_for_each_entry_safe(drg_dev, tmp, &freq_dev->slave_list, node) {
		ruler = drg_dev->ruler;

		for (i = 0; i < ruler->nr_master; i++) {
			struct drg_device *master = &ruler->master[i];

			if (IS_ERR_OR_NULL(master->freq_dev))
				continue;

			if (drg_dev->freq_dev->policy) {
				if (ruler->up_limit)
					ret = freq_qos_remove_request(
						&(drg_dev->max_freq[i]->req.cpufreq));

				if (ruler->dn_limit)
					ret = freq_qos_remove_request(
						&(drg_dev->min_freq[i]->req.cpufreq));
			} else if (drg_dev->freq_dev->df) {
				if (ruler->up_limit)
					ret = dev_pm_qos_remove_request(
						&(drg_dev->max_freq[i]->req.devfreq));

				if (ruler->dn_limit)
					ret = dev_pm_qos_remove_request(
						&(drg_dev->min_freq[i]->req.devfreq));
			}

			if (ret < 0)
				pr_err("rule %s remove qos fail, m=%d s=%s ret=%d\n",
				       ruler->name, i, freq_dev->np->full_name, ret);
		}
	}

	list_for_each_entry_safe(drg_dev, tmp, &freq_dev->master_list, node) {
		ruler = drg_dev->ruler;

		for (i = 0; i < ruler->nr_slave; i++) {
			struct drg_device *slave = &ruler->slave[i];

			if (IS_ERR_OR_NULL(slave->freq_dev))
				continue;

			if (slave->freq_dev->policy) {
				if (ruler->up_limit)
					ret = freq_qos_remove_request(
						&(drg_dev->max_freq[i]->req.cpufreq));

				if (ruler->dn_limit)
					ret = freq_qos_remove_request(
						&(drg_dev->min_freq[i]->req.cpufreq));
			} else if (slave->freq_dev->df) {
				if (ruler->up_limit)
					ret = dev_pm_qos_remove_request(
						&(drg_dev->max_freq[i]->req.devfreq));

				if (ruler->dn_limit)
					ret = dev_pm_qos_remove_request(
						&(drg_dev->min_freq[i]->req.devfreq));
			}

			if (ret < 0)
				pr_err("rule %s remove qos fail, m=%s s=%d ret=%d\n",
				       ruler->name, freq_dev->np->full_name, i, ret);
		}
	}
}

static void drg_update_freq_qos(struct drg_rule *ruler)
{
	int i, j, ret;

	for (i = 0; i < ruler->nr_master; i++) {
		struct drg_device *master = &ruler->master[i];
		int state;

		if (IS_ERR_OR_NULL(master->freq_dev))
			continue;

		state = master->freq_dev->idle ? 0 : master->state;
		for (j = 0; j < ruler->nr_slave; j++) {
			struct drg_device *slave = &ruler->slave[j];
			long min, max;

			if (IS_ERR_OR_NULL(slave->freq_dev))
				continue;

			if (ruler->dn_limit) {
				min = (!ruler->enable || state <= 0) ?
				      PM_QOS_MIN_FREQUENCY_DEFAULT_VALUE :
				      (slave->divider[state - 1] * slave->freq_dev->amp);

				if (slave->freq_dev->policy)
					ret = freq_qos_update_request(&(master->min_freq[j]->req.cpufreq),
								      (int)(min == PM_QOS_MIN_FREQUENCY_DEFAULT_VALUE ? min : min / KHZ));
				else if (slave->freq_dev->df)
					ret = dev_pm_qos_update_request(&(master->min_freq[j]->req.devfreq), (int)min);
			}

			if (ruler->up_limit) {
				if (!ruler->enable || state < 0 || state == ruler->divider_num)
					max = PM_QOS_MAX_FREQUENCY_DEFAULT_VALUE;
				else {
					max = (slave->divider[state] - 1) * slave->freq_dev->amp;
					(void)find_proper_upbound(slave->freq_dev, &max);
				}

				if (slave->freq_dev->policy)
					ret = freq_qos_update_request(&(master->max_freq[j]->req.cpufreq),
								      (int)(max == PM_QOS_MAX_FREQUENCY_DEFAULT_VALUE ? max : max / KHZ));
				else if (slave->freq_dev->df)
					ret = dev_pm_qos_update_request(&(master->max_freq[j]->req.devfreq), (int)max);
			}

			if (ret < 0)
				pr_err("rule %s update qos fail, m=%s s=%s ret=%d\n",
				       ruler->name, master->freq_dev->np->full_name,
				       slave->freq_dev->np->full_name, ret);
		}
	}
}

static void drg_rule_update_work(struct kthread_work *work)
{
	struct drg_rule *ruler = container_of(work, struct drg_rule, exec_work);

	get_online_cpus();
	drg_update_freq_qos(ruler);
	put_online_cpus();
}

static void drg_update_rule_state(struct drg_rule *ruler, bool force)
{
	if (!force && !ruler->enable)
		return;

	kthread_queue_work(&g_drg_worker, &ruler->exec_work);
}

static void drg_refresh_all_rule(void)
{
	struct drg_rule *ruler = NULL;

	if (!g_drg_initialized)
		return;

	list_for_each_entry(ruler, &g_drg_rule_list, node)
		drg_update_rule_state(ruler, false);
}

static void drg_update_master_state(struct drg_freq_device *freq_dev, bool idle_exit)
{
	struct drg_device *master = NULL;
	int old_state;

	/* update freq request when exiting idle or state change */
	list_for_each_entry(master, &freq_dev->master_list, node) {
		if (idle_exit)
			goto update;

		old_state = master->state;
		master->state = drg_freq2state(master->ruler,
					       master->divider, freq_dev->cur_freq);
		if (master->state == old_state)
			continue;

update:
		drg_update_rule_state(master->ruler, false);
	}
}

static void drg_idle_update_timer_func(struct timer_list *unused)
{
	drg_refresh_all_rule();
}

#ifdef CONFIG_LPCPU_MULTIDRV_CPUIDLE
static int drg_cpu_pm_notifier(struct notifier_block *nb, unsigned long action,
			       void *data)
{
	struct drg_freq_device *freq_dev = NULL;
	int cpu = smp_processor_id();
	bool trigger_cpufreq = false;

	if (!g_drg_initialized)
		goto out;

	freq_dev = per_cpu(cpufreq_dev, cpu);
	if (!freq_dev)
		goto out;

	/* only update master's idle state */
	if (list_empty(&freq_dev->master_list))
		goto out;

	spin_lock(&idle_state_lock);

	switch (action) {
	case CPU_PM_ENTER:
		if (!lpcpu_cluster_cpu_all_pwrdn())
			break;

		freq_dev->idle = true;
		mod_timer(&g_drg_idle_update_timer,
			  jiffies + msecs_to_jiffies(IDLE_UPDATE_SLACK_TIME));

		break;
	case CPU_PM_ENTER_FAILED:
	case CPU_PM_EXIT:
		if (freq_dev->idle) {
			trigger_cpufreq = true;
			freq_dev->idle = false;
		}

		del_timer(&g_drg_idle_update_timer);

		break;
	default:
		break;
	}

	spin_unlock(&idle_state_lock);

	if (trigger_cpufreq)
		drg_refresh_all_rule();

out:
	return NOTIFY_OK;
}

static struct notifier_block drg_cpu_pm_nb = {
	.notifier_call = drg_cpu_pm_notifier,
};
#endif

static void drg_freq_dev_release_work(struct kthread_work *work)
{
	struct drg_device *drg_dev = NULL;
	struct drg_device *tmp = NULL;
	struct drg_freq_device *freq_dev =
			container_of(work, struct drg_freq_device, destroy_work);

	drg_remove_freq_qos(freq_dev);

	list_for_each_entry_safe(drg_dev, tmp, &freq_dev->slave_list, node) {
		list_del(&drg_dev->node);
		drg_dev->freq_dev = NULL;
		drg_dev->state = INVALID_STATE;
	}

	list_for_each_entry_safe(drg_dev, tmp, &freq_dev->master_list, node) {
		list_del(&drg_dev->node);
		drg_dev->freq_dev = NULL;
		drg_dev->state = INVALID_STATE;
	}
}

static void _of_match_drg_rule(struct drg_freq_device *freq_dev,
			       struct drg_rule *ruler)
{
	struct drg_device *tmp_dev = NULL;
	int i;

	for (i = 0; i < ruler->nr_master; i++) {
		if (ruler->master[i].np == freq_dev->np) {
			tmp_dev = &ruler->master[i];
			tmp_dev->freq_dev = freq_dev;
			tmp_dev->state = drg_freq2state(ruler, tmp_dev->divider,
							freq_dev->cur_freq);
			list_add_tail(&tmp_dev->node, &freq_dev->master_list);

			drg_init_freq_qos(freq_dev, ruler);
			pr_debug("%s:match master%d %s\n",
				 ruler->name, i, freq_dev->np->full_name);
			return;
		}
	}

	for (i = 0; i < ruler->nr_slave; i++) {
		if (ruler->slave[i].np == freq_dev->np) {
			tmp_dev = &ruler->slave[i];
			tmp_dev->freq_dev = freq_dev;
			list_add_tail(&tmp_dev->node, &freq_dev->slave_list);

			drg_init_freq_qos(freq_dev, ruler);
			pr_debug("%s:match slave%d %s\n",
				 ruler->name, i, freq_dev->np->full_name);
			return;
		}
	}
}

static void of_match_drg_rule(struct drg_freq_device *freq_dev)
{
	struct drg_rule *ruler = NULL;

	list_for_each_entry(ruler, &g_drg_rule_list, node)
		_of_match_drg_rule(freq_dev, ruler);
}

static void of_match_drg_freq_device(struct drg_rule *ruler)
{
	struct drg_freq_device *freq_dev = NULL;

	list_for_each_entry(freq_dev, &g_drg_cpufreq_list, node)
		_of_match_drg_rule(freq_dev, ruler);

	list_for_each_entry(freq_dev, &g_drg_devfreq_list, node)
		_of_match_drg_rule(freq_dev, ruler);
}

static void drg_destroy_freq_dev(struct drg_freq_device *freq_dev)
{
	if (g_drg_initialized) {
		kthread_init_work(&freq_dev->destroy_work,
				  drg_freq_dev_release_work);
		kthread_queue_work(&g_drg_worker, &freq_dev->destroy_work);
		kthread_flush_work(&freq_dev->destroy_work);
	}

	kfree(freq_dev);
}

static int drg_cpufreq_notifier(struct notifier_block *nb,
				unsigned long event, void *data)
{
	struct cpufreq_freqs *freq = data;
	struct cpufreq_policy *policy = freq->policy;
	struct drg_freq_device *freq_dev = NULL;

	if (event != CPUFREQ_POSTCHANGE || !g_drg_initialized)
		return NOTIFY_DONE;

	freq_dev = drg_get_cpufreq_dev_by_policy(policy);
	if (!freq_dev)
		return NOTIFY_DONE;

	freq_dev->cur_freq = freq->new * freq_dev->amp / KHZ;
	/* return if it is not a master of any rule */
	if (list_empty(&freq_dev->master_list))
		return NOTIFY_DONE;

	drg_update_master_state(freq_dev, false);

	return NOTIFY_OK;
}

static struct notifier_block drg_cpufreq_notifier_nb = {
	.notifier_call = drg_cpufreq_notifier,
};

static int drg_devfreq_notifier(struct notifier_block *nb,
				unsigned long event, void *data)
{
	struct devfreq_freqs *freq = data;
	struct devfreq *devfreq = freq->devfreq;
	struct drg_freq_device *freq_dev = NULL;

	if (event != DEVFREQ_POSTCHANGE || !g_drg_initialized)
		return NOTIFY_DONE;

	freq_dev = drg_get_devfreq_dev(devfreq);
	if (!freq_dev)
		return NOTIFY_DONE;

	freq_dev->cur_freq = freq->new * freq_dev->amp / KHZ;
	/* return if it is not a master of any rule */
	if (list_empty(&freq_dev->master_list))
		return NOTIFY_DONE;

	drg_update_master_state(freq_dev, false);

	return NOTIFY_DONE;
}

static struct notifier_block drg_devfreq_notifier_nb = {
	.notifier_call = drg_devfreq_notifier,
};

static void drg_devfreq_register_notifier(struct drg_freq_device *freq_dev)
{
	/* listening to master devfreq freq transition */
	if (!list_empty(&freq_dev->master_list)) {
		int ret = devfreq_register_notifier(freq_dev->df, &drg_devfreq_notifier_nb,
						    DEVFREQ_TRANSITION_NOTIFIER);
		if (ret)
			pr_err("devfreq notify init for dev %s fail, ret=%d\n",
			       freq_dev->np->full_name, ret);
	}
}

static int gpucpu_freq_link_notify(struct notifier_block *nb,
				   unsigned long val, void *v)
{
	struct drg_rule *ruler = NULL;
	bool freq_link_enable = false;

	if (!g_drg_initialized)
		return NOTIFY_DONE;

	if (val == PERF_MODE_1)
		freq_link_enable = true;

	list_for_each_entry(ruler, &g_drg_rule_list, node) {
		if (ruler->user_mode_enable &&
		    ruler->enable != freq_link_enable) {
			ruler->enable = freq_link_enable;
			drg_update_rule_state(ruler, true);
		}
	}

	return NOTIFY_OK;
}

static struct notifier_block gpucpu_freq_link_notifier = {
	.notifier_call = gpucpu_freq_link_notify,
};

static void freq_link_notifier_init(void)
{
	(void)perf_mode_register_notifier(&gpucpu_freq_link_notifier);
}

/*
 * drg_cpufreq_register()
 * @policy: the cpufreq policy of the freq domain
 *
 * register drg freq device for the cpu policy, match it with related drg
 * device of all rules. drg_refresh_all_rule will be called to make sure no
 * violation of rule will happen after adding this device.
 *
 * Locking: this function will hold drg_list_lock during registration, cpu
 * cooling device should be registered after drg freq device registration.
 */
void drg_cpufreq_register(struct cpufreq_policy *policy)
{
	char name[30];
	struct drg_freq_device *freq_dev = NULL;
	int cpu, cluster;
	struct device_node *cn = NULL;

	if (IS_ERR_OR_NULL(policy)) {
		pr_err("%s:null cpufreq policy\n", __func__);
		return;
	}

	cpu = cpumask_first(policy->related_cpus);
	cluster = topology_physical_package_id(cpu);
	snprintf_s(name, sizeof(name), sizeof(name) - 1, "/cpus/cpu-map/cluster%d", cluster);
	cn = of_find_node_by_path(name);
	if (!cn) {
		pr_err("No Cluster%d information found\n", cluster);
		return;
	}

	mutex_lock(&drg_list_mutex);
	list_for_each_entry(freq_dev, &g_drg_cpufreq_list, node) {
		if (freq_dev->np == cn || freq_dev->policy == policy) {
			mutex_unlock(&drg_list_mutex);
			return;
		}
	}

	freq_dev = per_cpu(cpufreq_dev, cpu);
	if (!freq_dev) {
		freq_dev = (struct drg_freq_device *)
				kzalloc(sizeof(*freq_dev), GFP_ATOMIC);
		if (IS_ERR_OR_NULL(freq_dev)) {
			pr_err("alloc cpu%d freq dev err\n", policy->cpu);
			mutex_unlock(&drg_list_mutex);
			return;
		}

		freq_dev->policy = policy;
		freq_dev->np = cn;
		freq_dev->amp = CPUFREQ_AMP;
		freq_dev->cur_freq = policy->cur * CPUFREQ_AMP / KHZ;

		for_each_cpu(cpu, policy->related_cpus)
			per_cpu(cpufreq_dev, cpu) = freq_dev;
	}

	INIT_LIST_HEAD(&freq_dev->master_list);
	INIT_LIST_HEAD(&freq_dev->slave_list);
	list_add_tail(&freq_dev->node, &g_drg_cpufreq_list);

	if (g_drg_initialized)
		of_match_drg_rule(freq_dev);
	mutex_unlock(&drg_list_mutex);

	drg_refresh_all_rule();
}

/*
 * drg_cpufreq_unregister()
 * @policy: the cpufreq policy of the freq domain
 *
 * unregister drg freq device for the cpu policy, release the connection with
 * related drg device of all rules, and if this device is master of the rule,
 * update this rule.
 *
 * Locking: this function will hold drg_list_lock during unregistration, cpu
 * cooling device should be unregistered before drg freq device unregistration.
 * the caller may hold policy->lock, because there may be other work in worker
 * thread wait for policy->lock, so we can't put destroy_work to work thread
 * and wait for completion of the work, and drg_rule_update_work need hold
 * hotplug lock to protect against drg_cpufreq_unregister during hotplug.
 */
void drg_cpufreq_unregister(struct cpufreq_policy *policy)
{
	struct drg_freq_device *freq_dev = NULL;
	bool found = false;

	if (IS_ERR_OR_NULL(policy)) {
		pr_err("%s:null cpu policy\n", __func__);
		return;
	}

	mutex_lock(&drg_list_mutex);
	list_for_each_entry(freq_dev, &g_drg_cpufreq_list, node) {
		if (freq_dev->policy == policy) {
			found = true;
			list_del_init(&freq_dev->node);
			break;
		}
	}

	if (found)
		drg_freq_dev_release_work(&freq_dev->destroy_work);

	mutex_unlock(&drg_list_mutex);
}

/*
 * drg_devfreq_register()
 * @df: devfreq device
 *
 * register drg freq device for the devfreq device, match it with related drg
 * device of all rules. drg_refresh_all_rule will be called to make sure no
 * violation of rule will happen after adding this device.
 *
 * Locking: this function will hold drg_list_lock during registration, devfreq
 * cooling device should be registered after drg freq device registration.
 */
void drg_devfreq_register(struct devfreq *df)
{
	struct drg_freq_device *freq_dev = NULL;
	struct device *dev = NULL;

	if (IS_ERR_OR_NULL(df)) {
		pr_err("%s:null devfreq\n", __func__);
		return;
	}

	dev = df->dev.parent;
	if (IS_ERR_OR_NULL(dev)) {
		pr_err("%s:null devfreq parent dev\n", __func__);
		return;
	}

	mutex_lock(&drg_list_mutex);
	list_for_each_entry(freq_dev, &g_drg_devfreq_list, node) {
		if (freq_dev->df == df || freq_dev->np == dev->of_node) {
			mutex_unlock(&drg_list_mutex);
			return;
		}
	}

	freq_dev = (struct drg_freq_device *)
			kzalloc(sizeof(*freq_dev), GFP_ATOMIC);
	if (IS_ERR_OR_NULL(freq_dev)) {
		mutex_unlock(&drg_list_mutex);
		pr_err("alloc freq dev err\n");
		return;
	}

	freq_dev->df = df;
	freq_dev->np = dev->of_node;
	freq_dev->amp = DEVFREQ_AMP;
	freq_dev->cur_freq = df->previous_freq * DEVFREQ_AMP / KHZ;
	INIT_LIST_HEAD(&freq_dev->master_list);
	INIT_LIST_HEAD(&freq_dev->slave_list);

	list_add_tail(&freq_dev->node, &g_drg_devfreq_list);
	if (g_drg_initialized) {
		of_match_drg_rule(freq_dev);
		drg_devfreq_register_notifier(freq_dev);
	}

	mutex_unlock(&drg_list_mutex);

	drg_refresh_all_rule();
}
EXPORT_SYMBOL(drg_devfreq_register);

/*
 * drg_devfreq_unregister()
 * @df: devfreq device
 *
 * unregister drg freq device for devfreq device, release the connection with
 * related drg device of all rules, and if this device is master of the rule,
 * update this rule.
 *
 * Locking: this function will hold drg_list_lock during unregistration,
 * cooling device should be unregistered before drg freq device unregistration.
 * we will put destroy_work to worker thread and wait for completion of the work
 * to protect against drg_rule_update_work.
 */
void drg_devfreq_unregister(struct devfreq *df)
{
	struct drg_freq_device *freq_dev = NULL;
	bool found = false;

	if (IS_ERR_OR_NULL(df)) {
		pr_err("%s:null devfreq\n", __func__);
		return;
	}

	/*
	 * prevent devfreq scaling freq when unregister freq device,
	 * cpufreq itself can prevent this from happening
	 */
	mutex_lock(&df->lock);
	mutex_lock(&drg_list_mutex);

	list_for_each_entry(freq_dev, &g_drg_devfreq_list, node) {
		if (freq_dev->df == df) {
			found = true;
			list_del(&freq_dev->node);
			break;
		}
	}

	mutex_unlock(&drg_list_mutex);
	mutex_unlock(&df->lock);

	if (found) {
		if (!list_empty(&freq_dev->master_list))
			WARN_ON(devfreq_unregister_notifier(df, &drg_devfreq_notifier_nb,
					DEVFREQ_TRANSITION_NOTIFIER));

		drg_destroy_freq_dev(freq_dev);
	}
}
EXPORT_SYMBOL(drg_devfreq_unregister);

static ssize_t show_drg_device(struct drg_device *drg_dev, char *buf,
			       ssize_t count)
{
	int i;

	if (!drg_dev)
		return count;

	count += scnprintf(buf + count, PAGE_SIZE - count,
			   "\t%s:\n\t\tdivider:", drg_dev->np->full_name);

	for (i = 0; i < drg_dev->ruler->divider_num; i++)
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   " %u", drg_dev->divider[i]);
	count += scnprintf(buf + count, PAGE_SIZE - count, "\n");

	if (!drg_dev->freq_dev)
		return count;

	count += scnprintf(buf + count, PAGE_SIZE - count,
			   "\t\tstate: %d\n\t\tidle: %d\n",
			   drg_dev->state, drg_dev->freq_dev->idle);

	return count;
}

static ssize_t show_rule_state(const struct drg_rule *ruler, char *buf)
{
	ssize_t count = 0;
	struct drg_device *master = NULL;
	struct drg_device *slave = NULL;
	int i, j;

	if (!g_drg_initialized)
		return count;

	count += scnprintf(buf + count, PAGE_SIZE - count,
			   "up limit: %d\n", ruler->up_limit);
	count += scnprintf(buf + count, PAGE_SIZE - count,
			   "dn limit: %d\n", ruler->dn_limit);

	count += scnprintf(buf + count, PAGE_SIZE - count, "master:\n");
	for (i = 0; i < ruler->nr_master; i++)
		count = show_drg_device(&ruler->master[i], buf, count);

	count += scnprintf(buf + count, PAGE_SIZE - count, "slave:\n");
	for (i = 0; i < ruler->nr_slave; i++)
		count = show_drg_device(&ruler->slave[i], buf, count);

	count += scnprintf(buf + count, PAGE_SIZE - count, "\t");
	for (i = 0; i < ruler->nr_slave; i++) {
		slave = &ruler->slave[i];
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "\ts%d[%s]", i,
				   slave ? slave->np->full_name : "NULL");
	}
	count += scnprintf(buf + count, PAGE_SIZE - count, "\n");

	for (i = 0; i < ruler->nr_master; i++) {
		unsigned int value = 0;

		master = &ruler->master[i];
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "m%d[%s]\t", i,
				   master ? master->np->full_name : "NULL");

		for (j = 0; j < ruler->nr_slave; j++) {
			slave = &ruler->slave[j];
			if (!slave || !slave->freq_dev) {
				count += scnprintf(buf + count, PAGE_SIZE - count,
						   "NA\t");
				continue;
			}

			if (slave->freq_dev->policy) {
				if (ruler->up_limit)
					value = master->max_freq[j]->req.cpufreq.pnode.prio;
				if (ruler->dn_limit)
					value = master->min_freq[j]->req.cpufreq.pnode.prio;
			} else if (slave->freq_dev->df) {
				if (ruler->up_limit)
					value = master->max_freq[j]->req.devfreq.data.pnode.prio;
				if (ruler->dn_limit)
					value = master->min_freq[j]->req.devfreq.data.pnode.prio;
			}

			count += scnprintf(buf + count, PAGE_SIZE - count,
					   "%u\t", value);
		}
		count += scnprintf(buf + count, PAGE_SIZE - count, "\n");
	}

	return count;
}

static ssize_t store_enable(struct drg_rule *ruler __maybe_unused,
			    const char *buf __maybe_unused, size_t count)
{

	return count;
}

static ssize_t show_enable(const struct drg_rule *ruler, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", ruler->enable ? 1 : 0);
}

struct drg_attr {
	struct attribute attr;
	ssize_t (*show)(const struct drg_rule *, char *);
	ssize_t (*store)(struct drg_rule *, const char *, size_t count);
};

#define drg_attr_ro(_name) \
static struct drg_attr _name = __ATTR(_name, 0440, show_##_name, NULL)

#define drg_attr_rw(_name) \
static struct drg_attr _name = __ATTR(_name, 0640, show_##_name, store_##_name)

drg_attr_ro(rule_state);
drg_attr_rw(enable);

static struct attribute *default_attrs[] = {
	&rule_state.attr,
	&enable.attr,
	NULL
};

#define to_drg_rule(k) container_of(k, struct drg_rule, kobj)
#define to_attr(a) container_of(a, struct drg_attr, attr)

static ssize_t show(struct kobject *kobj, struct attribute *attr, char *buf)
{
	struct drg_rule *data = to_drg_rule(kobj);
	struct drg_attr *cattr = to_attr(attr);
	ssize_t ret = -EIO;

	if (cattr->show != NULL)
		ret = cattr->show(data, buf);

	return ret;
}

static ssize_t store(struct kobject *kobj, struct attribute *attr,
		     const char *buf, size_t count)
{
	struct drg_rule *data = to_drg_rule(kobj);
	struct drg_attr *cattr = to_attr(attr);
	ssize_t ret = -EIO;

	if (cattr->store != NULL)
		ret = cattr->store(data, buf, count);

	return ret;
}

static const struct sysfs_ops sysfs_ops = {
	.show = show,
	.store = store,
};

static struct kobj_type ktype_drg = {
	.sysfs_ops = &sysfs_ops,
	.default_attrs = default_attrs,
};

static int drg_device_init(struct device *dev, struct drg_device **drg_dev,
			   int *dev_num, struct drg_rule *ruler,
			   struct device_node *np, const char *prop)
{
	struct of_phandle_args drg_dev_spec;
	struct drg_device *t_drg_dev = NULL;
	int divider_num = ruler->divider_num;
	unsigned int divider_size;
	int i, ret;

	i = of_property_count_u32_elems(np, prop);
	if (i <= 0 || (i % (divider_num + 1))) {
		dev_err(dev, "count %s err:%d\n", prop, i);
		return -ENOENT;
	}

	*dev_num = i / (divider_num + 1);

	*drg_dev = (struct drg_device *)
			devm_kzalloc(dev, sizeof(**drg_dev) * (*dev_num),
				     GFP_KERNEL);
	if (IS_ERR_OR_NULL(*drg_dev)) {
		dev_err(dev, "alloc %s fail\n", prop);
		return -ENOMEM;
	}

	divider_size = sizeof(*(*drg_dev)->divider) * (unsigned int)divider_num;
	if (divider_size > sizeof(drg_dev_spec.args)) {
		dev_err(dev, "divider size too big:%u\n", divider_size);
		return -ENOMEM;
	}

	for (i = 0; i < *dev_num; i++) {
		t_drg_dev = &(*drg_dev)[i];
		t_drg_dev->divider = (unsigned int *)
					devm_kzalloc(dev,
						     divider_size, GFP_KERNEL);
		if (IS_ERR_OR_NULL(t_drg_dev->divider)) {
			dev_err(dev, "alloc %s%d fail\n", prop, i);
			return -ENOMEM;
		}

		if (of_parse_phandle_with_fixed_args(np, prop, divider_num,
						     i, &drg_dev_spec)) {
			dev_err(dev, "parse %s%d err\n", prop, i);
			return -ENOENT;
		}

		t_drg_dev->np = drg_dev_spec.np;
		t_drg_dev->ruler = ruler;
		t_drg_dev->state = INVALID_STATE;
		ret = memcpy_s(t_drg_dev->divider, divider_size,
			       drg_dev_spec.args, divider_size);
		if (ret != EOK) {
			dev_err(dev, "copy err:%d\n", ret);
			return -ret;
		}
	}

	return 0;
}

static void drg_init_ruler_work(struct drg_rule *ruler)
{
	kthread_init_work(&ruler->exec_work, drg_rule_update_work);
}

int perf_ctrl_get_drg_dev_freq(void __user *uarg)
{
	return 0;
}

static int drg_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *child = NULL;
	struct drg_rule *ruler = NULL;
	struct drg_freq_device *freq_dev = NULL;
	int ret;

	for_each_child_of_node(np, child) {
		if (of_property_read_bool(child, "rule_disable"))
			continue;

		ruler = (struct drg_rule *)
				devm_kzalloc(dev, sizeof(*ruler),
					     GFP_KERNEL);
		if (IS_ERR_OR_NULL(ruler)) {
			dev_err(dev, "alloc ruler fail\n");
			ret = -ENOMEM;
			goto err_probe;
		}

		if (of_property_read_u32(child, "divider-cells",
					 &ruler->divider_num)) {
			dev_err(dev, "read divider cells fail\n");
			ret = -ENOENT;
			goto err_probe;
		}

		ruler->up_limit =
			of_property_read_bool(child, "up_limit_enable");
		ruler->dn_limit =
			of_property_read_bool(child, "down_limit_enable");
		ruler->user_mode_enable =
			of_property_read_bool(child, "user_mode_enable");

		ret = drg_device_init(dev, &ruler->master,
				      &ruler->nr_master, ruler,
				      child, "drg-master");
		if (ret)
			goto err_probe;

		ret = drg_device_init(dev, &ruler->slave,
				      &ruler->nr_slave, ruler,
				      child, "drg-slave");
		if (ret)
			goto err_probe;

		ret = drg_alloc_freq_qos(dev, ruler);
		if (ret)
			goto err_probe;

		ruler->name = child->name;
		if (!ruler->user_mode_enable)
			ruler->enable = true;

		ret = kobject_init_and_add(&ruler->kobj, &ktype_drg,
					   &dev->kobj, "%s", ruler->name);
		if (ret) {
			dev_err(dev, "create kobj err %d\n", ret);
			goto err_probe;
		}

		drg_init_ruler_work(ruler);

		list_add_tail(&ruler->node, &g_drg_rule_list);
	}

	timer_setup(&g_drg_idle_update_timer,
		    drg_idle_update_timer_func, TIMER_DEFERRABLE);

#ifdef CONFIG_LPCPU_MULTIDRV_CPUIDLE
	ret = cpu_pm_register_notifier(&drg_cpu_pm_nb);
	if (ret) {
		dev_err(dev, "register cpu pm notifier fail\n");
		goto err_probe;
	}
#endif

	ret = cpufreq_register_notifier(&drg_cpufreq_notifier_nb,
					CPUFREQ_TRANSITION_NOTIFIER);
	if (ret) {
		dev_err(dev, "register cpufreq notifier fail\n");
		goto err_probe;
	}

	g_drg_work_thread = kthread_run(kthread_worker_fn,
					(void *)&g_drg_worker, "drg");
	if (IS_ERR(g_drg_work_thread)) {
		dev_err(dev, "create drg thread fail\n");
		ret = PTR_ERR(g_drg_work_thread);
		goto err_probe;
	}

	mutex_lock(&drg_list_mutex);

	list_for_each_entry(ruler, &g_drg_rule_list, node)
		of_match_drg_freq_device(ruler);

	list_for_each_entry(freq_dev, &g_drg_devfreq_list, node)
		drg_devfreq_register_notifier(freq_dev);

	g_drg_initialized = true;

	mutex_unlock(&drg_list_mutex);

	drg_refresh_all_rule();

	freq_link_notifier_init();

	return 0;

err_probe:
	list_for_each_entry(ruler, &g_drg_rule_list, node)
		kobject_del(&ruler->kobj);

	INIT_LIST_HEAD(&g_drg_rule_list);

	return ret;
}

static const struct of_device_id drg_of_match[] = {
	{
		.compatible = "dual-rail-gov",
	},
	{ } /* end */
};

static struct platform_driver drg_driver = {
	.probe = drg_probe,
	.driver = {
		.name = "drg",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(drg_of_match),
	},
};

module_platform_driver(drg_driver);
