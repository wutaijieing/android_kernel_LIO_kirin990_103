// SPDX-License-Identifier: GPL-2.0
/*
 * hc32l110_fw.c
 *
 * hc32l110 firmware driver
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
#include "hc32l110_fw.h"
#include "hc32l110_i2c.h"
#include "hc32l110_mtp.h"
#include <chipset_common/hwpower/common_module/power_delay.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG hc32l110_fw
HWLOG_REGIST();

static int hc32l110_fw_write_cmd(struct hc32l110_device_info *di, u16 cmd)
{
	int i;
	u8 ack;
	u8 buf[HC32L110_FW_CMD_SIZE] = { 0 };

	/* fill cmd */
	buf[0] = cmd >> 8;
	buf[1] = cmd & 0xFF;

	(void)hc32l110_write_word_bootloader(di, buf, HC32L110_FW_CMD_SIZE);
	for (i = 0; i < HC32L110_FW_ACK_COUNT; i++) {
		power_usleep(DT_USLEEP_1MS);
		ack = 0;
		(void)hc32l110_read_word_bootloader(di, &ack, 1);
		hwlog_info("write_cmd:ack=%x\n", ack);
		if (ack == HC32L110_FW_ACK_VAL) {
			hwlog_info("write_cmd succ: i=%d cmd=%x ack=%x\n", i, cmd, ack);
			return 0;
		}
	}

	hwlog_err("write_cmd fail: i=%d cmd=%x\n", i, cmd);
	return -EIO;
}

static int hc32l110_fw_write_addr(struct hc32l110_device_info *di, u32 addr)
{
	int i;
	u8 ack;
	u8 buf[HC32L110_FW_ADDR_SIZE] = { 0 };

	/* fill address */
	buf[0] = addr >> 24;
	buf[1] = (addr >> 16) & 0xFF;
	buf[2] = (addr >> 8) & 0xFF;
	buf[3] = (addr >> 0) & 0xFF;
	buf[4] = buf[0];
	for (i = 1; i < HC32L110_FW_ADDR_SIZE - 1; i++)
		buf[4] ^= buf[i];

	(void)hc32l110_write_word_bootloader(di, buf, HC32L110_FW_ADDR_SIZE);
	for (i = 0; i < HC32L110_FW_ACK_COUNT; i++) {
		power_usleep(DT_USLEEP_1MS);
		ack = 0;
		(void)hc32l110_read_word_bootloader(di, &ack, 1);
		hwlog_info("write_addr:ack=%x\n", ack);
		if (ack == HC32L110_FW_ACK_VAL) {
			hwlog_info("write_addr succ: i=%d addr=%x ack=%x\n", i, addr, ack);
			return 0;
		}
	}

	hwlog_err("write_addr fail: i=%d addr=%x\n", i, addr);
	return -EIO;
}

static int hc32l110_fw_erase(struct hc32l110_device_info *di)
{
	int ret;
	int i;
	u8 ack;
	u8 buf[HC32L110_FW_ERASE_SIZE] = { 0 };

	/* special erase */
	buf[0] = 0xFF;
	buf[1] = 0xFF;
	buf[2] = buf[0] ^ buf[1];

	ret = hc32l110_fw_write_cmd(di, HC32L110_FW_ERASE_CMD);
	if (ret)
		return ret;

	(void)hc32l110_write_word_bootloader(di, buf, HC32L110_FW_ERASE_SIZE);
	for (i = 0; i < HC32L110_FW_ERASE_ACK_COUNT; i++) {
		power_msleep(DT_MSLEEP_500MS, 0, NULL);
		ack = 0;
		(void)hc32l110_read_word_bootloader(di, &ack, 1);
		hwlog_info("erase:ack=%x\n", ack);
		if (ack == HC32L110_FW_ACK_VAL) {
			hwlog_info("erase succ: i=%d ack=%x\n", i, ack);
			return 0;
		}
	}

	hwlog_err("erase fail\n");
	return -EIO;
}

static int hc32l110_fw_write_data(struct hc32l110_device_info *di,
	const u8 *data, int len)
{
	int i;
	u8 ack;
	u8 checksum = len - 1;
	u8 buf[HC32L110_FW_PAGE_SIZE + 2] = {0};

	if ((len > HC32L110_FW_PAGE_SIZE) || (len <= 0)) {
		hwlog_err("data len illegal, len=%d\n", len);
		return -EINVAL;
	}

	/* buf content: (len of data - 1) + data + checksum */
	buf[0] = len - 1;
	for (i = 1; i <= len; i++) {
		buf[i] = data[i - 1];
		checksum ^= buf[i];
	}
	buf[len + 1] = checksum;

	(void)hc32l110_write_word_bootloader(di, buf, len + 2);
	for (i = 0; i < HC32L110_FW_ACK_COUNT; i++) {
		power_usleep(DT_USLEEP_20MS);
		ack = 0;
		(void)hc32l110_read_word_bootloader(di, &ack, 1);
		hwlog_info("write_data:ack=%x\n", ack);
		if (ack == HC32L110_FW_ACK_VAL) {
			hwlog_info("write_data succ: i=%d len=%d ack=%x\n", i, len, ack);
			return 0;
		}
	}

	hwlog_err("write_data fail\n");
	return -EIO;
}

