/*
 *
 * (C) COPYRIGHT 2017 ARM Limited. All rights reserved.
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
#include "mali_kbase_hisi_lb_callback.h"

extern cache_policy_callbacks cache_callbacks;
extern lb_pools_callbacks pools_callbacks;
extern lb_quota_manager quota_manager;

/**
 * Getter functions to retrieve lb callbacks.
 * We use kbase_device to get the callbacks not use lb_callbacks instance.
 */
hisi_lb_callbacks *kbase_hisi_get_lb_cbs(struct kbase_device *kbdev)
{
	struct kbase_hisi_callbacks *callbacks;

	KBASE_DEBUG_ASSERT(NULL != kbdev);
	callbacks = kbdev->hisi_dev_data.callbacks;
	KBASE_DEBUG_ASSERT(NULL != callbacks);

	return callbacks->lb_cbs;
}

/**
 * Getter functions to retrieve lb pools callbacks.
 * We use kbase_device to get the callbacks not use lb_callbacks instance.
 */
lb_pools_callbacks *kbase_hisi_get_lb_pools_cbs(struct kbase_device *kbdev)
{
	hisi_lb_callbacks *lb_cbs;

	KBASE_DEBUG_ASSERT(NULL != kbdev);
	lb_cbs= kbase_hisi_get_lb_cbs(kbdev);
	KBASE_DEBUG_ASSERT(NULL != lb_cbs);

	return lb_cbs->pools_callback;
}

/**
 * Getter functions to retrieve lb cache callbacks.
 * We use kbase_device to get the callbacks not use lb_callbacks instance.
 */
cache_policy_callbacks *kbase_hisi_get_lb_cache_cbs(struct kbase_device *kbdev)
{
	hisi_lb_callbacks *lb_cbs;

	KBASE_DEBUG_ASSERT(NULL != kbdev);
	lb_cbs= kbase_hisi_get_lb_cbs(kbdev);
	KBASE_DEBUG_ASSERT(NULL != lb_cbs);

	return lb_cbs->cache_callback;
}

/**
 * Getter functions to retrieve lb quota callbacks.
 * We use kbase_device to get the callbacks not use lb_callbacks instance.
 */
lb_quota_manager *kbase_hisi_get_lb_quota_cbs(struct kbase_device *kbdev)
{
	hisi_lb_callbacks *lb_cbs;

	KBASE_DEBUG_ASSERT(NULL != kbdev);
	lb_cbs= kbase_hisi_get_lb_cbs(kbdev);
	KBASE_DEBUG_ASSERT(NULL != lb_cbs);

	return lb_cbs->quota_callback;
}

/* The hisi_lb_callbacks instance. */
hisi_lb_callbacks lb_callbacks = {
	.cache_callback = &cache_callbacks,
	.pools_callback = &pools_callbacks,
	.quota_callback = &quota_manager,
};
