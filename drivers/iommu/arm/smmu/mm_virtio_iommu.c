/* Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include <linux/dma-buf.h>
#include <linux/ktime.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/virtio.h>
#include <linux/virtio_ids.h>
#include <linux/virtio_config.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include "ion.h"
#include "mm_virtio_iommu.h"

#define MAGIC_NUM			0x19d699d5
#define INIT_NENT			1
#define MAX_TIME_US			10000
#define RETRY_TIMES			1
#define ENTRY_NUMS_PAGE			(PAGE_SIZE / sizeof(struct map_entry))

#define SMMU_MAP_SYNC			0x1
#define SMMU_UNMAP_SYNC			0x2
#define SMMU_DUMP_PGT_SYNC		0x4

struct virtio_iommu_info {
	struct virtqueue *vq;
	spinlock_t request_lock;
};
struct virtio_iommu_info *g_viommu_info;

struct viommu_header_info {
	int smmuid;
	int ops_code;
	int ops_retry;
	int sg_nents;
	size_t map_size;
	unsigned long iova;
	size_t out_size;
	unsigned long magic_num;
	int err_code;
};

struct map_entry {
	u64 pa;
	u64 size;
};

struct vm_iova_dom {
	struct rb_node node;
	unsigned long iova;
	unsigned long size;
	atomic_long_t ref;
	u64 key;
	s64 _timedelta;
	int smmuid;
};

struct vm_mm_dom_cookie {
	spinlock_t iova_lock;
	struct rb_root iova_root;
};
struct vm_mm_dom_cookie vm_cookie[SMMUID_MAX];

static void viommu_header_info_add_sglist(struct scatterlist *sgl,
				struct viommu_header_info *head_info)
{
	struct page *page;
	unsigned int offset = 0;

	page = vmalloc_to_page(head_info);
	offset = ((uintptr_t)head_info) & (~PAGE_MASK);
	sg_set_page(sgl, page, sizeof(*head_info), offset);
}

static void sglist_to_map_entry(struct scatterlist *sg, int nents,
				struct map_entry *m_entry, int entry_nums)
{
	unsigned int i;
	struct scatterlist *s = NULL;

	for_each_sg(sg, s, nents, i) {
		m_entry[i].pa = sg_phys(s);
		m_entry[i].size = s->length;
		if (i >= entry_nums)
			break;
	}
}

static int map_entry_to_sglist(struct map_entry *m_entry, int entry_nums,
				struct scatterlist *sgl, int nents)
{
	int i;
	int nums = entry_nums;
	struct map_entry *entry = m_entry;
	struct page *page = NULL;
	struct scatterlist *s = sgl;

	for (i = 0; i < nents; i++) {
		unsigned long len;

		if (nums >= ENTRY_NUMS_PAGE) {
			len = PAGE_SIZE;
			nums -= ENTRY_NUMS_PAGE;
		} else {
			len = (unsigned long)nums * sizeof(struct map_entry);
		}

		page = vmalloc_to_page(entry);
		if (!page) {
			pr_err("%s, vmalloc_to_page err\n", __func__);
			return -EINVAL;
		}
		sg_set_page(s, page, len, 0);
		s = sg_next(s);

		entry += ENTRY_NUMS_PAGE;
	}

	return 0;
}

static int map_sg_nents(int sg_nents)
{
	int left_nents = 0;
	int page_nents = 0;

	left_nents = sg_nents % ENTRY_NUMS_PAGE;
	page_nents = sg_nents / ENTRY_NUMS_PAGE;

	return ((left_nents > 0) ? page_nents + 1 : page_nents);
}

static struct sg_table *create_sgtable(struct viommu_header_info *h_info)
{
	int ret;
	int nents = 0;
	struct sg_table *table = NULL;

	switch (h_info->ops_code) {
	case SMMU_MAP_SYNC:
		nents = INIT_NENT;
		nents += map_sg_nents(h_info->sg_nents);
		break;
	case SMMU_UNMAP_SYNC:
	case SMMU_DUMP_PGT_SYNC:
		nents = INIT_NENT;
		break;
	default:
		pr_err("%s, ops_code error\n", __func__);
		return NULL;
	}

	table = kmalloc(sizeof(*table), GFP_KERNEL);
	if (!table) {
		pr_err("%s, sgtable alloc fail\n", __func__);
		return NULL;
	}
	ret = sg_alloc_table(table, nents, GFP_KERNEL);
	if (ret) {
		pr_err("%s, sglist alloc fail, nents = %d\n", __func__, nents);
		goto free_table;
	}

	return table;

free_table:
	kfree(table);

	return NULL;
}

static int viommu_virtqueue_add_inbuf(struct viommu_header_info *h_info,
				struct scatterlist *sgl)
{
	int ret;
	ktime_t timeout;
	unsigned int len = 0;
	unsigned long flags = 0;
	struct map_entry *m_entry = NULL;
	struct sg_table *table = NULL;
	struct virtio_iommu_info *info = g_viommu_info;

	if (!info) {
		pr_err("%s, virtio queue is not init\n", __func__);
		return -EINVAL;
	}

	table = create_sgtable(h_info);
	if (!table) {
		pr_err("%s, creat table fail\n", __func__);
		return -ENOMEM;
	}

	viommu_header_info_add_sglist(table->sgl, h_info);
	/* For SMMU_MAP_SYNC, sglist data should be transmitted by virtio */
	if (h_info->ops_code == SMMU_MAP_SYNC) {
		int entry_nums = h_info->sg_nents;

		m_entry = vmalloc((unsigned long)entry_nums *
						sizeof(struct map_entry));
		if (!m_entry) {
			pr_err("%s, map entry alloc fail, numbers = 0x%lx\n",
						__func__, entry_nums);
			ret = -ENOMEM;
			goto free_sglist;
		}
		/*
		 * copy map sglist info to new table->sgl,
		 * table->sgl[0] save header info,
		 * map_entry save sglist addr and size info,
		 * copy map_entry to table->sgl start from table->sgl[1],
		 * virtio send table->sgl to uvmm.
		 */
		sglist_to_map_entry(sgl, h_info->sg_nents,
				m_entry, entry_nums);
		ret = map_entry_to_sglist(m_entry, entry_nums,
				sg_next(table->sgl), table->nents - INIT_NENT);
		if (ret) {
			pr_err("%s, map_entry to sglist fail, ret[%d]\n",
								__func__, ret);
			ret = -EAGAIN;
			goto free_buffer;
		}
	}
	spin_lock_irqsave(&info->request_lock, flags);
	ret = virtqueue_add_inbuf(info->vq, table->sgl,
				table->nents, h_info, GFP_KERNEL);
	if (ret < 0) {
		spin_unlock_irqrestore(&info->request_lock, flags);
		pr_err("%s, vq add buffer failed, ret[%d]\n", __func__, ret);
		ret = -EAGAIN;
		goto free_buffer;
	}
	virtqueue_kick(info->vq);
	timeout = ktime_add_us(ktime_get(), MAX_TIME_US);
	while (virtqueue_get_buf(info->vq, &len) == NULL) {
		if (ktime_compare(ktime_get(), timeout) > 0) {
			ret = -EAGAIN;
			pr_err("%s, viommu ops timeout\n", __func__);
			break;
		}
		udelay(1);
	}
	spin_unlock_irqrestore(&info->request_lock, flags);
