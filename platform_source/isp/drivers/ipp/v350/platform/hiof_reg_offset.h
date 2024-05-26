// ******************************************************************************
// Copyright     :  Copyright (C) 2020, Hisilicon Technologies Co. Ltd.
// File name     :  hiof_reg_offset.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Date          :  2020/04/14
// Description   :
// Others        :  Generated automatically by nManager V4.2
// ******************************************************************************

#ifndef __HIOF_REG_OFFSET_H__
#define __HIOF_REG_OFFSET_H__

/* HIOF Base address of Module's Register */
#define HIOF_BASE                       (0x0)

/******************************************************************************/
/*                      xxx HIOF Registers' Definitions                            */
/******************************************************************************/

#define HIOF_HIOF_CFG_REG      (HIOF_BASE + 0x4)  /* HIOF Functional mode Configuration */
#define HIOF_SIZE_CFG_REG      (HIOF_BASE + 0x8)  /* Size cfg */
#define HIOF_COEFF_CFG_REG     (HIOF_BASE + 0xC)  /* Coeff cfg */
#define HIOF_THRESHOLD_CFG_REG (HIOF_BASE + 0x10) /* Threshold cfg */
#define HIOF_DEBUG_RO1_REG     (HIOF_BASE + 0x20) /* debug information1 */
#define HIOF_DEBUG_RO2_REG     (HIOF_BASE + 0x24) /* debug information2 */
#define HIOF_DEBUG_RW_REG      (HIOF_BASE + 0x28) /* reserved for EC */

#endif // __HIOF_REG_OFFSET_H__
