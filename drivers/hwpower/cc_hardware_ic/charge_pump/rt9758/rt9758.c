// SPDX-License-Identifier: GPL-2.0
/*
 * rt9758.c
 *
 * charge-pump rt9758 driver
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
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

#include "rt9758.h"
#include <chipset_common/hwpower/common_module/power_i2c.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_devices_info.h>
#include <chipset_common/hwpower/hardware_ic/charge_pump.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG cp_rt9758
HWLOG_REGIST();

static int rt9758_read_byte(struct rt9758_dev_info *di, u8 reg, u8 *data)
{
	int i;

	if (!di) {
		hwlog_err("chip not init\n");
		return -ENODEV;
	}

	for (i = 0; i < CP_I2C_RETRY_CNT; i++) {
		if (!power_i2c_u8_read_byte(di->client, reg, data))
			return 0;
		power_usleep(DT_USLEEP_10MS);
	}

	return -EIO;
}

static int rt9758_write_byte(struct rt9758_dev_info *di, u8 reg, u8 data)
{
	int i;

	if (!di) {
		hwlog_err("chip not init\n");
		return -ENODEV;
	}

	for (i = 0; i < CP_I2C_RETRY_CNT; i++) {
		if (!power_i2c_u8_write_byte(di->client, reg, data))
			return 0;
		power_usleep(DT_USLEEP_10MS);
	}

	return -EIO;
}

static int rt9758_write_mask(struct rt9758_dev_info *di, u8 reg, u8 mask, u8 shift, u8 value)
{
	int ret;
	u8 val = 0;

	ret = rt9758_read_byte(di, reg, &val);
	if (ret)
		return ret;

	val &= ~mask;
	val |= ((value << shift) & mask);

	return rt9758_write_byte(di, reg, val);
}

static int rt9758_chip_init(void *dev_data)
{
	int ret;

	ret = rt9758_write_byte(dev_data, RT9758_CTRL1_REG, RT9758_CTRL1_VOUT_OV_FWD_VAL);
	ret += rt9758_write_byte(dev_data, RT9758_CTRL2_REG, RT9758_CTRL2_VBUS_OVP_VAL);
	ret += rt9758_write_byte(dev_data, RT9758_CTRL3_REG, RT9758_CTRL3_WRX_IRE_OCP_VAL);

	return ret;
}

static int rt9758_reverse_chip_init(void *dev_data)
{
	int ret;

	ret = rt9758_write_byte(dev_data, RT9758_CTRL1_REG, RT9758_CTRL1_VOUT_OV_FWD_VAL);
	ret += rt9758_write_byte(dev_data, RT9758_CTRL2_REG, RT9758_CTRL2_VBUS_OVP_VAL);
	ret += rt9758_write_byte(dev_data, RT9758_CTRL3_REG, RT9758_CTRL3_WRX_IRE_OCP_VAL);
	ret += rt9758_write_byte(dev_data, RT9758_CTRL6_REG, RT9758_CTRL6_MODE_REV_BP_VAL);
	ret += rt9758_write_mask(dev_data, RT9758_CTRL4_REG, RT9758_CTRL4_Q0_CONTROL_MASK,
		RT9758_CTRL4_Q0_CONTROL_SHIFT, RT9758_CTRL4_Q0_CONTROL_EN);

	return ret;
}

static bool rt9758_is_cp_open(void *dev_data)
{
	int ret;
	u8 val = 0;

	ret = rt9758_read_byte(dev_data, RT9758_IC_STAT_REG, &val);
	if (!ret && ((val &  RT9758_IC_STAT_CP_VAL) ==  RT9758_IC_STAT_CP_VAL))
		return true;

	return false;
}

static bool rt9758_is_bp_open(void *dev_data)
{
	int ret;
	u8 val = 0;

	ret = rt9758_read_byte(dev_data, RT9758_IC_STAT_REG, &val);
	if (!ret && ((val & RT9758_IC_STAT_BP_VAL) == RT9758_IC_STAT_BP_VAL))
		return true;

	return false;
}

static int rt9758_set_bp_mode(void *dev_data)
{
	int ret;

	ret = rt9758_write_mask(dev_data, RT9758_CTRL6_REG, RT9758_CTRL6_MODE_BPCP_MASK,
		RT9758_CTRL6_MODE_BPCP_SHIFT, RT9758_CTRL6_MODE_BP_EN);
	if (ret) {
		hwlog_err("set bp mode failed\n");
		return ret;
	}

	return 0;
}

static int rt9758_set_cp_mode(void *dev_data)
{
	int ret;

	ret = rt9758_write_mask(dev_data, RT9758_CTRL6_REG, RT9758_CTRL6_MODE_BPCP_MASK,
		RT9758_CTRL6_MODE_BPCP_SHIFT, RT9758_CTRL6_MODE_CP_EN);
	if (ret) {
		hwlog_err("set cp mode failed\n");
		return ret;
	}

	return 0;
}

static int rt9758_reverse_cp_chip_init(void *dev_data)
{
	int ret;

	ret = rt9758_write_byte(dev_data, RT9758_CTRL1_REG, RT9758_CTRL1_VOUT_OV_FWD_VAL);
	ret += rt9758_write_byte(dev_data, RT9758_CTRL2_REG, RT9758_CTRL2_VBUS_OVP_VAL);
	ret += rt9758_write_byte(dev_data, RT9758_CTRL3_REG, RT9758_CTRL3_WRX_IRE_OCP_VAL);
	ret += rt9758_write_byte(dev_data, RT9758_CTRL6_REG, RT9758_CTRL6_MODE_REV_CP_VAL);
	ret += rt9758_write_mask(dev_data, RT9758_CTRL4_REG, RT9758_CTRL4_Q0_CONTROL_MASK,
		RT9758_CTRL4_Q0_CONTROL_SHIFT, RT9758_CTRL4_Q0_CONTROL_EN);

	return ret;
}

static int rt9758_set_reverse_bp2cp_mode(void *dev_data)
{
	return 0;
}

static int rt9758_set_reverse_bp_mode(void *dev_data)
{
	return 0;
}

static int rt9758_set_reverse_cp_mode(void *dev_data)
{
	return 0;
}

static void rt9758_irq_work(struct work_struct *work)
{
	hwlog_info("[irq_work] ++\n");
}

static irqreturn_t rt9758_irq_handler(int irq, void *p)
{
	struct rt9758_dev_info *di = p;

	if (!di) {
		hwlog_err("di is null\n");
		return IRQ_NONE;
	}

	schedule_work(&di->irq_work);
	return IRQ_HANDLED;
}

static int rt9758_irq_init(struct rt9758_dev_info *di, struct device_node *np)
{
	int ret;

	if (power_gpio_config_interrupt(np,
		"gpio_int", "rt9758_int", &di->gpio_int, &di->irq_int))
		return 0;

	ret = request_irq(di->irq_int, rt9758_irq_handler,
		IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND, "rt9758_irq", di);
	if (ret) {
		hwlog_err("could not request rt9758_irq\n");
		di->irq_int = -EINVAL;
		gpio_free(di->gpio_int);
		return ret;
	}

	enable_irq_wake(di->irq_int);
	INIT_WORK(&di->irq_work, rt9758_irq_work);

	return 0;
}

static int rt9758_device_check(void *dev_data)
{
	int ret;
	struct rt9758_dev_info *di = dev_data;

	if (!di) {
		hwlog_err("di is null\n");
		return -ENODEV;
	}

	if (di->post_probe_done)
		return 0;

	ret = rt9758_read_byte(di, RT9758_DEVICE_INFO_REG, &di->chip_id);
	if (ret) {
		hwlog_err("get chip_id failed\n");
		return ret;
	}

	hwlog_info("chip_id=0x%x\n", di->chip_id);

	return 0;
}

static int rt9758_post_probe(void *dev_data)
{
	struct rt9758_dev_info *di = dev_data;
	struct power_devices_info_data *power_dev_info = NULL;

	if (!di) {
		hwlog_err("di is null\n");
		return -ENODEV;
	}

	if (di->post_probe_done)
		return 0;

	if (rt9758_irq_init(di, di->client->dev.of_node)) {
		hwlog_err("irq init failed\n");
		return -EPERM;
	}

	power_dev_info = power_devices_info_register();
	if (power_dev_info) {
		power_dev_info->dev_name = di->dev->driver->name;
		power_dev_info->dev_id = di->chip_id;
		power_dev_info->ver_id = 0;
	}

	di->post_probe_done = true;
	hwlog_info("post_probe done\n");

	return 0;
}

static struct charge_pump_ops rt9758_ops = {
	.cp_type = CP_TYPE_MAIN,
	.chip_name = "rt9758",
	.dev_check = rt9758_device_check,
	.post_probe = rt9758_post_probe,
	.chip_init = rt9758_chip_init,
	.rvs_chip_init = rt9758_reverse_chip_init,
	.rvs_cp_chip_init = rt9758_reverse_cp_chip_init,
	.set_bp_mode = rt9758_set_bp_mode,
	.set_cp_mode = rt9758_set_cp_mode,
	.set_rvs_bp_mode = rt9758_set_reverse_bp_mode,
	.set_rvs_cp_mode = rt9758_set_reverse_cp_mode,
	.is_cp_open = rt9758_is_cp_open,
	.is_bp_open = rt9758_is_bp_open,
	.set_rvs_bp2cp_mode = rt9758_set_reverse_bp2cp_mode,
};

static struct charge_pump_ops rt9758_ops_aux = {
	.cp_type = CP_TYPE_AUX,
	.chip_name = "rt9758_aux",
	.dev_check = rt9758_device_check,
	.post_probe = rt9758_post_probe,
	.chip_init = rt9758_chip_init,
	.rvs_chip_init = rt9758_reverse_chip_init,
	.rvs_cp_chip_init = rt9758_reverse_cp_chip_init,
	.set_bp_mode = rt9758_set_bp_mode,
	.set_cp_mode = rt9758_set_cp_mode,
	.set_rvs_bp_mode = rt9758_set_reverse_bp_mode,
	.set_rvs_cp_mode = rt9758_set_reverse_cp_mode,
	.is_cp_open = rt9758_is_cp_open,
	.is_bp_open = rt9758_is_bp_open,
	.set_rvs_bp2cp_mode = rt9758_set_reverse_bp2cp_mode,
};

static int rt9758_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	struct rt9758_dev_info *di = NULL;
	struct device_node *np = NULL;

	if (!client || !client->dev.of_node || !id)
		return -ENODEV;

	di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	di->dev = &client->dev;
	np = client->dev.of_node;
	di->client = client;

	if (id->driver_data == CP_TYPE_MAIN) {
		rt9758_ops.dev_data = (void *)di;
		ret = charge_pump_ops_register(&rt9758_ops);
	} else {
		rt9758_ops_aux.dev_data = (void *)di;
		ret = charge_pump_ops_register(&rt9758_ops_aux);
	}
	if (ret)
		goto ops_register_fail;

	i2c_set_clientdata(client, di);
	return 0;

ops_register_fail:
	i2c_set_clientdata(client, NULL);
	devm_kfree(&client->dev, di);
	return ret;
}

static int rt9758_remove(struct i2c_client *client)
{
	struct rt9758_dev_info *di = i2c_get_clientdata(client);

	if (!di)
		return -ENODEV;

	i2c_set_clientdata(client, NULL);
	return 0;
}

MODULE_DEVICE_TABLE(i2c, charge_pump_rt9758);
static const struct of_device_id rt9758_of_match[] = {
	{
		.compatible = "cp_rt9758",
		.data = NULL,
	},
	{
		.compatible = "cp_rt9758_aux",
		.data = NULL,
	},
	{},
};

static const struct i2c_device_id rt9758_i2c_id[] = {
	{ "cp_rt9758", CP_TYPE_MAIN },
	{ "cp_rt9758_aux", CP_TYPE_AUX },
	{},
};

static struct i2c_driver rt9758_driver = {
	.probe = rt9758_probe,
	.remove = rt9758_remove,
	.id_table = rt9758_i2c_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "cp_rt9758",
		.of_match_table = of_match_ptr(rt9758_of_match),
	},
};

static int __init rt9758_init(void)
{
	return i2c_add_driver(&rt9758_driver);
}

static void __exit rt9758_exit(void)
{
	i2c_del_driver(&rt9758_driver);
}

rootfs_initcall(rt9758_init);
module_exit(rt9758_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("rt9758 module driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
