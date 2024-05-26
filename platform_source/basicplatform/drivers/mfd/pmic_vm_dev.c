/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * pmic_vm_dev.c
 *
 * add pmic vm dev for hm_uvmm
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
#include <platform_include/basicplatform/linux/mfd/pmic_platform.h>
#include <linux/uaccess.h>

#define PMIC_VM_DEV_NAME        "pmic_dev"

struct pmic_data {
	size_t slave_id;
	size_t addr;
	size_t value;
};

struct pmic_vm_dev_device {
	struct device *dev;
	int     dev_major;
	struct class *dev_class;
	struct pmic_data pdata;
};

#define PMIC_REG_WRITE 0xabcd0001
#define PMIC_REG_READ 0xabcd0000
#define PMIC_MAIN_DEV 0x9
#define PMIC_SUB_DEV 0x1

static DEFINE_MUTEX(pmic_dev_lock);
struct pmic_vm_dev_device *g_pmic_dev = NULL;

static void pmic_dev_read_reg(struct pmic_data *pdata)
{
	unsigned int ret = 0;

	switch(pdata->slave_id) {
	case PMIC_MAIN_DEV:
		ret = pmic_read_reg(pdata->addr);
		break;
	case PMIC_SUB_DEV:
		ret = sub_pmic_reg_read(pdata->addr);
		break;
	default:
		pr_err("[%s] invalid cmd value\n", __func__);
	}

	pdata->value = ret;
}

static void pmic_dev_write_reg(struct pmic_data *pdata)
{
	switch(pdata->slave_id) {
	case PMIC_MAIN_DEV:
		pmic_write_reg(pdata->addr, pdata->value);
		break;
	case PMIC_SUB_DEV:
		sub_pmic_reg_write(pdata->addr, pdata->value);
		break;
	default:
		pr_err("[%s] invalid cmd value\n", __func__);
	}
}

static long pmic_vm_dev_ioctl(struct file *filp, unsigned int cmd,
	unsigned long args)
{
	void __user *user_args = NULL;
	struct pmic_data pdata;
	int ret = 0;

	mutex_lock(&pmic_dev_lock);
	user_args = (void __user *)(uintptr_t)args;
	if (user_args == NULL) {
		pr_err("[%s] user_args %pK\n", __func__, user_args);
		mutex_unlock(&pmic_dev_lock);
		return -EINVAL;
	}

	ret = (int)copy_from_user(&pdata, user_args,
			sizeof(pdata));
	if (ret != 0) {
		pr_err("[%s] copy_from_user %d\n", __func__, ret);
		mutex_unlock(&pmic_dev_lock);
		return -EFAULT;
	}

	switch (cmd) {
	case PMIC_REG_READ:
		pmic_dev_read_reg(&pdata);
		ret = (int)copy_to_user(user_args, &pdata, sizeof(pdata));
		if (ret != 0) {
			pr_err("[%s] copy_to_user %d\n", __func__, ret);
			ret = -EFAULT;
		}
		break;
	case PMIC_REG_WRITE:
		pmic_dev_write_reg(&pdata);
		break;
	default:
		ret = -EINVAL;
		pr_err("[%s] invalid cmd value\n", __func__);
	}

	mutex_unlock(&pmic_dev_lock);
	return ret;
}


const static struct file_operations pmic_vm_dev_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = pmic_vm_dev_ioctl,
};

static int __init pmic_vm_dev_init(void)
{
	struct pmic_vm_dev_device *pvm_dev = NULL;
	struct device *pdevice = NULL;
	int ret = 0;

	pvm_dev = (struct pmic_vm_dev_device *)kmalloc(
			sizeof(struct pmic_vm_dev_device), GFP_KERNEL);
	if (!pvm_dev) {
		pr_err("%s failed to alloc pvm_dev struct\n", __func__);
		return -ENOMEM;
	}

	pvm_dev->dev_major = register_chrdev(0, PMIC_VM_DEV_NAME, &pmic_vm_dev_fops);
	if (pvm_dev->dev_major < 0) {
		pr_err("[%s] unable to get dev_majorn",__func__);
		ret = -EFAULT;
		goto dev_major_err;
	}

	mutex_lock(&pmic_dev_lock);
	pvm_dev->dev_class = class_create(THIS_MODULE, PMIC_VM_DEV_NAME);
	if (IS_ERR(pvm_dev->dev_class)) {
		pr_err("[%s] class_create error\n",__func__);
		ret = -EFAULT;
		goto dev_class_err;
	}

	pdevice = device_create(pvm_dev->dev_class, NULL,
			MKDEV((unsigned int)pvm_dev->dev_major, 0), NULL, PMIC_VM_DEV_NAME);
	if (IS_ERR(pdevice)) {
		pr_err("pmic_vm_dev: device_create error\n");
		ret = -EFAULT;
		goto dev_create_err;
	}

	g_pmic_dev = pvm_dev;
	mutex_unlock(&pmic_dev_lock);
	pr_err("pmic_vm_dev: device_create success\n");
	return ret;

dev_create_err:
	class_destroy(pvm_dev->dev_class);
	pvm_dev->dev_class = NULL;

dev_class_err:
	unregister_chrdev(pvm_dev->dev_major, PMIC_VM_DEV_NAME);
	pvm_dev->dev_major = 0;
	mutex_unlock(&pmic_dev_lock);

dev_major_err:

	kfree(pvm_dev);
	return ret;
}

static void __exit  pmic_vm_dev_exit(void)
{
	if (g_pmic_dev == NULL)
		return;

	device_destroy(g_pmic_dev->dev_class, MKDEV(g_pmic_dev->dev_major, 0));
	class_destroy(g_pmic_dev->dev_class);
	g_pmic_dev->dev_class = NULL;
	unregister_chrdev(g_pmic_dev->dev_major, PMIC_VM_DEV_NAME);
	g_pmic_dev->dev_major = 0;

	kfree(g_pmic_dev);
	g_pmic_dev = NULL;
}

module_init(pmic_vm_dev_init);
module_exit(pmic_vm_dev_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("pmic vm dev module");
