/*
 * linux_osal.h
 *
 * This is for linux_osal proc.
 *
 * Copyright (c) 2019-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef LINUX_OSAL
#define LINUX_OSAL

#include "vfmw.h"
#include <asm/cacheflush.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/wait.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/poll.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <asm/ioctl.h>
#if (KERNEL_VERSION(3, 18, 0) >= LINUX_VERSION_CODE)
#include <asm/system.h>
#endif
#include <linux/semaphore.h>
#include <linux/random.h>
#include <linux/printk.h>
#include <linux/mutex.h>
#include <linux/io.h>

#define OFS_BASE1 0x020
#define OFS_BASE2 0x100
#define MDMA_BASE 0x00
#define REG_OFFSET 0x000

#define rd_vreg(base, reg, dat) \
	do { \
		(dat) = readl(((uint8_t *)(base) + (reg))); \
	} while (0)

#define wr_vreg(base, reg, dat) \
	do { \
		writel((dat), ((uint8_t *)(base) + (reg))); \
	} while (0)

typedef struct {
	wait_queue_head_t queue_head;
	int32_t flag;
} kern_event;

typedef struct {
	spinlock_t irq_lock;
	int is_init;
} kern_irq_lock;

typedef struct task_struct osal_task;
typedef kern_event         osal_event;
typedef kern_irq_lock      osal_irq_spin_lock;
typedef struct file        osal_file;
typedef struct semaphore   osal_sema;
typedef atomic_t           osal_atomic;

void osal_intf_init(void);
void osal_intf_exit(void);
sec_mode linux_get_cur_sec_mode(void);

#endif
