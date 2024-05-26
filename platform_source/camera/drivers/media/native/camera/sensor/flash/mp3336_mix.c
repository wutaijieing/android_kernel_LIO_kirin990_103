/*
 * mp3336_mix.c
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
#include <linux/device.h>
#include <linux/pm_wakeup.h>

/* MP3336 Registers define */
#define REG_CHIPID 0x00
#define REG_MODE 0x01
#define REG_PEAK_CURRENT 0x02
#define REG_FLASH_TIMER 0x03
#define REG_VBL 0x04 // Low battery voltage

#define REG_L1_FL 0x06
#define REG_L2_FL 0x07
#define REG_L1_TX 0x08
#define REG_L2_TX 0x09
#define REG_IFL_ACT 0x0A
#define REG_IFL_MIN 0x0B
#define REG_L1_TOR 0x0C
#define REG_L2_TOR 0x0D
#define REG_FAULT_INDICATION_A 0x0E
#define REG_FAULT_INDICATION_B 0x0F

#define CHIP_ID 0x38
#define CHIP_ID_MASK 0xF8
#define ASIST_MODE (0x2 << 1)
#define STR_FLASH_MODE (0x3 << 1)
#define LED2_EN (0x1 << 3)
#define LED1_EN (0x1 << 4)
#define STR_MOD (0x1 << 6)
#define LED_SD (0x1 << 3)

#define OVER_VOLTAGE_PROTECT (0x1 << 6)
#define OVER_TEMP_PROTECT (0x1 << 3)
#define LED_SHORT (0x1 << 4)
#define LED_OPEN 0x1

#define INVALID_GPIO 999
#define FLASH_TIMEOUT_TIME (0x04 << 4) // 400ms
#define MP3336_TORCH_MAX_CUR 188 // mA, ref lm3646 max is 188mA
#define MP3336_FLASH_MAX_CUR 1500 // mA, ref lm3646
#define MAX_LIGHTNESS_PARAM_NUM_RT 2
#define LIGHTNESS_PARAM_FD_INDEX_RT 0
#define LIGHTNESS_PARAM_MODE_INDEX_RT 1
/* Internal data struct defie */
#define MAX_BRIGHTNESS_FORMMI 0x09 // MMI

#define MAX_TX_CURRENT 149 // 186mA

#define WAKE_LOCK_ENABLE 1
#define WAKE_LOCK_DISABLE 0

#define BACK_FLASH_USE_LED2 1
#define torch_current_to_level(x) ((x) * 100 / 125) // reg value = 1.25 / step

enum {
	RESET = 0,
	STROBE,
	TORCH,
	MAX_PIN,
};

enum mp3336_mix_current_conf {
	CURRENT_TORCH_LEVEL_MMI_BACK = 0,
	CURRENT_TORCH_LEVEL_MMI_FRONT = 1,
	CURRENT_TORCH_LEVEL_RT_BACK = 2,
	CURRENT_TORCH_LEVEL_RT_FRONT = 3,
	CURRENT_MIX_MAX = 4,
};

/* Internal data struct define */
struct hw_mp3336_mix_private_data_t {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	struct wakeup_source mp3336_wakelock;
#else
	struct wakeup_source *mp3336_wakelock;
#endif
	unsigned int need_wakelock;
	/* flash control pin */
	unsigned int pin[MAX_PIN];
	/* flash electric current config */
	unsigned int ecurrent[CURRENT_MIX_MAX];
	unsigned int chipid;
	unsigned int ctrltype;
	unsigned int led_type;
	unsigned int front_cur;
	unsigned int back_cur;
};

/* Internal varible define */
static struct hw_mp3336_mix_private_data_t g_mp3336_pdata;
struct hw_flash_ctrl_t g_mp3336_mix_ctrl;
extern struct dsm_client *client_flash;

define_kernel_flash_mutex(mp3336);

/* Function define */
static int hw_mp3336_param_check(char *buf, unsigned long *param,
	int num_of_par);

static int hw_mp3336_set_pin(struct hw_flash_ctrl_t *flash_ctrl,
	unsigned int pin_type,
	unsigned int state)
{
	struct hw_mp3336_mix_private_data_t *pdata = NULL;
	int rc = 0;

	if (!flash_ctrl || !flash_ctrl->pdata) {
		cam_err("%s flash_ctrl or pdata is NULL", __func__);
		return -EINVAL;
	}

	pdata = (struct hw_mp3336_mix_private_data_t *)flash_ctrl->pdata;

	cam_debug("%s strobe0 = %d, state = %d", __func__,
		pdata->pin[pin_type], state);
	if (pdata->pin[pin_type] != INVALID_GPIO) {
		rc = gpio_direction_output(pdata->pin[pin_type], (int)state);
		if (rc < 0)
			cam_err("%s gpio output is err rc=%d", __func__, rc);
	}
	return rc;
}

