/*
 * ISP CPE
 *
 * Copyright (c) 2017 IPP Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/of_irq.h>
#include <linux/iommu.h>
#include <linux/pm_wakeup.h>
#include <linux/ion.h>
#include <linux/platform_drivers/mm_ion.h>
#include <linux/genalloc.h>
#include <linux/mm_iommu.h>
#include <linux/version.h>
#include <platform_include/isp/linux/hipp_common.h>
#include <linux/delay.h>
#include <linux/types.h>
#include "ipp.h"
#include "ipp_core.h"

struct hispcpe_s *hispcpe_dev;

unsigned int kmsgcat_mask = (INFO_BIT | ERROR_BIT); /* DEBUG_BIT not set */
/*lint -e21 -e846 -e514 -e778 -e866 -e84 -e429 -e613 -e668*/
module_param_named(kmsgcat_mask, kmsgcat_mask, int, 0664);

void __iomem *hipp_get_regaddr(unsigned int type)
{
	struct hispcpe_s *dev = hispcpe_dev;

	if (type >= hipp_min(MAX_HISP_CPE_REG, dev->reg_num)) {
		pr_err("[%s] unsupported type.0x%x\n", __func__, type);
		return NULL;
	}

	return dev->reg[type] ? dev->reg[type] : NULL; /*lint !e661 !e662 */
}

int hispcpe_reg_set(unsigned int mode, unsigned int offset, unsigned int value)
{
	struct hispcpe_s *dev = hispcpe_dev;
	void __iomem *reg_base = NULL;

	if (dev == NULL) {
		pr_err("[%s] Failed : dev.NULL\n", __func__);
		return -1;
	}

	if (mode > MAX_HISP_CPE_REG - 1) {
		pr_err("[%s] Failed : mode.%d\n", __func__, mode);
		return -1;
	}

	D("mode = %d, value = 0x%x\n", mode, value);
	reg_base = dev->reg[mode];

	if (reg_base == NULL) {
		pr_err("[%s] Failed : reg.NULL, mode.%d\n", __func__, mode);
		return -1;
	}

	writel(value, reg_base + offset);
	return ISP_IPP_OK;
}

unsigned int hispcpe_reg_get(unsigned int mode, unsigned int offset)
{
	struct hispcpe_s *dev = hispcpe_dev;
	unsigned int value;
	void __iomem *reg_base = NULL;

	if (dev == NULL) {
		pr_err("[%s] Failed : dev.NULL\n", __func__);
		return 1;
	}

	if (mode > MAX_HISP_CPE_REG - 1) {
		pr_err("[%s] Failed : mode.%d\n", __func__, mode);
		return ISP_IPP_OK;
	}

	reg_base = dev->reg[mode];
	if (reg_base == NULL) {
		pr_err("[%s] Failed : reg.NULL, mode.%d\n", __func__, mode);
		return 1;
	}

	value = readl(reg_base + offset);
	return value;
}

unsigned int get_clk_rate(unsigned int mode, unsigned int index)
{
	unsigned int type;
	unsigned int rate = 0;
	struct hispcpe_s *dev = hispcpe_dev;

	if ((mode > (IPP_CLK_MAX - 1)) || (index > (MAX_CLK_RATE - 1))) {
		pr_err("[%s] Failed : mode.%d, index.%d\n",
			__func__, mode, index);
		return rate;
	}

	if (dev == NULL) {
		pr_err("[%s] Failed : dev.NULL\n", __func__);
		return rate;
	}

	type = mode * MAX_CLK_RATE + index;

	switch (type) {
	case HIPPCORE_RATE_TBR:
		rate = dev->ippclk_value[IPPCORE_CLK];
		break;

	case HIPPCORE_RATE_NOR:
		rate = dev->ippclk_normal[IPPCORE_CLK];
		break;

	case HIPPCORE_RATE_HSVS:
		rate = dev->ippclk_hsvs[IPPCORE_CLK];
		break;

	case HIPPCORE_RATE_SVS:
		rate = dev->ippclk_svs[IPPCORE_CLK];
		break;

	case HIPPCORE_RATE_OFF:
		rate = dev->ippclk_off[IPPCORE_CLK];
		break;

	default:
		break;
	}

	return rate;
}

