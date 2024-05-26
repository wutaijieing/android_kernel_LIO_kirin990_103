/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "kerror.h"

/**
 * @brief error code
 *
 */
static ku_error_code g_error_number = KUE_SUCCESS;

/**
 * @brief set error code
 *
 * @param error
 */
void ku_set_error(ku_error_code error)
{
	g_error_number = error;
}

/**
 * @brief get error code
 *
 * @param error
 */
ku_error_code ku_get_error(void)
{
	return g_error_number;
}
