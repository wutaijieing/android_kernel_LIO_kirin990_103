/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2021. All rights reserved.
 * Description: This file describe GPU custom device frequency schedule method
 * Create: 2014-2-24
 */
#include "mali_kbase.h"
#include "mali_kbase_platform.h"
#ifdef CONFIG_GPU_IPA_THERMAL
#include <platform_include/maligpu/linux/gpu_ipa_thermal.h>
#include <lpm_thermal.h>
#endif
#ifdef CONFIG_DEVFREQ_THERMAL
#include <linux/devfreq_cooling.h>
#include "gpu_ipa/mali_kbase_ipa_ctx.h"
#endif

#include <trace/events/power.h>
#if (KERNEL_VERSION(3, 13, 0) <= LINUX_VERSION_CODE)
#include <linux/pm_opp.h>
#else
#include <linux/opp.h>
#endif

#ifdef CONFIG_HW_VOTE_GPU_FREQ
#include <linux/platform_drivers/hw_vote.h>
#endif

#ifdef CONFIG_REPORT_VSYNC
#include <linux/export.h>
#endif
#include <linux/platform_drivers/gpufreq.h>
#include "mali_kbase_gpu_dev_frequency.h"
#include "mali_kbase_config_platform.h"
#include <backend/gpu/mali_kbase_pm_internal.h>
#include <backend/gpu/mali_kbase_clk_rate_trace_mgr.h>

#define KHZ 1000 // KHz
#define MALI_TRUE ((uint32_t)1)
#define MALI_FALSE ((uint32_t)0)
#define DEFAULT_POLLING_MS        20 //ms

#ifdef CONFIG_PM_DEVFREQ
static struct devfreq_data gpu_devfreq_priv_data = {
	.vsync_hit = 0,
	.cl_boost = 0,
};

static inline void gpu_devfreq_rcu_read_lock(void)
{
#if (KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE)
	rcu_read_lock();
#endif
}

static inline void gpu_devfreq_rcu_read_unlock(void)
{
#if (KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE)
	rcu_read_unlock();
#endif
}

static inline void gpu_devfreq_opp_put(struct dev_pm_opp *opp)
{
#if (KERNEL_VERSION(4, 14, 0) <= LINUX_VERSION_CODE)
	dev_pm_opp_put(opp);
#endif
}

