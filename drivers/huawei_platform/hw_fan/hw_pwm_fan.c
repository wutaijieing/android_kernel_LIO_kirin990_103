// SPDX-License-Identifier: GPL-2.0
/*
 * hw_pwm_fan.c
 *
 * driver for hw_pwm_fan
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

#include "hw_pwm_fan.h"

#define HWLOG_TAG hw_pwm_fan
HWLOG_REGIST();

static struct pwm_fan_data *g_hw_pwm_fan_data;
static struct pinctrl_data g_pwmpctrl;
static struct pinctrl_cmd_desc pwm_pinctrl_init_cmds[] = {
	{DTYPE_PINCTRL_GET, &g_pwmpctrl, 0},
	{DTYPE_PINCTRL_STATE_GET, &g_pwmpctrl, DTYPE_PINCTRL_STATE_DEFAULT},
	{DTYPE_PINCTRL_STATE_GET, &g_pwmpctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static struct pinctrl_cmd_desc pwm_pinctrl_normal_cmds[] = {
	{DTYPE_PINCTRL_SET, &g_pwmpctrl, DTYPE_PINCTRL_STATE_DEFAULT},
};

static struct pinctrl_cmd_desc pwm_pinctrl_lowpower_cmds[] = {
	{DTYPE_PINCTRL_SET, &g_pwmpctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static struct pinctrl_cmd_desc pwm_pinctrl_finit_cmds[] = {
	{DTYPE_PINCTRL_PUT, &g_pwmpctrl, 0},
};

static ssize_t hw_fan_pwm_duty_show(struct device *dev,
	struct device_attribute *attr, char *buf);

static ssize_t hw_fan_status_show(struct device *dev,
	struct device_attribute *attr, char *buf);

static ssize_t hw_fan_speed_show(struct device *dev,
	struct device_attribute *attr, char *buf);

static ssize_t hw_fan_speed_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count);

static DEVICE_ATTR(hw_fan_pwm_duty, S_IRUGO, hw_fan_pwm_duty_show, NULL);
static DEVICE_ATTR(hw_fan_status, S_IRUGO, hw_fan_status_show, NULL);
static DEVICE_ATTR(hw_fan_speed, S_IWUSR | S_IRUGO, hw_fan_speed_show,
	hw_fan_speed_store);

static struct device_attribute *hw_fan_dev_attrs[] = {
	&dev_attr_hw_fan_pwm_duty,
	&dev_attr_hw_fan_status,
	&dev_attr_hw_fan_speed,
	NULL,
};

static int hw_fan_power_switch(int enable);

static struct pwm_fan_data *hw_pwm_fan_get_data(void)
{
	if (!g_hw_pwm_fan_data) {
		hwlog_err("data not init\n");
		return NULL;
	}

	return g_hw_pwm_fan_data;
}

static inline void outp32(void *addr, uint32_t val)
{
	writel(val, addr);
}

static int pinctrl_get_handler(struct platform_device *pdev, struct pinctrl_cmd_desc *cmds)
{
	if (!pdev) {
		hwlog_info("pdev is NULL");
		return -EINVAL;
	}
	cmds->pctrl_data->p = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(cmds->pctrl_data->p)) {
		hwlog_info("failed to get pinctrl\n");
		return -1;
	}

	return 0;
}

static int pinctrl_state_get_handler(struct pinctrl_cmd_desc *cmds)
{
	switch (cmds->mode) {
	case DTYPE_PINCTRL_STATE_DEFAULT:
		cmds->pctrl_data->pinctrl_def = pinctrl_lookup_state(cmds->pctrl_data->p,
			PINCTRL_STATE_DEFAULT);
		if (IS_ERR(cmds->pctrl_data->pinctrl_def)) {
			hwlog_info("failed to get pinctrl_def\n");
			return -1;
		}
		break;
	case DTYPE_PINCTRL_STATE_IDLE:
		cmds->pctrl_data->pinctrl_idle = pinctrl_lookup_state(cmds->pctrl_data->p,
			PINCTRL_STATE_IDLE);
		if (IS_ERR(cmds->pctrl_data->pinctrl_idle)) {
			hwlog_info("failed to get pinctrl_idle\n");
			return -1;
		}
		break;
	default:
		hwlog_info("unknown pinctrl type to get\n");
		return -1;
	}

	return 0;
}

static int pinctrl_set_handler(struct pinctrl_cmd_desc *cmds)
{
	int ret;

	switch (cmds->mode) {
	case DTYPE_PINCTRL_STATE_DEFAULT:
		if (cmds->pctrl_data->p && cmds->pctrl_data->pinctrl_def) {
			ret = pinctrl_select_state(cmds->pctrl_data->p,
				cmds->pctrl_data->pinctrl_def);
			if (ret) {
				hwlog_info("could not set this pin to default state\n");
				return -1;
			}
		}
		break;
	case DTYPE_PINCTRL_STATE_IDLE:
		if (cmds->pctrl_data->p && cmds->pctrl_data->pinctrl_idle) {
			ret = pinctrl_select_state(cmds->pctrl_data->p,
				cmds->pctrl_data->pinctrl_idle);
			if (ret) {
				hwlog_info("could not set this pin to idle state\n");
				return -1;
			}
		}
		break;
	default:
		hwlog_info("unknown pinctrl type to set\n");
		return -1;
	}

	return 0;
}

static int pinctrl_cmds_tx(struct platform_device *pdev,
	struct pinctrl_cmd_desc *cmds, int cnt)
{
	int ret;
	int i = 0;

	for (i = 0; i < cnt; i++) {
		if (!cmds) {
			hwlog_info("cmds is null! index=%d\n", i);
			continue;
		}

		switch (cmds->dtype) {
		case DTYPE_PINCTRL_GET:
			ret = pinctrl_get_handler(pdev, cmds);
			if (ret)
				return ret;
			break;
		case DTYPE_PINCTRL_STATE_GET:
			ret = pinctrl_state_get_handler(cmds);
			if (ret)
				return ret;
			break;
		case DTYPE_PINCTRL_SET:
			ret = pinctrl_set_handler(cmds);
			if (ret)
				return ret;
			break;
		case DTYPE_PINCTRL_PUT:
			if (cmds->pctrl_data->p)
				pinctrl_put(cmds->pctrl_data->p);
			break;
		default:
			hwlog_info("not supported command type\n");
			return -1;
		}
		cmds++;
	}

	return 0;
}

static void pwm_set_out_duty(void __iomem *pwm_base, u32 duty, struct pwm_fan_data *data)
{
	u32 start_high, start_low, blpwm_out_cfg_val;

	start_high = duty * FAN_BLPWM_TOTAL_CYCLE / PWM_OUT_PRECISION;
	start_low = FAN_BLPWM_TOTAL_CYCLE - start_high;
	blpwm_out_cfg_val = start_low | (start_high << PWM_WORD_OFFSET);

	switch (data->pwm_mode) {
	case PWM_OUT0_MODE:
		outp32(pwm_base + PWM_LOCK_OFFSET, PWM_CTRL_LOCK);
		outp32(pwm_base + PWM_CTL_OFFSET, 0x0);
		outp32(pwm_base + PWM_CFG_OFFSET, 0x2); /* enable channel 0 */
		outp32(pwm_base + PWM_PR0_OFFSET, PWM_OUT_DIV_PR0);
		outp32(pwm_base + PWM_PR1_OFFSET, data->freq_out_div);
		outp32(pwm_base + PWM_CTL_OFFSET, 0x1);
		outp32(pwm_base + PWM_C1_MR_OFFSET, (PWM_OUT_PRECISION - 1));
		outp32(pwm_base + PWM_C1_MR0_OFFSET, duty);
		break;
	case PWM_OUT1_MODE:
		outp32(pwm_base + PWM_LOCK_OFFSET, PWM_CTRL_LOCK);
		outp32(pwm_base + PWM_CTL_OFFSET, 0x0);
		outp32(pwm_base + PWM_CFG_OFFSET, 0x8); /* enable channel 1 */
		outp32(pwm_base + PWM_PR0_OFFSET, PWM_OUT_DIV_PR0);
		outp32(pwm_base + PWM_PR1_OFFSET, data->freq_out_div);
		outp32(pwm_base + PWM_CTL_OFFSET, 0x1);
		outp32(pwm_base + PWM_C1_MR_OFFSET, (PWM_OUT_PRECISION - 1));
		outp32(pwm_base + PWM_C1_MR0_OFFSET, duty);
		break;
	case BLPWM_OUT_MODE:
		outp32(pwm_base + BLPWM_OUT_CTL, 0x0);
		outp32(pwm_base + BLPWM_OUT_DIV, data->freq_out_div);
		outp32(pwm_base + BLPWM_OUT_CFG, blpwm_out_cfg_val);
		outp32(pwm_base + BLPWM_OUT_CTL, 0x1);
		break;
	default:
		hwlog_err("invalid mode\n");
		return;
	}

	return;
}

