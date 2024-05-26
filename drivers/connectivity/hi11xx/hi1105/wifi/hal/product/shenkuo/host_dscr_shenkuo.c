
#if (_PRE_OS_VERSION == _PRE_OS_VERSION_LINUX)
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/err.h>
#endif

#include "oal_util.h"
#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "oam_event_wifi.h"
#include "oal_net.h"

#include "host_hal_dscr.h"
#include "host_hal_ring.h"
#include "host_hal_device.h"

#include "host_dscr_shenkuo.h"
#include "host_mac_shenkuo.h"
#include "host_irq_shenkuo.h"
#include "pcie_host.h"
#include "mac_common.h"
#include "oal_net.h"
#include "host_hal_ops_shenkuo.h"
#if(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "plat_pm_wlan.h"
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HOST_HAL_DSCR_SHENKUO_C

static inline uint64_t make_word64(uint32_t lsw, uint32_t msw)
{
    return ((((uint64_t)(msw) << BIT_OFFSET_32) & 0xFFFFFFFF00000000) | (lsw));
}
static inline uint32_t get_low_32bits(uint64_t a)
{
    return ((uint32_t)(((uint64_t)(a)) & 0x00000000FFFFFFFFUL));
}
static inline uint32_t get_high_32bits(uint64_t a)
{
    return ((uint32_t)((((uint64_t)(a)) & 0xFFFFFFFF00000000UL) >> 32UL));
}
#define SHENKUO_MAC_RPT_HOST_INTR_NORM_RING_EMPTY_MASK              0x8
#define SHENKUO_MAC_RPT_HOST_INTR_SMALL_RING_EMPTY_MASK             0x10

static hal_ring_mac_regs_info g_ring_reg_tbl[HAL_RING_ID_BUTT] = {
    [HAL_RING_TYPE_L_F] = {
        .base_lsb = SHENKUO_MAC_RX_NORM_DATA_FREE_RING_ADDR_LSB_REG,
        .base_msb = SHENKUO_MAC_RX_NORM_DATA_FREE_RING_ADDR_MSB_REG,
        .size     = SHENKUO_MAC_RX_NORM_DATA_FREE_RING_SIZE_REG,
        .wptr_reg = SHENKUO_MAC_RX_NORM_DATA_FREE_RING_WPTR_REG,
        .rptr_reg = SHENKUO_MAC_RX_NORM_DATA_FREE_RING_RPTR_REG,
        .cfg_rptr_reg = SHENKUO_MAC_RX_NORM_DATA_FREE_RING_RPTR_CFG_REG,
    },
    [HAL_RING_TYPE_S_F] = {
        .base_lsb = SHENKUO_MAC_RX_SMALL_DATA_FREE_RING_ADDR_LSB_REG,
        .base_msb = SHENKUO_MAC_RX_SMALL_DATA_FREE_RING_ADDR_MSB_REG,
        .size     = SHENKUO_MAC_RX_SMALL_DATA_FREE_RING_SIZE_REG,
        .wptr_reg = SHENKUO_MAC_RX_SMALL_DATA_FREE_RING_WPTR_REG,
        .rptr_reg = SHENKUO_MAC_RX_SMALL_DATA_FREE_RING_RPTR_REG,
        .cfg_rptr_reg = SHENKUO_MAC_RX_SMALL_DATA_FREE_RING_RPTR_CFG_REG,
    },
    [HAL_RING_TYPE_COMP] = {
        .base_lsb = SHENKUO_MAC_RX_DATA_CMP_RING_ADDR_LSB_REG,
        .base_msb = SHENKUO_MAC_RX_DATA_CMP_RING_ADDR_MSB_REG,
        .len      = SHENKUO_MAC_RX_DATA_BUFF_LEN_REG,
        .size     = SHENKUO_MAC_RX_DATA_CMP_RING_SIZE_REG,
        .wptr_reg = SHENKUO_MAC_RX_DATA_CMP_RING_WPTR_REG,
        .rptr_reg = SHENKUO_MAC_RX_DATA_CMP_RING_RPTR_REG,
        .cfg_wptr_reg = SHENKUO_MAC_RX_DATA_CMP_RING_WPTR_CFG_REG,
    },
    [TX_BA_INFO_RING] = {
        .base_lsb = SHENKUO_MAC_TX_BA_INFO_BUF_ADDR_LSB_REG,
        .base_msb = SHENKUO_MAC_TX_BA_INFO_BUF_ADDR_MSB_REG,
        .len      = 0,
        .size     = SHENKUO_MAC_TX_BA_INFO_BUF_DEPTH_REG,
        .wptr_reg = SHENKUO_MAC_BA_INFO_BUF_WPTR_REG,
        .rptr_reg = SHENKUO_MAC_TX_BA_INFO_RPTR_REG,
        .cfg_wptr_reg = SHENKUO_MAC_TX_BA_INFO_WPTR_REG,
    }
};

void shenkuo_rx_buf_get_statics(hal_host_device_stru *hal_device)
{
    hal_device->rx_statics.comp_ring_get++;
    return;
}

int32_t shenkuo_rx_get_node_idx(hal_rx_nodes *nodes, dma_addr_t dma_addr)
{
    uint32_t count = 0;
    uint32_t idx;
    hal_rx_node *node = NULL;

    if (nodes == NULL) {
        return -OAL_EFAIL;
    }

    node  = nodes->tbl;
    idx   = nodes->last_idx;
    do {
        idx++;
        if (idx == nodes->tbl_size) {
            idx = 0;
        }
        if ((node[idx].dma_addr == dma_addr) && (node[idx].allocated)) {
            return idx;
        }
        count++;
    } while (count < nodes->tbl_size);

    oam_warning_log0(0, 0, "hal_rx_get_node_idx: not find");
    return -OAL_EFAIL;
}


oal_netbuf_stru *shenkuo_rx_get_node(hal_rx_nodes *rx_nodes, uint32_t idx)
{
    hal_rx_node *node = NULL;
    oal_pci_dev_stru *pcie_dev = oal_get_wifi_pcie_dev();

    if (pcie_dev == NULL) {
        oam_error_log0(0, 0, "hal_rx_get_node: pcie_dev null");
        return NULL;
    }

    if (idx >= rx_nodes->tbl_size) {
        oam_error_log1(0, 0, "hal_rx_get_node: idx [%d]", idx);
        return NULL;
    }
    if (rx_nodes->tbl[idx].allocated) {
        node = &rx_nodes->tbl[idx];
        /* 释放IO资源 */
        dma_unmap_single(&pcie_dev->dev, node->dma_addr,
            node->netbuf->len, rx_nodes->dma_dir);
        node->allocated = 0;
        return node->netbuf;
    } else {
        oam_error_log1(0, 0, "rx_get_node:node not alloced! node[%d]", idx);
        return NULL;
    }
}


void shenkuo_rx_alloc_list_free(hal_host_device_stru *hal_dev, hal_host_rx_alloc_list_stru *alloc_list)
{
    oal_netbuf_head_stru        *list           = &alloc_list->list;
    oal_pci_dev_stru            *pcie_dev       = oal_get_wifi_pcie_dev();
    oal_netbuf_stru             *skb            = NULL;
    oal_netbuf_stru             *next_skb       = NULL;
    pcie_cb_dma_res             *cb_res         = NULL;
    unsigned long                flags = 0;

    if (!hal_dev->inited) {
        oam_warning_log0(0, 0, "hal_rx_alloc_list_free:rx res not inited");
        return;
    }

    if (pcie_dev == NULL) {
        oam_error_log0(0, 0, "hal_rx_alloc_list_free:pcie_dev null");
        return;
    }

    oal_spin_lock_irq_save(&alloc_list->lock, &flags);
    if (oal_netbuf_peek(list) == NULL) {
        oal_spin_unlock_irq_restore(&alloc_list->lock, &flags);
        return;
    }
    oal_netbuf_search_for_each_safe(skb, next_skb, list) {
        cb_res = (pcie_cb_dma_res *)oal_netbuf_cb(skb);
        dma_unmap_single(&pcie_dev->dev, cb_res->paddr.addr,
            skb->len, hal_dev->rx_nodes->dma_dir);
        __netbuf_unlink(skb, list);
        oal_netbuf_free(skb);
    }
    oal_spin_unlock_irq_restore(&alloc_list->lock, &flags);
}

