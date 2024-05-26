/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2021. All rights reserved.
 * Description: hhee exception driver source file
 * Create: 2016/12/6
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/kthread.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include <platform_include/basicplatform/linux/util.h>
#include <platform_include/basicplatform/linux/rdr_platform.h>
#include <linux/compiler.h>
#include <linux/interrupt.h>
#include <linux/version.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0))
#include <asm/compiler.h>
#endif
#include <linux/debugfs.h>
#include "hhee.h"
#include "hhee_msg.h"
#include <linux/atomic.h>

struct rdr_exception_info_s hhee_excetption_info[] = {
	{ .e_modid            = MODID_AP_S_HHEE_PANIC,
	  .e_modid_end        = MODID_AP_S_HHEE_PANIC,
	  .e_process_priority = RDR_ERR,
	  .e_reboot_priority  = RDR_REBOOT_NOW,
	  .e_notify_core_mask = RDR_AP,
	  .e_reset_core_mask  = RDR_AP,
	  .e_from_core        = RDR_AP,
	  .e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,
	  .e_exce_type        = AP_S_HHEE_PANIC,
	  .e_upload_flag      = (u32)RDR_UPLOAD_YES,
	  .e_from_module      = "RDR HHEE PANIC",
	  .e_desc             = "RDR HHEE PANIC" }
};

int hhee_panic_handle(unsigned int len, void *buf)
{
	pr_err("hhee panic trigger system_error\n");
	(void)len;
	if (buf != NULL)
		pr_err("El2 trigger panic, buf should be null\n");
	rdr_syserr_process_for_ap((u32)MODID_AP_S_HHEE_PANIC, 0ULL, 0ULL);
	return 0;
}

/* check hhee enable */
static int g_hhee_enable = HHEE_ENABLE;
int hhee_check_enable(void)
{
	return g_hhee_enable;
}
EXPORT_SYMBOL(hhee_check_enable);

#define CPU_MASK 0xF
static void reset_hhee_irq_counters(void)
{
	struct arm_smccc_res res;
	arm_smccc_hvc((unsigned long)HHEE_MONITORLOG_RESET_COUNTERS, 0ul, 0ul,
		      0ul, 0ul, 0ul, 0ul, 0ul, &res);
}

void hkip_clean_counters(void)
{
	reset_hkip_irq_counters();
	reset_hhee_irq_counters();
}

#ifndef CONFIG_HHEE_USING_IPI
static int hhee_irq_get(struct platform_device *pdev)
{
	int ret;
	int irq;
	struct device *dev = &pdev->dev;

	/* irq num get and register */
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(dev, "get irq failed\n");
		return -ENXIO;
	}
	ret = devm_request_irq(dev, (unsigned int)irq, hhee_irq_handle, 0ul,
			       "hkip-hhee", pdev);
	if (ret < 0) {
		dev_err(dev, "devm request irq failed\n");
		return -EINVAL;
	}
	if (cpu_online(CPU_MASK) && (irq_set_affinity(irq, cpumask_of(CPU_MASK)) < 0))
		dev_err(dev, "set affinity failed\n");

	return 0;
}
#endif

static int hhee_xom_read_cnt(struct device *dev)
{
	int ret;
	unsigned int read_cnt = XOM_DEFAULT_READ_COUNT;

	/*
	 * Ignore the return value
	 * HHEE using 0 as default instead of
	 */
	ret = of_property_read_u32_index(dev->of_node, "hhee_read_cnt",
					 0, &read_cnt);
	if (ret) {
		dev_err(dev, "cannot get hhee_read_cnt\n");
		return -EINVAL;
	}

	(void)hhee_fn_hvc((unsigned long)HHEE_HVC_PAGERD_COUNT, read_cnt, 0ul, 0ul);

	return 0;
}

static int hkip_hhee_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;

	if (HHEE_DISABLE == hhee_check_enable())
		return 0;

#ifndef CONFIG_HHEE_USING_IPI
	ret = hhee_irq_get(pdev);
	if (ret < 0)
		dev_err(dev, "hhee irq get error\n");
#endif
	/* rdr struct register */
	ret = (s32)rdr_register_exception(&hhee_excetption_info[0]);
	if (!ret)
		dev_err(dev, "register hhee exception fail\n");

	hhee_module_init();

	ret = hhee_xom_read_cnt(dev);
	if (ret) {
		dev_err(dev, "hhee xom read cnt error\n");
		return ret;
	}

#ifdef CONFIG_HHEE_DEBUG
	ret = hhee_init_debugfs();
	if (ret)
		dev_err(dev, "hhee debugfs init error\n");

	/*
	 * The hhee log init should not affect the hhee probe init,
	 * and hhee log is not available in user mode.
	 */
	ret = hhee_logger_init();
	if (ret < 0)
		dev_err(dev, "hhee logger init error\n");
#endif

	ret = hhee_msg_init();
	if (ret)
		dev_err(dev, "hhee msg init error\n");

	ret = hhee_msg_register_handler(HHEE_MSG_ID_CRASH, hhee_panic_handle);
	if (ret)
		dev_err(dev, "hhee msg regist error\n");

	pr_info("hhee probe done\n");
	return 0;
}

static int hkip_hhee_remove(struct platform_device *pdev)
{
#ifdef CONFIG_HHEE_DEBUG
	hhee_cleanup_debugfs();
#endif
	return 0;
}

static const struct of_device_id hkip_hhee_of_match[] = {
	{ .compatible = "hkip,hkip-hhee" },
	{},
};

static struct platform_driver hkip_hhee_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "hkip-hhee",
		.of_match_table = of_match_ptr(hkip_hhee_of_match),
	},
	.probe = hkip_hhee_probe,
	.remove = hkip_hhee_remove,
};

static int __init hkip_hhee_cmd_get(char *arg)
{
	if (!arg)
		return -EINVAL;

	if (!strncmp(arg, "false", strlen("false"))) {
		g_hhee_enable = HHEE_DISABLE;
	}

	pr_err("hhee enable = %d.\n", g_hhee_enable);
	return 0;
}
early_param("hhee_enable", hkip_hhee_cmd_get);

static int __init hkip_hhee_device_init(void)
{
	int ret;

	pr_info("hhee probe init\n");
	ret = platform_driver_register(&hkip_hhee_driver);
	if (ret)
		pr_err("register hhee driver fail\n");

	return ret;
}

static void __exit hkip_hhee_device_exit(void)
{
	platform_driver_unregister(&hkip_hhee_driver);
}

module_init(hkip_hhee_device_init);
module_exit(hkip_hhee_device_exit);

MODULE_DESCRIPTION("hhee driver");
MODULE_ALIAS("hhee module");
MODULE_LICENSE("GPL");
