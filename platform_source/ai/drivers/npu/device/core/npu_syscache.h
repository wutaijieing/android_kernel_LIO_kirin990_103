/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description:
 * Author:
 * Create: 2021-07-16
 */
#ifndef __NPU_SYSCACHE_H
#define __NPU_SYSCACHE_H

#include <linux/workqueue.h>
#include <linux/atomic.h>


int npu_ioctl_attach_syscache(struct npu_proc_ctx *proc_ctx,
	unsigned long arg);

int npu_ioctl_set_sc_prio(struct npu_proc_ctx *proc_ctx,
	unsigned long arg);

int npu_ioctl_switch_sc(struct npu_proc_ctx *proc_ctx,
	unsigned long arg);


#endif
