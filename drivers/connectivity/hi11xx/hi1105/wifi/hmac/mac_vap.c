

/* 1 头文件包含 */
#include "wlan_spec.h"
#include "wlan_types.h"
#include "wlan_chip_i.h"

#include "mac_vap.h"
#include "mac_resource.h"
#include "mac_regdomain.h"
#include "mac_ie.h"
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "board.h"
#endif

#ifdef _PRE_WLAN_FEATURE_FTM
#include "mac_ftm.h"
#endif
#include "hmac_vowifi.h"
#ifdef _PRE_WLAN_FEATURE_DFS
#include "hmac_dfs.h"
#endif
#include "mac_mib.h"
#include "hmac_11ax.h"
#include "hmac_11w.h"
#include "mac_frame_inl.h"
#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_VAP_C

/* 2 全局变量定义 */
#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
mac_tx_large_amsdu_ampdu_stru g_st_tx_large_amsdu = { 0 };
#endif
#ifdef _PRE_WLAN_TCP_OPT
mac_tcp_ack_filter_stru g_st_tcp_ack_filter = { 0 };
#endif
mac_rx_buffer_size_stru g_st_rx_buffer_size_stru = { 0 };
mac_small_amsdu_switch_stru g_st_small_amsdu_switch = { 0 };

mac_tcp_ack_buf_switch_stru g_st_tcp_ack_buf_switch = { 0 };

mac_rx_dyn_bypass_extlna_stru g_st_rx_dyn_bypass_extlna_switch = { 0 };
#ifdef _PRE_WLAN_FEATURE_M2S
oal_bool_enum_uint8 g_en_mimo_blacklist = OAL_TRUE;
#endif
mac_data_collect_cfg_stru g_st_data_collect_cfg = {0};
#ifdef _PRE_WLAN_FEATURE_MBO
uint8_t g_uc_mbo_switch = 0;
#endif
uint8_t g_uc_dbac_dynamic_switch = 0;

mac_rx_dyn_bypass_extlna_stru *mac_vap_get_rx_dyn_bypass_extlna_switch(void)
{
    return &g_st_rx_dyn_bypass_extlna_switch;
}

mac_tcp_ack_buf_switch_stru *mac_vap_get_tcp_ack_buf_switch(void)
{
    return &g_st_tcp_ack_buf_switch;
}

mac_small_amsdu_switch_stru *mac_vap_get_small_amsdu_switch(void)
{
    return &g_st_small_amsdu_switch;
}

mac_rx_buffer_size_stru *mac_vap_get_rx_buffer_size(void)
{
    return &g_st_rx_buffer_size_stru;
}

mac_vap_stru *mac_vap_find_another_up_vap_by_mac_vap(mac_vap_stru *pst_src_vap)
{
    mac_device_stru *pst_mac_device = mac_res_get_dev(pst_src_vap->uc_device_id);
    mac_vap_stru *pst_another_vap_up = NULL;

    if (pst_mac_device == NULL) {
        oam_warning_log0(pst_src_vap->uc_vap_id,  OAM_SF_ANY,
            "mac_vap_find_another_vap_by_vap_id:: mac_device is NULL");
        return NULL;
    }

    /* 获取另一个vap */
    pst_another_vap_up = mac_device_find_another_up_vap(pst_mac_device, pst_src_vap->uc_vap_id);
    if (pst_another_vap_up == NULL) {
        return NULL;
    }

    return pst_another_vap_up;
}

oal_bool_enum_uint8 mac_vap_need_set_user_htc_cap_1103(mac_vap_stru *mac_vap)
{
    return (IS_LEGACY_STA(mac_vap));
}

oal_bool_enum_uint8 mac_vap_need_set_user_htc_cap_shenkuo(mac_vap_stru *mac_vap)
{
    return (IS_LEGACY_VAP(mac_vap));
}

oal_bool_enum_uint8 mac_get_rx_6g_flag_shenkuo(dmac_rx_ctl_stru *rx_ctrl)
{
    return rx_ctrl->st_rx_info.is_6ghz_flag;
}

oal_bool_enum_uint8 mac_get_rx_6g_flag_1103(dmac_rx_ctl_stru *rx_ctrl)
{
    return OAL_FALSE;
}


void mac_vap_tx_data_set_user_htc_cap(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    if (g_wlan_spec_cfg->feature_11ax_is_open) {
#ifdef _PRE_WLAN_FEATURE_11AX
        mac_he_hdl_stru *pst_he_hdl = NULL;

        MAC_USER_TX_DATA_INCLUDE_HTC(pst_mac_user) = OAL_FALSE;
        MAC_USER_TX_DATA_INCLUDE_OM(pst_mac_user)  = OAL_FALSE;

        if (!wlan_chip_mac_vap_need_set_user_htc_cap(pst_mac_vap)) {
            return;
        }

        if ((pst_mac_vap->bit_htc_include_custom_switch == OAL_TRUE) &&
            MAC_VAP_IS_WORK_HE_PROTOCOL(pst_mac_vap) && MAC_USER_IS_HE_USER(pst_mac_user)) {
            pst_he_hdl = MAC_USER_HE_HDL_STRU(pst_mac_user);
            /* user he cap +HTC-HE Support 为1 时才会携带htc头 */
            if (pst_he_hdl->st_he_cap_ie.st_he_mac_cap.bit_htc_he_support) {
                MAC_USER_TX_DATA_INCLUDE_HTC(pst_mac_user) = OAL_TRUE;
                /* user 支持rom */ /* 嵌套深度优化封装 */
                mac_vap_user_set_tx_data_include_om(pst_mac_vap, pst_mac_user);
            }
        }

        if (MAC_USER_ARP_PROBE_CLOSE_HTC(pst_mac_user)) {
            oam_warning_log0(0, 0, "{mac_vap_tx_data_set_user_htc_cap::bit_arp_probe_close_htc true, to close htc.}");
            MAC_USER_TX_DATA_INCLUDE_HTC(pst_mac_user) = OAL_FALSE;
            MAC_USER_TX_DATA_INCLUDE_OM(pst_mac_user)  = OAL_FALSE;
        }

        oam_warning_log4(pst_mac_vap->uc_vap_id, OAM_SF_11AX, "{mac_vap_tx_data_set_user_htc_cap:: \
            tx_data_include_htc=[%d], tx_data_include_om=[%d], vap_protocol=[%d], he_cap=[%d].}",
            MAC_USER_TX_DATA_INCLUDE_HTC(pst_mac_user), MAC_USER_TX_DATA_INCLUDE_OM(pst_mac_user),
            MAC_VAP_IS_WORK_HE_PROTOCOL(pst_mac_vap), MAC_USER_IS_HE_USER(pst_mac_user));

#endif
    } else {
        return;
    }
}


void mac_blacklist_free_pointer(mac_vap_stru *pst_mac_vap, mac_blacklist_info_stru *pst_blacklist_info)
{
    if ((pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) ||
        (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA)) {
        memset_s(pst_blacklist_info, sizeof(mac_blacklist_info_stru), 0, sizeof(mac_blacklist_info_stru));
        pst_blacklist_info->uc_blacklist_device_index = 0xFF;
        pst_blacklist_info->uc_blacklist_vap_index = 0xFF;
    }
}
OAL_STATIC wlan_channel_bandwidth_enum_uint8 mac_vap_get_sta_new_bandwith(mac_vap_stru *mac_sta,
    wlan_channel_bandwidth_enum_uint8 bandwidth_ap)
{
    wlan_channel_bandwidth_enum_uint8 sta_new_bandwidth;
/* wlan 2g 11ax先入网，chba 5g 80M后入网， chba退化为40M,当chba再关联一个80M用户时，
 * 取带宽只能取当前vap能力，而不是device最大能力，否则会把辅路设置成80M，不通。
 */
#ifdef _PRE_WLAN_CHBA_MGMT
    wlan_bw_cap_enum_uint8 cap = WLAN_BW_CAP_20M;
    if (mac_is_chba_mode(mac_sta) == OAL_TRUE) {
        mac_vap_get_bandwidth_cap(mac_sta, &cap);
        sta_new_bandwidth = mac_vap_get_bandwith(cap, bandwidth_ap);
    } else {
        sta_new_bandwidth = mac_vap_get_bandwith(mac_mib_get_dot11VapMaxBandWidth(mac_sta), bandwidth_ap);
    }
#else
    sta_new_bandwidth = mac_vap_get_bandwith(mac_mib_get_dot11VapMaxBandWidth(mac_sta), bandwidth_ap);
#endif
    return sta_new_bandwidth;
}

uint8_t mac_vap_get_ap_usr_opern_bandwidth(mac_vap_stru *pst_mac_sta, mac_user_stru *pst_mac_user)
{
    mac_user_ht_hdl_stru *pst_mac_ht_hdl = NULL;
    mac_vht_hdl_stru *pst_mac_vht_hdl = NULL;
    wlan_channel_bandwidth_enum_uint8 en_bandwidth_ap = WLAN_BAND_WIDTH_20M;
    wlan_channel_bandwidth_enum_uint8 en_sta_new_bandwidth;
    uint8_t uc_channel_center_freq_seg;

#ifdef _PRE_WLAN_FEATURE_11AX
    mac_he_hdl_stru *pst_mac_he_hdl = NULL;

    if (g_wlan_spec_cfg->feature_11ax_is_open) {
        pst_mac_he_hdl = MAC_USER_HE_HDL_STRU(pst_mac_user);
    }
#endif

    /* 更新用户的带宽能力cap */
    mac_user_update_ap_bandwidth_cap(pst_mac_user);
    /* 获取HT和VHT结构体指针 */
    pst_mac_ht_hdl = &(pst_mac_user->st_ht_hdl);
    pst_mac_vht_hdl = &(pst_mac_user->st_vht_hdl);

    /******************* VHT BSS operating channel width ****************
     -----------------------------------------------------------------
     |HT Oper Chl Width |VHT Oper Chl Width |BSS Oper Chl Width|
     -----------------------------------------------------------------
     |       0          |        0          |    20MHZ         |
     -----------------------------------------------------------------
     |       1          |        0          |    40MHZ         |
     -----------------------------------------------------------------
     |       1          |        1          |    80MHZ         |
     -----------------------------------------------------------------
     |       1          |        2          |    160MHZ        |
     -----------------------------------------------------------------
     |       1          |        3          |    80+80MHZ      |
     -----------------------------------------------------------------
    **********************************************************************/
    if ((pst_mac_vht_hdl->bit_supported_channel_width == 0) && (pst_mac_vht_hdl->bit_extend_nss_bw_supp != 0)) {
        uc_channel_center_freq_seg = (pst_mac_user->en_user_max_cap_nss == WLAN_SINGLE_NSS) ?
            0 : pst_mac_ht_hdl->uc_chan_center_freq_seg2;
    } else {
        uc_channel_center_freq_seg = pst_mac_vht_hdl->uc_channel_center_freq_seg1;
    }
    if (pst_mac_vht_hdl->en_vht_capable == OAL_TRUE) {
        en_bandwidth_ap = mac_get_bandwith_from_center_freq_seg0_seg1(pst_mac_vht_hdl->en_channel_width,
            pst_mac_sta->st_channel.uc_chan_number, pst_mac_vht_hdl->uc_channel_center_freq_seg0,
            uc_channel_center_freq_seg);
    }

#ifdef _PRE_WLAN_FEATURE_11AX
    if (g_wlan_spec_cfg->feature_11ax_is_open) {
        if ((OAL_TRUE == MAC_USER_IS_HE_USER(pst_mac_user)) && (pst_mac_he_hdl != NULL) &&
            (pst_mac_he_hdl->st_he_oper_ie.st_he_oper_param.bit_vht_operation_info_present == OAL_TRUE)) {
            en_bandwidth_ap = mac_get_bandwith_from_center_freq_seg0_seg1(
                pst_mac_he_hdl->st_he_oper_ie.st_vht_operation_info.uc_channel_width,
                pst_mac_sta->st_channel.uc_chan_number,
                pst_mac_he_hdl->st_he_oper_ie.st_vht_operation_info.uc_center_freq_seg0,
                pst_mac_he_hdl->st_he_oper_ie.st_vht_operation_info.uc_center_freq_seg1);
        }
    }
#endif

    /* ht 20/40M带宽的处理 */
    if ((pst_mac_ht_hdl->en_ht_capable == OAL_TRUE) && (en_bandwidth_ap <= WLAN_BAND_WIDTH_40MINUS) &&
        (OAL_TRUE == mac_mib_get_FortyMHzOperationImplemented(pst_mac_sta))) {
        /* 更新带宽模式 */
        en_bandwidth_ap = mac_get_bandwidth_from_sco(pst_mac_ht_hdl->bit_secondary_chan_offset);
    }

    /* 带宽不能超过mac device的最大能力 */
    en_sta_new_bandwidth = mac_vap_get_sta_new_bandwith(pst_mac_sta, en_bandwidth_ap);
    if (mac_regdomain_channel_is_support_bw(en_sta_new_bandwidth,
        pst_mac_sta->st_channel.uc_chan_number) == OAL_FALSE) {
        oam_warning_log2(0, 0, "{mac_vap_get_ap_usr_opern_bandwidth::channel[%d] is not support bw[%d],set 20MHz}",
                         pst_mac_sta->st_channel.uc_chan_number, en_sta_new_bandwidth);
        en_sta_new_bandwidth = WLAN_BAND_WIDTH_20M;
    }

    return en_sta_new_bandwidth;
}


