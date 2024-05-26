/*
 * modem_hkadc_temp.c
 *
 * hkadc module for component
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
#include <asm/page.h>
#include "modem_hkadc_temp.h"

#define DEFULT_ERROR_VALUE	(-40000)

static struct ithermal_mdm_adc_chip *g_tm_dev = NULL;
static struct ithermal_mdm_get_adc *g_mdm_adc = NULL;

struct mdm_adc_s {
	unsigned int channel;
	unsigned int mode;
};

struct ithermal_mdm_get_adc {
	int (*get_value_func)(struct mdm_adc_s *mdm_adc_para);
};

static int ithermal_mdm_adc_get_value_default(struct mdm_adc_s *mdm_adc_para)
{
	pr_info("%s return the default value\n", __func__);
	return (int)DEFULT_ERROR_VALUE;
}

int mdm_adc_get_value_register(int (*func)(struct mdm_adc_s *mdm_adc_para))
{
	if (func == NULL)
		return -EINVAL;

	if (g_mdm_adc == NULL) {
		g_mdm_adc = (struct ithermal_mdm_get_adc *)
			kzalloc(sizeof(struct ithermal_mdm_get_adc), GFP_KERNEL);
		if (g_mdm_adc == NULL)
			return -EINVAL;
	}
	g_mdm_adc->get_value_func = func;

	return 0;
}
EXPORT_SYMBOL(mdm_adc_get_value_register);

#ifndef CONFIG_AB_APCP_NEW_INTERFACE
int hisi_mdm_adc_get_value_register(int (*func)(struct mdm_adc_s *mdm_adc_para))
{
	return mdm_adc_get_value_register(func);
}
EXPORT_SYMBOL(hisi_mdm_adc_get_value_register);
#endif

static int ithermal_get_mdm_temp(struct thermal_zone_device *thermal, int *temp)
{
	struct mdm_adc_t *mdm_sensor = thermal->devdata;
	int ret, volt;
	struct mdm_adc_s mdm_adc;

	if (mdm_sensor == NULL || temp == NULL || g_mdm_adc == NULL)
		return -EINVAL;

	mdm_adc.channel = mdm_sensor->channel;
	mdm_adc.mode = mdm_sensor->mode;
	volt = g_mdm_adc->get_value_func(&mdm_adc);
	/* mdm has not registered */
	if (volt == DEFULT_ERROR_VALUE) {
		*temp = (int)DEFULT_ERROR_VALUE;
		return 0;
	}
	/* 0-0xe is adc channel; 0xf is xo channel */
	if (mdm_adc.channel <= 0xf) {
		if (volt <= 0) {
			pr_err("AP get mdm adc volt value is fail, "
			       "chan %d volt %d!\n",
			       mdm_sensor->channel, volt);
			*temp = (int)DEFULT_ERROR_VALUE;
			return 0;
		}

		ret = mdm_ntc_2_temp(mdm_sensor, temp, volt);
		if (ret < 0) {
			pr_err("%s get temp failed ret = %d\n", __func__, ret);
			return ret;
		}
	/* 0x10-0x13 is mdm tsensor, volt means temperature */
	} else if (mdm_adc.channel <= 0x13) {
		if (volt <= -30000) { /* -30000 means invalid volt */
			pr_err("AP get mdm tsensor value is fail,chan %d tsensor %d\n",
			       mdm_sensor->channel, volt);
			*temp = (int)DEFULT_ERROR_VALUE;
			return 0;
		}
		*temp = volt;
	/* 0x14 is mdm ddr */
	} else if (mdm_adc.channel == 0x14) {
		if (volt < 0) {
			pr_err("AP get mdm ddr volt value is fail,chan %d volt %d!\n",
			       mdm_sensor->channel, volt);
			*temp = (int)DEFULT_ERROR_VALUE;
			return 0;
		}
		*temp = volt;
	} else { /* others */
		pr_err("%s wrong channel chan %d\n", __func__,
		       mdm_sensor->channel);
		return -EINVAL;
	}

	return 0;
}

/*lint -e785*/
static struct thermal_zone_device_ops mdm_thermal_zone_ops = {
	.get_temp = ithermal_get_mdm_temp,
};

