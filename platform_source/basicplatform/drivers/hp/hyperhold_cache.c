/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
 * Description: hyperhold implement
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Author:	Wang Cheng Ke <wangchengke2@huawei.com>
 *		Wang Xiao Yuan <wangxiaoyuan6@huawei.com>
 *
 * Create: 2021-4-8
 *
 */

#define pr_fmt(fmt) "[HYPERHOLD]" fmt

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/zsmalloc.h>

#include "hyperhold_internal.h"
#include "hyperhold_area.h"
#include "hyperhold_cache.h"

#ifdef CONFIG_DFX_DEBUG_FS
#include <platform_include/basicplatform/linux/rdr_platform.h>
#endif

struct extent_cache *new_extent_cache(struct page *pages[EXTENT_PG_CNT],
		bool fast, bool nofail)
{
	struct extent_cache *ext =
		hyperhold_malloc(sizeof(struct extent_cache), fast, nofail);
	int k;

	if (!ext)
		return NULL;
	for (k = 0; k < EXTENT_PG_CNT; k++) {
#ifdef CONFIG_HISI_DEBUG_FS
		if (atomic_read(&pages[k]->_refcount) != 1)
			rdr_syserr_process_for_ap((u32)MODID_AP_S_PANIC_STORAGE,
				0ull, 0ull);
#endif
		ext->pages[k] = pages[k];
	}

	return ext;
}

void free_extent_cache(struct extent_cache *ext)
{
	int k;

	if (!ext)
		return;
	hh_print(HHLOG_DEBUG, "free cache, ext = %lx\n", ext);
	for (k = 0; k < EXTENT_PG_CNT; k++)
		if (ext->pages[k])
			put_page(ext->pages[k]);
	kfree(ext);
}

static void free_extent_cache_rcu(struct rcu_head *rcu_head)
{
	struct extent_cache *ext = NULL;

	if (!rcu_head)
		return;
	ext = container_of(rcu_head, struct extent_cache, rcu_head);
	free_extent_cache(ext);
}

struct hyperhold_cache *hyperhold_cache_alloc(int nr_exts)
{
	struct hyperhold_cache *cache =
		kzalloc(sizeof(struct hyperhold_cache), GFP_KERNEL);

	if (!cache) {
		hh_print(HHLOG_ERR, "alloc hyperhold cache failed.");
		return NULL;
	}
	cache->nr_cached_exts = 0;
	spin_lock_init(&cache->lock);
	INIT_RADIX_TREE(&cache->tree, GFP_NOWAIT);

	return cache;
}

void hyperhold_cache_clear(struct hyperhold_cache *cache)
{
	void **slot = NULL;
	struct radix_tree_iter iter;
	struct extent_cache *ext = NULL;

	hh_print(HHLOG_DEBUG, "clear cache\n");
	spin_lock(&cache->lock);
	radix_tree_for_each_slot(slot, &cache->tree, &iter, 0) {
		ext = radix_tree_lookup(&cache->tree, iter.index);
		if (ext && !ext->pin) {
			radix_tree_delete(&cache->tree, iter.index);
			free_extent_cache(ext);
			cache->nr_cached_exts--;
		}
	}
	spin_unlock(&cache->lock);
}

void hyperhold_cache_free(struct hyperhold_cache *cache)
{
	hyperhold_cache_clear(cache);
	kfree(cache);
}

bool hyperhold_cache_pin(struct hyperhold_cache *cache, int ext_id)
{
	struct extent_cache *ext = NULL;
	bool ret = false;

	spin_lock(&cache->lock);
	ext = radix_tree_lookup(&cache->tree, ext_id);
	if (ext) {
		ext->pin++;
		ret = true;
	}
	spin_unlock(&cache->lock);

	return ret;
}

void hyperhold_cache_unpin(struct hyperhold_cache *cache, int ext_id)
{
	struct extent_cache *ext = NULL;

	spin_lock(&cache->lock);
	ext = radix_tree_lookup(&cache->tree, ext_id);
	if (ext)
		ext->pin--;
	spin_unlock(&cache->lock);
}

