// SPDX-License-Identifier: GPL-2.0
/*
 * mt5785_fw.c
 *
 * mt5785 mtp, sram driver
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
#include "mt5785_fw.h"

#define HWLOG_TAG wireless_mt5785_fw
HWLOG_REGIST();

#define MT5785_MTP_CRC_CHECK_TIMES     5
#define MT5785_MTP_PGM_CHECK_TIMES     50
#define MT5785_LOAD_BL_NORMAL          0
#define MT5785_LOAD_BL_CALI            1

int mt5785_fw_sram_update(void *dev_data)
{
	int ret;
	struct mt5785_chip_info info = { 0 };
	struct mt5785_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	ret = mt5785_get_chip_info(di, &info);
	if (!ret)
		hwlog_info("[chip_info] chip_id=0x%x, mtp_version=0x%x\n",
			info.chip_id, info.mtp_ver);

	return 0;
}

int mt5785_fw_get_mtp_ver(struct mt5785_dev_info *di, u16 *mtp_ver)
{
	return mt5785_read_word(di, MT5785_MTP_VER_ADDR, mtp_ver);
}

static void mt5785_fw_ps_control(unsigned int ic_type, int scene, bool flag)
{
	static int ref_cnt;

	hwlog_info("[ps_control] ref_cnt=%d, flag=%d\n", ref_cnt, flag);
	if (flag)
		++ref_cnt;
	else if (--ref_cnt > 0)
		return;

	wlps_control(ic_type, scene, flag);
}

static int mt5785_fw_sram_write_data(struct mt5785_dev_info *di, u16 addr, u8 *data, u32 len)
{
	int ret;
	u32 size_to_write = 0;
	u32 remaining = len;
	u32 sram_addr = addr;
	u32 wr_already = 0;
	u8 wr_buff[MT5785_FW_PAGE_SIZE] = { 0 };

	while (remaining > 0) {
		if (remaining > MT5785_FW_PAGE_SIZE)
			size_to_write = MT5785_FW_PAGE_SIZE;
		else
			size_to_write = remaining;

		memcpy(wr_buff, data + wr_already, size_to_write);
		ret = mt5785_write_block(di, sram_addr, wr_buff, size_to_write);
		if (ret)
			return ret;

		sram_addr += size_to_write;
		wr_already += size_to_write;
		remaining -= size_to_write;
	}
	return 0;
}

static int mt5785_fw_check_mtp_version(struct mt5785_dev_info *di)
{
	int ret;
	u16 mtp_ver = 0;

	ret = mt5785_fw_get_mtp_ver(di, &mtp_ver);
	if (ret)
		return ret;

	hwlog_info("[check_mtp_version] mtp_ver=0x%04x\n", mtp_ver);
	if (mtp_ver != di->fw_mtp.mtp_ver)
		return -ENXIO;

	return 0;
}

static int mt5785_fw_check_mtp_crc(struct mt5785_dev_info *di)
{
	u32 cmd_status = 0;
	u16 crc_value = 0;
	int i;
	int ret;

	ret = mt5785_write_word(di, MT5785_MTP_CRC_ADDR, di->fw_mtp.mtp_crc);
	ret += mt5785_write_word(di, MT5785_MTP_TOTAL_LENGTH_ADDR, di->fw_mtp.mtp_len);
	ret += mt5785_write_dword_mask(di, MT5785_RX_CMD_ADDR,
		MT5785_RX_CMD_CRC_CHECK_MASK, MT5785_RX_CMD_CRC_CHECK_SHIFT, MT5785_RX_CMD_VAL);
	for (i = 0; i < MT5785_MTP_CRC_CHECK_TIMES; i++) {
		power_usleep(DT_USLEEP_10MS);
		ret += mt5785_read_dword(di, MT5785_RX_CMD_ADDR, &cmd_status);
		if (ret) {
			hwlog_err("check_mtp_crc: read failed\n");
			return ret;
		}

		if ((cmd_status & MT5785_RX_CMD_CRC_CHECK_MASK) == 0) {
			ret = mt5785_read_word(di, MT5785_RX_SYS_MODE_ADDR, &crc_value);
			if (ret || (crc_value & MT5785_RX_SYS_MODE_CRC_ERROR)) {
				hwlog_err("check_mtp_crc: failed\n");
				return -EIO;
			} else if (crc_value & MT5785_RX_SYS_MODE_CRC_OK) {
				hwlog_info("[check_mtp_crc:] success\n");
				return 0;
			} else {
				return -EIO;
			}
		}
	}

	hwlog_err("check_mtp_crc: timeout\n");
	return -EIO;
}

static int mt5785_fw_disable_watchdog(struct mt5785_dev_info *di)
{
	int ret;

	ret = mt5785_write_byte(di, 0x5808, 0x95); /* disable watchdog */
	ret += mt5785_write_byte(di, 0x5800, 0x01); /* clear watchdog flag */
	return ret;
}

