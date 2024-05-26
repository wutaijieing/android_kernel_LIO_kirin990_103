/*
 * hisi_namtso_pmu.c
 *
 * Namtso Performance Monitor Unit driver
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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

#define PMUNAME		"hisi_namtso"
#define DRVNAME		PMUNAME "_pmu"
#undef pr_fmt
#define pr_fmt(fmt)     "namtso_pmu: " fmt

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/version.h>
#include <linux/bug.h>
#include <linux/cpuhotplug.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <linux/smp.h>
#include <linux/spinlock.h>
#include <linux/cpu_pm.h>
#include <securec.h>
#include <namtso_pmu_plat.h>
#include "namtso_pmu.h"

/* All event counters are 32bit, with a 64bit Cycle counter */
#define namtso_pmu_counter_width(idx)	\
	(((idx) == NAMTSO_PMU_IDX_CYCLE_COUNTER) ? 64 : 32)

#define namtso_pmu_counter_mask(idx)	\
	GENMASK_ULL((namtso_pmu_counter_width((idx)) - 1), 0)

static unsigned long namtso_pmu_cpuhp_state;

/*
 * Select the counter register offset using the counter index
 */
static inline u32 get_counter_offset(int cntr_idx)
{
	return (u32)(NAMTSO_PMEVCNTR0_OFFSET + (cntr_idx * PMEVCNTR_STEP));
}

static inline u64 __namtso_pmu_read_counter(struct namtso_pmu *namtso_pmu,
					    int idx)
{
	u64 val;

	val = readl(namtso_pmu->base + get_counter_offset(idx));
	return val;
}

static u64 __namtso_pmu_read_pmccntr(struct namtso_pmu *namtso_pmu)
{
	u64 val_l;
	u64 val_h;
	u64 val64;

	val_l = readl(namtso_pmu->base + NAMTSO_PMCCNTR_L_OFFSET);
	val_h = readl(namtso_pmu->base + NAMTSO_PMCCNTR_H_OFFSET);
	val64 = (val_h << 32) | val_l;

	return val64;
}

static u64 namtso_pmu_read_counter(struct perf_event *event)
{
	struct namtso_pmu *namtso_pmu = to_namtso_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	int idx = hwc->idx;
	unsigned long flags;
	u64 val;

	if (WARN_ON(!cpumask_test_cpu(smp_processor_id(),
				      &namtso_pmu->associated_cpus)))
		return 0;

	if (!namtso_pmu_counter_valid(namtso_pmu, idx)) {
		dev_err(namtso_pmu->dev, "Unsupported event index:%d\n", idx);
		return 0;
	}

	raw_spin_lock_irqsave(&namtso_pmu->pmu_lock, flags);
	if (idx == NAMTSO_PMU_IDX_CYCLE_COUNTER)
		val = __namtso_pmu_read_pmccntr(namtso_pmu);
	else
		val = __namtso_pmu_read_counter(namtso_pmu, idx);
	raw_spin_unlock_irqrestore(&namtso_pmu->pmu_lock, flags);

	return val;
}

static inline void __namtso_pmu_write_counter(struct namtso_pmu *namtso_pmu,
					      int idx, u64 val)
{
	writel(val, namtso_pmu->base + get_counter_offset(idx));
}

#define U32_MAX_MASK			    ((1UL << 32) - 1)
static void __namtso_pmu_write_pmccntr(struct namtso_pmu *namtso_pmu,
				       u64 val)
{
	u32 val_l = val & U32_MAX_MASK;
	u32 val_h = (val >> 32) & U32_MAX_MASK;

	writel(val_l, namtso_pmu->base + NAMTSO_PMCCNTR_L_OFFSET);
	writel(val_h, namtso_pmu->base + NAMTSO_PMCCNTR_H_OFFSET);
}

static void namtso_pmu_write_counter(struct perf_event *event, u64 val)
{
	struct namtso_pmu *namtso_pmu = to_namtso_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	int idx = hwc->idx;
	unsigned long flags;

	if (WARN_ON(!cpumask_test_cpu(smp_processor_id(),
				      &namtso_pmu->associated_cpus)))
		return;

	if (!namtso_pmu_counter_valid(namtso_pmu, idx)) {
		dev_err(namtso_pmu->dev, "Unsupported event index:%d\n", idx);
		return;
	}

	raw_spin_lock_irqsave(&namtso_pmu->pmu_lock, flags);
	if (idx == NAMTSO_PMU_IDX_CYCLE_COUNTER)
		__namtso_pmu_write_pmccntr(namtso_pmu, val);
	else
		__namtso_pmu_write_counter(namtso_pmu, idx, val);
	raw_spin_unlock_irqrestore(&namtso_pmu->pmu_lock, flags);
}

