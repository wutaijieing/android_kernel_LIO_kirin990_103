/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2021. All rights reserved.
 *
 * jpeg jpu utils
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"

#include "jpu_utils.h"
#include "linux/delay.h"
#include <linux/interrupt.h>
#include <platform_include/camera/jpeg/jpeg_base.h>
#include "jpu.h"
#include "jpu_iommu.h"
#include "jpu_def.h"
#include "jpgdec_irq.h"
#include "jpgdec_platform.h"

static void jpu_set_reg(char __iomem *addr, uint32_t val, uint8_t bw, uint8_t bs)
{
	uint32_t mask = (uint32_t)((1UL << bw) - 1UL);
	uint32_t reg_tmp;

	reg_tmp = (uint32_t)inp32(addr);
	reg_tmp &= ~(mask << bs);

	outp32(addr, reg_tmp | ((val & mask) << bs));
}

static void jpu_dec_interrupt_unmask(const struct jpu_data_type *jpu_device)
{
	uint32_t unmask;

	unmask = ~0;
	unmask &= ~(BIT_JPGDEC_INT_DEC_ERR | BIT_JPGDEC_INT_DEC_FINISH);

	outp32(jpu_device->jpu_top_base + JPGDEC_IRQ_REG1, unmask);
}

static void jpu_dec_interrupt_mask(const struct jpu_data_type *jpu_device)
{
	uint32_t mask;

	mask = ~0;
	outp32(jpu_device->jpu_top_base + JPGDEC_IRQ_REG1, mask);
}

static void jpu_dec_interrupt_clear(const struct jpu_data_type *jpu_device)
{
	/* clear jpg decoder IRQ state
	 * [3]: jpgdec_int_over_time;
	 * [2]: jpgdec_int_dec_err;
	 * [1]: jpgdec_int_bs_res;
	 * [0]: jpgdec_int_dec_finish;
	 */
	outp32(jpu_device->jpu_top_base + JPGDEC_IRQ_REG0, 0xF); /* 0xF clera irq */
}

#if (!defined CONFIG_DPU_FB_V600) && (!defined CONFIG_DKMD_DPU_VERSION)
static int jpu_regulator_disable(
	const struct jpu_data_type *jpu_device)
{
	int ret;

	jpu_debug("+\n");
#ifdef CONFIG_NO_USE_INTERFACE
	ret = 0;
#else
	ret = regulator_disable(jpu_device->jpu_regulator);
	if (ret != 0)
		jpu_err("jpu regulator_disable failed, error = %d\n", ret);
#endif

	jpu_debug("-\n");
	return ret;
}
#endif

int jpu_register(struct jpu_data_type *jpu_device)
{
	int ret;

	jpu_check_null_return(jpu_device, -EINVAL);
	jpu_check_null_return(jpu_device->pdev, -EINVAL);
	jpu_device->jpu_dec_done_flag = 0;
	init_waitqueue_head(&jpu_device->jpu_dec_done_wq);

	sema_init(&jpu_device->blank_sem, 1);
	jpu_device->power_on = false;
	jpu_device->jpu_res_initialized = false;

	ret = jpu_enable_iommu(jpu_device);
	if (ret != 0) {
		dev_err(&jpu_device->pdev->dev, "jpu_enable_iommu failed\n");
		return ret;
	}

	ret = jpu_lb_alloc(jpu_device);
	if (ret != 0) {
		dev_err(&jpu_device->pdev->dev, "jpu_lb_alloc failed\n");
		return ret;
	}

	ret = jpgdec_request_irq(jpu_device);
	if (ret != 0) {
		dev_err(&jpu_device->pdev->dev, "jpgdec_request_irq failed\n");
		return ret;
	}

#if defined(CONFIG_DPU_FB_V501) || defined(CONFIG_DPU_FB_V510)
	jpu_device->jpgd_drv = hipp_register(JPEGD_UNIT, NONTRUS);
	jpu_err_if_cond((jpu_device->jpgd_drv == NULL), "Failed: hipp_register\n");
#endif

#if defined(CONFIG_DPU_FB_V600) || defined(CONFIG_DKMD_DPU_VERSION)
	jpu_device->jpgd_drv = hipp_common_register(JPEGD_UNIT, &jpu_device->pdev->dev);
	jpu_err_if_cond((jpu_device->jpgd_drv == NULL), "Failed: hipp_common_register\n");
#endif

	return 0;
}

int jpu_unregister(struct jpu_data_type *jpu_device)
{
	int ret;

	jpu_check_null_return(jpu_device, -EINVAL);
	jpu_check_null_return(jpu_device->pdev, -EINVAL);

#if defined(CONFIG_DPU_FB_V501) || defined(CONFIG_DPU_FB_V510)
	jpu_err_if_cond(hipp_unregister(JPEGD_UNIT),
		"Failed: hipp_unregister\n");
#endif

#if defined(CONFIG_DPU_FB_V600) || defined(CONFIG_DKMD_DPU_VERSION)
	ret = hipp_common_unregister(JPEGD_UNIT);
	jpu_err_if_cond(ret < 0, "Failed: hipp_common_unregister %d\n", ret);
#endif

	jpgdec_disable_irq(jpu_device);
	jpgdec_free_irq(jpu_device);
	jpu_lb_free(jpu_device);

	return 0;
}

static int jpu_check_reg_state(const char __iomem *addr, uint32_t val)
{
	uint32_t tmp;
	int delay_count = 0;
	bool is_timeout = true;

	while (1) {
		tmp = (uint32_t) inp32(addr);
		if (((tmp & val) == val) || (delay_count > 10)) { /* loop less 10 */
			is_timeout = (delay_count > 10) ? true : false;
			udelay(10); /* 10us */
			break;
		}

		udelay(10); /* 10us */
		++delay_count;
	}

	if (is_timeout) {
		jpu_err("fail to wait reg\n");
		return -1;
	}

	return 0;
}

