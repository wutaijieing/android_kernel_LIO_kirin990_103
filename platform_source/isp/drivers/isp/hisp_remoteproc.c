/*
 * HiStar Remote Processor driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/mod_devicetable.h>
#include <linux/amba/bus.h>
#include <linux/dma-mapping.h>
#include <linux/remoteproc.h>
#include <platform_include/isp/linux/hisp_remoteproc.h>
#include <platform_include/basicplatform/linux/ipc_rproc.h>
#include <linux/delay.h>
#include <linux/kfifo.h>
#include <linux/pm_wakeup.h>
#include <linux/mutex.h>
#include <linux/regulator/consumer.h>
#include <linux/scatterlist.h>
#include <linux/clk.h>
#include <uapi/linux/sched/types.h>
#include <linux/sched/rt.h>
#include <linux/kthread.h>
#include <uapi/linux/virtio_ring.h>
#include <global_ddr_map.h>
#include <asm/cacheflush.h>
#include <linux/firmware.h>
#include <linux/iommu.h>
#include <linux/mm_iommu.h>
#include <linux/crc32.h>
#include <linux/ion.h>
#include <linux/platform_drivers/mm_ion.h>
#include <linux/spinlock.h>
#include "isp_ddr_map.h"
#include <linux/dma-buf.h>
#include <linux/version.h>
#include <linux/syscalls.h>
#include <linux/sched/types.h>
#include <securec.h>
#include <asm/memory.h>
#include <hisp_internel.h>
#ifdef  CONFIG_RPMSG_HISP
#include <platform_include/isp/linux/hisp_rpmsg.h>
#endif
#ifdef DEBUG_HISP
#include "hisp_pcie.h"
#endif

#define RPMSG_RX_FIFO_DEEP      257
#define MBOX_REG_COUNT          8
#define HISP_VDEV_NAME_SIZE     64
#define RPROC_NAME_SIZE         32
#define ISP_RPMSG_VQ_SIZE       (0x40000)

#define DTS_COMP_NAME           "hisilicon,isp"
#define ISP_RSC_TABLE_SIZE      (0x1000)

enum hisp_rp_mbox_messages {
	RP_MBOX_READY           = 0xFFFFFF00,
	RP_MBOX_PENDING_MSG     = 0xFFFFFF01,
	RP_MBOX_CRASH           = 0xFFFFFF02,
	RP_MBOX_ECHO_REQUEST    = 0xFFFFFF03,
	RP_MBOX_ECHO_REPLY      = 0xFFFFFF04,
	RP_MBOX_ABORT_REQUEST   = 0xFFFFFF05,
};

enum hisp_fw_resource_type {
	RSC_RDRMEM_QUERY_ID      = 0,
	RSC_DYNMEM_QUERY_ID,
	RSC_SHARED_QUERY_ID,
	RSC_VINRG0_QUERY_ID,
	RSC_VINRG1_QUERY_ID,
	RSC_QUERY_ID_MAX,
	RSC_RDRMEM               = 128,
	RSC_DYNMEM,
	RSC_SHARED_PARA,
	RSC_VRING0MEM,
	RSC_VRING1MEM,
	RSC_MAX,
};

struct hisp_rproc_data {
	const char *name;
	const char *firmware;
	const char *mbox_name;
};

struct hisp_vring {
	dma_addr_t pa;
	void *va;
	size_t size;
};

struct hisp_vqueue {
	dma_addr_t pa;
	void *va;
	size_t size;
	u32 flags;
};

struct hisp_rx_mbox {
	struct kfifo rpmsg_rx_fifo;
	spinlock_t rpmsg_rx_lock;
	wait_queue_head_t wait;
	struct task_struct *rpmsg_rx_tsk;
	int can_be_wakeup;
};

struct hisp_rproc {
	unsigned int a7_ap_mbox;
	unsigned int ap_a7_mbox;
	unsigned int rpmsg_ready_state;
	void *imgva;
	spinlock_t rpmsg_ready_spin_mutex;
	struct iommu_domain *domain;
	struct list_head caches;
	struct hisp_vring vring[HISP_VRING_MAX];
	struct hisp_vqueue vqueue;
	struct hisp_rx_mbox rx_mbox;
	struct notifier_block nb;
	struct rproc *rproc;
	char vdev_name[HISP_VDEV_NAME_SIZE];
};

struct virtproc_info {
	struct virtio_device *vdev;
	struct virtqueue *rvq, *svq;
	void *rbufs, *sbufs;
	unsigned int num_bufs;
	unsigned int buf_size;
	int last_sbuf;
	dma_addr_t bufs_dma;
};

struct rproc_page_info {
	struct page *page;
	unsigned int order;
	struct list_head node;
};

/*
 * struct rproc_cache_entry - memory cache entry
 * @va: virtual address
 * @len: length, in bytes
 * @node: list node
 */
struct rproc_cache_entry {
	void *va;
	u32 len;
	struct list_head node;
};

struct rx_buf_data {
	bool is_have_data;
	unsigned int rpmsg_rx_len;
	mbox_msg_t rpmsg_rx_buf[MBOX_REG_COUNT];
};

typedef int (*hisp_handle_resource_t)(struct rproc *rproc,
		void *fw_rsc, int offset);

static unsigned int communicat_msg[8] = {0};
static const unsigned int orders[] = {8, 4, 0};
#define ISP_NUM_ORDERS ARRAY_SIZE(orders)

int hisp_rproc_set_domain(struct rproc *rproc, struct iommu_domain *domain)
{
	struct hisp_rproc *hproc = rproc->priv;

	if (hproc == NULL || domain == NULL) {
		pr_err("hproc or domain is NULL\n");
		return -EINVAL;
	}

	hproc->domain = domain;
	return 0;
}

struct iommu_domain *hisp_rproc_get_domain(struct rproc *rproc)
{
	struct hisp_rproc *hproc = rproc->priv;

	return hproc->domain;
}

void hisp_rproc_clr_domain(struct rproc *rproc)
{
	struct hisp_rproc *hproc = rproc->priv;

	hproc->domain = NULL;
}

void *hisp_rproc_get_imgva(struct rproc *rproc)
{
	struct hisp_rproc *hproc = rproc->priv;

	return hproc->imgva;
}

