/*
 * lm3644tt_mix_ir.c
 *
 * driver for lm3644tt_mix_ir
 *
 * Copyright (c) 2011-2019 Huawei Technologies Co., Ltd.
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
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>

/* lm3644tt Registers define */
#define LM3644TT_SLAVE_ADDRESS 0x63
#define REG_CHIP_ID 0x0c

#define REG_MODE 0x01
/* LED1: back flash / torch; LED2: front IR flash */
#define REG_L1_FLASH 0x03
#define REG_L2_FLASH 0x04
#define REG_L1_TORCH 0x05
#define REG_L2_TORCH 0x06
#define REG_FLAGS1 0x0A
#define REG_FLAGS2 0x0B
#define REG_FLASH_TIMEOUT 0x08
#define MODE_TEMP 0x09
#define MODE_TEMP_VALUE 0x08

#define LM3644TT_CHIP_ID 0x04
#define KTD2687_CHIP_ID 0x02

#define LED2_EN (0x1 << 1)
#define LED1_EN 0x1
#define LED2_DIS 0xFD
#define LED1_DIS 0xFF

/* Enable register 0x01 (REG_MODE) */
#define MODE_STANDBY (0x00 << 2)
#define MODE_TORCH (0x02 << 2)
#define MODE_FLASH (0x03 << 2)
#define MODE_IR_DRIVER (0x01 << 2)
#define MODE_STANDBY_DISABLE 0xF3

#define STROBE_ENABLE (0x01 << 5)
#define STROBE_DISABLE 0x00

#define INDUCTOR_CURRENT_LIMMIT 0x80
#define FLASH_TIMEOUT_TIME 0x09 /* 400ms */
#define KTD2687_FLASH_TIMEOUT_TIME 0x0F /* 400ms KTD2687 MAX time-out */

#define LM3644TT_TORCH_MAX_CUR 360 /* mA */
#define LM3644TT_FLASH_MAX_CUR 1500 /* mA */
#define LM3644TT_TORCH_FRONT_CUR 350 /* mA */

#define LM3644TT_FLASH_DEFAULT_CUR_LEV 63 /* 729mA */
#define LM3644TT_TORCH_DEFAULT_CUR_LEV 63 /* 178.6mA */

#define FLASH_LED_MAX 128
#define TORCH_LED_MAX 128
#define FLASH_LED_LEVEL_INVALID 0xff

#define LM3644TT_OVER_VOLTAGE_PROTECT 0x02
#define LM3644TT_LED_OPEN_SHORT  0x70
#define LM3644TT_OVER_TEMP_PROTECT 0x04
#define LM3644TT_OVER_CURRENT_PROTECT 0x10

#define flash_current_to_level_ti(x) (((x) * 1000 - 10900) / 11725)
#define torch_current_to_level_ti(x) (((x) * 1000 - 1954) / 2800)
#define flash_current_to_level_ktd2687(x) (((32 * (x)) / 375) - 1)
#define torch_current_to_level_ktd2687(x) (((256 * (x)) / 375) - 1)
#define flash_current_to_level_aw3644(x) (((x) * 100 - 1135) / 1172)
#define torch_current_to_level_aw3644(x) (((x) * 100 - 255) / 291)

/* lm3644tt_mix_ir define */
#define TOTAL_STEPS_OF_TORCH 8
#define TOTAL_STEPS_OF_FLASH 16
#define INVALID_GPIO 999
#define LM3644TT_STANDBY_MODE_CUR 0
#define WAKE_LOCK_ENABLE 1
#define WAKE_LOCK_DISABLE 0
#define UNDER_VOLTAGE_LOCKOUT 0x04
#define COUNT_BASE 100
#define LM3644TT_LED1_BASE_CUR_OF_FLASH 2304 /* 100ma */
#define LM3644TT_LED1_STEP_CUR_OF_FLASH 1172 /* 100ma */
#define LM3644TT_LED1_BASE_CUR_OF_TORCH 253  /* 100ma */
#define LM3644TT_LED1_STEP_CUR_OF_TORCH 146  /* 100ma */
#define LM3644TT_STANDBY_MODE_CUR 0
#define REG_MASK_0XFF 0xFF

/* 1 means I2C control, 0 means GPIO control */
#define LM3644TT_CTRL_TYPE_STROBE 0
#define LM3644TT_CTRL_TYPE_I2C 1
#define TX_ENABLE 0x08
#define ENABLE_SHORT_PROTECT 0x80
#define LM3644TT_FLASH_LED_VAL_MASK 0
#define LM3644TT_TORCH_LED_VAL_MASK 0
#define BACK_FLASH_USE_LED2 1
#define LM3644TT_TORCH_MAX_VAL_SHIFT 4
#define MAX_LIGHTNESS_PARAM_NUM_RT 3
#define LIGHTNESS_PARAM_FD_INDEX_RT 0
#define LIGHTNESS_PARAM_MODE_INDEX_RT 1
#define LIGHTNESS_PARAM_CURRENT_INDEX_RT 2
#define MAX_FLASH_ID 0xFF /* used to check flash_lightness_sotre */
#define MAX_BRIGHTNESS_FORMMI 0x09

enum lm3644tt_mix_ir_pin_type_t {
	STROBE = 0,
	FLASH_EN,
	TORCH,
	MAX_PIN,
};

enum lm3644tt_mix_ir_pin_status_t {
	FREED = 0,
	REQUESTED,
};

struct lm3644tt_mix_ir_pin_t {
	unsigned int pin_id;
	enum lm3644tt_mix_ir_pin_status_t pin_status;
};

enum lm3644tt_mix_ir_current_conf {
	CURRENT_TORCH_LEVEL_MMI_BACK = 0,
	CURRENT_TORCH_LEVEL_MMI_FRONT = 1,
	CURRENT_TORCH_LEVEL_RT_BACK = 2,
	CURRENT_TORCH_LEVEL_RT_FRONT = 3,
	CURRENT_MIX_MAX = 4,
};

/* flash mode for single front or back */
enum lm3644tt_mix_ir_single_mode_t {
	SINGLE_STANDBY_MODE = 0x00,
	SINGLE_FLASH_MODE = 0x01,
	SINGLE_TORCH_MODE = 0x02,
	SINGLE_IR_MODE = 0x03,
};

/* flash mode for entire IC which depends on */
/* current flash mode of single front and back */
enum lm3644tt_mix_ir_entire_ic_mode_t {
	ENTIRE_IC_STANDBY_MODE = 0,
	ENTIRE_IC_FLASH_MODE = 1,
	ENTIRE_IC_TORCH_MODE = 2,
	ENTIRE_IC_IR_MODE = 2,
};

/* ir flash RT self test param */
enum lm3644tt_mix_ir_selftest_param {
	IR_SELFTEST_ON_TIME_INDEX = 0,
	IR_SELFTEST_OFF_TIME_INDEX = 1,
	IR_SELFTEST_CURRENT_INDEX = 2,
	IR_SELFTEST_TIMES_INDEX = 3,
	IR_SELFTEST_MAX_INDEX = 4,
};

enum lm3644tt_mix_ir_chip_type_t {
	TYPE_MIX,
	LM3644TT, /* LM3644TT is 1 */
	AW3644,
	KTD2687,
	TYPE_MAX
};

/* Internal data struct define */
struct hw_lm3644tt_mix_ir_private_data_t {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
	struct wakeup_source lm3644tt_mix_ir_wakelock;
#else
	struct wakeup_source *lm3644tt_mix_ir_wakelock;
#endif
	unsigned int need_wakelock;
	/* flash control pin */
	struct lm3644tt_mix_ir_pin_t pin[MAX_PIN];
	/* flash electric current config */
	unsigned int ecurrent[CURRENT_MIX_MAX];
	unsigned int selftest_param[IR_SELFTEST_MAX_INDEX];
	unsigned int chipid;
	unsigned int ctrltype;
	unsigned int led_type;
	unsigned int front_cur;
	unsigned int back_cur;
	enum lm3644tt_mix_ir_single_mode_t front_mode;
	enum lm3644tt_mix_ir_single_mode_t back_mode;
	enum lm3644tt_mix_ir_entire_ic_mode_t entire_ic_mode;
	enum lm3644tt_mix_ir_chip_type_t chip_type;
};

