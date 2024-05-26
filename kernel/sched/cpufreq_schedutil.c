// SPDX-License-Identifier: GPL-2.0
/*
 * CPUFreq governor based on scheduler-provided CPU utilization data.
 *
 * Copyright (C) 2016, Intel Corporation
 * Author: Rafael J. Wysocki <rafael.j.wysocki@intel.com>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include "sched.h"

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
#include <linux/uidgid.h>
#include <linux/slab.h>
#include <linux/syscalls.h>

#define CREATE_TRACE_POINTS
#include <trace/events/cpufreq_schedutil.h>
#undef CREATE_TRACE_POINTS
#endif

#include <linux/sched/cpufreq.h>
#include <trace/events/power.h>
#include <trace/hooks/sched.h>

#define IOWAIT_BOOST_MIN	(SCHED_CAPACITY_SCALE / 8)

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
enum {
	FREQ_STAT_CPU_LOAD		= (1 << 0),
	FREQ_STAT_TOP_TASK		= (1 << 1),
	FREQ_STAT_PRED_LOAD		= (1 << 2),
	FREQ_STAT_MAX_PRED_LS		= (1 << 3),
	FREQ_STAT_PRED_CPU_LOAD_MIN	= (1 << 4),
};

#define FREQ_STAT_USE_PRED_WINDOW \
	(FREQ_STAT_PRED_LOAD | FREQ_STAT_MAX_PRED_LS | FREQ_STAT_PRED_CPU_LOAD_MIN)

__read_mostly unsigned int sched_io_is_busy = 0;

void sched_set_io_is_busy(int val)
{
	sched_io_is_busy = val;
}

static BLOCKING_NOTIFIER_HEAD(sugov_status_notifier_list);
int sugov_register_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&sugov_status_notifier_list, nb);
}

int sugov_unregister_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&sugov_status_notifier_list, nb);
}

/* Target load. Lower values result in higher CPU speeds. */
#define DEFAULT_TARGET_LOAD 80
static unsigned int default_target_loads[] = {DEFAULT_TARGET_LOAD};
#define DEFAULT_RATE_LIMIT_US (79 * USEC_PER_MSEC)
static unsigned int default_above_hispeed_delay[] = {
		DEFAULT_RATE_LIMIT_US };
static unsigned int default_min_sample_time[] = {
		DEFAULT_RATE_LIMIT_US };
#endif

struct sugov_tunables {
	struct gov_attr_set	attr_set;
	unsigned int		rate_limit_us;

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
#define DEFAULT_OVERLOAD_DURATION_MS	250
	/* boost freq to max when running without idle over this duration time */
	unsigned int overload_duration;
	/* Hi speed to bump to from lo speed when load burst (default max) */
	unsigned int hispeed_freq;

	/* Go to hi speed when CPU load at or above this value. */
#define DEFAULT_GO_HISPEED_LOAD 100
	unsigned long go_hispeed_load;

	/* Target load. Lower values result in higher CPU speeds. */
	spinlock_t target_loads_lock;
	unsigned int *target_loads;
	int ntarget_loads;

	/*
	 * Wait this long before raising speed above hispeed, by default a
	 * single timer interval.
	*/
	spinlock_t above_hispeed_delay_lock;
	unsigned int *above_hispeed_delay;
	int nabove_hispeed_delay;

	/*
	 * The minimum amount of time to spend at a frequency before we can ramp
	 * down.
	 */
	spinlock_t min_sample_time_lock;
	unsigned int *min_sample_time;
	int nmin_sample_time;

	/* Duration of a boot pulse in usecs */
#define DEFAULT_BOOSTPULSE_DURATION	(80 * USEC_PER_MSEC)
	int boostpulse_duration;
	/* End time of boost pulse in ktime converted to usecs */
	u64 boostpulse_endtime;
	bool boosted;
	/* Minimun boostpulse interval */
#define DEFAULT_MIN_BOOSTPULSE_INTERVAL (500 * USEC_PER_MSEC)
	int boostpulse_min_interval;

	/*
	 * Max additional time to wait in idle, beyond timer_rate, at speeds
	 * above minimum before wakeup to reduce speed, or -1 if unnecessary.
	 */
#define DEFAULT_TIMER_SLACK (80 * USEC_PER_MSEC)
	int timer_slack;

	bool io_is_busy;
	unsigned int iowait_boost_step;
#ifdef CONFIG_FREQ_IO_LIMIT
	unsigned int iowait_upper_limit;
#endif

	bool fast_ramp_up;
	bool fast_ramp_down;
#define DEFAULT_FREQ_REPORTING_POLICY \
	(FREQ_STAT_CPU_LOAD | FREQ_STAT_TOP_TASK)
	unsigned int freq_reporting_policy;

#ifdef CONFIG_SCHED_TOP_TASK
	unsigned int top_task_hist_size;
	unsigned int top_task_stats_policy;
	bool top_task_stats_empty_window;
#endif

#ifdef CONFIG_ED_TASK
	unsigned int ed_task_running_duration;
	unsigned int ed_task_waiting_duration;
	unsigned int ed_new_task_running_duration;
#endif

#ifdef CONFIG_MIGRATION_NOTIFY
	unsigned int freq_inc_notify;
	unsigned int freq_dec_notify;
#endif
#endif
};

struct sugov_policy {
	struct cpufreq_policy	*policy;

	struct sugov_tunables	*tunables;
	struct list_head	tunables_hook;

	raw_spinlock_t		update_lock;	/* For shared policies */
	u64			last_freq_update_time;
	s64			freq_update_delay_ns;
	unsigned int		next_freq;
	unsigned int		cached_raw_freq;

	/* The next fields are only needed if fast switch cannot be used: */
	struct			irq_work irq_work;
	struct			kthread_work work;
	struct			mutex work_lock;
	struct			kthread_worker worker;
	struct task_struct	*thread;
	bool			work_in_progress;

	bool			limits_changed;
	bool			need_freq_update;

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	u64 overload_duration_ns;
	u64 floor_validate_time;
	u64 hispeed_validate_time;
	u64 update_time;
	/* protect slack timer */
	raw_spinlock_t timer_lock;
	/* policy slack timer */
	struct timer_list pol_slack_timer;
	unsigned int min_freq; /* in kHz */
	/* remember last active cpu and set slack timer on it */
	unsigned int trigger_cpu;
	/* max util cpu */
	unsigned int max_cpu;
	unsigned int iowait_boost;
	u64 last_iowait;
#ifdef CONFIG_SCHED_RTG
	unsigned long rtg_util;
	unsigned int rtg_freq;
#endif
	atomic_t skip_min_sample_time;
	atomic_t skip_hispeed_logic;
	bool util_changed;
	bool governor_enabled;
#endif
};

struct sugov_cpu {
	struct update_util_data	update_util;
	struct sugov_policy	*sg_policy;
	unsigned int		cpu;

	bool			iowait_boost_pending;
	unsigned int		iowait_boost;
	u64			last_update;

	unsigned long		bw_dl;
	unsigned long		max;

	/* The field below is for single-CPU policies only: */
#ifdef CONFIG_NO_HZ_COMMON
	unsigned long		saved_idle_calls;
#endif

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	unsigned long util;
	unsigned int flags;

	u64 idle_update_ts;
	u64 last_idle_time;

	unsigned int iowait_boost_max;
	unsigned int iowait_boost_min;
	unsigned int iowait_boost_step;

	bool use_max_freq;
	bool enabled;
#endif
};

static DEFINE_PER_CPU(struct sugov_cpu, sugov_cpu);
#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
static DEFINE_PER_CPU(struct sugov_tunables *, cached_tunables);
#endif

/************************ Governor internals ***********************/
#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
static inline unsigned int get_freq_reporting_policy(int cpu)
{
	struct sugov_cpu *sg_cpu = &per_cpu(sugov_cpu, cpu);
	struct sugov_policy *sg_policy;
	unsigned int policy;

	if (!sg_cpu->enabled)
		return DEFAULT_FREQ_REPORTING_POLICY;

	sg_policy = sg_cpu->sg_policy;
	if (!sg_policy || !sg_policy->governor_enabled)
		return DEFAULT_FREQ_REPORTING_POLICY;

	policy = sg_policy->tunables->freq_reporting_policy;

#ifdef CONFIG_SCHED_PRED_LOAD
	if (!predl_enable &&
	    (policy & FREQ_STAT_USE_PRED_WINDOW))
		return DEFAULT_FREQ_REPORTING_POLICY;
#endif

	return policy;
}

#ifdef CONFIG_SCHED_PRED_LOAD
bool use_pred_load(int cpu)
{
	if (!predl_enable)
		return false;

	return !!(get_freq_reporting_policy(cpu) & FREQ_STAT_USE_PRED_WINDOW);
}
#endif

static unsigned int freq_to_targetload(struct sugov_tunables *tunables,
				       unsigned int freq)
{
	unsigned long flags;
	unsigned int ret;
	int i;

	spin_lock_irqsave(&tunables->target_loads_lock, flags);

	for (i = 0; i < tunables->ntarget_loads - 1 &&
	     freq >= tunables->target_loads[i + 1]; i += 2)
		;

	ret = tunables->target_loads[i];
	spin_unlock_irqrestore(&tunables->target_loads_lock, flags);
	return ret;
}

static unsigned int freq_to_above_hispeed_delay(struct sugov_tunables *tunables,
				       unsigned int freq)
{
	unsigned long flags;
	unsigned int ret;
	int i;

	spin_lock_irqsave(&tunables->above_hispeed_delay_lock, flags);

	for (i = 0; i < tunables->nabove_hispeed_delay - 1 &&
	     freq >= tunables->above_hispeed_delay[i + 1]; i += 2)
		;

	ret = tunables->above_hispeed_delay[i];
	spin_unlock_irqrestore(&tunables->above_hispeed_delay_lock, flags);
	return ret;
}

static unsigned int freq_to_min_sample_time(struct sugov_tunables *tunables,
				       unsigned int freq)
{
	unsigned long flags;
	unsigned int ret;
	int i;

	spin_lock_irqsave(&tunables->min_sample_time_lock, flags);

	for (i = 0; i < tunables->nmin_sample_time - 1 &&
	     freq >= tunables->min_sample_time[i + 1]; i += 2)
		;

	ret = tunables->min_sample_time[i];
	spin_unlock_irqrestore(&tunables->min_sample_time_lock, flags);
	return ret;
}

/*
 * If increasing frequencies never map to a lower target load then
 * choose_freq() will find the minimum frequency that does not exceed its
 * target load given the current load.
 */
static unsigned int choose_freq(struct sugov_policy *sg_policy,
				unsigned int loadadjfreq)
{
	struct cpufreq_policy *policy = sg_policy->policy;
	struct cpufreq_frequency_table *freq_table = policy->freq_table;
	unsigned int prevfreq, freqmin = 0, freqmax = UINT_MAX, tl;
	unsigned int freq = policy->cur;
	int index;

	do {
		prevfreq = freq;
		tl = freq_to_targetload(sg_policy->tunables, freq);

		/*
		 * Find the lowest frequency where the computed load is less
		 * than or equal to the target load.
		 */

		index = cpufreq_frequency_table_target(policy, loadadjfreq / tl,
			    CPUFREQ_RELATION_L);

		freq = freq_table[index].frequency;

		if (freq > prevfreq) {
			/* The previous frequency is too low */
			freqmin = prevfreq;

			if (freq < freqmax)
				continue;

			/* Find highest frequency that is less than freqmax */
			index = cpufreq_frequency_table_target(policy,
				    freqmax - 1, CPUFREQ_RELATION_H);

			freq = freq_table[index].frequency;

			if (freq == freqmin) {
				/*
				 * The first frequency below freqmax has already
				 * been found to be too low. freqmax is the
				 * lowest speed we found that is fast enough.
				 */
				freq = freqmax;
				break;
			}
		} else if (freq < prevfreq) {
			/* The previous frequency is high enough. */
			freqmax = prevfreq;

			if (freq > freqmin)
				continue;

			/* Find lowest frequency that is higher than freqmin */
			index = cpufreq_frequency_table_target(policy,
				    freqmin + 1, CPUFREQ_RELATION_L);

			freq = freq_table[index].frequency;

			/*
			 * If freqmax is the first frequency above
			 * freqmin then we have already found that
			 * this speed is fast enough.
			 */
			if (freq == freqmax)
				break;
		}

		/* If same frequency chosen as previous then done. */
	} while (freq != prevfreq);

	return freq;
}

