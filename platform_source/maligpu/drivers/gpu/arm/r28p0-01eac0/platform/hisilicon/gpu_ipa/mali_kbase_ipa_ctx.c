/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2021. All rights reserved.
 * Description: This file describe GPU IPA feature
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
#include <linux/sysfs.h>

#include <mali_kbase.h>
#include "mali_kbase_hwcnt_types.h"
#include "mali_kbase_hwcnt_virtualizer.h"
#ifdef CONFIG_GPU_IPA_THERMAL
#include <linux/devfreq_cooling.h>
#include <platform_include/maligpu/linux/gpu_ipa_thermal.h>
#endif

#define NR_IPA_GROUPS                   8
#define NR_IPA_UNIT                     5
#define KBASE_PM_TIME_SHIFT             8
#define DEFAULT_BOUND_THRESHOLD         300
#define HWC_BOOTING_UTIL                45
#define HWC_IDLE_COUNT                  10
/* GPU LB bypass bandwidth threshold default value in MBps */
#define GPU_LB_BYPASS_THRESHOLD 1250
#define NR_BYTES_PER_BEAT 16
#define EXT_READ_BEATS_OFFSET 32
#define EXT_WRITE_BEATS_OFFSET 47
/* Number of L2 slice */
#define NR_L2_SLICE 4
/* Time convert */
#define TIME_UNIT 1000

struct kbase_ipa_context;

/**
 * struct ipa_group - represents a single IPA group
 * @name:               name of the IPA group
 * @capacitance:        capacitance constant for IPA group
 * @calc_power:         function to calculate power for IPA group
 */
struct ipa_group {
	const char *name;
	u32 capacitance;
	u32 (*calc_power)(struct kbase_ipa_context *,
		struct ipa_group *);
};

#include "gpu_ipa/mali_kbase_ipa_tables_ctx.h"

/**
 * struct kbase_ipa_context - IPA context per device
 * @kbdev:              pointer to kbase device
 * @groups:             array of IPA groups for this context
 * @vinstr_cli:         vinstr client handle
 * @vinstr_buffer:      buffer to dump hardware counters onto
 * @ipa_lock:           protects the entire IPA context
 */
struct kbase_ipa_context {
	struct kbase_device *kbdev;
	struct ipa_group groups[NR_IPA_GROUPS];
	struct kbase_hwcnt_virtualizer_client *hvirt_cli;
	struct kbase_hwcnt_dump_buffer dump_buf;
	struct mutex ipa_lock;
	u32 dyn_power[NR_IPA_UNIT];
	u32 time_diff[NR_IPA_UNIT];
	ktime_t timestamp;
	u32 bound_threshold;
	bool bound_detection_enable;
	u32 idle_beats;
};

static ssize_t show_ipa_group(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct kbase_device *kbdev = NULL;
	struct kbase_ipa_context *ctx = NULL;
	ssize_t count = -EINVAL;
	size_t i;

	KBASE_DEBUG_ASSERT(dev != NULL);
	kbdev = dev_get_drvdata(dev);
	KBASE_DEBUG_ASSERT(kbdev != NULL);
	KBASE_DEBUG_ASSERT(attr != NULL);
	KBASE_DEBUG_ASSERT(buf != NULL);
	ctx = kbdev->gpu_dev_data.ipa_ctx;
	KBASE_DEBUG_ASSERT(ctx != NULL);
	mutex_lock(&ctx->ipa_lock);
	for (i = 0; i < ARRAY_SIZE(ctx->groups); i++) {
		if (!strcmp(ctx->groups[i].name, attr->attr.name)) {
			count = snprintf(buf, PAGE_SIZE, "%lu\n", /* unsafe_function_ignore: snprintf */
				(unsigned long)ctx->groups[i].capacitance);
			break;
		}
	}
	mutex_unlock(&ctx->ipa_lock);
	return count;
}

