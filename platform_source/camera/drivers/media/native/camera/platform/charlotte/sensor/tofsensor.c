/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2020. All rights reserved.
 * Description: dev drvier to communicate with sensorhub tof_swing app
 * Create: 2017-04-30
 */
#include <linux/version.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/rpmsg.h>
#include <linux/adc.h>
#include <linux/notifier.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>
#include <linux/debugfs.h>
#include <linux/io.h>
#include <linux/syscalls.h>
#include <linux/miscdevice.h>
#include <linux/completion.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <securec.h>
#include "sensor_commom.h"
#include "hwsensor.h"
#include "cam_log.h"

extern long tof_igs_sensor_ioctl(struct file *file,
	unsigned int cmd, unsigned long args);

static int tof_igs_sensor_open(struct inode *inode, struct file *file)
{
	cam_info("enter %s \n", __func__);

	return 0;
}

static int tof_igs_sensor_release(struct inode *inode, struct file *file)
{
	cam_info("enter %s \n", __func__);

	return 0;
}

static int get_tof_sensor_dts_status(void)
{
	struct device_node *node = NULL;

	node = of_find_compatible_node(NULL, NULL, "huawei,swing_tof_sensor");
	if (node == NULL) {
		cam_warn("%s, no swing tof sensor\n", __func__);
		return -1;
	}

	if (!of_device_is_available(node)) {
		cam_warn("%s swing tof sensor disabled\n", __func__);
		return -1;
	}

	cam_info("%s enabled\n", __func__);
	return 0;
}

static const struct file_operations tof_igs_sensor_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = tof_igs_sensor_ioctl,
	.open = tof_igs_sensor_open,
	.release = tof_igs_sensor_release,
};

static struct miscdevice tof_igs_miscsensor = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "tof_swing_sensor",
	.fops = &tof_igs_sensor_fops,
};

static int __init tof_igs_sensor_init(void)
{
	int ret;

	cam_info("%s : enter\n", __func__);
	ret = get_tof_sensor_dts_status();
	if (ret != 0) {
		cam_warn("%s tof_swing sensor is disabled\n", __func__);
		return ret;
	}

	ret = misc_register(&tof_igs_miscsensor);
	if (ret != 0) {
		cam_err("%s cannot register miscdev err=%d\n", __func__, ret);
		return ret;
	}

	return ret;
}

static void __exit tof_igs_sensor_exit(void)
{
	cam_info("%s : enter\n", __func__);

	misc_deregister(&tof_igs_miscsensor);

	return;
}

module_init(tof_igs_sensor_init);
module_exit(tof_igs_sensor_exit);
