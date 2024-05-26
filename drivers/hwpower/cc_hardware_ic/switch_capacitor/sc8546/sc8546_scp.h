/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sc8546_scp.h
 *
 * sc8546 scp header file
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

#ifndef _SC8546_SCP_H_
#define _SC8546_SCP_H_

#include "sc8546.h"

/* scp */
#define SC8546_SCP_ACK_RETRY_TIME              3
#define SC8546_SCP_RESTART_TIME                4
#define SC8546_SCP_RETRY_TIME                  3
#define SC8546_SCP_POLL_TIME                   100 /* ms */
#define SC8546_SCP_DETECT_MAX_CNT              10
#define SC8546_SCP_PING_DETECT_MAX_CNT         15
#define SC8546_SCP_NO_ERR                      0
#define SC8546_SCP_IS_ERR                      1
#define SC8546_SCP_WRITE_OP                    0
#define SC8546_SCP_READ_OP                     1
#define SC8546_SCP_MULTI_READ_LEN              2
#define SC8546_SCP_ACK_AND_CRC_LEN             2
#define SC8546_SCP_SBRWR_NUM                   1
#define SC8546_SCP_CMD_SBRRD                   0x0c
#define SC8546_SCP_CMD_SBRWR                   0x0b
#define SC8546_SCP_CMD_MBRRD                   0x1c
#define SC8546_SCP_CMD_MBRWR                   0x1b
#define SC8546_SCP_ACK                         0x08
#define SC8546_SCP_NACK                        0x03

/* fcp adapter vol value */
#define SC8546_FCP_ADAPTER_MAX_VOL             12000
#define SC8546_FCP_ADAPTER_MIN_VOL             5000
#define SC8546_FCP_ADAPTER_RST_VOL             5000
#define SC8546_FCP_ADAPTER_ERR_VOL             500
#define SC8546_FCP_ADAPTER_VOL_CHECK_TIME      10

/* DPDM_CTRL2 reg=0x22 */
#define SC8546_DPDM_CTRL2_REG                  0x22
#define SC8546_DPDM_OVP_DIS_MASK               BIT(6)
#define SC8546_DPDM_OVP_DIS_SHIFT              6
#define SC8546_DM_BUF_MASK                     (BIT(5) | BIT(4))
#define SC8546_DM_BUF_SHIFT                    4
#define SC8546_DP_BUF_MASK                     (BIT(3) | BIT(2))
#define SC8546_DP_BUF_SHIFT                    2
#define SC8546_DM_SINK_EN_MASK                 BIT(3)
#define SC8546_DM_SINK_EN_SHIFT                3
#define SC8546_DP_SINK_EN_MASK                 BIT(2)
#define SC8546_DP_SINK_EN_SHIFT                2
#define SC8546_DM_BUFF_EN_MASK                 BIT(1)
#define SC8546_DM_BUFF_EN_SHIFT                1
#define SC8546_DP_BUFF_EN_MASK                 BIT(0)
#define SC8546_DP_BUFF_EN_SHIFT                0

/* DPDM_STAT reg=0x23 */
#define SC8546_DPDM_STAT_REG                   0x23
#define SC8546_VDM_RD_MASK                     (BIT(5) | BIT(4) | BIT(3))
#define SC8546_VDM_RD_SHIFT                    3
#define SC8546_VDP_RD_MASK                     (BIT(2) | BIT(1) | BIT(0))
#define SC8546_VDP_RD_SHIFT                    0
#define SC8546_VDPDM_2V                        5
#define SC8546_VDPDM_3V                        6
#define SC8546_VDPDM_DETECT_SUCC_VAL           0x1
#define SC8546_VDM_RD_0_325                    0
#define SC8546_VDP_RD_425_1000                 2

/* DPDM_FLAG_MASK reg=0x24 */
#define SC8546_DPDM_FLAG_MASK_REG              0x24
#define SC8546_DM_LOW_MSK_MASK                 BIT(6)
#define SC8546_DM_LOW_MSK_SHIFT                6
#define SC8546_DM_LOW_FLAG_MASK                BIT(5)
#define SC8546_DM_LOW_FLAG_SHIFT               5
#define SC8546_DP_LOW_MSK_MASK                 BIT(4)
#define SC8546_DP_LOW_MSK_SHIFT                4
#define SC8546_DP_LOW_FLAG_MASK                BIT(3)
#define SC8546_DP_LOW_FLAG_SHIFT               3
#define SC8546_DPDM_OVP_MSK_MASK               BIT(2)
#define SC8546_DPDM_OVP_MASK_SHIFT             2
#define SC8546_DPDM_OVP_FLAG_MASK              BIT(1)
#define SC8546_DPDM_OVP_FLAG_SHIFT             1
#define SC8546_DPDM_OVP_STAT_MASK              BIT(0)
#define SC8546_DPDM_OVP_STAT_SHIFT             0

