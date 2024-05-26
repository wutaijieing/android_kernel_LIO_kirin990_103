// SPDX-License-Identifier: GPL-2.0
/*
 * stwlc33.c
 *
 * stwlc33 driver
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

#include "stwlc33.h"

#define HWLOG_TAG wireless_stwlc33
HWLOG_REGIST();

static int stwlc33_i2c_read(struct stwlc33_dev_info *di, u8 *cmd, int cmd_len,
	u8 *buf, int buf_len)
{
	int i;

	for (i = 0; i < WLTRX_IC_I2C_RETRY_CNT; i++) {
		if (!power_i2c_read_block(di->client, cmd, cmd_len, buf, buf_len))
			return 0;
		power_usleep(DT_USLEEP_10MS);
	}

	return -EIO;
}

static int stwlc33_i2c_write(struct stwlc33_dev_info *di, u8 *buf, int buf_len)
{
	int i;

	for (i = 0; i < WLTRX_IC_I2C_RETRY_CNT; i++) {
		if (!power_i2c_write_block(di->client, buf, buf_len))
			return 0;
		power_usleep(DT_USLEEP_10MS);
	}

	return -EIO;
}

int stwlc33_read_block(struct stwlc33_dev_info *di, u16 reg, u8 *data, u8 len)
{
	u8 cmd[STWLC33_ADDR_LEN];

	if (!di || !data) {
		hwlog_err("read_block: para null\n");
		return -EINVAL;
	}

	cmd[0] = reg >> POWER_BITS_PER_BYTE;
	cmd[1] = reg & POWER_MASK_BYTE;

	return stwlc33_i2c_read(di, cmd, STWLC33_ADDR_LEN, data, len);
}

int stwlc33_write_block(struct stwlc33_dev_info *di, u16 reg, u8 *data, u8 len)
{
	int ret;
	u8 *cmd = NULL;

	if (!di || !data) {
		hwlog_err("write_block: para null\n");
		return -EINVAL;
	}

	cmd = kzalloc(sizeof(u8) * (STWLC33_ADDR_LEN + len), GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	cmd[0] = reg >> POWER_BITS_PER_BYTE;
	cmd[1] = reg & POWER_MASK_BYTE;
	memcpy(&cmd[STWLC33_ADDR_LEN], data, len);

	ret = stwlc33_i2c_write(di, cmd, STWLC33_ADDR_LEN + len);

	kfree(cmd);
	return ret;
}

static int stwlc33_write_block_u8(struct stwlc33_dev_info *di, u8 reg, u8 *data, u8 len)
{
	int ret;
	u8 *cmd = NULL;

	if (!di || !data) {
		hwlog_err("write_block_u8: para null\n");
		return -EINVAL;
	}

	cmd = kzalloc(sizeof(u8) * (STWLC33_ADDR_LEN_U8 + len), GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	cmd[0] = reg & POWER_MASK_BYTE;
	memcpy(&cmd[STWLC33_ADDR_LEN_U8], data, len);

	ret = stwlc33_i2c_write(di, cmd, STWLC33_ADDR_LEN_U8 + len);

	kfree(cmd);
	return ret;
}

int stwlc33_read_byte(struct stwlc33_dev_info *di, u16 reg, u8 *data)
{
	return stwlc33_read_block(di, reg, data, POWER_BYTE_LEN);
}

int stwlc33_write_byte(struct stwlc33_dev_info *di, u16 reg, u8 data)
{
	return stwlc33_write_block(di, reg, &data, POWER_BYTE_LEN);
}

static int stwlc33_write_byte_u8(struct stwlc33_dev_info *di, u8 reg, u8 data)
{
	return stwlc33_write_block_u8(di, reg, &data, POWER_BYTE_LEN);
}

int stwlc33_read_word(struct stwlc33_dev_info *di, u16 reg, u16 *data)
{
	u8 buff[POWER_WORD_LEN] = { 0 };
	int ret;

	if (!di || !data) {
		hwlog_err("read_word: para null\n");
		return -EINVAL;
	}

	ret = stwlc33_read_block(di, reg, buff, POWER_WORD_LEN);
	if (ret)
		return -EIO;

	*data = (buff[0] | (buff[1] << POWER_BITS_PER_BYTE));
	return 0;
}

int stwlc33_write_word(struct stwlc33_dev_info *di, u16 reg, u16 data)
{
	u8 buff[POWER_WORD_LEN] = { 0 };

	buff[0] = data & POWER_MASK_BYTE;
	buff[1] = data >> POWER_BITS_PER_BYTE;

	return stwlc33_write_block(di, reg, buff, POWER_WORD_LEN);
}

int stwlc33_write_word_mask(struct stwlc33_dev_info *di,
	u16 reg, u16 mask, u16 shift, u16 data)
{
	int ret;
	u16 val = 0;

	ret = stwlc33_read_word(di, reg, &val);
	if (ret)
		return ret;

	val &= ~mask;
	val |= ((data << shift) & mask);

	return stwlc33_write_word(di, reg, val);
}

static int stwlc33_get_chip_id(struct stwlc33_dev_info *di, u16 *chip_id)
{
	return stwlc33_read_word(di, STWLC33_CHIP_ID_ADDR, chip_id);
}

void stwlc33_enable_irq(struct stwlc33_dev_info *di)
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

void stwlc33_disable_irq_nosync(struct stwlc33_dev_info *di)
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

void stwlc33_chip_enable(bool enable, void *dev_data)
{
	int gpio_val;
	struct stwlc33_dev_info *di = dev_data;

	if (!di)
		return;

	gpio_set_value(di->gpio_en, enable ? di->gpio_en_valid_val : !di->gpio_en_valid_val);
	gpio_val = gpio_get_value(di->gpio_en);
	hwlog_info("[chip_enable] gpio is %s now\n", gpio_val ? "high" : "low");
}

void stwlc33_chip_reset(void *dev_data)
{
	int ret;

	ret = stwlc33_write_byte(dev_data, STWLC33_CHIP_RST_ADDR, STWLC33_CHIP_RST_VAL);
	if (ret) {
		hwlog_err("chip_reset: failed\n");
		return;
	}

	hwlog_info("[chip_reset] succ\n");
}

int stwlc33_get_mode(struct stwlc33_dev_info *di, u8 *mode)
{
	return stwlc33_read_byte(di, STWLC33_SYS_MODE_ADDR, mode);
}

int stwlc33_check_nvm_for_power(struct stwlc33_dev_info *di)
{
	int ret;
	u8 val = 0;

	if (!di) {
		hwlog_err("check_nvm_for_power: di null\n");
		return -ENODEV;
	}

	if (!power_cmdline_is_factory_mode())
		return 0;

	ret = stwlc33_write_byte(di, STWLC33_TX_NVM_ADDR, STWLC33_TX_NVM_VAL);
	ret += stwlc33_read_byte(di, STWLC33_TX_NVM_ADDR, &val);
	if (ret) {
		hwlog_err("check_nvm_for_power: tx nvm failed\n");
		return -EIO;
	}
	if (val == STWLC33_TX_NVM_VAL) {
		hwlog_info("[check_nvm_for_power] device is already in 16-bit address mode\n");
		return -EINVAL;
	}

	ret = stwlc33_write_byte_u8(di, STWLC33_TX_I2C_PAGE1_ADDR,
		STWLC33_TX_I2C_PAGE1_VAL); /* switch to i2c page1 */
	ret += stwlc33_write_byte_u8(di, STWLC33_TX_16BIT_ADDR,
		STWLC33_TX_16BIT_VAL); /* switch to 16-bit address mode */
	if (ret) {
		hwlog_err("check_nvm_for_power: change to 16-bit mode failed\n");
		return -EIO;
	}

	hwlog_info("[check_nvm_for_power] succ\n");
	return 0;
}

