/*
 * lm3646_front.c
 *
 * driver for lm3646_front
 *
 * Copyright (c) 2014-2020 Huawei Technologies Co., Ltd.
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
#include "securec.h"

/* LM3646 Registers define */
#define REG_CHIPID 0x00
#define REG_ENABLE 0x01
#define REG_FLAGS1 0x08
#define REG_FLAGS2 0x09
#define REG_LED1_FLASH_CURRENT_CONTROL 0x06
#define REG_LED1_TORCH_CURRENT_CONTROL 0x07
#define REG_FLASH_TIMEOUT 0x04
#define REG_MAX_CURRENT 0x05

#define CHIP_ID 0x10
#define MODE_STANDBY 0x00

#define MODE_TORCH 0x02
#define MODE_FLASH 0x03
#define STROBE_DISABLE 0x00
#define TORCH_DISABLE 0x00
#define TX_ENABLE 0x08
#define INDUCTOR_CURRENT_LIMMIT 0xf0
#define FLASH_TIMEOUT_TIME 0x47 // 400ms

#define FLASH_LED_MAX 0x7f
#define TORCH_LED_MAX 0x7f
#define MAX_BRIGHTNESS_FORMMI 0x09
#define ENABLE_SHORT_PROTECT 0x80
#define LM3646_RESET_HOLD_TIME 2

/* Internal data struct define */
enum lm3646_front_pin_type {
	RESET = 0,
	MAX_PIN,
};

/* Internal data struct define */
struct hw_lm3646_front_private_data_t {
	unsigned char flash_led[FLASH_LED_MAX];
	unsigned char torch_led[TORCH_LED_MAX];
	unsigned int flash_led_num;
	unsigned int torch_led_num;

	/* flash control pin */
	unsigned int pin[MAX_PIN];

	unsigned int chipid;
};

/* Internal varible define */
static struct hw_lm3646_front_private_data_t g_lm3646_front_pdata;
static struct hw_flash_ctrl_t g_lm3646_front_ctrl;
static struct i2c_driver g_lm3646_front_i2c_driver;

struct hw_lm3646_front_flash_level_matrix {
	unsigned int flash_level_min;
	unsigned int flash_level_max;

	unsigned char max_current_flash;
	unsigned char max_current_torch;
};

/*
 * pre-flash current(mA)---current of strong flashing(mA)---combination number
 * 187.2---1500---128
 * 140.2---1124.9---96
 * 93.4---749.8---64
 * 46.5---374.7---32
 * 23.1---187.2---16
 */
static struct hw_lm3646_front_flash_level_matrix g_lm3646_front_flash_level[5] = {
	{ 0, 15, 1, 0 },
	{ 16, 47, 3, 1 },
	{ 48, 111, 7, 3 },
	{ 112, 207, 11, 5 },
	{ 208, 335, 15, 7 },
};

static int g_support_dual_tempeture = 1;

define_kernel_flash_mutex(lm3646_front);

/* Function define */
static int hw_lm3646_front_param_check(char *buf,
	unsigned long *param, int num_of_par);

static int hw_lm3646_front_set_pin_reset(struct hw_flash_ctrl_t *flash_ctrl,
	unsigned int state)
{
	struct hw_lm3646_front_private_data_t *pdata = NULL;

	if (!flash_ctrl) {
		cam_err("%s flash_ctrl is NULL", __func__);
		return -1;
	}

	pdata = (struct hw_lm3646_front_private_data_t *)flash_ctrl->pdata;

	cam_debug("%s reset = %d, state = %d", __func__,
		pdata->pin[RESET], state);
	gpio_direction_output(pdata->pin[RESET], state);
	return 0;
}

static int hw_lm3646_front_init(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_lm3646_front_private_data_t *pdata = NULL;
	int rc;

	cam_debug("%s enter\n", __func__);

	if (!flash_ctrl) {
		cam_err("%s flash_ctrl is NULL", __func__);
		return -1;
	}

	pdata = (struct hw_lm3646_front_private_data_t *)flash_ctrl->pdata;
	flash_ctrl->pctrl = devm_pinctrl_get_select(flash_ctrl->dev,
		PINCTRL_STATE_DEFAULT);
	if (!flash_ctrl->pctrl) {
		cam_err("%s failed to set pin", __func__);
		return -EIO;
	}

	rc = gpio_request(pdata->pin[RESET], "flash-reset");
	if (rc < 0) {
		cam_err("%s failed to request reset pin", __func__);
		return -EIO;
	}

	hw_lm3646_front_set_pin_reset(flash_ctrl, LOW);
	msleep(LM3646_RESET_HOLD_TIME);

	return rc;
}