static int hw_mp3336_init(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_mp3336_mix_private_data_t *pdata = NULL;
	int rc;

	cam_debug("%s enter", __func__);

	if (!flash_ctrl || !flash_ctrl->pdata) {
		cam_err("%s flash_ctrl or pdata is NULL", __func__);
		return -EINVAL;
	}

	pdata = (struct hw_mp3336_mix_private_data_t *)flash_ctrl->pdata;
	flash_ctrl->flash_type = FLASH_MIX; // mix flash
	flash_ctrl->pctrl = devm_pinctrl_get_select(flash_ctrl->dev,
		PINCTRL_STATE_DEFAULT);
	if (!flash_ctrl->pctrl) {
		cam_err("%s failed to set pin", __func__);
		return -EIO;
	}

	g_mp3336_pdata.front_cur = 0;
	g_mp3336_pdata.back_cur = 0;

	rc = gpio_request(pdata->pin[RESET], "flash-reset");
	if (rc < 0) {
		cam_err("%s failed to request reset pin", __func__);
		return -EIO;
	}

	if (pdata->pin[STROBE] != INVALID_GPIO) {
		rc = gpio_request(pdata->pin[STROBE], "flash-strobe");
		if (rc < 0) {
			cam_err("%s failed to request strobe pin", __func__);
			goto err1;
		}
	}

	if (pdata->pin[TORCH] != INVALID_GPIO) {
		rc = gpio_request(pdata->pin[TORCH], "flash-torch");
		if (rc < 0) {
			cam_err("%s failed to request torch pin", __func__);
			goto err2;
		}
	}

	rc = hw_mp3336_set_pin(flash_ctrl, RESET, LOW);
	if (rc < 0) {
		cam_err("%s failed to set reset pin", __func__);
		goto err3;
	}

	if (pdata->need_wakelock == WAKE_LOCK_ENABLE) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		wakeup_source_init(&pdata->mp3336_wakelock, "mp3336");
#else
		pdata->mp3336_wakelock = wakeup_source_register(flash_ctrl->dev, "mp3336");
		if (!pdata->mp3336_wakelock) {
			rc = -EINVAL;
			cam_err("%s: wakeup source register failed", __func__);
			goto err3;
		}
#endif
	}
	return rc;

err3:
	if (pdata->pin[TORCH] != INVALID_GPIO)
		gpio_free(pdata->pin[TORCH]);
err2:
	if (pdata->pin[STROBE] != INVALID_GPIO)
		gpio_free(pdata->pin[STROBE]);
err1:
	gpio_free(pdata->pin[RESET]);
	return rc;
}

static int hw_mp3336_exit(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_mp3336_mix_private_data_t *pdata = NULL;

	cam_debug("%s enter", __func__);

	if (!flash_ctrl || !flash_ctrl->func_tbl ||
		!flash_ctrl->func_tbl->flash_off ||
		!flash_ctrl->pdata) {
		cam_err("%s flash_ctrl or flash_ctrl->pdata or flash_ctrl->func_tbl or flash_ctrl->func_tbl->flash_off is NULL",
			__func__);
		return -EINVAL;
	}
	flash_ctrl->func_tbl->flash_off(flash_ctrl);

	pdata = (struct hw_mp3336_mix_private_data_t *)flash_ctrl->pdata;

	if (pdata->pin[TORCH] != INVALID_GPIO)
		gpio_free(pdata->pin[TORCH]);

	if (pdata->pin[STROBE] != INVALID_GPIO)
		gpio_free(pdata->pin[STROBE]);

	gpio_free(pdata->pin[RESET]);
	flash_ctrl->pctrl = devm_pinctrl_get_select(flash_ctrl->dev,
		PINCTRL_STATE_IDLE);

	return 0;
}

static void hw_mp3336_flash_use_led(struct hw_flash_ctrl_t *flash_ctrl,
	struct hw_flash_cfg_data *cdata,
	unsigned int reg_cur_flash_front, unsigned int reg_cur_flash_back,
	unsigned int reg_cur_torch_front, unsigned int reg_cur_torch_back)
{
	struct hw_mp3336_mix_private_data_t *pdata;
	struct hw_flash_i2c_client *i2c_client;
	struct hw_flash_i2c_fn_t *i2c_func;

	unsigned char reg3_val = 0;

	i2c_client = flash_ctrl->flash_i2c_client;
	i2c_func = i2c_client->i2c_func_tbl;
	pdata = flash_ctrl->pdata;

	if (cdata->mode == FLASH_MODE) {
		// set flash timeout
		i2c_func->i2c_read(i2c_client, REG_FLASH_TIMER, &reg3_val);
		i2c_func->i2c_write(i2c_client, REG_FLASH_TIMER,
			(reg3_val & (~LED_SD)) | FLASH_TIMEOUT_TIME);

		// set TX current
		i2c_func->i2c_write(i2c_client, REG_L1_TX, MAX_TX_CURRENT);
		i2c_func->i2c_write(i2c_client, REG_L2_TX, MAX_TX_CURRENT);

		if (pdata->led_type == BACK_FLASH_USE_LED2) {
			i2c_func->i2c_write(i2c_client, REG_L1_FL,
				(unsigned char)reg_cur_flash_front);
			i2c_func->i2c_write(i2c_client, REG_L2_FL,
				(unsigned char)reg_cur_flash_back);
			cam_info("flash1 = 0x%x, flash2 = 0x%x",
				reg_cur_flash_front, reg_cur_flash_back);
		} else {
			i2c_func->i2c_write(i2c_client, REG_L2_FL,
				(unsigned char)reg_cur_flash_front);
			i2c_func->i2c_write(i2c_client, REG_L1_FL,
				(unsigned char)reg_cur_flash_back);
			cam_info("flash2 = 0x%x, flash1 = 0x%x",
				reg_cur_flash_front, reg_cur_flash_back);
		}
	} else {
		if (pdata->led_type == BACK_FLASH_USE_LED2) {
			i2c_func->i2c_write(i2c_client, REG_L1_TOR,
				(unsigned char)reg_cur_torch_front);
			i2c_func->i2c_write(i2c_client, REG_L2_TOR,
				(unsigned char)reg_cur_torch_back);
			cam_info("torch1 = 0x%x, torch2 = 0x%x",
				reg_cur_torch_front, reg_cur_torch_back);
		} else {
			i2c_func->i2c_write(i2c_client, REG_L2_TOR,
				(unsigned char)reg_cur_torch_front);
			i2c_func->i2c_write(i2c_client, REG_L1_TOR,
				(unsigned char)reg_cur_torch_back);
			cam_info("torch2 = 0x%x, torch1 = 0x%x",
				reg_cur_torch_front, reg_cur_torch_back);
		}
	}
}