#ifdef CONFIG_SCHED_HISI_UTIL_CLAMP
static inline unsigned int cpu_min_util(int cpu)
{
	return cpu_rq(cpu)->uclamp[UCLAMP_MIN].value;
}

static unsigned int sg_min_util(const struct cpumask *cpus)
{
	unsigned int min_util = 0;
	int i;
	for_each_cpu(i, cpus)
		min_util = max(cpu_min_util(i), min_util);
	return min_util;
}
#else
static inline unsigned int cpu_min_util(int cpu) { return 0; }
#endif

/* Re-evaluate load to see if a frequency change is required or not */
static unsigned int eval_target_freq(struct sugov_policy *sg_policy,
				     unsigned long util, unsigned long max)
{
	int cpu_load = 0;
	unsigned int new_freq;
	struct sugov_tunables *tunables = sg_policy->tunables;
	struct cpufreq_policy *policy = sg_policy->policy;
	u64 now = ktime_to_us(ktime_get());

	tunables->boosted = now < tunables->boostpulse_endtime;

	if (tunables->boosted && policy->cur < tunables->hispeed_freq) {
		new_freq = tunables->hispeed_freq;
	} else {
		cpu_load = util * 100 / capacity_curr_of(policy->cpu);
		new_freq = choose_freq(sg_policy, cpu_load * policy->cur);

		if ((cpu_load >= tunables->go_hispeed_load || tunables->boosted) &&
		    new_freq < tunables->hispeed_freq)
			new_freq = tunables->hispeed_freq;
	}

	new_freq = max(util_to_freq(policy->cpu, sg_policy->iowait_boost), new_freq);
#ifdef CONFIG_SCHED_RTG
	new_freq = max(sg_policy->rtg_freq, new_freq);
#endif
#ifdef CONFIG_SCHED_HISI_UTIL_CLAMP
	/* Do not apply target_loads to min_util. */
	new_freq = max(util_to_freq(policy->cpu, sg_min_util(policy->cpus)), new_freq);
#endif

	trace_cpufreq_schedutil_eval_target(sg_policy->max_cpu,
					    util, max, cpu_load,
					    policy->cur, new_freq);

	return new_freq;
}

static void sugov_slack_timer_resched(struct sugov_policy *sg_policy)
{
	u64 expires;

	raw_spin_lock(&sg_policy->timer_lock);
	if (!sg_policy->governor_enabled)
		goto unlock;

	del_timer(&sg_policy->pol_slack_timer);
	if (sg_policy->tunables->timer_slack >= 0 &&
	    sg_policy->next_freq > sg_policy->policy->min) {
		expires = jiffies + usecs_to_jiffies(sg_policy->tunables->timer_slack);
		sg_policy->pol_slack_timer.expires = expires;
		add_timer_on(&sg_policy->pol_slack_timer, sg_policy->trigger_cpu);
	}
unlock:
	raw_spin_unlock(&sg_policy->timer_lock);
}
#endif

static bool sugov_should_update_freq(struct sugov_policy *sg_policy, u64 time)
{
	s64 delta_ns;

	/*
	 * Since cpufreq_update_util() is called with rq->lock held for
	 * the @target_cpu, our per-CPU data is fully serialized.
	 *
	 * However, drivers cannot in general deal with cross-CPU
	 * requests, so while get_next_freq() will work, our
	 * sugov_update_commit() call may not for the fast switching platforms.
	 *
	 * Hence stop here for remote requests if they aren't supported
	 * by the hardware, as calculating the frequency is pointless if
	 * we cannot in fact act on it.
	 *
	 * This is needed on the slow switching platforms too to prevent CPUs
	 * going offline from leaving stale IRQ work items behind.
	 */
	if (!cpufreq_this_cpu_can_update(sg_policy->policy))
		return false;

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	if (!sg_policy->governor_enabled)
		return false;

	/* When use WALT, depend on util_changed flag */
	if (likely(!use_pelt()))
		return sg_policy->util_changed;
#endif

	if (unlikely(sg_policy->limits_changed)) {
		sg_policy->limits_changed = false;
		sg_policy->need_freq_update = true;
		return true;
	}

	delta_ns = time - sg_policy->last_freq_update_time;

	return delta_ns >= sg_policy->freq_update_delay_ns;
}

static bool sugov_update_next_freq(struct sugov_policy *sg_policy, u64 time,
				   unsigned int next_freq)
{
#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	/* when use WALT, update sg_policy->next_freq in sugov_work */
	if (likely(!use_pelt()))
		return true;
#endif

	if (!sg_policy->need_freq_update) {
		if (sg_policy->next_freq == next_freq)
			return false;
	} else {
		sg_policy->need_freq_update = cpufreq_driver_test_flags(CPUFREQ_NEED_UPDATE_LIMITS);
	}

	sg_policy->next_freq = next_freq;
	sg_policy->last_freq_update_time = time;

	return true;
}

static void sugov_fast_switch(struct sugov_policy *sg_policy, u64 time,
			      unsigned int next_freq)
{
	if (sugov_update_next_freq(sg_policy, time, next_freq))
		cpufreq_driver_fast_switch(sg_policy->policy, next_freq);
}

static void sugov_deferred_update(struct sugov_policy *sg_policy, u64 time,
				  unsigned int next_freq)
{
	if (!sugov_update_next_freq(sg_policy, time, next_freq))
		return;

	if (!sg_policy->work_in_progress) {
#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
		sg_policy->update_time = time;
#endif
		sg_policy->work_in_progress = true;
		irq_work_queue(&sg_policy->irq_work);
	}
}

/**
 * get_next_freq - Compute a new frequency for a given cpufreq policy.
 * @sg_policy: schedutil policy object to compute the new frequency for.
 * @util: Current CPU utilization.
 * @max: CPU capacity.
 *
 * If the utilization is frequency-invariant, choose the new frequency to be
 * proportional to it, that is
 *
 * next_freq = C * max_freq * util / max
 *
 * Otherwise, approximate the would-be frequency-invariant utilization by
 * util_raw * (curr_freq / max_freq) which leads to
 *
 * next_freq = C * curr_freq * util_raw / max
 *
 * Take C = 1.25 for the frequency tipping point at (util / max) = 0.8.
 *
 * The lowest driver-supported frequency which is equal or greater than the raw
 * next_freq (as calculated above) is returned, subject to policy min/max and
 * cpufreq driver limitations.
 */
static unsigned int get_next_freq(struct sugov_policy *sg_policy,
				  unsigned long util, unsigned long max)
{
	struct cpufreq_policy *policy = sg_policy->policy;
	unsigned int freq = arch_scale_freq_invariant() ?
				policy->cpuinfo.max_freq : policy->cur;
	unsigned long next_freq = 0;

	trace_android_vh_map_util_freq(util, freq, max, &next_freq, policy,
			&sg_policy->need_freq_update);
	if (next_freq)
		freq = next_freq;
	else
#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
		freq = eval_target_freq(sg_policy, util, max);
#else
		freq = map_util_freq(util, freq, max);
#endif

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	if (likely(!use_pelt()))
		return freq;
#endif

	if (freq == sg_policy->cached_raw_freq && !sg_policy->need_freq_update)
		return sg_policy->next_freq;

	sg_policy->cached_raw_freq = freq;
	return cpufreq_driver_resolve_freq(policy, freq);
}

/*
 * This function computes an effective utilization for the given CPU, to be
 * used for frequency selection given the linear relation: f = u * f_max.
 *
 * The scheduler tracks the following metrics:
 *
 *   cpu_util_{cfs,rt,dl,irq}()
 *   cpu_bw_dl()
 *
 * Where the cfs,rt and dl util numbers are tracked with the same metric and
 * synchronized windows and are thus directly comparable.
 *
 * The cfs,rt,dl utilization are the running times measured with rq->clock_task
 * which excludes things like IRQ and steal-time. These latter are then accrued
 * in the irq utilization.
 *
 * The DL bandwidth number otoh is not a measured metric but a value computed
 * based on the task model parameters and gives the minimal utilization
 * required to meet deadlines.
 */
unsigned long schedutil_cpu_util(int cpu, unsigned long util_cfs,
				 unsigned long max, enum schedutil_type type,
				 struct task_struct *p)
{
	unsigned long dl_util, util, irq;
	struct rq *rq = cpu_rq(cpu);

	if (!uclamp_is_used() &&
	    type == FREQUENCY_UTIL && rt_rq_is_runnable(&rq->rt)) {
		return max;
	}

	/*
	 * Early check to see if IRQ/steal time saturates the CPU, can be
	 * because of inaccuracies in how we track these -- see
	 * update_irq_load_avg().
	 */
	irq = cpu_util_irq(rq);
	if (unlikely(irq >= max))
		return max;

	/*
	 * Because the time spend on RT/DL tasks is visible as 'lost' time to
	 * CFS tasks and we use the same metric to track the effective
	 * utilization (PELT windows are synchronized) we can directly add them
	 * to obtain the CPU's actual utilization.
	 *
	 * CFS and RT utilization can be boosted or capped, depending on
	 * utilization clamp constraints requested by currently RUNNABLE
	 * tasks.
	 * When there are no CFS RUNNABLE tasks, clamps are released and
	 * frequency will be gracefully reduced with the utilization decay.
	 */
	util = util_cfs + cpu_util_rt(rq);
	if (type == FREQUENCY_UTIL)
		util = uclamp_rq_util_with(rq, util, p);

	dl_util = cpu_util_dl(rq);

	/*
	 * For frequency selection we do not make cpu_util_dl() a permanent part
	 * of this sum because we want to use cpu_bw_dl() later on, but we need
	 * to check if the CFS+RT+DL sum is saturated (ie. no idle time) such
	 * that we select f_max when there is no idle time.
	 *
	 * NOTE: numerical errors or stop class might cause us to not quite hit
	 * saturation when we should -- something for later.
	 */
	if (util + dl_util >= max)
		return max;

	/*
	 * OTOH, for energy computation we need the estimated running time, so
	 * include util_dl and ignore dl_bw.
	 */
	if (type == ENERGY_UTIL)
		util += dl_util;

	/*
	 * There is still idle time; further improve the number by using the
	 * irq metric. Because IRQ/steal time is hidden from the task clock we
	 * need to scale the task numbers:
	 *
	 *              max - irq
	 *   U' = irq + --------- * U
	 *                 max
	 */
	util = scale_irq_capacity(util, irq, max);
	util += irq;

	/*
	 * Bandwidth required by DEADLINE must always be granted while, for
	 * FAIR and RT, we use blocked utilization of IDLE CPUs as a mechanism
	 * to gracefully reduce the frequency when no tasks show up for longer
	 * periods of time.
	 *
	 * Ideally we would like to set bw_dl as min/guaranteed freq and util +
	 * bw_dl as requested freq. However, cpufreq is not yet ready for such
	 * an interface. So, we only do the latter for now.
	 */
	if (type == FREQUENCY_UTIL)
		util += cpu_bw_dl(rq);

