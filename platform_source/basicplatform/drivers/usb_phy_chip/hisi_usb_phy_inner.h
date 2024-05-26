/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: usb phy chip inner interface defination
 * Author: Hisilicon
 * Create: 2020-09-15
 *
 * This software is distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation,
 * either version 2 of that License or (at your option) any later version.
 */
#ifndef __HISI_USB_PHY_INNER_H__
#define __HISI_USB_PHY_INNER_H__

void _hi6502_readl_u32(unsigned int *val, unsigned int addr);
void _hi6502_writel_u32(unsigned int val, unsigned int addr);

#endif /* __HISI_USB_PHY_INNER_H__ */
