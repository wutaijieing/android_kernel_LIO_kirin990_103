// ******************************************************************************
// Copyright     :  Copyright (C) 2020, Hisilicon Technologies Co. Ltd.
// File name     :  mc_reg_offset.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Date          :  2020/04/10
// Description   :
// Others        :  Generated automatically by nManager V4.2
// ******************************************************************************

#ifndef __MC_REG_OFFSET_H__
#define __MC_REG_OFFSET_H__

/* MC Base address of Module's Register */
#define MC_BASE                       (0x0)

/******************************************************************************/
/*                      MC Registers' Definitions                            */
/******************************************************************************/

#define MC_MC_CFG_REG                (MC_BASE + 0x0)  /* Basic Config */
#define MC_THRESHOLD_CFG_REG         (MC_BASE + 0x4)  /* Threshold Config */
#define MC_MATCH_POINTS_REG          (MC_BASE + 0x8)  /* Matched points number */
#define MC_INLIER_NUMBER_REG         (MC_BASE + 0xC)  /* Inlier numbers for FW */
#define MC_H_MATRIX_0_REG            (MC_BASE + 0x10) /* H matrix */
#define MC_H_MATRIX_1_REG            (MC_BASE + 0x14) /* H matrix */
#define MC_H_MATRIX_2_REG            (MC_BASE + 0x18) /* H matrix */
#define MC_H_MATRIX_3_REG            (MC_BASE + 0x1C) /* H matrix */
#define MC_H_MATRIX_4_REG            (MC_BASE + 0x20) /* H matrix */
#define MC_H_MATRIX_5_REG            (MC_BASE + 0x24) /* H matrix */
#define MC_H_MATRIX_6_REG            (MC_BASE + 0x28) /* H matrix */
#define MC_H_MATRIX_7_REG            (MC_BASE + 0x2C) /* H matrix */
#define MC_H_MATRIX_8_REG            (MC_BASE + 0x30) /* H matrix */
#define MC_H_MATRIX_9_REG            (MC_BASE + 0x34) /* H matrix */
#define MC_H_MATRIX_10_REG           (MC_BASE + 0x38) /* H matrix */
#define MC_H_MATRIX_11_REG           (MC_BASE + 0x3C) /* H matrix */
#define MC_H_MATRIX_12_REG           (MC_BASE + 0x40) /* H matrix */
#define MC_H_MATRIX_13_REG           (MC_BASE + 0x44) /* H matrix */
#define MC_H_MATRIX_14_REG           (MC_BASE + 0x48) /* H matrix */
#define MC_H_MATRIX_15_REG           (MC_BASE + 0x4C) /* H matrix */
#define MC_H_MATRIX_16_REG           (MC_BASE + 0x50) /* H matrix */
#define MC_H_MATRIX_17_REG           (MC_BASE + 0x54) /* H matrix */
#define MC_REF_PREFETCH_CFG_REG      (MC_BASE + 0x60) /* Configure the prefetch for reference */
#define MC_CUR_PREFETCH_CFG_REG      (MC_BASE + 0x64) /* Configure the prefetch for current */
#define MC_TABLE_CLEAR_CFG_REG       (MC_BASE + 0x70) /* Clear the configure tables */
#define MC_INDEX_CFG_0_REG           (MC_BASE + 0x80) /* Index pairs from CMP */
#define MC_INDEX_PAIRS_0_REG         (MC_BASE + 0x90) /* Index pairs from CMP */
#define MC_COORDINATE_CFG_0_REG      (MC_BASE + 0xA0) /* Coordinate pairs from CMP */
#define MC_COORDINATE_PAIRS_0_REG    (MC_BASE + 0xB0)
#define MC_DEBUG_0_REG               (MC_BASE + 0xC0) /* Debug 0 */
#define MC_DEBUG_1_REG               (MC_BASE + 0xC4) /* Debug 1 */
#define MC_EC_0_REG                  (MC_BASE + 0xC8) /* ec 0 */
#define MC_EC_1_REG                  (MC_BASE + 0xCC) /* ec 1 */

#endif // __MC_REG_OFFSET_H__