	return min(max, util);
}
EXPORT_SYMBOL_GPL(schedutil_cpu_util);

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
#ifdef CONFIG_SCHED_WALT
static inline unsigned long walt_cpu_util_freq(int cpu)
{
	u64 walt_cpu_util;

	walt_cpu_util = cpu_rq(cpu)->prev_runnable_sum;
	walt_cpu_util <<= SCHED_CAPACITY_SHIFT;
	do_div(walt_cpu_util, walt_ravg_window);

	return min_t(unsigned long, walt_cpu_util, capacity_orig_of(cpu));
}
#else
#define walt_cpu_util_freq(cpu) 0
#endif

unsigned long freq_policy_util(int cpu)
{
	struct rq *rq = cpu_rq(cpu);
	unsigned int reporting_policy = get_freq_reporting_policy(cpu);
	unsigned long util = 0;

	if (reporting_policy & FREQ_STAT_CPU_LOAD)
		util = max(util, walt_cpu_util_freq(cpu));

	if (reporting_policy & FREQ_STAT_TOP_TASK)
		util = max(util, top_task_util(rq));

	if (reporting_policy & FREQ_STAT_PRED_LOAD)
		util = max(util, predict_util(rq));

	if (reporting_policy & FREQ_STAT_MAX_PRED_LS)
		util = max(util, max_pred_ls(rq));

	if (reporting_policy & FREQ_STAT_PRED_CPU_LOAD_MIN)
		util = max(util, cpu_util_pred_min(rq));

	return util;
}
#endif

static unsigned long sugov_get_util(struct sugov_cpu *sg_cpu)
{
	struct rq *rq = cpu_rq(sg_cpu->cpu);
	unsigned long util = cpu_util_cfs(rq);
	unsigned long max = arch_scale_cpu_capacity(sg_cpu->cpu);

	sg_cpu->max = max;
	sg_cpu->bw_dl = cpu_bw_dl(rq);

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	if (likely(!use_pelt()))
		return freq_policy_util(sg_cpu->cpu);
#endif

	return schedutil_cpu_util(sg_cpu->cpu, util, max, FREQUENCY_UTIL, NULL);
}

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
#define IOWAIT_BOOST_INC_STEP 200000 /* 200MHz */
#define IOWAIT_BOOST_CLEAR_NS 8000000 /* 8ms */

bool sugov_iowait_boost_pending(int cpu)
{
	struct sugov_cpu *sg_cpu = &per_cpu(sugov_cpu, cpu);

	return sg_cpu->enabled && sg_cpu->iowait_boost_pending;
}
#endif

/**
 * sugov_iowait_reset() - Reset the IO boost status of a CPU.
 * @sg_cpu: the sugov data for the CPU to boost
 * @time: the update time from the caller
 * @set_iowait_boost: true if an IO boost has been requested
 *
 * The IO wait boost of a task is disabled after a tick since the last update
 * of a CPU. If a new IO wait boost is requested after more then a tick, then
 * we enable the boost starting from IOWAIT_BOOST_MIN, which improves energy
 * efficiency by ignoring sporadic wakeups from IO.
 */
static bool sugov_iowait_reset(struct sugov_cpu *sg_cpu, u64 time,
			       bool set_iowait_boost)
{
#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	/*
	 * For WALT, (time - sg_cpu->last_update) doesn't imply any
	 * information about how long the CPU has been idle. Use delta
	 * time from cluster's last SCHED_CPUFREQ_IOWAIT instead.
	 */
	s64 delta_ns = time - sg_cpu->sg_policy->last_iowait;

	if (delta_ns <= IOWAIT_BOOST_CLEAR_NS)
		return false;

	sg_cpu->iowait_boost = set_iowait_boost ? sg_cpu->iowait_boost_min : 0;
#else
	s64 delta_ns = time - sg_cpu->last_update;

	/* Reset boost only if a tick has elapsed since last request */
	if (delta_ns <= TICK_NSEC)
		return false;

	sg_cpu->iowait_boost = set_iowait_boost ? IOWAIT_BOOST_MIN : 0;
#endif
	sg_cpu->iowait_boost_pending = set_iowait_boost;

	return true;
}

/**
 * sugov_iowait_boost() - Updates the IO boost status of a CPU.
 * @sg_cpu: the sugov data for the CPU to boost
 * @time: the update time from the caller
 * @flags: SCHED_CPUFREQ_IOWAIT if the task is waking up after an IO wait
 *
 * Each time a task wakes up after an IO operation, the CPU utilization can be
 * boosted to a certain utilization which doubles at each "frequent and
 * successive" wakeup from IO, ranging from IOWAIT_BOOST_MIN to the utilization
 * of the maximum OPP.
 *
 * To keep doubling, an IO boost has to be requested at least once per tick,
 * otherwise we restart from the utilization of the minimum OPP.
 */
static void sugov_iowait_boost(struct sugov_cpu *sg_cpu, u64 time,
			       unsigned int flags)
{
	bool set_iowait_boost = flags & SCHED_CPUFREQ_IOWAIT;

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	if (set_iowait_boost || walt_cpu_overload_irqload(sg_cpu->cpu)) {
		sg_cpu->sg_policy->last_iowait = time;
		sg_cpu->iowait_boost_pending = true;

		/*
		 * Ignore pending and increase iowait_boost every time
		 * in a smooth way.
		 */
		if (sg_cpu->iowait_boost) {
			unsigned int boost_step = sg_cpu->iowait_boost_step;

			if (sched_io_is_busy != 0)
				boost_step >>= 1;

			sg_cpu->iowait_boost =
				min_t(unsigned int,
				      sg_cpu->iowait_boost + boost_step,
				      sg_cpu->iowait_boost_max);
		} else {
			/* First wakeup after IO: start with minimum boost */
			sg_cpu->iowait_boost = sg_cpu->iowait_boost_min;
		}
	} else if (sg_cpu->iowait_boost) {
		(void)sugov_iowait_reset(sg_cpu, time, set_iowait_boost);
	}
#else
	/* Reset boost if the CPU appears to have been idle enough */
	if (sg_cpu->iowait_boost &&
	    sugov_iowait_reset(sg_cpu, time, set_iowait_boost))
		return;

	/* Boost only tasks waking up after IO */
	if (!set_iowait_boost)
		return;

	/* Ensure boost doubles only one time at each request */
	if (sg_cpu->iowait_boost_pending)
		return;
	sg_cpu->iowait_boost_pending = true;

	/* Double the boost at each request */
	if (sg_cpu->iowait_boost) {
		sg_cpu->iowait_boost =
			min_t(unsigned int, sg_cpu->iowait_boost << 1, SCHED_CAPACITY_SCALE);
		return;
	}

	/* First wakeup after IO: start with minimum boost */
	sg_cpu->iowait_boost = IOWAIT_BOOST_MIN;
#endif
}

/**
 * sugov_iowait_apply() - Apply the IO boost to a CPU.
 * @sg_cpu: the sugov data for the cpu to boost
 * @time: the update time from the caller
 * @util: the utilization to (eventually) boost
 * @max: the maximum value the utilization can be boosted to
 *
 * A CPU running a task which woken up after an IO operation can have its
 * utilization boosted to speed up the completion of those IO operations.
 * The IO boost value is increased each time a task wakes up from IO, in
 * sugov_iowait_apply(), and it's instead decreased by this function,
 * each time an increase has not been requested (!iowait_boost_pending).
 *
 * A CPU which also appears to have been idle for at least one tick has also
 * its IO boost utilization reset.
 *
 * This mechanism is designed to boost high frequently IO waiting tasks, while
 * being more conservative on tasks which does sporadic IO operations.
 */
