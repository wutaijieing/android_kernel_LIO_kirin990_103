/*
 * l3p_karma.c
 *
 * L3Cache Karma DVFS Driver
 *
 * Copyright (c) 2018-2020 Huawei Technologies Co., Ltd.
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
#include <linux/devfreq.h>
#include <linux/ktime.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/version.h>
#include <asm/compiler.h>
#include <trace/events/power.h>
#include <linux/cpu_pm.h>
#include <linux/cpu.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/perf_event.h>
#include <l3p_karma.h>
#include <linux/platform_drivers/ddr_perf_ctrl.h>
#include <securec.h>
#include "governor.h"

#define CREATE_TRACE_POINTS
#include <trace/events/l3p_karma.h>
#include "l3cache_common.h"

#define L3P_KARMA_PLATFORM_DEVICE_NAME			"l3p_karma"
#define L3P_KARMA_GOVERNOR_NAME				"l3p_karma_gov"
#define L3P_KARMA_DEFAULT_POLLING_MS			20
#define WIDTH_TO_MASK(width)				((1UL << (width)) - 1)

/* CPU PMU EVENT ID */
#define INST_EV						0x08
#define CYC_EV						0x11

/* DSU PMU EVENT ID */
#define DSU_L3D_CACHE_EV				0x02B
#define DSU_L3D_REFILL_EV				0x02A
#define DSU_ACP_EV					0x119
#define DSU_HZD_ADDR_EV					0x0D0
#define DSU_CYCLE_EV					0x011
#define KARMA_ENABLE_ACP_OUTSTANDING			0xF8000
#define WORK_MS_INIT            			4

#ifdef CPU_HWMON_DEBUG
/* CPU PMU EVENT IDX */
enum cpu_ev_idx {
	INST_IDX,
	CYC_IDX,
	CPU_NUM_EVENTS
};
#endif

/* KARMA PMU EVENT IDX */
enum karma_ev_index {
	ACP_HIT_IDX,
	PT_GEN_IDX,
	REP_GOOD_IDX,
	DCP_GOOD_IDX,
	REP_GEN_IDX,
	DCP_GEN_IDX,
	KARMA_NUM_EVENTS
};

/* DSU PMU EVENT IDX */
enum dsu_ev_index {
	DSU_L3D_CACHE_IDX,
	DSU_L3D_REFILL_IDX,
	DSU_ACP_IDX,
	DSU_HZD_ADDR_IDX,
	DSU_CYCLE_IDX,
	DSU_NUM_EVENTS
};

/* monitor_enable vote souces */
enum vote_src_mon {
	VOTE_NODE,
	VOTE_SHARE,
	VOTE_MAX
};

struct event_data {
	struct perf_event *pevent;
	unsigned long prev_count;
};

struct cpu_evt_count {
	unsigned long inst_cnt;
	unsigned long cyc_cnt;
};

struct karma_evt_count {
	unsigned long acp_hit_cnt;
	unsigned long pt_gen_cnt;
	unsigned long rep_gd_cnt;
	unsigned long dcp_gd_cnt;
	unsigned long rep_gen_cnt;
	unsigned long dcp_gen_cnt;
};

struct dsu_evt_count {
	unsigned long l3d_cnt;
	unsigned long l3d_refill_cnt;
	unsigned long acp_cnt;
	unsigned long hzd_addr_cnt;
	unsigned long cycle_cnt;
};

#ifdef CPU_HWMON_DEBUG
struct cpu_evt_data {
	struct event_data events[CPU_NUM_EVENTS];
	ktime_t prev_ts;
};
#endif

struct karma_evt_data {
	struct event_data events[KARMA_NUM_EVENTS];
};

struct dsu_evt_data {
	struct event_data events[DSU_NUM_EVENTS];
};

#ifdef CPU_HWMON_DEBUG
struct cpu_grp_info {
	cpumask_t cpus;
	cpumask_t inited_cpus;
	unsigned int event_ids[CPU_NUM_EVENTS];
	struct cpu_evt_data *evtdata;
	struct notifier_block perf_cpu_notif;
	struct list_head mon_list;
};

struct cpu_hwmon {
	int (*start_hwmon)(struct cpu_hwmon *hw);
	void (*stop_hwmon)(struct cpu_hwmon *hw);
	unsigned long (*get_cnt)(struct cpu_hwmon *hw);
	struct cpu_grp_info *cpu_grp;
	cpumask_t cpus;
	unsigned int num_cores;
	struct cpu_evt_count *evtcnts;
	unsigned long total_inst;
	unsigned long total_cyc;
};
#endif

struct karma_hwmon {
	int (*start_hwmon)(struct karma_hwmon *hw);
	void (*stop_hwmon)(struct karma_hwmon *hw);
	unsigned long (*get_cnt)(struct karma_hwmon *hw);
	unsigned int event_ids[KARMA_NUM_EVENTS];
	struct karma_evt_count count;
	struct karma_evt_data hw_data;
};

struct dsu_hwmon {
	int (*start_hwmon)(struct dsu_hwmon *hw);
	void (*stop_hwmon)(struct dsu_hwmon *hw);
	unsigned long (*get_cnt)(struct dsu_hwmon *hw);
	unsigned int event_ids[DSU_NUM_EVENTS];
	struct dsu_evt_count count;
	struct dsu_evt_data hw_data;
};

#define to_evtdata(cpu_grp, cpu) \
	(&cpu_grp->evtdata[cpu - cpumask_first(&cpu_grp->cpus)])
#define to_evtcnts(hw, cpu) \
	(&hw->evtcnts[cpu - cpumask_first(&hw->cpus)])

static LIST_HEAD(g_perf_mon_list);
static DEFINE_MUTEX(g_list_lock);

struct set_reg {
	u32 offset;
	u32 shift;
	u32 width;
	u32 value;
};

struct init_reg {
	u32 offset;
	u32 value;
};

struct profile_data {
	struct set_reg *opp_entry;
	u8 opp_count;
};

struct time_type {
	unsigned long usec_delta;
	unsigned int last_update;
};

struct l3p_karma {
	struct devfreq *devfreq;
	struct platform_device *pdev;
	struct devfreq_dev_profile *devfreq_profile;
	void __iomem *base;
	u32 polling_ms;
	unsigned int monitor_enable;
	bool mon_started;
	u8 vote_mon_dis;
	/* to protect operation of hw monitor */
	struct mutex mon_mutex_lock;
	u32 control_data[KARMA_SYS_CTRL_NUM - 1];
	bool fcm_idle;
	/* to protect operation of karma system control registers */
	spinlock_t data_spinlock;

	struct profile_data *profile_table;
	unsigned long *freq_table;
	int table_len;
	unsigned long initial_freq;
	unsigned long cur_freq;
	unsigned long freq_min;
	unsigned long freq_max;
	struct time_type time;
	struct workqueue_struct *update_wq;
	struct delayed_work work;
	unsigned int work_ms;
#ifdef CONFIG_L3P_KARMA_DEBUG
	unsigned int off_debug;
#endif
	unsigned int quick_sleep_enable;

#ifdef CPU_HWMON_DEBUG
	struct cpu_hwmon *cpu_hw;
#endif
	struct karma_hwmon *karma_hw;
	struct dsu_hwmon *dsu_hw;
	struct ddr_flux flux;

	int neural_data[LEN_MAX];

	struct notifier_block idle_nb;
	struct notifier_block acp_notify;
	struct attribute_group *attr_grp;
};

static const struct of_device_id l3p_karma_devfreq_id[] = {
	{.compatible = "hisi,l3p_karma"},
	{}
};

/*
 * freq's value is in range [1,3],
 * However, mode'value is in range [0,2]
 * freq does not start with 0, as can not lock by
 * min_freq/max_freq
 * Moreover, mode is used as subscript to reference array
 */
static enum profile_mode freq_to_mode(unsigned long freq)
{
	return freq - 1;
}

static unsigned long mode_to_freq(enum profile_mode mode)
{
	return mode + 1;
}

static int l3p_start_monitor(struct devfreq *df)
{
	struct l3p_karma *l3p = dev_get_drvdata(df->dev.parent);
#ifdef CPU_HWMON_DEBUG
	struct cpu_hwmon *cpu_hw = NULL;
#endif
	struct karma_hwmon *karma_hw = NULL;
	struct dsu_hwmon *dsu_hw = NULL;
	struct device *dev = NULL;
	int ret;

	if (l3p->monitor_enable == 0 || l3p->mon_started)
		return 0;

#ifdef CPU_HWMON_DEBUG
	cpu_hw = l3p->cpu_hw;
#endif
	karma_hw = l3p->karma_hw;
	dsu_hw = l3p->dsu_hw;
	dev = df->dev.parent;

#ifdef CPU_HWMON_DEBUG
	ret = cpu_hw->start_hwmon(cpu_hw);
	if (ret != 0) {
		dev_err(dev, "Unable to start CPU HW monitor %d\n", ret);
		goto cpu_err;
	}
#endif

	ret = karma_hw->start_hwmon(karma_hw);
	if (ret != 0) {
		dev_err(dev, "Unable to start KARMA HW monitor %d\n", ret);
		goto cpu_err;
	}

	ret = dsu_hw->start_hwmon(dsu_hw);
	if (ret != 0) {
		dev_err(dev, "Unable to start DSU HW monitor %d\n", ret);
		goto karma_err;
	}

	devfreq_monitor_start(df);
	l3p->mon_started = true;

	return 0;

karma_err:
	karma_hw->stop_hwmon(karma_hw);
cpu_err:
#ifdef CPU_HWMON_DEBUG
	cpu_hw->stop_hwmon(cpu_hw);
#endif
	return ret;
}

