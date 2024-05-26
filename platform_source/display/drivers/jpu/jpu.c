/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2021. All rights reserved.
 *
 * This file is jpu device, driver registered and achieve.
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

#include "jpu.h"
#include <linux/slab.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/device.h>
#include <securec.h>
#include "jpgdec_irq.h"
#include "jpu_def.h"

#define JPU_DSS_LEN             8
#define DEV_NAME_JPU            "dev_jpu"
#define DTS_COMP_JPU_NAME       "hisilicon,devjpu"

uint32_t g_jpu_msg_level = 7;
int g_debug_jpu_dec;
int g_debug_jpu_dec_job_timediff;

module_param_named(debug_msg_level, g_jpu_msg_level, int, 0644);
MODULE_PARM_DESC(debug_msg_level, "jpu msg level");

#ifdef CONFIG_FB_DEBUG_USED
module_param_named(debug_jpu_dec, g_debug_jpu_dec, int, 0644);
MODULE_PARM_DESC(debug_jpu_dec, "jpu decode debug");
#endif

#ifdef CONFIG_FB_DEBUG_USED
module_param_named(debug_jpu_decode_job_timediff, g_debug_jpu_dec_job_timediff,
	int, 0644);
MODULE_PARM_DESC(debug_jpu_decode_job_timediff,
	"jpu decode job timediff debug");
#endif

static struct jpu_data_type *g_jpu_device = NULL;

static int jpu_open(struct inode *inode, struct file *filp)
{
	struct jpu_data_type *jpu_device = NULL;
	jpu_info("+\n");
	jpu_check_null_return(filp, -EINVAL);

	if (filp->private_data == NULL)
		filp->private_data = g_jpu_device;

	jpu_device = filp->private_data;
	jpu_check_null_return(jpu_device, -EINVAL);

	jpu_device->ref_cnt++;
	jpu_info("-\n");
	return 0;
}

static int jpu_release(struct inode *inode, struct file *filp)
{
	struct jpu_data_type *jpu_device = NULL;

	jpu_info("+\n");
	jpu_check_null_return(filp, -EINVAL);
	jpu_check_null_return(filp->private_data, -EINVAL);

	jpu_device = filp->private_data;
	if (jpu_device->ref_cnt <= 0) {
		jpu_info("try to close unopened jpu exit\n");
		return -EINVAL;
	}

	jpu_device->ref_cnt--;
	jpu_info("-\n");
	return 0;
}

#ifdef CONFIG_COMPAT
static long jpu_ioctl(struct file *filp, u_int cmd, u_long arg)
{
	int ret;
	struct jpu_data_type *jpu_device = NULL;
	void __user *argp = (void __user *)(uintptr_t)arg;

	jpu_debug("+\n");
	jpu_check_null_return(filp, -EINVAL);
	jpu_check_null_return(filp->private_data, -EINVAL);

	jpu_device = filp->private_data;

	if (cmd == JPU_JOB_EXEC)
		ret = jpu_job_exec(jpu_device, argp);
	else
		ret = -ENOSYS;

	if (ret != 0)
		jpu_err("unsupported ioctl %x, ret = %d\n", cmd, ret);

	jpu_debug("-\n");
	return ret;
}
#endif

static const struct file_operations jpu_fops = {
	.owner = THIS_MODULE,
	.open = jpu_open,
	.release = jpu_release,
	.unlocked_ioctl = jpu_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = jpu_ioctl,
#endif
	.mmap = NULL,
};

static int jpu_set_platform(struct jpu_data_type *jpu_device,
	struct device_node *np)
{
	int ret = of_property_read_string_index(np, "platform-names", 0,
		&(jpu_device->jpg_platform_name));
	if ((ret != 0) || (jpu_device->jpg_platform_name == NULL)) {
		jpu_err("failed to get jpg resource ret = %d\n", ret);
		return -EINVAL;
	}

