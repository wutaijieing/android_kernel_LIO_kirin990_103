// SPDX-License-Identifier: GPL-2.0
/*
 * cps4067_fw.c
 *
 * cps4067 mtp, sram driver
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
#include "cps4067_fw.h"

#define HWLOG_TAG wireless_cps4067_fw
HWLOG_REGIST();

static int cps4067_fw_read_byte(struct cps4067_dev_info *di, u32 addr, u8 *data)
{
	return cps4067_hw_read_block(di, addr, data, POWER_BYTE_LEN);
}

static int cps4067_fw_write_dword(struct cps4067_dev_info *di, u32 addr, u32 data)
{
	return cps4067_hw_write_block(di, addr, (u8 *)&data, POWER_DWORD_LEN);
}

static void cps4067_fw_ps_control(unsigned int ic_type, int scene, bool flag)
{
	static int ref_cnt;

	hwlog_info("[ps_control] ref_cnt=%d, flag=%d\n", ref_cnt, flag);
	if (flag)
		++ref_cnt;
	else if (--ref_cnt > 0)
		return;

	wlps_control(ic_type, scene, flag);
}

int cps4067_fw_get_mtp_ver(struct cps4067_dev_info *di, u16 *mtp_ver)
{
	return cps4067_read_word(di, CPS4067_MTP_VER_ADDR, mtp_ver);
}

static int cps4067_fw_check_mtp_version(struct cps4067_dev_info *di)
{
	int ret;
	u16 mtp_ver = 0;

	ret = cps4067_fw_get_mtp_ver(di, &mtp_ver);
	if (ret)
		return ret;
	hwlog_info("[version_check] mtp_ver=0x%04x\n", mtp_ver);

	if (mtp_ver != di->fw_mtp.mtp_ver)
		return -ENXIO;

	return 0;
}

static int cps4067_fw_check_mtp_crc(struct cps4067_dev_info *di)
{
	int i;
	int ret;
	u16 crc = 0;

	ret = cps4067_write_dword_mask(di, CPS4067_TX_CMD_ADDR, CPS4067_TX_CMD_CRC_CHK,
		CPS4067_TX_CMD_CRC_CHK_SHIFT, CPS4067_TX_CMD_VAL);
	if (ret) {
		hwlog_err("check_mtp_crc: write cmd failed\n");
		return ret;
	}

	/* 100ms*5=500ms timeout for status check */
	for (i = 0; i < 5; i++) {
		power_msleep(DT_MSLEEP_100MS, 0, NULL);
		ret = cps4067_read_word(di, CPS4067_CRC_ADDR, &crc);
		if (ret) {
			hwlog_err("check_mtp_crc: get crc failed\n");
			return ret;
		}
		if (crc == di->fw_mtp.mtp_crc)
			return 0;
	}

	hwlog_err("check_mtp_crc: timeout, crc=0x%x\n", crc);
	return -EIO;
}

static int cps4067_fw_status_check(struct cps4067_dev_info *di, u32 cmd)
{
	int i;
	int ret;
	u8 status = 0;

	ret = cps4067_fw_write_dword(di, CPS4067_SRAM_CHK_CMD_ADDR, 0);
	if (ret) {
		hwlog_err("status_check: clear status failed\n");
		return ret;
	}
	ret = cps4067_fw_write_dword(di, CPS4067_SRAM_STRAT_CMD_ADDR, cmd);
	if (ret) {
		hwlog_err("status_check: set check cmd failed\n");
		return ret;
	}

	/* wait for 50ms*10=500ms for status check */
	for (i = 0; i < 10; i++) {
		power_msleep(DT_MSLEEP_50MS, 0, NULL);
		ret = cps4067_fw_read_byte(di, CPS4067_SRAM_CHK_CMD_ADDR, &status);
		if (ret) {
			hwlog_err("status_check: get status failed\n");
			return ret;
		}
		if (status == CPS4067_CHK_SUCC)
			return 0;
		if (status == CPS4067_CHK_FAIL) {
			hwlog_err("status_check: failed, status=0x%x\n", status);
			return -ENXIO;
		}
	}

	hwlog_err("status_check: status=0x%x, program timeout\n", status);
	return -ENXIO;
}

