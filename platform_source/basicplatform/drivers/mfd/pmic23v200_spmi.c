/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * pmic_23v200_spmi.c
 *
 * Device driver for regulators in HISI pmic_23v200 IC
 */

#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <platform_include/basicplatform/linux/mfd/pmic_platform.h>
#include <platform_include/basicplatform/linux/mfd/pmic23V200.h>
#include <linux/irq.h>
#include <linux/pm_wakeup.h>
#include <platform_include/basicplatform/linux/spmi_platform.h>
#include <platform_include/basicplatform/linux/of_platform_spmi.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/pinctrl/consumer.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include <platform_include/basicplatform/linux/power/platform/coul/coul_drv.h>
#include <platform_include/basicplatform/linux/rdr_platform.h>

#ifdef CONFIG_HW_VOTE
#include <linux/platform_drivers/hw_vote.h>

static struct hvdev *limit_bigfreq_hvdev = NULL;
static struct hvdev *limit_gpufreq_hvdev = NULL;
#endif

static struct wakeup_source *vbatt_check_lock;
int battery_check_count = 0;
int vbatt_limit = 3250;

u32 subpmic_read(struct pmic_23v200 *subpmic, int reg)
{
	u32 ret;
	u8 read_value = 0;
	struct spmi_device *pdev;

	if (NULL == subpmic) {
		pr_err(" pmic_23v200 is NULL\n");
		return -1;/*lint !e570 */
	}

	pdev = to_spmi_device(subpmic->dev);
	if (NULL == pdev) {
		pr_err("%s:pdev get failed!\n", __func__);
		return -1;/*lint !e570 */
	}

	ret = vendor_spmi_ext_register_readl(pdev->ctrl, 6, reg, (unsigned char*)&read_value, 1);
	if (ret) {
		pr_err("%s:spmi_ext_register_readl failed!\n", __func__);
		return ret;
	}
	return (u32)read_value;
}
EXPORT_SYMBOL(subpmic_read);

void subpmic_write(struct pmic_23v200 *subpmic, int reg, u32 val)
{
	u32 ret;
	struct spmi_device *pdev;
	if (NULL == subpmic) {
		pr_err(" pmic_23v200 is NULL\n");
		return;
	}

	pdev = to_spmi_device(subpmic->dev);
	if (NULL == pdev) {
		pr_err("%s:pdev get failed!\n", __func__);
		return;
	}

	ret = vendor_spmi_ext_register_writel(pdev->ctrl, 6, reg, (unsigned char*)&val, 1);
	if (ret) {
		pr_err("%s:spmi_ext_register_writel failed!\n", __func__);
		return;
	}
}
EXPORT_SYMBOL(subpmic_write);

static struct of_device_id of_pmic_23v200_match_tbl[] = {
	{
		.compatible = "6423-pmic-spmi",
	},
	{ }   /* end */
};

static int get_pmic_23v200_device_tree_data(struct device_node *np, struct pmic_23v200 *subpmic)
{
	int ret;

	ret = of_property_read_u32_array(np, "slave_id", &(subpmic->sid), 1);
	if (ret) {
		pr_err("no slave_id property set\n");
		ret = -ENODEV;
		return ret;
	}

	ret = of_property_read_u32_array(np, "subpmic-irq-addr",
						&(subpmic->irq_addr), 1);
	if (ret) {
		pr_err("no hisilicon,hisi-23v200-pmic-irq-addr property set\n");
		ret = -ENODEV;
		return ret;
	}

	ret = of_property_read_u32_array(np, "subpmic-irq-mask-addr",
						&(subpmic->irq_mask_addr), 1);
	if (ret) {
		pr_err("no hisilicon,hisi-23v200-pmic-irq-mask-addr property set\n");
		ret = -ENODEV;
		return ret;
	}

	ret = of_property_read_u32_array(np, "subpmic-irq-np-record",
						&(subpmic->irq_np_record), 1);
	if (ret) {
		pr_err("no hisilicon,hisi-23v200-pmic-irq-np-record property set\n");
		ret = -ENODEV;
		return ret;
	}

	return ret;
}

#ifdef CONFIG_DFX_DEBUG_FS
void vbatt_limit_ctrl(int val)
{
	vbatt_limit = val;
	pr_err("vbatt_limit = %d mV", vbatt_limit);
	return;
}
#endif

