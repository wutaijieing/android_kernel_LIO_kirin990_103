/*
 * ko_adapt.h
 *
 * function for find symbols not exported
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef KO_ADAPT_H
#define KO_ADAPT_H

#include <linux/types.h>
#include <linux/cred.h>
#include <linux/version.h>
#if (KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE)
#include <linux/sched/task.h>
#endif
#include <linux/kthread.h>
#include <linux/cpumask.h>
#include <linux/syscalls.h>
#include <linux/version.h>
#include <linux/gfp.h>

#ifdef CONFIG_TZDRIVER_MODULE

const struct cred *koadpt_get_task_cred(struct task_struct *task);
void koadpt_kthread_bind_mask(struct task_struct *task,
	const struct cpumask *mask);
long koadpt_sys_chown(const char __user *filename, uid_t user, gid_t group);
ssize_t koadpt_vfs_write(struct file *file, const char __user *buf,
	size_t count, loff_t *pos);
ssize_t koadpt_vfs_read(struct file *file, char __user *buf,
	size_t count, loff_t *pos);
struct page *koadpt_alloc_pages(gfp_t gfp_mask, unsigned int order);
struct workqueue_attrs *koadpt_alloc_workqueue_attrs(gfp_t gfp_mask);
void koadpt_free_workqueue_attrs(struct workqueue_attrs *attrs);

#else

static inline const struct cred *koadpt_get_task_cred(struct task_struct *task)
{
	return get_task_cred(task);
}

static inline void koadpt_kthread_bind_mask(struct task_struct *task,
	const struct cpumask *mask)
{
	kthread_bind_mask(task, mask);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0))
static inline long koadpt_sys_chown(const char __user *filename,
	uid_t user, gid_t group)
{
	return sys_chown(filename, user, group);
}
#else
static inline long koadpt_sys_chown(const char __user *filename,
	uid_t user, gid_t group)
{
	return ksys_chown(filename, user, group);
}
#endif

static inline ssize_t koadpt_vfs_read(struct file *file, char __user *buf,
	size_t count, loff_t *pos)
{
	return vfs_read(file, buf, count, pos);
}

static inline ssize_t koadpt_vfs_write(struct file *file, const char __user *buf,
	size_t count, loff_t *pos)
{
	return vfs_write(file, buf, count, pos);
}

static inline struct page *koadpt_alloc_pages(gfp_t gfp_mask, unsigned int order)
{
	return alloc_pages(gfp_mask, order);
}

static inline struct workqueue_attrs *koadpt_alloc_workqueue_attrs(
	gfp_t gfp_mask)
{
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(4, 19, 0))
	return alloc_workqueue_attrs(gfp_mask);
#else
	(void)gfp_mask;
	return alloc_workqueue_attrs();
#endif
}

static inline void koadpt_free_workqueue_attrs(struct workqueue_attrs *attrs)
{
	return free_workqueue_attrs(attrs);
}

#endif

#endif