static void namtso_pmu_enable_counter(struct namtso_pmu *namtso_pmu,
				      struct hw_perf_event *hwc)
{
	/* Enable counter index in NAMTSO_ENSET register */
	writel(BIT(hwc->idx), namtso_pmu->base + NAMTSO_PMCNTENSET_OFFSET);
}

static void namtso_pmu_disable_counter(struct namtso_pmu *namtso_pmu,
				       struct hw_perf_event *hwc)
{
	/* Clear counter index in NAMTSO_ENCLR register */
	writel(BIT(hwc->idx), namtso_pmu->base + NAMTSO_PMCNTENCLR_OFFSET);
}

#ifdef CONFIG_NAMTSO_PMU_DEBUG
static struct attribute *namtso_pmu_format_attrs[] = {
	NAMTSO_FORMAT_ATTR(event, "config:0-31"),
	NULL,
};

static const struct attribute_group namtso_pmu_format_group = {
	.name = "format",
	.attrs = namtso_pmu_format_attrs,
};

static umode_t namtso_pmu_event_attr_is_visible(struct kobject *kobj __maybe_unused,
						struct attribute *attr,
						int unused __maybe_unused)
{
	return attr->mode;
}

static const struct attribute_group namtso_pmu_events_group = {
	.name = "events",
	.attrs = namtso_pmu_events_attrs,
	.is_visible = namtso_pmu_event_attr_is_visible,
};

static struct attribute *namtso_pmu_cpumask_attrs[] = {
	NAMTSO_CPUMASK_ATTR(cpumask, NAMTSO_ACTIVE_CPU_MASK),
	NAMTSO_CPUMASK_ATTR(associated_cpus, NAMTSO_ASSOCIATED_CPU_MASK),
	NULL,
};

static const struct attribute_group namtso_pmu_cpumask_attr_group = {
	.attrs = namtso_pmu_cpumask_attrs,
};

static const struct attribute_group *namtso_pmu_attr_groups[] = {
	&namtso_pmu_cpumask_attr_group,
	&namtso_pmu_events_group,
	&namtso_pmu_format_group,
	NULL,
};

/*
 * PMU event attributes
 */
ssize_t namtso_pmu_sysfs_event_show(struct device *dev __maybe_unused,
				    struct device_attribute *attr,
				    char *page)
{
	struct dev_ext_attribute *eattr = NULL;

	eattr = container_of(attr, struct dev_ext_attribute, attr);

	return snprintf_s(page, PAGE_SIZE, PAGE_SIZE - 1,
			  "config=0x%lx\n", (uintptr_t)eattr->var);
}

/*
 * PMU format attributes
 */
ssize_t namtso_pmu_sysfs_format_show(struct device *dev __maybe_unused,
				     struct device_attribute *attr,
				     char *buf)
{
	struct dev_ext_attribute *eattr = NULL;

	eattr = container_of(attr, struct dev_ext_attribute, attr);

	return snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1,
			  "%s\n", (char *)eattr->var);
}

ssize_t namtso_pmu_cpumask_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct namtso_pmu *namtso_pmu = to_namtso_pmu(dev_get_drvdata(dev));
	struct dev_ext_attribute *eattr = container_of(attr,
						       struct dev_ext_attribute,
						       attr);
	unsigned long mask_id = (uintptr_t)eattr->var;
	const cpumask_t *cpumask = NULL;

	switch (mask_id) {
	case NAMTSO_ACTIVE_CPU_MASK:
		cpumask = &namtso_pmu->active_cpu;
		break;
	case NAMTSO_ASSOCIATED_CPU_MASK:
		cpumask = &namtso_pmu->associated_cpus;
		break;
	default:
		return 0;
	}
	return cpumap_print_to_pagebuf(true, buf, cpumask);
}
#endif

bool namtso_pmu_counter_valid(struct namtso_pmu *namtso_pmu, int idx)
{
	return (idx >= 0 && idx < namtso_pmu->num_counters) ||
		(idx == NAMTSO_PMU_IDX_CYCLE_COUNTER);
}

static int namtso_pmu_get_event_idx(struct namtso_hw_events *hw_events, struct perf_event *event)
{
	struct namtso_pmu *namtso_pmu = to_namtso_pmu(event->pmu);
	unsigned long *used_mask = hw_events->used_mask;
	int num_counters = namtso_pmu->num_counters;
	unsigned long evtype = event->attr.config;
	int idx;

	if (evtype == NAMTSO_PMU_EVT_CYCLES) {
		if (test_and_set_bit(NAMTSO_PMU_IDX_CYCLE_COUNTER, used_mask))
			return -EAGAIN;
		return NAMTSO_PMU_IDX_CYCLE_COUNTER;
	}

	idx = find_first_zero_bit(used_mask, num_counters);
	if (idx >= num_counters) {
		dev_err(namtso_pmu->dev,
			"no idle counter to use idx = %d\n", idx);
		return -EAGAIN;
	}
	set_bit(idx, used_mask);
	return idx;
}

