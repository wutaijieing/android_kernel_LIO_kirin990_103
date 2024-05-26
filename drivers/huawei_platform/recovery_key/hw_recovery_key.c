// SPDX-License-Identifier: GPL-2.0
/*
 * hw_recovery_key.c
 *
 * driver for hw_recovery_key
 *
 * Copyright (c) 2022-2022 Huawei Technologies Co., Ltd.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/timer.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/device.h>
#include <linux/pm_wakeup.h>
#include <linux/version.h>
#include <asm/irq.h>
#include <linux/reboot.h>
#include <huawei_platform/log/hw_log.h>

#define HW_RECOVERY_GPIO_HIGH_VALUE                 1
#define HW_RECOVERY_GPIO_LOW_VALUE                  0
#define HW_RECOVERY_TIME_DEBOUNCE                   15
#define HW_RECOVERY_TIME_WAKEUP                     50
#define HW_RECOVERY_WAIT_DELAY_DEFAULT              5000
#define HW_RECOVERY_RESET_USER_CMD_STR              "resetuser"

#define HWLOG_TAG recovery_gpio_key
HWLOG_REGIST();

struct hw_recovery_key {
	int recovery_gpio;
	int recovery_irq;
	int wait_delay;
	struct mutex reboot_lock;
	struct wakeup_source *lock;
	struct delayed_work recovery_work;
	struct delayed_work do_recovery_work;
	struct timer_list recovery_timer;
};

static void reboot_reset_factory(struct hw_recovery_key *key)
{
	mutex_lock(&key->reboot_lock);
	kernel_restart(HW_RECOVERY_RESET_USER_CMD_STR);
	mutex_unlock(&key->reboot_lock);
}

static struct wakeup_source * hw_recovery_wakeup_source_register(struct device *dev,
	const char *name)
{
	struct wakeup_source *ws;

	if (!dev || !name) {
		hwlog_err("dev or name is null\n");
		return NULL;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 157))
	ws = wakeup_source_register(dev, name);
#else
	ws = wakeup_source_register(name);
#endif
	if (!ws)
		hwlog_err("wakeup source register fail\n");

	return ws;
}

static void hw_recovery_wakeup_source_unregister(struct wakeup_source *ws)
{
	if (ws && ws->name)
		wakeup_source_unregister(ws);
}

static void hw_recovery_wakeup_lock(struct wakeup_source *ws)
{
	if (ws && ws->name && !ws->active) {
		hwlog_info("wakeup source %s lock\n", ws->name);
		__pm_stay_awake(ws);
	}
}

static void hw_recovery_wakeup_unlock(struct wakeup_source *ws)
{
	if (ws && ws->name && ws->active) {
		hwlog_info("wakeup source %s lock\n", ws->name);
		__pm_relax(ws);
	}
}

static void hw_recovery_key_work(struct work_struct *work)
{
	int key_value;
	struct hw_recovery_key *key = container_of(work,
		struct hw_recovery_key, recovery_work.work);

	if (!gpio_is_valid(key->recovery_gpio))
		return ;

	key_value = gpio_get_value(key->recovery_gpio);
	if (key_value == HW_RECOVERY_GPIO_LOW_VALUE) {
		schedule_delayed_work(&key->do_recovery_work,
			msecs_to_jiffies(key->wait_delay));
		hwlog_info("recovery key pressed\n");
	} else if (key_value == HW_RECOVERY_GPIO_HIGH_VALUE) {
		hw_recovery_wakeup_unlock(key->lock);
		cancel_delayed_work(&key->do_recovery_work);
		hwlog_info("recovery key released!\n");
	}
}

static void hw_do_recovery_work(struct work_struct *work)
{
	int key_value;
	struct hw_recovery_key *key = container_of(work,
		struct hw_recovery_key, do_recovery_work.work);

	key_value = gpio_get_value(key->recovery_gpio);
	if (key_value == HW_RECOVERY_GPIO_LOW_VALUE) {
		reboot_reset_factory(key);
		hwlog_info("recovery work done!\n");
	}
}

static void gpio_recovery_key_timer(struct timer_list *data)
{
	int key_value;
	struct hw_recovery_key *key = from_timer(key, data, recovery_timer);

	key_value = gpio_get_value(key->recovery_gpio);
	if (key_value == HW_RECOVERY_GPIO_LOW_VALUE)
		hw_recovery_wakeup_lock(key->lock);

	schedule_delayed_work(&key->recovery_work, 0);
}

static irqreturn_t hw_recovery_key_irq_handler(int irq, void *dev_id)
{
	struct hw_recovery_key *key = (struct hw_recovery_key *)dev_id;

	mod_timer(&key->recovery_timer, jiffies +
		msecs_to_jiffies(HW_RECOVERY_TIME_DEBOUNCE));
	__pm_wakeup_event(key->lock,
		jiffies_to_msecs(HW_RECOVERY_TIME_WAKEUP));

	return IRQ_HANDLED;
}

static int hw_recovery_key_parse_dt(struct platform_device *pdev)
{
	int ret;
	struct device_node *np = pdev->dev.of_node;
	struct hw_recovery_key *key = platform_get_drvdata(pdev);

	if (!key)
		return -EINVAL;

	ret = of_property_read_u32(np, "wait_delay_time", &key->wait_delay);
	if (ret)
		key->wait_delay = HW_RECOVERY_WAIT_DELAY_DEFAULT;

	key->recovery_gpio = of_get_named_gpio(np, "gpio-recovery", 0);
	if (!gpio_is_valid(key->recovery_gpio)) {
		hwlog_err("recovery gpio error,check DTS\n");
		return  -EINVAL;
	}

	return 0;
}

static int hw_recovery_irq_init(struct platform_device *pdev)
{
	int ret;
	struct hw_recovery_key *key = platform_get_drvdata(pdev);

	if (!key)
		return -EINVAL;

	ret = gpio_request(key->recovery_gpio, "hw_recovery_gpio");
	if (ret)
		return ret;

	ret = gpio_direction_input(key->recovery_gpio);
	if (ret)
		goto err_gpio_setting;

	key->recovery_irq = gpio_to_irq((unsigned int)key->recovery_gpio);
	if (key->recovery_irq < 0) {
		hwlog_err("Failed to get gpio key press irq\n");
		ret = key->recovery_irq;
		goto err_gpio_setting;
	}

	/* falling edge : key pressed; rising edge key released */
	ret = request_irq(key->recovery_irq, hw_recovery_key_irq_handler,
		IRQF_NO_SUSPEND | IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
		pdev->name, key);
	if (ret) {
		hwlog_err("Failed to request press interupt handler\n");
		goto err_gpio_setting;
	}
	return 0;

