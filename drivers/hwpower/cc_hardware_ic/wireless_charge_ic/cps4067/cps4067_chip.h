/* SPDX-License-Identifier: GPL-2.0 */
/*
 * cps4067_chip.h
 *
 * cps4067 registers, chip info, etc.
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

#ifndef _CPS4067_CHIP_H_
#define _CPS4067_CHIP_H_

#define CPS4067_ADDR_LEN                      2
#define CPS4067_HW_ADDR_LEN                   4
#define CPS4067_GPIO_PWR_GOOD_VAL             1

/* chip id register */
#define CPS4067_CHIP_ID_ADDR                  0x0000
#define CPS4067_CHIP_ID_LEN                   2
#define CPS4067_CHIP_ID                       0x4067
/* mtp ver register */
#define CPS4067_MTP_VER_ADDR                  0x0008
/* sys mode register */
#define CPS4067_SYS_MODE_ADDR                 0x000A
#define CPS4067_SYS_MODE_LEN                  1
#define CPS4067_SYS_MODE_NA                   0x00
#define CPS4067_SYS_MODE_BP                   0x01 /* back_powered */
#define CPS4067_SYS_MODE_TX                   0x02
#define CPS4067_SYS_MODE_RX                   0x03
/* crc val register */
#define CPS4067_CRC_ADDR                      0x000C
#define CPS4067_CRC_LEN                       2
/* send_msg_data register */
#define CPS4067_SEND_MSG_HEADER_ADDR          0x0080
#define CPS4067_SEND_MSG_CMD_ADDR             0x0081
#define CPS4067_SEND_MSG_DATA_ADDR            0x0082
/* send_msg: bit[0]: header, bit[1]: cmd, bit[2:5]: data */
#define CPS4067_SEND_MSG_DATA_LEN             4
#define CPS4067_SEND_MSG_PKT_LEN              6
/* rcvd_msg_data register */
#define CPS4067_RCVD_MSG_HEADER_ADDR          0x00A0
#define CPS4067_RCVD_MSG_CMD_ADDR             0x00A1
#define CPS4067_RCVD_MSG_DATA_ADDR            0x00A2
/* rcvd_msg: bit[0]: header, bit[1]: cmd, bit[2:5]: data */
#define CPS4067_RCVD_MSG_DATA_LEN             4
#define CPS4067_RCVD_MSG_PKT_LEN              6
#define CPS4067_RCVD_PKT_BUFF_LEN             8
#define CPS4067_RCVD_PKT_STR_LEN              64

/*
 * rx mode
 */

