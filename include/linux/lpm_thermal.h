 /*
 * thermal.h
 *
 * head file for thermal
 *
 * Copyright (c) 2017-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef __LPM_THERMAL_H__
#define __LPM_THERMAL_H__
#include <linux/thermal.h>
#ifdef CONFIG_PERF_CTRL
#include <linux/thermal_perf_ctrl.h>
#endif
#ifdef CONFIG_ITS
#include <soc_its_para.h>
#endif
#include <linux/pm_qos.h>
#include <linux/version.h>
#include <linux/cpufreq.h>
#include <linux/cpu_cooling.h>
#include <linux/devfreq.h>
#include <linux/devfreq_cooling.h>

/*
 * This is used to ensure the compiler did actually allocate the register we
 * asked it for some inline assembly sequences.  Apparently we can't trust
 * the compiler from one version to another so a bit of paranoia won't hurt.
 * This string is meant to be concatenated with the inline asm string and
 * will cause compilation to stop on mismatch.
 * (for details, see gcc PR 15089)
 * For compatibility with clang, we have to specifically take the equivalence
 * of 'r11' <-> 'fp' and 'r12' <-> 'ip' into account as well.
 */
#define __asmeq(x, y)				\
	".ifnc " x "," y "; "			\
	  ".ifnc " x y ",fpr11; " 		\
	    ".ifnc " x y ",r11fp; "		\
	      ".ifnc " x y ",ipr12; " 		\
	        ".ifnc " x y ",r12ip; "		\
	          ".err; "			\
	        ".endif; "			\
	      ".endif; "			\
	  ".endif; "				\
	  ".endif; "				\
	".endif\n\t"

#ifdef CONFIG_IPA_THERMAL
/**
 * struct __thermal_cooling_bind_param - a cooling device for a trip point
 * @cooling_device: a pointer to identify the referred cooling device
 * @min: minimum cooling state used at this trip point
 * @max: maximum cooling state used at this trip point
 */
struct __thermal_cooling_bind_param {
	struct device_node *cooling_device;
	unsigned long min;
	unsigned long max;
#ifdef CONFIG_IPA_THERMAL
	bool is_soc_cdev;
#endif
};

/**
 * struct __thermal_bind_param - a match between trip and cooling device
 * @tcbp: a pointer to an array of cooling devices
 * @count: number of elements in array
 * @trip_id: the trip point index
 * @usage: the percentage (from 0 to 100) of cooling contribution
 */
struct __thermal_bind_params {
	struct __thermal_cooling_bind_param *tcbp;
	unsigned int count;
	unsigned int trip_id;
	unsigned int usage;
};

/**
 * struct __thermal_zone - internal representation of a thermal zone
 * @mode: current thermal zone device mode (enabled/disabled)
 * @passive_delay: polling interval while passive cooling is activated
 * @polling_delay: zone polling interval
 * @slope: slope of the temperature adjustment curve
 * @offset: offset of the temperature adjustment curve
 * @ntrips: number of trip points
 * @trips: an array of trip points (0..ntrips - 1)
 * @num_tbps: number of thermal bind params
 * @tbps: an array of thermal bind params (0..num_tbps - 1)
 * @sensor_data: sensor private data used while reading temperature and trend
 * @ops: set of callbacks to handle the thermal zone based on DT
 */
struct __thermal_zone {
	enum thermal_device_mode mode;
	int passive_delay;
	int polling_delay;
	int slope;
	int offset;

	/* trip data */
	int ntrips;
	struct thermal_trip *trips;

	/* cooling binding data */
	int num_tbps;
	struct __thermal_bind_params *tbps;

	/* sensor interface */
	void *sensor_data;
	const struct thermal_zone_of_device_ops *ops;
};

/**
 * struct freq_table - frequency table along with power entries
 * @frequency:	frequency in KHz
 * @power:	power in mW
 *
 * This structure is built when the cooling device registers and helps
 * in translating frequency to power and vice versa.
 */
struct freq_table {
	u32 frequency;
	u32 power;
};

/**
 * struct time_in_idle - Idle time stats
 * @time: previous reading of the absolute time that this cpu was idle
 * @timestamp: wall time of the last invocation of get_cpu_idle_time_us()
 */
struct time_in_idle {
	u64 time;
	u64 timestamp;
};