static int ipp_setclk_enable(struct hispcpe_s *dev, unsigned int devtype)
{
	int ret;
	int i = 0;

	if (dev == NULL) {
		pr_err("[%s] Failed : dev.NULL\n", __func__);
		return -EINVAL;
	}

	if (dev->ippclk[IPPCORE_CLK] == NULL) {
		pr_err("[%s] Failed : jpgclk.NULL\n", __func__);
		return -EINVAL;
	}

	if (dev->ippclk_off[IPPCORE_CLK] == 0) {
		pr_err("[%s] Failed : jpgclk.value.0\n", __func__);
		return -EINVAL;
	}

	if (devtype > MAX_HIPP_COM - 1) {
		pr_err("[%s] Failed : devtype.%d\n", __func__, devtype);
		return -EINVAL;
	}

	for (i = 0; i < MAX_HIPP_COM; i++)
		dev->jpeg_rate[i] = get_clk_rate(IPPCORE_CLK, CLK_RATE_OFF);

	ret = clk_set_rate(dev->ippclk[IPPCORE_CLK],
			   dev->ippclk_off[IPPCORE_CLK]);
	if (ret < 0) {
		pr_err("[%s] Failed: clk_set_rate %d.%d\n",
		       __func__, dev->ippclk_off[IPPCORE_CLK], ret);
		return -EINVAL;
	}

	ret = clk_prepare_enable(dev->ippclk[IPPCORE_CLK]);
	if (ret < 0) {
		pr_err("[%s] Failed: clk_prepare_enable.%d\n", __func__, ret);
		return -EINVAL;
	}

	dev->jpeg_current_rate = dev->ippclk_off[IPPCORE_CLK];
	dev->jpeg_rate[devtype] = dev->ippclk_off[IPPCORE_CLK];

	return ISP_IPP_OK;
}

static int ipp_setclk_disable(struct hispcpe_s *dev, unsigned int devtype)
{
	int ret;
	unsigned int i = 0;

	if (dev == NULL) {
		pr_err("[%s] Failed : dev.NULL\n", __func__);
		return -EINVAL;
	}

	if (dev->ippclk[IPPCORE_CLK] == NULL) {
		pr_err("[%s] Failed : jpgclk.NULL\n", __func__);
		return -EINVAL;
	}

	if (devtype > MAX_HIPP_COM - 1) {
		pr_err("[%s] Failed : devtype.%d\n", __func__, devtype);
		return -EINVAL;
	}

	ret = clk_set_rate(dev->ippclk[IPPCORE_CLK],
			   dev->ippclk_off[IPPCORE_CLK]);
	if (ret < 0) {
		pr_err("[%s] Failed: clk_set_rate %d.%d\n",
			__func__, dev->ippclk_off[IPPCORE_CLK], ret);
		return ret;
	}

	clk_disable_unprepare(dev->ippclk[IPPCORE_CLK]);
	dev->jpeg_rate[devtype] = dev->ippclk_off[IPPCORE_CLK];
	dev->jpeg_current_rate = dev->ippclk_off[IPPCORE_CLK];

	for (i = 0; i < MAX_HIPP_COM; i++) {
		if (dev->jpeg_rate[i] != dev->ippclk_off[IPPCORE_CLK])
			pr_err("[%s] ippclk_check: type.%d, rate.%d.%d M\n",
				__func__, i, dev->jpeg_rate[i] / 1000000,
				dev->jpeg_rate[i] % 1000000);
	}

	return ISP_IPP_OK;
}

int dataflow_cvdr_global_config(void)
{
	unsigned int reg_val;
	void __iomem *crg_base;

	crg_base = hipp_get_regaddr(CVDR_REG);
	if (crg_base == NULL ) {
		pr_err("[%s] Failed : hipp_get_regaddr\n", __func__);
		return -EINVAL;
	}

	reg_val = (1 << 0)	
	    | (34 << 8)		
	    | (63 << 16)	
	    | (31 << 24);
	writel(reg_val, crg_base + 0x0);

	return ISP_IPP_OK;
}

