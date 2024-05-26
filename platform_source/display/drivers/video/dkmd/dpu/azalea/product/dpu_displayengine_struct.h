/* Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
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

#ifndef DPU_DISPLAYENGINE_STRUCT_H
#define DPU_DISPLAYENGINE_STRUCT_H

#include <linux/types.h>

typedef struct display_engine_dpu_panel_info {
	/* Dither */
	uint8_t dither_effect_support;
	uint8_t dither_noise_mode;
} display_engine_dpu_panel_info_t;

#endif /* DPU_DISPLAYENGINE_STRUCT_H */
