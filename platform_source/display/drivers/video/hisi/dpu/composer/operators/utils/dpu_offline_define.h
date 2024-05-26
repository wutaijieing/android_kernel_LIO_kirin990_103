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

#ifndef _DPU_OFFLINE_DEFINE_H_
#define _DPU_OFFLINE_DEFINE_H_

#include <linux/semaphore.h>
#include <soc_dte_define.h>
#include "hisi_disp_debug.h"

// SCL
#define SCF_INPUT_OV 16
#define WDMA_ROT_LINEBUF 512
#define AFBCE_LINEBUF 480
#define HFBCE_LINEBUF 512
#ifdef CONFIG_DKMD_DPU_V720
#define HEBCE_LINEBUF 4096
#else
#define HEBCE_LINEBUF 512
#endif
#define HEBCE_ROT_LINEBUF 512
#define SCF_INC_FACTOR  (1 << 18) /* 262144 */

/* MACROS */
#define SCF_MIN_INPUT   (16) //SCF min input pix 16x16
#define SCF_MIN_OUTPUT  (16) //SCF min output pix 16x16

#define RCHN_V2_SCF_LINE_BUF 512
#define SHARPNESS_LINE_BUF 2560
#define WCH_SCF_LINE_BUF 512

#define MAX_OFFLINE_SCF 4
#define MAX_OFFLINE_LAYER_NUMBER 8
#define BLOCK_SIZE_INVALID 0xFFFF
#define BLOCK_WIDTH_MAX 8192
#define BLOCK_WIDTH_MIN 256
#define HISI_DPU_CMDLIST_BLOCK_MAX 28
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#define SCF_LINE_BUF (2560)
#define DPU_WCH0_OFFSET (0x00015000)
#define DPU_WCH1_OFFSET (0x0006D000)
#define DPU_WCH2_OFFSET (0x00071000)

/* block */
#define DPU_CMDLIST_BLOCK_MAX 28

/* align */
#ifndef ALIGN_DOWN
#define ALIGN_DOWN(val, al)  ((val) & ~((typeof(val))(al)-1))
#endif
#ifndef ALIGN_UP
#define ALIGN_UP(val, al)    (((val) + ((al)-1)) & ~((typeof(val))(al)-1))
#endif

enum dpu_wchn_idx {
	DPU_WCHN_W0 = 0,
	DPU_WCHN_W1,
	DPU_WCHN_W2,

	DPU_WCHN_MAX_DEFINE,
};

enum dpu_wchn_module {
	WCH_MODULE_DMA,
	WCH_MODULE_WLT,
	WCH_MODULE_DFC,
	WCH_MODULE_WDFC_DITHER,
	WCH_MODULE_SCF,
	WCH_MODULE_PCSC,
	WCH_MODULE_POST_CLIP,
	WCH_MODULE_CSC,
	WCH_MODULE_ROT,
	WCH_MODULE_WRAP_CTL,
	WCH_MODULE_SCL_LUT,
	WCH_MODULE_CHN_MAX,
};

enum dpu_addr {
	DPU_ADDR_PLANE0 = 0,
	DPU_ADDR_PLANE1,
	DPU_ADDR_PLANE2,
};

enum dpu_transform {
	DPU_TRANSFORM_NOP = 0x0,
	DPU_TRANSFORM_FLIP_H = 0x02,
	DPU_TRANSFORM_ROT = 0x03,
	DPU_TRANSFORM_FLIP_V = 0x04,
};

static uint32_t g_dpu_wch_base[DPU_WCHN_MAX_DEFINE] = {
	DPU_WCH0_OFFSET,
	DPU_WCH1_OFFSET,
	DPU_WCH2_OFFSET
};

/*******************************************************************************
** dpu offline module reg
*/

struct dpu_wch_wdma {
	DPU_WCH_DMA_CTRL0_UNION dma_ctrl0;
	DPU_WCH_DMA_CTRL1_UNION dma_ctrl1;
	DPU_WCH_DMA_IMG_SIZE_UNION dma_img_size;
	DPU_WCH_DMA_PTR0_UNION dma_ptr0;
	DPU_WCH_DMA_STRIDE0_UNION dma_stride0;
	DPU_WCH_DMA_PTR1_UNION dma_ptr1;
	DPU_WCH_DMA_STRIDE1_UNION dma_stride1;
	DPU_WCH_DMA_PTR2_UNION dma_ptr2;
	DPU_WCH_DMA_STRIDE2_UNION dma_stride2;
	DPU_WCH_DMA_PTR3_UNION dma_ptr3;
	DPU_WCH_DMA_STRIDE3_UNION dma_stride3;
	DPU_WCH_DMA_CMP_CTRL_UNION dma_cmp_ctrl;

