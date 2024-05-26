/*
 * ambient_temp.c
 *
 * ambient temp calculation module
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

#include <linux/module.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/rtc.h>
#include "peripheral_tm.h"
#include <platform_include/basicplatform/linux/power/platform/coul/coul_drv.h>
#include <linux/lpm_thermal.h>

#define DEFAULT_AMBIENT_TEMP	0
#define AMBIENT_MAX_TIME_SEC	(15 * 60)
#define HOURTOSEC		3600
#define LOW_CURRENT		50
#define KEEP_CURRENT		100

struct ithermal_ambient_sensor_t {
	u32 id;
	int temp;
};

struct ithermal_ambient_t {
	int sensor_count;
	u32 interval;
	int bias;
	int temp;
	int start_cc;
	u32 id;
	struct timeval last_time;
	struct timeval now;
	struct thermal_zone_device *tz_dev;
	struct ithermal_ambient_sensor_t ithermal_ambient_sensor[0];
};

static int ithermal_get_ambient_temp(struct thermal_zone_device *thermal, int *temp)
{
	struct ithermal_ambient_t *ithermal_ambient = thermal->devdata;

	if (ithermal_ambient == NULL || temp == NULL)
		return -EINVAL;

	*temp = ithermal_ambient->temp;

	return 0;
}

/*lint -e785*/
static struct thermal_zone_device_ops ambient_thermal_zone_ops = {
	.get_temp = ithermal_get_ambient_temp,
};

static int ithermal_ambient_sensors_dts_parse(struct device_node *np,
					  struct ithermal_ambient_t *ithermal_ambient)
{
	struct device_node *child = NULL;
	struct ithermal_ambient_sensor_t *ambient_sensor = NULL;
	const char *ptr_type = NULL;
	int i = 0;
	int ret;

	for_each_child_of_node(np, child) {
		ret = of_property_read_string(child, "type", &ptr_type);
		if (ret != 0) {
			pr_err("%s type read err\n", __func__);
			return -EINVAL;
		}

		ret = ipa_get_periph_id(ptr_type);
		if (ret < 0) {
			pr_err("%s sensor id get err\n", __func__);
			return -EINVAL;
		}
		ithermal_ambient->id = (u32)ret;
	}

	for_each_child_of_node(np, child) {
		ambient_sensor = (struct ithermal_ambient_sensor_t *)(uintptr_t)
				 ((u64)(uintptr_t)ithermal_ambient +
				  sizeof(struct ithermal_ambient_t) + (u64)((long)i) *
				  (sizeof(struct ithermal_ambient_sensor_t)));
		ret = of_property_read_string(child, "type", &ptr_type);
		if (ret != 0) {
			pr_err("%s type read err\n", __func__);
			return -EINVAL;
		}

		ret = ipa_get_periph_id(ptr_type);
		if (ret < 0) {
			pr_err("%s sensor id get err\n", __func__);
			return -EINVAL;
		}
		ambient_sensor->id = (u32)ret;

		i++;
	}

	return 0;
}


/*lint +e785*/
static int ithermal_ambient_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *dev_node = dev->of_node;
	int ret;
	int sensor_count;
	struct device_node *np = NULL;
	struct ithermal_ambient_t *ithermal_ambient = NULL;

	if (!of_device_is_available(dev_node)) {
		dev_err(dev, "ambient dev not found\n");
		return -ENODEV;
	}

	np = of_find_node_by_name(dev_node, "sensors");
	if (np == NULL) {
		pr_err("sensors node not found\n");
		ret = -ENODEV;
		goto exit;
	}

	sensor_count = of_get_child_count(np);
	if (sensor_count <= 0) {
		ret = -EINVAL;
		pr_err("%s sensor count read err\n", __func__);
		goto node_put;
	}

	ithermal_ambient = kzalloc(sizeof(struct ithermal_ambient_t) +
			       (u64)((long)sensor_count) *
			       (sizeof(struct ithermal_ambient_sensor_t)),
			       GFP_KERNEL);
	if (ithermal_ambient == NULL) {
		ret = -ENOMEM;
		pr_err("no enough memory\n");
		goto node_put;
	}

	ithermal_ambient->sensor_count = sensor_count;

	ret = of_property_read_u32(dev_node, "interval", &ithermal_ambient->interval);
	if (ret != 0) {
		pr_err("%s interval read err\n", __func__);
		goto free_mem;
	}

	ret = of_property_read_s32(dev_node, "bias", &ithermal_ambient->bias);
	if (ret != 0) {
		pr_err("%s bias read err\n", __func__);
		goto free_mem;
	}

	ret = ithermal_ambient_sensors_dts_parse(np, ithermal_ambient);
	if (ret != 0)
		goto free_mem;

	/* init data */
	do_gettimeofday(&ithermal_ambient->last_time);
	do_gettimeofday(&ithermal_ambient->now);
	ithermal_ambient->start_cc = -1;
	ithermal_ambient->temp = DEFAULT_AMBIENT_TEMP;

	ithermal_ambient->tz_dev =
		thermal_zone_device_register(dev_node->name, 0, 0, ithermal_ambient,
					     &ambient_thermal_zone_ops,
					     NULL, 0, 0);
	if (IS_ERR(ithermal_ambient->tz_dev)) {
		dev_err(dev, "register thermal zone for ambient failed.\n");
		ret = -ENODEV;
		goto free_mem;
	}

	of_node_put(np);

	platform_set_drvdata(pdev, ithermal_ambient);

	pr_info("%s ok\n", __func__);

	return 0; /*lint !e429*/

