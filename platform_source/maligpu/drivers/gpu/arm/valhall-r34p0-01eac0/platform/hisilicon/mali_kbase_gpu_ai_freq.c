/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2021. All rights reserved.
 * Description: This file describe GPU AI frequency schedule interfaces and functions
 * Create: 2019-2-24
 */
#include "mali_kbase.h"
#include "mali_kbase_platform.h"
#ifdef CONFIG_GPU_AI_FENCE_INFO
#include <platform_include/maligpu/linux/gpu_hook.h>
#endif
#ifdef CONFIG_MALI_LAST_BUFFER
#include "gpu_ipa/mali_kbase_ipa_ctx.h"
#endif

#ifdef CONFIG_GPU_AI_FENCE_INFO
static int mali_kbase_report_fence_info(struct kbase_fence_info *fence)
{
	struct kbase_device *kbdev = kbase_platform_get_device();
	if (kbdev == NULL)
		return -EINVAL;

	if (kbdev->gpu_dev_data.game_pid != fence->game_pid)
		kbdev->gpu_dev_data.signaled_seqno = 0;

	kbdev->gpu_dev_data.game_pid = fence->game_pid;
	fence->signaled_seqno = kbdev->gpu_dev_data.signaled_seqno;

	return 0;
}

int perf_ctrl_get_gpu_fence(void __user *uarg)
{
	struct kbase_fence_info gpu_fence;
	int ret;

	if (uarg == NULL)
		return -EINVAL;

	if (copy_from_user(&gpu_fence, uarg, sizeof(struct kbase_fence_info))) {
		pr_err("mali gpu: %s copy_from_user fail\n", __func__);
		return -EFAULT;
	}

	ret = mali_kbase_report_fence_info(&gpu_fence);
	if (ret != 0) {
		pr_err("mali gpu: get_gpu_fence mali fail, ret=%d\n", ret);
		return -EFAULT;
	}

	if (copy_to_user(uarg, &gpu_fence, sizeof(struct kbase_fence_info))) {
		pr_err("mali gpu: %s copy_to_user fail\n", __func__);
		return -EFAULT;
	}

	return 0;
}

/** The number of counter blocks that always present in the gpu.
 * - Job Manager
 * - Tiler
 *  */
#define ALWAYS_PRESENT_NUM_OF_HWCBLK_PER_GPU 2
/* Get performance counter raw dump blocks
 * The blocks include Job manager,Tiler,L2,Shader core
 **/
static unsigned int mali_kbase_get_hwc_buffer_size(void)
{
	struct kbase_gpu_props *kprops = NULL;
	unsigned int num_l2;
	unsigned int num_cores;
	struct kbase_device *kbdev = kbase_platform_get_device();

	if (kbdev == NULL)
		return 0;

	kprops = &kbdev->gpu_props;
	/* Number of L2 slice blocks */
	num_l2 = kprops->props.l2_props.num_l2_slices;
	/* Number of shader core blocks. coremask without the leading zeros
	 * Even coremask is not successive, the memory should reserve for dump`
	 */
	num_cores = fls64(kprops->props.coherency_info.group[0].core_mask);

	return ALWAYS_PRESENT_NUM_OF_HWCBLK_PER_GPU + num_l2 + num_cores;
}

int perf_ctrl_get_gpu_buffer_size(void __user *uarg)
{
	unsigned int gpu_buffer_size;

	if (uarg == NULL)
		return -EINVAL;

	gpu_buffer_size = mali_kbase_get_hwc_buffer_size();
	if (copy_to_user(uarg, &gpu_buffer_size, sizeof(unsigned int))) {
		pr_err("mali gpu: %s: copy_to_user fail\n", __func__);
		return -EFAULT;
	}

	return 0;
}

#ifdef CONFIG_MALI_LAST_BUFFER
/*
 * This function get lb_enable flag from AI freq schedule service
 * When enter games, AI freq will set lb_enable=1 to enable LB
 * When exit games, AI freq will set lb_enable=0 to bypass LB
 */
int perf_ctrl_enable_gpu_lb(void __user *uarg)
{
	unsigned int enable = 0;
	struct kbase_device *kbdev = kbase_platform_get_device();

	if (uarg == NULL || kbdev == NULL)
		return -EINVAL;

	if (copy_from_user(&enable, uarg, sizeof(unsigned int))) {
		pr_err("mali gpu: %s: Get LB enable fail\n", __func__);
		return -EFAULT;
	}
	if (enable != 0 && enable != 1) {
		pr_err("mali gpu: %s: Invalid LB parameters\n", __func__);
		return -EINVAL;
	}
	kbdev->gpu_dev_data.game_scene = enable;
	if (kbdev->gpu_dev_data.lb_enable == enable)
		return 0;
	mali_kbase_enable_lb(kbdev, enable);

	return 0;
}
#endif
#endif
