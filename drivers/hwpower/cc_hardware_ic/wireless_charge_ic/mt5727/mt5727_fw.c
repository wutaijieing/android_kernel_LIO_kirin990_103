// SPDX-License-Identifier: GPL-2.0
/*
 * mt5727_fw.c
 *
 * mt5727 mtp, sram driver
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

#include "mt5727.h"
#include "mt5727_mtp.h"

#define HWLOG_TAG wireless_mt5727_fw
HWLOG_REGIST();

static int mt5727_get_major_fw_version(struct mt5727_dev_info *di, u16 *fw)
{
	return mt5727_read_word(di, MT5727_MTP_MAJOR_ADDR, fw);
}

static int mt5727_get_minor_fw_version(struct mt5727_dev_info *di, u16 *fw)
{
	u8 min_h = 0;
	u8 min_l = 0;
	int ret;

	ret = mt5727_read_byte(di, MT5727_MTP_MINOR_ADDR, &min_l);
	ret += mt5727_read_byte(di, MT5727_MTP_MINOR_ADDR_H, &min_h);
	*fw = (min_h << 8) | min_l;

	return ret;
}

static int mt5727_mtp_version_check(struct mt5727_dev_info *di)
{
	int ret;
	u16 major_fw_ver = 0;
	u16 minor_fw_ver = 0;

	ret = mt5727_get_major_fw_version(di, &major_fw_ver);
	if (ret)
		return ret;
	hwlog_info("version_check: major_fw=0x%04x\n", major_fw_ver);

	ret = mt5727_get_minor_fw_version(di, &minor_fw_ver);
	if (ret)
		return ret;
	hwlog_info("version_check: minor_fw=0x%04x\n", minor_fw_ver);

	if (minor_fw_ver != MT5727_MTP_MINOR_VER)
		return -ENXIO;

	return 0;
}

static int mt5727_mtp_pwr_cycle_chip(struct mt5727_dev_info *di)
{
	int ret;

	ret = mt5727_write_byte(di, MT5727_PMU_WDGEN_ADDR,
		MT5727_WDG_DISABLE); /* disable wtd */
	ret += mt5727_write_byte(di, MT5727_PMU_FLAG_ADDR,
		MT5727_WDT_INTFALG); /* clear wtd flag */
	ret += mt5727_write_byte(di, MT5727_SYS_KEY_ADDR,
		MT5727_KEY_VAL); /* write key to map */
	ret += mt5727_write_byte(di, MT5727_M0_CTRL_ADDR,
		MT5727_M0_HOLD_VAL); /* hold M0 */
	ret += mt5727_write_word(di, MT5727_SRAM_REMAP_ADDR,
		MT5727_SRAM_REMAP_VAL); /* select sram map addr */
	ret += mt5727_write_byte(di, MT5727_CODE_REMAP_ADDR,
		MT5727_CODE_REMAP_VAL); /* select mtp map addr */
	if (ret) {
		hwlog_err("pwr_cycle_chip: failed\n");
		return -EIO;
	}

	(void)power_msleep(DT_MSLEEP_50MS, 0, NULL); /* for power on, typically 20ms */
	return 0;
}

static int mt5727_mtp_load_bootloader(struct mt5727_dev_info *di)
{
	int ret;
	int remaining = ARRAY_SIZE(g_mt5727_bootloader);
	int size_to_wr;
	int wr_already = 0;
	u16 chip_id = 0;
	u16 addr = MT5727_BTLOADR_ADDR;
	u8 wr_buff[MT5727_MTP_PGM_SIZE] = { 0 };

	while (remaining > 0) {
		size_to_wr = remaining > MT5727_MTP_PGM_SIZE ?
			MT5727_MTP_PGM_SIZE : remaining;
		memcpy(wr_buff, g_mt5727_bootloader + wr_already, size_to_wr);
		ret = mt5727_write_block(di, addr, wr_buff, size_to_wr);
		if (ret) {
			hwlog_err("load_bootloader: failed, addr=0x%04x\n", addr);
			return ret;
		}
		addr += size_to_wr;
		wr_already += size_to_wr;
		remaining -= size_to_wr;
	}

	ret = mt5727_write_byte(di, MT5727_M0_CTRL_ADDR, MT5727_M0_RST_VAL);
	if (ret) {
		hwlog_err("load_bootloader: reset M0 failed\n");
		return ret;
	}
	(void)power_msleep(DT_MSLEEP_50MS, 0, NULL); /* for power on, typically 20ms */
	ret = mt5727_read_word(di, MT5727_MTP_CHIP_ID_ADDR, &chip_id);
	if (ret)
		return ret;

	hwlog_info("load_bootloader: chip_id=0x%x\n", chip_id);
	if (chip_id != MT5727_CHIP_ID)
		return -ENXIO;

	hwlog_info("[load_bootloader] succ\n");
	return 0;
}

