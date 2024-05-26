#include "hisi_disp_iommu.h"
#include "hisi_disp_gadgets.h"

#ifdef DEBUG_BBIT_WITH_VCODEC
// bbit with venc
#include "vcodec_type.h"
#endif

struct platform_device *g_hdss_platform_device;
static struct dss_mm_info g_mm_list;

static phys_addr_t hisi_iommu_iova_to_phys(struct device *dev, dma_addr_t iova)
{
	struct iommu_domain *domain = NULL;

	domain = iommu_get_domain_for_dev(dev);
	if (!domain) {
		dev_err(dev, "%s has no iommu domain!\n", __func__);
		return 0;
	}

	return iommu_iova_to_phys(domain, iova);
}

void *hisi_iommu_iova_to_va(dma_addr_t iova)
{
	return phys_to_virt(hisi_iommu_iova_to_phys(__hdss_get_dev(), iova));
}

int hisi_disp_alloc_cma_buffer(size_t size, dma_addr_t *dma_handle, void **cpu_addr)
{
	*cpu_addr = dma_alloc_coherent(__hdss_get_dev(), size, dma_handle, GFP_KERNEL);
	if (!(*cpu_addr)) {
		disp_pr_info("dma alloc coherent failed!\n");
		return -ENOMEM;
	}

	return 0;
}

int hisi_dss_alloc_cma_buffer(size_t size, dma_addr_t *dma_handle, void **cpu_addr)
{
	*cpu_addr = dma_alloc_coherent(__hdss_get_dev(), size, dma_handle, GFP_KERNEL);
	if (!(*cpu_addr)) {
		disp_pr_info("dma alloc coherent failed!\n");
		return -ENOMEM;
	}

	return 0;
}

int hisi_dss_iommu_enable(struct platform_device *pdev)
{
	struct dss_mm_info *mm_list = &g_mm_list;
	int ret;

	if (!pdev) {
		disp_pr_err("pdev is NULL\n");
		return -EINVAL;
	}
	kernel_domain_get_ttbr(&pdev->dev);

	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));  /*lint !e598*/
	if (ret != 0)
		disp_pr_err("dma set mask and coherent failed\n");

	g_hdss_platform_device = pdev;

	spin_lock_init(&mm_list->map_lock);
	INIT_LIST_HEAD(&mm_list->mm_list);

	disp_pr_info(" ---- \n");

	return 0;
}

void hisi_dss_put_dmabuf(struct dma_buf *buf)
{
	if (IS_ERR_OR_NULL(buf)) {
		disp_pr_err("Invalid dmabuf(%pK)\n", buf);
		return;
	}

	dma_buf_put(buf);
}

struct dma_buf *hisi_dss_get_dmabuf(int sharefd)
{
	struct dma_buf *buf = NULL;

	/* dim layer share fd -1 */
	if (sharefd < 0) {
		disp_pr_err("Invalid file sharefd = %d\n", sharefd);
		return NULL;
	}

	buf = dma_buf_get(sharefd);
	if (IS_ERR_OR_NULL(buf)) {
		disp_pr_err("Invalid file buf(%pK), sharefd = %d\n", buf, sharefd);
		return NULL;
	}

	return buf;
}

struct dma_buf *hisi_dss_get_buffer_by_sharefd(uint64_t *iova, int fd, uint32_t size)
{
	struct dma_buf *buf = NULL;
	unsigned long buf_size = 0;

	buf = hisi_dss_get_dmabuf(fd);
	if (IS_ERR_OR_NULL(buf)) {
		disp_pr_err("Invalid file shared_fd[%d]\n", fd);
		return NULL;
	}

	*iova = kernel_iommu_map_dmabuf(__hdss_get_dev(), buf, 0, &buf_size);
	if ((*iova == 0) || (buf_size < size)) {
		disp_pr_err("get iova_size(0x%llx) smaller than size(0x%x)\n",
			buf_size, size);
		if (*iova != 0) {
			(void)kernel_iommu_unmap_dmabuf(__hdss_get_dev(), buf, *iova);
			*iova = 0;
		}
		hisi_dss_put_dmabuf(buf);
		return NULL;
	}

	return buf;
}