free_buffer:
	if (h_info->ops_code == SMMU_MAP_SYNC)
		vfree(m_entry);
free_sglist:
	sg_free_table(table);
	kfree(table);
	return ret;
}

static struct sg_table *dup_sg_table(struct sg_table *table)
{
	struct sg_table *new_table;
	int ret, i;
	struct scatterlist *sg, *new_sg;

	new_table = kzalloc(sizeof(*new_table), GFP_KERNEL);
	if (!new_table)
		return NULL;

	ret = sg_alloc_table(new_table, table->nents, GFP_KERNEL);
	if (ret) {
		kfree(new_table);
		return NULL;
	}

	new_sg = new_table->sgl;
	for_each_sg(table->sgl, sg, table->nents, i) {/*lint !e574*/
		memcpy(new_sg, sg, sizeof(*sg));
		new_sg = sg_next(new_sg);
	}

	return new_table;
}

static size_t sg_size_get(struct scatterlist *sgl, int nents)
{
	int i;
	size_t iova_size = 0;
	struct scatterlist *sg = NULL;

	for_each_sg(sgl, sg, nents, i)
		iova_size += (size_t)ALIGN(sg->length, PAGE_SIZE);

	return iova_size;
}

static struct vm_iova_dom *vm_mm_iova_dom_get(struct rb_root *rbroot,
				struct dma_buf *dmabuf, unsigned long iova)
{
	struct rb_node *n = NULL;
	struct vm_iova_dom *v_iova_dom = NULL;
	u64 key = (u64)(uintptr_t)dmabuf;

