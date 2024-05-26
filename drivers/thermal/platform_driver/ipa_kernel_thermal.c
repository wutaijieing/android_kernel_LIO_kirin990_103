#include <linux/lpm_thermal.h>
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
#include <linux/string.h>
#include <trace/events/thermal.h>
#include <trace/events/thermal_power_allocator.h>
#include <trace/events/ipa_thermal_trace.h>
#include <thermal_core.h>
#include <ipa_thermal.h>
#ifdef CONFIG_ITS_IPA
#include <linux/platform_drivers/dpm_hwmon_user.h>
#endif
#ifdef CONFIG_FREQ_LIMIT_COUNTER
#include <linux/platform_drivers/perfhub.h>
#endif

DEFINE_MUTEX(thermal_boost_lock);
#define MODE_LEN 10
int g_ipa_freq_limit_debug = 0;

unsigned int g_ipa_freq_limit[CAPACITY_OF_ARRAY] = {
	IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX,
	IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX
};
unsigned int g_ipa_soc_freq_limit[CAPACITY_OF_ARRAY] = {
	IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX,
	IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX
};
unsigned int g_ipa_board_freq_limit[CAPACITY_OF_ARRAY] = {
	IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX,
	IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX, IPA_FREQ_MAX
};
unsigned int g_ipa_soc_state[CAPACITY_OF_ARRAY] = {0};
unsigned int g_ipa_board_state[CAPACITY_OF_ARRAY] = {0};

unsigned int g_ipa_gpu_boost_weights[CAPACITY_OF_ARRAY] = {0};
unsigned int g_ipa_normal_weights[CAPACITY_OF_ARRAY] = {0};


bool thermal_of_get_cdev_type(struct thermal_zone_device *tzd,
			struct thermal_cooling_device *cdev);

void ipa_freq_debug(int onoff)
{
	g_ipa_freq_limit_debug = onoff;
}

void ipa_freq_limit_init(void)
{
	int i;
	for (i = 0; i < (int)g_ipa_actor_num; i++)
		g_ipa_freq_limit[i] = IPA_FREQ_MAX;
}

void ipa_freq_limit_reset(struct thermal_zone_device *tz)
{
	int i;

	struct thermal_instance *instance = NULL;

	if (tz->is_board_thermal) {
		for (i = 0; i < (int)g_ipa_actor_num; i++) {
			g_ipa_board_freq_limit[i] = IPA_FREQ_MAX;
			g_ipa_board_state[i] = 0;
		}
	} else {
		for (i = 0; i < (int)g_ipa_actor_num; i++) {
			g_ipa_soc_freq_limit[i] = IPA_FREQ_MAX;
			g_ipa_soc_state[i] = 0;
		}
	}

	for (i = 0; i < (int)g_ipa_actor_num; i++) {
		if (g_ipa_soc_freq_limit[i] != 0 && g_ipa_board_freq_limit[i] != 0)
			g_ipa_freq_limit[i] = min(g_ipa_soc_freq_limit[i], g_ipa_board_freq_limit[i]);
		else if (g_ipa_soc_freq_limit[i] == 0)
			g_ipa_freq_limit[i] = g_ipa_board_freq_limit[i];
		else if (g_ipa_board_freq_limit[i] == 0)
			g_ipa_freq_limit[i] = g_ipa_soc_freq_limit[i];
		else
			g_ipa_freq_limit[i] = IPA_FREQ_MAX;
	}

	list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
		instance->target = 0;
		instance->cdev->ops->set_cur_state(instance->cdev, 0UL);
		mutex_lock(&instance->cdev->lock);
		instance->cdev->updated = true;
		mutex_unlock(&instance->cdev->lock);
	}
}

unsigned int ipa_freq_limit(int actor, unsigned int target_freq)
{
	int i = 0;

	if (actor >= (int)g_ipa_actor_num)
		return target_freq;

	if (g_ipa_freq_limit_debug) {
		pr_err("actor[%d]target_freq[%u]IPA:", actor, target_freq);
		for (i = 0; i < (int)g_ipa_actor_num; i++)
			pr_err("[%u]", g_ipa_freq_limit[i]);
		pr_err("min[%u]\n", min(target_freq, g_ipa_freq_limit[actor]));
	}

#ifdef CONFIG_THERMAL_SPM
	if (get_spm_target_freq(actor, &target_freq))
		return target_freq;
#endif

#ifdef CONFIG_FREQ_LIMIT_COUNTER
	if (actor == ipa_get_actor_id("gpu"))
		update_gpu_freq_limit_counter(target_freq, g_ipa_freq_limit[actor]);
#endif
	return min(target_freq, g_ipa_freq_limit[actor]);
}
EXPORT_SYMBOL(ipa_freq_limit);

