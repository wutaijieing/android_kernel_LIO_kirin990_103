// SPDX-License-Identifier: GPL-2.0
/*
 * cps4067.c
 *
 * cps4067 driver
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

#include "cps4067.h"

#define HWLOG_TAG wireless_cps4067
HWLOG_REGIST();

bool cps4067_is_pwr_good(struct cps4067_dev_info *di)
{
	int gpio_val;

	if (!di)
		return false;

	if (!di->g_val.mtp_chk_complete)
		return true;

	gpio_val = gpio_get_value(di->gpio_pwr_good);
	return gpio_val == CPS4067_GPIO_PWR_GOOD_VAL;
}

static int cps4067_i2c_read(struct cps4067_dev_info *di, u8 *cmd, int cmd_len,
	u8 *buf, int buf_len)
{
	int i;

	for (i = 0; i < WLTRX_IC_I2C_RETRY_CNT; i++) {
		if (!cps4067_is_pwr_good(di))
			return -EIO;
		if (!power_i2c_read_block(di->client, cmd, cmd_len, buf, buf_len))
			return 0;
		power_usleep(DT_USLEEP_10MS);
	}

	return -EIO;
}

static int cps4067_i2c_write(struct cps4067_dev_info *di, u8 *buf, int buf_len)
{
	int i;

	for (i = 0; i < WLTRX_IC_I2C_RETRY_CNT; i++) {
		if (!cps4067_is_pwr_good(di))
			return -EIO;
		if (!power_i2c_write_block(di->client, buf, buf_len))
			return 0;
		power_usleep(DT_USLEEP_10MS);
	}

	return -EIO;
}

int cps4067_read_block(struct cps4067_dev_info *di, u16 reg, u8 *data, u8 len)
{
	u8 cmd[CPS4067_ADDR_LEN];

	if (!di || !data) {
		hwlog_err("read_block: para null\n");
		return -EINVAL;
	}

	cmd[0] = reg >> POWER_BITS_PER_BYTE;
	cmd[1] = reg & POWER_MASK_BYTE;

	return cps4067_i2c_read(di, cmd, CPS4067_ADDR_LEN, data, len);
}

int cps4067_write_block(struct cps4067_dev_info *di, u16 reg, u8 *data, u8 data_len)
{
	int ret;
	u8 *cmd = NULL;

	if (!di || !data) {
		hwlog_err("write_block: para null\n");
		return -EINVAL;
	}

	cmd = kzalloc(sizeof(u8) * (CPS4067_ADDR_LEN + data_len), GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	cmd[0] = reg >> POWER_BITS_PER_BYTE;
	cmd[1] = reg & POWER_MASK_BYTE;
	memcpy(&cmd[CPS4067_ADDR_LEN], data, data_len);

	ret = cps4067_i2c_write(di, cmd, CPS4067_ADDR_LEN + data_len);

	kfree(cmd);
	return ret;
}

int cps4067_read_byte(struct cps4067_dev_info *di, u16 reg, u8 *data)
{
	return cps4067_read_block(di, reg, data, POWER_BYTE_LEN);
}

int cps4067_read_word(struct cps4067_dev_info *di, u16 reg, u16 *data)
{
	u8 buff[POWER_WORD_LEN] = { 0 };

	if (!data || cps4067_read_block(di, reg, buff, POWER_WORD_LEN))
		return -EIO;

	*data = buff[0] | (buff[1] << POWER_BITS_PER_BYTE);
	return 0;
}

int cps4067_read_dword(struct cps4067_dev_info *di, u16 reg, u32 *data)
{
	u8 buff[POWER_DWORD_LEN] = { 0 };

	if (!data || cps4067_read_block(di, reg, buff, POWER_DWORD_LEN))
		return -EIO;

	/* 1dword=4bytes, 1byte=8bit */
	*data = buff[0] | (buff[1] << 8) | (buff[2] << 16) | (buff[3] << 24);
	return 0;
}

int cps4067_write_byte(struct cps4067_dev_info *di, u16 reg, u8 data)
{
	return cps4067_write_block(di, reg, &data, POWER_BYTE_LEN);
}

int cps4067_write_word(struct cps4067_dev_info *di, u16 reg, u16 data)
{
	return cps4067_write_block(di, reg, (u8 *)&data, POWER_WORD_LEN);
}

int cps4067_write_dword(struct cps4067_dev_info *di, u16 reg, u32 data)
{
	return cps4067_write_block(di, reg, (u8 *)&data, POWER_DWORD_LEN);
}

int cps4067_read_byte_mask(struct cps4067_dev_info *di, u16 reg, u8 mask, u8 shift, u8 *data)
{
	int ret;
	u8 val = 0;

	ret = cps4067_read_byte(di, reg, &val);
	if (ret)
		return ret;

	val &= mask;
	val >>= shift;
	*data = val;

	return 0;
}

