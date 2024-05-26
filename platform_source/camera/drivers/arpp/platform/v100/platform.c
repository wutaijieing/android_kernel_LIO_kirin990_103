/*
 * Copyright (C) 2019-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <platform_include/see/efuse_driver.h>

#include "platform.h"
#include "arpp_log.h"

#define OF_ARPP_CLK_RATE_LOW       "arpp_low_clk_rate"
#define OF_ARPP_CLK_RATE_MEDIUM    "arpp_medium_clk_rate"
#define OF_ARPP_CLK_RATE_HIGH      "arpp_high_clk_rate"
#define OF_ARPP_CLK_RATE_TURBO     "arpp_turbo_clk_rate"
#define OF_ARPP_CLK_RATE_LOWTEMP   "arpp_lowtemp_clk_rate"
#define OF_ARPP_CLK_RATE_PD        "arpp_pd_clk_rate"
#define OF_ARPP_CLK_RATE_LP        "arpp_lp_clk_rate"

/* efuse info */
#define EFUSE_BUFFER_NUM           3
#define EFUSE_CHIP_TYPE_NORMAL     0x0
#define EFUSE_CHIP_TYPE_LITE       0x6
#define EFUSE_ARPP_TYPE_NORMAL     0x1
#define EFUSE_ARPP_TYPE_BYPASS     0x0

int is_arpp_available(void)
{
	int i;
	int ret;
	unsigned int soc_type_len;
	unsigned int by_pass_chip_type_len;
	struct device_node *np = NULL;
	const char *soc_spec_set = NULL;
	const char *by_pass_chip_type[12] = {
		"lite-normal", "lite", "wifi-only-normal", "wifi-only", "lite2", "lite2-normal", "pc", "pc-normal", "lsd", "lsd-normal", "sd", "sd-normal",
	};

	np = of_find_compatible_node(NULL, NULL, "hisilicon, soc_spec");
	if (np == NULL) {
		hiar_logw("get soc_spec node failed");
		return 0;
	}

	ret = of_property_read_string_index(np, "soc_spec_set", 0, &soc_spec_set);
	if (ret != 0) {
		hiar_logw("read soc_spec failed, ret=%d", ret);
		of_node_put(np);
		return 0;
	}

	of_node_put(np);

	hiar_logi("read soc_spec type:%s", soc_spec_set);

	/* check whether chip is lite or force to lite */
	for (i = 0; i < 12; ++i) {
		soc_type_len = strlen(soc_spec_set);
		by_pass_chip_type_len = strlen(by_pass_chip_type[i]);
		if (soc_type_len != by_pass_chip_type_len)
			continue;

		if (strncmp(soc_spec_set, by_pass_chip_type[i], by_pass_chip_type_len) == 0) {
			hiar_logi("lite type");
			return -1;
		}
	}

	return 0;
}

int get_regulator(struct arpp_device *arpp_dev, struct device *dev)
{
	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return -EINVAL;
	}

	if (dev == NULL) {
		hiar_loge("dev is NULL!");
		return -EINVAL;
	}

	arpp_dev->media2_supply = devm_regulator_get(dev, "media2_subsys");
	if (IS_ERR(arpp_dev->media2_supply)) {
		hiar_loge("failed to get media2 regulator");
		return -EFAULT;
	}

	arpp_dev->arpp_supply = devm_regulator_get(dev, "arpp");
	if (IS_ERR(arpp_dev->arpp_supply)) {
		hiar_loge("failed to get arpp regulator");
		goto err_put_regulator;
	}

	arpp_dev->smmu_tcu_supply = devm_regulator_get(dev, "smmu-tcu-regulator");
	if (IS_ERR(arpp_dev->smmu_tcu_supply)) {
		hiar_loge("failed to get tcu regulator");
		goto err_put_regulator;
	}

	return 0;

err_put_regulator:
	(void)put_regulator(arpp_dev);

	return -EFAULT;
}

int put_regulator(struct arpp_device *arpp_dev)
{
	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return -EINVAL;
	}

	if (arpp_dev->media2_supply != NULL) {
		devm_regulator_put(arpp_dev->media2_supply);
		arpp_dev->media2_supply = NULL;
	}

	if (arpp_dev->arpp_supply != NULL) {
		devm_regulator_put(arpp_dev->arpp_supply);
		arpp_dev->arpp_supply = NULL;
	}

	if (arpp_dev->smmu_tcu_supply != NULL) {
		devm_regulator_put(arpp_dev->smmu_tcu_supply);
		arpp_dev->smmu_tcu_supply = NULL;
	}

	return 0;
}

