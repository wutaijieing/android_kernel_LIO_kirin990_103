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
#include <platform_include/basicplatform/linux/nve/nve_ap_kernel_interface.h>
#include "common/common.h"
#include <platform_include/smart/linux/base/ap/protocol.h>
#include "platform_include/smart/linux/sensorhub_as.h"
#include "iomcu_log.h"
#include "device_manager.h"


#ifdef CONFIG_CONTEXTHUB_WATCH_FACTORY
#define GYRO_OFFSET_NV_NUM 341
#define GYRO_OFFSET_NV_SIZE 72
#define GYRO_CALIB_ARRAY_SIZE 12

int write_calibrate_data_to_nv(int nv_number, int nv_size,
	const char *nv_name, const char *temp);
int read_calibrate_data_from_nv(int nv_number, int nv_size, const char *nv_name,
	struct opt_nve_info_user *user_info);

#define GYRO_STATUS_MAX 0xFF
enum gyro_error_code {
	IPC_FAIL = 1,
	PROCESS_MODE_FAIL = 2,
};

/*
 * test node: query chip id
 */
static ssize_t show_chip_id(struct device *dev, struct device_attribute *attr,
	char *buf)
{
	int ret;
	u8 r_buf[MAX_TX_RX_LEN] = {0};

	r_buf[0] = -1;
	ret = bus_type_process_factory(TAG_ACCEL, SUB_CMD_FACTORY_ACC_CHIP_ID,
		NULL, 0, r_buf, MAX_TX_RX_LEN);
	if (ret < 0)
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-1\n");

	if (r_buf[0] != 0)
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-2\n");

	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "0x%x\n", r_buf[1]);
}

/*
 * test node: interrupt sensor
 */
static ssize_t show_test_int(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "0\n");
}

/*
 * test node: query sensor power mode
 */
static ssize_t show_power_mode(struct device *dev, struct device_attribute *attr,
	char *buf)
{
	int ret;
	u8 r_buf[MAX_TX_RX_LEN] = {0};

	r_buf[0] = -PROCESS_MODE_FAIL;
	ret = bus_type_process_factory(TAG_GYRO, SUB_CMD_FACTORY_GYRO_GET_MODE,
		NULL, 0, r_buf, MAX_TX_RX_LEN);

	if (ret < 0)
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-1\n");

	if (r_buf[0] != 0)
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-2\n");

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
	u8 r_buf[MAX_TX_RX_LEN] = {0};
	u8 s_buf[MAX_TX_RX_LEN] = {0};

	if (kstrtoull(buf, 10, &val) < 0) //convert string to UINT
		return -EINVAL;

	ctxhub_info("power_mode_store val:%lu", val);
	if (val > (uint64_t)GYRO_STATUS_MAX)
		return -EINVAL;

	r_buf[0] = -PROCESS_MODE_FAIL;
	s_buf[0] = (u8)val;
	ret = bus_type_process_factory(TAG_GYRO, SUB_CMD_FACTORY_GYRO_SET_MODE,
		s_buf, 1, r_buf, MAX_TX_RX_LEN);
	if (ret < 0) {
		ctxhub_err("power_mode_store IPC_FAIL");
		return -IPC_FAIL;
	}

	if (r_buf[0] != 0) {
		ctxhub_err("power_mode_store PROCESS_MODE_FAIL");
		return -PROCESS_MODE_FAIL;
	}
	return count;
}

/*
 * test node: query sensor ODR
 */
static ssize_t show_odr_value(struct device *dev, struct device_attribute *attr,
	char *buf)
{
	int ret;
	u8 r_buf[MAX_TX_RX_LEN] = {0};

	r_buf[0] = -PROCESS_MODE_FAIL;
	ret = bus_type_process_factory(TAG_GYRO, SUB_CMD_FACTORY_GYRO_GET_ODR,
		NULL, 0, r_buf, MAX_TX_RX_LEN);
	if (ret < 0)
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-1\n");

	if (r_buf[0] != 0)
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-2\n");

	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "%d\n", r_buf[1]);
}

/*
 * test node: set sensor ODR
 */
static ssize_t odr_value_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	uint64_t val = 0;
	int ret = 0;
	u8 r_buf[MAX_TX_RX_LEN] = {0};
	u8 s_buf[MAX_TX_RX_LEN] = {0};

	if (kstrtoull(buf, 10, &val) < 0) //convert string to UINT
		return -EINVAL;

	ctxhub_info("power_mode_store val:%lu", val);
	if (val > (uint64_t)GYRO_STATUS_MAX)
		return -EINVAL;

	r_buf[0] = -PROCESS_MODE_FAIL;
	s_buf[0] = (u8)val;
	ret = bus_type_process_factory(TAG_GYRO, SUB_CMD_FACTORY_GYRO_SET_ODR,
		s_buf, 1, r_buf, MAX_TX_RX_LEN);
	if (ret < 0) {
		ctxhub_err("power_mode_store IPC_FAIL");
		return -IPC_FAIL;
	}

	if (r_buf[0] != 0) {
		ctxhub_err("power_mode_store PROCESS_MODE_FAIL");
		return -PROCESS_MODE_FAIL;
	}

	return count;
}

/*
 * test node: clear calibration
 */
