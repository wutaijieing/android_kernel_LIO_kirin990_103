/*
 * vip.c
 *
 * VIP sched implemented inside RT sched class
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

#include "sched.h"
#include <trace/events/sched.h>

#ifdef CONFIG_RT_PRIO_EXTEND_VIP
/*
 * For non-vip rt priority [1 .. RT_PRIO_SPARE_MAX], map it to
 * [1+VIP_PRIO_WIDTH .. RT_PRIO_SPARE_MAX] so that we can spare
 * [1 .. VIP_PRIO_WIDTH] for vip tasks.
 * i.e. move [1..60] to [11..70], [60..70] to 70.
 *
 * Background:
 * Vip is a virtual sched class between cfs class and rt class
 * for critical user interactive tasks. We don't think cfs works
 * well for those tasks. And we also don't want them to preempt
 * system rt tasks.
 * The scheduling behaviour we want for vip is very similar to rt.
 * So there's no need for a new sched class or sched policy but
 * only inserting a lowest priority range into original rt class.
 * And the easiest way is to change the internal rt priority from
 * sched_setscheduler instead of replacing data structures or
 * macros everywhere.
 * We choose the [1 .. RT_PRIO_SPARE_MAX] -> [1+VIP_PRIO_WIDTH ..
 * RT_PRIO_SPARE_MAX] approach because rt priority is assigned with
 * 0 + offset, MAX_[USER]_RT_PRIO/2 +/- offset and MAX_[USER]_RT_PRIO
 * - offset most of the time. And it doesn't matter too much if
 * we happened to fall into the *compressed* area and lose the
 * relativity.
 */
#define RT_PRIO_SPARE_MAX (MAX_RT_PRIO / 2 + 20)

#if RT_PRIO_SPARE_MAX > (MAX_USER_RT_PRIO - 10)
#error "Weird RT config. Please change the above definition or disable it"
#endif

/*
 * Kernel (the priority values in task struct) use the mapped rt_priority
 * while userspace is aware of the unmapped one. And userspace&kernel
 * users who's calling sched_setscheduler() should pass the unmapped
 * value to params.sched_priority.
 */
unsigned int vip_spare_map_rt_priority(unsigned int rt_priority)
{
	if (rt_priority <= RT_PRIO_SPARE_MAX) {
		rt_priority += VIP_PRIO_WIDTH;
		if (rt_priority > RT_PRIO_SPARE_MAX)
			rt_priority = RT_PRIO_SPARE_MAX;
	}
	return rt_priority;
}

unsigned int vip_spare_unmap_rt_priority(unsigned int rt_priority)
{
	if (rt_priority > VIP_PRIO_WIDTH &&
	    rt_priority <= RT_PRIO_SPARE_MAX)
		rt_priority -= VIP_PRIO_WIDTH;
	return rt_priority;
}

void spare_vip_width(struct task_struct *p, int policy,
		     struct sched_attr *attr)
{
	if (!rt_policy(policy))
		return;

	/*
	 * This flag means we are setting priority temporarily by
	 * some scheduling code but not the regular sched_setscheduler()
	 * by a user. And this flag wants no priority mapping/sparing.
	 * Sched_setattr_directly() sets this flag.
	 *
	 * We can't add another argument to __sched_setscheduler()
	 * so let's set a flag on current task to pass it through.
	 */
	if (test_tsk_thread_flag(current, TIF_NO_EXTEND))
		return;

	attr->sched_priority = vip_spare_map_rt_priority(attr->sched_priority);
}

/* Try to leave userspace unaware of the extending if possible. */
unsigned int rt_priority_userspace(struct task_struct *p)
{
	return vip_spare_unmap_rt_priority(p->rt_priority);
}
#endif

/*
 * Priority for vip tasks:
 * The attr->sched_priority and p->rt_priority:
 *     [1 .. VIP_PRIO_WIDTH].
 * The p->prio and p->normal_prio: MAX_RT_PRIO-1 - p->rt_priority, which is:
 *     [MAX_RT_PRIO-1 - VIP_PRIO_WIDTH .. MAX_RT_PRIO-2].
 * Note that MAX_RT_PRIO-1 for p->prio/p->normal_prio is logically valid but
 * not possible in fact.
 *
 * There are 3 ways a task has vip prio:
 * 1. set_vip_prio (with normal_vip_prio set)
 * 2. PI boost (native)
 * 3. SCHED_VIP_PI boost (request_vip_boost)
 * Boosted vip and normal vip share same scheduling behaviour.
 */
