/* SPDX-License-Identifier: GPL-2.0 */
/*
 * cps2021_ufcs.h
 *
 * cps2021 ufcs header file
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

#ifndef _CPS2021_UFCS_H_
#define _CPS2021_UFCS_H_

#include "cps2021.h"

#define CPS2021_UFCS_WAIT_RETRY_CYCLE                 80
#define CPS2021_UFCS_HANDSHARK_RETRY_CYCLE            10
#define CPS2021_UFCS_TX_BUF_WITHOUTHEAD_SIZE          34
#define CPS2021_UFCS_RX_BUF_SIZE                      125
#define CPS2021_UFCS_RX_BUF_WITHOUTHEAD_SIZE          123

/* CTL2 reg=0x41 */
#define CPS2021_UFCS_CTL2_REG                         0x41
#define CPS2021_UFCS_CTL2_DEV_ADDR_ID_MASK            BIT(1)
#define CPS2021_UFCS_CTL2_DEV_ADDR_ID_SHIFT           1
#define CPS2021_UFCS_CTL2_EN_DM_HIZ_MASK              BIT(0)
#define CPS2021_UFCS_CTL2_EN_DM_HIZ_SHIFT             0

#define CPS2021_UFCS_CTL2_SOURCE_ADDR                 0
#define CPS2021_UFCS_CTL2_CABLE_ADDR                  1

/* ISR1 reg=0x42 */
#define CPS2021_UFCS_ISR1_REG                         0x42
#define CPS2021_UFCS_ISR1_HANDSHAKE_FAIL_MASK         BIT(7)
#define CPS2021_UFCS_ISR1_HANDSHAKE_FAIL_SHIFT        7
#define CPS2021_UFCS_ISR1_HANDSHAKE_SUCC_MASK         BIT(6)
#define CPS2021_UFCS_ISR1_HANDSHAKE_SUCC_SHIFT        6
#define CPS2021_UFCS_ISR1_BAUD_RATE_ERROR_MASK        BIT(5)
#define CPS2021_UFCS_ISR1_BAUD_RATE_ERROR_SHIFT       5
#define CPS2021_UFCS_ISR1_CRC_ERROR_MASK              BIT(4)
#define CPS2021_UFCS_ISR1_CRC_ERROR_SHIFT             4
#define CPS2021_UFCS_ISR1_SEND_PACKET_COMPLETE_MASK   BIT(3)
#define CPS2021_UFCS_ISR1_SEND_PACKET_COMPLETE_SHIFT  3
#define CPS2021_UFCS_ISR1_DATA_READY_MASK             BIT(2)
#define CPS2021_UFCS_ISR1_DATA_READY_SHIFT            2
#define CPS2021_UFCS_ISR1_HARD_RESET_MASK             BIT(1)
#define CPS2021_UFCS_ISR1_HARD_RESET_SHIFT            1
#define CPS2021_UFCS_ISR1_ACK_REC_TIMEOUT_MASK        BIT(0)
#define CPS2021_UFCS_ISR1_ACK_REC_TIMEOUT_SHIFT       0

/* ISR2 reg=0x43 */
#define CPS2021_UFCS_ISR2_REG                         0x43
#define CPS2021_UFCS_ISR2_RX_BUF_OVERFLOW_MASK        BIT(6)
#define CPS2021_UFCS_ISR2_RX_BUF_OVERFLOW_SHIFT       6
#define CPS2021_UFCS_ISR2_RX_LEN_ERR_MASK             BIT(5)
#define CPS2021_UFCS_ISR2_RX_LEN_ERR_SHIFT            5
#define CPS2021_UFCS_ISR2_BAUD_RATE_CHANGE_MASK       BIT(4)
#define CPS2021_UFCS_ISR2_BAUD_RATE_CHANGE_SHIFT      4
#define CPS2021_UFCS_ISR2_FRAME_REC_TIMEOUT_MASK      BIT(3)
#define CPS2021_UFCS_ISR2_FRAME_REC_TIMEOUT_SHIFT     3
#define CPS2021_UFCS_ISR2_RX_BUF_BUSY_MASK            BIT(2)
#define CPS2021_UFCS_ISR2_RX_BUF_BUSY_SHIFT           2
#define CPS2021_UFCS_ISR2_MSG_TRANS_FAIL_MASK         BIT(1)
#define CPS2021_UFCS_ISR2_MSG_TRANS_FAIL_SHIFT        1
#define CPS2021_UFCS_ISR2_TRA_BYTE_ERR_MASK           BIT(0)
#define CPS2021_UFCS_ISR2_TRA_BYTE_ERR_SHIFT          0

