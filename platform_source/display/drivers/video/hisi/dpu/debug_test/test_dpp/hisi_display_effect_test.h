/* Copyright (c) 2013-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#ifndef HISI_DISPLAY_EFFECT_TEST_H
#define HISI_DISPLAY_EFFECT_TEST_H
#include <linux/types.h>
#define DPU_DPP_CH0_XCC_OFFSET (0xB5750)
#define DPU_DPP_CH0_NL_XCC_OFFSET (0xB5700)

void dpp_effect_test(char __iomem *dpu_base);
#endif
