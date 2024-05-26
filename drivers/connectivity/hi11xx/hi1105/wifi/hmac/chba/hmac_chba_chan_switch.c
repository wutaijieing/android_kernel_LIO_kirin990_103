

/* 1 头文件包含 */
#include "hmac_chba_chan_switch.h"
#include "hmac_chba_common_function.h"
#include "hmac_chba_function.h"
#include "hmac_chba_mgmt.h"
#include "hmac_chba_interface.h"
#include "hmac_chba_frame.h"
#include "hmac_chba_sync.h"
#include "hmac_chba_coex.h"
#include "hmac_chba_ps.h"
#include "hmac_chba_coex.h"
#include "hmac_mgmt_sta.h"
#include "mac_frame_inl.h"
#include "hmac_chan_mgmt.h"
#include "securec.h"
#include "oam_event_wifi.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_CHAN_SWITCH_C

#ifdef _PRE_WLAN_CHBA_MGMT
uint8_t g_chan_switch_enable = OAL_FALSE;


static void hmac_chba_set_chan_switch_attr(hmac_chba_chan_switch_notify_stru *chan_switch_info, uint8_t *attr_buf)
{
    uint8_t index;

    /* Attribute ID */
    attr_buf[0] = MAC_CHBA_ATTR_CHAN_SWITCH;
    index = MAC_CHBA_ATTR_HDR_LEN;

    attr_buf[index++] = chan_switch_info->notify_type;
    attr_buf[index++] = chan_switch_info->chan_number;
    attr_buf[index++] = chan_switch_info->bandwidth;
    attr_buf[index++] = chan_switch_info->switch_slot;
    attr_buf[index++] = chan_switch_info->status_code;

    /* 设置Attr Length字段 */
    attr_buf[MAC_CHBA_ATTR_ID_LEN] = index - MAC_CHBA_ATTR_HDR_LEN;
}

static uint32_t hmac_chba_encap_csn_action(hmac_vap_stru *hmac_vap, uint8_t *frame_buf,
    hmac_chba_chan_switch_notify_stru *chan_switch_ntf, uint8_t *peer_addr)
{
    uint16_t total_len = 0;
    hmac_chba_vap_stru *chba_vap_info = hmac_get_chba_vap();
    mac_vap_stru *mac_vap = NULL;
    uint8_t *action_buf = NULL;

    if (oal_any_null_ptr2(hmac_vap, frame_buf)) {
        return total_len;
    }

    mac_vap = &(hmac_vap->st_vap_base_info);
    mac_hdr_set_frame_control(frame_buf, WLAN_PROTOCOL_VERSION | WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_ACTION);
    oal_set_mac_addr(frame_buf + WLAN_HDR_ADDR1_OFFSET, peer_addr);
    oal_set_mac_addr(frame_buf + WLAN_HDR_ADDR2_OFFSET, mac_mib_get_StationID(mac_vap));
    mac_set_domain_bssid(frame_buf + WLAN_HDR_ADDR3_OFFSET, mac_vap, chba_vap_info);
    total_len += MAC_80211_FRAME_LEN;
    action_buf = frame_buf + total_len;
    mac_set_chba_action_hdr(action_buf, MAC_CHBA_CHANNEL_SWITCH_NOTIFICATION);
    // 如果CSN是广播地址，则修改category为vendor
    if (ether_is_multicast(peer_addr)) {
        action_buf[0] = MAC_ACTION_CATEGORY_VENDOR;
    }
    total_len += MAC_CHBA_ACTION_HDR_LEN;

    /* 封装Channel Switch属性 */
    hmac_chba_set_chan_switch_attr(chan_switch_ntf, frame_buf + total_len);
    total_len += MAC_CHBA_CHAN_SWITCH_ATTR_LEN;
    return total_len;
}


