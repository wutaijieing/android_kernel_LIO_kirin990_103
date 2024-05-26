/* SPDX-License-Identifier: GPL-2.0 */
/*
 * bsp_swi.c
 *
 * board command
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

#include "bsp_swi.h"
#include <linux/slab.h>
#include <linux/string.h>
#include "../platform/board.h"

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG ns3300_bsp_swi
HWLOG_REGIST();

#define CMD_DELAY_10MS     10000
#define CMD_DELAY_20MS     20000
#define CMD_DELAY_70MS     70000
#define ERROR_CODE         0xE5
#define ERROR_COUNT_ADDR   0xEA
#define STATUS_ADDR        0xE0
#define USER_PAGE_LOCK0_ADDR    0xE7
#define USER_PAGE_LOCK1_ADDR    0xE8
#define REG_STATUS              0xE9
#define LSC_LOCK_MASK           0x02
#define LSC_LOCK_VALUE          1
#define NS3300_PGLK_RTY         5
#define NS3300_LSC_LOCK_RTY     5

static uint8_t return_value = 0xFF;

bool swi_read_space(uint16_t uw_address, uint8_t ub_bytestoread, uint8_t *ubp_data)
{
	uint8_t value = 0x0;
	int i;

	if (!ubp_data) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return false;
	}

	for (i = 0; i < ub_bytestoread; i++) {
		if (!swi_send_rra_rd(uw_address + i, &value))
			return false;

		*(ubp_data + i) = value;
	}

	return true;
}

bool swi_write_user(uint16_t uw_address, uint8_t ub_bytes_to_write, uint8_t *ubp_data)
{
	int i;

	if (!ubp_data) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return false;
	}

	swi_send_raw_word_noirq(SWI_WRA | ((uw_address >> 8) & 0x01u), (uw_address & 0xFFu));
	for (i = 0; i < ub_bytes_to_write; i++) {
		swi_send_raw_word_noirq(SWI_WD, *(ubp_data + i));
		ns3300_udelay(CMD_DELAY_10MS);
	}

	return true;
}

bool swi_cmd_soft_reset(void)
{
	swi_write_byte(SWI_COMMAND, SWI_CMD_SFRST);
	ns3300_udelay(CMD_DELAY_10MS);
	return true;
}

bool swi_cmd_power_down(void)
{
	swi_write_byte(SWI_COMMAND, SWI_CMD_PDWN);
	ns3300_udelay(CMD_DELAY_10MS);
	return true;
}

bool swi_cmd_enable_ecc(void)
{
	swi_write_byte(SWI_COMMAND, SWI_CMD_ECCEN);

	ns3300_udelay(CMD_DELAY_70MS);

	if (!swi_send_rra_rd(SWI_COMMAND, &return_value))
		return false;

	if (return_value == ERROR_CODE) {
		swi_cmd_soft_reset();
		return false;
	}

	if (return_value)
		return false;

	return true;
}

bool swi_cmd_kill(void)
{
	uint8_t ubindex;
	uint8_t ubcode[8] = { 0xEFu, 0xCDu, 0x67u, 0x45u, 0xABu, 0x89u, 0x23u, 0x01u };

	for (ubindex = 0; ubindex < (0x07u + 1u); ubindex++)
		swi_write_byte(SWI_DATA0 + ubindex, *(ubcode + ubindex));

	swi_write_byte(SWI_COMMAND, SWI_CMD_KILL);

	ns3300_udelay(CMD_DELAY_10MS);

	if (!swi_send_rra_rd(SWI_COMMAND, &return_value))
		return false;

	if (return_value == ERROR_CODE) {
		swi_cmd_soft_reset();
		return false;
	}

	if (return_value)
		return false;

	return true;
}

bool swi_cmd_init_cnt(uint8_t *ubp_data)
{
	uint8_t ubindex;

	if (!ubp_data) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return false;
	}

	for (ubindex = 0; ubindex < (0x02u + 1u); ubindex++) {
		swi_write_byte(SWI_DATA0 + ubindex, *(ubp_data + ubindex));
		ns3300_udelay(CMD_DELAY_10MS);
	}

	for (ubindex = 0; ubindex < (0x02u + 1u); ubindex++) {
		swi_write_byte(SWI_DATA4 + ubindex, ~(*(ubp_data + ubindex)));
		ns3300_udelay(CMD_DELAY_10MS);
	}

	swi_write_byte(SWI_COMMAND, SWI_CMD_CNTINIT);
	ns3300_udelay(CMD_DELAY_10MS);

	if (!swi_send_rra_rd(SWI_COMMAND, &return_value))
		return false;

	if (return_value == ERROR_CODE) {
		swi_cmd_soft_reset();
		return false;
	}

	if (return_value)
		return false;

	return true;
}

bool swi_cmd_read_lock_cnt(uint8_t *value)
{
	if (!value) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return false;
	}

	return swi_send_rra_rd(REG_STATUS, value);
}

bool swi_cmd_lock_cnt(void)
{
	int retry;
	uint8_t value = 0;

	for (retry = 0; retry < NS3300_LSC_LOCK_RTY; retry++) {
		swi_write_byte(SWI_COMMAND, SWI_CMD_CNTLOCK);
		ns3300_udelay(CMD_DELAY_20MS);

		if (!swi_send_rra_rd(SWI_COMMAND, &return_value))
			continue;
		if (return_value == ERROR_CODE) {
			swi_cmd_soft_reset();
			continue;
		}
		if (return_value)
			continue;

		(void)swi_cmd_read_lock_cnt(&value);
		if (value & LSC_LOCK_MASK)
			return true;
		hwlog_err("%s value:0x%x\n", __func__, value);
	}

	hwlog_err("%s ret:0x%x\n", __func__, return_value);
	return false;
}

bool swi_cmd_count_add_one(void)
{
	swi_write_byte(SWI_COMMAND, SWI_CMD_CNTADD);

	ns3300_udelay(CMD_DELAY_10MS);

	if (!swi_send_rra_rd(SWI_COMMAND, &return_value))
		return false;

	if (return_value == ERROR_CODE) {
		swi_cmd_soft_reset();
		return false;
	}

	if (return_value)
		return false;

	return true;
}

bool swi_cmd_count_sub_one(void)
{
	swi_write_byte(SWI_COMMAND, SWI_CMD_CNTSUB);

	ns3300_udelay(CMD_DELAY_10MS);

	if (!swi_send_rra_rd(SWI_COMMAND, &return_value))
		return false;

	if (return_value == ERROR_CODE) {
		swi_cmd_soft_reset();
		return false;
	}

	if (return_value)
		return false;

	return true;
}

bool swi_cmd_enble_ecc_cnt(void)
{
	swi_write_byte(SWI_COMMAND, SWI_CMD_ECNTEN);

	ns3300_udelay(CMD_DELAY_10MS);

	if (!swi_send_rra_rd(SWI_COMMAND, &return_value))
		return false;

	if (return_value == ERROR_CODE) {
		swi_cmd_soft_reset();
		return false;
	}

	if (return_value)
		return false;

	return true;
}

bool swi_cmd_init_ecc_count(uint8_t *ubp_data)
{
	uint8_t ubindex;

	if (!ubp_data) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return false;
	}

	for (ubindex = 0; ubindex < (0x02u + 1u); ubindex++) {
		swi_write_byte(SWI_DATA0 + ubindex, *(ubp_data + ubindex));
		ns3300_udelay(CMD_DELAY_10MS);
	}

	for (ubindex = 0; ubindex < (0x02u + 1u); ubindex++) {
		swi_write_byte(SWI_DATA4 + ubindex, ~(*(ubp_data + ubindex)));
		ns3300_udelay(CMD_DELAY_10MS);
	}

	swi_write_byte(SWI_COMMAND, SWI_CMD_ECNTINIT);
	ns3300_udelay(CMD_DELAY_10MS);

	if (!swi_send_rra_rd(SWI_COMMAND, &return_value))
		return false;

	if (return_value == ERROR_CODE) {
		swi_cmd_soft_reset();
		return false;
	}

	if (return_value)
		return false;

	return true;
}

bool swi_cmd_lock_ecc_count(void)
{
	swi_write_byte(SWI_COMMAND, SWI_CMD_ECNTLOCK);

	ns3300_udelay(CMD_DELAY_10MS);

	if (!swi_send_rra_rd(SWI_COMMAND, &return_value))
		return false;

	if (return_value == ERROR_CODE) {
		swi_cmd_soft_reset();
		return false;
	}

	if (return_value)
		return false;

	return true;
}

bool swi_cmd_lock_user(uint8_t *ubp_data, unsigned int len)
{
	uint8_t value0 = 0;
	uint8_t value1 = 0;
	int retry;

	if (!ubp_data || (len == 0)) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return false;
	}

	for (retry = 0; retry < NS3300_PGLK_RTY; retry++) {
		swi_write_byte(SWI_DATA0, *(ubp_data));
		swi_write_byte(SWI_DATA1, *(ubp_data + 1));
		swi_write_byte(SWI_COMMAND, SWI_CMD_USERLOCK);

		ns3300_udelay(CMD_DELAY_20MS);

		if (!swi_send_rra_rd(SWI_COMMAND, &return_value))
			continue;

		if (return_value == ERROR_CODE) {
			swi_cmd_soft_reset();
			continue;
		}

		if (return_value)
			continue;

		if (swi_send_rra_rd(USER_PAGE_LOCK0_ADDR, &value0) &&
			swi_send_rra_rd(USER_PAGE_LOCK1_ADDR, &value1) &&
			(value0 == *(ubp_data)) &&
			(value1 == *(ubp_data + 1)))
			return true;
	}

	hwlog_err("%s: value0:0x%x value1:0x%x ret:0x%x\n",
		__func__, value0, value1, return_value);
	return false;
}

bool swi_cmd_read_status(uint8_t *ubp_data)
{
	uint8_t sta[32] = { 0 };
	uint8_t j;

	if (!ubp_data) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return false;
	}

	for (j = 0x0; j <= 31; j++) { /* read 0xe1~0xea */
		ns3300_udelay(100); /* 100us */
		if (!swi_send_rra_rd(STATUS_ADDR + j, &sta[j]))
			return false;
	}

	memcpy(ubp_data, &sta[1], 10); /* memcpy 0xe1~0xea */
	return true;
}

bool swi_cmd_read_swi_error_counter(uint8_t *ubp_data)
{
	if (!ubp_data) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return false;
	}

	if (swi_send_rra_rd(ERROR_COUNT_ADDR, ubp_data))
		return true;

	return false;
}

bool swi_cmd_read_page_lock(uint8_t *status, int lock_num)
{
	if (!status) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return false;
	}

	if (!lock_num)
		return swi_send_rra_rd(USER_PAGE_LOCK0_ADDR, status);
	else
		return swi_send_rra_rd(USER_PAGE_LOCK1_ADDR, status);
}

