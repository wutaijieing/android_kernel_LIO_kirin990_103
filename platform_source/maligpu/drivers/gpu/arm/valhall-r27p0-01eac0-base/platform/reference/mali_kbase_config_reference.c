/*
 *
 * (C) COPYRIGHT 2011-2017 ARM Limited. All rights reserved.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */



#include <linux/ioport.h>
#include <mali_kbase.h>
#include <mali_kbase_defs.h>
#include <mali_kbase_config.h>
#include <trace/events/power.h>
#if (KERNEL_VERSION(3, 13, 0) <= LINUX_VERSION_CODE)
#include <linux/pm_opp.h>
#else
#include <linux/opp.h>
#endif

#ifdef CONFIG_HW_VOTE_GPU_FREQ
#include <linux/platform_drivers/hw_vote.h>
#endif

#include <linux/pm_runtime.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/of_address.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <backend/gpu/mali_kbase_pm_internal.h>
#include <device/mali_kbase_device.h>
#include "mali_kbase_config_platform.h"
#include <linux/platform_drivers/gpufreq.h>

#define MALI_TRUE ((uint32_t)1)
#define MALI_FALSE ((uint32_t)0)

#define DEFAULT_POLLING_MS        20 // ms
#define KHZ 1000 // KHz
static struct kbase_device *kbase_dev;
#ifdef CONFIG_HW_VOTE_GPU_FREQ
static struct hvdev *gpu_hvdev;
#endif

#ifndef CONFIG_OF
static struct kbase_io_resources io_resources = {
	.job_irq_number = 68,
	.mmu_irq_number = 69,
	.gpu_irq_number = 70,
	.io_memory_region = {
	.start = 0xFC010000,
	.end = 0xFC010000 + (4096 * 4) - 1
	}
};
#endif /* CONFIG_OF */
static inline void kbase_platform_on(struct kbase_device *kbdev)
{
	if (!regulator_is_enabled(kbdev->reference_dev_data.regulator)) {
		if (unlikely(regulator_enable(kbdev->reference_dev_data.regulator))) {
			dev_err(kbdev->dev, "Failed to enable regulator");
			BUG_ON(1);
		}
	}
}

static inline void kbase_platform_off(struct kbase_device *kbdev)
{
	if (regulator_is_enabled(kbdev->reference_dev_data.regulator)) {
		if (unlikely(regulator_disable(kbdev->reference_dev_data.regulator))) {
			dev_err(kbdev->dev, "Failed to disable regulator");
		}
	}
}
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

	KBASE_DEBUG_ASSERT(dev != NULL);
	kbdev = (struct kbase_device *)dev->platform_data;
	KBASE_DEBUG_ASSERT(kbdev != NULL);
	old_freq = kbdev->reference_dev_data.devfreq->previous_freq;
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

	if (old_freq == freq)
		goto update_target;

	trace_clock_set_rate("clk-g3d", freq, raw_smp_processor_id());