#ifdef CONFIG_PERF_CTRL
int get_ipa_status(struct ipa_stat *status)
{
	status->cluster0 = g_ipa_freq_limit[0];
	status->cluster1 = g_ipa_freq_limit[1];
	if (ipa_get_actor_id("gpu") == 3) {
		status->cluster2 = g_ipa_freq_limit[2];
		status->gpu = g_ipa_freq_limit[3];
	} else {
		status->cluster2 = 0;
		status->gpu = g_ipa_freq_limit[2];
	}

	return 0;
}
#endif

int nametoactor(const char *weight_attr_name)
{
	int actor_id = -1;

	if (!strncmp(weight_attr_name, IPA_GPU_WEIGHT_NAME, sizeof(IPA_GPU_WEIGHT_NAME) - 1))
			actor_id = ipa_get_actor_id("gpu");
	else if (!strncmp(weight_attr_name, IPA_CLUSTER0_WEIGHT_NAME, sizeof(IPA_CLUSTER0_WEIGHT_NAME) - 1))
			actor_id = ipa_get_actor_id("cluster0");
	else if (!strncmp(weight_attr_name, IPA_CLUSTER1_WEIGHT_NAME, sizeof(IPA_CLUSTER1_WEIGHT_NAME) - 1))
			actor_id = ipa_get_actor_id("cluster1");
	else if (!strncmp(weight_attr_name, IPA_CLUSTER2_WEIGHT_NAME, sizeof(IPA_CLUSTER2_WEIGHT_NAME) - 1))
			actor_id = ipa_get_actor_id("cluster2");
	else
		actor_id = -1;

	return actor_id;
}

void restore_actor_weights(struct thermal_zone_device *tz)
{
	struct thermal_instance *pos = NULL;
	int actor_id = -1;

	list_for_each_entry(pos, &tz->thermal_instances, tz_node) {
		actor_id = nametoactor(pos->weight_attr_name);
		if (actor_id != -1)
			pos->weight = g_ipa_normal_weights[actor_id];
	}
}
EXPORT_SYMBOL_GPL(restore_actor_weights);

void update_actor_weights(struct thermal_zone_device *tz)
{
	struct thermal_instance *pos = NULL;
	bool b_gpu_bounded = false;
	struct thermal_cooling_device *cdev = NULL;
	int actor_id = -1;

	list_for_each_entry(pos, &tz->thermal_instances, tz_node) {
		cdev = pos->cdev;
		if (cdev == NULL)
			return;

		if (!strncmp(pos->weight_attr_name, IPA_GPU_WEIGHT_NAME, sizeof(IPA_GPU_WEIGHT_NAME) - 1) && cdev->bound_event) {
			b_gpu_bounded = true;
			break;
		}
	}

	list_for_each_entry(pos, &tz->thermal_instances, tz_node) {
		actor_id = nametoactor(pos->weight_attr_name);

		if (b_gpu_bounded) {
			if (actor_id != -1)
				pos->weight = g_ipa_gpu_boost_weights[actor_id];
		} else {
			if (actor_id != -1)
				pos->weight = g_ipa_normal_weights[actor_id];
		}
	}
}
EXPORT_SYMBOL_GPL(update_actor_weights);

ssize_t
boost_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	enum thermal_boost_mode mode;

	if (!tz->tzp)
		return -EIO;

	mutex_lock(&tz->lock);
	mode = tz->tzp->boost;
	mutex_unlock(&tz->lock);
	return snprintf_s(buf, MODE_LEN, MODE_LEN - 1, "%s\n",
			  mode == THERMAL_BOOST_ENABLED ? "enabled" : "disabled");
}
EXPORT_SYMBOL_GPL(boost_show);

ssize_t
boost_store(struct device *dev, struct device_attribute *attr,
	   const char *buf, size_t count)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	int result = 0;

	if (!tz->tzp)
		return -EIO;

	mutex_lock(&thermal_boost_lock);

	if (!strncmp(buf, "enabled", sizeof("enabled") - 1)) {
		mutex_lock(&tz->lock);
		if (THERMAL_BOOST_ENABLED == tz->tzp->boost)
			pr_info("perfhub boost high frequency timeout[%d]\n",
				tz->tzp->boost_timeout);

		tz->tzp->boost = THERMAL_BOOST_ENABLED;
		if (0 == tz->tzp->boost_timeout)
			tz->tzp->boost_timeout = 100;

		thermal_zone_device_set_polling(tz, tz->tzp->boost_timeout);
		ipa_freq_limit_reset(tz);
		mutex_unlock(&tz->lock);
	} else if (!strncmp(buf, "disabled", sizeof("disabled") - 1)) {
		tz->tzp->boost = THERMAL_BOOST_DISABLED;
		thermal_zone_device_update(tz, THERMAL_EVENT_UNSPECIFIED);
	} else {
		result = -EINVAL;
	}

	mutex_unlock(&thermal_boost_lock);

	if (result)
		return result;

	trace_IPA_boost(current->pid, tz, tz->tzp->boost,
			tz->tzp->boost_timeout);

	return count;
}
EXPORT_SYMBOL_GPL(boost_store);

