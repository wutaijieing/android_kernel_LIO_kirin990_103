// SPDX-License-Identifier: GPL-2.0
/*
 * sc200x.c
 *
 * sc200x driver
 *
 * Copyright (c) 2020-2021 Huawei Technologies Co., Ltd.
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

#include "sc200x.h"
#include "sc200x_i2c.h"
#include "sc200x_tcpc.h"
#include "sc200x_protocol.h"
#include <huawei_platform/hwpower/common_module/power_platform.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_sysfs.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_wakeup.h>
#include <chipset_common/hwpower/hardware_monitor/ship_mode.h>

#define HWLOG_TAG sc200x
HWLOG_REGIST();

static struct sc200x_device_info *g_sc200x_dev;

static int sc200x_get_device_id(struct sc200x_device_info *di)
{
	int id;
	int vid;
	int did;

	id = sc200x_read_reg(di, SC200X_REG_ID);
	if (id < 0)
		return -EPERM;

	vid = (id & SC200X_VENDOR_ID_MASK) >> SC200X_VENDOR_ID_SHIFT;
	did = (id & SC200X_DEVICE_ID_MASK) >> SC200X_DEVICE_ID_SHIFT;
	hwlog_info("vendor_id=0x%x, device_id=0x%x\n", vid, did);

	return did;
}

static int sc200x_get_version(struct sc200x_device_info *di)
{
	int ver;
	int major;
	int minor;

	ver = sc200x_read_reg(di, SC200X_REG_FW_VER);
	if (ver < 0)
		return -EPERM;

	major = (ver & SC200X_FW_MAJOR_VER_MASK) >> SC200X_FW_MAJOR_VER_SHIFT;
	minor = (ver & SC200X_FW_MINOR_VER_MASK) >> SC200X_FW_MINOR_VER_SHIFT;
	hwlog_info("version=%d.%d\n", major, minor);

	return ver;
}

static int sc200x_alert(struct sc200x_device_info *di)
{
	int alert_status;

	/* get alert status */
	alert_status = sc200x_read_reg(di, SC200X_REG_ALERT);
	if (alert_status < 0)
		return alert_status;

	hwlog_info("alert_status=%x\n", alert_status);

	/* clear alert status */
	sc200x_write_reg(di, SC200X_REG_ALERT, 0x00);

	/* handle alert event */
	if (alert_status & SC200X_ALERT_ACCP_MASK)
		sc200x_accp_alert_handler(di, (unsigned int)alert_status);

	if (alert_status & SC200X_ALERT_CC_STATUS_MASK)
		sc200x_cc_state_alert_handler(di, (unsigned int)alert_status);

	return SC200X_SUCCESS;
}

static void sc200x_irq_work_handler(struct work_struct *work)
{
	struct sc200x_device_info *di = NULL;

	if (!work)
		return;

	di = container_of(work, struct sc200x_device_info, irq_work);
	if (!di)
		return;

	sc200x_alert(di);
	enable_irq(di->irq_int);
	power_wakeup_unlock(di->wakelock, false);
}

static irqreturn_t sc200x_interrupt(int irq, void *_di)
{
	struct sc200x_device_info *di = _di;

	if (!di)
		return IRQ_NONE;

	power_wakeup_lock(di->wakelock, false);
	disable_irq_nosync(di->irq_int);
	schedule_work(&di->irq_work);

	return IRQ_HANDLED;
}

static int sc200x_irq_init(struct device_node *np,
	struct sc200x_device_info *di)
{
	int ret;
	int gpio_val;

	INIT_WORK(&di->irq_work, sc200x_irq_work_handler);
	ret = power_gpio_config_interrupt(np, "irq_gpio",
		"sc200x_gpio_int", &di->irq_gpio, &di->irq_int);
	if (ret)
		return -EINVAL;

	ret = request_irq(di->irq_int, sc200x_interrupt,
		IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND, "sc200x_int_irq", di);
	if (ret) {
		hwlog_err("gpio irq request fail\n");
		di->irq_int = -1;
		gpio_free(di->irq_gpio);
		cancel_work_sync(&di->irq_work);
		return ret;
	}
	enable_irq_wake(di->irq_int);

	gpio_val = gpio_get_value(di->irq_gpio);
	if (gpio_val == 0) {
		disable_irq(di->irq_int);
		schedule_work(&di->irq_work);
	}

	return 0;
}

static int sc200x_get_chip_info(struct sc200x_device_info *di)
{
	/* device identify */
	di->device_id = sc200x_get_device_id(di);
	if (di->device_id < 0) {
		hwlog_err("cannot get chip info\n");
		return SC200X_FAILED;
	}

	sc200x_get_version(di);

	return SC200X_SUCCESS;
}