void hisp_rproc_get_vqmem(struct rproc *rproc, u64 *pa,
		size_t *size, u32 *flags)
{
	struct hisp_rproc *hproc = rproc->priv;

	*pa = (u64)hproc->vqueue.pa;
	*size = hproc->vqueue.size;
	*flags = hproc->vqueue.flags;
}

int hisp_rproc_vqmem_init(struct rproc *rproc)
{
	struct hisp_rproc *hproc = rproc->priv;
	struct hisp_vqueue *vq = &hproc->vqueue;
	struct device *r_dev = NULL, *v_dev = NULL;
	struct virtio_device *vdev = NULL;
	struct virtproc_info *vrp = NULL;
	int ret = 0;

	pr_debug("vdev name is %s\n", hproc->vdev_name);
	r_dev = device_find_child_by_name(&rproc->dev, hproc->vdev_name);
	if (r_dev == NULL) {
		pr_err("%s: get rvdev device failed \n", __func__);
		return -EINVAL;
	}

	v_dev = device_find_child_by_name(r_dev, "virtio0");
	if (v_dev == NULL) {
		pr_err("%s: get v_dev device failed \n", __func__);
		ret = -EINVAL;
		goto free_rdev;
	}

	vdev = dev_to_virtio(v_dev);
	if (vdev == NULL) {
		pr_err("%s: get vdev device failed \n", __func__);
		ret = -EINVAL;
		goto free_vrdev;
	}

	if (vdev->priv != NULL) {
		vrp = vdev->priv;
	} else {
		pr_err("%s: fail: vdev->priv is NULL \n", __func__);
		ret = -EINVAL;
		goto free_vrdev;
	}

	if (vrp->bufs_dma != 0) {
		vq->pa = vrp->bufs_dma;
	} else {
		pr_err("%s: fail: vrp->bufs_dma is NULL.\n", __func__);
		ret = -EINVAL;
		goto free_vrdev;
	}

	pr_debug("%s: vqueue pa is 0x%llx\n", __func__, vq->pa);
	vq->size = vrp->num_bufs * vrp->buf_size;
	vq->flags = IOMMU_READ | IOMMU_WRITE;

free_vrdev:
	put_device(v_dev);
free_rdev:
	put_device(r_dev);

	return ret;
}

u64 hisp_rproc_get_vringmem_pa(struct rproc *rproc, int id)
{
	struct hisp_rproc *hproc = rproc->priv;

	if (id > HISP_VRING_MAX) {
		pr_err("%s: type out of bound\n", __func__);
		return 0;
	}

	return hproc->vring[id].pa;
}

void hisp_rproc_set_rpmsg_ready(struct rproc *rproc)
{
	struct hisp_rproc *hproc = rproc->priv;

	spin_lock_bh(&hproc->rpmsg_ready_spin_mutex);
	hproc->rpmsg_ready_state = 1;
	spin_unlock_bh(&hproc->rpmsg_ready_spin_mutex);/*lint !e456 */
}

static inline unsigned int hisp_order_to_size(int order)
{
	return PAGE_SIZE << order;
}
/* try to alloc largest page orders */
static struct page *hisp_alloc_largest_pages(gfp_t gfp_mask, unsigned long size,
				unsigned int max_order, unsigned int *order)
{
	struct page *page   = NULL;
	gfp_t gfp_flags     = 0;
	int i               = 0;

	if (order == NULL) {
		pr_err("%s: order is NULL\n", __func__);
		return NULL;
	}

	for (i = 0; i < ISP_NUM_ORDERS; i++) {/*lint !e574*/
		if (size < hisp_order_to_size(orders[i]))
			continue;
		if (max_order < orders[i])
			continue;

		if (orders[i] >= 8) {
			gfp_flags = gfp_mask & (~__GFP_RECLAIM);
			gfp_flags |= __GFP_NOWARN;
			gfp_flags |= __GFP_NORETRY;
		} else if (orders[i] >= 4) {
			gfp_flags = gfp_mask & (~__GFP_DIRECT_RECLAIM);
			gfp_flags |= __GFP_NOWARN;
			gfp_flags |= __GFP_NORETRY;
		} else {
			gfp_flags = gfp_mask;
		}

		page = alloc_pages(gfp_flags, orders[i]);
		if (page == NULL)
			continue;
		*order = orders[i];

		return page;
	}

	return NULL;
}

static int hisp_alloc_mem_page(unsigned long length,
	struct list_head *pages_list, int *alloc_pages_num)
{
	struct rproc_page_info *page_info = NULL;
	unsigned int max_order       = orders[0];
	unsigned long size_remaining = PAGE_ALIGN(length);

	INIT_LIST_HEAD(pages_list);

	while (size_remaining > 0) {
		page_info = kzalloc(sizeof(struct rproc_page_info), GFP_KERNEL);
		if (page_info == NULL)
			return -ENOMEM;
		page_info->page = hisp_alloc_largest_pages(GFP_KERNEL,
			size_remaining, max_order, &page_info->order);
		if (!page_info->page) {
			pr_err("%s: alloc largest pages failed!\n", __func__);
			kfree(page_info);
			return -ENOMEM;
		}

		list_add_tail(&page_info->node, pages_list);
		size_remaining -= PAGE_SIZE << page_info->order;
		max_order = page_info->order;
		(*alloc_pages_num)++;
	}

	return 0;
}

/* dynamic alloc page and creat sg_table */
static struct sg_table *hisp_alloc_dynmem_sg_table(unsigned long length)
{
	struct list_head pages_list             = { 0 };
	struct rproc_page_info *page_info       = NULL;
	struct rproc_page_info *tmp_page_info   = NULL;
	struct sg_table *table       = NULL;
	struct scatterlist *sg       = NULL;
	int alloc_pages_num          = 0;
	int ret;

	INIT_LIST_HEAD(&pages_list);

	ret = hisp_alloc_mem_page(length, &pages_list, &alloc_pages_num);
	if (ret < 0) {
		pr_err("%s: hisp_alloc_mem_page fail\n", __func__);
		goto free_pages;
	}

	table = kmalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (table == NULL)
		goto free_pages;

	if (sg_alloc_table(table, alloc_pages_num, GFP_KERNEL))
		goto free_table;