void jpu_dec_normal_reset(const struct jpu_data_type *jpu_device)
{
	int ret;

	if (jpu_device == NULL) {
		jpu_err("jpu_device is NULL\n");
		return;
	}

	jpu_set_reg(jpu_device->jpu_cvdr_base +
		JPGDEC_CVDR_AXI_RD_CFG1, 0x1, 1, REG_SET_25_BIT);

	ret = jpu_check_reg_state(jpu_device->jpu_cvdr_base +
		JPGDEC_CVDR_AXI_RD_CFG1, AXI_JPEG_NR_RD_STOP);
	if (ret != 0)
		jpu_err("fail to wait JPGDEC_CVDR_AXI_RD_CFG1\n");

	jpu_set_reg(jpu_device->jpu_cvdr_base +
		JPGDEC_CVDR_AXI_RD_CFG1, 0x0, 1, REG_SET_25_BIT);
}

static void jpu_set_reg_cvdr(const struct jpu_data_type *jpu_device,
	uint32_t val)
{
	jpu_set_reg(jpu_device->jpu_cvdr_base +
		JPGDEC_CVDR_AXI_RD_CFG1, val, 1, REG_SET_25_BIT);

	jpu_set_reg(jpu_device->jpu_cvdr_base +
		JPGDEC_CVDR_AXI_RD_CFG2, val, 1, REG_SET_25_BIT);

	jpu_set_reg(jpu_device->jpu_cvdr_base +
		JPGDEC_CVDR_AXI_WR_CFG1, val, 1, REG_SET_25_BIT);

	jpu_set_reg(jpu_device->jpu_cvdr_base +
		JPGDEC_CVDR_AXI_WR_CFG2, val, 1, REG_SET_25_BIT);
}

static void jpu_check_reg(struct jpu_data_type *jpu_device)
{
	int ret;

	ret = jpu_check_reg_state(jpu_device->jpu_cvdr_base +
		JPGDEC_CVDR_AXI_RD_CFG1, AXI_JPEG_NR_RD_STOP);
	jpu_err_if_cond(ret != 0, "fail to wait JPGDEC_CVDR_AXI_RD_CFG1\n");

	ret = jpu_check_reg_state(jpu_device->jpu_cvdr_base +
		JPGDEC_CVDR_AXI_RD_CFG2, AXI_JPEG_NR_RD_STOP);
	jpu_err_if_cond(ret != 0, "fail to wait JPGDEC_CVDR_AXI_RD_CFG2\n");

	ret = jpu_check_reg_state(jpu_device->jpu_cvdr_base +
		JPGDEC_CVDR_AXI_WR_CFG1, AXI_JPEG_NR_RD_STOP);
	jpu_err_if_cond(ret != 0, "fail to wait JPGDEC_CVDR_AXI_WR_CFG1\n");

	ret = jpu_check_reg_state(jpu_device->jpu_cvdr_base +
		JPGDEC_CVDR_AXI_WR_CFG2, AXI_JPEG_NR_RD_STOP);
	jpu_err_if_cond(ret != 0, "fail to wait JPGDEC_CVDR_AXI_WR_CFG2\n");
}

void jpu_dec_error_reset(struct jpu_data_type *jpu_device)
{
	int ret;
	if (jpu_device == NULL) {
		jpu_err("jpu_device is NULL\n");
		return;
	}

	/* step1 */
	if (jpu_device->jpu_support_platform == DSS_V510_CS) {
		jpu_set_reg(jpu_device->jpu_top_base + JPGDEC_PREF_STOP, 0x0, 1, 0);
	} else {
		jpu_set_reg(jpu_device->jpu_top_base + JPGDEC_CRG_CFG1,
			JPEGDEC_TPSRAM_2PRF_TRA, REG_SET_32_BIT, 0);
	}

	jpu_set_reg_cvdr(jpu_device, 0x1);

	/* step2 */
	if (jpu_device->jpu_support_platform == DSS_V510_CS) {
		ret = jpu_check_reg_state(jpu_device->jpu_top_base +
			JPGDEC_PREF_STOP, 0x10);

		jpu_err_if_cond(ret != 0, "fail to wait JPGDEC_PREF_STOP\n");
	} else {
		ret = jpu_check_reg_state(jpu_device->jpu_top_base +
			JPGDEC_RO_STATE, 0x2);

		jpu_err_if_cond(ret != 0, "fail to wait JPGDEC_RO_STATE\n");
	}
	jpu_check_reg(jpu_device);

	/* step3,read bit0 is 1 */
	jpu_set_reg(jpu_device->jpu_top_base + JPGDEC_CRG_CFG1, 1, 1, 0);
	ret = jpu_check_reg_state(jpu_device->jpu_top_base + JPGDEC_CRG_CFG1, 0x1);
	jpu_err_if_cond(ret != 0, "fail to wait JPGDEC_CRG_CFG1\n");

	/* step4 */
	jpu_set_reg_cvdr(jpu_device, 0x0);
	jpu_set_reg(jpu_device->jpu_top_base + JPGDEC_CRG_CFG1, 0x0, REG_SET_32_BIT, 0);
}

