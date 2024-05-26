/* SPDX-License-Identifier: GPL-2.0 */
/*
 * cps2021_i2c.h
 *
 * cps2021 i2c header file
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

#ifndef _CPS2021_I2C_H_
#define _CPS2021_I2C_H_

#include "cps2021.h"

int cps2021_write_block(struct cps2021_device_info *di, u8 reg, u8 *value, unsigned int num_bytes);
int cps2021_read_block(struct cps2021_device_info *di, u8 reg, u8 *value, unsigned int num_bytes);
int cps2021_write_byte(struct cps2021_device_info *di, u8 reg, u8 value);
int cps2021_read_byte(struct cps2021_device_info *di, u8 reg, u8 *value);
int cps2021_write_mask(struct cps2021_device_info *di, u8 reg, u8 mask, u8 shift, u8 value);
int cps2021_read_word(struct cps2021_device_info *di, u8 reg, u16 *value);

#endif /* _CPS2021_I2C_H_ */
