/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: provide gpio debug function interfaces.
 * Create: 2022-3-27
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/gpio.h>

int gpio_input_test(unsigned gpio)
{
	return gpiod_direction_input(gpio_to_desc(gpio));
}

int gpio_output_test(unsigned gpio, int value)
{
	return gpiod_direction_output(gpio_to_desc(gpio), value);
}