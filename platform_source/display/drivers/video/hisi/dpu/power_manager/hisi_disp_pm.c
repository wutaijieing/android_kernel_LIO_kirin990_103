/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/of.h>

#include "hisi_disp_pm.h"
#include "hisi_disp_debug.h"
#include "hisi_disp_gadgets.h"

static struct hisi_disp_pm g_composer_pm;
static bool g_pm_be_inited;

static int _hisi_enable_regulator(struct hisi_disp_pm_regulator *pm_regulator)
{
	int ret;
	disp_pr_info("++++");
	if (atomic_read(&pm_regulator->regu_ref_cnt) > 0)
		return 0;

	down(&pm_regulator->regu_sem);
	ret = regulator_enable(pm_regulator->regulator->consumer);
	if (ret) {
		disp_pr_err("enable %s regulator fail", pm_regulator->regulator->supply);
		up(&pm_regulator->regu_sem);
		return ret;
	}
	up(&pm_regulator->regu_sem);

	atomic_inc(&pm_regulator->regu_ref_cnt);
	return 0;
}

static int _hisi_disable_regulator(struct hisi_disp_pm_regulator *pm_regulator)
{
	int ret;

	/* fb have not been opened, return */
	if (atomic_read(&pm_regulator->regu_ref_cnt) == 0)
		return 0;

	/* ref_cnt is not 0 */
	if (atomic_dec_return(&pm_regulator->regu_ref_cnt) != 0)
		return 0;

	down(&pm_regulator->regu_sem);
	ret = regulator_disable(pm_regulator->regulator->consumer);
	if (ret) {
		disp_pr_err("enable %s regulator fail", pm_regulator->regulator->supply);
		up(&pm_regulator->regu_sem);
		return ret;
	}
	up(&pm_regulator->regu_sem);

	return 0;
}

static int _hisi_enable_clk(struct hisi_disp_pm_clk *clk)
{
	int ret = 0;
	disp_pr_info("++++");
	if (clk && clk->clk) {
		ret = clk_prepare_enable(clk->clk);
		if (ret) {
			disp_pr_err("enable clk fail");
		}
	}

	return ret;
}

static int _hisi_disable_clk(struct hisi_disp_pm_clk *clk)
{
	if (clk && clk->clk)
		clk_disable_unprepare(clk->clk);

	return 0;
}

static int _hisi_set_clk_rate(struct hisi_disp_pm_clk *clk, uint32_t clk_rate)
{
	int ret = 0;
	disp_pr_info("clk_rate:%d", clk_rate);
	if (clk_rate == 0)
		return 0;

	if (clk && clk->clk) {
		ret = clk_set_rate(clk->clk, clk_rate);
		if (ret) {
			disp_pr_err("set clk rate %u fail ", clk_rate);
			return ret;
		}
	}

	return ret;
}

static int _hisi_disp_pm_ops_resource(uint64_t bits, int (*ops)(uint32_t))
{
	uint32_t bit = 0;
	int ret = 0;

	if (!ops)
		return 0;

	while (bits != 0) {
		if (ENABLE_BITS(bits, bit)) {
			ret = ops(bit);
			if (ret) {
				disp_pr_err("enable ops fail");
				return ret;
			}
			CLEAR_BITS(bits, bit);
		}
		bit++;
	}

	return ret;
}

static void hisi_disp_pm_init_regulator(struct regulator_bulk_data *input, struct hisi_disp_pm_regulator *regulator)
{
	sema_init(&regulator->regu_sem, 1);
	atomic_set(&(regulator->regu_ref_cnt), 0);

	regulator->regulator = input;
	regulator->enable_regulator = _hisi_enable_regulator;
	regulator->disable_regulator = _hisi_disable_regulator;
}

static void hisi_disp_pm_init_clk(struct clk *input, struct hisi_disp_pm_clk *clk)
{
	clk->clk = input;

	clk->enable_clk = _hisi_enable_clk;
	clk->disable_clk = _hisi_disable_clk;
	clk->set_clk_rate = _hisi_set_clk_rate;
	clk->current_clk_rate = 0;
	clk->default_clk_rate = 0;
}

/* TODO: the init function maybe will be implement at different platform file */
static void hisi_lowerpower_init(struct hisi_disp_pm_lowpower *lp_mng)
{
	hisi_disp_pm_registe_base_lp_cb(lp_mng);
}

int hisi_disp_pm_enable_regulator(uint32_t regu_type)
{
	struct hisi_disp_pm_regulator *pm_regulator = NULL;
	int ret = 0;

	disp_pr_info("+++");
	if (regu_type >= PM_REGULATOR_MAX)
		return -1;

	pm_regulator = &g_composer_pm.pm_regulators[regu_type];
	if (pm_regulator->enable_regulator)
		ret = pm_regulator->enable_regulator(pm_regulator);

	return ret;
}