static int hw_lm3646_front_exit(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_lm3646_front_private_data_t *pdata = NULL;

	cam_debug("%s enter\n", __func__);

	if (!flash_ctrl) {
		cam_err("%s flash_ctrl is NULL", __func__);
		return -1;
	}

	flash_ctrl->func_tbl->flash_off(flash_ctrl);

	pdata = (struct hw_lm3646_front_private_data_t *)flash_ctrl->pdata;

	gpio_free(pdata->pin[RESET]);
	flash_ctrl->pctrl = devm_pinctrl_get_select(flash_ctrl->dev,
		PINCTRL_STATE_IDLE);

	return 0;
}

static int hw_lm3646_front_flash_mode(struct hw_flash_ctrl_t *flash_ctrl,
	int data)
{
	struct hw_flash_i2c_client *i2c_client = NULL;
	struct hw_flash_i2c_fn_t *i2c_func = NULL;
	struct hw_lm3646_front_private_data_t *pdata = NULL;
	unsigned char val = 0;

	cam_info("%s data = %d\n", __func__, data);
	if (!flash_ctrl) {
		cam_err("%s flash_ctrl is NULL", __func__);
		return -1;
	}

	i2c_client = flash_ctrl->flash_i2c_client;
	i2c_func = flash_ctrl->flash_i2c_client->i2c_func_tbl;
	pdata = flash_ctrl->pdata;

	/* clear error flag, resume chip */
	i2c_func->i2c_read(i2c_client, REG_FLAGS1, &val);
	i2c_func->i2c_read(i2c_client, REG_FLAGS2, &val);
	i2c_func->i2c_read(i2c_client, REG_LED1_FLASH_CURRENT_CONTROL, &val);

	/* set LED Flash current value */
	if (data < FLASH_LED_MAX) {
		cam_info("%s flash_led = 0x%x", __func__,
			pdata->flash_led[data]);
		/* REG_CURRENT_CONTROL[3:0] control flash current */
		val = ((val & 0x80) | (data & 0x7f));
	} else {
		cam_warn("%s data %d > flash_led_num %d", __func__,
			data, pdata->flash_led_num);
		/* REG_CURRENT_CONTROL[3:0] control flash current */
		val = ((val & 0x80) | 0x7f);
	}
	i2c_func->i2c_write(i2c_client, REG_MAX_CURRENT, 0x7c);
	i2c_func->i2c_write(i2c_client, REG_LED1_FLASH_CURRENT_CONTROL, 0x22);
	if (flash_ctrl->flash_mask_enable)
		i2c_func->i2c_write(i2c_client, REG_ENABLE, 0xd3);
	else
		i2c_func->i2c_write(i2c_client, REG_ENABLE, 0xd3);

	return 0;
}

static int hw_lm3646_front_torch_mode_mmi(struct hw_flash_ctrl_t *flash_ctrl,
	int led_mode, int brightness_level)
{
	struct hw_flash_i2c_client *i2c_client = NULL;
	struct hw_flash_i2c_fn_t *i2c_func = NULL;
	struct hw_lm3646_front_private_data_t *pdata = NULL;
	unsigned char val = 0;
	u8 brightness_val;

	cam_info("%s led_mode = %d level = %d", __func__,
		led_mode, brightness_level);
	if (!flash_ctrl) {
		cam_err("%s flash_ctrl is NULL", __func__);
		return -1;
	}

	i2c_client = flash_ctrl->flash_i2c_client;
	i2c_func = flash_ctrl->flash_i2c_client->i2c_func_tbl;
	pdata = (struct hw_lm3646_front_private_data_t *)flash_ctrl->pdata;

	/* clear error flag, resume chip */
	i2c_func->i2c_read(i2c_client, REG_FLAGS1, &val);
	i2c_func->i2c_read(i2c_client, REG_FLAGS2, &val);

	/* set max torch current */
	brightness_val = (brightness_level == 0) ? 0x1c : 0x7c;
	i2c_func->i2c_write(i2c_client, REG_MAX_CURRENT, brightness_val);

	/* set led1 torch current to max */
	i2c_func->i2c_read(i2c_client, REG_LED1_TORCH_CURRENT_CONTROL, &val);
	val = ((val & 0x80) | 0x7f);
	i2c_func->i2c_write(i2c_client, REG_LED1_TORCH_CURRENT_CONTROL, val);
	if (flash_ctrl->flash_mask_enable)
		i2c_func->i2c_write(i2c_client, REG_ENABLE, 0xd2);
	else
		i2c_func->i2c_write(i2c_client, REG_ENABLE, 0xd2);

	return 0;
}

