/*
 * hotplug_agent.c
 *
 * execute hotplug according to offline mask setting
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#include <linux/io.h>
#include <linux/cpumask.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/topology.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/sysfs.h>
#include <linux/ktime.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <linux/suspend.h>
#include <linux/sched/task.h>
#include <uapi/linux/sched/types.h>
#include <securec.h>
#include <linux/of_platform.h>

struct work_binder {
	kthread_work_func_t func;
	struct device *dev;
};

struct ha_data {
	struct kthread_work work;
	cpumask_t off_mask;
	struct work_binder *binder;
	void __iomem *base;
	ktime_t ts_start;
#ifdef CONFIG_HOTPLUG_AGENT_DEBUG
	s64 max_duration_ms;
#endif
	int irq;
	bool bypass_io;
};

static DEFINE_KTHREAD_WORKER(hotplug_agent_worker);
static DECLARE_COMPLETION(in_block);
static DEFINE_SPINLOCK(suspend_lock);
static struct task_struct *worker_task;
static bool suspended;

static void block_work_func(struct kthread_work *work)
{
	/* notify all works before blocking have been done */
	complete(&in_block);
	set_current_state(TASK_INTERRUPTIBLE);
	while (suspended) {
		schedule();
		if (kthread_should_stop())
			return;
	}
	set_current_state(TASK_RUNNING);
	pr_info("Resume unblock hotplug agent\n");
}

static DEFINE_KTHREAD_WORK(block_work, block_work_func);

static int suspend_callback(struct notifier_block *nb,
			    unsigned long action, void *ptr)
{
	unsigned long flags;

	if (action == PM_SUSPEND_PREPARE ||
	    action == PM_HIBERNATION_PREPARE) {
		/* lock protect no work will be inserted after block work */
		spin_lock_irqsave(&suspend_lock, flags);
		suspended = true;
		kthread_queue_work(&hotplug_agent_worker, &block_work);
		spin_unlock_irqrestore(&suspend_lock, flags);

		/* ensure no other work left in queue */
		wait_for_completion(&in_block);
		pr_info("Suspend block hotplug agent\n");
	}

	return NOTIFY_OK;
}

static int resume_callback(struct notifier_block *nb,
			    unsigned long action, void *ptr)
{
	if (action == PM_POST_SUSPEND ||
	    action == PM_POST_HIBERNATION) {
		suspended = false;
		wake_up_process(worker_task);
	}

	return NOTIFY_OK;
}

static struct notifier_block suspend_nb = {
	.notifier_call = suspend_callback,
	/* the priority of suspend callback must be higher
	 * than cpu callback which disable hotplug
	 */
	.priority = 1,
};

static struct notifier_block resume_nb = {
	.notifier_call = resume_callback,
	/* the priority of resume callback must be lower
	 * than cpu callback which enable hotplug
	 */
	.priority = -1,
};

static inline void hotplug_agent_queue_work(struct ha_data *pdata)
{
	unsigned long flags;

	spin_lock_irqsave(&suspend_lock, flags);

	/* work will not be done in suspend, drop it */
	if (suspended) {
		spin_unlock_irqrestore(&suspend_lock, flags);
		return;
	}

	pdata->ts_start = ktime_get();
	kthread_queue_work(&hotplug_agent_worker, &pdata->work);

	spin_unlock_irqrestore(&suspend_lock, flags);
}

static inline s64 hotplug_agent_get_duration(struct ha_data *pdata)
{
	return ktime_ms_delta(ktime_get(), pdata->ts_start);
}

static void hotplug_agent_update_duration(struct ha_data *pdata __maybe_unused)
{
#ifdef CONFIG_HOTPLUG_AGENT_DEBUG
	s64 duration_ms = hotplug_agent_get_duration(pdata);

	if (duration_ms > pdata->max_duration_ms)
		pdata->max_duration_ms = duration_ms;
#endif
}

#ifdef CONFIG_HOTPLUG_AGENT_DEBUG
static ssize_t offline_mask_show(struct device *dev,
				 struct device_attribute *attr __maybe_unused,
				 char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ha_data *pdata = platform_get_drvdata(pdev);

	return scnprintf(buf, PAGE_SIZE, "%x\n",
			 cpumask_bits(&pdata->off_mask)[0]);
}

