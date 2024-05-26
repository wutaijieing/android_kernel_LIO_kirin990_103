// SPDX-License-Identifier: GPL-2.0
/*
 * stwlc68.c
 *
 * stwlc68 driver
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

#include "stwlc68.h"

#define HWLOG_TAG wireless_stwlc68
HWLOG_REGIST();

static bool stwlc68_is_pwr_good(struct stwlc68_dev_info *di)
{
	int gpio_val;

	if (!di)
		return false;
	if ((di->gpio_pwr_good <= 0) || power_cmdline_is_factory_mode())
		return true;

	gpio_val = gpio_get_value(di->gpio_pwr_good);

	return gpio_val == STWLC68_GPIO_PWR_GOOD_VAL;
}

static bool stwlc68_is_i2c_addr_valid(struct stwlc68_dev_info *di, u16 addr)
{
	if (!di->g_val.sram_chk_complete) {
		if ((addr < STWLC68_RAM_FW_START_ADDR + STWLC68_RAM_MAX_SIZE) &&
			(addr >= STWLC68_RAM_FW_START_ADDR))
			return true;

		return false;
	}

	return true;
}

static int stwlc68_i2c_read(struct stwlc68_dev_info *di, u8 *cmd, int cmd_len,
	u8 *buf, int buf_len)
{
	int i;

	for (i = 0; i < WLTRX_IC_I2C_RETRY_CNT; i++) {
		if (!stwlc68_is_pwr_good(di))
			return -EIO;
		if (!power_i2c_read_block(di->client, cmd, cmd_len, buf, buf_len))
			return 0;
		power_usleep(DT_USLEEP_10MS);
	}

	return -EIO;
}

static int stwlc68_i2c_write(struct stwlc68_dev_info *di, u8 *buf, int buf_len)
{
	int i;

	for (i = 0; i < WLTRX_IC_I2C_RETRY_CNT; i++) {
		if (!stwlc68_is_pwr_good(di))
			return -EIO;
		if (!power_i2c_write_block(di->client, buf, buf_len))
			return 0;
		power_usleep(DT_USLEEP_10MS);
	}

	return -EIO;
}

int stwlc68_read_block(struct stwlc68_dev_info *di, u16 reg, u8 *data, u8 len)
{
	u8 cmd[STWLC68_ADDR_LEN];

	if (!di || !data) {
		hwlog_err("read_block: para null\n");
		return -EINVAL;
	}

	if (!stwlc68_is_i2c_addr_valid(di, reg))
		return -EIO;

	cmd[0] = reg >> POWER_BITS_PER_BYTE;
	cmd[1] = reg & POWER_MASK_BYTE;

	return stwlc68_i2c_read(di, cmd, STWLC68_ADDR_LEN, data, len);
}

int stwlc68_write_block(struct stwlc68_dev_info *di, u16 reg, u8 *data, u8 data_len)
{
	int ret;
	u8 *cmd = NULL;

	if (!di || !data) {
		hwlog_err("write_block: para null\n");
		return -EINVAL;
	}

	if (!stwlc68_is_i2c_addr_valid(di, reg))
		return -EIO;

	cmd = kzalloc(sizeof(u8) * (STWLC68_ADDR_LEN + data_len), GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	cmd[0] = reg >> POWER_BITS_PER_BYTE;
	cmd[1] = reg & POWER_MASK_BYTE;
	memcpy(&cmd[STWLC68_ADDR_LEN], data, data_len);

	ret = stwlc68_i2c_write(di, cmd, STWLC68_ADDR_LEN + data_len);

	kfree(cmd);
	return ret;
}

int stwlc68_4addr_read_block(struct stwlc68_dev_info *di, u32 addr, u8 *data, u8 len)
{
	u8 cmd[STWLC68_4ADDR_F_LEN];

	if (!di || !data) {
		hwlog_err("4addr_read_block: para null\n");
		return -EINVAL;
	}

	 /* bit[0]: flag 0xFA; bit[1:4]: addr */
	cmd[0] = STWLC68_4ADDR_FLAG;
	cmd[1] = (u8)((addr >> 24) & POWER_MASK_BYTE);
	cmd[2] = (u8)((addr >> 16) & POWER_MASK_BYTE);
	cmd[3] = (u8)((addr >> 8) & POWER_MASK_BYTE);
	cmd[4] = (u8)((addr >> 0) & POWER_MASK_BYTE);

	return stwlc68_i2c_read(di, cmd, STWLC68_4ADDR_F_LEN, data, len);
}

