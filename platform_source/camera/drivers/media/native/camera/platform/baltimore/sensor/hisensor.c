/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2020. All rights reserved.
 * Description: Implement of hisensor.
 * Create: 2019-07-08
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/rpmsg.h>
#include <linux/adc.h>

#include "hwsensor.h"
#include "sensor_commom.h"
#include <linux/io.h>

#define I2S(i) container_of(i, sensor_t, intf)
#define Sensor2Pdev(s) container_of((s).dev, struct platform_device, dev)
#define SENSOR_REG_SETTING_DELAY_1_MS 1
#define GPIO_LOW_STATE    (0)
#define GPIO_HIGH_STATE   (1)
#define CTL_RESET_HOLD    (0)
#define CTL_RESET_RELEASE (1)

#define LPM3_GPU_BUCK_ADDR       (0xFFF0A45C)
#define LPM3_GPU_BUCK_MAP_BYTE (8)
#define CYBERVERSE_RESET              0xCB
#define GPOI_RELEASE_SLEEP_2MS        2

enum gpio_status {
	GPIO_SUSPEND = 0,
	GPIO_GND = 1,
	GPIO_HIGH = 2,
};

#define CAMERA_RPC_WORKQUEUE "camera_rpc_workqueue"
unsigned int *lpm3;
unsigned int g_reset[HWSENSOR_POSITION_MAX];

extern struct hw_csi_pad hw_csi_pad;
extern int strncpy_s(char *strDest, size_t destMax,
		const char *strSrc, size_t count);
extern int memset_s(void *dest, size_t destMax, int c, size_t count);
extern int rpc_status_change_for_camera(unsigned int status);

//lint -save -e846 -e514 -e866 -e429 -e605 -e30 -e84 -e785 -e64 -e826 -e838 -e715 -e747 -e778 -e774 -e732 -e650 -e31 -e731 -e528 -e753 -e737

int hisensor_config(hwsensor_intf_t* si, void *argp);

struct mutex hisensor_power_lock;

char const* hisensor_get_name(hwsensor_intf_t* si)
{
	sensor_t* sensor = NULL;

	if (!si) {
		cam_err("%s. si is null.", __func__);
		return NULL;
	}

	sensor = I2S(si);
	if (sensor == NULL || sensor->board_info == NULL) {
		cam_err("%s. sensor or board_info->name is NULL.", __func__);
		return NULL;
	}

	return sensor->board_info->name;
}

static int hisensor_do_hw_buckle_check(hwsensor_intf_t *si, void *data, int tag)
{
	int status = -1;
	int ret = 0;
	sensor_t *sensor = NULL;
	hwsensor_board_info_t *b_info = NULL;
	struct sensor_cfg_data *cdata = NULL;
	unsigned int gpio_num = 0;
	int sensor_index;

	if (si == NULL || data == NULL) {
		cam_err("%s. si or data is NULL", __func__);
		return -EINVAL;
	}

	sensor = I2S(si);
	if (sensor == NULL ||
		sensor->board_info == NULL ||
		sensor->board_info->name == NULL) {
		cam_err("%s. sensor or board_info->name is NULL", __func__);
		return -EINVAL;
	}
	b_info = sensor->board_info;
	sensor_index = b_info->sensor_index;

	cam_info("%s enter.", __func__);
	cdata = (struct sensor_cfg_data *)data;
	gpio_num = b_info->gpios[tag].gpio;
	cam_info("%s. check gpio[%d]", __func__, gpio_num);
	if (gpio_num <= 0) {
		cam_err("%s.gpio get failed!", __func__);
		if (tag == BTB_CHECK) {
			if ((sensor_index > HWSENSOR_POSITION_INVALID && sensor_index < HWSENSOR_POSITION_MAX) &&
				g_reset[sensor_index] == GPIO_HIGH_STATE) {
				status = GPIO_HIGH_STATE;
				cam_err("%s reset is %d, sensor_index is %d, btb check is broken!",
					__func__, status, sensor_index);
				g_reset[sensor_index] = GPIO_LOW_STATE;
			} else {
				status = GPIO_LOW_STATE;
				cam_info("%s reset is %d, sensor_index is %d, camera %d is ok!",
					__func__, status, sensor_index, tag);
			}
			cdata->data = status;
		}
		return status;
	}
	ret = gpio_request(gpio_num, NULL);
	if (ret < 0) {
		cam_err("%s request gpio[%d] failed", __func__, gpio_num);
		return ret;
	}
	ret = gpio_direction_input(gpio_num);
	if (ret < 0) {
		cam_err("%s gpio_direction_input failed", __func__);
		return ret;
	}
	status = gpio_get_value(gpio_num);
	if (tag == BTB_CHECK) {
		if (status == GPIO_HIGH_STATE || ((sensor_index > HWSENSOR_POSITION_INVALID &&
			sensor_index < HWSENSOR_POSITION_MAX) && g_reset[sensor_index] == GPIO_HIGH_STATE)) {
			status = GPIO_HIGH_STATE;
			cam_err("%s gpio[%u] status is %d, reset is %u, sensor_index is %d, btb check is broken!",
				__func__, gpio_num, status, g_reset[sensor_index], sensor_index);
			g_reset[sensor_index] = GPIO_LOW_STATE;
		}
	}
	cdata->data = status;

	if (status == GPIO_LOW_STATE)
		cam_info("%s.gpio[%u] status is %d, camera %d is ok!",
			__func__, gpio_num, status, tag);
	else if (status == GPIO_HIGH_STATE)
		cam_err("%s.gpio[%u] status is %d, camera %d is broken!",
			__func__, gpio_num, status, tag);
	else
		cam_err("%s.gpio[%u] status is %d, camera gpio_get_value is wrong!",
			__func__, gpio_num, status);

	gpio_free(gpio_num);
	return status;
}

