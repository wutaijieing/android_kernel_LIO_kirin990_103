

#ifndef __HOST_SNIFFER_BISHENG_H__
#define __HOST_SNIFFER_BISHENG_H__

#ifdef _PRE_WLAN_FEATURE_SNIFFER

#include "oal_ext_if.h"
#include "hal_common.h"

#define HAL_RX_PPDU_FREE_RING_COUNT 32
#define MEM_RX_PPDU_DSCR_SIZE 300

/* Host RX PPDU 描述符相关定义 与 device 一致 */
typedef struct tag_bisheng_rx_ppdu_vector_info_stru {
    /* word 0 */
    uint32_t  bit_ampdu_en                 : 1;
    uint32_t  bit_protocol_mode            : 4;
    uint32_t  bit_freq_bw                  : 3;
    uint32_t  bit_gi_type                  : 2;
    uint32_t  bit_he_ltf_type              : 2;
    uint32_t  bit_preamble                 : 1;
    uint32_t  bit_uplink_flag              : 1;
    uint32_t  bit_sounding_mode            : 2;
    uint32_t  bit_smoothing                : 1;
    uint32_t  bit_stbc_mode                : 2;
    uint32_t  bit_hesigb_err               : 1;
    uint32_t  bit_extended_spatial_streams : 3;
    uint32_t  bit_txop_ps_not_allowed      : 1;
    uint32_t  bit_user_num                 : 7;
    uint32_t  bit_reserved0                : 1;

    /* word 1 */
    uint32_t bit_txop_duration             : 7;
    uint32_t bit_response_flag             : 1;
    uint32_t bit_group_id_bss_color        : 6;
    uint32_t bit_center_ru_allocation      : 2;
    uint32_t bit_spatial_reuse             : 4;
    uint32_t bit_legacy_length             : 12;

    /* word 2 */
    uint32_t bit_psdu_len                  : 23;
    uint32_t bit_reserved1                 : 1;
    uint32_t bit_ru_allocation             : 8;

    /* word 3 */
    int8_t bit_rpt_rssi_lltf_ant0; /* rpt_rssi_0ch_msb 主20M的天线0的RSSI 高8bit，单位：0.25dBm，表示-128~127dBm */
    int8_t bit_rpt_rssi_lltf_ant1; /* rpt_rssi_1ch_msb 主20M的天线1的RSSI 高8bit，单位：0.25dBm，表示-128~127dBm */
    int8_t bit_reserved2;
    int8_t bit_reserved3;

    /* word 4 */
    uint32_t bit_rpt_snr_ant0              : 8; /* bisheng rpt_snr_0ch */
    uint32_t bit_rpt_snr_ant1              : 8; /* bisheng rpt_snr_1ch */
    uint32_t bit_reserved4                 : 16;

    /* word 5 */
    uint32_t bit_partial_aid               : 9;
    uint32_t bit_bf_flag                   : 1;
    uint32_t bit_ofdma_flag                : 1;
    uint32_t bit_mu_mimo_flag              : 1;
    uint32_t bit_reserved5                 : 12;
    uint32_t bit_rpt_rssi_0ch_lsb          : 2; /* 主20M的天线0的RSSI 低2bit，单位：0.25dBm，表示-128~127dBm */
    uint32_t bit_rpt_rssi_1ch_lsb          : 2; /* 主20M的天线1的RSSI 低2bit，单位：0.25dBm，表示-128~127dBm */
    uint32_t bit_reserved6                 : 4;

    /* word 6, 7 */
    uint32_t he_sig_ru_allocation_2ch;
    uint32_t he_sig_ru_allocation_lch;
} bisheng_rx_ppdu_vector_info_stru;

