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
 * Author:	He Biao <hebiao6@huawei.com>
 *		Wang Cheng Ke <wangchengke2@huawei.com>
 *		Wang Fa <fa.wang@huawei.com>
 *
 * Create: 2020-4-16
 *
 */

#define pr_fmt(fmt) "[HYPERHOLD]" fmt

#include <linux/kernel.h>
#include <linux/swap.h>
#include <linux/memcg_policy.h>

#include "zram_drv.h"
#include "hyperhold.h"
#include "hyperhold_internal.h"

#include "hyperhold_list.h"
#include "hyperhold_area.h"
#include "hyperhold_cache.h"
#include "hyperhold_lru_rmap.h"

#ifdef CONFIG_CRYPTO_DELTA
#include "sddc/sddc.h"
#include "trace/events/block.h"
#include <linux/jiffies.h>
#endif

struct io_extent {
	int ext_id;
	struct zram *zram;
	struct mem_cgroup *mcg;
	struct page *pages[EXTENT_PG_CNT];
	u32 index[EXTENT_MAX_OBJ_CNT];
	int cnt;

	struct hyperhold_page_pool *pool;
};

static struct io_extent *alloc_io_extent(struct hyperhold_page_pool *pool,
				  bool fast, bool nofail)
{
	int i;
	struct io_extent *io_ext = hyperhold_malloc(sizeof(struct io_extent),
						     fast, nofail);

	if (!io_ext) {
		hh_print(HHLOG_ERR, "alloc io_ext failed\n");
		return NULL;
	}

	io_ext->ext_id = -EINVAL;
	io_ext->pool = pool;
	for (i = 0; i < (int)EXTENT_PG_CNT; i++) {
		io_ext->pages[i] = hyperhold_alloc_page(pool, GFP_ATOMIC,
							fast, nofail);
		if (!io_ext->pages[i]) {
			hh_print(HHLOG_ERR, "alloc page[%d] failed\n", i);
			goto page_free;
		}
	}
	return io_ext;
page_free:
	for (i = 0; i < (int)EXTENT_PG_CNT; i++)
		if (io_ext->pages[i])
			hyperhold_page_recycle(io_ext->pages[i], pool);
	hyperhold_free(io_ext);

	return NULL;
}

static void discard_io_extent(struct io_extent *io_ext, unsigned int op)
{
	struct zram *zram = NULL;
	int i;

	if (!io_ext) {
		hh_print(HHLOG_ERR, "NULL io_ext\n");
		return;
	}
	if (!io_ext->mcg)
		zram = io_ext->zram;
	else
		zram = io_ext->mcg->zram;
	if (!zram) {
		hh_print(HHLOG_ERR, "NULL zram\n");
		goto out;
	}
	for (i = 0; i < (int)EXTENT_PG_CNT; i++)
		if (io_ext->pages[i])
			hyperhold_page_recycle(io_ext->pages[i], io_ext->pool);
	if (io_ext->ext_id < 0)
		goto out;
	hh_print(HHLOG_DEBUG, "ext = %d, op = %d\n", io_ext->ext_id, op);
	if (op == REQ_OP_READ) {
		put_extent(zram->area, io_ext->ext_id);
		goto out;
	}
	for (i = 0; i < io_ext->cnt; i++) {
		u32 index = io_ext->index[i];

		zram_slot_lock(zram, index);
		if (io_ext->mcg)
			zram_lru_add_tail(zram, index, io_ext->mcg);
		zram_clear_flag(zram, index, ZRAM_UNDER_WB);
		zram_slot_unlock(zram, index);
	}
	hyperhold_free_extent(zram->area, io_ext->ext_id, true);
out:
	hyperhold_free(io_ext);
}

static void copy_to_pages(u8 *src, struct page *pages[],
		   unsigned long eswpentry, int size)
{
	u8 *dst = NULL;
	int pg_id = esentry_pgid(eswpentry);
	int offset = esentry_pgoff(eswpentry);

	if (!src) {
		hh_print(HHLOG_ERR, "NULL src\n");
		return;
	}
	if (!pages) {
		hh_print(HHLOG_ERR, "NULL pages\n");
		return;
	}
	if (size < 0 || size > (int)PAGE_SIZE) {
		hh_print(HHLOG_ERR, "size = %d invalid\n", size);
		return;
	}
	dst = page_to_virt(pages[pg_id]);
	if (offset + size <= (int)PAGE_SIZE) {
		memcpy(dst + offset, src, size);
		return;
	}
	if (pg_id == EXTENT_PG_CNT - 1) {
		hh_print(HHLOG_ERR, "ext overflow, addr = %lx, size = %d\n",
			 eswpentry, size);
		return;
	}
	memcpy(dst + offset, src, PAGE_SIZE - offset);
	dst = page_to_virt(pages[pg_id + 1]);
	memcpy(dst, src + PAGE_SIZE - offset, offset + size - PAGE_SIZE);
}

static void copy_from_pages(u8 *dst, struct page *pages[],
		     unsigned long eswpentry, int size)
{
	u8 *src = NULL;
	int pg_id = esentry_pgid(eswpentry);
	int offset = esentry_pgoff(eswpentry);

	if (!dst) {
		hh_print(HHLOG_ERR, "NULL dst\n");
		return;
	}
	if (!pages) {
		hh_print(HHLOG_ERR, "NULL pages\n");
		return;
	}
	if (size < 0 || size > (int)PAGE_SIZE) {
		hh_print(HHLOG_ERR, "size = %d invalid\n", size);
		return;
	}

	src = page_to_virt(pages[pg_id]);
	if (offset + size <= (int)PAGE_SIZE) {
		memcpy(dst, src + offset, size);
		return;
	}
	if (pg_id == EXTENT_PG_CNT - 1) {
		hh_print(HHLOG_ERR, "ext overflow, addr = %lx, size = %d\n",
			 eswpentry, size);
		return;
	}
	memcpy(dst, src + offset, PAGE_SIZE - offset);
	src = page_to_virt(pages[pg_id + 1]);
	memcpy(dst + PAGE_SIZE - offset, src, offset + size - PAGE_SIZE);
}

static bool zram_test_skip(struct zram *zram, u32 index, struct mem_cgroup *mcg)
{
	if (zram_test_flag(zram, index, ZRAM_WB))
		return true;
	if (zram_test_flag(zram, index, ZRAM_UNDER_WB))
		return true;
	if (zram_test_flag(zram, index, ZRAM_BATCHING_OUT))
		return true;
	if (zram_test_flag(zram, index, ZRAM_SAME))
		return true;
#ifdef CONFIG_CRYPTO_DELTA
	if (zram_test_flag(zram, index, ZRAM_SDDC_DUPLICATE))
		return true;
#endif
	if (mcg != zram_get_memcg(zram, index))
		return true;
	if (!zram_get_obj_size(zram, index))
		return true;

	return false;
}