ssize_t
boost_timeout_show(struct device *dev, struct device_attribute *devattr,
			char *buf)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);

	if (tz->tzp)
		return snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1,
				  "%u\n", tz->tzp->boost_timeout);
	else
		return -EIO;
}
EXPORT_SYMBOL_GPL(boost_timeout_show);

#define BOOST_TIMEOUT_THRES	5000
ssize_t
boost_timeout_store(struct device *dev, struct device_attribute *devattr,
			const char *buf, size_t count)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	u32 boost_timeout;

	if (!tz->tzp)
		return -EIO;

	if (kstrtou32(buf, 10, &boost_timeout))
		return -EINVAL;

	if (boost_timeout > BOOST_TIMEOUT_THRES)
		return -EINVAL;

	if (boost_timeout)
		tz->tzp->boost_timeout = boost_timeout;

	trace_IPA_boost(current->pid, tz, tz->tzp->boost,
			tz->tzp->boost_timeout);
	return count;
}
EXPORT_SYMBOL_GPL(boost_timeout_store);

ssize_t cur_power_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct thermal_zone_device *tz = NULL;
	struct thermal_instance *instance = NULL;
	ssize_t buf_len;
	ssize_t size = PAGE_SIZE;
	if (dev == NULL || attr == NULL || buf == NULL)
		return -EIO;

	tz = to_thermal_zone(dev);
	if (tz == NULL || tz->tzp == NULL)
		return -EIO;

	mutex_lock(&tz->lock);
	buf_len = snprintf_s(buf, size, size - 1, "%llu,%u,", tz->tzp->cur_ipa_total_power, tz->tzp->check_cnt);
	if (buf_len < 0)
		goto unlock;
	list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
		struct thermal_cooling_device *cdev = instance->cdev;
		ssize_t ret;
		size = PAGE_SIZE - buf_len;
		if (size < 0 || size > (ssize_t)PAGE_SIZE)
			break;
		ret = snprintf_s(buf + buf_len, size, size - 1, "%llu,", cdev->cdev_cur_power);
		if (ret < 0)
			break;
		buf_len += ret;
	}
unlock:
	mutex_unlock(&tz->lock);

	if (buf_len > 0)
		buf[buf_len-1] = '\0'; /* set string end with '\0' */
	return buf_len;
}
EXPORT_SYMBOL_GPL(cur_power_show);

ssize_t cur_enable_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);

	if (tz != NULL && tz->tzp != NULL)
		return snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "%u\n", tz->tzp->cur_enable);
	else
		return -EIO;
}
EXPORT_SYMBOL_GPL(cur_enable_show);

ssize_t cur_enable_store(struct device *dev, struct device_attribute *devattr,
	const char *buf, size_t count)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	u32 cur_enable;

	if (tz == NULL || tz->tzp == NULL)
		return -EIO;

	if (kstrtou32(buf, 10, &cur_enable))
		return -EINVAL;

	tz->tzp->cur_enable = (cur_enable > 0);

	return count;
}
EXPORT_SYMBOL_GPL(cur_enable_store);

void get_cur_power(struct thermal_zone_device *tz)
{
	struct thermal_instance *instance = NULL;
	struct power_allocator_params *params = NULL;
	u32 total_req_power;
	int trip_max_temp;

	if (tz == NULL || tz->tzp == NULL)
		return;

	if (0 == tz->tzp->cur_enable)
		return;

	mutex_lock(&tz->lock);
	if (tz->governor_data == NULL) {
		mutex_unlock(&tz->lock);
		return;
	}

	params = tz->governor_data;
	trip_max_temp = params->trip_max_desired_temperature;
	total_req_power = 0;

	list_for_each_entry(instance, &tz->thermal_instances, tz_node) {
		struct thermal_cooling_device *cdev = instance->cdev;
		u32 req_power;

		if (instance->trip != trip_max_temp)
			continue;

		if (!cdev_is_power_actor(cdev))
			continue;

		if (cdev->ops->get_requested_power(cdev, &req_power))
			continue;

		total_req_power += req_power;
		cdev->cdev_cur_power += req_power;
	}

	tz->tzp->cur_ipa_total_power += total_req_power;
	tz->tzp->check_cnt ++;

	mutex_unlock(&tz->lock);
}
EXPORT_SYMBOL_GPL(get_cur_power);

