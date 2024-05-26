// SPDX-License-Identifier: GPL-2.0
/*
 * hc32l110.c
 *
 * hc32l110 driver
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

#include "hc32l110.h"
#include "hc32l110_i2c.h"
#include "hc32l110_scp.h"
#include "hc32l110_fw.h"
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

#define HWLOG_TAG hc32l110
HWLOG_REGIST();

static ssize_t hc32l110_fw_write(void *dev_data, const char *buf, size_t size)
{
	struct hc32l110_device_info *di = (struct hc32l110_device_info *)dev_data;
	struct power_fw_hdr *hdr = NULL;
	int hdr_size;

	if (!di || !buf)
		return -EINVAL;

	hdr = (struct power_fw_hdr *)buf;
	hdr_size = sizeof(struct power_fw_hdr);

	hwlog_info("size=%ld hdr_size=%d bin_size=%ld\n", size, hdr_size, hdr->bin_size);
	hwlog_info("version_id=%x crc_id=%x patch_id=%x config_id=%x\n",
		hdr->version_id, hdr->crc_id, hdr->patch_id, hdr->config_id);

	hc32l110_fw_update_online_mtp(di, (u8 *)hdr + hdr_size, hdr->bin_size,
		hdr->version_id);
	return size;
}

static bool hc32l110_is_support_scp(struct hc32l110_device_info *di)
{
	if (di->param_dts.scp_support != 0) {
		hwlog_info("support scp charge\n");
		return true;
	}

	return false;
}

bool hc32l110_is_support_fcp(struct hc32l110_device_info *di)
{
	if (di->param_dts.fcp_support != 0) {
		hwlog_info("support fcp charge\n");
		return true;
	}

	return false;
}

static void hc32l110_hard_reset(struct hc32l110_device_info *di)
{
	hwlog_info("hard reset\n");

	/* reset pin pull low */
	(void)gpio_direction_output(di->gpio_reset, 0);
	power_usleep(DT_USLEEP_100US);

	/* reset pin pull high */
	(void)gpio_direction_output(di->gpio_reset, 1);
	power_msleep(DT_MSLEEP_50MS, 0, NULL);
}

static void hc32l110_set_low_power_mode(struct hc32l110_device_info *di)
{
	hwlog_info("set_low_power_mode\n");
	(void)hc32l110_write_mask(di, HC32L110_SCP_CTL_REG, HC32L110_SCP_CTL_LOW_POWER_MASK,
		HC32L110_SCP_CTL_LOW_POWER_SHIFT, 1);
	di->is_low_power_mode = true;
}

static void hc32l110_exit_low_power_mode(struct hc32l110_device_info *di)
{
	(void)gpio_direction_output(di->gpio_int, 0);
	power_usleep(DT_USLEEP_2MS);
	(void)gpio_direction_output(di->gpio_int, 1);
	di->is_low_power_mode = false;
}

