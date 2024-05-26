

/* 1 其他头文件包含 */
#include "pcie_linux.h"
#include "plat_pm_wlan.h"
#include "hmac_tx_msdu_dscr.h"
#include "hmac_tid_sche.h"
#include "hmac_tx_switch.h"
#include "hmac_tx_sche_switch_fsm.h"
#include "hmac_config.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DRIVER_HMAC_TX_MSDU_DSCR_C

#define oal_make_word64(lsw, msw) ((((uint64_t)(msw) << 32) & 0xFFFFFFFF00000000) | (lsw))

/* 4 函数实现 */

void hmac_tx_msdu_ring_inc_write_ptr(hmac_msdu_info_ring_stru *tx_ring)
{
    un_rw_ptr write_ptr = { .rw_ptr = tx_ring->base_ring_info.write_index };

    hmac_tx_rw_ptr_inc(&write_ptr, hal_host_tx_tid_ring_depth_get(tx_ring->base_ring_info.size));

    tx_ring->base_ring_info.write_index = write_ptr.rw_ptr;
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
static uint32_t hmac_tx_dma_map_phyaddr(uint8_t *host_addr, uint32_t len, dma_addr_t *physaddr)
{
    oal_pci_dev_stru *pcie_dev = oal_get_wifi_pcie_dev();
    if (pcie_dev == NULL) {
        oam_error_log0(0, 0, "{hmac_tx_dma_map_phyaddr::pcie_linux_res NULL}");
        return OAL_FAIL;
    }

    *physaddr = dma_map_single(&pcie_dev->dev, host_addr, len, DMA_TO_DEVICE);
    if (dma_mapping_error(&pcie_dev->dev, *physaddr)) {
        oam_error_log0(0, 0, "{hmac_tx_dma_map_phyaddr::addr map failed}");
        return OAL_FAIL;
    }

    return OAL_SUCC;
}

static uint32_t hmac_tx_dma_unmap_addr(uint8_t *dma_addr, uint32_t len)
{
    oal_pci_dev_stru *pcie_dev = oal_get_wifi_pcie_dev();
    if (pcie_dev == NULL) {
        oam_error_log0(0, OAM_SF_TX, "{hmac_tx_dma_unmap_addr::pcie_dev NULL}");
        return OAL_FAIL;
    }

    dma_unmap_single(&pcie_dev->dev, (dma_addr_t)dma_addr, len, DMA_TO_DEVICE);

    return OAL_SUCC;
}
#else
static uint32_t hmac_tx_dma_map_phyaddr(uint8_t *host_addr, uint32_t len, dma_addr_t *physaddr)
{
    *physaddr = (dma_addr_t)host_addr;

    return OAL_SUCC;
}

static uint32_t hmac_tx_dma_unmap_addr(uint8_t *dma_addr, uint32_t len)
{
    return OAL_SUCC;
}
#endif


uint32_t hmac_tx_hostva_to_devva(uint8_t *hostva, uint32_t alloc_size, uint64_t *devva)
{
    dma_addr_t pcie_dma_addr;

    if (hmac_tx_dma_map_phyaddr(hostva, alloc_size, &pcie_dma_addr) != OAL_SUCC) {
        return OAL_FAIL;
    }

    return pcie_if_hostca_to_devva(HCC_EP_WIFI_DEV, pcie_dma_addr, devva);
}


OAL_STATIC uint32_t hmac_tx_ring_set_update_info(
    hmac_tid_info_stru *tx_tid_info, tid_update_stru *tid_update_info, tid_cmd_enum_uint8 cmd)
{
    uint64_t devva = 0;
    uint8_t msdu_ring_size = sizeof(msdu_info_ring_stru);
    hmac_msdu_info_ring_stru *tx_ring = &tx_tid_info->tx_ring;

    tid_update_info->cmd = cmd;
    tid_update_info->user_id = tx_tid_info->user_index;
    tid_update_info->tid_no = tx_tid_info->tid_no;
    tid_update_info->update_wptr = !hmac_host_update_wptr_enabled();
    memcpy_s(&tid_update_info->msdu_ring, msdu_ring_size, &tx_ring->base_ring_info, msdu_ring_size);

    if (cmd != TID_CMDID_CREATE) {
        return OAL_SUCC;
    }

    if (pcie_if_hostca_to_devva(HCC_EP_WIFI_DEV, tx_ring->host_ring_dma_addr, &devva) != OAL_SUCC) {
        oam_error_log0(0, OAM_SF_TX, "{hmac_tx_ring_set_update_info::hostca to devva failed}");
        return OAL_FAIL;
    }

    tid_update_info->msdu_ring.base_addr_lsb = get_low_32_bits(devva);
    tid_update_info->msdu_ring.base_addr_msb = get_high_32_bits(devva);

    return OAL_SUCC;
}


OAL_STATIC uint32_t hmac_tx_sync_alloc_schedule_event(mac_vap_stru *mac_vap,
    hmac_to_dmac_syn_type_enum_uint8 syn_type, hmac_to_dmac_cfg_msg_stru **syn_msg,
    frw_event_mem_stru **event_mem, uint16_t us_len)
{
    frw_event_mem_stru *alloc_event_mem = NULL;
    frw_event_stru *alloc_event = NULL;

    if (mac_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    alloc_event_mem = frw_event_alloc_m(us_len + sizeof(hmac_to_dmac_cfg_msg_stru) -
        (sizeof(wlan_cfgid_enum_uint16) + sizeof(uint16_t)));
    if (oal_unlikely(alloc_event_mem == NULL)) {
        oam_error_log1(mac_vap->uc_vap_id, OAM_SF_CFG,
                       "{hmac_config_alloc_event::pst_event_mem null, us_len = %d }", us_len);
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    alloc_event = frw_get_event_stru(alloc_event_mem);

    /* 填充事件头 */
    frw_event_hdr_init(&(alloc_event->st_event_hdr), FRW_EVENT_TYPE_HOST_DRX, syn_type,
                       (us_len + sizeof(hmac_to_dmac_cfg_msg_stru) -
                       (sizeof(wlan_cfgid_enum_uint16) + sizeof(uint16_t))),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       mac_vap->uc_chip_id,
                       mac_vap->uc_device_id,
                       mac_vap->uc_vap_id);

    /* 出参赋值 */
    *event_mem = alloc_event_mem;
    *syn_msg = (hmac_to_dmac_cfg_msg_stru *)alloc_event->auc_event_data;

    return OAL_SUCC;
}


uint32_t hmac_tx_sync_h2d_scedule_event(mac_vap_stru *mac_vap, wlan_cfgid_enum_uint16 cfg_id, uint16_t us_len,
    uint8_t *payload)
{
    uint32_t ret;
    frw_event_mem_stru *event_mem = NULL;
    hmac_to_dmac_cfg_msg_stru *syn_msg = NULL;

    if (mac_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (wlan_chip_h2d_cmd_need_filter(cfg_id) == OAL_TRUE) {
        return OAL_SUCC;
    }

    ret = hmac_tx_sync_alloc_schedule_event(mac_vap, HMAC_TX_DMAC_SCH, &syn_msg, &event_mem, us_len);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_tx_sync_h2d_scedule_event::hmac_config_alloc_event failed[%d].}", ret);
        return ret;
    }

    HMAC_INIT_SYN_MSG_HDR(syn_msg, cfg_id, us_len);

    /* 填写配置同步消息内容 */
    if ((payload != NULL) && (us_len)) {
        if (EOK != memcpy_s(syn_msg->auc_msg_body, (uint32_t)us_len, payload, (uint32_t)us_len)) {
            oam_error_log0(0, OAM_SF_CFG, "hmac_tx_sync_h2d_scedule_event::memcpy fail!");
            frw_event_free_m(event_mem);
            return OAL_FAIL;
        }
    }

    /* 抛出事件 */
    ret = frw_event_dispatch_event(event_mem);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_tx_sync_h2d_scedule_event::frw_event_dispatch_event failed[%d].}", ret);
        frw_event_free_m(event_mem);
        return ret;
    }

    frw_event_free_m(event_mem);

    return OAL_SUCC;
}