static void pwm_out_config(struct pwm_fan_data *data, u32 duty)
{
	void __iomem *pwm_base = NULL;

	if (data->pwm_mode == BLPWM_OUT_MODE)
		pwm_base = data->blpwm_base;
	else
		pwm_base = data->pwm_base;
	if (!pwm_base) {
		hwlog_info("pwm_base is null\n");
		return;
	}

	pwm_set_out_duty(pwm_base, duty, data);
	data->pwmout_duty = duty;
	return;
}

static void hw_fan_set_speed(struct pwm_fan_data *data, u8 fan_speed)
{
	uint32_t speed = 0;

	hwlog_info("fan_speed=%d\n", fan_speed);

	if (fan_speed > FAN_PWM_DUTY_MAX)
		fan_speed = FAN_PWM_DUTY_MAX;
	if (fan_speed == 0)
		fan_speed = FAN_PWM_DUTY_MIN;

	data->pwmout_speed = fan_speed;
	speed = fan_speed * PWM_OUT_PRECISION / FAN_PWM_DUTY_MAX;
	pwm_out_config(data, speed);

	return;
}

static u8 look_up_pwm_duty_map(struct pwm_fan_data *data, int dest_speed)
{
	int i;
	int level = data->speed_map_level;

	for (i = 0; i < level; i++) {
		if (dest_speed <= data->speed_map[i].dest_speed)
			return data->speed_map[i].pwm_duty;
	}

	return data->speed_map[level - 1].pwm_duty;
}

