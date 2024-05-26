/*
 * func key driver
 *
 * Copyright (C) Huawei Technologies Co., Ltd. 2016-2021. All rights reserved.
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

#include "keypad.h"

#define DTS_STR_KEY_NAME "key_name"
#define DTS_STR_KEY_NODE_NAME "key_node_name"
#define DTS_STR_KEY_WAKE_NAME "key_wake_name"
#define DTS_STR_KEY_GPIO_TO_IRQ_NAME "gpio_to_irq_name"
#define DTS_STR_KEYCODE "keycode"

#define MAX_KEY_NUM 10

struct key_name_code {
	const char *key_name;
	int keycode;
};

struct keypad_key_devices *g_func_key;

static void report_key(struct keypad_key_devices *key_dev,
	struct keypad_common_data *g_key_common_data,
	unsigned int report_action)
{
#ifdef CONFIG_KEY_MASK
	/* make sure report in pair */
	if (g_key_common_data->key_mask_ctl == KEY_DISABLE_MASK)
		g_key_common_data->key_mask_flag = KEY_DISABLE_MASK;
	else if (g_key_common_data->key_mask_ctl == KEY_ENABLE_MASK)
		g_key_common_data->key_mask_flag = KEY_ENABLE_MASK;

	if (g_key_common_data->key_mask_flag == KEY_ENABLE_MASK) {
#endif
		pr_info("[funckey]func key %d action %u\n", key_dev->keycode, report_action);
		input_report_key(key_dev->input_dev, key_dev->keycode, report_action);
		input_sync(key_dev->input_dev);

#ifdef CONFIG_KEY_MASK
	} else {
		pr_info("[funckey]%s key is disabled %u\n",
			key_dev->devices_names, report_action);
	}
#endif
}

static void func_key_work(struct work_struct *work)
{
	struct keypad_key_devices *key_dev =
		container_of(work, struct keypad_key_devices, gpio_key_work.work);
	struct keypad_common_data *g_key_common_data = NULL;
	unsigned int key_func_value;
	unsigned int report_action = GPIO_KEY_RELEASE;

	key_func_value = gpio_get_value(
		(unsigned int)key_dev->keypad_num);
	/* judge key is pressed or released. */
	if (key_func_value == GPIO_LOW_VOLTAGE) {
		report_action = GPIO_KEY_PRESS;
	} else if (key_func_value == GPIO_HIGH_VOLTAGE) {
		report_action = GPIO_KEY_RELEASE;
	} else {
		pr_err("[funckey][%s]invalid gpio key_func\n", __func__);
		return;
	}

	g_key_common_data = keypad_get_common_data();

	report_key(key_dev, g_key_common_data, report_action);

	if (key_func_value == GPIO_HIGH_VOLTAGE)
		__pm_relax(key_dev->gpio_key_lock);
}

static irqreturn_t gpio_func_key_irq_handler(int irq, void *dev_id)
{
	struct keypad_key_devices *key_dev =
		(struct keypad_key_devices *)dev_id;

	__pm_stay_awake(key_dev->gpio_key_lock);
	queue_delayed_work(system_power_efficient_wq,
		&key_dev->gpio_key_work, TIMER_DEBOUNCE);

	return IRQ_HANDLED;
}

static void func_key_set_bit(struct keypad_key_devices *key_dev)
{
	set_bit(key_dev->keycode, key_dev->input_dev->keybit);
}

static int func_key_req_irq(struct keypad_key_devices *key_dev)
{
	int err;

	err = request_irq(key_dev->keypad_irq,
		gpio_func_key_irq_handler,
		IRQF_NO_SUSPEND |
		IRQF_TRIGGER_RISING |
		IRQF_TRIGGER_FALLING,
		key_dev->pdev->name, key_dev);
	if (err)
		return err;

	return 0;
}

void func_key_irq_disable(void)
{
	if (g_func_key == NULL) {
		pr_err("KEY_F26 doesn't exist\n");
		return;
	}
	pr_info("func key disabaled irq\n");
	disable_irq(g_func_key->keypad_irq);
}

void func_key_irq_enable(void)
{
	if (g_func_key == NULL) {
		pr_err("KEY_F26 doesn't exist\n");
		return;
	}
	pr_info("func key enable irq\n");
	enable_irq(g_func_key->keypad_irq);
}

static const struct of_device_id func_key_match[] = {
	{
		.compatible = "hisilicon,func-key"
	},
	{},
};
MODULE_DEVICE_TABLE(of, func_key_match);

