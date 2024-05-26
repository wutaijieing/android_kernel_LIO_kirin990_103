/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2020. All rights reserved.
 * Description: This file describe HISI GPU related init
 * Author: Huawei Technologies Co., Ltd.
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
#ifdef CONFIG_GPU_IPA_THERMAL
#include <linux/devfreq_cooling.h>
#include <platform_include/maligpu/linux/gpu_ipa_thermal.h>
#endif
#include "mali_kbase_hisi_callback.h"
#include "mali_kbase_efuse.h"

#include <trace/events/power.h>
#if (KERNEL_VERSION(3, 13, 0) <= LINUX_VERSION_CODE)
#include <linux/pm_opp.h>
#else
#include <linux/opp.h>
#endif

#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_REPORT_VSYNC
#include <linux/export.h>
#endif
#include <linux/delay.h>
#include <linux/of_address.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <backend/gpu/mali_kbase_pm_internal.h>
#include <backend/gpu/mali_kbase_device_internal.h>
#include "mali_kbase_config_platform.h"
#include "mali_kbase_config_hifeatures.h"
#ifdef CONFIG_GPU_DPM_PCR
#include "dpm/mali_kbase_dpm.h"
#endif
#include <linux/platform_drivers/gpufreq_v2.h>
#ifdef CONFIG_CMDLINE_PARSE
/* for runmode_is_factory */
#include <platform_include/basicplatform/linux/hw_cmdline_parse.h>
#endif

#if defined(CONFIG_GPU_AI_FENCE_INFO) || \
	defined(CONFIG_GPU_VIRTUAL_DEVFREQ) || \
	defined(CONFIG_GPU_THROTTLE_DEVFREQ)
#include <platform_include/maligpu/linux/gpu_hook.h>
#endif

#define MALI_TRUE ((uint32_t)1)
#define MALI_FALSE ((uint32_t)0)

#define DEFAULT_POLLING_MS        20 // ms
#define HARD_RESET_AT_POWER_OFF   0

#define KHZ 1000 // KHz
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

	gpu_vid = kbdev->hisi_dev_data.gpu_vid;

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
		set_bit(*hi_features, &kbdev->hisi_dev_data.hi_features_mask[0]);
	}

	return 0;
}

#ifdef CONFIG_GPU_CORE_HOTPLUG
/* For Norr, default core mask is 0xFFFF */
#ifdef CONFIG_MALI_NORR_PHX
#define CORE_MASK_LEVEL_1	4
#define CORE_MASK_LEVEL_2	8
#define CORE_MASK_LEVEL_3	12
#define CORE_MASK_LEVEL_4	16

#define CORE_MASK_LEVEL_1_VALUE	0x1111
#define CORE_MASK_LEVEL_2_VALUE	0x3333
#define CORE_MASK_LEVEL_3_VALUE	0x7777
#define CORE_MASK_LEVEL_4_VALUE	0xFFFF
#endif

/* For Borr, default core mask is 0x77777777 */
#ifdef CONFIG_MALI_BORR
#define CORE_MASK_LEVEL_1       4
#define CORE_MASK_LEVEL_2       8
#define CORE_MASK_LEVEL_3       16
#define CORE_MASK_LEVEL_4       24

#define CORE_MASK_LEVEL_1_VALUE       0x1111
#define CORE_MASK_LEVEL_2_VALUE       0x11111111
#define CORE_MASK_LEVEL_3_VALUE       0x33333333
#define CORE_MASK_LEVEL_4_VALUE       0x77777777

#ifdef CONFIG_GPU_SHADER_PRESENT_CFG
static u64 hisi_gpu_thermal_select_coremask(struct kbase_device *kbdev,
	u64 updated_core_mask, u32 *allowed_cores, u32 set)
{
	u32 available_cores = kbdev->hisi_dev_data.shader_present_lo_cfg;
	u32 i = 0;
	u64 core_mask = updated_core_mask;

	while (i < set) {
		if ((allowed_cores[i] & available_cores) == allowed_cores[i]) {
			core_mask = (u64)allowed_cores[i];
			break;
		}
		i++;
	}
	if (i == set) {
		core_mask &= (u64)available_cores;
		if (core_mask == 0)
			core_mask = (u64)available_cores;
		dev_err(kbdev->dev, "lite chip core_mask 0x%llx\n", core_mask);
	}
	return core_mask;
}

/* Get valid coremask for lite chips */
static u64 hisi_gpu_thermal_lite_coremask(struct kbase_device *kbdev,
	u64 updated_core_mask, u64 cores)
{
	u64 core_mask = updated_core_mask &
		(u64)kbdev->hisi_dev_data.shader_present_lo_cfg;

	switch (cores) {
	case CORE_MASK_LEVEL_1: {
		u32 allowed_cores[3] = { 0x1111, 0x2222, 0x4444 };
		u32 set = sizeof(allowed_cores) / sizeof(allowed_cores[0]);
		core_mask = hisi_gpu_thermal_select_coremask(kbdev,
			updated_core_mask, allowed_cores, set);
		break;
	}
	case CORE_MASK_LEVEL_2: {
		u32 allowed_cores[3] = { 0x11111111, 0x22222222, 0x44444444 };
		u32 set = sizeof(allowed_cores) / sizeof(allowed_cores[0]);
		core_mask = hisi_gpu_thermal_select_coremask(kbdev,
			updated_core_mask, allowed_cores, set);
		break;
	}
	case CORE_MASK_LEVEL_3: /* fall-through */
	case CORE_MASK_LEVEL_4:
		dev_err(kbdev->dev, "lite chip cores %llu core_mask 0x%llx\n",
			cores, core_mask);
		break;
	default:
		break;
	}

	return core_mask;
}
#endif
#endif

static u64 hisi_gpu_thermal_update_coremask(struct kbase_device *kbdev, u64 cores)
{
	u64 updated_core_mask = CORE_MASK_LEVEL_4_VALUE;

	switch (cores) {
	case CORE_MASK_LEVEL_1:
		updated_core_mask = CORE_MASK_LEVEL_1_VALUE;
		kbase_dev->pm.thermal_controlling = true;
		break;
	case CORE_MASK_LEVEL_2:
		updated_core_mask = CORE_MASK_LEVEL_2_VALUE;
		kbase_dev->pm.thermal_controlling = true;
		break;
	case CORE_MASK_LEVEL_3:
		updated_core_mask = CORE_MASK_LEVEL_3_VALUE;
		kbase_dev->pm.thermal_controlling = true;
		break;
	case CORE_MASK_LEVEL_4:
		updated_core_mask = CORE_MASK_LEVEL_4_VALUE;
		kbase_dev->pm.thermal_controlling = false;
		break;
	default:
		dev_err(kbdev->dev,
			"Invalid cores set 0x%llx\n", cores);
		break;
	}

#if defined(CONFIG_MALI_BORR) && defined(CONFIG_GPU_SHADER_PRESENT_CFG)
	updated_core_mask = hisi_gpu_thermal_lite_coremask(kbase_dev,
		updated_core_mask, cores);
#endif
	return updated_core_mask;
}