/**
 * struct cpufreq_cooling_device - data for cooling device with cpufreq
 * @id: unique integer value corresponding to each cpufreq_cooling_device
 *	registered.
 * @last_load: load measured by the latest call to cpufreq_get_requested_power()
 * @cpufreq_state: integer value representing the current state of cpufreq
 *	cooling	devices.
 * @max_level: maximum cooling level. One less than total number of valid
 *	cpufreq frequencies.
 * @freq_table: Freq table in descending order of frequencies
 * @cdev: thermal_cooling_device pointer to keep track of the
 *	registered cooling device.
 * @policy: cpufreq policy.
 * @node: list_head to link all cpufreq_cooling_device together.
 * @idle_time: idle time stats
 *
 * This structure is required for keeping information of each registered
 * cpufreq_cooling_device.
 */
struct cpufreq_cooling_device {
	int id;
	u32 last_load;
	unsigned int cpufreq_state;
	unsigned int max_level;
	struct freq_table *freq_table;	/* In descending order */
	struct em_perf_domain *em;
	struct cpufreq_policy *policy;
	struct list_head node;
	struct time_in_idle *idle_time;
	struct freq_qos_request qos_req;
#ifdef CONFIG_ITS_IPA
	unsigned long normalized_powerdata;
#endif
};

/**
 * struct devfreq_cooling_device - Devfreq cooling device
 * @id:		unique integer value corresponding to each
 *		devfreq_cooling_device registered.
 * @cdev:	Pointer to associated thermal cooling device.
 * @devfreq:	Pointer to associated devfreq device.
 * @cooling_state:	Current cooling state.
 * @power_table:	Pointer to table with maximum power draw for each
 *			cooling state. State is the index into the table, and
 *			the power is in mW.
 * @freq_table:	Pointer to a table with the frequencies sorted in descending
 *		order.  You can index the table by cooling device state
 * @freq_table_size:	Size of the @freq_table and @power_table
 * @power_ops:	Pointer to devfreq_cooling_power, used to generate the
 *		@power_table.
 * @res_util:	Resource utilization scaling factor for the power.
 *		It is multiplied by 100 to minimize the error. It is used
 *		for estimation of the power budget instead of using
 *		'utilization' (which is	'busy_time / 'total_time').
 *		The 'res_util' range is from 100 to (power_table[state] * 100)
 *		for the corresponding 'state'.
 *>----->-------for the corresponding 'state'.
 * @req_max_freq:	PM QoS request for limiting the maximum frequency
 *			of the devfreq device.
 */
struct devfreq_cooling_device {
	int id;
	struct thermal_cooling_device *cdev;
	struct devfreq *devfreq;
	unsigned long cooling_state;
	u32 *power_table;
	u32 *freq_table;
	size_t freq_table_size;
	struct devfreq_cooling_power *power_ops;
	u32 res_util;
	int capped_state;
	struct dev_pm_qos_request req_max_freq;
#ifdef CONFIG_ITS_IPA
  unsigned long normalized_powerdata;
#endif
};

/**
 * struct power_allocator_params - parameters for the power allocator governor
 * @allocated_tzp:	whether we have allocated tzp for this thermal zone and
 *			it needs to be freed on unbind
 * @err_integral:	accumulated error in the PID controller.
 * @prev_err:	error in the previous iteration of the PID controller.
 *		Used to calculate the derivative term.
 * @trip_switch_on:	first passive trip point of the thermal zone.  The
 *			governor switches on when this trip point is crossed.
 *			If the thermal zone only has one passive trip point,
 *			@trip_switch_on should be INVALID_TRIP.
 * @trip_max_desired_temperature:	last passive trip point of the thermal
 *					zone.  The temperature we are
 *					controlling for.
 */
struct power_allocator_params {
	bool allocated_tzp;
	s64 err_integral;
	s32 prev_err;
	int trip_switch_on;
	int trip_max_desired_temperature;
};

struct power_allocator_params;
struct __thermal_zone;
int gpu_cooling_get_limit_state(struct devfreq_cooling_device *dfc, unsigned long *state);
int gpu_update_limit_freq(struct devfreq_cooling_device *dfc, unsigned long state);
void gpu_print_power_table(struct devfreq_cooling_device *dfc,
	unsigned long freq, unsigned long voltage);
