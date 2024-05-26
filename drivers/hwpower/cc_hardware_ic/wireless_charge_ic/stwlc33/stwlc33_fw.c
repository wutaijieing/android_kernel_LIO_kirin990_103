// SPDX-License-Identifier: GPL-2.0
/*
 * stwlc33_fw.c
 *
 * stwlc33 nvm, sram driver
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
#include "stwlc33_fw.h"

#define HWLOG_TAG wireless_stwlc33_fw
HWLOG_REGIST();

static u8 g_st_nvm_bufs[STWLC33_NVM_SEC_VAL_SIZE];

static int stwlc33_fw_check_cmd_status(struct stwlc33_dev_info *di)
{
	int ret;
	u16 cmd_status = 0;

	ret = stwlc33_read_word(di, STWLC33_CMD_STATUS_ADDR, &cmd_status);
	if (ret) {
		hwlog_err("check_cmd_status: get cmd status failed\n");
		return ret;
	}
	if (cmd_status) {
		hwlog_err("check_cmd_status: pre cmd not completed [0x%x]\n", cmd_status);
		return -EINVAL;
	}

	return 0;
}

static char *stwlc33_fw_nvm_read(struct stwlc33_dev_info *di, int sec_no)
{
	int i;
	int ret;
	static char nvm_buf[STWLC33_NVM_VALUE_SIZE];
	char temp[STWLC33_NVM_SEC_VAL_SIZE] = { 0 };
	u8 sec_val[STWLC33_SEC_NO_SIZE] = { 0 };

	if (!di || (sec_no < 0) || (sec_no > STWLC33_NVM_SEC_NO_MAX)) {
		hwlog_err("nvm_read: sec_no or di invalid\n");
		return "error";
	}

	memset(nvm_buf, 0, STWLC33_NVM_VALUE_SIZE);
	/* step1: check for zero, to confirm pre cmds completed */
	if (stwlc33_fw_check_cmd_status(di))
		return "error";
	/* step2: update i2c sector no */
	sec_val[0] = sec_no;
	ret = stwlc33_write_block(di, STWLC33_OFFSET_REG_ADDR, sec_val, STWLC33_SEC_NO_SIZE);
	if (ret) {
		hwlog_err("nvm_read: write sector no failed\n");
		return "error";
	}
	/* step3: update i2c sector value length */
	ret = stwlc33_write_byte(di, STWLC33_SRAM_SIZE_ADDR, STWLC33_NVM_SEC_VAL_SIZE - 1);
	if (ret) {
		hwlog_err("nvm_read: write sector length failed\n");
		return "error";
	}
	/* step4: write i2c nvm read cmd val */
	ret = stwlc33_write_byte(di, STWLC33_CMD_STATUS_ADDR, STWLC33_NVM_RD_CMD);
	if (ret) {
		hwlog_err("nvm_read: write nvm read cmd failed\n");
		return "error";
	}
	/* step5: check for zero, to confirm pre cmds completed */
	if (stwlc33_fw_check_cmd_status(di))
		return "error";
	/* step6: read back corresponding nvm sector content */
	ret = stwlc33_read_block(di, STWLC33_NVM_ADDR, g_st_nvm_bufs, STWLC33_NVM_SEC_VAL_SIZE);
	if (ret) {
		hwlog_err("nvm_read: read NVM data failed\n");
		return "error";
	}
	for (i = 0; i < STWLC33_NVM_SEC_VAL_SIZE; i++) {
		snprintf(temp, STWLC33_NVM_SEC_VAL_SIZE, "%x ", g_st_nvm_bufs[i]);
		strncat(nvm_buf, temp, strlen(temp));
	}

	return nvm_buf;
}

