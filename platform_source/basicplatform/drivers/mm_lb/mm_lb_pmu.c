/*
 * mm_lb_pmu.c
 *
 * pmu event statistics for lb drv
 *
 * Copyright (c) 2018-2019 Huawei Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/idr.h>
#include <linux/sysfs.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

#include "mm_lb_priv.h"

#define LB_USER   BIT(0)
#define LB_KERN   BIT(1)
#define LB_INT    BIT(2)
#define LB_POLL   BIT(3)

#define LB_PMU_STATGO_MODE_MAX        4
#ifdef CONFIG_MM_LB_GEMINI
#define LB_E_MAX                      77
#define LB_PMU_STATGO_SHIFT           1
#define lb_pmu_statgo_enable(mode_tp)                 \
	((0x10000 << ((mode_tp) * LB_PMU_STATGO_SHIFT)) |   \
	 (0x1 << ((mode_tp) * LB_PMU_STATGO_SHIFT)))

#define lb_pmu_statgo_disable(mdoe_tp)      \
	(0x10000 << ((mdoe_tp) * LB_PMU_STATGO_SHIFT))
#else
#define LB_E_MAX                      76
#define LB_PMU_STATGO_SHIFT           4
#define lb_pmu_statgo_enable(mode_tp)                 \
	((0xf0000 << ((mode_tp) * LB_PMU_STATGO_SHIFT)) |   \
	 (0xf << ((mode_tp) * LB_PMU_STATGO_SHIFT)))

#define lb_pmu_statgo_disable(mdoe_tp)      \
	(0xf0000 << ((mdoe_tp) * LB_PMU_STATGO_SHIFT))
#endif

#define LB_PMU_CNT_IDX                  16
#define LB_PMU_EVENT_CNT_IDX            18
#define LB_PMU_COMMON_COUNTER_MAX       12
#define LB_PMU_CNT_8                    8
#define LB_PMU_CNT_11                   11
#define LB_PMU_CNT_MUX_IDX              3
#define LB_PMU_CNT_TH_IDX               12
#define PMU_MISS_REPLACE_CNT_START      8
#define CNT8_11_MODE_SEL                0x1111

#define PMU_CNT_STEP          8
#define PMU_CNT_MUX_STEP      8
#define PMU_CNT_MAX           0x3
#define PMU_CNT_MUX_MASK      0xff
#define get_cnt_mux(n)        (SLC_CM_CNT_MUX0 + (n) & ~PMU_CNT_MAX)
#define pmu_cnt_mux_shift(n)  (((n) & 0x3U) * 8U)

#define LB_COUNT_SHIFT 12
#define LB_COUNT_MASK  ((1U << LB_COUNT_SHIFT) - 1)

typedef void (*cfg_func)(u32 r_reg0, u32 f_reg1, u32 f_reg2, u32 f_reg3);

enum {
	STATGO_EVENT_0 = 0,
	STATGO_EVENT_1,
	STATGO_BW,
	STATGO_GLB,
};

struct lb_event {
	char *name;
	u32 cfg0;
	u32 cfg1;
	u32 cfg2;
	u32 cfg3;
	u32 wk;
	u32 mode;
	u32 countable;
	cfg_func ops;
};

struct lb_counter {
	int stat;
	int eno;
	int cno;
	int mode;

	u32 r_cfg0;
	u32 r_cfg1;
	u32 r_cfg2;
	u32 r_cfg3;
	struct list_head node;
};

struct lb_pmu_man {
	int mnr;
	int cur;
	int bitmap;
	spinlock_t lock;
	struct lb_device *lbd;
	struct list_head active;
};

static struct lb_pmu_man man;

/* pmu register config */
#include "event.c"

static struct lb_event etbl[LB_E_MAX] = {
#include "event.h"
};

static int __find_bit(struct lb_pmu_man *pman, int spbit, int mode)
{
	int i;
	int s = 0;

	spin_lock(&pman->lock);

	/* static counter map */
	if (spbit) {
		i = spbit;
		goto static_bit;
	}

	/* mode1 counter */
	if (mode == STATGO_EVENT_1)
		s = PMU_MISS_REPLACE_CNT_START;

	/* it is mean first fit */
	for (i = s; i < pman->mnr; i++) {
		if (!(pman->bitmap & BIT(i)))
			break;
	}

	/* i is overflow */
	if (i >= pman->mnr) {
		spin_unlock(&pman->lock);
		return -1;
	}

static_bit:
	pman->bitmap |= BIT(i);

	spin_unlock(&pman->lock);

	return i;
}

