/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
 * Description: sensor sysfs implement.
 * Create: 2019/11/05
 */

#include <securec.h>

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include<linux/device.h>
#include<linux/sysfs.h>
#include<linux/platform_device.h>

#include "sensor_channel.h"
#include "common/common.h"
#include "iomcu_log.h"

static ssize_t show_sensor_list_info(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	if (buf == NULL) {
		ctxhub_err("[%s], buffer is NULL!\n", __func__);
		return 0;
	}
	return get_sensor_list_info(buf);
}

static DEVICE_ATTR(sensor_list_info, 0444, show_sensor_list_info, NULL);

static struct attribute *g_sensor_attributes[] = {
	&dev_attr_sensor_list_info.attr,
	NULL
};

static const struct attribute_group g_sensor_node = {
	.attrs = g_sensor_attributes,
};

static struct platform_device g_sensor_input_info = {
	.name = "sensor",
	.id = -1,
};

static int __init sensor_input_info_init(void)
{
	int ret;

	if (get_contexthub_dts_status())
		return RET_FAIL;

	ctxhub_info("[%s] ++\n", __func__);
	ret = platform_device_register(&g_sensor_input_info);
	if (ret) {
		ctxhub_err("%s: platform_device_register failed, ret:%d\n",
			__func__, ret);
		goto REGISTER_ERR;
	}

	ret = sysfs_create_group(&g_sensor_input_info.dev.kobj, &g_sensor_node);
	if (ret) {
		ctxhub_err("%s sysfs_create_group error ret =%d\n",
			__func__, ret);
		goto SYSFS_CREATE_CGOUP_ERR;
	}
	ctxhub_info("[%s] --\n", __func__);
	return 0;
SYSFS_CREATE_CGOUP_ERR:
	platform_device_unregister(&g_sensor_input_info);
REGISTER_ERR:
	return ret;
}

late_initcall_sync(sensor_input_info_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Sensor input info");
MODULE_AUTHOR("Smart");
