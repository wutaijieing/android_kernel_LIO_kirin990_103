/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <linux/delay.h>
#include <linux/gpio.h>

#include "hisi_disp_panel.h"
#include "hisi_disp_gadgets.h"

static int gpio_cmds_tx_check_param(struct gpio_desc *cmds, int index)
{
	if (!cmds || !(cmds->label)) {
		disp_pr_err("cmds or cmds->label is NULL! index=%d\n", index);
		return -1;
	}

	if (!gpio_is_valid(*(cmds->gpio))) {
		disp_pr_err("gpio invalid, dtype=%d, lable=%s, gpio=%d!\n",
			cmds->dtype, cmds->label, *(cmds->gpio));
		return -1;
	}

	return 0;
}

int dpu_gpio_cmds_tx(struct gpio_desc *cmds, int cnt)
{
	int ret = 0;
	int i = 0;

	struct gpio_desc *cm = cmds;

	for (i = 0; i < cnt; i++) {
		if (gpio_cmds_tx_check_param(cm, i) != 0)
			return -1;

		switch (cm->dtype) {
		case DTYPE_GPIO_INPUT:
			ret = gpio_direction_input(*(cm->gpio));
			break;
		case DTYPE_GPIO_OUTPUT:
			ret = gpio_direction_output(*(cm->gpio), cm->value);
			break;
		case DTYPE_GPIO_REQUEST:
			ret = gpio_request(*(cm->gpio), cm->label);
			break;
		case DTYPE_GPIO_FREE:
			gpio_free(*(cm->gpio));
			break;
		default:
			disp_pr_err("dtype=%x NOT supported\n", cm->dtype);
			return -1;
		}

		if (ret) {
			disp_pr_err("failed to handle gpio action, dtype=%d, lable=%s, gpio=%d!\n",
					cm->dtype, cm->label, *(cm->gpio));
			return -1;
		}

		if (cm->wait) {
			if (cm->waittype == WAIT_TYPE_US)
				udelay(cm->wait);
			else if (cm->waittype == WAIT_TYPE_MS)
				mdelay(cm->wait);
			else
				mdelay(cm->wait * 1000);  /* ms */
		}

		cm++;
	}

	return 0;
}