static int stwlc33_fw_check_nvm(struct stwlc33_dev_info *di)
{
	int i;
	int j;
	int nvm_update_flag = STWLC33_NVM_PROGRAMED;
	u8 *nvm_fw_data = NULL;

	for (i = 0; i < ARRAY_SIZE(g_st_fw_nvm_data); i++) {
		if ((g_st_fw_nvm_data[i].sec_size != STWLC33_NVM_SEC_VAL_SIZE) ||
			(g_st_fw_nvm_data[i].sec_no < 0) ||
			(g_st_fw_nvm_data[i].sec_no > STWLC33_NVM_SEC_NO_MAX)) {
			hwlog_err("check_nvm: nvm sec no is invalid\n");
			return -EINVAL;
		}
		nvm_fw_data = stwlc33_fw_nvm_read(di, g_st_fw_nvm_data[i].sec_no);
		if (strcmp(nvm_fw_data, "error") == 0) {
			hwlog_err("check_nvm: read nvm data:%d failed\n",
				g_st_fw_nvm_data[i].sec_no);
			return STWLC33_NVM_ERR_PROGRAMED;
		}
		for (j = 0; j < g_st_fw_nvm_data[i].sec_size; j++) {
			if (g_st_nvm_bufs[j] != g_st_fw_nvm_data[i].sec_data[j]) {
				g_st_fw_nvm_data[i].same_flag = 0;
				nvm_update_flag = STWLC33_NVM_NON_PROGRAMED;
				hwlog_info("[check_nvm] sec_no:%d not same\n",
					g_st_fw_nvm_data[i].sec_no);
				break;
			}
		}
		if (j == g_st_fw_nvm_data[i].sec_size)
			g_st_fw_nvm_data[i].same_flag = 1;
	}

	return nvm_update_flag;
}

static int stwlc33_fw_nvm_write(struct stwlc33_dev_info *di)
{
	int i;
	int ret;
	u8 sec_val[STWLC33_SEC_NO_SIZE] = { 0 };

	for (i = 0; i < ARRAY_SIZE(g_st_fw_nvm_data); i++) {
		if (g_st_fw_nvm_data[i].same_flag)
			continue;
		/* step1: check for zero, to confirm pre cmds completed */
		if (stwlc33_fw_check_cmd_status(di))
			return -EINVAL;
		/* step2: write new nvm sector content  */
		ret = stwlc33_write_block(di, STWLC33_NVM_ADDR,
			(u8 *)g_st_fw_nvm_data[i].sec_data, g_st_fw_nvm_data[i].sec_size);
		if (ret) {
			hwlog_err("nvm_write: write nvm failed\n");
			return ret;
		}
		/* step3: update sector number */
		sec_val[0] = g_st_fw_nvm_data[i].sec_no;
		ret = stwlc33_write_block(di, STWLC33_OFFSET_REG_ADDR, sec_val,
			STWLC33_SEC_NO_SIZE);
		if (ret) {
			hwlog_err("nvm_write: write sector no failed\n");
			return ret;
		}
		/* step4: update nvm byte length */
		ret = stwlc33_write_byte(di, STWLC33_SRAM_SIZE_ADDR,
			g_st_fw_nvm_data[i].sec_size - 1);
		if (ret) {
			hwlog_err("nvm_write: write sector length failed\n");
			return ret;
		}
		/* step5: write nvm_wr command */
		ret = stwlc33_write_byte(di, STWLC33_CMD_STATUS_ADDR, STWLC33_NVM_WR_CMD);
		if (ret) {
			hwlog_err("nvm_write: write nvm write cmd failed\n");
			return ret;
		}
		(void)power_msleep(STWLC33_NVM_WR_TIME, 0, NULL);
		/* step6: check for zero, to confirm pre cmds completed */
		if (stwlc33_fw_check_cmd_status(di))
			return -EINVAL;
	}

	hwlog_info("[nvm_write] success\n");
	return 0;
}

static int stwlc33_fw_program_nvm(struct stwlc33_dev_info *di)
{
	int ret;

	if (!di)
		return -ENODEV;

	ret = stwlc33_fw_check_nvm(di);
	if (ret < 0) {
		hwlog_err("program_nvm: check nvm failed\n");
		return -EINVAL;
	}
	if (ret == STWLC33_NVM_PROGRAMED) {
		hwlog_info("[program_nvm] NVM is already programed\n");
		return 0;
	}

	stwlc33_disable_irq_nosync(di);
	ret = stwlc33_fw_nvm_write(di);
	if (ret)
		hwlog_err("program_nvm: write nvm failed\n");
	stwlc33_enable_irq(di);

	hwlog_info("[program_nvm] success\n");
	return ret;
}

