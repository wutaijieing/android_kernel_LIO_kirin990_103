

#ifndef __HOST_HAL_EXT_IF_H__
#define __HOST_HAL_EXT_IF_H__

#include "oal_ext_if.h"
#include "wlan_types.h"
#include "mac_common.h"

#include "host_hal_ops.h"
#include "host_hal_dscr.h"

extern const struct hal_common_ops_stru *g_hal_common_ops;

int32_t hal_main_init(void);

static inline void hal_host_rx_add_buff(hal_host_device_stru *hal_device, uint8_t en_queue_num)
{
    if (g_hal_common_ops->host_rx_add_buff != NULL) {
        return g_hal_common_ops->host_rx_add_buff(hal_device, en_queue_num);
    }
}

static inline uint32_t hal_host_rx_reset_smac_handler(hal_host_device_stru *hal_device)
{
    if (g_hal_common_ops->host_rx_reset_smac_handler != NULL) {
        return g_hal_common_ops->host_rx_reset_smac_handler(hal_device);
    }
    return OAL_FAIL;
}

static inline void hal_host_chip_irq_init(void)
{
    if (g_hal_common_ops->host_chip_irq_init != NULL) {
        g_hal_common_ops->host_chip_irq_init();
    }
}

static inline uint8_t hal_host_get_rx_msdu_status(oal_netbuf_stru *netbuf)
{
    if (g_hal_common_ops->host_get_rx_msdu_status != NULL) {
        return g_hal_common_ops->host_get_rx_msdu_status(netbuf);
    }
    return 0;
}

static inline void hal_host_rx_amsdu_list_build(hal_host_device_stru *hal_device, oal_netbuf_stru *netbuf)
{
    if (g_hal_common_ops->host_rx_amsdu_list_build != NULL) {
        g_hal_common_ops->host_rx_amsdu_list_build(hal_device, netbuf);
    }
}

static inline oal_netbuf_stru *hal_rx_get_next_sub_msdu(hal_host_device_stru *hal_device,
    oal_netbuf_stru *hal_rx_msdu_dscr)
{
    if (g_hal_common_ops->rx_get_next_sub_msdu != NULL) {
        return g_hal_common_ops->rx_get_next_sub_msdu(hal_device, hal_rx_msdu_dscr);
    }
    return NULL;
}

static inline uint32_t hal_rx_host_start_dscr_queue(uint8_t hal_dev_id)
{
    if (g_hal_common_ops->rx_host_start_dscr_queue != NULL) {
        return g_hal_common_ops->rx_host_start_dscr_queue(hal_dev_id);
    }
    return OAL_FAIL;
}

static inline int32_t hal_rx_host_init_dscr_queue(uint8_t hal_dev_id)
{
    if (g_hal_common_ops->rx_host_init_dscr_queue != NULL) {
        return g_hal_common_ops->rx_host_init_dscr_queue(hal_dev_id);
    }
    return OAL_FAIL;
}

static inline int32_t hal_rx_host_stop_dscr_queue(uint8_t hal_dev_id)
{
    if (g_hal_common_ops->rx_host_stop_dscr_queue != NULL) {
        return g_hal_common_ops->rx_host_stop_dscr_queue(hal_dev_id);
    }
    return OAL_SUCC;
}
static inline void hal_rx_alloc_list_free(hal_host_device_stru *hal_dev, hal_host_rx_alloc_list_stru *alloc_list)
{
    if (g_hal_common_ops->rx_alloc_list_free != NULL) {
        g_hal_common_ops->rx_alloc_list_free(hal_dev, alloc_list);
    }
}
static inline void hal_rx_free_res(hal_host_device_stru *hal_device)
{
    if (g_hal_common_ops->rx_free_res != NULL) {
        g_hal_common_ops->rx_free_res(hal_device);
    }
}
static inline void hal_tx_ba_info_dscr_get(uint8_t *ba_info_data, uint32_t len, hal_tx_ba_info_stru *tx_ba_info)
{
    if (g_hal_common_ops->tx_ba_info_dscr_get != NULL) {
        g_hal_common_ops->tx_ba_info_dscr_get(ba_info_data, tx_ba_info);
    }
}