void gpu_thermal_cores_control(u64 cores)
{
	u64 updated_core_mask;
	unsigned long flags;

	if (kbase_dev == NULL) {
		pr_err("[Mali]kbase_dev is null,seems platform uninitialized");
		return;
	}

	updated_core_mask = hisi_gpu_thermal_update_coremask(kbase_dev, cores);

#ifdef CONFIG_GPU_VIRTUAL_DEVFREQ
	if (cores != CORE_MASK_LEVEL_4)
		gpu_virtual_devfreq_set_enable(false);
#endif

	kbase_dev->pm.thermal_required_core_mask = updated_core_mask;
	mutex_lock(&kbase_dev->pm.lock);
	spin_lock_irqsave(&kbase_dev->hwaccess_lock, flags);
	if (kbase_dev->pm.debug_core_mask[0] != updated_core_mask)
		kbase_pm_set_debug_core_mask(kbase_dev, updated_core_mask,
			updated_core_mask, updated_core_mask);
	spin_unlock_irqrestore(&kbase_dev->hwaccess_lock, flags);
	mutex_unlock(&kbase_dev->pm.lock);

#ifdef CONFIG_GPU_VIRTUAL_DEVFREQ
	if (cores == CORE_MASK_LEVEL_4)
		gpu_virtual_devfreq_set_enable(true);
#endif
}
#endif

#ifdef CONFIG_GPU_AI_FENCE_INFO
static int mali_kbase_report_fence_info(struct kbase_fence_info *fence)
{
	if (kbase_dev == NULL)
		return -EINVAL;

	if (kbase_dev->hisi_dev_data.game_pid != fence->game_pid)
		kbase_dev->hisi_dev_data.signaled_seqno = 0;

	kbase_dev->hisi_dev_data.game_pid = fence->game_pid;
	fence->signaled_seqno = kbase_dev->hisi_dev_data.signaled_seqno;

	return 0;
}

int perf_ctrl_get_gpu_fence(void __user *uarg)
{
	struct kbase_fence_info gpu_fence;
	int ret;

	if (uarg == NULL)
		return -EINVAL;

	if (copy_from_user(&gpu_fence, uarg, sizeof(struct kbase_fence_info))) {
		pr_err("%s copy_from_user fail\n", __func__);
		return -EFAULT;
	}

	ret = mali_kbase_report_fence_info(&gpu_fence);
	if (ret != 0) {
		pr_err("get_gpu_fence mali fail, ret=%d\n", ret);
		return -EFAULT;
	}

	if (copy_to_user(uarg, &gpu_fence, sizeof(struct kbase_fence_info))) {
		pr_err("%s copy_to_user fail\n", __func__);
		return -EFAULT;
	}

	return 0;
}

/** The number of counter blocks that always present in the gpu.
 * - Job Manager
 * - Tiler
 *  */
#define ALWAYS_PRESENT_NUM_OF_HWCBLK_PER_GPU 2
/* Get performance counter raw dump blocks
 * The blocks include Job manager,Tiler,L2,Shader core
 **/
static unsigned int mali_kbase_get_hwc_buffer_size(void)
{
	struct kbase_gpu_props *kprops = NULL;
	unsigned int num_l2;
	unsigned int num_cores;

	if (kbase_dev == NULL)
		return 0;

	kprops = &kbase_dev->gpu_props;
	/* Number of L2 slice blocks */
	num_l2 = kprops->props.l2_props.num_l2_slices;
	/* Number of shader core blocks. coremask without the leading zeros
	 * Even coremask is not successive, the memory should reserve for dump`
	 */
	num_cores = fls64(kprops->props.coherency_info.group[0].core_mask);

	return ALWAYS_PRESENT_NUM_OF_HWCBLK_PER_GPU + num_l2 + num_cores;
}

int perf_ctrl_get_gpu_buffer_size(void __user *uarg)
{
	unsigned int gpu_buffer_size;

	if (uarg == NULL)
		return -EINVAL;

	gpu_buffer_size = mali_kbase_get_hwc_buffer_size();
	if (copy_to_user(uarg, &gpu_buffer_size, sizeof(unsigned int))) {
		pr_err("%s: copy_to_user fail\n", __func__);
		return -EFAULT;
	}

	return 0;
}

#ifdef CONFIG_MALI_LAST_BUFFER
/*
 * This function get lb_enable flag from AI freq schedule service
 * When enter games, AI freq will set lb_enable=1 to enable LB
 * When exit games, AI freq will set lb_enable=0 to bypass LB
 */
int perf_ctrl_enable_gpu_lb(void __user *uarg)
{
	unsigned int enable = 0;

	if (uarg == NULL || kbase_dev == NULL)
		return -EINVAL;

	if (copy_from_user(&enable, uarg, sizeof(unsigned int))) {
		pr_err("[Mali gpu]%s: Get LB enable fail\n", __func__);
		return -EFAULT;
	}
	if (enable != 0 && enable != 1) {
		pr_err("[Mali gpu]%s: Invalid LB parameters\n", __func__);
		return -EINVAL;
	}
	kbase_dev->hisi_dev_data.game_scene = enable;
	if (kbase_dev->hisi_dev_data.lb_enable == enable)
		return 0;
	mali_kbase_enable_lb(kbase_dev, enable);

	return 0;
}
#endif
#endif

/*
 * This function enables hisilicon special feature.
 * Note that those features must enable every time when power on
 */
static void kbase_hisi_feature_enable(struct kbase_device *kbdev)
{
	/* config power key if mtcos enable or cs2 chip type */
	if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_CORE_READY_PERIOD)) {
		kbase_reg_write(kbdev, GPU_CONTROL_REG(PWR_KEY),
			KBASE_PWR_KEY_VALUE);
		kbase_reg_write(kbdev, GPU_CONTROL_REG(PWR_OVERRIDE1),
			KBASE_PWR_OVERRIDE_VALUE);
	}

#ifdef CONFIG_MALI_BORR
	if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_STRIPING_GRANUL_SETTING)) {
		u32 value = readl(kbdev->hisi_dev_data.pctrlreg + PERICTRL_19_OFFSET);
		value |= HASH_STRIPING_GRANULE;
		writel(value, kbdev->hisi_dev_data.pctrlreg + PERICTRL_19_OFFSET);
	}
#endif
}

/*
 * Enable hisilicon special feature.
 * Note that those features should init only once when system start-up
 */
static void kbase_hisi_feature_init(struct kbase_device *kbdev)
{
#if defined (CONFIG_MALI_TRYM) || defined (CONFIG_MALI_NATT)
	/* enable g3d memory auto hardware shutdown */
	if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_AUTO_MEM_SHUTDOWN)) {
		u32 value = readl(kbdev->hisi_dev_data.pctrlreg + PERICTRL_93_OFFSET);
		value |= G3D_MEM_AUTO_SD_ENABLE;
		writel(value, kbdev->hisi_dev_data.pctrlreg + PERICTRL_93_OFFSET);
	}
#endif
}

