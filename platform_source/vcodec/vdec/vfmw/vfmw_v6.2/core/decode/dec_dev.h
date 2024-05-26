/*
 * dec_dev.h
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

#ifndef DEC_DEV_H
#define DEC_DEV_H

#include "dec_base.h"
#include "dec_hal.h"
#include "vfmw_osal.h"

#define DEC_VDH_WAIT_COUNT 20
#define DEC_VDH_WAIT_TIME 5
#define DEC_SEC_INSTANCE_ID 0

typedef enum {
	DEC_DEV_STATE_NULL = 0,
	DEC_DEV_STATE_RUN,
	DEC_DEV_STATE_SUSPEND,
	DEC_DEV_STATE_MAX,
} dec_dev_state;

typedef struct {
	int32_t (*open)(void *dev);
	void (*close)(void *dev);
	int32_t (*reset)(void *dev);
	int32_t (*dump_reg)(void *dev, uint16_t reg_id);
	void (*get_active_reg)(void *dev, uint16_t *reg_id);
	int32_t (*check_int)(void *dev, uint16_t reg_id);
	int32_t (*config_reg)(void *dev, void *task, uint16_t reg_id);
	void (*start_reg)(void *dev, uint16_t reg_id);
	int32_t (*cancel_reg)(void *dev, uint16_t reg_id);
	int32_t (*get_msg)(void *dev, uint16_t msg_id, void *msg);
	int32_t (*get_reg)(void *dev, uint16_t reg_id, void *reg);
	uint32_t (*get_pid_info)(void *dev);
	void (*set_pid_info)(void *dev, uint32_t pid);
} dec_dev_ops;

typedef struct {
	uint8_t used;
	uint16_t reg_id;
	uint32_t instance_id;
	uint32_t is_sec;
	uint32_t base_ofs[2];
	UADDR reg_phy_addr;
	uint8_t *reg_vir_addr;
	uint64_t reg_start_time;
	uint32_t decode_time;
	uint32_t chan_id;
} vdh_reg_info;

typedef struct {
	os_lock spin_lock;
	uint16_t reg_num;
	uint16_t next_cfg_reg;
	uint16_t next_isr_reg;
	uint16_t reserved;
	vdh_reg_info vdh_reg[VDH_REG_NUM];
} vdh_reg_pool_info;

typedef struct {
	vdh_reg_pool_info vdh_reg_pool;
} dec_dev_vdh;

typedef struct {
	UADDR reg_phy_addr;
	uint8_t *reg_vir_addr;
} mdma_reg_info;

typedef struct {
	uint8_t used;
	mdma_reg_info reg;
	uint32_t irq_num;
	uint32_t int_cnt;
	uint32_t reg_cfg_cnt;
} dec_dev_mdma;

typedef struct {
	uint16_t dev_id;
	dec_dev_type type;
	dec_dev_ops *ops;
	dec_dev_state state;
	dec_back_up back_up;
	void *handle;
	UADDR reg_phy_addr;
	uint32_t reg_size;
	uint32_t smmu_bypass;
	uint32_t perf_enable;
	uint32_t reset_disable;
	uint32_t force_clk;
	uint32_t reg_dump_enable;
	uint32_t poll_irq_enable;
	os_sema sema;
} dec_dev_info;

typedef struct {
	uint32_t used;
	dec_back_up back_up;
} dev_back_up_info;

typedef struct {
	dev_back_up_info dev_back_up_list[VDH_REG_NUM];
	uint16_t insert_index;
	uint16_t get_index;
	os_event event;
	os_lock lock;
} dec_back_up_list;

typedef struct {
	dec_dev_info       dev_inst[DEC_DEV_NUM];
	dec_back_up_list   dev_irq_backup_list[VFMW_CHAN_NUM];
} dec_dev_pool;

int32_t dec_dev_get_dev(uint16_t dev_id, dec_dev_info **dev);
dec_dev_pool *dec_dev_get_pool(void);
void dec_dev_init_entry(void);
void dec_dev_deinit_entry(void);

int32_t dec_dev_init(const void *args);
void dec_dev_deinit(void);
int32_t get_cur_active_reg(void *dev_cfg);
int32_t dec_dev_config_reg(uint32_t instance_id, void *dev_cfg);

int32_t dec_dev_isr_state(uint16_t dev_id);
int32_t dec_dev_isr(uint16_t dev_id);
int32_t dec_dev_get_backup(void *dev_reg_cfg, uint32_t instance_id, void *backup_out);

int32_t dec_dev_reset(dec_dev_info *dev);
void dec_dev_resume(void);
void dec_dev_suspend(void);

void dec_dev_write_store_info(uint32_t pid);
uint32_t dec_dev_read_store_info(void);
int32_t dec_dev_cancel_reg(uint32_t instance_id, void *dev_cfg);

int32_t dec_dev_acquire_back_up_fifo(uint32_t instance_id);
void dec_dev_release_back_up_fifo(uint32_t instance_id);
#endif