	for (n = rb_first(rbroot); n; n = rb_next(n)) {
		v_iova_dom = rb_entry(n, struct vm_iova_dom, node);
		if (v_iova_dom->key == key) {
		/*
		 * Sometimes a dmabuf may correspond to more than two iova,
		 * if iova mismatch, this circle need find next matched key
		 */
			if (!iova || iova == v_iova_dom->iova)
				return v_iova_dom;
			pr_err("%s iova 0x%lx mismatch\n", __func__, iova);
		}
	}

	return NULL;
}

static struct vm_iova_dom *vm_iommu_get_dmabuf_iova(int smmuid,
	struct dma_buf *dmabuf, unsigned long *iova, unsigned long *out_size)
{
	struct vm_iova_dom *v_iova_dom = NULL;

	spin_lock(&vm_cookie[smmuid].iova_lock);
	v_iova_dom = vm_mm_iova_dom_get(&vm_cookie[smmuid].iova_root,
								dmabuf, 0);
	if (!v_iova_dom) {
		spin_unlock(&vm_cookie[smmuid].iova_lock);
		return NULL;
	}
	atomic64_inc(&v_iova_dom->ref);
	spin_unlock(&vm_cookie[smmuid].iova_lock);
	*out_size = v_iova_dom->size;
	*iova = v_iova_dom->iova;

	return v_iova_dom;
}

static struct vm_iova_dom *init_iova_dom(struct dma_buf *dmabuf)
{
	struct vm_iova_dom *v_iova_dom = NULL;

	v_iova_dom = kzalloc(sizeof(*v_iova_dom), GFP_KERNEL);
	if (!v_iova_dom)
		return NULL;

	atomic64_set(&v_iova_dom->ref, 1);
	v_iova_dom->key = (u64)(uintptr_t)dmabuf;
	return v_iova_dom;
}

static void vm_iova_dom_add(struct rb_root *rb_root,
				struct vm_iova_dom *v_iova_dom)
{
	struct rb_node **p = &rb_root->rb_node;
	struct rb_node *parent = NULL;
	struct vm_iova_dom *entry = NULL;

	while (*p) {
		parent = *p;
		entry = rb_entry(parent, struct vm_iova_dom, node);

		if (v_iova_dom < entry) {
			p = &(*p)->rb_left;
		} else if (v_iova_dom > entry) {
			p = &(*p)->rb_right;
		} else {
			pr_err("%s, iova already in tree\n", __func__);
		}
	}

	rb_link_node(&v_iova_dom->node, parent, p);
	rb_insert_color(&v_iova_dom->node, rb_root);
}

