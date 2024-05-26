#include <mali_kbase.h>
#include <linux/of.h>
#ifdef CONFIG_MALI_LAST_BUFFER
#include <linux/platform_drivers/mm_lb.h>
#endif
#include "mali_kbase_hisi_lb_quota_manager.h"

static int kbase_lb_quota_operator(void *dev , quota_operator operater)
{
	int ret = 0;
	struct kbase_hisi_device_data *data;
	struct kbase_mem_cache_policy_info *info;
	struct kbase_device *kbdev = (struct kbase_device *)dev;
	KBASE_DEBUG_ASSERT(kbdev);
	data = &kbdev->hisi_dev_data;
	KBASE_DEBUG_ASSERT(data != NULL);

	/* Call hisi-driver function to operate the quota of GPU's policy id */
	info = data->cache_policy_info;
	while (info) {
		KBASE_DEBUG_ASSERT(info->policy_id != 0);

		ret = operater(info->policy_id);
		if (ret)
			break;

		info = info->next_info;
	}

	return ret;
}

static int kbase_lb_quota_request(void *dev)
{
	KBASE_DEBUG_ASSERT(dev);
#ifdef CONFIG_MALI_LAST_BUFFER
	return kbase_lb_quota_operator(dev, &lb_request_quota);
#endif

	/* Hisi driver does not support last buffer, ignore it. */
	return 0;
}

static int kbase_lb_quota_release(void *dev)
{
	KBASE_DEBUG_ASSERT(dev);
#ifdef CONFIG_MALI_LAST_BUFFER
	return kbase_lb_quota_operator(dev, &lb_release_quota);
#endif

	/* Hisi driver does not support last buffer, ignore it. */
	return 0;
}

static enum hrtimer_restart kbasep_quota_timer_callback(struct hrtimer *timer)
{
	struct kbase_device *kbdev;
	lb_quota_manager *manager = container_of(timer, lb_quota_manager, quota_timer);
	KBASE_DEBUG_ASSERT(manager);

	kbdev = manager->kbdev;
	KBASE_DEBUG_ASSERT(kbdev);

	kbase_lb_quota_release(kbdev);

	manager->quota_released = true;

	return HRTIMER_NORESTART;
}

static int kbase_lb_quota_timer_init(void *dev, struct hrtimer *quota_timer)
{
	KBASE_DEBUG_ASSERT(dev);
	KBASE_DEBUG_ASSERT(quota_timer);

	hrtimer_init(quota_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	quota_timer->function = kbasep_quota_timer_callback;

	return 0;
}

static int kbase_lb_quota_timer_start(void *dev, struct hrtimer *quota_timer)
{
	lb_quota_manager *manager;

	KBASE_DEBUG_ASSERT(dev);
	KBASE_DEBUG_ASSERT(quota_timer);

	manager = container_of(quota_timer, lb_quota_manager, quota_timer);
	KBASE_DEBUG_ASSERT(manager);
	manager->kbdev = (struct kbase_device*)dev;

	if (!manager->timer_active) {
		hrtimer_start(quota_timer,
			          HR_TIMER_DELAY_MSEC(manager->quota_release_timeout),
				      HRTIMER_MODE_REL);

		manager->timer_active = true;
	}

	return 0;
}

static int kbase_lb_quota_timer_cancel(void *dev, struct hrtimer *quota_timer)
{
	lb_quota_manager *manager;

	KBASE_DEBUG_ASSERT(dev);
	KBASE_DEBUG_ASSERT(quota_timer);

	manager = container_of(quota_timer, lb_quota_manager, quota_timer);
	KBASE_DEBUG_ASSERT(manager);

	hrtimer_cancel(quota_timer);
	manager->timer_active = false;

	return 0;
}

/* Hisi quota manager to manage the last buffer quota dynamically. */
lb_quota_manager quota_manager = {
	.kbdev = NULL,
	.timer_active = false,
	.quota_released = false,
	.quota_release_timeout = DEFAULT_RELEASE_CACHE_QUOTA_TIMEOUT_MS,

	.requestQuota = kbase_lb_quota_request,
	.releaseQuota = kbase_lb_quota_release,
	.quota_timer_init = kbase_lb_quota_timer_init,
	.quota_timer_start = kbase_lb_quota_timer_start,
	.quota_timer_cancel = kbase_lb_quota_timer_cancel,
};