	sg = table->sgl;
	list_for_each_entry_safe(page_info, tmp_page_info, &pages_list, node) {
		sg_set_page(sg, page_info->page,
			PAGE_SIZE << page_info->order, 0);
		sg = sg_next(sg);
		list_del(&page_info->node);
		kfree(page_info);
	}
	pr_debug("%s: pages num = %d, length = 0x%lx\n",
		__func__, alloc_pages_num, length);
	return table;

free_table:
	kfree(table);
free_pages:
	list_for_each_entry_safe(page_info, tmp_page_info, &pages_list, node) {
		__free_pages(page_info->page, page_info->order);
		list_del(&page_info->node);
		kfree(page_info);
	}

	return NULL;
}

/* dynamic free page and sg_table */
static void hisp_free_dynmem_sg_table(struct sg_table *table)
{
	struct scatterlist *sg  = NULL;
	unsigned int i          = 0;

	if (table == NULL) {
		pr_err("%s: table is NULL\n", __func__);
		return;
	}
	pr_debug("%s: table = 0x%pK\n", __func__, table);
	for_each_sg(table->sgl, sg, table->nents, i) {/*lint !e574*/
		__free_pages(sg_page(sg), get_order(PAGE_ALIGN(sg->length)));
	}
	sg_free_table(table);
	kfree(table);
}

static void hisp_get_alloced_page(struct sg_table *table,
	unsigned int npages, struct page **pages)
{
	unsigned int i, j;
	struct scatterlist *sg  = NULL;
	unsigned int npages_this_entry   = 0;
	struct page *page       = NULL;

	if ((table == NULL) || (pages == NULL)) {
		pr_err("%s: table or pages is NULL\n", __func__);
		return;
	}

	for_each_sg(table->sgl, sg, table->nents, i) {/*lint !e574*/
		npages_this_entry = (unsigned int)(PAGE_ALIGN(sg->length) / PAGE_SIZE);
		page = sg_page(sg);
		BUG_ON(i >= npages);
		for (j = 0; j < npages_this_entry; j++)
			*(pages++) = page++;/*lint !e613*/
	}
}

/* dynamic map kernel addr with sg_table */
static void *hisp_mem_map_kernel(struct sg_table *table, unsigned long length)
{
	void *vaddr             = NULL;
	unsigned int npages = (unsigned int)(PAGE_ALIGN(length) / PAGE_SIZE);
	struct page **pages;

	pages = vmalloc(sizeof(struct page *) * npages);
	if (pages == NULL)
		return NULL;

	hisp_get_alloced_page(table, npages, pages);

	vaddr = vmap(pages, npages, VM_MAP, PAGE_KERNEL);/*lint !e446 */
	vfree(pages);
	if (vaddr == NULL) {
		pr_err("%s: vmap failed.\n", __func__);
		return NULL;
	}
	return vaddr;
}
/* dynamic unmap kernel addr with sg_table */
static void hisp_mem_unmap_kernel(const void *va)
{
	if (va == NULL) {
		pr_err("%s: va is NULL\n", __func__);
		return;
	}
	vunmap(va);
}

static void *hisp_alloc_dynmem_table(struct fw_rsc_carveout *rsc)
{
	struct sg_table *table = NULL;

	if (rsc == NULL) {
		pr_err("%s: rsc is NULL!\n", __func__);
		return NULL;
	}

	pr_info("%s: dynamic_mem rsc: len %x, flags %x\n", __func__,
		rsc->len, rsc->flags);

	table = hisp_alloc_dynmem_sg_table(rsc->len);

	return table;
}

static int hisp_alloc_dynmem(struct rproc *rproc,
		struct rproc_mem_entry *mem)
{
	struct hisp_rproc *hproc = rproc->priv;
	struct sg_table *table = (struct sg_table *)mem->priv;
	struct rproc_cache_entry *cache_entry = NULL;
	void *va = NULL;
	size_t map_len;
	int ret;

	cache_entry = kzalloc(sizeof(*cache_entry), GFP_KERNEL);
	if (!cache_entry) {
		pr_err("%s: kzalloc cache entry fail\n", __func__);
		return -ENOMEM;
	}

	va = hisp_mem_map_kernel(table, mem->len);
	if (va == NULL) {
		pr_err("%s:vaddr_to_sgl failed\n", __func__);
		goto map_fail;
	}

	if (hproc->domain) {
		map_len = iommu_map_sg(hproc->domain, mem->da, table->sgl,
				sg_nents(table->sgl), mem->flags);
		if (map_len != mem->len) {
			pr_err("%s: iommu_map_sg failed: map_len %lx len 0x%lx\n",
					__func__, map_len, mem->len);
			goto free_mapping;
		}
	}

	pr_debug("%s: va %pK da 0x%x\n", __func__, va, mem->da);
	mem->va = va;
	cache_entry->va = va;
	cache_entry->len = mem->len;
	list_add_tail(&cache_entry->node, &hproc->caches);

	ret = strncmp(mem->name, "ISP_MEM_TEXT", strlen("ISP_MEM_TEXT"));
	if (!ret)
		hproc->imgva = mem->va;

	return 0;

free_mapping:
	hisp_mem_unmap_kernel(va);

map_fail:
	kfree(cache_entry);

	return -ENOMEM;
}

static int hisp_release_dynmem(struct rproc *rproc,
		struct rproc_mem_entry *mem)
{
	struct device *dev = &rproc->dev;
	struct hisp_rproc *hproc = rproc->priv;
	struct sg_table *table = (struct sg_table *)mem->priv;
	size_t unmapped = 0;

	if (hproc->domain) {
		if (hisp_smmuv3_mode()) {
			unmapped = mm_iommu_unmap_fast(dev->parent,
					mem->da, mem->len);
		} else {
			unmapped = iommu_unmap(hproc->domain,
					mem->da, mem->len);
		}

		if (unmapped != mem->len) {
			pr_err("%s, unmap fail, len 0x%lx\n", __func__, unmapped);
			return -EINVAL;
		}
	}

	hisp_mem_unmap_kernel(mem->va);
	hisp_free_dynmem_sg_table(table);
	hproc->imgva = NULL;

	return 0;
}

static int hisp_alloc_mem_common(struct rproc *rproc,
		struct rproc_mem_entry *mem)
{
	struct hisp_rproc *hproc = rproc->priv;
	int ret;

