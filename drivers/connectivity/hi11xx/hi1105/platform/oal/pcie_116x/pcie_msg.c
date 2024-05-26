
#include "oal_types.h"
#include "pcie_edma.h"
#include "pcie_host.h"

/* write message to ringbuf */
/* Returns, bytes we wrote to ringbuf */
OAL_STATIC int32_t oal_pcie_h2d_message_buf_write(oal_pcie_linux_res *pst_pci_res, pcie_ringbuf *pst_ringbuf,
                                                  pci_addr_map *pst_ringbuf_base, uint32_t message)
{
    uint32_t real_wr;

    if (oal_unlikely(pst_ringbuf->item_len != (uint16_t)sizeof(message))) {
        pci_print_log(PCI_LOG_ERR, "[%s]invalid item_len [%u!=%lu]\n", __FUNCTION__, pst_ringbuf->item_len,
                      (unsigned long)sizeof(pcie_read_ringbuf_item));
        return 0;
    }

    if (oal_warn_on(pst_ringbuf->wr - pst_ringbuf->rd >= pst_ringbuf->size)) {
        /* never touch here */
        pci_print_log(PCI_LOG_ERR, "message ringbuf full [wr:%u] [rd:%u] [size:%u]\n", pst_ringbuf->wr, pst_ringbuf->rd,
                      pst_ringbuf->size);
        return 0;
    }

    real_wr = pst_ringbuf->wr & (pst_ringbuf->size - 1);
    oal_pcie_io_trans(pst_ringbuf_base->va + real_wr, (uintptr_t)&message, sizeof(message));

    pst_ringbuf->wr += sizeof(message);

    return 1;
}

OAL_STATIC int32_t oal_pcie_h2d_message_buf_rd_update(oal_pcie_linux_res *pst_pci_res)
{
    /* 需要刷cache */
    /* h2d方向，同步device的读指针到HOST message ringbuf管理结构体 */
    uint32_t rd;
    pci_addr_map st_map;

    st_map.va = pst_pci_res->ep_res.st_message_res.h2d_res.ringbuf_ctrl_dma_addr.va + OAL_OFFSET_OF(pcie_ringbuf, rd);
    st_map.pa = pst_pci_res->ep_res.st_message_res.h2d_res.ringbuf_ctrl_dma_addr.pa + OAL_OFFSET_OF(pcie_ringbuf, rd);

    rd = oal_pcie_read_mem32(st_map.va);
    if (oal_unlikely(rd == 0xFFFFFFFF)) {
        if (oal_pcie_check_link_state(pst_pci_res) == OAL_FALSE) {
            pci_print_log(PCI_LOG_ERR, "h2d message ringbuf rd update: link down[va:0x%lx, pa:0x%lx]", st_map.va,
                          st_map.pa);
            return -OAL_ENODEV;
        }
    }
    pci_print_log(PCI_LOG_DBG, "h2d message ringbuf rd update:[0x%lx:rd:0x%x]", st_map.va, rd);
    pst_pci_res->ep_res.st_ringbuf.st_h2d_msg.rd = rd;

    return OAL_SUCC;
}

OAL_STATIC void oal_pcie_h2d_message_buf_wr_update(oal_pcie_linux_res *pst_pci_res)
{
    /* 需要刷cache */
    /* h2d方向，同步device的读指针到HOST message ringbuf管理结构体 */
    uint32_t wr_back;
    pci_addr_map st_map;

    st_map.va = pst_pci_res->ep_res.st_message_res.h2d_res.ringbuf_ctrl_dma_addr.va + OAL_OFFSET_OF(pcie_ringbuf, wr);
    st_map.pa = pst_pci_res->ep_res.st_message_res.h2d_res.ringbuf_ctrl_dma_addr.pa + OAL_OFFSET_OF(pcie_ringbuf, wr);

    oal_pcie_write_mem32(st_map.va, pst_pci_res->ep_res.st_ringbuf.st_h2d_msg.wr);

    wr_back = oal_pcie_read_mem32(st_map.va);
    if (wr_back != pst_pci_res->ep_res.st_ringbuf.st_h2d_msg.wr) {
        pci_print_log(PCI_LOG_ERR, "pcie h2d message wr write failed, wr_back=%u, host_wr=%u", wr_back,
                      pst_pci_res->ep_res.st_ringbuf.st_h2d_msg.wr);
        declare_dft_trace_key_info("h2d_message_wr_update_failed", OAL_DFT_TRACE_FAIL);
        (void)ssi_dump_err_regs(SSI_ERR_HCC_EXCP_PCIE);
    }
}

