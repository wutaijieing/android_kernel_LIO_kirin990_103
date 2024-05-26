/*
 * dynamic_prio.c
 *
 * Set a task to a new prio (can be a new sched class) temporarily and
 * restore what it was later.
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
#include <securec.h>
#include <trace/events/sched.h>

#define INVALID_POLICY 0xbbad

static bool dyn_switched(struct task_struct *p)
{
	/*
	 * Setting vip is a kind of dynamic switching but we didn't set a
	 * bit on the dyn_switched_flag to avoid redundancy. So that we
	 * check both normal_vip_prio and dyn_switched_flag here.
	 */
	return p->dyn_prio.normal_vip_prio || p->dyn_prio.dyn_switched_flag;
}

/*
 * One must backup sched attr before setting dynamic prio and the flags.
 * Must hold p->dyn_prio.lock.
 */
int backup_sched_attr(struct task_struct *p)
{
	if (task_has_dl_policy(p) ||
	    p->sched_class == &stop_sched_class ||
	    p->sched_class == &idle_sched_class)
		return -EINVAL;

	/* Already backed up. */
	if (dyn_switched(p))
		return 0;

	/* Only back up the priority before dynamic switched. */
	p->dyn_prio.saved_policy = p->policy;
	p->dyn_prio.saved_prio = p->normal_prio;

	return 0;
}

/* Must hold p->dyn_prio.lock. */
int restore_sched_attr(struct task_struct *p)
{
	unsigned int vip_prio;
	struct sched_attr attr = {
		.sched_flags = SCHED_FLAG_KEEP_ROF(p),
	};

	vip_prio = p->dyn_prio.normal_vip_prio;
#ifdef CONFIG_SCHED_VIP_PI
	vip_prio = max(vip_prio, get_max_vip_boost(p));
#endif

	if (vip_prio > 0) {
		attr.sched_policy = SCHED_RR;
		attr.sched_priority = vip_prio;
	} else {
		unsigned int policy = p->dyn_prio.saved_policy;
		unsigned int prio = p->dyn_prio.saved_prio;

		attr.sched_policy = policy;
		if (fair_policy(policy))
			attr.sched_nice = PRIO_TO_NICE(prio);
		else if (rt_policy(policy))
			attr.sched_priority = MAX_RT_PRIO-1 - prio;
		else
			return -EINVAL;
	}

	return sched_setattr_dynamic(p, &attr);
}

static void copy_saved_prio(struct task_struct *src, struct task_struct *dst)
{
	if (WARN_ON_ONCE(src->dyn_prio.saved_policy == INVALID_POLICY))
		return;

	dst->policy = src->dyn_prio.saved_policy;
	dst->prio = dst->normal_prio = dst->static_prio = src->dyn_prio.saved_prio;

	if (rt_policy(dst->policy))
		dst->rt_priority = MAX_RT_PRIO-1 - dst->normal_prio;
	else
		dst->rt_priority = 0;
}

void dynamic_prio_fork(struct task_struct *child)
{
	unsigned long flags;

	/*
	 * Always inherit parent's original priority instead of the
	 * dynamic switched one.
	 * Must be called before the reset_on_fork.
	 */
	spin_lock_irqsave(&current->dyn_prio.lock, flags);
	if (dyn_switched(current))
		copy_saved_prio(current, child);
	spin_unlock_irqrestore(&current->dyn_prio.lock, flags);
	WARN_ON_ONCE(is_vip_prio(child->normal_prio));

	/* Also initialize our data structure. */
	dynamic_prio_init(child);
}

int sched_setattr_dynamic(struct task_struct *p, const struct sched_attr *attr)
{
	int ret;

#ifdef CONFIG_RT_SWITCH_CFS_IF_TOO_LONG
	if (test_bit(LONG_RT_DOWNGRADE, &p->dyn_prio.dyn_switched_flag) &&
	    rt_policy(attr->sched_policy))
		return -EINVAL;
#endif

	/*
	 * To let __sched_setscheduler() know we are setting dynamic prio.
	 * That makes __sched_setscheduler() able to acquire p->dyn_prio.lock
	 * and deal with dyn_prio flags for callers other than us.
	 */
	set_tsk_thread_flag(current, TIF_DYN_PRIO);
	/* No check, no PI, no priority mapping. */
	ret = sched_setattr_directly(p, attr);
	clear_tsk_thread_flag(current, TIF_DYN_PRIO);

	return ret;
}

