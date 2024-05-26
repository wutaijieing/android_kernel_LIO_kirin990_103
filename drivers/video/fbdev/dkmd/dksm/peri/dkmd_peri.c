/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include "dkmd_log.h"
#include "peri/dkmd_peri.h"

struct peri_handle_data {
	int32_t dtype;
	int32_t (*handle_func)(void *desc);
};

static int32_t gpio_handle_input(void *desc)
{
	struct gpio_desc *cmd = (struct gpio_desc *)desc;

	return gpio_direction_input(*(cmd->gpio));
}

static int32_t gpio_handle_output(void *desc)
{
	struct gpio_desc *cmd = (struct gpio_desc *)desc;

	return gpio_direction_output(*(cmd->gpio), cmd->value);
}

static int32_t gpio_handle_request(void *desc)
{
	struct gpio_desc *cmd = (struct gpio_desc *)desc;

	return gpio_request(*(cmd->gpio), cmd->label);
}

static int32_t gpio_handle_free(void *desc)
{
	struct gpio_desc *cmd = (struct gpio_desc *)desc;

	gpio_free(*(cmd->gpio));

	return 0;
}

static struct peri_handle_data g_gpio_handle_data[] = {
	{ DTYPE_GPIO_REQUEST, gpio_handle_request },
	{ DTYPE_GPIO_INPUT, gpio_handle_input },
	{ DTYPE_GPIO_OUTPUT, gpio_handle_output },
	{ DTYPE_GPIO_FREE, gpio_handle_free },
};

static int32_t gpio_cmds_tx_check_param(struct gpio_desc *cmd, int32_t index)
{
	if (!cmd || !cmd->label) {
		dpu_pr_err("cmd or cmd->label is NULL! index=%d\n", index);
		return -1;
	}

	if (!gpio_is_valid(*(cmd->gpio))) {
		dpu_pr_err("gpio invalid, dtype=%d, lable=%s, gpio=%d!\n",
			cmd->dtype, cmd->label, *(cmd->gpio));
		return -1;
	}

	return 0;
}

static void cmds_tx_delay(int32_t waittype, uint32_t wait)
{
	if (wait) {
		if (waittype == WAIT_TYPE_US)
			udelay(wait);
		else if (waittype == WAIT_TYPE_MS)
			mdelay(wait);
		else
			mdelay(wait * 1000);
	}
}

