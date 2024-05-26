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
#ifndef HISI_DPP_GAMMA_TEST_H
#define HISI_DPP_GAMMA_TEST_H

#define DPU_DPP_CH0_DEGAMMA_LUT_OFFSET (0xB9000)
#define U_R_COEF (0x0000)
#define U_G_COEF (0x0400)
#define U_B_COEF (0x0800)
#define DPU_DPP_CH0_GAMA0_LUT_OFFSET (0xBA000)
#define DPU_DPP_CH0_GAMA1_LUT_OFFSET (0xBB000)
#define GAMA_ROI_OFFSET (0x4)
#define GAMA_ROI0_START_OFFSET (0x8)
#define GAMA_ROI0_END_OFFSET (0xc)
#define GAMA_ROI1_START_OFFSET (0x10)
#define GAMA_ROI1_END_OFFSET (0x14)
#define IGM_LUT_LEN ((uint32_t)257)
#define GAMMA_LUT_LEN ((uint32_t)257)

void dpp_effect_gamma_test(char __iomem *dpu_base);
void dpp_effect_degamma_test(char __iomem *dpu_base);
#endif