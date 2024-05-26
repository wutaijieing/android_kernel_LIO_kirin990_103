// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Universal Flash Storage Host controller Platform bus based glue driver
 * Copyright (C) 2011-2013 Samsung India Software Operations
 *
 * Authors:
 *	Santosh Yaraganavi <santosh.sy@samsung.com>
 *	Vinayak Holikatti <h.vinayak@samsung.com>
 */

#define pr_fmt(fmt) "ufshcd :" fmt

#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/blkdev.h>
#include <linux/blk-mq.h>
#include <linux/bootdevice.h>
#include <platform_include/basicplatform/linux/rdr_platform.h>

#include "ufshcd.h"
#include "ufshcd-pltfrm.h"
#include "unipro.h"
#include "ufs-hpb.h"
#include "hufs_plat.h"
#include "hufs_hcd.h"

static struct ufs_hba_variant_ops *get_variant_ops(struct device *dev)
{
	if (dev->of_node) {
		const struct of_device_id *match;

		match = of_match_node(ufs_of_match, dev->of_node);
		if (match)
			return (struct ufs_hba_variant_ops *)match->data;
	}

	return NULL;
}

#define UFSHCD_DEFAULT_LANES_PER_DIRECTION		2

static int ufshcd_parse_clock_info(struct ufs_hba *hba)
{
	int ret = 0;
	int cnt;
	int i;
	struct device *dev = hba->dev;
	struct device_node *np = dev->of_node;
	char *name;
	u32 *clkfreq = NULL;
	struct ufs_clk_info *clki;
	int len = 0;
	size_t sz = 0;

	if (!np)
		goto out;

	cnt = of_property_count_strings(np, "clock-names");
	if (!cnt || (cnt == -EINVAL)) {
		dev_info(dev, "%s: Unable to find clocks, assuming enabled\n",
				__func__);
	} else if (cnt < 0) {
		dev_err(dev, "%s: count clock strings failed, err %d\n",
				__func__, cnt);
		ret = cnt;
	}

	if (cnt <= 0)
		goto out;

	if (!of_get_property(np, "freq-table-hz", &len)) {
		dev_info(dev, "freq-table-hz property not specified\n");
		goto out;
	}

	if (len <= 0)
		goto out;

	sz = len / sizeof(*clkfreq);
	if (sz != 2 * cnt) {
		dev_err(dev, "%s len mismatch\n", "freq-table-hz");
		ret = -EINVAL;
		goto out;
	}

	clkfreq = devm_kcalloc(dev, sz, sizeof(*clkfreq),
			       GFP_KERNEL);
	if (!clkfreq) {
		ret = -ENOMEM;
		goto out;
	}

	ret = of_property_read_u32_array(np, "freq-table-hz",
			clkfreq, sz);
	if (ret && (ret != -EINVAL)) {
		dev_err(dev, "%s: error reading array %d\n",
				"freq-table-hz", ret);
		return ret;
	}

	for (i = 0; i < sz; i += 2) {
		ret = of_property_read_string_index(np,
				"clock-names", i/2, (const char **)&name);
		if (ret)
			goto out;

		clki = devm_kzalloc(dev, sizeof(*clki), GFP_KERNEL);
		if (!clki) {
			ret = -ENOMEM;
			goto out;
		}

		clki->min_freq = clkfreq[i];
		clki->max_freq = clkfreq[i+1];
		clki->name = kstrdup(name, GFP_KERNEL);
		if (!strcmp(name, "ref_clk"))
			clki->keep_link_active = true;
		dev_dbg(dev, "%s: min %u max %u name %s\n", "freq-table-hz",
				clki->min_freq, clki->max_freq, clki->name);
		list_add_tail(&clki->list, &hba->clk_list_head);
	}
out:
	return ret;
}

