/*
 * lm3646_mix.c
 *
 * driver for lm3646_mix
 *
 * Copyright (c) 2017-2020 Huawei Technologies Co., Ltd.
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

/* LM3646 Registers define */
#define REG_CHIPID 0x00
#define REG_ENABLE 0x01
#define REG_FLAGS1 0x08
#define REG_FLAGS2 0x09
#define REG_FLASH_TIMEOUT 0x04
#define REG_MAX_CURRENT 0x05
#define REG_LED1_FLASH_CURRENT_CONTROL 0x06
#define REG_LED1_TORCH_CURRENT_CONTROL 0x07

#define CHIP_ID 0x10
#define CHIP_ID_MASK 0x38
#define MODE_STANDBY 0x00
#define MODE_TORCH 0x02
#define MODE_FLASH 0x03
#define STROBE_ENABLE 0x80
#define TORCH_ENABLE 0x80
#define STROBE_DISABLE 0x00
#define TORCH_DISABLE 0x00
#define TX_ENABLE 0x08
#define INDUCTOR_CURRENT_LIMMIT 0xe0
#define FLASH_TIMEOUT_TIME 0x47 // 400ms

#define FLASH_LED_MAX 0x7f
#define TORCH_LED_MAX 0x7f

#define LM3646_TORCH_MAX_CUR 188 // mA
#define LM3646_FLASH_MAX_CUR 1500 // mA
#define MAX_LIGHTNESS_PARAM_NUM_RT 2
#define LIGHTNESS_PARAM_FD_INDEX_RT 0
#define LIGHTNESS_PARAM_MODE_INDEX_RT 1

#define MAX_TORCH_CUR_NUM 8
#define MAX_FLASH_CUR_NUM 16

#define MAX_BRIGHTNESS_FORMMI 0x09
#define ENABLE_SHORT_PROTECT 0x80
#define LM3646_RESET_HOLD_TIME 2 // 2ms

#define OVER_VOLTAGE_PROTECT 0x80
#define OVER_CURRENT_PROTECT 0x10
#define OVER_TEMP_PROTECT 0x20
#define LED_SHORT 0x0C
#define UNDER_VOLTAGE_LOCKOUT 0x04

#define INVALID_GPIO 999

#define WAKE_LOCK_ENABLE 1
#define WAKE_LOCK_DISABLE 0

#define BASE 100
#define LM3646_CUR_STEP_LEV 146
#define LM3646_BASE_TORCH_CUR 253
#define LM3646_MAX_TORCH_DEFAULT_LEVEL 0x04 // 116mA
#define LM3646_TORCH_DEFAULT_LEVEL 0x21 // 50mA

/* Internal data struct define */
#define BACK_FLASH_USE_LED2 1

enum lm3646_pin_type {
	RESET = 0,
	STROBE,
	TORCH,
	MAX_PIN,
};

enum lm3646_mix_current_conf {
	CURRENT_TORCH_LEVEL_MMI_BACK,
	CURRENT_TORCH_LEVEL_MMI_FRONT,
	CURRENT_TORCH_LEVEL_RT_BACK,
	CURRENT_TORCH_LEVEL_RT_FRONT,
	CURRENT_MIX_MAX,
};

