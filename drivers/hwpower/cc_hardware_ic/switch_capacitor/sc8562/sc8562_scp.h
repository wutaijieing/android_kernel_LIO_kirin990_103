/* SPDX-License-Identifier: GPL-2.0 */
/*
 * SC8562_protocol.h
 *
 * SC8562 protocol header file
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

#ifndef _SC8562_SCP_H_
#define _SC8562_SCP_H_

#include "sc8562.h"

/* scp */
#define SC8562_SCP_ACK_RETRY_TIME                 3
#define SC8562_SCP_RESTART_TIME                   4
#define SC8562_SCP_RETRY_TIME                     3
#define SC8562_SCP_POLL_TIME                      100 /* ms */
#define SC8562_SCP_DETECT_MAX_CNT                 10
#define SC8562_SCP_PING_DETECT_MAX_CNT            15
#define SC8562_SCP_NO_ERR                         0
#define SC8562_SCP_IS_ERR                         1
#define SC8562_SCP_WRITE_OP                       0
#define SC8562_SCP_READ_OP                        1
#define SC8562_SCP_MULTI_READ_LEN                 2
#define SC8562_SCP_ACK_AND_CRC_LEN                2
#define SC8562_SCP_SBRWR_NUM                      1
#define SC8562_SCP_CMD_SBRRD                      0x0c
#define SC8562_SCP_CMD_SBRWR                      0x0b
#define SC8562_SCP_CMD_MBRRD                      0x1c
#define SC8562_SCP_CMD_MBRWR                      0x1b
#define SC8562_SCP_ACK                            0x08
#define SC8562_SCP_NACK                           0x03

/* fcp adapter vol value */
#define SC8562_FCP_ADAPTER_MAX_VOL                12000
#define SC8562_FCP_ADAPTER_MIN_VOL                5000
#define SC8562_FCP_ADAPTER_RST_VOL                5000
#define SC8562_FCP_ADAPTER_ERR_VOL                500
#define SC8562_FCP_ADAPTER_VOL_CHECK_TIME         10

/* DPDM_CTRL1 reg=0x2A */
#define SC8562_DPDM_CTRL1_REG                     0x2A
#define SC8562_DM_500K_PD_EN_MASK                 BIT(7)
#define SC8562_DM_500K_PD_EN_SHIFT                7
#define SC8562_DP_500K_PD_EN_MASK                 BIT(6)
#define SC8562_DP_500K_PD_EN_SHIFT                6
#define SC8562_DM_20K_PD_EN_MASK                  BIT(5)
#define SC8562_DM_20K_PD_EN_SHIFT                 5
#define SC8562_DP_20K_PD_EN_MASK                  BIT(4)
#define SC8562_DP_20K_PD_EN_SHIFT                 4
#define SC8562_DM_SINK_EN_MASK                    BIT(3)
#define SC8562_DM_SINK_EN_SHIFT                   3
#define SC8562_DP_SINK_EN_MASK                    BIT(2)
#define SC8562_DP_SINK_EN_SHIFT                   2
#define SC8562_DP_SRC_10UA_MASK                   BIT(1)
#define SC8562_DP_SRC_10UA_SHIFT                  1
#define SC8562_DPDM_EN_MASK                       BIT(0)
#define SC8562_DPDM_EN_SHIFT                      0

/* DPDM_CTRL2 reg=0x2B */
#define SC8562_DPDM_CTRL2_REG                     0x2B
#define SC8562_DM_3P3_EN_MASK                     BIT(7)
#define SC8562_DM_3P3_EN_SHIFT                    7
#define SC8562_DPDM_OVP_DIS_MASK                  BIT(6)
#define SC8562_DPDM_OVP_DIS_SHIFT                 6
#define SC8562_DM_BUF_MASK                        (BIT(5) | BIT(4))
#define SC8562_DM_BUF_SHIFT                       4
#define SC8562_DP_BUF_MASK                        (BIT(3) | BIT(2))
#define SC8562_DP_BUF_SHIFT                       2
#define SC8562_DM_BUFF_EN_MASK                    BIT(1)
#define SC8562_DM_BUFF_EN_SHIFT                   1
#define SC8562_DP_BUFF_EN_MASK                    BIT(0)
#define SC8562_DP_BUFF_EN_SHIFT                   0
#define SC8562_DP_BUFF_EN_ENABLE                  1

/* DPDM_STAT reg=0x2C */
#define SC8562_DPDM_STAT_REG                      0x2C
#define SC8562_DPDM_STAT_VDM_RD_MASK              (BIT(5) | BIT(4) | BIT(3))
#define SC8562_DPDM_STAT_VDM_RD_SHIFT             3
#define SC8562_DPDM_STAT_VDP_RD_MASK              (BIT(2) | BIT(1) | BIT(0))
#define SC8562_DPDM_STAT_VDP_RD_SHIFT             0
#define SC8562_VDPDM_DETECT_SUCC_VAL              0x1

/* DPDM_FLAG_MASK reg=0x2D */
#define SC8562_DPDM_FLAG_MASK_REG                 0x2D
#define SC8562_DM_LOW_MASK_MASK                   BIT(6)
#define SC8562_DM_LOW_MASK_SHIFT                  6
#define SC8562_DM_LOW_FLAG_MASK                   BIT(5)
#define SC8562_DM_LOW_FLAG_SHIFT                  5
#define SC8562_DP_LOW_MASK_MASK                   BIT(4)
#define SC8562_DP_LOW_MASK_SHIFT                  4
#define SC8562_DP_LOW_FLAG_MASK                   BIT(3)
#define SC8562_DP_LOW_FLAG_SHIFT                  3
#define SC8562_DPDM_OVP_MASK_MASK                 BIT(2)
#define SC8562_DPDM_OVP_MASK_SHIFT                2
#define SC8562_DPDM_OVP_FLAG_MASK                 BIT(1)
#define SC8562_DPDM_OVP_FLAG_SHIFT                1
#define SC8562_DPDM_OVP_STAT_MASK                 BIT(0)
#define SC8562_DPDM_OVP_STAT_SHIFT                0

