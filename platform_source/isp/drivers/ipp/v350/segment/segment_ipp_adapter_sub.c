/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    segment_adapter.c
 * Description:
 *
 * Date         2020-04-17 16:28:10
 ********************************************************************/
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
#include "segment_gf.h"
#include "segment_arfeature.h"
#include "segment_reorder.h"
#include "segment_compare.h"
#include "segment_hiof.h"
#include "segment_orb_enh.h"
#include "segment_mc.h"
#include "segment_common.h"
#include "segment_ipp_path.h"
#include "ipp.h"
#include "ipp_top_drv_priv.h"
#include "ipp_top_reg_offset.h"
#include "memory.h"
#include "ipp_core.h"
#include "segment_ipp_adapter.h"

// debug
#include "cmdlst_reg_offset.h"
#include "cvdr_ipp_reg_offset.h"
#include "arfeature_reg_offset.h"
#include "orb_enh_reg_offset.h"

enum HIPP_CLK_STATUS {
	IPP_CLK_DISABLE = 0,
	IPP_CLK_EN = 1,
	IPP_CLK_STATUS_MAX
};

extern struct hipp_adapter_s *g_hipp_adapter;

int hipp_orcm_clk_enable(void)
{
	void __iomem *crg_base;
	logd("+");
	crg_base = hipp_get_regaddr(CPE_TOP);
	if (crg_base == NULL) {
		loge("Failed : hipp_get_regaddr");
		return -EINVAL;
	}

	writel(IPP_CLK_EN, crg_base + IPP_TOP_ENH_CRG_CFG0_REG);
	writel(IPP_CLK_EN, crg_base + IPP_TOP_ARF_CRG_CFG0_REG);
	writel(IPP_CLK_EN, crg_base + IPP_TOP_RDR_CRG_CFG0_REG);
	writel(IPP_CLK_EN, crg_base + IPP_TOP_CMP_CRG_CFG0_REG);
	writel(IPP_CLK_EN, crg_base + IPP_TOP_MC_CRG_CFG0_REG);
	logd("-");
	return ISP_IPP_OK;
}

int hipp_orcm_clk_disable(void)
{
	void __iomem *crg_base;
	logd("+");
	crg_base = hipp_get_regaddr(CPE_TOP);
	if (crg_base == NULL) {
		loge("Failed : hipp_get_regaddr");
		return -EINVAL;
	}

	writel(IPP_CLK_DISABLE, crg_base + IPP_TOP_ENH_CRG_CFG0_REG);
	writel(IPP_CLK_DISABLE, crg_base + IPP_TOP_ARF_CRG_CFG0_REG);
	writel(IPP_CLK_DISABLE, crg_base + IPP_TOP_RDR_CRG_CFG0_REG);
	writel(IPP_CLK_DISABLE, crg_base + IPP_TOP_CMP_CRG_CFG0_REG);
	writel(IPP_CLK_DISABLE, crg_base + IPP_TOP_MC_CRG_CFG0_REG);
	logd("-");
	return ISP_IPP_OK;
}

int hispcpe_gf_clk_enable(void)
{
	union u_gf_crg_cfg0 cfg;
	void __iomem *crg_base;
	logd("+");
	crg_base = hipp_get_regaddr(CPE_TOP);
	if (crg_base == NULL) {
		loge("Failed : hipp_get_regaddr");
		return -EINVAL;
	}

	cfg.u32 = readl(crg_base + IPP_TOP_GF_CRG_CFG0_REG);
	cfg.bits.gf_clken = 1;
	writel(cfg.u32, crg_base + IPP_TOP_GF_CRG_CFG0_REG);
	logd("-");
	return ISP_IPP_OK;
}

int hispcpe_gf_clk_disable(void)
{
	union u_gf_crg_cfg0 cfg;
	void __iomem *crg_base;
	logd("+");
	crg_base = hipp_get_regaddr(CPE_TOP);
	if (crg_base == NULL) {
		loge("Failed : hipp_get_regaddr");
		return -EINVAL;
	}

	cfg.u32 = readl(crg_base + IPP_TOP_GF_CRG_CFG0_REG);
	cfg.bits.gf_clken = 0;
	writel(cfg.u32, crg_base + IPP_TOP_GF_CRG_CFG0_REG);
	logd("-");
	return ISP_IPP_OK;
}