/* Internal data struct define */
struct hw_lm3646_mix_private_data_t {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	struct wakeup_source lm3646_wakelock;
#else
	struct wakeup_source *lm3646_wakelock;
#endif
	int need_wakelock;
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
static struct hw_lm3646_mix_private_data_t g_lm3646_pdata;
struct hw_flash_ctrl_t g_lm3646_mix_ctrl;

static int g_torch_arry[MAX_TORCH_CUR_NUM] = {
	24, 47, 70, 94, 117, 141, 164, 188
};

static int g_flash_arry[MAX_FLASH_CUR_NUM] = {
	94, 188, 281, 375,
	469, 563, 656, 750,
	844, 938, 1031, 1125,
	1219, 1313, 1406, 1500
};

extern struct dsm_client *client_flash;

define_kernel_flash_mutex(lm3646);

/* Function define */
static int hw_lm3646_param_check(char *buf, unsigned long *param,
	int num_of_par);

static int hw_lm3646_set_pin(struct hw_flash_ctrl_t *flash_ctrl,
	unsigned int pin_type,
	unsigned int state)
{
	struct hw_lm3646_mix_private_data_t *pdata = NULL;
	int rc = 0;

	if (!flash_ctrl || !flash_ctrl->pdata) {
		cam_err("%s flash_ctrl or pdata is NULL", __func__);
		return -EINVAL;
	}

	pdata = (struct hw_lm3646_mix_private_data_t *)flash_ctrl->pdata;

	cam_debug("%s strobe0 = %d, state = %d", __func__,
		pdata->pin[pin_type], state);
	if (pdata->pin[pin_type] != INVALID_GPIO) {
		rc = gpio_direction_output(pdata->pin[pin_type], (int)state);
		if (rc < 0)
			cam_err("%s gpio output is err rc = %d", __func__, rc);
	}
	return rc;
}

static int hw_lm3646_init(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_lm3646_mix_private_data_t *pdata = NULL;
	int rc;

	cam_info("%s enter", __func__);

	if (!flash_ctrl || !flash_ctrl->pdata) {
		cam_err("%s flash_ctrl or pdata is NULL", __func__);
		return -EINVAL;
	}

	pdata = (struct hw_lm3646_mix_private_data_t *)flash_ctrl->pdata;
	flash_ctrl->flash_type = FLASH_MIX; // mix flash
	flash_ctrl->pctrl = devm_pinctrl_get_select(flash_ctrl->dev,
		PINCTRL_STATE_DEFAULT);
	if (!flash_ctrl->pctrl) {
		cam_err("%s failed to set pin", __func__);
		return -EIO;
	}

	g_lm3646_pdata.front_cur = 0;
	g_lm3646_pdata.back_cur = 0;

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

	rc = hw_lm3646_set_pin(flash_ctrl, RESET, LOW);
	if (rc < 0) {
		cam_err("%s failed to set reset pin", __func__);
		goto err3;
	}
	msleep(LM3646_RESET_HOLD_TIME);

	if (pdata->need_wakelock == WAKE_LOCK_ENABLE) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		wakeup_source_init(&pdata->lm3646_wakelock, "lm3646");
#else
		pdata->lm3646_wakelock = wakeup_source_register(flash_ctrl->dev, "lm3646");
		if (!pdata->lm3646_wakelock) {
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

static int hw_lm3646_exit(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_lm3646_mix_private_data_t *pdata = NULL;

	cam_debug("%s enter", __func__);

	if (!flash_ctrl || !flash_ctrl->func_tbl ||
		!flash_ctrl->func_tbl->flash_off ||
		!flash_ctrl->pdata) {
		cam_err("%s flash_ctrl or flash_ctrl->pdata or flash_ctrl->func_tbl or flash_ctrl->func_tbl->flash_off is NULL",
			__func__);
		return -EINVAL;
	}
	flash_ctrl->func_tbl->flash_off(flash_ctrl);

	pdata = (struct hw_lm3646_mix_private_data_t *)flash_ctrl->pdata;

	if (pdata->pin[TORCH] != INVALID_GPIO)
		gpio_free(pdata->pin[TORCH]);

	if (pdata->pin[STROBE] != INVALID_GPIO)
		gpio_free(pdata->pin[STROBE]);

	gpio_free(pdata->pin[RESET]);
	flash_ctrl->pctrl = devm_pinctrl_get_select(flash_ctrl->dev,
		PINCTRL_STATE_IDLE);

	return 0;
}

static int hw_lm3646_find_match_flash_current(int cur_flash)
{
	int cur_level = 0;
	int i;

	cam_info("%s ernter cur_flash %d\n", __func__, cur_flash);
	if (cur_flash <= 0) {
		cam_err("%s current set is error", __func__);
		return -EINVAL;
	}

	if (cur_flash >= LM3646_FLASH_MAX_CUR) {
		cam_warn("%s current set is %d", __func__, cur_flash);
		return (MAX_FLASH_CUR_NUM - 1); // return max level
	}

	for (i = 0; i < MAX_FLASH_CUR_NUM; i++) {
		if (cur_flash <= g_flash_arry[i]) {
			cam_info("%s  i %d\n", __func__, i);
			break;
		}
	}

	if (i == 0) {
		cur_level = i;
	} else {
		if (i == MAX_FLASH_CUR_NUM)
			i = MAX_FLASH_CUR_NUM - 1; // find last valid data

		if ((cur_flash - g_flash_arry[i - 1]) <
			(g_flash_arry[i] - cur_flash))
			cur_level = i - 1; // last data
		else
			cur_level = i;
	}

	return cur_level;
}

static int hw_lm3646_find_match_torch_current(int cur_torch)
{
	int cur_level = 0;
	int i;

	cam_info("%s ernter cur_torch %d\n", __func__, cur_torch);
	if (cur_torch <= 0) {
		cam_err("%s current set is error", __func__);
		return -EINVAL;
	}

	if (cur_torch > LM3646_TORCH_MAX_CUR) {
		cam_warn("%s current set is %d", __func__, cur_torch);
		return (MAX_TORCH_CUR_NUM - 1); // return max level
	}

	for (i = 0; i < MAX_TORCH_CUR_NUM; i++) {
		if (cur_torch <= g_torch_arry[i]) {
			cam_info("%s  i %d\n", __func__, i);
			break;
		}
	}

	if (i == 0) {
		cur_level = i;
	} else {
		if (i == MAX_TORCH_CUR_NUM)
			i = MAX_TORCH_CUR_NUM - 1; // find last valid data

		if ((cur_torch - g_torch_arry[i - 1]) <
			(g_torch_arry[i] - cur_torch))
			cur_level = i - 1; // last data
		else
			cur_level = i;
	}

	return cur_level;
}

static int hw_lm3646_find_match_led1_torch_current(int cur_torch)
{
	int integer_torch;

	if (cur_torch < 0) {
		cam_err("%s current set is error", __func__);
		return -EINVAL;
	}

	if (cur_torch > LM3646_TORCH_MAX_CUR) {
		cam_warn("%s current set is %d", __func__, cur_torch);
		return TORCH_LED_MAX; // return max level
	}

	if (cur_torch * BASE < LM3646_BASE_TORCH_CUR)
		integer_torch = 0;
	else
		// first level start from 1
		integer_torch = (cur_torch * BASE - LM3646_BASE_TORCH_CUR) /
			LM3646_CUR_STEP_LEV + 1;

	cam_info("%s current = %d integer = %d ",
		__func__, cur_torch, integer_torch);

	return integer_torch;
}

static int hw_lm3646_set_dual_torch_mode(struct hw_flash_ctrl_t *flash_ctrl,
	int cur, unsigned char *regmaxcurrent, unsigned char *regcurrenttorch)
{
	int cur_sum;
	int led1_current;
	int led1_level;
	int level;
	int max_current_level;
	struct hw_lm3646_mix_private_data_t *pdata = NULL;

	if (!flash_ctrl || !flash_ctrl->pdata ||
		!regmaxcurrent || !regcurrenttorch) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	pdata = flash_ctrl->pdata;

	// calculate total current to modify max current
	if (flash_ctrl->mix_pos == HWFLASH_POSITION_FORE)
		cur_sum = cur + g_lm3646_pdata.back_cur;
	else
		cur_sum = cur + g_lm3646_pdata.front_cur;

	// calculate max current reg val
	level = hw_lm3646_find_match_torch_current(cur_sum);
	if (level < 0)
		level = LM3646_MAX_TORCH_DEFAULT_LEVEL;

	// bits[6-4] for cur torch level
	max_current_level = (level & 0x7) << 4;

	if (pdata->led_type == BACK_FLASH_USE_LED2) {
		// back is on, now open front torch
		if (flash_ctrl->mix_pos == HWFLASH_POSITION_FORE)
			// keep back current same
			led1_current = g_torch_arry[level] -
				g_lm3646_pdata.back_cur;
		else // front is on, now open back torch
			// keep front current same
			led1_current = g_lm3646_pdata.front_cur;
	} else {
		// front is on, now open back torch
		if (flash_ctrl->mix_pos == HWFLASH_POSITION_REAR)
			// keep front current same
			led1_current = g_torch_arry[level] -
				g_lm3646_pdata.front_cur;
		else // back is on, now open front torch
			// keep back current same
			led1_current = g_lm3646_pdata.back_cur;
	}

	// calculate led1 current reg val
	led1_level = hw_lm3646_find_match_led1_torch_current(led1_current);
	if (led1_level < 0)
		led1_level = LM3646_TORCH_DEFAULT_LEVEL;

	*regcurrenttorch |= led1_level;
	*regmaxcurrent = max_current_level;

	cam_info("%s now open: %s, cur: %d ,total_cur: %d, max_current:%d, max_level:%d, led1_current:%d ",
		__func__,
		(flash_ctrl->mix_pos == HWFLASH_POSITION_FORE) ? "fore" : "back",
		cur, cur_sum, g_torch_arry[level], level, led1_current);

	return 0;
}

static void hw_lm3646_clear_flag(struct hw_flash_ctrl_t *flash_ctrl)
{
	unsigned char val = 0;

	struct hw_flash_i2c_client *i2c_client = flash_ctrl->flash_i2c_client;
	struct hw_flash_i2c_fn_t *i2c_func = flash_ctrl->flash_i2c_client->i2c_func_tbl;

	/* clear error flag, resume chip */
	i2c_func->i2c_read(i2c_client, REG_FLAGS1, &val);
	if ((val & (OVER_VOLTAGE_PROTECT | OVER_CURRENT_PROTECT)) &&
		!dsm_client_ocuppy(client_flash)) {
		dsm_client_record(client_flash,
			"flash OVP or OCP FlagReg1[0x%x]\n", val);
		dsm_client_notify(client_flash, DSM_FLASH_OPEN_SHOTR_ERROR_NO);
		cam_warn("[I/DSM] %s dsm_client_notify",
			client_flash->client_name);
	}
	if ((val & OVER_TEMP_PROTECT) && !dsm_client_ocuppy(client_flash)) {
		dsm_client_record(client_flash,
			"flash temperature is too hot FlagReg1[0x%x]\n", val);
		dsm_client_notify(client_flash, DSM_FLASH_HOT_DIE_ERROR_NO);
		cam_warn("[I/DSM] %s dsm_client_notify",
			client_flash->client_name);
	}
	if ((val & UNDER_VOLTAGE_LOCKOUT) && !dsm_client_ocuppy(client_flash)) {
		dsm_client_record(client_flash,
			"flash UVLO! FlagReg1[0x%x]\n", val);
		dsm_client_notify(client_flash,
			DSM_FLASH_UNDER_VOLTAGE_LOCKOUT_ERROR_NO);
		cam_warn("[I/DSM] %s dsm_client_notify",
			client_flash->client_name);
	}
}
static int hw_lm3646_set_mode(struct hw_flash_ctrl_t *flash_ctrl, void *data)
{
	struct hw_flash_cfg_data *cdata = (struct hw_flash_cfg_data *)data;

	struct hw_flash_i2c_client *i2c_client = NULL;
	struct hw_flash_i2c_fn_t *i2c_func = NULL;
	struct hw_lm3646_mix_private_data_t *pdata = NULL;

	unsigned char val = 0;
	unsigned char mode;
	unsigned char regmaxcurrent = 0;
	unsigned char regcurrentflash = 0;
	unsigned char regcurrenttorch = 0;
	int cur;
	int level;
	int rc;

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
	cur = cdata->data;

	hw_lm3646_clear_flag(flash_ctrl);

	i2c_func->i2c_read(i2c_client, REG_FLAGS2, &val);

	if (cdata->mode == FLASH_MODE) {
		level = hw_lm3646_find_match_flash_current(cur);

		mode = INDUCTOR_CURRENT_LIMMIT | TX_ENABLE | MODE_FLASH;
		/* 1 means I2C control, 0 means GPIO control */
		if (pdata->ctrltype == 1)
			regcurrentflash = STROBE_DISABLE;
		else
			regcurrentflash = STROBE_ENABLE;

		if (pdata->led_type == BACK_FLASH_USE_LED2) {
			if (flash_ctrl->mix_pos == HWFLASH_POSITION_FORE)
				// 0-Back Flash, 0x7F-Front Flash
				regcurrentflash |= FLASH_LED_MAX;
		} else {
			if (flash_ctrl->mix_pos == HWFLASH_POSITION_REAR)
				// 0x7F-Back Flash, 0-Front Flash
				regcurrentflash |= FLASH_LED_MAX;
		}

		if (level > 0)
			// Low 4 bits for cur flash level
			regmaxcurrent = level;
	} else {
		level = hw_lm3646_find_match_torch_current(cur);
		if (level >= 0)
			// bits[6-4] for cur torch level
			regmaxcurrent = (level & 0x7) << 4;

		mode = INDUCTOR_CURRENT_LIMMIT | TX_ENABLE | MODE_TORCH;
		/* 1 means I2C control, 0 means GPIO control */
		if (pdata->ctrltype == 1)
			regcurrenttorch = TORCH_DISABLE;
		else
			regcurrenttorch = TORCH_ENABLE;

		/*
		 * Two cases just open one torch
		 * 1. initial state, front current = 0, back current = 0
		 * 2. back torch not open, front torch change from
		 *    first level to second level
		 */
		if ((g_lm3646_pdata.back_cur == 0 &&
			g_lm3646_pdata.front_cur == 0) ||
			(flash_ctrl->mix_pos == HWFLASH_POSITION_FORE &&
			g_lm3646_pdata.back_cur == 0)) {
			if (pdata->led_type == BACK_FLASH_USE_LED2) {
				if (flash_ctrl->mix_pos == HWFLASH_POSITION_FORE)
					// 0-Back Flash, 0x7F-Front Flash
					regcurrenttorch |= TORCH_LED_MAX;
			} else {
				if (flash_ctrl->mix_pos == HWFLASH_POSITION_REAR)
					// 0x7F-Back Flash, 0-Front Flash
					regcurrenttorch |= TORCH_LED_MAX;
			}
		} else {
			// two torch on
			rc = hw_lm3646_set_dual_torch_mode(flash_ctrl, cur,
				&regmaxcurrent, &regcurrenttorch);
			if (rc < 0)
				cam_err("%s set dual torch error", __func__);
		}

		if (flash_ctrl->mix_pos == HWFLASH_POSITION_FORE)
			g_lm3646_pdata.front_cur = cur;
		else
			g_lm3646_pdata.back_cur = cur;
	}
	i2c_func->i2c_write(i2c_client, REG_FLASH_TIMEOUT, FLASH_TIMEOUT_TIME);
	i2c_func->i2c_write(i2c_client, REG_MAX_CURRENT,
		regmaxcurrent | ENABLE_SHORT_PROTECT);
	i2c_func->i2c_write(i2c_client, REG_LED1_FLASH_CURRENT_CONTROL,
		regcurrentflash);
	i2c_func->i2c_write(i2c_client, REG_LED1_TORCH_CURRENT_CONTROL,
		regcurrenttorch);
	rc = i2c_func->i2c_write(i2c_client, REG_ENABLE, mode);
	if ((rc < 0) && !dsm_client_ocuppy(client_flash)) {
		dsm_client_record(client_flash,
			"flash i2c transfer fail rc=%d\n", rc);
		dsm_client_notify(client_flash, DSM_FLASH_I2C_ERROR_NO);
		cam_warn("[I/DSM] %s dsm_client_notify",
			client_flash->client_name);
	}

	cam_debug("%s mode = %d, level = %d\n",
		__func__, cdata->mode, cdata->data);
	cam_info("%s regmaxcurrent = 0x%x regcurrentflash = 0x%x regcurrenttorch = 0x%x",
		__func__, regmaxcurrent, regcurrentflash & 0x7f,
		regcurrenttorch & 0x7f);
	return 0;
}

static int hw_lm3646_on(struct hw_flash_ctrl_t *flash_ctrl, void *data)
{
	struct hw_lm3646_mix_private_data_t *pdata = NULL;
	struct hw_flash_cfg_data *cdata = (struct hw_flash_cfg_data *)data;

	if (!flash_ctrl || !cdata || !flash_ctrl->pdata) {
		cam_err("%s flash_ctrl or cdata is NULL", __func__);
		return -EINVAL;
	}
	pdata = (struct hw_lm3646_mix_private_data_t *)flash_ctrl->pdata;
	if (!pdata) {
		cam_err("%s pdata is NULL", __func__);
		return -EINVAL;
	}

	cam_info("%s mode = %d, level = %d",
		__func__, cdata->mode, cdata->data);

	mutex_lock(flash_ctrl->hw_flash_mutex);
	if (pdata->need_wakelock == WAKE_LOCK_ENABLE) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		__pm_stay_awake(&pdata->lm3646_wakelock);
#else
		__pm_stay_awake(pdata->lm3646_wakelock);
#endif
	}
	// Enable lm3646 switch to standby current is 10ua
	hw_lm3646_set_pin(flash_ctrl, RESET, HIGH);
	hw_lm3646_set_pin(flash_ctrl, STROBE, LOW);
	hw_lm3646_set_pin(flash_ctrl, TORCH, LOW);

	hw_lm3646_set_mode(flash_ctrl, data);
	flash_ctrl->state.mode = cdata->mode;
	flash_ctrl->state.data = cdata->data;
	hw_lm3646_set_pin(flash_ctrl, STROBE, HIGH);
	hw_lm3646_set_pin(flash_ctrl, TORCH, HIGH);

	mutex_unlock(flash_ctrl->hw_flash_mutex);

	return 0;
}

static int hw_lm3646_off(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_flash_i2c_client *i2c_client = NULL;
	struct hw_flash_i2c_fn_t *i2c_func = NULL;
	unsigned char val = 0;
	struct hw_lm3646_mix_private_data_t *pdata = NULL;
	struct hw_flash_cfg_data cdata = {0};

	cam_debug("%s enter", __func__);
	if (!flash_ctrl || !flash_ctrl->pdata ||
		!flash_ctrl->flash_i2c_client ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_read ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_write) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}
	pdata = (struct hw_lm3646_mix_private_data_t *)flash_ctrl->pdata;

	mutex_lock(flash_ctrl->hw_flash_mutex);
	if (flash_ctrl->state.mode == STANDBY_MODE) {
		mutex_unlock(flash_ctrl->hw_flash_mutex);
		return 0;
	}

	i2c_client = flash_ctrl->flash_i2c_client;
	i2c_func = i2c_client->i2c_func_tbl;

	i2c_func->i2c_read(i2c_client, REG_FLAGS1, &val);
	if (val & (OVER_VOLTAGE_PROTECT | OVER_CURRENT_PROTECT)) {
		if (!dsm_client_ocuppy(client_flash)) {
			dsm_client_record(client_flash,
				"flash short or open FlagReg1[0x%x]\n", val);
			dsm_client_notify(client_flash,
				DSM_FLASH_OPEN_SHOTR_ERROR_NO);
			cam_warn("[I/DSM] %s dsm_client_notify",
				client_flash->client_name);
		}
	}
	if (val & OVER_TEMP_PROTECT) {
		if (!dsm_client_ocuppy(client_flash)) {
			dsm_client_record(client_flash,
				"flash temperature is too hot FlagReg1[0x%x]\n",
				val);
			dsm_client_notify(client_flash,
				DSM_FLASH_HOT_DIE_ERROR_NO);
			cam_warn("[I/DSM] %s dsm_client_notify",
				client_flash->client_name);
		}
	}
	if (val & UNDER_VOLTAGE_LOCKOUT) {
		if (!dsm_client_ocuppy(client_flash)) {
			dsm_client_record(client_flash,
				"flash UVLO FlagReg1[0x%x]\n", val);
			dsm_client_notify(client_flash,
				DSM_FLASH_UNDER_VOLTAGE_LOCKOUT_ERROR_NO);
			cam_warn("[I/DSM] %s dsm_client_notify",
				client_flash->client_name);
		}
	}
	i2c_func->i2c_read(i2c_client, REG_FLAGS2, &val);
	if (val & LED_SHORT) {
		if (!dsm_client_ocuppy(client_flash)) {
			dsm_client_record(client_flash,
				"flash short fault FlagReg2[0x%x]\n", val);
			dsm_client_notify(client_flash,
				DSM_FLASH_OPEN_SHOTR_ERROR_NO);
			cam_warn("[I/DSM] %s dsm_client_notify",
				client_flash->client_name);
		}
	}

	if (flash_ctrl->mix_pos == HWFLASH_POSITION_FORE)
		g_lm3646_pdata.front_cur = 0;
	else
		g_lm3646_pdata.back_cur = 0;

	// two torch on
	if (g_lm3646_pdata.front_cur != 0 || g_lm3646_pdata.back_cur != 0) {
		cam_info("%s two torch, close %s", __func__,
			(flash_ctrl->mix_pos == HWFLASH_POSITION_FORE) ? "fore" : "back");

		cdata.mode = TORCH_MODE;
		if (flash_ctrl->mix_pos == HWFLASH_POSITION_FORE) {
			cdata.data = g_lm3646_pdata.back_cur;
			flash_ctrl->mix_pos = HWFLASH_POSITION_REAR;
			g_lm3646_pdata.back_cur = 0;
		} else {
			cdata.data = g_lm3646_pdata.front_cur;
			flash_ctrl->mix_pos = HWFLASH_POSITION_FORE;
			g_lm3646_pdata.front_cur = 0;
		}

		hw_lm3646_set_mode(flash_ctrl, &cdata);
	} else {
		i2c_func->i2c_write(i2c_client, REG_ENABLE, MODE_STANDBY);

		flash_ctrl->state.mode = STANDBY_MODE;
		flash_ctrl->state.data = 0;
		hw_lm3646_set_pin(flash_ctrl, STROBE, LOW);
		hw_lm3646_set_pin(flash_ctrl, TORCH, LOW);
		// Enable lm3646 switch to shutdown current is 1.3ua
		hw_lm3646_set_pin(flash_ctrl, RESET, LOW);
	}

	if (pdata->need_wakelock == WAKE_LOCK_ENABLE) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		__pm_relax(&pdata->lm3646_wakelock);
#else
		__pm_relax(pdata->lm3646_wakelock);
#endif
	}
	cam_info("%s end", __func__);
	mutex_unlock(flash_ctrl->hw_flash_mutex);