static uint32_t hmac_chba_send_chan_switch_notify_action(uint8_t *peer_addr,
    hmac_chba_chan_switch_notify_stru *chan_switch_ntf)
{
    uint32_t ret;
    hmac_chba_vap_stru *chba_vap_info = hmac_get_chba_vap();
    hmac_vap_stru *hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(chba_vap_info->mac_vap_id);
    oal_netbuf_stru *frame_buf = NULL;
    mac_tx_ctl_stru *tx_ctl = NULL;
    uint32_t frame_len;

    if (oal_any_null_ptr2(peer_addr, chan_switch_ntf)) {
        return OAL_FAIL;
    }
    frame_buf = oal_mem_netbuf_alloc(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if (frame_buf == NULL) {
        oam_error_log0(0, OAM_SF_CHBA, "hmac_chba_send_chan_switch_notify_action::frame alloc failed.");
        return OAL_ERR_CODE_PTR_NULL;
    }
    oal_mem_netbuf_trace(frame_buf, OAL_TRUE);

    /* 将mac header清零 */
    memset_s(oal_netbuf_cb(frame_buf), oal_netbuf_cb_size(), 0, oal_netbuf_cb_size());
    memset_s((uint8_t *)oal_netbuf_header(frame_buf), MAC_80211_FRAME_LEN, 0, MAC_80211_FRAME_LEN);

    frame_len = hmac_chba_encap_csn_action(hmac_vap, (uint8_t *)(oal_netbuf_header(frame_buf)),
        chan_switch_ntf, peer_addr);
    if (frame_len == 0) {
        oal_netbuf_free(frame_buf);
        return OAL_FAIL;
    }

    tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(frame_buf);
    MAC_GET_CB_MPDU_LEN(tx_ctl) = (uint16_t)frame_len;
    mac_vap_set_cb_tx_user_idx(&hmac_vap->st_vap_base_info, tx_ctl, peer_addr);
    MAC_GET_CB_WME_AC_TYPE(tx_ctl) = WLAN_WME_AC_MGMT;
    oal_netbuf_put(frame_buf, frame_len);

    ret = hmac_tx_mgmt_send_event(&(hmac_vap->st_vap_base_info), frame_buf, frame_len);
    if (ret != OAL_SUCC) {
        oal_netbuf_free(frame_buf);
        oam_warning_log1(0, 0, "hmac_send_chba_mgmt_action::hmac_tx_mgmt_send_event failed[%d].", ret);
        return ret;
    }
    oam_warning_log4(0, OAM_SF_CHBA,
        "hmac_chba_send_chan_switch_notify_action::CSN action tx, type %d, peer_addr XX:XX:XX:%2X:%2X:%2X",
        chan_switch_ntf->notify_type, peer_addr[MAC_ADDR_3], peer_addr[MAC_ADDR_4], peer_addr[MAC_ADDR_5]);

    return OAL_SUCC;
}

static uint32_t hmac_chba_vap_work_chan_switch(mac_vap_stru *mac_vap, mac_channel_stru *mac_channel,
    uint8_t switch_slot)
{
    uint32_t ret;
    chba_h2d_channel_cfg_stru channel_cfg = { 0 };

    if (oal_any_null_ptr2(mac_vap, mac_channel)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    channel_cfg.chan_cfg_type_bitmap = CHBA_CHANNEL_CFG_BITMAP_WORK;
    channel_cfg.chan_switch_slot_idx = switch_slot;
    channel_cfg.channel = *mac_channel;
    ret = hmac_config_send_event(mac_vap, WLAN_CFGID_CHBA_SET_CHANNEL, sizeof(chba_h2d_channel_cfg_stru),
        (uint8_t *)&channel_cfg);
    if (ret != OAL_SUCC) {
        oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_chba_config_channel::config vap work channel err[%u]}", ret);
        return ret;
    }

    return OAL_SUCC;
}

static uint32_t hmac_chba_work_channel_switch(hmac_vap_stru *hmac_vap, mac_channel_stru *mac_channel,
    uint8_t switch_slot)
{
    mac_vap_stru *mac_vap = NULL;
    hmac_chba_vap_stru *chba_vap_info = hmac_get_chba_vap();

    if (hmac_vap == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_work_channel_switch::hmac_vap NULL");
        return OAL_ERR_CODE_PTR_NULL;
    }
    mac_vap = &hmac_vap->st_vap_base_info;
    if (hmac_chba_chan_check_is_valid(hmac_vap, mac_channel) == OAL_FALSE) {
        oam_error_log0(mac_vap->uc_vap_id, OAM_SF_CHBA, "hmac_chba_work_channel_switch:chba chan check_is not valid!");
        return OAL_FAIL;
    }

    oam_warning_log3(0, OAM_SF_CHBA, "hmac_chba_work_channel_switch::chan %d, bw %d, update cnt %d",
        mac_channel->uc_chan_number, mac_channel->en_bandwidth, switch_slot);
    /* 配置social信道 */
    hmac_config_social_channel(chba_vap_info, mac_channel->uc_chan_number, mac_channel->en_bandwidth);

    /* 配置vap信道切换 */
    hmac_chba_vap_work_chan_switch(mac_vap, mac_channel, switch_slot);

    /* 将新的工作信道保存在work channel中 */
    chba_vap_info->work_channel = *mac_channel;
    return OAL_SUCC;
}


uint32_t hmac_chba_chan_work_channel_switch(hmac_vap_stru *hmac_vap, uint8_t channel_number,
    uint8_t bandwidth, uint8_t switch_slot)
{
    uint8_t device_id;
    uint32_t ret;
    mac_chba_set_work_chan set_work_chan = { 0 };
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();
    hmac_chba_per_device_id_info_stru *device_id_info = hmac_chba_get_device_id_info();

    set_work_chan.switch_slot = switch_slot;
    set_work_chan.channel.uc_chan_number = channel_number;
    set_work_chan.channel.en_bandwidth = bandwidth;
    set_work_chan.channel.en_band = (channel_number > MAX_CHANNEL_NUM_FREQ_2G) ? WLAN_BAND_5G : WLAN_BAND_2G;
    ret = mac_get_channel_idx_from_num(set_work_chan.channel.en_band, set_work_chan.channel.uc_chan_number,
        OAL_FALSE, &(set_work_chan.channel.uc_chan_idx));
    if (ret != OAL_SUCC) {
        return ret;
    }

    /* 配置信道切换 */
    hmac_chba_work_channel_switch(hmac_vap, &set_work_chan.channel, set_work_chan.switch_slot);

    device_id = self_conn_info->self_device_id;
    device_id_info[device_id].chan_number = set_work_chan.channel.uc_chan_number;
    device_id_info[device_id].bandwidth = set_work_chan.channel.en_bandwidth;

    /* 更新最新的Beacon和RNF到Driver */
    hmac_chba_save_update_beacon_pnf(hmac_chba_sync_get_domain_bssid());

    return OAL_SUCC;
}


static uint32_t hmac_chba_indicate_island_devices_switch_chan(hmac_chba_mgmt_info_stru *chba_mgmt_info,
    mac_channel_stru *target_channel, uint8_t switch_slot)
{
    uint32_t ret, idx;
    hmac_chba_chan_switch_notify_stru chan_switch_ntf = { 0 };
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();
    mac_chba_per_device_info_stru *device_list = NULL;

    device_list = self_conn_info->island_device_list;

    /* 填写信道切换指示信息 */
    chan_switch_ntf.notify_type = MAC_CHBA_CHAN_SWITCH_INDICATION;
    chan_switch_ntf.chan_number = target_channel->uc_chan_number;
    chan_switch_ntf.bandwidth = target_channel->en_bandwidth;
    chan_switch_ntf.switch_slot = switch_slot;

    /* 初始化信道切换指示帧的ack标记为false */
    for (idx = 0; idx < self_conn_info->island_device_cnt; idx++) {
        device_list[idx].chan_switch_ack = FALSE;
    }

    /* 广播发送信道切换指示帧 */
    oam_warning_log1(0, OAM_SF_CHBA,
        "hmac_chba_indicate_island_devices_switch_chan::notify island device switch chan. deviceCnt %d",
        self_conn_info->island_device_cnt);
    ret = hmac_chba_send_chan_switch_notify_action(BROADCAST_MACADDR, &chan_switch_ntf);
    if (ret != OAL_SUCC) {
        oam_warning_log1(0, OAM_SF_CHBA,
            "hmac_chba_indicate_island_devices_switch_chan::hmac_chba_send_chan_switch_notify_action fail[%d]", ret);
        return ret;
    }

    return OAL_SUCC;
}


static uint32_t hmac_chba_retx_chan_switch_ind(mac_chba_self_conn_info_stru *self_conn_info,
    hmac_chba_chan_switch_info_stru *chan_switch_info)
{
    uint32_t ret, idx;
    hmac_chba_chan_switch_notify_stru chan_switch_ntf = { 0 };
    mac_chba_per_device_info_stru *device_list = NULL;
    uint8_t *mac_addr = NULL;
    uint8_t *peer_addr = NULL;

    if (oal_any_null_ptr2(self_conn_info, chan_switch_info)) {
        return OAL_FAIL;
    }

    /* 填写信道切换指示信息 */
    chan_switch_ntf.notify_type = MAC_CHBA_CHAN_SWITCH_INDICATION;
    chan_switch_ntf.chan_number = chan_switch_info->last_target_channel;
    chan_switch_ntf.bandwidth = chan_switch_info->last_target_channel_bw;
    chan_switch_ntf.switch_slot = MAC_CHBA_CHAN_SWITCH_IMMEDIATE;

    /* 给岛内没有收到ack的节点发送信道切换指示帧 */
    oam_warning_log2(0, OAM_SF_CHBA, "hmac_chba_retx_chan_switch_ind::switch chan[%d], bw[%d]",
        chan_switch_ntf.chan_number, chan_switch_ntf.bandwidth);
    device_list = self_conn_info->island_device_list;
    mac_addr = hmac_chba_get_own_mac_addr();
    for (idx = 0; idx < self_conn_info->island_device_cnt; idx++) {
        peer_addr = device_list[idx].mac_addr;
        /* 跳过本设备自己，跳过收到ack的设备 */
        if ((oal_compare_mac_addr(mac_addr, peer_addr) == 0) ||
            (device_list[idx].chan_switch_ack == TRUE)) {
            continue;
        }

        /* 发送信道切换指示帧 */
        ret = hmac_chba_send_chan_switch_notify_action(peer_addr, &chan_switch_ntf);
        if (ret != OAL_SUCC) {
            return ret;
        }
    }

    return OAL_SUCC;
}

static void hmac_chba_clear_csa_lost_device_flag(hmac_chba_chan_switch_info_stru *chan_switch_info)
{
    chan_switch_info->csa_lost_device_num = 0;
    chan_switch_info->csa_save_lost_device_flag = FALSE;
}

/*
 * 功能描述  : dmac->hmac事件，补救未收到ack的设备流程
 */
uint32_t hmac_chba_csa_lost_device_proc(mac_vap_stru *mac_vap, uint8_t len, uint8_t *param)
{
    hmac_chba_mgmt_info_stru *chba_mgmt_info = hmac_chba_get_mgmt_info();
    hmac_chba_chan_switch_info_stru *chan_switch_info = &chba_mgmt_info->chan_switch_info;
    uint32_t idx, ret;
    uint8_t *peer_addr = NULL;
    hmac_chba_chan_switch_notify_stru chan_switch_ntf = { 0 };

    /* 补救流程结束，清标志位 */
    if (*param == 0) {
        hmac_chba_clear_csa_lost_device_flag(chan_switch_info);
        return OAL_SUCC;
    }

    /* 补救流程开始 */
    chan_switch_info->csa_save_lost_device_flag = TRUE;
    /* 填写信道切换指示信息 */
    chan_switch_ntf.notify_type = MAC_CHBA_CHAN_SWITCH_INDICATION;
    chan_switch_ntf.chan_number = chan_switch_info->last_target_channel;
    chan_switch_ntf.bandwidth = chan_switch_info->last_target_channel_bw;
    chan_switch_ntf.switch_slot = MAC_CHBA_CHAN_SWITCH_IMMEDIATE;

    /* 给没有收到ack的节点发送信道切换指示帧 */
    for (idx = 0; idx < chan_switch_info->csa_lost_device_num; idx++) {
        peer_addr = chan_switch_info->csa_lost_device[idx].mac_addr;
        /* 发送信道切换指示帧 */
        ret = hmac_chba_send_chan_switch_notify_action(peer_addr, &chan_switch_ntf);
        if (ret != OAL_SUCC) {
            return OAL_FALSE;
        }
    }

    return OAL_SUCC;
}

static uint8_t hmac_chba_lost_device_check_and_save(hmac_chba_vap_stru *chba_vap_info,
    hmac_chba_mgmt_info_stru *chba_mgmt_info, hmac_chba_chan_switch_info_stru *chan_switch_info)
{
    uint32_t idx;
    uint8_t *mac_addr = NULL;
    uint8_t *peer_addr = NULL;
    mac_chba_per_device_info_stru *device_list = NULL;
    uint8_t device_cnt;

    device_list = chba_mgmt_info->self_conn_info.island_device_list;
    device_cnt = chba_mgmt_info->self_conn_info.island_device_cnt;
    mac_addr = hmac_chba_get_own_mac_addr();
    for (idx = 0; idx < device_cnt; idx++) {
        peer_addr = device_list[idx].mac_addr;
        /* 跳过本设备自己，跳过收到ack的设备 */
        if ((oal_compare_mac_addr(mac_addr, peer_addr) == 0) ||
            (device_list[idx].chan_switch_ack == TRUE)) {
            continue;
        }

        /* 记录原信道, 未收到ack的对端设备mac地址。用于后续补救。 */
        if (chan_switch_info->csa_lost_device_num == 0) {
            chan_switch_info->old_channel = chba_vap_info->work_channel;
        }

        if (memcpy_s(chan_switch_info->csa_lost_device[chan_switch_info->csa_lost_device_num].mac_addr,
            OAL_MAC_ADDR_LEN, peer_addr, OAL_MAC_ADDR_LEN) != OAL_SUCC) {
            return 0;
        }

        oam_warning_log3(0, OAM_SF_CHBA, "hmac_chba_lost_device_check_and_save::lost_device_mac[XX:XX:XX:%2X:%2X:%2X]",
            chan_switch_info->csa_lost_device[chan_switch_info->csa_lost_device_num].mac_addr[3],
            chan_switch_info->csa_lost_device[chan_switch_info->csa_lost_device_num].mac_addr[4],
            chan_switch_info->csa_lost_device[chan_switch_info->csa_lost_device_num].mac_addr[5]);

        chan_switch_info->csa_lost_device_num++;
    }

    return chan_switch_info->csa_lost_device_num;
}

static void hmac_chba_csa_lost_device_notify(mac_vap_stru *mac_vap, hmac_chba_chan_switch_info_stru *chan_switch_info)
{
    chba_h2d_lost_device_info_stru lost_device_info = { 0 };
    uint32_t ret, idx;

    lost_device_info.csa_lost_device_num = chan_switch_info->csa_lost_device_num;
    if (lost_device_info.csa_lost_device_num != 0) {
        lost_device_info.old_channel = chan_switch_info->old_channel;
        for (idx = 0; idx < lost_device_info.csa_lost_device_num; idx++) {
            if (memcpy_s(lost_device_info.csa_lost_device[idx].mac_addr, OAL_MAC_ADDR_LEN,
                chan_switch_info->csa_lost_device[idx].mac_addr, OAL_MAC_ADDR_LEN) != OAL_SUCC) {
                return;
            }
        }
    }

    ret = hmac_config_send_event(mac_vap, WLAN_CFGID_CHBA_CSA_LOST_DEVICE_NOTIFY,
                                 sizeof(chba_h2d_lost_device_info_stru), (uint8_t *)(&lost_device_info));
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_csa_lost_device_notify::notify_csa_lost_device fail[%u]", ret);
    }

    oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_csa_lost_device_notify::Notify dmac lost device or rcv all confirm! \
        csa_lost_device_num=[%d]", chan_switch_info->csa_lost_device_num);
}