#define MAX_PROP_SIZE 32
static int ufshcd_populate_vreg(struct device *dev, const char *name,
		struct ufs_vreg **out_vreg)
{
	int ret = 0;
	char prop_name[MAX_PROP_SIZE];
	struct ufs_vreg *vreg = NULL;
	struct device_node *np = dev->of_node;

	if (!np) {
		dev_err(dev, "%s: non DT initialization\n", __func__);
		goto out;
	}

	snprintf(prop_name, MAX_PROP_SIZE, "%s-supply", name);
	if (!of_parse_phandle(np, prop_name, 0)) {
		dev_info(dev, "%s: Unable to find %s regulator, assuming enabled\n",
				__func__, prop_name);
		goto out;
	}

	vreg = devm_kzalloc(dev, sizeof(*vreg), GFP_KERNEL);
	if (!vreg)
		return -ENOMEM;

	vreg->name = kstrdup(name, GFP_KERNEL);

	snprintf(prop_name, MAX_PROP_SIZE, "%s-max-microamp", name);
	if (of_property_read_u32(np, prop_name, &vreg->max_uA)) {
		dev_info(dev, "%s: unable to find %s\n", __func__, prop_name);
		vreg->max_uA = 0;
	}
out:
	if (!ret)
		*out_vreg = vreg;
	return ret;
}

/**
 * ufshcd_parse_regulator_info - get regulator info from device tree
 * @hba: per adapter instance
 *
 * Get regulator info from device tree for vcc, vccq, vccq2 power supplies.
 * If any of the supplies are not defined it is assumed that they are always-on
 * and hence return zero. If the property is defined but parsing is failed
 * then return corresponding error.
 */
static int ufshcd_parse_regulator_info(struct ufs_hba *hba)
{
	int err;
	struct device *dev = hba->dev;
	struct ufs_vreg_info *info = &hba->vreg_info;

	err = ufshcd_populate_vreg(dev, "vdd-hba", &info->vdd_hba);
	if (err)
		goto out;

	err = ufshcd_populate_vreg(dev, "vcc", &info->vcc);
	if (err)
		goto out;

	err = ufshcd_populate_vreg(dev, "vccq", &info->vccq);
	if (err)
		goto out;

	err = ufshcd_populate_vreg(dev, "vccq2", &info->vccq2);
out:
	return err;
}

#ifdef CONFIG_PM
/**
 * ufshcd_pltfrm_suspend - suspend power management function
 * @dev: pointer to device handle
 *
 * Returns 0 if successful
 * Returns non-zero otherwise
 */
int ufshcd_pltfrm_suspend(struct device *dev)
{
	int ret;
	struct ufs_hba *hba = (struct ufs_hba *)dev_get_drvdata(dev);

	dev_info(dev, "%s:%d ++\n", __func__, __LINE__);
#ifdef CONFIG_MAS_BLK
	if (hba->host->queue_quirk_flag &
		SHOST_QUIRK(SHOST_QUIRK_SCSI_QUIESCE_IN_LLD)) {
		blk_generic_freeze(&(hba->host->tag_set.lld_func),
				BLK_LLD, true);
		__set_quiesce_for_each_device(hba->host);
	}
#endif
	ret = ufshcd_system_suspend(dev_get_drvdata(dev));
	if (ret) {
#ifdef CONFIG_MAS_BLK
		if (hba->host->queue_quirk_flag &
			SHOST_QUIRK(SHOST_QUIRK_SCSI_QUIESCE_IN_LLD)) {
			__clr_quiesce_for_each_device(hba->host);
			blk_generic_freeze(&(hba->host->tag_set.lld_func),
					BLK_LLD, false);
		}
#endif
	}
	dev_info(dev, "%s:%d ret:%d--\n", __func__, __LINE__, ret);
	return ret;
}
EXPORT_SYMBOL_GPL(ufshcd_pltfrm_suspend);

/**
 * ufshcd_pltfrm_resume - resume power management function
 * @dev: pointer to device handle
 *
 * Returns 0 if successful
 * Returns non-zero otherwise
 */
