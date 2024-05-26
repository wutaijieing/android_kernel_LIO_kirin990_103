/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ns3300_swi.c
 *
 * ns3300 swi bus
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

#include "ns3300_swi.h"
#include <huawei_platform/log/hw_log.h>
#include "../platform/board.h"

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG ns3300_swi
HWLOG_REGIST();

#define GPIO_DIR_OUT        1
#define GPIO_DIR_IN         0
#define GPIO_VALUE_HIGH     1
#define GPIO_VALUE_LOW      0
#define RETYR_TIME          5
#define CRC_LEN             8
#define TRAIN_SEQ_LEN       2
#define PAYLOAD_LEN         10
#define INVERSION_LEN       1
#define SWI_DATA_LEN        (INVERSION_LEN + PAYLOAD_LEN + TRAIN_SEQ_LEN + CRC_LEN)

static uint32_t g_ul_baud_low;               /* 1 * BIF_DEFAULT_TIMEBASE */
static uint32_t g_ul_baud_high;              /* 3 * BIF_DEFAULT_TIMEBASE */
static uint32_t g_ul_baud_stop;              /* >5 * BIF_DEFAULT_TIMEBASE */
static uint32_t g_ul_response_timeout;       /* >20 * BIF_DEFAULT_TIMEBASE */
static uint32_t g_ul_response_timeout_long;  /* >32ms for ECC completion */
static uint32_t g_ul_baud_powerdown_time;    /* >196us */
static uint32_t g_ul_baud_powerup_time;      /* >10ms */
static uint32_t g_ul_baud_reset_time;        /* >1ms */
static uint32_t g_cul_nvm_timeout;           /* 5.1ms */

void timing_init(struct ns3300_dev *di)
{
	if (!di) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return;
	}

	g_ul_baud_low = di->tau;                    /* 1 * tau */
	g_ul_baud_high = g_ul_baud_low * 3;         /* 3 * tau */
	g_ul_baud_stop = g_ul_baud_low * 5;         /* 5 * tau */
	g_ul_response_timeout = g_ul_baud_low * 10; /* 10 * tau */
	g_ul_response_timeout_long = 32000;     /* 32ms */
	g_ul_baud_powerdown_time = 10000;       /* 10ms */
	g_ul_baud_powerup_time = 50000;         /* 50ms */
	g_ul_baud_reset_time = 1000;            /* 1ms */
	g_cul_nvm_timeout = 5000;               /* 5.1ms */
}

void swi_power_down(void)
{
	set_pin_dir(GPIO_DIR_OUT, GPIO_VALUE_LOW);
	set_pin(GPIO_VALUE_LOW);
	ns3300_udelay(g_ul_baud_powerdown_time);
}

void swi_power_up(void)
{
	set_pin_dir(GPIO_DIR_OUT, GPIO_VALUE_HIGH);
	set_pin(GPIO_VALUE_HIGH);
	ns3300_udelay(g_ul_baud_powerup_time);
}

static bool swi_treat_invert_flag(uint8_t ub_code, uint8_t ub_data, uint8_t ub_crc8)
{
	uint8_t i;
	uint8_t ub_count = 0;
	uint32_t src_data;

	/* src_data = 4bit ub_code + 8bit ub_data + 8bit ub_crc */
	src_data = ((uint32_t)ub_code << 16) + (((uint32_t)ub_data) << 8) + ub_crc8;

	/* count 1-bit to ub_count */
	for (i = 0; i < 18; i++) {
		ub_count += src_data & 0x01;
		src_data >>= 1;
	}
	/* check, if invert required */
	return (ub_count > 9);
}

