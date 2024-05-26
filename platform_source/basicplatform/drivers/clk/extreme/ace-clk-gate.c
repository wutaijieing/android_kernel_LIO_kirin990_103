/*
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
 */
#include "ace-clk-gate.h"
#include "clk-hvc.h"

static int clk_name_from_virt_to_phys(struct ace_periclk *pclk, unsigned long *clk_name)
{
	char name[MAX_NAME_COUNT] = {0};
	int ret;

	ret = strncpy_s(name, MAX_NAME_COUNT, pclk->name, strlen(pclk->name));
	if (ret != EOK) {
		pr_err("%s strncpy_s failed!\n", __func__);
		return -ENOMEM;
	}

	*clk_name = virt_to_phys(name);

	return 0;
}

static int ace_clkgate_prepare(struct clk_hw *hw)
{
	int ret;
	unsigned long clk_name;
	struct ace_periclk *pclk = container_of(hw, struct ace_periclk, hw);

	ret = clk_name_from_virt_to_phys(pclk, &clk_name);
	if (ret) {
		pr_err("[%s] virt clk name to phy failed!\n", __func__);
		return ret;
	}

	ret = uvmm_service_clk_hvc(CLK_PREPARE, clk_name, 0, 0);
	if (ret) {
		pr_err("[%s] %s prepare failed, ret = %d !\n", __func__, pclk->name, ret);
		return ret;
	}
	return 0;
}

static void ace_clkgate_unprepare(struct clk_hw *hw)
{
	int ret;
	unsigned long clk_name;
	struct ace_periclk *pclk = container_of(hw, struct ace_periclk, hw);

	ret = clk_name_from_virt_to_phys(pclk, &clk_name);
	if (ret) {
		pr_err("[%s] %s virt clk name to phy failed!\n", __func__);
		return;
	}

	ret = uvmm_service_clk_hvc(CLK_UNPREPARE, clk_name, 0, 0);
	if (ret) {
		pr_err("[%s] %s unprepare failed, ret = %d !\n", __func__, pclk->name, ret);
		return;
	}
}

static int ace_clkgate_enable(struct clk_hw *hw)
{
	int ret;
	unsigned long clk_name;
	struct ace_periclk *pclk = container_of(hw, struct ace_periclk, hw);

	ret = clk_name_from_virt_to_phys(pclk, &clk_name);
	if (ret) {
		pr_err("[%s] %s virt clk name to phy failed!\n", __func__);
		return ret;
	}

	ret = uvmm_service_clk_hvc(CLK_ENABLE, clk_name, 0, 0);
	if (ret) {
		pr_err("[%s] %s enable failed, ret = %d !\n", __func__, pclk->name, ret);
		return ret;
	}
	return 0;
}

static void ace_clkgate_disable(struct clk_hw *hw)
{
	int ret;
	unsigned long clk_name;
	struct ace_periclk *pclk = container_of(hw, struct ace_periclk, hw);

	ret = clk_name_from_virt_to_phys(pclk, &clk_name);
	if (ret) {
		pr_err("[%s] %s virt clk name to phy failed!\n", __func__);
		return;
	}

	ret = uvmm_service_clk_hvc(CLK_DISABLE, clk_name, 0, 0);
	if (ret) {
		pr_err("[%s] %s disable failed, ret = %d !\n", __func__, pclk->name, ret);
		return;
	}
}

static int ace_clkgate_set_rate(struct clk_hw *hw, unsigned long rate,
	unsigned long parent_rate)
{
	int ret;
	unsigned long clk_name;
	struct ace_periclk *pclk = container_of(hw, struct ace_periclk, hw);

	ret = clk_name_from_virt_to_phys(pclk, &clk_name);
	if (ret) {
		pr_err("[%s] virt clk name to phy failed!\n", __func__);
		return ret;
	}

	ret = uvmm_service_clk_hvc(CLK_SET_RATE, clk_name, rate, parent_rate);
	if (ret) {
		pr_err("[%s] %s set rate failed, ret = %d !\n", __func__, pclk->name, ret);
		return ret;
	}
	return 0;
}

/*
 * func_stub, just for clk to register successfully
 */
static int ace_clkgate_determine_rate(struct clk_hw *hw,
	struct clk_rate_request *req)
{
	return 0;
}

static unsigned long ace_clkgate_get_rate(const char *pclkname)
{
	unsigned long rate = 0;
	unsigned long clk_name;
	int ret;
	char name[MAX_NAME_COUNT] = {0};

	ret = strncpy_s(name, MAX_NAME_COUNT, pclkname, strlen(pclkname));
	if (ret != EOK) {
		pr_err("%s strncpy_s failed!\n", __func__);
		return -ENOMEM;
	}

	clk_name = virt_to_phys(name);
	rate = uvmm_service_clk_hvc(CLK_GET_RATE, clk_name, 0, 0);
	return rate;
}

static unsigned long ace_clkgate_recalc_rate(struct clk_hw *hw,
	unsigned long parent_rate)
{
	struct ace_periclk *pclk = container_of(hw, struct ace_periclk, hw);

	unsigned long rate = ace_clkgate_get_rate(pclk->name);

	return rate;
}

static const struct clk_ops ace_clkgate_ops = {
	.prepare	= ace_clkgate_prepare,
	.unprepare	= ace_clkgate_unprepare,
	.enable		= ace_clkgate_enable,
	.disable	= ace_clkgate_disable,
	.set_rate	= ace_clkgate_set_rate,
	.determine_rate	= ace_clkgate_determine_rate,
	.recalc_rate	= ace_clkgate_recalc_rate,
};

static struct clk *__clk_register_gate(const struct gate_clock *gate,
	struct clock_data *data)
{
	struct ace_periclk *pclk = NULL;
	struct clk_init_data init;
	struct clk *clk = NULL;
	struct hs_clk *hs_clk = get_hs_clk_info();

#ifdef CONFIG_FMEA_FAULT_INJECTION
	if (clk_fault_injection()) {
		pr_err("[%s] Guest clk_register fail!\n", __func__);
		return clk;
	}
#endif

	pclk = kzalloc(sizeof(*pclk), GFP_KERNEL);
	if (pclk == NULL) {
		pr_err("[%s] fail to alloc pclk!\n", __func__);
		return clk;
	}

	init.name = gate->name;
	init.ops = &ace_clkgate_ops;
	init.flags = CLK_IGNORE_UNUSED | CLK_GET_RATE_NOCACHE;
	init.parent_names = NULL;
	init.num_parents = 0;

	pclk->lock = &hs_clk->lock;
	pclk->hw.init = &init;
	pclk->name = gate->alias;

	clk = clk_register(NULL, &pclk->hw);
	if (IS_ERR(clk)) {
		pr_err("[%s] fail to reigister clk %s!\n",
			__func__, gate->name);
		goto err_init;
	}

	/* init is local variable, need set NULL before func */
	pclk->hw.init = NULL;
	return clk;

err_init:
	kfree(pclk);
	return clk;
}

void ace_clk_register_gate(const struct gate_clock *clks,
	int nums, struct clock_data *data)
{
	struct clk *clk = NULL;
	int i;

	for (i = 0; i < nums; i++) {
		clk = __clk_register_gate(&clks[i], data);
		if (IS_ERR_OR_NULL(clk)) {
			pr_err("%s: failed to register clock %s\n",
			       __func__, clks[i].name);
			continue;
		}

		pr_debug("clks id %d, nums %d, clks name = %s!\n",
			clks[i].id, nums, clks[i].name);

		clk_data_init(clk, clks[i].alias, clks[i].id, data);
	}
}