static ssize_t offline_mask_store(struct device *dev,
				  struct device_attribute *attr __maybe_unused,
				  const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ha_data *pdata = platform_get_drvdata(pdev);
	unsigned int value;

	if (kstrtouint(buf, 0, &value) != 0)
		return -EINVAL;

	pdata->bypass_io = true;
	cpumask_bits(&pdata->off_mask)[0] = value;
	hotplug_agent_queue_work(pdata);
	kthread_flush_work(&pdata->work);
	pdata->bypass_io = (value != 0);

	return count;
}

static DEVICE_ATTR(offline_mask, 0640, offline_mask_show, offline_mask_store);

static ssize_t max_duration_show(struct device *dev,
				 struct device_attribute *attr __maybe_unused,
				 char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ha_data *pdata = platform_get_drvdata(pdev);

	return scnprintf(buf, PAGE_SIZE, "%lld\n", pdata->max_duration_ms);
}

static ssize_t max_duration_store(struct device *dev,
				  struct device_attribute *attr __maybe_unused,
				  const char *buf __maybe_unused, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ha_data *pdata = platform_get_drvdata(pdev);

	pdata->max_duration_ms = 0;

	return count;
}

static DEVICE_ATTR(max_duration, 0640, max_duration_show, max_duration_store);

static const struct attribute *hotplug_agent_attrs[] = {
	&dev_attr_offline_mask.attr,
	&dev_attr_max_duration.attr,
	NULL,
};
#endif

static irqreturn_t hotplug_agent_irq_handler(int irq __maybe_unused,
					     void *dev_id)
{
	struct ha_data *pdata = (struct ha_data *)dev_id;

	hotplug_agent_queue_work(pdata);

	return IRQ_HANDLED;
}

static void __remove_cpu(unsigned int cpu)
{
	struct device *cpu_dev = get_cpu_device(cpu);
	int ret;

	if (cpu_dev == NULL) {
		pr_err("%s:%u, no device\n", __func__, cpu);
		return;
	}

	ret = device_offline(cpu_dev);
#ifdef CONFIG_HOTPLUG_AGENT_DEBUG
	pr_info("hotplug agent remove cpu%u, %d\n", cpu, ret);
	if (ret == 1)
		panic("device offline status unmatch\n");
#else
	if (ret != 0)
		pr_err("hotplug agent failed to remove cpu%u, %d\n", cpu, ret);
#endif
}

static void __add_cpu(unsigned int cpu)
{
	struct device *cpu_dev = get_cpu_device(cpu);
	int ret;

	if (cpu_dev == NULL) {
		pr_err("%s:%u, no device\n", __func__, cpu);
		return;
	}

	ret = device_online(cpu_dev);
#ifdef CONFIG_HOTPLUG_AGENT_DEBUG
	pr_info("hotplug agent add cpu%u, %d\n", cpu, ret);
	if (ret == 1)
		panic("device online status unmatch\n");
#else
	if (ret != 0)
		pr_err("hotplug agent failed to add cpu%u, %d\n", cpu, ret);
#endif
}

static void cpu_hotplug_work(struct kthread_work *work)
{
	struct ha_data *pdata = container_of(work, struct ha_data, work);
	cpumask_t *off_mask = &pdata->off_mask;
	int cpu;

	/* protect against cpu on/off request from sysfs */
	lock_device_hotplug();

	if (pdata->base && !pdata->bypass_io)
		cpumask_bits(off_mask)[0] = readl(pdata->base);

	for_each_cpu_not(cpu, cpu_online_mask) {
		if (cpumask_test_cpu(cpu, off_mask) == 0)
			__add_cpu(cpu);
	}

	for_each_online_cpu(cpu) {
		if (cpumask_test_cpu(cpu, off_mask) != 0)
			__remove_cpu(cpu);
	}

	unlock_device_hotplug();

	hotplug_agent_update_duration(pdata);
}

static struct work_binder cpu_hotplug_binder = {
	.func = cpu_hotplug_work,
};

static const struct of_device_id hotplug_agent_of_match[] = {
	{
		.compatible = "cpu-hotplug-agent",
		.data = (void *)&cpu_hotplug_binder,
	},
	{},
};