int hisensor_power_up(hwsensor_intf_t* si)
{
	int ret          = 0;
	sensor_t* sensor = NULL;

	if (!si){
		cam_err("%s. si is null", __func__);
		return -EINVAL;
	}

	sensor = I2S(si);
	if (sensor == NULL || sensor->board_info == NULL || sensor->board_info->name == NULL) {
		cam_err("%s. sensor or board_info->name is NULL", __func__);
		return -EINVAL;
	}

	cam_info("enter %s. index = %d name = %s", __func__, sensor->board_info->sensor_index, sensor->board_info->name);

	if (hw_is_fpga_board())
		ret = do_sensor_power_on(sensor->board_info->sensor_index, sensor->board_info->name);
	else
		ret = hw_sensor_power_up(sensor);

	if (ret == 0)
		cam_info("%s. power up sensor success", __func__);
	else
		cam_err("%s. power up sensor fail", __func__);

	return ret;
}

int hisensor_power_down(hwsensor_intf_t* si)
{
	int ret          = 0;
	sensor_t* sensor = NULL;

	if (!si){
		cam_err("%s. si is null", __func__);
		return -EINVAL;
	}

	sensor = I2S(si);
	if (sensor == NULL || sensor->board_info == NULL || sensor->board_info->name == NULL) {
		cam_err("%s. sensor or board_info->name is NULL", __func__);
		return -EINVAL;
	}

	cam_info("enter %s. index = %d name = %s", __func__, sensor->board_info->sensor_index, sensor->board_info->name);

	if (hw_is_fpga_board())
		ret = do_sensor_power_off(sensor->board_info->sensor_index, sensor->board_info->name);
	else
		ret = hw_sensor_power_down(sensor);

	if (ret == 0)
		cam_info("%s. power down sensor success", __func__);
	else
		cam_err("%s. power down sensor fail", __func__);

	return ret;
}

int hisensor_csi_enable(hwsensor_intf_t* si)
{
	(void)si;
	return 0;
}

int hisensor_csi_disable(hwsensor_intf_t* si)
{
	(void)si;
	return 0;
}

int hisensor_get_gpio_status(sensor_t *sensor,
	unsigned int adc_gpio_index)
{
	struct pinctrl *p = NULL;
	struct pinctrl_state *pinctrl_def = NULL;
	struct pinctrl_state *pinctrl_idle = NULL;
	int rc;
	int gpio_val = 0;
	int adc_gpio_id;

	if (!sensor)
		return -1;

	p = devm_pinctrl_get(sensor->dev);
	if (!p) {
		cam_err("could not get pinctrl\n");
		return -1;
	}

	adc_gpio_id = sensor->board_info->adc_gpio[adc_gpio_index];

	rc = gpio_request(adc_gpio_id, "adc");
	if (rc < 0) {
		cam_err("%s failed to request gpio[%d]", __func__, adc_gpio_id);
		devm_pinctrl_put(p);
		return -1;
	}

	cam_info("%s gpio[%d]\n", __func__, adc_gpio_id);
	pinctrl_def = pinctrl_lookup_state(p, "default");
	if (!pinctrl_def) {
		cam_err("could not get defstate\n");
		gpio_free(adc_gpio_id);
		devm_pinctrl_put(p);
		return -1;
	}

	pinctrl_idle = pinctrl_lookup_state(p, "idle");
	if (!pinctrl_def) {
		cam_err("could not get idle defstate\n");
		gpio_free(adc_gpio_id);
		devm_pinctrl_put(p);
		return -1;
	}

	/* select default state. according to iomux.xml config */
	rc = pinctrl_select_state(p, pinctrl_def);
	if (rc) {
		cam_err("could not set pins to default state\n");
		gpio_free(adc_gpio_id);
		devm_pinctrl_put(p);
		return -1;
	}

	udelay(10);
	rc = gpio_direction_input(adc_gpio_id);
	if (rc < 0) {
		cam_err("%s failed to config gpio %d input\n", __func__, adc_gpio_id);
		gpio_free(adc_gpio_id);
		devm_pinctrl_put(p);
		return -1;
	}

	gpio_val = gpio_get_value(adc_gpio_id);
	if (gpio_val == 0) {
		cam_info("gpio val is low\n");
		gpio_free(adc_gpio_id);
		devm_pinctrl_put(p);
		return GPIO_GND;
	}
	/* select idle state. according to iomux.xml config */
	rc = pinctrl_select_state(p, pinctrl_idle);
	if (rc) {
		cam_err("could not set pins to idle state\n");
		gpio_free(adc_gpio_id);
		devm_pinctrl_put(p);
		return -1;
	}
	udelay(10);
	rc = gpio_direction_input(adc_gpio_id);
	if (rc < 0) {
		cam_err("%s failed to config gpio%d input\n",
			__func__, adc_gpio_id);
		gpio_free(adc_gpio_id);
		devm_pinctrl_put(p);
		return -1;
	}
	gpio_val = gpio_get_value(adc_gpio_id);
	if (gpio_val == 0) {
		cam_info("gpio val is suspend\n");
		gpio_free(adc_gpio_id);
		devm_pinctrl_put(p);
		return GPIO_SUSPEND;
	}
	cam_info("gpio val is high\n");
	gpio_free(adc_gpio_id);
	devm_pinctrl_put(p);

	return GPIO_HIGH;
}

