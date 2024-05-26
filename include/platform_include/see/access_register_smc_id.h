/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2014-2021. All rights reserved.
 * Description: kernel and bl31 smc id
 * Create: 2014/5/16
 */

#ifndef __ACCESS_REGISTER_SMC_ID_H__
#define __ACCESS_REGISTER_SMC_ID_H__

/*
 * this file should not be added any more,
 * if need to add smc id, please add in bl31_smc.h
 */

/* list sub-function id for access register service */
#define ACCESS_REGISTER_FN_MAIN_ID                          0xc500aa01UL
#define ACCESS_REGISTER_FN_SUB_ID_DDR_INTLV                 0x55bbcce0UL
#define ACCESS_REGISTER_FN_SUB_ID_DDR_FLUX_W                0x55bbcce1UL
#define ACCESS_REGISTER_FN_SUB_ID_DDR_FLUX_R                0x55bbcce2UL
#define ACCESS_REGISTER_FN_SUB_ID_DDR_DRAM_R                0x55bbcce3UL
#define ACCESS_REGISTER_FN_SUB_ID_DDR_STDID_W               0x55bbcce4UL
#define ACCESS_REGISTER_FN_SUB_ID_MASTER_SECURITY_CONFIG    0x55bbcce7UL
#define ACCESS_REGISTER_FN_SUB_ID_DDR_MODEM_SEC             0x55bbcce9UL
#define ACCESS_REGISTER_FN_SUB_ID_DDR_KERNEL_CODE_PROTECT   0x55bbccedUL
#define ACCESS_REGISTER_FN_SUB_ID_SECS_POWER_CTRL           0x55bbccf0UL
#define ACCESS_REGISTER_FN_SUB_ID_DDR_PERF_CTRL             0x55bbccf3UL

#endif
