/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: This file describe GPU virtual dev frequency in low load scenario
 * Create: 2021-1-24
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

#include "mali_kbase_gpu_virtual_devfreq.h"
#include "mali_kbase_defs.h"
#ifdef CONFIG_GPU_VIRTUAL_DEVFREQ
#include <platform_include/maligpu/linux/gpu_hook.h>
#endif
#include <backend/gpu/mali_kbase_pm_internal.h>
#include <linux/platform_drivers/gpufreq.h>

#ifdef CONFIG_GPU_VIRTUAL_DEVFREQ
#ifdef CONFIG_MALI_BORR
#define CORESTACK_COUNT_MAX	6
#define CORESTACK_MASK_UNIT	0x1111ULL
#define CORESTACK_CORE_NUMS	4
#define CORESTACK_CORE_BITS	4
#define CORESTACK_BASE_NUMS	2
#endif

struct gpu_virtual_devfreq gpu_virtual_devfreq_data;

int gpu_virtual_devfreq_get_freq(unsigned long *freq)
{
	unsigned int i;
	struct gpu_virtual_devfreq *priv_data = &gpu_virtual_devfreq_data;
	u64 core_mask;
	unsigned long flags;
	struct kbase_device *kbdev = kbase_platform_get_device();

	if (freq == NULL)
		return -1;
	if (!priv_data->virtual_devfreq_support ||
		!priv_data->virtual_devfreq_enable)
		return 0;
	if (kbdev == NULL) {
		pr_err("[Mali]kbase_dev is null,seems platform uninitialized");
		return -1;
	}

	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	core_mask = kbase_pm_ca_get_core_mask(kbdev);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);

	mutex_lock(&priv_data->lock);
	for (i = 0; i < priv_data->num_virtual_opps; i++) {
		if (priv_data->vfreq_table[i].core_mask ==
			core_mask) {
			*freq = priv_data->vfreq_table[i].opp_freq;
			break;
		}
	}
	mutex_unlock(&priv_data->lock);

	return 0;
}

int gpu_virtual_devfreq_get_devfreq_min(unsigned long *devfreq_min)
{
	struct gpu_virtual_devfreq *priv_data = &gpu_virtual_devfreq_data;

	if (devfreq_min == NULL)
		return -1;
	if (!priv_data->virtual_devfreq_support ||
		!priv_data->virtual_devfreq_enable)
		return 0;

	mutex_lock(&priv_data->lock);
	*devfreq_min = priv_data->devfreq_min;
	mutex_unlock(&priv_data->lock);

	return 0;
}

int gpu_virtual_devfreq_set_core_mask(unsigned long virtual_freq,
	bool virtual_freq_flag)
{
	unsigned int i;
	struct gpu_virtual_devfreq *priv_data = &gpu_virtual_devfreq_data;
	u64 shader_present;
	struct kbase_device *kbdev = kbase_platform_get_device();

	if (!priv_data->virtual_devfreq_support ||
		!priv_data->virtual_devfreq_enable)
		return 0;

	if (kbdev == NULL) {
		pr_err("[Mali]kbase_dev is null,seems platform uninitialized");
		return 0;
	}
	shader_present = kbdev->gpu_props.props.raw_props.shader_present;

	mutex_lock(&priv_data->lock);
	if (virtual_freq_flag == true) {
		for (i = 0; i < priv_data->num_virtual_opps; i++) {
			if (priv_data->vfreq_table[i].opp_freq >=
				virtual_freq) {
				priv_data->desired_core_mask =
					priv_data->vfreq_table[i].core_mask;
				break;
			}
		}
		if (i == priv_data->num_virtual_opps)
			priv_data->desired_core_mask = shader_present;
	} else {
		priv_data->desired_core_mask = shader_present;
	}
	mutex_unlock(&priv_data->lock);

	return 0;
}

int gpu_virtual_devfreq_set_mode(bool flag)
{
	struct gpu_virtual_devfreq *priv_data = &gpu_virtual_devfreq_data;
	unsigned long flags;
	u64 shader_present;
	struct kbase_device *kbdev = kbase_platform_get_device();

	if (!priv_data->virtual_devfreq_support ||
		!priv_data->virtual_devfreq_enable)
		return 0;
	if (kbdev == NULL) {
		pr_err("[Mali]kbase_dev is null,seems platform uninitialized");
		return 0;
	}
	shader_present = kbdev->gpu_props.props.raw_props.shader_present;

	mutex_lock(&priv_data->lock);
	if (priv_data->virtual_devfreq_mode != flag) {
		priv_data->virtual_devfreq_mode = flag;
		spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
		priv_data->current_core_mask =
			kbase_pm_ca_get_core_mask(kbdev);
		spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
		priv_data->desired_core_mask = shader_present;
		queue_work(system_freezable_power_efficient_wq,
			&priv_data->set_coremask_work);
	}
	mutex_unlock(&priv_data->lock);

	return 0;
}