static void l3p_stop_monitor(struct devfreq *df)
{
	struct l3p_karma *l3p = dev_get_drvdata(df->dev.parent);
#ifdef CPU_HWMON_DEBUG
	struct cpu_hwmon *cpu_hw = NULL;
#endif
	struct karma_hwmon *karma_hw = NULL;
	struct dsu_hwmon *dsu_hw = NULL;

	if (l3p->monitor_enable == 0 || !l3p->mon_started)
		return;

#ifdef CPU_HWMON_DEBUG
	cpu_hw = l3p->cpu_hw;
#endif
	karma_hw = l3p->karma_hw;
	dsu_hw = l3p->dsu_hw;

	l3p->mon_started = false;

	devfreq_monitor_stop(df);
#ifdef CPU_HWMON_DEBUG
	cpu_hw->stop_hwmon(cpu_hw);
#endif
	karma_hw->stop_hwmon(karma_hw);
	dsu_hw->stop_hwmon(dsu_hw);
}

static int l3p_karma_init(struct platform_device *pdev)
{
	struct l3p_karma *l3p = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	struct init_reg *init_table = NULL;
	struct device_node *node = pdev->dev.of_node;
	struct device_node *parent = NULL;
	struct device_node *init = NULL;
	int init_count;
	int i, offset, value;
	int ret = 0;

	of_node_get(node);

	parent = of_get_child_by_name(node, "initialize");
	if (parent == NULL) {
		dev_err(dev, "Failed to find initialize node\n");
		ret = -EINVAL;
		goto err_out;
	}

	init_count = of_get_child_count(parent);
	if (init_count == 0) {
		dev_err(dev, "no any initialize config\n");
		ret = -EINVAL;
		goto err_out;
	}
	dev_info(dev, "init_count = %d\n", init_count);

	init_table = devm_kzalloc(dev, sizeof(struct init_reg) *
				  init_count, GFP_ATOMIC);
	if (IS_ERR_OR_NULL(init_table)) {
		dev_err(dev, "Failed to allocate for karma init_table\n");
		ret =  -ENOMEM;
		goto err_out;
	}

	i = 0;
	for_each_child_of_node(parent, init) {
		dev_info(dev, "init name:%s\n", init->name);
		ret = of_property_read_u32(init, "offset",
					   &init_table[i].offset);
		if (ret != 0) {
			dev_err(dev, "parse %s offset failed%d!\n",
				init->name, ret);
			goto err_free;
		}
		dev_info(dev, "offset = 0x%x\n", init_table[i].offset);

		ret = of_property_read_u32(init, "value",
					   &init_table[i].value);
		if (ret != 0) {
			dev_err(dev, "parse %s value failed%d!\n",
				init->name, ret);
			goto err_free;
		}
		dev_info(dev, "value = 0x%x\n", init_table[i].value);
		i++;
	}

	for (i = 0; i < init_count; i++) {
		offset = init_table[i].offset;
		value = init_table[i].value;
		writel(value, l3p->base + offset);
	}

err_free:
	devm_kfree(dev, init_table);
err_out:
	of_node_put(node);
	return ret;
}

static void l3p_karma_enable(struct l3p_karma *l3p)
{
	/* enable karma */
	writel(1, (l3p->base + KARMA_MAIN_ENABLE_OFFSET));
}

#define KARMA_TIME_OUT		400
static void l3p_karma_disable(struct l3p_karma *l3p)
{
	enum profile_mode mode;
	u32 timeout = KARMA_TIME_OUT;
	u32 value;

	/* disable karma */
	writel(0, (l3p->base + KARMA_MAIN_ENABLE_OFFSET));

	/*
	 * if cur mode is sleep mode, enable ACP outstanding firstly,
	 * otherwise can not wait idle status
	 * bit15:19  ACP outstanding
	 */
	mode = freq_to_mode(l3p->cur_freq);
	if (mode == SLEEP_MODE) {
		value = readl(l3p->base + KARMA_GLOBAL_CONTROL_OFFSET);
		value |= KARMA_ENABLE_ACP_OUTSTANDING;
		writel(value, l3p->base + KARMA_GLOBAL_CONTROL_OFFSET);
	}

	/* check ACP outstanding is 0 or not, at last */
	value = readl(l3p->base + KARMA_GLOBAL_CONTROL_OFFSET);
	if ((value & KARMA_ENABLE_ACP_OUTSTANDING) == 0)
		BUG_ON(1);

	value = readl(l3p->base + KARMA_IDLE_STATUS_OFFSET);
	while ((value & BIT(0)) == 0) {
		value = readl(l3p->base + KARMA_IDLE_STATUS_OFFSET);
		udelay(1);

		timeout--;
		if (timeout == 0) {
			pr_err("l3p_karma: disable karma timeout\n");
			BUG_ON(1);
		}
	}
}

static void l3p_save_reg_data(struct l3p_karma *l3p)
{
	int i;
	u32 offset, value;

	/* not include idle status register */
	for (i = 0; i < (KARMA_SYS_CTRL_NUM - 1); i++) {
		offset = i * sizeof(u32);
		value = readl(l3p->base + offset);
		l3p->control_data[i] = value;
	}
}

static void l3p_restore_reg_data(struct l3p_karma *l3p)
{
	int i;
	u32 enable;
	u32 offset, value;

	/*
	 * if karma is enabled, disable it firstly
	 * eg: fcm not power off/on in fact
	 * suspend abort
	 */
	enable = readl(l3p->base + KARMA_MAIN_ENABLE_OFFSET);
	if ((enable & BIT(0)) == 1)
		l3p_karma_disable(l3p);

	/* firstly restore exclude enable register, at last do it */
	for (i = 1; i < (KARMA_SYS_CTRL_NUM - 1); i++) {
		offset = i * sizeof(u32);
		value = l3p->control_data[i];
		writel(value, (l3p->base + offset));
	}
	writel(l3p->control_data[0], (l3p->base + KARMA_MAIN_ENABLE_OFFSET));
}

#ifdef CONFIG_L3P_KARMA_DEBUG
static ssize_t show_monitor_enable(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	ssize_t ret;
	struct devfreq *df = to_devfreq(dev);
	struct l3p_karma *l3p = dev_get_drvdata(df->dev.parent);

	mutex_lock(&l3p->mon_mutex_lock);
	ret = scnprintf(buf, PAGE_SIZE, "monitor_enable=%u\n",
			l3p->monitor_enable);
	mutex_unlock(&l3p->mon_mutex_lock);

	return ret;
}

static ssize_t store_monitor_enable(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct devfreq *df = to_devfreq(dev);
	struct l3p_karma *l3p = dev_get_drvdata(df->dev.parent);
	unsigned int val = 0;
	int ret;

	ret = kstrtouint(buf, 10, &val);
	if (ret != 0)
		return ret;

	if (val != 0)
		val = 1;

	mutex_lock(&l3p->mon_mutex_lock);
	if (val == 1)
		l3p->vote_mon_dis &= (~BIT(VOTE_NODE));
	else if (val == 0)
		l3p->vote_mon_dis |= BIT(VOTE_NODE);

	if (l3p->monitor_enable == 0 && val == 1 && l3p->vote_mon_dis == 0) {
		l3p->monitor_enable = 1;
		ret = l3p_start_monitor(df);
		if (ret != 0) {
			/* handle error */
			l3p->monitor_enable = 0;
			ret = -EINVAL;
		} else {
			/* enable karma */
			spin_lock(&l3p->data_spinlock);
			l3p_restore_reg_data(l3p);
			l3p_karma_enable(l3p);
			spin_unlock(&l3p->data_spinlock);
		}
	} else if (l3p->monitor_enable == 1 && val == 0) {
		/*
		 * NOTICE
		 * this thread can not sleep before save register data,
		 * if get mon_mutex_lock already
		 */
		spin_lock(&l3p->data_spinlock);
		l3p_karma_disable(l3p);
		l3p_save_reg_data(l3p);
		spin_unlock(&l3p->data_spinlock);
		l3p_stop_monitor(df);
		l3p->monitor_enable = 0;
	}
	mutex_unlock(&l3p->mon_mutex_lock);

	return ret ? ret : count;
}
static DEVICE_ATTR(monitor_enable, 0660,
		   show_monitor_enable, store_monitor_enable);

static ssize_t show_reg_cfg(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	ssize_t ret;
	struct devfreq *df = to_devfreq(dev);
	struct l3p_karma *l3p = dev_get_drvdata(df->dev.parent);
	ssize_t count = 0;
	int i;

	spin_lock(&l3p->data_spinlock);
	for (i = 0; i < KARMA_SYS_CTRL_NUM; i++) {
		ret = snprintf_s(buf + count,
				 (PAGE_SIZE - count),
				 (PAGE_SIZE - count - 1),
				 "offset:0x%x \t value:0x%x\n",
				 (i * 4),
				 readl(l3p->base + i * 4));

		if (ret < 0)
			goto err;
		count += ret;
		if ((unsigned int)count >= PAGE_SIZE)
			goto err;
	}
err:
	spin_unlock(&l3p->data_spinlock);
	return count;
}
static DEVICE_ATTR(reg_cfg, 0660, show_reg_cfg, NULL);

/* quick_sleep_enable */
static ssize_t show_quick_sleep_enable(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	ssize_t ret;
	struct devfreq *df = to_devfreq(dev);
	struct l3p_karma *l3p = dev_get_drvdata(df->dev.parent);

	mutex_lock(&l3p->devfreq->lock);
	ret = scnprintf(buf, PAGE_SIZE, "quick_sleep_enable=%u\n",
			l3p->quick_sleep_enable);
	mutex_unlock(&l3p->devfreq->lock);

	return ret;
}

