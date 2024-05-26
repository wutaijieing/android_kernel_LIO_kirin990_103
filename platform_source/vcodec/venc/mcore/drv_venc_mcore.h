/*
 * drv_venc_mcore.h
 *
 * This is for Operations Related to mcore.
 *
 * Copyright (c) 2021-2021 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _DRV_VENC_MCU_H_
#define _DRV_VENC_MCU_H_

#include <linux/types.h>

int32_t venc_mcore_open(void);
void venc_mcore_close(void);
void venc_mcore_log_dump(void);
int32_t venc_mcore_load_image(void);
#endif