int hispcpe_hiof_clk_enable(void)
{
	union u_hiof_crg_cfg0 cfg;
	void __iomem *crg_base;
	logd("+");
	crg_base = hipp_get_regaddr(CPE_TOP);
	if (crg_base == NULL) {
		loge("Failed : hipp_get_regaddr");
		return -EINVAL;
	}

	cfg.u32 = readl(crg_base + IPP_TOP_HIOF_CRG_CFG0_REG);
	cfg.bits.hiof_clken = 1;
	writel(cfg.u32, (crg_base + IPP_TOP_HIOF_CRG_CFG0_REG));
	logd("-");
	return ISP_IPP_OK;
}

int hispcpe_hiof_clk_disable(void)
{
	union u_hiof_crg_cfg0 cfg;
	void __iomem *crg_base;
	logd("+");
	crg_base = hipp_get_regaddr(CPE_TOP);
	if (crg_base == NULL) {
		loge("Failed : hipp_get_regaddr");
		return -EINVAL;
	}

	cfg.u32 = readl(crg_base + IPP_TOP_HIOF_CRG_CFG0_REG);
	cfg.bits.hiof_clken = 0;
	writel(cfg.u32, (crg_base + IPP_TOP_HIOF_CRG_CFG0_REG));
	logd("-");
	return ISP_IPP_OK;
}

int hispcpe_orb_enh_request(struct hipp_adapter_s *dev,
							struct msg_enh_req_t *req_enh)
{
#ifdef CONFIG_IPP_DEBUG
	void __iomem *crg_base;
	int value = 0;
#endif
	int ret = 0,  ret_1 = 0;
	logd("+");

	if (dev == NULL || req_enh == NULL) {
		loge("Failed : dev.%pK, req_enh.%pK", dev, req_enh);
		return -EINVAL;
	}

	ret = hispcpe_clean_wait_flag(dev, WAIT_MODE_ENH);
	if (ret < 0) {
		loge("Failed : hispcpe_clean_wait_flag.%d", ret);
		return ret;
	}

	ret = orb_enh_request_handler(req_enh);
	if (ret != 0) {
		loge("Failed : orb_enh_request_handler.%d", ret);
		goto ORB_ENH_REQ_DONE;
	}

	ret = hispcpe_wait(dev, WAIT_MODE_ENH);
#ifdef CONFIG_IPP_DEBUG
	crg_base = hipp_get_regaddr(CPE_TOP);
	value = readl(crg_base + IPP_TOP_ENH_IRQ_REG3_REG);
	loge("enh_irq_state_raw = 0x%08x", value);
	value = readl(crg_base + IPP_TOP_ENH_IRQ_REG4_REG);
	loge("enh_irq_state_mask = 0x%08x", value);

	if (value == 0x2f) {
		loge("enh irq is coming!");
		ret = 0;
	}

#endif

	if (ret < 0) {
		loge("Failed : hispcpe_wait.%d", ret);
		ret = ISP_IPP_ERR;
	}

ORB_ENH_REQ_DONE:
	ret_1 = ipp_eop_handler(CMD_EOF_ORB_ENH_MODE);

	if (ret_1 != 0)
		loge("Failed : ipp_eop_handler.%d", ret);

	logd("-");

	if (ret_1 != 0 || ret != 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
}

#ifdef CONFIG_IPP_DEBUG
static int hispcpe_get_arfeature_irq_ret(struct msg_req_arfeature_request_t *ctrl)
{
	void __iomem *crg_base;
	int value = 0;
	int ret = 0;

	crg_base = hipp_get_regaddr(CPE_TOP);
	value = readl(crg_base + IPP_TOP_ARF_IRQ_REG3_REG);
	loge("arf_irq_state_raw = 0x%08x", value);
	value = readl(crg_base + IPP_TOP_ARF_IRQ_REG4_REG);
	loge("arf_irq_state_mask = 0x%08x", value);

	if (ctrl ->mode_cnt == 10 &&
		(value == 0x13e || value == 0x3fe || value == 0x3c2 || value == 0x2f06 || value == 0x3000 )) {
		loge("ar case: arf irq is coming!");
		ret = 0;
	} else if (ctrl ->mode_cnt == 3 &&
		(value == 0x12a || value == 0x3c2 || value == 0x2f06  || value == 0x3000 ) ) {
		loge("dmap case: arf irq is coming!");
		ret = 0;
	}

	return ret;
}
#endif

int hispcpe_arfeature_request(struct hipp_adapter_s *dev,
							  struct msg_req_arfeature_request_t *ctrl)
{
	int ret = 0,  ret_1 = 0;
	logd("+");

