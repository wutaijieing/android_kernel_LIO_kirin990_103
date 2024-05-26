/*
 * flashlights-sgm3785.c
 *
 * driver for sgm3785
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "hw_flash.h"
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/list.h>
#include <linux/pwm.h>

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif

/* define device tree */
#define SGM3785_DTNAME "huawei,sgm3785"

/* Device name */
#define SGM3785_NAME "huawei,sgm3785"

#define SGM3785_TIMEOUT_TIME 100
#define SGM3785_TORCH_MAX_RT_CUR 190
#define SGM3785_STATE_PERIOD 13333
#define SGM3785_MAX_DUTY_CYCLE 13333
#define SGM3785_TORCH_DUTY_CYCLE 10000
#define SGM3785_STEP_DUTY_CYCLE 13 // 13 * 1025 <= 13333
#define SGM3785_MAX_LEVEL 1025
#define SGM3785_MIN_LEVEL 0
#define SGM3785_DEFAULT_LEVEL 100

#define SGM3785_HARDWARE_TIMER 200
#define SGM3785_TORCH_MODE_DELAY 6

#define SGM3785_GPIO_HIGH 1
#define SGM3785_GPIO_LOW 0

define_kernel_flash_mutex(sgm3785);

static struct work_struct g_sgm3785_gpio_work;
static unsigned int g_enf_gpio_id;
static unsigned int g_enm_gpio_id;
static struct hw_flash_ctrl_t g_sgm3785_ctrl;

/* platform data */
struct sgm3785_gpio_platform_data {
	struct device *dev;
	struct pwm_device *pwm_dev;

	struct pinctrl *pinctrl;
	struct pinctrl_state *pinctrl_def;
	struct pinctrl_state *pinctrl_idle;
};
static struct sgm3785_gpio_platform_data g_sgm3785_pdata;

/*
 * Pinctrl configuration
 */
static int sgm3785_gpio_pinctrl_init(struct platform_device *pdev)
{
	int ret;
	struct device_node *node = pdev->dev.of_node;
	static struct sgm3785_gpio_platform_data *sgm3785_pdata = NULL;

	sgm3785_pdata = devm_kzalloc(&pdev->dev,
		sizeof(*sgm3785_pdata), GFP_KERNEL);
	if ((!sgm3785_pdata) || (!node)) {
		cam_err("The node or sgm3785_pdata fail\n");
		return -ENOMEM;
	}

	cam_info("%s enter\n", __func__);
	/* get pinctrl */
	sgm3785_pdata->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(sgm3785_pdata->pinctrl)) {
		cam_err("Failed to get flashlight pinctrl\n");
		ret = PTR_ERR(sgm3785_pdata->pinctrl);
		return ret;
	}

	/* get default mode */
	sgm3785_pdata->pinctrl_def =
		pinctrl_lookup_state(sgm3785_pdata->pinctrl, "default");
	if (IS_ERR(sgm3785_pdata->pinctrl_def)) {
		cam_err("Failed to init sgm3785_pdata->pinctrl_def\n");
		ret = PTR_ERR(sgm3785_pdata->pinctrl_def);
		return ret;
	}

	/* get idle mode */
	sgm3785_pdata->pinctrl_idle =
		pinctrl_lookup_state(sgm3785_pdata->pinctrl, "idle");
	if (IS_ERR(sgm3785_pdata->pinctrl_idle)) {
		cam_err("Failed to init sgm3785_pdata->pinctrl_idle\n");
		ret = PTR_ERR(sgm3785_pdata->pinctrl_idle);
		return ret;
	}

	g_sgm3785_pdata.pinctrl = sgm3785_pdata->pinctrl;
	g_sgm3785_pdata.pinctrl_def = sgm3785_pdata->pinctrl_def;
	g_sgm3785_pdata.pinctrl_idle = sgm3785_pdata->pinctrl_idle;
	g_enf_gpio_id = of_get_named_gpio(node, "enf_gpio", 0);
	if (g_enf_gpio_id < 0) {
		cam_err("could not get enf_gpio number\n");
		return -1;
	}
	g_enm_gpio_id = of_get_named_gpio(node, "enm_gpio", 0);
	if (g_enm_gpio_id < 0) {
		cam_err("could not get enm_gpio number\n");
		return -1;
	}
	ret = gpio_request(g_enf_gpio_id, NULL);
	if (ret < 0) {
		cam_err("could not reuquest gpio-%d\n", g_enf_gpio_id);
		return -1;
	}
	ret = gpio_request(g_enm_gpio_id, NULL);
	if (ret < 0) {
		cam_err("could not reuquest gpio-%d\n", g_enm_gpio_id);
		return -1;
	}

	return ret;
}

