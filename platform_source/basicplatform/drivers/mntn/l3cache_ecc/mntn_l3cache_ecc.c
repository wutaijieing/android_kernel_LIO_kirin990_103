/*
 * mntn_l3cache_ecc.c
 *
 * l3cache ecc
 *
 * Copyright (c) 2018-2021 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#define pr_fmt(fmt) "l3cache-ecc: " fmt
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/arm-smccc.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <asm/irq.h>
#include <platform_include/basicplatform/linux/util.h>
#include <linux/syscalls.h>
#include <platform_include/basicplatform/linux/rdr_platform.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#include <platform_include/basicplatform/linux/dfx_bbox_diaginfo.h>
#include "mntn_l3cache_ecc.h"
#define PR_LOG_TAG		MNTN_L3CACHE_ECC_TAG

noinline u64 atfd_l3cache_ecc_smc(u64 _function_id, u64 _arg0, u64 _arg1)
{
	struct arm_smccc_res res;

	arm_smccc_1_1_smc(_function_id, _arg0, _arg1, &res);

	return (u64)res.a1;
}

enum serr_type l3cache_ecc_get_status(u64 *err1_status, u64 *err1_misc0)
{
	u64 ret;
	enum serr_type serr;

	if (err1_status == NULL || err1_misc0 == NULL) {
		pr_err("invalid input\n");
		return NO_EEROR;
	}

	ret = atfd_l3cache_ecc_smc((u64)L3ECC_FID_VALUE,
				   (u64)L3CACHE_ECC_READ,
				   (u64)ERR1STATUS);

	pr_debug("ERR1STATUS: 0x%llx\n", ret);
	switch (ret & 0xff) {
	case 0x0:
		pr_err("No error\n");
		serr = NO_EEROR;
		break;
	case 0x2:
		pr_err("ECC error from internal data buffer\n");
		serr = INTERNAL_DATA_BUFFER;
		break;
	case 0x6:
		pr_err("ECC error on cache data RAM\n");
		serr = CACHE_DATA_RAM;
		break;
	case 0x7:
		pr_err("ECC error on cache tag or dirty RAM\n");
		serr = CACHE_TAG_DIRTY_RAM;
		break;
	case 0x12:
		pr_err("Bus error\n");
		serr = BUS_ERROR;
		break;
	default:
		pr_err("invalid value\n");
		serr = INVALID_VAL;
		break;
	}

	*err1_status = ret;
	ret = atfd_l3cache_ecc_smc((u64)L3ECC_FID_VALUE,
				   (u64)L3CACHE_ECC_READ,
				   (u64)ERR1MISC0);
	pr_debug("ERR1MISC0: 0x%llx\n", ret);
	*err1_misc0 = ret;

	return serr;
}

#ifdef CONFIG_DFX_L3CACHE_ECC_DEBUG
/* 0: 1-bit error; 1: 2-bit error */
void l3cache_ecc_error_input(u64 type)
{
	if (type > 1) {
		pr_err("[%s]parameter error\n", __func__);
		return;
	}

	atfd_l3cache_ecc_smc((u64)(L3ECC_FID_VALUE),
			     (u64)L3CACHE_ECC_ERR_INPUT,
			     (u64)type);
}
EXPORT_SYMBOL(l3cache_ecc_error_input);

static ssize_t panic_ecc_show(struct device *dev,
			      struct device_attribute *attr __maybe_unused,
			      char *buf)
{
	ssize_t ret;
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct l3cache_ecc_info *info = platform_get_drvdata(pdev);

	ret = scnprintf(buf, PAGE_SIZE, "%u\n",
			info->panic_on_ce);
	return ret;
}

static ssize_t panic_ecc_store(struct device *dev,
			       struct device_attribute *attr __maybe_unused,
			       const char *buf, size_t count)
{
	int ret;
	unsigned int val = 0;
	struct platform_device *pdev = container_of(dev, struct platform_device, dev);
	struct l3cache_ecc_info *info = platform_get_drvdata(pdev);

	ret = kstrtouint(buf, 10, &val);
	if (ret != 0)
		return -EINVAL;

	/* 1 for panic, 0 for DMD */
	if (val != 0)
		info->panic_on_ce = 1;
	else
		info->panic_on_ce = 0;

	return count;
}

static ssize_t error_input_store(struct device *dev __maybe_unused,
				 struct device_attribute *attr __maybe_unused,
				 const char *buf, size_t count)
{
	int ret;
	unsigned int val = 0;

	ret = kstrtouint(buf, 10, &val);
	if (ret != 0)
		return -EINVAL;

	val = !!val;
	l3cache_ecc_error_input(val);

	return count;
}

#define DEVICE_ATTR(_name, _mode, _show, _store) \
	struct device_attribute dev_attr_##_name = __ATTR(_name, _mode, _show, _store)

