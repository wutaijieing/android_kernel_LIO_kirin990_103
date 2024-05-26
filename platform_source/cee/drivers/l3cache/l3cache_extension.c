/*
 * l3cache_extension.c
 *
 * l3cache extension
 *
 * Copyright (c) 2019-2021 Huawei Technologies Co., Ltd.
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

#include <linux/of_platform.h>
#include <linux/of.h>
#include <linux/cpu_pm.h>
#include <linux/ktime.h>
#ifdef CONFIG_L3CACHE_EXTENSION_DSU
#include <linux/arm-smccc.h>
#endif
#include <securec.h>
#include "l3cache_common.h"
#include "mm_lb.h"
#define CREATE_TRACE_POINTS
#include <trace/events/l3cache_extension.h>
#include <platform_include/see/bl31_smc.h>

enum ai_type {
	AI_EXIT = 0,
	AI_ENTRY,
};

#ifdef CONFIG_L3CACHE_EXTENSION_DYNAMIC
struct algo_type {
	unsigned long miss_bw;
	unsigned int l3_hit_rate;
	unsigned int lb_hit_rate;
	unsigned long last_update;
};

struct level_type {
	unsigned long long miss_bw;
	unsigned int l3_hit_rate_top;
	unsigned int l3_hit_rate_btm;
	unsigned int lb_hit_rate_top;
	unsigned int lb_hit_rate_btm;
	bool enable;
};
#endif

struct l3extension {
	struct platform_device *pdev;
	bool enable_flag;
	/* to protect extension enable flag */
	spinlock_t enable_lock;
	bool mode_flag;
#ifdef CONFIG_L3CACHE_EXTENSION_DYNAMIC
#ifdef CONFIG_L3CACHE_EXTENSION_DEBUG
	/*
	 * keep_debug = -1, free run;
	 * keep_debug =  0, keep disable;
	 * keep_debug =  1, keep enable;
	 */
	int keep_debug;
#endif
	struct delayed_work work;
	unsigned int work_ms;
	unsigned int up_interval;
	unsigned int down_interval;
	void __iomem *base_addr;
	void __iomem *hit_base;
	void __iomem *miss_base;
	struct algo_type algo_data;
	struct level_type *level_table;
	int level_num;
#endif
#ifdef CONFIG_L3CACHE_EXTENSION_DSU
	bool fcm_idle;
	/* to protect operation of fcm idle race */
	spinlock_t data_lock;
	struct notifier_block idle_nb;
#endif
};

#ifdef CONFIG_L3CACHE_EXTENSION_DYNAMIC
bool g_extension_enable_status;
bool g_dpc_extension_flag = true;
DEFINE_SPINLOCK(g_l3extension_lock);
#endif