static int mali_kbase_devfreq_target(struct device *dev,
	unsigned long *t_freq, u32 flags)
{
	struct kbase_device *kbdev = NULL;
	unsigned long old_freq;
	struct dev_pm_opp *opp = NULL;
	unsigned long freq;
#ifdef CONFIG_IPA_THERMAL
	unsigned long freq_limit;
	int gpu_id;
#endif

	KBASE_DEBUG_ASSERT(dev != NULL);
	kbdev = (struct kbase_device *)dev->platform_data;
	KBASE_DEBUG_ASSERT(kbdev != NULL);
	old_freq = kbdev->gpu_dev_data.devfreq->previous_freq;
	gpu_devfreq_rcu_read_lock();
	opp = devfreq_recommended_opp(dev, t_freq, flags);
	if (IS_ERR(opp)) {
		pr_err("[mali]Failed to get Operating Performance Point\n");
		gpu_devfreq_rcu_read_unlock();
		return PTR_ERR(opp);
	}
	freq = dev_pm_opp_get_freq(opp);
	gpu_devfreq_opp_put(opp);
	gpu_devfreq_rcu_read_unlock();

#ifdef CONFIG_GPU_IPA_THERMAL
	gpu_id = ipa_get_actor_id("gpu");
	if (gpu_id < 0) {
		pr_err("[mali]Failed to get ipa actor id for gpu.\n");
		return -ENODEV;
	}
	freq_limit = ipa_freq_limit(gpu_id, freq);

	freq = freq_limit;
#endif

	if (old_freq == freq)
		goto update_target;

#ifdef CONFIG_HW_VOTE_GPU_FREQ
	struct kbase_clk_rate_trace_manager *clk_rtm = &kbdev->pm.clk_rtm;
	if (clk_rtm && kbdev->pm.backend.gpu_ready)
		kbase_clk_rate_trace_manager_notify_all(clk_rtm, 0, freq);
#endif

	trace_clock_set_rate("clk-g3d", freq, raw_smp_processor_id());
	gpuflink_notifiy(freq);
#ifdef CONFIG_HW_VOTE_GPU_FREQ
	if (hv_set_freq(kbdev->gpu_dev_data.gpu_hvdev, freq / KHZ)) {
#else
	if (clk_set_rate((kbdev->gpu_dev_data.clk), freq)) {
#endif
		pr_err("[mali]Failed to set gpu freqency, [%lu->%lu]\n",
			old_freq, freq);
		return -ENODEV;
	}

update_target:
	*t_freq = freq;

	return 0;
}

static int mali_kbase_get_dev_status(struct device *dev,
	struct devfreq_dev_status *stat)
{
	struct devfreq_data *priv_data = &gpu_devfreq_priv_data;
	struct kbase_device *kbdev = NULL;
#ifdef CONFIG_HW_VOTE_GPU_FREQ
	u32 freq = 0;
#endif

	KBASE_DEBUG_ASSERT(dev != NULL);
	KBASE_DEBUG_ASSERT(stat != NULL);
	kbdev = (struct kbase_device *)dev->platform_data;
	KBASE_DEBUG_ASSERT(kbdev != NULL);
	if (kbdev->pm.backend.metrics.kbdev != kbdev) {
		pr_err("%s pm backend metrics not initialized\n", __func__);
		return -EINVAL;
	}

	(void)kbase_pm_get_dvfs_action(kbdev);
	stat->busy_time = kbdev->pm.backend.metrics.utilisation;
	stat->total_time = 100; /* 100 is a percentage.*/
#ifdef CONFIG_HW_VOTE_GPU_FREQ
	hv_get_last(kbdev->gpu_dev_data.gpu_hvdev, &freq);
	stat->current_frequency = (unsigned long)freq * KHZ;
#else
	stat->current_frequency = clk_get_rate(kbdev->gpu_dev_data.clk);
#endif
	priv_data->vsync_hit = kbdev->pm.backend.metrics.vsync_hit;
	stat->private_data = (void *)priv_data;
#ifdef CONFIG_DEVFREQ_THERMAL
	/* Avoid sending HWC dump cmd to GPU when GPU is power-off */
	if (kbdev->pm.backend.gpu_powered)
		(void)mali_kbase_devfreq_detect_bound(kbdev,
			stat->current_frequency, stat->busy_time);
#if KERNEL_VERSION(4, 1, 15) <= LINUX_VERSION_CODE
	memcpy(&kbdev->gpu_dev_data.devfreq->last_status, stat, sizeof(*stat));
#else
	memcpy(&kbdev->gpu_dev_data.devfreq_cooling->last_status,
		stat, sizeof(*stat));
#endif
#endif
	return 0;
}

struct devfreq_dev_profile mali_kbase_devfreq_profile = {
	/* it would be abnormal to enable devfreq monitor
	 * during initialization.
	 */
	.polling_ms = DEFAULT_POLLING_MS, // STOP_POLLING,
	.target = mali_kbase_devfreq_target,
	.get_dev_status = mali_kbase_get_dev_status,
};
#endif /* CONFIG_PM_DEVFREQ */

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
#if MALI_USE_CSF
int kbase_platform_dvfs_event(struct kbase_device *kbdev, u32 utilisation)
{
	return 1;
}
#else
int kbase_platform_dvfs_event(struct kbase_device *kbdev, u32 utilisation,
	u32 util_gl_share, u32 util_cl_share[2])
{
	return 1;
}
#endif

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

#ifdef CONFIG_GPU_IPA_THERMAL
static void kbase_gpu_devfreq_cooling_init(struct kbase_device *kbdev)
{
	int err;

	ithermal_gpu_cdev_register(kbdev->dev->of_node, kbdev->gpu_dev_data.devfreq);
	kbdev->gpu_dev_data.devfreq_cooling = ithermal_get_gpu_cdev();
	if (IS_ERR_OR_NULL(kbdev->gpu_dev_data.devfreq_cooling)) {
		err = PTR_ERR(kbdev->gpu_dev_data.devfreq_cooling);
		dev_err(kbdev->dev,
			"Failed to register cooling device (%d)\n",
			err);
	}

}
#else
static inline void kbase_gpu_devfreq_cooling_init(struct kbase_device *kbdev)
{
	(void)kbdev;
}
#endif

#ifdef CONFIG_PM_DEVFREQ

static int kbase_gpu_hvdev_init(struct kbase_device *kbdev)
{
	KBASE_DEBUG_ASSERT(kbdev != NULL);
	struct device *dev = kbdev->dev;
	kbdev->gpu_dev_data.gpu_hvdev = hvdev_register(dev, "gpu-freq", "vote-src-1");
	if (kbdev->gpu_dev_data.gpu_hvdev == NULL) {
		pr_err("[%s] register hvdev fail!\n", __func__);
		return -EINVAL;
	}
	return 0;
}

int kbase_gpu_devfreq_initial_freq(struct kbase_device *kbdev)
{
	int ret = 0;
#ifdef CONFIG_HW_VOTE_GPU_FREQ
	u32 freq = 0;
	unsigned long freq_hz;
	struct dev_pm_opp *opp = NULL;
	struct device *dev = kbdev->dev;

	ret = kbase_gpu_hvdev_init(kbdev);
	if (ret == 0) {
		ret = hv_get_last(kbdev->gpu_dev_data.gpu_hvdev, &freq);
		if (ret != 0) {
			pr_err("[%s] get init frequency fail%d!\n", __func__, ret);
			freq = 0;
		}
	}

	freq_hz = (unsigned long)freq * KHZ;

	gpu_devfreq_rcu_read_lock();
	opp = dev_pm_opp_find_freq_ceil(dev, &freq_hz);
	if (IS_ERR(opp)) {
		pr_err("[%s] find initial opp failed\n", __func__);
		/* try to find an available frequency */
		freq_hz = 0;
		opp = dev_pm_opp_find_freq_ceil(dev, &freq_hz);
		if (IS_ERR(opp)) {
			pr_err("[%s] no available frequency exist\n", __func__);
			return -ENFILE;
		}
	}
	gpu_devfreq_opp_put(opp);
	gpu_devfreq_rcu_read_unlock();

	pr_info("[%s] initial frequency %lu\n", __func__, freq_hz);
	/* update last freq in hv driver */
	if (kbdev->gpu_dev_data.gpu_hvdev != NULL) {
		ret = hv_set_freq(kbdev->gpu_dev_data.gpu_hvdev, freq_hz / KHZ);
		if (ret != 0)
			pr_err("%s set initial frequency failed%d\n", __func__, ret);
	}

	mali_kbase_devfreq_profile.initial_freq = freq_hz;
#else
	mali_kbase_devfreq_profile.initial_freq =
		clk_get_rate(kbdev->gpu_dev_data.clk);
#endif
	return ret;
}

void kbase_gpu_devfreq_init(struct kbase_device *kbdev)
{
	struct device *dev = kbdev->dev;
	int opp_count;
	int ret;

	opp_count = dev_pm_opp_get_opp_count(dev);
	if (opp_count <= 0)
		return;

	if (fhss_init(dev))
		dev_err(dev, "[gpufreq] Failed to init fhss\n");

	/* dev_pm_opp_of_add_table(dev) has been called by ddk */
	ret = kbase_gpu_devfreq_initial_freq(kbdev);
	if (ret != 0)
		return;

	dev_set_name(dev, "gpufreq");
	kbdev->gpu_dev_data.devfreq = devfreq_add_device(dev,
		&mali_kbase_devfreq_profile,
		GPU_DEFAULT_GOVERNOR,
		NULL);

	if (!IS_ERR_OR_NULL(kbdev->gpu_dev_data.devfreq)) {
		kbase_gpu_devfreq_cooling_init(kbdev);
		ret = gpuflink_init(kbdev->gpu_dev_data.devfreq);
		if (ret != 0)
			dev_err(dev, "[gpufreq] Failed to init gpuflink\n");
	}
#ifdef CONFIG_DRG
	drg_devfreq_register(kbdev->gpu_dev_data.devfreq);
#endif
}

void kbase_gpu_devfreq_resume(struct kbase_device *kbdev)
{
#ifdef CONFIG_HW_VOTE_GPU_FREQ
	int ret;
	unsigned long freq;

	if (IS_ERR_OR_NULL(kbdev->gpu_dev_data.devfreq))
		return;

	freq = kbdev->gpu_dev_data.devfreq->previous_freq;

	/* update last freq in hv driver */
	if (kbdev->gpu_dev_data.gpu_hvdev != NULL) {
		ret = hv_set_freq(kbdev->gpu_dev_data.gpu_hvdev, freq / KHZ);
		if (ret != 0)
			pr_err("%s recover frequency failed%d\n",
			       __func__, ret);
	}
#endif
}
#endif /* CONFIG_PM_DEVFREQ */
