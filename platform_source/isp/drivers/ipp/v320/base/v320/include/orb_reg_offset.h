// ********************************************************
// Copyright     :  Copyright (C) 2019, HIPP Technologies Co. Ltd.
// File name     :  orb_reg_offset.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Version       :  1.0
// Date          :  2018/12/28
// Description   :  The description of xxx project
// Others        :  Generated automatically by nManager V4.1
// *********************************************************

#ifndef __ORB_REG_OFFSET_H__
#define __ORB_REG_OFFSET_H__


/* **************************************************************************** */
//                      ISP_V300 ORB Registers' Definitions
/* **************************************************************************** */

#define ORB_ORB_CFG_REG           0x0    /* ORB Top Config */
#define ORB_IMAGE_SIZE_REG         0x4    /* Image size config */
#define ORB_STAT_RANGE_REG         0x8    /* Stat range for stripe */
#define ORB_BLOCK_SIZE_CFG_INV_REG 0xC    /* BLOCK Size_INV */
#define ORB_PYRAMID_INC_CFG_REG    0x10   /* PYRAMID Scaler INC Config */
#define ORB_PYRAMID_VCROP_CFGB_REG 0x14   /* CROP Config bottom */
#define ORB_PYRAMID_VCROP_CFGT_REG 0x18   /* CROP Config top */
#define ORB_PYRAMID_HCROP_CFGR_REG 0x1C   /* CROP Config R */
#define ORB_PYRAMID_HCROP_CFGL_REG 0x20   /* CROP Config L */
#define ORB_GSBLUR_COEF_01_REG     0x30   /* Gauss Coefficients 0 and 1 */
#define ORB_GSBLUR_COEF_23_REG     0x34   /* Gauss Coefficients 2 and 3 */
#define ORB_THRESHOLD_CFG_REG      0x40   /* Threshold for min and initial */
#define ORB_NMS_WIN_CFG_REG        0x44   /* NMS window size */
#define ORB_LAYER0_START_H_REG     0x48   /* Horizontal start on 0 layer */
#define ORB_LAYER0_START_V_REG     0x4C   /* Vertical start on 0 layer */
#define ORB_BLOCK_NUM_CFG_REG      0x50   /* BLOCK Number */
#define ORB_BLOCK_SIZE_CFG_REG     0x54   /* BLOCK Size */
#define ORB_OCTREE_CFG_REG         0x58   /* octree limit */
#define ORB_INC_LUT_CFG_REG        0x5C   /* inc_lut configure */
#define ORB_UNDISTORT_CX_REG       0x60   /* CX */
#define ORB_UNDISTORT_CY_REG       0x64   /* CY */
#define ORB_UNDISTORT_FX_REG       0x68   /* FX */
#define ORB_UNDISTORT_FY_REG       0x6C   /* FY */
#define ORB_UNDISTORT_INVFX_REG    0x70   /* INVFX */
#define ORB_UNDISTORT_INVFY_REG    0x74   /* INVFY */
#define ORB_UNDISTORT_FIRST_REG       0x78   /* FIRST */
#define ORB_UNDISTORT_SECOND_REG       0x7C   /* SECOND */
#define ORB_UNDISTORT_THIRD_REG       0x80   /* THIRD */
#define ORB_UNDISTORT_P1_REG       0x84   /* P1 */
#define ORB_UNDISTORT_P2_REG       0x88   /* P2 */
#define ORB_CVDR_RD_CFG_REG        0x90   /* Video port read Configuration. */
#define ORB_CVDR_RD_LWG_REG        0x94   /* Line width generation. */
#define ORB_CVDR_RD_FHG_REG        0x98   /* Frame height generation. */
#define ORB_CVDR_RD_AXI_FS_REG     0x9C   /* AXI frame start. */
#define ORB_CVDR_RD_AXI_LINE_REG   0xA0   /* Line Wrap definition. */
#define ORB_CVDR_RD_IF_CFG_REG 0xA4   /* prefetch or reset or stall capability */
#define ORB_PRE_CVDR_RD_FHG_REG    0xA8   /* Frame height generation. */
#define ORB_CVDR_WR_CFG_REG        0xB0   /* Video port write Configuration. */
#define ORB_CVDR_WR_AXI_FS_REG     0xB4   /* AXI address Frame start. */
#define ORB_CVDR_WR_AXI_LINE_REG   0xB8   /* AXI line wrap and line stride. */
#define ORB_CVDR_WR_IF_CFG_REG 0xBC   /* prefetch or reset or stall capability */
#define ORB_PRE_CVDR_WR_AXI_FS_REG 0xC0   /* AXI address */
#define ORB_DEBUG_0_REG            0xD0   /* debug signal 0 */
#define ORB_DEBUG_1_REG            0xD4   /* debug signal 1 */
#define ORB_BRIEF_PATTERN_0_REG    0x100  /* pattern */
#define ORB_SCORE_THESHOLD_0_REG   0x500  /* score threshold of each block */
#define ORB_KPT_NUMBER_0_REG       0x700  /* Feature number in each block */
#define ORB_TOTAL_KPT_NUM_REG      0x9EC  /* Total kpt number */
#define ORB_KPNUM_NMS_REG          0x9F0  /* Key point number  after NMS */
#define ORB_GRIDSTAT_0_REG         0xA00  /* grid stat */
#endif // __ORB_REG_OFFSET_H__
