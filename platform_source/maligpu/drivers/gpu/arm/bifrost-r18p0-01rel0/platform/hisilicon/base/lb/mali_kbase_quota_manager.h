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

/*
 * Last buffer quota management callbacks.
 *
 * The last buffer's quota management callbacks, we are used to manage the quota
 * timer and the interfaces used to request/release quota.
 */

#ifndef _KBASE_HISI_QUOTA_H_
#define _KBASE_HISI_QUOTA_H_

#include <mali_kbase.h>

/**
 * Default number of milliseconds before GPU to release the last buffer quota.
 * If timeout, GPU will release the cache quota actively.
 */
#define QUOTA_RELEASE_TIMEOUT_MS 100 /* 100ms */


typedef int (*quota_operator)(unsigned int policy_id);

/* struct kbase_quota_manager - This struct holds all of the infomation needed
 *                              to manage the quota.
 * @kbdev: The Kbase device.
 * @timer: The timer used to release/request quota
 * @active: Indicate the quota timer is active or not.
 * @released: Indicate the quota has already been released or not.
 */
struct kbase_quota_manager {
	struct kbase_device *kbdev;
	struct hrtimer timer;

	bool active;
	bool released;
};

/* enum kbase_quota_op - The enum to operate the last buffer quota.
 * @QUOTA_REQUEST: The op represents request quota.
 * @QUOTA_RELEASE: The op represents release quota.
 */
enum kbase_quota_op {
	QUOTA_REQUEST,
	QUOTA_RELEASE
};

/**
 * kbase_lb_quota_operator() - The uniform interface to operate
 *                             the last buffer quota.
 * @kbdev: Kbase device
 * @op: The enum of kbase_quota_op, must be one of QUOTA_REQUEST
 *      and QUOTA_RELEASE.

 * This funciton is used to operate the quota of last buffer.
 * Return: 0 for success, error code otherwise.
 */
int kbase_lb_quota_operator(const struct kbase_device *kbdev,
		enum kbase_quota_op op);

/**
 * kbase_lb_init_quota() - Allocate the quota manager and do init.
 * @kbdev: Kbase device
 *
 * This funciton will allocate the quota manager for kbase device,
 * and init a quota timer used when free quota.
 *
 * Return: 0 for success, error code otherwise.
 */
int kbase_lb_init_quota(struct kbase_device *kbdev);

/**
 * kbase_lb_term_quota() - free the quota manager and do deinit.
 * @kbdev: Kbase device
 *
 * This funciton will free the quota manager for kbase device,
 * and cancel the quota timer.
 *
 * Return: 0 for success, error code otherwise.
 */
void kbase_lb_term_quota(const struct kbase_device *kbdev);

#endif /* _KBASE_HISI_QUOTA_H_ */
