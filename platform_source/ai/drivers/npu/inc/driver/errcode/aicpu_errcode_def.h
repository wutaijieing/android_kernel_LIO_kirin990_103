/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: aicpu_errcode_def.h
 * Create: 2019-12-21
 */

#ifndef AICPU_ERRCODE_DEF_H
#define AICPU_ERRCODE_DEF_H

#include "infra/base/npu_error_define.h"

/* bit 31-bit30 to be hiai aicpu local */
/* bit 29 -bit28 to be hiai aicpu code type */
/* bit 27 -bit25 to be hiai aicpu  error level */
/* bit 24 -bit17 to be hiai mod */
/* bit16 -bit7 to be hiai aicpu submodule id */
#define AICP_AICPU_DEVMODULEID_MASK   0x0001FF80
#define SHIFT_SUBMODE_MASK            7
#define AICP_AICPU_MID_MASK 0x3FF
/* bit6 -bit0 to be hiai aicpu module error id */
#define AICP_AICPU_ERROR_ID_MASK      0x0000007F
#define SHIFT_ERROR_ID_MASK           0

#define AICP_AICPU_DEVMOD_CFG(mid)      (AICP_AICPU_DEVMODULEID_MASK & \
    ((((unsigned int)mid) & AICP_AICPU_MID_MASK) << SHIFT_SUBMODE_MASK))

#define AICP_AICPU_ERRID_CFG(eid)       (AICP_AICPU_ERROR_ID_MASK & ((unsigned int)eid))

#define AICPU_EID(mid, eid)             (int)(AICP_NPU_ERR_CODE_HEAD(AICP_DEVICE, \
    ERROR_CODE, NORMAL_LEVEL, AICP_AICPU) | AICP_AICPU_DEVMOD_CFG(mid) | AICP_AICPU_ERRID_CFG(eid))

#define AICPU_EID_EX(code_type, err_lvl, aicpu2ex, mid, eid)  (AICP_NPU_ERR_CODE_HEAD(AICP_DEVICE, \
    code_type, err_lvl, aicpu2ex) | AICP_AICPU_DEVMOD_CFG(mid) | AICP_AICPU_ERRID_CFG(eid))

#define AICPU_EID2FW(mid, eid)          (AICP_NPU_ERR_CODE_HEAD(AICP_DEVICE, ERROR_CODE, NORMAL_LEVEL, AICP_AICPU) | \
                                            AICP_AICPU_DEVMOD_CFG(mid) | \
                                            AICP_AICPU_ERRID_CFG(eid))
#define AICPU_EID2OS(mid, eid)             AICPU_EID2FW(mid, eid)

#endif