static int hw_sgm3785_flash_mode(struct hw_flash_ctrl_t *flash_ctrl, int data)
{
	int ret;
	int value_enf;
	int value_enm;
	int temp_duty;
	struct pwm_state state;
	struct sgm3785_gpio_platform_data *pdata = &g_sgm3785_pdata;

	if (data > SGM3785_MAX_LEVEL)
		data = SGM3785_MAX_LEVEL;
	else if (data < SGM3785_MIN_LEVEL)
		data = SGM3785_DEFAULT_LEVEL;

	temp_duty = data * SGM3785_STEP_DUTY_CYCLE;
	value_enf = gpio_get_value(g_enf_gpio_id);
	value_enm = gpio_get_value(g_enm_gpio_id);
	cam_debug("%s value_enf = %d, value_enm = %d\n",
		__func__, value_enf, value_enm);

	ret = pinctrl_select_state(pdata->pinctrl, pdata->pinctrl_def);

	pdata->pwm_dev->chip->ops->get_state(pdata->pwm_dev->chip,
		pdata->pwm_dev, &state);

	state.period = SGM3785_STATE_PERIOD;
	state.duty_cycle = temp_duty;
	state.polarity = PWM_POLARITY_NORMAL;
	state.enabled = true;

	cam_info("%s set state period = %d, duty_cycle = %d\n",
		__func__, state.period, state.duty_cycle);
	ret += pdata->pwm_dev->chip->ops->apply(pdata->pwm_dev->chip,
		pdata->pwm_dev, &state);

	gpio_set_value(g_enf_gpio_id, SGM3785_GPIO_HIGH);

	cam_info("%s end, ret = %d\n", __func__, ret);
	return ret;
}

/*
 * gpio180:from 0 to 0;
 * gpio93:from 0 to 1;
 * msleep(6)
 */
static int hw_sgm3785_torch_mode(struct hw_flash_ctrl_t *flash_ctrl, int data)
{
	gpio_set_value(g_enf_gpio_id, SGM3785_GPIO_LOW);
	gpio_set_value(g_enm_gpio_id, SGM3785_GPIO_HIGH);

	msleep(SGM3785_TORCH_MODE_DELAY);

	cam_info("%s end\n", __func__);
	return 0;
}

static int hw_sgm3785_on(struct hw_flash_ctrl_t *flash_ctrl, void *data)
{
	int ret = 0;
	struct hw_flash_cfg_data *cdata = (struct hw_flash_cfg_data *)data;

	if ((flash_ctrl == NULL) || (cdata == NULL)) {
		cam_err("%s flash_ctrl or cdata is NULL", __func__);
		return -1;
	}

	cam_info("%s mode mode:%d, data:%d\n", __func__, cdata->mode, cdata->data);

	/*
	 * Select to use gpio, Set output and output Low level;
	 */
	struct sgm3785_gpio_platform_data *pdata = &g_sgm3785_pdata;
	ret += pinctrl_select_state(pdata->pinctrl, pdata->pinctrl_idle);

	ret += gpio_direction_output(g_enf_gpio_id, SGM3785_GPIO_LOW);
	ret += gpio_direction_output(g_enm_gpio_id, SGM3785_GPIO_LOW);
	if (ret < 0) {
		cam_err("could not set gpio-%d direction \n",  g_enm_gpio_id);
		ret = -EFAULT;
	}

	/* strobe is a trigger method of FLASH mode */
	if ((cdata->mode == FLASH_MODE) || (cdata->mode == FLASH_STROBE_MODE))
		ret = hw_sgm3785_flash_mode(flash_ctrl, cdata->data);
	else
		ret = hw_sgm3785_torch_mode(flash_ctrl, cdata->data);

	mutex_lock(flash_ctrl->hw_flash_mutex);
	flash_ctrl->state.mode = cdata->mode;
	flash_ctrl->state.data = cdata->data;
	mutex_unlock(flash_ctrl->hw_flash_mutex);

	return ret;
}

