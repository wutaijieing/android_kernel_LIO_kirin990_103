/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mt5785_chip.h
 *
 * mt5785 registers, chip info, etc.
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

#ifndef _MT5785_CHIP_H_
#define _MT5785_CHIP_H_

#define MT5785_GPIO_PWR_GOOD_VAL             1
#define MT5785_ADDR_LEN                      2

/* chip id register */
#define MT5785_CHIP_ID_ADDR                  0x0000
#define MT5785_CHIP_ID_LEN                   2

/* mtp ver register */
#define MT5785_MTP_VER_ADDR                  0x0008
#define MT5785_MTP_VER_LEN                   2

/*
 * rx mode
 */

/* sys mode register */
#define MT5785_RX_SYS_MODE_ADDR              0x0010
#define MT5785_RX_SYS_MODE_LEN               4
#define MT5785_RX_SYS_MODE_CRC_OK            BIT(2)
#define MT5785_RX_SYS_MODE_CRC_ERROR         BIT(3)
#define MT5785_RX_SYS_MODE_RX                BIT(8)
#define MT5785_RX_SYS_MODE_BRIDGE_MASK       BIT(18)
#define MT5785_RX_SYS_MODE_BRIDGE_SHIFT      18
#define MT5785_RX_SYS_MODE_FC_SUCC_MASK      BIT(20)
#define MT5785_RX_SYS_MODE_FC_SUCC_SHIFT     20
#define MT5785_RX_FC_VOUT_TIMEOUT            1500
#define MT5785_RX_FC_VOUT_SLEEP_TIME         50

/* interrupt flag register */
#define MT5785_RX_IRQ_ADDR                   0X0018
#define MT5785_RX_IRQ_LEN                    4
#define MT5785_RX_IRQ_READY                  BIT(9)
#define MT5785_RX_IRQ_POWER_ON               BIT(8)
#define MT5785_RX_IRQ_FSK_PKT                BIT(14)
#define MT5785_RX_IRQ_OCP                    BIT(17)
#define MT5785_RX_IRQ_OVP                    BIT(18)
#define MT5785_RX_IRQ_OTP                    BIT(19)

/* rx_irq_clr register */
#define MT5785_RX_IRQ_CLR_ADDR               0X001c
#define MT5785_RX_IRQ_CLR_LEN                4
#define MT5785_RX_IRQ_CLR_ALL                0xFFFFFFFF

/* rx_cmd register */
#define MT5785_RX_CMD_ADDR                   0x0020
#define MT5785_RX_CMD_LEN                    4
#define MT5785_RX_CMD_VAL                    1
#define MT5785_RX_CMD_SYS_RST_MASK           BIT(2)
#define MT5785_RX_CMD_SYS_RST_SHIFT          2
#define MT5785_RX_CMD_CRC_CHECK_MASK         BIT(3)
#define MT5785_RX_CMD_CRC_CHECK_SHIFT        3
#define MT5785_RX_CMD_CLEAR_INT_MASK         BIT(4)
#define MT5785_RX_CMD_CLEAR_INT_SHIFT        4
#define MT5785_RX_CMD_SEND_PPP_MASK          BIT(8)
#define MT5785_RX_CMD_SEND_PPP_SHIFT         8
#define MT5785_RX_CMD_SEND_EPT_MASK          BIT(9)
#define MT5785_RX_CMD_SEND_EPT_SHIFT         9
#define MT5785_RX_CMD_SEND_FC_MASK           BIT(10)
#define MT5785_RX_CMD_SEND_FC_SHIFT          10
#define MT5785_RX_CMD_VOUT_MASK              BIT(11)
#define MT5785_RX_CMD_VOUT_SHIFT             11
#define MT5785_RX_CMD_EXT_VCC_EN_MASK        BIT(16)
#define MT5785_RX_CMD_EXT_VCC_EN_SHIFT       16
#define MT5785_RX_CMD_EXT_VCC_DIS_MASK       BIT(17)
#define MT5785_RX_CMD_EXT_VCC_DIS_SHIFT      17
#define MT5785_RX_CMD_24BIT_RPP_MASK         BIT(19)
#define MT5785_RX_CMD_24BIT_RPP_SHIFT        19
#define MT5785_RX_CMD_SWITCH_BRG_MASK        BIT(20)
#define MT5785_RX_CMD_SWITCH_BRG_SHIFT       20

/* rx_op_freq register, in kHZ */
#define MT5785_RX_FOP_ADDR                   0x0030
#define MT5785_RX_FOP_LEN                    2