#ifdef CONFIG_HW_VOTE_GPU_FREQ
	if (hv_set_freq(gpu_hvdev, freq / KHZ)) {
#else
	if (clk_set_rate((kbdev->reference_dev_data.clk), freq)) {
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
	stat->total_time = 100; /* base time 100ns */
#ifdef CONFIG_HW_VOTE_GPU_FREQ
	hv_get_last(gpu_hvdev, &freq);
	stat->current_frequency = (unsigned long)freq * KHZ;
#else
	stat->current_frequency = clk_get_rate(kbdev->reference_dev_data.clk);
#endif
	priv_data->vsync_hit = 1;
	priv_data->cl_boost = 0;
	stat->private_data = (void *)priv_data;

	return 0;
}

static struct devfreq_dev_profile mali_kbase_devfreq_profile = {
	/* it would be abnormal to enable devfreq monitor
	 * during initialization.
	 */
	.polling_ms = DEFAULT_POLLING_MS, // STOP_POLLING,
	.target = mali_kbase_devfreq_target,
	.get_dev_status = mali_kbase_get_dev_status,
};
#endif /* CONFIG_PM_DEVFREQ */

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
#ifdef CONFIG_PM_DEVFREQ
int reference_gpu_devfreq_initial_freq(const struct kbase_device *kbdev)
{
#ifdef CONFIG_HW_VOTE_GPU_FREQ
	u32 freq = 0;
	int ret = 0;
	unsigned long freq_hz;
	struct dev_pm_opp *opp = NULL;
	struct device *dev = kbdev->dev;

	gpu_hvdev = hvdev_register(dev, "gpu-freq", "vote-src-1");
	if (gpu_hvdev == NULL) {
		pr_err("[%s] register hvdev fail!\n", __func__);
	} else {
		ret = hv_get_last(gpu_hvdev, &freq);
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
	if (gpu_hvdev != NULL) {
		ret = hv_set_freq(gpu_hvdev, freq_hz / KHZ);
		if (ret != 0)
			pr_err("%s set initial frequency failed%d\n", __func__, ret);
	}

	mali_kbase_devfreq_profile.initial_freq = freq_hz;
#else
	mali_kbase_devfreq_profile.initial_freq =
		clk_get_rate(kbdev->reference_dev_data.clk);
#endif
	return 0;
}

void reference_gpu_devfreq_init(struct kbase_device *kbdev)
{
	struct device *dev = kbdev->dev;
	struct devfreq *df = NULL;
	int opp_count;
	int ret;

	opp_count = dev_pm_opp_get_opp_count(dev);
	if (opp_count <= 0)
		return;

	if (fhss_init(dev))
		dev_err(dev, "[gpufreq] Failed to init fhss\n");

	/* dev_pm_opp_of_add_table(dev) has been called by ddk */
	ret = reference_gpu_devfreq_initial_freq(kbdev);
	if (ret != 0)
		return;

	dev_set_name(dev, "gpufreq");
	df = devfreq_add_device(dev,
				&mali_kbase_devfreq_profile,
				GPU_DEFAULT_GOVERNOR,
				NULL);

	if (IS_ERR_OR_NULL(df)) {
		kbdev->reference_dev_data.devfreq = NULL;
		return;
	}
	kbdev->reference_dev_data.devfreq = df;
}

void reference_gpu_devfreq_resume(const struct devfreq *df)
{
#ifdef CONFIG_HW_VOTE_GPU_FREQ
	int ret;
	unsigned long freq;

	if (IS_ERR_OR_NULL(df))
		return;

	freq = df->previous_freq;

	/* update last freq in hv driver */
	if (gpu_hvdev != NULL) {
		ret = hv_set_freq(gpu_hvdev, freq / KHZ);
		if (ret != 0)
			pr_err("%s recover frequency failed%d\n",
			       __func__, ret);
	}
#endif
}
#else
static inline void reference_gpu_devfreq_init(struct kbase_device *kbdev)
{
	(void)kbdev;
}

static inline void reference_gpu_devfreq_resume(const struct devfreq *df)
{
	(void)df;
}
#endif/*CONFIG_PM_DEVFREQ*/

static int kbase_init_device_tree(struct kbase_device * const kbdev)
{
	struct device_node *np = NULL;
	int ret;

	KBASE_DEBUG_ASSERT(kbdev != NULL);

	/* init soc special flag */
	kbdev->reference_dev_data.gpu_fpga_exist = 0;

#ifdef CONFIG_OF
	/* read outstanding value from dts*/
	np = of_find_compatible_node(NULL, NULL, "arm,mali-midgard");
	if (np) {
		ret = of_property_read_u32(np, "fpga-gpu-exist",
			&kbdev->reference_dev_data.gpu_fpga_exist);
		if (ret)
			dev_warn(kbdev->dev,
			"No fpga gpu exist setting, assume not exist\n");
	} else {
		dev_err(kbdev->dev,
			"not find device node arm,mali-midgard!\n");
	}
#endif

	return 0;
}

static int kbase_verify_fpga_exist(struct kbase_device *kbdev)
{
	int ret = 0;

	if (kbdev->reference_dev_data.gpu_fpga_exist) {
		unsigned int pctrl_value;
		void __iomem *pctrlreg;
		pctrlreg = ioremap(SYS_REG_PERICTRL_BASE_ADDR,
			SYS_REG_REMAP_SIZE);
		if (!pctrlreg) {
			dev_err(kbdev->dev, "Can't remap sys pctrl register window on platform\n");
			return -EINVAL;
		}
		pctrl_value = readl(pctrlreg +
			PERI_STAT_FPGA_GPU_EXIST) & PERI_STAT_FPGA_GPU_EXIST_MASK;
		iounmap(pctrlreg);
		if (pctrl_value == 0) {
			dev_err(kbdev->dev, "No FPGA FOR GPU\n");
			return -ENODEV;
		}
	}
	return ret;
}

static int kbase_platform_backend_init(struct kbase_device *kbdev)
{
	int err;

	err = kbase_init_device_tree(kbdev);
	if (err != 0) {
		dev_err(kbdev->dev, "Init special devce tree setting failed\n");
		return err;
	}

	// verify if FPGA, need use info of related register
	err = kbase_verify_fpga_exist(kbdev);
	if (err != 0) {
		dev_err(kbdev->dev, "No gpu hardware implementation on fpga\n");
		return -ENODEV;
	}

	return err;
}

static int kbase_platform_init(struct kbase_device *kbdev)
{
	struct device *dev = NULL;

	KBASE_DEBUG_ASSERT(kbdev != NULL);
	dev = kbdev->dev;
	dev->platform_data = kbdev;

	if (kbase_platform_backend_init(kbdev)) {
		dev_err(kbdev->dev, "platform backend init failed.\n");
		return -EINVAL;
	}

	kbdev->reference_dev_data.clk = devm_clk_get(dev, NULL);
	if (IS_ERR(kbdev->reference_dev_data.clk)) {
		dev_err(kbdev->dev, "Failed to get clk\n");
		return -EINVAL;
	}

	kbdev->reference_dev_data.regulator = devm_regulator_get(dev, "gpu");
	if (IS_ERR(kbdev->reference_dev_data.regulator)) {
		dev_err(kbdev->dev, "Failed to get regulator\n");
		return -EINVAL;
	}

	reference_gpu_devfreq_init(kbdev);

	kbase_dev = kbdev;
	/* dev name maybe modified by reference_gpu_devfreq_init */
	dev_set_name(dev, "gpu");
	return 0;
}

static void kbase_platform_term(struct kbase_device *kbdev)
{
	KBASE_DEBUG_ASSERT(kbdev != NULL);
#ifdef CONFIG_PM_DEVFREQ
	devfreq_remove_device(kbdev->reference_dev_data.devfreq);
#endif
}

struct kbase_platform_funcs_conf platform_funcs = {
	.platform_init_func = &kbase_platform_init,
	.platform_term_func = &kbase_platform_term,
};

static int pm_callback_power_on(struct kbase_device *kbdev)
{
	int result;
	struct device *dev = kbdev->dev;

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
	return 1;
}

static void pm_callback_power_off(struct kbase_device *kbdev)
{
	struct device *dev = kbdev->dev;
	int ret;
	int retry = 0;

	if (unlikely(dev->power.disable_depth > 0)) {
		kbase_platform_off(kbdev);
		return;
	}
	
	do {
		ret = pm_runtime_suspend(dev);
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
	devfreq_suspend_device(kbdev->reference_dev_data.devfreq);
#elif defined(CONFIG_MALI_MIDGARD_DVFS)
	kbase_platform_dvfs_enable(kbdev, false, 0);
#endif

	kbase_platform_off(kbdev);
}

static int pm_callback_runtime_on(struct kbase_device *kbdev)
{
	kbase_platform_on(kbdev);

#ifdef CONFIG_PM_DEVFREQ
	devfreq_resume_device(kbdev->reference_dev_data.devfreq);
#elif defined(CONFIG_MALI_MIDGARD_DVFS)
	if (kbase_platform_dvfs_enable(kbdev, true, 0) != MALI_TRUE)
		return -EPERM;
#endif

	return 0;
}

static inline void pm_callback_suspend(struct kbase_device *kbdev)
{
	pm_callback_runtime_off(kbdev);
}

static inline void pm_callback_resume(struct kbase_device *kbdev)
{
	if (!pm_runtime_status_suspended(kbdev->dev))
		pm_callback_runtime_on(kbdev);
	else
		pm_callback_power_on(kbdev);
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

static struct kbase_platform_config reference_platform_config = {
#ifndef CONFIG_OF
	.io_resources = &io_resources
#endif
};

struct kbase_platform_config *kbase_get_platform_config(void)
{
	return &reference_platform_config;
}