int get_ext_name(sensor_t *sensor, struct sensor_cfg_data *cdata)
{
	int i = 0;
	int j = 0;
	int volt = 0;
	int max = 0;
	int min = 0;
	int gpio_status;
	int ret;

	if (sensor == NULL || cdata == NULL) {
		cam_err("%s. si or data is NULL", __func__);
		return -EINVAL;
	}

	if (sensor->board_info->ext_type == EXT_INFO_NO_ADC) {
		cam_info("%s no adc channel, use ext_name config in overlay:%s",
				__func__, sensor->board_info->ext_name[0]);
		ret = strncpy_s(cdata->info.extend_name, DEVICE_NAME_SIZE - 1,
				  sensor->board_info->ext_name[0],
				  strlen(sensor->board_info->ext_name[0])+1);
		if (ret) {
			cam_err("%s:strncpy_c fail, error = %d\n", __func__, ret);
			return -1;
		}
	}

	if (sensor->board_info->ext_type == EXT_INFO_ADC) {
		volt = lpm_adc_get_value(sensor->board_info->adc_channel);
		if (volt < 0) { // negative means get adc fail
			cam_err("get volt fail\n");
			return -EINVAL;
		}
		cam_info("get adc value = %d\n", volt);
		// each ext_name has two adc threshold
		for (i = 0, j = 0; (i < (sensor->board_info->ext_num - 1)) &&
			(j < EXT_NAME_NUM); i += 2, j++) {
			max = sensor->board_info->adc_threshold[i];
			min = sensor->board_info->adc_threshold[(unsigned int)(i + 1)];
			if ((volt < max) && (volt > min)) {
				cam_info("%s adc ext_name: %s\n", __func__,
						 sensor->board_info->ext_name[j]);
				ret = strncpy_s(cdata->info.extend_name, DEVICE_NAME_SIZE - 1,
						  sensor->board_info->ext_name[j],
						  strlen(sensor->board_info->ext_name[j]) + 1);
				if (ret) {
					cam_err("%s:strncpy_c fail, errno = %d\n", __func__, ret);
					return -1;
				}
			}
		}
	}

	/*
	 * if there is only 1 gpio for detect, determine the ext_name
	 * according to the status of GPIO; if there are 2 gpios,
	 * determine the ext_name according to index of the GND gpio.
	 */
	if (sensor->board_info->ext_type == EXT_INFO_GPIO) {
		if (sensor->board_info->adc_gpio_num == 1) {
			/* select the only one adc_gpio, so the index is 0 */
			gpio_status = hisensor_get_gpio_status(sensor, 0);
			if (gpio_status < 0) {
				cam_err("get gpio status fail\n");
				return -1;
			}
			cam_info("%s sensor->board_info->ext_num is %d\n",
				__func__, sensor->board_info->ext_num);
			if (gpio_status < sensor->board_info->ext_num) {
				ret = strncpy_s(cdata->info.extend_name, DEVICE_NAME_SIZE - 1,
					sensor->board_info->ext_name[gpio_status],
					strlen(sensor->board_info->ext_name[gpio_status]) + 1);
				if (ret != EOK)
					return ret;
				cam_info("%s vcell name is %s\n",
					__func__, cdata->info.extend_name);
			}
		} else if (sensor->board_info->adc_gpio_num == 2) {
			/* find the index of GND gpio */
			for (i = 0; i < sensor->board_info->adc_gpio_num; i++) {
				gpio_status = hisensor_get_gpio_status(sensor, i);
				if (gpio_status < 0) {
					cam_err("get gpio status fail\n");
					return -1;
				}
				if (gpio_status == GPIO_GND) {
					ret = strncpy_s(cdata->info.extend_name, DEVICE_NAME_SIZE - 1,
						sensor->board_info->ext_name[i],
						strlen(sensor->board_info->ext_name[i]) + 1);
					if (ret != EOK)
						return ret;
					cam_info("%s vcell name is %s\n",
						__func__, cdata->info.extend_name);
					break;
				}
			}
			if (i == sensor->board_info->adc_gpio_num) {
				cam_err("read vendor info fail\n");
				return -1;
			}
		} else {
			cam_err("adc_gpio_num is wrong\n");
			return -1;
		}
	}

	return 0;
}