static inline void kbase_platform_on(struct kbase_device *kbdev)
{
	int refcount = atomic_read(&kbdev->hisi_dev_data.regulator_refcount);
	if (kbdev->hisi_dev_data.regulator && !refcount) {
		int loop = 0;
		const int max_loop = 3;
		const int delay_step = 5;
		while (loop < max_loop && unlikely(regulator_enable(kbdev->hisi_dev_data.regulator))) {
			dev_err(kbdev->dev, "Failed to enable regulator, retry...%d", ++loop);
			udelay(delay_step * loop);
		}
		BUG_ON(loop == max_loop);

		atomic_inc(&kbdev->hisi_dev_data.regulator_refcount);

		if (kbdev->hisi_dev_data.gpu_vid == 0) {
			kbdev->hisi_dev_data.gpu_vid = kbase_reg_read(kbdev,
				GPU_CONTROL_REG(GPU_ID));

			/* let's return the same hwver to software umd */
			if (kbdev->hisi_dev_data.gpu_vid == BORR_ES_ID)
				kbdev->hisi_dev_data.gpu_vid = BORR_CS_ID;

			if (unlikely(kbase_set_hi_features_mask(kbdev)))
				dev_err(kbdev->dev,
					"Failed to set hi features\n");
			/* init hisilicon special feature only once */
			kbase_hisi_feature_init(kbdev);
		}

		/* enable hisilicon special feature each time power on */
		kbase_hisi_feature_enable(kbdev);
#ifdef CONFIG_GPU_DPM_PCR
		dpm_pcr_enable(kbdev);
#endif
	} else {
		dev_err(kbdev->dev,"%s called zero, refcount:[%d]\n",__func__, refcount);
	}
}

static inline void kbase_platform_off(struct kbase_device *kbdev)
{
	int refcount = atomic_read(&kbdev->hisi_dev_data.regulator_refcount);

	if (kbdev->hisi_dev_data.regulator && refcount) {
		int loop = 0;
		const int max_loop = 3;
		const int delay_step = 5;
#ifdef CONFIG_GPU_DPM_PCR
		dpm_pcr_disable(kbdev);
#endif
		while (loop < max_loop && unlikely(regulator_disable(kbdev->hisi_dev_data.regulator))) {
			dev_err(kbdev->dev, "Failed to disable regulator, retry...%d", ++loop);
			udelay(delay_step * loop);
		}

		refcount = atomic_dec_return(&kbdev->hisi_dev_data.regulator_refcount);
		if (unlikely(refcount != 0))
			dev_err(kbdev->dev,
				"%s called not match, refcount:[%d]\n",
				__func__, refcount);
	}
}

#ifdef CONFIG_GPU_VIRTUAL_DEVFREQ
#ifdef CONFIG_MALI_BORR
#define CORE_MASK_ALL	0x77777777ULL
#define VFREQ_OPP_COUNT_MAX	6
#define CORESTACK_COUNT_MAX	6
#define CORESTACK_MASK_UNIT	0x1111ULL
#define CORESTACK_CORE_NUMS	4
#define CORESTACK_CORE_BITS	4
#define CORESTACK_BASE_NUMS	2
#endif

struct hisi_virtual_devfreq {
	u64 desired_core_mask;
	u64 current_core_mask;
	struct kbase_devfreq_opp vfreq_table[VFREQ_OPP_COUNT_MAX];
	int num_virtual_opps;
	unsigned long devfreq_min;
	struct work_struct set_coremask_work;
	bool virtual_devfreq_mode;
	bool virtual_devfreq_enable;
	bool virtual_devfreq_support;
	struct mutex lock;
};

static struct hisi_virtual_devfreq hisi_virtual_devfreq_data;

int gpu_virtual_devfreq_get_freq(unsigned long *freq)
{
	unsigned int i;
	struct hisi_virtual_devfreq *priv_data = &hisi_virtual_devfreq_data;
	u64 core_mask;
	unsigned long flags;

	if (freq == NULL)
		return -1;
	if (!priv_data->virtual_devfreq_support ||
		!priv_data->virtual_devfreq_enable)
		return 0;
	if (kbase_dev == NULL) {
		pr_err("[Mali]kbase_dev is null,seems platform uninitialized");
		return -1;
	}

	spin_lock_irqsave(&kbase_dev->hwaccess_lock, flags);
	core_mask = kbase_pm_ca_get_core_mask(kbase_dev);
	spin_unlock_irqrestore(&kbase_dev->hwaccess_lock, flags);

	mutex_lock(&priv_data->lock);
	for (i = 0; i < priv_data->num_virtual_opps; i++) {
		if (priv_data->vfreq_table[i].core_mask ==
			core_mask) {
			*freq = priv_data->vfreq_table[i].opp_freq;
			break;
		}
	}
	mutex_unlock(&priv_data->lock);

	return 0;
}

int gpu_virtual_devfreq_get_devfreq_min(unsigned long *devfreq_min)
{
	struct hisi_virtual_devfreq *priv_data = &hisi_virtual_devfreq_data;

	if (devfreq_min == NULL)
		return -1;
	if (!priv_data->virtual_devfreq_support ||
		!priv_data->virtual_devfreq_enable)
		return 0;

	mutex_lock(&priv_data->lock);
	*devfreq_min = priv_data->devfreq_min;
	mutex_unlock(&priv_data->lock);

	return 0;
}

int gpu_virtual_devfreq_set_core_mask(unsigned long virtual_freq,
	bool virtual_freq_flag)
{
	unsigned int i;
	struct hisi_virtual_devfreq *priv_data = &hisi_virtual_devfreq_data;
	u64 shader_present;

	if (!priv_data->virtual_devfreq_support ||
		!priv_data->virtual_devfreq_enable)
		return 0;

	if (kbase_dev == NULL) {
		pr_err("[Mali]kbase_dev is null,seems platform uninitialized");
		return 0;
	}
	shader_present = kbase_dev->gpu_props.props.raw_props.shader_present;

	mutex_lock(&priv_data->lock);
	if (virtual_freq_flag == true) {
		for (i = 0; i < priv_data->num_virtual_opps; i++) {
			if (priv_data->vfreq_table[i].opp_freq >=
				virtual_freq) {
				priv_data->desired_core_mask =
					priv_data->vfreq_table[i].core_mask;
				break;
			}
		}
		if (i == priv_data->num_virtual_opps)
			priv_data->desired_core_mask = shader_present;
	} else {
		priv_data->desired_core_mask = shader_present;
	}
	mutex_unlock(&priv_data->lock);

	return 0;
}

int gpu_virtual_devfreq_set_mode(bool flag)
{
	struct hisi_virtual_devfreq *priv_data = &hisi_virtual_devfreq_data;
	unsigned long flags;
	u64 shader_present;

	if (!priv_data->virtual_devfreq_support ||
		!priv_data->virtual_devfreq_enable)
		return 0;
	if (kbase_dev == NULL) {
		pr_err("[Mali]kbase_dev is null,seems platform uninitialized");
		return 0;
	}
	shader_present = kbase_dev->gpu_props.props.raw_props.shader_present;

	mutex_lock(&priv_data->lock);
	if (priv_data->virtual_devfreq_mode != flag) {
		priv_data->virtual_devfreq_mode = flag;
		spin_lock_irqsave(&kbase_dev->hwaccess_lock, flags);
		priv_data->current_core_mask =
			kbase_pm_ca_get_core_mask(kbase_dev);
		spin_unlock_irqrestore(&kbase_dev->hwaccess_lock, flags);
		priv_data->desired_core_mask = shader_present;
		queue_work(system_freezable_power_efficient_wq,
			&priv_data->set_coremask_work);
	}
	mutex_unlock(&priv_data->lock);

	return 0;
}

int gpu_virtual_devfreq_get_enable(bool *flag)
{
	struct hisi_virtual_devfreq *priv_data = &hisi_virtual_devfreq_data;

	if (flag == NULL)
		return -1;
	if (kbase_dev == NULL) {
		pr_err("[Mali]kbase_dev is null,seems platform uninitialized");
		return 0;
	}
	if (kbase_dev->hisi_dev_data.gpu_chip_type != 2) {
		pr_err("[Mali] Don't support virtual devfreq");
		return 0;
	}

	mutex_lock(&priv_data->lock);
	*flag = priv_data->virtual_devfreq_support &&
		priv_data->virtual_devfreq_enable;
	mutex_unlock(&priv_data->lock);

	return 0;
}