int get_clk(struct arpp_device *arpp_dev, struct device *dev,
	struct device_node *dev_node)
{
	int ret;

	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return -EINVAL;
	}

	if (dev_node == NULL) {
		hiar_loge("dev node is NULL");
		return -EINVAL;
	}

	if (dev == NULL) {
		hiar_loge("dev is NULL");
		return -EINVAL;
	}

	ret = of_property_read_string_index(dev_node,
		"clock-names", 0, &arpp_dev->clk_name);
	if (ret != 0) {
		hiar_loge("failed to get clk name! ret=%d", ret);
		return -EINVAL;
	}
	hiar_logi("arpp clk is %s", arpp_dev->clk_name);

	arpp_dev->clk = devm_clk_get(dev, arpp_dev->clk_name);
	if (IS_ERR(arpp_dev->clk)) {
		hiar_loge("failed to get arpp clk");
		return -EINVAL;
	}

	ret = of_property_read_u32(dev_node, OF_ARPP_CLK_RATE_LOW,
		&arpp_dev->clk_rate_low);
	if (ret != 0) {
		hiar_loge("failed to get low rate, ret=%d", ret);
		return -EINVAL;
	}

	ret = of_property_read_u32(dev_node, OF_ARPP_CLK_RATE_MEDIUM,
		&arpp_dev->clk_rate_medium);
	if (ret != 0) {
		hiar_loge("failed to get low rate, ret=%d", ret);
		return -EINVAL;
	}

	ret = of_property_read_u32(dev_node, OF_ARPP_CLK_RATE_HIGH,
		&arpp_dev->clk_rate_high);
	if (ret != 0) {
		hiar_loge("failed to get low rate, ret=%d", ret);
		return -EINVAL;
	}

	ret = of_property_read_u32(dev_node, OF_ARPP_CLK_RATE_TURBO,
		&arpp_dev->clk_rate_turbo);
	if (ret != 0) {
		hiar_loge("failed to get median rate, ret=%d", ret);
		return -EINVAL;
	}

	ret = of_property_read_u32(dev_node, OF_ARPP_CLK_RATE_LOWTEMP,
		&arpp_dev->clk_rate_lowtemp);
	if (ret != 0) {
		hiar_loge("failed to get lowtemp rate, ret=%d", ret);
		return -EINVAL;
	}

	ret = of_property_read_u32(dev_node, OF_ARPP_CLK_RATE_PD,
		&arpp_dev->clk_rate_pd);
	if (ret != 0) {
		hiar_loge("failed to get pd rate, ret=%d", ret);
		return -EINVAL;
	}

	ret = of_property_read_u32(dev_node, OF_ARPP_CLK_RATE_LP,
		&arpp_dev->clk_rate_lp);
	if (ret != 0) {
		hiar_loge("failed to get lp rate, ret=%d", ret);
		return -EINVAL;
	}

	return 0;
}

int put_clk(struct arpp_device *arpp_dev, struct device *dev)
{
	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL!");
		return -EINVAL;
	}

	if (dev == NULL) {
		hiar_loge("dev is NULL");
		return -EINVAL;
	}

	if (arpp_dev->clk != NULL) {
		devm_clk_put(dev, arpp_dev->clk);
		arpp_dev->clk = NULL;
	}

	return 0;
}

static u32 get_clk_rate_from_level(struct arpp_device *arpp_dev, int level)
{
	u32 clk_rate;

	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return 0;
	}

	switch (level) {
	case CLK_LEVEL_LOW:
		hiar_logi("arpp clk rate to low level");
		clk_rate = arpp_dev->clk_rate_low;
		break;
	case CLK_LEVEL_MEDIUM:
		hiar_logi("arpp clk rate to medium level");
		clk_rate = arpp_dev->clk_rate_medium;
		break;
	case CLK_LEVEL_HIGH:
		hiar_logi("arpp clk rate to high level");
		clk_rate = arpp_dev->clk_rate_high;
		break;
	case CLK_LEVEL_TURBO:
		hiar_logi("arpp clk rate to turbo level");
		clk_rate = arpp_dev->clk_rate_turbo;
		break;
	case CLK_LEVEL_LP:
		hiar_logi("arpp clk rate to low power level");
		clk_rate = arpp_dev->clk_rate_lp;
		break;
	default:
		hiar_logw("level is %d, use default level", arpp_dev->clk_level);
		clk_rate = arpp_dev->clk_rate_low;
		break;
	}

	return clk_rate;
}

