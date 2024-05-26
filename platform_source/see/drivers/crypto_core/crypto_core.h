/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: Header file for Crypto Core module.
 * Create: 2020/04/03
 */

#ifndef CRYPTO_CORE_H
#define CRYPTO_CORE_H

#include <linux/types.h>

#define MSPC_MODULE_UNREADY             0xC8723B6D
#define MSPC_MODULE_READY               0x378DC492

int32_t mspc_get_init_status(void);

#endif /*  CRYPTO_CORE_H */