uint8_t mac_vap_set_bw_check(mac_vap_stru *mac_vap, wlan_channel_bandwidth_enum_uint8 en_sta_new_bandwidth)
{
    wlan_channel_bandwidth_enum_uint8 en_band_with_sta_old;
    uint8_t uc_change;

    en_band_with_sta_old = mac_vap->st_channel.en_bandwidth;
    mac_vap->st_channel.en_bandwidth = en_sta_new_bandwidth;

    /* 判断是否需要通知硬件切换带宽 */
    uc_change = (en_band_with_sta_old == en_sta_new_bandwidth) ? MAC_NO_CHANGE : MAC_BW_CHANGE;

    oam_warning_log2(mac_vap->uc_vap_id, OAM_SF_ASSOC, "mac_vap_set_bw_check::bandwidth[%d]->[%d].",
                     en_band_with_sta_old, en_sta_new_bandwidth);

    return uc_change;
}

OAL_STATIC void mac_vap_csa_go_support_set(mac_vap_stru *pst_mac_vap)
{
    mac_user_stru *pst_mac_user = NULL;
    uint8_t uc_hash_user_index;
    oal_dlist_head_stru *pst_entry = NULL;

    if (pst_mac_vap->us_user_nums > 1) {
        pst_mac_vap->bit_vap_support_csa = OAL_FALSE;
        return;
    }

    for (uc_hash_user_index = 0; uc_hash_user_index < MAC_VAP_USER_HASH_MAX_VALUE; uc_hash_user_index++) {
        oal_dlist_search_for_each(pst_entry, &(pst_mac_vap->ast_user_hash[uc_hash_user_index]))
        {
            pst_mac_user = (mac_user_stru *)oal_dlist_get_entry(pst_entry, mac_user_stru, st_user_hash_dlist);
            if (pst_mac_user->st_cap_info.bit_p2p_support_csa) {
                pst_mac_vap->bit_vap_support_csa = OAL_TRUE;
            } else {
                pst_mac_vap->bit_vap_support_csa = OAL_FALSE;
                return;
            }
        }
    }
}


void mac_vap_csa_support_set(mac_vap_stru *pst_mac_vap, oal_bool_enum_uint8 en_cap)
{
    pst_mac_vap->bit_vap_support_csa = OAL_FALSE;

    if (IS_LEGACY_VAP(pst_mac_vap)) {
        pst_mac_vap->bit_vap_support_csa = OAL_TRUE;
        return;
    }

    if (IS_P2P_CL(pst_mac_vap)) {
        pst_mac_vap->bit_vap_support_csa = en_cap;
        return;
    }
    if (IS_P2P_GO(pst_mac_vap)) {
        mac_vap_csa_go_support_set(pst_mac_vap);
        oam_warning_log1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{mac_vap_csa_support_set::go_support_csa=[%d]!}",
            pst_mac_vap->bit_vap_support_csa);
    }
}


oal_bool_enum_uint8 mac_vap_go_can_not_in_160m_check(mac_vap_stru *p_mac_vap, uint8_t vap_channel)
{
    mac_vap_stru *p_another_mac_vap = mac_vap_find_another_up_vap_by_mac_vap(p_mac_vap);

    if (p_another_mac_vap == NULL) {
        return OAL_FALSE;
    }

    if (p_another_mac_vap->st_channel.uc_chan_number != vap_channel) {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}
#ifdef _PRE_WLAN_CHBA_MGMT
/* 判断当前device下是否有chba vap */
oal_bool_enum_uint8 mac_vap_is_find_chba_vap(mac_device_stru *mac_dev)
{
    uint8_t vap_idx;
    mac_vap_stru *mac_vap = NULL;
    /* chba定制化开关未开启, 无需进行查找 */
    if (!(g_optimized_feature_switch_bitmap & BIT(CUSTOM_OPTIMIZE_CHBA))) {
        return OAL_FALSE;
    }
    if (mac_dev == NULL) {
        return OAL_FALSE;
    }
    for (vap_idx = 0; vap_idx < mac_dev->uc_vap_num; vap_idx++) {
        mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(mac_dev->auc_vap_id[vap_idx]);
        if (mac_vap == NULL) {
            continue;
        }
        /* 如果存在chba vap且状态为up，则返回true */
        if (mac_is_chba_mode(mac_vap) == OAL_TRUE && mac_vap_is_up(mac_vap)) {
            return OAL_TRUE;
        }
    }
    return OAL_FALSE;
}
#endif
oal_bool_enum_uint8 mac_vap_need_proto_downgrade(mac_vap_stru *vap)
{
#ifdef _PRE_WLAN_CHBA_MGMT
    mac_device_stru *mac_dev = NULL;
#endif
    if (g_wlan_spec_cfg->feature_slave_ax_is_support) {
        return OAL_FALSE;
    }
#ifdef _PRE_WLAN_CHBA_MGMT
    mac_dev = mac_res_get_dev(vap->uc_device_id);
    if (mac_vap_is_find_chba_vap(mac_dev) == OAL_TRUE) {
        return OAL_TRUE;
    }
#endif
    if (mac_is_primary_legacy_vap(vap) == OAL_TRUE) {
        // 双sta模式下，wlan0不降协议。
        if (mac_is_dual_sta_mode() == OAL_TRUE) {
            return OAL_FALSE;
        }
        // 非双sta模式，wlan0只在漫游时可以降协议。
        if (vap->en_vap_state != MAC_VAP_STATE_ROAMING) {
            return OAL_FALSE;
        }
    }
    return OAL_TRUE;
}


oal_bool_enum_uint8 mac_vap_avoid_dbac_close_vht_protocol(mac_vap_stru *p_mac_vap)
{
#ifdef _PRE_WLAN_FEATURE_11AX
    mac_vap_stru *p_first_vap = NULL;

    if (mac_vap_need_proto_downgrade(p_mac_vap) == OAL_FALSE) {
        return OAL_FALSE;
    }

    p_first_vap = mac_vap_find_another_up_vap_by_mac_vap(p_mac_vap);
    if (p_first_vap == NULL) {
        return OAL_FALSE;
    }

    if (p_first_vap->st_channel.en_band == WLAN_BAND_2G && MAC_VAP_IS_WORK_HE_PROTOCOL(p_first_vap) &&
        p_mac_vap->st_channel.en_band == WLAN_BAND_5G) {
        /* 2G ax先入网，p2p 5G后入网时 */
        oam_warning_log4(p_mac_vap->uc_vap_id, OAM_SF_ANY,
            "mac_vap_p2p_avoid_dbac_close_vht_protocol::first start vap: band=[%d] is_he_protocol=[%d], \
            start vap:band=[%d], p2p mode[%d] close vht", p_first_vap->st_channel.en_band,
            MAC_VAP_IS_WORK_HE_PROTOCOL(p_first_vap), p_mac_vap->st_channel.en_band, p_mac_vap->en_p2p_mode);
        return OAL_TRUE;
    }
#endif
    return OAL_FALSE;
}


oal_bool_enum_uint8 mac_vap_can_not_start_he_protocol(mac_vap_stru *p_mac_vap)
{
    uint8_t close_he_flag = OAL_FALSE;
#ifdef _PRE_WLAN_FEATURE_11AX
    mac_vap_stru *p_first_vap = NULL;

    if (mac_vap_need_proto_downgrade(p_mac_vap) == OAL_FALSE) {
        return OAL_FALSE;
    }

    p_first_vap = mac_vap_find_another_up_vap_by_mac_vap(p_mac_vap);
    if (p_first_vap == NULL) {
        return OAL_FALSE;
    }

    /* 5G先入网，p2p/wlan1 2G后入网时 */
    if (p_first_vap->st_channel.en_band == WLAN_BAND_5G && (MAC_VAP_IS_WORK_HE_PROTOCOL(p_first_vap) ||
        p_first_vap->st_channel.en_bandwidth >= WLAN_BAND_WIDTH_80PLUSPLUS) &&
        p_mac_vap->st_channel.en_band == WLAN_BAND_2G) {
        close_he_flag =  OAL_TRUE;
    } else if (OAL_TRUE == mac_vap_avoid_dbac_close_vht_protocol(p_mac_vap)) {
        /* 2G ax先入网，p2p/wlan1 5G后入网时 */
        close_he_flag = OAL_TRUE;
    }

    if (close_he_flag == OAL_TRUE) {
        oam_warning_log3(p_mac_vap->uc_vap_id, OAM_SF_ANY,
            "mac_vap_p2p_can_not_start_he_protocol::first start vap: band=[%d] is_he_protocol=[%d], \
            now start vap:band=[%d] can not star he", p_first_vap->st_channel.en_band,
            MAC_VAP_IS_WORK_HE_PROTOCOL(p_first_vap), p_mac_vap->st_channel.en_band);
    }
#endif
    return close_he_flag;
}


oal_bool_enum_uint8 mac_vap_p2p_bw_back_to_40m(mac_vap_stru *p_mac_vap,
    int32_t channel,  wlan_channel_bandwidth_enum_uint8  *channel_bw)
{
#ifdef _PRE_WLAN_FEATURE_11AX
    mac_vap_stru *p_first_vap = NULL;
    wlan_channel_bandwidth_enum_uint8  channl_band_width;

    /* 辅路不支持ax 直接返回 OAL_FALSE */
    if (mac_vap_need_proto_downgrade(p_mac_vap) == OAL_FALSE) {
        return OAL_FALSE;
    }

    p_first_vap = mac_vap_find_another_up_vap_by_mac_vap(p_mac_vap);
    if (p_first_vap == NULL) {
        return OAL_FALSE;
    }

    /* 当前入网vap 大于等于80M && 2g ax先入网 , p2p回退到40M */
    if (*channel_bw >= WLAN_BAND_WIDTH_80PLUSPLUS &&
        p_first_vap->st_channel.en_band == WLAN_BAND_2G && MAC_VAP_IS_WORK_HE_PROTOCOL(p_first_vap)) {
        channl_band_width = mac_regdomain_get_bw_by_channel_bw_cap(channel, WLAN_BW_CAP_40M);
        if (channl_band_width == WLAN_BAND_WIDTH_40PLUS || channl_band_width == WLAN_BAND_WIDTH_40MINUS) {
            oam_warning_log1(0, 0, "{mac_vap_p2p_bw_back_to_40m::5G bw back to =%d", channl_band_width);
            *channel_bw = channl_band_width;
            return OAL_TRUE;
        }
    }
#endif
    return OAL_FALSE;
}


uint32_t mac_vap_set_cb_tx_user_idx(mac_vap_stru *pst_mac_vap, mac_tx_ctl_stru *pst_tx_ctl,
    const unsigned char *puc_data)
{
    uint16_t us_user_idx = g_wlan_spec_cfg->invalid_user_id;
    uint32_t ret = mac_vap_find_user_by_macaddr(pst_mac_vap, puc_data, &us_user_idx);
    if (ret != OAL_SUCC) {
        oam_warning_log4(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{mac_vap_set_cb_tx_user_idx:: cannot find user_idx from xx:xx:xx:%x:%x:%x, set TX_USER_IDX %d.}",
            puc_data[BYTE_OFFSET_3], puc_data[BYTE_OFFSET_4],
            puc_data[BYTE_OFFSET_5], g_wlan_spec_cfg->invalid_user_id);
        MAC_GET_CB_TX_USER_IDX(pst_tx_ctl) = g_wlan_spec_cfg->invalid_user_id;
        return ret;
    }

    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl) = us_user_idx;

    return OAL_SUCC;
}

uint8_t g_uc_uapsd_cap = WLAN_FEATURE_UAPSD_IS_OPEN;