static void hw_fan_dest_speed(struct pwm_fan_data *data, int dest_speed)
{
	u8 pwm_duty = look_up_pwm_duty_map(data, dest_speed);

	mutex_lock(&data->lock);
	if (abs(data->dest_speed - dest_speed) > REGULATE_SPEED_RANGE) {
		data->dest_changed = true;
		hw_fan_set_speed(data, pwm_duty);
	} else {
		data->dest_changed = false;
	}
	mutex_unlock(&data->lock);
	data->dest_speed = dest_speed;
}

static int get_fan_speed(struct pwm_fan_data *data)
{
	uint32_t continue_pwm_in_num;
	uint16_t continue_pwm_in_high_num;
	uint16_t continue_pwm_in_low_num;
	uint32_t debug_pwm_in_num;
	uint16_t debug_pwm_in_high_num;
	uint16_t debug_pwm_in_low_num;
	int speed;
	int duty_continue;
	int duty_debug;
	void __iomem *blpwm_base = data->blpwm_base;

	if (!blpwm_base || !data->blpwm_clk_rate) {
		hwlog_err("blpwm_base or clk_rate is null\n");
		return -ERANGE;
	}

	continue_pwm_in_num = readl(blpwm_base + PWM_IN_NUM_OFFSET);
	if (continue_pwm_in_num <= 0) {
		hwlog_info("blpwm_in_num is invalid\n");
		return -EINVAL;
	}

	debug_pwm_in_num = readl(blpwm_base + PWM_IN_DEBUG_NUM_OFFSET);
	if (debug_pwm_in_num <= 0) {
		hwlog_info("debug_num blpwm_in_num is invalid\n");
		return -EINVAL;
	}

	continue_pwm_in_high_num = continue_pwm_in_num >> PWM_WORD_OFFSET;
	continue_pwm_in_low_num  = continue_pwm_in_num & PWM_IN_MAX_COUNTER_VAL;
	debug_pwm_in_high_num = debug_pwm_in_num >> PWM_WORD_OFFSET;
	debug_pwm_in_low_num = debug_pwm_in_num & PWM_IN_MAX_COUNTER_VAL;

	duty_continue = PWM_DUTY_PERCENT * continue_pwm_in_high_num /
		(continue_pwm_in_high_num + continue_pwm_in_low_num);
	duty_debug = PWM_DUTY_PERCENT * debug_pwm_in_high_num /
		(debug_pwm_in_high_num + debug_pwm_in_low_num);
	if ((duty_continue > BLPWM_IN_PWM_MIN) && (duty_continue < BLPWM_IN_PWM_MAX) &&
		(duty_debug > BLPWM_IN_PWM_MIN) && (duty_debug < BLPWM_IN_PWM_MAX)) {
		speed = (continue_pwm_in_high_num + continue_pwm_in_low_num) *
			(BLPWM_IN_DIV + 1) / data->blpwm_clk_rate;
		speed += (debug_pwm_in_high_num + debug_pwm_in_low_num) *
			(BLPWM_IN_DIV + 1) / data->blpwm_clk_rate;
		speed = FAN_TIME_MINUTE / speed;
	} else {
		speed = 0;
	}

	return speed;
}