/* rx_irq_en register */
#define CPS4067_RX_IRQ_EN_ADDR                0x0040
#define CPS4067_RX_IRQ_EN_LEN                 4
/* rx_irq_latch register */
#define CPS4067_RX_IRQ_ADDR                   0x0044
#define CPS4067_RX_IRQ_LEN                    4
#define CPS4067_RX_IRQ_POWER_ON               BIT(0)
#define CPS4067_RX_IRQ_LDO_OFF                BIT(1)
#define CPS4067_RX_IRQ_LDO_ON                 BIT(2)
#define CPS4067_RX_IRQ_READY                  BIT(3)
#define CPS4067_RX_IRQ_FSK_ACK                BIT(4)
#define CPS4067_RX_IRQ_FSK_TIMEOUT            BIT(5)
#define CPS4067_RX_IRQ_FSK_PKT                BIT(6)
#define CPS4067_RX_IRQ_VRECT_OVP              BIT(7)
#define CPS4067_RX_IRQ_OTP                    BIT(11)
#define CPS4067_RX_IRQ_OCP                    BIT(13)
#define CPS4067_RX_IRQ_SCP                    BIT(15)
#define CPS4067_RX_IRQ_MLDO_OVP               BIT(18)
/* rx_irq_clr register */
#define CPS4067_RX_IRQ_CLR_ADDR               0x0048
#define CPS4067_RX_IRQ_CLR_LEN                4
#define CPS4067_RX_IRQ_CLR_ALL                0xFFFFFFFF
/* rx_cmd register */
#define CPS4067_RX_CMD_ADDR                   0x004C
#define CPS4067_RX_CMD_LEN                    4
#define CPS4067_RX_CMD_VAL                    1
#define CPS4067_RX_CMD_SEND_MSG               BIT(0)
#define CPS4067_RX_CMD_SEND_MSG_SHIFT         0
#define CPS4067_RX_CMD_SEND_FC                BIT(1)
#define CPS4067_RX_CMD_SEND_FC_SHIFT          1
#define CPS4067_RX_CMD_SEND_EPT               BIT(2)
#define CPS4067_RX_CMD_SEND_EPT_SHIFT         2
#define CPS4067_RX_CMD_SYS_RST                BIT(3)
#define CPS4067_RX_CMD_SYS_RST_SHIFT          3
#define CPS4067_RX_CMD_BST_EN                 BIT(4)
#define CPS4067_RX_CMD_BST_EN_SHIFT           4
#define CPS4067_RX_CMD_BST_DIS                BIT(5)
#define CPS4067_RX_CMD_BST_DIS_SHIFT          5
/* rx_func_en regiter */
#define CPS4067_RX_FUNC_EN_ADDR               0x0050
#define CPS4067_RX_FUNC_EN_LEN                4
#define CPS4067_RX_FUNC_EN                    1
#define CPS4067_RX_FUNC_DIS                   0
#define CPS4067_RX_HV_WDT_EN_MASK             BIT(2)
#define CPS4067_RX_HV_WDT_EN_SHIFT            2
#define CPS4067_RX_CMALL_EN_MASK              (BIT(3) | BIT(4) | BIT(5))
#define CPS4067_RX_CMALL_EN_SHIFT             3
#define CPS4067_RX_CMALL_EN_VAL               0x7 /* b3:CMA b4:CMB b5:CMC */
#define CPS4067_RX_CMC_EN_VAL                 0x4
#define CPS4067_RX_CMBC_EN_VAL                0x6
#define CPS4067_RX_CM_POLARITY_MASK           BIT(6)
#define CPS4067_RX_CM_POLARITY_SHIFT          6
#define CPS4067_RX_CM_POSITIVE                0
#define CPS4067_RX_CM_NEGTIVE                 1
#define CPS4067_RX_RP24BIT_EN_MASK            BIT(7)
#define CPS4067_RX_RP24BIT_EN_SHIFT           7
#define CPS4067_RX_RP24BIT_RES_EN_MASK        BIT(8)
#define CPS4067_RX_RP24BIT_RES_EN_SHIFT       8
#define CPS4067_RX_EXT_VCC_EN_MASK            BIT(9)
#define CPS4067_RX_EXT_VCC_EN_SHIFT           9
#define CPS4067_RX_MLDO_EN_MASK               BIT(10)
#define CPS4067_RX_MLDO_EN_SHIFT              10
/* rx_vout_set register, 3500-20000mV */
#define CPS4067_RX_VOUT_SET_ADDR              0x00C0
#define CPS4067_RX_VOUT_SET_LEN               2
#define CPS4067_RX_VOUT_MAX                   20000
#define CPS4067_RX_VOUT_MIN                   3500
/* rx_fc_volt register */
#define CPS4067_RX_FC_VPA_ADDR                0x00C2
#define CPS4067_RX_FC_VPA_LEN                 2
#define CPS4067_RX_FC_VPA_RETRY_CNT           3
#define CPS4067_RX_FC_VPA_SLEEP_TIME          50
#define CPS4067_RX_FC_VPA_TIMEOUT             1500
/* rx_fc_vmldo register */
#define CPS4067_RX_FC_VMLDO_ADDR              0x00C4
#define CPS4067_RX_FC_VMLDO_LEN               2
/* rx_fc_boost_mode_en register */
#define CPS4067_RX_FC_BST_EN_ADDR             0x00C6
#define CPS4067_RX_FC_BST_EN_LEN              1
/* rx_fc_boost_mode_dis register */
#define CPS4067_RX_FC_BST_DIS_ADDR            0x00C7
#define CPS4067_RX_FC_BST_DIS_LEN             1
/* rx_fc vrect delta register */
#define CPS4067_RX_FC_DELTA_ADDR              0x00C8
#define CPS4067_RX_FC_DELTA_LEN               2
#define CPS4067_RX_FC_DELTA_VAL               1500
/* rx_hv_watchdog register, in ms */
#define CPS4067_RX_WDT_ADDR                   0x00CA
#define CPS4067_RX_WDT_LEN                    2
#define CPS4067_RX_WDT_TO                     1000
/* rx_ldo_cfg: ldo_drop0-1 && ldo_cur_thres0-1 */
#define CPS4067_RX_LDO_CFG_ADDR               0x00CC
#define CPS4067_RX_LDO_CFG_LEN                8
/* no modulation dummy load size setting */
#define CPS4067_RX_DUMMY_LOAD_NO_MOD_ADDR     0x00D8
#define CPS4067_RX_DUMMY_LOAD_NO_MOD_LEN      1
#define CPS4067_RX_DUMMY_LOAD_NO_MOD_VAL      4
/* modulation dummy load size setting */
#define CPS4067_RX_DUMMY_LOAD_MOD_ADDR        0x00D9
#define CPS4067_RX_DUMMY_LOAD_MOD_LEN         1
#define CPS4067_RX_DUMMY_LOAD_MOD_VAL         16
/* rx_max_pwr for RP val calculation */
#define CPS4067_RX_RP_PMAX_ADDR               0x00F4
#define CPS4067_RX_RP_PMAX_LEN                1
#define CPS4067_RX_RP_VAL_UNIT                2 /* in 0.5Watt units */
/* rx_ept_val register, val to be sent in EPT packet */
#define CPS4067_RX_EPT_MSG_ADDR               0x00F5
#define CPS4067_RX_EPT_MSG_LEN                1
/* rx_fod_coef register */
#define CPS4067_RX_FOD_ADDR                   0x00F6
#define CPS4067_RX_FOD_LEN                    23
#define CPS4067_RX_FOD_STR_LEN                96
#define CPS4067_RX_FOD_TMP_STR_LEN            4
/* rx_vrect register */
#define CPS4067_RX_VRECT_ADDR                 0x0184
#define CPS4067_RX_VRECT_LEN                  2
/* rx_vrect_h register */
#define CPS4067_RX_VRECT_H_ADDR               0x0186
#define CPS4067_RX_VRECT_H_LEN                2
/* rx_iout register */
#define CPS4067_RX_IOUT_ADDR                  0x0188
#define CPS4067_RX_IOUT_LEN                   2
/* rx_vout register */
#define CPS4067_RX_VOUT_ADDR                  0x018A
#define CPS4067_RX_VOUT_LEN                   2
/* rx_chip_temp register, in degC */
#define CPS4067_RX_CHIP_TEMP_ADDR             0x018C
#define CPS4067_RX_CHIP_TEMP_LEN              2
/* rx_op_freq register, in kHZ */
#define CPS4067_RX_FOP_ADDR                   0x018E
#define CPS4067_RX_FOP_LEN                    2
/* rx_vrect_h register */
#define CPS4067_RX_VRECT_AVG_ADDR             0x0190
#define CPS4067_RX_VRECT_AVG_LEN              2
/* rx_iout register */
#define CPS4067_RX_IOUT_AVG_ADDR              0x0192
#define CPS4067_RX_IOUT_AVG_LEN               2
/* rx_signal_strength register */
#define CPS4067_RX_SS_ADDR                    0x0194
#define CPS4067_RX_SS_LEN                     2
#define CPS4067_RX_SS_MIN                     0
#define CPS4067_RX_SS_MAX                     255
/* rx_ce_val register */
#define CPS4067_RX_CE_VAL_ADDR                0x0196
#define CPS4067_RX_CE_VAL_LEN                 2
/* rx_rp_val register */
#define CPS4067_RX_RP_VAL_ADDR                0x0198
#define CPS4067_RX_RP_VAL_LEN                 2
/* rx_ept_code register */
#define CPS4067_RX_EPT_CODE_ADDR              0x019A
#define CPS4067_RX_EPT_CODE_LEN               2
/* rx_status register */
#define CPS4067_RX_STATUS_ADDR                0x019C
#define CPS4067_RX_STATUS_LEN                 1
#define CPS4067_RX_SR_BRI_MASK                BIT(0)
#define CPS4067_RX_SR_BRI_SHIFT               0
#define CPS4067_RX_FC_SUCC_MASK               BIT(1)
#define CPS4067_RX_FC_SUCC_SHIFT              1
#define CPS4067_RX_FC_VOUT_TIMEOUT            1500
#define CPS4067_RX_FC_VOUT_SLEEP_TIME         50
#define CPS4067_RX_SET_VMLDO_SLEEP_TIMEOUT    500
#define CPS4067_RX_SET_VMLDO_SLEEP_TIME       50

