// SPDX-License-Identifier: GPL-2.0
/*
 * stm32g031.c
 *
 * stm32g031 driver
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

#include "stm32g031.h"
#include "stm32g031_i2c.h"
#include "stm32g031_scp.h"
#include "stm32g031_fw.h"
#include "stm32g031_ufcs.h"
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <chipset_common/hwpower/common_module/power_firmware.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_printk.h>
#include <chipset_common/hwpower/common_module/power_devices_info.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <chipset_common/hwmanufac/dev_detect/dev_detect.h>
#endif

#define HWLOG_TAG stm32g031
HWLOG_REGIST();

static ssize_t stm32g031_fw_write(void *dev_data, const char *buf, size_t size)
{
	struct stm32g031_device_info *di = (struct stm32g031_device_info *)dev_data;
	struct power_fw_hdr *hdr = NULL;
	int hdr_size;

	if (!di || !buf)
		return -EINVAL;

	hdr = (struct power_fw_hdr *)buf;
	hdr_size = sizeof(struct power_fw_hdr);

	hwlog_info("size=%ld hdr_size=%d bin_size=%ld\n", size, hdr_size, hdr->bin_size);
	hwlog_info("version_id=%x crc_id=%x patch_id=%x config_id=%x\n",
		hdr->version_id, hdr->crc_id, hdr->patch_id, hdr->config_id);

	stm32g031_fw_update_online_mtp(di, (u8 *)hdr + hdr_size, hdr->bin_size,
		hdr->version_id);
	return size;
}

static bool stm32g031_is_support_scp(struct stm32g031_device_info *di)
{
	if (di->param_dts.scp_support != 0) {
		hwlog_info("support scp charge\n");
		return true;
	}

	return false;
}

bool stm32g031_is_support_fcp(struct stm32g031_device_info *di)
{
	if (di->param_dts.fcp_support != 0) {
		hwlog_info("support fcp charge\n");
		return true;
	}

	return false;
}

static bool stm32g031_is_support_ufcs(struct stm32g031_device_info *di)
{
	if (di->param_dts.ufcs_support != 0) {
		hwlog_info("support ufcs charge\n");
		return true;
	}

	return false;
}

void stm32g031_hard_reset(struct stm32g031_device_info *di)
{
	hwlog_info("hard reset\n");

	/* reset pin pull low */
	(void)gpio_direction_output(di->gpio_reset, 0);
	power_usleep(DT_USLEEP_1MS);

	/* reset pin pull high */
	(void)gpio_direction_output(di->gpio_reset, 1);
	power_usleep(DT_USLEEP_5MS);
}

static void stm32g031_set_low_power_mode(struct stm32g031_device_info *di)
{
	if (di->is_low_power_mode) {
		hwlog_info("is low power mode\n");
		return;
	}

	stm32g031_hard_reset(di);
	hwlog_info("set_low_power_mode\n");
	(void)stm32g031_write_mask(di, STM32G031_SCP_CTL_REG, STM32G031_SCP_CTL_LOW_POWER_MASK,
		STM32G031_SCP_CTL_LOW_POWER_SHIFT, 1);
	di->is_low_power_mode = true;
}

static void stm32g031_exit_low_power_mode(struct stm32g031_device_info *di)
{
	hwlog_info("exit_low_power_mode,type=%d\n", di->param_dts.int_gpio_type);
	if (di->param_dts.int_gpio_type == STM32G031_FW_INT_GPIO_FOR_INT) {
		stm32g031_hard_reset(di);
	} else {
		(void)gpio_direction_output(di->gpio_int, 0);
		power_usleep(DT_USLEEP_2MS);
		(void)gpio_direction_output(di->gpio_int, 1);
	}
	di->is_low_power_mode = false;
}

