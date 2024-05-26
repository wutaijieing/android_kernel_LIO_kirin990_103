
#include "oal_hcc_host_if.h"
#include "oam_ext_if.h"

int32_t g_pcie_dma_data_rx_check = 0; /* Wi-Fi关闭时可以修改此标记 */
oal_debug_module_param(g_pcie_dma_data_rx_check, int, S_IRUGO | S_IWUSR);

void oal_pcie_release_rx_netbuf(oal_pcie_linux_res *pci_res, oal_netbuf_stru *netbuf)
{
    pcie_cb_dma_res *cb_res = NULL;
    oal_pci_dev_stru *pci_dev = NULL;
    if (oal_warn_on(netbuf == NULL)) {
        return;
    }

    if (oal_warn_on(pci_res == NULL)) {
        declare_dft_trace_key_info("pcie release rx netbuf", OAL_DFT_TRACE_FAIL);
        oal_netbuf_free(netbuf);
        return;
    }

    pci_dev = pci_res->comm_res->pcie_dev;
    if (oal_warn_on(pci_dev == NULL)) {
        declare_dft_trace_key_info("pcie release rx netbuf", OAL_DFT_TRACE_FAIL);
        oal_netbuf_free(netbuf);
        return;
    }

    cb_res = (pcie_cb_dma_res *)oal_netbuf_cb(netbuf);
    if (oal_likely((cb_res->paddr.addr != 0) && (cb_res->len != 0))) {
        dma_unmap_single(&pci_dev->dev, (dma_addr_t)cb_res->paddr.addr, cb_res->len, PCI_DMA_FROMDEVICE);
    } else {
        declare_dft_trace_key_info("pcie release rx netbuf", OAL_DFT_TRACE_FAIL);
    }

    oal_netbuf_free(netbuf);
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
static inline void oal_pcie_invalid_rx_buf_dcache(oal_pci_dev_stru *pci_dev, dma_addr_t dma_addr, int32_t len)
{
    oal_pci_cache_inv(pci_dev, dma_addr, len);
}
#endif

static int32_t oal_pcie_rx_databuf_verify(oal_pci_dev_stru *pci_dev, oal_netbuf_stru *netbuf)
{
    if (g_pcie_dma_data_rx_check != 0) {
        // 规避方案打开
        return OAL_SUCC;
    }

    if ((*(volatile uint32_t*)oal_netbuf_data(netbuf)) != PCIE_RX_TRANS_FLAG) {
        // 默认 认为传输完成，after unmap
        return OAL_SUCC; // succ, buf had rewrite by pcie
    }

    oam_error_log0(0, OAM_SF_ANY, "pcie rx netbuf check failed, invalid netbuf, loss pkt!");
    declare_dft_trace_key_info("invalid_pcie_rx_netbuf", OAL_DFT_TRACE_FAIL);
    oal_print_hex_dump((uint8_t *)(oal_netbuf_data(netbuf)),
                       HCC_HDR_TOTAL_LEN, HEX_DUMP_GROUP_SIZE, "err_hdr ");

    return -OAL_EFAIL;
}

void oal_pcie_load_rx_buf_cache(oal_netbuf_stru *netbuf)
{
    volatile uint32_t j;
    uint32_t len = oal_netbuf_len(netbuf);
    volatile uint8_t *data = (volatile uint8_t*)oal_netbuf_data(netbuf);
    for (j = 0; j < len; j++) {
        *(data + j);
    }
}

#define PCIE_RX_BUF_PENDING_WAIT_CNT 1000
static int32_t oal_pcie_rx_databuf_verify_fix(oal_pci_dev_stru *pci_dev, oal_netbuf_stru *netbuf,
                                              dma_addr_t dma_addr, int32_t len)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    uint32_t i;
    unsigned long flags;

    if (g_pcie_dma_data_rx_check == 0) {
        // 规避方案关闭
        return OAL_SUCC;
    }

    local_irq_save(flags);
    // 这里防止CPU投机访问已经Load Cache需要invalid一次
    oal_pcie_invalid_rx_buf_dcache(pci_dev, dma_addr, len);
    /* pcie rx netbuf自己申请，首地址保证对齐 */
    if ((*(volatile uint32_t*)oal_netbuf_data(netbuf)) != PCIE_RX_TRANS_FLAG) {
        // 默认 认为传输完成，后面还有一次invalid cache, unmap操作
        local_irq_restore(flags);
        return OAL_SUCC; // succ, buf had rewrite by pcie
    }

    oal_pcie_invalid_rx_buf_dcache(pci_dev, dma_addr, len);
    local_irq_restore(flags);

    declare_dft_trace_key_info("pcie_rx_buf_pending_in_ddr", OAL_DFT_TRACE_OTHER);

    /* 等到ddr数据刷新,规避ddr 反压导致的pending问题 */
    local_irq_save(flags);
    for (i = 0; i < PCIE_RX_BUF_PENDING_WAIT_CNT; i++) {
        oal_pcie_invalid_rx_buf_dcache(pci_dev, dma_addr, len);
        local_irq_restore(flags);
        /* 防止关中断过久，保证cache缓存时中断是关闭状态 */
        local_irq_save(flags);
        if ((*(volatile uint32_t*)oal_netbuf_data(netbuf)) != PCIE_RX_TRANS_FLAG) {
            break;
        }
    }

    if (i == PCIE_RX_BUF_PENDING_WAIT_CNT) {
        // timeout, ddr invalid, rx failed
        declare_dft_trace_key_info("invalid_pcie_rx_netbuf_fix_failed", OAL_DFT_TRACE_FAIL);
        oal_print_hex_dump((uint8_t *)(oal_netbuf_data(netbuf)),
                           HCC_HDR_TOTAL_LEN, HEX_DUMP_GROUP_SIZE, "err_hdr ");
        local_irq_restore(flags);
        oam_error_log0(0, OAM_SF_ANY, "pcie rx netbuf check failed, try to fix failed!");
        return -OAL_ETIMEDOUT;
    }

    // 此处说明PCIE已经把buf 头4字节写完, 无效cache, 重新装载整个报文到cache
    oal_pcie_invalid_rx_buf_dcache(pci_dev, dma_addr, len);

    /* cpu 主动load一次cache,保证pcie把所有报文内容写入ddr
    * pcie burst写一般比cpu single读要快很多 */
    oal_pcie_load_rx_buf_cache(netbuf);

    oal_pcie_invalid_rx_buf_dcache(pci_dev, dma_addr, len);
    local_irq_restore(flags);

    /* wait pcie pending data to ddr succ, invalid cache, try to read data from ddr */
    declare_dft_trace_key_info("pcie_rx_buf_pending_retry_succ", OAL_DFT_TRACE_OTHER);
#endif

    return OAL_SUCC;
}