int update_freq_table(struct cpufreq_cooling_device *cpufreq_cdev,
		      u32 capacitance)
{
	struct freq_table *freq_table = NULL;
	struct dev_pm_opp *opp = NULL;
	struct device *dev = NULL;
	int num_opps = 0;
	int cpu, i;

	freq_table = cpufreq_cdev->freq_table;
	if (IS_ERR_OR_NULL(cpufreq_cdev->policy))
		pr_err("%s:%d:cpufreq_cdev policy is NULL or ERR.\n", __func__, __LINE__);
	cpu = cpufreq_cdev->policy->cpu;

	dev = get_cpu_device(cpu);
	if (unlikely(!dev)) {
		pr_warn("No cpu device for cpu %d\n", cpu);
		return -ENODEV;
	}

	num_opps = dev_pm_opp_get_opp_count(dev);
	if (num_opps < 0)
		return num_opps;

	/*
	 * The cpufreq table is also built from the OPP table and so the count
	 * should match.
	 */
	if (num_opps != cpufreq_cdev->max_level + 1) {
		dev_warn(dev, "Number of OPPs not matching with max_levels\n");
		return -EINVAL;
	}

	for (i = 0; i <= cpufreq_cdev->max_level; i++) {
		unsigned long freq = freq_table[i].frequency * 1000;
		u32 freq_mhz = freq_table[i].frequency / 1000;
		u64 power;
		u32 voltage_mv;

		/*
		 * Find ceil frequency as 'freq' may be slightly lower than OPP
		 * freq due to truncation while converting to kHz.
		 */
		opp = dev_pm_opp_find_freq_ceil(dev, &freq);
		if (IS_ERR(opp)) {
			dev_err(dev, "failed to get opp for %lu frequency\n",
				freq);
			return -EINVAL;
		}

		voltage_mv = dev_pm_opp_get_voltage(opp) / 1000;
		dev_pm_opp_put(opp);

		/*
		 * Do the multiplication with MHz and millivolt so as
		 * to not overflow.
		 */
		power = (u64)capacitance * freq_mhz * voltage_mv * voltage_mv;
		do_div(power, 1000000000);

		/* power is stored in mW */
		freq_table[i].power = power;
		pr_err("  %u MHz @ %u mV :  %u  mW\n", freq_mhz, voltage_mv, freq_table[i].power);
	}
	return 0;
}

void cpu_cooling_get_limit_state(struct cpufreq_cooling_device *cpufreq_cdev,
				 unsigned long state)
{
	unsigned int cpu = cpumask_any(cpufreq_cdev->policy->related_cpus);
	unsigned int cur_cluster, temp;
	unsigned long limit_state;
	int ret;
	struct mask_id_t mask_id;
	int cpuclustercnt;
	int oncpucluster[MAX_THERMAL_CLUSTER_NUM] = {0};
	unsigned int freq;
#ifdef CONFIG_THERMAL_SPM
	unsigned int actor;
#endif

	if (policy_is_inactive(cpufreq_cdev->policy))
		return;

	ipa_get_clustermask(mask_id.clustermask, MAX_THERMAL_CLUSTER_NUM, oncpucluster, &cpuclustercnt);
	temp = (unsigned int)topology_physical_package_id(cpu);
	cur_cluster = mask_id.clustermask[temp];

	if (g_ipa_soc_state[cur_cluster] <= cpufreq_cdev->max_level)
		g_ipa_soc_freq_limit[cur_cluster] = cpufreq_cdev->freq_table[g_ipa_soc_state[cur_cluster]].frequency;

	if (g_ipa_board_state[cur_cluster] <= cpufreq_cdev->max_level)
		g_ipa_board_freq_limit[cur_cluster] = cpufreq_cdev->freq_table[g_ipa_board_state[cur_cluster]].frequency;

	limit_state = max(g_ipa_soc_state[cur_cluster], g_ipa_board_state[cur_cluster]);
	state = max(state, limit_state);
	/* Check if the old cooling action is same as new cooling action */
	if (cpufreq_cdev->cpufreq_state == state)
		return;

#ifdef CONFIG_THERMAL_SPM
	if (is_spm_mode_enabled()) {
		actor = topology_physical_package_id(cpu);
		freq = get_powerhal_profile(mask_id.clustermask[actor]);
	} else {
		freq = get_state_freq(cpufreq_cdev, state);
		g_ipa_freq_limit[cur_cluster] = freq;
	}
#else
	freq = get_state_freq(cpufreq_cdev, state);
	g_ipa_freq_limit[cur_cluster] = freq;
#endif
	ret = freq_qos_update_request(&cpufreq_cdev->qos_req, freq);
	if (ret > 0) {
		cpufreq_cdev->cpufreq_state = state;
		ret = freq_qos_update_request(&cpufreq_cdev->qos_req, freq);
		pr_debug("ipa_cpufreq qos request try again, ret = %d.freq:%d.", ret, freq);
	}
#ifdef CONFIG_FREQ_LIMIT_COUNTER
	update_cpu_ipa_max_freq(cpufreq_cdev->policy, freq);
#endif
}

unsigned long get_level(struct cpufreq_cooling_device *cpufreq_cdev,
			unsigned int freq)
{
	struct freq_table *freq_table = cpufreq_cdev->freq_table;
	unsigned long level;

	for (level = 1; level <= cpufreq_cdev->max_level; level++)
		if (freq > freq_table[level].frequency)
			break;

	return level - 1;
}