int ufshcd_pltfrm_resume(struct device *dev)
{
	int ret;
	struct ufs_hba *hba = (struct ufs_hba *)dev_get_drvdata(dev);

	dev_info(dev, "%s:%d ++\n", __func__, __LINE__);
	ret = ufshcd_system_resume(dev_get_drvdata(dev));
#ifdef CONFIG_MAS_BLK
	if (hba->host->queue_quirk_flag &
		SHOST_QUIRK(SHOST_QUIRK_SCSI_QUIESCE_IN_LLD)) {
		__clr_quiesce_for_each_device(hba->host);
		blk_generic_freeze(&(hba->host->tag_set.lld_func),
				BLK_LLD, false);
	}
#endif
	hba->in_shutdown = false;
	dev_info(dev, "%s:%d ret:%d--\n", __func__, __LINE__, ret);
	return ret;
}
EXPORT_SYMBOL_GPL(ufshcd_pltfrm_resume);

static int ufshcd_pltfrm_restore(struct device *dev)
{
	struct ufs_hba *hba = (struct ufs_hba *)dev_get_drvdata(dev);
	dev_info(dev, "%s:%d ++\n", __func__, __LINE__);

	hba->wlun_dev_clr_ua = true;

	dev_info(dev, "%s:%d --\n", __func__, __LINE__);
	return 0;
}

int ufshcd_pltfrm_runtime_suspend(struct device *dev)
{
	return ufshcd_runtime_suspend((struct ufs_hba *)dev_get_drvdata(dev));
}
EXPORT_SYMBOL_GPL(ufshcd_pltfrm_runtime_suspend);

int ufshcd_pltfrm_runtime_resume(struct device *dev)
{
	return ufshcd_runtime_resume((struct ufs_hba *)dev_get_drvdata(dev));
}
EXPORT_SYMBOL_GPL(ufshcd_pltfrm_runtime_resume);

int ufshcd_pltfrm_runtime_idle(struct device *dev)
{
	return ufshcd_runtime_idle((struct ufs_hba *)dev_get_drvdata(dev));
}
EXPORT_SYMBOL_GPL(ufshcd_pltfrm_runtime_idle);

#else /* !CONFIG_PM */
#define ufshcd_pltfrm_suspend	NULL
#define ufshcd_pltfrm_resume	NULL
#define ufshcd_pltfrm_restore	NULL
#define ufshcd_pltfrm_runtime_suspend	NULL
#define ufshcd_pltfrm_runtime_resume	NULL
#define ufshcd_pltfrm_runtime_idle	NULL
#endif /* CONFIG_PM */

#define SHUTDOWN_TIMEOUT (32 * 1000)
void ufshcd_pltfrm_shutdown(struct platform_device *pdev)
{
	struct ufs_hba *hba = (struct ufs_hba *)platform_get_drvdata(pdev);
	unsigned long timeout = SHUTDOWN_TIMEOUT;

	dev_err(&pdev->dev, "%s ++ lrb_in_use 0x%llx\n", __func__, hba->lrb_in_use);
	hba->in_shutdown = true;
	suspend_wait_hpb(hba);

#ifdef CONFIG_MAS_BLK
	if (hba->host->queue_quirk_flag &
		SHOST_QUIRK(SHOST_QUIRK_HUFS_MQ)) {
		blk_generic_freeze(&(hba->host->tag_set.lld_func), BLK_LLD, true);
		/*
		 * set all scsi device state to quiet to
		 * forbid io form blk level
		 */
		__set_quiesce_for_each_device(hba->host);
	}
#endif

	while (hba->lrb_in_use) {
		if (timeout == 0)
			dev_err(&pdev->dev,
				"%s: wait cmdq complete reqs timeout!\n",
				__func__);
		timeout--;
		mdelay(1);
	}

	ufshcd_shutdown(hba);

	dev_err(&pdev->dev, "%s --\n", __func__);
}
EXPORT_SYMBOL_GPL(ufshcd_pltfrm_shutdown);