static int stm32g031_notifier_call(struct notifier_block *nb,
	unsigned long event, void *data)
{
	struct stm32g031_device_info *di = container_of(nb, struct stm32g031_device_info, event_nb);

	if (!di)
		return NOTIFY_OK;

	switch (event) {
	case POWER_NE_USB_DISCONNECT:
		stm32g031_set_low_power_mode(di);
		break;
	case POWER_NE_USB_CONNECT:
		stm32g031_exit_low_power_mode(di);
		hwlog_info("current fw version=0x%x\n", di->fw_ver_id);
		hwlog_info("current fw hw config=0x%x\n", di->param_dts.hw_config);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static void stm32g031_interrupt_work(struct work_struct *work)
{
	struct stm32g031_device_info *di = NULL;
	u8 isr1_val = 0;
	u8 isr2_val = 0;

	di = container_of(work, struct stm32g031_device_info, irq_work);
	if (!di || !di->client) {
		hwlog_err("di is null\n");
		return;
	}

	stm32g031_read_byte(di, STM32G031_UFCS_ISR1_REG, &isr1_val);
	stm32g031_read_byte(di, STM32G031_UFCS_ISR2_REG, &isr2_val);
	di->ufcs_isr[0] |= isr1_val;
	di->ufcs_isr[1] |= isr2_val;

	if (!di->ufcs_communicating_flag &&
		(isr1_val & STM32G031_UFCS_ISR1_DATA_READY_MASK))
		power_event_bnc_notify(POWER_BNT_UFCS,
			POWER_NE_UFCS_REC_UNSOLICITED_DATA, NULL);

	hwlog_info("isr1 [0x%x]=0x%x, isr2 [0x%x]=0x%x\n",
		STM32G031_UFCS_ISR1_REG, isr1_val, STM32G031_UFCS_ISR2_REG, isr2_val);
	enable_irq(di->irq_int);
}

static irqreturn_t stm32g031_interrupt(int irq, void *_di)
{
	struct stm32g031_device_info *di = _di;

	if (!di)
		return IRQ_HANDLED;

	hwlog_info("int happened\n");

	disable_irq_nosync(di->irq_int);
	schedule_work(&di->irq_work);

	return IRQ_HANDLED;
}

static int stm32g031_irq_init(struct stm32g031_device_info *di,
	struct device_node *np)
{
	int ret;

	if (di->param_dts.int_gpio_type == STM32G031_FW_INT_GPIO_FOR_WAKEUP)
		return power_gpio_config_output(np, "gpio_int", "stm32g031_gpio_int",
			&di->gpio_int, 1);

	INIT_WORK(&di->irq_work, stm32g031_interrupt_work);
	ret = power_gpio_config_interrupt(np,
		"gpio_int", "stm32g031_gpio_int", &di->gpio_int, &di->irq_int);
	if (ret)
		return ret;

	ret = request_irq(di->irq_int, stm32g031_interrupt,
		IRQF_TRIGGER_FALLING, "stm32g031_int_irq", di);
	if (ret) {
		hwlog_err("gpio irq request fail\n");
		di->irq_int = -1;
		gpio_free(di->gpio_int);
		return ret;
	}

	enable_irq_wake(di->irq_int);
	return 0;
}

static void stm32g031_ops_register(struct stm32g031_device_info *di)
{
	if (stm32g031_is_support_scp(di))
		stm32g031_hwscp_register(di);

	if (stm32g031_is_support_fcp(di))
		stm32g031_hwfcp_register(di);

	if (stm32g031_is_support_ufcs(di) &&
		(di->param_dts.int_gpio_type == STM32G031_FW_INT_GPIO_FOR_INT))
		stm32g031_hwufcs_register(di);

	power_fw_ops_register("stm32g031", (void *)di,
		NULL, (power_fw_write)stm32g031_fw_write);
}

static int stm32g031_parse_dts(struct device_node *np,
	struct stm32g031_device_info *di)
{
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "scp_support",
		(u32 *)&(di->param_dts.scp_support), 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "fcp_support",
		(u32 *)&(di->param_dts.fcp_support), 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "ufcs_support",
		(u32 *)&(di->param_dts.ufcs_support), 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "wait_time",
		(u32 *)&(di->param_dts.wait_time), 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "hw_config",
		(u32 *)&(di->param_dts.hw_config), 0x20);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "int_gpio_type",
		(u32 *)&(di->param_dts.int_gpio_type), 0);

	if (power_gpio_config_output(np, "gpio_reset", "stm32g031_gpio_reset",
		&di->gpio_reset, 1))
		goto reset_config_fail;

	if (power_gpio_config_output(np, "gpio_enable", "stm32g031_gpio_enable",
		&di->gpio_enable, 0))
		goto int_config_fail;

	return 0;

int_config_fail:
	gpio_free(di->gpio_reset);
reset_config_fail:
	return -EINVAL;
}