int stwlc68_4addr_write_block(struct stwlc68_dev_info *di, u32 addr, u8 *data, u8 data_len)
{
	int ret;
	u8 *cmd = NULL;

	if (!di || !data) {
		hwlog_err("4addr_write_block: para null\n");
		return -EINVAL;
	}

	cmd = kzalloc(sizeof(u8) * (STWLC68_4ADDR_F_LEN + data_len), GFP_KERNEL);
	if (!cmd)
		return -ENOMEM;

	 /* bit[0]: flag 0xFA; bit[1:4]: addr */
	cmd[0] = STWLC68_4ADDR_FLAG;
	cmd[1] = (u8)((addr >> 24) & POWER_MASK_BYTE);
	cmd[2] = (u8)((addr >> 16) & POWER_MASK_BYTE);
	cmd[3] = (u8)((addr >> 8) & POWER_MASK_BYTE);
	cmd[4] = (u8)((addr >> 0) & POWER_MASK_BYTE);
	memcpy(&cmd[STWLC68_4ADDR_F_LEN], data, data_len);

	ret = stwlc68_i2c_write(di, cmd, STWLC68_4ADDR_F_LEN + data_len);

	kfree(cmd);
	return ret;
}

int stwlc68_read_byte(struct stwlc68_dev_info *di, u16 reg, u8 *data)
{
	return stwlc68_read_block(di, reg, data, POWER_BYTE_LEN);
}

int stwlc68_write_byte(struct stwlc68_dev_info *di, u16 reg, u8 data)
{
	return stwlc68_write_block(di, reg, &data, POWER_BYTE_LEN);
}

int stwlc68_read_word(struct stwlc68_dev_info *di, u16 reg, u16 *data)
{
	int ret;
	u8 buff[POWER_WORD_LEN] = { 0 };

	if (!di || !data)
		return -EINVAL;

	ret = stwlc68_read_block(di, reg, buff, POWER_WORD_LEN);
	if (ret)
		return -EIO;

	*data = buff[0] | buff[1] << POWER_BITS_PER_BYTE;
	return 0;
}

int stwlc68_write_word(struct stwlc68_dev_info *di, u16 reg, u16 data)
{
	u8 buff[POWER_WORD_LEN];

	buff[0] = data & POWER_MASK_BYTE;
	buff[1] = data >> POWER_BITS_PER_BYTE;

	return stwlc68_write_block(di, reg, buff, POWER_WORD_LEN);
}

int stwlc68_write_word_mask(struct stwlc68_dev_info *di, u16 reg, u16 mask, u16 shift, u16 data)
{
	int ret;
	u16 val = 0;

	ret = stwlc68_read_word(di, reg, &val);
	if (ret)
		return ret;

	val &= ~mask;
	val |= ((data << shift) & mask);

	return stwlc68_write_word(di, reg, val);
}

static int stwlc68_get_chip_id(struct stwlc68_dev_info *di, u16 *chip_id)
{
	return stwlc68_read_word(di, STWLC68_CHIP_ID_ADDR, chip_id);
}

int stwlc68_get_cfg_id(struct stwlc68_dev_info *di, u16 *cfg_id)
{
	return stwlc68_read_word(di, STWLC68_CFG_ID_ADDR, cfg_id);
}

int stwlc68_get_patch_id(struct stwlc68_dev_info *di, u16 *patch_id)
{
	return stwlc68_read_word(di, STWLC68_PATCH_ID_ADDR, patch_id);
}

int stwlc68_get_cut_id(struct stwlc68_dev_info *di, u8 *cut_id)
{
	return stwlc68_4addr_read_block(di, STWLC68_CUT_ID_ADDR, cut_id, STWLC68_FW_ADDR_LEN);
}