	return 0;
}

static int hw_lm3646_get_dt_data(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_lm3646_mix_private_data_t *pdata = NULL;
	struct device_node *dev_node = NULL;
	int i;
	int rc = -EINVAL;

	cam_debug("%s enter", __func__);

	if (!flash_ctrl || !flash_ctrl->pdata ||
		!flash_ctrl->dev || !flash_ctrl->dev->of_node) {
		cam_err("%s invalid params", __func__);
		return rc;
	}

	pdata = (struct hw_lm3646_mix_private_data_t *)flash_ctrl->pdata;
	dev_node = flash_ctrl->dev->of_node;

	rc = of_property_read_u32_array(dev_node, "vendor,flash-pin",
		pdata->pin, (unsigned long)MAX_PIN);
	if (rc < 0) {
		cam_err("%s get flash-pin failed line %d", __func__, __LINE__);
		return rc;
	}
	for (i = 0; i < MAX_PIN; i++)
		cam_info("%s pin[%d] = %d", __func__, i, pdata->pin[i]);

	rc = of_property_read_u32(dev_node, "vendor,flash-chipid",
		&pdata->chipid);
	cam_info("%s vendor,flash-chipid 0x%x, rc %d", __func__,
		 pdata->chipid, rc);
	if (rc < 0) {
		cam_err("%s failed %d", __func__, __LINE__);
		return rc;
	}

	rc = of_property_read_u32(dev_node, "vendor,flash-ctrltype",
		&pdata->ctrltype);
	cam_info("%s vendor,ctrltype 0x%x, rc %d", __func__,
		pdata->ctrltype, rc);
	if (rc < 0) {
		cam_err("%s failed %d", __func__, __LINE__);
		return rc;
	}

	rc = of_property_read_u32(dev_node, "vendor,led-type",
		&pdata->led_type);
	cam_info("%s vendor, led-type %d, rc %d\n", __func__,
		pdata->led_type, rc);
	if (rc < 0) {
		cam_err("%s read led-type failed %d\n", __func__, __LINE__);
		return rc;
	}

	rc = of_property_read_u32(dev_node, "vendor,need-wakelock",
		(u32 *)&pdata->need_wakelock);
	cam_info("%s vendor, need-wakelock %d, rc %d\n",
		 __func__, pdata->need_wakelock, rc);
	if (rc < 0) {
		pdata->need_wakelock = WAKE_LOCK_DISABLE;
		cam_err("%s failed %d\n", __func__, __LINE__);
		return rc;
	}

	rc = of_property_read_u32_array(dev_node, "vendor,flash-current",
		pdata->ecurrent, CURRENT_MIX_MAX);
	if (rc < 0) {
		cam_err("%s read flash-current failed line %d\n",
			__func__, __LINE__);
		return rc;
	}
	for (i = 0; i < CURRENT_MIX_MAX; i++)
		cam_info("%s ecurrent[%d] = %d\n", __func__, i,
			pdata->ecurrent[i]);
	return rc;
}

