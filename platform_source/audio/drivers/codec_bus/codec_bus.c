/*
 * codec_bus.c -- codec bus driver
 *
 * Copyright (c) 2012-2021 Huawei Technologies Co., Ltd.
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

#include "codec_bus.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <rdr_audio_adapter.h>
#include "soc_sctrl_interface.h"
#include "platform_base_addr_info.h"

#include "codec_bus_inner.h"
#include "audio_log.h"

#define LOG_TAG "codec_bus"

#ifdef CONFIG_AUDIO_COMMON_IMAGE
#undef SOC_ACPU_SCTRL_BASE_ADDR
#define SOC_ACPU_SCTRL_BASE_ADDR 0
#endif

struct codec_bus_data {
	struct device *dev;
	struct codec_bus_ops ops;
	bool pm_runtime_support;
	void __iomem *sctrl_mem;
	bool is_active;
	int irq;

	struct pinctrl *pctrl;
	struct pinctrl_state *pin_default;
	struct pinctrl_state  *pin_idle;

	struct regulator *bus_regulator;

	struct clk *pmu_audio_clk; /* codec 19.2M clk */
	struct clk *asp_subsys_clk;
};

static const struct of_device_id codec_bus_platform_match[] = {
	{
		.compatible = "audio-codec-bus",
	},
	{},
};

static struct codec_bus_data g_bus_data;

#ifdef CONFIG_PM
static void print_clk_regulator_status(struct codec_bus_data *bus_data)
{
	unsigned int power_status = 0;
	unsigned int clk_status = 0;

#ifdef SOC_SCTRL_SCPWRACK_audio_mtcmos_ack_START
	power_status = readl(SOC_SCTRL_SCPWRACK_ADDR(bus_data->sctrl_mem));
#endif

#ifdef SOC_SCTRL_SCPERCLKEN1_gt_clk_asp_subsys_acpu_START
	power_status = readl(SOC_SCTRL_SCPERCLKEN1_ADDR(bus_data->sctrl_mem));
#endif

#ifdef SOC_SCTRL_SCPERSTAT0_st_clk_asp_subsys_START
	clk_status = readl(SOC_SCTRL_SCPERSTAT0_ADDR(bus_data->sctrl_mem));
#endif

	AUDIO_LOGI("power state: 0x%x, clk state: 0x%x, pmu count: %d, clk count: %d, usage_count: 0x%x, runtime_status: %x",
		power_status, clk_status,
		clk_get_enable_count(bus_data->pmu_audio_clk),
		clk_get_enable_count(bus_data->asp_subsys_clk),
		atomic_read(&(bus_data->dev->power.usage_count)), bus_data->dev->power.runtime_status);
}
#endif

static int irq_init(struct platform_device *pdev)
{
	struct codec_bus_data *priv = platform_get_drvdata(pdev);

	priv->irq = platform_get_irq_byname(pdev, "irq_codec_bus");
	if (priv->irq < 0) {
		AUDIO_LOGE("get irq failed");
		return -EFAULT;
	}

	return 0;
}

static int regulator_init(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct codec_bus_data *priv = platform_get_drvdata(pdev);

	priv->bus_regulator = devm_regulator_get(dev, "codec-bus");
	if (IS_ERR(priv->bus_regulator)) {
		AUDIO_LOGE("couldn't get regulators");
		return -EFAULT;
	}

	ret = regulator_enable(priv->bus_regulator);
	if (ret) {
		AUDIO_LOGE("couldn't enable regulators %d", ret);
		return -EFAULT;
	}

	return 0;
}

static void regulator_deinit(struct platform_device *pdev)
{
	struct codec_bus_data *priv = platform_get_drvdata(pdev);

	int ret = regulator_disable(priv->bus_regulator);
	if (ret)
		AUDIO_LOGE("regulator disable failed, ret: %d", ret);
}