uint32_t hmac_chba_retx_chan_switch_ind_timeout(void *p)
{
    hmac_chba_chan_switch_info_stru *chan_switch_info = NULL;
    hmac_chba_mgmt_info_stru *chba_mgmt_info = hmac_chba_get_mgmt_info();
    hmac_chba_vap_stru *chba_vap_info = hmac_get_chba_vap();
    hmac_vap_stru *hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(chba_vap_info->mac_vap_id);

    if (hmac_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    chan_switch_info = &chba_mgmt_info->chan_switch_info;
    if (chan_switch_info->chan_switch_timer.en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(&(chan_switch_info->chan_switch_timer));
    }

    /* 保存未收到ack的设备mac地址，已备后续补救, 并通知dmac计算出补救的时间点 */
    if (hmac_chba_lost_device_check_and_save(chba_vap_info, chba_mgmt_info, chan_switch_info) > 0) {
        hmac_chba_csa_lost_device_notify(&(hmac_vap->st_vap_base_info), chan_switch_info);
    }

    /* 抛事件给DMAC，配置自己的信道切换 */
    hmac_chba_chan_work_channel_switch(hmac_vap, chan_switch_info->last_target_channel,
        chan_switch_info->last_target_channel_bw, MAC_CHBA_CHAN_SWITCH_IMMEDIATE);

    /* 标记岛信道切换流程结束 */
    chan_switch_info->in_island_chan_switch = FALSE;
    return OAL_SUCC;
}

/*
 * 功能描述  : 检查是否存在保活设备没回CSN confirm帧
 */
uint32_t hmac_chba_check_keepalive_non_confirm(mac_chba_self_conn_info_stru *self_conn_info)
{
    uint32_t idx;
    uint8_t *peer_addr = NULL;
    uint8_t *mac_addr = hmac_chba_get_own_mac_addr();
    mac_chba_per_device_info_stru *device_list = self_conn_info->island_device_list;

    for (idx = 0; idx < self_conn_info->island_device_cnt; idx++) {
        peer_addr = device_list[idx].mac_addr;
        /* 跳过本设备自己，跳过收到ack的设备, 跳过保活设备 */
        if ((oal_compare_mac_addr(mac_addr, peer_addr) == 0) ||
            (device_list[idx].chan_switch_ack == TRUE)) {
            continue;
        }

        /* 有保活设备没回ack */
        if (device_list[idx].ps_bitmap == CHBA_PS_KEEP_ALIVE_BITMAP) {
            return TRUE;
        }
    }

    return FALSE;
}


uint32_t hmac_chba_collect_chan_switch_ack_timeout(void *p)
{
    uint32_t ret;
    uint32_t timeout = CHBA_RETX_CHAN_SWITCH_IND_TIMEOUT;
    hmac_chba_chan_switch_info_stru *chan_switch_info = NULL;
    hmac_chba_mgmt_info_stru *chba_mgmt_info = hmac_chba_get_mgmt_info();
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();
    hmac_chba_vap_stru *chba_vap_info = hmac_get_chba_vap();
    hmac_vap_stru *hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(chba_vap_info->mac_vap_id);

    if (hmac_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    chan_switch_info = &chba_mgmt_info->chan_switch_info;
    if (chan_switch_info->ack_timer.en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(&(chan_switch_info->ack_timer));
    }

    /* 存在保活设备没回ack，timeout时间加长 */
    if (hmac_chba_check_keepalive_non_confirm(self_conn_info) == TRUE) {
        timeout = CHBA_KEEPALIVE_RETX_CHAN_SWITCH_IND_TIMEOUT;
    }

    /* 给岛内没有收到ack的非保活节点重传信道切换指示帧 */
    ret = hmac_chba_retx_chan_switch_ind(self_conn_info, chan_switch_info);
    if (ret != OAL_SUCC) {
        return ret;
    }

    /* 启动定时器，到期后岛主切换信道 */
    oam_warning_log1(0, OAM_SF_CHBA,
        "hmac_chba_collect_chan_switch_ack_timeout::some devices not confirm. time=[%d]", timeout);
    if (chan_switch_info->chan_switch_timer.en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(&(chan_switch_info->chan_switch_timer));
    }
    hmac_chba_create_timer_ms(&(chan_switch_info->chan_switch_timer), timeout,
        hmac_chba_retx_chan_switch_ind_timeout);

    return OAL_SUCC;
}
oal_bool_enum_uint8 hmac_chba_chan_check_is_valid(hmac_vap_stru *hmac_vap, mac_channel_stru *target_channel)
{
    /* 信道有效值判断 */
    if (hmac_chba_is_valid_channel(target_channel->uc_chan_number) != OAL_SUCC) {
        oam_error_log2(0, OAM_SF_CHBA, "hmac_chba_chan_check_is_valid::invalid number %d, bwmode %d.",
            target_channel->uc_chan_number, target_channel->en_bandwidth);
        return OAL_FALSE;
    }
    /* 信道号和带宽参数组合不支持 */
    if (mac_regdomain_channel_is_support_bw(target_channel->en_bandwidth,
        target_channel->uc_chan_number) == OAL_FALSE) {
        oam_error_log2(0, OAM_SF_CHBA, "hmac_chba_chan_check_is_valid::target channel number=[%d], bw=[%d] invalid",
            target_channel->uc_chan_number, target_channel->en_bandwidth);
        return OAL_FALSE;
    }
    if (hmac_chba_coex_start_is_dbac(&hmac_vap->st_vap_base_info, target_channel) != OAL_FALSE) {
        oam_warning_log0(0, 0, "hmac_chba_chan_check_is_valid::work channel dbac");
        return OAL_FALSE;
    }
    return OAL_TRUE;
}

uint32_t hmac_chba_start_switch_chan_for_connect_perf(hmac_vap_stru *hmac_vap,
    hmac_chba_mgmt_info_stru *chba_mgmt_info, mac_channel_stru *target_channel, uint8_t remain_slot, uint8_t type)
{
    uint32_t curr_time;
    hmac_chba_chan_switch_info_stru *chan_switch_info = NULL;
    mac_chba_adjust_chan_info chan_notify = { 0 };
    if (hmac_chba_chan_check_is_valid(hmac_vap, target_channel)  == OAL_FALSE) {
        return OAL_FAIL;
    }

    chan_switch_info = &chba_mgmt_info->chan_switch_info;

    /* 广播channel switch indication帧 */
    hmac_chba_indicate_island_devices_switch_chan(chba_mgmt_info, target_channel, MAC_CHBA_CHAN_SWITCH_IMMEDIATE);
    /* 启动收集ack的定时器 */
    if (chan_switch_info->ack_timer.en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(&(chan_switch_info->ack_timer));
    }
    hmac_chba_create_timer_ms(&(chan_switch_info->ack_timer), CHBA_COLLECT_CHAN_SWITCH_ACK_TIMEOUT,
        hmac_chba_collect_chan_switch_ack_timeout);

    /* 保存目标信道，供重发切换指示时使用 */
    chan_switch_info->last_target_channel = target_channel->uc_chan_number;
    chan_switch_info->last_target_channel_bw = target_channel->en_bandwidth;
    /* 更新上一次信道切换时间 */
    curr_time = (uint32_t)oal_time_get_stamp_ms();
    chan_switch_info->last_chan_switch_time = curr_time;

    /* 标记岛信道切换流程开始，记录当前slot */
    chan_switch_info->in_island_chan_switch = TRUE;
    chan_switch_info->switch_type = type;
    chan_switch_info->csa_lost_device_num = 0;

    /* 通知FWK切换开始 */
    chan_notify.report_type = HMAC_CHBA_CHAN_SWITCH_REPORT;
    chan_notify.chan_number = chan_switch_info->last_target_channel;
    chan_notify.bandwidth = chan_switch_info->last_target_channel_bw;
    chan_notify.switch_type = type;
    chan_notify.status_code = MAC_CHBA_CHAN_SWITCH_START;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_cfg80211_chba_report((uint8_t *)(&chan_notify), sizeof(chan_notify));
#endif

    return OAL_SUCC;
}


void hmac_chba_start_switch_chan_for_avoid_dbac(hmac_vap_stru *hmac_vap,
    mac_channel_stru *target_channel, uint8_t remain_slot)
{
    if (hmac_chba_chan_check_is_valid(hmac_vap, target_channel) == OAL_FALSE) {
        return;
    }

    /* 抛事件给DMAC，配置自己的信道切换 */
    hmac_chba_chan_work_channel_switch(hmac_vap, target_channel->uc_chan_number,
        target_channel->en_bandwidth, MAC_CHBA_CHAN_SWITCH_IMMEDIATE);
    return;
}


static uint32_t hmac_chba_chan_switch_req_timeout(void *p_arg)
{
    uint32_t ret;
    uint32_t curr_time;
    hmac_chba_chan_switch_notify_stru chan_switch_ntf = { 0 };
    hmac_chba_chan_switch_info_stru *chan_switch_info = NULL;
    hmac_chba_mgmt_info_stru *chba_mgmt_info = hmac_chba_get_mgmt_info();
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();

    chan_switch_info = &chba_mgmt_info->chan_switch_info;
    if (chan_switch_info->req_timer.en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(&(chan_switch_info->req_timer));
    }
    chan_switch_info->req_timeout_cnt++;
    oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_chan_switch_req_timeout::req_timeout_cnt %d",
        chan_switch_info->req_timeout_cnt);

    /* 连续两次请求超时，退出切换失败 */
    if (chan_switch_info->req_timeout_cnt >= CHBA_MAX_CONTI_SWITCH_REQ_CNT) {
        oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_chan_switch_req_timeout::req_timeout_cnt %d > 2, give up switch!",
            chan_switch_info->req_timeout_cnt);
        chan_switch_info->req_timeout_cnt = 0;
        return OAL_SUCC;
    }

    /* 填写信道切换action帧信息结构体，切换请求只有notify_type字段有意义 */
    chan_switch_ntf.notify_type = MAC_CHBA_CHAN_SWITCH_REQ;

    /* 岛主无效，不发送请求 */
    if (self_conn_info->island_owner_valid == FALSE) {
        return OAL_SUCC;
    }

    /* 给岛主发送信道切换请求帧 */
    ret = hmac_chba_send_chan_switch_notify_action(self_conn_info->island_owner_addr, &chan_switch_ntf);
    if (ret != OAL_SUCC) {
        return ret;
    }

    /* 更新上次发送时间 */
    curr_time = (uint32_t)oal_time_get_stamp_ms();
    chan_switch_info->last_send_req_time = curr_time;

    /* 启动请求超时定时器 */
    hmac_chba_create_timer_ms(&(chan_switch_info->req_timer), CHBA_CHAN_SWITCH_REQ_TIMEOUT,
        hmac_chba_chan_switch_req_timeout);
    return OAL_SUCC;
}

