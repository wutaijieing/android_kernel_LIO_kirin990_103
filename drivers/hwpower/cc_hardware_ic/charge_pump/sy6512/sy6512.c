// SPDX-License-Identifier: GPL-2.0
/*
 * sy6512.c
 *
 * charge-pump sy6512 driver
 *
 * Copyright (c) 2022-2022 Huawei Technologies Co., Ltd.
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

#include "sy6512.h"
#include <chipset_common/hwpower/common_module/power_i2c.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_devices_info.h>
#include <chipset_common/hwpower/hardware_ic/charge_pump.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG cp_sy6512
HWLOG_REGIST();

static int sy6512_read_byte(struct sy6512_dev_info *di, u8 reg, u8 *data)
{
	int i;

	if (!di) {
		hwlog_err("read_byte: chip not init\n");
		return -ENODEV;
	}

	for (i = 0; i < CP_I2C_RETRY_CNT; i++) {
		if (!power_i2c_u8_read_byte(di->client, reg, data))
			return 0;
		power_usleep(DT_USLEEP_10MS);
	}

	return -EIO;
}

static int sy6512_write_byte(struct sy6512_dev_info *di, u8 reg, u8 data)
{
	int i;

	if (!di) {
		hwlog_err("write_byte: chip not init\n");
		return -ENODEV;
	}

	for (i = 0; i < CP_I2C_RETRY_CNT; i++) {
		if (!power_i2c_u8_write_byte(di->client, reg, data))
			return 0;
		power_usleep(DT_USLEEP_10MS);
	}

	return -EIO;
}

static int sy6512_write_mask(struct sy6512_dev_info *di, u8 reg, u8 mask, u8 shift, u8 value)
{
	int ret;
	u8 val = 0;

	ret = sy6512_read_byte(di, reg, &val);
	if (ret)
		return ret;

	val &= ~mask;
	val |= ((value << shift) & mask);

	return sy6512_write_byte(di, reg, val);
}

static int sy6512_read_mask(struct sy6512_dev_info *di, u8 reg, u8 mask, u8 shift, u8 *value)
{
	int ret;
	u8 val = 0;

	ret = sy6512_read_byte(di, reg, &val);
	if (ret)
		return ret;

	val &= mask;
	val >>= shift;
	*value = val;

	return ret;
}

static int sy6512_software_enable(struct sy6512_dev_info *di)
{
	return sy6512_write_mask(di, SY6512_ENABLE_REG, SY6512_EN_SOFT_MASK,
		SY6512_EN_SOFT_SHIFT, SY6512_SOFT_ENABLE);
}

static int sy6512_chip_init_common(struct sy6512_dev_info *di)
{
	int ret;

	/* diable watchdog */
	ret = sy6512_write_mask(di, SY6512_WD_BUS_OVP_REG,
		SY6512_WD_EN_MASK, SY6512_WD_EN_SHIFT, SY6512_WD_DISABLE);
	ret += sy6512_software_enable(di);
	return ret;
}

static int sy6512_switch_mode(struct sy6512_dev_info *di, u8 mode_val)
{
	return sy6512_write_mask(di, SY6512_CFG_REG1, SY6512_MODE_CONTROL_MASK,
		SY6512_MODE_CONTROL_SHIFT, mode_val);
}

static int sy6512_chip_init(void *dev_data)
{
	struct sy6512_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	return sy6512_chip_init_common(di);
}

static int sy6512_reverse_chip_init(void *dev_data)
{
	int ret;
	struct sy6512_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	ret = sy6512_switch_mode(di, SY6512_REVERSE_BP_MODE_VAL);
	ret += sy6512_chip_init_common(di);
	return ret;
}

static int sy6512_reverse_cp_chip_init(void *dev_data)
{
	int ret;
	struct sy6512_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	ret = sy6512_switch_mode(di, SY6512_REVERSE_CP_MODE_VAL);
	ret += sy6512_chip_init_common(di);
	return ret;
}

static bool sy6512_is_cp_open(void *dev_data)
{
	u8 mode = 0;
	int ret;
	struct sy6512_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	ret = sy6512_read_mask(di, SY6512_CFG_REG1, SY6512_MODE_CONTROL_MASK,
		SY6512_MODE_CONTROL_SHIFT, &mode);
	if (!ret && (mode == SY6512_CP_MODE_VAL))
		return true;

	hwlog_info("[is_cp_open] ret=%d, mode=0x%x\n", ret, mode);
	return false;
}

static bool sy6512_is_bp_open(void *dev_data)
{
	u8 mode = 0;
	int ret;
	struct sy6512_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	ret = sy6512_read_mask(di, SY6512_CFG_REG1, SY6512_MODE_CONTROL_MASK,
		SY6512_MODE_CONTROL_SHIFT, &mode);
	if (!ret && (mode == SY6512_BP_MODE_VAL))
		return true;

	hwlog_info("[is_bp_open] ret=%d, mode=0x%x\n", ret, mode);
	return false;
}

static int sy6512_set_bp_mode(void *dev_data)
{
	int ret;
	struct sy6512_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	ret = sy6512_switch_mode(di, SY6512_BP_MODE_VAL);
	if (ret) {
		hwlog_err("set_bp_mode: failed\n");
		return ret;
	}

	return 0;
}

static int sy6512_set_cp_mode(void *dev_data)
{
	int ret;
	struct sy6512_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	ret = sy6512_switch_mode(di, SY6512_CP_MODE_VAL);
	if (ret) {
		hwlog_err("set_cp_mode: failed\n");
		return ret;
	}

	return 0;
}