static ssize_t store_quick_sleep_enable(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct l3p_karma *l3p = dev_get_drvdata(devfreq->dev.parent);
	unsigned int val = 0;
	int ret;

	ret = kstrtouint(buf, 10, &val);
	if (ret != 0) {
		dev_err(dev, "input data is not valid, val = %u\n", val);
		return -EINVAL;
	}

	mutex_lock(&l3p->devfreq->lock);
	l3p->quick_sleep_enable = val;
	ret = update_devfreq(devfreq);
	if (ret == 0)
		ret = count;
	mutex_unlock(&l3p->devfreq->lock);

	return ret;
}

static DEVICE_ATTR(quick_sleep_enable, 0660,
	show_quick_sleep_enable, store_quick_sleep_enable);

/* work_ms */
static ssize_t show_work_ms(struct device *dev,
			    struct device_attribute *attr, char *buf)
{
	ssize_t ret;
	struct devfreq *df = to_devfreq(dev);
	struct l3p_karma *l3p = dev_get_drvdata(df->dev.parent);

	mutex_lock(&l3p->devfreq->lock);
	ret = scnprintf(buf, PAGE_SIZE, "work_ms=%u\n", l3p->work_ms);
	mutex_unlock(&l3p->devfreq->lock);

	return ret;
}

#define WORK_MS_MIN		2
#define WORK_MS_MAX		20
static ssize_t store_work_ms(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct l3p_karma *l3p = dev_get_drvdata(devfreq->dev.parent);
	unsigned int val = 0;
	int ret;

	ret = kstrtouint(buf, 10, &val);
	if (ret != 0 || val > WORK_MS_MAX || val < WORK_MS_MIN) {
		dev_err(dev, "input data is not valid, val = %u\n", val);
		return -EINVAL;
	}

	mutex_lock(&l3p->devfreq->lock);
	l3p->work_ms = val;
	ret = update_devfreq(devfreq);
	if (ret == 0)
		ret = count;
	mutex_unlock(&l3p->devfreq->lock);

	return ret;
}

static DEVICE_ATTR(work_ms, 0660, show_work_ms, store_work_ms);

/* off_debug */
static ssize_t show_off_debug(struct device *dev,
			      struct device_attribute *attr, char *buf)
{
	ssize_t ret;
	struct devfreq *df = to_devfreq(dev);
	struct l3p_karma *l3p = dev_get_drvdata(df->dev.parent);

	mutex_lock(&l3p->devfreq->lock);
	ret = scnprintf(buf, PAGE_SIZE, "off_debug=%u\n", l3p->off_debug);
	mutex_unlock(&l3p->devfreq->lock);

	return ret;
}

static ssize_t store_off_debug(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t count)
{
	struct devfreq *devfreq = to_devfreq(dev);
	struct l3p_karma *l3p = dev_get_drvdata(devfreq->dev.parent);
	unsigned int val = 0;
	int ret;

	ret = kstrtouint(buf, 10, &val);
	if (ret != 0) {
		dev_err(dev, "input data is not valid, val = %u\n", val);
		return -EINVAL;
	}

	mutex_lock(&l3p->devfreq->lock);
	l3p->off_debug = val;
	if (l3p->off_debug != 0) {
		ret = update_devfreq(devfreq);
		if (ret == 0)
			ret = count;
		spin_lock(&l3p->data_spinlock);
		l3p_karma_disable(l3p);
		spin_unlock(&l3p->data_spinlock);
	} else {
		spin_lock(&l3p->data_spinlock);
		l3p_karma_enable(l3p);
		spin_unlock(&l3p->data_spinlock);
		ret = update_devfreq(devfreq);
		if (ret == 0)
			ret = count;
	}
	mutex_unlock(&l3p->devfreq->lock);

	return ret;
}
static DEVICE_ATTR(off_debug, 0660, show_off_debug, store_off_debug);

static struct attribute *dev_attr[] = {
	&dev_attr_monitor_enable.attr,
	&dev_attr_quick_sleep_enable.attr,
	&dev_attr_work_ms.attr,
	&dev_attr_reg_cfg.attr,
	&dev_attr_off_debug.attr,
	NULL,
};

static struct attribute_group dev_attr_group = {
	.name = "karma",
	.attrs = dev_attr,
};
#endif

static unsigned long read_event(struct event_data *event)
{
	unsigned long ev_count;
	u64 total, enabled, running;

	if (event->pevent == NULL) {
		pr_err("l3p_karma: %s pevent is NULL\n", __func__);
		return 0;
	}

	total = perf_event_read_value(event->pevent, &enabled, &running);
	ev_count = total - event->prev_count;
	event->prev_count = total;
	return ev_count;
}

#ifdef CPU_HWMON_DEBUG
static void cpu_read_perf_counters(int cpu, struct cpu_hwmon *hw)
{
	struct cpu_grp_info *cpu_grp = hw->cpu_grp;
	struct cpu_evt_data *evtdata = to_evtdata(cpu_grp, cpu);
	struct cpu_evt_count *evtcnts = to_evtcnts(hw, cpu);

	evtcnts->inst_cnt = read_event(&evtdata->events[INST_IDX]);
	evtcnts->cyc_cnt = read_event(&evtdata->events[CYC_IDX]);
}

static unsigned long cpu_get_cnt(struct cpu_hwmon *hw)
{
	int cpu;
	struct cpu_grp_info *cpu_grp = hw->cpu_grp;
	struct cpu_evt_count *evtcnts = NULL;

	hw->total_inst = 0;
	hw->total_cyc = 0;
	for_each_cpu(cpu, &cpu_grp->inited_cpus) {
		cpu_read_perf_counters(cpu, hw);
		evtcnts = to_evtcnts(hw, cpu);
		hw->total_inst += evtcnts->inst_cnt;
		hw->total_cyc += evtcnts->cyc_cnt;
	}
	return 0;
}
#endif

static unsigned long karma_get_cnt(struct karma_hwmon *hw)
{
	struct karma_evt_data *hw_data = &hw->hw_data;

	hw->count.acp_hit_cnt =
			read_event(&hw_data->events[ACP_HIT_IDX]);
	hw->count.pt_gen_cnt =
			read_event(&hw_data->events[PT_GEN_IDX]);
	hw->count.rep_gd_cnt =
			read_event(&hw_data->events[REP_GOOD_IDX]);
	hw->count.dcp_gd_cnt =
			read_event(&hw_data->events[DCP_GOOD_IDX]);
	hw->count.rep_gen_cnt =
			read_event(&hw_data->events[REP_GEN_IDX]);
	hw->count.dcp_gen_cnt =
			read_event(&hw_data->events[DCP_GEN_IDX]);

	return 0;
}

static unsigned long dsu_get_cnt(struct dsu_hwmon *hw)
{
	struct dsu_evt_data *hw_data = &hw->hw_data;

	hw->count.l3d_cnt =
			read_event(&hw_data->events[DSU_L3D_CACHE_IDX]);
	hw->count.l3d_refill_cnt =
			read_event(&hw_data->events[DSU_L3D_REFILL_IDX]);
	hw->count.acp_cnt =
			read_event(&hw_data->events[DSU_ACP_IDX]);
	hw->count.hzd_addr_cnt =
			read_event(&hw_data->events[DSU_HZD_ADDR_IDX]);
	hw->count.cycle_cnt =
			read_event(&hw_data->events[DSU_CYCLE_IDX]);

	return 0;
}

#ifdef CPU_HWMON_DEBUG
static void cpu_delete_events(struct cpu_evt_data *evtdata)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(evtdata->events); i++) {
		evtdata->events[i].prev_count = 0;
		if (evtdata->events[i].pevent) {
			perf_event_disable(evtdata->events[i].pevent);
			perf_event_release_kernel(evtdata->events[i].pevent);
			evtdata->events[i].pevent = NULL;
		}
	}
}
#endif

static void karma_delete_events(struct karma_evt_data *hw_data)
{
	int i;

	for (i = 0; i < KARMA_NUM_EVENTS; i++) {
		hw_data->events[i].prev_count = 0;
		if (hw_data->events[i].pevent) {
			perf_event_release_kernel(hw_data->events[i].pevent);
			hw_data->events[i].pevent = NULL;
		}
	}
}

static void dsu_delete_events(struct dsu_evt_data *hw_data)
{
	int i;

	for (i = 0; i < DSU_NUM_EVENTS; i++) {
		hw_data->events[i].prev_count = 0;
		if (hw_data->events[i].pevent) {
			perf_event_release_kernel(hw_data->events[i].pevent);
			hw_data->events[i].pevent = NULL;
		}
	}
}

#ifdef CPU_HWMON_DEBUG
static void cpu_stop_hwmon(struct cpu_hwmon *hw)
{
	int cpu;
	struct cpu_grp_info *cpu_grp = hw->cpu_grp;
	struct cpu_evt_count *evtcnts = NULL;

	get_online_cpus();
	for_each_cpu(cpu, &cpu_grp->inited_cpus) {
		cpu_delete_events(to_evtdata(cpu_grp, cpu));
		evtcnts = to_evtcnts(hw, cpu);
		evtcnts->inst_cnt = 0;
		evtcnts->cyc_cnt = 0;
		hw->total_inst = 0;
		hw->total_cyc = 0;
	}
	mutex_lock(&g_list_lock);
	if (!cpumask_equal(&cpu_grp->cpus, &cpu_grp->inited_cpus))
		list_del(&cpu_grp->mon_list);
	mutex_unlock(&g_list_lock);
	cpumask_clear(&cpu_grp->inited_cpus);
	put_online_cpus();

	cpuhp_remove_state_nocalls(CPUHP_AP_HISI_L3P_KARMA_NOTIFY_PREPARE);
}
#endif