static int clk_init(struct platform_device *pdev)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct codec_bus_data *priv = platform_get_drvdata(pdev);

	priv->pmu_audio_clk = devm_clk_get(dev, "clk_pmuaudioclk");
	if (IS_ERR_OR_NULL(priv->pmu_audio_clk)) {
		AUDIO_LOGE("devm_clk_get: pmu_audio_clk not found");
		return -EFAULT;
	}

	ret = clk_prepare_enable(priv->pmu_audio_clk);
	if (ret) {
		AUDIO_LOGE("pmu_audio_clk :clk prepare enable failed");
		goto get_pmu_audio_clk_err;
	}
	/* make sure pmu clk has stable */
	mdelay(1);

	priv->asp_subsys_clk = devm_clk_get(dev, "clk_asp_subsys");
	if (IS_ERR_OR_NULL(priv->asp_subsys_clk)) {
		AUDIO_LOGE("devm_clk_get: clk_asp_subsys not found");
		goto pmu_audio_clk_enable_err;
	}

	ret = clk_prepare_enable(priv->asp_subsys_clk);
	if (ret) {
		AUDIO_LOGE("asp_subsys_clk :clk prepare enable failed");
		goto asp_subsys_clk_clk_err;
	}

	return 0;

asp_subsys_clk_clk_err:
pmu_audio_clk_enable_err:
	clk_disable_unprepare(priv->pmu_audio_clk);
get_pmu_audio_clk_err:
	return -EFAULT;
}

static void clk_deinit(struct platform_device *pdev)
{
	struct codec_bus_data *priv = platform_get_drvdata(pdev);

	clk_disable_unprepare(priv->asp_subsys_clk);
	clk_disable_unprepare(priv->pmu_audio_clk);
}

static int pinctrl_init(struct platform_device *pdev)
{
	int ret;
	struct codec_bus_data *priv = platform_get_drvdata(pdev);

	priv->pctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(priv->pctrl)) {
		AUDIO_LOGE("could not get pinctrl");
		return -EFAULT;
	}
	priv->pin_default = pinctrl_lookup_state(priv->pctrl, PINCTRL_STATE_DEFAULT);
	if (IS_ERR(priv->pin_default)) {
		AUDIO_LOGE("could not get defstate (%li)", PTR_ERR(priv->pin_default));
		return -EFAULT;
	}
	priv->pin_idle = pinctrl_lookup_state(priv->pctrl, PINCTRL_STATE_IDLE);
	if (IS_ERR(priv->pin_idle)) {
		AUDIO_LOGE("could not get defstate (%li)", PTR_ERR(priv->pin_idle));
		return -EFAULT;
	}

	ret = pinctrl_select_state(priv->pctrl, priv->pin_default);
	if (ret) {
		AUDIO_LOGE("could not set pins to default state");
		return -EFAULT;
	}

	return ret;
}

static void pm_enable(struct platform_device *pdev)
{
	struct codec_bus_data *priv = platform_get_drvdata(pdev);

	if (of_property_read_bool(pdev->dev.of_node, "pm_runtime_support"))
		priv->pm_runtime_support = true;

	AUDIO_LOGI("pm_runtime_suppport: %d", priv->pm_runtime_support);

	if (priv->pm_runtime_support) {
		pm_runtime_use_autosuspend(&pdev->dev);
		pm_runtime_set_autosuspend_delay(&pdev->dev, 2000); /* 2000ms */
		pm_runtime_set_active(&pdev->dev);
		pm_runtime_enable(&pdev->dev);
	}
}

static void pm_disable(struct platform_device *pdev)
{
	struct codec_bus_data *priv = platform_get_drvdata(pdev);

	if (priv->pm_runtime_support) {
		pm_runtime_resume(&pdev->dev);
		pm_runtime_disable(&pdev->dev);
		pm_runtime_set_suspended(&pdev->dev);
	}
}

static int io_remap_init(struct platform_device *pdev)
{
	struct codec_bus_data *priv = platform_get_drvdata(pdev);

	priv->sctrl_mem = devm_ioremap(priv->dev, get_phy_addr(SOC_ACPU_SCTRL_BASE_ADDR, PLT_SCTRL_MEM), 0x1000);
	if (priv->sctrl_mem == NULL) {
		AUDIO_LOGE("no memory for sctrl");
		return -ENOMEM;
	}

	return 0;
}