static unsigned long gpu_get_voltage(struct devfreq *df, unsigned long freq)
{
	struct device *dev = df->dev.parent;
	unsigned long voltage;
	struct dev_pm_opp *opp = NULL;

	opp = dev_pm_opp_find_freq_exact(dev, freq, true);
	if (PTR_ERR(opp) == -ERANGE)
	opp = dev_pm_opp_find_freq_exact(dev, freq, false);

	if (IS_ERR(opp)) {
		dev_err_ratelimited(dev, "Failed to find OPP for frequency %lu: %ld\n",
				    freq, PTR_ERR(opp));
		return 0;
	}

	voltage = dev_pm_opp_get_voltage(opp) / 1000; /* mV */
	dev_pm_opp_put(opp);

	if (voltage == 0)
		dev_err_ratelimited(dev,
				    "Failed to get voltage for frequency %lu\n",
				    freq);

	return voltage;
}

static unsigned long
gpu_get_static_power(struct devfreq_cooling_device *dfc, unsigned long freq)
{
	struct devfreq *df = dfc->devfreq;
	unsigned long voltage;

	if (!dfc->power_ops->get_static_power)
		return 0;

	voltage = gpu_get_voltage(df, freq);

	if (voltage == 0)
		return 0;

	return dfc->power_ops->get_static_power(df, voltage);
}
#ifdef CONFIG_ITS_IPA
int devfreq_normalized_data_to_freq(struct devfreq_cooling_device *devfreq_cdev,
				    unsigned long power);

struct power_status {
	unsigned long long leakage;
	unsigned long busy_time;
	s32 dyn_power;
	u32 static_power;
	s32 est_power;
	bool its_dpm_enabled;
};
void gpu_power_to_state(struct thermal_cooling_device *cdev,
			u32 power, unsigned long *state)
{
	struct devfreq_cooling_device *dfc = cdev->devdata;
	struct devfreq *df = dfc->devfreq;
	struct devfreq_dev_status *status = &df->last_status;
	struct power_status powers = {0, 0, 0, 0, 0, false};
	int ret, i;

		powers.its_dpm_enabled = check_its_enabled() &&
				  check_dpm_enabled(DPM_GPU_MODULE);
		if (strncmp(cdev->type, "thermal-devfreq-0",
			    strlen("thermal-devfreq-0")) == 0 &&
		    powers.its_dpm_enabled) {
			ret = get_gpu_leakage_power(&powers.leakage);
			if (ret != 0)
				powers.static_power = 0;
			else
				powers.static_power = (u32)powers.leakage;

			powers.dyn_power = power - powers.static_power;
			powers.dyn_power = powers.dyn_power > 0 ? powers.dyn_power : 0;
			*state = devfreq_normalized_data_to_freq(dfc, powers.dyn_power);
		}  else {
			powers.static_power = gpu_get_static_power(dfc, status->current_frequency);
			powers.dyn_power = power - powers.static_power;
			powers.dyn_power = powers.dyn_power > 0 ? powers.dyn_power : 0;

			/* Scale dynamic power for utilization */
			powers.busy_time = status->busy_time ?: 1;
			powers.est_power = (powers.dyn_power * status->total_time) / powers.busy_time;

			/*
			 * Find the first cooling state that is within the power
			 * budget for dynamic power.
			 */
			for (i = 0; i < dfc->freq_table_size - 1; i++)
				if (powers.est_power >= dfc->power_table[i])
					break;

			*state = i;
		}
}

u32 cpu_get_dynamic_power(struct cpufreq_cooling_device *cpufreq_cdev)
{
	struct cpufreq_policy *policy = cpufreq_cdev->policy;
	int cpu, ret;
	unsigned long long temp_power = 0;
	unsigned long long total_power = 0;

	for_each_cpu(cpu, policy->related_cpus) {
		ret = get_its_core_dynamic_power(cpu, &temp_power);
		if (ret == 0)
			total_power += temp_power;
	}
	return (u32)total_power;
}
#endif

