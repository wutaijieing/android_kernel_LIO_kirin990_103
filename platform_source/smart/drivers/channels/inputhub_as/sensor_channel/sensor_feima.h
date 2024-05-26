/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
 * Description: sensor_feima.h.
 * Create: 2019/11/05
 */

#ifndef __SENSOR_FEIMA_H
#define __SENSOR_FEIMA_H

#include<linux/device.h>
#include<linux/sysfs.h>

#define global_device_attr(_name, _mode, _show, _store) \
	struct device_attribute g_dev_attr_##_name = __ATTR(_name, _mode, _show, _store)

struct sensor_cookie {
	int tag;
	int shb_type;
	bool is_customized;
	const char *name;
	const struct attribute_group *attrs_group;
	struct device *dev;
};


/*
 * Function    : sensor_file_register
 * Description : sensor customize file node create
 *             : will mount in /sys/class/sensors
 * Input       : [sensor] sensor info, tag, name, attrs
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int sensor_file_register(struct sensor_cookie *sensor);

/*
 * Function    : check_sensor_cookie
 * Description : check if param is valid
 * Input       : [data] save sensor name, tag, shb type
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int check_sensor_cookie(struct sensor_cookie *data);

/*
 * Function    : store_set_delay
 * Description : set delay command to mcu
 * Input       : [dev] file node pointer
 *             : [attr] reserverd, not use
 *             : [buf] input param
 *             : [size] input length
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
ssize_t store_set_delay(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t size);

/*
 * Function    : store_enable
 * Description : set enable/disable command to mcu
 * Input       : [dev] file node pointer
 *             : [attr] reserverd, not use
 *             : [buf] input param
 *             : [size] input length
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
ssize_t store_enable(struct device *dev, struct device_attribute *attr,
		     const char *buf, size_t size);

/*
 * Function    : show_sensorlist_info
 * Description : show sensor list info
 * Input       : [dev] file node pointer
 *             : [attr] reserverd, not use
 *             : [buf] buf to save sensor list info
 * Output      : none
 * Return      : bytes that write to buf
 */
ssize_t show_sensorlist_info(struct device *dev,
		struct device_attribute *attr, char *buf);


/*
 * Function    : show_enable
 * Description : show sensor enable status
 * Input       : [dev] file node pointer
 *             : [attr] reserverd, not use
 *             : [buf] buf to save sensor enable status
 * Output      : none
 * Return      : bytes that write to buf
 */


ssize_t show_enable(struct device *dev,
			struct device_attribute *attr,
			char *buf);


/*
 * Function    : show_set_delay
 * Description : show sensor delay status
 * Input       : [dev] file node pointer
 *             : [attr] reserverd, not use
 *             : [buf] buf to save sensor delay status
 * Output      : none
 * Return      : bytes that write to buf
 */

ssize_t show_set_delay(struct device *dev,
			struct device_attribute *attr,
			char *buf);

/*
 * Function    : show_info
 * Description : show sensor info
 * Input       : [dev] file node pointer
 *             : [attr] reserverd, not use
 *             : [buf] buf to save sensor list info
 * Output      : none
 * Return      : bytes that write to buf
 */

ssize_t show_info(struct device *dev,
			struct device_attribute *attr,
			char *buf);

/*
 * Function    : show_sensorlist_info
 * Description : show sensor list info
 * Input       : [dev] file node pointer
 *             : [attr] reserverd, not use
 *             : [buf] buf to save sensor list info
 * Output      : none
 * Return      : bytes that write to buf
 */

ssize_t show_get_data(struct device *dev,
			struct device_attribute *attr,
			char *buf);

/*
 * Function    : store_get_data
 * Description : set delay
 * Input       : [dev] file node pointer
 *             : [attr] reserverd, not use
 *             : [buf] buf to save sensor list info
 * Output      : none
 * Return      : bytes that write to buf
 */

ssize_t store_get_data(struct device *dev,
			struct device_attribute *attr,
			const char *buf,
			size_t size);

#endif /* __SENSOR_FEIMA_H */