static void karma_stop_hwmon(struct karma_hwmon *hw)
{
	struct karma_evt_data *hw_data = &hw->hw_data;

	karma_delete_events(hw_data);

	hw->count.acp_hit_cnt = 0;
	hw->count.pt_gen_cnt = 0;
	hw->count.rep_gd_cnt = 0;
	hw->count.dcp_gd_cnt = 0;
	hw->count.rep_gen_cnt = 0;
	hw->count.dcp_gen_cnt = 0;
}

static void dsu_stop_hwmon(struct dsu_hwmon *hw)
{
	struct dsu_evt_data *hw_data = &hw->hw_data;

	dsu_delete_events(hw_data);
	hw->count.l3d_cnt = 0;
	hw->count.l3d_refill_cnt = 0;
	hw->count.acp_cnt = 0;
	hw->count.hzd_addr_cnt = 0;
	hw->count.cycle_cnt = 0;
}

#ifdef CPU_HWMON_DEBUG
static struct perf_event_attr *cpu_alloc_attr(void)
{
	struct perf_event_attr *attr = NULL;

	attr = kzalloc(sizeof(*attr), GFP_KERNEL);
	if (attr == NULL)
		return attr;

	attr->type = PERF_TYPE_RAW;
	attr->size = sizeof(struct perf_event_attr);
	attr->pinned = 1;
	attr->exclude_idle = 1;

	return attr;
}
#endif

static struct perf_event_attr *karma_alloc_attr(void)
{
	struct perf_event_attr *attr = NULL;

	attr = kzalloc(sizeof(*attr), GFP_KERNEL);
	if (attr == NULL)
		return attr;

	attr->type = PERF_TYPE_KARMA;
	attr->size = sizeof(struct perf_event_attr);
	attr->pinned = 1;

	return attr;
}

static struct perf_event_attr *dsu_alloc_attr(void)
{
	struct perf_event_attr *attr = NULL;

	attr = kzalloc(sizeof(*attr), GFP_KERNEL);
	if (attr == NULL)
		return attr;

	attr->type = PERF_TYPE_DSU;
	attr->size = sizeof(struct perf_event_attr);
	attr->pinned = 1;

	return attr;
}

#ifdef CPU_HWMON_DEBUG
static int cpu_set_events(struct cpu_grp_info *cpu_grp, int cpu)
{
	struct perf_event *pevent = NULL;
	struct perf_event_attr *attr = NULL;
	int err;
	unsigned int i, j;
	struct cpu_evt_data *evtdata = to_evtdata(cpu_grp, cpu);

	/* Allocate an attribute for event initialization */
	attr = cpu_alloc_attr();
	if (attr == NULL) {
		pr_info("cpu alloc attr failed\n");
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(evtdata->events); i++) {
		attr->config = cpu_grp->event_ids[i];
		pevent = perf_event_create_kernel_counter(attr, cpu, NULL,
							  NULL, NULL);
		if (IS_ERR(pevent))
			goto err_out;

		evtdata->events[i].pevent = pevent;
		perf_event_enable(pevent);
	}

	kfree(attr);
	return 0;

err_out:
	pr_err("l3p_karma: fail to create %d events\n", i);
	for (j = 0; j < i; j++) {
		perf_event_disable(evtdata->events[j].pevent);
		perf_event_release_kernel(evtdata->events[j].pevent);
		evtdata->events[j].pevent = NULL;
	}
	err = PTR_ERR(pevent);
	kfree(attr);
	return err;
}
#endif

static int karma_set_events(struct karma_hwmon *hw, int cpu)
{
	struct perf_event *pevent = NULL;
	struct perf_event_attr *attr = NULL;
	struct karma_evt_data *hw_data = &hw->hw_data;
	unsigned int i, j;
	int err;

	/* Allocate an attribute for event initialization */
	attr = karma_alloc_attr();
	if (attr == NULL) {
		pr_info("karma alloc attr failed\n");
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(hw_data->events); i++) {
		attr->config = hw->event_ids[i];
		pevent = perf_event_create_kernel_counter(attr, cpu, NULL,
							  NULL, NULL);
		if (IS_ERR(pevent))
			goto err_out;

		hw_data->events[i].pevent = pevent;
		perf_event_enable(pevent);
	}

	kfree(attr);
	return 0;

err_out:
	pr_err("[%s] fail to create %d events\n", __func__, i);
	for (j = 0; j < i; j++) {
		perf_event_disable(hw_data->events[j].pevent);
		perf_event_release_kernel(hw_data->events[j].pevent);
		hw_data->events[j].pevent = NULL;
	}
	err = PTR_ERR(pevent);
	kfree(attr);
	return err;
}

static int dsu_set_events(struct dsu_hwmon *hw, int cpu)
{
	struct perf_event *pevent = NULL;
	struct perf_event_attr *attr = NULL;
	struct dsu_evt_data *hw_data = &hw->hw_data;
	unsigned int i, j;
	int err;

	/* Allocate an attribute for event initialization */
	attr = dsu_alloc_attr();
	if (attr == NULL) {
		pr_info("dsu alloc attr failed\n");
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(hw_data->events); i++) {
		attr->config = hw->event_ids[i];
		pevent = perf_event_create_kernel_counter(attr, cpu, NULL,
							  NULL, NULL);
		if (IS_ERR(pevent))
			goto err_out;

		hw_data->events[i].pevent = pevent;
		perf_event_enable(pevent);
	}

	kfree(attr);
	return 0;

err_out:
	pr_err("[%s] fail to create %d events\n", __func__, i);
	for (j = 0; j < i; j++) {
		perf_event_disable(hw_data->events[j].pevent);
		perf_event_release_kernel(hw_data->events[j].pevent);
		hw_data->events[j].pevent = NULL;
	}
	err = PTR_ERR(pevent);
	kfree(attr);
	return err;
}

#ifdef CPU_HWMON_DEBUG
static int __ref l3p_karma_cpu_online(unsigned int cpu)
{
	struct cpu_grp_info *cpu_grp = NULL;
	struct cpu_grp_info *tmp = NULL;

	mutex_lock(&g_list_lock);
	list_for_each_entry_safe(cpu_grp, tmp, &g_perf_mon_list, mon_list) {
		if (!cpumask_test_cpu(cpu, &cpu_grp->cpus) ||
		    cpumask_test_cpu(cpu, &cpu_grp->inited_cpus))
			continue;

		if (cpu_set_events(cpu_grp, cpu))
			pr_warn("Failed to create perf ev for CPU%u\n", cpu);
		else
			cpumask_set_cpu(cpu, &cpu_grp->inited_cpus);

		if (cpumask_equal(&cpu_grp->cpus, &cpu_grp->inited_cpus))
			list_del(&cpu_grp->mon_list);
	}
	mutex_unlock(&g_list_lock);
	return 0;
}

static int cpu_start_hwmon(struct cpu_hwmon *hw)
{
	struct cpu_grp_info *cpu_grp = hw->cpu_grp;
	int cpu;
	int ret = 0;

	cpuhp_setup_state_nocalls(CPUHP_AP_HISI_L3P_KARMA_NOTIFY_PREPARE,
				  "l3p_karma:online",
				  l3p_karma_cpu_online, NULL);
	get_online_cpus();
	for_each_cpu(cpu, &cpu_grp->cpus) {
		ret = cpu_set_events(cpu_grp, cpu);
		if (ret != 0) {
			if (!cpu_online(cpu)) {
				ret = 0;
			} else {
				pr_warn("Perf event init failed on CPU%d\n",
					cpu);
				break;
			}
		} else {
			cpumask_set_cpu(cpu, &cpu_grp->inited_cpus);
		}
	}
	mutex_lock(&g_list_lock);
	if (!cpumask_equal(&cpu_grp->cpus, &cpu_grp->inited_cpus))
		list_add_tail(&cpu_grp->mon_list, &g_perf_mon_list);
	mutex_unlock(&g_list_lock);
	put_online_cpus();

	return ret;
}
#endif

static int karma_start_hwmon(struct karma_hwmon *hw)
{
	int ret;
	int cpu = 0;

	ret = karma_set_events(hw, cpu);
	if (ret != 0) {
		pr_err("[%s]perf event init failed on CPU%d\n", __func__, cpu);
		WARN_ON_ONCE(1);
		return ret;
	}

	return ret;
}

static int dsu_start_hwmon(struct dsu_hwmon *hw)
{
	int ret;
	/* cpu must be 0 */
	int cpu = 0;

	ret = dsu_set_events(hw, cpu);
	if (ret != 0) {
		pr_err("[%s]perf event init failed on CPU%d\n", __func__, cpu);
		WARN_ON_ONCE(1);
		return ret;
	}

	return ret;
}

#ifdef CPU_HWMON_DEBUG
static int cpu_hwmon_setup(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct l3p_karma *l3p = platform_get_drvdata(pdev);
	struct cpu_hwmon *hw = NULL;
	struct cpu_grp_info *cpu_grp = NULL;

	hw = devm_kzalloc(dev, sizeof(*hw), GFP_KERNEL);
	if (IS_ERR_OR_NULL(hw))
		return -ENOMEM;
	l3p->cpu_hw = hw;

	cpu_grp = devm_kzalloc(dev, sizeof(*cpu_grp), GFP_KERNEL);
	if (IS_ERR_OR_NULL(cpu_grp))
		return -ENOMEM;
	hw->cpu_grp = cpu_grp;
	cpumask_copy(&cpu_grp->cpus, cpu_online_mask);
	cpumask_copy(&hw->cpus, &cpu_grp->cpus);

	hw->num_cores = cpumask_weight(&cpu_grp->cpus);
	hw->evtcnts = devm_kzalloc(dev, hw->num_cores *
				sizeof(*hw->evtcnts), GFP_KERNEL);
	if (IS_ERR_OR_NULL(hw->evtcnts))
		return -ENOMEM;

	cpu_grp->evtdata = devm_kzalloc(dev, hw->num_cores *
		sizeof(*cpu_grp->evtdata), GFP_KERNEL);
	if (IS_ERR_OR_NULL(cpu_grp->evtdata))
		return -ENOMEM;
	cpu_grp->event_ids[INST_IDX] = INST_EV;
	cpu_grp->event_ids[CYC_IDX] = CYC_EV;

	hw->start_hwmon = &cpu_start_hwmon;
	hw->stop_hwmon = &cpu_stop_hwmon;
	hw->get_cnt = &cpu_get_cnt;

	return 0;
}
#endif

