/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2021. All rights reserved.
 * Description: This file describe GPU related init
 * Create: 2014-2-24
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */

#include <linux/ioport.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/of_address.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <device/mali_kbase_device.h>
#include "mali_kbase_config_platform.h"
#include "mali_kbase_config_hifeatures.h"
#include <linux/platform_drivers/gpufreq_v2.h>
#include <backend/gpu/mali_kbase_pm_internal.h>

#ifdef CONFIG_CMDLINE_PARSE
/* for runmode_is_factory */
#include <platform_include/basicplatform/linux/hw_cmdline_parse.h>
#endif

#define RUNTIME_PM_DELAY_SHORT    1  //ms
#define RUNTIME_PM_DELAY_LONG     30 //ms

#define HARD_RESET_AT_POWER_OFF   0

typedef uint32_t mali_bool;

#ifdef CONFIG_MALI_FENCE_DEBUG
struct kbase_device *kbase_dev;
#else
static struct kbase_device *kbase_dev;
#endif

typedef enum {
	MALI_ERROR_NONE = 0,
	MALI_ERROR_OUT_OF_GPU_MEMORY,
	MALI_ERROR_OUT_OF_MEMORY,
	MALI_ERROR_FUNCTION_FAILED,
} mali_error;

#ifndef CONFIG_OF
static struct kbase_io_resources io_resources = {
	.job_irq_number = 68, // irq number 68
	.mmu_irq_number = 69, // irq number 69
	.gpu_irq_number = 70, // irq number 70
	.io_memory_region = {
		.start = 0xFC010000,
		.end = 0xFC010000 + (4096 * 4) - 1 // end address
	}
};
#endif /* CONFIG_OF */

static int kbase_set_hi_features_mask(struct kbase_device *kbdev)
{
	const enum kbase_hi_feature *hi_features = NULL;
	u32 gpu_vid;

	gpu_vid = kbdev->gpu_dev_data.gpu_vid;

	switch (gpu_vid) {
		/* arch_major, arch_minor, arch_rev, product_major,
		 * version_major, version_minor, version_status
		 */
		case GPU_ID2_MAKE(6, 2, 2, 1, 0, 0, 0):
			hi_features = kbase_hi_feature_thex_r0p0;
			break;
		case GPU_ID2_MAKE(6, 2, 2, 1, 0, 0, 1):
			hi_features = kbase_hi_feature_thex_r0p0;
			break;
		case GPU_ID2_MAKE(7, 2, 1, 1, 0, 0, 0):
			hi_features = kbase_hi_feature_tnox_r0p0;
			break;
		case GPU_ID2_MAKE(7, 4, 0, 2, 1, 0, 0):
			hi_features = kbase_hi_feature_tgox_r1p0;
			break;
		case GPU_ID2_MAKE(7, 0, 9, 0, 1, 1, 0):
			hi_features = kbase_hi_feature_tsix_r1p1;
			break;
		case GPU_ID2_MAKE(9, 0, 8, 0, 0, 1, 1):
			hi_features = kbase_hi_feature_ttrx_r0p1;
			break;
		case GPU_ID2_MAKE(9, 2, 0, 2, 0, 0, 2):
			hi_features = kbase_hi_feature_tbex_r0p0;
			break;
		case GPU_ID2_MAKE(9, 2, 0, 2, 0, 0, 3):
			hi_features = kbase_hi_feature_tbex_r0p0;
			break;
		case GPU_ID2_MAKE(9, 2, 0, 2, 0, 1, 0):
			hi_features = kbase_hi_feature_tbex_r0p1;
			break;
		// only for tnax FPGA, because FPGA GPU ID is 90810011 different from ASIC
		case GPU_ID2_MAKE(9, 0, 8, 1, 0, 1, 1):
		case GPU_ID2_MAKE(9, 0, 9, 1, 0, 1, 0):
			hi_features = kbase_hi_feature_tnax_r0p1;
			break;
		default:
			dev_err(kbdev->dev,
				"[hi-feature] Unknown GPU ID %x", gpu_vid);
			return -EINVAL;
	}

	dev_info(kbdev->dev,
		"[hi-feature] GPU identified as 0x%04x r%dp%d status %d",
		((gpu_vid & GPU_ID_VERSION_PRODUCT_ID) >>
			GPU_ID_VERSION_PRODUCT_ID_SHIFT),
		(gpu_vid & GPU_ID_VERSION_MAJOR) >> GPU_ID_VERSION_MAJOR_SHIFT,
		(gpu_vid & GPU_ID_VERSION_MINOR) >> GPU_ID_VERSION_MINOR_SHIFT,
		((gpu_vid & GPU_ID_VERSION_STATUS) >>
			GPU_ID_VERSION_STATUS_SHIFT));

	for (; *hi_features != KBASE_FEATURE_MAX_COUNT; hi_features++) {
		set_bit(*hi_features, &kbdev->gpu_dev_data.hi_features_mask[0]);
	}

	return 0;
}

