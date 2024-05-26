/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: for fm core bbit debug
 */

#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include "npu_log.h"
#include "npu_pm_framework.h"
#include "npu_bbit_core.h"
#include "npu_bbit_debugfs.h"

typedef int (*core_func)(struct user_msg msg);

int npu_bbit_ctrl_core(struct user_msg msg)
{
	struct ctrl_core_para para = {0};
	int ret = 0;

	if (msg.buf == NULL) {
		npu_drv_err("invalid pointer.\n");
		return -1;
	}

	if (copy_from_user(&para, msg.buf, sizeof(para)) != 0) {
		npu_drv_err("copy from user failed \n");
		return -1;
	}

	ret = npu_ctrl_core(para.dev_id, para.core_num);
	if (ret != 0)
		npu_drv_err("set npu ctrl core fail ret %d", ret);

	npu_bbit_set_result(ret);
	return 0;
}

core_func core_func_map[] = {
	npu_bbit_ctrl_core,
};

ssize_t npu_bbit_core_write(struct file *filp, const char __user *buf,
	size_t count, loff_t *ppos)
{
	core_func func = NULL;
	struct user_msg msg = {0};
	int ret = 0;
	unused(filp);
	unused(ppos);

	if (buf == NULL) {
		npu_drv_err("invalid pointer.\n");
		return -1;
	}
	
	if (copy_from_user(&msg, buf, sizeof(msg)) != 0) {
		npu_drv_err("copy from user failed \n");
		return -1;
	}

	if (msg.interface_id >= CORE_MAX) {
		npu_drv_err("invalid interface_id %d.\n", msg.interface_id);
		return -1;
	}
	func = core_func_map[msg.interface_id];
	ret = func(msg);

	if (ret != 0) {
		npu_drv_err("npu bbit core write failed ret %d.\n", ret);
		return -1;
	}

	return count;
}

ssize_t npu_bbit_core_read(struct file *filp, char __user *buf,
	size_t count, loff_t *ppos)
{
	int result;
	int ret;
	unused(filp);
	unused(ppos);

	if (buf == NULL) {
		npu_drv_err("invalid pointer.\n");
		return -1;
	}

	ret = npu_bbit_get_result(&result);
	if (ret != 0) {
		npu_drv_err("get bbit result fail\n");
		result = -1;
	}

	if (copy_to_user(buf, &result, sizeof(result)) != 0) {
		npu_drv_err("copy to user buffer failed.\n");
		return -1;
	}

	return count;
}