static void vm_mm_iova_dom_info(int smmuid)
{
	struct rb_node *n = NULL;
	struct vm_iova_dom *v_iova_dom = NULL;
	unsigned long total_size = 0;

	spin_lock(&vm_cookie[smmuid].iova_lock);
	for (n = rb_first(&vm_cookie[smmuid].iova_root); n; n = rb_next(n)) {
		v_iova_dom = rb_entry(n, struct vm_iova_dom, node);
		total_size += v_iova_dom->size;
		pr_err("smmuid[%d]: iova:0x%lx, size:0x%lx, map_time:%ldus\n",
				smmuid, v_iova_dom->iova,
				v_iova_dom->size, v_iova_dom->_timedelta);
	}
	spin_unlock(&vm_cookie[smmuid].iova_lock);

	pr_err("viommu smmuid[%d]: total size: 0x%lx", smmuid, total_size);
}

static void init_header_info(struct viommu_header_info *h_info, int smmuid,
				int ops_code, int sg_nents,
				size_t map_size, unsigned long iova)
{
	h_info->smmuid = smmuid;
	h_info->ops_code = ops_code;
	h_info->ops_retry = RETRY_TIMES;
	h_info->sg_nents = sg_nents;
	h_info->map_size = map_size;
	h_info->iova = iova;
	h_info->out_size = 0;
	h_info->magic_num = MAGIC_NUM;
	h_info->err_code = 0;
}

static bool should_retry(int ret, int err_code, int retry)
{
	if (!retry)
		return false;

	if (ret == -ENOMEM || err_code == -ENOMEM)
		return true;

	if (ret == -EAGAIN || err_code == -EAGAIN)
		return true;

	return false;
}

unsigned long viommu_map_dmabuf(int smmuid, struct dma_buf *dmabuf)
{
	int ret;
	unsigned long size;
	unsigned long iova = 0;
	ktime_t _stime, _etime;
	struct sg_table *dmabuf_sgt = NULL;
	struct sg_table *sgt = NULL;
	struct vm_iova_dom *v_iova_dom = NULL;
	struct viommu_header_info *h_info = NULL;

	if (smmuid < 0 || smmuid >= SMMUID_MAX) {
		pr_err("%s, smmuid error, smmuid = %d\n", __func__, smmuid);
		return 0;
	}

	if (IS_ERR_OR_NULL(dmabuf) || IS_ERR_OR_NULL(dmabuf->priv)) {
		pr_err("%s, dmabuf or dmabuf->priv is NULL!\n", __func__);
		return 0;
	}

	if (vm_iommu_get_dmabuf_iova(smmuid, dmabuf, &iova, &size))
		return iova;

	dmabuf_sgt = ((struct ion_buffer *)dmabuf->priv)->sg_table;
	if (!dmabuf_sgt) {
		pr_err("%s, dmabuf_sgt is NULL!\n", __func__);
		return 0;
	}

	sgt = dup_sg_table(dmabuf_sgt);
	if (!sgt) {
		pr_err("%s, dup_sg_table fail, nents = %d\n",
					__func__, dmabuf_sgt->nents);
		return 0;
	}

	h_info = vzalloc(sizeof(*h_info));
	if (!h_info) {
		pr_err("%s, header info alloc fail!\n", __func__);
		goto free_sgt;
	}

	init_header_info(h_info, smmuid, SMMU_MAP_SYNC, sgt->nents,
				sg_size_get(sgt->sgl, sgt->nents), 0);

	v_iova_dom = init_iova_dom(dmabuf);
	if (!v_iova_dom) {
		pr_err("%s, init_iova_dom fail!\n", __func__);
		goto free_hinfo;
	}
retry:
	_stime = ktime_get();
	ret = viommu_virtqueue_add_inbuf(h_info, sgt->sgl);
	if (ret || h_info->err_code) {
		if (should_retry(ret, h_info->err_code, h_info->ops_retry)) {
			h_info->ops_retry--;
			goto retry;
		}
		_etime = ktime_get();
		pr_err("%s, map fail-smmuid[%d], ret[%d], err_code[%d], time[%ldus]\n",
				__func__, smmuid, ret, h_info->err_code,
				ktime_us_delta(_etime, _stime));
		vm_mm_iova_dom_info(smmuid);
		kfree(v_iova_dom);
		goto free_hinfo;
	}
	_etime = ktime_get();
	iova = h_info->iova;
	size = h_info->out_size;
	v_iova_dom->iova = h_info->iova;
	v_iova_dom->size = h_info->out_size;
	v_iova_dom->smmuid = smmuid;
	v_iova_dom->_timedelta = ktime_us_delta(_etime, _stime);
	spin_lock(&vm_cookie[smmuid].iova_lock);
	vm_iova_dom_add(&vm_cookie[smmuid].iova_root, v_iova_dom);
	spin_unlock(&vm_cookie[smmuid].iova_lock);

free_hinfo:
	vfree(h_info);
free_sgt:
	sg_free_table(sgt);
	kfree(sgt);

	return iova;
}

