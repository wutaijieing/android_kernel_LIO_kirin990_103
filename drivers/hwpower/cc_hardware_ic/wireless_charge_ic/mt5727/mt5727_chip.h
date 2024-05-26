/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mt5727_chip.h
 *
 * mt5727 registers, chip info, etc.
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

#ifndef _MT5727_CHIP_H_
#define _MT5727_CHIP_H_

#define MT5727_ADDR_LEN                      2

/* chip_info: 0x0000 ~ 0x000C */
#define MT5727_CHIP_INFO_ADDR                0x0000
#define MT5727_CHIP_INFO_LEN                 14
/* chip id register */
#define MT5727_CHIP_ID_ADDR                  0x0000
#define MT5727_HW_CHIP_ID_ADDR               0x5A50
#define MT5727_CHIP_ID_LEN                   2
#define MT5727_CHIP_ID                       0x5727
#define MT5727_HW_CHIP_ID                    0x5728
#define MT5727_CHIP_ID_AB                    0xFFFF /* abnormal chip id */
/* op mode register */
#define MT5727_OP_MODE_ADDR                  0x0005
#define MT5727_OP_MODE_LEN                   1
#define MT5727_OP_MODE_NA                    0x00
#define MT5727_OP_MODE_SA                    0x05 /* stand_alone */
#define MT5727_OP_MODE_RX                    BIT(0)
#define MT5727_OP_MODE_TX                    BIT(2)
#define MT5727_SYS_MODE_MASK                 (BIT(0) | BIT(2))
#define MT5727_OP_MODE_START_TX              BIT(4)
/* send_msg: bit[0]:header, bit[1]:cmd, bit[2:5]:data */
#define MT5727_SEND_MSG_DATA_LEN             4
#define MT5727_SEND_MSG_PKT_LEN              6
/* rcvd_msg: bit[0]:header, bit[1]:cmd, bit[2:5]:data */
#define MT5727_RCVD_MSG_DATA_LEN             4
#define MT5727_RCVD_MSG_PKT_LEN              6
#define MT5727_RCVD_PKT_BUFF_LEN             8
#define MT5727_RCVD_PKT_STR_LEN              64

/*
 * tx mode
 */