static int hisensor_match_id(hwsensor_intf_t* si, void * data)
{
	sensor_t* sensor = NULL;
	struct sensor_cfg_data *cdata = NULL;
	if (NULL == si || NULL == data) {
		cam_err("%s. si or data is NULL", __func__);
		return -EINVAL;
	}

	cam_info("%s enter", __func__);
	sensor = I2S(si);
	if (NULL == sensor || NULL == sensor->board_info) {
		cam_err("%s. sensor or board_info is NULL", __func__);
		return -EINVAL;
	}

	cdata = (struct sensor_cfg_data *)data;
	cdata->data = sensor->board_info->sensor_index;
	memset_s(cdata->info.extend_name, DEVICE_NAME_SIZE, 0, DEVICE_NAME_SIZE);

	if (sensor->board_info->ext_type != 0)
		get_ext_name(sensor, cdata);

	return 0;
}

static hwsensor_vtbl_t hisensor_vtbl =
{
	.get_name    = hisensor_get_name,
	.config      = hisensor_config,
	.power_up    = hisensor_power_up,
	.power_down  = hisensor_power_down,
	.match_id    = hisensor_match_id,
	.csi_enable  = hisensor_csi_enable,
	.csi_disable = hisensor_csi_disable,
};

enum camera_metadata_enum_android_hw_dual_primary_mode{
	ANDROID_HW_DUAL_PRIMARY_FIRST  = 0,
	ANDROID_HW_DUAL_PRIMARY_SECOND = 2,
	ANDROID_HW_DUAL_PRIMARY_BOTH   = 3,
};

#define RESET_TYPE_NONE (0)
#define RESET_TYPE_PRIMARY (1)
#define RESET_TYPE_SECOND (2)
#define RESET_TYPE_BOTH (3)

static void hisensor_deliver_camera_status(struct work_struct *work)
{
	rpc_info_t *rpc_info = NULL;

	if (work == NULL) {
		cam_err("%s work is null", __func__);
		return;
	}

	rpc_info = container_of(work, rpc_info_t, rpc_work);
	if (rpc_info == NULL) {
		cam_err("%s rpc_info is null", __func__);
		return;
	}

	if (rpc_status_change_for_camera(rpc_info->camera_status)) {
		cam_err("%s set_rpc %d fail", __func__, rpc_info->camera_status);
	} else {
		cam_info("%s set_rpc %d success", __func__, rpc_info->camera_status);
	}
}

/* camera radio power ctl for radio frequency interference */
static int hisensor_do_rpc_config(hwsensor_board_info_t *b_info, int rpc_state)
{
	if (rpc_state != SENSOR_RPC_ON &&
		rpc_state != SENSOR_RPC_OFF) {
		cam_err("%s invalid rpc_state %d", __func__, rpc_state);
		return -EINVAL;
	}
	if (!(b_info)) {
		cam_err("%s invalid params b_info", __func__);
		return -EINVAL;
	}

	cam_info("%s set_rpc = %d for work queue", __func__, rpc_state);

	if (!(b_info->rpc_info.rpc_work_queue)) {
		cam_err("%s invalid params rpc_work_queue", __func__);
		return -EINVAL;
	}
	b_info->rpc_info.camera_status = rpc_state; /* set rpc */
	queue_work(b_info->rpc_info.rpc_work_queue,
		&(b_info->rpc_info.rpc_work));

	return 0;
}

static void hisensor_rpc_config_when_hw_reset(hwsensor_board_info_t *b_info,
	int ctl)
{
	if (!(b_info)) {
		cam_err("%s invalid params b_info", __func__);
		return;
	}
	cam_info("%s: need rpc = %d, ctl = %s", __func__, b_info->need_rpc,
		(ctl == CTL_RESET_RELEASE) ? "release" : "hold");
	if (ctl == CTL_RESET_RELEASE) {
		if (b_info->need_rpc == RPC_SUPPORT)
		(void)hisensor_do_rpc_config(b_info, SENSOR_RPC_ON);
	} else {
		if (b_info->need_rpc == RPC_SUPPORT)
		(void)hisensor_do_rpc_config(b_info, SENSOR_RPC_OFF);
	}

	return;
}

/* reset unnecessary sensors to reduce power for cyberverse mode */
static int hisensor_hw_reset_for_cyberverse(hwsensor_board_info_t *b_info,
	int ctl)
{
	unsigned int i;
	int ret = 0;
	unsigned int resets[] = {RESETB, RESETB2, RESETB3};

	for (i = 0; i < ARRAY_SIZE(resets); i++) {
		if (b_info->gpios[resets[i]].gpio != 0) {
			cam_info("%s reset gpio %d", b_info->name,
				b_info->gpios[resets[i]].gpio);
			break;
		}
	}

	if (i >= ARRAY_SIZE(resets)) {
		cam_warn("Do not find reset gpio, just return");
		return ret;
	}

	ret = gpio_request(b_info->gpios[resets[i]].gpio, "reset-0");
	if (ret) {
		cam_err("%s requeset reset pin failed", __func__);
		return ret;
	}
	cam_info("%s: set %s reset %d", __func__, b_info->name, ctl);
	if (ctl == CTL_RESET_HOLD) {
		ret = gpio_direction_output(b_info->gpios[resets[i]].gpio,
				CTL_RESET_HOLD);
	} else {
		ret = gpio_direction_output(b_info->gpios[resets[i]].gpio,
				CTL_RESET_RELEASE);
		hw_camdrv_msleep(GPOI_RELEASE_SLEEP_2MS);
	}
	gpio_free(b_info->gpios[resets[i]].gpio);

	return ret;
}

