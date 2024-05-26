/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: for fm core bbit debug
 */

#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>

#include "npu_log.h"
#include "npu_pm_framework.h"
#include "npu_kprobe.h"
#include "npu_kprobe_debugfs.h"
#include "npu_kprobe_core.h"

int npu_ctrl_core_replace(u32 dev_id, u32 core_num)
{
	unused(dev_id);
	unused(core_num);
	npu_drv_warn("into ctrl core replace ret -1 ");
	return -1;
}

int  npu_ctrl_core_replace_pre(struct kprobe *kp, struct pt_regs *regs)
{
	if ((kp == NULL) || (regs == NULL)) {
		npu_drv_warn("invalid pointer");
		return -1;
	}

	regs->pc = (unsigned long)(uintptr_t)&npu_ctrl_core_replace;
	reset_current_kprobe();
	npu_drv_warn("out");
	return 1;
}

int npu_core_enable_kprobe(char *func_name, uint8_t index)
{
	int ret;
	unused(index);

	if (strncmp(func_name, NPU_CTRL_CORE_FUNC, strlen(func_name)) == 0) {
		ret = npu_enable_kprobe(NPU_CTRL_CORE_FUNC,
			npu_ctrl_core_replace_pre);
	} else {
		npu_drv_warn("invalid func nuame %s ", func_name);
		ret = -1;
	}
	return ret;
}

int npu_core_disable_kprobe(char *func_name, uint8_t index)
{
	int ret;
	unused(index);

	npu_drv_warn("disbale kprobe func nuame %s ", func_name);
	ret = npu_disable_kprobe(func_name, npu_ctrl_core_replace_pre);

	return ret;
}

int npu_kprobe_core_ablility(kprobe_cmd_t cmd, char *func_name, uint8_t index)
{
	int ret;

	if (cmd == KPROBE_ENABLE)
		ret = npu_core_enable_kprobe(func_name, index);
	else
		ret = npu_core_disable_kprobe(func_name, index);

	return ret;
}