/* 向Hcc层提交收到的netbuf */
void oal_pcie_rx_netbuf_submit(oal_pcie_linux_res *pci_lres, oal_netbuf_stru *netbuf)
{
    int32_t ret1, ret2;
    struct hcc_handler *hcc = NULL;
    oal_pci_dev_stru *pci_dev = NULL;
    pcie_cb_dma_res *cb_res = NULL;

    pci_dev = pci_lres->comm_res->pcie_dev;
    if (oal_unlikely(pci_dev == NULL)) {
        goto release_netbuf;
    }

    if (oal_unlikely(pci_lres->pst_bus == NULL)) {
        pci_print_log(PCI_LOG_ERR, "lres's bus is null");
        goto release_netbuf;
    }

    if (oal_unlikely(hbus_to_dev(pci_lres->pst_bus) == NULL)) {
        pci_print_log(PCI_LOG_ERR, "lres's dev is null");
        goto release_netbuf;
    }

    hcc = hbus_to_hcc(pci_lres->pst_bus);
    if (oal_unlikely(hcc == NULL)) {
        pci_print_log(PCI_LOG_ERR, "lres's hcc is null");
        goto release_netbuf;
    }

    cb_res = (pcie_cb_dma_res *)oal_netbuf_cb(netbuf);
    if (oal_unlikely((cb_res->paddr.addr == 0) || (cb_res->len == 0))) {
        goto release_netbuf;
    }

    /* verify rx done buf's hdr, transfer from pcie dma */
    ret1 = oal_pcie_rx_databuf_verify_fix(pci_dev, netbuf, (dma_addr_t)cb_res->paddr.addr,
                                          (int32_t)cb_res->len);

    /* unmap pcie dma addr */
    dma_unmap_single(&pci_dev->dev, (dma_addr_t)cb_res->paddr.addr, cb_res->len, PCI_DMA_FROMDEVICE);

    /* DDR pending 规避方案关闭 */
    ret2 = oal_pcie_rx_databuf_verify(pci_dev, netbuf);
    if (oal_likely((ret1 == OAL_SUCC) && (ret2 == OAL_SUCC))) {
        hcc_rx_submit(hcc, netbuf);
    } else {
        oal_netbuf_free(netbuf);
        declare_dft_trace_key_info("pcie_rx_netbuf_loss", OAL_DFT_TRACE_FAIL);
    }

    return;

release_netbuf:
    oal_pcie_release_rx_netbuf(pci_lres, netbuf);
    declare_dft_trace_key_info("pcie release rx netbuf", OAL_DFT_TRACE_OTHER);
    return;
}

