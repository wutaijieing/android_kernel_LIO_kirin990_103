

#ifdef _PRE_WLAN_FEATURE_SNIFFER

#include "oal_util.h"
#include "oal_ext_if.h"
#include "oam_ext_if.h"

#include "host_hal_dscr.h"
#include "host_hal_ring.h"
#include "host_hal_device.h"

#include "host_dscr_bisheng.h"
#include "host_mac_bisheng.h"
#include "host_sniffer_bisheng.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HOST_SNIFFER_BISHENG_C

/*
 * 函数描述：rx ppdu描述符 free ring 初始化
 */
uint32_t bisheng_rx_ppdu_dscr_free_ring_init(hal_host_device_stru *hal_dev)
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

    rx_ring_ctl = &(hal_dev->host_rx_ppdu_free_ring);
    memset_s(rx_ring_ctl, sizeof(hal_host_ring_ctl_stru), 0, sizeof(hal_host_ring_ctl_stru));

    entries = HAL_RX_PPDU_FREE_RING_COUNT;
    alloc_size = (entries * sizeof(uint64_t));
    /* 申请ring的内存，非netbuf的内存 */
    rbase_vaddr = dma_alloc_coherent(&pcie_dev->dev, alloc_size, &rbase_dma_addr, GFP_KERNEL);
    if (rbase_vaddr == NULL) {
        oam_error_log0(0, 0, "hal_rx_host_init_ppdu_free_ring alloc pcie_linux_res  null.");
        return OAL_FAIL;
    }

    if (OAL_SUCC != pcie_if_hostca_to_devva(HCC_EP_WIFI_DEV, (uint64_t)rbase_dma_addr, &devva)) {
        oam_error_log0(0, 0, "hal_rx_host_init_ppdu_free_ring alloc pcie_if_hostca_to_devva fail.");
        return OAL_FAIL;
    }

    rx_ring_ctl->entries    = entries;
    rx_ring_ctl->p_entries  = (uint64_t *)rbase_vaddr;
    rx_ring_ctl->entry_size = HAL_RX_ENTRY_SIZE;
    rx_ring_ctl->ring_type  = HAL_RING_TYPE_FREE_RING;
    rx_ring_ctl->devva = devva;

    if (OAL_FALSE == bisheng_rx_ring_reg_init(hal_dev, rx_ring_ctl, HAL_RING_TYPE_P_F)) {
        oam_error_log0(0, 0, "hal_rx_host_init_ppdu_free_ring alloc pcie_if_hostca_to_devva fail.");
        return OAL_FAIL;
    }

    oal_dlist_init_head(&hal_dev->host_rx_ppdu_alloc_list.list_head);
    oam_warning_log2(0, 0, "hal_rx_host_init_normal_free_ring :base[%x].size[%d]", devva, entries);
    return OAL_SUCC;
}

static uint32_t bisheng_dma_map_rx_ppdu_dscr(
    uint8_t dma_dir, hal_host_rx_ppdu_dscr_stru *ppdu_dscr, dma_addr_t *dma_addr)
{
    oal_pci_dev_stru *pcie_dev = oal_get_wifi_pcie_dev();
    if (pcie_dev == NULL) {
        oam_error_log0(0, 0, "bisheng_dma_map_rx_buff: pcie_linux_rsc null");
        return OAL_FAIL;
    }

    *dma_addr = dma_map_single(&pcie_dev->dev, (uint8_t *)&ppdu_dscr->hw_ppdu_data,
        sizeof(bisheng_rx_ppdu_desc_stru), dma_dir);

    if (dma_mapping_error(&pcie_dev->dev, *dma_addr)) {
        return OAL_FAIL;
    }

    return OAL_SUCC;
}

/*
 * 功能描述：向 free ring添加内存
 */
