/*
 * QIC. (QIC Mntn Module.)
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
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

#include "dfx_sec_qic.h"
#include "dfx_sec_qic_err_probe.h"
#include "dfx_sec_qic_dump.h"
#include <linux/arm-smccc.h>
#include <linux/compiler.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <securec.h>
#include <platform_include/see/bl31_smc.h>
#include <../common/sec_qic_share_info.h>
#ifdef CONFIG_DFX_BB
#include <platform_include/basicplatform/linux/rdr_platform.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#endif
#ifdef CONFIG_DFX_DEBUG_FS
#include <platform_include/basicplatform/linux/dfx_mntn_test.h>
#endif

static struct dfx_sec_qic_device *g_qic_dev = NULL;
static struct of_device_id const dfx_sec_qic_match[] = {
	{
		.compatible = "dfx,sec_qic",
		.data = (void *)NULL
	},
	{}
};

struct dfx_sec_qic_device *dfx_sec_qic_get_dev(void)
{
	return g_qic_dev;
}

int dfx_sec_qic_smc_call(unsigned long function_id, unsigned long bus_key)
{
	struct arm_smccc_res res;

	arm_smccc_smc((unsigned long)MNTN_QIC_FID_VALUE, function_id, 0, bus_key, 0, 0, 0, 0, &res);
	if (res.a0)
		return -1;
	return 0;
}

static irqreturn_t dfx_sec_qic_irq_handler(int irq, void *data)
{
	struct dfx_sec_qic_device *qic_dev = (struct dfx_sec_qic_device *)data;

	dfx_sec_qic_err_probe_handler(qic_dev);
	dfx_sec_qic_reset_handler();
	return IRQ_HANDLED;
}

static int dfx_sec_qic_get_component_reg(const struct device_node *qic_node, struct platform_device *pdev,
				     struct qic_src_reg **component_reg, u32 *component_reg_num, const char *name)
{
	u32 i;
	int ret;
	const struct device_node *np = NULL;
	char reg_field[QIC_REG_FIELD_NAME_SIZE] = {0};

	np = of_find_compatible_node((struct device_node *)qic_node, NULL, name);
	if (unlikely(!np)) {
		pr_err("[%s] cannot get %s node\n", __func__, name);
		return -ENODEV;
	}

	if (of_property_read_u32(np, "reg_num", component_reg_num) != 0) {
		pr_err("[%s] get %s reg_num from DTS error.\n", __func__, name);
		return -ENODEV;
	}

	*component_reg = devm_kzalloc(&pdev->dev,
		*component_reg_num * sizeof(*(*component_reg)), GFP_KERNEL);
	if (unlikely(!(*component_reg))) {
		pr_err("[%s] alloc %s fail\n", __func__, name);
		return -ENOMEM;
	}

	for (i = 0; i < *component_reg_num; i++) {
		(*component_reg)[i].reg_base = of_iomap((struct device_node *)np, i);
		if (unlikely(!((*component_reg)[i].reg_base))) {
			pr_err("[%s] %s:%u map fail\n", __func__, name, i);
			return -ENOMEM;
		}

		ret = snprintf_s(reg_field, sizeof(reg_field), sizeof(reg_field) - 1, "len%u", i);
		if (ret < 0) {
			pr_err("[%s] combine reg:%u len fail\n", __func__, i);
			return ret;
		}
		if (of_property_read_u32(np, (const char *)reg_field, &((*component_reg)[i].len)) != 0) {
			pr_err("[%s] get %s:%u len from DTS error.\n", __func__, name, i);
			return -ENODEV;
		}

		(*component_reg)[i].reg_mask = devm_kzalloc(&pdev->dev,
			(*component_reg)[i].len * sizeof(*((*component_reg)[i].reg_mask)), GFP_KERNEL);
		if (unlikely(!((*component_reg)[i].reg_mask))) {
			pr_err("[%s] alloc %s mask fail\n", __func__, name);
			return -ENOMEM;
		}
		ret = snprintf_s(reg_field, sizeof(reg_field), sizeof(reg_field) - 1, "masks%u", i);
		if (ret < 0) {
			pr_err("[%s] combine reg:%u mask fail\n", __func__, i);
			return ret;
		}
		if (of_property_read_u32_array(np, (const char *)reg_field,
			(*component_reg)[i].reg_mask, (*component_reg)[i].len) != 0) {
			pr_err("[%s] get %s:%u mask from DTS error.\n", __func__, name, i);
			return -ENODEV;
		}

		(*component_reg)[i].offsets = devm_kzalloc(&pdev->dev,
			(*component_reg)[i].len * sizeof(*((*component_reg)[i].offsets)), GFP_KERNEL);
		if (unlikely(!((*component_reg)[i].offsets))) {
			pr_err("[%s] alloc %s offset fail\n", __func__, name);
			return -ENOMEM;
		}
		ret = snprintf_s(reg_field, sizeof(reg_field), sizeof(reg_field) - 1, "offsets%u", i);
		if (ret < 0) {
			pr_err("[%s] combine reg:%u offset fail\n", __func__, i);
			return ret;
		}
		if (of_property_read_u32_array(np, (const char *)reg_field,
			(*component_reg)[i].offsets, (*component_reg)[i].len) != 0) {
			pr_err("[%s] get %s:%u offset from DTS error.\n", __func__, name, i);
			return -ENODEV;
		}
	}

	return 0;
}

static int dfx_sec_qic_ib_timeout_enable(const struct device_node *qic_node, struct platform_device *pdev)
{
	struct dfx_sec_qic_device *qic_dev = platform_get_drvdata(pdev);
	int ret;
	u32 i;
	u32 j;
	u32 value;

	if (unlikely(!qic_dev)) {
		dev_err(&pdev->dev, "cannot get qic dev\n");
		return -ENODEV;
	}

	ret = dfx_sec_qic_get_component_reg(qic_node, pdev, &(qic_dev->timeout_en_reg),
		&(qic_dev->timeout_en_reg_num), "dfx,sec_qic_timeout_en_reg");
	if (ret) {
		dev_err(&pdev->dev, "get timeout enable regs fail\n");
		return -ENODEV;
	}

	for (i = 0; i < qic_dev->timeout_en_reg_num; i++) {
		for (j = 0; j < qic_dev->timeout_en_reg[i].len; j++) {
			value = (qic_dev->timeout_en_reg[i].reg_mask[j] >> QIC_TIMEOUT_EN_OFFSET) +
				qic_dev->timeout_en_reg[i].reg_mask[j];
			writel_relaxed(value, qic_dev->timeout_en_reg[i].reg_base +
				       qic_dev->timeout_en_reg[i].offsets[j]);
		}
	}

	return 0;
}

static int dfx_sec_qic_irq_init(const struct device_node *qic_node, struct platform_device *pdev)
{
	int ret;
	struct dfx_sec_qic_device *qic_dev = platform_get_drvdata(pdev);

	if (unlikely(!qic_dev)) {
		dev_err(&pdev->dev, "cannot get qic dev\n");
		return -ENODEV;
	}

	ret = dfx_sec_qic_get_component_reg(qic_node, pdev, &(qic_dev->irq_reg),
		&(qic_dev->irq_reg_num), "dfx,sec_qic_irq_reg");
	if (ret) {
		dev_err(&pdev->dev, "cannot get qic irq reg\n");
		return ret;
	}

	ret = platform_get_irq(pdev, 0);
	if (ret < 0) {
		dev_err(&pdev->dev, "cannot find qic irq\n");
		return ret;
	}
	qic_dev->irq_number = (unsigned int)ret;

	ret = devm_request_irq(&pdev->dev, qic_dev->irq_number, dfx_sec_qic_irq_handler,
		IRQF_TRIGGER_HIGH, "dfx_sec_qic", qic_dev);
	if (ret < 0) {
		dev_err(&pdev->dev, "request qic_irq fail!\n");
		return ret;
	}
	return 0;
}

static int dfx_sec_qic_get_stop_cpu_buses(const struct device_node *qic_node, struct platform_device *pdev)
{
	struct dfx_sec_qic_device *qic_dev = platform_get_drvdata(pdev);

	if (unlikely(!qic_dev)) {
		dev_err(&pdev->dev, "cannot get qic dev\n");
		return -ENODEV;
	}

	if (of_property_read_u32(qic_node, "stop_cpu_bus_num", &(qic_dev->need_stop_cpu_buses_num)) != 0) {
		dev_err(&pdev->dev, "[%s] get stop_cpu_bus_num from DTS error.\n", __func__);
		return -ENODEV;
	}

	qic_dev->need_stop_cpu_buses = devm_kzalloc(&pdev->dev,
		qic_dev->need_stop_cpu_buses_num * sizeof(*(qic_dev->need_stop_cpu_buses)), GFP_KERNEL);
	if (unlikely(!(qic_dev->need_stop_cpu_buses))) {
		dev_err(&pdev->dev, "[%s] alloc buses fail\n", __func__);
		return -ENOMEM;
	}

	if (of_property_read_u32_array(qic_node, "stop_cpu_bus",
		qic_dev->need_stop_cpu_buses, qic_dev->need_stop_cpu_buses_num) != 0) {
		dev_err(&pdev->dev, "[%s] get stop_cpu_bus from DTS error\n", __func__);
		return -ENODEV;
	}

	return 0;
}

static int dfx_sec_qic_get_detail_bus_info(const struct device_node *businfo_node, struct dfx_sec_qic_device *qic_dev)
{
	const char **name = NULL;
	u32 *bus_key = NULL;
	u32 *mid_offset = NULL;
	u32 i;
	int ret;

	name = kmalloc(qic_dev->bus_info_num * sizeof(*name), GFP_KERNEL);
	if (unlikely(ZERO_OR_NULL_PTR((unsigned long)(uintptr_t)name))) {
		pr_err("[%s] alloc bus name fail\n", __func__);
		return -ENOMEM;
	}
	ret = of_property_read_string_array(businfo_node, "bus_name", name, qic_dev->bus_info_num);
	if (ret < 0) {
		pr_err("[%s] get bus name from DTS error,ret:%d\n", __func__, ret);
		kfree(name);
		return ret;
	}

	bus_key = kmalloc(qic_dev->bus_info_num * sizeof(*bus_key), GFP_KERNEL);
	if (unlikely(ZERO_OR_NULL_PTR((unsigned long)(uintptr_t)bus_key))) {
		pr_err("[%s] alloc bus key fail\n", __func__);
		kfree(name);
		return -ENOMEM;
	}
	ret = of_property_read_u32_array(businfo_node, "bus_key", bus_key, qic_dev->bus_info_num);
	if (ret) {
		pr_err("[%s] get bus key from DTS error,ret:%d\n", __func__, ret);
		kfree(name);
		kfree(bus_key);
		return ret;
	}

	mid_offset = kmalloc(qic_dev->bus_info_num * sizeof(*mid_offset), GFP_KERNEL);
	if (unlikely(ZERO_OR_NULL_PTR((unsigned long)(uintptr_t)mid_offset))) {
		pr_err("[%s] alloc mid_offset fail\n", __func__);
		kfree(name);
		kfree(bus_key);
		return -ENOMEM;
	}
	if (of_property_read_u32_array(businfo_node, "mid_offset", mid_offset, qic_dev->bus_info_num) != 0) {
		pr_err("[%s] get mid_offset from DTS error, use default value\n", __func__);
		kfree(mid_offset);
		mid_offset = NULL;
	}

	for (i = 0; i < qic_dev->bus_info_num; i++) {
		qic_dev->bus_info[i].bus_key = bus_key[i];
		qic_dev->bus_info[i].bus_name = name[i];
		if (mid_offset)
			qic_dev->bus_info[i].mid_offset = mid_offset[i];
		else
			qic_dev->bus_info[i].mid_offset = QIC_DEFAULT_MID_OFFSET;
	}

	kfree(name);
	kfree(bus_key);
	if (mid_offset)
		kfree(mid_offset);
	return 0;
}

static int dfx_sec_qic_get_bus_info(const struct device_node *qic_node, struct platform_device *pdev)
{
	const struct device_node *np = NULL;
	struct dfx_sec_qic_device *qic_dev = platform_get_drvdata(pdev);

	if (unlikely(!qic_dev)) {
		dev_err(&pdev->dev, "cannot get qic dev\n");
		return -ENODEV;
	}

	np = of_find_compatible_node((struct device_node *)qic_node, NULL, "dfx,sec_qic_bus_info");
	if (unlikely(!np)) {
		pr_err("[%s] cannot get sec_qic_bus_info node\n", __func__);
		return -ENODEV;
	}

	if (of_property_read_u32(np, "bus_num", &(qic_dev->bus_info_num)) != 0) {
		dev_err(&pdev->dev, "[%s] get bus_num from DTS error.\n", __func__);
		return -ENODEV;
	}
	qic_dev->bus_info = devm_kzalloc(&pdev->dev,
		qic_dev->bus_info_num * sizeof(*(qic_dev->bus_info)), GFP_KERNEL);
	if (unlikely(!(qic_dev->bus_info))) {
		dev_err(&pdev->dev, "[%s] alloc bus info fail\n", __func__);
		return -ENOMEM;
	}

	return dfx_sec_qic_get_detail_bus_info(np, qic_dev);
}

static int dfx_sec_qic_get_master_info(const struct device_node *qic_node, struct platform_device *pdev,
				   struct qic_master_info **master_info, u32 *master_num, const char *name)
{
	const struct device_node *np = NULL;
	const struct property *prop = NULL;
	const __be32 *val = NULL;
	u32 i;
	size_t cnt;

	np = of_find_compatible_node((struct device_node *)qic_node, NULL, name);
	if (unlikely(!np)) {
		pr_err("[%s] cannot get %s node\n", __func__, name);
		return -ENODEV;
	}

	if (of_property_read_u32(np, "target_num", master_num) != 0) {
		dev_err(&pdev->dev, "[%s] get %s target_num from DTS error.\n", __func__, name);
		return -ENODEV;
	}
	*master_info = devm_kzalloc(&pdev->dev,
		*master_num * sizeof(*(*master_info)), GFP_KERNEL);
	if (unlikely(!(*master_info))) {
		dev_err(&pdev->dev, "[%s] alloc mid info fail\n", __func__);
		return -ENOMEM;
	}

	prop = of_find_property(np, "target_list", NULL);
	if (!prop || !(prop->value)) {
		pr_err("get target_list fail\n");
		return -ENODEV;
	}

	val = prop->value;
	for (i = 0; i < *master_num; i++) {
		(*master_info)[i].masterid_value = be32_to_cpup(val++);
		(*master_info)[i].masterid_name = (const char *)val;
		cnt = strlen((const char *)val);
		val = (__be32 *)((char *)val + cnt + 1);
	}
	return 0;
}

static int dfx_sec_qic_get_mid_and_acpucore_info(const struct device_node *qic_node, struct platform_device *pdev)
{
	int ret;
	struct dfx_sec_qic_device *qic_dev = platform_get_drvdata(pdev);

	if (unlikely(!qic_dev)) {
		dev_err(&pdev->dev, "cannot get qic dev\n");
		return -ENODEV;
	}

	ret = dfx_sec_qic_get_master_info(qic_node, pdev, &(qic_dev->mid_info),
				      &(qic_dev->mid_info_num), "dfx,sec_qic_mid_info");
	if (ret)
		return ret;

	return dfx_sec_qic_get_master_info(qic_node, pdev, &(qic_dev->acpu_core_info),
				       &(qic_dev->acpu_core_info_num), "dfx,sec_qic_acpu_core_info");
}

static void dfx_sec_qic_unmap_reg(struct qic_src_reg *qic_reg, u32 reg_num)
{
	u32 i;

	for (i = 0; i < reg_num; i++) {
		if (qic_reg[i].reg_base)
			iounmap(qic_reg[i].reg_base);
	}
}

static void dfx_sec_qic_free_source(struct dfx_sec_qic_device *qic_dev)
{
	if (!qic_dev) {
		pr_err("no need to free\n");
		return;
	}

	dfx_sec_qic_unmap_reg(qic_dev->irq_reg, qic_dev->irq_reg_num);
	dfx_sec_qic_unmap_reg(qic_dev->timeout_en_reg, qic_dev->timeout_en_reg_num);
	if (qic_dev->qic_share_base)
		iounmap(qic_dev->qic_share_base);
	g_qic_dev = NULL;
}

static int dfx_sec_qic_probe(struct platform_device *pdev)
{
	struct dfx_sec_qic_device *qic_dev = NULL;
	const struct device_node *np = NULL;
	int ret;

	if (!pdev)
		return -ENOMEM;

	qic_dev = devm_kzalloc(&pdev->dev, sizeof(struct dfx_sec_qic_device), GFP_KERNEL);
	if (!qic_dev)
		return -ENOMEM;

	platform_set_drvdata(pdev, qic_dev);
	np = pdev->dev.of_node;

	ret = dfx_sec_qic_smc_call(QIC_INIT_FIQ_SUBTYPE, 0);
	if (ret)
		goto qic_probe_err;

	ret = dfx_sec_qic_dump_init();
	if (ret)
		goto qic_probe_err;

	qic_dev->qic_share_base = ioremap_wc(QIC_SHARE_MEM_BASE, QIC_SHARE_MEM_SIZE);
	if (!qic_dev->qic_share_base)
		goto qic_free_source;

	ret = dfx_sec_qic_ib_timeout_enable(np, pdev);
	if (ret)
		goto qic_free_source;

	ret = dfx_sec_qic_get_stop_cpu_buses(np, pdev);
	if (ret)
		goto qic_free_source;

	ret = dfx_sec_qic_get_bus_info(np, pdev);
	if (ret)
		goto qic_free_source;

	ret = dfx_sec_qic_get_mid_and_acpucore_info(np, pdev);
	if (ret)
		goto qic_free_source;

	ret = dfx_sec_qic_irq_init(np, pdev);
	if (ret)
		goto qic_free_source;

	g_qic_dev = qic_dev;
	dev_notice(&pdev->dev, "qic init success!\n");
	return 0;
qic_free_source:
	dfx_sec_qic_free_source(qic_dev);
qic_probe_err:
	dev_err(&pdev->dev, "qic init fail!");
	return ret;
}

static int dfx_sec_qic_remove(struct platform_device *pdev)
{
	struct dfx_sec_qic_device *qic_dev = NULL;

	if (!pdev)
		return -ENOMEM;

	qic_dev = platform_get_drvdata(pdev);
	if (qic_dev != NULL)
		free_irq(qic_dev->irq_number, qic_dev);

	dfx_sec_qic_free_source(qic_dev);
	return 0;
}

MODULE_DEVICE_TABLE(of, dfx_sec_qic_match);

#ifdef CONFIG_DFX_DEBUG_FS
void sec_qic_shutdown(struct platform_device *dev)
{
	mntn_test_shutdown_deadloop();
}
#endif

static struct platform_driver dfx_sec_qic_driver = {
	.probe = dfx_sec_qic_probe,
	.remove = dfx_sec_qic_remove,
	.driver = {
		   .name = QIC_MODULE_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(dfx_sec_qic_match),
		   },
#ifdef CONFIG_DFX_DEBUG_FS
	.shutdown = sec_qic_shutdown,
#endif
};

static int __init dfx_sec_qic_init(void)
{
	return platform_driver_register(&dfx_sec_qic_driver);
}

static void __exit dfx_sec_qic_exit(void)
{
	platform_driver_unregister(&dfx_sec_qic_driver);
}

late_initcall(dfx_sec_qic_init);
module_exit(dfx_sec_qic_exit);