/* WME初始参数定义，按照OFDM初始化 AP模式 值来自于TGn 9 Appendix D: Default WMM AC Parameters */

mac_wme_param_stru g_ast_wmm_initial_params_ap[WLAN_WME_AC_BUTT] = {
    {
        /* BE AIFS, cwmin, cwmax, txop */
        3,          4,     6,     0,
    },

    {
        /* BK AIFS, cwmin, cwmax, txop */
        7,          4,     10,    0,
    },

    {
        /* VI AIFS, cwmin, cwmax, txop */
        1,          3,     4,     3008,
    },

    {
        /* VO AIFS, cwmin, cwmax, txop */
        1,          2,     3,     1504,
    },
};

/* WMM初始参数定义，按照OFDM初始化 STA模式 */
mac_wme_param_stru g_ast_wmm_initial_params_sta[WLAN_WME_AC_BUTT] = {
    {
        /* BE AIFS, cwmin, cwmax, txop */
        3,          3,     10,     0,
    },

    {
        /* BK AIFS, cwmin, cwmax, txop */
        7,          4,     10,     0,
    },

    {
        /* VI AIFS, cwmin, cwmax, txop */
        2,          3,     4,     3008,
    },

    {
        /* VO AIFS, cwmin, cwmax, txop */
        2,          2,     3,     1504,
    },
};

/* WMM初始参数定义，aput建立的bss中STA的使用的EDCA参数 */
mac_wme_param_stru g_ast_wmm_initial_params_bss[WLAN_WME_AC_BUTT] = {
    {
        /* BE AIFS, cwmin, cwmax, txop */
        3,          4,     10,     0,
    },

    {
        /* BK AIFS, cwmin, cwmax, txop */
        7,          4,     10,     0,
    },

    {
        /* VI AIFS, cwmin, cwmax, txop */
        2,          3,     4,     3008,
    },

    {
        /* VO AIFS, cwmin, cwmax, txop */
        2,          2,     3,     1504,
    },
};

#ifdef _PRE_WLAN_FEATURE_EDCA_MULTI_USER_MULTI_AC
/* 多用户多优先级使用的EDCA参数 */
mac_wme_param_stru g_ast_wmm_multi_user_multi_ac_params_ap[WLAN_WME_AC_BUTT] = {
    {
        /* BE AIFS, cwmin, cwmax, txop */
        3,          5,     10,     0,
    },

    {
        /* BK AIFS, cwmin, cwmax, txop */
        3,          5,     10,     0,
    },

    {
        /* VI AIFS, cwmin, cwmax, txop */
        3,          5,     10,     0,
    },

    {
        /* VO AIFS, cwmin, cwmax, txop */
        3,          5,     10,     0,
    },
};
#endif


mac_wme_param_stru *mac_get_wmm_cfg(wlan_vap_mode_enum_uint8 en_vap_mode)
{
    /* 参考认证项配置，没有按照协议配置，WLAN_VAP_MODE_BUTT表示是ap广播给sta的edca参数 */
    if (en_vap_mode == WLAN_VAP_MODE_BUTT) {
        return (mac_wme_param_stru *)g_ast_wmm_initial_params_bss;
    } else if (en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        return (mac_wme_param_stru *)g_ast_wmm_initial_params_ap;
    }
    return (mac_wme_param_stru *)g_ast_wmm_initial_params_sta;
}

#ifdef _PRE_WLAN_FEATURE_EDCA_MULTI_USER_MULTI_AC

mac_wme_param_stru *mac_get_wmm_cfg_multi_user_multi_ac(oal_traffic_type_enum_uint8 uc_traffic_type)
{
    /* 多用户下业务类型采用新参数，否则采用ap模式下的默认值 */
    if (uc_traffic_type == OAL_TRAFFIC_MULTI_USER_MULTI_AC) {
        return (mac_wme_param_stru *)g_ast_wmm_multi_user_multi_ac_params_ap;
    }

    return (mac_wme_param_stru *)g_ast_wmm_initial_params_ap;
}
#endif

#ifdef _PRE_WLAN_FEATURE_TXOPPS

uint8_t mac_vap_get_txopps(mac_vap_stru *pst_vap)
{
    return pst_vap->st_cap_flag.bit_txop_ps;
}


void mac_vap_set_txopps(mac_vap_stru *pst_vap, uint8_t uc_value)
{
    pst_vap->st_cap_flag.bit_txop_ps = uc_value;
}


void mac_vap_update_txopps(mac_vap_stru *pst_vap, mac_user_stru *pst_user)
{
    /* 如果用户使能txop ps，则vap使能 */
    if (pst_user->st_vht_hdl.bit_vht_txop_ps == OAL_TRUE && OAL_TRUE == mac_mib_get_txopps(pst_vap)) {
        mac_vap_set_txopps(pst_vap, OAL_TRUE);
    }
}

#endif

#ifdef _PRE_WLAN_FEATURE_SMPS

wlan_mib_mimo_power_save_enum mac_vap_get_smps_mode(mac_vap_stru *pst_mac_vap)
{
    return pst_mac_vap->pst_mib_info->st_wlan_mib_ht_sta_cfg.en_dot11MIMOPowerSave;
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)

wlan_mib_mimo_power_save_enum mac_vap_get_smps_en(mac_vap_stru *pst_mac_vap)
{
    mac_device_stru *pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);

    if (pst_mac_device == NULL) {
        oam_error_log1(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{mac_vap_get_smps_en::pst_mac_device[%d] null.}",
                       pst_mac_vap->uc_device_id);

        return (wlan_mib_mimo_power_save_enum)OAL_ERR_CODE_PTR_NULL;
    }

    return (wlan_mib_mimo_power_save_enum)(pst_mac_device->en_mac_smps_mode);
}
#endif


void mac_vap_set_smps(mac_vap_stru *pst_vap, uint8_t uc_value)
{
    pst_vap->pst_mib_info->st_wlan_mib_ht_sta_cfg.en_dot11MIMOPowerSave = uc_value;
}
#endif


uint32_t mac_vap_set_uapsd_en(mac_vap_stru *pst_mac_vap, uint8_t uc_value)
{
    pst_mac_vap->st_cap_flag.bit_uapsd = (uc_value == OAL_TRUE) ? 1 : 0;

    return OAL_SUCC;
}


uint8_t mac_vap_get_uapsd_en(mac_vap_stru *pst_mac_vap)
{
    return pst_mac_vap->st_cap_flag.bit_uapsd;
}


uint32_t mac_vap_user_exist(oal_dlist_head_stru *pst_new, oal_dlist_head_stru *pst_head)
{
    oal_dlist_head_stru      *pst_user_list_head = NULL;
    oal_dlist_head_stru      *pst_member_entry = NULL;

    oal_dlist_search_for_each_safe(pst_member_entry, pst_user_list_head, pst_head)
    {
        if (pst_new == pst_member_entry) {
            oam_error_log0(0, OAM_SF_ASSOC, "{oal_dlist_check_head:dmac user doule add.}");
            return OAL_SUCC;
        }
    }

    return OAL_FAIL;
}


