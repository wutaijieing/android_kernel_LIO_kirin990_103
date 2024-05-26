/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: Device driver for regulators in PMIC IC
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/debugfs.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/regmap.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <securec.h>
#include <linux/version.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <platform_include/basicplatform/linux/pr_log.h>

#include "regulator_debug.h"

#define PR_LOG_TAG PMIC_REGULATOR_TAG
#define brand_debug(args...)	pr_debug(args)

#define TIME_CHANGE_US_MS 1000

#define PMIC_ECO_MODE_ENABLE		1
#define PMIC_ECO_MODE_DISABLE		0
#define READ_SIZE 2

struct regulator_register_info {
	u32 ctrl_reg;
	u32 enable_mask;
	u32 eco_mode_mask;
	u32 vset_reg;
	u32 vset_mask;
};

struct regmap_regulator {
	const char *name;
	struct regulator_register_info register_info;
	struct timespec64 last_off_time;
	u32 off_on_delay;
	u32 eco_uA;
	struct regulator_desc rdesc;
	int (*dt_parse)(struct regmap_regulator *, struct platform_device *);
};
static DEFINE_MUTEX(enable_mutex);

/*
 * helper function to ensure when it returns it is at least 'delay_us'
 * microseconds after 'since'.
 */
static void ensured_time_after(struct timespec64 since, u32 delay_us)
{
	struct timespec64 now;
	u64 elapsed_ns64, delay_ns64;
	u32 actual_us32;

	delay_ns64 = delay_us * NSEC_PER_USEC;
	ktime_get_real_ts64(&now);
	elapsed_ns64 = (now.tv_sec * NSEC_PER_SEC + now.tv_nsec) -
			(since.tv_sec * NSEC_PER_SEC +  since.tv_nsec);
	if (delay_ns64 > elapsed_ns64) {
		actual_us32 = ((u32)(delay_ns64 - elapsed_ns64) /
							NSEC_PER_USEC);
		if (actual_us32 >= TIME_CHANGE_US_MS) {
			mdelay((u32)(actual_us32 / TIME_CHANGE_US_MS));
			udelay(actual_us32 % TIME_CHANGE_US_MS);
		} else if (actual_us32 > 0) {
			udelay(actual_us32);
		}
	}
}

static int regmap_regulator_is_enabled(struct regulator_dev *dev)
{
	struct regmap_regulator *sreg = rdev_get_drvdata(dev);
	struct regmap *regmap = dev->regmap;
	u32 reg_val = 0;
	int ret;

	if (!sreg || !regmap)
		return 0;

	ret = regmap_read(regmap, sreg->register_info.ctrl_reg, &reg_val);
	if (ret < 0) {
		pr_err("%s read ctrl_reg 0x%x failed\n", __func__,
			sreg->register_info.ctrl_reg);
		return ret;
	}

	brand_debug("<[%s]: ctrl_reg=0x%x,enable_state=%d>\n", __func__,
			sreg->register_info.ctrl_reg,
			(reg_val & sreg->register_info.enable_mask));
	return ((reg_val & sreg->register_info.enable_mask) != 0);
}

static int regmap_regulator_enable(struct regulator_dev *dev)
{
	struct regmap_regulator *sreg = rdev_get_drvdata(dev);
	struct regmap *regmap = dev->regmap;
	int ret;

	if (!sreg || !regmap)
		return -EINVAL;

	/* keep a distance of off_on_delay from last time disabled */
	ensured_time_after(sreg->last_off_time, sreg->off_on_delay);

	brand_debug("<[%s]: off_on_delay = %d us>\n", __func__, sreg->off_on_delay);

	/* cannot enable more than one regulator at one time */
	mutex_lock(&enable_mutex);

	/* set enable register */
	ret = regmap_write_bits(regmap, sreg->register_info.ctrl_reg,
				sreg->register_info.enable_mask,
				sreg->register_info.enable_mask);
	if (ret < 0) {
		pr_err("%s regmap_write_bits ctrl_reg 0x%x failed\n", __func__,
			sreg->register_info.ctrl_reg);
		mutex_unlock(&enable_mutex);
		return ret;
	}

	brand_debug("<[%s]: ctrl_reg=0x%x,enable_mask=0x%x>\n",
			__func__, sreg->register_info.ctrl_reg,
			sreg->register_info.enable_mask);

	mutex_unlock(&enable_mutex);

	return 0;
}

