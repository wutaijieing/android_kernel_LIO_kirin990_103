/*! \file nta5332.c
 * \brief  nta5332 Driver
 *
 * Driver for the nta5332
 * Copyright (c) 2011 Nxp Corp
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include "nfc_kit.h"
#include "securec.h"

#define NTA5332_CHIP_NAME         "nxp,nta5332"
#define NTA5332_I2C_RETRY_TIMES   3

struct nfc_device_data nta5332_data = {0x0};

static int g_nta5332_i2c_dly1 = 4;
static int g_nta5332_i2c_dly2 = 500;
static int g_nta5332_i2c_dly3 = 5000;
static char  g_nta5332_ndef[256] = {0x0};
static size_t g_nta5332_ndef_len = 0x0;

void nta5332_i2c_delay_set(int ms_dly1, int ms_dly2, int ms_dly3)
{
	g_nta5332_i2c_dly1 = ms_dly1;
	g_nta5332_i2c_dly2 = ms_dly2;
	g_nta5332_i2c_dly3 = ms_dly3;
}

static int nta5332_i2c_block_write(u16 block_addr, u32 value)
{
	int ret;
	int i;
	struct nfc_kit_data *pdata = NULL;
	struct i2c_client *i2c = NULL;
	char txb[6] = {0};

	pdata = get_nfc_kit_data();
	if (pdata == NULL) {
		NFC_ERR("%s: pdata is NULL\n", __func__);
		return NFC_FAIL;
	}

	i2c = pdata->client;
	txb[0] = (char)((block_addr >> 8) & 0xff);
	txb[1] = (char)(block_addr & 0xff);
	txb[2] = (char)(value & 0xff);
	txb[3] = (char)((value >> 8) & 0xff);
	txb[4] = (char)((value >> 16) & 0xff);
	txb[5] = (char)((value >> 24) & 0xff);

	for (i = 0; i < NTA5332_I2C_RETRY_TIMES; i++) {
		ret = i2c_master_send(i2c, txb, 6);
		if (ret == 6) {
			break;
		}
	}
	if (ret != 6) {
		NFC_ERR("[block_write] send address: 0x%x error, ret = %d\n", block_addr, ret);
		return NFC_FAIL;
	}

	NFC_INFO("[block_write] txbuf: %02x, %02x, %02x, %02x, %02x, %02x\n", txb[0], txb[1], txb[2], txb[3], txb[4], txb[5]);
	NFC_INFO("[block_write] block_addr = 0x%x value = 0x%x\n", block_addr, value);

	return NFC_OK;
}

static int nta5332_i2c_block_read(u16 block_addr, u32 *value)
{
	int ret;
	int i;
	struct nfc_kit_data *pdata = NULL;
	struct i2c_client *i2c = NULL;
	char txb[2] = {0};
	char rxb[4] = {0};

	pdata = get_nfc_kit_data();
	if (pdata == NULL) {
		NFC_ERR("%s: pdata is NULL\n", __func__);
		return NFC_FAIL;
	}

	i2c = pdata->client;

	txb[0] = (char)((block_addr >> 8) & 0xff);
	txb[1] = (char)(block_addr & 0xff);

	/* nta5332有超时机制，send与recv之间不能打印，延时 */
	for (i = 0; i < NTA5332_I2C_RETRY_TIMES; i++) {
		ret = i2c_master_send(i2c, txb, 2);
		if (ret == 2) {
			break;
		}
	}
	if (ret != 2) {
		NFC_ERR("[block_read] send address: 0x%x error, ret = %d\n", block_addr, ret);
		return NFC_FAIL;
	}
	for (i = 0; i < NTA5332_I2C_RETRY_TIMES; i++) {
		ret = i2c_master_recv(i2c, rxb, 4);
		if (ret == 4) {
			*value = ((u32)rxb[3] << 24) | ((u32)rxb[2] << 16) | ((u32)rxb[1] << 8) | (u32)rxb[0];
			break;
		}
	}
	if (ret != 4) {
		NFC_ERR("[block_read] recv address: 0x%x error, ret = %d\n", block_addr, ret);
		return NFC_FAIL;
	}

	NFC_INFO("[block_read] txbuf: %02x, %02x\n", txb[0], txb[1]);
	NFC_INFO("[block_read] rxbuf: %02x, %02x, %02x, %02x\n", rxb[0], rxb[1], rxb[2], rxb[3]);
	NFC_INFO("[block_read] block_addr = 0x%x value = 0x%x\n", block_addr, *value);

	return NFC_OK;
}

