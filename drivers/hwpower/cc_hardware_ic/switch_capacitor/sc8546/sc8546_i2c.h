/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sc8546_i2c.h
 *
 * sc8546 i2c header file
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

#ifndef _SC8546_I2C_H_
#define _SC8546_I2C_H_

#include "sc8546.h"

int sc8546_write_byte(struct sc8546_device_info *di, u8 reg, u8 value);
int sc8546_read_byte(struct sc8546_device_info *di, u8 reg, u8 *value);
int sc8546_read_word(struct sc8546_device_info *di, u8 reg, s16 *value);
int sc8546_write_mask(struct sc8546_device_info *di, u8 reg, u8 mask, u8 shift, u8 value);
int sc8546_write_block(struct sc8546_device_info *di, u8 reg, u8 *value, unsigned int num_bytes);
int sc8546_read_block(struct sc8546_device_info *di, u8 reg, u8 *value, unsigned int num_bytes);

#endif /* _SC8546_I2C_H_ */