	if (dev == NULL || ctrl == NULL) {
		loge("Failed : dev.%pK, ctrl.%pK", dev, ctrl);
		return -EINVAL;
	}

	ret = hispcpe_clean_wait_flag(dev, WAIT_MODE_ARFEATURE);
	if (ret < 0) {
		loge("Failed : hispcpe_clean_wait_flag.%d", ret);
		return ret;
	}

	ret = arfeature_request_handler(ctrl);
	if (ret != 0) {
		loge("Failed : arfeature_request_handler.%d", ret);
		goto ARFEATURE_REQ_DONE;
	}

	ret = hispcpe_wait(dev, WAIT_MODE_ARFEATURE);
#ifdef CONFIG_IPP_DEBUG
	ret = hispcpe_get_arfeature_irq_ret(ctrl);
#endif
	if (ret < 0)
		loge("Failed : hispcpe_wait.%d", ret);

ARFEATURE_REQ_DONE:
	ret_1 = ipp_eop_handler(CMD_EOF_ARFEATURE_MODE);

	if (ret_1 != 0)
		loge("Failed : ipp_eop_handler.%d", ret);

	logd("-");

	if (ret_1 != 0 || ret != 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
}

static int hispcpe_reorder_request(struct hipp_adapter_s *dev,
								   struct msg_req_reorder_request_t *ctrl)
{
#ifdef CONFIG_IPP_DEBUG
	void __iomem *crg_base;
	int value = 0;
#endif

	int ret = 0,  ret_1 = 0;
	logd("+");

	if (dev == NULL || ctrl == NULL) {
		loge("Failed : dev.%pK, ctrl.%pK", dev, ctrl);
		return -EINVAL;
	}

	ret = hispcpe_clean_wait_flag(dev, WAIT_MODE_RDR);
	if (ret < 0) {
		loge("Failed : hispcpe_clean_wait_flag.%d", ret);
		return ret;
	}

	ret = reorder_request_handler(ctrl);
	if (ret != 0) {
		loge("Failed : reorder_request_handler.%d", ret);
		goto REORDER_REQ_DONE;
	}

	ret = hispcpe_wait(dev, WAIT_MODE_RDR);
#ifdef CONFIG_IPP_DEBUG
	crg_base = hipp_get_regaddr(CPE_TOP);
	value = readl(crg_base + IPP_TOP_RDR_IRQ_REG3_REG);
	loge("rdr_irq_state_raw = 0x%08x", value);
	value = readl(crg_base + IPP_TOP_RDR_IRQ_REG4_REG);
	loge("rdr_irq_state_mask = 0x%08x", value);

	if (value == 0xb) {
		loge("rdr irq is coming!");
		ret = 0;
	}
#endif

	if (ret < 0)
		loge("Failed : hispcpe_wait.%d", ret);

REORDER_REQ_DONE:
	ret_1 = ipp_eop_handler(CMD_EOF_RDR_MODE);

	if (ret_1 != 0)
		loge("Failed : ipp_eop_handler.%d", ret);

	logd("-");

	if (ret_1 != 0 || ret != 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
}

static int hispcpe_compare_request(struct hipp_adapter_s *dev,
								   struct msg_req_compare_request_t *ctrl)
{
#ifdef CONFIG_IPP_DEBUG
	void __iomem *crg_base;
	int value = 0;
#endif

	int ret = 0,  ret_1 = 0;
	logd("+");

	if (dev == NULL || ctrl == NULL) {
		loge("Failed : dev.%pK, ctrl.%pK", dev, ctrl);
		return -EINVAL;
	}

	ret = hispcpe_clean_wait_flag(dev, WAIT_MODE_CMP);
	if (ret < 0) {
		loge("Failed : hispcpe_clean_wait_flag.%d", ret);
		return ret;
	}

	ret = compare_request_handler(ctrl);
	if (ret != 0) {
		loge("Failed : compare_request_handler.%d", ret);
		goto COMPARE_REQ_DONE;
	}

	ret = hispcpe_wait(dev, WAIT_MODE_CMP);
#ifdef CONFIG_IPP_DEBUG
	crg_base = hipp_get_regaddr(CPE_TOP);
	value = readl(crg_base + IPP_TOP_CMP_IRQ_REG3_REG);
	loge("cmp_irq_state_raw = 0x%08x", value);
	value = readl(crg_base + IPP_TOP_CMP_IRQ_REG4_REG);
	loge("cmp_irq_state_mask = 0x%08x", value);

	if (value == 0xb || value == 0xc) {
		loge("cmp irq is coming!");
		ret = 0;
	}
#endif