static int hc32l110_fw_write_mtp_data(struct hc32l110_device_info *di,
	const u8 *mtp_data, int mtp_size)
{
	int ret;
	int size;
	int offset = 0;
	int remaining = mtp_size;
	u32 addr = HC32L110_FW_MTP_ADDR;

	while (remaining > 0) {
		ret = hc32l110_fw_write_cmd(di, HC32L110_FW_WRITE_CMD);
		ret += hc32l110_fw_write_addr(di, addr);
		size = (remaining > HC32L110_FW_PAGE_SIZE) ? HC32L110_FW_PAGE_SIZE : remaining;
		ret += hc32l110_fw_write_data(di, mtp_data + offset, size);
		if (ret) {
			hwlog_err("write mtp data fail\n");
			return ret;
		}

		offset += size;
		remaining -= size;
		addr += size;
	}

	return 0;
}

static void hc32l110_fw_program_begin(struct hc32l110_device_info *di)
{
	/* enable pin pull high */
	(void)gpio_direction_output(di->gpio_enable, 1);
	power_usleep(DT_USLEEP_5MS);

	/* reset pin pull low */
	(void)gpio_direction_output(di->gpio_reset, 0);
	power_usleep(DT_USLEEP_100US);

	/* reset pin pull high */
	(void)gpio_direction_output(di->gpio_reset, 1);
	power_msleep(DT_MSLEEP_50MS, 0, NULL);
}

static int hc32l110_fw_program_end(struct hc32l110_device_info *di)
{
	int ret;

	/* enable pin pull low */
	(void)gpio_direction_output(di->gpio_enable, 0);

	/* write go cmd */
	ret = hc32l110_fw_write_cmd(di, HC32L110_FW_GO_CMD);
	ret += hc32l110_fw_write_addr(di, HC32L110_FW_MTP_ADDR);

	/* wait for program jump */
	power_msleep(DT_MSLEEP_50MS, 0, NULL);

	return ret;
}

static int hc32l110_fw_update_mtp(struct hc32l110_device_info *di,
	const u8 *mtp_data, int mtp_size)
{
	int i;

	for (i = 0; i < HC32L110_FW_RETRY_COUNT; i++) {
		hwlog_info("mtp update begin\n");
		hc32l110_fw_program_begin(di);
		if (hc32l110_fw_erase(di))
			continue;
		if (hc32l110_fw_write_mtp_data(di, mtp_data, mtp_size))
			continue;
		if (hc32l110_fw_program_end(di))
			continue;
		hwlog_info("mtp update end\n");
		return 0;
	}

	return -EINVAL;
}

static int hc32l110_fw_check_bootloader_mode(struct hc32l110_device_info *di)
{
	int ret;
	u8 ack = 0;
	u8 data = 0;

	/* write ver cmd and wait ack */
	ret = hc32l110_fw_write_cmd(di, HC32L110_FW_GET_VER_CMD);
	if (ret) {
		hwlog_err("not work bootloader mode\n");
		return -EIO;
	}

	/* get data and wait ack */
	ret = hc32l110_read_word_bootloader(di, &data, 1);
	ret += hc32l110_read_word_bootloader(di, &ack, 1);
	hwlog_info("get_data=%x ack=0x%x\n", data, ack);
	if (ret) {
		hwlog_err("not work bootloader mode\n");
		return -EIO;
	}

	hwlog_info("work bootloader mode\n");
	return 0;
}

int hc32l110_fw_get_dev_id(struct hc32l110_device_info *di)
{
	u8 id = 0;
	int ret;

	ret = hc32l110_read_byte(di, HC32L110_FW_DEV_ID_REG, &id);
	if (ret)
		return -EINVAL;

	di->fw_dev_id = id;
	hwlog_info("fw_dev_id: [%x]=0x%x\n", HC32L110_FW_DEV_ID_REG, id);
	return 0;
}

int hc32l110_fw_get_ver_id(struct hc32l110_device_info *di)
{
	u8 id = 0;
	int ret;

	ret = hc32l110_read_byte(di, HC32L110_FW_VER_ID_REG, &id);
	if (ret)
		return -EINVAL;

	di->fw_ver_id = id;
	hwlog_info("fw_ver_id: [%x]=0x%x\n", HC32L110_FW_VER_ID_REG, id);
	return 0;
}

int hc32l110_fw_update_empty_mtp(struct hc32l110_device_info *di)
{
	if (hc32l110_fw_check_bootloader_mode(di))
		return 0;

	return hc32l110_fw_update_mtp(di, g_hc32l110_mtp_data, HC32L110_MTP_SIZE);
}

int hc32l110_fw_update_latest_mtp(struct hc32l110_device_info *di)
{
	hc32l110_fw_program_begin(di);

	if (hc32l110_fw_check_bootloader_mode(di)) {
		(void)gpio_direction_output(di->gpio_enable, 0);
		return -EINVAL;
	}

	hc32l110_fw_program_end(di);

	if (!hc32l110_fw_get_ver_id(di) && di->fw_ver_id >= HC32L110_MTP_VER)
		return 0;

	hwlog_info("need update mtp: ver_id=%x mtp_ver=%x\n", di->fw_ver_id, HC32L110_MTP_VER);
	return hc32l110_fw_update_mtp(di, g_hc32l110_mtp_data, HC32L110_MTP_SIZE);
}

int hc32l110_fw_update_online_mtp(struct hc32l110_device_info *di,
	u8 *mtp_data, int mtp_size, int ver_id)
{
	int ret;

	if (hc32l110_fw_get_ver_id(di))
		return -EINVAL;

	if (di->fw_ver_id >= ver_id)
		return 0;

	hwlog_info("need update mtp: ver_id=%x mtp_ver=%x\n", di->fw_ver_id, ver_id);
	ret = hc32l110_fw_update_mtp(di, mtp_data, mtp_size);
	ret += hc32l110_fw_get_ver_id(di);
	return ret;
}
