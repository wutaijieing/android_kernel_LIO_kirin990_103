

#ifdef _PRE_WLAN_CHBA_MGMT
/* 1 头文件包含 */
#include "hmac_chba_interface.h"
#include "hmac_chba_frame.h"
#include "hmac_chba_function.h"
#include "hmac_chba_mgmt.h"
#include "hmac_chba_ps.h"
#include "hmac_chba_chan_switch.h"
#include "hmac_chba_sync.h"
#include "hmac_chba_coex.h"
#include "hmac_config.h"
#include "hmac_chan_mgmt.h"
#include "hmac_chba_frame.h"
#include "hmac_chba_user.h"
#include "mac_ie.h"
#include "wlan_types.h"
#include "mac_device.h"
#include "oal_cfg80211.h"
#include "securec.h"
#include "securectype.h"
#include "hmac_hid2d.h"
#include "wal_linux_ioctl.h"
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include <linux/ktime.h>
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_INTERFACE_C
/* 3 函数声明 */
/* 4 函数实现 */

void hmac_chba_vap_start(uint8_t *mac_addr, mac_channel_stru *social_channel,
    mac_channel_stru *work_channel)
{
    int32_t ret;
    mac_chba_vap_start_rpt chba_vap_start_info;

    ret = memcpy_s(chba_vap_start_info.mac_addr, WLAN_MAC_ADDR_LEN, mac_addr, WLAN_MAC_ADDR_LEN);
    ret += memcpy_s(&chba_vap_start_info.social_channel, sizeof(mac_channel_stru),
        social_channel, sizeof(mac_channel_stru));
    ret += memcpy_s(&chba_vap_start_info.work_channel, sizeof(mac_channel_stru),
        work_channel, sizeof(mac_channel_stru));
    if (ret != EOK) {
        oam_warning_log1(0, OAM_SF_CHBA, "{hmac_chba_vap_start_report::memcpy fail, ret[%d].}", ret);
        return;
    }

    hmac_chba_vap_start_proc(&chba_vap_start_info);
}


void hmac_chba_vap_stop(void)
{
    uint8_t notify_msg = 0;
    oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_vap_stop_report::send HMAC_MAC_CHBA_VAP_STOP_RPT to ext");
    hmac_chba_vap_stop_proc(&notify_msg);
}


void hmac_chba_stub_channel_switch_report(hmac_vap_stru *hmac_vap, mac_channel_stru *target_channel)
{
    mac_chba_stub_chan_switch_rpt info_report;
    info_report.target_channel = *target_channel;

    hmac_chba_stub_chan_switch_report_proc(hmac_vap, (uint8_t *)(&info_report));
}