static bool zram_test_overwrite(struct zram *zram, u32 index, int ext_id)
{
	unsigned long handle;
	if (!zram_test_flag(zram, index, ZRAM_WB))
		return true;
#ifdef CONFIG_ZRAM_DEDUP
	handle = zram_get_direct_handle(zram, index);
#else
	handle = zram_get_handle(zram, index);
#endif
	if (ext_id != esentry_extid(handle))
		return true;

	return false;
}

static void move_to_hyperhold(struct zram *zram, u32 index,
		       unsigned long eswpentry, struct mem_cgroup *mcg)
{
	size_t size;
#ifndef CONFIG_ZRAM_DEDUP
	unsigned long handle;
#endif
	struct hyperhold_stat *stat = hyperhold_get_stat_obj();

	if (!stat) {
		hh_print(HHLOG_ERR, "NULL stat\n");
		return;
	}
	if (!zram) {
		hh_print(HHLOG_ERR, "NULL zram\n");
		return;
	}
	if (index >= (u32)zram->area->nr_objs) {
		hh_print(HHLOG_ERR, "index = %d invalid\n", index);
		return;
	}
	if (!mcg) {
		hh_print(HHLOG_ERR, "NULL mcg\n");
		return;
	}

	size = zram_get_obj_size(zram, index);

	zram_clear_flag(zram, index, ZRAM_UNDER_WB);
#ifdef CONFIG_ZRAM_DEDUP
	zram_free_handle(zram, index);
#else
	handle = zram_get_handle(zram, index);
	zs_free(zram->mem_pool, handle);
#endif
	atomic64_sub(size, &zram->stats.compr_data_size);
	atomic64_dec(&zram->stats.pages_stored);

	zram_set_memcg(zram, index, mcg);
	if (zram_test_flag(zram, index, ZRAM_IDLE))
		memcg_idle_dec(zram, index);
	zram_set_flag(zram, index, ZRAM_WB);
	zram_set_obj_size(zram, index, size);
	if (size == PAGE_SIZE)
		zram_set_flag(zram, index, ZRAM_HUGE);
	zram_set_handle(zram, index, eswpentry);
	zram_rmap_insert(zram, index);

	atomic64_add(size, &stat->stored_size);
	atomic64_add(size, &mcg->hyperhold_stored_size);
	atomic64_inc(&stat->stored_pages);
	atomic_inc(&zram->area->ext_stored_pages[esentry_extid(eswpentry)]);
	atomic64_add(size, &zram->area->ext_stored_size[esentry_extid(eswpentry)]);
	atomic64_inc(&mcg->hyperhold_stored_pages);
}

static void __move_to_zram(struct zram *zram, u32 index, unsigned long handle,
			struct io_extent *io_ext)
{
	size_t size;
	struct hyperhold_stat *stat = hyperhold_get_stat_obj();
	struct mem_cgroup *mcg = io_ext->mcg;

	if (!stat) {
		hh_print(HHLOG_ERR, "NULL stat\n");
		return;
	}

	zram_slot_lock(zram, index);
	size = zram_get_obj_size(zram, index);
	if (zram_test_overwrite(zram, index, io_ext->ext_id)) {
		zram_slot_unlock(zram, index);
		zs_free(zram->mem_pool, handle);
		return;
	}
	zram_rmap_erase(zram, index);
	zram_set_handle(zram, index, handle);
	if (zram_test_flag(zram, index, ZRAM_IDLE))
		memcg_idle_inc(zram, index);
	zram_clear_flag(zram, index, ZRAM_WB);
	if (mcg)
		zram_lru_add_tail(zram, index, mcg);
	zram_set_flag(zram, index, ZRAM_FROM_HYPERHOLD);
	zram_slot_unlock(zram, index);

	atomic64_inc(&stat->batchout_pages);
	atomic64_sub(size, &stat->stored_size);
	atomic64_dec(&stat->stored_pages);
	atomic_dec(&zram->area->ext_stored_pages[io_ext->ext_id]);
	atomic64_sub(size, &zram->area->ext_stored_size[io_ext->ext_id]);
	if (mcg) {
		atomic64_sub(size, &mcg->hyperhold_stored_size);
		atomic64_dec(&mcg->hyperhold_stored_pages);
	}
	atomic64_add(size, &zram->stats.compr_data_size);
	atomic64_inc(&zram->stats.pages_stored);
}

static int move_to_zram(struct zram *zram, u32 index, struct io_extent *io_ext)
{
	unsigned long handle, eswpentry;
	struct mem_cgroup *mcg = NULL;
	int i;
	size_t size;
	u8 *dst = NULL;

	if (!zram) {
		hh_print(HHLOG_ERR, "NULL zram\n");
		return -EINVAL;
	}
	if (index >= (u32)zram->area->nr_objs) {
		hh_print(HHLOG_ERR, "index = %d invalid\n", index);
		return -EINVAL;
	}
	if (!io_ext) {
		hh_print(HHLOG_ERR, "NULL io_ext\n");
		return -EINVAL;
	}

	mcg = io_ext->mcg;
	zram_slot_lock(zram, index);
#ifdef CONFIG_ZRAM_DEDUP
	eswpentry = zram_get_direct_handle(zram, index);
#else
	eswpentry = zram_get_handle(zram, index);
#endif
	if (zram_test_overwrite(zram, index, io_ext->ext_id)) {
		zram_slot_unlock(zram, index);
		return 0;
	}
	size = zram_get_obj_size(zram, index);
	zram_slot_unlock(zram, index);

	for (i = esentry_pgid(eswpentry) - 1; i >= 0 && io_ext->pages[i]; i--) {
		hyperhold_page_recycle(io_ext->pages[i], io_ext->pool);
		io_ext->pages[i] = NULL;
	}
	handle = hyperhold_zsmalloc(zram->mem_pool, size, io_ext->pool);
	if (!handle) {
		hh_print(HHLOG_ERR, "alloc handle failed, size = %d\n", size);
		return -ENOMEM;
	}
	dst = zs_map_object(zram->mem_pool, handle, ZS_MM_WO);
	copy_from_pages(dst, io_ext->pages, eswpentry, size);
	zs_unmap_object(zram->mem_pool, handle);
	__move_to_zram(zram, index, handle, io_ext);

	return 0;
}