static void pmic_23v200_vbatt_check(struct work_struct *work)
{
	int vbatt, ret, val;
	struct pmic_23v200 *subpmic = container_of(work, struct pmic_23v200, check_6423_vbatt_work.work);

	pr_err("Enter in schedule_delayed_work!\n");
	battery_check_count++;
	mdelay(2000);
	vbatt = coul_drv_battery_voltage();
	pr_err("battery_check_count: %d, battery voltage = %d\n", battery_check_count, vbatt);
	if (vbatt > vbatt_limit) {
		pr_err("Battery voltage over %d mV\n", vbatt_limit);
#ifdef CONFIG_HW_VOTE
		ret = hv_set_freq(limit_bigfreq_hvdev, 0xFFFFFFFF);
		if (!ret)
			pr_err("big cluster returns to normal frequency\n");

		ret = hv_set_freq(limit_gpufreq_hvdev, 0xFFFFFFFF);
		if (!ret)
			pr_err("gpu returns to normal frequency\n");
#endif
		/* clear interrupt status */
		val = subpmic_read(subpmic, subpmic->irq_addr);
		val = val | 0x10;
		subpmic_write(subpmic, subpmic->irq_addr, val);
		pr_err("subpmic->irq_addr = 0x%x\n", subpmic_read(subpmic, subpmic->irq_addr));

		val = subpmic_read(subpmic, subpmic->irq_mask_addr);
		val = val & 0xef;
		subpmic_write(subpmic, subpmic->irq_mask_addr, val);
		pr_err("subpmic->irq_mask_addr = 0x%x\n", subpmic_read(subpmic, subpmic->irq_mask_addr));

		val = subpmic_read(subpmic, subpmic->irq_np_record);
		val = val | 0x10;
		subpmic_write(subpmic, subpmic->irq_np_record, val);
		pr_err("subpmic->irq_np_record = 0x%x\n", subpmic_read(subpmic, subpmic->irq_np_record));

		battery_check_count = 0;
		enable_irq(subpmic->irq);
		__pm_relax(vbatt_check_lock);/*lint !e455*/
	} else {
		schedule_delayed_work(&subpmic->check_6423_vbatt_work, msecs_to_jiffies(2000));
	}

	return;
}

static irqreturn_t pmic_23v200_irq_handler(int irq, void *data)
{
	struct pmic_23v200 *subpmic = (struct pmic_23v200 *)data;
	unsigned char val;
	int ret;

	__pm_stay_awake(vbatt_check_lock);

	pr_err("pmic_23v200_irq_handler started!");
	val = subpmic_read(subpmic, subpmic->irq_np_record);
	if (val & 0x10) {
		/* interrupt mask */
		disable_irq_nosync(subpmic->irq);
		pr_err("pmic_23v200 interrupts under voltage\n");
#ifdef CONFIG_HW_VOTE
		/* vote freq */
		ret = hv_set_freq(limit_bigfreq_hvdev, 0);
		if (!ret)
			pr_err("big cluster votes to lowest frequency\n");

		ret = hv_set_freq(limit_gpufreq_hvdev, 0);
		if (!ret)
			pr_err("gpu votes to lowest frequency\n");
#endif
		/* delayed work: check battery voltage */
		schedule_delayed_work(&subpmic->check_6423_vbatt_work, 0);
	} else {
		pr_err("AP_S_SUBPMU, pmic_23v200_record_reg value = 0x%x\n", val);
		rdr_syserr_process_for_ap(MODID_AP_S_SUBPMU, 0, 0);
	}
	return IRQ_HANDLED;/*lint !e454*/
}

#ifdef CONFIG_HW_VOTE
void hisi_cluster_freq_limit_init(struct device *dev)
{
	struct device_node *np = dev->of_node;
	const char *ch_name;
	const char *vsrc;
	int ret;

	ret = of_property_read_string_index(np, "bigfreq-limit-channel", 0, &ch_name);
	if (ret) {
		dev_err(dev, "[%s]:parse channel name fail!\n", __func__);
		goto gpu_limit_init;
	}

	ret = of_property_read_string_index(np, "bigfreq-limit-channel", 1, &vsrc);
	if (ret) {
		dev_err(dev, "[%s]:parse vote src fail!\n", __func__);
		goto gpu_limit_init;
	}
	limit_bigfreq_hvdev = hvdev_register(dev, ch_name, vsrc);
	if (IS_ERR_OR_NULL(limit_bigfreq_hvdev))
		dev_err(dev, "[%s]: bigfreq limit vote register fail!\n", __func__);

gpu_limit_init:
	ret = of_property_read_string_index(np, "gpufreq-limit-channel", 0, &ch_name);
	if (ret) {
		dev_err(dev, "[%s]:parse channel name fail!\n", __func__);
		goto out;
	}
	ret = of_property_read_string_index(np, "gpufreq-limit-channel", 1, &vsrc);
	if (ret) {
		dev_err(dev, "[%s]:parse vote src fail!\n", __func__);
		goto out;
	}

	limit_gpufreq_hvdev = hvdev_register(dev, ch_name, vsrc);
	if (IS_ERR_OR_NULL(limit_gpufreq_hvdev))
		dev_err(dev, "[%s]: gpufreq limit vote register fail!\n", __func__);

out:
	return;
}
#endif

