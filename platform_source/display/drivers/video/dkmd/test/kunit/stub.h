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

#ifndef KUNIT_H_STUB
#define KUNIT_H_STUB

typedef void (*common_func)(void);

void clear_all_func_stubs(void);
int set_func_stub(common_func fn_old_func, common_func fn_new_func);
int set_func_stub_none(common_func fn_old_func);

#define SET_FUNC_STUB(old_func, new_func) set_func_stub((common_func)old_func, (common_func)new_func)
#define CLEAR_ALL_FUNC_STUBS clear_all_func_stubs()
#define CLEAR_FUNC_STUB(old_func)  set_func_stub_none((common_func)old_func)

#endif
