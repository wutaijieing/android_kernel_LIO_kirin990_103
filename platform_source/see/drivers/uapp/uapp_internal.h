/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: internal header of UAPP (Unique Auth Per Phone)
 * Create: 2022/04/20
 */

#ifndef __UAPP_INTERNAL_H__
#define __UAPP_INTERNAL_H__

#include <linux/types.h>                       /* uint32_t */

/* kernel to atf smc id */
#define FID_UAPP_GET_ENABLE_STATE              0xC3002F00u
#define FID_UAPP_SET_ENABLE_STATE              0xC3002F01u
#define FID_UAPP_VALID_BINDFILE_PUBKEY         0xC3002F02u
#define FID_UAPP_REVOKE_BINDFILE_PUBKEY        0xC3002F03u

#define UAPP_BINDFILE_PUBKEY_INDEX_MAX         32u

#endif /* __UAPP_INTERNAL_H__ */
