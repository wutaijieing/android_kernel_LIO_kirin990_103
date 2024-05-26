/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2019. All rights reserved.
 * Description: image load teek_init.
 * Create: 2019/12/03
 */
#ifndef __LOAD_IMAGE_EXTERNAL_H__
#define __LOAD_IMAGE_EXTERNAL_H__
#include "teek_client_type.h"
#ifdef __cplusplus
extern "C"
{
#endif

int teek_init(TEEC_Session *session, TEEC_Context *context);

#ifdef __cplusplus
}
#endif
#endif