static ssize_t set_ipa_group(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct kbase_device *kbdev = NULL;
	struct kbase_ipa_context *ctx = NULL;
	unsigned long capacitance = 0;
	size_t i;
	int err;

	KBASE_DEBUG_ASSERT(dev != NULL);
	kbdev = dev_get_drvdata(dev);
	KBASE_DEBUG_ASSERT(kbdev != NULL);
	KBASE_DEBUG_ASSERT(attr != NULL);
	KBASE_DEBUG_ASSERT(buf != NULL);
	ctx = kbdev->gpu_dev_data.ipa_ctx;
	KBASE_DEBUG_ASSERT(ctx != NULL);

	err = kstrtoul(buf, 0, &capacitance);
	if (err < 0)
		return err;
	if (capacitance > U32_MAX)
		return -ERANGE;

	mutex_lock(&ctx->ipa_lock);
	for (i = 0; i < ARRAY_SIZE(ctx->groups); i++) {
		if (!strcmp(ctx->groups[i].name, attr->attr.name)) {
			ctx->groups[i].capacitance = capacitance;
			mutex_unlock(&ctx->ipa_lock);
			return count;
		}
	}
	mutex_unlock(&ctx->ipa_lock);
	return -EINVAL;
}

static ssize_t show_boundth(struct device *dev, struct device_attribute *attr,
	char *buf)
{
	struct kbase_device *kbdev = NULL;
	struct kbase_ipa_context *ctx = NULL;
	ssize_t count;

	KBASE_DEBUG_ASSERT(dev != NULL);
	kbdev = dev_get_drvdata(dev);
	KBASE_DEBUG_ASSERT(kbdev != NULL);
	KBASE_DEBUG_ASSERT(attr != NULL);
	KBASE_DEBUG_ASSERT(buf != NULL);
	ctx = kbdev->gpu_dev_data.ipa_ctx;
	KBASE_DEBUG_ASSERT(ctx != NULL);

	mutex_lock(&ctx->ipa_lock);
	count = snprintf(buf, PAGE_SIZE, "%lu\n", /* unsafe_function_ignore: snprintf */
		(unsigned long)ctx->bound_threshold);
	mutex_unlock(&ctx->ipa_lock);
	return count;
}

static ssize_t set_boundth(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct kbase_device *kbdev = NULL;
	struct kbase_ipa_context *ctx = NULL;
	unsigned long threshold;
	int err;

	KBASE_DEBUG_ASSERT(dev != NULL);
	kbdev = dev_get_drvdata(dev);
	KBASE_DEBUG_ASSERT(kbdev != NULL);
	KBASE_DEBUG_ASSERT(attr != NULL);
	KBASE_DEBUG_ASSERT(buf != NULL);
	ctx = kbdev->gpu_dev_data.ipa_ctx;
	KBASE_DEBUG_ASSERT(ctx != NULL);

	err = kstrtoul(buf, 0, &threshold);
	if (err < 0)
		return err;
	if (threshold > U32_MAX)
		return -ERANGE;

	mutex_lock(&ctx->ipa_lock);
	ctx->bound_threshold = threshold;
	mutex_unlock(&ctx->ipa_lock);
	return count;
}

static ssize_t show_bound_dect_enable(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count;
	struct kbase_device *kbdev = NULL;
	struct kbase_ipa_context *ctx = NULL;

	KBASE_DEBUG_ASSERT(dev != NULL);
	kbdev = dev_get_drvdata(dev);
	KBASE_DEBUG_ASSERT(kbdev != NULL);
	KBASE_DEBUG_ASSERT(buf != NULL);
	KBASE_DEBUG_ASSERT(attr != NULL);
	ctx = kbdev->gpu_dev_data.ipa_ctx;
	KBASE_DEBUG_ASSERT(ctx != NULL);

	mutex_lock(&ctx->ipa_lock);

	if (ctx->bound_detection_enable)
		count = snprintf(buf, PAGE_SIZE, "enable\n"); /* unsafe_function_ignore: snprintf */
	else
		count = snprintf(buf, PAGE_SIZE, "disable\n"); /* unsafe_function_ignore: snprintf */

	mutex_unlock(&ctx->ipa_lock);

	return count;
}

