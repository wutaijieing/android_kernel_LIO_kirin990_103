/*
 * audio_gpio_ctl.c
 *
 * audio_gpio_ctl driver
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
 */

#include "audio_gpio_ctl.h"

#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_device.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/workqueue.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>

#define ANAHS_EVENT_SIZE 64

static struct audio_gpio_ctrl_priv *g_audio_gpio_priv = NULL;
static int g_support_tdd = 0;
static int g_not_support_anahs = 0;

static int audio_gpio_tdd_ctl(struct audio_gpio_ctrl_priv *gpio_priv,
	void __user *arg)
{
	int ret;
	int gpio_val;

	ret = copy_from_user(&gpio_val, arg, sizeof(int));
	if (ret != 0) {
		pr_err("%s: get gpio_val state fail\n", __func__);
		return ret;
	}

	pr_info("%s: gpio_val = %d\n", __func__, gpio_val);

	ret = gpio_direction_output(gpio_priv->tdd_gpio, !!gpio_val);
	if (ret != 0) {
		pr_err("%s: set gpio value fail\n", __func__);
		return ret;
	}

	return 0;
}

static int audio_gpio_ctl_do_ioctl(struct file *file, unsigned int cmd,
	void __user *arg, int compat_mode)
{
	int ret = 0;

	if (!file || !file->private_data) {
		pr_err("%s: invalid argument\n", __func__);
		return -EINVAL;
	}

	if (!g_audio_gpio_priv) {
		pr_err("%s: g_audio_gpio_priv is null\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&g_audio_gpio_priv->do_ioctl_lock);

	switch (cmd) {
	case TDD_CTRL:
		if (g_support_tdd) {
			ret = audio_gpio_tdd_ctl(g_audio_gpio_priv, arg);
		}
		break;
	default:
		pr_err("%s: not support cmd = 0x%x\n", __func__, cmd);
		ret = -EIO;
		break;
	}

	mutex_unlock(&g_audio_gpio_priv->do_ioctl_lock);

	return ret;
}

static long audio_gpio_ctl_ioctl(struct file *file, unsigned int command,
	unsigned long arg)
{
	return audio_gpio_ctl_do_ioctl(file, command, (void __user *)(uintptr_t)arg, 0);
}

#ifdef CONFIG_COMPAT
static long audio_gpio_ctl_ioctl_compat(struct file *file,
	unsigned int command, unsigned long arg)
{
	return audio_gpio_ctl_do_ioctl(file, command,
		compat_ptr((unsigned int)arg), 1);
}
#else
#define audio_gpio_ctl_ioctl_compat NULL /* function pointer */
#endif /* CONFIG_COMPAT */

static const struct file_operations audio_gpio_ctl_fops = {
	.owner          = THIS_MODULE,
	.open           = simple_open,
	.unlocked_ioctl = audio_gpio_ctl_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = audio_gpio_ctl_ioctl_compat,
#endif /* CONFIG_COMPAT */
};

static struct miscdevice audio_gpio_ctl_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "audio_gpio_ctl",
	.fops  = &audio_gpio_ctl_fops,
};

static int parse_dts_config(struct device_node *node, struct audio_gpio_ctrl_priv *gpio_priv)
{
	int ret;

	gpio_priv->tdd_gpio = of_get_named_gpio(node, "tdd_gpio", 0);
	if (!gpio_is_valid(gpio_priv->tdd_gpio)) {
		pr_err("%s: get tdd gpio fail\n", __func__);
		return -1;
	}

	ret = gpio_request(gpio_priv->tdd_gpio, "tdd_gpio");
	if (ret < 0) {
		pr_err("%s: gpio tdd request fail\n", __func__);
		return -1;
	}

	gpio_direction_output(gpio_priv->tdd_gpio, 0);

	return ret;
}

static int anahs_status_report(struct notifier_block *nb, unsigned long event, void *data)
{
	char kdata[ANAHS_EVENT_SIZE];
	char *envp[] = { kdata, NULL };
	int ret;

	if (!g_not_support_anahs || !g_audio_gpio_priv)
		return NOTIFY_OK;

	switch (event) {
	case POWER_NE_HW_USB_HEADPHONE:
		snprintf(kdata, sizeof(kdata), "ANAHS=INSERT");
		break;
	case POWER_NE_HW_USB_HEADPHONE_OUT:
		snprintf(kdata, sizeof(kdata), "ANAHS=REMOVE");
		break;
	default:
		return NOTIFY_OK;
	}

	pr_info("%s: report uevent: %s\n", __func__, kdata);
	ret = kobject_uevent_env(&g_audio_gpio_priv->dev->kobj, KOBJ_CHANGE, envp);
	if (ret < 0)
		pr_err("%s: uevent failed ret: %d, kdata: %s\n", __func__, ret, kdata);

	return NOTIFY_OK;
}

static void read_dts_support_config(struct device_node *node)
{
	const char *tdd = NULL;
	const char *anahs = NULL;

	of_property_read_string(node, "tdd_support", &tdd);
	if (tdd) {
		if (strcmp(tdd, "true") == 0)
			g_support_tdd = 1;
	}

	of_property_read_string(node, "anahs_not_support", &anahs);
	if (anahs) {
		if (strcmp(anahs, "true") == 0)
			g_not_support_anahs = 1;
	}

	pr_info("%s: tdd %d, not_anahs %d\n", __func__, g_support_tdd, g_not_support_anahs);
}

static int audio_gpio_ctl_probe(struct platform_device *pdev)
{
	struct audio_gpio_ctrl_priv *gpio_priv = NULL;
	int ret;

	pr_info("%s: enter\n", __func__);

	if (!pdev) {
		pr_err("%s: invalid argument\n", __func__);
		return -EINVAL;
	}

	gpio_priv = kzalloc(sizeof(*gpio_priv), GFP_KERNEL);
	if (!gpio_priv)
		return -ENOMEM;

	platform_set_drvdata(pdev, gpio_priv);

	mutex_init(&gpio_priv->do_ioctl_lock);

	g_audio_gpio_priv = gpio_priv;

	gpio_priv->dev = &pdev->dev;

	/* Whether TDD and analog headsets are supported */
	read_dts_support_config(pdev->dev.of_node);
	if (g_not_support_anahs) {
		gpio_priv->event_nb.notifier_call = anahs_status_report;
		ret = power_event_bnc_register(POWER_BNT_HW_USB, &gpio_priv->event_nb);
		if (ret) {
			pr_err("%s: anahs event register fail\n", __func__);
			goto err_out;
		}
	}

	if (g_support_tdd) {
		ret = parse_dts_config(pdev->dev.of_node, gpio_priv);
		if (ret < 0)
			pr_err("%s: parse_dts_config failed, %d\n", __func__, ret);

		ret = misc_register(&audio_gpio_ctl_miscdev);
		if (ret != 0) {
			pr_err("%s: register miscdev failed, %d\n", __func__, ret);
			goto err_out;
		}
	}
	pr_info("%s: end success\n", __func__);
	return 0;

err_out:
	kfree(gpio_priv);
	g_audio_gpio_priv = NULL;
	platform_set_drvdata(pdev, NULL);
	return ret;
}

static int audio_gpio_ctl_remove(struct platform_device *pdev)
{
	struct audio_gpio_ctrl_priv *gpio_priv = NULL;

	if (!pdev) {
		pr_err("%s: invalid argument\n", __func__);
		return -EINVAL;
	}

	misc_deregister(&audio_gpio_ctl_miscdev);

	gpio_priv = platform_get_drvdata(pdev);
	if (!gpio_priv) {
		pr_err("%s: pakit_priv invalid argument\n", __func__);
		return -EINVAL;
	}
	platform_set_drvdata(pdev, NULL);

	kfree(gpio_priv);
	g_audio_gpio_priv = NULL;

	return 0;
}

static const struct of_device_id audio_gpio_ctl_match[] = {
	{ .compatible = "huawei,audio_gpio_ctl", },
	{},
};
MODULE_DEVICE_TABLE(of, audio_gpio_ctl_match);

static struct platform_driver audio_gpio_ctl_driver = {
	.driver = {
		.name           = "audio_gpio_ctl",
		.owner          = THIS_MODULE,
		.of_match_table = of_match_ptr(audio_gpio_ctl_match),
	},
	.probe    = audio_gpio_ctl_probe,
	.remove   = audio_gpio_ctl_remove,
};

static int __init audio_gpio_ctl_init(void)
{
	int ret;

	ret = platform_driver_register(&audio_gpio_ctl_driver);
	if (ret != 0)
		pr_err("%s: platform_driver_register failed, %d\n",
			__func__, ret);

	return ret;
}

static void __exit audio_gpio_ctl_exit(void)
{
	platform_driver_unregister(&audio_gpio_ctl_driver);
}

late_initcall(audio_gpio_ctl_init);
module_exit(audio_gpio_ctl_exit);

MODULE_DESCRIPTION("audio gpio ctl driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
MODULE_LICENSE("GPL v2");

