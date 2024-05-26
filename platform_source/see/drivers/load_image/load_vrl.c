/*
 *  Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 *  Description: vrl_info driver
 *  Create : 2022/06/17
 */
#include <linux/device.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <securec.h>
#include "load_image.h"

#define VRL_INFO_NODE_NAME "vrl_info"
#define VRL_OPEN_INIT_FINISHED     0
#define VRL_OPEN_INIT_UNFINISHED   -1

static s32 vrl_open_init(struct inode *inode, struct file *file)
{
	s32 ret;
	s32 vrl_read_flag = VRL_OPEN_INIT_UNFINISHED;

	if (vrl_read_flag == VRL_OPEN_INIT_UNFINISHED) {
		ret = read_vrl_dev();
		vrl_read_flag = VRL_OPEN_INIT_FINISHED;
	} else {
		ret = -EFAULT;
	}
	return ret;
}

static const struct file_operations g_vrl_info_fops = {
	.owner = THIS_MODULE,
	.open = vrl_open_init,
};

static s32 __init vrl_info_driver_init(void)
{
	s32 ret = 0;
	s32 major;
	struct class *pclass = NULL;
	struct device *pdevice = NULL;

	major = register_chrdev(0, VRL_INFO_NODE_NAME, &g_vrl_info_fops);
	if (major <= 0) {
		ret = -EFAULT;
		pr_err("unable to get major.\n");
		goto error0;
	}

	pclass = class_create(THIS_MODULE, VRL_INFO_NODE_NAME);
	if (IS_ERR(pclass)) {
		ret = -EFAULT;
		pr_err("class_create error.\n");
		goto error1;
	}

	pdevice = device_create(pclass, NULL, MKDEV((u32)major, 0), NULL, VRL_INFO_NODE_NAME);
	if (IS_ERR(pdevice)) {
		ret = -EFAULT;
		pr_err("device_create error.\n");
		goto error2;
	}

	pr_info("success\n");
	return ret;

error2:
	class_destroy(pclass);
error1:
	unregister_chrdev(major, VRL_INFO_NODE_NAME);
error0:
	return ret;
}

fs_initcall_sync(vrl_info_driver_init);

MODULE_DESCRIPTION("vrl_info_driver_init");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL");