static ssize_t set_bound_dect_enable(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct kbase_device *kbdev = NULL;
	struct kbase_ipa_context *ctx = NULL;
	bool enable_mode = false;

	KBASE_DEBUG_ASSERT(dev != NULL);
	kbdev = dev_get_drvdata(dev);
	KBASE_DEBUG_ASSERT(kbdev != NULL);
	KBASE_DEBUG_ASSERT(attr != NULL);
	KBASE_DEBUG_ASSERT(buf != NULL);
	ctx = kbdev->gpu_dev_data.ipa_ctx;
	KBASE_DEBUG_ASSERT(ctx != NULL);

	if (!strncmp("disable", buf, sizeof("disable") - 1))
		enable_mode = false;
	else if (!strncmp("enable", buf, sizeof("enable") - 1))
		enable_mode = true;
	else
		return -EINVAL;

	mutex_lock(&ctx->ipa_lock);
	ctx->bound_detection_enable = enable_mode;
	mutex_unlock(&ctx->ipa_lock);
	return count;
}

static DEVICE_ATTR(group0, S_IRUGO | S_IWUSR, show_ipa_group, set_ipa_group);
static DEVICE_ATTR(group1, S_IRUGO | S_IWUSR, show_ipa_group, set_ipa_group);
static DEVICE_ATTR(group2, S_IRUGO | S_IWUSR, show_ipa_group, set_ipa_group);
static DEVICE_ATTR(group3, S_IRUGO | S_IWUSR, show_ipa_group, set_ipa_group);
static DEVICE_ATTR(group4, S_IRUGO | S_IWUSR, show_ipa_group, set_ipa_group);
static DEVICE_ATTR(group5, S_IRUGO | S_IWUSR, show_ipa_group, set_ipa_group);
static DEVICE_ATTR(group6, S_IRUGO | S_IWUSR, show_ipa_group, set_ipa_group);
static DEVICE_ATTR(group7, S_IRUGO | S_IWUSR, show_ipa_group, set_ipa_group);
static DEVICE_ATTR(boundth, S_IRUGO | S_IWUSR, show_boundth, set_boundth);
static DEVICE_ATTR(bound_enable, S_IRUGO | S_IWUSR, show_bound_dect_enable,
	set_bound_dect_enable);

static struct attribute *kbase_ipa_attrs[] = {
	&dev_attr_group0.attr,
	&dev_attr_group1.attr,
	&dev_attr_group2.attr,
	&dev_attr_group3.attr,
	&dev_attr_group4.attr,
	&dev_attr_group5.attr,
	&dev_attr_group6.attr,
	&dev_attr_group7.attr,
	&dev_attr_boundth.attr,
	&dev_attr_bound_enable.attr,
	NULL,
};

static struct attribute_group kbase_ipa_attr_group = {
	.name = "ipa",
	.attrs = kbase_ipa_attrs,
};

static void init_ipa_groups(struct kbase_ipa_context *ctx)
{
	memcpy(ctx->groups, ipa_groups_def, sizeof(ctx->groups)); /* unsafe_function_ignore: memcpy */
}

