/*
 * ISP driver, isp_mempool.h
 *
 * Copyright (c) 2013 ISP Technologies CO., Ltd.
 *
 */

#ifndef _PLAT_MEMPOOL_HISP_H
#define _PLAT_MEMPOOL_HISP_H

#include <linux/version.h>
#include <linux/scatterlist.h>
#include <linux/printk.h>
#include <linux/genalloc.h>
#include <linux/list.h>
#include <linux/mutex.h>

#ifdef __cplusplus
extern "C"
{
#endif

enum maptype {
	MAP_TYPE_DYNAMIC    = 0,
	MAP_TYPE_RAW2YUV,
	MAP_TYPE_STATIC,
	MAP_TYPE_STATIC_SEC,
	MAP_TYPE_DYNAMIC_CARVEOUT,
	MAP_TYPE_STATIC_ISP_SEC,
	MAP_TYPE_DYNAMIC_SEC,
	MAP_TYPE_COREDUMP   = 10,
	MAP_TYPE_MAX,
};

/* hisp map addr for ispcpu */
unsigned int hisp_alloc_cpu_map_addr(struct scatterlist *sgl,
		unsigned int prot, unsigned int size, unsigned int align);
int hisp_free_cpu_map_addr(unsigned int iova, unsigned int size);

/* hisp mempool api */
unsigned long hisp_mem_pool_alloc_iova(unsigned int size,
			unsigned int pool_num);
int hisp_mem_pool_free_iova(unsigned int pool_num,
				unsigned int va, unsigned int size);
unsigned int hisp_mem_map_setup(struct scatterlist *sgl,
				unsigned int iova, unsigned int size,
				unsigned int prot, unsigned int pool_num,
				unsigned int flag, unsigned int align);
void hisp_mem_pool_destroy(unsigned int pool_num);
void hisp_dynamic_mem_pool_clean(void);
unsigned int hisp_mem_pool_alloc_carveout(size_t size, unsigned int type);

/* MDC reserved memory */
static inline int hisp_mem_pool_free_carveout(unsigned int iova,
    size_t size) { return 0;  }

#ifdef __cplusplus
}
#endif

#endif