bool is_vip_prio(int prio)
{
	return MAX_RT_PRIO-1 - VIP_PRIO_WIDTH <= prio && prio < MAX_RT_PRIO-1;
}

/* Task's effective vip prio. */
unsigned int task_vip_prio(struct task_struct *p)
{
	unsigned int prio;
	return p && is_vip_prio(prio = p->prio)
		? MAX_RT_PRIO-1 - prio
		: 0;
}

unsigned int task_normal_vip_prio(struct task_struct *p)
{
	return p ? p->dyn_prio.normal_vip_prio : 0;
}

static void set_vip_flags(struct task_struct *p)
{
	set_tsk_thread_flag(p, TIF_ENERGY_EFFICIENT);
	set_tsk_thread_flag(p, TIF_EXPECTED_HEAVY);
}

static void clear_vip_flags(struct task_struct *p)
{
	clear_tsk_thread_flag(p, TIF_ENERGY_EFFICIENT);
	clear_tsk_thread_flag(p, TIF_EXPECTED_HEAVY);
}

void mark_vip_lightweight(struct task_struct *p)
{
	clear_tsk_thread_flag(p, TIF_EXPECTED_HEAVY);
}

static int sched_attr_set_vip_prio(struct task_struct *p, unsigned int prio)
{
	struct sched_attr attr = {
		.sched_flags = SCHED_FLAG_KEEP_ROF(p),
		.sched_policy = SCHED_RR,
		.sched_priority = clamp_val(prio, 1, VIP_PRIO_WIDTH),
	};

	return sched_setattr_dynamic(p, &attr);
}

/*
 * Can be called in interrupt or atomic context.
 * Must not hold rq lock or pi lock.
 */
int set_vip_prio(struct task_struct *p, unsigned int prio)
{
	int ret = 0;
	unsigned long flags;

	if (!p)
		return -EINVAL;

	if (prio > VIP_PRIO_WIDTH)
		prio = VIP_PRIO_WIDTH;

	if (prio == p->dyn_prio.normal_vip_prio)
		return 0;

	spin_lock_irqsave(&p->dyn_prio.lock, flags);

	if (prio > 0) {
		/* Backup the non-vip attr. */
		if (!p->dyn_prio.normal_vip_prio)
			if (backup_sched_attr(p))
				goto out;

		p->dyn_prio.normal_vip_prio = prio;
		/*
		 * Set the vip flags before setting sched_attr for proper
		 * rt pushing behaviour.
		 */
		set_vip_flags(p);
		ret = sched_attr_set_vip_prio(p, prio);
	} else {
		/* Restore what it was before setting to vip. */
		if (p->dyn_prio.normal_vip_prio) {
			p->dyn_prio.normal_vip_prio = 0;
			clear_vip_flags(p);
			ret = restore_sched_attr(p);
		}
	}

out:
	spin_unlock_irqrestore(&p->dyn_prio.lock, flags);
	if (ret)
		pr_err("error setting vip. ret=%d task=%s(%d) prio=%u",
			ret, p->comm, p->pid, prio);
	trace_set_vip_prio(p, prio, ret);
	return ret;
}

#ifdef CONFIG_SCHED_VIP_PI
unsigned int get_max_vip_boost(struct task_struct *p)
{
	unsigned int type, max = 0;
	for (type = 0; type < VIP_BOOST_TYPE_MAX; type++)
		if (p->dyn_prio.vip_boost[type] > max)
			max = p->dyn_prio.vip_boost[type];
	return max;
}

/*
 * Request_vip_boost() may be called multiple times but there must be
 * a clear_vip_boost() in the end.
 * We do not support nested request&clear for the same task and same
 * type. The first clear will clear all previous request of the type.
 * Must NOT hold rq lock or pi lock.
 */
void request_vip_boost(struct task_struct *p, unsigned int vip_prio,
			enum vip_boost_type type)
{
	unsigned long flags;
	int ret = -ECHILD;

	if (!p || type >= VIP_BOOST_TYPE_MAX)
		return;

	if (vip_prio == 0 || vip_prio > VIP_PRIO_WIDTH)
		return;