static void extent_unlock(struct io_extent *io_ext)
{
	int ext_id;
	struct mem_cgroup *mcg = NULL;
	struct zram *zram = NULL;
	int k;
	unsigned long eswpentry;

	if (!io_ext) {
		hh_print(HHLOG_ERR, "NULL io_ext\n");
		goto out;
	}

	mcg = io_ext->mcg;
	if (!mcg) {
		hh_print(HHLOG_ERR, "NULL mcg\n");
		goto out;
	}
	zram = mcg->zram;
	if (!zram) {
		hh_print(HHLOG_ERR, "NULL zram\n");
		goto out;
	}
	ext_id = io_ext->ext_id;
	if (ext_id < 0)
		goto out;

	ext_id = io_ext->ext_id;
	if (mcg->in_swapin)
		goto out;
	hh_print(HHLOG_DEBUG, "add ext_id = %d, cnt = %d.\n",
			ext_id, io_ext->cnt);
	eswpentry = ((unsigned long)ext_id) << EXTENT_SHIFT; /*lint !e571*/
	for (k = 0; k < io_ext->cnt; k++)
		zram_slot_lock(zram, io_ext->index[k]);
	for (k = 0; k < io_ext->cnt; k++) {
		move_to_hyperhold(zram, io_ext->index[k], eswpentry, mcg);
		eswpentry += zram_get_obj_size(zram, io_ext->index[k]);
	}
	put_extent(zram->area, ext_id);
	io_ext->ext_id = -EINVAL;
	for (k = 0; k < io_ext->cnt; k++)
		zram_slot_unlock(zram, io_ext->index[k]);
	hh_print(HHLOG_DEBUG, "add extent OK.\n");
out:
	discard_io_extent(io_ext, REQ_OP_WRITE);
	if (mcg)
		css_put(&mcg->css);
}

static void extent_add(struct io_extent *io_ext,
		       enum hyperhold_scenario scenario)
{
	struct mem_cgroup *mcg = NULL;
	struct zram *zram = NULL;
	int ext_id;
	int k;

	if (!io_ext) {
		hh_print(HHLOG_ERR, "NULL io_ext\n");
		return;
	}

	mcg = io_ext->mcg;
	if (!mcg)
		zram = io_ext->zram;
	else
		zram = mcg->zram;
	if (!zram) {
		hh_print(HHLOG_ERR, "NULL zram\n");
		goto out;
	}
	ext_id = io_ext->ext_id;
	if (ext_id < 0)
		goto out;

	io_ext->cnt = zram_rmap_get_extent_index(zram->area,
						 ext_id,
						 io_ext->index);
	hh_print(HHLOG_DEBUG, "ext_id = %d, cnt = %d.\n", ext_id, io_ext->cnt);
	for (k = 0; k < io_ext->cnt; k++) {
		int ret = move_to_zram(zram, io_ext->index[k], io_ext);

		if (ret)
			goto out;
	}
	hh_print(HHLOG_DEBUG, "extent add OK, free ext_id = %d.\n", ext_id);
	hyperhold_free_extent(zram->area, io_ext->ext_id, true);
	io_ext->ext_id = -EINVAL;
out:
	discard_io_extent(io_ext, REQ_OP_READ);
	if (mcg)
		css_put(&mcg->css);
}

static void extent_clear(struct zram *zram, int ext_id)
{
	int *index = NULL;
	int cnt;
	int k;
	struct hyperhold_stat *stat = hyperhold_get_stat_obj();

	if (!stat) {
		hh_print(HHLOG_ERR, "NULL stat\n");
		return;
	}

	index = kzalloc(sizeof(int) * EXTENT_MAX_OBJ_CNT, GFP_NOIO);
	if (!index)
		index = kzalloc(sizeof(int) * EXTENT_MAX_OBJ_CNT,
				GFP_NOIO | __GFP_NOFAIL);

	cnt = zram_rmap_get_extent_index(zram->area, ext_id, index);

	for (k = 0; k < cnt; k++) {
		zram_slot_lock(zram, index[k]);
		if (zram_test_overwrite(zram, index[k], ext_id)) {
			zram_slot_unlock(zram, index[k]);
			continue;
		}
		zram_set_memcg(zram, index[k], NULL);
		zram_set_flag(zram, index[k], ZRAM_MCGID_CLEAR);
		atomic64_inc(&stat->mcgid_clear);
		zram_slot_unlock(zram, index[k]);
	}

	kfree(index);
}

static int shrink_entry(struct zram *zram, u32 index, struct io_extent *io_ext,
		 unsigned long ext_off)
{
	unsigned long handle;
	size_t size;
	u8 *src = NULL;
	struct hyperhold_stat *stat = hyperhold_get_stat_obj();

	if (!stat) {
		hh_print(HHLOG_ERR, "NULL stat\n");
		return -EINVAL;
	}
	if (!zram) {
		hh_print(HHLOG_ERR, "NULL zram\n");
		return -EINVAL;
	}
	if (index >= (u32)zram->area->nr_objs) {
		hh_print(HHLOG_ERR, "index = %d invalid\n", index);
		return -EINVAL;
	}

	if (task_hyperhold_idle_reclaim(current) &&
			!zram_test_flag(zram, index, ZRAM_IDLE))
		return -ENOENT;

	zram_slot_lock(zram, index);
#ifdef CONFIG_ZRAM_DEDUP
	handle = zram_get_direct_handle(zram, index);
#else
	handle = zram_get_handle(zram, index);
#endif
	if (!handle || zram_test_skip(zram, index, io_ext->mcg)) {
		zram_slot_unlock(zram, index);
		return 0;
	}
	size = zram_get_obj_size(zram, index);
	if (ext_off + size > EXTENT_SIZE) {
		zram_slot_unlock(zram, index);
		return -ENOSPC;
	}

	src = zs_map_object(zram->mem_pool, handle, ZS_MM_RO);
#ifdef CONFIG_CRYPTO_DELTA
	if (zram_test_flag(zram, index, ZRAM_SDDC_DELTA)) {
		if (zram_sddc_ref_obj_cache_push(zram->ref_obj_cache,
					index,
					*(uint32_t*)src))
			return -ENOMEM;
	}
#endif
	copy_to_pages(src, io_ext->pages, ext_off, size);
	zs_unmap_object(zram->mem_pool, handle);
	io_ext->index[io_ext->cnt++] = index;

	zram_lru_del(zram, index);
	zram_set_flag(zram, index, ZRAM_UNDER_WB);
	if (zram_test_flag(zram, index, ZRAM_FROM_HYPERHOLD)) {
		atomic64_inc(&stat->reout_pages);
		atomic64_add(size, &stat->reout_bytes);
	}
	zram_slot_unlock(zram, index);
	atomic64_inc(&stat->reclaimin_pages);

	return size;
}