	if (ret < 0)
		loge("Failed : hispcpe_wait.%d", ret);

COMPARE_REQ_DONE:
	ret_1 = ipp_eop_handler(CMD_EOF_CMP_MODE);

	if (ret_1 != 0)
		loge("Failed : ipp_eop_handler.%d", ret);

	logd("-");

	if (ret_1 != 0 || ret != 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
}

int hispcpe_mc_request(struct hipp_adapter_s *dev,
					   struct msg_req_mc_request_t *ctrl)
{
#ifdef CONFIG_IPP_DEBUG
	void __iomem *crg_base;
	int value = 0;
#endif
	int ret = 0,  ret_1 = 0;
	logd("+");

	if (dev == NULL || ctrl == NULL) {
		loge("Failed : dev.%pK, ctrl.%pK", dev, ctrl);
		return -EINVAL;
	}

	ret = hispcpe_clean_wait_flag(dev, WAIT_MODE_MC);
	if (ret < 0) {
		loge("Failed : hispcpe_clean_wait_flag.%d", ret);
		return ret;
	}

	ret = mc_request_handler(ctrl);
	if (ret != 0) {
		loge("Failed : compare_request_handler.%d", ret);
		goto MC_REQ_DONE;
	}

	ret = hispcpe_wait(dev, WAIT_MODE_MC);
#ifdef CONFIG_IPP_DEBUG
	crg_base = hipp_get_regaddr(CPE_TOP);
	value = readl(crg_base + IPP_TOP_MC_IRQ_REG3_REG);
	loge("mc_irq_state_raw = 0x%08x", value);
	value = readl(crg_base + IPP_TOP_MC_IRQ_REG4_REG);
	loge("mc_irq_state_mask = 0x%08x", value);

	if (value == 0x6 || value == 0x5) {
		loge("mc irq is coming!");
		ret = 0;
	}

#endif

	if (ret < 0)
		loge("Failed : hispcpe_wait.%d", ret);

MC_REQ_DONE:
	ret_1 = ipp_eop_handler(CMD_EOF_MC_MODE);

	if (ret_1 != 0)
		loge("Failed : ipp_eop_handler.%d", ret);

	logd("-");

	if (ret_1 != 0 || ret != 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
}

int hispcpe_matcher_request(struct hipp_adapter_s *dev,
							struct msg_req_matcher_t *ctrl)
{
#ifdef CONFIG_IPP_DEBUG
	void __iomem *crg_base;
	int value = 0;
#endif
	int ret = 0,  ret_1 = 0;
	dataflow_check_para(dev);
	dataflow_check_para(ctrl);
	ret = hispcpe_clean_wait_flag(dev, WAIT_MODE_RDR);
	if (ret < 0) {
		loge("Failed : hispcpe_clean_wait_flag--reorder.%d", ret);
		return ret;
	}

	ret = hispcpe_clean_wait_flag(dev, WAIT_MODE_CMP);
	if (ret < 0) {
		loge("Failed : hispcpe_clean_wait_flag--compare.%d", ret);
		return ret;
	}

	ret = matcher_request_handler(ctrl);
	if (ret != 0) {
		loge("Failed : matcher_request_handler.%d", ret);
		goto MATCHER_REQ_DONE;
	}

	ret = hispcpe_wait(dev, WAIT_MODE_RDR);
	ret = 0; // debug

	if (ret < 0) {
		loge("Failed : hispcpe_wait reorder.%d", ret);
		goto MATCHER_REQ_DONE;
	}

	ret = hispcpe_wait(dev, WAIT_MODE_CMP);
#ifdef CONFIG_IPP_DEBUG
	crg_base = hipp_get_regaddr(CPE_TOP);
	value = readl(crg_base + IPP_TOP_RDR_IRQ_REG3_REG);
	loge("rdr_irq_state_raw = 0x%08x", value);
	value = readl(crg_base + IPP_TOP_RDR_IRQ_REG4_REG);
	loge("rdr_irq_state_mask = 0x%08x", value);
	value = readl(crg_base + IPP_TOP_CMP_IRQ_REG3_REG);
	loge("cmp_irq_state_raw = 0x%08x", value);
	value = readl(crg_base + IPP_TOP_CMP_IRQ_REG4_REG);
	loge("cmp_irq_state_mask = 0x%08x", value);

	if (value == 0xb || value == 0xc) {
		loge("cmp irq is coming!");
		ret = 0;
	}

#endif