int viommu_unmap_dmabuf(int smmuid, struct dma_buf *dmabuf, unsigned long iova)
{
	struct vm_iova_dom *v_iova_dom = NULL;
	struct viommu_header_info *h_info = NULL;
	unsigned long size;
	int ret = 0;

	if (smmuid < 0 || smmuid >= SMMUID_MAX) {
		pr_err("%s, smmuid error, smmuid = %d\n", __func__, smmuid);
		return -EINVAL;
	}

	if (IS_ERR_OR_NULL(dmabuf) || IS_ERR_OR_NULL(dmabuf->priv)) {
		pr_err("%s, dmabuf or dmabuf->priv is NULL!\n", __func__);
		return -EINVAL;
	}

	h_info = vzalloc(sizeof(*h_info));
	if (!h_info) {
		pr_err("%s, header info alloc fail!\n", __func__);
		return -ENOMEM;
	}

	spin_lock(&vm_cookie[smmuid].iova_lock);
	v_iova_dom = vm_mm_iova_dom_get(&vm_cookie[smmuid].iova_root,
								dmabuf, iova);
	if (!v_iova_dom) {
		spin_unlock(&vm_cookie[smmuid].iova_lock);
		pr_err("%s, unmap buf no map data!\n", __func__);
		ret = -EINVAL;
		goto free;
	}

	if (v_iova_dom->iova != iova) {
		spin_unlock(&vm_cookie[smmuid].iova_lock);
		pr_err("%s, iova not equal: 0x%lx, 0x%lx\n",
					__func__, v_iova_dom->iova, iova);
		ret = -EINVAL;
		goto free;
	}

	if (v_iova_dom->smmuid != smmuid) {
		spin_unlock(&vm_cookie[smmuid].iova_lock);
		pr_err("%s, smmuid not equal: %d, %d\n",
					__func__, v_iova_dom->smmuid, smmuid);
		ret = -EINVAL;
		goto free;
	}

	if (!atomic64_dec_and_test(&v_iova_dom->ref)) {
		spin_unlock(&vm_cookie[smmuid].iova_lock);
		goto free;
	}
	size = v_iova_dom->size;
	spin_unlock(&vm_cookie[smmuid].iova_lock);

	init_header_info(h_info, smmuid, SMMU_UNMAP_SYNC, 0,
				size, iova);
retry:
	ret = viommu_virtqueue_add_inbuf(h_info, NULL);
	if (ret || h_info->err_code) {
		if (should_retry(ret, h_info->err_code, h_info->ops_retry)) {
			h_info->ops_retry--;
			goto retry;
		}
		pr_err("%s, unmap fail-smmuid[%d], ret[%d], err_code[%d]\n",
				__func__, smmuid, ret, h_info->err_code);
		vm_mm_iova_dom_info(smmuid);
		spin_lock(&vm_cookie[smmuid].iova_lock);
		atomic64_inc(&v_iova_dom->ref);
		spin_unlock(&vm_cookie[smmuid].iova_lock);
		goto free;
	}
	spin_lock(&vm_cookie[smmuid].iova_lock);
	rb_erase(&v_iova_dom->node, &vm_cookie[smmuid].iova_root);
	spin_unlock(&vm_cookie[smmuid].iova_lock);

	kfree(v_iova_dom);
free:
	vfree(h_info);
	return ret;
}