/* 8-bit CRC: X^8+X^2+X+1 */
static uint8_t swi_calculate_crc8(uint8_t ub_code, uint8_t ub_data)
{
	uint8_t i;
	uint8_t crc_bit[9] = { 0 };  /* crc bit: 8 */
	uint16_t src_data;

	/* src_data = 4bit ubcode + 8bit ub_data */
	src_data = ((uint16_t)ub_code << 8) + ub_data;

	crc_bit[7] = 1 ^ ((src_data >> 7) & 0x1) ^ ((src_data >> 6) & 0x1) ^
		((src_data >> 5) & 0x1);
	crc_bit[6] = 1 ^ ((src_data >> 6) & 0x1) ^ ((src_data >> 5) & 0x1) ^
		((src_data >> 4) & 0x1);
	crc_bit[5] = ((src_data >> 9) & 0x1) ^ ((src_data >> 5) & 0x1) ^
		((src_data >> 4) & 0x1) ^ ((src_data >> 3) & 0x1);
	crc_bit[4] = ((src_data >> 8) & 0x1) ^ ((src_data >> 4) & 0x1) ^
		((src_data >> 3) & 0x1) ^ ((src_data >> 2) & 0x1);
	crc_bit[3] = ((src_data >> 9) & 0x1) ^ ((src_data >> 7) & 0x1) ^
		((src_data >> 3) & 0x1) ^ ((src_data >> 2) & 0x1) ^ ((src_data >> 1) & 0x1);
	crc_bit[2] = 1 ^ ((src_data >> 8) & 0x1) ^ ((src_data >> 6) & 0x1) ^
		((src_data >> 2) & 0x1) ^ ((src_data >> 1) & 0x1) ^ ((src_data >> 0) & 0x1);
	crc_bit[1] = ((src_data >> 9) & 0x1) ^ ((src_data >> 6) & 0x1) ^
		((src_data >> 1) & 0x1) ^ ((src_data >> 0) & 0x1);
	crc_bit[0] = 1 ^ ((src_data >> 8) & 0x1) ^ ((src_data >> 7) & 0x1) ^
		((src_data >> 6) & 0x1) ^ ((src_data >> 0) & 0x1);

	crc_bit[8] = 0;

	for (i = 0; i < 8; i++)
		crc_bit[8] |= (crc_bit[i] << i);

	return crc_bit[8];
}

static void swi_do_send_cmd_step1(uint8_t ub_code, uint8_t ub_data)
{
	/* Send a STOP singal first to have time to receive either IRQ or data! */
	/* set bif gpio as output */
	set_pin_dir(GPIO_DIR_OUT, GPIO_VALUE_HIGH);
	/* send a stop command first. */
	set_pin(GPIO_VALUE_HIGH);
	ns3300_udelay(g_ul_baud_stop);
	/* send BCF */
	set_pin(GPIO_VALUE_LOW);
	(ub_code & 0x08) ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);
	/* send _BCF */
	set_pin(GPIO_VALUE_HIGH);
	(ub_code & 0x04) ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);
	/* send BIT9 */
	set_pin(GPIO_VALUE_LOW);
	(ub_code & 0x02) ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);
	/* send BIT8 */
	set_pin(GPIO_VALUE_HIGH);
	(ub_code & 0x01) ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);
	/* send BIT7 */
	set_pin(GPIO_VALUE_LOW);
	(ub_data & 0x80) ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);
	/* send BIT6 */
	set_pin(GPIO_VALUE_HIGH);
	(ub_data & 0x40) ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);
	/* send BIT5 */
	set_pin(GPIO_VALUE_LOW);
	(ub_data & 0x20) ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);
	/* send BIT4 */
	set_pin(GPIO_VALUE_HIGH);
	(ub_data & 0x10) ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);
	/* send BIT3 */
	set_pin(GPIO_VALUE_LOW);
	(ub_data & 0x08) ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);
	/* send BIT2 */
	set_pin(GPIO_VALUE_HIGH);
	(ub_data & 0x04) ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);
	/* send BIT1 */
	set_pin(GPIO_VALUE_LOW);
	(ub_data & 0x02) ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);
	/* send BIT0 */
	set_pin(GPIO_VALUE_HIGH);
	(ub_data & 0x01) ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);
}

static void swi_do_send_cmd_step2(uint8_t crc8, uint8_t inversion)
{
	/* send CRC8 */
	set_pin(GPIO_VALUE_LOW);
	(crc8 & 0x80) ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);
	/* send BIT6 */
	set_pin(GPIO_VALUE_HIGH);
	(crc8 & 0x40) ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);
	/* send BIT5 */
	set_pin(GPIO_VALUE_LOW);
	(crc8 & 0x20) ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);
	/* send BIT4 */
	set_pin(GPIO_VALUE_HIGH);
	(crc8 & 0x10) ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);
	/* send BIT3 */
	set_pin(GPIO_VALUE_LOW);
	(crc8 & 0x08) ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);
	/* send BIT2 */
	set_pin(GPIO_VALUE_HIGH);
	(crc8 & 0x04) ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);
	/* send BIT1 */
	set_pin(GPIO_VALUE_LOW);
	(crc8 & 0x02) ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);
	/* send BIT0 */
	set_pin(GPIO_VALUE_HIGH);
	(crc8 & 0x01) ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);

	/* Send Inversion bit */
	set_pin(GPIO_VALUE_LOW);
	inversion ? ns3300_udelay(g_ul_baud_high) : ns3300_udelay(g_ul_baud_low);
	set_pin(GPIO_VALUE_HIGH);

	ns3300_udelay(g_ul_baud_stop);
}