/*
 * This function enables gpu special feature.
 * Note that those features must enable every time when power on
 */
static void kbase_gpu_feature_enable(struct kbase_device *kbdev)
{
	/* config power key if mtcos enable or cs2 chip type */
	if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_CORE_READY_PERIOD) ||
			(kbdev->gpu_dev_data.gpu_chip_type == 2)) {
		kbase_reg_write(kbdev, GPU_CONTROL_REG(PWR_KEY),
			KBASE_PWR_KEY_VALUE);
		kbase_reg_write(kbdev, GPU_CONTROL_REG(PWR_OVERRIDE1),
			KBASE_PWR_OVERRIDE_VALUE);
	}
}

/*
 * Init gpu special feature.
 * Note that those features should init only once when system start-up
 */
static void kbase_gpu_feature_init(struct kbase_device *kbdev)
{
	/* enable g3d memory auto hardware shutdown */
	/* norr cs read PERI_CTRL93 set [31]bit to 1
	 * trym read PERI_CTRL93 and gondul read PERI_CTRL91 set [1]bit to 1
	 */
	if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_AUTO_MEM_SHUTDOWN)) {
		u32 value = readl(kbdev->gpu_dev_data.pctrlreg + PERICTRL_93_OFFSET);
		value |= G3D_MEM_AUTO_SD_ENABLE;
		writel(value, kbdev->gpu_dev_data.pctrlreg + PERICTRL_93_OFFSET);
	}

	if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_STRIPING_GRANUL_SETTING)) {
		/* read PERI_CTRL19 and set it's [11:9]bit to 1
		 * set striping granule to hash function with 256 byte
		 */
		u32 value = readl(kbdev->gpu_dev_data.pctrlreg + PERICTRL_19_OFFSET) | HASH_STRIPING_GRANULE;
		writel(value, kbdev->gpu_dev_data.pctrlreg + PERICTRL_19_OFFSET);
	}
}

static inline void kbase_platform_on(struct kbase_device *kbdev)
{
	int refcount = atomic_read(&kbdev->gpu_dev_data.regulator_refcount);

	if (kbdev->gpu_dev_data.regulator && !refcount) {
		int loop = 0;
		const int max_loop = 3;
		const int delay_step = 5;
		while (loop < max_loop && unlikely(regulator_enable(kbdev->gpu_dev_data.regulator))) {
			dev_err(kbdev->dev, "Failed to enable regulator, retry...%d", ++loop);
			udelay(delay_step * loop);
		}
		BUG_ON(loop == max_loop);

		atomic_inc(&kbdev->gpu_dev_data.regulator_refcount);

		if (kbdev->gpu_dev_data.gpu_vid == 0) {
			kbdev->gpu_dev_data.gpu_vid = kbase_reg_read(kbdev,
				GPU_CONTROL_REG(GPU_ID));

			/* let's return the same hwver to software umd */
			if (kbdev->gpu_dev_data.gpu_vid == BORR_ES_ID)
				kbdev->gpu_dev_data.gpu_vid = BORR_CS_ID;

			if (unlikely(kbase_set_hi_features_mask(kbdev)))
				dev_err(kbdev->dev,
					"Failed to set hi features\n");
			/* init special feature only once */
			kbase_gpu_feature_init(kbdev);
		}

		/* enable special feature */
		kbase_gpu_feature_enable(kbdev);
#ifdef CONFIG_PM_DEVFREQ
#ifdef CONFIG_GPUFREQ_INDEPDENT
		gpufreq_devfreq_resume();
#endif
#elif defined(CONFIG_MALI_MIDGARD_DVFS)
		if (kbase_platform_dvfs_enable(kbdev, true, 0) != MALI_TRUE)
			return -EPERM;
#endif
	} else {
		dev_err(kbdev->dev,"%s called zero, refcount:[%d]\n",__func__, refcount);
	}
}