int cpu_get_static_power(struct cpufreq_cooling_device *cpufreq_cdev,
			    unsigned long freq, u32 *power)
{
	struct dev_pm_opp *opp = NULL;
	unsigned long voltage;
	struct cpufreq_policy *policy = cpufreq_cdev->policy;
	struct cpumask *cpumask = policy->related_cpus;
	unsigned long freq_hz = freq * 1000;
	struct device *dev = NULL;

#ifdef CONFIG_ITS_IPA
	int cpu, ret;
	unsigned long long temp_power = 0;

	if (check_its_enabled()) {
		*power = 0;
		for_each_cpu(cpu, policy->related_cpus) { /*lint !e574*/
			ret = get_its_core_leakage_power(cpu, &temp_power);
			if (ret == 0)
				*power += temp_power;
		}
		return 0;
	}
#endif

	dev = get_cpu_device(policy->cpu);
		WARN_ON(!dev); /*lint !e146 !e665*/

#ifdef CONFIG_IPA_THERMAL
	if (dev == NULL) {
		*power = 0;
		return 0;
	}

	device_lock(dev);
	if (dev->offline == true) {
		*power = 0;
		device_unlock(dev);
		return 0;
	}
	device_unlock(dev);
#endif
	opp = dev_pm_opp_find_freq_exact(dev, freq_hz, true);
	if (IS_ERR(opp)) {
		dev_warn_ratelimited(dev, "Failed to find OPP for frequency %lu: %ld\n",
				     freq_hz, PTR_ERR(opp));
		return -EINVAL;
	}

	voltage = dev_pm_opp_get_voltage(opp);
	dev_pm_opp_put(opp);
	if (voltage == 0) {
		dev_err_ratelimited(dev, "Failed to get voltage for frequency %lu\n",
				    freq_hz);
		return -EINVAL;
	}

	return cluster_get_static_power(cpumask, voltage, power);
}

#ifdef CONFIG_ITS_IPA
unsigned long cpu_get_voltage(struct cpufreq_cooling_device *cpufreq_cdev,
			      unsigned long freq)
{
	struct device *dev = NULL;
	struct dev_pm_opp *opp = NULL;
	struct cpufreq_policy *policy = cpufreq_cdev->policy;
	unsigned long freq_hz = freq * 1000;
	unsigned long voltage;

	dev = get_cpu_device(policy->cpu);

	if (dev == NULL)
		return 0;

	if (dev->offline == true)
		return 0;

	opp = dev_pm_opp_find_freq_exact(dev, freq_hz, true);

	if (IS_ERR(opp)) {
		dev_warn_ratelimited(dev,
				"Failed to find OPP for frequency %lu:%ld\n",
				freq_hz, PTR_ERR(opp));
		return 0;
	}

	voltage = dev_pm_opp_get_voltage(opp);
	dev_pm_opp_put(opp);
	if (voltage == 0) {
		dev_err_ratelimited(dev,
				"Failed to get voltage for frequency %lu\n",
				freq_hz);
		return 0;
	}
	return voltage;
}

unsigned long normalize_its_power(struct cpufreq_cooling_device *cpufreq_cdev,
				  unsigned int dynamic_power, unsigned long freq)
{
	unsigned long voltage, freq_mhz;
	unsigned long long normalized_data = 0;

	/* get_voltage() will get uV, so div 100 for mV */
	voltage = cpu_get_voltage(cpufreq_cdev, freq) / 1000;
	freq_mhz = freq / 1000;

	if (freq_mhz != 0 && voltage != 0)
		normalized_data = ITS_NORMALIZED_RATIO * dynamic_power /
					(voltage * voltage * freq_mhz);

	trace_IPA_actor_cpu_normalize_power(normalized_data, dynamic_power,
					    freq_mhz, voltage);
	return (unsigned long)normalized_data;
}

unsigned int
its_normalized_data_to_freq(struct cpufreq_cooling_device *cpufreq_cdev,
			    unsigned long power)
{
	struct freq_table *freq_table = cpufreq_cdev->freq_table;
	struct cpufreq_policy *policy = cpufreq_cdev->policy;
	unsigned long long calc_power = 0;
	unsigned int freq, freq_mhz, voltage, level;
	unsigned int ncpus = cpumask_weight(policy->related_cpus);

	for (level = 0; level < cpufreq_cdev->max_level; level++) {
		freq = freq_table[level].frequency;
		freq_mhz = freq / 1000;
		voltage = cpu_get_voltage(cpufreq_cdev, freq) / 1000;
		if (freq != 0)
			calc_power = ((unsigned long long)
				       cpufreq_cdev->normalized_powerdata *
				       freq_mhz * voltage * voltage) /
				       ITS_NORMALIZED_RATIO;

		trace_IPA_actor_cpu_pdata_to_freq(level, freq_mhz,
					voltage, calc_power,
					freq_table[level].power * ncpus,
					cpufreq_cdev->normalized_powerdata);
		if ((unsigned long long)power >= calc_power ||
		    power >= freq_table[level].power * ncpus)
			break;
	}
	return freq_table[level].frequency;
};

unsigned long normalize_devfreq_power(unsigned long voltage, unsigned int dynamic_power,
				      unsigned long freq)
{
	unsigned long normalized_data = 0;
	unsigned long freq_mhz;

	/* freq is hz, div 1000000 for mHz */
	freq_mhz = freq / HZ_TO_MHZ_DIVISOR;

	if (freq != 0 && voltage != 0)
		normalized_data = ITS_NORMALIZED_RATIO * dynamic_power /
				  (voltage * voltage * freq_mhz);

	trace_IPA_actor_gpu_normalize_power(normalized_data, dynamic_power,
					    freq_mhz, voltage);
	return normalized_data;
}

