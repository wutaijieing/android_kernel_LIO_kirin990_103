/*
 * platform_qos.c
 *
 * platform qos module
 *
 * Copyright (c) 2021-2022 Huawei Technologies Co., Ltd.
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
#include <linux/platform_drivers/platform_qos.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>

#include <linux/uaccess.h>
#include <linux/export.h>
#include <securec.h>

#define PLATFORM_QOS_ONSYS_AVS_LEVEL_DEFAULT_VALUE 1
#define PLATFORM_QOS_DEFAULT_VALUE    (-1)
#ifdef CONFIG_DEVFREQ_GOV_PLATFORM_QOS
#define PLATFORM_QOS_MEMORY_LATENCY_DEFAULT_VALUE	0
#define PLATFORM_QOS_MEMORY_THROUGHPUT_DEFAULT_VALUE	0
#define PLATFORM_QOS_MEMORY_THROUGHPUT_UP_THRESHOLD_DEFAULT_VALUE	50000
#endif
#ifdef CONFIG_DEVFREQ_L1BUS_LATENCY_PLATFORM
#define PLATFORM_QOS_L1BUS_LATENCY_DEFAULT_VALUE	0
#endif

/*
 * locking rule: all changes to constraints or notifiers lists
 * or platform_qos_object list and platform_qos_objects need to happen
 * with platform_qos_lock held, taken with _irqsave.  One lock to rule them all
 */
struct platform_qos_object {
	struct platform_qos_constraints *constraints;
	struct miscdevice platform_qos_power_miscdev;
	char *name;
};

static DEFINE_SPINLOCK(platform_qos_lock);

static struct platform_qos_object null_platform_qos;

#ifdef CONFIG_ONSYS_AVS
static BLOCKING_NOTIFIER_HEAD(onsys_avs_ctrl_notifier);
static struct platform_qos_constraints onsys_avs_ctrl_constraints = {
	.list = PLIST_HEAD_INIT(onsys_avs_ctrl_constraints.list),
	.target_value = PLATFORM_QOS_ONSYS_AVS_LEVEL_DEFAULT_VALUE,
	.default_value = PLATFORM_QOS_ONSYS_AVS_LEVEL_DEFAULT_VALUE,
	.no_constraint_value = PLATFORM_QOS_ONSYS_AVS_LEVEL_DEFAULT_VALUE,
	.type = PLATFORM_QOS_MIN,
	.notifiers = &onsys_avs_ctrl_notifier,
};

static struct platform_qos_object onsys_avs_ctrl_level_onsys_qos = {
	.constraints = &onsys_avs_ctrl_constraints,
	.name = "onsys_avs_ctrl_level",
};
#endif

#ifdef CONFIG_DEVFREQ_GOV_PLATFORM_QOS
static BLOCKING_NOTIFIER_HEAD(memory_latency_notifier);
static struct platform_qos_constraints memory_latency_constraints = {
	.list = PLIST_HEAD_INIT(memory_latency_constraints.list),
	.target_value = PLATFORM_QOS_MEMORY_LATENCY_DEFAULT_VALUE,
	.default_value = PLATFORM_QOS_MEMORY_LATENCY_DEFAULT_VALUE,
	.no_constraint_value = PLATFORM_QOS_MEMORY_LATENCY_DEFAULT_VALUE,
	.type = PLATFORM_QOS_MAX,
	.notifiers = &memory_latency_notifier,
};
static struct platform_qos_object memory_latency_platform_qos = {
	.constraints = &memory_latency_constraints,
	.name = "memory_latency",
};

static BLOCKING_NOTIFIER_HEAD(memory_throughput_notifier);
static struct platform_qos_constraints memory_tput_constraints = {
	.list = PLIST_HEAD_INIT(memory_tput_constraints.list),
	.target_value = PLATFORM_QOS_MEMORY_THROUGHPUT_DEFAULT_VALUE,
	.default_value = PLATFORM_QOS_MEMORY_THROUGHPUT_DEFAULT_VALUE,
	.no_constraint_value = PLATFORM_QOS_MEMORY_THROUGHPUT_DEFAULT_VALUE,
	.type = PLATFORM_QOS_SUM,
	.notifiers = &memory_throughput_notifier,
};
static struct platform_qos_object memory_throughput_platform_qos = {
	.constraints = &memory_tput_constraints,
	.name = "memory_throughput",
};

