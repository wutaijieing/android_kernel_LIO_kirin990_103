/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: connectivity device demo implement.
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
static struct device_manager_node *g_device_connectivity;


struct connectivity_platform_data {
	struct sensor_combo_cfg cfg;
	u16 poll_interval;
	gpio_num_type gpio1_gps_cmd_ap;
	gpio_num_type gpio1_gps_cmd_sh;
	gpio_num_type gpio2_gps_ready_ap;
	gpio_num_type gpio2_gps_ready_sh;
	gpio_num_type gpio3_wakeup_gps_ap;
	gpio_num_type gpio3_wakeup_gps_sh;
	u32 i3c_frequency;
	u16 gpio1_pinmut;
	u16 gpio2_pinmut;
	u16 gpio3_pinmut;
};

static struct connectivity_platform_data g_connectivity_data = {
	.cfg = {
		.bus_type = TAG_I3C,
		.bus_num = 1,
		.disable_sample_thread = 0,
		{ .i2c_address = 0x6E },
	},
	.poll_interval = 50,
	.gpio1_gps_cmd_ap = 200,
	.gpio1_gps_cmd_sh = 230,
	.gpio2_gps_ready_ap = 213,
	.gpio2_gps_ready_sh = 242,
	.gpio3_wakeup_gps_ap = 214,
	.gpio3_wakeup_gps_sh = 243,
	.i3c_frequency = 0,
	.gpio1_pinmut = 2, /* Function 2 */
	.gpio2_pinmut = 2, /* Function 2 */
	.gpio3_pinmut = 2, /* Function 2 */
};

static void read_connectivity_bus_type(struct device_node *dn, uint8_t *bus_type)
{
	const char *bus_string = NULL;
	int bus_type_val = (int)(*bus_type);
	int ret;

	if (of_property_read_string(dn, "bus_type", &bus_string)) {
		ctxhub_err("%s:connectivity bus_type not configured\n", __func__);
		return;
	}

	ret = get_combo_bus_tag(bus_string, (uint8_t *)&bus_type_val);
	if (ret != RET_SUCC) {
		ctxhub_warn("connectivity bus_type string invalid, next detect digit\n");
		if (of_property_read_u32(dn, "bus_type", &bus_type_val)) {
			ctxhub_err("%s:read bus_type digit fail\n", __func__);
			return;
		}
	}

	if (bus_type_val >= TAG_END) {
		ctxhub_err("%s:read bus_type %d invalid\n", __func__, bus_type_val);
		return;
	}
	*bus_type = (uint8_t)bus_type_val;
}

static void get_bus_info(struct device_node *dn)
{
	int temp = 0;

	read_connectivity_bus_type(dn, (uint8_t *)&g_connectivity_data.cfg.bus_type);

	if (of_property_read_u32(dn, "bus_number", &temp))
		ctxhub_err("%s:read bus_number fail\n", __func__);
	else
		g_connectivity_data.cfg.bus_num = (uint8_t)temp;

	if (of_property_read_u32(dn, "i2c_address", &temp))
		ctxhub_err("%s:read i2c_address fail\n", __func__);
	else
		g_connectivity_data.cfg.i2c_address = (uint32_t)temp;

	if (of_property_read_u32(dn, "i3c_frequency", &temp))
		ctxhub_err("%s:read i3c_frequency fail\n", __func__);
	else
		g_connectivity_data.i3c_frequency = (uint32_t)temp;
}