static void namtso_pmu_clear_event_idx(struct namtso_pmu *namtso_pmu, int idx)
{
	if (!namtso_pmu_counter_valid(namtso_pmu, idx)) {
		dev_err(namtso_pmu->dev, "Unsupported event index:%d\n", idx);
		return;
	}

	clear_bit(idx, namtso_pmu->hw_events.used_mask);
}

static bool namtso_pmu_validate_event(struct pmu *pmu,
				      struct namtso_hw_events *hw_events,
				      struct perf_event *event)
{
	if (is_software_event(event))
		return true;

	if (event->pmu != pmu)
		return false;

	return namtso_pmu_get_event_idx(hw_events, event) >= 0;
}

/*
 * Make sure the group of events can be scheduled at once
 * on the PMU
 */
static bool namtso_pmu_validate_group(struct perf_event *event)
{
	struct perf_event *sibling = NULL;
	struct perf_event *leader = event->group_leader;
	struct namtso_hw_events fake_hw;
	int ret;

	if (event->group_leader == event)
		return true;

	ret = memset_s(fake_hw.used_mask,
		       sizeof(fake_hw.used_mask),
		       0,
		       sizeof(fake_hw.used_mask));
	if (ret != EOK) {
		pr_err("namtso_pmu: memset_s failed\n");
		return false;
	}

	if (!namtso_pmu_validate_event(event->pmu, &fake_hw, leader))
		return false;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
	for_each_sibling_event(sibling, leader) {
#else
	list_for_each_entry(sibling, &leader->sibling_list, group_entry) {
#endif
		if (!namtso_pmu_validate_event(event->pmu, &fake_hw, sibling))
			return false;
	}

	return namtso_pmu_validate_event(event->pmu, &fake_hw, event);
}

static int namtso_pmu_event_init(struct perf_event *event)
{
	struct namtso_pmu *namtso_pmu = NULL;
	struct hw_perf_event *hwc = NULL;

	if (event == NULL)
		return -EINVAL;

	namtso_pmu = to_namtso_pmu(event->pmu);
	hwc = &event->hw;

	if (event->attr.type != (u32)(event->pmu->type))
		return -ENOENT;

	/* We don't support sampling */
	if (is_sampling_event(event)) {
		dev_err(namtso_pmu->pmu.dev, "Can't support sampling events\n");
		return -EOPNOTSUPP;
	}

	/* We cannot support task bound events */
	if (event->cpu < 0 || (event->attach_state & PERF_ATTACH_TASK) != 0) {
		dev_err(namtso_pmu->pmu.dev, "Can't support per-task counters\n");
		return -EINVAL;
	}

	/* counters do not have these bits */
	if (has_branch_stack(event)	||
	    event->attr.exclude_user	||
	    event->attr.exclude_kernel	||
	    event->attr.exclude_hv	||
	    event->attr.exclude_idle	||
	    event->attr.exclude_host	||
	    event->attr.exclude_guest) {
		dev_err(namtso_pmu->pmu.dev, "Can't support filtering\n");
		return -EINVAL;
	}

	if (cpumask_test_cpu(event->cpu, &namtso_pmu->associated_cpus) == 0) {
		dev_err(namtso_pmu->pmu.dev,
			"Requested cpu is not associated with the namtso\n");
		return -EINVAL;
	}

	if (event->attr.config > namtso_pmu->check_event) {
		dev_err(namtso_pmu->pmu.dev, "invalid attr.config=0x%lx\n",
			event->attr.config);
		return -EINVAL;
	}

	/*
	 * Choose the current active CPU to read the events. We don't want
	 * to migrate the event contexts, irq handling etc to the requested
	 * CPU. As long as the requested CPU is within the same namtso, we
	 * are fine.
	 */
	event->cpu = cpumask_first(&namtso_pmu->active_cpu);
	if ((unsigned int)event->cpu >= nr_cpu_ids) {
		dev_err(namtso_pmu->pmu.dev, "invalid event->cpu=%d\n",
			event->cpu);
		return -EINVAL;
	}

	if (!namtso_pmu_validate_group(event))
		return -EINVAL;

	hwc->config_base = event->attr.config;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,19,0))
	event->readable_on_cpus = CPU_MASK_ALL;
#endif

	return 0;
}

static void namtso_pmu_counter_interrupt_enable(struct namtso_pmu *namtso_pmu,
						struct hw_perf_event *hwc)
{
	writel(BIT(hwc->idx), namtso_pmu->base + NAMTSO_PMINTENSET_OFFSET);
}

