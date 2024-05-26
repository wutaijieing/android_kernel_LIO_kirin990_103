/*
 * reserved_mempool.h
 *
 * reserved memory managing for sharing memory with TEE.
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
#ifndef RESERVED_MEMPOOOL_H
#define RESERVED_MEMPOOOL_H

#include <linux/kernel.h>
#include <linux/types.h>

#ifndef CONFIG_MAX_RES_MEM_ORDER
#define CONFIG_MAX_RES_MEM_ORDER 15
#endif

#define RESERVED_MEM_POOL_SIZE 0x8000000 // 128M
#define RESEVED_MAX_ALLOC_SIZE (1 << (CONFIG_MAX_RES_MEM_ORDER + PAGE_SHIFT)) // 2^max_order * 4K
#define LIMIT_RES_MEM_ORDER 15

int load_reserved_mem(void);
void *reserved_mem_alloc(size_t size);
void free_reserved_mempool(void);
int reserved_mempool_init(void);
void reserved_mem_free(const void *ptr);
bool exist_res_mem(void);
unsigned long res_mem_virt_to_phys(unsigned long vaddr);
#endif