	if (strncmp(jpu_device->jpg_platform_name, "dss_v400", JPU_DSS_LEN) == 0) {
		jpu_device->jpu_support_platform = DSS_V400;
	} else if (strncmp(jpu_device->jpg_platform_name, "dss_v500", JPU_DSS_LEN) == 0) {
		jpu_device->jpu_support_platform = DSS_V500;
	} else if (strncmp(jpu_device->jpg_platform_name, "dss_v501", JPU_DSS_LEN) == 0) {
		jpu_device->jpu_support_platform = DSS_V501;
	} else if (strncmp(jpu_device->jpg_platform_name, "dss_v510", JPU_DSS_LEN) == 0) {
		jpu_device->jpu_support_platform = DSS_V510;
	} else if (strncmp(jpu_device->jpg_platform_name, "dss_v511", JPU_DSS_LEN) == 0) {
		jpu_device->jpu_support_platform = DSS_V510_CS;
	} else if (strncmp(jpu_device->jpg_platform_name, "dss_v600", JPU_DSS_LEN) == 0) {
		jpu_device->jpu_support_platform = DSS_V600;
	} else if (strncmp(jpu_device->jpg_platform_name, "dss_v700", JPU_DSS_LEN) == 0) {
		jpu_device->jpu_support_platform = DSS_V700;
	} else {
		jpu_device->jpu_support_platform = UNSUPPORT_PLATFORM;
		jpu_err("jpu_device jpg platform is not support\n");
		return -EINVAL;
	}
	jpu_info("jpu_device jpg platform is %d\n", jpu_device->jpu_support_platform);
	return ret;
}

static int jpu_chrdev_setup(struct jpu_data_type *jpu_device)
{
	const unsigned int minor = 0;

	/* get the major number of the character device */
	int ret = register_chrdev(jpu_device->jpu_major, DEV_NAME_JPU, &jpu_fops);
	if (ret < 0) {
		jpu_err("fail to register driver\n");
		return -ENXIO;
	}

	jpu_device->jpu_major = (uint32_t)ret;
	jpu_device->jpu_class = class_create(THIS_MODULE, DEV_NAME_JPU);
	if (jpu_device->jpu_class == NULL) {
		jpu_err("fail to create jpu class\n");
		ret = -ENOMEM;
		goto err_class_create;
	}

	jpu_device->jpu_dev = device_create(jpu_device->jpu_class, 0,
		MKDEV(jpu_device->jpu_major, minor), NULL, DEV_NAME_JPU);
	if (jpu_device->jpu_dev == NULL) {
		jpu_err("fail to create jpu device\n");
		ret = -ENOMEM;
		goto err_device_create;
	}

	return 0;

err_device_create:
	class_destroy(jpu_device->jpu_class);
	jpu_device->jpu_class = NULL;
err_class_create:
	unregister_chrdev(jpu_device->jpu_major, DEV_NAME_JPU);
	jpu_device->jpu_major = 0;
	return ret;
}

static void jpu_chrdev_remove(struct jpu_data_type *jpu_device)
{
	if (jpu_device->jpu_class != NULL) {
		if (jpu_device->jpu_dev != NULL) {
			device_destroy(jpu_device->jpu_class, MKDEV(jpu_device->jpu_major, 0));
			jpu_device->jpu_dev = NULL;
		}
		class_destroy(jpu_device->jpu_class);
		jpu_device->jpu_class = NULL;
	}

	if (jpu_device->jpu_major > 0) {
		unregister_chrdev(jpu_device->jpu_major, DEV_NAME_JPU);
		jpu_device->jpu_major = 0;
	}
}

static int jpu_get_reg_base(struct jpu_data_type *jpu_device,
	struct device_node *np)
{
	jpu_device->jpu_dec_base = of_iomap(np, JPEG_DECODER_REG);
	jpu_check_null_return(jpu_device->jpu_dec_base, -ENXIO);

	jpu_device->jpu_top_base = of_iomap(np, JPEG_TOP_REG);
	jpu_check_null_return(jpu_device->jpu_top_base, -ENXIO);

	jpu_device->jpu_cvdr_base = of_iomap(np, JPEG_CVDR_REG);
	jpu_check_null_return(jpu_device->jpu_cvdr_base, -ENXIO);

	jpu_device->jpu_smmu_base = of_iomap(np, JPEG_SMMU_REG);
	jpu_check_null_return(jpu_device->jpu_smmu_base, -ENXIO);

