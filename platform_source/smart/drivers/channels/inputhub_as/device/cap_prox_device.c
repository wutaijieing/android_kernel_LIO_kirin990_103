/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
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

static struct device_manager_node *g_device_cap_prox;

#define CAP_MODEM_THRESHOLE_LEN 8
#define CAP_CALIBRATE_THRESHOLE_LEN 4
#define CAP_DEFAULT_POLL_INTERVAL 200
#define CAP_DEFAULT_CALIBRATE_TYPE 5

struct semteck_sar_data {
	uint16_t threshold_to_ap;
	uint16_t phone_type;
	uint16_t threshold_to_modem[8]; /* default array length */
	uint32_t init_reg_val[17];
	uint8_t ph;
	uint16_t calibrate_thred[4];
};

struct abov_sar_data {
	uint16_t phone_type;
	uint16_t abov_project_id;
	uint16_t threshold_to_modem[CAP_MODEM_THRESHOLE_LEN];
	uint8_t ph;
	uint16_t calibrate_thred[CAP_CALIBRATE_THRESHOLE_LEN];
};

union sar_data {
	struct semteck_sar_data semteck_data;
	struct abov_sar_data abov_data;
};

/*
 * calibrate_type: config by bit(0~7): 0-free 1-near 2-far other-reserve
 * sar_datas: data for diffrent devices
 */
struct cap_prox_platform_data {
	struct sensor_combo_cfg cfg;
	gpio_num_type gpio_int;
	gpio_num_type gpio_int_sh;
	uint16_t poll_interval;
	uint16_t calibrate_type;
	union sar_data sar_datas;
};

static struct cap_prox_platform_data g_cap_prox_data = {
	.cfg = DEF_SENSOR_COM_SETTING,
	.poll_interval = CAP_DEFAULT_POLL_INTERVAL,
	.calibrate_type = CAP_DEFAULT_CALIBRATE_TYPE,
	.gpio_int = 0,
};

/*
 * read GPIO data from DTS
 */
static void read_cap_prox_gpio_data(struct device_node *dn,
	struct cap_prox_platform_data *cap_prox_data,
	const struct device_manager_node *node)
{
	int temp;

	temp = of_get_named_gpio(dn, "gpio_int", 0);
	if (temp < 0)
		ctxhub_err("%s:read %s gpio_int fail\n",
			    __func__, node->device_name_str);
	else
		cap_prox_data->gpio_int = (gpio_num_type)temp;

	if (of_property_read_u32(dn, "gpio_int_sh", &temp))
		ctxhub_err("%s:read %s gpio_int_sh fail\n",
			    __func__, node->device_name_str);
	else
		cap_prox_data->gpio_int_sh = (gpio_num_type)temp;

	if (of_property_read_u32(dn, "poll_interval", &temp))
		ctxhub_err("%s:read %s poll_interval fail\n",
			    __func__, node->device_name_str);
	else
		cap_prox_data->poll_interval = (uint16_t)temp;
}

/*
 * read bus number from DTS
 */
static void read_cap_prox_data_bus_info(struct device_node *dn,
	struct cap_prox_platform_data *cap_prox_data,
	const struct device_manager_node *node)
{
	int temp;
	const char *bus_type = NULL;

	if (of_property_read_string(dn, "bus_type", &bus_type)) {
		ctxhub_warn("%s:cap_prox %s bus_type not configured\n",
			    __func__, node->device_name_str);
	} else {
		if (get_combo_bus_tag(bus_type, (uint8_t *)&temp))
			ctxhub_warn("%s:cap_prox %s bus_type invalid\n",
				     __func__, node->device_name_str);
		else
			cap_prox_data->cfg.bus_type = temp;
	}

	if (of_property_read_u32(dn, "bus_number", &temp))
		ctxhub_warn("%s:cap_prox %s bus_number not configured\n",
			     __func__, node->device_name_str);
	else
		cap_prox_data->cfg.bus_num = temp;

	if (cap_prox_data->cfg.bus_type == TAG_I2C ||
	    cap_prox_data->cfg.bus_type == TAG_I3C) {
		if (of_property_read_u32(dn, "i2c_address", &temp))
			ctxhub_warn("%s:cap_prox %s i2c_address not configured\n",
				     __func__, node->device_name_str);
		else
			cap_prox_data->cfg.i2c_address = temp;
	}
}