int gpu_virtual_devfreq_set_enable(bool flag)
{
	struct hisi_virtual_devfreq *priv_data = &hisi_virtual_devfreq_data;
	u64 shader_present;
	unsigned long flags;

	if (kbase_dev == NULL) {
		pr_err("[Mali]kbase_dev is null,seems platform uninitialized");
		return 0;
	}
	shader_present = kbase_dev->gpu_props.props.raw_props.shader_present;
	if (kbase_dev->hisi_dev_data.gpu_chip_type != 2) {
		pr_err("[Mali] Don't support virtual devfreq");
		return 0;
	}

	mutex_lock(&priv_data->lock);
	if (priv_data->virtual_devfreq_support &&
		priv_data->virtual_devfreq_enable != flag) {
		priv_data->virtual_devfreq_enable = flag;
		spin_lock_irqsave(&kbase_dev->hwaccess_lock, flags);
		priv_data->current_core_mask =
			kbase_pm_ca_get_core_mask(kbase_dev);
		spin_unlock_irqrestore(&kbase_dev->hwaccess_lock, flags);
		priv_data->desired_core_mask = shader_present;
		queue_work(system_freezable_power_efficient_wq,
			&priv_data->set_coremask_work);
	}
	mutex_unlock(&priv_data->lock);

	return 0;
}
#endif

#ifdef CONFIG_GPU_THROTTLE_DEVFREQ
void gpu_devfreq_set_gpu_throttle(unsigned long thro_low_freq,
	unsigned long thro_high_freq, unsigned long *freq)
{
	unsigned long flags;
	int thro_hint;
	int thro_enable;

	if (kbase_dev == NULL || freq  == NULL) {
		pr_err("[Mali]kbase_dev is null or freq is null");
		return;
	}

	thro_enable = atomic_read(&kbase_dev->hisi_dev_data.thro_enable);
	if (thro_enable == 0)
		goto update_thro_hint;

	spin_lock_irqsave(&kbase_dev->pm.backend.metrics.lock, flags);
	thro_hint = kbase_dev->pm.backend.metrics.thro_hint;
	if (thro_hint == KBASE_JS_ATOM_SCHED_THRO_COUNT)
		thro_hint = kbase_dev->pm.backend.metrics.thro_hint_prev;
	else
		kbase_dev->pm.backend.metrics.thro_hint_prev = thro_hint;
	spin_unlock_irqrestore(&kbase_dev->pm.backend.metrics.lock, flags);

	if (thro_hint != KBASE_JS_ATOM_SCHED_THRO_DEFAULT) {
		if (thro_hint == KBASE_JS_ATOM_SCHED_THRO_LOW &&
			thro_low_freq != 0)
			*freq = thro_low_freq;
		if (thro_hint == KBASE_JS_ATOM_SCHED_THRO_HIGH &&
			thro_high_freq != 0)
			*freq = thro_high_freq;
	}

update_thro_hint:
	spin_lock_irqsave(&kbase_dev->pm.backend.metrics.lock, flags);
	kbase_dev->pm.backend.metrics.thro_hint =
		KBASE_JS_ATOM_SCHED_THRO_COUNT;
	spin_unlock_irqrestore(&kbase_dev->pm.backend.metrics.lock, flags);
}
#endif

#ifdef CONFIG_DEVFREQ_THERMAL
void mali_kbase_devfreq_detect_bound_worker(struct work_struct *work)
{
	int err = 0;
	struct kbase_hisi_device_data *hisi_data = NULL;
	struct kbase_device *kbdev = NULL;
	bool bound_event = false;
	struct thermal_cooling_device *cdev = NULL;

	KBASE_DEBUG_ASSERT(work != NULL);
	hisi_data = container_of(work,
		struct kbase_hisi_device_data, bound_detect_work);
	KBASE_DEBUG_ASSERT(hisi_data != NULL);
	kbdev = container_of(hisi_data,
		struct kbase_device, hisi_dev_data);
	KBASE_DEBUG_ASSERT(kbdev != NULL);

#ifdef CONFIG_GPU_IPA_THERMAL
	cdev = ithermal_get_gpu_cdev();
	if (cdev == NULL)
		return;
#endif
#ifdef CONFIG_MALI_MIDGARD_DVFS
	bound_event = kbase_ipa_dynamic_bound_detect(
		kbdev->hisi_dev_data.ipa_ctx, &err,
		kbdev->hisi_dev_data.bound_detect_freq,
		kbdev->hisi_dev_data.bound_detect_btime,
#ifdef CONFIG_GPU_IPA_THERMAL
		cdev->ipa_enabled);
#else
		false);
#endif
#endif
#ifdef CONFIG_GPU_IPA_THERMAL
	cdev->ipa_enabled = false;

	cdev->bound_event = bound_event;
#endif
}

static void mali_kbase_devfreq_detect_bound(struct kbase_device *kbdev,
	unsigned long cur_freq, unsigned long btime)
{
	kbdev->hisi_dev_data.bound_detect_freq = cur_freq;
	kbdev->hisi_dev_data.bound_detect_btime = btime;
	/*
	 * Use freezable workqueue so that the work can freeze
	 * when device is going to suspend
	 */
	queue_work(system_freezable_power_efficient_wq,
		&kbdev->hisi_dev_data.bound_detect_work);
}
#endif

#ifdef CONFIG_REPORT_VSYNC
/* DSS hisifb_vsync_register have refence on this function, add for compile */
void mali_kbase_pm_report_vsync(int buffer_updated)
{
	unsigned long flags;

	if (kbase_dev) {
		spin_lock_irqsave(&kbase_dev->pm.backend.metrics.lock, flags);
		kbase_dev->pm.backend.metrics.vsync_hit = buffer_updated;
		spin_unlock_irqrestore(&kbase_dev->pm.backend.metrics.lock,
			flags);
	}
}
EXPORT_SYMBOL(mali_kbase_pm_report_vsync);
#endif

#ifdef CONFIG_MALI_MIDGARD_DVFS
int kbase_platform_dvfs_event(struct kbase_device *kbdev, u32 utilisation,
	u32 util_gl_share, u32 util_cl_share[2])
{
	return 1;
}

int kbase_platform_dvfs_enable(struct kbase_device *kbdev, bool enable,
	int freq)
{
	unsigned long flags;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	if (enable != kbdev->pm.backend.metrics.timer_active) {
		if (enable) {
			spin_lock_irqsave(&kbdev->pm.backend.metrics.lock,
				flags);
			kbdev->pm.backend.metrics.timer_active = MALI_TRUE;
			spin_unlock_irqrestore(&kbdev->pm.backend.metrics.lock,
				flags);
			hrtimer_start(&kbdev->pm.backend.metrics.timer,
				HR_TIMER_DELAY_MSEC(kbdev->pm.dvfs_period),
				HRTIMER_MODE_REL);
		} else {
			spin_lock_irqsave(&kbdev->pm.backend.metrics.lock,
				flags);
			kbdev->pm.backend.metrics.timer_active = MALI_FALSE;
			spin_unlock_irqrestore(&kbdev->pm.backend.metrics.lock,
				flags);
			hrtimer_cancel(&kbdev->pm.backend.metrics.timer);
		}
	}

	return 1;
}
#endif