#if defined(CONFIG_OF) && (KERNEL_VERSION(3, 7, 0) <= LINUX_VERSION_CODE)
static int update_ipa_groups_from_dt(struct kbase_ipa_context *ctx)
{
	struct kbase_device *kbdev = ctx->kbdev;
	struct device_node *np = NULL;
	struct device_node *child = NULL;
	struct ipa_group *group = NULL;
	size_t nr_groups;
	size_t i;
	int err;

	np = of_get_child_by_name(kbdev->dev->of_node, "ipa-groups");
	if (!np)
		return 0;

	nr_groups = 0;
	for_each_available_child_of_node(np, child)
		nr_groups++;
	if (!nr_groups || nr_groups > ARRAY_SIZE(ctx->groups)) {
		dev_err(kbdev->dev, "invalid number of IPA groups: %zu",
			nr_groups);
		err = -EINVAL;
		goto error;
	}

	for_each_available_child_of_node(np, child) {
		const char *name;
		u32 capacitance;

		name = of_get_property(child, "label", NULL);
		if (!name) {
			dev_err(kbdev->dev, "label missing for IPA group");
			err = -EINVAL;
			goto error;
		}
		err = of_property_read_u32(child, "capacitance",
			&capacitance);
		if (err < 0) {
			dev_err(kbdev->dev, "capacitance missing for IPA group");
			goto error;
		}

		for (i = 0; i < ARRAY_SIZE(ctx->groups); i++) {
			group = &ctx->groups[i];
			if (!strcmp(group->name, name)) {
				group->capacitance = capacitance;
				break;
			}
		}
	}

	of_node_put(np);
	return 0;
error:
	of_node_put(np);
	return err;
}
#else
static int update_ipa_groups_from_dt(struct kbase_ipa_context *ctx)
{
	return 0;
}
#endif

static int reset_ipa_groups(struct kbase_ipa_context *ctx)
{
	init_ipa_groups(ctx);
	return update_ipa_groups_from_dt(ctx);
}

static inline u32 read_hwcnt(const struct kbase_ipa_context *ctx,
	u32 offset)
{
	u8 *p = (u8 *)ctx->dump_buf.dump_buf;

	return *(u32 *)&p[offset];
}

static inline u32 add_saturate(u32 a, u32 b)
{
	if (U32_MAX - a < b)
		return U32_MAX;
	return a + b;
}

#ifdef CONFIG_MALI_LAST_BUFFER
/* Enable/Bypass GPU LB */
void mali_kbase_enable_lb(struct kbase_device *kbdev, unsigned int enable)
{
	struct kbase_policy_manager *manager = NULL;
	int i;

	manager = kbdev->gpu_dev_data.policy_manager;
	KBASE_DEBUG_ASSERT(manager);
	kbdev->gpu_dev_data.lb_enable = enable;
	for (i = 0; i < BASE_MEM_GROUP_COUNT; i++) {
		unsigned int pid = manager->policy[i].policy_id;
		if (pid == 0)
			continue;
		if (enable == 1)
			lb_gid_enable(pid);
		else
			lb_gid_bypass(pid);
	}
}

static u64 mali_kbase_get_bandwidth_counter(const struct kbase_ipa_context *ctx)
{
	int i;
	u64 total_beats = 0;
	u64 bandwidth;
	/*
	 * JM TILER L2 each block has 64 counters
	 * L2_EXT_READ_BEATS offsetïbytes）：JM+TILER+(L2*N)+32
	 * L2_EXT_WRITE_BEATS offsetïbytes）：JM+TILER+(L2*N)+47
	 * 4 L2 slice, calculate total beats
	 */
	for (i = 0; i < NR_L2_SLICE; i++) {
		total_beats += read_hwcnt(ctx, MMU_BASE + NR_BYTES_PER_CNT *
			(NR_CNT_PER_BLOCK * i + EXT_READ_BEATS_OFFSET));
		total_beats += read_hwcnt(ctx, MMU_BASE + NR_BYTES_PER_CNT *
			(NR_CNT_PER_BLOCK * i + EXT_WRITE_BEATS_OFFSET));
	}
	/*
	 * bandwidth = sum(read_beats + write_beats) * bus_width_in_bytes
	 * For Borr/Trym, 1 beats = 16 bytes
	 */
	bandwidth = total_beats * NR_BYTES_PER_BEAT;
	return bandwidth;
}

/*
 * Get total bytes of gpu bandwidth from external r/w hwcounters
 */