/* 功能描述 : 释放rx ring上的netbuff，释放对应的io资源 */
void shenkuo_rx_free_res(hal_host_device_stru *hal_device)
{
    hal_rx_node *node = NULL;
    oal_pci_dev_stru *pcie_dev = oal_get_wifi_pcie_dev();
    uint32_t count = 0;
    uint32_t free_num = 0;

    if (hal_device == NULL) {
        oam_error_log0(0, 0, "hal_rx_free_res: hal_device null");
        return;
    }

    if ((pcie_dev == NULL) || (hal_device->rx_nodes == NULL)) {
        oam_error_log0(0, 0, "hal_rx_free_res: pcie_dev null or rx_nodes now inited");
        return;
    }

    node = hal_device->rx_nodes->tbl;
    do {
        if ((node[count].allocated) && (node[count].netbuf != NULL)) {
            /* 释放IO资源 */
            dma_unmap_single(&pcie_dev->dev,
                node[count].dma_addr,
                node[count].netbuf->len,
                hal_device->rx_nodes->dma_dir);

            node[count].allocated = 0;
            oal_netbuf_free(node[count].netbuf);
            free_num++;
        }
        count++;
    } while (count < hal_device->rx_nodes->tbl_size);

    hal_device->rx_nodes->last_idx = 0;
    oam_info_log1(0, 0, "hal_rx_free_res: free succ. release cnt[%d]", free_num);
    return;
}


uint32_t shenkuo_rx_alloc_node(hal_rx_nodes *rx_nodes, oal_netbuf_stru *netbuf, dma_addr_t *dma_addr)
{
    hal_rx_node *node = NULL;
    uint32_t count = 0;
    uint32_t idx;
    oal_pci_dev_stru *pcie_dev = oal_get_wifi_pcie_dev();

    if (pcie_dev == NULL) {
        oam_error_log0(0, 0, "hal_rx_alloc_node: pcie_linux_rsc null");
        return OAL_FAIL;
    }

    node = rx_nodes->tbl;
    *dma_addr = dma_map_single(&pcie_dev->dev,
        oal_netbuf_data(netbuf), oal_netbuf_len(netbuf), rx_nodes->dma_dir);

    if (dma_mapping_error(&pcie_dev->dev, *dma_addr)) {
        oam_warning_log1(0, 0, "hal_rx_alloc_node: len[%d]", oal_netbuf_len(netbuf));
        return OAL_FAIL;
    }

    idx = rx_nodes->last_idx;
    do {
        idx++;
        if (idx == rx_nodes->tbl_size) {
            idx = 0;
        }
        if (node[idx].allocated == 0) {
            node[idx].allocated = 1;
            break;
        }
        count++;
    } while (count < rx_nodes->tbl_size);

    if (count == rx_nodes->tbl_size) {
        oam_error_log0(0, 0, "hal_rx_alloc_node:alloc node fail.");
        return OAL_FAIL;
    }

    node[idx].dma_addr = *dma_addr;
    node[idx].netbuf   = netbuf;
    rx_nodes->last_idx = idx;
    return OAL_SUCC;
}

oal_netbuf_stru *shenkuo_rx_get_next_sub_msdu(hal_host_device_stru *hal_device,
    oal_netbuf_stru *netbuf)
{
    shenkuo_rx_mpdu_desc_stru    *rx_hw_dscr = NULL;
    dma_addr_t                   host_iova = 0;
    uint64_t                     next_dscr_addr;
    hal_host_rx_alloc_list_stru *alloc_list = &hal_device->host_rx_normal_alloc_list;
    unsigned long                flags = 0;
    uint32_t                     pre_num = 0;
    oal_netbuf_stru             *sub_msdu = NULL;

    shenkuo_rx_buf_get_statics(hal_device);
    rx_hw_dscr = (shenkuo_rx_mpdu_desc_stru *)oal_netbuf_data(netbuf);
    if ((!rx_hw_dscr->next_sub_msdu_addr_lsb) && (!rx_hw_dscr->next_sub_msdu_addr_msb)) {
        return NULL;
    }

    next_dscr_addr = make_word64(rx_hw_dscr->next_sub_msdu_addr_lsb, rx_hw_dscr->next_sub_msdu_addr_msb);
    if (OAL_SUCC != pcie_if_devva_to_hostca(HCC_EP_WIFI_DEV, next_dscr_addr, (uint64_t *)&host_iova)) {
        oam_warning_log0(0, OAM_SF_RX, "{hal_rx_get_next_sub_msdu:get next msdu fail.}");
        return NULL;
    }
    oal_spin_lock_irq_save(&alloc_list->lock, &flags);
    sub_msdu = hal_alloc_list_delete_netbuf(hal_device, alloc_list, host_iova, &pre_num);
    oal_spin_unlock_irq_restore(&alloc_list->lock, &flags);
    if (pre_num) {
        oam_error_log1(hal_device->device_id, OAM_SF_RX, "shenkuo_rx_get_next_sub_msdu:pre num[%d]", pre_num);
    }
    if (sub_msdu == NULL) {
        oam_error_log0(hal_device->device_id, OAM_SF_RX, "shenkuo_rx_get_next_sub_msdu:sub msdu not found");
    }
    return sub_msdu;
}

void shenkuo_host_rx_amsdu_list_build(hal_host_device_stru *hal_device, oal_netbuf_stru *netbuf)
{
    oal_netbuf_stru                     *next_netbuf = NULL;

    while (netbuf != NULL) {
        next_netbuf = shenkuo_rx_get_next_sub_msdu(hal_device, netbuf);
        oal_netbuf_next(netbuf) = next_netbuf;
        netbuf = next_netbuf;
    }
}

uint8_t shenkuo_host_get_rx_msdu_status(oal_netbuf_stru *netbuf)
{
    shenkuo_rx_mpdu_desc_stru *rx_hw_dscr = NULL;

    rx_hw_dscr = (shenkuo_rx_mpdu_desc_stru *)oal_netbuf_data(netbuf);
    return rx_hw_dscr->bit_rx_status;
}


uint8_t shenkuo_rx_is_frag_frm(mac_header_frame_control_stru *fc_ctl_hdr,
    shenkuo_rx_mpdu_desc_stru *rx_msdu_hw_dscr)
{
    return (uint8_t)((rx_msdu_hw_dscr->bit_frame_format == MAC_FRAME_TYPE_80211) &&
        (fc_ctl_hdr->bit_more_frag || rx_msdu_hw_dscr->bit_fragment_num));
}

