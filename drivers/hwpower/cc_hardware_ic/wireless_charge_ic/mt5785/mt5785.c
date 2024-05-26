// SPDX-License-Identifier: GPL-2.0
/*
 * mt5785.c
 *
 * mt5785 driver
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

#include "mt5785.h"

#define HWLOG_TAG wireless_mt5785
HWLOG_REGIST();

#define MT5785_PINCTRL_LEN                   2

bool mt5785_is_pwr_good(struct mt5785_dev_info *di)
{
	int gpio_val;

	if (!di)
		return false;

	if (!di->g_val.mtp_chk_complete)
		return true;

	gpio_val = gpio_get_value(di->gpio_pwr_good);
	return gpio_val == MT5785_GPIO_PWR_GOOD_VAL;
}

static int mt5785_i2c_read(struct mt5785_dev_info *di, u8 *cmd, int cmd_len,
	u8 *buf, int buf_len)
{
	int i;

	for (i = 0; i < WLTRX_IC_I2C_RETRY_CNT; i++) {
		if (!mt5785_is_pwr_good(di))
			return -EIO;
		if (!power_i2c_read_block(di->client, cmd, cmd_len, buf, buf_len))
			return 0;
		power_usleep(DT_USLEEP_10MS);
	}

	return -EIO;
}

static int mt5785_i2c_write(struct mt5785_dev_info *di, u8 *buf, int buf_len)
{
	int i;

	for (i = 0; i < WLTRX_IC_I2C_RETRY_CNT; i++) {
		if (!mt5785_is_pwr_good(di))
			return -EIO;
		if (!power_i2c_write_block(di->client, buf, buf_len))
			return 0;
		power_usleep(DT_USLEEP_10MS);
	}

	return -EIO;
}

int mt5785_read_block(struct mt5785_dev_info *di, u16 reg, u8 *data, u8 len)
{
	u8 cmd[MT5785_ADDR_LEN];

	if (!di || !data) {
		hwlog_err("read_block: para null\n");
		return -EINVAL;
	}

	cmd[0] = reg >> POWER_BITS_PER_BYTE;
	cmd[1] = reg & POWER_MASK_BYTE;

	return mt5785_i2c_read(di, cmd, MT5785_ADDR_LEN, data, len);
}

int mt5785_write_block(struct mt5785_dev_info *di, u16 reg, u8 *data, u8 data_len)
{
	int ret;
	u8 *cmd = NULL;

	if (!di || !data) {
		hwlog_err("write_block: para null\n");
		return -EINVAL;
	}

	cmd = kzalloc(sizeof(u8) * (MT5785_ADDR_LEN + data_len), GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	cmd[0] = reg >> POWER_BITS_PER_BYTE;
	cmd[1] = reg & POWER_MASK_BYTE;
	memcpy(&cmd[MT5785_ADDR_LEN], data, data_len);

	ret = mt5785_i2c_write(di, cmd, MT5785_ADDR_LEN + data_len);

	kfree(cmd);
	return ret;
}

int mt5785_read_byte(struct mt5785_dev_info *di, u16 reg, u8 *data)
{
	return mt5785_read_block(di, reg, data, POWER_BYTE_LEN);
}

int mt5785_read_word(struct mt5785_dev_info *di, u16 reg, u16 *data)
{
	u8 buff[POWER_WORD_LEN] = { 0 };

	if (!data || mt5785_read_block(di, reg, buff, POWER_WORD_LEN))
		return -EIO;

	*data = buff[0] | (buff[1] << POWER_BITS_PER_BYTE);
	return 0;
}

int mt5785_read_dword(struct mt5785_dev_info *di, u16 reg, u32 *data)
{
	u8 buff[POWER_DWORD_LEN] = { 0 };

	if (!data || mt5785_read_block(di, reg, buff, POWER_DWORD_LEN))
		return -EIO;

	/* 1dword=4bytes, 1byte=8bit */
	*data = buff[0] | (buff[1] << 8) | (buff[2] << 16) | (buff[3] << 24);
	return 0;
}

