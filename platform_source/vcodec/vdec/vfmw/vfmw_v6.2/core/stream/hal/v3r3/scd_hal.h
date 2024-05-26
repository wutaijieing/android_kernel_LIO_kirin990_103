/*
 * scd_hal.h
 *
 * This is for scd hal.
 *
 * Copyright (c) 2012-2020 Huawei Technologies CO., Ltd.
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

#ifndef SCD_HAL_H
#define SCD_HAL_H

#include "vfmw.h"
#include "vfmw_sys.h"

#define SCD_ADDR_SHIFT 4

#define REG_SCD_START         (REG_OFFSET + 0x000)
#define REG_SCD_INT_MASK      (REG_OFFSET + 0x004)
#define REG_SCD_INT_CLR       (REG_OFFSET + 0x008)
#define REG_SCD_INT_FLAG      (REG_OFFSET + 0x00c)
#define REG_SCD_DESTORY       (REG_OFFSET + 0x010)
#define REG_SCD_PROTOCOL      (REG_OFFSET + 0x014)
#define REG_PRE_BYTE_LSB      (REG_OFFSET + 0x018)
#define REG_PRE_BYTE_MSB      (REG_OFFSET + 0x01c)
#define REG_LIST_ADDRESS      (REG_OFFSET + 0x020)
#define REG_UP_ADDRESS        (REG_OFFSET + 0x024)
#define REG_SCD_UP_LEN        (REG_OFFSET + 0x028)
#define REG_BUFFER_FIRST      (REG_OFFSET + 0x02c)
#define REG_BUFFER_LAST       (REG_OFFSET + 0x030)
#define REG_BUFFER_INIT       (REG_OFFSET + 0x034)
#define REG_BUFFER_INIT_MSB   (REG_OFFSET + 0x038)
#define REG_BYTE_VALID        (REG_OFFSET + 0x03c)
#define REG_SCD_NUM           (REG_OFFSET + 0x040)
#define REG_SRC_EATEN         (REG_OFFSET + 0x048)
#define REG_SEG_NEXT_ADDR     (REG_OFFSET + 0x04c)
#define REG_SEG_NEXT_ADDR_MSB (REG_OFFSET + 0x050)
#define REG_RUN_CYCLE         (REG_OFFSET + 0x054)
#define REG_ROLL_ADDR         (REG_OFFSET + 0x058)
#define REG_ROLL_ADDR_MSB     (REG_OFFSET + 0x05c)

#define RESET_SCD_COUNT 100

struct stm_dev;
int32_t scd_hal_open(stm_dev *dev);
void scd_hal_close(stm_dev *dev);
void scd_hal_start(stm_dev *dev);
void scd_hal_reset(stm_dev *dev);
int32_t scd_hal_cancel(void);
void scd_hal_write_reg(scd_reg_ioctl *reg_cfg_info);

void scd_hal_clear_int(void);
void scd_hal_enable_int(void);
int32_t scd_hal_isr_state(void);
void scd_hal_read_reg(stm_dev *dev);

#endif
