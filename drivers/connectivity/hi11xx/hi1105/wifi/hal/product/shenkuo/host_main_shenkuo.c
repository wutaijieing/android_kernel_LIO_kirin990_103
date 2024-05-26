

#include "host_hal_ops.h"
#include "host_hal_ops_shenkuo.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HOST_HAL_MAIN_SHENKUO_C

const struct hal_common_ops_stru g_hal_common_ops_shenkuo = {
    .host_chip_irq_init          = shenkuo_host_chip_irq_init,
    .host_rx_add_buff            = shenkuo_host_rx_add_buff,
    .host_get_rx_msdu_status     = shenkuo_host_get_rx_msdu_status,
    .host_rx_amsdu_list_build    = shenkuo_host_rx_amsdu_list_build,
    .rx_get_next_sub_msdu        = shenkuo_rx_get_next_sub_msdu,
    .host_rx_get_msdu_info_dscr  = shenkuo_host_rx_get_msdu_info_dscr,
    .rx_host_start_dscr_queue    = shenkuo_rx_host_start_dscr_queue,
    .rx_host_init_dscr_queue     = shenkuo_rx_host_init_dscr_queue,
    .rx_host_stop_dscr_queue     = shenkuo_rx_host_stop_dscr_queue,
    .rx_free_res                 = shenkuo_rx_free_res,
    .tx_ba_info_dscr_get         = shenkuo_tx_ba_info_dscr_get,
    .tx_msdu_dscr_info_get       = shenkuo_tx_msdu_dscr_info_get,
    .host_intr_regs_init         = shenkuo_host_intr_regs_init,
    .host_ba_ring_regs_init      = shenkuo_host_ba_ring_regs_init,
    .host_ba_ring_depth_get      = shenkuo_host_ba_ring_depth_get,
    .host_ring_tx_init           = shenkuo_host_ring_tx_init,
    .host_rx_reset_smac_handler  = shenkuo_host_rx_reset_smac_handler,
    .rx_alloc_list_free          = shenkuo_rx_alloc_list_free,
    .host_al_rx_fcs_info         = shenkuo_host_al_rx_fcs_info,
    .host_get_rx_pckt_info       = shenkuo_host_get_rx_pckt_info,
    .host_rx_proc_msdu_dscr      = shenkuo_host_rx_proc_msdu_dscr,
    .host_init_common_timer      = shenkuo_host_init_common_timer,
    .host_set_mac_common_timer   = shenkuo_host_set_mac_common_timer,
    .host_get_tx_tid_ring_size   = shenkuo_host_get_tx_tid_ring_size,
    .host_get_tx_tid_ring_depth  = shenkuo_host_get_tx_tid_ring_depth,
#ifdef _PRE_WLAN_FEATURE_CSI
    .host_get_csi_info           = shenkuo_get_csi_info,
    .host_csi_config             = shenkuo_csi_config,
    .host_ftm_csi_init           = shenkuo_host_ftm_csi_init,
#endif
#ifdef _PRE_WLAN_FEATURE_FTM
    .host_vap_get_hal_vap_id     = shenkuo_host_vap_get_by_hal_vap_id,
    .host_ftm_reg_init           = shenkuo_host_ftm_reg_init,
    .host_ftm_get_info           = shenkuo_host_ftm_get_info,
    .host_ftm_get_divider        = shenkuo_host_ftm_get_divider,
    .host_ftm_set_sample         = shenkuo_host_ftm_set_sample,
    .host_ftm_set_enable         = shenkuo_host_ftm_set_enable,
    .host_ftm_set_m2s_phy        = shenkuo_host_ftm_set_m2s_phy,
    .host_ftm_set_buf_base_addr  = shenkuo_host_ftm_set_buf_base_addr,
    .host_ftm_set_buf_size       = shenkuo_host_ftm_set_buf_size,
    .host_ftm_set_white_list     = shenkuo_host_ftm_set_white_list,
#endif
    .host_tx_clear_msdu_padding  = shenkuo_host_tx_clear_msdu_padding,
#ifdef _PRE_WLAN_FEATURE_SNIFFER
    .host_sniffer_rx_ppdu_free_ring_init = NULL,
    .host_sniffer_rx_ppdu_free_ring_deinit = NULL,
    .host_sniffer_rx_info_fill = NULL,
    .host_sniffer_add_rx_ppdu_dscr = NULL,
#endif
#ifdef _PRE_WLAN_FEATURE_VSP
    .host_vsp_msdu_dscr_info_get = shenkuo_vsp_msdu_dscr_info_get,
    .host_rx_buff_recycle = shenkuo_host_rx_buff_recycle,
#endif
    .host_get_tsf_lo = shenkuo_host_get_tsf_lo,
    .host_get_device_tx_ring_num = shenkuo_host_get_device_tx_ring_num,
    .host_rx_tcp_udp_checksum = NULL,
    .host_rx_ip_checksum_is_pass  = NULL,
};