static int hw_get_fan_speed(struct pwm_fan_data *data)
{
	int speed;
	int retry = RETRY_MAX_COUNT;

	while (retry > 0) {
		speed = get_fan_speed(data);
		if (speed > 0)
			break;
		msleep(FAN_SLEEP_TIME);
		retry--;
	}

	return speed;
}

static int hw_pwm_input_switch(int enable)
{
	struct pwm_fan_data *data = hw_pwm_fan_get_data();

	if (!data)
		return -EINVAL;

	if (enable) {
		outp32(data->blpwm_base + PWM_IN_CTRL_OFFSET, 1);
		outp32(data->blpwm_base + PWM_IN_DIV_OFFSET, BLPWM_IN_DIV);
		outp32(data->blpwm_base + PWM_IN_MAX_COUNTER_OFFSET,
			PWM_IN_MAX_COUNTER_VAL);
	} else {
		outp32(data->blpwm_base + PWM_IN_CTRL_OFFSET, 0);
	}

	return 0;
}

static int hw_pwm_clk_enable(int enable)
{
	int ret = 0;
	struct pwm_fan_data *data = hw_pwm_fan_get_data();

	if (!data || data->blpwm_clk)
		return -EINVAL;

	if (enable) {
		ret = clk_prepare(data->blpwm_clk);
		if (ret)
			goto exit;

		ret = clk_enable(data->blpwm_clk);
		if (ret)
			goto exit;

		if (data->pwm_mode == BLPWM_OUT_MODE)
			goto exit;

		ret = clk_prepare(data->pwm_clk);
		if (ret)
			goto exit;

		ret = clk_enable(data->pwm_clk);
		if (ret)
			goto exit;
	} else {
		clk_disable(data->blpwm_clk);
		clk_unprepare(data->blpwm_clk);
		if (data->pwm_mode != BLPWM_OUT_MODE) {
			clk_disable(data->pwm_clk);
			clk_unprepare(data->pwm_clk);
		}
	}

exit:
	return ret;
}

static int hw_fan_switch(int turn_on)
{
	int ret;
	struct pwm_fan_data *data = hw_pwm_fan_get_data();

	if (!data)
		return -EINVAL;

	if (turn_on) {
		if (data->pwm_on) {
			hwlog_info("already turn on\n");
			return 0;
		}
		/* dis-reset pwm */
		outp32(data->peri_crg_base + PEREN2, 0x1);
		hw_pwm_clk_enable(turn_on);
		ret = pinctrl_cmds_tx(data->pwm_pdev, pwm_pinctrl_normal_cmds,
			ARRAY_SIZE(pwm_pinctrl_normal_cmds));
		data->pwm_on = true;
	} else {
		if (data->pwm_mode != BLPWM_OUT_MODE)
			outp32(data->pwm_base + PWM_CTL_OFFSET, 0x0);
		hw_pwm_clk_enable(turn_on);
		ret = pinctrl_cmds_tx(data->pwm_pdev, pwm_pinctrl_lowpower_cmds,
			ARRAY_SIZE(pwm_pinctrl_lowpower_cmds));
		data->pwm_on = false;
	}

	return ret;
}

static ssize_t hw_fan_pwm_duty_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct pwm_fan_data *data = hw_pwm_fan_get_data();

	if (!buf || !data)
		return -ENXIO;

	return scnprintf(buf, PAGE_SIZE, "%d", data->pwmout_duty);
}