/* rx_iout register */
#define MT5785_RX_IOUT_ADDR                  0x0034
#define MT5785_RX_IOUT_LEN                   2

/* rx_vout register */
#define MT5785_RX_VOUT_ADDR                  0x0036
#define MT5785_RX_VOUT_LEN                   2

/* rx_vrect register */
#define MT5785_RX_VRECT_ADDR                 0x0038
#define MT5785_RX_VRECT_LEN                  2

/* rx_chip_temp register, in degC */
#define MT5785_RX_CHIP_TEMP_ADDR             0x003a
#define MT5785_RX_CHIP_TEMP_LEN              2

/* Parameter of  Set_Max_Power CMD */
#define MT5785_RX_MAX_POWER_ADDR             0x0048
#define MT5785_RX_MAX_POWER_LEN              2
#define MT5785_RX_RP_VAL_UNIT                2

/* rx_fc_volt register */
#define MT5785_RX_FC_VPA_ADDR                0x004e
#define MT5785_RX_FC_VPA_LEN                 2
#define MT5785_RX_FC_VPA_RETRY_CNT           3

/* rx watchdog register*/
#define MT5785_RX_WDT_ADDR                   0x006c
#define MT5785_RX_WDT_LEN                    2
#define MT5785_RX_KICK_WDT_VAL               0

/* rx_vout_set register, 3500-20000mV */
#define MT5785_RX_VOUT_SET_ADDR              0x0072
#define MT5785_RX_VOUT_SET_LEN               2
#define MT5785_RX_VOUT_MAX                   20000
#define MT5785_RX_VOUT_MIN                   3500

/* rx_fc_vmldo register */
#define MT5785_RX_FC_VMLDO_ADDR              0x0074
#define MT5785_RX_FC_VMLDO_LEN               2
#define MT5785_RX_SET_VMLDO_SLEEP_TIMEOUT    500
#define MT5785_RX_SET_VMLDO_SLEEP_TIME       50

/* Vrect curve parameter */
#define MT5785_RX_LDO_CFG_ADDR               0x0078
#define MT5785_RX_LDO_CFG_LEN                8

/* full bridge current */
#define MT5785_RX_REC_IfULL_ADDR             0x0084
#define MT5785_RX_REC_IfULL_LEN              2

/* watchdog overflow threshold */
#define MT5785_RX_WDT_TIMEOUT_ADDR           0x008a
#define MT5785_RX_WDT_TIMEOUT_LEN            2
#define MT5785_RX_WDT_TIMEOUT                1000

/* ASK header register */
#define MT5785_RX_SEND_MSG_HEADER_ADDR       0x0090
#define MT5785_RX_SEND_MSG_HEADER_LEN        1

/* ask msg cmd */
#define MT5785_RX_SEND_MSG_CMD_ADDR          0x0091
#define MT5785_RX_SEND_MSG_CMD_LEN           1

/* ask msg data */
#define MT5785_RX_SEND_MSG_DATA_ADDR         0x0092
#define MT5785_RX_SEND_MSG_DATA_LEN          4

/* FSK Proprietary packet header */
#define MT5785_RX_FSK_HEADER_ADDR            0x00a6
#define MT5785_RX_FSK_HEADER_LEN             1

/* FSK Proprietary packet message0 */
#define MT5785_RX_FSK_CMD_ADDR               0x00a7
#define MT5785_RX_FSK_CMD_LEN                1

/* ask modulation cap */
#define MT5785_RX_ASK_CAP_ADDR               0x00b0
#define MT5785_RX_ASK_CAP_LEN                1
#define MT5785_RX_ASK_CAP_CM_MASK            (BIT(0) | BIT(1))
#define MT5785_RX_ASK_CAP_POLARITY_SHIFT     3
#define MT5785_RX_CM_POSITIVE                0
#define MT5785_RX_CM_NEGTIVE                 1
#define MT5785_RX_CMALL_EN_VAL               0x3

/* ask modulation cap */
#define MT5785_RX_SWITCH_BRIDGE_MODE_ADDR    0x00b3
#define MT5785_RX_SWITCH_BRIDGE_MODE_LEN     1
#define MT5785_RX_FULL_BRIDGE                1
#define MT5785_RX_HALF_BRIDGE                0

/* EPT type */
#define MT5785_RX_EPT_MSG_ADDR               0x0110
#define MT5785_RX_EPT_MSG_LEN                1