static void shenkuo_host_rx_cb_set(hal_host_device_stru *hal_dev,
    mac_rx_ctl_stru *rx_ctl, shenkuo_rx_mpdu_desc_stru *rx_hw_dscr, uint8_t first_msdu)
{
    mac_header_frame_control_stru *fc_ctl_hdr = NULL;
    /* mpdu级别 */
    if (first_msdu) {
        rx_ctl->bit_mcast_bcast     = rx_hw_dscr->bit_mcast_bcast;
        rx_ctl->bit_amsdu_enable    = rx_hw_dscr->bit_is_amsdu;
        rx_ctl->bit_buff_nums       = 1;
        rx_ctl->us_frame_control    = rx_hw_dscr->bit_frame_control;
        rx_ctl->bit_start_seqnum    = rx_hw_dscr->bit_start_seqnum;
        rx_ctl->bit_cipher_type     = rx_hw_dscr->bit_cipher_type;
        rx_ctl->bit_release_start_sn = rx_hw_dscr->bit_release_start_seqnum;
        rx_ctl->bit_release_end_sn  = rx_hw_dscr->bit_release_end_seqnum;
        rx_ctl->bit_fragment_num    = rx_hw_dscr->bit_fragment_num;
        rx_ctl->bit_process_flag    = rx_hw_dscr->bit_process_flag;
        rx_ctl->bit_rx_user_id      = rx_hw_dscr->bit_user_id;
        rx_ctl->us_seq_num          = rx_hw_dscr->bit_sequence_num;
        rx_ctl->bit_is_first_buffer = rx_hw_dscr->bit_first_sub_msdu;
        rx_ctl->bit_rx_tid          = rx_hw_dscr->bit_tid;
        rx_ctl->rx_status           = rx_hw_dscr->bit_rx_status;
        rx_ctl->bit_is_ampdu        = rx_hw_dscr->bit_is_ampdu;
        rx_ctl->uc_msdu_in_buffer   = rx_hw_dscr->bit_sub_msdu_num;
        rx_ctl->rx_lsb_pn           = rx_hw_dscr->pn_lsb;
        rx_ctl->us_rx_msb_pn        = rx_hw_dscr->bit_pn_msb;
        rx_ctl->bit_vap_id          = rx_hw_dscr->bit_vap_id;
        rx_ctl->bit_release_valid   = rx_hw_dscr->bit_release_is_valid;
        rx_ctl->bit_band_id         = rx_hw_dscr->bit_band_id;
        rx_ctl->bit_ta_user_idx     = rx_hw_dscr->bit_user_id & 0x1f; /* 1f valid bits */
        rx_ctl->last_msdu_in_amsdu  = rx_hw_dscr->bit_sub_msdu_num == 0;
        if ((rx_hw_dscr->bit_vap_id != SHENKUO_OTHER_BSS_ID) && (rx_hw_dscr->bit_vap_id >= SHENKUO_HAL_VAP_OFFSET)) {
            rx_ctl->bit_vap_id -= SHENKUO_HAL_VAP_OFFSET;
        }
        if (rx_ctl->bit_vap_id < HAL_MAX_VAP_NUM) {
            rx_ctl->uc_mac_vap_id = hal_dev->hal_vap_sts_info[rx_ctl->bit_vap_id].mac_vap_id;
        }
        fc_ctl_hdr = (mac_header_frame_control_stru *)&rx_ctl->us_frame_control;
        if (shenkuo_rx_is_frag_frm(fc_ctl_hdr, rx_hw_dscr)) {
            rx_ctl->bit_is_fragmented = OAL_TRUE;
        }
        /* 私有传输 */
        rx_ctl->rx_priv_trans = fc_ctl_hdr->bit_more_data;
    }

    /* msdu级别 */
    rx_ctl->bit_frame_format    = rx_hw_dscr->bit_frame_format;
    rx_ctl->dst_hal_vap_id      = rx_hw_dscr->bit_dst_vap_id;
    rx_ctl->us_frame_len        = rx_hw_dscr->bit_packet_len;
    rx_ctl->bit_dst_band_id     = rx_hw_dscr->bit_dst_band_id;
}

/* 接收上报sdt rx msdu描述符 */
void shenkuo_host_rx_dump_msdu_dscr(shenkuo_rx_mpdu_desc_stru *rx_dscr)
{
    uint8_t macaddr[WLAN_MAC_ADDR_LEN] = {0};

    if (oam_ota_get_switch(OAM_OTA_SWITCH_RX_MSDU_DSCR) == OAL_SWITCH_OFF) {
        return;
    }

    if (oam_report_dscr(&macaddr[0], (uint8_t *)rx_dscr, HAL_RX_DSCR_LEN, OAM_OTA_TYPE_RX_MSDU_DSCR) != OAL_SUCC) {
        oam_info_log0(0, OAM_SF_RX, "{rx_dump_msdu_dscr::rpt rx dscr fail}");
        return;
    }

    return;
}


uint32_t shenkuo_host_rx_proc_msdu_dscr(hal_host_device_stru *hal_dev, oal_netbuf_stru *netbuf)
{
    shenkuo_rx_mpdu_desc_stru *rx_msdu_hw_dscr = NULL;

    while (netbuf != NULL) {
        rx_msdu_hw_dscr = (shenkuo_rx_mpdu_desc_stru *)oal_netbuf_data(netbuf);
        shenkuo_host_rx_dump_msdu_dscr(rx_msdu_hw_dscr);
        netbuf = oal_netbuf_next(netbuf);
    }
    return OAL_SUCC;
}


uint8_t shenkuo_host_rx_get_mac_header_len(mac_rx_ctl_stru *rx_ctl)
{
    uint8_t mac_hdr_len = MAC_80211_FRAME_LEN;
    mac_header_frame_control_stru *fc_ctl_hdr = NULL;

    fc_ctl_hdr = (mac_header_frame_control_stru *)&rx_ctl->us_frame_control;
    if (rx_ctl->bit_frame_format == MAC_FRAME_TYPE_80211) {
        if ((fc_ctl_hdr->bit_type == WLAN_DATA_BASICTYPE) || (fc_ctl_hdr->bit_type == WLAN_MANAGEMENT)) {
            /* 4-Address Header */
            if ((fc_ctl_hdr->bit_to_ds == 1) && (fc_ctl_hdr->bit_from_ds == 1)) {
                mac_hdr_len += OAL_MAC_ADDR_LEN;
            }

            /* HTC+ Header */
            mac_hdr_len += (fc_ctl_hdr->bit_order == 1) ? sizeof(uint32_t) : 0;
            /* QoS Header */
            if ((fc_ctl_hdr->bit_type) == WLAN_DATA_BASICTYPE &&
                ((fc_ctl_hdr->bit_sub_type & WLAN_QOS_DATA) == WLAN_QOS_DATA)) {
                mac_hdr_len += sizeof(uint16_t);
            }
        } else {
            /* ctrl frame */
            mac_hdr_len = MAC_80211_CTL_HEADER_LEN;
            /* 适配monitor ack rts cts等 */
            if (rx_ctl->us_frame_len < mac_hdr_len) {
                mac_hdr_len = (uint8_t)(rx_ctl->us_frame_len);
            }
        }
    } else {
        /* 以太网帧头长度 */
        mac_hdr_len = ETHER_HDR_LEN;
    }

    return mac_hdr_len;
}


