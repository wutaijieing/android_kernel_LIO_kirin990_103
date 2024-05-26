/*
 * pmu_led.c
 *
 * Copyright (c) 2011-2020 Huawei Technologies Co., Ltd.
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

#include "hw_flash.h"
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/consumer.h>
#include "securec.h"

/* PMU LED Registers define */
#define LED_CTRL1 0x0d9
#define LED_CTRL2 0x0da
#define LED_CTRL3 0x0db
#define LED_CTRL4 0x0dc
#define LED_CTRL5 0x0dd
#define LED_CTRL6 0x0de
#define LED_CTRL7 0x0df

#define LED_FLASH_ENABLE 0x01
#define LED_TORCH_ENABLE 0x02

#define FLASH_LED_MAX 11
#define TORCH_LED_MAX 8

/* Internal data struct define */
enum hw_pmu_led_ctl_type {
	LED_CTRL_1 = 0,
	LED_CTRL_2,
	LED_CTRL_3,
	LED_CTRL_4,
	LED_CTRL_5,
	LED_CTRL_6,
	LED_CTRL_7,
	LED_CTRL_MAX,
};

enum hw_pmu_led_position_type {
	FLASH_POSITION_REAR = 0,
	FLASH_POSITION_FRONT,
};

struct hw_pmu_led_private_data_t {
	unsigned int flash_led[FLASH_LED_MAX];
	unsigned int torch_led[TORCH_LED_MAX];
	unsigned int flash_led_num;
	unsigned int torch_led_num;
	unsigned int led_ctrl_flash[LED_CTRL_MAX];
	unsigned int led_ctrl_torch[LED_CTRL_MAX];
	unsigned int flash_position;
	void __iomem *pmu_base;
	struct regulator_bulk_data led_vcc;
};

/* Internal varible define */
static struct hw_pmu_led_private_data_t g_pmu_led_pdata;
static struct hw_flash_ctrl_t g_pmu_led_ctrl;

define_kernel_flash_mutex(pmu_led);

extern void boost5v_flash_led_enable(bool enable);

/* Function define */
static void hw_pmu_led_write(unsigned int reg, unsigned int val)
{
	struct hw_pmu_led_private_data_t *pdata = NULL;

	cam_debug("%s reg=0x%x, val=0x%x", __func__, reg, val);

	pdata = (struct hw_pmu_led_private_data_t *)g_pmu_led_ctrl.pdata;
	writel(val, (pdata->pmu_base + (reg << 2)));
}

static int hw_pmu_led_init(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_pmu_led_private_data_t *pdata = NULL;
	struct device_node *np = NULL;

	if (!flash_ctrl) {
		cam_err("%s flash_ctrl is NULL", __func__);
		return -1;
	}

	pdata = (struct hw_pmu_led_private_data_t *)flash_ctrl->pdata;

	/* map pmic io register address */
	np = of_find_node_by_name(NULL, "pmic");
	if (!np) {
		cam_err("%s failed to find pmic node", __func__);
		return -1;
	}
	pdata->pmu_base = of_iomap(np, 0);
	if (!pdata->pmu_base) {
		cam_err("%s failed to iomap pmu", __func__);
		return -1;
	}

	return 0;
}

static int hw_pmu_led_exit(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_pmu_led_private_data_t *pdata = NULL;

	if (!flash_ctrl) {
		cam_err("%s flash_ctrl is NULL", __func__);
		return -1;
	}

	pdata = (struct hw_pmu_led_private_data_t *)flash_ctrl->pdata;

	iounmap(pdata->pmu_base);

	return 0;
}