#if defined(CONFIG_DPU_FB_V600) || defined(CONFIG_DKMD_DPU_VERSION)
/* balitmore new smmu version, IPP only need to config smmuv3 tbu */
static int do_cfg_smmuv3_on_ippcomm(const struct jpu_data_type *jpu_device)
{
	int ret;
	jpu_check_null_return(jpu_device->jpgd_drv, -EINVAL);
	jpu_check_null_return(jpu_device->jpgd_drv->enable_smmu, -EINVAL);
	jpu_check_null_return(jpu_device->jpgd_drv->setsid_smmu, -EINVAL);
	jpu_check_null_return(jpu_device->jpgd_drv->set_pref, -EINVAL);

	ret = jpu_device->jpgd_drv->enable_smmu(jpu_device->jpgd_drv);
	if (ret != 0) {
		jpu_err("enable_smmu failed:%d\n", ret);
		return ret;
	}

	ret = jpu_device->jpgd_drv->setsid_smmu(jpu_device->jpgd_drv,
		jpu_device->jpgd_secadapt_prop.swid[0], MAX_SECADAPT_SWID_NUM,
		jpu_device->jpgd_secadapt_prop.sid, jpu_device->jpgd_secadapt_prop.ssid_ns);
	if (ret != 0) {
		jpu_err("setsid_smmu failed:%d\n", ret);
		if ((jpu_device->jpgd_drv->disable_smmu(jpu_device->jpgd_drv)) != 0)
			jpu_err("failed to disable smmu\n");

		return ret;
	}

	ret = jpu_device->jpgd_drv->set_pref(jpu_device->jpgd_drv,
		jpu_device->jpgd_secadapt_prop.swid[0], MAX_SECADAPT_SWID_NUM);
	if (ret != 0) {
		jpu_err("set_pref failed:%d\n", ret);
		if ((jpu_device->jpgd_drv->disable_smmu(jpu_device->jpgd_drv)) != 0)
			jpu_err("failed to disable smmu\n");
	}

	return ret;
}
#endif

static int jpu_smmu_on(struct jpu_data_type *jpu_device)
{
	uint32_t fama_ptw_msb;
	int ret = 0;

	jpu_debug("+\n");
#if defined(CONFIG_DPU_FB_V501) || defined(CONFIG_DPU_FB_V510)
	if (jpu_device->jpgd_drv && jpu_device->jpgd_drv->enable_smmu) {
		if ((jpu_device->jpgd_drv->enable_smmu(jpu_device->jpgd_drv)) != 0) {
			jpu_err("failed to enable smmu\n");
			ret = -EINVAL;
		}
		jpu_debug("-\n");
	}

#elif defined(CONFIG_DPU_FB_V600) || defined(CONFIG_DKMD_DPU_VERSION)
	ret = do_cfg_smmuv3_on_ippcomm(jpu_device);
	if (ret != 0)
		jpu_err("do_cfg_smmuv3_on_ippcomm failed:%d\n", ret);

	jpu_debug("-\n");
	jpu_info("do_cfg_smmuv3_on_ippcomm\n");
#else
	/*
	 * Set global mode:
	 * SMMU_SCR_S.glb_nscfg = 0x3
	 * SMMU_SCR_P.glb_prot_cfg = 0x0
	 * SMMU_SCR.glb_bypass = 0x0
	 */
	jpu_set_reg(jpu_device->jpu_smmu_base + SMMU_SCR, 0x0, 1, 0);

	/* for performance Ptw_mid: 0x1d, Ptw_pf: 0x1 */
	jpu_set_reg(jpu_device->jpu_smmu_base + SMMU_SCR, 0x1, REG_SET_4_BIT,
		REG_SET_16_BIT);
	jpu_set_reg(jpu_device->jpu_smmu_base + SMMU_SCR, 0x1D, REG_SET_6_BIT,
		REG_SET_20_BIT);

	/*
	 * Set interrupt mode:
	 * Clear all interrupt state: SMMU_INCLR_NS = 0xFF
	 * Enable interrupt: SMMU_INTMASK_NS = 0x0
	 */
	jpu_set_reg(jpu_device->jpu_smmu_base + SMMU_INTRAW_NS, 0xFF,
		REG_SET_32_BIT, 0);
	jpu_set_reg(jpu_device->jpu_smmu_base + SMMU_INTMASK_NS, 0x0,
		REG_SET_32_BIT, 0);

	/*
	 * Set non-secure pagetable addr:
	 * SMMU_CB_TTBR0 = non-secure pagetable address
	 * Set non-secure pagetable type:
	 * SMMU_CB_TTBCR.cb_ttbcr_des= 0x1 (long descriptor)
	 */
	if (jpu_domain_get_ttbr(jpu_device) == 0) {
		jpu_err("get ttbr failed\n");
		return -EINVAL;
	}
	jpu_set_reg(jpu_device->jpu_smmu_base + SMMU_CB_TTBR0,
		(uint32_t)jpu_domain_get_ttbr(jpu_device), REG_SET_32_BIT, 0);
	jpu_set_reg(jpu_device->jpu_smmu_base + SMMU_CB_TTBCR, 0x1, 1, 0);

	/* FAMA configuration */
	fama_ptw_msb = (jpu_domain_get_ttbr(jpu_device) >> SHIFT_32_BIT) &
		JPEGDEC_FAMA_PTW_MSB;
	jpu_set_reg(jpu_device->jpu_smmu_base + SMMU_FAMA_CTRL0, JPEGDEC_FAMA_PTW_MID,
		REG_SET_14_BIT, 0);
	jpu_set_reg(jpu_device->jpu_smmu_base + SMMU_FAMA_CTRL1,
		fama_ptw_msb, REG_SET_7_BIT, 0);
	jpu_debug("-\n");
#endif
	return ret;
}

