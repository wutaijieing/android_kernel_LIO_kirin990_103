/*
 * Copyright (c) 2021, The Linux Foundation.All rights reserved.
 * Description: This file describe GPU thermal related init
 * Create: 2021-5-24
 *
 */

#include <linux/lpm_thermal.h>
#include <linux/devfreq_cooling.h>
#include <linux/delay.h>
#include <linux/of_address.h>
#include <linux/of.h>
#include <linux/of_platform.h>

struct thermal_cooling_device *g_devfreq_cooling = NULL;
#define MAX_UINT64 ((uint64_t)0xFFFFFFFFFFFFFFFFULL)

#ifdef OVERFLOW_DEBUG
static uint32_t ithermal_check_uint64_multi_overflow(uint64_t a, uint64_t b)
{
	if (a == 0 || b == 0)
		return 0;

	if (a > (MAX_UINT64 / b))
		return 1; /* overflow */

	return 0;
}
#else
static inline uint32_t ithermal_check_uint64_multi_overflow(uint64_t a, uint64_t b)
{
	(void)a;
	(void)b;
	return 0;
}
#endif

void ithermal_uint64_mulcheck(uint64_t a, uint64_t b)
{
	if (ithermal_check_uint64_multi_overflow(a, b) != 0)
		pr_err("Failed:Uint64 %llu and %llu multiply overflow!",
			a, b);
}

#ifdef OVERFLOW_DEBUG
static uint32_t ithermal_check_uint64_add_overflow(uint64_t a, uint64_t b)
{
	if (a == 0 || b == 0)
		return 0;

	if (a > (MAX_UINT64 - b))
		return 1; /* overflow */

	return 0;
}
#else
static inline uint32_t ithermal_check_uint64_add_overflow(uint64_t a, uint64_t b)
{
	(void)a;
	(void)b;
	return 0;
}
#endif

void ithermal_uint64_addcheck(uint64_t a, uint64_t b)
{
	if (ithermal_check_uint64_add_overflow(a, b) != 0)
		pr_err("Failed:Unt64 %llu and %llu addition overflow!",
			a, b);
}

#ifdef CONFIG_GPU_IPA_THERMAL
int ithermal_get_ipa_gpufreq_limit(unsigned long freq, unsigned long *freq_limit)
{
	int gpu_id;

	gpu_id = ipa_get_actor_id("gpu");
	if (gpu_id < 0) {
		pr_err("Failed to get ipa actor id for gpu.\n");
		return -ENODEV;
	}
	*freq_limit = ipa_freq_limit(gpu_id, freq);
	return 0;
}
#endif

#define NUMBER_OF_PARAMETER	5
int paras[NUMBER_OF_PARAMETER] = {0};
static unsigned long
ithermal_model_static_power(struct devfreq *devfreq __maybe_unused,
	unsigned long voltage)
{
	int temperature;
	const unsigned long voltage_cubed =
		(voltage * voltage * voltage) >> 10;
	unsigned long temp;
	unsigned long temp_squared;
	unsigned long temp_cubed;
	unsigned long temp_scaling_factor;

	/* multiplication overflow check */
	ithermal_uint64_mulcheck(voltage, voltage);
	ithermal_uint64_mulcheck(voltage * voltage, voltage);
#ifdef CONFIG_GPU_IPA_THERMAL
	temperature = get_soc_temp();
#else
	temperature = 0;
#endif
	/* convert to temp, temperature is 90000, so temp is 90 */
	temp = (unsigned long)((long)temperature) / 1000;
	/* multiplication overflow check */
	ithermal_uint64_mulcheck(temp, temp);
	temp_squared = temp * temp;
	ithermal_uint64_mulcheck(temp_squared, temp);
	temp_cubed = temp_squared * temp;
	/* multiplication overflow check */
	ithermal_uint64_mulcheck((unsigned long)paras[3], temp_cubed);
	ithermal_uint64_mulcheck((unsigned long)paras[2], temp_squared);
	ithermal_uint64_mulcheck((unsigned long)paras[1], temp);
	ithermal_uint64_addcheck((paras[3] * temp_cubed),
		(paras[2] * temp_squared));
	ithermal_uint64_addcheck((paras[1] * temp),
		(paras[3] * temp_cubed + paras[2] * temp_squared));
	ithermal_uint64_addcheck((unsigned long)paras[0],
		(paras[3] * temp_cubed + paras[2] * temp_squared +
		(paras[1] * temp)));
	temp_scaling_factor = paras[3] * temp_cubed +
		paras[2] * temp_squared +
		paras[1] * temp + paras[0];

	/* multiplication overflow check */
	ithermal_uint64_mulcheck((unsigned long)paras[4], voltage_cubed);
	ithermal_uint64_mulcheck(((paras[4] * voltage_cubed) >> 20),
		temp_scaling_factor);
	/* mW */
	return (((paras[4] * voltage_cubed) >> 20) *
		temp_scaling_factor) / 1000000; /* [false alarm]: no problem - fortify check */
}