static inline void kbase_platform_off(struct kbase_device *kbdev)
{
	int refcount = atomic_read(&kbdev->gpu_dev_data.regulator_refcount);

	if (kbdev->gpu_dev_data.regulator && refcount) {
		int loop = 0;
		const int max_loop = 3;
		const int delay_step = 5;
#ifdef CONFIG_PM_DEVFREQ
#ifdef CONFIG_GPUFREQ_INDEPDENT
		gpufreq_devfreq_suspend();
#endif
#elif defined(CONFIG_MALI_MIDGARD_DVFS)
		kbase_platform_dvfs_enable(kbdev, false, 0);
#endif

		while (loop < max_loop && unlikely(regulator_disable(kbdev->gpu_dev_data.regulator))) {
			dev_err(kbdev->dev, "Failed to disable regulator, retry...%d", ++loop);
			udelay(delay_step * loop);
		}

		refcount = atomic_dec_return(&kbdev->gpu_dev_data.regulator_refcount);
		if (unlikely(refcount != 0))
			dev_err(kbdev->dev,
				"%s called not match, refcount:[%d]\n",
				__func__, refcount);
	}
}

/* Init GPU platform related register map */
static int kbase_gpu_register_map(struct kbase_device *kbdev)
{
	int err = 0;

	kbdev->gpu_dev_data.crgreg = ioremap(SYS_REG_PERICRG_BASE_ADDR,
		SYS_REG_REMAP_SIZE);
	if (!kbdev->gpu_dev_data.crgreg) {
		dev_err(kbdev->dev, "Can't remap sys crg register window on platform\n");
		err = -EINVAL;
		goto out_crg_ioremap;
	}

	kbdev->gpu_dev_data.pmctrlreg = ioremap(SYS_REG_PMCTRL_BASE_ADDR,
			SYS_REG_REMAP_SIZE);
	if (!kbdev->gpu_dev_data.pmctrlreg) {
		dev_err(kbdev->dev, "Can't remap sys pmctrl register window on platform\n");
		err = -EINVAL;
		goto out_pmctrl_ioremap;
	}

	if (kbdev->gpu_dev_data.gpu_chip_type == 2)
		kbdev->gpu_dev_data.pctrlreg = ioremap(SYS_REG_PERICTRL_BASE_ADDR_CS2,
			SYS_REG_REMAP_SIZE);
	else
		kbdev->gpu_dev_data.pctrlreg = ioremap(SYS_REG_PERICTRL_BASE_ADDR,
			SYS_REG_REMAP_SIZE);

	if (!kbdev->gpu_dev_data.pctrlreg) {
		dev_err(kbdev->dev, "Can't remap sys pctrl register window on platform\n");
		err = -EINVAL;
		goto out_pctrl_ioremap;
	}

	return err;

out_pctrl_ioremap:
	iounmap(kbdev->gpu_dev_data.pmctrlreg);

out_pmctrl_ioremap:
	iounmap(kbdev->gpu_dev_data.crgreg);

out_crg_ioremap:
	return err;
}

static void kbase_gpu_register_unmap(struct kbase_device *kbdev)
{
	if (kbdev->gpu_dev_data.crgreg != NULL)
		iounmap(kbdev->gpu_dev_data.crgreg);

	if (kbdev->gpu_dev_data.pmctrlreg != NULL)
		iounmap(kbdev->gpu_dev_data.pmctrlreg);

	if (kbdev->gpu_dev_data.pctrlreg != NULL)
		iounmap(kbdev->gpu_dev_data.pctrlreg);
}

static int kbase_gpu_verify_fpga_exist(struct kbase_device *kbdev)
{
	int ret = 0;

	if (kbdev->gpu_dev_data.gpu_fpga_exist) {
		unsigned int pctrl_value;
		pctrl_value = readl(kbdev->gpu_dev_data.pctrlreg +
			PERI_STAT_FPGA_GPU_EXIST) & PERI_STAT_FPGA_GPU_EXIST_MASK;
		if (pctrl_value == 0) {
			dev_err(kbdev->dev, "No FPGA FOR GPU\n");
			return -ENODEV;
		}
	}
	return ret;
}