static int hc32l110_notifier_call(struct notifier_block *nb,
	unsigned long event, void *data)
{
	struct hc32l110_device_info *di = container_of(nb, struct hc32l110_device_info, event_nb);

	if (!di)
		return NOTIFY_OK;

	switch (event) {
	case POWER_NE_USB_DISCONNECT:
		hc32l110_hard_reset(di);
		hc32l110_set_low_power_mode(di);
		break;
	case POWER_NE_USB_CONNECT:
		hc32l110_exit_low_power_mode(di);
		hwlog_info("current fw version=0x%x\n", di->fw_ver_id);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static void hc32l110_ops_register(struct hc32l110_device_info *di)
{
	if (hc32l110_is_support_scp(di))
		hc32l110_hwscp_register(di);

	if (hc32l110_is_support_fcp(di))
		hc32l110_hwfcp_register(di);

	power_fw_ops_register("hc32l110", (void *)di,
		NULL, (power_fw_write)hc32l110_fw_write);
}

static int hc32l110_parse_dts(struct device_node *np,
	struct hc32l110_device_info *di)
{
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "scp_support",
		(u32 *)&(di->param_dts.scp_support), 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "fcp_support",
		(u32 *)&(di->param_dts.fcp_support), 0);
	(void)power_dts_read_u32(power_dts_tag(HWLOG_TAG), np, "wait_time",
		(u32 *)&(di->param_dts.wait_time), 0);

	if (power_gpio_config_output(np, "gpio_reset", "hc32l110_gpio_reset",
		&di->gpio_reset, 1))
		goto reset_config_fail;

	if (power_gpio_config_output(np, "gpio_int", "hc32l110_gpio_int",
		&di->gpio_int, 1))
		goto int_config_fail;

	if (power_gpio_config_output(np, "gpio_enable", "hc32l110_gpio_enable",
		&di->gpio_enable, 0))
		goto enable_config_fail;

	return 0;

enable_config_fail:
	gpio_free(di->gpio_int);
int_config_fail:
	gpio_free(di->gpio_reset);
reset_config_fail:
	return -EINVAL;
}

static int hc32l110_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct hc32l110_device_info *di = NULL;
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
	di->event_nb.notifier_call = hc32l110_notifier_call;

	if (hc32l110_parse_dts(np, di))
		goto fail_free_mem;

	power_msleep(di->param_dts.wait_time, 0, NULL);
	/* need update mtp when mtp is empty */
	if (hc32l110_fw_update_empty_mtp(di))
		goto fail_free_gpio;

	/* need update mtp when mtp is latest */
	if (hc32l110_fw_update_latest_mtp(di))
		goto fail_free_gpio;

	if (hc32l110_fw_get_dev_id(di))
		goto fail_free_gpio;

	if (hc32l110_fw_get_ver_id(di))
		goto fail_free_gpio;

	hc32l110_hard_reset(di);
	hc32l110_set_low_power_mode(di);

	if (power_event_bnc_register(POWER_BNT_CONNECT, &di->event_nb))
		goto fail_free_gpio;

	mutex_init(&di->scp_detect_lock);
	mutex_init(&di->accp_adapter_reg_lock);
	mutex_init(&di->ufcs_detect_lock);
	hc32l110_ops_register(di);

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	set_hw_dev_flag(DEV_I2C_PROTOCOL_IC);
#endif /* CONFIG_HUAWEI_HW_DEV_DCT */
	power_dev_info = power_devices_info_register();
	if (power_dev_info) {
		power_dev_info->dev_name = di->dev->driver->name;
		power_dev_info->dev_id = di->fw_dev_id;
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

static int hc32l110_remove(struct i2c_client *client)
{
	struct hc32l110_device_info *di = i2c_get_clientdata(client);

	if (!di)
		return -ENODEV;

	mutex_destroy(&di->scp_detect_lock);
	mutex_destroy(&di->accp_adapter_reg_lock);
	mutex_destroy(&di->ufcs_detect_lock);
	power_event_bnc_unregister(POWER_BNT_CONNECT, &di->event_nb);
	devm_kfree(&client->dev, di);

	return 0;
}

static void hc32l110_shutdown(struct i2c_client *client)
{
	struct hc32l110_device_info *di = i2c_get_clientdata(client);

	if (!di)
		return;
}

MODULE_DEVICE_TABLE(i2c, hc32l110);
static const struct of_device_id hc32l110_of_match[] = {
	{
		.compatible = "hc32l110",
		.data = NULL,
	},
	{},
};

static const struct i2c_device_id hc32l110_i2c_id[] = {
	{ "hc32l110", 0 }, {}
};

static struct i2c_driver hc32l110_driver = {
	.probe = hc32l110_probe,
	.remove = hc32l110_remove,
	.shutdown = hc32l110_shutdown,
	.id_table = hc32l110_i2c_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "hc32l110",
		.of_match_table = of_match_ptr(hc32l110_of_match),
	},
};

static __init int hc32l110_init(void)
{
	return i2c_add_driver(&hc32l110_driver);
}

static __exit void hc32l110_exit(void)
{
	i2c_del_driver(&hc32l110_driver);
}

module_init(hc32l110_init);
module_exit(hc32l110_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("hc32l110 module driver");
MODULE_AUTHOR("huawei Technologies Co., Ltd.");