/* MASK1 reg=0x44 */
#define CPS2021_UFCS_MASK1_REG                        0x44
#define CPS2021_UFCS_MASK1_HANDSHAKE_FAIL_MASK        BIT(7)
#define CPS2021_UFCS_MASK1_HANDSHAKE_FAIL_SHIFT       7
#define CPS2021_UFCS_MASK1_HANDSHAKE_SUCC_MASK        BIT(6)
#define CPS2021_UFCS_MASK1_HANDSHAKE_SUCC_SHIFT       6
#define CPS2021_UFCS_MASK1_BAUD_RATE_ERROR_MASK       BIT(5)
#define CPS2021_UFCS_MASK1_BAUD_RATE_ERROR_SHIFT      5
#define CPS2021_UFCS_MASK1_CRC_ERROR_MASK             BIT(4)
#define CPS2021_UFCS_MASK1_CRC_ERROR_SHIFT            4
#define CPS2021_UFCS_MASK1_SEND_PACKET_COMPLETE_MASK  BIT(3)
#define CPS2021_UFCS_MASK1_SEND_PACKET_COMPLETE_SHIFT 3
#define CPS2021_UFCS_MASK1_DATA_READY_MASK            BIT(2)
#define CPS2021_UFCS_MASK1_DATA_READY_SHIFT           2
#define CPS2021_UFCS_MASK1_HARD_RESET_MASK            BIT(1)
#define CPS2021_UFCS_MASK1_HARD_RESET_SHIFT           1
#define CPS2021_UFCS_MASK1_ACK_REC_TIMEOUT_MASK       BIT(0)
#define CPS2021_UFCS_MASK1_ACK_REC_TIMEOUT_SHIFT      0

/* MASK2 reg=0x45 */
#define CPS2021_UFCS_MASK2_REG                        0x45
#define CPS2021_UFCS_MASK2_RX_BUF_OVERFLOW_MASK       BIT(6)
#define CPS2021_UFCS_MASK2_RX_BUF_OVERFLOW_SHIFT      6
#define CPS2021_UFCS_MASK2_RX_LEN_ERR_MASK            BIT(5)
#define CPS2021_UFCS_MASK2_RX_LEN_ERR_SHIFT           5
#define CPS2021_UFCS_MASK2_BAUD_RATE_CHANGE_MASK      BIT(4)
#define CPS2021_UFCS_MASK2_BAUD_RATE_CHANGE_SHIFT     4
#define CPS2021_UFCS_MASK2_FRAME_REC_TIMEOUT_MASK     BIT(3)
#define CPS2021_UFCS_MASK2_FRAME_REC_TIMEOUT_SHIFT    3
#define CPS2021_UFCS_MASK2_RX_BUF_BUSY_MASK           BIT(2)
#define CPS2021_UFCS_MASK2_RX_BUF_BUSY_SHIFT          2
#define CPS2021_UFCS_MASK2_MSG_TRANS_FAIL_MASK        BIT(1)
#define CPS2021_UFCS_MASK2_MSG_TRANS_FAIL_SHIFT       1
#define CPS2021_UFCS_MASK2_TRA_BYTE_ERR_MASK          BIT(0)
#define CPS2021_UFCS_MASK2_TRA_BYTE_ERR_SHIFT         0

/* TX_LENGTH reg=0x47 */
#define CPS2021_UFCS_TX_LENGTH_REG                    0x47

/* TX_BUFFER reg=0x4A */
#define CPS2021_UFCS_TX_BUFFER_REG                    0x48

/* RX_LENGTH reg=0x6C */
#define CPS2021_UFCS_RX_LENGTH_REG                    0x6C

/* RX_BUFFER reg=0x6D */
#define CPS2021_UFCS_RX_BUFFER_REG                    0x6D

struct cps2021_ufcs_msg_node {
	u8 irq1;
	u8 irq2;
	int len;
	u8 data[CPS2021_UFCS_RX_BUF_SIZE];
	struct cps2021_ufcs_msg_node *next;
};

struct cps2021_ufcs_msg_head {
	int num;
	struct cps2021_ufcs_msg_node *next;
};

int cps2021_ufcs_ops_register(struct cps2021_device_info *di);
void cps2021_ufcs_work(struct work_struct *work);
int cps2021_ufcs_init_msg_head(struct cps2021_device_info *di);
void cps2021_ufcs_free_node_list(struct cps2021_device_info *di);

#endif /* _CPS2021_UFCS_H_ */
