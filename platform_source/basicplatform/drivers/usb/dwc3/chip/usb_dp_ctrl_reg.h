/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: dp ctrl registers define
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2  of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef USB_DP_CTRL_REG_H
#define USB_DP_CTRL_REG_H

/* registers of USB_DP_CTRL */
#define USB_PHY_REG_ADDR	 0x8
#define USB_PHY_REG_DATA	 0xC
#define USB_CTRL_CFG0		 0x10
#define USB_PHY_CFG0		 0x20
#define USB_PHY_CFG6		 0x48
#define USB_PHY_STAT0		 0x74

/* bits of USB_PHY_CFG0 */
#define PIPE_RX_CDR_LEGACY_EN	BIT(11)
#define PHY0_CR_PARA_CLK_EN	BIT(3)
#define PHY0_CR_PARA_SEL	BIT(4)
#define PHY0_SS_MPLLA_SSC_EN	BIT(1)

/* bits of USB_PHY_REG_ADDR */
#define PHY_REG_ADDR_MASK	0xFFFF
#define PHY_REG_EN		BIT(16)

/* bits of USB_PHY_CFG6 */
#define PHY0_SRAM_BYPASS	BIT(24)
#define PHY0_SRAM_EXT_LD_DONE	BIT(23)

/* bits of USB_PHY_STAT0 */
#define PHY0_SRAM_INIT_DONE	BIT(2)

/* bits of USB_CTRL_CFG0 */
#define HOST_U3_PORT_DISABLE	BIT(10)
#define HOST_FORCE_GEN1_SPEED	BIT(19)
#define HOST_NUM_U3_PORT(n)	(((n) & 0xf) << 25)
#define HOST_NUM_U3_PORT_MASK	(0xf << 25)

#endif /* USB_DP_CTRL_REG_H */