uint32_t hmac_chba_send_chan_switch_req(hmac_chba_mgmt_info_stru *chba_mgmt_info)
{
    uint32_t ret;
    uint64_t time_offset;
    uint32_t curr_time;
    hmac_chba_chan_switch_notify_stru chan_switch_ntf = { 0 };
    hmac_chba_chan_switch_info_stru *chan_switch_info = NULL;
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();

    if (chba_mgmt_info == NULL) {
        return OAL_FAIL;
    }

    chan_switch_info = &chba_mgmt_info->chan_switch_info;
    curr_time = (uint32_t)oal_time_get_stamp_ms();
    time_offset = (uint32_t)oal_time_get_runtime(chan_switch_info->last_send_req_time, curr_time);
    /* 距上次发送切换请求小于512ms，或者切换请求定时器已经启动，不发送 */
    if ((time_offset < CHBA_CHAN_SWITCH_REQ_TIMEOUT) || (chan_switch_info->req_timer.en_is_registerd == OAL_TRUE)) {
        oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_send_chan_switch_req::timeOffset %d", (uint32_t)time_offset);
        return OAL_SUCC;
    }

    /* 初始化超时次数为0 */
    chan_switch_info->req_timeout_cnt = 0;

    /* 填写信道切换action帧信息结构体，切换请求只有notify_type字段有意义 */
    chan_switch_ntf.notify_type = MAC_CHBA_CHAN_SWITCH_REQ;
    chan_switch_ntf.chan_number = 0;
    chan_switch_ntf.bandwidth = 0;
    chan_switch_ntf.switch_slot = 0;

    /* 岛主无效，不发送请求 */
    if (self_conn_info->island_owner_valid == FALSE) {
        return OAL_SUCC;
    }

    /* 给岛主发送信道切换请求帧 */
    ret = hmac_chba_send_chan_switch_notify_action(self_conn_info->island_owner_addr, &chan_switch_ntf);
    if (ret != OAL_SUCC) {
        return ret;
    }
    oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_send_chan_switch_req::send chan switch req.");

    /* 更新上次发送时间 */
    chan_switch_info->last_send_req_time = curr_time;

    /* 启动请求超时定时器 */
    if (chan_switch_info->req_timer.en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(&(chan_switch_info->req_timer));
    }
    hmac_chba_create_timer_ms(&(chan_switch_info->req_timer), CHBA_CHAN_SWITCH_REQ_TIMEOUT,
        hmac_chba_chan_switch_req_timeout);

    return OAL_SUCC;
}