static int nta5332_i2c_block_check(u16 block_addr, u32 mask, u32 expect_val)
{
	int ret;
	u32 actual_val = 0x0;

	ret = nta5332_i2c_block_read(block_addr, &actual_val);
	if (ret != NFC_OK) {
		return ret;
	}

	if ((actual_val & mask) != expect_val) {
		return NFC_FAIL;
	}
	return NFC_OK;
}

static int nta5332_i2c_session_write(u16 block_addr, u8 byte_addr, u8 mask, u8 data)
{
	int ret;
	int i;
	struct nfc_kit_data *pdata = NULL;
	struct i2c_client *i2c = NULL;
	char txb[5] = {0};

	pdata = get_nfc_kit_data();
	if (pdata == NULL) {
		NFC_ERR("%s: pdata is NULL\n", __func__);
		return NFC_FAIL;
	}

	i2c = pdata->client;

	txb[0] = (char)((block_addr >> 8) & 0xff);
	txb[1] = (char)(block_addr & 0xff);
	txb[2] = (char)(byte_addr);
	txb[3] = (char)(mask);
	txb[4] = (char)(data);

	for (i = 0; i < NTA5332_I2C_RETRY_TIMES; i++) {
		ret = i2c_master_send(i2c, txb, 5);
		if (ret == 5) {
			break;
		}
	}
	if (ret != 5) {
		NFC_ERR("[session_write] send address: 0x%x error, ret = %d\n", block_addr, ret);
		return NFC_FAIL;
	}

	NFC_INFO("[session_write] txbuf: %02x, %02x, %02x, %02x, %02x\n", txb[0], txb[1], txb[2], txb[3], txb[4]);
	NFC_INFO("[session_write] block_addr = 0x%x byte = %u value = 0x%x\n", block_addr, byte_addr, data);

	return NFC_OK;
}

static int nta5332_i2c_session_read(u16 block_addr, u8 byte_addr, u8* data)
{
	int ret;
	int i;
	struct nfc_kit_data *pdata = NULL;
	struct i2c_client *i2c = NULL;
	char txb[3] = {0};
	char rxb[1] = {0};

	pdata = get_nfc_kit_data();
	if (pdata == NULL) {
		NFC_ERR("%s: pdata is NULL\n", __func__);
		return NFC_FAIL;
	}

	i2c = pdata->client;

	txb[0] = (char)((block_addr >> 8) & 0xff);
	txb[1] = (char)(block_addr & 0xff);
	txb[2] = byte_addr;

	/* nta5332有超时机制，send与recv之间不能打印，延时 */
	for (i = 0; i < NTA5332_I2C_RETRY_TIMES; i++) {
		ret = i2c_master_send(i2c, txb, 3);
		if (ret == 3) {
			break;
		}
	}
	if (ret != 3) {
		NFC_ERR("[session_read] send address: 0x%x error, ret = %d\n", block_addr, ret);
		return NFC_FAIL;
	}
	for (i = 0; i < NTA5332_I2C_RETRY_TIMES; i++) {
		ret = i2c_master_recv(i2c, rxb, 1);
		if (ret == 1) {
			*data = rxb[0];
			break;
		}
	}
	if (ret != 1) {
		NFC_ERR("[session_read] recv address: 0x%x error, ret = %d\n", block_addr, ret);
		return NFC_FAIL;
	}

	NFC_INFO("[session_read] txbuf: %02x, %02x, %02x\n", txb[0], txb[1], txb[2]);
	NFC_INFO("[session_read] rxbuf: %02x\n", rxb[0]);
	NFC_INFO("[session_read] block_addr = 0x%x byte = %u value = 0x%x\n", block_addr, byte_addr, *data);

	return NFC_OK;
}