static int stm32g031_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct stm32g031_device_info *di = NULL;
	struct device_node *np = NULL;
	struct power_devices_info_data *power_dev_info = NULL;

	if (!client || !client->dev.of_node || !id)
		return -ENODEV;

	di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	di->dev = &client->dev;
	np = di->dev->of_node;
	di->client = client;
	i2c_set_clientdata(client, di);
	di->chip_already_init = 1;
	di->event_nb.notifier_call = stm32g031_notifier_call;

	if (stm32g031_parse_dts(np, di))
		goto fail_free_mem;

	power_msleep(di->param_dts.wait_time, 0, NULL);
	/* need update mtp when mtp is latest */
	if (stm32g031_fw_update_mtp_check(di))
		goto fail_free_gpio;

	if (stm32g031_fw_get_ver_id(di))
		goto fail_free_gpio;

	if (power_event_bnc_register(POWER_BNT_CONNECT, &di->event_nb))
		goto fail_free_gpio;

	if (stm32g031_irq_init(di, np))
		goto fail_free_gpio;

	stm32g031_set_low_power_mode(di);
	mutex_init(&di->scp_detect_lock);
	mutex_init(&di->accp_adapter_reg_lock);
	mutex_init(&di->ufcs_detect_lock);
	stm32g031_ops_register(di);

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	set_hw_dev_flag(DEV_I2C_PROTOCOL_IC);
#endif /* CONFIG_HUAWEI_HW_DEV_DCT */
	power_dev_info = power_devices_info_register();
	if (power_dev_info) {
		power_dev_info->dev_name = di->dev->driver->name;
		power_dev_info->dev_id = di->fw_hw_id;
		power_dev_info->ver_id = di->fw_ver_id;
	}

	return 0;

fail_free_gpio:
	gpio_free(di->gpio_enable);
	gpio_free(di->gpio_reset);
fail_free_mem:
	devm_kfree(&client->dev, di);
	return -EPERM;
}

static int stm32g031_remove(struct i2c_client *client)
{
	struct stm32g031_device_info *di = i2c_get_clientdata(client);

	if (!di)
		return -ENODEV;

	mutex_destroy(&di->scp_detect_lock);
	mutex_destroy(&di->accp_adapter_reg_lock);
	mutex_destroy(&di->ufcs_detect_lock);
	power_event_bnc_unregister(POWER_BNT_CONNECT, &di->event_nb);
	devm_kfree(&client->dev, di);

	return 0;
}

static void stm32g031_shutdown(struct i2c_client *client)
{
	struct stm32g031_device_info *di = i2c_get_clientdata(client);

	if (!di)
		return;
}

MODULE_DEVICE_TABLE(i2c, stm32g031);
static const struct of_device_id stm32g031_of_match[] = {
	{
		.compatible = "stm32g031",
		.data = NULL,
	},
	{},
};

static const struct i2c_device_id stm32g031_i2c_id[] = {
	{ "stm32g031", 0 }, {}
};

static struct i2c_driver stm32g031_driver = {
	.probe = stm32g031_probe,
	.remove = stm32g031_remove,
	.shutdown = stm32g031_shutdown,
	.id_table = stm32g031_i2c_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "stm32g031",
		.of_match_table = of_match_ptr(stm32g031_of_match),
	},
};

static __init int stm32g031_init(void)
{
	return i2c_add_driver(&stm32g031_driver);
}

static __exit void stm32g031_exit(void)
{
	i2c_del_driver(&stm32g031_driver);
}

module_init(stm32g031_init);
module_exit(stm32g031_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("stm32g031 module driver");
MODULE_AUTHOR("huawei Technologies Co., Ltd.");