void hmac_chba_check_island_owner_chan(hmac_vap_stru *hmac_vap, uint8_t *sa_addr, uint8_t *payload,
    uint16_t payload_len)
{
    uint8_t owner_chan_number, owner_chan_bandwidth;
    uint8_t *data = NULL;
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();
    hmac_chba_mgmt_info_stru *chba_mgmt_info = hmac_chba_get_mgmt_info();
    hmac_chba_vap_stru *chba_vap_info = hmac_get_chba_vap();

    /* social信道跟随work信道, 不处理 */
    if (!g_chba_social_chan_cap) {
        return;
    }

    /* 没有链路，不处理 */
    if (hmac_vap->st_vap_base_info.us_user_nums == 0) {
        return;
    }

    /* 如果自己是岛主，不处理 */
    if ((self_conn_info->role == CHBA_MASTER) || (self_conn_info->role == CHBA_ISLAND_OWNER)) {
        return;
    }

    /* 如果不是岛主发送的PNF或者Beacon，不处理 */
    if (oal_compare_mac_addr(sa_addr, self_conn_info->island_owner_addr) != 0) {
        return;
    }

    /* 如果正处于岛信道切换流程中，不处理 */
    if (chba_mgmt_info->chan_switch_info.in_island_chan_switch == TRUE) {
        return;
    }

    data = hmac_find_chba_attr(MAC_CHBA_ATTR_LINK_INFO, payload, payload_len);
    if (data == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "{hmac_chba_check_island_owner_chan::no link info attr}");
        return;
    }
    if (MAC_CHBA_ATTR_HDR_LEN + data[MAC_CHBA_ATTR_ID_LEN] < MAC_CHBA_LINK_INFO_BW_POS + 1) {
        oam_error_log1(0, OAM_SF_CHBA, "{hmac_chba_check_island_owner_chan::link info attr len[%d] invalid!}",
            data[MAC_CHBA_ATTR_ID_LEN]);
        return;
    }

    data += MAC_CHBA_ATTR_HDR_LEN;
    owner_chan_number = *data;
    data++;
    owner_chan_bandwidth = *data;

    /* 与岛主工作信道不一致，切换到岛主所在的信道 */
    if ((chba_vap_info->work_channel.uc_chan_number != owner_chan_number) ||
        (chba_vap_info->work_channel.en_bandwidth != owner_chan_bandwidth)) {
        /* 抛事件给DMAC，配置自己的信道切换 */
        oam_warning_log4(0, OAM_SF_CHBA, "hmac_chba_check_island_owner_chan::check_island_owner_chan. \
            switch from chan %d %d to chan %d %d.", chba_vap_info->work_channel.uc_chan_number,
            chba_vap_info->work_channel.en_bandwidth, owner_chan_number, owner_chan_bandwidth);
        hmac_chba_chan_work_channel_switch(hmac_vap, owner_chan_number,
            owner_chan_bandwidth, MAC_CHBA_CHAN_SWITCH_IMMEDIATE);
    }
    return;
}


static uint32_t hmac_chba_check_all_device_confirm(hmac_chba_chan_switch_info_stru *chan_switch_info,
    uint8_t device_cnt, mac_chba_per_device_info_stru *device_list)
{
    uint32_t idx;
    uint8_t *mac_addr = NULL;
    uint8_t *peer_addr = NULL;

    if (oal_any_null_ptr2(chan_switch_info, device_list)) {
        return OAL_FAIL;
    }

    /* 给岛内没有收到ack的节点发送信道切换指示帧 */
    mac_addr = hmac_chba_get_own_mac_addr();
    for (idx = 0; idx < device_cnt; idx++) {
        peer_addr = device_list[idx].mac_addr;
        /* 跳过本设备自己 */
        if (oal_compare_mac_addr(mac_addr, peer_addr) == 0) {
            continue;
        }

        if (device_list[idx].chan_switch_ack != TRUE) {
            oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_check_all_device_confirm::FAIL");
            return OAL_FAIL;
        }
    }

    return OAL_SUCC;
}