static int shrink_entry_list(struct io_extent *io_ext)
{
	struct mem_cgroup *mcg = NULL;
	struct zram *zram = NULL;
	unsigned long stored_size;
	int *swap_index = NULL;
	int swap_cnt, k;
	int swap_size = 0;

	if (!io_ext) {
		hh_print(HHLOG_ERR, "NULL io_ext\n");
		return -EINVAL;
	}

	mcg = io_ext->mcg;
	zram = mcg->zram;
	hh_print(HHLOG_DEBUG, "mcg = %d\n", mcg->id.id);
	if (task_hyperhold_idle_reclaim(current))
		stored_size = atomic64_read(&mcg->zram_idle_size);
	else
		stored_size = atomic64_read(&mcg->zram_stored_size);
	hh_print(HHLOG_DEBUG, "zram_stored_size = %ld\n", stored_size);
#ifdef CONFIG_RAMTURBO
	if (!mcg->active_swapout &&
		!mcg->force_swapout && stored_size < EXTENT_SIZE)
#else
	if (!mcg->force_swapout && stored_size < EXTENT_SIZE)
#endif
		return -ENOENT;
	if (!stored_size)
		return -ENOENT;

	swap_index = kzalloc(sizeof(int) * EXTENT_MAX_OBJ_CNT, GFP_NOIO);
	if (!swap_index)
		return -ENOMEM;
	io_ext->ext_id = hyperhold_alloc_extent(zram->area, mcg);
	if (io_ext->ext_id < 0) {
		kfree(swap_index);
		return io_ext->ext_id;
	}
	swap_cnt = zram_get_memcg_coldest_index(zram->area, mcg, swap_index,
						EXTENT_MAX_OBJ_CNT);
	io_ext->cnt = 0;
	for (k = 0; k < swap_cnt && swap_size < (int)EXTENT_SIZE; k++) {
		int size = shrink_entry(zram, swap_index[k], io_ext, swap_size);

		if (size < 0)
			break;
		swap_size += size;
	}
	kfree(swap_index);
	hh_print(HHLOG_DEBUG, "fill extent = %d, cnt = %d, overhead = %ld.\n",
		 io_ext->ext_id, io_ext->cnt, EXTENT_SIZE - swap_size);
	if (swap_size == 0) {
		hh_print(HHLOG_ERR, "swap_size = 0, free ext_id = %d.\n",
				io_ext->ext_id);
		hyperhold_free_extent(zram->area, io_ext->ext_id, true);
		io_ext->ext_id = -EINVAL;
		return -ENOENT;
	}

	return swap_size;
}

#ifndef CONFIG_CRYPTO_DELTA
int hyperhold_cache_decomp(struct zram *zram, u32 index, struct page *page)
{
	u8 *dst = NULL;
	u8 *src = NULL;
	struct zcomp_strm *zstrm = NULL;
	unsigned long addr;
	size_t size;
	int ret = 0;

	addr = zram_get_handle(zram, index);
	size = zram_get_obj_size(zram, index);
	zstrm = zcomp_stream_get(zram->comp);
	src = zstrm->buffer;
	if (hyperhold_cache_read_obj(zram->area->cache, addr, src, size)) {
		zcomp_stream_put(zram->comp);
		return -ENOENT;
	}
	dst = kmap_atomic(page);
	if (size == PAGE_SIZE)
		memcpy(dst, src, PAGE_SIZE);
	else
		ret = zcomp_decompress(zstrm, src, size, dst);
	kunmap_atomic(dst);
	zcomp_stream_put(zram->comp);

	/* Should NEVER happen. */
	if (unlikely(ret)) {
		hh_print(HHLOG_ERR,
			"decomp failed, idx = %d, addr = %lx, size = %d\n",
			index, addr, size);
		BUG();
		return -EIO;
	}

	return 0;
}
#else /* CONFIG_CRYPTO_DELTA */
int hyperhold_cache_decomp(struct zram *zram, u32 index, struct page *page)
{
	u8 *src = NULL;
	struct zcomp_ext_strm *zstrm = NULL;
	unsigned long addr;
	size_t size;
	int ret = 0;

	addr = zram_get_handle(zram, index);
	size = zram_get_obj_size(zram, index);
	zstrm = zcomp_ext_stream_get(zram->comp);
	src = sddc_get_encoded(zstrm);
	if (hyperhold_cache_read_obj(zram->area->cache, addr, src, size)) {
		zcomp_ext_stream_put(zram->comp);
		return -ENOENT;
	}

	ret = sddc_decode(zstrm, page, index, src);
	zcomp_ext_stream_put(zram->comp);

	/* Should NEVER happen. */
	if (unlikely(ret)) {
		hh_print(HHLOG_ERR,
			"decomp failed, idx = %d, addr = %lx, size = %d\n",
			index, addr, size);
		BUG();
		return -EIO;
	}

	return 0;
}
#endif

bool hyperhold_cache_objs_del(struct zram *zram, u32 index,
	unsigned long handle)
{
	struct mem_cgroup *mcg = NULL;
	int ret = 0;
	unsigned long addr = zram_get_handle(zram, index);
	struct hyperhold_stat *stat = hyperhold_get_stat_obj();
	int ext_id = esentry_extid(addr);
	size_t size = zram_get_obj_size(zram, index);

	if (!zram_test_flag(zram, index, ZRAM_WB)
		|| zram_get_handle(zram, index) != addr) {
		zs_free(zram->mem_pool, handle);
		return false;
	}

	atomic64_sub(size, &zram->area->ext_stored_size[ext_id]);
	ret = atomic_dec_and_test(&zram->area->ext_stored_pages[ext_id]);

	zram_rmap_erase(zram, index);
	hh_print(HHLOG_DEBUG,
	"set handle, index = %d, addr = %lx, handle = %lx\n",
		index, addr, handle, size);
	zram_set_handle(zram, index, handle);
	zram_clear_flag(zram, index, ZRAM_WB);
	atomic64_add(size, &zram->stats.compr_data_size);
	atomic64_inc(&zram->stats.pages_stored);
	mcg = zram_get_memcg(zram, index);
	if (mcg)
		zram_lru_add_tail(zram, index, mcg);
	zram_set_flag(zram, index, ZRAM_FROM_HYPERHOLD);

	if (stat) {
		atomic64_inc(&stat->batchout_pages);
		atomic64_sub(size, &stat->stored_size);
		atomic64_dec(&stat->stored_pages);
	}

	if (mcg) {
		atomic64_sub(size, &mcg->hyperhold_stored_size);
		atomic64_dec(&mcg->hyperhold_stored_pages);
	}
	if (!ret)
		return false;
	return true;
}

int hyperhold_cache_out(struct zram *zram, u32 index,
		unsigned long eswpentry, bool unpin)
{
	u8 *dst = NULL;
	unsigned long handle;
	unsigned long addr = zram_get_handle(zram, index);
	size_t size = zram_get_obj_size(zram, index);
	int ext_id = esentry_extid(addr);

	if (!zram_test_flag(zram, index, ZRAM_WB)) {
		if (unpin)
			hyperhold_cache_unpin(zram->area->cache,
					esentry_extid(eswpentry));
		return -ENOENT;
	}

	if (!hyperhold_cache_lookup(zram->area->cache, ext_id))
		return -ENOENT;

	zram_slot_unlock(zram, index);

	handle = hyperhold_zsmalloc(zram->mem_pool, size, NULL);
	if (!handle) {
		hh_print(HHLOG_ERR, "alloc handle failed!\n");
		zram_slot_lock(zram, index);
		return -ENOMEM;
	}

	dst = zs_map_object(zram->mem_pool, handle, ZS_MM_WO);
	if (hyperhold_cache_read_obj(zram->area->cache, addr, dst, size)) {
		zs_unmap_object(zram->mem_pool, handle);
		zs_free(zram->mem_pool, handle);
		zram_slot_lock(zram, index);
		return -ENOENT;
	}
	zs_unmap_object(zram->mem_pool, handle);

	if (unpin)
		hyperhold_cache_unpin(zram->area->cache, ext_id);

	zram_slot_lock(zram, index);
	if (!hyperhold_cache_objs_del(zram, index, handle))
		return 0;
	ext_id = get_extent(zram->area, ext_id);
	if (ext_id < 0)
		return 0;

	hh_print(HHLOG_DEBUG, "free ext_id = %d\n", ext_id);
	hyperhold_free_extent(zram->area, ext_id, true);

	return 0;
}

