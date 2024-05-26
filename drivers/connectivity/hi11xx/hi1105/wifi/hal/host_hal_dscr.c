/*
 * 版权所有 (c) 华为技术有限公司 2021-2021
 * 功能说明 : HOST HAL DSCR FUNCTION
 * 作    者 : duankaiyong
 * 创建日期 : 2021年2月23日
 */

#include "host_hal_dscr.h"

#include "pcie_linux.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HOST_HAL_DSCR_C



oal_netbuf_stru *hal_alloc_list_delete_netbuf(hal_host_device_stru *hal_dev,
    hal_host_rx_alloc_list_stru *alloc_list, dma_addr_t host_iova, uint32_t *pre_num)
{
    oal_netbuf_stru             *next_skb       = NULL;
    oal_netbuf_stru             *skb            = NULL;
    pcie_cb_dma_res             *cb_res         = NULL;
    oal_netbuf_head_stru        *list           = &alloc_list->list;
    oal_pci_dev_stru            *pcie_dev       = oal_get_wifi_pcie_dev();

    if (pcie_dev == NULL) {
        oam_error_log0(0, 0, "hal_alloc_list_delete_netbuf:pcie_dev null");
        return NULL;
    }
    oal_netbuf_search_for_each_safe(skb, next_skb, list) {
        cb_res = (pcie_cb_dma_res *)oal_netbuf_cb(skb);
        if (cb_res->paddr.addr == host_iova) {
            /* 释放IO资源 */
            dma_unmap_single(&pcie_dev->dev, cb_res->paddr.addr,
                skb->len, hal_dev->rx_nodes->dma_dir);
            __netbuf_unlink(skb, list);
            return skb;
        }
        /* pre_num计数到达一定门限后，添加自愈机制，TO BE DONE */
        (*pre_num)++;
    }
    return NULL;
}
