// ******************************************************************************
// Copyright     :  Copyright (C) 2020, Hisilicon Technologies Co. Ltd.
// File name     :  ar_feature_reg_offset.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Date          :  2020/04/14
// Description   :
// Others        :  Generated automatically by nManager V4.2
// ******************************************************************************

#ifndef __AR_FEATURE_REG_OFFSET_H__
#define __AR_FEATURE_REG_OFFSET_H__

/* AR_FEATURE Base address of Module's Register */
#define AR_FEATURE_BASE                       (0x0)

/******************************************************************************/
/*                      AR_FEATURE Registers' Definitions                            */
/******************************************************************************/

#define AR_FEATURE_TOP_CFG_REG              (AR_FEATURE_BASE + 0x0)    /* Top Config */
#define AR_FEATURE_IMAGE_SIZE_REG           (AR_FEATURE_BASE + 0x4)    /* Image size config */
#define AR_FEATURE_BLOCK_NUM_CFG_REG        (AR_FEATURE_BASE + 0x8)    /* BLOCK Number */
#define AR_FEATURE_BLOCK_SIZE_CFG_INV_REG   (AR_FEATURE_BASE + 0xC)    /* BLOCK Size_INV */
#define AR_FEATURE_DETECT_NUMBER_LIMIT_REG  (AR_FEATURE_BASE + 0x10)   /* number limit */
#define AR_FEATURE_SIGMA_COEF_REG           (AR_FEATURE_BASE + 0x14)   /* Sigma values */
#define AR_FEATURE_GAUSS_ORG_REG            (AR_FEATURE_BASE + 0x20)   /* Gauss Coefficients */
#define AR_FEATURE_GSBLUR_1ST_REG           (AR_FEATURE_BASE + 0x24)   /* Gauss Coefficients */
#define AR_FEATURE_GAUSS_2ND_REG            (AR_FEATURE_BASE + 0x28)   /* Gauss Coefficients */
#define AR_FEATURE_DOG_EDGE_THRESHOLD_REG   (AR_FEATURE_BASE + 0x30)   /* dog edge threshold */
#define AR_FEATURE_DESCRIPTOR_THRESHOLD_REG (AR_FEATURE_BASE + 0x34)   /* descriptor threshold */
#define AR_FEATURE_CVDR_RD_CFG_REG          (AR_FEATURE_BASE + 0x40)   /* Video port read Configuration. */
#define AR_FEATURE_CVDR_RD_LWG_REG          (AR_FEATURE_BASE + 0x44)   /* Line width generation. */
#define AR_FEATURE_CVDR_RD_FHG_REG          (AR_FEATURE_BASE + 0x48)   /* Frame height generation. */
#define AR_FEATURE_CVDR_RD_AXI_FS_REG       (AR_FEATURE_BASE + 0x4C)   /* AXI frame start. */
#define AR_FEATURE_CVDR_RD_AXI_LINE_REG     (AR_FEATURE_BASE + 0x50)   /* Line Wrap definition. */
#define AR_FEATURE_CVDR_RD_IF_CFG_REG       (AR_FEATURE_BASE + 0x54)   /* Video port read interface configuration */
#define AR_FEATURE_PRE_CVDR_RD_FHG_REG      (AR_FEATURE_BASE + 0x58)   /* Frame height generation. */
#define AR_FEATURE_CVDR_WR_CFG_REG          (AR_FEATURE_BASE + 0x5C)   /* Video port write Configuration. */
#define AR_FEATURE_CVDR_WR_AXI_FS_REG       (AR_FEATURE_BASE + 0x60)   /* AXI address Frame start. */
#define AR_FEATURE_CVDR_WR_AXI_LINE_REG     (AR_FEATURE_BASE + 0x64)   /* AXI line wrap and line stride. */
#define AR_FEATURE_CVDR_WR_IF_CFG_REG       (AR_FEATURE_BASE + 0x68)   /* Video port write interface configuration */
#define AR_FEATURE_PRE_CVDR_WR_AXI_FS_REG   (AR_FEATURE_BASE + 0x6C)   /* AXI address Frame start for first. */
#define AR_FEATURE_KPT_LIMIT_0_REG          (AR_FEATURE_BASE + 0x80)   /* control the number of KPT */
#define AR_FEATURE_KPT_LIMIT_373_REG        (AR_FEATURE_BASE + 0x654)  /* control the number of KPT */
#define AR_FEATURE_GRIDSTAT_1_0_REG         (AR_FEATURE_BASE + 0x680)  /* grid stat 1 */
#define AR_FEATURE_GRIDSTAT_1_373_REG       (AR_FEATURE_BASE + 0xC54)  /* grid stat 1 */
#define AR_FEATURE_GRIDSTAT_2_0_REG         (AR_FEATURE_BASE + 0xC80)  /* grid stat 2 */
#define AR_FEATURE_GRIDSTAT_2_373_REG       (AR_FEATURE_BASE + 0x1254) /* grid stat 2 */
#define AR_FEATURE_KPT_NUMBER_0_REG         (AR_FEATURE_BASE + 0x1280) /* Feature number in each block */
#define AR_FEATURE_KPT_NUMBER_93_REG        (AR_FEATURE_BASE + 0x13F4) /* Feature number in each block */
#define AR_FEATURE_TOTAL_KPT_NUM_REG        (AR_FEATURE_BASE + 0x13F8) /* Total kpt number */
#define AR_FEATURE_DEBUG_0_REG              (AR_FEATURE_BASE + 0x1400) /* debug signal 0 */
#define AR_FEATURE_DEBUG_1_REG              (AR_FEATURE_BASE + 0x1404) /* debug signal 1 */
#define AR_FEATURE_DEBUG_2_REG              (AR_FEATURE_BASE + 0x1408) /* debug signal 2 */
#define AR_FEATURE_DEBUG_3_REG              (AR_FEATURE_BASE + 0x140C) /* debug signal 3 */

#endif // __AR_FEATURE_REG_OFFSET_H__