int devfreq_normalized_data_to_freq(struct devfreq_cooling_device *devfreq_cdev,
				    unsigned long power)
{
	struct devfreq *df = devfreq_cdev->devfreq;
	unsigned long state, freq, voltage, freq_mhz;
	unsigned long long calc_power = 0;

	for (state = 0; state < devfreq_cdev->freq_table_size - 1; state++) {
		freq = devfreq_cdev->freq_table[state];
		freq_mhz = freq / HZ_TO_MHZ_DIVISOR;
		voltage = gpu_get_voltage(df, freq);
		if (freq != 0)
			calc_power = ((unsigned long long)
				      devfreq_cdev->normalized_powerdata *
				      freq_mhz * voltage * voltage) /
				      ITS_NORMALIZED_RATIO;

		trace_IPA_actor_gpu_pdata_to_freq(state, freq_mhz, voltage,
				calc_power, devfreq_cdev->power_table[state],
				devfreq_cdev->normalized_powerdata);
		if ((unsigned long long)power >= calc_power ||
		    power >= devfreq_cdev->power_table[state])
			break;
	}
	return state;
};

#endif
int cpu_power_to_state(struct thermal_cooling_device *cdev, u32 power,
		       unsigned long *state)
{
	unsigned int cur_freq, target_freq;
	int ret, dyn_power;
	u32 last_load, normalised_power, static_power;
	struct cpufreq_cooling_device *cpufreq_cdev = cdev->devdata;
	struct cpufreq_policy *policy = cpufreq_cdev->policy;
	int i = 0;

	cur_freq = cpufreq_quick_get(policy->cpu);
	if (!cur_freq)
		return -EINVAL;
	ret = cpu_get_static_power(cpufreq_cdev, cur_freq, &static_power);
	if (ret != 0)
		return ret;

	dyn_power = power - static_power;
	dyn_power = dyn_power > 0 ? dyn_power : 0;

#ifdef CONFIG_ITS_IPA
	if (check_its_enabled()) {
		target_freq = its_normalized_data_to_freq(cpufreq_cdev,
							  dyn_power);
	} else {
#endif
		last_load = cpufreq_cdev->last_load ?: 1;
		normalised_power = (dyn_power * 100) / last_load;
		for (i = 0; i < cpufreq_cdev->max_level; i++)
			if (normalised_power >= cpufreq_cdev->freq_table[i].power)
				break;
		target_freq = cpufreq_cdev->freq_table[i].frequency;
#ifdef CONFIG_ITS_IPA
	}
#endif

	*state = get_level(cpufreq_cdev, target_freq);
	trace_thermal_power_cpu_limit(policy->related_cpus, target_freq, *state, power);
	trace_IPA_actor_cpu_limit(policy->related_cpus, target_freq, *state, power);
	return 0;
}

int gpu_cooling_get_limit_state(struct devfreq_cooling_device *dfc, unsigned long *state)
{
	unsigned long limit_state;
	int gpu_id = -1;

	if (dfc == NULL)
		return -EINVAL;

	gpu_id = ipa_get_actor_id("gpu");
	if (gpu_id < 0)
		return -EINVAL;

	if (g_ipa_soc_state[gpu_id] < dfc->freq_table_size)
		g_ipa_soc_freq_limit[gpu_id] = dfc->freq_table[g_ipa_soc_state[gpu_id]];

	if (g_ipa_board_state[gpu_id] < dfc->freq_table_size)
		g_ipa_board_freq_limit[gpu_id] = dfc->freq_table[g_ipa_board_state[gpu_id]];

	limit_state = max(g_ipa_soc_state[gpu_id], g_ipa_board_state[gpu_id]); /*lint !e1058*/
	if (limit_state < dfc->freq_table_size)
		*state = max(*state, limit_state);
	return 0;
}

extern int update_devfreq(struct devfreq *devfreq);
#define HZ_PER_KHZ	1000

int gpu_update_limit_freq(struct devfreq_cooling_device *dfc,
	unsigned long state)
{
	struct devfreq *df = dfc->devfreq;
	struct device *dev = df->dev.parent;
	unsigned long freq = 0;
	int gpu_id = -1;
	int ret;

	if (dfc == NULL)
		return -EINVAL;

	gpu_id = ipa_get_actor_id("gpu");
	if (gpu_id < 0)
		return -EINVAL;

	if (state == THERMAL_NO_LIMIT) {
		freq = 0;
	} else {
		if (state >= dfc->freq_table_size)
			return -EINVAL;

		freq = dfc->freq_table[state];
	}

#ifdef CONFIG_THERMAL_SPM
	if (is_spm_mode_enabled())
		freq = get_powerhal_profile(gpu_id);
#endif
	g_ipa_freq_limit[gpu_id] = freq;
	trace_IPA_actor_gpu_cooling(freq/HZ_PER_KHZ, state);
	ret = dev_pm_qos_update_request(&dfc->req_max_freq,
					DIV_ROUND_UP(freq, HZ_PER_KHZ));
	if (ret < 0)
		dev_info(dev, "update devfreq fail %d\n", ret);
	return 0;
}