uint32_t hmac_tx_sync_ring_info(hmac_tid_info_stru *tx_tid_info, tid_cmd_enum_uint8 cmd)
{
    tid_update_stru tid_update_info = { 0 };

    if (hmac_tx_ring_set_update_info(tx_tid_info, &tid_update_info, cmd) != OAL_SUCC) {
        return OAL_FAIL;
    }

    return hmac_tx_sync_h2d_scedule_event(mac_res_get_mac_vap(0), WLAN_CFGID_TX_TID_UPDATE,
                                          sizeof(tid_update_stru), (uint8_t *)&tid_update_info);
}


OAL_STATIC void hmac_tx_reg_write_base_addr(hmac_tx_ring_device_info_stru *tx_ring_device_info,
    uint64_t devva)
{
    oal_writel(get_low_32_bits(devva), (uintptr_t)tx_ring_device_info->word_addr[BYTE_OFFSET_0]);
    oal_writel(get_high_32_bits(devva), (uintptr_t)tx_ring_device_info->word_addr[BYTE_OFFSET_1]);
}


OAL_STATIC OAL_INLINE void hmac_tx_reg_write_wptr(hmac_tx_ring_device_info_stru *tx_ring_device_info,
    uint16_t wptr)
{
    tx_ring_device_info->msdu_info_word2.word2_bit.write_ptr = wptr;
    oal_writel(tx_ring_device_info->msdu_info_word2.word2, (uintptr_t)tx_ring_device_info->word_addr[BYTE_OFFSET_2]);
}