static unsigned long sugov_iowait_apply(struct sugov_cpu *sg_cpu, u64 time,
					unsigned long util, unsigned long max)
{
	unsigned long boost;

	/* No boost currently required */
	if (!sg_cpu->iowait_boost)
		return util;

	/* Reset boost if the CPU appears to have been idle enough */
	if (sugov_iowait_reset(sg_cpu, time, false))
		return util;

	if (!sg_cpu->iowait_boost_pending) {
		/*
		 * No boost pending; reduce the boost value.
		 */
		sg_cpu->iowait_boost >>= 1;
#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
		if (sg_cpu->iowait_boost < sg_cpu->iowait_boost_min) {
#else
		if (sg_cpu->iowait_boost < IOWAIT_BOOST_MIN) {
#endif
			sg_cpu->iowait_boost = 0;
			return util;
		}
	}

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	if (sg_cpu->iowait_boost > sg_cpu->sg_policy->iowait_boost)
		sg_cpu->sg_policy->iowait_boost = sg_cpu->iowait_boost;
#endif

	sg_cpu->iowait_boost_pending = false;

	/*
	 * @util is already in capacity scale; convert iowait_boost
	 * into the same scale so we can compare.
	 */
	boost = (sg_cpu->iowait_boost * max) >> SCHED_CAPACITY_SHIFT;
	return max(boost, util);
}

#ifdef CONFIG_NO_HZ_COMMON
static bool sugov_cpu_is_busy(struct sugov_cpu *sg_cpu)
{
	unsigned long idle_calls = tick_nohz_get_idle_calls_cpu(sg_cpu->cpu);
	bool ret = idle_calls == sg_cpu->saved_idle_calls;

	sg_cpu->saved_idle_calls = idle_calls;
	return ret;
}
#else
static inline bool sugov_cpu_is_busy(struct sugov_cpu *sg_cpu) { return false; }
#endif /* CONFIG_NO_HZ_COMMON */

/*
 * Make sugov_should_update_freq() ignore the rate limit when DL
 * has increased the utilization.
 */
static inline void ignore_dl_rate_limit(struct sugov_cpu *sg_cpu, struct sugov_policy *sg_policy)
{
	if (cpu_bw_dl(cpu_rq(sg_cpu->cpu)) > sg_cpu->bw_dl)
		sg_policy->limits_changed = true;
}

static void sugov_update_single(struct update_util_data *hook, u64 time,
				unsigned int flags)
{
	struct sugov_cpu *sg_cpu = container_of(hook, struct sugov_cpu, update_util);
	struct sugov_policy *sg_policy = sg_cpu->sg_policy;
	unsigned long util, max;
	unsigned int next_f;
	unsigned int cached_freq = sg_policy->cached_raw_freq;

	sugov_iowait_boost(sg_cpu, time, flags);
	sg_cpu->last_update = time;

	ignore_dl_rate_limit(sg_cpu, sg_policy);

	if (!sugov_should_update_freq(sg_policy, time))
		return;

	util = sugov_get_util(sg_cpu);
	max = sg_cpu->max;
	util = sugov_iowait_apply(sg_cpu, time, util, max);
	next_f = get_next_freq(sg_policy, util, max);
	/*
	 * Do not reduce the frequency if the CPU has not been idle
	 * recently, as the reduction is likely to be premature then.
	 */
	if (sugov_cpu_is_busy(sg_cpu) && next_f < sg_policy->next_freq) {
		next_f = sg_policy->next_freq;

		/* Restore cached freq as next_freq has changed */
		sg_policy->cached_raw_freq = cached_freq;
	}

	/*
	 * This code runs under rq->lock for the target CPU, so it won't run
	 * concurrently on two different CPUs for the same target and it is not
	 * necessary to acquire the lock in the fast switch case.
	 */
	if (sg_policy->policy->fast_switch_enabled) {
		sugov_fast_switch(sg_policy, time, next_f);
	} else {
		raw_spin_lock(&sg_policy->update_lock);
		sugov_deferred_update(sg_policy, time, next_f);
		raw_spin_unlock(&sg_policy->update_lock);
	}
}

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
static inline bool sugov_cpu_is_overload(struct sugov_cpu *sg_cpu)
{
	u64 idle_time, delta;

	if (cpu_rq(sg_cpu->cpu)->skip_overload_detect)
		return false;

	idle_time = get_cpu_idle_time(sg_cpu->cpu, NULL, 0);
	if (sg_cpu->last_idle_time != idle_time)
		sg_cpu->idle_update_ts = sg_cpu->sg_policy->update_time;

	sg_cpu->last_idle_time = idle_time;
	delta = sg_cpu->sg_policy->update_time - sg_cpu->idle_update_ts;

	return (delta > sg_cpu->sg_policy->overload_duration_ns);
}

static bool sugov_should_use_max_freq(struct sugov_cpu *sg_cpu)
{
	if (sugov_cpu_is_overload(sg_cpu))
		return true;

#ifdef CONFIG_ED_TASK
	if (cpu_rq(sg_cpu->cpu)->ed_task != NULL)
		return true;
#endif

	return false;
}

#ifdef CONFIG_CORE_CTRL
static void sugov_report_load(int cpu, unsigned long util)
{
	struct cpufreq_govinfo govinfo;

	/*
	 * Send govinfo notification.
	 * Govinfo notification could potentially wake up another thread
	 * managed by its clients. Thread wakeups might trigger a load
	 * change callback that executes this function again. Therefore
	 * no spinlock could be held when sending the notification.
	 */
	govinfo.cpu = cpu;
	govinfo.load = util * 100 / capacity_curr_of(cpu);
	govinfo.sampling_rate_us = 0;
	atomic_notifier_call_chain(&cpufreq_govinfo_notifier_list,
				   CPUFREQ_LOAD_CHANGE, &govinfo);
}
#endif

static unsigned int sugov_next_freq_shared_policy(struct sugov_policy *sg_policy)
{
	struct cpufreq_policy *policy = sg_policy->policy;
	unsigned long util = 0, max = 1;
	int j;

	sg_policy->max_cpu = cpumask_first(policy->cpus);
	sg_policy->iowait_boost = 0;

	for_each_cpu(j, policy->cpus) {
		struct sugov_cpu *j_sg_cpu = &per_cpu(sugov_cpu, j);
		unsigned long j_util, j_max;

		j_util = sugov_get_util(j_sg_cpu);
		j_sg_cpu->util = j_util;
		j_max = j_sg_cpu->max;

		if (j_sg_cpu->use_max_freq)
			j_util = j_max;

#ifdef CONFIG_CORE_CTRL
		sugov_report_load(j, j_util);
#endif
		if (j_util * max > j_max * util) {
			util = j_util;
			max = j_max;
			sg_policy->max_cpu = j;
		}

		/*
		 * Below load signal don't use target_loads,
		 * don't update util, only update the field of sg_policy
		 */
		j_util = sugov_iowait_apply(j_sg_cpu, sg_policy->update_time, j_util, j_max);

		trace_cpufreq_schedutil_get_util(j, j_sg_cpu->util,
						 j_sg_cpu->max,
						 top_task_util(cpu_rq(j)),
						 predict_util(cpu_rq(j)),
						 max_pred_ls(cpu_rq(j)),
						 cpu_util_pred_min(cpu_rq(j)),
						 j_sg_cpu->iowait_boost,
						 cpu_min_util(j),
						 j_sg_cpu->flags,
						 j_sg_cpu->use_max_freq);

		j_sg_cpu->flags = 0;
	}

#ifdef CONFIG_SCHED_RTG
	sched_get_max_group_util(policy->cpus, &sg_policy->rtg_util, &sg_policy->rtg_freq);
	util = max(sg_policy->rtg_util, util);
#endif

	return get_next_freq(sg_policy, util, max);
}
#endif

static unsigned int sugov_next_freq_shared(struct sugov_cpu *sg_cpu, u64 time)
{
	struct sugov_policy *sg_policy = sg_cpu->sg_policy;
	struct cpufreq_policy *policy = sg_policy->policy;
	unsigned long util = 0, max = 1;
	unsigned int j;

	for_each_cpu(j, policy->cpus) {
		struct sugov_cpu *j_sg_cpu = &per_cpu(sugov_cpu, j);
		unsigned long j_util, j_max;

		j_util = sugov_get_util(j_sg_cpu);
		j_max = j_sg_cpu->max;
		j_util = sugov_iowait_apply(j_sg_cpu, time, j_util, j_max);

		if (j_util * max > j_max * util) {
			util = j_util;
			max = j_max;
		}
	}

	return get_next_freq(sg_policy, util, max);
}

static void
sugov_update_shared(struct update_util_data *hook, u64 time, unsigned int flags)
{
	struct sugov_cpu *sg_cpu = container_of(hook, struct sugov_cpu, update_util);
	struct sugov_policy *sg_policy = sg_cpu->sg_policy;
	unsigned int next_f;
#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	unsigned long irq_flag;
#endif

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	raw_spin_lock_irqsave(&sg_policy->update_lock, irq_flag);
#else
	raw_spin_lock(&sg_policy->update_lock);
#endif
	sugov_iowait_boost(sg_cpu, time, flags);
	sg_cpu->last_update = time;

	ignore_dl_rate_limit(sg_cpu, sg_policy);

	if (sugov_should_update_freq(sg_policy, time)) {
#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
		if (likely(!use_pelt())) {
			sg_policy->util_changed = false;
			sg_policy->trigger_cpu = sg_cpu->cpu;
			next_f = 0;
		} else
			next_f = sugov_next_freq_shared(sg_cpu, time);
#else
		next_f = sugov_next_freq_shared(sg_cpu, time);
#endif
		if (sg_policy->policy->fast_switch_enabled)
			sugov_fast_switch(sg_policy, time, next_f);
		else
			sugov_deferred_update(sg_policy, time, next_f);
	}

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	raw_spin_unlock_irqrestore(&sg_policy->update_lock, irq_flag);
#else
	raw_spin_unlock(&sg_policy->update_lock);
#endif
}

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
static bool sugov_time_limit(struct sugov_policy *sg_policy, unsigned int next_freq,
			     int skip_min_sample_time, int skip_hispeed_logic)
{
	u64 delta_ns;
	unsigned int min_sample_time;

	if (!skip_hispeed_logic &&
	    next_freq > sg_policy->next_freq &&
	    sg_policy->next_freq >= sg_policy->tunables->hispeed_freq) {
		delta_ns = sg_policy->update_time - sg_policy->hispeed_validate_time;
		if (delta_ns < NSEC_PER_USEC *
		    freq_to_above_hispeed_delay(sg_policy->tunables, sg_policy->next_freq)) {
			trace_cpufreq_schedutil_notyet(sg_policy->max_cpu,
						       "above_hispeed_delay", delta_ns,
						       sg_policy->next_freq, next_freq);
			return true;
		}
	}

	sg_policy->hispeed_validate_time = sg_policy->update_time;
	/*
	 * Do not scale below floor_freq unless we have been at or above the
	 * floor frequency for the minimum sample time since last validated.
	 */
	if (next_freq < sg_policy->next_freq) {
		min_sample_time = freq_to_min_sample_time(sg_policy->tunables, sg_policy->next_freq);

		if (!skip_min_sample_time) {
			delta_ns = sg_policy->update_time - sg_policy->floor_validate_time;
			if (delta_ns < NSEC_PER_USEC * min_sample_time) {
				trace_cpufreq_schedutil_notyet(sg_policy->max_cpu,
						       "min_sample_time", delta_ns,
						       sg_policy->next_freq, next_freq);
				return true;
			}
		}
	}

	if (!sg_policy->tunables->boosted ||
	    next_freq > sg_policy->tunables->hispeed_freq)
		sg_policy->floor_validate_time = sg_policy->update_time;

	return false;
}

unsigned int check_freq_reporting_policy(int cpu, unsigned int flags)
{
	unsigned int reporting_policy = get_freq_reporting_policy(cpu);
	unsigned int ignore_events = 0;

	if (reporting_policy & FREQ_STAT_USE_PRED_WINDOW)
		ignore_events |= WALT_WINDOW_ROLLOVER;
	else
		ignore_events |= (PRED_LOAD_WINDOW_ROLLOVER |
				PRED_LOAD_CHANGE | PRED_LOAD_ENQUEUE);

	if (!(reporting_policy & FREQ_STAT_TOP_TASK))
		ignore_events |= ADD_TOP_TASK;

	if (!(reporting_policy & FREQ_STAT_PRED_LOAD))
		ignore_events |= PRED_LOAD_ENQUEUE;

	return flags & (~ignore_events);
}

void sugov_mark_util_change(int cpu, unsigned int flags)
{
	struct sugov_cpu *sg_cpu = &per_cpu(sugov_cpu, cpu);
	struct sugov_policy *sg_policy;
	bool skip_min_sample_time = false;
	bool skip_hispeed_logic = false;

	if (!sg_cpu->enabled)
		return;

	sg_policy = sg_cpu->sg_policy;
	if (!sg_policy || !sg_policy->governor_enabled)
		return;

	flags = check_freq_reporting_policy(cpu, flags);
	if (!flags)
		return;

	sg_cpu->flags |= flags;

	if (flags & INTER_CLUSTER_MIGRATION_SRC)
		if (sg_policy->tunables->fast_ramp_down)
			skip_min_sample_time = true;

	if (flags & INTER_CLUSTER_MIGRATION_DST)
		if (sg_policy->tunables->fast_ramp_up)
			skip_hispeed_logic = true;

#ifdef CONFIG_ED_TASK
	if (flags & CLEAR_ED_TASK)
		skip_min_sample_time = true;

	if (flags & ADD_ED_TASK)
		skip_hispeed_logic = true;
#endif

#ifdef CONFIG_SCHED_TOP_TASK_SKIP_HISPEED_LOGIC
	if (flags & ADD_TOP_TASK)
		skip_hispeed_logic = true;
#endif

	if (flags & FORCE_UPDATE) {
		skip_min_sample_time = true;
		skip_hispeed_logic = true;
	}

#ifdef CONFIG_SCHED_RTG
	if (flags & FRAME_UPDATE) {
		/*
		 * Boosted scene like appstart do not want to fast
		 * ramp down. FRAME_UPDATE should not break it.
		 */
		if (sg_policy->tunables->fast_ramp_down)
			skip_min_sample_time = true;
		skip_hispeed_logic = true;
	}
#endif

#ifdef CONFIG_SCHED_HISI_UTIL_CLAMP
	if (flags & (SET_MIN_UTIL | ENQUEUE_MIN_UTIL))
		skip_hispeed_logic = true;
#endif

	if (skip_min_sample_time)
		atomic_set(&sg_policy->skip_min_sample_time, 1);
	if (skip_hispeed_logic)
		atomic_set(&sg_policy->skip_hispeed_logic, 1);

	sg_policy->util_changed = true;
}

void sugov_check_freq_update(int cpu)
{
	struct sugov_cpu *sg_cpu;
	struct sugov_policy *sg_policy;

	if (cpu >= nr_cpu_ids)
		return;

	sg_cpu = &per_cpu(sugov_cpu, cpu);
	if (!sg_cpu->enabled)
		return;

	sg_policy = sg_cpu->sg_policy;
	if (!sg_policy || !sg_policy->governor_enabled)
		return;

	if (sg_policy->util_changed)
		cpufreq_update_util(cpu_rq(cpu), SCHED_CPUFREQ_WALT);
}

static void walt_sugov_work(struct kthread_work *work)
{
	struct sugov_policy *sg_policy = container_of(work, struct sugov_policy, work);
	struct cpufreq_policy *policy = sg_policy->policy;
	unsigned int next_freq, cpu;
	struct rq *rq;
	int skip_min_sample_time, skip_hispeed_logic;

	mutex_lock(&sg_policy->work_lock);

	for_each_cpu(cpu, policy->cpus) {
		struct sugov_cpu *j_sg_cpu = &per_cpu(sugov_cpu, cpu);

		if (!j_sg_cpu->enabled)
			goto out;

		rq = cpu_rq(cpu);
		raw_spin_rq_lock_irq(rq);

		j_sg_cpu->use_max_freq = sugov_should_use_max_freq(j_sg_cpu);
		if (j_sg_cpu->use_max_freq)
			atomic_set(&sg_policy->skip_hispeed_logic, 1);
		else
			walt_update_task_ravg(rq->curr, rq, TASK_UPDATE, walt_ktime_clock(), 0);

		raw_spin_rq_unlock_irq(rq);
	}

	/* Clear util changes marked above in walt update. */
	sg_policy->util_changed = false;

	/* Allow a new sugov_work to be queued now. */
	sg_policy->work_in_progress = false;

	/*
	 * Before we getting util, read and clear the skip_xxx requirements
	 * for this update.
	 *
	 * Otherwise,
	 * sugov_work():               on cpuX:
	 *   get cpuX's util(high)
	 *                               lower down cpuX's util(e.g. migration)
	 *                               set skip_min_sample_time
	 *   read and clear skip_xxx
	 *   set freq(high)
	 *                               another sugov_work()
	 *                               limited by min_sample_time
	 *
	 * In the following order we will make it consistent:
	 *   atomic_xchg(&skip_xxx, 0)   update any cpu util
	 *   get all cpu's util          atomic_set(&skip_xxx, 1)
	 *   ...                         ...
	 *   adjust frequency            adjust frequency
	 */
	skip_min_sample_time = atomic_xchg(&sg_policy->skip_min_sample_time, 0);
	skip_hispeed_logic = atomic_xchg(&sg_policy->skip_hispeed_logic, 0);

	next_freq = sugov_next_freq_shared_policy(sg_policy);

	if (sugov_time_limit(sg_policy, next_freq, skip_min_sample_time, skip_hispeed_logic))
		goto out;

	if (next_freq == sg_policy->next_freq) {
		trace_cpufreq_schedutil_already(sg_policy->max_cpu,
						sg_policy->next_freq, next_freq);
		goto out;
	}

	sg_policy->next_freq = next_freq;
	sg_policy->last_freq_update_time = sg_policy->update_time;

	__cpufreq_driver_target(sg_policy->policy, sg_policy->next_freq,
				CPUFREQ_RELATION_L);

out:
	mutex_unlock(&sg_policy->work_lock);

	sugov_slack_timer_resched(sg_policy);

	blocking_notifier_call_chain(&sugov_status_notifier_list,
			SUGOV_ACTIVE, &sg_policy->trigger_cpu);
}
#endif

static void sugov_work(struct kthread_work *work)
{
	struct sugov_policy *sg_policy = container_of(work, struct sugov_policy, work);
	unsigned int freq;
	unsigned long flags;

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	if (likely(!use_pelt())) {
		walt_sugov_work(work);
		return;
	}
#endif

	/*
	 * Hold sg_policy->update_lock shortly to handle the case where:
	 * incase sg_policy->next_freq is read here, and then updated by
	 * sugov_deferred_update() just before work_in_progress is set to false
	 * here, we may miss queueing the new update.
	 *
	 * Note: If a work was queued after the update_lock is released,
	 * sugov_work() will just be called again by kthread_work code; and the
	 * request will be proceed before the sugov thread sleeps.
	 */
	raw_spin_lock_irqsave(&sg_policy->update_lock, flags);
	freq = sg_policy->next_freq;
	sg_policy->work_in_progress = false;
	raw_spin_unlock_irqrestore(&sg_policy->update_lock, flags);

	mutex_lock(&sg_policy->work_lock);
	__cpufreq_driver_target(sg_policy->policy, freq, CPUFREQ_RELATION_L);
	mutex_unlock(&sg_policy->work_lock);
}

static void sugov_irq_work(struct irq_work *irq_work)
{
	struct sugov_policy *sg_policy;

	sg_policy = container_of(irq_work, struct sugov_policy, irq_work);

	kthread_queue_work(&sg_policy->worker, &sg_policy->work);
}

/************************** sysfs interface ************************/

static struct sugov_tunables *global_tunables;
static DEFINE_MUTEX(global_tunables_lock);

static inline struct sugov_tunables *to_sugov_tunables(struct gov_attr_set *attr_set)
{
	return container_of(attr_set, struct sugov_tunables, attr_set);
}

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
#define show_one(file_name, object, format)		\
static ssize_t file_name##_show				\
(struct gov_attr_set *attr_set, char *buf)		\
{							\
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);	\
	return scnprintf(buf, PAGE_SIZE, format, tunables->object);\
}

