
#ifdef _PRE_WLAN_FEATURE_GNSS_RSMC
#include "hmac_gnss_rsmc.h"
#include "hmac_config.h"
#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_GNSS_RSMC_C
/*
 * 功能描述  : 将2G vap下的用户删除
 */
OAL_STATIC void hmac_gnss_rsmc_kick_user(mac_vap_stru *mac_vap)
{
    mac_cfg_kick_user_param_stru kick_user_param = {
        { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
        MAC_UNSPEC_REASON };

    if ((mac_vap->st_channel.en_band != WLAN_BAND_2G) && (mac_vap->en_p2p_mode == WLAN_LEGACY_VAP_MODE)) {
        return;
    }
    hmac_config_kick_user(mac_vap, sizeof(mac_cfg_kick_user_param_stru), (uint8_t *)&kick_user_param);
}
/*
 * 功能描述  : 通知device需要迁移的vap以及迁移到主路还是辅路
 */
OAL_STATIC void hmac_gnss_rsmc_shift_vap(mac_vap_stru *mac_vap, oal_bool_enum_uint8 is_mst2slv)
{
    uint32_t ret;
    /* p2p会被断开连接所以不需要迁移 */
    if (mac_vap->en_p2p_mode != WLAN_LEGACY_VAP_MODE) {
        return;
    }
    ret = hmac_config_send_event(mac_vap, WLAN_CFGID_GNSS_RSMC_SHIFT_VAP,
        sizeof(oal_bool_enum_uint8), &is_mst2slv);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_gnss_rsmc_shift_vap::send event failed[%d].}", ret);
    }
}
/*
 * 功能描述  : 通知device迁移结束
 */
OAL_STATIC void hmac_gnss_rsmc_shift_vap_finish(mac_vap_stru *mac_vap, oal_bool_enum_uint8 gnss_rsmc_status)
{
    uint32_t ret;
    if (mac_vap == NULL) {
        oam_error_log0(0, 0, "hmac_gnss_rsmc_shift_vap_finish:mac_vap is null");
        return;
    }
    ret = hmac_config_send_event(mac_vap, WLAN_CFGID_GNSS_RSMC_SHIFT_VAP_FINISH,
        sizeof(gnss_rsmc_status), &gnss_rsmc_status);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_gnss_rsmc_shift_vap_finish:send event failed[%d]", ret);
    }
}
/*
 * 功能描述  : 处理device侧上报的gnss rsmc事件
 */
uint32_t hmac_process_gnss_rsmc_status_sync(mac_vap_stru *mac_vap, uint8_t len, uint8_t *params)
{
    oal_bool_enum_uint8 gnss_rsmc_status = (oal_bool_enum_uint8)params[0];
    mac_device_stru *mac_device = mac_res_get_mac_dev();
    mac_vap_stru *mac_vap_temp = NULL;
    uint8_t vap_index;
    oam_warning_log1(0, 0, "hmac_process_gnss_rsmc_status_sync:gnss_rsmc_status=%d", gnss_rsmc_status);
    for (vap_index = 0; vap_index < mac_device->uc_vap_num; vap_index++) {
        mac_vap_temp = (mac_vap_stru *)mac_res_get_mac_vap(mac_device->auc_vap_id[vap_index]);
        if (mac_vap_temp == NULL) {
            continue;
        }
        if (mac_vap_temp->en_p2p_mode == WLAN_P2P_DEV_MODE) {
            continue;
        }
        oam_warning_log3(0, 0, "hmac_process_gnss_rsmc_status_sync:vap_id=%d, vap_mode=%d, p2p_mode=%d",
            mac_vap_temp->uc_vap_id, mac_vap_temp->en_vap_mode, mac_vap_temp->en_p2p_mode);
        if (gnss_rsmc_status == OAL_TRUE) {
            /* 删除2G VAP的所有user */
            hmac_gnss_rsmc_kick_user(mac_vap_temp);
            /* 迁移VAP */
            hmac_gnss_rsmc_shift_vap(mac_vap_temp, OAL_TRUE);
        } else {
            hmac_gnss_rsmc_shift_vap(mac_vap_temp, OAL_FALSE);
        }
    }
    hmac_gnss_rsmc_shift_vap_finish(mac_vap_temp, gnss_rsmc_status);
    return OAL_SUCC;
}
/*
 * 功能描述  : 调试命令接口
 */
uint32_t hmac_process_gnss_rsmc_status_cmd(mac_vap_stru *mac_vap, uint32_t *params)
{
    oal_bool_enum_uint8 gnss_rsmc_status = (oal_bool_enum_uint8)params[0];
    return hmac_process_gnss_rsmc_status_sync(mac_vap, sizeof(oal_bool_enum_uint8), &gnss_rsmc_status);
}
#endif