static int sy6512_device_check(void *dev_data)
{
	int ret;
	u8 status = 0;
	struct sy6512_dev_info *di = dev_data;

	if (!di) {
		hwlog_err("device_check: di is null\n");
		return -ENODEV;
	}

	if (di->post_probe_done)
		return 0;

	ret = sy6512_read_mask(di, SY6512_DEVICE_STATUS_REG, SY6512_POWER_READY_MASK,
		SY6512_POWER_READY_SHIFT, &status);
	if (ret) {
		hwlog_err("device_check: failed\n");
		return ret;
	}

	return 0;
}

static void sy6512_irq_work(struct work_struct *work)
{
	u8 status = 0;
	int ret;
	struct sy6512_dev_info *di = container_of(work,
		struct sy6512_dev_info, irq_work);

	if (!di) {
		hwlog_err("irq_work: di is null\n");
		return;
	}

	ret = sy6512_read_byte(di, SY6512_STATUS_REG, &status);
	hwlog_info("int happened, ret=%d, status=0x%x\n", ret, status);
}

static irqreturn_t sy6512_irq_handler(int irq, void *p)
{
	struct sy6512_dev_info *di = p;

	if (!di) {
		hwlog_err("irq_handler: di is null\n");
		return IRQ_NONE;
	}

	schedule_work(&di->irq_work);
	return IRQ_HANDLED;
}

static int sy6512_irq_init(struct sy6512_dev_info *di, struct device_node *np)
{
	int ret;

	if (power_gpio_config_interrupt(np,
		"gpio_int", "sy6512_int", &di->gpio_int, &di->irq_int))
		return 0;

	ret = request_irq(di->irq_int, sy6512_irq_handler,
		IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND, "sy6512_irq", di);
	if (ret) {
		hwlog_err("could not request sy6512_irq\n");
		di->irq_int = -EINVAL;
		gpio_free(di->gpio_int);
		return ret;
	}

	enable_irq_wake(di->irq_int);
	INIT_WORK(&di->irq_work, sy6512_irq_work);

	return 0;
}

static int sy6512_post_probe(void *dev_data)
{
	struct sy6512_dev_info *di = dev_data;
	struct power_devices_info_data *power_dev_info = NULL;

	if (!di) {
		hwlog_err("post_probe: di is null\n");
		return -ENODEV;
	}

	if (di->post_probe_done)
		return 0;

	if (sy6512_irq_init(di, di->client->dev.of_node)) {
		hwlog_err("irq init failed\n");
		return -EPERM;
	}

	power_dev_info = power_devices_info_register();
	if (power_dev_info) {
		power_dev_info->dev_name = di->dev->driver->name;
		power_dev_info->dev_id = 0;
		power_dev_info->ver_id = 0;
	}

	di->post_probe_done = true;
	hwlog_info("[post_probe] done\n");

	return 0;
}

static struct charge_pump_ops sy6512_ops = {
	.cp_type = CP_TYPE_MAIN,
	.chip_name = "sy6512",
	.dev_check = sy6512_device_check,
	.post_probe = sy6512_post_probe,
	.chip_init = sy6512_chip_init,
	.rvs_chip_init = sy6512_reverse_chip_init,
	.rvs_cp_chip_init = sy6512_reverse_cp_chip_init,
	.set_bp_mode = sy6512_set_bp_mode,
	.set_cp_mode = sy6512_set_cp_mode,
	.is_cp_open = sy6512_is_cp_open,
	.is_bp_open = sy6512_is_bp_open,
};

static int sy6512_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	struct sy6512_dev_info *di = NULL;
	struct device_node *np = NULL;

	hwlog_info("[probe] begin\n");

	if (!client || !client->dev.of_node || !id)
		return -ENODEV;

	di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	di->dev = &client->dev;
	np = client->dev.of_node;
	di->client = client;

	sy6512_ops.dev_data = (void *)di;
	ret = charge_pump_ops_register(&sy6512_ops);
	if (ret)
		goto ops_register_fail;

	i2c_set_clientdata(client, di);
	hwlog_info("[probe] end\n");
	return 0;

ops_register_fail:
	i2c_set_clientdata(client, NULL);
	devm_kfree(&client->dev, di);
	return ret;
}

static int sy6512_remove(struct i2c_client *client)
{
	struct sy6512_dev_info *di = i2c_get_clientdata(client);

	if (!di)
		return -ENODEV;

	i2c_set_clientdata(client, NULL);
	return 0;
}

MODULE_DEVICE_TABLE(i2c, charge_pump_sy6512);
static const struct of_device_id sy6512_of_match[] = {
	{
		.compatible = "cp_sy6512",
		.data = NULL,
	},
	{},
};

static const struct i2c_device_id sy6512_i2c_id[] = {
	{ "cp_sy6512", CP_TYPE_MAIN },
	{},
};

static struct i2c_driver sy6512_driver = {
	.probe = sy6512_probe,
	.remove = sy6512_remove,
	.id_table = sy6512_i2c_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "cp_sy6512",
		.of_match_table = of_match_ptr(sy6512_of_match),
	},
};

static int __init sy6512_init(void)
{
	return i2c_add_driver(&sy6512_driver);
}

static void __exit sy6512_exit(void)
{
	i2c_del_driver(&sy6512_driver);
}

rootfs_initcall(sy6512_init);
module_exit(sy6512_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("sy6512 module driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
