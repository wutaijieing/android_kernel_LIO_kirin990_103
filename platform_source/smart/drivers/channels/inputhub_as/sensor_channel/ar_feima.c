/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
 * Description: ar debug.
 * Create: 2020/03/05
 */

#include "sensor_feima.h"

#include <securec.h>

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include "ar.h"
#include "common/common.h"
#include <platform_include/smart/linux/base/ap/protocol.h>
#include "iomcu_log.h"
#include "platform_include/smart/linux/sensorhub_as.h"
#include "device_manager.h"


#define ar_attrs(TYPE) \
static struct attribute *g_##TYPE##_sensor_attrs[] = { \
	&g_dev_attr_##TYPE##_sensorlist_info.attr, \
	&g_dev_attr_ar_enable.attr, \
	&g_dev_attr_ar_set_delay.attr, \
	&g_dev_attr_info.attr, \
	&g_dev_attr_get_data.attr, \
	NULL, \
}

#define ar_attrs_grp(TYPE) \
static const struct attribute_group g_##TYPE##_sensor_attrs_grp = { \
	.attrs = g_##TYPE##_sensor_attrs, \
}

#define ar_sensor_cookie(TAG, SHB_TYPE, IS_CUSTOMIZED, NAME, TYPE) \
static struct sensor_cookie g_##TYPE##_sensor = { \
	.tag = TAG, \
	.shb_type = SHB_TYPE, \
	.is_customized = IS_CUSTOMIZED, \
	.name = NAME, \
	.attrs_group = &g_##TYPE##_sensor_attrs_grp, \
}

static ssize_t show_ar_sensorlist_info(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	int ret;
	struct sensor_cookie *data =
		(struct sensor_cookie *)dev_get_drvdata(dev);
	struct sensorlist_info *ar_sensorlist_info = NULL;

	if (check_sensor_cookie(data) != RET_SUCC)
		return -EINVAL;

	ar_sensorlist_info = get_ar_sensor_list_info(data->shb_type);
	if (!ar_sensorlist_info) {
		ctxhub_err("[%s], get ar sensor info pointer null\n", __func__);
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

static ssize_t ar_show_delay_info(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	struct sensor_cookie *data =
		(struct sensor_cookie *)dev_get_drvdata(dev);
	struct ar_status *p_ar_status = NULL;

	if (check_sensor_cookie(data) != RET_SUCC)
		return -EINVAL;

	p_ar_status = get_ar_status(data->shb_type);
	if (!p_ar_status) {
		ctxhub_err("%s, get ar status err!\n", __func__);
		return -EINVAL;
	}

	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "%d\n",
			p_ar_status->report_latency);
}

static ssize_t ar_show_enable_info(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	struct sensor_cookie *data =
		(struct sensor_cookie *)dev_get_drvdata(dev);
	struct ar_status *p_ar_status = NULL;

	if (check_sensor_cookie(data) != RET_SUCC)
		return -EINVAL;

	p_ar_status = get_ar_status(data->shb_type);
	if (!p_ar_status) {
		ctxhub_err("%s, get ar status err!\n", __func__);
		return -EINVAL;
	}

	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "%d\n",
			(int)p_ar_status->enable);
}

static global_device_attr(ar_enable, 0660, ar_show_enable_info, store_enable);
static global_device_attr(ar_set_delay, 0660, ar_show_delay_info, store_set_delay);

static global_device_attr(ar_walking_sensorlist_info, 0660, show_ar_sensorlist_info, NULL);
static global_device_attr(ar_running_sensorlist_info, 0660, show_ar_sensorlist_info, NULL);
static global_device_attr(ar_stationary_sensorlist_info, 0660, show_ar_sensorlist_info, NULL);
static global_device_attr(ar_wrist_down_sensorlist_info, 0660, show_ar_sensorlist_info, NULL);
static global_device_attr(ar_motion_sensorlist_info, 0660, show_ar_sensorlist_info, NULL);
static global_device_attr(ar_offbody_sensorlist_info, 0660, show_ar_sensorlist_info, NULL);

// dev_attr_get_data use global_device_attr, can not add g_ prefix
ar_attrs(ar_walking);
ar_attrs(ar_running);
ar_attrs(ar_stationary);
ar_attrs(ar_wrist_down);
ar_attrs(ar_motion);
ar_attrs(ar_offbody);

ar_attrs_grp(ar_walking);
ar_attrs_grp(ar_running);
ar_attrs_grp(ar_stationary);
ar_attrs_grp(ar_wrist_down);
ar_attrs_grp(ar_motion);
ar_attrs_grp(ar_offbody);

ar_sensor_cookie(TAG_AR, SENSORHUB_TYPE_AR_MOTION, true, "ar_motion_sensor", ar_motion);
ar_sensor_cookie(TAG_AR, SENSORHUB_TYPE_AR_WALKING, true, "ar_walking_sensor", ar_walking);
ar_sensor_cookie(TAG_AR, SENSORHUB_TYPE_AR_RUNNING, true, "ar_running_sensor", ar_running);
ar_sensor_cookie(TAG_AR, SENSORHUB_TYPE_AR_WRIST_DOWN, true, "ar_wrist_down_sensor", ar_wrist_down);
ar_sensor_cookie(TAG_AR, SENSORHUB_TYPE_AR_STATIONARY, true, "ar_stationary_sensor", ar_stationary);
ar_sensor_cookie(TAG_AR, SENSORHUB_TYPE_AR_OFFBODY, true, "ar_offbody_sensor", ar_offbody);

static int ar_debug_init(void)
{
	int ret;

	if (get_contexthub_dts_status())
		return -EINVAL;

	ret = sensor_file_register(&g_ar_motion_sensor);
	if (ret != 0)
		ctxhub_warn("create motion node failed!\n");

	ret = sensor_file_register(&g_ar_walking_sensor);
	if (ret != 0)
		ctxhub_warn("create walking node failed!\n");

	ret = sensor_file_register(&g_ar_running_sensor);
	if (ret != 0)
		ctxhub_warn("create running node failed!\n");

	ret = sensor_file_register(&g_ar_wrist_down_sensor);
	if (ret != 0)
		ctxhub_warn("create wrist down node failed!\n");

	ret = sensor_file_register(&g_ar_stationary_sensor);
	if (ret != 0)
		ctxhub_warn("create stationary node failed!\n");

	ret = sensor_file_register(&g_ar_offbody_sensor);
	if (ret != 0)
		ctxhub_warn("create offbody node failed!\n");
	return 0;
}

static void ar_debug_exit(void)
{
}

late_initcall_sync(ar_debug_init);
module_exit(ar_debug_exit);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ar debug driver");
MODULE_AUTHOR("Smart");