static int sc200x_shipmode_handler(void)
{
	if (!g_sc200x_dev)
		return -EINVAL;

	if (g_sc200x_dev->shipmode_enable) {
		g_sc200x_dev->shipmode_enable = false;
		if (sc200x_write_reg(g_sc200x_dev,
			SC200X_REG_PMU_CTRL, SC200X_PMU_CTRL_VALUE0))
			return SC200X_FAILED;

		if (sc200x_write_reg(g_sc200x_dev,
			SC200X_REG_PMU_CTRL, SC200X_PMU_CTRL_VALUE1))
			return SC200X_FAILED;
	}

	return SC200X_SUCCESS;
}

static void sc200x_set_work_mode(unsigned int mode, void *dev_data)
{
	struct sc200x_device_info *di = dev_data;

	if (!di)
		return;

	if (mode != SHIP_MODE_IN_SHIP)
		return;

	di->shipmode_enable = true;
	hwlog_info("set_work_mode succ\n");
}

static struct ship_mode_ops sc200x_ship_mode_ops = {
	.ops_name = "sc200x",
	.set_work_mode = sc200x_set_work_mode,
};

static void sc200x_parse_dts(struct device_node *np,
	struct sc200x_device_info *di)
{
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"fcp_support", &di->fcp_support, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"scp_support", &di->scp_support, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"src_enable", &di->src_enable, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np,
		"rp_value", &di->rp_value, 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG),
		of_find_compatible_node(NULL, NULL, "huawei,charger"),
		"pd_support", &di->pd_support, 0);
}

static int sc200x_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct device_node *node = NULL;
	struct sc200x_device_info *di = NULL;

	if (!client || !client->dev.of_node)
		return -ENODEV;

	di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	g_sc200x_dev = di;
	di->client = client;
	di->dev = &client->dev;
	node = di->dev->of_node;
	i2c_set_clientdata(client, di);
	sc200x_ship_mode_ops.dev_data = di;
	ship_mode_ops_register(&sc200x_ship_mode_ops);

	mutex_init(&di->accp_detect_lock);
	mutex_init(&di->accp_adpt_reg_lock);
	init_completion(&di->accp_rsp_completion);
	init_completion(&di->accp_detect_completion);
	di->wakelock = power_wakeup_source_register(di->dev, "sc200x_wakelock");

	sc200x_parse_dts(node, di);
	sc200x_get_chip_info(di);
	sc200x_tcpc_init(di);
	if (sc200x_irq_init(node, di))
		goto irq_init_fail;

	sc200x_protocol_init(di);

	return 0;

irq_init_fail:
	power_wakeup_source_unregister(di->wakelock);
	mutex_destroy(&di->accp_detect_lock);
	mutex_destroy(&di->accp_adpt_reg_lock);
	devm_kfree(&client->dev, di);
	g_sc200x_dev = NULL;
	return -EPERM;
}

static int sc200x_remove(struct i2c_client *client)
{
	struct sc200x_device_info *di = i2c_get_clientdata(client);

	if (!di)
		return -ENODEV;

	if (di->irq_int)
		free_irq(di->irq_int, di);

	if (di->irq_gpio)
		gpio_free(di->irq_gpio);

	cancel_work_sync(&di->irq_work);
	power_wakeup_source_unregister(di->wakelock);
	mutex_destroy(&di->accp_detect_lock);
	mutex_destroy(&di->accp_adpt_reg_lock);
	devm_kfree(&client->dev, di);
	g_sc200x_dev = NULL;

	return 0;
}

static void sc200x_shutdown(struct i2c_client *client)
{
	struct sc200x_device_info *di = i2c_get_clientdata(client);

	if (!di)
		return;

	/* clear accp enable bit */
	sc200x_write_reg_mask(di, SC200X_REG_DEV_CTRL,
		0x00, SC200X_ACCP_EN_MASK);

	if (sc200x_shipmode_handler())
		hwlog_err("enter shipmode failed\n");
}

static const struct of_device_id sc200x_ids[] = {
	{ .compatible = "huawei,southchip_sc200x" },
	{},
};
MODULE_DEVICE_TABLE(of, sc200x_ids);

static const struct i2c_device_id sc200x_i2c_id[] = {
	{ "sc200x", 0 },
	{}
};

static struct i2c_driver sc200x_i2c_driver = {
	.probe = sc200x_probe,
	.remove = sc200x_remove,
	.shutdown = sc200x_shutdown,
	.id_table = sc200x_i2c_id,
	.driver = {
		.name = "sc200x",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(sc200x_ids),
	},
};

static __init int sc200x_init(void)
{
	return i2c_add_driver(&sc200x_i2c_driver);
}

static __exit void sc200x_exit(void)
{
	i2c_del_driver(&sc200x_i2c_driver);
}

device_initcall_sync(sc200x_init);
module_exit(sc200x_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("huawei charge protocol ic(sc200x) driver");
MODULE_AUTHOR("huawei Technologies Co., Ltd.");
