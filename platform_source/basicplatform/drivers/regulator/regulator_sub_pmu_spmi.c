/*
 * Copyright (c) Hisilicon technologies Co., Ltd.2019-2019. All rights reserved.
 * Description: provide sub regulator functions
 * Create: 2019-03-17
 */
#include <platform_include/basicplatform/linux/spmi_platform.h>
#include <platform_include/basicplatform/linux/of_platform_spmi.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <platform_include/basicplatform/linux/mfd/pmic_platform.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/version.h>
#include <securec.h>
#ifdef CONFIG_PMIC_PLATFORM_DEBUG
#include <linux/debugfs.h>
#endif
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#define PR_LOG_TAG SUB_PMIC_REGULATOR_TAG

#define BRAND_DEBUG(args...)	pr_debug(args)

#define TIME_CHANGE_US_MS 1000

struct sub_regulator_register_info {
	u32 chip_version;
	u32 version_1;
	u32 version_2;
	u32 ctrl_reg;
	u32 enable_mask;
	u32 vset_reg;
	u32 vset_mask;
};

struct sub_pmic_regulator {
	const char *name;
	struct sub_regulator_register_info register_info;
	struct timespec64 last_off_time;
	u32 off_on_delay;
	struct regulator_desc rdesc;
	int (*dt_parse)(struct sub_pmic_regulator *, struct spmi_device *,
		struct device_node *);
};
static DEFINE_MUTEX(enable_mutex);
static struct timespec64 g_last_en;

static inline struct vendor_pmic *rdev_to_pmic(struct regulator_dev *dev)
{
	if ((rdev_get_dev(dev) == NULL) || (rdev_get_dev(dev)->parent == NULL))
		return NULL;

	return dev_get_drvdata(rdev_get_dev(dev)->parent->parent);
}

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
		actual_us32 = ((u32)(delay_ns64 - elapsed_ns64) / NSEC_PER_USEC);
		if (actual_us32 >= TIME_CHANGE_US_MS) {
			mdelay((u32)(actual_us32 / TIME_CHANGE_US_MS));
			udelay(actual_us32 % TIME_CHANGE_US_MS);
		} else if (actual_us32 > 0) {
			udelay(actual_us32);
		}
	}
}

static int sub_pmic_regulator_is_enabled(struct regulator_dev *dev)
{
	u32 reg_val;
	struct sub_pmic_regulator *sreg = rdev_get_drvdata(dev);
	struct vendor_pmic *pmic = rdev_to_pmic(dev);

	if (!sreg)
		return 0;

	reg_val = sub_pmic_read(pmic, sreg->register_info.ctrl_reg);
	BRAND_DEBUG("<[%s]: ctrl_reg=0x%x,enable_state=%d>\n", __func__,
		sreg->register_info.ctrl_reg,
		(reg_val & sreg->register_info.enable_mask));

	return ((reg_val & sreg->register_info.enable_mask) != 0);
}

static int sub_pmic_regulator_enable(struct regulator_dev *dev)
{
	struct sub_pmic_regulator *sreg = rdev_get_drvdata(dev);
	struct vendor_pmic *pmic = rdev_to_pmic(dev);
	u32 reg_val;

	if (!sreg)
		return -EINVAL;

	ensured_time_after(sreg->last_off_time, sreg->off_on_delay);
	BRAND_DEBUG(
		"<[%s]: off_on_delay=%dus>\n", __func__, sreg->off_on_delay);
	mutex_lock(&enable_mutex);
	ensured_time_after(g_last_en, PMIC_REGS_ENA_PROTECT_TIME);

	reg_val = sub_pmic_read(pmic, sreg->register_info.ctrl_reg);
	BRAND_DEBUG("<[%s]: ctrl_reg=0x%x\n", __func__,
		sreg->register_info.ctrl_reg);
	/* set enable register */
	sub_pmic_write(pmic, sreg->register_info.ctrl_reg,
		(reg_val & (~sreg->register_info.enable_mask)) |
			sreg->register_info.enable_mask);
	BRAND_DEBUG("<[%s]: ctrl_reg=0x%x,enable_mask=0x%x>\n", __func__,
		sreg->register_info.ctrl_reg, sreg->register_info.enable_mask);
	ktime_get_real_ts64(&g_last_en);
	mutex_unlock(&enable_mutex);
	return 0;
}

