/* SPDX-License-Identifier: GPL-2.0 */
/*
 * stwlc33_chip.h
 *
 * stwlc33 registers, chip info, etc.
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

#ifndef _STWLC33_CHIP_H_
#define _STWLC33_CHIP_H_

/* chip id register */
#define STWLC33_CHIP_ID_ADDR                  0x0000
#define STWLC33_CHIP_ID                       0x21
#define ERR_CHIP_ID                           0
/* interrupt status register */
#define STWLC33_TX_INT_STATUS_ADDR            0x0034
#define STWLC33_TX_STATUS_EPT_TYPE            BIT(0) /* end power transfer */
#define STWLC33_TX_STATUS_START_DPING         BIT(1) /* start digital ping */
#define STWLC33_TX_STATUS_GET_SS              BIT(2) /* get signal strength */
#define STWLC33_TX_STATUS_GET_ID              BIT(3) /* get id packet */
#define STWLC33_TX_STATUS_GET_CFG             BIT(4) /* get configure packet */
#define STWLC33_TX_STATUS_GET_PPP             BIT(5) /* get ppp */
/* system operating mode register */
#define STWLC33_SYS_MODE_ADDR                 0x004d
#define STWLC33_OTPONLY_MODE                  0x10
#define STWLC33_WPC_MODE                      0x01
#define STWLC33_PMA_MODE                      0x02
#define STWLC33_TX_WPCMODE                    0x04
#define STWLC33_BACKPOWERED                   0x08
#define STWLC33_RAMPROGRAM_MODE               0x40
/* send message register */
#define STWLC33_RX_TO_TX_HEADER_ADDR          0x0050
#define STWLC33_RX_TO_TX_CMD_ADDR             0x0051
#define STWLC33_RX_TO_TX_DATA_ADDR            0x0052
#define STWLC33_RX_TO_TX_DATA_LEN             4
#define STWLC33_RX_TO_TX_PACKET_LEN           6
#define STWLC33_RX_TO_TX_PACKET_BUFF_LEN      8
#define STWLC33_RX_TO_TX_PACKET_STR_LEN       64
/* clear interrupt register */
#define STWLC33_TX_INT_CLEAR_ADDR             0x0056
#define STWLC33_TX_INT_CLEAR_LEN              2
/* received message register */
#define STWLC33_TX_TO_RX_HEADER_ADDR          0x0058
#define STWLC33_TX_TO_RX_CMD_ADDR             0x0059
#define STWLC33_TX_TO_RX_DATA_ADDR            0x005a
#define STWLC33_TX_TO_RX_MESSAGE_LEN          5
#define STWLC33_TX_TO_RX_DATA_LEN             4
/* stwlc33 command */
#define STWLC33_CMD_GET_TX_ID                 0x3b
#define STWLC33_CMD_GET_TX_CAP                0x41
#define STWLC33_CMD_SEND_CHRG_STATE           0x43
#define STWLC33_CMD_RX_TO_TX_BT_MAC           0x52
#define STWLC33_CMD_RX_TO_TX_BT_MAC2          0x53
#define STWLC33_CMD_RX_TO_TX_BT_MODEL_ID      0x54
#define STWLC33_CMD_ACK                       0xff
#define STWLC33_CMD_ACK_HEAD                  0x1e
#define STWLC33_TX_ID_LEN                     3
#define STWLC33_CHRG_STATE_LEN                1
/* tx fop register */
#define STWLC33_TX_FOP_ADDR                   0x0062
#define STWLC33_TX_MAX_FOP_ADDR               0x0064
#define STWLC33_TX_MAX_FOP_VAL                148 /* kHz */
#define STWLC33_TX_MIN_FOP_ADDR               0x0066
#define STWLC33_TX_MIN_FOP_VAL                110 /* kHz */
/* tx ping frequency register */
#define STWLC33_TX_PING_FREQUENCY_ADDR        0x0068
#define STWLC33_TX_PING_FREQUENCY_MIN         127 /* kHz */
#define STWLC33_TX_PING_FREQUENCY_MAX         135 /* kHz */
#define STWLC33_TX_PING_FREQUENCY_INIT        135
/* tx ocp register */
#define STWLC33_TX_OCP_ADDR                   0x006a
#define STWLC33_TX_OCP_VAL                    2000
/* tx ovp register */
#define STWLC33_TX_OVP_ADDR                   0x006c
#define STWLC33_TX_OVP_VAL                    7500
/* tx iin register */
#define STWLC33_TX_IIN_ADDR                   0x006e
#define STWLC33_TX_IIN_LEN                    2
/* tx vin register */
#define STWLC33_TX_VIN_ADDR                   0x0070
#define STWLC33_TX_VIN_LEN                    2
/* tx vrect register */
#define STWLC33_TX_VRECT_ADDR                 0x0072
#define STWLC33_TX_VRECT_LEN                  2
/* tx ept type register */
#define STWLC33_TX_EPT_TYPE_ADDR              0X0074
#define STWLC33_TX_EPT_CMD                    BIT(0)
#define STWLC33_TX_EPT_SS                     BIT(1)
#define STWLC33_TX_EPT_ID                     BIT(2)
#define STWLC33_TX_EPT_XID                    BIT(3)
#define STWLC33_TX_EPT_CFG_COUNT_ERR          BIT(4)
#define STWLC33_TX_EPT_PCH                    BIT(5)
#define STWLC33_TX_EPT_FIRSTCEP               BIT(6)
#define STWLC33_TX_EPT_TIMEOUT                BIT(7)
#define STWLC33_TX_EPT_CEP_TIMEOUT            BIT(8)
#define STWLC33_TX_EPT_EPT_RPP_TIMEOUT        BIT(9)
#define STWLC33_TX_EPT_OCP                    BIT(10)
#define STWLC33_TX_EPT_OVP                    BIT(11)
#define STWLC33_TX_EPT_LVP                    BIT(12)
#define STWLC33_TX_EPT_FOD                    BIT(13)
#define STWLC33_TX_EPT_OTP                    BIT(14)
#define STWLC33_TX_EPT_LCP                    BIT(15)
/* command register */
#define STWLC33_CMD3_ADDR                     0x0076
#define STWLC33_CMD3_VAL                      1
#define STWLC33_CMD3_TX_EN_MASK               BIT(0)
#define STWLC33_CMD3_TX_EN_SHIFT              0
#define STWLC33_CMD3_TX_CLRINT_MASK           BIT(1)
#define STWLC33_CMD3_TX_CLRINT_SHIFT          1
#define STWLC33_CMD3_TX_DIS_MASK              BIT(2)
#define STWLC33_CMD3_TX_DIS_SHIFT             2
#define STWLC33_CMD3_TX_SEND_FSK_MASK         BIT(3)
#define STWLC33_CMD3_TX_SEND_FSK_SHIFT        3
#define STWLC33_CMD3_TX_FOD_EN_MASK           BIT(5)
#define STWLC33_CMD3_TX_FOD_EN_SHIFT          5
/* tx ping interval register */
#define STWLC33_TX_PING_INTERVAL_ADDR         0x0079
#define STWLC33_TX_PING_INTERVAL_MIN          50 /* ms */
#define STWLC33_TX_PING_INTERVAL_MAX          1000 /* ms */
#define STWLC33_TX_PING_INTERVAL_INIT         50 /* ms */
#define STWLC33_TX_PING_INTERVAL_STEP         10
/* tx chip temp register */
#define STWLC33_CHIP_TEMP_ADDR                0x007A
#define STWLC33_CHIP_TEMP_LEN                 1
/* tx ping dynamic frequency register */
#define STWLC33_TX_PING_FREQUENCY_ADDR_DYM    0x0084
#define STWLC33_TX_PING_FREQUENCY_INIT_DYM    125 /* kHz */
/* tx max freq register */
#define STWLC33_TX_MAX_FREQ_ADDR              0x0085
#define STWLC33_TX_MAX_FREQ                   142
#define STWLC33_TX_MAX_FREQ_DEFAULT           148
/* tx min freq register */
#define STWLC33_TX_MIN_FREQ_ADDR              0x0086
#define STWLC33_TX_MIN_FREQ                   130
#define STWLC33_TX_MIN_FREQ_DEFAULT           120
/* tx gpio_b2 register */
#define STWLC33_TX_GPIO_B2_ADDR               0x0087
#define STWLC33_TX_MAX_IIN_AVG                430 /* > 430mA, 3c */
#define STWLC33_TX_MIN_IIN_AVG                410 /* < 410mA, 1.5c */
#define STWLC33_TX_GPIO_OPEN                  0
#define STWLC33_TX_GPIO_PU                    1
#define STWLC33_TX_GPIO_PD                    2
#define STWLC33_TX_VOLT_5V5                   5500
#define STWLC33_TX_VOLT_4V8                   4850
/* tx fod th register */
#define STWLC33_TX_FOD_THD0_ADDR              0x00B2
#define STWLC33_TX_FOD_THD0_VAL               300
#define STWLC33_QI_SIGNAL_STRENGTH            0x01
#define STWLC33_TX_ADAPTER_OTG                0x09
#define STWLC33_TX_ADAPTER_OTG_MAX_VOL        50
#define STWLC33_TX_ADAPTER_OTG_MAX_CUR        5
/* chip reset register */
#define STWLC33_CHIP_RST_ADDR                 0x0188
#define STWLC33_CHIP_RST_VAL                  0x02
#define STWLC33_TX_PING_EN_MASK               BIT(0)
#define STWLC33_TX_PING_EN_SHIFT              0
#define STWLC33_TX_PING_EN                    1
/* tx fod register */
#define STWLC33_TX_FOD_PL_ADDR                0x01A7
#define STWLC33_TX_FOD_PL_VAL                 25  /* unit 100mW */
/* otp firmware version register */
#define STWLC33_OTP_FW_VERSION_ADDR           0x0008
#define STWLC33_OTP_FW_VERSION_LEN            2
#define STWLC33_OTP_FW_VERSION_STRING_LEN     32
#define STWLC33_TMP_BUFF_LEN                  32
/* otp firmware version */
#define STWLC33_OTP_FW_HEAD                   "0x88 0x66"
#define STWLC33_OTP_FW_VERSION_1100H          "0x88 0x66 0x02 0x17"
#define STWLC33_OTP_FW_VERSION_1100H_CUT23    "0x88 0x66 0x03 0x07"
/* ram version register */
#define STWLC33_RAM_VER_ADDR                  0x000e
#define STWLC33_RAM_VER_LEN                   2
/* nvm data register */
#define STWLC33_NVM_ADDR                      0x0100
#define STWLC33_NVM_SEC_NO_MAX                15
#define STWLC33_SEC_NO_SIZE                   4
#define STWLC33_NVM_REG_SIZE                  6
#define STWLC33_NVM_VALUE_SIZE                128
#define STWLC33_NVM_WR_TIME                   40
#define STWLC33_NVM_SEC_VAL_SIZE              0x20
#define STWLC33_NVM_RD_CMD                    0x20
#define STWLC33_NVM_WR_CMD                    0x40
#define STWLC33_NVM_PROGRAMED                 1
#define STWLC33_NVM_NON_PROGRAMED             0
#define STWLC33_NVM_ERR_PROGRAMED             2
/* sram update register */
#define STWLC33_SRAMUPDATE_ADDR               0x0100
#define STWLC33_OFFSET_REG_ADDR               0x0180
#define STWLC33_OFFSET_VALUE_SIZE             4
#define STWLC33_OFFSET_REG_SIZE               6
#define STWLC33_SRAM_SIZE_ADDR                0x0184
#define STWLC33_CMD_STATUS_ADDR               0x0185
#define STWLC33_ACT_CMD_ADDR                  0x0186
#define STWLC33_PAGE_SIZE                     128
#define STWLC33_ADDR_LEN                      2
#define STWLC33_ADDR_LEN_U8                   1
#define STWLC33_READ_CMD_VALUE                0x01
#define STWLC33_WRITE_CMD_VALUE               0x02
#define STWLC33_EXEC_CMD_VALUE                0x04
#define STWLC33_SRAM_EXEC_TIME                20
/* tx nvm register */
#define STWLC33_TX_NVM_ADDR                   0x0100
#define STWLC33_TX_NVM_VAL                    0xaa
#define STWLC33_TX_I2C_PAGE1_ADDR             0xff
#define STWLC33_TX_I2C_PAGE1_VAL              0x01
#define STWLC33_TX_16BIT_ADDR                 0x8e
#define STWLC33_TX_16BIT_VAL                  0x01

#endif /* _STWLC33_CHIP_H_ */