/* update wr pointer to host ,check the read space */
OAL_STATIC int32_t oal_pcie_d2h_message_buf_wr_update(oal_pcie_linux_res *pst_pci_res)
{
    uint32_t wr;
    pci_addr_map st_map;

    st_map.va = pst_pci_res->ep_res.st_message_res.d2h_res.ringbuf_ctrl_dma_addr.va + OAL_OFFSET_OF(pcie_ringbuf, wr);
    st_map.pa = pst_pci_res->ep_res.st_message_res.d2h_res.ringbuf_ctrl_dma_addr.pa + OAL_OFFSET_OF(pcie_ringbuf, wr);

    wr = oal_pcie_read_mem32(st_map.va);
    if (oal_unlikely(wr == 0xFFFFFFFF)) {
        if (oal_pcie_check_link_state(pst_pci_res) == OAL_FALSE) {
            pci_print_log(PCI_LOG_ERR, "d2h message ringbuf wr update: link down[va:0x%lx, pa:0x%lx]", st_map.va,
                          st_map.pa);
            return -OAL_ENODEV;
        }
    }

    pci_print_log(PCI_LOG_DBG, "d2h message ringbuf wr update:[0x%lx:wr:0x%x]", st_map.va, wr);
    pst_pci_res->ep_res.st_ringbuf.st_d2h_msg.wr = wr;
    return OAL_SUCC;
}

/* update rd pointer to device */
OAL_STATIC void oal_pcie_d2h_message_buf_rd_update(oal_pcie_linux_res *pst_pci_res)
{
    pci_addr_map st_map;

    st_map.va = pst_pci_res->ep_res.st_message_res.d2h_res.ringbuf_ctrl_dma_addr.va + OAL_OFFSET_OF(pcie_ringbuf, rd);
    st_map.pa = pst_pci_res->ep_res.st_message_res.d2h_res.ringbuf_ctrl_dma_addr.pa + OAL_OFFSET_OF(pcie_ringbuf, rd);

    oal_pcie_write_mem32(st_map.va, pst_pci_res->ep_res.st_ringbuf.st_d2h_msg.rd);
}

/* Update rd pointer, Return the bytes we read */
OAL_STATIC int32_t oal_pcie_d2h_message_buf_read(oal_pcie_linux_res *pst_pci_res, pcie_ringbuf *pst_ringbuf,
                                                 pci_addr_map *pst_ringbuf_base, uint32_t *message)
{
    uint32_t real_rd, wr, rd, data_size;

    rd = pst_ringbuf->rd;
    wr = pst_ringbuf->wr;

    data_size = wr - rd;

    if (oal_unlikely((data_size < pst_ringbuf->item_len) || (pst_ringbuf->item_len != sizeof(uint32_t)))) {
        pci_print_log(PCI_LOG_ERR, "d2h message buf read failed, date_size[%d] < item_len:%d, wr:%u, rd:%u", data_size,
                      pst_ringbuf->item_len, wr, rd);
        return 0;
    }

    real_rd = rd & (pst_ringbuf->size - 1);

    if (oal_pcie_check_link_state(pst_pci_res) == OAL_FALSE) {
        /* LinkDown */
        pci_print_log(PCI_LOG_ERR, "d2h message read detect linkdown.");
        return 0;
    }

    oal_pcie_io_trans((uintptr_t)(message), (uintptr_t)(pst_ringbuf_base->va + (unsigned long)real_rd),
                      pst_ringbuf->item_len);

    pst_ringbuf->rd += pst_ringbuf->item_len;

    /* Update device's read pointer */
    oal_pcie_d2h_message_buf_rd_update(pst_pci_res);

    return pst_ringbuf->item_len;
}

