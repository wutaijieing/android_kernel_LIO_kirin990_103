
#include "hmac_chan_meas.h"
#include "hmac_resource.h"
#include "hmac_scan.h"
#include "hmac_chba_function.h"
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "oal_cfg80211.h"
#endif
#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHAN_MEAS_C

hmac_chan_stru g_aus_channel_candidate_list_2g[HMAC_CHANNEL_NUM_2G] = {
    // idx  chan_idx  chan_num   bandwidth
    {0,     0,      1,    WLAN_BAND_WIDTH_20M},
    {1,     1,      2,    WLAN_BAND_WIDTH_20M},
    {2,     2,      3,    WLAN_BAND_WIDTH_20M},
    {3,     3,      4,    WLAN_BAND_WIDTH_20M},
    {4,     4,      5,    WLAN_BAND_WIDTH_20M},
    {5,     5,      6,    WLAN_BAND_WIDTH_20M},
    {6,     6,      7,    WLAN_BAND_WIDTH_20M},
    {7,     7,      8,    WLAN_BAND_WIDTH_20M},
    {8,     8,      9,    WLAN_BAND_WIDTH_20M},
    {9,     9,      10,   WLAN_BAND_WIDTH_20M},
    {10,    10,     11,   WLAN_BAND_WIDTH_20M},
    {11,    11,     12,   WLAN_BAND_WIDTH_20M},
    {12,    12,     13,   WLAN_BAND_WIDTH_20M},
    {13,    13,     14,   WLAN_BAND_WIDTH_20M},
    {14,    0,      1,    WLAN_BAND_WIDTH_40PLUS},
    {15,    4,      5,    WLAN_BAND_WIDTH_40PLUS},
    {16,    6,      7,    WLAN_BAND_WIDTH_40PLUS},
    {17,    8,      9,    WLAN_BAND_WIDTH_40PLUS},
    {18,    1,      2,    WLAN_BAND_WIDTH_40PLUS},
    {19,    5,      6,    WLAN_BAND_WIDTH_40PLUS},
    {20,    7,      8,    WLAN_BAND_WIDTH_40PLUS},
    {21,    12,     13,   WLAN_BAND_WIDTH_40MINUS},
    {22,    4,      5,    WLAN_BAND_WIDTH_40MINUS},
    {23,    6,      7,    WLAN_BAND_WIDTH_40MINUS},
    {24,    10,     11,   WLAN_BAND_WIDTH_40MINUS},
    {25,    11,     12,   WLAN_BAND_WIDTH_40MINUS},
    {26,    9,      10,   WLAN_BAND_WIDTH_40MINUS},
    {27,    2,      3,    WLAN_BAND_WIDTH_40PLUS},
    {28,    3,      4,    WLAN_BAND_WIDTH_40PLUS},
    {29,    5,      6,    WLAN_BAND_WIDTH_40MINUS},
    {30,    7,      8,    WLAN_BAND_WIDTH_40MINUS},
    {31,    8,      9,    WLAN_BAND_WIDTH_40MINUS},
};
/* 5G全频段待选信道 */
hmac_chan_stru g_aus_channel_candidate_list_5g[HMAC_CHANNEL_NUM_5G] = {
    // idx  chan_idx  chan_num   bandwidth
    {0,     0,      36,    WLAN_BAND_WIDTH_20M},
    {1,     1,      40,    WLAN_BAND_WIDTH_20M},
    {2,     2,      44,    WLAN_BAND_WIDTH_20M},
    {3,     3,      48,    WLAN_BAND_WIDTH_20M},
    {4,     0,      36,    WLAN_BAND_WIDTH_40PLUS},
    {5,     1,      40,    WLAN_BAND_WIDTH_40MINUS},
    {6,     2,      44,    WLAN_BAND_WIDTH_40PLUS},
    {7,     3,      48,    WLAN_BAND_WIDTH_40MINUS},
    {8,     0,      36,    WLAN_BAND_WIDTH_80PLUSPLUS},
    {9,     1,      40,    WLAN_BAND_WIDTH_80MINUSPLUS},
    {10,    2,      44,    WLAN_BAND_WIDTH_80PLUSMINUS},
    {11,    3,      48,    WLAN_BAND_WIDTH_80MINUSMINUS},
    {12,    4,      52,    WLAN_BAND_WIDTH_20M},
    {13,    5,      56,    WLAN_BAND_WIDTH_20M},
    {14,    6,      60,    WLAN_BAND_WIDTH_20M},
    {15,    7,      64,    WLAN_BAND_WIDTH_20M},
    {16,    4,      52,    WLAN_BAND_WIDTH_40PLUS},
    {17,    5,      56,    WLAN_BAND_WIDTH_40MINUS},
    {18,    6,      60,    WLAN_BAND_WIDTH_40PLUS},
    {19,    7,      64,    WLAN_BAND_WIDTH_40MINUS},
    {20,    4,      52,    WLAN_BAND_WIDTH_80PLUSPLUS},
    {21,    5,      56,    WLAN_BAND_WIDTH_80MINUSPLUS},
    {22,    6,      60,    WLAN_BAND_WIDTH_80PLUSMINUS},
    {23,    7,      64,    WLAN_BAND_WIDTH_80MINUSMINUS},
    {24,    8,      100,   WLAN_BAND_WIDTH_20M},
    {25,    9,      104,   WLAN_BAND_WIDTH_20M},
    {26,    10,     108,   WLAN_BAND_WIDTH_20M},
    {27,    11,     112,   WLAN_BAND_WIDTH_20M},
    {28,    8,      100,   WLAN_BAND_WIDTH_40PLUS},
    {29,    9,      104,   WLAN_BAND_WIDTH_40MINUS},
    {30,    10,     108,   WLAN_BAND_WIDTH_40PLUS},
    {31,    11,     112,   WLAN_BAND_WIDTH_40MINUS},
    {32,    8,      100,   WLAN_BAND_WIDTH_80PLUSPLUS},
    {33,    9,      104,   WLAN_BAND_WIDTH_80MINUSPLUS},
    {34,    10,     108,   WLAN_BAND_WIDTH_80PLUSMINUS},
    {35,    11,     112,   WLAN_BAND_WIDTH_80MINUSMINUS},
    {36,    12,     116,   WLAN_BAND_WIDTH_20M},
    {37,    13,     120,   WLAN_BAND_WIDTH_20M},
    {38,    14,     124,   WLAN_BAND_WIDTH_20M},
    {39,    15,     128,   WLAN_BAND_WIDTH_20M},
    {40,    12,     116,   WLAN_BAND_WIDTH_40PLUS},
    {41,    13,     120,   WLAN_BAND_WIDTH_40MINUS},
    {42,    14,     124,   WLAN_BAND_WIDTH_40PLUS},
    {43,    15,     128,   WLAN_BAND_WIDTH_40MINUS},
    {44,    12,     116,   WLAN_BAND_WIDTH_80PLUSPLUS},
    {45,    13,     120,   WLAN_BAND_WIDTH_80MINUSPLUS},
    {46,    14,     124,   WLAN_BAND_WIDTH_80PLUSMINUS},
    {47,    15,     128,   WLAN_BAND_WIDTH_80MINUSMINUS},
    {48,    16,     132,   WLAN_BAND_WIDTH_20M},
    {49,    17,     136,   WLAN_BAND_WIDTH_20M},
    {50,    18,     140,   WLAN_BAND_WIDTH_20M},
    {51,    19,     144,   WLAN_BAND_WIDTH_20M},
    {52,    16,     132,   WLAN_BAND_WIDTH_40PLUS},
    {53,    17,     136,   WLAN_BAND_WIDTH_40MINUS},
    {54,    18,     140,   WLAN_BAND_WIDTH_40PLUS},
    {55,    19,     144,   WLAN_BAND_WIDTH_40MINUS},
    {56,    16,     132,   WLAN_BAND_WIDTH_80PLUSPLUS},
    {57,    17,     136,   WLAN_BAND_WIDTH_80MINUSPLUS},
    {58,    18,     140,   WLAN_BAND_WIDTH_80PLUSMINUS},
    {59,    19,     144,   WLAN_BAND_WIDTH_80MINUSMINUS},
    {60,    20,     149,   WLAN_BAND_WIDTH_20M},
    {61,    21,     153,   WLAN_BAND_WIDTH_20M},
    {62,    22,     157,   WLAN_BAND_WIDTH_20M},
    {63,    23,     161,   WLAN_BAND_WIDTH_20M},
    {64,    20,     149,   WLAN_BAND_WIDTH_40PLUS},
    {65,    21,     153,   WLAN_BAND_WIDTH_40MINUS},
    {66,    22,     157,   WLAN_BAND_WIDTH_40PLUS},
    {67,    23,     161,   WLAN_BAND_WIDTH_40MINUS},
    {68,    20,     149,   WLAN_BAND_WIDTH_80PLUSPLUS},
    {69,    21,     153,   WLAN_BAND_WIDTH_80MINUSPLUS},
    {70,    22,     157,   WLAN_BAND_WIDTH_80PLUSMINUS},
    {71,    23,     161,   WLAN_BAND_WIDTH_80MINUSMINUS},
    {72,    24,     165,   WLAN_BAND_WIDTH_20M}
};