	if (ret < 0)
		loge("Failed : hispcpe_wait compare.%d", ret);

MATCHER_REQ_DONE:
	ret_1 = matcher_eof_handler(ctrl);

	if (ret_1 != 0)
		loge("Failed : matcher_eof_handler.%d", ret);

	if (ret_1 != 0 || ret != 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
}

static int orb_enh_process_internal(struct hipp_adapter_s *dev,
									struct msg_enh_req_t *ctrl_orb_enh)
{
	unsigned int ret;
	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv, dev->curr_cpe_rate_value[CLK_RATE_INDEX_ARFEATURE]);
	if (ret != 0) {
		loge("Failed : ipp_set_rate.%d", ret);
		ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv, CLK_RATE_SVS);
		if (ret != 0) {
			loge("Failed to set ipp  rate mode: %d", CLK_RATE_SVS);
			return ret;
		}
	}

	ret = hipp_orcm_clk_enable();
	if (ret != 0) {
		loge("Failed : hipp_orcm_clk_enable.%d", ret);
		return ret;
	}

	ret = hispcpe_orb_enh_request(dev, ctrl_orb_enh);
	if (ret != 0) {
		loge("Failed : hispcpe_orb_enh_request.%d", ret);

		if (hipp_orcm_clk_disable() != 0)
			loge("Failed : hipp_orcm_clk_disable");

		return ret;
	}

	ret = hipp_orcm_clk_disable();
	if (ret != 0) {
		loge("Failed : hipp_orcm_clk_disable.%d", ret);
		return ret;
	}

	dev->curr_cpe_rate_value[CLK_RATE_INDEX_ARFEATURE] = CLK_RATE_SVS;
	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
									   dev->curr_cpe_rate_value[CLK_RATE_INDEX_ARFEATURE]);
	if (ret != 0) {
		loge("Failed : ipp_set_rate.%d", ret);
		return ret;
	}

	return ret;
}

int orb_enh_process(unsigned long args)
{
	unsigned int ret = 0;
	unsigned int enh_rate_index = 0;
	struct msg_enh_req_t *ctrl_orb_enh = NULL;
	void __user *args_orb_enh = (void __user *)(uintptr_t) args;
	struct hipp_adapter_s *dev = g_hipp_adapter;
	loge_if_null(args_orb_enh);
	ctrl_orb_enh = (struct msg_enh_req_t *)vmalloc(sizeof(struct msg_enh_req_t));
	if (ctrl_orb_enh == NULL) {
		loge("Failed: fail to vmalloc ctrl_orb_enh");
		return -EINVAL;
	}

	if (memset_s(ctrl_orb_enh, sizeof(struct msg_enh_req_t), 0, sizeof(struct msg_enh_req_t))) {
		ret = ISP_IPP_ERR;
		loge("Failed : fail to memset_s ctrl_orb_enh");
		goto free_orb_enh_kmalloc_memory;
	}

	ret = copy_from_user(ctrl_orb_enh, args_orb_enh, sizeof(struct msg_enh_req_t));
	if (ret != 0) {
		loge("Failed : copy_from_user.%d", ret);
		goto free_orb_enh_kmalloc_memory;
	}

	enh_rate_index = ctrl_orb_enh->orb_enh_rate_value;
	dev->curr_cpe_rate_value[CLK_RATE_INDEX_ARFEATURE] = enh_rate_index > CLK_RATE_SVS ? 0 : enh_rate_index;
	logd("dev->curr_cpe_rate_value[CLK_RATE_INDEX_ARFEATURE] = %d", dev->curr_cpe_rate_value[CLK_RATE_INDEX_ARFEATURE]);
	ret = orb_enh_process_internal(dev, ctrl_orb_enh);
	if (ret != 0) {
		loge("Failed : orb_enh_process_internal.%d", ret);
		goto free_orb_enh_kmalloc_memory;
	}

free_orb_enh_kmalloc_memory:
	vfree(ctrl_orb_enh);
	return ret;
}

static int arfeature_process_internal(struct hipp_adapter_s *dev,
									  struct msg_req_arfeature_request_t *ctrl_arf)
{
	unsigned int ret;
	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
									   dev->curr_cpe_rate_value[CLK_RATE_INDEX_ARFEATURE]);
	if (ret != 0) {
		loge("Failed : ipp_set_rate.%d", ret);
		ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv, CLK_RATE_SVS);
		if (ret != 0) {
			loge("Failed : to set ipp  rate mode: %d", CLK_RATE_SVS);
			return ret;
		}
	}

