// SPDX-License-Identifier: GPL-2.0
/*
 * power_gpio.c
 *
 * gpio interface for power module
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <chipset_common/hwpower/common_module/power_debug.h>
#include <chipset_common/hwpower/common_module/power_gpio.h>
#include <chipset_common/hwpower/common_module/power_sysfs.h>
#include <chipset_common/hwpower/common_module/power_interface.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG power_gpio
HWLOG_REGIST();

static struct device *g_power_gpio_dev;
static int g_power_gpio_enable;
static struct list_head g_power_gpio_list;
static DEFINE_SPINLOCK(g_power_gpio_list_slock);

#if (defined(CONFIG_HUAWEI_POWER_DEBUG) && defined(CONFIG_SYSFS))
static const char * const g_power_gpio_op_user_table[] = {
	[POWER_GPIO_OP_USER_SHELL] = "shell",
	[POWER_GPIO_OP_USER_KERNEL] = "kernel",
};

static int power_gpio_check_op_user(const char *str)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(g_power_gpio_op_user_table); i++) {
		if (!strcmp(str, g_power_gpio_op_user_table[i]))
			return 0;
	}

	hwlog_err("invalid user_str=%s\n", str);
	return -EPERM;
}
#endif /* CONFIG_HUAWEI_POWER_DEBUG && CONFIG_SYSFS */

static void power_gpio_add_list(const char *prop, const char *label, int gpio)
{
	struct power_gpio_attr *new_attr = NULL;
	unsigned long flags;

	if (!g_power_gpio_list.next) {
		hwlog_info("list init\n");
		INIT_LIST_HEAD(&g_power_gpio_list);
	}

	new_attr = kzalloc(sizeof(*new_attr), GFP_KERNEL);
	if (!new_attr)
		return;

	spin_lock_irqsave(&g_power_gpio_list_slock, flags);
	strncpy(new_attr->prop, prop, POWER_GPIO_ATTR_NAME_LEN - 1);
	new_attr->prop[POWER_GPIO_ATTR_NAME_LEN - 1] = '\0';
	strncpy(new_attr->label, label, POWER_GPIO_ATTR_NAME_LEN - 1);
	new_attr->label[POWER_GPIO_ATTR_NAME_LEN - 1] = '\0';
	new_attr->gpio = gpio;
	list_add(&new_attr->list, &g_power_gpio_list);
	spin_unlock_irqrestore(&g_power_gpio_list_slock, flags);
}

static void power_gpio_del_list(void)
{
	struct list_head *pos = NULL;
	struct list_head *next = NULL;
	struct power_gpio_attr *pattr = NULL;
	unsigned long flags;

	spin_lock_irqsave(&g_power_gpio_list_slock, flags);
	list_for_each_safe(pos, next, &g_power_gpio_list) {
		pattr = list_entry(pos, struct power_gpio_attr, list);
		list_del(&pattr->list);
		kfree(pattr);
	}
	INIT_LIST_HEAD(&g_power_gpio_list);
	spin_unlock_irqrestore(&g_power_gpio_list_slock, flags);
}

static ssize_t power_gpio_dbg_info_show(void *dev_data, char *buf, size_t size)
{
	struct list_head *pos = NULL;
	struct list_head *next = NULL;
	struct power_gpio_attr *pattr = NULL;
	unsigned long flags;
	int ret;
	int len = 0;

	ret = snprintf(buf + len, PAGE_SIZE - len, "%-4s %-32s %-32s\n",
		"gpio", "property", "label");
	if (ret < 0)
		return -EINVAL;
	len += ret;

	spin_lock_irqsave(&g_power_gpio_list_slock, flags);
	list_for_each_safe(pos, next, &g_power_gpio_list) {
		pattr = list_entry(pos, struct power_gpio_attr, list);
		ret = snprintf(buf + len, PAGE_SIZE - len, "%-4d %-32s %-32s\n",
			 pattr->gpio, pattr->prop, pattr->label);
		if (ret < 0)
			break;
		len += ret;
	}
	spin_unlock_irqrestore(&g_power_gpio_list_slock, flags);

	return len;
}

static struct device_node *power_gpio_get_compatible(const char *compatible)
{
	struct device_node *np = NULL;

	if (!compatible) {
		hwlog_err("compatible is null\n");
		return NULL;
	}

	np = of_find_compatible_node(NULL, NULL, compatible);
	if (!np) {
		hwlog_err("compatible %s get fail\n", compatible);
		return NULL;
	}

	return np;
}

int power_gpio_request(struct device_node *np,
	const char *prop, const char *label, unsigned int *gpio)
{
	if (!np || !prop || !label || !gpio) {
		hwlog_err("np or prop or label or gpio is null\n");
		return -EINVAL;
	}

	*gpio = of_get_named_gpio(np, prop, 0);
	hwlog_info("%s: %s=%d\n", label, prop, *gpio);

	if (!gpio_is_valid(*gpio)) {
		hwlog_err("gpio %d is not valid\n", *gpio);
		return -EINVAL;
	}

	if (gpio_request(*gpio, label)) {
		hwlog_err("gpio %d request fail\n", *gpio);
		return -EINVAL;
	}

	power_gpio_add_list(prop, label, *gpio);
	return 0;
}