static int mt5785_fw_load_bootloader_enable_map(struct mt5785_dev_info *di, int type)
{
	int ret;

	ret = mt5785_write_byte(di, 0x5244, 0x57); /* write key */
	ret += mt5785_write_byte(di, 0x5200, 0x20); /* hold Cortex M0 */
	if (type == MT5785_LOAD_BL_NORMAL) {
		ret += mt5785_write_word(di, 0x5218, 0x0FFF); /* enable map */
		ret += mt5785_write_byte(di, 0x5208, 0x08); /* enable map */
	} else {
		ret += mt5785_write_word(di, 0x5218, 0x20ff); /* enable map */
		ret += mt5785_write_byte(di, 0x5208, 0x0c); /* enable map */
	}
	return ret;
}

static int mt5785_fw_load_bootloader_disable_map(struct mt5785_dev_info *di)
{
	int ret;

	ret = mt5785_write_byte(di, 0x5244, 0x57); /* write key */
	ret += mt5785_write_byte(di, 0x5200, 0x20); /* hold Cortex M0 */
	ret += mt5785_write_word(di, 0x5218, 0x00); /* disable map */
	ret += mt5785_write_byte(di, 0x5208, 0x00); /* disable map */
	return ret;
}

static int mt5785_fw_reset(struct mt5785_dev_info *di)
{
	return mt5785_write_byte(di, 0x5200, 0x80); /* Cortex M0 reset */
}

static int mt5785_fw_load_bootloader(struct mt5785_dev_info *di, int type)
{
	int ret;
	u16 chipid = 0;
	u16 addr = MT5785_BTLOADER_ADDR;
	u8 *data = (u8 *)g_mt5785_bootloader;
	u32 size = MT5785_FW_BTL_SIZE;
	u16 chipid_addr = MT5785_MTP_CHIPID_ADDR;

	ret = mt5785_fw_disable_watchdog(di);
	if (ret) {
		hwlog_err("load bootloader: diable watchdog failed %d\n", ret);
		return ret;
	}

	ret = mt5785_fw_load_bootloader_enable_map(di, type);
	if (ret) {
		hwlog_err("load bootloader: enable map failed %d\n", ret);
		return ret;
	}

	power_msleep(DT_MSLEEP_50MS, 0, NULL);
	if (type == MT5785_LOAD_BL_CALI) {
		addr = MT5785_CALI_BTLOADER_ADDR;
		data = (u8 *)g_mt5785_bl_cali;
		size = MT5785_FW_BL_CALI_SIZE;
		chipid_addr = MT5785_CALI_MTP_CHIPID_ADDR;
	}
	ret = mt5785_fw_sram_write_data(di, addr, data, size);
	if (ret) {
		hwlog_err("load bootloader: load sram data failed %d\n", ret);
		return ret;
	}

	power_msleep(DT_MSLEEP_50MS, 0, NULL);
	ret = mt5785_fw_reset(di);
	if (ret) {
		hwlog_err("load bootloader: reset mcu failed\n");
		return -EIO;
	}

	power_msleep(DT_MSLEEP_50MS, 0, NULL);
	ret = mt5785_read_word(di, chipid_addr, &chipid);

	if (ret || (chipid != MT5785_MTP_CHIPID_VALUE)) {
		hwlog_err("load bootloader: run bootloader failed,chipid=0x%x\n", chipid);
		return -EIO;
	}

	hwlog_info("load bootloader: success\n");
	return 0;
}

