/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Header file for Crypto Core smx.
 * Create: 2020/03/01
 */

#ifndef CRYPTO_CORE_SMX_H
#define CRYPTO_CORE_SMX_H

#include <linux/types.h>

#define SMX_ENABLE        0x5A5AA5A5
#define SMX_DISABLE       0xA5A55A5A

int32_t mspc_get_smx_status(void);
int32_t mspc_disable_smx(void);

#endif /* CRYPTO_CORE_SMX_H */