static DEVICE_ATTR(panic_on_ce, 0660, panic_ecc_show, panic_ecc_store);
static DEVICE_ATTR(error_input, 0660, NULL, error_input_store);

static struct attribute *ecc_attrs[] = {
	&dev_attr_panic_on_ce.attr,
	&dev_attr_error_input.attr,
	NULL
};

static const struct attribute_group ecc_attr_group = {
	.attrs = ecc_attrs,
};
#endif

static void l3cache_ecc_open(void)
{
	atfd_l3cache_ecc_smc((u64)(L3ECC_FID_VALUE),
			     (u64)L3CACHE_ECC_OPEN,
			     (u64)NULL);
}

static void l3cache_ecc_int_clear(void)
{
	atfd_l3cache_ecc_smc((u64)(L3ECC_FID_VALUE),
			     (u64)L3CACHE_ECC_INT_CLEAR,
			     (u64)NULL);
}

static void l3cache_ecc_diaginfo_record(enum serr_type serr, u32 err_type,
					const char *irq_name, u64 err1_status,
					u64 err1_misc0)
{
	if (err_type == 1U) {
		/* 1-bit error */
		if (serr == BUS_ERROR) {
			pr_err("[%s] l3 ecc %u-bit bus error of %s, err1status:0x%llx, err1misc0:0x%llx",
			       __func__, err_type, irq_name,
			       err1_status, err1_misc0);
		} else {
			bbox_diaginfo_record(L3_ECC_CE, NULL,
					     "l3 ecc %u-bit error of %s, err1status:0x%llx, err1misc0:0x%llx",
					     err_type, irq_name,
					     err1_status, err1_misc0);
		}
	} else if (err_type == 2U) {
		/* 2-bit error */
		if (serr == BUS_ERROR) {
			pr_err("[%s] l3 ecc %u-bit bus error of %s, err1status:0x%llx, err1misc0:0x%llx",
			       __func__, err_type, irq_name,
			       err1_status, err1_misc0);
		} else {
			bbox_diaginfo_record(L3_ECC_UE, NULL,
					     "l3 ecc %u-bit error of %s, err1status:0x%llx, err1misc0:0x%llx",
					     err_type, irq_name,
					     err1_status, err1_misc0);
		}
	}
}

static void dfx_l3cache_ecc_record(struct l3cache_ecc_info *info __maybe_unused,
				   u32 modid __maybe_unused, u32 err_type,
				   const char *irq_name)
{
	u64 err1_status, err1_misc0;
	enum serr_type serr;

	serr = l3cache_ecc_get_status(&err1_status, &err1_misc0);
#ifdef CONFIG_DFX_L3CACHE_ECC_DEBUG
	pr_err("[%s:%d] %u-bit err happen of %s\n",
	       __func__, __LINE__, err_type, irq_name);
	if (serr != BUS_ERROR && serr != INVALID_VAL) {
		if (err_type == 1U && info->panic_on_ce == 0) {
			l3cache_ecc_diaginfo_record(serr, err_type, irq_name,
						    err1_status, err1_misc0);
		} else {
			/* record reboot reason, then reboot */
			rdr_syserr_process_for_ap(modid, 0ULL, 0ULL);
		}
	} else if (serr == BUS_ERROR) {
		/* bus error */
		l3cache_ecc_diaginfo_record(serr, err_type, irq_name,
					    err1_status, err1_misc0);
	}
#else
	if (serr != INVALID_VAL)
		/* record bit error */
		l3cache_ecc_diaginfo_record(serr, err_type, irq_name,
					    err1_status, err1_misc0);
#endif
}

static irqreturn_t dfx_l3cache_ecc_handler(int irq, void *data)
{
	struct l3cache_ecc_info *info = (struct l3cache_ecc_info *)data;
	u32 ret;

	if (info->irq_fault == irq) {
		ret = (u32)atfd_l3cache_ecc_smc((u64)(L3ECC_FID_VALUE),
						(u64)L3CACHE_ECC_READ,
						(u64)ERR1STATUS);
		if (ret & ERR1STATUS_V_BIT_MASK) {
			/* 1-bit err of faultirq */
			if (!(ret & ERR1STATUS_UE_BIT_MASK)) {
				dfx_l3cache_ecc_record(info,
					(u32)MODID_AP_S_L3CACHE_ECC1,
					1U, "faultirq");
			} else {
				/* 2-bit err of faultirq */
				dfx_l3cache_ecc_record(info,
					(u32)MODID_AP_S_L3CACHE_ECC2,
					2U, "faultirq");
			}
		} else {
			pr_info("[%s:%d] ERR1STATUS is not valid in faultirq\n",
				__func__, __LINE__);
		}
		/* clear ecc interrupt */
		l3cache_ecc_int_clear();
	} else if (info->irq_err == irq) {
		ret = (u32)atfd_l3cache_ecc_smc((u64)(L3ECC_FID_VALUE),
						(u64)L3CACHE_ECC_READ,
						(u64)ERR1STATUS);
		if (ret & ERR1STATUS_V_BIT_MASK)
			dfx_l3cache_ecc_record(info, (u32)MODID_AP_S_L3CACHE_ECC2,
						2U, "errirq");
		else
			pr_info("[%s:%d] ERR1STATUS is not valid in errirq\n",
				__func__, __LINE__);
		/* clear ecc interrupt */
		l3cache_ecc_int_clear();
	} else {
		pr_err("[%s:%d]invalid irq %d\n", __func__, __LINE__, irq);
		return IRQ_NONE;
	}

	return IRQ_HANDLED;
}