static int kbase_gpu_init_device_tree(struct kbase_device * const kbdev)
{
	struct device_node *np = NULL;
	int ret;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	/* init soc special flag */
	kbdev->gpu_dev_data.gpu_fpga_exist = 0;
	kbdev->gpu_dev_data.gpu_chip_type = 0;

#ifdef CONFIG_GPU_SHADER_PRESENT_CFG
	kbdev->gpu_dev_data.shader_present_lo_cfg = 0;
#endif

#ifdef CONFIG_OF
	/* read outstanding value from dts*/
	np = of_find_compatible_node(NULL, NULL, "arm,mali-midgard");
	if (np) {
		ret = of_property_read_u32(np, "fpga-gpu-exist",
			&kbdev->gpu_dev_data.gpu_fpga_exist);
		if (ret)
			dev_warn(kbdev->dev,
			"No fpga gpu exist setting, assume not exist\n");

		ret = of_property_read_u32(np, "gpu-chip-type",
			&kbdev->gpu_dev_data.gpu_chip_type);
		if (ret)
			dev_warn(kbdev->dev,
			"No gpu chip type setting, default means cs\n");

	} else {
		dev_err(kbdev->dev,
			"not find device node arm,mali-midgard!\n");
	}
#endif

	return 0;
}

#ifdef CONFIG_HUAWEI_DSM
static struct dsm_dev dsm_gpu = {
	.name = "dsm_gpu",
	.device_name = "gpu",
	.ic_name = NULL,
	.module_name = NULL,
	.fops = NULL,
	.buff_size = 1024,
};
#endif

static int kbase_platform_backend_init(struct kbase_device *kbdev)
{
	int err;

	// init gpu_outstanding, fpga_exist, chip_type and others from device tree
	err = kbase_gpu_init_device_tree(kbdev);
	if (err != 0) {
		dev_err(kbdev->dev, "Init special devce tree setting failed\n");
		return err;
	}

	// register map for GPU, especially for chip_type related register
	err = kbase_gpu_register_map(kbdev);
	if (err != 0) {
		dev_err(kbdev->dev, "Can't remap gpu register\n");
		return err;
	}

	// verify if FPGA, need use info of related register
	err = kbase_gpu_verify_fpga_exist(kbdev);
	if (err != 0) {
		dev_err(kbdev->dev, "No gpu hardware implementation on fpga\n");
		return -ENODEV;
	}

#ifdef CONFIG_HUAWEI_DSM
	kbdev->gpu_dev_data.gpu_dsm_client = dsm_register_client(&dsm_gpu);
#ifdef CONFIG_CMDLINE_PARSE
	kbdev->gpu_dev_data.factory_mode = runmode_is_factory();
#endif
#endif

	return err;
}

static void kbase_platform_backend_term(struct kbase_device *kbdev)
{
	kbase_gpu_register_unmap(kbdev);
}

static int kbase_get_busytime(void)
{
	struct kbase_device *kbdev = kbase_dev;
	unsigned long cur_freq;
	int busy_time;

	KBASE_DEBUG_ASSERT(kbdev != NULL);
	if (kbdev->pm.backend.metrics.kbdev != kbdev) {
		pr_err("%s pm backend metrics not initialized\n", __func__);
		return -EINVAL;
	}
#ifdef CONFIG_MALI_MIDGARD_DVFS
	(void)kbase_pm_get_dvfs_action(kbdev);
#endif
	busy_time = kbdev->pm.backend.metrics.utilisation;
	cur_freq = gpufreq_get_cur_freq();
	return busy_time;
}

static int kbase_get_vsync_hit(void)
{
	struct kbase_device *kbdev = kbase_dev;

	KBASE_DEBUG_ASSERT(kbdev != NULL);
	if (kbdev->pm.backend.metrics.kbdev != kbdev) {
		pr_err("%s pm backend metrics not initialized\n", __func__);
		return -EINVAL;
	}
#ifdef CONFIG_MALI_MIDGARD_DVFS
	(void)kbase_pm_get_dvfs_action(kbdev);
#endif
	return kbdev->pm.backend.metrics.vsync_hit;
}

static int kbase_queue_coremask_work(void)
{
	return 0;
}

int gpufreq_init(void)
{
#ifdef CONFIG_GPUFREQ_INDEPDENT
	int ret;

	ret = gpufreq_register_callback(GF_CB_BUSYTIME, kbase_get_busytime);
	if (ret != 0) {
		pr_err("%s get_busytime callback register failed \n", __func__);
		return ret;
	}

	ret = gpufreq_register_callback(GF_CB_VSYNC_HIT, kbase_get_vsync_hit);
	if (ret != 0) {
		pr_err("%s get_vsync_hit callback register failed \n", __func__);
		return ret;
	}

	ret = gpufreq_register_callback(GF_CB_CORE_MASK, kbase_queue_coremask_work);
	if (ret != 0) {
		pr_err("%s kbase_queue_coremask_work callback register failed \n", __func__);
		return ret;
	}

	return gpufreq_start();
#else
	return 0;
#endif
}