static void mali_kbase_calc_bandwidth(const struct kbase_ipa_context *ctx)
{
	u64 bandwidth;
	u64 ns_time;
	struct kbase_device *kbdev = ctx->kbdev;
	ktime_t diff;
	ktime_t now = ktime_get();
	unsigned int enable;

	/* Early return if NULL kbdev or in game scenario */
	if (kbdev == NULL || kbdev->gpu_dev_data.game_scene == 1)
		return;
	if (kbdev->gpu_dev_data.time_period_start == 0) {
		kbdev->gpu_dev_data.time_period_start = ktime_get();
		return;
	}

	diff = ktime_sub(now, kbdev->gpu_dev_data.time_period_start);
	ns_time = (u64)ktime_to_ns(diff);
	if (ns_time <= 0)
		return;
	kbdev->gpu_dev_data.time_period_start = now;

	bandwidth = mali_kbase_get_bandwidth_counter(ctx);
	/* Convert to MBps */
	bandwidth = bandwidth * TIME_UNIT / ns_time;
	if (bandwidth > GPU_LB_BYPASS_THRESHOLD)
		enable = 1;
	else
		enable = 0;
	if ((kbdev->gpu_dev_data.lb_enable == enable) ||
		(kbdev->gpu_dev_data.game_scene == 1))
		return;
	mali_kbase_enable_lb(kbdev, enable);
}
#endif

/*
 * Calculate power estimation based on hardware counter `c'
 * across all shader cores.
 */
static u32 calc_power_sc_single(const struct kbase_ipa_context *ctx,
	const struct ipa_group *group, u32 c)
{
	struct kbase_device *kbdev = ctx->kbdev;
	u64 core_mask;
	u32 base = 0;
	u32 r = 0;

	core_mask = kbdev->gpu_props.props.coherency_info.group[0].core_mask;
	while (core_mask != 0ull) {
		if ((core_mask & 1ull) != 0ull) {
			u64 n = read_hwcnt(ctx, base + c);
			u32 d = read_hwcnt(ctx, GPU_ACTIVE);
			u32 s = group->capacitance;

			if (d != 0)
				r = add_saturate(r, div_u64(n * s, d));
		}
		base += NR_CNT_PER_BLOCK * NR_BYTES_PER_CNT;
		core_mask >>= 1;
	}
	return r;
}

/*
 * Calculate power estimation based on hardware counter `c1'
 * and `c2' across all shader cores.
 */
static u32 calc_power_sc_double(const struct kbase_ipa_context *ctx,
	const struct ipa_group *group, u32 c1, u32 c2)
{
	struct kbase_device *kbdev = ctx->kbdev;
	u64 core_mask;
	u32 base = 0;
	u32 r = 0;

	core_mask = kbdev->gpu_props.props.coherency_info.group[0].core_mask;
	while (core_mask != 0ull) {
		if ((core_mask & 1ull) != 0ull) {
			u64 n = read_hwcnt(ctx, base + c1);
			u32 d = read_hwcnt(ctx, GPU_ACTIVE);
			u32 s = group->capacitance;

			if (d != 0) {
				r = add_saturate(r, div_u64(n * s, d));
				n = read_hwcnt(ctx, base + c2);
				r = add_saturate(r, div_u64(n * s, d));
			}
		}
		base += NR_CNT_PER_BLOCK * NR_BYTES_PER_CNT;
		core_mask >>= 1;
	}
	return r;
}

static u32 calc_power_single(const struct kbase_ipa_context *ctx,
	const struct ipa_group *group, u32 c)
{
	u64 n = read_hwcnt(ctx, c);
	u32 d = read_hwcnt(ctx, GPU_ACTIVE);
	u32 s = group->capacitance;

	if (d != 0)
		return div_u64(n * s, d);
	else
		return 0;
}

static u32 calc_power_group0(struct kbase_ipa_context *ctx,
	struct ipa_group *group)
{
	return calc_power_single(ctx, group, L2_ANY_LOOKUP);
}

static u32 calc_power_group1(struct kbase_ipa_context *ctx,
	struct ipa_group *group)
{
	return calc_power_single(ctx, group, TILER_ACTIVE);
}

static u32 calc_power_group2(struct kbase_ipa_context *ctx,
	struct ipa_group *group)
{
	return calc_power_sc_single(ctx, group, FRAG_ACTIVE);
}

