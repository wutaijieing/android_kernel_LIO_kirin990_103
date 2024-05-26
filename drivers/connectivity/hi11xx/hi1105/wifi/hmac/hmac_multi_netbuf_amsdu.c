
#include "hmac_multi_netbuf_amsdu.h"
#include "hmac_config.h"
#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_MULTI_NETBUF_AMSDU_C

#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
static inline uint8_t wlan_amsdu_level_to_num(uint8_t level)
{
    return (((level) == WLAN_TX_AMSDU_NONE) ? 0 : \
        /* level number 2 */
        ((level) == WLAN_TX_AMSDU_BY_2) ? 2 : \
        /* level number 3 */
        ((level) == WLAN_TX_AMSDU_BY_3) ? 3 : \
        /* level number 4 */
        ((level) == WLAN_TX_AMSDU_BY_4) ? 4 : \
        /* level number 8 */
        ((level) == WLAN_TX_AMSDU_BY_8) ? 8 : 0);
}


uint8_t hmac_set_amsdu_num_based_protocol(mac_vap_stru *mac_vap, wlan_tx_amsdu_enum_uint8 tx_amsdu_level_type)
{
    /* Э��涨HT mpdu���Ȳ��ó���4095�ֽ� */
    if (oal_value_eq_any3(mac_vap->en_protocol, WLAN_HT_ONLY_MODE, WLAN_HT_11G_MODE, WLAN_HT_MODE)) {
        return  WLAN_TX_AMSDU_BY_2;
    } else if ((mac_vap->en_protocol == WLAN_VHT_MODE) ||
               (mac_vap->en_protocol == WLAN_VHT_ONLY_MODE) ||
               (g_wlan_spec_cfg->feature_11ax_is_open && (mac_vap->en_protocol == WLAN_HE_MODE))) {
        return  tx_amsdu_level_type;
    } else {
        return  WLAN_TX_AMSDU_NONE;
    }
}


uint8_t hmac_update_amsdu_num_1103(mac_vap_stru *mac_vap, hmac_performance_stat_stru *performance_stat_params,
    oal_bool_enum_uint8 mu_vap_flag)
{
    uint32_t limit_throughput_high;
    uint32_t limit_throughput_low;
    uint32_t tx_throughput_mbps = performance_stat_params->tx_throughput_mbps;
    /* ÿ������������ */
    limit_throughput_high = g_st_tx_large_amsdu.us_amsdu_throughput_high >> mu_vap_flag;
    limit_throughput_low  = g_st_tx_large_amsdu.us_amsdu_throughput_low >> mu_vap_flag;

    if (tx_throughput_mbps > limit_throughput_high) {
        /* ���ڸ�����,�л�amsdu����ۺ� */
        return WLAN_TX_AMSDU_BY_2;
    } else if (tx_throughput_mbps < limit_throughput_low) {
        /* ���ڵ�����,�ر�amsdu����ۺ� */
        return WLAN_TX_AMSDU_NONE;
    } else {
        /* ���ڵ�����-������֮��,�����л� */
        return g_st_tx_large_amsdu.en_tx_amsdu_level[mac_vap->uc_vap_id];
    }
}

