

#ifndef _PCIE_PCS_REGS_VER01_H_
#define _PCIE_PCS_REGS_VER01_H_


/* phy优化寄存器 */
#define PCIE_PHY_ICP_CTRL_EN_OFFSET             0x0F08
#define PCIE_PHY_ICP_CTRL_VAL_OFFSET            0x0F44
#define PCIE_PHY_TXEQVBST_OFFSET                0x1518
#define PCIE_PHY_UPDTCTRL_OFFSET                0x152C
#define PCIE_PHY_RXGSACTRL_OFFSET               0x1530
#define PCIE_PHY_RXEQACTRL_OFFSET               0x1534
#define PCIE_PHY_RXCTRL0_OFFSET                 0x1540
#define PCIE_PHY_RX_AFE_CFG_REG_OFFSET          0x1574
#define PCIE_PHY_DA_RX0_GEN_EN_OFFSET           0x1B14
#define PCIE_PHY_DA_0X_EQ_0_OFFSET              0x1B18 // PMA_INTF_LANE_SIG_CTRL_EN_REG_2
#define PCIE_PHY_DA_RX0_GEN_VAL_OFFSET          0x1B58
#define PCIE_PHY_DA_DFE_EN_OFFSET               0x1B24
#define PCIE_PHY_DA_FOM_0_OFFSET                0x1B5C // PMA_INTF_LANE_SIG_CTRL_VAL_REG_2
#define PCIE_PHY_DA_DFE_VAL_OFFSET              0x1B68
#define PCIE_PHY_DA_0X_EQ_0_STS_OFFSET          0x1B9C // PMA_INTF_LANE_SIG_CTRL_STS_REG_2


/* PHY寄存器偏移，根据lane number, PCLK_LANE */
#define PCIE_PCLK_LANE_OFFSET(lane, offset)      ((0x2000 * (lane)) + (offset))

#endif
