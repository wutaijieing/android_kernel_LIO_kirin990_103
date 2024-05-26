/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
 * Description: pedo debug.
 * Create: 2020/03/05
 */

#include "sensor_feima.h"
#include <securec.h>

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/kernel.h>

#include "pedometer.h"
#include "common/common.h"
#include <platform_include/smart/linux/base/ap/protocol.h>
#include "iomcu_log.h"
#include "platform_include/smart/linux/sensorhub_as.h"
#include "device_manager.h"


#define pedo_attrs(TYPE) \
static struct attribute *g_##TYPE##_sensor_attrs[] = { \
	&g_dev_attr_##TYPE##_sensorlist_info.attr, \
	&g_dev_attr_pedo_enable.attr, \
	&g_dev_attr_pedo_set_delay.attr, \
	&g_dev_attr_info.attr, \
	&g_dev_attr_get_data.attr, \
	NULL, \
}

#define pedo_attrs_grp(TYPE) \
static const struct attribute_group g_##TYPE##_sensor_attrs_grp = { \
	.attrs = g_##TYPE##_sensor_attrs, \
}

#define pedo_sensor_cookie(TAG, SHB_TYPE, IS_CUSTOMIZED, NAME, TYPE) \
static struct sensor_cookie g_##TYPE##_sensor = { \
	.tag = TAG, \
	.shb_type = SHB_TYPE, \
	.is_customized = IS_CUSTOMIZED, \
	.name = NAME, \
	.attrs_group = &g_##TYPE##_sensor_attrs_grp, \
}

static ssize_t show_pedo_sensorlist_info(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	int ret;
	struct sensor_cookie *data =
		(struct sensor_cookie *)dev_get_drvdata(dev);
	struct sensorlist_info *ar_sensorlist_info = NULL;

	if (check_sensor_cookie(data) != RET_SUCC)
		return -EINVAL;

	ar_sensorlist_info = get_pedo_sensor_list_info(data->shb_type);
	if (!ar_sensorlist_info) {
		ctxhub_err("[%s], get pedo sensor info pointer null\n", __func__);
		return 0;
	}
	ret = memcpy_s(buf, PAGE_SIZE, ar_sensorlist_info,
		       sizeof(struct sensorlist_info));
	if (ret != EOK) {
		ctxhub_err("[%s], memcpy failed!\n", __func__);
		return 0;
	}
	return sizeof(struct sensorlist_info);
}

static ssize_t pedo_show_delay_info(struct device *dev,
				    struct device_attribute *attr,
				    char *buf)
{
	struct sensor_cookie *data =
		(struct sensor_cookie *)dev_get_drvdata(dev);
	struct pedometer_status *p_pedo_status = NULL;

	if (check_sensor_cookie(data) != RET_SUCC)
		return -EINVAL;

	p_pedo_status = get_pedo_status(data->shb_type);
	if (!p_pedo_status) {
		ctxhub_err("%s, get pedo status err!\n", __func__);
		return -EINVAL;
	}

	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "%d %d\n",
			p_pedo_status->interval, p_pedo_status->latency);
}

static ssize_t pedo_show_enable_info(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	struct sensor_cookie *data =
		(struct sensor_cookie *)dev_get_drvdata(dev);
	struct pedometer_status *p_pedo_status = NULL;

	if (check_sensor_cookie(data) != RET_SUCC)
		return -EINVAL;

	p_pedo_status = get_pedo_status(data->shb_type);
	if (!p_pedo_status) {
		ctxhub_err("%s, get pedo status err!\n", __func__);
		return -EINVAL;
	}

	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "%d\n",
			p_pedo_status->enable);
}

static global_device_attr(pedo_enable, 0660, pedo_show_enable_info, store_enable); // access permission
static global_device_attr(pedo_set_delay, 0660, pedo_show_delay_info, store_set_delay); // access permission

static global_device_attr(step_counter_sensorlist_info, 0660, show_pedo_sensorlist_info, NULL); // access permission
static global_device_attr(step_counter_wakeup_sensorlist_info, 0660, show_pedo_sensorlist_info, NULL); // access permission
static global_device_attr(step_detector_sensorlist_info, 0660, show_pedo_sensorlist_info, NULL); // access permission

pedo_attrs(step_counter);
pedo_attrs(step_counter_wakeup);
pedo_attrs(step_detector);

pedo_attrs_grp(step_counter);
pedo_attrs_grp(step_counter_wakeup);
pedo_attrs_grp(step_detector);

pedo_sensor_cookie(TAG_PEDOMETER, SENSORHUB_TYPE_STEP_COUNTER, true, "step_counter", step_counter);
pedo_sensor_cookie(TAG_PEDOMETER, SENSORHUB_TYPE_STEP_COUNTER_WAKEUP, true, "step_counter_wakeup", step_counter_wakeup);
pedo_sensor_cookie(TAG_PEDOMETER, SENSORHUB_TYPE_STEP_DETECTOR, true, "step_detector", step_detector);

static int pedo_debug_init(void)
{
	int ret;
	if (get_contexthub_dts_status())
		return -EINVAL;

	ret = sensor_file_register(&g_step_counter_sensor);
	if (ret != 0 )
		ctxhub_warn("create step counter node failed\n");

	ret = sensor_file_register(&g_step_counter_wakeup_sensor);
	if (ret != 0 )
		ctxhub_warn("create step counter wakeup node failed\n");

	ret = sensor_file_register(&g_step_detector_sensor);
	if (ret != 0 )
		ctxhub_warn("create step detector node failed\n");
	return 0;
}

static void pedo_debug_exit(void)
{
}

late_initcall_sync(pedo_debug_init);
module_exit(pedo_debug_exit);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("pedo debug driver");
MODULE_AUTHOR("Smart");