static void hmac_chan_pack_chan_stat_report_info(mac_scan_chan_stats_stru *chan_result,
    hmac_chba_chan_stat_report_stru *info, uint8_t scan_chan_idx)
{
    info->acs_info_type = HMAC_CHAN_MEAS_CHAN_STAT_REPORT;
    info->scan_chan_sbs = scan_chan_idx;
    info->scan_chan_idx = scan_chan_idx;
    info->total_free_time_20m_us = chan_result->ul_total_free_time_20M_us;
    info->total_free_time_40m_us = chan_result->ul_total_free_time_40M_us;
    info->total_free_time_80m_us = chan_result->ul_total_free_time_80M_us;
    info->total_send_time_us = chan_result->ul_total_send_time_us;
    info->total_recv_time_us = chan_result->ul_total_recv_time_us;
    info->total_stats_time_us = chan_result->ul_total_stats_time_us;
    info->free_power_cnt = chan_result->uc_free_power_cnt;
    info->free_power_stats_20m = chan_result->s_free_power_stats_20M;
    info->free_power_stats_40m = chan_result->s_free_power_stats_40M;
    info->free_power_stats_80m = chan_result->s_free_power_stats_80M;
}

void hmac_chan_meas_scan_complete_handler(hmac_scan_record_stru *pst_scan_record,
    uint8_t uc_scan_idx)
{
    mac_scan_chan_stats_stru *chan_result = NULL;
    hmac_chba_chan_stat_report_stru chan_stats_report;
    hmac_vap_stru *hmac_vap = NULL;

    if (pst_scan_record == NULL) {
        return;
    }
    if (pst_scan_record->en_scan_mode != WLAN_SCAN_MODE_CHAN_MEAS_SCAN) {
        return;
    }
    hmac_vap = mac_res_get_hmac_vap(pst_scan_record->uc_vap_id);
    if (hmac_vap == NULL) {
        return;
    }
    chan_result = &(pst_scan_record->ast_chan_results[uc_scan_idx]);
    /* 上报信息 */
    hmac_chan_pack_chan_stat_report_info(chan_result, &chan_stats_report, hmac_vap->chan_meas_scan_chan);
#ifdef _PRE_WLAN_CHBA_MGMT
    if (!mac_is_chba_mode(&hmac_vap->st_vap_base_info) || (mac_is_chba_mode(&hmac_vap->st_vap_base_info) &&
        hmac_vap->hmac_chba_vap_info->chba_log_level == CHBA_DEBUG_ALL_LOG)) {
        oam_warning_log3(0, 0, "hmac_chba_scan_complete_handler:rpt_chan_num[%d], rpt_chan_idx[%d], scan_chan_sbs[%d]",
            chan_result->uc_channel_number, chan_stats_report.scan_chan_idx, chan_stats_report.scan_chan_sbs);

        oam_warning_log3(0, 0, "hmac_chba_scan_complete_handler: free_time_20m_us=[%d], free_time_40m_us=[%d], \
            free_time_80m_us=[%d]", chan_stats_report.total_free_time_20m_us,
            chan_stats_report.total_free_time_40m_us, chan_stats_report.total_free_time_80m_us);

        oam_warning_log4(0, 0, "hmac_chba_scan_complete_handler: send_time_us=[%d], recv_time_us=[%d], \
            stats_time_us=[%d], free_power_cnt=[%d]", chan_stats_report.total_send_time_us,
            chan_stats_report.total_recv_time_us, chan_stats_report.total_stats_time_us,
            chan_stats_report.free_power_cnt);

        oam_warning_log3(0, 0, "hmac_chba_scan_complete_handler: free_power_stats_20m=[%d], free_power_stats_40m=[%d], \
            free_power_stats_80m=[%d]", chan_stats_report.free_power_stats_20m, chan_stats_report.free_power_stats_40m,
            chan_stats_report.free_power_stats_80m);
    }
#endif
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_cfg80211_link_meas_res_report(hmac_vap->pst_net_device, GFP_KERNEL,
        (uint8_t *)(&chan_stats_report), sizeof(chan_stats_report));
#endif
    return;
}