free_mem:
	kfree(ithermal_ambient);
node_put:
	of_node_put(np);
exit:

	return ret;
}

static int ithermal_ambient_remove(struct platform_device *pdev)
{
	struct ithermal_ambient_t *ithermal_ambient = platform_get_drvdata(pdev);

	if (ithermal_ambient != NULL) {
		platform_set_drvdata(pdev, NULL);
		thermal_zone_device_unregister(ithermal_ambient->tz_dev);
		kfree(ithermal_ambient);
	}

	return 0;
}

/*lint -e785*/

static struct of_device_id ithermal_ambient_of_match[] = {
	{ .compatible = "ithermal,ambient-temp" },
	{},
};

/*lint +e785*/

MODULE_DEVICE_TABLE(of, ithermal_ambient_of_match);

int ithermal_ambient_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct ithermal_ambient_t *ithermal_ambient = NULL;
	int now_cc = 0;
	long delta_time = 0;
	int delta_cc = 0;
	int avg_current = 0;

	pr_info("%s+\n", __func__);

	ithermal_ambient = platform_get_drvdata(pdev);
	if (ithermal_ambient != NULL) {
		now_cc = coul_get_battery_cc();
		/* check avg_current, and update the new time. */
		if (ithermal_ambient->start_cc == -1) {
			pr_info("ambient start_cc == -1, start to log.\n");
			ithermal_ambient->start_cc = now_cc;
			do_gettimeofday(&ithermal_ambient->last_time);
		} else {
			do_gettimeofday(&ithermal_ambient->now);
			delta_time = ithermal_ambient->now.tv_sec -
				     ithermal_ambient->last_time.tv_sec;
			if (delta_time == 0)
				return 0;
			/* unit modify from uah to mah */
			delta_cc = abs(now_cc - ithermal_ambient->start_cc) / 1000;
			if (delta_cc > (INT_MAX / HOURTOSEC)) {
				pr_err("%s-,delta battery CC is too big,"
				       "now_cc is %d, start_cc is %d .\n",
				       __func__, now_cc, ithermal_ambient->start_cc);
				ithermal_ambient->start_cc = -1;
				return 0;
			}
			/* unit is ma */
			avg_current = (delta_cc * HOURTOSEC / (int)delta_time);
			if (avg_current > KEEP_CURRENT) {
				pr_info("ambient avg.current %d too high,"
					"reset ambient monitor data.\n",
					avg_current);
				do_gettimeofday(&ithermal_ambient->last_time);
				ithermal_ambient->start_cc = now_cc;
			}
		}
	}

	pr_info("%s-\n", __func__);

	return 0;
}