static ssize_t hw_fan_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct pwm_fan_data *data = hw_pwm_fan_get_data();

	if (!buf || !data)
		return -ENXIO;

	hwlog_info("fan cur_speed: %d\n", data->cur_speed);
	return scnprintf(buf, PAGE_SIZE, "%d", data->fan_status);
}

static ssize_t hw_fan_speed_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int speed = -1;
	struct pwm_fan_data *data = hw_pwm_fan_get_data();

	if (!buf || !data)
		return -ENXIO;

	if (data->power_enable)
		speed = hw_get_fan_speed(data);

	return scnprintf(buf, PAGE_SIZE, "%d", speed);
}

static ssize_t hw_fan_speed_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret;
	int val = 0;
	struct pwm_fan_data *data = hw_pwm_fan_get_data();

	if (!buf || !data)
		return -ENXIO;

	ret = kstrtoint(buf, 0, &val);
	if (ret < 0)
		return ret;

	if ((val == FAN_SWITCH_OFF_CMD) && data->power_enable) {
		hw_fan_power_switch(false);
	} else if ((val != FAN_SWITCH_OFF_CMD) && data->power_enable) {
		hw_fan_dest_speed(data, val);
	} else if ((val != FAN_SWITCH_OFF_CMD) && !data->power_enable) {
		hw_fan_power_switch(true);
		hw_fan_dest_speed(data, val);
	}

	return count;
}

static int get_reg_base(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct pwm_fan_data *data = hw_pwm_fan_get_data();

	if (!np || !data) {
		hwlog_err("np or data null\n");
		return -EINVAL;
	}

	/* get pwm reg base */
	data->pwm_base = of_iomap(np, 0);
	if (!data->pwm_base)
		goto error_exit;

	/* get Peri base */
	data->peri_crg_base = of_iomap(np, 1);
	if (!data->peri_crg_base)
		goto error_exit;

	/* get blpwm base */
	data->blpwm_base = of_iomap(np, 2);
	if (!data->blpwm_base)
		goto error_exit;

	return 0;
error_exit:
	hwlog_err("failed to get reg_base resource\n");
	return -EINVAL;
}

static int get_clk_resource(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct pwm_fan_data *data = hw_pwm_fan_get_data();

	if (!np || !data) {
		hwlog_err("np or data null\n");
		return -EINVAL;
	}

	if (data->pwm_mode != BLPWM_OUT_MODE) {
		/* get pwm clk resource */
		data->pwm_clk = of_clk_get(np, 0);
		if (!data->pwm_clk)
			goto error_exit;
		hwlog_info("pwm_clk:[%lu]->[%lu]\n",
			PWM_CLK_DEFAULT_RATE, clk_get_rate(data->pwm_clk));
	}

	/* get blpwm clk resource */
	data->blpwm_clk = of_clk_get(np, 1);
	if (!data->blpwm_clk)
		goto error_exit;

	return 0;

error_exit:
	hwlog_err("failed to get reg_base resource\n");
	return -EINVAL;
}

static int get_fan_speed_map(struct platform_device *pdev, const char *prop)
{
	int i, len, temp_data;
	const char *temp_string = NULL;
	struct device_node *np = pdev->dev.of_node;
	struct pwm_fan_data *data = hw_pwm_fan_get_data();

	if (!np || !data) {
		hwlog_err("np or data null\n");
		return -EINVAL;
	}

	len = of_property_count_strings(np, prop);
	if ((len <= 0) || (len % FAN_SPEED_PARA_TOTAL != 0) ||
		(len > FAN_SPEED_PARA_LEVEL * FAN_SPEED_PARA_TOTAL)) {
		hwlog_err("prop %s length read fail\n", prop);
		return -EINVAL;
	}

	for (i = 0; i < len; i++) {
		if (of_property_read_string_index(np, prop, i, &temp_string)) {
			hwlog_err("prop %s[%d] read fail\n", prop, i);
			return -EINVAL;
		}
		if (kstrtoint(temp_string, 0, &temp_data))
			return -EINVAL;

		switch (i % FAN_SPEED_PARA_TOTAL) {
		case FAN_PWM_DUTY_PARA:
			data->speed_map[i / FAN_SPEED_PARA_TOTAL].pwm_duty = temp_data;
			break;
		case FAN_DEST_SPEED_PARA:
			data->speed_map[i / FAN_SPEED_PARA_TOTAL].dest_speed = temp_data;
			break;
		default:
			break;
		}
	}

	data->speed_map_level = len / FAN_SPEED_PARA_TOTAL;
	return len;
}