static ssize_t hw_lm3646_flash_lightness_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc;

	if (!buf) {
		cam_err("%s buf is NULL", __func__);
		return -EINVAL;
	}
	rc = scnprintf(buf, PAGE_SIZE, "mode = %d, data = %d\n",
		g_lm3646_mix_ctrl.state.mode,
		g_lm3646_mix_ctrl.state.data);

	return rc;
}

static int hw_lm3646_param_check(char *buf, unsigned long *param,
	int num_of_par)
{
	char *token = NULL;
	unsigned int base;
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
				base = 16; // Hexadecimal
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

static ssize_t hw_lm3646_flash_lightness_store_imp(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct hw_flash_cfg_data cdata = {0};
	unsigned long param[MAX_LIGHTNESS_PARAM_NUM_RT] = {0};
	int rc;
	int flash_id;

	rc = hw_lm3646_param_check((char *)buf, param,
		MAX_LIGHTNESS_PARAM_NUM_RT);
	if (rc < 0) {
		cam_err("%s failed to check param", __func__);
		return rc;
	}

	flash_id = (int)param[LIGHTNESS_PARAM_FD_INDEX_RT]; // 0 - flash id
	cdata.mode = (int)param[LIGHTNESS_PARAM_MODE_INDEX_RT]; // 1 - mode
	cam_info("%s flash_id = %d,cdata.mode = %d",
		__func__, flash_id, cdata.mode);

	if (cdata.mode == STANDBY_MODE) {
		rc = hw_lm3646_off(&g_lm3646_mix_ctrl);
		if (rc < 0) {
			cam_err("%s flash off error", __func__);
			return rc;
		}
	} else if (cdata.mode == TORCH_MODE) {
		if (g_lm3646_mix_ctrl.mix_pos == HWFLASH_POSITION_FORE)
			cdata.data =
				g_lm3646_pdata.ecurrent[CURRENT_TORCH_LEVEL_RT_FRONT];
		else
			cdata.data =
				g_lm3646_pdata.ecurrent[CURRENT_TORCH_LEVEL_RT_BACK];

		cam_info("%s mode = %d, max_current = %d",
			__func__, cdata.mode, cdata.data);

		rc = hw_lm3646_on(&g_lm3646_mix_ctrl, &cdata);
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

static ssize_t hw_lm3646_flash_lightness_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	g_lm3646_mix_ctrl.mix_pos = HWFLASH_POSITION_REAR; // call back flash
	return hw_lm3646_flash_lightness_store_imp(dev, attr, buf, count);
}

static ssize_t hw_lm3646_flash_lightness_f_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	g_lm3646_mix_ctrl.mix_pos = HWFLASH_POSITION_FORE; // call front flash
	return hw_lm3646_flash_lightness_store_imp(dev, attr, buf, count);
}

static ssize_t hw_lm3646_flash_mask_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc;

	if (!buf) {
		cam_err("%s buf is NULL", __func__);
		return -EINVAL;
	}
	rc = scnprintf(buf, PAGE_SIZE, "flash_mask_disabled = %d\n",
		g_lm3646_mix_ctrl.flash_mask_enable);

	return rc;
}

