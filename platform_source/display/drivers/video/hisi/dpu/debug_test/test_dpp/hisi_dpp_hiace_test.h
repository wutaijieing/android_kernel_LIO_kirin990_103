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
#include <linux/fb.h>
#include <linux/workqueue.h>

#ifndef HISI_DPP_HIACE_TEST_H
#define HISI_DPP_HIACE_TEST_H
#define DPU_DPP_CH0_OFFSET (0xB4000)
#define HI_ACE_OFFSET (0xC9000)
#define HI_ACE_GAMMA_AB_SHADOW_OFFSET (0x0018)
#define HI_ACE_GAMMA_AB_WORK_OFFSET (0x001c)
#define HI_ACE_GLOBAL_HIST_AB_SHADOW_OFFSET (0x0020)
#define HI_ACE_GLOBAL_HIST_AB_WORK_OFFSET (0x0024)
#define HI_ACE_GAMMA_R_OFFSET (0x0110)
#define HI_ACE_GAMMA_EN_HV_R_OFFSET (0x0110)
#define HI_ACE_GAMMA_VH_R_OFFSET (0x0114)
#define FNA_VALID_OFFSET (0x210)

#define DPP_CH0_HI_ACE_DETAIL_WEIGHT_OFFSET (0xBD500)
#define DPP_CH0_HI_ACE_LOG_LUM (0xBD600)
#define DPP_CH0_HI_ACE_LUMA_GAMMA (0xBD700)

#define XBLOCKNUM (6)
#define YBLOCKNUM (6)
#define HIACE_LHIST_RANK (16)
#define HIACE_GAMMA_RANK (8)
#define HIACE_GHIST_RANK (32)
#define HIACE_FNA_RANK (1)
#define HIACE_SKIN_COUNT_RANK (36)
#define HIACE_SKIN_COUNT (0XBDB00)
#define DPU_SAT_GLOBAL_HIST_LUT_ADDR (0x0180)

#define CE_SIZE_HIST                                                                                            \
	(HIACE_GHIST_RANK * 2 + YBLOCKNUM * XBLOCKNUM * HIACE_LHIST_RANK + YBLOCKNUM * XBLOCKNUM * HIACE_FNA_RANK + \
	HIACE_SKIN_COUNT_RANK + 1)
#define CE_SIZE_LUT (YBLOCKNUM * XBLOCKNUM * HIACE_GAMMA_RANK)
#define DETAIL_WEIGHT_SIZE (9)
#define LOG_LUM_EOTF_LUT_SIZE (32)
#define LUMA_GAMA_LUT_SIZE (21)
#define DPU_GLOBAL_HIST_LUT_ADDR (0x0080)
#define HIACE_DETAIL_WEIGHT_TABLE_LEN 33
#define HIACE_LOGLUM_EOTF_TABLE_LEN 63
#define HIACE_LUMA_GAMA_TABLE_LEN 63
#define DPE_GAMMA_AB_SHADOW (0x0018)
#define DPE_GAMMA_AB_WORK (0x001c)
#define DPE_GAMMA_WRITE_EN (0x0108)
#define DPE_GAMMA_VH_W (0x010c)

void dpp_hiace_test(char __iomem *dpu_base);
void hiace_end_handle_func(struct work_struct *work);
#endif