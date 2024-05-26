/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef KUNIT_H_UTIL
#define KUNIT_H_UTIL
#include <linux/syscalls.h>

/** The maximum length string. */
#define KUNIT_MAX_STRING_LENGTH    1024

/** Maximum number of characters in the process of parsing XML entities. */
#define KUNIT_MAX_ENTITY_LEN 5

/** Special characters parsed. */
int ku_translate_special_characters(const char* sz_src, char* sz_dest, size_t maxlen);

#endif