uint8_t hmac_adjust_tx_amsdu_num_1105(mac_vap_stru *mac_vap, uint32_t tx_throughput_mbps,
    oal_bool_enum_uint8 mu_vap_flag)
{
    uint32_t limit_throughput_high = g_st_tx_large_amsdu.us_amsdu_throughput_high >> mu_vap_flag;
    uint32_t limit_throughput_middle = g_st_tx_large_amsdu.us_amsdu_throughput_middle >> mu_vap_flag;
    uint32_t limit_throughput_low = g_st_tx_large_amsdu.us_amsdu_throughput_low >> mu_vap_flag;
    wlan_tx_amsdu_enum_uint8 max_amsdu;

#ifdef _PRE_WLAN_FEATURE_11AX
    if (g_wlan_spec_cfg->feature_11ax_is_open) {
        /* �����1103��sdio���������⣺11ax���ޱ�11ac��100M */
        if (mac_vap->en_protocol == WLAN_HE_MODE) {
            limit_throughput_high -= WLAN_AMSDU_THROUGHPUT_TH_HE_VHT_DIFF;
        }
    }
#endif

    /* ��AMSDU, middle�����ǿ���amsdu����ʼ���� */
    if (tx_throughput_mbps > limit_throughput_middle) {
        if (tx_throughput_mbps > limit_throughput_high) {
            /* ���ڸ�����, ����4xmsdu */
            max_amsdu = WLAN_TX_AMSDU_BY_4;
        } else if (tx_throughput_mbps > (limit_throughput_middle << 1)) {
            /* ��ʼ���޵��������ϲ���С�ڸ�����, ����3xmsdu */
            max_amsdu = WLAN_TX_AMSDU_BY_3;
        } else {
            /* ������ʼ����,����2xmsdu */
            max_amsdu = WLAN_TX_AMSDU_BY_2;
        }

        /* �Ѿ����ǶԶ˵�AMSDU�ۺϳ��� */
        max_amsdu = hmac_set_amsdu_num_based_protocol(mac_vap, max_amsdu);
    } else if (tx_throughput_mbps > limit_throughput_low) {
        /* ������-������, ���ϴ���AMSDU�ۺϵģ���ۺ�2��; ���򲻾ۺ� */
        if (g_st_tx_large_amsdu.en_tx_amsdu_level[mac_vap->uc_vap_id] != WLAN_TX_AMSDU_NONE) {
            max_amsdu = hmac_set_amsdu_num_based_protocol(mac_vap, WLAN_TX_AMSDU_BY_2);
        } else {
            max_amsdu = WLAN_TX_AMSDU_NONE;
        }
    } else {
        /* ���ڵ�����, �ر�amsdu����ۺ� */
        max_amsdu = WLAN_TX_AMSDU_NONE;
    }

#ifdef _PRE_WLAN_FEATURE_11AX
    if (g_wlan_spec_cfg->feature_11ax_is_open) {
        max_amsdu = oal_min(oal_max(max_amsdu, mac_vap->bit_ofdma_aggr_num), WLAN_TX_AMSDU_BY_8);
    }
#endif

    return max_amsdu;
}


uint8_t hmac_update_amsdu_num_1105(mac_vap_stru *mac_vap, hmac_performance_stat_stru *performance_stat_params,
    oal_bool_enum_uint8 mu_vap_flag)
{
    return hmac_adjust_tx_amsdu_num_1105(mac_vap, performance_stat_params->tx_throughput_mbps, mu_vap_flag);
}
uint8_t hmac_adjust_rx_amsdu_num_shenkuo(mac_vap_stru *mac_vap, uint32_t rx_throughput_mbps, uint32_t tx_pps,
    wlan_tx_amsdu_enum_uint8 tx_amsdu)
{
    uint32_t limit_throughput_high;
    uint32_t limit_throughput_low;
    uint32_t limit_pps_high;
    uint32_t limit_pps_low;
    wlan_tx_amsdu_enum_uint8 amsdu_level_rx, amsdu_level;

    /* ÿ�����������ޣ�rx�������о�amsdu_level */
    hmac_tx_small_amsdu_get_limit_throughput(&limit_throughput_high, &limit_throughput_low);
    /* ����low�ſ����ۺ� */
    if (rx_throughput_mbps > limit_throughput_low) {
        if (rx_throughput_mbps > (limit_throughput_high << 1)) {
            amsdu_level_rx = WLAN_TX_AMSDU_BY_8;
        } else if (rx_throughput_mbps > limit_throughput_high) {
            amsdu_level_rx = WLAN_TX_AMSDU_BY_4;
        } else {
            amsdu_level_rx = WLAN_TX_AMSDU_BY_2;
        }
    } else {
        amsdu_level_rx = WLAN_TX_AMSDU_NONE;
    }
    amsdu_level_rx = oal_max(amsdu_level_rx, tx_amsdu);
    /* ÿ��PPS���ޣ�pps�о�amsdu_level */
    hmac_tx_small_amsdu_get_limit_pps(&limit_pps_high, &limit_pps_low);
    if (tx_pps > limit_pps_low) {
        if (tx_pps > (limit_pps_high << 1)) {
            amsdu_level = WLAN_TX_AMSDU_BY_8;
        } else if (tx_pps > limit_pps_high) {
            amsdu_level = WLAN_TX_AMSDU_BY_4;
        } else {
            amsdu_level = WLAN_TX_AMSDU_BY_2;
        }
    } else {
        amsdu_level = WLAN_TX_AMSDU_NONE;
    }
    amsdu_level = oal_max(amsdu_level_rx, amsdu_level);

    return amsdu_level;
}

