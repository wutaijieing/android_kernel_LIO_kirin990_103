/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: for fm core bbit debug
 */
#ifndef __NPU_KPROBE_DEBUGFS_H
#define __NPU_KPROBE_DEBUGFS_H

typedef enum kprobe_cmd {
	KPROBE_ENABLE,
	KPROBE_DISABLE,
} kprobe_cmd_t;

typedef enum npu_drv_fm {
	NPU_DRV_FM_CORE,
} npu_drv_fm_t;

#define MAX_FUNC_NAME 50

struct kprobe_para {
	kprobe_cmd_t cmd;
	npu_drv_fm_t fm;
	char func_name[MAX_FUNC_NAME];
	uint8_t index; // default 0 ,only one probe function
};

ssize_t npu_kprobe_write(struct file *filp, const char __user *buf,
	size_t count, loff_t *ppos);
ssize_t npu_kprobe_read(struct file *filp, char __user *buf,
	size_t count, loff_t *ppos);

#endif