	if (hproc->domain) {
		ret = iommu_map(hproc->domain, mem->da, (u64)mem->dma,
					mem->len, mem->flags);
		if (ret) {
			pr_err("%s, iommu_map failed: %d\n", __func__, ret);
			return ret;
		}
	}

	return 0;
}

static int hisp_release_mem_common(struct rproc *rproc,
		struct rproc_mem_entry *mem)
{
	struct device *dev = &rproc->dev;
	struct hisp_rproc *hproc = rproc->priv;
	size_t unmapped = 0;

	if (hproc->domain) {
		if (hisp_smmuv3_mode()) {
			unmapped = mm_iommu_unmap_fast(dev->parent,
					mem->da, mem->len);
		} else {
			unmapped = iommu_unmap(hproc->domain,
					mem->da, mem->len);
		}

		if (unmapped != mem->len) {
			pr_err("%s, unmap fail, len 0x%lx\n", __func__, unmapped);
			return -EINVAL;
		}
	}

	return 0;
}

/**
 * hisp_handle_vringmem() - handle the vring memory
 * @rproc: the remote processor
 * @fw_rsc: the carveout resource descriptor
 * @offset: offset of version rsc in resource table
 * @index: vring id
 */
static int hisp_handle_vringmem(struct rproc *rproc,
		void *fw_rsc, int offset, int index)
{
	struct rproc_mem_entry *mem = NULL;
	struct fw_rsc_carveout *rsc = NULL;
	struct device *dev = &rproc->dev;
	struct hisp_rproc *hproc = rproc->priv;

	if (index >= HISP_VRING_MAX) {
		pr_err("%s: vring index %d out of bound\n", __func__, index);
		return -EINVAL;
	}

	rsc = (struct fw_rsc_carveout *)fw_rsc;
	pr_debug("%s: vring mem%d pa is 0x%llx\n", __func__, index, hproc->vring[index].pa);
	mem = rproc_mem_entry_init(dev, hproc->vring[index].va,
			hproc->vring[index].pa, rsc->len, rsc->da,
			hisp_alloc_mem_common, hisp_release_mem_common, rsc->name);
	if (mem == NULL) {
		pr_err("%s, Can't allocate memory entry structure\n", __func__);
		return -ENOMEM;
	}

	mem->flags = rsc->flags;
	rsc->pa = (u32)hproc->vring[index].pa;
	rproc_add_carveout(rproc, mem);

	return 0;
}

static int hisp_handle_vring0mem(struct rproc *rproc,
		void *fw_rsc, int offset)
{
	return hisp_handle_vringmem(rproc, fw_rsc, offset, HISP_VRING0);
}

static int hisp_handle_vring1mem(struct rproc *rproc,
		void *fw_rsc, int offset)
{
	return hisp_handle_vringmem(rproc, fw_rsc, offset, HISP_VRING1);
}

/**
 * hisp_handle_shrmem() - handle the share memory
 * @rproc: the remote processor
 * @fw_rsc: the carveout resource descriptor
 * @offset: offset of version rsc in resource table
 */
static int hisp_handle_shrmem(struct rproc *rproc,
		void *fw_rsc, int offset)
{
	struct rproc_mem_entry *mem = NULL;
	struct fw_rsc_carveout *rsc = NULL;
	struct device *dev = &rproc->dev;
	u64 pa = 0;

	rsc = (struct fw_rsc_carveout *)fw_rsc;
	pa = hisp_get_ispcpu_shrmem_pa();
	pr_debug("%s: sharemem pa is 0x%llx\n", __func__, pa);
	mem = rproc_mem_entry_init(dev, NULL, (u32)pa, rsc->len, rsc->da,
			hisp_alloc_mem_common, hisp_release_mem_common, rsc->name);
	if (mem == NULL) {
		pr_err("%s, Can't allocate memory entry structure\n", __func__);
		return -ENOMEM;
	}

	mem->flags = rsc->flags;
	rproc_add_carveout(rproc, mem);

	return 0;
}

/**
 * hisp_handle_dynmem() - handle the dynamic memory
 * @rproc: the remote processor
 * @fw_rsc: the carveout resource descriptor
 * @offset: offset of version rsc in resource table
 */
static int hisp_handle_dynmem(struct rproc *rproc,
		void *fw_rsc, int offset)
{
	struct rproc_mem_entry *mem = NULL;
	struct fw_rsc_carveout *rsc = NULL;
	struct device *dev = &rproc->dev;
	struct sg_table *table = NULL;

	rsc = (struct fw_rsc_carveout *)fw_rsc;
	table = hisp_alloc_dynmem_table(rsc);
	if (table == NULL) {
		dev_err(dev, "%s:vaddr_to_sgl failed\n", __func__);
		return -EINVAL;
	}

	mem = rproc_mem_entry_init(dev, NULL, 0, rsc->len, rsc->da,
			hisp_alloc_dynmem, hisp_release_dynmem, rsc->name);
	if (mem == NULL) {
		pr_err("%s, Can't allocate memory entry structure\n", __func__);
		hisp_free_dynmem_sg_table(table);
		return -ENOMEM;
	}

	mem->flags = rsc->flags;
	mem->priv = (void *)table;
	mem->rsc_offset = (unsigned int)offset;
	rproc_add_carveout(rproc, mem);

	return 0;
}

/**
 * hisp_handle_rdrmem() - handle the rdr memory
 * @rproc: the remote processor
 * @fw_rsc: the carveout resource descriptor
 * @offset: offset of version rsc in resource table
 */
static int hisp_handle_rdrmem(struct rproc *rproc,
		void *fw_rsc, int offset)
{
	struct rproc_mem_entry *mem = NULL;
	struct fw_rsc_carveout *rsc = NULL;
	struct device *dev = &rproc->dev;
	dma_addr_t pa;
	void *va = NULL;
	int ret;

	rsc = (struct fw_rsc_carveout *)fw_rsc;
	pa = (dma_addr_t)hisp_rdr_addr_get();
	pr_debug("%s: rdr mem pa is 0x%llx\n", __func__, pa);
	mem = rproc_mem_entry_init(dev, NULL, pa, rsc->len, rsc->da,
			hisp_alloc_mem_common, hisp_release_mem_common, rsc->name);
	if (mem == NULL) {
		pr_err("%s, Can't allocate memory entry structure\n", __func__);
		return -ENOMEM;
	}

	va = hisp_rdr_va_get();
	if (va == NULL) {
		pr_err("[%s] Failed : rdr_va is NULL\n", __func__);
	} else {
		ret = memset_s(va, mem->len, 0, mem->len);
		if (ret != 0)
			pr_err("%s: memset_s rdr to 0.%d!\n",
				__func__, ret);
	}

	mem->flags = rsc->flags;
	rproc_add_carveout(rproc, mem);

	return 0;
}