static int hufs_ufshcd_use_hc_value(struct device *dev)
{
	int use_hufs_hc = 0;
	struct device_node *np = dev->of_node;

	if (of_property_read_u32(np, "ufs-use-hufs-hc", &use_hufs_hc))
		dev_info(dev, "find ufs-use-hufs-hc value, is %d\n",
		    use_hufs_hc);

	return use_hufs_hc;
}

static int hufs_ufshcd_unipro_base(struct platform_device *pdev,
				   void __iomem **ufs_unipro_base,
				   struct resource **mem_res)
{
	int use_hufs_hc;
	struct device *dev = &pdev->dev;
	int err;

	use_hufs_hc = hufs_ufshcd_use_hc_value(dev);
	if (use_hufs_hc) {
		dev_info(dev, "use hufs host controller\n");
		*mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
		*ufs_unipro_base = devm_ioremap_resource(dev, *mem_res);
		if (IS_ERR(*ufs_unipro_base)) {
			err = PTR_ERR(*ufs_unipro_base);
			return err;
		}
	} else {
		*ufs_unipro_base = 0;
	}

	return 0;
}

#ifdef CONFIG_HUFS_HC_CORE_UTR
static int hufs_ufshcd_get_core_irqs(struct platform_device *pdev, struct ufs_hba *hba)
{
	unsigned int i;
	unsigned int core0_interrupt_index;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;

	if (of_property_read_u32(np, "core0-interrupt-index",
				 &core0_interrupt_index)) {
		dev_err(dev, "ufshc core interrupt is not available!\n");
		return -ENODEV;
	}
	for (i = 0; i < UTR_CORE_NUM; i++) {
		hba->core_irq[i] =
			platform_get_irq(pdev, core0_interrupt_index + i);
		if (hba->core_irq[i] < 0) {
			dev_err(dev, "core %d irq resource not available!\n", i);
			return -ENODEV;
		}
#ifdef CONFIG_DFX_DEBUG_FS
		if ((hba->core_irq[i] - hba->core_irq[0]) != i)
			rdr_syserr_process_for_ap(
				(u32)MODID_AP_S_PANIC_STORAGE, 0ull, 0ull);
#endif
		snprintf(hba->core_irq_name[i], CORE_IRQ_NAME_LEN, "ufshcd_hufs_core%d_irq", i);
	}

	return 0;
}
#endif

static int hufs_ufshcd_pltfrm_irq(struct platform_device *pdev,
				  struct ufs_hba *hba,
				  void __iomem *ufs_unipro_base)
{
	struct device *dev = &pdev->dev;
	int err = -ENODEV;

	hba->use_hufs_hc = hufs_ufshcd_use_hc_value(dev);
	if (!hba->use_hufs_hc)
		return 0;

#ifdef CONFIG_SCSI_UFS_INTR_HUB
	hba->ufs_intr_hub_irq = platform_get_irq(pdev, 0);
	if (hba->ufs_intr_hub_irq < 0) {
		dev_err(dev, "IRQ resource not available\n");
		return err;
	}
#else
	hba->unipro_irq = platform_get_irq(pdev, 1);
	if (hba->unipro_irq < 0) {
		dev_err(dev, "IRQ resource not available\n");
		return err;
	}

	hba->fatal_err_irq = platform_get_irq(pdev, 2);
	if (hba->fatal_err_irq < 0) {
		dev_err(dev, "IRQ resource not available\n");
		return err;
	}
#endif

#ifdef CONFIG_HUFS_HC_CORE_UTR
	if (hufs_ufshcd_get_core_irqs(pdev, hba))
		return err;
#endif
	hba->ufs_unipro_base = ufs_unipro_base;

	return 0;
}

