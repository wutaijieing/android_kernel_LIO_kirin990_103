

/* 1 头文件包含 */
#include "hmac_chba_coex.h"
#include "hmac_chba_frame.h"
#include "hmac_chba_sync.h"
#include "hmac_chba_chan_switch.h"
#include "hmac_mgmt_sta.h"
#include "hmac_resource.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_COEX_C
#ifdef _PRE_WLAN_CHBA_MGMT
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
    if (cfg_coex_chan_info->supported_channel_cnt > MAX_CHANNEL_NUM_FREQ_5G) {
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
        OAM_ERROR_LOG0(mac_vap->uc_vap_id, 0, "{wal_ioctl_chba_set_coex::pst_hmac_vap null.}");
        return -OAL_EFAIL;
    }
    if (hmac_chba_vap_start_check(hmac_vap) == OAL_FALSE) {
        return -OAL_EFAIL;
    }
    return OAL_SUCC;
}
uint32_t hmac_chba_coex_info_changed_report(mac_vap_stru *mac_vap, oal_uint8 len, oal_uint8 *param)
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
            OAM_WARNING_LOG1(0, OAM_SF_CHBA, "hmac_chba_coex_info_changed_report:invalid number %d or already in list.",
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

void hmac_chba_coex_island_info_report(mac_chba_self_conn_info_stru *self_conn_info)
{
    hmac_chba_report_island_info rpt_island_info;
    uint8_t dev_idx;

    rpt_island_info.report_type = HMAC_CHBA_COEX_ISLAND_INFO_REPORT;
    rpt_island_info.island_dev_cnt = self_conn_info->island_device_cnt;

    OAM_WARNING_LOG1(0, OAM_SF_ANY,
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
    uint32_t ret;
    hmac_device_stru *hmac_device = NULL;
    mac_chba_set_coex_chan_info *cfg_coex_chan_info = NULL;
    hmac_chba_vap_stru *chba_vap_info = NULL;
    hmac_vap_stru *hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);

    if (hmac_vap == NULL) {
        return OAL_FAIL;
    }

    chba_vap_info = hmac_vap->hmac_chba_vap_info;
    hmac_device = hmac_res_get_mac_dev(mac_vap->uc_device_id);
    if (oal_unlikely(hmac_device == OAL_PTR_NULL)) {
        OAM_ERROR_LOG0(mac_vap->uc_device_id, OAM_SF_ANY, "{hmac_config_chba_set_coex_chan::hmac_device null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    cfg_coex_chan_info = (mac_chba_set_coex_chan_info *)params;
    if (cfg_coex_chan_info->supported_channel_cnt != 0) {
        oam_warning_log4(0, OAM_SF_ANY, "hmac_config_chba_set_coex_chan::cfg_cmd_type[%d][0:self dev 1:island], \
            coex_chan_cnt[%d], first chan[%d], last chan[%d]", cfg_coex_chan_info->cfg_cmd_type,
            cfg_coex_chan_info->supported_channel_cnt, cfg_coex_chan_info->supported_channel_list[0],
            cfg_coex_chan_info->supported_channel_list[cfg_coex_chan_info->supported_channel_cnt - 1]);
    } else {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_chba_set_coex_chan::cfg_cmd_type[%d][0:self dev 1:island], \
            coex_chan_cnt is 0", cfg_coex_chan_info->cfg_cmd_type);
    }
    // 将本设备共存信道信息保存在chba_vap下，岛内信息保存在mac_device下
    if (cfg_coex_chan_info->cfg_cmd_type == HMAC_CHBA_SELF_DEV_COEX_CFG_TYPE) {
        chba_vap_info->self_coex_chan_cnt = cfg_coex_chan_info->supported_channel_cnt;
        memset_s(chba_vap_info->self_coex_channels_list, MAX_CHANNEL_NUM_FREQ_5G, 0, MAX_CHANNEL_NUM_FREQ_5G);
        if (memcpy_s(chba_vap_info->self_coex_channels_list, MAX_CHANNEL_NUM_FREQ_5G,
            cfg_coex_chan_info->supported_channel_list, cfg_coex_chan_info->supported_channel_cnt) != EOK) {
            return OAL_FAIL;
        }
        hmac_chba_save_update_beacon_pnf(hmac_chba_sync_get_domain_bssid());
        return OAL_SUCC;
    } else if (cfg_coex_chan_info->cfg_cmd_type == HMAC_CHBA_ISLAND_COEX_CFG_TYPE) {
        hmac_device->island_coex_info.island_coex_chan_cnt = cfg_coex_chan_info->supported_channel_cnt;
        if (memcpy_s(hmac_device->island_coex_info.island_coex_channels_list, MAX_CHANNEL_NUM_FREQ_5G,
            cfg_coex_chan_info->supported_channel_list, cfg_coex_chan_info->supported_channel_cnt) != EOK) {
            return OAL_FAIL;
        }
    }
    // 抛事件到dmac
    ret = hmac_config_send_event(mac_vap, WLAN_CFGID_CHBA_SET_COEX_CHAN_INFO, sizeof(mac_chba_set_coex_chan_info),
        (uint8_t *)cfg_coex_chan_info);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG1(mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_chba_set_coex_chan:set coex chan err[%u]}", ret);
        return ret;
    }
    return OAL_SUCC;
}

#endif