unsigned long viommu_map_sg(int smmuid, struct scatterlist *sgl)
{
	int ret;
	int sg_nent;
	unsigned long size;
	unsigned long iova = 0;
	struct viommu_header_info *h_info = NULL;

	if (smmuid < 0 || smmuid >= SMMUID_MAX) {
		pr_err("%s, smmuid error!\n", __func__);
		return 0;
	}

	if (IS_ERR_OR_NULL(sgl)) {
		pr_err("%s, sgl is NULL!\n", __func__);
		return 0;
	}

	h_info = vzalloc(sizeof(*h_info));
	if (!h_info) {
		pr_err("%s, header info alloc fail!\n", __func__);
		return 0;
	}

	sg_nent = sg_nents(sgl);
	size = sg_size_get(sgl, sg_nent);
	init_header_info(h_info, smmuid, SMMU_MAP_SYNC, sg_nent,
				size, 0);
retry:
	ret = viommu_virtqueue_add_inbuf(h_info, sgl);
	if (ret || h_info->err_code) {
		if (should_retry(ret, h_info->err_code, h_info->ops_retry)) {
			h_info->ops_retry--;
			goto retry;
		}
		pr_err("%s, map fail-smmuid[%d], ret[%d], err_code[%d]\n",
				__func__, smmuid, ret, h_info->err_code);
		goto free;
	}

	iova = h_info->iova;
free:
	vfree(h_info);

	return iova;
}

int viommu_unmap_sg(int smmuid, struct scatterlist *sgl, unsigned long iova)
{
	struct viommu_header_info *h_info = NULL;
	int ret;
	int sg_nent;
	unsigned long size;

	if (smmuid < 0 || smmuid >= SMMUID_MAX) {
		pr_err("%s, smmuid error!\n", __func__);
		return -EINVAL;
	}

	if (IS_ERR_OR_NULL(sgl)) {
		pr_err("%s, sgl is NULL!\n", __func__);
		return -EINVAL;
	}

	h_info = vzalloc(sizeof(*h_info));
	if (!h_info) {
		pr_err("%s, header info alloc fail!\n", __func__);
		return -ENOMEM;
	}

	sg_nent = sg_nents(sgl);
	size = sg_size_get(sgl, sg_nent);
	init_header_info(h_info, smmuid, SMMU_UNMAP_SYNC, 0,
							size, iova);
retry:
	ret = viommu_virtqueue_add_inbuf(h_info, NULL);
	if (ret || h_info->err_code) {
		if (should_retry(ret, h_info->err_code, h_info->ops_retry)) {
			h_info->ops_retry--;
			goto retry;
		}
		pr_err("%s, unmap fail-smmuid[%d], ret[%d], err_code[%d]\n",
				__func__, smmuid, ret, h_info->err_code);
		return -EINVAL;
	}

	vfree(h_info);

	return 0;
}

