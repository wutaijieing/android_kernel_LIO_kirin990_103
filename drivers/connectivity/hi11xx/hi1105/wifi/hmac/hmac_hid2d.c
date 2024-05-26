

#ifdef _PRE_WLAN_FEATURE_HID2D
/* 1 ͷ�ļ����� */
#include "mac_ie.h"
#include "mac_vap.h"
#include "mac_device.h"
#include "wlan_types.h"
#include "hmac_chan_mgmt.h"
#include "mac_device.h"
#include "hmac_hid2d.h"
#include "hmac_scan.h"
#include "oal_cfg80211.h"
#include "hmac_config.h"
#ifdef _PRE_WLAN_CHBA_MGMT
#include "hmac_chba_function.h"
#endif
#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_HID2D_C

#define DECIMAL_TO_INTEGER 100 /* ���ڽ�С��ת�������������ڼ��� */
/* 2 ȫ�ֱ������� */
/* ȫ�ִ�ѡ�ŵ����������ŵ���ʵ�������п�ѡ��Ĵ�ѡ�ŵ�����GO��GC��֧�ֵ��ŵ��б��󽻼����ã�
   ��ȫ�ִ�ѡ�ŵ�����ű�ʾ */
/* 2.4GȫƵ�δ�ѡ�ŵ� */
hmac_hid2d_chan_stru g_aus_channel_candidate_list_2g[HMAC_HID2D_CHANNEL_NUM_2G] = {
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
/* 5GȫƵ�δ�ѡ�ŵ� */
hmac_hid2d_chan_stru g_aus_channel_candidate_list_5g[HMAC_HID2D_CHANNEL_NUM_5G] = {
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
/* ɨ��˳������: ��ʼ��Ϊ�����᳡����ɨ��˳������ҵ�񳡾���Ҫ�ڿ�ʼɨ��ʱ����ʵ�ʴ�ѡ�ŵ��б�ȷ������ֵ */
uint8_t g_scan_chan_list[HMAC_HID2D_MAX_SCAN_CHAN_NUM] = { 8, 20, 32, 44, 56, 68 };
/* һ��ɨ�������ֵ��ȷ��g_scan_chan_list�ı߽磬��ʼ��ֵΪ�����᳡�� */
uint8_t g_scan_chan_num_max = HMAC_HID2D_SCAN_CHAN_NUM_FOR_APK;
/* ʵ�ʿ�֧�ֵ��ŵ��б����� */
uint8_t g_supp_chan_list[HMAC_HID2D_CHANNEL_NUM_5G] = { 0 };
/* ʵ�ʿ�֧�ֵ��ŵ�������ȷ��g_supp_chan_list�ı߽� */
uint8_t g_supp_chan_num_max = 0;
/* ɨ����Ϣ */
hmac_hid2d_auto_channel_switch_stru g_st_hmac_hid2d_acs_info = {
    .uc_acs_mode = OAL_FALSE,
};

OAL_STATIC const wal_hid2d_cmd_entry_stru g_hid2d_vendor_cmd[] = {
    { WAL_HID2D_INIT_CMD,          wal_hipriv_hid2d_presentation_init },
    { WAL_HID2D_CHAN_SWITCH_CMD,   wal_hipriv_hid2d_swchan },
    { WAL_HID2D_ACS_CMD,           wal_ioctl_set_hid2d_acs_mode },
    { WAL_HID2D_ACS_STATE_CMD,     wal_ioctl_update_acs_state },
    { WAL_HID2D_LINK_MEAS_CMD,     wal_ioctl_hid2d_link_meas },
};

/* 3 �������� */
uint32_t hmac_hid2d_scan_chan_once(mac_vap_stru *pst_mac_vap, uint8_t chan_index, uint8_t scan_mode);
uint8_t hmac_hid2d_get_best_chan(mac_channel_stru *pst_scan_chan);
uint32_t hmac_hid2d_acs_scan_handler(hmac_scan_record_stru *scan_record, uint8_t scan_idx,
    hmac_hid2d_auto_channel_switch_stru *hmac_hid2d_acs_info);
uint32_t hmac_hid2d_acs_once_scan_handler(hmac_scan_record_stru *scan_record, uint8_t scan_idx,
    hmac_hid2d_auto_channel_switch_stru *hmac_hid2d_acs_info);
uint32_t hmac_hid2d_acs_timeout_handle(void *arg);
uint32_t hmac_hid2d_chan_stat_report_timeout_handle(void *arg);
uint32_t hmac_hid2d_acs_get_best_chan(hmac_hid2d_auto_channel_switch_stru *hmac_hid2d_acs_info,
    hmac_hid2d_chan_info_stru *chan_info, hmac_hid2d_chan_stru *candidate_chan_list);

/* 4 ����ʵ�� */

uint32_t hmac_hid2d_find_p2p_vap(mac_device_stru *pst_mac_device, mac_vap_stru **ppst_mac_vap)
{
    uint8_t uc_vap_index;
    uint8_t uc_no_p2p_vap_flag = OAL_TRUE;
    mac_vap_stru *pst_p2p_mac_vap = NULL;

    /* �ҵ��ҽ��ڸ�device�ϵ�p2p vap */
    for (uc_vap_index = 0; uc_vap_index < pst_mac_device->uc_vap_num; uc_vap_index++) {
        pst_p2p_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_index]);
        if (pst_p2p_mac_vap == NULL) {
            continue;
        }
        if (pst_p2p_mac_vap->en_p2p_mode != WLAN_LEGACY_VAP_MODE) {
            uc_no_p2p_vap_flag = OAL_FALSE;
            break;
        }
    }
    if (uc_no_p2p_vap_flag == OAL_TRUE) {
        oam_error_log0(0, OAM_SF_ANTI_INTF, "{hmac_hid2d_find_p2p_vap_ext::no p2p vap error!}");
        return OAL_FAIL;
    }
    *ppst_mac_vap = pst_p2p_mac_vap;
    return OAL_SUCC;
}
uint32_t hmac_hid2d_find_up_p2p_vap(mac_device_stru *pst_mac_device, mac_vap_stru **ppst_mac_vap)
{
    uint8_t uc_vap_index;
    uint8_t uc_no_p2p_vap_flag = OAL_TRUE;
    mac_vap_stru *pst_p2p_mac_vap = NULL;
    mac_vap_stru *mac_vap = NULL;

    /* �ҵ��ҽ��ڸ�device�ϵ�p2p vap */
    for (uc_vap_index = 0; uc_vap_index < pst_mac_device->uc_vap_num; uc_vap_index++) {
        pst_p2p_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_index]);
        if (pst_p2p_mac_vap == NULL) {
            continue;
        }
        mac_vap = pst_p2p_mac_vap;
        if (pst_p2p_mac_vap->en_p2p_mode == WLAN_P2P_GO_MODE || pst_p2p_mac_vap->en_p2p_mode == WLAN_P2P_CL_MODE) {
            uc_no_p2p_vap_flag = OAL_FALSE;
            break;
        }
    }
    if (uc_no_p2p_vap_flag == OAL_TRUE) {
        oam_error_log0(0, 0, "{hmac_hid2d_find_p2p_vap::no up p2p vap error!}");
        *ppst_mac_vap = mac_vap; /* pst_mac_vap�Ƿ�p2p GO/CL vap������p2p������ķ����ƺ��� */
        return OAL_FAIL;
    }
    *ppst_mac_vap = pst_p2p_mac_vap;
    return OAL_SUCC;
}