	if (vip_prio <= p->dyn_prio.vip_boost[type])
		return;

	/* Don't boost higher priority RT task (quick check). */
	if (p->rt_priority > VIP_PRIO_WIDTH)
		return;

	spin_lock_irqsave(&p->dyn_prio.lock, flags);

	/* Double check inside the lock. */
	if (p->rt_priority > VIP_PRIO_WIDTH ||
	    test_bit(LONG_RT_DOWNGRADE, &p->dyn_prio.dyn_switched_flag))
		goto out_unlock;

	/* Must backup before setting the dyn_switched_flag. */
	if (backup_sched_attr(p))
		goto out_unlock;

	p->dyn_prio.vip_boost[type] = (unsigned char)vip_prio;
	/*
	 * VIP_BOOSTED bit must be consistent with max_vip_boost > 0.
	 * It works like normal_vip_prio. See restore_sched_attr().
	 */
	__set_bit(VIP_BOOSTED, &p->dyn_prio.dyn_switched_flag);

	/* Already boosted higher. Set vip_boost[type] and go out. */
	if (task_vip_prio(p) >= vip_prio)
		goto out_unlock;

	/*
	 * Set/clear the vip flags except it's a normal vip task
	 * or rt task who must have set the flag on its own and
	 * should keep it unchanged on vip boost.
	 */
	if (!p->dyn_prio.normal_vip_prio)
		set_vip_flags(p);

	ret = sched_attr_set_vip_prio(p, vip_prio);
	if (ret)
		goto out_unlock;

out_unlock:
	spin_unlock_irqrestore(&p->dyn_prio.lock, flags);
	trace_vip_boost('+', p, type, vip_prio, ret);
}

void clear_vip_boost(struct task_struct *p, enum vip_boost_type type)
{
	unsigned long flags;
	unsigned int max_vip_boost = 0, new_vip_prio = 0;
	int ret = -ECHILD;

	if (!p || type >= VIP_BOOST_TYPE_MAX)
		return;

	if (p->dyn_prio.vip_boost[type] == 0)
		return;

	spin_lock_irqsave(&p->dyn_prio.lock, flags);

	p->dyn_prio.vip_boost[type] = 0;

	max_vip_boost = get_max_vip_boost(p);
	if (max_vip_boost == 0) {
		__clear_bit(VIP_BOOSTED, &p->dyn_prio.dyn_switched_flag);

		if (!p->dyn_prio.normal_vip_prio)
			clear_vip_flags(p);
	}

	new_vip_prio = max(max_vip_boost, p->dyn_prio.normal_vip_prio);
	if (new_vip_prio == task_vip_prio(p))
		goto out_unlock;

	/*
	 * Reasons of deferring the restore for a finished binder:
	 * 1. The cost of __sched_setscheduler for a dequeued task
	 *    is much cheaper than a current queued task.
	 * 2. Prevent the binder caller preempts binder because of
	 *    lowering our priority. It will save one scheduling.
	 */
	if (type == VIP_BOOST_BINDER && p->on_rq) {
		__set_bit(BINDER_DEFERRED_RESTORE,
			&p->dyn_prio.dyn_switched_flag);
		ret = -EAGAIN;
	} else {
		ret = restore_sched_attr(p);
	}

out_unlock:
	spin_unlock_irqrestore(&p->dyn_prio.lock, flags);
	trace_vip_boost('-', p, type, max_vip_boost, ret);
}

void set_vip_boost(struct task_struct *p, unsigned int vip_prio,
			enum vip_boost_type type)
{
	if (vip_prio > 0)
		request_vip_boost(p, vip_prio, type);
	else
		clear_vip_boost(p, type);
}
#endif

#ifdef CONFIG_SCHED_VIP_CGROUP
void set_cgroup_vip_prio(struct task_struct *p)
{
	unsigned int new_vip_prio = uclamp_vip_prio(p);
	unsigned int curr_vip_prio = p->dyn_prio.vip_boost[VIP_BOOST_CGROUP];

	if (likely(new_vip_prio == curr_vip_prio))
		return;

	if (new_vip_prio < curr_vip_prio)
		clear_vip_boost(p, VIP_BOOST_CGROUP);

	request_vip_boost(p, new_vip_prio, VIP_BOOST_CGROUP);
}
#endif
