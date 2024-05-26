/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * Description: This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation;
 * either version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef __ION_MM_CMA_HEAP_H
#define __ION_MM_CMA_HEAP_H

#include "ion.h"

struct ion_cma_heap {
	struct ion_heap heap;
	struct cma *cma;

#ifdef CONFIG_ION_MM_CMA_HEAP
	/* heap total size */
	size_t heap_size;
	const char *cma_name;
#endif
};

#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0))
#define to_cma_heap(x) container_of(x, struct ion_cma_heap, heap)
#endif

#ifdef CONFIG_ION_MM_CMA_HEAP
extern void ion_mm_cma_heap_ops(struct ion_cma_heap *cma_heap);
#endif

#endif
