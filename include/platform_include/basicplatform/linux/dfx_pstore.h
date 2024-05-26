/*
 *
 * Copyright (c) 2012-2019 Huawei Technologies Co., Ltd.
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
#ifndef __BB_DFX_PERSIST_STORE__
#define __BB_DFX_PERSIST_STORE__
#include <linux/types.h>

#ifdef CONFIG_DFX_BB
void dfx_save_pstore_log(const char *name, const void *data, size_t size);
void dfx_free_persist_store(void);
void dfx_create_pstore_entry(void);
void dfx_remove_pstore_entry(void);
void rdr_creat_console_log(void);
void rdr_remove_console_log(void);
#else
static inline void dfx_save_pstore_log(const char *name, const void *data, size_t size) {};
static inline void dfx_free_persist_store(void) {};
static inline void dfx_create_pstore_entry(void) {};
static inline void dfx_remove_pstore_entry(void) {};
static inline void rdr_creat_console_log(void) {};
static inline void rdr_remove_console_log(void) {};
#endif
#ifdef CONFIG_DFX_PSTORE
void registe_info_to_mntndump(char *big_oops_buf, size_t big_oops_buf_sz);
#endif
#endif /* __BB_DFX_PERSIST_STORE__ */