bool move_cache_to_zram(struct zram *zram, int ext_id)
{
	int *index = kzalloc(sizeof(int) * EXTENT_MAX_OBJ_CNT, GFP_NOIO);
	int k, cnt;
	int remain = 0;

	hh_print(HHLOG_DEBUG, "move start, ext = %d\n", ext_id);
	if (!index) {
		hh_print(HHLOG_ERR, "alloc index failed\n");
		goto err_out;
	}
	cnt = zram_rmap_get_extent_index(zram->area, ext_id, index);
	for (k = 0; k < cnt; k++) {
		int ret;

		zram_slot_lock(zram, index[k]);
		if (zram_test_flag(zram, index[k], ZRAM_BATCHING_OUT) ||
			zram_test_overwrite(zram, index[k], ext_id)) {
			zram_slot_unlock(zram, index[k]);
			continue;
		}
		ret = hyperhold_cache_out(zram, index[k], 0, false);
		zram_slot_unlock(zram, index[k]);
		if (ret == -ENOMEM) {
			hh_print(HHLOG_ERR,
				"cache out failed, k = %d, index = %d\n",
				k, index[k]);
			break;
		}
	}
	kfree(index);
	remain = atomic_read(&zram->area->ext_stored_pages[ext_id]);
	if (remain != 0)
		goto err_out;
	hyperhold_free_extent(zram->area, ext_id, true);
	hh_print(HHLOG_DEBUG, "move end, ext = %d\n", ext_id);

	return true;
err_out:
	put_extent(zram->area, ext_id);
	hh_print(HHLOG_DEBUG,
		"move failed, ext = %d, cnt = %d, k = %d, remain = %d\n",
		ext_id, cnt, k, remain);

	return false;
}

bool extent_can_move(struct hyperhold_area *area, int ext_id)
{
	if (!hyperhold_cache_lookup(area->cache, ext_id))
		return false;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	if (atomic64_read(&area->ext_stored_size[ext_id]) > CACHE_SIZE_HIGH)
#else
	if (atomic_read(&area->ext_stored_size[ext_id]) > CACHE_SIZE_HIGH)
#endif
		return false;

	return true;
}

int hyperhold_memcg_move_cache(struct zram *zram, struct mem_cgroup *mcg,
				int nr_exts)
{
	int moved = 0;

	while (1) {
		int ext_id = get_memcg_extent(zram->area, mcg, extent_can_move);

		if (ext_id == -ENOENT)
			break;

		if (move_cache_to_zram(zram, ext_id)) {
			moved++;
			if (moved >= nr_exts)
				break;
		}
	}

	return moved;
}

int hyperhold_move_cache(struct zram *zram, int nr_exts)
{
	int moved = 0;
	struct mem_cgroup *mcg = NULL;

	atomic64_inc(&zram->area->cache_move_runs);
	hh_print(HHLOG_DEBUG, "run move cache, total = %ld, nr = %d\n",
		atomic64_read(&zram->area->cache_move_runs), nr_exts);

	for (mcg = get_prev_memcg(NULL); mcg; mcg = get_prev_memcg(mcg)) {
		if (!mcg->zram)
			continue;
		moved += hyperhold_memcg_move_cache(zram, mcg, nr_exts - moved);
		if (moved >= nr_exts)
			break;
	}

	if (mcg) {
		get_prev_memcg_break(mcg);
		mcg = NULL;
	}
	atomic64_add(moved, &zram->area->cache_move_exts);
	hh_print(HHLOG_DEBUG, "moved extent %d, total = %ld\n",
		moved, atomic64_read(&zram->area->cache_move_exts));

	return moved;
}

unsigned long hyperhold_drop_cache(struct zram *zram, unsigned long nr_pages)
{
	unsigned long nr_cached = hyperhold_cache_pages(zram);
	if (!nr_pages && (HYPERHOLD_CACHE_SIZE_MAX >> PAGE_SHIFT) < nr_cached)
		nr_pages = nr_cached - (HYPERHOLD_CACHE_SIZE_MAX >> PAGE_SHIFT);

	if (!nr_pages)
		return 0;

	return hyperhold_shrink_cache(zram->area, nr_pages);
}

int hyperhold_set_cache_level(struct zram *zram, int level)
{
	if (!zram || !zram->area)
		return -EINVAL;

	zram->area->fault_out_cache_enable = level & HYPERHOLD_CACHE_FAULT_OUT;
	zram->area->batch_out_cache_enable = level & HYPERHOLD_CACHE_BATCH_OUT;
	zram->area->reclaim_in_cache_enable = level & HYPERHOLD_CACHE_RECLAIM_IN;

	return 0;
}

unsigned long hyperhold_cache_pages(struct zram *zram)
{
	if (!zram->area)
		return 0;

	return zram->area->cache->nr_cached_exts * EXTENT_PG_CNT;
}

void hp_cache_show(struct seq_file *m, struct zram *zram)
{
	if (!zram || !zram->area || !zram->area->cache)
		return;

	seq_printf(m, "hyperhold_cache_pages: %lld\n",
		hyperhold_cache_pages(zram));
	seq_printf(m, "hyperhold_cache_shrinker_runs: %lld\n",
		atomic64_read(&zram->area->cache_shrinker_runs));
	seq_printf(m, "hyperhold_cache_shrink_pages: %lld\n",
		atomic64_read(&zram->area->cache_shrinker_reclaim_pages));
	seq_printf(m, "hyperhold_cache_move_runs: %lld\n",
		atomic64_read(&zram->area->cache_move_runs));
	seq_printf(m, "hyperhold_cache_move_exts: %lld\n",
		atomic64_read(&zram->area->cache_move_exts));
}

void hyperhold_cache_state(struct zram *zram, char *buf, ssize_t *size)
{
	struct hyperhold_area *area = NULL;

	if (!zram || !zram->area)
		return;

	area = zram->area;

	if(!area->cache)
		return;

	*size += scnprintf(buf + *size, PAGE_SIZE - *size,
			"Hyperhold cache enable:%s%s%s\n",
			area->fault_out_cache_enable ? " fault_out" : "",
			area->batch_out_cache_enable ? " batch_out" : "",
			area->reclaim_in_cache_enable ? " reclaim_in" : "");
	*size += scnprintf(buf + *size, PAGE_SIZE - *size,
			"Hyperhold cache exts: %d\n",
			area->cache->nr_cached_exts);
	*size += scnprintf(buf + *size, PAGE_SIZE - *size,
			"Hyperhold cache shrink runs: %ld, reclaim page: %ld\n",
			atomic64_read(&area->cache_shrinker_runs),
			atomic64_read(&area->cache_shrinker_reclaim_pages));
	*size += scnprintf(buf + *size, PAGE_SIZE - *size,
			"Hyperhold cache move runs: %ld, move exts: %ld\n",
			atomic64_read(&area->cache_move_runs),
			atomic64_read(&area->cache_move_exts));
}