void gpu_scale_power_by_time(struct thermal_cooling_device *cdev,
			     unsigned long state, u32 *static_power, u32 *dyn_power);
void gpu_power_to_state(struct thermal_cooling_device *cdev, u32 power, unsigned long *state);
void ipa_parse_pid_parameter(struct device_node *child, struct thermal_zone_params *tzp);
unsigned int get_state_freq(struct cpufreq_cooling_device *cpufreq_cdev,
			    unsigned long state);
int update_freq_table(struct cpufreq_cooling_device *cpufreq_cdev, u32 capacitance);
int cluster_get_static_power(cpumask_t *cpumask, unsigned long u_volt, u32 *static_power);
int cpu_get_static_power(struct cpufreq_cooling_device *cpufreq_cdev,
		     unsigned long freq, u32 *power);
void cpu_cooling_get_limit_state(struct cpufreq_cooling_device *cpufreq_cdev,
				 unsigned long state);
void cpu_print_freq_table(struct cpufreq_cooling_device *cpufreq_cdev,
	u32 freq_mhz, u32 voltage_mv, u64 power);
void ipa_freq_debug(int onoff);
void ipa_freq_limit_init(void);
void ipa_freq_limit_reset(struct thermal_zone_device *tz);
unsigned int ipa_freq_limit(int actor, unsigned int target_freq);
#ifdef CONFIG_PERF_CTRL
int get_ipa_status(struct ipa_stat *status);
#endif
bool get_spm_target_freq(int actor, unsigned int *target_freq);
void spm_cpufreq_verify_within_limits(struct cpufreq_policy *policy, unsigned long clipped_freq);
void thermal_zone_device_set_polling(struct thermal_zone_device *tz, int delay);
void get_cur_power(struct thermal_zone_device *tz);
#endif

#ifdef CONFIG_THERMAL_TSENSOR
ssize_t
trip_point_type_activate(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count);
#endif

#ifdef CONFIG_IPA_THERMAL
#define IPA_PERIPH_NUM 8
#define MIN_POWER_DIFF 50
#define BOARDIPA_PID_RESET_TEMP 2000
#define MAX_S48  0x7FFFFFFFFFFF
#define IPA_FREQ_MAX	(~0U)
#define USER_SPACE_GOVERNOR       "user_space"
#define SOC_THERMAL_NAME "soc_thermal"
#define BOARD_THERMAL_NAME "board_thermal"
#define CDEV_GPU_NAME "thermal-devfreq-0"
#define CDEV_CPU_CLUSTER0_NAME "thermal-cpufreq-0"
#define CDEV_CPU_CLUSTER1_NAME "thermal-cpufreq-1"
#define CDEV_CPU_CLUSTER2_NAME "thermal-cpufreq-2"
#define BOARDIPA_PID_RESET_TEMP 2000
#define CAPACITY_OF_ARRAY  10
#define IPA_CLUSTER0_WEIGHT_NAME       "cdev1_weight"  //default name value
#define IPA_CLUSTER1_WEIGHT_NAME       "cdev2_weight"
#define IPA_CLUSTER2_WEIGHT_NAME       "cdev3_weight"
#define IPA_GPU_WEIGHT_NAME            "cdev0_weight"

#ifdef CONFIG_THERMAL_SHELL
#define IPA_SENSOR_SHELL "shell_frame"
#define IPA_SENSOR_SHELLID    254
#endif
#define MAX_THERMAL_CLUSTER_NUM	3

enum thermal_boost_mode {
	THERMAL_BOOST_DISABLED = 0,
	THERMAL_BOOST_ENABLED,
};

#ifdef CONFIG_THERMAL_SPM
extern unsigned int get_powerhal_profile(int actor);
extern unsigned int get_minfreq_profile(int actor);
extern bool is_spm_mode_enabled(void);
#endif

typedef u32 (*get_power_t)(void);
u32 get_camera_power(void);
u32 get_backlight_power(void);
u32 get_charger_power(void);
void board_cooling_unregister(struct thermal_cooling_device *cdev);
struct thermal_cooling_device *board_power_cooling_register(struct device_node *np,
							    get_power_t get_power);