uint8_t hmac_adjust_tx_amsdu_num_shenkuo(mac_vap_stru *mac_vap, uint32_t tx_throughput_mbps,
    oal_bool_enum_uint8 mu_vap_flag)
{
    uint32_t limit_throughput_high = g_st_tx_large_amsdu.us_amsdu_throughput_high >> mu_vap_flag;
    uint32_t limit_throughput_middle = g_st_tx_large_amsdu.us_amsdu_throughput_middle >> mu_vap_flag;
    uint32_t limit_throughput_low = g_st_tx_large_amsdu.us_amsdu_throughput_low >> mu_vap_flag;
    wlan_tx_amsdu_enum_uint8 max_amsdu;

#ifdef _PRE_WLAN_FEATURE_11AX
    if (g_wlan_spec_cfg->feature_11ax_is_open) {
        /* �����1103��sdio���������⣺11ax���ޱ�11ac��100M */
        if (mac_vap->en_protocol == WLAN_HE_MODE) {
            limit_throughput_high -= WLAN_AMSDU_THROUGHPUT_TH_HE_VHT_DIFF;
        }
    }
#endif

    /* ��AMSDU, middle�����ǿ���amsdu����ʼ���� */
    if (tx_throughput_mbps > limit_throughput_middle) {
        if (tx_throughput_mbps > (limit_throughput_high << 1)) {
            /* ����2��������, ����8xmsdu */
            max_amsdu = WLAN_TX_AMSDU_BY_8;
        } else if (tx_throughput_mbps > limit_throughput_high) {
            /* ���ڸ�����, ����4xmsdu */
            max_amsdu = WLAN_TX_AMSDU_BY_4;
        } else if (tx_throughput_mbps > (limit_throughput_middle << 1)) {
            /* ��ʼ���޵��������ϲ���С�ڸ�����, ����3xmsdu */
            max_amsdu = WLAN_TX_AMSDU_BY_3;
        } else {
            /* ������ʼ����,����2xmsdu */
            max_amsdu = WLAN_TX_AMSDU_BY_2;
        }
        /* �Ѿ����ǶԶ˵�AMSDU�ۺϳ��� */
        max_amsdu = hmac_set_amsdu_num_based_protocol(mac_vap, max_amsdu);
    } else if (tx_throughput_mbps > limit_throughput_low) {
        /* ������-������, ���ϴ���AMSDU�ۺϵģ���ۺ�2��; ���򲻾ۺ� */
        if (g_st_tx_large_amsdu.en_tx_amsdu_level[mac_vap->uc_vap_id] != WLAN_TX_AMSDU_NONE) {
            max_amsdu = hmac_set_amsdu_num_based_protocol(mac_vap, WLAN_TX_AMSDU_BY_2);
        } else {
            max_amsdu = WLAN_TX_AMSDU_NONE;
        }
    } else {
        /* ���ڵ�����, �ر�amsdu����ۺ� */
        max_amsdu = WLAN_TX_AMSDU_NONE;
    }

    return max_amsdu;
}