static int hw_mp3336_set_mode(struct hw_flash_ctrl_t *flash_ctrl, void *data)
{
	struct hw_flash_cfg_data *cdata = (struct hw_flash_cfg_data *)data;
	struct hw_flash_i2c_client *i2c_client = NULL;
	struct hw_flash_i2c_fn_t *i2c_func = NULL;
	struct hw_mp3336_mix_private_data_t *pdata = NULL;

	unsigned char reg1_val = 0;
	unsigned char reg2_val = 0;
	unsigned char reg4_val = 0;
	unsigned char mode = 0;

	unsigned int reg_cur_flash_front = 0;
	unsigned int reg_cur_flash_back = 0;
	unsigned int reg_cur_torch_front = 0;
	unsigned int reg_cur_torch_back = 0;
	unsigned int tmp_led = 0;

	unsigned int cur = 0;
	int rc = 0;

	if (!flash_ctrl || !cdata || !flash_ctrl->flash_i2c_client ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl ||
		!flash_ctrl->pdata ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_read ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_write) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	i2c_client = flash_ctrl->flash_i2c_client;
	i2c_func = i2c_client->i2c_func_tbl;
	pdata = flash_ctrl->pdata;

	cur = (unsigned int)cdata->data;
	// clear error status
	i2c_func->i2c_read(i2c_client, REG_FAULT_INDICATION_A, &reg1_val);
	i2c_func->i2c_read(i2c_client, REG_FAULT_INDICATION_B, &reg1_val);

	i2c_func->i2c_read(i2c_client, REG_MODE, &reg1_val);
	i2c_func->i2c_read(i2c_client, REG_PEAK_CURRENT, &reg2_val);
	i2c_func->i2c_read(i2c_client, REG_VBL, &reg4_val);

	// current limit 2.5A
	reg2_val &= ~(0x3 << 3); // clear bit3, bit4
	reg2_val |= (0x1 << 3); // set 2.5A(0x1)
	i2c_func->i2c_write(i2c_client, REG_PEAK_CURRENT, reg2_val);

	// input DC Current limit 3A
	reg4_val &= 0xF0; // clear bit 0~3
	reg4_val |= 0x08; // set 3A(0x8)
	i2c_func->i2c_write(i2c_client, REG_VBL, reg4_val);

	if (cdata->mode == FLASH_MODE) {
		mode = STR_FLASH_MODE;
		if (cur > MP3336_FLASH_MAX_CUR)
			cur = MP3336_FLASH_MAX_CUR;

		tmp_led = cur * 100 / 784; // reg value = 7.84 / step
		if (flash_ctrl->mix_pos == HWFLASH_POSITION_FORE)
			reg_cur_flash_front = tmp_led;
		else
			reg_cur_flash_back = tmp_led;

		cam_info("FLASH_MODE reg_cur_torch_front = %d, reg_cur_torch_back = %d",
			reg_cur_flash_front, reg_cur_flash_back);
	} else {
		mode = ASIST_MODE; // ASIST_MODE for software control torch
		if (cur > MP3336_TORCH_MAX_CUR)
			cur = MP3336_TORCH_MAX_CUR;
		// reg value = 1.25 / step
		tmp_led = torch_current_to_level(cur);
		if (flash_ctrl->mix_pos == HWFLASH_POSITION_FORE) {
			// keep back current same
			reg_cur_torch_back =
				torch_current_to_level(g_mp3336_pdata.back_cur);
			reg_cur_torch_front = tmp_led; // update front current
		} else {
			// keep front current same
			reg_cur_torch_front =
				torch_current_to_level(g_mp3336_pdata.front_cur);
			reg_cur_torch_back = tmp_led; // update back current
		}
		cam_info("ASIST_MODE reg_cur_torch_front = %d, reg_cur_torch_back = %d",
			reg_cur_torch_front, reg_cur_torch_back);

		if (flash_ctrl->mix_pos == HWFLASH_POSITION_FORE)
			g_mp3336_pdata.front_cur = cur;
		else
			g_mp3336_pdata.back_cur = cur;
	}
	i2c_func->i2c_write(i2c_client, REG_MODE, reg1_val | mode);

	hw_mp3336_flash_use_led(flash_ctrl, cdata, reg_cur_flash_front,
		reg_cur_flash_back, reg_cur_torch_front, reg_cur_torch_back);

	i2c_func->i2c_read(i2c_client, REG_MODE, &reg1_val);
	if (cdata->mode == FLASH_MODE)
		rc = i2c_func->i2c_write(i2c_client, REG_MODE,
			(reg1_val | LED1_EN | LED2_EN) & (~STR_MOD));
	else
		rc = i2c_func->i2c_write(i2c_client, REG_MODE,
			(reg1_val | LED1_EN | LED2_EN));

	if ((rc < 0) && (!dsm_client_ocuppy(client_flash))) {
		dsm_client_record(client_flash, "flash i2c transfer fail\n");
		dsm_client_notify(client_flash, DSM_FLASH_I2C_ERROR_NO);
		cam_warn("[I/DSM] %s dsm_client_notify",
			client_flash->client_name);
	}
	return 0;
}