static int cps4067_fw_write_sram_data(struct cps4067_dev_info *di, u32 cur_addr,
	const u8 *data, int len)
{
	int ret;
	int size_to_wr;
	u32 wr_already = 0;
	u32 addr = cur_addr;
	int remaining = len;
	u8 wr_buff[CPS4067_FW_PAGE_SIZE] = { 0 };

	while (remaining > 0) {
		size_to_wr = remaining > CPS4067_FW_PAGE_SIZE ? CPS4067_FW_PAGE_SIZE : remaining;
		memcpy(wr_buff, data + wr_already, size_to_wr);
		ret = cps4067_hw_write_block(di, addr, wr_buff, size_to_wr);
		if (ret) {
			hwlog_err("write_sram_data: fail, addr=0x%x\n", addr);
			return ret;
		}
		addr += size_to_wr;
		wr_already += size_to_wr;
		remaining -= size_to_wr;
	}

	return 0;
}

static int cps4067_fw_write_mtp_data(struct cps4067_dev_info *di, const u8 *mtp_data, u16 mtp_size)
{
	int ret;
	int offset = 0;
	int remaining = mtp_size;
	int wr_size;

	while (remaining > 0) {
		wr_size = remaining > CPS4067_HW_MTP_BUFF_SIZE ?
			CPS4067_HW_MTP_BUFF_SIZE : remaining;
		ret = cps4067_fw_write_sram_data(di, CPS4067_SRAM_MTP_BUFF0,
			mtp_data + offset, wr_size);
		if (ret) {
			hwlog_err("write_mtp_data: write mtp failed\n");
			return ret;
		}
		ret = cps4067_fw_status_check(di, CPS4067_STRAT_CARRY_BUF0);
		if (ret) {
			hwlog_err("write_mtp_data: verify integrity failed\n");
			return ret;
		}
		offset += wr_size;
		remaining -= wr_size;
	}

	return 0;
}

static int cps4067_fw_load_mtp(struct cps4067_dev_info *di, const u8 *mtp_data, u16 mtp_size)
{
	int ret;

	ret = cps4067_fw_write_dword(di, 0x20001808, 0x00000200); /* wr_buff 512 dwords */
	if (ret) {
		hwlog_err("load_mtp: set write buff size failed\n");
		return ret;
	}
	ret = cps4067_fw_write_mtp_data(di, mtp_data, mtp_size);
	if (ret) {
		hwlog_err("load_mtp: write mtp data failed\n");
		return ret;
	}

	ret = cps4067_fw_status_check(di, CPS4067_START_CHK_MTP);
	if (ret) {
		hwlog_err("load_mtp: verify integrity failed\n");
		return ret;
	}

	hwlog_info("[load_mtp] succ\n");
	return 0;
}

static int cps4067_fw_check_bootloader(struct cps4067_dev_info *di)
{
	int ret;

	ret = cps4067_fw_write_dword(di, 0xFFFFFF00, 0x0000000E); /* config i2c */
	if (ret) {
		hwlog_err("check_bootloader: config i2c failed\n");
		return ret;
	}
	ret = cps4067_fw_status_check(di, CPS4067_START_CHK_BTL);
	if (ret) {
		hwlog_err("check_bootloader: check bootloader failed\n");
		return ret;
	}

	return 0;
}

static int cps4067_fw_load_bootloader(struct cps4067_dev_info *di)
{
	int ret;

	ret = cps4067_fw_write_sram_data(di, CPS4067_SRAM_BTL_ADDR,
		g_cps4067_bootloader, CPS4067_FW_BTL_SIZE);
	if (ret) {
		hwlog_err("load_bootloader: load bootloader data failed\n");
		return ret;
	}
	ret = cps4067_fw_write_dword(di, 0x400400A0, 0x000000FF); /* enable sram running */
	if (ret) {
		hwlog_err("load_bootloader: enable sram running failed\n");
		return ret;
	}
	ret = cps4067_fw_write_dword(di, 0x40040010, 0x00008003); /* reset system */
	if (ret) {
		hwlog_err("load_bootloader: reset system failed\n");
		return ret;
	}
	power_usleep(DT_USLEEP_10MS);
	ret = cps4067_fw_check_bootloader(di);
	if (ret) {
		hwlog_err("load_bootloader: check bootloader failed\n");
		return ret;
	}

	hwlog_info("[load_bootloader] succ\n");
	return 0;
}