int ipp_cfg_qos_reg(void)
{
	unsigned int ipp_qos_value = 0;
	void __iomem *crg_base;

	crg_base = hipp_get_regaddr(CPE_TOP);
	if (crg_base == NULL ) {
		pr_err("[%s] Failed : hipp_get_regaddr\n", __func__);
		return -EINVAL;
	}

	writel(0x00000000, crg_base + 0x80);
	writel(0x08000205, crg_base + 0x84);
	writel(0x00000000, crg_base + 0x88);
	writel(0x08000205, crg_base + 0x8c);

	ipp_qos_value = readl(crg_base + 0x80);
	pr_info("[%s] value = 0x%x (0x0 wanted)\n",
		__func__, ipp_qos_value);

	ipp_qos_value = readl(crg_base + 0x84);
	pr_info("[%s] value = 0x%x (0x08000205 wanted)\n",
		__func__, ipp_qos_value);

	ipp_qos_value = readl(crg_base + 0x88);
	pr_info("[%s] value = 0x%x (0x0 wanted)\n",
		__func__, ipp_qos_value);

	ipp_qos_value = readl(crg_base + 0x8c);
	pr_info("[%s] value = 0x%x (0x08000205 wanted)\n",
		__func__, ipp_qos_value);

	return ISP_IPP_OK;
}

int hipp_adapter_cfg_qos_cvdr(void)
{
	int ret;

	ret = dataflow_cvdr_global_config();
	if (ret < 0) {
		pr_err("[%s] Failed: dataflow_cvdr_global_config.%d\n",
		       __func__, ret);
		return ret;
	}

	ret = ipp_cfg_qos_reg();
	if (ret < 0) {
		pr_err("[%s] Failed : ipp_cfg_qos_reg.\n", __func__);
		return ret;
	}

	return ISP_IPP_OK;
}

int hipp_core_powerup(unsigned int type)
{
	int ret;
	struct hispcpe_s *dev = hispcpe_dev;

	if (dev == NULL)
		return -ENODEV;

	ret = regulator_enable(dev->media2_supply);
	if (ret != 0) {
		pr_err("[%s] Failed: media2.%d\n", __func__, ret);
		return -ENODEV;
	}

	ret = regulator_enable(dev->smmu_tcu_supply);
	if (ret != 0) {
		pr_err("[%s] Failed: smmu tcu.%d\n", __func__, ret);
		goto fail_tcu_poweron;
	}

	ret = ipp_setclk_enable(dev, type);
	if (ret < 0) {
		pr_err("[%s] Failed: ipp_setclk_enable.%d\n", __func__, ret);
		goto fail_ipp_setclk;
	}

	ret = regulator_enable(dev->cpe_supply);
	if (ret != 0) {
		pr_err("[%s] Failed: cpe.%d\n", __func__, ret);
		goto fail_cpe_supply;
	}

	ret = hipp_adapter_cfg_qos_cvdr();
	if (ret != 0) {
		pr_err("[%s] Failed : hipp_adapter_cfg_qos_cvdr\n", __func__);
		goto fail_ipp_cfg;
	}

	return 0;

fail_ipp_cfg:
	ret = regulator_disable(dev->cpe_supply);
	if (ret != 0)
		pr_err("[%s]Failed: cpe disable.%d\n", __func__, ret);

fail_cpe_supply:
	ret = ipp_setclk_disable(dev, type);
	if (ret != 0)
		pr_err("[%s] Failed : ipp_setclk_disable.%d\n", __func__, ret);

fail_ipp_setclk:
	ret = regulator_disable(dev->smmu_tcu_supply);
	if (ret != 0)
		pr_err("[%s] Failed: smmu tcu disable.%d\n", __func__, ret);

fail_tcu_poweron:
	ret = regulator_disable(dev->media2_supply);
	if (ret != 0)
		pr_err("[%s] Failed: media2 disable.%d\n", __func__, ret);

	return -EINVAL;
}

int hipp_core_powerdn(unsigned int type)
{
	int ret;
	struct hispcpe_s *dev = hispcpe_dev;

	if (dev == NULL) {
		pr_err("[%s] Failed: hispcpe_dev NULL\n", __func__);
		return -ENODEV;
	}

	ret = regulator_disable(dev->cpe_supply);
	if (ret != 0)
		pr_err("[%s] Failed: orb regulator_disable.%d\n",
			__func__, ret);

	ret = ipp_setclk_disable(dev, type);
	if (ret < 0)
		pr_err("[%s] Failed: ipp_setclk_enable.%d\n",
			__func__, ret);

	ret = regulator_disable(dev->smmu_tcu_supply);
	if (ret != 0)
		pr_err("[%s] Failed: smmu tcu regulator_enable.%d\n",
			__func__, ret);

	ret = regulator_disable(dev->media2_supply);
	if (ret != 0)
		pr_err("[%s] Failed: vcodec regulator_enable.%d\n",
			__func__, ret);

	return ISP_IPP_OK;
}