static ssize_t hw_lm3646_flash_mask_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	if (!buf) {
		cam_err("%s buf is NULL", __func__);
		return -EINVAL;
	}
	if (buf[0] == '0') // char '0' for mask disable
		g_lm3646_mix_ctrl.flash_mask_enable = false;
	else
		g_lm3646_mix_ctrl.flash_mask_enable = true;

	cam_debug("%s flash_mask_enable = %d", __func__,
		g_lm3646_mix_ctrl.flash_mask_enable);
	return (ssize_t)count;
}

static void hw_lm3646_torch_brightness_set_imp(struct led_classdev *cdev,
	enum led_brightness brightness)
{
	struct hw_flash_cfg_data cdata;
	int rc;
	unsigned int led_bright = brightness;

	cam_info("%s brightness = %d", __func__, brightness);
	if (led_bright == LED_OFF) {
		rc = hw_lm3646_off(&g_lm3646_mix_ctrl);
		if (rc < 0) {
			cam_err("%s pmu_led off error", __func__);
			return;
		}
	} else {
		cdata.mode = TORCH_MODE;
		if (g_lm3646_mix_ctrl.mix_pos == HWFLASH_POSITION_FORE)
			cdata.data =
				g_lm3646_pdata.ecurrent[CURRENT_TORCH_LEVEL_MMI_FRONT];
		else
			cdata.data =
				g_lm3646_pdata.ecurrent[CURRENT_TORCH_LEVEL_MMI_BACK];

		cam_info("%s brightness = 0x%x, mode = %d, data = %d",
			__func__, brightness, cdata.mode, cdata.data);
		rc = hw_lm3646_on(&g_lm3646_mix_ctrl, &cdata);
		if (rc < 0) {
			cam_err("%s flash on error", __func__);
			return;
		}
	}
}

