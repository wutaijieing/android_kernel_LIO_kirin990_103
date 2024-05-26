/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: for fm core bbit debug
 */

#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include "npu_log.h"
#include "npu_pm_framework.h"
#include "npu_kprobe_debugfs.h"
#include "npu_kprobe_core.h"

ssize_t npu_kprobe_write(struct file *filp, const char __user *buf,
	size_t count, loff_t *ppos)
{
	int ret = 0;
	struct kprobe_para para = {0};
	unused(filp);
	unused(ppos);

	npu_drv_debug("into\n");
	if (buf == NULL) {
		npu_drv_err("invalid pointer.\n");
		return -1;
	}
	if (copy_from_user(&para, buf, sizeof(para)) != 0) {
		npu_drv_err("copy from user failed \n");
		return -1;
	}

	para.func_name[sizeof(para.func_name) - 1] = 0;
	switch (para.fm) {
	case NPU_DRV_FM_CORE:
		ret = npu_kprobe_core_ablility(para.cmd, para.func_name, para.index);
		break;
	default:
		npu_drv_err("invalid fm %d.\n", para.fm);
		ret = -1;
		break;
	}

	if (ret != 0)
		return -1;

	npu_drv_debug("out\n");
	return count;
}

ssize_t npu_kprobe_read(struct file *filp, char __user *buf, size_t count,
	loff_t *ppos)
{
	unused(filp);
	unused(buf);
	unused(ppos);
	npu_drv_debug("for test\n");
	return count;
}
