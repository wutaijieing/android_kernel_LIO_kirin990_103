 /*
  * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
  * Description: stop page_referenced() in advance to reduce vma walk.
  * Author: Gong Chen <gongchen4@huawei.com>
  * Create: 2021-12-09
  */
#ifndef _STOP_PAGE_REF_
#define _STOP_PAGE_REF_

#include <linux/mm_types.h>

enum page_ref_type {
	ACTIVE_REF = 0,
	INACTIVE_REF,
	INVALID_REF,
};

#ifdef CONFIG_STOP_PAGE_REF_DEBUG
void count_ref_pages(int ref, enum page_ref_type type);
#else
static inline void count_ref_pages(int ref, enum page_ref_type type)
{
}
#endif

int page_referenced_spr(struct page *, int is_locked,
			struct mem_cgroup *memcg,
			unsigned long *vm_flags,
			int ref_type);

int get_page_ref_type(struct page *page, int ref_type);

bool should_stop_page_ref(struct page *page, int referenced, int ref_type,
			  unsigned long vm_flags);
#endif