/* signal strength value */
#define MT5785_RX_SS_ADDR                    0x0111
#define MT5785_RX_SS_LEN                     1
#define MT5785_RX_SS_MAX                     255

/* rx_cep_val register */
#define MT5785_RX_CEP_VAL_ADDR               0x0112
#define MT5785_RX_CEP_VAL_LEN                1

/*
 * tx mode
 */

/* sys mode register */
#define MT5785_TX_SYS_MODE_ADDR              0x0010
#define MT5785_TX_SYS_MODE_LEN               4
#define MT5785_TX_SYS_MODE_RX                BIT(8)
#define MT5785_TX_SYS_MODE_TX                BIT(9)

/* enable int register */
#define MT5785_TX_INT_EN_ADDR                0x0014
#define MT5785_TX_INT_EN_LEN                 4
#define MT5785_TX_INT_EN_ALL                 0XFFFFFFFF

/* interrupt flag register */
#define MT5785_TX_INT_FLAG_ADDR              0x0018
#define MT5785_TX_INT_FLAG_LEN               4
#define MT5785_TX_INT_SS_PKG_RCVD            BIT(8)
#define MT5785_TX_INT_ID_PKT_RCVD            BIT(9)
#define MT5785_TX_INT_CFG_PKT_RCVD           BIT(10)
#define MT5785_TX_INT_ASK_PKT_RCVD           BIT(15)
#define MT5785_TX_INT_AC_DET                 BIT(16)
#define MT5785_TX_INT_EPT                    BIT(17)
#define MT5785_TX_INT_START_PING             BIT(18)
#define MT5785_TX_INT_FSK_PP_ACK             BIT(21)
#define MT5785_TX_INT_RPP_TIMEOUT            BIT(24)

/* clear interrrupt flag */
#define MT5785_TX_INT_CLEAR_ADDR             0x001c
#define MT5785_TX_INT_CLEAR_LEN              4
#define MT5785_TX_CLEAR_ALL_INT              0xFFFFFFFF

/* cmd register */
#define MT5785_TX_CMD_ADDR                   0x0020
#define MT5785_TX_CMD_LEN                    4
#define MT5785_TX_CMD_VALUE                  1
#define MT5785_TX_CMD_ENTER_TX_MASK          BIT(0)
#define MT5785_TX_CMD_ENTER_TX_SHIFT         0
#define MT5785_TX_CMD_CHIP_RST_MASK          BIT(2)
#define MT5785_TX_CMD_CHIP_RST_SHIFT         2
#define MT5785_TX_CMD_CLEAR_INT_MASK         BIT(4)
#define MT5785_TX_CMD_CLEAR_INT_SHIFT        4
#define MT5785_TX_CMD_START_WORK_MASK        BIT(8)
#define MT5785_TX_CMD_START_WORK_SHIFT       8
#define MT5785_TX_CMD_STOP_WORK_MASK         BIT(9)
#define MT5785_TX_CMD_STOP_WORK_SHIFT        9
#define MT5785_TX_CMD_SEND_PPP_MASK          BIT(10)
#define MT5785_TX_CMD_SEND_PPP_SHIFT         10
#define MT5785_TX_CMD_24BIT_RPP_EN_MASK      BIT(20)
#define MT5785_TX_CMD_24BIT_RPP_EN_SHIFT     20

/* fop register */
#define MT5785_TX_FOP_ADDR                   0x0030
#define MT5785_TX_FOP_LEN                    2

/* input current register */
#define MT5785_TX_IIN_ADDR                   0x0034
#define MT5785_TX_IIN_LEN                    2

/* input voltage register */
#define MT5785_TX_VIN_ADDR                   0x0036
#define MT5785_TX_VIN_LEN                    2

/* bridge voltage register */
#define MT5785_TX_VBRG_ADDR                  0X0038
#define MT5785_TX_VBRG_LEN                   2

/* temp register */
#define MT5785_TX_TEMP_ADDR                  0x003a
#define MT5785_TX_TEMP_LEN                   2

/* max operation freq register */
#define MT5785_TX_MAX_OP_FREQ_ADDR           0x0050
#define MT5785_TX_MAX_OP_FREQ_LEN            2

/* min operation freq register */
#define MT5785_TX_MIN_OP_FREQ_ADDR           0x0052
#define MT5785_TX_MIN_OP_FREQ_LEN            2
#define MT5785_TX_MIN_FOP                    113
#define MT5785_TX_MAX_FOP                    145
#define MT5785_TX_FOP_STEP                   10

