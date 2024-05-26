/******************************************************************
 * Copyright    Copyright (c) 2020- Hisilicon Technologies CO., Ltd.
 * File name    segment_adapter.c
 * Description:
 *
 * Date         2020-04-17 16:28:10
 ********************************************************************/
#include <linux/module.h>
#include <linux/pci.h>
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
#include "segment_ipp_adapter_sub.h"

// debug
#include "cmdlst_reg_offset.h"
#include "cvdr_ipp_reg_offset.h"

#define IPPCORE_SWID_INDEX0 0
#define IPPCORE_SWID_LEN0 34
#define IPPCORE_SWID_INDEX1 42
#define IPPCORE_SWID_LEN1 11

#ifdef CONFIG_IPP_DEBUG
#define HISP_CPE_TIMEOUT_MS (5000)
#else
#define HISP_CPE_TIMEOUT_MS 200
#endif

struct memory_block_s {
	int shared_fd;
	int size;
	unsigned long prot;
	unsigned int da;
	int usage;
	void *viraddr;
};

struct map_buff_block_s {
	int32_t     secure;
	int32_t     shared_fd;
	int32_t     size;
	u_int64_t prot;
};

struct hipp_adapter_s *g_hipp_adapter;

int hispcpe_wait(struct hipp_adapter_s *dev,
	enum hipp_wait_mode_type_e wait_mode)
{
	long tmp_ret = 0;
	int times = 0;
	unsigned long timeout;
	logi("+");
	dataflow_check_para(dev);
	timeout = (unsigned long)msecs_to_jiffies(HISP_CPE_TIMEOUT_MS);

	while (1) {
		if (wait_mode == WAIT_MODE_GF)
			tmp_ret = wait_event_interruptible_timeout(dev->gf_wait, dev->gf_ready, timeout);
		else if (wait_mode == WAIT_MODE_ARFEATURE)
			tmp_ret = wait_event_interruptible_timeout(dev->arfeature_wait, dev->arfeature_ready, timeout);
		else if (wait_mode == WAIT_MODE_RDR)
			tmp_ret = wait_event_interruptible_timeout(dev->reorder_wait, dev->reorder_ready, timeout);
		else if (wait_mode == WAIT_MODE_CMP)
			tmp_ret = wait_event_interruptible_timeout(dev->compare_wait, dev->compare_ready, timeout);
		else if (wait_mode == WAIT_MODE_HIOF)
			tmp_ret = wait_event_interruptible_timeout(dev->hiof_wait, dev->hiof_ready, timeout);
		else if (wait_mode == WAIT_MODE_ENH)
			tmp_ret = wait_event_interruptible_timeout(dev->orb_enh_wait, dev->orb_enh_ready, timeout);
		else if (wait_mode == WAIT_MODE_MC)
			tmp_ret = wait_event_interruptible_timeout(dev->mc_wait, dev->mc_ready, timeout);

		if ((tmp_ret == -ERESTARTSYS) && (times++ < 200))
			msleep(5);
		else
			break;
	}

	if (tmp_ret <= 0) {
		loge("Failed :mode.%d, ret.%ld, times.%d", wait_mode, tmp_ret, times);
		return -EINVAL;
	}

	logi("wait_mode.%d, ret.%ld", wait_mode, tmp_ret);
	logi("-");
	return ISP_IPP_OK;
}