uint32_t bisheng_add_rx_ppdu_dscr(hal_host_device_stru *hal_device)
{
    uint64_t devva = 0;
    uint64_t pci_dma_addr;
    uint16_t need_add = 0;
    uint16_t succ_add_num = 0;
    uint16_t loop;
    hal_host_ring_ctl_stru *ring_ctl = &hal_device->host_rx_ppdu_free_ring;
    hal_host_rx_ppdu_dscr_list_head_stru *alloc_list = &hal_device->host_rx_ppdu_alloc_list;
    hal_host_rx_ppdu_dscr_stru *ppdu_dscr = NULL;
    uint32_t ret;

    ret = hal_ring_get_entry_count(ring_ctl, &need_add);
    if (ret != OAL_SUCC) {
        oam_warning_log1(0, OAM_SF_RX, "{bisheng_add_rx_ppdu_dscr, return code %d.}", ret);
        return ret;
    }
    for (loop = 0; loop < need_add; loop++) {
        ppdu_dscr = oal_memalloc(sizeof(hal_host_rx_ppdu_dscr_stru));
        if (ppdu_dscr == NULL) {
            oam_error_log0(0, OAM_SF_RX, "bisheng_host_rx_add_buff_alloc alloc fail");
            break;
        }
        memset_s(ppdu_dscr, sizeof(hal_host_rx_ppdu_dscr_stru), 0, sizeof(hal_host_rx_ppdu_dscr_stru));

        if (bisheng_dma_map_rx_ppdu_dscr(DMA_FROM_DEVICE, ppdu_dscr, &pci_dma_addr) != OAL_SUCC) {
            oal_free(ppdu_dscr);
            oam_warning_log0(0, OAM_SF_RX, "bisheng_host_rx_add_buff_alloc::alloc node fail");
            break;
        }

        if (pcie_if_hostca_to_devva(HCC_EP_WIFI_DEV, pci_dma_addr, &devva) != OAL_SUCC) {
            oal_free(ppdu_dscr);
            break;
        }

        ppdu_dscr->paddr.addr = pci_dma_addr;
        oal_dlist_add_tail(&ppdu_dscr->entry, &alloc_list->list_head);
        hal_ring_set_entries(ring_ctl, devva);
        succ_add_num++;
    }
    if (succ_add_num > 0) {
        hal_ring_set_sw2hw(ring_ctl);
    }
    return OAL_SUCC;
}

/*
 * 功能描述：sniffer功能 rx ppdu描述符 free ring 初始化
 */
uint32_t bisheng_sniffer_rx_ppdu_free_ring_init(hal_host_device_stru *hal_device)
{
    /* ring 初始化 */
    if (bisheng_rx_ppdu_dscr_free_ring_init(hal_device) != OAL_SUCC) {
        oam_error_log0(0, 0, "bisheng_sniffer_rx_ppdu_free_ring_init::init_rx_ppdu_free_ring fail.");
        return OAL_FAIL;
    }
    bisheng_rx_set_ring_regs(&(hal_device->host_rx_ppdu_free_ring));
    /* 向 free ring添加内存 */
    if (bisheng_add_rx_ppdu_dscr(hal_device) != OAL_SUCC) {
        oam_error_log0(0, 0, "bisheng_sniffer_rx_ppdu_free_ring_init::add buff fail.");
        return OAL_FAIL;
    }
    return OAL_SUCC;
}

/*
 * 功能描述: 释放rx ppdu ring
 */
void bisheng_rx_ppdu_from_list_free(hal_host_rx_ppdu_dscr_list_head_stru *ppdu_list)
{
    hal_host_rx_ppdu_dscr_stru *ppdu_dscr = NULL;

    if (ppdu_list == NULL) {
        oam_error_log0(0, OAM_SF_RX, "bisheng_rx_ppdu_from_list_free::ppdu_list null");
        return;
    }

    /* 循环查找保存的链表，找到该ppdu_dscr, 并从链表中释放 */
    while (!oal_dlist_is_empty(&(ppdu_list->list_head))) {
        ppdu_dscr = (hal_host_rx_ppdu_dscr_stru *)oal_dlist_delete_head(&ppdu_list->list_head);
        oal_mem_free_m(ppdu_dscr, OAL_TRUE);
    }

    ppdu_list->list_num = 0;
}

/*
 * 功能描述：sniffer功能 rx ppdu描述符 free ring 去初始化
 */
void bisheng_sniffer_rx_ppdu_free_ring_deinit(hal_host_device_stru *hal_dev)
{
    hal_host_ring_ctl_stru *free_rctl = &hal_dev->host_rx_ppdu_free_ring;
    hal_host_rx_ppdu_dscr_list_head_stru *alloc_list = &hal_dev->host_rx_ppdu_alloc_list;

    if (alloc_list == NULL) {
        oam_warning_log0(0, OAM_SF_RX, "bisheng_sniffer_rx_ppdu_free_ring_deinit::ppdu_list null");
        return;
    }

    /* 释放alloc list */
    bisheng_rx_ppdu_from_list_free(&hal_dev->host_rx_ppdu_alloc_list);
    /* 释放ring本身 */
    free_rctl->un_read_ptr.read_ptr = 0;
    free_rctl->un_write_ptr.write_ptr = 0;
    hal_ring_set_sw2hw(free_rctl);
}

/*
 * 功能描述：rx status statistic 信息填充
 */
