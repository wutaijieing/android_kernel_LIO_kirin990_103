/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
 * Description: crypto engine at command common interface.
 * Create: 2020/4/9
 */
#ifndef CRYPTO_ENGINE_AT_H
#define CRYPTO_ENGINE_AT_H
#include <linux/types.h>
#include <linux/device.h>

ssize_t mspe_at_ctrl_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count);

ssize_t mspe_at_ctrl_show(struct device *dev, struct device_attribute *attr, char *buf);

#endif /* CRYPTO_ENGINE_AT_H */