OAL_STATIC uint32_t shenkuo_host_rx_adjust_netbuf_len(hal_host_device_stru *hal_dev,
    oal_netbuf_stru *netbuf, mac_rx_ctl_stru *rx_ctl, shenkuo_rx_mpdu_desc_stru *rx_hw_dscr, uint8_t first_msdu)
{
    pcie_cb_dma_res               *cb_res = NULL;

    cb_res = (pcie_cb_dma_res *)oal_netbuf_cb(netbuf);

    rx_ctl->uc_mac_header_len = shenkuo_host_rx_get_mac_header_len(rx_ctl);

    /* 1.fcs错误，后续字段信息不需要查看，本身不可信  2.fcs错误的amsdu非首帧信息本身不回填，字段不可信 */
    if ((rx_hw_dscr->bit_rx_status == HAL_RX_SUCCESS) &&
        (rx_hw_dscr->bit_packet_len == 0 || (rx_hw_dscr->bit_packet_len > (cb_res->len - HAL_RX_DSCR_LEN)))) {
        oam_error_log3(0, OAM_SF_RX, "shenkuo_host_rx_adjust_netbuf_len:dscr addr:0x%x, pkt len:%d, pcie cb len:%d",
            (uintptr_t)rx_hw_dscr, rx_hw_dscr->bit_packet_len, cb_res->len);
        oam_report_dscr(BROADCAST_MACADDR, (uint8_t *)rx_hw_dscr, HAL_RX_DSCR_LEN, OAM_OTA_TYPE_RX_MSDU_DSCR);
        return OAL_FAIL;
    }

    oal_netbuf_pull(netbuf, HAL_RX_DSCR_LEN);
    skb_trim(netbuf, rx_hw_dscr->bit_packet_len);
    return OAL_SUCC;
}


uint32_t shenkuo_rx_msdu_info_wait_valid(shenkuo_rx_mpdu_desc_stru *rx_msdu_hw_dscr, uint8_t first_msdu)
{
#define RX_MUSD_INFO_WAIT_CNT 1000
    uint16_t   wait_cnt = 0;
    /* check rx msdu if info valid */
    while (rx_msdu_hw_dscr->bit_rx_status == HAL_RX_NEW && first_msdu == OAL_TRUE) {
        wait_cnt++;
        if (wait_cnt > RX_MUSD_INFO_WAIT_CNT) {
            oam_warning_log2(0, OAM_SF_RX, "shenkuo_rx_msdu_info_wait_valid:waitcnt[%d]>waitmax[%d].",
                wait_cnt, RX_MUSD_INFO_WAIT_CNT);
            return OAL_FAIL;
        }
    }

    return OAL_SUCC;
}

uint32_t shenkuo_host_rx_get_msdu_info_dscr(hal_host_device_stru *hal_dev,
    oal_netbuf_stru *netbuf, mac_rx_ctl_stru *rx_ctl, uint8_t first_msdu)
{
    shenkuo_rx_mpdu_desc_stru      *rx_msdu_hw_dscr = NULL;
    uint32_t                       ret;

    memset_s(rx_ctl, sizeof(mac_rx_ctl_stru), 0, sizeof(mac_rx_ctl_stru));
    rx_msdu_hw_dscr = (shenkuo_rx_mpdu_desc_stru *)oal_netbuf_data(netbuf);

    ret = shenkuo_rx_msdu_info_wait_valid(rx_msdu_hw_dscr, first_msdu);
    if (ret != OAL_SUCC) {
        return ret;
    }
    shenkuo_host_rx_cb_set(hal_dev, rx_ctl, rx_msdu_hw_dscr, first_msdu);
    ret = shenkuo_host_rx_adjust_netbuf_len(hal_dev, netbuf, rx_ctl, rx_msdu_hw_dscr, first_msdu);
    if (ret != OAL_SUCC) {
        return ret;
    }
    if (rx_ctl->bit_frame_format == MAC_FRAME_TYPE_80211) {
        rx_ctl->st_expand_cb.pul_mac_hdr_start_addr = (uint32_t *)oal_netbuf_data(netbuf);
    }
    return OAL_SUCC;
}

uint32_t shenkuo_dma_map_rx_buff(hal_rx_nodes *rx_nodes, oal_netbuf_stru *netbuf, dma_addr_t *dma_addr)
{
    oal_pci_dev_stru *pcie_dev = oal_get_wifi_pcie_dev();
    if (pcie_dev == NULL) {
        oam_error_log0(0, 0, "shenkuo_dma_map_rx_buff: pcie_linux_rsc null");
        return OAL_FAIL;
    }

    *dma_addr = dma_map_single(&pcie_dev->dev,
        oal_netbuf_data(netbuf), oal_netbuf_len(netbuf), rx_nodes->dma_dir);

    if (dma_mapping_error(&pcie_dev->dev, *dma_addr)) {
        oam_warning_log1(0, 0, "shenkuo_dma_map_rx_buff: len[%d]", oal_netbuf_len(netbuf));
        return OAL_FAIL;
    }
    return OAL_SUCC;
}

OAL_STATIC OAL_INLINE oal_netbuf_stru *shenkuo_alloc_rx_netbuf(hal_host_device_stru *hal_device, uint32_t buff_size)
{
#ifdef _PRE_WLAN_FEATURE_VSP
    oal_netbuf_stru *netbuf = NULL;
    if (!oal_netbuf_list_empty(&hal_device->vsp_netbuf_recycle_list) &&
        buff_size == LARGE_NETBUF_SIZE + HAL_RX_DSCR_LEN) {
        netbuf = oal_netbuf_delist(&hal_device->vsp_netbuf_recycle_list);
        if (netbuf != NULL) {
            return netbuf;
        }
    }
#endif
    return oal_mem_netbuf_alloc(OAL_NORMAL_NETBUF, buff_size, OAL_NETBUF_PRIORITY_HIGH);
}

uint32_t shenkuo_host_rx_add_buff_alloc(hal_host_device_stru *hal_device,
    hal_host_ring_ctl_stru *ring_ctl, uint32_t buff_size, hal_host_rx_alloc_list_stru *alloc_list)
{
    uint16_t need_add = 0;
    uint16_t succ_add_num = 0;
    uint16_t loop;
    oal_netbuf_stru *netbuf = NULL;
    size_t mpdu_desc_size = sizeof(shenkuo_rx_mpdu_desc_stru);
    pcie_cb_dma_res *cb_res = NULL;
    dma_addr_t pci_dma_addr;
    uint64_t  devva = 0;
    uint32_t ret;

    ret = hal_ring_get_entry_count(ring_ctl, &need_add);
    if (ret != OAL_SUCC) {
        oam_warning_log1(0, OAM_SF_RX, "{shenkuo_host_rx_add_buff_alloc::return code %d.}", ret);
        return ret;
    }
    for (loop = 0; loop < need_add; loop++) {
        netbuf = shenkuo_alloc_rx_netbuf(hal_device, buff_size);
        if (netbuf == NULL) {
            oam_warning_log0(0, OAM_SF_RX, "shenkuo_host_rx_add_buff_alloc::skb alloc fail");
            break;
        }
        oal_netbuf_set_len(netbuf, buff_size);
        memset_s(oal_netbuf_data(netbuf), mpdu_desc_size, 0, mpdu_desc_size);
        if (OAL_SUCC != shenkuo_dma_map_rx_buff(hal_device->rx_nodes, netbuf, &pci_dma_addr)) {
            oal_netbuf_free(netbuf);
            oam_warning_log0(0, OAM_SF_RX, "shenkuo_host_rx_add_buff_alloc::alloc node fail");
            break;
        }
        cb_res = (pcie_cb_dma_res *)oal_netbuf_cb(netbuf);
        cb_res->paddr.addr = pci_dma_addr;
        cb_res->len = oal_netbuf_len(netbuf);
        if (OAL_SUCC != pcie_if_hostca_to_devva(HCC_EP_WIFI_DEV, pci_dma_addr, &devva)) {
            oal_netbuf_free(netbuf);
            break;
        }
        oal_netbuf_list_tail(&alloc_list->list, netbuf);
        hal_ring_set_entries(ring_ctl, devva);
        succ_add_num++;
    }

    return succ_add_num;
}


