

/* 1 Í·ÎÄ¼þ°üº¬ */
#include "hmac_chba_coex.h"
#include "hmac_chba_frame.h"
#include "hmac_chba_sync.h"
#include "hmac_chba_chan_switch.h"
#include "hmac_mgmt_sta.h"
#include "hmac_resource.h"
#include "hmac_roam_alg.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_COEX_C
#ifdef _PRE_WLAN_CHBA_MGMT
oal_bool_enum_uint8 mac_chba_support_dbac(void)
{
    /* chba定制化开关未开启 */
    if (!(g_optimized_feature_switch_bitmap & BIT(CUSTOM_OPTIMIZE_CHBA))) {
        return OAL_FALSE;
    }

    return (g_uc_dbac_dynamic_switch & BIT1) ? OAL_TRUE : OAL_FALSE;
}

oal_bool_enum_uint8 hmac_chba_coex_other_is_dbac(mac_device_stru *mac_device,
    mac_channel_stru *chan_info)
{
    hmac_vap_stru *hmac_chba_vap = NULL;
    hmac_chba_vap_stru *chba_vap_info = NULL;

    if (mac_device->uc_vap_num <= HMAC_CHBA_INVALID_COEX_VAP_CNT) {
        return OAL_FALSE;
    }
    hmac_chba_vap = hmac_chba_find_chba_vap(mac_device);
    if (hmac_chba_vap == NULL) {
        return OAL_FALSE;
    }
    chba_vap_info = hmac_chba_vap->hmac_chba_vap_info;
    if (chba_vap_info == NULL) {
        return OAL_FALSE;
    }
    /* 支持CHBA DBAC 无需检查 */
    if (mac_chba_support_dbac() == OAL_TRUE) {
        return OAL_FALSE;
    }
    if ((chan_info->en_band == chba_vap_info->work_channel.en_band) &&
        (chan_info->uc_chan_number != chba_vap_info->work_channel.uc_chan_number)) {
        return OAL_TRUE;
    }
    return OAL_FALSE;
}
/* STA/GO CSA、STA漫游、CHBA切信道时满足DBAC时通知上层，此函数调用处已判断CHBA共存条件并且满足DBAC */
void hmac_chba_coex_switch_chan_dbac_rpt(mac_channel_stru *mac_channel, uint8_t rpt_type)
{
    mac_chba_adjust_chan_info chan_notify = { 0 };

    /* 通知FWK因需要切信道满足DBAC，将切换信道信息上报 */
    chan_notify.report_type = HMAC_CHBA_CHAN_SWITCH_REPORT;
    chan_notify.chan_number = mac_channel->uc_chan_number;
    chan_notify.bandwidth = mac_channel->en_bandwidth;
    chan_notify.switch_type = rpt_type;
    chan_notify.status_code = 0;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_cfg80211_chba_report((uint8_t *)(&chan_notify), sizeof(chan_notify));
#endif
    oam_warning_log3(0, OAM_SF_CHBA, "hmac_chba_coex_switch_chan_dbac_rpt::already rpt, chan:%d, bw:%d, rpt_type:%d",
        mac_channel->uc_chan_number, mac_channel->en_bandwidth, rpt_type);
    return;
}

oal_bool_enum_uint8 hmac_chba_coex_go_csa_chan_is_dbac(mac_device_stru *mac_device,
    uint8_t new_channel, wlan_channel_bandwidth_enum_uint8 go_new_bw)
{
    mac_channel_stru mac_channel = { 0 };

    mac_channel.uc_chan_number = new_channel;
    mac_channel.en_bandwidth = go_new_bw;
    mac_channel.en_band = mac_get_band_by_channel_num(mac_channel.uc_chan_number);
    mac_get_channel_idx_from_num(mac_channel.en_band, mac_channel.uc_chan_number,
        OAL_FALSE, &(mac_channel.uc_chan_idx));
    if (hmac_chba_coex_other_is_dbac(mac_device, &mac_channel) == OAL_TRUE) {
        oam_warning_log0(0, OAM_SF_ROAM, "{hmac_p2p_change_go_channel::go csa chan not supp chba coex!}");
        return OAL_TRUE;
    }
    return OAL_FALSE;
}

