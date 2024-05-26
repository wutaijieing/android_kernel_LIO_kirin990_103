/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019. All rights reserved.
 * Description: sensor feima implement.
 * Create: 2019/11/05
 */
#include "sensor_feima.h"

#include <securec.h>

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/mm.h>

#include "platform_include/smart/linux/iomcu_dump.h"
#include "iomcu_log.h"
#include "sensor_channel.h"
#include "common/common.h"
#include "device_manager.h"
#include "device_interface.h"
#include "iomcu_debug.h"
#include "plat_func.h"


#define ALS_MCU_HAL_CONVER 10
#define ACC_CONVERT_COFF 1000
#define BASE 10

#define MIN_DELAY 10
#define MAX_DELAY 1000

static struct class *g_sensors_class;

int check_sensor_cookie(struct sensor_cookie *data)
{
	if (!data || (!data->name) || !(data->tag >= TAG_BEGIN && data->tag < TAG_END) ||
		!(data->shb_type >= SENSORHUB_TYPE_BEGIN && data->shb_type < SENSORHUB_TYPE_END)) {
		ctxhub_err("error in %s\n", __func__);
		return -EINVAL;
	}
	return RET_SUCC;
}

static int check_device_tag(int tag)
{
	if (!(tag >= TAG_DEVICE_BEGIN && tag < TAG_DEVICE_END)) {
		ctxhub_err("%s, tag is not device tag\n", __func__);
		return -EINVAL;
	}
	return RET_SUCC;
}

ssize_t show_enable(struct device *dev,
		   struct device_attribute *attr, char *buf)
{
	struct sensor_cookie *data =
		(struct sensor_cookie *)dev_get_drvdata(dev);
	if (check_sensor_cookie(data) != RET_SUCC)
		return -EINVAL;
	if (check_device_tag(data->tag) != RET_SUCC)
		return -EINVAL;
	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "%d\n",
			g_device_status.opened[data->tag]);
}

ssize_t store_enable(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t size)
{
	unsigned long val = 0;
	int ret;
	const char *operation = NULL;
	struct sensor_cookie *data =
		(struct sensor_cookie *)dev_get_drvdata(dev);
	bool enable = true;

	if (check_sensor_cookie(data) != RET_SUCC)
		return -EINVAL;

	if (kstrtoul(buf, BASE, &val))
		return -EINVAL;

	enable = (val == 1);
	operation = (enable ? "enable" : "disable");
	ctxhub_info("%s, shb_type is %d, tag is %d\n", __func__, data->shb_type, data->tag);
	ret = inputhub_sensor_channel_enable(data->shb_type, data->tag, enable);
	if (ret) {
		ctxhub_err("%s %s failed, ret = %d in %s\n",
			operation, data->name, ret, __func__);
		return size;
	}

	return size;
}

ssize_t show_set_delay(struct device *dev,
			      struct device_attribute *attr,
				char *buf)
{
	struct sensor_cookie *data =
		(struct sensor_cookie *)dev_get_drvdata(dev);

	if (check_sensor_cookie(data) != RET_SUCC)
		return -EINVAL;
	if (check_device_tag(data->tag) != RET_SUCC)
		return -EINVAL;
	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "%d\n",
			g_device_status.delay[data->tag]);
}

ssize_t store_set_delay(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			size_t size)
{
	unsigned int val = 0;
	int ret;
	struct sensor_cookie *data =
		(struct sensor_cookie *)dev_get_drvdata(dev);
	struct ioctl_para para;
	const char *cmd_buf = get_str_begin(buf);
	bool is_batch = false;

	if (check_sensor_cookie(data) != RET_SUCC)
		return -EINVAL;

	(void)memset_s(&para, sizeof(para), 0, sizeof(para));
	/* 
	 * one param at least, two param at most.
	 * one param is delay.
	 * two param is delay, timeout.
	 */
	if (get_arg(cmd_buf, &val))
		para.delay_ms = val;
	else
		return -EINVAL;

	cmd_buf = get_str_begin(get_str_end(cmd_buf));
	if (get_arg(cmd_buf, &val)) {
		para.timeout_ms = val;
		is_batch = true;
	}

	para.shbtype = data->shb_type;
	ret = inputhub_sensor_channel_setdelay(data->tag, &para, is_batch);
	if (ret) {
		ctxhub_err("set %s delay_ms %d ms timeout %d ms error in %s\n",
			data->name, (int)para.delay_ms, (int)para.timeout_ms, __func__);
		return -EINVAL;
	}

	ctxhub_info("set %s delay_ms %d ms timeout %d ms in %s\n",
		data->name, (int)para.delay_ms, (int)para.timeout_ms, __func__);

	return size;
}