static int regmap_regulator_disable(struct regulator_dev *dev)
{
	struct regmap_regulator *sreg = rdev_get_drvdata(dev);
	struct regmap *regmap = dev->regmap;
	int ret;

	if (!sreg || !regmap)
		return -EINVAL;
	/* set enable register to 0 */
	ret = regmap_write_bits(regmap, sreg->register_info.ctrl_reg,
				sreg->register_info.enable_mask, 0);
	if (ret < 0) {
		pr_err("%s regmap_write_bits ctrl_reg 0x%x failed\n", __func__,
			sreg->register_info.ctrl_reg);
		return ret;
	}

	ktime_get_real_ts64(&sreg->last_off_time);

	return 0;
}

static int regmap_regulator_get_voltage(struct regulator_dev *dev)
{
	struct regmap_regulator *sreg = rdev_get_drvdata(dev);
	struct regmap *regmap = dev->regmap;
	u32 reg_val = 0;
	u32 selector;
	int ret;
	int volt;

	if (!sreg || !regmap)
		return -EINVAL;

	/* get voltage selector */
	ret = regmap_read(regmap, sreg->register_info.vset_reg, &reg_val);
	if (ret < 0) {
		pr_err("%s read vset_reg 0x%x failed\n", __func__,
			sreg->register_info.vset_reg);
		return ret;
	}

	selector = (reg_val & sreg->register_info.vset_mask) >>
			((unsigned int)ffs(sreg->register_info.vset_mask) - 1);

	volt = sreg->rdesc.ops->list_voltage(dev, selector);

	brand_debug("<[%s]: vset_reg 0x%x = 0x%x, volt %d>\n", __func__,
				sreg->register_info.vset_reg, reg_val, volt);

	return volt;
}

static int regmap_regulator_set_voltage(struct regulator_dev *dev,
				int min_uv, int max_uv, unsigned int *selector)
{
	struct regmap_regulator *sreg = rdev_get_drvdata(dev);
	struct regmap *regmap = dev->regmap;
	u32 vsel;
	int uv;
	int ret;

	if (!sreg || !regmap)
		return -EINVAL;

	for (vsel = 0; vsel < sreg->rdesc.n_voltages; vsel++) {
		uv = sreg->rdesc.volt_table[vsel];
		/* Break at the first in-range value */
		if (min_uv <= uv && uv <= max_uv)
			break;
	}

	/* unlikely to happen. sanity test done by regulator core */
	if (unlikely(vsel == sreg->rdesc.n_voltages))
		return -EINVAL;

	*selector = vsel;
	/* set voltage selector */
	ret = regmap_write_bits(regmap, sreg->register_info.vset_reg,
		sreg->register_info.vset_mask,
		vsel << ((unsigned int)ffs(sreg->register_info.vset_mask) - 1));
	if (ret < 0) {
		pr_err("%s regmap_write_bits vset reg 0x%x failed\n", __func__,
			sreg->register_info.vset_reg);
		return ret;
	}

	brand_debug("<[%s]: vset_reg=0x%x, vset_mask=0x%x, value=0x%x>\n",
			__func__, sreg->register_info.vset_reg,
			sreg->register_info.vset_mask,
			vsel << (ffs(sreg->register_info.vset_mask) - 1));

	return 0;
}

static unsigned int regmap_regulator_get_mode(struct regulator_dev *dev)
{
	struct regmap_regulator *sreg = rdev_get_drvdata(dev);
	struct regmap *regmap = dev->regmap;
	u32 reg_val = 0;
	int ret;

	if (!sreg || !regmap)
		return 0;

	ret = regmap_read(regmap, sreg->register_info.ctrl_reg, &reg_val);
	if (ret < 0) {
		pr_err("%s regmap read ctrl reg 0x%x failed\n", __func__,
			sreg->register_info.ctrl_reg);
		return 0;
	}

	brand_debug("<[%s]: reg_val=%d, ctrl_reg=0x%x, eco_mode_mask=0x%x>\n",
		__func__, reg_val,
		sreg->register_info.ctrl_reg,
		sreg->register_info.eco_mode_mask);

	if (reg_val & sreg->register_info.eco_mode_mask)
		return REGULATOR_MODE_IDLE;
	else
		return REGULATOR_MODE_NORMAL;
}

