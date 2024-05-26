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
#ifndef HISI_DPP_GMP_TEST_H
#define HISI_DPP_GMP_TEST_H

#define DPU_DPP_CH0_GMP_OFFSET (0xB49A0)
#define DPU_DPP_CH0_GMP_LUT_OFFSET (0xBF000)
#define DPU_DPP_CH1_GMP_OFFSET (0xDD9A0)
#define DPU_DPP_CH1_GMP_LUT_OFFSET (0xE8000)
#define GMP_EN_OFFSET (0x0)
#define GMP_SIZE_OFFSET (0x1C)
#define GMP_ROI_START_OFFSET (0x20)
#define GMP_ROI_END_OFFSET (0x24)
#define GMP_COFE_CNT 4913  // 17*17*17

void dpp_effect_gmp_test(char __iomem *dpu_base);
#endif