void bisheng_sniffer_set_rx_info(hal_sniffer_extend_info *sniffer_rx_info, mac_rx_ctl_stru *cb,
    bisheng_rx_ppdu_desc_stru *ppdu_dscr)
{
    hal_sniffer_rx_status_stru *sniffer_rx_status = &sniffer_rx_info->sniffer_rx_status;
    hal_sniffer_rx_statistic_stru *sniffer_rx_statistic = &sniffer_rx_info->sniffer_rx_statistic;
    sniffer_rx_status->bit_cipher_protocol_type = cb->bit_cipher_type;
    sniffer_rx_status->bit_dscr_status = cb->rx_status;
    sniffer_rx_status->bit_AMPDU = ppdu_dscr->rx_ppdu_vector_info.bit_ampdu_en;
    sniffer_rx_status->bit_last_mpdu_flag = 0;
    sniffer_rx_status->bit_gi_type = ppdu_dscr->rx_ppdu_vector_info.bit_gi_type;
    sniffer_rx_status->bit_he_ltf_type = ppdu_dscr->rx_ppdu_vector_info.bit_he_ltf_type;
    sniffer_rx_status->bit_sounding_mode = ppdu_dscr->rx_ppdu_vector_info.bit_sounding_mode;
    sniffer_rx_status->bit_freq_bandwidth_mode = ppdu_dscr->rx_ppdu_vector_info.bit_freq_bw;
    sniffer_rx_status->bit_rx_himit_flag = ppdu_dscr->rx_ppdu_vector_info.bit_preamble;
    sniffer_rx_status->bit_ext_spatial_streams = ppdu_dscr->rx_ppdu_vector_info.bit_spatial_reuse;
    sniffer_rx_status->bit_smoothing = ppdu_dscr->rx_ppdu_vector_info.bit_smoothing;
    sniffer_rx_status->bit_fec_coding = ppdu_dscr->rx_ppdu_user_info.bit_fec_coding;
    sniffer_rx_status->un_nss_rate.st_rate.bit_rate_mcs = ppdu_dscr->rx_ppdu_user_info.bit_nss_mcs_rate;
    sniffer_rx_status->un_nss_rate.st_rate.bit_nss_mode = 0;
    sniffer_rx_status->un_nss_rate.st_rate.bit_protocol_mode = ppdu_dscr->rx_ppdu_vector_info.bit_protocol_mode;
    sniffer_rx_status->un_nss_rate.st_rate.bit_is_rx_vip = 0;
    sniffer_rx_status->un_nss_rate.st_rate.bit_rsp_flag = ppdu_dscr->rx_ppdu_vector_info.bit_response_flag;
    sniffer_rx_status->un_nss_rate.st_rate.bit_mu_mimo_flag = ppdu_dscr->rx_ppdu_vector_info.bit_mu_mimo_flag;
    sniffer_rx_status->un_nss_rate.st_rate.bit_ofdma_flag = ppdu_dscr->rx_ppdu_vector_info.bit_ofdma_flag;
    sniffer_rx_status->un_nss_rate.st_rate.bit_beamforming_flag = 0;
    sniffer_rx_status->un_nss_rate.st_rate.bit_STBC = ppdu_dscr->rx_ppdu_vector_info.bit_stbc_mode;

    sniffer_rx_statistic->c_rssi_dbm = 0;
    sniffer_rx_statistic->uc_code_book = 0;
    sniffer_rx_statistic->uc_grouping = 0;
    sniffer_rx_statistic->uc_row_number = 0;
    sniffer_rx_statistic->c_snr_ant0 = (int8_t)ppdu_dscr->rx_ppdu_vector_info.bit_rpt_snr_ant0;
    sniffer_rx_statistic->c_snr_ant1 = (int8_t)ppdu_dscr->rx_ppdu_vector_info.bit_rpt_snr_ant1;
    sniffer_rx_statistic->c_ant0_rssi = ppdu_dscr->rx_ppdu_vector_info.bit_rpt_rssi_lltf_ant0;
    sniffer_rx_statistic->c_ant1_rssi = ppdu_dscr->rx_ppdu_vector_info.bit_rpt_rssi_lltf_ant0;
}

/*
 * 功能描述：rx速率信息填充
 */
void bisheng_sniffer_set_rx_rate_info(hal_sniffer_extend_info *sniffer_rx_info,
    bisheng_rx_ppdu_desc_stru *ppdu_dscr)
{
    hal_statistic_stru *per_rate = &sniffer_rx_info->per_rate;
    uint32_t *rate_kbps = &sniffer_rx_info->rate_kbps;

    /* 获取接收帧的速率 */
    per_rate->un_nss_rate.st_ht_rate.bit_ht_mcs = ppdu_dscr->rx_ppdu_user_info.bit_nss_mcs_rate;
    per_rate->un_nss_rate.st_ht_rate.bit_protocol_mode = ppdu_dscr->rx_ppdu_vector_info.bit_protocol_mode;
    per_rate->uc_short_gi = ppdu_dscr->rx_ppdu_vector_info.bit_gi_type;
    per_rate->bit_preamble = ppdu_dscr->rx_ppdu_vector_info.bit_preamble;
    per_rate->bit_stbc = ppdu_dscr->rx_ppdu_vector_info.bit_stbc_mode;
    per_rate->bit_channel_code = ppdu_dscr->rx_ppdu_user_info.bit_fec_coding;
    per_rate->uc_bandwidth = ppdu_dscr->rx_ppdu_vector_info.bit_freq_bw;
    if (hmac_get_rate_kbps(per_rate, rate_kbps) == OAL_FALSE) {
        oam_warning_log0(0, OAM_SF_ANY, "{bisheng_sniffer_set_rx_rate_info::get rate failed.}");
        /* 若获取速率失败, 默认速率为6Mbps（6000kbps） */
        *rate_kbps = 6000;
    }
}