int stwlc68_get_chip_info(struct stwlc68_dev_info *di, struct stwlc68_chip_info *info)
{
	int ret;
	u8 chip_info[STWLC68_CHIP_INFO_LEN] = { 0 };

	ret = stwlc68_read_block(di, STWLC68_CHIP_INFO_ADDR, chip_info, STWLC68_CHIP_INFO_LEN);
	if (ret)
		return ret;

	/*
	 * addr[0:1]:   chip unique id;
	 * addr[2:2]:   chip revision number;
	 * addr[3:3]:   customer id;
	 * addr[4:5]:   sram id;
	 * addr[6:7]:   svn revision number;
	 * addr[8:9]:   configuration id;
	 * addr[10:11]: pe id;
	 * addr[12:13]: patch id;
	 * 1byte = 8bit
	 */
	info->chip_id = (u16)(chip_info[0] | (chip_info[1] << 8));
	info->chip_rev = chip_info[2];
	info->cust_id = chip_info[3];
	info->sram_id = (u16)(chip_info[4] | (chip_info[5] << 8));
	info->svn_rev = (u16)(chip_info[6] | (chip_info[7] << 8));
	info->cfg_id = (u16)(chip_info[8] | (chip_info[9] << 8));
	info->pe_id = (u16)(chip_info[10] | (chip_info[11] << 8));
	info->patch_id = (u16)(chip_info[12] | (chip_info[13] << 8));

	return stwlc68_get_cut_id(di, &info->cut_id);
}

int stwlc68_get_chip_info_str(char *info_str, int len, void *dev_data)
{
	int ret;
	struct stwlc68_chip_info chip_info = { 0 };

	if (!info_str || (len < WLTRX_IC_CHIP_INFO_LEN))
		return -EINVAL;

	ret = stwlc68_get_chip_info(dev_data, &chip_info);
	if (ret)
		return -EIO;

	return snprintf(info_str, WLTRX_IC_CHIP_INFO_LEN,
		"chip_id:%d cfg_id:0x%x patch_id:0x%x cut_id:%d sram_id:0x%x", chip_info.chip_id,
		chip_info.cfg_id, chip_info.patch_id, chip_info.cut_id, chip_info.sram_id);
}

int stwlc68_get_chip_fw_version(u8 *data, int len, void *dev_data)
{
	struct stwlc68_chip_info chip_info;

	/* fw version length must be 4 */
	if (!data || (len != 4)) {
		hwlog_err("get_fw_version: para err");
		return -EINVAL;
	}

	if (stwlc68_get_chip_info(dev_data, &chip_info)) {
		hwlog_err("get_chip_fw_version: get chip info fail\n");
		return -EIO;
	}

	/* byte[0:1]=patch_id, byte[2:3]=sram_id */
	data[0] = (u8)((chip_info.patch_id >> 0) & POWER_MASK_BYTE);
	data[1] = (u8)((chip_info.patch_id >> POWER_BITS_PER_BYTE) & POWER_MASK_BYTE);
	data[2] = (u8)((chip_info.sram_id >> 0) & POWER_MASK_BYTE);
	data[3] = (u8)((chip_info.sram_id >> POWER_BITS_PER_BYTE) & POWER_MASK_BYTE);
	return 0;
}

int stwlc68_get_mode(struct stwlc68_dev_info *di, u8 *mode)
{
	int ret;

	if (!mode)
		return -EINVAL;

	ret = stwlc68_read_byte(di, STWLC68_OP_MODE_ADDR, mode);
	if (ret) {
		hwlog_err("get_mode: fail\n");
		return -EIO;
	}

	return 0;
}

void stwlc68_enable_irq(struct stwlc68_dev_info *di)
{
	mutex_lock(&di->mutex_irq);
	if (!di->irq_active) {
		hwlog_info("[enable_irq] ++\n");
		enable_irq(di->irq_int);
		di->irq_active = 1;
	}
	hwlog_info("[enable_irq] --\n");
	mutex_unlock(&di->mutex_irq);
}

void stwlc68_disable_irq_nosync(struct stwlc68_dev_info *di)
{
	mutex_lock(&di->mutex_irq);
	if (di->irq_active) {
		hwlog_info("[disable_irq_nosync] ++\n");
		disable_irq_nosync(di->irq_int);
		di->irq_active = 0;
	}
	hwlog_info("[disable_irq_nosync] --\n");
	mutex_unlock(&di->mutex_irq);
}