/*
 * tx mode
 */

/* tx_irq_en register */
#define CPS4067_TX_IRQ_EN_ADDR                0x0040
#define CPS4067_TX_IRQ_VAL                    0XFFFFFBFF
#define CPS4067_TX_IRQ_EN_LEN                 4
/* tx_irq_latch register */
#define CPS4067_TX_IRQ_ADDR                   0x0044
#define CPS4067_TX_IRQ_LEN                    4
#define CPS4067_TX_IRQ_START_PING             BIT(0)
#define CPS4067_TX_IRQ_SS_PKG_RCVD            BIT(1)
#define CPS4067_TX_IRQ_ID_PKT_RCVD            BIT(2)
#define CPS4067_TX_IRQ_CFG_PKT_RCVD           BIT(3)
#define CPS4067_TX_IRQ_ASK_PKT_RCVD           BIT(4)
#define CPS4067_TX_IRQ_EPT_PKT_RCVD           BIT(5)
#define CPS4067_TX_IRQ_RPP_TIMEOUT            BIT(6)
#define CPS4067_TX_IRQ_CEP_TIMEOUT            BIT(7)
#define CPS4067_TX_IRQ_AC_DET                 BIT(8)
#define CPS4067_TX_IRQ_TX_INIT                BIT(9)
#define CPS4067_TX_IRQ_ASK_ALL                BIT(10)
#define CPS4067_TX_IRQ_RPP_TYPE_ERR           BIT(11)
#define CPS4067_TX_IRQ_FSK_ACK                BIT(12)
#define CPS4067_TX_IRQ_DET_OTHER_TX           BIT(13)
#define CPS4067_TX_IRQ_TX_HTP                 BIT(14)
#define CPS4067_TX_IRQ_POVP                   BIT(15)
#define CPS4067_TX_IRQ_LP_END                 BIT(16)
#define CPS4067_TX_IRQ_FULL_BRIDGE            BIT(17)
#define CPS4067_TX_IRQ_HALF_BRIDGE            BIT(18)
/* tx_irq_clr register */
#define CPS4067_TX_IRQ_CLR_ADDR               0x0048
#define CPS4067_TX_IRQ_CLR_LEN                4
#define CPS4067_TX_IRQ_CLR_ALL                0xFFFFFFFF
/* tx_cmd register */
#define CPS4067_TX_CMD_ADDR                   0x004C
#define CPS4067_TX_CMD_LEN                    4
#define CPS4067_TX_CMD_VAL                    1
#define CPS4067_TX_CMD_CRC_CHK                BIT(0)
#define CPS4067_TX_CMD_CRC_CHK_SHIFT          0
#define CPS4067_TX_CMD_EN_TX                  BIT(1)
#define CPS4067_TX_CMD_EN_TX_SHIFT            1
#define CPS4067_TX_CMD_DIS_TX                 BIT(2)
#define CPS4067_TX_CMD_DIS_TX_SHIFT           2
#define CPS4067_TX_CMD_SEND_MSG               BIT(3)
#define CPS4067_TX_CMD_SEND_MSG_SHIFT         3
#define CPS4067_TX_CMD_SYS_RST                BIT(4)
#define CPS4067_TX_CMD_SYS_RST_SHIFT          4
/* func_en register */
#define CPS4067_TX_FUNC_EN_ADDR               0x0050
#define CPS4067_TX_FUNC_EN_LEN                4
#define CPS4067_TX_FUNC_EN                    1
#define CPS4067_TX_FUNC_DIS                   0
#define CPS4067_TX_SWITCH_BRI_CTL_VTH         7500 /* mV */
#define CPS4067_TX_FOD_EN_MASK                BIT(0)
#define CPS4067_TX_FOD_EN_SHIFT               0
#define CPS4067_TX_FULL_BRI_EN_MASK           BIT(1)
#define CPS4067_TX_FULL_BRI_EN_SHIFT          1
#define CPS4067_TX_FULL_BRI_ITH               200 /* mA */
#define CPS4067_TX_PING_EN_MASK               BIT(2) /* 1: start ping 0: stop pt */
#define CPS4067_TX_PING_EN_SHIFT              2
#define CPS4067_TX_24BITRPP_EN_MASK           BIT(3)
#define CPS4067_TX_24BITRPP_EN_SHIFT          3
#define CPS4067_TX_QVAL_EN_MASK               BIT(4)
#define CPS4067_TX_QVAL_EN_SHIFT              4
#define CPS4067_TX_LP_EN_MASK                 BIT(5)
#define CPS4067_TX_LP_EN_SHIFT                5
#define CPS4067_TX_SWITCH_BRI_CTL_MASK        BIT(6) /* 1: set bridge by ap 0: set bridge by fw */
#define CPS4067_TX_SWITCH_BRI_CTL_SHIFT       6
/* tx_pmax register */
#define CPS4067_TX_PMAX_ADDR                  0x0200
#define CPS4067_TX_PMAX_LEN                   1
#define CPS4067_TX_RPP_VAL_UNIT               2 /* in 0.5Watt units */
/* tx_ocp_thres register, in mA */
#define CPS4067_TX_OCP_TH_ADDR                0x0202
#define CPS4067_TX_OCP_TH_LEN                 2
#define CPS4067_TX_OCP_TH                     2000
/* tx_ovp_thres register, in mV */
#define CPS4067_TX_OVP_TH_ADDR                0x0206
#define CPS4067_TX_OVP_TH_LEN                 2
#define CPS4067_TX_OVP_TH                     20000
/* tx_ilimit register */
#define CPS4067_TX_ILIM_ADDR                  0x020A
#define CPS4067_TX_ILIM_LEN                   2
#define CPS4067_TX_ILIM_MIN                   500
#define CPS4067_TX_ILIM_MAX                   2000
/* tx_min_fop, in kHz */
#define CPS4067_TX_MIN_FOP_ADDR               0x0210
#define CPS4067_TX_MIN_FOP_LEN                1
#define CPS4067_TX_MIN_FOP                    113
/* tx_max_fop, in kHz */
#define CPS4067_TX_MAX_FOP_ADDR               0x0211
#define CPS4067_TX_MAX_FOP_LEN                1
#define CPS4067_TX_MAX_FOP                    145
/* tx_ping_freq, in kHz */
#define CPS4067_TX_PING_FREQ_ADDR             0x0212
#define CPS4067_TX_PING_FREQ_LEN              1
#define CPS4067_TX_PING_FREQ                  115
#define CPS4067_TX_PING_FREQ_MIN              100
#define CPS4067_TX_PING_FREQ_MAX              180
/* tx_ping_interval, in ms */
#define CPS4067_TX_PING_INTERVAL_ADDR         0x0216
#define CPS4067_TX_PING_INTERVAL_LEN          2
#define CPS4067_TX_PING_INTERVAL_MIN          0
#define CPS4067_TX_PING_INTERVAL_MAX          1000
#define CPS4067_TX_PING_INTERVAL              500
/* tx_fod_ploss_cnt register */
#define CPS4067_TX_PLOSS_CNT_ADDR             0x021D
#define CPS4067_TX_PLOSS_CNT_VAL              3
#define CPS4067_TX_PLOSS_CNT_LEN              1
/* tx_fod_ploss_thres register, in mW */
#define CPS4067_TX_PLOSS_TH0_ADDR             0x021E /* 5v full bridge */
#define CPS4067_TX_PLOSS_TH0_VAL              4000
#define CPS4067_TX_PLOSS_TH0_LEN              2
#define CPS4067_TX_PLOSS_TH1_ADDR             0x0220 /* 10v half bridge */
#define CPS4067_TX_PLOSS_TH1_VAL              4000
#define CPS4067_TX_PLOSS_TH1_LEN              2
#define CPS4067_TX_PLOSS_TH2_ADDR             0x0222 /* 10v full bridge low vol */
#define CPS4067_TX_PLOSS_TH2_VAL              4000
#define CPS4067_TX_PLOSS_TH2_LEN              2
#define CPS4067_TX_PLOSS_TH3_ADDR             0x0224 /* 10v full bridge high vol */
#define CPS4067_TX_PLOSS_TH3_VAL              4000
#define CPS4067_TX_PLOSS_TH3_LEN              2
/* tx_q_fod_only_coil_th */
#define CPS4067_TX_Q_ONLY_COIL_ADDR           0x0232
#define CPS4067_TX_Q_ONLY_COIL_TH             140
/* tx_q_fod_only_coil_th */
#define CPS4067_TX_Q_TH_ADDR                  0x0234
#define CPS4067_TX_Q_TH                       110
/* tx_bridge set by fw register */
#define CPS4067_TX_BRIDGE_EN_ADDR             0x024A
#define CPS4067_TX_BRIDGE_EN_LEN              2
#define CPS4067_TX_BRIDGE_EN_TH               7500 /* mV */
/* tx_vin register, in mV */
#define CPS4067_TX_VIN_ADDR                   0x02C2
#define CPS4067_TX_VIN_LEN                    2
/* tx_vrect register, in mV */
#define CPS4067_TX_VRECT_ADDR                 0x02C4
#define CPS4067_TX_VRECT_LEN                  2
/* tx_iin register, in mA */
#define CPS4067_TX_IIN_ADDR                   0x02C6
#define CPS4067_TX_IIN_LEN                    2
/* tx_chip_temp register, in degC */
#define CPS4067_TX_CHIP_TEMP_ADDR             0x02C8
#define CPS4067_TX_CHIP_TEMP_LEN              2
/* tx_oper_freq register, in Hz */
#define CPS4067_TX_OP_FREQ_ADDR               0x02C0
#define CPS4067_TX_OP_FREQ_LEN                2
/* tx_cep_value register */
#define CPS4067_TX_CEP_ADDR                   0x02CB
#define CPS4067_TX_CEP_LEN                    1
/* tx_ept_rsn register */
#define CPS4067_TX_EPT_SRC_ADDR               0x02CE
#define CPS4067_TX_EPT_SRC_CLR                0
#define CPS4067_TX_EPT_SRC_LEN                2
#define CPS4067_TX_EPT_SRC_WRONG_PKT          BIT(0) /* re ping */
#define CPS4067_TX_EPT_SRC_POVP               BIT(1) /* re ping */
#define CPS4067_TX_EPT_SRC_AC_DET             BIT(2) /* re ping */
#define CPS4067_TX_EPT_SRC_RX_EPT             BIT(3) /* re ping */
#define CPS4067_TX_EPT_SRC_CEP_TIMEOUT        BIT(4) /* re ping */
#define CPS4067_TX_EPT_SRC_RPP_TIMEOUT        BIT(5) /* re ping */
#define CPS4067_TX_EPT_SRC_OCP                BIT(6) /* re ping */
#define CPS4067_TX_EPT_SRC_OVP                BIT(7) /* re ping */
#define CPS4067_TX_EPT_SRC_UVP                BIT(8) /* re ping */
#define CPS4067_TX_EPT_SRC_FOD                BIT(9) /* stop */
#define CPS4067_TX_EPT_SRC_OTP                BIT(10) /* re ping */
#define CPS4067_TX_EPT_SRC_POCP               BIT(11) /* re ping */
#define CPS4067_TX_EPT_SRC_Q_FOD              BIT(12) /* stop */
#define CPS4067_TX_EPT_SRC_SR_OCP             BIT(13) /* re ping */
#define CPS4067_TX_EPT_SRC_VRECT_OVP          BIT(14) /* re ping */
#define CPS4067_TX_EPT_SRC_VRECTH_OVP         BIT(15) /* re ping */
/* rx_qi_status register */
#define CPS4067_TX_QI_STATUS_ADDR             0x02E1
#define CPS4067_TX_QI_STATUS_LEN              1
#define CPS4067_TX_QI_PING_DIS                0x00
#define CPS4067_TX_QI_PING_PHASE              0x11
#define CPS4067_TX_QI_IDCFG_PHASE             0x22
#define CPS4067_TX_QI_PT_PHASE                0x33

/*
 * firmware register
 */
#define CPS4067_SRAM_BTL_ADDR                 0x20000000 /* 2k */
#define CPS4067_SRAM_MTP_BUFF0                0x20000800 /* 2k */
#define CPS4067_HW_MTP_BUFF_SIZE              2048 /* 2k */
#define CPS4067_SRAM_STRAT_CMD_ADDR           0x20001800
#define CPS4067_STRAT_CARRY_BUF0              0x00000010
#define CPS4067_STRAT_CARRY_BUF1              0x00000020
#define CPS4067_START_CHK_BTL                 0x000000B0
#define CPS4067_START_CHK_MTP                 0x00000090
#define CPS4067_SRAM_CHK_CMD_ADDR             0x20001804
#define CPS4067_CHK_SUCC                      0x55
#define CPS4067_CHK_FAIL                      0xAA /* 0x66: running */
#define CPS4067_SRAM_BTL_VER_ADDR             0x2000180C
/* fw crc calculation */
#define CPS4067_MTP_CRC_SEED                  0x1021
#define CPS4067_MTP_CRC_HIGHEST_BIT           0x8000

#endif /* _CPS4067_CHIP_H_ */
