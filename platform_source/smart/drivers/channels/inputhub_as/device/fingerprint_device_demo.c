 /*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: fingerprint device demo implement.
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
#include <linux/delay.h>
#include <platform_include/smart/linux/inputhub_as/device_manager.h>
#include <platform_include/smart/linux/iomcu_log.h>
#include <platform_include/smart/linux/inputhub_as/bus_interface.h>
#include "device_common.h"
#include "common/common.h"
static struct device_manager_node *g_device_fingerprint;

#define FP_SPI_NUM	2
#define GPIO_STAT_HIGH	1
#define GPIO_STAT_LOW	0
#define RESET_SHORT_SLEEP	5
#define RESET_LONG_SLEEP	10
#define FPC_NAME_LEN	3
#define SYNA_NAME_LEN	4
#define GOODIX_NAME_LEN	6
#define GOODIX_G2_NAME_LEN	15
#define SILEAD_NAME_LEN	6
#define QFP_NAME_LEN	3
#define EGIS_NAME_LEN	4

struct fingerprint_platform_data {
	struct sensor_combo_cfg cfg;
	u16 reg;
	u16 chip_id;
	u16 product_id;
	gpio_num_type gpio_irq;
	gpio_num_type gpio_irq_sh;
	gpio_num_type gpio_cs;
	gpio_num_type gpio_reset;
	gpio_num_type gpio_reset_sh;
	u16 poll_interval;
	u16 tp_hover_support;
};

static struct fingerprint_platform_data g_fingerprint_data = {
	.cfg = {
		.bus_type = TAG_SPI,
		.bus_num = 2,
		.disable_sample_thread = 1,
		{ .ctrl = { .data = 218 } },
	},
	.reg = 0xFC,
	.chip_id = 0x021b,
	.product_id = 9,
	.gpio_irq = 207,
	.gpio_irq_sh = 236,
	.gpio_reset = 149,
	.gpio_reset_sh = 1013,
	.gpio_cs = 218,
	.poll_interval = 50,
};

#define FINGERPRINT_SENSOR_DETECT_SUCCESS 10
#define FINGERPRINT_WRITE_CMD_LEN 7
#define FINGERPRINT_READ_CONTENT_LEN 2

static int fingerprint_get_gpio_config(struct device_node *dn,
				       gpio_num_type *gpio_reset, gpio_num_type *gpio_cs,
	gpio_num_type *gpio_irq)
{
	int temp = 0;
	int ret = RET_SUCC;

	if (of_property_read_u32(dn, "gpio_reset", &temp)) {
		ctxhub_err("%s:read gpio_reset fail\n", __func__);
		ret = RET_FAIL;
	} else {
		*gpio_reset = (gpio_num_type)temp;
	}

	if (of_property_read_u32(dn, "gpio_cs", &temp)) {
		ctxhub_err("%s:read gpio_cs fail\n", __func__);
		ret = RET_FAIL;
	} else {
		*gpio_cs = (gpio_num_type)temp;
	}

	if (of_property_read_u32(dn, "gpio_irq", &temp)) {
		ctxhub_err("%s:read gpio_irq fail\n", __func__);
		ret = RET_FAIL;
	} else {
		*gpio_irq = (gpio_num_type)temp;
	}

	return ret;
}

static void fingerprint_gpio_reset(gpio_num_type gpio_reset,
				   unsigned int first_sleep, unsigned int second_sleep)
{
	gpio_direction_output(gpio_reset, GPIO_STAT_HIGH);
	msleep(first_sleep);
	gpio_direction_output(gpio_reset, GPIO_STAT_LOW);
	msleep(second_sleep);
	gpio_direction_output(gpio_reset, GPIO_STAT_HIGH);
}

static void set_fpc_config(gpio_num_type gpio_reset)
{
	gpio_direction_output(gpio_reset, GPIO_STAT_HIGH);
	msleep(RESET_LONG_SLEEP);
}

static void set_syna_config(gpio_num_type gpio_reset, gpio_num_type gpio_cs)
{
	union spi_ctrl ctrl;
	u8 tx[2] = { 0 }; /* 2 : tx register length */
	u32 tx_len;

	fingerprint_gpio_reset(gpio_reset, RESET_LONG_SLEEP, RESET_LONG_SLEEP);

	/* send 2 byte 0xF0 cmd to software reset sensor */
	ctrl.data = gpio_cs;
	tx[0] = 0xF0;
	tx[1] = 0xF0;
	tx_len = 2;
	mcu_spi_rw(FP_SPI_NUM, ctrl, tx, tx_len, NULL, 0);
	msleep(100); /* 100 : sleep time after soft reset */
}

