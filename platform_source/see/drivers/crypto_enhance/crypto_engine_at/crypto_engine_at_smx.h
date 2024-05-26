/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: crypto engine at command smx function.
 * Create: 2020/4/9
 */
#ifndef CRYPTO_ENGINE_AT_SMX_H
#define CRYPTO_ENGINE_AT_SMX_H

#include <linux/types.h>
#include <linux/kernel.h>

int32_t mspe_at_set_smx(const char *value, int32_t (*resp)(char *buf, int len));
int32_t mspe_at_get_smx(const char *value, int32_t (*resp)(char *buf, int len));

#endif /* CRYPTO_ENGINE_AT_SMX_H */