/* SCP_WDATA reg=0x26 */
#define SC8546_SCP_TX_DATA_REG                 0x26
#define SC8546_SCP_TX_DATA_RST                 0

/* SCP_RDATA reg=0x27 */
#define SC8546_SCP_RX_DATA_REG                 0x27
#define SC8546_SCP_RX_DATA_RST                 0

/* SCP_FIFO_STAT reg=0x28 */
#define SC8546_SCP_FIFO_STAT_REG               0x28
#define SC8546_DM_BUSY_STAT_MASK               BIT(7)
#define SC8546_DM_BUSY_STAT_SHIFT              7
#define SC8546_TX_FIFO_CNT_STAT_MASK           (BIT(5) | BIT(4) | BIT(3))
#define SC8546_TX_FIFO_CNT_STAT_SHIFT          3
#define SC8546_RX_FIFO_CNT_STAT_MASK           (BIT(2) | BIT(1) | BIT(0))
#define SC8546_RX_FIFO_CNT_STAT_SHIFT          0
#define SC8546_SCP_FIFO_DEEPTH                 5

/* SCP_STAT reg=0x29 */
#define SC8546_SCP_STAT_REG                    0x29
#define SC8546_FIRST_PING_DET_STAT             0x78
#define SC8546_NO_1ST_SLAVE_PING_STAT_MASK     BIT(7)
#define SC8546_NO_1ST_SLAVE_PING_STAT_SHIFT    7
#define SC8546_NO_MED_SLAVE_PING_STAT_MASK     BIT(6)
#define SC8546_NO_MED_SLAVE_PING_STAT_SHIFT    6
#define SC8546_NO_LAST_SLAVE_PING_STAT_MASK    BIT(5)
#define SC8546_NO_LAST_SLAVE_PING_STAT_SHIFT   5
#define SC8546_NO_RX_PKT_STAS_MASK             BIT(4)
#define SC8546_NO_RX_PKT_STAS_SHIFT            4
#define SC8546_NO_TX_PKT_STAT_MASK             BIT(3)
#define SC8546_NO_TX_PKT_STAT_SHIFT            3
#define SC8546_WDT_EXPIRED_STAT_MASK           BIT(2)
#define SC8546_WDT_EXPIRED_STAT_SHIFT          2
#define SC8546_RX_CRC_ERR_STAT_MASK            BIT(1)
#define SC8546_RX_CRC_ERR_STAT_SHIFT           1
#define SC8546_RX_PAR_ERR_STAT_MASK            BIT(0)
#define SC8546_RX_PAR_ERR_STAT_SHIFT           0

/* SCP_FLAG_MASK reg=0x2A */
#define SC8546_SCP_FLAG_MASK_REG               0x2A
#define SC8546_SCP_FLAG_MASK_REG_INIT          0x20
#define SC8546_TX_FIFO_EMPTY_MSK_MASK          BIT(5)
#define SC8546_TX_FIFO_EMPTY_MSK_SHIFT         5
#define SC8546_TX_FIFO_EMPTY_FLAG_MASK         BIT(4)
#define SC8546_TX_FIFO_EMPTY_FLAG_SHIFT        4
#define SC8546_RX_FIFO_FULL_MSK_MASK           BIT(3)
#define SC8546_RX_FIFO_FULL_MSK_SHIFT          3
#define SC8546_RX_FIFO_FULL_FLAG_MASK          BIT(2)
#define SC8546_RX_FIFO_FULL_FLAG_SHIFT         2
#define SC8546_TRANS_DONE_MSK_MASK             BIT(1)
#define SC8546_TRANS_DONE_MSK_SHIFT            1
#define SC8546_TRANS_DONE_FLAG_MASK            BIT(0)
#define SC8546_TRANS_DONE_FLAG_SHIFT           0

int sc8546_protocol_ops_register(struct sc8546_device_info *di);

#endif /* _SC8546_SCP_H_ */