#define SNIFFER_INVALIAD_ADDR 0xffffffff
static uint64_t bisheng_get_rx_ppdu_dscr_addr(oal_netbuf_stru *netbuf)
{
    uint64_t rx_ppdu_dscr_addr = SNIFFER_INVALIAD_ADDR;
    bisheng_rx_mpdu_desc_stru *rx_hw_dscr = NULL;

    /* 之前函数中修改data指针指向帧体 为获取帧头部分mpdu描述符中的lsb和msb 修改data指针指向帧头 */
    oal_netbuf_push(netbuf, HAL_RX_DSCR_LEN);

    rx_hw_dscr = (bisheng_rx_mpdu_desc_stru *)oal_netbuf_data(netbuf);
    if (rx_hw_dscr->ppdu_host_buf_addr_lsb != SNIFFER_INVALIAD_ADDR ||
        rx_hw_dscr->ppdu_host_buf_addr_msb != SNIFFER_INVALIAD_ADDR) {
        rx_ppdu_dscr_addr = oal_make_word64(rx_hw_dscr->ppdu_host_buf_addr_lsb, rx_hw_dscr->ppdu_host_buf_addr_msb);
    }

    /* 获取到地址值后 再修改data指针指向帧体部分 */
    oal_netbuf_pull(netbuf, HAL_RX_DSCR_LEN);

    return rx_ppdu_dscr_addr;
}

hal_host_rx_ppdu_dscr_stru *bisheng_sniffer_get_rx_ppdu_dscr(hal_host_device_stru *hal_dev, oal_netbuf_stru *netbuf)
{
    uint64_t rx_ppdu_dscr_addr;
    uint64_t host_iova = 0;
    hal_host_rx_ppdu_dscr_stru *rx_ppdu = NULL;
    oal_dlist_head_stru *entry = NULL;
    oal_dlist_head_stru *entry_tmp = NULL;

    rx_ppdu_dscr_addr = bisheng_get_rx_ppdu_dscr_addr(netbuf);
    if (oal_unlikely(rx_ppdu_dscr_addr == SNIFFER_INVALIAD_ADDR)) {
        return NULL;
    }

    if (oal_unlikely(pcie_if_devva_to_hostca(HCC_EP_WIFI_DEV, rx_ppdu_dscr_addr, (uint64_t *)&host_iova) != OAL_SUCC)) {
        return NULL;
    }

    oal_dlist_search_for_each_safe(entry, entry_tmp, &hal_dev->host_rx_ppdu_alloc_list.list_head) {
        rx_ppdu = oal_dlist_get_entry(entry, hal_host_rx_ppdu_dscr_stru, entry);
        if (rx_ppdu->paddr.addr == host_iova) {
            return rx_ppdu;
        }
    }

    return NULL;
}

void bisheng_sniffer_rx_info_fill(hal_host_device_stru *hal_device, oal_netbuf_stru *netbuf,
    hal_sniffer_extend_info *sniffer_rx_info, mac_rx_ctl_stru *rx_ctl)
{
    hal_host_rx_ppdu_dscr_stru *rx_ppdu = NULL;
    oal_pci_dev_stru *pcie_dev = oal_get_wifi_pcie_dev();

    if (oal_unlikely(pcie_dev == NULL)) {
        oam_error_log0(0, 0, "bisheng_sniffer_rx_info_fill:pcie_linux_res null");
        return;
    }

    if (!rx_ctl->bit_is_first_buffer) {
        return;
    }

    rx_ppdu = bisheng_sniffer_get_rx_ppdu_dscr(hal_device, netbuf);
    if (rx_ppdu == NULL) {
        return;
    }

    dma_unmap_single(&pcie_dev->dev, rx_ppdu->paddr.addr,
        sizeof(bisheng_rx_ppdu_desc_stru), DMA_FROM_DEVICE);

    bisheng_sniffer_set_rx_info(sniffer_rx_info, rx_ctl, &rx_ppdu->hw_ppdu_data);
    bisheng_sniffer_set_rx_rate_info(sniffer_rx_info, &rx_ppdu->hw_ppdu_data);

    oal_dlist_delete_entry(&rx_ppdu->entry);
    oal_free(rx_ppdu);
}

#endif /* _PRE_WLAN_FEATURE_SNIFFER */
