

#include "hmac_vsp_source.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_VSP_SOURCE_BUF_C

#ifdef _PRE_WLAN_FEATURE_VSP
typedef struct {
    struct dma_buf *dmabuf;
    struct dma_buf_attachment *attachment;
    struct sg_table *table;
    oal_pci_dev_stru *pci_dev;
} hmac_vsp_dma_buf_context_stru;

static uint32_t hmac_vsp_source_init_dma_buf_context(hmac_vsp_dma_buf_context_stru *context)
{
    oal_pcie_linux_res *pcie_linux_res = oal_get_pcie_linux_res();

    if (!pcie_linux_res) {
        return OAL_FAIL;
    }

    context->pci_dev = pcie_linux_res->pst_pcie_dev;
    context->attachment = dma_buf_attach(context->dmabuf, &context->pci_dev->dev);
    context->table = dma_buf_map_attachment(attachment, DMA_BIDIRECTIONAL);

    return OAL_SUCC;
}

static void hmac_vsp_source_deinit_dma_buf_context(hmac_vsp_dma_buf_context_stru *context)
{
    dma_buf_unmap_attachment(context->attachment, context->table, DMA_BIDIRECTIONAL);
    dma_buf_detach(context->dmabuf, context->attachment);
}

uint32_t hmac_vsp_source_map_dma_buf(struct dma_buf *dmabuf, uintptr_t *iova)
{
    hmac_vsp_dma_buf_context_stru context = { .dmabuf = dmabuf };

    if (dma_buf_begin_cpu_access(dmabuf, DMA_FROM_DEVICE)) {
        return OAL_FAIL;
    }

    if (mac_vsp_source_init_dma_buf_context(&context) != OAL_SUCC) {
        return OAL_FAIL;
    }

    if (!dma_map_sg(&context.pci_dev->dev, context->table->sgl, context->table->nents, DMA_BIDIRECTIONAL)) {
        return OAL_FAIL;
    }

    *iova_addr = sg_dma_address(context->table->sgl);
    hmac_vsp_source_deinit_dma_buf_context(&context);

    return OAL_SUCC;
}

uint32_t hmac_vsp_source_unmap_dma_buf(struct dma_buf *dmabuf)
{
    hmac_vsp_dma_buf_context_stru context = { .dmabuf = dmabuf };

    dma_buf_kunmap(dmabuf, 0, NULL);

    if (dma_buf_end_cpu_access(dmabuf, DMA_TO_DEVICE)) {
        return OAL_FAIL;
    }

    if (hmac_vsp_source_init_dma_buf_context(&context) != OAL_SUCC) {
        return OAL_FAIL;
    }

    dma_unmap_sg(&context.pci_dev->dev, context->table->sgl, context->table->nents, DMA_BIDIRECTIONAL);
    hmac_vsp_source_deinit_dma_buf_context(&context);

    return OAL_SUCC;
}

#endif
