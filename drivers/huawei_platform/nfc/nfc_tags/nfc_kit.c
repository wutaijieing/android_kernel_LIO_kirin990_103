/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: nfc_kit driver
 * Author: zhangxiaohui chip.zhangxiaohui
 * Create: 2019-08-05
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "nfc_kit.h"
#include "nfc_kit_ndef.h"

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif

#define NFC_INVALID_ID 0xFF

static u32 g_nfc_kit_log_level = NFC_LL_DEBUG;
static struct nfc_kit_data *g_nfc_kit_data = NULL;

static struct vendor_info g_vendor_info[] = {
	{ NFC_NXP, nxp_probe   },
	{ NFC_FM,  fm11n_probe },
};

struct nfc_kit_data *get_nfc_kit_data(void)
{
	return g_nfc_kit_data;
}

u32 get_nfc_kit_log_level(void)
{
	return g_nfc_kit_log_level;
}

void nfc_log_level_change(u32 log_level)
{
	if ((log_level > NFC_LL_DEBUG) || (log_level < NFC_LL_ERR)) {
		NFC_ERR("[%s][%d]: invalid input log_level: %u\n", __func__, __LINE__, log_level);
		return;
	}

	NFC_ERR("[%s][%d]: nfc_kit log level has been changed to [%u]\n", __func__, __LINE__, log_level);
	g_nfc_kit_log_level = log_level;
}

int nfc_chip_register(struct nfc_device_data *chip_data)
{
	if (chip_data == NULL) {
		NFC_ERR("%s: chip_data is NULL\n", __func__);
		return NFC_FAIL;
	}

	if (g_nfc_kit_data == NULL) {
		NFC_ERR("%s: g_nfc_kit_data is NULL\n", __func__);
		return NFC_FAIL;
	}

	mutex_lock(&g_nfc_kit_data->mutex);
	if (g_nfc_kit_data->is_registered) {
		NFC_ERR("%s: already registered\n", __func__);
		mutex_unlock(&g_nfc_kit_data->mutex);
		return NFC_FAIL;
	}

	g_nfc_kit_data->chip_data = chip_data;
	g_nfc_kit_data->is_registered = true;
	mutex_unlock(&g_nfc_kit_data->mutex);
	return NFC_OK;
}

int nfc_of_read_u32(const struct device_node *node, const char *prop_name, u32 *value)
{
	int ret;

	ret = of_property_read_u32(node, prop_name, value);
	if (ret != NFC_OK)
		NFC_ERR("%s: %s get failed, ret=%d\n", __func__, prop_name, ret);

	NFC_MAIN("%s: %s = %u\n", __func__, prop_name, *value);
	return ret;
}

int nfc_of_read_string(const struct device_node *node, const char *prop_name, const char **buf)
{
	int ret;

	ret = of_property_read_string(node, prop_name, buf);
	if (ret != NFC_OK)
		NFC_ERR("%s: %s get failed, ret=%d\n", __func__, prop_name, ret);

	NFC_MAIN("%s: %s = %s\n", __func__, prop_name, *buf);
	return ret;
}

int nfc_of_read_u8_array(const struct device_node *node, const char *prop_name, u8 *value, size_t sz)
{
	int ret;

	ret = of_property_read_u8_array(node, prop_name, value, sz);
	if (ret != NFC_OK)
		NFC_ERR("%s: %s get failed, ret=%d\n", __func__, prop_name, ret);

	NFC_MAIN("%s: %s = %02x, etc\n", __func__, prop_name, value[0]);
	return ret;
}

static int nfc_parse_dts(const struct device_node *node, struct nfc_kit_data *pdata)
{
	int ret;

	ret = nfc_of_read_u32(node, "irq_gpio", &pdata->irq_gpio);
	if (ret != NFC_OK) {
		return ret;
	}

	return ret;
}

static void get_nfc_id(struct nfc_kit_data *pdata)
{
	pdata->nfc_id = NFC_FM;
	NFC_MAIN("%s: nfc id is %u\n", __func__, pdata->nfc_id);
}

static int nfc_try_all_drivers(struct nfc_kit_data *pdata)
{
	u32 i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(g_vendor_info); i++) {
		ret = g_vendor_info[i].chip_probe(pdata);
		if (ret == NFC_OK) {
			NFC_INFO("%s: try nfc id [%u] success\n", __func__, i);
			return ret;
		}
		NFC_INFO("%s: try nfc id [%u] failed, begin to try next\n", __func__, i);
	}

	NFC_ERR("%s: has tried all nfc drivers, but still failed\n", __func__);
	return NFC_FAIL;
}