/*
 * 功能描述  : 检查是否有vap正在入网
 */
uint8_t hmac_chba_check_is_vap_in_assoc(void)
{
    mac_vap_stru *other_vap = NULL;
    mac_device_stru *mac_device = NULL;
    uint8_t vap_idx;

    mac_device = mac_res_get_dev(0);
    for (vap_idx = 0; vap_idx < mac_device->uc_vap_num; vap_idx++) {
        other_vap = mac_res_get_mac_vap(mac_device->auc_vap_id[vap_idx]);
        if (other_vap == NULL) {
            oam_warning_log1(0, OAM_SF_CHBA,
                "{hmac_chba_check_is_vap_in_assoc::vap is null! vap id is %d}", mac_device->auc_vap_id[vap_idx]);
            continue;
        }
        if (oal_value_in_valid_range(other_vap->en_vap_state,
            MAC_VAP_STATE_STA_JOIN_COMP, MAC_VAP_STATE_STA_WAIT_ASOC)
            || (other_vap->en_vap_state == MAC_VAP_STATE_ROAMING)) {
            oam_warning_log1(other_vap->uc_vap_id, OAM_SF_CHBA,
                "{hmac_chba_check_is_vap_in_assoc::roaming or connecting state[%d]!}", other_vap->en_vap_state);
            return TRUE;
        }
    }
    return FALSE;
}


static void hmac_chba_rx_chan_switch_req(hmac_chba_mgmt_info_stru *chba_mgmt_info,
    hmac_chba_chan_switch_notify_stru *chan_switch_ntf, uint8_t *peer_addr)
{
    hmac_info_report_type_enum report_type;
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();
    hmac_chba_chan_switch_notify_stru chan_switch_resp = { 0 };
    uint32_t ret;

    if (oal_any_null_ptr2(chan_switch_ntf, peer_addr)) {
        return;
    }

    oam_warning_log3(0, OAM_SF_CHBA, "hmac_chba_rx_chan_switch_req::rx chan switch req. type %d, chan %d, bw %d",
        chan_switch_ntf->notify_type, chan_switch_ntf->chan_number, chan_switch_ntf->bandwidth);
    /* 非岛主，不处理切换请求 */
    if ((self_conn_info->role != CHBA_MASTER) && (self_conn_info->role != CHBA_ISLAND_OWNER)) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_rx_chan_switch_req::not owner, rcv switch req.");
        return;
    }

    /* 发送方不属于本岛内设备，不处理切换请求 */
    if (hmac_chba_island_device_check(peer_addr) != TRUE) {
        oam_warning_log0(0, OAM_SF_CHBA,
            "hmac_chba_rx_chan_switch_req::rx chan switch req. peer not belong island");
        return;
    }

    /* 填写信道切换action帧信息结构体，CSN resp只有notify_type和status_code字段有意义 */
    chan_switch_resp.notify_type = MAC_CHBA_CHAN_SWITCH_RESP;
    chan_switch_resp.status_code = MAC_CHBA_CHAN_SWITCH_ACCEPT;
    if (hmac_chba_check_is_vap_in_assoc() != FALSE) {
        chan_switch_resp.status_code = MAC_CHBA_CHAN_SWITCH_REJECT;
    }

    /* 回复CSN resp帧 */
    ret = hmac_chba_send_chan_switch_notify_action(peer_addr, &chan_switch_resp);
    if (ret != OAL_SUCC) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_rx_chan_switch_req::send CSN resp fail");
    }

    /* 拒绝的话不需要通知上层 */
    if (chan_switch_resp.status_code == MAC_CHBA_CHAN_SWITCH_REJECT) {
        return;
    }
    /* 通知上层下发岛主切换到最优信道指示 */
    report_type = HMAC_CHBA_GET_BEST_CHANNEL;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_cfg80211_chba_report((uint8_t *)(&report_type), sizeof(report_type));
#endif
}
static oal_bool_enum_uint8 hmac_chba_rx_csn_coex_chan_is_dbac(
    hmac_chba_vap_stru *chba_vap_info, hmac_chba_chan_switch_notify_stru *chan_switch_ntf)
{
    hmac_vap_stru *hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(chba_vap_info->mac_vap_id);
    mac_channel_stru mac_channel = { 0 };

    if (hmac_vap == NULL) {
        return OAL_FALSE;
    }

    mac_channel.uc_chan_number = chan_switch_ntf->chan_number;
    mac_channel.en_band = mac_get_band_by_channel_num(mac_channel.uc_chan_number);
    mac_channel.en_bandwidth = chan_switch_ntf->bandwidth;
    // 如果收到岛内设备的CSN indication帧，先回复confirm，如果满足DBAC通知上层处理
    if (hmac_chba_coex_channel_is_dbac(&hmac_vap->st_vap_base_info, &mac_channel) == OAL_TRUE) {
        oam_warning_log0(0, OAM_SF_ROAM, "{hmac_chba_rx_csn_indication_coex_chan_check::coex DBAC, do not switch!}");
        hmac_chba_coex_switch_chan_dbac_rpt(&mac_channel, HMAC_CHBA_COEX_CHAN_SWITCH_CHBA_CHAN_RPT);
        return OAL_TRUE;
    }
    return OAL_FALSE;
}


static void hmac_chba_rx_chan_switch_indication(hmac_vap_stru *hmac_vap,
    hmac_chba_mgmt_info_stru *chba_mgmt_info, hmac_chba_chan_switch_notify_stru *chan_switch_ntf, uint8_t *peer_addr)
{
    uint32_t ret;
    hmac_chba_chan_switch_info_stru *chan_switch_info = NULL;
    hmac_chba_chan_switch_notify_stru chan_switch_confirm = { 0 };
    hmac_chba_vap_stru *chba_vap_info = hmac_get_chba_vap();

    if (oal_any_null_ptr3(chba_mgmt_info, chan_switch_ntf, peer_addr)) {
        return;
    }

    chan_switch_info = &chba_mgmt_info->chan_switch_info;
    oam_warning_log4(0, OAM_SF_CHBA, "hmac_chba_rx_chan_switch_indication::peer_addr[%02X:%02X:XX:XX:%02X:%02X]",
        peer_addr[MAC_ADDR_0], peer_addr[MAC_ADDR_1], peer_addr[MAC_ADDR_4], peer_addr[MAC_ADDR_5]);

    /* 发送方不属于本岛内设备，不处理切换指示 */
    if (hmac_chba_island_device_check(peer_addr) != TRUE) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_rx_chan_switch_indication::switch ind not from island device.");
        return;
    }

    /* 注销req Timer定时器 */
    if (chan_switch_info->req_timer.en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(&(chan_switch_info->req_timer));
    }

    /* 信道号和带宽参数组合不支持 */
    if (mac_regdomain_channel_is_support_bw(chan_switch_ntf->bandwidth, chan_switch_ntf->chan_number) == OAL_FALSE) {
        oam_error_log2(0, OAM_SF_CHBA, "hmac_chba_rx_chan_switch_indication::channel number=[%d], bw=[%d] invalid",
            chan_switch_ntf->chan_number, chan_switch_ntf->bandwidth);
        return;
    }

    /* 已经在新信道上了，不回confirm */
    if (chba_vap_info->work_channel.uc_chan_number ==  chan_switch_ntf->chan_number &&
        chba_vap_info->work_channel.en_bandwidth == chan_switch_ntf->bandwidth) {
        oam_warning_log2(0, OAM_SF_CHBA, "hmac_chba_rx_chan_switch_indication::already on new \
            channel[%d] bw=[%d], not confirm!", chan_switch_ntf->chan_number, chan_switch_ntf->bandwidth);
        return;
    }
    /* 连续请求超时次数清0 */
    chan_switch_info->req_timeout_cnt = 0;

    /* 填写信道切换action帧信息结构体，切换请求只有notify_type字段有意义 */
    chan_switch_confirm.notify_type = MAC_CHBA_CHAN_SWITCH_CONFIRM;

    /* 回复信道切换confirm帧 */
    ret = hmac_chba_send_chan_switch_notify_action(peer_addr, &chan_switch_confirm);
    if (ret != OAL_SUCC) {
        oam_warning_log0(0, OAM_SF_CHBA,
            "hmac_chba_rx_chan_switch_indication::hmac_chba_send_chan_switch_notify_action fail");
    }
    if (hmac_chba_rx_csn_coex_chan_is_dbac(chba_vap_info, chan_switch_ntf) == OAL_TRUE) {
        return;
    }

    /* 抛事件给DMAC，配置自己的信道切换 */
    hmac_chba_chan_work_channel_switch(hmac_vap, chan_switch_ntf->chan_number, chan_switch_ntf->bandwidth,
        MAC_CHBA_CHAN_SWITCH_IMMEDIATE);
}