/*
 * What is raw_xxx?
 * The non-dynamic sched attributes of p.
 * That is, if dyn_switched(p) return attr based on p->dyn_prio.saved_*
 * else the normal sched priority.
 */

static unsigned int _get_raw_policy(struct task_struct *p)
{
	if (dyn_switched(p))
		return p->dyn_prio.saved_policy;
	return p->policy;
}

/* Checked rt raw_policy */
static unsigned int _get_raw_rt_priority(struct task_struct *p)
{
	unsigned int rt_priority;

	if (dyn_switched(p))
		rt_priority = MAX_RT_PRIO-1 - p->dyn_prio.saved_prio;
	else
		rt_priority = p->rt_priority;

#ifdef CONFIG_RT_PRIO_EXTEND_VIP
	return vip_spare_unmap_rt_priority(rt_priority);
#else
	return rt_priority;
#endif
}

/* Checked fair raw_policy */
static int _get_raw_static_prio(struct task_struct *p)
{
	if (dyn_switched(p))
		return p->dyn_prio.saved_prio;
	return p->static_prio;
}

static int _get_raw_nice(struct task_struct *p)
{
	return PRIO_TO_NICE(_get_raw_static_prio(p));
}

unsigned int get_raw_sched_priority(struct task_struct *p)
{
	unsigned long flags;
	unsigned int rt_priority = 0;

	spin_lock_irqsave(&p->dyn_prio.lock, flags);

	if (rt_policy(_get_raw_policy(p)))
		rt_priority = _get_raw_rt_priority(p);

	spin_unlock_irqrestore(&p->dyn_prio.lock, flags);

	return rt_priority;
}

void get_raw_attr(struct task_struct *p, struct sched_attr *attr)
{
	unsigned long flags;
	unsigned int raw_policy;

	spin_lock_irqsave(&p->dyn_prio.lock, flags);

	raw_policy = _get_raw_policy(p);

	attr->sched_policy = raw_policy;
	if (p->sched_reset_on_fork)
		attr->sched_flags |= SCHED_FLAG_RESET_ON_FORK;

	if (dl_policy(raw_policy))
		__getparam_dl(p, attr);
	else if (rt_policy(raw_policy))
		attr->sched_priority = _get_raw_rt_priority(p);
	else
		attr->sched_nice = _get_raw_nice(p);

	spin_unlock_irqrestore(&p->dyn_prio.lock, flags);
}

unsigned int get_raw_scheduler_policy(struct task_struct *p)
{
	unsigned long flags;
	unsigned int policy;

	spin_lock_irqsave(&p->dyn_prio.lock, flags);
	policy = _get_raw_policy(p);
	policy |= (p->sched_reset_on_fork ? SCHED_RESET_ON_FORK : 0);
	spin_unlock_irqrestore(&p->dyn_prio.lock, flags);
	return policy;
}

#ifdef CONFIG_VIP_RAISE_BINDED_KTHREAD

static void vip_raise_kthread_func(struct irq_work *irq_work)
{
	struct sched_attr attr;
	struct task_struct *p = container_of(
			container_of(irq_work,
				     struct sched_dynamic_prio,
				     vip_raise_kthread_work),
			struct task_struct,
			dyn_prio);

	spin_lock(&p->dyn_prio.lock);

	if (backup_sched_attr(p))
		goto out;

	if (__test_and_set_bit(RAISED_KTHREAD,
				&p->dyn_prio.dyn_switched_flag))
		goto out;

	attr.sched_flags = SCHED_FLAG_KEEP_ROF(p);
	attr.sched_policy = SCHED_RR;
	attr.sched_priority = VIP_PRIO_WIDTH + 1;
	sched_setattr_dynamic(p, &attr);

out:
	spin_unlock(&p->dyn_prio.lock);
	trace_dynamic_prio_func(__func__, p);
	put_task_struct(p);
}

#define PER_CPU_KTHREAD_DELAY_NS (10 * NSEC_PER_MSEC)

/*
 * Called from check_for_rt_migration() with rq lock held.
 * If there's a vip thread running and a per cpu kthread delayed,
 * raise the kthread with VIP_PRIO_WIDTH+1 to preempt the vip.
 */