static int hw_mp3336_on(struct hw_flash_ctrl_t *flash_ctrl, void *data)
{
	struct hw_mp3336_mix_private_data_t *pdata = NULL;
	struct hw_flash_cfg_data *cdata = (struct hw_flash_cfg_data *)data;

	if (!flash_ctrl || !cdata || !flash_ctrl->pdata) {
		cam_err("%s flash_ctrl or cdata is NULL", __func__);
		return -EINVAL;
	}
	pdata = (struct hw_mp3336_mix_private_data_t *)flash_ctrl->pdata;
	if (!pdata) {
		cam_err("%s pdata is NULL", __func__);
		return -EINVAL;
	}

	cam_info("%s mode = %d, level = %d", __func__,
		cdata->mode, cdata->data);

	mutex_lock(flash_ctrl->hw_flash_mutex);
	if (pdata->need_wakelock == WAKE_LOCK_ENABLE) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		__pm_stay_awake(&pdata->mp3336_wakelock);
#else
		__pm_stay_awake(pdata->mp3336_wakelock);
#endif
	}

	hw_mp3336_set_pin(flash_ctrl, STROBE, LOW);
	hw_mp3336_set_pin(flash_ctrl, TORCH, LOW);

	hw_mp3336_set_mode(flash_ctrl, data);
	flash_ctrl->state.mode = cdata->mode;
	flash_ctrl->state.data = cdata->data;
	hw_mp3336_set_pin(flash_ctrl, STROBE, HIGH);
	hw_mp3336_set_pin(flash_ctrl, TORCH, HIGH);

	mutex_unlock(flash_ctrl->hw_flash_mutex);

	return 0;
}

// read back error status
static void hw_mp3336_read_error_status(struct hw_flash_i2c_client *i2c_client,
	struct hw_flash_i2c_fn_t *i2c_func)
{
	unsigned char fault_val = 0;
	unsigned char fault_val2 = 0;

	if (!i2c_func || !i2c_client) {
		cam_err("%s invalid params", __func__);
		return;
	}
	i2c_func->i2c_read(i2c_client, REG_FAULT_INDICATION_A, &fault_val);
	if ((fault_val & OVER_VOLTAGE_PROTECT) &&
		(!dsm_client_ocuppy(client_flash))) {
		dsm_client_record(client_flash,
			"flash over voltage protect Flag1[0x%x]\n", fault_val);
		dsm_client_notify(client_flash, DSM_FLASH_OPEN_SHOTR_ERROR_NO);
		cam_warn("[I/DSM] %s dsm_client_notify",
			client_flash->client_name);
	}
	if ((fault_val & OVER_TEMP_PROTECT) &&
		(!dsm_client_ocuppy(client_flash))) {
		dsm_client_record(client_flash,
			"flash temperature too hot Flag1[0x%x]\n", fault_val);
		dsm_client_notify(client_flash, DSM_FLASH_HOT_DIE_ERROR_NO);
		cam_warn("[I/DSM] %s dsm_client_notify",
			client_flash->client_name);
	}
	if ((fault_val & LED_SHORT) && (!dsm_client_ocuppy(client_flash))) {
		dsm_client_record(client_flash,
			"flash led short fault FlagReg2[0x%x]\n", fault_val);
		dsm_client_notify(client_flash, DSM_FLASH_OPEN_SHOTR_ERROR_NO);
		cam_warn("[I/DSM] %s dsm_client_notify",
			client_flash->client_name);
	}