static int chip_probe(struct nfc_kit_data *pdata)
{
	u32 i;
	int ret = NFC_FAIL;

	for (i = 0; i < ARRAY_SIZE(g_vendor_info); i++) {
		if (pdata->nfc_id == g_vendor_info[i].nfc_id) {
			break;
		}
	}

	if (i >= ARRAY_SIZE(g_vendor_info)) {
		NFC_ERR("%s: can not find nfc id[%d] in g_vendor_info, begin to try all nfc drivers\n",
			__func__, pdata->nfc_id);
		goto try_all_drivers;
	}

	if (g_vendor_info[i].chip_probe == NULL) {
		NFC_ERR("%s: chip_probe is NULL\n", __func__);
		return ret;
	}
	ret = g_vendor_info[i].chip_probe(pdata);
	if (ret != NFC_OK) { /* 根据硬件提供的nfc id加载probe失败了，则执行轮寻加载驱动 */
		goto try_all_drivers;
	}
	return ret;

try_all_drivers:
	return nfc_try_all_drivers(pdata);
}

static DEVICE_ATTR(ndef_test, S_IRUGO | S_IWUSR | S_IWGRP, nfc_ndef_test_show, NULL);
static DEVICE_ATTR(ndef_safe_token, S_IRUGO | S_IWUSR | S_IWGRP, nfc_ndef_safe_token_show, nfc_ndef_safe_token_store);
static DEVICE_ATTR(ndef_mac, S_IRUGO | S_IWUSR | S_IWGRP, nfc_ndef_mac_show, nfc_ndef_mac_store);
static DEVICE_ATTR(ndef_passwd, S_IRUGO | S_IWUSR | S_IWGRP, nfc_ndef_passwd_show, nfc_ndef_passwd_store);
static DEVICE_ATTR(ndef_udid, S_IRUGO | S_IWUSR | S_IWGRP, nfc_ndef_udid_show, nfc_ndef_udid_store);
static DEVICE_ATTR(ndef_sn_code, S_IRUGO | S_IWUSR | S_IWGRP, nfc_ndef_sn_code_show, nfc_ndef_sn_code_store);
static DEVICE_ATTR(ndef, S_IRUGO | S_IWUSR | S_IWGRP, nfc_ndef_show, nfc_ndef_store);
static DEVICE_ATTR(ndef_ldn_mmi, S_IRUGO | S_IWUSR | S_IWGRP, nfc_ndef_ldn_mmi_show, nfc_ndef_ldn_mmi_store);
static DEVICE_ATTR(ndef_direct, S_IRUGO | S_IWUSR | S_IWGRP, nfc_ndef_direct_show, nfc_ndef_direct_store);
static DEVICE_ATTR(ndef_ldn_at, S_IRUGO | S_IWUSR | S_IWGRP, nfc_ndef_ldn_at_show, NULL);

static struct attribute *g_nfc_attrs[] = {
	/* 需要创建的节点，在此添加，并在上面定义 */
	&dev_attr_ndef_test.attr,
	&dev_attr_ndef_safe_token.attr,
	&dev_attr_ndef_mac.attr,
	&dev_attr_ndef_passwd.attr,
	&dev_attr_ndef_udid.attr,
	&dev_attr_ndef_sn_code.attr,
	&dev_attr_ndef.attr,
	&dev_attr_ndef_ldn_mmi.attr,
	&dev_attr_ndef_direct.attr,
	&dev_attr_ndef_ldn_at.attr,
	NULL,
};

static struct attribute_group g_nfc_attr_group = {
	.name = NULL,
	.attrs = g_nfc_attrs,
};

static int nfc_create_sysfs(struct nfc_kit_data *pdata)
{
	int ret;

	NFC_INFO("%s: nfc create sysfs begin, dir:%s \n", __func__, pdata->client->dev.kobj.name);
	ret = sysfs_create_group(&pdata->client->dev.kobj, &g_nfc_attr_group);
	if (ret != NFC_OK)
		NFC_ERR("%s: sysfs_create_group failed\n", __func__);

	NFC_INFO("%s: nfc create sysfs end, ret = %d \n", __func__, ret);
	return ret;
}

static void nfc_remove_sysfs(struct nfc_kit_data *pdata)
{
	NFC_INFO("%s: nfc remove sysfs begin \n", __func__);
	sysfs_remove_group(&pdata->client->dev.kobj, &g_nfc_attr_group);
	return;
}

static int nfc_input_device_register(struct nfc_kit_data *pdata)
{
	int ret;
	struct input_dev *input = NULL;

	input = input_allocate_device();
	if (input == NULL) {
		NFC_ERR("%s: input_allocate_device failed\n", __func__);
		return -ENOMEM;
	}

	input->name = NFC_KIT_NAME;
	set_bit(EV_MSC, input->evbit);
	input_set_capability(input, EV_MSC, MSC_RAW);

	ret = input_register_device(input);
	if (ret != NFC_OK) {
		NFC_ERR("%s: input_register_device failed\n", __func__);
		input_free_device(input);
		return ret;
	}
	pdata->input_dev = input;
	input_set_drvdata(pdata->input_dev, pdata);
	return NFC_OK;
}