static int set_clk_rate_and_enable(struct arpp_device *arpp_dev)
{
	int ret;
	int cur_level;
	u32 clk_rate;

	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return -EINVAL;
	}

	if (arpp_dev->clk == NULL) {
		hiar_loge("clk is NULL!");
		return -EINVAL;
	}

	cur_level = arpp_dev->clk_level;
	clk_rate = get_clk_rate_from_level(arpp_dev, cur_level);
	if (clk_rate == 0)
		return -EINVAL;

	ret = clk_set_rate(arpp_dev->clk, (unsigned long)clk_rate);
	if (ret != 0) {
		hiar_loge("set clk rate failed, error=%d", ret);
		return ret;
	}

	hiar_logi("arpp set clk rate to %d", clk_rate);

	ret = clk_prepare_enable(arpp_dev->clk);
	if (ret != 0) {
		hiar_loge("clk prepare enable failed, error=%d", ret);
		return ret;
	}

	return 0;
}

static int clock_enable(struct arpp_device *arpp_dev)
{
	int ret;

	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return -EINVAL;
	}

	ret = set_clk_rate_and_enable(arpp_dev);
	if (ret != 0)
		goto low_temp_mode;

	return 0;

low_temp_mode:
	/*
	 * If arpp is in low temp and clk level is TURBO_LEVEL,
	 * move to next clk level.
	 */
	if (arpp_dev->clk_level == CLK_LEVEL_TURBO) {
		--arpp_dev->clk_level;
		return set_clk_rate_and_enable(arpp_dev);
	}
	return ret;
}

static void clock_disable(struct arpp_device *arpp_dev)
{
	int ret;
	u32 clk_rate;

	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return;
	}

	if (arpp_dev->clk == NULL) {
		hiar_loge("clk is NULL!");
		return;
	}

	clk_rate = arpp_dev->clk_rate_pd;

	ret = clk_set_rate(arpp_dev->clk, (unsigned long)clk_rate);
	if (ret != 0)
		hiar_logw("set pd clk rate fail, error=%d", ret);
	else
		hiar_logi("arpp set pd clk rate to %d", clk_rate);

	clk_disable_unprepare(arpp_dev->clk);
}

int change_clk_level(struct arpp_device *arpp_dev, int level)
{
	int ret;
	u32 clk_rate;

	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return -EINVAL;
	}

	if (arpp_dev->clk == NULL) {
		hiar_loge("clk is NULL!");
		return -EINVAL;
	}

	clk_rate = get_clk_rate_from_level(arpp_dev, level);
	if (clk_rate == 0)
		return -EINVAL;

	ret = clk_set_rate(arpp_dev->clk, (unsigned long)clk_rate);
	if (ret != 0)
		hiar_logw("change clk rate fail, error=%d", ret);

	return ret;
}

int power_up(struct arpp_device *arpp_dev)
{
	int ret;

	if (arpp_dev == NULL) {
		hiar_loge("arpp dev is NULL");
		return -EINVAL;
	}

	if (arpp_dev->media2_supply == NULL) {
		hiar_loge("media2_supply is NULL");
		return -EINVAL;
	}

	if (arpp_dev->arpp_supply == NULL) {
		hiar_loge("arpp supply is NULL");
		return -EINVAL;
	}

	if (arpp_dev->smmu_tcu_supply == NULL) {
		hiar_loge("smmu_tcu supply is NULL");
		return -EINVAL;
	}

	/* 1st step: power up media2 */
	ret = regulator_enable(arpp_dev->media2_supply);
	if (ret != 0) {
		hiar_loge("media2 power up failed, error=%d", ret);
		return ret;
	}

	/* 2nd step: power up media2 smmu tcu */
	ret = regulator_enable(arpp_dev->smmu_tcu_supply);
	if (ret != 0) {
		hiar_loge("smmu tcu power up failed, error=%d", ret);
		goto err_pd_media2;
	}

	/* 3rd step: set clk rate and open clk */
	ret = clock_enable(arpp_dev);
	if (ret != 0) {
		hiar_loge("clock_enable failed, error=%d", ret);
		goto err_pd_smmu;
	}

	/* 4th step: power down arpp */
	ret = regulator_enable(arpp_dev->arpp_supply);
	if (ret != 0) {
		hiar_loge("arpp power up failed, error=%d", ret);
		goto err_clock_disable;
	}

	/* 5th step: get wake lock */
	if (!arpp_dev->wake_lock->active) {
		__pm_stay_awake(arpp_dev->wake_lock);
		hiar_logi("arpp get wake lock");
	}

	return 0;

err_clock_disable:
	(void)clock_disable(arpp_dev);

err_pd_smmu:
	(void)regulator_disable(arpp_dev->smmu_tcu_supply);

err_pd_media2:
	(void)regulator_disable(arpp_dev->media2_supply);

	return ret;
}