	i2c_func->i2c_read(i2c_client, REG_FAULT_INDICATION_B, &fault_val2);
	if ((fault_val2 & LED_OPEN) && (!dsm_client_ocuppy(client_flash))) {
		dsm_client_record(client_flash,
			"flash led open fault FlagReg2[0x%x]\n", fault_val2);
		dsm_client_notify(client_flash, DSM_FLASH_OPEN_SHOTR_ERROR_NO);
		cam_warn("[I/DSM] %s dsm_client_notify",
			client_flash->client_name);
	}

	cam_info("%s Fault and Flag Indication Reg E 0x%x Reg F 0x%x",
		__func__, fault_val, fault_val2);
}
static int hw_mp3336_off(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_flash_i2c_client *i2c_client = NULL;
	struct hw_flash_i2c_fn_t *i2c_func = NULL;
	unsigned char mode_val = 0;
	unsigned char timeout_val = 0;
	unsigned char flash_act_cur = 0;
	unsigned char flash_min_cur = 0;
	struct hw_flash_cfg_data cdata = {0};

	struct hw_mp3336_mix_private_data_t *pdata;

	cam_debug("%s enter", __func__);
	if (!flash_ctrl || !flash_ctrl->pdata ||
		!flash_ctrl->flash_i2c_client ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_read ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_write) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}
	pdata = (struct hw_mp3336_mix_private_data_t *)flash_ctrl->pdata;

	mutex_lock(flash_ctrl->hw_flash_mutex);
	if (flash_ctrl->state.mode == STANDBY_MODE) {
		mutex_unlock(flash_ctrl->hw_flash_mutex);
		return 0;
	}

	i2c_client = flash_ctrl->flash_i2c_client;
	i2c_func = i2c_client->i2c_func_tbl;

	hw_mp3336_read_error_status(i2c_client, i2c_func);

	i2c_func->i2c_read(i2c_client, REG_IFL_ACT, &flash_act_cur);
	i2c_func->i2c_read(i2c_client, REG_IFL_MIN, &flash_min_cur);
	cam_info("%s IFL ACT current 0x%x MIN Current 0x%x",
		__func__, flash_act_cur, flash_min_cur);

	if (flash_ctrl->mix_pos == HWFLASH_POSITION_FORE)
		g_mp3336_pdata.front_cur = 0;
	else
		g_mp3336_pdata.back_cur = 0;

	// two torch on
	if (g_mp3336_pdata.front_cur != 0 || g_mp3336_pdata.back_cur != 0) {
		cam_info("%s two torch, close %s", __func__,
			(flash_ctrl->mix_pos == HWFLASH_POSITION_FORE) ? "fore" : "back");

		cdata.mode = TORCH_MODE;
		if (flash_ctrl->mix_pos == HWFLASH_POSITION_FORE) {
			cdata.data = g_mp3336_pdata.back_cur;
			flash_ctrl->mix_pos = HWFLASH_POSITION_REAR;
		} else {
			cdata.data = g_mp3336_pdata.front_cur;
			flash_ctrl->mix_pos = HWFLASH_POSITION_FORE;
		}

		hw_mp3336_set_mode(flash_ctrl, &cdata);
	} else {
		flash_ctrl->state.mode = STANDBY_MODE;
		flash_ctrl->state.data = 0;

		i2c_func->i2c_read(i2c_client, REG_MODE, &mode_val);
		i2c_func->i2c_read(i2c_client, REG_FLASH_TIMER, &timeout_val);
		i2c_func->i2c_write(i2c_client, REG_MODE,
			(mode_val & (~LED1_EN) & (~LED2_EN) & (~ASIST_MODE) & (~STR_FLASH_MODE)));
		i2c_func->i2c_write(i2c_client, REG_FLASH_TIMER,
			(timeout_val & (~LED_SD)));

		hw_mp3336_set_pin(flash_ctrl, STROBE, LOW);
		hw_mp3336_set_pin(flash_ctrl, TORCH, LOW);
	}

	if (pdata->need_wakelock == WAKE_LOCK_ENABLE) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		__pm_relax(&pdata->mp3336_wakelock);
#else
		__pm_relax(pdata->mp3336_wakelock);
#endif
	}

	cam_info("%s end", __func__);
	mutex_unlock(flash_ctrl->hw_flash_mutex);

	return 0;
}