int hyperhold_cache_insert(struct hyperhold_cache *cache, int ext_id,
			struct extent_cache *ext, bool fast, bool nofail)
{
	int ret = -ENOMEM;

	hh_print(HHLOG_DEBUG, "insert cache, ext = %d(%lx)\n", ext_id, ext);
	if (fast) {
		ret = radix_tree_preload(GFP_ATOMIC);
		if (ret && nofail)
			ret = radix_tree_preload(GFP_NOIO);
	} else {
		ret = radix_tree_preload(GFP_NOIO);
	}

	if (ret) {
		hh_print(HHLOG_ERR, "preload fail, ext = %d\n", ext_id);
		return ret;
	}
	spin_lock(&cache->lock);
	ret = radix_tree_insert(&cache->tree, ext_id, ext);
	if (!ret)
		cache->nr_cached_exts++;
	else
		hh_print(HHLOG_ERR, "insert cache failed, ext = %d\n", ext_id);
	spin_unlock(&cache->lock);
	radix_tree_preload_end();

	return ret;
}

int hyperhold_cache_delete(struct hyperhold_cache *cache, int ext_id)
{
	struct extent_cache *ext = NULL;
	int ret = 0;

	spin_lock(&cache->lock);
	ext = radix_tree_lookup(&cache->tree, ext_id);
	if (ext && !ext->pin) {
		radix_tree_delete(&cache->tree, ext_id);
		cache->nr_cached_exts--;
		hh_print(HHLOG_DEBUG, "delete cache, ext = %d(%lx)\n", ext_id, ext);
	} else {
		ret = ext ? -EBUSY : -ENOENT;
	}
	spin_unlock(&cache->lock);
	if (!ret)
		call_rcu(&ext->rcu_head, free_extent_cache_rcu);

	return ret;
}

bool hyperhold_cache_lookup(struct hyperhold_cache *cache,
						int ext_id)
{
	struct extent_cache *ext = NULL;

	rcu_read_lock();
	ext = radix_tree_lookup(&cache->tree, ext_id);
	rcu_read_unlock();

	return ext != NULL;
}

int hyperhold_cache_read_obj(struct hyperhold_cache *cache,
			unsigned long addr, u8 *dst, int size)
{
	int ext_id = esentry_extid(addr);
	int pg_id = esentry_pgid(addr);
	int pg_off = esentry_pgoff(addr);
	u8 *src = NULL;
	void **slot = NULL;
	struct extent_cache *ext = NULL;
	struct page *page1 = NULL;
	struct page *page2 = NULL;

	rcu_read_lock();
repeat:
	slot = radix_tree_lookup_slot(&cache->tree, ext_id);
	if (!slot)
		goto err_out;
	ext = radix_tree_deref_slot(slot);
	if (!ext)
		goto err_out;
	if (radix_tree_exception(ext)) {
		if (radix_tree_deref_retry(ext))
			goto repeat;
		goto err_out;
	}
	page1 = ext->pages[pg_id];
	get_page(page1);
	if (pg_off + size > PAGE_SIZE) {
		page2 = ext->pages[pg_id + 1];
		get_page(page2);
	}
	rcu_read_unlock();

	hh_print(HHLOG_DEBUG, "read cache, ext = %d, addr = %lx, size = %d\n",
		ext_id, addr, size);
	src = page_to_virt(page1);
	if (pg_off + size <= PAGE_SIZE) {
		memcpy(dst, src + pg_off, size);
		put_page(page1);
	} else {
		memcpy(dst, src + pg_off, PAGE_SIZE - pg_off);
		put_page(page1);
		src = page_to_virt(page2);
		memcpy(dst + PAGE_SIZE - pg_off, src, size + pg_off - PAGE_SIZE);
		put_page(page2);
	}
	return 0;
err_out:
	rcu_read_unlock();
	return -ENOENT;
}