int gpu_virtual_devfreq_get_enable(bool *flag)
{
	struct gpu_virtual_devfreq *priv_data = &gpu_virtual_devfreq_data;
	struct kbase_device *kbdev = kbase_platform_get_device();

	if (flag == NULL)
		return -1;
	if (kbdev == NULL) {
		pr_err("[Mali]kbase_dev is null,seems platform uninitialized");
		return 0;
	}
	if (kbdev->gpu_dev_data.gpu_chip_type != 2) {
		pr_err("[Mali] Don't support virtual devfreq");
		return 0;
	}

	mutex_lock(&priv_data->lock);
	*flag = priv_data->virtual_devfreq_support &&
		priv_data->virtual_devfreq_enable;
	mutex_unlock(&priv_data->lock);

	return 0;
}

int gpu_virtual_devfreq_set_enable(bool flag)
{
	struct gpu_virtual_devfreq *priv_data = &gpu_virtual_devfreq_data;
	u64 shader_present;
	unsigned long flags;
	struct kbase_device *kbdev = kbase_platform_get_device();

	if (kbdev == NULL) {
		pr_err("[Mali]kbase_dev is null,seems platform uninitialized");
		return 0;
	}
	shader_present = kbdev->gpu_props.props.raw_props.shader_present;
	if (kbdev->gpu_dev_data.gpu_chip_type != 2) {
		pr_err("[Mali] Don't support virtual devfreq");
		return 0;
	}

	mutex_lock(&priv_data->lock);
	if (priv_data->virtual_devfreq_support &&
		priv_data->virtual_devfreq_enable != flag) {
		priv_data->virtual_devfreq_enable = flag;
		spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
		priv_data->current_core_mask =
			kbase_pm_ca_get_core_mask(kbdev);
		spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
		priv_data->desired_core_mask = shader_present;
		queue_work(system_freezable_power_efficient_wq,
			&priv_data->set_coremask_work);
	}
	mutex_unlock(&priv_data->lock);

	return 0;
}


static bool check_coremask_validity(u64 core_mask)
{
	unsigned int i;
	bool valid = false;
	struct gpu_virtual_devfreq *priv_data = &gpu_virtual_devfreq_data;
	u64 shader_present;
	struct kbase_device *kbdev = kbase_platform_get_device();

	if (kbdev == NULL) {
		pr_err("[Mali]kbase_dev is null,seems platform uninitialized");
		return 0;
	}
	shader_present = kbdev->gpu_props.props.raw_props.shader_present;

	mutex_lock(&priv_data->lock);
	for (i = 0; i < priv_data->num_virtual_opps; i++) {
		if (priv_data->vfreq_table[i].core_mask ==
			core_mask) {
			valid = true;
			break;
		}
	}
	if (i == priv_data->num_virtual_opps && core_mask == shader_present)
		valid = true;
	mutex_unlock(&priv_data->lock);

	return valid;
}

void mali_kbase_devfreq_set_coremask_worker(struct work_struct *work)
{
	u64 core_mask;
	bool valid = false;
	struct gpu_virtual_devfreq *priv_data = &gpu_virtual_devfreq_data;
	unsigned long flags;
	struct kbase_device *kbdev = kbase_platform_get_device();

	if (kbdev == NULL) {
		pr_err("[Mali]kbase_dev is null,seems platform uninitialized");
		return;
	}

#ifdef CONFIG_GPU_CORE_HOTPLUG
	/* don't debug core_mask if the thermal is controlling */
	if (kbdev->pm.thermal_controlling)
		return;
#endif

	core_mask = priv_data->desired_core_mask;
	valid = check_coremask_validity(core_mask);
	if (valid != true)
		return;

	mutex_lock(&kbdev->pm.lock);
	spin_lock_irqsave(&kbdev->hwaccess_lock, flags);
	if (priv_data->current_core_mask != core_mask)
		kbase_pm_set_debug_core_mask(kbdev, core_mask,
			core_mask, core_mask);
	spin_unlock_irqrestore(&kbdev->hwaccess_lock, flags);
	mutex_unlock(&kbdev->pm.lock);

	mutex_lock(&priv_data->lock);
	priv_data->current_core_mask = core_mask;
	mutex_unlock(&priv_data->lock);
}