/*
 * The function is used to initialize global settings
 */
void hyperhold_manager_deinit(struct zram *zram)
{
	if (!zram) {
		hh_print(HHLOG_ERR, "NULL zram\n");
		return;
	}

	free_hyperhold_area(zram->area);
	zram->area = NULL;
}

int hyperhold_manager_init(struct zram *zram)
{
	int ret;

	if (!zram) {
		hh_print(HHLOG_ERR, "NULL zram\n");
		ret = -EINVAL;
		goto out;
	}

	zram->area = alloc_hyperhold_area(zram->disksize,
					  zram->nr_pages << PAGE_SHIFT);
	if (!zram->area) {
		ret = -ENOMEM;
		goto out;
	}
	return 0;
out:
	hyperhold_manager_deinit(zram);

	return ret;
}

/*
 * The function is used to initialize private member in memcg
 */
void hyperhold_manager_memcg_init(struct mem_cgroup *mcg, struct zram *zram)
{
	if (!mcg || !zram || !zram->area) {
		hh_print(HHLOG_ERR, "invalid mcg\n");
		return;
	}

	mcg->in_swapin = false;
	hh_list_init(mcg_idx(zram->area, mcg->id.id), zram->area->obj_table);
	hh_list_init(mcg_idx(zram->area, mcg->id.id), zram->area->ext_table);

	atomic64_set(&mcg->zram_stored_size, 0);
	atomic64_set(&mcg->zram_page_size, 0);
	atomic64_set(&mcg->hyperhold_stored_pages, 0);
	atomic64_set(&mcg->hyperhold_stored_size, 0);
	atomic64_set(&mcg->hyperhold_allfaultcnt, 0);

	atomic64_set(&mcg->hyperhold_outcnt, 0);
	atomic64_set(&mcg->hyperhold_incnt, 0);
	atomic64_set(&mcg->hyperhold_faultcnt, 0);
	atomic64_set(&mcg->hyperhold_outextcnt, 0);
	atomic64_set(&mcg->hyperhold_inextcnt, 0);

	atomic_set(&mcg->hyperhold_extcnt, 0);
	atomic_set(&mcg->hyperhold_peakextcnt, 0);

	smp_wmb();
	mcg->zram = zram;
	hh_print(HHLOG_DEBUG, "new memcg in zram, id = %d.\n", mcg->id.id);
}

void hyperhold_manager_memcg_deinit(struct mem_cgroup *mcg)
{
	struct zram *zram = NULL;
	struct hyperhold_area *area = NULL;
	struct hyperhold_stat *stat = hyperhold_get_stat_obj();
	int last_index = -1;

	if (!stat) {
		hh_print(HHLOG_ERR, "NULL stat\n");
		return;
	}

	if (!mcg || !mcg->zram || !mcg->zram->area) {
		hh_print(HHLOG_ERR, "invalid mcg\n");
		return;
	}
	hh_print(HHLOG_DEBUG, "deinit mcg %d\n", mcg->id.id);
	zram = mcg->zram;
	area = zram->area;
	while (1) {
		int index = get_memcg_zram_entry(area, mcg);
		if (index == -ENOENT)
			break;
		if (index < 0) {
			hh_print(HHLOG_ERR, "invalid index\n");
			return;
		}

		if (last_index == index) {
			hh_print(HHLOG_ERR, "dup index %d, mcg[%s] head %lx\n",
					index, (strlen(mcg->name) != 0) ? mcg->name : "null",
					mcg->zram_lru);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
			show_stack(current, NULL, KERN_ERR);
#else
			show_stack(current, NULL);
#endif
		}

		zram_slot_lock(zram, index);
		if (index == last_index || mcg == zram_get_memcg(zram, index)) {
			hh_list_del(obj_idx(zram->area, index),
					mcg_idx(zram->area, mcg->id.id),
					zram->area->obj_table);
			zram_set_memcg(zram, index, NULL);
			zram_set_flag(zram, index, ZRAM_MCGID_CLEAR);
			atomic64_inc(&stat->mcgid_clear);
		}
		zram_slot_unlock(zram, index);
		last_index = index;
	}
	hh_print(HHLOG_DEBUG, "deinit mcg %d, entry done\n", mcg->id.id);
	while (1) {
		int ext_id = get_memcg_extent(area, mcg, NULL);
		if (ext_id == -ENOENT)
			break;
		extent_clear(zram, ext_id);
		hh_list_set_mcgid(ext_idx(area, ext_id), area->ext_table, 0);
		put_extent(area, ext_id);
	}
	hh_print(HHLOG_DEBUG, "deinit mcg %d, extent done\n", mcg->id.id);
}

#ifdef CONFIG_CRYPTO_DELTA
void hyperhold_change_size_of_page(struct zram *zram, u32 index, u32 size_diff)
{
	if (!zram) {
		hh_print(HHLOG_ERR, "NULL zram\n");
		return;
	}
	if (index >= (u32)zram->area->nr_objs) {
		hh_print(HHLOG_ERR, "index = %d invalid\n", index);
		return;
	}
	zram_lru_change_size_of_page(zram, index, size_diff);
}
#endif

/*
 * The function is used to add ZS object to ZRAM LRU list
 */
void hyperhold_zram_lru_add(struct zram *zram,
			    u32 index,
			    struct mem_cgroup *mcg)
{
	if (!zram) {
		hh_print(HHLOG_ERR, "NULL zram\n");
		return;
	}
	if (index >= (u32)zram->area->nr_objs) {
		hh_print(HHLOG_ERR, "index = %d invalid\n", index);
		return;
	}
	if (!mcg) {
		hh_print(HHLOG_ERR, "NULL mcg\n");
		return;
	}

	zram_lru_add(zram, index, mcg);
}

/*
 * The function is used to del ZS object from ZRAM LRU list
 */
void hyperhold_zram_lru_del(struct zram *zram, u32 index)
{
	struct hyperhold_stat *stat = hyperhold_get_stat_obj();

	if (!stat) {
		hh_print(HHLOG_ERR, "NULL stat\n");
		return;
	}
	if (!zram) {
		hh_print(HHLOG_ERR, "NULL zram\n");
		return;
	}
	if (index >= (u32)zram->area->nr_objs) {
		hh_print(HHLOG_ERR, "index = %d invalid\n", index);
		return;
	}

	zram_clear_flag(zram, index, ZRAM_FROM_HYPERHOLD);
	if (zram_test_flag(zram, index, ZRAM_MCGID_CLEAR)) {
		zram_clear_flag(zram, index, ZRAM_MCGID_CLEAR);
		atomic64_dec(&stat->mcgid_clear);
	}
	if (zram_test_flag(zram, index, ZRAM_WB)) {
		hyperhold_extent_objs_del(zram, index);
		zram_rmap_erase(zram, index);
		zram_clear_flag(zram, index, ZRAM_WB);
		zram_set_memcg(zram, index, NULL);
		zram_set_handle(zram, index, 0);
	} else {
		if (zram_test_flag(zram, index, ZRAM_IDLE))
			memcg_idle_dec(zram, index);
		zram_lru_del(zram, index);
	}
}