static int mt5727_mtp_status_check(struct mt5727_dev_info *di, u16 expect_status)
{
	int i;
	int ret;
	u16 status = 0;

	/* wait for 10ms*50=500ms for status check, typically 300ms */
	for (i = 0; i < 50; i++) {
		power_usleep(DT_USLEEP_10MS);
		ret = mt5727_read_word(di, MT5727_MTP_STATUS_ADDR, &status);
		if (ret) {
			hwlog_err("status_check: read failed\n");
			return ret;
		}
		if (status == expect_status)
			return 0;
	}

	return -ENXIO;
}

static int mt5727_mtp_crc_check(struct mt5727_dev_info *di)
{
	int ret;

	ret = mt5727_mtp_load_bootloader(di);
	if (ret) {
		hwlog_err("crc_check: load bootloader failed\n");
		return ret;
	}

	/* write mtp start address */
	ret = mt5727_write_word(di, MT5727_MTP_PGM_ADDR_ADDR, MT5727_MTP_PGM_ADDR_VAL);
	/* write 16-bit MTP data size */
	ret += mt5727_write_word(di, MT5727_MTP_PGM_LEN_ADDR, (u16)ARRAY_SIZE(g_mt5727_mtp));
	/* write the 16-bit CRC */
	ret += mt5727_write_word(di, MT5727_MTP_PGM_CHKSUM_ADDR, MT5727_MTP_CRC);
	/* start MTP data CRC-16 check */
	ret += mt5727_write_word(di, MT5727_MTP_CRC_ADDR, MT5727_MTP_CRC_CMD);
	ret += mt5727_mtp_status_check(di, MT5727_MTP_CRC_OK);
	if (ret) {
		hwlog_err("crc_check: failed\n");
		return ret;
	}

	hwlog_err("[crc_check] succ\n");
	return 0;
}

static int mt5727_check_mtp_match(struct mt5727_dev_info *di)
{
	int ret;

	ret = mt5727_mtp_version_check(di);
	if (ret)
		return ret;

	ret = mt5727_mtp_pwr_cycle_chip(di);
	if (ret)
		return ret;

	ret = mt5727_mtp_crc_check(di);
	if (ret)
		return ret;

	return 0;
}

static int mt5727_mtp_load_fw(struct mt5727_dev_info *di)
{
	int i;
	int ret;
	u16 addr = 0; /* start from adrr 0 */
	u16 remaining = (u16)ARRAY_SIZE(g_mt5727_mtp);
	u16 wr_size;
	u16 chksum;

	hwlog_info("[load_fw] mtp_size=%d\n", remaining);
	while (remaining > 0) {
		wr_size = remaining > MT5727_MTP_PGM_SIZE ?
			MT5727_MTP_PGM_SIZE : remaining;
		ret = mt5727_write_word(di, MT5727_MTP_PGM_ADDR_ADDR, addr);
		if (ret) {
			hwlog_err("load_fw: write addr failed\n");
			return ret;
		}
		ret = mt5727_write_word(di, MT5727_MTP_PGM_LEN_ADDR, wr_size);
		if (ret) {
			hwlog_err("load_fw: write len failed\n");
			return ret;
		}
		chksum = addr + wr_size;
		for (i = 0; i < wr_size; i++)
			chksum += g_mt5727_mtp[addr + i];
		ret = mt5727_write_word(di, MT5727_MTP_PGM_CHKSUM_ADDR, chksum);
		if (ret) {
			hwlog_err("load_fw: write checksum failed\n");
			return ret;
		}
		ret = mt5727_write_block(di, MT5727_MTP_PGM_DATA_ADDR,
			(u8 *)&g_mt5727_mtp[addr], wr_size);
		if (ret) {
			hwlog_err("load_fw: write mtp_data failed\n");
			return ret;
		}
		ret = mt5727_write_byte(di, MT5727_MTP_PGM_CMD_ADDR, MT5727_MTP_PGM_CMD);
		if (ret) {
			hwlog_err("load_fw: start programming failed\n");
			return ret;
		}
		ret = mt5727_mtp_status_check(di, MT5727_MTP_PGM_OK);
		if (ret) {
			hwlog_err("load_fw: check mtp status failed\n");
			return ret;
		}
		addr += wr_size;
		remaining -= wr_size;
	}

	return 0;
}