static int is_need_transition_rate(
	unsigned int cur, unsigned int *tran_rate)
{
	int i = 0;

	for (i = 0; i < HIPP_TRAN_NUM; i++) {
		if (jpeg_trans_rate[i].source_rate == cur) {
			*tran_rate = jpeg_trans_rate[i].transition_rate;
			pr_info("[%s] : transition_rate.%d.%d M\n",
			       __func__, *tran_rate / 1000000,
			       *tran_rate % 1000000);

			return ISP_IPP_OK;
		}
	}

	return ISP_IPP_ERR;
}

static unsigned int hipp_set_rate_check(unsigned int devtype,
	unsigned int clktype)
{
	struct hispcpe_s *dev = hispcpe_dev;
	unsigned int set_rate;

	if (dev == NULL) {
		pr_err("[%s] Failed : hispcpe_dev.NULL\n", __func__);
		return 0;
	}

	if (clktype > (MAX_CLK_RATE - 1)) {
		pr_err("[%s] Failed : clktype.%d\n", __func__, clktype);
		return 0;
	}

	if (devtype > (MAX_HIPP_COM - 1)) {
		pr_err("[%s] Failed : devtype.%d\n", __func__, devtype);
		return 0;
	}

	set_rate = get_clk_rate(IPPCORE_CLK, clktype);

	return set_rate;
}

int hipp_set_rate(unsigned int devtype, unsigned int clktype)
{
	struct hispcpe_s *dev = hispcpe_dev;
	unsigned int set_rate;
	unsigned int max_rate;
	unsigned int transition_rate = 0;
	int ret;
	unsigned int jpeg_rate_buf[MAX_HIPP_COM] = { 0 };
	int i = 0;

	set_rate = hipp_set_rate_check(devtype, clktype);
	if (set_rate == 0) {
		pr_err("[%s] Failed : hipp_set_rate_check\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < MAX_HIPP_COM; i++)
		jpeg_rate_buf[i] = dev->jpeg_rate[i];

	jpeg_rate_buf[devtype] = set_rate;
	max_rate = jpeg_rate_buf[0];

	for (i = 1; i < MAX_HIPP_COM; i++) {
		if (max_rate < jpeg_rate_buf[i])
			max_rate = jpeg_rate_buf[i];
	}

	pr_info("[%s]: set_rate.%d.%dM, jpg_cur_rate.%d.%dM, max_rate.%d.%dM\n",
	     __func__, set_rate / 1000000, set_rate % 1000000,
	     dev->jpeg_current_rate / 1000000, dev->jpeg_current_rate % 1000000,
	     max_rate / 1000000, max_rate % 1000000);

	if (max_rate == dev->jpeg_current_rate) {
		pr_info("[%s] : no need change jpgclk.\n", __func__);
		dev->jpeg_rate[devtype] = set_rate;
		return ISP_IPP_OK;
	}

	ret = is_need_transition_rate(dev->jpeg_current_rate, &transition_rate);
	if (ret == 0) {
		ret = clk_set_rate(dev->ippclk[IPPCORE_CLK], transition_rate);
		if (ret < 0) {
			pr_err("[%s] Failed: set transition_rate\n", __func__);
			return ISP_IPP_ERR;
		}

		dev->jpeg_current_rate = transition_rate;
	}

	ret = clk_set_rate(dev->ippclk[IPPCORE_CLK], set_rate);
	if (ret < 0) {
		pr_err("[%s] Failed: set_rate.%d.%d M\n",
			__func__, set_rate / 1000000, set_rate % 1000000);
		return ISP_IPP_ERR;
	}

	dev->jpeg_current_rate = set_rate;
	dev->jpeg_rate[devtype] = set_rate;
	return ISP_IPP_OK;
}

static int hispcpe_ioremap_reg(struct hispcpe_s *dev)
{
	struct device *device = NULL;
	int i = 0;
	unsigned int min;

	D("+\n");
	device = &dev->pdev->dev;

	min = hipp_min(MAX_HISP_CPE_REG, dev->reg_num);
	for (i = 0; i < min; i++) { /*lint !e574 */
		dev->reg[i] = devm_ioremap_resource(device, dev->r[i]);

		if (dev->reg[i] == NULL) {
			pr_err("[%s] Failed : %d.devm_ioremap_resource.%pK\n",
				__func__, i, dev->reg[i]);
			return -ENOMEM;
		}
	}

	D("-\n");
	return ISP_IPP_OK;
}

static int hispcpe_resource_init(struct hispcpe_s *dev)
{
	int ret;

	ret = hispcpe_ioremap_reg(dev);
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_ioremap_reg.%d\n",
			__func__, ret);
		return ret;
	}

	return ISP_IPP_OK;
}