static int hw_sgm3785_off(struct hw_flash_ctrl_t *flash_ctrl)
{
	int ret = 0;

	struct pwm_state state;
	struct sgm3785_gpio_platform_data *pdata = &g_sgm3785_pdata;

	cam_debug("%s enter\n", __func__);

	if ((flash_ctrl->state.mode == FLASH_MODE) ||
		(flash_ctrl->state.mode == FLASH_STROBE_MODE)) {
		ret += pinctrl_select_state(pdata->pinctrl, pdata->pinctrl_def);

		state.period = 0;
		state.duty_cycle = 0;
		state.polarity = PWM_POLARITY_NORMAL;
		state.enabled = false;

		cam_info("%s period = %d, duty_cycle = %d\n",
			__func__, state.period, state.duty_cycle);
		ret += pdata->pwm_dev->chip->ops->apply(pdata->pwm_dev->chip,
			pdata->pwm_dev, &state);
	}

	/*
	 * Final setting to GPIO status
	 */
	ret += pinctrl_select_state(pdata->pinctrl, pdata->pinctrl_idle);
	gpio_set_value(g_enf_gpio_id, SGM3785_GPIO_LOW);
	gpio_set_value(g_enm_gpio_id, SGM3785_GPIO_LOW);

	msleep(SGM3785_TORCH_MODE_DELAY);

	mutex_lock(flash_ctrl->hw_flash_mutex);
	flash_ctrl->state.mode = STANDBY_MODE;
	flash_ctrl->state.data = 0;
	mutex_unlock(flash_ctrl->hw_flash_mutex);

	return ret;
}

/*
 * Timer and work queue
 */
static struct hrtimer g_sgm3785_gpio_timer;
static unsigned int g_sgm3785_gpio_timeout_ms;

static void sgm3785_gpio_work_disable(struct work_struct *data)
{
	int ret;

	cam_info("%s enter\n", __func__);

	ret = hw_sgm3785_off(&g_sgm3785_ctrl);
	if (ret)
		cam_err("%s failed\n", __func__);

	return;
}

static enum hrtimer_restart sgm3785_gpio_timer_func(struct hrtimer *timer)
{
	schedule_work(&g_sgm3785_gpio_work);
	return HRTIMER_NORESTART;
}

static int hw_sgm3785_init(struct hw_flash_ctrl_t *flash_ctrl)
{
	cam_debug("%s enter\n", __func__);
	if (flash_ctrl == NULL) {
		cam_err("%s flash_ctrl is NULL", __func__);
		return -1;
	}
	return 0;
}

static int hw_sgm3785_exit(struct hw_flash_ctrl_t *flash_ctrl)
{
	int ret;

	cam_debug("%s enter\n", __func__);
	if (flash_ctrl == NULL) {
		cam_err("%s flash_ctrl is NULL", __func__);
		return -1;
	}

	ret = hw_sgm3785_off(flash_ctrl);
	if (ret < 0) {
		cam_err("%s pmu_led off error", __func__);
		return ret;
	}

	return 0;
}

static int hw_sgm3785_get_dt_data(struct hw_flash_ctrl_t *flash_ctrl)
{
	return 0;
}

static int hw_sgm3785_match(struct hw_flash_ctrl_t *flash_ctrl)
{
	return 0;
}

static void hw_sgm3785_torch_brightness_set(struct led_classdev *cdev,
	enum led_brightness brightness)
{
	struct hw_flash_cfg_data cdata;
	int rc;
	unsigned int led_bright = brightness;

	if (led_bright == STANDBY_MODE) {
		rc = hw_sgm3785_off(&g_sgm3785_ctrl);
		if (rc < 0) {
			cam_err("%s pmu_led off error", __func__);
			return;
		}
	} else {
		cdata.mode = TORCH_MODE;
		cdata.data = brightness - 1;
		rc = hw_sgm3785_on(&g_sgm3785_ctrl, &cdata);
		if (rc < 0) {
			cam_err("%s pmu_led on error", __func__);
			return;
		}
	}
}
#ifdef CAMERA_FLASH_FACTORY_TEST
static ssize_t hw_sgm3785_lightness_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf_s(buf, MAX_ATTRIBUTE_BUFFER_SIZE,
		MAX_ATTRIBUTE_BUFFER_SIZE - 1,
		"mode = %d, data = %d\n",
		g_sgm3785_ctrl.state.mode, g_sgm3785_ctrl.state.data);
	if (rc < 0)
		cam_err("%s snprintf_s wrong ", __func__);

	rc = strlen(buf) + 1;
	return rc;
}

static int hw_sgm3785_param_check(char *buf, unsigned long *param,
	int num_of_par)
{
	char *token = NULL;
	int base;
	int cnt;

	token = strsep(&buf, " ");

	for (cnt = 0; cnt < num_of_par; cnt++) {
		if (token != NULL) {
			if ((strlen(token) > 1) && ((token[1] == 'x') || (token[1] == 'X')))
				base = 16;
			else
				base = 10;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
			if (strict_strtoul(token, base, &param[cnt]) != 0)
#else
			if (kstrtoul(token, base, &param[cnt]) != 0)
#endif
					return -EINVAL;

			token = strsep(&buf, " ");
		} else {
			return -EINVAL;
		}
	}
	return 0;
}

