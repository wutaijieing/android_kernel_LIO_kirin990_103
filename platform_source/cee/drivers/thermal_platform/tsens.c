/*
 * tsens.c
 *
 * tsensor module
 *
 * Copyright (c) 2017-2021 Huawei Technologies Co., Ltd.
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
#include <linux/platform_device.h>
#include <linux/thermal.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>

#include <linux/compiler.h>
#include <linux/lpm_thermal.h>
#include <securec.h>
#include <thermal_core.h>

#define TSENS_DRIVER_NAME	"ithermal-tsens"
#define READTEMP_SVC_REG_RD	0xc5009900UL
#define CAPACITY_OF_TSENSOR_ARRAY	10
#define MAX_SENSOR_NUM 40

static struct tsens_tm_device *g_tmdev;
const char *g_tsensor_name[CAPACITY_OF_TSENSOR_ARRAY];
u32 g_tsensor_mode[CAPACITY_OF_TSENSOR_ARRAY];

/* Trips: warm and cool */
enum tsens_trip_type {
	TSENS_TRIP_ORIGNUM = 0,
#ifdef CONFIG_THERMAL_TRIP
	TSENS_TRIP_THROTTLING = TSENS_TRIP_ORIGNUM,
	TSENS_TRIP_SHUTDOWN,
	TSENS_TRIP_BELOW_VR_MIN,
	TSENS_TRIP_NUM,
#endif
};

enum sensor_type {
	TYPE_TSENSOR = 0,
	TYPE_PVTSENSOR,
};

struct tsens_tm_device_sensor {
	struct thermal_zone_device *tz_dev;
	enum thermal_device_mode mode;
	int reg_no;
	unsigned int sensor_num;
	int sensor_type;
#ifdef CONFIG_THERMAL_TRIP
	s32 temp_throttling;
	s32 temp_shutdown;
	s32 temp_below_vr_min;
#endif
};

struct tsens_tm_device {
	struct platform_device *pdev;
	int tsens_num_sensor;
	u32 adc_start_value;
	u32 adc_end_value;
	u32 adc_delta_value;
	u32 pvtsensor_adc_start_value;
	u32 pvtsensor_adc_end_value;
	struct tsens_tm_device_sensor sensor[0];
};

struct tsens_inf {
	unsigned int register_info;
	int tsens_num_sensors;
};

struct tsens_lite_inf {
	int oncpucluster[MAX_THERMAL_CLUSTER_NUM];
	int cpuclustercnt;
	int gpuclustercnt;
	int npuclustercnt;
};

#define TSENSOR_8BIT			8
#define TSENS_THRESHOLD_MIN_CODE	0x0
#define TSENS_TEMP_START_VALUE		(-40) /* -40 deg C */
#define TSENS_TEMP_END_VALUE		125

int g_tsensor_debug = 0;

void tsen_debug(int onoff)
{
	g_tsensor_debug = onoff;
}

ssize_t trip_point_type_activate(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	struct thermal_zone_device *tz = to_thermal_zone(dev);
	int trip = 0, result = 0;

	if (!tz->ops->activate_trip_type)
		return -EPERM;

	if (!sscanf_s(attr->attr.name, "trip_point_%d_type", &trip))
		return -EINVAL;

	if (!strncmp(buf, "enabled", strlen("enabled"))) {
		result =
			tz->ops->activate_trip_type(tz, trip, THERMAL_TRIP_ACTIVATION_ENABLED);
	} else if (!strncmp(buf, "disabled", strlen("disabled"))) {
		result =
			tz->ops->activate_trip_type(tz, trip, THERMAL_TRIP_ACTIVATION_DISABLED);
	} else {
		result = -EINVAL;
	}
	if (result)
		return result;
	return count;
}