static hisp_handle_resource_t hisp_loading_handlers[RSC_QUERY_ID_MAX] = {
	[RSC_RDRMEM_QUERY_ID]     = hisp_handle_rdrmem,
	[RSC_DYNMEM_QUERY_ID]     = hisp_handle_dynmem,
	[RSC_SHARED_QUERY_ID]     = hisp_handle_shrmem,
	[RSC_VINRG0_QUERY_ID]     = hisp_handle_vring0mem,
	[RSC_VINRG1_QUERY_ID]     = hisp_handle_vring1mem,
};

static int hisp_rproc_start(struct rproc *rproc)
{
	struct hisp_rproc *hproc = rproc->priv;
	struct rproc_cache_entry *tmp = NULL;
	int ret;

	ret = hisp_device_start();
	if (ret != 0) {
		pr_err("%s: hisp_device_start fail.%d\n", __func__, ret);
		return ret;
	}

	list_for_each_entry(tmp, &hproc->caches, node) {
		__flush_dcache_area(tmp->va, tmp->len);
	}

	return 0;
}

static int hisp_rproc_stop(struct rproc *rproc)
{
	struct hisp_rproc *hproc = rproc->priv;
	struct rproc_cache_entry *tmp = NULL, *entry = NULL;

	RPROC_FLUSH_TX(hproc->ap_a7_mbox);
	list_for_each_entry_safe(entry, tmp, &hproc->caches, node) {
		list_del(&entry->node);
		kfree(entry);
	}

	spin_lock_bh(&hproc->rpmsg_ready_spin_mutex);
	hproc->rpmsg_ready_state = 0;
	spin_unlock_bh(&hproc->rpmsg_ready_spin_mutex);/*lint !e456 */

	return hisp_device_stop();
}

static int hisp_rproc_attach(struct rproc *rproc)
{
	struct hisp_rproc *hproc = rproc->priv;
	struct rproc_cache_entry *tmp = NULL;
	int ret;

	ret = hisp_device_attach(rproc);
	if (ret != 0) {
		pr_err("%s: hisp_device_attach fail.%d\n", __func__, ret);
		return ret;
	}

	list_for_each_entry(tmp, &hproc->caches, node) {
		__flush_dcache_area(tmp->va, tmp->len);
	}

	return 0;
}

static void hisp_rproc_kick(struct rproc *rproc, int vqid)
{
	struct hisp_rproc *dev = rproc->priv;
	int status = ISP_NORMAL_MODE;
	int ret = 0;

	/* Send the index of the triggered virtqueue in the mailbox payload */
	communicat_msg[0] = (unsigned int)vqid;

#ifdef DEBUG_HISP
	status = hisp_check_pcie_stat();
#endif
	switch (status) {
	case ISP_NORMAL_MODE:
		ret = RPROC_ASYNC_SEND(dev->ap_a7_mbox,
			communicat_msg, sizeof(communicat_msg[0]));
		break;
#ifdef DEBUG_HISP
	case ISP_PCIE_MODE:
		pr_debug("PCIE PLATFORM, use R8 mailbox\n");
		if (vqid == 0x0) {
			pr_err("%s: ignore first communicate at vqid: %d", __func__, vqid);
			return;
		}
		ret = hisp_pcie_send_ipc2isp(communicat_msg, sizeof(communicat_msg[0]));
		break;
#endif
	default:
		pr_err("unkown PLATFORM: 0x%x\n", status);
		return;
	}

	if (ret != 0)
		pr_err("Failed: RPROC_ASYNC_SEND.%d\n", ret);
	hisp_sendx();
	communicat_msg[0] = 0;
}

static int hisp_rproc_handle_rsc(struct rproc *rproc, u32 rsc_type,
		void *rsc, int offset, int avail)
{
	hisp_handle_resource_t handler;
	int ret;

	if (rsc == NULL) {
		pr_err("%s, rproc is NULL\n", __func__);
		return -EINVAL;
	}

	if (sizeof(struct fw_rsc_carveout) > (unsigned int)avail) {
		pr_err("%s: rsc table is truncated\n", __func__);
		return -EINVAL;
	}

	if (rsc_type >= RSC_MAX) {
		pr_err("%s: unsupported resource %u\n", __func__, rsc_type);
		return -EINVAL;
	}

	handler = hisp_loading_handlers[rsc_type - RSC_RDRMEM];

	ret = handler(rproc, rsc, offset);
	if (ret < 0) {
		pr_err("%s: handler resource %u fail\n", __func__, rsc_type);
		return ret;
	}

	return RSC_HANDLED;
}

static const struct rproc_ops hisp_rproc_ops = {
	.start             = hisp_rproc_start,
	.stop              = hisp_rproc_stop,
	.attach            = hisp_rproc_attach,
	.kick              = hisp_rproc_kick,
	.handle_rsc        = hisp_rproc_handle_rsc,
};

static void hisp_mbox_rx_work(struct rproc *rproc)
{
	struct hisp_rproc *dev = rproc->priv;
	struct hisp_rx_mbox *hisp_rx_mbox = &dev->rx_mbox;
	struct rx_buf_data mbox_reg;
	unsigned int len;
	irqreturn_t irqret;

	while (kfifo_len(&hisp_rx_mbox->rpmsg_rx_fifo)
			>= sizeof(struct rx_buf_data)) {
		len = kfifo_out_locked(&hisp_rx_mbox->rpmsg_rx_fifo,
					(unsigned char *)&mbox_reg,
					sizeof(struct rx_buf_data),
					&hisp_rx_mbox->rpmsg_rx_lock);
		if (len == 0) {/*lint !e84 */
			pr_err("Failed : kfifo_out_locked.%d\n", len);
			return;
		}

		/* maybe here need the flag of is_have_data */
		irqret = rproc_vq_interrupt(rproc,
				(int)mbox_reg.rpmsg_rx_buf[0]);
		if (irqret == IRQ_NONE)
			pr_debug("no message was found in vqid\n");
	}
}