	ret = hipp_orcm_clk_enable();
	if (ret != 0) {
		loge("Failed : hipp_orcm_clk_enable.%d", ret);
		return ret;
	}

	ret = hispcpe_arfeature_request(dev, ctrl_arf);
	if (ret != 0) {
		loge("Failed : hispcpe_arfeature_request.%d", ret);

		if (hipp_orcm_clk_disable() != 0)
			loge("Failed : hipp_orcm_clk_disable");

		return ret;
	}

	ret = hipp_orcm_clk_disable();
	if (ret != 0) {
		loge("Failed : hipp_orcm_clk_disable.%d", ret);
		return ret;
	}

	dev->curr_cpe_rate_value[CLK_RATE_INDEX_ARFEATURE] = CLK_RATE_SVS;
	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
									   dev->curr_cpe_rate_value[CLK_RATE_INDEX_ARFEATURE]);
	if (ret != 0) {
		loge("Failed : ipp_set_rate.%d", ret);
		return ret;
	}

	return ret;
}


int arfeature_process(unsigned long args)
{
	unsigned int ret = 0;
	unsigned int arf_rate_index = 0;
	struct msg_req_arfeature_request_t *ctrl_arf = NULL;
	void __user *args_arf = (void __user *)(uintptr_t) args;
	struct hipp_adapter_s *dev = g_hipp_adapter;

	if (args_arf == NULL) {
		loge("Failed : args_arf NULL.%pK", args_arf);
		return -EINVAL;
	}

	ctrl_arf = (struct msg_req_arfeature_request_t *)vmalloc(sizeof(struct msg_req_arfeature_request_t));
	if (ctrl_arf == NULL) {
		loge("Failed: fail to vmalloc ctrl_arf");
		return -EINVAL;
	}

	if (memset_s(ctrl_arf, sizeof(struct msg_req_arfeature_request_t),
				 0, sizeof(struct msg_req_arfeature_request_t))) {
		loge("Failed : fail to memset_s ctrl_arf");
		ret = ISP_IPP_ERR;
		goto free_arf_vmalloc_memory;
	}

	ret = copy_from_user(ctrl_arf, args_arf,
						 sizeof(struct msg_req_arfeature_request_t));
	if (ret != 0) {
		loge("Failed : copy_from_user.%d", ret);
		goto free_arf_vmalloc_memory;
	}

	arf_rate_index = ctrl_arf->arfeature_rate_value;
	dev->curr_cpe_rate_value[CLK_RATE_INDEX_ARFEATURE] = arf_rate_index > CLK_RATE_SVS ? 0 : arf_rate_index;
	logd("dev->curr_cpe_rate_value[CLK_RATE_INDEX_ARFEATURE] = %d", dev->curr_cpe_rate_value[CLK_RATE_INDEX_ARFEATURE]);
	ret = arfeature_process_internal(dev, ctrl_arf);
	if (ret != 0) {
		loge("Failed : compare_process_internal.%d", ret);
		goto free_arf_vmalloc_memory;
	}

free_arf_vmalloc_memory:
	vfree(ctrl_arf);
	return ret;
}

static int reorder_process_internal(struct hipp_adapter_s *dev,
									struct msg_req_reorder_request_t *ctrl_reorder)
{
	unsigned int ret;
	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
									   dev->curr_cpe_rate_value[CLK_RATE_INDEX_MATCHER]);
	if (ret != 0) {
		loge("Failed : ipp_set_rate.%d", ret);
		ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv, CLK_RATE_SVS);
		if (ret != 0) {
			loge("Failed to set ipp  rate mode: %d", CLK_RATE_SVS);
			return ret;
		}
	}

	ret = hipp_orcm_clk_enable();
	if (ret != 0) {
		loge("Failed : hipp_orcm_clk_enable.%d", ret);
		return ret;
	}

	ret = hispcpe_reorder_request(dev, ctrl_reorder);
	if (ret != 0) {
		loge("Failed : hispcpe_reorder_request.%d", ret);

		if (hipp_orcm_clk_disable() != 0)
			loge("Failed : hipp_orcm_clk_disable");

		return ret;
	}

	ret = hipp_orcm_clk_disable();
	if (ret != 0) {
		loge("Failed : hipp_orcm_clk_disable.%d", ret);
		return ret;
	}

	dev->curr_cpe_rate_value[CLK_RATE_INDEX_MATCHER] = CLK_RATE_SVS;
	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
									   dev->curr_cpe_rate_value[CLK_RATE_INDEX_MATCHER]);
	if (ret != 0) {
		loge("Failed : ipp_set_rate.%d", ret);
		return ret;
	}

	return ret;
}