int mt5785_fw_load_bootloader_cali(struct mt5785_dev_info *di)
{
	return mt5785_fw_load_bootloader(di, MT5785_LOAD_BL_CALI);
}

static int mt5785_fw_load_mtp(struct mt5785_dev_info *di, const u8 *mtp_data, u16 mtp_size)
{
	u32 i;
	u32 ret;
	u16 wr_already = 0;
	u16 status = 0;
	u16 addr = 0;
	u16 checksum;
	u16 remaining = mtp_size;
	u16 size_to_write;
	u8 buff[MT5785_MTP_BUFF_SIZE] = { 0 };

	while (remaining > 0) {
		memset(buff, 0, MT5785_MTP_BUFF_SIZE);
		size_to_write = remaining > MT5785_MTP_BUFF_SIZE ?
			MT5785_MTP_BUFF_SIZE : remaining;
		memcpy(buff, (mtp_data + wr_already), size_to_write);
		checksum = addr + size_to_write;
		for (i = 0; i < size_to_write; i++)
			checksum += buff[i];

		ret = mt5785_write_word(di, MT5785_MTP_PGM_ADDR_ADDR, addr);
		ret += mt5785_write_word(di, MT5785_MTP_PGM_LEN_ADDR, size_to_write);
		ret += mt5785_write_word(di, MT5785_MTP_PGM_CHKSUM_ADDR, checksum);
		ret += mt5785_fw_sram_write_data(di, MT5785_MTP_PGM_DATA_ADDR, buff, size_to_write);
		ret += mt5785_write_word(di, MT5785_MTP_PGM_CMD_ADDR, MT5785_MTP_PGM_CMD);
		if (ret) {
			hwlog_err("load_mtp: write failed\n");
			return ret;
		}

		for (i = 0; i < MT5785_MTP_PGM_CHECK_TIMES; i++) {
			ret = mt5785_read_word(di, MT5785_MTP_PGM_CMD_ADDR, &status);
			if (ret) {
				hwlog_err("load_mtp: read failed\n");
				return ret;
			}

			if (status == MT5785_MTP_PGM_SUCESS)
				break;

			power_usleep(DT_USLEEP_2MS);
		}

		if (status != MT5785_MTP_PGM_SUCESS) {
			hwlog_err("load_mtp: status failed\n");
			return -EIO;
		}

		wr_already += size_to_write;
		addr += size_to_write;
		remaining -= size_to_write;
	}

	return 0;
}

static int mt5785_fw_check_mtp(void *dev_data)
{
	int ret;
	struct mt5785_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	if (di->g_val.mtp_latest)
		return 0;

	mt5785_disable_irq_nosync(di);
	mt5785_fw_ps_control(di->ic_type, WLPS_RX_EXT_PWR, true);
	wlps_control(WLTRX_IC_MAIN, WLPS_RX_SW, false);
	power_msleep(DT_MSLEEP_100MS, 0, NULL); /* for power on */

	ret = mt5785_fw_check_mtp_version(di);
	if (ret) {
		hwlog_err("check_mtp: mtp_ver mismatch\n");
		goto exit;
	}

	ret = mt5785_fw_check_mtp_crc(di);
	if (ret) {
		hwlog_err("check_mtp: mtp_crc mismatch\n");
		goto exit;
	}

	di->g_val.mtp_latest = true;
	hwlog_info("[check_mtp] mtp latest\n");

exit:
	mt5785_enable_irq(di);
	mt5785_fw_ps_control(di->ic_type, WLPS_RX_EXT_PWR, false);
	return ret;
}