void stwlc68_chip_enable(bool enable, void *dev_data)
{
	int gpio_val;
	struct stwlc68_dev_info *di = dev_data;

	if (!di)
		return;

	gpio_set_value(di->gpio_en, enable ? di->gpio_en_valid_val : !di->gpio_en_valid_val);
	gpio_val = gpio_get_value(di->gpio_en);
	hwlog_info("[chip_enable] gpio is %s now\n", gpio_val ? "high" : "low");
}

bool stwlc68_is_chip_enable(void *dev_data)
{
	int gpio_val;
	struct stwlc68_dev_info *di = dev_data;

	if (!di)
		return false;

	gpio_val = gpio_get_value(di->gpio_en);
	return gpio_val == di->gpio_en_valid_val;
}

void stwlc68_chip_reset(void *dev_data)
{
	int ret;
	u8 data = STWLC68_CHIP_RESET;
	struct stwlc68_dev_info *di = dev_data;

	ret = stwlc68_4addr_write_block(di, STWLC68_CHIP_RESET_ADDR, &data, STWLC68_FW_ADDR_LEN);
	if (ret) {
		hwlog_err("chip_reset: fail\n");
		return;
	}

	hwlog_info("[chip_reset] succ\n");
}

static void stwlc68_irq_work(struct work_struct *work)
{
	int ret;
	int gpio_val;
	u8 mode = 0;
	struct stwlc68_dev_info *di = container_of(work, struct stwlc68_dev_info, irq_work);

	if (!di)
		goto exit;

	gpio_val = gpio_get_value(di->gpio_en);
	if (gpio_val != di->gpio_en_valid_val) {
		hwlog_err("[irq_work] gpio is %s now\n", gpio_val ? "high" : "low");
		goto exit;
	}
	gpio_val = gpio_get_value(di->gpio_int);
	if (gpio_val != STWLC68_IRQ_INT_VALID_VAL) {
		hwlog_err("[irq_work] gpio_int is %s now\n", gpio_val ? "high" : "low");
		goto exit;
	}
	/* get System Operating Mode */
	ret = stwlc68_get_mode(di, &mode);
	if (!ret)
		hwlog_info("[irq_work] mode = 0x%x\n", mode);
	else
		stwlc68_rx_handle_abnormal_irq(di);

	/* handler interrupt */
	if ((mode == STWLC68_TX_MODE) || (mode == STWLC68_STANDALONE_MODE))
		stwlc68_tx_mode_irq_handler(di);
	else if (mode == STWLC68_RX_MODE)
		stwlc68_rx_mode_irq_handler(di);

exit:
	if (!di->g_val.irq_abnormal_flag && di)
		stwlc68_enable_irq(di);

	power_wakeup_unlock(di->wakelock, false);
}

static irqreturn_t stwlc68_interrupt(int irq, void *_di)
{
	struct stwlc68_dev_info *di = _di;

	if (!di) {
		hwlog_err("interrupt: di null\n");
		return IRQ_HANDLED;
	}

	power_wakeup_lock(di->wakelock, false);
	hwlog_info("[interrupt] ++\n");
	if (di->irq_active) {
		disable_irq_nosync(di->irq_int);
		di->irq_active = 0;
		schedule_work(&di->irq_work);
	} else {
		hwlog_info("[interrupt] irq is not enable,do nothing\n");
		power_wakeup_unlock(di->wakelock, false);
	}
	hwlog_info("[interrupt] --\n");

	return IRQ_HANDLED;
}

static int stwlc68_dev_check(struct stwlc68_dev_info *di)
{
	int ret;
	u16 chip_id = 0;

	wlps_control(di->ic_type, WLPS_RX_EXT_PWR, true);
	power_usleep(DT_USLEEP_10MS);
	di->g_val.sram_chk_complete = true;
	ret = stwlc68_get_chip_id(di, &chip_id);
	if (ret) {
		hwlog_err("dev_check: fail\n");
		wlps_control(di->ic_type, WLPS_RX_EXT_PWR, false);
		di->g_val.sram_chk_complete = false;
		return ret;
	}
	di->g_val.sram_chk_complete = false;
	wlps_control(di->ic_type, WLPS_RX_EXT_PWR, false);

	hwlog_info("[dev_check] chip_id = %d\n", chip_id);

	di->chip_id = chip_id;
	if (chip_id != STWLC68_CHIP_ID)
		hwlog_err("dev_check: wlc_chip not match\n");

	return 0;
}

