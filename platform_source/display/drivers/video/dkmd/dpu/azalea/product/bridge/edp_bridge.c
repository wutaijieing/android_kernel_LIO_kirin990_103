/*
 * edp_bridge.c
 *
 * i2c driver for mipi2edp bridge
 *
 * Copyright (c) 2019-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/regmap.h>
#ifdef CONFIG_LCD_KIT_DRIVER
#include "lcd_kit_bl.h"
#endif
#include "edp_bridge.h"

unsigned edp_bridge_msg_level = 7;
static struct gpio_config  gpio_set;
static struct mipi2edp *mipi2edp_pdata = NULL;

static struct gpio_desc bridge_power_set_init_cmds[] = {
	/*lcd vdd enable gpio, set high to enable vdd(3.3V)*/
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,"vcc_3v3_gpio", &gpio_set.lcd_3v3_gpio, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,"vcc_3v3_gpio", &gpio_set.lcd_3v3_gpio, 1},
	/*lcd vdd enable gpio, set high to enable vdd(1.2V)*/
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,"vcc_1v8_gpio", &gpio_set.lcd_1v8_gpio, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,"vcc_1v8_gpio", &gpio_set.lcd_1v8_gpio, 1},
};

static struct gpio_desc bridge_lcd_3v3_enable_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 2,"vcc_3v3_gpio", &gpio_set.lcd_3v3_gpio, 1},
};

static struct gpio_desc bridge_lcd_1v8_enable_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 2,"vcc_1v8_gpio", &gpio_set.lcd_1v8_gpio, 1},
};

static struct gpio_desc bridge_lcd_1v8_disable_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,"vcc_1v8_gpio", &gpio_set.lcd_1v8_gpio, 0},
};

static struct gpio_desc bridge_gpio_reset_init_cmds[] = {
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,"reset_gpio", &gpio_set.reset_gpio, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,"reset_gpio", &gpio_set.reset_gpio, 0},
};

static struct gpio_desc bridge_gpio_reset_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 2,"hw_enable_gpio", &gpio_set.reset_gpio, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 2,"hw_enable_gpio", &gpio_set.reset_gpio, 1},
};

static struct gpio_desc lcd_bl_init_cmds[] = {
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,"bl_bst_gpio", &gpio_set.bl_bst_gpio, 0},
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,"bl_en_gpio", &gpio_set.bl_en_gpio, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 2,"bl_bst_gpio", &gpio_set.bl_bst_gpio, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 2,"bl_en_gpio", &gpio_set.bl_en_gpio, 1},
};

static struct gpio_desc lcd_bl_enable_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 2,"bl_bst_gpio", &gpio_set.bl_bst_gpio, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,"bl_en_gpio", &gpio_set.bl_en_gpio, 1},
};

static struct gpio_desc lcd_bl_disable_cmds[] = {
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 2,"bl_bst_gpio", &gpio_set.bl_bst_gpio, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,"bl_en_gpio", &gpio_set.bl_en_gpio, 0},
};

int is_lt9711a_onboard(struct mipi2edp *pdata)
{
	int i,ret;
	unsigned int reg = 0;
	unsigned int chipid = 0;

	for (i = 0; i < MAX_CHIP_PROBE_TIMES; i++) {
		ret = regmap_read(pdata->regmap,CHIP_ID_REG_0,&reg);
		if (ret)
			continue;
		else
			chipid = (reg&0xFF) << 8;

		ret = regmap_read(pdata->regmap,CHIP_ID_REG_1,&reg);
		if (ret)
			continue;
		else
			chipid |= (reg&0xFF);

		edp_bridge_info("lt9711a is onboard,chipid is 0x%x\n",chipid);
		return ret;
	}

	edp_bridge_err("lt9711a isn't onboard!\n");
	return ret;
}

#ifdef CONFIG_LCD_KIT_DRIVER
static bool g_init_status;