oal_bool_enum_uint8 hmac_chba_coex_start_is_dbac(mac_vap_stru *start_vap, mac_channel_stru *new_channel)
{
    uint32_t idx;
    uint32_t ret;
    uint32_t up_vap_num;
    mac_device_stru *mac_device = NULL;
    mac_vap_stru *up_vap_array[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE] = {NULL};

    /* 1、非CHBA不检查 */
    if (start_vap->chba_mode != CHBA_MODE) {
        return OAL_FALSE;
    }
    /* 2、支持CHBA DBAC 无需检查 */
    if (mac_chba_support_dbac() == OAL_TRUE) {
        return OAL_FALSE;
    }
    mac_device = mac_res_get_dev(start_vap->uc_device_id);
    if (mac_device == NULL) {
        return OAL_FALSE;
    }
    /* 3、仅有chba vap存在不检查 */
    ret = mac_device_find_up_vaps(mac_device, up_vap_array, WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE);
    if (ret != OAL_SUCC) {
        return OAL_FALSE;
    }
    /* 4、检查是否有其他UP VAP和 chba不在同一band */
    up_vap_num = mac_device_calc_up_vap_num(mac_device);
    for (idx = 0; idx < up_vap_num; idx++) {
        if ((up_vap_array[idx] != NULL && up_vap_array[idx]->uc_vap_id != start_vap->uc_vap_id) &&
            (up_vap_array[idx]->st_channel.en_band == new_channel->en_band) &&
            (up_vap_array[idx]->st_channel.uc_chan_number != new_channel->uc_chan_number)) {
            oam_warning_log0(start_vap->uc_vap_id, OAM_SF_CHAN, "{hmac_chba_start_check_dbac::dbac check failed.}");
            return OAL_TRUE;
        }
    }
    return OAL_FALSE;
}
oal_bool_enum_uint8 hmac_chba_coex_channel_is_dbac(mac_vap_stru *start_vap,
    mac_channel_stru *new_channel)
{
    mac_device_stru *mac_device = NULL;

    /* 支持CHBA DBAC 无需检查 */
    if (mac_chba_support_dbac() == OAL_TRUE) {
        return OAL_FALSE;
    }
    if (IS_LEGACY_AP(start_vap)) {
        return hmac_chba_coex_start_is_dbac(start_vap, new_channel); /* CHBA VAP检查是否有不满足的up vap */
    } else {
        mac_device = mac_res_get_dev(start_vap->uc_device_id);
        if (mac_device != NULL) {
            return hmac_chba_coex_other_is_dbac(mac_device, new_channel); /* 非CHBA VAP检查是否有不满足条件的chba */
        }
    }
    return OAL_FALSE;
}

