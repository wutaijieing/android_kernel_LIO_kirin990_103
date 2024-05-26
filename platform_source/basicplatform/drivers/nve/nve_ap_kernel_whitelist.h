/* Copyright (c) Hisilicon Technologies Co., Ltd. 2012-2019. All rights reserved.
 * Description: define nve whitelist
 * Revision history:2019-06-20 NVE CSEC
 */
#ifndef __NVE_AP_KERNEL_WHITELIST__
#define __NVE_AP_KERNEL_WHITELIST__

/* NV IDs to be protected */
static uint32_t nv_num_whitelist[] = {2, 193, 194, 364};
/*
  processes that could modify protected NV IDs.
  process name length MUST be less than 16 Bytes.
*/
static char *nv_process_whitelist[] = {"oeminfo_nvm_ser"};

#endif