#ifdef CONFIG_NO_USE_INTERFACE
static void jpu_set_power_up_reg(const struct jpu_data_type *jpu_device)
{
	uint32_t tmp;

	/* set_pu_media1_subsys */
	/* step1 module mtcmos on */
	SOC_PCTRL_PERI_STAT49_ADDR_MTCMOS_ON(jpu_device->peri_crg_base);
	udelay(100); /* 100 us */

	/* module MEM SD OFF */
	SOC_PCTRL_PERI_CTRL102_ADDR_NENSD_OFF(jpu_device->peri_crg_base);
	udelay(JPG_CLK_ENABLE_DELAY);

	/* step1.1 module unrst */
	SOC_PCTRL_PERI_STAT3_ADDR_UNRST(jpu_device->peri_crg_base);

	/* step2 module clk enable */
	SOC_PCTRL_RESOURCE1_UNLOCK_ADDR_CLK_ENABLE(jpu_device->peri_crg_base);
	SOC_SCTRL_SCPEREN4_ADDR_CLK_ENABLE(jpu_device->sctrl_base);
	SOC_PCTRL_PERI_CTRL15_ADDR_CLK_ENABLE(jpu_device->peri_crg_base);
	SOC_PCTRL_RESOURCE2_LOCK_ST_ADDR_CLK_ENABLE(jpu_device->peri_crg_base);
	udelay(JPG_CLK_ENABLE_DELAY);

	/* step3 module clk disable */
	SOC_PCTRL_RESOURCE1_LOCK_ST_ADDR_CLK_DISABLE(jpu_device->peri_crg_base);
	SOC_SCTRL_SCPERDIS4_ADDR_CLK_DISABLE(jpu_device->sctrl_base);
	SOC_PCTRL_PERI_CTRL16_ADDR_CLK_DISABLE(jpu_device->peri_crg_base);
	SOC_PCTRL_RESOURCE3_LOCK_ADDR_CLK_DISABLE(jpu_device->peri_crg_base);
	udelay(JPG_CLK_ENABLE_DELAY);

	/* step4 module iso disable */
	SOC_PCTRL_PERI_STAT47_ADDR_ISO_DISABLE(jpu_device->peri_crg_base);

	/* step5 memory rempair */
	tmp = (uint32_t)inp32(SOC_PCTRL_PERI_STAT64_ADDR(jpu_device->peri_crg_base));
	jpu_err_if_cond(((tmp & BIT(10)) == BIT(10)), /* 10 bit */
		"memory repaire failed, reg value is: 0x%x\n", tmp);
	udelay(JPG_MEMORY_REMPAIR_DELAY);

	/* step6 module unrst */
	SOC_PCTRL_PERI_STAT3_ADDR_UNRST1(jpu_device->peri_crg_base);

	/* step7 module clk enable */
	SOC_PCTRL_RESOURCE1_UNLOCK_ADDR_CLK_ENABLE(jpu_device->peri_crg_base);
	SOC_SCTRL_SCPEREN4_ADDR_CLK_ENABLE(jpu_device->sctrl_base);
	SOC_PCTRL_PERI_CTRL15_ADDR_CLK_ENABLE(jpu_device->peri_crg_base);
	SOC_PCTRL_RESOURCE2_LOCK_ST_ADDR_CLK_ENABLE(jpu_device->peri_crg_base);

	/* set_pu_vivobus */
	/* step2 module clk enable */
	SOC_MEDIA1_CRG_CLKDIV9_ADDR_ENABLE(jpu_device->media1_crg_base);
	outp32(jpu_device->media1_crg_base, MEDIA1_CLK_ENABLE);
	udelay(JPG_CLK_ENABLE_DELAY);

	/* step3 module clk disable */
	SOC_MEDIA1_CRG_PERDIS0_ADDR_CLK_DISABLE(jpu_device->media1_crg_base);
	udelay(JPG_CLK_ENABLE_DELAY);

	/* step 7 module clk enable */
	outp32(jpu_device->media1_crg_base, MEDIA1_CLK_ENABLE);
}

static int jpu_media1_power_up_others(
	const struct jpu_data_type *jpu_device)
{
	uint32_t tmp;

	/* set_pu_media1_subsys */
	SOC_PCTRL_PERI_STAT49_ADDR_MTCMOS_ON(jpu_device->peri_crg_base);
	udelay(100); /* 100us */

	SOC_PCTRL_PERI_STAT3_ADDR_UNRST(jpu_device->peri_crg_base);

	/* step2 module clk enable */
	SOC_PCTRL_RESOURCE1_UNLOCK_ADDR_ENABLE(jpu_device->peri_crg_base);
	SOC_SCTRL_SCPEREN4_ADDR_CLK_ENABLE(jpu_device->sctrl_base);
	SOC_PCTRL_PERI_CTRL15_ADDR_CLK_ENABLE(jpu_device->peri_crg_base);
	udelay(JPG_CLK_ENABLE_DELAY);

	/* step3 module clk disable */
	SOC_PCTRL_RESOURCE1_LOCK_ST_ADDR_DISABLE(jpu_device->peri_crg_base);
	SOC_SCTRL_SCPERDIS4_ADDR_CLK_DISABLE(jpu_device->sctrl_base);
	SOC_PCTRL_PERI_CTRL16_ADDR_CLK_DISABLE(jpu_device->peri_crg_base);
	udelay(JPG_CLK_ENABLE_DELAY);

	/* step4 module iso disable */
	SOC_PCTRL_PERI_STAT47_ADDR_ISO_DISABLE(jpu_device->peri_crg_base);

	/* step5 memory rempair */
	udelay(JPG_MEMORY_REMPAIR_DELAY);

	/* step6 module unrst */
	SOC_PCTRL_PERI_STAT3_ADDR_UNRST1(jpu_device->peri_crg_base);

	/* step7 module clk enable */
	SOC_PCTRL_RESOURCE1_UNLOCK_ADDR_ENABLE(jpu_device->peri_crg_base);
	SOC_SCTRL_SCPEREN4_ADDR_CLK_ENABLE(jpu_device->sctrl_base);
	SOC_PCTRL_PERI_CTRL15_ADDR_CLK_ENABLE(jpu_device->peri_crg_base);

	/* set_pu_vivobus */
	/* step2 module clk enable */
	SOC_MEDIA1_CRG_CLKDIV9_ADDR_ENABLE(jpu_device->media1_crg_base);
	outp32(jpu_device->media1_crg_base, MEDIA1_CLK_ENABLE);
	udelay(JPG_CLK_ENABLE_DELAY);

	/* step3 module clk disable */
	SOC_MEDIA1_CRG_PERDIS0_ADDR_DISABLE(jpu_device->media1_crg_base);
	udelay(JPG_CLK_ENABLE_DELAY);

	/* module clk enable */
	outp32(jpu_device->media1_crg_base, MEDIA1_OTHERS_CLK_ENABLE);

	/* stepbus idle clear */
	SOC_PMCTRL_NOC_POWER_IDLEREQ_0_ADDR_CLEAR(jpu_device->pmctrl_base);
	tmp = (uint32_t)inp32(SOC_PMCTRL_NOC_POWER_IDLEACK_0_ADDR(jpu_device->pmctrl_base));

	jpu_err_if_cond(((tmp | 0xFFFF7FFF) == 0xFFFFFFFF),
		"vivobus fail to clear bus idle tmp 0x%x\n", tmp);
	udelay(JPG_CLK_ENABLE_DELAY);

	tmp = (uint32_t)inp32(jpu_device->pmctrl_base + 0x388);

	udelay(JPG_CLK_ENABLE_DELAY);
	jpu_err_if_cond(((tmp | 0xFFFF7FFF) == 0xFFFFFFFF),
		"vivobus fail to clear bus idle 0x388 tmp 0x%x\n", tmp);
	return 0;
}