	DPU_WCH_REG_DEFAULT_UNION reg_default;
	DPU_WCH_CH_CTL_UNION ch_ctl;
	DPU_WCH_ROT_SIZE_UNION rot_size;

	/* other data */
};

struct dpu_wch_wlt {
	DPU_WCH_WTL_CTRL_UNION wlt_ctrl;
	DPU_WCH_WLT_LINE_THRESHOLD1_UNION wlt_line_threshold1;
	DPU_WCH_WLT_LINE_THRESHOLD3_UNION wlt_line_threshold3;
	DPU_WCH_WLT_BASE_LINE_OFFSET_UNION wlt_base_line_offset;
	DPU_WCH_WLT_DMA_ADDR_UNION wlt_dma_addr;
	DPU_WCH_WLT_DMA_QOS_UNION wlt_dma_qos;
	DPU_WCH_WLT_C_LINE_NUM_UNION wlt_c_line_num; // soc reserved, not be used
	DPU_WCH_WLT_C_SLICE_NUM_UNION wlt_c_slice_num; // soc reserved, not be used

    /* other data */
};

struct dpu_wdfc {
	DPU_WCH_DFC_DISP_SIZE_UNION disp_size;
	DPU_WCH_DFC_PIX_IN_NUM_UNION pix_in_num;
	DPU_WCH_DFC_GLB_ALPHA01_UNION glb_alpha01;
	DPU_WCH_DFC_DISP_FMT_UNION disp_fmt;
	DPU_WCH_DFC_CLIP_CTL_HRZ_UNION clip_ctl_hrz;
	DPU_WCH_DFC_CLIP_CTL_VRZ_UNION clip_ctl_vrz;
	DPU_WCH_DFC_CTL_CLIP_EN_UNION ctl_clip_en;
	DPU_WCH_DFC_ICG_MODULE_UNION icg_module;
	DPU_WCH_DFC_DITHER_ENABLE_UNION dither_enable;
	DPU_WCH_DFC_PADDING_CTL_UNION padding_ctl;
	DPU_WCH_DFC_GLB_ALPHA23_UNION glb_alpha23;
	DPU_WCH_BITEXT_CTL_UNION bitext_ctl;
};

struct dpu_csc {
	DPU_WCH_CSC_IDC0_UNION idc0;
	DPU_WCH_CSC_IDC2_UNION idc2;
	DPU_WCH_CSC_ODC0_UNION odc0;
	DPU_WCH_CSC_ODC2_UNION odc2;
	DPU_WCH_CSC_P00_UNION p00;
	DPU_WCH_CSC_P01_UNION p01;
	DPU_WCH_CSC_P02_UNION p02;
	DPU_WCH_CSC_P10_UNION p10;
	DPU_WCH_CSC_P11_UNION p11;
	DPU_WCH_CSC_P12_UNION p12;
	DPU_WCH_CSC_P20_UNION p20;
	DPU_WCH_CSC_P21_UNION p21;
	DPU_WCH_CSC_P22_UNION p22;
	DPU_WCH_CSC_ICG_MODULE_UNION icg_module;
};

struct dpu_scf {
	DPU_WCH_SCF_EN_HSCL_STR_UNION en_hscl_str;
	DPU_WCH_SCF_EN_VSCL_STR_UNION en_vscl_str;
	DPU_WCH_SCF_H_V_ORDER_UNION h_v_order;
	DPU_WCH_SCF_CH_CORE_GT_UNION ch_core_gt;
	DPU_WCH_SCF_INPUT_WIDTH_HEIGHT_UNION input_width_height;
	DPU_WCH_SCF_OUTPUT_WIDTH_HEIGHT_UNION output_width_height;
	DPU_WCH_SCF_COEF_MEM_CTRL_UNION coef_mem_ctrl;
	DPU_WCH_SCF_EN_HSCL_UNION en_hscl;
	DPU_WCH_SCF_EN_VSCL_UNION en_vscl;
	DPU_WCH_SCF_ACC_HSCL_UNION acc_hscl;
	DPU_WCH_SCF_ACC_HSCL1_UNION acc_hscl1;
	DPU_WCH_SCF_INC_HSCL_UNION inc_hscl;
	DPU_WCH_SCF_ACC_VSCL_UNION acc_vscl;
	DPU_WCH_SCF_ACC_VSCL1_UNION acc_vscl1;
	DPU_WCH_SCF_INC_VSCL_UNION inc_vscl;
	DPU_WCH_SCF_EN_NONLINEAR_UNION en_nonlinear;
	DPU_WCH_SCF_EN_MMP_UNION en_mmp;
	DPU_WCH_SCF_DB_H0_UNION db_h0;
	DPU_WCH_SCF_DB_H1_UNION db_h1;
	DPU_WCH_SCF_DB_V0_UNION db_v0;
	DPU_WCH_SCF_DB_V1_UNION db_v1;
	DPU_WCH_SCF_LB_MEM_CTRL_UNION lb_mem_ctrl;
};

