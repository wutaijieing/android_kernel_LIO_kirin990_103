/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ns3300_swi.h
 *
 * ns3300 swi bus head file
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

#ifndef _NS3300_SWI_H_
#define _NS3300_SWI_H_

#include <linux/types.h>

#define SWI_BC              (0x04u)    /* Bus Command */
#define SWI_WD              (0x05u)    /* Write Data */
#define SWI_WRA             (0x08u)    /* Write Register Address */
#define SWI_RRA             (0x0au)    /* Read Register Address */

/* Bus Command */
#define SWI_SFCD            (0x00u)    /* Bus Reset */
#define SWI_BRES            (0x01u)    /* Bus Reset */
#define SWI_PDWN            (0x02u)    /* Bus Reset */
#define SWI_ECCE            (0x03u)    /* Bus Reset */
#define SWI_CLEC            (0x04u)    /* Bus Reset */

void swi_power_down(void);
void swi_power_up(void);

/* SWI Low Level Functions */
bool swi_write_byte(uint16_t uw_address, uint8_t ubp_data);
bool swi_send_rra_rd(uint16_t addr, uint8_t *data);
void swi_send_raw_word_noirq(uint8_t ub_code, uint8_t ub_data);

void ns3300_dev_lock(void);
void ns3300_dev_unlock(void);

#endif /* _NS3300_SWI_H_ */
