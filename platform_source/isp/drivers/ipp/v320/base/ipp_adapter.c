/*
 * IPP CPE
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
#include "gf.h"
#include "orb.h"
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
			/*lint -e578 */
			ret = wait_event_interruptible_timeout(dev->gf_wait,
					dev->gf_ready,
					timeout);
		/*lint +e578 */
		else if (wait_mode == WAIT_MODE_ORB)
			/*lint -e578 */
			ret = wait_event_interruptible_timeout(dev->orb_wait,
					dev->orb_ready,
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
	if (crg_base == NULL) {
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
		if (value & (0x1 << IPP_ORB_CHANNEL_ID)) {
			dev->orb_ready = 1;
			wake_up_interruptible(&dev->orb_wait);
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
	if (atomic_read(&dev->mapbuf_ready) <= 0) {
		pr_err("[%s] no map_kernel ops before.0x%x\n",
			__func__, atomic_read(&dev->mapbuf_ready));
		return 0;
	}
	return 1;
}


int dataflow_cvdr_global_config(void)
{
	unsigned int reg_val;
	void __iomem *crg_base;
	crg_base = hipp_get_regaddr(CVDR_REG);
	if (crg_base == NULL) {
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
	loge_if_ret(hispcpe_reg_set(CPE_TOP, IPP_TOP_CMDLST_IRQ_CLR_0_REG,
			(0xFFFF & ipp_irq_mask)));
	loge_if_ret(hispcpe_reg_set(CPE_TOP, IPP_TOP_CMDLST_IRQ_MSK_0_REG,
			(0xFFFF & ~ipp_irq_mask)));
	loge_if_ret(hispcpe_reg_set(CMDLIST_REG, 0x80 + IPP_GF_CHANNEL_ID * 0x80,
			cmdlst_channel_value));
	loge_if_ret(hispcpe_reg_set(CMDLIST_REG, 0x80 + IPP_ORB_CHANNEL_ID * 0x80,
			cmdlst_channel_value));
	return ISP_IPP_OK;
}

int ipp_cfg_qos_reg(void)
{
	unsigned int ipp_qos_value = 0;
	void __iomem *crg_base;
	crg_base = hipp_get_regaddr(CPE_TOP);
	if (crg_base == NULL) {
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
		pr_err("[%s] Failed : dev->ippdrv NULL\n", __func__);
		return -EINVAL;
	}
	if (dev->ippdrv->power_up == NULL) {
		pr_err("[%s] Failed : dev->ippdrv->power_up.NULL\n", __func__);
		return -EINVAL;
	}
	if (dev->ippdrv->enable_smmu == NULL) {
		pr_err("[%s] Failed : ippdrv->enable_smmu.NULL\n", __func__);
		return -EINVAL;
	}
	if (dev->ippdrv->disable_smmu == NULL) {
		pr_err("[%s] Failed : ippdrv->disable_smmu.NULL\n", __func__);
		return -EINVAL;
	}
	if (dev->ippdrv->setsid_smmu == NULL) {
		pr_err("[%s] Failed : ippdrv->setsid_smmu.NULL\n", __func__);
		return -EINVAL;
	}
	mutex_lock(&dev->ipp_power_lock);
	if (dev->refs_power_up > 0) {
		dev->refs_power_up++;
		pr_info("[%s]: refs_power_up.%d\n", __func__, dev->refs_power_up);
		mutex_unlock(&dev->ipp_power_lock);
		return ISP_IPP_OK;
	}
	ret = dev->ippdrv->power_up(dev->ippdrv);
	if (ret != 0) {
		pr_err("[%s] Failed : dev->ippdrv->power_up.%d\n",
			__func__, ret);
		mutex_unlock(&dev->ipp_power_lock);
		return ret;
	}
	ret = dev->ippdrv->enable_smmu(dev->ippdrv);
	if (ret != 0) {
		pr_err("[%s] Failed : dev->ippdrv->enable_smmu.%d\n",
			__func__, ret);
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
		pr_err("[%s] Failed : dev->ippdrv->enable_smmu.%d\n",
			__func__, ret);
		goto fail_disable_smmu;
	}
	ret = dev->ippdrv->setsid_smmu(dev->ippdrv, IPPCORE_SWID_INDEX1,
			IPPCORE_SWID_LEN1, sid, ssid);
	if (ret != 0) {
		pr_err("[%s] Failed : dev->ippdrv->enable_smmu.%d\n",
			__func__, ret);
		goto fail_disable_smmu;
	}
	ret = dev->ippdrv->set_pref(dev->ippdrv,
			IPPCORE_SWID_INDEX0, IPPCORE_SWID_LEN0);
	if (ret != 0) {
		pr_err("[%s] Failed : dev->ippdrv->enable_smmu.%d\n",
			__func__, ret);
		goto fail_disable_smmu;
	}
	ret = dev->ippdrv->set_pref(dev->ippdrv,
			IPPCORE_SWID_INDEX1, IPPCORE_SWID_LEN1);
	if (ret != 0) {
		pr_err("[%s] Failed : dev->ippdrv->enable_smmu.%d\n",
			__func__, ret);
		goto fail_disable_smmu;
	}
	ret = hispcpe_irq_config();
	if (ret != 0) {
		pr_err("[%s] Failed : set irq config.%d\n",
			__func__, ret);
		goto fail_disable_smmu;
	}
	dev->refs_power_up++;
	mutex_unlock(&dev->ipp_power_lock);
	D("-\n");
	return ISP_IPP_OK;
fail_disable_smmu:
	ret = dev->ippdrv->disable_smmu(dev->ippdrv);
	if (ret != 0)
		pr_err("[%s] Failed : dev->ippdrv->disable_smmu.%d\n",
			__func__, ret);
fail_power_down:
	ret = dev->ippdrv->power_dn(dev->ippdrv);
	if (ret != 0)
		pr_err("[%s] Failed : dev->ippdrv->power_dw.%d\n",
			__func__, ret);
	mutex_unlock(&dev->ipp_power_lock);
	return ret;
}

int hipp_powerdn(void)
{
	int ret;
	struct hipp_adapter_s *dev = g_hipp_adapter;
	pr_info("%s: +", __func__);
	if (dev == NULL) {
		pr_err("[%s] Failed : dev.%pK\n", __func__, dev);
		return -EINVAL;
	}
	if (dev->ippdrv == NULL) {
		pr_err("[%s] Failed : ippdrv.%pK\n", __func__, dev->ippdrv);
		return -ENOMEM;
	}
	if (dev->ippdrv->disable_smmu == NULL) {
		pr_err("[%s] Failed : ippdrv->disable_smmu.%pK\n",
			__func__, dev->ippdrv->disable_smmu);
		return -ENOMEM;
	}
	mutex_lock(&dev->ipp_work_lock);
	if (dev->hipp_refs[REFS_TYPE_GF] > 0) {
		pr_err("[%s] : gf  refs Opened .\n", __func__);
		mutex_unlock(&dev->ipp_work_lock);
		return -ENOMEM;
	}
	if (dev->hipp_refs[REFS_TYP_ORB] > 0) {
		pr_err("[%s] : orcm  refs Opened .\n", __func__);
		mutex_unlock(&dev->ipp_work_lock);
		return -ENOMEM;
	}
	mutex_unlock(&dev->ipp_work_lock);
	mutex_lock(&dev->ipp_power_lock);
	if (dev->refs_power_up <= 0) {
		pr_err("[%s] : ippcore no pw\n",  __func__);
		mutex_unlock(&dev->ipp_power_lock);
		return ISP_IPP_OK;
	}
	dev->refs_power_up--;
	if (dev->refs_power_up > 0) {
		pr_info("[%s]: refs_power_up.%d\n", __func__, dev->refs_power_up);
		mutex_unlock(&dev->ipp_power_lock);
		return ISP_IPP_OK;
	}
	ret = dev->ippdrv->disable_smmu(dev->ippdrv);
	if (ret != 0)
		pr_err("[%s] Failed : disable_smmu.%d\n", __func__, ret);
	ret = dev->ippdrv->power_dn(dev->ippdrv);
	if (ret != 0)
		pr_err("[%s] Failed : power_dn.%d\n", __func__, ret);
	mutex_unlock(&dev->ipp_power_lock);
	pr_info("%s: -", __func__);
	return ISP_IPP_OK;
}

static int hipp_orb_clk_enable(void)
{
	void __iomem *crg_base;
	D("+\n");
	crg_base = hipp_get_regaddr(CPE_TOP);
	if (crg_base == NULL) {
		pr_err("[%s] Failed : hipp_get_regaddr\n", __func__);
		return -EINVAL;
	}
	writel(IPP_CLK_EN, crg_base + IPP_TOP_ORB_CRG_CFG0_REG);
	D("-\n");
	return ISP_IPP_OK;
}

static int hipp_orb_clk_disable(void)
{
	void __iomem *crg_base;
	D("+\n");
	crg_base = hipp_get_regaddr(CPE_TOP);
	if (crg_base == NULL) {
		pr_err("[%s] Failed : hipp_get_regaddr\n", __func__);
		return -EINVAL;
	}
	writel(IPP_CLK_DISABLE, crg_base + IPP_TOP_ORB_CRG_CFG0_REG);
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
	if (crg_base == NULL) {
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
	if (crg_base == NULL) {
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
		pr_err("[%s] Failed : gf_eop_handler.%d\n", __func__, ret1);
	D("-\n");
	if (ret1 != 0 || ret != 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
}

static int  hipp_orb_wait_flag(struct hipp_adapter_s *dev)
{
	int ret0 = 0;
	ret0 = hispcpe_clean_wait_flag(dev,
			WAIT_MODE_ORB);
	if (ret0 < 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
}

static int hipp_orb_wait(struct hipp_adapter_s *dev)
{
	int ret0 = 0;
	ret0 = hispcpe_wait(dev,
			WAIT_MODE_ORB);
	if (ret0 < 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
}

static int  hipp_orb_eop_handle(struct hipp_adapter_s *dev)
{
	int ret0 = 0;
	ret0 = ipp_eop_handler(CMD_EOF_ORB_MODE);
	if (ret0 < 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
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
	ret = hipp_orb_wait_flag(dev);
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
	ret = hipp_orb_wait(dev);
	if (ret < 0) {
		pr_err("[%s] Failed : hispcpe_orcm_wait.%d\n",
			__func__, ret);
		ret = ISP_IPP_ERR;
		goto ORB_REQ_DONE;
	}
ORB_REQ_DONE:
	ret1 = hipp_orb_eop_handle(dev);
	if (ret1 != 0)
		pr_err("[%s] Failed : orb_orcm_eop_handle.%d\n",
			__func__, ret);
	D("-\n");
	if (ret != 0 || ret1 != 0)
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
	if (ret != 0) {
		pr_err("[%s] Failed : ipp_set_rate.%d\n", __func__, ret);
		return ret;
	}
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
	if (atomic_read(&dev->mapbuf_ready) <= 0) {
		pr_err("[%s] no map_kernel ops before.0x%x\n",
			__func__, atomic_read(&dev->mapbuf_ready));
		return -EINVAL;
	}
	mutex_lock(&dev->ipp_power_lock);
	if (dev->refs_power_up <= 0) {
		pr_err("[%s] Failed : ipp_power_lock.%d\n", __func__, dev->refs_power_up);
		mutex_unlock(&dev->ipp_power_lock);
		return -EINVAL;
	}
	mutex_unlock(&dev->ipp_power_lock);
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
	if (ret != 0) {
		pr_err("[%s] Failed : ipp_set_rate.%d\n", __func__, ret);
		goto free_mcf_kmalloc_memory;
	}
free_mcf_kmalloc_memory:
	kfree(ctrl_mcf);
	return ret;
}

int hipp_adapter_map_iommu(struct memory_block_s *frame_buf)
{
	unsigned long iova = 0;
	unsigned long iova_size = 0;
	int ret = 0;
	struct hipp_adapter_s *dev = g_hipp_adapter;
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
	iova = dev->ippdrv->iommu_map(dev->ippdrv,
			frame_buf->shared_fd,
			frame_buf->prot, &iova_size);
	if (iova == 0) {
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
	pr_info("[after map iommu]da.(0x%x)", frame_buf->da);
	I("-\n");
	return ISP_IPP_OK;
err_dma_buf_get:
	if (dev->ippdrv->iommu_unmap == NULL) {
		pr_err("[%s] Failed : iommu_unmap.NULL\n", __func__);
		return -EINVAL;
	}
	ret = dev->ippdrv->iommu_unmap(dev->ippdrv,
			frame_buf->shared_fd, iova);
	if (ret == 0)
		pr_err
		("[%s] Failed : ipp: sharedfd.%d, iova.0x%lx\n",
			__func__, frame_buf->shared_fd, iova);
	return -EINVAL;
}

int hipp_adapter_unmap_iommu(struct memory_block_s *frame_buf)
{
	int ret = 0;
	struct hipp_adapter_s *dev = g_hipp_adapter;
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
	ret = dev->ippdrv->iommu_unmap(dev->ippdrv,
			frame_buf->shared_fd, frame_buf->da);
	if (ret != 0)
		pr_err("[%s] Failed : ipp: sharedfd.%d\n",
			__func__, frame_buf->shared_fd);
	I("-\n");
	return ret;
}

int hispcpe_map_kernel(unsigned long args)
{
	void *virt_addr = NULL;
	struct map_buff_block_s frame_buf = { 0 };
	unsigned long iova;
	unsigned long iova_size = 0;
	int ret;
	void __user *args_mapkernel = (void __user *)(uintptr_t) args;
	struct hipp_adapter_s *dev = g_hipp_adapter;
	pr_info("[%s] +\n", __func__);
	if ((args_mapkernel == NULL) || (dev == NULL)) {
		pr_err("[%s] args_mapkernel.%pK\n", __func__, args_mapkernel);
		return -EINVAL;
	}
	ret = atomic_read(&dev->mapbuf_ready);
	if (ret == 1) {
		pr_info("[%s] Failed : map_kernel already done.%d\n",
			__func__, ret);
		return -EINVAL;
	}
	ret = copy_from_user(&frame_buf, args_mapkernel, sizeof(frame_buf));
	if (ret != 0) {
		pr_err("[%s] copy_from_user.%d\n", __func__, ret);
		return -EFAULT;
	}
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
	iova = dev->ippdrv->iommu_map(dev->ippdrv,
			frame_buf.shared_fd, frame_buf.prot,
			&iova_size);
	if (iova == 0) {
		pr_err("[%s] Failed : iommu sharedfd.%d, prot.0x%llu\n",
			__func__, frame_buf.shared_fd, frame_buf.prot);
		return -EINVAL;
	}
	if (iova_size != frame_buf.size) {
		pr_err("[%s] Failed : iommu_map sharedfd.%d, size.%d\n",
			__func__, frame_buf.shared_fd, frame_buf.size);
		goto free_iommu_map;
	}
	virt_addr = dev->ippdrv->kmap(dev->ippdrv, frame_buf.shared_fd);
	if (virt_addr == NULL) {
		pr_err("[%s] Failed : kmap,sharedfd.%d\n",
			__func__, frame_buf.shared_fd);
		goto free_iommu_map;
	}
	dev->virt_addr = virt_addr;
	dev->daddr = iova;
	dev->shared_fd = frame_buf.shared_fd;
	atomic_set(&dev->mapbuf_ready, 1);
	pr_info("[after map kernel]virt_addr.0x%pK, da.0x%lx", virt_addr, iova);
	pr_info("[%s] -\n", __func__);
	return ISP_IPP_OK;
free_iommu_map:
	if (dev->ippdrv->iommu_unmap == NULL) {
		pr_err("[%s] Failed : ippdrv->iommu_unmap NULL\n", __func__);
		return -EINVAL;
	}
	ret = dev->ippdrv->iommu_unmap(dev->ippdrv,
			frame_buf.shared_fd, iova);
	if (ret == 0)
		pr_err("[%s] Failed : iommu_unmap, sharedfd.%d, iova.0x%lx\n",
			__func__, frame_buf.shared_fd, iova);
	return -EINVAL;
}

int hispcpe_unmap_kernel(void)
{
	int ret;
	struct hipp_adapter_s *dev = g_hipp_adapter;
	I("+\n");
	ret = atomic_read(&dev->mapbuf_ready);
	if (ret == 0) {
		pr_info("[%s] Failed : no map_kernel ops before.%d\n",
			__func__, ret);
		return -EINVAL;
	}
	atomic_set(&dev->mapbuf_ready, 0);
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
	mutex_lock(&dev->ipp_work_lock);
	if (dev->hipp_refs[REFS_TYPE_GF] > 0) {
		pr_err("[%s] : gf  refs Opened .\n", __func__);
		mutex_unlock(&dev->ipp_work_lock);
		return -ENOMEM;
	}
	if (dev->hipp_refs[REFS_TYP_ORB] > 0) {
		pr_err("[%s] : orcm  refs Opened .\n", __func__);
		mutex_unlock(&dev->ipp_work_lock);
		return -ENOMEM;
	}
	mutex_unlock(&dev->ipp_work_lock);
	ret = dev->ippdrv->iommu_unmap(dev->ippdrv, dev->shared_fd, dev->daddr);
	if (ret != 0) {
		pr_err("[%s] Failed : iommu_unmap.%d\n", __func__, ret);
		return -EINVAL;
	}
	ret = dev->ippdrv->kunmap(dev->ippdrv, dev->shared_fd, dev->virt_addr);
	if (ret != 0) {
		pr_err("[%s] Failed : dev->ippdrv->kunmap.%d\n", __func__, ret);
		return -EINVAL;
	}
	I("-\n");
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
	if (atomic_read(&dev->mapbuf_ready) <= 0) {
		pr_err("[%s] no map_kernel ops before.0x%x\n",
			__func__, atomic_read(&dev->mapbuf_ready));
		return -EINVAL;
	}
	mutex_lock(&dev->ipp_power_lock);
	if (dev->refs_power_up <= 0) {
		pr_err("[%s] Failed : ipp_power_lock.%d\n", __func__, dev->refs_power_up);
		mutex_unlock(&dev->ipp_power_lock);
		return -EINVAL;
	}
	mutex_unlock(&dev->ipp_power_lock);
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
	ret = hipp_orb_clk_enable();
	if (ret != 0) {
		pr_err("[%s] Failed : hipp_orb_clk_enable.%d\n",
			__func__, ret);
		goto free_orb_kmalloc_memory;
	}
	ret = hispcpe_orb_request(dev, ctrl_orb);
	if (ret != 0) {
		pr_err("[%s] Failed : hispcpe_orb_request.%d\n",
			__func__, ret);
		if (hipp_orb_clk_disable() != 0)
			pr_err("[%s] Failed : hipp_orb_clk_disable\n",
				__func__);
		goto free_orb_kmalloc_memory;
	}
	ret = hipp_orb_clk_disable();
	if (ret != 0) {
		pr_err("[%s] Failed : hipp_orb_clk_disable.%d\n",
			__func__, ret);
		goto free_orb_kmalloc_memory;
	}
	dev->curr_cpe_rate_value[CLK_RATE_INDEX_ORB] = CLK_RATE_SVS;
	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
			dev->curr_cpe_rate_value[CLK_RATE_INDEX_ORB]);
	if (ret != 0) {
		pr_err("[%s] Failed : ipp_set_rate.%d\n", __func__, ret);
		goto free_orb_kmalloc_memory;
	}
free_orb_kmalloc_memory:
	vfree(ctrl_orb);
	return ret;
}

int hipp_cmd_gf_req(unsigned long args)
{
	struct hipp_adapter_s *dev = g_hipp_adapter;
	int ret;
	mutex_lock(&dev->ipp_work_lock);
	if (dev->hipp_refs[REFS_TYPE_GF] > 0) {
		pr_err("[%s] : gf  refs Opened .\n", __func__);
		mutex_unlock(&dev->ipp_work_lock);
		return -EINVAL;
	}
	dev->hipp_refs[REFS_TYPE_GF] = 1;
	mutex_unlock(&dev->ipp_work_lock);
	ret = gf_process(args);
	if (ret < 0)
		pr_err("[%s] Failed : hispcpe_gf_request.%d\n", __func__, ret);
	mutex_lock(&dev->ipp_work_lock);
	dev->hipp_refs[REFS_TYPE_GF] = 0;
	mutex_unlock(&dev->ipp_work_lock);
	return ret;
}
int hipp_cmd_orb_req(unsigned long args)
{
	struct hipp_adapter_s *dev = g_hipp_adapter;
	int ret;
	mutex_lock(&dev->ipp_work_lock);
	if (dev->hipp_refs[REFS_TYP_ORB] > 0) {
		pr_err("[%s] : orcm  refs Opened .\n", __func__);
		mutex_unlock(&dev->ipp_work_lock);
		return -EINVAL;
	}
	dev->hipp_refs[REFS_TYP_ORB] = 1;
	mutex_unlock(&dev->ipp_work_lock);
	ret = orb_process(args);
	if (ret < 0)
		pr_err("[%s] Failed : orb_process.%d\n", __func__, ret);
	mutex_lock(&dev->ipp_work_lock);
	dev->hipp_refs[REFS_TYP_ORB] = 0;
	mutex_unlock(&dev->ipp_work_lock);
	return ret;
}

void hipp_adapter_exception(void)
{
	struct hipp_adapter_s *dev = g_hipp_adapter;
	int ret;
	pr_alert("[%s] : enter\n", __func__);
	atomic_set(&dev->mapbuf_ready, 0);
	mutex_lock(&dev->ipp_power_lock);
	if (dev->refs_power_up > 0) {
		dev->refs_power_up = 1;
		mutex_unlock(&dev->ipp_power_lock);
		ret = hipp_powerdn();
		if (ret < 0)
			pr_err("Failed : hipp_powerdn.%d\n", ret);
	} else {
		pr_info("[%s] : hipp no need powerdn.%d\n",
			__func__, dev->refs_power_up);
		mutex_unlock(&dev->ipp_power_lock);
	}
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
	init_waitqueue_head(&dev->orb_wait);
	for (i = 0; i < REFS_TYP_MAX; i++)
		dev->hipp_refs[i] = 0;
	mutex_init(&dev->ipp_power_lock);
	mutex_init(&dev->ipp_work_lock);
	dev->refs_power_up = 0;
	atomic_set(&dev->mapbuf_ready, 0);
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
	atomic_set(&dev->mapbuf_ready, 0);
	ret = hipp_common_unregister(IPP_UNIT);
	if (ret != 0)
		pr_err("[%s] Faield : IPP unregister.%d\n", __func__, ret);
	if (dev->ippdrv != NULL)
		dev->ippdrv = NULL;
	for (i = 0; i < REFS_TYP_MAX; i++)
		dev->hipp_refs[i] = 0;
	mutex_destroy(&dev->ipp_power_lock);
	mutex_destroy(&dev->ipp_work_lock);
	kfree(dev);
	I("-\n");
}