show_one(overload_duration, overload_duration, "%u\n");

static ssize_t overload_duration_store(struct gov_attr_set *attr_set,
		const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	struct sugov_policy *sg_policy;
	unsigned int val;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	tunables->overload_duration = val;

	list_for_each_entry(sg_policy, &attr_set->policy_list, tunables_hook)
		sg_policy->overload_duration_ns = val * NSEC_PER_MSEC;

	return count;
}

show_one(go_hispeed_load, go_hispeed_load, "%u\n");

static ssize_t go_hispeed_load_store(struct gov_attr_set *attr_set,
		const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	unsigned int val;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	tunables->go_hispeed_load = val;

	return count;
}

show_one(hispeed_freq, hispeed_freq, "%u\n");

static ssize_t hispeed_freq_store(struct gov_attr_set *attr_set,
		const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	unsigned int val;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	tunables->hispeed_freq = val;

	return count;
}

static void sugov_boost(struct gov_attr_set *attr_set)
{
	struct sugov_policy *sg_policy;
	u64 now;

	now = use_pelt() ? ktime_get_ns() : walt_ktime_clock();

	list_for_each_entry(sg_policy, &attr_set->policy_list, tunables_hook) {
		sugov_deferred_update(sg_policy, now, sg_policy->tunables->hispeed_freq);
	}
}

static ssize_t boostpulse_store(struct gov_attr_set *attr_set,
		const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	unsigned int val;
	u64 now;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	now = ktime_to_us(ktime_get());
	if (tunables->boostpulse_endtime + tunables->boostpulse_min_interval > now)
		return count;

	tunables->boostpulse_endtime = now + tunables->boostpulse_duration;
	trace_cpufreq_schedutil_boost("pulse");

	if (!tunables->boosted)
		sugov_boost(attr_set);

	return count;
}

show_one(boostpulse_duration, boostpulse_duration, "%u\n");

static ssize_t boostpulse_duration_store(struct gov_attr_set *attr_set,
		const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	unsigned int val;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	tunables->boostpulse_duration = val;

	return count;
}

show_one(io_is_busy, io_is_busy, "%u\n");

static ssize_t io_is_busy_store(struct gov_attr_set *attr_set,
		const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	unsigned int val;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	tunables->io_is_busy = val;
	sched_set_io_is_busy(val);

	return count;
}

show_one(fast_ramp_down, fast_ramp_down, "%u\n");

static ssize_t fast_ramp_down_store(struct gov_attr_set *attr_set,
		const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	unsigned int val;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	tunables->fast_ramp_down = val;

	return count;
}

show_one(fast_ramp_up, fast_ramp_up, "%u\n");

static ssize_t fast_ramp_up_store(struct gov_attr_set *attr_set,
		const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	unsigned int val;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	tunables->fast_ramp_up = val;

	return count;
}

show_one(freq_reporting_policy, freq_reporting_policy, "%u\n");

static ssize_t freq_reporting_policy_store(struct gov_attr_set *attr_set,
		const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	unsigned int val;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	tunables->freq_reporting_policy = val;

	return count;
}

#ifdef CONFIG_MIGRATION_NOTIFY
show_one(freq_inc_notify, freq_inc_notify, "%u\n");

static ssize_t freq_inc_notify_store(struct gov_attr_set *attr_set,
				      const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	struct sugov_policy *sg_policy;
	unsigned int val;
	int cpu;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	tunables->freq_inc_notify = val;

	list_for_each_entry(sg_policy, &attr_set->policy_list, tunables_hook) {
		for_each_cpu(cpu, sg_policy->policy->cpus)
			cpu_rq(cpu)->freq_inc_notify = val;
	}

	return count;
}

show_one(freq_dec_notify, freq_dec_notify, "%u\n");

static ssize_t freq_dec_notify_store(struct gov_attr_set *attr_set,
				      const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	struct sugov_policy *sg_policy;
	unsigned int val;
	int cpu;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	tunables->freq_dec_notify = val;

	list_for_each_entry(sg_policy, &attr_set->policy_list, tunables_hook) {
		for_each_cpu(cpu, sg_policy->policy->cpus)
			cpu_rq(cpu)->freq_dec_notify = val;
	}

	return count;
}
#endif /* CONFIG_MIGRATION_NOTIFY */

#ifdef CONFIG_SCHED_TOP_TASK
show_one(top_task_hist_size, top_task_hist_size, "%u\n");

static ssize_t top_task_hist_size_store(struct gov_attr_set *attr_set,
				      const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	struct sugov_policy *sg_policy;
	unsigned int val;
	int cpu;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	/* Allowed range: [1, RAVG_HIST_SIZE_MAX] */
	if (val < 1 || val > RAVG_HIST_SIZE_MAX)
		return -EINVAL;

	tunables->top_task_hist_size = val;

	list_for_each_entry(sg_policy, &attr_set->policy_list, tunables_hook) {
		for_each_cpu(cpu, sg_policy->policy->cpus)
			cpu_rq(cpu)->top_task_hist_size = val;
	}

	return count;
}

show_one(top_task_stats_policy, top_task_stats_policy, "%u\n");

static ssize_t top_task_stats_policy_store(struct gov_attr_set *attr_set,
				      const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	struct sugov_policy *sg_policy;
	unsigned int val;
	int cpu;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	tunables->top_task_stats_policy = val;

	list_for_each_entry(sg_policy, &attr_set->policy_list, tunables_hook) {
		for_each_cpu(cpu, sg_policy->policy->cpus)
			cpu_rq(cpu)->top_task_stats_policy = val;
	}

	return count;
}

show_one(top_task_stats_empty_window, top_task_stats_empty_window, "%u\n");

static ssize_t top_task_stats_empty_window_store(struct gov_attr_set *attr_set,
				      const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	struct sugov_policy *sg_policy;
	unsigned int val;
	int cpu;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	tunables->top_task_stats_empty_window = val;

	list_for_each_entry(sg_policy, &attr_set->policy_list, tunables_hook) {
		for_each_cpu(cpu, sg_policy->policy->cpus)
			cpu_rq(cpu)->top_task_stats_empty_window = val;
	}

	return count;
}
#endif /* CONFIG_SCHED_TOP_TASK */

#ifdef CONFIG_ED_TASK
show_one(ed_task_running_duration, ed_task_running_duration, "%u\n");

static ssize_t ed_task_running_duration_store(struct gov_attr_set *attr_set,
				      const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	struct sugov_policy *sg_policy;
	unsigned int val;
	int cpu;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	tunables->ed_task_running_duration = val;

	list_for_each_entry(sg_policy, &attr_set->policy_list, tunables_hook) {
		for_each_cpu(cpu, sg_policy->policy->cpus)
			cpu_rq(cpu)->ed_task_running_duration = val;
	}

	return count;
}

show_one(ed_task_waiting_duration, ed_task_waiting_duration, "%u\n");