static int jpu_power_up_v501(const struct jpu_data_type *jpu_device)
{
	/* set_pu_isp */
	uint32_t tmp;

	/* step 1 module mtcmos on */
	SOC_PCTRL_PERI_STAT49_ADDR_MTCMOS_ON_V501(jpu_device->peri_crg_base);
	udelay(100); /* 100us */

	/* module sd off */
	SOC_PCTRL_PERI_CTRL102_ADDR_SD_OFF_V501(jpu_device->peri_crg_base);
	udelay(JPG_CLK_ENABLE_DELAY);

	/* step2 module unrst */
	SOC_MEDIA1_CRG_PERRSTDIS0_ADDR_UNRST(jpu_device->media1_crg_base);

	/* step3 module clock enable */
	SOC_MEDIA1_CRG_CLKDIV9_ADDR_ENABLE_V501(jpu_device->media1_crg_base);
	SOC_MEDIA1_CRG_PEREN1_ADDR_ENABLE(jpu_device->media1_crg_base);
	SOC_MEDIA1_CRG_PEREN0_ADDR_ENABLE(jpu_device->media1_crg_base);
	udelay(JPG_CLK_ENABLE_DELAY);

	/* step4 module clk disable */
	SOC_MEDIA1_CRG_PERDIS1_ADDR_DISABLE(jpu_device->media1_crg_base);
	udelay(JPG_CLK_ENABLE_DELAY);

	/* step5 module iso disable */
	SOC_PCTRL_PERI_STAT47_ADDR_DISABLE_V501(jpu_device->peri_crg_base);

	/* step6 memory rempair, BIT(11) */
	tmp = inp32(SOC_PCTRL_PERI_STAT64_ADDR(jpu_device->peri_crg_base));
	jpu_err_if_cond((tmp & BIT(11)) == BIT(11), /* 11bit */
		"memory repaire failed, reg value is: 0x%x\n", tmp);
	udelay(JPG_MEMORY_REMPAIR_DELAY);

	/* step7 module unrst */
	SOC_MEDIA1_CRG_PERRSTDIS0_ADDR_UNRST1(jpu_device->media1_crg_base);
	SOC_MEDIA1_CRG_PERRSTDIS1_ADDR_UNRST(jpu_device->media1_crg_base);

	/* step8 module clk enable */
	SOC_MEDIA1_CRG_PEREN1_ADDR_ENABLE1(jpu_device->media1_crg_base);

	/* step9 bus idle clear */
	SOC_PMCTRL_NOC_POWER_IDLEREQ_0_ADDR_V501(jpu_device->pmctrl_base);

	tmp = (uint32_t)inp32(SOC_PMCTRL_NOC_POWER_IDLEACK_0_ADDR(jpu_device->pmctrl_base));
	jpu_err_if_cond((tmp & BIT(5)) == 0x0, /* 5bit */
		"fail to isp clear bus idle tmp 0x%x\n", tmp);
	udelay(JPG_CLK_ENABLE_DELAY);

	tmp = (uint32_t)inp32(SOC_PMCTRL_NOC_POWER_IDLE_0_ADDR(jpu_device->pmctrl_base));
	jpu_err_if_cond((tmp & BIT(5)) == 0x0, /* 5bit */
		"fail to isp clear bus idle 0x388 tmp 0x%x\n", tmp);
	udelay(JPG_CLK_ENABLE_DELAY);

	/* step10 isp_rst_n /ISP_infra/ISP_SRT */
	SOC_MEDIA1_CRG_PERDIS0_ADDR_V501(jpu_device->media1_crg_base);
	SOC_MEDIA1_CRG_PERRSTDIS_ISP_SEC_ADDR_V501(jpu_device->media1_crg_base);
	SOC_MEDIA1_CRG_PEREN0_ADDR_ISP_SRT(jpu_device->media1_crg_base);
	return 0;
}

