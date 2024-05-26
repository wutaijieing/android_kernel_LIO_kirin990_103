/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2020. All rights reserved.
 * Description: acc device demo implement.
 * Create: 2019-11-05
 */
#include <securec.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <platform_include/basicplatform/linux/nve/nve_ap_kernel_interface.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <platform_include/smart/linux/inputhub_as/device_manager.h>
#include <platform_include/smart/linux/iomcu_log.h>
#include <inputhub_as/sensor_channel/sensor_channel.h>
#include "device_common.h"
#include "common/common.h"

static struct device_manager_node *g_device_acc;

#define ACC_CALIBRATE_DATA_LENGTH        15
#define GSENSOR_OFFSET_SEN_NUM 6

struct g_sensor_platform_data {
	struct sensor_combo_cfg cfg;
	u8 axis_map_x;
	u8 axis_map_y;
	u8 axis_map_z;
	u8 negate_x;
	u8 negate_y;
	u8 negate_z;
	u8 used_int_pin;
	gpio_num_type gpio_int1;
	gpio_num_type gpio_int2;
	gpio_num_type gpio_int2_sh;
	u16 poll_interval;
	int offset_x;
	int offset_y;
	int offset_z;
	int sensitivity_x;
	int sensitivity_y;
	int sensitivity_z;
	u8 device_type;
	u8 calibrate_style;
	u8 calibrate_way;
	u16 x_calibrate_thredhold;
	u16 y_calibrate_thredhold;
	u16 z_calibrate_thredhold;
	u8 wakeup_duration;
	u8 g_sensor_extend_data[SENSOR_PLATFORM_EXTEND_DATA_SIZE];
	u8 gpio_int2_sh_func;
};

u8 g_acc_cali_way;
static u8 gsensor_calibrate_data[MAX_SENSOR_CALIBRATE_DATA_LENGTH];
static int gsensor_offset[ACC_CALIBRATE_DATA_LENGTH];

static struct g_sensor_platform_data g_gsensor_data = {
	.cfg = DEF_SENSOR_COM_SETTING,
	.poll_interval = 10,
	.axis_map_x = 1,
	.axis_map_y = 0,
	.axis_map_z = 2,
	.negate_x = 0,
	.negate_y = 1,
	.negate_z = 0,
	.used_int_pin = 2,
	.gpio_int1 = 208,
	.gpio_int2 = 0,
	.gpio_int2_sh = 0,
	.device_type = 0,
	.calibrate_style = 0,
	.calibrate_way = 0,
	.x_calibrate_thredhold = 250, /* 250 mg */
	.y_calibrate_thredhold = 250, /* 250 mg */
	.z_calibrate_thredhold = 320, /* 320 mg */
	.wakeup_duration = 0x60, /* default set up 3 duration */
	.gpio_int2_sh_func = 2,
};

#define ACC_OFFSET_NV_NUM 307
#define ACC_OFFSET_NV_SIZE 60

static void read_acc_gpio_data(struct device_node *dn,
			       struct g_sensor_platform_data *gsensor_data,
		const struct device_manager_node *node)
{
	int temp;

	if (of_property_read_u32(dn, "used_int_pin", &temp))
		ctxhub_err("%s:read %s used_int_pin fail\n",
			__func__, node->device_name_str);
	else
		gsensor_data->used_int_pin = (uint8_t)temp;

	temp = of_get_named_gpio(dn, "gpio_int1", 0);
	if (temp < 0)
		ctxhub_err("%s:read %s gpio_int1 fail\n",
			__func__, node->device_name_str);
	else
		gsensor_data->gpio_int1 = (gpio_num_type) temp;

	temp = of_get_named_gpio(dn, "gpio_int2", 0);
	if (temp < 0)
		ctxhub_err("%s:read %s gpio_int2 fail\n",
			__func__, node->device_name_str);
	else
		gsensor_data->gpio_int2 = (gpio_num_type) temp;

	if (of_property_read_u32(dn, "gpio_int2_sh", &temp))
		ctxhub_err("%s:read %s gpio_int2_sh fail\n",
			__func__, node->device_name_str);
	else
		gsensor_data->gpio_int2_sh = (gpio_num_type) temp;

	if (of_property_read_u32(dn, "gpio_int2_sh_func", &temp))
		ctxhub_err("%s:read %s gpio_int2_sh_func fail\n", __func__,
			node->device_name_str);
	else
		gsensor_data->gpio_int2_sh_func = (uint8_t)temp;

	if (of_property_read_u32(dn, "poll_interval", &temp))
		ctxhub_err("%s:read %s poll_interval fail\n",
			__func__, node->device_name_str);
	else
		gsensor_data->poll_interval = (uint16_t)temp;
}

