/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: device_common.h.
 * Create: 2019-11-05
 */
#ifndef __DEVICE_COMMON_H
#define __DEVICE_COMMON_H

#include <platform_include/basicplatform/linux/nve/nve_ap_kernel_interface.h>
#include <platform_include/smart/linux/inputhub_as/device_manager.h>
#include <platform_include/smart/linux/inputhub_as/bus_interface.h>

typedef u16 gpio_num_type;

#define MAX_SENSOR_CALIBRATE_DATA_LENGTH     60

#define NV_WRITE_TAG 0
#define NV_READ_TAG  1

#define SENSOR_PLATFORM_EXTEND_DATA_SIZE 50

#define TP_COORDINATE_THRESHOLD 4

#define DEF_SENSOR_COM_SETTING \
{\
	.bus_type = TAG_I2C,\
	.bus_num = 0,\
	.disable_sample_thread = 0,\
	{ .data = 0 } \
}

#define MAX_TX_RX_LEN 32
struct detect_word {
	struct sensor_combo_cfg cfg;
	u32 tx_len;
	u8 tx[MAX_TX_RX_LEN];
	u32 rx_len;
	u8 rx_msk[MAX_TX_RX_LEN];
	u32 exp_n;
	u8 rx_exp[MAX_TX_RX_LEN];
};

/*
 * Function    : send_calibrate_data_to_mcu
 * Description : Send calibrate data to contexthub
 * Input       : [tag] key, identify an app of sensor
 *             : [subcmd] customize sub cmd
 *             : [data] calibrate data
 *             : [length] data length
 *             : [is_recovery] if contexthub is recoverying
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int send_calibrate_data_to_mcu(int tag, uint32_t subcmd, const void *data,
			       int length, bool is_recovery);

/*
 * Function    : write_calibrate_data_to_nv
 * Description : Write calibrate data
 * Input       : [nv_number] nv number
 *             : [nv_size] nv size
 *             : [nv_name] nv name
 *             : [temp] nv data
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int write_calibrate_data_to_nv(int nv_number, int nv_size,
			       const char *nv_name, const char *temp);

/*
 * Function    : read_calibrate_data_from_nv
 * Description : According to nv number, size and name, read calibrate data
 *             : from nv. Nv data save in user_info
 * Input       : [nv_number] nv numer
 *             : [nv_size] nv size
 *             : [nv_name] nv name
 *             : [user_info] nv data save in this varible
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int read_calibrate_data_from_nv(int nv_number,
				int nv_size, const char *nv_name,
				struct opt_nve_info_user *user_info);

/*
 * Function    : send_parameter_to_mcu
 * Description : Send device parameter to mcu
 * Input       : [node] device manager node
 *             : [cmd] command
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int send_parameter_to_mcu(struct device_manager_node *node, int cmd);

/*
 * Function    : read_sensorlist_info
 * Description : Read sensor info from dts(only for sensor)
 * Input       : [dn] dts node
 *             : [sensor_info] sensorlist info
 *             : [tag] sensor identify
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
void read_sensorlist_info(struct device_node *dn, struct sensorlist_info *sensor_info, int tag);

/*
 * Function    : read_chip_info
 * Description : Read chip info from dts
 * Input       : [dn] dts node
 * ............: [node] device manager node
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
void read_chip_info(struct device_node *dn, struct device_manager_node *node);

/*
 * Function    : sensor_device_detect
 * Description : Common detect process for sensor device.
 *             : If your detect process is different from this, you need
 *             : to imple detect func by yourself.
 * Input       : [dn] dts node
 * ............: [device] device manager node
 *             : [read_data_from_dts] after detect success, need read sensor
 *             : info from dts
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int sensor_device_detect(struct device_node *dn, struct device_manager_node *device,
			 void (*read_data_from_dts)(struct device_node*, struct device_manager_node*));

#endif /* __DEVICE_COMMON_H */
