/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: for hwts register config stub compile
 */

#include "npu_bbit_hwts_config.h"
#include "npu_common.h"

ssize_t npu_bbit_hwts_write(struct file *filp, const char __user *buf,
	size_t count, loff_t *ppos)
{
	unused(filp);
	unused(buf);
	unused(count);
	unused(ppos);
	return 0;
}

ssize_t npu_bbit_hwts_read(struct file *filp, char __user *buf, size_t count,
	loff_t *ppos)
{
	unused(filp);
	unused(buf);
	unused(count);
	unused(ppos);
	return 0;
}