/* ping freq register */
#define MT5785_TX_PING_FREQ_ADDR             0X0054
#define MT5785_TX_PING_FREQ_LEN              2
#define MT5785_TX_PING_FREQ_STEP             10
#define MT5785_TX_PING_FREQ_MIN              100
#define MT5785_TX_PING_FREQ_MAX              180
#define MT5785_TX_PING_FREQ                  115
#define MT5785_TX_PING_FREQ_COIL_TEST        115
#define MT5785_TX_PING_FREQ_BAT_HEATING      100

/* over current register */
#define MT5785_TX_OCP_ADDR                   0x0058
#define MT5785_TX_OCP_LEN                    2
#define MT5785_TX_ILIM_MIN                   500
#define MT5785_TX_ILIM_MAX                   2000

/* max input current */
#define MT5785_TX_OCP1_ADDR                  0x005a
#define MT5785_TX_OCP1_LEN                   2
#define MT5785_TX_OCP1_TH                    2000

/* max input voltage threshold */
#define MT5785_TX_OVP_ADDR                   0x005e
#define MT5785_TX_OVP_LEN                    2
#define MT5785_TX_OVP_TH                     20000

/* min input voltage threshold */
#define MT5785_TX_LVP_ADDR                   0x0060
#define MT5785_TX_LVP_LEN                    2
#define MT5785_TX_LVP_TH                     3500

/* Digital Ping Interval */
#define MT5785_TX_PINT_INTERVAL_ADDR         0x0066
#define MT5785_TX_PINT_INTERVAL_LEN          2
#define MT5785_TX_PING_INTERVAL_MIN          0
#define MT5785_TX_PING_INTERVAL_MAX          1000
#define MT5785_TX_PING_INTERVAL              500
#define MT5785_TX_PING_INTERVAL_COIL_TEST    0
#define MT5785_TX_PING_INTERVAL_BAT_HEATING  0

/* Max Transmit Power */
#define MT5785_TX_MAX_POWER_ADDR             0x0068
#define MT5785_TX_MAX_POWER_LEN              2
#define MT5785_TX_RP_VAL_UNIT                2

/* Power loss */
#define MT5785_TX_PLOSS_ADDR                 0x0084
#define MT5785_TX_PLOSS_LEN                  4
#define MT5785_TX_PLOSS_CNT_VAL              3
#define MT5785_TX_PLOSS_TH0_VAL              5000
#define MT5785_TX_PLOSS_TH1_VAL              5000
#define MT5785_TX_PLOSS_TH2_VAL              5000
#define MT5785_TX_PLOSS_TH3_VAL              5000
#define MT5785_TX_Q_ONLY_COIL_TH             140
#define MT5785_TX_Q_TH                       110
#define MT5785_TX_FUNC_DIS                   0

/* ask packet header */
#define MT5785_TX_RCVD_MSG_HEADER_ADDR       0x0092
#define MT5785_TX_RCVD_MSG_HEADER_LEN        1

/* FSK Header */
#define MT5785_TX_SEND_MSG_HEADER_ADDR       0x00a8
#define MT5785_TX_SEND_MSG_HEADER_LEN        1

/* FSK Message0 */
#define MT5785_TX_SEND_MSG_CMD_ADDR          0x00a9
#define MT5785_TX_SEND_MSG_CMD_LEN           1

/* FSK Message1 */
#define MT5785_TX_SEND_MSG_DATA_ADDR         0x00aa
#define MT5785_TX_SEND_MSG_DATA_LEN          1

/* end power transfer type */
#define MT5785_TX_EPT_TYPE_ADDR              0x010c
#define MT5785_TX_EPT_TYPE_LEN               4
#define MT5785_TX_EPT_CLEAR                  0
#define MT5785_TX_EPT_RX                     BIT(5)
#define MT5785_TX_EPT_CEP_TIMEOUT            BIT(6)
#define MT5785_TX_EPT_RX_RST                 BIT(8)
#define MT5785_TX_EPT_FOD                    BIT(3)

/* control error packet value */
#define MT5785_TX_CEP_VAL_ADDR               0x0112
#define MT5785_TX_CEP_VAL_LEN                1
#define MT5785_TX_CEP_VAL_STEP               10

/*
 * firmware register
 */

/* chip id */
#define MT5785_MTP_CHIPID_ADDR               0x0008
#define MT5785_MTP_CHIPID_LEN                2
#define MT5785_MTP_CHIPID_VALUE              0x5785

/* program mtp */
#define MT5785_MTP_PGM_ADDR_ADDR             0x0002
#define MT5785_MTP_PGM_ADDR_LEN              2

