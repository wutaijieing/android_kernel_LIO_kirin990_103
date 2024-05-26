/*
 * 版权所有 (c) 华为技术有限公司 2013-2018
 * 功能说明 : 配置相关实现hmac接口实现源文件
 * 作    者 : zourong
 * 创建日期 : 2013年1月8日
 */

/* 1 头文件包含 */
#include "hmac_config.h"
#include "hmac_fsm.h"
#include "hmac_mgmt_join.h"
#ifdef _PRE_WLAN_CHBA_MGMT
#include "hmac_chba_function.h"
#include "hmac_chba_ps.h"
#include "hmac_chba_chan_switch.h"
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CONFIG_CHBA_C

#ifdef _PRE_WLAN_CHBA_MGMT
static hmac_join_req_stru g_join_param = {
    .st_bss_dscr = {
        .auc_bssid = {0x10, 0xe9, 0x53, 0x44, 0x55, 0x66},
        .us_beacon_period = 100,
        .st_channel = {
            .en_band= WLAN_BAND_5G,
            .ext6g_band = OAL_FALSE,
            .uc_chan_number = 36,
            .en_bandwidth = WLAN_BAND_WIDTH_20M,
            .uc_chan_idx = 0,
        },

        .ac_country = {'C', 'N'},
        .en_channel_bandwidth = WLAN_BAND_WIDTH_20M, // hmac_sta_update_join_bw
        .en_ht_capable = 1,
        .auc_supp_rates = {0x8c, 0x12, 0x98, 0x24, 0xb0, 0x48, 0x60, 0x6c},
        .uc_num_supp_rates = 8,
    }
};

uint8_t g_dummy_user_mac[] = {0x10, 0xe9, 0x53, 0x66, 0x55, 0x44};
uint32_t hmac_sta_update_join_req_params_post_event(hmac_vap_stru *pst_hmac_vap,
    hmac_join_req_stru *pst_join_req, mac_ap_type_enum_uint16 en_ap_type);
OAL_STATIC OAL_INLINE void hmac_sta_set_mode_params(mac_cfg_mode_param_stru *cfg_mode,
    wlan_protocol_enum_uint8 protocol, wlan_channel_band_enum_uint8 band, uint8_t channel_idx,
    wlan_channel_bandwidth_enum_uint8 bandwidth)
{
    cfg_mode->en_protocol = protocol;
    cfg_mode->en_band = band;
    cfg_mode->en_bandwidth = bandwidth;
    cfg_mode->en_channel_idx = channel_idx;
}

void hmac_demo_main(hmac_vap_stru *hmac_vap)
{
    mac_cfg_mode_param_stru cfg_mode = {WLAN_VHT_MODE, WLAN_BAND_5G, WLAN_BAND_WIDTH_20M};
    mac_cfg_add_user_param_stru add_user_param;
    /* 1.vap初始化 */
    mac_vap_set_bssid(&hmac_vap->st_vap_base_info, g_join_param.st_bss_dscr.auc_bssid, WLAN_MAC_ADDR_LEN);
    mac_vap_set_current_channel(&hmac_vap->st_vap_base_info, WLAN_BAND_5G, 36, OAL_FALSE);
    hmac_vap->st_vap_base_info.st_channel.en_bandwidth = WLAN_BAND_WIDTH_20M;
    hmac_vap->st_vap_base_info.en_protocol = WLAN_VHT_MODE;
    hmac_sta_set_mode_params(&cfg_mode, hmac_vap->st_vap_base_info.en_protocol, \
        hmac_vap->st_vap_base_info.st_channel.en_band, \
        g_join_param.st_bss_dscr.st_channel.uc_chan_number, hmac_vap->st_vap_base_info.st_channel.en_bandwidth);
    // 协议模式11n，速率 && start vap。
    hmac_config_sta_update_rates(&hmac_vap->st_vap_base_info, &cfg_mode, &g_join_param.st_bss_dscr);
    // join event h2d
    hmac_sta_update_join_req_params_post_event(hmac_vap, &g_join_param, 0);
    hmac_fsm_change_state(hmac_vap, MAC_VAP_STATE_STA_JOIN_COMP);
    // open
    mac_mib_set_AuthenticationMode(&hmac_vap->st_vap_base_info, WLAN_WITP_AUTH_OPEN_SYSTEM);
    /* 2.创建user */
    oal_set_mac_addr(add_user_param.auc_mac_addr, g_dummy_user_mac);
    add_user_param.en_ht_cap = OAL_TRUE;
    hmac_config_add_user(&hmac_vap->st_vap_base_info, sizeof(add_user_param), (uint8_t*)&add_user_param);
}