OAL_STATIC OAL_INLINE void hmac_tx_reg_write_rptr(hmac_tx_ring_device_info_stru *tx_ring_device_info, uint16_t rptr)
{
    tx_ring_device_info->msdu_info_word3.word3_bit.read_ptr = rptr;

    oal_writel(tx_ring_device_info->msdu_info_word3.word3, (uintptr_t)tx_ring_device_info->word_addr[BYTE_OFFSET_3]);
}


OAL_STATIC OAL_INLINE void hmac_tx_reg_write_ring_depth(
    hmac_tx_ring_device_info_stru *tx_ring_device_info, uint16_t ring_depth)
{
    tx_ring_device_info->msdu_info_word2.word2_bit.tx_msdu_ring_depth = ring_depth;

    oal_writel(tx_ring_device_info->msdu_info_word2.word2, (uintptr_t)tx_ring_device_info->word_addr[BYTE_OFFSET_2]);
}


void hmac_tx_reg_write_max_amsdu_num(hmac_tx_ring_device_info_stru *tx_ring_device_info, uint16_t max_amsdu_num)
{
    tx_ring_device_info->msdu_info_word2.word2_bit.max_aggr_amsdu_num = max_amsdu_num;

    oal_writel(tx_ring_device_info->msdu_info_word2.word2, (uintptr_t)tx_ring_device_info->word_addr[BYTE_OFFSET_2]);
}


OAL_STATIC void hmac_reg_write_create_ring_info(
    hmac_tx_ring_device_info_stru *tx_ring_device_info, hmac_msdu_info_ring_stru *tx_ring)
{
    uint64_t devva = 0;

    if (pcie_if_hostca_to_devva(HCC_EP_WIFI_DEV, tx_ring->host_ring_dma_addr, &devva) != OAL_SUCC) {
        oam_error_log0(0, OAM_SF_TX, "{hmac_tx_ring_set_update_info::hostca to devva failed}");
        return;
    }

    hmac_tx_reg_write_base_addr(tx_ring_device_info, devva);
    hmac_tx_reg_write_ring_depth(tx_ring_device_info, tx_ring->base_ring_info.size);
    hmac_tx_reg_write_max_amsdu_num(tx_ring_device_info, tx_ring->base_ring_info.max_amsdu_num);
    hmac_tx_reg_write_rptr(tx_ring_device_info, tx_ring->base_ring_info.read_index);
    hmac_tx_reg_write_wptr(tx_ring_device_info, tx_ring->base_ring_info.write_index);
}


OAL_STATIC void hmac_reg_write_del_ring_info(hmac_tx_ring_device_info_stru *tx_ring_device_info)
{
    hmac_tx_reg_write_base_addr(tx_ring_device_info, 0);
    hmac_tx_reg_write_ring_depth(tx_ring_device_info, 0);
    hmac_tx_reg_write_max_amsdu_num(tx_ring_device_info, 0);
    hmac_tx_reg_write_rptr(tx_ring_device_info, 0);
    hmac_tx_reg_write_wptr(tx_ring_device_info, 0);
}