static int regmap_regulator_set_mode(struct regulator_dev *dev,
						unsigned int mode)
{
	struct regmap_regulator *sreg = rdev_get_drvdata(dev);
	struct regmap *regmap = dev->regmap;
	u32 eco_mode;
	int ret;

	if (!sreg || !regmap)
		return -EINVAL;

	switch (mode) {
	case REGULATOR_MODE_NORMAL:
		eco_mode = PMIC_ECO_MODE_DISABLE;
		break;
	case REGULATOR_MODE_IDLE:
		eco_mode = PMIC_ECO_MODE_ENABLE;
		break;
	default:
		return -EINVAL;
	}

	/* set mode */
	ret = regmap_write_bits(regmap, sreg->register_info.ctrl_reg,
		sreg->register_info.eco_mode_mask,
		eco_mode << ((unsigned int)ffs(sreg->register_info.eco_mode_mask) - 1));
	if (ret < 0) {
		pr_err("%s regmap write_bits ctrl reg 0x%x failed\n", __func__,
			sreg->register_info.ctrl_reg);
		return ret;
	}

	brand_debug("<[%s]: ctrl_reg=0x%x, eco_mode_mask=0x%x, value=0x%x>\n",
		__func__, sreg->register_info.ctrl_reg,
		sreg->register_info.eco_mode_mask,
		eco_mode << (ffs(sreg->register_info.eco_mode_mask) - 1));
	return 0;
}


static unsigned int regmap_regulator_get_optimum_mode(struct regulator_dev *dev,
			int input_uV, int output_uV, int load_uA)
{
	struct regmap_regulator *sreg = rdev_get_drvdata(dev);

	if (!sreg)
		return 0;

	if ((load_uA == 0) || ((unsigned int)load_uA > sreg->eco_uA))
		return REGULATOR_MODE_NORMAL;
	else
		return REGULATOR_MODE_IDLE;
}

static int regmap_dt_select(
	struct regmap_regulator *sreg, struct platform_device *pdev,
	struct device_node *np)
{
	struct device *dev = &pdev->dev;
	unsigned int register_info[3] = {0}; /* the length is determined by dts parameter */
	u32 chip_version, version_1, version_2;
	struct regmap *reg_map = NULL;
	int ret;
	unsigned int reg_val = 0;

	/* if np no "hisilicon,select" node, just return */
	ret = of_property_read_u32_array(np, "hisilicon,select",
						register_info, 3);
	if (ret != 0)
		return 0;

	chip_version = register_info[0];
	version_1 = register_info[1];
	version_2 = register_info[2];
	reg_map = dev_get_regmap(dev->parent, NULL);
	if (reg_map == NULL) {
		dev_err(dev, "pdev regmap get failed\n");
		return -ENODEV;
	}

	ret = regmap_read(reg_map, chip_version, &reg_val);
	if (ret < 0) {
		dev_err(dev, "pdev regmap read chip version failed\n");
		return ret;
	}

	if (reg_val == version_1) {
		/* sub pmu is pmic6422v300 */
		ret = of_property_read_u32_array(np,
			"pmic-ctrl-1", register_info, 2);
		if (ret) {
			dev_err(dev, "no sub-spmi-ctrl-1 property set\n");
			return ret;
		}
		sreg->register_info.ctrl_reg = register_info[0];
		sreg->register_info.enable_mask = register_info[1];
		ret = of_property_read_u32_array(np,
			"pmic-vset-1", register_info, 2);
		if (ret) {
			dev_err(dev, "no sub-spmi-vset-1 property set\n");
			return ret;
		}
		sreg->register_info.vset_reg = register_info[0];
		sreg->register_info.vset_mask = register_info[1];
	} else if (reg_val == version_2){
		/* sub pmu is pmic6422v500 */
		return 0;
	} else {
		dev_err(dev, "sub-spmi select property not Match\n");
		return -ENODEV;
	}

	return 0;
}

static int regmap_dt_parse_common(struct regmap_regulator *sreg,
					struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct regulator_desc *rdesc = &sreg->rdesc;
	unsigned int register_info[3] = {0}; /* the length is determined by dts parameter */
	int ret;

	/* parse .register_info.ctrl_reg */
	ret = of_property_read_u32_array(np, "pmic-ctrl",
						register_info, READ_SIZE);
	if (ret) {
		dev_err(dev, "no pmic-ctrl property set\n");
		return ret;
	}
	sreg->register_info.ctrl_reg = register_info[0];
	sreg->register_info.enable_mask = register_info[1];
	sreg->register_info.eco_mode_mask = register_info[2];

	/* parse .register_info.vset_reg */
	ret = of_property_read_u32_array(np, "pmic-vset",
						register_info, READ_SIZE);
	if (ret) {
		dev_err(dev, "no pmic-vset property set\n");
		return ret;
	}
	sreg->register_info.vset_reg = register_info[0];
	sreg->register_info.vset_mask = register_info[1];

	/* parse .register_info.chip_version for  distinguishing chip version */
	ret = regmap_dt_select(sreg, pdev, np);
	if (ret)
		return ret;

	/* parse .off-on-delay */
	ret = of_property_read_u32(np, "pmic-off-on-delay-us",
						&sreg->off_on_delay);
	if (ret) {
		dev_err(dev, "no pmic-off-on-delay-us property set\n");
		return ret;
	}

	/* parse .enable_time */
	ret = of_property_read_u32(np, "pmic-enable-time-us",
				   &rdesc->enable_time);
	if (ret) {
		dev_err(dev, "no pmic-enable-time-us property set\n");
		return ret;
	}

	/* parse .eco_uA */
	ret = of_property_read_u32(np, "pmic-eco-microamp",
				   &sreg->eco_uA);
	if (ret) {
		sreg->eco_uA = 0;
		ret = 0;
	}
	return ret;
}
static int fake_of_get_regulator_constraint(struct regulation_constraints *constraints,
						struct device_node *np)
{
	unsigned int temp_modes = 0;
	int ret;