static void hw_lm3646_torch_brightness_set(struct led_classdev *cdev,
	enum led_brightness brightness)
{
	g_lm3646_mix_ctrl.mix_pos = HWFLASH_POSITION_REAR;
	hw_lm3646_torch_brightness_set_imp(cdev, brightness);
}

static void hw_lm3646_torch_brightness_f_set(struct led_classdev *cdev,
	enum led_brightness brightness)
{
	g_lm3646_mix_ctrl.mix_pos = HWFLASH_POSITION_FORE;
	hw_lm3646_torch_brightness_set_imp(cdev, brightness);
}

// for RT
// 660:-wr-wr---
static struct device_attribute g_lm3646_flash_lightness =
	__ATTR(flash_lightness,
	0660, hw_lm3646_flash_lightness_show,
	hw_lm3646_flash_lightness_store);

// 660:-wr-wr---
static struct device_attribute g_lm3646_flash_lightness_f =
	__ATTR(flash_lightness_f,
	0660, hw_lm3646_flash_lightness_show,
	hw_lm3646_flash_lightness_f_store);

// 660:-wr-wr---
static struct device_attribute g_lm3646_flash_mask = __ATTR(flash_mask, 0660,
	hw_lm3646_flash_mask_show, hw_lm3646_flash_mask_store);