static int lt9711a_set_backlight(uint32_t bl_value)
{
	int ret = -1;

	if (mipi2edp_pdata == NULL) {
		edp_bridge_err("lt9711a_set_backlight: mipi2edp_pdata in NULL\n");
		return ret;
	}

	/* first set backlight, enable backlight and eDP power  */
	if ((g_init_status == false) && (bl_value > 0)) {
		/* 3.3V power on */
		ret = regulator_enable(mipi2edp_pdata->ldo_vcc);
		mdelay(POWER_DELAY_MS);

		/* 1.2V power on */
		ret = regulator_enable(mipi2edp_pdata->ldo_vdd);
		mdelay(POWER_DELAY_MS);

		/* 1.8V power on */
		gpio_cmds_tx(bridge_lcd_1v8_enable_cmds, ARRAY_SIZE(bridge_lcd_1v8_enable_cmds));

		/* reset the bridge chip*/
		gpio_cmds_tx(bridge_gpio_reset_cmds, ARRAY_SIZE(bridge_gpio_reset_cmds));

		/* backlight power on */
		gpio_cmds_tx(lcd_bl_enable_cmds, ARRAY_SIZE(lcd_bl_enable_cmds));

		g_init_status = true;
	}

	/* if set backlight level 0, disable backlight and eDP power */
	if ((g_init_status == true) && (bl_value == 0)) {

		/* backlight power off */
		gpio_cmds_tx(lcd_bl_disable_cmds, ARRAY_SIZE(lcd_bl_disable_cmds));

		/* 3.3V power off */
		ret = regulator_disable(mipi2edp_pdata->ldo_vcc);
		/* 1.2V power off */
		ret = regulator_disable(mipi2edp_pdata->ldo_vdd);
		/* 1.8V power off */
		gpio_cmds_tx(bridge_lcd_1v8_disable_cmds, ARRAY_SIZE(bridge_lcd_1v8_disable_cmds));

		g_init_status = false;
	}
	edp_bridge_info("lt9711a_set_backlight: g_init_status=%d, bl_value=%d\n", g_init_status, bl_value);
	return ret;
}

static struct lcd_kit_bl_ops bl_ops = {
	.set_backlight = lt9711a_set_backlight,
};
#endif

int lt9711a_power_init(struct mipi2edp *pdata, struct device_node *np)
{
	int ret = 0;
	/* get vdd1(1.2V) regulator*/
	pdata->ldo_vdd = devm_regulator_get(pdata->dev, "vdd");
	if (IS_ERR(pdata->ldo_vdd)) {
		ret = PTR_ERR(pdata->ldo_vdd);
		edp_bridge_err("Failed to get 'vdd' regulator: %d\n",ret);
		return ret;
	}

	ret = of_property_read_u32(np, "vdd_supply_voltage", &pdata->vdd_voltage);
	if (ret < 0) {
		pdata->vdd_voltage = 0;
		edp_bridge_info("get vdd_supply_voltage 1.2v failed\n");
		return ret;
	}

	/* bridge chip 3.3V power on */
	gpio_cmds_tx(bridge_power_set_init_cmds, ARRAY_SIZE(bridge_power_set_init_cmds));
	gpio_cmds_tx(bridge_lcd_3v3_enable_cmds, ARRAY_SIZE(bridge_lcd_3v3_enable_cmds));

	/* get vcc(3.3V) regulator*/
	pdata->ldo_vcc = devm_regulator_get(pdata->dev, "vcc");
	if (IS_ERR(pdata->ldo_vcc)) {
		ret = PTR_ERR(pdata->ldo_vcc);
		edp_bridge_err("Failed to get 'vcc' regulator: %d\n",ret);
		return ret;
	}

	ret = of_property_read_u32(np, "vcc_supply_voltage", &pdata->vcc_voltage);
	if (ret < 0) {
		pdata->vcc_voltage = 0;
		edp_bridge_info("get vcc_supply_voltage 3.3v failed\n");
		return ret;
	}

	/* 3.3V power on */
	regulator_set_voltage(pdata->ldo_vcc, pdata->vcc_voltage, pdata->vcc_voltage);
	regulator_enable(pdata->ldo_vcc);
	mdelay(POWER_DELAY_MS);

	/* 1.2V power on */
	regulator_set_voltage(pdata->ldo_vdd, pdata->vdd_voltage, pdata->vdd_voltage);
	regulator_enable(pdata->ldo_vdd);
	mdelay(POWER_DELAY_MS);
	/* 1.8V power on */
	gpio_cmds_tx(bridge_lcd_1v8_enable_cmds, ARRAY_SIZE(bridge_lcd_1v8_enable_cmds));

	/* reset the bridge chip*/
	gpio_cmds_tx(bridge_gpio_reset_init_cmds, ARRAY_SIZE(bridge_gpio_reset_init_cmds));
	gpio_cmds_tx(bridge_gpio_reset_cmds, ARRAY_SIZE(bridge_gpio_reset_cmds));

	return ret;
}