/*
 * The function is used to alloc a new extent,
 * then reclaim ZRAM by LRU list to the new extent.
 */
unsigned long hyperhold_extent_create(struct mem_cgroup *mcg,
				      int *ext_id,
				      struct hyperhold_buffer *buf,
				      void **private)
{
	struct io_extent *io_ext = NULL;
	struct zram *zram = NULL;
	struct hyperhold_area *area = NULL;
	int reclaim_size;
	int ext_cnt;

	if (!mcg) {
		hh_print(HHLOG_ERR, "NULL mcg\n");
		return 0;
	}
	if (!ext_id) {
		hh_print(HHLOG_ERR, "NULL ext_id\n");
		return 0;
	}
	(*ext_id) = -EINVAL;
	if (!buf) {
		hh_print(HHLOG_ERR, "NULL buf\n");
		return 0;
	}
	if (!private) {
		hh_print(HHLOG_ERR, "NULL private\n");
		return 0;
	}

	io_ext = alloc_io_extent(buf->pool, false, true);
	if (!io_ext)
		return 0;
	io_ext->mcg = mcg;
	io_ext->zram = mcg->zram;
	reclaim_size = shrink_entry_list(io_ext);
	if (reclaim_size < 0) {
		discard_io_extent(io_ext, REQ_OP_WRITE);
		(*ext_id) = reclaim_size;
		return 0;
	}
	css_get(&mcg->css);
	(*ext_id) = io_ext->ext_id;
	buf->dest_pages = io_ext->pages;
	(*private) = io_ext;
	hh_print(HHLOG_DEBUG, "mcg = %d, ext_id = %d\n",
		 mcg->id.id, io_ext->ext_id);

	zram = mcg->zram;
	area = zram->area;
	ext_cnt = atomic_read(&area->stored_exts);
	if (ext_cnt >= hp_high_ext_num(area->nr_exts)) {
		int nr_to_move = ext_cnt - hp_low_ext_num(area->nr_exts);
		int moved = hyperhold_move_cache(zram, nr_to_move);

		hh_print(HHLOG_DEBUG, "to move = %d, moved = %d",
				nr_to_move, moved);
	}

	return reclaim_size;
}

/*
 * The function will be called when hyperhold write done.
 * The function should register extent to hyperhold manager.
 */
void hyperhold_extent_register(void *private)
{
	struct io_extent *io_ext = private;
	struct extent_cache *ext = NULL;
	int k, ret;

	if (!io_ext) {
		hh_print(HHLOG_ERR, "NULL io_ext\n");
		return;
	}
	hh_print(HHLOG_DEBUG, "ext_id = %d\n", io_ext->ext_id);

	if (!io_ext->zram->area->reclaim_in_cache_enable)
		goto out;

	ext = new_extent_cache(io_ext->pages, false, true);
	if (!ext) {
		hh_print(HHLOG_ERR, "alloc cache failed, ext = %d\n",
				io_ext->ext_id);
		goto out;
	}

	ext->pin = false;

	ret = hyperhold_cache_insert(io_ext->zram->area->cache,
				io_ext->ext_id, ext, false, true);
	if (ret) {
		free_extent_cache(ext);
		hh_print(HHLOG_ERR, "insert cache failed, ext = %d, ret = %d\n",
				io_ext->ext_id, ret);
		goto out;
	}

	for (k = 0; k < EXTENT_PG_CNT; k++)
		io_ext->pages[k] = NULL;
out:
	extent_unlock(io_ext);
}

/*
 * If extent empty, return false, otherise return true.
 */
void hyperhold_extent_objs_del(struct zram *zram, u32 index)
{
	int ext_id;
	struct mem_cgroup *mcg = NULL;
	unsigned long eswpentry;
	size_t size;
	struct hyperhold_stat *stat = hyperhold_get_stat_obj();

	if (!stat) {
		hh_print(HHLOG_ERR, "NULL stat\n");
		return;
	}
	if (!zram || !zram->area) {
		hh_print(HHLOG_ERR, "NULL zram\n");
		return;
	}
	if (index >= (u32)zram->area->nr_objs) {
		hh_print(HHLOG_ERR, "index = %d invalid\n", index);
		return;
	}
	if (!zram_test_flag(zram, index, ZRAM_WB)) {
		hh_print(HHLOG_ERR, "not WB object\n");
		return;
	}

#ifdef CONFIG_ZRAM_DEDUP
	eswpentry = zram_get_direct_handle(zram, index);
#else
	eswpentry = zram_get_handle(zram, index);
#endif
	size = zram_get_obj_size(zram, index);
	atomic64_sub(size, &stat->stored_size);
	atomic64_dec(&stat->stored_pages);
	mcg = zram_get_memcg(zram, index);
	if (mcg) {
		atomic64_sub(size, &mcg->hyperhold_stored_size);
		atomic64_dec(&mcg->hyperhold_stored_pages);
	}
	atomic64_sub(size,
		&zram->area->ext_stored_size[esentry_extid(eswpentry)]);
	if (!atomic_dec_and_test(
		&zram->area->ext_stored_pages[esentry_extid(eswpentry)]))
		return;
	ext_id = get_extent(zram->area, esentry_extid(eswpentry));
	if (ext_id < 0)
		return;

	atomic64_inc(&stat->notify_free);
	if (mcg)
		atomic64_inc(&mcg->hyperhold_ext_notify_free);
	hh_print(HHLOG_DEBUG, "free ext_id = %d\n", ext_id);
	hyperhold_free_extent(zram->area, ext_id, true);
}

/*
 * The function will be called when hyperhold execute fault out.
 */
int hyperhold_find_extent_by_idx(unsigned long eswpentry,
				 struct hyperhold_buffer *buf,
				 void **private)
{
	int ext_id;
	struct io_extent *io_ext = NULL;
	struct zram *zram = NULL;

	if (!buf) {
		hh_print(HHLOG_ERR, "NULL buf\n");
		return -EINVAL;
	}
	if (!private) {
		hh_print(HHLOG_ERR, "NULL private\n");
		return -EINVAL;
	}

	zram = buf->zram;
	ext_id = get_extent(zram->area, esentry_extid(eswpentry));
	if (ext_id < 0)
		return ext_id;
	if (hyperhold_cache_pin(zram->area->cache, ext_id)) {
		put_extent(zram->area, ext_id);
		return -EAGAIN;
	}
	io_ext = alloc_io_extent(buf->pool, true, true);
	if (!io_ext) {
		hh_print(HHLOG_ERR, "io_ext alloc failed\n");
		put_extent(zram->area, ext_id);
		return -ENOMEM;
	}

	io_ext->ext_id = ext_id;
	io_ext->zram = zram;
	io_ext->mcg = get_memcg_with_css_get(
				hh_list_get_mcgid(ext_idx(zram->area, ext_id),
						  zram->area->ext_table));

	buf->dest_pages = io_ext->pages;
	(*private) = io_ext;
	hh_print(HHLOG_DEBUG, "get entry = %lx ext = %d\n", eswpentry, ext_id);

	return ext_id;
}

