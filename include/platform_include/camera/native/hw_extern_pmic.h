/*
 * sensor_common.h
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
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
#ifndef _H_HW_EXTERN_PMIC_
#define _H_HW_EXTERN_PMIC_
int hw_extern_pmic_config(int index, int voltage, int enable);
int hw_extern_pmic_query_state(int index, int *state);
#endif