static int hisensor_do_hw_reset(hwsensor_intf_t* si, int ctl, int id)
{
	//lint -save -e826 -e778 -e774 -e747
	sensor_t* sensor = I2S(si);
	hwsensor_board_info_t *b_info = NULL;
	int ret = 0;

	if (NULL == sensor) {
		cam_err("%s. sensor is NULL", __func__);
		return -EINVAL;
	}

	b_info = sensor->board_info;
	if (NULL == b_info) {
		cam_warn("%s invalid sensor board info", __func__);
		return 0;
	}
	cam_info("reset_type = %d ctl %d id %d", b_info->reset_type, ctl, id);

	if (id == CYBERVERSE_RESET) {
		ret = hisensor_hw_reset_for_cyberverse(b_info, ctl);
		return ret;
	}

	hisensor_rpc_config_when_hw_reset(b_info, ctl);

	if (b_info->reset_type == RESET_TYPE_PRIMARY) {
		ret  = gpio_request(b_info->gpios[RESETB].gpio, "reset-0");
		if (ret) {
			cam_err("%s requeset reset pin failed", __func__);
			return ret;
		}

		if (ctl == CTL_RESET_HOLD) {
			ret  = gpio_direction_output(b_info->gpios[RESETB].gpio, CTL_RESET_HOLD);
		} else {
			ret  = gpio_direction_output(b_info->gpios[RESETB].gpio, CTL_RESET_RELEASE);
			hw_camdrv_msleep(2);
		}
		gpio_free(b_info->gpios[RESETB].gpio);
	} else if (b_info->reset_type == RESET_TYPE_SECOND) {
		ret = gpio_request(b_info->gpios[RESETB2].gpio, "reset-1");
		if (ret) {
			cam_err("%s requeset reset2 pin failed", __func__);
			return ret;
		}

		if (CTL_RESET_HOLD == ctl) {
			if ((ANDROID_HW_DUAL_PRIMARY_SECOND == id) || (ANDROID_HW_DUAL_PRIMARY_BOTH == id)) {
				ret = gpio_direction_output(b_info->gpios[RESETB2].gpio, CTL_RESET_HOLD);
				cam_info("RESETB2 = CTL_RESET_HOLD");
			}
		} else if (CTL_RESET_RELEASE == ctl) {
			if ((ANDROID_HW_DUAL_PRIMARY_SECOND == id) || (ANDROID_HW_DUAL_PRIMARY_BOTH == id)) {
				ret = gpio_direction_output(b_info->gpios[RESETB2].gpio, CTL_RESET_RELEASE);
				cam_info("RESETB2 = CTL_RESET_RELEASE");
				hw_camdrv_msleep(2);
			}
		}

		gpio_free(b_info->gpios[RESETB2].gpio);
	} else if (b_info->reset_type == RESET_TYPE_BOTH) {
		ret  = gpio_request(b_info->gpios[RESETB].gpio, "reset-0");
		if (ret) {
			cam_err("%s:%d requeset reset pin failed", __func__, __LINE__);
			return ret;
		}

		ret = gpio_request(b_info->gpios[RESETB2].gpio, "reset-1");
		if (ret) {
			cam_err("%s:%d requeset reset pin failed", __func__, __LINE__);
			gpio_free(b_info->gpios[RESETB].gpio);
			return ret;
		}

		if(CTL_RESET_HOLD == ctl) {
			ret += gpio_direction_output(b_info->gpios[RESETB].gpio,CTL_RESET_HOLD);
			ret += gpio_direction_output(b_info->gpios[RESETB2].gpio, CTL_RESET_HOLD);
			cam_info("RESETB = CTL_RESET_HOLD, RESETB2 = CTL_RESET_HOLD");
			udelay(2000);
		} else if (CTL_RESET_RELEASE == ctl) {
			if(id == ANDROID_HW_DUAL_PRIMARY_FIRST) {
				ret += gpio_direction_output(b_info->gpios[RESETB].gpio, CTL_RESET_RELEASE);
				ret += gpio_direction_output(b_info->gpios[RESETB2].gpio, CTL_RESET_HOLD);
				cam_info("RESETB = CTL_RESET_RELEASE, RESETB2 = CTL_RESET_HOLD");
			} else if (id == ANDROID_HW_DUAL_PRIMARY_BOTH) {
				ret += gpio_direction_output(b_info->gpios[RESETB].gpio, CTL_RESET_RELEASE);
				ret += gpio_direction_output(b_info->gpios[RESETB2].gpio, CTL_RESET_RELEASE);
				cam_info("RESETB = CTL_RESET_RELEASE, RESETB2 = CTL_RESET_RELEASE");
			} else if (id == ANDROID_HW_DUAL_PRIMARY_SECOND) {
				ret += gpio_direction_output(b_info->gpios[RESETB2].gpio, CTL_RESET_RELEASE);
				ret += gpio_direction_output(b_info->gpios[RESETB].gpio, CTL_RESET_HOLD);
				cam_info("RESETB = CTL_RESET_HOLD, RESETB2 = CTL_RESET_RELEASE");
			}
		}

		gpio_free(b_info->gpios[RESETB].gpio);
		gpio_free(b_info->gpios[RESETB2].gpio);
	} else {
		return 0;
	}

	if (ret)
		cam_err("%s set reset pin failed", __func__);
	else
		cam_info("%s: set reset state=%d, mode=%d", __func__, ctl, id);

	//lint -restore
	return ret;
}