/*lint +e785*/
static int ithermal_mdm_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *dev_node = dev->of_node;
	int ret, sensor_count;
	int i = 0;
	struct device_node *np = NULL;
	struct device_node *child = NULL;
	int mdm_channel = 0;
	int mdm_mode = 0;

	pr_info("enter %s\n", __func__);

	if (!of_device_is_available(dev_node)) {
		dev_err(dev, "mdm dev not found\n");
		ret = -ENODEV;
		goto exit;
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

	g_tm_dev = devm_kzalloc(dev, sizeof(struct ithermal_mdm_adc_chip) +
			       sensor_count * sizeof(struct mdm_adc_t),
			       GFP_KERNEL);
	if (g_tm_dev == NULL) {
		pr_err("%s: kzalloc() failed\n", __func__);
		ret = -ENOMEM;
		goto node_put;
	}
	g_tm_dev->tsens_num_sensor = sensor_count;

	for_each_child_of_node(np, child) {
		ret = of_property_read_u32(child, "channel", &mdm_channel);
		if (ret != 0) {
			dev_err(dev, "canot get %d mdm ntc channel\n", i);
			ret = -ENODEV;
			goto read_channel_fail;
		}
		g_tm_dev->mdm_sensor[i].channel = mdm_channel;

		ret = of_property_read_u32(child, "mode", &mdm_mode);
		if (ret != 0) {
			dev_err(dev, "canot get %d mdm ntc mode\n", i);
			ret = -ENODEV;
			goto read_mode_fail;
		}
		g_tm_dev->mdm_sensor[i].mode = mdm_mode;

		ret = of_property_read_string(child, "type",
					      &(g_tm_dev->mdm_sensor[i].name));
		if (ret != 0) {
			pr_err("%s type read err\n", __func__);
			goto read_type_fail;
		}

		ret = of_property_read_string(child, "tm_ntc_table",
					      &(g_tm_dev->mdm_sensor[i].ntc_name));
		if (ret != 0) {
			dev_err(dev, "detect %d tm_ntc_table name failed\n", i);
			ret = -EINVAL;
			goto read_ntc_table_fail;
		}

		g_tm_dev->mdm_sensor[i].tz_dev =
			thermal_zone_device_register(g_tm_dev->mdm_sensor[i].name, 0,
						     0, &g_tm_dev->mdm_sensor[i],
						     &mdm_thermal_zone_ops, NULL, 0, 0);
		if (IS_ERR(g_tm_dev->mdm_sensor[i].tz_dev)) {
			dev_err(dev, "register thermal zone for mdm failed.\n");
			ret = -ENODEV;
			goto unregister;
		}
		i++;
	}

	of_node_put(np);
	platform_set_drvdata(pdev, g_tm_dev);

	if (g_mdm_adc == NULL)
		 mdm_adc_get_value_register(ithermal_mdm_adc_get_value_default);

	pr_info("%s ok\n", __func__);

	goto exit;

unregister:
	for (; i >= 0; i--)
		thermal_zone_device_unregister(g_tm_dev->mdm_sensor[i].tz_dev);

read_ntc_table_fail:
read_type_fail:
read_mode_fail:
read_channel_fail:
node_put:
	of_node_put(np);
exit:
	return ret;
}

static int ithermal_mdm_remove(struct platform_device *pdev)
{
	struct ithermal_mdm_adc_chip *chip = platform_get_drvdata(pdev);
	int i;

	if (chip != NULL) {
		for (i = 0; i < chip->tsens_num_sensor; i++)
			thermal_zone_device_unregister(chip->mdm_sensor[i].tz_dev);
		platform_set_drvdata(pdev, NULL);
	}

	if (g_mdm_adc != NULL) {
		kfree(g_mdm_adc);
		g_mdm_adc = NULL;
	}

	return 0;
}

static struct of_device_id ithermal_mdm_of_match[] = {
	{ .compatible = "ithermal,mdm_temp" },
	{},
};

MODULE_DEVICE_TABLE(of, ithermal_mdm_of_match);

static struct platform_driver ithermal_mdm_platdrv = {
	.driver = {
		.name = "ithermal_mdm_temp",
		.owner = THIS_MODULE,
		.of_match_table = ithermal_mdm_of_match,
	},
	.probe = ithermal_mdm_probe,
	.remove = ithermal_mdm_remove,
};

static int __init ithermal_mdm_init(void)
{
	return platform_driver_register(&ithermal_mdm_platdrv);
}

static void __exit ithermal_mdm_exit(void)
{
	platform_driver_unregister(&ithermal_mdm_platdrv);
}
module_init(ithermal_mdm_init);
module_exit(ithermal_mdm_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("thermal modem hkadc module driver");