	jpu_device->media1_crg_base = of_iomap(np, JPEG_MEDIA1_REG);
	jpu_check_null_return(jpu_device->media1_crg_base, -ENXIO);

	jpu_device->peri_crg_base = of_iomap(np, JPEG_PERI_REG);
	jpu_check_null_return(jpu_device->peri_crg_base, -ENXIO);

	jpu_device->pmctrl_base = of_iomap(np, JPEG_PMCTRL_REG);
	jpu_check_null_return(jpu_device->pmctrl_base, -ENXIO);

	jpu_device->sctrl_base = of_iomap(np, JPEG_SCTRL_REG);
	jpu_check_null_return(jpu_device->sctrl_base, -ENXIO);

#if defined(CONFIG_DPU_FB_V600) || defined(CONFIG_DKMD_DPU_VERSION)
	jpu_device->secadapt_base = of_iomap(np, JPEG_SECADAPT_REG);
	jpu_check_null_return(jpu_device->secadapt_base, -ENXIO);
	jpu_info("secadapt_base: 0x%pK\n", jpu_device->secadapt_base);
#endif

	return 0;
}

#if defined(CONFIG_DPU_FB_V600) || defined(CONFIG_DKMD_DPU_VERSION)
static int jpu_get_secadapt_prop(struct jpu_data_type *jpu_device,
	struct device_node *np)
{
	uint32_t base_array[MAX_SECADAPT_SWID_NUM] = {0};
	size_t count = 2; /* sid to ssid, num = 2 */

	if (of_property_read_u32_array(np, "sid-ssid", base_array, count) < 0) {
		jpu_err("failed to get sid-ssid\n");
		return -ENXIO;
	}

	jpu_device->jpgd_secadapt_prop.sid = base_array[0];
	jpu_device->jpgd_secadapt_prop.ssid_ns = base_array[1];
	jpu_info("sid: %u, ssidv_ns: %u\n", base_array[0], base_array[1]);

	count = MAX_SECADAPT_SWID_NUM;
	if (of_property_read_u32_array(np, "jpgd-swid", base_array, count) < 0) {
		jpu_err("failed get sid-ssid property\n");
		return -ENXIO;
	}

	jpu_device->jpgd_secadapt_prop.swid[0] = base_array[0];
	jpu_device->jpgd_secadapt_prop.swid[1] = base_array[1]; /* 1 is index */
	jpu_device->jpgd_secadapt_prop.swid[2] = base_array[2]; /* 2 is index */
	jpu_device->jpgd_secadapt_prop.swid[3] = base_array[3]; /* 3 is index */
	jpu_device->jpgd_secadapt_prop.swid[4] = base_array[4]; /* 4 is index */
	jpu_info("swid: %u %u %u %u %u\n", base_array[0], base_array[1],
		base_array[2], base_array[3], base_array[4]);

	return 0;
}
#endif

static int jpu_get_regulators(struct jpu_data_type *jpu_device)
{
	/* get jpu regulator */
	jpu_device->jpu_regulator = devm_regulator_get(&(jpu_device->pdev->dev),
		"jpu-regulator");
	if (IS_ERR(jpu_device->jpu_regulator)) {
		jpu_err("failed to get jpu_regulator\n");
		return -ENXIO;
	}

	/* get media1 regulator */
	jpu_device->media1_regulator = devm_regulator_get(&(jpu_device->pdev->dev),
		"media1-regulator");
	if (IS_ERR(jpu_device->media1_regulator)) {
		jpu_err("failed to get media1_regulator\n");
		return -ENXIO;
	}

	return 0;
}

static void jpu_put_regulators(struct jpu_data_type *jpu_device)
{
	if (jpu_device->jpu_regulator != NULL)
		devm_regulator_put(jpu_device->jpu_regulator);

	if (jpu_device->media1_regulator != NULL)
		devm_regulator_put(jpu_device->media1_regulator);
}