int power_down(struct arpp_device *arpp_dev)
{
	int ret = 0;

	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return -EINVAL;
	}

	/* 1st step: power down arpp */
	if (arpp_dev->arpp_supply != NULL) {
		ret = regulator_disable(arpp_dev->arpp_supply);
		if (ret != 0)
			hiar_loge("arpp power down failed, error=%d", ret);
	}

	/* 2nd step: set pd rate, then close clk */
	clock_disable(arpp_dev);

	/* 3rd step: power down smmu tcu */
	if (arpp_dev->smmu_tcu_supply != NULL) {
		ret = regulator_disable(arpp_dev->smmu_tcu_supply);
		if (ret != 0)
			hiar_loge("smmu tcu power down failed, error=%d", ret);
	}

	/* 4th step: power down media2 */
	if (arpp_dev->media2_supply != NULL) {
		ret = regulator_disable(arpp_dev->media2_supply);
		if (ret != 0)
			hiar_loge("media2 power down failed, error=%d", ret);
	}

	/* 5th step: release wake lock */
	if (arpp_dev->wake_lock->active) {
		__pm_relax(arpp_dev->wake_lock);
		hiar_logi("arpp release wake lock");
	}

	return ret;
}

int get_irqs(struct arpp_device *arpp_dev, struct device_node *dev_node)
{
	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return -EINVAL;
	}

	if (dev_node == NULL) {
		hiar_loge("dev_node is NULL");
		return -EINVAL;
	}

	arpp_dev->ahc_normal_irq = irq_of_parse_and_map(dev_node, 0);
	if (arpp_dev->ahc_normal_irq == 0) {
		hiar_loge("ahc_normal_irq is invalid");
		return -1;
	}

	arpp_dev->ahc_error_irq = irq_of_parse_and_map(dev_node, 1);
	if (arpp_dev->ahc_error_irq == 0) {
		hiar_loge("ahc_error_irq is invalid");
		return -1;
	}

	hiar_logi("get ahc info: normal=%d, err=%d",
		arpp_dev->ahc_normal_irq, arpp_dev->ahc_error_irq);

	return 0;
}

int get_sid_ssid(struct arpp_device *arpp_dev, struct device_node *dev_node)
{
	int ret;
	/* count 2 is sid and ssid */
	const size_t count = 2;
	uint32_t sid_array[2];

	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return -EINVAL;
	}

	if (dev_node == NULL) {
		hiar_loge("dev_node is NULL");
		return -EINVAL;
	}

	ret = of_property_read_u32_array(dev_node, "sid-ssid", sid_array, count);
	if (ret != 0) {
		hiar_loge("get sid ssid failed");
		return -EINVAL;
	}

	arpp_dev->sid = sid_array[0];
	arpp_dev->ssid = sid_array[1];

	return 0;
}