static int kbase_virtual_devfreq_init_core_mask_table(
	struct kbase_device *kbdev)
{
	struct device_node *opp_node = of_parse_phandle(kbdev->dev->of_node,
			"virtual-operating-points-v2", 0);
	struct device_node *node;
	int i = 0;
	u64 shader_present = kbdev->gpu_props.props.raw_props.shader_present;
	struct gpu_virtual_devfreq *priv_data = &gpu_virtual_devfreq_data;
	int j = 0;

	if (!opp_node)
		return 0;
	if (!of_device_is_compatible(opp_node, "virtual-operating-points-v2"))
		return 0;

	for_each_available_child_of_node(opp_node, node) {
		u64 core_mask;
		u64 opp_freq;
		u64 real_freqs[BASE_MAX_NR_CLOCKS_REGULATORS];
		int err;

		err = of_property_read_u64(node, "opp-hz", &opp_freq);
		if (err) {
			dev_warn(kbdev->dev, "Failed to read opp-hz property with error %d\n",
					err);
			continue;
		}
#if BASE_MAX_NR_CLOCKS_REGULATORS > 1
		err = of_property_read_u64_array(node, "opp-hz-real",
				real_freqs, kbdev->nr_clocks);
#else
		err = of_property_read_u64_array(node, "opp-hz-real", real_freqs, 1);
#endif
		if (err < 0) {
			dev_warn(kbdev->dev, "Failed to read opp-hz-real property with error %d\n",
					err);
			continue;
		}

		if (of_property_read_u64(node, "opp-core-mask", &core_mask))
			core_mask = shader_present;
		if (core_mask != shader_present && corestack_driver_control) {
			dev_warn(kbdev->dev, "Ignoring OPP %llu - Dynamic Core Scaling not supported on this GPU\n",
					opp_freq);
			continue;
		}

		if (!core_mask) {
			dev_err(kbdev->dev, "OPP has invalid core mask of 0\n");
			return -ENODEV;
		}

		if (i >= VFREQ_OPP_COUNT_MAX) {
			priv_data->num_virtual_opps = i;
			dev_err(kbdev->dev, "OPP reaches maximum limit\n");
			return -ENODEV;
		}

		priv_data->vfreq_table[i].opp_freq = opp_freq;
		priv_data->vfreq_table[i].core_mask = core_mask;

		for (j = 0; j < BASE_MAX_NR_CLOCKS_REGULATORS; j++) {
			priv_data->vfreq_table[i].real_freqs[j] =
				real_freqs[j];
			priv_data->devfreq_min = real_freqs[j];
		}

		dev_info(kbdev->dev, "OPP %d : opp_freq=%llu core_mask=%llx\n",
				i, opp_freq, core_mask);
		i++;
	}

	priv_data->num_virtual_opps = i;

	return 0;
}

int gpu_get_vfreq_num(void)
{
	return gpu_virtual_devfreq_data.num_virtual_opps;
}

void gpu_get_vfreq_val(int num, u64 *opp_freq, u64 *core_mask)
{
	if (num < 0 || num >= VFREQ_OPP_COUNT_MAX)
		return;

	*opp_freq = gpu_virtual_devfreq_data.vfreq_table[num].opp_freq;
	*core_mask = gpu_virtual_devfreq_data.vfreq_table[num].core_mask;
}

#ifdef CONFIG_MALI_BORR
static inline int gpu_get_corestack_shift_bits(int corestack)
{
	if (corestack < 3)
		return corestack;
	else
		return corestack - 3 + 16;
}
#endif

static void gpu_lite_get_corestack_status(u64 shader_present,
	int stack_stat[], int stack_cnt_max)
{
	u64 core_mask = 0;
	u32 shift_bits = 0;
	int sum_bits = 0;
	int i = 0;
	int j = 0;

	for (i = 0; i < stack_cnt_max; i++) {
		shift_bits = gpu_get_corestack_shift_bits(i);
		core_mask = ((CORESTACK_MASK_UNIT << shift_bits) &
			shader_present) >> shift_bits;
		sum_bits = 0;
		for (j = 0; j < CORESTACK_CORE_NUMS; j++)
			sum_bits += (core_mask >>
				(u64)(j * CORESTACK_CORE_BITS)) & 0x1;
		if (sum_bits >= 0 && sum_bits <= CORESTACK_CORE_NUMS)
			stack_stat[i] = CORESTACK_CORE_NUMS - sum_bits;
		else
			stack_stat[i] = CORESTACK_CORE_NUMS;
	}
}

