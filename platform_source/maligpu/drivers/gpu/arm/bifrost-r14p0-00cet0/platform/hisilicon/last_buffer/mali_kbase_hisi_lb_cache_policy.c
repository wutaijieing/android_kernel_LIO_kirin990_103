#include <mali_kbase.h>
#include <linux/of.h>
#include "mali_kbase_hisi_lb_cache_policy.h"

/**
 * kbase_cache_policy_info_alloc - alloc a kbase_mem_cache_policy_info object.
 * This funciton is used to alloc a kbase_mem_cache_policy_info object.
 */
static struct kbase_mem_cache_policy_info *kbase_cache_policy_info_alloc(void)
{
	return kzalloc(sizeof(struct kbase_mem_cache_policy_info), GFP_KERNEL);
}

/**
 * kbase_cache_policy_info_free - free a kbase_mem_cache_policy_info object.
 * This funciton is used to free a kbase_mem_cache_policy_info object.
 */
static void kbase_cache_policy_info_free(struct kbase_mem_cache_policy_info *info)
{
	kfree(info);
}

/**
 * kbase_cache_policy_info_new - new a kbase_mem_cache_policy_info object.
 * @dev: Kbase device.
 * @policy_id: The cache policy id.
 * @cache_policy: The last buffer cache policy.
 *
 * This funciton is used to alloc a kbase_mem_cache_policy_info object,
 * then init it.
 */
static int kbase_cache_policy_info_new(void *dev,
                                       unsigned int policy_id,
                                       unsigned int cache_policy)
{
	struct kbase_device *kbdev;
	struct kbase_hisi_device_data *data;
	struct kbase_mem_cache_policy_info *info;

	kbdev = (struct kbase_device *)dev;
	KBASE_DEBUG_ASSERT(kbdev != NULL);
	data = &kbdev->hisi_dev_data;
	KBASE_DEBUG_ASSERT(data != NULL);

	info = kbase_cache_policy_info_alloc();
	if (!info) {
		dev_err(kbdev->dev, "Allocate cache policy info failed.\n");
		return -ENOMEM;
	}

	info->policy_id = policy_id;
	info->cache_policy = cache_policy;
	info->next_info = data->cache_policy_info;

	data->cache_policy_info = info;
	data->nr_cache_policy++;

	return 0;
}

/**
 * kbase_cache_policy_info_delete - delete a kbase_mem_cache_policy_info object.
 * @dev: Kbase device.
 * @policy_id: The cache policy id.
 *
 * This funciton is used to remove a kbase_mem_cache_policy_info object from list,
 * then free it.
 */
static int kbase_cache_policy_info_delete(void *dev, unsigned int policy_id)
{
	struct kbase_device *kbdev;
	struct kbase_hisi_device_data *data;
	struct kbase_mem_cache_policy_info *prev;
	struct kbase_mem_cache_policy_info *info;

	kbdev = (struct kbase_device *)dev;
	KBASE_DEBUG_ASSERT(kbdev != NULL);
	data = &kbdev->hisi_dev_data;
	KBASE_DEBUG_ASSERT(data != NULL);
	info = data->cache_policy_info;

	while (info) {
		if (info->policy_id == policy_id) { /* find it */
			if (info == data->cache_policy_info) {
				data->cache_policy_info = info->next_info;
			} else {
				prev->next_info = info->next_info;
			}

			kbase_cache_policy_info_free(info);
			data->nr_cache_policy--;
			return 0;
		}

		prev = info;
		info = info->next_info;
	}

	dev_warn(kbdev->dev, "Try to delete invalid policy info with id(%d)\n", policy_id);
	return -EINVAL;
}

/**
 * kbase_destroy_cache_policy_info - destroy all the kbase_mem_cache_policy_info object.
 * @dev: Kbase device.
 *
 * Loop the list to remove the kbase_mem_cache_policy_info object and free them.
 */
