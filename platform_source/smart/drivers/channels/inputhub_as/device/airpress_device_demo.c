/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: airpress device demo implement.
 * Create: 2019-11-05
 */
#include <securec.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <platform_include/basicplatform/linux/nve/nve_ap_kernel_interface.h>
#include <linux/of_gpio.h>
#include <platform_include/smart/linux/inputhub_as/device_manager.h>
#include <platform_include/smart/linux/iomcu_log.h>
#include <inputhub_as/sensor_channel/sensor_channel.h>
#include "device_common.h"
#include "common/common.h"

static struct device_manager_node *g_device_airpress;

#define AIRPRESS_CALIDATA_NV_NUM 332
#define AIRPRESS_CALIDATA_NV_SIZE 4
#define AIRPRESS_DEFAULT_POLL_INTERVAL 1000

struct airpress_platform_data {
	struct sensor_combo_cfg cfg;
	int offset;
	int is_support_touch;
	u16 poll_interval;
	u16 touch_fac_order;
	u16 touch_fac_wait_time;
	u16 tp_touch_coordinate_threshold[TP_COORDINATE_THRESHOLD];
	u8 airpress_extend_data[SENSOR_PLATFORM_EXTEND_DATA_SIZE];
};

struct airpress_platform_data g_airpress_data = {
	.cfg = DEF_SENSOR_COM_SETTING,
	.poll_interval = AIRPRESS_DEFAULT_POLL_INTERVAL,
};

static void get_sensorlist_id(struct device_node *dn,
			      struct device_manager_node *node)
{
	int temp = 0;

	if (of_property_read_u32(dn, "sensor_list_info_id", &temp))
		ctxhub_err("%s:read ps sensor_list_info_id fail\n", __func__);
	else
		add_sensorlist(node->tag, (uint16_t)temp);
}

static void read_airpress_data_from_dts(struct device_node *dn,
					struct device_manager_node *node)
{
	int temp = 0;

	read_chip_info(dn, node);

	if (of_property_read_u32(dn, "poll_interval", &temp))
		ctxhub_err("%s:read poll_interval fail\n", __func__);
	else
		g_airpress_data.poll_interval = (uint16_t)temp;

	if (of_property_read_u32(dn, "isSupportTouch", &temp)) {
		ctxhub_err("%s:read isSupportTouch fail\n", __func__);
		g_airpress_data.is_support_touch = 0;
	} else {
		g_airpress_data.is_support_touch = temp;
	}

	if (of_property_read_u32(dn, "airpress_touch_calibrate_order",
				 &temp)) {
		ctxhub_err("%s:read airpress_touch_calibrate_order fail\n",
			__func__);
		g_airpress_data.touch_fac_order = 0;
	} else {
		g_airpress_data.touch_fac_order = (uint16_t)temp;
	}

	if (of_property_read_u32(dn, "touch_fac_wait_time", &temp)) {
		ctxhub_err("%s:read touch_fac_wait_time fail\n", __func__);
		g_airpress_data.touch_fac_wait_time = 0;
	} else {
		g_airpress_data.touch_fac_wait_time = (uint16_t)temp;
	}

	if (of_property_read_u16_array(dn, "tp_touch_coordinate_threshold",
				       g_airpress_data.tp_touch_coordinate_threshold,
				       TP_COORDINATE_THRESHOLD)) {
		ctxhub_err("%s:read tp_touch_coordinate_threshold fail\n",
			__func__);
		for (temp = 0; temp < TP_COORDINATE_THRESHOLD; temp++)
			g_airpress_data.tp_touch_coordinate_threshold[temp] = 0;
	}

	if (of_property_read_u32(dn, "reg", &temp))
		ctxhub_err("%s:read airpress reg fail\n", __func__);
	else
		g_airpress_data.cfg.i2c_address = (uint8_t)temp;

