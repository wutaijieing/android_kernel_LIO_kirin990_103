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

#ifndef KUNIT_H_XML
#define KUNIT_H_XML

#include "kerror.h"
#include "regist.h"
#include "test.h"

/** kunit version number. */
#define KU_VERSION "1.0"

/** Maximum length to write XML string. */
#define BUFFER_SIZE 1024

ku_error_code ku_list_tests_to_file(void);
void ku_automated_run_tests(void);
void ku_set_output_filename(const char* sz_file_name_root);

#endif