static int get_frequence_out_div(struct platform_device *pdev)
{
	int ret;
	struct device_node *np = pdev->dev.of_node;
	struct pwm_fan_data *data = hw_pwm_fan_get_data();

	if (!np || !data) {
		hwlog_err("np or data null\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "fan_work_freq", &data->fan_work_freq);
	if (ret) {
		data->fan_work_freq = FAN_WORK_FREQ_DEFAULT;
		hwlog_info("get fan_work_freq failed, use default 25KHZ\n");
	}

	ret = of_property_read_u32(np, "blpwm_freq_ratio", &data->blpwm_freq_ratio);
	if (ret) {
		data->blpwm_freq_ratio = BLM_FREQ_DEAULT_RATIO;
		hwlog_info("failed to get blpwm_freq_ratio, use default 1\n");
	}
	data->blpwm_clk_rate = data->blpwm_freq_ratio *
		clk_get_rate(data->blpwm_clk) / FAN_FREQ_10KHZ;
	hwlog_info("blpwm_clk:[%lu]->[%d]KHZ\n", BLPWM_CLK_DEFAULT_RATE,
		data->blpwm_clk_rate);

	ret = of_property_read_u32(np, "pwm_mode", &data->pwm_mode);
	if (ret) {
		data->pwm_mode = PWM_OUT0_MODE;
		hwlog_err("failed to get pwm_mode, default PWM_OUT0\n");
	}

	if (data->pwm_mode == BLPWM_OUT_MODE)
		data->freq_out_div = clk_get_rate(data->blpwm_clk) * data->blpwm_freq_ratio /
			(data->fan_work_freq * FAN_BLPWM_TOTAL_CYCLE);
	else
		data->freq_out_div = clk_get_rate(data->pwm_clk) /
			(data->fan_work_freq * PWM_OUT_PRECISION * (PWM_OUT_DIV_PR0 + 1));
	if (data->freq_out_div > 0)
		data->freq_out_div -= 1;

	hwlog_info("pwm_mode = %d, freq_out_div = %u\n", data->pwm_mode, data->freq_out_div);
	return 0;
}

static int hw_pwm_fan_parse_dt(struct platform_device *pdev)
{
	int ret;
	struct device_node *np = pdev->dev.of_node;
	struct pwm_fan_data *data = hw_pwm_fan_get_data();

	if (!np || !data) {
		hwlog_err("np or data null\n");
		return -EINVAL;
	}

	ret = get_reg_base(pdev);
	if (ret)
		return ret;

	ret = get_clk_resource(pdev);
	if (ret)
		return ret;

	data->gpio_power = of_get_named_gpio(np, "gpio_power", 0);
	if (!gpio_is_valid(data->gpio_power)) {
		hwlog_err("get gpio_power fail\n");
		return -EINVAL;
	}

	ret = get_fan_speed_map(pdev, "fan_speed_map");
	if (ret <= 0)
		return -EINVAL;

	ret = get_frequence_out_div(pdev);
	return ret;
}

static void fan_sysfs_create(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(hw_fan_dev_attrs); i++) {
		if (!hw_fan_dev_attrs[i])
			break;
		device_create_file(&pdev->dev, hw_fan_dev_attrs[i]);
	}

	return;
}

static int hw_fan_power_switch(int enable)
{
	int ret;
	struct pwm_fan_data* data = hw_pwm_fan_get_data();

	if (!data)
		return -EINVAL;

	if (enable)
		ret = gpio_direction_output(data->gpio_power, 1);
	else
		ret = gpio_direction_output(data->gpio_power, 0);

	if (ret) {
		hwlog_err("gpio set output fail\n");
		return -EINVAL;
	}

	if (enable)
		data->power_enable = true;
	else
		data->power_enable = false;

	hwlog_info("power switch is %d\n", enable);
	return ret;
}

static int hw_pwm_fan_gpio_power_init(void)
{
	int ret = 0;
	struct pwm_fan_data* data = hw_pwm_fan_get_data();

	if (!data)
		return -EINVAL;

	ret = gpio_request(data->gpio_power, "fan_gpio_power");
	if (ret) {
		hwlog_err("gpio request fail\n");
		return ret;
	}

	return hw_fan_power_switch(true);
}
static int fan_boot_init(struct platform_device *pdev)
{
	int ret;
	struct pwm_fan_data* data = hw_pwm_fan_get_data();

	if (!data)
		return -EINVAL;

	ret = hw_pwm_fan_gpio_power_init();
	if (ret)
		return ret;
	/* pwm pinctrl init */
	ret = pinctrl_cmds_tx(pdev, pwm_pinctrl_init_cmds,
		ARRAY_SIZE(pwm_pinctrl_init_cmds));
	if (ret) {
		dev_err(&pdev->dev, "Init pwm pinctrl failed\n");
		return -ENXIO;
	}

	/* pwm input monitor enable */
	ret = hw_pwm_input_switch(true);
	if (ret)
		return -ENXIO;

	hw_fan_switch(true);
	msleep(FAN_SLEEP_TIME);
	hw_fan_dest_speed(data, FAN_BOOT_INIT_SPEED);
	hw_fan_power_switch(false);
	return ret;
}

static void do_all_speed_regulate(struct pwm_fan_data *data)
{
	int speed = data->cur_speed;
	int pwm_duty = data->pwmout_duty;

	if (speed < data->dest_speed) {
		while (pwm_duty < PWM_OUT_PRECISION) {
			pwm_duty++;
			pwm_out_config(data, pwm_duty);
			msleep(REGULATING_SPEED_TIMEOUT);
			speed = get_fan_speed(data);
			data->cur_speed = speed;
			if ((speed >= (data->dest_speed - FINE_TUNING_SPEED_RANGE)) ||
				data->dest_changed)
				break;
		}
	} else {
		while (pwm_duty > PWM_OUT_DUTY_START_WORK) {
			pwm_duty--;
			pwm_out_config(data, pwm_duty);
			msleep(REGULATING_SPEED_TIMEOUT);
			speed = get_fan_speed(data);
			data->cur_speed = speed;
			if ((speed <= (data->dest_speed + FINE_TUNING_SPEED_RANGE)) ||
				data->dest_changed)
				break;
		}
	}
	return;
}

static void speed_regulate_work(struct work_struct *work)
{
	int cur_speed;
	struct pwm_fan_data *data =
	    container_of(work, struct pwm_fan_data, speed_regulate_work.work);

	if (!data || (data->dest_speed < FAN_MIN_SPEED))
		goto end_3;

	cur_speed = get_fan_speed(data);
	data->cur_speed = cur_speed;
	if (data->last_speed == cur_speed) {
		data->same_count_val++;
		if (data->same_count_val >= FAN_STATUS_CHECK_COUNT) {
			if (cur_speed == -EINVAL)
				data->fan_status = FAN_ABNORMAL_WORKING;
			else if (cur_speed >= 0)
				data->fan_status = FAN_DISCONNCET;
		}
	} else {
		data->last_speed = cur_speed;
		data->same_count_val = 0;
		data->fan_status = FAN_WORKING_NORMAL;
	}

	if (data->dest_changed) {
		mutex_lock(&data->lock);
		data->dest_changed = false;
		mutex_unlock(&data->lock);
		goto end_1;
	}

	if (abs(data->dest_speed - cur_speed) >= REGULATE_SPEED_RANGE) {
		data->retry++;
		if (data->retry == RETRY_MAX_COUNT) {
			do_all_speed_regulate(data);
			goto end_1;
		}
		goto end_2;
	}

end_1:
	data->retry = 0;
	schedule_delayed_work(&data->speed_regulate_work,
		msecs_to_jiffies(REGULATING_WORK_TIMEOUT));
	return;
end_2:
	schedule_delayed_work(&data->speed_regulate_work,
		msecs_to_jiffies(RETRY_WORK_TIMEOUT));
	return;
end_3:
	data->retry = 0;
	schedule_delayed_work(&data->speed_regulate_work,
		msecs_to_jiffies(SPEED_CHANGED_WORK_TIMEOUT));
	return;
}

static int hw_pwm_fan_probe(struct platform_device *pdev)
{
	int ret;
	struct pwm_fan_data *data = NULL;
	struct device_node *np = NULL;

	if (!pdev) {
		hwlog_err("pdev is NULL\n");
		return -EINVAL;
	}

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		hwlog_err("pwm_fan_data is NULL\n");
		return -EINVAL;
	}

	data->pwm_pdev = pdev;
	data->pwm_on = 0;
	data->pwmout_speed = 0;
	data->fan_status = FAN_WORKING_NORMAL;
	data->power_enable = false;
	mutex_init(&data->lock);
	platform_set_drvdata(pdev, data);
	g_hw_pwm_fan_data = data;

	ret = hw_pwm_fan_parse_dt(pdev);
	if (ret)
		goto parse_dt_fail;

	ret = fan_boot_init(pdev);
	if (ret < 0) {
		hwlog_err("boot init fail\n");
		goto fail_exit;
	}

	fan_sysfs_create(pdev);
	INIT_DELAYED_WORK(&data->speed_regulate_work, speed_regulate_work);
	mod_delayed_work(system_wq, &data->speed_regulate_work, msecs_to_jiffies(0));
	return 0;

fail_exit:
	gpio_free(data->gpio_power);
parse_dt_fail:
	mutex_destroy(&data->lock);
	kfree(data);
	g_hw_pwm_fan_data = NULL;
	return ret;
}