static const char *get_sensor_info_by_tag(int tag)
{
	struct device_manager_node *node = NULL;

	node = get_device_manager_node_by_tag(tag);

	return (node) ? node->device_chip_info : "";
}

ssize_t show_info(struct device *dev, struct device_attribute *attr,
		 char *buf)
{
	struct sensor_cookie *data =
		(struct sensor_cookie *)dev_get_drvdata(dev);

	if (check_sensor_cookie(data) != RET_SUCC)
		return -EINVAL;

	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "%s\n",
			get_sensor_info_by_tag(data->tag));
}

ssize_t show_get_data(struct device *dev, struct device_attribute *attr,
		     char *buf)
{
	struct sensor_cookie *data =
		(struct sensor_cookie *)dev_get_drvdata(dev);
	unsigned int hal_sensor_tag;
	struct t_sensor_get_data *get_data = NULL;
	int offset = 0;
	int i = 0;
	int mem_num;
	int ret;

	if (check_sensor_cookie(data) != RET_SUCC)
		return -EINVAL;

	hal_sensor_tag = data->shb_type;

	get_data = get_sensors_status_data(hal_sensor_tag);
	if (get_data == NULL)
		return -EINVAL;

	atomic_set(&get_data->reading, 1);
	reinit_completion(&get_data->complete);
	/* return -ERESTARTSYS if interrupted, 0 if completed */
	if (wait_for_completion_interruptible(&get_data->complete) == 0) {
		mem_num = get_data->data.length /
			sizeof(get_data->data.value[0]);
		for (; i < mem_num; i++) {
			if ((data->tag == TAG_ALS) && (i == 0))
				get_data->data.value[0] =
					get_data->data.value[0] /
					ALS_MCU_HAL_CONVER;

			if (data->tag == TAG_ACCEL)
				/* need be devicdd by 1000.0 for high resolu */
				get_data->data.value[i] =
					get_data->data.value[i] /
					ACC_CONVERT_COFF;

			ret = sprintf_s(buf + offset, PAGE_SIZE - offset,
					"%d\t", get_data->data.value[i]);
			if (ret < 0) {
				ctxhub_err("[%s], sprintf_s failed!\n", __func__);
				return 0;
			}
			offset += ret;
		}
		ret = sprintf_s(buf + offset, PAGE_SIZE - offset, "\n");
		if (ret < 0) {
			ctxhub_err("[%s], sprintf_s failed!\n", __func__);
			return 0;
		}
		offset += ret;
	}

	return offset;
}

ssize_t store_get_data(struct device *dev,
		       struct device_attribute *attr, const char *buf,
			size_t size)
{
	struct sensor_cookie *data =
		(struct sensor_cookie *)dev_get_drvdata(dev);
	struct sensor_data event;
	int arg;
	unsigned int argc = 0;

	if (check_sensor_cookie(data) != RET_SUCC)
		return -EINVAL;

	/* parse cmd buffer */
	for (; (buf = get_str_begin(buf)) != NULL; buf = get_str_end(buf)) {
		if (get_arg(buf, &arg)) {
			if (argc < (sizeof(event.value) /
				sizeof(event.value[0]))) {
				event.value[argc++] = arg;
			} else {
				ctxhub_err("too many args, %d will be ignored\n",
					arg);
			}
		}
	}
	/* fill sensor event and write to sensor event buffer */
	report_sensor_event(data->shb_type, event.value,
			    sizeof(event.value[0]) * argc);

	return size;
}


ssize_t show_sensorlist_info(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	int ret;
	struct device_manager_node *dev_node = NULL;
	struct sensor_cookie *data =
		(struct sensor_cookie *)dev_get_drvdata(dev);

	if (check_sensor_cookie(data) != RET_SUCC)
		return -EINVAL;

	dev_node = get_device_manager_node_by_tag(data->tag);
	if (!dev_node) {
		ctxhub_err("[%s], get tag %d dev_node pointer null\n",
							__func__, data->tag);
		return 0;
	}

	ret = memcpy_s(buf, PAGE_SIZE, &dev_node->sensorlist_info,
		       sizeof(struct sensorlist_info));
	if (ret != EOK) {
		ctxhub_err("[%s], tag %d memcpy failed!\n",
					__func__, data->tag);
		return 0;
	}
	return sizeof(struct sensorlist_info);
}