static void namtso_pmu_counter_interrupt_disable(struct namtso_pmu *namtso_pmu,
						 struct hw_perf_event *hwc)
{
	writel(BIT(hwc->idx), namtso_pmu->base + NAMTSO_PMINTENCLR_OFFSET);
}

/*
 * Set the counter to count the event that we're interested in,
 * and enable interrupt and counter.
 */
static void namtso_pmu_enable_event(struct perf_event *event)
{
	struct namtso_pmu *namtso_pmu = to_namtso_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;

	namtso_pmu_counter_interrupt_enable(namtso_pmu, hwc);
	namtso_pmu_enable_counter(namtso_pmu, hwc);
}

/*
 * Disable counter and interrupt.
 */
static void namtso_pmu_disable_event(struct perf_event *event)
{
	struct namtso_pmu *namtso_pmu = to_namtso_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;

	namtso_pmu_disable_counter(namtso_pmu, hwc);
	namtso_pmu_counter_interrupt_disable(namtso_pmu, hwc);
}

/*
 * namtso_pmu_set_event_period: Set the period for the counter.
 *
 * All NAMTSO PMU event counters, except the cycle counter are 32bit
 * counters. To handle cases of extreme interrupt latency, we program
 * the counter with half of the max count for the counters.
 */
static void namtso_pmu_set_event_period(struct perf_event *event)
{
	int idx = event->hw.idx;
	u64 val = ((unsigned long long)namtso_pmu_counter_mask(idx)) >> 1;

	local64_set(&event->hw.prev_count, val);
	/* Write start value to the hardware event counter */
	namtso_pmu_write_counter(event, val);
}

static void namtso_pmu_event_update(struct perf_event *event)
{
	struct hw_perf_event *hwc = &event->hw;
	u64 delta, prev_raw_count, new_raw_count;

	do {
		/* Read the count from the counter register */
		new_raw_count = namtso_pmu_read_counter(event);
		prev_raw_count = local64_read(&hwc->prev_count);
	} while ((u64)local64_cmpxchg(&hwc->prev_count, prev_raw_count,
				 new_raw_count) != prev_raw_count);
	/*
	 * compute the delta
	 */
	delta = (new_raw_count - prev_raw_count) &
		namtso_pmu_counter_mask(hwc->idx);

	local64_add(delta, &event->count);
}

static u32 namtso_pmu_get_reset_overflow(struct namtso_pmu *namtso_pmu)
{
	u32 val;

	val = readl(namtso_pmu->base + NAMTSO_PMOVSCLR_OFFSET);
	/* Clear the bit */
	writel(val, namtso_pmu->base + NAMTSO_PMOVSCLR_OFFSET);

	return val;
}

static irqreturn_t namtso_pmu_handle_irq(int irq_num __maybe_unused, void *dev)
{
	int i;
	bool handled = false;
	struct namtso_pmu *namtso_pmu = dev;
	struct namtso_hw_events *hw_events = &namtso_pmu->hw_events;
	unsigned long overflow;

	overflow = namtso_pmu_get_reset_overflow(namtso_pmu);
	if (overflow == 0)
		return IRQ_NONE;

	for_each_set_bit(i, &overflow, NAMTSO_PMU_MAX_HW_CNTRS) {
		struct perf_event *event = hw_events->events[i];

		if (event == NULL)
			continue;

		namtso_pmu_event_update(event);
		namtso_pmu_set_event_period(event);
		handled = true;
	}

	return IRQ_RETVAL(handled);
}

static void namtso_pmu_set_event(struct namtso_pmu *namtso_pmu,
				 struct perf_event *event)
{
	int idx = event->hw.idx;

	if (!namtso_pmu_counter_valid(namtso_pmu, idx)) {
		dev_err(namtso_pmu->dev, "Unsupported event index:%d\n", idx);
		return;
	}

	writel(pmu_get_eventid(event),
	       namtso_pmu->base + NAMTSO_PMEVTYPER0_OFFSET + idx * PMEVTYPER_STEP);
}

static void namtso_pmu_start(struct perf_event *event, int pmu_flags)
{
	struct namtso_pmu *namtso_pmu = to_namtso_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	unsigned long flags;

	 /* We always reprogram the counter */
	if ((unsigned int)pmu_flags & PERF_EF_RELOAD)
		WARN_ON(!(event->hw.state & PERF_HES_UPTODATE));

	namtso_pmu_set_event_period(event);
	hwc->state = 0;

	raw_spin_lock_irqsave(&namtso_pmu->pmu_lock, flags);
	if (hwc->idx != NAMTSO_PMU_IDX_CYCLE_COUNTER)
		namtso_pmu_set_event(namtso_pmu, event);
	namtso_pmu_enable_event(event);
	raw_spin_unlock_irqrestore(&namtso_pmu->pmu_lock, flags);
}