err_gpio_setting:
	gpio_free(key->recovery_gpio);
	hwlog_err("recovery gpio irq init fail\n");
	return ret;
}

static const struct of_device_id hw_recovery_key_match[] = {
	{.compatible = "huawei,recovery-key" },
	{},
};

static int hw_recovery_key_probe(struct platform_device* pdev)
{
	int ret;
	struct hw_recovery_key *gpio_key;

	if (!pdev) {
		hwlog_err("pdev null!\n");
		return -EINVAL;
	}

	gpio_key = devm_kzalloc(&pdev->dev, sizeof(struct hw_recovery_key),
		GFP_KERNEL);
	if (IS_ERR_OR_NULL(gpio_key)) {
		hwlog_err("failed to allocate memory\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, gpio_key);
	INIT_DELAYED_WORK(&(gpio_key->recovery_work), hw_recovery_key_work);
	INIT_DELAYED_WORK(&(gpio_key->do_recovery_work), hw_do_recovery_work);
	gpio_key->lock = hw_recovery_wakeup_source_register(&pdev->dev,
		"hw_recovery_key_lock");
	mutex_init(&gpio_key->reboot_lock);

	ret = hw_recovery_key_parse_dt(pdev);
	if (ret)
		goto err_parse_dt;

	ret = hw_recovery_irq_init(pdev);
	if (ret)
		goto err_parse_dt;

	timer_setup(&(gpio_key->recovery_timer), gpio_recovery_key_timer, (uintptr_t)gpio_key);
	device_init_wakeup(&pdev->dev, true);
	return 0;

err_parse_dt:
	mutex_destroy(&gpio_key->reboot_lock);
	hw_recovery_wakeup_source_unregister(gpio_key->lock);
	devm_kfree(&pdev->dev, gpio_key);
	return ret;
}

static int hw_recovery_key_remove(struct platform_device* pdev)
{
	struct hw_recovery_key *gpio_key = platform_get_drvdata(pdev);

	if (!gpio_key) {
		hwlog_err("get recovery key pointer failed!\n");
		return -EINVAL;
	}

	free_irq(gpio_key->recovery_irq, gpio_key);
	gpio_free((unsigned int) gpio_key->recovery_gpio);
	cancel_delayed_work(&gpio_key->recovery_work);
	cancel_delayed_work(&gpio_key->do_recovery_work);
	mutex_destroy(&gpio_key->reboot_lock);
	hw_recovery_wakeup_source_unregister(gpio_key->lock);
	platform_set_drvdata(pdev, NULL);
	devm_kfree(&pdev->dev, gpio_key);

	return 0;
}

#ifdef CONFIG_PM
static int hw_recovery_key_suspend(struct platform_device *pdev, pm_message_t state)
{
	hwlog_info("suspend +\n", __func__);
	return 0;
}

static int hw_recovery_key_resume(struct platform_device *pdev)
{
	hwlog_info("resume +\n", __func__);
	return 0;
}
#endif /* CONFIG_PM */

static struct platform_driver hw_recovery_key_driver = {
	.probe = hw_recovery_key_probe,
	.remove = hw_recovery_key_remove,
	.driver = {
		.name = "hw_recovery_key",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hw_recovery_key_match),
	},
#ifdef CONFIG_PM
	.suspend = hw_recovery_key_suspend,
	.resume = hw_recovery_key_resume,
#endif /* CONFIG_PM */
};

static int __init hw_recovery_key_init(void)
{
	int ret;

	ret = platform_driver_register(&hw_recovery_key_driver);
	if (ret)
		hwlog_err("platform_driver_register failed");

	return ret;
}

static void __exit hw_recovery_key_exit(void)
{
	platform_driver_unregister(&hw_recovery_key_driver);
}

module_init(hw_recovery_key_init);
module_exit(hw_recovery_key_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("huawei recovery key platform driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