static int hispcpe_getdts_pwr(struct hispcpe_s *dev)
{
	struct device *device = NULL;

	if ((dev == NULL) || (dev->pdev == NULL)) {
		pr_err("[%s] Failed : dev or pdev.NULL\n", __func__);
		return -ENXIO;
	}

	device = &dev->pdev->dev;

	dev->media2_supply = devm_regulator_get(device, "ipp-media2");
	if (IS_ERR(dev->media2_supply)) {
		pr_err("[%s] Failed : MEDIA2 devm_regulator_get.%pK\n",
			__func__, dev->media2_supply);
		return -EINVAL;
	}

	dev->cpe_supply = devm_regulator_get(device, "ipp-cpe");
	if (IS_ERR(dev->cpe_supply)) {
		pr_err("[%s] Failed : CPE devm_regulator_get.%pK\n",
			__func__, dev->cpe_supply);
		return -EINVAL;
	}

	dev->smmu_tcu_supply = devm_regulator_get(device, "ipp-smmu-tcu");
	if (IS_ERR(dev->smmu_tcu_supply)) {
		pr_err("[%s] Failed : smmu tcu devm_regulator_get.%pK\n",
			__func__, dev->smmu_tcu_supply);
		return -EINVAL;
	}

	pr_info("[%s] Hisp CPE cpe_supply.%pK\n", __func__, dev->cpe_supply);

	dev->ippclk[IPPCORE_CLK] = devm_clk_get(device, "clk_jpg_func");
	if (IS_ERR(dev->ippclk[IPPCORE_CLK])) {
		pr_err("[%s] Failed: get jpg_clk", __func__);
		return -EINVAL;
	}

	return 0;
}

static int hispcpe_getdts_clk(struct hispcpe_s *dev)
{
	struct device_node *np = NULL;
	int ret;

	if ((dev == NULL) || (dev->pdev == NULL)) {
		pr_err("[%s] Failed : dev or pdev.NULL\n", __func__);
		return -ENXIO;
	}

	np = dev->pdev->dev.of_node;
	if (np == NULL) {
		pr_err("%s: of node NULL", __func__);
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "clock-num",
		(unsigned int *)(&dev->clock_num));
	if (ret < 0) {
		pr_err("[%s] Failed: clock-num.%d\n", __func__, ret);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "clock-value",
		dev->ippclk_value, dev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: clock-value.%d\n", __func__, ret);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "clock-value-normal",
		dev->ippclk_normal, dev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: clock nor.%d\n", __func__, ret);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "clock-value-hsvs",
		dev->ippclk_hsvs, dev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: clock hsvs.%d\n", __func__, ret);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "clock-value-svs",
		dev->ippclk_svs, dev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: clock svs.%d\n", __func__, ret);
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "clock-value-off",
		dev->ippclk_off, dev->clock_num);
	if (ret < 0) {
		pr_err("[%s] Failed: clock off.%d\n", __func__, ret);
		return -EINVAL;
	}

	return 0;
}