oal_bool_enum_uint8 hmac_chba_coex_chan_is_in_list(uint8_t *coex_chan_lists,
    uint8_t chan_idx, uint8_t chan_number)
{
    uint8_t idx;
    for (idx = 0; idx < chan_idx; idx++) {
        if (chan_number == coex_chan_lists[idx]) {
            return OAL_TRUE;
        }
    }
    return OAL_FALSE;
}
int32_t wal_ioctl_chba_set_coex_cmd_para_check(mac_chba_set_coex_chan_info *cfg_coex_chan_info)
{
    if (cfg_coex_chan_info->cfg_cmd_type >= HMAC_CHBA_COEX_CFG_TYPE_BUTT) {
        oam_warning_log0(0, OAM_SF_ANY, "{wal_ioctl_chba_set_coex:cfg_cmd_type invalid.}");
        return -OAL_EFAIL;
    }
    if (cfg_coex_chan_info->supported_channel_cnt > WLAN_5G_CHANNEL_NUM) {
        oam_warning_log0(0, OAM_SF_ANY, "{wal_ioctl_chba_set_coex:supported_channel_cnt invalid.}");
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}
int32_t wal_ioctl_chba_set_coex_start_check(oal_net_device_stru *net_dev)
{
    mac_vap_stru *mac_vap = oal_net_dev_priv(net_dev);
    hmac_vap_stru *hmac_vap = NULL;

    if (mac_vap == NULL) {
        return -OAL_EFAIL;
    }
    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (hmac_vap == NULL) {
        oam_error_log0(mac_vap->uc_vap_id, 0, "{wal_ioctl_chba_set_coex::pst_hmac_vap null.}");
        return -OAL_EFAIL;
    }
    if (hmac_chba_vap_start_check(hmac_vap) == OAL_FALSE) {
        return -OAL_EFAIL;
    }
    return OAL_SUCC;
}

uint32_t hmac_chba_coex_info_changed_report(mac_vap_stru *mac_vap, uint8_t len, uint8_t *param)
{
    hmac_chba_report_coex_chan_info rpt_coex_info;
    mac_chba_rpt_coex_info *d2h_coex_info = (mac_chba_rpt_coex_info *)param;
    uint8_t d2h_chan_idx = 0;
    uint8_t rpt_chan_idx = 0;
    uint8_t chan_number;

    memset_s(&rpt_coex_info, sizeof(rpt_coex_info), 0, sizeof(rpt_coex_info));
    rpt_coex_info.report_type = HMAC_CHBA_COEX_CHAN_INFO_REPORT;
    memcpy_s(rpt_coex_info.dev_mac_addr, WLAN_MAC_ADDR_LEN, d2h_coex_info->dev_mac_addr, WLAN_MAC_ADDR_LEN);

    while (d2h_chan_idx < d2h_coex_info->coex_chan_cnt) {
        chan_number = d2h_coex_info->coex_chan_lists[d2h_chan_idx];
        if (hmac_chba_is_valid_channel(chan_number) != OAL_SUCC ||
            hmac_chba_coex_chan_is_in_list(rpt_coex_info.coex_chan_lists, rpt_chan_idx, chan_number) == OAL_TRUE) {
            oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_coex_info_changed_report:invalid number %d or already in list.",
                chan_number);
            d2h_chan_idx++;
            continue;
        }
        rpt_coex_info.coex_chan_lists[rpt_chan_idx++] = chan_number;
        d2h_chan_idx++;
    }
    rpt_coex_info.coex_chan_cnt = rpt_chan_idx;
    if (rpt_coex_info.coex_chan_cnt != 0) {
        oam_warning_log3(0, 0, "hmac_chba_coex_info_changed_report::peer coex cnt[%d], first chan[%d], last[%d]",
            rpt_coex_info.coex_chan_cnt, rpt_coex_info.coex_chan_lists[0],
            rpt_coex_info.coex_chan_lists[rpt_coex_info.coex_chan_cnt - 1]);
    } else {
        oam_warning_log0(0, OAM_SF_ANY, "hmac_chba_coex_info_changed_report::peer coex cnt 0, no coex list!");
    }

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_cfg80211_chba_report((uint8_t *)(&rpt_coex_info), sizeof(rpt_coex_info));
#endif
    return OAL_SUCC;
}
uint32_t hmac_chba_coex_d2h_sta_csa_dbac_rpt(mac_vap_stru *mac_vap, uint8_t len, uint8_t *param)
{
    mac_channel_stru *mac_channel = (mac_channel_stru *)param;

    hmac_chba_coex_switch_chan_dbac_rpt(mac_channel, HMAC_CHBA_COEX_CHAN_SWITCH_STA_CSA_RPT);
    return OAL_SUCC;
}


void hmac_chba_coex_island_info_report(mac_chba_self_conn_info_stru *self_conn_info)
{
    hmac_chba_report_island_info rpt_island_info;
    uint8_t dev_idx;

    rpt_island_info.report_type = HMAC_CHBA_COEX_ISLAND_INFO_REPORT;
    rpt_island_info.island_dev_cnt = self_conn_info->island_device_cnt;

    oam_warning_log1(0, OAM_SF_ANY,
        "hmac_chba_coex_island_info_report::island dev cnt[%d]", rpt_island_info.island_dev_cnt);
    for (dev_idx = 0; dev_idx < self_conn_info->island_device_cnt; dev_idx++) {
        memcpy_s(rpt_island_info.island_dev_lists[dev_idx], WLAN_MAC_ADDR_LEN,
            self_conn_info->island_device_list[dev_idx].mac_addr, WLAN_MAC_ADDR_LEN);
    }

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_cfg80211_chba_report((uint8_t *)(&rpt_island_info), sizeof(rpt_island_info));
#endif
    return;
}

