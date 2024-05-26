/*
 * ashmem_status.c
 *
 * Add For Detect Ashmem Leak
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

#include <linux/sched/signal.h>
#include <linux/sched/task.h>
#include <linux/fdtable.h>

struct ashmem_debug_process_heap_args {
	struct seq_file *seq;
	struct task_struct *tsk;
	size_t *total_ashmem_size;
};

static int ashmem_debug_process_heap_cb(const void *data,
					struct file *f, unsigned int fd)
{
	const struct ashmem_debug_process_heap_args *args = data;
	struct task_struct *tsk = args->tsk;
	struct ashmem_area *asma = NULL;

	if (f == NULL || f->f_op == NULL || f->f_op != &ashmem_fops)
		return 0;

	asma = f->private_data;
	*args->total_ashmem_size += asma->size;
	pr_info("%16s %16u %16u %50s %16u\n",
		tsk->comm, tsk->pid, fd, asma->name, asma->size);

	return 0;
}

int ashmem_heap_show(void)
{
	struct task_struct *tsk = NULL;
	size_t task_total_ashmem_size;
	struct ashmem_debug_process_heap_args cb_args;

	pr_info("Process ashmem heap info:\n");
	pr_info("-------------------------------------------------\n");
	pr_info("%16s %16s %16s %50s %16s\n",
		"Process name", "Process ID",
		"fd", "ashmem_name", "size");

	mutex_lock(&ashmem_mutex);
	rcu_read_lock();
	for_each_process(tsk) {
		if (tsk->flags & PF_KTHREAD)
			continue;

		task_total_ashmem_size = 0;
		cb_args.tsk = tsk;
		cb_args.total_ashmem_size = &task_total_ashmem_size;

		task_lock(tsk);
		iterate_fd(tsk->files, 0,
			   ashmem_debug_process_heap_cb, (void *)&cb_args);
		if (task_total_ashmem_size > 0)
			pr_info("%16s %-16s %16zu\n",
				"Total ashmem size of ",
				tsk->comm, task_total_ashmem_size);
		task_unlock(tsk);
	}
	rcu_read_unlock();
	mutex_unlock(&ashmem_mutex);

	return 0;
}
EXPORT_SYMBOL(ashmem_heap_show);