static int nta5332_i2c_session_check(u16 block_addr, u8 byte_addr, u8 mask, u8 expect_val)
{
	int ret;
	u8 actual_val = 0x0;

	ret = nta5332_i2c_session_read(block_addr, byte_addr, &actual_val);
	if (ret != NFC_OK) {
		return ret;
	}

	if ((actual_val & mask) != expect_val) {
		return NFC_FAIL;
	}
	return NFC_OK;
}

/* NTA5332 block空间地址 */
#define CONFIG_BLK_ADDR             0x1037
#define SYNC_DATA_BLOCK_BLK_ADDR    0x1038
#define EH_ED_CFG_BLK_ADDR          0x103D
#define I2C_SLAVE_CFG_BLK_ADDR      0x103E
#define NFC_LOCK_0_BLK_ADDR         0x1092
#define NFC_LOCK_1_BLK_ADDR         0x1093
#define SRAM_BLK_ADDR               0x2000

/* NTA5332 session空间地址 */
#define CONFIG_SSN_ADDR             0x10a1
#define I2C_SLAVE_CFG_SSN_ADDR      0x10A9

static int nta5332_enable(void)
{
	int ret = NFC_OK;
	u8 data = 0x0;

	// enable NFC interface
	if (nta5332_data.dts.sram_enable) {
		ret = nta5332_i2c_session_read(CONFIG_SSN_ADDR, 0, &data);
		ret |= nta5332_i2c_session_write(CONFIG_SSN_ADDR, 0, 0xFF, (data & 0xDF));
		ret |= nta5332_i2c_session_check(CONFIG_SSN_ADDR, 0, 0x20, 0);
		if (ret != NFC_OK) {
			NFC_ERR("%s: enable NFC interface fail, ret:%d\n", __func__, ret);
			return NFC_FAIL;
		}
	} else {
		NFC_ERR("%s: enable NFC interface not support, ret:%d\n", __func__, ret);
		return NFC_FAIL;
	}

	return ret;
}

static int nta5332_disable(void)
{
	int ret = NFC_OK;
	u8 data = 0x0;

	// disable NFC interface
	if (nta5332_data.dts.sram_enable) {
		ret = nta5332_i2c_session_read(CONFIG_SSN_ADDR, 0, &data);
		ret |= nta5332_i2c_session_write(CONFIG_SSN_ADDR, 0, 0xFF, (data | 0x20));
		ret |= nta5332_i2c_session_check(CONFIG_SSN_ADDR, 0, 0x20, 0x20);
		if (ret != NFC_OK) {
			NFC_ERR("%s: disable NFC interface fail, ret:%d\n", __func__, ret);
			return NFC_FAIL;
		}
	} else {
		NFC_ERR("%s: disable NFC interface not support, ret:%d\n", __func__, ret);
		return NFC_FAIL;
	}

	return ret;
}

static int nta5332_sram_enable(bool *flag)
{
	int ret;
	u32 value;

	// enable SRAM access, can only be done by config block -> need repower
	ret = nta5332_i2c_block_read(CONFIG_BLK_ADDR, &value);
	if (ret != NFC_OK) {
		return ret;
	}
	if ((value & 0x200) != 0) {
		*flag = false;
		return ret;
	}
	*flag = true;
	// bit1 of Config_1: 0-SRAM NOT enabled; 1-SRAM enabled
	value |= 0x0200;
	ret = nta5332_i2c_block_write(CONFIG_BLK_ADDR, value);
	if (ret != NFC_OK) {
		return ret;
	}
	msleep(5);

	return nta5332_i2c_block_check(CONFIG_BLK_ADDR, 0x200, 0x200);
}