static ssize_t hw_sgm3785_lightness_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct hw_flash_cfg_data cdata = {0};
	unsigned long param[2] = {0};
	int rc;

	rc = hw_sgm3785_param_check((char *)buf, param, 2);
	if (rc < 0) {
		cam_err("%s failed to check param", __func__);
		return rc;
	}

	int flash_id = (int)param[0];

	cdata.mode = (int)param[1];
	cam_info("%s flash_id = %d, cdata.mode = %d",
		__func__, flash_id, cdata.mode);
	/*
	 * bit[0]- rear first flash light.
	 * bit[1]- rear sencond flash light.
	 * bit[2]- front flash light;
	 * dallas product using only rear first flash light
	 */
	if (flash_id != 1) {
		cam_err("%s wrong flash_id = %d", __func__, flash_id);
		return -1;
	}

	if (cdata.mode == STANDBY_MODE) {
		rc = hw_sgm3785_off(&g_sgm3785_ctrl);
		if (rc < 0) {
			cam_err("%s sgm3785 flash off error", __func__);
			return rc;
		}
	} else if (cdata.mode == TORCH_MODE) {
		/*  hardware test requiring the max torch mode current */
		cdata.data = SGM3785_TORCH_MAX_RT_CUR;
		cam_info("%s mode=%d, max_current = %d",
			__func__, cdata.mode, cdata.data);

		rc = hw_sgm3785_on(&g_sgm3785_ctrl, &cdata);
		if (rc < 0) {
			cam_err("%s sgm3785 flash on error", __func__);
			return rc;
		}
	} else {
		cam_err("%s scharger wrong mode = %d", __func__, cdata.mode);
		return -1;
	}

	return count;
}
#endif

static ssize_t hw_sgm3785_flash_mask_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf_s(buf, MAX_ATTRIBUTE_BUFFER_SIZE,
		MAX_ATTRIBUTE_BUFFER_SIZE - 1, "flash_mask_disabled = %d\n",
		g_sgm3785_ctrl.flash_mask_enable);
	if (rc < 0)
		cam_err("%s snprintf_s wrong ", __func__);

	rc = strlen(buf) + 1;

	return rc;
}

static ssize_t hw_sgm3785_flash_mask_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	if (buf[0] == '0')
		g_sgm3785_ctrl.flash_mask_enable = false;
	else
		g_sgm3785_ctrl.flash_mask_enable = true;
	cam_debug("%s flash_mask_enable = %d", __func__,
		g_sgm3785_ctrl.flash_mask_enable);
	return count;
}

#ifdef CAMERA_FLASH_FACTORY_TEST
static struct device_attribute hw_sgm3785_lightness =
	__ATTR(flash_lightness, 0664, hw_sgm3785_lightness_show,
		hw_sgm3785_lightness_store);
#endif

static struct device_attribute hw_sgm3785_flash_mask =
	__ATTR(flash_mask, 0664, hw_sgm3785_flash_mask_show,
		hw_sgm3785_flash_mask_store);

static int hw_sgm3785_register_attribute(struct hw_flash_ctrl_t *flash_ctrl,
	struct device *dev)
{
	int rc;

	if ((flash_ctrl == NULL) || (dev == NULL)) {
		cam_err("%s flash_ctrl or dev is NULL", __func__);
		return -1;
	}

	cam_info("register_attribute enter\n");

	flash_ctrl->cdev_torch.name = "torch";

	flash_ctrl->cdev_torch.brightness_set = hw_sgm3785_torch_brightness_set;
	rc = led_classdev_register((struct device *)dev, &flash_ctrl->cdev_torch);
	if (rc < 0) {
		cam_err("%s failed to register torch classdev", __func__);
		goto err_out;
	}
#ifdef CAMERA_FLASH_FACTORY_TEST
	cam_info(" FACTORY_TEST enter\n");
	rc = device_create_file(dev, &hw_sgm3785_lightness);
	if (rc < 0) {
		cam_err("%s failed to creat lightness attribute", __func__);
		goto err_create_lightness_file;
	}
#endif
	rc = device_create_file(dev, &hw_sgm3785_flash_mask);
	if (rc < 0) {
		cam_err("%s failed to creat flash_mask attribute", __func__);
		goto err_create_flash_mask_file;
	}
	return 0;
err_create_flash_mask_file:
#ifdef CAMERA_FLASH_FACTORY_TEST
	device_remove_file(dev, &hw_sgm3785_lightness);
err_create_lightness_file:
#endif
	led_classdev_unregister(&flash_ctrl->cdev_torch);
err_out:
	return rc;
}