static int stwlc33_fw_write_ram_data(struct stwlc33_dev_info *di,
	const struct fw_update_info *fw_sram_info)
{
	int i;
	int ret;
	u16 offset;
	u8 write_size;
	u8 st_fw_start_addr[STWLC33_OFFSET_VALUE_SIZE] = { 0x00, 0x00, 0x00, 0x00 };

	if (!di || !fw_sram_info || !fw_sram_info->fw_sram) {
		hwlog_err("write_ram: para is null\n");
		return -EINVAL;
	}

	/* write fw into sram */
	i = fw_sram_info->fw_sram_size;
	offset = 0;
	while (i > 0) {
		write_size = (i >= STWLC33_PAGE_SIZE) ? STWLC33_PAGE_SIZE : i;
		/* step1: confirm previous commands completed */
		if (stwlc33_fw_check_cmd_status(di))
			return -EINVAL;
		/* step2: load data */
		ret = stwlc33_write_block(di, fw_sram_info->fw_sram_update_addr,
			(u8 *)(fw_sram_info->fw_sram + offset), write_size);
		if (ret) {
			hwlog_err("write_ram: write SRAM fm failed\n");
			return ret;
		}
		/* step3: update i2c reg mem_access_offset */
		st_fw_start_addr[0] = offset & POWER_MASK_BYTE;
		st_fw_start_addr[1] = offset >> POWER_BITS_PER_BYTE;
		ret = stwlc33_write_block(di, STWLC33_OFFSET_REG_ADDR,
			st_fw_start_addr, STWLC33_OFFSET_VALUE_SIZE);
		/* step4: update number of bytes */
		ret += stwlc33_write_byte(di, STWLC33_SRAM_SIZE_ADDR, write_size - 1);
		if (ret) {
			hwlog_err("write_ram: Update write size failed\n");
			return -EIO;
		}
		/* step5: issue patch_wr command */
		ret = stwlc33_write_byte(di, STWLC33_ACT_CMD_ADDR, STWLC33_WRITE_CMD_VALUE);
		if (ret) {
			hwlog_err("write_ram: Issue patch_wr cmd failed\n");
			return ret;
		}
		offset += write_size;
		i -= write_size;
	}

	hwlog_info("[write_ram] write FW into SRAM succ\n");
	return 0;
}

static int stwlc33_fw_check_sram_data(const unsigned char *src,
	const unsigned char *dest, u16 len)
{
	u16 i;

	if (!src || !dest) {
		hwlog_err("check_fw_sram: input vaild\n");
		return -EINVAL;
	}

	for (i = 0; i < len; i++) {
		if (src[i] != dest[i]) {
			hwlog_err("check_fw_sram: read 0x%x != write 0x%x\n", src[i], dest[i]);
			return -EINVAL;
		}
	}

	return 0;
}

static int stwlc33_fw_check_ram_data(struct stwlc33_dev_info *di,
	const struct fw_update_info *fw_sram_info)
{
	int i;
	int ret;
	u16 offset;
	u8 write_size;
	/* write datas must offset 2 bytes */
	u8 bufs[STWLC33_PAGE_SIZE + STWLC33_ADDR_LEN] = { 0 };
	u8 st_fw_start_addr[STWLC33_OFFSET_VALUE_SIZE] = { 0x00, 0x00, 0x00, 0x00 };

	if (!di || !fw_sram_info || !fw_sram_info->fw_sram)
		return -EINVAL;

	/* read ram patch and check */
	i = fw_sram_info->fw_sram_size;
	offset = 0;
	while (i > 0) {
		write_size = (i >= STWLC33_PAGE_SIZE) ? STWLC33_PAGE_SIZE : i;
		/* step1: confirm previous commands completed */
		if (stwlc33_fw_check_cmd_status(di))
			return -EINVAL;
		/* step2: update i2c reg mem_access_offset */
		st_fw_start_addr[0] = offset & POWER_MASK_BYTE;
		st_fw_start_addr[1] = offset >> POWER_BITS_PER_BYTE;
		ret = stwlc33_write_block(di, STWLC33_OFFSET_REG_ADDR,
			st_fw_start_addr, STWLC33_OFFSET_VALUE_SIZE);
		/* step3: update number of bytes */
		ret += stwlc33_write_byte(di, STWLC33_SRAM_SIZE_ADDR, write_size - 1);
		if (ret) {
			hwlog_err("check_ram: Update read size failed\n");
			return -EIO;
		}
		/* step4: issue patch_rd command */
		ret = stwlc33_write_byte(di, STWLC33_ACT_CMD_ADDR, STWLC33_READ_CMD_VALUE);
		if (ret) {
			hwlog_err("check_ram: Issue patch_rd cmd failed\n");
			return ret;
		}
		/* step5: confirm previous commands completed */
		if (stwlc33_fw_check_cmd_status(di))
			return -EINVAL;
		/* step6: read data from buffer and check */
		ret = stwlc33_read_block(di, fw_sram_info->fw_sram_update_addr, bufs, write_size);
		if (ret) {
			hwlog_err("check_ram: read SRAM failed\n");
			return ret;
		}
		ret = stwlc33_fw_check_sram_data(bufs, fw_sram_info->fw_sram + offset, write_size);
		if (ret) {
			hwlog_err("check_ram: check_fw_sram_data failed\n");
			return ret;
		}
		i -= write_size;
		offset += write_size;
	}

	hwlog_info("[check_ram] success\n");
	return 0;
}