static int hw_lm3646_front_set_mode(struct hw_flash_ctrl_t *flash_ctrl,
	void *data)
{
	struct hw_flash_cfg_data *cdata = (struct hw_flash_cfg_data *)data;

	struct hw_flash_i2c_client *i2c_client = NULL;
	struct hw_flash_i2c_fn_t *i2c_func = NULL;
	struct hw_lm3646_front_private_data_t *pdata = NULL;
	struct hw_lm3646_front_flash_level_matrix *matrix = NULL;
	unsigned char val = 0;
	unsigned char mode = 0;
	unsigned char regmaxcurrent = 0;
	unsigned char regcurrentflash = 0;
	unsigned char regcurrenttorch = 0;
	int current_index;
	int i;

	if (!flash_ctrl || !cdata) {
		cam_err("%s flash_ctrl or cdata is NULL", __func__);
		return -1;
	}

	i2c_client = flash_ctrl->flash_i2c_client;
	i2c_func = flash_ctrl->flash_i2c_client->i2c_func_tbl;
	pdata = flash_ctrl->pdata;

	current_index = cdata->data;
	if (current_index >= pdata->flash_led_num) {
		current_index = pdata->flash_led_num - 1;
		cam_err("%s user set flash curret[%d] index is out of range %d, set to maxval",
			__func__, cdata->data, pdata->flash_led_num);
	}

	for (i = 0; i < 5; i++) {
		if (g_lm3646_front_flash_level[i].flash_level_min <= current_index &&
			g_lm3646_front_flash_level[i].flash_level_max >= current_index) {
			matrix = &g_lm3646_front_flash_level[i];
			break;
		}
	}

	/* clear error flag, resume chip */
	i2c_func->i2c_read(i2c_client, REG_FLAGS1, &val);
	i2c_func->i2c_read(i2c_client, REG_FLAGS2, &val);
	if (matrix) {
		if (cdata->mode == FLASH_MODE) {
			mode = INDUCTOR_CURRENT_LIMMIT | MODE_FLASH;
			regcurrentflash = STROBE_DISABLE |
				(current_index - matrix->flash_level_min);
			regmaxcurrent = matrix->max_current_flash;
		} else {
			mode = INDUCTOR_CURRENT_LIMMIT | MODE_TORCH;
			regcurrenttorch = TORCH_DISABLE |
				(current_index - matrix->flash_level_min);
			regmaxcurrent = matrix->max_current_torch << 4;
		}
	}

	i2c_func->i2c_write(i2c_client, REG_FLASH_TIMEOUT, FLASH_TIMEOUT_TIME);
	i2c_func->i2c_write(i2c_client, REG_MAX_CURRENT,
		regmaxcurrent | ENABLE_SHORT_PROTECT);
	i2c_func->i2c_write(i2c_client, REG_LED1_FLASH_CURRENT_CONTROL,
		regcurrentflash);
	i2c_func->i2c_write(i2c_client, REG_LED1_TORCH_CURRENT_CONTROL,
		regcurrenttorch);
	i2c_func->i2c_write(i2c_client, REG_ENABLE, mode);

	cam_debug("%s mode = %d, level = %d\n",
		  __func__, cdata->mode, cdata->data);
	cam_info("%s regmaxcurrent = %x regcurrentflash = %x regcurrenttorch = %x",
		__func__, regmaxcurrent, regcurrentflash & 0x7f,
		regcurrenttorch & 0x7f);
	return 0;
}