static int hw_pwm_fan_remove(struct platform_device *pdev)
{
	int i, ret;
	struct clk *clk_tmp = NULL;
	struct pwm_fan_data *data = platform_get_drvdata(pdev);
	if (!data)
		return -EINVAL;

	cancel_delayed_work(&data->speed_regulate_work);
	mutex_destroy(&data->lock);
	for (i = 0; i < ARRAY_SIZE(hw_fan_dev_attrs); i++) {
		if (!hw_fan_dev_attrs[i])
			break;
		device_remove_file(&pdev->dev, hw_fan_dev_attrs[i]);
	}
	ret = pinctrl_cmds_tx(pdev, pwm_pinctrl_finit_cmds,
		ARRAY_SIZE(pwm_pinctrl_finit_cmds));

	clk_tmp = data->pwm_clk;
	if (clk_tmp) {
		clk_put(clk_tmp);
		clk_tmp = NULL;
	}

	clk_tmp = data->blpwm_clk;
	if (clk_tmp) {
		clk_put(clk_tmp);
		clk_tmp = NULL;
	}

	if (data->gpio_power)
		gpio_free(data->gpio_power);

	kfree(data);
	g_hw_pwm_fan_data = NULL;
	return ret;
}

#ifdef CONFIG_PM
static int hw_pwm_fan_suspend(struct platform_device *pdev, pm_message_t state)
{
	hwlog_info("suspend --\n");
	return hw_pwm_input_switch(false);
}