static int tsens_tz_code_to_degc(int adc_val, int sensor_type)
{
	int temp = 0;
	int adc_start_value, adc_end_value, adc_delta_value;

	if (g_tmdev == NULL)
		goto ERROR;

	if (sensor_type == TYPE_TSENSOR) {
		adc_start_value = (int)g_tmdev->adc_start_value;
		adc_end_value = (int)g_tmdev->adc_end_value;
		adc_delta_value = (int)g_tmdev->adc_delta_value;
	} else if (sensor_type == TYPE_PVTSENSOR) {
		adc_start_value = (int)g_tmdev->pvtsensor_adc_start_value;
		adc_end_value = (int)g_tmdev->pvtsensor_adc_end_value;
	} else {
		adc_start_value = 0;
		adc_end_value = 0;
	}

	if (adc_start_value > adc_val || adc_val > adc_end_value ||
	    adc_start_value == 0 || adc_end_value == 0)
		goto ERROR;

	temp = ((adc_val - adc_start_value) *
		(TSENS_TEMP_END_VALUE - TSENS_TEMP_START_VALUE)) /
		(adc_end_value - adc_start_value) - adc_delta_value;
	temp = temp + TSENS_TEMP_START_VALUE;

	return temp;

ERROR:
	pr_debug("adc_to_temp_conversion failed\n");
	return temp;
}

/*lint -e715*/
noinline int atfd_readtemp_smc(u64 _function_id, u64 _arg0, u64 _arg1, u64 _arg2)
{
	register u64 function_id asm("x0") = _function_id;
	register u64 arg0 asm("x1") = _arg0;
	register u64 arg1 asm("x2") = _arg1;
	register u64 arg2 asm("x3") = _arg2;
	asm volatile (
		__asmeq("%0", "x0")
		__asmeq("%1", "x1")
		__asmeq("%2", "x2")
		__asmeq("%3", "x3")
#ifndef CONFIG_LIBLINUX
		"smc #0\n"
#else
		"svc #0x4200\n"
#endif
		: "+r" (function_id)
		: "r" (arg0), "r" (arg1), "r" (arg2));

	return (int)function_id;
}

/*lint +e715*/

int sec_readtemp_read(u64 reg_address)
{
	return atfd_readtemp_smc(READTEMP_SVC_REG_RD, reg_address, 0UL, 0UL);
}

static int tsens_tz_get_temp(struct thermal_zone_device *thermal,
			     int *temp)
{
	struct tsens_tm_device_sensor *tm_sensor = thermal->devdata;
	int adc_value = 0;

	if (tm_sensor == NULL || tm_sensor->mode != THERMAL_DEVICE_ENABLED ||
	    temp == NULL)
		return -EINVAL;

	if (tm_sensor->reg_no >= 0 && tm_sensor->reg_no < g_tmdev->tsens_num_sensor) {
		adc_value = sec_readtemp_read(tm_sensor->reg_no);
		*temp = tsens_tz_code_to_degc(adc_value, tm_sensor->sensor_type);
	} else {
		*temp = TSENS_TEMP_START_VALUE;
	}

	return 0;
}

/* add for IPA */
int tsens_get_temp(u32 sensor, int *temp)
{
	int i;
	struct thermal_zone_device *thermal = NULL;
	int ret = -EINVAL;
	int tmp = 0;

	for (i = 0; i < g_tmdev->tsens_num_sensor; i++) {
		if (sensor == g_tmdev->sensor[i].sensor_num) {
			thermal = g_tmdev->sensor[i].tz_dev;
			ret = tsens_tz_get_temp(thermal, &tmp);
			if (tmp < 0)
				tmp = 0;
			*temp = tmp * 1000;
		}
	}

	return ret;
}

int ipa_get_tsensor_id(const char *name)
{
	int ret = -ENODEV;
	int id;

	if (name == NULL) {
		pr_err("%s:name == NULL!\n", __func__);
		return ret;
	}

	pr_debug("IPA tsensor_num = %d\n", g_tmdev->tsens_num_sensor);
	for (id = 0; id < g_tmdev->tsens_num_sensor; id++) {
		pr_debug("IPA: sensor_name = %s, g_tsensor_name %d = %s\n",
			 name, id, g_tsensor_name[id]);

		if (strncmp(name, g_tsensor_name[id],
			    strlen(g_tsensor_name[id])) == 0) {
			ret = id;
			pr_debug("sensor_id = %d\n", ret);
			return ret;
		}
	}

#ifdef CONFIG_THERMAL_SHELL
	if (strncmp(name, IPA_SENSOR_SHELL, strlen(IPA_SENSOR_SHELL)) == 0)
		ret = IPA_SENSOR_SHELLID;
#endif

	return ret;
}
EXPORT_SYMBOL_GPL(ipa_get_tsensor_id);

