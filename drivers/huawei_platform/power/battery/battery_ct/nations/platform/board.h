/* SPDX-License-Identifier: GPL-2.0 */
/*
 * board.h
 *
 * board head file for board oprate and swi bus init
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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

#ifndef _BOARD_H_
#define _BOARD_H_

#include <linux/types.h>
#include "../ns3300.h"

#define TIME_BASE 10
#define NS3300_LOOP_INTERVAL 166 /* 166ns */

int swi_init(struct ns3300_dev *dev);
void timing_init(struct ns3300_dev *dev);
uint8_t get_pin(void);
void set_pin(uint8_t level);
void set_pin_dir(uint8_t dir, uint8_t level);
void ns3300_udelay(uint32_t ul_ticks);
void ns3300_ndelay(uint32_t ns);

#endif /* _BOARD_H_ */