int cps4067_write_byte_mask(struct cps4067_dev_info *di, u16 reg, u8 mask, u8 shift, u8 data)
{
	int ret;
	u8 val = 0;

	ret = cps4067_read_byte(di, reg, &val);
	if (ret)
		return ret;

	val &= ~mask;
	val |= ((data << shift) & mask);

	return cps4067_write_byte(di, reg, val);
}

int cps4067_write_word_mask(struct cps4067_dev_info *di, u16 reg, u16 mask, u16 shift, u16 data)
{
	int ret;
	u16 val = 0;

	ret = cps4067_read_word(di, reg, &val);
	if (ret)
		return ret;

	val &= ~mask;
	val |= ((data << shift) & mask);

	return cps4067_write_word(di, reg, val);
}

int cps4067_read_dword_mask(struct cps4067_dev_info *di, u16 reg, u32 mask, u32 shift, u32 *data)
{
	int ret;
	u32 val = 0;

	ret = cps4067_read_dword(di, reg, &val);
	if (ret)
		return ret;

	val &= mask;
	val >>= shift;
	*data = val;

	return 0;
}

int cps4067_write_dword_mask(struct cps4067_dev_info *di, u16 reg, u32 mask, u32 shift, u32 data)
{
	int ret;
	u32 val = 0;

	ret = cps4067_read_dword(di, reg, &val);
	if (ret)
		return ret;

	val &= ~mask;
	val |= ((data << shift) & mask);

	return cps4067_write_dword(di, reg, val);
}

int cps4067_hw_read_block(struct cps4067_dev_info *di, u32 addr, u8 *data, u8 len)
{
	u8 cmd[CPS4067_HW_ADDR_LEN];

	if (!di || !data) {
		hwlog_err("4addr_read_block: para null\n");
		return -EINVAL;
	}

	/* 1dword=4bytes, 1byte=8bits 0xff: byte mask */
	cmd[0] = (u8)((addr >> 24) & 0xff);
	cmd[1] = (u8)((addr >> 16) & 0xff);
	cmd[2] = (u8)((addr >> 8) & 0xff);
	cmd[3] = (u8)((addr >> 0) & 0xff);

	return cps4067_i2c_read(di, cmd, CPS4067_HW_ADDR_LEN, data, len);
}

