/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2013-2022. All rights reserved.
 *
 * hisi_dieid.c
 *
 * hisilicon dieid driver
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
#include <asm/compiler.h>
#endif
#include <linux/compiler.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/cpumask.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <asm/memory.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/smp.h>
#include <linux/dma-mapping.h>
#include <linux/workqueue.h>
#include <linux/timer.h>

#include <platform_include/see/efuse_driver.h>
#include <platform_include/basicplatform/linux/mfd/pmic_platform.h>

#include <platform_include/basicplatform/linux/hisi_usb_phy_chip_efuse.h>
#include <linux/platform_drivers/audio_dieid.h>
#include <huawei_platform/power/huawei_charger.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
#include "../../scsi/ufs/hufs-lib.h"
#else
#include <linux/platform_drivers/ufs_dieid_interface.h>
#endif

#define OK                        0
#define NOEXIST                   (-1)

#define HISI_DIEID_DEV_NAME       "dieid"

#define HISI_DIEID_READ_DIEID     0x2000
#define HISI_DIEID_TIMEOUT_1000MS 1000

#define HISI_UFS_BUF_SIZE         800
#define HISI_SOC_BUF_SIZE         200
#define HISI_SOC_IP_SIZE          20
#define HISI_AP_DIEID_BUF_SIZE    1400
#define HISI_SOC_FLAG_SIZE        8

/* Hisi dieid test */
enum  hisi_ap_dieid_test {
	HISI_SOC_DIEID_MIN,
	HISI_SOC_DIEID = 1,
	HISI_MAIN_PMU_DIEID,
	HISI_SUB_PMU_DIEID,
	HISI_CHARGER_DIEID,
	HUFS_DIEID,
	HISI_CODEC_DIEID,
	HISI_AP_DIEID,
	HISI_USB_PHY_CHIP_DIEID,
	HISI_SOC_DIEID_MAX
};

struct hisi_dieid_info {
	unsigned int hisi_dieid;
	unsigned int buf_length;
	char *dieid_name;
	int (*get_dieid_func)(char *dieid, unsigned int len);
};

static int hisi_ap_get_dieid(char *dieid, unsigned int len);

static int hisi_charger_get_dieid(char *dieid, unsigned int len)
{
#ifdef CONFIG_HUAWEI_CHARGER_AP
	return huawei_charger_get_dieid(dieid, len);
#else
	pr_err("%s:no charger module!\n", __func__);
	return -EFAULT;
#endif
}

/*
 * these are temporary,after the soc adds interface
 * in the drvier,delete the function
 */
static int hisi_soc_get_dieid(char *dieid, unsigned int len)
{
	int ret, index;
	unsigned int length;
	unsigned char ip[HISI_SOC_IP_SIZE] = {0};
	char soc_buf[HISI_SOC_BUF_SIZE] = {0};

	ret = get_efuse_dieid_value(ip, sizeof(ip), HISI_DIEID_TIMEOUT_1000MS);
	if (ret != OK)
		pr_err("%s: %d: get_efuse_dieid_value failed,ret=%d\n",
			__func__, __LINE__, ret);

	ret = snprintf(soc_buf, sizeof(soc_buf), "\r\nSOC:0x");
	if (ret < OK) {
		pr_err("%s:snprintf is error!\n", __func__);
		return ret;
	}
	/* 2: the size of ip value */
	for (index = HISI_SOC_IP_SIZE - 1; index >= 0; index--) {
		ret = snprintf(soc_buf + HISI_SOC_FLAG_SIZE +
			2 * (HISI_SOC_IP_SIZE - index - 1),
			sizeof(soc_buf) - ret, "%02x", ip[index]);
		if (ret < OK) {
			pr_err("%s:snprintf is error!\n", __func__);
			return ret;
		}
	}
	/* 2: the size of ip value */
	ret = snprintf(soc_buf + HISI_SOC_FLAG_SIZE + 2 * HISI_SOC_IP_SIZE,
		sizeof(soc_buf) - ret, "\r\n");
	if (ret < OK) {
		pr_err("%s:snprintf is error!\n", __func__);
		return ret;
	}

	length = strlen(soc_buf);
	if (len >= length) {
		strncat(dieid, soc_buf, length);
	} else {
		pr_err("%s:dieid buf length is not enough!\n", __func__);
		return  length;
	}

	return OK;
}

struct hisi_dieid_info g_hisi_get_dieid[] = {
	{ HISI_SOC_DIEID, HISI_SOC_BUF_SIZE, "soc", hisi_soc_get_dieid },
	{ HISI_MAIN_PMU_DIEID, HISI_SOC_BUF_SIZE, "pmic", pmic_get_dieid },
	{ HISI_SUB_PMU_DIEID, HISI_SOC_BUF_SIZE, "sub pmu", sub_pmu_pmic_dieid },
	{ HISI_CHARGER_DIEID, HISI_SOC_BUF_SIZE, "charge", hisi_charger_get_dieid },
	{ HUFS_DIEID, HISI_AP_DIEID_BUF_SIZE, "ufs", hufs_get_dieid },
	{ HISI_CODEC_DIEID, HISI_SOC_BUF_SIZE, "codec", codec_get_dieid },
	{ HISI_AP_DIEID, HISI_AP_DIEID_BUF_SIZE, "ap", hisi_ap_get_dieid },
	{ HISI_USB_PHY_CHIP_DIEID, HISI_SOC_BUF_SIZE, "usb phy chip", hisi_usb_phy_chip_get_dieid },
};