static void hmac_chba_rx_chan_switch_resp(hmac_chba_mgmt_info_stru *chba_mgmt_info,
    hmac_chba_chan_switch_notify_stru *chan_switch_ntf, uint8_t *peer_addr)
{
    hmac_chba_chan_switch_info_stru *chan_switch_info = NULL;
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();

    if (oal_any_null_ptr3(chba_mgmt_info, chan_switch_ntf, peer_addr)) {
        return;
    }

    chan_switch_info = &chba_mgmt_info->chan_switch_info;
    /* 检查发送方是否是本设备所属的岛主，不是则不处理 */
    oam_warning_log4(0, OAM_SF_CHBA, "hmac_chba_rx_chan_switch_resp::peer_addr[%02X:%02X:XX:XX:%02X:%02X]",
        peer_addr[MAC_ADDR_0], peer_addr[MAC_ADDR_1], peer_addr[MAC_ADDR_4], peer_addr[MAC_ADDR_5]);
    if (oal_compare_mac_addr(peer_addr, self_conn_info->island_owner_addr) != 0) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_rx_chan_switch_resp::switch ind not from island owner.");
        return;
    }

    /* 注销req Timer定时器 */
    if (chan_switch_info->req_timer.en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(&(chan_switch_info->req_timer));
    }

    /* 连续请求超时次数清0 */
    chan_switch_info->req_timeout_cnt = 0;
}
static uint32_t hmac_chba_check_lost_device_confirm(hmac_chba_chan_switch_info_stru *chan_switch_info,
    uint8_t *peer_addr)
{
    uint32_t idx;
    uint8_t *mac_addr = NULL;
    for (idx = 0; idx < chan_switch_info->csa_lost_device_num; idx++) {
        mac_addr = chan_switch_info->csa_lost_device[idx].mac_addr;
        /* 将对应设备的ack标记置为TRUE */
        if (oal_compare_mac_addr(mac_addr, peer_addr) == 0) {
            chan_switch_info->csa_lost_device[idx].csa_ack = TRUE;
        }
    }

    /* 判断是否集齐ack */
    for (idx = 0; idx < chan_switch_info->csa_lost_device_num; idx++) {
        if (chan_switch_info->csa_lost_device[idx].csa_ack != TRUE) {
            return OAL_FAIL;
        }
    }

    return OAL_SUCC;
}

static void hmac_chba_rx_chan_switch_confirm(hmac_vap_stru *hmac_vap,
    hmac_chba_mgmt_info_stru *chba_mgmt_info, hmac_chba_chan_switch_notify_stru *chan_switch_ntf, uint8_t *peer_addr)
{
    uint32_t idx, ret;
    uint8_t device_cnt;
    uint8_t *mac_addr = NULL;
    mac_chba_per_device_info_stru *device_list = NULL;
    hmac_chba_chan_switch_info_stru *chan_switch_info = NULL;
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();

    if (oal_any_null_ptr3(chba_mgmt_info, chan_switch_ntf, peer_addr)) {
        return;
    }

    chan_switch_info = &chba_mgmt_info->chan_switch_info;
    /* 处于补救流程，判断是否收集齐ack */
    if (chan_switch_info->csa_lost_device_num > 0) {
        ret = hmac_chba_check_lost_device_confirm(chan_switch_info, peer_addr);
        if (ret == OAL_SUCC) {
            /* 标记补救流程结束 */
            hmac_chba_clear_csa_lost_device_flag(chan_switch_info);

            /* 通知dmac补救完成 */
            hmac_chba_csa_lost_device_notify(&(hmac_vap->st_vap_base_info), chan_switch_info);
        }
        return;
    }

    /* 不处于岛信道切换流程中，不处理confirm */
    if (chan_switch_info->in_island_chan_switch == FALSE) {
        return;
    }

    device_list = self_conn_info->island_device_list;
    device_cnt = self_conn_info->island_device_cnt;
    for (idx = 0; idx < device_cnt; idx++) {
        mac_addr = device_list[idx].mac_addr;
        /* 将对应设备的ack标记置为TRUE */
        if (oal_compare_mac_addr(mac_addr, peer_addr) == 0) {
            device_list[idx].chan_switch_ack = TRUE;
        }
    }

    /* 如果收齐了岛内设备的confirm，立即切换信道 */
    ret = hmac_chba_check_all_device_confirm(chan_switch_info, device_cnt, device_list);
    if (ret == OAL_SUCC) {
        /* 抛事件给DMAC，配置自己的信道切换 */
        hmac_chba_chan_work_channel_switch(hmac_vap, chan_switch_info->last_target_channel,
            chan_switch_info->last_target_channel_bw, MAC_CHBA_CHAN_SWITCH_IMMEDIATE);

        /* 标记岛信道切换流程结束 */
        chan_switch_info->in_island_chan_switch = FALSE;

        /* 注销ack Timer定时器 */
        if (chan_switch_info->ack_timer.en_is_registerd) {
            frw_timer_immediate_destroy_timer_m(&(chan_switch_info->ack_timer));
        }
        if (chan_switch_info->chan_switch_timer.en_is_registerd) {
            frw_timer_immediate_destroy_timer_m(&(chan_switch_info->chan_switch_timer));
        }
    }
}