static void __reset_bit(struct lb_pmu_man *pman, int i)
{
	if (i >= pman->mnr)
		return;

	spin_lock(&pman->lock);

	pman->bitmap &= ~BIT(i);

	spin_unlock(&pman->lock);
}

static struct lb_counter *create_counter(struct lb_pmu_man *pman,
		struct lb_counter input)
{
	int nr;
	struct lb_counter *lbc = NULL;

	/* just for the static map */
	nr = __find_bit(pman, etbl[input.eno].countable, etbl[input.eno].mode);
	if (nr < 0)
		goto out;

	lbc = kzalloc(sizeof(*lbc), GFP_KERNEL);
	if (!lbc) {
		lb_print(LB_ERROR, "zalloc lb counter is failed\n");
		goto find_bit;
	}

	lbc->eno = input.eno;
	lbc->cno = nr;
	lbc->r_cfg0 = input.r_cfg0;
	lbc->r_cfg1 = input.r_cfg1;
	lbc->r_cfg2 = input.r_cfg2;
	lbc->r_cfg3 = input.r_cfg3;
	lbc->stat = LB_USER;
	lbc->mode = etbl[input.eno].mode;

	/* map event */
	pmu_map_event(lbc->cno, lbc->eno, lbc->mode);

	return lbc;

find_bit:
	__reset_bit(pman, nr);
out:
	return lbc;
}

static struct lb_counter *find_counter(struct lb_pmu_man *pman, int eno)
{
	bool flag = 0;
	struct lb_counter *pos = NULL;
	struct lb_counter *next = NULL;

	spin_lock(&pman->lock);

	list_for_each_entry_safe(pos, next, &man.active, node) {
		if (pos->eno == eno) {
			flag = true;
			break;
		}
	}

	spin_unlock(&pman->lock);

	return flag ? pos : NULL;
}

static void set_counter(struct lb_pmu_man *pman, struct lb_counter *cnter)
{
	u32 reg0, reg1, reg2, reg3;

	if (cnter->r_cfg0 || cnter->r_cfg1
	    || cnter->r_cfg2 || cnter->r_cfg3) {
		reg0 = cnter->r_cfg0;
		reg1 = cnter->r_cfg1;
		reg2 = cnter->r_cfg2;
		reg3 = cnter->r_cfg3;
	} else {
		reg0 = etbl[cnter->eno].cfg0;
		reg1 = etbl[cnter->eno].cfg1;
		reg2 = etbl[cnter->eno].cfg2;
		reg3 = etbl[cnter->eno].cfg3;
	}

	etbl[cnter->eno].ops(reg0, reg1, reg2, reg3);
}

static void add_counter(struct lb_pmu_man *pman, struct lb_counter *cnter)
{
	spin_lock(&pman->lock);
	list_add(&cnter->node, &pman->active);
	spin_unlock(&pman->lock);
}

static void del_counter(struct lb_pmu_man *pman, struct lb_counter *cnter)
{
	spin_lock(&pman->lock);
	list_del(&cnter->node);
	spin_unlock(&pman->lock);
}

static void reset_counter(struct lb_pmu_man *pman, struct lb_counter *cnter)
{
	etbl[cnter->eno].ops(0, 0, 0, 0);
}

static void distory_counter(struct lb_pmu_man *pman, struct lb_counter *cnter)
{
	int nr = cnter->cno;

	kfree(cnter);

	pmu_unmap_event(nr);

	__reset_bit(pman, nr);
}

static void start_count(struct lb_pmu_man *pman)
{
	int i;
	u32 mode = 0x0;
	struct lb_counter *pos = NULL;
	struct lb_counter *next = NULL;

	spin_lock(&pman->lock);

	list_for_each_entry_safe(pos, next, &pman->active, node)
		mode |= BIT(pos->mode);

	for (i = 0; i < LB_PMU_STATGO_MODE_MAX; i++)
		if (mode & BIT(i))
			pmu_start_count(i);

	spin_unlock(&pman->lock);
}

static void end_count(struct lb_pmu_man *pman)
{
	int i;
	u32 mode = 0x0;
	struct lb_counter *pos = NULL;
	struct lb_counter *next = NULL;

	spin_lock(&pman->lock);

	list_for_each_entry_safe(pos, next, &pman->active, node)
		mode |= BIT(pos->mode);

	for (i = 0; i < LB_PMU_STATGO_MODE_MAX; i++)
		if (mode & BIT(i))
			pmu_end_count(i);

	spin_unlock(&pman->lock);
}