/* Init HISI platform related register map */
static int kbase_hisi_register_map(struct kbase_device *kbdev)
{
	int err = 0;

	kbdev->hisi_dev_data.crgreg = ioremap(SYS_REG_PERICRG_BASE_ADDR,
		SYS_REG_REMAP_SIZE);
	if (!kbdev->hisi_dev_data.crgreg) {
		dev_err(kbdev->dev, "Can't remap sys crg register window on platform\n");
		err = -EINVAL;
		goto out_crg_ioremap;
	}

	kbdev->hisi_dev_data.pmctrlreg = ioremap(SYS_REG_PMCTRL_BASE_ADDR,
			SYS_REG_REMAP_SIZE);
	if (!kbdev->hisi_dev_data.pmctrlreg) {
		dev_err(kbdev->dev, "Can't remap sys pmctrl register window on platform\n");
		err = -EINVAL;
		goto out_pmctrl_ioremap;
	}
#ifdef CONFIG_MALI_NORR_PHX
	if (kbdev->hisi_dev_data.gpu_chip_type == 2)
		kbdev->hisi_dev_data.pctrlreg = ioremap(SYS_REG_PERICTRL_BASE_ADDR_CS2,
			SYS_REG_REMAP_SIZE);
	else
#endif
		kbdev->hisi_dev_data.pctrlreg = ioremap(SYS_REG_PERICTRL_BASE_ADDR,
			SYS_REG_REMAP_SIZE);

	if (!kbdev->hisi_dev_data.pctrlreg) {
		dev_err(kbdev->dev, "Can't remap sys pctrl register window on platform\n");
		err = -EINVAL;
		goto out_pctrl_ioremap;
	}

#ifdef CONFIG_MALI_BORR
	kbdev->hisi_dev_data.sctrlreg = ioremap(SYS_REG_SYSCTRL_BASE_ADDR,
			SYS_REG_REMAP_SIZE);
	if (!kbdev->hisi_dev_data.sctrlreg) {
		dev_err(kbdev->dev, "Can't remap sctrl register window on platform \n");
		err = -EINVAL;
		goto out_sctrl_ioremap;
	}
#endif /*CONFIG_MALI_BORR*/

	return err;

#ifdef CONFIG_MALI_BORR
out_sctrl_ioremap:
	iounmap(kbdev->hisi_dev_data.pctrlreg);
#endif /*CONFIG_MALI_BORR*/

out_pctrl_ioremap:
	iounmap(kbdev->hisi_dev_data.pmctrlreg);

out_pmctrl_ioremap:
	iounmap(kbdev->hisi_dev_data.crgreg);

out_crg_ioremap:
	return err;
}

static void kbase_hisi_register_unmap(struct kbase_device *kbdev)
{
	if (kbdev->hisi_dev_data.crgreg != NULL)
		iounmap(kbdev->hisi_dev_data.crgreg);

	if (kbdev->hisi_dev_data.pmctrlreg != NULL)
		iounmap(kbdev->hisi_dev_data.pmctrlreg);

	if (kbdev->hisi_dev_data.pctrlreg != NULL)
		iounmap(kbdev->hisi_dev_data.pctrlreg);

#ifdef CONFIG_MALI_BORR
	if (kbdev->hisi_dev_data.sctrlreg != NULL)
		iounmap(kbdev->hisi_dev_data.sctrlreg);
#endif /*CONFIG_MALI_BORR*/
}

static int kbase_hisi_verify_fpga_exist(struct kbase_device *kbdev)
{
	int ret = 0;
#ifdef CONFIG_MALI_BORR
	bool logic_ver = false;
	unsigned int sctrl_value = 0;

	sctrl_value = readl(kbdev->hisi_dev_data.sctrlreg + SYSCTRL_SOC_ID0_OFFSET);
	dev_info(kbdev->dev, "mali gpu sctrl_value = 0x%x\n", sctrl_value);

	/* platform is borr, if logic is B010, do not load GPU kernel */
	logic_ver = (sctrl_value == MASK_BALTIMORE_FPGA_B010);
	if (logic_ver) {
		dev_err(kbdev->dev, "logic is B010, do not load GPU kernel\n");
		return -ENODEV;
	}
	/* platform is borr, if logic is B02x, do load GPU kernel */
	logic_ver = (sctrl_value == MASK_BALTIMORE_FPGA_B020) ||
		(sctrl_value == MASK_BALTIMORE_FPGA_B021);
	if (logic_ver) {
		dev_err(kbdev->dev, "logic is B020 or B021, load GPU kernel\n");
		return ret;
	}
#endif /* CONFIG_MALI_BORR */

	if (kbdev->hisi_dev_data.gpu_fpga_exist) {
		unsigned int pctrl_value;
		pctrl_value = readl(kbdev->hisi_dev_data.pctrlreg +
			PERI_STAT_FPGA_GPU_EXIST) & PERI_STAT_FPGA_GPU_EXIST_MASK;
		if (pctrl_value == 0) {
			dev_err(kbdev->dev, "No FPGA FOR GPU\n");
			return -ENODEV;
		}
	}
	return ret;
}

#ifdef CONFIG_GPU_SHADER_PRESENT_CFG
static int kbase_check_gpu_status(struct kbase_device * const kbdev)
{
	struct device_node *np = NULL;
	int ret;
	int gpu_status = 1;
#ifdef CONFIG_OF
	/* read outstanding value from dts*/
	np = of_find_compatible_node(NULL, NULL, "arm,mali-midgard");
	if (np) {
		ret = of_property_read_u32(np, "gpu-status", &gpu_status);
		if (ret) {
			dev_warn(kbdev->dev, "Failted to get gpu status settings\n");
			return 0;
		}

		if (!gpu_status) {
			dev_warn(kbdev->dev, "gpu status is disable\n");
			return -ENODEV;
		}
	} else {
		dev_err(kbdev->dev, "not find device node arm,mali-midgard!\n");
	}
#endif
	return 0;
}
#endif