int reorder_process(unsigned long args)
{
	unsigned int ret = 0;
	unsigned int rdr_rate_index = 0;
	struct msg_req_reorder_request_t *ctrl_reorder = NULL;
	void __user *args_rdr = (void __user *)(uintptr_t)args;
	struct hipp_adapter_s *dev = g_hipp_adapter;
	loge_if_null(args_rdr);
	ctrl_reorder = (struct msg_req_reorder_request_t *)vmalloc(sizeof(struct msg_req_reorder_request_t));
	if (ctrl_reorder == NULL) {
		loge("Failed : ctrl_reorder is NULL");
		return -EINVAL;
	}

	if (memset_s(ctrl_reorder, sizeof(struct msg_req_reorder_request_t),
				 0, sizeof(struct msg_req_reorder_request_t))) {
		loge("Failed : fail to memset_s ctrl_reorder");
		ret = ISP_IPP_ERR;
		goto free_reorder_vmalloc_memory;
	}

	ret = copy_from_user(ctrl_reorder, args_rdr,
						 sizeof(struct msg_req_reorder_request_t));
	if (ret != 0) {
		loge("Failed : copy_from_user.%d", ret);
		goto free_reorder_vmalloc_memory;
	}

	rdr_rate_index = ctrl_reorder->rdr_rate_value;
	dev->curr_cpe_rate_value[CLK_RATE_INDEX_MATCHER] = rdr_rate_index > CLK_RATE_SVS ? 0 : rdr_rate_index;
	logd("dev->curr_cpe_rate_value[CLK_RATE_INDEX_MATCHER] = %d", dev->curr_cpe_rate_value[CLK_RATE_INDEX_MATCHER]);
	ret = reorder_process_internal(dev, ctrl_reorder);
	if (ret != 0) {
		loge("Failed : reorder_process_internal.%d", ret);
		goto free_reorder_vmalloc_memory;
	}

free_reorder_vmalloc_memory:
	vfree(ctrl_reorder);
	return ret;
}

static int compare_process_internal(struct hipp_adapter_s *dev,
									struct msg_req_compare_request_t *ctrl_compare)
{
	unsigned int ret;
	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
									   dev->curr_cpe_rate_value[CLK_RATE_INDEX_MATCHER]);
	if (ret != 0) {
		loge("Failed : ipp_set_rate.%d", ret);
		ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv, CLK_RATE_SVS);
		if (ret != 0) {
			loge("Failed to set ipp  rate mode: %d", CLK_RATE_SVS);
			return ret;
		}
	}

	ret = hipp_orcm_clk_enable();
	if (ret != 0) {
		loge("Failed : hipp_orcm_clk_enable.%d", ret);
		return ret;
	}

	ret = hispcpe_compare_request(dev, ctrl_compare);
	if (ret != 0) {
		loge("Failed : hispcpe_compare_request.%d", ret);

		if (hipp_orcm_clk_disable() != 0)
			loge("Failed : hipp_orcm_clk_disable");

		return ret;
	}

	ret = hipp_orcm_clk_disable();
	if (ret != 0) {
		loge("Failed : hipp_orcm_clk_disable.%d", ret);
		return ret;
	}

	dev->curr_cpe_rate_value[CLK_RATE_INDEX_MATCHER] = CLK_RATE_SVS;
	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
									   dev->curr_cpe_rate_value[CLK_RATE_INDEX_MATCHER]);
	if (ret != 0) {
		loge("Failed : ipp_set_rate.%d", ret);
		return ret;
	}

	return ret;
}