static int get_hwacc_registers(struct arpp_device *arpp_dev,
	struct device_node *dev_node)
{
	struct arpp_iomem *io_info = NULL;

	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return -EINVAL;
	}

	if (dev_node == NULL) {
		hiar_loge("dev_node is NULL");
		return -EINVAL;
	}

	io_info = &arpp_dev->io_info;

	io_info->lba0_base = of_iomap(dev_node, 0);
	if (io_info->lba0_base == NULL) {
		hiar_loge("get lba0_base failed");
		return -EINVAL;
	}

	io_info->hwacc_ini0_cfg_base = of_iomap(dev_node, 1);
	if (io_info->hwacc_ini0_cfg_base == NULL) {
		hiar_loge("get hwacc_ini0_cfg_base failed");
		return -EINVAL;
	}

	io_info->hwacc1_cfg_base = of_iomap(dev_node, 2);
	if (io_info->hwacc1_cfg_base == NULL) {
		hiar_loge("get hwacc1_cfg_base failed");
		return -EINVAL;
	}

	io_info->hwacc0_cfg_base = of_iomap(dev_node, 3);
	if (io_info->hwacc0_cfg_base == NULL) {
		hiar_loge("get hwacc0_cfg failed");
		return -EINVAL;
	}

	io_info->ahc_ns_base = of_iomap(dev_node, 6);
	if (io_info->ahc_ns_base == NULL) {
		hiar_loge("get ahc_ns_base failed");
		return -EINVAL;
	}

	io_info->crg_base = of_iomap(dev_node, 7);
	if (io_info->crg_base == NULL) {
		hiar_loge("get crg_base failed");
		return -EINVAL;
	}

	io_info->hwacc_ini1_cfg_base = of_iomap(dev_node, 16);
	if (io_info->hwacc_ini1_cfg_base == NULL) {
		hiar_loge("get hwacc_ini1_cfg_base failed");
		return -EINVAL;
	}

	return 0;
}

static int get_ctrl_registers(struct arpp_device *arpp_dev,
	struct device_node *dev_node)
{
	struct arpp_iomem *io_info = NULL;

	if (dev_node == NULL) {
		hiar_loge("dev node is NULL");
		return -EINVAL;
	}

	if (arpp_dev == NULL) {
		hiar_loge("arpp dev is NULL");
		return -EINVAL;
	}

	io_info = &arpp_dev->io_info;

	io_info->tzpc_base = of_iomap(dev_node, 8);
	if (io_info->tzpc_base == NULL) {
		hiar_loge("get tzpc_base failed");
		return -EINVAL;
	}

	io_info->pctrl_base = of_iomap(dev_node, 9);
	if (io_info->pctrl_base == NULL) {
		hiar_loge("get pctrl_base failed");
		return -EINVAL;
	}

	io_info->sctrl_base = of_iomap(dev_node, 10);
	if (io_info->sctrl_base == NULL) {
		hiar_loge("get sctrl_base failed");
		return -EINVAL;
	}

	io_info->asc0_base = of_iomap(dev_node, 11);
	if (io_info->asc0_base == NULL) {
		hiar_loge("get asc0_base failed");
		return -EINVAL;
	}

	io_info->asc1_base = of_iomap(dev_node, 12);
	if (io_info->asc1_base == NULL) {
		hiar_loge("get asc1_base failed");
		return -EINVAL;
	}

	io_info->tbu_base = of_iomap(dev_node, 14);
	if (io_info->tbu_base == NULL) {
		hiar_loge("get tbu_base failed");
		return -EINVAL;
	}

	io_info->actrl_base = of_iomap(dev_node, 15);
	if (io_info->actrl_base == NULL) {
		hiar_loge("get actrl_base failed");
		return -EINVAL;
	}

	return 0;
}

static void put_hwacc_registers(struct arpp_device *arpp_dev)
{
	struct arpp_iomem *io_info = NULL;

	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return;
	}

	io_info = &arpp_dev->io_info;

	if (io_info->lba0_base != NULL) {
		iounmap(io_info->lba0_base);
		io_info->lba0_base = NULL;
	}

	if (io_info->hwacc_ini0_cfg_base != NULL) {
		iounmap(io_info->hwacc_ini0_cfg_base);
		io_info->hwacc_ini0_cfg_base = NULL;
	}

	if (io_info->hwacc1_cfg_base != NULL) {
		iounmap(io_info->hwacc1_cfg_base);
		io_info->hwacc1_cfg_base = NULL;
	}

	if (io_info->hwacc0_cfg_base != NULL) {
		iounmap(io_info->hwacc0_cfg_base);
		io_info->hwacc0_cfg_base = NULL;
	}

	if (io_info->ahc_ns_base != NULL) {
		iounmap(io_info->ahc_ns_base);
		io_info->ahc_ns_base = NULL;
	}

	if (io_info->crg_base != NULL) {
		iounmap(io_info->crg_base);
		io_info->crg_base = NULL;
	}

	if (io_info->hwacc_ini1_cfg_base != NULL) {
		iounmap(io_info->hwacc_ini1_cfg_base);
		io_info->hwacc_ini1_cfg_base = NULL;
	}
}