uint32_t hmac_chan_meas_scan_chan_once(mac_vap_stru *pst_mac_vap, mac_cfg_link_meas_stru *meas_cmd)
{
    hmac_chan_stru *pst_candidate_list = NULL;
    mac_channel_stru scan_channel = {0};
    mac_scan_req_stru st_h2d_scan_params = {0};
    hmac_vap_stru *pst_hmac_vap = NULL;
    uint8_t chan_index;
    uint32_t ret;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == NULL) {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, 0, "{hmac_chan_meas_scan_chan_once::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (hmac_vap_is_connecting(pst_mac_vap)) {
        oam_warning_log0(pst_mac_vap->uc_vap_id, 0, "{hmac_chan_meas_scan_chan_once::has user connecting, stop scan!}");
        return OAL_FAIL;
    }
    chan_index = meas_cmd->scan_chan;
    pst_hmac_vap->chan_meas_scan_chan = meas_cmd->scan_chan;
    scan_channel.en_band = meas_cmd->scan_band;
    pst_candidate_list = (scan_channel.en_band == WLAN_BAND_5G) ? g_aus_channel_candidate_list_5g :
        g_aus_channel_candidate_list_2g;
    scan_channel.uc_chan_number = pst_candidate_list[chan_index].uc_chan_number;
    scan_channel.en_bandwidth = pst_candidate_list[chan_index].en_bandwidth;
    scan_channel.uc_idx = pst_candidate_list[chan_index].uc_chan_idx;

    /* 设置扫描参数 */
    st_h2d_scan_params.uc_vap_id = pst_mac_vap->uc_vap_id;
    st_h2d_scan_params.en_bss_type = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    st_h2d_scan_params.en_scan_type = WLAN_SCAN_TYPE_PASSIVE;
    st_h2d_scan_params.uc_probe_delay = 0;
    st_h2d_scan_params.uc_scan_func = MAC_SCAN_FUNC_MEAS | MAC_SCAN_FUNC_STATS;
    st_h2d_scan_params.uc_max_scan_count_per_channel = 1;
    st_h2d_scan_params.us_scan_time = meas_cmd->meas_time;
    st_h2d_scan_params.en_scan_mode = WLAN_SCAN_MODE_CHAN_MEAS_SCAN;
    st_h2d_scan_params.uc_channel_nums = 1;
    st_h2d_scan_params.ast_channel_list[0] = scan_channel;

    ret = hmac_scan_proc_scan_req_event(pst_hmac_vap, &st_h2d_scan_params);
    if (ret != OAL_SUCC) {
        oam_warning_log0(pst_mac_vap->uc_vap_id, 0, "hmac_chan_meas_scan_chan_once:hmac scan req failed");
    }

    return ret;
}