static int hw_lm3646_front_on(struct hw_flash_ctrl_t *flash_ctrl, void *data)
{
	struct hw_flash_cfg_data *cdata = (struct hw_flash_cfg_data *)data;

	if (!flash_ctrl || !cdata) {
		cam_err("%s flash_ctrl or cdata is NULL", __func__);
		return -1;
	}
	cam_info("%s mode = %d, level = %d\n",
		__func__, cdata->mode, cdata->data);

	mutex_lock(flash_ctrl->hw_flash_mutex);
	// Enable lm3646_front switch to standby current is 10ua
	hw_lm3646_front_set_pin_reset(flash_ctrl, HIGH);

	hw_lm3646_front_set_mode(flash_ctrl, data);
	flash_ctrl->state.mode = cdata->mode;
	flash_ctrl->state.data = cdata->data;

	mutex_unlock(flash_ctrl->hw_flash_mutex);

	return 0;
}

static int hw_lm3646_front_brightness(struct hw_flash_ctrl_t *flash_ctrl,
	void *data)
{
	struct hw_flash_cfg_data *cdata = (struct hw_flash_cfg_data *)data;
	int rc = -1;

	if (!flash_ctrl || !cdata) {
		cam_err("%s flash_ctrl or cdata is NULL", __func__);
		return -1;
	}

	cam_debug("%s mode = %d, level = %d\n",
		  __func__, cdata->mode, cdata->data);
	cam_info("%s enter\n", __func__);
	mutex_lock(flash_ctrl->hw_flash_mutex);
	// Enable lm3646_front switch to standby current is 10ua
	hw_lm3646_front_set_pin_reset(flash_ctrl, HIGH);
	if (cdata->mode == FLASH_MODE)
		rc = hw_lm3646_front_flash_mode(flash_ctrl, cdata->data);
	else
		rc = hw_lm3646_front_torch_mode_mmi(flash_ctrl,
			cdata->mode, cdata->data);

	flash_ctrl->state.mode = cdata->mode;
	flash_ctrl->state.data = cdata->data;
	mutex_unlock(flash_ctrl->hw_flash_mutex);

	return rc;
}

static int hw_lm3646_front_off(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_flash_i2c_client *i2c_client = NULL;
	struct hw_flash_i2c_fn_t *i2c_func = NULL;
	unsigned char val = 0;

	cam_debug("%s enter\n", __func__);
	if (!flash_ctrl) {
		cam_err("%s flash_ctrl is NULL", __func__);
		return -1;
	}

	mutex_lock(flash_ctrl->hw_flash_mutex);
	if (flash_ctrl->state.mode == STANDBY_MODE) {
		mutex_unlock(flash_ctrl->hw_flash_mutex);
		return 0;
	}
	flash_ctrl->state.mode = STANDBY_MODE;
	flash_ctrl->state.data = 0;
	i2c_client = flash_ctrl->flash_i2c_client;
	i2c_func = flash_ctrl->flash_i2c_client->i2c_func_tbl;

	i2c_func->i2c_read(i2c_client, REG_FLAGS1, &val);
	i2c_func->i2c_read(i2c_client, REG_FLAGS2, &val);
	i2c_func->i2c_write(i2c_client, REG_ENABLE, MODE_STANDBY);

	// Enable lm3646_front switch to shutdown current is 1.3ua
	hw_lm3646_front_set_pin_reset(flash_ctrl, LOW);
	cam_info("%s end", __func__);
	mutex_unlock(flash_ctrl->hw_flash_mutex);

	return 0;
}