uint8_t hmac_update_amsdu_num_shenkuo(mac_vap_stru *mac_vap, hmac_performance_stat_stru *performance_stat_params,
    oal_bool_enum_uint8 mu_vap_flag)
{
    wlan_tx_amsdu_enum_uint8 max_amsdu;

    max_amsdu = hmac_adjust_tx_amsdu_num_shenkuo(mac_vap, performance_stat_params->tx_throughput_mbps, mu_vap_flag);

    max_amsdu = hmac_adjust_rx_amsdu_num_shenkuo(mac_vap, performance_stat_params->rx_throughput_mbps,
        performance_stat_params->tx_pps, max_amsdu);

#ifdef _PRE_WLAN_FEATURE_11AX
    if (g_wlan_spec_cfg->feature_11ax_is_open) {
        max_amsdu = oal_min(oal_max(max_amsdu, mac_vap->bit_ofdma_aggr_num), WLAN_TX_AMSDU_BY_8);
    }
#endif

    return max_amsdu;
}
OAL_STATIC uint32_t hmac_get_amsdu_judge_result(uint32_t ul_ret,
    oal_bool_enum_uint8 en_mu_vap_flag, mac_vap_stru *pst_vap1, mac_vap_stru *pst_vap2, uint32_t *up_vap_num)
{
    if ((ul_ret != OAL_SUCC) || (pst_vap1 == NULL) || (en_mu_vap_flag && (pst_vap2 == NULL))) {
        return OAL_FAIL;
    }
#ifdef _PRE_WLAN_CHBA_MGMT
    /* ���vap upʱֻ֧������vap���㷨 */
    if ((g_optimized_feature_switch_bitmap & BIT(CUSTOM_OPTIMIZE_CHBA)) && (*up_vap_num > TWO_UP_VAP_NUMS)) {
        *up_vap_num = TWO_UP_VAP_NUMS;
    }
#endif
    return OAL_SUCC;
}
void hmac_cfg_amsdu_info_to_dmac(mac_vap_stru *mac_vap)
{
    uint32_t ret;
    uint8_t amsdu_maxnum;

    amsdu_maxnum = (uint8_t)wlan_amsdu_level_to_num(g_st_tx_large_amsdu.en_tx_amsdu_level[mac_vap->uc_vap_id]);
    ret = hmac_config_send_event(mac_vap, WLAN_CFGID_DMAC_SYNC_AMSDU_MAXNUM,
        sizeof(uint8_t), (uint8_t *)&amsdu_maxnum);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_ANY,
                         "{hmac_cfg_amsdu_info_to_dmac::hmac_config_send_event failed[%d].}", ret);
        return;
    }
}

void hmac_tx_amsdu_ampdu_switch(uint32_t tx_throughput_mbps, uint32_t rx_throughput_mbps)
{
    mac_device_stru *mac_device = mac_res_get_dev(0);
    mac_vap_stru *mac_vap[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE] = {NULL};
    wlan_tx_amsdu_enum_uint8 tx_amsdu = 0;
    uint32_t up_vap_num = mac_device_calc_up_vap_num(mac_device);
    oal_bool_enum_uint8 mu_vap = (up_vap_num > 1);
    uint32_t ret;
    uint8_t  idx;
    hmac_vap_stru *hmac_vap = NULL;
    hmac_performance_stat_stru performance_stat_params = {tx_throughput_mbps, rx_throughput_mbps,
        g_freq_lock_control.tx_pps};
    /* ������ƻ���֧��Ӳ���ۺ� */
    if (g_st_tx_large_amsdu.uc_host_large_amsdu_en == OAL_FALSE) {
        return;
    }

    if (mu_vap) {
        ret = mac_device_find_up_vaps(mac_device, mac_vap, WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE);
    } else {
        ret = mac_device_find_up_vap(mac_device, &mac_vap[0]);
    }

    if (hmac_get_amsdu_judge_result(ret, mu_vap, mac_vap[0], mac_vap[1], &up_vap_num) == OAL_FAIL) {
        return;
    }
    for (idx = 0; idx < up_vap_num; idx++) {
        tx_amsdu = wlan_chip_tx_update_amsdu_num(mac_vap[idx], (hmac_performance_stat_stru *)&performance_stat_params,
            mu_vap);
        /* �ۺ��쳣��ǰ�ۺϷ�ʽ��ͬ,������ */
        if (tx_amsdu == WLAN_TX_AMSDU_BUTT ||
            g_st_tx_large_amsdu.en_tx_amsdu_level[mac_vap[idx]->uc_vap_id] == tx_amsdu) {
            continue;
        }

        g_st_tx_large_amsdu.en_tx_amsdu_level[mac_vap[idx]->uc_vap_id] = tx_amsdu;
        hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap[idx]->uc_vap_id);
        if ((hmac_vap != NULL) && (hmac_vap->d2h_amsdu_switch == OAL_FALSE)) {
            g_st_tx_large_amsdu.en_tx_amsdu_level[mac_vap[idx]->uc_vap_id] = WLAN_TX_AMSDU_NONE;
        }

        oam_warning_log4(mac_vap[idx]->uc_vap_id, OAM_SF_ANY,
            "{hmac_tx_amsdu_ampdu_switch: up vap num[%d], mu_vap[%d], amsdu level[%d],tx_throught= [%d]!}",
            up_vap_num, mu_vap, tx_amsdu, tx_throughput_mbps);

        hmac_cfg_amsdu_info_to_dmac(mac_vap[idx]);
    }
}
#endif