static u32 calc_power_group3(struct kbase_ipa_context *ctx,
	struct ipa_group *group)
{
	return calc_power_sc_double(ctx, group, VARY_SLOT_32,
		VARY_SLOT_16);
}

static u32 calc_power_group4(struct kbase_ipa_context *ctx,
	struct ipa_group *group)
{
	return calc_power_sc_single(ctx, group, TEX_COORD_ISSUE);
}

static u32 calc_power_group5(struct kbase_ipa_context *ctx,
	struct ipa_group *group)
{
	return calc_power_sc_single(ctx, group, EXEC_INSTR_COUNT);
}

static u32 calc_power_group6(struct kbase_ipa_context *ctx,
	struct ipa_group *group)
{
	return calc_power_sc_double(ctx, group, BEATS_RD_LSC,
			BEATS_WR_LSC);
}

static u32 calc_power_group7(struct kbase_ipa_context *ctx,
	struct ipa_group *group)
{
	return calc_power_sc_single(ctx, group, EXEC_CORE_ACTIVE);
}

static int attach_vinstr(struct kbase_ipa_context *ctx)
{
	int errcode;
	struct kbase_device *kbdev = ctx->kbdev;
	struct kbase_hwcnt_virtualizer *hvirt = kbdev->hwcnt_gpu_virt;
	struct kbase_hwcnt_enable_map enable_map;
	const struct kbase_hwcnt_metadata *metadata =
		kbase_hwcnt_virtualizer_metadata(hvirt);

	if (!metadata)
		return -1;

	errcode = kbase_hwcnt_enable_map_alloc(metadata, &enable_map);
	if (errcode) {
		dev_err(kbdev->dev, "Failed to allocate IPA enable map");
		return errcode;
	}

	kbase_hwcnt_enable_map_enable_all(&enable_map);

	errcode = kbase_hwcnt_virtualizer_client_create(
		hvirt, &enable_map, &ctx->hvirt_cli);
	kbase_hwcnt_enable_map_free(&enable_map);
	if (errcode) {
		dev_err(kbdev->dev, "Failed to register IPA with virtualizer");
		ctx->hvirt_cli = NULL;
		return errcode;
	}

	errcode = kbase_hwcnt_dump_buffer_alloc(
		metadata, &ctx->dump_buf);
	if (errcode) {
		dev_err(kbdev->dev, "Failed to allocate IPA dump buffer");
		kbase_hwcnt_virtualizer_client_destroy(ctx->hvirt_cli);
		ctx->hvirt_cli = NULL;
		return errcode;
	}

	return 0;
}

static void detach_vinstr(struct kbase_ipa_context *ctx)
{
	if (ctx->hvirt_cli) {
		kbase_hwcnt_virtualizer_client_destroy(ctx->hvirt_cli);
		kbase_hwcnt_dump_buffer_free(&ctx->dump_buf);
		ctx->hvirt_cli = NULL;
	}
}

struct kbase_ipa_context *kbase_dynipa_init(struct kbase_device *kbdev)
{
	struct kbase_ipa_context *ctx = NULL;
	int err;
	int i;
	const void *bound_threshold_dts = NULL;
	const char *bound_detection_enable = NULL;
	u32 bound_threshold;

	KBASE_DEBUG_ASSERT(kbdev != NULL);
	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return NULL;

	mutex_init(&ctx->ipa_lock);
	ctx->kbdev = kbdev;

	err = reset_ipa_groups(ctx);
	if (err < 0)
		goto error;

	err = sysfs_create_group(&kbdev->dev->kobj, &kbase_ipa_attr_group);
	if (err < 0)
		goto error;

	ctx->timestamp = ktime_get();
	for (i = 0; i < NR_IPA_UNIT; i++) {
		ctx->dyn_power[i] = 0;
		ctx->time_diff[i] = 0;
	}

