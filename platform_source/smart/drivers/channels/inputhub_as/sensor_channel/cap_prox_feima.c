/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: acc debug demo.
 * Create: 2019-11-05
 */

#include "sensor_feima.h"

#include <securec.h>

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/kernel.h>

#include "common/common.h"
#include <platform_include/smart/linux/base/ap/protocol.h>
#include "platform_include/smart/linux/sensorhub_as.h"
#include "iomcu_log.h"
#include "device_manager.h"

#ifdef CONFIG_CONTEXTHUB_WATCH_FACTORY
#include <platform_include/basicplatform/linux/nve/nve_ap_kernel_interface.h>

#define CAP_OFFSET_NV_NUM 310
#define CAP_OFFSET_NV_SIZE 64
#define A96T356_CALIB_ARRAY_SIZE 32
#define CAP_STATUS_MAX 0xFF

enum cap_error_code {
	IPC_FAIL = 1,
	PROCESS_MODE_FAIL = 2,
};

int write_calibrate_data_to_nv(int nv_number, int nv_size,
	const char *nv_name, const char *temp);
int read_calibrate_data_from_nv(int nv_number, int nv_size, const char *nv_name,
	struct opt_nve_info_user *user_info);

/*
 * test node: query chip id
 */
static ssize_t show_chip_id(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;
	u8 r_buf[CAP_OFFSET_NV_SIZE] = {0};

	r_buf[0] = -1;
	ret = bus_type_process_factory(TAG_CAP_PROX, SUB_CMD_FACTORY_CAP_CHIP_ID,
		NULL, 0, r_buf, CAP_OFFSET_NV_SIZE);
	if (ret < 0) {
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-1\n");
	}

	if (r_buf[0] != 0) {
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-2\n");
	}

	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "0x%x\n", r_buf[1]);
}

/*
 * test node: calibration
 */
static ssize_t show_calibration(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i, ret;
	u8 r_buf[CAP_OFFSET_NV_SIZE + 1] = {0};  // only 32 byte avalible
	struct opt_nve_info_user user_info;

	r_buf[0] = -1;
	ret = bus_type_process_factory(TAG_CAP_PROX, SUB_CMD_FACTORY_CAP_CALIB,
		NULL, 0, r_buf, CAP_OFFSET_NV_SIZE);
	if (ret < 0) {
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-1\n");
	}

	if (r_buf[0] != 0) {
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-2\n");
	}

	ret = write_calibrate_data_to_nv(CAP_OFFSET_NV_NUM, CAP_OFFSET_NV_SIZE,
				"Csensor", &(r_buf[1]));
	if (ret != 0) {
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-3\n");
	}

	ret = read_calibrate_data_from_nv(CAP_OFFSET_NV_NUM, CAP_OFFSET_NV_SIZE,
				"Csensor", &user_info);
	if (ret != 0) {
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-4\n");
	}

	for (i = 0; i < A96T356_CALIB_ARRAY_SIZE; i++) {
		ctxhub_info("show_calibration value:%d, %d",
			r_buf[i + 1], user_info.nv_data[i]);
		if (r_buf[i + 1] != user_info.nv_data[i])
			return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-5\n");
	}

	for (i = 0; i < A96T356_CALIB_ARRAY_SIZE; i++)
		ret += snprintf_s(buf + i * 5, MAX_STR_SIZE - i * 5, MAX_STR_SIZE - i * 5 - 1,
		"0x%02x ", r_buf[i+1]);
	ret += snprintf_s(buf + A96T356_CALIB_ARRAY_SIZE * 5,
		MAX_STR_SIZE - A96T356_CALIB_ARRAY_SIZE * 5,
		MAX_STR_SIZE - A96T356_CALIB_ARRAY_SIZE * 5 - 1, "\n");

	return ret;
}

/*
 * test node: clear calibration
 */