static int karma_hwmon_setup(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct l3p_karma *l3p = platform_get_drvdata(pdev);
	struct karma_hwmon *hw = NULL;

	hw = devm_kzalloc(dev, sizeof(*hw), GFP_KERNEL);
	if (IS_ERR_OR_NULL(hw))
		return -ENOMEM;

	l3p->karma_hw = hw;

	hw->start_hwmon = &karma_start_hwmon;
	hw->stop_hwmon = &karma_stop_hwmon;
	hw->get_cnt = &karma_get_cnt;

	hw->event_ids[ACP_HIT_IDX] = ACP_HIT_EV;
	hw->event_ids[PT_GEN_IDX] = PT_GEN_EV;
	hw->event_ids[REP_GOOD_IDX] = REP_GOOD_EV;
	hw->event_ids[DCP_GOOD_IDX] = DCP_GOOD_EV;
	hw->event_ids[REP_GEN_IDX] = REP_GEN_EV;
	hw->event_ids[DCP_GEN_IDX] = DCP_GEN_EV;

	return 0;
}

static int dsu_hwmon_setup(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct l3p_karma *l3p = platform_get_drvdata(pdev);
	struct dsu_hwmon *hw = NULL;

	hw = devm_kzalloc(dev, sizeof(*hw), GFP_KERNEL);
	if (IS_ERR_OR_NULL(hw))
		return -ENOMEM;

	l3p->dsu_hw = hw;

	hw->start_hwmon = &dsu_start_hwmon;
	hw->stop_hwmon = &dsu_stop_hwmon;
	hw->get_cnt = &dsu_get_cnt;

	hw->event_ids[DSU_L3D_CACHE_IDX] = DSU_L3D_CACHE_EV;
	hw->event_ids[DSU_L3D_REFILL_IDX] = DSU_L3D_REFILL_EV;
	hw->event_ids[DSU_ACP_IDX] = DSU_ACP_EV;
	hw->event_ids[DSU_HZD_ADDR_IDX] = DSU_HZD_ADDR_EV;
	hw->event_ids[DSU_CYCLE_IDX] = DSU_CYCLE_EV;

	return 0;
}

static int l3p_karma_target_pre(struct l3p_karma *l3p, unsigned long freq)
{
	if (mutex_trylock(&l3p->mon_mutex_lock) == 0)
		return -EPERM;

#ifdef CONFIG_L3P_KARMA_DEBUG
	/* only for test */
	if (l3p->off_debug != 0) {
		mutex_unlock(&l3p->mon_mutex_lock);
		return -EPERM;
	}
#endif

	/*
	 * if devfreq monitor was stopped, by calling update_devfreq
	 * through min_freq/max_freq, it will enable karma
	 */
	if (!l3p->mon_started) {
		mutex_unlock(&l3p->mon_mutex_lock);
		return -EPERM;
	}
	mutex_unlock(&l3p->mon_mutex_lock);

	if (freq > l3p->freq_max || freq < l3p->freq_min) {
		pr_err("l3p_karma: freq %lu is invalid\n", freq);
		return -EINVAL;
	}

	if (freq == l3p->cur_freq)
		return -EPERM;

	return 0;
}

static int l3p_karma_target(struct device *dev,
			    unsigned long *freq, u32 flags)
{
	struct l3p_karma *l3p = dev_get_drvdata(dev);
	struct set_reg *opp_entry = NULL;
	u8 opp_count;
	u32 offset, shift, width;
	u32 value, data;
	enum profile_mode mode;
	int ret;
	int i;

	ret = l3p_karma_target_pre(l3p, *freq);
	if (ret != 0)
		return 0;

	spin_lock(&l3p->data_spinlock);
	if (l3p->fcm_idle) {
		dev_dbg(dev, "do not update cur_freq before fcm_idle changed to false\n");
		goto err;
	}

	mode = freq_to_mode(*freq);
	if (mode >= l3p->table_len) {
		dev_err(dev, "invalid mode %d\n", mode);
		goto err;
	}

	l3p_karma_disable(l3p);

	opp_entry = l3p->profile_table[mode].opp_entry;
	opp_count = l3p->profile_table[mode].opp_count;

	for (i = 0; i < opp_count; i++) {
		offset = opp_entry[i].offset;
		shift = opp_entry[i].shift;
		width = opp_entry[i].width;
		value = opp_entry[i].value;
		if (value > (WIDTH_TO_MASK(width))) {
			dev_err(dev, "value 0x%x is not valid\n", value);
			ret = -EINVAL;
			goto err;
		} else if (offset > KARMA_RW_OFFSET_MAX || (offset % 4) != 0) {
			dev_err(dev, "offset 0x%x is not valid\n", offset);
			ret = -EINVAL;
			goto err;
		}

		data = readl(l3p->base + offset);
		data &= (~(WIDTH_TO_MASK(width) << shift));
		data |= value << shift;
		writel(data, l3p->base + offset);
	}
	/* at last enable karma if it is enabled just now */
	l3p_karma_enable(l3p);

	trace_l3p_karma_target(*freq);

	if (l3p->quick_sleep_enable && mode == EN_TMP_MODE) {
		queue_delayed_work(l3p->update_wq, &l3p->work,
				   msecs_to_jiffies(l3p->work_ms));
	}

	l3p->cur_freq = *freq;
err:
	spin_unlock(&l3p->data_spinlock);
	return ret;
}

static int l3p_df_get_dev_status(struct device *dev,
				 struct devfreq_dev_status *stat)
{
	return 0;
}

static void neuron_hidden(int *factor, const s16 *coef, const s16 *bias,
			  unsigned int row, unsigned int col)
{
	unsigned int i, j;
	int sum;
	int tmp_factor[LEN_MAX] = {0};

	for (i = 0; i < col; i++) {
		sum = 0;
		for (j = 0; j < row; j++)
			sum += factor[j] * coef[j * col + i];
		tmp_factor[i] = sum + bias[i];
		if (tmp_factor[i] < 0)
			tmp_factor[i] = 0;
		tmp_factor[i] = tmp_factor[i] >> AMPLIFY_FACTOR;
	}

	for (i = 0; i < col; i++)
		factor[i] = tmp_factor[i];
}

static void neuron_output(int *factor, const s16 *coef, const s16 *bias,
			  unsigned int row, unsigned int col)
{
	unsigned int i, j;
	int sum;
	int tmp_factor[LEN_MAX] = {0};

	for (i = 0; i < col; i++) {
		sum = 0;
		for (j = 0; j < row; j++)
			sum += factor[j] * coef[j * col + i];
		tmp_factor[i] = sum + bias[i];
		tmp_factor[i] = tmp_factor[i] >> AMPLIFY_FACTOR;
	}

	for (i = 0; i < col; i++)
		factor[i] = tmp_factor[i];
}

static void neuron_rescale(int *factor, const s16 *mean, const s16 *var)
{
	unsigned int i;

	for (i = 0 ; i < FACTOR_MAX; i++) {
		if (var[i])
			factor[i] = ((s16)factor[i] - mean[i]) / var[i];
		else
			factor[i] = 0;
	}
}

#define FACTOR_SCALE_MAX		1024
static void __l3p_get_input_factors(struct l3p_karma *l3p)
{
	struct karma_hwmon *karma_hw = l3p->karma_hw;
	struct karma_evt_count *kam_cnt = &karma_hw->count;
	struct dsu_hwmon *dsu_hw = l3p->dsu_hw;
	struct dsu_evt_count *dsu_cnt = &dsu_hw->count;
	struct ddr_flux *flux = &l3p->flux;
	int *factor = l3p->neural_data;

	if (dsu_cnt->l3d_cnt != 0)
		factor[HZD_RATE] = (int)(dsu_cnt->hzd_addr_cnt *
				   FACTOR_SCALE_MAX / dsu_cnt->l3d_cnt);

	if (dsu_cnt->acp_cnt != 0)
		factor[ACP_HIT_RATE] = (int)(kam_cnt->acp_hit_cnt *
				       FACTOR_SCALE_MAX / dsu_cnt->acp_cnt);

	if (dsu_cnt->l3d_cnt != 0)
		factor[PT_RATE] = (int)(kam_cnt->pt_gen_cnt * FACTOR_SCALE_MAX /
				  dsu_cnt->l3d_cnt);

	if (dsu_cnt->cycle_cnt != 0 && flux->rd_flux != 0)
		factor[DMC_BW] = (int)(flux->rd_flux * FACTOR_SCALE_MAX /
				 dsu_cnt->cycle_cnt);
	else
		factor[DMC_BW] = FACTOR_SCALE_MAX;
}