static int kbase_platform_init(struct kbase_device *kbdev)
{
	struct device *dev = NULL;

	KBASE_DEBUG_ASSERT(kbdev != NULL);
	dev = kbdev->dev;
	dev->platform_data = kbdev;

	/* Init the platform related data first. */
	if (kbase_platform_backend_init(kbdev)) {
		dev_err(kbdev->dev, "platform backend init failed.\n");
		return -EINVAL;
	}

	kbdev->gpu_dev_data.regulator = devm_regulator_get(dev, "gpu");
	if (IS_ERR(kbdev->gpu_dev_data.regulator)) {
		dev_err(kbdev->dev, "Failed to get regulator\n");
		return -EINVAL;
	}
	atomic_set(&kbdev->gpu_dev_data.runtime_pm_delay_ms, RUNTIME_PM_DELAY_LONG);

	atomic_set(&kbdev->gpu_dev_data.regulator_refcount, 0);

	kbase_dev = kbdev;
	if (gpufreq_init())
		dev_err(kbdev->dev, "gpufreq init failed, dvfs may not works.\n");
	/* dev name maybe modified by gpufreq_init */
	dev_set_name(dev, "gpu");
	return 0;
}

/* Get kbase_device, the caller should check the return value */
struct kbase_device *kbase_platform_get_device(void)
{
	return kbase_dev;
}

#ifdef CONFIG_MALI_FENCE_DEBUG
/* Get kbase_device, the caller should check the return value */
struct kbase_device *kbase_platform_get_device(void)
{
	return kbase_dev;
}
#endif

static void kbase_platform_term(struct kbase_device *kbdev)
{
	KBASE_DEBUG_ASSERT(kbdev != NULL);
#ifdef CONFIG_GPUFREQ_INDEPDENT
	gpufreq_term();
#endif
	/* term the platform related data at last. */
	kbase_platform_backend_term(kbdev);
}

struct kbase_platform_funcs_conf platform_funcs = {
	.platform_init_func = &kbase_platform_init,
	.platform_term_func = &kbase_platform_term,
};

static int pm_callback_power_on(struct kbase_device *kbdev)
{
#ifdef CONFIG_MALI_MIDGARD_RT_PM
	int result;
	struct device *dev = kbdev->dev;

	// disable g3d memory dslp before shader core poweron
	if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_AUTO_MEM_SHUTDOWN)) {
		u32 value = readl(kbdev->gpu_dev_data.pctrlreg + PERICTRL_92_OFFSET);
		value &= G3D_MEM_DSLP_DISABLE;
		writel(value, kbdev->gpu_dev_data.pctrlreg + PERICTRL_92_OFFSET);
	}

	if (unlikely(dev->power.disable_depth > 0)) {
		kbase_platform_on(kbdev);
	} else {
		result = pm_runtime_resume(dev);
		if (result < 0 && result == -EAGAIN)
			kbase_platform_on(kbdev);
		else if (result < 0)
			pr_err("[mali]pm_runtime_resume failed with result %d\n",
				result);
	}
#else
	kbase_platform_on(kbdev);
#endif
	return 1;
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
#ifdef CONFIG_MALI_MIDGARD_RT_PM
	struct device *dev = kbdev->dev;
	int ret;
	int retry = 0;
	unsigned int delay_ms =
		atomic_read(&kbdev->gpu_dev_data.runtime_pm_delay_ms);

	// enable g3d memory dslp after shader core poweroff
	if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_AUTO_MEM_SHUTDOWN)) {
		u32 value = readl(kbdev->gpu_dev_data.pctrlreg + PERICTRL_92_OFFSET);
		value |= G3D_MEM_DSLP_ENABLE;
		writel(value, kbdev->gpu_dev_data.pctrlreg + PERICTRL_92_OFFSET);
	}

#if HARD_RESET_AT_POWER_OFF
	/* Cause a GPU hard reset to test whether we have actually idled the GPU
	 * and that we properly reconfigure the GPU on power up.
	 * Usually this would be dangerous, but if the GPU is working correctly
	 * it should be completely safe as the GPU should not be active at this
	 * point. However this is disabled normally because it will most likely
	 * interfere with bus logging etc.
	 */
	KBASE_KTRACE_ADD(kbdev, CORE_GPU_HARD_RESET, NULL, 0u);
	kbase_reg_write(kbdev, GPU_CONTROL_REG(GPU_COMMAND),
		GPU_COMMAND_HARD_RESET);