static int hisensor_do_hw_mipisw(hwsensor_intf_t *si)
{
	//lint -save -e826 -e778 -e774 -e747
	sensor_t *sensor = NULL;
	hwsensor_board_info_t *b_info = NULL;
	int ret = 0;
	int index = 0;
	struct sensor_power_setting_array *power_setting_array = NULL;
	struct sensor_power_setting *power_setting = NULL;

	if (si == NULL) {
		cam_err("%s si is null\n", __func__);
		return -EINVAL;
	}

	sensor = I2S(si);
	if (sensor == NULL) {
		cam_err("%s sensor is null\n", __func__);
		return -EINVAL;
	}

	b_info = sensor->board_info;
	if (b_info == NULL) {
		cam_warn("%s invalid sensor board info", __func__);
		return 0;
	}

	cam_info("%s enter.", __func__);
	power_setting_array = &sensor->power_setting_array; // SENSOR_MIPI_SW

	for (index = 0; index < power_setting_array->size; index++) {/*lint !e574*/
		power_setting = &power_setting_array->power_setting[index];
		if (power_setting->seq_type == SENSOR_MIPI_SW)
			break;
	}

	ret  = gpio_request(b_info->gpios[MIPI_SW].gpio, NULL);
	if (ret) {
		cam_err("%s requeset reset pin failed", __func__);
		return ret;
	}

	if (power_setting == NULL) {
		cam_warn("%s invalid sensor board info", __func__);
		return 0;
	}
	ret  = gpio_direction_output(b_info->gpios[MIPI_SW].gpio,
			(power_setting->config_val + 1) % 2);
	cam_info("mipi_sw = gpio[%d], config_val = %d, index = %d",
			b_info->gpios[MIPI_SW].gpio, power_setting->config_val, index);

	gpio_free(b_info->gpios[MIPI_SW].gpio);

	//lint -restore
	return ret;
}

int hisensor_config(hwsensor_intf_t* si, void *argp)
{
	int ret =0;
	struct sensor_cfg_data *data = NULL;
	sensor_t* sensor = NULL;
	hwsensor_board_info_t *b_info = NULL;
	if (si == NULL || argp == NULL || si->vtbl == NULL) {
		cam_err("%s si or argp is null\n", __func__);
		return -EINVAL;
	}

	sensor = I2S(si);
	if (sensor == NULL) {
		cam_err("%s sensor is null\n", __func__);
		return -EINVAL;
	}
	if (!(sensor->board_info)) {
		cam_err("%s invalid params sensor->board_info", __func__);
		return -EINVAL;
	}
	b_info = sensor->board_info;

	data = (struct sensor_cfg_data *)argp;
	cam_info("hisensor cfgtype = %d",data->cfgtype);

	switch(data->cfgtype){
	case SEN_CONFIG_POWER_ON:
		mutex_lock(&hisensor_power_lock);
		if (HWSENSRO_POWER_DOWN == sensor->state) {
			if (si->vtbl->power_up == NULL){
				cam_err("%s. si->vtbl->power_up is null.", __func__);
				mutex_unlock(&hisensor_power_lock);
				return -EINVAL;
			} else {
				ret = si->vtbl->power_up(si);
				if (0 == ret) {
					sensor->state = HWSENSOR_POWER_UP;
				} else {
					cam_err("%s. power up fail.", __func__);
				}
			}
		}
		/*lint -e455 -esym(455,*)*/
		mutex_unlock(&hisensor_power_lock);
		/*lint -e455 +esym(455,*)*/
		break;

	case SEN_CONFIG_POWER_OFF:
		mutex_lock(&hisensor_power_lock);
		if (HWSENSOR_POWER_UP == sensor->state) {
			if (si->vtbl->power_down == NULL) {
				cam_err("%s. si->vtbl->power_down is null", __func__);
				mutex_unlock(&hisensor_power_lock);
				return -EINVAL;
			} else {
				ret = si->vtbl->power_down(si);
				if (0 != ret)
					cam_err("%s. power_down fail", __func__);
				sensor->state = HWSENSRO_POWER_DOWN;
			}

			if (b_info->need_rpc == RPC_SUPPORT_DYNAMIC) {
				ret = hisensor_do_rpc_config(
					sensor->board_info,
					SENSOR_RPC_OFF);
				cam_info("%s: cfgtype(POWER_OFF), rpc_state%d",
					__func__, data->data);
			}
		}
		/*lint -e455 -esym(455,*)*/
		mutex_unlock(&hisensor_power_lock);
		/*lint -e455 +esym(455,*)*/
		break;

	case SEN_CONFIG_WRITE_REG:
		break;

	case SEN_CONFIG_READ_REG:
		break;

	case SEN_CONFIG_WRITE_REG_SETTINGS:
		break;

	case SEN_CONFIG_READ_REG_SETTINGS:
		break;

	case SEN_CONFIG_ENABLE_CSI:
		break;

	case SEN_CONFIG_DISABLE_CSI:
		break;

	case SEN_CONFIG_MATCH_ID:
		if (NULL == si->vtbl->match_id) {
			cam_err("%s. si->vtbl->match_id is null", __func__);
			ret = -EINVAL;
		} else {
			ret = si->vtbl->match_id(si,argp);
		}
		break;

	case SEN_CONFIG_RESET_HOLD:
		ret = hisensor_do_hw_reset(si, CTL_RESET_HOLD, data->mode);
		break;

	case SEN_CONFIG_RESET_RELEASE:
		ret = hisensor_do_hw_reset(si, CTL_RESET_RELEASE, data->mode);
		break;

	case SEN_CONFIG_MIPI_SWITCH:
		ret = hisensor_do_hw_mipisw(si);
		break;

	case SEN_CONFIG_FPC_CHECK:
		ret = hisensor_do_hw_buckle_check(si, argp, FSIN);
		break;

	case SEN_CONFIG_BTB_CHECK:
		ret = hisensor_do_hw_buckle_check(si, argp, BTB_CHECK);
		break;

	case SEN_CONFIG_RPC_STATE:
		if (b_info->need_rpc == RPC_SUPPORT_DYNAMIC) {
			ret = hisensor_do_rpc_config(sensor->board_info,
				data->data);
			cam_info("%s cfgtype(CONFIG_RPC), rpc_state%d",
				__func__, data->data);
		}
		break;

	default:
		cam_err("%s cfgtype %d is error", __func__, data->cfgtype);
		break;
	}

	cam_debug("%s exit %d", __func__, ret);
	return ret;
}

