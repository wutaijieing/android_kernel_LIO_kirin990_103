/*
 * 版权所有 (c) 华为技术有限公司 2012-2018
 * 功能说明 : mac_vap.c
 * 作    者 : huxiaotong
 * 创建日期 : 2012年10月19日
 */

/* 1 头文件包含 */
#include "mac_vap.h"
#include "mac_mib.h"
#include "hmac_11ax.h"
#include "hmac_11w.h"
#include "hmac_vowifi.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_VAP_INIT_C

typedef enum {
    MAC_VAP_LEGACY_RATE_1M  = 1,
    MAC_VAP_LEGACY_RATE_2M  = 2,
    MAC_VAP_LEGACY_RATE_5M  = 5,
    MAC_VAP_LEGACY_RATE_6M  = 6,
    MAC_VAP_LEGACY_RATE_9M  = 9,
    MAC_VAP_LEGACY_RATE_11M = 11,
    MAC_VAP_LEGACY_RATE_12M = 12,
    MAC_VAP_LEGACY_RATE_18M = 18,
    MAC_VAP_LEGACY_RATE_24M = 24,
    MAC_VAP_LEGACY_RATE_36M = 36,
    MAC_VAP_LEGACY_RATE_48M = 48,
    MAC_VAP_LEGACY_RATE_54M = 54,
} mac_vap_legacy_rate_enum;


void mac_vap_init_mib_11n_txbf(mac_vap_stru *pst_mac_vap)
{
    /* txbf能力信息 注:11n txbf用宏区分 */
#if (defined(_PRE_WLAN_FEATURE_TXBF) && defined(_PRE_WLAN_FEATURE_TXBF_HT))
    mac_device_stru *pst_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_dev == NULL) {
        return;
    }

    mac_mib_set_TransmitStaggerSoundingOptionImplemented(pst_mac_vap, MAC_DEVICE_GET_CAP_SUBFER(pst_dev));
    mac_mib_set_ReceiveStaggerSoundingOptionImplemented(pst_mac_vap, MAC_DEVICE_GET_CAP_SUBFEE(pst_dev));
    mac_mib_set_ExplicitCompressedBeamformingFeedbackOptionImplemented(pst_mac_vap, WLAN_MIB_HT_ECBF_DELAYED);
    mac_mib_set_NumberCompressedBeamformingMatrixSupportAntenna(pst_mac_vap, HT_BFEE_NTX_SUPP_ANTA_NUM);
#else
    mac_mib_set_TransmitStaggerSoundingOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_ReceiveStaggerSoundingOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_ExplicitCompressedBeamformingFeedbackOptionImplemented(pst_mac_vap, WLAN_MIB_HT_ECBF_INCAPABLE);
    mac_mib_set_NumberCompressedBeamformingMatrixSupportAntenna(pst_mac_vap, 1);
#endif
}

void mac_vap_init_11n_rates_extend(mac_vap_stru *pst_mac_vap, mac_device_stru *pst_mac_dev)
{
#ifdef _PRE_WLAN_FEATURE_11AC2G
    if ((pst_mac_vap->en_protocol == WLAN_HT_MODE) &&
        (pst_mac_vap->st_cap_flag.bit_11ac2g == OAL_TRUE) &&
        (pst_mac_vap->st_channel.en_band == WLAN_BAND_2G)) {
        mac_vap_init_11ac_rates(pst_mac_vap, pst_mac_dev);
    }
#endif

#ifdef _PRE_WLAN_FEATURE_M2S
    /* m2s spec模式下，需要根据vap nss能力 + cap是否支持siso，刷新空间流 */
    if (MAC_VAP_SPEC_IS_SW_NEED_M2S_SWITCH(pst_mac_vap) && IS_VAP_SINGLE_NSS(pst_mac_vap)) {
        /* 将MIB值的MCS MAP清零 */
        memset_s(mac_mib_get_SupportedMCSTx(pst_mac_vap), WLAN_HT_MCS_BITMASK_LEN, 0, WLAN_HT_MCS_BITMASK_LEN);
        memset_s(mac_mib_get_SupportedMCSRx(pst_mac_vap), WLAN_HT_MCS_BITMASK_LEN, 0, WLAN_HT_MCS_BITMASK_LEN);

        mac_mib_set_TxMaximumNumberSpatialStreamsSupported(pst_mac_vap, 1);
        mac_mib_set_SupportedMCSRxValue(pst_mac_vap, 0, 0xFF); /* 支持 RX MCS 0-7，8位全置为1 */
        mac_mib_set_SupportedMCSTxValue(pst_mac_vap, 0, 0xFF); /* 支持 TX MCS 0-7，8位全置为1 */

        mac_mib_set_HighestSupportedDataRate(pst_mac_vap, MAC_MAX_RATE_SINGLE_NSS_20M_11N);

        if ((pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40MINUS) ||
            (pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40PLUS)) {
            /* 40M 支持MCS32 */
            mac_mib_set_SupportedMCSRxValue(pst_mac_vap, 4, 0x01); /* 4 MCSRxValue 支持 RX MCS 32,最后一位为1 */
            mac_mib_set_SupportedMCSTxValue(pst_mac_vap, 4, 0x01); /* 4 MCSRxValue 支持 RX MCS 32,最后一位为1 */
            mac_mib_set_HighestSupportedDataRate(pst_mac_vap, MAC_MAX_RATE_SINGLE_NSS_40M_11N);
        }

        oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{mac_vap_init_11n_rates_cb::m2s spec update rate to siso.}");
    }
#endif
}

#ifdef _PRE_WLAN_FEATURE_TXBF_HT
OAL_STATIC OAL_INLINE void mac_vap_init_txbf_ht_cap(mac_vap_stru *pst_vap)
{
    if (OAL_TRUE == mac_mib_get_TransmitStaggerSoundingOptionImplemented(pst_vap)) {
        pst_vap->st_txbf_add_cap.bit_exp_comp_txbf_cap = OAL_TRUE;
    }
    pst_vap->st_txbf_add_cap.bit_imbf_receive_cap = 0;
    pst_vap->st_txbf_add_cap.bit_channel_est_cap = 0;
    pst_vap->st_txbf_add_cap.bit_min_grouping = 0;
    pst_vap->st_txbf_add_cap.bit_csi_bfee_max_rows = 0;
    pst_vap->bit_ap_11ntxbf = 0;
    pst_vap->st_cap_flag.bit_11ntxbf = OAL_TRUE;
}
#endif

void mac_vap_rom_init(mac_vap_stru *pst_mac_vap, mac_cfg_add_vap_param_stru *pst_param)
{
    oal_bool_enum_uint8 temp_flag;
    /* 仅p2p device支持probe req应答模式切换 */
    pst_mac_vap->st_probe_resp_ctrl.en_probe_resp_enable = OAL_FALSE;
    pst_mac_vap->st_probe_resp_ctrl.en_probe_resp_status = MAC_PROBE_RESP_MODE_ACTIVE;
    temp_flag = ((pst_param->en_vap_mode == WLAN_VAP_MODE_BSS_STA) &&
                 (pst_param->en_p2p_mode == WLAN_P2P_DEV_MODE));
    if (temp_flag) {
        pst_mac_vap->st_probe_resp_ctrl.en_probe_resp_enable = (uint8_t)pst_param->probe_resp_enable;
        pst_mac_vap->st_probe_resp_ctrl.en_probe_resp_status = (uint8_t)pst_param->probe_resp_status;
    }
    pst_mac_vap->en_ps_rx_amsdu = OAL_TRUE;
}