static int hisp_mbox_rx_thread(void *context)
{
	struct hisp_rproc *dev = context;
	struct hisp_rx_mbox *hisp_rx_mbox = &dev->rx_mbox;
	struct rx_buf_data mbox_reg;
	int ret = 0;
	unsigned int len;

	while (!kthread_should_stop()) {
		ret = wait_event_interruptible(hisp_rx_mbox->wait,
				hisp_rx_mbox->can_be_wakeup == 1);
		hisp_rx_mbox->can_be_wakeup = 0;
		if (ret != 0) {
			pr_err("wait event failed\n");
			continue;
		}

		hisp_rproc_state_lock();
		if (dev->rproc->state != RPROC_RUNNING) {
			pr_err("[%s] hisp_rproc disable no power.%d\n",
				__func__, dev->rproc->state);
			while (kfifo_len(&hisp_rx_mbox->rpmsg_rx_fifo)
				>= sizeof(struct rx_buf_data)) {/*lint !e84 */
				len = kfifo_out_locked(
						&hisp_rx_mbox->rpmsg_rx_fifo,
						(unsigned char *)&mbox_reg,
						sizeof(struct rx_buf_data),
						&hisp_rx_mbox->rpmsg_rx_lock);
				if (len == 0)
					pr_err("Failed:kfifo_out.%d\n", len);
			}
			hisp_rproc_state_unlock();
			continue;
		}
		hisp_recvthread();
		hisp_mbox_rx_work(dev->rproc);
		hisp_rproc_state_unlock();
	}

	return 0;
}

static int hisp_rproc_mbox_kfifo(struct hisp_rproc *dev,
		mbox_msg_t *msg)
{
	struct hisp_rx_mbox *hisp_rx_mbox = &dev->rx_mbox;
	unsigned int i;
	unsigned int len;
	struct rx_buf_data mbox_reg;

	/* [ptr] msg : out layer not NULL*/
	pr_debug("default.%d\n", msg[0]);

	mbox_reg.rpmsg_rx_len = MBOX_REG_COUNT;
	for (i = 0; i < mbox_reg.rpmsg_rx_len; i++)
		mbox_reg.rpmsg_rx_buf[i] = msg[i];

	if (kfifo_avail(&hisp_rx_mbox->rpmsg_rx_fifo) <
			sizeof(struct rx_buf_data)) {/*lint !e84 */
		pr_err("rpmsg_rx_fifo is full\n");
		return -EINVAL;
	}

	len = kfifo_in_locked(&hisp_rx_mbox->rpmsg_rx_fifo,
				(unsigned char *)&mbox_reg,
				sizeof(struct rx_buf_data),
				&hisp_rx_mbox->rpmsg_rx_lock);
	if (len == 0) {
		pr_err("kfifo_in_locked failed\n");
		return -EINVAL;
	}
	pr_debug("kfifo_in_locked success!\n");

	return 0;
}

static int hisp_rproc_mbox_callback(struct notifier_block *this,
					unsigned long len, void *data)
{
	struct hisp_rproc *dev = container_of(this, struct hisp_rproc, nb);
	struct hisp_rx_mbox *hisp_rx_mbox = &dev->rx_mbox;
	mbox_msg_t *msg = (mbox_msg_t *)data;
	int ret;

	if (msg == NULL) {
		pr_err("%s: msg is NULL\n", __func__);
		return -EINVAL;
	}

	spin_lock_bh(&dev->rpmsg_ready_spin_mutex);
	if (dev->rpmsg_ready_state == 0) {
		pr_info("isp is powered off state\n");
		spin_unlock_bh(&dev->rpmsg_ready_spin_mutex);/*lint !e456 */
		return NOTIFY_DONE;
	}

	hisp_recvtask();
	switch (msg[0]) {
	case RP_MBOX_CRASH:/*lint !e650 */
		pr_info("hisp rproc crashed\n");
		break;

	case RP_MBOX_ECHO_REPLY:/*lint !e650 */
		pr_info("received echo reply\n");
		break;

	default:
		ret = hisp_rproc_mbox_kfifo(dev, msg);
		if (ret < 0) {
			pr_err("hisp_rproc_mbox_kfifo failed\n");
			spin_unlock_bh(&dev->rpmsg_ready_spin_mutex);
			return ret;
		}

		hisp_rx_mbox->can_be_wakeup = 1;
		wake_up_interruptible(&hisp_rx_mbox->wait);
	}
	spin_unlock_bh(&dev->rpmsg_ready_spin_mutex);/*lint !e456 */
	pr_debug("----%s rx msg X----\n", __func__);

	return NOTIFY_DONE;
}

static int hisp_rproc_data_name_getdts(struct device_node *np,
			struct hisp_rproc_data *data)
{
	int ret;

	ret = of_property_read_string(np, "isp-names", &data->name);
	if (ret < 0) {
		pr_err("[%s] Failed : isp-names.%s ret.%d\n",
			__func__, data->name, ret);
		return -EINVAL;
	}

	ret = of_property_read_string(np, "firmware-names", &data->firmware);
	if (ret < 0) {
		pr_err("[%s] Failed : firmware-names.%s ret.%d\n",
			__func__, data->firmware, ret);
		return -EINVAL;
	}

	ret = of_property_read_string(np, "mailbox-names", &data->mbox_name);
	if (ret < 0) {
		pr_err("[%s] Failed : mailbox-names.%s ret.%d\n",
			__func__, data->mbox_name, ret);
		return -EINVAL;
	}

	return ret;
}

static struct hisp_rproc_data *hisp_rproc_data_getdts(struct device *pdev)
{
	struct device_node *np = pdev->of_node;
	struct hisp_rproc_data *data = NULL;
	int ret;

	if (np == NULL) {
		pr_err("Failed : No dt node\n");
		return NULL;
	}

	data = devm_kzalloc(pdev, sizeof(struct hisp_rproc_data), GFP_KERNEL);
	if (data == NULL)
		return NULL;

	ret = hisp_rproc_data_name_getdts(np, data);
	if (ret < 0) {
		pr_err("Failed : hisp_rproc_data_name_getdts ret.%d\n", ret);
		goto isp_rproc_fail;
	}

	return data;
isp_rproc_fail:
	devm_kfree(pdev, data);
	return NULL;
}

