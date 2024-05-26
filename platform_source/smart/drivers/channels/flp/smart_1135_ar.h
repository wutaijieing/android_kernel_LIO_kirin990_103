/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2020. All rights reserved.
 * Description: Contexthub activity recognition driver.
 * Create: 2017-03-31
 */

#ifndef	SMART_1135_AR_H
#define	SMART_1135_AR_H
#include <linux/of.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/completion.h>
#include "smart_softtimer.h"

int get_ar_data_from_mcu_ext(unsigned char service_id, unsigned char command_id, unsigned char *data, int data_len);
int send_cmd_ext_intputhub(unsigned char cmd_type, unsigned int subtype, const char *buf, size_t count);
void register_data_callback_from_1135(void);
void unregister_data_callback_from_1135(void);

#endif