static int dfx_l3cache_ecc_probe(struct platform_device *pdev)
{
	struct l3cache_ecc_info *info = NULL;
	struct device *dev = &pdev->dev;
	int ret;

	info = devm_kzalloc(dev, sizeof(*info), GFP_KERNEL);
	if (info == NULL) {
		pr_err("[%s]devm_kzalloc error\n", __func__);
		ret = -ENOMEM;
		goto err;
	}

	info->irq_fault = platform_get_irq_byname(pdev, "faultirq");
	if (info->irq_fault < 0) {
		pr_err("[%s]failed to get irq_fault id\n", __func__);
		ret = -ENOENT;
		goto err;
	}

	ret = devm_request_threaded_irq(dev, info->irq_fault,
					dfx_l3cache_ecc_handler, NULL,
					IRQF_TRIGGER_NONE | IRQF_ONESHOT,
					"faultirq", info);
	if (ret < 0) {
		pr_err("[%s]failed to request faultirq\n", __func__);
		ret = -EIO;
		goto err;
	}

	info->irq_err = platform_get_irq_byname(pdev, "errirq");
	if (info->irq_err < 0) {
		pr_err("[%s]failed to get irq_err id\n", __func__);
		ret = -ENOENT;
		goto err_devm_request_irq;
	}
	ret = devm_request_threaded_irq(dev, info->irq_err,
					dfx_l3cache_ecc_handler, NULL,
					IRQF_TRIGGER_NONE | IRQF_ONESHOT,
					"errirq", info);
	if (ret < 0) {
		pr_err("[%s]failed to request errirq\n", __func__);
		ret = -EIO;
		goto err_devm_request_irq;
	}

	platform_set_drvdata(pdev, info);

#ifdef CONFIG_DFX_L3CACHE_ECC_DEBUG
	/* Set the default type of CE, 1 is panic, 0 is DMD */
	info->panic_on_ce = 1;

	ret = sysfs_create_group(&pdev->dev.kobj, &ecc_attr_group);
	if (ret != 0) {
		dev_err(dev, "sysfs create failed\n");
		goto err;
	}
#endif

	l3cache_ecc_open();

	pr_err("[%s]end\n", __func__);
	return ret;

err:
	return ret;

err_devm_request_irq:
	devm_free_irq(dev, info->irq_fault, info);
	return ret;
}

static int dfx_l3cache_ecc_remove(struct platform_device *pdev)
{
	struct l3cache_ecc_info *info = platform_get_drvdata(pdev);

	if (info == NULL)
		return ECC_ERR;
#ifdef CONFIG_DFX_L3CACHE_ECC_DEBUG
	sysfs_remove_group(&pdev->dev.kobj, &ecc_attr_group);
#endif
	devm_free_irq(&pdev->dev, info->irq_err, info);
	devm_free_irq(&pdev->dev, info->irq_fault, info);
	return ECC_OK;
}

static const struct of_device_id dfx_l3cache_ecc_of_match[] = {
	{
		.compatible = "hisilicon,hisi-l3cache-ecc",
	},
	{ },
};

MODULE_DEVICE_TABLE(of, dfx_l3cache_ecc_of_match);

static struct platform_driver dfx_l3cache_ecc_driver = {
	.probe = dfx_l3cache_ecc_probe,
	.remove = dfx_l3cache_ecc_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "hisi-l3cache-ecc",
		   .of_match_table = dfx_l3cache_ecc_of_match,
		   },
};

static int __init l3cache_ecc_mntn_init(void)
{
	return platform_driver_register(&dfx_l3cache_ecc_driver);
}

static void __exit l3cache_ecc_mntn_exit(void)
{
	platform_driver_unregister(&dfx_l3cache_ecc_driver);
}

late_initcall(l3cache_ecc_mntn_init);

MODULE_LICENSE("GPL v2");