/* tx_cmd register */
#define MT5727_TX_CMD_ADDR                   0x0008
#define MT5727_TX_CMD_LEN                    4
#define MT5727_TX_CMD_VAL                    1
#define MT5727_TX_CMD_OPENLOOP               BIT(0)
#define MT5727_TX_CMD_OPENLOOP_SHIFT         0
#define MT5727_TX_CMD_RENORM                 BIT(1)
#define MT5727_TX_CMD_RENORM_SHIFT           1
#define MT5727_TX_CMD_SETPERIOD              BIT(2)
#define MT5727_TX_CMD_SETPERIOD_SHIFT        2
#define MT5727_TX_CMD_RST_SYS                BIT(4)
#define MT5727_TX_CMD_RST_SYS_SHIFT          4
#define MT5727_TX_CMD_CLEAR_INT              BIT(5)
#define MT5727_TX_CMD_CLEAR_INT_SHIFT        5
#define MT5727_TX_CMD_SEND_MSG               BIT(6)
#define MT5727_TX_CMD_SEND_MSG_SHIFT         6
#define MT5727_TX_CMD_OVP                    BIT(7)
#define MT5727_TX_CMD_OVP_SHIFT              7
#define MT5727_TX_CMD_OCP                    BIT(8)
#define MT5727_TX_CMD_OCP_SHIFT              8
#define MT5727_TX_CMD_PING_OCP               BIT(9)
#define MT5727_TX_CMD_PING_OCP_SHIFT         9
#define MT5727_TX_CMD_FOD_CTRL               BIT(10)
#define MT5727_TX_CMD_FOD_CTRL_SHIFT         10
#define MT5727_TX_CMD_LOW_POWER              BIT(11)
#define MT5727_TX_CMD_LOW_POWER_SHIFT        11
#define MT5727_TX_CMD_START_TX               BIT(12)
#define MT5727_TX_CMD_START_TX_SHIFT         12
#define MT5727_TX_CMD_STOP_TX                BIT(13)
#define MT5727_TX_CMD_STOP_TX_SHIFT          13
#define MT5727_TX_CMD_CLEAR_EPT              BIT(14)
#define MT5727_TX_CMD_CLEAR_EPT_SHIFT        14
/* tx_irq_en register */
#define MT5727_TX_IRQ_EN_ADDR                0x0010
#define MT5727_TX_IRQ_EN_LEN                 4
#define MT5727_TX_IRQ_EN_VAL                 0xCEEFFF
/* tx_irq_latch register */
#define MT5727_TX_IRQ_ADDR                   0x0014
#define MT5727_TX_IRQ_LEN                    4
#define MT5727_TX_IRQ_SS_PKG_RCVD            BIT(0)
#define MT5727_TX_IRQ_ID_PKT_RCVD            BIT(1)
#define MT5727_TX_IRQ_CFG_PKT_RCVD           BIT(2)
#define MT5727_TX_IRQ_DETECT_RX              BIT(3)
#define MT5727_TX_IRQ_OCP                    BIT(4)
#define MT5727_TX_IRQ_FOD_DET                BIT(5)
#define MT5727_TX_IRQ_REMOVE_POWER           BIT(6)
#define MT5727_TX_IRQ_CHIPRST                BIT(7)
#define MT5727_TX_IRQ_POWER_TRANS            BIT(8)
#define MT5727_TX_IRQ_POWERON                BIT(9)
#define MT5727_TX_IRQ_OVP                    BIT(10)
#define MT5727_TX_IRQ_PP_PKT_RCVD            BIT(11)
#define MT5727_TX_IRQ_TX_DISABLE             BIT(13)
#define MT5727_TX_IRQ_TX_ENABLE              BIT(14)
#define MT5727_TX_IRQ_EPT_PKT_RCVD           BIT(15)
#define MT5727_TX_IRQ_CHARGE_STATUS          BIT(16)
#define MT5727_TX_IRQ_START_PING             BIT(17)
#define MT5727_TX_IRQ_CEP_TIMEOUT            BIT(18)
#define MT5727_TX_IRQ_RPP_TIMEOUT            BIT(19)
#define MT5727_TX_IRQ_CEP_RECEIVED           BIT(20)
#define MT5727_TX_IRQ_RPP_RECEIVED           BIT(21)
#define MT5727_TX_IRQ_PING_OCP               BIT(22)
#define MT5727_TX_IRQ_PING_OVP               BIT(23)
/* tx_irq_clr register */
#define MT5727_TX_IRQ_CLR_ADDR               0x0018
#define MT5727_TX_IRQ_CLR_LEN                4
#define MT5727_TX_IRQ_CLR_ALL                0xFFFFFFFF
/* tx_rcvd_msg_data register */
#define MT5727_TX_RCVD_MSG_HEADER_ADDR       0x0020
#define MT5727_TX_RCVD_MSG_CMD_ADDR          0x0021
#define MT5727_TX_RCVD_MSG_DATA_ADDR         0x0022
/* tx_send_msg_data register */
#define MT5727_TX_SEND_MSG_HEADER_ADDR       0x0036
#define MT5727_TX_SEND_MSG_CMD_ADDR          0x0037
#define MT5727_TX_SEND_MSG_DATA_ADDR         0x0038
/* tx_max_fop, in kHz */
#define MT5727_TX_MAX_FOP_ADDR               0x004C
#define MT5727_TX_MAX_FOP_LEN                2
#define MT5727_TX_MAX_FOP                    145
#define MT5727_TX_FOP_STEP                   1
/* tx_min_fop, in kHz */
#define MT5727_TX_MIN_FOP_ADDR               0x004E
#define MT5727_TX_MIN_FOP_LEN                2
#define MT5727_TX_MIN_FOP                    113
/* tx_ping_freq, in kHz */
#define MT5727_TX_PING_FREQ_ADDR             0x0050
#define MT5727_TX_PING_FREQ_LEN              2
#define MT5727_TX_PING_FREQ                  130
#define MT5727_TX_PING_FREQ_MIN              100
#define MT5727_TX_PING_FREQ_MAX              150
#define MT5727_TX_PING_STEP                  1
/* tx_ping_ocp_thres register, in mA */
#define MT5727_TX_PING_OCP_TH_ADDR           0x0052
#define MT5727_TX_PING_OCP_TH_LEN            2
#define MT5727_TX_PING_OCP_TH                700
/* tx_ocp_thres register, in mA */
#define MT5727_TX_OCP_TH_ADDR                0x0054
#define MT5727_TX_OCP_TH_LEN                 2
#define MT5727_TX_OCP_TH                     2000
#define MT5727_TX_OCP_TH_STEP                1
/* tx_ovp_thres register, in mV */
#define MT5727_TX_OVP_TH_ADDR                0x0058
#define MT5727_TX_OVP_TH_LEN                 2
#define MT5727_TX_OVP_TH                     20000
#define MT5727_TX_OVP_TH_STEP                1
/* tx_ping_duty_cycle register */
#define MT5727_TX_PT_DC_ADDR                 0x0063
#define MT5727_TX_HALF_BRIDGE_DC             255
#define MT5727_TX_FULL_BRIDGE_DC             150
/* tx ept_reason register */
#define MT5727_TX_EPT_REASON_ADDR            0x006A
#define MT5727_TX_EPT_REASON_LEN             2
/* tx_fod_cnt register */
#define MT5727_TX_PLOSS_CNT_ADDR             0x006C
#define MT5727_TX_PLOSS_CNT_LEN              2
#define MT5727_TX_PLOSS_CNT_VAL              3
/* tx_oper_freq register, in 4Hz */
#define MT5727_TX_OP_FREQ_ADDR               0x006E
#define MT5727_TX_OP_FREQ_LEN                2
#define MT5727_TX_OP_FREQ_STEP               10
/* tx_clr_int_flag register */
#define MT5727_TX_IRQ_CLR_CTRL_ADDR          0x0070
#define MT5727_TX_IRQ_CLR_CTRL_LEN           1
#define MT5727_TX_IRQ_CLR_CTRL               1
/* tx_vrect register, in mV */
#define MT5727_TX_VRECT_ADDR                 0x008A
#define MT5727_TX_VRECT_LEN                  2
/* tx_iin register, in mA */
#define MT5727_TX_IIN_ADDR                   0x008E
#define MT5727_TX_IIN_LEN                    2
/* tx_vin register, in mV */
#define MT5727_TX_VIN_ADDR                   0x0090
#define MT5727_TX_VIN_LEN                    2
/* tx_fod_th register */
#define MT5727_TX_PLOSS_TH0_ADDR             0x0094
#define MT5727_TX_PLOSS_TH0_LEN              2
#define MT5727_TX_PLOSS_TH0_VAL              2000
/* tx_ping_interval, in ms */
#define MT5727_TX_PING_INTERVAL_ADDR         0x00A6
#define MT5727_TX_PING_INTERVAL_LEN          2
#define MT5727_TX_PING_INTERVAL_STEP         1
#define MT5727_TX_PING_INTERVAL_MIN          0
#define MT5727_TX_PING_INTERVAL_MAX          1000
#define MT5727_TX_PING_INTERVAL              120
/* tx_pwm_duty register */
#define MT5727_TX_PWM_DUTY_ADDR              0x00A8
#define MT5727_TX_PWM_DUTY_LEN               1
/* tx fsk depthoffset register */
#define MT5727_TX_FSK_DEPTH_ADDR             0x00A9
#define MT5727_TX_FSK_DEPTH_OFFSET           130
/* tx_ept_type register */
#define MT5727_TX_EPT_SRC_ADDR               0x00B0
#define MT5727_TX_EPT_SRC_LEN                4
#define MT5727_TX_EPT_SRC_CMD                BIT(0)
#define MT5727_TX_EPT_SRC_SS                 BIT(1)
#define MT5727_TX_EPT_SRC_ID                 BIT(2)
#define MT5727_TX_EPT_SRC_XID                BIT(3)
#define MT5727_TX_EPT_SRC_CFG_CNT            BIT(4)
#define MT5727_TX_EPT_SRC_PCH                BIT(5)
#define MT5727_TX_EPT_SRC_FIRSTCEP           BIT(6)
#define MT5727_TX_EPT_SRC_PING_TIMEOUT       BIT(7)
#define MT5727_TX_EPT_SRC_CEP_TIMEOUT        BIT(8)
#define MT5727_TX_EPT_SRC_RPP_TIMEOUT        BIT(9)
#define MT5727_TX_EPT_SRC_LVP                BIT(12)
#define MT5727_TX_EPT_SRC_OTP                BIT(14)
#define MT5727_TX_EPT_SRC_LCP                BIT(15)
#define MT5727_TX_EPT_SRC_CFG                BIT(16)
#define MT5727_TX_EPT_SRC_PING_OVP           BIT(17)
#define MT5727_TX_EPT_SRC_PKTERR             BIT(18)