static int stwlc33_fw_exec_ram_data(struct stwlc33_dev_info *di)
{
	int ret;
	unsigned char magic_number[STWLC33_OFFSET_VALUE_SIZE] = { 0x01, 0x10, 0x00, 0x20 };

	/* step1: check for zero, to confirm previous commands completed */
	if (stwlc33_fw_check_cmd_status(di))
		return -EINVAL;
	/* step2: write 4-byte magic number */
	ret = stwlc33_write_block(di, STWLC33_OFFSET_REG_ADDR,
		magic_number, STWLC33_OFFSET_VALUE_SIZE);
	if (ret) {
		hwlog_err("exec_ram: write magic number failed\n");
		return ret;
	}
	/* step3: issue patch_exec command */
	ret += stwlc33_write_byte(di, STWLC33_ACT_CMD_ADDR, STWLC33_EXEC_CMD_VALUE);
	if (ret) {
		hwlog_err("exec_ram: issue patch_exec cmd failed\n");
		return -EIO;
	}
	/* wait for swtiching to sram running */
	(void)power_msleep(STWLC33_SRAM_EXEC_TIME, 0, NULL);
	/* step4: check for zero, to confirm patch execution */
	if (stwlc33_fw_check_cmd_status(di))
		return -EINVAL;

	hwlog_info("[exec_ram] success\n");
	return 0;
}

static int stwlc33_fw_program_sramupdate(struct stwlc33_dev_info *di,
	const struct fw_update_info *fw_sram_info)
{
	int ret;
	u16 ram_version;

	/* step1: write fw patch into sram */
	ret = stwlc33_fw_write_ram_data(di, fw_sram_info);
	if (ret) {
		hwlog_err("program_sramupdate: write ram data failed\n");
		return ret;
	}
	/* step2: read ram patch and check */
	ret = stwlc33_fw_check_ram_data(di, fw_sram_info);
	if (ret) {
		hwlog_err("program_sramupdate: check ram data failed\n");
		return ret;
	}
	/* step3: to execute ram patch */
	ret = stwlc33_fw_exec_ram_data(di);
	if (ret) {
		hwlog_err("program_sramupdate: exec ram data failed\n");
		return ret;
	}
	if (fw_sram_info->fw_sram_mode == WIRELESS_TX) {
		power_usleep(DT_USLEEP_10MS);
		(void)stwlc33_tx_enable_tx_mode(true, di);
		ret = stwlc33_fw_exec_ram_data(di);
		if (ret) {
			hwlog_err("program_sramupdate: exec ram data failed\n");
			return ret;
		}
	}
	ret = stwlc33_read_word(di, STWLC33_RAM_VER_ADDR, &ram_version);
	if (ret) {
		hwlog_err("program_sramupdate: read ram version failed\n");
		return ret;
	}

	hwlog_info("[program_sramupdate] success ram_version:0x%x\n", ram_version);
	return 0;
}

