/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 */

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <huawei_platform/log/hw_log.h>
#include <securec.h>

#include "light_ring.h"

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG aw210xx
HWLOG_REGIST();

#define CHIP_AW210XX_NAME "awinic,aw210xx"
/* Register list */
#define REG_GCR    0x00
#define REG_BR0    0x01
#define REG_UPDATE 0x49
#define REG_COL0   0x4a
#define REG_GCCR   0x6e
#define REG_GCR2   0x7a
#define REG_GCR4   0x7c
#define REG_VER    0x7e
#define REG_RESET  0x7f
#define REG_WBR    0x90
#define REG_WBG    0x91
#define REG_WBB    0x92
#define REG_FADEH  0xa6
#define REG_FADEL  0xa7
#define REG_GCOLR  0xa8
#define REG_GCOLG  0xa9
#define REG_GCOLB  0xaa
/* Register default */
#define CHIPID           0x18
#define GCC_MAX          0xff
#define DEFAULT_CHIPEN   0x01
#define DEFAULT_RESET    0x00
#define DEFAULT_UPDATE   0x00
#define BRIGHTNESS_MAX   0xff

#define AW210XX_DELAY    20

// returning negative errno else zero on success
static int32_t aw210xx_update(struct light_ring_data *data)
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

// returning negative errno else zero on success
static int32_t aw210xx_chip_init(struct light_ring_data *data)
{
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

	/* delay 2ms at least */
	msleep(AW210XX_DELAY);

	/* enable chip and enable auto power-saving */
	ret = light_ring_i2c_write_byte(data->i2c, REG_GCR, DEFAULT_CHIPEN);
	if (ret < 0)
		return ret;

	/* check id */
	ret = light_ring_i2c_read(data->i2c, REG_RESET);
	if (ret != CHIPID) {
		hwlog_err("%s: read chipid error[0x%x]", __func__, ret);
		return -EINVAL;
	}

	/* set gccr */
	ret = light_ring_i2c_write_byte(data->i2c, REG_GCCR, GCC_MAX);
	if (ret < 0)
		return ret;

	/* write brightness max */
	ret = memset_s(data->led_brightness_data, LED_NUM, BRIGHTNESS_MAX, LED_NUM);
	if (ret != EOK)
		hwlog_err("%s:memset led_brightness_data error[%d]", __func__, ret);
	return ret;
}

static int32_t aw210xx_parse_dts(struct light_ring_data *data, struct device_node *np)
{
	data->gpio_init = of_get_named_gpio(np, "gpio_init", 0);
	if (data->gpio_init < 0) {
		hwlog_err("%s: get gpio_init failed[%d]", __func__, data->gpio_init);
		return -EINVAL;
	}
	return 0;
}

static int32_t aw210xx_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
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

	if (!np || (aw210xx_parse_dts(data, np) != 0)) {
		hwlog_err("%s: failed to parse device tree node", __func__);
		goto err_parse_dts;
	}
	if (aw210xx_chip_init(data) != 0)
		goto err_chip_init;

	light_ring_ext_func_get()->chip_update = aw210xx_update;
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

static int32_t aw210xx_i2c_remove(struct i2c_client *i2c)
{
	int32_t i;
	struct light_ring_data *data = i2c_get_clientdata(i2c);

	light_ring_unregister(data);
	if (gpio_is_valid(data->gpio_init))
		devm_gpio_free(&i2c->dev, data->gpio_init);
	light_ring_deinit_data(data);
	devm_kfree(&i2c->dev, data);
	return 0;
}

static const struct i2c_device_id aw210xx_i2c_id[] = {
	{ CHIP_AW210XX_NAME, 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, aw210xx_i2c_id);

static const struct of_device_id aw210xx_dt_match[] = {
	{ .compatible = CHIP_AW210XX_NAME },
	{},
};

static struct i2c_driver aw210xx_i2c_driver = {
	.driver = {
		.name = CHIP_AW210XX_NAME,
		.owner = THIS_MODULE,
		.of_match_table = aw210xx_dt_match,
	},
	.id_table = aw210xx_i2c_id,
	.probe = aw210xx_i2c_probe,
	.remove = aw210xx_i2c_remove,
};

module_i2c_driver(aw210xx_i2c_driver);
MODULE_DESCRIPTION("AW210xx LED Driver");
MODULE_LICENSE("GPL v2");
