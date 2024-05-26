/*
 * pcie-kport-phy-mcu.h
 *
 * kport PHY mcu Driver
 *
 * Copyright (c) 2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#ifndef _PCIE_KPORT_PHY_MCU
#define _PCIE_KPORT_PHY_MCU

#define REG_DWORD_ALIGN		4

enum mcu_test_cmd {
	MCU_NONE = 0,
	MCU_RD_PHY_APB,
	MCU_WR_PHY_APB,
	MCU_RD_PHY,
	MCU_WR_PHY,
	MCU_DONE_FLAG,
};

/* test phy apb & phy addr */
#define PHYAPB_TEST_ADDR		0x00CC  /* sc_pciephy_ctrl51 [31:0] */
#define PCS_TOP_TEST_ADDR		0x0970  /* GP_INTR5_TRIG_VAL [31:0] */

/* ctx */
#define UCE_PROG_CTRL			0x010
#define MCU_START_MAGIC			0xABABABAB
#define MCU_START_MASK			0xFFFFFFFF /* [31:0] */

/* cmd */
#define UCE_PROG_CMD			0x014
#define UCE_PROG_CMD_MASK		0xFF /* [7:0] */
#define CMD_WAIT_TIMEOUT		50000

/* phy_core layout */
#define CORE_PC_START_ADDR	0x280
#define CORE_RAM_SIZE		0x6000

/* CPU to MCU IRQ */
#define PCS_IRQ_CPU_2_MCU		0x0014

/*
 * core memory layout
 * item		Offset	Size
 * TCM		0	32KB
 * IPC		32KB	4KB
 * UART		36KB	4KB
 * SCTRL	40KB	4KB
 */
#define PHY_CORE_SCTRL_OFFSET	(SIZE_32KB + SIZE_4KB + SIZE_4KB)

/* UCE Sctrl */
#define UCE_MCU_CTRL		0x0
#define UCE_MCU_CORE_WAIT	(1 << 3)
#define UCE_MCU_POR_PC		0x18
#define UCE_CRG_GT_EN		0x100
#define UCE_CRG_GT_BIT		0x1FF
#define UCE_CRG_RST_DIS		0x114
#define UCE_CRG_RST_BIT		0x1FF
#define MCU_APB_INT_MASK	0x10C

#endif /* _PCIE_KPORT_PHY_MCU */