static void ufshcd_init_lanes_per_dir(struct ufs_hba *hba)
{
	struct device *dev = hba->dev;
	int ret;

	ret = of_property_read_u32(dev->of_node, "lanes-per-direction",
		&hba->lanes_per_direction);
	if (ret) {
		dev_dbg(hba->dev,
			"%s: failed to read lanes-per-direction, ret=%d\n",
			__func__, ret);
		hba->lanes_per_direction = UFSHCD_DEFAULT_LANES_PER_DIRECTION;
	}
}

/**
 * ufshcd_get_pwr_dev_param - get finally agreed attributes for
 *                            power mode change
 * @pltfrm_param: pointer to platform parameters
 * @dev_max: pointer to device attributes
 * @agreed_pwr: returned agreed attributes
 *
 * Returns 0 on success, non-zero value on failure
 */
int ufshcd_get_pwr_dev_param(struct ufs_dev_params *pltfrm_param,
			     struct ufs_pa_layer_attr *dev_max,
			     struct ufs_pa_layer_attr *agreed_pwr)
{
	int min_pltfrm_gear;
	int min_dev_gear;
	bool is_dev_sup_hs = false;
	bool is_pltfrm_max_hs = false;

	if (dev_max->pwr_rx == FAST_MODE)
		is_dev_sup_hs = true;

	if (pltfrm_param->desired_working_mode == UFS_HS_MODE) {
		is_pltfrm_max_hs = true;
		min_pltfrm_gear = min_t(u32, pltfrm_param->hs_rx_gear,
					pltfrm_param->hs_tx_gear);
	} else {
		min_pltfrm_gear = min_t(u32, pltfrm_param->pwm_rx_gear,
					pltfrm_param->pwm_tx_gear);
	}

	/*
	 * device doesn't support HS but
	 * pltfrm_param->desired_working_mode is HS,
	 * thus device and pltfrm_param don't agree
	 */
	if (!is_dev_sup_hs && is_pltfrm_max_hs) {
		pr_info("%s: device doesn't support HS\n",
			__func__);
		return -ENOTSUPP;
	} else if (is_dev_sup_hs && is_pltfrm_max_hs) {
		/*
		 * since device supports HS, it supports FAST_MODE.
		 * since pltfrm_param->desired_working_mode is also HS
		 * then final decision (FAST/FASTAUTO) is done according
		 * to pltfrm_params as it is the restricting factor
		 */
		agreed_pwr->pwr_rx = pltfrm_param->rx_pwr_hs;
		agreed_pwr->pwr_tx = agreed_pwr->pwr_rx;
	} else {
		/*
		 * here pltfrm_param->desired_working_mode is PWM.
		 * it doesn't matter whether device supports HS or PWM,
		 * in both cases pltfrm_param->desired_working_mode will
		 * determine the mode
		 */
		agreed_pwr->pwr_rx = pltfrm_param->rx_pwr_pwm;
		agreed_pwr->pwr_tx = agreed_pwr->pwr_rx;
	}

	/*
	 * we would like tx to work in the minimum number of lanes
	 * between device capability and vendor preferences.
	 * the same decision will be made for rx
	 */
	agreed_pwr->lane_tx = min_t(u32, dev_max->lane_tx,
				    pltfrm_param->tx_lanes);
	agreed_pwr->lane_rx = min_t(u32, dev_max->lane_rx,
				    pltfrm_param->rx_lanes);

	/* device maximum gear is the minimum between device rx and tx gears */
	min_dev_gear = min_t(u32, dev_max->gear_rx, dev_max->gear_tx);