static int jpu_remove(struct platform_device *pdev)
{
	struct jpu_data_type *jpu_device = NULL;

	jpu_info("+\n");
	jpu_check_null_return(pdev, -ENXIO);

	jpu_device = platform_get_drvdata(pdev);
	jpu_check_null_return(jpu_device, -EINVAL);

	if (jpu_unregister(jpu_device) != 0) {
		jpu_err("jpu_unregister failed\n");
		return -EINVAL;
	}

	jpu_chrdev_remove(jpu_device);

	/* clk handle */
	if ((jpu_device->jpg_func_clk != NULL) && (jpu_device->pdev != NULL))
		devm_clk_put(&(jpu_device->pdev->dev), jpu_device->jpg_func_clk);

	jpu_put_regulators(jpu_device);

	if (jpu_device != NULL) {
		kfree(jpu_device);
		jpu_device = NULL;
		platform_set_drvdata(pdev, jpu_device);
	}
	g_jpu_device = NULL;
	jpu_info("-\n");
	return 0;
}

static int jpu_get_reg_name_resource(struct device_node *np,
	struct jpu_data_type *jpu_device, struct device *dev)
{
	int ret = 0;

	if (jpu_get_reg_base(jpu_device, np)) {
		dev_err(dev, "failed to get reg base resource\n");
		return -ENXIO;
	}
	if (jpu_get_regulators(jpu_device)) {
		dev_err(dev, "failed to get jpu regulator\n");
		return -ENXIO;
	}

#if (!defined CONFIG_DPU_FB_V600) && (!defined CONFIG_DKMD_DPU_VERSION)
	/* get jpg_func_clk_name resource */
	ret = of_property_read_string_index(np, "clock-names", 0,
		&(jpu_device->jpg_func_clk_name));
	if (ret != 0) {
		dev_err(dev, "failed to get jpg_func_clk_name resource ret = %d\n",
			ret);
		return -ENXIO;
	}
#endif

	/* get jpg platfrom name resource */
	if (jpu_set_platform(jpu_device, np) != 0) {
		dev_err(dev, "failed to set platform info\n");
		return -ENXIO;
	}
	return ret;
}

static int jpu_get_dts_resource(struct device_node *np,
	struct jpu_data_type *jpu_device, struct device *dev)
{
	if (of_property_read_u32(np, "fpga_flag", &(jpu_device->fpga_flag)) != 0) {
		dev_err(dev, "failed to get fpga_flag resource\n");
		return -ENXIO;
	}
	jpu_info("fpga_flag = %u\n", jpu_device->fpga_flag);

	if (jpu_get_reg_name_resource(np, jpu_device, dev) != 0)
		return -ENXIO;

	/* get irq no */
	if (jpgdec_get_irq(jpu_device, np)) {
		dev_err(dev, "failed to get jpu irq\n");
		return -ENXIO;
	}

#if defined(CONFIG_DPU_FB_V600) || defined(CONFIG_DKMD_DPU_VERSION)
	if (jpu_get_secadapt_prop(jpu_device, np)) {
		dev_err(dev, "failed to get secadapt_prop\n");
		return -ENXIO;
	}
#else
	jpu_device->jpg_func_clk = devm_clk_get(&(jpu_device->pdev->dev),
		jpu_device->jpg_func_clk_name);
	if (IS_ERR(jpu_device->jpg_func_clk)) {
		dev_err(dev, "jpg_func_clk devm_clk_get error");
		return -ENXIO;
	}
#endif

	if (jpu_chrdev_setup(jpu_device) != 0) {
		dev_err(dev, "fail to jpu_chrdev_setup\n");
		return -ENXIO;
	}
	return 0;
}

static int jpu_dev_alloc(struct device *dev,
	struct device_node **np, struct jpu_data_type **jpu_device)
{
	*np = of_find_compatible_node(NULL, NULL, DTS_COMP_JPU_NAME);
	if (*np == NULL) {
		dev_err(dev, "NOT FOUND device node %s\n", DTS_COMP_JPU_NAME);
		return -ENXIO;
	}

	*jpu_device = (struct jpu_data_type *)kzalloc(
		sizeof(struct jpu_data_type), GFP_KERNEL);
	if (*jpu_device == NULL) {
		dev_err(dev, "failed to alloc jpu_device\n");
		return -ENXIO;
	}
	return 0;
}

