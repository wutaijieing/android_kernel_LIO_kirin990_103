// ******************************************************************************
// Copyright     :  Copyright (C) 2020, Hisilicon Technologies Co. Ltd.
// File name     :  ipp_top_reg_offset.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Date          :  2013/3/10
// Description   :  The description of xxx project
// Others        :  Generated automatically by nManager V4.2
// History       :  xxx 2020/04/15 14:54:24 Create file
// ******************************************************************************

#ifndef __IPP_TOP_REG_OFFSET_H__
#define __IPP_TOP_REG_OFFSET_H__

/* IPP_TOP Base address of Module's Register */

/******************************************************************************/
/*                     IPP_TOP Registers' Definitions                         */
/******************************************************************************/

#define IPP_TOP_DMA_CRG_CFG0_REG           0x0   /* configure register for top axi path */
#define IPP_TOP_DMA_CRG_CFG1_REG           0x4   /* CRG configure register for reset */
#define IPP_TOP_DMA_CRG_CFG2_REG           0x8   /* CRG configure register for reset */
#define IPP_TOP_CVDR_IRQ_REG0_REG          0x10  /* IRQ related cfg register */
#define IPP_TOP_CVDR_IRQ_REG2_REG          0x14  /* IRQ related cfg register */
#define IPP_TOP_CVDR_IRQ_REG3_REG          0x18  /* IRQ related cfg register */
#define IPP_TOP_CVDR_IRQ_REG4_REG          0x1C  /* IRQ related cfg register */
#define IPP_TOP_CVDR_MEM_CFG0_REG          0x20  /* SPSRAM configure register for subsys */
#define IPP_TOP_CVDR_MEM_CFG1_REG          0x24  /* TPSRAM configure register for subsys */
#define IPP_TOP_MEM_CTRL_SPUA_REG          0x28  /* SPUARAM configure register for subsys */
#define IPP_TOP_IPP_RESERVED_0_REG         0x40  /* reserved for EC */
#define IPP_TOP_IPP_RESERVED_1_REG         0x44  /* reserved for EC */
#define IPP_TOP_IPP_RESERVED_2_REG         0x48  /* reserved for EC */
#define IPP_TOP_IPP_RESERVED_3_REG         0x4C  /* reserved for EC */
#define IPP_TOP_IPP_RESERVED_4_REG         0x50  /* reserved for EC */
#define IPP_TOP_IPP_RESERVED_5_REG         0x54  /* reserved for EC */
#define IPP_TOP_IPP_RESERVED_6_REG         0x58  /* reserved for EC */
#define IPP_TOP_IPP_RESERVED_7_REG         0x5C  /* reserved for EC */
#define IPP_TOP_IPP_RESERVED_8_REG         0x60  /* reserved for EC */
#define IPP_TOP_IPP_RESERVED_9_REG         0x64  /* reserved for EC */
#define IPP_TOP_IPP_RESERVED_10_REG        0x68  /* reserved for EC */
#define IPP_TOP_IPP_RESERVED_11_REG        0x6C  /* reserved for EC */
#define IPP_TOP_IPP_RESERVED_12_REG        0x70  /* reserved for EC */
#define IPP_TOP_IPP_RESERVED_13_REG        0x74  /* reserved for EC */
#define IPP_TOP_IPP_RESERVED_14_REG        0x78  /* reserved for EC */
#define IPP_TOP_IPP_RESERVED_15_REG        0x7C  /* reserved for EC */
#define IPP_TOP_JPG_FLUX_CTRL0_0_REG       0x80  /* JPG   flux ctrl grp0 for CVDR RT AXI R */
#define IPP_TOP_JPG_FLUX_CTRL0_1_REG       0x84  /* JPG   flux ctrl grp1 for CVDR RT AXI R */
#define IPP_TOP_JPG_FLUX_CTRL1_0_REG       0x88  /* JPG   flux ctrl grp0 for CVDR RT AXI W */
#define IPP_TOP_JPG_FLUX_CTRL1_1_REG       0x8C  /* JPG   flux ctrl grp1 for CVDR RT AXI W */
#define IPP_TOP_JPG_BWC_CHN_AR_REG         0xF0  /* JPG bandwidth ctrolor config */
#define IPP_TOP_JPG_RO_STATE_REG           0xFC  /* some read only state */
#define IPP_TOP_JPGENC_CRG_CFG0_REG        0x100 /* CRG configure register for clk */
#define IPP_TOP_JPGENC_CRG_CFG1_REG        0x104 /* CRG configure register for reset */
#define IPP_TOP_JPGENC_MEM_CFG_REG         0x108 /* SRAM configure register */
#define IPP_TOP_JPGENC_IRQ_REG0_REG        0x110 /* IRQ related cfg register */
#define IPP_TOP_JPGENC_IRQ_REG2_REG        0x114 /* IRQ related cfg register */
#define IPP_TOP_JPGENC_IRQ_REG3_REG        0x118 /* IRQ related cfg register */
#define IPP_TOP_JPGENC_IRQ_REG4_REG        0x11C /* IRQ related cfg register */
#define IPP_TOP_JPGDEC_CRG_CFG0_REG        0x180 /* CRG config register for clk */
#define IPP_TOP_JPGDEC_CRG_CFG1_REG        0x184 /* CRG configure register for reset */
#define IPP_TOP_JPGDEC_MEM_CFG_REG         0x188 /* SRAM configure register for SLAM MCF */
#define IPP_TOP_JPGDEC_IRQ_REG0_REG        0x190 /* IRQ related cfg register */
#define IPP_TOP_JPGDEC_IRQ_REG2_REG        0x194 /* IRQ related cfg register */
#define IPP_TOP_JPGDEC_IRQ_REG3_REG        0x198 /* IRQ related cfg register */
#define IPP_TOP_JPGDEC_IRQ_REG4_REG        0x19C /* IRQ related cfg register */
#define IPP_TOP_GF_CRG_CFG0_REG            0x200 /* CRG config register for clk */
#define IPP_TOP_GF_CRG_CFG1_REG            0x204 /* CRG configure register for reset */
#define IPP_TOP_GF_MEM_CFG_REG             0x208 /* SRAM configure register for CPE MCF */
#define IPP_TOP_GF_IRQ_REG0_REG            0x210 /* IRQ related cfg register */
#define IPP_TOP_GF_IRQ_REG1_REG            0x214 /* IRQ related cfg register */
#define IPP_TOP_GF_IRQ_REG2_REG            0x218 /* IRQ related cfg register */
#define IPP_TOP_GF_IRQ_REG3_REG            0x21C /* IRQ related cfg register */
#define IPP_TOP_GF_IRQ_REG4_REG            0x220 /* IRQ related cfg register */
#define IPP_TOP_ARF_CRG_CFG0_REG           0x240 /* CRG config register for clk */
#define IPP_TOP_ARF_CRG_CFG1_REG           0x244 /* CRG configure register for reset */
#define IPP_TOP_ARF_MEM_CFG_REG            0x248 /* SRAM configure register for SLAM MCF */
#define IPP_TOP_ARF_IRQ_REG0_REG           0x24C /* IRQ related cfg register */
#define IPP_TOP_ARF_IRQ_REG1_REG           0x250 /* IRQ related cfg register */
#define IPP_TOP_ARF_IRQ_REG2_REG           0x254 /* IRQ related cfg register */
#define IPP_TOP_ARF_IRQ_REG3_REG           0x258 /* IRQ related cfg register */
#define IPP_TOP_ARF_IRQ_REG4_REG           0x25C /* IRQ related cfg register */
#define IPP_TOP_RDR_CRG_CFG0_REG           0x300 /* CRG config register for clk */
#define IPP_TOP_RDR_CRG_CFG1_REG           0x304 /* CRG configure register for reset */
#define IPP_TOP_RDR_MEM_CFG_REG            0x308 /* SRAM configure register for SLAM MCF */
#define IPP_TOP_RDR_IRQ_REG0_REG           0x310 /* IRQ related cfg register */
#define IPP_TOP_RDR_IRQ_REG1_REG           0x314 /* IRQ related cfg register */
#define IPP_TOP_RDR_IRQ_REG2_REG           0x318 /* IRQ related cfg register */
#define IPP_TOP_RDR_IRQ_REG3_REG           0x31C /* IRQ related cfg register */
#define IPP_TOP_RDR_IRQ_REG4_REG           0x320 /* IRQ related cfg register */
#define IPP_TOP_CMP_CRG_CFG0_REG           0x330 /* CRG config register for clk */
#define IPP_TOP_CMP_CRG_CFG1_REG           0x334 /* CRG configure register for reset */
#define IPP_TOP_CMP_MEM_CFG_REG            0x338 /* SRAM configure register for SLAM MCF */
#define IPP_TOP_CMP_IRQ_REG0_REG           0x340 /* IRQ related cfg register */
#define IPP_TOP_CMP_IRQ_REG1_REG           0x344 /* IRQ related cfg register */
#define IPP_TOP_CMP_IRQ_REG2_REG           0x348 /* IRQ related cfg register */
#define IPP_TOP_CMP_IRQ_REG3_REG           0x34C /* IRQ related cfg register */
#define IPP_TOP_CMP_IRQ_REG4_REG           0x350 /* IRQ related cfg register */
#define IPP_TOP_ENH_CRG_CFG0_REG           0x360 /* CRG config register for clk */
#define IPP_TOP_ENH_CRG_CFG1_REG           0x364 /* CRG configure register for reset */
#define IPP_TOP_ENH_MEM_CFG_REG            0x368 /* SRAM configure register for SLAM MCF */
#define IPP_TOP_ENH_VPB_CFG_REG            0x36C /* CRG configure register for reset */
#define IPP_TOP_ENH_IRQ_REG0_REG           0x380 /* IRQ related cfg register */
#define IPP_TOP_ENH_IRQ_REG1_REG           0x384 /* IRQ related cfg register */
#define IPP_TOP_ENH_IRQ_REG2_REG           0x388 /* IRQ related cfg register */
#define IPP_TOP_ENH_IRQ_REG3_REG           0x38C /* IRQ related cfg register */
#define IPP_TOP_ENH_IRQ_REG4_REG           0x390 /* IRQ related cfg register */
#define IPP_TOP_ICP_CRG_CFG0_REG           0x4A0 /* CRG config register for clk */
#define IPP_TOP_ICP_CRG_CFG1_REG           0x4A4 /* CRG configure register for reset */
#define IPP_TOP_ICP_MEM_CFG_REG            0x4A8 /* SRAM configure register for CPE MCF */
#define IPP_TOP_ICP_IRQ_REG0_REG           0x4AC /* IRQ related cfg register */
#define IPP_TOP_ICP_IRQ_REG1_REG           0x4B0 /* IRQ related cfg register */
#define IPP_TOP_ICP_IRQ_REG2_REG           0x4B4 /* IRQ related cfg register */
#define IPP_TOP_ICP_IRQ_REG3_REG           0x4B8 /* IRQ related cfg register */
#define IPP_TOP_ICP_IRQ_REG4_REG           0x4BC /* IRQ related cfg register */
#define IPP_TOP_HIOF_CRG_CFG0_REG          0x4E0 /* CRG config register for clk */
#define IPP_TOP_HIOF_CRG_CFG1_REG          0x4E4 /* CRG configure register for reset */
#define IPP_TOP_HIOF_MEM_CFG_REG           0x4E8 /* SRAM configure register for CPE MCF */
#define IPP_TOP_HIOF_IRQ_REG0_REG          0x4EC /* IRQ related cfg register */
#define IPP_TOP_HIOF_IRQ_REG1_REG          0x4F0 /* IRQ related cfg register */
#define IPP_TOP_HIOF_IRQ_REG2_REG          0x4F4 /* IRQ related cfg register */
#define IPP_TOP_HIOF_IRQ_REG3_REG          0x4F8 /* IRQ related cfg register */
#define IPP_TOP_HIOF_IRQ_REG4_REG          0x4FC /* IRQ related cfg register */
#define IPP_TOP_CMDLST_CTRL_MAP_0_REG      0x500 /* CMDLST cfg register for channel mapping */
#define IPP_TOP_CMDLST_CTRL_MAP_1_REG      0x510 /* CMDLST cfg register for channel mapping */
#define IPP_TOP_CMDLST_CTRL_MAP_2_REG      0x520 /* CMDLST cfg register for channel mapping */
#define IPP_TOP_CMDLST_CTRL_MAP_3_REG      0x530 /* CMDLST cfg register for channel mapping */
#define IPP_TOP_CMDLST_CTRL_MAP_4_REG      0x540 /* CMDLST cfg register for channel mapping */
#define IPP_TOP_CMDLST_CTRL_MAP_5_REG      0x550 /* CMDLST cfg register for channel mapping */
#define IPP_TOP_CMDLST_CTRL_MAP_6_REG      0x560 /* CMDLST cfg register for channel mapping */
#define IPP_TOP_CMDLST_CTRL_MAP_7_REG      0x570 /* CMDLST cfg register for channel mapping */
#define IPP_TOP_CMDLST_CTRL_MAP_8_REG      0x580 /* CMDLST cfg register for channel mapping */
#define IPP_TOP_CMDLST_CTRL_MAP_9_REG      0x590 /* CMDLST cfg register for channel mapping */
#define IPP_TOP_CMDLST_CTRL_MAP_10_REG     0x5A0 /* CMDLST cfg register for channel mapping */
#define IPP_TOP_CMDLST_CTRL_MAP_11_REG     0x5B0 /* CMDLST cfg register for channel mapping */
#define IPP_TOP_CMDLST_CTRL_MAP_12_REG     0x5C0 /* CMDLST cfg register for channel mapping */
#define IPP_TOP_CMDLST_CTRL_MAP_13_REG     0x5D0 /* CMDLST cfg register for channel mapping */
#define IPP_TOP_CMDLST_CTRL_MAP_14_REG     0x5E0 /* CMDLST cfg register for channel mapping */
#define IPP_TOP_CMDLST_CTRL_MAP_15_REG     0x5F0 /* CMDLST cfg register for channel mapping */
#define IPP_TOP_CMDLST_CTRL_PM_0_REG       0x504 /* CMDLST cfg register for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_1_REG       0x514 /* CMDLST cfg register for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_2_REG       0x524 /* CMDLST cfg register for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_3_REG       0x534 /* CMDLST cfg register for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_4_REG       0x544 /* CMDLST cfg register for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_5_REG       0x554 /* CMDLST cfg register for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_6_REG       0x564 /* CMDLST cfg register for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_7_REG       0x574 /* CMDLST cfg register for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_8_REG       0x584 /* CMDLST cfg register for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_9_REG       0x594 /* CMDLST cfg register for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_10_REG      0x5A4 /* CMDLST cfg register for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_11_REG      0x5B4 /* CMDLST cfg register for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_12_REG      0x5C4 /* CMDLST cfg register for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_13_REG      0x5D4 /* CMDLST cfg register for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_14_REG      0x5E4 /* CMDLST cfg register for channel0 */
#define IPP_TOP_CMDLST_CTRL_PM_15_REG      0x5F4 /* CMDLST cfg register for channel0 */
#define IPP_TOP_CMDLST_IRQ_CLR_0_REG       0x600 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_CLR_1_REG       0x610 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_CLR_2_REG       0x620 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_CLR_3_REG       0x630 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_CLR_4_REG       0x640 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_CLR_5_REG       0x650 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_MSK_0_REG       0x604 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_MSK_1_REG       0x614 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_MSK_2_REG       0x624 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_MSK_3_REG       0x634 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_MSK_4_REG       0x644 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_MSK_5_REG       0x654 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_STATE_RAW_0_REG 0x608 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_STATE_RAW_1_REG 0x618 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_STATE_RAW_2_REG 0x628 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_STATE_RAW_3_REG 0x638 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_STATE_RAW_4_REG 0x648 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_STATE_RAW_5_REG 0x658 /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_STATE_MSK_0_REG 0x60C /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_STATE_MSK_1_REG 0x61C /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_STATE_MSK_2_REG 0x62C /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_STATE_MSK_3_REG 0x63C /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_STATE_MSK_4_REG 0x64C /* IRQ related cfg register */
#define IPP_TOP_CMDLST_IRQ_STATE_MSK_5_REG 0x65C /* IRQ related cfg register */
#define IPP_TOP_MC_CRG_CFG0_REG            0x700 /* CRG config register for clk */
#define IPP_TOP_MC_CRG_CFG1_REG            0x704 /* CRG configure register for reset */
#define IPP_TOP_MC_MEM_CFG_REG             0x708 /* SRAM configure register for CPE MCF */
#define IPP_TOP_MC_IRQ_REG0_REG            0x70C /* IRQ related cfg register */
#define IPP_TOP_MC_IRQ_REG1_REG            0x710 /* IRQ related cfg register */
#define IPP_TOP_MC_IRQ_REG2_REG            0x714 /* IRQ related cfg register */
#define IPP_TOP_MC_IRQ_REG3_REG            0x718 /* IRQ related cfg register */
#define IPP_TOP_MC_IRQ_REG4_REG            0x71C /* IRQ related cfg register */
#define IPP_TOP_JPG_DEBUG_0_REG            0x7C0 /* debug register 0 */
#define IPP_TOP_JPG_DEBUG_1_REG            0x7C4 /* debug register 1 */
#define IPP_TOP_JPG_DEBUG_2_REG            0x7C8 /* debug register 2 */
#define IPP_TOP_JPG_DEBUG_3_REG            0x7CC /* debug register 3 */
#define IPP_TOP_JPG_SEC_CTRL_S_REG         0x800 /* JPG secure cfg register */

#endif // __IPP_TOP_REG_OFFSET_H__