static ssize_t show_clear_param(struct device *dev, struct device_attribute *attr,
	char *buf)
{
	int ret;
	u8 r_buf[CAP_OFFSET_NV_SIZE] = {0};  // only 32 byte avalible

	r_buf[0] = -1;
	ret = bus_type_process_factory(TAG_CAP_PROX, SUB_CMD_FACTORY_CAP_ERASE,
		NULL, 0, r_buf, CAP_OFFSET_NV_SIZE);
	if (ret < 0) {
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-1\n");
	}

	if (r_buf[0] != 0) {
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-2\n");
	}

	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "0\n");
}

/*
 * test node: query original sensor data
 */
static ssize_t show_get_data_factory(struct device *dev, struct device_attribute *attr,
	char *buf)
{
	int ret;
	u8 r_buf[CAP_OFFSET_NV_SIZE] = {0};
	int *cap_data = (int *)(&r_buf[1]);

	r_buf[0] = -1;
	ret = bus_type_process_factory(TAG_CAP_PROX, SUB_CMD_FACTORY_CAP_GET_DATA,
		NULL, 0, r_buf, CAP_OFFSET_NV_SIZE);
	if (ret < 0) {
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-1\n");
	}

	if (r_buf[0] != 0) {
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-2\n");
	}

	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "%d %d %d\n",
		cap_data[0], cap_data[1], cap_data[2]);
}

/*
 * test node: query sensor data
 */
static ssize_t show_get_data_brd(struct device *dev, struct device_attribute *attr,
	char *buf)
{
	int ret;
	u8 r_buf[CAP_OFFSET_NV_SIZE] = {0};
	u16 *cap_data = (u16 *)(&r_buf[1]);

	r_buf[0] = -1;
	ret = bus_type_process_factory(TAG_CAP_PROX, SUB_CMD_FACTORY_CAP_GET_DATA_BRD,
		NULL, 0, r_buf, CAP_OFFSET_NV_SIZE);
	if (ret < 0) {
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-1\n");
	}

	if (r_buf[0] != 0) {
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-2\n");
	}

	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "%d %d %d\n",
		cap_data[0], cap_data[1], cap_data[2]);
}

/*
 * test node: compare data in flash and NV
 */
static ssize_t show_cmp_param(struct device *dev, struct device_attribute *attr,
	char *buf)
{
	int i, ret;
	u8 r_buf[CAP_OFFSET_NV_SIZE] = {0};
	struct opt_nve_info_user user_info;

	r_buf[0] = -1;
	ret = bus_type_process_factory(TAG_CAP_PROX, SUB_CMD_FACTORY_CAP_CMP_PARAM,
				       NULL, 0, r_buf, CAP_OFFSET_NV_SIZE);
	if (ret < 0)
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-1\n");

	if (r_buf[0] != 0)
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-2\n");

	ret = read_calibrate_data_from_nv(CAP_OFFSET_NV_NUM, CAP_OFFSET_NV_SIZE,
					  "Csensor", &user_info);
	if (ret != 0)
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-4\n");

	for (i = 0; i < A96T356_CALIB_ARRAY_SIZE; i++) {
		ctxhub_info("show_cmp_param value:%d, %d",
			    r_buf[i + 1], user_info.nv_data[i]);
		if (r_buf[i + 1] != user_info.nv_data[i])
			return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-5\n");
	}
	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "0\n");
}

/*
 * test node: query sensor power mode
 */
static ssize_t show_power_mode(struct device *dev, struct device_attribute *attr,
	char *buf)
{
	int ret;
	u8 r_buf[CAP_OFFSET_NV_SIZE] = {0};

	r_buf[0] = -1;
	ret = bus_type_process_factory(TAG_CAP_PROX, SUB_CMD_FACTORY_CAP_GET_MODE,
		NULL, 0, r_buf, CAP_OFFSET_NV_SIZE);
	if (ret < 0) {
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-1\n");
	}

	if (r_buf[0] != 0) {
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-2\n");
	}

	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "%d\n", r_buf[1]);
}

/*
 * test node: set sensor power mode
 */