static inline void hal_tx_msdu_dscr_info_get(oal_netbuf_stru *netbuf, hal_tx_msdu_dscr_info_stru *tx_msdu_info)
{
    if (g_hal_common_ops->tx_msdu_dscr_info_get != NULL) {
        g_hal_common_ops->tx_msdu_dscr_info_get(netbuf, tx_msdu_info);
    }
}

static inline uint8_t hal_host_tx_tid_ring_size_get(void)
{
    if (g_hal_common_ops->host_get_tx_tid_ring_size != NULL) {
        return g_hal_common_ops->host_get_tx_tid_ring_size();
    }
    return 0;
}
static inline uint32_t hal_host_tx_tid_ring_depth_get(uint8_t size)
{
    if (g_hal_common_ops->host_get_tx_tid_ring_depth != NULL) {
        return g_hal_common_ops->host_get_tx_tid_ring_depth(size);
    }
    return 0;
}

static inline void hal_host_intr_regs_init(uint8_t hal_dev_id)
{
    if (g_hal_common_ops->host_intr_regs_init != NULL) {
        g_hal_common_ops->host_intr_regs_init(hal_dev_id);
    }
}

static inline void hal_host_ring_tx_init(uint8_t hal_dev_id)
{
    if (g_hal_common_ops->host_ring_tx_init != NULL) {
        g_hal_common_ops->host_ring_tx_init(hal_dev_id);
    }
}

static inline void hal_host_ba_ring_regs_init(uint8_t hal_dev_id)
{
    if (g_hal_common_ops->host_ba_ring_regs_init != NULL) {
        g_hal_common_ops->host_ba_ring_regs_init(hal_dev_id);
    }
}

static inline uint32_t hal_host_ba_ring_depth_get(void)
{
    if (g_hal_common_ops->host_ba_ring_depth_get != NULL) {
        return g_hal_common_ops->host_ba_ring_depth_get();
    }

    return HAL_DEFAULT_BA_INFO_COUNT;
}

static inline void hal_host_al_rx_fcs_info(hmac_vap_stru *hmac_vap)
{
    if (g_hal_common_ops->host_al_rx_fcs_info != NULL) {
        g_hal_common_ops->host_al_rx_fcs_info(hmac_vap);
    }
}
static inline void hal_host_get_rx_pckt_info(hmac_vap_stru *hmac_vap,
    dmac_atcmdsrv_atcmd_response_event *rx_pkcg_event_info)
{
    if (g_hal_common_ops->host_get_rx_pckt_info != NULL) {
        g_hal_common_ops->host_get_rx_pckt_info(hmac_vap, rx_pkcg_event_info);
    }
}

static inline int32_t hal_host_init_common_timer(hal_mac_common_timer *mac_timer)
{
    if (g_hal_common_ops->host_init_common_timer != NULL) {
        return g_hal_common_ops->host_init_common_timer(mac_timer);
    }
    return OAL_FAIL;
}

static inline void hal_host_set_mac_common_timer(hal_mac_common_timer *mac_timer)
{
    if (g_hal_common_ops->host_set_mac_common_timer != NULL) {
        g_hal_common_ops->host_set_mac_common_timer(mac_timer);
    }
}

static inline uint32_t hal_host_rx_get_msdu_info_dscr(hal_host_device_stru *hal_dev,
    oal_netbuf_stru *netbuf, mac_rx_ctl_stru *rx_info, uint8_t first_msdu)
{
    if (g_hal_common_ops->host_rx_get_msdu_info_dscr != NULL) {
        return g_hal_common_ops->host_rx_get_msdu_info_dscr(hal_dev, netbuf, rx_info, first_msdu);
    }
    return OAL_FAIL;
}

static inline uint32_t hal_host_rx_proc_msdu_dscr(hal_host_device_stru *hal_dev, oal_netbuf_stru *netbuf)
{
    if (g_hal_common_ops->host_rx_proc_msdu_dscr != NULL) {
        return g_hal_common_ops->host_rx_proc_msdu_dscr(hal_dev, netbuf);
    }
    return OAL_FAIL;
}