#define sensor_comm_attr(TYPE, TAG, NAME, SHB_TYPE) \
static global_device_attr(TYPE##_sensorlist_info, 0660, show_sensorlist_info, NULL); \
static struct attribute *g_##TYPE##_sensor_attrs[] = { \
	&g_dev_attr_##TYPE##_sensorlist_info.attr, \
	&g_dev_attr_enable.attr, \
	&g_dev_attr_set_delay.attr, \
	&g_dev_attr_info.attr, \
	&g_dev_attr_get_data.attr, \
	NULL, \
}; \
static const struct attribute_group g_##TYPE##_sensor_attrs_grp = { \
	.attrs = g_##TYPE##_sensor_attrs, \
}; \
static struct sensor_cookie g_##TYPE##_sensor = { \
	.tag = TAG, \
	.name = NAME, \
	.shb_type = SHB_TYPE, \
	.attrs_group = &g_##TYPE##_sensor_attrs_grp, \
}

/* files create for every sensor */
global_device_attr(enable, 0660, show_enable, store_enable);
global_device_attr(set_delay, 0660, show_set_delay, store_set_delay);
global_device_attr(info, 0440, show_info, NULL);
global_device_attr(get_data, 0660, show_get_data, store_get_data);

int create_class_files(struct class_attribute *cls_atr)
{
	if (!cls_atr) {
		ctxhub_err("[%s], input class attribute pointer is NULL\n",
			__func__);
		return -EINVAL;
	}

	if (class_create_file(g_sensors_class, cls_atr)) {
		ctxhub_err("[%s], create class files failed!\n", __func__);
		return -EINVAL;
	}
	return 0;
}

int sensor_file_register(struct sensor_cookie *sensor)
{
	if (!sensor) {
		ctxhub_err("[%s], input sensor pointer is NULL!\n", __func__);
		return -EINVAL;
	}

	sensor->dev =
		device_create(g_sensors_class, NULL, 0, sensor, sensor->name); //lint !e592
	if (!sensor->dev) {
		ctxhub_err("[%s], device create Failed", __func__);
		return -EINVAL;
	}
	if (sensor->attrs_group) {
		if (sysfs_create_group(&sensor->dev->kobj,
				       sensor->attrs_group)) {
			ctxhub_err("[%s], create sensor files failed\n",
				__func__);
			return -EINVAL;
		}
	}
	return 0;
}

static ssize_t cls_attr_dump_show_func(struct class *cls,
				       struct class_attribute *attr, char *buf)
{
	ctxhub_info("read sensorhub_dump node, IOM7 will restart\n");
	iom3_need_recovery(SENSORHUB_USER_MODID, SH_FAULT_USER_DUMP);
	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, //lint !e592
			"read sensorhub_dump node, IOM7 will restart\n");
}

static struct class_attribute g_class_attr_sensorhub_dump =
__ATTR(sensorhub_dump, 0660, cls_attr_dump_show_func, NULL);

static ssize_t cls_attr_kernel_support_lib_ver_show_func(struct class *cls,
							 struct class_attribute *attr, char *buf)
{
	int ret;
	u32 ver = 15; // for support large resolution acc sensor

	ctxhub_info("read %s %u\n", __func__, ver);
	ret = memcpy_s(buf, PAGE_SIZE, &ver, sizeof(ver));
	if (ret != EOK) {
		ctxhub_err("[%s], memecpy failed!\n", __func__);
		return 0;
	}

	return sizeof(ver);
}

static struct class_attribute g_class_attr_libsensor_ver =
	__ATTR(libsensor_ver, 0660,
	       cls_attr_kernel_support_lib_ver_show_func, NULL);

static int sensors_feima_init(void)
{
	int ret;

	if (get_contexthub_dts_status())
		return -EINVAL;
	g_sensors_class = class_create(THIS_MODULE, "sensors");
	if (IS_ERR(g_sensors_class))
		return PTR_ERR(g_sensors_class);

	ret = create_class_files(&g_class_attr_sensorhub_dump);
	if (ret != 0)
		ctxhub_err("create sensor dump class files failed\n");

	ret = create_class_files(&g_class_attr_libsensor_ver);
	if (ret != 0)
		ctxhub_err("create sensor version class files failed\n");
	return 0;
}

static void sensors_feima_exit(void)
{
	device_destroy(g_sensors_class, 0);
	class_destroy(g_sensors_class);
}

late_initcall(sensors_feima_init);
module_exit(sensors_feima_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("SensorHub feima driver");
MODULE_AUTHOR("Smart");
