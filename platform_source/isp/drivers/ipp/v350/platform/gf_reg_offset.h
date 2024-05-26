// ******************************************************************************
// Copyright     :  Copyright (C) 2020, Hisilicon Technologies Co. Ltd.
// File name     :  gf_reg_offset.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Date          :  2020/4/10
// Description   :
// Others        :  Generated automatically by nManager V4.2
// ******************************************************************************

#ifndef __GF_REG_REG_OFFSET_H__
#define __GF_REG_REG_OFFSET_H__

/* GF_reg Base address of Module's Register */
#define GF_BASE                       (0x0)

/******************************************************************************/
/*                      GF_reg Registers' Definitions                            */
/******************************************************************************/

#define GF_IMAGE_SIZE_REG   (GF_BASE + 0x0)  /* GF input image size */
#define GF_MODE_REG         (GF_BASE + 0x4)  /* GF input and output mode */
#define GF_FILTER_COEFF_REG (GF_BASE + 0x8)  /* GF filter coefficient */
#define GF_CROP_H_REG       (GF_BASE + 0xC)  /* Horizontal boundry of CROP for output port */
#define GF_CROP_V_REG       (GF_BASE + 0x10) /* Vertical boundry of CROP for output port */
#define GF_DEBUG_0_REG      (GF_BASE + 0x14) /* Debug 1 */
#define GF_DEBUG_1_REG      (GF_BASE + 0x18) /* Debug 1 */
#define GF_EC_0_REG         (GF_BASE + 0x1C) /* ec 0 */
#define GF_EC_1_REG         (GF_BASE + 0x20) /* ec 1 */

#endif // __GF_REG_REG_OFFSET_H__