/* word8-13 */
typedef struct tag_bisheng_rx_ppdu_frame_info_stru {
    /* word 8 */
    uint32_t bit_is_data_frm               : 1;
    uint32_t bit_mc_flag                   : 1;
    uint32_t bit_himit_recv_mode           : 2;
    uint32_t bit_reserved0                 : 4;
    uint32_t bit_rx_vap_id                 : 8;
    uint32_t bit_reserved1                 : 16;

    /* word 9 */
    uint32_t txbf_matrix_rpt_addr;

    /* word 10 */
    uint32_t rx_start_timestamp;

    /* word 11 */
    uint32_t bit_rx_ppdu_proc_time         : 16;
    uint32_t bit_sequence_num              : 12;
    uint32_t bit_reserved2                 : 4;

    /* word 12 */
    uint32_t bit_mgmt_first_wptr           : 16;
    uint32_t bit_mgmt_mpdu_num             : 15;
    uint32_t bit_mgmt_ring_valid           : 1;

    /* word 13 */
    uint32_t bit_data_first_wptr           : 16;
    uint32_t bit_data_mpdu_num             : 15;
    uint32_t bit_data_ring_valid           : 1;
} bisheng_rx_ppdu_frame_info_stru;

#define BISHENG_HIMIT_RECV_TX_ADN_NUM        8

typedef struct tag_bisheng_rx_ppdu_himit_stru {
    uint32_t aul_himit_recv_tx_adn[BISHENG_HIMIT_RECV_TX_ADN_NUM];
} bisheng_rx_ppdu_himit_stru;

typedef struct tag_bisheng_rx_ppdu_user_info_stru {
    uint32_t bit_rpt_evm_ss0_avg           : 8;
    uint32_t bit_rpt_evm_ss1_avg           : 8;
    uint32_t bit_rpt_evm_ss2_avg           : 8;
    uint32_t bit_rpt_evm_ss3_avg           : 8;

    uint32_t bit_rpt_freq_offset           : 10;
    uint32_t bit_time_of_arrival           : 6;
    uint32_t bit_sum_mpdu_len              : 16;

    uint32_t bit_delimiter_pass_mpdu_num   : 12;
    uint32_t bit_fcs_pass_mpdu_num         : 12;
    uint32_t bit_nss_mcs_rate              : 6;
    uint32_t bit_fec_coding                : 1;
    uint32_t bit_dcm                       : 1;

    uint32_t bit_reserved3                 : 8;
    uint32_t bit_tid_mask                  : 8;
    uint32_t bit_user_id                   : 12;
    uint32_t bit_htc_vld_flag              : 1;
    uint32_t bit_reserved4                 : 1;
    uint32_t bit_ps_flag                   : 1;
    uint32_t bit_ampdu_en                  : 1;

    uint32_t htc_fld;

    uint32_t bit_frame_control             : 16;
    uint32_t bit_qos_control               : 16;
} bisheng_rx_ppdu_user_info_stru;

/* 接收ppdu描述信息 */
typedef struct tag_bisheng_rx_ppdu_desc_stru {
    /* word0-7 */
    bisheng_rx_ppdu_vector_info_stru rx_ppdu_vector_info;
    /* word8-13 */
    bisheng_rx_ppdu_frame_info_stru  rx_ppdu_frame_info;
    /* word14-21 */
    bisheng_rx_ppdu_himit_stru       rx_ppdu_himit;
    /* word22-27 */
    bisheng_rx_ppdu_user_info_stru   rx_ppdu_user_info;
} bisheng_rx_ppdu_desc_stru;

typedef struct {
    oal_dlist_head_stru entry;
    edma_paddr_t paddr;
    bisheng_rx_ppdu_desc_stru hw_ppdu_data;
} hal_host_rx_ppdu_dscr_stru;

void bisheng_rx_set_ring_regs(hal_host_ring_ctl_stru *rx_ring_ctl);
uint32_t bisheng_sniffer_rx_ppdu_free_ring_init(hal_host_device_stru *hal_device);
void bisheng_sniffer_rx_ppdu_free_ring_deinit(hal_host_device_stru *hal_device);
void bisheng_sniffer_rx_info_fill(hal_host_device_stru *hal_device, oal_netbuf_stru *netbuf,
    hal_sniffer_extend_info *sniffer_rx_info, mac_rx_ctl_stru *rx_ctl);
uint32_t bisheng_add_rx_ppdu_dscr(hal_host_device_stru *hal_device);
uint32_t hmac_get_rate_kbps(hal_statistic_stru *rate_info, uint32_t *rate_kbps);

#endif
#endif