static int32_t hisi_dss_get_sgt_domain(struct dma_buf *dmabuf, struct sg_table *sgt, struct iommu_domain *domain)
{
	struct dma_buf_attachment *attach = NULL;

	attach = dma_buf_attach(dmabuf, __hdss_get_dev());
	if (!attach) {
		disp_pr_err("failed to attach the dmabuf\n");
		return -ENOMEM;
	}

	sgt = dma_buf_map_attachment(attach, DMA_TO_DEVICE);
	if (!sgt) {
		disp_pr_err("failed to map dma buf to get sgt\n");
		return -ENOMEM;
	}

	domain = iommu_get_domain_for_dev(__hdss_get_dev());
	if (!domain) {
		disp_pr_err("failed to get domain\n");
		return -ENOMEM;
	}

	disp_pr_info("domain=0x%x\n", domain);
	return 0;
}

// DEBUG: map shared iova
int32_t hisi_dss_dmabuf_map_iova(struct dma_buf *dmabuf, uint64_t iova)
{
	uint32_t i = 0;
	size_t mapped = 0;
	struct sg_table *sgt = NULL;
	struct iommu_domain *domain = NULL;
	struct scatterlist *sg = NULL;
	struct page *pg = NULL;
	phys_addr_t phys;

	disp_pr_info("+++ iova=0x%lx\n", iova);

	if (!g_hdss_platform_device) {
		disp_pr_err("g_hdss_platform_device is NULL\n");
		return -EINVAL;;
	}

	if(hisi_dss_get_sgt_domain(dmabuf, sgt, domain))
		return -ENOMEM;

	uint32_t prot = IOMMU_READ | IOMMU_WRITE | IOMMU_CACHE;
	for_each_sg(sgt->sgl, sg, sgt->nents, i) {
		pg = sg_page(sg);
		phys = page_to_phys(pg + sg->offset);
		disp_pr_info("phys=0x%x length=%d\n", phys, sg->length);
		iommu_map(domain, iova + mapped, phys, sg->length, prot); // prot;
		mapped += sg->length;
	}

#ifdef DEBUG_BBIT_WITH_VCODEC
	// bbit with venc
	if (g_debug_bbit_venc == 1) {
		if (venc_dmabuf_map_iova(dmabuf, iova) == 0) {
			disp_pr_info("venc map iova succ\n");
		} else {
			disp_pr_info("venc map iova failed\n");
		}
	}
#endif

	disp_pr_info("---\n");

	return 0;
}

// DEBUG: unmap shared iova
int32_t hisi_dss_dmabuf_unmap_iova(struct dma_buf *dmabuf, uint64_t iova)
{
	uint32_t i = 0;
	size_t iova_size = 0;
	size_t unmap_size = 0;
	struct sg_table *sgt = NULL;
	struct iommu_domain *domain = NULL;
	struct scatterlist *sg = NULL;

	disp_pr_info("+++ iova=0x%lx\n", iova);

#ifdef DEBUG_BBIT_WITH_VCODEC
	// bbit with venc
	if (g_debug_bbit_venc == 1) {
		if (venc_dmabuf_unmap_iova(dmabuf, iova) == 0) {
			disp_pr_info("venc unmap iova succ\n");
		} else {
			disp_pr_info("venc unmap iova failed\n");
		}
	}
#endif

	if (!g_hdss_platform_device) {
		disp_pr_err("g_hdss_platform_device is NULL\n");
		return -EINVAL;;
	}

	if(hisi_dss_get_sgt_domain(dmabuf, sgt, domain))
		return -ENOMEM;

	for_each_sg(sgt->sgl, sg, sgt->nents, i)
		iova_size += sg->length;

	unmap_size = iommu_unmap(domain, iova, iova_size);
	if (unmap_size != iova_size) {
		disp_pr_err("unmap fail!size 0x%zx, unmap_size 0x%zx\n",
			iova_size, unmap_size);
		return -EFAULT;
	}

	disp_pr_info("---\n");

	return 0;
}

struct dma_buf *hisi_dss_binding_iova(uint64_t share_iova, int fd)
{
	struct dma_buf *buf = NULL;
	unsigned long buf_size = 0;

	buf = hisi_dss_get_dmabuf(fd);
	if (IS_ERR_OR_NULL(buf)) {
		disp_pr_err("Invalid file shared_fd[%d]\n", fd);
		return NULL;
	}

	int32_t ret = hisi_dss_dmabuf_map_iova(buf, share_iova);
	if (ret) {
		disp_pr_err("dmabuf map iova failed\n");
		return NULL;
	}

