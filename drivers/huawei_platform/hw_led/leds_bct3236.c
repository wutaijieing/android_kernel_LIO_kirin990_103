/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 */

#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <huawei_platform/log/hw_log.h>
#include <securec.h>

#include "light_ring.h"

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG bct3236
HWLOG_REGIST();

#define CHIP_BCT3236_NAME "broadchip,bct3236"
/* Register list */
#define REG_COL0     0x01
#define REG_UPDATE   0x25
#define REG_BR0      0x26
#define REG_SHUTDOWN 0x00
#define REG_RESET    0x4f
/* Register default */
#define DEFAULT_SHUTDOWN 0x00
#define DEFAULT_CHIPEN   0x01
#define DEFAULT_UPDATE   0x00
#define DEFAULT_RESET    0x00

static int32_t bct3236_update(struct light_ring_data *data)
{
	int32_t ret;
	unsigned char *buf = NULL;

	if (!data) {
		hwlog_err("%s:invalid args", __func__);
		return -EINVAL;
	}

	buf = data->led_brightness_data;
	ret = light_ring_i2c_write_bytes(data->i2c, REG_BR0, buf, LED_NUM);
	if (ret < 0)
		return ret;

	buf = data->led_color_data;
	ret = light_ring_i2c_write_bytes(data->i2c, REG_COL0, buf, LED_NUM);
	if (ret < 0)
		return ret;

	ret = light_ring_i2c_write_byte(data->i2c, REG_UPDATE, DEFAULT_UPDATE);
	return ret;
}

static int32_t bct3236_chip_init(struct light_ring_data *data)
{
	int32_t i;
	int32_t ret;

	/* power on */
	ret = devm_gpio_request_one(&data->i2c->dev, data->gpio_init, GPIOF_OUT_INIT_HIGH,
		"light-ring-gpio-init");
	if (ret != 0) {
		hwlog_err("%s: request gpio%d init error[0x%x]", __func__, data->gpio_init, ret);
		return ret;
	}

	/* software reset */
	ret = light_ring_i2c_write_byte(data->i2c, REG_RESET, DEFAULT_RESET);
	if (ret < 0)
		return ret;

	/* set normal operation */
	ret = light_ring_i2c_write_byte(data->i2c, REG_SHUTDOWN, DEFAULT_CHIPEN);
	if (ret < 0)
		return ret;

	/* prepare control all leds on, Iout set max */
	for (i = 0; i < LED_NUM; i++)
		data->led_brightness_data[i] = 1;
	return 0;
}

static int32_t bct3236_parse_dts(struct light_ring_data *data, struct device_node *np)
{
	data->gpio_init = of_get_named_gpio(np, "gpio_init", 0);
	if (data->gpio_init < 0) {
		hwlog_err("%s: get gpio_init failed[%d]", __func__, data->gpio_init);
		return -EINVAL;
	}
	return 0;
}

static int32_t bct3236_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	int32_t i;
	struct light_ring_data *data = NULL;
	struct device_node *np = i2c->dev.of_node;

	hwlog_info("%s:begin", __func__);
	data = devm_kzalloc(&i2c->dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	if (light_ring_init_data(i2c, data) != 0)
		goto err_init_data;

	if (!np || (bct3236_parse_dts(data, np) != 0)) {
		hwlog_err("%s: failed to parse device tree node", __func__);
		goto err_parse_dts;
	}
	if (bct3236_chip_init(data) != 0)
		goto err_chip_init;

	light_ring_ext_func_get()->chip_update = bct3236_update;
	if (light_ring_register(data) != 0)
		goto err_light_ring_register;

	hwlog_info("%s:end", __func__);
	return 0;

err_light_ring_register:
err_chip_init:
	if (gpio_is_valid(data->gpio_init))
		devm_gpio_free(&i2c->dev, data->gpio_init);
err_parse_dts:
err_init_data:
	light_ring_deinit_data(data);
	devm_kfree(&i2c->dev, data);
	hwlog_err("%s:failed", __func__);
	return -EINVAL;
}

static int32_t bct3236_i2c_remove(struct i2c_client *i2c)
{
	struct light_ring_data *data = i2c_get_clientdata(i2c);

	hwlog_info("%s:begin", __func__);
	light_ring_unregister(data);
	if (gpio_is_valid(data->gpio_init))
		devm_gpio_free(&i2c->dev, data->gpio_init);
	light_ring_deinit_data(data);
	devm_kfree(&i2c->dev, data);
	hwlog_info("%s:end", __func__);
	return 0;
}

static void bct3236_i2c_shutdown(struct i2c_client *i2c)
{
	hwlog_info("%s:begin", __func__);
	/* software reset */
	(void)light_ring_i2c_write_byte(i2c, REG_RESET, DEFAULT_RESET);
	/* set normal operation */
	(void)light_ring_i2c_write_byte(i2c, REG_SHUTDOWN, DEFAULT_SHUTDOWN);
	hwlog_info("%s:end", __func__);
}

static const struct i2c_device_id bct3236_i2c_id[] = {
	{ CHIP_BCT3236_NAME, 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, bct3236_i2c_id);

static const struct of_device_id bct3236_dt_match[] = {
	{ .compatible = CHIP_BCT3236_NAME },
	{},
};

static struct i2c_driver bct3236_i2c_driver = {
	.driver = {
		.name = CHIP_BCT3236_NAME,
		.owner = THIS_MODULE,
		.of_match_table = bct3236_dt_match,
	},
	.id_table = bct3236_i2c_id,
	.probe = bct3236_i2c_probe,
	.remove = bct3236_i2c_remove,
	.shutdown = bct3236_i2c_shutdown,
};

module_i2c_driver(bct3236_i2c_driver);
MODULE_DESCRIPTION("BCT3236 LED Driver");
MODULE_LICENSE("GPL");