int compare_process(unsigned long args)
{
	unsigned int ret = 0;
	unsigned int cmp_rate_index = 0;
	struct msg_req_compare_request_t *ctrl_compare = NULL;
	void __user *args_cmp = (void __user *)(uintptr_t) args;
	struct hipp_adapter_s *dev = g_hipp_adapter;
	loge_if_null(args_cmp);
	ctrl_compare = (struct msg_req_compare_request_t *)vmalloc(sizeof(struct msg_req_compare_request_t));
	if (ctrl_compare == NULL) {
		loge("Failed : vmalloc ctrl_compare");
		return -EINVAL;
	}

	if (memset_s(ctrl_compare, sizeof(struct msg_req_compare_request_t),
				 0, sizeof(struct msg_req_compare_request_t))) {
		loge("Failed : fail to memset_s ctrl_compare");
		ret = ISP_IPP_ERR;
		goto free_compare_vmalloc_memory;
	}

	ret = copy_from_user(ctrl_compare, args_cmp,
						 sizeof(struct msg_req_compare_request_t));
	if (ret != 0) {
		loge("Failed : copy_from_user.%d", ret);
		goto free_compare_vmalloc_memory;
	}

	cmp_rate_index = ctrl_compare->cmp_rate_value;
	dev->curr_cpe_rate_value[CLK_RATE_INDEX_MATCHER] = cmp_rate_index > CLK_RATE_SVS ? 0 : cmp_rate_index;
	logd("dev->curr_cpe_rate_value[CLK_RATE_INDEX_MATCHER] = %d", dev->curr_cpe_rate_value[CLK_RATE_INDEX_MATCHER]);
	ret = compare_process_internal(dev, ctrl_compare);
	if (ret != 0) {
		loge("Failed : compare_process_internal.%d", ret);
		goto free_compare_vmalloc_memory;
	}

free_compare_vmalloc_memory:
	vfree(ctrl_compare);
	return ret;
}

static int mc_process_internal(struct hipp_adapter_s *dev, struct msg_req_mc_request_t *ctrl_mc)
{
	unsigned int ret;
	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv, dev->curr_cpe_rate_value[CLK_RATE_INDEX_MC]);
	if (ret != 0) {
		loge("Failed : ipp_set_rate.%d", ret);
		ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv, CLK_RATE_SVS);
		if (ret != 0) {
			loge("Failed to set ipp  rate mode: %d", CLK_RATE_SVS);
			return ret;
		}
	}

	ret = hipp_orcm_clk_enable();
	if (ret != 0) {
		loge("Failed : hipp_orcm_clk_enable.%d", ret);
		return ret;
	}

	ret = hispcpe_mc_request(dev, ctrl_mc);
	if (ret != 0) {
		loge("Failed : hispcpe_mc_request.%d", ret);

		if (hipp_orcm_clk_disable() != 0)
			loge("Failed : hipp_orcm_clk_disable");

		return ret;
	}

	ret = hipp_orcm_clk_disable();
	if (ret != 0) {
		loge("Failed : hipp_orcm_clk_disable.%d", ret);
		return ret;
	}

	dev->curr_cpe_rate_value[CLK_RATE_INDEX_MC] = CLK_RATE_SVS;
	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
									   dev->curr_cpe_rate_value[CLK_RATE_INDEX_MC]);
	if (ret != 0) {
		loge("Failed : ipp_set_rate.%d", ret);
		return ret;
	}

	return ret;
}

int mc_process(unsigned long args)
{
	unsigned int ret = 0;
	unsigned int mc_rate_index = 0;
	struct msg_req_mc_request_t *ctrl_mc = NULL;
	void __user *args_mc = (void __user *)(uintptr_t) args;
	struct hipp_adapter_s *dev = g_hipp_adapter;
	loge_if_null(args_mc);
	ctrl_mc = (struct msg_req_mc_request_t *)vmalloc(sizeof(struct msg_req_mc_request_t));
	if (ctrl_mc == NULL) {
		loge("Failed : vmalloc ctrl_mc");
		return -EINVAL;
	}

	if (memset_s(ctrl_mc, sizeof(struct msg_req_mc_request_t), 0, sizeof(struct msg_req_mc_request_t))) {
		loge("Failed : fail to memset_s ctrl_mc");
		ret = ISP_IPP_ERR;
		goto free_mc_vmalloc_memory;
	}

	ret = copy_from_user(ctrl_mc, args_mc, sizeof(struct msg_req_mc_request_t));
	if (ret != 0) {
		loge("Failed : copy_from_user.%d", ret);
		goto free_mc_vmalloc_memory;
	}

	mc_rate_index = ctrl_mc->mc_rate_value;
	dev->curr_cpe_rate_value[CLK_RATE_INDEX_MC] = mc_rate_index > CLK_RATE_SVS ? 0 : mc_rate_index;
	logd("dev->curr_cpe_rate_value[CLK_RATE_INDEX_MC] = %d", dev->curr_cpe_rate_value[CLK_RATE_INDEX_MC]);
	ret = mc_process_internal(dev, ctrl_mc);
	if (ret != 0) {
		loge("Failed : mc_process_internal.%d", ret);
		goto free_mc_vmalloc_memory;
	}

free_mc_vmalloc_memory:
	vfree(ctrl_mc);
	return ret;
}
// -------------End-------------

