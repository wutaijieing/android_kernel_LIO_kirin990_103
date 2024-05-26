/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2019. All rights reserved.
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

#include "mali_kbase_hisi_lb.h"

/**
 * Getter functions to retrieve lb callbacks.
 * We use kbase_device to get the callbacks not use lb_callbacks instance.
 */
hisi_lb_callbacks *kbase_hisi_get_lb_cbs(const struct kbase_device *kbdev)
{
	struct kbase_hisi_callbacks *callbacks = NULL;

	KBASE_DEBUG_ASSERT(kbdev != NULL);
	callbacks = kbdev->hisi_dev_data.callbacks;

	KBASE_DEBUG_ASSERT(callbacks != NULL);

	return callbacks->lb_cbs;
}

static int kbase_platform_lb_init(struct kbase_device *kbdev)
{
	/* This will parse the device tree and create policy info. */
	int err = kbase_mem_init_lb_policy(kbdev);

	if (err) {
		dev_err(kbdev->dev, "Init last buffer cache policy failed.\n");
		return err;
	}

	return kbase_lb_init_quota(kbdev);
}

static int kbase_platform_lb_term(struct kbase_device *kbdev)
{
	/* Term the last buffer policy manager. */
	kbase_mem_term_lb_policy(kbdev);

	/* Term the last buffer quota manager. */
	kbase_lb_term_quota(kbdev);

	return 0;
}

static int kbase_platform_lb_alloc(struct kbase_device *kbdev)
{
	struct kbase_quota_manager *manager = NULL;

	KBASE_DEBUG_ASSERT(kbdev);
	manager = kbdev->hisi_dev_data.quota_manager;
	KBASE_DEBUG_ASSERT(manager);

	/* Cancel the quota timer when alloc. */
	if (manager->active) {
		hrtimer_cancel(&manager->timer);
		manager->active = false;
	}

	/* Request last buffer quota again if quota released. */
	if (manager->released) {
		kbase_lb_quota_operator(kbdev, QUOTA_REQUEST);
		manager->released = false;
	}

	return 0;
}

static int kbase_platform_lb_free(struct kbase_device *kbdev)
{
	struct kbase_quota_manager *manager = NULL;

	KBASE_DEBUG_ASSERT(kbdev);
	manager = kbdev->hisi_dev_data.quota_manager;
	KBASE_DEBUG_ASSERT(manager);

	if (!manager->active) {
		hrtimer_start(&manager->timer,
			HR_TIMER_DELAY_MSEC(QUOTA_RELEASE_TIMEOUT_MS),
			HRTIMER_MODE_REL);
		manager->active = true;
	}

	return 0;
}

/* The hisi_lb_callbacks instance. */
hisi_lb_callbacks lb_callbacks = {
	.lb_init  = &kbase_platform_lb_init,
	.lb_term  = &kbase_platform_lb_term,
	.lb_alloc = &kbase_platform_lb_alloc,
	.lb_free  = &kbase_platform_lb_free,
};