static void init_rpc_workqueue(hwsensor_board_info_t *b_info)
{
	if (!(b_info)) {
		cam_err("%s invalid params b_info", __func__);
		return;
	}
	if ((b_info->need_rpc != RPC_SUPPORT) &&
		(b_info->need_rpc != RPC_SUPPORT_DYNAMIC))
		return;

	INIT_WORK(&(b_info->rpc_info.rpc_work), hisensor_deliver_camera_status);

	b_info->rpc_info.rpc_work_queue =
		create_singlethread_workqueue(CAMERA_RPC_WORKQUEUE);

	if (!b_info->rpc_info.rpc_work_queue) {
		cam_err("%s create rpc workqueue error", __func__);
		return;
	}

	cam_info("%s create rpc workqueue success", __func__);
	return;
}

static int32_t hisensor_platform_probe(struct platform_device* pdev)
{
	int rc = 0;

	sensor_t *s_ctrl = NULL;
	atomic_t* hisensor_powered = NULL;
	cam_info("%s enter", __func__);
	if (NULL == pdev) {
		cam_err("%s:%d pdev is null.\n", __func__, __LINE__);
		return -EINVAL;
	}

	s_ctrl = kzalloc(sizeof(sensor_t), GFP_KERNEL);
	if (!s_ctrl) {
		cam_err("%s:%d kzalloc failed\n", __func__, __LINE__);
		return -ENOMEM;
	}

	rc = hw_sensor_get_dt_data(pdev, s_ctrl);
	if (rc < 0) {
		cam_err("%s:%d no dt data rc %d", __func__, __LINE__, rc);
		rc = -ENODEV;
		goto hisensor_probe_fail;
	}

	rc = hw_sensor_get_dt_power_setting_data(pdev, s_ctrl);
	if (rc < 0) {
		cam_err("%s:%d no dt power setting data rc %d", __func__, __LINE__, rc);
		rc = -ENODEV;
		goto hisensor_probe_fail;
	}

	mutex_init(&hisensor_power_lock);
	hisensor_powered = kzalloc(sizeof(atomic_t), GFP_KERNEL);
	if (!hisensor_powered) {
		cam_err("%s:%d kzalloc failed\n", __func__, __LINE__);
		goto hisensor_probe_fail;
	}

	s_ctrl->intf.vtbl = &hisensor_vtbl;
	s_ctrl->p_atpowercnt = hisensor_powered;
	s_ctrl->dev = &pdev->dev;
	rc = hwsensor_register(pdev, &(s_ctrl->intf));
	if (0 != rc) {
		cam_err("%s:%d hwsensor_register fail.\n", __func__, __LINE__);
		goto hisensor_probe_fail;
	}

	rc = rpmsg_sensor_register(pdev, (void*)s_ctrl);
	if (0 != rc) {
		cam_err("%s:%d rpmsg_sensor_register fail.\n", __func__, __LINE__);
		hwsensor_unregister(pdev);
		goto hisensor_probe_fail;
	}

	init_rpc_workqueue(s_ctrl->board_info);

	/* this code just for the bug at elle,
	 * this config is lpm3_gpu_buck in dts
	 **/
	if (s_ctrl->board_info->lpm3_gpu_buck == 1) {
		if (!lpm3) {
			lpm3 = ioremap(LPM3_GPU_BUCK_ADDR, LPM3_GPU_BUCK_MAP_BYTE);
			if (!lpm3)
				cam_err("%s:ioremap failed!", __func__);
			else
				cam_info("%s:ioremap success!", __func__);
		} else {
			cam_info("%s:lpm3 is not NULL,continue!", __func__);
		}
	} else {
		cam_info("%s:do not set lpm3_gpu_buck!", __func__);
	}

	return rc;

hisensor_probe_fail:
	if (s_ctrl->power_setting_array.power_setting != NULL)
		kfree(s_ctrl->power_setting_array.power_setting);
	if (s_ctrl->power_down_setting_array.power_setting != NULL)
		kfree(s_ctrl->power_down_setting_array.power_setting);
	if (s_ctrl->board_info != NULL) {
		kfree(s_ctrl->board_info);
		s_ctrl->board_info = NULL;
	}
	if (s_ctrl->p_atpowercnt != NULL) {
		kfree((atomic_t*)s_ctrl->p_atpowercnt);
		s_ctrl->p_atpowercnt = NULL;
	}
	if (s_ctrl  != NULL) {
		kfree(s_ctrl);
		s_ctrl = NULL;
	}

	return rc;
}