	ctx->bound_threshold = DEFAULT_BOUND_THRESHOLD;
	ctx->bound_detection_enable = true;
	ctx->idle_beats = 0;

#ifdef CONFIG_OF
	bound_threshold_dts = of_get_property(kbdev->dev->of_node,
		"gpu-bound-check-threshold", NULL);
	if (bound_threshold_dts) {
		bound_threshold = be32_to_cpup(bound_threshold_dts);
		ctx->bound_threshold = bound_threshold;

		dev_info(kbdev->dev,
			 "Using gpu bound-check-threshold %u set from dtb",
			 bound_threshold);
	}

	bound_detection_enable = of_get_property(kbdev->dev->of_node,
		"gpu-bound-detection-enable", NULL);
	if (bound_detection_enable) {
		if (!strcmp("disable", bound_detection_enable))
			ctx->bound_detection_enable = false;
	}
#endif

	return ctx;
error:
	kfree(ctx);
	return NULL;
}
KBASE_EXPORT_TEST_API(kbase_dynipa_init);

void kbase_dynipa_term(struct kbase_ipa_context *ctx)
{
	struct kbase_device *kbdev = NULL;

	KBASE_DEBUG_ASSERT(ctx != NULL);
	kbdev = ctx->kbdev;
	KBASE_DEBUG_ASSERT(kbdev != NULL);
	detach_vinstr(ctx);
	sysfs_remove_group(&kbdev->dev->kobj, &kbase_ipa_attr_group);
	kfree(ctx);
	ctx = NULL;
}
KBASE_EXPORT_TEST_API(kbase_dynipa_term);

u32 kbase_ipa_dynamic_core_power(struct kbase_ipa_context *ctx, int *err)
{
	struct ipa_group *group = NULL;
	u32 power = 0;
	size_t i;
	u64 start_ts_ns;
	u64 end_ts_ns;

	KBASE_DEBUG_ASSERT(ctx != NULL);
	KBASE_DEBUG_ASSERT(err != NULL);
	mutex_lock(&ctx->ipa_lock);
	if (!ctx->hvirt_cli) {
		*err = attach_vinstr(ctx);
		if (*err < 0)
			goto error;
	}
	*err = kbase_hwcnt_virtualizer_client_dump(ctx->hvirt_cli,
		&start_ts_ns, &end_ts_ns, &ctx->dump_buf);
	if (*err)
		goto error;
#ifdef CONFIG_MALI_LAST_BUFFER
	mali_kbase_calc_bandwidth(ctx);
#endif
	for (i = 0; i < ARRAY_SIZE(ctx->groups); i++) {
		group = &ctx->groups[i];
		power = add_saturate(power, group->calc_power(ctx, group));
	}
error:
	mutex_unlock(&ctx->ipa_lock);
	return power;
}
KBASE_EXPORT_TEST_API(kbase_ipa_dynamic_core_power);

static void kbase_ipa_hwcounts(struct kbase_ipa_context *ctx, int *err)
{
	u32 loading;
	static unsigned int statistics_idx;
	ktime_t time_now;
	ktime_t timestamp_diff;

	/*
	 * Dynamic power is defined in mw @ 1Ghz, Dynamic power will be high
	 * if GPU is extremely busy. But we found it is not quite proportional
	 * to GPU frequency. So it will be normalied it by GPU frequency. In
	 * different freqency range, we use different coefficient.We statistic
	 * latest 5 dynamic power samples. GPU bound event is signalled when
	 * average dynamic power value is high.
	 */
	loading = kbase_ipa_dynamic_core_power(ctx, err);
	if (*err)
		goto error;

	time_now = ktime_get();
	timestamp_diff = ktime_sub(time_now, ctx->timestamp);
	ctx->timestamp = time_now;
	ctx->time_diff[statistics_idx % NR_IPA_UNIT] =
		(u32) (ktime_to_ns(timestamp_diff) >> KBASE_PM_TIME_SHIFT);
	ctx->dyn_power[statistics_idx % NR_IPA_UNIT] = loading;

	statistics_idx++;

error:
	return;
}

