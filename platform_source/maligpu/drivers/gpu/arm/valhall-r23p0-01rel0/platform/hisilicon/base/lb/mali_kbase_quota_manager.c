/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2020. All rights reserved.
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

#include <mali_kbase.h>
#include <linux/of.h>
#include <linux/platform_drivers/mm_lb.h>
#include "mali_kbase_quota_manager.h"

/* The private function used to operate the quota using hisilicon
 * specific interfaces
 */
static int kbase_lb_quota_op(const struct kbase_device *kbdev,
		quota_operator operator)
{
	int i;
	struct kbase_policy_manager *manager = NULL;

	KBASE_DEBUG_ASSERT(kbdev);
	manager = kbdev->hisi_dev_data.policy_manager;
	KBASE_DEBUG_ASSERT(manager);

	/* Call hisi-driver function to operate the quota of GPU's policy id */
	for (i = 0; i < BASE_MEM_GROUP_COUNT; ++i) {
		unsigned int pid = manager->policy[i].policy_id;

		if (pid && operator(pid))
			break;
	}

	return 0;
}

int kbase_lb_quota_operator(const struct kbase_device *kbdev,
		enum kbase_quota_op op)
{
	quota_operator func = NULL;

	switch (op) {
	case QUOTA_REQUEST:
		func = &lb_request_quota;
		break;
	case QUOTA_RELEASE:
		func = &lb_release_quota;
		break;
	default:
		KBASE_DEBUG_ASSERT_MSG(false, "unreachable");
		return -EINVAL;
	}

	return kbase_lb_quota_op(kbdev, func);
}

static enum hrtimer_restart kbasep_timer_callback(struct hrtimer *timer)
{
	struct kbase_quota_manager *manager = container_of(
		timer, struct kbase_quota_manager, timer);
	KBASE_DEBUG_ASSERT(manager);

	kbase_lb_quota_operator(manager->kbdev, QUOTA_RELEASE);
	manager->released = true;

	return HRTIMER_NORESTART;
}

int kbase_lb_init_quota(struct kbase_device *kbdev)
{
	struct kbase_quota_manager *manager = NULL;

	KBASE_DEBUG_ASSERT(kbdev);

	/* Alloc the kbase_quota_manager struct */
	manager = kzalloc(sizeof(struct kbase_quota_manager), GFP_KERNEL);
	if (!manager)
		return -ENOMEM;

	manager->kbdev = kbdev;
	hrtimer_init(&manager->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	manager->timer.function = kbasep_timer_callback;

	kbdev->hisi_dev_data.quota_manager = manager;
	return 0;
}

void kbase_lb_term_quota(const struct kbase_device *kbdev)
{
	struct kbase_quota_manager *manager = NULL;

	KBASE_DEBUG_ASSERT(kbdev);
	manager = kbdev->hisi_dev_data.quota_manager;
	KBASE_DEBUG_ASSERT(manager);

	hrtimer_cancel(&manager->timer);
	kfree(manager);
}