static void destory_rpc_workqueue(hwsensor_board_info_t *b_info)
{
	if (!(b_info)) {
		cam_err("%s invalid params b_info", __func__);
		return;
	}
	if ((b_info->need_rpc != RPC_SUPPORT) &&
		(b_info->need_rpc != RPC_SUPPORT_DYNAMIC))
		return;
	if (!(b_info->rpc_info.rpc_work_queue)) {
		return;
	}
	destroy_workqueue(b_info->rpc_info.rpc_work_queue);
	b_info->rpc_info.rpc_work_queue = NULL;
	cam_info("%s: destroy rpc workqueue success", __func__);
	return;
}

static int32_t hisensor_platform_remove(struct platform_device* pdev)
{
	struct v4l2_subdev *sd = platform_get_drvdata(pdev);
	sensor_t *s_ctrl = NULL;
	if (!sd) {
		cam_err("%s: Subdevice is NULL\n", __func__);
		return 0;
	}

	s_ctrl = (sensor_t *)v4l2_get_subdevdata(sd);
	if (!s_ctrl) {
		cam_err("%s: eeprom device is NULL\n", __func__);
		return 0;
	}

	if ((s_ctrl->board_info != NULL) &&
			(s_ctrl->board_info->lpm3_gpu_buck == 1)) {
		if (lpm3) {
			iounmap(lpm3);
			lpm3 = NULL;
			cam_info("%s:iounmap success!", __func__);
		} else {
			cam_info("%s:lpm3 is NULL, do not iounmap!", __func__);
		}
	} else {
		cam_info("%s:do not lpm3 iounmap!", __func__);
	}

	destory_rpc_workqueue(s_ctrl->board_info);

	rpmsg_sensor_unregister((void*)s_ctrl);

	hwsensor_unregister(pdev);

	if (s_ctrl->power_setting_array.power_setting != NULL)
		kfree(s_ctrl->power_setting_array.power_setting);
	if (s_ctrl->power_down_setting_array.power_setting != NULL)
		kfree(s_ctrl->power_down_setting_array.power_setting);
	if (s_ctrl->p_atpowercnt != NULL) {
		kfree((atomic_t*)s_ctrl->p_atpowercnt);
		s_ctrl->p_atpowercnt = NULL;
	}
	if (s_ctrl->board_info)
		kfree(s_ctrl->board_info);

	kfree(s_ctrl);

	return 0;
}

static const struct of_device_id hisensor_dt_match[] =
{
	{
		.compatible = "huawei,sensor",
	},
	{
	},
};

MODULE_DEVICE_TABLE(of, hisensor_dt_match);

static struct platform_driver hisensor_platform_driver = {
	.driver = {
		.name           = "huawei,sensor",
		.owner          = THIS_MODULE,
		.of_match_table = hisensor_dt_match,
	},

	.probe = hisensor_platform_probe,
	.remove = hisensor_platform_remove,
};

static int __init hisensor_init_module(void)
{
	cam_info("enter %s",__func__);
	return platform_driver_register(&hisensor_platform_driver);
}

static void __exit hisensor_exit_module(void)
{
	platform_driver_unregister(&hisensor_platform_driver);
}
//lint -restore

/*lint -e528 -esym(528,*)*/
module_init(hisensor_init_module);
module_exit(hisensor_exit_module);
/*lint -e528 +esym(528,*)*/
/*lint -e753 -esym(753,*)*/
MODULE_DESCRIPTION("hisensor");
MODULE_LICENSE("GPL v2");
/*lint -e753 +esym(753,*)*/

