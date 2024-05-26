/*
 * dec_hal.h
 *
 * This is for dec dev module intf.
 *
 * Copyright (c) 2019-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef DEC_HAL_H
#define DEC_HAL_H

#include "dec_base.h"
#include "vfmw_sys.h"

#define DEC_DEV_NUM            (VDH_NUM + MDMA_NUM)
#define DEC_DEV_START_IDX_VDH  0
#define DEC_DEV_START_IDX_MDMA VDH_NUM
#define DEC_CANCEL_ACK_TIMEOUT 1000

#define hw_addr_shift(addr) ((addr) >> (4))

int32_t dec_hal_open_vdh(void *dec_dev);
void dec_hal_close_vdh(void *dec_dev);
int32_t dec_hal_reset_vdh(void *dec_dev);
void dec_hal_get_active_reg_id(void *dec_dev, uint16_t *reg_id);

void dec_hal_start_dec(void *dec_dev, uint16_t reg_id);
int32_t dec_hal_cancel_vdh(void *dec_dev, uint16_t reg_id);
int32_t dec_hal_config_reg_vdh(void *dec_dev, void *dec_vdh_cfg, uint16_t reg_id);

int32_t dec_hal_check_int(void *dec_dev, uint16_t reg_id);
int32_t dec_hal_read_int_info(void *dec_dev, uint16_t reg_id);
void dec_hal_clear_int_state(void *dec_dev);

void dec_hal_set_pid_info(void *dec_dev, uint32_t pid);
uint32_t dec_hal_get_pid_info(void *dec_dev);
int32_t dec_hal_dump_reg(void *dec_dev, uint16_t reg_id);

#endif