static int mt5727_fw_program_mtp(struct mt5727_dev_info *di)
{
	int ret;

	if (di->g_val.mtp_latest)
		return 0;

	mt5727_disable_irq_nosync(di);
	wlps_control(di->ic_type, WLPS_TX_PWR_SW, true);
	(void)mt5727_chip_enable(true, di);
	(void)power_msleep(DT_MSLEEP_100MS, 0, NULL); /* for power on, typically 50ms */

	ret = mt5727_mtp_pwr_cycle_chip(di);
	if (ret) {
		hwlog_err("program_mtp: power cycle chip failed\n");
		goto exit;
	}
	ret = mt5727_mtp_load_bootloader(di);
	if (ret) {
		hwlog_err("program_mtp: load bootloader failed\n");
		goto exit;
	}
	ret = mt5727_mtp_load_fw(di);
	if (ret) {
		hwlog_err("program_mtp: load fw failed\n");
		goto exit;
	}
	wlps_control(di->ic_type, WLPS_TX_PWR_SW, false);
	power_usleep(DT_USLEEP_10MS); /* for power off, typically 5ms */
	wlps_control(di->ic_type, WLPS_TX_PWR_SW, true);
	power_usleep(DT_USLEEP_10MS); /* for power on, typically 5ms */
	ret = mt5727_check_mtp_match(di);
	if (ret) {
		hwlog_err("program_mtp: mtp mismatch\n");
		goto exit;
	}

	di->g_val.mtp_latest = true;
	hwlog_info("[program_mtp] succ\n");

exit:
	mt5727_enable_irq(di);
	wlps_control(di->ic_type, WLPS_TX_PWR_SW, false);
	return ret;
}

static int mt5727_fw_rx_program_mtp(unsigned int proc_type, void *dev_data)
{
	int ret;
	struct mt5727_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	hwlog_info("rx_program_mtp: type=%u\n", proc_type);
	if (!di->g_val.mtp_chk_complete)
		return -EINVAL;
	di->g_val.mtp_chk_complete = false;
	ret = mt5727_fw_program_mtp(di);
	if (!ret)
		hwlog_info("[rx_program_mtp] succ\n");
	di->g_val.mtp_chk_complete = true;

	return ret;
}

static int mt5727_fw_check_mtp(void *dev_data)
{
	int ret;
	struct mt5727_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	if (di->g_val.mtp_latest)
		return 0;

	mt5727_disable_irq_nosync(di);
	wlps_control(di->ic_type, WLPS_TX_PWR_SW, true);
	(void)power_msleep(DT_MSLEEP_100MS, 0, NULL); /* for power on, typically 50ms */

	ret = mt5727_check_mtp_match(di);
	if (ret)
		goto exit;

	di->g_val.mtp_latest = true;
	hwlog_info("[check_mtp] mtp latest\n");

exit:
	mt5727_enable_irq(di);
	wlps_control(di->ic_type, WLPS_TX_PWR_SW, false);
	return ret;
}

int mt5727_fw_sram_update(void *dev_data)
{
	int ret;
	u16 minor_fw_ver = 0;
	struct mt5727_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	ret = mt5727_get_minor_fw_version(di, &minor_fw_ver);
	if (ret) {
		hwlog_err("sram_update: get minor fw_ver failed\n");
		return ret;
	}
	hwlog_info("mtp_version=0x%x\n", minor_fw_ver);

	return 0;
}

static int mt5727_fw_get_mtp_status(unsigned int *status, void *dev_data)
{
	int ret;
	struct mt5727_dev_info *di = dev_data;

	if (!di || !status)
		return -EINVAL;

	di->g_val.mtp_chk_complete = false;
	ret = mt5727_fw_check_mtp(di);
	if (!ret)
		*status = WIRELESS_FW_PROGRAMED;
	else
		*status = WIRELESS_FW_ERR_PROGRAMED;
	di->g_val.mtp_chk_complete = true;

	return 0;
}

void mt5727_fw_mtp_check_work(struct work_struct *work)
{
	int i;
	int ret;
	struct mt5727_dev_info *di = container_of(work,
		struct mt5727_dev_info, mtp_check_work.work);

	if (!di)
		return;

	di->g_val.mtp_chk_complete = false;
	ret = mt5727_fw_check_mtp(di);
	if (!ret) {
		hwlog_info("[mtp_check_work] succ\n");
		goto exit;
	}

	/* program for 3 times until it's ok */
	for (i = 0; i < 3; i++) {
		ret = mt5727_fw_program_mtp(di);
		if (ret)
			continue;
		hwlog_info("mtp_check_work: update mtp succ, cnt=%d\n", i + 1);
		goto exit;
	}
	hwlog_err("mtp_check_work: update mtp failed\n");

exit:
	di->g_val.mtp_chk_complete = true;
}

static struct wireless_fw_ops g_mt5727_fw_ops = {
	.ic_name       = "mt5727",
	.program_fw    = mt5727_fw_rx_program_mtp,
	.get_fw_status = mt5727_fw_get_mtp_status,
	.check_fw      = mt5727_fw_check_mtp,
};

int mt5727_fw_ops_register(struct wltrx_ic_ops *ops, struct mt5727_dev_info *di)
{
	if (!ops || !di)
		return -ENODEV;

	ops->fw_ops = kzalloc(sizeof(*(ops->fw_ops)), GFP_KERNEL);
	if (!ops->fw_ops)
		return -ENOMEM;

	memcpy(ops->fw_ops, &g_mt5727_fw_ops, sizeof(g_mt5727_fw_ops));
	ops->fw_ops->dev_data = (void *)di;

	return wireless_fw_ops_register(ops->fw_ops, di->ic_type);
}