static unsigned long
get_dfc_static_power(struct devfreq_cooling_device *dfc, unsigned long freq)
{
	struct devfreq *df = dfc->devfreq;
	unsigned long voltage;

	if (!dfc->power_ops->get_static_power)
		return 0;

	voltage = gpu_get_voltage(df, freq);

	if (voltage == 0)
		return 0;

	return dfc->power_ops->get_static_power(df, voltage);
}
static unsigned long
get_dfc_dynamic_power(struct devfreq_cooling_device *dfc, unsigned long freq,
		      unsigned long voltage)
{
	u64 power;
	u32 freq_mhz;
	struct devfreq_cooling_power *dfc_power = dfc->power_ops;

	if (dfc_power->get_dynamic_power)
		return dfc_power->get_dynamic_power(dfc->devfreq, freq,
						    voltage);

	freq_mhz = freq / 1000000;
	power = (u64)dfc_power->dyn_power_coeff * freq_mhz * voltage * voltage;
	do_div(power, 1000000000);

	return power;
}

void gpu_print_power_table(struct devfreq_cooling_device *dfc,
	unsigned long freq, unsigned long voltage)
{
	unsigned long power_static;
	unsigned long power_dyn;

	if (dfc == NULL)
		return;

	power_static = get_dfc_static_power(dfc, freq);
	power_dyn = get_dfc_dynamic_power(dfc, freq, voltage);
	pr_err("%lu MHz @ %lu mV: %lu + %lu = %lu mW\n",
		 freq / 1000000, voltage,
		 power_dyn, power_static, power_dyn + power_static);
}

void gpu_scale_power_by_time(struct thermal_cooling_device *cdev,
			     unsigned long state, u32 *static_power,
			     u32 *dyn_power)
{
	struct devfreq_cooling_device *dfc = cdev->devdata;
	struct devfreq *df = dfc->devfreq;
	struct devfreq_dev_status *status = &df->last_status;
	unsigned long freq = status->current_frequency;
	u32 dync_power = 0;
	unsigned long load = 0;
#ifdef CONFIG_ITS_IPA
	unsigned long long temp_power = 0;
	unsigned long voltage;
	int ret;

	/* check device is GPU */
	if (strncmp(cdev->type, "thermal-devfreq-0",
		strlen("thermal-devfreq-0")) == 0 &&
		check_its_enabled() &&
		check_dpm_enabled(DPM_GPU_MODULE)) {
		voltage = gpu_get_voltage(df, freq);
		*dyn_power = 0;
		*static_power = 0;
		ret = get_gpu_dynamic_power(&temp_power);
		if (ret == 0)
		*dyn_power = (unsigned long)temp_power;
		ret = get_gpu_leakage_power(&temp_power);
		if (ret == 0)
			*static_power = (unsigned long)temp_power;

		dfc->normalized_powerdata =
			normalize_devfreq_power(voltage, *dyn_power, freq);
	} else {
#endif
		dync_power = dfc->power_table[state];
		/* Scale dynamic power for utilization */
		if (status->total_time) {
			dync_power *= status->busy_time;
			dync_power /= status->total_time;
			*dyn_power = dync_power;
		}
		/* Get static power */
		*static_power = gpu_get_static_power(dfc, freq);
#ifdef CONFIG_ITS_IPA
	}
#endif
	if (status->total_time)
		load = 100 * status->busy_time / status->total_time;

	trace_IPA_actor_gpu_get_power((freq / 1000), load, *dyn_power, *static_power,
			*dyn_power + *static_power);

	cdev->current_load = load;
	cdev->current_freq = freq;
}

void ipa_parse_pid_parameter(struct device_node *child, struct thermal_zone_params *tzp)
{
	u32 prop = 0;
	const char *gov_name = NULL;

	if (!of_property_read_u32(child, "k_po", &prop))
		tzp->k_po = prop;
	if (!of_property_read_u32(child, "k_pu", &prop))
		tzp->k_pu = prop;
	if (!of_property_read_u32(child, "k_i", &prop))
		tzp->k_i = prop;
	if (!of_property_read_u32(child, "integral_cutoff", &prop))
		tzp->integral_cutoff = prop;

	/* set power allocator governor as defualt */
	if (!of_property_read_string(child, "governor_name", (const char **)&gov_name))
		(void)strncpy_s(tzp->governor_name, sizeof(tzp->governor_name),
				gov_name, strlen(gov_name));

	tzp->max_sustainable_power = tzp->sustainable_power;
}