	/*
	 * if both device capabilities and vendor pre-defined preferences are
	 * both HS or both PWM then set the minimum gear to be the chosen
	 * working gear.
	 * if one is PWM and one is HS then the one that is PWM get to decide
	 * what is the gear, as it is the one that also decided previously what
	 * pwr the device will be configured to.
	 */
	if ((is_dev_sup_hs && is_pltfrm_max_hs) ||
	    (!is_dev_sup_hs && !is_pltfrm_max_hs)) {
		agreed_pwr->gear_rx =
			min_t(u32, min_dev_gear, min_pltfrm_gear);
	} else if (!is_dev_sup_hs) {
		agreed_pwr->gear_rx = min_dev_gear;
	} else {
		agreed_pwr->gear_rx = min_pltfrm_gear;
	}
	agreed_pwr->gear_tx = agreed_pwr->gear_rx;

	agreed_pwr->hs_rate = pltfrm_param->hs_rate;

	return 0;
}
EXPORT_SYMBOL_GPL(ufshcd_get_pwr_dev_param);

/**
 * ufshcd_pltfrm_init - probe routine of the driver
 * @pdev: pointer to Platform device handle
 * @vops: pointer to variant ops
 *
 * Returns 0 on success, non-zero value on failure
 */
int ufshcd_pltfrm_init(struct platform_device *pdev,
		       const struct ufs_hba_variant_ops *vops)
{
	struct ufs_hba *hba;
	void __iomem *mmio_base;
	int irq, err;
	int timer_irq = -1;
	struct device *dev = &pdev->dev;

	mmio_base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(mmio_base)) {
		err = PTR_ERR(mmio_base);
		goto out;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		err = irq;
		goto out;
	}

	err = ufshcd_alloc_host(dev, &hba);
	if (err) {
		dev_err(&pdev->dev, "Allocation failed\n");
		goto out;
	}

	hba->vops = vops;

	err = ufshcd_parse_clock_info(hba);
	if (err) {
		dev_err(&pdev->dev, "%s: clock parse failed %d\n",
				__func__, err);
		goto dealloc_host;
	}
	err = ufshcd_parse_regulator_info(hba);
	if (err) {
		dev_err(&pdev->dev, "%s: regulator init failed %d\n",
				__func__, err);
		goto dealloc_host;
	}

	ufshcd_init_lanes_per_dir(hba);

	err = ufshcd_init(hba, mmio_base, irq, timer_irq);
	if (err) {
		dev_err(dev, "Initialization failed\n");
		goto dealloc_host;
	}

	platform_set_drvdata(pdev, hba);

	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	return 0;

dealloc_host:
	ufshcd_dealloc_host(hba);
out:
	return err;
}
EXPORT_SYMBOL_GPL(ufshcd_pltfrm_init);

/*
 * ufshcd_pltfrm_probe - probe routine of the driver
 * @pdev: pointer to Platform device handle
 *
 * Returns 0 on success, non-zero value on failure
 */