static int mt5785_fw_program_mtp(struct mt5785_dev_info *di, const u8 *mtp_data, u16 mtp_size)
{
	int ret;

	if (di->g_val.mtp_latest)
		return 0;

	mt5785_disable_irq_nosync(di);
	mt5785_fw_ps_control(di->ic_type, WLPS_PROC_OTP_PWR, true);
	(void)mt5785_chip_enable(true, di);
	power_msleep(DT_MSLEEP_100MS, 0, NULL); /* for power on */

	ret = mt5785_fw_load_bootloader(di, MT5785_LOAD_BL_NORMAL);
	if (ret)
		goto exit;

	ret = mt5785_fw_load_mtp(di, mtp_data, mtp_size);
	if (ret)
		goto exit;

	power_usleep(DT_USLEEP_10MS);

	ret = mt5785_fw_load_bootloader_disable_map(di);
	if (ret) {
		hwlog_err("fw_program_mtp: disable map fail\r\n");
		goto exit;
	}

	ret = mt5785_fw_reset(di);
	if (ret) {
		hwlog_err("fw_program_mtp: reset mcu fail\r\n");
		goto exit;
	}

	power_usleep(DT_USLEEP_10MS);
	ret = mt5785_fw_check_mtp_version(di);
	ret += mt5785_fw_check_mtp_crc(di);
	if (ret)
		hwlog_err("fw_program_mtp: failed\n");

	di->g_val.mtp_latest = true;
	hwlog_info("[fw_program_mtp] succ\n");

exit:
	mt5785_enable_irq(di);
	mt5785_fw_ps_control(di->ic_type, WLPS_PROC_OTP_PWR, false);
	return ret;
}

void mt5785_fw_mtp_check_work(struct work_struct *work)
{
	int i;
	int ret;
	struct mt5785_dev_info *di = container_of(work,
		struct mt5785_dev_info, mtp_check_work.work);

	if (!di)
		return;

	di->g_val.mtp_chk_complete = false;
	ret = mt5785_fw_check_mtp(di);
	if (!ret) {
		hwlog_info("[mtp_check_work] succ\n");
		goto exit;
	}

	/* program for 3 times until it's ok */
	for (i = 0; i < 3; i++) {
		ret = mt5785_fw_program_mtp(di, g_mt5785_fw, MT5785_FW_LENGTH);
		if (ret)
			continue;

		hwlog_info("[mtp_check_work] update mtp succ, cnt=%d\n", i + 1);
		goto exit;
	}

	hwlog_err("mtp_check_work: update mtp failed\n");

exit:
	di->g_val.mtp_chk_complete = true;
}

static int mt5785_fw_rx_program_mtp(unsigned int proc_type, void *dev_data)
{
	int ret;
	struct mt5785_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	hwlog_info("[rx_program_mtp] type=%u\n", proc_type);

	if (!di->g_val.mtp_chk_complete)
		return -EPERM;

	di->g_val.mtp_chk_complete = false;
	ret = mt5785_fw_program_mtp(di, g_mt5785_fw, MT5785_FW_LENGTH);
	if (!ret)
		hwlog_info("[rx_program_mtp] succ\n");

	di->g_val.mtp_chk_complete = true;

	return ret;
}