irqreturn_t hispcpe_irq_handler(int irq, void *devid)
{
	struct hipp_adapter_s *dev = NULL;
	unsigned int value = 0;
	void __iomem *crg_base;
	dev = (struct hipp_adapter_s *)devid;

	if (dev != g_hipp_adapter) {
		loge("Failed : dev.%pK", dev);
		return IRQ_NONE;
	}

	crg_base = hipp_get_regaddr(CPE_TOP);

	if (irq == dev->irq) {
		value = readl(crg_base + IPP_TOP_CMDLST_IRQ_STATE_RAW_0_REG);
		value = readl(crg_base + IPP_TOP_CMDLST_IRQ_STATE_RAW_0_REG);
		value &= 0xFFFF;
		logi("IRQ VALUE = 0x%08x", value);
		writel(value, crg_base + IPP_TOP_CMDLST_IRQ_CLR_0_REG);

		if (value & (0x1 << IPP_GF_CHANNEL_ID)) {
			dev->gf_ready = 1;
			wake_up_interruptible(&dev->gf_wait);
		}

		if (value & (0x1 << IPP_HIOF_CHANNEL_ID)) {
			dev->hiof_ready = 1;
			wake_up_interruptible(&dev->hiof_wait);
		}

		if (value & (0x1 << IPP_ARFEATURE_CHANNEL_ID)) {
			dev->arfeature_ready = 1;
			wake_up_interruptible(&dev->arfeature_wait);
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

	return IRQ_HANDLED;
}

int hipp_adapter_register_irq(int irq)
{
	int ret;
	struct hipp_adapter_s *dev = g_hipp_adapter;

	if (dev == NULL) {
		loge("Failed : dev NULL");
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
		loge("Failed: NONE cpe_mem_info!");
		return ISP_IPP_OK;
	}

	return dev->daddr;
}

void *get_cpe_addr_va(void)
{
	struct hipp_adapter_s *dev = g_hipp_adapter;

	if (dev == NULL) {
		loge("Failed: NONE cpe_mem_info!");
		return ISP_IPP_OK;
	}

	return dev->virt_addr;
}

int dataflow_cvdr_global_config(void)
{
	unsigned int reg_val;
	void __iomem *crg_base;
	crg_base = hipp_get_regaddr(CVDR_REG);
	if (crg_base == NULL) {
		loge("Failed : hipp_get_regaddr");
		return -EINVAL;
	}

	reg_val = (1 << 0) // axiwrite_du_threshold = 0
			  | (34 << 8) // du_threshold_reached = 34
			  | (63 << 16) // max_axiread_id = 19
			  | (31 << 24); // max_axiwrite_id = 11
	writel(reg_val, crg_base + 0x0);
	return ISP_IPP_OK;
}

static int hispcpe_irq_config(void)
{
	unsigned int token_nbr_enable = 0; // token function not enabled
	unsigned int nrt_channel = 1; // non RT channel
	unsigned int ipp_irq_mask = 0x1DF; // except ANF channel
	unsigned int cmdlst_channel_value =
		(token_nbr_enable << 7) | (nrt_channel << 8);
	loge_if_ret(hispcpe_reg_set(CPE_TOP, IPP_TOP_ENH_VPB_CFG_REG, 0x1)); // bit1:to arfeature bit0:to CVDR
	loge_if_ret(hispcpe_reg_set(CPE_TOP, IPP_TOP_CMDLST_IRQ_CLR_0_REG,
								(0xFFFF & ipp_irq_mask)));
	loge_if_ret(hispcpe_reg_set(CPE_TOP, IPP_TOP_CMDLST_IRQ_MSK_0_REG,
								(0xFFFF & ~ipp_irq_mask)));
	loge_if_ret(hispcpe_reg_set(CMDLIST_REG, 0x80 + IPP_GF_CHANNEL_ID * 0x80,
								cmdlst_channel_value));
	loge_if_ret(hispcpe_reg_set(CMDLIST_REG, 0x80 + IPP_HIOF_CHANNEL_ID * 0x80,
								cmdlst_channel_value));
	loge_if_ret(hispcpe_reg_set(CMDLIST_REG, 0x80 + IPP_ARFEATURE_CHANNEL_ID * 0x80,
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
	if (crg_base == NULL) {
		loge("Failed : hipp_get_regaddr");
		return -EINVAL;
	}

	writel(0x00000000, crg_base + IPP_TOP_JPG_FLUX_CTRL0_0_REG);
	writel(0x08000105, crg_base + IPP_TOP_JPG_FLUX_CTRL0_1_REG);
	writel(0x00000000, crg_base + IPP_TOP_JPG_FLUX_CTRL1_0_REG);
	writel(0x08000085, crg_base + IPP_TOP_JPG_FLUX_CTRL1_1_REG);
	ipp_qos_value = readl(crg_base + IPP_TOP_JPG_FLUX_CTRL0_0_REG);
	logi("JPG_FLUX_CTRL0_0 = 0x%x (0x0 wanted)", ipp_qos_value);
	ipp_qos_value = readl(crg_base + IPP_TOP_JPG_FLUX_CTRL0_1_REG);
	logi("JPG_FLUX_CTRL0_1 = 0x%x (0x08000105 wanted)", ipp_qos_value);
	ipp_qos_value = readl(crg_base + IPP_TOP_JPG_FLUX_CTRL1_0_REG);
	logi("JPG_FLUX_CTRL1_0 = 0x%x (0x0 wanted)", ipp_qos_value);
	ipp_qos_value = readl(crg_base + IPP_TOP_JPG_FLUX_CTRL1_1_REG);
	logi("JPG_FLUX_CTRL1_1 = 0x%x (0x08000085 wanted)", ipp_qos_value);
	return ISP_IPP_OK;
}

static int hipp_adapter_cfg_qos_cvdr(void)
{
	int ret;
	ret = dataflow_cvdr_global_config();
	if (ret < 0) {
		loge("Failed: dataflow_cvdr_global_config.%d", ret);
		return ret;
	}

	ret = ipp_cfg_qos_reg();
	if (ret < 0) {
		loge("Failed : ipp_cfg_qos_reg");
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

static int para_check_hipp_powerup(struct hipp_adapter_s *dev)
{
	if (dev == NULL) {
		loge("Failed : dev NULL");
		return -EINVAL;
	}

	if (dev->ippdrv == NULL) {
		loge("Failed : dev->ippdrv NULL");
		return -EINVAL;
	}

	if (dev->ippdrv->power_up == NULL) {
		loge("Failed : dev->ippdrv->power_up.NULL");
		return -EINVAL;
	}

	if (dev->ippdrv->enable_smmu == NULL) {
		loge("Failed : ippdrv->enable_smmu.NULL");
		return -EINVAL;
	}

	if (dev->ippdrv->disable_smmu == NULL) {
		loge("Failed : ippdrv->disable_smmu.NULL");
		return -EINVAL;
	}

	if (dev->ippdrv->setsid_smmu == NULL) {
		loge("Failed : ippdrv->setsid_smmu.NULL");
		return -EINVAL;
	}

	return ISP_IPP_OK;
}

static int hipp_setsid_smmu(struct hipp_adapter_s *dev, int sid, int ssid)
{
	int ret = 0;
	ret = dev->ippdrv->setsid_smmu(dev->ippdrv, IPPCORE_SWID_INDEX0,
					IPPCORE_SWID_LEN0, sid, ssid);
	if (ret != 0) {
		loge("Failed : dev->ippdrv->enable_smmu.%d", ret);
		return ret;
	}

	ret = dev->ippdrv->setsid_smmu(dev->ippdrv, IPPCORE_SWID_INDEX1,
								   IPPCORE_SWID_LEN1, sid, ssid);
	if (ret != 0) {
		loge("Failed : dev->ippdrv->enable_smmu.%d", ret);
		return ret;
	}

	return ISP_IPP_OK;
}

static int hipp_set_pref(struct hipp_adapter_s *dev)
{
	int ret = 0;
	ret = dev->ippdrv->set_pref(dev->ippdrv,
								IPPCORE_SWID_INDEX0, IPPCORE_SWID_LEN0);
	if (ret != 0) {
		loge("Failed : dev->ippdrv->enable_smmu.%d", ret);
		return ret;
	}

	ret = dev->ippdrv->set_pref(dev->ippdrv,
								IPPCORE_SWID_INDEX1, IPPCORE_SWID_LEN1);
	if (ret != 0) {
		loge("Failed : dev->ippdrv->enable_smmu.%d", ret);
		return ret;
	}

	return ISP_IPP_OK;
}

int ippdev_lock(void)
{
	struct hipp_adapter_s *dev = g_hipp_adapter;

	if (dev == NULL) {
		pr_err("[%s], NONE cpe info!\n", __func__);
		return ISP_IPP_ERR;
	}

	mutex_lock(&dev->cvdr_lock);

	return ISP_IPP_OK;
}

int ippdev_unlock(void)
{
	struct hipp_adapter_s *dev = g_hipp_adapter;

	if (dev == NULL) {
		pr_err("[%s], NONE cpe info!\n", __func__);
		return ISP_IPP_ERR;
	}

	mutex_unlock(&dev->cvdr_lock);
	return ISP_IPP_OK;
}

int hipp_powerup(void)
{
	int ret;
	struct hipp_adapter_s *dev = g_hipp_adapter;
	int sid;
	int ssid;
	loge_if_ret(para_check_hipp_powerup(dev));

	ret = dev->ippdrv->power_up(dev->ippdrv);
	if (ret != 0) {
		loge("Failed : dev->ippdrv->power_up.%d", ret);
		return ret;
	}

	ret = dev->ippdrv->enable_smmu(dev->ippdrv);
	if (ret != 0) {
		loge("Failed : dev->ippdrv->enable_smmu.%d", ret);
		goto fail_power_down;
	}

	ret = get_hipp_smmu_info(&sid, &ssid);
	if (ret < 0) {
		loge("Failed : get_hipp_smmu_info.%d", ret);
		goto fail_disable_smmu;
	}

	ret = hipp_setsid_smmu(dev, sid, ssid);
	if (ret != 0) goto fail_disable_smmu;

	ret = hipp_set_pref(dev);
	if (ret != 0) goto fail_disable_smmu;

	ret = hispcpe_irq_config();
	if (ret != 0) {
		loge("Failed : set irq config.%d", ret);
		goto fail_disable_smmu;
	}

	return ISP_IPP_OK;

fail_disable_smmu:
	ret = dev->ippdrv->disable_smmu(dev->ippdrv);

	if (ret != 0) loge("Failed : disable_smmu.%d", ret);

fail_power_down:
	ret = dev->ippdrv->power_dn(dev->ippdrv);

	if (ret != 0) loge("Failed : power_dn.%d", ret);

	return ret;
}

int hispcpe_work_check(void)
{
	int i;
	struct hipp_adapter_s *dev = g_hipp_adapter;

	if (dev == NULL) {
		loge("Failed : dev.%pK", dev);
		return 0;
	}

	mutex_lock(&dev->ipp_work_lock);
	for(i = 0; i < REFS_TYP_MAX; i++) {
		if (dev->hipp_refs[i] > 0) {
			loge("Failed: type.%d Opened", i);
			mutex_unlock(&dev->ipp_work_lock);
			return -ENOMEM;
		}
	}
	mutex_unlock(&dev->ipp_work_lock);

	return 0;
}

int hipp_powerdn(void)
{
	int ret;
	struct hipp_adapter_s *dev = g_hipp_adapter;
	logd("+");

	if (dev == NULL) {
		loge("Failed : dev.%pK", dev);
		return -EINVAL;
	}

	if (dev->ippdrv == NULL) {
		loge("Failed : ippdrv.%pK", dev->ippdrv);
		return -ENOMEM;
	}

	if (dev->ippdrv->disable_smmu == NULL) {
		loge("Failed : ippdrv->disable_smmu.%pK", dev->ippdrv->disable_smmu);
		return -ENOMEM;
	}

	ret = dev->ippdrv->disable_smmu(dev->ippdrv);
	if (ret != 0)
		loge("Failed : disable_smmu.%d", ret);

	ret = dev->ippdrv->power_dn(dev->ippdrv);
	if (ret != 0)
		loge("Failed : power_dn.%d", ret);

	logd("-");
	return ISP_IPP_OK;
}

int hispcpe_clean_wait_flag(struct hipp_adapter_s *dev, unsigned int wait_mode)
{
	logd("+");

	if (dev == NULL) {
		loge("Failed : dev.%pK", dev);
		return -EINVAL;
	}

	switch (wait_mode) {
	case WAIT_MODE_GF:
		dev->gf_ready = 0;
		break;

	case WAIT_MODE_ARFEATURE:
		dev->arfeature_ready = 0;
		break;

	case WAIT_MODE_RDR:
		dev->reorder_ready = 0;
		break;

	case WAIT_MODE_CMP:
		dev->compare_ready = 0;
		break;

	case WAIT_MODE_HIOF:
		dev->hiof_ready = 0;
		break;

	case WAIT_MODE_ENH:
		dev->orb_enh_ready = 0;
		break;

	case WAIT_MODE_MC:
		dev->mc_ready = 0;
		break;

	default:
		loge("Failed : wrong wait mode .%d", wait_mode);
		return ISP_IPP_ERR;
	}

	logd("-");
	return ISP_IPP_OK;
}

static void gf_debug_reg_dump_func()
{
	void __iomem *crg_base;
	int value = 0;
	crg_base = hipp_get_regaddr(CPE_TOP);
	value = readl(crg_base + IPP_TOP_GF_IRQ_REG3_REG);
	logi("gf_irq_state_mask = 0x%08x", value);
	value = readl(crg_base + IPP_TOP_GF_IRQ_REG4_REG);
	logi("gf_irq_state_raw = 0x%08x", value);
	crg_base = hipp_get_regaddr(CMDLIST_REG);
	value = readl(crg_base + CMDLST_LAST_EXEC_PTR_0_REG + 0x80 * IPP_GF_CHANNEL_ID);
	logi("cmdlst mc last exec ptr= 0x%08x", value);
	value = readl(crg_base + CMDLST_DEBUG_CHANNEL_0_REG + 0x80 * IPP_GF_CHANNEL_ID);
	logi("cmdlst debug channel = 0x%08x", value);
	crg_base = hipp_get_regaddr(CVDR_REG);
	value = readl(crg_base + CVDR_IPP_CVDR_IPP_VP_WR_SID_DEBUG_SID0_20_REG);
	logi("wr_20_LF_A_port: debug_reg = 0x%08x", value);
	value = readl(crg_base + CVDR_IPP_CVDR_IPP_VP_WR_SID_DEBUG_SID0_21_REG);
	logi("wr_21_HF_B_port: debug_reg = 0x%08x", value);
}

static int hispcpe_gf_request(struct hipp_adapter_s *dev,
							  struct msg_req_mcf_request_t *ctrl)
{
	int ret;
	int ret1;
	logd("+");

	if (dev == NULL || ctrl == NULL) {
		loge("Failed : dev.%pK, ctrl.%pK", dev, ctrl);
		return -EINVAL;
	}

	ret = hispcpe_clean_wait_flag(dev, WAIT_MODE_GF);
	if (ret < 0) {
		loge("Failed : hispcpe_clean_wait_flag.%d", ret);
		return ret;
	}

	ret = gf_request_handler(ctrl);
	if (ret != 0) {
		loge("Failed : gf_request_handler.%d", ret);
		goto GF_REQ_DONE;
	}

	ret = hispcpe_wait(dev, WAIT_MODE_GF);
	gf_debug_reg_dump_func(); // for gf_test_dual_in_lf_hf_out_5984_3360
	ret = 0; // only debug now, should be deleted on the commercial version.

	if (ret < 0) {
		loge("Failed : hispcpe_wait.%d", ret);
		ret = ISP_IPP_ERR;
		goto GF_REQ_DONE;
	}

GF_REQ_DONE:
	ret1 = ipp_eop_handler(CMD_EOF_GF_MODE);

	if (ret1 != 0)
		loge("gf_eop_handler.%d", ret1);

	logd("-");

	if (ret1 != 0 || ret != 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
}

static int  hipp_orcm_wait_flag(
	struct hipp_adapter_s *dev,
	enum work_module_e module)
{
	int ret0 = 0;
	int ret1 = 0;
	int ret2 = 0;
	int ret3 = 0;

	if (module == ENH_ARF ||
		module == ARFEATURE_ONLY) {
		ret0 = hispcpe_clean_wait_flag(dev,
									   WAIT_MODE_ARFEATURE);
	} else if (module == ARF_MATCHER ||
			   module  == ENH_ARF_MATCHER) {
		ret0 = hispcpe_clean_wait_flag(dev,
									   WAIT_MODE_ARFEATURE);
		ret1 = hispcpe_clean_wait_flag(dev,
									   WAIT_MODE_RDR);
		ret2 = hispcpe_clean_wait_flag(dev,
									   WAIT_MODE_CMP);
	} else if (module == MATCHER_MC) {
		ret0 = hispcpe_clean_wait_flag(dev,
									   WAIT_MODE_RDR);
		ret1 = hispcpe_clean_wait_flag(dev,
									   WAIT_MODE_CMP);
		ret2 = hispcpe_clean_wait_flag(dev,
									   WAIT_MODE_MC);
	} else if (module == ARF_MATCHER_MC ||
			   module == ENH_ARF_MATCHER_MC) {
		ret0 = hispcpe_clean_wait_flag(dev,
									   WAIT_MODE_ARFEATURE);
		ret1 = hispcpe_clean_wait_flag(dev,
									   WAIT_MODE_RDR);
		ret2 = hispcpe_clean_wait_flag(dev,
									   WAIT_MODE_CMP);
		ret3 = hispcpe_clean_wait_flag(dev,
									   WAIT_MODE_MC);
	} else if (module == MATCHER_ONLY) {
		ret0 = hispcpe_clean_wait_flag(dev,
									   WAIT_MODE_RDR);
		ret1 = hispcpe_clean_wait_flag(dev,
									   WAIT_MODE_CMP);
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
	logi("module = %d", module);

	if (module == ENH_ARF ||
		module == ARFEATURE_ONLY) {
		ret0 = hispcpe_wait(dev,
							WAIT_MODE_ARFEATURE);
	} else if (module == ARF_MATCHER ||
			   module  == ENH_ARF_MATCHER) {
		ret0 = hispcpe_wait(dev, WAIT_MODE_ARFEATURE);
		ret1 = hispcpe_wait(dev, WAIT_MODE_RDR);
		ret2 = hispcpe_wait(dev, WAIT_MODE_CMP);
	} else if (module == MATCHER_MC) {
		ret0 = hispcpe_wait(dev, WAIT_MODE_RDR);
		ret1 = hispcpe_wait(dev, WAIT_MODE_CMP);
		ret2 = hispcpe_wait(dev, WAIT_MODE_MC);
	} else if (module == ARF_MATCHER_MC ||
			   module == ENH_ARF_MATCHER_MC) {
		ret0 = hispcpe_wait(dev, WAIT_MODE_ARFEATURE);
		ret1 = hispcpe_wait(dev, WAIT_MODE_RDR);
		ret2 = hispcpe_wait(dev, WAIT_MODE_CMP);
		ret3 = hispcpe_wait(dev, WAIT_MODE_MC);
	} else if (module == MATCHER_ONLY) {
		ret0 = hispcpe_wait(dev, WAIT_MODE_RDR);
		ret1 = hispcpe_wait(dev, WAIT_MODE_CMP);
	}

	if (ret0 < 0 || ret1 < 0 || ret2 < 0 || ret3 < 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
}

static int  hipp_orcm_eop_handle(struct hipp_adapter_s *dev, struct msg_req_ipp_path_t *ctrl)
{
	int ret0 = 0;
	int ret1 = 0;
	int ret2 = 0;
	int ret3 = 0;
	enum work_module_e module = ctrl->work_module;

	if (module == ENH_ARF ||
		module == ARFEATURE_ONLY) {
		ret0 = ipp_eop_handler(CMD_EOF_ARFEATURE_MODE);
	} else if (module == ARF_MATCHER ||
			   module  == ENH_ARF_MATCHER) {
		ret0 = ipp_eop_handler(CMD_EOF_ARFEATURE_MODE);
		ret1 = ipp_eop_handler(CMD_EOF_RDR_MODE);
		ret2 = ipp_eop_handler(CMD_EOF_CMP_MODE);
	} else if (module == MATCHER_MC) {
		ret0 = ipp_eop_handler(CMD_EOF_RDR_MODE);
		ret1 = ipp_eop_handler(CMD_EOF_CMP_MODE);
		ret2 = ipp_eop_handler(CMD_EOF_MC_MODE);
	} else if (module == ARF_MATCHER_MC ||
			   module == ENH_ARF_MATCHER_MC) {
		ret0 =  ipp_eop_handler(CMD_EOF_ARFEATURE_MODE);
		ret1 =  ipp_eop_handler(CMD_EOF_RDR_MODE);
		ret2 =  ipp_eop_handler(CMD_EOF_CMP_MODE);
		ret3 =  ipp_eop_handler(CMD_EOF_MC_MODE);
	} else if (module == MATCHER_ONLY) {
		if (ctrl->req_matcher.rdr_pyramid_layer != 0)
			ret0 = ipp_eop_handler(CMD_EOF_RDR_MODE);

		if (ctrl->req_matcher.cmp_pyramid_layer != 0)
			ret1 = ipp_eop_handler(CMD_EOF_CMP_MODE);
	}

	if (ret0 < 0 || ret1 < 0 || ret2 < 0 || ret3 < 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
}

int hispcpe_hiof_request(struct hipp_adapter_s *dev, struct msg_req_hiof_request_t *ctrl)
{
#ifdef CONFIG_IPP_DEBUG
	void __iomem *crg_base;
	int value = 0;
#endif
	int ret = 0,  ret_1 = 0;
	logd("+");

	if (dev == NULL || ctrl == NULL) {
		loge("Failed: dev.%pK, ctrl.%pK", dev, ctrl);
		return -EINVAL;
	}

	ret = hispcpe_clean_wait_flag(dev, WAIT_MODE_HIOF);
	if (ret < 0) {
		loge("Failed: hispcpe_clean_wait_flag.%d", ret);
		return ret;
	}

	ret = hiof_request_handler(ctrl);
	if (ret != 0) {
		loge("Failed: hiof_request_handler.%d", ret);
		goto HIOF_REQ_DONE;
	}

	ret = hispcpe_wait(dev, WAIT_MODE_HIOF);
#ifdef CONFIG_IPP_DEBUG
	crg_base = hipp_get_regaddr(CPE_TOP);
	value = readl(crg_base + IPP_TOP_HIOF_IRQ_REG3_REG);
	loge("hiof_irq_state_raw = 0x%08x", value);
	value = readl(crg_base + IPP_TOP_HIOF_IRQ_REG4_REG);
	loge("hiof_irq_state_mask = 0x%08x", value);

	if (ctrl->hiof_cfg.hiof_ctrl_cfg.mode == HIOF_MODE_MD) {
		if (value == 0x5d) {
			loge("hiof irq is coming!");
			ret = 0;
		}
	} else if (ctrl->hiof_cfg.hiof_ctrl_cfg.mode == HIOF_MODE_TNR) {
		if (value == 0x5b) {
			loge("hiof irq is coming!");
			ret = 0;
		}
	} else if (ctrl->hiof_cfg.hiof_ctrl_cfg.mode == HIOF_MODE_MD_TNR) {
		if (value == 0x5f) {
			loge("hiof irq is coming!");
			ret = 0;
		}
	}

#endif

	if (ret < 0)
		loge("Failed: hispcpe_wait.%d", ret);

HIOF_REQ_DONE:
	ret_1 = ipp_eop_handler(CMD_EOF_HIOF_MODE);

	if (ret_1 != 0)
		loge("Failed: hiof_eop_handler.%d", ret_1);

	logd("-");

	if (ret_1 != 0 || ret != 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
}

#ifdef CONFIG_IPP_DEBUG
static int hipp_get_irq_ret(struct msg_req_ipp_path_t *ctrl)
{
	int arfvalue;
	int cmpvalue;
	int ret = ISP_IPP_ERR;
	int value;
	void __iomem *crg_base;
	crg_base = hipp_get_regaddr(CPE_TOP);
	value = readl(crg_base + IPP_TOP_ENH_IRQ_REG3_REG);
	loge("enh_irq_state_mask = 0x%08x", value);
	value = readl(crg_base + IPP_TOP_ENH_IRQ_REG4_REG);
	loge("enh_irq_state_raw = 0x%08x", value);
	value = readl(crg_base + IPP_TOP_ARF_IRQ_REG3_REG);
	loge("arf_irq_state_mask = 0x%08x", value);
	arfvalue = readl(crg_base + IPP_TOP_ARF_IRQ_REG4_REG);
	loge("arf_irq_state_raw = 0x%08x", arfvalue);
	value = readl(crg_base + IPP_TOP_RDR_IRQ_REG3_REG);
	loge("rdr_irq_state_mask = 0x%08x", value);
	value = readl(crg_base + IPP_TOP_RDR_IRQ_REG4_REG);
	loge("rdr_irq_state_raw = 0x%08x", value);
	value = readl(crg_base + IPP_TOP_CMP_IRQ_REG3_REG);
	loge("cmp_irq_state_mask = 0x%08x", value);
	cmpvalue = readl(crg_base + IPP_TOP_CMP_IRQ_REG4_REG);
	loge("cmp_irq_state_raw = 0x%08x", cmpvalue);
	value = readl(crg_base + IPP_TOP_MC_IRQ_REG3_REG);
	loge("mc_irq_state_mask = 0x%08x", value);
	value = readl(crg_base + IPP_TOP_MC_IRQ_REG4_REG);
	loge("mc_irq_state_raw = 0x%08x", value);

	if (ctrl->work_module == ARFEATURE_ONLY || ctrl->work_module == ENH_ARF) {
		if (arfvalue == 0x2f06 || arfvalue == 0x3000) {
			loge("arf irq is coming!");
			ret = 0;
		}
	} else if (ctrl->work_module == ARF_MATCHER || ctrl->work_module == ENH_ARF_MATCHER) {
		if (cmpvalue == 0xc) {
			loge("cmp irq is coming!");
			ret = 0;
		}
	} else if (ctrl->work_module == MATCHER_MC || ctrl->work_module == ARF_MATCHER_MC ||
			   ctrl->work_module == ENH_ARF_MATCHER_MC) {
		if (value == 0x2) {
			loge("mc irq is coming!");
			ret = 0;
		}
	}

	crg_base = hipp_get_regaddr(CMDLIST_REG);
	value = readl(crg_base + CMDLST_LAST_EXEC_PTR_0_REG + 0x80 * IPP_ARFEATURE_CHANNEL_ID);
	loge("arf cmdlst last exec ptr= 0x%08x", value);
	value = readl(crg_base + CMDLST_DEBUG_CHANNEL_0_REG + 0x80 * IPP_ARFEATURE_CHANNEL_ID);
	loge("arf cmdlst debug channel = 0x%08x", value);
	value = readl(crg_base + CMDLST_LAST_EXEC_PTR_0_REG + 0x80 * IPP_RDR_CHANNEL_ID);
	loge("rdr cmdlst last exec ptr= 0x%08x", value);
	value = readl(crg_base + CMDLST_DEBUG_CHANNEL_0_REG + 0x80 * IPP_RDR_CHANNEL_ID);
	loge("rdr cmdlst debug channel = 0x%08x", value);
	value = readl(crg_base + CMDLST_LAST_EXEC_PTR_0_REG + 0x80 * IPP_CMP_CHANNEL_ID);
	loge("cmp cmdlst last exec ptr= 0x%08x", value);
	value = readl(crg_base + CMDLST_DEBUG_CHANNEL_0_REG + 0x80 * IPP_CMP_CHANNEL_ID);
	loge("cmp cmdlst debug channel = 0x%08x", value);
	value = readl(crg_base + CMDLST_LAST_EXEC_PTR_0_REG + 0x80 * IPP_MC_CHANNEL_ID);
	loge("mc cmdlst last exec ptr= 0x%08x", value);
	value = readl(crg_base + CMDLST_DEBUG_CHANNEL_0_REG + 0x80 * IPP_MC_CHANNEL_ID);
	loge("mc cmdlst debug channel = 0x%08x", value);
	return ret;
}
#endif

int hispcpe_ipp_path_request(struct hipp_adapter_s *dev, struct msg_req_ipp_path_t *ctrl)
{
	int ret = 0,  ret_1 = 0;
	logd("+");

	if (dev == NULL || ctrl == NULL) {
		loge("Failed: dev.%pK, ctrl.%pK", dev, ctrl);
		return -EINVAL;
	}

	if (ctrl->work_module == MC_ONLY) {
		ret = hispcpe_mc_request(dev, &(ctrl->req_mc));
		return ret;
	} else if (ctrl->work_module == ENH_ONLY) {
		ret = hispcpe_orb_enh_request(dev, &(ctrl->req_arf_cur.req_enh));
		return ret;
	}

	ret = hipp_orcm_wait_flag(dev, ctrl->work_module);
	if (ret < 0) {
		loge("Failed: hispcpe_orcm_wait_flag.%d", ret);
		return ret;
	}

	ret = ipp_path_request_handler(ctrl);
	if (ret != 0) {
		loge("Failed: ipp_path_request_handler.%d", ret);
		goto IPP_PATH_REQ_DONE;
	}

	ret = hipp_orcm_wait(dev, ctrl->work_module);
#ifdef CONFIG_IPP_DEBUG
	ret = hipp_get_irq_ret(ctrl);
#endif
	if (ret < 0) {
		loge("Failed: hispcpe_orcm_wait.%d", ret);
		ret = ISP_IPP_ERR;
		goto IPP_PATH_REQ_DONE;
	}

IPP_PATH_REQ_DONE:
	ret_1 = hipp_orcm_eop_handle(dev, ctrl);

	if (ret_1 != 0)
		loge("Failed : hipp_orcm_eop_handle.%d", ret_1);

	logd("-");

	if (ret_1 != 0 || ret != 0)
		return ISP_IPP_ERR;
	else
		return ISP_IPP_OK;
}

static int ipp_path_process_internal(struct hipp_adapter_s *dev,
									 struct msg_req_ipp_path_t *ctrl_ipp_path)
{
	int ret;
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

	ret = hispcpe_ipp_path_request(dev, ctrl_ipp_path);
	if (ret != 0) {
		loge("Failed : hispcpe_ipp_path_request.%d", ret);

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
	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv, dev->curr_cpe_rate_value[CLK_RATE_INDEX_ARFEATURE]);
	if (ret != 0) {
		loge("Failed : ipp_set_rate.%d", ret);
		return ret;
	}

	return ret;
}

static int gf_process_internal(struct hipp_adapter_s *dev,
							   struct msg_req_mcf_request_t *ctrl_mcf)
{
	unsigned int ret;
	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv, dev->curr_cpe_rate_value[CLK_RATE_INDEX_GF]);
	if (ret != 0) {
		loge("Failed : ipp_set_rate.%d", ret);
		ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv, CLK_RATE_SVS);
		if (ret != 0) {
			loge("Failed to set ipp rate mode: %d", CLK_RATE_SVS);
			return ret;
		}
	}

	ret = hispcpe_gf_clk_enable();
	if (ret != 0) {
		loge("Failed : hispcpe_gf_clk_enable.%d", ret);
		return ret;
	}

	ret = hispcpe_gf_request(dev, ctrl_mcf);
	if (ret != 0) {
		loge("Failed : hispcpe_gf_request.%d", ret);

		if (hispcpe_gf_clk_disable() != 0)
			loge("Failed : hispcpe_gf_clk_disable");

		return ret;
	}

	ret = hispcpe_gf_clk_disable();
	if (ret != 0) {
		loge("Failed : hispcpe_gf_clk_disable.%d", ret);
		return ret;
	}

	dev->curr_cpe_rate_value[CLK_RATE_INDEX_GF] = CLK_RATE_SVS;
	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
									   dev->curr_cpe_rate_value[CLK_RATE_INDEX_GF]);
	if (ret != 0) {
		loge("Failed : ipp_set_rate.%d", ret);
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
	loge_if_null(args_mcf);

	ctrl_mcf = (struct msg_req_mcf_request_t *)vmalloc(sizeof(struct msg_req_mcf_request_t));
	if (ctrl_mcf == NULL) {
		loge("Failed : fail to vmalloc ctrl_mcf");
		return -EINVAL;
	}

	if (memset_s(ctrl_mcf, sizeof(struct msg_req_mcf_request_t), 0, sizeof(struct msg_req_mcf_request_t))) {
		loge("Failed : fail to memset_s ctrl_mcf");
		ret = ISP_IPP_ERR;
		goto free_mcf_vmalloc_memory;
	}

	ret = copy_from_user(ctrl_mcf, args_mcf, sizeof(struct msg_req_mcf_request_t));
	if (ret != 0) {
		loge("Failed : copy_from_user.%d", ret);
		goto free_mcf_vmalloc_memory;
	}

	gf_rate_index = ctrl_mcf->mcf_rate_value;
	gf_rate_index = gf_rate_index > CLK_RATE_SVS ? 0 : gf_rate_index;
	dev->curr_cpe_rate_value[CLK_RATE_INDEX_GF] = gf_rate_index;
	ret = gf_process_internal(dev, ctrl_mcf);
	if (ret != 0) {
		loge("Failed : gf_process_internal.%d", ret);
		goto free_mcf_vmalloc_memory;
	}

free_mcf_vmalloc_memory:
	vfree(ctrl_mcf);
	ctrl_mcf = NULL;
	return ret;
}

static int hiof_process_internal(struct hipp_adapter_s *dev,
								 struct msg_req_hiof_request_t *ctrl_hiof)
{
	unsigned int ret;
	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
									   dev->curr_cpe_rate_value[CLK_RATE_INDEX_HIOF]);
	if (ret != 0) {
		loge("Failed : ipp_set_rate.%d", ret);
		ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv, CLK_RATE_SVS);
		if (ret != 0) {
			loge("Failed to set ipp  rate mode: %d", CLK_RATE_SVS);
			return ret;
		}
	}

	ret = hispcpe_hiof_clk_enable();
	if (ret != 0) {
		loge("Failed : hispcpe_hiof_clk_enable.%d", ret);
		return ret;
	}

	ret = hispcpe_hiof_request(dev, ctrl_hiof);
	if (ret != 0) {
		loge("Failed : hispcpe_hiof_request.%d", ret);

		if (hispcpe_hiof_clk_disable() != 0)
			loge("Failed : hispcpe_hiof_clk_disable");

		return ret;
	}

	ret = hispcpe_hiof_clk_disable();
	if (ret != 0) {
		loge("Failed : hispcpe_hiof_clk_disable.%d", ret);
		return ret;
	}

	dev->curr_cpe_rate_value[CLK_RATE_INDEX_HIOF] = CLK_RATE_SVS;
	ret = dev->ippdrv->set_jpgclk_rate(dev->ippdrv,
									   dev->curr_cpe_rate_value[CLK_RATE_INDEX_HIOF]);
	if (ret != 0) {
		loge("Failed : ipp_set_rate.%d", ret);
		return ret;
	}

	return ret;
}

int hiof_process(unsigned long args)
{
	unsigned int ret = 0;
	unsigned int hiof_rate_index = 0;
	struct msg_req_hiof_request_t *ctrl_hiof = NULL;
	void __user *args_hiof = (void __user *)(uintptr_t) args;
	struct hipp_adapter_s *dev = g_hipp_adapter;
	loge_if_null(args_hiof);

	ctrl_hiof = (struct msg_req_hiof_request_t *)vmalloc(sizeof(struct msg_req_hiof_request_t));
	if (ctrl_hiof == NULL) {
		loge("Failed : vmalloc ctrl_hiof");
		return -EINVAL;
	}

	if (memset_s(ctrl_hiof, sizeof(struct msg_req_hiof_request_t),
				 0, sizeof(struct msg_req_hiof_request_t))) {
		loge("Failed : fail to memset_s ctrl_hiof");
		ret = ISP_IPP_ERR;
		goto free_hiof_vmalloc_memory;
	}

	ret = copy_from_user(ctrl_hiof, args_hiof, sizeof(struct msg_req_hiof_request_t));
	if (ret != 0) {
		loge("Failed : copy_from_user.%d", ret);
		goto free_hiof_vmalloc_memory;
	}

	hiof_rate_index = ctrl_hiof->hiof_rate_value;
	dev->curr_cpe_rate_value[CLK_RATE_INDEX_HIOF] = hiof_rate_index > CLK_RATE_SVS ? 0 : hiof_rate_index;
	logd("dev->curr_cpe_rate_value[CLK_RATE_INDEX_HIOF] = %d", dev->curr_cpe_rate_value[CLK_RATE_INDEX_HIOF]);
	ret = hiof_process_internal(dev, ctrl_hiof);
	if (ret != 0) {
		loge("Failed : hiof_process_internal.%d", ret);
		goto free_hiof_vmalloc_memory;
	}

free_hiof_vmalloc_memory:
	vfree(ctrl_hiof);
	return ret;
}

static int hipp_of_process(struct msg_req_hiof_request_t *ctrl_hiof)
{
	unsigned int ret = 0;
	unsigned int hiof_rate_index;
	struct hipp_adapter_s *dev = g_hipp_adapter;

	mutex_lock(&dev->ipp_work_lock);

	if (dev->hipp_refs[REFS_TYP_HIOF] > 0) {
		loge("Failed : hiof refs Opened");
		mutex_unlock(&dev->ipp_work_lock);
		return -EINVAL;
	}

	dev->hipp_refs[REFS_TYP_HIOF] = 1;
	mutex_unlock(&dev->ipp_work_lock);
	hiof_rate_index = ctrl_hiof->hiof_rate_value;
	hiof_rate_index = hiof_rate_index > CLK_RATE_SVS ? 0 : hiof_rate_index;
	dev->curr_cpe_rate_value[CLK_RATE_INDEX_HIOF] = hiof_rate_index;
	ret = hiof_process_internal(dev, ctrl_hiof);
	if (ret != 0)
		loge("Failed : hiof_process_internal.%d", ret);

	dev->hipp_refs[REFS_TYP_HIOF] = 0;
	return ret;
}

static int hipp_path_process(struct msg_req_ipp_path_t *ctrl_ipp_path)
{
	int ret = 0;
	unsigned int arf_rate_index;
	struct hipp_adapter_s *dev = g_hipp_adapter;

	mutex_lock(&dev->ipp_work_lock);
	if (dev->hipp_refs[REFS_TYP_ORCM] > 0) {
		loge("Failed: ipp_path refs Opened");
		mutex_unlock(&dev->ipp_work_lock);
		return -EINVAL;
	}
	dev->hipp_refs[REFS_TYP_ORCM] = 1;
	mutex_unlock(&dev->ipp_work_lock);
	arf_rate_index = ctrl_ipp_path->arfeature_rate_value;
	arf_rate_index = arf_rate_index > CLK_RATE_SVS ? 0 : arf_rate_index;
	dev->curr_cpe_rate_value[CLK_RATE_INDEX_ARFEATURE] = arf_rate_index;
	logd("dev->curr_cpe_rate_value[CLK_RATE_INDEX_ARFEATURE] = %d", dev->curr_cpe_rate_value[CLK_RATE_INDEX_ARFEATURE]);
	if (ctrl_ipp_path->mode_cnt > ARF_AR_MODE_CNT ||
		ctrl_ipp_path->req_matcher.cmp_pyramid_layer > MATCHER_LAYER_MAX) {
		loge("Failed:Invalid arf mode_cnt = %d,cmp_pyramid_layer = %d",
			 ctrl_ipp_path->mode_cnt, ctrl_ipp_path->req_matcher.cmp_pyramid_layer);
		goto free_ipp_path_lock;
	}

	ret = ipp_path_process_internal(dev, ctrl_ipp_path);
	if (ret != 0) {
		loge("Failed : ipp_path_process_internal.%d", ret);
		goto free_ipp_path_lock;
	}

free_ipp_path_lock:
	dev->hipp_refs[REFS_TYP_ORCM] = 0;
	return ret;
}

int hipp_path_of_process(unsigned long args)
{
	int ret = 0;
	unsigned long remaining_bytes = 0;
	struct msg_req_ipp_path_t *ctrl_ipp_path = NULL;
	void __user *args_ipp_path = (void __user *)(uintptr_t) args;
	loge_if_null(args_ipp_path);
	ctrl_ipp_path = (struct msg_req_ipp_path_t *)vmalloc(sizeof(struct msg_req_ipp_path_t));
	if (ctrl_ipp_path == NULL) {
		loge("fail to vmalloc ctrl_ipp_ptah");
		return -EINVAL;
	}

	if (memset_s(ctrl_ipp_path, sizeof(struct msg_req_ipp_path_t), 0, sizeof(struct msg_req_ipp_path_t))) {
		loge("Failed : fail to memset_s ctrl_ipp_path");
		ret = ISP_IPP_ERR;
		goto free_ipp_path_vmalloc_memory;
	}

	remaining_bytes = copy_from_user(ctrl_ipp_path, args_ipp_path, sizeof(struct msg_req_ipp_path_t));
	if (remaining_bytes != 0) {
		loge("Failed : copy_from_user, remaining_bytes = %ld", remaining_bytes);
		ret = ISP_IPP_ERR;
		goto free_ipp_path_vmalloc_memory;
	}

	if (ctrl_ipp_path->work_module == OPTICAL_FLOW) {
		loge("ipp_path_req: frame_num = %d, secure_flag = %d, work_module = %s",
			ctrl_ipp_path->frame_num, ctrl_ipp_path->secure_flag, "OPTICAL_FLOW");
		ret = hipp_of_process(&ctrl_ipp_path->req_hiof);
	} else {
		ret = hipp_path_process(ctrl_ipp_path);
	}

free_ipp_path_vmalloc_memory:
	vfree(ctrl_ipp_path);
	return ret;
}

int hipp_adapter_map_iommu(unsigned long args)
{
	unsigned long iova = 0;
	unsigned long iova_size = 0;
	int ret = 0;
	struct hipp_adapter_s *dev = g_hipp_adapter;
	void __user *map_iommu = NULL;
	struct memory_block_s frame_buf = { 0 };
	struct dma_buf *modulebuf = NULL;

	logi("+");

	map_iommu = (void __user *)(uintptr_t)args;
	if (map_iommu == NULL) {
		pr_err("[%s] args_map_iommu.NULL\n", __func__);
		return -EINVAL;
	}

	ret = copy_from_user(&frame_buf, map_iommu, sizeof(struct memory_block_s));
	if (ret != 0) {
		pr_err("[%s] copy_from_user.%d\n", __func__, ret);
		return -EFAULT;
	}

	dataflow_check_para(dev);

	if (frame_buf.shared_fd < 0) {
		loge("Failed : share_fd.%d", frame_buf.shared_fd);
		return -EINVAL;
	}

	if (dev->ippdrv == NULL) {
		loge("Failed : ippdrv NULL");
		return -EINVAL;
	}

	if (dev->ippdrv->iommu_map == NULL) {
		loge("Failed : iommu.NULL");
		return -EINVAL;
	}

	modulebuf = dma_buf_get(frame_buf.shared_fd);
	if (IS_ERR_OR_NULL(modulebuf)) {
		pr_err("[%s] Failed : shared_fd.%d\n", __func__, frame_buf.shared_fd);
		return -EINVAL;
	}

	if (modulebuf->size != frame_buf.size) {
		pr_err("[%s] Failed : size not equal\n", __func__);
		dma_buf_put(modulebuf);
		return -EINVAL;
	}

	iova = dev->ippdrv->iommu_map(dev->ippdrv, modulebuf, frame_buf.prot, &iova_size);
	if (iova == 0) {
		dma_buf_put(modulebuf);
		loge("Failed : iommu sharedfd.%d, prot.0x%lx", frame_buf.shared_fd,
			 frame_buf.prot);
		return -EINVAL;
	}

	if (frame_buf.size != iova_size) {
		loge("Failed : iommu_map, size.0x%x, out.0x%lx", frame_buf.size, iova_size);
		goto err_dma_buf_get;
	}

	frame_buf.da = (unsigned int)iova;
	logd("[after map iommu]da.(0x%x)", frame_buf.da);

	ret = copy_to_user(map_iommu, &frame_buf, sizeof(struct memory_block_s));
	if (ret != 0) {
		pr_err("[%s] copy_to_user.%d\n", __func__, ret);
		goto err_dma_buf_get;
	}

	dma_buf_put(modulebuf);
	logi("-");
	return ISP_IPP_OK;

err_dma_buf_get:
	if (dev->ippdrv->iommu_unmap != NULL) {
		ret = dev->ippdrv->iommu_unmap(dev->ippdrv, modulebuf, iova);
		if (ret == 0)
			loge("Failed : ipp: sharedfd.%d, iova.0x%lx",
				frame_buf.shared_fd, iova);
	}

	dma_buf_put(modulebuf);

	return -EINVAL;
}

int hipp_adapter_unmap_iommu(unsigned long args)
{
	int ret = 0;
	struct hipp_adapter_s *dev = g_hipp_adapter;
	void __user *unmap_iommu = NULL;
	struct memory_block_s frame_buf = {0};
	struct dma_buf *modulebuf = NULL;

	logi("+");
	unmap_iommu = (void __user *)(uintptr_t)args;
	if (unmap_iommu == NULL) {
		pr_err("[%s] args_unmap_iommu.NULL\n", __func__);
		return -EINVAL;
	}

	ret = copy_from_user(&frame_buf, unmap_iommu,	sizeof(struct memory_block_s));
	if (ret != 0) {
		pr_err("[%s] copy_from_user.%d\n", __func__, ret);
		return -EFAULT;
	}

	if (dev == NULL) {
		loge("Failed : pstFrameBuf or dev NULL");
		return -EINVAL;
	}

	if (frame_buf.shared_fd < 0) {
		loge("Failed : share_fd.%d", frame_buf.shared_fd);
		return -EINVAL;
	}

	if (frame_buf.da == 0) {
		loge("Failed : iova.%d", frame_buf.da);
		return -EINVAL;
	}

	if (dev->ippdrv == NULL) {
		loge("Failed : ippdrv NULL");
		return -EINVAL;
	}

	if (dev->ippdrv->iommu_unmap == NULL) {
		loge("Failed : iommu_unmap.NULL");
		return -EINVAL;
	}

	modulebuf = dma_buf_get(frame_buf.shared_fd);
	if (IS_ERR_OR_NULL(modulebuf)) {
		pr_err("[%s] Failed : shared_fd.%d\n", __func__, frame_buf.shared_fd);
		return -EINVAL;
	}

	ret = dev->ippdrv->iommu_unmap(dev->ippdrv, modulebuf, frame_buf.da);
	if (ret != 0)
		loge("Failed : ipp: sharedfd.%d", frame_buf.shared_fd);
	dma_buf_put(modulebuf);

	logi("-");
	return ret;
}

/**
 * warning: unused function
 *
static int get_frame_buf(struct map_buff_block_s *frame_buf, void __user *args_mapkernel)
{
	int ret;
	ret = copy_from_user(frame_buf, args_mapkernel, sizeof(struct map_buff_block_s));

	if (ret != 0) {
		loge("Failed : copy_from_user.%d", ret);
		return -EFAULT;
	}

	if (frame_buf->shared_fd < 0) {
		loge("Failed : share_fd.%d", frame_buf->shared_fd);
		return -EINVAL;
	}

	return ISP_IPP_OK;
}

static int dev_iommu_map_kmap(struct hipp_adapter_s *dev, struct map_buff_block_s *frame_buf)
{
	int ret;
	void *virt_addr = NULL;
	unsigned long iova;
	unsigned long iova_size = 0;
	iova = dev->ippdrv->iommu_map(dev->ippdrv, frame_buf->shared_fd,
								  frame_buf->prot, &iova_size);
	if (iova == 0) {
		loge("Failed : iommu sharedfd.%d, prot.0x%llu",
			 frame_buf->shared_fd, frame_buf->prot);
		return -EINVAL;
	}

	if (iova_size != frame_buf->size) {
		loge("Failed : iommu_map sharedfd.%d, size.%d",
			 frame_buf->shared_fd, frame_buf->size);
		goto free_iommu_map;
	}

	virt_addr = dev->ippdrv->kmap(dev->ippdrv, frame_buf->shared_fd);
	if (virt_addr == NULL) {
		loge("Failed : kmap,sharedfd.%d", frame_buf->shared_fd);
		goto free_iommu_map;
	}

	dev->virt_addr = virt_addr;
	dev->daddr = iova;
	dev->shared_fd = frame_buf->shared_fd;
	logd("[after map kernel]virt_addr.0x%pK, da.0x%lx, sharedfd.0x%d",
		 dev->virt_addr, dev->daddr, dev->shared_fd);
	return ISP_IPP_OK;
free_iommu_map:

	if (dev->ippdrv->iommu_unmap == NULL) {
		loge("Failed : ippdrv->iommu_unmap NULL");
		return -EINVAL;
	}

	ret = dev->ippdrv->iommu_unmap(dev->ippdrv, frame_buf->shared_fd, iova);
	if (ret == 0)
		loge("Failed : iommu_unmap, sharedfd.%d, iova.0x%lx", frame_buf->shared_fd, iova);

	return -EINVAL;
}
 *
 *
**/

static int check_shared_fd_and_dev_para(struct hipp_adapter_s *dev, struct map_buff_block_s *frame_buf)
{
	if (frame_buf->shared_fd < 0) {
		loge("Failed : share_fd.%d", frame_buf->shared_fd);
		return -EINVAL;
	}

	if (dev->ippdrv == NULL) {
		loge("Failed : ippdrv NULL");
		return -EINVAL;
	}

	if (dev->ippdrv->iommu_map == NULL) {
		loge("Failed : ippdrv->iommu_map NULL");
		return -EINVAL;
	}

	if (dev->ippdrv->kmap == NULL) {
		loge("Failed : ippdrv->kmap NULL");
		return -EINVAL;
	}

	return ISP_IPP_OK;
}

int hispcpe_map_kernel(unsigned long args)
{
	void *virt_addr = NULL;
	struct map_buff_block_s frame_buf = { 0 };
	unsigned long iova, iova_size = 0;
	int ret;
	void __user *args_mapkernel = (void __user *)(uintptr_t) args;
	struct hipp_adapter_s *dev = g_hipp_adapter;
	struct dma_buf *modulebuf = NULL;

	logi("+");

	if ((args_mapkernel == NULL) || (dev == NULL)) {
		loge("Failed : args_mapkernel.%pK", args_mapkernel);
		return -EINVAL;
	}

	loge_if_ret_efault(copy_from_user(&frame_buf, args_mapkernel, sizeof(frame_buf)));
	loge_if_ret_einval(check_shared_fd_and_dev_para(dev, &frame_buf));

	modulebuf = dma_buf_get(frame_buf.shared_fd);
	if (IS_ERR_OR_NULL(modulebuf)) {
		pr_err("[%s] Failed : dma_buf_get\n", __func__);
		return -EINVAL;
	}

	if (modulebuf->size != frame_buf.size) {
		pr_err("[%s] Failed : size not equal\n", __func__);
		dma_buf_put(modulebuf);
		return -EINVAL;
	}

	iova = dev->ippdrv->iommu_map(dev->ippdrv, modulebuf, frame_buf.prot, &iova_size);
	if (iova == 0) {
		loge("Failed : prot.0x%llu", frame_buf.prot);
		dma_buf_put(modulebuf);
		return -EINVAL;
	}

	if (iova_size != frame_buf.size) {
		loge("Failed : iommu_map size.%d", frame_buf.size);
		goto free_iommu_map;
	}

	virt_addr = dev->ippdrv->kmap(dev->ippdrv, modulebuf);
	if (virt_addr == NULL) {
		loge("Failed : kmap,sharedfd.%d", frame_buf.shared_fd);
		goto free_iommu_map;
	}

	dev->virt_addr = virt_addr;
	dev->daddr = iova;
	dev->dmabuf = modulebuf;
	logd("[after map kernel]virt_addr.0x%pK, da.0x%lx", virt_addr, iova);
	logi("-");
	return ISP_IPP_OK;

free_iommu_map:
	if (dev->ippdrv->iommu_unmap != NULL) {
		ret = dev->ippdrv->iommu_unmap(dev->ippdrv, modulebuf, iova);
		if (ret == 0)
			loge("Failed : iommu_unmap, sharedfd.%d, iova.0x%lx",
				frame_buf.shared_fd, iova);
	}

	dma_buf_put(modulebuf);
	dev->dmabuf = NULL;
	return -EINVAL;
}

int hispcpe_unmap_kernel(void)
{
	int ret;
	struct hipp_adapter_s *dev = g_hipp_adapter;
	logi("+");

	if (dev == NULL) {
		loge("Failed : dev.%pK", dev);
		return -EINVAL;
	}

	if (dev->ippdrv == NULL) {
		loge("Failed : ippdrv NULL");
		return -EINVAL;
	}

	if (dev->ippdrv->iommu_unmap == NULL) {
		loge("Failed : ippdrv->iommu_unmap NULL");
		return -EINVAL;
	}

	if (dev->ippdrv->kunmap == NULL) {
		loge("Failed : ippdrv->kunmap NULL");
		return -EINVAL;
	}

	if (dev->daddr != 0) {
		ret = dev->ippdrv->iommu_unmap(dev->ippdrv, dev->dmabuf, dev->daddr);
		if (ret != 0) {
			loge("Failed: iommu_unmap.%d", ret);
			return -EINVAL;
		}
		dev->daddr = 0;
	}

	ret = dev->ippdrv->kunmap(dev->ippdrv, dev->dmabuf, dev->virt_addr);
	if (ret != 0) {
		loge("Failed : dev->ippdrv->kunmap.%d", ret);
		return -EINVAL;
	}

	dma_buf_put(dev->dmabuf);
	dev->dmabuf = NULL;
	dev->virt_addr = NULL;

	logi("-");
	return ISP_IPP_OK;
}

void hipp_adapter_exception(void)
{
	int ret;

	logi("+");
	ret = hispcpe_unmap_kernel();
	if (ret != 0)
		loge("Failed : hispcpe_unmap_kernel.%d", ret);

	logi("-");
}

int hipp_adapter_probe(struct platform_device *pdev)
{
	struct hipp_adapter_s *dev = NULL;
	unsigned int i = 0;
	logi("+");
	g_hipp_adapter = NULL;
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (dev == NULL)
		return -ENOMEM;

	dev->ippdrv = hipp_common_register(IPP_UNIT, &pdev->dev);

	if (dev->ippdrv == NULL) {
		loge("Failed : IPP_UNIT register");
		goto free_dev;
	}

	init_waitqueue_head(&dev->gf_wait);
	init_waitqueue_head(&dev->hiof_wait);
	init_waitqueue_head(&dev->reorder_wait);
	init_waitqueue_head(&dev->compare_wait);
	init_waitqueue_head(&dev->orb_enh_wait);
	init_waitqueue_head(&dev->mc_wait);
	init_waitqueue_head(&dev->arfeature_wait);

	for (i = 0; i < REFS_TYP_MAX; i++)
		dev->hipp_refs[i] = 0;

	mutex_init(&dev->ipp_work_lock);
	mutex_init(&dev->cvdr_lock);
	g_hipp_adapter = dev;
	cmdlst_priv_prepare();
	logi("-");
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
	logi("+");

	if (dev == NULL) {
		loge("Failed : drvdata NULL");
		return;
	}

	ret = hipp_common_unregister(IPP_UNIT);
	if (ret != 0)
		loge("Failed: IPP unregister.%d", ret);

	if (dev->ippdrv != NULL)
		dev->ippdrv = NULL;

	for (i = 0; i < REFS_TYP_MAX; i++)
		dev->hipp_refs[i] = 0;

	mutex_destroy(&dev->ipp_work_lock);
	mutex_destroy(&dev->cvdr_lock);
	kfree(dev);
	logi("-");
}