struct device_node *stwlc68_dts_dev_node(void *dev_data)
{
	struct stwlc68_dev_info *di = dev_data;

	if (!di || !di->dev)
		return NULL;

	return di->dev->of_node;
}

static int stwlc68_gpio_init(struct stwlc68_dev_info *di, struct device_node *np)
{
	/* gpio_en */
	if (power_gpio_config_output(np, "gpio_en", "stwlc68_en",
		&di->gpio_en, di->gpio_en_valid_val))
		goto gpio_en_fail;

	/* gpio_sleep_en */
	if (!di->gpio_sleep_en_pending && power_gpio_config_output(np, "gpio_sleep_en",
		"stwlc68_sleep_en", &di->gpio_sleep_en,	RX_SLEEP_EN_DISABLE))
		goto gpio_sleep_en_fail;

	/* gpio_pwr_good */
	if (!di->gpio_pwr_good_pending && power_gpio_config_input(np, "gpio_pwr_good",
		"stwlc68_pwr_good", &di->gpio_pwr_good))
		goto gpio_pwr_good_fail;

	return 0;

gpio_pwr_good_fail:
	gpio_free(di->gpio_sleep_en);
gpio_sleep_en_fail:
	gpio_free(di->gpio_en);
gpio_en_fail:
	return -EINVAL;
}

static int stwlc68_irq_init(struct stwlc68_dev_info *di, struct device_node *np)
{
	int ret;

	if (power_gpio_config_interrupt(np,
		"gpio_int", "stwlc68_int", &di->gpio_int, &di->irq_int)) {
		ret = -EINVAL;
		goto irq_init_fail_0;
	}

	ret = request_irq(di->irq_int, stwlc68_interrupt, IRQF_TRIGGER_FALLING, "stwlc68_irq", di);
	if (ret) {
		hwlog_err("irq_init: request stwlc68_irq failed\n");
		di->irq_int = -EINVAL;
		goto irq_init_fail_1;
	}
	enable_irq_wake(di->irq_int);
	di->irq_active = 1;
	INIT_WORK(&di->irq_work, stwlc68_irq_work);

	return 0;

irq_init_fail_1:
	gpio_free(di->gpio_int);
irq_init_fail_0:
	return ret;
}

