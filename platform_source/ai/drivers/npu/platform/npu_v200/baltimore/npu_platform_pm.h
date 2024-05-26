/*
 * npu_platform_pm.h
 *
 * about npu hwts plat
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
#ifndef __NPU_PLATFORM_PM_H
#define __NPU_PLATFORM_PM_H

#include "npu_common.h"

typedef struct {
	u32 data;
} npu_work_mode_set;

typedef struct {
	u32 data;
} npu_subip_set;

enum npu_subip {
	NPU_TOP     = 0,
	NPU_SUBSYS  = 1,  /* (npu_bus/npu_subsys/hwts/smmu_tcu(ts_subsys)/gic_connect/L2/SDMA) */
	NPU_TSCPU   = 2,
	NPU_SMMU    = 3,
	NPU_AICORE0 = 4,
	NPU_AICORE1 = 5,
	NPU_SUBIP_MAX,
};

enum npu_pm_ops {
	POWER_UP    = 0,
	POWER_DOWN  = 1,
	OPS_MAX,
};

struct npu_pm_work_strategy {
	enum npu_workmode work_mode;        /* curr_work_mode */
	npu_work_mode_set work_mode_set;    /* curr_work_mode is mutually exclusive with work_mode in work_mode_set */
	bool is_idle_support;               /* is curr_work_mode support idle timer */
	npu_subip_set subip_set;            /* curr_work_mode need these subip */
};

struct npu_pm_subip_action {
	enum npu_subip type;
	int (*power_up)(u32, u32, void **);
	int (*power_down)(u32, u32, void *);
};

enum npu_subip *get_powerup_order(void);

enum npu_subip *get_powerdown_order(void);

int hwts_disable_aicore(u32);

int npu_pm_init(struct npu_dev_ctx *dev_ctx);

#endif