int32_t oal_pcie_ringbuf_read_rd(oal_pcie_linux_res *pst_pci_res, pcie_comm_ringbuf_type type)
{
    uint32_t rd;
    pci_addr_map st_map;
    pcie_ringbuf *pst_ringbuf = NULL;

    if (oal_unlikely(pst_pci_res->comm_res->link_state < PCI_WLAN_LINK_RES_UP)) {
        pci_print_log(PCI_LOG_WARN, "comm ringbuf %d read rd, link_state:%s", type,
                      oal_pcie_get_link_state_str(pst_pci_res->comm_res->link_state));
        return -OAL_ENODEV;
    }

    pst_ringbuf = &pst_pci_res->ep_res.st_ringbuf.st_ringbuf[type];

    st_map.va = pst_pci_res->ep_res.st_ringbuf_res.comm_rb_res[type].ctrl_daddr.va + OAL_OFFSET_OF(pcie_ringbuf, rd);
    st_map.pa = pst_pci_res->ep_res.st_ringbuf_res.comm_rb_res[type].ctrl_daddr.pa + OAL_OFFSET_OF(pcie_ringbuf, rd);

    rd = oal_pcie_read_mem32(st_map.va);
    if (oal_unlikely(rd == 0xFFFFFFFF)) {
        if (oal_pcie_check_link_state(pst_pci_res) == OAL_FALSE) {
            pci_print_log(PCI_LOG_ERR, "ringbuf %d read rd: link down[va:0x%lx, pa:0x%lx]", type, st_map.va, st_map.pa);
            return -OAL_ENODEV;
        }
    }

    pci_print_log(PCI_LOG_DBG, "ringbuf %d read rd:[0x%lx:rd:0x%x]", type, st_map.va, rd);

    pst_ringbuf->rd = rd;

    return OAL_SUCC;
}

int32_t oal_pcie_ringbuf_write(oal_pcie_linux_res *pst_pci_res, pcie_comm_ringbuf_type type, uint8_t *buf, uint32_t len,
                               int32_t ep_id)
{
    /* 不判断写指针，此函数只执行写操作 */
    oal_pci_dev_stru *pst_pci_dev;
    uint32_t real_wr;

    pcie_ringbuf *pst_ringbuf = &pst_pci_res->ep_res.st_ringbuf.st_ringbuf[type];
    pci_addr_map *pst_ringbuf_base = &pst_pci_res->ep_res.st_ringbuf_res.comm_rb_res[type].data_daddr;

    pst_pci_dev = pst_pci_res->comm_res->pcie_dev;

    /* Debug */
    if (oal_unlikely(pst_pci_res->comm_res->link_state < PCI_WLAN_LINK_RES_UP)) {
        pci_print_log(PCI_LOG_WARN, "comm ringbuf %d write failed, link_state:%s", type,
                      oal_pcie_get_link_state_str(pst_pci_res->comm_res->link_state));
        return 0;
    }

    if (oal_warn_on(len != pst_ringbuf->item_len)) {
        pci_print_log(PCI_LOG_WARN, "ringbuf %d write request len %u not equal to %u", type, len,
                      pst_ringbuf->item_len);
        return 0;
    }

    if (oal_warn_on(pst_ringbuf->wr - pst_ringbuf->rd >= pst_ringbuf->size)) {
        /* never touch here */
        pci_print_log(PCI_LOG_ERR, "ringbuf %d full [wr:%u] [rd:%u] [size:%u]\n", type, pst_ringbuf->wr,
                      pst_ringbuf->rd, pst_ringbuf->size);
        return 0;
    }

    real_wr = pst_ringbuf->wr & (pst_ringbuf->size - 1);
    oal_pcie_io_trans(pst_ringbuf_base->va + real_wr, (uintptr_t)buf, pst_ringbuf->item_len);
    if (pci_dbg_condtion()) {
        int32_t ret;
        uint64_t cpuaddr;
        ret = oal_pcie_get_ca_by_pa(pst_pci_res->comm_res, pst_ringbuf_base->pa, &cpuaddr);
        if (ret == OAL_SUCC) {
            pci_print_log(PCI_LOG_DBG, "ringbuf %d write ringbuf data cpu address:0x%llx", type, cpuaddr);
        } else {
            pci_print_log(PCI_LOG_DBG, "ringbuf %d rd pa:0x%lx invaild", type, pst_ringbuf_base->pa);
        }
        oal_print_hex_dump((uint8_t *)buf, pst_ringbuf->item_len, pst_ringbuf->item_len, "ringbuf write: ");
    }

    pst_ringbuf->wr += pst_ringbuf->item_len;

    return 1;
}

