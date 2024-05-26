/**
 * @file chash.h
 * @brief Interface for cmdlist hash function
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2021. All rights reserved.
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

#ifndef __CHASH_H__
#define __CHASH_H__

#include <linux/slab.h>

#include "clist.h"

typedef bool (*compare_func)(const struct cnode *a, const struct cnode *b);

/**
 * @brief this is a fake hash table, to manage different scene created cmdlist node
 *
 */
struct chash_table {
	size_t bkt_size;
	compare_func equal;
	struct clist_head *bkts;
};

static inline int chash_init(struct chash_table *ht, size_t bkt_size, compare_func equal)
{
	size_t i;

	ht->bkt_size = bkt_size;
	ht->equal = equal;
	ht->bkts = (struct clist_head *)kzalloc(sizeof(struct clist_head) * bkt_size, GFP_KERNEL);
	if (!ht->bkts)
		return -1;

	for (i = 0; i < bkt_size; i++)
		clist_init(&ht->bkts[i]);

	return 0;
}

static inline void chash_deinit(struct chash_table *ht, cnode_func node_release)
{
	size_t i;

	if (likely(node_release)) {
		for (i = 0; i < ht->bkt_size; i++)
			clist_deinit(&ht->bkts[i], node_release);
	}

	kfree(ht->bkts);
	ht->bkts = NULL;
}

static inline void chash_bkt_dump(struct chash_table *ht,
	size_t bkt_idx, cnode_func node_dump)
{
	struct clist_head *list = &ht->bkts[bkt_idx % ht->bkt_size];

	clist_dump(list, node_dump);
}

static inline void chash_bkt_clear(struct chash_table *ht,
	size_t bkt_idx, cnode_func node_release)
{
	struct clist_head *list = &ht->bkts[bkt_idx % ht->bkt_size];

	clist_deinit(list, node_release);
}

static inline void chash_add(struct chash_table *ht,
	size_t bkt_idx, struct cnode *node)
{
	struct clist_head *list = &ht->bkts[bkt_idx % ht->bkt_size];

	clist_add_tail(list, node);
}

static inline void chash_remove(struct cnode *node)
{
	clist_remove(node);
}

static inline struct cnode *chash_find(const struct chash_table *ht,
	size_t bkt_idx, const struct cnode *cmp_node)
{
	struct cnode *node = NULL;
	struct clist_head *list = &ht->bkts[bkt_idx % ht->bkt_size];

	clist_for_each(node, list) {
		if (ht->equal(cmp_node, node))
			return node;
	}
	return NULL;
}

#endif