#define L3EXTENSION_ATTR_RW(_name) \
	static DEVICE_ATTR(_name, 0660, show_##_name, store_##_name)

#ifdef CONFIG_L3CACHE_EXTENSION_DYNAMIC
static void clear_hit_miss_cnt(struct l3extension *l3e)
{
	writeq(0, l3e->hit_base);
	writeq(0, l3e->miss_base);
}

static void read_hit_miss_cnt(struct l3extension *l3e,
			      unsigned long *hit_cnt,
			      unsigned long *miss_cnt)
{
	*hit_cnt = readq(l3e->hit_base);
	*miss_cnt = readq(l3e->miss_base);
}

#ifdef CONFIG_L3CACHE_EXTENSION_DEBUG
#define POLLING_TIME_MIN		10
#define POLLING_TIME_MAX		200

/* work_ms */
static ssize_t store_work_ms(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct l3extension *l3e = dev_get_drvdata(dev);
	unsigned int val = 0;
	int ret;

	ret = kstrtouint(buf, 10, &val);
	if (ret != 0 || val > POLLING_TIME_MAX || val < POLLING_TIME_MIN) {
		dev_err(dev, "invalid, val = %u, ret = %d\n", val, ret);
		return -EINVAL;
	}

	cancel_delayed_work_sync(&l3e->work);
	spin_lock(&l3e->enable_lock);
	l3e->work_ms = val;
	queue_delayed_work(system_power_efficient_wq, &l3e->work,
			   msecs_to_jiffies(l3e->work_ms));
	spin_unlock(&l3e->enable_lock);

	return count;
}

static ssize_t show_work_ms(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	struct l3extension *l3e = dev_get_drvdata(dev);
	int ret;

	spin_lock(&l3e->enable_lock);
	ret = scnprintf(buf, PAGE_SIZE, "work_ms = %u\n", l3e->work_ms);
	spin_unlock(&l3e->enable_lock);

	return ret;
}

/* level_data */
#define LOCAL_BUF_MAX       128
#define ARGV_NUM	    3
enum algo_character {
	ALGO_MISS_BW = 0,
	ALGO_L3_HIT_RATE_TOP,
	ALGO_L3_HIT_RATE_BTM,
	ALGO_LB_HIT_RATE_TOP,
	ALGO_LB_HIT_RATE_BTM,
	ALGO_MAX_NUM,
};

static int cmd_parse(char *para_cmd, char *argv[], int max_args)
{
	int para_id = 0;

	while (*para_cmd != '\0') {
		if (para_id >= max_args)
			break;

		while (*para_cmd == ' ')
			para_cmd++;

		if (*para_cmd == '\0')
			break;

		argv[para_id] = para_cmd;
		para_id++;

		while ((*para_cmd != ' ') && (*para_cmd != '\0'))
			para_cmd++;

		if (*para_cmd == '\0')
			break;

		*para_cmd = '\0';
		para_cmd++;
	}

	return para_id;
}

static int update_level_data(struct l3extension *l3e, char *argv[])
{
	struct device *dev = &l3e->pdev->dev;
	struct level_type *lvl_data = l3e->level_table;
	unsigned int lvl, entry, value;
	int ret;

	ret = sscanf_s(argv[0], "%u", &lvl);
	if (ret != 1 || lvl >= (unsigned int)l3e->level_num) {
		dev_err(dev, "lvl error %s, lvl = %u\n", argv[0], lvl);
		ret = -EINVAL;
		return ret;
	}

	ret = sscanf_s(argv[1], "%u", &entry);
	if (ret != 1) {
		dev_err(dev, "entry error %s\n", argv[1]);
		ret = -EINVAL;
		return ret;
	}

	ret = sscanf_s(argv[2], "%u", &value);
	if (ret != 1) {
		dev_err(dev, "value error %s\n", argv[2]);
		ret = -EINVAL;
		return ret;
	}

	cancel_delayed_work_sync(&l3e->work);
	spin_lock(&l3e->enable_lock);
	switch (entry) {
	case ALGO_MISS_BW:
		lvl_data[lvl].miss_bw = value;
		break;
	case ALGO_L3_HIT_RATE_TOP:
		lvl_data[lvl].l3_hit_rate_top = value;
		break;
	case ALGO_L3_HIT_RATE_BTM:
		lvl_data[lvl].l3_hit_rate_btm = value;
		break;
	case ALGO_LB_HIT_RATE_TOP:
		lvl_data[lvl].lb_hit_rate_top = value;
		break;
	case ALGO_LB_HIT_RATE_BTM:
		lvl_data[lvl].lb_hit_rate_btm = value;
		break;
	default:
		dev_err(dev, "invalid entry %u\n", entry);
		break;
	}
	queue_delayed_work(system_power_efficient_wq, &l3e->work,
			   msecs_to_jiffies(l3e->work_ms));
	spin_unlock(&l3e->enable_lock);

	return 0;
}

static ssize_t store_level_data(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct l3extension *l3e = dev_get_drvdata(dev);
	char local_buf[LOCAL_BUF_MAX] = {0};
	char *argv[ARGV_NUM] = {0};
	int argc, ret;

	if (count >= sizeof(local_buf))
		return -ENOMEM;

	ret = strncpy_s(local_buf,
			LOCAL_BUF_MAX,
			buf,
			min_t(size_t, sizeof(local_buf) - 1, count));
	if (ret != EOK) {
		dev_err(dev, "copy string to local_buf failed\n");
		ret = -EINVAL;
		goto err_handle;
	}

	argc = cmd_parse(local_buf, argv, ARRAY_SIZE(argv));
	if (argc != ARGV_NUM) {
		dev_err(dev, "error, para num invalid, argc = %d\n", argc);
		ret = -EINVAL;
		goto err_handle;
	}

	ret = update_level_data(l3e, argv);
	if (ret == 0)
		ret = (int)count;

err_handle:
	return ret;
}

static ssize_t show_level_data(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct l3extension *l3e = dev_get_drvdata(dev);
	struct level_type *lvl_data = l3e->level_table;
	ssize_t count = 0;
	int ret;
	int i;

	for (i = 0; i < l3e->level_num; i++) {
		ret = snprintf_s(buf + count,
				 (PAGE_SIZE - count),
				 (PAGE_SIZE - count - 1),
				 "level-%d:\n",
				 i);
		if (ret < 0)
			goto err;
		count += ret;
		if ((unsigned int)count >= PAGE_SIZE)
			goto err;
		ret = snprintf_s(buf + count,
				 (PAGE_SIZE - count),
				 (PAGE_SIZE - count - 1),
				 "miss_bw = %lu, l3_top = %u, l3_btm = %u, lb_top = %u, lb_btm = %u\n",
				 lvl_data[i].miss_bw,
				 lvl_data[i].l3_hit_rate_top,
				 lvl_data[i].l3_hit_rate_btm,
				 lvl_data[i].lb_hit_rate_top,
				 lvl_data[i].lb_hit_rate_btm);
		if (ret < 0)
			goto err;
		count += ret;
		if ((unsigned int)count >= PAGE_SIZE)
			goto err;
	}
err:
	return count;
}

#define MAX_INTERVAL	20
/* up_interval */
static ssize_t store_up_interval(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf,
				 size_t count)
{
	struct l3extension *l3e = dev_get_drvdata(dev);
	unsigned int val = 0;
	int ret;

	ret = kstrtouint(buf, 10, &val);
	if (ret != 0) {
		dev_err(dev, "input data is not valid, ret = %d\n", ret);
		return -EINVAL;
	}

	spin_lock(&l3e->enable_lock);
	if (val <= MAX_INTERVAL)
		l3e->up_interval = val;
	spin_unlock(&l3e->enable_lock);

	return count;
}

static ssize_t show_up_interval(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	struct l3extension *l3e = dev_get_drvdata(dev);
	ssize_t ret;

	spin_lock(&l3e->enable_lock);
	ret = scnprintf(buf,
			PAGE_SIZE,
			"up_interval = %u\n",
			l3e->up_interval);
	spin_unlock(&l3e->enable_lock);

	return ret;
}

/* down_interval */
static ssize_t store_down_interval(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf,
				   size_t count)
{
	struct l3extension *l3e = dev_get_drvdata(dev);
	unsigned int val = 0;
	int ret;

	ret = kstrtouint(buf, 10, &val);
	if (ret != 0) {
		dev_err(dev, "input data is not valid, ret = %d\n", ret);
		return -EINVAL;
	}

	spin_lock(&l3e->enable_lock);
	if (val <= MAX_INTERVAL)
		l3e->down_interval = val;
	spin_unlock(&l3e->enable_lock);

	return count;
}

static ssize_t show_down_interval(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	struct l3extension *l3e = dev_get_drvdata(dev);
	ssize_t ret;

	spin_lock(&l3e->enable_lock);
	ret = scnprintf(buf,
			PAGE_SIZE,
			"down_interval = %u\n",
			l3e->down_interval);
	spin_unlock(&l3e->enable_lock);

	return ret;
}

/* keep_debug */
static ssize_t store_keep_debug(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct l3extension *l3e = dev_get_drvdata(dev);
	int val = 0;
	int ret;

	ret = kstrtoint(buf, 10, &val);
	if (ret != 0) {
		dev_err(dev, "input data is not valid, val = %d\n", val);
		return -EINVAL;
	}

	spin_lock(&l3e->enable_lock);
	l3e->keep_debug = val;
	spin_unlock(&l3e->enable_lock);

	return count;
}

static ssize_t show_keep_debug(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct l3extension *l3e = dev_get_drvdata(dev);
	ssize_t ret;

	spin_lock(&l3e->enable_lock);
	ret = scnprintf(buf, PAGE_SIZE, "keep_debug = %d enable_flag = %d\n",
			l3e->keep_debug, l3e->enable_flag);
	spin_unlock(&l3e->enable_lock);

	return ret;
}

L3EXTENSION_ATTR_RW(work_ms);
L3EXTENSION_ATTR_RW(level_data);
L3EXTENSION_ATTR_RW(up_interval);
L3EXTENSION_ATTR_RW(down_interval);
L3EXTENSION_ATTR_RW(keep_debug);
#endif
#endif

/* mode */
static ssize_t store_mode(struct device *dev,
			  struct device_attribute *attr,
			  const char *buf, size_t count)
{
	struct l3extension *l3e = dev_get_drvdata(dev);
	int val = 0;
	int ret;

	ret = kstrtoint(buf, 10, &val);
	if (ret != 0) {
		dev_err(dev, "mode is not valid, val = %d\n", val);
		return -EINVAL;
	}
	val = !!val;

	switch (val) {
	case AI_ENTRY:
#ifdef CONFIG_L3CACHE_EXTENSION_DYNAMIC
		/* cancel work, enable forcedly */
		cancel_delayed_work_sync(&l3e->work);
#endif
		spin_lock(&l3e->enable_lock);
		if (!l3e->mode_flag) {
			l3e->mode_flag = true;
			l3e->enable_flag = true;
			lb_gid_enable(0);
			trace_l3extension_switch("enable forcedly");
		}
		spin_unlock(&l3e->enable_lock);
		break;
	case AI_EXIT:
		/* disable forcedly, restart work */
		spin_lock(&l3e->enable_lock);
		if (l3e->mode_flag) {
			l3e->mode_flag = false;
#ifdef CONFIG_L3CACHE_EXTENSION_DYNAMIC
			l3e->enable_flag = false;
			lb_gid_bypass(0);
			clear_hit_miss_cnt(l3e);
			queue_delayed_work(system_power_efficient_wq, &l3e->work,
					   msecs_to_jiffies(l3e->work_ms));
#endif
			trace_l3extension_switch("disable forcedly");
		}
		spin_unlock(&l3e->enable_lock);
		break;
	default:
		pr_err("invalid mode value, val = %d\n", val);
		return -EINVAL;
	}

	return count;
}

static ssize_t show_mode(struct device *dev,
			 struct device_attribute *attr, char *buf)
{
	struct l3extension *l3e = dev_get_drvdata(dev);
	ssize_t ret;

	spin_lock(&l3e->enable_lock);
	ret = scnprintf(buf, PAGE_SIZE, "mode = %d\n", l3e->mode_flag);
	spin_unlock(&l3e->enable_lock);

	return ret;
}

L3EXTENSION_ATTR_RW(mode);

static struct attribute *dev_entries[] = {
#ifdef CONFIG_L3CACHE_EXTENSION_DYNAMIC
#ifdef CONFIG_L3CACHE_EXTENSION_DEBUG
	&dev_attr_work_ms.attr,
	&dev_attr_level_data.attr,
	&dev_attr_up_interval.attr,
	&dev_attr_down_interval.attr,
	&dev_attr_keep_debug.attr,
#endif
#endif
	&dev_attr_mode.attr,
	NULL,
};

static struct attribute_group dev_attr_group = {
	.name	= "control",
	.attrs	= dev_entries,
};

#ifdef CONFIG_L3CACHE_EXTENSION_DSU
static void enable_write_evict(void)
{
	struct arm_smccc_res res;

	arm_smccc_1_1_smc((u64)L3EXCLUSIVE_FID_VALUE, (u64)CMD_ENABLE_WE, &res);
}

#define CACHE_EVICT_CONTROL		14
static int check_write_evict_enabled(void)
{
	u64 val = 0;

	asm volatile("mrs %0, s3_0_c15_c3_4" : "=r" (val));
	if ((val & BIT(CACHE_EVICT_CONTROL)) == 0)
		return 0;
	else
		return 1;
}

static int l3ex_fcm_idle_notif(struct notifier_block *nb,
			       unsigned long action, void *data)
{
	bool fcm_pwrdn = true;
	struct l3extension *l3e = container_of(nb,
					       struct l3extension,
					       idle_nb);

	switch (action) {
	case CPU_PM_ENTER:
		spin_lock(&l3e->data_lock);
		fcm_pwrdn = lpcpu_fcm_cluster_pwrdn();
		if (fcm_pwrdn && !l3e->fcm_idle)
			l3e->fcm_idle = true;
		spin_unlock(&l3e->data_lock);
		break;
	case CPU_PM_ENTER_FAILED:
	case CPU_PM_EXIT:
		spin_lock(&l3e->data_lock);
		if (l3e->fcm_idle) {
			l3e->fcm_idle = false;
			spin_lock(&l3e->enable_lock);
			if (check_write_evict_enabled() == 0 &&
			    l3e->enable_flag)
				enable_write_evict();
			spin_unlock(&l3e->enable_lock);
		}
		spin_unlock(&l3e->data_lock);
		break;
	default:
		return NOTIFY_DONE;
	}
	return NOTIFY_OK;
}
#endif

#ifdef CONFIG_L3CACHE_EXTENSION_DYNAMIC
static bool check_level_enable(const struct l3extension *l3e, int lvl_idx)
{
	const struct algo_type *algo = &l3e->algo_data;
	const struct level_type *lvl_data = NULL;
	bool ret = false;

	if (lvl_idx >= l3e->level_num)
		return false;

	lvl_data = &l3e->level_table[lvl_idx];

	if (algo->miss_bw < lvl_data->miss_bw)
		return false;

	if (l3e->enable_flag) {
		if (algo->lb_hit_rate < lvl_data->lb_hit_rate_top &&
		    algo->lb_hit_rate > lvl_data->lb_hit_rate_btm)
			ret = true;
		else
			ret = false;
	} else {
		if (algo->l3_hit_rate < lvl_data->l3_hit_rate_top &&
		    algo->l3_hit_rate > lvl_data->l3_hit_rate_btm)
			ret = true;
		else
			ret = false;
	}

	return ret;
}

static void extension_switch(struct l3extension *l3e)
{
	struct level_type *lvl_data = NULL;
	bool will_enable = false;
	static unsigned int try_en_cnt;
	static unsigned int try_dis_cnt;
	int i;

	for (i = 0; i < l3e->level_num; i++) {
		lvl_data = &l3e->level_table[i];
		lvl_data->enable = check_level_enable(l3e, i);
		will_enable = will_enable || lvl_data->enable;
		if (will_enable)
			break;
	}

	spin_lock(&l3e->enable_lock);
#ifdef CONFIG_L3CACHE_EXTENSION_DEBUG
	if (l3e->keep_debug == 0)
		will_enable = false;
	else if (l3e->keep_debug == 1)
		will_enable = true;
#endif

	spin_lock(&g_l3extension_lock);
	if (g_dpc_extension_flag && will_enable) {
		try_dis_cnt = 0;
		if (!l3e->enable_flag) {
			try_en_cnt++;
			if (try_en_cnt >= l3e->up_interval) {
				trace_l3extension_switch("enable normally");
				l3e->enable_flag = true;
				lb_gid_enable(0);
				try_en_cnt = 0;
			}
		}
	} else if (!will_enable) {
		try_en_cnt = 0;
		if (l3e->enable_flag) {
			try_dis_cnt++;
			if (try_dis_cnt >= l3e->down_interval) {
				trace_l3extension_switch("disable normally");
				l3e->enable_flag = false;
				lb_gid_bypass(0);
				try_dis_cnt = 0;
			}
		}
	} else {
		try_en_cnt = 0;
	}
	g_extension_enable_status = l3e->enable_flag;
	spin_unlock(&g_l3extension_lock);
	spin_unlock(&l3e->enable_lock);
}

static unsigned int get_lb_hit_rate(void)
{
	struct lb_policy_flow flow;
	int ret;

	flow.pid = 0;
	ret = get_flow_stat(&flow);
	if (ret != 0)
		return 0;

	if (flow.in != 0)
		return (unsigned int)(100 - (flow.out * 100UL / flow.in));

	return 0;
}

static void l3ex_handle_update(struct work_struct *work)
{
	struct l3extension *l3e = container_of(work,
						struct l3extension,
						work.work);
	struct algo_type *algo = &l3e->algo_data;
	unsigned long const usec = ktime_to_us((unsigned int)ktime_get());
	unsigned long hit_cnt;
	unsigned long miss_cnt;
	unsigned long total;
	unsigned long delta;

	delta = usec - algo->last_update;
	algo->last_update = usec;

	read_hit_miss_cnt(l3e, &hit_cnt, &miss_cnt);
	clear_hit_miss_cnt(l3e);
	total = hit_cnt + miss_cnt;

	if (total)
		algo->l3_hit_rate = hit_cnt * 100 / total;
	else
		algo->l3_hit_rate = hit_cnt * 100 / MAX_COUNT_VAL;

	if (delta)
		algo->miss_bw = miss_cnt * 64 / delta;
	else
		algo->miss_bw = miss_cnt * 64 / l3e->work_ms;

	if (l3e->enable_flag)
		algo->lb_hit_rate = get_lb_hit_rate();
	else
		algo->lb_hit_rate = 0;

	trace_update_polling_info(algo->miss_bw,
				  algo->lb_hit_rate,
				  algo->l3_hit_rate,
				  delta);

	extension_switch(l3e);

	queue_delayed_work(system_power_efficient_wq, &l3e->work,
			   msecs_to_jiffies(l3e->work_ms));
}

#define OFFSET_REG_NUM	    2
#define REMAP_SIZE	    4096
static int l3extension_dt_base(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = pdev->dev.of_node;
	struct l3extension *l3e = platform_get_drvdata(pdev);
	unsigned int base_reg;
	unsigned int temp[OFFSET_REG_NUM] = {0};
	int ret;

	of_node_get(node);
	ret = of_property_read_u32(node, "work-ms",
				   &l3e->work_ms);
	if (ret != 0) {
		dev_err(dev, "failed to find work-ms node\n");
		ret = -EINVAL;
		goto err;
	}
	dev_dbg(dev, "work_ms = %u\n", l3e->work_ms);

	ret = of_property_read_u32(node, "up-interval",
				   &l3e->up_interval);
	if (ret != 0) {
		dev_err(dev, "failed to find up-interval node\n");
		ret = -EINVAL;
		goto err;
	}
	dev_dbg(dev, "up_interval = %u\n", l3e->up_interval);

	ret = of_property_read_u32(node, "down-interval",
				   &l3e->down_interval);
	if (ret != 0) {
		dev_err(dev, "failed to find down-interval node\n");
		ret = -EINVAL;
		goto err;
	}
	dev_dbg(dev, "down_interval = %u\n", l3e->down_interval);

	ret = of_property_read_u32(node, "base-reg",
				   &base_reg);
	if (ret != 0) {
		dev_err(dev, "failed to find base-reg node\n");
		ret = -EINVAL;
		goto err;
	}
	dev_dbg(dev, "base_reg = 0x%x\n", base_reg);

	ret = of_property_read_u32_array(node, "offset-reg", temp, OFFSET_REG_NUM);
	if (ret != 0) {
		dev_err(dev, "failed to find offset-reg node\n");
		ret = -EINVAL;
		goto err;
	}
	dev_dbg(dev, "hit_offset = 0x%x miss_offset = 0x%x\n", temp[0], temp[1]);

	l3e->base_addr = ioremap(base_reg, REMAP_SIZE);
	if (l3e->base_addr == NULL) {
		dev_err(dev, "failed to ioremap for base_reg\n");
		ret = -ENOMEM;
		goto err;
	}

	l3e->hit_base = l3e->base_addr + temp[0];
	l3e->miss_base = l3e->base_addr + temp[1];

err:
	of_node_put(node);
	return ret;
}

#define HIT_RATE_NUM	2
static int dt_level_parse(struct device *dev,
			  struct device_node *level,
			  struct level_type *level_table)
{
	int ret;
	unsigned int temp[HIT_RATE_NUM] = {0};

	ret = of_property_read_u64(level, "miss-bw", &level_table->miss_bw);
	if (ret != 0) {
		dev_err(dev, "parse %s miss-bw failed %d\n",
			level->name, ret);
		return ret;
	}
	dev_dbg(dev, "miss_bw = %lu\n", level_table->miss_bw);

	ret = of_property_read_u32_array(level, "l3-hit-rate", temp, HIT_RATE_NUM);
	if (ret != 0) {
		dev_err(dev, "parse %s l3-hit-rate failed %d\n",
			level->name, ret);
		return ret;
	}

	level_table->l3_hit_rate_btm = temp[0];
	level_table->l3_hit_rate_top = temp[1];
	dev_dbg(dev, "l3_hit_rate_btm = %u\n", level_table->l3_hit_rate_btm);
	dev_dbg(dev, "l3_hit_rate_top = %u\n", level_table->l3_hit_rate_top);

	ret = of_property_read_u32_array(level, "lb-hit-rate", temp, HIT_RATE_NUM);
	if (ret != 0) {
		dev_err(dev, "parse %s lb-hit-rate failed %d\n",
			level->name, ret);
		return ret;
	}

	level_table->lb_hit_rate_btm = temp[0];
	level_table->lb_hit_rate_top = temp[1];
	dev_dbg(dev, "lb_hit_rate_btm = %u\n", level_table->lb_hit_rate_btm);
	dev_dbg(dev, "lb_hit_rate_top = %u\n", level_table->lb_hit_rate_top);

	return ret;
}

static int l3extension_dt_level(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = pdev->dev.of_node;
	struct l3extension *l3e = platform_get_drvdata(pdev);
	struct device_node *parent = NULL;
	struct device_node *level = NULL;
	struct level_type *level_table = NULL;
	int ret = 0;

	of_node_get(node);
	parent = of_get_child_by_name(node, "level");
	if (parent == NULL) {
		dev_err(dev, "failed to find level node\n");
		ret = -EINVAL;
		goto err;
	}

	l3e->level_num = of_get_child_count(parent);
	if (l3e->level_num <= 0) {
		dev_err(dev, "no any level child node\n");
		ret = -EINVAL;
		goto err;
	}
	dev_dbg(dev, "level_num = %d\n", l3e->level_num);

	level_table = devm_kzalloc(dev,
				   sizeof(struct level_type) * l3e->level_num,
				   GFP_KERNEL);
	if (IS_ERR_OR_NULL(level_table)) {
		dev_err(dev, "failed to allocate for level_table\n");
		ret = -ENOMEM;
		goto err;
	}
	l3e->level_table = level_table;

	for_each_child_of_node(parent, level) {
		dev_dbg(dev, "level name:%s\n", level->name);
		ret = dt_level_parse(dev, level, level_table);
		if (ret != 0)
			goto err;
		level_table++;
	}

err:
	of_node_put(node);
	return ret;
}
#endif

static int l3extension_suspend(struct device *dev)
{
	struct l3extension *l3e = dev_get_drvdata(dev);

	spin_lock(&l3e->enable_lock);
	if (l3e->enable_flag) {
		l3e->enable_flag = false;
		dev_err(dev, "lb_gid_bypass\n");
		lb_gid_bypass(0);
	}
	spin_unlock(&l3e->enable_lock);

	return 0;
}

static int l3extension_resume(struct device *dev)
{
#ifndef CONFIG_L3CACHE_EXTENSION_DYNAMIC
	struct l3extension *l3e = dev_get_drvdata(dev);

	spin_lock(&l3e->enable_lock);
	if (!l3e->enable_flag) {
		l3e->enable_flag = true;
		dev_err(dev, "lb_gid_enable\n");
		lb_gid_enable(0);
	}
	spin_unlock(&l3e->enable_lock);
#endif
	return 0;
}

static SIMPLE_DEV_PM_OPS(l3extension_pm,
			 l3extension_suspend,
			 l3extension_resume);

static const struct of_device_id l3extension_id[] = {
	{.compatible = "hisilicon,l3extension"},
	{}
};

static int l3extension_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct l3extension *l3e = NULL;
	int ret;

	dev_err(dev, "l3extension enter\n");
	if (is_lb_available() == 0) {
		dev_err(dev, "lb is not available\n");
		return -ENODEV;
	}

	l3e = devm_kzalloc(dev, sizeof(struct l3extension), GFP_KERNEL);
	if (l3e == NULL)
		return -ENOMEM;

	dev_set_name(dev, "l3extension");
	platform_set_drvdata(pdev, l3e);
	l3e->pdev = pdev;
	dev_set_drvdata(&pdev->dev, l3e);

#ifdef CONFIG_L3CACHE_EXTENSION_DYNAMIC
	ret = l3extension_dt_base(pdev);
	if (ret != 0) {
		dev_err(&pdev->dev, "parse base dt failed\n");
		goto out;
	}

	ret = l3extension_dt_level(pdev);
	if (ret != 0) {
		dev_err(&pdev->dev, "parse level dt failed\n");
		goto err;
	}

	INIT_DEFERRABLE_WORK(&l3e->work, l3ex_handle_update);
	l3e->enable_flag = false;
#ifdef CONFIG_L3CACHE_EXTENSION_DEBUG
	l3e->keep_debug = -1;
#endif
#endif

	spin_lock_init(&l3e->enable_lock);
	l3e->mode_flag = false;

#ifdef CONFIG_L3CACHE_EXTENSION_DSU
	spin_lock_init(&l3e->data_lock);
	l3e->fcm_idle = false;
	l3e->idle_nb.notifier_call = l3ex_fcm_idle_notif;
	ret = cpu_pm_register_notifier(&l3e->idle_nb);
	if (ret != 0) {
		dev_err(dev, "cpu pm register failed\n");
		goto err;
	}
#endif

	ret = sysfs_create_group(&pdev->dev.kobj, &dev_attr_group);
	if (ret != 0) {
		dev_err(&pdev->dev, "sysfs create err %d\n", ret);
		goto err_sysfs;
	}

#ifdef CONFIG_L3CACHE_EXTENSION_DYNAMIC
	/* at last, start work to poll */
	queue_delayed_work(system_power_efficient_wq, &l3e->work,
			   msecs_to_jiffies(l3e->work_ms));
#else
	spin_lock(&l3e->enable_lock);
	l3e->enable_flag = true;
	lb_gid_enable(0);
	spin_unlock(&l3e->enable_lock);
#endif

	dev_err(dev, "l3extension exit\n");
	return 0;

err_sysfs:
#ifdef CONFIG_L3CACHE_EXTENSION_DSU
	cpu_pm_unregister_notifier(&l3e->idle_nb);
#endif
err:
#ifdef CONFIG_L3CACHE_EXTENSION_DYNAMIC
	iounmap(l3e->base_addr);
out:
#endif
	return ret;
}