static void read_acc_data_bus_info(struct device_node *dn,
				   struct g_sensor_platform_data *gsensor_data,
		const struct device_manager_node *node)
{
	int temp;
	const char *bus_type = NULL;

	if (of_property_read_string(dn, "bus_type", &bus_type)) {
		ctxhub_warn("%s:acc %s bus_type not configured\n",
			 __func__, node->device_name_str);
	} else {
		if (get_combo_bus_tag(bus_type, (uint8_t *)&temp))
			ctxhub_warn("%s:acc %s bus_type invalid\n",
				 __func__, node->device_name_str);
		else
			gsensor_data->cfg.bus_type = temp;
	}

	if (of_property_read_u32(dn, "bus_number", &temp))
		ctxhub_warn("%s:acc %s bus_number not configured\n",
			 __func__, node->device_name_str);
	else
		gsensor_data->cfg.bus_num = temp;

	if (gsensor_data->cfg.bus_type == TAG_I2C ||
	    gsensor_data->cfg.bus_type == TAG_I3C) {
		if (of_property_read_u32(dn, "i2c_address", &temp))
			ctxhub_warn("%s:acc %s i2c_address not configured\n",
				 __func__, node->device_name_str);
		else
			gsensor_data->cfg.i2c_address = temp;
	}
}

static void read_acc_data_axis_info(struct device_node *dn,
				    struct g_sensor_platform_data *gsensor_data,
		const struct device_manager_node *node)
{
	int temp;

	if (of_property_read_u32(dn, "calibrate_style", &temp))
		ctxhub_err("%s:read %s calibrate_style fail\n",
			__func__, node->device_name_str);
	else
		gsensor_data->calibrate_style = (uint8_t)temp;

	if (of_property_read_u32(dn, "axis_map_x", &temp))
		ctxhub_err("%s:read %s axis_map_x fail\n",
			__func__, node->device_name_str);
	else
		gsensor_data->axis_map_x = (uint8_t)temp;

	if (of_property_read_u32(dn, "axis_map_y", &temp))
		ctxhub_err("%s:read %s axis_map_y fail\n",
			__func__, node->device_name_str);
	else
		gsensor_data->axis_map_y = (uint8_t)temp;

	if (of_property_read_u32(dn, "axis_map_z", &temp))
		ctxhub_err("%s:read %s axis_map_z fail\n",
			__func__, node->device_name_str);
	else
		gsensor_data->axis_map_z = (uint8_t)temp;

	if (of_property_read_u32(dn, "negate_x", &temp))
		ctxhub_err("%s:read %s negate_x fail\n",
			__func__, node->device_name_str);
	else
		gsensor_data->negate_x = (uint8_t)temp;

	if (of_property_read_u32(dn, "negate_y", &temp))
		ctxhub_err("%s:read %s negate_y fail\n",
			__func__, node->device_name_str);
	else
		gsensor_data->negate_y = (uint8_t)temp;

	if (of_property_read_u32(dn, "negate_z", &temp))
		ctxhub_err("%s:read %s negate_z fail\n",
			__func__, node->device_name_str);
	else
		gsensor_data->negate_z = (uint8_t)temp;
}

static void read_acc_data_calibrate_info(struct device_node *dn,
					 struct g_sensor_platform_data *gsensor_data,
			const struct device_manager_node *node)
{
	int temp;

	if (of_property_read_u32(dn, "x_calibrate_thredhold", &temp))
		ctxhub_err("%s:read %s x_calibrate_thredhold fail\n",
			__func__, node->device_name_str);
	else
		gsensor_data->x_calibrate_thredhold = (uint16_t)temp;

	if (of_property_read_u32(dn, "y_calibrate_thredhold", &temp))
		ctxhub_err("%s:read %s y_calibrate_thredhold fail\n",
			__func__, node->device_name_str);
	else
		gsensor_data->y_calibrate_thredhold = (uint16_t)temp;

	if (of_property_read_u32(dn, "z_calibrate_thredhold", &temp))
		ctxhub_err("%s:read %s z_calibrate_thredhold fail\n",
			__func__, node->device_name_str);
	else
		gsensor_data->z_calibrate_thredhold = (uint16_t)temp;

	if (of_property_read_u32(dn, "calibrate_way", &temp)) {
		ctxhub_err("%s:read %s calibrate_way fail\n",
			__func__, node->device_name_str);
	} else {
		gsensor_data->calibrate_way = (uint8_t)temp;
		g_acc_cali_way = ((node->tag == TAG_ACCEL) ?
			(uint8_t)temp : g_acc_cali_way);
	}
}

