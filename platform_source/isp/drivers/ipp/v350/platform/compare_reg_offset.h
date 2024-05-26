// ******************************************************************************
// Copyright     :  Copyright (C) 2020, Hisilicon Technologies Co. Ltd.
// File name     :  slam_compare_reg_offset.h
// Project line  :  Platform And Key Technologies Development
// Department    :  CAD Development Department
// Date          :  2020/04/10
// Description   :
// Others        :  Generated automatically by nManager V4.2
// ******************************************************************************

#ifndef __SLAM_COMPARE_REG_OFFSET_H__
#define __SLAM_COMPARE_REG_OFFSET_H__

/* SLAM_COMPARE Base address of Module's Register */
#define SLAM_COMPARE_BASE                       (0x0)

/******************************************************************************/
/*                      SLAM_COMPARE Registers' Definitions                            */
/******************************************************************************/

#define COMPARE_COMPARE_CFG_REG       (SLAM_COMPARE_BASE + 0x0)   /* Compare Config */
#define COMPARE_BLOCK_CFG_REG         (SLAM_COMPARE_BASE + 0x4)   /* BLOCK Config */
#define COMPARE_SEARCH_CFG_REG        (SLAM_COMPARE_BASE + 0x8)   /* Search */
#define COMPARE_STAT_CFG_REG          (SLAM_COMPARE_BASE + 0xC)   /* Statistic */
#define COMPARE_PREFETCH_CFG_REG      (SLAM_COMPARE_BASE + 0x10)  /* Configure the prefetch */
#define COMPARE_OFFSET_CFG_REG        (SLAM_COMPARE_BASE + 0x14)  /* Configure the center offset */
#define COMPARE_TOTAL_KPT_NUM_REG     (SLAM_COMPARE_BASE + 0x18)  /* Total kpt number */
#define COMPARE_DEBUG_0_REG           (SLAM_COMPARE_BASE + 0x20)  /* Debug 0 */
#define COMPARE_DEBUG_1_REG           (SLAM_COMPARE_BASE + 0x24)  /* Debug 1 */
#define COMPARE_EC_0_REG              (SLAM_COMPARE_BASE + 0x28)  /* ec 0 */
#define COMPARE_EC_1_REG              (SLAM_COMPARE_BASE + 0x2C)  /* ec 1 */
#define COMPARE_REF_KPT_NUMBER_0_REG  (SLAM_COMPARE_BASE + 0x100) /* Reference feature number in each block */
#define COMPARE_CUR_KPT_NUMBER_0_REG  (SLAM_COMPARE_BASE + 0x300) /* Current feature number in each block */

#define COMPARE_MATCH_POINTS_REG      (SLAM_COMPARE_BASE + 0x4F0) /* Matched points number */

#define COMPARE_INDEX_0_REG           (SLAM_COMPARE_BASE + 0x500) /* Matched Pairs */
#define COMPARE_DISTANCE_0_REG        (SLAM_COMPARE_BASE + 0x504) /* Matched distance */

#endif // __SLAM_COMPARE_REG_OFFSET_H__