	return buf;
}

void hisi_dss_put_buffer_by_dmabuf(uint64_t iova, struct dma_buf *dmabuf)
{
	if (IS_ERR_OR_NULL(dmabuf)) {
		disp_pr_err("Invalid dmabuf(%pK)\n", dmabuf);
		return;
	}
	(void)kernel_iommu_unmap_dmabuf(__hdss_get_dev(), dmabuf, iova);

	hisi_dss_put_dmabuf(dmabuf);
}

int hisi_dss_buffer_map_iova(struct fb_info *info, void __user *arg)
{
	disp_pr_debug(" ++++ \n");
	struct dss_iova_info *node = NULL;
	struct dss_mm_info *mm_list = &g_mm_list;
	iova_info_t map_data;

	if (!arg) {
		disp_pr_err("arg is NULL\n");
		return -EFAULT;
	}
	node = kzalloc(sizeof(struct dss_iova_info), GFP_KERNEL);
	if (!node) {
		disp_pr_err("alloc display meminfo failed\n");
		goto error;
	}

	if (copy_from_user(&map_data, (void __user *)arg, sizeof(map_data))) {
		disp_pr_err("copy_from_user failed\n");
		goto error;
	}

	node->iova_info.share_fd = map_data.share_fd;
	node->iova_info.calling_pid = map_data.calling_pid;
	node->iova_info.size = map_data.size;
	node->is_shared_iova = false;

	disp_pr_debug("map_data.iova=0x%lx\n", map_data.iova);
	if (map_data.iova) {
		node->dmabuf = hisi_dss_binding_iova(map_data.iova, map_data.share_fd);
		node->is_shared_iova = true;
	} else {
		node->dmabuf = hisi_dss_get_buffer_by_sharefd(&map_data.iova,
			map_data.share_fd, map_data.size);
	}
	if (!node->dmabuf) {
		disp_pr_err("dma buf map share_fd(%d) failed\n", map_data.share_fd);
		goto error;
	}
	node->iova_info.iova = map_data.iova;

	if (copy_to_user((void __user *)arg, &map_data, sizeof(map_data))) {
		disp_pr_err("copy_to_user failed\n");
		goto error;
	}

	/* save map list */
	spin_lock(&mm_list->map_lock);
	list_add_tail(&node->list_node, &mm_list->mm_list);
	spin_unlock(&mm_list->map_lock);

	//DDTF(g_debug_dump_iova, hisi_dss_buffer_iova_dump);
	disp_pr_debug(" ---- \n");
	return 0;

error:
	if (node) {
		if (node->dmabuf)
			hisi_dss_put_buffer_by_dmabuf(node->iova_info.iova, node->dmabuf);
		kfree(node);
	}
	return -EFAULT;
}

int hisi_dss_buffer_unmap_iova(struct fb_info *info, const void __user *arg)
{
	struct dma_buf *dmabuf = NULL;
	struct dss_iova_info *node = NULL;
	struct dss_iova_info *_node_ = NULL;
	struct dss_mm_info *mm_list = &g_mm_list;
	iova_info_t umap_data;

	disp_pr_debug(" ++++ \n");

	if (!arg) {
		disp_pr_err("arg is NULL\n");
		return -EFAULT;
	}

	if (copy_from_user(&umap_data, (void __user *)arg, sizeof(umap_data))) {
		disp_pr_err("copy_from_user failed\n");
		return -EFAULT;
	}
	dmabuf = hisi_dss_get_dmabuf(umap_data.share_fd);

	spin_lock(&mm_list->map_lock);
	list_for_each_entry_safe(node, _node_, &mm_list->mm_list, list_node) {
		if (node->dmabuf == dmabuf) {
			list_del(&node->list_node);
			if (node->is_shared_iova) {
				(void)hisi_dss_dmabuf_unmap_iova(node->dmabuf, node->iova_info.iova);
			}

			/* already map, need put twice. */
			hisi_dss_put_dmabuf(node->dmabuf);
			/* iova would be unmapped by dmabuf put. */
			kfree(node);
		}
	}
	spin_unlock(&mm_list->map_lock);

	hisi_dss_put_dmabuf(dmabuf);

	//DDTF(g_debug_dump_iova, hisi_dss_buffer_iova_dump);
	disp_pr_debug(" ---- \n");

	return 0;
}