static void read_acc_data_file_id(struct device_node *dn,
			struct device_manager_node *node)
{
	int aux;
	int temp;

	if (of_property_read_u32(dn, "aux_prop", &aux))
		ctxhub_err("%s:read %s aux_prop fail\n",
			__func__, node->device_name_str);
	else
		ctxhub_info("%s:read %s aux_prop %d\n",
			 __func__, node->device_name_str, aux);

	if (of_property_read_u32(dn, "file_id", &temp))
		ctxhub_err("%s:read %s file_id fail\n",
			__func__, node->device_name_str);
	else if (aux)
		node->aux_file_id = (uint16_t)temp;
	else
		node->dyn_file_id = (uint16_t)temp;
}

static void read_acc_data_from_dts(struct device_node *dn,
				   struct device_manager_node *node)
{
	int temp = 0;

	struct g_sensor_platform_data *gsensor_data =
		(struct g_sensor_platform_data *)node->spara;
	if (!gsensor_data) {
		ctxhub_err("[%s], param pointer is null\n", __func__);
		return;
	}

	read_chip_info(dn, node);

	read_acc_gpio_data(dn, gsensor_data, node);

	read_acc_data_axis_info(dn, gsensor_data, node);

	if (of_property_read_u32(dn, "device_type", &temp))
		ctxhub_err("%s:read %s device_type fail\n",
			__func__, node->device_name_str);
	else
		gsensor_data->device_type = (uint8_t)temp;

	read_acc_data_calibrate_info(dn, gsensor_data, node);

	read_acc_data_file_id(dn, node);

	if (of_property_read_u32(dn, "sensor_list_info_id", &temp))
		ctxhub_err("%s:read %s sensor_list_info_id fail\n",
			__func__, node->device_name_str);
	else
		add_sensorlist(node->tag, (uint16_t)temp);

	if (of_property_read_u32(dn, "wakeup_duration", &temp))
		ctxhub_err("%s:read %s wakeup_duration fail\n",
			__func__, node->device_name_str);
	else
		gsensor_data->wakeup_duration = (uint8_t)temp;

	read_acc_data_bus_info(dn, gsensor_data, node);

	read_sensorlist_info(dn, &node->sensorlist_info, node->tag);
}

static int set_acc_cfg_data(void)
{
	int ret = 0;

	if (g_device_acc->detect_result == DET_SUCC)
		ret = send_parameter_to_mcu(g_device_acc,
					    SUB_CMD_SET_PARAMET_REQ);
	return ret;
}

static int acc_device_detect(struct device_node *dn)
{
	int ret;

	ret = sensor_device_detect(dn, g_device_acc, read_acc_data_from_dts);
	if (ret != RET_SUCC) {
		ctxhub_err("[%s], acc detect failed!\n", __func__);
		return RET_FAIL;
	}
	g_device_acc->detect_result = DET_SUCC;
	return RET_SUCC;
}

int send_acc_calibrate_data_to_mcu(bool is_lock)
{
	// if use new ipc interface, this interface parameter will change
	unsigned int copy_len;
	struct opt_nve_info_user user_info;
	static int acc_first_read_flag;
	int i;

	if (acc_first_read_flag == 0) {
		if (read_calibrate_data_from_nv(ACC_OFFSET_NV_NUM,
						ACC_OFFSET_NV_SIZE, "gsensor", &user_info))
			return RET_FAIL;
		acc_first_read_flag = 1;
		/* copy to gsensor_offset by pass */
		if (sizeof(user_info.nv_data) < sizeof(gsensor_offset)) {
			ctxhub_err("[%s], copy nv data failed, src len is %lu, copy len is %lu\n",
				__func__,
				sizeof(user_info.nv_data),
				sizeof(gsensor_offset));
			return RET_FAIL;
		}

		if (memcpy_s(gsensor_offset, sizeof(gsensor_offset),
			user_info.nv_data, sizeof(gsensor_offset)) != EOK) {
			ctxhub_err("[%s], memcpy failed\n", __func__);
			return RET_FAIL;
		}

		for (i = 0; i < GSENSOR_OFFSET_SEN_NUM; i++)
			ctxhub_info("nve_direct_access read gsensor_offset_sen[%d], %d\n",
				 i, gsensor_offset[i]);

		for (i = GSENSOR_OFFSET_SEN_NUM; i < ACC_CALIBRATE_DATA_LENGTH;
			i++)
			ctxhub_info("nve_direct_access read gsensor_xis_angle[%d], %d\n",
				 i, gsensor_offset[i]);

		copy_len = (sizeof(gsensor_calibrate_data) <
			ACC_OFFSET_NV_SIZE) ? sizeof(gsensor_calibrate_data) :
			ACC_OFFSET_NV_SIZE;
		if (copy_len > sizeof(gsensor_offset)) {
			ctxhub_err("[%s], copy failed, src len is %lu, copy len is %u\n",
				__func__,
				sizeof(gsensor_offset), copy_len);
			return RET_FAIL;
		}

		if (memcpy_s(gsensor_calibrate_data,
			     sizeof(gsensor_calibrate_data), gsensor_offset,
			copy_len) != EOK) {
			ctxhub_err("[%s], memcpy calibrate data error\n",
				__func__);
			return RET_FAIL;
		}
	}

	if (send_calibrate_data_to_mcu(TAG_ACCEL, SUB_CMD_SET_OFFSET_REQ,
				       gsensor_offset, ACC_OFFSET_NV_SIZE,
				is_lock) != 0)
		return RET_FAIL;

	return 0;
}