static void stwlc33_irq_work(struct work_struct *work)
{
	int ret;
	u8 mode = 0;
	struct stwlc33_dev_info *di = container_of(work, struct stwlc33_dev_info, irq_work);

	if (!di) {
		hwlog_err("irq_work: di null\n");
		return;
	}

	/* get System Operating Mode */
	ret = stwlc33_get_mode(di, &mode);
	if (!ret)
		hwlog_info("[irq_work] mode=0x%x\n", mode);

	/* handler interrupt */
	if ((mode & STWLC33_TX_WPCMODE) || (mode & STWLC33_BACKPOWERED))
		stwlc33_tx_mode_irq_handler(di);

	stwlc33_enable_irq(di);
	power_wakeup_unlock(di->wakelock, false);
}

static irqreturn_t stwlc33_interrupt(int irq, void *_di)
{
	struct stwlc33_dev_info *di = _di;

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

static int stwlc33_gpio_init(struct stwlc33_dev_info *di, struct device_node *np)
{
	if (power_gpio_config_output(np, "gpio_en", "stwlc33_en",
		&di->gpio_en, di->gpio_en_valid_val))
		return -EINVAL;

	return 0;
}

static int stwlc33_dev_check(struct stwlc33_dev_info *di, struct device_node *np)
{
	int ret;
	u16 chip_id = 0;

	/* gpio_en init to enable chip i2c */
	ret = stwlc33_gpio_init(di, np);
	if (ret)
		goto gpio_init_fail;

	wlps_control(di->ic_type, WLPS_TX_PWR_SW, true);
	power_usleep(DT_USLEEP_10MS);
	(void)stwlc33_check_nvm_for_power(di);
	ret = stwlc33_get_chip_id(di, &chip_id);
	if (ret) {
		hwlog_err("dev_check: failed\n");
		wlps_control(di->ic_type, WLPS_TX_PWR_SW, false);
		goto chip_id_fail;
	}
	wlps_control(di->ic_type, WLPS_TX_PWR_SW, false);

	hwlog_info("[dev_check] ic_type=%d chip_id=%d\n", di->ic_type, chip_id);

	di->chip_id = chip_id;
	if (chip_id != STWLC33_CHIP_ID) {
		hwlog_err("dev_check: chip_id not match\n");
		goto chip_id_fail;
	}

	return 0;

chip_id_fail:
	gpio_free(di->gpio_en);
gpio_init_fail:
	return -EINVAL;
}

struct device_node *stwlc33_dts_dev_node(void *dev_data)
{
	struct stwlc33_dev_info *di = dev_data;

	if (!di || !di->dev)
		return NULL;

	return di->dev->of_node;
}

static int stwlc33_irq_init(struct stwlc33_dev_info *di, struct device_node *np)
{
	int ret;

	if (power_gpio_config_interrupt(np,
		"gpio_int", "stwlc33_int", &di->gpio_int, &di->irq_int)) {
		ret = -EINVAL;
		goto irq_init_fail_0;
	}

	ret = request_irq(di->irq_int, stwlc33_interrupt, IRQF_TRIGGER_FALLING, "stwlc33_irq", di);
	if (ret) {
		hwlog_err("irq_init: request stwlc33_irq failed\n");
		di->irq_int = -EINVAL;
		goto irq_init_fail_1;
	}
	enable_irq_wake(di->irq_int);
	di->irq_active = true;
	INIT_WORK(&di->irq_work, stwlc33_irq_work);

	return 0;

irq_init_fail_1:
	gpio_free(di->gpio_int);
irq_init_fail_0:
	return ret;
}

static int stwlc33_request_dev_resources(struct stwlc33_dev_info *di, struct device_node *np)
{
	int ret;

	ret = stwlc33_parse_dts(np, di);
	if (ret)
		return ret;

	ret = stwlc33_irq_init(di, np);
	if (ret)
		return ret;

	di->wakelock = power_wakeup_source_register(di->dev, "stwlc33_wakelock");
	mutex_init(&di->mutex_irq);

	return 0;
}

static void stwlc33_free_dev_resources(struct stwlc33_dev_info *di)
{
	if (!di)
		return;

	power_wakeup_source_unregister(di->wakelock);
	mutex_destroy(&di->mutex_irq);
	gpio_free(di->gpio_int);
	free_irq(di->irq_int, di);
}

static void stwlc33_register_pwr_dev_info(struct stwlc33_dev_info *di)
{
	struct power_devices_info_data *pwr_dev_info = NULL;

	pwr_dev_info = power_devices_info_register();
	if (pwr_dev_info) {
		pwr_dev_info->dev_name = di->dev->driver->name;
		pwr_dev_info->dev_id = di->chip_id;
		pwr_dev_info->ver_id = 0;
	}
}

static void stwlc33_ops_unregister(struct wltrx_ic_ops *ops)
{
	if (!ops)
		return;

	kfree(ops->tx_ops);
	kfree(ops->fw_ops);
	kfree(ops->qi_ops);
	kfree(ops);
}

static int stwlc33_ops_register(struct wltrx_ic_ops *ops, struct stwlc33_dev_info *di)
{
	int ret;

	ret = stwlc33_fw_ops_register(ops, di);
	if (ret) {
		hwlog_err("ops_register: register fw_ops failed\n");
		goto register_fail;
	}

	ret = stwlc33_tx_ops_register(ops, di);
	if (ret) {
		hwlog_err("ops_register: register tx_ops failed\n");
		goto register_fail;
	}

	ret = stwlc33_qi_ops_register(ops, di);
	if (ret) {
		hwlog_err("ops_register: register qi_ops failed\n");
		goto register_fail;
	}
	di->g_val.qi_hdl = hwqi_get_handle();

	return 0;

register_fail:
	stwlc33_ops_unregister(ops);
	return ret;
}

static void stwlc33_shutdown(struct i2c_client *client)
{
	int ret;
	struct stwlc33_dev_info *di = i2c_get_clientdata(client);

	if (!di)
		return;

	hwlog_info("[shutdown] ++\n");
	if (stwlc33_tx_in_tx_mode(di)) {
		ret = stwlc33_tx_enable_tx_mode(false, di);
		if (ret)
			hwlog_err("shutdown: disable tx mode failed\n");
	}
	hwlog_info("[shutdown] --\n");
}

static int stwlc33_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	struct stwlc33_dev_info *di = NULL;
	struct device_node *np = NULL;
	struct wltrx_ic_ops *ops = NULL;

	di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
	if (!di)
		return -ENOMEM;
	ops = kzalloc(sizeof(*ops), GFP_KERNEL);
	if (!ops) {
		devm_kfree(&client->dev, di);
		return -ENOMEM;
	}

	di->dev = &client->dev;
	np = di->dev->of_node;
	di->client = client;
	di->ic_type = id->driver_data;
	i2c_set_clientdata(client, di);

	ret = stwlc33_dev_check(di, np);
	if (ret)
		goto dev_ck_fail;

	ret = stwlc33_request_dev_resources(di, np);
	if (ret)
		goto req_dev_res_fail;

	ret = stwlc33_ops_register(ops, di);
	if (ret)
		goto ops_register_fail;

	stwlc33_register_pwr_dev_info(di);

	hwlog_info("wireless_chip probe ok\n");
	return 0;

ops_register_fail:
	power_wakeup_source_unregister(di->wakelock);
	mutex_destroy(&di->mutex_irq);
	stwlc33_free_dev_resources(di);
req_dev_res_fail:
	gpio_free(di->gpio_en);
dev_ck_fail:
	stwlc33_ops_unregister(ops);
	devm_kfree(&client->dev, di);
	return ret;
}

MODULE_DEVICE_TABLE(i2c, stwlc33);
static const struct of_device_id stwlc33_of_match[] = {
	{
		.compatible = "st,stwlc33_aux",
		.data = NULL,
	},
	{},
};

static const struct i2c_device_id stwlc33_i2c_id[] = {
	{ "stwlc33_aux", WLTRX_IC_AUX },
	{}
};

static struct i2c_driver stwlc33_driver = {
	.probe = stwlc33_probe,
	.shutdown = stwlc33_shutdown,
	.id_table = stwlc33_i2c_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "wireless_stwlc33",
		.of_match_table = of_match_ptr(stwlc33_of_match),
	},
};

static int __init stwlc33_init(void)
{
	return i2c_add_driver(&stwlc33_driver);
}

static void __exit stwlc33_exit(void)
{
	i2c_del_driver(&stwlc33_driver);
}

device_initcall(stwlc33_init);
module_exit(stwlc33_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("stwlc33 module driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