static int kbase_hisi_init_device_tree(struct kbase_device * const kbdev)
{
	struct device_node *np = NULL;
	int ret;
	enum SOC_SPEC_TYPE soc_type;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	/* init soc special flag */
	kbdev->hisi_dev_data.gpu_fpga_exist = 0;
	kbdev->hisi_dev_data.gpu_chip_type = 0;
	kbdev->hisi_dev_data.lb_enabled = 0;
	kbdev->hisi_dev_data.smart_cache_capacity = 0;

#ifdef CONFIG_GPU_SHADER_PRESENT_CFG
	kbdev->hisi_dev_data.shader_present_lo_cfg = 0;
#endif

#ifdef CONFIG_OF
	/* read outstanding value from dts*/
	np = of_find_compatible_node(NULL, NULL, "arm,mali-midgard");
	if (np) {
		ret = of_property_read_u32(np, "fpga-gpu-exist",
			&kbdev->hisi_dev_data.gpu_fpga_exist);
		if (ret)
			dev_warn(kbdev->dev,
			"No fpga gpu exist setting, assume not exist\n");

		ret = of_property_read_u32(np, "last-buffer",
			&kbdev->hisi_dev_data.lb_enabled);
		if (ret)
			dev_warn(kbdev->dev,
			"Failed to get last buffer setting.\n");

#ifdef CONFIG_MALI_LAST_BUFFER
		if (kbdev->hisi_dev_data.lb_enabled == 1) {
			kbdev->hisi_dev_data.lb_enabled = is_lb_available();
		}
#endif

		ret = of_property_read_u32(np, "sys-cache-capacity",
			&kbdev->hisi_dev_data.smart_cache_capacity);
		if(ret)
			dev_warn(kbdev->dev,
			"Failed to get capacity of sys cache.\n");

		ret = of_property_read_u32(np, "gpu-chip-type",
			&kbdev->hisi_dev_data.gpu_chip_type);
		if (ret)
			dev_warn(kbdev->dev,
			"No gpu chip type setting, default means cs\n");

#ifdef CONFIG_GPU_SHADER_PRESENT_CFG
		soc_type = get_current_chip_type(kbdev);
		if (soc_type == PC_CHIP || soc_type == PC_NORMAL_CHIP) {
			ret = get_gpu_efuse_cfg(kbdev, &kbdev->hisi_dev_data.shader_present_lo_cfg);
			if (ret) {
				dev_err(kbdev->dev, "Failed to get shader present configuration by efuse!\n");
				return -ENOENT;
			}
		} else {
			ret = of_property_read_u32(np, "shader-present-lo-cfg",
				&kbdev->hisi_dev_data.shader_present_lo_cfg);
			if (ret)
				dev_warn(kbdev->dev, "Failed to get shader present configuration by dts!\n");
		}
#endif

	} else {
		dev_err(kbdev->dev,
			"not find device node arm,mali-midgard!\n");
	}
#endif

	return 0;
}

#ifdef CONFIG_MALI_LAST_BUFFER
static void kbase_hisi_init_lb(struct kbase_device *kbdev)
{
	hisi_lb_callbacks *lb_cbs = kbase_hisi_get_lb_cbs(kbdev);

	if (lb_cbs == NULL || lb_cbs->lb_init(kbdev))
		dev_err(kbdev->dev, "Init last buffer failed.\n");
}

static void kbase_hisi_term_lb(struct kbase_device *kbdev)
{
	hisi_lb_callbacks *lb_cbs = kbase_hisi_get_lb_cbs(kbdev);

	if (lb_cbs == NULL || lb_cbs->lb_term(kbdev))
		dev_err(kbdev->dev, "Term last buffer failed.\n");
}
#endif

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
#ifdef CONFIG_GPU_SHADER_PRESENT_CFG
	err = kbase_check_gpu_status(kbdev);
	if (err != 0) {
		dev_err(kbdev->dev, "No gpu hardware implementation\n");
		return -ENODEV;
	}
#endif
	// init gpu_outstanding, fpga_exist, chip_type and others from device tree
	err = kbase_hisi_init_device_tree(kbdev);
	if (err != 0) {
		dev_err(kbdev->dev, "Init special devce tree setting failed\n");
		return err;
	}

	// register map for hisilicon, especially for chip_type related register
	err = kbase_hisi_register_map(kbdev);
	if (err != 0) {
		dev_err(kbdev->dev, "Can't remap hisi register\n");
		return err;
	}

	// verify if FPGA, need use info of related register
	err = kbase_hisi_verify_fpga_exist(kbdev);
	if (err != 0) {
		dev_err(kbdev->dev, "No gpu hardware implementation on fpga\n");
		return -ENODEV;
	}

#ifdef CONFIG_HUAWEI_DSM
	kbdev->hisi_dev_data.gpu_dsm_client = dsm_register_client(&dsm_gpu);
#ifdef CONFIG_CMDLINE_PARSE
	kbdev->hisi_dev_data.factory_mode = runmode_is_factory();
#endif
#endif

#ifdef CONFIG_GPU_GMC_GENERIC
	/* Register GMC device */
	kbase_gmc_device_init(kbdev);
#endif

	/* init kbdev->hisi_dev_data.callbacks */
	kbdev->hisi_dev_data.callbacks = (struct kbase_hisi_callbacks *)gpu_get_callbacks();

#ifdef CONFIG_MALI_LAST_BUFFER
	kbase_hisi_init_lb(kbdev);
#endif

#if defined(CONFIG_MALI_MIDGARD_DVFS) && defined(CONFIG_DEVFREQ_THERMAL)
	/* Add ipa GPU HW bound detection */
	kbdev->hisi_dev_data.ipa_ctx = kbase_dynipa_init(kbdev);
	if (!kbdev->hisi_dev_data.ipa_ctx) {
		dev_err(kbdev->dev,
			"GPU HW bound detection sub sys initialization failed\n");
	} else {
		INIT_WORK(&kbdev->hisi_dev_data.bound_detect_work,
			mali_kbase_devfreq_detect_bound_worker);
	}
#endif

#ifdef CONFIG_GPU_DPM_PCR
	dpm_pcr_resource_init(kbdev);
#endif
	return err;
}

static void kbase_platform_backend_term(struct kbase_device *kbdev)
{
#ifdef CONFIG_GPU_DPM_PCR
	dpm_pcr_resource_exit(kbdev);
#endif

	kbase_hisi_register_unmap(kbdev);

#ifdef CONFIG_MALI_LAST_BUFFER
	kbase_hisi_term_lb(kbdev);
#endif

#if defined(CONFIG_MALI_MIDGARD_DVFS) && defined(CONFIG_DEVFREQ_THERMAL)
	if (kbdev->hisi_dev_data.ipa_ctx)
		kbase_dynipa_term(kbdev->hisi_dev_data.ipa_ctx);
#endif
#ifdef CONFIG_GPU_GMC_GENERIC
	/* GMC device term */
	kbase_gmc_device_term(kbdev);
#endif
}

#ifdef CONFIG_GPU_VIRTUAL_DEVFREQ

static bool check_coremask_validity(u64 core_mask)
{
	unsigned int i;
	bool valid = false;
	struct hisi_virtual_devfreq *priv_data = &hisi_virtual_devfreq_data;
	u64 shader_present;

	if (kbase_dev == NULL) {
		pr_err("[Mali]kbase_dev is null,seems platform uninitialized");
		return 0;
	}
	shader_present = kbase_dev->gpu_props.props.raw_props.shader_present;

	mutex_lock(&priv_data->lock);
	for (i = 0; i < priv_data->num_virtual_opps; i++) {
		if (priv_data->vfreq_table[i].core_mask ==
			core_mask) {
			valid = true;
			break;
		}
	}
	if (i == priv_data->num_virtual_opps && core_mask == shader_present)
		valid = true;
	mutex_unlock(&priv_data->lock);

	return valid;
}

static void mali_kbase_devfreq_set_coremask_worker(struct work_struct *work)
{
	u64 core_mask;
	bool valid = false;
	struct hisi_virtual_devfreq *priv_data = &hisi_virtual_devfreq_data;
	unsigned long flags;

	if (kbase_dev == NULL) {
		pr_err("[Mali]kbase_dev is null,seems platform uninitialized");
		return;
	}

#ifdef CONFIG_GPU_CORE_HOTPLUG
	/* don't debug core_mask if the thermal is controlling */
	if (kbase_dev->pm.thermal_controlling)
		return;
#endif

	core_mask = priv_data->desired_core_mask;
	valid = check_coremask_validity(core_mask);
	if (valid != true)
		return;

	mutex_lock(&kbase_dev->pm.lock);
	spin_lock_irqsave(&kbase_dev->hwaccess_lock, flags);
	if (priv_data->current_core_mask != core_mask)
		kbase_pm_set_debug_core_mask(kbase_dev, core_mask,
			core_mask, core_mask);
	spin_unlock_irqrestore(&kbase_dev->hwaccess_lock, flags);
	mutex_unlock(&kbase_dev->pm.lock);

	mutex_lock(&priv_data->lock);
	priv_data->current_core_mask = core_mask;
	mutex_unlock(&priv_data->lock);
}