static void l3p_get_input_factors(struct l3p_karma *l3p)
{
	struct karma_hwmon *karma_hw = l3p->karma_hw;
	struct karma_evt_count *kam_cnt = &karma_hw->count;
	struct dsu_hwmon *dsu_hw = l3p->dsu_hw;
	struct dsu_evt_count *dsu_cnt = &dsu_hw->count;
	int *factor = l3p->neural_data;
	unsigned long dsu_molecule, dsu_denominator;
	bool cpu_l3_total = false;
	bool cpu_l3_miss_total = false;
	int i;

	(void)memset_s(factor, (sizeof(factor[0]) * LEN_MAX), 0,
			(sizeof(factor[0]) * LEN_MAX));
	cpu_l3_total = dsu_cnt->l3d_cnt > dsu_cnt->acp_cnt;
	cpu_l3_miss_total = dsu_cnt->l3d_refill_cnt +
		kam_cnt->acp_hit_cnt > dsu_cnt->acp_cnt;
	dsu_molecule = dsu_cnt->l3d_refill_cnt +
		kam_cnt->acp_hit_cnt - dsu_cnt->acp_cnt;
	dsu_denominator = dsu_cnt->l3d_cnt - dsu_cnt->acp_cnt;

	if ((cpu_l3_total) && (cpu_l3_miss_total) &&
			(dsu_molecule < dsu_denominator)) {
		factor[DSU_HIT_RATE] = (int)(FACTOR_SCALE_MAX -
					     (dsu_molecule * FACTOR_SCALE_MAX /
					      dsu_denominator));
	} else {
		factor[DSU_HIT_RATE] = FACTOR_SCALE_MAX;
	}

	if (dsu_cnt->l3d_cnt > dsu_cnt->acp_cnt &&
	    dsu_cnt->cycle_cnt) {
		factor[DSU_BW] = (int)((dsu_cnt->l3d_cnt - dsu_cnt->acp_cnt) *
				       FACTOR_SCALE_MAX / dsu_cnt->cycle_cnt);
	} else {
		factor[DSU_BW] = FACTOR_SCALE_MAX;
	}

	if (kam_cnt->rep_gen_cnt) {
		factor[REP_EFF] = (int)(kam_cnt->rep_gd_cnt * FACTOR_SCALE_MAX /
					kam_cnt->rep_gen_cnt);
	}

	if (kam_cnt->dcp_gen_cnt) {
		factor[DCP_EFF] = (int)(kam_cnt->dcp_gd_cnt * FACTOR_SCALE_MAX /
					kam_cnt->dcp_gen_cnt);
	}

	__l3p_get_input_factors(l3p);

	/* handle abnormal values */
	for (i = 0; i < FACTOR_MAX; i++) {
		if (factor[i] > FACTOR_SCALE_MAX || factor[i] < 0)
			factor[i] = FACTOR_SCALE_MAX;
	}

	trace_l3p_karma_factor_info(factor[DSU_HIT_RATE],
				    factor[DSU_BW],
				    factor[REP_EFF],
				    factor[DCP_EFF],
				    factor[HZD_RATE],
				    factor[ACP_HIT_RATE],
				    factor[PT_RATE],
				    factor[DMC_BW]);
}

static void l3p_neural_factors_calc(struct l3p_karma *l3p,
				    enum profile_mode mode)
{
	int *factor = l3p->neural_data;

	BUILD_BUG_ON(FACTOR_MAX != LEN_0);
	BUILD_BUG_ON(FACTOR_MAX > LEN_MAX);

	l3p_get_input_factors(l3p);

	neuron_rescale(factor, rescale[mode].mean, rescale[mode].var);
	neuron_hidden(factor, hidden[mode].coeff_0,
		      hidden[mode].bias_0, LEN_0, LEN_1);
	neuron_hidden(factor, hidden[mode].coeff_1,
		      hidden[mode].bias_1, LEN_1, LEN_2);
	neuron_hidden(factor, hidden[mode].coeff_2,
		      hidden[mode].bias_2, LEN_2, LEN_3);
	neuron_output(factor, hidden[mode].coeff_3,
		      hidden[mode].bias_3, LEN_3, LEN_4);
}

static unsigned long l3p_df_calc_next_freq(struct l3p_karma *l3p)
{
	unsigned long freq = l3p->cur_freq;
	enum profile_mode mode;
	int karma_boost;
	int ddr_bw_boost;

	mode = freq_to_mode(freq);
	if (mode >= MAX_MODE) {
		pr_err("l3p_karma: invalid mode = %u\n", mode);
		freq = l3p->freq_min;
		return freq;
	}

	l3p_neural_factors_calc(l3p, mode);
	karma_boost = l3p->neural_data[0];
	ddr_bw_boost = l3p->neural_data[1];

	switch (mode) {
	case SLEEP_MODE:
		/*
		 * boost > 0, sleep -> en_tmp
		 */
		if (karma_boost > 0)
			mode = EN_TMP_MODE;
		break;
	case EN_TMP_MODE:
		/*
		 * boost > 0, en_tmp -> enable
		 */
		if (karma_boost > 0)
			mode = ENABLE_MODE;
		else
			mode = SLEEP_MODE;
		break;
	case ENABLE_MODE:
		/*
		 * boost <= 0, enable -> sleep
		 */
		if (karma_boost <= 0)
			mode = SLEEP_MODE;
		break;
	default:
		pr_err("l3p_karma: profile mode is not valid, mode = %d\n",
		       mode);
		mode = SLEEP_MODE;
		break;
	}

	freq = mode_to_freq(mode);

	trace_l3p_karma_freq_trans(karma_boost, ddr_bw_boost,
				   l3p->cur_freq, freq);

	return freq;
}

static int l3p_gov_get_target(struct devfreq *df, unsigned long *freq)
{
	struct l3p_karma *l3p = dev_get_drvdata(df->dev.parent);
#ifdef CPU_HWMON_DEBUG
	struct cpu_hwmon *cpu_hw = l3p->cpu_hw;
#endif
	struct karma_hwmon *karma_hw = l3p->karma_hw;
	struct dsu_hwmon *dsu_hw = l3p->dsu_hw;
	struct ddr_flux *flux = &l3p->flux;
	unsigned int const usec = ktime_to_us(ktime_get());
	unsigned int delta;
	int ret;

	ret = devfreq_update_stats(df);
	if (ret != 0)
		return ret;

	delta = usec - l3p->time.last_update;
	l3p->time.last_update = usec;
	l3p->time.usec_delta = delta;

	if (!mutex_trylock(&l3p->mon_mutex_lock))
		return 0;

	if (!l3p->mon_started) {
		mutex_unlock(&l3p->mon_mutex_lock);
		*freq = l3p->freq_min;
		return 0;
	}

	/* cpu pmu counter for debug */
#ifdef CPU_HWMON_DEBUG
	cpu_get_cnt(cpu_hw);
#endif
	karma_get_cnt(karma_hw);
	dsu_get_cnt(dsu_hw);
	mutex_unlock(&l3p->mon_mutex_lock);

#ifdef CPU_HWMON_DEBUG
	trace_printk("total_inst=%lu, total_cyc=%lu\n",
		     cpu_hw->total_inst, cpu_hw->total_cyc);
#endif

	flux->rd_flux = 0;
	flux->wr_flux = 0;
#ifdef CONFIG_PERF_CTRL
	/* for ddr's dmc counter */
	ret = get_ddrc_flux_all_ch(flux, DDR_PERFDATA_KARMA);
	if (ret != 0) {
		pr_err("l3p_karma get ddrc flux err\n");
		flux->rd_flux = 0;
	}
#endif

	*freq = l3p_df_calc_next_freq(l3p);

	if (*freq < l3p->freq_min)
		*freq = l3p->freq_min;
	else if (*freq > l3p->freq_max)
		*freq = l3p->freq_max;

	return 0;
}

static int l3p_gov_start(struct devfreq *df)
{
	struct l3p_karma *l3p = dev_get_drvdata(df->dev.parent);
	int ret;

	mutex_lock(&l3p->mon_mutex_lock);
	l3p->monitor_enable = 1;
	ret = l3p_start_monitor(df);
	if (ret != 0) {
		l3p->monitor_enable = 0;
		goto err_start;
	}

err_start:
	mutex_unlock(&l3p->mon_mutex_lock);
	return ret;
}

static void l3p_gov_stop(struct devfreq *df)
{
	struct l3p_karma *l3p = df->data;

	mutex_lock(&l3p->mon_mutex_lock);
	l3p_stop_monitor(df);
	l3p->monitor_enable = 0;
	mutex_unlock(&l3p->mon_mutex_lock);
}

#define MIN_MS		    10U
#define MAX_MS		    1000U
static int l3p_gov_ev_handler(struct devfreq *devfreq,
			      unsigned int event, void *data)
{
	int ret = 0;
	unsigned int sample_ms;

	switch (event) {
	case DEVFREQ_GOV_START:
		ret = l3p_gov_start(devfreq);
		if (ret)
			return ret;
		dev_dbg(devfreq->dev.parent,
			"started L3 karma governor\n");
		break;

	case DEVFREQ_GOV_STOP:
		l3p_gov_stop(devfreq);
		dev_dbg(devfreq->dev.parent,
			"stopped L3 karma governor\n");
		break;

	case DEVFREQ_GOV_SUSPEND:
		devfreq_monitor_suspend(devfreq);
		break;

	case DEVFREQ_GOV_RESUME:
		devfreq_monitor_resume(devfreq);
		break;

	case DEVFREQ_GOV_INTERVAL:
		sample_ms = *(unsigned int *)data;
		sample_ms = max(MIN_MS, sample_ms);
		sample_ms = min(MAX_MS, sample_ms);

		mutex_lock(&devfreq->lock);
		devfreq->profile->polling_ms = sample_ms;
		dev_dbg(devfreq->dev.parent,
			"interval polling_ms=%u\n",
			devfreq->profile->polling_ms);
		mutex_unlock(&devfreq->lock);
		break;

	default:
		break;
	}

	return ret;
}

static struct devfreq_governor l3p_karma_governor = {
	.name		 = L3P_KARMA_GOVERNOR_NAME,
	.immutable = 1,
	.get_target_freq = l3p_gov_get_target,
	.event_handler	 = l3p_gov_ev_handler,
};