static int hw_pmu_led_flash_mode(struct hw_flash_ctrl_t *flash_ctrl,
	int data)
{
	struct hw_pmu_led_private_data_t *pdata = NULL;

	cam_debug("%s data=%d\n", __func__, data);

	if (!flash_ctrl) {
		cam_err("%s flash_ctrl is NULL", __func__);
		return -1;
	}

	pdata = (struct hw_pmu_led_private_data_t *)flash_ctrl->pdata;

	hw_pmu_led_write(LED_CTRL1, pdata->led_ctrl_flash[LED_CTRL_1]);
	hw_pmu_led_write(LED_CTRL2, pdata->led_ctrl_flash[LED_CTRL_2]);
	hw_pmu_led_write(LED_CTRL3, pdata->led_ctrl_flash[LED_CTRL_3]);

	/* set LED Flash current value */
	if (data < pdata->flash_led_num) {
		cam_debug("%s flash_led=0x%x", __func__,
			pdata->flash_led[data]);
		hw_pmu_led_write(LED_CTRL4, (pdata->flash_led[data] & 0x0f));
	} else {
		cam_warn("%s data %d > flash_led_num %d", __func__,
			data, pdata->flash_led_num);
		hw_pmu_led_write(LED_CTRL4,
			(pdata->flash_led[pdata->flash_led_num-1] & 0x0f));
	}

	hw_pmu_led_write(LED_CTRL5, pdata->led_ctrl_flash[LED_CTRL_5]);
	if (flash_ctrl->flash_mask_enable)
		hw_pmu_led_write(LED_CTRL6,
			(pdata->led_ctrl_flash[LED_CTRL_6] & 0xf7));
	else
		hw_pmu_led_write(LED_CTRL6,
			(pdata->led_ctrl_flash[LED_CTRL_6] | 0x8));

	hw_pmu_led_write(LED_CTRL7, pdata->led_ctrl_flash[LED_CTRL_7]);

	hw_pmu_led_write(LED_CTRL2, LED_FLASH_ENABLE);

	return 0;
}

static int hw_pmu_led_torch_mode(struct hw_flash_ctrl_t *flash_ctrl,
	int data)
{
	struct hw_pmu_led_private_data_t *pdata = NULL;

	cam_debug("%s data=%d\n", __func__, data);

	if (!flash_ctrl) {
		cam_err("%s flash_ctrl is NULL", __func__);
		return -1;
	}

	pdata = (struct hw_pmu_led_private_data_t *)flash_ctrl->pdata;

	hw_pmu_led_write(LED_CTRL1, pdata->led_ctrl_torch[LED_CTRL_1]);
	hw_pmu_led_write(LED_CTRL2, pdata->led_ctrl_torch[LED_CTRL_2]);
	hw_pmu_led_write(LED_CTRL3, pdata->led_ctrl_torch[LED_CTRL_3]);

	/* set LED Torch current value */
	if (data < pdata->torch_led_num) {
		cam_debug("%s torch_led=0x%x", __func__,
			pdata->torch_led[data]);
		hw_pmu_led_write(LED_CTRL3,
			((pdata->torch_led[data] & 0x07) | (pdata->led_ctrl_torch[LED_CTRL_3] & 0x70)));
	} else {
		cam_warn("%s data %d > torch_led_num %d", __func__,
			data, pdata->torch_led_num);
		hw_pmu_led_write(LED_CTRL3,
			((pdata->torch_led[pdata->torch_led_num-1] & 0x07) | (pdata->led_ctrl_torch[LED_CTRL_3] & 0x70)));
	}
	hw_pmu_led_write(LED_CTRL4, pdata->led_ctrl_torch[LED_CTRL_4]);
	hw_pmu_led_write(LED_CTRL5, pdata->led_ctrl_torch[LED_CTRL_5]);
	hw_pmu_led_write(LED_CTRL6, pdata->led_ctrl_torch[LED_CTRL_6]);
	hw_pmu_led_write(LED_CTRL7, pdata->led_ctrl_torch[LED_CTRL_7]);

	hw_pmu_led_write(LED_CTRL2, LED_TORCH_ENABLE);

	return 0;
}

static int hw_pmu_led_on(struct hw_flash_ctrl_t *flash_ctrl, void *data)
{
	struct hw_pmu_led_private_data_t *pdata = NULL;
	struct hw_flash_cfg_data *cdata = (struct hw_flash_cfg_data *)data;
	int rc = -1;

	if (!flash_ctrl || !cdata) {
		cam_err("%s flash_ctrl or cdata is NULL", __func__);
		return -1;
	}

	cam_debug("%s mode=%d, level=%d\n", __func__, cdata->mode, cdata->data);

	mutex_lock(flash_ctrl->hw_flash_mutex);
	pdata = (struct hw_pmu_led_private_data_t *)flash_ctrl->pdata;

	if (flash_ctrl->state.mode == STANDBY_MODE)

#ifdef CONFIG_HISI_BOOST5V_CONTROL
		boost5v_flash_led_enable(true);
#endif
	else
		cam_notice("%s flash led has been power on", __func__);

	if (cdata->mode == FLASH_MODE)
		rc = hw_pmu_led_flash_mode(flash_ctrl, cdata->data);
	else
		rc = hw_pmu_led_torch_mode(flash_ctrl, cdata->data);

	flash_ctrl->state.mode = cdata->mode;
	flash_ctrl->state.data = cdata->data;
	mutex_unlock(flash_ctrl->hw_flash_mutex);

	return rc;
}