static BLOCKING_NOTIFIER_HEAD(memory_throughput_up_threshold_notifier);
static struct platform_qos_constraints memory_tput_up_th_constraints = {
	.list = PLIST_HEAD_INIT(memory_tput_up_th_constraints.list),
	.target_value = PLATFORM_QOS_MEMORY_THROUGHPUT_UP_THRESHOLD_DEFAULT_VALUE,
	.default_value = PLATFORM_QOS_MEMORY_THROUGHPUT_UP_THRESHOLD_DEFAULT_VALUE,
	.no_constraint_value = PLATFORM_QOS_MEMORY_THROUGHPUT_UP_THRESHOLD_DEFAULT_VALUE,
	.type = PLATFORM_QOS_MIN,
	.notifiers = &memory_throughput_up_threshold_notifier,
};
static struct platform_qos_object memory_throughput_up_th_platform_qos = {
	.constraints = &memory_tput_up_th_constraints,
	.name = "memory_throughput_up_threshold",
};
#endif

#ifdef CONFIG_DEVFREQ_L1BUS_LATENCY_PLATFORM
static BLOCKING_NOTIFIER_HEAD(l1bus_latency_notifier);
static struct platform_qos_constraints l1bus_latency_constraints = {
	.list = PLIST_HEAD_INIT(l1bus_latency_constraints.list),
	.target_value = PLATFORM_QOS_L1BUS_LATENCY_DEFAULT_VALUE,
	.default_value = PLATFORM_QOS_L1BUS_LATENCY_DEFAULT_VALUE,
	.no_constraint_value = PLATFORM_QOS_L1BUS_LATENCY_DEFAULT_VALUE,
	.type = PLATFORM_QOS_MAX,
	.notifiers = &l1bus_latency_notifier,
};
static struct platform_qos_object l1bus_latency_platform_qos = {
	.constraints = &l1bus_latency_constraints,
	.name = "l1bus_latency",
};
#endif

#ifdef CONFIG_GPUTOP_FREQ
static BLOCKING_NOTIFIER_HEAD(gputop_freq_dnlimit_notifier);
static struct platform_qos_constraints gputop_freq_dnlimit_constraints = {
	.list = PLIST_HEAD_INIT(gputop_freq_dnlimit_constraints.list),
	.target_value = PLATFORM_QOS_GPUTOP_FREQ_DNLIMIT_DEFAULT_VALUE,
	.default_value = PLATFORM_QOS_GPUTOP_FREQ_DNLIMIT_DEFAULT_VALUE,
	.no_constraint_value = PLATFORM_QOS_GPUTOP_FREQ_DNLIMIT_DEFAULT_VALUE,
	.type = PLATFORM_QOS_MAX,
	.notifiers = &gputop_freq_dnlimit_notifier,
};
static struct platform_qos_object gputop_freq_dnlimit_platform_qos = {
	.constraints = &gputop_freq_dnlimit_constraints,
	.name = "gputop_freq_dnlimit",
};
static BLOCKING_NOTIFIER_HEAD(gputop_freq_uplimit_notifier);
static struct platform_qos_constraints gputop_freq_uplimit_constraints = {
	.list = PLIST_HEAD_INIT(gputop_freq_uplimit_constraints.list),
	.target_value = PLATFORM_QOS_GPUTOP_FREQ_UPLIMIT_DEFAULT_VALUE,
	.default_value = PLATFORM_QOS_GPUTOP_FREQ_UPLIMIT_DEFAULT_VALUE,
	.no_constraint_value = PLATFORM_QOS_GPUTOP_FREQ_UPLIMIT_DEFAULT_VALUE,
	.type = PLATFORM_QOS_MIN,
	.notifiers = &gputop_freq_uplimit_notifier,
};
static struct platform_qos_object gputop_freq_uplimit_platform_qos = {
	.constraints = &gputop_freq_uplimit_constraints,
	.name = "gputop_freq_uplimit",
};
#endif
#ifdef CONFIG_NPUFREQ_PLATFORM_QOS
static BLOCKING_NOTIFIER_HEAD(npu_pm_freq_dnlimit_notifier);
static struct platform_qos_constraints npu_pm_freq_dnlimit_constraints = {
	.list = PLIST_HEAD_INIT(npu_pm_freq_dnlimit_constraints.list),
	.target_value = PLATFORM_QOS_NPU_FREQ_DNLIMIT_DEFAULT_VALUE,
	.default_value = PLATFORM_QOS_NPU_FREQ_DNLIMIT_DEFAULT_VALUE,
	.no_constraint_value = PLATFORM_QOS_NPU_FREQ_DNLIMIT_DEFAULT_VALUE,
	.type = PLATFORM_QOS_MAX,
	.notifiers = &npu_pm_freq_dnlimit_notifier,
};
static struct platform_qos_object npu_pm_freq_dnlimit_platform_qos = {
	.constraints = &npu_pm_freq_dnlimit_constraints,
	.name = "npu_freq_dnlimit",
};
static BLOCKING_NOTIFIER_HEAD(npu_pm_freq_uplimit_notifier);
static struct platform_qos_constraints npu_pm_freq_uplimit_constraints = {
	.list = PLIST_HEAD_INIT(npu_pm_freq_uplimit_constraints.list),
	.target_value = PLATFORM_QOS_NPU_FREQ_UPLIMIT_DEFAULT_VALUE,
	.default_value = PLATFORM_QOS_NPU_FREQ_UPLIMIT_DEFAULT_VALUE,
	.no_constraint_value = PLATFORM_QOS_NPU_FREQ_UPLIMIT_DEFAULT_VALUE,
	.type = PLATFORM_QOS_MIN,
	.notifiers = &npu_pm_freq_uplimit_notifier,
};
static struct platform_qos_object npu_pm_freq_uplimit_platform_qos = {
	.constraints = &npu_pm_freq_uplimit_constraints,
	.name = "npu_freq_uplimit",
};
#endif