static ssize_t ed_task_waiting_duration_store(struct gov_attr_set *attr_set,
				      const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	struct sugov_policy *sg_policy;
	unsigned int val;
	int cpu;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	tunables->ed_task_waiting_duration = val;

	list_for_each_entry(sg_policy, &attr_set->policy_list, tunables_hook) {
		for_each_cpu(cpu, sg_policy->policy->cpus)
			cpu_rq(cpu)->ed_task_waiting_duration = val;
	}

	return count;
}

show_one(ed_new_task_running_duration, ed_new_task_running_duration, "%u\n");

static ssize_t ed_new_task_running_duration_store(struct gov_attr_set *attr_set,
				      const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	struct sugov_policy *sg_policy;
	unsigned int val;
	int cpu;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	tunables->ed_new_task_running_duration = val;

	list_for_each_entry(sg_policy, &attr_set->policy_list, tunables_hook) {
		for_each_cpu(cpu, sg_policy->policy->cpus)
			cpu_rq(cpu)->ed_new_task_running_duration = val;
	}

	return count;
}
#endif /* CONFIG_ED_TASK */

static unsigned int *get_tokenized_data(const char *buf, int *num_tokens)
{
	const char *cp;
	int i;
	int ntokens = 1;
	unsigned int *tokenized_data;
	int err = -EINVAL;

	cp = buf;
	while ((cp = strpbrk(cp + 1, " :")))
		ntokens++;

	if (!(ntokens & 0x1))
		goto err;

	tokenized_data = kmalloc(ntokens * sizeof(unsigned int), GFP_KERNEL);
	if (!tokenized_data) {
		err = -ENOMEM;
		goto err;
	}

	cp = buf;
	i = 0;
	while (i < ntokens) {
		if (sscanf(cp, "%u", &tokenized_data[i++]) != 1) /* [false alarm]:fortify */
			goto err_kfree;

		cp = strpbrk(cp, " :");
		if (!cp)
			break;
		cp++;
	}

	if (i != ntokens)
		goto err_kfree;

	*num_tokens = ntokens;
	return tokenized_data;

err_kfree:
	kfree(tokenized_data);
err:
	return ERR_PTR(err);
}

static ssize_t target_loads_show(struct gov_attr_set *attr_set, char *buf)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	unsigned long flags;
	ssize_t ret = 0;
	int i;

	spin_lock_irqsave(&tunables->target_loads_lock, flags);

	for (i = 0; i < tunables->ntarget_loads; i++)
		ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%u%s", tunables->target_loads[i],
			       i & 0x1 ? ":" : " ");

	scnprintf(buf + ret - 1, PAGE_SIZE - ret + 1, "\n");
	spin_unlock_irqrestore(&tunables->target_loads_lock, flags);

	return ret;
}

static ssize_t target_loads_store(struct gov_attr_set *attr_set,
				  const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	unsigned int *new_target_loads;
	unsigned long flags;
	int ntokens;
	int i;

	new_target_loads = get_tokenized_data(buf, &ntokens);
	if (IS_ERR(new_target_loads))
		return PTR_ERR(new_target_loads);

	for (i = 0; i < ntokens; i++) {
		if (new_target_loads[i] == 0) {
			kfree(new_target_loads);
			return -EINVAL;
		}
	}

	spin_lock_irqsave(&tunables->target_loads_lock, flags);
	if (tunables->target_loads != default_target_loads)
		kfree(tunables->target_loads);
	tunables->target_loads = new_target_loads;
	tunables->ntarget_loads = ntokens;
	spin_unlock_irqrestore(&tunables->target_loads_lock, flags);

	return count;
}

static ssize_t above_hispeed_delay_show(struct gov_attr_set *attr_set, char *buf)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	unsigned long flags;
	ssize_t ret = 0;
	int i;

	spin_lock_irqsave(&tunables->above_hispeed_delay_lock, flags);

	for (i = 0; i < tunables->nabove_hispeed_delay; i++)
		ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%u%s", tunables->above_hispeed_delay[i],
			       i & 0x1 ? ":" : " ");

	scnprintf(buf + ret - 1, PAGE_SIZE - ret + 1, "\n");
	spin_unlock_irqrestore(&tunables->above_hispeed_delay_lock, flags);

	return ret;
}

static ssize_t above_hispeed_delay_store(struct gov_attr_set *attr_set,
				  const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	unsigned int *new_above_hispeed_delay;
	unsigned long flags;
	int ntokens;

	new_above_hispeed_delay = get_tokenized_data(buf, &ntokens);
	if (IS_ERR(new_above_hispeed_delay))
		return PTR_ERR(new_above_hispeed_delay);

	spin_lock_irqsave(&tunables->above_hispeed_delay_lock, flags);
	if (tunables->above_hispeed_delay != default_above_hispeed_delay)
		kfree(tunables->above_hispeed_delay);
	tunables->above_hispeed_delay = new_above_hispeed_delay;
	tunables->nabove_hispeed_delay = ntokens;
	spin_unlock_irqrestore(&tunables->above_hispeed_delay_lock, flags);

	return count;
}

static ssize_t min_sample_time_show(struct gov_attr_set *attr_set, char *buf)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	unsigned long flags;
	ssize_t ret = 0;
	int i;

	spin_lock_irqsave(&tunables->min_sample_time_lock, flags);

	for (i = 0; i < tunables->nmin_sample_time; i++)
		ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%u%s", tunables->min_sample_time[i],
			       i & 0x1 ? ":" : " ");

	scnprintf(buf + ret - 1, PAGE_SIZE - ret + 1, "\n");
	spin_unlock_irqrestore(&tunables->min_sample_time_lock, flags);

	return ret;
}

static ssize_t min_sample_time_store(struct gov_attr_set *attr_set,
				  const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	unsigned int *new_min_sample_time;
	unsigned long flags;
	int ntokens;

	new_min_sample_time = get_tokenized_data(buf, &ntokens);
	if (IS_ERR(new_min_sample_time))
		return PTR_ERR(new_min_sample_time);

	spin_lock_irqsave(&tunables->min_sample_time_lock, flags);
	if (tunables->min_sample_time != default_min_sample_time)
		kfree(tunables->min_sample_time);
	tunables->min_sample_time = new_min_sample_time;
	tunables->nmin_sample_time = ntokens;
	spin_unlock_irqrestore(&tunables->min_sample_time_lock, flags);

	return count;
}

show_one(timer_slack, timer_slack, "%u\n");

static ssize_t timer_slack_store(struct gov_attr_set *attr_set,
				  const char *buf, size_t count)
{
	int ret;
	unsigned long val;
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);

	ret = kstrtol(buf, 10, &val);
	if (ret < 0)
		return ret;

	tunables->timer_slack = val;
	return count;
}

show_one(iowait_boost_step, iowait_boost_step, "%u\n");

static ssize_t iowait_boost_step_store(struct gov_attr_set *attr_set,
				       const char *buf, size_t count)
{
	int ret, cpu;
	unsigned int val;
	struct sugov_policy *sg_policy = NULL;
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);

	ret = kstrtouint(buf, 10, &val);
	if (ret < 0)
		return ret;

	tunables->iowait_boost_step = val;

	list_for_each_entry(sg_policy, &attr_set->policy_list, tunables_hook) {
		if (sg_policy->tunables == tunables) {
			for_each_cpu(cpu, sg_policy->policy->cpus) {
				struct sugov_cpu *sg_cpu = &per_cpu(sugov_cpu, cpu);

				sg_cpu->iowait_boost_step = freq_to_util(cpu, tunables->iowait_boost_step);
			}
			break;
		}
	}

	return count;
}

#ifdef CONFIG_FREQ_IO_LIMIT
show_one(iowait_upper_limit, iowait_upper_limit, "%u\n");

static ssize_t iowait_upper_limit_store(struct gov_attr_set *attr_set, const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	struct sugov_policy *sg_policy = NULL;
	unsigned int val;
	int cpu;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	tunables->iowait_upper_limit = val;

	list_for_each_entry(sg_policy, &attr_set->policy_list, tunables_hook) {
		if (sg_policy->tunables == tunables) {
			for_each_cpu(cpu, sg_policy->policy->cpus) {
				struct sugov_cpu *sg_cpu = &per_cpu(sugov_cpu, cpu);

				sg_cpu->iowait_boost_max = freq_to_util(cpu, tunables->iowait_upper_limit);
			}
			break;
		}
	}

	return count;
}
#endif
#endif

static ssize_t rate_limit_us_show(struct gov_attr_set *attr_set, char *buf)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);

	return sprintf(buf, "%u\n", tunables->rate_limit_us);
}

static ssize_t
rate_limit_us_store(struct gov_attr_set *attr_set, const char *buf, size_t count)
{
	struct sugov_tunables *tunables = to_sugov_tunables(attr_set);
	struct sugov_policy *sg_policy;
	unsigned int rate_limit_us;

	if (kstrtouint(buf, 10, &rate_limit_us))
		return -EINVAL;

	tunables->rate_limit_us = rate_limit_us;

	list_for_each_entry(sg_policy, &attr_set->policy_list, tunables_hook)
		sg_policy->freq_update_delay_ns = rate_limit_us * NSEC_PER_USEC;

	return count;
}

static struct governor_attr rate_limit_us = __ATTR_RW(rate_limit_us);

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
static struct governor_attr overload_duration = __ATTR_RW(overload_duration);
static struct governor_attr go_hispeed_load = __ATTR_RW(go_hispeed_load);
static struct governor_attr hispeed_freq = __ATTR_RW(hispeed_freq);
static struct governor_attr target_loads = __ATTR_RW(target_loads);
static struct governor_attr above_hispeed_delay = __ATTR_RW(above_hispeed_delay);
static struct governor_attr min_sample_time = __ATTR_RW(min_sample_time);
static struct governor_attr boostpulse = __ATTR_WO(boostpulse);
static struct governor_attr boostpulse_duration = __ATTR_RW(boostpulse_duration);
static struct governor_attr io_is_busy = __ATTR_RW(io_is_busy);
static struct governor_attr timer_slack = __ATTR_RW(timer_slack);
static struct governor_attr fast_ramp_down = __ATTR_RW(fast_ramp_down);
static struct governor_attr fast_ramp_up = __ATTR_RW(fast_ramp_up);
static struct governor_attr freq_reporting_policy = __ATTR_RW(freq_reporting_policy);
static struct governor_attr iowait_boost_step = __ATTR_RW(iowait_boost_step);
#ifdef CONFIG_SCHED_TOP_TASK
static struct governor_attr top_task_hist_size = __ATTR_RW(top_task_hist_size);
static struct governor_attr top_task_stats_policy = __ATTR_RW(top_task_stats_policy);
static struct governor_attr top_task_stats_empty_window = __ATTR_RW(top_task_stats_empty_window);
#endif
#ifdef CONFIG_ED_TASK
static struct governor_attr ed_task_running_duration = __ATTR_RW(ed_task_running_duration);
static struct governor_attr ed_task_waiting_duration = __ATTR_RW(ed_task_waiting_duration);
static struct governor_attr ed_new_task_running_duration = __ATTR_RW(ed_new_task_running_duration);
#endif
#ifdef CONFIG_MIGRATION_NOTIFY
static struct governor_attr freq_inc_notify = __ATTR_RW(freq_inc_notify);
static struct governor_attr freq_dec_notify = __ATTR_RW(freq_dec_notify);
#endif
#ifdef CONFIG_FREQ_IO_LIMIT
static struct governor_attr iowait_upper_limit = __ATTR_RW(iowait_upper_limit);
#endif
#endif