static int nta5332_ed_pin_cfg(bool *flag)
{
	int ret;
	u32 value;

	// config ED to be triggered upon RF detected
	ret = nta5332_i2c_block_read(EH_ED_CFG_BLK_ADDR, &value);
	if (ret != NFC_OK) {
		return ret;
	}
	if ((value & 0x00ff0000) == 0x010000) {
		*flag = false;
		return ret;
	}
	*flag = true;
	value &= 0xff00ffff;
	value |= 0x010000;
	ret = nta5332_i2c_block_write(EH_ED_CFG_BLK_ADDR, value);
	if (ret != NFC_OK) {
		return ret;
	}
	msleep(5);

	return nta5332_i2c_block_check(EH_ED_CFG_BLK_ADDR, 0x00ff0000, 0x010000);
}

static int nta5332_sync_data_block_set(bool flag)
{
	int ret;
	u32 value;

	if (flag == false) {
		return NFC_OK;
	}

	// Set SYNCH_DATA_BLOCK to 0-> after reading block0, ED pin will be triggered
	ret = nta5332_i2c_block_read(SYNC_DATA_BLOCK_BLK_ADDR, &value);
	if (ret != NFC_OK) {
		return ret;
	}
	value &= 0xffff0000;
	ret = nta5332_i2c_block_write(SYNC_DATA_BLOCK_BLK_ADDR, value);
	if (ret != NFC_OK) {
		return ret;
	}
	msleep(5);

	return nta5332_i2c_block_check(SYNC_DATA_BLOCK_BLK_ADDR, 0x0000ffff, 0x0);
}

static int nta5332_nfc_lock_set(bool *flag)
{
	int ret;
	u32 lock0;
	u32 lock1;

	// 禁止从nfc接口写芯片
	ret = nta5332_i2c_block_read(NFC_LOCK_0_BLK_ADDR, &lock0);
	ret |= nta5332_i2c_block_read(NFC_LOCK_1_BLK_ADDR, &lock1);
	if (ret != NFC_OK) {
		return ret;
	}
	if ((lock0 == 0xff) && ((lock1 & 0x80) == 0x80)) {
		*flag = false;
		return ret;
	}
	*flag = true;

	lock0 = 0xff;
	ret = nta5332_i2c_block_write(NFC_LOCK_0_BLK_ADDR, lock0);
	if (ret != NFC_OK) {
		return ret;
	}
	msleep(5);
	ret = nta5332_i2c_block_check(NFC_LOCK_0_BLK_ADDR, lock0, lock0);
	if (ret != NFC_OK) {
		return ret;
	}

	lock1 |= 0x80;
	ret = nta5332_i2c_block_write(NFC_LOCK_1_BLK_ADDR, lock1);
	if (ret != NFC_OK) {
		return ret;
	}
	msleep(5);
	ret = nta5332_i2c_block_check(NFC_LOCK_1_BLK_ADDR, lock1, lock1);
	if (ret != NFC_OK) {
		return ret;
	}

	return ret;
}

static int nta5332_chip_init(void)
{
	int ret;
	bool flag = false;
	bool sram_flag = false;
	bool ed_flag = false;
	bool lock_flag = false;

	/* 1.vcc上电 --由硬件完成 */

	/* 2.hpd拉低 --由硬件完成 */

	/* ---------------- below configurations run ONCE-only and need repower -------------*/
	if (nta5332_data.dts.sram_enable == 0x1) {
		ret = nta5332_sram_enable(&sram_flag);
		if (ret != NFC_OK) {
			return ret;
		}
	}

	if (nta5332_data.dts.event_detect == 0x1) {
		ret = nta5332_ed_pin_cfg(&ed_flag);
		ret |= nta5332_sync_data_block_set(ed_flag);
		if (ret != NFC_OK) {
			return ret;
		}
	}

	ret = nta5332_nfc_lock_set(&lock_flag);
	if (ret != NFC_OK) {
		return ret;
	}

	flag = sram_flag | ed_flag | lock_flag;
	if ((flag == true) && (nta5332_data.dts.vcc_vld)) {
		gpio_set_value(nta5332_data.dts.vcc_gpio, 0);
		msleep(50);
		gpio_set_value(nta5332_data.dts.vcc_gpio, 1);
		msleep(50);
	}
	/* ----------------------------------------------------------------------------------*/

	NFC_INFO("%s: finished\n", __func__);
	return ret;
}