#ifdef CONFIG_NETWORK_LATENCY_PLATFORM_QOS
static BLOCKING_NOTIFIER_HEAD(network_latency_notifier);
static struct platform_qos_constraints network_latency_constraints = {
	.list = PLIST_HEAD_INIT(network_latency_constraints.list),
	.target_value = PLATFORM_QOS_NETWORK_LAT_DEFAULT_VALUE,
	.default_value = PLATFORM_QOS_NETWORK_LAT_DEFAULT_VALUE,
	.no_constraint_value = PLATFORM_QOS_NETWORK_LAT_DEFAULT_VALUE,
	.type = PLATFORM_QOS_MIN,
	.notifiers = &network_latency_notifier,
};
static struct platform_qos_object network_latency_platform_qos = {
	.constraints = &network_latency_constraints,
	.name = "network_latency",
};
#endif

static struct platform_qos_object *qos_array[] = {
	&null_platform_qos,
#ifdef CONFIG_ONSYS_AVS
	&onsys_avs_ctrl_level_onsys_qos,
#endif
#ifdef CONFIG_DEVFREQ_GOV_PLATFORM_QOS
	&memory_latency_platform_qos,
	&memory_throughput_platform_qos,
	&memory_throughput_up_th_platform_qos,
#endif
#ifdef CONFIG_DEVFREQ_L1BUS_LATENCY_PLATFORM
	&l1bus_latency_platform_qos,
#endif
#ifdef CONFIG_GPUTOP_FREQ
	&gputop_freq_dnlimit_platform_qos,
	&gputop_freq_uplimit_platform_qos,
#endif
#ifdef CONFIG_NPUFREQ_PLATFORM_QOS
	&npu_pm_freq_dnlimit_platform_qos,
	&npu_pm_freq_uplimit_platform_qos,
#endif
#ifdef CONFIG_NETWORK_LATENCY_PLATFORM_QOS
	&network_latency_platform_qos,
#endif
};

static ssize_t platform_qos_power_write(struct file *filp,
					const char __user *buf,
					size_t count,
					loff_t *f_pos __maybe_unused);
static ssize_t platform_qos_power_read(struct file *filp, char __user *buf,
				       size_t count, loff_t *f_pos);
static int platform_qos_power_open(struct inode *inode, struct file *filp);
static int platform_qos_power_release(struct inode *inode __maybe_unused,
				      struct file *filp);