OAL_STATIC int32_t oal_pcie_unmap_tx_netbuf(oal_pcie_linux_res *pst_pci_res, oal_netbuf_stru *pst_netbuf)
{
    /* dma_addr 存放在CB字段里 */
    pcie_cb_dma_res st_cb_dma;
    oal_pci_dev_stru *pst_pci_dev;
    int32_t ret;

    pst_pci_dev = pst_pci_res->comm_res->pcie_dev;

    /* 不是从CB的首地址开始，必须拷贝，对齐问题。 */
    ret = memcpy_s(&st_cb_dma, sizeof(pcie_cb_dma_res),
                   (uint8_t *)oal_netbuf_cb(pst_netbuf) + sizeof(struct hcc_tx_cb_stru),
                   sizeof(pcie_cb_dma_res));
    if (ret != EOK) {
        pci_print_log(PCI_LOG_ERR, "get dma addr from cb filed failed");
        return -OAL_EFAIL;
    }
#ifdef _PRE_PLAT_FEATURE_PCIE_DEBUG
    /* Debug */
    memset_s((uint8_t *)oal_netbuf_cb(pst_netbuf) + sizeof(struct hcc_tx_cb_stru),
             oal_netbuf_cb_size() - sizeof(struct hcc_tx_cb_stru), 0, sizeof(st_cb_dma));
#endif

    /* unmap pcie dma addr */
    if (oal_likely((st_cb_dma.paddr.addr != 0) && (st_cb_dma.len != 0))) {
        dma_unmap_single(&pst_pci_dev->dev, (dma_addr_t)st_cb_dma.paddr.addr, st_cb_dma.len, PCI_DMA_TODEVICE);
    } else {
        declare_dft_trace_key_info("pcie tx netbuf free fail", OAL_DFT_TRACE_FAIL);
        oal_print_hex_dump((uint8_t *)oal_netbuf_cb(pst_netbuf), oal_netbuf_cb_size(),
                           HEX_DUMP_GROUP_SIZE, "invalid cb: ");
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}

void oal_pcie_tx_netbuf_free(oal_pcie_linux_res *pci_res, oal_netbuf_stru *netbuf)
{
    oal_pcie_unmap_tx_netbuf(pci_res, netbuf);
    hcc_tx_netbuf_free(netbuf, hbus_to_hcc(pci_res->pst_bus));
}

/* ringbuf functions */
uint32_t oal_pcie_ringbuf_freecount(pcie_ringbuf *pst_ringbuf)
{
    /* 无符号，已经考虑了翻转 */
    uint32_t len = pst_ringbuf->size - (pst_ringbuf->wr - pst_ringbuf->rd);
    if (len == 0) {
        return 0;
    }

    if (len % pst_ringbuf->item_len) {
        pci_print_log(PCI_LOG_ERR, "oal_pcie_ringbuf_freecount, size:%u, wr:%u, rd:%u",
                      pst_ringbuf->size,
                      pst_ringbuf->wr,
                      pst_ringbuf->rd);
        return 0;
    }

    if (pst_ringbuf->item_mask) {
        /* item len 如果是2的N次幂，则移位 */
        len = len >> pst_ringbuf->item_mask;
    } else {
        len /= pst_ringbuf->item_len;
    }
    return len;
}