int ipa_get_tsensor_id(const char *name);
int ipa_get_sensor_value(u32 sensor, int *val);
int ipa_get_periph_id(const char *name);
int ipa_get_periph_value(u32 sensor, int *val);
struct cpufreq_frequency_table *cpufreq_frequency_get_table(unsigned int cpu);
void ipa_get_clustermask(int *clustermask, int len,
	int *oncpucluster, int *cpuclustercnt);
#ifdef CONFIG_NPU_DEVDRV
extern int npu_ctrl_core(u32 dev_id, u32 core_num);
#else
static inline int npu_ctrl_core(u32 dev_id, u32 core_num)
{
	(void)dev_id;
	(void)core_num;
	return 0;
}
#endif

extern int coul_get_battery_cc(void);
extern int cpufreq_update_policies(void);
extern int gpufreq_update_policies(void);
void trip_attrs_store(struct thermal_zone_device *tz, int mask, int indx);
void ipa_parse_pid_parameter(struct device_node *child,
			     struct thermal_zone_params *tzp);
//void ipa_power_efficient_wq_set_polling(struct thermal_zone_device *tz,
//	int delay);

/* the num of ipa cpufreq table equals cluster num , but
cluster num is a variable. So define a larger arrays in advance.
*/
extern u32 g_cluster_num;
extern u32 g_ipa_sensor_num;
extern u32 g_ipa_actor_num;
extern u32 ipa_cpufreq_table_index[CAPACITY_OF_ARRAY];
extern u32 ipa_actor_index[CAPACITY_OF_ARRAY];
extern u32 g_ipa_gpu_boost_weights[CAPACITY_OF_ARRAY];
extern u32 g_ipa_normal_weights[CAPACITY_OF_ARRAY];
extern const char *ipa_actor_name[CAPACITY_OF_ARRAY];
int nametoactor(const char *weight_attr_name);
s32 thermal_zone_temp_check(s32 temperature);
int thermal_zone_cdev_get_power(const char *thermal_zone_name, const char *cdev_name, unsigned int *power);
int of_parse_ipa_sensor_index_table(void);
int ipa_weights_cfg_init(void);
int ipa_get_actor_id(const char *name);
void ipa_freq_limit_init(void);
void ipa_freq_limit_reset(struct thermal_zone_device *tz);
unsigned int ipa_freq_limit(int actor, unsigned int target_freq);
int get_soc_temp(void);
void update_pid_value(struct thermal_zone_device *tz);
bool thermal_of_get_cdev_type(struct thermal_zone_device *tzd,
			      struct thermal_cooling_device *cdev);
void set_tz_type(struct thermal_zone_device *tz);
int power_actor_set_powers(struct thermal_zone_device *tz,
			   struct thermal_instance *instance,
			   u32 *soc_sustainable_power,
			   u32 granted_power);
#else
static inline int get_soc_temp(void)
{
	return 0;
}
#endif

#ifdef CONFIG_THERMAL_SHELL
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvisibility"
int get_shell_temp(struct thermal_zone_device *thermal, int *temp);
#pragma GCC diagnostic pop
#endif

#ifdef CONFIG_THERMAL_GPU_HOTPLUG
extern void gpu_thermal_cores_control(u64 cores);
#endif
#ifdef CONFIG_PERF_CTRL
int get_ipa_status(struct ipa_stat *status);
#endif

#ifdef CONFIG_ITS
extern int reset_power_result(void);
extern int get_its_power_result(its_cpu_power_t *result);
extern int get_its_core_dynamic_power(int core, unsigned long long *power);
extern int get_its_core_leakage_power(int core, unsigned long long *power);
extern bool check_its_enabled(void);
extern int get_gpu_leakage_power(unsigned long long *power);
extern int get_gpu_dynamic_power(unsigned long long *power);
#endif
int cpu_power_to_state(struct thermal_cooling_device *cdev, u32 power,
		       unsigned long *state);
#ifdef CONFIG_ITS_IPA
#define ITS_NORMALIZED_RATIO	1000000000UL
#define HZ_TO_MHZ_DIVISOR	1000000
unsigned long normalize_its_power(struct cpufreq_cooling_device *cpufreq_cdev,
				  unsigned int dynamic_power, unsigned long freq);
u32 cpu_get_dynamic_power(struct cpufreq_cooling_device *cpufreq_cdev);
#endif
#endif /* __LPM_THERMAL_H__ */