uint32_t hmac_config_start_demo(mac_vap_stru *mac_vap, uint16_t us_len, uint8_t *puc_param)
{
    hmac_vap_stru *hmac_vap = NULL;

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (hmac_vap == NULL) {
        oam_error_log0(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_demo::hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_demo_main(hmac_vap);
    return OAL_SUCC;
}

/*
 * 功能描述  : CHBA维测日志开关接口
 * 日    期  : 2021年09月06日
 * 作    者  : ywx5321377
 */
uint32_t hmac_config_chba_log_level(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params)
{
    uint32_t ret;
    hmac_vap_stru *hmac_vap = NULL;
    hmac_chba_vap_stru *chba_vap_info = NULL;

    if ((mac_vap == NULL) || (params == NULL)) {
        oam_error_log0(0, OAM_SF_CFG, "{hmac_config_chba_log_level:ptr is null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 非CHBA vap不做处理 */
    hmac_vap = mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (hmac_chba_vap_start_check(hmac_vap) == OAL_FALSE) {
        oam_warning_log0(0, OAM_SF_CHBA, "{hmac_config_chba_log_level:hmac_vap is not chba vap}");
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    chba_vap_info = hmac_get_up_chba_vap_info();
    if (chba_vap_info == NULL) {
        oam_error_log0(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_chba_log_level:chba_vap_info is null}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    chba_vap_info->chba_log_level = *(uint8_t *)params;
    oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_CFG,
        "{hmac_config_chba_log_level: chba_log_level is %d}", chba_vap_info->chba_log_level);
    /***************************************************************************
       抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ret = hmac_config_send_event(mac_vap, WLAN_CFGID_CHBA_LOG_LEVEL, len, params);
    if (ret != OAL_SUCC) {
        oam_warning_log0(mac_vap->uc_vap_id, OAM_SF_CHBA, "{hmac_config_chba_log_level:fail to send event to dmac}");
        return ret;
    }
    return OAL_SUCC;
}
/*
 * 功能描述  : 配置CHBA低功耗吞吐门限
 * 日    期  : 2021年12月07日
 * 作    者  : ywx5321377
 */
uint32_t hmac_config_chba_set_ps_thres(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params)
{
    hmac_vap_stru *hmac_vap = NULL;
    hmac_chba_vap_stru *chba_vap_info = NULL;
    chba_ps_thres_config_stru *ps_thres_cfg = NULL;
    hmac_chba_ps_throught_thres_stru *chba_thres = NULL;

    if ((mac_vap == NULL) || (params == NULL)) {
        oam_error_log0(0, OAM_SF_CFG, "{hmac_config_chba_set_ps_thres:ptr is null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 非CHBA vap不做处理 */
    hmac_vap = mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (hmac_chba_vap_start_check(hmac_vap) == OAL_FALSE) {
        oam_warning_log0(0, OAM_SF_CHBA, "{hmac_config_chba_set_ps_thres:hmac_vap is not chba vap}");
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    chba_vap_info = hmac_get_up_chba_vap_info();
    if (chba_vap_info == NULL) {
        oam_error_log0(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_chba_log_level:chba_vap_info is null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ps_thres_cfg = (chba_ps_thres_config_stru *)params;
    oam_warning_log2(mac_vap->uc_vap_id, OAM_SF_CFG,
        "{hmac_config_chba_set_ps_thres: thres_type[%d], new_ps_throught_thres %d(100ms)}",
        ps_thres_cfg->thres_type, ps_thres_cfg->thres_val);
    chba_thres = hmac_chba_get_ps_throught_thres(hmac_vap);
    switch (ps_thres_cfg->thres_type) {
        case CHBA_PS_THRES_LEVEL_THREE_TO_TWO:
            chba_thres->upgrade_to_lv_two_thres = ps_thres_cfg->thres_val;
            break;
        case CHBA_PS_THRES_LEVEL_TWO_TO_ONE:
            chba_thres->upgrade_to_lv_one_thres = ps_thres_cfg->thres_val;
            break;
        case CHBA_PS_THRES_LEVEL_ONE_TO_TWO:
            chba_thres->downgrade_to_lv_two_thres = ps_thres_cfg->thres_val;
            break;
        case CHBA_PS_THRES_LEVEL_TWO_TO_THREE:
            chba_thres->downgrade_to_lv_three_thres = ps_thres_cfg->thres_val;
            break;
        default:
            oam_error_log1(0, OAM_SF_CHBA,
                "{hmac_config_chba_set_ps_thres:invalid thres_type[%d]}", ps_thres_cfg->thres_type);
            break;
    }
    oam_warning_log4(mac_vap->uc_vap_id, OAM_SF_CFG,
        "{hmac_config_chba_set_ps_thres:three_to_two[%d], two_to_one[%d], one_to_two[%d], two_to_three[%d]}",
        chba_thres->upgrade_to_lv_two_thres, chba_thres->upgrade_to_lv_one_thres,
        chba_thres->downgrade_to_lv_two_thres, chba_thres->downgrade_to_lv_three_thres);
    return OAL_SUCC;
}

/*
 * 功能描述  : CHBA整岛切信道命令接口
 * 日    期  : 2021年09月14日
 * 作    者  : ywx5321377
 */
uint32_t hmac_config_chba_island_chan_switch(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params)
{
    hmac_vap_stru *hmac_vap = NULL;
    mac_chba_adjust_chan_info *chan_info = NULL;

    if (oal_any_null_ptr2(mac_vap, params)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 非chba vap不处理 */
    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (hmac_chba_vap_start_check(hmac_vap) == OAL_FALSE) {
        return OAL_SUCC;
    }

    chan_info = (mac_chba_adjust_chan_info *)params;
    oam_warning_log3(0, OAM_SF_ANY, "CHBA: hmac_config_chba_island_chan_switch: chan[%d] bw[%d], switch_type[%d].",
        chan_info->chan_number, chan_info->bandwidth, chan_info->switch_type);

    /* 启动全岛切信道流程 */
    hmac_chba_adjust_channel_proc(hmac_vap, chan_info);
    return OAL_SUCC;
}
#endif