uint32_t oal_pcie_comm_ringbuf_freecount(oal_pcie_linux_res *pst_pci_res, pcie_comm_ringbuf_type type, int32_t ep_id)
{
    pcie_ringbuf *pst_ringbuf = &pst_pci_res->ep_res.st_ringbuf.st_ringbuf[type];
    return oal_pcie_ringbuf_freecount(pst_ringbuf);
}

int32_t oal_pcie_read_d2h_message(oal_pcie_linux_res *pst_pci_res, uint32_t *message)
{
    int32_t ret;
    uint32_t len;
    pcie_ringbuf *pst_ringbuf = NULL;
    if (oal_unlikely(pst_pci_res == NULL)) {
        return -OAL_EINVAL;
    }

    if (oal_unlikely(pst_pci_res->comm_res->link_state < PCI_WLAN_LINK_RES_UP)) {
        pci_print_log(PCI_LOG_INFO, "link state is disabled:%s!",
                      oal_pcie_get_link_state_str(pst_pci_res->comm_res->link_state));
        return -OAL_ENODEV;
    }

    pst_ringbuf = &pst_pci_res->ep_res.st_ringbuf.st_d2h_msg;
    pci_print_log(PCI_LOG_DBG, "oal_pcie_read_d2h_message ++");

    len = pcie_ringbuf_len(pst_ringbuf);
    if (len == 0) {
        /* No Message, update wr pointer and retry */
        ret = oal_pcie_d2h_message_buf_wr_update(pst_pci_res);
        if (ret != OAL_SUCC) {
            return ret;
        }
        len = pcie_ringbuf_len(&pst_pci_res->ep_res.st_ringbuf.st_d2h_msg);
    }

    if (len == 0) {
        return -OAL_ENODEV;
    }

    if (oal_pcie_d2h_message_buf_read(pst_pci_res, pst_ringbuf,
                                      &pst_pci_res->ep_res.st_message_res.d2h_res.ringbuf_data_dma_addr, message)) {
        pci_print_log(PCI_LOG_DBG, "oal_pcie_read_d2h_message --");
        return OAL_SUCC;
    } else {
        pci_print_log(PCI_LOG_DBG, "oal_pcie_read_d2h_message ^^");
        return -OAL_EINVAL;
    }
}

void oal_pcie_trigger_message(oal_pcie_linux_res *pst_pci_res)
{
    if (pst_pci_res->ep_res.chip_info.edma_support == OAL_TRUE) {
        /* 触发h2d int */
        oal_writel(PCIE_H2D_TRIGGER_VALUE, pst_pci_res->ep_res.pst_pci_ctrl_base + PCIE_D2H_DOORBELL_OFF);
    } else if (pst_pci_res->ep_res.chip_info.ete_support == OAL_TRUE) {
        oal_pcie_h2d_int(pst_pci_res);
    } else {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "unkown dma");
    }
}