static struct hw_flash_fn_t g_sgm3785_func_tbl = {
	.flash_config = hw_flash_config,
	.flash_init = hw_sgm3785_init,
	.flash_exit = hw_sgm3785_exit,
	.flash_on = hw_sgm3785_on,
	.flash_off = hw_sgm3785_off,
	.flash_match = hw_sgm3785_match,
	.flash_get_dt_data = hw_sgm3785_get_dt_data,
	.flash_register_attribute = hw_sgm3785_register_attribute,
};

static struct hw_flash_ctrl_t g_sgm3785_ctrl = {
	.func_tbl = &g_sgm3785_func_tbl,
	.hw_flash_mutex = &flash_mut_sgm3785,
	.pdata = (void *)&g_sgm3785_pdata,
	.flash_mask_enable = false,
	.state = {
		.mode = STANDBY_MODE,
	},
};

static int sgm3785_gpio_probe(struct platform_device *pdev)
{
	struct sgm3785_gpio_platform_data *pdata = NULL;

	cam_info("Probe start\n");

	pdata = dev_get_platdata(&pdev->dev);
	/* init platform data */
	if (!pdata) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata)
			return -ENOMEM;
		pdev->dev.platform_data = pdata;
	}

	/* init pinctrl */
	if (sgm3785_gpio_pinctrl_init(pdev)) {
		cam_err("Failed to init pinctrl\n");
		return -EFAULT;
	}

	pdata->pwm_dev = devm_pwm_get(&pdev->dev, NULL);
	/*
	 * IS_ERR:Check invalid pointers
	 * PTR_ERR:Convert invalid pointers to error codes
	 * EPROBE_DEFER:517-Driver requests probe retry
	 */
	if (IS_ERR(pdata->pwm_dev)) {
		cam_err("unable to request PWM, trying legacy API\n");
		return -ENOMEM;
	}

	g_sgm3785_pdata.pwm_dev = pdata->pwm_dev;

	/* init work queue */
	INIT_WORK(&g_sgm3785_gpio_work, sgm3785_gpio_work_disable);

	/* init timer */
	hrtimer_init(&g_sgm3785_gpio_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	g_sgm3785_gpio_timer.function = sgm3785_gpio_timer_func;
	g_sgm3785_gpio_timeout_ms = SGM3785_TIMEOUT_TIME;

	/* register flashlight device */
	g_sgm3785_ctrl.pdata = &pdata;
	platform_set_drvdata(pdev, &g_sgm3785_ctrl);
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	/*
	* i2c_test,tps
	* detect current device successful, set the flag as present
	*/
	cam_info("sgm3785 i2c_test tps enter\n");
	(void)set_hw_dev_flag(DEV_I2C_TPS);
#endif

	return hw_flash_platform_probe(pdev, &g_sgm3785_ctrl);
}

static int sgm3785_gpio_remove(struct platform_device *pdev)
{
	gpio_free(g_enf_gpio_id);
	gpio_free(g_enm_gpio_id);
	hrtimer_cancel(&g_sgm3785_gpio_timer);

	return g_sgm3785_ctrl.func_tbl->flash_exit(&g_sgm3785_ctrl);
}
static const struct of_device_id sgm3785_gpio_of_match[] = {
	{ .compatible = SGM3785_DTNAME },
	{},
};
MODULE_DEVICE_TABLE(of, sgm3785_gpio_of_match);

static struct platform_driver sgm3785_gpio_platform_driver = {
	.probe = sgm3785_gpio_probe,
	.remove = sgm3785_gpio_remove,
	.driver = {
		.name = SGM3785_NAME,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(sgm3785_gpio_of_match),
	},
};

static int __init hw_sgm3785_module_init(void)
{
	int ret;

	cam_info("%s erter\n", __func__);

	ret = platform_driver_register(&sgm3785_gpio_platform_driver);
	if (ret) {
		pr_err("Failed to register platform driver\n");
		return ret;
	}

	cam_info("%s Init done\n", __func__);
	return 0;
}

static void __exit hw_sgm3785_module_exit(void)
{
	cam_info("Exit start\n");

	platform_driver_unregister(&sgm3785_gpio_platform_driver);

	cam_info("Exit done\n");
}

module_init(hw_sgm3785_module_init);
module_exit(hw_sgm3785_module_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
MODULE_DESCRIPTION("SGM3785 GPIO Driver");
