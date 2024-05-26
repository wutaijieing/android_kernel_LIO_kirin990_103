#ifndef HISI_DISP_IOMMU_H
#define HISI_DISP_IOMMU_H
#include <linux/iommu.h>
#include <linux/fb.h>
#include <linux/dma-mapping.h>
#include <linux/dma-buf.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <linux/mm_iommu.h>
#include <linux/uaccess.h>

struct dss_mm_info {
	struct list_head mm_list;
	spinlock_t map_lock;
};

typedef struct iova_info {
	uint64_t iova;
	uint64_t size;
	int share_fd;
	int calling_pid;
} iova_info_t;

struct dss_iova_info {
	struct list_head list_node;
	struct dma_buf *dmabuf;
	iova_info_t iova_info;
	bool is_shared_iova;
};

#ifdef CONFIG_DPU_FB_ENG_DBG
#define DDTF(tag, func)    \
	do {                    \
		if (tag) func();     \
	} while (0)
#else
#define DDTF(tag, func)
#endif

extern struct platform_device *g_hdss_platform_device;

static inline struct device *__hdss_get_dev(void)
{
	if (g_hdss_platform_device == NULL) {
		pr_err("g_hdss_platform_device is null.\n");
		return NULL;
	}

	return &(g_hdss_platform_device->dev);
}

int hisi_dss_buffer_map_iova(struct fb_info *info, void __user *arg);
int hisi_dss_buffer_unmap_iova(struct fb_info *info, const void __user *arg);
int hisi_dss_iommu_enable(struct platform_device *pdev);
int hisi_dss_alloc_cma_buffer(size_t size, dma_addr_t *dma_handle, void **cpu_addr);
int hisi_disp_alloc_cma_buffer(size_t size, dma_addr_t *dma_handle, void **cpu_addr);
void *hisi_iommu_iova_to_va(dma_addr_t iova);
struct dma_buf *hisi_dss_get_buffer_by_sharefd(uint64_t *iova, int fd, uint32_t size);
int32_t hisi_dss_dmabuf_map_iova(struct dma_buf *dmabuf, uint64_t iova);
int32_t hisi_dss_dmabuf_unmap_iova(struct dma_buf *dmabuf, uint64_t iova);
#endif