static int jpu_power_up_v500(const struct jpu_data_type *jpu_device)
{
	/* set_pu_isp */
	uint32_t tmp;

	/* step 1 module mtcmos on */
	SOC_PCTRL_PERI_STAT49_ADDR_MTCMOS_ON_V501(jpu_device->peri_crg_base);
	udelay(100); /* 100us */

	/* module sd off */
	SOC_PCTRL_PERI_CTRL102_ADDR_SD_OFF_V501(jpu_device->peri_crg_base);
	udelay(JPG_CLK_ENABLE_DELAY);

	/* step1.1 module unrst */
	SOC_MEDIA1_CRG_PERRSTDIS0_ADDR_UNRST(jpu_device->media1_crg_base);

	/* step2 module clock enable */
	SOC_MEDIA1_CRG_CLKDIV9_ADDR_ENABLE_V500(jpu_device->media1_crg_base);
	SOC_MEDIA1_CRG_PEREN1_ADDR_ENABLE_V500(jpu_device->media1_crg_base);
	udelay(JPG_CLK_ENABLE_DELAY);

	/* step3 module clk disable */
	SOC_MEDIA1_CRG_PERDIS1_ADDR_DISABLE_V500(jpu_device->media1_crg_base);
	udelay(JPG_CLK_ENABLE_DELAY);

	/* step4 module iso disable */
	SOC_PCTRL_PERI_STAT47_ADDR_DISABLE_V501(jpu_device->peri_crg_base);

	/* step6 module unrst */
	SOC_MEDIA1_CRG_PERRSTDIS0_ADDR_UNRST_V500(jpu_device->media1_crg_base);
	SOC_MEDIA1_CRG_PERRSTDIS1_ADDR_UNRST(jpu_device->media1_crg_base);

	/* step7 module clk enable */
	SOC_MEDIA1_CRG_PEREN1_ADDR_ENABLE1_V500(jpu_device->media1_crg_base);

	/* step8 bus idle clear */
	SOC_PMCTRL_PERI_CTRL0_ADDR_V500(jpu_device->pmctrl_base);

	tmp = (uint32_t)inp32(SOC_PMCTRL_PERI_CTRL3_ADDR(jpu_device->pmctrl_base));
	jpu_err_if_cond((tmp & BIT(5)) == 0x0, /* 5bit */
		"fail to isp clear bus idle 0x34C tmp 0x%x\n", tmp);
	udelay(JPG_CLK_ENABLE_DELAY);

	tmp = (uint32_t)inp32(SOC_PMCTRL_PERI_STAT0_ADDR(jpu_device->pmctrl_base));
	jpu_err_if_cond((tmp & BIT(5)) == 0x0, /* 5bit */
		"fail to isp clear bus idle 0x360 tmp 0x%x\n", tmp);
	udelay(JPG_CLK_ENABLE_DELAY);

	return 0;
}

static int jpu_power_up_others(const struct jpu_data_type *jpu_device)
{
	uint32_t tmp;

	/* set_pu_isp */
	/* step 1 module mtcmos on */
	SOC_PCTRL_PERI_STAT49_ADDR_MTCMOS_ON_V501(jpu_device->peri_crg_base);
	udelay(100); /* 100 us */

	SOC_MEDIA1_CRG_PERRSTDIS0_ADDR_UNRST(jpu_device->media1_crg_base);

	/* step2 module clock enable */
	SOC_MEDIA1_CRG_CLKDIV9_ADDR_ENABLE_V500(jpu_device->media1_crg_base);
	SOC_MEDIA1_CRG_PEREN1_ADDR_ENABLE_V500(jpu_device->media1_crg_base);
	udelay(JPG_CLK_ENABLE_DELAY);

	/* step3 module clk disable 3 */
	SOC_MEDIA1_CRG_PERDIS1_ADDR_DISABLE_V500(jpu_device->media1_crg_base);
	udelay(JPG_CLK_ENABLE_DELAY);

	/* step4 module iso disable */
	SOC_PCTRL_PERI_STAT47_ADDR_DISABLE_V501(jpu_device->peri_crg_base);

	/* step5 memory rempair */
	udelay(JPG_MEMORY_REMPAIR_DELAY);

	/* step6 module unrst */
	SOC_MEDIA1_CRG_PERRSTDIS0_ADDR_UNRST_V500(jpu_device->media1_crg_base);
	SOC_MEDIA1_CRG_PERRSTDIS1_ADDR_UNRST(jpu_device->media1_crg_base);

	/* step7 module clk enable */
	SOC_MEDIA1_CRG_PEREN1_ADDR_ENABLE1_V500(jpu_device->media1_crg_base);

	/* step8 bus idle clear */
	SOC_PMCTRL_NOC_POWER_IDLEREQ_0_ADDR_V501(jpu_device->pmctrl_base);
	mdelay(JPG_CLK_ENABLE_DELAY);

	tmp = (uint32_t)inp32(SOC_PMCTRL_NOC_POWER_IDLEACK_0_ADDR(jpu_device->pmctrl_base));
	jpu_err_if_cond((tmp | 0xFFFFFFDF) == 0xFFFFFFFF,
		"fail to isp clear bus idle tmp 0x%x\n", tmp);
	mdelay(JPG_CLK_ENABLE_DELAY);

	tmp = (uint32_t)inp32(SOC_PMCTRL_NOC_POWER_IDLE_0_ADDR(jpu_device->pmctrl_base));
	jpu_err_if_cond((tmp | 0xFFFFFFDF) == 0xFFFFFFFF,
		"fail to isp clear bus idle 0x388 tmp 0x%x\n", tmp);
	mdelay(JPG_CLK_ENABLE_DELAY);

	return 0;
}

static int jpu_media1_power_up_v501v500(
	const struct jpu_data_type *jpu_device)
{
	uint32_t tmp;

	jpu_set_power_up_reg(jpu_device);
	if (jpu_device->jpu_support_platform == DSS_V501) {
		/* step 8 bus idle clear */
		SOC_PMCTRL_NOC_POWER_IDLEREQ_0_ADDR_CLEAR(jpu_device->pmctrl_base);
		tmp = (uint32_t)inp32(SOC_PMCTRL_NOC_POWER_IDLEACK_0_ADDR(jpu_device->pmctrl_base));
		jpu_err_if_cond(((tmp & BIT(15)) == 0x0), /* 15bit */
			"vivobus fail to clear bus idle 0x384 tmp 0x%x\n", tmp);
		udelay(JPG_CLK_ENABLE_DELAY);

		tmp = (uint32_t)inp32(SOC_PMCTRL_NOC_POWER_IDLE_0_ADDR(jpu_device->pmctrl_base));
		jpu_err_if_cond(((tmp & BIT(15)) == 0x0), /* 15bit */
			"vivobus fail to clear bus idle 0x388 tmp 0x%x\n", tmp);
		udelay(JPG_CLK_ENABLE_DELAY);
	}

	if (jpu_device->jpu_support_platform == DSS_V500) {
		/* step 8 bus idle clear */
		SOC_PMCTRL_PERI_CTRL0_ADDR_CLEAR(jpu_device->pmctrl_base);
		tmp = (uint32_t)inp32(SOC_PMCTRL_PERI_CTRL3_ADDR(jpu_device->pmctrl_base));
		jpu_err_if_cond((tmp & BIT(15)) == 0x0, /* 15bit */
			"vivobus fail to clear bus idle 0x34C tmp 0x%x\n", tmp);
		udelay(JPG_CLK_ENABLE_DELAY);

		tmp = (uint32_t)inp32(SOC_PMCTRL_PERI_STAT0_ADDR(jpu_device->pmctrl_base));
		jpu_err_if_cond((tmp & BIT(15)) == 0x0, /* 15bit */
			"vivobus fail to clear bus idle 0x360 tmp 0x%x\n", tmp);
		udelay(JPG_CLK_ENABLE_DELAY);
	}
	return 0;
}
#endif