int hisi_disp_pm_disable_regulator(uint32_t regu_type)
{
	struct hisi_disp_pm_regulator *pm_regulator = NULL;
	int ret = 0;

	disp_pr_info("+++");
	if (regu_type >= PM_REGULATOR_MAX)
		return -1;

	pm_regulator = &g_composer_pm.pm_regulators[regu_type];
	if (pm_regulator->disable_regulator)
		ret = pm_regulator->disable_regulator(pm_regulator);

	return ret;
}

int hisi_disp_pm_enable_clk(uint32_t clk_type)
{
	struct hisi_disp_pm_clk *pm_clk = NULL;
	int ret = 0;

	disp_pr_info("+++");
	if (clk_type >= PM_CLK_MAX)
		return -1;

	pm_clk = &g_composer_pm.pm_clks[clk_type];
	if (pm_clk->enable_clk)
		ret = pm_clk->enable_clk(pm_clk);

	return ret;
}

int hisi_disp_pm_disable_clk(uint32_t clk_type)
{
	struct hisi_disp_pm_clk *pm_clk = NULL;
	int ret = 0;

	if (clk_type >= PM_CLK_MAX)
		return -1;

	pm_clk = &g_composer_pm.pm_clks[clk_type];
	if (pm_clk->disable_clk)
		ret = pm_clk->disable_clk(pm_clk);

	return ret;
}

int hisi_disp_pm_set_clk_rate(uint32_t clk_type, uint32_t clk_rate)
{
	struct hisi_disp_pm_clk *pm_clk = NULL;
	int ret = 0;
	disp_pr_info("clk_rate:%d", clk_rate);
	if (clk_type >= PM_CLK_MAX)
		return -1;

	pm_clk = &g_composer_pm.pm_clks[clk_type];
	if (!pm_clk->set_clk_rate)
		return -1;

	if (clk_rate == pm_clk->current_clk_rate)
		return 0;

	ret = pm_clk->set_clk_rate(pm_clk, clk_rate);
	if (ret) {
		disp_pr_err("set clk rate fail, rate=%u", clk_rate);
		return ret;
	}

	pm_clk->current_clk_rate = clk_rate;
	return ret;
}

int hisi_disp_pm_set_clk_default_rate(uint32_t clk_type)
{
	struct hisi_disp_pm_clk *pm_clk = NULL;
	int ret = 0;
	disp_pr_info("clk_type:%d", clk_type);
	if (clk_type >= PM_CLK_MAX)
		return -1;

	pm_clk = &g_composer_pm.pm_clks[clk_type];
	if (pm_clk->default_clk_rate == 0)
		return 0;

	if (pm_clk->set_clk_rate)
		ret = pm_clk->set_clk_rate(pm_clk, pm_clk->default_clk_rate);

	return ret;
}


void hisi_disp_pm_set_lp(char __iomem *dpu_base_addr)
{
	struct hisi_disp_pm_lowpower *lp = NULL;
	disp_pr_info("dpu_base_addr:%d", dpu_base_addr);
	lp = &g_composer_pm.lp_mng;
	lp->base_addr = dpu_base_addr;
	if (lp->set_lp_clk)
		lp->set_lp_clk(lp);

	if (lp->set_lp_memory)
		lp->set_lp_memory(lp);
}

void hisi_disp_pm_init(struct regulator_bulk_data *regulators, uint32_t regu_count,
		struct clk *clks[], uint32_t clk_count)
{
	uint32_t i;
	disp_pr_info(" +++++++ \n");
	if (g_pm_be_inited)
		return;

	for (i = 0; i < regu_count; i++)
		hisi_disp_pm_init_regulator(&regulators[i], &g_composer_pm.pm_regulators[i]);

	for (i = 0; i < clk_count; i++)
		hisi_disp_pm_init_clk(clks[i], &g_composer_pm.pm_clks[i]);

	hisi_lowerpower_init(&g_composer_pm.lp_mng);

	g_pm_be_inited = true;
}

void hisi_disp_pm_get_regulators(ulong bits, struct hisi_disp_pm_regulator **pm_regulators, uint32_t count)
{
	uint32_t i = 0;
	disp_pr_info("++++");
	if (count > PM_REGULATOR_MAX)
		return;

	for_each_set_bit(i, &bits, PM_REGULATOR_MAX)
		pm_regulators[i] = &g_composer_pm.pm_regulators[i];
}

void hisi_disp_pm_get_clks(ulong bits, struct hisi_disp_pm_clk **pm_clk, uint32_t count)
{
	uint32_t i = 0;
	disp_pr_info("++++");
	if (count > PM_CLK_MAX)
		return;

	for_each_set_bit(i, &bits, PM_CLK_MAX)
		pm_clk[i] = &g_composer_pm.pm_clks[i];
}