static int hw_pmu_led_off(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_pmu_led_private_data_t *pdata = NULL;

	cam_debug("%s ernter\n", __func__);

	if (!flash_ctrl) {
		cam_err("%s flash_ctrl is NULL", __func__);
		return -1;
	}

	if (flash_ctrl->state.mode == STANDBY_MODE) {
		cam_notice("%s flash led has been powered off", __func__);
		return 0;
	}

	mutex_lock(flash_ctrl->hw_flash_mutex);
	pdata = (struct hw_pmu_led_private_data_t *)flash_ctrl->pdata;
	flash_ctrl->state.mode = STANDBY_MODE;
	flash_ctrl->state.data = 0;
	hw_pmu_led_write(LED_CTRL2, 0);

#ifdef CONFIG_HISI_BOOST5V_CONTROL
	boost5v_flash_led_enable(false);
#endif
	mutex_unlock(flash_ctrl->hw_flash_mutex);

	return 0;
}

static int hw_pmu_led_match(struct hw_flash_ctrl_t *flash_ctrl)
{
	cam_debug("%s ernter\n", __func__);
	return 0;
}

static int hw_pmu_led_get_dt_data(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_pmu_led_private_data_t *pdata = NULL;
	struct device_node *of_node = NULL;
	int i;
	const char *flash_position = NULL;
	int rc = -1;

	cam_debug("%s enter\n", __func__);

	if (!flash_ctrl) {
		cam_err("%s flash_ctrl is NULL", __func__);
		return rc;
	}

	pdata = (struct hw_pmu_led_private_data_t *)flash_ctrl->pdata;
	of_node = flash_ctrl->dev->of_node;

	rc = of_property_read_u32(of_node, "vendor,flash_led_num",
		&pdata->flash_led_num);
	cam_debug("%s vendor,flash_led_num %d, rc %d\n", __func__,
		pdata->flash_led_num, rc);
	if (rc < 0) {
		cam_err("%s failed %d\n", __func__, __LINE__);
		return rc;
	}

	rc = of_property_read_u32(of_node, "vendor,torch_led_num",
		&pdata->torch_led_num);
	cam_debug("%s vendor,torch_led_num %d, rc %d\n", __func__,
		pdata->torch_led_num, rc);
	if (rc < 0) {
		cam_err("%s failed %d\n", __func__, __LINE__);
		return rc;
	}

	rc = of_property_read_u32_array(of_node, "vendor,flash_led",
		pdata->flash_led, pdata->flash_led_num);
	if (rc < 0) {
		cam_err("%s failed line %d\n", __func__, __LINE__);
		return rc;
	}
	for (i = 0; i < pdata->flash_led_num; i++)
		cam_debug("%s flash_led[%d] = 0x%x\n", __func__, i,
			pdata->flash_led[i]);

	rc = of_property_read_u32_array(of_node, "vendor,torch_led",
		pdata->torch_led, pdata->torch_led_num);
	if (rc < 0) {
		cam_err("%s failed line %d\n", __func__, __LINE__);
		return rc;
	}
	for (i = 0; i < pdata->torch_led_num; i++)
		cam_debug("%s torch_led[%d]=0x%x\n", __func__, i,
			pdata->torch_led[i]);

	rc = of_property_read_u32_array(of_node, "vendor,led_ctrl_flash",
		pdata->led_ctrl_flash, LED_CTRL_MAX);
	if (rc < 0) {
		cam_err("%s failed line %d\n", __func__, __LINE__);
		return rc;
	}
	for (i = 0; i < LED_CTRL_MAX; i++)
		cam_debug("%s led_ctrl_flash[%d]=0x%x\n", __func__, i,
			pdata->led_ctrl_flash[i]);

	rc = of_property_read_u32_array(of_node, "vendor,led_ctrl_torch",
		pdata->led_ctrl_torch, LED_CTRL_MAX);
	if (rc < 0) {
		cam_err("%s failed line %d\n", __func__, __LINE__);
		return rc;
	}
	for (i = 0; i < LED_CTRL_MAX; i++)
		cam_debug("%s led_ctrl_torch[%d]=0x%x\n", __func__, i,
			pdata->led_ctrl_torch[i]);

	pdata->flash_position = FLASH_POSITION_REAR;
	rc = of_property_read_string(of_node, "vendor,flash-position",
		&flash_position);
	if ((rc == 0) && flash_position) {
		if (!strcmp(flash_position, "front"))
			pdata->flash_position = FLASH_POSITION_FRONT;
	}
	cam_debug("%s flash_position=%d\n", __func__, pdata->flash_position);

	return rc;
}

