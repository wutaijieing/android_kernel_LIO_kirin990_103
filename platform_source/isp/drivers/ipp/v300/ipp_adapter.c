/*
 * Hisi ISP CPE
 *
 * Copyright (c) 2017 Hisilicon Technologies CO., Ltd.
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
#include <linux/dma-buf.h>
#include "gf.h"
#include "vbk.h"
#include "orb.h"
#include "reorder.h"
#include "compare.h"
#include "orb_enh.h"
#include "mc.h"
#include "adapter_common.h"
#include "ipp.h"
#include "ipp_top_drv_priv.h"
#include "ipp_top_reg_offset.h"
#include "memory.h"
#include "ipp_core.h"

static struct hipp_adapter_s *g_hipp_adapter;

static int hispcpe_wait(struct hipp_adapter_s *dev,
		enum hipp_wait_mode_type_e wait_mode)
{
	long ret = 0;
	int times = 0;
	unsigned long timeout;

	D("+\n");

	if (dev == NULL) {
		pr_err("[%s] Failed : dev.%pK\n", __func__, dev);
		return -EINVAL;
	}

	timeout = msecs_to_jiffies(HISP_CPE_TIMEOUT_MS);

	while (1) {
		if (wait_mode == WAIT_MODE_GF)
			ret = wait_event_interruptible_timeout(dev->gf_wait,
							     dev->gf_ready,
							     timeout);
		else if (wait_mode == WAIT_MODE_ORB)
			ret = wait_event_interruptible_timeout(dev->orb_wait,
							     dev->orb_ready,
							     timeout);
		else if (wait_mode == WAIT_MODE_RDR)
			ret = wait_event_interruptible_timeout(dev->reorder_wait,
							     dev->reorder_ready,
							     timeout);
		else if (wait_mode == WAIT_MODE_CMP)
			ret = wait_event_interruptible_timeout(dev->compare_wait,
							     dev->compare_ready,
							     timeout);
		else if (wait_mode == WAIT_MODE_VBK)
			ret = wait_event_interruptible_timeout(dev->vbk_wait,
							     dev->vbk_ready,
							     timeout);
		else if (wait_mode == WAIT_MODE_ENH)
			ret = wait_event_interruptible_timeout(dev->orb_enh_wait,
							     dev->orb_enh_ready,
							     timeout);
		else if (wait_mode == WAIT_MODE_MC)
			ret = wait_event_interruptible_timeout(dev->mc_wait,
							     dev->mc_ready,
							     timeout);

		if ((ret == -ERESTARTSYS) && (times++ < 200))
			msleep(5);
		else
			break;
	}

	if (ret <= 0) {
		pr_err("[%s] Failed :mode.%d, ret.%ld, times.%d\n",
		       __func__, wait_mode, ret, times);
		return -EINVAL;
	}
	pr_err("[%s] :wait_mode.%d, ret.%ld\n", __func__, wait_mode, ret);

	D("-\n");
	return ISP_IPP_OK;
}

irqreturn_t hispcpe_irq_handler(int irq, void *devid)
{
	struct hipp_adapter_s *dev = NULL;
	unsigned int value = 0;
	void __iomem *crg_base;

	pr_debug("[%s] +\n", __func__);

	dev = (struct hipp_adapter_s *)devid;
	if (dev != g_hipp_adapter) {
		pr_err("[%s] Failed : dev.%pK\n", __func__, dev);
		return IRQ_NONE;
	}

	crg_base = hipp_get_regaddr(CPE_TOP);
	if (crg_base == NULL ) {
		pr_err("[%s] Failed : hipp_get_regaddr\n", __func__);
		return IRQ_NONE;
	}

	if (irq == dev->irq) {
		value = readl(crg_base + IPP_TOP_CMDLST_IRQ_STA_0_REG);
		value = readl(crg_base + IPP_TOP_CMDLST_IRQ_STA_0_REG);
		value &= 0xFFFF;
		pr_info("IRQ VALUE = 0x%08x\n", value);
		writel(value, crg_base + IPP_TOP_CMDLST_IRQ_CLR_0_REG);

		if (value & (0x1 << IPP_GF_CHANNEL_ID)) {
			dev->gf_ready = 1;
			wake_up_interruptible(&dev->gf_wait);
		}

		if (value & (0x1 << IPP_VBK_CHANNEL_ID)) {
			dev->vbk_ready = 1;
			wake_up_interruptible(&dev->vbk_wait);
		}

		if (value & (0x1 << IPP_ORB_CHANNEL_ID)) {
			dev->orb_ready = 1;
			wake_up_interruptible(&dev->orb_wait);
		}

		if (value & (0x1 << IPP_RDR_CHANNEL_ID)) {
			dev->reorder_ready = 1;
			wake_up_interruptible(&dev->reorder_wait);
		}

		if (value & (0x1 << IPP_CMP_CHANNEL_ID)) {
			dev->compare_ready = 1;
			wake_up_interruptible(&dev->compare_wait);
		}

		if (value & (0x1 << IPP_ORB_ENH_CHANNEL_ID)) {
			dev->orb_enh_ready = 1;
			wake_up_interruptible(&dev->orb_enh_wait);
		}

		if (value & (0x1 << IPP_MC_CHANNEL_ID)) {
			dev->mc_ready = 1;
			wake_up_interruptible(&dev->mc_wait);
		}
	}

	pr_debug("[%s] -\n", __func__);

	return IRQ_HANDLED;
}

int hipp_adapter_register_irq(int irq)
{
	int ret;
	struct hipp_adapter_s *dev = g_hipp_adapter;

	if (dev == NULL) {
		pr_err("[%s] Failed : dev NULL\n", __func__);
		return -EINVAL;
	}
	ret = request_irq(irq, hispcpe_irq_handler, 0,
		"HIPP_ADAPTER_IRQ", (void *)dev);

	dev->irq = irq;

	return ret;
}

unsigned int get_cpe_addr_da(void)
{
	struct hipp_adapter_s *dev = g_hipp_adapter;

	if (dev == NULL) {
		pr_err("[%s], NONE cpe_mem_info!\n", __func__);
		return ISP_IPP_OK;
	}

	return dev->daddr;
}

void *get_cpe_addr_va(void)
{
	struct hipp_adapter_s *dev = g_hipp_adapter;

	if (dev == NULL) {
		pr_err("[%s], NONE cpe_mem_info!\n", __func__);
		return ISP_IPP_OK;
	}

	return dev->virt_addr;
}

int is_hipp_mapbuf_ready(void)
{
	struct hipp_adapter_s *dev = g_hipp_adapter;

	if (dev == NULL) {
		pr_err("[%s], NONE cpe_mem_info!\n", __func__);
		return ISP_IPP_OK;
	}

	if (dev->mapbuf_ready == 0) {
		pr_err("[%s] no map_kernel ops before\n", __func__);
		return 0;
	}

	return 1;
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

	reg_val = (1 << 0)	// axiwrite_du_threshold = 0
	    | (34 << 8)		// du_threshold_reached = 34
	    | (63 << 16)	// max_axiread_id = 19
	    | (31 << 24);	// max_axiwrite_id = 11
	writel(reg_val, crg_base + 0x0);

	return ISP_IPP_OK;
}

static int hispcpe_irq_config(void)
{
	unsigned int token_nbr_disable = 0;
	unsigned int nrt_channel = 1;
	unsigned int ipp_irq_mask = 0x1DF;
	unsigned int cmdlst_channel_value =
	    (token_nbr_disable << 7) | (nrt_channel << 8);

	loge_if_ret(hispcpe_reg_set(CPE_TOP, IPP_TOP_ENH_VPB_CFG_REG, 0x1));
	loge_if_ret(hispcpe_reg_set(CPE_TOP, IPP_TOP_CMDLST_IRQ_CLR_0_REG,
			(0xFFFF & ipp_irq_mask)));
	loge_if_ret(hispcpe_reg_set(CPE_TOP, IPP_TOP_CMDLST_IRQ_MSK_0_REG,
			(0xFFFF & ~ipp_irq_mask)));
	loge_if_ret(hispcpe_reg_set(CMDLIST_REG, 0x80 + IPP_GF_CHANNEL_ID * 0x80,
			cmdlst_channel_value));
	loge_if_ret(hispcpe_reg_set(CMDLIST_REG, 0x80 + IPP_VBK_CHANNEL_ID * 0x80,
			cmdlst_channel_value));
	loge_if_ret(hispcpe_reg_set(CMDLIST_REG, 0x80 + IPP_ORB_CHANNEL_ID * 0x80,
			cmdlst_channel_value));
	loge_if_ret(hispcpe_reg_set(CMDLIST_REG, 0x80 + IPP_RDR_CHANNEL_ID * 0x80,
			cmdlst_channel_value));
	loge_if_ret(hispcpe_reg_set(CMDLIST_REG, 0x80 + IPP_CMP_CHANNEL_ID * 0x80,
			cmdlst_channel_value));
	loge_if_ret(hispcpe_reg_set(CMDLIST_REG, 0x80 + IPP_MC_CHANNEL_ID * 0x80,
			cmdlst_channel_value));
	loge_if_ret(hispcpe_reg_set(CMDLIST_REG, 0x80 + IPP_ORB_ENH_CHANNEL_ID * 0x80,
			cmdlst_channel_value));

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

	writel(0x00000000, crg_base + IPP_TOP_JPG_FLUX_CTRL0_0_REG);
	writel(0x08000205, crg_base + IPP_TOP_JPG_FLUX_CTRL0_1_REG);
	writel(0x00000000, crg_base + IPP_TOP_JPG_FLUX_CTRL1_0_REG);
	writel(0x08000205, crg_base + IPP_TOP_JPG_FLUX_CTRL1_1_REG);

	ipp_qos_value = readl(crg_base + IPP_TOP_JPG_FLUX_CTRL0_0_REG);
	pr_info("[%s] JPG_FLUX_CTRL0_0 = 0x%x (0x0 wanted)\n",
		__func__, ipp_qos_value);

	ipp_qos_value = readl(crg_base + IPP_TOP_JPG_FLUX_CTRL0_1_REG);
	pr_info("[%s] JPG_FLUX_CTRL0_1 = 0x%x (0x08000205 wanted)\n",
		__func__, ipp_qos_value);

	ipp_qos_value = readl(crg_base + IPP_TOP_JPG_FLUX_CTRL1_0_REG);
	pr_info("[%s] JPG_FLUX_CTRL1_0 = 0x%x (0x0 wanted)\n",
		__func__, ipp_qos_value);

	ipp_qos_value = readl(crg_base + IPP_TOP_JPG_FLUX_CTRL1_1_REG);
	pr_info("[%s] JPG_FLUX_CTRL1_1 = 0x%x (0x08000205 wanted)\n",
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

int hipp_special_cfg(void)
{
	int ret;

	ret = hipp_adapter_cfg_qos_cvdr();
	if (ret < 0) {
		pr_err("[%s] Failed : hipp_adapter_cfg_qos_cvdr.%d\n", __func__, ret);
		return ret;
	}

	return 0;
}

int hipp_powerup(void)
{
	int ret;
	struct hipp_adapter_s *dev = g_hipp_adapter;
	int sid;
	int ssid;

	D("+\n");

	if (dev == NULL) {
		pr_err("[%s] Failed : dev NULL\n", __func__);
		return -EINVAL;
	}

	if (dev->ippdrv == NULL) {
		pr_err("[%s] Failed : ippdrv NULL\n", __func__);
		return -EINVAL;
	}

	if (dev->ippdrv->power_up == NULL) {
		pr_err("[%s] Failed : ippdrv->power_up.NULL\n", __func__);
		return -EINVAL;
	}

	if (dev->ippdrv->enable_smmu == NULL) {
		pr_err("[%s] Failed : ippdrv->enable_smmu.NULL\n", __func__);
		return -EINVAL;
	}

	if (dev->ippdrv->set_pref == NULL) {
		pr_err("[%s] Failed : ippdrv->set_pref.NULL\n", __func__);
		return -EINVAL;
	}

	if (dev->ippdrv->setsid_smmu == NULL) {
		pr_err("[%s] Failed : ippdrv->setsid_smmu.NULL\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&dev->dev_lock);
	if (dev->refs_power_up > 0) {
		dev->refs_power_up++;
		pr_info("[%s]: refs_power_up.%d\n", __func__, dev->refs_power_up);
		mutex_unlock(&dev->dev_lock);
		return ISP_IPP_OK;
	}

	ret = dev->ippdrv->power_up(dev->ippdrv);
	if (ret != 0) {
		pr_err("[%s] Failed : ippdrv->power_up.%d\n", __func__, ret);
		mutex_unlock(&dev->dev_lock);
		return ret;
	}

	ret = dev->ippdrv->enable_smmu(dev->ippdrv);
	if (ret != 0) {
		pr_err("[%s] Failed : ippdrv->enable_smmu.%d\n", __func__, ret);
		goto fail_power_down;
	}

	ret = get_hipp_smmu_info(&sid, &ssid);
	if (ret < 0) {
		pr_err("[%s] Failed : get_hipp_smmu_info.%d\n", __func__, ret);
		goto fail_disable_smmu;
	}

	ret = dev->ippdrv->setsid_smmu(dev->ippdrv, IPPCORE_SWID_INDEX0,
			IPPCORE_SWID_LEN0, sid, ssid);
	if (ret != 0) {
		pr_err("[%s] Failed : ippdrv->enable_smmu1.%d\n", __func__, ret);
		goto fail_disable_smmu;
	}

	ret = dev->ippdrv->setsid_smmu(dev->ippdrv, IPPCORE_SWID_INDEX1,
			IPPCORE_SWID_LEN1, sid, ssid);
	if (ret != 0) {
		pr_err("[%s] Failed : ippdrv->enable_smmu2.%d\n", __func__, ret);
		goto fail_disable_smmu;
	}

	ret = dev->ippdrv->set_pref(dev->ippdrv, IPPCORE_SWID_INDEX0, IPPCORE_SWID_LEN0);
	if (ret != 0) {
		pr_err("[%s] Failed : ippdrv->set_pref1.%d\n", __func__, ret);
		goto fail_disable_smmu;
	}

	ret = dev->ippdrv->set_pref(dev->ippdrv, IPPCORE_SWID_INDEX1, IPPCORE_SWID_LEN1);
	if (ret != 0) {
		pr_err("[%s] Failed : ippdrv->set_pref2.%d\n", __func__, ret);
		goto fail_disable_smmu;
	}

	ret = hispcpe_irq_config();
	if (ret != 0) {
		pr_err("[%s] Failed : set irq config.%d\n", __func__, ret);
		goto fail_disable_smmu;
	}

	dev->refs_power_up++;
	pr_info("[%s]: refs_power_up.%d\n", __func__, dev->refs_power_up);
	mutex_unlock(&dev->dev_lock);

	D("-\n");
	return ISP_IPP_OK;

fail_disable_smmu:
	if (dev->ippdrv->disable_smmu != NULL) {
		ret = dev->ippdrv->disable_smmu(dev->ippdrv);
		if (ret != 0)
			pr_err("[%s] Failed : dev->ippdrv->disable_smmu.%d\n",
				__func__, ret);
	}

fail_power_down:
	if (dev->ippdrv->power_dn != NULL) {
		ret = dev->ippdrv->power_dn(dev->ippdrv);
		if (ret != 0)
			pr_err("[%s] Failed : dev->ippdrv->power_dw.%d\n",
				__func__, ret);
		}
	mutex_unlock(&dev->dev_lock);
	return ret;
}

static int hispcpe_work_check(struct hipp_adapter_s *dev, int pd_flag)
{
	int i;

	if (pd_flag == HIPP_PD_FORCE) {
		dev->refs_power_up = 1;
		for (i = 0; i < REFS_TYP_MAX; i++)
			dev->hipp_refs[i] = 0;
		return 0;
	}

	if (dev->hipp_refs[REFS_TYPE_GF] > 0) {
		pr_err("[%s] : gf  refs Opened .\n", __func__);
		return -ENOMEM;
	}

	if (dev->hipp_refs[REFS_TYP_VBK] > 0) {
		pr_err("[%s] : vbk  refs Opened .\n", __func__);
		return -ENOMEM;
	}

	if (dev->hipp_refs[REFS_TYP_ORCM] > 0) {
		pr_err("[%s] : orcm  refs Opened .\n", __func__);
		return -ENOMEM;
	}

	return 0;
}

int hipp_powerdn(int pd_flag)
{
	int ret;
	struct hipp_adapter_s *dev = g_hipp_adapter;

	if (dev == NULL) {
		pr_err("[%s] Failed : dev.%pK\n", __func__, dev);
		return -EINVAL;
	}

	if (dev->ippdrv == NULL) {
		pr_err("[%s] Failed : ippdrv.%pK\n", __func__, dev->ippdrv);
		return -ENOMEM;
	}

	if (dev->ippdrv->disable_smmu == NULL) {
		pr_err("[%s] Failed : ippdrv->disable_smmu.NULL\n", __func__);
		return -ENOMEM;
	}

	if (dev->ippdrv->power_dn == NULL) {
		pr_err("[%s] Failed : ippdrv->power_dn.NULL\n", __func__);
		return -ENOMEM;
	}

	mutex_lock(&dev->dev_lock);
	if (dev->refs_power_up <= 0) {
		pr_err("[%s] : ippcore no pw\n", __func__);
		mutex_unlock(&dev->dev_lock);
		return ISP_IPP_OK;
	}

	ret = hispcpe_work_check(dev, pd_flag);
	if (ret < 0){
		mutex_unlock(&dev->dev_lock);
		return -ENOMEM;
	}

	dev->refs_power_up--;
	if (dev->refs_power_up > 0) {
		pr_info("[%s]: refs_power_up.%d\n", __func__, dev->refs_power_up);
		mutex_unlock(&dev->dev_lock);
		return ISP_IPP_OK;
	}

	ret = dev->ippdrv->disable_smmu(dev->ippdrv);
	if (ret != 0)
		pr_err("[%s] Failed : disable_smmu.%d\n", __func__, ret);

	ret = dev->ippdrv->power_dn(dev->ippdrv);
	if (ret != 0)
		pr_err("[%s] Failed : power_dn.%d\n", __func__, ret);

	pr_info("[%s]: refs_power_up.%d\n", __func__, dev->refs_power_up);

	mutex_unlock(&dev->dev_lock);

	return ISP_IPP_OK;
}

static int hispcpe_vbk_clk_enable(void)
{
	union U_VBK_CRG_CFG0 cfg;
	void __iomem *crg_base;

	D("+\n");

	crg_base = hipp_get_regaddr(CPE_TOP);
	if (crg_base == NULL ) {
		pr_err("[%s] Failed : hipp_get_regaddr\n", __func__);
		return -EINVAL;
	}

	cfg.u32 = readl(crg_base + IPP_TOP_VBK_CRG_CFG0_REG);
	cfg.bits.vbk_clken = 1;
	writel(cfg.u32, crg_base + IPP_TOP_VBK_CRG_CFG0_REG);
	D("-\n");
	return ISP_IPP_OK;
}

static int hispcpe_vbk_clk_disable(void)
{
	union U_VBK_CRG_CFG0 cfg;
	void __iomem *crg_base;

	D("+\n");

	crg_base = hipp_get_regaddr(CPE_TOP);
	if (crg_base == NULL ) {
		pr_err("[%s] Failed : hipp_get_regaddr\n", __func__);
		return -EINVAL;
	}

	cfg.u32 = readl(crg_base + IPP_TOP_VBK_CRG_CFG0_REG);
	cfg.bits.vbk_clken = 0;
	writel(cfg.u32, crg_base + IPP_TOP_VBK_CRG_CFG0_REG);

	D("-\n");

	return ISP_IPP_OK;
}

static int hipp_orcm_clk_enable(void)
{
	void __iomem *crg_base;
	D("+\n");

	crg_base = hipp_get_regaddr(CPE_TOP);
	if (crg_base == NULL ) {
		pr_err("[%s] Failed : hipp_get_regaddr\n", __func__);
		return -EINVAL;
	}

	writel(IPP_CLK_EN, crg_base + IPP_TOP_ENH_CRG_CFG0_REG);
	writel(IPP_CLK_EN, crg_base + IPP_TOP_ORB_CRG_CFG0_REG);
	writel(IPP_CLK_EN, crg_base + IPP_TOP_RDR_CRG_CFG0_REG);
	writel(IPP_CLK_EN, crg_base + IPP_TOP_CMP_CRG_CFG0_REG);
	writel(IPP_CLK_EN, crg_base + IPP_TOP_MC_CRG_CFG0_REG);

	D("-\n");
	return ISP_IPP_OK;
}

static int hipp_orcm_clk_disable(void)
{
	void __iomem *crg_base;
	D("+\n");

	crg_base = hipp_get_regaddr(CPE_TOP);
	if (crg_base == NULL ) {
		pr_err("[%s] Failed : hipp_get_regaddr\n", __func__);
		return -EINVAL;
	}

	writel(IPP_CLK_DISABLE, crg_base + IPP_TOP_ENH_CRG_CFG0_REG);
	writel(IPP_CLK_DISABLE, crg_base + IPP_TOP_ORB_CRG_CFG0_REG);
	writel(IPP_CLK_DISABLE, crg_base + IPP_TOP_RDR_CRG_CFG0_REG);
	writel(IPP_CLK_DISABLE, crg_base + IPP_TOP_CMP_CRG_CFG0_REG);
	writel(IPP_CLK_DISABLE, crg_base + IPP_TOP_MC_CRG_CFG0_REG);

	D("-\n");
	return ISP_IPP_OK;
}

static int hispcpe_gf_clk_enable(struct hipp_adapter_s *dev)
{
	union U_GF_CRG_CFG0 cfg;
	void __iomem *crg_base;

	D("+\n");

	if (dev == NULL) {
		pr_err("[%s] Failed : dev.%pK\n", __func__, dev);
		return -EINVAL;
	}

	crg_base = hipp_get_regaddr(CPE_TOP);
	if (crg_base == NULL ) {
		pr_err("[%s] Failed : hipp_get_regaddr\n", __func__);
		return -EINVAL;
	}

	cfg.u32 = readl(crg_base + IPP_TOP_GF_CRG_CFG0_REG);
	cfg.bits.gf_clken = 1;
	writel(cfg.u32, crg_base + IPP_TOP_GF_CRG_CFG0_REG);

	D("-\n");

	return ISP_IPP_OK;
}

static int hispcpe_gf_clk_disable(struct hipp_adapter_s *dev)
{
	union U_GF_CRG_CFG0 cfg;
	void __iomem *crg_base;

	D("+\n");

	crg_base = hipp_get_regaddr(CPE_TOP);
	if (crg_base == NULL ) {
		pr_err("[%s] Failed : hipp_get_regaddr\n", __func__);
		return -EINVAL;
	}

	cfg.u32 = readl(crg_base + IPP_TOP_GF_CRG_CFG0_REG);
	cfg.bits.gf_clken = 0;
	writel(cfg.u32, crg_base + IPP_TOP_GF_CRG_CFG0_REG);
	D("-\n");

	return ISP_IPP_OK;
}

int hispcpe_clean_wait_flag(struct hipp_adapter_s *dev, unsigned int wait_mode)
{
	D("+\n");

	if (dev == NULL) {
		pr_err("[%s] Failed : dev.%pK\n", __func__, dev);
		return -EINVAL;
	}

	switch (wait_mode) {
	case WAIT_MODE_GF:
		dev->gf_ready = 0;
		break;

	case WAIT_MODE_ORB:
		dev->orb_ready = 0;
		break;

	case WAIT_MODE_RDR:
		dev->reorder_ready = 0;
		break;

	case WAIT_MODE_CMP:
		dev->compare_ready = 0;
		break;

	case WAIT_MODE_VBK:
		dev->vbk_ready = 0;
		break;

	case WAIT_MODE_ENH:
		dev->orb_enh_ready = 0;
		break;

	case WAIT_MODE_MC:
		dev->mc_ready = 0;
		break;

	default:
		pr_err("[%s] Failed : wrong wait mode .%d\n",
		       __func__, wait_mode);
		return ISP_IPP_ERR;
	}

	D("-\n");
	return ISP_IPP_OK;
}

static int hispcpe_gf_request(struct hipp_adapter_s *dev,
	struct msg_req_mcf_request_t *ctrl)
{
	int ret;
	int ret1;

	D("+\n");

	if (dev == NULL || ctrl == NULL) {
		pr_err("[%s] Failed : dev.%pK, ctrl.%pK\n",
		       __func__, dev, ctrl);
		return -EINVAL;
	}

	ret = hispcpe_clean_wait_flag(dev, WAIT_MODE_GF);
	if (ret < 0) {
		pr_err("[%s] Failed : hispcpe_clean_wait_flag.%d\n",
		       __func__, ret);
		return ret;
	}

	ret = gf_request_handler(ctrl);
	if (ret < 0) {
		pr_err("[%s] Failed : gf_request_handler.%d\n",
			__func__, ret);
		goto GF_REQ_DONE;
	}

	ret = hispcpe_wait(dev, WAIT_MODE_GF);
	if (ret < 0) {
		pr_err("[%s] Failed : hispcpe_wait.%d\n",
			__func__, ret);
		ret = ISP_IPP_ERR;
		goto GF_REQ_DONE;
	}

GF_REQ_DONE:
	ret1 = ipp_eop_handler(CMD_EOF_GF_MODE);
	if (ret1 != 0)
		pr_err("[%s] Failed : gf_eop_handler.%d\n",__func__, ret1);
	D("-\n");
	if (ret1 != 0|| ret != 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
}

int hispcpe_orb_enh_request(struct hipp_adapter_s *dev,
		struct msg_enh_req_t *req_enh)
{
	int ret;

	D("+\n");

	if (dev == NULL || req_enh == NULL) {
		pr_err("[%s] Failed : dev.%pK, req_enh.%pK\n",
		       __func__, dev, req_enh);
		return -EINVAL;
	}

	ret = hispcpe_clean_wait_flag(dev, WAIT_MODE_ENH);
	if (ret < 0) {
		pr_err("[%s] Failed : hispcpe_clean_wait_flag.%d\n",
		       __func__, ret);
		return ret;
	}

	ret = orb_enh_request_handler(req_enh);

	if (ret < 0) {
		pr_err("[%s] Failed : orb_enh_request_handler.%d\n",
			__func__, ret);
		return ret;
	}

	ret = hispcpe_wait(dev, WAIT_MODE_ENH);
	if (ret < 0)
		pr_err("[%s] Failed : hispcpe_wait.%d\n", __func__, ret);

	ret = ipp_eop_handler(CMD_EOF_ORB_ENH_MODE);
	if (ret < 0)
		pr_err("[%s] Failed : ipp_eop_handler.%d\n", __func__, ret);

	D("-\n");
	return ret;
}

int hispcpe_mc_request(struct hipp_adapter_s *dev,
	struct msg_req_mc_request_t *ctrl)
{
	int ret = 0;
	struct orb_schedule_flag_t * flag = NULL;

	D("+\n");

	if (dev == NULL || ctrl == NULL) {
		pr_err("[%s] Failed : dev.%pK, ctrl.%pK\n",
		       __func__, dev, ctrl);
		return -EINVAL;
	}

	ret = hispcpe_clean_wait_flag(dev, WAIT_MODE_MC);
	if (ret < 0) {
		pr_err("[%s] Failed : hispcpe_clean_wait_flag.%d\n",
		       __func__, ret);
		return ret;
	}


	flag = kmalloc(sizeof(struct orb_schedule_flag_t), GFP_KERNEL);
	if (flag  == NULL)
		return -EINVAL;

	ret = memset_s(flag, sizeof(struct orb_schedule_flag_t),
			 0, sizeof(struct orb_schedule_flag_t));
	if (ret!= 0) {
		pr_err(" fail to memset_s orb_schedule_flag_t\n");
		goto MC_REQ_FAIL;
	}

	ret = mc_request_handler(ctrl,flag);
	if (ret!= 0) {
		pr_err(" mc_request_handler\n");
		goto MC_REQ_FAIL;
	}

	ret = hispcpe_wait(dev, WAIT_MODE_MC);
	if (ret < 0)
		pr_err("[%s] Failed : hispcpe_wait.%d\n", __func__, ret);

	ret = ipp_eop_handler(CMD_EOF_MC_MODE);
	if (ret < 0)
		pr_err("[%s] Failed : ipp_eop_handler.%d\n", __func__, ret);

MC_REQ_FAIL:
	kfree(flag);
	D("-\n");
	return ret;
}

int hispcpe_vbk_request(struct hipp_adapter_s *dev,
	struct msg_req_vbk_request_t *ctrl)
{
	int ret;
	int ret1;

	D("+\n");

	if (dev == NULL || ctrl == NULL) {
		pr_err("[%s] Failed : dev.%pK, ctrl.%pK\n",
		       __func__, dev, ctrl);
		return -EINVAL;
	}

	ret = hispcpe_clean_wait_flag(dev, WAIT_MODE_VBK);
	if (ret < 0) {
		pr_err("[%s] Failed : hispcpe_clean_wait_flag.%d\n",
		       __func__, ret);
		return ret;
	}

	ret = vbk_request_handler(ctrl);
	if (ret < 0) {
		pr_err("[%s] Failed : vbk_request_handler.%d\n",
			__func__, ret);
		goto VBK_REQ_DONE;
	}

	ret = hispcpe_wait(dev, WAIT_MODE_VBK);
	if (ret < 0) {
		pr_err("[%s] Failed : hispcpe_wait.%d\n", __func__, ret);
		ret = ISP_IPP_ERR;
		goto VBK_REQ_DONE;
	}

VBK_REQ_DONE:
	ret1 = ipp_eop_handler(CMD_EOF_VBK_MODE);
	if (ret1 != 0)
		pr_err("[%s] Failed : vbk_eop_handler.%d\n", __func__, ret1);

	D("-\n");
	if (ret1 != 0 || ret != 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
}

static int  hipp_orcm_wait_flag(struct hipp_adapter_s *dev,
		enum work_module_e module)
{
	int ret0 = 0;
	int ret1 = 0;
	int ret2 = 0;
	int ret3 = 0;

	if (dev == NULL) {
		pr_err("[%s] Failed : dev.%pK\n",
		       __func__, dev);
		return -EINVAL;
	}

	if (module == ENH_ORB ||
		module == ORB_ONLY) {
		ret0 = hispcpe_clean_wait_flag(dev,
			WAIT_MODE_ORB);
	} else if (module == ORB_MATCHER ||
		module  == ENH_ORB_MATCHER) {
		ret0 = hispcpe_clean_wait_flag(dev,
			WAIT_MODE_ORB);
		ret1 = hispcpe_clean_wait_flag(dev,
			WAIT_MODE_RDR);
		ret2 = hispcpe_clean_wait_flag(dev,
			WAIT_MODE_CMP);
	} else if(module == MATCHER_MC) {
		ret0 = hispcpe_clean_wait_flag(dev,
			WAIT_MODE_RDR);
		ret1 = hispcpe_clean_wait_flag(dev,
			WAIT_MODE_CMP);
		ret2 = hispcpe_clean_wait_flag(dev,
			WAIT_MODE_MC);
	} else if(module == ORB_MATCHER_MC ||
		module == ENH_ORB_MATCHER_MC) {
		ret0 = hispcpe_clean_wait_flag(dev,
			WAIT_MODE_ORB);
		ret1 = hispcpe_clean_wait_flag(dev,
			WAIT_MODE_RDR);
		ret2 = hispcpe_clean_wait_flag(dev,
			WAIT_MODE_CMP);
		ret3 = hispcpe_clean_wait_flag(dev,
			WAIT_MODE_MC);
	}

	if (ret0 < 0 || ret1 < 0 || ret2 < 0 || ret3 < 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
}

static int hipp_orcm_wait(struct hipp_adapter_s *dev, enum work_module_e module)
{
	int ret0 = 0;
	int ret1 = 0;
	int ret2 = 0;
	int ret3 = 0;

	if (dev == NULL) {
		pr_err("[%s] Failed : dev.%pK\n",
		       __func__, dev);
		return -EINVAL;
	}

	I("module = %d", module);
	if(module == ENH_ORB ||
		module == ORB_ONLY) {
		ret0 = hispcpe_wait(dev,
			WAIT_MODE_ORB);
	} else if (module == ORB_MATCHER ||
		module  == ENH_ORB_MATCHER) {
		ret0 = hispcpe_wait(dev, WAIT_MODE_ORB);
		ret1 = hispcpe_wait(dev, WAIT_MODE_RDR);
		ret2 = hispcpe_wait(dev, WAIT_MODE_CMP);
	} else if(module == MATCHER_MC) {
		ret0 = hispcpe_wait(dev, WAIT_MODE_RDR);
		ret1 = hispcpe_wait(dev, WAIT_MODE_CMP);
		ret2 = hispcpe_wait(dev, WAIT_MODE_MC);
	} else if(module == ORB_MATCHER_MC ||
		module == ENH_ORB_MATCHER_MC) {
		ret0 = hispcpe_wait(dev, WAIT_MODE_ORB);
		ret1 = hispcpe_wait(dev, WAIT_MODE_RDR);
		ret2 = hispcpe_wait(dev, WAIT_MODE_CMP);
		ret3 = hispcpe_wait(dev, WAIT_MODE_MC);
	}

	if (ret0 < 0 || ret1 < 0 || ret2 < 0 || ret3 < 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
}

static int  hipp_orcm_eop_handle(struct hipp_adapter_s *dev,
		enum work_module_e module)
{
	int ret0 = 0;
	int ret1 = 0;
	int ret2 = 0;
	int ret3 = 0;

	if (dev == NULL) {
		pr_err("[%s] Failed : dev.%pK\n",
		       __func__, dev);
		return -EINVAL;
	}

	if(module == ENH_ORB ||
		module == ORB_ONLY) {
		ret0 = ipp_eop_handler(CMD_EOF_ORB_MODE);
	} else if (module == ORB_MATCHER ||
		module  == ENH_ORB_MATCHER) {
		ret0 = ipp_eop_handler(CMD_EOF_ORB_MODE);
		ret1 = ipp_eop_handler(CMD_EOF_RDR_MODE);
		ret2 = ipp_eop_handler(CMD_EOF_CMP_MODE);
	} else if(module == MATCHER_MC) {
		ret0 = ipp_eop_handler(CMD_EOF_RDR_MODE);
		ret1 = ipp_eop_handler(CMD_EOF_CMP_MODE);
		ret2 = ipp_eop_handler(CMD_EOF_MC_MODE);
	} else if(module == ORB_MATCHER_MC ||
		module == ENH_ORB_MATCHER_MC) {
		ret0 =  ipp_eop_handler(CMD_EOF_ORB_MODE);
		ret1 =  ipp_eop_handler(CMD_EOF_RDR_MODE);
		ret2 =  ipp_eop_handler(CMD_EOF_CMP_MODE);
		ret3 =  ipp_eop_handler(CMD_EOF_MC_MODE);
	}

	if (ret0 < 0 || ret1 < 0 || ret2 < 0 || ret3 < 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
}

static int hispcpe_reorder_request(struct hipp_adapter_s *dev,
	struct msg_req_reorder_request_t *ctrl)
{
	int ret;

	D("+\n");

	if (dev == NULL || ctrl == NULL) {
		pr_err("[%s] Failed : dev.%pK, ctrl.%pK\n",
		       __func__, dev, ctrl);
		return -EINVAL;
	}

	ret = hispcpe_clean_wait_flag(dev, WAIT_MODE_RDR);
	if (ret < 0) {
		pr_err("[%s] Failed : hispcpe_clean_wait_flag.%d\n",
		       __func__, ret);
		return ret;
	}

	reorder_request_handler(ctrl);

	ret = hispcpe_wait(dev, WAIT_MODE_RDR);
	if (ret < 0)
		pr_err("[%s] Failed : hispcpe_wait.%d\n", __func__, ret);

	ret = ipp_eop_handler(CMD_EOF_RDR_MODE);
	if (ret < 0)
		pr_err("[%s] Failed : ipp_eop_handler.%d\n", __func__, ret);

	D("-\n");
	return ret;
}

static int hispcpe_compare_request(struct hipp_adapter_s *dev,
	struct msg_req_compare_request_t *ctrl)
{
	int ret;

	D("+\n");

	if (dev == NULL || ctrl == NULL) {
		pr_err("[%s] Failed : dev.%pK, ctrl.%pK\n",
		       __func__, dev, ctrl);
		return -EINVAL;
	}

	ret = hispcpe_clean_wait_flag(dev, WAIT_MODE_CMP);
	if (ret < 0) {
		pr_err("[%s] Failed : hispcpe_clean_wait_flag.%d\n",
		       __func__, ret);
		return ret;
	}

	compare_request_handler(ctrl);

	ret = hispcpe_wait(dev, WAIT_MODE_CMP);
	if (ret < 0)
		pr_err("[%s] Failed : hispcpe_wait.%d\n", __func__, ret);

	ret = ipp_eop_handler(CMD_EOF_CMP_MODE);
	if (ret < 0)
		pr_err("[%s] Failed : ipp_eop_handler.%d\n", __func__, ret);

	D("-\n");
	return ret;
}

int hispcpe_matcher_request(struct hipp_adapter_s *dev,
		struct msg_req_matcher_t *ctrl)
{
	int ret;
	unsigned int matched_kpt = 0;
	struct orb_schedule_flag_t * flag = NULL;
	void __iomem *crg_base;

	if (dev == NULL || ctrl == NULL) {
		pr_err("[%s] Failed : dev.%pK, ctrl.%pK\n",
		       __func__, dev, ctrl);
		return -EINVAL;
	}

	ret = hispcpe_clean_wait_flag(dev, WAIT_MODE_RDR);
	if (ret < 0) {
		pr_err("[%s] Failed : hispcpe_clean_wait_flag--reorder.%d\n",
		       __func__, ret);
		return ret;
	}

	ret = hispcpe_clean_wait_flag(dev, WAIT_MODE_CMP);
	if (ret < 0) {
		pr_err("[%s] Failed : hispcpe_clean_wait_flag--compare.%d\n",
		       __func__, ret);
		return ret;
	}

	flag = kmalloc(sizeof(struct orb_schedule_flag_t), GFP_KERNEL);
	if (flag  == NULL)
		return -EINVAL;

	ret = memset_s(flag, sizeof(struct orb_schedule_flag_t),
		     0, sizeof(struct orb_schedule_flag_t));

	if (ret!= 0) {
		pr_err(" fail to memset_s orb_schedule_flag_t\n");
		goto MATCHER_REQ_FAIL;
	}

	ret = matcher_request_handler(ctrl,flag);
	if (ret!= 0) {
		pr_err(" matcher_request_handler\n");
		goto MATCHER_REQ_FAIL;
	}

	ret = hispcpe_wait(dev, WAIT_MODE_RDR);
	if (ret < 0)
		pr_err("[%s] Failed : hispcpe_wait reorder.%d\n",
		       __func__, ret);

	ret = hispcpe_wait(dev, WAIT_MODE_CMP);
	if (ret < 0)
		pr_err("[%s] Failed : hispcpe_wait compare.%d\n",
		       __func__, ret);

	crg_base = hipp_get_regaddr(COMPARE_REG);
	if (crg_base == NULL ) {
		pr_err("[%s] Failed : hipp_get_regaddr\n", __func__);
		ret = -EINVAL;
		goto MATCHER_REQ_FAIL;
	}

	matched_kpt = (unsigned int)readl(crg_base) >> 8;

	D("@@@@@@@@ matched_kpt = %d", matched_kpt);
	matcher_eof_handler(ctrl);

MATCHER_REQ_FAIL:
	kfree(flag);
	return ret;
}

int hispcpe_orb_request(struct hipp_adapter_s *dev,
		struct msg_req_orb_request_t *ctrl)
{
	int ret = 0;
	int ret1 = 0;

	D("+\n");

	if (dev == NULL || ctrl == NULL) {
		pr_err("[%s] Failed : dev.%pK, ctrl.%pK\n",
		       __func__, dev, ctrl);
		return -EINVAL;
	}

	if (ctrl->work_module == MATCHER_ONLY) {
		ret = hispcpe_matcher_request(dev, &(ctrl->req_matcher));
		return ret;
	}else if (ctrl->work_module == MC_ONLY) {
		ret = hispcpe_mc_request(dev, &(ctrl->req_mc));
		return ret;
	}else if (ctrl->work_module == ENH_ONLY) {
		ret = hispcpe_orb_enh_request(dev, &(ctrl->req_orb_cur.req_enh));
		return ret;
	}

	ret = hipp_orcm_wait_flag(dev, ctrl->work_module);
	if (ret < 0) {
		pr_err("[%s] Failed : hispcpe_orcm_wait_flag.%d\n",
		       __func__, ret);
		return ret;
	}

	ret = orb_request_handler(ctrl);
	if (ret != 0) {
		pr_err("[%s] Failed : orb_request_handler.%d\n",
		       __func__, ret);
		goto ORB_REQ_DONE;
	}

	ret = hipp_orcm_wait(dev, ctrl->work_module);
	if (ret < 0)
		pr_err("[%s] Failed : hispcpe_orcm_wait.%d\n",
			__func__, ret);

ORB_REQ_DONE:
	ret1 = hipp_orcm_eop_handle(dev, ctrl->work_module);
	if (ret1 != 0)
		pr_err("[%s] Failed : orb_orcm_eop_handle.%d\n",
			__func__, ret);

	D("-\n");
	if(ret != 0 || ret1 != 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
}

static int gf_process_internal(struct hipp_adapter_s *dev,
	struct msg_req_mcf_request_t *ctrl_mcf)
{
	unsigned int ret;

	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
				dev->curr_cpe_rate_value[CLK_RATE_INDEX_GF]);
	if (ret != 0) {
		pr_err("[%s] Failed : ipp_set_rate.%d\n", __func__, ret);

		ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv, CLK_RATE_SVS);
		if (ret != 0) {
			pr_err("[%s] Failed to set ipp  rate mode: %d.\n",
			       __func__, CLK_RATE_SVS);
			return ret;
		}
	}

	ret = hispcpe_gf_clk_enable(dev);
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_gf_clk_enable.%d\n",
		       __func__, ret);
		return ret;
	}

	ret = hispcpe_gf_request(dev, ctrl_mcf);
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_gf_request.%d\n", __func__, ret);

		if (hispcpe_gf_clk_disable(dev) != 0)
			pr_err("[%s] Failed : hispcpe_cpe_clk_disable\n",
			       __func__);

		return ret;
	}

	ret = hispcpe_gf_clk_disable(dev);
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_cpe_clk_disable.%d\n",
		       __func__, ret);
		return ret;
	}

	dev->curr_cpe_rate_value[CLK_RATE_INDEX_GF] = CLK_RATE_SVS;

	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
				dev->curr_cpe_rate_value[CLK_RATE_INDEX_GF]);
	if (ret != 0)
		pr_err("[%s] Failed : ipp_set_rate.%d\n", __func__, ret);

	return ret;
}

int gf_process(unsigned long args)
{
	unsigned int ret = 0;
	struct msg_req_mcf_request_t *ctrl_mcf = NULL;
	unsigned int gf_rate_index;
	void __user *args_mcf = (void __user *)(uintptr_t) args;
	struct hipp_adapter_s *dev = g_hipp_adapter;

	if ((args_mcf == NULL) || (dev == NULL)) {
		pr_err("[%s] args_mcf NULL.%pK\n", __func__, args_mcf);
		return -EINVAL;
	}

	ctrl_mcf = kmalloc(sizeof(struct msg_req_mcf_request_t), GFP_KERNEL);
	if (ctrl_mcf == NULL) {
		pr_err(" fail to vmalloc ctrl_mcf\n");
		return -EINVAL;
	}

	if (memset_s(ctrl_mcf, sizeof(struct msg_req_mcf_request_t),
		     0, sizeof(struct msg_req_mcf_request_t))) {
		pr_err(" fail to memset_s ctrl_mcf\n");
		goto free_mcf_kmalloc_memory;
	}

	ret = copy_from_user(ctrl_mcf, args_mcf,
			     sizeof(struct msg_req_mcf_request_t));
	if (ret != 0) {
		pr_err("[%s] copy_from_user.%d\n", __func__, ret);
		goto free_mcf_kmalloc_memory;
	}

	gf_rate_index = ctrl_mcf->mcf_rate_value;
	gf_rate_index = gf_rate_index > CLK_RATE_SVS ? 0 : gf_rate_index;
	dev->curr_cpe_rate_value[CLK_RATE_INDEX_GF] = gf_rate_index;

	ret = gf_process_internal(dev, ctrl_mcf);
	if (ret != 0)
		pr_err("[%s] Failed : ipp_set_rate.%d\n", __func__, ret);

free_mcf_kmalloc_memory:
	kfree(ctrl_mcf);
	return ret;
}

int vbk_process(unsigned long args)
{
	unsigned int ret = 0;
	struct msg_req_vbk_request_t *ctrl_vbk = NULL;
	unsigned int vbk_rate_index;
	void __user *args_vbk = (void __user *)(uintptr_t) args;
	struct hipp_adapter_s *dev = g_hipp_adapter;

	if (args_vbk == NULL) {
		pr_err("[%s] args_vbk NULL.%pK\n", __func__, args_vbk);
		return -EINVAL;
	}

	ctrl_vbk = kmalloc(sizeof(struct msg_req_vbk_request_t), GFP_KERNEL);
	if (ctrl_vbk == NULL)
		return -EINVAL;

	if (memset_s(ctrl_vbk,
		     sizeof(struct msg_req_vbk_request_t),
		     0, sizeof(struct msg_req_vbk_request_t))) {
		pr_err(" fail to memset_s ctrl_vbk\n");
		goto free_vbk_kmalloc_memory;
	}

	ret = copy_from_user(ctrl_vbk, args_vbk,
			     sizeof(struct msg_req_vbk_request_t));
	if (ret != 0) {
		pr_err("[%s] copy_from_user.%d\n", __func__, ret);
		goto free_vbk_kmalloc_memory;
	}

	vbk_rate_index = ctrl_vbk->vbk_rate_value;
	vbk_rate_index = vbk_rate_index > CLK_RATE_SVS ? 0 : vbk_rate_index;
	dev->curr_cpe_rate_value[CLK_RATE_INDEX_VBK] = vbk_rate_index;

	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
					   dev->curr_cpe_rate_value
					   [CLK_RATE_INDEX_VBK]);
	if (ret != 0) {
		pr_err("[%s] Failed : ipp_set_rate.%d\n", __func__, ret);

		ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv, CLK_RATE_SVS);
		if (ret != 0) {
			pr_err("[%s] Failed to set ipp  rate mode: %d.\n",
			       __func__, CLK_RATE_SVS);
			goto free_vbk_kmalloc_memory;
		}
	}

	ret = hispcpe_vbk_clk_enable();
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_gf_clk_enable.%d\n",
		       __func__, ret);
		goto free_vbk_kmalloc_memory;
	}

	ret = hispcpe_vbk_request(dev, ctrl_vbk);
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_vbk_request.%d\n", __func__, ret);

		if (hispcpe_vbk_clk_disable() != 0)
			pr_err("[%s] Failed : hispcpe_cpe_clk_disable\n",
			       __func__);

		goto free_vbk_kmalloc_memory;
	}

	ret = hispcpe_vbk_clk_disable();
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_cpe_clk_disable.%d\n",
		       __func__, ret);
		goto free_vbk_kmalloc_memory;
	}

	dev->curr_cpe_rate_value[CLK_RATE_INDEX_VBK] = CLK_RATE_SVS;

	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
				dev->curr_cpe_rate_value[CLK_RATE_INDEX_VBK]);
	if (ret != 0)
		pr_err("[%s] Failed : ipp_set_rate.%d\n", __func__, ret);

free_vbk_kmalloc_memory:
	kfree(ctrl_vbk);
	return ret;
}

static int reorder_process_internal(struct hipp_adapter_s *dev,
		struct msg_req_reorder_request_t *ctrl_reorder)
{
	unsigned int ret;

	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
		dev->curr_cpe_rate_value[CLK_RATE_INDEX_MATCHER]);
	if (ret != 0) {
		pr_err("[%s] Failed : ipp_set_rate.%d\n", __func__, ret);

		ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv, CLK_RATE_SVS);
		if (ret != 0) {
			pr_err("[%s] Failed to set ipp  rate mode: %d.\n",
			       __func__, CLK_RATE_SVS);
			return ret;
		}
	}

	ret = hipp_orcm_clk_enable();
	if (ret != 0) {
		pr_err("[%s] Failed : hipp_orcm_clk_enable.%d\n",
		       __func__, ret);
		return ret;
	}

	ret = hispcpe_reorder_request(dev, ctrl_reorder);
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_reorder_request.%d\n",
		       __func__, ret);

		if (hipp_orcm_clk_disable() != 0)
			pr_err("[%s] Failed : hipp_orcm_clk_disable\n",
			       __func__);

		return ret;
	}

	ret = hipp_orcm_clk_disable();
	if (ret != 0) {
		pr_err("[%s] Failed : hipp_orcm_clk_disable.%d\n",
		       __func__, ret);
		return ret;
	}

	dev->curr_cpe_rate_value[CLK_RATE_INDEX_MATCHER] = CLK_RATE_SVS;

	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
		dev->curr_cpe_rate_value[CLK_RATE_INDEX_MATCHER]);
	if (ret != 0)
		pr_err("[%s] Failed : ipp_set_rate.%d\n", __func__, ret);

	return ret;
}

int reorder_process(unsigned long args)
{
	int ret = 0;
	struct msg_req_reorder_request_t *ctrl_reorder = NULL;
	void __user *args_rdr = (void __user *)(uintptr_t)args;
	struct hipp_adapter_s *dev = g_hipp_adapter;

	if ((args_rdr == NULL) || (dev == NULL)) {
		pr_err("[%s] args_rdr or dev NULL\n", __func__);
		return -EINVAL;
	}

	ctrl_reorder = kmalloc(sizeof(struct msg_req_reorder_request_t), GFP_KERNEL);

	if (ctrl_reorder == NULL)
		return -EINVAL;

	if (memset_s(ctrl_reorder, sizeof(struct msg_req_reorder_request_t),
		     0, sizeof(struct msg_req_reorder_request_t))) {
		pr_err(" fail to memset_s ctrl_reorder\n");
		goto free_reorder_kmalloc_memory;
	}

	ret = copy_from_user(ctrl_reorder, args_rdr,
			     sizeof(struct msg_req_reorder_request_t));
	if (ret != 0) {
		pr_err("[%s] copy_from_user.%d\n", __func__, ret);
		goto free_reorder_kmalloc_memory;
	}

	dev->curr_cpe_rate_value[CLK_RATE_INDEX_MATCHER] = CLK_RATE_TURBO;
	ret = reorder_process_internal(dev, ctrl_reorder);
	if (ret != 0)
		pr_err("[%s] Failed : reorder_process_internal.%d\n",
		       __func__, ret);

free_reorder_kmalloc_memory:
	kfree(ctrl_reorder);
	return ret;
}

static int compare_process_internal(struct hipp_adapter_s *dev,
		struct msg_req_compare_request_t *ctrl_compare)
{
	unsigned int ret;

	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
		dev->curr_cpe_rate_value[CLK_RATE_INDEX_MATCHER]);
	if (ret != 0) {
		pr_err("[%s] Failed : ipp_set_rate.%d\n", __func__, ret);

		ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv, CLK_RATE_SVS);
		if (ret != 0) {
			pr_err("[%s] Failed to set ipp  rate mode: %d.\n",
				__func__, CLK_RATE_SVS);
			return ret;
		}
	}

	ret = hipp_orcm_clk_enable();
	if (ret != 0) {
		pr_err("[%s] Failed : hipp_orcm_clk_enable.%d\n",
			__func__, ret);
		return ret;
	}

	ret = hispcpe_compare_request(dev, ctrl_compare);
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_compare_request.%d\n",
			__func__, ret);

		if (hipp_orcm_clk_disable() != 0)
			pr_err("[%s] Failed : hipp_orcm_clk_disable\n",
			__func__);

		return ret;
	}

	ret = hipp_orcm_clk_disable();
	if (ret != 0) {
		pr_err("[%s] Failed : hipp_orcm_clk_disable.%d\n",
			__func__, ret);
		return ret;
	}

	dev->curr_cpe_rate_value[CLK_RATE_INDEX_MATCHER] = CLK_RATE_SVS;

	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
		dev->curr_cpe_rate_value[CLK_RATE_INDEX_MATCHER]);
	if (ret != 0)
		pr_err("[%s] Failed : ipp_set_rate.%d\n",
			__func__, ret);

	return ret;
}

int compare_process(unsigned long args)
{
	unsigned int ret = 0;
	struct msg_req_compare_request_t *ctrl_compare = NULL;
	void __user *args_cmp = (void __user *)(uintptr_t) args;
	struct hipp_adapter_s *dev = g_hipp_adapter;

	if ((args_cmp == NULL) || (dev == NULL)) {
		pr_err("[%s] args_cmp NULL.%pK\n", __func__, args_cmp);
		return -EINVAL;
	}

	ctrl_compare = kmalloc(sizeof(struct msg_req_compare_request_t), GFP_KERNEL);

	if (ctrl_compare == NULL)
		return -EINVAL;

	if (memset_s(ctrl_compare, sizeof(struct msg_req_compare_request_t),
		     0, sizeof(struct msg_req_compare_request_t))) {
		pr_err(" fail to memset_s ctrl_compare\n");
		goto free_compare_kmalloc_memory;
	}

	ret = copy_from_user(ctrl_compare, args_cmp,
			     sizeof(struct msg_req_compare_request_t));
	if (ret != 0) {
		pr_err("[%s] copy_from_user.%d\n", __func__, ret);
		goto free_compare_kmalloc_memory;
	}

	dev->curr_cpe_rate_value[CLK_RATE_INDEX_MATCHER] = CLK_RATE_TURBO;

	ret = compare_process_internal(dev, ctrl_compare);
	if (ret != 0)
		pr_err("[%s] Failed : compare_process_internal.%d\n",
		       __func__, ret);

free_compare_kmalloc_memory:
	kfree(ctrl_compare);
	return ret;
}

#ifdef CONFIG_IPP_DEBUG
int hipp_adapter_map_iommu(struct memory_block_s *frame_buf)
{
	unsigned long iova = 0;
	unsigned long iova_size = 0;
	int ret = 0;
	struct hipp_adapter_s *dev = g_hipp_adapter;
	struct dma_buf *modulebuf = NULL;

	I("+\n");

	if ((frame_buf == NULL) || (dev == NULL)) {
		pr_err("[%s] Failed : pstFrameBuf or dev NULL\n", __func__);
		return -EINVAL;
	}

	if (frame_buf->shared_fd < 0) {
		pr_err("[%s] Failed : share_fd.%d\n",
		       __func__, frame_buf->shared_fd);
		return -EINVAL;
	}

	if (dev->ippdrv == NULL) {
		pr_err("[%s] Failed : ippdrv NULL\n", __func__);
		return -EINVAL;
	}

	if (dev->ippdrv->iommu_map == NULL) {
		pr_err("[%s] Failed : iommu.NULL\n", __func__);
		return -EINVAL;
	}

	modulebuf = dma_buf_get(frame_buf->shared_fd);
	if (IS_ERR_OR_NULL(modulebuf)) {
		pr_err("[%s] Failed : shared_fd.%d\n", __func__, frame_buf->shared_fd);
		return -EINVAL;
	}

	if (modulebuf->size != (size_t)frame_buf->size) {
		pr_err("[%s] Failed : size not equal\n", __func__);
		dma_buf_put(modulebuf);
		return -EINVAL;
	}

	iova = dev->ippdrv->iommu_map(dev->ippdrv, modulebuf,
				      frame_buf->prot, &iova_size);
	if (iova == 0) {
		dma_buf_put(modulebuf);
		pr_err("[%s] Failed : iommu sharedfd.%d, prot.0x%lx\n",
		       __func__, frame_buf->shared_fd,
		       frame_buf->prot);
		return -EINVAL;
	}

	if (frame_buf->size != iova_size) {
		pr_err("[%s] Failed : iommu_map, size.0x%x, out.0x%lx\n",
		       __func__, frame_buf->size, iova_size);
		goto err_dma_buf_get;
	}

	frame_buf->da = iova;
	dma_buf_put(modulebuf);
	pr_info("[after map iommu]da.(0x%x)", frame_buf->da);
	I("-\n");
	return ISP_IPP_OK;

err_dma_buf_get:
	if (dev->ippdrv->iommu_unmap != NULL) {
		ret = dev->ippdrv->iommu_unmap(dev->ippdrv, modulebuf, iova);
		if (ret != 0)
			pr_err ("[%s] Failed :sharedfd.%d, iova.0x%lx\n",
				__func__, frame_buf->shared_fd, iova);
	}
	dma_buf_put(modulebuf);

	return -EINVAL;
}

int hipp_adapter_unmap_iommu(struct memory_block_s *frame_buf)
{
	int ret = 0;
	struct hipp_adapter_s *dev = g_hipp_adapter;
	struct dma_buf *modulebuf = NULL;

	I("+\n");

	if ((frame_buf == NULL) || (dev == NULL)) {
		pr_err("[%s] Failed : pstFrameBuf or dev NULL\n", __func__);
		return -EINVAL;
	}

	if (frame_buf->shared_fd < 0) {
		pr_err("[%s] Failed : share_fd.%d\n",
		       __func__, frame_buf->shared_fd);
		return -EINVAL;
	}

	if (frame_buf->da == 0) {
		pr_err("[%s] Failed : iova.%d\n", __func__, frame_buf->da);
		return -EINVAL;
	}

	if (dev->ippdrv == NULL) {
		pr_err("[%s] Failed : ippdrv NULL\n", __func__);
		return -EINVAL;
	}

	if (dev->ippdrv->iommu_unmap == NULL) {
		pr_err("[%s] Failed : iommu_unmap.NULL\n", __func__);
		return -EINVAL;
	}

	modulebuf = dma_buf_get(frame_buf->shared_fd);
	if (IS_ERR_OR_NULL(modulebuf)) {
		pr_err("[%s] Failed : shared_fd.%d\n", __func__, frame_buf->shared_fd);
		return -EINVAL;
	}

	ret = dev->ippdrv->iommu_unmap(dev->ippdrv, modulebuf, frame_buf->da);
	if (ret != 0)
		pr_err("[%s] Failed : ipp: sharedfd.%d\n", __func__, frame_buf->shared_fd);
	dma_buf_put(modulebuf);

	I("-\n");

	return ret;
}
#endif

int hispcpe_map_kernel(unsigned long args)
{
	void* virt_addr = NULL;
	struct map_buff_block_s frame_buf = { 0 };
	unsigned long iova;
	unsigned long iova_size = 0;
	int ret;
	void __user *args_mapkernel = (void __user *)(uintptr_t) args;
	struct hipp_adapter_s *dev = g_hipp_adapter;
	struct dma_buf *modulebuf = NULL;

	pr_info("[%s] +\n", __func__);

	if ((args_mapkernel == NULL) || (dev == NULL)) {
		pr_err("[%s] args_mapkernel.%pK\n", __func__, args_mapkernel);
		return -EINVAL;
	}

	ret = copy_from_user(&frame_buf, args_mapkernel, sizeof(frame_buf));
	if (ret != 0) {
		pr_err("[%s] copy_from_user.%d\n", __func__, ret);
		return -EFAULT;
	}

	pr_info("[%s] :sharedfd.%d\n", __func__, frame_buf.shared_fd);
	if (frame_buf.shared_fd < 0) {
		pr_err("[%s] Failed : share_fd.%d\n",
		       __func__, frame_buf.shared_fd);
		return -EINVAL;
	}

	if (dev->ippdrv == NULL) {
		pr_err("[%s] Failed : ippdrv NULL\n", __func__);
		return -EINVAL;
	}

	if (dev->ippdrv->iommu_map == NULL) {
		pr_err("[%s] Failed : ippdrv->iommu_map NULL\n", __func__);
		return -EINVAL;
	}

	if (dev->ippdrv->kmap == NULL) {
		pr_err("[%s] Failed : ippdrv->kmap NULL\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&dev->dev_lock);

	if (dev->mapbuf_ready == 1) {
		pr_info("[%s] Failed : map_kernel already done.%d\n",
			__func__, ret);
		mutex_unlock(&dev->dev_lock);
		return -EINVAL;
	}

	modulebuf = dma_buf_get(frame_buf.shared_fd);
	if (IS_ERR_OR_NULL(modulebuf)) {
		pr_err("[%s] Failed : shared_fd.%d\n", __func__, frame_buf.shared_fd);
		mutex_unlock(&dev->dev_lock);
		return -EINVAL;
	}

	if (modulebuf->size != frame_buf.size) {
		pr_err("[%s] Failed : size not equal\n", __func__);
		dma_buf_put(modulebuf);
		mutex_unlock(&dev->dev_lock);
		return -EINVAL;
	}

	iova = dev->ippdrv->iommu_map(dev->ippdrv, modulebuf, frame_buf.prot, &iova_size);
	if (iova == 0) {
		pr_err("[%s] Failed : iommu sharedfd.%d, prot.0x%llu\n",
			__func__, frame_buf.shared_fd, frame_buf.prot);
		dma_buf_put(modulebuf);
		mutex_unlock(&dev->dev_lock);
		return -EINVAL;
	}

	if (iova_size != frame_buf.size) {
		pr_err("[%s] Failed : iommu_map sharedfd.%d, size.%d\n",
			__func__, frame_buf.shared_fd, frame_buf.size);
		goto free_iommu_map;
	}

	virt_addr = dev->ippdrv->kmap(dev->ippdrv, modulebuf);
	if (virt_addr == NULL) {
		pr_err("[%s] Failed : kmap,sharedfd.%d\n",
			__func__, frame_buf.shared_fd);
		goto free_iommu_map;
	}

	dev->virt_addr = virt_addr;
	dev->daddr = iova;
	dev->dmabuf = modulebuf;

	dev->mapbuf_ready = 1;

	pr_info("[%s]: da.0x%lx", __func__, iova);
	mutex_unlock(&dev->dev_lock);
	return ISP_IPP_OK;

free_iommu_map:
	if (dev->ippdrv->iommu_unmap != NULL) {
		ret = dev->ippdrv->iommu_unmap(dev->ippdrv, modulebuf, iova);
		if (ret != 0)
			pr_err("[%s] Failed : iommu_unmap, sharedfd.%d, iova.0x%lx\n",
				__func__, frame_buf.shared_fd, iova);
	}
	dma_buf_put(modulebuf);
	dev->dmabuf = NULL;
	mutex_unlock(&dev->dev_lock);
	return -EINVAL;
}

int hispcpe_unmap_kernel(void)
{
	int ret;
	struct hipp_adapter_s *dev = g_hipp_adapter;

	if (dev == NULL) {
		pr_err("[%s] Failed : dev.%pK\n", __func__, dev);
		return -EINVAL;
	}

	if (dev->ippdrv == NULL) {
		pr_err("[%s] Failed : ippdrv NULL\n", __func__);
		return -EINVAL;
	}

	if (dev->ippdrv->iommu_unmap == NULL) {
		pr_err("[%s] Failed : ippdrv->iommu_unmap NULL\n", __func__);
		return -EINVAL;
	}

	if (dev->ippdrv->kunmap == NULL) {
		pr_err("[%s] Failed : ippdrv->kunmap NULL\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&dev->dev_lock);
	if (dev->mapbuf_ready == 0) {
		pr_info("[%s] Failed : no map_kernel ops before\n", __func__);
		mutex_unlock(&dev->dev_lock);
		return 0;
	}

	ret = hispcpe_work_check(dev, HIPP_PD_CHECK);
	if (ret < 0) {
		mutex_unlock(&dev->dev_lock);
		return -ENOMEM;
	}

	if (dev->daddr != 0) {
		ret = dev->ippdrv->iommu_unmap(dev->ippdrv, dev->dmabuf, dev->daddr);
		if (ret != 0) {
			pr_err("[%s] Failed : iommu_unmap.%d\n", __func__, ret);
			mutex_unlock(&dev->dev_lock);
			return -EINVAL;
		}
		dev->daddr = 0;
	}

	ret = dev->ippdrv->kunmap(dev->ippdrv, dev->dmabuf, dev->virt_addr);
	if (ret != 0) {
		pr_err("[%s] Failed : dev->ippdrv->kunmap.%d\n", __func__, ret);
		mutex_unlock(&dev->dev_lock);
		return -EINVAL;
	}

	dma_buf_put(dev->dmabuf);
	dev->dmabuf = NULL;
	dev->virt_addr = NULL;
	dev->mapbuf_ready = 0;

	mutex_unlock(&dev->dev_lock);
	pr_info("[%s] : exit\n", __func__);

	return ISP_IPP_OK;
}

int orb_process(unsigned long args)
{
	unsigned int ret = 0;
	unsigned int orb_rate_index;
	struct msg_req_orb_request_t *ctrl_orb = NULL;
	void __user *args_orb = (void __user *)(uintptr_t) args;
	struct hipp_adapter_s *dev = g_hipp_adapter;

	if (args_orb == NULL) {
		pr_err("[%s] args_orb NULL.%pK\n", __func__, args_orb);
		return -EINVAL;
	}

	ctrl_orb = vmalloc(sizeof(struct msg_req_orb_request_t));
	if (ctrl_orb == NULL)
		return -EINVAL;

	if (memset_s(ctrl_orb, sizeof(struct msg_req_orb_request_t),
		     0, sizeof(struct msg_req_orb_request_t))) {
		pr_err(" fail to memset_s ctrl_orb\n");
		goto free_orb_kmalloc_memory;
	}

	ret = copy_from_user(ctrl_orb, args_orb,
			     sizeof(struct msg_req_orb_request_t));
	if (ret != 0) {
		pr_err("[%s] copy_from_user.%d\n", __func__, ret);
		goto free_orb_kmalloc_memory;
	}

	orb_rate_index = ctrl_orb->orb_rate_value;
	orb_rate_index = (orb_rate_index > ORB_RATE_INDEX_MAX) ?
	    0 : orb_rate_index;
	dev->curr_cpe_rate_value[CLK_RATE_INDEX_ORB] = orb_rate_index;

	if (ctrl_orb->orb_pyramid_layer > ORB_LAYER_MAX ||
		ctrl_orb->req_matcher.cmp_pyramid_layer > ORB_LAYER_MAX) {
		pr_err("[%s]Failed:Invalid orb_pyramid_layer = %d,cmp_pyramid_layer = %d\n",
			__func__, ctrl_orb->orb_pyramid_layer,
			ctrl_orb->req_matcher.cmp_pyramid_layer);
		goto free_orb_kmalloc_memory;
	}

	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
		dev->curr_cpe_rate_value[CLK_RATE_INDEX_ORB]);
	if (ret != 0) {
		pr_err("[%s] Failed : ipp_set_rate.%d\n", __func__, ret);

		ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv, CLK_RATE_SVS);
		if (ret != 0) {
			pr_err("[%s] Failed to set ipp  rate mode: %d.\n",
			       __func__, CLK_RATE_SVS);
			goto free_orb_kmalloc_memory;
		}
	}

	ret = hipp_orcm_clk_enable();
	if (ret != 0) {
		pr_err("[%s] Failed : hipp_orcm_clk_enable.%d\n",
		       __func__, ret);
		goto free_orb_kmalloc_memory;
	}

	ret = hispcpe_orb_request(dev, ctrl_orb);
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_orb_request.%d\n",
			__func__, ret);

		if (hipp_orcm_clk_disable() != 0)
			pr_err("[%s] Failed : hipp_orcm_clk_disable\n",
			       __func__);

		goto free_orb_kmalloc_memory;
	}

	ret = hipp_orcm_clk_disable();
	if (ret != 0) {
		pr_err("[%s] Failed : hipp_orcm_clk_disable.%d\n",
		       __func__, ret);
		goto free_orb_kmalloc_memory;
	}

	dev->curr_cpe_rate_value[CLK_RATE_INDEX_ORB] = CLK_RATE_SVS;

	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
		dev->curr_cpe_rate_value[CLK_RATE_INDEX_ORB]);
	if (ret != 0)
		pr_err("[%s] Failed : ipp_set_rate.%d\n", __func__, ret);

free_orb_kmalloc_memory:
	vfree(ctrl_orb);
	return ret;
}

int hipp_cmd_gf_req(unsigned long args)
{
	struct hipp_adapter_s *dev = g_hipp_adapter;
	int ret;

	mutex_lock(&dev->dev_lock);
	if (dev->mapbuf_ready == 0) {
		pr_err("[%s] no map_kernel ops before\n", __func__);
		mutex_unlock(&dev->dev_lock);
		return -EINVAL;
	}

	if (dev->refs_power_up<= 0) {
		pr_err("[%s] Failed : ipp_power_lock.%d\n", __func__, dev->refs_power_up);
		mutex_unlock(&dev->dev_lock);
		return -EINVAL;
	}

	if (dev->hipp_refs[REFS_TYPE_GF] > 0) {
		pr_err("[%s] : gf  refs Opened .\n", __func__);
		mutex_unlock(&dev->dev_lock);
		return -EINVAL;
	}

	dev->hipp_refs[REFS_TYPE_GF] = 1;
	mutex_unlock(&dev->dev_lock);

	ret = gf_process(args);
	if (ret < 0)
		pr_err("[%s] Failed : hispcpe_gf_request.%d\n", __func__, ret);

	dev->hipp_refs[REFS_TYPE_GF] = 0;
	return ret;
}

int hipp_cmd_vbk_req(unsigned long args)
{
	struct hipp_adapter_s *dev = g_hipp_adapter;
	int ret;

	pr_info("[%s] +\n", __func__);

	mutex_lock(&dev->dev_lock);
	if (dev->refs_power_up <= 0) {
		pr_err("[%s] Failed : ipp_power_lock.%d\n", __func__, dev->refs_power_up);
		mutex_unlock(&dev->dev_lock);
		return -EINVAL;
	}

	if (dev->mapbuf_ready == 0) {
		pr_err("[%s] no map_kernel ops before\n", __func__);
		mutex_unlock(&dev->dev_lock);
		return -EINVAL;
	}

	if (dev->hipp_refs[REFS_TYP_VBK] > 0) {
		pr_err("[%s] : vbk  refs Opened .\n", __func__);
		mutex_unlock(&dev->dev_lock);
		return -EINVAL;
	}

	dev->hipp_refs[REFS_TYP_VBK] = 1;
	mutex_unlock(&dev->dev_lock);

	ret = vbk_process(args);
	if (ret < 0)
		pr_err("[%s] Failed : vbk_process.%d\n", __func__, ret);

	dev->hipp_refs[REFS_TYP_VBK] = 0;

	pr_info("[%s] -\n", __func__);
	return ret;
}

int hipp_cmd_orb_req(unsigned long args)
{
	struct hipp_adapter_s *dev = g_hipp_adapter;
	int ret;

	pr_info("[%s] +\n", __func__);

	mutex_lock(&dev->dev_lock);
	if (dev->mapbuf_ready == 0) {
		pr_err("[%s] no map_kernel ops before\n", __func__);
		mutex_unlock(&dev->dev_lock);
		return -EINVAL;
	}

	if (dev->refs_power_up<= 0) {
		pr_err("[%s] Failed : ipp_power_lock.%d\n", __func__, dev->refs_power_up);
		mutex_unlock(&dev->dev_lock);
		return -EINVAL;
	}

	if (dev->hipp_refs[REFS_TYP_ORCM] > 0) {
		pr_err("[%s] : orcm refs Opened\n", __func__);
		mutex_unlock(&dev->dev_lock);
		return -EINVAL;
	}

	dev->hipp_refs[REFS_TYP_ORCM] = 1;
	mutex_unlock(&dev->dev_lock);

	ret = orb_process(args);
	if (ret < 0)
		pr_err("[%s] Failed : orb_process.%d\n", __func__, ret);

	dev->hipp_refs[REFS_TYP_ORCM] = 0;

	pr_info("[%s] -\n", __func__);
	return ret;
}

void hipp_adapter_exception(void)
{
	int ret;

	pr_alert("[%s] : enter\n", __func__);

	ret = hipp_powerdn(HIPP_PD_FORCE);
	if (ret < 0)
		pr_err("[%s] Failed : hipp_powerdn.%d\n", __func__, ret);

	ret = hispcpe_unmap_kernel();
	if (ret < 0)
		pr_err("[%s] Failed : hispcpe_umap_kernel.%d\n", __func__, ret);

	relax_ipp_wakelock();
}

int hipp_adapter_probe(struct platform_device *pdev)
{
	struct hipp_adapter_s *dev = NULL;
	unsigned int i = 0;

	pr_info("[%s] +\n", __func__);

	g_hipp_adapter = NULL;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (dev == NULL)
		return -ENOMEM;

	dev->ippdrv = hipp_common_register(IPP_UNIT, &pdev->dev);
	if (dev->ippdrv == NULL) {
		pr_err("[%s] Failed : IPP_UNIT register\n", __func__);
		goto free_dev;
	}

	init_waitqueue_head(&dev->gf_wait);
	init_waitqueue_head(&dev->vbk_wait);
	init_waitqueue_head(&dev->reorder_wait);
	init_waitqueue_head(&dev->compare_wait);
	init_waitqueue_head(&dev->orb_enh_wait);
	init_waitqueue_head(&dev->mc_wait);
	init_waitqueue_head(&dev->orb_wait);

	for (i = 0; i < REFS_TYP_MAX; i++)
		dev->hipp_refs[i] = 0;

	mutex_init(&dev->dev_lock);
	dev->refs_power_up = 0;
	dev->mapbuf_ready = 0;

	g_hipp_adapter = dev;

	cmdlst_priv_prepare();

	return 0;

free_dev:
	kfree(dev);
	return -ENOMEM;
}

void hipp_adapter_remove(void)
{
	struct hipp_adapter_s *dev = g_hipp_adapter;
	int ret;
	unsigned int i = 0;

	I("+\n");

	if (dev == NULL) {
		pr_err("[%s] Failed : drvdata NULL\n", __func__);
		return;
	}
	dev->mapbuf_ready = 0;

	ret = hipp_common_unregister(IPP_UNIT);
	if (ret != 0)
		pr_err("[%s] Faield : IPP unregister.%d\n", __func__, ret);

	if (dev->ippdrv != NULL)
		dev->ippdrv = NULL;

	for (i = 0; i < REFS_TYP_MAX; i++)
		dev->hipp_refs[i] = 0;

	mutex_destroy(&dev->dev_lock);
	kfree(dev);

	I("-\n");
}

