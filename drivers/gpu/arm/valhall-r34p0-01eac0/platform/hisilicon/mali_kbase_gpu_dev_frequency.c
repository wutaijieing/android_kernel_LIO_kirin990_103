/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2021. All rights reserved.
 * Description: This file describe GPU custom device frequency schedule method
 * Create: 2014-2-24
 */
#include "mali_kbase.h"
#include "mali_kbase_platform.h"
#ifdef CONFIG_GPU_IPA_THERMAL
#include <linux/devfreq_cooling.h>
#include "gpu_ipa/mali_kbase_ipa_ctx.h"
#include <platform_include/maligpu/linux/gpu_ipa_thermal.h>
#endif

#include <trace/events/power.h>
#if (KERNEL_VERSION(3, 13, 0) <= LINUX_VERSION_CODE)
#include <linux/pm_opp.h>
#else
#include <linux/opp.h>
#endif

#ifdef CONFIG_GPU_THROTTLE_DEVFREQ
#include <platform_include/maligpu/linux/gpu_hook.h>
#endif

#ifdef CONFIG_REPORT_VSYNC
#include <linux/export.h>
#endif
#include <linux/platform_drivers/gpufreq_v2.h>
#include "mali_kbase_gpu_dev_frequency.h"
#include "mali_kbase_config_platform.h"
#include <backend/gpu/mali_kbase_pm_internal.h>

#include "mali_kbase_gpu_callback.h"
#ifdef CONFIG_GPU_VIRTUAL_DEVFREQ
#include "mali_kbase_gpu_virtual_devfreq.h"
#endif

#define KHZ 1000 // KHz
#define MALI_TRUE ((uint32_t)1)
#define MALI_FALSE ((uint32_t)0)
#define DEFAULT_POLLING_MS        20 //ms

#ifdef CONFIG_GPU_THROTTLE_DEVFREQ
void gpu_devfreq_set_gpu_throttle(unsigned long thro_low_freq,
	unsigned long thro_high_freq, unsigned long *freq)
{
	unsigned long flags;
	int thro_hint;
	int thro_enable;
	struct kbase_device *kbdev = kbase_platform_get_device();

	if (kbdev == NULL || freq  == NULL) {
		pr_err("[Mali]kbase_dev is null or freq is null");
		return;
	}

	thro_enable = atomic_read(&kbdev->gpu_dev_data.thro_enable);
	if (thro_enable == 0)
		goto update_thro_hint;

	spin_lock_irqsave(&kbdev->pm.backend.metrics.lock, flags);
	thro_hint = kbdev->pm.backend.metrics.thro_hint;
	if (thro_hint == KBASE_JS_ATOM_SCHED_THRO_COUNT)
		thro_hint = kbdev->pm.backend.metrics.thro_hint_prev;
	else
		kbdev->pm.backend.metrics.thro_hint_prev = thro_hint;
	spin_unlock_irqrestore(&kbdev->pm.backend.metrics.lock, flags);

	if (thro_hint != KBASE_JS_ATOM_SCHED_THRO_DEFAULT) {
		if (thro_hint == KBASE_JS_ATOM_SCHED_THRO_LOW &&
			thro_low_freq != 0)
			*freq = thro_low_freq;
		if (thro_hint == KBASE_JS_ATOM_SCHED_THRO_HIGH &&
			thro_high_freq != 0)
			*freq = thro_high_freq;
	}

update_thro_hint:
	spin_lock_irqsave(&kbdev->pm.backend.metrics.lock, flags);
	kbdev->pm.backend.metrics.thro_hint =
		KBASE_JS_ATOM_SCHED_THRO_COUNT;
	spin_unlock_irqrestore(&kbdev->pm.backend.metrics.lock, flags);
}
#endif

#ifdef CONFIG_REPORT_VSYNC
/* DSS fb_vsync_register have refence on this function, add for compile */
void mali_kbase_pm_report_vsync(int buffer_updated)
{
	unsigned long flags;
	struct kbase_device *kbdev = kbase_platform_get_device();

	if (kbdev) {
		spin_lock_irqsave(&kbdev->pm.backend.metrics.lock, flags);
		kbdev->pm.backend.metrics.vsync_hit = buffer_updated;
		spin_unlock_irqrestore(&kbdev->pm.backend.metrics.lock, flags);
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

