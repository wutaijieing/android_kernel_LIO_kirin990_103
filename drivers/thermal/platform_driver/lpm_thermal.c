#include "ipa_thermal.h"
#include <linux/cpu_cooling.h>
#include <linux/debugfs.h>
#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/thermal.h>
#include <linux/topology.h>
#include <linux/version.h>
#include <linux/pm_opp.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/kthread.h>
#include <linux/sched/task.h>
#include <uapi/linux/sched/types.h>
#include <securec.h>
#include <linux/of_platform.h>
#ifdef CONFIG_THERMAL_SPM
#include <linux/string.h>
#endif
#include <linux/lpm_thermal.h>
#include <trace/events/thermal_power_allocator.h>
#include <trace/events/ipa_thermal_trace.h>
#include <thermal_core.h>

extern struct mutex cooling_cpufreq_lock;
extern struct list_head cpufreq_cdev_list;
extern struct list_head thermal_cdev_list;
extern unsigned int g_ipa_soc_state[CAPACITY_OF_ARRAY];
extern unsigned int g_ipa_board_state[CAPACITY_OF_ARRAY];

bool thermal_of_get_cdev_type(struct thermal_zone_device *tzd,
			struct thermal_cooling_device *cdev)
{
	struct __thermal_zone *tz = NULL;
	int i, j;
	struct __thermal_bind_params *tbp = NULL;
	struct __thermal_cooling_bind_param *tcbp = NULL;

	if (!tzd)
		return false;

	tz = (struct __thermal_zone *)tzd->devdata;
	if (IS_ERR_OR_NULL(tz))
		return false;

	for (i = 0; i < tz->num_tbps; i++) {
		tbp = tz->tbps + i;
		if (IS_ERR_OR_NULL(tbp)) {
			pr_err("tbp is null\n");
			return false;
		}

		if (IS_ERR_OR_NULL(cdev->np)) {
			pr_err("cdev->np is null\n");
			return false;
		}

		for (j = 0; j < tbp->count; j++) {
			tcbp = tbp->tcbp + j;
			if (IS_ERR_OR_NULL(tcbp->cooling_device)) {
				pr_err("tcbp->cooling_device is null\n");
				return false;
			}

			if (tcbp->cooling_device == cdev->np)
				return tcbp->is_soc_cdev;
		}
	}

	return false;
}
EXPORT_SYMBOL(thermal_of_get_cdev_type);

int power_actor_set_powers(struct thermal_zone_device *tz,
				struct thermal_instance *instance, u32 *soc_sustainable_power,
				u32 granted_power)
{
	unsigned long state = 0;
	int ret;
	int actor_id; // 0:Little, 1:Big, 2:GPU,

	if (!cdev_is_power_actor(instance->cdev))
		return -EINVAL;

	ret = instance->cdev->ops->power2state(instance->cdev, granted_power, &state);
	if (ret)
		return ret;

	if (!strncmp(instance->cdev->type, "thermal-devfreq-0", 17)) {
		actor_id = ipa_get_actor_id("gpu");
	} else if (!strncmp(instance->cdev->type, "thermal-cpufreq-0", 17)) {
		actor_id = ipa_get_actor_id("cluster0");
	} else if (!strncmp(instance->cdev->type, "thermal-cpufreq-1", 17)) {
		actor_id = ipa_get_actor_id("cluster1");
	} else if (!strncmp(instance->cdev->type, "thermal-cpufreq-2", 17)) {
		actor_id = ipa_get_actor_id("cluster2");
	} else
		actor_id = -1;

	if (actor_id < 0) {
		instance->target = state;
	} else if (tz->is_board_thermal) {
		g_ipa_board_state[actor_id] = state;
		instance->target = max(g_ipa_board_state[actor_id], g_ipa_soc_state[actor_id]);
	} else {
		g_ipa_soc_state[actor_id] = state;
		instance->target = max(g_ipa_board_state[actor_id], g_ipa_soc_state[actor_id]);
	}

	instance->cdev->updated = false;
	thermal_cdev_update(instance->cdev);

	return 0;
}