static const struct file_operations platform_qos_power_fops = {
	.write = platform_qos_power_write,
	.read = platform_qos_power_read,
	.open = platform_qos_power_open,
	.release = platform_qos_power_release,
	.llseek = noop_llseek,
};

/* unlocked internal variant */
static int platform_qos_get_value(struct platform_qos_constraints *c)
{
	struct plist_node *node = NULL;
	int total_value = 0;

	if (plist_head_empty(&c->list))
		return c->no_constraint_value;

	switch (c->type) {
	case PLATFORM_QOS_MIN:
		return plist_first(&c->list)->prio;

	case PLATFORM_QOS_MAX:
		return plist_last(&c->list)->prio;

	case PLATFORM_QOS_SUM:
		plist_for_each(node, &c->list)
			total_value += node->prio;

		return total_value;

	default:
		/* runtime check for not using enum */
		WARN_ON(true);
		return PLATFORM_QOS_DEFAULT_VALUE;
	}
}

s32 platform_qos_read_value(struct platform_qos_constraints *c)
{
	return c->target_value;
}

static inline void platform_qos_set_value(struct platform_qos_constraints *c,
					  s32 value)
{
	c->target_value = value;
}

static int platform_qos_debug_show(struct seq_file *s,
				   void *unused __maybe_unused)
{
	struct platform_qos_object *qos =
		(struct platform_qos_object *)s->private;
	struct platform_qos_constraints *c = NULL;
	struct platform_qos_request *req = NULL;
	char *type = NULL;
	unsigned long flags;
	int tot_reqs = 0;
	int active_reqs = 0;

	if (IS_ERR_OR_NULL(qos)) {
		pr_err("%s: bad qos param!\n", __func__);
		return -EINVAL;
	}
	c = qos->constraints;
	if (IS_ERR_OR_NULL(c)) {
		pr_err("%s: Bad constraints on qos?\n", __func__);
		return -EINVAL;
	}

	/* Lock to ensure we have a snapshot */
	spin_lock_irqsave(&platform_qos_lock, flags);
	if (plist_head_empty(&c->list)) {
		seq_puts(s, "Empty!\n");
		goto out;
	}

	switch (c->type) {
	case PLATFORM_QOS_MIN:
		type = "Minimum";
		break;
	case PLATFORM_QOS_MAX:
		type = "Maximum";
		break;
	case PLATFORM_QOS_SUM:
		type = "Sum";
		break;
	default:
		type = "Unknown";
	}

	plist_for_each_entry(req, &c->list, node) {
		char *state = "Default";

		if (req->node.prio != c->default_value) {
			active_reqs++;
			state = "Active";
		}
		tot_reqs++;
		seq_printf(s, "%d: %d: %s\n", tot_reqs,
			   (req->node).prio, state);
	}

	seq_printf(s, "Type=%s, Value=%d, Requests: active=%d / total=%d\n",
		   type, platform_qos_get_value(c), active_reqs, tot_reqs);

out:
	spin_unlock_irqrestore(&platform_qos_lock, flags);
	return 0;
}

DEFINE_SHOW_ATTRIBUTE(platform_qos_debug);

/**
 * platform_qos_update_target -
 * manages the constraints list and calls the notifiers
 *  if needed
 * @c: constraints data struct
 * @node: request to add to the list, to update or to remove
 * @action: action to take on the constraints list
 * @value: value of the request to add or update
 *
 * This function returns 1 if the aggregated constraint value has changed, 0
 *  otherwise.
 */
int platform_qos_update_target(struct platform_qos_constraints *c,
			       struct plist_node *node,
			       enum platform_qos_req_action action, int value)
{
	unsigned long flags;
	int prev_value, curr_value, new_value;
	int ret;

	spin_lock_irqsave(&platform_qos_lock, flags);
	prev_value = platform_qos_get_value(c);
	if (value == PLATFORM_QOS_DEFAULT_VALUE)
		new_value = c->default_value;
	else
		new_value = value;

	switch (action) {
	case PLATFORM_QOS_REMOVE_REQ:
		plist_del(node, &c->list);
		break;
	case PLATFORM_QOS_UPDATE_REQ:
		/*
		 * to change the list, we atomically remove, reinit
		 * with new value and add, then see if the extremal
		 * changed
		 */
		plist_del(node, &c->list);
		/* fall through */
	case PLATFORM_QOS_ADD_REQ:
		plist_node_init(node, new_value);
		plist_add(node, &c->list);
		break;
	default:
		/* no action */
		break;
	}