static void swi_send_raw_word(uint8_t ub_code, uint8_t ub_data)
{
	uint8_t inversion = 0;
	uint8_t crc8;

	/* calculate crc8 */
	crc8 = swi_calculate_crc8(ub_code, ub_data);
	if (swi_treat_invert_flag(ub_code, ub_data, crc8)) {
		ub_code ^= 0x03; /* ub_code The first two bits remain unchanged */
		ub_data = ~ub_data;
		crc8 = ~crc8;
		inversion = 1;
	}
	swi_do_send_cmd_step1(ub_code, ub_data);
	swi_do_send_cmd_step2(crc8, inversion);
}

static bool swi_get_raw_word_time(uint32_t *ul_times, uint32_t ul_len, uint32_t *ul_threshold)
{
	bool b_previous_swi_state = false;
	uint8_t ubindex = ul_len - 1;
	uint8_t ub_bits_to_capture;
	uint32_t ul_timeout = g_ul_response_timeout;
	uint32_t ul_max_time = 0u;
	uint32_t ul_min_time = ~ul_max_time;
	uint32_t ul_count;

	/* set gpio as input */
	set_pin_dir(GPIO_DIR_IN, GPIO_VALUE_LOW);
	while (get_pin() && ul_timeout) {
		ns3300_udelay(1); /* delay 1 us */
		ul_timeout--;
	}
	/* exit with fail, if timeout criteria triggered */
	if (ul_timeout == 0u) {
		hwlog_err("[%s] ul_timeout\n", __func__);
		return false;
	}
	/* get port state */
	b_previous_swi_state = get_pin();

	ul_count = 0u;
	/* measure time of high and low phases */
	for (ub_bits_to_capture = ul_len; ub_bits_to_capture != 0u; ub_bits_to_capture--) {
		/* ulCount = 0u; */
		ul_timeout = g_ul_response_timeout;
		while (get_pin() == b_previous_swi_state && ul_timeout) {
			ns3300_udelay(1);
			ul_count++;
			ul_timeout--;
		}

		b_previous_swi_state = get_pin();
		ul_times[ubindex] += ul_count;
		ubindex--;
		ul_count = 0u;
	}

	/* evaluate detected results */
	for (ubindex = ul_len - 1; ubindex != 0u; ubindex--) {
		ul_count = ul_times[ubindex];
		if (ul_count < ul_min_time)
			ul_min_time = ul_count;
		else if (ul_count > ul_max_time)
			ul_max_time = ul_count;
	/* no change required */
	}

	/* calculate threshold */
	*ul_threshold = ((ul_max_time - ul_min_time) >> 1u);
	*ul_threshold += ul_min_time;
	return true;
}