struct dpu_wdfc_dither {
	DPU_WCH_DITHER_CTL1_UNION ctl1;
	DPU_WCH_DITHER_TRI_THD10_UNION tri_thd10;
	DPU_WCH_DITHER_TRI_THD10_UNI_UNION tri_thd10_uni;
	DPU_WCH_BAYER_CTL_UNION bayer_ctl;
	DPU_WCH_BAYER_MATRIX_PART1_UNION bayer_matrix_part1;
	DPU_WCH_BAYER_MATRIX_PART0_UNION bayer_matrix_part0;
};

struct dpu_post_clip {
	DPU_WCH_POST_CLIP_DISP_SIZE_UNION disp_size;
	DPU_WCH_POST_CLIP_CTL_HRZ_UNION clip_ctl_hrz;
	DPU_WCH_POST_CLIP_CTL_VRZ_UNION clip_ctl_vrz;
	DPU_WCH_POST_CLIP_EN_UNION ctl_clip_en;
};

struct dpu_wrap_ctl {
};


enum wb_type {
	WB_TYPE_WCH0,
	WB_TYPE_WCH1,
	WB_TYPE_WCH2,
	WB_TYPE_WCH0_WCH1,

	WB_TYPE_MAX,
};

struct offline_isr_info {
	struct semaphore wb_common_sem;  /* for primary offline & copybit */
	struct semaphore wb_sem[WB_TYPE_MAX];
	wait_queue_head_t wb_wq[WB_TYPE_MAX];
	uint32_t wb_done[WB_TYPE_MAX];
	uint32_t wb_flag[WB_TYPE_MAX];
};

/* TODO: change each wch has operators not each operator has wch */
struct dpu_offline_module {
	char __iomem *wch_base[DPU_WCHN_MAX_DEFINE];

	struct dpu_wch_wdma wdma[DPU_WCHN_MAX_DEFINE];
	struct dpu_wch_wlt wlt[DPU_WCHN_MAX_DEFINE];
	struct dpu_wdfc dfc[DPU_WCHN_MAX_DEFINE];
	struct dpu_csc csc[DPU_WCHN_MAX_DEFINE];
	struct dpu_csc pcsc[DPU_WCHN_MAX_DEFINE];
	struct dpu_scf scf[DPU_WCHN_MAX_DEFINE];
	struct dpu_wdfc_dither dither[DPU_WCHN_MAX_DEFINE];
	struct dpu_post_clip clip[DPU_WCHN_MAX_DEFINE];
	struct dpu_wrap_ctl wrap_ctl[DPU_WCHN_MAX_DEFINE];

	uint8_t dma_used[DPU_WCHN_MAX_DEFINE];
	uint8_t wlt_used[DPU_WCHN_MAX_DEFINE];
	uint8_t dfc_used[DPU_WCHN_MAX_DEFINE];
	uint8_t wdfc_dither_used[DPU_WCHN_MAX_DEFINE];
	uint8_t scf_used[DPU_WCHN_MAX_DEFINE];
	uint8_t pcsc_used[DPU_WCHN_MAX_DEFINE];
	uint8_t post_clip_used[DPU_WCHN_MAX_DEFINE];
	uint8_t csc_used[DPU_WCHN_MAX_DEFINE];
	uint8_t rot_used[DPU_WCHN_MAX_DEFINE];
	uint8_t wrap_ctl_used[DPU_WCHN_MAX_DEFINE];

	uint8_t has_wlt[DPU_WCHN_MAX_DEFINE];
	uint8_t has_scf[DPU_WCHN_MAX_DEFINE];
	uint8_t has_scf_lut[DPU_WCHN_MAX_DEFINE];
	uint8_t has_pcsc[DPU_WCHN_MAX_DEFINE];
	uint8_t has_post_clip[DPU_WCHN_MAX_DEFINE];
	uint8_t has_rot[DPU_WCHN_MAX_DEFINE];
	/* other data */
};

#endif