static unsigned long
ithermal_model_dynamic_power(struct devfreq *devfreq __maybe_unused,
	unsigned long freq, unsigned long voltage)
{
	/* The inputs: freq (f) is in Hz, and voltage (v) in mV.
	 * The coefficient (c) is in mW/(MHz mV mV).
	 *
	 * This function calculates the dynamic power after this formula:
	 * Pdyn (mW) = c (mW/(MHz*mV*mV)) * v (mV) * v (mV) * f (MHz)
	 */
	const unsigned long v2 = (voltage * voltage) / 1000; /* m*(V*V) */
	const unsigned long f_mhz = freq / 1000000; /* MHz */
	unsigned long coefficient = 3600; /* mW/(MHz*mV*mV) */
	struct device_node *dev_node = NULL;
	u32 prop = 0;

	/* multiplication overflow check */
	ithermal_uint64_mulcheck(voltage, voltage);

	dev_node = of_find_node_by_name(NULL, "capacitances");
	if (dev_node != NULL) {
		int ret = of_property_read_u32(dev_node,
			"ithermal,gpu_dyn_capacitance", &prop);
		if (ret == 0)
			coefficient = prop;
	}

	/* multiplication overflow check */
	ithermal_uint64_mulcheck(coefficient, v2);
	ithermal_uint64_mulcheck(coefficient * v2, f_mhz);
	return (coefficient * v2 * f_mhz) / 1000000; /* mW */
}

static struct devfreq_cooling_power ithermal_model_ops = {
	.get_static_power = ithermal_model_static_power,
	.get_dynamic_power = ithermal_model_dynamic_power,
	.get_real_power = NULL,
};

void ithermal_gpu_cdev_register(struct device_node *df_node, struct devfreq *df)
{
	const char *temp_scale_cap[5];
	struct device_node *dev_node = NULL;
	int ret, i;

	if (df_node == NULL || df == NULL) {
		pr_err("%s:%d:df_node or df is null.\n", __func__, __LINE__);
		return;
	}

	dev_node = of_find_node_by_name(NULL, "capacitances");
	if (dev_node != NULL) {
		for (i = 0; i < 5; i++) {
			ret = of_property_read_string_index(dev_node,
				"ithermal,gpu_temp_scale_capacitance", i,
				&temp_scale_cap[i]);
			if (ret) {
				pr_err("%s temp_scale_cap %d read err\n",
					__func__, i);
				continue;
			}
			// string to 10-based int
			paras[i] = 0;
			ret = kstrtoint(temp_scale_cap[i], 10, &paras[i]);
			if (ret)
				continue;
		}
	}

	g_devfreq_cooling = of_devfreq_cooling_register_power(df_node,
		df, &ithermal_model_ops);
	if (IS_ERR_OR_NULL(g_devfreq_cooling))
		pr_err("%s:%d: gpu devfreq cooling init failed.\n",
		       __func__, __LINE__);
}

void ithermal_gpu_cdev_unregister(void)
{
	if (g_devfreq_cooling != NULL)
		devfreq_cooling_unregister(g_devfreq_cooling);
}

struct thermal_cooling_device *ithermal_get_gpu_cdev(void)
{
	return g_devfreq_cooling;
}
EXPORT_SYMBOL(ithermal_get_gpu_cdev);