uint32_t hmac_hid2d_scan_chan_once(mac_vap_stru *pst_mac_vap, uint8_t chan_index, uint8_t scan_mode)
{
    hmac_hid2d_auto_channel_switch_stru *pst_hmac_hid2d_acs_info = NULL;
    hmac_hid2d_chan_stru *pst_candidate_list = NULL;
    mac_channel_stru scan_channel = {0};
    mac_scan_req_h2d_stru st_h2d_scan_params = {0};
    hmac_vap_stru *pst_hmac_vap = NULL;
    uint32_t ret;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (pst_hmac_vap == NULL) {
        oam_error_log0(pst_mac_vap->uc_vap_id, 0, "{hmac_hid2d_scan_chan_once::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (hmac_vap_is_connecting(pst_mac_vap)) {
        oam_warning_log0(pst_mac_vap->uc_vap_id, 0, "{hmac_hid2d_scan_chan_once::has user connecting, stop scan!}");
        return OAL_FAIL;
    }
    pst_hmac_hid2d_acs_info = &g_st_hmac_hid2d_acs_info;

    if (scan_mode == HMAC_HID2D_ACS_SCAN) {
        scan_channel.en_band = pst_hmac_hid2d_acs_info->uc_cur_band;
        pst_candidate_list = (scan_channel.en_band == WLAN_BAND_5G) ? g_aus_channel_candidate_list_5g :
            g_aus_channel_candidate_list_2g;
    } else {
        scan_channel.en_band = pst_hmac_hid2d_acs_info->scan_band;
        pst_candidate_list = (scan_channel.en_band == WLAN_BAND_5G) ? g_aus_channel_candidate_list_5g :
            g_aus_channel_candidate_list_2g;
    }
    scan_channel.uc_chan_number = pst_candidate_list[chan_index].uc_chan_number;
    scan_channel.en_bandwidth = pst_candidate_list[chan_index].en_bandwidth;
    scan_channel.uc_chan_idx = pst_candidate_list[chan_index].uc_chan_idx;

    /* ����ɨ����� */
    st_h2d_scan_params.st_scan_params.uc_vap_id = pst_mac_vap->uc_vap_id;
    st_h2d_scan_params.st_scan_params.en_bss_type = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    st_h2d_scan_params.st_scan_params.en_scan_type = WLAN_SCAN_TYPE_PASSIVE;
    st_h2d_scan_params.st_scan_params.uc_probe_delay = 0;
    st_h2d_scan_params.st_scan_params.uc_scan_func = MAC_SCAN_FUNC_MEAS | MAC_SCAN_FUNC_STATS;
    st_h2d_scan_params.st_scan_params.uc_max_scan_count_per_channel = 1;
    st_h2d_scan_params.st_scan_params.us_scan_time = pst_hmac_hid2d_acs_info->scan_time;
    st_h2d_scan_params.st_scan_params.en_scan_mode = WLAN_SCAN_MODE_HID2D_SCAN;
    st_h2d_scan_params.st_scan_params.uc_channel_nums = 1;
    st_h2d_scan_params.st_scan_params.ast_channel_list[0] = scan_channel;

    ret = hmac_scan_proc_scan_req_event(pst_hmac_vap, &st_h2d_scan_params);
    if (ret != OAL_SUCC) {
        oam_warning_log0(pst_mac_vap->uc_vap_id, 0, "hmac_hid2d_scan_chan_once:hmac scan req failed");
    }

    return ret;
}

uint8_t hmac_hid2d_get_best_chan(mac_channel_stru *pst_scan_chan)
{
    hmac_hid2d_chan_info_stru *pst_chan_info = NULL;
    hmac_hid2d_auto_channel_switch_stru *pst_hmac_hid2d_acs_info = NULL;
    uint8_t uc_index;
    uint8_t uc_chan_idx;
    int16_t highest_chan_load = 0;
    int16_t lowest_noise_rssi = 0;
    uint8_t best_chan_of_chan_load = 0;
    uint8_t best_chan_of_noise_rssi = 0;

    pst_hmac_hid2d_acs_info = &g_st_hmac_hid2d_acs_info;
    pst_chan_info = pst_hmac_hid2d_acs_info->chan_info_5g;

    for (uc_index = 0; uc_index < g_scan_chan_num_max; uc_index++) {
        uc_chan_idx = g_scan_chan_list[uc_index];
        oam_warning_log3(0, OAM_SF_SCAN, "{HiD2D results: channel[%d], avg chan_load is %d, avg noise rssi is %d}",
            g_aus_channel_candidate_list_5g[uc_chan_idx].uc_chan_number,
            pst_chan_info[uc_chan_idx].chan_load, pst_chan_info[uc_chan_idx].chan_noise_rssi);
        if (pst_chan_info[uc_chan_idx].chan_load > highest_chan_load) {
            highest_chan_load = pst_chan_info[uc_chan_idx].chan_load;
            best_chan_of_chan_load = uc_chan_idx;
        }
        if (pst_chan_info[uc_chan_idx].chan_noise_rssi < lowest_noise_rssi) {
            lowest_noise_rssi = pst_chan_info[uc_chan_idx].chan_noise_rssi;
            best_chan_of_noise_rssi = uc_chan_idx;
        }
    }
    /* �ֱ���ռ�ձ���ߺ͵�����Сѡ�����ŵ�A���ŵ�B */
    /* ����ŵ�A���ŵ�B��һ���ŵ������report���ŵ�����������ѡ��ռ�ձ���ߵ��ŵ�A�������ŵ�B��ռ�ձ���С��A��
        �������A��5dB���� */
    if (best_chan_of_chan_load == best_chan_of_noise_rssi) {
        uc_chan_idx = best_chan_of_chan_load;
    } else if ((highest_chan_load - pst_chan_info[best_chan_of_noise_rssi].chan_load < HMAC_HID2D_CHAN_LOAD_DIFF)
        && (pst_chan_info[best_chan_of_chan_load].chan_noise_rssi - lowest_noise_rssi > HMAC_HID2D_NOISE_DIFF)) {
        uc_chan_idx = best_chan_of_noise_rssi;
    } else {
        uc_chan_idx = best_chan_of_chan_load;
    }
    pst_scan_chan->en_band = WLAN_BAND_5G;
    pst_scan_chan->uc_chan_number = g_aus_channel_candidate_list_5g[uc_chan_idx].uc_chan_number;
    pst_scan_chan->en_bandwidth = g_aus_channel_candidate_list_5g[uc_chan_idx].en_bandwidth;
    pst_scan_chan->uc_chan_idx = g_aus_channel_candidate_list_5g[uc_chan_idx].uc_chan_idx;
    oam_warning_log2(0, OAM_SF_SCAN, "{HiD2D results: best channel num is %d, bw is %d}",
        pst_scan_chan->uc_chan_number, pst_scan_chan->en_bandwidth);

    return OAL_SUCC;
}


uint32_t hmac_hid2d_scan_complete_handler(hmac_scan_record_stru *pst_scan_record, uint8_t uc_scan_idx)
{
    hmac_hid2d_auto_channel_switch_stru *pst_hmac_hid2d_acs_info = NULL;
    uint32_t ret = OAL_SUCC;

    if (pst_scan_record == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_hid2d_acs_info = &g_st_hmac_hid2d_acs_info;
    if (pst_hmac_hid2d_acs_info->uc_acs_mode) {
        oam_warning_log0(0, OAM_SF_SCAN, "hmac_hid2d_scan_complete_handler: acs enabled");
        ret = hmac_hid2d_acs_scan_handler(pst_scan_record, uc_scan_idx, pst_hmac_hid2d_acs_info);
    } else if (pst_hmac_hid2d_acs_info->scan_mode == HMAC_HID2D_ONCE_SCAN) {
        ret = hmac_hid2d_acs_once_scan_handler(pst_scan_record, uc_scan_idx, pst_hmac_hid2d_acs_info);
    }

    return ret;
}


uint16_t hmac_hid2d_get_idx_from_chan_num(uint8_t band, uint8_t channel_num, uint8_t bw)
{
    uint8_t channel_idx = 0;
    uint8_t idx;
    hmac_hid2d_chan_stru *candidate_list = NULL;
    uint8_t total_num;

    if (band == WLAN_BAND_2G) {
        candidate_list = g_aus_channel_candidate_list_2g;
        total_num = HMAC_HID2D_CHANNEL_NUM_2G;
    } else {
        candidate_list = g_aus_channel_candidate_list_5g;
        total_num = HMAC_HID2D_CHANNEL_NUM_5G;
    }

    for (idx = 0; idx < total_num; idx++) {
        if (candidate_list[idx].uc_chan_number == channel_num &&
            candidate_list[idx].en_bandwidth == bw) {
            channel_idx = idx;
        }
    }

    return channel_idx;
}


uint8_t hmac_hid2d_bw_mode_convert(uint8_t band, uint8_t channel_num, uint8_t bw)
{
    uint8_t idx, bandwidth;
    hmac_hid2d_chan_stru *candidate_list = NULL;
    uint8_t total_num;

    if (band == WLAN_BAND_2G) {
        candidate_list = g_aus_channel_candidate_list_2g;
        total_num = HMAC_HID2D_CHANNEL_NUM_2G;
    } else {
        candidate_list = g_aus_channel_candidate_list_5g;
        total_num = HMAC_HID2D_CHANNEL_NUM_5G;
    }

    bandwidth = WLAN_BAND_WIDTH_20M;
    for (idx = 0; idx < total_num; idx++) {
        if (candidate_list[idx].uc_chan_number == channel_num) {
            bandwidth = candidate_list[idx].en_bandwidth;
            if (bw == (oal_nl80211_chan_width)WLAN_BANDWIDTH_TO_IEEE_CHAN_WIDTH(bandwidth, OAL_TRUE)) {
                break;
            }
        }
    }

    return bandwidth;
}


uint32_t hmac_hid2d_get_valid_2g_channel_list(void)
{
    uint8_t *scan_chan_list = g_scan_chan_list;
    uint8_t *supp_chan_list = g_supp_chan_list;
    uint8_t max_scan_chan_num = 0;
    uint8_t max_supp_chan_num = 0;
    uint8_t idx;
    uint8_t chan_idx;
    uint8_t valid_flag1 = 0;
    uint8_t valid_flag2;

    for (idx = 0; idx < 14; idx++) { /* 2.4G��20M�ŵ�����Ϊ14�� */
        chan_idx = g_aus_channel_candidate_list_2g[idx].uc_chan_idx;
        if (g_aus_channel_candidate_list_2g[idx].en_bandwidth == WLAN_BAND_WIDTH_20M) {
            valid_flag1 = (mac_is_channel_idx_valid(WLAN_BAND_2G, chan_idx, OAL_FALSE) == OAL_SUCC) ? 1 : 0;
        }
        if (valid_flag1) {
            supp_chan_list[max_supp_chan_num] = idx;
            max_supp_chan_num++;
        }
    }
    for (idx = 14; idx < HMAC_HID2D_CHANNEL_NUM_2G; idx++) { /* 2.4G��20M�ŵ�����Ϊ14�� */
        chan_idx = g_aus_channel_candidate_list_2g[idx].uc_chan_idx;
        if (g_aus_channel_candidate_list_2g[idx].en_bandwidth == WLAN_BAND_WIDTH_40PLUS) {
            valid_flag1 = (mac_is_channel_idx_valid(WLAN_BAND_2G, chan_idx, OAL_FALSE) == OAL_SUCC) ? 1 : 0;
            valid_flag2 = (mac_is_channel_idx_valid(WLAN_BAND_2G,
                chan_idx + 4, OAL_FALSE) == OAL_SUCC) ? 1 : 0; /* idx+4�ŵ��� */
        } else {
            valid_flag1 = (mac_is_channel_idx_valid(WLAN_BAND_2G, chan_idx, OAL_FALSE) == OAL_SUCC) ? 1 : 0;
            valid_flag2 = (mac_is_channel_idx_valid(WLAN_BAND_2G,
                chan_idx - 4, OAL_FALSE) == OAL_SUCC) ? 1 : 0; /* idx-4�ŵ��� */
        }
        if (valid_flag1 && valid_flag2) {
            supp_chan_list[max_supp_chan_num] = idx;
            max_supp_chan_num++;
            scan_chan_list[max_scan_chan_num] = idx;
            max_scan_chan_num++;
        }
    }
    g_supp_chan_num_max = max_supp_chan_num;
    g_scan_chan_num_max = max_scan_chan_num;
    return OAL_SUCC;
}


uint8_t hmac_hid2d_is_valid_5g_channel(uint8_t idx)
{
    uint8_t chan_idx;
    uint8_t valid, dfs;
    uint8_t valid_flag;

    if (idx >= HMAC_HID2D_CHANNEL_NUM_5G) {
        oam_warning_log1(0, 0, "hmac_hid2d_is_valid_5g_channel:: idx[%d] is invalid.", idx);
        return OAL_FALSE;
    }
    chan_idx = g_aus_channel_candidate_list_5g[idx].uc_chan_idx;
    valid = (uint8_t)mac_is_channel_idx_valid(WLAN_BAND_5G, chan_idx, OAL_FALSE);
    dfs = mac_is_dfs_channel(WLAN_BAND_5G, g_aus_channel_candidate_list_5g[idx].uc_chan_number);
    valid_flag = (valid == OAL_SUCC && dfs == OAL_FALSE) ? 1 : 0;
    return valid_flag;
}


uint32_t hmac_hid2d_get_valid_5g_channel_list(void)
{
    uint8_t *scan_chan_list = g_scan_chan_list;
    uint8_t *supp_chan_list = g_supp_chan_list;
    uint8_t max_scan_chan_num = 0;
    uint8_t max_supp_chan_num = 0;
    uint8_t idx, k;
    uint8_t valid_flag[HMAC_HID2D_MAX_CHANNELS];
    uint8_t cnt;

    idx = 0;
    while (idx < HMAC_HID2D_CHANNEL_NUM_5G - 1) {
        for (k = 0; k < HMAC_HID2D_MAX_CHANNELS; k++) {
            valid_flag[k] = hmac_hid2d_is_valid_5g_channel(idx + k);
            if (valid_flag[k] == OAL_TRUE) {
                supp_chan_list[max_supp_chan_num++] = idx + k;
            }
        }
        idx = idx + HMAC_HID2D_MAX_CHANNELS;
        if (valid_flag[0] && valid_flag[1]) {
            supp_chan_list[max_supp_chan_num++] = idx++;
            supp_chan_list[max_supp_chan_num++] = idx++;
        }
        if (valid_flag[2] && valid_flag[3]) { /* �鿴80M�ŵ��к�40M�ŵ�(���2��3)�Ƿ���� */
            supp_chan_list[max_supp_chan_num++] = idx++;
            supp_chan_list[max_supp_chan_num++] = idx++;
        }
        if (valid_flag[0] && valid_flag[1] && valid_flag[2] && valid_flag[3]) { /* �鿴��20M�ŵ�(0, 1, 2, 3)�Ƿ���� */
            scan_chan_list[max_scan_chan_num++] = idx;
            supp_chan_list[max_supp_chan_num++] = idx++;
            supp_chan_list[max_supp_chan_num++] = idx++;
            supp_chan_list[max_supp_chan_num++] = idx++;
            supp_chan_list[max_supp_chan_num++] = idx++;
        }
    }
    cnt = max_scan_chan_num;
    for (k = 0; k < cnt; k++) {
        scan_chan_list[cnt + k] = scan_chan_list[k] + 1; /* ��Ӵ�20MΪ���ŵ���80M�ŵ� */
        scan_chan_list[cnt * 2 + k] = scan_chan_list[k] + 2; /* ����Ե�3��20M(���2)Ϊ���ŵ���80M�ŵ� */
        scan_chan_list[cnt * 3 + k] = scan_chan_list[k] + 3; /* ����Ե�4��20M(���3)Ϊ���ŵ���80M�ŵ� */
        max_scan_chan_num = max_scan_chan_num + 3; /* ���������3��ɨ���ŵ� */
    }
    /* ����165�ŵ������⴦�� */
    valid_flag[0] = hmac_hid2d_is_valid_5g_channel(idx);
    if (valid_flag[0] == OAL_TRUE) {
        supp_chan_list[max_supp_chan_num++] = idx;
        scan_chan_list[max_scan_chan_num++] = idx;
    }
    g_supp_chan_num_max = max_supp_chan_num;
    g_scan_chan_num_max = max_scan_chan_num;
    return OAL_SUCC;
}


hmac_hid2d_net_mode_enum hmac_hid2d_check_vap_state(mac_vap_stru *mac_vap, mac_device_stru *mac_device)
{
    hmac_hid2d_net_mode_enum state;
    uint8_t is_dbdc;
    uint8_t is_only_p2p;
    mac_vap_stru *another_vap = NULL;

    /* �жϵ�ǰvap�Ƿ���GO�����������ֱ���˳� */
    if (mac_vap->en_p2p_mode != WLAN_P2P_GO_MODE) {
        return HMAC_HID2D_NET_MODE_BUTT;
    }

    /* �ж��Ƿ������һ��up��vap */
    another_vap = mac_vap_find_another_up_vap_by_mac_vap(mac_vap);
    /* ������ҵ�����p2p vap֮���up��vap��˵�����ǵ�p2p */
    is_only_p2p = (another_vap == NULL) ? OAL_TRUE : OAL_FALSE;
    is_dbdc = mac_is_dbdc_running(mac_device);
    if (is_only_p2p) {
        state = HMAC_HID2D_P2P_ONLY;
    } else if (is_dbdc) {
        state = HMAC_HID2D_DBDC;
    } else {
        state = HMAC_HID2D_SAME_BAND;
    }
    return state;
}


uint8_t hmac_hid2d_get_next_scan_channel(hmac_hid2d_auto_channel_switch_stru *hmac_hid2d_acs_info)
{
    uint8_t next_chan_idx;

    /* ��һ�δ���ɨ�裬ֱ�ӷ���0 */
    if (hmac_hid2d_acs_info->uc_cur_scan_chan_idx == g_scan_chan_num_max) {
        hmac_hid2d_acs_info->uc_cur_scan_chan_idx = 0;
        return 0;
    }
    /* �Ȼ�ȡ�ϴ�ɨ����ŵ���g_scan_chan_list�е�idx��Ȼ���1��ȡ����Ӧ��ɨ����ŵ�idx */
    next_chan_idx = (hmac_hid2d_acs_info->uc_cur_scan_chan_idx + 1) % g_scan_chan_num_max;
    /* �ж��Ƿ��ǵ�ǰ�ŵ�������ǵ�ǰ�ŵ���ֱ������ */
    /* ��ǰ���жϵ�ǰ�ŵ��Ƿ���ɨ���ŵ��ϸ�һ��
       �����ǰ�ŵ�Ϊ20M/40M��оƬ�ϱ���40M/80M��Ϣ��Ч���ж�������Ϊɨ���ŵ��Ƿ������ǰ�ŵ� */
    if (hmac_hid2d_acs_info->uc_cur_chan_idx == g_scan_chan_list[next_chan_idx]) {
        next_chan_idx = (next_chan_idx + 1) % g_scan_chan_num_max;
    }
    hmac_hid2d_acs_info->uc_cur_scan_chan_idx = next_chan_idx;
    return next_chan_idx;
}


void hmac_hid2d_acs_destroy_timer(hmac_hid2d_auto_channel_switch_stru *hmac_hid2d_acs_info)
{
    if (hmac_hid2d_acs_info->st_chan_stat_report_timer.en_is_registerd == OAL_TRUE) {
        frw_timer_immediate_destroy_timer_m(&(hmac_hid2d_acs_info->st_chan_stat_report_timer));
    }
    if (hmac_hid2d_acs_info->st_scan_chan_timer.en_is_registerd == OAL_TRUE) {
        frw_timer_immediate_destroy_timer_m(&(hmac_hid2d_acs_info->st_scan_chan_timer));
    }
}


void hmac_hid2d_acs_init_params(hmac_hid2d_auto_channel_switch_stru *hmac_hid2d_acs_info, mac_vap_stru *p2p_vap)
{
    hmac_hid2d_acs_destroy_timer(hmac_hid2d_acs_info);
    memset_s(hmac_hid2d_acs_info, sizeof(hmac_hid2d_auto_channel_switch_stru),
        0, sizeof(hmac_hid2d_auto_channel_switch_stru));
    hmac_hid2d_acs_info->uc_acs_mode = OAL_TRUE;
    hmac_hid2d_acs_info->uc_vap_id = p2p_vap->uc_vap_id;
    hmac_hid2d_acs_info->uc_cur_band = p2p_vap->st_channel.en_band;
    if (hmac_hid2d_acs_info->uc_cur_band == WLAN_BAND_2G) {
        hmac_hid2d_acs_info->candidate_list = g_aus_channel_candidate_list_2g;
        hmac_hid2d_get_valid_2g_channel_list();
    } else {
        hmac_hid2d_acs_info->candidate_list = g_aus_channel_candidate_list_5g;
        hmac_hid2d_get_valid_5g_channel_list();
    }
    hmac_hid2d_acs_info->uc_cur_chan_idx = hmac_hid2d_get_idx_from_chan_num(p2p_vap->st_channel.en_band,
        p2p_vap->st_channel.uc_chan_number, p2p_vap->st_channel.en_bandwidth);
    hmac_hid2d_acs_info->uc_cur_scan_chan_idx = g_scan_chan_num_max;
    hmac_hid2d_acs_info->scan_chan_idx = HMAC_HID2D_CHANNEL_NUM_5G;
    hmac_hid2d_acs_info->scan_time = HMAC_HID2D_SCAN_TIME_PER_CHAN_ACS;
}


void hmac_hid2d_acs_exit(hmac_hid2d_auto_channel_switch_stru *hmac_hid2d_acs_info, mac_vap_stru *p2p_vap,
    uint8_t exit_type)
{
    uint32_t ret;
    uint8_t acs_mode;
    hmac_vap_stru *hmac_vap = NULL;
    hmac_hid2d_acs_init_report_stru acs_init_report;
    hmac_hid2d_acs_exit_report_stru acs_exit_report;

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(p2p_vap->uc_vap_id);
    if (hmac_vap == NULL) {
        return;
    }
    if (exit_type == HMAC_HID2D_NON_GO) {
        acs_init_report.acs_info_type = HMAC_HID2D_INIT_REPORT;
        acs_init_report.acs_exit_type = HMAC_HID2D_NON_GO;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        oal_cfg80211_hid2d_acs_report(hmac_vap->pst_net_device, GFP_KERNEL, (uint8_t *)(&acs_init_report),
            sizeof(acs_init_report));
#endif
        return;
    } else if (exit_type != HMAC_HID2D_FWK_DISABLE) {
        /* �ϱ��ں˷����˳���ԭ�� */
        acs_exit_report.acs_info_type = HMAC_HID2D_EXTI_REPORT;
        acs_exit_report.acs_exit_type = exit_type;
        oam_warning_log1(0, OAM_SF_ANY, "HiD2D ACS: exit and report to hal, exit type[%d].", exit_type);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        oal_cfg80211_hid2d_acs_report(hmac_vap->pst_net_device, GFP_KERNEL,
            (uint8_t *)(&acs_exit_report), sizeof(acs_exit_report));
#endif
    }

    /* ע����ʱ�� */
    hmac_hid2d_acs_destroy_timer(hmac_hid2d_acs_info);
    /* ��״̬ͬ����dmac */
    acs_mode = OAL_FALSE;
    hmac_hid2d_acs_info->uc_acs_mode = acs_mode;
    hmac_hid2d_acs_info->acs_state = HMAC_HID2D_ACS_DISABLE;
    ret = hmac_config_send_event(p2p_vap, WLAN_CFGID_HID2D_ACS_MODE, sizeof(uint8_t), &acs_mode);
    if (oal_unlikely(ret != OAL_SUCC)) {
        return;
    }
}


void hmac_hid2d_init_report(hmac_hid2d_auto_channel_switch_stru *hmac_hid2d_acs_info, mac_vap_stru *p2p_vap,
    mac_device_stru *mac_device)
{
    hmac_hid2d_acs_init_report_stru acs_init_report;
    hmac_vap_stru *hmac_vap = NULL;
    int32_t ret;

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(p2p_vap->uc_vap_id);
    if (hmac_vap == NULL) {
        return;
    }
    acs_init_report.acs_info_type = HMAC_HID2D_INIT_REPORT;
    acs_init_report.cur_band = hmac_hid2d_acs_info->uc_cur_band;
    acs_init_report.chan_number = p2p_vap->st_channel.uc_chan_number;
    acs_init_report.bandwidth = p2p_vap->st_channel.en_bandwidth;
    acs_init_report.go_mode = hmac_hid2d_check_vap_state(p2p_vap, mac_device);
    ret = memcpy_s(acs_init_report.supp_chan_list, sizeof(uint8_t) * HMAC_HID2D_CHANNEL_NUM_5G,
        g_supp_chan_list, sizeof(uint8_t) * g_supp_chan_num_max);
    acs_init_report.supp_chan_num_max = g_supp_chan_num_max;
    ret += memcpy_s(acs_init_report.scan_chan_list, sizeof(uint8_t) * HMAC_HID2D_CHANNEL_NUM_5G,
        g_supp_chan_list, sizeof(uint8_t) * g_scan_chan_num_max);
    if (ret != EOK) {
        oam_warning_log1(0, 0, "{hmac_hid2d_init_report::memcpy fail, ret[%d].}", ret);
    }
    acs_init_report.scan_chan_num_max = g_scan_chan_num_max;

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    /* �ϱ��ں� */
    oal_cfg80211_hid2d_acs_report(hmac_vap->pst_net_device, GFP_KERNEL,
        (uint8_t *)(&acs_init_report), sizeof(acs_init_report));
#endif
}


uint32_t hmac_hid2d_set_acs_mode(mac_vap_stru *mac_vap, uint8_t acs_mode)
{
    hmac_hid2d_auto_channel_switch_stru *hmac_hid2d_acs_info = &g_st_hmac_hid2d_acs_info;
    mac_device_stru *mac_device = NULL;
    mac_vap_stru *p2p_vap = NULL;
    uint32_t ret;

    oam_warning_log1(0, 0, "{hmac_hid2d_set_acs_mode: %d", acs_mode);

    /* �ҵ���Ӧ��P2P vap���ж��Ƿ���GO��������ֱ�ӷ��� */
    mac_device = mac_res_get_dev(mac_vap->uc_device_id);
    if (mac_device == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    ret = hmac_hid2d_find_up_p2p_vap(mac_device, (mac_vap_stru **)&p2p_vap);
    if (ret != OAL_SUCC) {
        hmac_hid2d_acs_exit(hmac_hid2d_acs_info, p2p_vap, HMAC_HID2D_P2P_LOST);
        return ret;
    }
    if (!IS_P2P_GO(p2p_vap)) {
        hmac_hid2d_acs_exit(hmac_hid2d_acs_info, p2p_vap, HMAC_HID2D_NON_GO);
        return OAL_SUCC;
    }

    if (acs_mode == OAL_FALSE) {
        hmac_hid2d_acs_exit(hmac_hid2d_acs_info, p2p_vap, HMAC_HID2D_FWK_DISABLE);
        return OAL_SUCC;
    }

    /* ��ʼ�� */
    hmac_hid2d_acs_init_params(hmac_hid2d_acs_info, p2p_vap);
    /* �ϴ���Ϣ��hal�� */
    hmac_hid2d_init_report(hmac_hid2d_acs_info, p2p_vap, mac_device);

    /* ͬ����״̬��dmac�� */
    ret = hmac_config_send_event(p2p_vap, WLAN_CFGID_HID2D_ACS_MODE, sizeof(uint8_t), &acs_mode);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_warning_log1(p2p_vap->uc_vap_id, 0, "HiD2D ACS: send hid2d acs mode to dmac failed[%d].", ret);
        return ret;
    }
    /* ����dmac�ϱ���ʱ��ʱ��(60s) */
    frw_timer_create_timer_m(&hmac_hid2d_acs_info->st_chan_stat_report_timer,
        hmac_hid2d_chan_stat_report_timeout_handle, HMAC_HID2D_REPORT_TIMEOUT_MS,
        mac_device, OAL_FALSE, OAM_MODULE_ID_HMAC, mac_device->core_id);

    return OAL_SUCC;
}

uint32_t hmac_hid2d_update_acs_state(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param)
{
    hmac_hid2d_auto_channel_switch_stru *hmac_hid2d_acs_info = NULL;

    oam_warning_log1(0, OAM_SF_ANY, "HiD2D ACS: acs state updated to [%d].", *param);
    hmac_hid2d_acs_info = &g_st_hmac_hid2d_acs_info;
    hmac_hid2d_acs_info->acs_state = *param;
    return OAL_SUCC;
}

uint32_t hmac_hid2d_link_meas(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param)
{
    mac_device_stru *mac_device = NULL;
    mac_hid2d_link_meas_stru *hid2d_meas_cmd = NULL;
    hmac_hid2d_auto_channel_switch_stru *hmac_hid2d_acs_info = NULL;

    mac_device = mac_res_get_dev(mac_vap->uc_device_id);
    if (mac_device == NULL) {
        return OAL_SUCC;
    }
    hmac_hid2d_acs_info = &g_st_hmac_hid2d_acs_info;
    hid2d_meas_cmd = (mac_hid2d_link_meas_stru *)param;

    if (hid2d_meas_cmd->link_meas_cmd_type == HMAC_HID2D_LINK_MEAS_START_BY_CHAN_LIST) {
        hmac_hid2d_acs_info->scan_mode = HMAC_HID2D_ACS_SCAN;
        hmac_hid2d_acs_info->scan_time = hid2d_meas_cmd->meas_time;
        hmac_hid2d_acs_info->scan_interval = hid2d_meas_cmd->scan_interval;
        oam_warning_log2(0, OAM_SF_ANY, "HiD2D ACS: start link measurement by default scan list, scan time[%u]ms, \
            scan interval[%u]s.", hmac_hid2d_acs_info->scan_time, hmac_hid2d_acs_info->scan_interval);
        frw_timer_create_timer_m(&hmac_hid2d_acs_info->st_scan_chan_timer,
            hmac_hid2d_acs_timeout_handle,
            HMAC_HID2D_SCAN_TIMER_CYCLE_MS * hmac_hid2d_acs_info->scan_interval,
            mac_device, OAL_FALSE, OAM_MODULE_ID_HMAC, mac_device->core_id);
    } else if (hid2d_meas_cmd->link_meas_cmd_type == HMAC_HID2D_LINK_MEAS_UPDATE_SCAN_INTERVAL) {
        hmac_hid2d_acs_info->scan_mode = HMAC_HID2D_ACS_SCAN;
        hmac_hid2d_acs_info->scan_interval = hid2d_meas_cmd->scan_interval;
        oam_warning_log1(0, OAM_SF_ANY, "HiD2D ACS: update scan interval[%u]s.", hmac_hid2d_acs_info->scan_interval);
        if (hmac_hid2d_acs_info->st_scan_chan_timer.en_is_registerd == OAL_TRUE) {
            frw_timer_immediate_destroy_timer_m(&(hmac_hid2d_acs_info->st_scan_chan_timer));
        }
        frw_timer_create_timer_m(&hmac_hid2d_acs_info->st_scan_chan_timer, hmac_hid2d_acs_timeout_handle,
            HMAC_HID2D_SCAN_TIMER_CYCLE_MS * hmac_hid2d_acs_info->scan_interval,
            mac_device, OAL_FALSE, OAM_MODULE_ID_HMAC, mac_device->core_id);
    } else if (hid2d_meas_cmd->link_meas_cmd_type == HMAC_HID2D_LINK_MEAS_UPDATE_SCAN_TIME) {
        hmac_hid2d_acs_info->scan_mode = HMAC_HID2D_ACS_SCAN;
        hmac_hid2d_acs_info->scan_time = hid2d_meas_cmd->meas_time;
        oam_warning_log1(0, OAM_SF_ANY, "HiD2D ACS: update scan time[%u]ms.", hmac_hid2d_acs_info->scan_time);
    } else if (hid2d_meas_cmd->link_meas_cmd_type == HMAC_HID2D_LINK_MEAS_START_BY_CHAN) {
        if (hmac_hid2d_acs_info->uc_acs_mode == OAL_TRUE) {
            oam_warning_log0(0, OAM_SF_ANY, "HiD2D ACS: acs enabled, not support link measurement.");
            return OAL_FAIL;
        }
        hmac_hid2d_acs_info->scan_mode = HMAC_HID2D_ONCE_SCAN;
        hmac_hid2d_acs_info->scan_chan_idx = hid2d_meas_cmd->scan_chan;
        hmac_hid2d_acs_info->scan_time = hid2d_meas_cmd->meas_time;
        hmac_hid2d_acs_info->scan_band = hid2d_meas_cmd->scan_band;
        oam_warning_log2(0, OAM_SF_ANY, "HiD2D ACS: scan channel idx[%d] once for [%u]ms.",
            hmac_hid2d_acs_info->scan_chan_idx, hmac_hid2d_acs_info->scan_time);
        hmac_hid2d_scan_chan_once(mac_vap, hmac_hid2d_acs_info->scan_chan_idx, HMAC_HID2D_ONCE_SCAN);
    }

    return OAL_SUCC;
}


uint32_t hmac_hid2d_acs_timeout_handle(void *arg)
{
    mac_device_stru *mac_device = NULL;
    mac_vap_stru *p2p_vap = NULL;
    hmac_hid2d_auto_channel_switch_stru *hmac_hid2d_acs_info = NULL;
    uint8_t next_chan_idx;
    uint8_t next_scan_chan;
    uint32_t ret;

    hmac_hid2d_acs_info = &g_st_hmac_hid2d_acs_info;
    mac_device = (mac_device_stru *)arg;
    if (mac_device == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    ret = hmac_hid2d_find_up_p2p_vap(mac_device, (mac_vap_stru **)&p2p_vap);
    if (ret != OAL_SUCC) {
        /* P2P ����, �˳����� */
        hmac_hid2d_acs_exit(hmac_hid2d_acs_info, p2p_vap, HMAC_HID2D_P2P_LOST);
        return ret;
    }

    if (hmac_hid2d_acs_info->acs_state == HMAC_HID2D_ACS_DISABLE) {
        return OAL_SUCC;
    } else if (hmac_hid2d_acs_info->acs_state == HMAC_HID2D_ACS_WORK) {
        next_chan_idx = hmac_hid2d_get_next_scan_channel(hmac_hid2d_acs_info);
        next_scan_chan = g_scan_chan_list[next_chan_idx];
        hmac_hid2d_scan_chan_once(p2p_vap, next_scan_chan, HMAC_HID2D_ACS_SCAN);
    }
    /* ������һ��ɨ��Ķ�ʱ�� */
    frw_timer_create_timer_m(&hmac_hid2d_acs_info->st_scan_chan_timer,
        hmac_hid2d_acs_timeout_handle,
        HMAC_HID2D_SCAN_TIMER_CYCLE_MS * hmac_hid2d_acs_info->scan_interval,
        mac_device,
        OAL_FALSE,
        OAM_MODULE_ID_HMAC,
        mac_device->core_id);
    return OAL_SUCC;
}

static void hmac_hid2d_pack_chan_stat_report_info(wlan_scan_chan_stats_stru *chan_result,
    hmac_hid2d_chan_stat_report_stru *info, uint8_t scan_chan_sbs, uint8_t scan_chan_idx)
{
    info->acs_info_type = HMAC_HID2D_CHAN_STAT_REPORT;
    info->scan_chan_sbs = scan_chan_sbs;
    info->scan_chan_idx = scan_chan_idx;
    info->total_free_time_20m_us = chan_result->total_free_time_20M_us;
    info->total_free_time_40m_us = chan_result->total_free_time_40M_us;
    info->total_free_time_80m_us = chan_result->total_free_time_80M_us;
    info->total_send_time_us = chan_result->total_send_time_us;
    info->total_recv_time_us = chan_result->total_recv_time_us;
    info->total_stats_time_us = chan_result->total_stats_time_us;
    info->free_power_cnt = chan_result->uc_free_power_cnt;
    info->free_power_stats_20m = chan_result->s_free_power_stats_20M;
    info->free_power_stats_40m = chan_result->s_free_power_stats_40M;
    info->free_power_stats_80m = chan_result->s_free_power_stats_80M;
    info->free_power_stats_160m = chan_result->s_free_power_stats_160M;
}


uint32_t hmac_hid2d_acs_scan_handler(hmac_scan_record_stru *scan_record, uint8_t scan_idx,
    hmac_hid2d_auto_channel_switch_stru *hmac_hid2d_acs_info)
{
    hmac_hid2d_chan_stru *candidate_chan_list = NULL;
    wlan_scan_chan_stats_stru *chan_result = NULL;
    hmac_hid2d_chan_stat_report_stru chan_stats_report;
    uint8_t channel_number;
    uint8_t channel_idx;
    hmac_vap_stru *hmac_vap = NULL;

    /* �������acs�·���ɨ�裬ɨ���������� */
    if (scan_record->en_scan_mode != WLAN_SCAN_MODE_HID2D_SCAN) {
        return OAL_FAIL;
    }

    hmac_vap = mac_res_get_hmac_vap(scan_record->uc_vap_id);
    if (hmac_vap == NULL) {
        return OAL_FAIL;
    }
    /* ��ȡ��ǰ�ϱ�����Ϣ��Ϣ */
    chan_result = &(scan_record->ast_chan_results[scan_idx]);
    channel_number = chan_result->uc_channel_number;
    candidate_chan_list = hmac_hid2d_acs_info->candidate_list;
    if (hmac_hid2d_acs_info->scan_chan_idx < HMAC_HID2D_CHANNEL_NUM_5G) {
        channel_idx = hmac_hid2d_acs_info->scan_chan_idx;
    } else {
        channel_idx = g_scan_chan_list[hmac_hid2d_acs_info->uc_cur_scan_chan_idx];
    }
    /* �ж��ϱ����ŵ����·�ɨ����ŵ��Ƿ�һ�� */
    if (channel_number != candidate_chan_list[channel_idx].uc_chan_number) {
        oam_warning_log2(0, 0, "HiD2D ACS: channel not match, scan chan is [%d], report chan is [%d]!",
            candidate_chan_list[channel_idx].uc_chan_number, channel_number);
        return OAL_FAIL;
    }
    /* �ϱ���Ϣ */
    hmac_hid2d_pack_chan_stat_report_info(chan_result, &chan_stats_report,
        hmac_hid2d_acs_info->scan_chan_idx, channel_idx);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_cfg80211_hid2d_acs_report(hmac_vap->pst_net_device, GFP_KERNEL,
        (uint8_t *)(&chan_stats_report), sizeof(chan_stats_report));
#endif
    return OAL_SUCC;
}

uint32_t hmac_hid2d_acs_once_scan_handler(hmac_scan_record_stru *scan_record, uint8_t scan_idx,
    hmac_hid2d_auto_channel_switch_stru *hmac_hid2d_acs_info)
{
    wlan_scan_chan_stats_stru *chan_result = NULL;
    hmac_hid2d_chan_stat_report_stru chan_stats_report;
    hmac_vap_stru *hmac_vap = NULL;

    if (scan_record == NULL) {
        oam_warning_log0(0, 0, "scan record is null");
        return OAL_FAIL;
    }
    if (scan_record->en_scan_mode != WLAN_SCAN_MODE_HID2D_SCAN) {
        oam_warning_log1(0, 0, "HiD2D : not hid2d scan [%d]!", scan_record->en_scan_mode);
        return OAL_FAIL;
    }
    hmac_vap = mac_res_get_hmac_vap(scan_record->uc_vap_id);
    if (hmac_vap == NULL) {
        return OAL_FAIL;
    }
    chan_result = &(scan_record->ast_chan_results[scan_idx]);
    /* �ϱ���Ϣ */
    hmac_hid2d_pack_chan_stat_report_info(chan_result, &chan_stats_report,
        hmac_hid2d_acs_info->scan_chan_idx, hmac_hid2d_acs_info->scan_chan_idx);
#ifdef _PRE_WLAN_CHBA_MGMT
    if (hmac_vap->st_vap_base_info.chba_mode != CHBA_MODE || (hmac_vap->st_vap_base_info.chba_mode == CHBA_MODE &&
        hmac_vap->hmac_chba_vap_info->chba_log_level == CHBA_DEBUG_ALL_LOG)) {
#endif
        oam_warning_log3(hmac_vap->st_vap_base_info.uc_vap_id, 0,
            "HiD2D ACS: report_chan_num=[%d], report_chan_idx=[%d], scan_chan_sbs=[%d]",
            chan_result->uc_channel_number, chan_stats_report.scan_chan_idx, chan_stats_report.scan_chan_sbs);

        oam_warning_log3(0, 0, "HiD2D ACS: free_time_20m_us=[%d], free_time_40m_us=[%d], \
            free_time_80m_us=[%d]", chan_stats_report.total_free_time_20m_us,
            chan_stats_report.total_free_time_40m_us, chan_stats_report.total_free_time_80m_us);

        oam_warning_log4(0, 0, "HiD2D ACS: send_time_us=%d, recv_time_us=[%d], stats_time_us=[%d], free_power_cnt=[%d]",
            chan_stats_report.total_send_time_us, chan_stats_report.total_recv_time_us,
            chan_stats_report.total_stats_time_us, chan_stats_report.free_power_cnt);

        oam_warning_log4(0, 0, "HiD2D ACS: free_power_stats_20m=[%d], free_power_stats_40m=[%d], \
            free_power_stats_80m=[%d], free_power_stats_160m=[%d]",
            chan_stats_report.free_power_stats_20m, chan_stats_report.free_power_stats_40m,
            chan_stats_report.free_power_stats_80m, chan_stats_report.free_power_stats_160m);
#ifdef _PRE_WLAN_CHBA_MGMT
    }
#endif
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_cfg80211_hid2d_acs_report(hmac_vap->pst_net_device, GFP_KERNEL,
        (uint8_t *)(&chan_stats_report), sizeof(chan_stats_report));
#endif
    return OAL_SUCC;
}


void hmac_hid2d_acs_switch_completed(mac_vap_stru *mac_vap)
{
    hmac_hid2d_auto_channel_switch_stru *hmac_hid2d_acs_info = NULL;
    hmac_hid2d_switch_report_stru switch_info;
    hmac_vap_stru *hmac_vap = NULL;

    hmac_hid2d_acs_info = &g_st_hmac_hid2d_acs_info;

    if (mac_vap->en_p2p_mode != WLAN_P2P_GO_MODE || hmac_hid2d_acs_info->uc_acs_mode != OAL_TRUE) {
        return;
    }
    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (hmac_vap == NULL) {
        return;
    }

    /* ���л�����ŵ��ʹ�����Ϣ�ϱ��ں� */
    oam_warning_log0(0, OAM_SF_FRW, "HiD2D ACS: switch completed, report to hal.\n");
    switch_info.acs_info_type = HMAC_HID2D_SWITCH_SUCC_REPORT;
    switch_info.chan_number = mac_vap->st_channel.uc_chan_number;
    switch_info.bandwidth = mac_vap->st_channel.en_bandwidth;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_cfg80211_hid2d_acs_report(hmac_vap->pst_net_device, GFP_KERNEL,
        (uint8_t *)(&switch_info), sizeof(switch_info));
#endif

    return;
}


uint32_t hmac_hid2d_cur_chan_stat_handler(mac_vap_stru *mac_vap, uint8_t len, uint8_t *param)
{
    hmac_hid2d_chan_stat_stru *chan_stats = NULL;
    hmac_hid2d_auto_channel_switch_stru *hmac_hid2d_acs_info = NULL;

    mac_device_stru *mac_device = NULL;
    hmac_vap_stru *hmac_vap = NULL;
    hmac_hid2d_link_stat_report_stru link_stats;

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (hmac_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    chan_stats = (hmac_hid2d_chan_stat_stru *)param;
    hmac_hid2d_acs_info = &g_st_hmac_hid2d_acs_info;

    mac_device = mac_res_get_dev(mac_vap->uc_device_id);
    if (mac_device == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* �������ʱ�ϱ���ʱ�� */
    if (hmac_hid2d_acs_info->st_chan_stat_report_timer.en_is_registerd == OAL_TRUE) {
        frw_timer_immediate_destroy_timer_m(&(hmac_hid2d_acs_info->st_chan_stat_report_timer));
    }

    /* �ϱ��ں� */
    link_stats.acs_info_type = HMAC_HID2D_LINK_INFO_REPORT;
    link_stats.chan_number = mac_vap->st_channel.uc_chan_number;
    link_stats.bandwidth = mac_vap->st_channel.en_bandwidth;
    link_stats.go_mode = hmac_hid2d_check_vap_state(mac_vap, mac_device);
    if (link_stats.go_mode == HMAC_HID2D_NET_MODE_BUTT) {
        hmac_hid2d_acs_exit(hmac_hid2d_acs_info, mac_vap, HMAC_HID2D_P2P_LOST);
        return OAL_SUCC;
    }
    memcpy_s(&(link_stats.link_stat), sizeof(hmac_hid2d_chan_stat_stru),
        chan_stats, sizeof(hmac_hid2d_chan_stat_stru));
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_cfg80211_hid2d_acs_report(hmac_vap->pst_net_device, GFP_KERNEL,
        (uint8_t *)(&link_stats), sizeof(link_stats));
#endif
    /* ����dmac�ϱ���ʱ��ʱ�� */
    frw_timer_create_timer_m(&hmac_hid2d_acs_info->st_chan_stat_report_timer,
        hmac_hid2d_chan_stat_report_timeout_handle, HMAC_HID2D_REPORT_TIMEOUT_MS,
        mac_device, OAL_FALSE, OAM_MODULE_ID_HMAC, mac_device->core_id);
    return OAL_SUCC;
}


uint32_t hmac_hid2d_chan_stat_report_timeout_handle(void *arg)
{
    mac_device_stru *mac_device = NULL;
    mac_vap_stru *p2p_vap = NULL;
    hmac_hid2d_auto_channel_switch_stru *hmac_hid2d_acs_info;
    uint32_t ret;

    hmac_hid2d_acs_info = &g_st_hmac_hid2d_acs_info;
    mac_device = (mac_device_stru *)arg;
    if (mac_device == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    ret = hmac_hid2d_find_up_p2p_vap(mac_device, (mac_vap_stru **)&p2p_vap);
    if (ret != OAL_SUCC) {
        /* P2P ����, �˳����� */
        hmac_hid2d_acs_exit(hmac_hid2d_acs_info, p2p_vap, HMAC_HID2D_P2P_LOST);
        return ret;
    }
    oam_warning_log0(p2p_vap->uc_vap_id, 0,
        "{HiD2D ACS: Too long to obtain the channel stats report from dmac. Disable ACS.");
    hmac_hid2d_acs_exit(hmac_hid2d_acs_info, p2p_vap, HMAC_HID2D_DMAC_REPORT_TIMEOUT);

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_HID2D_PRESENTATION

uint32_t wal_hipriv_hid2d_presentation_init(oal_net_device_stru *net_dev, int8_t *cmd)
{
    oal_net_device_stru *hisilicon_net_dev = NULL;
    int8_t setcountry[] = {" 99"};
    int8_t fixcountry[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {"hid2d_presentation"};
    uint32_t ret_type;
    int32_t ret;

    hisilicon_net_dev = wal_config_get_netdev("Hisilicon0", OAL_STRLEN("Hisilicon0"));
    if (hisilicon_net_dev == NULL) {
        oam_warning_log0(0, OAM_SF_ANY, "{HiD2D Presentation::wal_config_get_netdev return null ptr!}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* ����oal_dev_get_by_name�󣬱������oal_dev_putʹnet_dev�����ü�����һ */
    oal_dev_put(hisilicon_net_dev);
    /* ���ù�������Ҫʹ��"Hisilicon0"������ */
    ret_type = wal_hipriv_setcountry(hisilicon_net_dev, setcountry);
    if (ret_type != OAL_SUCC) {
        return ret_type;
    }
    ret = strcat_s(fixcountry, sizeof(fixcountry), cmd);
    if (ret != EOK) {
        return OAL_FAIL;
    }
    ret_type = wal_hipriv_set_val_cmd(net_dev, fixcountry);
    if (ret_type != OAL_SUCC) {
        return ret_type;
    }
    return OAL_SUCC;
}


uint32_t wal_hipriv_hid2d_swchan(oal_net_device_stru *net_dev, int8_t *cmd)
{
    int8_t sw_cmd[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {" "};
    int8_t value_cmd[WAL_HIPRIV_CMD_VALUE_MAX_LEN] = {0};
    uint32_t ret_type, value, offset;
    int32_t ret;
    uint16_t prim_chan, bw;

    /* ת�����ŵ������Ȼ���·����� */
    ret_type = wal_get_cmd_one_arg(cmd, value_cmd, WAL_HIPRIV_CMD_VALUE_MAX_LEN, &offset);
    if (ret_type != OAL_SUCC) {
        return ret_type;
    }
    value = (uint32_t)oal_atoi(value_cmd);
    prim_chan = (uint16_t)(value >> 8); /* ǰ8λ��ʾ���ŵ��� */
    bw = (uint16_t)(value & 0xFF); /* ��8λ��ʾ���� */

    memset_s(value_cmd, sizeof(value_cmd), 0, sizeof(value_cmd));
    oal_itoa(prim_chan, value_cmd, sizeof(value_cmd));
    ret = strcat_s(sw_cmd, sizeof(sw_cmd), value_cmd);
    if (ret != EOK) {
        return OAL_FAIL;
    }
    ret = strcat_s(sw_cmd, sizeof(sw_cmd), " ");
    if (ret != EOK) {
        return OAL_FAIL;
    }
    memset_s(value_cmd, sizeof(value_cmd), 0, sizeof(value_cmd));
    oal_itoa(bw, value_cmd, sizeof(value_cmd));
    ret = strcat_s(sw_cmd, sizeof(sw_cmd), value_cmd);
    if (ret != EOK) {
        return OAL_FAIL;
    }
    ret_type = wal_hipriv_hid2d_switch_channel(net_dev, sw_cmd);
    if (ret_type != OAL_SUCC) {
        return ret_type;
    }
    return OAL_SUCC;
}


uint32_t wal_vendor_priv_cmd_hid2d_apk(oal_net_device_stru *net_dev, oal_ifreq_stru *ifr, uint8_t *cmd)
{
    int8_t cmd_type_str[WAL_HIPRIV_CMD_VALUE_MAX_LEN] = {0};
    int32_t cmd_type;
    uint32_t ret, offset;
    uint8_t cmd_idx;
    uint8_t is_valid_cmd = OAL_FALSE;

    /* �ж��ǲ���HiD2D APK�������, ��HiD2D APK���ֱ�ӷ��� */
    if (oal_strncasecmp(cmd, CMD_HID2D_PARAMS, CMD_HID2D_PARAMS_LEN) != 0) {
        return OAL_SUCC;
    }
    /* ��ȡtypeֵ */
    cmd += CMD_HID2D_PARAMS_LEN;
    ret = wal_get_cmd_one_arg(cmd, cmd_type_str, WAL_HIPRIV_CMD_VALUE_MAX_LEN, &offset);
    if (ret != OAL_SUCC) {
        oam_warning_log0(0, OAM_SF_ANY, "{wal_vendor_priv_cmd_hid2d_apk::invalid cmd}");
        return ret;
    }
    cmd_type = (int32_t)oal_atoi(cmd_type_str);
    cmd += offset;

    /* ����Typeֵ */
    if (cmd_type == 256) { /* 256Ϊ��������ŵ���cmd type */
        oam_warning_log1(0, OAM_SF_ANY, "{wal_vendor_priv_cmd_hid2d_apk::cmd[%d] NOT support!}", cmd_type);
        return OAL_SUCC;
    }
    if ((uint8_t)cmd_type >= WAL_HID2D_HIGH_BW_MCS_CMD && (uint8_t)cmd_type <= WAL_HID2D_CCA_CMD) {
        oam_warning_log1(0, OAM_SF_ANY, "{wal_vendor_priv_cmd_hid2d_apk::cmd[%d] NOT support!}", cmd_type);
        return OAL_SUCC;
    }
    for (cmd_idx = 0; cmd_idx < oal_array_size(g_hid2d_vendor_cmd); cmd_idx++) {
        if ((uint8_t)cmd_type == g_hid2d_vendor_cmd[cmd_idx].hid2d_cfg_type) {
            is_valid_cmd = OAL_TRUE;
            return g_hid2d_vendor_cmd[cmd_idx].func(net_dev, cmd);
        }
    }
    if (is_valid_cmd == OAL_FALSE) {
        oam_warning_log0(0, OAM_SF_ANY, "{wal_vendor_priv_cmd_hid2d::invalid cmd}");
        return OAL_FAIL;
    }

    return OAL_SUCC;
}
#endif

uint32_t wal_hipriv_hid2d_switch_channel(oal_net_device_stru *pst_net_dev, int8_t *pc_param)
{
    wal_msg_write_stru st_write_msg;
    uint32_t off_set;
    int8_t ac_name[WAL_HIPRIV_CMD_NAME_MAX_LEN] = {0};
    mac_csa_debug_stru st_csa_cfg;
    uint8_t uc_channel;
    wlan_channel_bandwidth_enum_uint8 en_bw_mode;
    int32_t l_ret;
    uint32_t ret;

    memset_s(&st_csa_cfg, sizeof(st_csa_cfg), 0, sizeof(st_csa_cfg));

    /* ��ȡҪ�л������ŵ��ŵ��� */
    ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &off_set);
    if (ret != OAL_SUCC) {
        oam_warning_log0(0, OAM_SF_ANY, "{HiD2D Presentation::get channel error, return.}");
        return ret;
    }
    pc_param += off_set;
    uc_channel = (oal_bool_enum_uint8)oal_atoi(ac_name);

    /* ��ȡ�ŵ�����ģʽ */
    ret = wal_get_cmd_one_arg(pc_param, ac_name, WAL_HIPRIV_CMD_NAME_MAX_LEN, &off_set);
    if (ret != OAL_SUCC) {
        oam_warning_log0(0, OAM_SF_CFG, "{HiD2D Presentation::get bandwidth error, return.}");
        return ret;
    }
    ret = mac_regdomain_get_bw_mode_by_cmd(ac_name, uc_channel, &en_bw_mode);
    if (ret != OAL_SUCC) {
        return ret;
    }
    pc_param += off_set;
    st_csa_cfg.en_bandwidth = en_bw_mode;
    st_csa_cfg.uc_channel = uc_channel;

    oam_warning_log2(0, OAM_SF_CFG, "{HiD2D Presentation::switch to CH%d, BW is %d}", uc_channel, en_bw_mode);

    /* ���¼���wal�㴦�� */
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_HID2D_SWITCH_CHAN, sizeof(st_csa_cfg));

    /* ��������������� */
    if (EOK != memcpy_s(st_write_msg.auc_value, sizeof(st_write_msg.auc_value),
        (const void *)&st_csa_cfg, sizeof(st_csa_cfg))) {
        oam_error_log0(0, OAM_SF_ANY, "HiD2D Presentation::memcpy fail!");
        return OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_net_dev,
        WAL_MSG_TYPE_WRITE,
        WAL_MSG_WRITE_MSG_HDR_LENGTH + sizeof(st_csa_cfg),
        (uint8_t *)&st_write_msg,
        OAL_FALSE,
        NULL);
    if (oal_unlikely(l_ret != OAL_SUCC)) {
        oam_warning_log1(0, OAM_SF_ANY, "{HiD2D Presentation::err[%d]!}", ret);
        return (uint32_t)l_ret;
    }
    return OAL_SUCC;
}


int32_t wal_ioctl_set_hid2d_state(oal_net_device_stru *pst_net_dev, uint8_t uc_param,
    wal_wifi_priv_cmd_stru *priv_cmd)
{
    if (uc_param == ENABLE) {
        hi110x_hcc_ip_pm_ctrl(DISABLE, HCC_EP_WIFI_DEV);
        wal_hid2d_sleep_mode(pst_net_dev, DISABLE, priv_cmd);
        wal_hid2d_napi_mode(pst_net_dev, DISABLE);
        wal_hid2d_freq_boost(pst_net_dev, ENABLE);
    } else if (uc_param == DISABLE) {
        hi110x_hcc_ip_pm_ctrl(ENABLE, HCC_EP_WIFI_DEV);
        wal_hid2d_sleep_mode(pst_net_dev, ENABLE, priv_cmd);
        wal_hid2d_napi_mode(pst_net_dev, ENABLE);
        wal_hid2d_freq_boost(pst_net_dev, DISABLE);
    }
    oam_warning_log1(0, OAM_SF_ANY, "{wal_ioctl_set_hid2d_state::uc_param[%d].}", uc_param);
    return OAL_SUCC;
}
#endif /* end of this file */