void shenkuo_host_rx_add_buff(hal_host_device_stru *hal_device, uint8_t q_num)
{
    uint16_t                     add_num;
    uint32_t                     buff_size;
    hal_host_ring_ctl_stru      *ring_ctl = NULL;
    hal_host_rx_alloc_list_stru *alloc_list = NULL;

    if (!oal_pcie_link_state_up(HCC_EP_WIFI_DEV)) {
        oam_warning_log0(0, OAM_SF_RX, "{shenkuo_host_rx_add_buff::pcie link down!}");
        return;
    }

    if (q_num == HAL_RX_DSCR_NORMAL_PRI_QUEUE) {
        ring_ctl = &hal_device->st_host_rx_normal_free_ring;
        buff_size = (hal_device->st_alrx.en_al_rx_flag ? ALRX_NETBUF_SIZE : LARGE_NETBUF_SIZE) + HAL_RX_DSCR_LEN;
        alloc_list = &hal_device->host_rx_normal_alloc_list;
    } else if (q_num == HAL_RX_DSCR_SMALL_QUEUE) {
        ring_ctl = &hal_device->st_host_rx_small_free_ring;
        buff_size = SMALL_NETBUF_SIZE + HAL_RX_DSCR_LEN;
        alloc_list = &hal_device->host_rx_small_alloc_list;
    } else {
        oam_warning_log1(0, 0, "invalid queue num[%d] in rx irq", q_num);
        return;
    }

    add_num = shenkuo_host_rx_add_buff_alloc(hal_device, ring_ctl, buff_size, alloc_list);
    if (add_num > 0) {
        hal_ring_set_sw2hw(ring_ctl);
    }

    return;
}

#ifdef _PRE_WLAN_FEATURE_VSP
#define MAX_BUFFER_IN_RECYCLE_LIST 20
uint32_t shenkuo_host_rx_buff_recycle(hal_host_device_stru *hal_device, oal_netbuf_head_stru *netbuf_head)
{
    oal_netbuf_stru *netbuf = NULL;
    uint32_t netbuf_total_len, headroom;
    uint32_t recycle_cnt = 0;

    if (oal_netbuf_list_len(&hal_device->vsp_netbuf_recycle_list) > MAX_BUFFER_IN_RECYCLE_LIST) {
        oal_netbuf_list_purge(netbuf_head);
        return 0;
    }

    while ((netbuf = oal_netbuf_delist_nolock(netbuf_head)) != NULL) {
        headroom = oal_netbuf_headroom(netbuf);
        netbuf_total_len = headroom + oal_netbuf_get_len(netbuf) + oal_netbuf_tailroom(netbuf);
        if (netbuf_total_len < LARGE_NETBUF_SIZE + HAL_RX_DSCR_LEN) {
            oam_warning_log1(0, 0, "{shenkuo_host_rx_buff_recycle::buffer room[%d] not sufficent}", netbuf_total_len);
            oal_netbuf_free(netbuf);
            continue;
        }
        oal_netbuf_push(netbuf, headroom);
        oal_netbuf_set_len(netbuf, LARGE_NETBUF_SIZE + HAL_RX_DSCR_LEN);
        oal_netbuf_add_to_list_tail(netbuf, &hal_device->vsp_netbuf_recycle_list);
        recycle_cnt++;
    }
    return recycle_cnt;
}
#endif


uint32_t shenkuo_host_rx_reset_smac_handler(hal_host_device_stru *hal_dev)
{
    hal_host_rx_alloc_list_stru *alloc_list = &hal_dev->host_rx_normal_alloc_list;

    shenkuo_rx_alloc_list_free(hal_dev, alloc_list);
    hal_dev->rx_q_inited = OAL_FALSE;
    oam_warning_log1(0, OAM_SF_RX, "{shenkuo_host_rx_reset_smac_handler::succ device[%d]}", hal_dev->device_id);
    return OAL_SUCC;
}

uint32_t shenkuo_rx_mpdu_que_len(hal_host_device_stru *hal_device)
{
    unsigned long lock_flag;
    uint32_t que_len;
    oal_netbuf_head_stru *netbuf_head = NULL;
    hal_rx_mpdu_que *rx_mpdu;

    rx_mpdu = &hal_device->st_rx_mpdu;
    oal_spin_lock_irq_save(&rx_mpdu->st_spin_lock, &lock_flag);

    /* 获取工作队列当前缓存的MPDU数目 */
    netbuf_head = &(rx_mpdu->ast_rx_mpdu_list[(rx_mpdu->cur_idx + 1) % RXQ_NUM]);
    que_len = oal_netbuf_list_len(netbuf_head);
    oal_spin_unlock_irq_restore(&rx_mpdu->st_spin_lock, &lock_flag);

    return que_len;
}

static uint64_t shenkuo_rx_ring_regs_addr_transfer(uint8_t device_id, uint32_t reg_addr)
{
    uint64_t host_va = 0;
    uint32_t mac_reg_offset[WLAN_DEVICE_MAX_NUM_PER_CHIP] = {0, SHENKUO_MAC_BANK_REG_OFFSET};
    if (device_id >= HAL_DEVICE_ID_BUTT) {
        oam_error_log2(0, OAM_SF_CFG, "{ring_ctl_regs_addr_transfer::device_id[%d],addr[%x], check device_id fail}",
            device_id, reg_addr);
        return 0;
    }
    if (OAL_SUCC == oal_pcie_devca_to_hostva(HCC_EP_WIFI_DEV, reg_addr + mac_reg_offset[device_id], &host_va)) {
        return host_va;
    } else {
        oam_error_log2(0, OAM_SF_CFG, "{ring_ctl_regs_addr_transfer::device_id[%d],addr[%x] transfer fail}",
            device_id, reg_addr);
        return 0;
    }
}

uint8_t shenkuo_rx_ring_reg_init(hal_host_device_stru *hal_dev, hal_host_ring_ctl_stru *rctl, uint8_t ring_id)
{
    if ((hal_dev == NULL) || (rctl == NULL) || (ring_id >= HAL_RING_ID_BUTT)) {
        oam_error_log0(0, OAM_SF_CFG, "{hal_rx_ring_reg_init::null input}");
        return OAL_FALSE;
    }

    rctl->base_lsb      = shenkuo_rx_ring_regs_addr_transfer(hal_dev->device_id, g_ring_reg_tbl[ring_id].base_lsb);
    rctl->base_msb      = shenkuo_rx_ring_regs_addr_transfer(hal_dev->device_id, g_ring_reg_tbl[ring_id].base_msb);
    rctl->ring_size_reg = shenkuo_rx_ring_regs_addr_transfer(hal_dev->device_id, g_ring_reg_tbl[ring_id].size);
    rctl->write_ptr_reg = shenkuo_rx_ring_regs_addr_transfer(hal_dev->device_id, g_ring_reg_tbl[ring_id].wptr_reg);
    rctl->read_ptr_reg  = shenkuo_rx_ring_regs_addr_transfer(hal_dev->device_id, g_ring_reg_tbl[ring_id].rptr_reg);

    if (!((rctl->base_lsb) && (rctl->base_msb) && (rctl->ring_size_reg) &&
        (rctl->write_ptr_reg) && (rctl->read_ptr_reg))) {
        oam_error_log2(0, OAM_SF_CFG, "{shenkuo_rx_ring_reg_init::cfg NULL! base_lsb[0x%x] base_msb[0x%x]}",
            rctl->base_lsb, rctl->base_msb);
        oam_error_log3(0, OAM_SF_CFG, "{shenkuo_rx_ring_reg_init::ring_size_reg[0x%x] wptr_reg[0x%x] rptr_reg[0x%x]}",
            rctl->ring_size_reg, rctl->write_ptr_reg, rctl->read_ptr_reg);
        return OAL_FALSE;
    }
    if (ring_id < HAL_RING_TYPE_COMP) {
        rctl->cfg_read_ptr_reg =
            shenkuo_rx_ring_regs_addr_transfer(hal_dev->device_id, g_ring_reg_tbl[ring_id].cfg_rptr_reg);
        return !!(rctl->cfg_read_ptr_reg);
    } else {
        if (g_ring_reg_tbl[ring_id].len) {
            rctl->len_reg  = shenkuo_rx_ring_regs_addr_transfer(hal_dev->device_id, g_ring_reg_tbl[ring_id].len);
            rctl->cfg_write_ptr_reg =
                shenkuo_rx_ring_regs_addr_transfer(hal_dev->device_id, g_ring_reg_tbl[ring_id].cfg_wptr_reg);
            return ((rctl->len_reg) && (rctl->cfg_write_ptr_reg));
        } else {
            rctl->cfg_write_ptr_reg =
                shenkuo_rx_ring_regs_addr_transfer(hal_dev->device_id, g_ring_reg_tbl[ring_id].cfg_wptr_reg);
            return OAL_TRUE;
        }
    }
}


