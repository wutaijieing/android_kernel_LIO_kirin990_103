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

#ifndef HISI_DPP_XCC_TEST_H
#define HISI_DPP_XCC_TEST_H

#define DPU_DPP_XCC_EN_OFFSET (0x30)
#define DPU_DPP_NLXCC_EN_OFFSET (0x30)
#define DPU_DPP_NLXCC_DUAL_PXL_EN_OFFSET (0x34)
#define DPU_DPP_NLXCC_SIZE_OFFSET (0x3C)
#define DPU_DPP_NLXCC_ROI_START_OFFSET (0x40)
#define DPU_DPP_NLXCC_ROI_END_OFFSET (0x44)
#define XCC_COEF_LEN 12

#define DPU_DPP_XCC_SIZE_OFFSET (0x34)
#define DPU_DPP_XCC_ROI_START_OFFSET (0x38)
#define DPU_DPP_XCC_ROI_END_OFFSET (0x3c)

#define XCC_COEF_00 (0x000)
#define XCC_COEF_01 (0x004)
#define XCC_COEF_02 (0x008)
#define XCC_COEF_03 (0x00C)
#define XCC_COEF_10 (0x010)
#define XCC_COEF_11 (0x014)
#define XCC_COEF_12 (0x018)
#define XCC_COEF_13 (0x01C)
#define XCC_COEF_20 (0x020)
#define XCC_COEF_21 (0x024)
#define XCC_COEF_22 (0x028)
#define XCC_COEF_23 (0x02C)

void dpp_effect_xcc_test(char __iomem *xcc_base);
#endif