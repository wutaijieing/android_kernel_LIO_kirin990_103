/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019. All rights reserved.
 * Description:dft channel.c
 * Create:2019.09.22
 */
#include <securec.h>

#include <linux/module.h>
#include <linux/types.h>
#include <platform_include/basicplatform/linux/nve/nve_ap_kernel_interface.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <linux/completion.h>
#include <linux/hwspinlock.h>

#include <platform_include/smart/linux/base/ap/protocol.h>
#include "platform_include/smart/linux/iomcu_dump.h"
#include "platform_include/smart/linux/iomcu_pm.h"
#include "common/common.h"
#include "bus_interface.h"
#include "iomcu_log.h"


#define DEBUG_DATA_LENGTH 10
#define I2C_BUF_SIZE 2
static uint8_t g_debug_read_data_buf[DEBUG_DATA_LENGTH];
static uint8_t g_i2c_rw16_data_buf[I2C_BUF_SIZE]; /* 2: i2c data buf length */

struct bus_rw_info {
	uint8_t bus_num;
	uint8_t i2c_address;
	uint8_t reg_add;
	uint8_t len;
	uint8_t rw;
};

static void extract_bus_rw_info(uint64_t val, struct bus_rw_info *info)
{
	// param check outside

	/* ##(bus_num)##(i2c_addr)##(reg_addr)##(len) */
	info->bus_num = (val >> 40) & 0xff;
	info->i2c_address = (val >> 32) & 0xff;
	info->reg_add = (val >> 24) & 0xff;
	info->len = (val >> 16) & 0xff;
	if (info->len > DEBUG_DATA_LENGTH - 1) {
		ctxhub_err("len exceed %d\n", info->len);
		info->len = DEBUG_DATA_LENGTH - 1;
	}
	info->rw = (val >> 8) & 0xff;
}

static ssize_t i2c_rw_pi(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	uint64_t val = 0;
	int ret = 0;
	struct bus_rw_info info;
	uint8_t buf_temp[DEBUG_DATA_LENGTH] = { 0 };

	if (kstrtoull(buf, 16, &val) < 0) // convert a string to an unsigned int
		return -EINVAL;

	extract_bus_rw_info(val, &info);
	buf_temp[0] = info.reg_add;
	buf_temp[1] = (uint8_t)(val & 0xff);

	if (info.rw)
		ret = mcu_i2c_rw(info.bus_num, info.i2c_address, &buf_temp[0], 1, &buf_temp[1], info.len); // trans one data and recv one data
	else
		ret = mcu_i2c_rw(info.bus_num, info.i2c_address, buf_temp, 2, NULL, 0); // trans two data

	if (ret < 0)
		ctxhub_err("oper %d(1/32:r 0/31:w) i2c reg fail\n", info.rw);
	if (info.rw) {
		ctxhub_err("i2c reg %x value %x %x %x %x\n", info.reg_add,
			buf_temp[1], buf_temp[2], buf_temp[3], buf_temp[4]);
		ret = memcpy_s(g_debug_read_data_buf, DEBUG_DATA_LENGTH, &buf_temp[1], info.len);
		if (ret != EOK) {
			ctxhub_err("%s memcpy_s error %d\n", __func__, ret);
			return ret;
		}
	}
	return count;
}

static ssize_t i2c_rw_pi_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	unsigned int i;
	unsigned int len = 0;
	char *p = buf;
	int ret;

	if (buf == NULL)
		return -EINVAL;

	/* 4,5,6 used to show i2c debug data */
	for (i = 0; i < DEBUG_DATA_LENGTH; i++) {
		/*  max src str len is 6 */
		ret = snprintf_s(p, PAGE_SIZE - len, 6, "0x%x,",
			g_debug_read_data_buf[i]);
		if (ret < 0) {
			ctxhub_err("[%s], snprintf_s failed!\n", __func__);
			return ret;
		}
		if (g_debug_read_data_buf[i] > 0xf) {
			p += 5; /* value > 0xf, str len is 5 */
			len += 5;
		} else {
			p += 4; /* value > 0xf, str len is 4 */
			len += 4;
		}
	}

	p = buf;
	*(p + len - 1) = 0;

	return len;
}

static DEVICE_ATTR(i2c_rw, 0660, i2c_rw_pi_show, i2c_rw_pi);

static void extract_bus_rw16_info(uint64_t val, struct bus_rw_info *info)
{
	// param check outside

	/* ##(bus_num)##(i2c_addr)##(reg_addr)##(len) */
	info->bus_num = (val >> 48) & 0xff;
	info->i2c_address = (val >> 40) & 0xff;
	info->reg_add = (val >> 32) & 0xff;
	info->len = (val >> 24) & 0xff;
	if (info->len > 2) { // only need U16
		ctxhub_err("len exceed %d\n", info->len);
		info->len = 2; // only need U16
	}
	info->rw = (val >> 16) & 0xff;
}