OAL_STATIC OAL_INLINE void hmac_reg_write_enque_ring_info(
    hmac_tx_ring_device_info_stru *tx_ring_device_info, hmac_msdu_info_ring_stru *tx_ring)
{
    un_rw_ptr write_origin = { .rw_ptr = tx_ring_device_info->msdu_info_word2.word2_bit.write_ptr };
    un_rw_ptr write_new = { .rw_ptr = tx_ring->base_ring_info.write_index };

    if (hmac_tx_rw_ptr_compare(write_new, write_origin) != RING_PTR_GREATER) {
        return;
    }

    hmac_tx_reg_write_wptr(tx_ring_device_info, tx_ring->base_ring_info.write_index);
}


void hmac_tx_reg_write_ring_info(hmac_tid_info_stru *tx_tid_info, tid_cmd_enum_uint8 cmd)
{
    hmac_msdu_info_ring_stru *tx_ring = &tx_tid_info->tx_ring;
    hmac_tx_ring_device_info_stru *tx_ring_device_info = tx_tid_info->tx_ring_device_info;

    if (oal_unlikely(tx_ring_device_info == NULL)) {
        oam_error_log1(0, OAM_SF_TX, "{hmac_tx_reg_write_ring_info::user[%d] info NULL}", tx_tid_info->user_index);
        return;
    }

    if (cmd == TID_CMDID_ENQUE) {
        hmac_reg_write_enque_ring_info(tx_ring_device_info, tx_ring);
    } else if (cmd == TID_CMDID_CREATE) {
        hmac_reg_write_create_ring_info(tx_ring_device_info, tx_ring);
    } else if (cmd == TID_CMDID_DEL) {
        hmac_reg_write_del_ring_info(tx_ring_device_info);
    }
}


uint32_t hmac_tx_unmap_msdu_dma_addr(
    hmac_msdu_info_ring_stru *tx_ring, msdu_info_stru *msdu_info, uint32_t netbuf_len)
{
    uint64_t netbuf_iova = 0;
    int32_t ret;

    ret = (int32_t)pcie_if_devva_to_hostca(HCC_EP_WIFI_DEV,
        oal_make_word64(msdu_info->msdu_addr_lsb, msdu_info->msdu_addr_msb), &netbuf_iova);
    if (ret != OAL_SUCC) {
        oam_error_log1(0, OAM_SF_TX, "{hmac_tx_unmap_msdu_dma_addr::hostca to devva fail[%d]", ret);
        return (uint32_t)ret;
    }

    return hmac_tx_dma_unmap_addr((uint8_t *)(uintptr_t)netbuf_iova, netbuf_len + HAL_TX_MSDU_DSCR_LEN);
}


void hmac_tx_msdu_update_free_cnt(hmac_msdu_info_ring_stru *tx_ring)
{
    oal_atomic_dec(&tx_ring->msdu_cnt);
    oal_atomic_dec(&g_tid_schedule_list.ring_mpdu_num);
}


void hmac_tx_ring_release_netbuf(
    hmac_msdu_info_ring_stru *tx_ring, oal_netbuf_stru *netbuf, uint16_t release_index)
{
    tx_ring->netbuf_list[release_index] = NULL;
    oal_smp_mb();
    oal_netbuf_free(netbuf);

    hmac_tx_msdu_update_free_cnt(tx_ring);
}


