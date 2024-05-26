/* SPDX-License-Identifier: GPL-2.0 */
/*
 * bsp_swi.h
 *
 * board command head file
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

#ifndef _BSP_SWI_H_
#define _BSP_SWI_H_

#include <linux/types.h>
#include "ns3300_swi.h"

/* SAC Address */
#define SAC_IADR  (0x004u)
#define SAC_OADR  (0x008u)
#define SAC_SR    (0x010u)

/* Configration Register Address */
#define SWI_BASE_ADDR       (0xF0u)

#define SWI_ADDR0           (SWI_BASE_ADDR + 0x00u)    /* Control Register 0 */
#define SWI_ADDR1           (SWI_BASE_ADDR + 0x01u)    /* Control Register 1 */
#define SWI_COMMAND         (SWI_BASE_ADDR + 0x02u)    /* Control Register 2 */
#define SWI_INDEX           (SWI_BASE_ADDR + 0x03u)    /* Control Register 7 */
#define SWI_DATA0           (SWI_BASE_ADDR + 0x04u)    /* Capability Register 0 */
#define SWI_DATA1           (SWI_BASE_ADDR + 0x05u)    /* Capability Register 1 */
#define SWI_DATA2           (SWI_BASE_ADDR + 0x06u)    /* Capability Register 4 */
#define SWI_DATA3           (SWI_BASE_ADDR + 0x07u)    /* Capability Register 5 */
#define SWI_DATA4           (SWI_BASE_ADDR + 0x08u)    /* Capability Register 6 */
#define SWI_DATA5           (SWI_BASE_ADDR + 0x09u)    /* Capability Register 7 */
#define SWI_DATA6           (SWI_BASE_ADDR + 0x0Au)    /* Interrupt Register 0 */
#define SWI_DATA7           (SWI_BASE_ADDR + 0x0Bu)    /* Interrupt Register 4 */

/* INFO Addresss */
#define SWI_INFO_SIGN       (0x100u) /* 64 Bytes */
#define SWI_INFO_PUBKEY     (0x140u) /* 42 Bytes */
#define SWI_INFO_UID        (0x170u) /* 12 Bytes */

#define SWI_USER_DATA       (0x180u)
#define SWI_USER_DATA_SIZE  (0x80u) /* 128BYTES */

/* Configration Register Command */
#define SWI_CMD_SFRST       (0x80u)
#define SWI_CMD_PDWN        (0x81u)
#define SWI_CMD_ECCEN       (0x82u)
#define SWI_CMD_WRITE       (0x83u)
#define SWI_CMD_READ        (0x84u)
#define SWI_CMD_KILL        (0x85u)
#define SWI_CMD_CNTINIT     (0x86u)
#define SWI_CMD_CNTLOCK     (0x87u)
#define SWI_CMD_CNTADD      (0x88u)
#define SWI_CMD_CNTSUB      (0x89u)
#define SWI_CMD_ECNTEN      (0x8Au)
#define SWI_CMD_ECNTINIT    (0x8Bu)
#define SWI_CMD_ECNTLOCK    (0x8Cu)
#define SWI_CMD_USERLOCK    (0x8Du)

bool swi_read_space(uint16_t uw_address, uint8_t ub_bytestoread, uint8_t *ubp_data);
bool swi_write_user(uint16_t uw_address, uint8_t ub_bytes_to_write, uint8_t *ubp_data);
bool swi_cmd_soft_reset(void);
bool swi_cmd_power_down(void);
bool swi_cmd_enable_ecc(void);

bool swi_cmd_kill(void);
bool swi_cmd_init_cnt(uint8_t *ubp_data);
bool swi_cmd_lock_cnt(void);
bool swi_cmd_count_add_one(void);
bool swi_cmd_count_sub_one(void);
bool swi_cmd_enble_ecc_cnt(void);
bool swi_cmd_init_ecc_count(uint8_t *ubp_data);
bool swi_cmd_lock_ecc_count(void);
bool swi_cmd_lock_user(uint8_t *ubp_data, unsigned int len);
bool swi_cmd_read_status(uint8_t *ubp_data);
bool swi_cmd_read_swi_error_counter(uint8_t *ubp_data);
bool swi_cmd_read_page_lock(uint8_t *status, int lock_num);
bool swi_cmd_read_lock_cnt(uint8_t *value);

#endif /* _BSP_SWI_H_ */