/* SCP_CTRL reg=0x2E */
#define SC8562_SCP_CTRL_REG                       0x2E
#define SC8562_SCP_CTRL_SCP_EN_MASK               BIT(7)
#define SC8562_SCP_CTRL_SCP_EN_SHIFT              7
#define SC8562_SCP_CTRL_SCP_SOFT_RST_MASK         BIT(6)
#define SC8562_SCP_CTRL_SCP_SOFT_RST_SHIFT        6
#define SC8562_SCP_CTRL_TX_CRC_DIS_MASK           BIT(5)
#define SC8562_SCP_CTRL_TX_CRC_DIS_SHIFT          5
#define SC8562_SCP_CTRL_CLR_TX_FIFO_MASK          BIT(3)
#define SC8562_SCP_CTRL_CLR_TX_FIFO_SHIFT         3
#define SC8562_SCP_CTRL_CLR_RX_FIFO_MASK          BIT(2)
#define SC8562_SCP_CTRL_CLR_RX_FIFO_SHIFT         2
#define SC8562_SCP_CTRL_SND_RST_TRANS_MASK        BIT(1)
#define SC8562_SCP_CTRL_SND_RST_TRANS_SHIFT       1
#define SC8562_SCP_CTRL_SND_TRANS_MASK            BIT(0)
#define SC8562_SCP_CTRL_SND_TRANS_SHIFT           0

/* SCP_WDATA reg=0x2F */
#define SC8562_SCP_WDATA_REG                      0x2F
#define SC8562_SCP_WDATA_TX_DATA_MASK             0xFF
#define SC8562_SCP_WDATA_TX_DATA_SHIFT            0

/* SCP_RDATA reg=0x30 */
#define SC8562_SCP_RDATA_REG                      0x30
#define SC8562_SCP_RDATA_RX_DATA_MASK             0xFF
#define SC8562_SCP_RDATA_RX_DATA_SHIFT            0

/* SCP_FIFO_STAT reg=0x31 */
#define SC8562_SCP_FIFO_STAT_REG                  0x31
#define SC8562_TX_FIFO_CNT_STAT_MASK              (BIT(7) | BIT(6) | BIT(5) | BIT(4))
#define SC8562_TX_FIFO_CNT_STAT_SHIFT             4
#define SC8562_RX_FIFO_CNT_STAT_MASK              (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define SC8562_RX_FIFO_CNT_STAT_SHIFT             0

/* SCP_STAT reg=0x32 */
#define SC8562_SCP_STAT_REG                       0x32
#define SC8562_NO_FIRST_SLAVE_PING_STAT_MASK      BIT(7)
#define SC8562_NO_FIRST_SLAVE_PING_STAT_SHIFT     7
#define SC8562_NO_MED_SLAVE_PING_STAT_MASK        BIT(6)
#define SC8562_NO_MED_SLAVE_PING_STAT_SHIFT       6
#define SC8562_NO_LAST_SLAVE_PING_STAT_MASK       BIT(5)
#define SC8562_NO_LAST_SLAVE_PING_STAT_SHIFT      5
#define SC8562_NO_RX_PKT_STAT_MASK                BIT(4)
#define SC8562_NO_RX_PKT_STAT_SHIFT               4
#define SC8562_NO_TX_PKT_STAT_MASK                BIT(3)
#define SC8562_NO_TX_PKT_STAT_SHIFT               3
#define SC8562_WDT_EXPIRED_STAT_MASK              BIT(2)
#define SC8562_WDT_EXPIRED_STAT_SHIFT             2
#define SC8562_RX_CRC_ERR_STAT_MASK               BIT(1)
#define SC8562_RX_CRC_ERR_STAT_SHIFT              1
#define SC8562_RX_PAR_ERR_STAT_MASK               BIT(0)
#define SC8562_RX_PAR_ERR_STAT_SHIFT              0

/* SCP_FLAG_MASK reg=0x33 */
#define SC8562_SCP_FLAG_MASK_REG                  0x33
#define SC8562_SCP_FLAG_MASK_REG_INIT             0x20
#define SC8562_DM_BUSY_STAT_MASK                  BIT(7)
#define SC8562_DM_BUSY_STAT_SHIFT                 7
#define SC8562_TX_FIFO_EMPTY_MASK_MASK            BIT(5)
#define SC8562_TX_FIFO_EMPTY_MASK_SHIFT           5
#define SC8562_TX_FIFO_EMPTY_FLAG_MASK            BIT(4)
#define SC8562_TX_FIFO_EMPTY_FLAG_SHIFT           4
#define SC8562_RX_FIFO_FULL_MASK_MASK             BIT(3)
#define SC8562_RX_FIFO_FULL_MASK_SHIFT            3
#define SC8562_RX_FIFO_FULL_FLAG_MASK             BIT(2)
#define SC8562_RX_FIFO_FULL_FLAG_SHIFT            2
#define SC8562_TRANS_DONE_MASK_MASK               BIT(1)
#define SC8562_TRANS_DONE_MASK_SHIFT              1
#define SC8562_TRANS_DONE_FLAG_MASK               BIT(0)
#define SC8562_TRANS_DONE_FLAG_SHIFT              0

int sc8562_protocol_ops_register(struct sc8562_device_info *di);

#endif /* _SC8562_SCP_H_ */