static int kbase_virtual_devfreq_init_core_mask_table(
	struct kbase_device *kbdev)
{
	struct device_node *opp_node = of_parse_phandle(kbdev->dev->of_node,
			"virtual-operating-points-v2", 0);
	struct device_node *node = NULL;
	int i = 0;
	u64 shader_present = kbdev->gpu_props.props.raw_props.shader_present;
	struct hisi_virtual_devfreq *priv_data = &hisi_virtual_devfreq_data;
	int j = 0;

	if (!opp_node)
		return 0;
	if (!of_device_is_compatible(opp_node, "virtual-operating-points-v2"))
		return 0;

	for_each_available_child_of_node(opp_node, node) {
		u64 core_mask;
		u64 opp_freq;
		u64 real_freqs[BASE_MAX_NR_CLOCKS_REGULATORS];
		int err;

		err = of_property_read_u64(node, "opp-hz", &opp_freq);
		if (err) {
			dev_warn(kbdev->dev, "Failed to read opp-hz property with error %d\n",
					err);
			continue;
		}
#if BASE_MAX_NR_CLOCKS_REGULATORS > 1
		err = of_property_read_u64_array(node, "opp-hz-real",
				real_freqs, kbdev->nr_clocks);
#else
		err = of_property_read_u64_array(node, "opp-hz-real", real_freqs, 1);
#endif
		if (err < 0) {
			dev_warn(kbdev->dev, "Failed to read opp-hz-real property with error %d\n",
					err);
			continue;
		}

		if (of_property_read_u64(node, "opp-core-mask", &core_mask))
			core_mask = shader_present;
		if (core_mask != shader_present && corestack_driver_control) {
			dev_warn(kbdev->dev, "Ignoring OPP %llu - Dynamic Core Scaling not supported on this GPU\n",
					opp_freq);
			continue;
		}

		if (!core_mask) {
			dev_err(kbdev->dev, "OPP has invalid core mask of 0\n");
			return -ENODEV;
		}

		if (i >= VFREQ_OPP_COUNT_MAX) {
			priv_data->num_virtual_opps = i;
			dev_err(kbdev->dev, "OPP reaches maximum limit\n");
			return -ENODEV;
		}

		priv_data->vfreq_table[i].opp_freq = opp_freq;
		priv_data->vfreq_table[i].core_mask = core_mask;

		for (j = 0; j < BASE_MAX_NR_CLOCKS_REGULATORS; j++) {
			priv_data->vfreq_table[i].real_freqs[j] =
				real_freqs[j];
			priv_data->devfreq_min = real_freqs[j];
		}

		dev_info(kbdev->dev, "OPP %d : opp_freq=%llu core_mask=%llx\n",
				i, opp_freq, core_mask);
		i++;
	}

	priv_data->num_virtual_opps = i;

	return 0;
}

int hisi_gpu_get_vfreq_num(void)
{
	return hisi_virtual_devfreq_data.num_virtual_opps;
}

void hisi_gpu_get_vfreq_val(int num, u64 *opp_freq, u64 *core_mask)
{
	if (num < 0 || num >= VFREQ_OPP_COUNT_MAX)
		return;

	*opp_freq = hisi_virtual_devfreq_data.vfreq_table[num].opp_freq;
	*core_mask = hisi_virtual_devfreq_data.vfreq_table[num].core_mask;
}

#ifdef CONFIG_MALI_BORR
static inline int hisi_get_corestack_shift_bits(int corestack)
{
	if (corestack < 3)
		return corestack;
	else
		return corestack - 3 + 16;
}
#endif

static void hisi_gpu_lite_get_corestack_status(u64 shader_present,
	int stack_stat[], int stack_cnt_max)
{
	u64 core_mask = 0;
	u32 shift_bits = 0;
	int sum_bits = 0;
	int i = 0;
	int j = 0;

	for (i = 0; i < stack_cnt_max; i++) {
		shift_bits = hisi_get_corestack_shift_bits(i);
		core_mask = ((CORESTACK_MASK_UNIT << shift_bits) &
			shader_present) >> shift_bits;
		sum_bits = 0;
		for (j = 0; j < CORESTACK_CORE_NUMS; j++)
			sum_bits += (core_mask >>
			(u64)(j * CORESTACK_CORE_BITS)) & 0x1;
		if (sum_bits >= 0 && sum_bits <= CORESTACK_CORE_NUMS)
			stack_stat[i] = CORESTACK_CORE_NUMS - sum_bits;
		else
			stack_stat[i] = CORESTACK_CORE_NUMS;
	}
}

static void hisi_gpu_lite_update_virtual_devfreq(const int stack_stat[],
	int stack_cnt_max, struct hisi_virtual_devfreq *virtual_devfreq)
{
	int val = 0;
	int i = 0;

	for (i = 0; i < stack_cnt_max; i++)
		val += stack_stat[i];
	val /= CORESTACK_CORE_NUMS;
	if (val < 0 || val >= virtual_devfreq->num_virtual_opps) {
		virtual_devfreq->num_virtual_opps = 0;
		virtual_devfreq->virtual_devfreq_support = false;
		virtual_devfreq->virtual_devfreq_enable = false;
	} else {
		virtual_devfreq->num_virtual_opps -= val;
	}
}

static u64 hisi_gpu_lite_get_coremask(u64 shader_present, const int stack_stat[],
	int stack_cnt_max, int stack_num)
{
	u64 core_mask = 0;
	int shift_bits = 0;
	int i = 0;
	int j = 0;
	int k = 0;

	if (stack_num > stack_cnt_max)
		return 0;
	for (i = 0; i <= CORESTACK_CORE_NUMS; i++) {
		for (j = 0; j < stack_cnt_max; j++) {
			if (k  >=  stack_num)
				return core_mask;
			if (stack_stat[j] == i) {
				shift_bits = hisi_get_corestack_shift_bits(j);
				core_mask |= shader_present &
					(CORESTACK_MASK_UNIT << (unsigned int)shift_bits);
				k++;
			}
		}
	}
	if (k < stack_num)
		return 0;

	return core_mask;
}

static int hisi_gpu_lite_init_coremask(struct kbase_device *kbdev)
{
	struct hisi_virtual_devfreq *priv_data = &hisi_virtual_devfreq_data;
	u64 shader_present = kbdev->gpu_props.props.raw_props.shader_present;
	int stack_stat[CORESTACK_COUNT_MAX] = {0};
	u64 core_mask = 0;
	int i = 0;

	hisi_gpu_lite_get_corestack_status(shader_present,
		stack_stat, CORESTACK_COUNT_MAX);

	hisi_gpu_lite_update_virtual_devfreq(stack_stat,
		CORESTACK_COUNT_MAX, priv_data);

	for (i = 0; i < priv_data->num_virtual_opps; i++) {
		core_mask = hisi_gpu_lite_get_coremask(shader_present,
			stack_stat, CORESTACK_COUNT_MAX,
			i + CORESTACK_BASE_NUMS);
		if (core_mask != 0) {
			priv_data->vfreq_table[i].core_mask = core_mask;
		} else {
			priv_data->virtual_devfreq_support = false;
			priv_data->virtual_devfreq_enable = false;
			priv_data->num_virtual_opps = 0;
			return 0;
		}
	}

	return 0;
}