static int codec_bus_probe(struct platform_device *pdev)
{
	int ret;

	platform_set_drvdata(pdev, &g_bus_data);
	g_bus_data.dev = &pdev->dev;

	ret = irq_init(pdev);
	if (ret) {
		AUDIO_LOGE("irq init failed");
		return -EFAULT;
	}

	ret = io_remap_init(pdev);
	if (ret) {
		AUDIO_LOGE("reg remap failed");
		return -EFAULT;
	}

	ret = regulator_init(pdev);
	if (ret) {
		AUDIO_LOGE("regulator init failed");
		return ret;
	}

	ret = clk_init(pdev);
	if (ret) {
		AUDIO_LOGE("bus clk init failed, err code 0x%x", ret);
		goto clk_init_err;
	}

	ret = pinctrl_init(pdev);
	if (ret) {
		AUDIO_LOGE("pinctrl init failed, err code 0x%x", ret);
		goto pinctrl_init_err;
	}

	pm_enable(pdev);

	ret = of_platform_populate(pdev->dev.of_node, NULL, NULL, &pdev->dev);
	if (ret) {
		AUDIO_LOGE("creat platform device failed, err code 0x%x", ret);
		return ret;
	}

	AUDIO_LOGI("codec bus probe success");

	return ret;

pinctrl_init_err:
	clk_deinit(pdev);
clk_init_err:
	regulator_deinit(pdev);

	return -EFAULT;
}

static int codec_bus_remove(struct platform_device *pdev)
{
	pm_disable(pdev);
	clk_deinit(pdev);
	regulator_deinit(pdev);
	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int codec_bus_suspend(struct device *dev)
{
	int ret = 0;
	int pm_ret;
	struct platform_device *pdev = to_platform_device(dev);
	struct codec_bus_data *priv = platform_get_drvdata(pdev);

	AUDIO_LOGI("codec bus suspend");

	if (priv->pm_runtime_support) {
		pm_ret = pm_runtime_get_sync(dev);
		if (pm_ret < 0) {
			AUDIO_LOGE("pm get sync error, pm_ret: %d", pm_ret);
#ifdef CONFIG_DFX_BB
			rdr_system_error(RDR_AUDIO_RUNTIME_SYNC_FAIL_MODID, 0, 0);
#endif
			return pm_ret;
		}
	}

	if (priv->is_active) {
		AUDIO_LOGI("bus is active, continue");
		goto exit;
	}

	ret = pinctrl_select_state(priv->pctrl, priv->pin_idle);
	if (ret) {
		AUDIO_LOGE("could not set pins to idle state");
		goto exit;
	}

	clk_disable_unprepare(priv->pmu_audio_clk);

	ret = regulator_disable(priv->bus_regulator);
	if (ret) {
		AUDIO_LOGE("regulator disable failed");
		goto exit;
	}

	clk_disable_unprepare(priv->asp_subsys_clk);

exit:
	print_clk_regulator_status(priv);
	return ret;
}

static int codec_bus_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct codec_bus_data *priv = platform_get_drvdata(pdev);
	int ret = 0;

	AUDIO_LOGI("codec bus resume");

	if (priv->is_active) {
		AUDIO_LOGI("bus is active, continue");
		goto exit;
	}

	ret = clk_prepare_enable(priv->asp_subsys_clk);
	if (ret) {
		AUDIO_LOGE("asp_subsys_clk :clk prepare enable failed");
		goto exit;
	}

	ret = regulator_enable(priv->bus_regulator);
	if (ret) {
		AUDIO_LOGE("couldn't enable regulators %d", ret);
		goto exit;
	}

	ret = clk_prepare_enable(priv->pmu_audio_clk);
	if (ret) {
		AUDIO_LOGE("pmu_audio_clk :clk prepare enable failed");
		goto exit;
	}
	/* make sure pmu clk has stable */
	mdelay(1);

	ret = pinctrl_select_state(priv->pctrl, priv->pin_default);
	if (ret) {
		AUDIO_LOGE("could not set pins to default state");
		goto exit;
	}

exit:
	if (priv->pm_runtime_support) {
		pm_runtime_mark_last_busy(dev);
		pm_runtime_put_autosuspend(dev);
	}

	print_clk_regulator_status(priv);
	return ret;
}
#endif