int ipa_get_sensor_value(u32 sensor, int *val)
{
	int ret = -EINVAL;
	u32 sensor_num = sizeof(g_tsensor_name) / sizeof(char *);

	if (val == NULL) {
		pr_err("%s parm null\n", __func__);
		return ret;
	}

	if (sensor < sensor_num)
		ret = tsens_get_temp(sensor, val);

	return ret;
}
EXPORT_SYMBOL_GPL(ipa_get_sensor_value);
/* end of add for IPA */

static int tsens_tz_get_trip_type(struct thermal_zone_device *thermal,
				  int trip, enum thermal_trip_type *type)
{
	struct tsens_tm_device_sensor *tm_sensor = thermal->devdata;
	int ret = 0;

	if (tm_sensor == NULL || trip < 0 || type == NULL)
		return -EINVAL;

	switch (trip) {
#ifdef CONFIG_THERMAL_TRIP
	case TSENS_TRIP_THROTTLING:
		*type = THERMAL_TRIP_THROTTLING;
		break;
	case TSENS_TRIP_SHUTDOWN:
		*type = THERMAL_TRIP_SHUTDOWN;
		break;
	case TSENS_TRIP_BELOW_VR_MIN:
		*type = THERMAL_TRIP_BELOW_VR_MIN;
		break;
#endif
	default:
		ret = -EINVAL;
	}

	return ret;
}

/*lint -e764 -e527 -esym(764,527,*)*/
static int tsens_tz_get_trip_temp(struct thermal_zone_device *thermal,
				  int trip, int *temp)
{
	struct tsens_tm_device_sensor *tm_sensor = thermal->devdata;
	int ret = 0;

	if (tm_sensor == NULL || trip < 0 || temp == NULL)
		return -EINVAL;

	switch (trip) {
#ifdef CONFIG_THERMAL_TRIP
	case TSENS_TRIP_THROTTLING:
		*temp = tm_sensor->temp_throttling;
		break;
	case TSENS_TRIP_SHUTDOWN:
		*temp = tm_sensor->temp_shutdown;
		break;
	case TSENS_TRIP_BELOW_VR_MIN:
		*temp = tm_sensor->temp_below_vr_min;
		break;
#endif
	default:
		ret = -EINVAL;
	}

	return ret;
}

/*lint -e764 -e527 +esym(764,527,*)*/

static struct thermal_zone_device_ops tsens_thermal_zone_ops = {
	.get_temp = tsens_tz_get_temp,
	.get_trip_type = tsens_tz_get_trip_type,
	.get_trip_temp = tsens_tz_get_trip_temp,
};

#ifdef CONFIG_THERMAL_SUPPORT_LITE
static int update_sens_num(int tsens_num_sensors, int cpuclustercnt,
			   int gpuclustercnt, int npuclustercnt)
{
	struct device_node *np = NULL;
	int ret;

	np = of_find_node_by_name(NULL, "ipa_sensors_info");
	if (np != NULL) {
		ret = of_property_read_u32(np, "ithermal,cluster_num", &g_cluster_num);
		if (ret != 0)
			return ret;
	}
	g_tmdev->tsens_num_sensor = tsens_num_sensors - (g_cluster_num - cpuclustercnt) -
				(1 - gpuclustercnt) - (1 - npuclustercnt);
	return 0;
}