int kbase_virtual_devfreq_init(struct kbase_device *kbdev)
{
	struct hisi_virtual_devfreq *priv_data = &hisi_virtual_devfreq_data;
	u64 shader_present = kbdev->gpu_props.props.raw_props.shader_present;

	mutex_init(&priv_data->lock);
	if (kbdev->hisi_dev_data.gpu_chip_type == 2) {
		priv_data->virtual_devfreq_support = true;
		priv_data->virtual_devfreq_enable = true;
	} else {
		priv_data->virtual_devfreq_support = false;
		priv_data->virtual_devfreq_enable = false;
		return 0;
	}

	INIT_WORK(&priv_data->set_coremask_work,
			mali_kbase_devfreq_set_coremask_worker);

	priv_data->devfreq_min = 0;

	kbase_virtual_devfreq_init_core_mask_table(kbdev);

	if (shader_present != CORE_MASK_ALL)
		hisi_gpu_lite_init_coremask(kbdev);

	priv_data->current_core_mask = shader_present;
	priv_data->desired_core_mask = shader_present;
	priv_data->virtual_devfreq_mode = true;

	return 0;
}
#endif

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

	(void)kbase_pm_get_dvfs_action(kbdev);

	busy_time = kbdev->pm.backend.metrics.utilisation;
	cur_freq = gpufreq_get_cur_freq();
#ifdef CONFIG_DEVFREQ_THERMAL
	/* Avoid sending HWC dump cmd to GPU when GPU is power-off */
	if (kbdev->pm.backend.gpu_powered)
		(void)mali_kbase_devfreq_detect_bound(kbdev, cur_freq, (unsigned long)busy_time);
#endif
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

	(void)kbase_pm_get_dvfs_action(kbdev);

	return kbdev->pm.backend.metrics.vsync_hit;
}

static int kbase_get_cl_boost(void)
{
	struct kbase_device *kbdev = kbase_dev;

	KBASE_DEBUG_ASSERT(kbdev != NULL);
	if (kbdev->pm.backend.metrics.kbdev != kbdev) {
		pr_err("%s pm backend metrics not initialized\n", __func__);
		return -EINVAL;
	}

	(void)kbase_pm_get_dvfs_action(kbdev);

	return kbdev->pm.backend.metrics.cl_boost;
}

static int kbase_queue_coremask_work(void)
{
#ifdef CONFIG_GPU_VIRTUAL_DEVFREQ
	struct hisi_virtual_devfreq *priv_data = &hisi_virtual_devfreq_data;

	mutex_lock(&priv_data->lock);
	if (priv_data->virtual_devfreq_support &&
		priv_data->virtual_devfreq_enable &&
		priv_data->virtual_devfreq_mode &&
		priv_data->current_core_mask != priv_data->desired_core_mask)
		queue_work(system_freezable_power_efficient_wq,
			&priv_data->set_coremask_work);
	mutex_unlock(&priv_data->lock);

#endif
	return 0;
}


static int gpufreq_init(void)
{
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

	ret = gpufreq_register_callback(GF_CB_CL_BOOST, kbase_get_cl_boost);
	if (ret != 0) {
		pr_err("%s get_cl_boost callback register failed \n", __func__);
		return ret;
	}

	ret = gpufreq_register_callback(GF_CB_CORE_MASK, kbase_queue_coremask_work);
	if (ret != 0) {
		pr_err("%s kbase_queue_coremask_work callback register failed \n", __func__);
		return ret;
	}

	return gpufreq_start();
}

static int kbase_platform_init(struct kbase_device *kbdev)
{
	struct device *dev = NULL;

	KBASE_DEBUG_ASSERT(kbdev != NULL);
	dev = kbdev->dev;
	dev->platform_data = kbdev;

	/* Init the hisilicon platform related data first. */
	if (kbase_platform_backend_init(kbdev)) {
		dev_err(kbdev->dev, "platform backend init failed.\n");
		return -EINVAL;
	}

	kbdev->hisi_dev_data.regulator = devm_regulator_get(dev, "gpu");
	if (IS_ERR(kbdev->hisi_dev_data.regulator)) {
		dev_err(kbdev->dev, "Failed to get regulator\n");
		return -EINVAL;
	}
	atomic_set(&kbdev->hisi_dev_data.runtime_pm_delay_ms, RUNTIME_PM_DELAY_SHORT);
	kbdev->hisi_dev_data.callbacks = (struct kbase_hisi_callbacks *)gpu_get_callbacks();

	atomic_set(&kbdev->hisi_dev_data.regulator_refcount, 0);

	kbase_dev = kbdev;

	if (gpufreq_init())
	dev_err(kbdev->dev, "gpufreq init failed, dvfs may not works.\n");

	/* dev name maybe modified by gpufreq_init */
	dev_set_name(dev, "gpu");
	return 0;
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
	gpufreq_term();
	/* term the hisilicon platform related data at last. */
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
		u32 value = readl(kbdev->hisi_dev_data.pctrlreg + PERICTRL_92_OFFSET);
		value &= G3D_MEM_DSLP_DISABLE;
		writel(value, kbdev->hisi_dev_data.pctrlreg + PERICTRL_92_OFFSET);
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
		atomic_read(&kbdev->hisi_dev_data.runtime_pm_delay_ms);

	// enable g3d memory dslp after shader core poweroff
	if (kbase_has_hi_feature(kbdev, KBASE_FEATURE_AUTO_MEM_SHUTDOWN)) {
		u32 value = readl(kbdev->hisi_dev_data.pctrlreg + PERICTRL_92_OFFSET);
		value |= G3D_MEM_DSLP_ENABLE;
		writel(value, kbdev->hisi_dev_data.pctrlreg + PERICTRL_92_OFFSET);
	}

#if HARD_RESET_AT_POWER_OFF
	/* Cause a GPU hard reset to test whether we have actually idled the GPU
	 * and that we properly reconfigure the GPU on power up.
	 * Usually this would be dangerous, but if the GPU is working correctly
	 * it should be completely safe as the GPU should not be active at this
	 * point. However this is disabled normally because it will most likely
	 * interfere with bus logging etc.
	 */
	KBASE_TRACE_ADD(kbdev, CORE_GPU_HARD_RESET, NULL, NULL, 0u, 0);
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
#ifdef CONFIG_PM_DEVFREQ
	gpufreq_devfreq_suspend();
#elif defined(CONFIG_MALI_MIDGARD_DVFS)
	kbase_platform_dvfs_enable(kbdev, false, 0);
#endif

	kbase_platform_off(kbdev);
}

static int pm_callback_runtime_on(struct kbase_device *kbdev)
{
	kbase_platform_on(kbdev);

#ifdef CONFIG_PM_DEVFREQ
	gpufreq_devfreq_resume();
#elif defined(CONFIG_MALI_MIDGARD_DVFS)
	if (kbase_platform_dvfs_enable(kbdev, true, 0) != MALI_TRUE)
		return -EPERM;
#endif

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
