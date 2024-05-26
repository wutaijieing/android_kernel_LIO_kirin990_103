/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Drivers for Crypto Core factory task
 * Create: 2021/06/02
 */

#ifndef CRYPTO_CORE_FACTORY_TASK_H
#define CRYPTO_CORE_FACTORY_TASK_H
#include "crypto_core_factory.h"

struct crypto_core_task {
	atomic_t state;
	atomic_t errcode;
	int32_t try_cnt;
	int32_t (*taskfn)(void);
	bool (*conditon_fn)(void);
	const char *name;
	struct mutex *lock;
};

int32_t crypto_core_create_task(struct crypto_core_task *task);

int32_t crypto_core_check_task(struct crypto_core_task *task);

#endif