static int nta5332_irq_phase1(void)
{
	int ret;
	struct nfc_kit_data *pdata = NULL;

	pdata = get_nfc_kit_data();
	if (pdata == NULL) {
		NFC_ERR("%s: pdata is NULL\n", __func__);
		return NFC_FAIL;
	}

	if (gpio_get_value(pdata->irq_gpio)) {
		NFC_ERR("%s: NFC field absent\n", __func__);
		return NFC_FAIL;
	}

	ret = nta5332_disable();

	return ret;
}

static int nta5332_sram_mirror_mode_set(bool enable)
{
	int ret;
	u8 data = 0x0;
	u8 arbiter_mode = (enable == true) ? 0x4 : 0x0;

	if (nta5332_data.dts.sram_enable == 0x0) {
		return NFC_OK;
	}
	// enable or disable SRAM Mirror mode
	ret = nta5332_i2c_session_read(CONFIG_SSN_ADDR, 1, &data);
	ret |= nta5332_i2c_session_write(CONFIG_SSN_ADDR, 1, 0xFF, ((data & 0xF3) | arbiter_mode));
	if (ret != NFC_OK) {
		NFC_ERR("%s: enable:%x, ret:%d\n", __func__, enable, ret);
		return ret;
	}

	return ret;
}

static int nta5332_ndef_set(const char *ndef, size_t len)
{
	int ret = NFC_OK;
	u32 i;
	u32 value;
	u16 base_addr = (nta5332_data.dts.sram_enable) ? SRAM_BLK_ADDR : 0x0; /* EEPROM */
	u16 addr;

	if (len % 4 != 0) {
		NFC_ERR("%s: len %zu error\n", __func__, len);
		return NFC_FAIL;
	}
	/* 第0~255B根据sram_enable，选择配置sram还是配置eeprom;
	   从第256B开始，只能配置eeprom */
	for (i = 0; i < len / 4; i++) {
		value = (u32)ndef[i * 4] | ((u32)ndef[i * 4 + 1] << 8) |
			((u32)ndef[i * 4 + 2] << 16) | ((u32)ndef[i * 4 + 3] << 24);
		addr = (i < 64) ? (base_addr + i) : i;
		ret |= nta5332_i2c_block_write(addr, value);
	}
	if (ret != NFC_OK) {
		NFC_ERR("%s: fail, ret:%d\n", __func__, ret);
		return ret;
	}
	return ret;
}

static int nta5332_irq_phase2(void)
{
	int ret;

	ret = nta5332_ndef_set(g_nta5332_ndef, g_nta5332_ndef_len);
	if (ret != NFC_OK) {
		return ret;
	}

	// enable SRAM Mirror mode
	ret = nta5332_sram_mirror_mode_set(true);
	if (ret != NFC_OK) {
		return ret;
	}

	// enable NFC interface
	ret = nta5332_enable();
	if (ret != NFC_OK) {
		return ret;
	}

	return ret;
}

int nta5332_i2c_test1(void)
{
	int ret;
	u32 value;
	u8  data;

	ret = nta5332_i2c_block_read(I2C_SLAVE_CFG_BLK_ADDR, &value);
	if (nta5332_data.dts.sram_enable) {
		ret |= nta5332_i2c_session_read(I2C_SLAVE_CFG_SSN_ADDR, 0, &data);
		ret |= nta5332_i2c_session_read(I2C_SLAVE_CFG_SSN_ADDR, 1, &data);
		ret |= nta5332_i2c_session_read(I2C_SLAVE_CFG_SSN_ADDR, 2, &data);
		ret |= nta5332_i2c_session_read(I2C_SLAVE_CFG_SSN_ADDR, 3, &data);
	}
	ret |= nta5332_i2c_block_read(CONFIG_BLK_ADDR, &value);
	ret |= nta5332_i2c_block_read(EH_ED_CFG_BLK_ADDR, &value);
	ret |= nta5332_i2c_block_read(SYNC_DATA_BLOCK_BLK_ADDR, &value);
	if (ret != NFC_OK) {
		NFC_ERR("%s: nfc i2c read test fail, ret:%d\n", __func__, ret);
		return NFC_FAIL;
	}

	return ret;
}