static int sub_pmic_regulator_disable(struct regulator_dev *dev)
{
	struct sub_pmic_regulator *sreg = rdev_get_drvdata(dev);
	struct vendor_pmic *pmic = rdev_to_pmic(dev);
	u32 reg_val;

	if (!sreg)
		return -EINVAL;

	reg_val = sub_pmic_read(pmic, sreg->register_info.ctrl_reg);
	BRAND_DEBUG("<[%s]: ctrl_reg=0x%x\n", __func__,
		sreg->register_info.ctrl_reg);
	/* set enable register to 0 */
	sub_pmic_write(pmic, sreg->register_info.ctrl_reg,
		reg_val & (~sreg->register_info.enable_mask));
	BRAND_DEBUG("<[%s]: disable:ctrl_reg=0x%x,enable_mask=0x%x>\n",
		__func__, sreg->register_info.ctrl_reg,
		sreg->register_info.enable_mask);
	ktime_get_real_ts64(&sreg->last_off_time);

	return 0;
}
static int sub_pmic_regulator_get_voltage(struct regulator_dev *dev)
{
	struct sub_pmic_regulator *sreg = rdev_get_drvdata(dev);
	struct vendor_pmic *pmic = rdev_to_pmic(dev);
	u32 reg_val, selector;

	if (!sreg)
		return -EINVAL;

	/* get voltage selector */
	reg_val = sub_pmic_read(pmic, sreg->register_info.vset_reg);
	BRAND_DEBUG("<[%s]: vset_reg=0x%x>\n", __func__,
		sreg->register_info.vset_reg);

	selector = (reg_val & sreg->register_info.vset_mask) >>
		   ((unsigned int)ffs(sreg->register_info.vset_mask) - 1);

	return sreg->rdesc.ops->list_voltage(dev, selector);
}

static int sub_pmic_regulator_set_voltage(
	struct regulator_dev *dev, int min_uv, int max_uv, u32 *selector)
{
	struct sub_pmic_regulator *sreg = rdev_get_drvdata(dev);
	struct vendor_pmic *pmic = rdev_to_pmic(dev);
	u32 vsel;
	u32 reg_val, bits;
	int uv;

	if (!sreg)
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
	reg_val = sub_pmic_read(pmic, sreg->register_info.vset_reg);
	BRAND_DEBUG("<[%s]: vset_reg=0x%x>\n", __func__,
		sreg->register_info.vset_reg);
	bits = vsel << ((unsigned int)ffs(sreg->register_info.vset_mask) - 1);
	sub_pmic_write(pmic, sreg->register_info.vset_reg,
		((sreg->register_info.vset_mask & bits) |
			(reg_val & ~sreg->register_info.vset_mask)));
	BRAND_DEBUG("<[%s]: vset_reg=0x%x, vset_mask=0x%x, value=0x%x>\n",
		__func__, sreg->register_info.vset_reg,
		sreg->register_info.vset_mask, bits);

	return 0;
}
static int sub_pmic_regulator_dt_select(
	struct sub_pmic_regulator *sreg, struct spmi_device *pdev,
	struct device_node *np)
{
	struct device *dev = &pdev->dev;
	unsigned int register_info[3] = {0}; /* the length is determined by dts parameter */
	int ret;

	ret = of_property_read_u32_array(np, "hisilicon,select",
						register_info, 3);
	if (!ret) {
		sreg->register_info.chip_version = register_info[0];
		sreg->register_info.version_1 = register_info[1];
		sreg->register_info.version_2 = register_info[2];
		if (sub_pmic_reg_read(sreg->register_info.chip_version) !=
					sreg->register_info.version_1) {
			if (sub_pmic_reg_read(sreg->register_info.chip_version) !=
						sreg->register_info.version_2) {
				ret = -ENOMEM;
				dev_err(dev, "sub-spmi select property not Match\n");
				return ret;
			} else {
				/* sub pmu is pmic6422v500 */
				ret = 0;
				return ret;
			}
		} else {
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
		}
	}
	return 0;
}

static int sub_pmic_regulator_dt_common(
	struct sub_pmic_regulator *sreg, struct spmi_device *pdev,
	struct device_node *np)
{
	struct device *dev = &pdev->dev;
	struct regulator_desc *rdesc = &sreg->rdesc;
	unsigned int register_info[2] = {0}; /* the length is determined by dts parameter */
	int ret;

	/* parse .register_info.ctrl_reg */
	ret = of_property_read_u32_array(np, "pmic-ctrl",
					register_info, 2);
	if (ret) {
		dev_err(dev, "no sub-spmi-ctrl property set\n");
		return ret;
	}
	sreg->register_info.ctrl_reg = register_info[0];
	sreg->register_info.enable_mask = register_info[1];