static int pmic_23v200_probe(struct spmi_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct pmic_23v200 *subpmic = NULL;
	struct rdr_exception_info_s einfo;
	enum of_gpio_flags flags;
	unsigned int val;
	int ret;

	subpmic = devm_kzalloc(dev, sizeof(*subpmic), GFP_KERNEL);
	if (!subpmic) {
		dev_err(dev, "cannot allocate pmic_23v200 device info\n");
		return -ENOMEM;
	}

	ret = get_pmic_23v200_device_tree_data(np, subpmic);
	if (ret) {
		dev_err(&pdev->dev, "Error reading hisi pmic_23v200 dts \n");
		return ret;
	}

	subpmic->dev = dev;

	subpmic->gpio = of_get_gpio_flags(np, 0, &flags);
	if (!gpio_is_valid(subpmic->gpio))
		return -EINVAL;

	ret = gpio_request_one(subpmic->gpio, GPIOF_IN, "pmic_23v200");
	if (ret < 0) {
		dev_err(dev, "failed to request 23V200_gpio%d\n", subpmic->gpio);
		return ret;
	}

	subpmic->irq = gpio_to_irq(subpmic->gpio);
	pr_err("pmic_23v200->irq = %d\n", subpmic->irq);

	subpmic->pctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(subpmic->pctrl)) {
		dev_err(&pdev->dev, "failed to 23v200 devm pinctrl get\n");
		ret = -EINVAL;
		goto err_pinctrl;
	}

	subpmic->pctrl_default = pinctrl_lookup_state(subpmic->pctrl, PINCTRL_STATE_DEFAULT);
	if (IS_ERR(subpmic->pctrl_default)) {
		dev_err(&pdev->dev, "failed to 23v200 pinctrl lookup state default\n");
		ret = -EINVAL;
		goto err_pinctrl_put;
	}

	subpmic->pctrl_idle = pinctrl_lookup_state(subpmic->pctrl, PINCTRL_STATE_IDLE);
	if (IS_ERR(subpmic->pctrl_idle)) {
		dev_err(&pdev->dev, "failed to 23v200 pinctrl lookup state idle\n");
		ret = -EINVAL;
		goto err_pinctrl_put;
	}

	ret = pinctrl_select_state(subpmic->pctrl, subpmic->pctrl_default);
	if (ret < 0) {
		dev_err(&pdev->dev, "set 23v200 iomux normal error, %d\n", ret);
		goto err_pinctrl_put;
	}

	/* mask && clear IRQ status */
	val = subpmic_read(subpmic, subpmic->irq_addr);
	pr_err("PMU_6423 IRQ address value:irq[0x%x] = 0x%x\n", subpmic->irq_addr, val);
	val = val | 0x10;
	subpmic_write(subpmic, subpmic->irq_addr, val);

	val = subpmic_read(subpmic, subpmic->irq_mask_addr);
	pr_err("PMU_6423 IRQ_MASK address value:irq[0x%x] = 0x%x\n", subpmic->irq_mask_addr, val);
	val = val & 0xef;
	subpmic_write(subpmic, subpmic->irq_mask_addr, val);

#ifdef CONFIG_HW_VOTE
	hisi_cluster_freq_limit_init(dev);
#endif

	INIT_DELAYED_WORK(&subpmic->check_6423_vbatt_work, pmic_23v200_vbatt_check);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	vbatt_check_lock = wakeup_source_register(dev, "6423_vbatt_check_wake");
#else
	vbatt_check_lock = wakeup_source_register("6423_vbatt_check_wake");