static struct work_binder *hotplug_agent_bind_work(struct device *dev)
{
	const struct of_device_id *match = NULL;

	match = of_match_device(hotplug_agent_of_match, dev);
	if (match == NULL)
		return NULL;

	return (struct work_binder *)match->data;
}

static int hotplug_agent_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ha_data *pdata = NULL;
	struct resource *res = NULL;
	int ret = 0;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (pdata == NULL) {
		dev_err(dev, "Failed to alloc pdata for hotplug agent!\n");
		return -ENOMEM;
	}

	pdata->binder = hotplug_agent_bind_work(dev);
	if (pdata->binder == NULL) {
		dev_err(dev, "Failed to match work for hotplug agent!\n");
		return -EINVAL;
	}

	kthread_init_work(&pdata->work, pdata->binder->func);

	/* Get the base address */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res != NULL) {
		pdata->base = devm_ioremap_resource(dev, res);
		if (IS_ERR(pdata->base)) {
			dev_err(dev, "Failed to ioremap!\n");
			return PTR_ERR(pdata->base);
		}
	} else {
		dev_err(dev, "no MEM resource.\n");
	}

	/* Get the IRQ */
	pdata->irq = platform_get_irq(pdev, 0);
	if (pdata->irq > 0) {
		ret = devm_request_irq(dev, pdata->irq,
				       hotplug_agent_irq_handler,
				       IRQF_SHARED, NULL, pdata);
		if (ret != 0) {
			dev_err(dev, "Failed to request irq!\n");
			return ret;
		}
	} else {
		dev_err(dev, "no IRQ resource.\n");
	}

	platform_set_drvdata(pdev, pdata);
	pdata->binder->dev = dev;

#ifdef CONFIG_HOTPLUG_AGENT_DEBUG
	ret = sysfs_create_files(&dev->kobj, hotplug_agent_attrs);
#endif
	return ret;
}

static int hotplug_agent_remove(struct platform_device *pdev)
{
	struct ha_data *pdata = platform_get_drvdata(pdev);

#ifdef CONFIG_HOTPLUG_AGENT_DEBUG
	sysfs_remove_files(&pdev->dev.kobj, hotplug_agent_attrs);
#endif
	if (pdata->irq > 0)
		disable_irq(pdata->irq);

	pdata->binder->dev = NULL;

	(void)kthread_cancel_work_sync(&pdata->work);

	return 0;
}

MODULE_DEVICE_TABLE(of, hotplug_agent_of_match);

static struct platform_driver hotplug_agent_driver = {
	.probe = hotplug_agent_probe,
	.remove = hotplug_agent_remove,
	.driver = {
		.name = "hotplug_agent",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hotplug_agent_of_match),
	},
};

static int __init hotplug_agent_init(void)
{
	struct sched_param param = { .sched_priority = MAX_RT_PRIO - 1 };
	int ret;

	worker_task = kthread_create(kthread_worker_fn,
				     &hotplug_agent_worker, "hotplug_agent");
	if (IS_ERR(worker_task))
		return PTR_ERR(worker_task);

	sched_setscheduler_nocheck(worker_task, SCHED_FIFO, &param);
	/*
	 * make sure irq handler will never access illegal space
	 * when thread dead unexpectedly
	 */
	get_task_struct(worker_task);
	wake_up_process(worker_task);
	ret = register_pm_notifier(&suspend_nb);
	if (ret)
		pr_warn("Failed to register suspend notifier %d\n", ret);
	ret = register_pm_notifier(&resume_nb);
	if (ret)
		pr_warn("Failed to register resume notifier %d\n", ret);

	return platform_driver_register(&hotplug_agent_driver);
}

static void __exit hotplug_agent_exit(void)
{
	platform_driver_unregister(&hotplug_agent_driver);
	(void)unregister_pm_notifier(&suspend_nb);
	(void)unregister_pm_notifier(&resume_nb);
	kthread_stop(worker_task);
	put_task_struct(worker_task);
}

module_init(hotplug_agent_init);
module_exit(hotplug_agent_exit);

MODULE_DESCRIPTION("hotplug agent");
MODULE_LICENSE("GPL v2");