static void set_goodix_config(gpio_num_type gpio_reset, gpio_num_type gpio_cs)
{
	u8 tx[FINGERPRINT_WRITE_CMD_LEN] = { 0 };
	u8 rx[FINGERPRINT_READ_CONTENT_LEN] = { 0 };
	u32 tx_len;
	u32 rx_len;
	union spi_ctrl ctrl;

	fingerprint_gpio_reset(gpio_reset, RESET_LONG_SLEEP, RESET_LONG_SLEEP);

	/* set sensor to idle mode, cmd is 0xC0, length is 1 */
	ctrl.data = gpio_cs;
	tx[0] = 0xC0;
	tx_len = 1;
	mcu_spi_rw(FP_SPI_NUM, ctrl, tx, tx_len, NULL, 0);

	/* write cmd 0xF0 & address 0x0126, length is 3 */
	tx[0] = 0xF0;
	tx[1] = 0x01;
	tx[2] = 0x26;
	tx_len = 3;
	mcu_spi_rw(FP_SPI_NUM, ctrl, tx, tx_len, NULL, 0);

	/* read cmd 0xF1, cmd length is 1, rx length is 2 */
	tx[0] = 0xF1;
	tx_len = 1;
	rx_len = 2;
	mcu_spi_rw(FP_SPI_NUM, ctrl, tx, tx_len, rx, rx_len);

	/* write cmd 0xF0 & address 0x0124 and 0x0001 */
	/* clear irq, tx length is 7 */
	if ((rx[0] != 0x00) || (rx[1] != 0x00)) {
		tx[0] = 0xF0;
		tx[1] = 0x01;
		tx[2] = 0x24;
		tx[3] = 0x00;
		tx[4] = 0x01;
		tx[5] = rx[0];
		tx[6] = rx[1];
		tx_len = 7;
		mcu_spi_rw(FP_SPI_NUM, ctrl, tx, tx_len, NULL, 0);
	}

	/* set sensor to idle mode, cmd is 0xC0, length is 1 */
	tx[0] = 0xC0;
	tx_len = 1;
	mcu_spi_rw(FP_SPI_NUM, ctrl, tx, tx_len, NULL, 0);

	/* write cmd 0xF0 & address 0x0000, length is 3 */
	tx[0] = 0xF0;
	tx[1] = 0x00;
	tx[2] = 0x00;
	tx_len = 3;
	mcu_spi_rw(FP_SPI_NUM, ctrl, tx, tx_len, NULL, 0);
}

static void set_silead_config(gpio_num_type gpio_reset)
{
	fingerprint_gpio_reset(gpio_reset, RESET_LONG_SLEEP, RESET_LONG_SLEEP);
	msleep(RESET_SHORT_SLEEP);
}

static void read_gpio_info_from_dts(struct device_node *dn)
{
	int temp = 0;

	if (of_property_read_u32(dn, "gpio_irq", &temp))
		ctxhub_err("%s:read gpio_irq fail\n", __func__);
	else
		g_fingerprint_data.gpio_irq = (gpio_num_type)temp;

	if (of_property_read_u32(dn, "gpio_irq_sh", &temp))
		ctxhub_err("%s:read gpio_irq_sh fail\n", __func__);
	else
		g_fingerprint_data.gpio_irq_sh = (gpio_num_type)temp;

	if (of_property_read_u32(dn, "gpio_reset", &temp))
		ctxhub_err("%s:read gpio_reset fail\n", __func__);
	else
		g_fingerprint_data.gpio_reset = (gpio_num_type)temp;

	if (of_property_read_u32(dn, "gpio_reset_sh", &temp))
		ctxhub_err("%s:read gpio_reset_sh fail\n", __func__);
	else
		g_fingerprint_data.gpio_reset_sh = (gpio_num_type)temp;

	if (of_property_read_u32(dn, "gpio_cs", &temp))
		ctxhub_err("%s:read gpio_cs fail\n", __func__);
	else
		g_fingerprint_data.gpio_cs = (gpio_num_type)temp;
}

static void read_id_info_from_dts(struct device_node *dn)
{
	int temp = 0;

	if (of_property_read_u32(dn, "chip_id_register", &temp))
		ctxhub_err("%s:read chip_id_register fail\n", __func__);
	else
		g_fingerprint_data.reg = (uint16_t)temp;

	if (of_property_read_u32(dn, "chip_id_value", &temp))
		ctxhub_err("%s:read chip_id_value fail\n", __func__);
	else
		g_fingerprint_data.chip_id = (uint16_t)temp;

	if (of_property_read_u32(dn, "product_id_value", &temp))
		ctxhub_err("%s:read product_id_value fail\n", __func__);
	else
		g_fingerprint_data.product_id = (uint16_t)temp;
}

