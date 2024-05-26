/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2020. All rights reserved.
 * Description: This file describe GPU related init
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

#include <linux/of.h>
#include <linux/platform_drivers/mm_lb.h>
#include "mali_kbase_policy_manager.h"

static int kbase_mem_get_lb_policy(const struct kbase_device *kbdev,
		struct kbase_policy_manager *manager)
{
	int ret = 0;
	struct device_node *np = NULL;
	const struct device_node *dt_node = NULL;
	const char *lb_name = NULL;
	unsigned int lb_pid = 0;
	unsigned int lb_policy = 0;

	/* Get the device tree node by name. */
	dt_node = of_find_node_by_path("/hisi,lb");
	if (dt_node == NULL) {
		dev_err(kbdev->dev, "NOT FOUND device node hisi,lb!\n");
		return -EINVAL;
	}

	/* Parse the device tree to get the system cache policy id. */
	for_each_child_of_node(dt_node, np) {
		ret = of_property_read_string(np, "lb-name", &lb_name);
		if (ret < 0 || NULL == strstr(lb_name, "gpu"))
			continue;

		/* Read out GPU's policy id & cache policy. */
		ret = of_property_read_u32(np, "lb-pid", &lb_pid);
		if (ret < 0) {
			dev_err(kbdev->dev, "Check the lb-pid of node %s\n",
				np->name);
			continue;
		}
		WARN_ON(lb_pid >= BASE_MEM_GROUP_COUNT);

		ret = of_property_read_u32(np, "lb-plc", &lb_policy);
		if (ret < 0) {
			dev_err(kbdev->dev, "Check the lb-plc of node %s\n",
				np->name);
			continue;
		}

		strncpy(manager->policy[lb_pid].name, lb_name, strlen(lb_name)); /* unsafe_function_ignore: strncpy */
		manager->policy[lb_pid].policy_id = lb_pid;
		manager->policy[lb_pid].cache_policy = lb_policy;
	}
	return ret;
}

int kbase_mem_init_lb_policy(struct kbase_device *kbdev)
{
	struct kbase_policy_manager *manager = NULL;

	KBASE_DEBUG_ASSERT(kbdev);

	/* Alloc the kbase_mem_lb_policy struct */
	manager = kzalloc(sizeof(struct kbase_policy_manager), GFP_KERNEL);
	if (!manager) {
		dev_err(kbdev->dev, "kzalloc kbase_policy_manager failed.");
		return -ENOMEM;
	}

	/* Parse the device tree and get the last buffer policy */
	if (kbase_mem_get_lb_policy(kbdev, manager)) {
		dev_err(kbdev->dev, "get the last buffer policy failed.");
		kfree(manager);
		return -EINVAL;
	}

	kbdev->gpu_dev_data.policy_manager = manager;
	return 0;
}

void kbase_mem_term_lb_policy(const struct kbase_device *kbdev)
{
	struct kbase_policy_manager *manager = NULL;

	KBASE_DEBUG_ASSERT(kbdev);

	manager = kbdev->gpu_dev_data.policy_manager;
	KBASE_DEBUG_ASSERT(manager);

	kfree(manager);
}