int mt5785_write_byte(struct mt5785_dev_info *di, u16 reg, u8 data)
{
	return mt5785_write_block(di, reg, &data, POWER_BYTE_LEN);
}

int mt5785_write_word(struct mt5785_dev_info *di, u16 reg, u16 data)
{
	return mt5785_write_block(di, reg, (u8 *)&data, POWER_WORD_LEN);
}

int mt5785_write_dword(struct mt5785_dev_info *di, u16 reg, u32 data)
{
	return mt5785_write_block(di, reg, (u8 *)&data, POWER_DWORD_LEN);
}

int mt5785_read_byte_mask(struct mt5785_dev_info *di, u16 reg, u8 mask, u8 shift, u8 *data)
{
	int ret;
	u8 val = 0;

	ret = mt5785_read_byte(di, reg, &val);
	if (ret)
		return ret;

	val &= mask;
	val >>= shift;
	*data = val;

	return 0;
}

int mt5785_write_byte_mask(struct mt5785_dev_info *di, u16 reg, u8 mask, u8 shift, u8 data)
{
	int ret;
	u8 val = 0;

	ret = mt5785_read_byte(di, reg, &val);
	if (ret)
		return ret;

	val &= ~mask;
	val |= ((data << shift) & mask);

	return mt5785_write_byte(di, reg, val);
}

int mt5785_write_word_mask(struct mt5785_dev_info *di, u16 reg, u16 mask, u16 shift, u16 data)
{
	int ret;
	u16 val = 0;

	ret = mt5785_read_word(di, reg, &val);
	if (ret)
		return ret;

	val &= ~mask;
	val |= ((data << shift) & mask);

	return mt5785_write_word(di, reg, val);
}

int mt5785_read_dword_mask(struct mt5785_dev_info *di, u16 reg, u32 mask, u32 shift, u32 *data)
{
	int ret;
	u32 val = 0;

	ret = mt5785_read_dword(di, reg, &val);
	if (ret)
		return ret;

	val &= mask;
	val >>= shift;
	*data = val;

	return 0;
}

int mt5785_write_dword_mask(struct mt5785_dev_info *di, u16 reg, u32 mask, u32 shift, u32 data)
{
	int ret;
	u32 val = 0;

	ret = mt5785_read_dword(di, reg, &val);
	if (ret)
		return ret;

	val &= ~mask;
	val |= ((data << shift) & mask);

	return mt5785_write_dword(di, reg, val);
}

static int mt5785_get_chip_id(struct mt5785_dev_info *di, u16 *chip_id)
{
	return mt5785_read_word(di, MT5785_CHIP_ID_ADDR, chip_id);
}

int mt5785_get_chip_info(struct mt5785_dev_info *di, struct mt5785_chip_info *info)
{
	int ret;

	if (!info)
		return -EINVAL;

	ret = mt5785_get_chip_id(di, &info->chip_id);
	ret += mt5785_fw_get_mtp_ver(di, &info->mtp_ver);
	if (ret) {
		hwlog_err("get_chip_info: failed\n");
		return ret;
	}

	return 0;
}

int mt5785_get_chip_info_str(char *info_str, int len, void *dev_data)
{
	int ret;
	struct mt5785_chip_info info = { 0 };

	if (!info_str || (len < WLTRX_IC_CHIP_INFO_LEN))
		return -EINVAL;

	ret = mt5785_get_chip_info(dev_data, &info);
	if (ret)
		return -EIO;

	return snprintf(info_str, WLTRX_IC_CHIP_INFO_LEN,
		"chip_id:0x%x mtp_ver=0x%04x", info.chip_id, info.mtp_ver);
}

int mt5785_get_mode(struct mt5785_dev_info *di, u32 *mode)
{
	if (!di || !mode)
		return -ENODEV;

	return mt5785_read_dword(di, MT5785_RX_SYS_MODE_ADDR, mode);
}

void mt5785_enable_irq(struct mt5785_dev_info *di)
{
	if (!di)
		return;

	mutex_lock(&di->mutex_irq);
	if (!di->irq_active) {
		hwlog_info("[enable_irq] ++\n");
		enable_irq(di->irq_int);
		di->irq_active = true;
	}
	hwlog_info("[enable_irq] --\n");
	mutex_unlock(&di->mutex_irq);
}