OAL_STATIC uint32_t hmac_tx_netbuf_expand_headroom(oal_netbuf_stru *netbuf, mac_tx_ctl_stru *tx_ctl)
{
    int32_t ret;
    uint8_t mac_hdr[MAX_MAC_HEAD_LEN];

    if (oal_netbuf_headroom(netbuf) > HAL_TX_MSDU_DSCR_LEN) {
        return OAL_SUCC;
    }

    oam_warning_log1(0, OAM_SF_TX, "{hmac_tx_netbuf_expand_headroom::headroom %d not enough}",
        oal_netbuf_headroom(netbuf));

    if (memcpy_s(mac_hdr, MAX_MAC_HEAD_LEN, (uint8_t *)MAC_GET_CB_FRAME_HEADER_ADDR(tx_ctl),
        MAX_MAC_HEAD_LEN) != EOK) {
        return OAL_FAIL;
    }

    ret = oal_netbuf_expand_head(netbuf, HAL_TX_MSDU_DSCR_LEN, 0, GFP_ATOMIC);
    if (ret != OAL_SUCC) {
        oam_error_log0(0, OAM_SF_ANY, "{hmac_tx_netbuf_expand_headroom::expand headroom failed}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    if (memcpy_s(oal_netbuf_data(netbuf), MAX_MAC_HEAD_LEN, mac_hdr, MAX_MAC_HEAD_LEN) != EOK) {
        return OAL_FAIL;
    }

    MAC_GET_CB_FRAME_HEADER_ADDR(tx_ctl) = (mac_ieee80211_frame_stru *)oal_netbuf_data(netbuf);

    return OAL_SUCC;
}


OAL_STATIC void hmac_tx_set_msdu_frag_info(msdu_info_stru *msdu_info,
    mac_tx_ctl_stru *tx_ctl, oal_netbuf_stru *netbuf)
{
    mac_ieee80211_frame_stru *mac_header = MAC_GET_CB_FRAME_HEADER_ADDR(tx_ctl);

    msdu_info->more_frag = mac_header->st_frame_control.bit_more_frag;
    msdu_info->frag_num = mac_header->bit_frag_num;
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)


OAL_STATIC void hmac_tx_set_msdu_csum_info(msdu_info_stru *msdu_info, oal_netbuf_stru *netbuf)
{
    mac_ether_header_stru *ether_header = (mac_ether_header_stru *)oal_netbuf_data(netbuf);
    if (ether_header->us_ether_type == oal_host2net_short(ETHER_TYPE_IP)) {
        struct iphdr *iph = (struct iphdr *)(netbuf->head + netbuf->network_header);
        if (iph->protocol == OAL_IPPROTO_TCP) {
            msdu_info->csum_type = CSUM_TYPE_IPV4_TCP;
        } else if (iph->protocol == OAL_IPPROTO_UDP) {
            msdu_info->csum_type = CSUM_TYPE_IPV4_UDP;
        }
    } else if (ether_header->us_ether_type == oal_host2net_short(ETHER_TYPE_IPV6)) {
        struct ipv6hdr *ipv6h = (struct ipv6hdr *)(netbuf->head + netbuf->network_header);
        if (ipv6h->nexthdr == OAL_IPPROTO_TCP) {
            msdu_info->csum_type = CSUM_TYPE_IPV6_TCP;
        } else if (ipv6h->nexthdr == OAL_IPPROTO_UDP) {
            msdu_info->csum_type = CSUM_TYPE_IPV6_UDP;
        }
    }
}
#else
OAL_STATIC void hmac_tx_set_msdu_csum_info(msdu_info_stru *msdu_info, oal_netbuf_stru *netbuf)
{
}
#endif


OAL_STATIC void hmac_tx_set_msdu_info_from_netbuf(
    mac_vap_stru *mac_vap, msdu_info_stru *msdu_info, oal_netbuf_stru *netbuf, mac_tx_ctl_stru *tx_ctl, uint64_t devva)
{
#ifdef _PRE_WLAN_CHBA_MGMT
    hmac_vap_stru *hmac_vap = NULL;
#endif
    msdu_info->msdu_addr_lsb = get_low_32_bits(devva);
    msdu_info->msdu_addr_msb = get_high_32_bits(devva);
    msdu_info->msdu_len = oal_netbuf_len(netbuf);
    msdu_info->data_type = MAC_GET_CB_DATA_TYPE(tx_ctl);

    msdu_info->csum_type = 0;

    if (netbuf->ip_summed == CHECKSUM_PARTIAL) {
        hmac_tx_set_msdu_csum_info(msdu_info, netbuf);
    }

    if (MAC_GET_CB_DATA_TYPE(tx_ctl) == DATA_TYPE_80211) {
        hmac_tx_set_msdu_frag_info(msdu_info, tx_ctl, netbuf);
    }

    if (mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP && !MAC_GET_CB_IS_4ADDRESS(tx_ctl)) {
        msdu_info->from_ds = 1;
        msdu_info->to_ds = 0;
    } else if (mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA && !MAC_GET_CB_IS_4ADDRESS(tx_ctl)) {
        msdu_info->from_ds = 0;
        msdu_info->to_ds = 1;
    } else {
        msdu_info->from_ds = 1;
        msdu_info->to_ds = 1;
    }
#ifdef _PRE_WLAN_CHBA_MGMT
    hmac_vap = mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    /* 对于CHBA相关的管理帧不走状态机接口 */
    if (hmac_chba_vap_start_check(hmac_vap)) {
        msdu_info->from_ds = 0;
        msdu_info->to_ds = 0;
    }
#endif
}


uint8_t *hmac_tx_netbuf_init_msdu_dscr(oal_netbuf_stru *netbuf)
{
    uint8_t *msdu_dscr_addr = (uint8_t *)((uintptr_t)oal_netbuf_data(netbuf) - HAL_TX_MSDU_DSCR_LEN);

    memset_s(msdu_dscr_addr, HAL_TX_MSDU_DSCR_LEN, 0, HAL_TX_MSDU_DSCR_LEN);

    return msdu_dscr_addr;
}


uint32_t hmac_tx_set_msdu_info(mac_vap_stru *mac_vap, msdu_info_stru *msdu_info, oal_netbuf_stru *netbuf)
{
    uint32_t ret;
    uint64_t devva = 0;
    mac_tx_ctl_stru *tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(netbuf);

    if (hmac_tx_netbuf_expand_headroom(netbuf, tx_ctl) != OAL_SUCC) {
        return OAL_FAIL;
    }

    ret = hmac_tx_hostva_to_devva(hmac_tx_netbuf_init_msdu_dscr(netbuf), netbuf->len + HAL_TX_MSDU_DSCR_LEN, &devva);
    if (ret != OAL_SUCC) {
        oam_error_log1(0, OAM_SF_TX, "{hmac_tx_set_msdu_info::hostva to devva failed[%d]}", ret);
        return ret;
    }

    hmac_tx_set_msdu_info_from_netbuf(mac_vap, msdu_info, netbuf, tx_ctl, devva);

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE void hmac_record_netbuf_addr(hmac_msdu_info_ring_stru *tx_msdu_ring, oal_netbuf_stru *netbuf)
{
    un_rw_ptr write_ptr = { .rw_ptr = tx_msdu_ring->base_ring_info.write_index };

    tx_msdu_ring->netbuf_list[write_ptr.st_rw_ptr.bit_rw_ptr] = netbuf;
}


static uint32_t hmac_tx_set_msdu_info_to_ring(
    mac_vap_stru *mac_vap, hmac_msdu_info_ring_stru *tx_msdu_ring, oal_netbuf_stru *netbuf)
{
    un_rw_ptr write_ptr = { .rw_ptr = tx_msdu_ring->base_ring_info.write_index };
    msdu_info_stru *ring_msdu_info = NULL;

    if (oal_unlikely(hmac_tid_ring_full(tx_msdu_ring) || hal_host_tx_clear_msdu_padding(netbuf) != OAL_SUCC)) {
        return OAL_FAIL;
    }

    ring_msdu_info = hmac_tx_get_ring_msdu_info(tx_msdu_ring, write_ptr.st_rw_ptr.bit_rw_ptr);
    memset_s(ring_msdu_info, sizeof(msdu_info_stru), 0, sizeof(msdu_info_stru));
    if (oal_unlikely(hmac_tx_set_msdu_info(mac_vap, ring_msdu_info, netbuf) != OAL_SUCC)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_record_netbuf_addr(tx_msdu_ring, netbuf);
    hmac_tx_msdu_ring_inc_write_ptr(tx_msdu_ring);

    return OAL_SUCC;
}


static inline void hmac_tx_update_statistics(hmac_msdu_info_ring_stru *tx_msdu_ring)
{
    oal_atomic_inc(&tx_msdu_ring->msdu_cnt);
    oal_atomic_inc(&tx_msdu_ring->last_period_tx_msdu);
    oal_atomic_inc(&g_tid_schedule_list.ring_mpdu_num);
}


uint32_t hmac_tx_ring_push_msdu(mac_vap_stru *mac_vap, hmac_msdu_info_ring_stru *tx_ring, oal_netbuf_stru *netbuf)
{
    if (hmac_tx_set_msdu_info_to_ring(mac_vap, tx_ring, netbuf) != OAL_SUCC) {
        return OAL_FAIL;
    }

    hmac_tx_update_statistics(tx_ring);

    return OAL_SUCC;
}


uint32_t hmac_get_host_ring_state(frw_event_mem_stru *event_mem)
{
    if (!hmac_host_ring_tx_enabled()) {
        return OAL_SUCC;
    }

    oam_error_log3(0, OAM_SF_PWR,
        "hmac_get_host_ring_state::tid not empty[%d] ring mpdu[%d] ddr rx[%d]",
        hmac_is_tid_empty(), oal_atomic_read(&g_tid_schedule_list.ring_mpdu_num), hal_device_is_in_ddr_rx());

    return OAL_SUCC;
}