bool check_delayed_kthread(struct rq *rq)
{
	struct task_struct *p = rq->binded_kthread;

	if (likely(!p))
		return false;

	if (!is_vip_prio(rq->rt.highest_prio.curr))
		return false;

	if (task_delay(p, rq) < PER_CPU_KTHREAD_DELAY_NS)
		return false;

	if (test_bit(RAISED_KTHREAD, &p->dyn_prio.dyn_switched_flag))
		return false;

	if (irq_work_queue(&p->dyn_prio.vip_raise_kthread_work)) {
		get_task_struct(p);
		return true;
	}

	return false;
}

#endif /* CONFIG_VIP_RAISE_BINDED_KTHREAD */

#ifdef CONFIG_RT_SWITCH_CFS_IF_TOO_LONG

static void long_rt_downgrade_func(struct irq_work *irq_work)
{
	struct sched_attr attr;
	struct task_struct *p = container_of(
			container_of(irq_work,
				     struct sched_dynamic_prio,
				     long_rt_downgrade_work),
			struct task_struct,
			dyn_prio);

	spin_lock(&p->dyn_prio.lock);

	if (backup_sched_attr(p))
		goto out;

	if (__test_and_set_bit(LONG_RT_DOWNGRADE,
				&p->dyn_prio.dyn_switched_flag))
		goto out;

	attr.sched_flags = SCHED_FLAG_KEEP_ROF(p);
	attr.sched_policy = SCHED_NORMAL;
	attr.sched_priority = 0;
	attr.sched_nice = 0;
	sched_setattr_dynamic(p, &attr);

out:
	spin_unlock(&p->dyn_prio.lock);
	trace_dynamic_prio_func(__func__, p);
	put_task_struct(p);
}

#define TOO_LONG_RT_RUNNING_NS (200 * NSEC_PER_MSEC)

/* Called from check_for_rt_migration(). */
bool check_long_rt(struct task_struct *p)
{
	s64 delta_exec = p->se.sum_exec_runtime - p->sum_exec_runtime_enqueue;

	if (likely(delta_exec < TOO_LONG_RT_RUNNING_NS))
		return false;

	if (test_bit(LONG_RT_DOWNGRADE, &p->dyn_prio.dyn_switched_flag))
		return false;

	if (irq_work_queue(&p->dyn_prio.long_rt_downgrade_work)) {
		get_task_struct(p);
		return true;
	}

	return false;
}

#endif /* CONFIG_RT_SWITCH_CFS_IF_TOO_LONG */

static void sleeping_reset_func(struct irq_work *irq_work)
{
	struct task_struct *p;
	p = container_of(container_of(irq_work,
				      struct sched_dynamic_prio,
				      sleeping_reset_work),
			 struct task_struct,
			 dyn_prio);

	spin_lock(&p->dyn_prio.lock);

	if (!(p->dyn_prio.dyn_switched_flag & RESTORE_ON_SLEEP))
		goto out;

	p->dyn_prio.dyn_switched_flag &= ~RESTORE_ON_SLEEP;

	restore_sched_attr(p);

out:
	spin_unlock(&p->dyn_prio.lock);
	trace_dynamic_prio_func(__func__, p);
	put_task_struct(p);
}

/* Called from p sleeping. */
void reset_dyn_prio_on_sleep(struct task_struct *p)
{
	if (unlikely(p->dyn_prio.dyn_switched_flag & RESTORE_ON_SLEEP))
		if (irq_work_queue(&p->dyn_prio.sleeping_reset_work))
			get_task_struct(p);
}

void dynamic_prio_clear(struct task_struct *p)
{
	p->dyn_prio.saved_policy = INVALID_POLICY;
	p->dyn_prio.dyn_switched_flag = 0;
#ifdef CONFIG_SCHED_VIP
	p->dyn_prio.normal_vip_prio = 0;
#endif
#ifdef CONFIG_SCHED_VIP_PI
	memset_s(p->dyn_prio.vip_boost, sizeof(p->dyn_prio.vip_boost), 0, sizeof(p->dyn_prio.vip_boost));
#endif
}

void dynamic_prio_init(struct task_struct *p)
{
	spin_lock_init(&p->dyn_prio.lock);
	init_irq_work(&p->dyn_prio.sleeping_reset_work, sleeping_reset_func);
#ifdef CONFIG_VIP_RAISE_BINDED_KTHREAD
	init_irq_work(&p->dyn_prio.vip_raise_kthread_work, vip_raise_kthread_func);
#endif
#ifdef CONFIG_RT_SWITCH_CFS_IF_TOO_LONG
	init_irq_work(&p->dyn_prio.long_rt_downgrade_work, long_rt_downgrade_func);
#endif
	dynamic_prio_clear(p);
}
