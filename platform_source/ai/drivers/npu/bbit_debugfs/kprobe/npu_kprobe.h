/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: for npu kprobe
 */

#ifndef __BLKBK_TEST_KPROBE_H__
#define __BLKBK_TEST_KPROBE_H__

#include <linux/kprobes.h>

int npu_add_kprobe(char *func_name, kprobe_pre_handler_t handler_pre,
	kprobe_post_handler_t handler_post, kprobe_fault_handler_t handler_fault);

int npu_enable_kprobe(char *func_name, kprobe_pre_handler_t handler_pre);

int npu_disable_kprobe(char *func_name, kprobe_pre_handler_t handler_pre);

int npu_load_kprobe(void);

void npu_unload_kprobe(void);

#endif

