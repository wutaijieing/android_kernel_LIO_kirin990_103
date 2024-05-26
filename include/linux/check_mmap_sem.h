/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: provide function to find out mmap_sem's owner.
 * Author: Gong Chen <gongchen4@huawei.com>
 * Create: 2020-11-11
 */
#ifndef _MMAP_SEM_CHECK_H_
#define _MMAP_SEM_CHECK_H_

#include <linux/mm_types.h>
#include <linux/rwsem.h>
#include <linux/types.h>

void check_mmap_sem(pid_t pid);

void get_mmap_sem_debug(struct mm_struct *mm,
	void (*get_mmap_sem)(struct mm_struct *));

int get_mmap_sem_killable_debug(struct mm_struct *mm,
	int (*get_mmap_sem)(struct mm_struct *));

#ifdef CONFIG_DETECT_MMAP_SEM_DBG
void mmap_sem_debug(const struct rw_semaphore *sem);
#endif

#endif