static struct attribute *sugov_attrs[] = {
	&rate_limit_us.attr,
#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	&overload_duration.attr,
	&go_hispeed_load.attr,
	&hispeed_freq.attr,
	&target_loads.attr,
	&above_hispeed_delay.attr,
	&min_sample_time.attr,
	&boostpulse.attr,
	&boostpulse_duration.attr,
	&io_is_busy.attr,
	&timer_slack.attr,
	&fast_ramp_down.attr,
	&fast_ramp_up.attr,
	&freq_reporting_policy.attr,
	&iowait_boost_step.attr,
#ifdef CONFIG_SCHED_TOP_TASK
	&top_task_hist_size.attr,
	&top_task_stats_policy.attr,
	&top_task_stats_empty_window.attr,
#endif
#ifdef CONFIG_ED_TASK
	&ed_task_running_duration.attr,
	&ed_task_waiting_duration.attr,
	&ed_new_task_running_duration.attr,
#endif
#ifdef CONFIG_MIGRATION_NOTIFY
	&freq_inc_notify.attr,
	&freq_dec_notify.attr,
#endif
#ifdef CONFIG_FREQ_IO_LIMIT
	&iowait_upper_limit.attr,
#endif
#endif
	NULL
};
ATTRIBUTE_GROUPS(sugov);

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
struct governor_user_attr {
	const char *name;
	uid_t uid;
	gid_t gid;
	mode_t mode;
};

#define SYSTEM_UID (uid_t)1000
#define SYSTEM_GID (uid_t)1000

#define INVALID_ATTR \
	{.name = NULL, .uid = (uid_t)(-1), .gid = (uid_t)(-1), .mode = 0000}