	/* parse .register_info.vset_reg */
	ret = of_property_read_u32_array(np, "pmic-vset",
						register_info, 2);
	if (ret) {
		dev_err(dev, "no sub-spmi-vset property set\n");
		return ret;
	}
	sreg->register_info.vset_reg = register_info[0];
	sreg->register_info.vset_mask = register_info[1];

	/* parse .register_info.chip_version for  distinguishing chip version */
	ret = sub_pmic_regulator_dt_select(sreg, pdev, np);
	if (ret)
		return ret;

	ret = of_property_read_u32(
		np, "pmic-off-on-delay-us", &sreg->off_on_delay);
	if (ret) {
		dev_err(dev,
			"no sub-spmi-off-on-delay-us property set\n");
		return ret;
	}

	/* parse .enable_time */
	ret = of_property_read_u32(
		np, "pmic-enable-time-us", &rdesc->enable_time);
	if (ret) {
		dev_err(dev, "no sub-spmi-enable-time-us property set\n");
		return ret;
	}
	return ret;
}

static int sub_pmic_regulator_dt_parse(
	struct sub_pmic_regulator *sreg, struct spmi_device *pdev,
	struct device_node *np)
{
	struct device *dev = &pdev->dev;
	struct regulator_desc *rdesc = &sreg->rdesc;
	unsigned int *v_table = NULL;
	int ret;

	if (!np) {
		dev_err(dev, "no child root\n");
		ret = -ENOMEM;
		return ret;
	}
	ret = of_property_read_u32(
		np, "pmic-n-voltages", &rdesc->n_voltages);
	if (ret) {
		dev_err(dev, "no sub-spmi-n-voltages property set\n");
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

	ret = of_property_read_u32_array(
		np, "pmic-vset-table", v_table, rdesc->n_voltages);
	if (ret) {
		devm_kfree(dev, v_table);
		dev_err(dev, "no sub-spmi-vset-table property set\n");
		return ret;
	}
	rdesc->volt_table = v_table;

	/* parse regulator's dt common part */
	ret = sub_pmic_regulator_dt_common(sreg, pdev, np);
	if (ret) {
		dev_err(dev, "failure in sub_pmic_regulator_dt_common\n");
		devm_kfree(dev, v_table);
		rdesc->volt_table = NULL;
		return ret;
	}
	return ret;
}

static struct regulator_ops regulator_sub_pmic_rops = {
	.is_enabled = sub_pmic_regulator_is_enabled,
	.enable = sub_pmic_regulator_enable,
	.disable = sub_pmic_regulator_disable,
	.list_voltage = regulator_list_voltage_table,
	.get_voltage = sub_pmic_regulator_get_voltage,
	.set_voltage = sub_pmic_regulator_set_voltage,
	.get_current_limit = NULL,
	.set_current_limit = NULL,
	.get_mode = NULL,
	.set_mode = NULL,
};
static const struct sub_pmic_regulator sub_pmic_regulator = {
	.rdesc = {
			.ops = &regulator_sub_pmic_rops,
			.type = REGULATOR_VOLTAGE,
			.owner = THIS_MODULE,
		},
	.dt_parse = sub_pmic_regulator_dt_parse,
};

static const struct of_device_id of_sub_regulator_match_tbl[] = {
	{
		.compatible = "pmic-sub-ldo",
		.data = &sub_pmic_regulator,
	},
	{}
};

static int fake_of_get_regulator_constraint(
	struct regulation_constraints *constraints, struct device_node *np)
{
	const __be32 *min_uv = NULL;
	const __be32 *max_uv = NULL;
	unsigned int *valid_ops_mask = NULL;
	unsigned int *valid_modes_mask = NULL;

	if ((!np) || (!constraints))
		return -1;
	(constraints)->name = of_get_property(np, "regulator-name", NULL);

	min_uv = of_get_property(np, "regulator-min-microvolt", NULL);
	if (min_uv) {
		(constraints)->min_uV = be32_to_cpu(*min_uv);
		(constraints)->min_uA = be32_to_cpu(*min_uv);
	}