static ssize_t show_clr_calibration(struct device *dev, struct device_attribute *attr,
	char *buf)
{
	int ret;
	char r_buf[GYRO_OFFSET_NV_SIZE] = { 0 };

	ret = write_calibrate_data_to_nv(GYRO_OFFSET_NV_NUM, GYRO_OFFSET_NV_SIZE,
		"GYRO", (const char *)r_buf);
	if (ret != 0)
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-3\n");
	return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "0\n");
}

/*
 * test node: query calibration data
 */
static ssize_t show_set_calibration(struct device *dev, struct device_attribute *attr,
	char *buf)
{
	int i, ret;
	u8 r_buf[GYRO_OFFSET_NV_SIZE + 1] = { 0 };
	struct opt_nve_info_user user_info;

	r_buf[0] = -2; /* -2: ipc fail */
	ret = bus_type_process_factory(TAG_GYRO, SUB_CMD_SAVE_GYRO_CALIBRATION,
		NULL, 0, r_buf, GYRO_OFFSET_NV_SIZE);
	if (ret < 0)
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-1\n");

	if (r_buf[0] != 0)
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-2\n");

	ret = write_calibrate_data_to_nv(GYRO_OFFSET_NV_NUM, GYRO_OFFSET_NV_SIZE,
		"GYRO", &(r_buf[1]));
	if (ret != 0)
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-3\n");

	ret = read_calibrate_data_from_nv(GYRO_OFFSET_NV_NUM, GYRO_OFFSET_NV_SIZE,
		"GYRO", &user_info);
	if (ret != 0)
		return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-4\n");

	for (i = 0; i < GYRO_CALIB_ARRAY_SIZE; i++) {
		ctxhub_info("show value:%d, %d", r_buf[i + 1], user_info.nv_data[i]);
		if (r_buf[i + 1] != user_info.nv_data[i])
			return snprintf_s(buf, MAX_STR_SIZE, MAX_STR_SIZE - 1, "-5\n");
	}

	for (i = 0; i < GYRO_CALIB_ARRAY_SIZE; i++)
		ret += snprintf_s(buf + i * 5, MAX_STR_SIZE - i * 5, MAX_STR_SIZE - i * 5 - 1,
			"0x%02x ", r_buf[i+1]);
	ret += snprintf_s(buf + GYRO_CALIB_ARRAY_SIZE * 5, MAX_STR_SIZE - GYRO_CALIB_ARRAY_SIZE * 5,
		MAX_STR_SIZE - GYRO_CALIB_ARRAY_SIZE * 5 - 1, "\n");
	return ret;
}

static DEVICE_ATTR(chip_id, 0660, show_chip_id, NULL); // access permission
static DEVICE_ATTR(test_int, 0660, show_test_int, NULL); // access permission
static DEVICE_ATTR(set_calibration, 0664, show_set_calibration, NULL); // access permission
static DEVICE_ATTR(clr_calibration, 0664, show_clr_calibration, NULL); // access permission
static DEVICE_ATTR(power_mode, 0664, show_power_mode, power_mode_store); // access permission
static DEVICE_ATTR(odr_value, 0664, show_odr_value, odr_value_store); // access permission
#endif

static DEVICE_ATTR(gyro_sensorlist_info, 0660, show_sensorlist_info, NULL); // access permission
static DEVICE_ATTR(enable, 0660, show_enable, store_enable); // access permission
static DEVICE_ATTR(set_delay, 0660, show_set_delay, store_set_delay); // access permission
static DEVICE_ATTR(info, 0440, show_info, NULL); // access permission
static DEVICE_ATTR(get_data, 0660, show_get_data, store_get_data); // access permission

/* dev_attr_get_data use DEVICE_ATTR, can not add g_ prefix */
static struct attribute *g_gyro_sensor_attrs[] = {
	&dev_attr_gyro_sensorlist_info.attr,
	&dev_attr_enable.attr,
	&dev_attr_set_delay.attr,
	&dev_attr_info.attr,
	&dev_attr_get_data.attr,
#ifdef CONFIG_CONTEXTHUB_WATCH_FACTORY
	&dev_attr_chip_id.attr,
	&dev_attr_test_int.attr,
	&dev_attr_set_calibration.attr,
	&dev_attr_clr_calibration.attr,
	&dev_attr_power_mode.attr,
	&dev_attr_odr_value.attr,
#endif
	NULL,
};

static const struct attribute_group g_gyro_sensor_attrs_grp = {
	.attrs = g_gyro_sensor_attrs,
};

static struct sensor_cookie g_gyro_sensor = {
	.tag = TAG_GYRO,
	.name = "gyro_sensor",
	.shb_type = SENSORHUB_TYPE_GYROSCOPE,
	.attrs_group = &g_gyro_sensor_attrs_grp,
};

static int gyro_debug_init(void)
{
	int ret;

	if (get_contexthub_dts_status())
		return -EINVAL;
	ret = sensor_file_register(&g_gyro_sensor);
	if (ret != RET_SUCC)
		ctxhub_warn("%s, gyrp node register failed!\n", __func__);
	return 0;
}

static void gyro_debug_exit(void)
{
}

late_initcall_sync(gyro_debug_init);
module_exit(gyro_debug_exit);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("gyro debug driver");
MODULE_AUTHOR("Smart");