static int hw_mp3336_get_dt_data(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_mp3336_mix_private_data_t *pdata = NULL;
	struct device_node *node = NULL;
	int i;
	int rc = -EINVAL;

	cam_debug("%s enter", __func__);

	if (!flash_ctrl || !flash_ctrl->pdata ||
		!flash_ctrl->dev || !flash_ctrl->dev->of_node) {
		cam_err("%s invalid params", __func__);
		return rc;
	}
	pdata = (struct hw_mp3336_mix_private_data_t *)flash_ctrl->pdata;

	node = flash_ctrl->dev->of_node;

	rc = of_property_read_u32_array(node, "vendor,flash-pin",
		pdata->pin, (unsigned long)MAX_PIN);
	if (rc < 0) {
		cam_err("%s get dt flash-pin failed line %d",
			__func__, __LINE__);
		return rc;
	}
	for (i = 0; i < MAX_PIN; i++)
		cam_info("%s pin[%d] = %d", __func__,
			i, pdata->pin[i]);

	rc = of_property_read_u32(node, "vendor,flash-chipid", &pdata->chipid);
	cam_info("%s vendor,flash-chipid 0x%x, rc %d", __func__, pdata->chipid, rc);
	if (rc < 0) {
		cam_err("%s failed %d", __func__, __LINE__);
		return rc;
	}

	rc = of_property_read_u32(node, "vendor,flash-ctrltype",
		&pdata->ctrltype);
	cam_info("%s vendor,ctrltype 0x%x, rc %d", __func__, pdata->ctrltype, rc);
	if (rc < 0) {
		cam_err("%s failed %d", __func__, __LINE__);
		return rc;
	}

	rc = of_property_read_u32(node, "vendor,led-type", &pdata->led_type);
	cam_info("%s vendor,led-type %d, rc %d\n",
		__func__, pdata->led_type, rc);
	if (rc < 0) {
		cam_err("%s read led-type failed %d\n", __func__, __LINE__);
		return rc;
	}

	rc = of_property_read_u32(node, "vendor,need-wakelock",
		(u32 *)&pdata->need_wakelock);
	cam_info("%s vendor,need-wakelock %d, rc %d\n",
		__func__, pdata->need_wakelock, rc);
	if (rc < 0) {
		pdata->need_wakelock = WAKE_LOCK_DISABLE;
		cam_err("%s failed %d\n", __func__, __LINE__);
		return rc;
	}

	rc = of_property_read_u32_array(node, "vendor,flash-current",
		pdata->ecurrent, CURRENT_MIX_MAX);
	if (rc < 0) {
		cam_err("%s read flash-current failed line %d\n",
			__func__, __LINE__);
		return rc;
	}
	for (i = 0; i < CURRENT_MIX_MAX; i++)
		cam_info("%s ecurrent[%d]=%d.\n", __func__, i,
			pdata->ecurrent[i]);

	return rc;
}

static ssize_t hw_mp3336_flash_lightness_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc;

	if (!buf) {
		cam_err("%s buf is NULL", __func__);
		return -EINVAL;
	}
	rc = scnprintf(buf, PAGE_SIZE, "mode=%d, data=%d\n",
		g_mp3336_mix_ctrl.state.mode, g_mp3336_mix_ctrl.state.data);

	return rc;
}

static int hw_mp3336_param_check(char *buf, unsigned long *param,
	int num_of_par)
{
	char *token = NULL;
	unsigned int base = 0;
	int cnt;

	if (!buf || !param) {
		cam_err("%s buf or param is NULL", __func__);
		return -EINVAL;
	}

	token = strsep(&buf, " ");

	for (cnt = 0; cnt < num_of_par; cnt++) {
		if (token) {
			// format 0x**
			if ((token[1] == 'x') || (token[1] == 'X'))
				base = 16; // hex
			else
				base = 10; // decimal
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

static ssize_t hw_mp3336_flash_lightness_store_imp(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct hw_flash_cfg_data cdata = {0};
	unsigned long param[MAX_LIGHTNESS_PARAM_NUM_RT] = {0};
	int rc;
	int flash_id = 0;

	rc = hw_mp3336_param_check((char *)buf, param,
		MAX_LIGHTNESS_PARAM_NUM_RT);
	if (rc < 0) {
		cam_err("%s failed to check param", __func__);
		return rc;
	}

	flash_id = (int)param[LIGHTNESS_PARAM_FD_INDEX_RT]; // 0 - flash id
	cdata.mode = (int)param[LIGHTNESS_PARAM_MODE_INDEX_RT]; // 1 - mode
	cam_info("%s flash_id = %d, cdata.mode = %d", __func__,
		flash_id, cdata.mode);

	if (cdata.mode == STANDBY_MODE) {
		rc = hw_mp3336_off(&g_mp3336_mix_ctrl);
		if (rc < 0) {
			cam_err("%s flash off error", __func__);
			return rc;
		}
	} else if (cdata.mode == TORCH_MODE) {
		if (g_mp3336_mix_ctrl.mix_pos == HWFLASH_POSITION_FORE)
			cdata.data =
				g_mp3336_pdata.ecurrent[CURRENT_TORCH_LEVEL_RT_FRONT];
		else
			cdata.data =
				g_mp3336_pdata.ecurrent[CURRENT_TORCH_LEVEL_RT_BACK];

		cam_info("%s mode = %d, max_current = %d", __func__,
			cdata.mode, cdata.data);

		rc = hw_mp3336_on(&g_mp3336_mix_ctrl, &cdata);
		if (rc < 0) {
			cam_err("%s flash on error", __func__);
			return rc;
		}
	} else {
		cam_err("%s wrong mode=%d", __func__, cdata.mode);
		return -EINVAL;
	}

	return count;
}

static ssize_t hw_mp3336_flash_lightness_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	g_mp3336_mix_ctrl.mix_pos = HWFLASH_POSITION_REAR; // call back flash
	return hw_mp3336_flash_lightness_store_imp(dev, attr, buf, count);
}

static ssize_t hw_mp3336_flash_lightness_f_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	g_mp3336_mix_ctrl.mix_pos = HWFLASH_POSITION_FORE; // call front flash
	return hw_mp3336_flash_lightness_store_imp(dev, attr, buf, count);
}

static ssize_t hw_mp3336_flash_mask_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc;

	if (!buf) {
		cam_err("%s buf is NULL", __func__);
		return -EINVAL;
	}
	rc = scnprintf(buf, PAGE_SIZE, "flash_mask_disabled=%d\n",
		g_mp3336_mix_ctrl.flash_mask_enable);

	return rc;
}

