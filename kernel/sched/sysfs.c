// SPDX-License-Identifier: GPL-2.0
/*
 * sysfs.c
 *
 * eas sysfs
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

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <trace/events/sched.h>
#include <securec.h>

#include "sched.h"


struct eas_attr {
	struct attribute attr;
	ssize_t (*show)(struct kobject *kobj,
			struct kobj_attribute *kattr, char *buf);
	ssize_t (*store)(struct kobject *kobj, struct kobj_attribute *kattr,
			 const char *buf, size_t count);
	int *value;
};

static ssize_t eas_show(struct kobject *kobj,
			struct kobj_attribute *kattr, char *buf)
{
	struct eas_attr *eas_attr =
		container_of(&kattr->attr, struct eas_attr, attr);
	int temp;

	temp = *(eas_attr->value);
	return (ssize_t)sprintf_s(buf, PAGE_SIZE, "%d\n", temp);
}

static ssize_t eas_store(struct kobject *kobj, struct kobj_attribute *kattr,
			 const char *buf, size_t count)
{
	int temp;
	ssize_t ret = count;
	struct eas_attr *eas_attr =
		container_of(&kattr->attr, struct eas_attr, attr);

	if (kstrtoint(buf, 10, &temp))
		return -EINVAL;

	if (temp < 0)
		ret = -EINVAL;
	else
		*(eas_attr->value) = temp;

	/* trace the name and value of the attribute */
	trace_eas_attr_store(kattr->attr.name, temp);
	return ret;
}

#ifdef CONFIG_MULTI_MARGIN
static unsigned int *get_tokenized_data(const char *buf)
{
	const char *cp = NULL;
	int i;
	int ntokens = 1;
	unsigned int *tokenized_data = NULL;
	int err = -EINVAL;

	cp = buf;
	while ((cp = strpbrk(cp + 1, " :")))
		ntokens++;

	if (ntokens != num_clusters)
		goto err;

	tokenized_data = kmalloc(ntokens * sizeof(unsigned int), GFP_KERNEL);
	if (!tokenized_data) {
		err = -ENOMEM;
		goto err;
	}

	cp = buf;
	i = 0;
	while (i < ntokens) {
		if (sscanf_s(cp, "%u", &tokenized_data[i++]) != 1)
			goto err_kfree;

		cp = strpbrk(cp, " :");
		if (!cp)
			break;
		cp++;
	}

	if (i != ntokens)
		goto err_kfree;

	return tokenized_data;

err_kfree:
	kfree(tokenized_data);
err:
	return ERR_PTR(err);
}

static ssize_t capacity_margin_show(struct kobject *kobj,
				    struct kobj_attribute *kattr, char *buf)
{
	struct sched_cluster *cluster = NULL;
	ssize_t ret = 0;

	for_each_sched_cluster(cluster) {
		ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%u%s",
				 cluster->capacity_margin, ":");
	}

	if (ret > 0)
		(void)sprintf_s(buf + ret - 1, PAGE_SIZE - ret + 1, "\n");
	return ret;
}

static ssize_t sd_capacity_margin_show(struct kobject *kobj,
				       struct kobj_attribute *kattr, char *buf)
{
	struct sched_cluster *cluster = NULL;
	ssize_t ret = 0;

	for_each_sched_cluster(cluster) {
		ret += scnprintf(buf + ret, PAGE_SIZE - ret, "%u%s",
				 cluster->sd_capacity_margin, ":");
	}

	if (ret > 0)
		(void)sprintf_s(buf + ret - 1, PAGE_SIZE - ret + 1, "\n");
	return ret;
}

static ssize_t capacity_margin_store(struct kobject *kobj, struct kobj_attribute *kattr,
				     const char *buf, size_t count)
{
	unsigned int *new_margin = NULL;
	struct sched_cluster *cluster = NULL;
	int i;

	new_margin = get_tokenized_data(buf);
	if (IS_ERR(new_margin))
		return PTR_ERR(new_margin);

	i = 0;
	for_each_sched_cluster(cluster) {
		cluster->capacity_margin = new_margin[i++];
	}

	kfree(new_margin);
	return count;
}

static ssize_t sd_capacity_margin_store(struct kobject *kobj, struct kobj_attribute *kattr,
					const char *buf, size_t count)
{
	unsigned int *new_margin = NULL;
	struct sched_cluster *cluster = NULL;
	int i;

	new_margin = get_tokenized_data(buf);
	if (IS_ERR(new_margin))
		return PTR_ERR(new_margin);

	i = 0;
	for_each_sched_cluster(cluster) {
		cluster->sd_capacity_margin = new_margin[i++];
	}

	kfree(new_margin);
	return count;
}
#endif /* CONFIG_MULTI_MARGIN */