/* Internal varible define */
static struct hw_lm3644tt_mix_ir_private_data_t g_lm3644tt_mix_ir_pdata;
struct hw_flash_ctrl_t g_lm3644tt_mix_ir_ctrl;
extern struct dsm_client *client_flash;

define_kernel_flash_mutex(lm3644tt_mix_ir);

/* Function define */
static int hw_lm3644tt_mix_ir_param_check(char *buf, unsigned long *param,
	int num_of_par);
static int hw_lm3644tt_mix_ir_set_pin(struct hw_flash_ctrl_t   *flash_ctrl,
	enum lm3644tt_mix_ir_pin_type_t  pin_type, int state)
{
	struct hw_lm3644tt_mix_ir_private_data_t *pdata = NULL;
	int rc = 0;

	if (!flash_ctrl || !flash_ctrl->pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}
	if (pin_type < STROBE || pin_type >= MAX_PIN) {
		cam_err("%s error", __func__);
		return -EINVAL;
	}

	pdata = (struct hw_lm3644tt_mix_ir_private_data_t *)flash_ctrl->pdata;
	cam_debug("%s pin-type=%d, state=%d", __func__, pin_type, state);

	if ((pdata->pin[pin_type].pin_id != INVALID_GPIO)
		&& (pdata->pin[pin_type].pin_status == REQUESTED)) {
		rc = gpio_direction_output(pdata->pin[pin_type].pin_id, state);
		if (rc < 0)
			cam_err("%s gpio output is err rc=%d", __func__, rc);
	}

	return rc;
}