u32 kbase_ipa_dynamic_bound_measure(struct kbase_ipa_context *ctx, int *err)
{
	size_t i;
	u32 total_power = 0;
	u32 total_time = 0;

	(void)kbase_ipa_hwcounts(ctx, err);
	for (i = 0; i < NR_IPA_UNIT; i++) {
		total_power += ctx->dyn_power[i];
		total_time += ctx->time_diff[i];
	}
	total_power = total_power * 1000; // mW
	total_time = total_time / 4000; // time shift

	if (total_time != 0)
		return total_power / total_time;

	return 0;
}

bool kbase_ipa_dynamic_bound_detect(struct kbase_ipa_context *ctx, int *err,
	unsigned long cur_freq, unsigned long load, bool ipa_enable)
{
	u32 bound_measure = 0;
	unsigned long mhz = cur_freq / 1000000; // MHz

	KBASE_DEBUG_ASSERT(ctx != NULL);
	KBASE_DEBUG_ASSERT(err != NULL);
	/* GPU won't bound, if GPU loading less than 45%. */
#ifdef CONFIG_MALI_LAST_BUFFER
	if (load < HWC_BOOTING_UTIL && ctx->kbdev->gpu_dev_data.game_scene == 1) {
#else
	if (load < HWC_BOOTING_UTIL) {
#endif
		ctx->idle_beats++;
		/*
		 * If GPU keep be in low ultilization rate for a while, disable
		 * the hwcount dump, so that the GPU can be idle.
		 */
		if (ctx->idle_beats > HWC_IDLE_COUNT && ctx->hvirt_cli)
			detach_vinstr(ctx);

		return false;
	}
	ctx->idle_beats = 0;
#ifdef CONFIG_MALI_LAST_BUFFER
	if (ipa_enable || ctx->kbdev->gpu_dev_data.game_scene == 0)
#else
	if (ipa_enable)
#endif
		bound_measure = kbase_ipa_dynamic_bound_measure(ctx, err);

	if (*err)
		goto error;

	if (ctx->bound_detection_enable && (mhz != 0))
		return ((bound_measure / mhz) > ctx->bound_threshold);

error:
	return false;
}

#ifdef CONFIG_DEVFREQ_THERMAL
void mali_kbase_devfreq_detect_bound_worker(struct work_struct *work)
{
	int err = 0;
	struct kbase_gpu_device_data *gpu_data = NULL;
	struct kbase_device *kbdev = NULL;
	bool bound_event = false;
	struct thermal_cooling_device *cdev = NULL;

	KBASE_DEBUG_ASSERT(work != NULL);
	gpu_data = container_of(work,
		struct kbase_gpu_device_data, bound_detect_work);
	KBASE_DEBUG_ASSERT(gpu_data != NULL);
	kbdev = container_of(gpu_data,
		struct kbase_device, gpu_dev_data);
	KBASE_DEBUG_ASSERT(kbdev != NULL);

#ifdef CONFIG_GPU_IPA_THERMAL
	cdev = ithermal_get_gpu_cdev();
	if (cdev == NULL)
		return;
#endif
#ifdef CONFIG_MALI_MIDGARD_DVFS
	bound_event = kbase_ipa_dynamic_bound_detect(
		kbdev->gpu_dev_data.ipa_ctx, &err,
		kbdev->gpu_dev_data.bound_detect_freq,
		kbdev->gpu_dev_data.bound_detect_btime,
#ifdef CONFIG_GPU_IPA_THERMAL
		cdev->ipa_enabled);
#else
		false);
#endif
#endif
#ifdef CONFIG_GPU_IPA_THERMAL
	cdev->ipa_enabled = false;

	cdev->bound_event = bound_event;
#endif
}

void mali_kbase_devfreq_detect_bound(struct kbase_device *kbdev,
	unsigned long cur_freq, unsigned long btime)
{
	kbdev->gpu_dev_data.bound_detect_freq = cur_freq;
	kbdev->gpu_dev_data.bound_detect_btime = btime;
	/*
	 * Use freezable workqueue so that the work can freeze
	 * when device is going to suspend
	 */
	queue_work(system_freezable_power_efficient_wq,
		&kbdev->gpu_dev_data.bound_detect_work);
}
#endif