	curr_value = platform_qos_get_value(c);
	platform_qos_set_value(c, curr_value);

	spin_unlock_irqrestore(&platform_qos_lock, flags);

	if (prev_value != curr_value) {
		ret = 1;
		if (c->notifiers)
			blocking_notifier_call_chain(c->notifiers,
						     (unsigned long)curr_value,
						     NULL);
	} else {
		ret = 0;
	}
	return ret;
}

/**
 * platform_qos_flags_remove_req - Remove device PM QoS flags request.
 * @pqf: Device PM QoS flags set to remove the request from.
 * @req: Request to remove from the set.
 */
static void platform_qos_flags_remove_req(
		struct platform_qos_flags *pqf,
		struct platform_qos_flags_request *req)
{
	u32 val = 0;

	list_del(&req->node);
	list_for_each_entry(req, &pqf->list, node)
		val |= req->flags;

	pqf->effective_flags = val;
}

/**
 * platform_qos_update_flags - Update a set of PM QoS flags.
 * @pqf: Set of flags to update.
 * @req: Request to add to the set, to modify, or to remove from the set.
 * @action: Action to take on the set.
 * @val: Value of the request to add or modify.
 *
 * Update the given set of PM QoS flags and call notifiers if the aggregate
 * value has changed.  Returns 1 if the aggregate constraint value has changed,
 * 0 otherwise.
 */
bool platform_qos_update_flags(struct platform_qos_flags *pqf,
			       struct platform_qos_flags_request *req,
			       enum platform_qos_req_action action, u32 val)
{
	unsigned long irqflags;
	u32 prev_value, curr_value;

	spin_lock_irqsave(&platform_qos_lock, irqflags);

	prev_value = list_empty(&pqf->list) ? 0 : pqf->effective_flags;

	switch (action) {
	case PLATFORM_QOS_REMOVE_REQ:
		platform_qos_flags_remove_req(pqf, req);
		break;
	case PLATFORM_QOS_UPDATE_REQ:
		platform_qos_flags_remove_req(pqf, req);
		/* fall through */
	case PLATFORM_QOS_ADD_REQ:
		req->flags = val;
		INIT_LIST_HEAD(&req->node);
		list_add_tail(&req->node, &pqf->list);
		pqf->effective_flags |= val;
		break;
	default:
		/* no action */
		break;
	}

	curr_value = list_empty(&pqf->list) ? 0 : pqf->effective_flags;

	spin_unlock_irqrestore(&platform_qos_lock, irqflags);

	return prev_value != curr_value;
}

/**
 * platform_qos_request - returns current system wide qos expectation
 * @platform_qos_class: identification of which qos value is requested
 *
 * This function returns the current target value.
 */
int platform_qos_request(int platform_qos_class)
{
	return platform_qos_read_value(
			qos_array[platform_qos_class]->constraints);
}

int platform_qos_request_active(struct platform_qos_request *req)
{
	return req->platform_qos_class != 0;
}
EXPORT_SYMBOL_GPL(platform_qos_request_active);

static void __platform_qos_update_request(struct platform_qos_request *req,
					  s32 new_value)
{
	if (new_value != req->node.prio)
		platform_qos_update_target(
			qos_array[req->platform_qos_class]->constraints,
			&req->node, PLATFORM_QOS_UPDATE_REQ, new_value);
}

/**
 * platform_qos_work_fn - the timeout handler of
 * platform_qos_update_request_timeout
 * @work: work struct for the delayed work (timeout)
 *
 * This cancels the timeout request by falling back to the default at timeout.
 */
static void platform_qos_work_fn(struct work_struct *work)
{
	struct platform_qos_request *req = container_of(to_delayed_work(work),
			struct platform_qos_request, work);

	__platform_qos_update_request(req, PLATFORM_QOS_DEFAULT_VALUE);
}

/**
 * platform_qos_add_request - inserts new qos request into the list
 * @req: pointer to a preallocated handle
 * @platform_qos_class: identifies which list of qos request to use
 * @value: defines the qos request
 *
 * This function inserts a new entry in the platform_qos_class list
 * of requested qos performance characteristics.
 * It recomputes the aggregate QoS expectations
 * for the platform_qos_class of parameters
 * and initializes the platform_qos_request
 * handle.  Caller needs to save this handle for later use in updates and
 * removal.
 */