static int lite_sensor_exist(const struct device_node *of_node, int i, int cpuclustercnt,
			     int gpuclustercnt, int npuclustercnt)
{
	const char *g_local_tsensor_name = NULL;

	if (cpuclustercnt != g_cluster_num && i >= cpuclustercnt &&
	    i < g_cluster_num) {
		pr_err("%s-%d %d %d %d\n", __func__, __LINE__, i, cpuclustercnt, g_cluster_num);
		return -ENODEV;
	}

	of_property_read_string_index(of_node, "ithermal,tsensor_name",
				      i, &g_local_tsensor_name); /*lint !e605*/
	if (g_local_tsensor_name != NULL) {
		if (strncmp(g_local_tsensor_name, "gpu", strlen(g_local_tsensor_name)) == 0 &&
			    gpuclustercnt == 0)
			return -ENODEV;
		if (strncmp(g_local_tsensor_name, "npu", strlen(g_local_tsensor_name)) == 0 &&
			    npuclustercnt == 0)
			return -ENODEV;
	}
	return 0;
}
#endif

static int get_device_tree_data(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct device_node *of_node = pdev->dev.of_node; /*lint !e578*/
	struct tsens_inf inf = {0};
	int i;
	int rc;
#ifdef CONFIG_THERMAL_SUPPORT_LITE
	struct tsens_lite_inf lite_inf = {0};
	int j = 0;

	get_clustermask_cnt(lite_inf.oncpucluster, MAX_THERMAL_CLUSTER_NUM,
			    &(lite_inf.cpuclustercnt), &(lite_inf.gpuclustercnt),
			    &(lite_inf.npuclustercnt));
#endif
	/* parse tsensor number */
	rc = of_property_read_s32(of_node, "ithermal,sensors", &(inf.tsens_num_sensors));
	if (rc != 0) {
		dev_err(&pdev->dev, "missing sensor number\n");
		return -ENODEV;
	}

	g_tmdev = devm_kzalloc(&pdev->dev, sizeof(struct tsens_tm_device) +
			       inf.tsens_num_sensors *
			       sizeof(struct tsens_tm_device_sensor), GFP_KERNEL);
	if (g_tmdev == NULL) {
		dev_err(&pdev->dev, "kzalloc() failed.\n");
		return -ENOMEM;
	}

	g_tmdev->tsens_num_sensor = inf.tsens_num_sensors;
#ifdef CONFIG_THERMAL_SUPPORT_LITE
	rc = update_sens_num(inf.tsens_num_sensors, lite_inf.cpuclustercnt,
			     lite_inf.gpuclustercnt, lite_inf.npuclustercnt);
	if (rc != 0)
		return -ENODEV;
#endif
	for (i = 0; i < inf.tsens_num_sensors; i++) {
#ifdef CONFIG_THERMAL_SUPPORT_LITE
		rc = lite_sensor_exist(of_node, i, lite_inf.cpuclustercnt,
					lite_inf.gpuclustercnt, lite_inf.npuclustercnt);
		if (rc != 0)
			continue;
		rc = of_property_read_string_index(of_node, "ithermal,tsensor_name",
						   i, &g_tsensor_name[j++]); /*lint !e605*/
#else
		rc = of_property_read_string_index(of_node, "ithermal,tsensor_name",
						   i, &g_tsensor_name[i]); /*lint !e605*/
#endif
		if (rc != 0) {
			pr_err("%s g_tsensor_name %d read err\n", __func__, i);
			return -EINVAL;
		}
	}

	rc = of_property_read_u32_array(of_node, "ithermal,tsensor_mode",
					g_tsensor_mode, inf.tsens_num_sensors);
	if (rc != 0) {
		pr_err("%s g_tsensor_mode read err\n", __func__);
		return -EINVAL;
	}

	rc = of_property_read_u32(of_node, "ithermal,tsensor_adc_start_value",
				       &(inf.register_info));
	if (rc != 0) {
		dev_err(dev, "no ithermal,tsensor_adc_start_value!\n");
		return -EINVAL;
	}
	g_tmdev->adc_start_value = inf.register_info;

	rc = of_property_read_u32(of_node, "ithermal,tsensor_adc_end_value",
				       &(inf.register_info));
	if (rc != 0) {
		dev_err(dev, "no ithermal,tsensor_adc_end_value!\n");
		return -EINVAL;
	}
	g_tmdev->adc_end_value = inf.register_info;

	rc = of_property_read_u32(of_node, "ithermal,tsensor_adc_delta_value",
				       &(inf.register_info));
	if (rc != 0) {
		dev_err(dev, "no ithermal,tsensor_adc_delta_value!\n");
		g_tmdev->adc_delta_value = 0;
	} else {
		g_tmdev->adc_delta_value = inf.register_info;
	}

	rc = of_property_read_u32(of_node, "ithermal,pvtsensor_adc_start_value",
				  &(inf.register_info));
	if (rc != 0) {
		dev_err(dev, "no ithermal,pvtsensor_adc_start_value!\n");
		g_tmdev->pvtsensor_adc_start_value = 0;
	} else {
		g_tmdev->pvtsensor_adc_start_value = inf.register_info;
	}

	rc = of_property_read_u32(of_node, "ithermal,pvtsensor_adc_end_value",
				  &(inf.register_info));
	if (rc != 0) {
		dev_err(dev, "no ithermal,pvtsensor_adc_end_value!\n");
		g_tmdev->pvtsensor_adc_end_value = 0;
	} else {
		g_tmdev->pvtsensor_adc_end_value = inf.register_info;
	}

	g_tmdev->pdev = pdev;
	return 0;
}