	max_uv = of_get_property(np, "regulator-max-microvolt", NULL);
	if (max_uv) {
		(constraints)->max_uV = be32_to_cpu(*max_uv);
		(constraints)->max_uA = be32_to_cpu(*max_uv);
	}
	valid_ops_mask = (unsigned int *)of_get_property(
		np, "pmic-valid-ops-mask", NULL);
	if (valid_ops_mask)
		(constraints)->valid_ops_mask = be32_to_cpu(*valid_ops_mask);
	valid_modes_mask = (unsigned int *)of_get_property(
		np, "pmic-valid-modes-mask", NULL);
	if (valid_modes_mask)
		(constraints)->valid_modes_mask = be32_to_cpu(*valid_modes_mask);
	return 0;
}
static int sub_regulator_probe(struct spmi_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct regulator_dev *rdev = NULL;
	struct regulator_desc *rdesc = NULL;
	struct sub_pmic_regulator *sreg = NULL;
	struct regulator_init_data *initdata = NULL;
	const struct of_device_id *match = NULL;
	struct sub_pmic_regulator *template = NULL;
	struct regulator_config config;
	const char *supplyname = NULL;

	match = of_match_device(of_sub_regulator_match_tbl, &pdev->dev);
	if (!match) {
		pr_err("get regulator fail!\n");
		return -EINVAL;
	}

	template = (struct sub_pmic_regulator *)(match->data);
	if (!template) {
		pr_err("[%s]no pmic-sub-ldo template data node\n", __func__);
		return -ENODEV;
	}
	initdata = of_get_regulator_init_data(dev, np, NULL);
	if (!initdata) {
		pr_err("[%s]:get regulator init data error!\n", __func__);
		return -EINVAL;
	}
	ret = memset_s(&config, sizeof(struct regulator_config),
				0, sizeof(struct regulator_config));
	if (ret != EOK)
		pr_err("memset_s fail!\n");

	ret = fake_of_get_regulator_constraint(&initdata->constraints, np);
	if (!!ret) {
		pr_err("[%s]:get regulator constraint error!\n", __func__);
		return -EINVAL;
	}
	sreg = kmemdup(template, sizeof(*sreg), GFP_KERNEL);
	if (!sreg)
		return -ENOMEM;
	sreg->name = initdata->constraints.name;
	rdesc = &sreg->rdesc;
	rdesc->name = sreg->name;
	rdesc->min_uV = initdata->constraints.min_uV;
	supplyname = of_get_property(np, "hisilicon,supply_name", NULL);
	if (supplyname != NULL)
		initdata->supply_regulator = supplyname;
	/* to parse device tree data for regulator specific */
	ret = sreg->dt_parse(sreg, pdev, np);
	if (ret) {
		dev_err(dev, "device tree parameter parse error!\n");
		goto sub_regulator_end;
	}
	config.dev = &pdev->dev;
	config.init_data = initdata;
	config.driver_data = sreg;
	config.of_node = pdev->dev.of_node;
	/* register regulator */
	rdev = regulator_register(rdesc, &config);
	if (IS_ERR(rdev)) {
		pr_err("[%s]:regulator failed to register %s\n", __func__,
			rdesc->name);
		ret = PTR_ERR(rdev);
		goto sub_regulator_end;
	}
	dev_set_drvdata(dev, rdev);
	return ret;
sub_regulator_end:
		kfree(sreg);
	return ret;
}

static int _sub_regulator_remove(struct spmi_device *pdev)
{
	struct regulator_dev *rdev = NULL;
	struct sub_pmic_regulator *sreg = NULL;

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

static void sub_regulator_remove(struct spmi_device *pdev)
{
	(void)_sub_regulator_remove(pdev);
}

static const struct spmi_device_id regulator_spmi_id[] = {
	{ "pmic-sub-ldo", 0 },
	{}
};

MODULE_DEVICE_TABLE(spmi, pmic_spmi_id);
static struct spmi_driver sub_regulator_driver = {
	.driver = {
			.name = "regulator-sub-pmic",
			.owner = THIS_MODULE,
			.of_match_table = of_sub_regulator_match_tbl,
		},
	.id_table = regulator_spmi_id,
	.probe = sub_regulator_probe,
	.remove = sub_regulator_remove,
};

static int __init sub_regulator_init(void)
{
	return spmi_driver_register(&sub_regulator_driver);
}

static void __exit sub_regulator_exit(void)
{
	spmi_driver_unregister(&sub_regulator_driver);
}

fs_initcall(sub_regulator_init);
module_exit(sub_regulator_exit);

MODULE_DESCRIPTION("sub regulator driver");
MODULE_LICENSE("GPL v2");
