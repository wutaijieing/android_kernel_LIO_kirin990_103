/*
 * (C) COPYRIGHT 2014, 2018 HISI Limited. All rights reserved.
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

/*
 * Last buffer quota management callbacks.
 *
 * The last buffer's quota management callbacks, we are used to manage the quota
 * timer and the interfaces used to request/release quota.
 */

#ifndef _HISI_QUOTA_MANAGER_H_
#define _HISI_QUOTA_MANAGER_H_

#include <mali_kbase.h>

/**
 * Default number of milliseconds before GPU to release the last buffer quota.
 * If timeout, GPU will release the cache quota actively.
 */
#define DEFAULT_RELEASE_CACHE_QUOTA_TIMEOUT_MS (100) /* 100ms */


typedef int (*quota_operator)(unsigned int policy_id);

/* This struct holds all of the infomation needed to manage the quota. */
struct kbase_hisi_lb_quota_manager {

	struct kbase_device *kbdev;

	/* The timer used to release/request quota */
	struct hrtimer quota_timer;

	/* Indicate the quota timer is active or not. */
	bool timer_active;

	/* Indicate the quota has already been released or not. */
	bool quota_released;

	/* The timeout before release quota */
	u32 quota_release_timeout;

	/* This callback is used to release quota of GPU's policy id */
	int  (*requestQuota)(void *dev);

	/* This callback is used to release quota of GPU's policy id */
	int  (*releaseQuota)(void *dev);

	/* This callback is used to init the quota manager timer. */
	int (*quota_timer_init)(void *dev, struct hrtimer *quota_timer);

	/* This callback is used to start the quota manager timer. */
	int (*quota_timer_start)(void *dev, struct hrtimer *quota_timer);

	/* This callback is used to cancel the quota manager timer. */
	int (*quota_timer_cancel)(void *dev, struct hrtimer *quota_timer);
};

typedef struct kbase_hisi_lb_quota_manager lb_quota_manager;

#endif