int thermal_zone_cdev_get_power(const char *thermal_zone_name, const char *cdev_name, unsigned int *power)
{
	struct thermal_instance *instance = NULL;
	int ret = 0;
	struct thermal_zone_device *tz = NULL;
	int find = 0;

	tz = thermal_zone_get_zone_by_name(thermal_zone_name);
	if (IS_ERR(tz)) {
		return -ENODEV;
	}

	mutex_lock(&tz->lock);
	list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
		struct thermal_cooling_device *cdev = instance->cdev;

		if (!cdev_is_power_actor(cdev))
			continue;

		if (strncasecmp(instance->cdev->type, cdev_name, THERMAL_NAME_LENGTH))
			continue;

		if (!cdev->ops->get_requested_power(cdev, power)) {
			find = 1;
			break;
		}
	}

	if(!find)
		ret = -ENODEV;

	mutex_unlock(&tz->lock);
	return ret;
}
EXPORT_SYMBOL(thermal_zone_cdev_get_power);

#define FRAC_BITS 10
#define int_to_frac(x) ((x) << FRAC_BITS)

static u32 __estimate_sustainable_power(struct thermal_zone_device *tz)
{
	u32 sustainable_power = 0;
	struct thermal_instance *instance = NULL;
	struct power_allocator_params *params = tz->governor_data;

	list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
		struct thermal_cooling_device *cdev = instance->cdev;
		u32 min_power;
		if (instance->trip != params->trip_max_desired_temperature)
			continue;
		if (power_actor_get_min_power(cdev, &min_power))
			continue;
		sustainable_power += min_power;
	}
	return sustainable_power;
}

static void __estimate_pid_constants(struct thermal_zone_device *tz,
				     u32 sustainable_power, int trip_switch_on,
				     int control_temp, bool force)
{
	int ret;
	int switch_on_temp;
	u32 temperature_threshold;
	ret = tz->ops->get_trip_temp(tz, trip_switch_on, &switch_on_temp);
	if (ret)
		switch_on_temp = 0;

	temperature_threshold = control_temp - switch_on_temp;
	if (!temperature_threshold)
		return;

	if (!tz->tzp->k_po || force)
		tz->tzp->k_po = int_to_frac(sustainable_power) /
			temperature_threshold;

	if (!tz->tzp->k_pu || force)
		tz->tzp->k_pu = int_to_frac(2 * sustainable_power) /
			temperature_threshold;

	if (!tz->tzp->k_i || force)
		tz->tzp->k_i = int_to_frac(10) / 1000;
}

void update_pid_value(struct thermal_zone_device *tz)
{
	int ret;
	int control_temp;
	u32 sustainable_power = 0;
	struct power_allocator_params *params = NULL;

	if (tz->governor_data == NULL)
		return;
	params = tz->governor_data;
	mutex_lock(&tz->lock);
	if (!strncmp(tz->governor->name, "power_allocator", strlen("power_allocator"))) {
		ret = tz->ops->get_trip_temp(tz, params->trip_max_desired_temperature,
					     &control_temp);
		if (ret) {
			dev_dbg(&tz->device,
			"Update PID failed to get control_temp: %d\n", ret);
		} else {
			if (tz->tzp->sustainable_power) {
				sustainable_power = tz->tzp->sustainable_power;
			} else {
				sustainable_power = __estimate_sustainable_power(tz);
			}
			if (tz->is_soc_thermal)
				__estimate_pid_constants(tz, sustainable_power,
					params->trip_switch_on,
					control_temp, (bool)true);
		}
	}
	mutex_unlock(&tz->lock);
}

void set_tz_type(struct thermal_zone_device *tz)
{
	if (!strncmp(tz->type, BOARD_THERMAL_NAME, sizeof(BOARD_THERMAL_NAME) - 1))
		tz->is_board_thermal = true;
	else if (!strncmp(tz->type, SOC_THERMAL_NAME, sizeof(SOC_THERMAL_NAME) - 1))
		tz->is_soc_thermal = true;
}

#define MAX_TEMP 145000
#define MIN_TEMP -40000
s32 thermal_zone_temp_check(s32 temperature)
{
	if (temperature > MAX_TEMP) {
		temperature = MAX_TEMP;
	} else if (temperature < MIN_TEMP) {
		temperature = MIN_TEMP;
	}
	return temperature;
}

#ifdef CONFIG_THERMAL_SPM
#define HZ_PER_KHZ	1000
bool get_spm_target_freq(int actor, unsigned int *target_freq)
{
	unsigned int freq, min_freq;

	if (is_spm_mode_enabled()) {
		freq = get_powerhal_profile(actor);
		min_freq = get_minfreq_profile(actor);

		if (*target_freq >= freq)
			*target_freq = freq;
		if (*target_freq <= min_freq)
			*target_freq = min_freq;

		return true;
	}
	return false;
}