	/* regulator supports two modes */
	ret = of_property_read_u32_array(np, "pmic-valid-modes-mask",
					&(constraints->valid_modes_mask), 1);
	if (ret) {
		pr_err("no pmic-valid-modes-mask property set\n");
		ret = -ENODEV;
		return ret;
	}
	ret = of_property_read_u32_array(np, "pmic-valid-idle-mask",
						&temp_modes, 1);
	if (ret) {
		pr_err("no pmic-valid-idle-mask property set\n");
		ret = -ENODEV;
		return ret;
	}
	constraints->valid_ops_mask |= temp_modes;
	return ret;
}
static int regmap_dt_parse_ldo(struct regmap_regulator *sreg,
				struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct regulator_desc *rdesc = &sreg->rdesc;
	unsigned int *v_table = NULL;
	int ret;

	/* parse .n_voltages, and .volt_table */
	ret = of_property_read_u32(np, "pmic-n-voltages",
				   &rdesc->n_voltages);
	if (ret) {
		dev_err(dev, "no pmic-n-voltages property set\n");
		return ret;
	}
	/* alloc space for .volt_table */
	v_table = devm_kzalloc(
		dev, sizeof(unsigned int) * rdesc->n_voltages, GFP_KERNEL);
	if (!v_table) {
		ret = -ENOMEM;
		dev_err(dev, "no memory for .volt_table\n");
		return ret;
	}
	ret = of_property_read_u32_array(np, "pmic-vset-table",
						v_table, rdesc->n_voltages);
	if (ret) {
		dev_err(dev, "no pmic-vset-table property set\n");
		devm_kfree(dev, v_table);
		return ret;
	}
	rdesc->volt_table = v_table;

	/* parse regulator's dt common part */
	ret = regmap_dt_parse_common(sreg, pdev);
	if (ret) {
		dev_err(dev, "failure in regmap_dt_parse_common\n");
		devm_kfree(dev, v_table);
		return ret;
	}
	return ret;
}

static struct regulator_ops pmic_ldo_rops = {
	.is_enabled = regmap_regulator_is_enabled,
	.enable = regmap_regulator_enable,
	.disable = regmap_regulator_disable,
	.list_voltage = regulator_list_voltage_table,
	.get_voltage = regmap_regulator_get_voltage,
	.set_voltage = regmap_regulator_set_voltage,
	.get_mode = regmap_regulator_get_mode,
	.set_mode = regmap_regulator_set_mode,
	.get_optimum_mode = regmap_regulator_get_optimum_mode,
};

static const struct regmap_regulator regmap_regulator_ldo = {
	.rdesc = {
	.ops = &pmic_ldo_rops,
		.type = REGULATOR_VOLTAGE,
		.owner = THIS_MODULE,
	},
	.dt_parse = regmap_dt_parse_ldo,
};

static const struct of_device_id of_regmap_regulator_match_tbl[] = {
	{
		.compatible = "pmic-ldo",
		.data = &regmap_regulator_ldo,
	},
	{  }   /* end */
};

static struct regmap_regulator *regulator_rdesc_prepare(
										struct platform_device *pdev,
										struct regulator_init_data *initdata)
{
	struct device *dev = &pdev->dev;
	const struct of_device_id *match = NULL;
	struct regmap_regulator *sreg = NULL;
	const struct regmap_regulator *template = NULL;
	struct regulator_desc *rdesc = NULL;
	int ret;

