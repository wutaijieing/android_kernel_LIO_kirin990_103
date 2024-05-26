/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Only give sleepers 50% of their service deficit. This allows
 * them to run sooner, but does not allow tons of sleepers to
 * rip the spread apart.
 */
SCHED_FEAT(GENTLE_FAIR_SLEEPERS, true)

/*
 * Place new tasks ahead so that they do not starve already running
 * tasks
 */
SCHED_FEAT(START_DEBIT, true)

/*
 * Prefer to schedule the task we woke last (assuming it failed
 * wakeup-preemption), since its likely going to consume data we
 * touched, increases cache locality.
 */
SCHED_FEAT(NEXT_BUDDY, false)

/*
 * Prefer to schedule the task that ran last (when we did
 * wake-preempt) as that likely will touch the same data, increases
 * cache locality.
 */
SCHED_FEAT(LAST_BUDDY, true)

/*
 * Consider buddies to be cache hot, decreases the likelyness of a
 * cache buddy being migrated away, increases cache locality.
 */
SCHED_FEAT(CACHE_HOT_BUDDY, true)

/*
 * Allow wakeup-time preemption of the current task:
 */
SCHED_FEAT(WAKEUP_PREEMPTION, true)

SCHED_FEAT(HRTICK, false)
SCHED_FEAT(DOUBLE_TICK, false)

/*
 * Decrement CPU capacity based on time not spent running tasks
 */
SCHED_FEAT(NONTASK_CAPACITY, true)

/*
 * Queue remote wakeups on the target CPU and process them
 * using the scheduler IPI. Reduces rq->lock contention/bounces.
 */
#ifdef CONFIG_HISI_EAS_SCHED
SCHED_FEAT(TTWU_QUEUE, false)
#else
SCHED_FEAT(TTWU_QUEUE, true)
#endif

/*
 * When doing wakeups, attempt to limit superfluous scans of the LLC domain.
 */
SCHED_FEAT(SIS_AVG_CPU, false)
SCHED_FEAT(SIS_PROP, true)

/*
 * Issue a WARN when we do multiple update_rq_clock() calls
 * in a single rq->lock section. Default disabled because the
 * annotations are not complete.
 */
SCHED_FEAT(WARN_DOUBLE_CLOCK, false)

#ifdef HAVE_RT_PUSH_IPI
#ifdef CONFIG_RT_ENERGY_EFFICIENT_SUPPORT
/*
 * Yes, we don't need that. RT_PUSH_IPI is a good idea but we
 * don't have worries about the large contention.
 * With RT_ENERGY_EFFICIENT_SUPPORT we would like to leave
 * some rt tasks to be runnable so that we might often have
 * rt overloaded cpus and RT_PUSH_IPI will take more expenses
 * because it will tell each cpu in rto_mask to do a pushing
 * whenever there is a chance of pulling.
 *
 * Consider there are some cpus with many low prio rt tasks,
 * the pull logic will ignore them without any locking but
 * the RT_PUSH_IPI logic will send an IPI and try to push,
 * which is a little bit heavy under RT_CAS enabled.
 * And it's not very easy to rewrite the RT_PUSH_IPI and get
 * what we want.
 */
SCHED_FEAT(RT_PUSH_IPI, false)
#else
/*
 * In order to avoid a thundering herd attack of CPUs that are
 * lowering their priorities at the same time, and there being
 * a single CPU that has an RT task that can migrate and is waiting
 * to run, where the other CPUs will try to take that CPUs
 * rq lock and possibly create a large contention, sending an
 * IPI to that CPU and let that CPU push the RT task to where
 * it should go may be a better scenario.
 */
SCHED_FEAT(RT_PUSH_IPI, true)
#endif
#endif

#ifdef CONFIG_SCHED_VIP
/*
 * Obviously SCHED_VIP needs RT_RUNTIME_SHARE to be true and
 * it has solved the concerns of commit 2586af1ac187f.
 */
SCHED_FEAT(RT_RUNTIME_SHARE, true)
#else
SCHED_FEAT(RT_RUNTIME_SHARE, false)
#endif
SCHED_FEAT(LB_MIN, false)
SCHED_FEAT(ATTACH_AGE_LOAD, true)

SCHED_FEAT(WA_IDLE, true)
SCHED_FEAT(WA_WEIGHT, true)
SCHED_FEAT(WA_BIAS, true)

/*
 * UtilEstimation. Use estimated CPU utilization.
 */
#ifdef CONFIG_SCHED_WALT
SCHED_FEAT(UTIL_EST, false)
SCHED_FEAT(UTIL_EST_FASTUP, false)
#else
SCHED_FEAT(UTIL_EST, true)
SCHED_FEAT(UTIL_EST_FASTUP, true)
#endif

SCHED_FEAT(ALT_PERIOD, true)
SCHED_FEAT(BASE_SLICE, true)