static void l3p_karma_handle_update(struct work_struct *work)
{
	struct l3p_karma *l3p = container_of(work, struct l3p_karma,
					     work.work);

	mutex_lock(&l3p->devfreq->lock);
	(void)update_devfreq(l3p->devfreq);
	mutex_unlock(&l3p->devfreq->lock);
}

static int l3p_init_update_time(struct device *dev)
{
	struct l3p_karma *l3p = dev_get_drvdata(dev);

	/* Clean the time statistics and start from scrach */
	l3p->time.usec_delta = 0;
	l3p->time.last_update = ktime_to_us(ktime_get());

	return 0;
}

static int l3p_setup_devfreq_profile(struct platform_device *pdev)
{
	struct l3p_karma *l3p = platform_get_drvdata(pdev);
	struct devfreq_dev_profile *df_profile = NULL;

	l3p->devfreq_profile = devm_kzalloc(&pdev->dev,
					    sizeof(struct devfreq_dev_profile),
					    GFP_KERNEL);
	if (IS_ERR_OR_NULL(l3p->devfreq_profile)) {
		dev_err(&pdev->dev, "no memory\n");
		return PTR_ERR(l3p->devfreq_profile);
	}

	df_profile = l3p->devfreq_profile;

	df_profile->target = l3p_karma_target;
	df_profile->get_dev_status = l3p_df_get_dev_status;
	df_profile->freq_table = l3p->freq_table;
	df_profile->max_state = l3p->table_len;
	df_profile->polling_ms = l3p->polling_ms;
	df_profile->initial_freq = l3p->initial_freq;

	return 0;
}

static int l3p_parse_dt_profile(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct l3p_karma *l3p = platform_get_drvdata(pdev);
	struct device_node *node = pdev->dev.of_node;
	struct device_node *parent = NULL;
	struct device_node *profile = NULL;
	struct device_node *opp = NULL;
	struct profile_data *profile_table = NULL;
	struct set_reg *opp_entry = NULL;
	int table_len;
	int opp_count;
	u32 mask;
	int ret = 0;

	of_node_get(node);

	parent = of_get_child_by_name(node, "profile");
	if (parent == NULL) {
		dev_err(dev, "Failed to find profile node\n");
		ret = -EINVAL;
		goto err_out;
	}

	table_len = of_get_child_count(parent);
	if (table_len == 0) {
		dev_err(dev, "no any profile child node\n");
		ret = -EINVAL;
		goto err_out;
	}
	dev_info(dev, "table_len = %d\n", table_len);

	profile_table = devm_kzalloc(dev, sizeof(struct profile_data) *
				     table_len, GFP_KERNEL);
	if (IS_ERR_OR_NULL(profile_table)) {
		dev_err(dev, "Failed to allocate for karma profile_table\n");
		ret = -ENOMEM;
		goto err_out;
	}
	l3p->profile_table = profile_table;
	l3p->table_len = table_len;
	if (table_len != MAX_MODE) {
		dev_err(dev, "table_len is not equal to MAX_MODE\n");
		goto err_out;
	}

	for_each_child_of_node(parent, profile) {
		opp_count = of_get_child_count(profile);
		if (opp_count == 0) {
			dev_err(dev, "profile name:%s has no any opp child\n",
				profile->name);
			ret = -EINVAL;
			goto err_out;
		}
		dev_info(dev, "profile name:%s opp_count = %d\n",
			 profile->name, opp_count);
		opp_entry = devm_kzalloc(dev, sizeof(struct set_reg) *
					 opp_count, GFP_KERNEL);
		if (IS_ERR_OR_NULL(opp_entry)) {
			dev_err(dev, "Failed to allocate for karma opp_entry\n");
			ret =  -ENOMEM;
			goto err_out;
		}
		profile_table->opp_entry = opp_entry;
		profile_table->opp_count = opp_count;

		for_each_child_of_node(profile, opp) {
			dev_info(dev, "opp name:%s\n", opp->name);
			ret = of_property_read_u32(opp, "offset",
						   &opp_entry->offset);
			if (ret != 0) {
				dev_err(dev, "parse %s offset failed%d\n",
					opp->name, ret);
				goto err_out;
			}
			dev_info(dev, "offset = 0x%x\n", opp_entry->offset);

			ret = of_property_read_u32(opp, "mask",  &mask);
			if (ret != 0) {
				dev_err(dev, "parse %s mask failed%d\n",
					opp->name, ret);
				goto err_out;
			}
			opp_entry->shift = ffs(mask) - 1;
			opp_entry->width = fls(mask) - ffs(mask) + 1;
			dev_info(dev, "shift = 0x%x, width = 0x%x\n",
				 opp_entry->shift, opp_entry->width);

			ret = of_property_read_u32(opp, "value",
						   &opp_entry->value);
			if (ret != 0) {
				dev_err(dev, "parse %s value failed%d\n",
					opp->name, ret);
				goto err_out;
			}
			dev_info(dev, "value = 0x%x\n", opp_entry->value);

			opp_entry++;
		}
		profile_table++;
	}

err_out:
	of_node_put(node);
	return ret;
}

static int l3p_parse_dt_base(struct platform_device *pdev)
{
	struct l3p_karma *l3p = platform_get_drvdata(pdev);
	struct device_node *node = pdev->dev.of_node;
	int ret;

	of_node_get(node);
	ret = of_property_read_u32(node, "polling", &l3p->polling_ms);
	if (ret != 0)
		l3p->polling_ms = L3P_KARMA_DEFAULT_POLLING_MS;
	dev_info(&pdev->dev, "polling_ms = %d\n", l3p->polling_ms);

	of_node_put(node);
	return 0;
}

#ifdef CONFIG_L3P_KARMA_DEBUG
void l3p_karma_show_registers(struct l3p_karma *l3p)
{
	int i;
	u32 value;

	for (i = 0; i < KARMA_SYS_CTRL_NUM; i++) {
		value = readl(l3p->base + i * 4);
		pr_err("l3p_karma: offset = 0x%x, value = 0x%x\n", i * 4, value);
	}
}
#endif

static int l3p_karma_setup(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct l3p_karma *l3p = platform_get_drvdata(pdev);
	int i;
	int ret;

	l3p->base = of_iomap(dev->of_node, 0);
	if (l3p->base == NULL) {
		dev_err(dev, "l3p karma ctrl baseaddr iomap failed\n");
		return -ENOMEM;
	}

#ifdef CONFIG_L3P_KARMA_DEBUG
	l3p->attr_grp = &dev_attr_group;
#endif
	l3p->monitor_enable = 0;
	l3p->vote_mon_dis = 0;
	l3p->mon_started = false;
	mutex_init(&l3p->mon_mutex_lock);
	spin_lock_init(&l3p->data_spinlock);

	l3p->update_wq = create_freezable_workqueue("hisi_l3p_karma_wq");
	if (IS_ERR_OR_NULL(l3p->update_wq)) {
		dev_err(&pdev->dev, "cannot create workqueue\n");
		ret = PTR_ERR(l3p->update_wq);
		goto out;
	}
	INIT_DEFERRABLE_WORK(&l3p->work, l3p_karma_handle_update);

	l3p->work_ms = WORK_MS_INIT; /* 4000us */
	l3p->quick_sleep_enable = 1;

	l3p->freq_table = devm_kcalloc(dev, l3p->table_len,
				       sizeof(*l3p->freq_table),
				       GFP_KERNEL);
	if (IS_ERR_OR_NULL(l3p->freq_table)) {
		dev_err(dev, "failed to alloc mem for freq_table\n");
		ret = -ENOMEM;
		goto er_wq;
	}

	/*
	 * freq's value range is in [1,3] discretely
	 */
	for (i = 0; i < l3p->table_len; i++) {
		l3p->freq_table[i] = i + 1;
		dev_info(dev, "i = %d, freq_table = %lu\n",
			 i, l3p->freq_table[i]);
	}

	l3p->freq_min = l3p->freq_table[0];
	l3p->freq_max = l3p->freq_table[l3p->table_len - 1];

	l3p->initial_freq = l3p->freq_max;
	l3p->cur_freq = l3p->initial_freq;

	dev_info(dev, "cur_freq = %lu\n",  l3p->cur_freq);

	ret = l3p_setup_devfreq_profile(pdev);
	if (ret != 0) {
		dev_err(dev, "device setup devfreq profile failed.\n");
		goto er_wq;
	}

	l3p->devfreq = devm_devfreq_add_device(dev,
					       l3p->devfreq_profile,
					       L3P_KARMA_GOVERNOR_NAME, l3p);

	if (IS_ERR_OR_NULL(l3p->devfreq)) {
		dev_err(dev, "registering to devfreq failed\n");
		ret = PTR_ERR(l3p->devfreq);
		goto er_wq;
	}

	l3p_init_update_time(dev);

	mutex_lock(&l3p->devfreq->lock);
	l3p->devfreq->min_freq = l3p->freq_min;
	l3p->devfreq->max_freq = l3p->freq_max;
	dev_info(dev, "min_freq = %lu, max_freq = %lu\n",
		 l3p->devfreq->min_freq, l3p->devfreq->max_freq);
	mutex_unlock(&l3p->devfreq->lock);

	spin_lock(&l3p->data_spinlock);
	ret = l3p_karma_init(pdev);
	if (ret != 0) {
		dev_err(dev, "karma device init failed\n");
		spin_unlock(&l3p->data_spinlock);
		goto err_devfreq;
	}
	l3p_karma_enable(l3p);
#ifdef CONFIG_L3P_KARMA_DEBUG
	l3p_karma_show_registers(l3p);
#endif
	spin_unlock(&l3p->data_spinlock);

	return 0;

err_devfreq:
	devm_devfreq_remove_device(dev, l3p->devfreq);
er_wq:
	/* Wait for pending work */
	flush_workqueue(l3p->update_wq);
	/* Destroy workqueue */
	destroy_workqueue(l3p->update_wq);
out:
	iounmap(l3p->base);
	return ret;
}

