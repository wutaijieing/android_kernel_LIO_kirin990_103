/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sc8562_i2c.h
 *
 * sc8562 i2c header file
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

#ifndef _SC8562_I2C_H_
#define _SC8562_I2C_H_

#include "sc8562.h"

int sc8562_write_byte(struct sc8562_device_info *di, u8 reg, u8 value);
int sc8562_read_byte(struct sc8562_device_info *di, u8 reg, u8 *value);
int sc8562_read_word(struct sc8562_device_info *di, u8 reg, s16 *value);
int sc8562_write_mask(struct sc8562_device_info *di, u8 reg, u8 mask, u8 shift, u8 value);

#endif /* _SC8562_I2C_H_ */