static int hisp_ipc_resource_init(struct hisp_rproc *dev)
{
	struct hisp_rx_mbox *hisp_rx_mbox = &dev->rx_mbox;
	int ret;

	init_waitqueue_head(&hisp_rx_mbox->wait);
	hisp_rx_mbox->can_be_wakeup = 0;
	hisp_rx_mbox->rpmsg_rx_tsk =
		kthread_create(hisp_mbox_rx_thread, (void *)dev, "rpmsg_tx_tsk");

	if (unlikely(IS_ERR(hisp_rx_mbox->rpmsg_rx_tsk))) {
		pr_err("Failed : create kthread tx_kthread\n");
		return -EINVAL;
	} else {
		struct sched_param param;

		param.sched_priority = (MAX_RT_PRIO - 25);

		ret = sched_setscheduler(hisp_rx_mbox->rpmsg_rx_tsk,
					SCHED_RR, &param);
		if (ret < 0) {
			pr_err("Failed : sched_setscheduler\n");
			return ret;
		}
		wake_up_process(hisp_rx_mbox->rpmsg_rx_tsk);
	}

	spin_lock_init(&hisp_rx_mbox->rpmsg_rx_lock);

	if (kfifo_alloc(&hisp_rx_mbox->rpmsg_rx_fifo,
			sizeof(struct rx_buf_data) * RPMSG_RX_FIFO_DEEP,
			GFP_KERNEL)) {
		pr_err("Failed : kfifo_alloc\n");
		ret = -ENOMEM;
		goto kfifo_failure;
	}

	return 0;
kfifo_failure:
	if (IS_ERR(hisp_rx_mbox->rpmsg_rx_tsk))
		kthread_stop(hisp_rx_mbox->rpmsg_rx_tsk);

	return ret;
}

static void hisp_ipc_resource_exit(struct hisp_rproc *dev)
{
	struct hisp_rx_mbox *hisp_rx_mbox = &dev->rx_mbox;

	kfifo_free(&hisp_rx_mbox->rpmsg_rx_fifo);
	if (IS_ERR(hisp_rx_mbox->rpmsg_rx_tsk))
		kthread_stop(hisp_rx_mbox->rpmsg_rx_tsk);
}

static int hisp_rproc_vring_init(struct hisp_rproc *dev)
{
	struct hisp_vring *vring = NULL;
	struct device *device = dev->rproc->dev.parent;
	int i = 0;

	for (i = 0; i < HISP_VRING_MAX; i++) {
		vring = &dev->vring[i];
		vring->size = ISP_RPMSG_VR_SIZE;
		vring->va = dma_alloc_coherent(device,
			vring->size, &vring->pa, GFP_KERNEL);
		if (!vring->va) {
			pr_err("%s: hisp vring va map fail\n", __func__);
			goto out;
		}
	}

	return 0;

out:
	for (; i >= 0; i--) {
		vring = &dev->vring[i];
		if (vring == NULL)
			continue;

		dma_free_coherent(device, vring->size,
			vring->va, vring->pa);
	}

	return -ENOMEM;
}

static void hisp_rproc_vring_exit(struct hisp_rproc *dev)
{
	struct hisp_vring *vring = NULL;
	struct device *device = dev->rproc->dev.parent;
	int i = 0;

	for (i = 0; i < HISP_VRING_MAX; i++) {
		vring = &dev->vring[i];
		if (vring == NULL) {
			pr_err("%s: hisp_vring%d is NULL\n", __func__, i);
			continue;
		}

		dma_free_coherent(device, vring->size,
			vring->va, vring->pa);
		vring->va = NULL;
	}
}

static int hisp_rproc_vqueue_init(struct hisp_rproc *dev)
{
	struct hisp_vqueue *vqueue = &dev->vqueue;
	struct device *device = dev->rproc->dev.parent;

	vqueue->size = ISP_RPMSG_VQ_SIZE;
	vqueue->va = dma_alloc_coherent(device,
		vqueue->size, &vqueue->pa, GFP_KERNEL);
	if (!vqueue->va) {
		pr_err("%s: hisp vqueue va map fail\n", __func__);
		return -ENOMEM;
	}
	vqueue->flags = IOMMU_READ | IOMMU_WRITE;

	return 0;
}

static void hisp_rproc_vqueue_exit(struct hisp_rproc *dev)
{
	struct hisp_vqueue *vqueue = &dev->vqueue;
	struct device *device = dev->rproc->dev.parent;
	int i = 0;

	if (vqueue == NULL)
		pr_err("%s: hisp_vqueue is NULL\n", __func__);

	dma_free_coherent(device, vqueue->size,
		vqueue->va, vqueue->pa);
	vqueue->va = NULL;
}

static inline struct hisp_rproc *vdev_to_hisp_rproc(struct virtio_device *vdev)
{
	struct rproc_vdev *rvdev = NULL;
	struct rproc *rproc = NULL;

	rvdev = container_of(vdev->dev.parent, struct rproc_vdev, dev);
	rproc = rvdev->rproc;
	return rproc->priv;
}

void *get_vqueue_dma_addr(struct virtio_device *vdev, u64 *dma_handle, size_t size)
{
	struct hisp_rproc *dev = NULL;
	struct hisp_vqueue *vqueue = NULL;

	pr_info("[%s] : +\n", __func__);
	if (vdev == NULL) {
		pr_err("%s: vdev is NULL\n", __func__);
		return NULL;
	}

	dev = vdev_to_hisp_rproc(vdev);
	if (dev == NULL) {
		pr_err("%s: dev is NULL\n", __func__);
		return NULL;
	}

	vqueue = &dev->vqueue;
	if (vqueue == NULL) {
		pr_err("%s: hisp_vqueue is NULL\n", __func__);
		return NULL;
	}

	if (vqueue->size != size) {
		pr_err("%s: hisp_vqueue size not same: 0x%lx --> 0x%lx\n",
				__func__, vqueue->size, size);
		return NULL;
	}

	*dma_handle = vqueue->pa;
	pr_info("[%s] : -\n", __func__);
	return vqueue->va;
}