static int gpio_request_assemble(struct hw_lm3644tt_mix_ir_private_data_t *pdata)
{
	int i;
	int rc = 0;
	const char *gpio_name[MAX_PIN] = { "flash-strobe", "flash-en",
		"flash-torch" };

	if (!pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	for (i = STROBE; i < MAX_PIN; i++) {
		if ((pdata->pin[i].pin_id != INVALID_GPIO) &&
			(pdata->pin[i].pin_status == FREED)) {
			rc = gpio_request(pdata->pin[i].pin_id, gpio_name[i]);
			if (rc < 0) {
				cam_err("%s failed to request pin %s",
					__func__, gpio_name[i]);
				break;
			}
			pdata->pin[i].pin_status = REQUESTED;
		}
	}

	return rc;
}

static int gpio_free_assemble(struct hw_lm3644tt_mix_ir_private_data_t *pdata)
{
	int i;

	if (!pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	for (i = STROBE; i < MAX_PIN; i++) {
		if ((pdata->pin[i].pin_id != INVALID_GPIO) &&
			(pdata->pin[i].pin_status == REQUESTED)) {
			gpio_free(pdata->pin[i].pin_id);
			pdata->pin[i].pin_status = FREED;
		}
	}

	return 0;
}

static int hw_lm3644tt_mix_ir_init(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_lm3644tt_mix_ir_private_data_t *pdata = NULL;
	int rc;

	if (!flash_ctrl || !flash_ctrl->pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	/* init data */
	flash_ctrl->flash_type = FLASH_MIX; /* mix flash */
	flash_ctrl->pctrl = devm_pinctrl_get_select(flash_ctrl->dev,
		PINCTRL_STATE_DEFAULT);
	if (!flash_ctrl->pctrl) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	/* stub:check other params which need to init */
	pdata = (struct hw_lm3644tt_mix_ir_private_data_t *)flash_ctrl->pdata;
	pdata->front_cur = LM3644TT_STANDBY_MODE_CUR; /* init current as 0 */
	pdata->back_cur = LM3644TT_STANDBY_MODE_CUR; /* init current as 0 */
	pdata->front_mode = SINGLE_STANDBY_MODE;
	pdata->back_mode = SINGLE_STANDBY_MODE;
	pdata->entire_ic_mode = ENTIRE_IC_STANDBY_MODE;

	mutex_lock(flash_ctrl->hw_flash_mutex);
	flash_ctrl->state.mode = STANDBY_MODE;
	/* set init value 0 */
	flash_ctrl->state.data = LM3644TT_STANDBY_MODE_CUR;
	mutex_unlock(flash_ctrl->hw_flash_mutex);

	rc = gpio_request_assemble(pdata);
	if (rc < 0) {
		cam_err("%s failde to request gpio", __func__);
		goto gpio_fail_handler;
	}

	rc = hw_lm3644tt_mix_ir_set_pin(flash_ctrl, FLASH_EN, LOW);
	if (rc < 0) {
		cam_err("%s failed to set flash_en pin", __func__);
		goto gpio_fail_handler;
	}

	if (pdata->need_wakelock == WAKE_LOCK_ENABLE) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		wakeup_source_init(&pdata->lm3644tt_mix_ir_wakelock, "lm3644tt_mix_ir");
#else
		pdata->lm3644tt_mix_ir_wakelock = wakeup_source_register(flash_ctrl->dev, "lm3644tt_mix_ir");
		if (!pdata->lm3644tt_mix_ir_wakelock) {
			rc = -EINVAL;
			cam_err("%s: wakeup source register failed", __func__);
			goto gpio_fail_handler;
		}
#endif
    }
	return rc;

gpio_fail_handler:
	gpio_free_assemble(pdata);
	return rc;
}

static int hw_lm3644tt_mix_ir_exit(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_lm3644tt_mix_ir_private_data_t *pdata = NULL;

	cam_debug("%s enter", __func__);

	if (!flash_ctrl || !flash_ctrl->pdata || !flash_ctrl->func_tbl ||
		!flash_ctrl->func_tbl->flash_off) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	flash_ctrl->func_tbl->flash_off(flash_ctrl);
	pdata = (struct hw_lm3644tt_mix_ir_private_data_t *)flash_ctrl->pdata;
	gpio_free_assemble(pdata);

	flash_ctrl->pctrl = devm_pinctrl_get_select(flash_ctrl->dev,
		PINCTRL_STATE_IDLE);

	return 0;
}

static void report_dsm(void)
{
	if (!dsm_client_ocuppy(client_flash)) {
		dsm_client_record(client_flash, "flash i2c transfer fail\n");
		dsm_client_notify(client_flash, DSM_FLASH_I2C_ERROR_NO);
		cam_warn("[I/DSM] %s dsm_client_notify",
			client_flash->client_name);
	}
}

static void check_fault_and_report_dsm(unsigned char fault_val)
{
	if (fault_val & (LM3644TT_OVER_VOLTAGE_PROTECT |
		LM3644TT_OVER_CURRENT_PROTECT)) {
		if (!dsm_client_ocuppy(client_flash)) {
			dsm_client_record(client_flash,
				"flash OVP or OCP FlagReg1[0x%x]\n",
				fault_val);
			dsm_client_notify(client_flash,
				DSM_FLASH_OPEN_SHOTR_ERROR_NO);
			cam_warn("[I/DSM] %s dsm_client_notify",
				client_flash->client_name);
		}
	}
	if (fault_val & LM3644TT_OVER_TEMP_PROTECT) {
		if (!dsm_client_ocuppy(client_flash)) {
			dsm_client_record(client_flash,
				"flash temperature is too hot FlagReg1[0x%x]\n",
				fault_val);
			dsm_client_notify(client_flash,
				DSM_FLASH_HOT_DIE_ERROR_NO);
			cam_warn("[I/DSM] %s dsm_client_notify",
				client_flash->client_name);
		}
	}
	if (fault_val & UNDER_VOLTAGE_LOCKOUT) {
		if (!dsm_client_ocuppy(client_flash)) {
			dsm_client_record(client_flash,
				"flash UVLO FlagReg1[0x%x]\n",
				fault_val);
			dsm_client_notify(client_flash,
				DSM_FLASH_UNDER_VOLTAGE_LOCKOUT_ERROR_NO);
			cam_warn("[I/DSM] %s dsm_client_notify",
				client_flash->client_name);
		}
	}
}

static void hw_lm3644tt_enter_mode(struct hw_lm3644tt_mix_ir_private_data_t *pdata,
	struct hw_flash_cfg_data *cdata, unsigned char *reg_mode,
	unsigned int front_cur)
{
	/* strobe mode enter */
	if ((cdata->mode == FLASH_STROBE_MODE) && /* flash strobe mode */
		/* DTS config the stobe pin */
		(pdata->ctrltype == LM3644TT_CTRL_TYPE_STROBE) &&
		/* is not using front flash */
		(front_cur == LM3644TT_STANDBY_MODE_CUR))
		*reg_mode = (*reg_mode | STROBE_ENABLE) & MODE_STANDBY_DISABLE;
	else /* I2C mode */
		*reg_mode |= STROBE_DISABLE;
}

static int hw_lm3644tt_mix_ir_flash_mode_cfg_calculate(unsigned char *led1_val,
	unsigned char *led2_val, unsigned char *reg_mode,
	struct hw_lm3644tt_mix_ir_private_data_t *pdata,
	struct hw_flash_cfg_data *cdata)
{
	unsigned int led1_cur = 0;
	unsigned int led2_cur = 0;
	unsigned int front_cur = 0;
	unsigned int back_cur = 0;
	unsigned int led1_cur_calc = 0;
	unsigned int led2_cur_calc = 0;

	if (!led1_val || !reg_mode || !pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	front_cur = pdata->front_cur;
	back_cur = pdata->back_cur;

	if (front_cur == FLASH_LED_LEVEL_INVALID)
		front_cur = LM3644TT_FLASH_DEFAULT_CUR_LEV;
	if (back_cur == FLASH_LED_LEVEL_INVALID)
		back_cur = LM3644TT_FLASH_DEFAULT_CUR_LEV;

	/* driver ic: LM3644TT: 1, AW3644: 2, KTD2687: 3 */
	cam_info("%s led_type = %d, flash driver ic is %u", __func__,
		pdata->led_type, g_lm3644tt_mix_ir_pdata.chip_type);
	if (pdata->led_type == BACK_FLASH_USE_LED2) {
		led1_cur = front_cur;
		led2_cur = back_cur;
	} else {
		led1_cur = back_cur;
		led2_cur = front_cur;
	}

	switch (g_lm3644tt_mix_ir_pdata.chip_type) {
	case KTD2687:
		led1_cur_calc = flash_current_to_level_ktd2687(led1_cur);
		led2_cur_calc = flash_current_to_level_ktd2687(led2_cur);
		break;
	case LM3644TT:
		led1_cur_calc = flash_current_to_level_ti(led1_cur);
		led2_cur_calc = flash_current_to_level_ti(led2_cur);
		break;
	case AW3644:
		led1_cur_calc = flash_current_to_level_aw3644(led1_cur);
		led2_cur_calc = flash_current_to_level_aw3644(led2_cur);
		break;
	default:
		led1_cur_calc = flash_current_to_level_ti(led1_cur);
		led2_cur_calc = flash_current_to_level_ti(led2_cur);
		break;
	}

	if ((led1_cur != LM3644TT_STANDBY_MODE_CUR) &&
		(led2_cur == LM3644TT_STANDBY_MODE_CUR)) {
		/* using LED1 only */
		/* LED1 = flash1 current value ; disable LED2 reg bit */
		*reg_mode = ((LED1_EN | MODE_FLASH) & LED2_DIS);
		*led1_val = led1_cur_calc;
		*led2_val = LM3644TT_FLASH_LED_VAL_MASK;
	} else if ((led1_cur == LM3644TT_STANDBY_MODE_CUR) &&
		(led2_cur != LM3644TT_STANDBY_MODE_CUR)) {
		/* using LED2 only*/
		/* LED2 = falsh2 current value; disable LED1 reg bit */
		*reg_mode = ((LED2_EN | MODE_FLASH) & LED1_DIS);
		*led1_val = LM3644TT_FLASH_LED_VAL_MASK;
		*led2_val = led2_cur_calc;
	} else if ((led1_cur != LM3644TT_STANDBY_MODE_CUR) &&
		(led2_cur != LM3644TT_STANDBY_MODE_CUR)) {
		/* using LED1 & LED2 */
		/* LED1 & LED2 = enable LED1 & LED2 reg bit, */
		/* set two flashs current value */
		*reg_mode = (LED1_EN | LED2_EN | MODE_FLASH);
		*led1_val = led1_cur_calc;
		*led2_val = led2_cur_calc;
	} else {
		cam_err("%s flash current error LED1=%d LED2=%d",
			__func__, led1_cur, led2_cur);
		return -EINVAL;
	}
	hw_lm3644tt_enter_mode(pdata, cdata, reg_mode, front_cur);

	return 0;
}

static int hw_lm3644tt_mix_ir_torch_mode_cfg_calculate(unsigned char *led1_val,
	unsigned char *led2_val,
	unsigned char *reg_mode,
	struct hw_lm3644tt_mix_ir_private_data_t *pdata)
{
	unsigned int led1_cur = 0;
	unsigned int led2_cur = 0;
	unsigned int front_cur = 0;
	unsigned int back_cur = 0;
	unsigned int led1_cur_calc = 0;
	unsigned int led2_cur_calc = 0;

	if (!led1_val || !led2_val || !reg_mode || !pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	front_cur = pdata->front_cur;
	back_cur  = pdata->back_cur;

	if (front_cur == FLASH_LED_LEVEL_INVALID)
		front_cur = LM3644TT_TORCH_DEFAULT_CUR_LEV;
	if (back_cur == FLASH_LED_LEVEL_INVALID)
		back_cur = LM3644TT_TORCH_DEFAULT_CUR_LEV;

	/* driver ic: LM3644TT: 1, AW3644: 2, KTD2687: 3 */
	cam_info("%s led_type = %d, flash driver ic is %u", __func__,
		pdata->led_type, g_lm3644tt_mix_ir_pdata.chip_type);
	/* check hardware menu and check DTS */
	if (pdata->led_type == BACK_FLASH_USE_LED2) {
		led1_cur = front_cur;
		led2_cur = back_cur;
	} else {
		led1_cur = back_cur;
		led2_cur = front_cur;
	}

	switch (g_lm3644tt_mix_ir_pdata.chip_type) {
	case KTD2687:
		led1_cur_calc =
			torch_current_to_level_ktd2687(led1_cur);
		led2_cur_calc =
			torch_current_to_level_ktd2687(led2_cur);
		break;
	case LM3644TT:
		led1_cur_calc = torch_current_to_level_ti(led1_cur);
		led2_cur_calc = torch_current_to_level_ti(led2_cur);
		break;
	case AW3644:
		led1_cur_calc =
			torch_current_to_level_aw3644(led1_cur);
		led2_cur_calc =
			torch_current_to_level_aw3644(led2_cur);
		break;
	default:
		led1_cur_calc = torch_current_to_level_ti(led1_cur);
		led2_cur_calc = torch_current_to_level_ti(led2_cur);
		break;
	}

	if ((led1_cur != LM3644TT_STANDBY_MODE_CUR) &&
		(led2_cur == LM3644TT_STANDBY_MODE_CUR)) {
		/* using LED1 only */
		/* LED1 = torch1 current Value ; disable LED2 reg bit */
		*reg_mode = ((LED1_EN | MODE_TORCH) & LED2_DIS);
		*led1_val = led1_cur_calc;
		*led2_val = LM3644TT_TORCH_LED_VAL_MASK;
	} else if ((led1_cur == LM3644TT_STANDBY_MODE_CUR) &&
		(led2_cur != LM3644TT_STANDBY_MODE_CUR)) {
		/* using LED2 only*/
		/* LED2 = torch2 current Value; disable LED1 reg bit */
		*reg_mode = ((LED2_EN | MODE_TORCH) & LED1_DIS);
		*led1_val = LM3644TT_TORCH_LED_VAL_MASK;
		*led2_val = led2_cur_calc;
	} else if ((led1_cur != LM3644TT_STANDBY_MODE_CUR) &&
		(led2_cur != LM3644TT_STANDBY_MODE_CUR)) {
		/* using LED1 & LED2 */
		/* LED1 & LED2 = enable LED1 & LED2 reg bit */
		/* set two torchs current Value */
		*reg_mode = (LED1_EN | LED2_EN | MODE_TORCH);
		*led1_val = led1_cur_calc;
		*led2_val = led2_cur_calc;
	} else {
		cam_err("%s flash current error LED1=%d LED2=%d",
			__func__, led1_cur, led2_cur);
		return -EINVAL;
	}
	/* touch I2C mode */
	*reg_mode |= STROBE_DISABLE;

	return 0;
}

static int hw_lm3644tt_mix_ir_flash_mode_regs_cfg(struct hw_flash_ctrl_t *flash_ctrl,
	unsigned char led1_val,
	unsigned char led2_val,
	unsigned char reg_mode)
{
	int rc = 0;
	unsigned char reg_fault_clean = 0;
	struct hw_flash_i2c_fn_t *i2c_func = NULL;
	struct hw_flash_i2c_client *i2c_client = NULL;
	int flash_time = 0;

	if (!flash_ctrl || !flash_ctrl->flash_i2c_client ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_read ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_write) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	i2c_client = flash_ctrl->flash_i2c_client;
	i2c_func = i2c_client->i2c_func_tbl;

	/* clear error status */
	i2c_func->i2c_read(i2c_client, REG_FLAGS1, &reg_fault_clean);
	check_fault_and_report_dsm(reg_fault_clean);
	i2c_func->i2c_read(i2c_client, REG_FLAGS2, &reg_fault_clean);

	switch (g_lm3644tt_mix_ir_pdata.chip_type) {
	case KTD2687:
		flash_time = KTD2687_FLASH_TIMEOUT_TIME;
		break;
	case LM3644TT:
	case AW3644:
		flash_time = FLASH_TIMEOUT_TIME;
		break;
	default:
		flash_time = FLASH_TIMEOUT_TIME;
		break;
	}
	rc += i2c_func->i2c_write(i2c_client, REG_FLASH_TIMEOUT, flash_time);
	rc += i2c_func->i2c_write(i2c_client, REG_L1_FLASH, led1_val);
	rc += i2c_func->i2c_write(i2c_client, REG_L2_FLASH, led2_val);
	rc += i2c_func->i2c_write(i2c_client, REG_MODE, reg_mode);

	cam_info("%s config flash1_val = 0x%02x, flash2_val = 0x%02x, reg_mode = 0x%02x",
		__func__, led1_val, led2_val, reg_mode);

	if (rc < 0)
		report_dsm();

	return rc;
}

static int hw_lm3644tt_mix_ir_torch_mode_regs_cfg(struct hw_flash_ctrl_t *flash_ctrl,
	unsigned char led1_val,
	unsigned char led2_val,
	unsigned char reg_mode)
{
	int rc = 0;
	unsigned char reg_fault_clean = 0;
	struct hw_flash_i2c_fn_t *i2c_func = NULL;
	struct hw_flash_i2c_client *i2c_client = NULL;

	if (!flash_ctrl || !flash_ctrl->flash_i2c_client ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_read ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_write) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	i2c_client = flash_ctrl->flash_i2c_client;
	i2c_func = i2c_client->i2c_func_tbl;

	/* clear error status */
	i2c_func->i2c_read(i2c_client, REG_FLAGS1, &reg_fault_clean);
	check_fault_and_report_dsm(reg_fault_clean);
	i2c_func->i2c_read(i2c_client, REG_FLAGS2, &reg_fault_clean);

	rc += i2c_func->i2c_write(i2c_client, REG_L1_TORCH, led1_val);
	rc += i2c_func->i2c_write(i2c_client, REG_L2_TORCH, led2_val);
	rc += i2c_func->i2c_write(i2c_client, REG_MODE, reg_mode);

	cam_info("%s config torch1_val = 0x%02x, torch2_val = 0x%02x, reg_mode = 0x%02x",
		__func__, led1_val, led2_val, reg_mode);

	if (rc < 0)
		report_dsm();

	return rc;
}

static int hw_lm3644tt_mix_ir_action_ic_standby(struct hw_flash_ctrl_t *flash_ctrl)
{
	int rc = 0;
	unsigned char reg_fault_clean = 0;
	struct hw_flash_i2c_fn_t *i2c_func = NULL;
	struct hw_flash_i2c_client *i2c_client = NULL;
	struct hw_lm3644tt_mix_ir_private_data_t *pdata = NULL;

	if (!flash_ctrl || !flash_ctrl->pdata ||
		!flash_ctrl->flash_i2c_client ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_read ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_write) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	pdata = (struct hw_lm3644tt_mix_ir_private_data_t *)flash_ctrl->pdata;
	i2c_client = flash_ctrl->flash_i2c_client;
	i2c_func = i2c_client->i2c_func_tbl;

	/* read back error status */
	i2c_func->i2c_read(i2c_client, REG_FLAGS1, &reg_fault_clean);
	check_fault_and_report_dsm(reg_fault_clean);
	i2c_func->i2c_read(i2c_client, REG_FLAGS2, &reg_fault_clean);

	rc += i2c_func->i2c_write(i2c_client, REG_MODE, MODE_STANDBY);
	if (rc < 0)
		report_dsm();

	rc += hw_lm3644tt_mix_ir_set_pin(flash_ctrl, STROBE, LOW);
	rc += hw_lm3644tt_mix_ir_set_pin(flash_ctrl, TORCH, LOW);
	rc += hw_lm3644tt_mix_ir_set_pin(flash_ctrl, FLASH_EN, LOW);

	if (pdata->need_wakelock == WAKE_LOCK_ENABLE) {
		/*
		 * wake_unlock() will check
		 * whether the wakeup source is active before
		 * doing the unlock operation,
		 * so there is no dangers for the mutex being
		 * unlocked before locked.
		 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		__pm_relax(&pdata->lm3644tt_mix_ir_wakelock);
#else
		__pm_relax(pdata->lm3644tt_mix_ir_wakelock);
#endif
	}
	return rc;
}

static int hw_lm3644tt_mix_ir_action_ic_flash_mode(struct hw_flash_ctrl_t *flash_ctrl,
	struct hw_flash_cfg_data *cdata)
{
	int rc;
	unsigned char reg_mode = 0;
	unsigned char led1_val = 0;
	unsigned char led2_val = 0;
	struct hw_lm3644tt_mix_ir_private_data_t *pdata = NULL;

	if (!flash_ctrl || !flash_ctrl->pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	pdata = (struct hw_lm3644tt_mix_ir_private_data_t *)flash_ctrl->pdata;

	hw_lm3644tt_mix_ir_set_pin(flash_ctrl, FLASH_EN, HIGH);
	hw_lm3644tt_mix_ir_set_pin(flash_ctrl, STROBE, LOW);
	hw_lm3644tt_mix_ir_set_pin(flash_ctrl, TORCH, LOW);

	rc = hw_lm3644tt_mix_ir_flash_mode_cfg_calculate(&led1_val, &led2_val,
		&reg_mode, pdata, cdata);
	if (rc < 0) {
		cam_err("%s flash mode cfg calculate faild", __func__);
		return rc;
	}

	rc = hw_lm3644tt_mix_ir_flash_mode_regs_cfg(flash_ctrl, led1_val,
		led2_val, reg_mode);
	if (rc < 0) {
		cam_err("%s flash mode regs cfg faild", __func__);
		return rc;
	}

	return rc;
}

static int hw_lm3644tt_mix_ir_action_ic_torch_mode(struct hw_flash_ctrl_t *flash_ctrl)
{
	int rc;
	unsigned char led1_val = 0;
	unsigned char led2_val = 0;
	unsigned char reg_mode = 0;
	struct hw_lm3644tt_mix_ir_private_data_t *pdata = NULL;

	if (!flash_ctrl || !flash_ctrl->pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	pdata = (struct hw_lm3644tt_mix_ir_private_data_t *)flash_ctrl->pdata;

	hw_lm3644tt_mix_ir_set_pin(flash_ctrl, FLASH_EN,  HIGH);
	hw_lm3644tt_mix_ir_set_pin(flash_ctrl, STROBE, LOW);
	hw_lm3644tt_mix_ir_set_pin(flash_ctrl, TORCH,  LOW);

	rc = hw_lm3644tt_mix_ir_torch_mode_cfg_calculate(&led1_val, &led2_val,
		&reg_mode, pdata);
	if (rc < 0) {
		cam_err("%s torch mode cfg calculate faild", __func__);
		return rc;
	}

	rc = hw_lm3644tt_mix_ir_torch_mode_regs_cfg(flash_ctrl, led1_val,
		led2_val, reg_mode);
	if (rc < 0) {
		cam_err("%s torch mode reg cfg faild", __func__);
		return rc;
	}

	hw_lm3644tt_mix_ir_set_pin(flash_ctrl, TORCH,  HIGH);

	return rc;
}

static int hw_lm3644tt_mix_ir_update_inner_status(struct hw_flash_ctrl_t *flash_ctrl,
	struct hw_flash_cfg_data *cdata)
{
	struct hw_lm3644tt_mix_ir_private_data_t *pdata = NULL;
	enum lm3644tt_mix_ir_single_mode_t inner_mode = SINGLE_STANDBY_MODE;
	unsigned int cur = 0;

	if (!flash_ctrl || !cdata || !flash_ctrl->pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	pdata = (struct hw_lm3644tt_mix_ir_private_data_t *)flash_ctrl->pdata;
	cur = (unsigned int)(cdata->data);

	switch (cdata->mode) {
	case STANDBY_MODE:
		inner_mode = SINGLE_STANDBY_MODE;
		/* flash_ctrl->state will be update */
		/* when ic standby mode cfg is finished */
		break;
	case FLASH_MODE:
	case FLASH_STROBE_MODE:
		inner_mode = SINGLE_FLASH_MODE;
		/* flash_ctrl->state will be update here */
		/* and may be used in thermal protect */
		flash_ctrl->state.mode = cdata->mode;
		flash_ctrl->state.data = cdata->data;
		break;
	default:
		inner_mode = SINGLE_TORCH_MODE; /* set as default */
		/* flash_ctrl->state will be update here */
		/* and may be used in thermal protect */
		flash_ctrl->state.mode = cdata->mode;
		flash_ctrl->state.data = cdata->data;
		break;
	}

	if (flash_ctrl->mix_pos == HWFLASH_POSITION_FORE) {
		pdata->front_mode = inner_mode;
		pdata->front_cur  = cur;
		cam_info("%s update front mode:%d, front cur %u", __func__,
			pdata->front_mode, pdata->front_cur);
	} else {
		pdata->back_mode  = inner_mode;
		pdata->back_cur   = cur;
		cam_info("%s update back mode:%d, back cur %u",  __func__,
			pdata->back_mode, pdata->back_cur);
	}

	return 0;
}

static int hw_lm3644tt_mix_ir_get_work_mode_strategy(struct hw_flash_ctrl_t *flash_ctrl,
	enum lm3644tt_mix_ir_entire_ic_mode_t *ic_work_mode)
{
	struct hw_lm3644tt_mix_ir_private_data_t *pdata = NULL;
	unsigned int front_mode = SINGLE_STANDBY_MODE;
	unsigned int back_mode  = SINGLE_STANDBY_MODE;

	if (!flash_ctrl || !ic_work_mode || !flash_ctrl->pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	pdata = (struct hw_lm3644tt_mix_ir_private_data_t *)flash_ctrl->pdata;
	front_mode = pdata->front_mode;
	back_mode  = pdata->back_mode;

	*ic_work_mode = ENTIRE_IC_STANDBY_MODE; /* set as default */

	if ((front_mode | back_mode) == SINGLE_STANDBY_MODE)
		*ic_work_mode = ENTIRE_IC_STANDBY_MODE;
	else if (((front_mode | back_mode) & SINGLE_FLASH_MODE) == SINGLE_FLASH_MODE)
		*ic_work_mode = ENTIRE_IC_FLASH_MODE;
	else if (((front_mode | back_mode) & SINGLE_TORCH_MODE) == SINGLE_TORCH_MODE)
		*ic_work_mode = ENTIRE_IC_TORCH_MODE;

	if (g_lm3644tt_mix_ir_pdata.chip_type == LM3644TT)
		if (((front_mode & SINGLE_FLASH_MODE) == SINGLE_FLASH_MODE) &&
			((back_mode & SINGLE_TORCH_MODE) == SINGLE_TORCH_MODE)) {
			/*
			 * this part is only for lm3644tt,
			 * when this type flash-light run faceid task,
			 * the front mode and back mode turn to torch mode,
			 * and front current is 350mA,
			 * back current is still 120mA.
			 */
			cam_info("%s LM3644TT back mode is troch mode, front flash mode need to change to front torch mode",
				__func__);
			*ic_work_mode = ENTIRE_IC_TORCH_MODE;
			pdata->front_cur = LM3644TT_TORCH_FRONT_CUR;
		}

	if (pdata->front_cur == LM3644TT_STANDBY_MODE_CUR
		&& pdata->back_cur == LM3644TT_STANDBY_MODE_CUR)
		*ic_work_mode = ENTIRE_IC_STANDBY_MODE;

	return 0;
}

static int hw_lm3644tt_mix_ir_update_work_mode_dispatch(struct hw_flash_ctrl_t *flash_ctrl,
	enum lm3644tt_mix_ir_entire_ic_mode_t ic_work_mode,
	struct hw_flash_cfg_data *cdata)
{
	int rc = 0;
	struct hw_lm3644tt_mix_ir_private_data_t *pdata = NULL;

	if (!flash_ctrl || !flash_ctrl->pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	pdata = (struct hw_lm3644tt_mix_ir_private_data_t *)flash_ctrl->pdata;

	switch (ic_work_mode) {
	case ENTIRE_IC_STANDBY_MODE:
		if (pdata->entire_ic_mode != ENTIRE_IC_STANDBY_MODE) {
			/* only when current mode is not in standby, */
			/* action the standby mode */
			rc = hw_lm3644tt_mix_ir_action_ic_standby(flash_ctrl);
			pdata->entire_ic_mode = ENTIRE_IC_STANDBY_MODE;
			flash_ctrl->state.mode = STANDBY_MODE;
			flash_ctrl->state.data =
				LM3644TT_STANDBY_MODE_CUR;
		}
		break;
	case ENTIRE_IC_FLASH_MODE:
		rc = hw_lm3644tt_mix_ir_action_ic_flash_mode(flash_ctrl,
			cdata);
		pdata->entire_ic_mode = ENTIRE_IC_FLASH_MODE;
		break;
	case ENTIRE_IC_TORCH_MODE:
		rc = hw_lm3644tt_mix_ir_action_ic_torch_mode(flash_ctrl);
		pdata->entire_ic_mode = ENTIRE_IC_TORCH_MODE;
		break;
	default:
		cam_err("%s invalid ic work mode", __func__);
		rc = -EINVAL;
		break;
	}

	return rc;
}

static int hw_lm3644tt_mix_ir_update_work_mode(struct hw_flash_ctrl_t *flash_ctrl,
	struct hw_flash_cfg_data *cdata)
{
	int rc;
	enum lm3644tt_mix_ir_entire_ic_mode_t ic_work_mode = ENTIRE_IC_STANDBY_MODE;

	if (!flash_ctrl || !cdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	rc = hw_lm3644tt_mix_ir_update_inner_status(flash_ctrl, cdata);
	if (rc < 0) {
		cam_err("%s update inner status failed", __func__);
		goto work_mode_err;
	}

	rc = hw_lm3644tt_mix_ir_get_work_mode_strategy(flash_ctrl,
		&ic_work_mode);
	if (rc < 0) {
		cam_err("%s get work mode strategy failed", __func__);
		goto work_mode_err;
	}

	rc = hw_lm3644tt_mix_ir_update_work_mode_dispatch(flash_ctrl,
		ic_work_mode, cdata);
	if (rc < 0) {
		cam_err("%s update work mode dispatch failed", __func__);
		goto work_mode_err;
	}

	return rc;

work_mode_err:
	/* error handler may need to add here */
	return rc;
}

static int hw_lm3644tt_mix_ir_on(struct hw_flash_ctrl_t *flash_ctrl, void *data)
{
	struct hw_lm3644tt_mix_ir_private_data_t *pdata = NULL;
	struct hw_flash_cfg_data *cdata = NULL;
	int rc = 0;

	if (!data || !flash_ctrl || !flash_ctrl->pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	cdata = (struct hw_flash_cfg_data *)data;
	pdata = (struct hw_lm3644tt_mix_ir_private_data_t *)flash_ctrl->pdata;

	cam_info("%s mode=%d, cur=%d", __func__, cdata->mode, cdata->data);

	mutex_lock(flash_ctrl->hw_flash_mutex);

	if (pdata->need_wakelock == WAKE_LOCK_ENABLE) {
		/*
		 * if the wake lock source has already been active,
		 * repeat call of
		 * 'wake_lock' will just add the event_count
		 * of the wake lock source,
		 * and will not cause evil effects.
		 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
		__pm_stay_awake(&pdata->lm3644tt_mix_ir_wakelock);
#else
		__pm_stay_awake(pdata->lm3644tt_mix_ir_wakelock);
#endif
	}
	rc = hw_lm3644tt_mix_ir_update_work_mode(flash_ctrl, cdata);

	mutex_unlock(flash_ctrl->hw_flash_mutex);

	return rc;
}

static int hw_lm3644tt_mix_ir_off(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_flash_cfg_data cdata = {0};
	struct hw_lm3644tt_mix_ir_private_data_t *pdata = NULL;
	int rc;

	if (!flash_ctrl || !flash_ctrl->pdata) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	cam_debug("%s enter", __func__);

	pdata = (struct hw_lm3644tt_mix_ir_private_data_t *)flash_ctrl->pdata;
	cdata.mode = (flash_mode)SINGLE_STANDBY_MODE;
	cdata.data = LM3644TT_STANDBY_MODE_CUR; /* set standby current as 0 */

	mutex_lock(flash_ctrl->hw_flash_mutex);
	rc = hw_lm3644tt_mix_ir_update_work_mode(flash_ctrl, &cdata);
	mutex_unlock(flash_ctrl->hw_flash_mutex);

	cam_info("%s end", __func__);

	return rc;
}

static int hw_lm3644tt_mix_ir_get_dt_data(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_lm3644tt_mix_ir_private_data_t *pdata = NULL;
	struct device_node *node = NULL;
	int i;
	int rc = -EINVAL;
	unsigned int pin_tmp[MAX_PIN] = {0};

	if (!flash_ctrl || !flash_ctrl->pdata || !flash_ctrl->dev ||
		!flash_ctrl->dev->of_node) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	pdata = (struct hw_lm3644tt_mix_ir_private_data_t *)flash_ctrl->pdata;
	node = flash_ctrl->dev->of_node;

	rc = of_property_read_u32_array(node, "vendor,flash-pin",
		pin_tmp, (unsigned long)MAX_PIN);
	if (rc < 0) {
		cam_err("%s get dt flash-pin failed line %d",
			__func__, __LINE__);
		/* if failed, init gpio num as invalid */
		for (i = 0; i < MAX_PIN; i++) {
			pdata->pin[i].pin_id = INVALID_GPIO;
			pdata->pin[i].pin_status = FREED;
		}
		return rc;
	}
	for (i = 0; i < MAX_PIN; i++) {
		pdata->pin[i].pin_id = pin_tmp[i];
		pdata->pin[i].pin_status = FREED;
		cam_info("%s pin[%d] = %u", __func__, i,
			pdata->pin[i].pin_id);
	}

	rc = of_property_read_u32(node, "vendor,flash-chipid",
		&pdata->chipid);
	cam_info("%s vendor,flash-chipid 0x%x, rc %d", __func__,
		pdata->chipid, rc);
	if (rc < 0) {
		cam_err("%s failed %d", __func__, __LINE__);
		return rc;
	}

	rc = of_property_read_u32(node, "vendor,flash-ctrltype",
		&pdata->ctrltype);
	cam_info("%s vendor,ctrltype 0x%x, rc %d", __func__,
		pdata->ctrltype, rc);
	if (rc < 0) {
		cam_err("%s failed %d", __func__, __LINE__);
		return rc;
	} else if (pdata->pin[STROBE].pin_id != INVALID_GPIO) {
		pdata->ctrltype = LM3644TT_CTRL_TYPE_STROBE;
		cam_info("%s ctrltype change 0x%x", __func__, pdata->ctrltype);
	}

	rc = of_property_read_u32(node, "vendor,led-type", &pdata->led_type);
	cam_info("%s vendor,led-type %d, rc %d\n", __func__,
		pdata->led_type, rc);
	if (rc < 0) {
		cam_err("%s read led-type failed %d\n", __func__, __LINE__);
		return rc;
	}

	rc = of_property_read_u32(node, "vendor,need-wakelock",
		(u32 *)&pdata->need_wakelock);
	cam_info("%s vendor,need-wakelock %d, rc %d\n", __func__,
		pdata->need_wakelock, rc);
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
		cam_info("%s ecurrent[%d]=%d\n", __func__,
			i, pdata->ecurrent[i]);

	rc = of_property_read_u32_array(node, "vendor,selftest-param",
		pdata->selftest_param, IR_SELFTEST_MAX_INDEX);
	if (rc < 0) {
		cam_err("%s read selftest-param failed line %d\n",
			__func__, __LINE__);
		return rc;
	}
	for (i = 0; i < IR_SELFTEST_MAX_INDEX; i++)
		cam_info("%s selftest_param[%d]=%d\n", __func__, i,
			pdata->selftest_param[i]);

	return rc;
}

static ssize_t hw_lm3644tt_mix_ir_flash_lightness_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc;

	if (!buf) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	/* PAGE_SIZE = 4096 */
	rc = scnprintf(buf, PAGE_SIZE,
		"front_mode=%d, data=%d; back_mode=%d, data=%d\n",
		g_lm3644tt_mix_ir_pdata.front_mode,
		g_lm3644tt_mix_ir_pdata.front_cur,
		g_lm3644tt_mix_ir_pdata.back_mode,
		g_lm3644tt_mix_ir_pdata.back_cur);

	return rc;
}

static ssize_t hw_lm3644tt_mix_ir_flash_lightness_show_f(struct device *dev,
	struct device_attribute *attr,
	char *buf)
{
	int rc;

	if (!buf) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	/* show for ir selftest in RT, PAGE_SIZE = 4096 */
	rc = scnprintf(buf, PAGE_SIZE, "%d %d %d %d",
		g_lm3644tt_mix_ir_pdata.selftest_param[IR_SELFTEST_ON_TIME_INDEX],
		g_lm3644tt_mix_ir_pdata.selftest_param[IR_SELFTEST_OFF_TIME_INDEX],
		g_lm3644tt_mix_ir_pdata.selftest_param[IR_SELFTEST_CURRENT_INDEX],
		g_lm3644tt_mix_ir_pdata.selftest_param[IR_SELFTEST_TIMES_INDEX]);

	return rc;
}

static int hw_lm3644tt_mix_ir_param_check(char *buf, unsigned long *param,
	int num_of_par)
{
	char *token = NULL;
	unsigned int base = 0;
	int cnt;

	if (!buf || !param) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	token = strsep(&buf, " ");

	for (cnt = 0; cnt < num_of_par; cnt++) {
		if (token) {
			/* add for check token[1] has valid element */
			if (strlen(token) < 2) {
				base = 10; /* decimal */
			} else {
				/* format 0x** */
				if ((token[1] == 'x') || (token[1] == 'X'))
					base = 16; /* hex */
				else
					base = 10; /* decimal */
			}
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
			if (strict_strtoul(token, base, &param[cnt]) != 0)
#else
			if (kstrtoul(token, base, &param[cnt]) != 0)
#endif
				return -EINVAL;
			token = strsep(&buf, " ");
		} else {
			cam_info("%s the %d param is null\n",
				__func__, num_of_par);
		}
	}

	return 0;
}

static ssize_t hw_lm3644tt_mix_ir_flash_lightness_store_imp(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct hw_flash_cfg_data cdata = {0};
	unsigned long param[MAX_LIGHTNESS_PARAM_NUM_RT] = {0};
	int rc;
	int flash_id = 0;

	if (!buf) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	rc = hw_lm3644tt_mix_ir_param_check((char *)buf, param,
		MAX_LIGHTNESS_PARAM_NUM_RT);
	if (rc < 0) {
		cam_err("%s failed to check param", __func__);
		return rc;
	}

	/* 0 - flash id */
	flash_id = (int)param[LIGHTNESS_PARAM_FD_INDEX_RT];
	/* 1 - mode */
	cdata.mode = (int)param[LIGHTNESS_PARAM_MODE_INDEX_RT];
	/* 2 - current */
	cdata.data = (int)param[LIGHTNESS_PARAM_CURRENT_INDEX_RT];
	cam_info("%s flash_id=%d, cdata.mode=%d, cdata.data=%d", __func__,
		flash_id, cdata.mode, cdata.data);

	if ((cdata.mode < STANDBY_MODE) || (cdata.mode > MAX_MODE)) {
		cam_err("%s cdata.mode out of bounds", __func__);
		return -EINVAL;
	}
	/* 0~1500 mA */
	if ((cdata.data < 0) || (cdata.data > LM3644TT_FLASH_MAX_CUR)) {
		cam_err("%s cdata.data out of bounds", __func__);
		return -EINVAL;
	}
	/* 0~0xFF, pass from HAL, but not use just print */
	if ((flash_id < 0) || (flash_id > MAX_FLASH_ID)) {
		cam_err("%s flash_id out of bounds", __func__);
		return -EINVAL;
	}

	if (cdata.mode == STANDBY_MODE) {
		rc = hw_lm3644tt_mix_ir_off(&g_lm3644tt_mix_ir_ctrl);
		if (rc < 0) {
			cam_err("%s flash off error", __func__);
			return rc;
		}
	} else if (cdata.mode == TORCH_MODE) {
		if (g_lm3644tt_mix_ir_ctrl.mix_pos == HWFLASH_POSITION_FORE)
			cdata.data =
				g_lm3644tt_mix_ir_pdata.ecurrent[CURRENT_TORCH_LEVEL_RT_FRONT];
		else
			cdata.data =
				g_lm3644tt_mix_ir_pdata.ecurrent[CURRENT_TORCH_LEVEL_RT_BACK];

		cam_info("%s mode=%d, current=%d", __func__,
			cdata.mode, cdata.data);

		rc = hw_lm3644tt_mix_ir_on(&g_lm3644tt_mix_ir_ctrl, &cdata);
		if (rc < 0) {
			cam_err("%s flash on error", __func__);
			return rc;
		}
	} else if (cdata.mode == FLASH_MODE) {
		cam_info("%s mode=%d, current=%d", __func__,
			cdata.mode, cdata.data);
		rc = hw_lm3644tt_mix_ir_on(&g_lm3644tt_mix_ir_ctrl, &cdata);
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

static ssize_t hw_lm3644tt_mix_ir_flash_lightness_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	/* call back flash */
	g_lm3644tt_mix_ir_ctrl.mix_pos = HWFLASH_POSITION_REAR;
	return hw_lm3644tt_mix_ir_flash_lightness_store_imp(dev, attr,
		buf, count);
}

static ssize_t hw_lm3644tt_mix_ir_flash_lightness_f_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count)
{
	/* call front flash */
	g_lm3644tt_mix_ir_ctrl.mix_pos = HWFLASH_POSITION_FORE;
	return hw_lm3644tt_mix_ir_flash_lightness_store_imp(dev, attr,
		buf, count);
}

static ssize_t hw_lm3644tt_mix_ir_flash_mask_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int rc;

	if (!buf) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	rc = scnprintf(buf, PAGE_SIZE, "flash_mask_disabled=%d\n",
		g_lm3644tt_mix_ir_ctrl.flash_mask_enable);

	return rc;
}

static ssize_t hw_lm3644tt_mix_ir_flash_mask_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	if (!buf) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	if (buf[0] == '0') /* char '0' for mask disable */
		g_lm3644tt_mix_ir_ctrl.flash_mask_enable = false;
	else
		g_lm3644tt_mix_ir_ctrl.flash_mask_enable = true;

	cam_info("%s flash_mask_enable=%d", __func__,
		g_lm3644tt_mix_ir_ctrl.flash_mask_enable);
	return (ssize_t)count;
}

static void hw_lm3644tt_mix_ir_torch_brightness_set_imp(struct led_classdev *cdev,
	enum led_brightness brightness)
{
	int rc = 0;
	struct hw_flash_cfg_data cdata = {0};
	unsigned int led_bright = brightness;

	cam_info("%s brightness= %d", __func__, brightness);

	if (led_bright == LED_OFF) {
		rc = hw_lm3644tt_mix_ir_off(&g_lm3644tt_mix_ir_ctrl);
		if (rc < 0) {
			cam_err("%s pmu_led off error", __func__);
			return;
		}
	} else {
		cdata.mode = TORCH_MODE;
		if (g_lm3644tt_mix_ir_ctrl.mix_pos == HWFLASH_POSITION_FORE)
			cdata.data =
				g_lm3644tt_mix_ir_pdata.ecurrent[CURRENT_TORCH_LEVEL_MMI_FRONT];
		else
			cdata.data =
				g_lm3644tt_mix_ir_pdata.ecurrent[CURRENT_TORCH_LEVEL_MMI_BACK];

		cam_info("%s brightness=0x%x, mode=%d, data=%d", __func__,
			brightness, cdata.mode, cdata.data);
		rc = hw_lm3644tt_mix_ir_on(&g_lm3644tt_mix_ir_ctrl, &cdata);
		if (rc < 0) {
			cam_err("%s flash on error", __func__);
			return;
		}
	}
}

static void hw_lm3644tt_mix_ir_torch_brightness_set(struct led_classdev *cdev,
	enum led_brightness brightness)
{
	g_lm3644tt_mix_ir_ctrl.mix_pos = HWFLASH_POSITION_REAR;
	hw_lm3644tt_mix_ir_torch_brightness_set_imp(cdev, brightness);
}

static void hw_lm3644tt_mix_ir_torch_brightness_f_set(struct led_classdev *cdev,
	enum led_brightness brightness)
{
	g_lm3644tt_mix_ir_ctrl.mix_pos = HWFLASH_POSITION_FORE;
	hw_lm3644tt_mix_ir_torch_brightness_set_imp(cdev, brightness);
}

/* for RT */
static struct device_attribute g_lm3644tt_mix_ir_flash_lightness =
	__ATTR(flash_lightness, 0660, hw_lm3644tt_mix_ir_flash_lightness_show,
	hw_lm3644tt_mix_ir_flash_lightness_store); /* 660:-wr-wr--- */

static struct device_attribute g_lm3644tt_mix_ir_flash_lightness_f =
	__ATTR(flash_lightness_f, 0660,
	hw_lm3644tt_mix_ir_flash_lightness_show_f,
	hw_lm3644tt_mix_ir_flash_lightness_f_store); /* 660:-wr-wr--- */

static struct device_attribute g_lm3644tt_mix_ir_flash_mask =
	__ATTR(flash_mask, 0660, hw_lm3644tt_mix_ir_flash_mask_show,
	hw_lm3644tt_mix_ir_flash_mask_store); /* 660:-wr-wr--- */

extern int register_camerafs_attr(struct device_attribute *attr);
static int hw_lm3644tt_mix_ir_register_attribute(struct hw_flash_ctrl_t *flash_ctrl,
	struct device *dev)
{
	int rc;

	if (!flash_ctrl || !dev) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	register_camerafs_attr(&g_lm3644tt_mix_ir_flash_lightness);
	register_camerafs_attr(&g_lm3644tt_mix_ir_flash_lightness_f);

	flash_ctrl->cdev_torch.name = "torch";
	flash_ctrl->cdev_torch.max_brightness
		= (enum led_brightness)MAX_BRIGHTNESS_FORMMI;
	flash_ctrl->cdev_torch.brightness_set =
		hw_lm3644tt_mix_ir_torch_brightness_set;
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
		hw_lm3644tt_mix_ir_torch_brightness_f_set;
	rc = led_classdev_register((struct device *)dev,
		&flash_ctrl->cdev_torch1);
	if (rc < 0) {
		cam_err("%s failed to register torch_front classdev", __func__);
		goto err_create_torch_front_file;
	}

	rc = device_create_file(dev, &g_lm3644tt_mix_ir_flash_lightness);
	if (rc < 0) {
		cam_err("%s failed to creat flash_lightness attribute",
			__func__);
		goto err_create_flash_lightness_file;
	}

	rc = device_create_file(dev, &g_lm3644tt_mix_ir_flash_lightness_f);
	if (rc < 0) {
		cam_err("%s failed to creat flash_f_lightness attribute",
			__func__);
		goto err_create_flash_f_lightness_file;
	}

	rc = device_create_file(dev, &g_lm3644tt_mix_ir_flash_mask);
	if (rc < 0) {
		cam_err("%s failed to creat flash_mask attribute", __func__);
		goto err_create_flash_mask_file;
	}

	return 0;

err_create_flash_mask_file:
	device_remove_file(dev, &g_lm3644tt_mix_ir_flash_lightness_f);
err_create_flash_f_lightness_file:
	device_remove_file(dev, &g_lm3644tt_mix_ir_flash_lightness);
err_create_flash_lightness_file:
	led_classdev_unregister(&flash_ctrl->cdev_torch1);
err_create_torch_front_file:
	led_classdev_unregister(&flash_ctrl->cdev_torch);
err_out:
	return rc;
}

static int hw_lm3644tt_mix_ir_match(struct hw_flash_ctrl_t *flash_ctrl)
{
	struct hw_flash_i2c_client *i2c_client = NULL;
	struct hw_flash_i2c_fn_t *i2c_func = NULL;
	struct hw_lm3644tt_mix_ir_private_data_t *pdata = NULL;
	unsigned char id = 0;
	unsigned char mode = 0;

	if (!flash_ctrl || !flash_ctrl->pdata ||
		!flash_ctrl->flash_i2c_client ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl ||
		!flash_ctrl->flash_i2c_client->i2c_func_tbl->i2c_read) {
		cam_err("%s invalid params", __func__);
		return -EINVAL;
	}

	i2c_client = flash_ctrl->flash_i2c_client;

	/* change to AW3644tt actual slave address 0x63 */
	i2c_client->client->addr = LM3644TT_SLAVE_ADDRESS;
	i2c_func = i2c_client->i2c_func_tbl;
	pdata = (struct hw_lm3644tt_mix_ir_private_data_t *)flash_ctrl->pdata;

	/*
	 * Enable lm3644tt_mix_ir switch to standby current is 10ua,
	 * if match id success, flash_en pin will always be enabled.
	 */
	hw_lm3644tt_mix_ir_set_pin(flash_ctrl, FLASH_EN, HIGH);
	i2c_func->i2c_read(i2c_client, REG_CHIP_ID, &id);

	if (id == LM3644TT_CHIP_ID) {
		g_lm3644tt_mix_ir_pdata.chip_type = LM3644TT;
		cam_info("%s 0x%x chip type is LM3644TT", __func__, id);
	} else if (id == KTD2687_CHIP_ID) {
		loge_if_ret(i2c_func->i2c_read(i2c_client, MODE_TEMP, &mode) < 0);
		if (mode == MODE_TEMP_VALUE) {
			g_lm3644tt_mix_ir_pdata.chip_type = AW3644;
			cam_info("%s 0x%x mode=0x%x chip type is AW3644",
				__func__, id, mode);
		} else {
			g_lm3644tt_mix_ir_pdata.chip_type = KTD2687;
			cam_info("%s 0x%x mode=0x%x chip type is KTD2687",
				__func__, id, mode);
		}
	} else {
		cam_err("%s match error, 0x%x!= 0x%x or 0x%x",
			__func__, id, LM3644TT_CHIP_ID, KTD2687_CHIP_ID);

		/*
		 * Enable lm3644tt_mix_ir switch to shutdown
		 * when matchid fail, current is 1.3ua
		 */
		hw_lm3644tt_mix_ir_set_pin(flash_ctrl, FLASH_EN, LOW);
		return -EINVAL;
	}

	return 0;
}

static int hw_lm3644tt_mix_ir_remove(struct i2c_client *client)
{
	cam_debug("%s enter", __func__);
	if (!client) {
		cam_err("%s client is NULL", __func__);
		return -EINVAL;
	}
	g_lm3644tt_mix_ir_ctrl.func_tbl->flash_exit(&g_lm3644tt_mix_ir_ctrl);

	client->adapter = NULL;
	return 0;
}

static const struct i2c_device_id g_lm3644tt_mix_ir_id[] = {
	{ "lm3644tt_mix_ir", (unsigned long)&g_lm3644tt_mix_ir_ctrl },
	{}
};

static const struct of_device_id g_lm3644tt_mix_ir_dt_match[] = {
	{ .compatible = "vendor,lm3644tt_mix_ir" },
	{}
};
MODULE_DEVICE_TABLE(of, lm3644tt_mix_ir_dt_match);

static struct i2c_driver g_lm3644tt_mix_ir_i2c_driver = {
	.probe = hw_flash_i2c_probe,
	.remove = hw_lm3644tt_mix_ir_remove,
	.id_table = g_lm3644tt_mix_ir_id,
	.driver = {
		.name = "hw_lm3644tt_mix_ir",
		.of_match_table = g_lm3644tt_mix_ir_dt_match,
	},
};

static int __init hw_lm3644tt_mix_ir_module_init(void)
{
	cam_info("%s erter", __func__);
	return i2c_add_driver(&g_lm3644tt_mix_ir_i2c_driver);
}

static void __exit hw_lm3644tt_mix_ir_module_exit(void)
{
	cam_info("%s enter", __func__);
	i2c_del_driver(&g_lm3644tt_mix_ir_i2c_driver);
}

static struct hw_flash_i2c_client g_lm3644tt_mix_ir_i2c_client;

static struct hw_flash_fn_t g_lm3644tt_mix_ir_func_tbl = {
	.flash_config = hw_flash_config,
	.flash_init = hw_lm3644tt_mix_ir_init,
	.flash_exit = hw_lm3644tt_mix_ir_exit,
	.flash_on = hw_lm3644tt_mix_ir_on,
	.flash_off = hw_lm3644tt_mix_ir_off,
	.flash_match = hw_lm3644tt_mix_ir_match,
	.flash_get_dt_data = hw_lm3644tt_mix_ir_get_dt_data,
	.flash_register_attribute = hw_lm3644tt_mix_ir_register_attribute,
};

struct hw_flash_ctrl_t g_lm3644tt_mix_ir_ctrl = {
	.flash_i2c_client = &g_lm3644tt_mix_ir_i2c_client,
	.func_tbl = &g_lm3644tt_mix_ir_func_tbl,
	.hw_flash_mutex = &flash_mut_lm3644tt_mix_ir,
	.pdata = (void *)&g_lm3644tt_mix_ir_pdata,
	.flash_mask_enable = true,
	.state = {
		.mode = STANDBY_MODE,
	},
};

module_init(hw_lm3644tt_mix_ir_module_init);
module_exit(hw_lm3644tt_mix_ir_module_exit);
MODULE_DESCRIPTION("LM3644TT FLASH");
MODULE_LICENSE("GPL v2");