#ifdef CONFIG_SCHED_PRED_LOAD
static ssize_t predl_window_size_show(struct kobject *kobj,
				      struct kobj_attribute *kattr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%u\n", predl_window_size);
}

#define MIN_PREDL_WINDOW_SIZE 4000000 /* 4ms */
#define MAX_PREDL_WINDOW_SIZE 100000000 /* 100ms */
static ssize_t predl_window_size_store(struct kobject *kobj,
				       struct kobj_attribute *kattr, const char *buf, size_t count)
{
	unsigned int val;

	if (kstrtouint(buf, 10, &val))
		return -EINVAL;

	if (val < MIN_PREDL_WINDOW_SIZE ||
	    val > MAX_PREDL_WINDOW_SIZE)
		return -EINVAL;

	predl_window_size = val;

	return count;
}
#endif

/*
 * Note:
 * The field 'value' of eas_attr point to int, don't use type shorter than int.
 */
static struct eas_attr attrs[] = {
	{
		.attr = { .name = "boost", .mode = 0640, },
		.show = eas_show,
		.store = eas_store,
		.value = &global_boost_enable,
	},
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,4,0) || LINUX_VERSION_CODE >= KERNEL_VERSION(5,5,0))
	{
		.attr = { .name = "boot_boost", .mode = 0640, },
		.show = eas_show,
		.store = eas_store,
		.value = &boot_boost,
	},
	{
		.attr = { .name = "task_boost_limit", .mode = 0640, },
		.show = eas_show,
		.store = eas_store,
		.value = &task_boost_limit,
	},
	{
		.attr = { .name = "up_migration_util_filter", .mode = 0640, },
		.show = eas_show,
		.store = eas_store,
		.value = &up_migration_util_filter,
	},
#endif
#ifdef CONFIG_MULTI_MARGIN
	{
		.attr = { .name = "capacity_margin", .mode = 0640, },
		.show = capacity_margin_show,
		.store = capacity_margin_store,
		.value = NULL,
	},
	{
		.attr = { .name = "sd_capacity_margin", .mode = 0640, },
		.show = sd_capacity_margin_show,
		.store = sd_capacity_margin_store,
		.value = NULL,
	},
#else
	{
		.attr = { .name = "capacity_margin", .mode = 0640, },
		.show = eas_show,
		.store = eas_store,
		.value = &capacity_margin,
	},
	{
		.attr = { .name = "sd_capacity_margin", .mode = 0640, },
		.show = eas_show,
		.store = eas_store,
		.value = &sd_capacity_margin,
	},
#endif
#ifdef CONFIG_SCHED_PRED_LOAD
	{
		.attr = { .name = "predl_enable", .mode = 0640, },
		.show = eas_show,
		.store = eas_store,
		.value = &predl_enable,
	},
	{
		.attr = { .name = "predl_jump_load", .mode = 0640, },
		.show = eas_show,
		.store = eas_store,
		.value = &predl_jump_load,
	},
	{
		.attr = { .name = "predl_do_predict", .mode = 0640, },
		.show = eas_show,
		.store = eas_store,
		.value = &predl_do_predict,
	},
	{
		.attr = { .name = "predl_window_size", .mode = 0640, },
		.show = predl_window_size_show,
		.store = predl_window_size_store,
		.value = NULL,
	},
#endif
#ifdef CONFIG_SCHED_WALT
	{
		.attr = { .name = "walt_init_task_load_pct", .mode = 0640, },
		.show = eas_show,
		.store = eas_store,
		.value = &sysctl_sched_walt_init_task_load_pct,
	},
#endif
#ifdef CONFIG_PRODUCT_ARMPC
	{
		.attr = { .name = "sync_hint", .mode = 0640, },
		.show = eas_show,
		.store = eas_store,
		.value = &sysctl_sched_sync_hint_enable,
	},
#endif
};

struct eas_data_struct {
	struct attribute_group attr_group;
	struct attribute *attributes[ARRAY_SIZE(attrs) + 1];
} eas_data;

static int eas_attr_init(void)
{
	int i, nr_attr;

	nr_attr = ARRAY_SIZE(attrs);
	for (i = 0; i < nr_attr; i++)
		eas_data.attributes[i] = &attrs[i].attr;
	eas_data.attributes[i] = NULL;

	eas_data.attr_group.name = "eas";
	eas_data.attr_group.attrs = eas_data.attributes;

	return sysfs_create_group(kernel_kobj, &eas_data.attr_group);
}
late_initcall(eas_attr_init);