static void namtso_pmu_stop(struct perf_event *event, int pmu_flags __maybe_unused)
{
	struct namtso_pmu *namtso_pmu = to_namtso_pmu(event->pmu);
	unsigned long flags;

	if (event->hw.state & PERF_HES_STOPPED)
		return;

	raw_spin_lock_irqsave(&namtso_pmu->pmu_lock, flags);
	namtso_pmu_disable_event(event);
	raw_spin_unlock_irqrestore(&namtso_pmu->pmu_lock, flags);

	namtso_pmu_event_update(event);
	event->hw.state |= PERF_HES_STOPPED | PERF_HES_UPTODATE;
}

static int namtso_pmu_add(struct perf_event *event, int pmu_flags)
{
	struct namtso_pmu *namtso_pmu = to_namtso_pmu(event->pmu);
	struct namtso_hw_events *hw_events = &namtso_pmu->hw_events;
	struct hw_perf_event *hwc = &event->hw;
	int idx;

	if (WARN_ON_ONCE(!cpumask_test_cpu(smp_processor_id(),
					   &namtso_pmu->associated_cpus)))
		return -ENOENT;

	/* Get an available counter index for counting */
	idx = namtso_pmu_get_event_idx(hw_events, event);
	if (idx < 0)
		return idx;

	hwc->idx = idx;
	hw_events->events[idx] = event;
	hwc->state = PERF_HES_STOPPED | PERF_HES_UPTODATE;

	if ((unsigned int)pmu_flags & PERF_EF_START)
		namtso_pmu_start(event, PERF_EF_RELOAD);

	perf_event_update_userpage(event);
	return 0;
}

static void namtso_pmu_del(struct perf_event *event, int pmu_flags __maybe_unused)
{
	struct namtso_pmu *namtso_pmu = to_namtso_pmu(event->pmu);
	struct hw_perf_event *hwc = &event->hw;
	int idx = hwc->idx;

	namtso_pmu_stop(event, PERF_EF_UPDATE);
	namtso_pmu->hw_events.events[idx] = NULL;
	namtso_pmu_clear_event_idx(namtso_pmu, idx);
	perf_event_update_userpage(event);
}

static void namtso_pmu_read(struct perf_event *event)
{
	/* Read hardware counter and update the perf counter statistics */
	namtso_pmu_event_update(event);
}

static void namtso_pmu_enable(struct pmu *pmu)
{
	struct namtso_pmu *namtso_pmu = to_namtso_pmu(pmu);
	unsigned long flags;
	int enabled;
	u32 pmcr;

	enabled = bitmap_empty(namtso_pmu->hw_events.used_mask,
			       NAMTSO_PMU_MAX_HW_CNTRS);
	if (enabled != 0)
		return;

	/*
	 * Set enable bit in NAMTSO_PMCR register to start counting
	 * for all enabled counters.
	 */
	raw_spin_lock_irqsave(&namtso_pmu->pmu_lock, flags);
	pmcr = readl(namtso_pmu->base + NAMTSO_PMCR_OFFSET);
	pmcr |= NAMTSO_PMCR_EN;
	writel(pmcr, namtso_pmu->base + NAMTSO_PMCR_OFFSET);
	raw_spin_unlock_irqrestore(&namtso_pmu->pmu_lock, flags);
}

static void namtso_pmu_disable(struct pmu *pmu)
{
	struct namtso_pmu *namtso_pmu = to_namtso_pmu(pmu);
	unsigned long flags;
	u32 pmcr;

	/*
	 * Clear enable bit in NAMTSO_PMCR register to stop counting
	 * for all enabled counters.
	 */
	raw_spin_lock_irqsave(&namtso_pmu->pmu_lock, flags);
	pmcr = readl(namtso_pmu->base + NAMTSO_PMCR_OFFSET);
	pmcr &= ~(NAMTSO_PMCR_EN);
	writel(pmcr, namtso_pmu->base + NAMTSO_PMCR_OFFSET);
	raw_spin_unlock_irqrestore(&namtso_pmu->pmu_lock, flags);
}

struct cpu_pm_namtso_pmu_args {
	struct namtso_pmu *namtso_pmu;
	unsigned long cmd;
	int cpu;
	int ret;
};

