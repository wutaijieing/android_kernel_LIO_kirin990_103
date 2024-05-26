/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 *
 * pinctrl_vm_dev.c
 *
 * add pinctrl vm dev for hm_uvmm
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
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define PINCTRL_VM_DEV_NAME        "pinctrl_dev"

#define PCS_IOMAP_NUM 4 /* pincfg pinmux common base */
#define PCS_IOMAP_SIZE 0x1000
#define PCS_REG_BASE_MASK 0xFFFFF000
#define PCS_REG_OFFSET_MASK 0xFFF
#define FUNC_MASK 0x7

struct pinctrl_data {
	size_t addr;
	size_t value;
};

struct pcs_ioremap {
	unsigned int phys_base;
	void __iomem *base;
};

struct pinctrl_vm_dev_device {
	struct device *dev;
	int     dev_major;
	struct class *dev_class;
	struct pinctrl_data pdata;
	struct pcs_ioremap reg_map[PCS_IOMAP_NUM];
	int map_num;
};

#define PINCTRL_REG_READ 0xD00D0001
#define PINCTRL_REG_WRITE 0xD00D0002
#define PINCTRL_REG_SET 0xD00D0003

struct pinctrl_vm_dev_device *g_pinctrl_dev = NULL;

static void __iomem *pinctrl_vm_dev_ioremap(unsigned int phys_base)
{
	struct pinctrl_vm_dev_device *vm_dev = NULL;
	int i;
	int num;

	vm_dev = g_pinctrl_dev; /* g_pinctrl_dev not be nullptr */
	num = (vm_dev->map_num < PCS_IOMAP_NUM) ? vm_dev->map_num : PCS_IOMAP_NUM;

	/* find */
	for(i = 0; i < num; i++) {
		if(vm_dev->reg_map[i].phys_base == phys_base)
			return vm_dev->reg_map[i].base;
	}

	if(i == PCS_IOMAP_NUM) {
		pr_err("[%s] vm_dev->map_num %d fail\n", __func__, vm_dev->map_num);
		i = PCS_IOMAP_NUM - 1;
		iounmap(vm_dev->reg_map[i].base);
	}

	/* not find */
	vm_dev->reg_map[i].phys_base = phys_base;
	vm_dev->reg_map[i].base = ioremap(phys_base, PCS_IOMAP_SIZE);
	vm_dev->map_num++;
	pr_err("[%s] phys %p, %p\n", __func__, phys_base, vm_dev->reg_map[i].base);

	return vm_dev->reg_map[i].base;
}

static long pinctrl_vm_dev_ioctl(struct file *filp, unsigned int cmd,
	unsigned long args)
{
	void __user *user_args = NULL;
	void __iomem *v_addr = NULL;
	unsigned int val;
	struct pinctrl_data pdata;
	int ret = 0;

	user_args = (void __user *)(uintptr_t)args;
	if (user_args == NULL) {
		pr_err("[%s] user_args %pK\n", __func__, user_args);
		return -EINVAL;
	}

	ret = copy_from_user(&pdata, user_args,
			sizeof(pdata));
	if (ret != 0) {
		pr_err("[%s] copy_from_user %d\n", __func__, ret);
		return -EFAULT;
	}

	v_addr = pinctrl_vm_dev_ioremap(pdata.addr & PCS_REG_BASE_MASK);
	v_addr += (pdata.addr & PCS_REG_OFFSET_MASK);

	switch (cmd) {
	case PINCTRL_REG_READ:
		pdata.value = readl(v_addr);
		ret = copy_to_user(user_args, &pdata, sizeof(pdata));
		break;
	case PINCTRL_REG_WRITE:
		writel(pdata.value, v_addr);
		break;
	case PINCTRL_REG_SET:
		val = readl(v_addr);
		val &= ~FUNC_MASK;
		val |= (pdata.value & FUNC_MASK);
		writel(val, v_addr);
		break;
	default:
		ret = -EINVAL;
		pr_err("[%s] invalid cmd value\n", __func__);
	}

	return ret;
}


const static struct file_operations pinctrl_vm_dev_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = pinctrl_vm_dev_ioctl,
};

static int __init pinctrl_vm_dev_init(void)
{
	struct pinctrl_vm_dev_device *pvm_dev = NULL;
	struct device *pdevice = NULL;
	int ret = 0;

	pvm_dev = (struct pinctrl_vm_dev_device *)kzalloc(
			sizeof(struct pinctrl_vm_dev_device), GFP_KERNEL);
	if (!pvm_dev) {
		pr_err("%s failed to alloc pvm_dev struct\n", __func__);
		return -ENOMEM;
	}

	pvm_dev->dev_major = register_chrdev(0, PINCTRL_VM_DEV_NAME, &pinctrl_vm_dev_fops);
	if (pvm_dev->dev_major < 0) {
		pr_err("[%s] unable to get dev_majorn",__func__);
		ret = -EFAULT;
		goto dev_major_err;
	}

	pvm_dev->dev_class = class_create(THIS_MODULE, PINCTRL_VM_DEV_NAME);
	if (IS_ERR(pvm_dev->dev_class)) {
		pr_err("[%s] class_create error\n",__func__);
		ret = -EFAULT;
		goto dev_class_err;
	}

	pdevice = device_create(pvm_dev->dev_class, NULL,
			MKDEV(pvm_dev->dev_major, 0), NULL, PINCTRL_VM_DEV_NAME);
	if (IS_ERR(pdevice)) {
		pr_err("pinctrl_vm_dev: device_create error\n");
		ret = -EFAULT;
		goto dev_create_err;
	}

	g_pinctrl_dev = pvm_dev;
	pr_err("pinctrl_vm_dev: device_create success\n");
	return ret;

dev_create_err:
	class_destroy(pvm_dev->dev_class);
	pvm_dev->dev_class = NULL;

dev_class_err:
	unregister_chrdev(pvm_dev->dev_major, PINCTRL_VM_DEV_NAME);
	pvm_dev->dev_major = 0;

dev_major_err:
	kfree(pvm_dev);
	return ret;
}

static void __exit  pinctrl_vm_dev_exit(void)
{
	if (g_pinctrl_dev == NULL)
		return;

	device_destroy(g_pinctrl_dev->dev_class, MKDEV(g_pinctrl_dev->dev_major, 0));
	class_destroy(g_pinctrl_dev->dev_class);
	g_pinctrl_dev->dev_class = NULL;
	unregister_chrdev(g_pinctrl_dev->dev_major, PINCTRL_VM_DEV_NAME);
	g_pinctrl_dev->dev_major = 0;

	kfree(g_pinctrl_dev);
	g_pinctrl_dev = NULL;
}

module_init(pinctrl_vm_dev_init);
module_exit(pinctrl_vm_dev_exit);