static void put_ctrl_registers(struct arpp_device *arpp_dev)
{
	struct arpp_iomem *io_info = NULL;

	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return;
	}

	io_info = &arpp_dev->io_info;

	if (io_info->tzpc_base != NULL) {
		iounmap(io_info->tzpc_base);
		io_info->tzpc_base = NULL;
	}

	if (io_info->pctrl_base != NULL) {
		iounmap(io_info->pctrl_base);
		io_info->pctrl_base = NULL;
	}

	if (io_info->sctrl_base != NULL) {
		iounmap(io_info->sctrl_base);
		io_info->sctrl_base = NULL;
	}

	if (io_info->asc0_base != NULL) {
		iounmap(io_info->asc0_base);
		io_info->asc0_base = NULL;
	}

	if (io_info->asc1_base != NULL) {
		iounmap(io_info->asc1_base);
		io_info->asc1_base = NULL;
	}

	if (io_info->tbu_base != NULL) {
		iounmap(io_info->tbu_base);
		io_info->tbu_base = NULL;
	}

	if (io_info->actrl_base != NULL) {
		iounmap(io_info->actrl_base);
		io_info->actrl_base = NULL;
	}
}

int get_all_registers(struct arpp_device *arpp_dev,
	struct device_node *dev_node)
{
	int ret;

	ret = get_hwacc_registers(arpp_dev, dev_node);
	if (ret != 0) {
		hiar_loge("get hwacc reg failed");
		goto err_put_all_registers;
	}

	ret = get_ctrl_registers(arpp_dev, dev_node);
	if (ret != 0) {
		hiar_loge("get ctrl regs failed");
		goto err_put_all_registers;
	}

	return 0;

err_put_all_registers:
	put_all_registers(arpp_dev);

	return ret;
}

void put_all_registers(struct arpp_device *arpp_dev)
{
	put_hwacc_registers(arpp_dev);
	put_ctrl_registers(arpp_dev);
}

void init_and_reset(struct arpp_device *arpp_dev)
{
	struct arpp_iomem *io_info = NULL;
	char __iomem *crg_base = NULL;
	char __iomem *pctrl_base = NULL;
	char __iomem *hwacc0_base = NULL;
	char __iomem *hwacc1_base = NULL;

	hiar_logi("+");

	if (arpp_dev == NULL) {
		hiar_loge("arpp dev is NULL");
		return;
	}

	io_info = &arpp_dev->io_info;
	crg_base = io_info->crg_base;
	if (crg_base == NULL) {
		hiar_loge("crg_base is NULL");
		return;
	}

	hwacc0_base = io_info->hwacc0_cfg_base;
	if (hwacc0_base == NULL) {
		hiar_loge("hwacc0_base is NULL");
		return;
	}

	hwacc1_base = io_info->hwacc1_cfg_base;
	if (hwacc1_base == NULL) {
		hiar_loge("hwacc1_base is NULL");
		return;
	}

	pctrl_base = io_info->pctrl_base;
	if (pctrl_base == NULL) {
		hiar_loge("pctrl_base is NULL");
		return;
	}

	/* disable clk of all module */
	writel(0xFFF0200F, crg_base + PERI_CLK_DISABLE);
	/* soft reset */
	writel(0x20000000, crg_base + PERI_SRST);
	/* reset all module */
	writel(0x1FF02005, crg_base + PERI_SRST);
	/* enable clk of all module */
	writel(0xFFF0200F, crg_base + PERI_CLK_ENABLE);

	/* lba0 spram enable */
	writel(0x00015858, pctrl_base + FUNC0_SPRAM_CTRL);
	/* lba0 tpram enable */
	writel(0x00000850, pctrl_base + FUNC0_TPRAM_CTRL);
	/* lba1 spram enable */
	writel(0x00015858, pctrl_base + FUNC1_SPRAM_CTRL);
	/* lba1 tpram enable */
	writel(0x00000850, pctrl_base + FUNC1_TPRAM_CTRL);

	/* open lba0 debug3 */
	writel(0x00000001, hwacc0_base + DEBUG_REG3);
	/* open lba1 debug3 */
	writel(0x00000001, hwacc1_base + DEBUG_REG3);

	hiar_logi("-");
}

