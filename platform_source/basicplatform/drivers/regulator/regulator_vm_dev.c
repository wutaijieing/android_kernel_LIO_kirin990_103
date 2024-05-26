/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * regulator_vm_dev.c
 *
 * add regulator vm dev for hm_uvmm
 *
 * This software is licensed under the terms of the GNU General Public
 * either version 2 of that License or (at your option) any later version.
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/regulator/consumer.h>
#include <securec.h>

#define REGULATOR_VM_DEV_NAME "regulator_dev"

#define NAME_MAX_SIZE           24
#define REGULATOR_HVC_ID        0xAACC00E0
#define REGULATOR_VM_ENABLE     0xAACC00E1
#define REGULATOR_VM_DISABLE    0xAACC00E2

struct regulator_vm_dev_device {
	struct device *dev;
	int dev_major;
	struct class *dev_class;
};

struct regulator_st {
	char name[NAME_MAX_SIZE];
	int *val;
};

static DEFINE_MUTEX(regulator_dev_lock);
struct regulator_vm_dev_device *g_regulator_dev = NULL;

static int regulator_from_virt_to_phys(unsigned long args, struct regulator_st *reg)
{
	unsigned int ret;
	void __user *user_args = NULL;

	user_args = (void __user *)(uintptr_t)args;
	if (user_args == NULL) {
		pr_err("[%s] user_args %pK\n", __func__, user_args);
		return -EINVAL;
	}

	ret = copy_from_user((void*)reg, user_args, sizeof(*reg));
	if (ret != 0) {
		pr_err("[%s] copy_from_user %d\n", __func__, ret);
		return -EINVAL;
	}
	return 0;
}

static int regulator_vm_dev_enable(unsigned long args, unsigned int enable)
{
	int ret;
	unsigned int flag;
	struct regulator *sreg = NULL;
	struct regulator_st reg = {0};

	ret = regulator_from_virt_to_phys(args, &reg);
	if (ret != EOK)
		return -EINVAL;

	sreg = regulator_get(g_regulator_dev->dev, reg.name);
	if (IS_ERR_OR_NULL(sreg)) {
		pr_err("[%s] regulator_get error.\n", __func__);
		return -EINVAL;
	}

	if (enable != 0)
		flag = (unsigned int)regulator_enable(sreg);
	else
		flag = (unsigned int)regulator_disable(sreg);

	ret = (int)copy_to_user((void __user *)reg.val, (void*)&flag, sizeof(int));
	regulator_put(sreg);
	return ret;
}

static long regulator_vm_dev_ioctl(struct file *filp, unsigned int cmd,
	unsigned long args)
{
	int ret;

	mutex_lock(&regulator_dev_lock);
	switch (cmd) {
	case REGULATOR_VM_ENABLE:
		ret = regulator_vm_dev_enable(args, 1);
		break;

	case REGULATOR_VM_DISABLE:
		ret = regulator_vm_dev_enable(args, 0);
		break;

	default:
		ret = -EINVAL;
		pr_err("[%s] invalid cmd value\n", __func__);
	}
	mutex_unlock(&regulator_dev_lock);
	return ret;
}

const static struct file_operations regulator_vm_dev_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = regulator_vm_dev_ioctl,
};

int __init regulator_vm_dev_init(void)
{
	struct regulator_vm_dev_device *vm_dev = NULL;
	struct device *pdevice = NULL;
	int ret = 0;

	vm_dev = (struct regulator_vm_dev_device *)kzalloc(
			sizeof(struct regulator_vm_dev_device), GFP_KERNEL);
	if (!vm_dev) {
		pr_err("%s failed to alloc regulator_vm_dev_device struct\n", __func__);
		return -ENOMEM;
	}

	vm_dev->dev_major = register_chrdev(0, REGULATOR_VM_DEV_NAME, &regulator_vm_dev_fops);
	if (vm_dev->dev_major < 0) {
		pr_err("[%s] unable to get dev_majorn", __func__);
		ret = -EFAULT;
		goto dev_major_err;
	}

	mutex_lock(&regulator_dev_lock);
	vm_dev->dev_class = class_create(THIS_MODULE, REGULATOR_VM_DEV_NAME);
	if (IS_ERR(vm_dev->dev_class)) {
		pr_err("[%s] class_create error\n", __func__);
		ret = -EFAULT;
		mutex_unlock(&regulator_dev_lock);
		goto dev_class_err;
	}

	pdevice = device_create(vm_dev->dev_class, NULL,
			MKDEV((unsigned int)vm_dev->dev_major, 0), NULL, REGULATOR_VM_DEV_NAME);
	if (IS_ERR(pdevice)) {
		pr_err("regulator_vm_dev: device_create error\n");
		ret = -EFAULT;
		mutex_unlock(&regulator_dev_lock);
		goto dev_create_err;
	}
	g_regulator_dev = vm_dev;

	mutex_unlock(&regulator_dev_lock);
	return ret;

dev_create_err:
	class_destroy(vm_dev->dev_class);
	vm_dev->dev_class = NULL;

dev_class_err:
	unregister_chrdev(vm_dev->dev_major, REGULATOR_VM_DEV_NAME);
	vm_dev->dev_major = 0;

dev_major_err:
	kfree(vm_dev);
	return ret;
}

static void __exit  regulator_vm_dev_exit(void)
{
	if (IS_ERR_OR_NULL(g_regulator_dev))
		return;

	device_destroy(g_regulator_dev->dev_class, MKDEV((unsigned int)g_regulator_dev->dev_major, 0));
	class_destroy(g_regulator_dev->dev_class);
	g_regulator_dev->dev_class = NULL;
	unregister_chrdev(g_regulator_dev->dev_major, REGULATOR_VM_DEV_NAME);
	g_regulator_dev->dev_major = 0;

	kfree(g_regulator_dev);
	g_regulator_dev = NULL;
}

module_init(regulator_vm_dev_init);
module_exit(regulator_vm_dev_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("regulator vm dev module");