static inline uint32_t hal_get_csi_info(hmac_csi_info_stru *hmac_csi_info, uint8_t *addr)
{
    if (g_hal_common_ops->host_get_csi_info != NULL) {
        return g_hal_common_ops->host_get_csi_info(hmac_csi_info, addr);
    }
    return OAL_FAIL;
}
static inline hmac_vap_stru *hal_host_vap_get_hal_vap_id(uint8_t hal_device_id, uint8_t hal_vap_id)
{
    if (g_hal_common_ops->host_vap_get_hal_vap_id != NULL) {
        return g_hal_common_ops->host_vap_get_hal_vap_id(hal_device_id, hal_vap_id);
    }
    return NULL;
}
static inline uint32_t hal_host_ftm_reg_init(uint8_t hal_dev_id)
{
    if (g_hal_common_ops->host_ftm_reg_init != NULL) {
        return g_hal_common_ops->host_ftm_reg_init(hal_dev_id);
    }
    return OAL_FAIL;
}

static inline uint32_t hal_host_ftm_get_info(hal_ftm_info_stru *hal_ftm_info, uint8_t *addr)
{
    if (g_hal_common_ops->host_ftm_get_info != NULL) {
        return g_hal_common_ops->host_ftm_get_info(hal_ftm_info, addr);
    }
    return OAL_FAIL;
}

static inline void hal_host_ftm_get_divider(hal_host_device_stru *hal_device, uint8_t *divider)
{
    if (g_hal_common_ops->host_ftm_get_divider != NULL) {
        return g_hal_common_ops->host_ftm_get_divider(hal_device, divider);
    }
}

static inline void hal_host_ftm_set_sample(hal_host_device_stru *hal_device, oal_bool_enum_uint8 ftm_status)
{
    if (g_hal_common_ops->host_ftm_set_sample != NULL) {
        g_hal_common_ops->host_ftm_set_sample(hal_device, ftm_status);
    }
}

static inline void hal_host_ftm_set_enable(hal_host_device_stru *hal_device, oal_bool_enum_uint8 ftm_status)
{
    if (g_hal_common_ops->host_ftm_set_enable != NULL) {
        return g_hal_common_ops->host_ftm_set_enable(hal_device, ftm_status);
    }
}

static inline void hal_host_ftm_set_m2s_phy(hal_host_device_stru *hal_device, uint8_t tx_chain_selection,
                                            uint8_t tx_rssi_selection)
{
    if (g_hal_common_ops->host_ftm_set_m2s_phy != NULL) {
        g_hal_common_ops->host_ftm_set_m2s_phy(hal_device, tx_chain_selection, tx_rssi_selection);
    }
}

static inline void hal_host_ftm_set_buf_base_addr(hal_host_device_stru *hal_device, uint64_t devva)
{
    if (g_hal_common_ops->host_ftm_set_buf_base_addr != NULL) {
        g_hal_common_ops->host_ftm_set_buf_base_addr(hal_device, devva);
    }
}

static inline void hal_host_ftm_set_buf_size(hal_host_device_stru *hal_device, uint16_t cfg_ftm_buf_size)
{
    if (g_hal_common_ops->host_ftm_set_buf_size != NULL) {
        g_hal_common_ops->host_ftm_set_buf_size(hal_device, cfg_ftm_buf_size);
    }
}

static inline uint32_t hal_host_ftm_set_white_list(hal_host_device_stru *hal_device, uint8_t idx,
                                                   uint8_t *mac_addr)
{
    if (g_hal_common_ops->host_ftm_set_white_list != NULL) {
        return g_hal_common_ops->host_ftm_set_white_list(hal_device, idx, mac_addr);
    }
    return OAL_FAIL;
}

static inline uint32_t hal_host_csi_config(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param)
{
    if (g_hal_common_ops->host_csi_config != NULL) {
        return g_hal_common_ops->host_csi_config(mac_vap, len, param);
    }
    return OAL_FAIL;
}


static inline uint32_t hal_host_ftm_csi_init(hal_host_device_stru *hal_device)
{
    if (g_hal_common_ops->host_ftm_csi_init != NULL) {
        return g_hal_common_ops->host_ftm_csi_init(hal_device);
    }
    return OAL_FAIL;
}

static inline uint32_t hal_host_tx_clear_msdu_padding(oal_netbuf_stru *netbuf)
{
    if (g_hal_common_ops->host_tx_clear_msdu_padding != NULL) {
        return g_hal_common_ops->host_tx_clear_msdu_padding(netbuf);
    }

    /* 默认不需要清零padding, 返回成功 */
    return OAL_SUCC;
}