static u8 *stwlc33_fw_get_otp_version(struct stwlc33_dev_info *di)
{
	int i;
	int ret;
	static u8 fw_version[STWLC33_OTP_FW_VERSION_STRING_LEN];
	u8 buff[STWLC33_TMP_BUFF_LEN] = { 0 };
	u8 data[STWLC33_OTP_FW_VERSION_LEN] = { 0 };

	if (!di)
		return "error";

	memset(fw_version, 0, STWLC33_OTP_FW_VERSION_STRING_LEN);
	ret = stwlc33_read_block(di, STWLC33_OTP_FW_VERSION_ADDR,
		data, STWLC33_OTP_FW_VERSION_LEN);
	if (ret) {
		hwlog_err("get_otp_version: read failed\n");
		return "error";
	}
	strncat(fw_version, STWLC33_OTP_FW_HEAD, strlen(STWLC33_OTP_FW_HEAD));
	for (i = STWLC33_OTP_FW_VERSION_LEN - 1; i >= 0; i--) {
		snprintf(buff, STWLC33_TMP_BUFF_LEN, " 0x%02x", data[i]);
		strncat(fw_version, buff, strlen(buff));
	}

	hwlog_info("[get_otp_version] otp version:%s\n", fw_version);
	return fw_version;
}

int stwlc33_fw_check_fwupdate(struct stwlc33_dev_info *di, u32 fw_sram_mode)
{
	int ret;
	int i;
	u8 *otp_fw_version = NULL;
	unsigned int fw_update_size;

	if (power_cmdline_is_factory_mode() && stwlc33_fw_program_nvm(di)) {
		hwlog_err("check_fwupdate: program nvm failed\n");
		return -EINVAL;
	}

	fw_update_size = ARRAY_SIZE(stwlc33_sram);
	otp_fw_version = stwlc33_fw_get_otp_version(di);
	if (strcmp(otp_fw_version, "error") == 0) {
		hwlog_err("check_fwupdate: get firmware version failed\n");
		return -EINVAL;
	}

	for (i = 0; i < fw_update_size; i++) {
		if ((fw_sram_mode == stwlc33_sram[i].fw_sram_mode) &&
			(strcmp(otp_fw_version, stwlc33_sram[i].name_fw_update_from) >= 0) &&
			(strcmp(otp_fw_version, stwlc33_sram[i].name_fw_update_to) <= 0)) {
			hwlog_info("[check_fwupdate] SRAM update start, otp_fw_version = %s\n",
				otp_fw_version);
			ret = stwlc33_fw_program_sramupdate(di, &stwlc33_sram[i]);
			if (ret) {
				hwlog_err("check_fwupdate: SRAM update failed\n");
				return ret;
			}
			otp_fw_version = stwlc33_fw_get_otp_version(di);
			hwlog_info("[check_fwupdate] SRAM update succ! otp_fw_version=%s\n",
				otp_fw_version);
			return 0;
		}
	}

	hwlog_err("check_fwupdate: SRAM no need update, otp_fw_version=%s\n", otp_fw_version);
	return -EINVAL;
}

static int stwlc33_fw_program_otp(unsigned int proc_type, void *dev_data)
{
	return 0;
}

static int stwlc33_fw_get_otp_status(unsigned int *status, void *dev_data)
{
	if (!dev_data || !status)
		return -EINVAL;

	*status = WIRELESS_FW_PROGRAMED;
	return 0;
}

static int stwlc33_fw_check_otp(void *dev_data)
{
	return 0;
}

static struct wireless_fw_ops g_stwlc33_fw_ops = {
	.ic_name                = "stwlc33",
	.program_fw             = stwlc33_fw_program_otp,
	.get_fw_status          = stwlc33_fw_get_otp_status,
	.check_fw               = stwlc33_fw_check_otp,
};

int stwlc33_fw_ops_register(struct wltrx_ic_ops *ops, struct stwlc33_dev_info *di)
{
	if (!ops || !di)
		return -ENODEV;

	ops->fw_ops = kzalloc(sizeof(*(ops->fw_ops)), GFP_KERNEL);
	if (!ops->fw_ops)
		return -ENODEV;

	memcpy(ops->fw_ops, &g_stwlc33_fw_ops, sizeof(g_stwlc33_fw_ops));
	ops->fw_ops->dev_data = (void *)di;
	return wireless_fw_ops_register(ops->fw_ops, di->ic_type);
}