static int l3p_fcm_idle_notif(struct notifier_block *nb,
			      unsigned long action, void *data)
{
	bool fcm_pwrdn = true;
	struct l3p_karma *l3p = container_of(nb, struct l3p_karma, idle_nb);

	/*
	 * device hasn't been initilzed yet,
	 * mon_started is necessary, otherwise although monitor_enable is true,
	 * but thread is sleep in l3p_start_monitor, so fcm can enter idle.
	 * However, karma is not enabled, reset value is saved.
	 */
	if (l3p->monitor_enable == 0  || !l3p->mon_started)
		return NOTIFY_OK;

	/* In fact, karma maybe not power off */
	switch (action) {
	case CPU_PM_ENTER:
		spin_lock(&l3p->data_spinlock);
		fcm_pwrdn = lpcpu_fcm_cluster_pwrdn();
		if (fcm_pwrdn) {
			l3p_save_reg_data(l3p);
			l3p->fcm_idle = true;
		}
		spin_unlock(&l3p->data_spinlock);
		break;

	case CPU_PM_ENTER_FAILED:
	case CPU_PM_EXIT:
		spin_lock(&l3p->data_spinlock);
		if (l3p->fcm_idle) {
			l3p->fcm_idle = false;
			l3p_restore_reg_data(l3p);
		}
		spin_unlock(&l3p->data_spinlock);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static int l3p_karma_suspend(struct device *dev)
{
	struct l3p_karma *l3p = dev_get_drvdata(dev);
	struct devfreq *devfreq = l3p->devfreq;

	mutex_lock(&l3p->mon_mutex_lock);
	/* save all karma contrl register */
	spin_lock(&l3p->data_spinlock);
	if (l3p->monitor_enable != 0)
		l3p_save_reg_data(l3p);
	spin_unlock(&l3p->data_spinlock);

	l3p_stop_monitor(devfreq);
	mutex_unlock(&l3p->mon_mutex_lock);

	return 0;
}

static int l3p_karma_resume(struct device *dev)
{
	struct l3p_karma *l3p = dev_get_drvdata(dev);
	struct devfreq *devfreq = l3p->devfreq;
	int ret;

	l3p_init_update_time(dev);

	mutex_lock(&l3p->mon_mutex_lock);
	/* restore all karma contrl register */
	spin_lock(&l3p->data_spinlock);
	if (l3p->monitor_enable != 0)
		l3p_restore_reg_data(l3p);
	spin_unlock(&l3p->data_spinlock);

	ret = l3p_start_monitor(devfreq);
	if (ret != 0)
		l3p->monitor_enable = 0;
	mutex_unlock(&l3p->mon_mutex_lock);

	return ret;
}

static SIMPLE_DEV_PM_OPS(l3p_karma_pm, l3p_karma_suspend, l3p_karma_resume);

static int l3p_karma_acp_callback(struct notifier_block *nb,
				  unsigned long mode, void *data)
{
	struct l3p_karma *l3p = container_of(nb, struct l3p_karma,
					     acp_notify);
	struct devfreq *df = l3p->devfreq;
	int ret;

	switch (mode) {
	case L3SHARE_ACP_ENABLE:
		mutex_lock(&l3p->mon_mutex_lock);
		if (l3p->monitor_enable == 1) {
			/*
			 * NOTICE
			 * this thread can not sleep before save register data,
			 * if get mon_mutex_lock already
			 */
			spin_lock(&l3p->data_spinlock);
			l3p_karma_disable(l3p);
			l3p_save_reg_data(l3p);
			spin_unlock(&l3p->data_spinlock);
			l3p_stop_monitor(df);
			l3p->monitor_enable = 0;
			trace_l3p_karma_acp_callback("stop monitor and disable karma", mode);
		}
		l3p->vote_mon_dis |= BIT(VOTE_SHARE);
		mutex_unlock(&l3p->mon_mutex_lock);
		break;
	case L3SHARE_ACP_DISABLE:
		mutex_lock(&l3p->mon_mutex_lock);
		l3p->vote_mon_dis &= (~BIT(VOTE_SHARE));
		if (l3p->monitor_enable == 0 && l3p->vote_mon_dis == 0) {
			trace_l3p_karma_acp_callback("start monitor and enable karma", mode);
			l3p->monitor_enable = 1;
			ret = l3p_start_monitor(df);
			if (ret != 0) {
				/* handle error */
				l3p->monitor_enable = 0;
			} else {
				/* enable karma */
				spin_lock(&l3p->data_spinlock);
				l3p_restore_reg_data(l3p);
				l3p_karma_enable(l3p);
				spin_unlock(&l3p->data_spinlock);
			}
		}
		mutex_unlock(&l3p->mon_mutex_lock);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static int l3p_karma_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct l3p_karma *l3p = NULL;
	int ret;

	l3p = devm_kzalloc(dev, sizeof(*l3p), GFP_KERNEL);
	if (l3p == NULL)
		return -ENOMEM;

	dev_set_name(dev, "l3p_karma");
	platform_set_drvdata(pdev, l3p);
	l3p->pdev = pdev;

	ret = l3p_parse_dt_base(pdev);
	if (ret != 0) {
		dev_err(dev, "parse dt base failed\n");
		goto err;
	}

	ret = l3p_parse_dt_profile(pdev);
	if (ret != 0) {
		dev_err(dev, "parse dt profile failed\n");
		goto err;
	}

#ifdef CPU_HWMON_DEBUG
	ret = cpu_hwmon_setup(pdev);
	if (ret != 0) {
		dev_err(dev, "cpu hwmon setup failed\n");
		goto err;
	}
#endif

	ret = karma_hwmon_setup(pdev);
	if (ret != 0) {
		dev_err(dev, "karma hwmon setup failed\n");
		goto err;
	}

	ret = dsu_hwmon_setup(pdev);
	if (ret != 0) {
		dev_err(dev, "dsu hwmon setup failed\n");
		goto err;
	}

	l3p->idle_nb.notifier_call = l3p_fcm_idle_notif;
	ret = cpu_pm_register_notifier(&l3p->idle_nb);
	if (ret != 0) {
		dev_err(dev, "cpu pm register failed\n");
		goto err;
	}

	ret = l3p_karma_setup(pdev);
	if (ret != 0) {
		dev_err(dev, "setup failed\n");
		goto err_cpu_pm;
	}

#ifdef CONFIG_L3P_KARMA_DEBUG
	ret = sysfs_create_group(&l3p->devfreq->dev.kobj, l3p->attr_grp);
	if (ret != 0) {
		dev_err(dev, "sysfs create failed\n");
		goto err_setup;
	}
#endif

	l3p->acp_notify.notifier_call = l3p_karma_acp_callback;
	ret = register_l3share_acp_notifier(&l3p->acp_notify);
	if (ret != 0) {
		dev_err(&pdev->dev, "register l3p acp notifier failed\n");
		goto err_sysfs;
	}

	dev_err(dev, "register l3p karma exit\n");
	return 0;

err_sysfs:
#ifdef CONFIG_L3P_KARMA_DEBUG
	sysfs_remove_group(&l3p->devfreq->dev.kobj, l3p->attr_grp);
err_setup:
#endif
	devm_devfreq_remove_device(dev, l3p->devfreq);
	/* Wait for pending work */
	flush_workqueue(l3p->update_wq);
	/* Destroy workqueue */
	destroy_workqueue(l3p->update_wq);
	l3p_karma_disable(l3p);
	iounmap(l3p->base);
err_cpu_pm:
	cpu_pm_unregister_notifier(&l3p->idle_nb);
err:
	dev_err(dev, "failed to register driver, err %d\n", ret);
	return ret;
}

static int l3p_karma_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct l3p_karma *l3p = platform_get_drvdata(pdev);

	unregister_l3share_acp_notifier(&l3p->acp_notify);
	/* Wait for pending work */
	flush_workqueue(l3p->update_wq);
	/* Destroy workqueue */
	destroy_workqueue(l3p->update_wq);

	devm_devfreq_remove_device(dev, l3p->devfreq);
	cpu_pm_unregister_notifier(&l3p->idle_nb);

	return 0;
}

MODULE_DEVICE_TABLE(of, l3p_karma_devfreq_id);

static struct platform_driver l3p_karma_driver = {
	.probe	= l3p_karma_probe,
	.remove = l3p_karma_remove,
	.driver = {
		.name = L3P_KARMA_PLATFORM_DEVICE_NAME,
		.of_match_table = l3p_karma_devfreq_id,
		.pm = &l3p_karma_pm,
		.owner = THIS_MODULE,
	},
};

static int __init l3p_karma_devfreq_init(void)
{
	int ret;

	ret = devfreq_add_governor(&l3p_karma_governor);
	if (ret != 0) {
		pr_err("%s: failed to add governor: %d.\n", __func__, ret);
		return ret;
	}

	ret = platform_driver_register(&l3p_karma_driver);
	if (ret != 0) {
		ret = devfreq_remove_governor(&l3p_karma_governor);
		if (ret != 0)
			pr_err("%s: failed to remove governor: %d.\n",
			       __func__, ret);
	}

	return ret;
}

static void __exit l3p_karma_devfreq_exit(void)
{
	int ret;

	ret = devfreq_remove_governor(&l3p_karma_governor);
	if (ret != 0)
		pr_err("%s: failed to remove governor: %d.\n", __func__, ret);

	platform_driver_unregister(&l3p_karma_driver);
}

late_initcall(l3p_karma_devfreq_init)

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("L3Cache Karma DVFS Driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