static ssize_t hw_mp3336_flash_mask_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	if (!buf) {
		cam_err("%s buf is NULL", __func__);
		return -EINVAL;
	}
	if (buf[0] == '0') // char '0' for mask disable
		g_mp3336_mix_ctrl.flash_mask_enable = false;
	else
		g_mp3336_mix_ctrl.flash_mask_enable = true;

	cam_info("%s flash_mask_enable=%d", __func__,
		g_mp3336_mix_ctrl.flash_mask_enable);
	return (ssize_t)count;
}

static void hw_mp3336_torch_brightness_set_imp(struct led_classdev *cdev,
	enum led_brightness brightness)
{
	struct hw_flash_cfg_data cdata;
	int rc = 0;
	unsigned int led_bright = brightness;

	cam_info("%s brightness = %d", __func__, brightness);
	if (led_bright == LED_OFF) {
		rc = hw_mp3336_off(&g_mp3336_mix_ctrl);
		if (rc < 0) {
			cam_err("%s pmu_led off error", __func__);
			return;
		}
	} else {
		cdata.mode = TORCH_MODE;
		if (g_mp3336_mix_ctrl.mix_pos == HWFLASH_POSITION_FORE)
			cdata.data =
				g_mp3336_pdata.ecurrent[CURRENT_TORCH_LEVEL_MMI_FRONT];
		else
			cdata.data =
				g_mp3336_pdata.ecurrent[CURRENT_TORCH_LEVEL_MMI_BACK];

		cam_info("%s brightness=0x%x, mode=%d, data=%d",
			__func__, brightness, cdata.mode, cdata.data);
		rc = hw_mp3336_on(&g_mp3336_mix_ctrl, &cdata);
		if (rc < 0) {
			cam_err("%s flash on error", __func__);
			return;
		}
	}
}

static void hw_mp3336_torch_brightness_set(struct led_classdev *cdev,
	enum led_brightness brightness)
{
	g_mp3336_mix_ctrl.mix_pos = HWFLASH_POSITION_REAR;
	hw_mp3336_torch_brightness_set_imp(cdev, brightness);
}

static void hw_mp3336_torch_brightness_f_set(struct led_classdev *cdev,
	enum led_brightness brightness)
{
	g_mp3336_mix_ctrl.mix_pos = HWFLASH_POSITION_FORE;
	hw_mp3336_torch_brightness_set_imp(cdev, brightness);
}

// for RT
static struct device_attribute g_mp3336_flash_lightness =
	__ATTR(flash_lightness, 0660, hw_mp3336_flash_lightness_show,
	hw_mp3336_flash_lightness_store); // 660:-wr-wr---

static struct device_attribute g_mp3336_flash_lightness_f =
	__ATTR(flash_lightness_f, 0660, hw_mp3336_flash_lightness_show,
	hw_mp3336_flash_lightness_f_store); // 660:-wr-wr---

static struct device_attribute g_mp3336_flash_mask =
	__ATTR(flash_mask, 0660, hw_mp3336_flash_mask_show,
	hw_mp3336_flash_mask_store); // 660:-wr-wr---

static int hw_mp3336_register_attribute(struct hw_flash_ctrl_t *flash_ctrl,
	struct device *dev)
{
	int rc;

	if (!flash_ctrl || !dev) {
		cam_err("%s flash_ctrl or dev is NULL", __func__);
		return -EINVAL;
	}

	flash_ctrl->cdev_torch.name = "torch";
	flash_ctrl->cdev_torch.max_brightness =
		(enum led_brightness)MAX_BRIGHTNESS_FORMMI;
	flash_ctrl->cdev_torch.brightness_set = hw_mp3336_torch_brightness_set;
	rc = led_classdev_register((struct device *)dev,
		&flash_ctrl->cdev_torch);
	if (rc < 0) {
		cam_err("%s failed to register torch classdev", __func__);
		goto err_out;
	}

	flash_ctrl->cdev_torch1.name = "torch_front";
	flash_ctrl->cdev_torch1.max_brightness =
		(enum led_brightness)MAX_BRIGHTNESS_FORMMI;
	flash_ctrl->cdev_torch1.brightness_set =
		hw_mp3336_torch_brightness_f_set;
	rc = led_classdev_register((struct device *)dev,
		&flash_ctrl->cdev_torch1);
	if (rc < 0) {
		cam_err("%s failed to register torch_front classdev", __func__);
		goto err_create_torch_front_file;
	}
	rc = device_create_file(dev, &g_mp3336_flash_lightness);
	if (rc < 0) {
		cam_err("%s failed to creat flash_lightness attribute",
			__func__);
		goto err_create_flash_lightness_file;
	}

