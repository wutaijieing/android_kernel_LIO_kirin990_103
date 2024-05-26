// *********************************************************
// Copyright     :  Copyright (C) 2019, Hisilicon Technologies Co. Ltd.
// File name     :  ipp_top_reg_offset.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Version       :  1.0
// Date          :  2019/01/02
// Description   :  The description of xxx project
// Others        :  Generated automatically by nManager V4.1
// **********************************************************

#ifndef __IPP_TOP_REG_OFFSET_H__
#define __IPP_TOP_REG_OFFSET_H__


// ************************************************************
// *                      Baltimore_ISP_V300 IPP_TOP Registers' Definitions
// ************************************************************

#define IPP_TOP_DMA_CRG_CFG0_REG       0x0   /* configure for top axi path */
#define IPP_TOP_DMA_CRG_CFG1_REG       0x4   /* CRG configure for reset */
#define IPP_TOP_DMA_CRG_CFG2_REG       0x8   /* CRG configure for reset */
#define IPP_TOP_CVDR_IRQ_REG0_REG      0x10  /* IRQ related cfg register */
#define IPP_TOP_CVDR_IRQ_REG1_REG      0x14  /* IRQ related cfg register */
#define IPP_TOP_CVDR_IRQ_REG2_REG      0x18  /* IRQ related cfg register */
#define IPP_TOP_CVDR_MEM_CFG0_REG      0x20  /* SPSRAM configure for subsys */
#define IPP_TOP_CVDR_MEM_CFG1_REG      0x24  /* TPSRAM configure for subsys */
#define IPP_TOP_DPM_CTRL_REG           0x28  /* SPSRAM configure for subsys */
#define IPP_TOP_IPP_RESERVED_REG       0x7C  /* reserved for EC */
#define IPP_TOP_JPG_FLUX_CTRL0_0_REG   0x80  /* grp0 for CVDR RT AXI R */
#define IPP_TOP_JPG_FLUX_CTRL0_1_REG   0x84  /* grp1 for CVDR RT AXI R */
#define IPP_TOP_JPG_FLUX_CTRL1_0_REG   0x88  /* grp0 for CVDR RT AXI W */
#define IPP_TOP_JPG_FLUX_CTRL1_1_REG   0x8C  /* grp1 for CVDR RT AXI W */
#define IPP_TOP_JPG_BWC_CHN_AR_REG     0xF0  /* JPG bandwidth ctrolor config */
#define IPP_TOP_JPG_RO_STATE_REG       0xFC  /* some read only state */
#define IPP_TOP_JPGENC_CRG_CFG0_REG    0x100 /* FOR CLK */
#define IPP_TOP_JPGENC_CRG_CFG1_REG    0x104 /* FOR RESET */
#define IPP_TOP_JPGENC_MEM_CFG_REG     0x108 /* FOR IPP JPEGEN */
#define IPP_TOP_JPGENC_IRQ_REG0_REG    0x110 /* IRQ related cfg register */
#define IPP_TOP_JPGENC_IRQ_REG1_REG    0x114 /* IRQ related cfg register */
#define IPP_TOP_JPGENC_IRQ_REG2_REG    0x118 /* IRQ related cfg register */
#define IPP_TOP_JPGDEC_CRG_CFG0_REG    0x180 /* FOR CLK */
#define IPP_TOP_JPGDEC_CRG_CFG1_REG    0x184 /* FOR RESET */
#define IPP_TOP_JPGDEC_MEM_CFG_REG     0x188 /* FOR IPP JPEGDEN */
#define IPP_TOP_JPGDEC_IRQ_REG0_REG    0x190 /* IRQ related cfg register */
#define IPP_TOP_JPGDEC_IRQ_REG1_REG    0x194 /* IRQ related cfg register */
#define IPP_TOP_JPGDEC_IRQ_REG2_REG    0x198 /* IRQ related cfg register */
#define IPP_TOP_GF_CRG_CFG0_REG        0x200 /* FOR CLK */
#define IPP_TOP_GF_CRG_CFG1_REG        0x204 /*  FOR RESET */
#define IPP_TOP_GF_MEM_CFG_REG         0x208 /* FOR IPP GF */
#define IPP_TOP_GF_IRQ_REG0_REG        0x210 /* IRQ related cfg register */
#define IPP_TOP_GF_IRQ_REG1_REG        0x214 /* IRQ related cfg register */
#define IPP_TOP_GF_IRQ_REG2_REG        0x218 /* IRQ related cfg register */
#define IPP_TOP_ORB_CRG_CFG0_REG       0x280 /* FOR CLK */
#define IPP_TOP_ORB_CRG_CFG1_REG       0x284 /* FOR RESET */
#define IPP_TOP_ORB_MEM_CFG_REG        0x288 /* FOR IPP ORB */
#define IPP_TOP_ORB_IRQ_REG0_REG       0x290 /* IRQ related cfg register */
#define IPP_TOP_ORB_IRQ_REG1_REG       0x294 /* IRQ related cfg register */
#define IPP_TOP_ORB_IRQ_REG2_REG       0x298 /* IRQ related cfg register */
#define IPP_TOP_RDR_CRG_CFG0_REG       0x300 /* FOR CLK */
#define IPP_TOP_RDR_CRG_CFG1_REG       0x304 /* FOR RESET */
#define IPP_TOP_RDR_MEM_CFG_REG        0x308 /* FOR IPP REORDER */
#define IPP_TOP_RDR_IRQ_REG0_REG       0x310 /* IRQ related cfg register */
#define IPP_TOP_RDR_IRQ_REG1_REG       0x314 /* IRQ related cfg register */
#define IPP_TOP_RDR_IRQ_REG2_REG       0x318 /* IRQ related cfg register */
#define IPP_TOP_CMP_CRG_CFG0_REG       0x380 /* FOR CLK */
#define IPP_TOP_CMP_CRG_CFG1_REG       0x384 /* FOR RESET */
#define IPP_TOP_CMP_MEM_CFG_REG        0x388 /* FOR IPP CMP */
#define IPP_TOP_CMP_IRQ_REG0_REG       0x390 /* IRQ related cfg register */
#define IPP_TOP_CMP_IRQ_REG1_REG       0x394 /* IRQ related cfg register */
#define IPP_TOP_CMP_IRQ_REG2_REG       0x398 /* IRQ related cfg register */
#define IPP_TOP_ENH_CRG_CFG0_REG       0x400 /*  FOR CLK */
#define IPP_TOP_ENH_CRG_CFG1_REG       0x404 /*  FOR RESET */
#define IPP_TOP_ENH_MEM_CFG_REG        0x408 /* FOR IPP ORB_ENH */
#define IPP_TOP_ENH_VPB_CFG_REG        0x40C /*  FOR VP PORT */
#define IPP_TOP_ENH_IRQ_REG0_REG       0x410 /* IRQ related cfg register */
#define IPP_TOP_ENH_IRQ_REG1_REG       0x414 /* IRQ related cfg register */
#define IPP_TOP_ENH_IRQ_REG2_REG       0x418 /* IRQ related cfg register */
#define IPP_TOP_VBK_CRG_CFG0_REG       0x480 /*  FOR CLK */
#define IPP_TOP_VBK_CRG_CFG1_REG       0x484 /*  FOR RESET */
#define IPP_TOP_VBK_MEM_CFG_REG        0x488 /* FOR IPP VBK */
#define IPP_TOP_VBK_IRQ_REG0_REG       0x48C /* IRQ related cfg register */
#define IPP_TOP_VBK_IRQ_REG1_REG       0x490 /* IRQ related cfg register */
#define IPP_TOP_VBK_IRQ_REG2_REG       0x494 /* IRQ related cfg register */
#define IPP_TOP_VBK_IRQ_REG3_REG       0x498 /* IRQ related cfg register */
#define IPP_TOP_VBK_IRQ_REG4_REG       0x49C /* IRQ related cfg register */
#define IPP_TOP_ANF_CRG_CFG0_REG       0x4C0 /*  FOR CLK */
#define IPP_TOP_ANF_CRG_CFG1_REG       0x4C4 /*  FOR RESET */
#define IPP_TOP_ANF_MEM_CFG_REG        0x4C8 /* FOR IPP ANF */
#define IPP_TOP_ANF_IRQ_REG0_REG       0x4CC /* IRQ related cfg register */
#define IPP_TOP_ANF_IRQ_REG1_REG       0x4D0 /* IRQ related cfg register */
#define IPP_TOP_ANF_IRQ_REG2_REG       0x4D4 /* IRQ related cfg register */
#define IPP_TOP_ANF_IRQ_REG3_REG       0x4D8 /* IRQ related cfg register */
#define IPP_TOP_ANF_IRQ_REG4_REG       0x4DC /* IRQ related cfg register */
#define IPP_TOP_CMDLST_CTRL_MAP_0_REG  0x500 /* CMDLST cfg  for channel */
#define IPP_TOP_CMDLST_CTRL_MAP_1_REG  0x510 /* CMDLST cfg  for channel */
#define IPP_TOP_CMDLST_CTRL_MAP_2_REG  0x520 /* CMDLST cfg  for channel */
#define IPP_TOP_CMDLST_CTRL_MAP_3_REG  0x530 /* CMDLST cfg  for channel */
#define IPP_TOP_CMDLST_CTRL_MAP_4_REG  0x540 /* CMDLST cfg  for channel */
#define IPP_TOP_CMDLST_CTRL_MAP_5_REG  0x550 /* CMDLST cfg  for channel */
#define IPP_TOP_CMDLST_CTRL_MAP_6_REG  0x560 /* CMDLST cfg  for channel */
#define IPP_TOP_CMDLST_CTRL_MAP_7_REG  0x570 /* CMDLST cfg  for channel */
#define IPP_TOP_CMDLST_CTRL_MAP_8_REG  0x580 /* CMDLST cfg  for channel */
#define IPP_TOP_CMDLST_CTRL_MAP_9_REG  0x590 /* CMDLST cfg  for channel */
#define IPP_TOP_CMDLST_CTRL_MAP_10_REG 0x5A0 /* CMDLST cfg for channel */
#define IPP_TOP_CMDLST_CTRL_MAP_11_REG 0x5B0 /* CMDLST cfg for channel */
#define IPP_TOP_CMDLST_CTRL_MAP_12_REG 0x5C0 /* CMDLST cfg for channel */
#define IPP_TOP_CMDLST_CTRL_MAP_13_REG 0x5D0 /* CMDLST cfg for channel */
#define IPP_TOP_CMDLST_CTRL_MAP_14_REG 0x5E0 /* CMDLST cfg for channel */
#define IPP_TOP_CMDLST_CTRL_MAP_15_REG 0x5F0 /* CMDLST cfg for channel */
#define IPP_TOP_CMDLST_CTRL_PM_0_REG   0x504 /* CMDLST cfg  for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_1_REG   0x514 /* CMDLST cfg  for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_2_REG   0x524 /* CMDLST cfg  for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_3_REG   0x534 /* CMDLST cfg  for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_4_REG   0x544 /* CMDLST cfg  for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_5_REG   0x554 /* CMDLST cfg  for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_6_REG   0x564 /* CMDLST cfg  for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_7_REG   0x574 /* CMDLST cfg  for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_8_REG   0x584 /* CMDLST cfg  for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_9_REG   0x594 /* CMDLST cfg  for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_10_REG  0x5A4 /* CMDLST cfg for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_11_REG  0x5B4 /* CMDLST cfg for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_12_REG  0x5C4 /* CMDLST cfg for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_13_REG  0x5D4 /* CMDLST cfg for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_14_REG  0x5E4 /* CMDLST cfg for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_15_REG  0x5F4 /* CMDLST cfg for channel0 */
#define IPP_TOP_CMDLST_IRQ_CLR_0_REG   0x600 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_CLR_1_REG   0x610 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_CLR_2_REG   0x620 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_CLR_3_REG   0x630 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_CLR_4_REG   0x640 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_CLR_5_REG   0x650 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_MSK_0_REG   0x604 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_MSK_1_REG   0x614 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_MSK_2_REG   0x624 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_MSK_3_REG   0x634 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_MSK_4_REG   0x644 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_MSK_5_REG   0x654 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_STA_0_REG   0x608 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_STA_1_REG   0x618 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_STA_2_REG   0x628 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_STA_3_REG   0x638 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_STA_4_REG   0x648 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_STA_5_REG   0x658 /* IRQ related cfg register */
#define IPP_TOP_MC_CRG_CFG0_REG        0x700 /* CRG config register for clk */
#define IPP_TOP_MC_CRG_CFG1_REG        0x704 /*  for reset */
#define IPP_TOP_MC_MEM_CFG_REG         0x708 /* for IPP MC */
#define IPP_TOP_MC_IRQ_REG0_REG        0x70C /* IRQ related cfg register */
#define IPP_TOP_MC_IRQ_REG1_REG        0x710 /* IRQ related cfg register */
#define IPP_TOP_MC_IRQ_REG2_REG        0x714 /* IRQ related cfg register */
#define IPP_TOP_MC_IRQ_REG3_REG        0x718 /* IRQ related cfg register */
#define IPP_TOP_MC_IRQ_REG4_REG        0x71C /* IRQ related cfg register */
#define IPP_TOP_JPG_DEBUG_0_REG        0x700 /* debug register 0 */
#define IPP_TOP_JPG_DEBUG_1_REG        0x704 /* debug register 1 */
#define IPP_TOP_JPG_DEBUG_2_REG        0x708 /* debug register 2 */
#define IPP_TOP_JPG_DEBUG_3_REG        0x70C /* debug register 3 */
#define IPP_TOP_JPG_SEC_CTRL_S_REG     0x800 /* JPG secure cfg register */

#endif // __IPP_TOP_REG_OFFSET_H__