#ifdef CONFIG_CPU_PM
static void cpu_pm_namtso_pmu_setup(struct namtso_pmu *namtso_pmu,
				    unsigned long cmd)
{
	struct namtso_hw_events *hw_events = &namtso_pmu->hw_events;
	struct perf_event *event = NULL;
	int idx;

	for (idx = 0; idx < NAMTSO_PMU_MAX_HW_CNTRS; idx++) {
		/*
		 * If the counter is not used skip it, there is no
		 * need of stopping/restarting it.
		 */
		if (!test_bit(idx, hw_events->used_mask))
			continue;

		event = hw_events->events[idx];
		if (event == NULL)
			continue;

		/*
		 * Check if an attempt was made to free this event during
		 * the CPU went offline.
		 */
		if (event->state != PERF_EVENT_STATE_ACTIVE)
			continue;

		switch (cmd) {
		case CPU_PM_ENTER:
			/*
			 * Stop and update the counter
			 */
			namtso_pmu_stop(event, PERF_EF_UPDATE);
			break;
		case CPU_PM_EXIT:
		case CPU_PM_ENTER_FAILED:
			 /*
			  * Restore and enable the counter.
			  * namtso_pmu_start() indirectly calls
			  *
			  * perf_event_update_userpage()
			  *
			  * that requires RCU read locking to be functional,
			  * wrap the call within RCU_NONIDLE to make the
			  * RCU subsystem aware this cpu is not idle from
			  * an RCU perspective for the namtso_pmu_start() call
			  * duration.
			  */
			RCU_NONIDLE(namtso_pmu_start(event, PERF_EF_RELOAD));
			break;
		default:
			break;
		}
	}
}

static void cpu_pm_namtso_pmu_common(void *info)
{
	struct cpu_pm_namtso_pmu_args *data = info;
	struct namtso_pmu *namtso_pmu = data->namtso_pmu;
	unsigned long cmd = data->cmd;
	int cpu = data->cpu;
	struct namtso_hw_events *hw_events = &namtso_pmu->hw_events;
	int enabled = false;
	bool fcm_pwrdn = 0;

	if (cpumask_test_cpu(cpu, &namtso_pmu->associated_cpus) == 0) {
		data->ret = NOTIFY_DONE;
		return;
	}

	enabled = test_bit(NAMTSO_PMU_IDX_CYCLE_COUNTER, hw_events->used_mask) > 0 ||
			bitmap_weight(hw_events->used_mask, namtso_pmu->num_counters);
	if (enabled == 0) {
		data->ret = NOTIFY_OK;
		return;
	}

	data->ret = NOTIFY_OK;

	switch (cmd) {
	case CPU_PM_ENTER:
		spin_lock(&namtso_pmu->fcm_idle_lock);
		fcm_pwrdn = lpcpu_fcm_cluster_pwrdn();
		if (fcm_pwrdn && !namtso_pmu->fcm_idle) {
			namtso_pmu->fcm_idle = true;
			namtso_pmu_disable(&namtso_pmu->pmu);
			cpu_pm_namtso_pmu_setup(namtso_pmu, cmd);
		}
		spin_unlock(&namtso_pmu->fcm_idle_lock);
		break;
	case CPU_PM_EXIT:
	case CPU_PM_ENTER_FAILED:
		spin_lock(&namtso_pmu->fcm_idle_lock);
		if (namtso_pmu->fcm_idle) {
			namtso_pmu->fcm_idle = false;
			cpu_pm_namtso_pmu_setup(namtso_pmu, cmd);
			namtso_pmu_enable(&namtso_pmu->pmu);
		}
		spin_unlock(&namtso_pmu->fcm_idle_lock);
		break;
	default:
		data->ret = NOTIFY_DONE;
		break;
	}
}

static int cpu_pm_namtso_pmu_notify(struct notifier_block *b,
				    unsigned long cmd, void *v __maybe_unused)
{
	struct cpu_pm_namtso_pmu_args data = {
		.namtso_pmu = container_of(b, struct namtso_pmu, cpu_pm_nb),
		.cmd = cmd,
		.cpu = smp_processor_id(),
	};

	cpu_pm_namtso_pmu_common(&data);
	return data.ret;
}

static int cpu_pm_namtso_pmu_register(struct namtso_pmu *namtso_pmu)
{
	namtso_pmu->cpu_pm_nb.notifier_call = cpu_pm_namtso_pmu_notify;
	return cpu_pm_register_notifier(&namtso_pmu->cpu_pm_nb);
}

static void cpu_pm_namtso_pmu_unregister(struct namtso_pmu *namtso_pmu)
{
	cpu_pm_unregister_notifier(&namtso_pmu->cpu_pm_nb);
}

#else
static inline int cpu_pm_namtso_pmu_register(struct namtso_pmu *namtso_pmu) { return 0; }
static inline void cpu_pm_namtso_pmu_unregister(struct namtso_pmu *namtso_pmu) { }
static void cpu_pm_namtso_pmu_common(void *info) { }
#endif

static int namtso_pmu_dev_init(struct platform_device *pdev,
			       struct namtso_pmu *namtso_pmu)
{
	struct device *dev = &pdev->dev;

	namtso_pmu->base = of_iomap(dev->of_node, 0);
	if (namtso_pmu->base == NULL) {
		dev_err(&pdev->dev, "ioremap failed for namtso_pmu\n");
		return -ENOMEM;
	}

