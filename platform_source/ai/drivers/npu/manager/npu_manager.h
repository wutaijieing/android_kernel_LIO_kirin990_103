/*
 * npu_manager.c
 *
 * about npu manager
 *
 * Copyright (c) 2012-2019 Huawei Technologies Co., Ltd.
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
#ifndef __NPU_MANAGER_H
#define __NPU_MANAGER_H

#include <linux/list.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/uio_driver.h>
#include <linux/notifier.h>
#include <linux/radix-tree.h>

#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif
#include "npu_user_common.h"

#define NPU_MANAGER_DEVICE_ENV  0

#define NPU_AI_SUBSYS_SDMA_WORKING_STATUS_OFFSET  5
#define NPU_AI_SUBSYS_SPCIE_WORKING_STATUS_OFFSET 6

struct npu_black_box {
	struct semaphore black_box_sema;
	spinlock_t spinlock;
	struct list_head exception_list;
	u32 exception_num;
	pid_t black_box_pid;
	u8 thread_should_stop;
};

struct npu_manager_info {
	u32 plat_info;   /* 0:device side, 1: host side */
	struct cdev cdev;
	struct list_head pm_list_header;  /* for power manager */
	spinlock_t pm_list_lock;   /* for power manager */
};

struct npu_manager_info *npu_get_manager_info(void);

#ifdef CONFIG_HUAWEI_DSM
struct dsm_client * npu_get_dsm_client(void);
#endif

#endif /* __NPU_MANAGER_H */