void platform_qos_add_request(struct platform_qos_request *req,
			      int platform_qos_class, s32 value)
{
	if (!req) /* guard against callers passing in null */
		return;

	if (platform_qos_request_active(req)) {
		WARN(1, "%s called for already added request\n", __func__);
		return;
	}
	req->platform_qos_class = platform_qos_class;
	INIT_DELAYED_WORK(&req->work, platform_qos_work_fn);
	platform_qos_update_target(qos_array[platform_qos_class]->constraints,
				   &req->node, PLATFORM_QOS_ADD_REQ, value);
}
EXPORT_SYMBOL_GPL(platform_qos_add_request);

/**
 * platform_qos_update_request - modifies an existing qos request
 * @req : handle to list element holding a platform_qos request to use
 * @value: defines the qos request
 *
 * Updates an existing qos request for
 * the platform_qos_class of parameters along
 * with updating the target platform_qos_class value.
 *
 * Attempts are made to make this code callable on hot code paths.
 */
void platform_qos_update_request(struct platform_qos_request *req,
				 s32 new_value)
{
	if (!req) /* guard against callers passing in null */
		return;

	if (!platform_qos_request_active(req)) {
		WARN(1, "%s called for unknown object\n", __func__);
		return;
	}

	cancel_delayed_work_sync(&req->work);
	__platform_qos_update_request(req, new_value);
}
EXPORT_SYMBOL_GPL(platform_qos_update_request);

/**
 * platform_qos_update_request_timeout -
 * modifies an existing qos request temporarily.
 * @req : handle to list element holding a platform_qos request to use
 * @new_value: defines the temporal qos request
 * @timeout_us: the effective duration of this qos request in usecs.
 *
 * After timeout_us, this qos request is cancelled automatically.
 */
void platform_qos_update_request_timeout(struct platform_qos_request *req,
					 s32 new_value,
					 unsigned long timeout_us)
{
	if (!req)
		return;
	if (WARN(!platform_qos_request_active(req),
		 "%s called for unknown object.", __func__))
		return;

	cancel_delayed_work_sync(&req->work);

	if (new_value != req->node.prio)
		platform_qos_update_target(
			qos_array[req->platform_qos_class]->constraints,
			&req->node, PLATFORM_QOS_UPDATE_REQ, new_value);

	schedule_delayed_work(&req->work, usecs_to_jiffies(timeout_us));
}

/**
 * platform_qos_remove_request - modifies an existing qos request
 * @req: handle to request list element
 *
 * Will remove pm qos request from the list of constraints and
 * recompute the current target value for the platform_qos_class.  Call this
 * on slow code paths.
 */
void platform_qos_remove_request(struct platform_qos_request *req)
{
	if (!req) /* guard against callers passing in null */
		return;
		/* silent return to keep pcm code cleaner */

	if (!platform_qos_request_active(req)) {
		WARN(1, "%s called for unknown object\n", __func__);
		return;
	}

	cancel_delayed_work_sync(&req->work);

	platform_qos_update_target(
			qos_array[req->platform_qos_class]->constraints,
			&req->node, PLATFORM_QOS_REMOVE_REQ,
			PLATFORM_QOS_DEFAULT_VALUE);
	(void)memset_s(req, sizeof(*req), 0, sizeof(*req));
}
EXPORT_SYMBOL_GPL(platform_qos_remove_request);

/**
 * platform_qos_add_notifier - sets notification entry
 * for changes to target value
 * @platform_qos_class: identifies which qos target changes should be notified.
 * @notifier: notifier block managed by caller.
 *
 * will register the notifier into a notification chain that gets called
 * upon changes to the platform_qos_class target value.
 */
int platform_qos_add_notifier(int platform_qos_class,
			      struct notifier_block *notifier)
{
	int retval;

	retval = blocking_notifier_chain_register(
			qos_array[platform_qos_class]->constraints->notifiers,
			notifier);

	return retval;
}
EXPORT_SYMBOL_GPL(platform_qos_add_notifier);