int32_t peri_gpio_cmds_tx(struct gpio_desc *cmds, int32_t cnt)
{
	int32_t ret = 0;
	int32_t i;
	uint32_t j;

	struct gpio_desc *cm = cmds;

	for (i = 0; i < cnt; i++) {
		ret = gpio_cmds_tx_check_param(cm, i);
		if (ret)
			break;

		for (j = 0; j < ARRAY_SIZE(g_gpio_handle_data); j++) {
			if (cm->dtype == g_gpio_handle_data[j].dtype) {
				dpu_pr_info("dtype=%d label: %s, gpio=%d\n", cm->dtype, cm->label, *(cm->gpio));
				ret = g_gpio_handle_data[j].handle_func((void *)cm);
				break;
			}
		}

		if (ret != 0)
			dpu_pr_warn("dtype=%x handle failed, ret=%d, label: %s, gpio=%d\n",
				cm->dtype, ret, cm->label, *(cm->gpio));

		cmds_tx_delay(cm->waittype, cm->wait);
		cm++;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(peri_gpio_cmds_tx);

static int32_t vcc_handle_put(void *desc)
{
	struct vcc_desc *cmd = (struct vcc_desc *)desc;

	devm_regulator_put(*(cmd->regulator));

	return 0;
}

static int32_t vcc_handle_enable(void *desc)
{
	struct vcc_desc *cmd = (struct vcc_desc *)desc;

	return regulator_enable(*(cmd->regulator));
}

static int32_t vcc_handle_disable(void *desc)
{
	struct vcc_desc *cmd = (struct vcc_desc *)desc;

	return regulator_disable(*(cmd->regulator));
}

static int32_t vcc_handle_set_voltage(void *desc)
{
	struct vcc_desc *cmd = (struct vcc_desc *)desc;

	return regulator_set_voltage(*(cmd->regulator), cmd->min_uv, cmd->max_uv);
}

static struct peri_handle_data g_vcc_handle_data[] = {
	{ DTYPE_VCC_PUT, vcc_handle_put },
	{ DTYPE_VCC_ENABLE, vcc_handle_enable },
	{ DTYPE_VCC_DISABLE, vcc_handle_disable },
	{ DTYPE_VCC_SET_VOLTAGE, vcc_handle_set_voltage },
};

int32_t peri_vcc_cmds_tx(struct platform_device *pdev, struct vcc_desc *cmds, int32_t cnt)
{
	int32_t ret = 0;
	int32_t i;
	uint32_t j;
	struct vcc_desc *cm = cmds;

	for (i = 0; i < cnt; i++) {
		if (!cm || !cm->label) {
			dpu_pr_err("cmds or cmds->label is NULL! i=%d\n", i);
			return -1;
		}

		for (j = 0; j < ARRAY_SIZE(g_vcc_handle_data); j++) {
			if (IS_ERR(*(cm->regulator)))
				return -1;
			if (cm->dtype == g_vcc_handle_data[j].dtype) {
				ret = g_vcc_handle_data[j].handle_func((void *)cm);
				dpu_pr_info("dtype=%x handle ret=%d, label: %s\n", cm->dtype, ret, cm->label);
				break;
			}
		}

		if (cm->dtype == DTYPE_VCC_GET) {
			if (!pdev)
				return -1;
			*(cm->regulator) = devm_regulator_get(&pdev->dev, cm->label);
			if (IS_ERR(*(cm->regulator))) {
				dpu_pr_err("failed to get %s regulator!\n", cm->label);
				return -1;
			}
		}

		if (ret != 0) {
			dpu_pr_err("dtype=%x handle failed, ret=%d, label: %s\n", cm->dtype, ret, cm->label);
			return ret;
		}
		cmds_tx_delay(cm->waittype, cm->wait);
		cm++;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(peri_vcc_cmds_tx);

static int32_t pinctrl_get_state_by_mode(struct pinctrl_cmd_desc *cmds)
{
	if (cmds->mode == DTYPE_PINCTRL_STATE_DEFAULT) {
		cmds->pctrl_data->pinctrl_def = pinctrl_lookup_state(cmds->pctrl_data->p, PINCTRL_STATE_DEFAULT);
		if (IS_ERR(cmds->pctrl_data->pinctrl_def)) {
			dpu_pr_err("failed to get pinctrl_def!\n");
			return -1;
		}
	} else if (cmds->mode == DTYPE_PINCTRL_STATE_IDLE) {
		cmds->pctrl_data->pinctrl_idle = pinctrl_lookup_state(cmds->pctrl_data->p, PINCTRL_STATE_IDLE);
		if (IS_ERR(cmds->pctrl_data->pinctrl_idle)) {
			dpu_pr_err("failed to get pinctrl_idle!\n");
			return -1;
		}
	}
	return 0;
}

static int32_t pinctrl_set_state_by_mode(struct pinctrl_cmd_desc *cmds)
{
	if (cmds->mode == DTYPE_PINCTRL_STATE_DEFAULT) {
		if (cmds->pctrl_data->p && cmds->pctrl_data->pinctrl_def)
			return pinctrl_select_state(cmds->pctrl_data->p, cmds->pctrl_data->pinctrl_def);
	} else if (cmds->mode == DTYPE_PINCTRL_STATE_IDLE) {
		if (cmds->pctrl_data->p && cmds->pctrl_data->pinctrl_idle)
			return pinctrl_select_state(cmds->pctrl_data->p, cmds->pctrl_data->pinctrl_idle);
	}

	return 0;
}

int32_t peri_pinctrl_cmds_tx(struct platform_device *pdev, struct pinctrl_cmd_desc *cmds, int32_t cnt)
{
	int32_t ret = -1;
	int32_t i;
	struct pinctrl_cmd_desc *cm = NULL;

	cm = cmds;
	for (i = 0; i < cnt; i++) {
		if (!cm) continue;
		if (cm->dtype == DTYPE_PINCTRL_GET) {
			if (!pdev)
				return -EINVAL;
			cm->pctrl_data->p = devm_pinctrl_get(&pdev->dev);
			ret = IS_ERR(cm->pctrl_data->p);
		} else if (cm->dtype == DTYPE_PINCTRL_STATE_GET) {
			ret = pinctrl_get_state_by_mode(cm);
		} else if (cm->dtype == DTYPE_PINCTRL_SET) {
			ret = pinctrl_set_state_by_mode(cm);
		} else if (cm->dtype == DTYPE_PINCTRL_PUT) {
			if (cm->pctrl_data->p)
				pinctrl_put(cm->pctrl_data->p);
		}

		if (ret != 0) {
			dpu_pr_err("ret:%d err handle dtype: %d index: %d!\n", ret, cm->dtype, i);
			return ret;
		}
		cm++;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(peri_pinctrl_cmds_tx);

int32_t pipeline_next_on(struct platform_device *pdev, struct dkmd_connector_info *pinfo)
{
	int32_t ret = 0;
	struct dkmd_conn_handle_data *pdata = NULL;

	if (!pdev) {
		dpu_pr_err("pdev is NULL!\n");
		return -EINVAL;
	}

	pdata = dev_get_platdata(&pdev->dev);
	if (pdata) {
		if (pinfo)
			ret = pdata->on_func(pinfo);
		else
			ret = pdata->on_func(pdata->conn_info);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(pipeline_next_on);

int32_t pipeline_next_off(struct platform_device *pdev, struct dkmd_connector_info *pinfo)
{
	int32_t ret = 0;
	struct dkmd_conn_handle_data *pdata = NULL;

	if (!pdev) {
		dpu_pr_err("pdev is NULL!\n");
		return -EINVAL;
	}

	pdata = dev_get_platdata(&pdev->dev);
	if (pdata) {
		if (pinfo)
			ret = pdata->off_func(pinfo);
		else
			ret = pdata->off_func(pdata->conn_info);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(pipeline_next_off);

int32_t pipeline_next_ops_handle(struct platform_device *pdev, struct dkmd_connector_info *pinfo,
	uint32_t ops_cmd_id, void *value)
{
	int32_t ret = 0;
	struct dkmd_conn_handle_data *pdata = NULL;

	if (!pdev) {
		dpu_pr_err("pdev is NULL!\n");
		return -EINVAL;
	}

	pdata = dev_get_platdata(&pdev->dev);
	if (pdata) {
		if (pinfo)
			ret = pdata->ops_handle_func(pinfo, ops_cmd_id, value);
		else
			ret = pdata->ops_handle_func(pdata->conn_info, ops_cmd_id, value);
	}
	return ret;
}
EXPORT_SYMBOL_GPL(pipeline_next_ops_handle);

bool check_addr_status_is_valid(const char __iomem* check_addr, uint32_t status,
	uint32_t udelay_time, uint32_t times)
{
	uint32_t i = 0;
	uint32_t val = inp32(check_addr);

	do {
		dpu_pr_info("val: %#x ?= %#x", val, status);
		if ((val & status) == status)
			return true;
		udelay(udelay_time);
		val = inp32(check_addr);
	} while (i++ < times);

	return false;
}
EXPORT_SYMBOL_GPL(check_addr_status_is_valid);

MODULE_LICENSE("GPL");