static irqreturn_t nfc_irq_handler(int irq, void *handle)
{
	struct nfc_kit_data *pdata = (struct nfc_kit_data *)handle;
	int irq_val;

	if (pdata == NULL || pdata->chip_data == NULL) {
		NFC_ERR("%s: pdata or pdata->chip_data is NULL\n", __func__);
		return IRQ_NONE;
	}
	
	if (pdata->chip_data->ops == NULL || pdata->chip_data->ops->irq_handler == NULL) {
		NFC_ERR("%s: ops or irq_handler is NULL\n", __func__);
		return IRQ_NONE;
	}

	disable_irq(pdata->irq_gpio);
	irq_val = pdata->chip_data->ops->irq_handler();
	if (irq_val == 0x0)
		schedule_work(&pdata->irq0_sch_work);

	enable_irq(pdata->irq_gpio);

	return IRQ_HANDLED;
}

static void irq0_sch_work_func(struct work_struct *irq_sch_work)
{
	int ret;

	if (g_nfc_kit_data == NULL || g_nfc_kit_data->chip_data == NULL) {
		NFC_ERR("%s: g_nfc_kit_data or chip_data is NULL\n", __func__);
		return;
	}

	if (g_nfc_kit_data->chip_data->ops == NULL) {
		NFC_ERR("%s: ops is NULL\n", __func__);
		return;
	}

	if (g_nfc_kit_data->chip_data->ops->irq0_sch_work == NULL) {
		NFC_ERR("%s: irq0_sch_work is NULL\n", __func__);
		return;
	}

	mutex_lock(&g_nfc_kit_data->mutex);
	ret = g_nfc_kit_data->chip_data->ops->irq0_sch_work();
	if (ret != NFC_OK)
		NFC_ERR("%s: irq0_sch_work failed\n", __func__);

	mutex_unlock(&g_nfc_kit_data->mutex);
}

static void irq1_sch_work_func(struct work_struct *irq_sch_work)
{
	int ret;

	if (g_nfc_kit_data == NULL || g_nfc_kit_data->chip_data == NULL) {
		NFC_ERR("%s: g_nfc_kit_data or chip_data is NULL\n", __func__);
		return;
	}

	if (g_nfc_kit_data->chip_data->ops == NULL) {
		NFC_ERR("%s: ops is NULL\n", __func__);
		return;
	}

	if (g_nfc_kit_data->chip_data->ops->irq1_sch_work == NULL) {
		NFC_ERR("%s: irq1_sch_work is NULL\n", __func__);
		return;
	}

	ret = g_nfc_kit_data->chip_data->ops->irq1_sch_work();
	if (ret != NFC_OK)
		NFC_ERR("%s: irq1_sch_work failed\n", __func__);
}

static int nfc_irq_init(struct nfc_kit_data *pdata)
{
	int ret;

	INIT_WORK(&pdata->irq0_sch_work, irq0_sch_work_func);
	INIT_WORK(&pdata->irq1_sch_work, irq1_sch_work_func);
	pdata->irq_id = gpio_to_irq(pdata->irq_gpio);
	pdata->is_first_irq = true;

	ret = request_irq(pdata->irq_id, nfc_irq_handler, IRQF_TRIGGER_FALLING | IRQF_ONESHOT, NFC_KIT_NAME, pdata);
	if (ret != NFC_OK) {
		NFC_ERR("%s: failed, irq gpio is [%u], irq id is [%d]\n", __func__, pdata->irq_gpio, pdata->irq_id);
		return ret;
	}
	gpio_set_debounce(pdata->irq_id, 10);
	schedule_work(&pdata->irq1_sch_work);
	return ret;
}

static int nfc_chip_init(const struct nfc_kit_data *pdata)
{
	int ret;
	const struct nfc_device_ops *ops = NULL;

	if (pdata == NULL || pdata->chip_data == NULL || pdata->chip_data->ops == NULL) {
		NFC_ERR("%s: nfc_chip_init is NULL\n", __func__);
		return NFC_FAIL;
	}
	ops = pdata->chip_data->ops;

	if (ops->chip_init == NULL) {
		NFC_ERR("%s: chip_init is NULL\n", __func__);
		return NFC_FAIL;
	}

	ret = ops->chip_init();
	if (ret != NFC_OK)
		NFC_ERR("%s: chip_init failed, ret = %d\n", __func__, ret);

	return ret;
}