/* program mtp len */
#define MT5785_MTP_PGM_LEN_ADDR              0x0004
#define MT5785_MTP_PGM_LEN_LEN               2

/* check sum */
#define MT5785_MTP_PGM_CHKSUM_ADDR           0x0006
#define MT5785_MTP_PGM_CHKSUM_LEN            2

/* mtp data */
#define MT5785_MTP_PGM_DATA_ADDR             0x0008
#define MT5785_MTP_BUFF_SIZE                 256

/* program cmd */
#define MT5785_MTP_PGM_CMD_ADDR              0x0000
#define MT5785_MTP_PGM_CMD_LEN               2
#define MT5785_MTP_PGM_CMD                   0x01
#define MT5785_MTP_PGM_SUCESS                0x02

/* crc address */
#define MT5785_MTP_CRC_ADDR                  0x0024
#define MT5785_MTP_CRC_LEN                   2

/* total mtp length */
#define MT5785_MTP_TOTAL_LENGTH_ADDR         0x0026
#define MT5785_MTP_TOTAL_LENGTH_LEN          2

/* load bootloader */
#define MT5785_BTLOADER_ADDR                 0x1800

/* fw crc calculation */
#define MT5785_MTP_CRC_SEED                  0x1021
#define MT5785_MTP_CRC_INIT                  0xffff
#define MT5785_MTP_CRC_HIGHEST_BIT           0x8000

/*
 * calibration
 */

/* cmd register */
#define MT5785_CALI_CMD_ADDR                  0x0004
#define MT5785_CALI_CMD_LEN                   2
#define MT5785_CALI_CMD_ZC_MASK               BIT(0)
#define MT5785_CALI_CMD_ZC_SHIFT              0
#define MT5785_CALI_CMD_WRITE_MTP_MASK        BIT(1)
#define MT5785_CALI_CMD_WRITE_MTP_SHIFT       1
#define MT5785_CALI_CMD_READ_IOUT_MASK        BIT(2)
#define MT5785_CALI_CMD_READ_IOUT_SHIFT       2
#define MT5785_CALI_CMD_RESET_MASK            BIT(3)
#define MT5785_CALI_CMD_RESET_SHIFT           3
#define MT5785_CALI_CMD_ENLDO_MASK            BIT(4)
#define MT5785_CALI_CMD_ENLDO_SHIFT           4
#define MT5785_CALI_CMD_DISLDO_MASK           BIT(5)
#define MT5785_CALI_CMD_DISLDO_SHIFT          5
#define MT5785_CALI_CMD_VOUTSET_MASK          BIT(6)
#define MT5785_CALI_CMD_VOUTSET_SHIFT         6
#define MT5785_CALI_CMD_EN                    1

/* status register: mtp write */
#define MT5785_CALI_STATUS_ADDR               0x0006
#define MT5785_CALI_STATUS_LEN                2
#define MT5785_CALI_STATUS_SUCC               BIT(1)
#define MT5785_CALI_STATUS_FAIL               BIT(0)

/* calibration para register */
#define MT5785_CALI_GAIN_ADDR                 0x000a
#define MT5785_CALI_GAIN_LEN                  2
#define MT5785_CALI_OFFSET_ADDR               0x000c
#define MT5785_CALI_OFFSET_LEN                2

/* 0 load current register */
#define MT5785_CALI_ILOAD_ADDR                0x001a
#define MT5785_CALI_ILOAD_LEN                 2

/* iout register */
#define MT5785_CALI_IOUT_ADDR                 0x0018
#define MT5785_CALI_IOUT_LEN                  2

/* vout register */
#define MT5785_CALI_VOUT_ADDR                 0x0064
#define MT5785_CALI_VOUT_LEN                  2

/* vrect register */
#define MT5785_CALI_VRECT_ADDR                0x0062
#define MT5785_CALI_VRECT_LEN                 2

/* vout register */
#define MT5785_CALI_VOUT_SET_ADDR             0x0066
#define MT5785_CALI_VOUT_SET_LEN              2

/* cnt register */
#define MT5785_CALI_CNT_ADDR                  0x0080
#define MT5785_CALI_CNT_LEN                   2

/* load bootloader cal */
#define MT5785_CALI_BTLOADER_ADDR             0x1000
#define MT5785_CALI_MTP_CHIPID_ADDR           0x0000
#define MT5785_CALI_MTP_CHIPID_LEN            2

#endif /* _MT5785_CHIP_H_ */
