/* SPDX-License-Identifier: GPL-2.0 */
/*
 * hc32l110_i2c.h
 *
 * hc32l110 i2c header file
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

#ifndef _HC32L110_I2C_H_
#define _HC32L110_I2C_H_

#include "hc32l110.h"

int hc32l110_write_block(struct hc32l110_device_info *di, u8 reg, u8 *value,
	unsigned int num_bytes);
int hc32l110_read_block(struct hc32l110_device_info *di, u8 reg, u8 *value,
	unsigned int num_bytes);
int hc32l110_write_byte(struct hc32l110_device_info *di, u8 reg, u8 value);
int hc32l110_read_byte(struct hc32l110_device_info *di, u8 reg, u8 *value);
int hc32l110_write_mask(struct hc32l110_device_info *di, u8 reg, u8 mask,
	u8 shift, u8 value);
int hc32l110_read_word_bootloader(struct hc32l110_device_info *di,
	u8 *buf, u8 buf_len);
int hc32l110_write_word_bootloader(struct hc32l110_device_info *di,
	u8 *cmd, u8 cmd_len);

#endif /* _HC32L110_I2C_H_ */