static void swi_get_raw_word_bin(uint32_t *ul_times, unsigned int ul_len,
	uint32_t ul_threshold, uint8_t *up_byte, uint8_t *crc8_receive, uint8_t *ub_code)
{
	if (ul_len == 0)
		return;

	/* last 8 bits of payload */
	*up_byte = (((ul_times[8 + 8] > ul_threshold) ? 1u : 0u) << 7) |
		(((ul_times[7 + 8] > ul_threshold) ? 1u : 0u) << 6) |
		(((ul_times[6 + 8] > ul_threshold) ? 1u : 0u) << 5) |
		(((ul_times[5 + 8] > ul_threshold) ? 1u : 0u) << 4) |
		(((ul_times[4 + 8] > ul_threshold) ? 1u : 0u) << 3) |
		(((ul_times[3 + 8] > ul_threshold) ? 1u : 0u) << 2) |
		(((ul_times[2 + 8] > ul_threshold) ? 1u : 0u) << 1) |
		(((ul_times[1 + 8] > ul_threshold) ? 1u : 0u) << 0);

	/* get crc8 */
	*crc8_receive = (((ul_times[8] > ul_threshold) ? 1u : 0u) << 7) |
		(((ul_times[7] > ul_threshold) ? 1u : 0u) << 6) |
		(((ul_times[6] > ul_threshold) ? 1u : 0u) << 5) |
		(((ul_times[5] > ul_threshold) ? 1u : 0u) << 4) |
		(((ul_times[4] > ul_threshold) ? 1u : 0u) << 3) |
		(((ul_times[3] > ul_threshold) ? 1u : 0u) << 2) |
		(((ul_times[2] > ul_threshold) ? 1u : 0u) << 1) |
		(((ul_times[1] > ul_threshold) ? 1u : 0u) << 0);

	/* first two-bit of payload */
	*ub_code = (((ul_times[10 + 8] > ul_threshold) ? 1u : 0u) << 1) |
		(((ul_times[9 + 8] > ul_threshold) ? 1u : 0u) << 0);
}

static bool swi_receive_raw_word(uint8_t *up_byte)
{
	uint32_t ul_times[SWI_DATA_LEN] = { 0 };
	uint32_t ul_threshold;
	uint8_t ub_code = 0;
	uint8_t crc8_receive = 0;
	uint8_t crc8_calculate;
	bool ret;

	ret = swi_get_raw_word_time(ul_times, SWI_DATA_LEN, &ul_threshold);
	if (!ret)
		return ret;

	swi_get_raw_word_bin(ul_times, SWI_DATA_LEN, ul_threshold,
		up_byte, &crc8_receive, &ub_code);
	/* check inversion */
	if (((ul_times[0] > ul_threshold) ? 1u : 0u) == 1) {
		ub_code ^= 0x03;
		*up_byte = ~(*up_byte);
		crc8_receive = ~crc8_receive;
	}

	/* check crc */
	crc8_calculate = swi_calculate_crc8(ub_code, *up_byte);
	if (crc8_receive != crc8_calculate) {
		hwlog_err("%s crc8 fail, crc8_receive 0x%x, crc8_calculate 0x%x\n",
			__func__, crc8_receive, crc8_calculate);
		return false;
	}

	/* check training sequence */
	if (((ul_times[12 + 8] > ul_threshold) ? 1u : 0u) ==
		((ul_times[11 + 8] > ul_threshold) ? 1u : 0u)) {
		hwlog_err("%s training sequence error\n", __func__);
		return false;
	}

	/* all done well, set gpio as input */
	set_pin_dir(GPIO_DIR_OUT, GPIO_VALUE_HIGH);
	set_pin(GPIO_VALUE_HIGH);
	return true;
}

static bool swi_send_rra_rd_retry(uint16_t addr, uint8_t *data)
{
	uint8_t ub_code;
	uint8_t ub_data;

	ub_code = (uint8_t)(SWI_RRA | ((addr >> 8) & 0x1));
	ub_data = (uint8_t)(addr & 0xff);

	swi_send_raw_word(ub_code, ub_data);

	/* set gpio as input */
	return swi_receive_raw_word(data);
}

bool swi_send_rra_rd(uint16_t addr, uint8_t *data)
{
	int i;

	if (!data) {
		hwlog_err("%s: NULL pointer\n", __func__);
		return false;
	}

	ns3300_dev_lock();
	for (i = 0; i < RETYR_TIME; i++) {
		if (swi_send_rra_rd_retry(addr, data)) {
			ns3300_dev_unlock();
			return true;
		}
	}

	ns3300_dev_unlock();
	hwlog_err("%s fail\n", __func__);
	return false;
}

bool swi_write_byte(uint16_t uw_address, uint8_t ubp_data)
{
	swi_send_raw_word_noirq(SWI_WRA | ((uw_address >> 8) & 0x01u), (uw_address & 0xFFu));
	swi_send_raw_word_noirq(SWI_WD, ubp_data);
	return true;
}

void swi_send_raw_word_noirq(uint8_t ub_code, uint8_t ub_data)
{
	ns3300_dev_lock();
	swi_send_raw_word(ub_code, ub_data);
	ns3300_dev_unlock();
}