static int hisi_ap_get_dieid(char *dieid, unsigned int len)
{
	int ret;
	char dieid_buf[HISI_UFS_BUF_SIZE] = {0};
	unsigned int length = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(g_hisi_get_dieid); i++) {
		if (i == (HISI_AP_DIEID - 1))
			continue;
		len -= length;
		memset(dieid_buf, 0, sizeof(dieid_buf));
		ret = g_hisi_get_dieid[i].get_dieid_func(dieid_buf,
			g_hisi_get_dieid[i].buf_length - 1);
		if (ret == OK) {
			length = strlen(dieid_buf);
			if (len >= length)
				strncat(dieid, dieid_buf, strlen(dieid_buf));
			else
				return -EFAULT;
		} else if (ret == NOEXIST) {
			length = 0;
			pr_err("%s:hisi %s get dieid is not exist!\n",
				__func__, g_hisi_get_dieid[i].dieid_name);
		} else {
			length = 0;
			pr_err("%s:hisi %s get dieid is error!\n", __func__,
				g_hisi_get_dieid[i].dieid_name);
		}
	}

	return OK;
}

int hisi_ap_get_dieid_test(int cmd)
{
	int ret;
	char ap_dieid[HISI_AP_DIEID_BUF_SIZE] = {0};
	int dieid_index;

	if (cmd >= HISI_SOC_DIEID_MAX || cmd <= HISI_SOC_DIEID_MIN) {
		pr_err("%s:test cmd:%d is fail!\n", __func__, cmd);
		return -EFAULT;
	}
	dieid_index = cmd - 1;
	if (g_hisi_get_dieid[dieid_index].buf_length > HISI_AP_DIEID_BUF_SIZE)
		return -EFAULT;
	ret = g_hisi_get_dieid[dieid_index].get_dieid_func(ap_dieid,
		g_hisi_get_dieid[dieid_index].buf_length - 1);
	if (ret == OK)
		pr_err("%s:test %s dieid is %s\n", __func__,
			g_hisi_get_dieid[dieid_index].dieid_name, ap_dieid);
	else if (ret == NOEXIST)
		pr_err("%s:test %s dieid is not exist!\n",
			__func__, g_hisi_get_dieid[dieid_index].dieid_name);
	else
		pr_err("%s:test read sub pmu dieid fail!\n",
			__func__);

	return ret;
}

/*
 * Function name:dieid_ioctl.
 * Discription:complement read/write dieid by terms of sending command-words.
 * return value:
 *          @ 0 - success.
 *          @ -1- failure.
 */
static long dieid_ioctl(struct file *file, u_int cmd, u_long arg)
{
	int ret;
	void __user *argp = (void __user *)arg;
	char ap_buffer[HISI_AP_DIEID_BUF_SIZE] = {0};
	int buf_length;

	pr_info("%s: %d: cmd=0x%x\n", __func__, __LINE__, cmd);

	if (!arg)
		return -EFAULT;
	switch (cmd) {
	case HISI_DIEID_READ_DIEID:
		ret = hisi_ap_get_dieid(ap_buffer, HISI_AP_DIEID_BUF_SIZE - 1);
		if (ret != 0) {
			pr_err("%s:pmic_get_dieid is error!\n",
				__func__);
			return -EFAULT;
		}
		buf_length = strlen(ap_buffer) + 1;
		if (buf_length > HISI_AP_DIEID_BUF_SIZE) {
			/* send back to user */
			if (copy_to_user(argp, ap_buffer,
				HISI_AP_DIEID_BUF_SIZE))
				ret = -EFAULT;
		} else {
			/* send back to user */
			if (copy_to_user(argp, ap_buffer,
				strlen(ap_buffer) + 1))
				ret = -EFAULT;
		}

		break;
	default:
		pr_info("[DIEID][%s] Unknow command!\n", __func__);
		ret = -ENOTTY;
		break;
	}

	return ret;
}

static const struct file_operations hisi_dieid_fops = {
	.owner =	THIS_MODULE,
	.unlocked_ioctl = dieid_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = dieid_ioctl,
#endif
};

static int dieid_major;
static struct class *dieid_class;

static int __init hisi_dieid_init(void)
{
	int ret = 0;
	struct device *pdevice = NULL;

	dieid_major = register_chrdev(0, HISI_DIEID_DEV_NAME,
		&hisi_dieid_fops);
	if (dieid_major <= 0) {
		pr_err("hisi dieid: unable to get dieid_major for memory devs\n");
		return -EFAULT;
	}

	dieid_class = class_create(THIS_MODULE, HISI_DIEID_DEV_NAME);
	if (IS_ERR(dieid_class)) {
		ret = -EFAULT;
		pr_err("hisi dieid: class_create error\n");
		goto error1;
	}

	pdevice = device_create(dieid_class, NULL, MKDEV(dieid_major, 0),
		NULL, HISI_DIEID_DEV_NAME);
	if (IS_ERR(pdevice)) {
		ret = -EFAULT;
		pr_err("hisi dieid: device_create error\n");
		goto error2;
	}

	return ret;

error2:
	class_destroy(dieid_class);
	dieid_class = NULL;
error1:
	unregister_chrdev(dieid_major, HISI_DIEID_DEV_NAME);
	dieid_major = 0;
	return ret;
}

static void __exit hisi_dieid_exit(void)
{
	device_destroy(dieid_class, MKDEV(dieid_major, 0));
	class_destroy(dieid_class);
	dieid_class = NULL;
	unregister_chrdev(dieid_major, HISI_DIEID_DEV_NAME);
	dieid_major = 0;
}

late_initcall_sync(hisi_dieid_init);
module_exit(hisi_dieid_exit);

MODULE_DESCRIPTION("Huawei dieid module");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
MODULE_LICENSE("GPL");
