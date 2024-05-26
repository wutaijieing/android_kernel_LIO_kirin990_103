/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2021. All rights reserved.
 * Description: This file describe GPU core hotplug feature
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
#include "mali_kbase.h"
#include "mali_kbase_platform.h"
#ifdef CONFIG_GPU_VIRTUAL_DEVFREQ
#include <platform_include/maligpu/linux/gpu_hook.h>
#endif

#ifdef CONFIG_GPU_CORE_HOTPLUG
/* For Norr, default core mask is 0xFFFF */
#if defined CONFIG_MALI_NORR_PHX || defined (CONFIG_MALI_NORR_CDC)
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
static u64 kbase_gpu_thermal_select_coremask(struct kbase_device *kbdev,
	u64 updated_core_mask, u32 *allowed_cores, u32 set)
{
	u32 available_cores = kbdev->gpu_dev_data.shader_present_lo_cfg;
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
static u64 kbase_gpu_thermal_lite_coremask(struct kbase_device *kbdev,
	u64 updated_core_mask, u64 cores)
{
	u64 core_mask = updated_core_mask &
		(u64)kbdev->gpu_dev_data.shader_present_lo_cfg;

	switch (cores) {
	case CORE_MASK_LEVEL_1: {
		u32 allowed_cores[3] = { 0x1111, 0x2222, 0x4444 };
		u32 set = sizeof(allowed_cores) / sizeof(allowed_cores[0]);
		core_mask = kbase_gpu_thermal_select_coremask(kbdev,
			updated_core_mask, allowed_cores, set);
		break;
	}
	case CORE_MASK_LEVEL_2: {
		u32 allowed_cores[3] = { 0x11111111, 0x22222222, 0x44444444 };
		u32 set = sizeof(allowed_cores) / sizeof(allowed_cores[0]);
		core_mask = kbase_gpu_thermal_select_coremask(kbdev,
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

static u64 kbase_gpu_thermal_update_coremask(struct kbase_device *kbdev, u64 cores)
{
	u64 updated_core_mask = CORE_MASK_LEVEL_4_VALUE;

	switch (cores) {
	case CORE_MASK_LEVEL_1:
		updated_core_mask = CORE_MASK_LEVEL_1_VALUE;
		kbdev->pm.thermal_controlling = true;
		break;
	case CORE_MASK_LEVEL_2:
		updated_core_mask = CORE_MASK_LEVEL_2_VALUE;
		kbdev->pm.thermal_controlling = true;
		break;
	case CORE_MASK_LEVEL_3:
		updated_core_mask = CORE_MASK_LEVEL_3_VALUE;
		kbdev->pm.thermal_controlling = true;
		break;
	case CORE_MASK_LEVEL_4:
		updated_core_mask = CORE_MASK_LEVEL_4_VALUE;
		kbdev->pm.thermal_controlling = false;
		break;
	default:
		dev_err(kbdev->dev,
			"Invalid cores set 0x%llx\n", cores);
		break;
	}

#if defined(CONFIG_MALI_BORR) && defined(CONFIG_GPU_SHADER_PRESENT_CFG)
	updated_core_mask = kbase_gpu_thermal_lite_coremask(kbdev,
		updated_core_mask, cores);
#endif
	return updated_core_mask;
}

void gpu_thermal_cores_control(u64 cores)
{
	u64 updated_core_mask;
	unsigned long flags;
	struct kbase_device *kbdev = kbase_platform_get_device();

	if (kbdev == NULL) {
		pr_err("[Mali]kbase_dev is null,seems platform uninitialized");
		return;
	}

	updated_core_mask = kbase_gpu_thermal_update_coremask(kbdev, cores);

#ifdef CONFIG_GPU_VIRTUAL_DEVFREQ
        if (cores != CORE_MASK_LEVEL_4)
                gpu_virtual_devfreq_set_enable(false);
#endif

	kbdev->pm.thermal_required_core_mask = updated_core_mask;
	mutex_lock(&kbdev->pm.lock);
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	if (kbdev->pm.debug_core_mask[0] != updated_core_mask)
		kbase_pm_set_debug_core_mask(kbdev, updated_core_mask,
			updated_core_mask, updated_core_mask);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
	mutex_unlock(&kbdev->pm.lock);

#ifdef CONFIG_GPU_VIRTUAL_DEVFREQ
        if (cores == CORE_MASK_LEVEL_4)
                gpu_virtual_devfreq_set_enable(true);
#endif
}
EXPORT_SYMBOL(gpu_thermal_cores_control);
#endif
