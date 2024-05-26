// ******************************************************************************
// Copyright     :  Copyright (C) 2020, Hisilicon Technologies Co. Ltd.
// File name     :  cmdlst_reg_offset.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Date          :  2013/3/10
// Description   :  The description of xxx project
// Others        :  Generated automatically by nManager V4.2
// History       :  xxx 2020/04/02 14:01:20 Create file
// ******************************************************************************

#ifndef __CMDLST_REG_OFFSET_H__
#define __CMDLST_REG_OFFSET_H__

/* CMDLST Base address of Module's Register */
#define CMDLST_BASE                       (0x0)

/******************************************************************************/
/*                      xxx CMDLST Registers' Definitions                            */
/******************************************************************************/

#define CMDLST_ID_REG                   (CMDLST_BASE + 0x0)   /* Module Identifier */
#define CMDLST_VERSION_REG              (CMDLST_BASE + 0x4)   /* Module Version */
#define CMDLST_ST_LD_REG                (CMDLST_BASE + 0x10)  /* Read/Write register */
#define CMDLST_CFG_REG                  (CMDLST_BASE + 0x2C)  /* Module configuration */
#define CMDLST_START_ADDR_REG           (CMDLST_BASE + 0x30)  /* CMDLST start address */
#define CMDLST_END_ADDR_REG             (CMDLST_BASE + 0x34)  /* CMDLST end address */
#define CMDLST_CHECK_ERROR_STATUS_REG   (CMDLST_BASE + 0x40)  /* check error */
#define CMDLST_VCD_TRACE_REG            (CMDLST_BASE + 0x44)  /* Configure VCD trace signals to output */
#define CMDLST_DEBUG_0_REG              (CMDLST_BASE + 0x48)  /* Debug register */
#define CMDLST_DEBUG_1_REG              (CMDLST_BASE + 0x4C)  /* Debug register */
#define CMDLST_DEBUG_2_REG              (CMDLST_BASE + 0x50)  /* Debug register */
#define CMDLST_DEBUG_3_REG              (CMDLST_BASE + 0x54)  /* Debug register */
#define CMDLST_DEBUG_4_0_REG            (CMDLST_BASE + 0x58)  /* Debug register */
#define CMDLST_DEBUG_4_1_REG            (CMDLST_BASE + 0x5C)  /* Debug register*/
#define CMDLST_DEBUG_4_2_REG            (CMDLST_BASE + 0x60)  /* Debug register*/
#define CMDLST_DEBUG_4_3_REG            (CMDLST_BASE + 0x64)  /* Debug register*/
#define CMDLST_CH_CFG_0_REG             (CMDLST_BASE + 0x80)  /* Channel configuration register. */
#define CMDLST_CH_CFG_1_REG             (CMDLST_BASE + 0x100) /* Channel configuration register. */
#define CMDLST_CH_CFG_2_REG             (CMDLST_BASE + 0x180) /* Channel configuration register. */
#define CMDLST_CH_CFG_3_REG             (CMDLST_BASE + 0x200) /* Channel configuration register. */
#define CMDLST_CH_CFG_4_REG             (CMDLST_BASE + 0x280) /* Channel configuration register. */
#define CMDLST_CH_CFG_5_REG             (CMDLST_BASE + 0x300) /* Channel configuration register. */
#define CMDLST_CH_CFG_6_REG             (CMDLST_BASE + 0x380) /* Channel configuration register. */
#define CMDLST_CH_CFG_7_REG             (CMDLST_BASE + 0x400) /* Channel configuration register. */
#define CMDLST_CH_CFG_8_REG             (CMDLST_BASE + 0x480) /* Channel configuration register. */
#define CMDLST_CH_CFG_9_REG             (CMDLST_BASE + 0x500) /* Channel configuration register. */
#define CMDLST_CH_CFG_10_REG            (CMDLST_BASE + 0x580) /* Channel configuration register. */
#define CMDLST_CH_CFG_11_REG            (CMDLST_BASE + 0x600) /* Channel configuration register. */
#define CMDLST_CH_CFG_12_REG            (CMDLST_BASE + 0x680) /* Channel configuration register. */
#define CMDLST_CH_CFG_13_REG            (CMDLST_BASE + 0x700) /* Channel configuration register. */
#define CMDLST_CH_CFG_14_REG            (CMDLST_BASE + 0x780) /* Channel configuration register. */
#define CMDLST_CH_CFG_15_REG            (CMDLST_BASE + 0x800) /* Channel configuration register. */
#define CMDLST_CH_CFG_16_REG            (CMDLST_BASE + 0x880) /* Channel configuration register. */
#define CMDLST_CH_CFG_17_REG            (CMDLST_BASE + 0x900) /* Channel configuration register. */
#define CMDLST_CH_CFG_18_REG            (CMDLST_BASE + 0x980) /* Channel configuration register. */
#define CMDLST_CH_CFG_19_REG            (CMDLST_BASE + 0xA00) /* Channel configuration register. */
#define CMDLST_CH_CFG_20_REG            (CMDLST_BASE + 0xA80) /* Channel configuration register. */
#define CMDLST_CH_CFG_21_REG            (CMDLST_BASE + 0xB00) /* Channel configuration register. */
#define CMDLST_CH_CFG_22_REG            (CMDLST_BASE + 0xB80) /* Channel configuration register. */
#define CMDLST_CH_CFG_23_REG            (CMDLST_BASE + 0xC00) /* Channel configuration register. */
#define CMDLST_CHECK_ERROR_PTR_0_REG    (CMDLST_BASE + 0x84)  /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_1_REG    (CMDLST_BASE + 0x104) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_2_REG    (CMDLST_BASE + 0x184) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_3_REG    (CMDLST_BASE + 0x204) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_4_REG    (CMDLST_BASE + 0x284) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_5_REG    (CMDLST_BASE + 0x304) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_6_REG    (CMDLST_BASE + 0x384) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_7_REG    (CMDLST_BASE + 0x404) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_8_REG    (CMDLST_BASE + 0x484) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_9_REG    (CMDLST_BASE + 0x504) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_10_REG   (CMDLST_BASE + 0x584) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_11_REG   (CMDLST_BASE + 0x604) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_12_REG   (CMDLST_BASE + 0x684) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_13_REG   (CMDLST_BASE + 0x704) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_14_REG   (CMDLST_BASE + 0x784) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_15_REG   (CMDLST_BASE + 0x804) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_16_REG   (CMDLST_BASE + 0x884) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_17_REG   (CMDLST_BASE + 0x904) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_18_REG   (CMDLST_BASE + 0x984) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_19_REG   (CMDLST_BASE + 0xA04) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_20_REG   (CMDLST_BASE + 0xA84) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_21_REG   (CMDLST_BASE + 0xB04) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_22_REG   (CMDLST_BASE + 0xB84) /* error information ptr */
#define CMDLST_CHECK_ERROR_PTR_23_REG   (CMDLST_BASE + 0xC04) /* error information ptr */
#define CMDLST_CHECK_ERROR_TSK_0_REG    (CMDLST_BASE + 0x88)  /* error information */
#define CMDLST_CHECK_ERROR_TSK_1_REG    (CMDLST_BASE + 0x108) /* error information */
#define CMDLST_CHECK_ERROR_TSK_2_REG    (CMDLST_BASE + 0x188) /* error information */
#define CMDLST_CHECK_ERROR_TSK_3_REG    (CMDLST_BASE + 0x208) /* error information */
#define CMDLST_CHECK_ERROR_TSK_4_REG    (CMDLST_BASE + 0x288) /* error information */
#define CMDLST_CHECK_ERROR_TSK_5_REG    (CMDLST_BASE + 0x308) /* error information */
#define CMDLST_CHECK_ERROR_TSK_6_REG    (CMDLST_BASE + 0x388) /* error information */
#define CMDLST_CHECK_ERROR_TSK_7_REG    (CMDLST_BASE + 0x408) /* error information */
#define CMDLST_CHECK_ERROR_TSK_8_REG    (CMDLST_BASE + 0x488) /* error information */
#define CMDLST_CHECK_ERROR_TSK_9_REG    (CMDLST_BASE + 0x508) /* error information */
#define CMDLST_CHECK_ERROR_TSK_10_REG   (CMDLST_BASE + 0x588) /* error information */
#define CMDLST_CHECK_ERROR_TSK_11_REG   (CMDLST_BASE + 0x608) /* error information */
#define CMDLST_CHECK_ERROR_TSK_12_REG   (CMDLST_BASE + 0x688) /* error information */
#define CMDLST_CHECK_ERROR_TSK_13_REG   (CMDLST_BASE + 0x708) /* error information */
#define CMDLST_CHECK_ERROR_TSK_14_REG   (CMDLST_BASE + 0x788) /* error information */
#define CMDLST_CHECK_ERROR_TSK_15_REG   (CMDLST_BASE + 0x808) /* error information */
#define CMDLST_CHECK_ERROR_TSK_16_REG   (CMDLST_BASE + 0x888) /* error information */
#define CMDLST_CHECK_ERROR_TSK_17_REG   (CMDLST_BASE + 0x908) /* error information */
#define CMDLST_CHECK_ERROR_TSK_18_REG   (CMDLST_BASE + 0x988) /* error information */
#define CMDLST_CHECK_ERROR_TSK_19_REG   (CMDLST_BASE + 0xA08) /* error information */
#define CMDLST_CHECK_ERROR_TSK_20_REG   (CMDLST_BASE + 0xA88) /* error information */
#define CMDLST_CHECK_ERROR_TSK_21_REG   (CMDLST_BASE + 0xB08) /* error information */
#define CMDLST_CHECK_ERROR_TSK_22_REG   (CMDLST_BASE + 0xB88) /* error information */
#define CMDLST_CHECK_ERROR_TSK_23_REG   (CMDLST_BASE + 0xC08) /* error information */
#define CMDLST_CHECK_ERROR_CMD_0_REG    (CMDLST_BASE + 0x8C)  /* error information */
#define CMDLST_CHECK_ERROR_CMD_1_REG    (CMDLST_BASE + 0x10C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_2_REG    (CMDLST_BASE + 0x18C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_3_REG    (CMDLST_BASE + 0x20C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_4_REG    (CMDLST_BASE + 0x28C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_5_REG    (CMDLST_BASE + 0x30C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_6_REG    (CMDLST_BASE + 0x38C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_7_REG    (CMDLST_BASE + 0x40C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_8_REG    (CMDLST_BASE + 0x48C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_9_REG    (CMDLST_BASE + 0x50C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_10_REG   (CMDLST_BASE + 0x58C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_11_REG   (CMDLST_BASE + 0x60C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_12_REG   (CMDLST_BASE + 0x68C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_13_REG   (CMDLST_BASE + 0x70C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_14_REG   (CMDLST_BASE + 0x78C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_15_REG   (CMDLST_BASE + 0x80C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_16_REG   (CMDLST_BASE + 0x88C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_17_REG   (CMDLST_BASE + 0x90C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_18_REG   (CMDLST_BASE + 0x98C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_19_REG   (CMDLST_BASE + 0xA0C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_20_REG   (CMDLST_BASE + 0xA8C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_21_REG   (CMDLST_BASE + 0xB0C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_22_REG   (CMDLST_BASE + 0xB8C) /* error information */
#define CMDLST_CHECK_ERROR_CMD_23_REG   (CMDLST_BASE + 0xC0C) /* error information */
#define CMDLST_SW_BRANCH_0_REG          (CMDLST_BASE + 0x90)  /* Branching request */
#define CMDLST_SW_BRANCH_1_REG          (CMDLST_BASE + 0x110) /* Branching request */
#define CMDLST_SW_BRANCH_2_REG          (CMDLST_BASE + 0x190) /* Branching request */
#define CMDLST_SW_BRANCH_3_REG          (CMDLST_BASE + 0x210) /* Branching request */
#define CMDLST_SW_BRANCH_4_REG          (CMDLST_BASE + 0x290) /* Branching request */
#define CMDLST_SW_BRANCH_5_REG          (CMDLST_BASE + 0x310) /* Branching request */
#define CMDLST_SW_BRANCH_6_REG          (CMDLST_BASE + 0x390) /* Branching request */
#define CMDLST_SW_BRANCH_7_REG          (CMDLST_BASE + 0x410) /* Branching request */
#define CMDLST_SW_BRANCH_8_REG          (CMDLST_BASE + 0x490) /* Branching request */
#define CMDLST_SW_BRANCH_9_REG          (CMDLST_BASE + 0x510) /* Branching request */
#define CMDLST_SW_BRANCH_10_REG         (CMDLST_BASE + 0x590) /* Branching request */
#define CMDLST_SW_BRANCH_11_REG         (CMDLST_BASE + 0x610) /* Branching request */
#define CMDLST_SW_BRANCH_12_REG         (CMDLST_BASE + 0x690) /* Branching request */
#define CMDLST_SW_BRANCH_13_REG         (CMDLST_BASE + 0x710) /* Branching request */
#define CMDLST_SW_BRANCH_14_REG         (CMDLST_BASE + 0x790) /* Branching request */
#define CMDLST_SW_BRANCH_15_REG         (CMDLST_BASE + 0x810) /* Branching request */
#define CMDLST_SW_BRANCH_16_REG         (CMDLST_BASE + 0x890) /* Branching request */
#define CMDLST_SW_BRANCH_17_REG         (CMDLST_BASE + 0x910) /* Branching request */
#define CMDLST_SW_BRANCH_18_REG         (CMDLST_BASE + 0x990) /* Branching request */
#define CMDLST_SW_BRANCH_19_REG         (CMDLST_BASE + 0xA10) /* Branching request */
#define CMDLST_SW_BRANCH_20_REG         (CMDLST_BASE + 0xA90) /* Branching request */
#define CMDLST_SW_BRANCH_21_REG         (CMDLST_BASE + 0xB10) /* Branching request */
#define CMDLST_SW_BRANCH_22_REG         (CMDLST_BASE + 0xB90) /* Branching request */
#define CMDLST_SW_BRANCH_23_REG         (CMDLST_BASE + 0xC10) /* Branching request */
#define CMDLST_LAST_EXEC_PTR_0_REG      (CMDLST_BASE + 0x94)  /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_1_REG      (CMDLST_BASE + 0x114) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_2_REG      (CMDLST_BASE + 0x194) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_3_REG      (CMDLST_BASE + 0x214) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_4_REG      (CMDLST_BASE + 0x294) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_5_REG      (CMDLST_BASE + 0x314) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_6_REG      (CMDLST_BASE + 0x394) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_7_REG      (CMDLST_BASE + 0x414) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_8_REG      (CMDLST_BASE + 0x494) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_9_REG      (CMDLST_BASE + 0x514) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_10_REG     (CMDLST_BASE + 0x594) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_11_REG     (CMDLST_BASE + 0x614) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_12_REG     (CMDLST_BASE + 0x694) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_13_REG     (CMDLST_BASE + 0x714) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_14_REG     (CMDLST_BASE + 0x794) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_15_REG     (CMDLST_BASE + 0x814) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_16_REG     (CMDLST_BASE + 0x894) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_17_REG     (CMDLST_BASE + 0x914) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_18_REG     (CMDLST_BASE + 0x994) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_19_REG     (CMDLST_BASE + 0xA14) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_20_REG     (CMDLST_BASE + 0xA94) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_21_REG     (CMDLST_BASE + 0xB14) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_22_REG     (CMDLST_BASE + 0xB94) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_PTR_23_REG     (CMDLST_BASE + 0xC14) /* last CVDR_RD_DATA_3 */
#define CMDLST_LAST_EXEC_INFO_0_REG     (CMDLST_BASE + 0x98)  /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_1_REG     (CMDLST_BASE + 0x118) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_2_REG     (CMDLST_BASE + 0x198) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_3_REG     (CMDLST_BASE + 0x218) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_4_REG     (CMDLST_BASE + 0x298) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_5_REG     (CMDLST_BASE + 0x318) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_6_REG     (CMDLST_BASE + 0x398) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_7_REG     (CMDLST_BASE + 0x418) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_8_REG     (CMDLST_BASE + 0x498) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_9_REG     (CMDLST_BASE + 0x518) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_10_REG    (CMDLST_BASE + 0x598) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_11_REG    (CMDLST_BASE + 0x618) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_12_REG    (CMDLST_BASE + 0x698) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_13_REG    (CMDLST_BASE + 0x718) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_14_REG    (CMDLST_BASE + 0x798) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_15_REG    (CMDLST_BASE + 0x818) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_16_REG    (CMDLST_BASE + 0x898) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_17_REG    (CMDLST_BASE + 0x918) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_18_REG    (CMDLST_BASE + 0x998) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_19_REG    (CMDLST_BASE + 0xA18) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_20_REG    (CMDLST_BASE + 0xA98) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_21_REG    (CMDLST_BASE + 0xB18) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_22_REG    (CMDLST_BASE + 0xB98) /* last executed TASK ID */
#define CMDLST_LAST_EXEC_INFO_23_REG    (CMDLST_BASE + 0xC18) /* last executed TASK ID */
#define CMDLST_SW_CH_MNGR_0_REG         (CMDLST_BASE + 0x9C)  /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_1_REG         (CMDLST_BASE + 0x11C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_2_REG         (CMDLST_BASE + 0x19C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_3_REG         (CMDLST_BASE + 0x21C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_4_REG         (CMDLST_BASE + 0x29C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_5_REG         (CMDLST_BASE + 0x31C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_6_REG         (CMDLST_BASE + 0x39C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_7_REG         (CMDLST_BASE + 0x41C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_8_REG         (CMDLST_BASE + 0x49C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_9_REG         (CMDLST_BASE + 0x51C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_10_REG        (CMDLST_BASE + 0x59C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_11_REG        (CMDLST_BASE + 0x61C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_12_REG        (CMDLST_BASE + 0x69C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_13_REG        (CMDLST_BASE + 0x71C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_14_REG        (CMDLST_BASE + 0x79C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_15_REG        (CMDLST_BASE + 0x81C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_16_REG        (CMDLST_BASE + 0x89C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_17_REG        (CMDLST_BASE + 0x91C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_18_REG        (CMDLST_BASE + 0x99C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_19_REG        (CMDLST_BASE + 0xA1C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_20_REG        (CMDLST_BASE + 0xA9C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_21_REG        (CMDLST_BASE + 0xB1C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_22_REG        (CMDLST_BASE + 0xB9C) /* Priority/resource/task ID */
#define CMDLST_SW_CH_MNGR_23_REG        (CMDLST_BASE + 0xC1C) /* Priority/resource/task ID */
#define CMDLST_SW_CVDR_RD_ADDR_0_REG    (CMDLST_BASE + 0xA0)  /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_1_REG    (CMDLST_BASE + 0x120) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_2_REG    (CMDLST_BASE + 0x1A0) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_3_REG    (CMDLST_BASE + 0x220) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_4_REG    (CMDLST_BASE + 0x2A0) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_5_REG    (CMDLST_BASE + 0x320) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_6_REG    (CMDLST_BASE + 0x3A0) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_7_REG    (CMDLST_BASE + 0x420) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_8_REG    (CMDLST_BASE + 0x4A0) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_9_REG    (CMDLST_BASE + 0x520) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_10_REG   (CMDLST_BASE + 0x5A0) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_11_REG   (CMDLST_BASE + 0x620) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_12_REG   (CMDLST_BASE + 0x6A0) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_13_REG   (CMDLST_BASE + 0x720) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_14_REG   (CMDLST_BASE + 0x7A0) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_15_REG   (CMDLST_BASE + 0x820) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_16_REG   (CMDLST_BASE + 0x8A0) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_17_REG   (CMDLST_BASE + 0x920) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_18_REG   (CMDLST_BASE + 0x9A0) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_19_REG   (CMDLST_BASE + 0xA20) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_20_REG   (CMDLST_BASE + 0xAA0) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_21_REG   (CMDLST_BASE + 0xB20) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_22_REG   (CMDLST_BASE + 0xBA0) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_ADDR_23_REG   (CMDLST_BASE + 0xC20) /* CVDR Video port read address to */
#define CMDLST_SW_CVDR_RD_DATA_0_0_REG  (CMDLST_BASE + 0xA4)  /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_1_REG  (CMDLST_BASE + 0x124) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_2_REG  (CMDLST_BASE + 0x1A4) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_3_REG  (CMDLST_BASE + 0x224) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_4_REG  (CMDLST_BASE + 0x2A4) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_5_REG  (CMDLST_BASE + 0x324) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_6_REG  (CMDLST_BASE + 0x3A4) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_7_REG  (CMDLST_BASE + 0x424) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_8_REG  (CMDLST_BASE + 0x4A4) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_9_REG  (CMDLST_BASE + 0x524) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_10_REG (CMDLST_BASE + 0x5A4) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_11_REG (CMDLST_BASE + 0x624) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_12_REG (CMDLST_BASE + 0x6A4) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_13_REG (CMDLST_BASE + 0x724) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_14_REG (CMDLST_BASE + 0x7A4) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_15_REG (CMDLST_BASE + 0x824) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_16_REG (CMDLST_BASE + 0x8A4) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_17_REG (CMDLST_BASE + 0x924) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_18_REG (CMDLST_BASE + 0x9A4) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_19_REG (CMDLST_BASE + 0xA24) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_20_REG (CMDLST_BASE + 0xAA4) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_21_REG (CMDLST_BASE + 0xB24) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_22_REG (CMDLST_BASE + 0xBA4) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_0_23_REG (CMDLST_BASE + 0xC24) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_SW_CVDR_RD_DATA_1_0_REG  (CMDLST_BASE + 0xA8)  /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_1_REG  (CMDLST_BASE + 0x128) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_2_REG  (CMDLST_BASE + 0x1A8) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_3_REG  (CMDLST_BASE + 0x228) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_4_REG  (CMDLST_BASE + 0x2A8) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_5_REG  (CMDLST_BASE + 0x328) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_6_REG  (CMDLST_BASE + 0x3A8) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_7_REG  (CMDLST_BASE + 0x428) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_8_REG  (CMDLST_BASE + 0x4A8) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_9_REG  (CMDLST_BASE + 0x528) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_10_REG (CMDLST_BASE + 0x5A8) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_11_REG (CMDLST_BASE + 0x628) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_12_REG (CMDLST_BASE + 0x6A8) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_13_REG (CMDLST_BASE + 0x728) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_14_REG (CMDLST_BASE + 0x7A8) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_15_REG (CMDLST_BASE + 0x828) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_16_REG (CMDLST_BASE + 0x8A8) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_17_REG (CMDLST_BASE + 0x928) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_18_REG (CMDLST_BASE + 0x9A8) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_19_REG (CMDLST_BASE + 0xA28) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_20_REG (CMDLST_BASE + 0xAA8) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_21_REG (CMDLST_BASE + 0xB28) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_22_REG (CMDLST_BASE + 0xBA8) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_1_23_REG (CMDLST_BASE + 0xC28) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_SW_CVDR_RD_DATA_2_0_REG  (CMDLST_BASE + 0xAC)  /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_1_REG  (CMDLST_BASE + 0x12C) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_2_REG  (CMDLST_BASE + 0x1AC) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_3_REG  (CMDLST_BASE + 0x22C) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_4_REG  (CMDLST_BASE + 0x2AC) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_5_REG  (CMDLST_BASE + 0x32C) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_6_REG  (CMDLST_BASE + 0x3AC) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_7_REG  (CMDLST_BASE + 0x42C) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_8_REG  (CMDLST_BASE + 0x4AC) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_9_REG  (CMDLST_BASE + 0x52C) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_10_REG (CMDLST_BASE + 0x5AC) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_11_REG (CMDLST_BASE + 0x62C) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_12_REG (CMDLST_BASE + 0x6AC) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_13_REG (CMDLST_BASE + 0x72C) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_14_REG (CMDLST_BASE + 0x7AC) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_15_REG (CMDLST_BASE + 0x82C) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_16_REG (CMDLST_BASE + 0x8AC) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_17_REG (CMDLST_BASE + 0x92C) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_18_REG (CMDLST_BASE + 0x9AC) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_19_REG (CMDLST_BASE + 0xA2C) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_20_REG (CMDLST_BASE + 0xAAC) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_21_REG (CMDLST_BASE + 0xB2C) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_22_REG (CMDLST_BASE + 0xBAC) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_2_23_REG (CMDLST_BASE + 0xC2C) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_SW_CVDR_RD_DATA_3_0_REG  (CMDLST_BASE + 0xB0)  /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_1_REG  (CMDLST_BASE + 0x130) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_2_REG  (CMDLST_BASE + 0x1B0) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_3_REG  (CMDLST_BASE + 0x230) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_4_REG  (CMDLST_BASE + 0x2B0) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_5_REG  (CMDLST_BASE + 0x330) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_6_REG  (CMDLST_BASE + 0x3B0) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_7_REG  (CMDLST_BASE + 0x430) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_8_REG  (CMDLST_BASE + 0x4B0) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_9_REG  (CMDLST_BASE + 0x530) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_10_REG (CMDLST_BASE + 0x5B0) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_11_REG (CMDLST_BASE + 0x630) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_12_REG (CMDLST_BASE + 0x6B0) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_13_REG (CMDLST_BASE + 0x730) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_14_REG (CMDLST_BASE + 0x7B0) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_15_REG (CMDLST_BASE + 0x830) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_16_REG (CMDLST_BASE + 0x8B0) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_17_REG (CMDLST_BASE + 0x930) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_18_REG (CMDLST_BASE + 0x9B0) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_19_REG (CMDLST_BASE + 0xA30) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_20_REG (CMDLST_BASE + 0xAB0) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_21_REG (CMDLST_BASE + 0xB30) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_22_REG (CMDLST_BASE + 0xBB0) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_SW_CVDR_RD_DATA_3_23_REG (CMDLST_BASE + 0xC30) /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_HW_CH_MNGR_0_REG         (CMDLST_BASE + 0xB4)  /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_1_REG         (CMDLST_BASE + 0x134) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_2_REG         (CMDLST_BASE + 0x1B4) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_3_REG         (CMDLST_BASE + 0x234) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_4_REG         (CMDLST_BASE + 0x2B4) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_5_REG         (CMDLST_BASE + 0x334) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_6_REG         (CMDLST_BASE + 0x3B4) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_7_REG         (CMDLST_BASE + 0x434) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_8_REG         (CMDLST_BASE + 0x4B4) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_9_REG         (CMDLST_BASE + 0x534) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_10_REG        (CMDLST_BASE + 0x5B4) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_11_REG        (CMDLST_BASE + 0x634) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_12_REG        (CMDLST_BASE + 0x6B4) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_13_REG        (CMDLST_BASE + 0x734) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_14_REG        (CMDLST_BASE + 0x7B4) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_15_REG        (CMDLST_BASE + 0x834) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_16_REG        (CMDLST_BASE + 0x8B4) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_17_REG        (CMDLST_BASE + 0x934) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_18_REG        (CMDLST_BASE + 0x9B4) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_19_REG        (CMDLST_BASE + 0xA34) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_20_REG        (CMDLST_BASE + 0xAB4) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_21_REG        (CMDLST_BASE + 0xB34) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_22_REG        (CMDLST_BASE + 0xBB4) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CH_MNGR_23_REG        (CMDLST_BASE + 0xC34) /* Allows to control the priority of the channel by HW command list queues.And stall channel correctly using task ID. */
#define CMDLST_HW_CVDR_RD_ADDR_0_REG    (CMDLST_BASE + 0xB8)  /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_1_REG    (CMDLST_BASE + 0x138) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_2_REG    (CMDLST_BASE + 0x1B8) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_3_REG    (CMDLST_BASE + 0x238) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_4_REG    (CMDLST_BASE + 0x2B8) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_5_REG    (CMDLST_BASE + 0x338) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_6_REG    (CMDLST_BASE + 0x3B8) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_7_REG    (CMDLST_BASE + 0x438) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_8_REG    (CMDLST_BASE + 0x4B8) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_9_REG    (CMDLST_BASE + 0x538) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_10_REG   (CMDLST_BASE + 0x5B8) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_11_REG   (CMDLST_BASE + 0x638) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_12_REG   (CMDLST_BASE + 0x6B8) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_13_REG   (CMDLST_BASE + 0x738) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_14_REG   (CMDLST_BASE + 0x7B8) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_15_REG   (CMDLST_BASE + 0x838) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_16_REG   (CMDLST_BASE + 0x8B8) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_17_REG   (CMDLST_BASE + 0x938) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_18_REG   (CMDLST_BASE + 0x9B8) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_19_REG   (CMDLST_BASE + 0xA38) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_20_REG   (CMDLST_BASE + 0xAB8) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_21_REG   (CMDLST_BASE + 0xB38) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_22_REG   (CMDLST_BASE + 0xBB8) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_ADDR_23_REG   (CMDLST_BASE + 0xC38) /* CVDR Video port read address to write the configuration data (CMDLST_HW_CVDR_RD_DATA*) */
#define CMDLST_HW_CVDR_RD_DATA_0_0_REG  (CMDLST_BASE + 0xBC)  /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_1_REG  (CMDLST_BASE + 0x13C) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_2_REG  (CMDLST_BASE + 0x1BC) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_3_REG  (CMDLST_BASE + 0x23C) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_4_REG  (CMDLST_BASE + 0x2BC) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_5_REG  (CMDLST_BASE + 0x33C) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_6_REG  (CMDLST_BASE + 0x3BC) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_7_REG  (CMDLST_BASE + 0x43C) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_8_REG  (CMDLST_BASE + 0x4BC) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_9_REG  (CMDLST_BASE + 0x53C) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_10_REG (CMDLST_BASE + 0x5BC) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_11_REG (CMDLST_BASE + 0x63C) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_12_REG (CMDLST_BASE + 0x6BC) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_13_REG (CMDLST_BASE + 0x73C) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_14_REG (CMDLST_BASE + 0x7BC) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_15_REG (CMDLST_BASE + 0x83C) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_16_REG (CMDLST_BASE + 0x8BC) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_17_REG (CMDLST_BASE + 0x93C) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_18_REG (CMDLST_BASE + 0x9BC) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_19_REG (CMDLST_BASE + 0xA3C) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_20_REG (CMDLST_BASE + 0xABC) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_21_REG (CMDLST_BASE + 0xB3C) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_22_REG (CMDLST_BASE + 0xBBC) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_0_23_REG (CMDLST_BASE + 0xC3C) /* 1st Data to write (VP_RD_CFG) */
#define CMDLST_HW_CVDR_RD_DATA_1_0_REG  (CMDLST_BASE + 0xC0)  /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_1_REG  (CMDLST_BASE + 0x140) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_2_REG  (CMDLST_BASE + 0x1C0) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_3_REG  (CMDLST_BASE + 0x240) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_4_REG  (CMDLST_BASE + 0x2C0) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_5_REG  (CMDLST_BASE + 0x340) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_6_REG  (CMDLST_BASE + 0x3C0) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_7_REG  (CMDLST_BASE + 0x440) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_8_REG  (CMDLST_BASE + 0x4C0) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_9_REG  (CMDLST_BASE + 0x540) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_10_REG (CMDLST_BASE + 0x5C0) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_11_REG (CMDLST_BASE + 0x640) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_12_REG (CMDLST_BASE + 0x6C0) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_13_REG (CMDLST_BASE + 0x740) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_14_REG (CMDLST_BASE + 0x7C0) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_15_REG (CMDLST_BASE + 0x840) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_16_REG (CMDLST_BASE + 0x8C0) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_17_REG (CMDLST_BASE + 0x940) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_18_REG (CMDLST_BASE + 0x9C0) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_19_REG (CMDLST_BASE + 0xA40) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_20_REG (CMDLST_BASE + 0xAC0) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_21_REG (CMDLST_BASE + 0xB40) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_22_REG (CMDLST_BASE + 0xBC0) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_1_23_REG (CMDLST_BASE + 0xC40) /* 2nd data to write (VP_RD_LWG) */
#define CMDLST_HW_CVDR_RD_DATA_2_0_REG  (CMDLST_BASE + 0xC4)  /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_1_REG  (CMDLST_BASE + 0x144) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_2_REG  (CMDLST_BASE + 0x1C4) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_3_REG  (CMDLST_BASE + 0x244) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_4_REG  (CMDLST_BASE + 0x2C4) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_5_REG  (CMDLST_BASE + 0x344) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_6_REG  (CMDLST_BASE + 0x3C4) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_7_REG  (CMDLST_BASE + 0x444) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_8_REG  (CMDLST_BASE + 0x4C4) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_9_REG  (CMDLST_BASE + 0x544) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_10_REG (CMDLST_BASE + 0x5C4) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_11_REG (CMDLST_BASE + 0x644) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_12_REG (CMDLST_BASE + 0x6C4) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_13_REG (CMDLST_BASE + 0x744) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_14_REG (CMDLST_BASE + 0x7C4) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_15_REG (CMDLST_BASE + 0x844) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_16_REG (CMDLST_BASE + 0x8C4) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_17_REG (CMDLST_BASE + 0x944) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_18_REG (CMDLST_BASE + 0x9C4) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_19_REG (CMDLST_BASE + 0xA44) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_20_REG (CMDLST_BASE + 0xAC4) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_21_REG (CMDLST_BASE + 0xB44) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_22_REG (CMDLST_BASE + 0xBC4) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_2_23_REG (CMDLST_BASE + 0xC44) /* 3rd Data to write (VP_RD_FHG) */
#define CMDLST_HW_CVDR_RD_DATA_3_0_REG  (CMDLST_BASE + 0xC8)  /* 4th Data to write (VP_RD_AXI_FS) */
#define CMDLST_CMD_CFG_0_REG            (CMDLST_BASE + 0xCC)  /* Command configuration */
#define CMDLST_TOKEN_CFG_0_REG          (CMDLST_BASE + 0xD0)  /* Token configuration */
#define CMDLST_EOS_0_REG                (CMDLST_BASE + 0xD4)  /* EOS configuration */
#define CMDLST_CHECK_CTRL_0_REG         (CMDLST_BASE + 0xDC)  /* check control register */
#define CMDLST_VP_WR_FLUSH_0_REG        (CMDLST_BASE + 0xE8)  /* Force VP WR close */
#define CMDLST_DEBUG_EOS_0_REG          (CMDLST_BASE + 0xEC)  /* EOS processing debug */
#define CMDLST_DEBUG_CHANNEL_0_REG      (CMDLST_BASE + 0xF0)  /* Debug register */

#endif // __CMDLST_REG_OFFSET_H__