static int l3extension_remove(struct platform_device *pdev)
{
	struct l3extension *l3e = platform_get_drvdata(pdev);

	sysfs_remove_group(&pdev->dev.kobj, &dev_attr_group);

#ifdef CONFIG_L3CACHE_EXTENSION_DYNAMIC
	cancel_delayed_work_sync(&l3e->work);
	iounmap(l3e->base_addr);
#endif

	spin_lock(&l3e->enable_lock);
	l3e->enable_flag = false;
	lb_gid_bypass(0);
	spin_unlock(&l3e->enable_lock);

#ifdef CONFIG_L3CACHE_EXTENSION_DSU
	cpu_pm_unregister_notifier(&l3e->idle_nb);
#endif

	return 0;
}

MODULE_DEVICE_TABLE(of, l3extension_id);
#define MODULE_NAME	"l3extension"
static struct platform_driver l3extension_driver = {
	.probe	= l3extension_probe,
	.remove = l3extension_remove,
	.driver = {
		.name = MODULE_NAME,
		.of_match_table = l3extension_id,
		.pm = &l3extension_pm,
		.owner = THIS_MODULE,
	},
};

static int __init l3extension_init(void)
{
	return platform_driver_register(&l3extension_driver);
}

static void __exit l3extension_exit(void)
{
	platform_driver_unregister(&l3extension_driver);
}

module_init(l3extension_init);
module_exit(l3extension_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("L3Cache extension driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