	raw_spin_lock_init(&namtso_pmu->pmu_lock);
	spin_lock_init(&namtso_pmu->fcm_idle_lock);
	cpumask_copy(&namtso_pmu->associated_cpus, cpu_online_mask);

	namtso_pmu->dev = &pdev->dev;
	namtso_pmu->check_event = NAMTSO_EV_ID_MAX;
	namtso_pmu->num_counters = -1;
	namtso_pmu->fcm_idle = false;

	return 0;
}

static int namtso_pmu_probe(struct platform_device *pdev)
{
	struct namtso_pmu *namtso_pmu = NULL;
	struct device *dev = &pdev->dev;
	char *name = NULL;
	int ret;
	int irq;

	dev_err(dev, "%s enter\n", __func__);
	namtso_pmu = devm_kzalloc(dev, sizeof(*namtso_pmu), GFP_KERNEL);
	if (namtso_pmu == NULL)
		return -ENOMEM;

	platform_set_drvdata(pdev, namtso_pmu);
	dev_set_name(dev, "namtso_pmu");

	name = devm_kasprintf(dev, GFP_KERNEL, "%s", DRVNAME);
	if (name == NULL)
		return -ENOMEM;

	ret = namtso_pmu_dev_init(pdev, namtso_pmu);
	if (ret != 0)
		return ret;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(dev, "failed to find namtso pmu IRQ\n");
		ret = -EINVAL;
		goto err;
	}

	ret = devm_request_irq(dev, irq, namtso_pmu_handle_irq,
			       IRQF_NOBALANCING, name, namtso_pmu);
	if (ret != 0) {
		dev_err(dev, "failed to request irq\n");
		goto err;
	}
	namtso_pmu->irq = irq;

	ret = cpuhp_state_add_instance(namtso_pmu_cpuhp_state,
				       &namtso_pmu->cpuhp_node);
	if (ret != 0) {
		dev_err(dev, "error %d registering hotplug\n", ret);
		goto cpuhp;
	}

	namtso_pmu->pmu = (struct pmu) {
		.task_ctx_nr		= perf_invalid_context,
		.module			= THIS_MODULE,
		.event_init		= namtso_pmu_event_init,
		.pmu_enable		= namtso_pmu_enable,
		.pmu_disable		= namtso_pmu_disable,
		.add			= namtso_pmu_add,
		.del			= namtso_pmu_del,
		.start			= namtso_pmu_start,
		.stop			= namtso_pmu_stop,
		.read			= namtso_pmu_read,
#ifdef CONFIG_NAMTSO_PMU_DEBUG
		.attr_groups	= namtso_pmu_attr_groups,
#endif
	};

	ret = cpu_pm_namtso_pmu_register(namtso_pmu);
	if (ret != 0) {
		dev_err(dev, "cpu_pm register failed\n");
		goto cpu_pm;
	}

	ret = perf_pmu_register(&namtso_pmu->pmu, name, PERF_TYPE_NAMTSO);
	if (ret != 0) {
		dev_err(dev, "namtso pmu register failed\n");
		goto pmu_reg;
	}

	dev_err(dev, "%s exit\n", __func__);
	return ret;

pmu_reg:
	cpu_pm_namtso_pmu_unregister(namtso_pmu);
cpu_pm:
	cpuhp_state_remove_instance(namtso_pmu_cpuhp_state,
				    &namtso_pmu->cpuhp_node);
cpuhp:
	irq_set_affinity_hint(namtso_pmu->irq, NULL);
err:
	iounmap(namtso_pmu->base);
	return ret;
}

static int namtso_pmu_remove(struct platform_device *pdev)
{
	struct namtso_pmu *namtso_pmu = platform_get_drvdata(pdev);

	perf_pmu_unregister(&namtso_pmu->pmu);
	cpu_pm_namtso_pmu_unregister(namtso_pmu);
	cpuhp_state_remove_instance(namtso_pmu_cpuhp_state,
				    &namtso_pmu->cpuhp_node);
	irq_set_affinity_hint(namtso_pmu->irq, NULL);
	iounmap(namtso_pmu->base);

	return 0;
}

static const struct of_device_id namtso_pmu_match[] = {
	{ .compatible = "hisi,namtso_pmu", },
	{},
};

MODULE_DEVICE_TABLE(of, namtso_pmu_match);

static struct platform_driver namtso_pmu_driver = {
	.driver = {
		.name = DRVNAME,
		.of_match_table = namtso_pmu_match,
	},
	.probe = namtso_pmu_probe,
	.remove = namtso_pmu_remove,
};

static void namtso_pmu_set_active_cpu(int cpu, struct namtso_pmu *namtso_pmu)
{
	cpumask_set_cpu(cpu, &namtso_pmu->active_cpu);
	if (irq_set_affinity_hint(namtso_pmu->irq, &namtso_pmu->active_cpu))
		pr_err("namtso_pmu: failed to set irq affinity to %d\n", cpu);
}