int cps4067_hw_write_block(struct cps4067_dev_info *di, u32 addr, u8 *data, u8 data_len)
{
	int ret;
	u8 *cmd = NULL;

	if (!di || !data) {
		hwlog_err("4addr_write_block: para null\n");
		return -EINVAL;
	}

	cmd = kzalloc(sizeof(u8) * (CPS4067_HW_ADDR_LEN + data_len), GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	/* 1dword=4bytes, 1byte=8bits 0xff: byte mask */
	cmd[0] = (u8)((addr >> 24) & 0xff);
	cmd[1] = (u8)((addr >> 16) & 0xff);
	cmd[2] = (u8)((addr >> 8) & 0xff);
	cmd[3] = (u8)((addr >> 0) & 0xff);
	memcpy(&cmd[CPS4067_HW_ADDR_LEN], data, data_len);

	ret = cps4067_i2c_write(di, cmd, CPS4067_HW_ADDR_LEN + data_len);

	kfree(cmd);
	return ret;
}

/*
 * cps4067 chip_info
 */
static int cps4067_get_chip_id(struct cps4067_dev_info *di, u16 *chip_id)
{
	return cps4067_read_word(di, CPS4067_CHIP_ID_ADDR, chip_id);
}

int cps4067_get_chip_info(struct cps4067_dev_info *di, struct cps4067_chip_info *info)
{
	int ret;

	if (!info)
		return -EINVAL;

	ret = cps4067_get_chip_id(di, &info->chip_id);
	ret += cps4067_fw_get_mtp_ver(di, &info->mtp_ver);
	if (ret) {
		hwlog_err("get_chip_info: failed\n");
		return ret;
	}

	return 0;
}

int cps4067_get_chip_info_str(char *info_str, int len, void *dev_data)
{
	int ret;
	struct cps4067_chip_info info = { 0 };

	if (!info_str || (len < WLTRX_IC_CHIP_INFO_LEN))
		return -EINVAL;

	ret = cps4067_get_chip_info(dev_data, &info);
	if (ret)
		return -EIO;

	return snprintf(info_str, WLTRX_IC_CHIP_INFO_LEN,
		"chip_id:0x%x mtp_ver=0x%04x", info.chip_id, info.mtp_ver);
}

int cps4067_get_mode(struct cps4067_dev_info *di, u8 *mode)
{
	return cps4067_read_byte(di, CPS4067_SYS_MODE_ADDR, mode);
}

void cps4067_enable_irq(struct cps4067_dev_info *di)
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

void cps4067_disable_irq_nosync(struct cps4067_dev_info *di)
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

void cps4067_chip_enable(bool enable, void *dev_data)
{
	int gpio_val;
	struct cps4067_dev_info *di = dev_data;

	if (!di)
		return;

	gpio_set_value(di->gpio_en,
		enable ? di->gpio_en_valid_val : !di->gpio_en_valid_val);
	gpio_val = gpio_get_value(di->gpio_en);
	hwlog_info("[chip_enable] gpio %s now\n", gpio_val ? "high" : "low");
}

bool cps4067_is_chip_enable(void *dev_data)
{
	int gpio_val;
	struct cps4067_dev_info *di = dev_data;

	if (!di)
		return false;

	gpio_val = gpio_get_value(di->gpio_en);
	return gpio_val == di->gpio_en_valid_val;
}

static void cps4067_irq_work(struct work_struct *work)
{
	int ret;
	int gpio_val;
	u8 mode = 0;
	struct cps4067_dev_info *di = container_of(work, struct cps4067_dev_info, irq_work);

	if (!di)
		goto exit;

	gpio_val = gpio_get_value(di->gpio_en);
	if (gpio_val != di->gpio_en_valid_val) {
		hwlog_err("[irq_work] gpio %s\n", gpio_val ? "high" : "low");
		goto exit;
	}
	if (di->need_ignore_irq) {
		hwlog_info("[irq_work] ignore irq\n");
		goto exit;
	}
	ret = cps4067_get_mode(di, &mode);
	if (!ret)
		hwlog_info("[irq_work] mode=0x%x\n", mode);
	else
		cps4067_rx_abnormal_irq_handler(di);

	if ((mode == CPS4067_SYS_MODE_TX) || (mode == CPS4067_SYS_MODE_BP))
		cps4067_tx_mode_irq_handler(di);
	else if (mode == CPS4067_SYS_MODE_RX)
		cps4067_rx_mode_irq_handler(di);
exit:
	if (di && !di->g_val.irq_abnormal)
		cps4067_enable_irq(di);

	power_wakeup_unlock(di->wakelock, false);
}

static irqreturn_t cps4067_interrupt(int irq, void *_di)
{
	struct cps4067_dev_info *di = _di;

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

static void cps4067_fw_mtp_check(struct cps4067_dev_info *di)
{
	u32 mtp_check_delay;

	if (power_cmdline_is_powerdown_charging_mode() ||
		(!power_cmdline_is_factory_mode() && cps4067_rx_is_tx_exist(di))) {
		di->g_val.mtp_chk_complete = true;
		return;
	}

	if (!power_cmdline_is_factory_mode())
		mtp_check_delay = di->mtp_check_delay.user;
	else
		mtp_check_delay = di->mtp_check_delay.fac;

	INIT_DELAYED_WORK(&di->mtp_check_work, cps4067_fw_mtp_check_work);
	schedule_delayed_work(&di->mtp_check_work, msecs_to_jiffies(mtp_check_delay));
}

static void cps4067_register_pwr_dev_info(struct cps4067_dev_info *di)
{
	struct power_devices_info_data *pwr_dev_info = NULL;

	pwr_dev_info = power_devices_info_register();
	if (pwr_dev_info) {
		pwr_dev_info->dev_name = di->dev->driver->name;
		pwr_dev_info->dev_id = di->chip_id;
		pwr_dev_info->ver_id = 0;
	}
}

static void cps4067_ops_unregister(struct wltrx_ic_ops *ops)
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

static int cps4067_ops_register(struct wltrx_ic_ops *ops, struct cps4067_dev_info *di)
{
	int ret;

	ret = cps4067_fw_ops_register(ops, di);
	if (ret) {
		hwlog_err("ops_register: register fw_ops failed\n");
		return ret;
	}

	ret = cps4067_rx_ops_register(ops, di);
	if (ret) {
		hwlog_err("ops_register: register rx_ops failed\n");
		return ret;
	}

	ret = cps4067_tx_ops_register(ops, di);
	if (ret) {
		hwlog_err("ops_register: register tx_ops failed\n");
		return ret;
	}

	ret = cps4067_qi_ops_register(ops, di);
	if (ret) {
		hwlog_err("ops_register: register qi_ops failed\n");
		return ret;
	}
	di->g_val.qi_hdl = hwqi_get_handle();

	ret = cps4067_hw_test_ops_register(ops, di);
	if (ret) {
		hwlog_err("ops_register: register hw_test_ops failed\n");
		return ret;
	}

	return 0;
}

struct device_node *cps4067_dts_dev_node(void *dev_data)
{
	struct cps4067_dev_info *di = dev_data;

	if (!di || !di->dev)
		return NULL;

	return di->dev->of_node;
}

static int cps4067_gpio_init(struct cps4067_dev_info *di, struct device_node *np)
{
	if (power_gpio_config_output(np, "gpio_en", "cps4067_en",
		&di->gpio_en, di->gpio_en_valid_val))
		goto gpio_en_fail;

	if (power_gpio_config_output(np, "gpio_sleep_en", "cps4067_sleep_en",
		&di->gpio_sleep_en, WLTRX_IC_SLEEP_DISABLE))
		goto gpio_sleep_en_fail;

	if (power_gpio_config_input(np, "gpio_pwr_good", "cps4067_pwr_good",
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

static int cps4067_irq_init(struct cps4067_dev_info *di,
	struct device_node *np)
{
	if (power_gpio_config_interrupt(np, "gpio_int", "cps4067_int",
		&di->gpio_int, &di->irq_int))
		return -EINVAL;

	if (request_irq(di->irq_int, cps4067_interrupt,
		IRQF_TRIGGER_FALLING, "cps4067_irq", di)) {
		hwlog_err("irq_init: request irq failed\n");
		gpio_free(di->gpio_int);
		return -EINVAL;
	}

	enable_irq_wake(di->irq_int);
	di->irq_active = true;
	INIT_WORK(&di->irq_work, cps4067_irq_work);

	return 0;
}

static void cps4067_free_dev_resources(struct cps4067_dev_info *di)
{
	power_wakeup_source_unregister(di->wakelock);
	mutex_destroy(&di->mutex_irq);
	gpio_free(di->gpio_int);
	free_irq(di->irq_int, di);
	gpio_free(di->gpio_en);
	gpio_free(di->gpio_sleep_en);
	gpio_free(di->gpio_pwr_good);
}

static int cps4067_request_dev_resources(struct cps4067_dev_info *di, struct device_node *np)
{
	int ret;

	ret = cps4067_parse_dts(np, di);
	if (ret)
		goto parse_dts_fail;
	ret = power_pinctrl_config(di->dev, "pinctrl-names", CPS4067_PINCTRL_LEN);
	if (ret)
		return ret;
	ret = cps4067_gpio_init(di, np);
	if (ret)
		goto gpio_init_fail;
	ret = cps4067_irq_init(di, np);
	if (ret)
		goto irq_init_fail;
	di->wakelock = power_wakeup_source_register(di->dev, "cps4067_wakelock");
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

static int cps4067_dev_check(struct cps4067_dev_info *di)
{
	int ret;
	u16 chip_id = 0;

	wlps_control(di->ic_type, WLPS_RX_EXT_PWR, true);
	power_usleep(DT_USLEEP_10MS);
	ret = cps4067_get_chip_id(di, &chip_id);
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

static int cps4067_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret;
	struct cps4067_dev_info *di = NULL;
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

	ret = cps4067_dev_check(di);
	if (ret) {
		ret = -EPROBE_DEFER;
		goto dev_ck_fail;
	}
	ret = cps4067_request_dev_resources(di, np);
	if (ret)
		goto req_dev_res_fail;
	ret = cps4067_ops_register(ops, di);
	if (ret)
		goto ops_regist_fail;

	wlic_iout_init(di->ic_type, np, NULL);
	cps4067_fw_mtp_check(di);
	cps4067_rx_probe_check_tx_exist(di);
	cps4067_register_pwr_dev_info(di);

	hwlog_info("wireless_chip probe ok\n");
	return 0;

ops_regist_fail:
	cps4067_free_dev_resources(di);
req_dev_res_fail:
dev_ck_fail:
	cps4067_ops_unregister(ops);
	devm_kfree(&client->dev, di);
	return ret;
}

static void cps4067_shutdown(struct i2c_client *client)
{
	struct cps4067_dev_info *di = i2c_get_clientdata(client);

	if (!di)
		return;

	cps4067_rx_shutdown_handler(di);
}

static const struct of_device_id cps4067_of_match[] = {
	{
		.compatible = "cps,cps4067",
	},
	{},
};
MODULE_DEVICE_TABLE(of, cps4067_of_match);

static const struct i2c_device_id cps4067_i2c_id[] = {
	{ "cps4067", WLTRX_IC_MAIN }, {}
};
MODULE_DEVICE_TABLE(i2c, cps4067_i2c_id);

static struct i2c_driver cps4067_driver = {
	.probe = cps4067_probe,
	.shutdown = cps4067_shutdown,
	.id_table = cps4067_i2c_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "wireless_cps4067",
		.of_match_table = of_match_ptr(cps4067_of_match),
	},
};

static int __init cps4067_init(void)
{
	return i2c_add_driver(&cps4067_driver);
}

static void __exit cps4067_exit(void)
{
	i2c_del_driver(&cps4067_driver);
}

device_initcall(cps4067_init);
module_exit(cps4067_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("cps4067 module driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