void hmac_chba_rx_chan_switch_notify_action(hmac_vap_stru *hmac_vap, uint8_t *sa_addr, uint8_t sa_len, uint8_t *payload,
    uint16_t payload_len)
{
    hmac_chba_chan_switch_notify_stru chan_switch_ntf = { 0 };
    hmac_chba_mgmt_info_stru *chba_mgmt_info = hmac_chba_get_mgmt_info();

    if (payload_len < BYTE_OFFSET_2) {
        oam_error_log1(0, OAM_SF_CHBA, "hmac_chba_rx_chan_switch_notify_action::payload_len invalid[%d].", payload_len);
        return;
    }
    if ((*payload) != MAC_CHBA_ATTR_CHAN_SWITCH) {
        oam_warning_log1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CHBA,
            "hmac_chba_rx_chan_switch_notify_action::error attribute type[%d].", *payload);
        return;
    }

    if (MAC_CHBA_ATTR_HDR_LEN + payload[MAC_CHBA_ATTR_ID_LEN] < MAC_CHBA_CHAN_SWITCH_ATTR_LEN) {
        oam_error_log1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CHBA,
            "hmac_chba_rx_chan_switch_notify_action::csa attr len[%d] invalid.", payload[MAC_CHBA_ATTR_ID_LEN]);
        return;
    }
    payload = payload + MAC_CHBA_ATTR_HDR_LEN;

    chan_switch_ntf.notify_type = *payload;
    payload++;
    chan_switch_ntf.chan_number = *payload;
    payload++;
    chan_switch_ntf.bandwidth = *payload;
    payload++;
    chan_switch_ntf.switch_slot = *payload;
    payload++;
    chan_switch_ntf.status_code = *payload;

    oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_rx_chan_switch_notify_action::type %d", chan_switch_ntf.notify_type);

    /* 根据类型区分处理请求和指示 */
    if (chan_switch_ntf.notify_type == MAC_CHBA_CHAN_SWITCH_REQ) {
        hmac_chba_rx_chan_switch_req(chba_mgmt_info, &chan_switch_ntf, sa_addr);
    } else if (chan_switch_ntf.notify_type == MAC_CHBA_CHAN_SWITCH_RESP) {
        hmac_chba_rx_chan_switch_resp(chba_mgmt_info, &chan_switch_ntf, sa_addr);
    } else if (chan_switch_ntf.notify_type == MAC_CHBA_CHAN_SWITCH_INDICATION) {
        hmac_chba_rx_chan_switch_indication(hmac_vap, chba_mgmt_info, &chan_switch_ntf, sa_addr);
    } else if (chan_switch_ntf.notify_type == MAC_CHBA_CHAN_SWITCH_CONFIRM) {
        hmac_chba_rx_chan_switch_confirm(hmac_vap, chba_mgmt_info, &chan_switch_ntf, sa_addr);
    } else {
        oam_warning_log0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CHBA,
            "hmac_chba_rx_chan_switch_notify_action::error chan switch type.");
    }
}

void hmac_chba_adjust_channel_proc(hmac_vap_stru *hmac_vap, mac_chba_adjust_chan_info *chan_info)
{
    mac_channel_stru target_channel;
    uint8_t pedding_data = 0;
    hmac_chba_mgmt_info_stru *mgmt_info = hmac_chba_get_mgmt_info();
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();

    if ((hmac_chba_vap_start_check(hmac_vap) == OAL_FALSE) || (chan_info == NULL)) {
        return;
    }

    /* 未建链不处理 */
    if (hmac_vap->st_vap_base_info.us_user_nums == 0) {
        return;
    }

    /* 存在要补救的设备不处理 */
    if (mgmt_info->chan_switch_info.csa_lost_device_num > 0) {
        return;
    }
    /* 进信道切换流程前停掉扫描 */
    hmac_config_scan_abort(&hmac_vap->st_vap_base_info, sizeof(uint32_t), &pedding_data);

    oam_warning_log3(0, OAM_SF_CHBA,
        "hmac_chba_adjust_channel_proc::target channel channel number %d, bwmode %d, switch type[%d].",
        chan_info->chan_number, chan_info->bandwidth, chan_info->switch_type);
    target_channel.uc_chan_number = chan_info->chan_number;
    target_channel.en_bandwidth = chan_info->bandwidth;
    target_channel.en_band = mac_get_band_by_channel_num(target_channel.uc_chan_number);
    switch (chan_info->switch_type) {
        case MAC_CHBA_CHAN_SWITCH_FOR_CONNECT:
            hmac_chba_start_switch_chan_for_connect_perf(hmac_vap, mgmt_info, &target_channel,
                MAC_CHBA_CHAN_SWITCH_IMMEDIATE, MAC_CHBA_CHAN_SWITCH_FOR_CONNECT);
            break;

        case MAC_CHBA_CHAN_SWITCH_FOR_PERFORMANCE:
            if (self_conn_info->role == CHBA_MASTER || self_conn_info->role == CHBA_ISLAND_OWNER) {
                /* 启动信道切换流程 */
                hmac_chba_start_switch_chan_for_connect_perf(hmac_vap, mgmt_info, &target_channel,
                    MAC_CHBA_CHAN_SWITCH_IMMEDIATE, MAC_CHBA_CHAN_SWITCH_FOR_PERFORMANCE);
            } else if (self_conn_info->role == CHBA_OTHER_DEVICE) {
                hmac_chba_send_chan_switch_req(mgmt_info);
            }
            break;

        case MAC_CHBA_CHAN_SWITCH_FOR_AVOID_DBAC:
            hmac_chba_start_switch_chan_for_avoid_dbac(hmac_vap, &target_channel,
                MAC_CHBA_CHAN_SWITCH_IMMEDIATE);
            break;

        default:
            break;
    }
    return;
}

/* 信道打桩命令处理 */

void hmac_chba_stub_chan_switch_report_proc(hmac_vap_stru *hmac_vap, uint8_t *recv_msg)
{
    mac_chba_stub_chan_switch_rpt *info_report = NULL;
    if (recv_msg == NULL) {
        return;
    }

    info_report = (mac_chba_stub_chan_switch_rpt *)recv_msg;
    oam_warning_log2(0, OAM_SF_CHBA, "hmac_chba_stub_chan_switch_report_proc::StubChanSwitch. target chan %d %d",
        info_report->target_channel.uc_chan_number, info_report->target_channel.en_bandwidth);

    /* 抛事件给DMAC，配置自己的信道切换 */
    hmac_chba_chan_work_channel_switch(hmac_vap, info_report->target_channel.uc_chan_number,
        info_report->target_channel.en_bandwidth, MAC_CHBA_CHAN_SWITCH_IMMEDIATE);
}

uint32_t hmac_chba_test_chan_switch_cmd(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params)
{
    hmac_chba_vap_stru *chba_vap_info = NULL;
    hmac_vap_stru *hmac_vap = NULL;
    mac_set_chan_switch_test_params *test_params = NULL;
    mac_channel_stru mac_channel;
    uint32_t ret;
    test_params = (mac_set_chan_switch_test_params *)params;

    oam_warning_log3(0, 0, "hmac_chba_test_chan_switch_cmd::chanswitch cmd, intf slot %d, switch chan %d %d",
        test_params->intf_rpt_slot, test_params->switch_chan_num, test_params->switch_chan_bw);

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (hmac_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    chba_vap_info = hmac_vap->hmac_chba_vap_info;
    if (chba_vap_info == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (test_params->switch_chan_num != 0xFF) {
        mac_channel.uc_chan_number = test_params->switch_chan_num;
        mac_channel.en_bandwidth = test_params->switch_chan_bw;
        mac_channel.en_band = mac_get_band_by_channel_num(mac_channel.uc_chan_number);
        /* 填写channel_idx */
        ret = mac_get_channel_idx_from_num(mac_channel.en_band, mac_channel.uc_chan_number, OAL_FALSE,
            &(mac_channel.uc_chan_idx));
        if (ret != OAL_SUCC) {
            return ret;
        }

        hmac_chba_stub_channel_switch_report(hmac_vap, &mac_channel);
    }

    return OAL_SUCC;
}
#endif