void open_auto_lp_mode(struct arpp_device *arpp_dev)
{
	struct arpp_iomem *io_info = NULL;
	char __iomem *pctrl_base = NULL;
	char __iomem *sctrl_base = NULL;
	char __iomem *hwacc0_base = NULL;
	char __iomem *hwacc1_base = NULL;

	hiar_logi("+");

	if (arpp_dev == NULL) {
		hiar_loge("arpp dev is NULL");
		return;
	}

	io_info = &arpp_dev->io_info;
	pctrl_base = io_info->pctrl_base;
	if (pctrl_base == NULL) {
		hiar_loge("pctrl_base is NULL");
		return;
	}

	sctrl_base = io_info->sctrl_base;
	if (sctrl_base == NULL) {
		hiar_loge("arpp_sctrl_base is NULL");
		return;
	}

	hwacc0_base = io_info->hwacc0_cfg_base;
	if (hwacc0_base == NULL) {
		hiar_loge("hwacc0 base is NULL");
		return;
	}

	hwacc1_base = io_info->hwacc1_cfg_base;
	if (hwacc1_base == NULL) {
		hiar_loge("hwacc1 base is NULL");
		return;
	}

	/* axi auto clk enable*/
	writel(0xFFFFFFFF, pctrl_base + DW_BUS_TOP_PERI_CG_EN);
	writel(0xFFFFFFFF, sctrl_base + DW_BUS_TOP_DATA_CG_EN);
	writel(0xFFFFFFFF, sctrl_base + DW_BUS_TOP_QIC_CG_EN);
	/* lba0 auto clk enable*/
	writel(0xFFFFFFFF, hwacc0_base + DW_BUS_LITE_CG_EN);
	/* lba1 auto clk enable*/
	writel(0xFFFFFFFF, hwacc1_base + DW_BUS_LITE_CG_EN);

	hiar_logi("-");
}

void mask_and_clear_isr(struct arpp_device *arpp_dev)
{
	struct arpp_iomem *io_info = NULL;
	char __iomem *pctrl_base = NULL;

	if (arpp_dev == NULL) {
		hiar_loge("arpp_dev is NULL");
		return;
	}

	io_info = &arpp_dev->io_info;
	pctrl_base = io_info->pctrl_base;
	if (pctrl_base == NULL) {
		hiar_loge("pctrl_base is NULL");
		return;
	}

	/* mask all isr */
	writel(0xff, pctrl_base + INTR_MASK0);
	writel(0xff, pctrl_base + INTR_MASK1);
	writel(0xff, pctrl_base + INTR_MASK2);
	writel(0xff, pctrl_base + INTR_MASK3);
	writel(0xff, pctrl_base + INTR_MASK4);
	writel(0xff, pctrl_base + INTR_MASK5);
	writel(0xff, pctrl_base + INTR_MASK6);
	writel(0xff, pctrl_base + INTR_MASK7);
	writel(0xff, pctrl_base + INTR_MASK8);
	writel(0xff, pctrl_base + INTR_MASK9);

	/* clear all isr */
	writel(0xff, pctrl_base + INTR_CLEAR0);
	writel(0xff, pctrl_base + INTR_CLEAR1);
	writel(0xff, pctrl_base + INTR_CLEAR2);
	writel(0xff, pctrl_base + INTR_CLEAR3);
	writel(0xff, pctrl_base + INTR_CLEAR4);
	writel(0xff, pctrl_base + INTR_CLEAR5);
	writel(0xff, pctrl_base + INTR_CLEAR6);
	writel(0xff, pctrl_base + INTR_CLEAR7);
	writel(0xff, pctrl_base + INTR_CLEAR8);
	writel(0xff, pctrl_base + INTR_CLEAR9);

	writel(0x00, pctrl_base + INTR_CLEAR0);
	writel(0x00, pctrl_base + INTR_CLEAR1);
	writel(0x00, pctrl_base + INTR_CLEAR2);
	writel(0x00, pctrl_base + INTR_CLEAR3);
	writel(0x00, pctrl_base + INTR_CLEAR4);
	writel(0x00, pctrl_base + INTR_CLEAR5);
	writel(0x00, pctrl_base + INTR_CLEAR6);
	writel(0x00, pctrl_base + INTR_CLEAR7);
	writel(0x00, pctrl_base + INTR_CLEAR8);
	writel(0x00, pctrl_base + INTR_CLEAR9);
}