bool rpmsg_mem_alloc_beforehand(struct virtio_device *vdev)
{
	struct rproc_vdev *rvdev = NULL;
	struct rproc *rproc = NULL;

	if (vdev == NULL) {
		pr_err("%s: vdev is NULL\n", __func__);
		return false;
	}

	rvdev = container_of(vdev->dev.parent, struct rproc_vdev, dev);
	rproc = rvdev->rproc;

	if (strncmp(rproc->name, "ISPCPU", RPROC_NAME_SIZE))
		return false;

	return true;
}

static int hisp_rproc_init(struct hisp_rproc *dev)
{
	int ret;

	dev->ap_a7_mbox = IPC_ACPU_ISP_MBX_1;
	dev->a7_ap_mbox = IPC_ISP_ACPU_MBX_1;
	spin_lock_init(&dev->rpmsg_ready_spin_mutex);
	INIT_LIST_HEAD(&dev->caches);
	spin_lock_bh(&dev->rpmsg_ready_spin_mutex);
	dev->rpmsg_ready_state = 0;
	spin_unlock_bh(&dev->rpmsg_ready_spin_mutex);

	ret = hisp_rproc_vring_init(dev);
	if (ret != 0) {
		pr_err("hisp_rproc_vring_init fail. ret %d\n", ret);
		return ret;
	}
#ifdef CONFIG_HISP_VQUEUE_DMAALLOC_DEBUG
	ret = hisp_rproc_vqueue_init(dev);
	if (ret != 0) {
		pr_err("hisp_rproc_vqueue_init fail. ret %d\n", ret);
		goto vqueue_init_fail;
	}
#endif
	ret = hisp_ipc_resource_init(dev);
	if (ret != 0) {
		pr_err("hisp_ipc_resource_init fail. ret %d\n", ret);
		goto ipc_init_fail;
	}

	dev->nb.notifier_call = hisp_rproc_mbox_callback;
	ret = RPROC_MONITOR_REGISTER(dev->a7_ap_mbox,
		&dev->nb);/*lint !e64 */
	if (ret != 0) {
		pr_err("Failed : RPROC_MONITOR_REGISTER.%d\n", ret);
		goto isp_mbox_register_fail;
	}

	return 0;

isp_mbox_register_fail:
	hisp_ipc_resource_exit(dev);
ipc_init_fail:
#ifdef CONFIG_HISP_VQUEUE_DMAALLOC_DEBUG
	hisp_rproc_vqueue_exit(dev);
vqueue_init_fail:
#endif
	hisp_rproc_vring_exit(dev);

	return ret;
}

static void hisp_rproc_exit(struct hisp_rproc *dev)
{
	int ret;

	hisp_rproc_vring_exit(dev);
#ifdef CONFIG_HISP_VQUEUE_DMAALLOC_DEBUG
	hisp_rproc_vqueue_exit(dev);
#endif
	spin_lock_bh(&dev->rpmsg_ready_spin_mutex);
	dev->rpmsg_ready_state = 0;
	spin_unlock_bh(&dev->rpmsg_ready_spin_mutex);
	ret = RPROC_MONITOR_UNREGISTER(dev->a7_ap_mbox, &dev->nb);
	if (ret != 0)
		pr_err("Failed :RPROC_MONITOR_UNREGISTER.%d\n", ret);

	RPROC_FLUSH_TX(dev->ap_a7_mbox);
	dev->nb.notifier_call = NULL;
	hisp_ipc_resource_exit(dev);
}

static int hisp_rproc_probe(struct platform_device *pdev)
{
	struct rproc *rproc = NULL;
	struct hisp_rproc_data *data = NULL;
	struct hisp_rproc *dev = NULL;
	int ret;

	data = hisp_rproc_data_getdts(&pdev->dev);
	if (data == NULL) {
		pr_err("rproc data init fail\n");
		return -EINVAL;
	}

	pr_info("rproc_alloc\n");
	rproc = devm_rproc_alloc(&pdev->dev, data->name, &hisp_rproc_ops,
			data->firmware, sizeof(struct hisp_rproc));
	if (rproc == NULL) {
		pr_err("%s: devm_rproc_alloc fail\n", __func__);
		return -ENOMEM;
	}

	rproc->auto_boot = false;
	rproc->has_iommu = false;

	dev = rproc->priv;
	dev->rproc = rproc;
	platform_set_drvdata(pdev, rproc);

	pr_info("rproc_add\n");
	ret = devm_rproc_add(&pdev->dev, rproc);
	if (ret != 0) {
		pr_err("%s: devm_rproc_add fail. ret %d\n", __func__, ret);
		return ret;
	}

	ret = dma_set_coherent_mask(&pdev->dev,
			DMA_BIT_MASK(64));
	if (ret != 0) {
		dev_err(&pdev->dev, "dma_set_coherent_mask: %d\n", ret);
		return ret;
	}

	ret = snprintf_s(dev->vdev_name, sizeof(dev->vdev_name),
			sizeof(dev->vdev_name) - 1, "%s#%s",
			dev_name(&rproc->dev), "vdev0buffer");
	if (ret < 0) {
		pr_err("[%s]:snprintf_s vdev name failed.%d\n", __func__, ret);
		return ret;
	}

	ret = hisp_rproc_init(dev);
	if (ret != 0) {
		pr_err("%s: hisp_rproc_init fail. ret %d\n", __func__, ret);
		return ret;
	}

	ret = hisp_device_probe(pdev);
	if (ret != 0) {
		pr_err("%s: hisp_device_probe fail. ret %d\n", __func__, ret);
		hisp_rproc_exit(dev);
		return ret;
	}

	hisp_notify_probe(pdev);

	return 0;
}

static int hisp_rproc_remove(struct platform_device *pdev)
{
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct hisp_rproc *dev = rproc->priv;

	(void)hisp_device_remove(pdev);
	(void)hisp_rproc_exit(dev);
	hisp_notify_remove();

	return 0;
}

static const struct of_device_id hisp_rproc_of_match[] = {
	{ .compatible = DTS_COMP_NAME},
	{ },
};
MODULE_DEVICE_TABLE(of, hisp_rproc_of_match);

static struct platform_driver hisp_rproc_driver = {
	.driver = {
		.owner      = THIS_MODULE,
		.name       = "isp",
		.of_match_table = of_match_ptr(hisp_rproc_of_match),
	},
	.probe  = hisp_rproc_probe,
	.remove = hisp_rproc_remove,
};
module_platform_driver(hisp_rproc_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("HiStar Remote Processor control driver");