static struct governor_user_attr schedutil_user_attrs[] = {
	{.name = "target_loads", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
	{.name = "hispeed_freq", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
	{.name = "go_hispeed_load", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
	{.name = "above_hispeed_delay", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
	{.name = "min_sample_time", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
	{.name = "timer_slack", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
	{.name = "boostpulse", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0220},
	{.name = "boostpulse_duration", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
	{.name = "fast_ramp_down", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
	{.name = "fast_ramp_up", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
	{.name = "freq_reporting_policy", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
	{.name = "iowait_boost_step", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
	{.name = "overload_duration", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
#ifdef CONFIG_SCHED_TOP_TASK
	{.name = "top_task_hist_size", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
	{.name = "top_task_stats_policy", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
	{.name = "top_task_stats_empty_window", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
#endif
#ifdef CONFIG_ED_TASK
	{.name = "ed_task_running_duration", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
	{.name = "ed_task_waiting_duration", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
	{.name = "ed_new_task_running_duration", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
#endif
#ifdef CONFIG_MIGRATION_NOTIFY
	{.name = "freq_inc_notify", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
	{.name = "freq_dec_notify", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
#endif
#ifdef CONFIG_FREQ_IO_LIMIT
	{.name = "iowait_upper_limit", .uid = SYSTEM_UID, .gid = SYSTEM_GID, .mode = 0660},
#endif
	/* add below */
	INVALID_ATTR,
};

static void gov_sysfs_set_attr(unsigned int cpu, char *gov_name,
			       struct governor_user_attr *attrs)
{
	char *buf = NULL;
	int i = 0;
	int gov_dir_len, gov_attr_len;
	long ret;
	mm_segment_t fs = 0;

	if (!capable(CAP_SYS_ADMIN))
		return;

	buf = kzalloc(PATH_MAX, GFP_KERNEL);
	if (!buf)
		return;

	gov_dir_len = scnprintf(buf, PATH_MAX,
				"/sys/devices/system/cpu/cpu%u/cpufreq/%s/",
				cpu, gov_name);

	fs = get_fs();
	set_fs(KERNEL_DS);

	while (attrs[i].name) {
		gov_attr_len = scnprintf(buf + gov_dir_len,
					 PATH_MAX - gov_dir_len, "%s", attrs[i].name);

		if (gov_dir_len + gov_attr_len >= PATH_MAX) {
			i++;
			continue;
		}
		buf[gov_dir_len + gov_attr_len] = '\0';

		ret = ksys_chown(buf, attrs[i].uid, attrs[i].gid);
		if (ret)
			pr_err("chown fail:%s ret=%ld\n", buf, ret);

		ret = ksys_chmod(buf, attrs[i].mode);
		if (ret)
			pr_err("chmod fail:%s ret=%ld\n", buf, ret);
		i++;
	}

	set_fs(fs);
	kfree(buf);
}
#endif

static struct kobj_type sugov_tunables_ktype = {
	.default_groups = sugov_groups,
	.sysfs_ops = &governor_sysfs_ops,
};

/********************** cpufreq governor interface *********************/

struct cpufreq_governor schedutil_gov;

static struct sugov_policy *sugov_policy_alloc(struct cpufreq_policy *policy)
{
	struct sugov_policy *sg_policy;

	sg_policy = kzalloc(sizeof(*sg_policy), GFP_KERNEL);
	if (!sg_policy)
		return NULL;

	sg_policy->policy = policy;
	raw_spin_lock_init(&sg_policy->update_lock);
#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	raw_spin_lock_init(&sg_policy->timer_lock);
#endif
	return sg_policy;
}

static void sugov_policy_free(struct sugov_policy *sg_policy)
{
	kfree(sg_policy);
}

static int sugov_kthread_create(struct sugov_policy *sg_policy)
{
	struct task_struct *thread;
#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	struct sched_param param = { .sched_priority = MAX_USER_RT_PRIO / 2 };
#else
	struct sched_attr attr = {
		.size		= sizeof(struct sched_attr),
		.sched_policy	= SCHED_DEADLINE,
		.sched_flags	= SCHED_FLAG_SUGOV,
		.sched_nice	= 0,
		.sched_priority	= 0,
		/*
		 * Fake (unused) bandwidth; workaround to "fix"
		 * priority inheritance.
		 */
		.sched_runtime	=  1000000,
		.sched_deadline = 10000000,
		.sched_period	= 10000000,
	};
#endif
	struct cpufreq_policy *policy = sg_policy->policy;
	int ret;

	/* kthread only required for slow path */
	if (policy->fast_switch_enabled)
		return 0;

	kthread_init_work(&sg_policy->work, sugov_work);
	kthread_init_worker(&sg_policy->worker);
	thread = kthread_create(kthread_worker_fn, &sg_policy->worker,
				"sugov:%d",
				cpumask_first(policy->related_cpus));
	if (IS_ERR(thread)) {
		pr_err("failed to create sugov thread: %ld\n", PTR_ERR(thread));
		return PTR_ERR(thread);
	}

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	ret = sched_setscheduler_nocheck(thread, SCHED_FIFO, &param);
	if (ret) {
		kthread_stop(thread);
		pr_warn("%s: failed to set SCHED_FIFO\n", __func__);
		return ret;
	}
#else
	ret = sched_setattr_nocheck(thread, &attr);
	if (ret) {
		kthread_stop(thread);
		pr_warn("%s: failed to set SCHED_DEADLINE\n", __func__);
		return ret;
	}
#endif

	sg_policy->thread = thread;

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	if (!policy->dvfs_possible_from_any_cpu)
#endif
	kthread_bind_mask(thread, policy->related_cpus);
	init_irq_work(&sg_policy->irq_work, sugov_irq_work);
	mutex_init(&sg_policy->work_lock);

	wake_up_process(thread);

	return 0;
}

static void sugov_kthread_stop(struct sugov_policy *sg_policy)
{
	/* kthread only required for slow path */
	if (sg_policy->policy->fast_switch_enabled)
		return;

	kthread_flush_worker(&sg_policy->worker);
	kthread_stop(sg_policy->thread);
	mutex_destroy(&sg_policy->work_lock);
}

static struct sugov_tunables *sugov_tunables_alloc(struct sugov_policy *sg_policy)
{
	struct sugov_tunables *tunables;

	tunables = kzalloc(sizeof(*tunables), GFP_KERNEL);
	if (tunables) {
		gov_attr_set_init(&tunables->attr_set, &sg_policy->tunables_hook);
		if (!have_governor_per_policy())
			global_tunables = tunables;
	}
	return tunables;
}

static void sugov_tunables_free(struct sugov_tunables *tunables)
{
	if (!have_governor_per_policy())
		global_tunables = NULL;

	kfree(tunables);
}

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
static void sugov_nop_timer(struct timer_list *unused)
{
}

static void sugov_tunables_init(struct cpufreq_policy *policy,
				struct sugov_tunables *tunables)
{
	tunables->overload_duration = DEFAULT_OVERLOAD_DURATION_MS;
	tunables->above_hispeed_delay = default_above_hispeed_delay;
	tunables->nabove_hispeed_delay =
		ARRAY_SIZE(default_above_hispeed_delay);
	tunables->min_sample_time = default_min_sample_time;
	tunables->nmin_sample_time =
		ARRAY_SIZE(default_min_sample_time);
	tunables->go_hispeed_load = DEFAULT_GO_HISPEED_LOAD;
	tunables->target_loads = default_target_loads;
	tunables->ntarget_loads = ARRAY_SIZE(default_target_loads);
	tunables->boostpulse_duration = DEFAULT_BOOSTPULSE_DURATION;
	tunables->boostpulse_min_interval = DEFAULT_MIN_BOOSTPULSE_INTERVAL;
	tunables->timer_slack = DEFAULT_TIMER_SLACK;
	tunables->freq_reporting_policy = DEFAULT_FREQ_REPORTING_POLICY;
	tunables->iowait_boost_step = IOWAIT_BOOST_INC_STEP;

#ifdef CONFIG_ED_TASK
	tunables->ed_task_running_duration = EARLY_DETECTION_TASK_RUNNING_DURATION;
	tunables->ed_task_waiting_duration = EARLY_DETECTION_TASK_WAITING_DURATION;
	tunables->ed_new_task_running_duration = EARLY_DETECTION_NEW_TASK_RUNNING_DURATION;
#endif

#ifdef CONFIG_MIGRATION_NOTIFY
	tunables->freq_inc_notify = DEFAULT_FREQ_INC_NOTIFY;
	tunables->freq_dec_notify = DEFAULT_FREQ_DEC_NOTIFY;
#endif

#ifdef CONFIG_FREQ_IO_LIMIT
	tunables->iowait_upper_limit = policy->cpuinfo.max_freq;
#endif

	spin_lock_init(&tunables->target_loads_lock);
	spin_lock_init(&tunables->above_hispeed_delay_lock);
	spin_lock_init(&tunables->min_sample_time_lock);
}

static void sugov_tunables_save(struct cpufreq_policy *policy,
				struct sugov_tunables *tunables)
{
	int cpu;
	struct sugov_tunables *cached = per_cpu(cached_tunables, policy->cpu);

	if (!have_governor_per_policy())
		return;

	if (!cached) {
		cached = kzalloc(sizeof(*tunables), GFP_KERNEL);
		if (!cached)
			return;

		for_each_cpu(cpu, policy->related_cpus)
			per_cpu(cached_tunables, cpu) = cached;
	}

	cached->hispeed_freq = tunables->hispeed_freq;
	cached->go_hispeed_load = tunables->go_hispeed_load;
	cached->target_loads = tunables->target_loads;
	cached->ntarget_loads = tunables->ntarget_loads;
	cached->overload_duration = tunables->overload_duration;
	cached->above_hispeed_delay = tunables->above_hispeed_delay;
	cached->nabove_hispeed_delay = tunables->nabove_hispeed_delay;
	cached->min_sample_time = tunables->min_sample_time;
	cached->nmin_sample_time = tunables->nmin_sample_time;
	cached->boostpulse_duration = tunables->boostpulse_duration;
	cached->boostpulse_min_interval = tunables->boostpulse_min_interval;
	cached->timer_slack = tunables->timer_slack;
	cached->fast_ramp_up = tunables->fast_ramp_up;
	cached->fast_ramp_down = tunables->fast_ramp_down;
	cached->freq_reporting_policy = tunables->freq_reporting_policy;
	cached->iowait_boost_step = tunables->iowait_boost_step;

#ifdef CONFIG_SCHED_TOP_TASK
	cached->top_task_hist_size = tunables->top_task_hist_size;
	cached->top_task_stats_empty_window = tunables->top_task_stats_empty_window;
	cached->top_task_stats_policy = tunables->top_task_stats_policy;
#endif
#ifdef CONFIG_ED_TASK
	cached->ed_task_running_duration = tunables->ed_task_running_duration;
	cached->ed_task_waiting_duration = tunables->ed_task_waiting_duration;
	cached->ed_new_task_running_duration = tunables->ed_new_task_running_duration;
#endif
#ifdef CONFIG_MIGRATION_NOTIFY
	cached->freq_inc_notify = tunables->freq_inc_notify;
	cached->freq_dec_notify = tunables->freq_dec_notify;
#endif
#ifdef CONFIG_FREQ_IO_LIMIT
	cached->iowait_upper_limit = tunables->iowait_upper_limit;
#endif
}

static void sugov_tunables_restore(struct cpufreq_policy *policy)
{
	struct sugov_policy *sg_policy = policy->governor_data;
	struct sugov_tunables *tunables = sg_policy->tunables;
	struct sugov_tunables *cached = per_cpu(cached_tunables, policy->cpu);

	if (!cached)
		return;

	tunables->hispeed_freq = cached->hispeed_freq;
	tunables->go_hispeed_load = cached->go_hispeed_load;
	tunables->target_loads = cached->target_loads;
	tunables->ntarget_loads = cached->ntarget_loads;
	tunables->overload_duration = cached->overload_duration;
	tunables->above_hispeed_delay = cached->above_hispeed_delay;
	tunables->nabove_hispeed_delay = cached->nabove_hispeed_delay;
	tunables->min_sample_time = cached->min_sample_time;
	tunables->nmin_sample_time = cached->nmin_sample_time;
	tunables->boostpulse_duration = cached->boostpulse_duration;
	tunables->boostpulse_min_interval = cached->boostpulse_min_interval;
	tunables->timer_slack = cached->timer_slack;
	tunables->fast_ramp_up = cached->fast_ramp_up;
	tunables->fast_ramp_down = cached->fast_ramp_down;
	tunables->freq_reporting_policy = cached->freq_reporting_policy;
	tunables->iowait_boost_step = cached->iowait_boost_step;

#ifdef CONFIG_SCHED_TOP_TASK
	tunables->top_task_hist_size = cached->top_task_hist_size;
	tunables->top_task_stats_empty_window = cached->top_task_stats_empty_window;
	tunables->top_task_stats_policy = cached->top_task_stats_policy;
#endif
#ifdef CONFIG_ED_TASK
	tunables->ed_task_running_duration = cached->ed_task_running_duration;
	tunables->ed_task_waiting_duration = cached->ed_task_waiting_duration;
	tunables->ed_new_task_running_duration = cached->ed_new_task_running_duration;
#endif
#ifdef CONFIG_MIGRATION_NOTIFY
	tunables->freq_inc_notify = cached->freq_inc_notify;
	tunables->freq_dec_notify = cached->freq_dec_notify;
#endif
#ifdef CONFIG_FREQ_IO_LIMIT
	tunables->iowait_upper_limit = cached->iowait_upper_limit;
#endif
}
#endif

static int sugov_init(struct cpufreq_policy *policy)
{
	struct sugov_policy *sg_policy;
	struct sugov_tunables *tunables;
	int ret = 0;

	/* State should be equivalent to EXIT */
	if (policy->governor_data)
		return -EBUSY;

	cpufreq_enable_fast_switch(policy);

	sg_policy = sugov_policy_alloc(policy);
	if (!sg_policy) {
		ret = -ENOMEM;
		goto disable_fast_switch;
	}

	ret = sugov_kthread_create(sg_policy);
	if (ret)
		goto free_sg_policy;

	mutex_lock(&global_tunables_lock);

	if (global_tunables) {
		if (WARN_ON(have_governor_per_policy())) {
			ret = -EINVAL;
			goto stop_kthread;
		}
		policy->governor_data = sg_policy;
		sg_policy->tunables = global_tunables;

		gov_attr_set_get(&global_tunables->attr_set, &sg_policy->tunables_hook);
		goto out;
	}

	tunables = sugov_tunables_alloc(sg_policy);
	if (!tunables) {
		ret = -ENOMEM;
		goto stop_kthread;
	}

	tunables->rate_limit_us = cpufreq_policy_transition_delay_us(policy);

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	sugov_tunables_init(policy, tunables);
#endif

	policy->governor_data = sg_policy;
	sg_policy->tunables = tunables;

	ret = kobject_init_and_add(&tunables->attr_set.kobj, &sugov_tunables_ktype,
				   get_governor_parent_kobj(policy), "%s",
				   schedutil_gov.name);
	if (ret)
		goto fail;

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	sugov_tunables_restore(policy);
	gov_sysfs_set_attr(policy->cpu, schedutil_gov.name, schedutil_user_attrs);
#endif
out:
	mutex_unlock(&global_tunables_lock);
	return 0;

fail:
	kobject_put(&tunables->attr_set.kobj);
	policy->governor_data = NULL;
	sugov_tunables_free(tunables);

stop_kthread:
	sugov_kthread_stop(sg_policy);
	mutex_unlock(&global_tunables_lock);

free_sg_policy:
	sugov_policy_free(sg_policy);

disable_fast_switch:
	cpufreq_disable_fast_switch(policy);

	pr_err("initialization failed (error %d)\n", ret);
	return ret;
}

static void sugov_exit(struct cpufreq_policy *policy)
{
	struct sugov_policy *sg_policy = policy->governor_data;
	struct sugov_tunables *tunables = sg_policy->tunables;
	unsigned int count;

	mutex_lock(&global_tunables_lock);

	count = gov_attr_set_put(&tunables->attr_set, &sg_policy->tunables_hook);
	policy->governor_data = NULL;
	if (!count) {
#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
		sugov_tunables_save(policy, tunables);
#endif
		sugov_tunables_free(tunables);
	}

	mutex_unlock(&global_tunables_lock);

	sugov_kthread_stop(sg_policy);
	sugov_policy_free(sg_policy);
	cpufreq_disable_fast_switch(policy);
}

static int sugov_start(struct cpufreq_policy *policy)
{
	struct sugov_policy *sg_policy = policy->governor_data;
	unsigned int cpu;

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	sg_policy->overload_duration_ns =
		sg_policy->tunables->overload_duration * NSEC_PER_MSEC;
	sg_policy->floor_validate_time = 0;
	sg_policy->hispeed_validate_time = 0;
	/* allow freq drop as soon as possible when policy->min restored */
	sg_policy->min_freq = policy->min;
	atomic_set(&sg_policy->skip_min_sample_time, 0);
	atomic_set(&sg_policy->skip_hispeed_logic, 0);
	sg_policy->util_changed = false;
	sg_policy->update_time = 0;
	sg_policy->last_iowait = 0;
	sg_policy->trigger_cpu = policy->cpu;
#endif
	sg_policy->freq_update_delay_ns	= sg_policy->tunables->rate_limit_us * NSEC_PER_USEC;
	sg_policy->last_freq_update_time	= 0;
	sg_policy->next_freq			= 0;
	sg_policy->work_in_progress		= false;
	sg_policy->limits_changed		= false;
	sg_policy->cached_raw_freq		= 0;

	sg_policy->need_freq_update = cpufreq_driver_test_flags(CPUFREQ_NEED_UPDATE_LIMITS);

	for_each_cpu(cpu, policy->cpus) {
		struct sugov_cpu *sg_cpu = &per_cpu(sugov_cpu, cpu);

		memset(sg_cpu, 0, sizeof(*sg_cpu));
		sg_cpu->cpu			= cpu;
		sg_cpu->sg_policy		= sg_policy;
#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
#ifdef CONFIG_FREQ_IO_LIMIT
		sg_cpu->iowait_boost_max =
			freq_to_util(cpu, min(policy->cpuinfo.max_freq,
					      sg_policy->tunables->iowait_upper_limit));
#else
		sg_cpu->iowait_boost_max = freq_to_util(cpu, policy->cpuinfo.max_freq);
#endif
		sg_cpu->iowait_boost_min = freq_to_util(cpu, policy->cpuinfo.min_freq);
		sg_cpu->iowait_boost_step = freq_to_util(cpu, sg_policy->tunables->iowait_boost_step);
#endif
	}

	for_each_cpu(cpu, policy->cpus) {
		struct sugov_cpu *sg_cpu = &per_cpu(sugov_cpu, cpu);

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
		cpufreq_add_update_util_hook(cpu, &sg_cpu->update_util,
					     sugov_update_shared);
		sg_cpu->enabled = true;
#else
		cpufreq_add_update_util_hook(cpu, &sg_cpu->update_util,
					     policy_is_shared(policy) ?
							sugov_update_shared :
							sugov_update_single);
#endif
	}

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	raw_spin_lock(&sg_policy->timer_lock);
	timer_setup(&sg_policy->pol_slack_timer, sugov_nop_timer, 0);
	add_timer_on(&sg_policy->pol_slack_timer, policy->cpu);
	sg_policy->governor_enabled = true;
	raw_spin_unlock(&sg_policy->timer_lock);

	blocking_notifier_call_chain(&sugov_status_notifier_list,
			SUGOV_START, &policy->cpu);
#endif

	return 0;
}

static void sugov_stop(struct cpufreq_policy *policy)
{
	struct sugov_policy *sg_policy = policy->governor_data;
	unsigned int cpu;

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	blocking_notifier_call_chain(&sugov_status_notifier_list,
			SUGOV_STOP, &policy->cpu);

	raw_spin_lock(&sg_policy->timer_lock);
	sg_policy->governor_enabled = false;
	del_timer_sync(&sg_policy->pol_slack_timer);
	raw_spin_unlock(&sg_policy->timer_lock);
#endif

	for_each_cpu(cpu, policy->cpus) {
#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
		struct sugov_cpu *sg_cpu = &per_cpu(sugov_cpu, cpu);
		sg_cpu->enabled = false;
#endif
		cpufreq_remove_update_util_hook(cpu);
	}

#ifndef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	synchronize_rcu();
#endif

	if (!policy->fast_switch_enabled) {
		irq_work_sync(&sg_policy->irq_work);
		kthread_cancel_work_sync(&sg_policy->work);
	}
}

static void sugov_limits(struct cpufreq_policy *policy)
{
	struct sugov_policy *sg_policy = policy->governor_data;

	if (!policy->fast_switch_enabled) {
		mutex_lock(&sg_policy->work_lock);
		cpufreq_policy_apply_limits(policy);
#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
		sg_policy->next_freq = policy->cur;
#endif
		mutex_unlock(&sg_policy->work_lock);
	}

#ifdef CONFIG_CPU_FREQ_GOV_SCHEDUTIL_OPT
	/* policy->min restored */
	if (sg_policy->min_freq > policy->min) {
		sugov_mark_util_change(policy->cpu, POLICY_MIN_RESTORE);
		sugov_check_freq_update(policy->cpu);
	}

	sg_policy->min_freq = policy->min;
#endif
	sg_policy->limits_changed = true;
}

struct cpufreq_governor schedutil_gov = {
	.name			= "schedutil",
	.owner			= THIS_MODULE,
	.flags			= CPUFREQ_GOV_DYNAMIC_SWITCHING,
	.init			= sugov_init,
	.exit			= sugov_exit,
	.start			= sugov_start,
	.stop			= sugov_stop,
	.limits			= sugov_limits,
};

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_SCHEDUTIL
struct cpufreq_governor *cpufreq_default_governor(void)
{
	return &schedutil_gov;
}
#endif

cpufreq_governor_init(schedutil_gov);