void shenkuo_init_alloc_list(hal_host_device_stru *hal_dev, hal_host_rx_alloc_list_stru *alloc_list)
{
    oal_netbuf_head_stru    *list = &alloc_list->list;
    uint32_t                 len;

    oal_spin_lock_init(&alloc_list->lock);

    len = oal_netbuf_list_len(list);
    if ((oal_netbuf_peek(list) != NULL) || (len != 0)) {
        oam_error_log1(hal_dev->device_id, OAM_SF_RX, "shenkuo_init_alloc_list:list len[%d] not empty", len);
        shenkuo_rx_alloc_list_free(hal_dev, alloc_list);
    }
    oal_netbuf_head_init(list);
}

OAL_STATIC int32_t  shenkuo_rx_host_init_small_free_ring(hal_host_device_stru *pst_device)
{
    uint16_t entries;
    hal_host_ring_ctl_stru *rx_ring_ctl = NULL;
    uint32_t alloc_size;
    dma_addr_t rbase_dma_addr = 0;
    void *rbase_vaddr = NULL;
    uint64_t devva = 0;
    int32_t ret;
    oal_pci_dev_stru  *pcie_dev = oal_get_wifi_pcie_dev();

    if (pcie_dev == NULL) {
        return OAL_FAIL;
    }

    rx_ring_ctl = &(pst_device->st_host_rx_small_free_ring);
    memset_s(rx_ring_ctl, sizeof(hal_host_ring_ctl_stru), 0, sizeof(hal_host_ring_ctl_stru));

    entries = (pst_device->device_id == 0) ? HAL_RX_MSDU_SMALL_RING_NUM0 : HAL_RX_MSDU_SMALL_RING_NUM0;
    alloc_size = (entries * sizeof(uint64_t));
    rbase_vaddr = dma_alloc_coherent(&pcie_dev->dev, alloc_size, &rbase_dma_addr, GFP_KERNEL);
    if (rbase_vaddr == NULL) {
        oam_error_log1(0, 0, "normal_free:dma_alloc fail, size[%d]", alloc_size);
        return OAL_FAIL;
    }

    ret = pcie_if_hostca_to_devva(HCC_EP_WIFI_DEV, (uint64_t)rbase_dma_addr, &devva);
    if (ret != OAL_SUCC) {
        oam_error_log0(0, 0, "shenkuo_rx_host_init_small_free_ring alloc pcie_if_hostca_to_devva fail.");
        return OAL_FAIL;
    }

    rx_ring_ctl->entries = entries;
    rx_ring_ctl->p_entries = (uint64_t *)rbase_vaddr;
    rx_ring_ctl->entry_size = HAL_RX_ENTRY_SIZE;
    rx_ring_ctl->ring_type = HAL_RING_TYPE_FREE_RING;
    rx_ring_ctl->devva = devva;

    if (OAL_FALSE == shenkuo_rx_ring_reg_init(pst_device, rx_ring_ctl, HAL_RING_TYPE_S_F)) {
        oam_error_log0(0, 0, "shenkuo_rx_host_init_small_free_ring alloc pcie_if_hostca_to_devva fail.");
        return OAL_FAIL;
    }
    shenkuo_init_alloc_list(pst_device, &pst_device->host_rx_small_alloc_list);
    return OAL_SUCC;
}

OAL_STATIC int32_t  shenkuo_rx_host_init_normal_free_ring(hal_host_device_stru *pst_device)
{
    uint16_t entries;
    hal_host_ring_ctl_stru *rx_ring_ctl = NULL;
    uint32_t alloc_size;
    void *rbase_vaddr = NULL;
    uint64_t devva = 0;
    dma_addr_t rbase_dma_addr = 0;
    oal_pci_dev_stru *pcie_dev = oal_get_wifi_pcie_dev();

    if (pcie_dev == NULL) {
        return OAL_FAIL;
    }

    rx_ring_ctl = &(pst_device->st_host_rx_normal_free_ring);
    memset_s(rx_ring_ctl, sizeof(hal_host_ring_ctl_stru), 0, sizeof(hal_host_ring_ctl_stru));

    entries = (pst_device->device_id == 0) ?
        HAL_RX_MSDU_NORMAL_RING_NUM1 : HAL_RX_MSDU_NORMAL_RING_NUM1;
    alloc_size = (entries * sizeof(uint64_t));

    rbase_vaddr = dma_alloc_coherent(&pcie_dev->dev, alloc_size, &rbase_dma_addr, GFP_KERNEL);
    if (rbase_vaddr == NULL) {
        oam_error_log0(0, 0, "hal_rx_host_init_normal_free_ring alloc pcie_dev  null.");
        return OAL_FAIL;
    }

    if (OAL_SUCC != pcie_if_hostca_to_devva(HCC_EP_WIFI_DEV, (uint64_t)rbase_dma_addr, &devva)) {
        oam_error_log0(0, 0, "hal_rx_host_init_normal_free_ring alloc pcie_if_hostca_to_devva fail.");
        return OAL_FAIL;
    }

    rx_ring_ctl->entries    = entries;
    rx_ring_ctl->p_entries  = (uint64_t *)rbase_vaddr;
    rx_ring_ctl->entry_size = HAL_RX_ENTRY_SIZE;
    rx_ring_ctl->ring_type  = HAL_RING_TYPE_FREE_RING;
    rx_ring_ctl->devva = devva;

    if (OAL_FALSE == shenkuo_rx_ring_reg_init(pst_device, rx_ring_ctl, HAL_RING_TYPE_L_F)) {
        oam_error_log0(0, 0, "hal_rx_host_init_normal_free_ring alloc pcie_if_hostca_to_devva fail.");
        return OAL_FAIL;
    }
    shenkuo_init_alloc_list(pst_device, &pst_device->host_rx_normal_alloc_list);
    oam_warning_log2(0, 0, "hal_rx_host_init_normal_free_ring :base[%x].size[%d]", devva, entries);
    return OAL_SUCC;
}