static int hw_lm3646_register_attribute(struct hw_flash_ctrl_t *flash_ctrl,
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
	flash_ctrl->cdev_torch.brightness_set = hw_lm3646_torch_brightness_set;
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
		hw_lm3646_torch_brightness_f_set;
	rc = led_classdev_register((struct device *)dev,
		&flash_ctrl->cdev_torch1);
	if (rc < 0) {
		cam_err("%s failed to register torch_front classdev",
			__func__);
		goto err_create_torch_front_file;
	}

	rc = device_create_file(dev, &g_lm3646_flash_lightness);
	if (rc < 0) {
		cam_err("%s failed to creat flash_lightness attribute",
			__func__);
		goto err_create_flash_lightness_file;
	}

	rc = device_create_file(dev, &g_lm3646_flash_lightness_f);
	if (rc < 0) {
		cam_err("%s failed to creat flash_f_lightness attribute",
			__func__);
		goto err_create_flash_f_lightness_file;
	}

	rc = device_create_file(dev, &g_lm3646_flash_mask);
	if (rc < 0) {
		cam_err("%s failed to creat flash_mask attribute", __func__);
		goto err_create_flash_mask_file;
	}
	return 0;

err_create_flash_mask_file:
	device_remove_file(dev, &g_lm3646_flash_lightness_f);
err_create_flash_f_lightness_file:
	device_remove_file(dev, &g_lm3646_flash_lightness);
err_create_flash_lightness_file:
	led_classdev_unregister(&flash_ctrl->cdev_torch1);
err_create_torch_front_file:
	led_classdev_unregister(&flash_ctrl->cdev_torch);
err_out:
	return rc;
}