int nta5332_i2c_test2(void)
{
	int ret = NFC_OK;
	u32 value;
	u8  data;

	if (nta5332_data.dts.sram_enable) {
		ret = nta5332_i2c_session_read(I2C_SLAVE_CFG_SSN_ADDR, 0, &data);
		ret |= nta5332_i2c_session_read(I2C_SLAVE_CFG_SSN_ADDR, 1, &data);
		ret |= nta5332_i2c_session_read(I2C_SLAVE_CFG_SSN_ADDR, 2, &data);
		ret |= nta5332_i2c_session_read(I2C_SLAVE_CFG_SSN_ADDR, 3, &data);
	}
	ret |= nta5332_i2c_block_read(I2C_SLAVE_CFG_BLK_ADDR, &value);
	ret |= nta5332_i2c_block_read(CONFIG_BLK_ADDR, &value);
	ret |= nta5332_i2c_block_read(EH_ED_CFG_BLK_ADDR, &value);
	ret |= nta5332_i2c_block_read(SYNC_DATA_BLOCK_BLK_ADDR, &value);
	if (ret != NFC_OK) {
		NFC_ERR("%s: nfc i2c read test fail, ret:%d\n", __func__, ret);
		return NFC_FAIL;
	}

	return ret;
}

int nta5332_i2c_session_test(int times)
{
	int ret = NFC_OK;
	u8  data;
	int i;

	if (nta5332_data.dts.sram_enable == 0x0) {
		return NFC_OK;
	}
	for (i = 0; i < times; i++) {
		ret |= nta5332_i2c_session_read(I2C_SLAVE_CFG_SSN_ADDR, 0, &data);
		ret |= nta5332_i2c_session_read(I2C_SLAVE_CFG_SSN_ADDR, 1, &data);
		ret |= nta5332_i2c_session_read(I2C_SLAVE_CFG_SSN_ADDR, 2, &data);
		ret |= nta5332_i2c_session_read(I2C_SLAVE_CFG_SSN_ADDR, 3, &data);
	}
	if (ret != NFC_OK) {
		NFC_ERR("%s: nfc i2c read test fail, ret:%d\n", __func__, ret);
		return NFC_FAIL;
	}

	return ret;
}

int nta5332_i2c_block_test(int times)
{
	int ret = NFC_OK;
	u32 value;
	int i;

	for (i = 0; i < times; i++) {
		ret |= nta5332_i2c_block_read(I2C_SLAVE_CFG_BLK_ADDR, &value);
		ret |= nta5332_i2c_block_read(CONFIG_BLK_ADDR, &value);
		ret |= nta5332_i2c_block_read(EH_ED_CFG_BLK_ADDR, &value);
		ret |= nta5332_i2c_block_read(SYNC_DATA_BLOCK_BLK_ADDR, &value);
	}
	if (ret != NFC_OK) {
		NFC_ERR("%s: nfc i2c read test fail, ret:%d\n", __func__, ret);
		return NFC_FAIL;
	}

	return ret;
}

