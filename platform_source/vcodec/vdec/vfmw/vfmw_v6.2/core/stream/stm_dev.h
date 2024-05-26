/*
 * stm_dev.h
 *
 * This is for decoder.
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

#ifndef STM_DEV_H
#define STM_DEV_H

#include "vfmw_osal.h"
#include "stm_base.h"

typedef enum {
	STM_DEV_STATE_NULL = 0,
	STM_DEV_STATE_IDLE,
	STM_DEV_STATE_BUSY,
	STM_DEV_STATE_MAX,
} stm_dev_state;

typedef struct {
	UADDR reg_phy;
	uint8_t *reg_vir;
	uint32_t reg_size;
	uint32_t smmu_bypass;
	uint32_t perf_enable;
	uint32_t poll_irq_enable;
	stm_dev_state state;
	scd_reg reg;
	os_event event;
	os_sema sema;
} stm_dev;

stm_dev *stm_dev_get_dev(void);
void stm_dev_init_entry(void);
void stm_dev_deinit_entry(void);

int32_t stm_dev_init(void *args);
void stm_dev_deinit(void);
int32_t stm_dev_config_reg(void *stm_cfg, void *scd_state_reg);

int32_t stm_dev_isr(void);
void stm_dev_suspend(void);
void stm_dev_resume(void);

#endif