static int hw_lm3646_front_get_dt_data(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_lm3646_front_private_data_t *pdata = NULL;
	struct device_node *of_node = NULL;
	int i;
	int rc = -1;

	cam_debug("%s enter\n", __func__);

	if (!flash_ctrl) {
		cam_err("%s flash_ctrl is NULL", __func__);
		return rc;
	}

	pdata = (struct hw_lm3646_front_private_data_t *)flash_ctrl->pdata;
	of_node = flash_ctrl->dev->of_node;

	rc = of_property_read_u32_array(of_node, "vendor,flash-pin",
		pdata->pin, MAX_PIN);
	if (rc < 0) {
		cam_err("%s bbbb failed line %d\n", __func__, __LINE__);
		return rc;
	}
	for (i = 0; i < MAX_PIN; i++)
		cam_info("%s pin[%d] = %d\n", __func__, i, pdata->pin[i]);

	rc = of_property_read_u32(of_node, "vendor,flash-chipid",
		&pdata->chipid);
	cam_info("%s vendor,flash-chipid 0x%x, rc %d\n", __func__,
		pdata->chipid, rc);
	if (rc < 0) {
		cam_err("%s failed %d\n", __func__, __LINE__);
		return rc;
	}

	rc = of_property_read_u32(of_node, "vendor,flash_led_num",
		&pdata->flash_led_num);
	cam_info("%s chip, flash_led_num %d, rc %d\n", __func__,
		 pdata->flash_led_num, rc);
	if (rc < 0) {
		cam_err("%s failed %d\n", __func__, __LINE__);
		return rc;
	}

	rc = of_property_read_u32(of_node, "vendor,torch_led_num",
		&pdata->torch_led_num);
	cam_info("%s chip, torch_led_num %d, rc %d\n", __func__,
		 pdata->torch_led_num, rc);
	if (rc < 0) {
		cam_err("%s aaa failed %d\n", __func__, __LINE__);
		return rc;
	}
	return rc;
}

static ssize_t hw_lm3646_front_dual_leds_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf_s(buf, MAX_ATTRIBUTE_BUFFER_SIZE,
		MAX_ATTRIBUTE_BUFFER_SIZE - 1, "%d\n", g_support_dual_tempeture);
	if (rc <= 0) {
		cam_err("%s %d::snprintf_s return error %d",
			__func__, __LINE__, rc);
		return -EINVAL;
	}
	rc = strlen(buf) + 1;
	return rc;
}

static ssize_t hw_lm3646_front_dual_leds_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long param[2] = {0};
	int rc;

	rc = hw_lm3646_front_param_check((char *)buf, param, 1);
	if (rc < 0) {
		cam_err("%s failed to check param", __func__);
		return rc;
	}
	g_support_dual_tempeture = (int)param[0];
	return count;
}

static ssize_t hw_lm3646_front_lightness_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf_s(buf, MAX_ATTRIBUTE_BUFFER_SIZE,
		MAX_ATTRIBUTE_BUFFER_SIZE - 1,
		"mode=%d, data=%d.\n",
		g_lm3646_front_ctrl.state.mode,
		g_lm3646_front_ctrl.state.mode);
	if (rc <= 0) {
		cam_err("%s, %d, ::snprintf_s return error %d",
			__func__, __LINE__, rc);
		return -EINVAL;
	}
	rc = strlen(buf) + 1;
	return rc;
}

static int hw_lm3646_front_param_check(char *buf, unsigned long *param,
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

static ssize_t hw_lm3646_front_lightness_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct hw_flash_cfg_data cdata = {0};
	unsigned long param[2] = {0};
	int rc;

	rc = hw_lm3646_front_param_check((char *)buf, param, 2);
	if (rc < 0) {
		cam_err("%s failed to check param", __func__);
		return rc;
	}

	cdata.mode = (int)param[0];
	cdata.data = (int)param[1];

	if (cdata.mode == STANDBY_MODE) {
		rc = hw_lm3646_front_off(&g_lm3646_front_ctrl);
		if (rc < 0) {
			cam_err("%s lm3646_front flash off error", __func__);
			return rc;
		}
	} else {
		rc = hw_lm3646_front_on(&g_lm3646_front_ctrl, &cdata);
		if (rc < 0) {
			cam_err("%s lm3646_front flash on error", __func__);
			return rc;
		}
	}

	return count;
}

static ssize_t hw_lm3646_front_flash_mask_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc;

	rc = snprintf_s(buf, MAX_ATTRIBUTE_BUFFER_SIZE,
		MAX_ATTRIBUTE_BUFFER_SIZE - 1,
		"flash_mask_disabled=%d.\n",
		g_lm3646_front_ctrl.flash_mask_enable);
	if (rc <= 0) {
		cam_err("%s, %d, ::snprintf_s return error %d",
			__func__, __LINE__, rc);
		return -EINVAL;
	}
	rc = strlen(buf) + 1;
	return rc;
}