#endif

	if (unlikely(dev->power.disable_depth > 0)) {
		kbase_platform_off(kbdev);
		return;
	}
	do {
		if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_POWER_OFF_DIRECTLY))
			ret = pm_runtime_suspend(dev);
		else
			ret = pm_schedule_suspend(dev, delay_ms);
		if (ret != -EAGAIN) {
			if (unlikely(ret < 0)) {
				pr_err("[mali] pm_schedule_suspend fail %d\n", ret);
				WARN_ON(1);
			}
			/* correct status */
			break;
		}
		/* -EAGAIN, repeated attempts for 1s totally */
		msleep(50); /* sleep 50 ms */
	} while (++retry < 20); /* loop 20 */
#else
	kbase_platform_off(kbdev);
#endif
}

static int pm_callback_runtime_init(struct kbase_device *kbdev)
{
	KBASE_DEBUG_ASSERT(kbdev != NULL);
	pm_suspend_ignore_children(kbdev->dev, true);
	pm_runtime_enable(kbdev->dev);
	return 0;
}

static void pm_callback_runtime_term(struct kbase_device *kbdev)
{
	KBASE_DEBUG_ASSERT(kbdev != NULL);
	pm_runtime_disable(kbdev->dev);
}

static void pm_callback_runtime_off(struct kbase_device *kbdev)
{
	kbase_platform_off(kbdev);
}

static int pm_callback_runtime_on(struct kbase_device *kbdev)
{
	/* norr cs, trym es, gondul */
	if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_AUTO_MEM_SHUTDOWN)) {
		unsigned int value;
		/**
		 * disable deep sleep by software
		 * norr cs read PERI_CTRL93 set [1~17]bit to 0,
		 * trym read PERI_CTRL92 set [0~16]bit to 0,
		 * gondul read PERI_CTRL92 set [0~6]bit to 0
		 */
		value = readl(kbdev->gpu_dev_data.pctrlreg + DEEP_SLEEP_BYSW) &
			MASK_DISABLEDSBYSF;
		writel(value, kbdev->gpu_dev_data.pctrlreg + DEEP_SLEEP_BYSW);
	}

	kbase_platform_on(kbdev);

	return 0;
}

static inline void pm_callback_suspend(struct kbase_device *kbdev)
{
#ifdef CONFIG_MALI_MIDGARD_RT_PM
	pm_callback_runtime_off(kbdev);
#else
	pm_callback_power_off(kbdev);
#endif
}

static inline void pm_callback_resume(struct kbase_device *kbdev)
{
#ifdef CONFIG_MALI_MIDGARD_RT_PM
	if (!pm_runtime_status_suspended(kbdev->dev))
		pm_callback_runtime_on(kbdev);
	else
		pm_callback_power_on(kbdev);
#else
	pm_callback_power_on(kbdev);
#endif
}

static inline int pm_callback_runtime_idle(struct kbase_device *kbdev)
{
	return 1;
}

struct kbase_pm_callback_conf pm_callbacks = {
	.power_on_callback = pm_callback_power_on,
	.power_off_callback = pm_callback_power_off,
	.power_suspend_callback = pm_callback_suspend,
	.power_resume_callback = pm_callback_resume,
#ifdef CONFIG_MALI_MIDGARD_RT_PM
	.power_runtime_init_callback = pm_callback_runtime_init,
	.power_runtime_term_callback = pm_callback_runtime_term,
	.power_runtime_off_callback = pm_callback_runtime_off,
	.power_runtime_on_callback = pm_callback_runtime_on,
	.power_runtime_idle_callback = pm_callback_runtime_idle
#else
	.power_runtime_init_callback = NULL,
	.power_runtime_term_callback = NULL,
	.power_runtime_off_callback = NULL,
	.power_runtime_on_callback = NULL,
	.power_runtime_idle_callback = NULL
#endif
};

static struct kbase_platform_config hi_platform_config = {
#ifndef CONFIG_OF
	.io_resources = &io_resources
#endif
};

struct kbase_platform_config *kbase_get_platform_config(void)
{
	return &hi_platform_config;
}

int kbase_platform_early_init(void)
{
	/* Nothing needed at this stage */
	return 0;
}