static int hispcpe_getdts_reg(struct hispcpe_s *dev)
{
	struct device *device = NULL;
	int i = 0, ret;
	unsigned int min;

	D("+\n");

	if ((dev == NULL) || (dev->pdev == NULL)) {
		pr_err("[%s] Failed : dev or pdev.NULL\n", __func__);
		return -ENXIO;
	}

	device = &dev->pdev->dev;

	ret = of_property_read_u32(device->of_node, "reg-num",
		(unsigned int *)(&dev->reg_num));
	if (ret < 0) {
		pr_err("[%s] Failed: reg-num.%d\n", __func__, ret);
		return -EINVAL;
	}
	pr_info("[%s] Hipp reg_num.%d\n", __func__, dev->reg_num);

	min = hipp_min(MAX_HISP_CPE_REG, dev->reg_num);
	for (i = 0; i < min; i++) { /*lint !e574 */
		dev->r[i] = platform_get_resource(dev->pdev, IORESOURCE_MEM, i);
		if (dev->r[i] == NULL) {
			pr_err("[%s] Failed : platform_get_resource.%pK\n",
				__func__, dev->r[i]);
			return -ENXIO;
		}
	}

	ret = of_property_read_u32(device->of_node, "sid-num",
		(unsigned int *)(&dev->sid));
	if (ret < 0) {
		pr_err("[%s] Failed: ret.%d\n", __func__, ret);
		return -EINVAL;
	}
	ret = of_property_read_u32(device->of_node, "ssid-num",
		(unsigned int *)(&dev->ssid));
	if (ret < 0) {
		pr_err("[%s] Failed: ret.%d\n", __func__, ret);
		return -EINVAL;
	}

	I("-\n");
	return ISP_IPP_OK;
}

static int hispcpe_getdts(struct hispcpe_s *dev)
{
	int ret;

	ret = hispcpe_getdts_pwr(dev);
	if (ret < 0) {
		pr_err("[%s] Failed : hispcpe_getdts_pwr.%d\n", __func__, ret);
		return ret;
	}

	ret = hispcpe_getdts_clk(dev);
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_getdts_clk.%d\n", __func__, ret);
		return ret;
	}

	ret = hispcpe_getdts_reg(dev);
	if (ret != 0) {
		pr_err("[%s] Failed : reg.%d\n", __func__, ret);
		return ret;
	}

	return ISP_IPP_OK;
}

static int hispcpe_probe(struct platform_device *pdev)
{
	struct hispcpe_s *dev = NULL;
	int ret;

	pr_info("[%s] +\n", __func__);

	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));//lint !e598
	if (ret < 0) {
		pr_err("[%s] Failed : dma_set\n", __func__);
		return ret;
	}

	dev = kzalloc(sizeof(struct hispcpe_s), GFP_KERNEL);
	if (dev == NULL)
		return -ENOMEM;

	dev->pdev = pdev;
	platform_set_drvdata(pdev, dev);

	ret = hispcpe_getdts(dev);
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_getdts.%d\n", __func__, ret);
		goto free_dev;
	}

	ret = hispcpe_resource_init(dev);
	if (ret != 0) {
		pr_err("[%s] Failed : resource.%d\n", __func__, ret);
		goto free_dev;
	}

	dev->initialized = 1;
	hispcpe_dev = dev;
	pr_info("[%s] -\n", __func__);

	return ISP_IPP_OK;

free_dev:
	kfree(dev);

	return -ENODEV;
}

static int hispcpe_remove(struct platform_device *pdev)
{
	struct hispcpe_s *dev = NULL;
	I("+\n");

	dev = (struct hispcpe_s *)platform_get_drvdata(pdev);
	if (dev == NULL) {
		pr_err("[%s] Failed : drvdata, pdev.%pK\n", __func__, pdev);
		return -ENODEV;
	}

	dev->initialized = 0;
	kfree(dev);
	dev = NULL;

	I("-\n");
	return ISP_IPP_OK;
}

#ifdef CONFIG_OF
static const struct of_device_id ipp_of_id[] = {
	{.compatible = DTS_NAME_IPP},
	{}
};
#endif

static struct platform_driver hispcpe_pdrvr = {
	.probe          = hispcpe_probe,
	.remove         = hispcpe_remove,
	.driver         = {
		.name           = "ipp",
		.owner          = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(ipp_of_id),
#endif
	},
};

module_platform_driver(hispcpe_pdrvr);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ISP CPE Driver");
MODULE_AUTHOR("isp");

/*lint +e21 +e846 +e514 +e778 +e866 +e84 +e429 +e613 +e668*/