#ifdef CONFIG_CONTEXTHUB_WATCH_FACTORY
/*
 * send acc offset to sensorhub
 */
int send_acc_offset_to_sensorhub(void)
{
	struct opt_nve_info_user user_info;
	int i;

	if (read_calibrate_data_from_nv(ACC_OFFSET_NV_NUM,
		ACC_OFFSET_NV_SIZE, "gsensor", &user_info)) {
		ctxhub_err("read_calibrate_data_from_nv error!\n");
		return RET_FAIL;
	}

	ctxhub_info("acc_feima show_offset [0x%x 0x%x 0x%x 0x%x] [0x%x 0x%x 0x%x 0x%x] [0x%x 0x%x 0x%x 0x%x].\n",
			user_info.nv_data[0], user_info.nv_data[1], user_info.nv_data[2],
			user_info.nv_data[3], user_info.nv_data[4], user_info.nv_data[5],
			user_info.nv_data[6], user_info.nv_data[7], user_info.nv_data[8],
			user_info.nv_data[9], user_info.nv_data[10], user_info.nv_data[11]);

	/* copy to gsensor_offset by pass */
	if (sizeof(user_info.nv_data) < sizeof(gsensor_offset)) {
		ctxhub_err("[%s], copy nv data failed, src len is %lu, copy len is %lu\n",
			__func__,
			sizeof(user_info.nv_data),
			sizeof(gsensor_offset));
		return RET_FAIL;
	}

	(void)memcpy_s(gsensor_offset, sizeof(gsensor_offset),
		user_info.nv_data, sizeof(gsensor_offset));

	for (i = 0; i < GSENSOR_OFFSET_SEN_NUM; i++)
		ctxhub_info("nve_direct_access read gsensor_offset_sen[%d], %d\n",
			     i, gsensor_offset[i]);

	if (send_calibrate_data_to_mcu(TAG_ACCEL, SUB_CMD_SET_OFFSET_REQ,
		gsensor_offset, sizeof(gsensor_offset), false) != 0)
		return RET_FAIL;

	return 0;
}
#endif

static int acc_device_init(void)
{
	int ret;

	if (get_contexthub_dts_status())
		return -EINVAL;
	g_device_acc = device_manager_node_alloc();
	if (!g_device_acc) {
		ctxhub_err("[%s], g_device_acc alloc failed!\n", __func__);
		return -EINVAL;
	}
	g_device_acc->device_name_str = "acc";
	g_device_acc->tag = TAG_ACCEL;
	g_device_acc->spara = &g_gsensor_data;
	g_device_acc->cfg_data_length = sizeof(g_gsensor_data);
	g_device_acc->detect = acc_device_detect;
	g_device_acc->cfg = set_acc_cfg_data;
	g_device_acc->send_calibrate_data = send_acc_calibrate_data_to_mcu;
	ret = register_contexthub_device(g_device_acc);
	if (ret != 0) {
		ctxhub_err("%s, register %d acc failed\n",
			__func__, g_device_acc->tag);
		device_manager_node_free(g_device_acc);
		g_device_acc = NULL;
		return ret;
	}

	ctxhub_info("----%s, tag %d--->\n", __func__, g_device_acc->tag);
	return 0;
}

static void __exit acc_device_exit(void)
{
	unregister_contexthub_device(g_device_acc);
	device_manager_node_free(g_device_acc);
}

device_initcall_sync(acc_device_init);
module_exit(acc_device_exit);

MODULE_AUTHOR("Smart");
MODULE_DESCRIPTION("device driver");
MODULE_LICENSE("GPL");
