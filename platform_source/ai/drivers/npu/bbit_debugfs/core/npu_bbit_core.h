/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: for fm core bbit debug
 */
#ifndef __NPU_BBIT_CORE_H
#define __NPU_BBIT_CORE_H
#include "npu_proc_ctx.h"

struct ctrl_core_para {
	uint32_t dev_id;
	uint32_t core_num;
};

enum CORE_INTERFACE {
	NPU_CTRL_CORE,
	CORE_MAX,
};

ssize_t npu_bbit_core_write(struct file *filp, const char __user *buf,
	size_t count, loff_t *ppos);
ssize_t npu_bbit_core_read(struct file *filp, char __user *buf,
	size_t count, loff_t *ppos);

#endif