OAL_STATIC int32_t  shenkuo_rx_host_init_complete_ring(hal_host_device_stru *pst_device)
{
    uint16_t entries;
    hal_host_ring_ctl_stru *rx_ring_ctl = NULL;
    uint32_t alloc_size;
    dma_addr_t rbase_dma_addr = 0;
    void *rbase_vaddr = NULL;
    uint64_t devva = 0;
    oal_pci_dev_stru *pcie_dev = oal_get_wifi_pcie_dev();

    if (pcie_dev == NULL) {
        return OAL_FAIL;
    }

    oal_spin_lock_init(&pst_device->st_free_ring_lock);
    rx_ring_ctl = &(pst_device->st_host_rx_complete_ring);
    memset_s(rx_ring_ctl, sizeof(hal_host_ring_ctl_stru), 0, sizeof(hal_host_ring_ctl_stru));

    entries = HAL_RX_COMPLETE_RING_MAX;
    alloc_size = (entries * sizeof(uint64_t));
    rbase_vaddr = dma_alloc_coherent(&pcie_dev->dev, alloc_size, &rbase_dma_addr, GFP_KERNEL);
    if (rbase_vaddr == NULL) {
        return OAL_FAIL;
    }

    if (OAL_SUCC != pcie_if_hostca_to_devva(HCC_EP_WIFI_DEV, (uint64_t)rbase_dma_addr, &devva)) {
        return OAL_FAIL;
    }

    rx_ring_ctl->entries = entries;
    rx_ring_ctl->p_entries = (uint64_t *)rbase_vaddr;
    rx_ring_ctl->entry_size = HAL_RX_ENTRY_SIZE;
    rx_ring_ctl->ring_type = HAL_RING_TYPE_COMPLETE_RING;
    rx_ring_ctl->devva = devva;

    if (OAL_FALSE == shenkuo_rx_ring_reg_init(pst_device, rx_ring_ctl, HAL_RING_TYPE_COMP)) {
        oam_error_log0(0, 0, "hal_rx_host_init_complete_ring alloc pcie_if_hostca_to_devva  fail.");
        return OAL_FAIL;
    }

    return OAL_SUCC;
}

static void shenkuo_init_host_ba_ring_ctl(hal_host_device_stru *hal_device)
{
    hal_host_ring_ctl_stru *rx_ring_ctl = NULL;
    uint32_t alloc_size;
    dma_addr_t rbase_dma_addr = 0;
    oal_uint *rbase_vaddr = NULL;
    uint64_t devva = 0;
    oal_pci_dev_stru *pcie_dev = oal_get_wifi_pcie_dev();

    if (pcie_dev == NULL) {
        oam_error_log0(0, 0, "shenkuo_init_host_ba_ring_ctl::pcie_dev null!");
        return;
    }

    rx_ring_ctl = &hal_device->host_ba_info_ring;
    memset_s(rx_ring_ctl, sizeof(hal_host_ring_ctl_stru), 0, sizeof(hal_host_ring_ctl_stru));

    alloc_size = (HAL_BA_INFO_COUNT * BA_INFO_DATA_SIZE);
    rbase_vaddr = dma_alloc_coherent(&pcie_dev->dev, alloc_size, &rbase_dma_addr, GFP_KERNEL);
    if (rbase_vaddr == NULL) {
        oam_error_log0(0, 0, "shenkuo_init_host_ba_ring_ctl::rbase_vaddr null!");
        return;
    }

    if (OAL_SUCC != pcie_if_hostca_to_devva(HCC_EP_WIFI_DEV, (uint64_t)rbase_dma_addr, &devva)) {
        oam_error_log0(0, 0, "shenkuo_init_host_ba_ring_ctl::hostca_to_devva fail!");
        return;
    }

    rx_ring_ctl->entries = HAL_BA_INFO_COUNT;
    rx_ring_ctl->p_entries = (uint64_t *)rbase_vaddr;
    rx_ring_ctl->entry_size = BA_INFO_DATA_SIZE;
    rx_ring_ctl->ring_type = HAL_RING_TYPE_COMPLETE_RING;
    rx_ring_ctl->devva = devva;

    if (shenkuo_rx_ring_reg_init(hal_device, rx_ring_ctl, TX_BA_INFO_RING) == OAL_FALSE) {
        oam_error_log0(0, 0, "shenkuo_init_host_ba_ring_ctl ring init fail.");
    }

    oam_warning_log3(0, 0, "{shenkuo_init_host_ba_ring_ctl::dev id[%d] devva lsb[0x%x], msb[0x%x]}",
        hal_device->device_id, get_low_32bits(rx_ring_ctl->devva), get_high_32bits(rx_ring_ctl->devva));
}

int32_t shenkuo_rx_host_init_dscr_queue(uint8_t hal_dev_id)
{
    hal_host_device_stru *hal_device = hal_get_host_device(hal_dev_id);

    if ((hal_device == NULL) || (hal_dev_id >= WLAN_DEVICE_MAX_NUM_PER_CHIP)) {
        oam_error_log1(0, 0, "hal_rx_host_init_dscr_queue::hal dev null. id[%d].", hal_dev_id);
        return OAL_FAIL;
    }

    if (hal_device->inited) {
        oam_warning_log0(0, 0, "hal_rx_host_init_dscr_queue::already inited.");
        return OAL_SUCC;
    }

    /* host访问device寄存器的地址映射操作 */
    shenkuo_host_regs_addr_init(hal_device);

    /* 初始化接收硬件队列 */
    if (OAL_SUCC != shenkuo_rx_host_init_normal_free_ring(hal_device)) {
        oam_error_log0(0, 0, "hal_rx_host_init_dscr_queue::init_normal_free_ring fail.");
        return OAL_FAIL;
    }

    if (OAL_SUCC != shenkuo_rx_host_init_small_free_ring(hal_device)) {
        oam_error_log0(0, 0, "hal_rx_host_init_dscr_queue::init_small_free_ring fail.");
        return OAL_FAIL;
    }

    if (OAL_SUCC != shenkuo_rx_host_init_complete_ring(hal_device)) {
        oam_error_log0(0, 0, "hal_rx_host_init_dscr_queue::init_complete_ring fail.");
        return OAL_FAIL;
    }
    hal_device->inited = OAL_TRUE;
    oam_warning_log0(0, 0, "hal_rx_host_init_dscr_queue::inited succ.");

    return OAL_SUCC;
}

OAL_STATIC void shenkuo_rx_host_destory_free_ring(hal_host_ring_ctl_stru *rx_ring_ctl)
{
}

OAL_STATIC void shenkuo_rx_host_destory_comp_ring(hal_host_ring_ctl_stru *rx_ring_ctl)
{
}

void shenkuo_rx_set_ring_regs(hal_host_ring_ctl_stru *rx_ring_ctl)
{
    if (rx_ring_ctl->devva == 0) {
        oam_error_log2(0, 0, "shenkuo_rx_set_ring_regs::ring_type %d, subtype %d!",
            rx_ring_ctl->ring_type, (uintptr_t)rx_ring_ctl->ring_subtype);
        return;
    }

    oal_writel(get_low_32bits(rx_ring_ctl->devva), (uintptr_t)rx_ring_ctl->base_lsb);
    oal_writel(get_high_32bits(rx_ring_ctl->devva), (uintptr_t)rx_ring_ctl->base_msb);
    oal_writel(rx_ring_ctl->entries, (uintptr_t)rx_ring_ctl->ring_size_reg);

    rx_ring_ctl->un_read_ptr.read_ptr = 0;
    rx_ring_ctl->un_write_ptr.write_ptr = 0;
}