static ssize_t hw_lm3646_front_flash_mask_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	if (buf[0] == '0')
		g_lm3646_front_ctrl.flash_mask_enable = false;
	else
		g_lm3646_front_ctrl.flash_mask_enable = true;

	cam_debug("%s flash_mask_enable = %d", __func__,
		  g_lm3646_front_ctrl.flash_mask_enable);
	return count;
}

static void hw_lm3646_front_torch_brightness_set(struct led_classdev *cdev,
	enum led_brightness brightness)
{
	struct hw_flash_cfg_data cdata;
	int rc;
	unsigned int led_bright = brightness;

	cam_info("%s brightness = %d", __func__, brightness);
	if (led_bright == STANDBY_MODE) {
		rc = hw_lm3646_front_off(&g_lm3646_front_ctrl);
		if (rc < 0) {
			cam_err("%s pmu_led off error", __func__);
			return;
		}
	} else {
		int max_level;

		max_level = 3;
		cdata.mode = ((brightness - 1) / max_level) + TORCH_MODE;
		cdata.data = ((brightness - 1) % max_level);

		cam_info("%s brightness = 0x%x, mode = %d, data = %d",
			__func__, brightness,
			cdata.mode, cdata.data);

		rc = hw_lm3646_front_brightness(&g_lm3646_front_ctrl, &cdata);
		if (rc < 0) {
			cam_err("%s pmu_led on error", __func__);
			return;
		}
	}
}

static struct device_attribute g_lm3646_front_lightness =
	__ATTR(front_lightness,
	0664, hw_lm3646_front_lightness_show, hw_lm3646_front_lightness_store);

static struct device_attribute g_lm3646_front_dual_leds =
	__ATTR(front_dual_leds,
	0664, hw_lm3646_front_dual_leds_show, hw_lm3646_front_dual_leds_store);

static struct device_attribute g_lm3646_front_flash_mask =
	__ATTR(front_flash_mask,
	0664, hw_lm3646_front_flash_mask_show,
	hw_lm3646_front_flash_mask_store);

static int hw_lm3646_front_register_attribute(struct hw_flash_ctrl_t *flash_ctrl,
	struct device *dev)
{
	int rc;

	if (!flash_ctrl || !dev) {
		cam_err("%s flash_ctrl or dev is NULL", __func__);
		return -1;
	}

	if (flash_ctrl->flash_info.index == 0)
		flash_ctrl->cdev_torch.name = "torch";
	else if (flash_ctrl->flash_info.index == 1)
		flash_ctrl->cdev_torch.name = "torch_front";

	flash_ctrl->cdev_torch.max_brightness = MAX_BRIGHTNESS_FORMMI;
	flash_ctrl->cdev_torch.brightness_set =
		hw_lm3646_front_torch_brightness_set;
	rc = led_classdev_register((struct device *)dev,
		&flash_ctrl->cdev_torch);
	if (rc < 0) {
		cam_err("%s failed to register torch classdev", __func__);
		goto err_out;
	}

	rc = device_create_file(dev, &g_lm3646_front_lightness);
	if (rc < 0) {
		cam_err("%s failed to creat lightness attribute", __func__);
		goto err_create_lightness_file;
	}

	rc = device_create_file(dev, &g_lm3646_front_flash_mask);
	if (rc < 0) {
		cam_err("%s failed to creat flash_mask attribute", __func__);
		goto err_create_front_flash_mask_file;
	}
	return 0;
err_create_front_flash_mask_file:
	device_remove_file(dev, &g_lm3646_front_lightness);
err_create_lightness_file:
	led_classdev_unregister(&flash_ctrl->cdev_torch);
err_out:
	return rc;
}