extern int register_camerafs_attr(struct device_attribute *attr);
static int hw_lm3646_match(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_flash_i2c_client *i2c_client = NULL;
	struct hw_flash_i2c_fn_t *i2c_func = NULL;
	struct hw_lm3646_mix_private_data_t *pdata = NULL;
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
	pdata = (struct hw_lm3646_mix_private_data_t *)flash_ctrl->pdata;

	// Enable lm3646 switch to standby current is 10ua
	hw_lm3646_set_pin(flash_ctrl, RESET, HIGH);
	i2c_func->i2c_read(i2c_client, REG_CHIPID, &id);
	cam_info("%s 0x%x", __func__, id);
	id = id & CHIP_ID_MASK;
	if (id != CHIP_ID) {
		cam_err("%s match error, 0x%x != 0x%x",
			__func__, id, CHIP_ID);
		return -EINVAL;
	}
	// Enable lm3646 switch to shutdown current is 1.3ua
	hw_lm3646_set_pin(flash_ctrl, RESET, LOW);

	register_camerafs_attr(&g_lm3646_flash_lightness);

	register_camerafs_attr(&g_lm3646_flash_lightness_f);
	return 0;
}

static int hw_lm3646_remove(struct i2c_client *client)
{
	cam_debug("%s enter", __func__);
	if (!client) {
		cam_err("%s client is NULL", __func__);
		return -EINVAL;
	}

	g_lm3646_mix_ctrl.func_tbl->flash_exit(&g_lm3646_mix_ctrl);

	client->adapter = NULL;
	return 0;
}

static const struct i2c_device_id g_lm3646_id[] = {
	{ "lm3646_mix", (unsigned long) &g_lm3646_mix_ctrl },
	{}
};

static const struct of_device_id g_lm3646_dt_match[] = {
	{ .compatible = "vendor,lm3646_mix" },
	{}
};
MODULE_DEVICE_TABLE(of, lm3646_dt_match);

static struct i2c_driver g_lm3646_i2c_driver = {
	.probe = hw_flash_i2c_probe,
	.remove = hw_lm3646_remove,
	.id_table = g_lm3646_id,
	.driver = {
		.name = "hw_lm3646_mix",
		.of_match_table = g_lm3646_dt_match,
	},
};

static int __init hw_lm3646_mix_module_init(void)
{
	cam_info("%s enter", __func__);
	return i2c_add_driver(&g_lm3646_i2c_driver);
}

static void __exit hw_lm3646_mix_module_exit(void)
{
	cam_info("%s enter", __func__);
	i2c_del_driver(&g_lm3646_i2c_driver);
}

static struct hw_flash_i2c_client g_lm3646_i2c_client;

static struct hw_flash_fn_t g_lm3646_func_tbl = {
	.flash_config = hw_flash_config,
	.flash_init = hw_lm3646_init,
	.flash_exit = hw_lm3646_exit,
	.flash_on = hw_lm3646_on,
	.flash_off = hw_lm3646_off,
	.flash_match = hw_lm3646_match,
	.flash_get_dt_data = hw_lm3646_get_dt_data,
	.flash_register_attribute = hw_lm3646_register_attribute,
};

struct hw_flash_ctrl_t g_lm3646_mix_ctrl = {
	.flash_i2c_client = &g_lm3646_i2c_client,
	.func_tbl = &g_lm3646_func_tbl,
	.hw_flash_mutex = &flash_mut_lm3646,
	.pdata = (void *)&g_lm3646_pdata,
	.flash_mask_enable = true,
	.state = {
		.mode = STANDBY_MODE,
	},
};

module_init(hw_lm3646_mix_module_init);
module_exit(hw_lm3646_mix_module_exit);
MODULE_DESCRIPTION("LM3646 FLASH");
MODULE_LICENSE("GPL v2");
