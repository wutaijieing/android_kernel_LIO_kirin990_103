/*
 * regulator.h
 *
 * This is for vdec regulator manager.
 *
 * Copyright (c) 2017-2020 Huawei Technologies CO., Ltd.
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

#ifndef __VDEC_REGULATOR_H__
#define __VDEC_REGULATOR_H__

#include <linux/mod_devicetable.h>
#include "vcodec_types.h"
#include "vfmw_dts.h"

#define READ_CLOCK_RATE_INDEX_ONE    1
#define READ_CLOCK_RATE_INDEX_TWO    2
#define READ_CLOCK_RATE_INDEX_THREE  3

#define VDEC_TRQ_NUM_PROT            323
#define VDEC_TRQ_NUM_SAFE            324

typedef enum {
#ifdef LOWER_FREQUENCY_SUPPORT
	VDEC_CLK_RATE_LOWER = 0,
#endif
	VDEC_CLK_RATE_LOW,
	VDEC_CLK_RATE_NORMAL,
	VDEC_CLK_RATE_HIGH,
	VDEC_CLK_RATE_MAX,
} clk_rate_e;

struct perf_load_range {
	clk_rate_e clk_rate;
	UINT64 lower_limit;
	UINT64 upper_limit;
};

struct performance_params_s {
	UINT64 load;
	clk_rate_e base_rate;
	struct perf_load_range load_range_map[VDEC_CLK_RATE_MAX];
};

struct smmu_cfg_info {
	uint32_t smmu_tbu_offset;
	uint32_t smmu_sid_offset;
	uint32_t smmu_norm_sid;
	uint32_t smmu_need_cfg;
};

int32_t  vdec_regulator_probe(struct device *dev);
int32_t  vdec_regulator_remove(struct device *dev);
int32_t  vdec_regulator_enable(void);
int32_t  vdec_regulator_disable(void);
void vdec_regulator_get_clk_rate(clk_rate_e *clk_rate);
int32_t  vdec_regulator_set_clk_rate(clk_rate_e e_clk_rate);
#ifdef VCODEC_SMMUV300
struct regulator* get_tcu_regulator(void);
struct smmu_cfg_info* get_smmu_cfg_info(void);
#endif
#endif