static void kbase_destroy_cache_policy_info(void *dev)
{
	struct kbase_device *kbdev;
	struct kbase_hisi_device_data *data;
	struct kbase_mem_cache_policy_info *info;

	kbdev = (struct kbase_device *)dev;
	KBASE_DEBUG_ASSERT(kbdev != NULL);
	data = &kbdev->hisi_dev_data;
	KBASE_DEBUG_ASSERT(data != NULL);

	info = data->cache_policy_info;
	while (info) {
		struct kbase_mem_cache_policy_info * next = info->next_info;
		kbase_cache_policy_info_free(info);
		info = next;
		data->nr_cache_policy--;
	}

	data->cache_policy_info = NULL;
	KBASE_DEBUG_ASSERT(data->nr_cache_policy == 0);
}

/**
 * kbase_create_cache_policy_info - Parse device tree to create cache policy info object.
 * @dev:  Kbase device.
 *
 * This funciton is used to parse the specific device tree and get all of the GPU's policy id
 * and cache policy, we create a kbase_mem_cache_policy_info object to hold these.
 */
static int kbase_create_cache_policy_info(void *dev)
{
	struct device_node *np;
	const struct device_node *dt_node;
	const char *plc_name;
	int ret = 0;
	unsigned int policy_id = 0;
	unsigned int alloc_plc = 0;

	struct kbase_device *kbdev = (struct kbase_device *)dev;
	if (!kbdev)
		return -ENODEV;

	dt_node = of_find_node_by_path("/hisi,lb");
	if (NULL == dt_node) {
		dev_err(kbdev->dev, "NOT FOUND device node hisi,lb!\n");
		return -EINVAL;
	}

	/* Parse the device tree to get the system cache policy id. */
	for_each_child_of_node(dt_node, np) {
		ret = of_property_read_string(np, "lb-name", &plc_name);
		if (ret < 0) {
			dev_err(kbdev->dev, "invalid lb-name in node %s, please check the name.\n", np->name);
			continue;
		}

		/* Check the device node is GPU's or not. */
		if (NULL == strstr(plc_name, "gpu")) {
			continue;
		}

		/* Read out GPU's policy id & cache policy. */
		ret = of_property_read_u32(np, "lb-pid", &policy_id);
		if (ret < 0) {
			dev_err(kbdev->dev, "check the lb-pid of node %s\n", np->name);
			continue;
		}

		ret = of_property_read_u32(np, "lb-plc", &alloc_plc);
		if (ret < 0) {
			dev_err(kbdev->dev, "check the lb-plc of node %s\n", np->name);
			continue;
		}

		if (kbase_cache_policy_info_new(kbdev, policy_id, alloc_plc)) {
			goto failed;
		}
	}

	return 0;

failed:
	kbase_destroy_cache_policy_info(kbdev);
	return ret;
}

/**
 * kbase_get_cache_policy_id - Get the cache policy id from the given cache policy.
 * @dev:  Kbase device.
 * @cache_policy: The specific cache policy.
 *
 * This funciton is used to get the last buffer's policy id from cache_policy.
 */
static unsigned int kbase_get_cache_policy_id(void *dev, u8 cache_policy)
{
	struct kbase_device *kbdev;
	struct kbase_hisi_device_data *data;
	struct kbase_mem_cache_policy_info *info;

	kbdev = (struct kbase_device *)dev;
	KBASE_DEBUG_ASSERT(kbdev != NULL);
	data = &kbdev->hisi_dev_data;
	KBASE_DEBUG_ASSERT(data != NULL);

	info = data->cache_policy_info;
	while (info) {
		/* The policy id is saved in info->cache_policy's lower 4 bits and
		 * cache_policy's higher 4 bits.
		 */
		if ((info->cache_policy & 0xF) == ((cache_policy & 0xF0) >> 4))
			return info->policy_id;

		info = info->next_info;
	}

	/* Can not find, it is not an error, we think it as normal memory,
	 * that is, bypass last buffer.
	 */

	return 0;
}

/* Hisi callbacks used to manage the last buffer's cache policy info object. */
cache_policy_callbacks cache_callbacks = {

	.lb_create_policy_info = kbase_create_cache_policy_info,
	.lb_destroy_policy_info = kbase_destroy_cache_policy_info,
	.lb_new_policy_info = kbase_cache_policy_info_new,
	.lb_del_policy_info = kbase_cache_policy_info_delete,
	.lb_get_policy_id = kbase_get_cache_policy_id,

};