uint32_t shenkuo_rx_host_start_dscr_queue(uint8_t hal_dev_id)
{
    hal_host_device_stru *hal_device  = hal_get_host_device(hal_dev_id);
    if (hal_device == NULL) {
        oam_error_log0(0, 0, "{shenkuo_rx_host_start_dscr_queue::hal_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (hal_device->rx_q_inited) {
        return OAL_SUCC;
    }

    wlan_pm_wakeup_dev_lock();

    /* 队列的初始化可以放到hal device init函数内做一次 */
    if (OAL_SUCC != shenkuo_rx_host_init_dscr_queue(hal_dev_id)) {
        return OAL_FAIL;
    }

    /* LEN REG keep default */
    shenkuo_rx_set_ring_regs(&(hal_device->st_host_rx_normal_free_ring));
    shenkuo_rx_set_ring_regs(&(hal_device->st_host_rx_small_free_ring));
    shenkuo_rx_set_ring_regs(&(hal_device->st_host_rx_complete_ring));

    if (hal_device->st_alrx.en_al_rx_flag) {
        if (!hal_device->st_host_rx_complete_ring.len_reg) {
            oam_error_log0(0, 0, "rx_host_start_dscr_queue::len_reg not inited!");
            return OAL_FAIL;
        }
        oal_writel(((ALRX_NETBUF_SIZE + HAL_RX_DSCR_LEN) | (HAL_RX_DSCR_LEN << NUM_16_BITS)),
            (uintptr_t)hal_device->st_host_rx_complete_ring.len_reg);
    }

    /* 向数据帧normal free ring添加内存 */
    shenkuo_host_rx_add_buff(hal_device, HAL_RX_DSCR_NORMAL_PRI_QUEUE);
    hal_device->rx_q_inited = OAL_TRUE;
    hal_device->dscr_prev = 0;
    oam_warning_log1(0, OAM_SF_RX, "shenkuo_rx_host_start_dscr_queue::dev[%d]succ!", hal_dev_id);
    return OAL_SUCC;
}

int32_t shenkuo_rx_host_stop_dscr_queue(uint8_t hal_dev_id)
{
    hal_host_device_stru   *hal_device  = hal_get_host_device(hal_dev_id);

    if (hal_device == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 不用host去reset ring cfg regs，避免处理不必要的同步问题 */
    shenkuo_rx_host_destory_free_ring(&hal_device->st_host_rx_small_free_ring);
    shenkuo_rx_host_destory_free_ring(&hal_device->st_host_rx_normal_free_ring);
    shenkuo_rx_host_destory_comp_ring(&hal_device->st_host_rx_complete_ring);

    return OAL_SUCC;
}

static inline void shenkuo_host_reset_ba_info_ring_rwptr(hal_host_ring_ctl_stru *ba_ring_ctl)
{
    oal_writel(0, (uintptr_t)ba_ring_ctl->read_ptr_reg);
    oal_writel(0, (uintptr_t)ba_ring_ctl->cfg_write_ptr_reg);
    ba_ring_ctl->un_read_ptr.read_ptr = 0;
    ba_ring_ctl->un_write_ptr.write_ptr = 0;
}

void shenkuo_host_intr_regs_init(uint8_t hal_dev_id)
{
    hal_host_device_stru *hal_device = hal_get_host_device(hal_dev_id);

    if (hal_device == NULL) {
        oam_error_log0(0, 0, "{shenkuo_host_ring_tx_init::haldev null!}");
        return;
    }

    oam_warning_log2(0, 0, "shenkuo_host_intr_regs_init::dev_id[%d], initr_inited state[%d]!",
                     hal_device->device_id, hal_device->intr_inited);
    if (hal_device->intr_inited) {
        return;
    }

    shenkuo_host_regs_addr_init(hal_device);
    hal_device->intr_inited = OAL_TRUE;
}

void shenkuo_host_ba_ring_regs_init(uint8_t hal_dev_id)
{
    hal_host_device_stru *hal_device = hal_get_host_device(hal_dev_id);
    if (hal_device == NULL) {
        oam_error_log1(0, 0, "{shenkuo_host_ba_ring_regs_init::hal device[%d] null!}", hal_dev_id);
        return;
    }

    if (hal_device->host_ba_ring_regs_inited) {
        return;
    }

    shenkuo_init_host_ba_ring_ctl(hal_device);
    hal_device->host_ba_ring_regs_inited = OAL_TRUE;
}

void shenkuo_host_ring_tx_init(uint8_t hal_dev_id)
{
    hal_host_device_stru *hal_device = hal_get_host_device(hal_dev_id);

    if (hal_device == NULL) {
        oam_error_log1(0, 0, "{shenkuo_host_ring_tx_init::hal device[%d] null!}", hal_dev_id);
        return;
    }

    if (!hal_device->host_ba_ring_regs_inited) {
        oam_error_log1(0, 0, "{shenkuo_host_ring_tx_init::hal device[%d] host ba ring regs not inited!}", hal_dev_id);
        return;
    }

    if (oal_atomic_read(&hal_device->ba_ring_type) == HOST_BA_INFO_RING) {
        return;
    }

    shenkuo_host_reset_ba_info_ring_rwptr(&hal_device->host_ba_info_ring);
    shenkuo_rx_set_ring_regs(&hal_device->host_ba_info_ring);
    oal_atomic_set(&hal_device->ba_ring_type, HOST_BA_INFO_RING);

    oam_warning_log1(0, 0, "{shenkuo_host_ring_tx_init::hal device[%d] host ba ring inited}", hal_dev_id);
}

void shenkuo_tx_ba_info_dscr_get(uint8_t *ba_info_data, hal_tx_ba_info_stru *tx_ba_info)
{
    hal_tx_ba_info_dscr_stru *tx_ba_info_dscr = (hal_tx_ba_info_dscr_stru *)ba_info_data;

    tx_ba_info->user_id = tx_ba_info_dscr->bit_user_id;
    tx_ba_info->tid_no = tx_ba_info_dscr->bit_tid;
    tx_ba_info->ba_info_vld = tx_ba_info_dscr->bit_ba_info_vld;
    tx_ba_info->ba_ssn = tx_ba_info_dscr->bit_tx_ba_ssn;
    tx_ba_info->rptr_valid = OAL_FALSE;
}

void shenkuo_tx_msdu_dscr_info_get(oal_netbuf_stru *netbuf, hal_tx_msdu_dscr_info_stru *tx_msdu_info)
{
    shenkuo_tx_msdu_dscr_stru *tx_msdu_dscr = (shenkuo_tx_msdu_dscr_stru *)((uintptr_t)oal_netbuf_data(netbuf) -
                                              HAL_TX_MSDU_DSCR_LEN);

    tx_msdu_info->seq_num = tx_msdu_dscr->seq_num;
    tx_msdu_info->sn_vld = tx_msdu_dscr->sn_vld;
    tx_msdu_info->tx_count = tx_msdu_dscr->tx_count;
}

uint8_t shenkuo_host_get_tx_tid_ring_size(void)
{
    return HAL_HOST_USER_TID_TX_RING_SIZE;
}

uint32_t shenkuo_host_get_tx_tid_ring_depth(uint8_t size)
{
    return ((128) << (size));
}

void shenkuo_vsp_msdu_dscr_info_get(uint8_t *buffer, hal_tx_msdu_dscr_info_stru *tx_msdu_info)
{
    shenkuo_tx_msdu_dscr_stru *tx_msdu_dscr = (shenkuo_tx_msdu_dscr_stru *)buffer;

    tx_msdu_info->seq_num = tx_msdu_dscr->seq_num;
    tx_msdu_info->sn_vld = tx_msdu_dscr->sn_vld;
    tx_msdu_info->tx_count = tx_msdu_dscr->tx_count;
}