static int mt5785_mtp_crc_calc(const u8 *buf, u16 mtp_size)
{
	int i, j;
	u16 crc = MT5785_MTP_CRC_INIT;

	/* 2byte cycle, 1byte = 8bit */
	for (j = 0; j < mtp_size; j += 2) {
		crc ^= (buf[j + 1] << 8);
		for (i = 0; i < 8; i++)
			crc = (crc & MT5785_MTP_CRC_HIGHEST_BIT) ?
				(((crc << 1) & POWER_MASK_WORD) ^ MT5785_MTP_CRC_SEED) : (crc << 1);
		crc ^= (buf[j] << 8);
		for (i = 0; i < 8; i++)
			crc = (crc & MT5785_MTP_CRC_HIGHEST_BIT) ?
				(((crc << 1) & POWER_MASK_WORD) ^ MT5785_MTP_CRC_SEED) : (crc << 1);
	}

	hwlog_info("[mtp_crc_calc] crc=0x%x\n", crc);
	return crc;
}

static int mt5785_fw_get_mtp_status(unsigned int *status, void *dev_data)
{
	int ret;
	struct mt5785_dev_info *di = dev_data;

	if (!di || !status)
		return -EINVAL;

	di->g_val.mtp_chk_complete = false;
	ret = mt5785_fw_check_mtp(di);
	if (!ret)
		*status = WIRELESS_FW_PROGRAMED;
	else
		*status = WIRELESS_FW_ERR_PROGRAMED;

	di->g_val.mtp_chk_complete = true;

	return 0;
}

static ssize_t mt5785_fw_write(void *dev_data, const char *buf, size_t size)
{
	int hdr_size;
	u16 crc;
	struct power_fw_hdr *hdr = NULL;
	struct mt5785_dev_info *di = dev_data;

	if (!di || !buf)
		return -EINVAL;

	hdr = (struct power_fw_hdr *)buf;
	hdr_size = sizeof(struct power_fw_hdr);
	crc = mt5785_mtp_crc_calc((const u8 *)hdr + hdr_size, (u16)hdr->bin_size);
	hwlog_info("[fw_write] bin_size=%ld version_id=0x%x crc_id=0x%x\n",
		hdr->bin_size, hdr->version_id, hdr->crc_id);

	if ((hdr->unlock_val != WLTRX_UNLOCK_VAL) || (hdr->fw_size != hdr->bin_size) ||
		(hdr->crc_id != crc)) {
		hwlog_err("fw_write: config mismatch\n");
		return -EINVAL;
	}

	di->g_val.mtp_latest = false;
	di->fw_mtp.mtp_ver = hdr->version_id;
	di->fw_mtp.mtp_crc = hdr->crc_id;
	di->fw_mtp.mtp_len = hdr->bin_size;

	(void)mt5785_fw_check_mtp(di);
	if (di->g_val.mtp_latest)
		return size;

	(void)mt5785_fw_program_mtp(di, (const u8 *)hdr + hdr_size, (u16)hdr->bin_size);
	return size;
}

static struct wireless_fw_ops g_mt5785_fw_ops = {
	.ic_name                = "mt5785",
	.program_fw             = mt5785_fw_rx_program_mtp,
	.get_fw_status          = mt5785_fw_get_mtp_status,
	.check_fw               = mt5785_fw_check_mtp,
	.write_fw               = mt5785_fw_write,
};

int mt5785_fw_ops_register(struct wltrx_ic_ops *ops, struct mt5785_dev_info *di)
{
	if (!ops || !di)
		return -ENODEV;

	ops->fw_ops = kzalloc(sizeof(*(ops->fw_ops)), GFP_KERNEL);
	if (!ops->fw_ops)
		return -ENODEV;

	di->fw_mtp.mtp_ver = MT5785_FW_VER;
	di->fw_mtp.mtp_crc = MT5785_FW_CRC;
	di->fw_mtp.mtp_len = MT5785_FW_LENGTH;
	memcpy(ops->fw_ops, &g_mt5785_fw_ops, sizeof(g_mt5785_fw_ops));
	ops->fw_ops->dev_data = (void *)di;
	return wireless_fw_ops_register(ops->fw_ops, di->ic_type);
}