uint32_t hmac_config_chba_set_coex_chan(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params)
{
    hmac_device_stru *hmac_device = NULL;
    mac_chba_set_coex_chan_info *cfg_coex_chan_info = NULL;
    hmac_vap_stru *hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    hmac_chba_vap_stru *chba_vap_info = NULL;
    uint32_t ret;

    if (hmac_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    chba_vap_info = hmac_vap->hmac_chba_vap_info;
    hmac_device = hmac_res_get_mac_dev(mac_vap->uc_device_id);
    if (oal_unlikely(hmac_device == NULL || chba_vap_info == NULL)) {
        oam_error_log0(mac_vap->uc_device_id, OAM_SF_ANY, "{hmac_config_chba_set_coex_chan::para null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    cfg_coex_chan_info = (mac_chba_set_coex_chan_info *)params;
    if (cfg_coex_chan_info->supported_channel_cnt != 0) {
        oam_warning_log4(0, OAM_SF_ANY, "hmac_config_chba_set_coex_chan::cfg_cmd_type[%d][0:self dev 1:island], \
            coex_chan_cnt[%d], first chan[%d], last chan[%d]", cfg_coex_chan_info->cfg_cmd_type,
            cfg_coex_chan_info->supported_channel_cnt, cfg_coex_chan_info->supported_channel_list[0],
            cfg_coex_chan_info->supported_channel_list[cfg_coex_chan_info->supported_channel_cnt - 1]);
    } else {
        oam_warning_log1(0, OAM_SF_ANY, "hmac_config_chba_set_coex_chan::cfg_cmd_type[%d][0:self dev 1:island], \
            coex_chan_cnt is 0", cfg_coex_chan_info->cfg_cmd_type);
    }
    // 将本设备共存信道信息保存在chba_vap下，岛内信息保存在mac_device下
    if (cfg_coex_chan_info->cfg_cmd_type == HMAC_CHBA_SELF_DEV_COEX_CFG_TYPE) {
        chba_vap_info->self_coex_chan_cnt = cfg_coex_chan_info->supported_channel_cnt;
        memset_s(chba_vap_info->self_coex_channels_list, WLAN_5G_CHANNEL_NUM, 0, WLAN_5G_CHANNEL_NUM);
        if (memcpy_s(chba_vap_info->self_coex_channels_list, WLAN_5G_CHANNEL_NUM,
            cfg_coex_chan_info->supported_channel_list, cfg_coex_chan_info->supported_channel_cnt) != EOK) {
            return OAL_FAIL;
        }
        hmac_chba_save_update_beacon_pnf(hmac_chba_sync_get_domain_bssid());
        return OAL_SUCC;
    } else if (cfg_coex_chan_info->cfg_cmd_type == HMAC_CHBA_ISLAND_COEX_CFG_TYPE) {
        hmac_device->island_coex_info.island_coex_chan_cnt = cfg_coex_chan_info->supported_channel_cnt;
        if (memcpy_s(hmac_device->island_coex_info.island_coex_channels_list, WLAN_5G_CHANNEL_NUM,
            cfg_coex_chan_info->supported_channel_list, cfg_coex_chan_info->supported_channel_cnt) != EOK) {
            return OAL_FAIL;
        }
    }
    // 抛事件到dmac
    ret = hmac_config_send_event(mac_vap, WLAN_CFGID_CHBA_SET_COEX_CHAN_INFO, sizeof(mac_chba_set_coex_chan_info),
        (uint8_t *)cfg_coex_chan_info);
    if (ret != OAL_SUCC) {
        oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_chba_set_coex_chan:set coex chan err[%u]}", ret);
        return ret;
    }
    return OAL_SUCC;
}

#endif