int nta5332_sram_test(int times)
{
	int ret = NFC_OK;
	u32 value;
	struct nfc_kit_data *pdata = NULL;
	u8 ndef[6][4] = {
		{0xE1, 0x40, 0x80, 0x01},
		{0x03, 0x0C, 0xD1, 0x01},
		{0x08, 0x54, 0x02, 0x65},         // 08: payload length, 0x54:Text, 02: language length, 0x65:'e'
		{0x6E, 0x68, 0x65, 0x6C},         // 'n' 'h' 'e' 'l'
		{0x6C, 0x6F, 0x00, 0x00},         // 'l' 'o'
		{0xFE, 0x00, 0x00, 0x00}
	};
	u8 buf[6] = {0x0};

	int i;
	int cnt = 0;

	pdata = get_nfc_kit_data();
	if (pdata == NULL) {
		NFC_ERR("%s: pdata is NULL\n", __func__);
		return NFC_FAIL;
	}
	while (cnt++ < times) {
		while (gpio_get_value(pdata->irq_gpio)) {
			if (g_nta5332_i2c_dly1 <= 65536) {
				mdelay(g_nta5332_i2c_dly1);
			} else {
				udelay(g_nta5332_i2c_dly1 - 65536);
			}
		}

		// disable NFC interface
		ret = nta5332_disable();
		if (ret != NFC_OK) {
			return ret;
		}

		msleep(g_nta5332_i2c_dly2);

	    for (i = 0; i < 6; i++) {
			value = (u32)ndef[i][0] | ((u32)ndef[i][1] << 8) |
				((u32)ndef[i][2] << 16) | ((u32)ndef[i][3] << 24);
			ret |= nta5332_i2c_block_write((0x2000 + i), value);
		}
		if (ret != NFC_OK) {
			NFC_ERR("%s: write sram fail, ret:%d\n", __func__, ret);
			return NFC_FAIL;
		}

		// enable SRAM Mirror mode
		ret = nta5332_sram_mirror_mode_set(true);
		if (ret != NFC_OK) {
			return ret;
		}

		// enable NFC interface
		ret = nta5332_enable();
		if (ret != NFC_OK) {
			return ret;
		}

		ret = snprintf_s(buf, sizeof(buf), sizeof(buf) - 1, "%05d", (cnt % 100000));
		if (ret < 0) {
			return NFC_FAIL;
		}
		ndef[3][1] = buf[0];
		ndef[3][2] = buf[1];
		ndef[3][3] = buf[2];
		ndef[4][0] = buf[3];
		ndef[4][1] = buf[4];

		msleep(g_nta5332_i2c_dly3);
	}

	return ret;
}

static int nta5332_parse_dts(struct nfc_kit_data *pdata, struct nfc_device_dts *dts)
{
	int ret;
	bool found = false;
	struct device_node *node = NULL;
	u32 slave_addr = 0;

	/* 查询nta5332芯片设备树节点信息 */
	for_each_child_of_node(pdata->client->dev.of_node, node) {
		if (of_device_is_compatible(node, NTA5332_CHIP_NAME)) {
			found = true;
			break;
		}
	}
	if (!found) {
		NFC_ERR("%s:%s node not found\n", __func__, NTA5332_CHIP_NAME);
		return NFC_FAIL;
	}

	/* 获取芯片的slave_address */
	ret = nfc_of_read_u32(node, "slave_address", &slave_addr);
	if (ret != NFC_OK) {
		return NFC_FAIL;
	}
	pdata->client->addr = slave_addr;

	/* 获取芯片的控制信息 */
	ret = nfc_of_read_u32(node, "vcc_vld", &dts->vcc_vld);
	ret |= nfc_of_read_u32(node, "vcc_gpio", &dts->vcc_gpio);
	ret |= nfc_of_read_u32(node, "hpd_vld", &dts->hpd_vld);
	ret |= nfc_of_read_u32(node, "hpd_gpio", &dts->hpd_gpio);
	ret |= nfc_of_read_u32(node, "sram_enable", &dts->sram_enable);
	ret |= nfc_of_read_u32(node, "event_detect", &dts->event_detect);
	if (ret != NFC_OK) {
		return NFC_FAIL;
	}

	return NFC_OK;
}