	/* to check which type of regulator this is */
	match = of_match_device(of_regmap_regulator_match_tbl, &pdev->dev);
	if (!match) {
		dev_err(dev, "get regulator fail\n");
		return NULL;
	}
	template = match->data;

	sreg = kmemdup(template, sizeof(*sreg), GFP_KERNEL);
	if (!sreg) {
		pr_err("template kememdup is fail\n");
		return NULL;
	}

	sreg->name = initdata->constraints.name;
	rdesc = &sreg->rdesc;
	rdesc->name = sreg->name;
	rdesc->min_uV = initdata->constraints.min_uV;

	/* to parse device tree data for regulator specific */
	ret = sreg->dt_parse(sreg, pdev);
	if (ret) {
		dev_err(dev, "device tree parameter parse error!\n");
		kfree(sreg);
		return NULL;
	}

	return sreg;
}

static struct regulator_init_data *regulator_initdata_prepare(
								struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct regulation_constraints *constraint = NULL;
	struct regulator_init_data *initdata = NULL;
	const char *supplyname = NULL;
	int ret;

	initdata = of_get_regulator_init_data(dev, np, NULL);
	if (!initdata) {
		dev_err(dev, "%s get regulator init data error\n", __func__);
		return NULL;
	}

	supplyname = of_get_property(np, "hisilicon,supply_name", NULL);
	if (supplyname)
		initdata->supply_regulator = supplyname;

	/* regulator supports two modes */
	constraint = &initdata->constraints;
	ret = fake_of_get_regulator_constraint(constraint, np);
	if (ret) {
		dev_err(dev, "%s get regulator constraint fail %d\n", __func__, ret);
		return NULL;
	}

	return initdata;
}

static int regmap_regulator_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct regulator_dev *rdev = NULL;
	struct regulator_init_data *initdata = NULL;
	struct regmap_regulator *sreg = NULL;
	struct regulator_config config;
	int ret;

	ret = memset_s(&config, sizeof(struct regulator_config),
				0, sizeof(struct regulator_config));
	if (ret != EOK) {
		dev_err(dev, "%s memset_s config fail\n", __func__);
		return ret;
	}

	initdata = regulator_initdata_prepare(pdev);
	if (!initdata) {
		dev_err(dev, "%s initdata prepare fail %d\n", __func__, ret);
		return -1;
	}

	sreg = regulator_rdesc_prepare(pdev, initdata);
	if (!sreg) {
		dev_err(dev, "%s initdata prepare fail %d\n", __func__, ret);
		return -1;
	}

	config.dev = &pdev->dev;
	config.init_data = initdata;
	config.driver_data = sreg;
	config.of_node = pdev->dev.of_node;

	/* register regulator */
	rdev = regulator_register(&sreg->rdesc, &config);
	if (IS_ERR(rdev)) {
		dev_err(dev, "failed to register %s\n",	sreg->rdesc.name);
		ret = PTR_ERR(rdev);
		goto probe_end;
	}

	dev_set_drvdata(dev, rdev);

probe_end:
	if (ret)
		kfree(sreg);

	return ret;
}

static int regmap_regulator_remove(struct platform_device *pdev)
{
	struct regulator_dev *rdev = NULL;
	struct regmap_regulator *sreg = NULL;

	rdev = dev_get_drvdata(&pdev->dev);
	if (!rdev) {
		pr_err("%s:rdev is NULL\n", __func__);
		return -ENOMEM;
	}

	sreg = rdev_get_drvdata(rdev);
	if (!sreg) {
		pr_err("%s:sreg is NULL\n", __func__);
		return -ENOMEM;
	}

	regulator_unregister(rdev);

	if (sreg->rdesc.volt_table)
		devm_kfree(&pdev->dev, (unsigned int *)sreg->rdesc.volt_table);

	kfree(sreg);
	return 0;
}

static const struct platform_device_id regulator_id_tbl[] = {
	{ "pmic-ldo", 0 },
	{},
};

static struct platform_driver regmap_pmic_driver = {
	.driver = {
		.name	= "regmap_regulator",
		.owner  = THIS_MODULE,
		.of_match_table = of_regmap_regulator_match_tbl,
	},
	.id_table = regulator_id_tbl,
	.probe	= regmap_regulator_probe,
	.remove	= regmap_regulator_remove,
};

static int __init regmap_regulator_init(void)
{
	return platform_driver_register(&regmap_pmic_driver);
}

static void __exit regmap_regulator_exit(void)
{
	platform_driver_unregister(&regmap_pmic_driver);
}

fs_initcall(regmap_regulator_init);
module_exit(regmap_regulator_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("regulator regmap driver");