static void get_gpio_info(struct device_node *dn)
{
	int temp = 0;

	if (of_property_read_u32(dn, "gpio1_gps_cmd_ap", &temp))
		ctxhub_err("%s:read gpio1_gps_cmd_ap fail\n", __func__);
	else
		g_connectivity_data.gpio1_gps_cmd_ap = (gpio_num_type)temp;

	if (of_property_read_u32(dn, "gpio1_gps_cmd_sh", &temp))
		ctxhub_err("%s:read gpio1_gps_cmd_sh fail\n", __func__);
	else
		g_connectivity_data.gpio1_gps_cmd_sh = (gpio_num_type)temp;

	if (of_property_read_u32(dn, "gpio1_gps_cmd_pinmux", &temp))
		ctxhub_err("%s:read gpio1_gps_cmd_pinmux fail\n", __func__);
	else
		g_connectivity_data.gpio1_pinmut = (u16)temp;
	if (of_property_read_u32(dn, "gpio2_gps_ready_ap", &temp))
		ctxhub_err("%s:read gpio2_gps_ready_ap fail\n", __func__);
	else
		g_connectivity_data.gpio2_gps_ready_ap = (gpio_num_type)temp;

	if (of_property_read_u32(dn, "gpio2_gps_ready_sh", &temp))
		ctxhub_err("%s:read gpio2_gps_ready_sh fail\n", __func__);
	else
		g_connectivity_data.gpio2_gps_ready_sh = (gpio_num_type)temp;

	if (of_property_read_u32(dn, "gpio2_gps_ready_pinmux", &temp))
		ctxhub_err("%s:read gpio2_gps_ready_pinmux fail\n", __func__);
	else
		g_connectivity_data.gpio2_pinmut = (u16)temp;

	if (of_property_read_u32(dn, "gpio3_wakeup_gps_ap", &temp))
		ctxhub_err("%s:read gpio3_wakeup_gps_ap fail\n", __func__);
	else
		g_connectivity_data.gpio3_wakeup_gps_ap = (gpio_num_type) temp;

	if (of_property_read_u32(dn, "gpio3_wakeup_gps_sh", &temp))
		ctxhub_err("%s:read gpio3_wakeup_gps_sh fail\n", __func__);
	else
		g_connectivity_data.gpio3_wakeup_gps_sh = (gpio_num_type)temp;

	if (of_property_read_u32(dn, "gpio3_wakeup_gps_pinmux", &temp))
		ctxhub_err("%s:read gpio3_wakeup_gps_pinmux fail\n", __func__);
	else
		g_connectivity_data.gpio3_pinmut = (u16)temp;
}

static int read_connectivity_data_from_dts(struct device_node *dn)
{
	int temp = 0;

	if (!dn) {
		ctxhub_err("[%s], input device node is null\n", __func__);
		g_device_connectivity->detect_result = DET_FAIL;
		return RET_FAIL;
	}

	read_chip_info(dn, g_device_connectivity);

	get_gpio_info(dn);
	get_bus_info(dn);
	if (of_property_read_u32(dn, "file_id", &temp))
		ctxhub_err("%s:read connectivity file_id fail\n", __func__);
	else
		g_device_connectivity->dyn_file_id = (uint16_t)temp;

	ctxhub_info("connectivity file id is %d\n", temp);

	g_device_connectivity->detect_result = DET_SUCC;
	return RET_SUCC;
}

static int set_connectivity_cfg_data(void)
{
	int ret = 0;

	if (g_device_connectivity->detect_result == DET_SUCC)
		ret = send_parameter_to_mcu(g_device_connectivity,
					    SUB_CMD_SET_PARAMET_REQ);
	return ret;
}

static int connectivity_device_init(void)
{
	int ret;

	if (get_contexthub_dts_status())
		return -EINVAL;
	g_device_connectivity = device_manager_node_alloc();
	if (!g_device_connectivity) {
		ctxhub_err("[%s], g_device_connectivity alloc failed!\n", __func__);
		return -EINVAL;
	}
	g_device_connectivity->device_name_str = "connectivity";
	g_device_connectivity->tag = TAG_CONNECTIVITY;
	g_device_connectivity->detect = read_connectivity_data_from_dts;
	g_device_connectivity->cfg = set_connectivity_cfg_data;
	g_device_connectivity->spara = &g_connectivity_data;
	g_device_connectivity->cfg_data_length = sizeof(g_connectivity_data);
	ret = register_contexthub_device(g_device_connectivity);
	if (ret != 0) {
		ctxhub_err("%s, register %d connectivity failed\n",
			__func__, g_device_connectivity->tag);
		device_manager_node_free(g_device_connectivity);
		g_device_connectivity = NULL;
		return ret;
	}

	ctxhub_info("----%s, tag %d--->\n", __func__, g_device_connectivity->tag);
	return 0;
}

static void __exit connectivity_device_exit(void)
{
	unregister_contexthub_device(g_device_connectivity);
	device_manager_node_free(g_device_connectivity);
}

device_initcall_sync(connectivity_device_init);
module_exit(connectivity_device_exit);

MODULE_AUTHOR("Smart");
MODULE_DESCRIPTION("device driver");
MODULE_LICENSE("GPL");