static int get_dts_key_info(const struct device_node *node, struct keypad_key_devices *func_key)
{
	int ret;

	ret = of_property_read_string(node, DTS_STR_KEY_NAME, &(func_key->devices_names));
	if (ret != 0) {
		pr_err("%s read key_name fail, ret:%d\n", __func__, ret);
		return ret;
	}
	ret = of_property_read_string(node, DTS_STR_KEY_NODE_NAME, &(func_key->key_node_name));
	if (ret != 0) {
		pr_err("%s read key_node_name fail, ret:%d\n", __func__, ret);
		return ret;
	}
	ret = of_property_read_string(node, DTS_STR_KEY_WAKE_NAME, &(func_key->work_init_name));
	if (ret != 0) {
		pr_err("%s read key_wake_name fail, ret:%d\n", __func__, ret);
		return ret;
	}
	ret = of_property_read_string(node, DTS_STR_KEY_GPIO_TO_IRQ_NAME,
		&(func_key->gpio_to_irq_name));
	if (ret != 0) {
		pr_err("%s read gpio_to_irq_name fail, ret:%d\n", __func__, ret);
		return ret;
	}
	ret = of_property_read_s32(node, DTS_STR_KEYCODE, &(func_key->keycode));

	pr_info("key_name:%s, node_name:%s, wake_name:%s, irq_name:%s, keycode:%d, ret:%d\n",
		func_key->devices_names, func_key->key_node_name,
		func_key->work_init_name, func_key->gpio_to_irq_name, func_key->keycode, ret);
	return ret;
}

static int func_key_probe(struct platform_device *pdev)
{
	struct keypad_key_devices *func_keys = NULL;
	struct device_node *child_node = NULL;
	int ret;
	int i = 0;
	int supported_key_num;

	dev_info(&pdev->dev, "%s\n", __func__);

	ret = keypad_get_of_node(func_key_match, pdev);
	if (ret) {
		pr_err("keypad_get_of_node fail\n");
		return -EINVAL;
	}

	supported_key_num = of_get_child_count(pdev->dev.of_node);
	if (supported_key_num <= 0 || supported_key_num > MAX_KEY_NUM) {
		pr_err("get key num fail, key_num[%d]", supported_key_num);
		return -EINVAL;
	}

	func_keys = devm_kzalloc(&pdev->dev,
		sizeof(*func_keys) * supported_key_num, GFP_KERNEL);
	if (func_keys == NULL)
		return -ENOMEM;

	for_each_child_of_node(pdev->dev.of_node, child_node) {
		if (i >= supported_key_num) {
			pr_err("%s child node num over max num, check DTS\n", __func__);
			break;
		}
		ret = get_dts_key_info(child_node, &func_keys[i]);
		if (ret != 0) {
			pr_err("%s get dts info fail, ret:%d\n", __func__, ret);
			continue;
		}
		func_keys[i].key_set_bit = func_key_set_bit;
		func_keys[i].key_work = func_key_work;
		func_keys[i].keypad_req_irq = func_key_req_irq;
		func_keys[i].pdev = pdev;
		if (func_keys[i].keycode == KEY_F26)
			g_func_key = &func_keys[i];

		ret = keypad_register_dev(&func_keys[i], child_node);
		if (ret != 0)
			pr_err("%s register fail\n", func_keys[i].devices_names);
		i++;
	}

	platform_set_drvdata(pdev, func_keys);
	dev_info(&pdev->dev, "%s success\n", __func__);
	return 0;
}

static int func_key_remove(struct platform_device *pdev)
{
	unsigned int i;
	unsigned int key_nums;
	struct keypad_key_devices *func_keys = platform_get_drvdata(pdev);

	if (!func_keys) {
		pr_err("%s:failed to get drvdata\n", __func__);
		return -EINVAL;
	}
	key_nums = sizeof(*func_keys) / sizeof(struct keypad_key_devices);
	for (i = 0; i < key_nums; i++)
		(void)keypad_unregister_dev(&func_keys[i]);

	platform_set_drvdata(pdev, NULL);

	return 0;
}

#ifdef CONFIG_PM
static int gpio_func_key_suspend(
	struct platform_device *pdev, pm_message_t state)
{
	dev_info(&pdev->dev, "%s: suspend +\n", __func__);
	dev_info(&pdev->dev, "%s: suspend -\n", __func__);

	return 0;
}

static int gpio_func_key_resume(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "%s: resume +\n", __func__);
	dev_info(&pdev->dev, "%s: resume -\n", __func__);

	return 0;
}
#endif

struct platform_driver func_key_driver = {
	.probe = func_key_probe,
	.remove = func_key_remove,
	.driver = {
		.name = "func_key",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(func_key_match),
	},
#ifdef CONFIG_PM
	.suspend = gpio_func_key_suspend,
	.resume = gpio_func_key_resume,
#endif
};

module_platform_driver(func_key_driver);

MODULE_DESCRIPTION("Func key platform driver");
MODULE_LICENSE("GPL v2");