/**
 * platform_qos_remove_notifier - deletes notification entry from chain.
 * @platform_qos_class: identifies which qos target changes are notified.
 * @notifier: notifier block to be removed.
 *
 * will remove the notifier from the notification chain that gets called
 * upon changes to the platform_qos_class target value.
 */
int platform_qos_remove_notifier(int platform_qos_class,
				 struct notifier_block *notifier)
{
	int retval;

	retval = blocking_notifier_chain_unregister(
			qos_array[platform_qos_class]->constraints->notifiers,
			notifier);

	return retval;
}
EXPORT_SYMBOL_GPL(platform_qos_remove_notifier);

/* User space interface to PM QoS classes via misc devices */
static int register_platform_qos_misc(struct platform_qos_object *qos,
				      struct dentry *d)
{
	qos->platform_qos_power_miscdev.minor = MISC_DYNAMIC_MINOR;
	qos->platform_qos_power_miscdev.name = qos->name;
	qos->platform_qos_power_miscdev.fops = &platform_qos_power_fops;

	debugfs_create_file(qos->name, 0444, d, (void *)qos,
			    &platform_qos_debug_fops);

	return misc_register(&qos->platform_qos_power_miscdev);
}

static int find_platform_qos_object_by_minor(int minor)
{
	int qos_class;

	for (qos_class = PLATFORM_QOS_RESERVED + 1;
	     qos_class < PLATFORM_QOS_NUM_CLASSES;
	     qos_class++) {
		if (minor ==
			qos_array[qos_class]->platform_qos_power_miscdev.minor)
			return qos_class;
	}
	return -1;
}

static int platform_qos_power_open(struct inode *inode, struct file *filp)
{
	long platform_qos_class;

	platform_qos_class = find_platform_qos_object_by_minor(iminor(inode));
	if (platform_qos_class > PLATFORM_QOS_RESERVED) {
		struct platform_qos_request *req =
			kzalloc(sizeof(*req), GFP_KERNEL);
		if (!req)
			return -ENOMEM;

		platform_qos_add_request(req, platform_qos_class,
					 PLATFORM_QOS_DEFAULT_VALUE);
		filp->private_data = req;

		return 0;
	}
	return -EPERM;
}

static int platform_qos_power_release(struct inode *inode __maybe_unused,
				      struct file *filp)
{
	struct platform_qos_request *req;

	req = filp->private_data;
	platform_qos_remove_request(req);
	kfree(req);

	return 0;
}

static ssize_t platform_qos_power_read(struct file *filp, char __user *buf,
				       size_t count, loff_t *f_pos)
{
	s32 value;
	unsigned long flags;
	struct platform_qos_request *req = filp->private_data;

	if (!req)
		return -EINVAL;
	if (!platform_qos_request_active(req))
		return -EINVAL;

	spin_lock_irqsave(&platform_qos_lock, flags);
	value = platform_qos_get_value(
			qos_array[req->platform_qos_class]->constraints);
	spin_unlock_irqrestore(&platform_qos_lock, flags);

	return simple_read_from_buffer(buf, count, f_pos, &value, sizeof(s32));
}

static ssize_t platform_qos_power_write(struct file *filp,
					const char __user *buf,
					size_t count,
					loff_t *f_pos __maybe_unused)
{
	s32 value;
	struct platform_qos_request *req = NULL;

	if (count == sizeof(s32)) {
		if (copy_from_user(&value, buf, sizeof(s32)))
			return -EFAULT;
	} else {
		int ret;

		ret = kstrtos32_from_user(buf, count, 16, &value);
		if (ret)
			return ret;
	}

	req = filp->private_data;
	platform_qos_update_request(req, value);

	return count;
}

static int __init platform_qos_power_init(void)
{
	int ret = 0;
	int i;
	struct dentry *d = NULL;

	BUILD_BUG_ON(ARRAY_SIZE(qos_array) != PLATFORM_QOS_NUM_CLASSES);
	d = debugfs_create_dir("platform_qos", NULL);

	for (i = PLATFORM_QOS_RESERVED + 1;
	     i < PLATFORM_QOS_NUM_CLASSES; i++) {
		ret = register_platform_qos_misc(qos_array[i], d);
		if (ret < 0) {
			pr_err("%s: %s setup failed\n",
			       __func__, qos_array[i]->name);
			return ret;
		}
	}

	return ret;
}

late_initcall(platform_qos_power_init);
