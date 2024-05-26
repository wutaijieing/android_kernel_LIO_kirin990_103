/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef HISI_PM_OVERLAY_H
#define HISI_PM_OVERLAY_H

#include <linux/regulator/consumer.h>
#include <linux/types.h>
#include <linux/clk.h>
#include <linux/semaphore.h>
#include <linux/atomic.h>

#include "hisi_disp_config.h"
#include "hisi_disp_gadgets.h"

#define DISP_1M  1000000ul

enum {
	PM_LP_MEMORY_1 = 0, /* TODO: rename the memory */
	PM_LP_MEMORY_MAX,
};

struct hisi_disp_pm_regulator {
	atomic_t regu_ref_cnt;
	struct semaphore regu_sem;
	struct regulator_bulk_data *regulator;

	int (*enable_regulator)(struct hisi_disp_pm_regulator *pm_regulator);
	int (*disable_regulator)(struct hisi_disp_pm_regulator *pm_regulator);
};

struct hisi_disp_pm_clk {
	struct clk *clk;
	uint32_t default_clk_rate;
	uint32_t current_clk_rate;

	int (*enable_clk)(struct hisi_disp_pm_clk *clk);
	int (*disable_clk)(struct hisi_disp_pm_clk *clk);
	int (*set_clk_rate)(struct hisi_disp_pm_clk *clk, uint32_t clk_rate);
};

struct hisi_disp_pm_memory {
	atomic_t ref_cnt;

	int (*set_default)(struct hisi_disp_pm_memory *lm_memory);
	int (*enter_lowpower)(struct hisi_disp_pm_memory *lm_memory);
	int (*exit_lowpower)(struct hisi_disp_pm_memory *lm_memory);
};

struct hisi_disp_pm_lowpower {
	void (*set_lp_clk)(struct hisi_disp_pm_lowpower *lp_mng);
	void (*set_lp_memory)(struct hisi_disp_pm_lowpower *lp_mng);

	char __iomem *base_addr;
	struct hisi_disp_pm_memory lp_memory[PM_LP_MEMORY_MAX];
};

struct hisi_disp_pm {
	struct hisi_disp_pm_regulator pm_regulators[PM_REGULATOR_MAX];
	struct hisi_disp_pm_clk pm_clks[PM_CLK_MAX];
	struct hisi_disp_pm_lowpower lp_mng;
};

int hisi_disp_pm_enable_regulator(uint32_t regu_type);
int hisi_disp_pm_disable_regulator(uint32_t regu_type);
int hisi_disp_pm_enable_clk(uint32_t clk_type);
int hisi_disp_pm_disable_clk(uint32_t clk_type);
int hisi_disp_pm_set_clk_rate(uint32_t clk_type, uint32_t clk_rate);
int hisi_disp_pm_set_clk_default_rate(uint32_t clk_type);

static inline int hisi_disp_pm_enable_regulators(ulong regu_bits)
{
	int ret = 0;

	disp_pr_info(" +++++++ \n");

	/* power on media1->vivobus->tcu->dss */
	ret = hisi_disp_pm_enable_regulator(PM_REGULATOR_MEDIA1);
	ret = hisi_disp_pm_enable_regulator(PM_REGULATOR_VIVOBUS);
	ret = hisi_disp_pm_enable_regulator(PM_REGULATOR_SMMU);
	ret = hisi_disp_pm_enable_regulator(PM_REGULATOR_DPU);
	return ret;
}

static inline int hisi_disp_pm_disable_regulators(ulong regu_bits)
{
	disp_pr_info(" +++++++ \n");

	uint32_t i = 0;
	int ret = 0;

	for_each_set_bit(i, &regu_bits, PM_REGULATOR_MAX)
		ret = hisi_disp_pm_disable_regulator(i);

	return ret;
}

static inline int hisi_disp_pm_enable_clks(ulong clk_bits)
{
	uint32_t i = 0;
	int ret = 0;

	disp_pr_info(" +++++++ \n");

	for_each_set_bit(i, &clk_bits, PM_CLK_MAX)
		ret = hisi_disp_pm_enable_clk(i);

	disp_pr_info(" ---- \n");
	return ret;
}

static inline int hisi_disp_pm_disable_clks(ulong clk_bits)
{
	uint32_t i = 0;
	int ret = 0;

	disp_pr_info(" +++++++ \n");

	for_each_set_bit(i, &clk_bits, PM_CLK_MAX)
		ret = hisi_disp_pm_disable_clk(i);

	return ret;
}

static inline int hisi_disp_pm_set_clks_default_rate(ulong clk_bits)
{
	uint32_t i = 0;
	int ret = 0;

	disp_pr_info(" +++++++ \n");
	for_each_set_bit(i, &clk_bits, PM_CLK_MAX)
		ret = hisi_disp_pm_set_clk_default_rate(i);

	return ret;
}

void hisi_disp_pm_set_lp(char __iomem *dpu_base_addr);
void hisi_disp_pm_init(struct regulator_bulk_data *regulators, uint32_t regu_count, struct clk *clks[], uint32_t clk_count);
void hisi_disp_pm_get_clks(ulong bits, struct hisi_disp_pm_clk **pm_clk, uint32_t count);
void hisi_disp_pm_get_regulators(ulong bits, struct hisi_disp_pm_regulator **pm_regulators, uint32_t count);
void hisi_disp_pm_registe_base_lp_cb(struct hisi_disp_pm_lowpower *lp_mng);
void hisi_disp_pm_registe_platform_lp_cb(struct hisi_disp_pm_lowpower *lp_mng);

#endif /* HISI_PM_OVERLAY_H */