static int jpu_probe(struct platform_device *pdev)
{
	int ret;
	struct device *dev = NULL;
	struct device_node *np = NULL;
	struct jpu_data_type *jpu_device = NULL;

	jpu_info("+\n");
	jpu_check_null_return(pdev, -ENXIO);

	dev = &pdev->dev;
	if (jpu_dev_alloc(dev, &np, &jpu_device) != 0)
		return -ENXIO;

	/* cppcheck-suppress */
	(void)memset_s(jpu_device, sizeof(struct jpu_data_type),
		0, sizeof(struct jpu_data_type));
	jpu_device->pdev = pdev;

	/* get jpu dts config */
	ret = jpu_get_dts_resource(np, jpu_device, dev);
	if (ret != 0) {
		dev_err(dev, "failed to get dts resource\n");
		goto err_device_put;
	}

	platform_set_drvdata(pdev, jpu_device);
	g_jpu_device = platform_get_drvdata(pdev);
	if (g_jpu_device == NULL) {
		dev_err(dev, "jpu_device load and reload failed\n");
		ret = -ENXIO;
		goto err_device_put;
	}

	/* jpu register */
	ret = jpu_register(jpu_device);
	if (ret != 0) {
		dev_err(dev, "fail to jpu_register\n");
		goto err_device_put;
	}
	jpu_info("-\n");
	return 0;
err_device_put:
	if (jpu_remove(pdev) != 0)
		dev_err(dev, "jpu_remove failed\n");

	jpu_device = platform_get_drvdata(pdev);
	if (jpu_device != NULL) {
		kfree(jpu_device);
		jpu_device = NULL;
		platform_set_drvdata(pdev, jpu_device);
	}
	g_jpu_device = NULL;
	return ret;
}

#if defined(CONFIG_PM_SLEEP)
static int jpu_suspend(struct device *dev)
{
	struct jpu_data_type *jpu_device = NULL;

	jpu_info("+\n");
	jpu_check_null_return(dev, -EINVAL);

	jpu_device = dev_get_drvdata(dev);
	jpu_check_null_return(jpu_device, -EINVAL);

	jpu_info("-\n");
	return 0;
}
#else
#define jpu_suspend NULL
#endif

static void jpu_shutdown(struct platform_device *pdev)
{
	struct jpu_data_type *jpu_device = NULL;
	jpu_info("+\n");
	if (pdev == NULL) {
		jpu_err("pdev is NULL\n");
		return;
	}

	jpu_device = platform_get_drvdata(pdev);
	if (jpu_device == NULL)
		jpu_err("jpu_device is NULL\n");

	jpu_info("-\n");
}

static const struct of_device_id jpu_match_table[] = {
	{
		.compatible = DTS_COMP_JPU_NAME,
		.data = NULL,
	},
	{},
};

MODULE_DEVICE_TABLE(of, jpu_match_table);

static const struct dev_pm_ops jpu_dev_pm_ops = {
#ifdef CONFIG_PM_RUNTIME
	.runtime_suspend = NULL,
	.runtime_resume = NULL,
	.runtime_idle = NULL,
#endif
#ifdef CONFIG_PM_SLEEP
	.suspend = jpu_suspend,
	.resume = NULL,
#endif
};

static struct platform_driver jpu_driver = {
	.probe = jpu_probe,
	.remove = jpu_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend = NULL,
	.resume = NULL,
#endif
	.shutdown = jpu_shutdown,
	.driver = {
		.name = DEV_NAME_JPU,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(jpu_match_table),
		.pm = &jpu_dev_pm_ops,
	},
};

static int __init jpu_init(void)
{
	int ret = platform_driver_register(&jpu_driver);
	if (ret != 0) {
		jpu_err("driver register failed, error=%d\n", ret);
		return ret;
	}

	return ret;
}

static void __exit jpu_exit(void)
{
	platform_driver_unregister(&jpu_driver);
}

module_init(jpu_init);
module_exit(jpu_exit);

MODULE_DESCRIPTION("JPU Driver");
MODULE_LICENSE("GPL v2");