#if (!defined CONFIG_DPU_FB_V600) && (!defined CONFIG_DKMD_DPU_VERSION)
static int jpu_clk_try_low_freq(const struct jpu_data_type *jpu_device)
{
	int ret;

	ret = jpeg_dec_set_rate(jpu_device->jpg_func_clk, JPGDEC_LOWLEVEL_CLK_RATE);
	if (ret != 0) {
		jpu_err("jpg_func_clk set clk failed, error = %d\n", ret);
		return -EINVAL;
	}

	ret = jpeg_dec_clk_prepare_enable(jpu_device->jpg_func_clk);
	if (ret != 0) {
		jpu_err("jpg_func_clk clk_prepare failed, error = %d\n", ret);
		return -EINVAL;
	}

	return ret;
}

int jpu_clk_enable(struct jpu_data_type *jpu_device)
{
	int ret;

	jpu_debug("+\n");
	jpu_check_null_return(jpu_device->jpg_func_clk, -EINVAL);

	jpu_debug("jpg func clk default rate is: %ld\n",
		JPGDEC_DEFALUTE_CLK_RATE);

	ret = jpeg_dec_set_rate(jpu_device->jpg_func_clk, JPGDEC_DEFALUTE_CLK_RATE);
	if (ret != 0) {
		jpu_err("jpg_func_clk set clk failed, error = %d\n", ret);
		goto TRY_LOW_FREQ;
	}

	ret = jpeg_dec_clk_prepare_enable(jpu_device->jpg_func_clk);
	if (ret != 0) {
		jpu_err("jpg_func_clk clk_prepare failed, error = %d\n", ret);
		goto TRY_LOW_FREQ;
	}

	jpu_debug("-\n");
	return ret;

TRY_LOW_FREQ:
	return jpu_clk_try_low_freq(jpu_device);
}

int jpu_clk_disable(const struct jpu_data_type *jpu_device)
{
	int ret;

	jpu_debug("+\n");
	jpu_check_null_return(jpu_device->jpg_func_clk, -EINVAL);

	jpeg_dec_clk_disable_unprepare(jpu_device->jpg_func_clk);

	ret = jpeg_dec_set_rate(jpu_device->jpg_func_clk, JPGDEC_POWERDOWN_CLK_RATE);
	if (ret != 0) {
		jpu_err("fail to set power down rate, ret = %d\n", ret);
		return -EINVAL;
	}

	jpu_debug("-\n");
	return ret;
}

static int jpu_media1_regulator_disable(
	const struct jpu_data_type *jpu_device)
{
	int ret;

	jpu_debug("+\n");
#ifdef CONFIG_NO_USE_INTERFACE
	ret = 0;
#else
	ret = regulator_disable(jpu_device->media1_regulator);
	if (ret != 0)
		jpu_err("jpu media1 regulator_disable failed, error = %d\n", ret);
#endif

	jpu_debug("-\n");
	return ret;
}

static int jpu_media1_regulator_enable(struct jpu_data_type *jpu_device)
{
	int ret = 0;

	jpu_debug("+\n");
#ifdef CONFIG_NO_USE_INTERFACE
	bool cond = ((jpu_device->jpu_support_platform == DSS_V501) ||
		(jpu_device->jpu_support_platform == DSS_V500));
	if (cond)
		jpu_media1_power_up_v501v500(jpu_device);
	else
		jpu_media1_power_up_others(jpu_device);
#else
	ret = regulator_enable(jpu_device->media1_regulator);
	if (ret != 0)
		jpu_err("jpu media1_regulator failed, error = %d\n", ret);
#endif

	jpu_debug("-\n");
	return ret;
}

static int jpu_jpu_regulator_enable(struct jpu_data_type *jpu_device)
{
	int ret = 0;

	jpu_debug("+\n");
#ifdef CONFIG_NO_USE_INTERFACE
	if (jpu_device->jpu_support_platform == DSS_V501) {
		jpu_power_up_v501(jpu_device);
	} else if (jpu_device->jpu_support_platform == DSS_V500) {
		jpu_power_up_v500(jpu_device);
	} else {
		jpu_power_up_others(jpu_device);
	}
#else
	ret = regulator_enable(jpu_device->jpu_regulator);
	if (ret != 0)
		jpu_err("jpu regulator_enable failed, error = %d\n", ret);
#endif

	jpu_debug("-\n");
	return ret;
}
#endif

