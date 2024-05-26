/*
 * teek_load_api.h
 *
 * function declaration for load app operations for kernel CA.
 *
 * Copyright (c) 2021-2021 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#ifndef TEEK_APP_LOAD_H
#define TEEK_APP_LOAD_H

#include "teek_client_api.h"
#include "tc_ns_client.h"

#define MAX_IMAGE_LEN 0x800000 /* max image len */

int32_t teek_get_app(const char *ta_path, char **file_buf, uint32_t *file_len);
void teek_free_app(bool load_app_flag, char **file_buf);

#endif