int ufshcd_pltfrm_probe(struct platform_device *pdev)
{
	struct ufs_hba *hba = NULL;
	void __iomem *mmio_base = NULL;
	struct resource *mem_res = NULL;
	int irq;
	int timer_irq = -1;
	int err;
	void __iomem *ufs_unipro_base = NULL;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	unsigned int timer_intr_index;

#ifdef CONFIG_BOOTDEVICE
	if (get_bootdevice_type() != BOOT_DEVICE_UFS) {
		dev_err(dev, "system is't booted from UFS on ARIES FPGA board\n");
		return -ENODEV;
	}
#endif

	mem_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	mmio_base = devm_ioremap_resource(dev, mem_res);
	if (IS_ERR(mmio_base)) {
		err = PTR_ERR(mmio_base);
		goto out;
	}

	err = hufs_ufshcd_unipro_base(pdev, &ufs_unipro_base, &mem_res);
	if (err)
		goto out;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(dev, "IRQ resource not available\n");
		err = -ENODEV;
		goto out;
	}

	err = ufshcd_alloc_host(dev, &hba);
	if (err) {
		dev_err(&pdev->dev, "Allocation failed\n");
		goto out;
	}

	hba->vops = get_variant_ops(&pdev->dev);

	if (!of_property_read_u32(
		    np, "timer-interrupt-index", &timer_intr_index)) {
		timer_irq = platform_get_irq(pdev, timer_intr_index);
		if (timer_irq < 0)
			dev_err(dev, "UFS timer interrupt is not available!\n");
	}

	pm_runtime_set_active(&pdev->dev);
	pm_runtime_irq_safe(&pdev->dev);
	pm_suspend_ignore_children(&pdev->dev, true);
	/* pm runtime delay 5 us time */
	pm_runtime_set_autosuspend_delay(&pdev->dev, 5);
	pm_runtime_use_autosuspend(&pdev->dev);
	if (of_find_property(np, "hufs-disable-pm-runtime", NULL))
		hba->caps |= DISABLE_UFS_PMRUNTIME;

	/* auto hibern8 can not exist with pm runtime */
	if ((hba->caps & DISABLE_UFS_PMRUNTIME) ||
		of_find_property(np, "hufs-use-auto-H8", NULL))
		pm_runtime_forbid(hba->dev);

	parse_hpb_dts(hba, np);

	err = hufs_ufshcd_pltfrm_irq(pdev, hba, ufs_unipro_base);
	if (err)
		goto out;

	err = ufshcd_init(hba, mmio_base, irq, timer_irq);
	if (err) {
		dev_err(dev, "Initialization failed\n");
		goto out_disable_rpm;
	}

#ifndef CONFIG_SCSI_UFS_ENHANCED_INLINE_CRYPTO_V2
#ifdef CONFIG_SCSI_UFS_INLINE_CRYPTO
	/*
	 * to improve writing key efficiency,
	 * remap key regs with writecombine
	 */
	err = ufshcd_keyregs_remap_wc(hba, mem_res->start);
	if (err) {
		dev_err(dev, "ufshcd_keyregs_remap_wc err\n");
		goto out_disable_rpm;
	}

#endif
#endif
	platform_set_drvdata(pdev, hba);

	return 0;

out_disable_rpm:
	pm_runtime_disable(&pdev->dev);
	pm_runtime_set_suspended(&pdev->dev);
out:
	return err;
}

/*
 * ufshcd_pltfrm_remove - remove platform driver routine
 * @pdev: pointer to platform device handle
 *
 * Returns 0 on success, non-zero value on failure
 */
static int ufshcd_pltfrm_remove(struct platform_device *pdev)
{
	struct ufs_hba *hba =  (struct ufs_hba *)platform_get_drvdata(pdev);

	pm_runtime_get_sync(&(pdev)->dev);
	ufshcd_remove(hba);
	return 0;
}

static const struct dev_pm_ops ufshcd_dev_pm_ops = {
	.suspend	= ufshcd_pltfrm_suspend,
	.resume		= ufshcd_pltfrm_resume,
	.restore	= ufshcd_pltfrm_restore,
	.runtime_suspend = ufshcd_pltfrm_runtime_suspend,
	.runtime_resume  = ufshcd_pltfrm_runtime_resume,
	.runtime_idle    = ufshcd_pltfrm_runtime_idle,
};

static struct platform_driver ufshcd_pltfrm_driver = {
	.probe	= ufshcd_pltfrm_probe,
	.remove	= ufshcd_pltfrm_remove,
	.shutdown = ufshcd_pltfrm_shutdown,
	.driver	= {
		.name	= "ufshcd",
		.pm	= &ufshcd_dev_pm_ops,
		.of_match_table = ufs_of_match,
	},
};

module_platform_driver(ufshcd_pltfrm_driver);

MODULE_AUTHOR("Santosh Yaragnavi <santosh.sy@samsung.com>");
MODULE_AUTHOR("Vinayak Holikatti <h.vinayak@samsung.com>");
MODULE_DESCRIPTION("UFS host controller Platform bus based glue driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(UFSHCD_DRIVER_VERSION);
