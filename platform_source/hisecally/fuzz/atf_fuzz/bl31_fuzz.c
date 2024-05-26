/*
 * bl31_fuzz.c
 *
 * about atf fuzz driver
 *
 * Copyright (c) 2022-2030 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/arm-smccc.h>
#include <linux/uaccess.h>
#include <global_ddr_map.h>
#include <asm/smp.h>
#include <linux/io.h>
#include <linux/kcov.h>

#define DEVICE_NAME       "atf_fuzz"
#define SMC_CALL           _IO(0x67, 0)
#define FUZZ_SHMEM_MARK    0xdeadbeef

static int dev_major = 0;

static struct class *bl31_fuzz_class = NULL;

typedef struct bl31_fuzz_device_data {
	struct cdev cdev;
}bl31_fuzz_device_data;

static bl31_fuzz_device_data bl31_fuzz_data;

unsigned long *atf_fuzz_shmem;

typedef struct smc_args {
	unsigned long a0;
	unsigned long a1;
	unsigned long a2;
	unsigned long a3;
	unsigned long a4;
	unsigned long a5;
	unsigned long a6;
	unsigned long a7;
}smc_args;

static int bl31_fuzz_open(struct inode *inode, struct file *file)
{
	printk(KERN_EMERG "Open ATF Fuzz module.\n");
	return 0;
}

static ssize_t bl31_fuzz_write(struct file *file, const char __user * buf, size_t count, loff_t *ppos)
{
	printk(KERN_EMERG "Write to ATF Fuzz module.\n");
	return 0;
}

static long bl31_fuzz_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	smc_args smc_arg;
	struct arm_smccc_res res;
	unsigned int *fuzz_share_mem;
	unsigned int cpuid;
	int flags;

	switch (cmd) {
	case SMC_CALL:
		copy_from_user(&smc_arg, (char __user *)arg, sizeof(smc_arg));
		fuzz_share_mem = (unsigned int *)atf_fuzz_shmem;
		if (!fuzz_share_mem) {
			pr_err("%s:%d, atf_fuzz_shmem is NULL\n", __func__, __LINE__);
			return -1;
		}
		cpuid = read_cpuid_mpidr();
		printk(KERN_EMERG "smc call return 0x%pK, 0x%pK, 0x%pK, 0x%pK, \
			0x%pK, 0x%pK, 0x%pK, 0x%pK\n",
			smc_arg.a0, smc_arg.a1, smc_arg.a2, smc_arg.a3,
			smc_arg.a4, smc_arg.a5, smc_arg.a6, smc_arg.a7);
		WRITE_ONCE(fuzz_share_mem[1], cpuid);
		arm_smccc_smc(smc_arg.a0, smc_arg.a1, smc_arg.a2, smc_arg.a3, smc_arg.a4,
			smc_arg.a5, smc_arg.a6, smc_arg.a7, &res);
		WRITE_ONCE(fuzz_share_mem[1], FUZZ_SHMEM_MARK);
		printk(KERN_EMERG "smc call return 0x%pK.\n", res.a0);
		break;
	default:
		printk(KERN_EMERG "Unknown ioctl.\n");
	}

	return 0;
}

static struct file_operations bl31_fuzz_fops = {
	.owner  =   THIS_MODULE,
	.open   =   bl31_fuzz_open,
	.write  =   bl31_fuzz_write,
	.unlocked_ioctl = bl31_fuzz_ioctl,
};

static int __init bl31_fuzz_init(void)
{
	int err;
	dev_t dev;

	err = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
	dev_major = MAJOR(dev);
	bl31_fuzz_class = class_create(THIS_MODULE, DEVICE_NAME);
	cdev_init(&bl31_fuzz_data.cdev, &bl31_fuzz_fops);
	bl31_fuzz_data.cdev.owner = THIS_MODULE;
	cdev_add(&bl31_fuzz_data.cdev, MKDEV(dev_major, 0), 1);
	device_create(bl31_fuzz_class, NULL, MKDEV(dev_major, 0), NULL, DEVICE_NAME, 0);
	atf_fuzz_shmem = ioremap_wc(RESERVED_BL31_FUZZ_SMEM_BASE,
		RESERVED_BL31_FUZZ_SMEM_END - RESERVED_BL31_FUZZ_SMEM_BASE);
	if (!atf_fuzz_shmem) {
		pr_err("bl31 fuzz_share_mem remap failed\n");
		return -1;
	}
	printk(KERN_ALERT DEVICE_NAME " initilized.\n");
	return 0;
}

static void __exit bl31_fuzz_exit(void)
{
	device_destroy(bl31_fuzz_class,  MKDEV(dev_major, 0));
	cdev_del(&bl31_fuzz_data.cdev);
	class_destroy(bl31_fuzz_class);
	unregister_chrdev_region(MKDEV(dev_major, 0), 0);
	printk(KERN_EMERG DEVICE_NAME " removed.\n");
}

module_init(bl31_fuzz_init);
module_exit(bl31_fuzz_exit);
MODULE_LICENSE("GPL");