int32_t oal_pcie_send_message_to_dev(oal_pcie_linux_res *pst_pci_res, uint32_t message)
{
    int32_t ret;
    uint32_t freecount;
    if (oal_warn_on(pst_pci_res == NULL)) {
        return -OAL_ENODEV;
    }

    if (oal_unlikely(!pst_pci_res->comm_res->regions.inited)) {
        pci_print_log(PCI_LOG_ERR, "region is disabled!");
        return -OAL_EFAUL;
    }

    if (oal_unlikely(pst_pci_res->comm_res->link_state < PCI_WLAN_LINK_RES_UP)) {
        pci_print_log(PCI_LOG_WARN, "send message link invaild, link_state:%s",
                      oal_pcie_get_link_state_str(pst_pci_res->comm_res->link_state));
        return -OAL_EBUSY;
    }

    /* message ringbuf freecount */
    oal_spin_lock(&pst_pci_res->ep_res.st_message_res.h2d_res.lock);
    freecount = oal_pcie_ringbuf_freecount(&pst_pci_res->ep_res.st_ringbuf.st_h2d_msg);
    if (freecount == 0) {
        /* no space, sync rd pointer */
        oal_pcie_h2d_message_buf_rd_update(pst_pci_res);
        freecount = oal_pcie_ringbuf_freecount(&pst_pci_res->ep_res.st_ringbuf.st_h2d_msg);
    }

    if (freecount == 0) {
        oal_spin_unlock(&pst_pci_res->ep_res.st_message_res.h2d_res.lock);
        return -OAL_EBUSY;
    }

    /* write message to ringbuf */
    ret = oal_pcie_h2d_message_buf_write(pst_pci_res, &pst_pci_res->ep_res.st_ringbuf.st_h2d_msg,
                                         &pst_pci_res->ep_res.st_message_res.h2d_res.ringbuf_data_dma_addr, message);
    if (ret <= 0) {
        oal_spin_unlock(&pst_pci_res->ep_res.st_message_res.h2d_res.lock);
        if (oal_pcie_check_link_state(pst_pci_res) == OAL_FALSE) {
            /* Should trigger DFR here */
            pci_print_log(PCI_LOG_ERR, "h2d message send failed: link down, ret=%d", ret);
        }
        return -OAL_EIO;
    }

    /* 更新写指针 */
    oal_pcie_h2d_message_buf_wr_update(pst_pci_res);

    /* 触发h2d int */
    oal_pcie_trigger_message(pst_pci_res);

    oal_spin_unlock(&pst_pci_res->ep_res.st_message_res.h2d_res.lock);

    return OAL_SUCC;
}

int32_t oal_pcie_get_host_trans_count(oal_pcie_linux_res *pst_pci_res, uint64_t *tx, uint64_t *rx)
{
    if (tx != NULL) {
        int32_t i;
        *tx = 0;
        for (i = 0; i < PCIE_H2D_QTYPE_BUTT; i++) {
            *tx += (uint64_t)pst_pci_res->ep_res.st_tx_res[i].stat.tx_count;
        }
    }
    if (rx != NULL) {
        *rx = (uint64_t)pst_pci_res->ep_res.st_rx_res.stat.rx_count;
    }
    return OAL_SUCC;
}

void oal_pcie_reset_transfer_info(oal_pcie_linux_res *pst_pci_res)
{
    int32_t i;

    if (pst_pci_res == NULL) {
        return;
    }

    pci_print_log(PCI_LOG_INFO, "reset transfer info");
    pst_pci_res->ep_res.stat.intx_total_count = 0;
    pst_pci_res->ep_res.stat.intx_tx_count = 0;
    pst_pci_res->ep_res.stat.intx_rx_count = 0;
    pst_pci_res->ep_res.stat.h2d_doorbell_cnt = 0;
    pst_pci_res->ep_res.stat.d2h_doorbell_cnt = 0;
    pst_pci_res->ep_res.st_rx_res.stat.rx_count = 0;
    pst_pci_res->ep_res.st_rx_res.stat.rx_done_count = 0;

    for (i = 0; i < PCIE_H2D_QTYPE_BUTT; i++) {
        pst_pci_res->ep_res.st_tx_res[i].stat.tx_count = 0;
        pst_pci_res->ep_res.st_tx_res[i].stat.tx_done_count = 0;
        memset_s((void *)pst_pci_res->ep_res.st_tx_res[i].stat.tx_burst_cnt,
                 sizeof(pst_pci_res->ep_res.st_tx_res[i].stat.tx_burst_cnt), 0,
                 sizeof(pst_pci_res->ep_res.st_tx_res[i].stat.tx_burst_cnt));
    }

    memset_s((void *)pst_pci_res->ep_res.st_rx_res.stat.rx_burst_cnt,
             sizeof(pst_pci_res->ep_res.st_rx_res.stat.rx_burst_cnt), 0,
             sizeof(pst_pci_res->ep_res.st_rx_res.stat.rx_burst_cnt));
}