uint32_t hmac_chba_params_sync(mac_vap_stru *mac_vap, chba_params_config_stru *chba_params_sync)
{
    dmac_tx_event_stru *tx_event = NULL;
    frw_event_mem_stru *event_mem = NULL;
    oal_netbuf_stru *cmd_netbuf = NULL;
    frw_event_stru *hmac_to_dmac_ctx_event = NULL;
    uint32_t ret;
    int32_t err;

    if (oal_any_null_ptr2(mac_vap, chba_params_sync)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 申请netbuf */
    cmd_netbuf = oal_mem_netbuf_alloc(OAL_NORMAL_NETBUF, sizeof(chba_params_config_stru), OAL_NETBUF_PRIORITY_MID);
    if (cmd_netbuf == NULL) {
        oam_error_log0(0, OAM_SF_CHBA, "hmac_chba_params_sync::netbuf alloc null!");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* 拷贝命令结构体到netbuf */
    err = memcpy_s(oal_netbuf_data(cmd_netbuf), sizeof(chba_params_config_stru),
        chba_params_sync, sizeof(chba_params_config_stru));
    if (err != EOK) {
        oam_error_log0(0, OAM_SF_CHBA, "hmac_chba_params_sync::memcpy fail!");
        oal_netbuf_free(cmd_netbuf);
        return OAL_FAIL;
    }
    oal_netbuf_put(cmd_netbuf, sizeof(chba_params_config_stru));

    /* 抛事件到DMAC */
    event_mem = frw_event_alloc_m(sizeof(dmac_tx_event_stru));
    if (event_mem == NULL) {
        oam_error_log0(0, OAM_SF_CHBA, "hmac_chba_params_sync::event_mem null.");
        oal_netbuf_free(cmd_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_to_dmac_ctx_event = (frw_event_stru *)event_mem->puc_data;
    frw_event_hdr_init(&(hmac_to_dmac_ctx_event->st_event_hdr), FRW_EVENT_TYPE_WLAN_CTX,
        DMAC_WLAN_CTX_EVENT_SUB_TYPE_CHBA_PARAMS, sizeof(dmac_tx_event_stru), FRW_EVENT_PIPELINE_STAGE_1,
        mac_vap->uc_chip_id, mac_vap->uc_device_id, mac_vap->uc_vap_id);

    tx_event = (dmac_tx_event_stru *)(hmac_to_dmac_ctx_event->auc_event_data);
    tx_event->pst_netbuf = cmd_netbuf;
    tx_event->us_frame_len = oal_netbuf_len(cmd_netbuf);

    ret = frw_event_dispatch_event(event_mem);
    if (ret != OAL_SUCC) {
        oam_error_log1(0, OAM_SF_CHBA, "hmac_chba_params_sync::dispatch_event fail[%d]!", ret);
    }

    oal_netbuf_free(cmd_netbuf);
    frw_event_free_m(event_mem);

    return ret;
}


uint32_t hmac_config_vap_discovery_channel(mac_vap_stru *mac_vap, hmac_chba_vap_stru *chba_vap_info)
{
    uint32_t ret;
    chba_h2d_channel_cfg_stru channel_cfg = { 0 };

    if (mac_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    channel_cfg.chan_cfg_type_bitmap = CHBA_CHANNEL_CFG_BITMAP_DISCOVER;
    channel_cfg.channel = chba_vap_info->social_channel;
    ret = hmac_config_send_event(mac_vap, WLAN_CFGID_CHBA_SET_CHANNEL, sizeof(chba_h2d_channel_cfg_stru),
        (uint8_t *)&channel_cfg);
    if (ret != OAL_SUCC) {
        oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_config_vap_discovery_channel::config vap work channel err[%u]}", ret);
        return ret;
    }

    return OAL_SUCC;
}

uint32_t hmac_chba_update_work_channel(mac_vap_stru *mac_vap)
{
    hmac_chba_vap_stru *chba_vap_info = NULL;

    chba_h2d_channel_cfg_stru channel_cfg = { 0 };
    uint32_t ret;

    if (mac_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    chba_vap_info = hmac_get_up_chba_vap_info();
    if (chba_vap_info == NULL) {
        return OAL_SUCC;
    }

    channel_cfg.chan_cfg_type_bitmap = CHBA_CHANNEL_CFG_BITMAP_WORK;
    channel_cfg.channel = chba_vap_info->work_channel;
    ret = hmac_config_send_event(mac_vap, WLAN_CFGID_CHBA_SET_CHANNEL, sizeof(chba_h2d_channel_cfg_stru),
        (uint8_t *)&channel_cfg);
    if (ret != OAL_SUCC) {
        oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_update_work_channel:config vap work channel err[%u]", ret);
        return ret;
    }
    return OAL_SUCC;
}


uint32_t hmac_chba_params_sync_after_start(mac_vap_stru *mac_vap, hmac_chba_vap_stru *chba_vap)
{
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();
    hmac_chba_system_params *system_params = hmac_get_chba_system_params();
    chba_params_config_stru chba_params_sync = { 0 };
    uint32_t ret;

    if (oal_any_null_ptr2(mac_vap, chba_vap)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 填写下发到dmac的结构体 */
    chba_params_sync.schedule_period = (uint8_t)system_params->schedule_period;
    chba_params_sync.island_beacon_slot = 2; /* 岛主转发beacon的时隙为2 */
    chba_params_sync.chba_state = hmac_chba_get_sync_state();
    chba_params_sync.chba_role = hmac_chba_get_role();
    chba_params_sync.master_info = sync_info->cur_master_info;
    oal_set_mac_addr(chba_params_sync.domain_bssid, mac_vap->auc_bssid);

    /* 设置info_bitmap */
    chba_params_sync.info_bitmap =
        CHBA_MASTER_INFO | CHBA_SCHD | CHBA_ISLAND_BEACON | CHBA_STATE | CHBA_ROLE | CHBA_BSSID;
    oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_params_sync_after_start::sync params to dmac, info bitmap 0x%04X",
        chba_params_sync.info_bitmap);
    ret = hmac_chba_params_sync(mac_vap, &chba_params_sync);
    if (ret != OAL_SUCC) {
        oam_warning_log1(0, OAM_SF_CHBA,
            "hmac_chba_params_sync_after_start::send chba params to dmac err[%u]", ret);
        return ret;
    }

    /* 配置social信道和work信道 */
    hmac_config_vap_discovery_channel(mac_vap, chba_vap);
    hmac_chba_update_work_channel(mac_vap);

    return OAL_SUCC;
}



uint32_t hmac_chba_update_beacon_pnf(mac_vap_stru *mac_vap, uint8_t *domain_bssid, uint8_t frame_type)
{
    hmac_chba_vap_stru *chba_vap_info = NULL;
    chba_params_config_stru chba_params_sync = { 0 };
    uint8_t *frame_buf = NULL;
    uint32_t ret;

    if (oal_any_null_ptr2(mac_vap, domain_bssid)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    chba_vap_info = hmac_get_up_chba_vap_info();
    if (chba_vap_info == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (frame_type == MAC_CHBA_SYNC_BEACON) {
        frame_buf = chba_params_sync.sync_beacon;
        if (chba_vap_info->beacon_buf_len == 0) {
            oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_update_beacon_pnf: no saved beacon.");
            return OAL_SUCC;
        }
        hmac_chba_encap_sync_beacon_frame(mac_vap, frame_buf, &chba_params_sync.sync_beacon_len,
            domain_bssid, chba_vap_info->beacon_buf, chba_vap_info->beacon_buf_len);
        chba_params_sync.info_bitmap = CHBA_BEACON_BUF | CHBA_BEACON_LEN;
    } else if (frame_type == MAC_CHBA_PNF) {
        frame_buf = chba_params_sync.pnf;
        hmac_chba_encap_pnf_action_frame(mac_vap, frame_buf, &chba_params_sync.pnf_len);
        chba_params_sync.info_bitmap = CHBA_PNF_BUF | CHBA_PNF_LEN;
    } else {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_update_beacon_pnf: invalid frame type.");
        return OAL_SUCC;
    }

    ret = hmac_chba_params_sync(mac_vap, &chba_params_sync);
    if (ret != OAL_SUCC) {
        return ret;
    }

    return OAL_SUCC;
}

uint8_t hmac_chba_bw_mode_convert(uint8_t band, uint8_t channel_num, uint8_t bw)
{
    uint8_t idx, bandwidth;
    hmac_chba_chan_stru *candidate_list = NULL;
    uint8_t total_num;

    if (band == WLAN_BAND_2G) {
        candidate_list = (hmac_chba_chan_stru *)g_aus_channel_candidate_list_2g;
        total_num = HMAC_MAC_CHBA_CHANNEL_NUM_2G;
    } else {
        candidate_list =  (hmac_chba_chan_stru *)g_aus_channel_candidate_list_5g;
        total_num = HMAC_MAC_CHBA_CHANNEL_NUM_5G;
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

uint32_t hmac_chba_chan_convert_center_freq_to_bw(mac_kernel_channel_stru *kernel_channel,
    mac_channel_stru *mac_channel)
{
    int32_t primer_channel;
    uint32_t ret;
    int32_t err;

    if (oal_any_null_ptr2(kernel_channel, mac_channel)) {
        return OAL_FAIL;
    }

    oam_warning_log3(0, OAM_SF_CHBA, "hmac_chba_chan_convert_center_freq_to_bw::channel from kernel: \
        centerfreq20M[%d]MHz, centerfreq1[%d]MHz, bandwidth[%d]", kernel_channel->center_freq_20m,
        kernel_channel->center_freq1, kernel_channel->bandwidth);
    /* 填写band */
    if (kernel_channel->center_freq_20m <= 2484) { /* 2.4G最大中心频点为2484MHz */
        mac_channel->en_band = WLAN_BAND_2G;
    } else if (kernel_channel->center_freq_20m < 5955) { /* 暂不支持6G，5955是6G的起始中心频点 */
        mac_channel->en_band = WLAN_BAND_5G;
    } else {
        return OAL_FAIL;
    }

    /* 获取主20M信道号与中心频点对应的信道号 */
    primer_channel = oal_ieee80211_frequency_to_channel(kernel_channel->center_freq_20m);
    /* 判断信道号是否合法 */
    err = (int32_t)mac_is_channel_num_valid(mac_channel->en_band, (uint8_t)primer_channel, OAL_FALSE);
    if (err != OAL_SUCC) {
        return OAL_FAIL;
    }
    mac_channel->uc_chan_number = (uint8_t)primer_channel;
    /* 暂不支持80M+80M与160M模式，后续升级后进行处理 */
    if (kernel_channel->bandwidth > NL80211_CHAN_WIDTH_80) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_chan_convert_center_freq_to_bw:not support 80M + 80M or 160M");
        return OAL_FAIL;
    }
    mac_channel->en_bandwidth = hmac_chba_bw_mode_convert(mac_channel->en_band, (uint8_t)primer_channel,
        kernel_channel->bandwidth);
    /* 填写channel_idx */
    ret = mac_get_channel_idx_from_num(mac_channel->en_band, (uint8_t)primer_channel,
        OAL_FALSE, &(mac_channel->uc_chan_idx));
    if (ret != OAL_SUCC) {
        return ret;
    }

    oam_warning_log4(0, OAM_SF_CHBA, "hmac_chba_chan_convert_center_freq_to_bw::channel at hmac: band[%d], \
        uc_chan_idx[%d], uc_chan_number[%d], bandwidth[%d]", mac_channel->en_band, mac_channel->uc_chan_idx,
        mac_channel->uc_chan_number, mac_channel->en_bandwidth);

    return OAL_SUCC;
}

uint32_t hmac_chba_chan_convert_bw_to_center_freq(mac_kernel_channel_stru *kernel_channel,
    mac_channel_stru *mac_channel, mac_vap_stru *mac_vap)
{
    if (oal_any_null_ptr3(kernel_channel, mac_channel, mac_vap)) {
        return OAL_FAIL;
    }

    kernel_channel->center_freq_20m = (mac_channel->en_band == WLAN_BAND_2G) ?
        (g_ast_freq_map_2g[mac_channel->uc_chan_idx].us_freq) :
        (g_ast_freq_map_5g[mac_channel->uc_chan_idx].us_freq);
    kernel_channel->bandwidth = (oal_nl80211_chan_width)WLAN_BANDWIDTH_TO_IEEE_CHAN_WIDTH(mac_channel->en_bandwidth,
        mac_mib_get_HighThroughputOptionImplemented(mac_vap));
    kernel_channel->center_freq1 = mac_get_center_freq1_from_freq_and_bandwidth(kernel_channel->center_freq_20m,
        mac_channel->en_bandwidth);
    return OAL_SUCC;
}
#endif /* end of this file */

