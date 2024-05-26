/*
 * dec_reg.h
 *
 * This is for dec registers definition.
 *
 * Copyright (c) 2019-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef DEC_REG_H
#define DEC_REG_H

#include "dec_base.h"

#define VREG_OFS_DOMN_CLK_CNT 0x0
#define VREG_OFS_DOMN_CLK_BSP 0x4
#define VREG_OFS_ROM_VRG_EMA  0x10
#define VREG_OFS_PXP_CORE_EN  0x14

#define VREG_OFS_BSP_INTR_STATE    0x00
#define VREG_OFS_BSP_INTR_MSKSTATE 0x04
#define VREG_OFS_BSP_INTR_MASK     0x08
#define VREG_OFS_BSP_CUR_REG       0x0C
#define VREG_OFS_PXP_INTR_STATE    0x10
#define VREG_OFS_PXP_INTR_MSKSTATE 0x14
#define VREG_OFS_PXP_INTR_MASK     0x18
#define VREG_OFS_PXP_CUR_REG       0x1C
#define VREG_OFS_VDH_COEFF         0x20
#define VREG_OFS_BSP_CYCLES        0x24
#define VREG_OFS_PXP_CYCLES        0x28

#define VREG_OFS_DEC_STORE         0x50

#define VREG_OFS_BETWEEN_REG 0x20

#define VREG_OFS_DEC_START     0x000
#define VREG_OFS_INFO_STATE    0x004
#define VREG_OFS_AVM_ADDR_CNT  0x008
#define VREG_OFS_PROCESS_STATE 0x00C
#define VREG_OFS_PXP_START     0x010

#define VREG_OFS_MDMA_START      (MDMA_BASE + 0x00)
#define VREG_OFS_MDMA_AVM_ADDR   (MDMA_BASE + 0x04)
#define VREG_OFS_MDMA_SAFE_MMU   (MDMA_BASE + 0x08)
#define VREG_OFS_MDMA_UNID       (MDMA_BASE + 0x0C)
#define VREG_OFS_MDMA_CANCEL     (MDMA_BASE + 0x10)
#define VREG_OFS_MDMA_INTR_MASK  (MDMA_BASE + 0x14)
#define VREG_OFS_MDMA_INTR_CLR   (MDMA_BASE + 0x18)
#define VREG_OFS_MDMA_INTR_STATE (MDMA_BASE + 0x1C)

typedef struct {
	uint32_t frame_indicator : 16;
	uint32_t reserved : 1;
	uint32_t intrst_vdh_dec_over : 1;
	uint32_t reserved1 : 14;
} vreg_bsp_intr_state;

typedef struct {
	uint32_t frame_indicator : 16;
	uint32_t reserved : 1;
	uint32_t intrst_vdh_dec_over : 1;
	uint32_t intrst_vdh_dec_err : 1;
	uint32_t intrst_vdh_dec_part : 1;
	uint32_t intrmskst_dss2vdh_level_int : 1;
	uint32_t reserved1 : 3;
	uint32_t pxp_cur_reg : 3;
	uint32_t reserved2 : 5;
} vreg_pxp_intr_state;

typedef struct {
	uint32_t reserved : 3;
	uint32_t bsp_interrupt_en : 1;
	uint32_t low_latency_err_rsten : 1;
	uint32_t vp9av1_frame_parallel : 1;
	uint32_t mfde_mmu_en : 1;
	uint32_t pxp_finish_rst_bsp_en : 1;
	uint32_t reserved1 : 1;
	uint32_t wireless_lowdelay_en : 1;
	uint32_t int_120hz_sel : 2;
	uint32_t reserved2 : 20;
} vreg_coeff;

typedef struct {
	uint32_t vdh_safe_flag : 1;
	uint32_t use_bsp_num : 3;
	uint32_t reserved: 1;
	uint32_t vdh_mdma_sel : 1;
	uint32_t int_report_en : 1;
	uint32_t av1_intrabc : 1;
	uint32_t bsp_internal_ram : 1;
	uint32_t pxp_internal_ram : 1;
	uint32_t line_num_en : 1;
	uint32_t slice_low_dly : 1;
	uint32_t frame_indicator : 16;
	uint32_t dec_cancel : 1;
	uint32_t pxp_state : 1;
	uint32_t bsp_state : 1;
	uint32_t vdec_busy_state : 1;
} vreg_info_state;

typedef struct {
	uint32_t proc_state : 2;
	uint32_t reserved1 : 2;
	uint32_t set_err : 1;
	uint32_t enhance_layer_valid_num : 3;
	uint32_t vid_std : 5;
	uint32_t reserved3 : 3;
	uint32_t cur_pic_priority : 4;
	uint32_t filmgrain_caltab_en : 1;
	uint32_t pxp_core_len : 3;
	uint32_t reserved4 : 8;
} vreg_proc_state;

#endif