static int mipi2edp_i2c_probe(
	struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = NULL;
	struct mipi2edp *pdata = NULL;
	struct device *dev = NULL;
	struct device_node *np = NULL;
	int ret;

	edp_bridge_info("+");

	adapter = client->adapter;
	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		edp_bridge_err("failed to i2c_check_functionality!");
		return -EOPNOTSUPP;
	}

	dev = &client->dev;
	pdata = devm_kzalloc(dev, sizeof(struct mipi2edp), GFP_KERNEL);
	if (!pdata) {
		edp_bridge_err("failed to alloc i2c!");
		return -ENOMEM;
	}

	if (client->addr == LT9711A_I2C_ADDR) {
		pdata->regmap_config = &lt9711a_regmap;
	} else {
		edp_bridge_err("i2c addr 0x%x is invalid!", client->addr);
		ret = -EINVAL;
		goto err_out;
	}

	pdata->regmap = devm_regmap_init_i2c(client, pdata->regmap_config);
	if (IS_ERR(pdata->regmap)) {
		edp_bridge_err("failed to regmap iit i2c!");
		ret = -ENOMEM;
		goto err_out;
	}

	dev_set_drvdata(dev,pdata);
	pdata->dev = dev;
	pdata->client = client;
	i2c_set_clientdata(client, pdata);

	pdata->gpio_set = (struct gpio_config *)&gpio_set;

	np = client->dev.of_node;
	ret = of_property_read_u32(np, "vcc_3v3_gpio",
		&pdata->gpio_set->lcd_3v3_gpio);
	if (ret < 0) {
		edp_bridge_info("get lcd_3v3_gpio dts config failed!\n");
	}
	ret = of_property_read_u32(np, "vcc_1v8_gpio",
		&pdata->gpio_set->lcd_1v8_gpio);
	if (ret < 0) {
		edp_bridge_err("get lcd_1v8_gpio dts config failed!\n");
		goto err_out;
	}
	ret = of_property_read_u32(np, "reset_gpio",
		&pdata->gpio_set->reset_gpio);
	if (ret < 0) {
		edp_bridge_err("get reset_gpio dts config failed!\n");
		goto err_out;
	}

	ret = of_property_read_u32(np, "bl_bst_gpio",
		&pdata->gpio_set->bl_bst_gpio);
	if (ret < 0)
		edp_bridge_info("get bl_bst_gpio failed\n");

	ret = of_property_read_u32(np, "bl_en_gpio",
		&pdata->gpio_set->bl_en_gpio);
	if (ret < 0) {
		edp_bridge_info("get bl_en_gpio failed\n");
	} else {
		/* backlight power on */
		gpio_cmds_tx(lcd_bl_init_cmds, ARRAY_SIZE(lcd_bl_init_cmds));
	}

	ret = lt9711a_power_init(pdata, np);
	if (ret < 0) {
		edp_bridge_err("bridge chip power on failed!");
		goto err_out;
	}

	ret = is_lt9711a_onboard(pdata);
	if (ret < 0) {
		edp_bridge_err("failed to find the bridge chip!");
		goto err_out;
	}

	/* reset the bridge chip again */
	gpio_cmds_tx(bridge_gpio_reset_cmds, ARRAY_SIZE(bridge_gpio_reset_cmds));
	mipi2edp_pdata = pdata;
#ifdef CONFIG_LCD_KIT_DRIVER
	lcd_kit_bl_register(&bl_ops);
#endif

	edp_bridge_info("-");
	return ret;

err_out:
	devm_kfree(&client->dev, pdata);

	return ret;
}

static int mipi2edp_i2c_remove(struct i2c_client *client)
{
	edp_bridge_info("+");

	if(!client){
		edp_bridge_err("client is nullptr!");
		return -EINVAL;
	}

	edp_bridge_info("-");

	return 0;
}

static const struct i2c_device_id mipi2edp_i2c_ids[] = {
	{ "lt,lt9711a", 0 },
	{}
};

static const struct of_device_id mipi2edp_i2c_of_ids[] = {
	{.compatible = "lt,lt9711a"},
	{}
};


static struct i2c_driver mipi2edp_i2c_driver = {
	.driver = {
		.name = "mipi2edp",
		.of_match_table = mipi2edp_i2c_of_ids,
	},
	.id_table = mipi2edp_i2c_ids,
	.probe = mipi2edp_i2c_probe,
	.remove = mipi2edp_i2c_remove,
};

static int __init mipi2edp_init(void)
{
	int ret;

	edp_bridge_info("+");

	ret = i2c_add_driver(&mipi2edp_i2c_driver);
	if (ret) {
		edp_bridge_err("failed to i2c_add_driver, ret=%d!", ret);
		return ret;
	}

	edp_bridge_info("-");

	return ret;
}
late_initcall(mipi2edp_init);
static void __exit mipi2edp_exit(void)
{
	edp_bridge_info("+");
	i2c_del_driver(&mipi2edp_i2c_driver);
	edp_bridge_info("-");
}
module_exit(mipi2edp_exit);

MODULE_ALIAS("mipi2edp bridge module");
MODULE_DESCRIPTION("mipi2edp bridge driver");
MODULE_LICENSE("GPL");