static int jpu_on_v600(struct jpu_data_type *jpu_device)
{
	int ret;
#if (!defined CONFIG_DPU_FB_V600) && (!defined CONFIG_DKMD_DPU_VERSION)
	/* step 1 mediasubsys enable */
	ret = jpu_media1_regulator_enable(jpu_device);
	if (ret != 0) {
		jpu_err("media1_regulator_enable fail\n");
		return ret;
	}

	/* step 2 clk_enable */
	ret = jpu_clk_enable(jpu_device);
	if (ret != 0) {
		jpu_err("jpu_clk_enable fail\n");
		jpu_media1_regulator_disable(jpu_device);
		return ret;
	}

	/* step 3 regulator_enable ispsubsys */
	ret = jpu_jpu_regulator_enable(jpu_device);
	if (ret != 0) {
		jpu_err("jpu_regulator_enable fail\n");
		jpu_clk_disable(jpu_device);
		jpu_media1_regulator_disable(jpu_device);
		return ret;
	}
#endif

#if defined(CONFIG_DPU_FB_V600) || defined(CONFIG_DKMD_DPU_VERSION)
	if ((jpu_device->jpgd_drv == NULL) || (jpu_device->jpgd_drv->power_up == NULL) ||
		(jpu_device->jpgd_drv->set_jpgclk_rate == NULL)) {
		jpu_err("jpu_device->jpgd_drv is NULL\n");
		return -EINVAL;
	}

	ret = jpu_device->jpgd_drv->power_up(jpu_device->jpgd_drv);
	if (ret != 0) {
		jpu_info("power_up failed:%d\n", ret);
		return ret;
	}

	/* set clk rate 640M */
	ret = jpu_device->jpgd_drv->set_jpgclk_rate(jpu_device->jpgd_drv, CLK_RATE_TURBO);
	if (ret != 0) {
		jpu_err("set_jpgclk_rate fail, change to set low temp rate %d\n", ret);
		/* low temp, when clk set fail, set clk rate to 480M */
		ret = jpu_device->jpgd_drv->set_jpgclk_rate(jpu_device->jpgd_drv, CLK_RATE_NORMAL);
		if (ret != 0) {
			jpu_err("set_jpgclk_rate fail %d\n", ret);
			return ret;
		}
	}
#endif

	return 0;
}

int jpu_on(struct jpu_data_type *jpu_device)
{
	int ret = 0;

	jpu_check_null_return(jpu_device, -EINVAL);
	if (jpu_device->power_on) {
		jpu_info("jpu_device has already power_on\n");
		return ret;
	}

	ret = jpu_on_v600(jpu_device);
	if (ret != 0)
		return ret;

	/* step 4 jpeg decoder inside clock enable */
	outp32(jpu_device->jpu_top_base + JPGDEC_CRG_CFG0, 0x1);

	if (jpu_device->jpu_support_platform == DSS_V400)
		outp32(jpu_device->jpu_top_base + JPGDEC_MEM_CFG, 0x02605550);

	ret = jpu_smmu_on(jpu_device);
	if (ret != 0) {
		jpu_err("jpu smmu on failed\n");
		return ret;
	}

	jpu_dec_interrupt_mask(jpu_device);
	jpu_dec_interrupt_clear(jpu_device);

	ret = jpgdec_enable_irq(jpu_device);
	if (ret != 0) {
		jpu_err("jpu_irq_enable failed\n");
#if defined(CONFIG_DPU_FB_V600) || defined(CONFIG_DKMD_DPU_VERSION)
		/* V600 should disable smmu */
		if ((jpu_device->jpgd_drv->disable_smmu(jpu_device->jpgd_drv)) != 0)
			jpu_err("failed to disable smmu\n");
#endif
		return -EINVAL;
	}

	jpu_dec_interrupt_unmask(jpu_device);
	jpu_device->power_on = true;

	return ret;
}

static int jpu_off_v600(struct jpu_data_type *jpu_device)
{
	int ret;
#if (!defined CONFIG_DPU_FB_V600) && (!defined CONFIG_DKMD_DPU_VERSION)
	/* ispsubsys regulator disable */
	ret = jpu_regulator_disable(jpu_device);
	if (ret != 0) {
		jpu_err("jpu_regulator_disable failed\n");
		return -EINVAL;
	}

	/* clk disable */
	ret = jpu_clk_disable(jpu_device);
	if (ret != 0) {
		jpu_err("jpu_clk_disable failed\n");
		return -EINVAL;
	}

	/* media disable */
	ret = jpu_media1_regulator_disable(jpu_device);
	if (ret != 0) {
		jpu_err("jpu_media1_regulator_disable failed\n");
		return -EINVAL;
	}
#endif

#if defined(CONFIG_DPU_FB_V600) || defined(CONFIG_DKMD_DPU_VERSION)
	if ((jpu_device->jpgd_drv == NULL) || (jpu_device->jpgd_drv->set_jpgclk_rate == NULL) ||
		(jpu_device->jpgd_drv->power_dn == NULL)) {
		jpu_err("jpu_device->jpgd_drv is NULL\n");
		return -EINVAL;
	}

	/* set clk rate 104M */
	ret = jpu_device->jpgd_drv->set_jpgclk_rate(jpu_device->jpgd_drv, CLK_RATE_OFF);
	if (ret != 0) {
		jpu_err("set_jpgclk_rate fail %d\n", ret);
		return ret;
	}

	ret = jpu_device->jpgd_drv->power_dn(jpu_device->jpgd_drv);
	if (ret != 0)
		jpu_err("Failed: fail to jpu powerdown%d\n", ret);
#endif
	return ret;
}

int jpu_off(struct jpu_data_type *jpu_device)
{
	int ret = 0;

	jpu_check_null_return(jpu_device, -EINVAL);
	if (!jpu_device->power_on) {
		jpu_debug("jpu_device has already power off\n");
		return ret;
	}

#if defined(CONFIG_DPU_FB_V501) || defined(CONFIG_DPU_FB_V510) || \
	defined(CONFIG_DPU_FB_V600) || defined(CONFIG_DKMD_DPU_VERSION)
	if (jpu_device->jpgd_drv && jpu_device->jpgd_drv->disable_smmu) {
		if ((jpu_device->jpgd_drv->disable_smmu(jpu_device->jpgd_drv)) != 0)
			jpu_err("failed to disable smmu\n");
	}
#endif

	jpu_dec_interrupt_mask(jpu_device);
	jpgdec_disable_irq(jpu_device);

	/* jpeg decoder inside clock disable */
	outp32(jpu_device->jpu_top_base + JPGDEC_CRG_CFG0, 0x0);
	ret = jpu_off_v600(jpu_device);
	if (ret != 0)
		return ret;
	jpu_device->power_on = false;

	return ret;
}

#pragma GCC diagnostic pop