static ssize_t power_mode_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	uint64_t val = 0;
	int ret = 0;
	u8 r_buf[CAP_OFFSET_NV_SIZE] = {0};
	u8 s_buf[CAP_OFFSET_NV_SIZE] = {0};

	if (kstrtoull(buf, 10, &val) < 0)
		return -EINVAL;

	ctxhub_info("cap power_mode_store val:%lu", val);
	if (val > (uint64_t)CAP_STATUS_MAX)
		return -EINVAL;

	r_buf[0] = -1;
	s_buf[0] = (u8)val;
	ret = bus_type_process_factory(TAG_CAP_PROX, SUB_CMD_FACTORY_CAP_SET_MODE,
				       s_buf, 1, r_buf, CAP_OFFSET_NV_SIZE);
	if (ret < 0) {
		ctxhub_err("cap power_mode_store IPC_FAIL");
		return -IPC_FAIL;
	}

	if (r_buf[0] != 0) {
		ctxhub_err("cap power_mode_store PROCESS_MODE_FAIL");
		return -PROCESS_MODE_FAIL;
	}

	return count;
}

static DEVICE_ATTR(chip_id, 0660, show_chip_id, NULL); // access permission
static DEVICE_ATTR(calibration, 0660, show_calibration, NULL); // access permission
static DEVICE_ATTR(clear_param, 0660, show_clear_param, NULL); // access permission
static DEVICE_ATTR(get_data_factory, 0660, show_get_data_factory, NULL); // access permission
static DEVICE_ATTR(get_data_brd, 0660, show_get_data_brd, NULL); // access permission
static DEVICE_ATTR(cmp_param, 0660, show_cmp_param, NULL); // access permission
static DEVICE_ATTR(power_mode, 0660, show_power_mode, power_mode_store); // access permission
#endif
static DEVICE_ATTR(cap_prox_sensorlist_info, 0660, show_sensorlist_info, NULL); // access permission
static DEVICE_ATTR(enable, 0660, show_enable, store_enable); // access permission
static DEVICE_ATTR(set_delay, 0660, show_set_delay, store_set_delay); // access permission
static DEVICE_ATTR(info, 0440, show_info, NULL); // access permission
static DEVICE_ATTR(get_data, 0660, show_get_data, store_get_data); // access permission

// dev_attr_get_data use DEVICE_ATTR, can not add g_ prefix
static struct attribute *g_cap_prox_sensor_attrs[] = {
	&dev_attr_cap_prox_sensorlist_info.attr,
	&dev_attr_enable.attr,
	&dev_attr_set_delay.attr,
	&dev_attr_info.attr,
	&dev_attr_get_data.attr,
#ifdef CONFIG_CONTEXTHUB_WATCH_FACTORY
	&dev_attr_chip_id.attr,
	&dev_attr_calibration.attr,
	&dev_attr_clear_param.attr,
	&dev_attr_get_data_factory.attr,
	&dev_attr_get_data_brd.attr,
	&dev_attr_cmp_param.attr,
	&dev_attr_power_mode.attr,
#endif
	NULL,
};

static const struct attribute_group g_cap_prox_sensor_attrs_grp = {
	.attrs = g_cap_prox_sensor_attrs,
};

static struct sensor_cookie g_cap_prox_sensor = {
	.tag = TAG_CAP_PROX,
	.shb_type = SENSORHUB_TYPE_CAP_PROX,
	.name = "cap_prox_sensor",
	.attrs_group = &g_cap_prox_sensor_attrs_grp,
};

static int cap_prox_debug_init(void)
{
	int ret;

	if (get_contexthub_dts_status())
		return -EINVAL;
	ret = sensor_file_register(&g_cap_prox_sensor);
	if (ret != RET_SUCC)
		ctxhub_warn("%s, cap node register failed!\n", __func__);
	return 0;
}

static void cap_prox_debug_exit(void)
{
}

late_initcall_sync(cap_prox_debug_init);
module_exit(cap_prox_debug_exit);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("cap_prox debug driver");
MODULE_AUTHOR("Smart");