/*
 * read file id from DTS
 */
static void read_cap_prox_data_file_id(struct device_node *dn,
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

/*
 * read sensor list information from DTS
 */
static void read_cap_prox_data_from_dts(struct device_node *dn,
	struct device_manager_node *node)
{
	int temp = 0;

	struct cap_prox_platform_data *cap_prox_data =
		(struct cap_prox_platform_data *)node->spara;
	if (!cap_prox_data) {
		ctxhub_err("[%s], param pointer is null\n", __func__);
		return;
	}

	read_chip_info(dn, node);

	read_cap_prox_gpio_data(dn, cap_prox_data, node);

	read_cap_prox_data_file_id(dn, node);

	if (of_property_read_u32(dn, "sensor_list_info_id", &temp))
		ctxhub_err("%s:read %s sensor_list_info_id fail\n",
			    __func__, node->device_name_str);
	else
		add_sensorlist(node->tag, (uint16_t)temp);

	read_cap_prox_data_bus_info(dn, cap_prox_data, node);

	read_sensorlist_info(dn, &node->sensorlist_info, node->tag);
}

/*
 * send ipc, notify sensorhub config cap sensor
 */
static int set_cap_prox_cfg_data(void)
{
	int ret = 0;

	if (g_device_cap_prox->detect_result == DET_SUCC)
		ret = send_parameter_to_mcu(g_device_cap_prox,
					    SUB_CMD_SET_PARAMET_REQ);
	return ret;
}

/*
 * detect cap sensor
 */
static int cap_prox_device_detect(struct device_node *dn)
{
	int ret;

	ret = sensor_device_detect(dn, g_device_cap_prox, read_cap_prox_data_from_dts);
	if (ret != RET_SUCC) {
		ctxhub_err("[%s], cap_prox detect failed!\n", __func__);
		return RET_FAIL;
	}
	g_device_cap_prox->detect_result = DET_SUCC;
	return RET_SUCC;
}

int send_cap_prox_calibrate_data_to_mcu(bool is_lock)
{
	ctxhub_info("%s send calibrate data [%d].\n", __func__, is_lock);
	return 0;
}

static int cap_prox_device_init(void)
{
	int ret;

	if (get_contexthub_dts_status())
		return -EINVAL;
	g_device_cap_prox = device_manager_node_alloc();
	if (!g_device_cap_prox) {
		ctxhub_err("[%s], g_device_cap_prox alloc failed!\n", __func__);
		return -EINVAL;
	}
	g_device_cap_prox->device_name_str = "cap_prox";
	g_device_cap_prox->tag = TAG_CAP_PROX;
	g_device_cap_prox->spara = &g_cap_prox_data;
	g_device_cap_prox->cfg_data_length = sizeof(g_cap_prox_data);
	g_device_cap_prox->detect = cap_prox_device_detect;
	g_device_cap_prox->cfg = set_cap_prox_cfg_data;
	g_device_cap_prox->send_calibrate_data = send_cap_prox_calibrate_data_to_mcu;
	ret = register_contexthub_device(g_device_cap_prox);
	if (ret != 0) {
		ctxhub_err("%s, register %d cap_prox failed\n",
			    __func__, g_device_cap_prox->tag);
		device_manager_node_free(g_device_cap_prox);
		g_device_cap_prox = NULL;
		return ret;
	}

	ctxhub_info("----%s, tag %d--->\n", __func__, g_device_cap_prox->tag);
	return 0;
}

static void __exit cap_prox_device_exit(void)
{
	unregister_contexthub_device(g_device_cap_prox);
	device_manager_node_free(g_device_cap_prox);
}

device_initcall_sync(cap_prox_device_init);
module_exit(cap_prox_device_exit);

MODULE_AUTHOR("Smart");
MODULE_DESCRIPTION("device driver");
MODULE_LICENSE("GPL");