static int hw_pwm_fan_resume(struct platform_device *pdev)
{
	hwlog_info("resume --\n");
	return hw_pwm_input_switch(true);
}
#endif /* CONFIG_PM */

static const struct of_device_id hw_pwm_fan_match_table[] = {
	{
		.compatible = DTS_COMP_FAN_NAME,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, hw_pwm_fan_match_table);

static struct platform_driver hw_pwm_fan_driver = {
	.probe = hw_pwm_fan_probe,
	.remove = hw_pwm_fan_remove,
#ifdef CONFIG_PM
	.suspend = hw_pwm_fan_suspend,
	.resume = hw_pwm_fan_resume,
#endif /* CONFIG_PM */
	.shutdown = NULL,
	.driver = {
		.name = "hw_pwm_fan",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hw_pwm_fan_match_table),
	},
};

static int __init hw_pwm_fan_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&hw_pwm_fan_driver);
	if (ret) {
		hwlog_info("platform_driver_register failed, error=%d\n", ret);
		return ret;
	}

	return ret;
}

static void __exit hw_pwm_fan_exit(void)
{
	platform_driver_unregister(&hw_pwm_fan_driver);
}

module_init(hw_pwm_fan_init);
module_exit(hw_pwm_fan_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Pwm Fan Driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