void mt5785_disable_irq_nosync(struct mt5785_dev_info *di)
{
	if (!di)
		return;

	mutex_lock(&di->mutex_irq);
	if (di->irq_active) {
		hwlog_info("[disable_irq_nosync] ++\n");
		disable_irq_nosync(di->irq_int);
		di->irq_active = false;
	}
	hwlog_info("[disable_irq_nosync] --\n");
	mutex_unlock(&di->mutex_irq);
}

void mt5785_chip_enable(bool enable, void *dev_data)
{
	int gpio_val;
	struct mt5785_dev_info *di = dev_data;

	if (!di)
		return;

	gpio_set_value(di->gpio_en,
		enable ? di->gpio_en_valid_val : !di->gpio_en_valid_val);
	gpio_val = gpio_get_value(di->gpio_en);
	hwlog_info("[chip_enable] gpio %s now\n", gpio_val ? "high" : "low");
}

bool mt5785_is_chip_enable(void *dev_data)
{
	int gpio_val;
	struct mt5785_dev_info *di = dev_data;

	if (!di)
		return false;

	gpio_val = gpio_get_value(di->gpio_en);
	return gpio_val == di->gpio_en_valid_val;
}

static void mt5785_irq_work(struct work_struct *work)
{
	int ret;
	int gpio_val;
	u32 mode = 0;
	struct mt5785_dev_info *di = container_of(work, struct mt5785_dev_info, irq_work);

	if (!di)
		goto exit;

	gpio_val = gpio_get_value(di->gpio_en);
	if (gpio_val != di->gpio_en_valid_val) {
		hwlog_err("irq_work: gpio %s\n", gpio_val ? "high" : "low");
		goto exit;
	}

	ret = mt5785_get_mode(di, &mode);
	if (!ret)
		hwlog_info("[irq_work] sys_mode=0x%x\n", mode);
	else
		mt5785_rx_abnormal_irq_handler(di);

	if (mode & MT5785_TX_SYS_MODE_TX)
		mt5785_tx_mode_irq_handler(di);
	else if (mode & MT5785_RX_SYS_MODE_RX)
		mt5785_rx_mode_irq_handler(di);
exit:
	if (di && !di->g_val.irq_abnormal)
		mt5785_enable_irq(di);

	power_wakeup_unlock(di->wakelock, false);
}

static irqreturn_t mt5785_interrupt(int irq, void *_di)
{
	struct mt5785_dev_info *di = _di;

	if (!di) {
		hwlog_err("interrupt: di null\n");
		return IRQ_HANDLED;
	}

	power_wakeup_lock(di->wakelock, false);
	hwlog_info("[interrupt] ++\n");
	if (di->irq_active) {
		disable_irq_nosync(di->irq_int);
		di->irq_active = false;
		schedule_work(&di->irq_work);
	} else {
		hwlog_info("[interrupt] irq is not enable,do nothing\n");
		power_wakeup_unlock(di->wakelock, false);
	}
	hwlog_info("[interrupt] --\n");
	return IRQ_HANDLED;
}

static void mt5785_fw_mtp_check(struct mt5785_dev_info *di)
{
	u32 mtp_check_delay;

	if (power_cmdline_is_powerdown_charging_mode() ||
		(!power_cmdline_is_factory_mode() && mt5785_rx_is_tx_exist(di))) {
		di->g_val.mtp_chk_complete = true;
		return;
	}

	if (!power_cmdline_is_factory_mode())
		mtp_check_delay = di->mtp_check_delay.user;
	else
		mtp_check_delay = di->mtp_check_delay.fac;

	INIT_DELAYED_WORK(&di->mtp_check_work, mt5785_fw_mtp_check_work);
	schedule_delayed_work(&di->mtp_check_work, msecs_to_jiffies(mtp_check_delay));
}

static void mt5785_register_pwr_dev_info(struct mt5785_dev_info *di)
{
	struct power_devices_info_data *pwr_dev_info = NULL;

	pwr_dev_info = power_devices_info_register();
	if (pwr_dev_info) {
		pwr_dev_info->dev_name = di->dev->driver->name;
		pwr_dev_info->dev_id = di->chip_id;
		pwr_dev_info->ver_id = 0;
	}
}