static ssize_t hw_pmu_led_lightness_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf_s(buf, MAX_ATTRIBUTE_BUFFER_SIZE,
		MAX_ATTRIBUTE_BUFFER_SIZE - 1,
		"mode=%d, data=%d.\n",
		g_pmu_led_ctrl.state.mode, g_pmu_led_ctrl.state.data);
	if (rc <= 0) {
		cam_err("%s, %d, ::snprintf_s return error %d",
			__func__, __LINE__, rc);
		return -EINVAL;
	}
	rc = strlen(buf) + 1;
	return rc;
}


static int hw_pmu_led_param_check(char *buf, unsigned long *param,
	int num_of_par)
{
	char *token = NULL;
	int base;
	int cnt;

	token = strsep(&buf, " ");

	for (cnt = 0; cnt < num_of_par; cnt++) {
		if (token) {
			if ((token[1] == 'x') || (token[1] == 'X'))
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

static ssize_t hw_pmu_led_lightness_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct hw_flash_cfg_data cdata = {0};
	unsigned long param[2] = {0};
	int rc;

	rc = hw_pmu_led_param_check((char *)buf, param, 2);
	if (rc < 0) {
		cam_err("%s failed to check param", __func__);
		return rc;
	}

	cdata.mode = (int)param[0];
	cdata.data = (int)param[1];

	if (cdata.mode == STANDBY_MODE) {
		rc = hw_pmu_led_off(&g_pmu_led_ctrl);
		if (rc < 0) {
			cam_err("%s pmu_led flash off error", __func__);
			return rc;
		}
	} else {
		rc = hw_pmu_led_on(&g_pmu_led_ctrl, &cdata);
		if (rc < 0) {
			cam_err("%s pmu_led flash on error", __func__);
			return rc;
		}
	}

	return count;
}

static ssize_t hw_pmu_led_flash_mask_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf_s(buf, MAX_ATTRIBUTE_BUFFER_SIZE,
		MAX_ATTRIBUTE_BUFFER_SIZE - 1,
		"flash_mask_disabled=%d.\n",
		g_pmu_led_ctrl.flash_mask_enable);
	if (rc <= 0) {
		cam_err("%s, %d, ::snprintf_s return error %d",
			__func__, __LINE__, rc);
		return -EINVAL;
	}
	rc = strlen(buf) + 1;
	return rc;
}

static ssize_t hw_pmu_led_flash_mask_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	if (buf[0] == '0')
		g_pmu_led_ctrl.flash_mask_enable = false;
	else
		g_pmu_led_ctrl.flash_mask_enable = true;

	cam_debug("%s flash_mask_enable=%d", __func__,
		g_pmu_led_ctrl.flash_mask_enable);
	return count;
}

static void hw_pmu_torch_brightness_set(struct led_classdev *cdev,
	enum led_brightness brightness)
{
	struct hw_flash_cfg_data cdata;
	int rc;
	unsigned int led_bright = brightness;

	if (led_bright == STANDBY_MODE) {
		rc = hw_pmu_led_off(&g_pmu_led_ctrl);
		if (rc < 0) {
			cam_err("%s pmu_led off error", __func__);
			return;
		}
	} else {
		cdata.mode = TORCH_MODE;
		cdata.data = brightness - 1;
		rc = hw_pmu_led_on(&g_pmu_led_ctrl, &cdata);
		if (rc < 0) {
			cam_err("%s pmu_led on error", __func__);
			return;
		}
	}
}

static struct device_attribute g_pmu_led_lightness =
	__ATTR(lightness, 0664, hw_pmu_led_lightness_show,
	hw_pmu_led_lightness_store);

static struct device_attribute g_pmu_led_flash_mask =
	__ATTR(flash_mask, 0664, hw_pmu_led_flash_mask_show,
	hw_pmu_led_flash_mask_store);