static void namtso_pmu_probe_pmu(struct namtso_pmu *namtso_pmu)
{
	struct device *dev = namtso_pmu->dev;
	u32 num_counters;
	u32 cpmceid[2];
	u32 value;

	value = readl(namtso_pmu->base + NAMTSO_PMCR_OFFSET);
	num_counters = (value >> NAMTSO_PMCR_N_SHIFT) & NAMTSO_PMCR_N_MASK;
	dev_dbg(dev, "num_counters = %u\n", num_counters);

	if (WARN_ON(num_counters > 31))
		num_counters = 31;

	namtso_pmu->num_counters = num_counters;
	if (num_counters == 0)
		return;

	cpmceid[0] = readl(namtso_pmu->base + NAMTSO_PMCEID0_OFFSET);
	cpmceid[1] = readl(namtso_pmu->base + NAMTSO_PMCEID1_OFFSET);
	dev_dbg(dev, "cpmceid[0] = 0x%x\n", cpmceid[0]);
	dev_dbg(dev, "cpmceid[1] = 0x%x\n", cpmceid[1]);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0))
	bitmap_from_arr32(namtso_pmu->cpmceid_bitmap, cpmceid,
			  NAMTSO_PMU_MAX_COMMON_EVENTS);
#else
	bitmap_from_u32array(namtso_pmu->cpmceid_bitmap,
			     NAMTSO_PMU_MAX_COMMON_EVENTS,
			     cpmceid,
			     ARRAY_SIZE(cpmceid));
#endif
}

static void namtso_pmu_init_pmu(struct namtso_pmu *namtso_pmu)
{
	if (namtso_pmu->num_counters == -1)
		namtso_pmu_probe_pmu(namtso_pmu);

	namtso_pmu_get_reset_overflow(namtso_pmu);
}

static int namtso_pmu_cpu_online(unsigned int cpu, struct hlist_node *node)
{
	struct namtso_pmu *namtso_pmu = hlist_entry_safe(node,
							 struct namtso_pmu,
							 cpuhp_node);

	if (namtso_pmu == NULL ||
	    cpumask_test_cpu(cpu, &namtso_pmu->associated_cpus) == 0)
		return 0;

	/* If another CPU is already managing this PMU, simply return. */
	if (!cpumask_empty(&namtso_pmu->active_cpu))
		return 0;

	namtso_pmu_init_pmu(namtso_pmu);
	namtso_pmu_set_active_cpu(cpu, namtso_pmu);

	return 0;
}

static int get_online_cpu_any_but(struct namtso_pmu *namtso_pmu, int cpu)
{
	struct cpumask online_supported;

	cpumask_and(&online_supported,
		    &namtso_pmu->associated_cpus, cpu_online_mask);
	return cpumask_any_but(&online_supported, cpu);
}

static int namtso_pmu_cpu_offline(unsigned int cpu, struct hlist_node *node)
{
	struct namtso_pmu *namtso_pmu = hlist_entry_safe(node,
							 struct namtso_pmu,
							 cpuhp_node);
	unsigned int target;

	if (namtso_pmu == NULL ||
	    cpumask_test_and_clear_cpu(cpu, &namtso_pmu->active_cpu) == 0)
		return 0;

	target = get_online_cpu_any_but(namtso_pmu, cpu);
	if (target >= nr_cpu_ids) {
		pr_err("namtso_pmu: target %u is invalid\n", target);
		irq_set_affinity_hint(namtso_pmu->irq, NULL);
		return 0;
	}

	perf_pmu_migrate_context(&namtso_pmu->pmu, cpu, target);
	namtso_pmu_set_active_cpu(target, namtso_pmu);

	return 0;
}

static int __init namtso_pmu_module_init(void)
{
	int ret;

	ret = cpuhp_setup_state_multi(CPUHP_AP_ONLINE_DYN,
				      DRVNAME,
				      namtso_pmu_cpu_online,
				      namtso_pmu_cpu_offline);
	if (ret < 0) {
		pr_err("NAMTSO PMU: Error setup hotplug, ret = %d\n", ret);
		return ret;
	}

	namtso_pmu_cpuhp_state = ret;
	ret = platform_driver_register(&namtso_pmu_driver);
	if (ret != 0)
		cpuhp_remove_multi_state(namtso_pmu_cpuhp_state);

	return ret;
}
module_init(namtso_pmu_module_init);

static void __exit namtso_pmu_module_exit(void)
{
	platform_driver_unregister(&namtso_pmu_driver);
	cpuhp_remove_multi_state(namtso_pmu_cpuhp_state);
}
module_exit(namtso_pmu_module_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("HISI namtso pmu driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