extern int register_camerafs_attr(struct device_attribute *attr);
static int hw_lm3646_front_match(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_flash_i2c_client *i2c_client = NULL;
	struct hw_flash_i2c_fn_t *i2c_func = NULL;
	struct hw_lm3646_front_private_data_t *pdata = NULL;
	unsigned char id = 0;
	unsigned short slave_address;

	cam_debug("%s enter\n", __func__);

	if (!flash_ctrl) {
		cam_err("%s flash_ctrl is NULL", __func__);
		return -1;
	}

	i2c_client = flash_ctrl->flash_i2c_client;
	i2c_func = flash_ctrl->flash_i2c_client->i2c_func_tbl;
	pdata = (struct hw_lm3646_front_private_data_t *)flash_ctrl->pdata;

	/*
	 * front LM3646's i2c address need to be re-adjusted to
	 * avoid conflict as workaround, 0x68->0x67
	 */
	slave_address = (unsigned short)flash_ctrl->flash_info.slave_address;
	cam_info("%s real i2c address = 0x%x, fake i2c address = 0x%x",
		__func__, slave_address, i2c_client->client->addr);
	i2c_client->client->addr = slave_address;

	// Enable lm3646_front switch to standby current is 10ua
	hw_lm3646_front_set_pin_reset(flash_ctrl, HIGH);
	i2c_func->i2c_read(i2c_client, REG_CHIPID, &id);
	cam_info("%s 0x%x\n", __func__, id);
	id = id & CHIP_ID;
	if (id != CHIP_ID) {
		cam_err("%s match error, 0x%x != 0x%x",
			__func__, id, CHIP_ID);
		return -1;
	}
	// Enable lm3646_front switch to shutdown current is 1.3ua
	hw_lm3646_front_set_pin_reset(flash_ctrl, LOW);
	register_camerafs_attr(&g_lm3646_front_dual_leds);
	// add for debug only
	register_camerafs_attr(&g_lm3646_front_lightness);
	return 0;
}

static int hw_lm3646_front_remove(struct i2c_client *client)
{
	cam_debug("%s enter", __func__);

	g_lm3646_front_ctrl.func_tbl->flash_exit(&g_lm3646_front_ctrl);

	client->adapter = NULL;
	return 0;
}

static const struct i2c_device_id g_lm3646_front_id[] = {
	{ "lm3646_front", (unsigned long) &g_lm3646_front_ctrl },
	{}
};

static const struct of_device_id g_lm3646_front_dt_match[] = {
	{ .compatible = "vendor,lm3646_front" },
	{}
};
MODULE_DEVICE_TABLE(of, lm3646_front_dt_match);

static struct i2c_driver g_lm3646_front_i2c_driver = {
	.probe = hw_flash_i2c_probe,
	.remove = hw_lm3646_front_remove,
	.id_table = g_lm3646_front_id,
	.driver = {
		.name = "hw_lm3646_front",
		.of_match_table = g_lm3646_front_dt_match,
	},
};

static int __init hw_lm3646_front_module_init(void)
{
	cam_info("%s erter\n", __func__);
	return i2c_add_driver(&g_lm3646_front_i2c_driver);
}

static void __exit hw_lm3646_front_module_exit(void)
{
	cam_info("%s enter", __func__);
	i2c_del_driver(&g_lm3646_front_i2c_driver);
}

static struct hw_flash_i2c_client g_lm3646_front_i2c_client;

static struct hw_flash_fn_t g_lm3646_front_func_tbl = {
	.flash_config = hw_flash_config,
	.flash_init = hw_lm3646_front_init,
	.flash_exit = hw_lm3646_front_exit,
	.flash_on = hw_lm3646_front_on,
	.flash_off = hw_lm3646_front_off,
	.flash_match = hw_lm3646_front_match,
	.flash_get_dt_data = hw_lm3646_front_get_dt_data,
	.flash_register_attribute = hw_lm3646_front_register_attribute,
};

static struct hw_flash_ctrl_t g_lm3646_front_ctrl = {
	.flash_i2c_client = &g_lm3646_front_i2c_client,
	.func_tbl = &g_lm3646_front_func_tbl,
	.hw_flash_mutex = &flash_mut_lm3646_front,
	.pdata = (void *)&g_lm3646_front_pdata,
	.flash_mask_enable = true,
	.state = {
		.mode = STANDBY_MODE,
	},
};

module_init(hw_lm3646_front_module_init);
module_exit(hw_lm3646_front_module_exit);
MODULE_DESCRIPTION("LM3646 FRONT FLASH");
MODULE_LICENSE("GPL v2");