int ithermal_ambient_resume(struct platform_device *pdev)
{
	int i;
	int ambient_temp = 0;
	int min_temp = 0;
	int now_cc = 0;
	long delta_time = 0;
	int delta_cc = 0;
	int avg_current = 0;
	int ret;
	struct ithermal_ambient_sensor_t *ambient_sensor = NULL;
	struct ithermal_ambient_t *ithermal_ambient = NULL;
	struct rtc_time tm = {0};

	pr_info("%s+\n", __func__);

	ithermal_ambient = platform_get_drvdata(pdev);
	if (ithermal_ambient == NULL)
		goto out;

	do_gettimeofday(&ithermal_ambient->now);
	now_cc = coul_get_battery_cc();
	delta_time = ithermal_ambient->now.tv_sec -
		     ithermal_ambient->last_time.tv_sec;
	if (delta_time == 0) {
		pr_info("%s-, delta_time is 0.\n", __func__);
		return 0;
	}
	/* unit modify from uah to mah */
	delta_cc = abs(now_cc - ithermal_ambient->start_cc) / 1000;
	if (delta_cc > (INT_MAX / HOURTOSEC)) {
		pr_err("%s-,delta battery CC is too big. now_cc is %d, "
		       "start_cc is %d .\n",
		       __func__, now_cc, ithermal_ambient->start_cc);
		ithermal_ambient->start_cc = -1;
		return 0;
	}
	/* unit is ma */
	avg_current = (delta_cc * HOURTOSEC / (int)delta_time);
	if (delta_time >= ithermal_ambient->interval &&
	    avg_current <= LOW_CURRENT) {
		/*
		 * time > interval (15 mins) and avg current
		 * is lower than LOW_CURRENT (50mA).
		 */
		pr_info("%s time pass %ld > interval %d s , "
			"now_cc = %d, start_cc = %d, "
			"avg_current = %d < LOW_CURRENT.\n ",
			__func__, delta_time,  ithermal_ambient->interval,
			now_cc, ithermal_ambient->start_cc, avg_current);

		for (i = 0; i < ithermal_ambient->sensor_count ; i++) {
			ambient_sensor = (struct ithermal_ambient_sensor_t *)(uintptr_t)
					  ((u64)(uintptr_t)ithermal_ambient +
					  sizeof(struct ithermal_ambient_t) +
					  (u64)((long)i) *
					  (sizeof(struct ithermal_ambient_sensor_t)));

			ret = ipa_get_periph_value(ambient_sensor->id,
						   &ambient_temp);
			if (ret != 0) {
				pr_err("%s get_periph_value fail", __func__);
				return 0;
			}

			if (i == 0 || min_temp > ambient_temp)
				min_temp = ambient_temp;
		}
		/* temp is highres, so div 1000) */
		min_temp = min_temp / 1000;

		/* update ambient temp and print UTC time */
		ithermal_ambient->temp = (min_temp - ithermal_ambient->bias);
		rtc_time_to_tm(ithermal_ambient->now.tv_sec, &tm);

		pr_err("%s ambient temp: %d at UTC time: %d-%d-%d %d:%d:%d.\n",
		       __func__, ithermal_ambient->temp, tm.tm_year + 1900,
		       tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min,
		       tm.tm_sec);
		/* reset calc cycle */
		ithermal_ambient->start_cc = -1;
		do_gettimeofday(&ithermal_ambient->last_time);
		do_gettimeofday(&ithermal_ambient->now);
	} else if (avg_current > KEEP_CURRENT) {
		/* clean the data, because current is too high. */
		pr_info("%s avg_current = %d > KEEP_CURRENT %d, "
			"reset calc cycle.\n ",
			__func__, avg_current, KEEP_CURRENT);
		ithermal_ambient->start_cc = -1;
	} else {
		/*
		 * current is lower KEEP_CURRNET,
		 * but time is not enough. Just wait.
		 */
		pr_info("%s time pass %ld, avg_current = %d.\n ",
			__func__, delta_time, avg_current);
	}

out:
	pr_info("%s-\n", __func__);

	return 0;
}

static struct platform_driver ithermal_ambient_platdrv = {
	.driver = {
		.name = "ithermal-ambient-temp",
		.owner = THIS_MODULE,
		.of_match_table = ithermal_ambient_of_match,
	},
	.probe = ithermal_ambient_probe,
	.remove = ithermal_ambient_remove,
	.suspend = ithermal_ambient_suspend,
	.resume = ithermal_ambient_resume,
};

static int __init ithermal_ambient_init(void)
{
	return platform_driver_register(&ithermal_ambient_platdrv); /*lint !e64*/
}

static void __exit ithermal_ambient_exit(void)
{
	platform_driver_unregister(&ithermal_ambient_platdrv);
}

module_init(ithermal_ambient_init);
module_exit(ithermal_ambient_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("thermal ambient module driver");