static int nfc_kit_probe(struct i2c_client *client, const struct i2c_device_id *i2c_id)
{
	int ret;
	struct nfc_kit_data *pdata = NULL;

	NFC_MAIN("%s: nfc kit driver probe start\n", __func__);

	pdata = (struct nfc_kit_data *)kzalloc(sizeof(struct nfc_kit_data), GFP_KERNEL);
	if (pdata == NULL) {
		NFC_ERR("%s: kzalloc failed\n", __func__);
		return -ENOMEM;
	}

	if (client == NULL) {
		NFC_ERR("%s: nfc_kit_probe client is NULL\n", __func__);
		return -ENOMEM;
	}
	pdata->init_flag = false;
	pdata->client = client;
	mutex_init(&pdata->mutex);
	g_nfc_kit_data = pdata;

#ifdef CONFIG_OF
	if (pdata->client->dev.of_node == NULL) {
		NFC_ERR("%s, %d: dts node is NULL\n", __func__, __LINE__);
		ret = NFC_FAIL;
		goto err_parse_dts;
	}
	ret = nfc_parse_dts(pdata->client->dev.of_node, pdata); /* 解析平台层的设备树 */
	if (ret != NFC_OK) {
		NFC_ERR("%s: parse dts error, ret = %d\n", __func__, ret);
		goto err_parse_dts;
	}
	NFC_INFO("%s: dts parse success\n", __func__);
#else
	/* 如果平台不支持设备树，通过编译错误进行提醒 */
#error : nfc kit only support device tree platform
#endif

	/* 根据硬件提供的方案，确定出芯片ID */
	get_nfc_id(pdata);
	/* 根据芯片id执行对应的probe，如果上一步中获取id失败则轮寻加载芯片驱动 */
	ret = chip_probe(pdata);
	if (ret != NFC_OK) {
		NFC_ERR("%s: chip_probe failed\n", __func__);
		goto err_parse_dts;
	}

	ret = nfc_kit_ndef_init(&pdata->client->dev);
	if (ret != NFC_OK) {
		NFC_ERR("%s: chip_probe failed\n", __func__);
		goto err_parse_dts;
	}

	ret = nfc_create_sysfs(pdata);
	if (ret != NFC_OK) {
		NFC_ERR("%s: failed\n", __func__);
		goto err_parse_dts;
	}

	/* input子系统注册 */
	ret = nfc_input_device_register(pdata);
	if (ret != NFC_OK) {
		NFC_ERR("%s: ts_kit_input_device_register failed\n", __func__);
		goto err_input_register;
	}

	ret = nfc_chip_init(pdata);
	if (ret != NFC_OK) {
		NFC_ERR("%s: chip init failed\n", __func__);
		goto err_irq_init;
	}

	/* 中断注册 */
	ret = nfc_irq_init(pdata);
	if (ret != NFC_OK) {
		NFC_ERR("%s: irq init failed\n", __func__);
		goto err_irq_init;
	}

	/* 相关数据的初始化 */
	i2c_set_clientdata(client, pdata);
	pdata->init_flag = true;

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	/* detect current device successful, set the flag as present */
	set_hw_dev_flag(DEV_I2C_NFC);
#endif

	NFC_MAIN("%s: nfc i2c driver add success\n", __func__);
	return NFC_OK;
err_irq_init:
	input_unregister_device(pdata->input_dev);
	pdata->input_dev = NULL;
err_input_register:
	nfc_remove_sysfs(pdata);
err_parse_dts:
	nfc_kit_ndef_free();
	if (pdata != NULL) {
		kfree(pdata);
		g_nfc_kit_data = NULL;
	}
	return ret;
}

static struct of_device_id nfc_kit_of_match[] = {
	{ .compatible = "huawei,nfc_kit", },
	{}
};

static const struct i2c_device_id nfc_kit_id[] = {
	{ NFC_KIT_NAME, 0 },
	{}
};

static struct i2c_driver nfc_kit_driver = {
	.driver = {
		.name = NFC_KIT_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = nfc_kit_of_match,
#endif
	},
	.probe = nfc_kit_probe,
	.id_table = nfc_kit_id,
};

static int __init nfc_kit_init(void)
{
	int ret;

	NFC_INFO("%s: NFC-i2c driver add started\n", __func__);
	ret = i2c_add_driver(&nfc_kit_driver);
	if (ret != NFC_OK) {
		NFC_ERR("%s: NFC-i2c driver add failed, ret = %d\n", __func__, ret);
	}
	NFC_INFO("%s: NFC-i2c driver add finished\n", __func__);
	return ret;
}

static void __exit nfc_kit_exit(void)
{
	i2c_del_driver(&nfc_kit_driver);
}

module_init(nfc_kit_init);
module_exit(nfc_kit_exit);
MODULE_DESCRIPTION("NfcTags Driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("NfcTags Driver");