static inline void hal_host_psd_rpt_to_file_func(oal_netbuf_stru *netbuf, uint16_t psd_info_size)
{
    if (g_hal_common_ops->host_psd_rpt_to_file != NULL) {
        g_hal_common_ops->host_psd_rpt_to_file(netbuf, psd_info_size);
    }
}

#ifdef _PRE_WLAN_FEATURE_SNIFFER
static inline uint32_t hal_host_sniffer_rx_ppdu_free_ring_init(hal_host_device_stru *hal_device)
{
    if (g_hal_common_ops->host_sniffer_rx_ppdu_free_ring_init != NULL) {
        return g_hal_common_ops->host_sniffer_rx_ppdu_free_ring_init(hal_device);
    }
    return OAL_FAIL;
}

static inline void hal_host_sniffer_rx_ppdu_free_ring_deinit(hal_host_device_stru *hal_device)
{
    if (g_hal_common_ops->host_sniffer_rx_ppdu_free_ring_deinit != NULL) {
        g_hal_common_ops->host_sniffer_rx_ppdu_free_ring_deinit(hal_device);
    }
}

static inline void hal_host_sniffer_rx_info_fill(hal_host_device_stru *hal_device,
    oal_netbuf_stru *netbuf, hal_sniffer_extend_info *sniffer_rx_info, mac_rx_ctl_stru *rx_ctl)
{
    if (g_hal_common_ops->host_sniffer_rx_info_fill != NULL) {
        g_hal_common_ops->host_sniffer_rx_info_fill(hal_device, netbuf, sniffer_rx_info, rx_ctl);
    }
}

static inline void hal_host_sniffer_add_rx_ppdu_dscr(hal_host_device_stru *hal_device)
{
    if (g_hal_common_ops->host_sniffer_add_rx_ppdu_dscr != NULL) {
        g_hal_common_ops->host_sniffer_add_rx_ppdu_dscr(hal_device);
    }
}

#endif

#ifdef _PRE_WLAN_FEATURE_VSP
static inline void hal_vsp_msdu_dscr_info_get(uint8_t *buffer, hal_tx_msdu_dscr_info_stru *tx_msdu_info)
{
    if (g_hal_common_ops->host_vsp_msdu_dscr_info_get != NULL) {
        g_hal_common_ops->host_vsp_msdu_dscr_info_get(buffer, tx_msdu_info);
    }
}

static inline uint32_t hal_host_rx_buff_recycle(hal_host_device_stru *hal_device, oal_netbuf_head_stru *netbuf_head)
{
    if (g_hal_common_ops->host_rx_buff_recycle != NULL) {
        return g_hal_common_ops->host_rx_buff_recycle(hal_device, netbuf_head);
    }
    return 0;
}
#endif

static inline uint32_t  hal_host_get_tsf_lo(hal_host_device_stru *hal_device, uint8_t hal_vap_id, uint32_t *tsf)
{
    if (g_hal_common_ops->host_get_tsf_lo != NULL) {
        return g_hal_common_ops->host_get_tsf_lo(hal_device, hal_vap_id, tsf);
    }
    return OAL_FAIL;
}
uint32_t hal_host_device_exit(uint8_t hal_dev_id);

static inline uint8_t hal_host_get_device_tx_ring_num(void)
{
    if (g_hal_common_ops->host_get_device_tx_ring_num != NULL) {
        return g_hal_common_ops->host_get_device_tx_ring_num();
    }

    return 0;
}
static inline void hal_host_rx_tcp_udp_checksum(oal_netbuf_stru *netbuf)
{
    if (g_hal_common_ops->host_rx_tcp_udp_checksum != NULL) {
        g_hal_common_ops->host_rx_tcp_udp_checksum(netbuf);
    }
}
static inline uint32_t hal_host_rx_ipv4_checksum_is_pass(oal_netbuf_stru *netbuf)
{
    if (g_hal_common_ops->host_rx_ip_checksum_is_pass != NULL) {
        return g_hal_common_ops->host_rx_ip_checksum_is_pass(netbuf);
    }
    return OAL_TRUE;
}

#endif