	if (of_property_read_u32(dn, "file_id", &temp))
		ctxhub_err("%s:read airpress file_id fail\n", __func__);
	else
		node->dyn_file_id = (uint16_t)temp;

	get_sensorlist_id(dn, node);
	read_sensorlist_info(dn, &node->sensorlist_info, node->tag);
}

static int set_airpress_cfg_data(void)
{
	int ret = 0;

	if (g_device_airpress->detect_result == DET_SUCC)
		ret = send_parameter_to_mcu(g_device_airpress,
					    SUB_CMD_SET_PARAMET_REQ);

	return ret;
}

static int airpress_device_detect(struct device_node *dn)
{
	int ret;

	ret = sensor_device_detect(dn, g_device_airpress, read_airpress_data_from_dts);
	if (ret != RET_SUCC) {
		ctxhub_err("[%s], airpress detect failed!\n", __func__);
		return RET_FAIL;
	}

	return RET_SUCC;
}

int send_airpress_calibrate_data_to_mcu(bool is_lock)
{
	struct opt_nve_info_user user_info;
	static int airpress_first_read_flag;
	unsigned int copy_len;

	if (airpress_first_read_flag == 0) {
		if (read_calibrate_data_from_nv(AIRPRESS_CALIDATA_NV_NUM,
						AIRPRESS_CALIDATA_NV_SIZE, "AIRDATA", &user_info))
			return RET_FAIL;

		airpress_first_read_flag = 1;

		copy_len = sizeof(g_airpress_data.offset);
		if (copy_len > sizeof(user_info.nv_data)) {
			ctxhub_err("[%s], src max %lu less than copy len %u\n",
				__func__, sizeof(user_info.nv_data), copy_len);
			return RET_FAIL;
		}

		if (memcpy_s(&g_airpress_data.offset, sizeof(g_airpress_data.offset),
			user_info.nv_data, sizeof(g_airpress_data.offset)) != EOK) {
			ctxhub_err("[%s], memcpy failed\n", __func__);
			return RET_FAIL;
		}

		ctxhub_info("airpress offset data=%d\n", g_airpress_data.offset);
	}
	if (send_calibrate_data_to_mcu(TAG_PRESSURE, SUB_CMD_SET_OFFSET_REQ,
				       &g_airpress_data.offset,
					sizeof(g_airpress_data.offset), is_lock))
		return RET_FAIL;

	return 0;
}

static int airpress_device_init(void)
{
	int ret;

	if (get_contexthub_dts_status())
		return -EINVAL;
	g_device_airpress = device_manager_node_alloc();
	if (!g_device_airpress) {
		ctxhub_err("[%s], g_device_airpress alloc failed!\n", __func__);
		return -EINVAL;
	}

	g_device_airpress->device_name_str = "airpress";
	g_device_airpress->tag = TAG_PRESSURE;
	g_device_airpress->spara = &g_airpress_data;
	g_device_airpress->cfg_data_length = sizeof(g_airpress_data);
	g_device_airpress->detect = airpress_device_detect;
	g_device_airpress->cfg = set_airpress_cfg_data;
	g_device_airpress->send_calibrate_data =
		send_airpress_calibrate_data_to_mcu;
	ret = register_contexthub_device(g_device_airpress);
	if (ret != 0) {
		ctxhub_err("%s, register %d airpress failed\n",
			__func__, g_device_airpress->tag);
		device_manager_node_free(g_device_airpress);
		g_device_airpress = NULL;
		return ret;
	}

	ctxhub_info("----%s, tag %d--->\n", __func__, g_device_airpress->tag);
	return 0;
}

static void __exit airpress_device_exit(void)
{
	unregister_contexthub_device(g_device_airpress);
	device_manager_node_free(g_device_airpress);
}

device_initcall_sync(airpress_device_init);
module_exit(airpress_device_exit);

MODULE_AUTHOR("Smart");
MODULE_DESCRIPTION("device driver");
MODULE_LICENSE("GPL");
