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

#ifndef __DPU_HDR_DEF_H__
#define __DPU_HDR_DEF_H__

#include <linux/types.h>
#include "soc_dte_interface.h"

#define GTM_PQ2SLF_SIZE 8
#define GTM_LUT_PQ2SLF_SIZE 64
#define GTM_LUT_LUMA_SIZE 256
#define GTM_LUT_CHROMA_SIZE 256
#define GTM_LUT_CHROMA0_SIZE 256
#define GTM_LUT_CHROMA1_SIZE 256
#define GTM_LUT_CHROMA2_SIZE 256
#define GTM_LUT_LUMALUT0_SIZE 64
#define GTM_LUT_CHROMALUT0_SIZE 16
#define MITM_LUT_GAMMA_SIZE 64

enum hihdr_flag{
	FLAG_DEFAULT = 0,
	FLAG_BASIC   = 1,
	FLAG_DYNAMIC = 2,
	FLAG_STOP    = 3,
};

typedef struct hihdr_gtm_info {
	int isinslf;
	int input_min_e;
	int output_min_e;

	int ratiolutinclip;
	int ratiolutoutclip;
	int lumaoutbit0;
	int lumaoutbit1;

	int chromaratiolutstep0;
	int chromaratiolutstep1;

	int lumaratiolutstep0;
	int lumaratiolutstep1;

	int chromalutthres;
	int lumalutthres;

	int y_transform[3];

	int chromaoutbit1;
	int chromaoutbit2;

	int chromaratioclip;

	uint32_t pq2slf_step[GTM_PQ2SLF_SIZE];
	uint32_t pq2slf_pos[GTM_PQ2SLF_SIZE];
	uint32_t pq2slf_num[GTM_PQ2SLF_SIZE];

	uint32_t lut_pq2slf[GTM_LUT_PQ2SLF_SIZE];
	uint32_t lut_luma[GTM_LUT_LUMA_SIZE];
	uint32_t lut_chroma[GTM_LUT_CHROMA_SIZE];
	uint32_t lut_chroma0[GTM_LUT_CHROMA0_SIZE];
	uint32_t lut_chroma1[GTM_LUT_CHROMA1_SIZE];
	uint32_t lut_chroma2[GTM_LUT_CHROMA2_SIZE];
	uint32_t lut_lumalut0[GTM_LUT_LUMALUT0_SIZE];
	uint32_t lut_chromalut0[GTM_LUT_CHROMALUT0_SIZE];
} hihdr_gtm_info_t;

typedef struct wcg_mitm_info {
	uint32_t itm_slf_degamma_step1;
	uint32_t itm_slf_degamma_step2;
	uint32_t itm_slf_degamma_pos1;
	uint32_t itm_slf_degamma_pos2;
	uint32_t itm_slf_degamma_pos3;
	uint32_t itm_slf_degamma_pos4;
	uint32_t itm_slf_degamma_num1;
	uint32_t itm_slf_degamma_num2;

	uint32_t itm_slf_gamut_coef00;
	uint32_t itm_slf_gamut_coef01;
	uint32_t itm_slf_gamut_coef02;
	uint32_t itm_slf_gamut_coef10;
	uint32_t itm_slf_gamut_coef11;
	uint32_t itm_slf_gamut_coef12;
	uint32_t itm_slf_gamut_coef20;
	uint32_t itm_slf_gamut_coef21;
	uint32_t itm_slf_gamut_coef22;
	uint32_t itm_slf_gamut_scale;
	uint32_t itm_slf_gamut_clip_max;

	uint32_t itm_slf_gamma_step1;
	uint32_t itm_slf_gamma_step2;
	uint32_t itm_slf_gamma_pos1;
	uint32_t itm_slf_gamma_pos2;
	uint32_t itm_slf_gamma_pos3;
	uint32_t itm_slf_gamma_pos4;
	uint32_t itm_slf_gamma_pos5;
	uint32_t itm_slf_gamma_pos6;
	uint32_t itm_slf_gamma_pos7;
	uint32_t itm_slf_gamma_pos8;
	uint32_t itm_slf_gamma_num1;
	uint32_t itm_slf_gamma_num2;

	uint32_t itm_degamma_lut[MITM_LUT_GAMMA_SIZE];
	uint32_t itm_gamma_lut[MITM_LUT_GAMMA_SIZE];
} wcg_mitm_info_t;

struct hihdr_csc_info {
	DPU_HDR_CSC_IDC0_UNION hdr_csc_idc0_union;
	DPU_HDR_CSC_IDC2_UNION hdr_csc_idc2_union;
	DPU_HDR_CSC_ODC0_UNION hdr_csc_odc0_union;
	DPU_HDR_CSC_ODC2_UNION hdr_csc_odc2_union;
	DPU_HDR_CSC_P00_UNION   hdr_csc_p00_union;
	DPU_HDR_CSC_P01_UNION   hdr_csc_p01_union;
	DPU_HDR_CSC_P02_UNION   hdr_csc_p02_union;
	DPU_HDR_CSC_P10_UNION   hdr_csc_p10_union;
	DPU_HDR_CSC_P11_UNION   hdr_csc_p11_union;
	DPU_HDR_CSC_P12_UNION   hdr_csc_p12_union;
	DPU_HDR_CSC_P20_UNION   hdr_csc_p20_union;
	DPU_HDR_CSC_P21_UNION   hdr_csc_p21_union;
	DPU_HDR_CSC_P22_UNION   hdr_csc_p22_union;
};

typedef struct dpu_hdr_info {
	uint32_t flag;
	uint32_t mean;
	struct hihdr_gtm_info gtm_info;
	struct hihdr_csc_info  csc_info;
} dpu_hdr_info_t;

typedef struct dpu_wcg_info {
	struct wcg_mitm_info mitm_info;
	bool update;
	uint8_t rsv[3];
} dpu_wcg_info_t;


#endif