static int cps4067_fw_program_post_process(struct cps4067_dev_info *di)
{
	int ret;

	ret = cps4067_fw_write_dword(di, 0x40040010, 0x0000000B); /* global system reset */
	if (ret) {
		hwlog_err("progam_post_process: global system reset failed\n");
		return ret;
	}
	cps4067_fw_ps_control(di->ic_type, WLPS_PROC_OTP_PWR, false);
	power_usleep(DT_USLEEP_10MS); /* for power off, typically 5ms */
	cps4067_fw_ps_control(di->ic_type, WLPS_PROC_OTP_PWR, true);
	power_msleep(DT_MSLEEP_100MS, 0, NULL); /* for power on, typically 50ms */

	ret = cps4067_fw_check_mtp_version(di);
	if (ret) {
		hwlog_err("progam_post_process: mtp_version mismatch\n");
		return ret;
	}

	return 0;
}

static int cps4067_fw_program_prev_process(struct cps4067_dev_info *di)
{
	int ret;

	ret = cps4067_fw_write_dword(di, 0xFFFFFF00, 0x0000000E); /* config i2c */
	if (ret) {
		hwlog_err("program_prev_process: config i2c failed\n");
		return ret;
	}

	ret = cps4067_fw_write_dword(di, 0x4000E75C, 0x00001250); /* unlock i2c */
	if (ret) {
		hwlog_err("program_prev_process: unlock i2c failed\n");
		return ret;
	}
	ret = cps4067_fw_write_dword(di, 0x40040010, 0x00000006); /* hold mcu */
	if (ret) {
		hwlog_err("program_prev_process: hold mcu failed\n");
		return ret;
	}

	return 0;
}

static int cps4067_fw_program_mtp(struct cps4067_dev_info *di, const u8 *mtp_data, u16 mtp_size)
{
	int ret;

	if (di->g_val.mtp_latest)
		return 0;

	cps4067_disable_irq_nosync(di);
	cps4067_fw_ps_control(di->ic_type, WLPS_PROC_OTP_PWR, true);
	(void)cps4067_chip_enable(true, di);
	power_msleep(DT_MSLEEP_100MS, 0, NULL); /* for power on, typically 50ms */

	ret = cps4067_fw_program_prev_process(di);
	if (ret)
		goto exit;
	power_msleep(DT_MSLEEP_100MS, 0, NULL); /* for power on, typically 50ms */
	ret = cps4067_fw_load_bootloader(di);
	if (ret)
		goto exit;
	ret = cps4067_fw_load_mtp(di, mtp_data, mtp_size);
	if (ret)
		goto exit;
	ret = cps4067_fw_program_post_process(di);
	if (ret)
		goto exit;

	di->g_val.mtp_latest = true;
	hwlog_info("[program_mtp] succ\n");

exit:
	cps4067_enable_irq(di);
	cps4067_fw_ps_control(di->ic_type, WLPS_PROC_OTP_PWR, false);
	return ret;
}

static int cps4067_fw_rx_program_mtp(unsigned int proc_type, void *dev_data)
{
	int ret;
	struct cps4067_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	hwlog_info("[rx_program_mtp] type=%u\n", proc_type);

	if (!di->g_val.mtp_chk_complete)
		return -EPERM;

	di->g_val.mtp_chk_complete = false;
	ret = cps4067_fw_program_mtp(di, g_cps4067_mtp, CPS4067_FW_MTP_SIZE);
	if (!ret)
		hwlog_info("[rx_program_mtp] succ\n");
	di->g_val.mtp_chk_complete = true;

	return ret;
}

static int cps4067_fw_check_mtp(void *dev_data)
{
	int ret;
	struct cps4067_dev_info *di = dev_data;

	if (!di)
		return -ENODEV;

	if (di->g_val.mtp_latest)
		return 0;

	cps4067_disable_irq_nosync(di);
	cps4067_fw_ps_control(di->ic_type, WLPS_RX_EXT_PWR, true);
	wlps_control(WLTRX_IC_MAIN, WLPS_RX_SW, false);
	power_msleep(DT_MSLEEP_100MS, 0, NULL); /* for power on, typically 50ms */

	ret = cps4067_fw_check_mtp_version(di);
	if (ret) {
		hwlog_err("check_mtp: mtp_ver mismatch\n");
		goto exit;
	}

	ret = cps4067_fw_check_mtp_crc(di);
	if (ret) {
		hwlog_err("check_mtp: mtp_crc mismatch\n");
		goto exit;
	}
	di->g_val.mtp_latest = true;
	hwlog_info("[check_mtp] mtp latest\n");

exit:
	cps4067_enable_irq(di);
	cps4067_fw_ps_control(di->ic_type, WLPS_RX_EXT_PWR, false);
	return ret;
}

