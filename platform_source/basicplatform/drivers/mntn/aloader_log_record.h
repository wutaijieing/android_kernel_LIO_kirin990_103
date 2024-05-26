/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 *
 * head file of aloader_log
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef __ALOADER_LOG_RECORD_H__
#define __ALOADER_LOG_RECORD_H__
#include <platform_include/basicplatform/linux/rdr_platform.h>

#define ALOADER_DUMP_LOG_ADDR RESERVED_ALOADER_LOG_PHYMEM_BASE
#define ALOADER_DUMP_LOG_SIZE RESERVED_ALOADER_LOG_PHYMEM_SIZE

#define ALOADER_LOG_FILE       "/proc/rdr/log/aloader_log"
#define LAST_ALOADER_LOG_FILE  "/proc/rdr/log/last_aloader_log"

void save_aloader_log(const char *dst_dir_str);
#endif