static int tsens_tm_probe(struct platform_device *pdev)
{
	int rc = 0;

	if (g_tmdev != NULL) {
		dev_err(&pdev->dev, "TSENS device already in use\n");
		return -EBUSY;
	}

	if (pdev->dev.of_node != NULL) {
		rc = get_device_tree_data(pdev);
		if (rc != 0) {
			dev_err(&pdev->dev, "Error reading TSENS DT\n");
			return rc;
		}
	} else {
		return -ENODEV;
	}

	platform_set_drvdata(pdev, g_tmdev);

	return 0;
}

static void get_detect_tsensor(struct platform_device *pdev, int i)
{
	struct device_node *node = pdev->dev.of_node;
	char reg_no_name[MAX_SENSOR_NUM] = {0};
	char sensor_type_name[MAX_SENSOR_NUM] = {0};
	int reg_no = 0;
	int sensor_type = 0;
	int rc;

	rc = snprintf_s(reg_no_name, sizeof(reg_no_name),
			(sizeof(reg_no_name) - 1),
			"ithermal,detect_%s_regno", g_tsensor_name[i]);
	if (rc < 0)
		dev_err(&pdev->dev, "snprintf_s for regno fail!\n");
	rc = of_property_read_s32(node, reg_no_name, &reg_no);
	if (rc != 0) {
		dev_err(&pdev->dev, "%s node not found!\n", reg_no_name);
		reg_no = -1;
	}
	g_tmdev->sensor[i].reg_no = reg_no;

	(void)memset_s(sensor_type_name, sizeof(sensor_type_name),
		       0, sizeof(sensor_type_name));
	rc = snprintf_s(sensor_type_name, sizeof(sensor_type_name),
			(sizeof(sensor_type_name) - 1),
			"ithermal,detect_%s_sensortype",
			g_tsensor_name[i]);
	if (rc < 0)
		dev_err(&pdev->dev, "snprintf_s for sensortype fail!\n");
	rc = of_property_read_s32(node, sensor_type_name, &sensor_type);
	if (rc != 0) {
		dev_err(&pdev->dev, "%s node not found, use default type!\n",
			sensor_type_name);
		sensor_type = 0; /* default tsensor */
	}
	g_tmdev->sensor[i].sensor_type = sensor_type;
}

#ifdef CONFIG_THERMAL_TRIP
static void get_node_property(struct platform_device *pdev, int i,
			      struct device_node *np)
{
	s32 temp_throttling, temp_shutdown, temp_below_vr_min;
	int rc;

	rc = of_property_read_s32(np, "temp_throttling",
				  &temp_throttling);
	if (rc != 0) {
		dev_err(&pdev->dev, "temp_throttling node not found!\n");
		temp_throttling = 0;
	}
	g_tmdev->sensor[i].temp_throttling = temp_throttling;

	rc = of_property_read_s32(np, "temp_shutdown",
				  &temp_shutdown);
	if (rc != 0) {
		dev_err(&pdev->dev, "temp_shutdown node not found!\n");
		temp_shutdown = 0;
	}
	g_tmdev->sensor[i].temp_shutdown = temp_shutdown;