int cps4067_fw_sram_update(void *dev_data)
{
	int ret;
	struct cps4067_chip_info info = { 0 };

	ret = cps4067_get_chip_info(dev_data, &info);
	if (!ret)
		hwlog_info("[chip_info] chip_id=0x%x mtp_ver=0x%04x\n",
			info.chip_id, info.mtp_ver);

	return 0;
}

static int cps4067_fw_get_mtp_status(unsigned int *status, void *dev_data)
{
	int ret;
	struct cps4067_dev_info *di = dev_data;

	if (!di || !status)
		return -EINVAL;

	di->g_val.mtp_chk_complete = false;
	ret = cps4067_fw_check_mtp(di);
	if (!ret)
		*status = WIRELESS_FW_PROGRAMED;
	else
		*status = WIRELESS_FW_ERR_PROGRAMED;
	di->g_val.mtp_chk_complete = true;

	return 0;
}

void cps4067_fw_mtp_check_work(struct work_struct *work)
{
	int i;
	int ret;
	struct cps4067_dev_info *di = container_of(work,
		struct cps4067_dev_info, mtp_check_work.work);

	if (!di)
		return;

	di->g_val.mtp_chk_complete = false;
	ret = cps4067_fw_check_mtp(di);
	if (!ret) {
		hwlog_info("[mtp_check_work] succ\n");
		goto exit;
	}

	/* program for 3 times until it's ok */
	for (i = 0; i < 3; i++) {
		ret = cps4067_fw_program_mtp(di, g_cps4067_mtp, CPS4067_FW_MTP_SIZE);
		if (ret)
			continue;
		hwlog_info("[mtp_check_work] update mtp succ, cnt=%d\n", i + 1);
		goto exit;
	}
	hwlog_err("mtp_check_work: update mtp failed\n");

exit:
	di->g_val.mtp_chk_complete = true;
}

static u16 cps4067_mtp_crc_calc(const u8 *buf, u16 mtp_size)
{
	int i, j;
	u16 crc_in = 0x0;
	u16 crc_poly = CPS4067_MTP_CRC_SEED;

	for (i = 0; i < mtp_size; i++) {
		crc_in ^= buf[i] << POWER_BITS_PER_BYTE;
		for (j = 0; j < POWER_BITS_PER_BYTE; j++) {
			if (crc_in & CPS4067_MTP_CRC_HIGHEST_BIT)
				crc_in = (crc_in << 1) ^ crc_poly;
			else
				crc_in = crc_in << 1;
		}
	}

	hwlog_info("[mtp_crc_calc] crc_in=0x%x\n", crc_in);
	return crc_in;
}

static ssize_t cps4067_fw_write(void *dev_data, const char *buf, size_t size)
{
	int hdr_size;
	u16 crc;
	struct power_fw_hdr *hdr = NULL;
	struct cps4067_dev_info *di = (struct cps4067_dev_info *)dev_data;

	if (!di || !buf)
		return -EINVAL;

	hdr = (struct power_fw_hdr *)buf;
	hdr_size = sizeof(struct power_fw_hdr);
	crc = cps4067_mtp_crc_calc((const u8 *)hdr + hdr_size, (u16)hdr->bin_size);
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

	(void)cps4067_fw_check_mtp(di);
	if (di->g_val.mtp_latest)
		return size;

	(void)cps4067_fw_program_mtp(di, (const u8 *)hdr + hdr_size, (u16)hdr->bin_size);
	return size;
}

static struct wireless_fw_ops g_cps4067_fw_ops = {
	.ic_name                = "cps4067",
	.program_fw             = cps4067_fw_rx_program_mtp,
	.get_fw_status          = cps4067_fw_get_mtp_status,
	.check_fw               = cps4067_fw_check_mtp,
	.write_fw               = cps4067_fw_write,
};

int cps4067_fw_ops_register(struct wltrx_ic_ops *ops, struct cps4067_dev_info *di)
{
	if (!ops || !di)
		return -ENODEV;

	ops->fw_ops = kzalloc(sizeof(*(ops->fw_ops)), GFP_KERNEL);
	if (!ops->fw_ops)
		return -ENODEV;

	di->fw_mtp.mtp_ver = CPS4067_MTP_VER;
	di->fw_mtp.mtp_crc = CPS4067_MTP_CRC;
	memcpy(ops->fw_ops, &g_cps4067_fw_ops, sizeof(g_cps4067_fw_ops));
	ops->fw_ops->dev_data = (void *)di;
	return wireless_fw_ops_register(ops->fw_ops, di->ic_type);
}