/*
 * firmware register
 */

/* mtp register */
#define MT5727_MTP_MINOR_ADDR_H              0x0003
#define MT5727_MTP_MINOR_ADDR                0x0004
#define MT5727_MTP_MAJOR_ADDR                0x0006
#define MT5727_MTP_CHIP_ID_ADDR              0x0008
#define MT5727_BTLOADR_ADDR                  0x1800
/* mtp program */
#define MT5727_MTP_PGM_CMD_ADDR              0x0000
#define MT5727_MTP_PGM_CMD                   0x10
#define MT5727_MTP_PGM_SIZE                  16
#define MT5727_MTP_PGM_ADDR_ADDR             0x0002
#define MT5727_MTP_PGM_ADDR_VAL              0x0
#define MT5727_MTP_PGM_LEN_ADDR              0x0004
#define MT5727_MTP_PGM_CHKSUM_ADDR           0x0006
#define MT5727_MTP_PGM_DATA_ADDR             0x0008
/* crc check */
#define MT5727_MTP_CRC_ADDR                  0x0000
#define MT5727_MTP_CRC_CMD                   0x0040
/* mtp_status_addr */
#define MT5727_MTP_STATUS_ADDR               0x0000
#define MT5727_MTP_PGM_RUNNING               0x0001
#define MT5727_MTP_PGM_OK                    0x0002
#define MT5727_MTP_PGM_CHKSUM_ERR            0x0004
#define MT5727_MTP_PGM_OTHER_ERR             0x0020
#define MT5727_MTP_CRC_RUNNING               0x0040
#define MT5727_MTP_CRC_ERR                   0x0080
#define MT5727_MTP_CRC_OK                    0x0100
/* cortex M0 core */
#define MT5727_PMU_WDGEN_ADDR                0x5808
#define MT5727_WDG_DISABLE                   0x95
#define MT5727_WDG_ENABLE                    0x59
#define MT5727_PMU_FLAG_ADDR                 0x5800
#define MT5727_WDT_INTFALG                   0x03
#define MT5727_SYS_KEY_ADDR                  0x5244
#define MT5727_KEY_VAL                       0x57
#define MT5727_CODE_REMAP_ADDR               0x5208
#define MT5727_CODE_REMAP_VAL                0x08
#define MT5727_SRAM_REMAP_ADDR               0x5218
#define MT5727_SRAM_REMAP_VAL                0x0FFF
#define MT5727_M0_CTRL_ADDR                  0x5200
#define MT5727_M0_HOLD_VAL                   0x20
#define MT5727_M0_RST_VAL                    0x80

/* AC operation frequency to AC operation period */
#define OSCF2P(f)                            (80000 / (f) - 2)
/* AC operation period to AC operation frequency */
#define OSCP2F(p)                            (80000 / (p) + 2)

#endif /* _MT5727_CHIP_H_ */