uint32_t mac_vap_protocol_init(mac_vap_stru *pst_vap, mac_device_stru *pst_mac_device)
{
    switch (pst_mac_device->en_protocol_cap) {
        case WLAN_PROTOCOL_CAP_LEGACY:
        case WLAN_PROTOCOL_CAP_HT:
            pst_vap->en_protocol = WLAN_HT_MODE;
            break;

        case WLAN_PROTOCOL_CAP_VHT:
            pst_vap->en_protocol = WLAN_VHT_MODE;
            break;
#ifdef _PRE_WLAN_FEATURE_11AX
        case WLAN_PROTOCOL_CAP_HE:
            if (g_wlan_spec_cfg->feature_11ax_is_open) {
                pst_vap->en_protocol = g_pst_mac_device_capability[0].en_11ax_switch ?
                    WLAN_HE_MODE : WLAN_VHT_MODE;
                break;
            }
            oam_warning_log1(pst_vap->uc_vap_id, OAM_SF_CFG,
                "{mac_vap_init::en_protocol_cap[%d] is not supportted.}", pst_mac_device->en_protocol_cap);
            return OAL_ERR_CODE_CONFIG_UNSUPPORT;
#endif

        default:
            oam_warning_log1(pst_vap->uc_vap_id, OAM_SF_CFG,
                "{mac_vap_init::en_protocol_cap[%d] is not supportted.}", pst_mac_device->en_protocol_cap);
            return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    return OAL_SUCC;
}

uint32_t mac_vap_init_band(mac_vap_stru *pst_vap, mac_device_stru *pst_mac_device)
{
    switch (pst_mac_device->en_band_cap) {
        case WLAN_BAND_CAP_2G:
            pst_vap->st_channel.en_band = WLAN_BAND_2G;
            break;

        case WLAN_BAND_CAP_5G:
        case WLAN_BAND_CAP_2G_5G:
            pst_vap->st_channel.en_band = WLAN_BAND_5G;
            break;

        default:
            oam_warning_log1(pst_vap->uc_vap_id, OAM_SF_CFG,
                "{mac_vap_init::en_band_cap[%d] is not supportted.}", pst_mac_device->en_band_cap);
            return OAL_ERR_CODE_CONFIG_UNSUPPORT;
            }
    return OAL_SUCC;
}

void mac_vap_ap_init(mac_vap_stru *vap)
{
    vap->us_assoc_vap_id = g_wlan_spec_cfg->invalid_user_id;
    vap->us_cache_user_id = g_wlan_spec_cfg->invalid_user_id;
    vap->uc_tx_power = WLAN_MAX_TXPOWER;
    vap->st_protection.en_protection_mode = WLAN_PROT_NO;
    vap->st_cap_flag.bit_dsss_cck_mode_40mhz = OAL_TRUE;

    /* 初始化特性标识 */
    vap->st_cap_flag.bit_uapsd = WLAN_FEATURE_UAPSD_IS_OPEN;
    if (vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        vap->st_cap_flag.bit_uapsd = g_uc_uapsd_cap;
    }
    /* 初始化dpd能力 */
    vap->st_cap_flag.bit_dpd_enbale = OAL_TRUE;

    vap->st_cap_flag.bit_dpd_done = OAL_FALSE;
    /* 初始化TDLS prohibited关闭 */
    vap->st_cap_flag.bit_tdls_prohibited = OAL_FALSE;
    /* 初始化TDLS channel switch prohibited关闭 */
    vap->st_cap_flag.bit_tdls_channel_switch_prohibited = OAL_FALSE;

    /* 初始化KeepALive开关 */
    vap->st_cap_flag.bit_keepalive = OAL_TRUE;
    /* 初始化安全特性值 */
    vap->st_cap_flag.bit_wpa = OAL_FALSE;
    vap->st_cap_flag.bit_wpa2 = OAL_FALSE;

    mac_vap_set_peer_obss_scan(vap, OAL_FALSE);

    /* 初始化协议模式与带宽为非法值，需通过配置命令配置 */
    vap->st_channel.en_band = WLAN_BAND_BUTT;
    vap->st_channel.en_bandwidth = WLAN_BAND_WIDTH_BUTT;
    vap->st_channel.uc_chan_number = 0;
    vap->st_channel.ext6g_band = OAL_FALSE;
    vap->en_protocol = WLAN_PROTOCOL_BUTT;

    /* 设置自动保护开启 */
    vap->st_protection.bit_auto_protection = OAL_SWITCH_ON;

    memset_s(vap->ast_app_ie, sizeof(mac_app_ie_stru) * OAL_APP_IE_NUM,
             0, sizeof(mac_app_ie_stru) * OAL_APP_IE_NUM);
    /* 初始化vap wmm开关，默认打开 */
    vap->en_vap_wmm = OAL_TRUE;

    /* 设置初始化rx nss值,之后按协议初始化 */
    vap->en_vap_rx_nss = WLAN_NSS_LIMIT;
#ifdef _PRE_WLAN_FEATURE_DEGRADE_SWITCH
    vap->en_vap_degrade_trx_nss = WLAN_NSS_LIMIT;
#endif
    /* 设置VAP状态为初始状态INIT */
    mac_vap_state_change(vap, MAC_VAP_STATE_INIT);

    /* 清mac vap下的uapsd的状态,否则状态会有残留，导致host device uapsd信息不同步 */
    memset_s(&(vap->st_sta_uapsd_cfg), sizeof(mac_cfg_uapsd_sta_stru),
             0, sizeof(mac_cfg_uapsd_sta_stru));
}

OAL_STATIC OAL_INLINE void mac_vap_update_ip_filter(mac_vap_stru *pst_vap, mac_cfg_add_vap_param_stru *pst_param)
{
    if (IS_STA(pst_vap) && (pst_param->en_p2p_mode == WLAN_LEGACY_VAP_MODE)) {
        /* 仅LEGACY_STA支持 */
        pst_vap->st_cap_flag.bit_ip_filter = OAL_TRUE;
        pst_vap->st_cap_flag.bit_icmp_filter = OAL_TRUE;
    } else {
        pst_vap->st_cap_flag.bit_ip_filter = OAL_FALSE;
        pst_vap->st_cap_flag.bit_icmp_filter = OAL_FALSE;
    }
}

void mac_vap_init_basic(mac_vap_stru *vap, mac_device_stru *mac_device, uint8_t vap_id,
    mac_cfg_add_vap_param_stru *param)
{
    uint32_t loop;

    vap->uc_chip_id = mac_device->uc_chip_id;
    vap->uc_device_id = mac_device->uc_device_id;
    vap->uc_vap_id = vap_id;
    vap->en_vap_mode = param->en_vap_mode;
    vap->en_p2p_mode = param->en_p2p_mode;
    vap->core_id = mac_device->core_id;
    vap->is_primary_vap = param->is_primary_vap;

    vap->bit_has_user_bw_limit = OAL_FALSE;
    vap->bit_vap_bw_limit = 0;
#ifdef _PRE_WLAN_FEATURE_VO_AGGR
    vap->bit_voice_aggr = OAL_TRUE;
#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
    /* 组播聚合场景，普通的vo队列不支持聚合;非组播场景下，支持普通vo队列聚合 */
    vap->bit_voice_aggr = (mac_get_mcast_ampdu_switch() == OAL_FALSE);
#endif
#else
    vap->bit_voice_aggr = OAL_FALSE;
#endif
    vap->uc_random_mac = OAL_FALSE;
    vap->bit_bw_fixed = OAL_FALSE;
    vap->bit_use_rts_threshold = OAL_FALSE;

    oal_set_mac_addr_zero(vap->auc_bssid);

    for (loop = 0; loop < MAC_VAP_USER_HASH_MAX_VALUE; loop++) {
        oal_dlist_init_head(&(vap->ast_user_hash[loop]));
    }

    /* cache user 锁初始化 */
    oal_spin_lock_init(&vap->st_cache_user_lock);
    oal_dlist_init_head(&vap->st_mac_user_list_head);

    /* 初始化支持2.4G 11ac私有增强 */
#ifdef _PRE_WLAN_FEATURE_11AC2G
    vap->st_cap_flag.bit_11ac2g = param->bit_11ac2g_enable;
#endif
    /* 默认APUT不支持随环境进行自动2040带宽切换 */
    vap->st_cap_flag.bit_2040_autoswitch = OAL_FALSE;
    /* 根据定制化刷新2g ht40能力 */
    vap->st_cap_flag.bit_disable_2ght40 = param->bit_disable_capab_2ght40;
    mac_vap_update_ip_filter(vap, param);
    mac_vap_rom_init(vap, param);
}

uint32_t mac_vap_sta_init(mac_vap_stru *mac_vap, mac_device_stru *mac_device)
{
    /* 初始化sta协议模式为11ac */ /* 嵌套深度优化封装 */
    if (mac_vap_protocol_init(mac_vap, mac_device) != OAL_SUCC) {
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    switch (MAC_DEVICE_GET_CAP_BW(mac_device)) {
        case WLAN_BW_CAP_20M:
            mac_vap->st_channel.en_bandwidth = WLAN_BAND_WIDTH_20M;
            break;
        case WLAN_BW_CAP_40M:
            mac_vap->st_channel.en_bandwidth = WLAN_BAND_WIDTH_40MINUS;
            break;
        case WLAN_BW_CAP_80M:
            mac_vap->st_channel.en_bandwidth = WLAN_BAND_WIDTH_80PLUSMINUS;
            break;
#ifdef _PRE_WLAN_FEATURE_160M
        case WLAN_BW_CAP_160M:
            mac_vap->st_channel.en_bandwidth = WLAN_BAND_WIDTH_160PLUSPLUSMINUS;
            break;
#endif
        default:
            oam_error_log1(mac_vap->uc_vap_id, OAM_SF_CFG, "{mac_vap_init::bandwidth_cap[%d] is not supportted.}",
                MAC_DEVICE_GET_CAP_BW(mac_device));
            return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }
    if (mac_vap_init_band(mac_vap, mac_device) != OAL_SUCC) {
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    if (mac_vap_init_by_protocol(mac_vap, mac_vap->en_protocol) != OAL_SUCC) {
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
    mac_vap_init_rates(mac_vap);
    return OAL_SUCC;
}

uint32_t mac_vap_mib_feature_init(mac_vap_stru *vap, mac_device_stru *mac_device, mac_cfg_add_vap_param_stru *param)
{
#ifdef _PRE_WLAN_FEATURE_11AX
    if (g_wlan_spec_cfg->feature_11ax_is_open) {
        mac_vap_11ax_cap_init(vap);
    }
    if (IS_LEGACY_AP(vap)) {
        vap->aput_bss_color_info = 1;
    }
#endif

#ifdef _PRE_WLAN_FEATURE_MBO
    vap->st_mbo_para_info.uc_mbo_enable = g_uc_mbo_switch;
#endif
    /* 初始化mib值 */
    mac_init_mib(vap);
    /* 根据协议规定 ，VHT rx_ampdu_factor应设置为7             */
    mac_mib_set_vht_max_rx_ampdu_factor(vap, MAC_VHT_AMPDU_MAX_LEN_EXP); /* 2^(13+factor)-1字节 */
#if defined(_PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU)
    /* 从定制化获取是否开启AMSDU_AMPDU */
    mac_mib_set_AmsduPlusAmpduActive(vap, g_st_tx_large_amsdu.uc_host_large_amsdu_en);
#endif
    /* 嵌套深度优化封装 */
    mac_vap_init_vowifi(vap, param);
#ifdef _PRE_WLAN_FEATURE_TXBF_HT
    mac_vap_init_txbf_ht_cap(vap);
#endif

#ifdef _PRE_WLAN_FEATURE_1024QAM
    vap->st_cap_flag.bit_1024qam = 1;
#endif

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
    vap->st_cap_flag.bit_opmode = 1;
#endif
    /* sta以最大能力启用 */
    if (vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        return mac_vap_sta_init(vap, mac_device);
    }
    return OAL_SUCC;
}

/*
 * 功能描述  : mac vap init
 * 1.日    期  : 2013年5月29日
 *   作    者  : chenyan
 *   修改内容  : 新生成函数
 */
uint32_t mac_vap_init(mac_vap_stru *pst_vap, uint8_t uc_chip_id, uint8_t uc_device_id, uint8_t uc_vap_id,
    mac_cfg_add_vap_param_stru *pst_param)
{
    wlan_mib_ieee802dot11_stru *pst_mib_info = NULL;
    mac_device_stru *mac_device = mac_res_get_dev(uc_device_id);
    uint8_t *puc_addr = NULL;
    oal_bool_enum_uint8 temp_flag;
    uint32_t ret;

    if (oal_unlikely(mac_device == NULL)) {
        oam_error_log1(0, OAM_SF_ANY, "{mac_vap_init::pst_mac_device[%d] null!}", uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    mac_vap_init_basic(pst_vap, mac_device, uc_vap_id, pst_param);
    switch (pst_vap->en_vap_mode) {
        case WLAN_VAP_MODE_CONFIG:
            pst_vap->uc_init_flag = MAC_VAP_VAILD;  // CFG VAP也需置位保证不重复初始化
            return OAL_SUCC;
        case WLAN_VAP_MODE_BSS_STA:
        case WLAN_VAP_MODE_BSS_AP:
            mac_vap_ap_init(pst_vap);
            break;
        case WLAN_VAP_MODE_WDS:
        case WLAN_VAP_MODE_MONITOER:
        case WLAN_VAP_HW_TEST:
            break;
        default:
            oam_warning_log1(uc_vap_id, OAM_SF_ANY, "{mac_vap_init::invalid vap mode[%d].}", pst_vap->en_vap_mode);
            return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* 申请MIB内存空间，配置VAP没有MIB */
    temp_flag = ((pst_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) ||
        (pst_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) || (pst_vap->en_vap_mode == WLAN_VAP_MODE_WDS));
    if (temp_flag) {
        pst_vap->pst_mib_info = hmac_res_get_mib_info(pst_vap->uc_vap_id);
        if (pst_vap->pst_mib_info == NULL) {
            oam_error_log1(pst_vap->uc_vap_id, OAM_SF_ANY,
                "{mac_vap_init::pst_mib_info alloc null, size[%d].}", sizeof(wlan_mib_ieee802dot11_stru));
            return OAL_ERR_CODE_ALLOC_MEM_FAIL;
        }

        pst_mib_info = pst_vap->pst_mib_info;
        memset_s(pst_mib_info, sizeof(wlan_mib_ieee802dot11_stru), 0, sizeof(wlan_mib_ieee802dot11_stru));

        /* 设置mac地址 */
        puc_addr = mac_mib_get_StationID(pst_vap);
        oal_set_mac_addr(puc_addr, mac_device->auc_hw_addr);
        puc_addr[WLAN_MAC_ADDR_LEN - 1] += uc_vap_id;
        ret = mac_vap_mib_feature_init(pst_vap, mac_device, pst_param);
        if (ret != OAL_SUCC) {
            return ret;
        }
    }
    pst_vap->uc_init_flag = MAC_VAP_VAILD;

    return OAL_SUCC;
}

/*
 * 功能描述  : 初始化wme参数, 除sta之外的模式
 * 1.日    期  : 2012年12月13日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
uint32_t mac_vap_init_wme_param(mac_vap_stru *pst_mac_vap)
{
    const mac_wme_param_stru   *pst_wmm_param = NULL;
    const mac_wme_param_stru   *pst_wmm_param_sta = NULL;
    uint8_t                       uc_ac_type;

    pst_wmm_param = mac_get_wmm_cfg(pst_mac_vap->en_vap_mode);
    if (pst_wmm_param == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    for (uc_ac_type = 0; uc_ac_type < WLAN_WME_AC_BUTT; uc_ac_type++) {
        /* VAP自身的EDCA参数 */
        mac_mib_set_QAPEDCATableIndex(pst_mac_vap, uc_ac_type, uc_ac_type + 1); /* 注: 协议规定取值1 2 3 4 */
        mac_mib_set_QAPEDCATableAIFSN(pst_mac_vap, uc_ac_type, pst_wmm_param[uc_ac_type].aifsn);
        mac_mib_set_QAPEDCATableCWmin(pst_mac_vap, uc_ac_type, pst_wmm_param[uc_ac_type].logcwmin);
        mac_mib_set_QAPEDCATableCWmax(pst_mac_vap, uc_ac_type, pst_wmm_param[uc_ac_type].logcwmax);
        mac_mib_set_QAPEDCATableTXOPLimit(pst_mac_vap, uc_ac_type, pst_wmm_param[uc_ac_type].txop_limit);
    }

    if (pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        /* AP模式时广播给STA的EDCA参数，只在AP模式需要初始化此值，使用WLAN_VAP_MODE_BUTT， */
        pst_wmm_param_sta = mac_get_wmm_cfg(WLAN_VAP_MODE_BUTT);

        for (uc_ac_type = 0; uc_ac_type < WLAN_WME_AC_BUTT; uc_ac_type++) {
            mac_mib_set_EDCATableIndex(pst_mac_vap, uc_ac_type, uc_ac_type + 1); /* 注: 协议规定取值1 2 3 4 */
            mac_mib_set_EDCATableAIFSN(pst_mac_vap, uc_ac_type, pst_wmm_param_sta[uc_ac_type].aifsn);
            mac_mib_set_EDCATableCWmin(pst_mac_vap, uc_ac_type, pst_wmm_param_sta[uc_ac_type].logcwmin);
            mac_mib_set_EDCATableCWmax(pst_mac_vap, uc_ac_type, pst_wmm_param_sta[uc_ac_type].logcwmax);
            mac_mib_set_EDCATableTXOPLimit(pst_mac_vap, uc_ac_type, pst_wmm_param_sta[uc_ac_type].txop_limit);
        }
    }

    return OAL_SUCC;
}

/*
 * 功能描述  : 初始化11ac mib中mcs信息
 * 1.日    期  : 2013年8月26日
 *   作    者  : zhangyu
 *   修改内容  : 新生成函数
 */
void mac_vap_init_11ac_mcs_singlenss(mac_vap_stru *pst_mac_vap,
    wlan_channel_bandwidth_enum_uint8 en_bandwidth)
{
    mac_tx_max_mcs_map_stru         *pst_tx_max_mcs_map;
    mac_rx_max_mcs_map_stru         *pst_rx_max_mcs_map;

    /* 获取mib值指针 */
    pst_rx_max_mcs_map = (mac_rx_max_mcs_map_stru *)(mac_mib_get_ptr_vht_rx_mcs_map(pst_mac_vap));
    pst_tx_max_mcs_map = (mac_tx_max_mcs_map_stru *)(mac_mib_get_ptr_vht_tx_mcs_map(pst_mac_vap));

    /* 20MHz带宽的情况下，支持MCS0-MCS8 */
    if (en_bandwidth == WLAN_BAND_WIDTH_20M) {
#ifdef _PRE_WLAN_FEATURE_11AC_20M_MCS9
        pst_rx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
#else
        pst_rx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS8_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS8_11AC_EACH_NSS;
#endif
        mac_mib_set_us_rx_highest_rate(pst_mac_vap, MAC_MAX_RATE_SINGLE_NSS_20M_11AC);
        mac_mib_set_us_tx_highest_rate(pst_mac_vap, MAC_MAX_RATE_SINGLE_NSS_20M_11AC);
    } else if ((en_bandwidth == WLAN_BAND_WIDTH_40MINUS) || (en_bandwidth == WLAN_BAND_WIDTH_40PLUS)) {
        /* 40MHz带宽的情况下，支持MCS0-MCS9 */
        pst_rx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        mac_mib_set_us_rx_highest_rate(pst_mac_vap, MAC_MAX_RATE_SINGLE_NSS_40M_11AC);
        mac_mib_set_us_tx_highest_rate(pst_mac_vap, MAC_MAX_RATE_SINGLE_NSS_40M_11AC);
    } else if ((en_bandwidth >= WLAN_BAND_WIDTH_80PLUSPLUS) && (en_bandwidth <= WLAN_BAND_WIDTH_80MINUSMINUS)) {
        /* 80MHz带宽的情况下，支持MCS0-MCS9 */
        pst_rx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        mac_mib_set_us_rx_highest_rate(pst_mac_vap, MAC_MAX_RATE_SINGLE_NSS_80M_11AC);
        mac_mib_set_us_tx_highest_rate(pst_mac_vap, MAC_MAX_RATE_SINGLE_NSS_80M_11AC);
#ifdef _PRE_WLAN_FEATURE_160M
    } else if ((en_bandwidth >= WLAN_BAND_WIDTH_160PLUSPLUSPLUS) &&
        (en_bandwidth <= WLAN_BAND_WIDTH_160MINUSMINUSMINUS)) {
        pst_rx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        mac_mib_set_us_rx_highest_rate(pst_mac_vap, MAC_MAX_RATE_SINGLE_NSS_160M_11AC);
        mac_mib_set_us_tx_highest_rate(pst_mac_vap, MAC_MAX_RATE_SINGLE_NSS_160M_11AC);
#endif
    }
}

/*
 * 功能描述  : 初始化11ac mib中mcs信息
 * 1.日    期  : 2013年8月26日
 *   作    者  : zhangyu
 *   修改内容  : 新生成函数
 */
void mac_vap_init_11ac_mcs_doublenss(mac_vap_stru *pst_mac_vap,
    wlan_channel_bandwidth_enum_uint8 en_bandwidth)
{
    mac_tx_max_mcs_map_stru *pst_tx_max_mcs_map;
    mac_rx_max_mcs_map_stru *pst_rx_max_mcs_map;

    /* 获取mib值指针 */
    pst_rx_max_mcs_map = (mac_rx_max_mcs_map_stru *)(mac_mib_get_ptr_vht_rx_mcs_map(pst_mac_vap));
    pst_tx_max_mcs_map = (mac_tx_max_mcs_map_stru *)(mac_mib_get_ptr_vht_tx_mcs_map(pst_mac_vap));

    /* 20MHz带宽的情况下，支持MCS0-MCS8 */
    if (en_bandwidth == WLAN_BAND_WIDTH_20M) {
#ifdef _PRE_WLAN_FEATURE_11AC_20M_MCS9
        pst_rx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_rx_max_mcs_map->us_max_mcs_2ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_2ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
#else
        pst_rx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS8_11AC_EACH_NSS;
        pst_rx_max_mcs_map->us_max_mcs_2ss = MAC_MAX_SUP_MCS8_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS8_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_2ss = MAC_MAX_SUP_MCS8_11AC_EACH_NSS;
#endif
        mac_mib_set_us_rx_highest_rate(pst_mac_vap, MAC_MAX_RATE_DOUBLE_NSS_20M_11AC);
        mac_mib_set_us_tx_highest_rate(pst_mac_vap, MAC_MAX_RATE_DOUBLE_NSS_20M_11AC);
    } else if ((en_bandwidth == WLAN_BAND_WIDTH_40MINUS) || (en_bandwidth == WLAN_BAND_WIDTH_40PLUS)) {
        /* 40MHz带宽的情况下，支持MCS0-MCS9 */
        pst_rx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_rx_max_mcs_map->us_max_mcs_2ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_2ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        mac_mib_set_us_rx_highest_rate(pst_mac_vap, MAC_MAX_RATE_DOUBLE_NSS_40M_11AC);
        mac_mib_set_us_tx_highest_rate(pst_mac_vap, MAC_MAX_RATE_DOUBLE_NSS_40M_11AC);
    } else if ((en_bandwidth >= WLAN_BAND_WIDTH_80PLUSPLUS) && (en_bandwidth <= WLAN_BAND_WIDTH_80MINUSMINUS)) {
        /* 80MHz带宽的情况下，支持MCS0-MCS9 */
        pst_rx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_rx_max_mcs_map->us_max_mcs_2ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_2ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        mac_mib_set_us_rx_highest_rate(pst_mac_vap, MAC_MAX_RATE_DOUBLE_NSS_80M_11AC);
        mac_mib_set_us_tx_highest_rate(pst_mac_vap, MAC_MAX_RATE_DOUBLE_NSS_80M_11AC);
#ifdef _PRE_WLAN_FEATURE_160M
    } else if ((en_bandwidth >= WLAN_BAND_WIDTH_160PLUSPLUSPLUS) &&
        (en_bandwidth <= WLAN_BAND_WIDTH_160MINUSMINUSMINUS)) {
        pst_rx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_rx_max_mcs_map->us_max_mcs_2ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_2ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        mac_mib_set_us_rx_highest_rate(pst_mac_vap, MAC_MAX_RATE_DOUBLE_NSS_160M_11AC);
        mac_mib_set_us_tx_highest_rate(pst_mac_vap, MAC_MAX_RATE_DOUBLE_NSS_160M_11AC);
#endif
    }
}


/*
 * 功能描述  : 初始化11a 11g速率
 * 1.日    期  : 2013年7月31日
 *   作    者  : zhangyu
 *   修改内容  : 新生成函数
 */
void mac_vap_init_legacy_rates(mac_vap_stru *pst_vap, mac_data_rate_stru *pst_rates)
{
    uint8_t                      uc_rate_index;
    uint8_t                      uc_curr_rate_index = 0;
    mac_data_rate_stru            *puc_orig_rate = NULL;
    mac_data_rate_stru            *puc_curr_rate = NULL;
    uint8_t                      uc_rates_num;
    int32_t                      l_ret = EOK;

    /* 初始化速率集 */
    uc_rates_num = MAC_DATARATES_PHY_80211G_NUM;

    /* 初始化速率个数，基本速率个数，非基本速率个数 */
    pst_vap->st_curr_sup_rates.st_rate.uc_rs_nrates = MAC_NUM_DR_802_11A;
    pst_vap->st_curr_sup_rates.uc_br_rate_num       = MAC_NUM_BR_802_11A;
    pst_vap->st_curr_sup_rates.uc_nbr_rate_num      = MAC_NUM_NBR_802_11A;
    pst_vap->st_curr_sup_rates.uc_min_rate          = MAC_VAP_LEGACY_RATE_6M;
    pst_vap->st_curr_sup_rates.uc_max_rate          = MAC_VAP_LEGACY_RATE_24M;

    /* 将速率拷贝到VAP结构体下的速率集中 */
    for (uc_rate_index = 0; uc_rate_index < uc_rates_num; uc_rate_index++) {
        puc_orig_rate = &pst_rates[uc_rate_index];
        puc_curr_rate = &(pst_vap->st_curr_sup_rates.st_rate.ast_rs_rates[uc_curr_rate_index]);

        /* Basic Rates */
        if ((puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_6M) ||
            (puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_12M) ||
            (puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_24M)) {
            l_ret = memcpy_s(puc_curr_rate, sizeof(mac_data_rate_stru), puc_orig_rate, sizeof(mac_data_rate_stru));
            puc_curr_rate->uc_mac_rate |= 0x80;
            uc_curr_rate_index++;
        } else if ((puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_9M) ||
            (puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_18M) ||
            (puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_36M) ||
            (puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_48M) ||
            (puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_54M)) {
            /* Non-basic rates */
            l_ret = memcpy_s(puc_curr_rate, sizeof(mac_data_rate_stru), puc_orig_rate, sizeof(mac_data_rate_stru));
            uc_curr_rate_index++;
        }
        if (l_ret != EOK) {
            oam_error_log0(0, OAM_SF_ANY, "mac_vap_init_legacy_rates::memcpy fail!");
            return;
        }

        if (uc_curr_rate_index == pst_vap->st_curr_sup_rates.st_rate.uc_rs_nrates) {
            break;
        }
    }
}

/*
 * 功能描述  : 初始化11b速率
 * 1.日    期  : 2013年7月31日
 *   作    者  : zhangyu
 *   修改内容  : 新生成函数
 */
void mac_vap_init_11b_rates(mac_vap_stru *pst_vap, mac_data_rate_stru *pst_rates)
{
    uint8_t                      uc_rate_index;
    uint8_t                      uc_curr_rate_index = 0;
    mac_data_rate_stru            *puc_orig_rate = NULL;
    mac_data_rate_stru            *puc_curr_rate = NULL;
    uint8_t                      uc_rates_num;
    int32_t                      l_ret = EOK;

    /* 初始化速率集 */
    uc_rates_num = MAC_DATARATES_PHY_80211G_NUM;

    /* 初始化速率个数，基本速率个数，非基本速率个数 */
    pst_vap->st_curr_sup_rates.st_rate.uc_rs_nrates = MAC_NUM_DR_802_11B;
    pst_vap->st_curr_sup_rates.uc_br_rate_num       = 0;
    pst_vap->st_curr_sup_rates.uc_nbr_rate_num      = MAC_NUM_NBR_802_11B;
    pst_vap->st_curr_sup_rates.uc_min_rate          = MAC_VAP_LEGACY_RATE_1M;
    pst_vap->st_curr_sup_rates.uc_max_rate          = MAC_VAP_LEGACY_RATE_2M;

    /* 将速率拷贝到VAP结构体下的速率集中 */
    for (uc_rate_index = 0; uc_rate_index < uc_rates_num; uc_rate_index++) {
        puc_orig_rate = &pst_rates[uc_rate_index];
        puc_curr_rate = &(pst_vap->st_curr_sup_rates.st_rate.ast_rs_rates[uc_curr_rate_index]);

        /* Basic Rates */
        if ((puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_1M) || (puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_2M) ||
            ((pst_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) && ((puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_5M) ||
            (puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_11M)))) {
            pst_vap->st_curr_sup_rates.uc_br_rate_num++;
            l_ret = memcpy_s(puc_curr_rate, sizeof(mac_data_rate_stru), puc_orig_rate, sizeof(mac_data_rate_stru));
            puc_curr_rate->uc_mac_rate |= 0x80;
            uc_curr_rate_index++;
        } else if ((pst_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) &&
            ((puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_5M) ||
            (puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_11M))) {
            /* Non-basic rates */
            l_ret = memcpy_s(puc_curr_rate, sizeof(mac_data_rate_stru), puc_orig_rate, sizeof(mac_data_rate_stru));
            uc_curr_rate_index++;
        } else {
            continue;
        }

        if (l_ret != EOK) {
            oam_error_log0(0, OAM_SF_ANY, "mac_vap_init_11b_rates::memcpy fail!");
            return;
        }

        if (uc_curr_rate_index == pst_vap->st_curr_sup_rates.st_rate.uc_rs_nrates) {
            break;
        }
    }
}

/*
 * 功能描述  : 初始化11b速率
 * 1.日    期  : 2013年7月31日
 *   作    者  : zhangyu
 *   修改内容  : 新生成函数
 */
void mac_vap_init_11g_mixed_one_rates(mac_vap_stru *pst_vap, mac_data_rate_stru *pst_rates)
{
    uint8_t                      uc_rate_index;
    mac_data_rate_stru            *puc_orig_rate = NULL;
    mac_data_rate_stru            *puc_curr_rate = NULL;
    uint8_t                      uc_rates_num;
    int32_t                      l_ret;

    /* 初始化速率集 */
    uc_rates_num = MAC_DATARATES_PHY_80211G_NUM;

    /* 初始化速率个数，基本速率个数，非基本速率个数 */
    pst_vap->st_curr_sup_rates.st_rate.uc_rs_nrates = MAC_NUM_DR_802_11G_MIXED;
    pst_vap->st_curr_sup_rates.uc_br_rate_num       = MAC_NUM_BR_802_11G_MIXED_ONE;
    pst_vap->st_curr_sup_rates.uc_nbr_rate_num      = MAC_NUM_NBR_802_11G_MIXED_ONE;
    pst_vap->st_curr_sup_rates.uc_min_rate          = MAC_VAP_LEGACY_RATE_1M;
    pst_vap->st_curr_sup_rates.uc_max_rate          = MAC_VAP_LEGACY_RATE_11M;

    /* 将速率拷贝到VAP结构体下的速率集中 */
    for (uc_rate_index = 0; uc_rate_index < uc_rates_num; uc_rate_index++) {
        puc_orig_rate = &pst_rates[uc_rate_index];
        puc_curr_rate = &(pst_vap->st_curr_sup_rates.st_rate.ast_rs_rates[uc_rate_index]);

        l_ret = memcpy_s(puc_curr_rate, sizeof(mac_data_rate_stru), puc_orig_rate, sizeof(mac_data_rate_stru));
        if (l_ret != EOK) {
            oam_error_log0(0, OAM_SF_ANY, "mac_vap_init_11g_mixed_one_rates::memcpy fail!");
            return;
        }

        /* Basic Rates */
        if ((puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_1M) ||
            (puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_2M) ||
            (puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_5M) ||
            (puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_11M)) {
            puc_curr_rate->uc_mac_rate |= 0x80;
        }
    }
}

/*
 * 功能描述  : 初始化11g mixed two速率
 * 1.日    期  : 2013年7月31日
 *   作    者  : zhangyu
 *   修改内容  : 新生成函数
 */
void mac_vap_init_11g_mixed_two_rates(mac_vap_stru *pst_vap, mac_data_rate_stru *pst_rates)
{
    uint8_t                      uc_rate_index;
    mac_data_rate_stru            *puc_orig_rate = NULL;
    mac_data_rate_stru            *puc_curr_rate = NULL;
    uint8_t                      uc_rates_num;
    int32_t                      l_ret;

    /* 初始化速率集 */
    uc_rates_num = MAC_DATARATES_PHY_80211G_NUM;

    /* 初始化速率个数，基本速率个数，非基本速率个数 */
    pst_vap->st_curr_sup_rates.st_rate.uc_rs_nrates = MAC_NUM_DR_802_11G_MIXED;
    pst_vap->st_curr_sup_rates.uc_br_rate_num       = MAC_NUM_BR_802_11G_MIXED_TWO;
    pst_vap->st_curr_sup_rates.uc_nbr_rate_num      = MAC_NUM_NBR_802_11G_MIXED_TWO;
    pst_vap->st_curr_sup_rates.uc_min_rate          = MAC_VAP_LEGACY_RATE_1M;
    pst_vap->st_curr_sup_rates.uc_max_rate          = MAC_VAP_LEGACY_RATE_24M;

    /* 将速率拷贝到VAP结构体下的速率集中 */
    for (uc_rate_index = 0; uc_rate_index < uc_rates_num; uc_rate_index++) {
        puc_orig_rate = &pst_rates[uc_rate_index];
        puc_curr_rate = &(pst_vap->st_curr_sup_rates.st_rate.ast_rs_rates[uc_rate_index]);

        l_ret = memcpy_s(puc_curr_rate, sizeof(mac_data_rate_stru), puc_orig_rate, sizeof(mac_data_rate_stru));
        if (l_ret != EOK) {
            oam_error_log0(0, OAM_SF_ANY, "mac_vap_init_11g_mixed_two_rates::memcpy fail!");
            return;
        }

        /* Basic Rates */
        if ((puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_1M) || (puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_2M) ||
            (puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_5M) || (puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_11M) ||
            (puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_6M) || (puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_12M) ||
            (puc_orig_rate->uc_mbps == MAC_VAP_LEGACY_RATE_24M)) {
            puc_curr_rate->uc_mac_rate |= 0x80;
        }
    }
}


OAL_STATIC void mac_vap_init_11n_40m_rates(mac_vap_stru *mac_vap, wlan_nss_enum_uint8 nss_num)
{
    /* 40M 支持MCS32 */
    if ((mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40MINUS) ||
        (mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40PLUS)) {
        mac_mib_set_SupportedMCSRxValue(mac_vap, 4, 0x01); /* 4 mcsindex, 支持 RX MCS 32,最后一位为1 */
        mac_mib_set_SupportedMCSTxValue(mac_vap, 4, 0x01); /* 4 mcsindex, 支持 TX MCS 32,最后一位为1 */
        switch (nss_num) {
            case WLAN_SINGLE_NSS:
                mac_mib_set_HighestSupportedDataRate(mac_vap, MAC_MAX_RATE_SINGLE_NSS_40M_11N);
                break;
            case WLAN_DOUBLE_NSS:
                mac_mib_set_HighestSupportedDataRate(mac_vap, MAC_MAX_RATE_DOUBLE_NSS_40M_11N);
                break;
            case WLAN_TRIPLE_NSS:
                mac_mib_set_HighestSupportedDataRate(mac_vap, MAC_MAX_RATE_TRIPLE_NSS_40M_11N);
                break;
            case WLAN_FOUR_NSS:
                mac_mib_set_HighestSupportedDataRate(mac_vap, MAC_MAX_RATE_FOUR_NSS_40M_11N);
                break;
            default:
                break;
        }
    }
}


void mac_vap_init_11n_rates(mac_vap_stru *mac_vap, mac_device_stru *mac_dev)
{
    wlan_nss_enum_uint8 nss_num_tx = 0;
    wlan_nss_enum_uint8 nss_num_rx = 0;
    uint8_t nss_idx;
    uint32_t high_rate[] = {
        (uint32_t)MAC_MAX_RATE_SINGLE_NSS_20M_11N,
        (uint32_t)MAC_MAX_RATE_DOUBLE_NSS_20M_11N,
        (uint32_t)MAC_MAX_RATE_TRIPLE_NSS_20M_11N,
        (uint32_t)MAC_MAX_RATE_FOUR_NSS_20M_11N,
    };
    mac_vap_ini_get_nss_num(mac_dev, &nss_num_rx, &nss_num_tx);

    /* MCS相关MIB值初始化 */
    mac_mib_set_TxMCSSetDefined(mac_vap, OAL_TRUE);
    mac_mib_set_TxUnequalModulationSupported(mac_vap, OAL_FALSE);
    nss_num_rx == nss_num_tx ? mac_mib_set_TxRxMCSSetNotEqual(mac_vap, OAL_FALSE) :
        mac_mib_set_TxRxMCSSetNotEqual(mac_vap, OAL_TRUE);

    /* 将MIB值的MCS MAP清零 */
    memset_s(mac_mib_get_SupportedMCSTx(mac_vap), WLAN_HT_MCS_BITMASK_LEN, 0, WLAN_HT_MCS_BITMASK_LEN);
    memset_s(mac_mib_get_SupportedMCSRx(mac_vap), WLAN_HT_MCS_BITMASK_LEN, 0, WLAN_HT_MCS_BITMASK_LEN);

    mac_mib_set_TxMaximumNumberSpatialStreamsSupported(mac_vap, nss_num_tx);

    /* 设置TX MCS */
    for (nss_idx = 0; nss_idx <= nss_num_tx; nss_idx++) {
        mac_mib_set_SupportedMCSTxValue(mac_vap, nss_idx, 0xFF);
    }

    /* 设置RX MCS */
    for (nss_idx = 0; nss_idx <= nss_num_rx; nss_idx++) {
        mac_mib_set_SupportedMCSRxValue(mac_vap, nss_idx, 0xFF);
    }
    nss_num_tx > nss_num_rx ? mac_mib_set_HighestSupportedDataRate(mac_vap, high_rate[nss_num_tx]) :
        mac_mib_set_HighestSupportedDataRate(mac_vap, high_rate[nss_num_rx]);

    mac_vap_init_11n_40m_rates(mac_vap, oal_max(nss_num_tx, nss_num_rx));
    mac_vap_init_11n_rates_extend(mac_vap, mac_dev);
}

OAL_STATIC void mac_vap_init_11ac_highest_rate(mac_vap_stru *mac_vap,
    wlan_channel_bandwidth_enum_uint8 en_bandwidth, wlan_nss_enum_uint8 nss_num_rx, wlan_nss_enum_uint8 nss_num_tx)
{
    uint32_t high_rate_20m[] = {
        (uint32_t)MAC_MAX_RATE_SINGLE_NSS_20M_11AC,
        (uint32_t)MAC_MAX_RATE_DOUBLE_NSS_20M_11AC,
        (uint32_t)MAC_MAX_RATE_TRIPLE_NSS_20M_11AC,
        (uint32_t)MAC_MAX_RATE_FOUR_NSS_20M_11AC,
    };
    uint32_t high_rate_40m[] = {
        (uint32_t)MAC_MAX_RATE_SINGLE_NSS_40M_11AC,
        (uint32_t)MAC_MAX_RATE_DOUBLE_NSS_40M_11AC,
        (uint32_t)MAC_MAX_RATE_TRIPLE_NSS_40M_11AC,
        (uint32_t)MAC_MAX_RATE_FOUR_NSS_40M_11AC,
    };
    uint32_t high_rate_80m[] = {
        (uint32_t)MAC_MAX_RATE_SINGLE_NSS_80M_11AC,
        (uint32_t)MAC_MAX_RATE_DOUBLE_NSS_80M_11AC,
        (uint32_t)MAC_MAX_RATE_TRIPLE_NSS_80M_11AC,
        (uint32_t)MAC_MAX_RATE_FOUR_NSS_80M_11AC,
    };
    uint32_t high_rate_160m[] = {
        (uint32_t)MAC_MAX_RATE_SINGLE_NSS_160M_11AC,
        (uint32_t)MAC_MAX_RATE_DOUBLE_NSS_160M_11AC,
        (uint32_t)MAC_MAX_RATE_TRIPLE_NSS_160M_11AC,
        (uint32_t)MAC_MAX_RATE_FOUR_NSS_160M_11AC,
    };

    if (en_bandwidth == WLAN_BAND_WIDTH_20M) {
        mac_mib_set_us_rx_highest_rate(mac_vap, high_rate_20m[nss_num_rx]);
        mac_mib_set_us_tx_highest_rate(mac_vap, high_rate_20m[nss_num_tx]);
    } else if ((en_bandwidth == WLAN_BAND_WIDTH_40MINUS) || (en_bandwidth == WLAN_BAND_WIDTH_40PLUS)) {
        mac_mib_set_us_rx_highest_rate(mac_vap, high_rate_40m[nss_num_rx]);
        mac_mib_set_us_tx_highest_rate(mac_vap, high_rate_40m[nss_num_tx]);
    } else if ((en_bandwidth >= WLAN_BAND_WIDTH_80PLUSPLUS) && (en_bandwidth <= WLAN_BAND_WIDTH_80MINUSMINUS)) {
        mac_mib_set_us_rx_highest_rate(mac_vap, high_rate_80m[nss_num_rx]);
        mac_mib_set_us_tx_highest_rate(mac_vap, high_rate_80m[nss_num_tx]);
#ifdef _PRE_WLAN_FEATURE_160M
    } else if ((en_bandwidth >= WLAN_BAND_WIDTH_160PLUSPLUSPLUS) &&
        (en_bandwidth <= WLAN_BAND_WIDTH_160MINUSMINUSMINUS)) {
        mac_mib_set_us_rx_highest_rate(mac_vap, high_rate_160m[nss_num_rx]);
        mac_mib_set_us_tx_highest_rate(mac_vap, high_rate_160m[nss_num_tx]);
#endif
    }
}


OAL_STATIC void mac_set_tx_rx_vht_mcs(mac_max_mcs_map_stru *tx_rx_vht_mcs, uint8_t nss_num, uint8_t support_nss)
{
    tx_rx_vht_mcs->us_max_mcs_1ss = support_nss;
    tx_rx_vht_mcs->us_max_mcs_2ss = (nss_num >= WLAN_DOUBLE_NSS ? support_nss : MAC_MAX_UNSUP_MCS_11AC_EACH_NSS);
    tx_rx_vht_mcs->us_max_mcs_3ss = (nss_num >= WLAN_TRIPLE_NSS ? support_nss : MAC_MAX_UNSUP_MCS_11AC_EACH_NSS);
    tx_rx_vht_mcs->us_max_mcs_4ss = (nss_num >= WLAN_FOUR_NSS ? support_nss : MAC_MAX_UNSUP_MCS_11AC_EACH_NSS);
}

/*
 * 函 数 名  : mac_vap_init_11ac_rates
 * 功能描述  : 初始化11ac速率
 * 1.日    期  : 2013年7月31日
 *   作    者  : zhangyu
 *   修改内容  : 新生成函数
 */
void mac_vap_init_11ac_rates(mac_vap_stru *mac_vap, mac_device_stru *mac_dev)
{
    wlan_nss_enum_uint8 nss_num_tx = 0;
    wlan_nss_enum_uint8 nss_num_rx = 0;
    mac_tx_max_mcs_map_stru *tx_max_mcs_map = NULL;
    mac_rx_max_mcs_map_stru *rx_max_mcs_map = NULL;

    /* 先将TX RX MCSMAP初始化为所有空间流都不支持 0xFFFF */
    mac_mib_set_vht_rx_mcs_map(mac_vap, 0xFFFF);
    mac_mib_set_vht_tx_mcs_map(mac_vap, 0xFFFF);

    mac_vap_ini_get_nss_num(mac_dev, &nss_num_rx, &nss_num_tx);

    /* 获取mib值指针 */
    rx_max_mcs_map = (mac_rx_max_mcs_map_stru *)(mac_mib_get_ptr_vht_rx_mcs_map(mac_vap));
    tx_max_mcs_map = (mac_tx_max_mcs_map_stru *)(mac_mib_get_ptr_vht_tx_mcs_map(mac_vap));

    /* 填rx_mcs能力 */
    mac_set_tx_rx_vht_mcs((mac_max_mcs_map_stru *)rx_max_mcs_map, nss_num_rx, (uint8_t)MAC_MAX_SUP_MCS9_11AC_EACH_NSS);
    /* 填tx_mcs能力 */
    mac_set_tx_rx_vht_mcs((mac_max_mcs_map_stru *)tx_max_mcs_map, nss_num_tx, (uint8_t)MAC_MAX_SUP_MCS9_11AC_EACH_NSS);

    /* 20MHz带宽的情况下，支持MCS0-MCS8 */
    if (mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_20M) {
#ifndef _PRE_WLAN_FEATURE_11AC_20M_MCS9
        /* 填rx_mcs能力 */
        mac_set_tx_rx_vht_mcs((mac_max_mcs_map_stru *)rx_max_mcs_map, nss_num_rx,
            (uint8_t)MAC_MAX_SUP_MCS8_11AC_EACH_NSS);
        /* 填tx_mcs能力 */
        mac_set_tx_rx_vht_mcs((mac_max_mcs_map_stru *)tx_max_mcs_map, nss_num_tx,
            (uint8_t)MAC_MAX_SUP_MCS8_11AC_EACH_NSS);
#endif
    }
    mac_vap_init_11ac_highest_rate(mac_vap, mac_vap->st_channel.en_bandwidth, nss_num_rx, nss_num_tx);
#ifdef _PRE_WLAN_FEATURE_M2S
    /* m2s spec模式下，需要根据vap nss能力 + cap是否支持siso，刷新空间流 */
    if (MAC_VAP_SPEC_IS_SW_NEED_M2S_SWITCH(mac_vap) && IS_VAP_SINGLE_NSS(mac_vap)) {
        /* 先将TX RX MCSMAP初始化为所有空间流都不支持 0xFFFF */
        mac_mib_set_vht_rx_mcs_map(mac_vap, 0xFFFF);
        mac_mib_set_vht_tx_mcs_map(mac_vap, 0xFFFF);

        /* 1个空间流的情况 */
        mac_vap_init_11ac_mcs_singlenss(mac_vap, mac_vap->st_channel.en_bandwidth);

        oam_warning_log0(mac_vap->uc_vap_id, OAM_SF_ANY, "{mac_vap_init_11ac_rates::m2s spec update rate to siso.}");
    }
#endif
}


void mac_vap_init_p2p_rates(mac_vap_stru *pst_vap, wlan_protocol_enum_uint8 en_vap_protocol,
    mac_data_rate_stru *pst_rates)
{
    mac_device_stru *pst_mac_dev;
    int32_t        l_ret;

    pst_mac_dev = mac_res_get_dev(pst_vap->uc_device_id);
    if (pst_mac_dev == NULL) {
        oam_error_log1(pst_vap->uc_vap_id, OAM_SF_ANY,
                       "{mac_vap_init_p2p_rates::pst_mac_dev[%d] null.}",
                       pst_vap->uc_device_id);

        return;
    }

    mac_vap_init_legacy_rates(pst_vap, pst_rates);

    l_ret = memcpy_s(&pst_vap->ast_sta_sup_rates_ie[WLAN_BAND_5G],
                     sizeof(mac_curr_rateset_stru),
                     &pst_vap->st_curr_sup_rates,
                     sizeof(pst_vap->st_curr_sup_rates));
    l_ret += memcpy_s(&pst_vap->ast_sta_sup_rates_ie[WLAN_BAND_2G],
                      sizeof(mac_curr_rateset_stru),
                      &pst_vap->st_curr_sup_rates,
                      sizeof(pst_vap->st_curr_sup_rates));
    if (l_ret != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "mac_vap_init_p2p_rates::memcpy fail!");
        return;
    }
    mac_vap_init_11n_rates(pst_vap, pst_mac_dev);
    if (en_vap_protocol == WLAN_VHT_MODE) {
        mac_vap_init_11ac_rates(pst_vap, pst_mac_dev);
    }
#ifdef _PRE_WLAN_FEATURE_11AX
    if (g_wlan_spec_cfg->feature_11ax_is_open) {
        if (en_vap_protocol == WLAN_HE_MODE) {
            mac_vap_init_11ac_rates(pst_vap, pst_mac_dev);
            mac_vap_init_11ax_rates(pst_vap, pst_mac_dev);
        }
    }
#endif
}

#ifdef _PRE_WLAN_FEATURE_11AX
void mac_vap_init_rates_in_he_protocol(mac_vap_stru *vap, mac_device_stru *mac_dev, mac_data_rate_stru *rates)
{
    if (vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA) {
        /* 用于STA全信道扫描 5G时 填写支持速率集ie */
        mac_vap_init_legacy_rates(vap, rates);
        memcpy_s(&vap->ast_sta_sup_rates_ie[WLAN_BAND_5G], sizeof(mac_curr_rateset_stru),
                 &vap->st_curr_sup_rates, sizeof(vap->st_curr_sup_rates));

        /* 用于STA全信道扫描 2G时 填写支持速率集ie */
        mac_vap_init_11g_mixed_one_rates(vap, rates);
        memcpy_s(&vap->ast_sta_sup_rates_ie[WLAN_BAND_2G], sizeof(mac_curr_rateset_stru),
                 &vap->st_curr_sup_rates, sizeof(vap->st_curr_sup_rates));
        // 以下三个语句以及feature_11ax_is_open重复，待进一步整改
        mac_vap_init_11n_rates(vap, mac_dev);
        mac_vap_init_11ac_rates(vap, mac_dev);
        mac_vap_init_11ax_rates(vap, mac_dev);
    } else if (vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
        if (vap->st_channel.en_band == WLAN_BAND_2G) {
            mac_vap_init_11g_mixed_one_rates(vap, rates);
        } else {
            mac_vap_init_legacy_rates(vap, rates);
        }
        mac_vap_init_11n_rates(vap, mac_dev);
        mac_vap_init_11ac_rates(vap, mac_dev);
        mac_vap_init_11ax_rates(vap, mac_dev);
    }
}
#endif

void mac_vap_init_rates_by_protocol(mac_vap_stru *pst_vap,
    wlan_protocol_enum_uint8 en_vap_protocol, mac_data_rate_stru *pst_rates)
{
    mac_device_stru *pst_mac_dev = (mac_device_stru *)mac_res_get_dev(pst_vap->uc_device_id);
    if (pst_mac_dev == NULL) {
        return;
    }

    /* STA模式默认协议模式是11ac，初始化速率集为所有速率集 */
    if (!IS_LEGACY_VAP(pst_vap)) {
        mac_vap_init_p2p_rates(pst_vap, en_vap_protocol, pst_rates);
        return;
    }

#ifdef _PRE_WLAN_FEATURE_11AX
    if (g_wlan_spec_cfg->feature_11ax_is_open && en_vap_protocol == WLAN_HE_MODE) {
        mac_vap_init_rates_in_he_protocol(pst_vap, pst_mac_dev, pst_rates);
    }
#endif
    if (pst_vap->en_vap_mode == WLAN_VAP_MODE_BSS_STA && en_vap_protocol == WLAN_VHT_MODE) {
        /* 用于STA全信道扫描 5G时 填写支持速率集ie */
        mac_vap_init_legacy_rates(pst_vap, pst_rates);
        memcpy_s(&pst_vap->ast_sta_sup_rates_ie[WLAN_BAND_5G], sizeof(mac_curr_rateset_stru),
                 &pst_vap->st_curr_sup_rates, sizeof(pst_vap->st_curr_sup_rates));

        /* 用于STA全信道扫描 2G时 填写支持速率集ie */
        mac_vap_init_11g_mixed_one_rates(pst_vap, pst_rates);
        memcpy_s(&pst_vap->ast_sta_sup_rates_ie[WLAN_BAND_2G], sizeof(mac_curr_rateset_stru),
                 &pst_vap->st_curr_sup_rates, sizeof(pst_vap->st_curr_sup_rates));

        mac_vap_init_11n_rates(pst_vap, pst_mac_dev);
        mac_vap_init_11ac_rates(pst_vap, pst_mac_dev);
    } else if (oal_value_eq_any2(en_vap_protocol, WLAN_VHT_ONLY_MODE, WLAN_VHT_MODE)) {
#ifdef _PRE_WLAN_FEATURE_11AC2G
        (pst_vap->st_channel.en_band == WLAN_BAND_2G) ? mac_vap_init_11g_mixed_one_rates(pst_vap, pst_rates) :
            mac_vap_init_legacy_rates(pst_vap, pst_rates);
#else
        mac_vap_init_legacy_rates(pst_vap, pst_rates);
#endif
        mac_vap_init_11n_rates(pst_vap, pst_mac_dev);
        mac_vap_init_11ac_rates(pst_vap, pst_mac_dev);
    } else if (oal_value_eq_any2(en_vap_protocol, WLAN_HT_ONLY_MODE, WLAN_HT_MODE)) {
        if (pst_vap->st_channel.en_band == WLAN_BAND_5G) {
            mac_vap_init_legacy_rates(pst_vap, pst_rates);
        } else if (pst_vap->st_channel.en_band == WLAN_BAND_2G) {
            mac_vap_init_11g_mixed_one_rates(pst_vap, pst_rates);
        }

        mac_vap_init_11n_rates(pst_vap, pst_mac_dev);
    } else if (oal_value_eq_any2(en_vap_protocol, WLAN_LEGACY_11A_MODE, WLAN_LEGACY_11G_MODE)) {
        mac_vap_init_legacy_rates(pst_vap, pst_rates);
    } else if (en_vap_protocol == WLAN_LEGACY_11B_MODE) {
        mac_vap_init_11b_rates(pst_vap, pst_rates);
    } else if (en_vap_protocol == WLAN_MIXED_ONE_11G_MODE) {
        mac_vap_init_11g_mixed_one_rates(pst_vap, pst_rates);
    } else if (en_vap_protocol == WLAN_MIXED_TWO_11G_MODE) {
        mac_vap_init_11g_mixed_two_rates(pst_vap, pst_rates);
    }
}


void mac_vap_init_rates(mac_vap_stru *pst_vap)
{
    mac_device_stru               *pst_mac_dev;
    wlan_protocol_enum_uint8       en_vap_protocol;
    mac_data_rate_stru            *pst_rates = NULL;

    pst_mac_dev = mac_res_get_dev(pst_vap->uc_device_id);
    if (pst_mac_dev == NULL) {
        oam_error_log1(pst_vap->uc_vap_id, OAM_SF_ANY,
            "{mac_vap_init_rates::pst_mac_dev[%d] null.}", pst_vap->uc_device_id);

        return;
    }

    /* 初始化速率集 */
    pst_rates = mac_device_get_all_rates(pst_mac_dev);

    en_vap_protocol = pst_vap->en_protocol;

    mac_vap_init_rates_by_protocol(pst_vap, en_vap_protocol, pst_rates);
}

/*
 * 功能描述  : legacy协议初始化vap能力
 * 1.日    期  : 2013年11月18日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
void mac_vap_cap_init_legacy(mac_vap_stru *pst_mac_vap)
{
    pst_mac_vap->st_cap_flag.bit_rifs_tx_on = OAL_FALSE;

    /* 非VHT不使能 txop ps */
    pst_mac_vap->st_cap_flag.bit_txop_ps = OAL_FALSE;

    if (pst_mac_vap->pst_mib_info != NULL) {
        mac_mib_set_txopps(pst_mac_vap, OAL_FALSE);
    }

    return;
}

/*
 * 功能描述  : ht vht协议初始化vap能力
 * 1.日    期  : 2013年11月18日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
uint32_t mac_vap_cap_init_htvht(mac_vap_stru *pst_mac_vap)
{
    pst_mac_vap->st_cap_flag.bit_rifs_tx_on = OAL_FALSE;

#ifdef _PRE_WLAN_FEATURE_TXOPPS
    if (pst_mac_vap->pst_mib_info == NULL) {
        oam_error_log3(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
                       "{mac_vap_cap_init_htvht::pst_mib_info null,vap mode[%d] state[%d] user num[%d].}",
                       pst_mac_vap->en_vap_mode, pst_mac_vap->en_vap_state, pst_mac_vap->us_user_nums);
        return OAL_FAIL;
    }

    pst_mac_vap->st_cap_flag.bit_txop_ps = OAL_FALSE;
    if ((pst_mac_vap->en_protocol == WLAN_VHT_MODE || pst_mac_vap->en_protocol == WLAN_VHT_ONLY_MODE ||
        (g_wlan_spec_cfg->feature_11ax_is_open && (pst_mac_vap->en_protocol == WLAN_HE_MODE))) &&
        mac_mib_get_txopps(pst_mac_vap)) {
        mac_mib_set_txopps(pst_mac_vap, OAL_TRUE);
    } else {
        mac_mib_set_txopps(pst_mac_vap, OAL_FALSE);
    }
#endif

    return OAL_SUCC;
}

/*
 * 功能描述  : 初始化rx nss
 * 1.日    期  : 2014年6月27日
 *   作    者  : zhangyu
 *   修改内容  : 新生成函数
 */
void mac_vap_init_rx_nss_by_protocol(mac_vap_stru *pst_mac_vap)
{
    wlan_protocol_enum_uint8 en_protocol;
    mac_device_stru *pst_mac_device;
    uint8_t nss_num_cap;

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (oal_unlikely(pst_mac_device == NULL)) {
        oam_error_log1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{mac_vap_init_rx_nss_by_protocol::\
            pst_mac_device[%d] null.}", pst_mac_vap->uc_device_id);
        return;
    }
    en_protocol = pst_mac_vap->en_protocol;
    nss_num_cap = MAC_DEVICE_GET_NSS_NUM(pst_mac_device);
    if (en_protocol <= WLAN_MIXED_TWO_11G_MODE) {
        pst_mac_vap->en_vap_rx_nss = WLAN_SINGLE_NSS;
    } else if (en_protocol < WLAN_PROTOCOL_BUTT) {
        pst_mac_vap->en_vap_rx_nss = nss_num_cap;
    } else {
        pst_mac_vap->en_vap_rx_nss = WLAN_NSS_LIMIT;
    }
#ifdef _PRE_WLAN_FEATURE_M2S
    pst_mac_vap->en_vap_rx_nss = oal_min(pst_mac_vap->en_vap_rx_nss,
        MAC_M2S_CALI_NSS_FROM_SMPS_MODE(MAC_DEVICE_GET_MODE_SMPS(pst_mac_device)));
#endif
}

/*
 * 功能描述  : 依据协议初始化vap相应能力
 * 1.日    期  : 2013年11月18日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
uint32_t mac_vap_init_by_protocol(mac_vap_stru *pst_mac_vap, wlan_protocol_enum_uint8 en_protocol)
{
    pst_mac_vap->en_protocol = en_protocol;

    if (en_protocol < WLAN_HT_MODE) {
        mac_vap_cap_init_legacy(pst_mac_vap);
    } else {
        if (OAL_SUCC != mac_vap_cap_init_htvht(pst_mac_vap)) {
            return OAL_FAIL;
        }
    }

    /* 根据协议模式更新mib值 */
    if (mac_vap_config_vht_ht_mib_by_protocol(pst_mac_vap) != OAL_SUCC) {
        return OAL_FAIL;
    }

    /* 根据协议更新初始化空间流个数 */
    mac_vap_init_rx_nss_by_protocol(pst_mac_vap);
#ifdef _PRE_WLAN_FEATURE_11AC2G
    if ((en_protocol == WLAN_HT_MODE) &&
        (pst_mac_vap->st_cap_flag.bit_11ac2g == OAL_TRUE) &&
        (pst_mac_vap->st_channel.en_band == WLAN_BAND_2G)) {
        mac_mib_set_VHTOptionImplemented(pst_mac_vap, OAL_TRUE);
    }
#endif

    return OAL_SUCC;
}


void mac_sta_init_bss_rates(mac_vap_stru *pst_vap, mac_bss_dscr_stru *bss_dscr)
{
    mac_device_stru               *pst_mac_dev;
    wlan_protocol_enum_uint8       en_vap_protocol;
    mac_data_rate_stru            *pst_rates = NULL;
    uint32_t                     i, j;

    pst_mac_dev = mac_res_get_dev(pst_vap->uc_device_id);
    if (pst_mac_dev == NULL) {
        oam_error_log1(pst_vap->uc_vap_id, OAM_SF_ANY, "{mac_vap_init_rates::pst_mac_dev[%d] null.}",
            pst_vap->uc_device_id);
        return;
    }

    /* 初始化速率集 */
    pst_rates = mac_device_get_all_rates(pst_mac_dev);
    if (bss_dscr != NULL) {
        for (i = 0; i < bss_dscr->uc_num_supp_rates; i++) {
            for (j = 0; j < MAC_DATARATES_PHY_80211G_NUM; j++) {
                if (IS_EQUAL_RATES(pst_rates[j].uc_mac_rate, bss_dscr->auc_supp_rates[i])) {
                    pst_rates[j].uc_mac_rate = bss_dscr->auc_supp_rates[i];
                    break;
                }
            }
        }
    }

    en_vap_protocol = pst_vap->en_protocol;

    mac_vap_init_rates_by_protocol(pst_vap, en_vap_protocol, pst_rates);
}

/*
 * 功能描述  : 根据内核下发的关联能力，赋值加密相关的mib 值
 * 1.日    期  : 2014年1月26日
 *   作    者  : duankaiyong
 *   修改内容  : 新生成函数
 */
uint32_t mac_vap_init_privacy(mac_vap_stru *pst_mac_vap, mac_conn_security_stru *pst_conn_sec)
{
    mac_crypto_settings_stru           *pst_crypto = NULL;
    uint32_t                          ret;

    mac_mib_set_privacyinvoked(pst_mac_vap, OAL_FALSE);
    /* 初始化 RSNActive 为FALSE */
    mac_mib_set_rsnaactivated(pst_mac_vap, OAL_FALSE);
    /* 清除加密套件信息 */
    mac_mib_init_rsnacfg_suites(pst_mac_vap);

    pst_mac_vap->st_cap_flag.bit_wpa = OAL_FALSE;
    pst_mac_vap->st_cap_flag.bit_wpa2 = OAL_FALSE;

#ifdef _PRE_WLAN_FEATURE_PMF
    ret = mac_vap_init_pmf(pst_mac_vap, pst_conn_sec->en_pmf_cap, pst_conn_sec->en_mgmt_proteced);
    if (ret != OAL_SUCC) {
        oam_error_log1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                       "{mac_11i_init_privacy::mac_11w_init_privacy failed[%d].}", ret);
        return ret;
    }
#endif
    /* 不加密 */
    if (pst_conn_sec->en_privacy == OAL_FALSE) {
        return OAL_SUCC;
    }

    /* WEP加密 */
    if (pst_conn_sec->uc_wep_key_len != 0) {
        return mac_vap_add_wep_key(pst_mac_vap, pst_conn_sec->uc_wep_key_index,
                                   pst_conn_sec->uc_wep_key_len, pst_conn_sec->auc_wep_key);
    }

    /* WPA/WPA2加密 */
    pst_crypto = &(pst_conn_sec->st_crypto);

    /* 初始化RSNA mib 为 TRUR */
    mac_mib_set_privacyinvoked(pst_mac_vap, OAL_TRUE);
    mac_mib_set_rsnaactivated(pst_mac_vap, OAL_TRUE);

    /* 初始化单播密钥套件 */
    if (pst_crypto->wpa_versions == WITP_WPA_VERSION_1) {
        pst_mac_vap->st_cap_flag.bit_wpa = OAL_TRUE;
        mac_mib_set_wpa_pair_suites(pst_mac_vap, pst_crypto->aul_pair_suite);
        mac_mib_set_wpa_akm_suites(pst_mac_vap, pst_crypto->aul_akm_suite);
        mac_mib_set_wpa_group_suite(pst_mac_vap, pst_crypto->group_suite);
    } else if (pst_crypto->wpa_versions == WITP_WPA_VERSION_2) {
        pst_mac_vap->st_cap_flag.bit_wpa2 = OAL_TRUE;
        mac_mib_set_rsn_pair_suites(pst_mac_vap, pst_crypto->aul_pair_suite);
        mac_mib_set_rsn_akm_suites(pst_mac_vap, pst_crypto->aul_akm_suite);
        mac_mib_set_rsn_group_suite(pst_mac_vap, pst_crypto->group_suite);
        mac_mib_set_rsn_group_mgmt_suite(pst_mac_vap, pst_crypto->group_mgmt_suite);
    }

    return OAL_SUCC;
}