static ssize_t i2c_rw16_pi(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	uint64_t val = 0;
	int ret = 0;
	struct bus_rw_info info;
	uint8_t buf_temp[3] = { 0 };

	if (kstrtoull(buf, 16, &val) < 0) // convert a string to an unsinged int
		return -EINVAL;

	extract_bus_rw16_info(val, &info);

	buf_temp[0] = info.reg_add;
	buf_temp[1] = (uint8_t)(val >> 8);
	buf_temp[2] = (uint8_t)(val & 0xff);

	if (info.rw)
		ret = mcu_i2c_rw(info.bus_num, info.i2c_address, buf_temp, 1, &buf_temp[1], (uint32_t)info.len); // trans one data and recv two
	else
		ret = mcu_i2c_rw(info.bus_num, info.i2c_address, buf_temp, 1 + info.len, NULL, 0); // trans all data
	if (ret < 0)
		ctxhub_err("oper %d(1:r 0:w) i2c reg fail\n", info.rw);

	if (info.rw) {
		ctxhub_err("i2c reg %x value %x %x\n",
			info.reg_add, buf_temp[1], buf_temp[2]);
		ret = memcpy_s(g_i2c_rw16_data_buf, I2C_BUF_SIZE, &buf_temp[1], 2);
		if (ret != EOK) {
			ctxhub_err("%s memcpy_s error %d\n", __func__, ret);
			return ret;
		}
	}
	return count;
}

static ssize_t i2c_rw16_pi_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	char *p = buf;

	if (buf == NULL)
		return -EINVAL;
	/* 8 is str length */
	return snprintf_s(p, PAGE_SIZE, 8, "0x%02x%02x\n",
		g_i2c_rw16_data_buf[0], g_i2c_rw16_data_buf[1]);
}
static DEVICE_ATTR(i2c_rw16, 0660, i2c_rw16_pi_show, i2c_rw16_pi); // set device permission


void __weak disable_sensors_when_suspend(void)
{
}

void __weak enable_sensors_when_resume(void)
{
}

static ssize_t attr_iom3_sr_test_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long long val = 0;
	unsigned long long times;

	if (kstrtoull(buf, 10, &val) < 0) // convert a string to an unsinged int
		return -EINVAL;

	times = val;
	if (val > 0) {
		for (; val > 0; val--) {
			disable_sensors_when_suspend();
			tell_ap_status_to_mcu(ST_SLEEP);
			msleep(2); // sleep 2 ms
			tell_ap_status_to_mcu(ST_WAKEUP);
			enable_sensors_when_resume();
		}
		if (times % 2) { // the second time, do something
			tell_ap_status_to_mcu(ST_SLEEP);
			enable_sensors_when_resume();
		}
	}
	return count;
}

static DEVICE_ATTR(iom3_sr_test, 0660, NULL, attr_iom3_sr_test_store);

static ssize_t show_iom3_sr_status(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf_s(buf, PAGE_SIZE, MAX_STR_SIZE, "%s\n",
		(get_iom3_sr_status() == ST_SLEEP) ? "ST_SLEEP" : "ST_WAKEUP");
}
static DEVICE_ATTR(iom3_sr_status, 0660, show_iom3_sr_status, NULL);

static ssize_t start_iom3_recovery(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	ctxhub_info("%s +\n", __func__);
	iom3_need_recovery(SENSORHUB_USER_MODID, SH_FAULT_USER_DUMP);
	ctxhub_info("%s -\n", __func__);
	return size;
}

static DEVICE_ATTR(iom3_recovery, 0660, NULL, start_iom3_recovery); // set device permission

static struct attribute *g_smart_sensor_attributes[] = {
	&dev_attr_i2c_rw.attr,
	&dev_attr_i2c_rw16.attr,
	&dev_attr_iom3_recovery.attr,
	&dev_attr_iom3_sr_test.attr,
	&dev_attr_iom3_sr_status.attr,
	NULL
};

static const struct attribute_group g_smart_sensor_node = {
	.attrs = g_smart_sensor_attributes,
};

static struct platform_device g_smart_sensor_input_info = {
	.name = "smart_sensor",
	.id = -1,
};

static int __init smart_sensor_input_info_init(void)
{
	int ret;

	if (get_contexthub_dts_status())
		return -EINVAL;

	ctxhub_info("[%s] ++\n", __func__);
	ret = platform_device_register(&g_smart_sensor_input_info);
	if (ret) {
		ctxhub_err("%s: smart sensor platform_device_register failed, ret:%d\n",
			__func__, ret);
		goto REGISTER_ERR;
	}

	ret = sysfs_create_group(&g_smart_sensor_input_info.dev.kobj, &g_smart_sensor_node);
	if (ret) {
		ctxhub_err("%s: sysfs_create_group error ret =%d\n",
			__func__, ret);
		goto SYSFS_CREATE_CGOUP_ERR;
	}
	ctxhub_info("[%s] --\n", __func__);
	return 0;
SYSFS_CREATE_CGOUP_ERR:
	platform_device_unregister(&g_smart_sensor_input_info);
REGISTER_ERR:
	return ret;
}

late_initcall_sync(smart_sensor_input_info_init);

MODULE_ALIAS("platform:contexthub"MODULE_NAME);
MODULE_LICENSE("GPL v2");