static void gpu_lite_update_virtual_devfreq(const int stack_stat[],
	int stack_cnt_max, struct gpu_virtual_devfreq *virtual_devfreq)
{
	int val = 0;
	int i = 0;

	for (i = 0; i < stack_cnt_max; i++)
		val += stack_stat[i];
	val /= CORESTACK_CORE_NUMS;
	if (val < 0 || val >= virtual_devfreq->num_virtual_opps) {
		virtual_devfreq->num_virtual_opps = 0;
		virtual_devfreq->virtual_devfreq_support = false;
		virtual_devfreq->virtual_devfreq_enable = false;
	} else {
		virtual_devfreq->num_virtual_opps -= val;
	}
}

static u64 gpu_lite_get_coremask(u64 shader_present, const int stack_stat[],
	int stack_cnt_max, int stack_num)
{
	u64 core_mask = 0;
	int shift_bits = 0;
	int i = 0;
	int j = 0;
	int k = 0;

	if (stack_num > stack_cnt_max)
		return 0;
	for (i = 0; i <= CORESTACK_CORE_NUMS; i++) {
		for (j = 0; j < stack_cnt_max; j++) {
			if (k  >=  stack_num)
				return core_mask;
			if (stack_stat[j] == i) {
				shift_bits = gpu_get_corestack_shift_bits(j);
				core_mask |= shader_present &
					(CORESTACK_MASK_UNIT << (unsigned int)shift_bits);
				k++;
			}
		}
	}
	if (k < stack_num)
		return 0;

	return core_mask;
}

static int gpu_lite_init_coremask(struct kbase_device *kbdev)
{
	struct gpu_virtual_devfreq *priv_data = &gpu_virtual_devfreq_data;
	u64 shader_present = kbdev->gpu_props.props.raw_props.shader_present;
	int stack_stat[CORESTACK_COUNT_MAX] = {0};
	u64 core_mask = 0;
	int i = 0;

	gpu_lite_get_corestack_status(shader_present,
		stack_stat, CORESTACK_COUNT_MAX);

	gpu_lite_update_virtual_devfreq(stack_stat,
		CORESTACK_COUNT_MAX, priv_data);

	for (i = 0; i < priv_data->num_virtual_opps; i++) {
		core_mask = gpu_lite_get_coremask(shader_present,
			stack_stat, CORESTACK_COUNT_MAX,
			i + CORESTACK_BASE_NUMS);
		if (core_mask != 0) {
			priv_data->vfreq_table[i].core_mask = core_mask;
		} else {
			priv_data->virtual_devfreq_support = false;
			priv_data->virtual_devfreq_enable = false;
			priv_data->num_virtual_opps = 0;
			return 0;
		}
	}

	return 0;
}

int kbase_virtual_devfreq_init(struct kbase_device *kbdev)
{
	struct gpu_virtual_devfreq *priv_data = kbase_get_virtual_devfreq_data();
	u64 shader_present = kbdev->gpu_props.props.raw_props.shader_present;

	KBASE_DEBUG_ASSERT(priv_data != NULL);
	mutex_init(&priv_data->lock);
	if (kbdev->gpu_dev_data.gpu_chip_type == 2) {
		priv_data->virtual_devfreq_support = true;
		priv_data->virtual_devfreq_enable = true;
	} else {
		priv_data->virtual_devfreq_support = false;
		priv_data->virtual_devfreq_enable = false;
		return 0;
	}

	INIT_WORK(&priv_data->set_coremask_work,
			mali_kbase_devfreq_set_coremask_worker);

	priv_data->devfreq_min = 0;

	kbase_virtual_devfreq_init_core_mask_table(kbdev);

	if (shader_present != CORE_MASK_ALL)
		gpu_lite_init_coremask(kbdev);

	priv_data->current_core_mask = shader_present;
	priv_data->desired_core_mask = shader_present;
	priv_data->virtual_devfreq_mode = true;

	return 0;
}

struct gpu_virtual_devfreq *kbase_get_virtual_devfreq_data(void)
{
	return &gpu_virtual_devfreq_data;
}

#endif