static int stwlc68_request_dev_resources(struct stwlc68_dev_info *di, struct device_node *np)
{
	int ret;

	ret = stwlc68_parse_dts(np, di);
	if (ret)
		goto parse_dts_fail;
	ret = stwlc68_gpio_init(di, np);
	if (ret)
		goto gpio_init_fail;
	ret = stwlc68_irq_init(di, np);
	if (ret)
		goto irq_init_fail;
	di->wakelock = power_wakeup_source_register(di->dev, "stwlc68_wakelock");
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

static void stwlc68_free_dev_resources(struct stwlc68_dev_info *di)
{
	power_wakeup_source_unregister(di->wakelock);
	mutex_destroy(&di->mutex_irq);
	gpio_free(di->gpio_int);
	free_irq(di->irq_int, di);
	gpio_free(di->gpio_en);
	gpio_free(di->gpio_sleep_en);
	gpio_free(di->gpio_pwr_good);
}

static void stwlc68_register_pwr_dev_info(struct stwlc68_dev_info *di)
{
	struct power_devices_info_data *pwr_dev_info = NULL;

	pwr_dev_info = power_devices_info_register();
	if (pwr_dev_info) {
		pwr_dev_info->dev_name = di->dev->driver->name;
		pwr_dev_info->dev_id = di->chip_id;
		pwr_dev_info->ver_id = 0;
	}
}

static void stwlc68_ops_unregister(struct wltrx_ic_ops *ops)
{
	if (!ops)
		return;

	kfree(ops->rx_ops);
	kfree(ops->tx_ops);
	kfree(ops->fw_ops);
	kfree(ops->qi_ops);
	kfree(ops);
}

static int stwlc68_ops_register(struct wltrx_ic_ops *ops, struct stwlc68_dev_info *di)
{
	int ret;

	ret = stwlc68_fw_ops_register(ops, di);
	if (ret) {
		hwlog_err("ops_register: register fw_ops failed\n");
		goto register_fail;
	}

	ret = stwlc68_rx_ops_register(ops, di);
	if (ret) {
		hwlog_err("ops_register: register rx_ic_ops failed\n");
		goto register_fail;
	}

	ret = stwlc68_tx_ops_register(ops, di);
	if (ret) {
		hwlog_err("ops_register: register tx_ic_ops failed\n");
		goto register_fail;
	}

	ret = stwlc68_qi_ops_register(ops, di);
	if (ret) {
		hwlog_err("ops_register: register qi_ops failed\n");
		goto register_fail;
	}
	di->g_val.qi_hdl = hwqi_get_handle();

	return 0;

register_fail:
	stwlc68_ops_unregister(ops);
	return ret;
}

static void stwlc68_fw_sram_check(struct stwlc68_dev_info *di)
{
	if (power_cmdline_is_powerdown_charging_mode()) {
		di->g_val.sram_chk_complete = true;
		return;
	}

	INIT_DELAYED_WORK(&di->sram_scan_work, stwlc68_fw_sram_scan_work);
	schedule_delayed_work(&di->sram_scan_work,
		msecs_to_jiffies(1500)); /* delay for wireless probe */
}

static int stwlc68_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	struct stwlc68_dev_info *di = NULL;
	struct device_node *np = NULL;
	struct wltrx_ic_ops *ops = NULL;

	if (!client || !id || !client->dev.of_node)
		return -ENODEV;

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

	ret = stwlc68_dev_check(di);
	if (ret)
		goto dev_ck_fail;

	ret = stwlc68_request_dev_resources(di, np);
	if (ret)
		goto req_dev_res_fail;

	ret = stwlc68_ops_register(ops, di);
	if (ret)
		goto ops_regist_fail;

	wlic_iout_init(di->ic_type, np, NULL);
	stwlc68_rx_probe_check_tx_exist(di);
	stwlc68_register_pwr_dev_info(di);
	stwlc68_fw_sram_check(di);

	hwlog_info("wireless_stwlc68 probe ok\n");
	return 0;

ops_regist_fail:
	stwlc68_free_dev_resources(di);
req_dev_res_fail:
dev_ck_fail:
	stwlc68_ops_unregister(ops);
	devm_kfree(&client->dev, di);
	return ret;
}

static void stwlc68_shutdown(struct i2c_client *client)
{
	struct stwlc68_dev_info *di = i2c_get_clientdata(client);

	if (!di)
		return;

	hwlog_info("[shutdown] ++\n");
	if (wlrx_get_wireless_channel_state() == WIRELESS_CHANNEL_ON)
		stwlc68_rx_shutdown_handler(di);
	hwlog_info("[shutdown] --\n");
}

MODULE_DEVICE_TABLE(i2c, wireless_stwlc68);
static const struct of_device_id stwlc68_of_match[] = {
	{
		.compatible = "st,stwlc68",
		.data = NULL,
	},
	{
		.compatible = "st,stwlc68_aux",
		.data = NULL,
	},
	{},
};

static const struct i2c_device_id stwlc68_i2c_id[] = {
	{ "stwlc68", WLTRX_IC_MAIN },
	{ "stwlc68_aux", WLTRX_IC_AUX },
	{}
};

static struct i2c_driver stwlc68_driver = {
	.probe = stwlc68_probe,
	.shutdown = stwlc68_shutdown,
	.id_table = stwlc68_i2c_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "wireless_stwlc68",
		.of_match_table = of_match_ptr(stwlc68_of_match),
	},
};

static int __init stwlc68_init(void)
{
	return i2c_add_driver(&stwlc68_driver);
}

static void __exit stwlc68_exit(void)
{
	i2c_del_driver(&stwlc68_driver);
}

device_initcall(stwlc68_init);
module_exit(stwlc68_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("stwlc68 module driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