static void read_fingerprint_from_dts(struct device_node *dn, struct device_manager_node *node)
{
	int temp = 0;

	read_chip_info(dn, node);

	if (of_property_read_u32(dn, "file_id", &temp))
		ctxhub_err("%s:read fingerprint file_id fail\n", __func__);
	else
		node->dyn_file_id = (uint16_t) temp;

	ctxhub_err("fingerprint  file id is %d\n", temp);

	read_id_info_from_dts(dn);
	read_gpio_info_from_dts(dn);
}

static int fingerprint_sensor_detect(struct device_node *dn)
{
	int ret;
	int irq_value = 0;
	int reset_value = 0;
	char *sensor_vendor = NULL;
	gpio_num_type gpio_reset = 0;
	gpio_num_type gpio_cs = 0;
	gpio_num_type gpio_irq = 0;

	if (fingerprint_get_gpio_config(dn, &gpio_reset,
					&gpio_cs, &gpio_irq) != 0)
		ctxhub_err("%s:read fingerprint gpio fail\n", __func__);

	if (g_device_fingerprint->detect_result == DET_FAIL) {
		reset_value = gpio_get_value(gpio_reset);
		irq_value = gpio_get_value(gpio_irq);
	}

	ret = of_property_read_string(dn, "compatible",
				      (const char **)&sensor_vendor);
	if (!ret) {
		if (!strncmp(sensor_vendor, "fpc", FPC_NAME_LEN)) {
			set_fpc_config(gpio_reset);
		} else if (!strncmp(sensor_vendor, "syna", SYNA_NAME_LEN)) {
			set_syna_config(gpio_reset, gpio_cs);
		} else if (!strncmp(sensor_vendor, "goodix", GOODIX_NAME_LEN)) {
			set_goodix_config(gpio_reset, gpio_cs);
		} else if (!strncmp(sensor_vendor, "silead", SILEAD_NAME_LEN)) {
			set_silead_config(gpio_reset);
		} else if (!strncmp(sensor_vendor, "qfp", QFP_NAME_LEN)) {
			sensor_device_detect(dn, g_device_fingerprint, read_fingerprint_from_dts);
			ctxhub_info("%s: fingerprint device %s detect bypass\n",
				 __func__, sensor_vendor);
			return RET_SUCC;
		}
		ctxhub_info("%s: fingerprint device %s\n", __func__,
			 sensor_vendor);
	} else {
		ctxhub_err("%s: get sensor_vendor err\n", __func__);
		// do nothing
	}

	if (g_device_fingerprint->detect_result == DET_FAIL) {
		if (irq_value == GPIO_STAT_HIGH) {
			gpio_direction_output(gpio_reset, reset_value);
			gpio_direction_output(gpio_irq, irq_value);
		}
		ctxhub_info("%s: irq_value = %d, reset_value = %d\n",
			 __func__, irq_value, reset_value);
	}

	sensor_device_detect(dn, g_device_fingerprint, read_fingerprint_from_dts);
	/* origin implement detect_result will be DET_SUCC, even when detect failed */
	g_device_fingerprint->detect_result = DET_SUCC;
	return RET_SUCC;
}

static int set_fingerprint_cfg_data(void)
{
	int ret = 0;

	if (g_device_fingerprint->detect_result == DET_SUCC)
		ret = send_parameter_to_mcu(g_device_fingerprint,
					    SUB_CMD_SET_PARAMET_REQ);
	return ret;
}

static int fingerprint_device_init(void)
{
	int ret;

	if (get_contexthub_dts_status())
		return -EINVAL;
	g_device_fingerprint = device_manager_node_alloc();
	if (!g_device_fingerprint) {
		ctxhub_err("[%s], g_device_fingerprint alloc failed!\n", __func__);
		return -EINVAL;
	}
	g_device_fingerprint->device_name_str = "fingerprint";
	g_device_fingerprint->tag = TAG_FP;
	g_device_fingerprint->spara = &g_fingerprint_data;
	g_device_fingerprint->cfg_data_length = sizeof(g_fingerprint_data);
	g_device_fingerprint->detect = fingerprint_sensor_detect;

	g_device_fingerprint->cfg = set_fingerprint_cfg_data;
	ret = register_contexthub_device(g_device_fingerprint);
	if (ret != 0) {
		ctxhub_err("%s, register %d fingerprint failed\n",
			__func__, g_device_fingerprint->tag);
		device_manager_node_free(g_device_fingerprint);
		g_device_fingerprint = NULL;
		return ret;
	}

	ctxhub_info("----%s, tag %d--->\n", __func__, g_device_fingerprint->tag);
	return 0;
}

static void __exit fingerprint_device_exit(void)
{
	unregister_contexthub_device(g_device_fingerprint);
	device_manager_node_free(g_device_fingerprint);
}

device_initcall_sync(fingerprint_device_init);
module_exit(fingerprint_device_exit);

MODULE_AUTHOR("Smart");
MODULE_DESCRIPTION("device driver");
MODULE_LICENSE("GPL");