static int hw_pmu_led_register_attribute(struct hw_flash_ctrl_t *flash_ctrl,
	struct device *dev)
{
	int rc;
	struct hw_pmu_led_private_data_t *pdata = NULL;

	if (!flash_ctrl || !dev) {
		cam_err("%s flash_ctrl or dev is NULL", __func__);
		return -1;
	}

	pdata = (struct hw_pmu_led_private_data_t *)(flash_ctrl->pdata);
	if (!pdata) {
		cam_err("%s pdata is NULL", __func__);
		return -1;
	}

	if (pdata->flash_position == FLASH_POSITION_FRONT)
		flash_ctrl->cdev_torch.name = "torch_front";
	else
		flash_ctrl->cdev_torch.name = "torch";

	flash_ctrl->cdev_torch.max_brightness = pdata->torch_led_num;
	flash_ctrl->cdev_torch.brightness_set = hw_pmu_torch_brightness_set;
	rc = led_classdev_register((struct device *)dev,
		&flash_ctrl->cdev_torch);
	if (rc < 0) {
		cam_err("%s failed to register torch classdev", __func__);
		goto err_out;
	}

	rc = device_create_file(dev, &g_pmu_led_lightness);
	if (rc < 0) {
		cam_err("%s failed to creat lightness attribute", __func__);
		goto err_create_lightness_file;
	}

	rc = device_create_file(dev, &g_pmu_led_flash_mask);
	if (rc < 0) {
		cam_err("%s failed to creat flash_mask attribute", __func__);
		goto err_create_flash_mask_file;
	}

	return 0;

err_create_flash_mask_file:
	device_remove_file(dev, &g_pmu_led_lightness);
err_create_lightness_file:
	led_classdev_unregister(&flash_ctrl->cdev_torch);
err_out:

	return rc;
}

static const struct of_device_id g_pmu_led_dt_match[] = {
	{ .compatible = "vendor,pmu_led", .data = &g_pmu_led_ctrl },
	{}
};
MODULE_DEVICE_TABLE(of, pmu_led_dt_match);

static struct platform_driver g_pmu_led_platform_driver = {
	.driver = {
		.name = "pmu_led",
		.owner = THIS_MODULE,
		.of_match_table = g_pmu_led_dt_match,
	},
};

static int32_t hw_pmu_led_platform_probe(struct platform_device *pdev)
{
	const struct of_device_id *match = NULL;

	match = of_match_device(g_pmu_led_dt_match, &pdev->dev);
	if (!match) {
		cam_err("pmu led match device failed");
		return -1;
	}
	cam_debug("%s compatible=%s\n", __func__, match->compatible);
	return hw_flash_platform_probe(pdev, (void *)match->data);
}

static int __init hw_pmu_led_module_init(void)
{
	int rc;

	cam_debug("%s:%d\n", __func__, __LINE__);

	rc = platform_driver_probe(&g_pmu_led_platform_driver,
		hw_pmu_led_platform_probe);
	if (rc < 0)
		cam_notice("%s platform_driver_probe error %d\n", __func__, rc);

	return rc;
}

static void __exit hw_pmu_led_module_exit(void)
{
	g_pmu_led_ctrl.func_tbl->flash_exit(&g_pmu_led_ctrl);
	platform_driver_unregister(&g_pmu_led_platform_driver);
}

static struct hw_flash_fn_t g_pmu_led_func_tbl = {
	.flash_config = hw_flash_config,
	.flash_init = hw_pmu_led_init,
	.flash_exit = hw_pmu_led_exit,
	.flash_on = hw_pmu_led_on,
	.flash_off = hw_pmu_led_off,
	.flash_match = hw_pmu_led_match,
	.flash_get_dt_data = hw_pmu_led_get_dt_data,
	.flash_register_attribute = hw_pmu_led_register_attribute,
};

static struct hw_flash_ctrl_t g_pmu_led_ctrl = {
	.func_tbl = &g_pmu_led_func_tbl,
	.hw_flash_mutex = &flash_mut_pmu_led,
	.pdata = (void *)&g_pmu_led_pdata,
	.flash_mask_enable = true,
	.state = {
		.mode = STANDBY_MODE,
	},
};

module_init(hw_pmu_led_module_init);
module_exit(hw_pmu_led_module_exit);
MODULE_DESCRIPTION("PMU_LED FLASH");
MODULE_LICENSE("GPL v2");