	rc = of_property_read_s32(np, "temp_below_vr_min",
				  &temp_below_vr_min);
	if (rc != 0) {
		dev_err(&pdev->dev, "temp_below_vr_min node not found!\n");
		temp_below_vr_min = 0;
	}
	g_tmdev->sensor[i].temp_below_vr_min = temp_below_vr_min;
}
#endif

static int _tsens_register_thermal(void)
{
	int rc, i, j;
	unsigned int trips_num = 0;
#ifdef CONFIG_THERMAL_TRIP
	struct device_node *np = NULL;
	char node_name[30] = {0};
#endif

	if (g_tmdev == NULL) {
		pr_err("TSENS early init not done!\n");
		return -ENODEV;
	}

	for (i = 0, j = 0; i < g_tmdev->tsens_num_sensor; i++, j++) {
		unsigned int mask = 0;

		g_tmdev->sensor[i].mode = g_tsensor_mode[i];
		g_tmdev->sensor[i].sensor_num = (unsigned int)i;
		get_detect_tsensor(g_tmdev->pdev, i);
#ifdef CONFIG_THERMAL_TRIP
		(void)memset_s((void *)node_name, sizeof(node_name), 0,
			       sizeof(node_name));
		rc = snprintf_s(node_name, sizeof(node_name),
				(sizeof(node_name) - 1), "ithermal_tsens_%s",
				g_tsensor_name[i]);
		if (rc < 0)
			dev_err(&g_tmdev->pdev->dev, "snprintf_s for nodename fail!\n");
		np = of_find_node_by_name(g_tmdev->pdev->dev.of_node, node_name);
		if (np != NULL) {
			get_node_property(g_tmdev->pdev, i, np);
			of_node_put(np);
			trips_num = TSENS_TRIP_NUM;
		} else {
			trips_num = TSENS_TRIP_ORIGNUM;
		}
#else
		trips_num = TSENS_TRIP_ORIGNUM;
#endif
		mask |= (unsigned int)((1 << trips_num) - 1);

		g_tmdev->sensor[i].tz_dev =
			thermal_zone_device_register(g_tsensor_name[i],
						     trips_num, (int)mask,
						     &g_tmdev->sensor[i],
						     &tsens_thermal_zone_ops,
						     NULL, 0, 0);
		if (IS_ERR(g_tmdev->sensor[i].tz_dev)) {
			dev_err(&g_tmdev->pdev->dev, "Tsensor \
				thermal_zone_device_register failed\n");
			rc = -ENODEV;
			goto register_fail;
		}
	}

	platform_set_drvdata(g_tmdev->pdev, g_tmdev);

	return 0;

register_fail:
	for (i = 0; i < j; i++)
		thermal_zone_device_unregister(g_tmdev->sensor[i].tz_dev);
	return rc;
}

static int tsens_tm_remove(struct platform_device *pdev)
{
	int i;
	struct tsens_tm_device *tmdev = platform_get_drvdata(pdev);

	if (tmdev == NULL)
		return -1;

	for (i = 0; i < tmdev->tsens_num_sensor; i++)
		thermal_zone_device_unregister(tmdev->sensor[i].tz_dev);

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static struct of_device_id tsens_match[] = {
	{ .compatible = "ithermal,tsens", },
	{}
};

static struct platform_driver tsens_tm_driver = {
	.probe = tsens_tm_probe,
	.remove = tsens_tm_remove,
	.driver = {
		.name = "ithermal-tsens",
		.owner = THIS_MODULE,
		.of_match_table = tsens_match,
	},
};

static int __init tsens_tm_init_driver(void)
{
	return platform_driver_register(&tsens_tm_driver);
}
arch_initcall(tsens_tm_init_driver);

static int __init tsens_thermal_register(void)
{
	return _tsens_register_thermal();
}
module_init(tsens_thermal_register);

static void __exit _tsens_tm_remove(void)
{
	platform_driver_unregister(&tsens_tm_driver);
}
module_exit(_tsens_tm_remove);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("thermal tsens module driver");
