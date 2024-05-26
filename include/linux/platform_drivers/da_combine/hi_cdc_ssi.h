/*
 * hi_cdc_ssi.h
 *
 * codec ssi driver
 *
 * Copyright (c) 2014-2020 Huawei Technologies Co., Ltd.All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __HI_CDC_SSI_H__
#define __HI_CDC_SSI_H__

unsigned int ssi_reg_read8(unsigned int reg);
unsigned int ssi_reg_read32(unsigned int reg);
void ssi_reg_write8(unsigned int reg, unsigned int val);
void ssi_reg_write32(unsigned int reg, unsigned int val);

#endif /* __HI_CDC_SSI_H__ */