void dmabuf_release_viommu(struct dma_buf *dmabuf)
{
	struct vm_iova_dom *v_iova_dom = NULL;
	struct viommu_header_info *h_info = NULL;
	unsigned long iova;
	unsigned long size;
	int smmuid;
	int ret;
	int i;

	if (IS_ERR_OR_NULL(dmabuf) || IS_ERR_OR_NULL(dmabuf->priv)) {
		pr_err("%s, dmabuf or dmabuf->priv is NULL!\n", __func__);
		return;
	}

	h_info = vzalloc(sizeof(*h_info));
	if (!h_info) {
		pr_err("%s, header info alloc fail!\n", __func__);
		return;
	}

	for (i = 0; i < SMMUID_MAX; i++) {
		spin_lock(&vm_cookie[i].iova_lock);
		v_iova_dom = vm_mm_iova_dom_get(&vm_cookie[i].iova_root,
								dmabuf, 0);
		if (!v_iova_dom) {
			spin_unlock(&vm_cookie[i].iova_lock);
			continue;
		}
		iova = v_iova_dom->iova;
		size = v_iova_dom->size;
		smmuid = v_iova_dom->smmuid;
		spin_unlock(&vm_cookie[i].iova_lock);

		init_header_info(h_info, smmuid, SMMU_UNMAP_SYNC, 0,
					size, iova);
retry:
		ret = viommu_virtqueue_add_inbuf(h_info, NULL);
		if (ret || h_info->err_code) {
			if (should_retry(ret, h_info->err_code,
						h_info->ops_retry)) {
				h_info->ops_retry--;
				goto retry;
			}
			pr_err("%s, unmap fail-smmuid[%d], ret[%d], err_code[%d]\n",
				__func__, smmuid, ret, h_info->err_code);
			vm_mm_iova_dom_info(i);
			continue;
		}
		spin_lock(&vm_cookie[i].iova_lock);
		atomic64_set(&v_iova_dom->ref, 0);
		rb_erase(&v_iova_dom->node, &vm_cookie[i].iova_root);
		spin_unlock(&vm_cookie[i].iova_lock);

		kfree(v_iova_dom);
	}

	vfree(h_info);
}

#ifdef CONFIG_MM_IOMMU_TEST
void viommu_dump_pgtable(int smmuid, unsigned long iova, size_t iova_size)
{
	struct viommu_header_info *h_info = NULL;
	int ret;

	h_info = vzalloc(sizeof(*h_info));
	if (!h_info) {
		pr_err("%s, header info alloc fail!\n", __func__);
		return;
	}

	init_header_info(h_info, smmuid, SMMU_DUMP_PGT_SYNC, 0,
				iova_size, iova);

	ret = viommu_virtqueue_add_inbuf(h_info, NULL);
	if (ret || h_info->err_code)
		pr_err("%s, dump pgtable fail, ret = %d, err_code = %d\n",
					__func__, ret, h_info->err_code);

	vfree(h_info);
}
#endif

static int virtio_iommu_probe(struct virtio_device *dev)
{
	struct virtio_iommu_info *vfi;
	int i;

	vfi = kmalloc(sizeof(*vfi), GFP_KERNEL);
	if (!vfi)
		return -ENOMEM;

	vfi->vq = virtio_find_single_vq(dev, NULL, "viommu");
	if (vfi->vq == NULL) {
		pr_err("%s, can not find virtqueue!\n", __func__);
		kfree(vfi);
		return -EINVAL;
	}

	spin_lock_init(&vfi->request_lock);
	g_viommu_info = vfi;

	for (i = 0; i < SMMUID_MAX; i++) {
		spin_lock_init(&vm_cookie[i].iova_lock);
		vm_cookie[i].iova_root = RB_ROOT;
	}

	return 0;
}

static void virtio_iommu_remove(struct virtio_device *dev)
{
	kfree(g_viommu_info);
}

static struct virtio_device_id id_table[] = {
	{ VIRTIO_ID_IOMMU, VIRTIO_DEV_ANY_ID },
	{ 0 },
};

static struct virtio_driver virtio_iommu_driver = {
	.driver.name = KBUILD_MODNAME,
	.driver.owner = THIS_MODULE,
	.id_table = id_table,
	.probe = virtio_iommu_probe,
	.remove = virtio_iommu_remove,
};

static int virtio_iommu_init(void)
{
	return register_virtio_driver(&virtio_iommu_driver);
}

module_init(virtio_iommu_init);

MODULE_DEVICE_TABLE(virtio, id_table);
MODULE_DESCRIPTION("Virtio iommu driver");