bool extent_not_cached(struct hyperhold_area *area, int ext_id)
{
	return !hyperhold_cache_lookup(area->cache, ext_id);
}

/*
 * The function will be called when hyperhold executes batch out.
 */
int hyperhold_find_extent_by_memcg(struct mem_cgroup *mcg,
				   struct hyperhold_buffer *buf,
				   void **private)
{
	int ext_id;
	struct io_extent *io_ext = NULL;

	if (!mcg) {
		hh_print(HHLOG_ERR, "NULL mcg\n");
		return -EINVAL;
	}
	if (!buf) {
		hh_print(HHLOG_ERR, "NULL buf\n");
		return -EINVAL;
	}
	if (!private) {
		hh_print(HHLOG_ERR, "NULL private\n");
		return -EINVAL;
	}

	ext_id = get_memcg_extent(mcg->zram->area, mcg, extent_not_cached);
	if (ext_id < 0)
		return ext_id;
	io_ext = alloc_io_extent(buf->pool, true, false);
	if (!io_ext) {
		hh_print(HHLOG_ERR, "io_ext alloc failed\n");
		put_extent(mcg->zram->area, ext_id);
		return -ENOMEM;
	}
	io_ext->ext_id = ext_id;
	io_ext->mcg = mcg;
	io_ext->zram = mcg->zram;
	css_get(&mcg->css);
	buf->dest_pages = io_ext->pages;
	(*private) = io_ext;
	hh_print(HHLOG_DEBUG, "get mcg = %d, ext = %d\n", mcg->id.id, ext_id);

	return ext_id;
}

/*
 * The function will be called when hyperhold read done.
 * The function should extra the extent to ZRAM, then destroy
 */
void hyperhold_extent_destroy(void *private, enum hyperhold_scenario scenario)
{
	struct io_extent *io_ext = private;
	struct extent_cache *ext = NULL;
	int ret = 0;

	if (!io_ext) {
		hh_print(HHLOG_ERR, "NULL io_ext\n");
		return;
	}

	hh_print(HHLOG_DEBUG, "ext_id = %d\n", io_ext->ext_id);

	if (!io_ext->zram->area->batch_out_cache_enable &&
			scenario == HYPERHOLD_BATCH_OUT) {
		extent_add(io_ext, scenario);
		return;
	}

	if (!io_ext->zram->area->fault_out_cache_enable &&
			scenario == HYPERHOLD_FAULT_OUT) {
		extent_add(io_ext, scenario);
		return;
	}

	ext = new_extent_cache(io_ext->pages, false, true);
	if (!ext) {
		hh_print(HHLOG_ERR, "alloc cache failed, ext = %d\n",
				io_ext->ext_id);
		goto out;
	}

	ext->pin = scenario == HYPERHOLD_FAULT_OUT;

	ret = hyperhold_cache_insert(io_ext->zram->area->cache,
				io_ext->ext_id, ext, false, true);
	if (ret) {
		free_extent_cache(ext);
		hh_print(HHLOG_ERR, "insert cache failed, ext = %d, ret = %d\n",
				io_ext->ext_id, ret);
		goto out;
	}

	atomic64_add(
		atomic_read(&io_ext->zram->area->ext_stored_pages[io_ext->ext_id]),
		&hyperhold_get_stat_obj()->batchout_pages);
out:
	put_extent(io_ext->zram->area, io_ext->ext_id);
	if (io_ext->mcg)
		css_put(&io_ext->mcg->css);
	kfree(io_ext);
}

/*
 * The function will be called
 * when schedule meets exception proceeding extent
 */
void hyperhold_extent_exception(enum hyperhold_scenario scenario,
			       void *private)
{
	struct io_extent *io_ext = private;
	struct mem_cgroup *mcg = NULL;
	unsigned int op = (scenario == HYPERHOLD_RECLAIM_IN) ?
			  REQ_OP_WRITE : REQ_OP_READ;

	if (!io_ext) {
		hh_print(HHLOG_ERR, "NULL io_ext\n");
		return;
	}

	hh_print(HHLOG_DEBUG, "ext_id = %d, op = %d\n", io_ext->ext_id, op);
	mcg = io_ext->mcg;
	discard_io_extent(io_ext, op);
	if (mcg)
		css_put(&mcg->css);
}

struct mem_cgroup *hyperhold_zram_get_memcg(struct zram *zram, u32 index)
{
	return zram_get_memcg(zram, index);
}

void memcg_idle_inc(struct zram *zram, u32 index)
{
	struct mem_cgroup *mcg = zram_get_memcg(zram, index);
	size_t size = zram_get_obj_size(zram, index);

	if (!mcg)
		return;

	atomic64_inc(&mcg->zram_idle_page);
	atomic64_add(size, &mcg->zram_idle_size);
}

void memcg_idle_dec(struct zram *zram, u32 index)
{
	struct mem_cgroup *mcg = zram_get_memcg(zram, index);
	size_t size = zram_get_obj_size(zram, index);

	if (!mcg)
		return;

	atomic64_dec(&mcg->zram_idle_page);
	atomic64_sub(size, &mcg->zram_idle_size);
}

void memcg_idle_count(struct zram *zram)
{
	struct mem_cgroup *mcg = NULL;

	hh_print(HHLOG_ERR, "==========start============\n");
	for (mcg = get_next_memcg(NULL); mcg; mcg = get_next_memcg(mcg))
		hh_print(HHLOG_ERR,
			"[%d:%s] idle = %ld, zfault = %ld, hfault = %ld\n",
			mcg->id.id, mcg->name,
			atomic64_read(&mcg->zram_idle_page),
			atomic64_read(&mcg->hyperhold_allfaultcnt),
			atomic64_read(&mcg->hyperhold_faultcnt));
	hh_print(HHLOG_ERR, "==========end============\n");
}

void hyperhold_get_page_stat(u64 *orig, u64 *compr)
{
	struct hyperhold_stat *stat = NULL;

	if (unlikely(orig == NULL || compr == NULL)) {
		return;
	}

	stat = hyperhold_get_stat_obj();

	if (!stat) {
		hh_print(HHLOG_ERR, "NULL stat\n");
		return;
	}

	*orig = atomic64_read(&stat->stored_pages) << PAGE_SHIFT;
	*compr = atomic64_read(&stat->stored_size);
}