static void mt5785_ops_unregister(struct wltrx_ic_ops *ops)
{
	if (!ops)
		return;

	kfree(ops->rx_ops);
	kfree(ops->tx_ops);
	kfree(ops->qi_ops);
	kfree(ops->fw_ops);
	kfree(ops->hw_tst_ops);
	kfree(ops);
}

static int mt5785_ops_register(struct wltrx_ic_ops *ops, struct mt5785_dev_info *di)
{
	int ret;

	ret = mt5785_fw_ops_register(ops, di);
	if (ret) {
		hwlog_err("ops_register: register fw_ops failed\n");
		return ret;
	}

	ret = mt5785_rx_ops_register(ops, di);
	if (ret) {
		hwlog_err("ops_register: register rx_ops failed\n");
		return ret;
	}

	ret = mt5785_tx_ops_register(ops, di);
	if (ret) {
		hwlog_err("ops_register: register tx_ops failed\n");
		return ret;
	}

	ret = mt5785_qi_ops_register(ops, di);
	if (ret) {
		hwlog_err("ops_register: register qi_ops failed\n");
		return ret;
	}

	di->g_val.qi_hdl = hwqi_get_handle();
	ret = mt5785_hw_test_ops_register(ops, di);
	if (ret) {
		hwlog_err("ops_register: register hw_test_ops failed\n");
		return ret;
	}

	ret = mt5785_cali_ops_register(ops, di);
	if (ret) {
		hwlog_err("ops_register: register cali_ops failed\n");
		return ret;
	}

	return 0;
}

static int mt5785_gpio_init(struct mt5785_dev_info *di, struct device_node *np)
{
	if (power_gpio_config_output(np, "gpio_en", "mt5785_en",
		&di->gpio_en, di->gpio_en_valid_val))
		goto gpio_en_fail;

	if (power_gpio_config_output(np, "gpio_sleep_en", "mt5785_sleep_en",
		&di->gpio_sleep_en, WLTRX_IC_SLEEP_DISABLE))
		goto gpio_sleep_en_fail;

	if (power_gpio_config_input(np, "gpio_pwr_good", "mt5785_pwr_good",
		&di->gpio_pwr_good))
		goto gpio_pwr_good_fail;

	return 0;

gpio_pwr_good_fail:
	gpio_free(di->gpio_sleep_en);
gpio_sleep_en_fail:
	gpio_free(di->gpio_en);
gpio_en_fail:
	return -EINVAL;
}

static int mt5785_irq_init(struct mt5785_dev_info *di,
	struct device_node *np)
{
	if (power_gpio_config_interrupt(np, "gpio_int", "mt5785_int",
		&di->gpio_int, &di->irq_int))
		return -EINVAL;

	if (request_irq(di->irq_int, mt5785_interrupt,
		IRQF_TRIGGER_FALLING, "mt5785_irq", di)) {
		hwlog_err("irq_init: request irq failed\n");
		gpio_free(di->gpio_int);
		return -EINVAL;
	}

	enable_irq_wake(di->irq_int);
	di->irq_active = true;
	INIT_WORK(&di->irq_work, mt5785_irq_work);
	return 0;
}

static void mt5785_free_dev_resources(struct mt5785_dev_info *di)
{
	power_wakeup_source_unregister(di->wakelock);
	mutex_destroy(&di->mutex_irq);
	gpio_free(di->gpio_int);
	free_irq(di->irq_int, di);
	gpio_free(di->gpio_en);
	gpio_free(di->gpio_sleep_en);
	gpio_free(di->gpio_pwr_good);
}