int power_gpio_config_input(struct device_node *np,
	const char *prop, const char *label, unsigned int *gpio)
{
	if (power_gpio_request(np, prop, label, gpio))
		return -EINVAL;

	if (gpio_direction_input(*gpio)) {
		gpio_free(*gpio);
		hwlog_err("gpio %d set input fail\n", *gpio);
		return -EINVAL;
	}

	return 0;
}

int power_gpio_config_output(struct device_node *np,
	const char *prop, const char *label, unsigned int *gpio, int value)
{
	if (power_gpio_request(np, prop, label, gpio))
		return -EINVAL;

	if (gpio_direction_output(*gpio, value)) {
		gpio_free(*gpio);
		hwlog_err("gpio %d set output fail\n", *gpio);
		return -EINVAL;
	}

	return 0;
}

int power_gpio_config_interrupt(struct device_node *np,
	const char *prop, const char *label, unsigned int *gpio, int *irq)
{
	if (!irq) {
		hwlog_err("irq is null\n");
		return -EINVAL;
	}

	if (power_gpio_request(np, prop, label, gpio))
		return -EINVAL;

	if (gpio_direction_input(*gpio)) {
		gpio_free(*gpio);
		hwlog_err("gpio %d set input fail\n", *gpio);
		return -EINVAL;
	}

	*irq = gpio_to_irq(*gpio);
	if (*irq < 0) {
		gpio_free(*gpio);
		hwlog_err("gpio %d map to irq fail\n", *gpio);
		return -EINVAL;
	}

	hwlog_info("%s: irq=%d\n", label, *irq);
	return 0;
}

int power_gpio_config_output_compatible(const char *comp,
	const char *prop, const char *label, unsigned int *gpio, int value)
{
	struct device_node *np = power_gpio_get_compatible(comp);

	if (!np)
		return -EINVAL;

	return power_gpio_config_output(np, prop, label, gpio, value);
}

#if (defined(CONFIG_HUAWEI_POWER_DEBUG) && defined(CONFIG_SYSFS))
static ssize_t power_gpio_set_info(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	char user[POWER_GPIO_RD_BUF_SIZE] = { 0 };
	unsigned int gpio;
	unsigned int value;

	if (!g_power_gpio_enable)
		return -EINVAL;

	/* reserve 2 bytes to prevent buffer overflow */
	if (count >= (POWER_GPIO_RD_BUF_SIZE - 2)) {
		hwlog_err("input too long\n");
		return -EINVAL;
	}

	/* 3: the fields of "user gpio value" */
	if (sscanf(buf, "%s %d %d", user, &gpio, &value) != 3) {
		hwlog_err("unable to parse input:%s\n", buf);
		return -EINVAL;
	}

	if (power_gpio_check_op_user(user))
		return -EINVAL;

	/* gpio must be (1, 500) */
	if ((gpio < 1) || (gpio > 500)) {
		hwlog_err("invalid gpio=%d\n", gpio);
		return -EINVAL;
	}

	/* value must be 0 or 1, 0: low, 1: high */
	if ((value < 0) || (value > 1)) {
		hwlog_err("invalid value=%d\n", value);
		return -EINVAL;
	}

	hwlog_info("set: user=%s gpio=%d value=%d\n", user, gpio, value);

	if (gpio_direction_output(gpio, value)) {
		hwlog_err("gpio %d set output fail\n", gpio);
		return -EINVAL;
	}

	return count;
}

static DEVICE_ATTR(info, 0220, NULL, power_gpio_set_info);

static struct attribute *power_gpio_attributes[] = {
	&dev_attr_info.attr,
	NULL,
};

static const struct attribute_group power_gpio_attr_group = {
	.attrs = power_gpio_attributes,
};

static struct device *power_gpio_create_group(void)
{
	return power_sysfs_create_group("hw_power", "power_gpio",
		&power_gpio_attr_group);
}

static void power_gpio_remove_group(struct device *dev)
{
	power_sysfs_remove_group(dev, &power_gpio_attr_group);
}
#else
static inline struct device *power_gpio_create_group(void)
{
	return NULL;
}

static inline void power_gpio_remove_group(struct device *dev)
{
}
#endif /* CONFIG_HUAWEI_POWER_DEBUG && CONFIG_SYSFS */

static int power_gpio_set_enable(unsigned int val)
{
	if (!g_power_gpio_dev)
		return -EPERM;

	g_power_gpio_enable = val;
	return 0;
}

static int power_gpio_get_enable(unsigned int *val)
{
	if (!val)
		return -EPERM;

	*val = g_power_gpio_enable;
	return 0;
}

static struct power_if_ops power_gpio_if_ops = {
	.set_enable = power_gpio_set_enable,
	.get_enable = power_gpio_get_enable,
	.type_name = "power_gpio",
};

static int __init power_gpio_init(void)
{
	g_power_gpio_dev = power_gpio_create_group();
	power_if_ops_register(&power_gpio_if_ops);
	power_dbg_ops_register("power_gpio", "info", (void *)g_power_gpio_dev,
		power_gpio_dbg_info_show, NULL);
	return 0;
}

static void __exit power_gpio_exit(void)
{
	power_gpio_remove_group(g_power_gpio_dev);
	g_power_gpio_dev = NULL;
	power_gpio_del_list();
}

device_initcall_sync(power_gpio_init);
module_exit(power_gpio_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("gpio for power module driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