/* list all events */
static ssize_t list_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int i;
	int ret = 0;
	char *str = buf;
	ssize_t len = PAGE_SIZE;

	if (!man.lbd) {
		lb_print(LB_ERROR, "Lb device is not init\n");
		return -EINVAL;
	}

	for (i = 0; i < LB_E_MAX; i++) {
		ret += scnprintf(str + ret, len - ret,
				"[%d]:%s\n", i, etbl[i].name);
		if (unlikely(len < ret))
			break;
	}

	return ret;
}

/* cat the enable event */
static ssize_t poll_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	char *str = buf;
	ssize_t len = PAGE_SIZE;
	struct lb_counter *pos = NULL;
	struct lb_counter *next = NULL;

	if (!man.lbd) {
		lb_print(LB_ERROR, "Lb device is not init\n");
		return -EINVAL;
	}

	spin_lock(&man.lock);
	list_for_each_entry_safe(pos, next, &man.active, node) {
		ret += scnprintf(str + ret, len - ret,
				"Event[%d]:%s Counter[%d] result[%lx]\n",
				pos->eno, etbl[pos->eno].name, pos->cno,
				pmu_show(pos->cno));
		if (unlikely(ret >= len - 1))
			break;
	}
	spin_unlock(&man.lock);

	return ret;
}

/* create events */
static ssize_t set_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	char c;
	struct lb_counter input;
	struct lb_counter *cnter = NULL;

	if (!man.lbd) {
		lb_print(LB_ERROR, "Pmu is not init\n");
		return -EINVAL;
	}

	/* parse the buf or called cmd */
	(void)sscanf(buf, "%c %d %x %x %x %x", &c, &input.eno,
			&input.r_cfg0, &input.r_cfg1, &input.r_cfg2,
			&input.r_cfg3);
	if (input.eno > LB_E_MAX - 1 || input.eno < 0) {
		lb_print(LB_ERROR, "eno is invalid %d\n", input.eno);
		return -EINVAL;
	}

	switch (c) {
	case 'a':
		/* if countable != 0, it mean event use static counter */
		cnter = create_counter(&man, input);
		if (!cnter) {
			lb_print(LB_ERROR, "create_counter is failed\n");
			return -EINVAL;
		}

		set_counter(&man, cnter);
		add_counter(&man, cnter);

		break;

	case 'd':
		cnter = find_counter(&man, input.eno);
		if (!cnter)
			return -EINVAL;

		reset_counter(&man, cnter);
		del_counter(&man, cnter);
		distory_counter(&man, cnter);

		break;

	default:
		return -EINVAL;
	}

	return len;
}

static ssize_t onoff_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
	char c;

	if (!man.lbd) {
		lb_print(LB_ERROR, "Pmu is not init\n");
		return -EINVAL;
	}

	/* parse the buf or called cmd */
	(void)sscanf(buf, "%c", &c);
	switch (c) {
	/* on */
	case 's':
		start_count(&man);

		lb_print(LB_ERROR, "on branch\n");
		break;

	/* off */
	case 'e':
		end_count(&man);

		lb_print(LB_ERROR, "off branch\n");
		break;

	default:

		lb_print(LB_ERROR, "default branch\n");
		return -EINVAL;
	}

	return len;
}

static DEVICE_ATTR_RO(list);
static DEVICE_ATTR_RO(poll);
static DEVICE_ATTR_WO(set);
static DEVICE_ATTR_WO(onoff);

static struct attribute *lb_pmu_attrs[] = {
	&dev_attr_list.attr,
	&dev_attr_poll.attr,
	&dev_attr_set.attr,
	&dev_attr_onoff.attr,

	NULL,
};

static struct attribute_group lb_pmu_attr_group = {
	.attrs = lb_pmu_attrs,
};

int lb_pmu_init(struct platform_device *pdev, struct lb_device *lbd)
{
	int ret;

	man.lbd = lbd;
	man.bitmap = (0xffffffff & ~LB_COUNT_MASK);
	man.mnr = LB_COUNT_SHIFT;
	man.cur = 0;

	spin_lock_init(&man.lock);
	INIT_LIST_HEAD(&man.active);

	ret = sysfs_create_group(&pdev->dev.kobj, &lb_pmu_attr_group);
	if (ret < 0) {
		lb_print(LB_ERROR, "Error creating sysfs group for device\n");
		goto sysfs_err;
	}

	return 0;

sysfs_err:
	memset(&man, 0x0, sizeof(man));

	return ret;
}