static int mt5785_request_dev_resources(struct mt5785_dev_info *di, struct device_node *np)
{
	int ret;

	ret = mt5785_parse_dts(np, di);
	if (ret)
		goto parse_dts_fail;

	ret = power_pinctrl_config(di->dev, "pinctrl-names", MT5785_PINCTRL_LEN);
	if (ret)
		return ret;
	ret = mt5785_gpio_init(di, np);
	if (ret)
		goto gpio_init_fail;
	ret = mt5785_irq_init(di, np);
	if (ret)
		goto irq_init_fail;
	di->wakelock = power_wakeup_source_register(di->dev, "mt5785_wakelock");
	mutex_init(&di->mutex_irq);
	return 0;

irq_init_fail:
	gpio_free(di->gpio_en);
	gpio_free(di->gpio_sleep_en);
	gpio_free(di->gpio_pwr_good);
gpio_init_fail:
parse_dts_fail:
	return ret;
}

static int mt5785_dev_check(struct mt5785_dev_info *di)
{
	int ret;
	u16 chip_id = 0;

	wlps_control(di->ic_type, WLPS_RX_EXT_PWR, true);
	power_usleep(DT_USLEEP_10MS);
	ret = mt5785_get_chip_id(di, &chip_id);
	if (ret) {
		hwlog_err("dev_check: failed\n");
		wlps_control(di->ic_type, WLPS_RX_EXT_PWR, false);
		return ret;
	}
	wlps_control(di->ic_type, WLPS_RX_EXT_PWR, false);

	di->chip_id = chip_id;
	hwlog_info("[dev_check] chip_id=0x%04x\n", chip_id);
	return 0;
}

static int mt5785_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret;
	struct mt5785_dev_info *di = NULL;
	struct device_node *np = NULL;
	struct wltrx_ic_ops *ops = NULL;

	if (!client || !id || !client->dev.of_node)
		return -ENODEV;

	if (wlrx_ic_is_ops_registered(id->driver_data) ||
		wltx_ic_is_ops_registered(id->driver_data))
		return 0;

	di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;

	ops = kzalloc(sizeof(*ops), GFP_KERNEL);
	if (!ops) {
		devm_kfree(&client->dev, di);
		return -ENOMEM;
	}

	di->dev = &client->dev;
	np = client->dev.of_node;
	di->client = client;
	di->ic_type = id->driver_data;
	i2c_set_clientdata(client, di);

	ret = mt5785_dev_check(di);
	if (ret) {
		ret = -EPROBE_DEFER;
		goto dev_ck_fail;
	}

	ret = mt5785_request_dev_resources(di, np);
	if (ret)
		goto req_dev_res_fail;

	ret = mt5785_ops_register(ops, di);
	if (ret)
		goto ops_regist_fail;

	wlic_iout_init(di->ic_type, np, NULL);
	mt5785_fw_mtp_check(di);
	mt5785_rx_probe_check_tx_exist(di);
	mt5785_register_pwr_dev_info(di);

	hwlog_info("[probe] ok\n");
	return 0;

ops_regist_fail:
	mt5785_free_dev_resources(di);
req_dev_res_fail:
dev_ck_fail:
	mt5785_ops_unregister(ops);
	devm_kfree(&client->dev, di);
	return ret;
}

static void mt5785_shutdown(struct i2c_client *client)
{
	struct mt5785_dev_info *di = i2c_get_clientdata(client);

	if (!di)
		return;

	mt5785_rx_shutdown_handler(di);
}

static const struct of_device_id mt5785_of_match[] = {
	{
		.compatible = "mt,mt5785",
	},
	{},
};
MODULE_DEVICE_TABLE(of, mt5785_of_match);

static const struct i2c_device_id mt5785_i2c_id[] = {
	{ "mt5785", WLTRX_IC_MAIN }, {}
};
MODULE_DEVICE_TABLE(i2c, mt5785_i2c_id);

static struct i2c_driver mt5785_driver = {
	.probe = mt5785_probe,
	.shutdown = mt5785_shutdown,
	.id_table = mt5785_i2c_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "wireless_mt5785",
		.of_match_table = of_match_ptr(mt5785_of_match),
	},
};

static int __init mt5785_init(void)
{
	return i2c_add_driver(&mt5785_driver);
}

static void __exit mt5785_exit(void)
{
	i2c_del_driver(&mt5785_driver);
}

device_initcall(mt5785_init);
module_exit(mt5785_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("mt5785 module driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