	rc = device_create_file(dev, &g_mp3336_flash_lightness_f);
	if (rc < 0) {
		cam_err("%s failed to creat flash_f_lightness attribute",
			__func__);
		goto err_create_flash_f_lightness_file;
	}
	rc = device_create_file(dev, &g_mp3336_flash_mask);
	if (rc < 0) {
		cam_err("%s failed to creat flash_mask attribute", __func__);
		goto err_create_flash_mask_file;
	}
	return 0;
err_create_flash_mask_file:
	device_remove_file(dev, &g_mp3336_flash_lightness_f);
err_create_flash_f_lightness_file:
	device_remove_file(dev, &g_mp3336_flash_lightness);
err_create_flash_lightness_file:
	led_classdev_unregister(&flash_ctrl->cdev_torch1);
err_create_torch_front_file:
	led_classdev_unregister(&flash_ctrl->cdev_torch);
err_out:
	return rc;
}

extern int register_camerafs_attr(struct device_attribute *attr);
static int hw_mp3336_match(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_flash_i2c_client *i2c_client = NULL;
	struct hw_flash_i2c_fn_t *i2c_func = NULL;
	struct hw_mp3336_mix_private_data_t *pdata = NULL;
	unsigned char id = 0;

	cam_debug("%s enter", __func__);

	if (!flash_ctrl || !flash_ctrl->pdata ||
		!flash_ctrl->flash_i2c_client ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_read) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	i2c_client = flash_ctrl->flash_i2c_client;
	i2c_func = i2c_client->i2c_func_tbl;
	pdata = (struct hw_mp3336_mix_private_data_t *)flash_ctrl->pdata;

	/*
	 * Enable mp3336 switch to standby current is 10ua,
	 * if match id success, reset pin will always be enabled.
	 */
	hw_mp3336_set_pin(flash_ctrl, RESET, HIGH);
	i2c_func->i2c_read(i2c_client, REG_CHIPID, &id);
	cam_info("%s 0x%x", __func__, id);
	id = id & CHIP_ID_MASK;
	if (id != CHIP_ID) {
		cam_err("%s match error, 0x%x != 0x%x",
			__func__, id, CHIP_ID);

		/*
		 * Enable mp3336 switch to shutdown when matchid fail,
		 * current is 1.3ua
		 */
		hw_mp3336_set_pin(flash_ctrl, RESET, LOW);
		return -EINVAL;
	}

	register_camerafs_attr(&g_mp3336_flash_lightness);

	register_camerafs_attr(&g_mp3336_flash_lightness_f);
	return 0;
}

static int hw_mp3336_remove(struct i2c_client *client)
{
	cam_debug("%s enter", __func__);
	if (!client) {
		cam_err("%s client is NULL", __func__);
		return -EINVAL;
	}
	g_mp3336_mix_ctrl.func_tbl->flash_exit(&g_mp3336_mix_ctrl);

	client->adapter = NULL;
	return 0;
}

static const struct i2c_device_id g_mp3336_id[] = {
	{ "mp3336_mix", (unsigned long) &g_mp3336_mix_ctrl },
	{}
};

static const struct of_device_id g_mp3336_dt_match[] = {
	{ .compatible = "vendor,mp3336_mix" },
	{}
};
MODULE_DEVICE_TABLE(of, mp3336_dt_match);

static struct i2c_driver g_mp3336_i2c_driver = {
	.probe = hw_flash_i2c_probe,
	.remove = hw_mp3336_remove,
	.id_table = g_mp3336_id,
	.driver = {
		.name = "hw_mp3336_mix",
		.of_match_table = g_mp3336_dt_match,
	},
};

static int __init hw_mp3336_module_init(void)
{
	cam_info("%s erter", __func__);
	return i2c_add_driver(&g_mp3336_i2c_driver);
}

static void __exit hw_mp3336_module_exit(void)
{
	cam_info("%s enter", __func__);
	i2c_del_driver(&g_mp3336_i2c_driver);
}

static struct hw_flash_i2c_client g_mp3336_i2c_client;

static struct hw_flash_fn_t g_mp3336_func_tbl = {
	.flash_config = hw_flash_config,
	.flash_init = hw_mp3336_init,
	.flash_exit = hw_mp3336_exit,
	.flash_on = hw_mp3336_on,
	.flash_off = hw_mp3336_off,
	.flash_match = hw_mp3336_match,
	.flash_get_dt_data = hw_mp3336_get_dt_data,
	.flash_register_attribute = hw_mp3336_register_attribute,
};

struct hw_flash_ctrl_t g_mp3336_mix_ctrl = {
	.flash_i2c_client = &g_mp3336_i2c_client,
	.func_tbl = &g_mp3336_func_tbl,
	.hw_flash_mutex = &flash_mut_mp3336,
	.pdata = (void *)&g_mp3336_pdata,
	.flash_mask_enable = true,
	.state = {
		.mode = STANDBY_MODE,
	},
};

module_init(hw_mp3336_module_init);
module_exit(hw_mp3336_module_exit);
MODULE_DESCRIPTION("MP3336 FLASH");
MODULE_LICENSE("GPL v2");
