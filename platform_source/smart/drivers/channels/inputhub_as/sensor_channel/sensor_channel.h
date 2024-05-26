/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
 * Description: sensor_channel.h.
 * Create: 2019/11/05
 */

#ifndef __SENSOR_CHANNEL_H
#define __SENSOR_CHANNEL_H

#include <linux/completion.h>
#include <linux/mutex.h>

#include "platform_include/smart/linux/sensorhub_as.h"
#include <platform_include/smart/linux/base/ap/protocol.h>
#include "iomcu_route.h"


// this is android standard
#define SENSOR_FLAG_ONE_SHOT_MODE 4
#define SENSOR_FLAG_ON_CHANGE_MODE 2
#define SENSOR_FLAG_SPECIAL_MODE 6

#define REPORTING_MODE_MASK 0xE

#define SENSOR_LIST_NUM 50

#define SENSOR_WAKEUP_FLAG 1

/*
 * for interface not use sensor channel universal process
 */
struct customized_interface {
	int (*enable)(int shb_type, bool enable);
	int (*setdelay)(int shb_type, int period, int timeout, bool is_batch);
	int (*report_event)(struct inputhub_route_table *route_item, const struct pkt_header *head);
	int (*flush)(int shb_type);
};

/*
 * ap sensor callback structure
 */
struct sensor_operation {
	int (*enable)(bool enable);
	int (*setdelay)(int ms);
};

/*
 * data structure for sensor data upload to sensor hal
 */
struct sensor_data {
	unsigned short type;
	unsigned short length;
	union {
		int value[16]; /* xyza... */
		struct {
			int serial;
			int data_type;
			int sensor_type;
			int info[13];
		};
	};
};

/*
 * structure for get sensor data from iomcu
 * you can get cat sensor data by
 * cat /sys/class/sensors/xxx_sensor/get_data
 */
struct t_sensor_get_data {
	atomic_t reading;
	struct completion complete;
	struct sensor_data data;
};

/*
 * record sensor info for reume, recovery or test
 */
struct sensor_status {
	/*
	 * record whether sensor is in activate status,
	 * already opened and setdelay
	 */
	struct t_sensor_get_data get_data[SENSORHUB_TYPE_END];
	bool have_send_calibrate_data[TAG_SENSOR_END];
};

/*
 * Function    : report_sensor_event
 * Description : write sensor event to kernel buffer
 * Input       : [shb_type] key, identify an app of sensor
 *             : [value] sensor data
 *             : [length] sensor data length
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int report_sensor_event(int shb_type, const int value[], unsigned int length);

/*
 * Function    : add_sensorlist
 * Description : add sensor list info id to array
 * Input       : [tag] key, identify an app of sensor
 *             : [sensor_list_info_id] id for sensor hal use
 * Output      : none
 * Return      : none
 */
void add_sensorlist(int32_t tag, uint16_t sensor_list_info_id);

/*
 * Function    : sensor_get_data
 * Description : save report data to global var
 * Input       : [data] report data
 * Output      : none
 * Return      : none
 */
void sensor_get_data(struct sensor_data *data);

/*
 * Function    : is_one_shot
 * Description : check one shot mode according to flags
 * Input       : [flags] sensor flag
 * Output      : none
 * Return      : true is one shot, else is not
 */
bool is_one_shot(u32 flags);

/*
 * Function    : register_customized_interface
 * Description : register customized interface to sensor channel
 * Input       : [shb_type] sensor identify, use in sensor hal
 *             : [tag] sensor identify, use in sensor kernel
 *             : [interface] struct for customized interface
 * Output      : none
 * Return      : 0 is ok, otherwise fail
 */
int register_customized_interface(int shb_type, int tag, const struct customized_interface *interface);

/*
 * Function    : unregister_customized_interface
 * Description : unregister customized interface to sensor channel
 * Input       : [shb_type] sensor identify, use in sensor hal
 * Output      : none
 * Return      : 0 is ok, otherwise fail
 */
int unregister_customized_interface(int shb_type);

/*
 * Function    : inputhub_sensor_channel_enable
 * Description : enable command send to sensorhub
 * Input       : [shb_type] sensor identify, use in sensor hal
 *             : [tag] sensor identify, use in sensor kernel
 *             : [enable] enable or disable
 * Output      : none
 * Return      : 0 is ok, otherwise fail
 */
int inputhub_sensor_channel_enable(int shb_type, int tag, bool enable);

/*
 * Function    : inputhub_sensor_channel_setdelay
 * Description : batch param send to sensorhub
 * Input       : [tag] sensor identify, use in sensor kernel
 *             : [para] include period, timeout, shb_type; set delay will not use timeout
 *             : [is_batch] true is batch, false is set delay
 * Output      : none
 * Return      : 0 is ok, otherwise fail
 */
int inputhub_sensor_channel_setdelay(int tag, const struct ioctl_para *para, bool is_batch);

/*
 * Function    : get_sensor_list_info
 * Description : get info of sensor list
 * Input       : [buf]:buff waitting to fill sensor info
 * Output      : None
 * Return      : length of valid data
 */
ssize_t get_sensor_list_info(char *buf);

/*
 * Function    : get_sensors_status_data
 * Description : get stats of sensor
 * Input       : [shb_type]:shb type
 * Output      : None
 * Return      : pointer of sensor list
 */
struct t_sensor_get_data* get_sensors_status_data(int shb_type);

#endif /* __SENSOR_CHANNEL_H */