#endif
	if (!vbatt_check_lock) {
		dev_err("%s wakeup source register failed\n", __func__);
		return -ENOMEM;
	}

	ret = devm_request_irq(dev, subpmic->irq, pmic_23v200_irq_handler, IRQF_TRIGGER_FALLING, "sub_pmic_23v200", subpmic);
	if (ret < 0) {
		dev_err(dev, "could not claim pmic_23v200 %d\n", ret);
		ret = -ENODEV;
		goto err_devm_request_irq;
	}

	dev_set_drvdata(&pdev->dev, subpmic);

	/* register rdr exception type */
	memset((void *)&einfo, 0, sizeof(struct rdr_exception_info_s)); /* unsafe_function_ignore: memset  */
	einfo.e_modid = MODID_AP_S_SUBPMU;
	einfo.e_modid_end = MODID_AP_S_SUBPMU;
	einfo.e_process_priority = RDR_ERR;
	einfo.e_reboot_priority = RDR_REBOOT_NOW;
	einfo.e_notify_core_mask = RDR_AP;
	einfo.e_reset_core_mask = RDR_AP;
	einfo.e_reentrant = (u32)RDR_REENTRANT_DISALLOW;
	einfo.e_exce_type = AP_S_SUBPMU;
	einfo.e_from_core = RDR_AP;
	memcpy((void *)einfo.e_from_module, (const void *)"RDR SUBPMU", sizeof("RDR SUBPMU")); /* unsafe_function_ignore: memcpy */ /* [false alarm]:this is original code */
	memcpy((void *)einfo.e_desc, (const void *)"RDR SUBPMU", sizeof("RDR SUBPMU")); /* unsafe_function_ignore: memcpy */ /* [false alarm]:this is original code */
	ret = (s32)rdr_register_exception(&einfo);
	if (!ret)
		pr_err("register 23v200 exception fail.\n");

	return 0;

err_pinctrl_put:
	devm_pinctrl_put(subpmic->pctrl);

err_pinctrl:
err_devm_request_irq:
	gpio_free(subpmic->gpio);
	return ret;
}

static int pmic_23v200_remove(struct spmi_device *pdev)
{
	struct pmic_23v200 *subpmic = dev_get_drvdata(&pdev->dev);

	if (NULL == subpmic) {
		pr_err("%s:pmic_23v200 is NULL\n", __func__);
		return -ENOMEM;
	}

	free_irq(subpmic->irq, subpmic);
	gpio_free(subpmic->gpio);
	devm_pinctrl_put(subpmic->pctrl);
	cancel_delayed_work(&subpmic->check_6423_vbatt_work);
	wakeup_source_unregister(vbatt_check_lock);
	devm_kfree(&pdev->dev, subpmic);
	return 0;
}

static int pmic_23v200_suspend(struct spmi_device *pdev, pm_message_t state)
{
	struct pmic_23v200 *subpmic = dev_get_drvdata(&pdev->dev);
	int val;

	if (NULL == subpmic) {
		pr_err("%s:pmic_23v200 is NULL\n", __func__);
		return -ENOMEM;
	}

	pr_info("%s: suspend +\n", __func__);

	val = subpmic_read(subpmic, subpmic->irq_mask_addr);
	pr_err("PMU_6423 IRQ_MASK value before suspend:irq[0x%x] = 0x%x\n", subpmic->irq_mask_addr, val);
	val = val | 0x10;
	subpmic_write(subpmic, subpmic->irq_mask_addr, val);

	pr_info("%s: suspend -\n", __func__);
	return 0;
}

static int pmic_23v200_resume(struct spmi_device *pdev)
{
	struct pmic_23v200 *subpmic = dev_get_drvdata(&pdev->dev);
	int val;

	if (NULL == subpmic) {
		pr_err("%s:pmic_23v200 is NULL\n", __func__);
		return -ENOMEM;
	}

	pr_info("%s: resume +\n", __func__);

	val = subpmic_read(subpmic, subpmic->irq_mask_addr);
	pr_err("PMU_pmic_23v200 IRQ_MASK value before resume:irq[0x%x] = 0x%x\n", subpmic->irq_mask_addr, val);
	val = val & 0xef;
	subpmic_write(subpmic, subpmic->irq_mask_addr, val);

	pr_info("%s: resume -\n", __func__);
	return 0;
}

static const struct spmi_device_id subpmic_spmi_id[] = {
	{"subpmic", 6},
	{}
};

MODULE_DEVICE_TABLE(spmi, subpmic_spmi_id);
static struct spmi_driver pmic_23v200_driver = {
	.driver = {
		.name   = "pmic_23v200",
		.owner  = THIS_MODULE,
		.of_match_table = of_pmic_23v200_match_tbl,
	},
	.id_table = subpmic_spmi_id,
	.probe  = pmic_23v200_probe,
	.remove = pmic_23v200_remove,
	.suspend = pmic_23v200_suspend,
	.resume = pmic_23v200_resume,
};

static int __init pmic_23v200_pmic_init(void)
{
	return spmi_driver_register(&pmic_23v200_driver);
}

static void __exit pmic_23v200_pmic_exit(void)
{
	spmi_driver_unregister(&pmic_23v200_driver);
}

subsys_initcall_sync(pmic_23v200_pmic_init);
module_exit(pmic_23v200_pmic_exit);

MODULE_DESCRIPTION("PMIC_23V200 driver");
MODULE_LICENSE("GPL v2");