#ifdef CONFIG_PM
static int codec_bus_runtime_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct codec_bus_data *priv = platform_get_drvdata(pdev);

	AUDIO_LOGI("codec bus runtime suspend");
	clk_disable_unprepare(priv->pmu_audio_clk);
	clk_disable_unprepare(priv->asp_subsys_clk);
	print_clk_regulator_status(priv);
	return 0;
}

static int codec_bus_runtime_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct codec_bus_data *priv = platform_get_drvdata(pdev);
	int ret;

	AUDIO_LOGI("codec bus runtime resume");
	ret = clk_prepare_enable(priv->asp_subsys_clk);
	if (ret != 0) {
		AUDIO_LOGE("asp_subsys_clk :clk prepare enable failed");
		goto exit;
	}

	ret = clk_prepare_enable(priv->pmu_audio_clk);
	if (ret != 0)
		AUDIO_LOGE("pmu_audio_clk :clk prepare enable failed");
	/* make sure pmu clk has stable */
	mdelay(1);

exit:
	print_clk_regulator_status(priv);
	return ret;
}
#endif

const struct dev_pm_ops codec_bus_pm_ops = {
#ifdef CONFIG_HIBERNATION
	.suspend = codec_bus_suspend,
	.resume = codec_bus_resume,
	.runtime_suspend = codec_bus_runtime_suspend,
	.runtime_resume = codec_bus_runtime_resume
#else
	SET_SYSTEM_SLEEP_PM_OPS(codec_bus_suspend, codec_bus_resume)
	SET_RUNTIME_PM_OPS(codec_bus_runtime_suspend, codec_bus_runtime_resume, NULL)
#endif
};

static struct platform_driver codec_bus_driver = {
	.probe = codec_bus_probe,
	.remove = codec_bus_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "codec_bus",
		.of_match_table = of_match_ptr(codec_bus_platform_match),
		.pm = &codec_bus_pm_ops,
	},
};

void register_ops(struct codec_bus_ops *bus_ops)
{
	if (bus_ops == NULL) {
		AUDIO_LOGE("ops is null");
		return;
	}

	memcpy(&g_bus_data.ops, bus_ops, sizeof(g_bus_data.ops));
}

bool is_pm_runtime_support(void)
{
	return g_bus_data.pm_runtime_support;
}

int get_irq_id(void)
{
	return g_bus_data.irq;
}

void set_bus_active(bool active)
{
	g_bus_data.is_active = active;
}

bool is_bus_active(void)
{
	return g_bus_data.is_active;
}

int codec_bus_activate(const char* scene_name, struct scene_param *params)
{
	if (scene_name == NULL) {
		AUDIO_LOGE("param is null");
		return -EINVAL;
	}

	return g_bus_data.ops.activate != NULL ? g_bus_data.ops.activate(scene_name, params) : -EINVAL;
}

int codec_bus_deactivate(const char* scene_name, struct scene_param *params)
{
	if (scene_name == NULL) {
		AUDIO_LOGE("param is null");
		return -EINVAL;
	}

	return g_bus_data.ops.deactivate != NULL ? g_bus_data.ops.deactivate(scene_name, params) : -EINVAL;
}

int codec_bus_switch_framer(enum framer_type framer_type)
{
	return g_bus_data.ops.switch_framer != NULL ? g_bus_data.ops.switch_framer(framer_type) : -EINVAL;
}

enum framer_type codec_bus_get_framer_type(void)
{
	return g_bus_data.ops.get_framer_type != NULL ? g_bus_data.ops.get_framer_type() : FRAMER_SOC;
}

int codec_bus_enum_device(void)
{
	return g_bus_data.ops.enum_device != NULL ? g_bus_data.ops.enum_device() : -EINVAL;
}

int codec_bus_runtime_get(void)
{
	return g_bus_data.ops.runtime_get != NULL ? g_bus_data.ops.runtime_get() : -EFAULT;
}

void codec_bus_runtime_put(void)
{
	if (g_bus_data.ops.runtime_put != NULL)
		g_bus_data.ops.runtime_put();
}

static int __init codec_bus_driver_init(void)
{
	return platform_driver_register(&codec_bus_driver);
}
fs_initcall(codec_bus_driver_init);

static void __exit codec_bus_driver_exit(void)
{
	platform_driver_unregister(&codec_bus_driver);
}
module_exit(codec_bus_driver_exit);
