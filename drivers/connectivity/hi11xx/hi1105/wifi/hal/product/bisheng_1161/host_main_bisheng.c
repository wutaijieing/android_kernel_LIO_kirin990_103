

#include "host_hal_ops.h"
#include "host_dscr_bisheng.h"
#include "host_irq_bisheng.h"
#include "host_mac_bisheng.h"

#include "host_ftm_bisheng.h"
#include "host_csi_bisheng.h"
#include "host_psd_bisheng.h"
#include "host_sniffer_bisheng.h"
#include "host_tx_bisheng.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HOST_HAL_MAIN_BISHENG_C

const struct hal_common_ops_stru g_hal_common_ops_bisheng = {
    .host_chip_irq_init          = bisheng_host_chip_irq_init,
    .host_rx_add_buff            = bisheng_host_rx_add_buff,
    .host_get_rx_msdu_status     = bisheng_host_get_rx_msdu_status,
    .host_rx_amsdu_list_build    = bisheng_host_rx_amsdu_list_build,
    .rx_get_next_sub_msdu        = bisheng_rx_get_next_sub_msdu,
    .host_rx_get_msdu_info_dscr  = bisheng_host_rx_get_msdu_info_dscr,
    .rx_host_start_dscr_queue    = bisheng_rx_host_start_dscr_queue,
    .rx_host_init_dscr_queue     = bisheng_rx_host_init_dscr_queue,
    .rx_host_stop_dscr_queue     = bisheng_rx_host_stop_dscr_queue,
    .rx_free_res                 = bisheng_rx_free_res,
    .tx_ba_info_dscr_get         = bisheng_tx_ba_info_dscr_get,
    .tx_msdu_dscr_info_get       = bisheng_tx_msdu_dscr_info_get,
    .host_intr_regs_init         = bisheng_host_intr_regs_init,
    .host_ba_ring_regs_init      = bisheng_host_ba_ring_regs_init,
    .host_ba_ring_depth_get      = bisheng_host_ba_ring_depth_get,
    .host_ring_tx_init           = bisheng_host_ring_tx_init,
    .host_rx_reset_smac_handler  = bisheng_host_rx_reset_smac_handler,
    .rx_alloc_list_free          = bisheng_rx_alloc_list_free,
    .host_al_rx_fcs_info         = bisheng_host_al_rx_fcs_info,
    .host_get_rx_pckt_info       = bisheng_host_get_rx_pckt_info,
    .host_rx_proc_msdu_dscr      = bisheng_host_rx_proc_msdu_dscr,
    .host_init_common_timer      = bisheng_host_init_common_timer,
    .host_set_mac_common_timer   = bisheng_host_set_mac_common_timer,
    .host_get_tx_tid_ring_size   = bisheng_host_get_tx_tid_ring_size,
    .host_get_tx_tid_ring_depth  = bisheng_host_get_tx_tid_ring_depth,
#ifdef _PRE_WLAN_FEATURE_CSI
    .host_get_csi_info           = bisheng_get_csi_info,
    .host_csi_config             = bisheng_csi_config,
    .host_ftm_csi_init           = bisheng_host_ftm_csi_init,
#endif
#ifdef _PRE_WLAN_FEATURE_FTM
    .host_vap_get_hal_vap_id     = bisheng_host_vap_get_by_hal_vap_id,
    .host_ftm_reg_init           = bisheng_host_ftm_reg_init,
    .host_ftm_get_info           = bisheng_host_ftm_get_info,
    .host_ftm_get_divider        = bisheng_host_ftm_get_divider,
    .host_ftm_set_sample         = bisheng_host_ftm_set_sample,
    .host_ftm_set_enable         = bisheng_host_ftm_set_enable,
    .host_ftm_set_m2s_phy        = bisheng_host_ftm_set_m2s_phy,
    .host_ftm_set_buf_base_addr  = bisheng_host_ftm_set_buf_base_addr,
    .host_ftm_set_buf_size       = bisheng_host_ftm_set_buf_size,
    .host_ftm_set_white_list     = bisheng_host_ftm_set_white_list,
#endif
#ifdef _PRE_WLAN_FEATURE_PSD_ANALYSIS
    .host_psd_rpt_to_file        = bisheng_psd_rpt_to_file,
#endif
#ifdef _PRE_WLAN_FEATURE_SNIFFER
    .host_sniffer_rx_ppdu_free_ring_init = bisheng_sniffer_rx_ppdu_free_ring_init,
    .host_sniffer_rx_ppdu_free_ring_deinit = bisheng_sniffer_rx_ppdu_free_ring_deinit,
    .host_sniffer_rx_info_fill = bisheng_sniffer_rx_info_fill,
    .host_sniffer_add_rx_ppdu_dscr = bisheng_add_rx_ppdu_dscr,
#endif
    .host_get_device_tx_ring_num = bisheng_host_get_device_tx_ring_num,
    .host_rx_tcp_udp_checksum      = bisheng_host_rx_tcp_udp_checksum,
    .host_rx_ip_checksum_is_pass   = bisheng_host_rx_ip_checksum_is_pass,
};
