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

#ifndef _HISI_LB_CALLBACK_H_
#define _HISI_LB_CALLBACK_H_

#include <mali_kbase.h>
#include "mali_kbase_hisi_lb_cache_policy.h"
#include "mali_kbase_hisi_lb_mem_pools.h"
#include "mali_kbase_hisi_lb_quota_manager.h"

/**
 * struct kbase_hisi_lb_callbacks - struct holds all of the callbacks related with last buffer.
 * @cache_callback: The callbacks of last buffer cache policy info.
 * @pools_callback: The callbacks of last buffer memory pools.
 * @quota_callback: The callbacks of last buffer quota. Used to manage the quota
 *                  timer and release/request quota.
 */
struct kbase_hisi_lb_callbacks {
	cache_policy_callbacks *cache_callback;
	lb_pools_callbacks     *pools_callback;
	lb_quota_manager       *quota_callback;
};

typedef struct kbase_hisi_lb_callbacks hisi_lb_callbacks;

/**
 * kbase_hisi_get_lb_cbs() - Get the pointer of hisi_lb_callbacks instance.
 * @kbdev: Kbase device
 *
 * Return: Pointer to hisi_lb_callbacks object.
 */
hisi_lb_callbacks *kbase_hisi_get_lb_cbs(struct kbase_device *kbdev);

/**
 * kbase_hisi_get_lb_pools_cbs() - Get the pointer of lb_pools_callbacks instance.
 * @kbdev: Kbase device
 *
 * Return: Pointer to lb_pools_callbacks object.
 */
lb_pools_callbacks *kbase_hisi_get_lb_pools_cbs(struct kbase_device *kbdev);

/**
 * kbase_hisi_get_lb_cache_cbs() - Get the pointer of cache_policy_callbacks instance.
 * @kbdev: Kbase device
 *
 * Return: Pointer to cache_policy_callbacks object.
 */
cache_policy_callbacks *kbase_hisi_get_lb_cache_cbs(struct kbase_device *kbdev);

/**
 * kbase_hisi_get_lb_quota_cbs() - Get the pointer of lb_quota_manager instance.
 * @kbdev: Kbase device
 *
 * Return: Pointer to lb_quota_manager object.
 */
lb_quota_manager *kbase_hisi_get_lb_quota_cbs(struct kbase_device *kbdev);

#endif /* _HISI_LB_CALLBACK_H_ */