uint32_t mac_vap_add_assoc_user(mac_vap_stru *pst_vap, uint16_t us_user_idx)
{
    mac_user_stru              *pst_user = NULL;
    oal_dlist_head_stru        *pst_dlist_head = NULL;

    if (oal_unlikely(pst_vap == NULL)) {
        oam_error_log0(0, OAM_SF_ASSOC, "{mac_vap_add_assoc_user::pst_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_user = (mac_user_stru *)mac_res_get_mac_user(us_user_idx);
    if (oal_unlikely(pst_user == NULL)) {
        oam_warning_log1(pst_vap->uc_vap_id, OAM_SF_ASSOC,
                         "{mac_vap_add_assoc_user::pst_user[%d] null.}", us_user_idx);

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_user->us_user_hash_idx = MAC_CALCULATE_HASH_VALUE(pst_user->auc_user_mac_addr);

    if (OAL_SUCC == mac_vap_user_exist(&(pst_user->st_user_dlist), &(pst_vap->st_mac_user_list_head))) {
        oam_error_log1(pst_vap->uc_vap_id, OAM_SF_ASSOC,
                       "{mac_vap_add_assoc_user::user[%d] already exist.}", us_user_idx);

        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_spin_lock_bh(&pst_vap->st_cache_user_lock);

    pst_dlist_head = &(pst_vap->ast_user_hash[pst_user->us_user_hash_idx]);
#ifdef _PRE_WLAN_DFT_STAT
    (pst_vap->hash_cnt)++;
#endif

    /* 加入双向hash链表表头 */
    oal_dlist_add_head(&(pst_user->st_user_hash_dlist), pst_dlist_head);

    /* 加入双向链表表头 */
    pst_dlist_head = &(pst_vap->st_mac_user_list_head);
    oal_dlist_add_head(&(pst_user->st_user_dlist), pst_dlist_head);
#ifdef _PRE_WLAN_DFT_STAT
    (pst_vap->dlist_cnt)++;
#endif

    /* 更新cache user */
    oal_set_mac_addr(pst_vap->auc_cache_user_mac_addr, pst_user->auc_user_mac_addr);
    pst_vap->us_cache_user_id = us_user_idx;

    /* 记录STA模式下的与之关联的VAP的id */
    if (pst_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        mac_vap_set_assoc_id(pst_vap, us_user_idx);
    }

    /* vap已关联 user个数++ */
    pst_vap->us_user_nums++;

    oal_spin_unlock_bh(&pst_vap->st_cache_user_lock);

    return OAL_SUCC;
}

uint32_t mac_vap_hash_del_user(mac_vap_stru *pst_vap, uint16_t us_user_idx, mac_user_stru *pst_user)
{
    mac_user_stru          *pst_user_temp       = NULL;
    oal_dlist_head_stru    *pst_hash_head       = NULL;
    oal_dlist_head_stru    *pst_entry           = NULL;
    oal_dlist_head_stru    *pst_dlist_tmp       = NULL;
    uint32_t              ret              = OAL_FAIL;
    uint8_t               uc_txop_ps_user_cnt = 0;

    pst_hash_head = &(pst_vap->ast_user_hash[pst_user->us_user_hash_idx]);

    oal_dlist_search_for_each_safe(pst_entry, pst_dlist_tmp, pst_hash_head) {
        pst_user_temp = (mac_user_stru *)oal_dlist_get_entry(pst_entry, mac_user_stru, st_user_hash_dlist);
        /*lint -save -e774 */
        if (pst_user_temp == NULL) {
            oam_error_log1(pst_vap->uc_vap_id, OAM_SF_ASSOC,
                           "{mac_vap_del_user::pst_user_temp null,us_user_idx is %d}", us_user_idx);

            continue;
        }
        /*lint -restore */
        if (pst_user_temp->st_vht_hdl.bit_vht_txop_ps) {
            uc_txop_ps_user_cnt++;
        }

        if (!oal_compare_mac_addr(pst_user->auc_user_mac_addr, pst_user_temp->auc_user_mac_addr)) {
            oal_dlist_delete_entry(pst_entry);

            /* 从双向链表中拆掉 */
            oal_dlist_delete_entry(&(pst_user->st_user_dlist));

            oal_dlist_delete_entry(&(pst_user->st_user_hash_dlist));
            ret = OAL_SUCC;

#ifdef _PRE_WLAN_DFT_STAT
            (pst_vap->hash_cnt)--;
            (pst_vap->dlist_cnt)--;
#endif
            /* 初始化相应成员 */
            pst_user->us_user_hash_idx = 0xffff;
            pst_user->us_assoc_id      = us_user_idx;
            pst_user->en_is_multi_user = OAL_FALSE;
            memset_s(pst_user->auc_user_mac_addr, WLAN_MAC_ADDR_LEN, 0, WLAN_MAC_ADDR_LEN);
            pst_user->uc_vap_id        = 0xff;
            pst_user->uc_device_id     = 0xff;
            pst_user->uc_chip_id       = 0xff;
            pst_user->en_user_asoc_state = MAC_USER_STATE_BUTT;
        }
    }
    /* 没有关联的用户则去初始化vap能力 */
    if (uc_txop_ps_user_cnt == 0) {
#ifdef _PRE_WLAN_FEATURE_TXOPPS
        mac_vap_set_txopps(pst_vap, OAL_FALSE);
#endif
    }
    return ret;
}

uint32_t mac_vap_del_user(mac_vap_stru *pst_vap, uint16_t us_user_idx)
{
    mac_user_stru          *pst_user            = NULL;
    uint32_t              ret;
    if (oal_unlikely(pst_vap == NULL)) {
        oam_error_log1(0, OAM_SF_ASSOC, "{mac_vap_del_user::pst_vap null,us_user_idx is %d}", us_user_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_spin_lock_bh(&pst_vap->st_cache_user_lock);

    /* 与cache user id对比 , 相等则清空cache user */
    if (us_user_idx == pst_vap->us_cache_user_id) {
        oal_set_mac_addr_zero(pst_vap->auc_cache_user_mac_addr);
        pst_vap->us_cache_user_id = g_wlan_spec_cfg->invalid_user_id;
    }

    pst_user = (mac_user_stru *)mac_res_get_mac_user(us_user_idx);
    if (oal_unlikely(pst_user == NULL)) {
        oal_spin_unlock_bh(&pst_vap->st_cache_user_lock);

        oam_warning_log1(pst_vap->uc_vap_id, OAM_SF_ASSOC, "{mac_vap_del_user::pst_user[%d] null.}", us_user_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    mac_user_set_asoc_state(pst_user, MAC_USER_STATE_BUTT);

    if (pst_user->us_user_hash_idx >= MAC_VAP_USER_HASH_MAX_VALUE) {
        /* ADD USER命令丢失，或者重复删除User都可能进入此分支。 */
        oal_spin_unlock_bh(&pst_vap->st_cache_user_lock);

        oam_error_log1(pst_vap->uc_vap_id, OAM_SF_ASSOC, "{mac_vap_del_user::hash idx invaild %u}",
                       pst_user->us_user_hash_idx);
        return OAL_FAIL;
    }

    ret = mac_vap_hash_del_user(pst_vap, us_user_idx, pst_user);
    if (ret == OAL_SUCC) {
        /* vap已关联 user个数-- */
        if (pst_vap->us_user_nums) {
            pst_vap->us_user_nums--;
        }
        /* STA模式下将关联的VAP的id置为非法值 */
        if (pst_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
            mac_vap_set_assoc_id(pst_vap, g_wlan_spec_cfg->invalid_user_id);
        }

        oal_spin_unlock_bh(&pst_vap->st_cache_user_lock);

        return OAL_SUCC;
    }

    oal_spin_unlock_bh(&pst_vap->st_cache_user_lock);

    oam_warning_log1(pst_vap->uc_vap_id, OAM_SF_ASSOC,
                     "{mac_vap_del_user::delete user failed,user idx is %d.}", us_user_idx);
    return OAL_FAIL;
}


uint32_t mac_vap_find_user_by_macaddr(mac_vap_stru *pst_vap, const unsigned char *puc_sta_mac_addr,
    uint16_t *pus_user_idx)
{
    mac_user_stru              *pst_mac_user = NULL;
    uint32_t                  user_hash_value;
    oal_dlist_head_stru        *pst_entry = NULL;

    if (oal_unlikely(oal_any_null_ptr3(pst_vap, puc_sta_mac_addr, pus_user_idx))) {
        oam_error_log0(0, OAM_SF_ANY, "{mac_vap_find_user_by_macaddr::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*
     * 1.本函数*pus_user_idx先初始化为无效值，防止调用者没有初始化，可能出现使用返回值异常;
     * 2.根据查找结果刷新*pus_user_idx值，如果是有效，返回SUCC,无效MAC_INVALID_USER_ID返回FAIL;
     * 3.调用函数根据首先根据本函数返回值做处理，其次根据*pus_user_idx进行其他需要的判断和操作
     */
    *pus_user_idx = g_wlan_spec_cfg->invalid_user_id;

    if (pst_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        pst_mac_user = (mac_user_stru *)mac_res_get_mac_user(pst_vap->us_assoc_vap_id);
        if (pst_mac_user == NULL) {
            return OAL_FAIL;
        }

        if (!oal_compare_mac_addr(pst_mac_user->auc_user_mac_addr, puc_sta_mac_addr)) {
            *pus_user_idx = pst_vap->us_assoc_vap_id;
        }
    } else {
        oal_spin_lock_bh(&pst_vap->st_cache_user_lock);

        /* 与cache user对比 , 相等则直接返回cache user id */
        if (!oal_compare_mac_addr(pst_vap->auc_cache_user_mac_addr, puc_sta_mac_addr)) {
            /* 用户删除后，user macaddr和cache user macaddr地址均为0，但实际上用户已经删除，此时user id无效 */
            *pus_user_idx = pst_vap->us_cache_user_id;
        } else {
            user_hash_value = MAC_CALCULATE_HASH_VALUE(puc_sta_mac_addr);

            oal_dlist_search_for_each(pst_entry, &(pst_vap->ast_user_hash[user_hash_value]))
            {
                pst_mac_user = (mac_user_stru *)oal_dlist_get_entry(pst_entry, mac_user_stru, st_user_hash_dlist);
                /*lint -save -e774 */
                if (pst_mac_user == NULL) {
                    oam_error_log0(pst_vap->uc_vap_id, OAM_SF_ANY,
                                   "{mac_vap_find_user_by_macaddr::pst_mac_user null.}");
                    continue;
                }
                /*lint -restore */
                /* 相同的MAC地址 */
                if (!oal_compare_mac_addr(pst_mac_user->auc_user_mac_addr, puc_sta_mac_addr)) {
                    *pus_user_idx = pst_mac_user->us_assoc_id;
                    /* 更新cache user */
                    oal_set_mac_addr(pst_vap->auc_cache_user_mac_addr, pst_mac_user->auc_user_mac_addr);
                    pst_vap->us_cache_user_id = pst_mac_user->us_assoc_id;
                }
            }
        }
        oal_spin_unlock_bh(&pst_vap->st_cache_user_lock);
    }

    /*
     * user id有效的话，返回SUCC给调用者处理，user id无效的话，
     * 返回user id为MAC_INVALID_USER_ID，并返回查找结果FAIL给调用者处理
     */
    if (*pus_user_idx == g_wlan_spec_cfg->invalid_user_id) {
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


uint32_t mac_device_find_user_by_macaddr(uint8_t device_id, const unsigned char *sta_mac_addr, uint16_t *user_idx)
{
    mac_device_stru            *pst_device = mac_res_get_dev(device_id);
    mac_vap_stru               *pst_mac_vap = NULL;
    uint8_t                   uc_vap_idx;
    uint32_t                  ret;

    if (pst_device == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "mac_device_find_user_by_macaddr:get_dev return null ");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 对device下的所有vap进行遍历 */
    for (uc_vap_idx = 0; uc_vap_idx < pst_device->uc_vap_num; uc_vap_idx++) {
        /* 配置vap不需要处理 */
        if (pst_device->auc_vap_id[uc_vap_idx] == pst_device->uc_cfg_vap_id) {
            continue;
        }

        pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_device->auc_vap_id[uc_vap_idx]);
        if (pst_mac_vap == NULL) {
            continue;
        }

        /* 只处理AP模式 */
        if (pst_mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_AP) {
            continue;
        }

        ret = mac_vap_find_user_by_macaddr(pst_mac_vap, sta_mac_addr, user_idx);
        if (ret == OAL_SUCC) {
            return OAL_SUCC;
        }
    }

    return OAL_FAIL;
}


uint32_t mac_chip_find_user_by_macaddr(uint8_t uc_chip_id, const unsigned char *puc_sta_mac_addr,
    uint16_t *pus_user_idx)
{
    mac_chip_stru              *pst_mac_chip;
    uint8_t                   uc_device_idx;
    uint32_t                  ret;

    /* 获取device */
    pst_mac_chip = mac_res_get_mac_chip(uc_chip_id);
    if (pst_mac_chip == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "{mac_chip_find_user_by_macaddr:get_chip return nul!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    for (uc_device_idx = 0; uc_device_idx < pst_mac_chip->uc_device_nums; uc_device_idx++) {
        ret = mac_device_find_user_by_macaddr(pst_mac_chip->auc_device_id[uc_device_idx],
            puc_sta_mac_addr, pus_user_idx);
        if (ret == OAL_SUCC) {
            return OAL_SUCC;
        }
    }

    return OAL_FAIL;
}

uint32_t mac_board_find_user_by_macaddr(const unsigned char *puc_sta_mac_addr, uint16_t *pus_user_idx)
{
    uint8_t  uc_chip_idx;
    uint32_t ret;

    /* 遍历board下所有chip */
    for (uc_chip_idx = 0; uc_chip_idx < WLAN_CHIP_MAX_NUM_PER_BOARD; uc_chip_idx++) {
        ret = mac_chip_find_user_by_macaddr(uc_chip_idx, puc_sta_mac_addr, pus_user_idx);
        if (ret == OAL_SUCC) {
            return OAL_SUCC;
        }
    }

    return OAL_FAIL;
}

uint8_t g_mcm_mask_custom = 0;


void mac_vap_ini_get_nss_num(mac_device_stru *mac_device,
    wlan_nss_enum_uint8 *nss_num_rx, wlan_nss_enum_uint8 *nss_num_tx)
{
    wlan_nss_enum_uint8 dev_nss_num;
    dev_nss_num = MAC_DEVICE_GET_NSS_NUM(mac_device);
    /* FPGA阶段四天线支持四发双收，可根据定制化控制tx rx的空间流支持四流or双流 */
    /* mcm_mask_custom定制化bit4置1表示4天线时只支持双发，置0表示默认支持四发 */
    /* mcm_mask_custom定制化bit5置1表示4天线时只支持双收，置0表示默认支持四发 */
    if (dev_nss_num == WLAN_FOUR_NSS) {
        *nss_num_tx = ((g_mcm_mask_custom & BIT4) == 0 ? dev_nss_num : dev_nss_num >> 1);
        *nss_num_rx = ((g_mcm_mask_custom & BIT5) == 0 ? dev_nss_num : dev_nss_num >> 1);
    } else {
        *nss_num_tx = dev_nss_num;
        *nss_num_rx = dev_nss_num;
    }
}


void mac_vap_set_tx_power(mac_vap_stru *pst_vap, uint8_t uc_tx_power)
{
    pst_vap->uc_tx_power = uc_tx_power;
}


void mac_vap_set_aid(mac_vap_stru *pst_vap, uint16_t us_aid)
{
    pst_vap->us_sta_aid = us_aid;
}


void mac_vap_set_assoc_id(mac_vap_stru *pst_vap, uint16_t us_assoc_vap_id)
{
    pst_vap->us_assoc_vap_id = us_assoc_vap_id;
}


void mac_vap_set_uapsd_cap(mac_vap_stru *pst_vap, uint8_t uc_uapsd_cap)
{
    pst_vap->uc_uapsd_cap = uc_uapsd_cap;
}


void mac_vap_set_p2p_mode(mac_vap_stru *pst_vap, wlan_p2p_mode_enum_uint8 en_p2p_mode)
{
    pst_vap->en_p2p_mode = en_p2p_mode;
}


void mac_vap_set_multi_user_idx(mac_vap_stru *pst_vap, uint16_t us_multi_user_idx)
{
    pst_vap->us_multi_user_idx = us_multi_user_idx;
}


void mac_vap_set_rx_nss(mac_vap_stru *pst_vap, wlan_nss_enum_uint8 en_rx_nss)
{
    pst_vap->en_vap_rx_nss = en_rx_nss;
}


void mac_vap_set_al_tx_payload_flag(mac_vap_stru *pst_vap, uint8_t uc_paylod)
{
    pst_vap->bit_payload_flag = uc_paylod;
}


void mac_vap_set_al_tx_flag(mac_vap_stru *pst_vap, oal_bool_enum_uint8 en_flag)
{
    pst_vap->bit_al_tx_flag = en_flag;
}


void mac_vap_set_uapsd_para(mac_vap_stru *pst_mac_vap, mac_cfg_uapsd_sta_stru *pst_uapsd_info)
{
    uint8_t uc_ac;

    pst_mac_vap->st_sta_uapsd_cfg.uc_max_sp_len = pst_uapsd_info->uc_max_sp_len;

    for (uc_ac = 0; uc_ac < WLAN_WME_AC_BUTT; uc_ac++) {
        pst_mac_vap->st_sta_uapsd_cfg.uc_delivery_enabled[uc_ac] = pst_uapsd_info->uc_delivery_enabled[uc_ac];
        pst_mac_vap->st_sta_uapsd_cfg.uc_trigger_enabled[uc_ac] = pst_uapsd_info->uc_trigger_enabled[uc_ac];
    }
}


void mac_vap_set_wmm_params_update_count(mac_vap_stru *pst_vap, uint8_t uc_update_count)
{
    pst_vap->uc_wmm_params_update_count = uc_update_count;
}

/* 后续tdls功能可能会打开 */
#ifdef _PRE_DEBUG_CODE

void mac_vap_set_tdls_prohibited(mac_vap_stru *pst_vap, uint8_t uc_value)
{
    pst_vap->st_cap_flag.bit_tdls_prohibited = uc_value;
}


void mac_vap_set_tdls_channel_switch_prohibited(mac_vap_stru *pst_vap, uint8_t uc_value)
{
    pst_vap->st_cap_flag.bit_tdls_channel_switch_prohibited = uc_value;
}
#endif


void mac_vap_set_hide_ssid(mac_vap_stru *pst_vap, uint8_t uc_value)
{
    pst_vap->st_cap_flag.bit_hide_ssid = uc_value;
}


uint8_t mac_vap_get_peer_obss_scan(mac_vap_stru *pst_vap)
{
    return pst_vap->st_cap_flag.bit_peer_obss_scan;
}


void mac_vap_set_peer_obss_scan(mac_vap_stru *pst_vap, uint8_t uc_value)
{
    pst_vap->st_cap_flag.bit_peer_obss_scan = uc_value;
}


wlan_p2p_mode_enum_uint8 mac_get_p2p_mode(mac_vap_stru *pst_vap)
{
    return (pst_vap->en_p2p_mode);
}


void mac_dec_p2p_num(mac_vap_stru *pst_vap)
{
    mac_device_stru *pst_device;

    pst_device = mac_res_get_dev(pst_vap->uc_device_id);
    if (oal_unlikely(pst_device == NULL)) {
        oam_error_log1(pst_vap->uc_vap_id, OAM_SF_ANY,
            "{mac_p2p_dec_num::pst_device[%d] null.}", pst_vap->uc_device_id);
        return;
    }

    if (IS_P2P_DEV(pst_vap)) {
        pst_device->st_p2p_info.uc_p2p_device_num--;
    } else if (IS_P2P_GO(pst_vap) || IS_P2P_CL(pst_vap)) {
        pst_device->st_p2p_info.uc_p2p_goclient_num--;
    }
}

void mac_inc_p2p_num(mac_vap_stru *pst_vap)
{
    mac_device_stru *pst_dev;

    pst_dev = mac_res_get_dev(pst_vap->uc_device_id);
    if (oal_unlikely(pst_dev == NULL)) {
        oam_error_log1(pst_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_inc_p2p_num::pst_dev[%d] null}", pst_vap->uc_device_id);
        return;
    }

    if (IS_P2P_DEV(pst_vap)) {
        /* device下sta个数加1 */
        pst_dev->st_p2p_info.uc_p2p_device_num++;
    } else if (IS_P2P_GO(pst_vap)) {
        pst_dev->st_p2p_info.uc_p2p_goclient_num++;
    } else if (IS_P2P_CL(pst_vap)) {
        pst_dev->st_p2p_info.uc_p2p_goclient_num++;
    }
}


uint32_t mac_vap_save_app_ie(mac_vap_stru *mac_vap, oal_app_ie_stru *pst_app_ie, en_app_ie_type_uint8 en_type)
{
    uint8_t           *puc_ie = NULL;
    uint32_t           ie_len;
    oal_app_ie_stru      st_tmp_app_ie;
    int32_t            l_ret;

    memset_s(&st_tmp_app_ie, sizeof(st_tmp_app_ie), 0, sizeof(st_tmp_app_ie));

    if (en_type >= OAL_APP_IE_NUM) {
        oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_CFG, "{mac_vap_save_app_ie::invalid en_type[%d].}", en_type);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    ie_len = pst_app_ie->ie_len;

    /* 如果输入WPS 长度为0， 则直接释放VAP 中资源 */
    if (ie_len == 0) {
        if (mac_vap->ast_app_ie[en_type].puc_ie != NULL) {
            oal_mem_free_m(mac_vap->ast_app_ie[en_type].puc_ie, OAL_TRUE);
        }

        mac_vap->ast_app_ie[en_type].puc_ie         = NULL;
        mac_vap->ast_app_ie[en_type].ie_len      = 0;
        mac_vap->ast_app_ie[en_type].ie_max_len  = 0;

        return OAL_SUCC;
    }

    /* 检查该类型的IE是否需要申请内存 */
    if ((mac_vap->ast_app_ie[en_type].ie_max_len < ie_len) ||
        (mac_vap->ast_app_ie[en_type].puc_ie == NULL)) {
        /* 这种情况不应该出现，维测需要 */
        if (mac_vap->ast_app_ie[en_type].puc_ie == NULL && mac_vap->ast_app_ie[en_type].ie_max_len != 0) {
            oam_error_log1(mac_vap->uc_vap_id, OAM_SF_CFG, "{mac_vap_save_app_ie::invalid len[%d].}",
                mac_vap->ast_app_ie[en_type].ie_max_len);
        }

        /* 如果以前的内存空间小于新信息元素需要的长度，则需要重新申请内存 */
        puc_ie = oal_mem_alloc_m(OAL_MEM_POOL_ID_LOCAL, (uint16_t)(ie_len), OAL_TRUE);
        if (puc_ie == NULL) {
            oam_warning_log2(mac_vap->uc_vap_id, OAM_SF_CFG, "{mac_vap_save_app_ie::LOCAL_MEM_POOL is empty!,\
                len[%d], en_type[%d].}", pst_app_ie->ie_len, en_type);
            return OAL_ERR_CODE_ALLOC_MEM_FAIL;
        }

        oal_mem_free_m(mac_vap->ast_app_ie[en_type].puc_ie, OAL_TRUE);

        mac_vap->ast_app_ie[en_type].puc_ie = puc_ie;
        mac_vap->ast_app_ie[en_type].ie_max_len = ie_len;
    }

    l_ret = memcpy_s((void *)mac_vap->ast_app_ie[en_type].puc_ie, mac_vap->ast_app_ie[en_type].ie_max_len,
        (void *)pst_app_ie->auc_ie, ie_len);
    if (l_ret != EOK) {
        oam_error_log0(0, OAM_SF_CFG, "mac_vap_save_app_ie::memcpy fail!");
        return OAL_FAIL;
    }

    mac_vap->ast_app_ie[en_type].ie_len = ie_len;
    if (is_not_p2p_listen_printf(mac_vap) == OAL_TRUE) {
        oam_warning_log4(mac_vap->uc_vap_id, OAM_SF_CFG, "{mac_vap_save_app_ie::IE:[0x%2x][0x%2x][0x%2x][0x%2x]}",
            *(mac_vap->ast_app_ie[en_type].puc_ie), *(mac_vap->ast_app_ie[en_type].puc_ie + BYTE_OFFSET_1),
            *(mac_vap->ast_app_ie[en_type].puc_ie + BYTE_OFFSET_2),
            *(mac_vap->ast_app_ie[en_type].puc_ie + ie_len - BYTE_OFFSET_1));
    }
    return OAL_SUCC;
}

uint32_t mac_vap_clear_app_ie(mac_vap_stru *pst_mac_vap, en_app_ie_type_uint8 en_type)
{
    if (en_type < OAL_APP_IE_NUM) {
        if (pst_mac_vap->ast_app_ie[en_type].puc_ie != NULL) {
            oal_mem_free_m(pst_mac_vap->ast_app_ie[en_type].puc_ie, OAL_TRUE);
            pst_mac_vap->ast_app_ie[en_type].puc_ie = NULL;
        }
        pst_mac_vap->ast_app_ie[en_type].ie_len = 0;
        pst_mac_vap->ast_app_ie[en_type].ie_max_len = 0;
    }

    return OAL_SUCC;
}


uint32_t mac_vap_exit(mac_vap_stru *vap)
{
    mac_device_stru               *mac_dev = NULL;
    uint8_t                      uc_index;

    if (oal_unlikely(vap == NULL)) {
        oam_error_log0(0, OAM_SF_ANY, "{mac_vap_exit::pst_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    vap->uc_init_flag = MAC_VAP_INVAILD;

    /* 释放vowifi相关内存 */
    mac_vap_vowifi_exit(vap);

    /* 释放WPS信息元素内存 */
    for (uc_index = 0; uc_index < OAL_APP_IE_NUM; uc_index++) {
        if (mac_vap_clear_app_ie(vap, uc_index) != OAL_SUCC) {
            oam_warning_log0(0, OAM_SF_ANY, "{mac_vap_exit::mac_vap_clear_app_ie failed.}");
        }
    }

    /* 业务vap已删除，从device上去掉 */
    mac_dev = mac_res_get_dev(vap->uc_device_id);
    if (oal_unlikely(mac_dev == NULL)) {
        oam_error_log1(vap->uc_vap_id, OAM_SF_ANY, "{mac_vap_exit::mac_dev[%d] null.}", vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 业务vap已经删除，从device中去掉 */
    for (uc_index = 0; uc_index < mac_dev->uc_vap_num; uc_index++) {
        /* 从device中找到vap id */
        if (mac_dev->auc_vap_id[uc_index] == vap->uc_vap_id) {
            /* 如果不是最后一个vap，则把最后一个vap id移动到这个位置，使得该数组是紧凑的 */
            if (uc_index < (mac_dev->uc_vap_num - 1)) {
                mac_dev->auc_vap_id[uc_index] = mac_dev->auc_vap_id[mac_dev->uc_vap_num - 1];
                break;
            }
        }
    }

    if (mac_dev->uc_vap_num != 0) {
        /* device下的vap总数减1 */
        mac_dev->uc_vap_num--;
    } else {
        oam_error_log1(vap->uc_vap_id, OAM_SF_CFG, "{mac_vap_exit::vap_num is 0. sta_num = %d}", mac_dev->uc_sta_num);
    }
    /* 清除数组中已删除的vap id，保证非零数组元素均为未删除vap */
    mac_dev->auc_vap_id[mac_dev->uc_vap_num] = 0;

    /* device下sta个数减1 */
    if (vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        if (mac_dev->uc_sta_num != 0) {
            mac_dev->uc_sta_num--;
        } else {
            oam_error_log1(vap->uc_vap_id, OAM_SF_CFG, "{mac_vap_exit::sta is 0. vap_num = %d}", mac_dev->uc_vap_num);
        }
    }

    mac_dec_p2p_num(vap);

    vap->en_protocol = WLAN_PROTOCOL_BUTT;

    /* 最后1个vap删除时，清除device级带宽信息 */
    if (mac_dev->uc_vap_num == 0) {
        mac_dev->uc_max_channel = 0;
        mac_dev->en_max_band = WLAN_BAND_BUTT;
        mac_dev->en_max_bandwidth = WLAN_BAND_WIDTH_BUTT;
    }

    /* 删除之后将vap的状态置位非法 */
    mac_vap_state_change(vap, MAC_VAP_STATE_BUTT);

    return OAL_SUCC;
}


uint32_t mac_vap_config_vht_ht_mib_by_protocol(mac_vap_stru *pst_mac_vap)
{
    if (pst_mac_vap->pst_mib_info == NULL) {
        oam_error_log3(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
                       "{mac_vap_config_vht_ht_mib_by_protocol::pst_mib_info null,  \
                       vap mode[%d] state[%d] user num[%d].}",
                       pst_mac_vap->en_vap_mode, pst_mac_vap->en_vap_state, pst_mac_vap->us_user_nums);
        return OAL_FAIL;
    }
    /* 根据协议模式更新 HT/VHT mib值 */
    if (pst_mac_vap->en_protocol == WLAN_HT_MODE || pst_mac_vap->en_protocol == WLAN_HT_ONLY_MODE) {
        mac_mib_set_HighThroughputOptionImplemented(pst_mac_vap, OAL_TRUE);
        mac_mib_set_VHTOptionImplemented(pst_mac_vap, OAL_FALSE);

#ifdef _PRE_WLAN_FEATURE_11AX
        if (g_wlan_spec_cfg->feature_11ax_is_open) {
            mac_mib_set_HEOptionImplemented(pst_mac_vap, OAL_FALSE);
        }
#endif
    } else if (pst_mac_vap->en_protocol == WLAN_VHT_MODE || pst_mac_vap->en_protocol == WLAN_VHT_ONLY_MODE) {
        mac_mib_set_HighThroughputOptionImplemented(pst_mac_vap, OAL_TRUE);
        mac_mib_set_VHTOptionImplemented(pst_mac_vap, OAL_TRUE);
#ifdef _PRE_WLAN_FEATURE_11AX
        if (g_wlan_spec_cfg->feature_11ax_is_open) {
            mac_mib_set_HEOptionImplemented(pst_mac_vap, OAL_FALSE);
        }
#endif
#ifdef _PRE_WLAN_FEATURE_11AX
    } else if (g_wlan_spec_cfg->feature_11ax_is_open && pst_mac_vap->en_protocol == WLAN_HE_MODE) {
        mac_mib_set_HighThroughputOptionImplemented(pst_mac_vap, OAL_TRUE);
        mac_mib_set_VHTOptionImplemented(pst_mac_vap, OAL_TRUE);
        mac_mib_set_HEOptionImplemented(pst_mac_vap, OAL_TRUE);
#endif
    } else {
        mac_mib_set_HighThroughputOptionImplemented(pst_mac_vap, OAL_FALSE);
        mac_mib_set_VHTOptionImplemented(pst_mac_vap, OAL_FALSE);
#ifdef _PRE_WLAN_FEATURE_11AX
        if (g_wlan_spec_cfg->feature_11ax_is_open) {
            mac_mib_set_HEOptionImplemented(pst_mac_vap, OAL_FALSE);
        }
#endif
    }

    if (!pst_mac_vap->en_vap_wmm) {
        mac_mib_set_HighThroughputOptionImplemented(pst_mac_vap, OAL_FALSE);
        mac_mib_set_VHTOptionImplemented(pst_mac_vap, OAL_FALSE);
    }

    return OAL_SUCC;
}


wlan_bw_cap_enum_uint8 mac_vap_bw_mode_to_bw(wlan_channel_bandwidth_enum_uint8 en_mode)
{
    OAL_STATIC wlan_bw_cap_enum_uint8 g_bw_mode_to_bw_table[] = {
        WLAN_BW_CAP_20M,  // WLAN_BAND_WIDTH_20M                 = 0,
        WLAN_BW_CAP_40M,  // WLAN_BAND_WIDTH_40PLUS              = 1,    /* 从20信道+1 */
        WLAN_BW_CAP_40M,  // WLAN_BAND_WIDTH_40MINUS             = 2,    /* 从20信道-1 */
        WLAN_BW_CAP_80M,  // WLAN_BAND_WIDTH_80PLUSPLUS          = 3,    /* 从20信道+1, 从40信道+1 */
        WLAN_BW_CAP_80M,  // WLAN_BAND_WIDTH_80PLUSMINUS         = 4,    /* 从20信道+1, 从40信道-1 */
        WLAN_BW_CAP_80M,  // WLAN_BAND_WIDTH_80MINUSPLUS         = 5,    /* 从20信道-1, 从40信道+1 */
        WLAN_BW_CAP_80M,  // WLAN_BAND_WIDTH_80MINUSMINUS        = 6,    /* 从20信道-1, 从40信道-1 */
#ifdef _PRE_WLAN_FEATURE_160M
        WLAN_BW_CAP_160M,  // WLAN_BAND_WIDTH_160PLUSPLUSPLUS,            /* 从20信道+1, 从40信道+1, 从80信道+1 */
        WLAN_BW_CAP_160M,  // WLAN_BAND_WIDTH_160PLUSPLUSMINUS,           /* 从20信道+1, 从40信道+1, 从80信道-1 */
        WLAN_BW_CAP_160M,  // WLAN_BAND_WIDTH_160PLUSMINUSPLUS,           /* 从20信道+1, 从40信道-1, 从80信道+1 */
        WLAN_BW_CAP_160M,  // WLAN_BAND_WIDTH_160PLUSMINUSMINUS,          /* 从20信道+1, 从40信道-1, 从80信道-1 */
        WLAN_BW_CAP_160M,  // WLAN_BAND_WIDTH_160MINUSPLUSPLUS,           /* 从20信道-1, 从40信道+1, 从80信道+1 */
        WLAN_BW_CAP_160M,  // WLAN_BAND_WIDTH_160MINUSPLUSMINUS,          /* 从20信道-1, 从40信道+1, 从80信道-1 */
        WLAN_BW_CAP_160M,  // WLAN_BAND_WIDTH_160MINUSMINUSPLUS,          /* 从20信道-1, 从40信道-1, 从80信道+1 */
        WLAN_BW_CAP_160M,  // WLAN_BAND_WIDTH_160MINUSMINUSMINUS,         /* 从20信道-1, 从40信道-1, 从80信道-1 */
#endif

        WLAN_BW_CAP_40M,  // WLAN_BAND_WIDTH_40M,
        WLAN_BW_CAP_80M,  // WLAN_BAND_WIDTH_80M,
    };

    if (en_mode >= WLAN_BAND_WIDTH_BUTT) {
        oam_error_log1(0, OAM_SF_ANY, "mac_vap_bw_mode_to_bw::invalid en_mode = %d, force 20M", en_mode);
        en_mode = 0;
    }

    return g_bw_mode_to_bw_table[en_mode];
}



uint32_t mac_vap_set_bssid(mac_vap_stru *pst_mac_vap, uint8_t *puc_bssid, uint8_t bssid_len)
{
    if (EOK != memcpy_s(pst_mac_vap->auc_bssid, WLAN_MAC_ADDR_LEN, puc_bssid, WLAN_MAC_ADDR_LEN)) {
        oam_error_log0(0, OAM_SF_ANY, "mac_vap_set_bssid::memcpy fail!");
        return OAL_FAIL;
    }
    return OAL_SUCC;
}



void mac_vap_state_change(mac_vap_stru *pst_mac_vap, mac_vap_state_enum_uint8 en_vap_state)
{
    pst_mac_vap->vap_last_state = pst_mac_vap->en_vap_state;
    pst_mac_vap->en_vap_state = en_vap_state;
    if (is_not_p2p_listen_printf(pst_mac_vap) == OAL_TRUE) {
        oam_warning_log2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{mac_vap_state_change:from[%d]to[%d]}", pst_mac_vap->vap_last_state, en_vap_state);
    }
}


void mac_vap_get_bandwidth_cap(mac_vap_stru *mac_vap, wlan_bw_cap_enum_uint8 *pen_cap)
{
    wlan_bw_cap_enum_uint8 en_band_cap = WLAN_BW_CAP_20M;

    if (WLAN_BAND_WIDTH_40PLUS == mac_vap->st_channel.en_bandwidth ||
        WLAN_BAND_WIDTH_40MINUS == mac_vap->st_channel.en_bandwidth) {
        en_band_cap = WLAN_BW_CAP_40M;
    } else if (WLAN_BAND_WIDTH_80PLUSPLUS <= mac_vap->st_channel.en_bandwidth &&
               mac_vap->st_channel.en_bandwidth <= WLAN_BAND_WIDTH_80MINUSMINUS) {
        en_band_cap = WLAN_BW_CAP_80M;
    }
#ifdef _PRE_WLAN_FEATURE_160M
    if (WLAN_BAND_WIDTH_160PLUSPLUSPLUS <= mac_vap->st_channel.en_bandwidth &&
        mac_vap->st_channel.en_bandwidth <= WLAN_BAND_WIDTH_160MINUSMINUSMINUS) {
        en_band_cap = WLAN_BW_CAP_160M;
    }
#endif

    *pen_cap = en_band_cap;
}

wlan_channel_bandwidth_enum_uint8 mac_vap_cap_40m_get_bandwith(wlan_channel_bandwidth_enum_uint8 bss_bandwith)
{
    wlan_channel_bandwidth_enum_uint8 band_with = WLAN_BAND_WIDTH_20M;

    if (bss_bandwith <= WLAN_BAND_WIDTH_40MINUS) {
        band_with = bss_bandwith;
    } else if ((WLAN_BAND_WIDTH_80PLUSPLUS <= bss_bandwith) && (bss_bandwith <= WLAN_BAND_WIDTH_80PLUSMINUS)) {
        band_with = WLAN_BAND_WIDTH_40PLUS;
    } else if ((WLAN_BAND_WIDTH_80MINUSPLUS <= bss_bandwith) && (bss_bandwith <= WLAN_BAND_WIDTH_80MINUSMINUS)) {
        band_with = WLAN_BAND_WIDTH_40MINUS;
    }
#ifdef _PRE_WLAN_FEATURE_160M
    if ((WLAN_BAND_WIDTH_160PLUSPLUSPLUS <= bss_bandwith) && (bss_bandwith <= WLAN_BAND_WIDTH_160PLUSMINUSMINUS)) {
        band_with = WLAN_BAND_WIDTH_40PLUS;
    } else if ((WLAN_BAND_WIDTH_160MINUSPLUSPLUS <= bss_bandwith) &&
        (bss_bandwith <= WLAN_BAND_WIDTH_160MINUSMINUSMINUS)) {
        band_with = WLAN_BAND_WIDTH_40MINUS;
    }
#endif
    return band_with;
}

wlan_channel_bandwidth_enum_uint8 mac_vap_cap_80m_get_bandwith(wlan_channel_bandwidth_enum_uint8 bss_bandwith)
{
    wlan_channel_bandwidth_enum_uint8 band_with = WLAN_BAND_WIDTH_20M;

    if (bss_bandwith <= WLAN_BAND_WIDTH_80MINUSMINUS) {
        band_with = bss_bandwith;
    }
#ifdef _PRE_WLAN_FEATURE_160M
    if ((WLAN_BAND_WIDTH_160PLUSPLUSPLUS <= bss_bandwith) && (bss_bandwith <= WLAN_BAND_WIDTH_160PLUSPLUSMINUS)) {
        band_with = WLAN_BAND_WIDTH_80PLUSPLUS;
    } else if ((WLAN_BAND_WIDTH_160PLUSMINUSPLUS <= bss_bandwith) &&
        (bss_bandwith <= WLAN_BAND_WIDTH_160PLUSMINUSMINUS)) {
        band_with = WLAN_BAND_WIDTH_80PLUSMINUS;
    } else if ((WLAN_BAND_WIDTH_160MINUSPLUSPLUS <= bss_bandwith) &&
        (bss_bandwith <= WLAN_BAND_WIDTH_160MINUSPLUSMINUS)) {
        band_with = WLAN_BAND_WIDTH_80MINUSPLUS;
    } else if ((WLAN_BAND_WIDTH_160MINUSMINUSPLUS <= bss_bandwith) &&
        (bss_bandwith <= WLAN_BAND_WIDTH_160MINUSMINUSMINUS)) {
        band_with = WLAN_BAND_WIDTH_80MINUSMINUS;
    }
#endif

    return band_with;
}


uint8_t mac_vap_get_bandwith(wlan_bw_cap_enum_uint8 en_dev_cap, wlan_channel_bandwidth_enum_uint8 en_bss_cap)
{
    wlan_channel_bandwidth_enum_uint8 en_band_with = WLAN_BAND_WIDTH_20M;

    if (en_bss_cap >= WLAN_BAND_WIDTH_BUTT) {
        oam_error_log2(0, OAM_SF_ANY, "mac_vap_get_bandwith:bss cap is invaild en_dev_cap[%d] to en_bss_cap[%d]",
            en_dev_cap, en_bss_cap);
        return en_band_with;
    }

    switch (en_dev_cap) {
        case WLAN_BW_CAP_20M:
            break;
        case WLAN_BW_CAP_40M:
            en_band_with = mac_vap_cap_40m_get_bandwith(en_bss_cap);
            break;
        case WLAN_BW_CAP_80M:
            en_band_with = mac_vap_cap_80m_get_bandwith(en_bss_cap);
            break;
#ifdef _PRE_WLAN_FEATURE_160M
        case WLAN_BW_CAP_160M:
            if (en_bss_cap < WLAN_BAND_WIDTH_BUTT) {
                en_band_with = en_bss_cap;
            }
#endif
            break;
        default:
            oam_error_log2(0, OAM_SF_ANY,
                "mac_vap_get_bandwith: bandwith en_dev_cap[%d] to en_bss_cap[%d]", en_dev_cap, en_bss_cap);
            break;
    }

    return en_band_with;
}


void mac_vap_set_rifs_tx_on(mac_vap_stru *pst_vap, uint8_t uc_value)
{
    pst_vap->st_cap_flag.bit_rifs_tx_on = uc_value;
}



void mac_vap_set_11ac2g(mac_vap_stru *pst_vap, uint8_t uc_value)
{
    pst_vap->st_cap_flag.bit_11ac2g = uc_value;
}



uint32_t mac_vap_set_current_channel(mac_vap_stru *pst_vap, wlan_channel_band_enum_uint8 en_band,
    uint8_t uc_channel, uint8_t is_6ghz)
{
    uint8_t  uc_channel_idx = 0;
    uint32_t ret;

    /* 检查信道号 */
    ret = mac_is_channel_num_valid(en_band, uc_channel, is_6ghz);
    if (ret != OAL_SUCC) {
        return ret;
    }

    /* 根据信道号找到索引号 */
    ret = mac_get_channel_idx_from_num(en_band, uc_channel, is_6ghz, &uc_channel_idx);
    if (ret != OAL_SUCC) {
        return ret;
    }

    pst_vap->st_channel.uc_chan_number = uc_channel;
    pst_vap->st_channel.en_band = en_band;
    pst_vap->st_channel.uc_chan_idx = uc_channel_idx;
    pst_vap->st_channel.ext6g_band = is_6ghz;

    return OAL_SUCC;
}



oal_bool_enum_uint8 mac_vap_check_bss_cap_info_phy_ap(uint16_t us_cap_info, mac_vap_stru *pst_mac_vap)
{
    mac_cap_info_stru *pst_cap_info = (mac_cap_info_stru *)(&us_cap_info);

    if (pst_mac_vap->st_channel.en_band != WLAN_BAND_2G) {
        return OAL_TRUE;
    }

    /* PBCC */
    if ((OAL_FALSE == mac_mib_get_PBCCOptionImplemented(pst_mac_vap)) &&
        (pst_cap_info->bit_pbcc == 1)) {
        oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                         "{mac_vap_check_bss_cap_info_phy_ap::PBCC is different.}");
    }

    /* Channel Agility */
    if ((OAL_FALSE == mac_mib_get_ChannelAgilityPresent(pst_mac_vap)) &&
        (pst_cap_info->bit_channel_agility == 1)) {
        oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                         "{mac_vap_check_bss_cap_info_phy_ap::Channel Agility is different.}");
    }

    /* DSSS-OFDM Capabilities */
    if ((OAL_FALSE == mac_mib_get_DSSSOFDMOptionActivated(pst_mac_vap)) &&
        (pst_cap_info->bit_dsss_ofdm == 1)) {
        oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                         "{mac_vap_check_bss_cap_info_phy_ap::DSSS-OFDM Capabilities is different.}");
    }

    return OAL_TRUE;
}


uint32_t mac_dump_protection(mac_vap_stru *pst_mac_vap, uint8_t *puc_param)
{
    mac_h2d_protection_stru     *pst_h2d_prot = NULL;
    mac_protection_stru         *pst_protection = NULL;

    if (puc_param == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_h2d_prot = (mac_h2d_protection_stru *)puc_param;
    pst_protection = &pst_h2d_prot->st_protection;

    oam_info_log4(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                  "11RIFSMode=%d, LSIGTXOPFullProtectionActivated=%d, NonGFEntitiesPresent=%d, protection_mode=%d\r\n",
                  pst_h2d_prot->en_dot11RIFSMode, pst_h2d_prot->en_dot11LSIGTXOPFullProtectionActivated,
                  pst_h2d_prot->en_dot11NonGFEntitiesPresent, pst_protection->en_protection_mode);
    oam_info_log4(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                  "non_erp_aging_cnt=%d, non_ht_aging_cnt=%d, auto_protection=%d, non_erp_present=%d\r\n",
                  pst_protection->uc_obss_non_erp_aging_cnt, pst_protection->uc_obss_non_ht_aging_cnt,
                  pst_protection->bit_auto_protection, pst_protection->bit_obss_non_erp_present);
    oam_info_log4(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                  "non_ht_present=%d, rts_cts_protect_mode=%d, lsig_txop_protect_mode=%d, sta_no_short_slot_num=%d\r\n",
                  pst_protection->bit_obss_non_ht_present, pst_protection->bit_rts_cts_protect_mode,
                  pst_protection->bit_lsig_txop_protect_mode, pst_protection->uc_sta_no_short_slot_num);
    return OAL_SUCC;
}


wlan_prot_mode_enum_uint8 mac_vap_get_user_protection_mode(mac_vap_stru *pst_mac_vap_sta,
    mac_user_stru *pst_mac_user)
{
    wlan_prot_mode_enum_uint8 en_protection_mode = WLAN_PROT_NO;

    if (oal_any_null_ptr2(pst_mac_vap_sta, pst_mac_user)) {
        return en_protection_mode;
    }

    /* 在2G频段下，如果AP发送的beacon帧ERP ie中Use Protection bit置为1，则将保护级别设置为ERP保护 */
    if ((pst_mac_vap_sta->st_channel.en_band == WLAN_BAND_2G) &&
        (pst_mac_user->st_cap_info.bit_erp_use_protect == OAL_TRUE)) {
        en_protection_mode = WLAN_PROT_ERP;
    } else if ((pst_mac_user->st_ht_hdl.bit_ht_protection == WLAN_MIB_HT_NON_HT_MIXED) ||
        (pst_mac_user->st_ht_hdl.bit_ht_protection == WLAN_MIB_HT_NONMEMBER_PROTECTION)) {
        /* 如果AP发送的beacon帧ht operation ie中ht protection字段为mixed或non-member，则将保护级别设置为HT保护 */
        en_protection_mode = WLAN_PROT_HT;
    } else if (pst_mac_user->st_ht_hdl.bit_nongf_sta_present == OAL_TRUE) {
        /* 如果AP发送的beacon帧ht operation ie中non-gf sta present字段为1，则将保护级别设置为GF保护 */
        en_protection_mode = WLAN_PROT_GF;
    } else {
        /* 剩下的情况不做保护 */
        en_protection_mode = WLAN_PROT_NO;
    }

    return en_protection_mode;
}

oal_bool_enum mac_protection_lsigtxop_check(mac_vap_stru *pst_mac_vap)
{
    mac_user_stru *pst_mac_user = NULL;

    /* 如果不是11n站点，则不支持lsigtxop保护 */
    if ((pst_mac_vap->en_protocol != WLAN_HT_MODE) && (pst_mac_vap->en_protocol != WLAN_HT_ONLY_MODE) &&
        (pst_mac_vap->en_protocol != WLAN_HT_11G_MODE)) {
        return OAL_FALSE;
    }

    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        pst_mac_user = (mac_user_stru *)mac_res_get_mac_user(pst_mac_vap->us_assoc_vap_id); /* user保存的是AP的信息 */
        if ((pst_mac_user == NULL) || (pst_mac_user->st_ht_hdl.bit_lsig_txop_protection_full_support == OAL_FALSE)) {
            return OAL_FALSE;
        } else {
            return OAL_TRUE;
        }
    }
    /* lint -e644 */
    /* BSS 中所有站点都支持Lsig txop protection, 则使用Lsig txop protection机制，开销小, AP和STA采用不同的判断 */
    if ((pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) &&
        (OAL_TRUE == mac_mib_get_LsigTxopFullProtectionActivated(pst_mac_vap))) {
        return OAL_TRUE;
    } else {
        return OAL_FALSE;
    }
    /* lint +e644 */
}

void mac_protection_set_lsig_txop_mechanism(mac_vap_stru *pst_mac_vap, oal_switch_enum_uint8 en_flag)
{
    /* 数据帧/管理帧发送时候，需要根据bit_lsig_txop_protect_mode值填写发送描述符中的L-SIG TXOP enable位 */
    pst_mac_vap->st_protection.bit_lsig_txop_protect_mode = en_flag;
    oam_warning_log1(0, OAM_SF_CFG, "mac_protection_set_lsig_txop_mechanism:on[%d]?", en_flag);
}


uint32_t mac_vap_add_wep_key(mac_vap_stru *pst_mac_vap, uint8_t us_key_idx,
    uint8_t uc_key_len, uint8_t *puc_key)
{
    mac_user_stru                   *pst_multi_user        = NULL;
    wlan_priv_key_param_stru        *pst_wep_key           = NULL;
    uint32_t                       cipher_type        = WLAN_CIPHER_SUITE_WEP40;
    uint8_t                        uc_wep_cipher_type    = WLAN_80211_CIPHER_SUITE_WEP_40;
    int32_t                        l_ret;

    /* wep 密钥最大为4个 */
    if (us_key_idx >= WLAN_MAX_WEP_KEY_COUNT) {
        return OAL_ERR_CODE_SECURITY_KEY_ID;
    }

    switch (uc_key_len) {
        case WLAN_WEP40_KEY_LEN:
            uc_wep_cipher_type = WLAN_80211_CIPHER_SUITE_WEP_40;
            cipher_type = WLAN_CIPHER_SUITE_WEP40;
            break;
        case WLAN_WEP104_KEY_LEN:
            uc_wep_cipher_type = WLAN_80211_CIPHER_SUITE_WEP_104;
            cipher_type = WLAN_CIPHER_SUITE_WEP104;
            break;
        default:
            return OAL_ERR_CODE_SECURITY_KEY_LEN;
    }

    /* WEP密钥信息记录到组播用户中 */
    pst_multi_user = mac_res_get_mac_user(pst_mac_vap->us_multi_user_idx);
    if (pst_multi_user == NULL) {
        return OAL_ERR_CODE_SECURITY_USER_INVAILD;
    }
    mac_mib_set_privacyinvoked(pst_mac_vap, OAL_TRUE);

    /* 初始化组播用户的安全信息 */
    pst_multi_user->st_key_info.en_cipher_type     = uc_wep_cipher_type;
    pst_multi_user->st_key_info.uc_default_index   = us_key_idx;
    pst_multi_user->st_key_info.uc_igtk_key_index  = 0xff;      /* wep时设置为无效 */
    pst_multi_user->st_key_info.bit_gtk            = 0;

    pst_wep_key = &pst_multi_user->st_key_info.ast_key[us_key_idx];

    pst_wep_key->cipher        = cipher_type;
    pst_wep_key->key_len       = (uint32_t)uc_key_len;

    l_ret = memcpy_s(pst_wep_key->auc_key, WLAN_WPA_KEY_LEN, puc_key, uc_key_len);
    if (l_ret != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "mac_vap_add_wep_key::memcpy fail!");
        return OAL_FAIL;
    }

    /* 初始化组播用户的发送信息 */
    pst_multi_user->st_user_tx_info.st_security.en_cipher_key_type = us_key_idx + HAL_KEY_TYPE_PTK;
    pst_multi_user->st_user_tx_info.st_security.en_cipher_protocol_type = uc_wep_cipher_type;

    return OAL_SUCC;
}


mac_user_stru *mac_vap_get_user_by_addr(mac_vap_stru *pst_mac_vap, uint8_t *puc_mac_addr)
{
    uint32_t              ret;
    uint16_t              us_user_idx  = 0xffff;
    mac_user_stru          *pst_mac_user = NULL;

    /* 根据mac addr找到sta索引 */
    ret = mac_vap_find_user_by_macaddr(pst_mac_vap, puc_mac_addr, &us_user_idx);
    if (ret != OAL_SUCC) {
        oam_warning_log1(0, OAM_SF_ANY, "{mac_vap_get_user_by_addr::find_user_by_macaddr failed[%d].}", ret);
        if (puc_mac_addr != NULL) {
            oam_warning_log3(0, OAM_SF_ANY, "{mac_vap_get_user_by_addr::\
                mac[%x:XX:XX:XX:%x:%x] cant be found!}",puc_mac_addr[BYTE_OFFSET_0],
                puc_mac_addr[BYTE_OFFSET_4], puc_mac_addr[BYTE_OFFSET_5]);
        }
        return NULL;
    }

    /* 根据sta索引找到user内存区域 */
    pst_mac_user = (mac_user_stru *)mac_res_get_mac_user(us_user_idx);
    if (pst_mac_user == NULL) {
        oam_warning_log1(0, OAM_SF_ANY, "{mac_vap_get_user_by_addr::user[%d] ptr null.}", us_user_idx);
    }

    return pst_mac_user;
}

OAL_STATIC uint32_t mac_vap_set_rsn_security(mac_vap_stru *mac_vap, mac_beacon_param_stru *beacon_param,
    mac_crypto_settings_stru             *crypto)
{
    uint32_t ret;
    uint16_t rsn_cap;
    mac_vap->st_cap_flag.bit_wpa2 = OAL_TRUE;
    mac_mib_set_rsnaactivated(mac_vap, OAL_TRUE);
    ret = mac_ie_get_rsn_cipher(beacon_param->auc_rsn_ie, crypto);
    if (ret != OAL_SUCC) {
        return ret;
    }
    rsn_cap = mac_get_rsn_capability(beacon_param->auc_rsn_ie);
    mac_mib_set_rsn_group_suite(mac_vap, crypto->group_suite);
    mac_mib_set_rsn_pair_suites(mac_vap, crypto->aul_pair_suite);
    mac_mib_set_rsn_akm_suites(mac_vap, crypto->aul_akm_suite);
    mac_mib_set_rsn_group_mgmt_suite(mac_vap, crypto->group_mgmt_suite);
    /* RSN 能力 */
    mac_mib_set_dot11RSNAMFPR(mac_vap, (rsn_cap & BIT6) ? OAL_TRUE : OAL_FALSE);
    mac_mib_set_dot11RSNAMFPC(mac_vap, (rsn_cap & BIT7) ? OAL_TRUE : OAL_FALSE);
    mac_mib_set_pre_auth_actived(mac_vap, rsn_cap & BIT0);
    mac_mib_set_rsnacfg_ptksareplaycounters(mac_vap, (uint8_t)(rsn_cap & 0x0C) >> BYTE_OFFSET_2);
    mac_mib_set_rsnacfg_gtksareplaycounters(mac_vap, (uint8_t)(rsn_cap & 0x30) >> BYTE_OFFSET_4);

    return OAL_SUCC;
}


uint32_t mac_vap_set_security(mac_vap_stru *pst_mac_vap, mac_beacon_param_stru *pst_beacon_param)
{
    mac_user_stru                        *pst_multi_user = NULL;
    mac_crypto_settings_stru              st_crypto;
    uint32_t                            ret;

    if (oal_any_null_ptr2(pst_mac_vap, pst_beacon_param)) {
        oam_error_log0(0, OAM_SF_WPA, "{mac_vap_add_beacon::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 清除之前的加密配置信息 */
    mac_mib_set_privacyinvoked(pst_mac_vap, OAL_FALSE);
    mac_mib_set_rsnaactivated(pst_mac_vap, OAL_FALSE);
    mac_mib_init_rsnacfg_suites(pst_mac_vap);
    pst_mac_vap->st_cap_flag.bit_wpa = OAL_FALSE;
    pst_mac_vap->st_cap_flag.bit_wpa2 = OAL_FALSE;
    mac_mib_set_dot11RSNAMFPR(pst_mac_vap, OAL_FALSE);
    mac_mib_set_dot11RSNAMFPC(pst_mac_vap, OAL_FALSE);

    /* 清除组播密钥信息 */
    pst_multi_user = mac_res_get_mac_user(pst_mac_vap->us_multi_user_idx);
    if (pst_multi_user == NULL) {
        oam_warning_log1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{mac_vap_add_beacon::pst_multi_user[%d] null.}",
                         pst_mac_vap->us_multi_user_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_beacon_param->en_privacy == OAL_FALSE) {
        /* 只在非加密场景下清除，加密场景会重新设置覆盖 */
        mac_user_init_key(pst_multi_user);
        pst_multi_user->st_user_tx_info.st_security.en_cipher_key_type = WLAN_KEY_TYPE_TX_GTK;
        return OAL_SUCC;
    }

    /* 使能加密 */
    mac_mib_set_privacyinvoked(pst_mac_vap, OAL_TRUE);

    memset_s(&st_crypto, sizeof(mac_crypto_settings_stru), 0, sizeof(mac_crypto_settings_stru));

    if (pst_beacon_param->auc_wpa_ie[0] == MAC_EID_VENDOR) {
        pst_mac_vap->st_cap_flag.bit_wpa = OAL_TRUE;
        mac_mib_set_rsnaactivated(pst_mac_vap, OAL_TRUE);
        mac_ie_get_wpa_cipher(pst_beacon_param->auc_wpa_ie, &st_crypto);
        mac_mib_set_wpa_group_suite(pst_mac_vap, st_crypto.group_suite);
        mac_mib_set_wpa_pair_suites(pst_mac_vap, st_crypto.aul_pair_suite);
        mac_mib_set_wpa_akm_suites(pst_mac_vap, st_crypto.aul_akm_suite);
    }

    if (pst_beacon_param->auc_rsn_ie[0] == MAC_EID_RSN) {
        ret = mac_vap_set_rsn_security(pst_mac_vap, pst_beacon_param, &st_crypto);
        if (ret != OAL_SUCC) {
            return ret;
        }
    }

    return OAL_SUCC;
}

uint32_t mac_vap_add_key(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user,
    uint8_t uc_key_id, mac_key_params_stru *pst_key)
{
    uint32_t ret;

    switch ((uint8_t)pst_key->cipher) {
        case WLAN_80211_CIPHER_SUITE_WEP_40:
        case WLAN_80211_CIPHER_SUITE_WEP_104:
            /* 设置mib */
            mac_mib_set_privacyinvoked(pst_mac_vap, OAL_TRUE);
            mac_mib_set_rsnaactivated(pst_mac_vap, OAL_FALSE);
            // 设置组播密钥套件应该放在set default key
            ret = mac_user_add_wep_key(pst_mac_user, uc_key_id, pst_key);
            break;
        case WLAN_80211_CIPHER_SUITE_TKIP:
        case WLAN_80211_CIPHER_SUITE_CCMP:
        case WLAN_80211_CIPHER_SUITE_GCMP:
        case WLAN_80211_CIPHER_SUITE_GCMP_256:
        case WLAN_80211_CIPHER_SUITE_CCMP_256:
            ret = mac_user_add_rsn_key(pst_mac_user, uc_key_id, pst_key);
            break;
        case WLAN_80211_CIPHER_SUITE_BIP:
        case WLAN_80211_CIPHER_SUITE_BIP_GMAC_128:
        case WLAN_80211_CIPHER_SUITE_BIP_GMAC_256:
        case WLAN_80211_CIPHER_SUITE_BIP_CMAC_256:
            ret = mac_user_add_bip_key(pst_mac_user, uc_key_id, pst_key);
            break;
        default:
            return OAL_ERR_CODE_SECURITY_CHIPER_TYPE;
    }

    return ret;
}


uint8_t mac_vap_get_default_key_id(mac_vap_stru *pst_mac_vap)
{
    mac_user_stru                *pst_multi_user;
    uint8_t                     uc_default_key_id;

    /* 根据索引，从组播用户密钥信息中查找密钥 */
    pst_multi_user = mac_res_get_mac_user(pst_mac_vap->us_multi_user_idx);
    if (pst_multi_user == NULL) {
        oam_warning_log1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                         "{mac_vap_get_default_key_id::multi_user[%d] NULL}",
                         pst_mac_vap->us_multi_user_idx);
        return 0;
    }

    if ((pst_multi_user->st_key_info.en_cipher_type != WLAN_80211_CIPHER_SUITE_WEP_40) &&
        (pst_multi_user->st_key_info.en_cipher_type != WLAN_80211_CIPHER_SUITE_WEP_104)) {
        oam_error_log1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                       "{mac_vap_get_default_key_id::unexpectd cipher_type[%d]}",
                       pst_multi_user->st_key_info.en_cipher_type);
        return 0;
    }
    uc_default_key_id = pst_multi_user->st_key_info.uc_default_index;
    if (uc_default_key_id >= WLAN_NUM_TK) {
        oam_error_log1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                       "{mac_vap_get_default_key_id::unexpectd keyid[%d]}",
                       uc_default_key_id);
        return 0;
    }

    return uc_default_key_id;
}


uint32_t mac_vap_set_default_wep_key(mac_vap_stru *pst_mac_vap, uint8_t uc_key_index)
{
    wlan_priv_key_param_stru     *pst_wep_key = NULL;
    mac_user_stru                *pst_multi_user = NULL;

    /* 1.1 如果非wep 加密，则直接返回 */
    if (OAL_TRUE != mac_is_wep_enabled(pst_mac_vap)) {
        return OAL_SUCC;
    }

    /* 2.1 根据索引，从组播用户密钥信息中查找密钥 */
    pst_multi_user = mac_res_get_mac_user(pst_mac_vap->us_multi_user_idx);
    if (pst_multi_user == NULL) {
        return OAL_ERR_CODE_SECURITY_USER_INVAILD;
    }
    pst_wep_key = &pst_multi_user->st_key_info.ast_key[uc_key_index];

    if (pst_wep_key->cipher != WLAN_CIPHER_SUITE_WEP40 &&
        pst_wep_key->cipher != WLAN_CIPHER_SUITE_WEP104) {
        return OAL_ERR_CODE_SECURITY_CHIPER_TYPE;
    }

    /* 3.1 更新密钥类型及default id */
    pst_multi_user->st_key_info.en_cipher_type     = (uint8_t)(pst_wep_key->cipher);
    pst_multi_user->st_key_info.uc_default_index   = uc_key_index;

    /* 4.1 设置mib属性 */
    mac_set_wep_default_keyid(pst_mac_vap, uc_key_index);

    return OAL_SUCC;
}


uint32_t mac_vap_set_default_mgmt_key(mac_vap_stru *pst_mac_vap, uint8_t uc_key_index)
{
    mac_user_stru *pst_multi_user;

    /* 管理帧加密信息保存在组播用户中 */
    pst_multi_user = mac_res_get_mac_user(pst_mac_vap->us_multi_user_idx);
    if (pst_multi_user == NULL) {
        return OAL_ERR_CODE_SECURITY_USER_INVAILD;
    }

    /* keyid校验 */
    if (uc_key_index < WLAN_NUM_TK || uc_key_index > WLAN_MAX_IGTK_KEY_INDEX) {
        return OAL_ERR_CODE_SECURITY_KEY_ID;
    }

    switch ((uint8_t)pst_multi_user->st_key_info.ast_key[uc_key_index].cipher) {
        case WLAN_80211_CIPHER_SUITE_BIP:
        case WLAN_80211_CIPHER_SUITE_BIP_GMAC_128:
        case WLAN_80211_CIPHER_SUITE_BIP_GMAC_256:
        case WLAN_80211_CIPHER_SUITE_BIP_CMAC_256:
            /* 更新IGTK的keyid */
            pst_multi_user->st_key_info.uc_igtk_key_index = uc_key_index;
            break;
        default:
            return OAL_ERR_CODE_SECURITY_CHIPER_TYPE;
    }

    return OAL_SUCC;
}


void mac_vap_init_user_security_port(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    if (OAL_TRUE != mac_mib_get_rsnaactivated(pst_mac_vap)) {
        mac_user_set_port(pst_mac_user, OAL_TRUE);
        return;
    }
}


uint8_t *mac_vap_get_mac_addr(mac_vap_stru *pst_mac_vap)
{
    if (IS_P2P_DEV(pst_mac_vap)) {
        /* 获取P2P DEV MAC 地址，赋值到probe req 帧中 */
        return mac_mib_get_p2p0_dot11StationID(pst_mac_vap);
    } else {
        /* 设置地址2为自己的MAC地址 */
        return mac_mib_get_StationID(pst_mac_vap);
    }
}


oal_switch_enum_uint8 mac_vap_protection_autoprot_is_enabled(mac_vap_stru *pst_mac_vap)
{
    return pst_mac_vap->st_protection.bit_auto_protection;
}

oal_bool_enum_uint8 is_not_p2p_listen_printf(mac_vap_stru *mac_vap)
{
    if (g_feature_switch[HMAC_MIRACAST_SINK_SWITCH] == OAL_TRUE) {
        return (mac_vap->en_vap_state != MAC_VAP_STATE_STA_LISTEN &&
            mac_vap->vap_last_state != MAC_VAP_STATE_STA_LISTEN);
    }
    return OAL_TRUE;
}

oal_bool_enum mac_vap_is_up(mac_vap_stru *mac_vap)
{
    if (mac_vap == NULL) {
        return OAL_FALSE;
    }
    return ((mac_vap->en_vap_state == MAC_VAP_STATE_UP)
            || (mac_vap->en_vap_state == MAC_VAP_STATE_PAUSE)
            || (mac_vap->en_vap_state == MAC_VAP_STATE_ROAMING)
            || (mac_vap->en_vap_state == MAC_VAP_STATE_STA_LISTEN && mac_vap->us_user_nums > 0));
}


#ifdef _PRE_WLAN_CHBA_MGMT

oal_bool_enum mac_is_chba_mode(const mac_vap_stru *mac_vap)
{
    if ((g_optimized_feature_switch_bitmap & BIT(CUSTOM_OPTIMIZE_CHBA)) == OAL_FALSE) {
        return OAL_FALSE;
    }

    if (mac_vap == NULL) {
        return OAL_FALSE;
    }

    return (mac_vap->chba_mode == CHBA_MODE);
}
#endif