static int nta5332_ndef_store(const char *buf, size_t count)
{
	u8 ndef[24] = {
					0xE1, 0x40, 0x80, 0x01, 
					0x03, 0x0C, 0xD1, 0x01,
					0x08, 0x54, 0x02, 0x65,         // 08: payload length, 0x54:Text, 02: language length, 0x65:'e'
					0x6E, 0x68, 0x65, 0x6C,         // 'n' 'h' 'e' 'l'
					0x6C, 0x6F, 0x00, 0x00,         // 'l' 'o'
					0xFE, 0x00, 0x00, 0x00
					};
	int ret;

	if (buf == NULL) {
		NFC_ERR("%s: buf is NULL\n", __func__);
		return NFC_FAIL;
	}
	ret = memcpy_s(g_nta5332_ndef, sizeof(g_nta5332_ndef), ndef, sizeof(ndef));
	if (ret != EOK) {
		NFC_ERR("%s: memcpy_s failed\n", __func__);
		return NFC_FAIL;
	}

	ret = memcpy_s(g_nta5332_ndef + 13, sizeof(g_nta5332_ndef) - 13, buf, count);
	if (ret != EOK) {
		NFC_ERR("%s: memcpy_s failed\n", __func__);
		return NFC_FAIL;
	}
	g_nta5332_ndef_len = sizeof(ndef);

	return NFC_OK;
}

static int nta5332_ndef_show(char *buf, size_t *count)
{
	int ret;
	u32 i;

	if (buf == NULL) {
		NFC_ERR("%s: buf is NULL\n", __func__);
		return NFC_FAIL;
	}
	*count = 0;
	for (i = 0; i < g_nta5332_ndef_len; i += 4) {
		ret = snprintf_s(buf + *count, PAGE_SIZE - *count, PAGE_SIZE - *count - 1, "%02x, %02x, %02x, %02x,\n",
			g_nta5332_ndef[i], g_nta5332_ndef[i + 1], g_nta5332_ndef[i + 2], g_nta5332_ndef[i + 3]);
		if (ret < 0) {
			NFC_ERR("%s: snprintf_s failed\n", __func__);
			return NFC_FAIL;
		}
		*count += ret;
	}

	return NFC_OK;
}

struct nfc_device_ops nfc_nta5332_ops = {
	.irq_handler = nta5332_irq_phase1,
	.irq0_sch_work = nta5332_irq_phase2,
	.irq1_sch_work = NULL,
	.chip_init = nta5332_chip_init,
	.ndef_store = nta5332_ndef_store,
	.ndef_show = nta5332_ndef_show,
};

int nxp_probe(struct nfc_kit_data *pdata)
{
	int ret;

	if (pdata == NULL) {
		NFC_ERR("%s: nfc_kit_data is NULL, error\n", __func__);
		return NFC_FAIL;
	}

	NFC_INFO("%s: called\n", __func__);

	/* 检查I2C功能 */
	if (!i2c_check_functionality(pdata->client->adapter, I2C_FUNC_I2C | I2C_FUNC_SMBUS_READ_WORD_DATA)) {
		NFC_ERR("%s: i2c function check error\n", __func__);
		return NFC_FAIL;
	}

	/* 解析设备树 */
#ifdef CONFIG_OF
	ret = nta5332_parse_dts(pdata, &nta5332_data.dts);
	if (ret != NFC_OK) {
		NFC_ERR("%s: parse dts error, ret = %d\n", __func__, ret);
		return NFC_FAIL;
	}
	NFC_INFO("%s: dts parse success\n", __func__);
#else
#error: nta5332 only support device tree platform
#endif

	/* 注册芯片数据 */
	nta5332_data.ops = &nfc_nta5332_ops;
	ret = nfc_chip_register(&nta5332_data);
	if (ret != NFC_OK) {
		NFC_ERR("%s:chip register failed, ret = %d\n", __func__, ret);
		return NFC_FAIL;
	}

	NFC_INFO("%s:success\n", __func__);
	return NFC_OK;
}
EXPORT_SYMBOL(nxp_probe);