int gpufreq_update_policies(void)
{
	struct thermal_cooling_device *cdev = NULL;
	struct devfreq_cooling_device *dfc = NULL;
	unsigned int min_freq = 0;
	unsigned int freq = 0;
	int actor, ret;

	list_for_each_entry(cdev, &thermal_cdev_list, node) {
		if (cdev->np == NULL || cdev->np->name == NULL)
			continue;
		if (!strncmp(cdev->np->name, "gpufreq", strlen("gpufreq"))) {
			dfc = cdev->devdata;
			if (is_spm_mode_enabled()) {
				actor = ipa_get_actor_id("gpu");
				freq = get_powerhal_profile(actor);
				min_freq = get_minfreq_profile(actor);
			} else {
				freq = dfc->freq_table[0];
				min_freq = dfc->freq_table[dfc->freq_table_size - 1];
			}

			ret = dev_pm_qos_update_request(&dfc->devfreq->user_min_freq_req,
							DIV_ROUND_UP(min_freq, HZ_PER_KHZ));

			ret |= dev_pm_qos_update_request(&dfc->devfreq->user_max_freq_req,
							 DIV_ROUND_UP(freq, HZ_PER_KHZ));

			ret |= dev_pm_qos_update_request(&dfc->req_max_freq,
							 DIV_ROUND_UP(freq, HZ_PER_KHZ));
			if (ret < 0)
				pr_err("update gpu freq fail %d\n", ret);
		}
	}

	return 0;
}
EXPORT_SYMBOL(gpufreq_update_policies);

int cpufreq_update_policies(void)
{
	struct cpufreq_cooling_device *cpufreq_cdev = NULL;
	unsigned int cpu = 0;
	int actor;
	unsigned int min_freq = 0;
	unsigned int freq = 0;
	struct mask_id_t mask_id;
	int cpuclustercnt;
	int oncpucluster[MAX_THERMAL_CLUSTER_NUM] = {0};

	ipa_get_clustermask(mask_id.clustermask, MAX_THERMAL_CLUSTER_NUM, oncpucluster, &cpuclustercnt);
	mutex_lock(&cooling_cpufreq_lock);
	list_for_each_entry(cpufreq_cdev, &cpufreq_cdev_list, node) {
		cpu = cpumask_any(cpufreq_cdev->policy->related_cpus);
		if (is_spm_mode_enabled()) {
			actor = topology_physical_package_id(cpu);
			freq = get_powerhal_profile(mask_id.clustermask[actor]);
			min_freq = get_minfreq_profile(mask_id.clustermask[actor]);
		} else {
			freq = get_state_freq(cpufreq_cdev, 0);
			min_freq = get_state_freq(cpufreq_cdev, cpufreq_cdev->max_level);
		}

		freq_qos_update_request(cpufreq_cdev->policy->max_freq_req, freq);
		freq_qos_update_request(cpufreq_cdev->policy->min_freq_req, min_freq);
		freq_qos_update_request(&cpufreq_cdev->qos_req, freq);
	}
	mutex_unlock(&cooling_cpufreq_lock);

	return 0;
}
EXPORT_SYMBOL(cpufreq_update_policies);
#endif

void trip_attrs_store(struct thermal_zone_device *tz, int mask, int indx)
{
#ifdef CONFIG_THERMAL_TSENSOR
	if (IS_ENABLED(CONFIG_THERMAL_WRITABLE_TRIPS) &&
	    mask & (1 << indx)) {
		if (tz->ops->activate_trip_type) {
			tz->trip_type_attrs[indx].attr.attr.mode |= S_IWUSR;
			tz->trip_type_attrs[indx].attr.store =
				trip_point_type_activate;
		}
	}
#endif
}

void update_debugfs(struct ipa_sensor *sensor_data)
{
#ifdef CONFIG_DFX_DEBUG_FS
	struct dentry *filter_d = NULL;
	char filter_name[25];
	int rc;

	rc = snprintf_s(filter_name, sizeof(filter_name), (sizeof(filter_name) - 1),
			"thermal_lpf_filter%d", sensor_data->sensor_id);
	if (rc < 0) {
		pr_err("snprintf_s error.\n");
		return;
	}
	filter_d = debugfs_create_dir(filter_name, NULL);
	if (IS_ERR_OR_NULL(filter_d)) {
		pr_warn("unable to create debugfs directory for the LPF filter\n");
		return;
	}

	debugfs_create_u32("alpha", S_IWUSR | S_IRUGO, filter_d,
			   (u32 *)&sensor_data->alpha);
	return;
#endif
}
