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

#ifdef CONFIG_GPU_CORE_HOTPLUG
/* For Norr, default core mask is 0xFFFF */
#define CORE_MASK_LEVEL_1	4
#define CORE_MASK_LEVEL_2	8
#define CORE_MASK_LEVEL_3	12
#define CORE_MASK_LEVEL_4	16

#define CORE_MASK_LEVEL_1_VALUE	0x1111
#define CORE_MASK_LEVEL_2_VALUE	0x3333
#define CORE_MASK_LEVEL_3_VALUE	0x7777
#define CORE_MASK_LEVEL_4_VALUE	0xFFFF

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

	kbdev->pm.thermal_required_core_mask = updated_core_mask;
	mutex_lock(&kbdev->pm.lock);
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	if (kbdev->pm.debug_core_mask[0] != updated_core_mask)
		kbase_pm_set_debug_core_mask(kbdev, updated_core_mask,
			updated_core_mask, updated_core_mask);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
	mutex_unlock(&kbdev->pm.lock);
}
EXPORT_SYMBOL(gpu_thermal_cores_control);
#endif
