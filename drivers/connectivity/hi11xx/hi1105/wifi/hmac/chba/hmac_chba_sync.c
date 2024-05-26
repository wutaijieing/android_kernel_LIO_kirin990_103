

/* 1 头文件包含 */
#include "hmac_chba_sync.h"
#include "hmac_chba_frame.h"
#include "hmac_chba_function.h"
#include "hmac_chba_interface.h"
#include "hmac_mgmt_sta.h"
#include "securec.h"
#include "hmac_chba_chan_switch.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_SYNC_C

#ifdef _PRE_WLAN_CHBA_MGMT
/* 2 全局变量定义 */
/* CHBA同步信息 */
hmac_chba_sync_info g_chba_sync_info;

/* 3 函数声明 */
uint8_t hmac_chba_sync_check_master_change(hmac_chba_sync_info *sync_info, master_info *new_master,
    uint8_t *payload, uint16_t payload_len, uint8_t *sa_addr);
void hmac_chba_sync_update_master_info(hmac_chba_sync_info *sync_info, master_info *new_master, uint8_t update_flag);
int32_t hmac_chba_sync_become_island_owner_handler(hmac_chba_sync_info *sync_info);
int32_t hmac_chba_sync_become_other_dev_handler(hmac_chba_sync_info *sync_info);
void hmac_chba_sync_master_election_compare(ranking_priority *peer_rp, uint8_t *peer_addr,
    uint8_t work_channel);
void hmac_chba_sync_master_bcn_process(sync_info_flags *sync_flags,
    uint8_t *payload, uint16_t payload_len);
int32_t hmac_chba_try_once_sync_request(hmac_chba_sync_info *sync_info, sync_info_flags *sync_flags,
    sync_request *sync_req_ctrl);
void hmac_chba_domain_merge_sync_handler(hmac_vap_stru *hmac_vap, hmac_chba_sync_info *sync_info);
uint32_t hmac_chba_sync_listen_timeout(void *p);
uint32_t hmac_chba_sync_wait_timeout(void *p);
/* 4 函数实现 */
uint8_t hmac_chba_get_sync_request_flag(sync_info_flags *sync_flags)
{
    return sync_flags->sync_request_flag;
}
void hmac_chba_set_sync_request_flag(sync_info_flags *sync_flags, uint8_t new_flag)
{
    uint32_t ret;
    mac_vap_stru *mac_vap = NULL;
    uint8_t  old_flag = hmac_chba_get_sync_request_flag(sync_flags);
    hmac_chba_vap_stru *hmac_chba_vap = hmac_get_chba_vap();
    /* lzhqi不应重复下发同一个状态,后面fix */
    if (old_flag == new_flag) {
        oam_warning_log2(0, OAM_SF_CFG, "{hmac_chba_set_sync_request_flag::old[%d]==new[%d].",
            old_flag, new_flag);
    }
    sync_flags->sync_request_flag = new_flag;
    mac_vap = mac_res_get_mac_vap(hmac_chba_vap->mac_vap_id);
    ret = hmac_config_send_event(mac_vap, WLAN_CFGID_CHBA_SYNC_REQUEST_FLAG, sizeof(uint8_t), &new_flag);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_warning_log1(0, OAM_SF_CFG, "{hmac_chba_set_sync_request_flag::hmac_config_send_event fail[%d].", ret);
    }
    oam_warning_log2(0, OAM_SF_CFG, "{hmac_chba_set_sync_request_flag::old[%d]new[%d].",
        old_flag, new_flag);
}
uint8_t hmac_chba_get_domain_merge_flag(hmac_chba_domain_merge_info *domain_merge)
{
    return domain_merge->domain_merge_flag;
}
void hmac_chba_set_sync_domain_merge_flag(hmac_chba_domain_merge_info *domain_merge, uint8_t new_flag)
{
    uint32_t ret;
    mac_vap_stru *mac_vap = NULL;
    uint8_t  old_flag = hmac_chba_get_domain_merge_flag(domain_merge);
    hmac_chba_vap_stru *hmac_chba_vap = hmac_get_chba_vap();
    if (old_flag == new_flag) {
        return;
    }
    domain_merge->domain_merge_flag = new_flag;
    mac_vap = mac_res_get_mac_vap(hmac_chba_vap->mac_vap_id);
    ret = hmac_config_send_event(mac_vap, WLAN_CFGID_CHBA_SYNC_DOMAIN_MERGE_FLAG, sizeof(uint8_t), &new_flag);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_warning_log1(0, OAM_SF_CFG, "{hmac_chba_set_sync_domain_merge_flag::hmac_config_send_event fail[%d].", ret);
    }
}

void hmac_chba_sync_module_init(hmac_chba_device_para_stru *device_info)
{
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();

    if (device_info == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_module_init::input pointer is null.");
        return;
    }

    /* 注销定时器 & 释放资源 */
    if (sync_info->sync_waiting_timer.en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(&sync_info->sync_waiting_timer);
    }
    if (sync_info->domain_merge_waiting_timer.en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(&sync_info->domain_merge_waiting_timer);
    }
    if (sync_info->domain_merge_next_timer.en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(&sync_info->domain_merge_next_timer);
    }
    if (sync_info->sync_request_timer.en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(&sync_info->sync_request_timer);
    }

    /* 清空结构体 */
    memset_s(sync_info, sizeof(hmac_chba_sync_info), 0, sizeof(hmac_chba_sync_info));

    /* 初始化相关信息 */
    sync_info->sync_state = CHBA_INIT;
    hmac_chba_sync_set_rp_dev_type(device_info->device_type);
    hmac_chba_sync_set_rp_capability(device_info->hardwire_cap);
    hmac_chba_sync_set_rp_power(device_info->remain_power);
    hmac_chba_sync_set_rp_version(device_info->config_para.chba_version);

    oam_warning_log2(0, OAM_SF_CHBA, "hmac_chba_sync_module_init::Init CHBA sync info. dev type[%d], remain_power[%d].",
        sync_info->own_rp_info.device_level.dl.device_type,
        sync_info->own_rp_info.device_level.dl.remain_power);
}


void hmac_chba_sync_module_deinit(void)
{
    hmac_chba_device_para_stru *device_info = hmac_chba_get_device_info();
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();
    uint8_t sync_state;

    if (device_info == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_module_deinit::input pointer is null.");
        return;
    }

    /* 注销定时器 & 释放资源 */
    if (sync_info->sync_waiting_timer.en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(&sync_info->sync_waiting_timer);
    }
    if (sync_info->domain_merge_waiting_timer.en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(&sync_info->domain_merge_waiting_timer);
    }
    if (sync_info->domain_merge_next_timer.en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(&sync_info->domain_merge_next_timer);
    }
    if (sync_info->sync_request_timer.en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(&sync_info->sync_request_timer);
    }

    sync_state = sync_info->sync_state;

    /* 清空结构体 */
    memset_s(sync_info, sizeof(hmac_chba_sync_info), 0, sizeof(hmac_chba_sync_info));

    /* 初始化相关信息 */
    sync_info->sync_state = (sync_state >= CHBA_INIT) ? CHBA_INIT : sync_state;
    hmac_chba_sync_set_rp_dev_type(device_info->device_type);
    hmac_chba_sync_set_rp_capability(device_info->hardwire_cap);
    hmac_chba_sync_set_rp_power(device_info->remain_power);
    hmac_chba_sync_set_rp_version(device_info->config_para.chba_version);
}


void hmac_chba_clear_sync_info(hmac_chba_sync_info *sync_info)
{
    if (sync_info == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_clear_sync_info::input pointer is null.");
        return;
    }

    memset_s(&sync_info->sync_req_ctrl, sizeof(sync_request), 0, sizeof(sync_request));
    memset_s(sync_info->try_bssid, OAL_MAC_ADDR_LEN, 0, OAL_MAC_ADDR_LEN);

    /* 注销定时器 */
    if (sync_info->sync_waiting_timer.en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(&sync_info->sync_waiting_timer);
    }
    if (sync_info->sync_request_timer.en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(&sync_info->sync_request_timer);
    }
}
static void hmac_chba_sync_own_rp_info(hmac_chba_sync_info *sync_info)
{
    hmac_chba_vap_stru *chba_vap_info = hmac_get_chba_vap();
    mac_vap_stru *mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(chba_vap_info->mac_vap_id);

    hmac_config_send_event(mac_vap, WLAN_CFGID_CHBA_OWN_RP_SYNC, sizeof(ranking_priority),
        (uint8_t *)&sync_info->own_rp_info);
}
static void hmac_chba_h2d_sync_master_info(hmac_chba_sync_info *sync_info)
{
    hmac_chba_vap_stru *chba_vap_info = hmac_get_chba_vap();
    mac_vap_stru *mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(chba_vap_info->mac_vap_id);
    chba_params_config_stru chba_params_sync = { 0 };

    chba_params_sync.info_bitmap = CHBA_MASTER_INFO;
    chba_params_sync.master_info = sync_info->cur_master_info;
    hmac_chba_params_sync(mac_vap, &chba_params_sync);
}

void hmac_chba_sync_update_own_rp(hmac_vap_stru *hmac_vap)
{
    uint8_t need_post_event = OAL_TRUE;
    int32_t err;
    hmac_chba_device_para_stru *device_info = hmac_chba_get_device_info();
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();
    ranking_priority old_rp = sync_info->own_rp_info;
    ranking_priority old_master_rp = sync_info->cur_master_info.master_rp;

    if (hmac_vap == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "{hmac_chba_sync_update_own_rp:hmac_vap is NULL!}");
        return;
    }

    hmac_chba_sync_set_rp_power(device_info->remain_power);
    hmac_chba_sync_set_rp_link_cnt(hmac_vap->st_vap_base_info.us_user_nums);
    if (oal_memcmp(&old_rp, &sync_info->own_rp_info, sizeof(ranking_priority) != 0)) {
        need_post_event = OAL_TRUE;
        oam_warning_log4(0, OAM_SF_CHBA, "Update OwnRp, remainPower[%d], link_cnt[%d] role[%d].device level[%d]",
            sync_info->own_rp_info.device_level.dl.remain_power, sync_info->own_rp_info.link_cnt, hmac_chba_get_role(),
            (uint8_t)sync_info->own_rp_info.device_level.value);
    }
    /* 如果是Master，则同步更新到curMaster中 */
    if (hmac_chba_get_role() == CHBA_MASTER) {
        err = memcpy_s(&(sync_info->cur_master_info.master_rp), sizeof(ranking_priority),
            &(sync_info->own_rp_info), sizeof(ranking_priority));
        if (err != EOK) {
            oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_sync_update_own_rp::memcpy failed, err[%d].", err);
        }
        if (oal_memcmp(&old_master_rp, &sync_info->cur_master_info.master_rp, sizeof(ranking_priority)) != 0) {
            oam_warning_log4(0, OAM_SF_CHBA, "Update masterrp, remainPower[%d],link_cnt[%d]role[%d]device level[%d].",
                sync_info->cur_master_info.master_rp.device_level.dl.remain_power,
                sync_info->cur_master_info.master_rp.link_cnt, hmac_chba_get_role(),
                (uint8_t)sync_info->cur_master_info.master_rp.device_level.value);
            need_post_event = OAL_TRUE;
        }
    }
    if (need_post_event) {
        hmac_chba_sync_own_rp_info(sync_info);
    }
}


void hmac_chba_vap_start_sync_proc(mac_chba_vap_start_rpt *info_report)
{
    uint8_t *own_addr = NULL;

    if (info_report == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_vap_start_sync_proc::input pointer is null.");
        return;
    }

    /* 初始化同步状态为NON_SYNC，角色为OTHER DEVICE */
    hmac_chba_set_role(CHBA_OTHER_DEVICE);
    hmac_chba_set_sync_state(CHBA_NON_SYNC);

    oam_warning_log3(0, OAM_SF_CHBA, "hmac_chba_vap_start_sync_proc::start vap, mac addr XX:XX:XX:%02X:%02X:%02X",
        info_report->mac_addr[3], info_report->mac_addr[4], info_report->mac_addr[5]);

    /* 生成域BSSID */
    own_addr = hmac_chba_get_own_mac_addr();
    if (own_addr == NULL) {
        return;
    }
    hmac_chba_generate_domain_bssid(hmac_chba_sync_get_domain_bssid(), own_addr, OAL_MAC_ADDR_LEN);

    /* Master设备信息初始化为自身 */
    hmac_chba_cur_master_info_init(info_report->mac_addr, &info_report->work_channel);
}


static void hmac_chba_add_peer_into_request_list(uint8_t chan_number, uint8_t *peer_addr, uint8_t *master_addr,
    uint8_t island_peer_flag)
{
    uint8_t next_idx, end_idx;
    sync_request *sync_req_ctrl = hmac_chba_get_sync_req_ctrl();
    sync_peer_info *request_list = hmac_chba_get_sync_req_list();

    if (peer_addr == NULL || master_addr == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_add_peer_into_request_list::input pointer is null.");
        return;
    }
    if (chan_number == 0) {
        oam_warning_log0(0, 0, "hmac_chba_add_peer_into_request_list::invalid channel number.");
        return;
    }
    next_idx = sync_req_ctrl->next_request_idx;
    end_idx = sync_req_ctrl->end_idx;

    if (island_peer_flag == TRUE) {
        sync_req_ctrl->island_peer_flag = TRUE;
        oal_set_mac_addr(sync_req_ctrl->island_master_addr, master_addr);
    }

    /* RequestList为空的条件: nextRequestIdx和endIdx都为0 */
    /* RequestList为满的条件: nextRequestIdx与endIdx相等，且不为0 */
    request_list[end_idx].chan_num = chan_number;
    oal_set_mac_addr(request_list[end_idx].peer_addr, peer_addr);
    oal_set_mac_addr(request_list[end_idx].master_addr, master_addr);
    /* 如果RequestList为满，则将nextRequestIdx往后移一位 */
    if (next_idx == end_idx && next_idx != 0) {
        sync_req_ctrl->next_request_idx = (next_idx + 1) % MAC_CHBA_REQUEST_LIST_NUM;
    }
    sync_req_ctrl->end_idx = (end_idx + 1) % MAC_CHBA_REQUEST_LIST_NUM;

    oam_warning_log4(0, OAM_SF_CHBA,
        "hmac_chba_add_peer_into_request_list::add peer(XX:XX:XX:%02X:%02X:%02X, work chan[%d]) into sync req list.",
        peer_addr[3], peer_addr[4], peer_addr[5], chan_number);
}

void hmac_chba_ask_for_sync(uint8_t chan_number, uint8_t *peer_addr, uint8_t *master_addr)
{
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();

    if (peer_addr == NULL || master_addr == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_ask_for_sync:input pointer is null");
        return;
    }

    /* 未初始化不处理 */
    if (hmac_chba_get_module_state() == MAC_CHBA_MODULE_STATE_UNINIT) {
        return;
    }

    /* 将requestPeer添加到syncRequestList中 */
    hmac_chba_add_peer_into_request_list(chan_number, peer_addr, master_addr, TRUE);
    /* 启动主动同步请求 */
    hmac_chba_start_sync_request(sync_info);
}

static uint32_t hmac_chba_fill_sync_request_list(hmac_vap_stru *hmac_vap, hmac_chba_sync_info *sync_info,
    sync_request *sync_req_ctrl)
{
    hmac_chba_vap_stru *chba_vap_info = hmac_get_chba_vap();
    uint8_t user_cnt;
    uint8_t idx;
    uint8_t *own_addr = hmac_chba_get_own_mac_addr();
    oal_dlist_head_stru *entry = NULL;
    oal_dlist_head_stru *dlist_tmp = NULL;
    mac_user_stru *mac_user = NULL;
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();
    mac_chba_per_device_info_stru *device_list = self_conn_info->island_device_list;

    if (oal_any_null_ptr4(sync_info, sync_req_ctrl, own_addr, hmac_vap)) {
        return OAL_FAIL;
    }

    /* 如果当前deviceList不为空，则从deviceList中选取与自己有链路，且同步上的设备放入List */
    user_cnt = 0;
    if (self_conn_info->island_device_cnt > 0) {
        for (idx = 0; idx < self_conn_info->island_device_cnt; idx++) {
            /* 跳过本设备 */
            if (oal_compare_mac_addr(device_list[idx].mac_addr, own_addr) == 0) {
                continue;
            }
            if (device_list[idx].is_conn_flag == TRUE) {
                hmac_chba_add_peer_into_request_list(chba_vap_info->work_channel.uc_chan_number,
                    device_list[idx].mac_addr, sync_info->cur_master_info.master_addr, TRUE);
                user_cnt++;
            }
        }
        if (user_cnt == 0) {
            oam_warning_log0(0, OAM_SF_CHBA,
                "hmac_chba_fill_sync_request_list::non sync, connected peers in island, should sync on social channel");
            return OAL_FAIL;
        }
        oam_warning_log1(0, 0, "hmac_chba_fill_sync_request_list::add [%d] island devices into req list.", user_cnt);
        return OAL_SUCC;
    }

    /* 如果deviceList为空，但是有链路，一般发生刚建链但未同步上，则将有链路的设备添加到List */
    /* 此时islandPeerFlag如果为空，则打印warning */
    if (hmac_vap->st_vap_base_info.us_user_nums > 0 && sync_req_ctrl->island_peer_flag == TRUE) {
        oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_fill_sync_request_list::add [%d] users into req list.",
            hmac_vap->st_vap_base_info.us_user_nums);
        oal_dlist_search_for_each_safe(entry, dlist_tmp, &(hmac_vap->st_vap_base_info.st_mac_user_list_head))
        {
            mac_user = oal_dlist_get_entry(entry, mac_user_stru, st_user_dlist);
            if (mac_user == NULL) {
                continue;
            }
            hmac_chba_add_peer_into_request_list(chba_vap_info->work_channel.uc_chan_number,
                mac_user->auc_user_mac_addr, sync_req_ctrl->island_master_addr, TRUE);
        }
        return OAL_SUCC;
    }
    return OAL_FAIL;
}


int32_t hmac_chba_start_sync_request(hmac_chba_sync_info *sync_info)
{
    sync_info_flags *sync_flags = hmac_chba_get_sync_flags();
    sync_request *sync_req_ctrl = hmac_chba_get_sync_req_ctrl();
    int32_t err;

    if (sync_info == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_start_sync_request::input pointer is null.");
        return OAL_FAIL;
    }

    /* 如果已经在主动请求过程中，则不再重复处理 */
    if (hmac_chba_get_sync_request_flag(sync_flags) == TRUE) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_start_sync_request::already in request sync processing, return.");
        return OAL_SUCC;
    }
    hmac_chba_set_sync_request_flag(sync_flags, OAL_TRUE);
    /* 尝试一次主动同步请求 */
    err = hmac_chba_try_once_sync_request(sync_info, sync_flags, sync_req_ctrl);
    if (err != OAL_SUCC) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_start_sync_request::fail to try once sync request.");
    }
    return OAL_SUCC;
}


static void hmac_chba_sync_recovery_handler(sync_info_flags *sync_flags)
{
    hmac_chba_set_sync_request_flag(sync_flags, OAL_FALSE);
    oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_recovery_handler:stop chba try once sync requset!");
}


int32_t hmac_chba_try_once_sync_request(hmac_chba_sync_info *sync_info, sync_info_flags *sync_flags,
    sync_request *sync_req_ctrl)
{
    sync_peer_info *request_list = hmac_chba_get_sync_req_list();
    mac_chba_set_sync_request sync_info_param = { 0 };
    uint8_t next_idx, end_idx;
    uint8_t *frame_buf = NULL;
    uint16_t frame_len = 0;
    hmac_chba_vap_stru *chba_vap_info = hmac_get_chba_vap();
    hmac_vap_stru *hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(chba_vap_info->mac_vap_id);

    if (sync_info == NULL || sync_flags == NULL || sync_req_ctrl == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_try_once_sync_request::input pointer is null.");
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (hmac_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    next_idx = sync_req_ctrl->next_request_idx;
    end_idx = sync_req_ctrl->end_idx;

    /* 检查RequestList是否为空，为空的条件: nextRequestIdx和endIdx都为0 */
    if (next_idx == 0 && end_idx == 0) {
        if (hmac_chba_fill_sync_request_list(hmac_vap, sync_info, sync_req_ctrl) != OAL_SUCC) {
            hmac_chba_sync_recovery_handler(sync_flags);
            oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_try_once_sync_request:fail to try once sync!");
            return OAL_FAIL;
        }
    }

    /* 将sync_with_new_master_flag置为0 */
    sync_flags->sync_with_new_master_flag = FALSE;

    /* 启动256ms同步超时定时器 */
    hmac_chba_create_timer_ms(&sync_info->sync_waiting_timer, WIFI_MAC_CHBA_SYNC_WAITING_TIMEOUT,
        hmac_chba_sync_wait_timeout);

    /* 获取本次同步的域BSSID */
    hmac_chba_generate_domain_bssid(sync_info->try_bssid, request_list[next_idx].master_addr, OAL_MAC_ADDR_LEN);

    /* 取nextRequestIdx的设备，下发给Driver进行同步 */
    sync_info_param.sync_type = MAC_CHBA_SYNC_WITH_REQUEST;
    if (memcpy_s(&sync_info_param.request_peer, sizeof(sync_peer_info), &(request_list[next_idx]),
        sizeof(sync_peer_info)) != EOK) {
        return OAL_FAIL;
    }
    /* 封装Sync Request帧 */
    sync_info_param.mgmt_type = MAC_CHBA_SYNC_REQUEST;
    frame_buf = sync_info_param.mgmt_buf;
    /* 填写Action 头 */
    mac_set_chba_action_hdr(frame_buf, MAC_CHBA_SYNC_REQUEST);
    frame_len += MAC_CHBA_ACTION_HDR_LEN;
    /* 填写type */
    frame_buf[frame_len] = MAC_CHBA_REQUEST_ACTION;
    frame_len++;
    /* 封装主设备选举字段 */
    hmac_chba_set_master_election_attr(frame_buf + frame_len);
    frame_len += MAC_CHBA_MASTER_ELECTION_ATTR_LEN;
    sync_info_param.mgmt_buf_len = frame_len;

    oam_warning_log4(0, OAM_SF_CHBA,
        "hmac_chba_try_once_sync_request::send sync request(type:%d), try bssid 00:E0:FC:%02X:%02X:%02X.",
        sync_info_param.sync_type, sync_info_param.request_peer.master_addr[3],
        sync_info_param.request_peer.master_addr[4], sync_info_param.request_peer.master_addr[5]);
    hmac_chba_request_sync(&hmac_vap->st_vap_base_info, chba_vap_info, &sync_info_param);

    return OAL_SUCC;
}


static void hmac_chba_resync_with_same_peer(uint8_t *master_addr,
    hmac_chba_sync_info *sync_info, sync_peer_info *cur_peer)
{
    sync_peer_info req_peer = { 0 };
    uint8_t island_device_flag;
    int32_t err;

    if (sync_info == NULL || cur_peer == NULL) {
        return;
    }

    oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_resync_with_same_peer::master changed, re-ask for the same peer.");

    /* 注销等待定时器 */
    if (sync_info->sync_waiting_timer.en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(&sync_info->sync_waiting_timer);
    }

    /* 保存当前的requestPeer信息 */
    err = memcpy_s(&req_peer, sizeof(sync_peer_info), cur_peer, sizeof(sync_peer_info));
    if (err != EOK) {
        oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_resync_with_same_peer::memcpy failed, err[%d].", err);
        return;
    }
    /* 更新其中的Master地址 */
    oal_set_mac_addr(req_peer.master_addr, master_addr);

    /* 清空当前的主动同步请求信息 */
    hmac_chba_clear_sync_info(sync_info);
    /* 判断当前设备是否在岛内 */
    island_device_flag = hmac_chba_island_device_check(req_peer.peer_addr);
    /* 将当前设备添加到ReqList中 */
    hmac_chba_add_peer_into_request_list(req_peer.chan_num, req_peer.peer_addr, req_peer.master_addr,
        island_device_flag);

    /* 重新启动主动同步流程 */
    hmac_chba_start_sync_request(sync_info);
}


uint32_t hmac_chba_sync_wait_timeout(void *p)
{
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();
    sync_request *sync_req_ctrl = hmac_chba_get_sync_req_ctrl();
    sync_peer_info *request_list = hmac_chba_get_sync_req_list();
    frw_timeout_stru *p_timer = &sync_info->sync_waiting_timer;
    uint8_t next_idx, end_idx;
    int32_t err;

    /* 该定时器已经被注销 */
    if (p_timer->en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(p_timer);
    }

    /* 如果当前状态为已同步且trySyncFlag为0，则直接返回 */
    if ((hmac_chba_get_sync_state() > CHBA_NON_SYNC) &&
        (hmac_chba_get_sync_request_flag(&sync_info->sync_flags) == FALSE)) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_wait_timeout::already sync, return.");
        return OAL_SUCC;
    }

    /* 将next_request_idx右移，如果出现next_request_idx + 1 = end_idx的case，说明已经取完Request List,
        则将next_request_idx和end_idx都置为0 */
    next_idx = sync_req_ctrl->next_request_idx;
    end_idx = sync_req_ctrl->end_idx;
    next_idx = (next_idx + 1) % MAC_CHBA_REQUEST_LIST_NUM;
    if (next_idx == end_idx) {
        memset_s(request_list, sizeof(sync_peer_info) * MAC_CHBA_REQUEST_LIST_NUM, 0,
            sizeof(sync_peer_info) * MAC_CHBA_REQUEST_LIST_NUM);
        sync_req_ctrl->next_request_idx = 0;
        sync_req_ctrl->end_idx = 0;
        oam_warning_log0(0, OAM_SF_CHBA,
            "hmac_chba_sync_wait_timeout::sync request waiting timer timeout, no peer in sync request list.");
    } else {
        sync_req_ctrl->next_request_idx = next_idx;
        oam_warning_log1(0, OAM_SF_CHBA,
            "hmac_chba_sync_wait_timeout::sync request waiting timer timeout, next idx is [%d].", next_idx);
    }

    /* 尝试下一次主动同步请求 */
    err = hmac_chba_try_once_sync_request(sync_info, &sync_info->sync_flags, sync_req_ctrl);
    if (err != OAL_SUCC) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_wait_timeout::fail to try once sync request.");
    }
    return OAL_SUCC;
}

static void hmac_chba_first_sync(master_info *new_master, uint8_t *sa_addr,
    hmac_chba_sync_info *sync_info, sync_info_flags *sync_flags)
{
    uint8_t *own_addr = NULL;

    own_addr = hmac_chba_get_own_mac_addr();
    if (own_addr == NULL) {
        return;
    }

    sync_flags->sync_with_new_master_flag = TRUE;
    hmac_chba_set_sync_request_flag(sync_flags, OAL_FALSE); /* 结束主动同步请求 */

    /* 如果当前状态为NON_SYNC或trySyncFlag为1，即从失同步到同步的场景 */
    /* 如果newMaster是自己，则再次将自己变成Master */
    if (oal_compare_mac_addr(new_master->master_addr, own_addr) == 0) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_first_sync::new master is me.");
        hmac_chba_sync_become_master_handler(sync_info, FALSE);
    } else {
        /* 将newMaster的信息更新到当前Master */
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_first_sync::sync with master now.");
        hmac_chba_set_role(CHBA_OTHER_DEVICE);
        hmac_chba_set_sync_state(CHBA_WORK);
        /* 清空同步相关信息 */
        hmac_chba_clear_sync_info(sync_info);
        hmac_chba_sync_update_master_info(sync_info, new_master, FALSE);
    }
    oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_first_sync::new sync state[%d]", hmac_chba_get_sync_state());
}

static void hmac_chba_domain_merge_sync(hmac_vap_stru *hmac_vap, uint8_t *sa_addr,
    hmac_chba_sync_info *sync_info, hmac_chba_domain_merge_info *domain_merge)
{
    if (sync_info == NULL || domain_merge == NULL) {
        return;
    }

    oam_warning_log3(0, OAM_SF_CHBA, "domain_merge_sync::domain_merge:%d, masterUpdate:%d, domainMergeSyncFlag:%d.",
        hmac_chba_get_domain_merge_flag(domain_merge), sync_info->sync_flags.master_update_flag,
        domain_merge->domain_merge_sync_flag);

    if (domain_merge->domain_merge_sync_flag == OAL_TRUE) {
        return;
    }
    /* 如果是域合并过程中与新Master同步的场景 */
    oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_domain_merge_sync::sync with another domain now.");
    domain_merge->domain_merge_sync_flag = OAL_TRUE;

    hmac_chba_domain_merge_sync_handler(hmac_vap, sync_info);
}


void hmac_chba_rx_bcn_non_sync_state_handler(hmac_vap_stru *hmac_vap, uint8_t *sa_addr, uint8_t *domain_bssid,
    uint8_t *payload, uint16_t payload_len)
{
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();
    sync_peer_info *request_list = hmac_chba_get_sync_req_list();
    sync_info_flags *sync_flags = hmac_chba_get_sync_flags();
    uint32_t result;
    uint8_t next_idx;
    uint8_t peer_work_channel;
    master_info master_info = {0};

    oam_warning_log3(0, OAM_SF_CHBA,
        "{rx_bcn_non_sync_state_handler:sync with master flag[%d], try bssid[0x%x:%x]}",
        sync_flags->sync_with_new_master_flag, sync_info->try_bssid[MAC_ADDR_4], sync_info->try_bssid[MAC_ADDR_5]);
    hmac_chba_get_master_info_from_frame(payload, payload_len, sa_addr, &master_info);
    /* 如果sync_with_new_master_flag为0，且该帧的域BSSID与正在同步的域BSSID匹配，则说明该帧为同步帧 */
    result = oal_compare_mac_addr(domain_bssid, sync_info->try_bssid);
    if (sync_flags->sync_with_new_master_flag == FALSE && result == 0) {
        hmac_chba_first_sync(&master_info, sa_addr, sync_info, sync_flags);
        return;
    }

    /* 如果syncRequestFlag为1，且该帧的发送地址等于当前请求的peerAddr，则判断是否是Master发生更新 */
    next_idx = sync_info->sync_req_ctrl.next_request_idx;
    result = oal_compare_mac_addr(sa_addr, request_list[next_idx].peer_addr);
    if (hmac_chba_get_sync_request_flag(sync_flags) == TRUE && result == 0) {
        hmac_chba_resync_with_same_peer(master_info.master_addr, sync_info, &(request_list[next_idx]));
        return;
    }

    peer_work_channel = master_info.master_work_channel;

    /* 未收到请求的bssid的beacon,收到任何一个bcn都向其发起主动同步 */
    hmac_chba_add_peer_into_request_list(peer_work_channel, sa_addr, master_info.master_addr, FALSE);

    /* 启动主动同步流程 */
    hmac_chba_start_sync_request(sync_info);
}


static void hmac_chba_rx_island_bcn_update_master_info(uint8_t *sa_addr, uint8_t *domain_bssid,
    uint8_t *payload, uint16_t payload_len)
{
    uint8_t *master_attr = NULL;
    uint8_t *master_addr = NULL;
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();
    master_info new_master;
    uint8_t peer_domain_bssid[WLAN_MAC_ADDR_LEN];

    /* 其他域的beacon不处理 */
    if (oal_compare_mac_addr(sync_info->domain_bssid, domain_bssid) != 0) {
        return;
    }

    /* 只处理岛主或Master所发的beacon帧 */
    if (oal_compare_mac_addr(sa_addr, self_conn_info->island_owner_addr) != 0) {
        return;
    }

    /* 获取主设备选举字段 */
    master_attr = hmac_find_chba_attr(MAC_CHBA_ATTR_MASTER_ELECTION, payload, payload_len);
    if (master_attr == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA,
            "{hmac_chba_rx_island_bcn_update_master_info:no master election attr, invalid beacon}");
        return;
    }
    if (MAC_CHBA_ATTR_HDR_LEN + master_attr[MAC_CHBA_ATTR_ID_LEN] < MAC_CHBA_MASTER_ELECTION_ATTR_LEN) {
        oam_error_log1(0, OAM_SF_CHBA, "{hmac_chba_rx_island_bcn_update_master_info:master election attr len[%d] \
            invalid.}", master_attr[MAC_CHBA_ATTR_ID_LEN]);
        return;
    }

    /* 从主设备选举字段中的Master信息计算其对应的bssid */
    master_addr = master_attr + MAC_CHBA_MASTER_ADDR_POS;
    hmac_generate_chba_domain_bssid(peer_domain_bssid, WLAN_MAC_ADDR_LEN, master_addr, WLAN_MAC_ADDR_LEN);
    /* beacon帧的bssid与其master对应的CHBA域bssid信息不一致, 表明是域合并后的通知beacon, 全岛需切换至此Master的域中 */
    /* 获取新master信息, 并判断当前是否已在master更新流程中 */
    if (hmac_chba_sync_check_master_change(sync_info, &new_master, payload, payload_len, sa_addr) == FALSE) {
        return;
    }

    /* beacon帧携带的master字段与自身master仍不一致， 直接更新Master信息 */
    hmac_chba_sync_update_master_info(sync_info, &new_master, OAL_TRUE);
    oam_warning_log3(0, OAM_SF_CHBA,
        "{hmac_chba_rx_island_bcn_update_master_info:update bssid XX:XX:XX:%02x:%02x:%02x}",
        peer_domain_bssid[3], peer_domain_bssid[4], peer_domain_bssid[5]);
}


void hmac_chba_rx_bcn_sync_handler(hmac_vap_stru *hmac_vap, uint8_t *sa_addr, uint8_t *domain_bssid, uint8_t *payload,
    uint16_t payload_len)
{
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();
    hmac_chba_domain_merge_info *domain_merge = hmac_chba_get_domain_merge_info();
    sync_info_flags *sync_flags = hmac_chba_get_sync_flags();

    /* 如果处于域合并过程中，且该帧的域BSSID与正在同步的域BSSID匹配，则说明该帧为同步帧 */
    if (hmac_chba_get_domain_merge_flag(domain_merge) == TRUE) {
        if (oal_compare_mac_addr(domain_bssid, hmac_chba_sync_get_domain_bssid()) == 0) {
            hmac_chba_domain_merge_sync(hmac_vap, sa_addr, sync_info, domain_merge);
        }
        return;
    }

    /* 收到岛内通知beacon, 更新自身信息 */
    hmac_chba_rx_island_bcn_update_master_info(sa_addr, domain_bssid, payload, payload_len);

    /* 收到master所发beacon */
    if (oal_compare_mac_addr(sa_addr, sync_info->cur_master_info.master_addr) == 0) {
        hmac_chba_sync_master_bcn_process(sync_flags, payload, payload_len);
    }

    /* 如果发送地址等于master地址, 岛内节点检查工作信道与岛主信道是否一致，不一致则切换到岛主信道 */
    if (oal_compare_mac_addr(sa_addr, sync_info->cur_master_info.master_addr) == 0) {
        hmac_chba_check_island_owner_chan(hmac_vap, sa_addr, payload, payload_len);
    }
}


uint32_t hmac_chba_domain_merge_sync_wait_timeout(void *p)
{
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();
    hmac_chba_domain_merge_info *domain_merge = hmac_chba_get_domain_merge_info();
    frw_timeout_stru *p_timer = &sync_info->domain_merge_waiting_timer;

    /* 该定时器已经被注销 */
    if (p_timer->en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(p_timer);
    }

    /* 如果已经同步上，直接返回 */
    if (domain_merge->domain_merge_sync_flag == TRUE) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_domain_merge_sync_wait_timeout::already sync with another domain.");
        return OAL_SUCC;
    }

    domain_merge->merge_req_cnt++;
    /* 连续请求同步次数超过门限, 放弃域合并 */
    if (domain_merge->merge_req_cnt > MAC_MERGE_REQ_MAX_CNT) {
        hmac_chba_domain_merge_recovery_handler();
        oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_domain_merge_sync_wait_timeout::exceed cnt[%d],finish send.",
            domain_merge->merge_req_cnt);
        domain_merge->merge_req_cnt = 0;
        return OAL_SUCC;
    }

    if (domain_merge->req_action_type == MAC_CHBA_SYNC_REQUEST) {
        hmac_chba_send_sync_action(domain_merge->peer_addr, hmac_chba_sync_get_domain_bssid(),
            MAC_CHBA_SYNC_REQUEST, MAC_CHBA_REQUEST_ACTION);
    } else {
        hmac_chba_send_sync_action(domain_merge->peer_addr, hmac_chba_sync_get_domain_bssid(),
            MAC_CHBA_DOMAIN_MERGE, MAC_CHBA_RESPONSE_ACTION);
    }

    /* 启动等待定时器(16ms) */
    hmac_chba_create_timer_ms(&sync_info->domain_merge_waiting_timer, MAC_CHBA_SLOT_DURATION,
        hmac_chba_domain_merge_sync_wait_timeout);

    /* 如果是强制执行的域合并，继续发送帧，直到同步上 */
    oam_warning_log0(0, OAM_SF_CHBA,
        "hmac_chba_domain_merge_sync_wait_timeout::mandatory domain merge, continue sending request action.");
    return OAL_SUCC;
}


uint32_t hmac_chba_domain_merge_resp_wait_timeout(void *p)
{
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();
    hmac_chba_domain_merge_info *domain_merge = hmac_chba_get_domain_merge_info();
    frw_timeout_stru *p_timer = &sync_info->domain_merge_waiting_timer;

    /* 该定时器已经被注销 */
    if (p_timer->en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(p_timer);
    }

    /* 只有其他设备会进入该函数 */
    if (domain_merge->his_role == CHBA_MASTER || domain_merge->his_role == CHBA_ISLAND_OWNER) {
        oam_error_log0(0, OAM_SF_CHBA, "hmac_chba_domain_merge_resp_wait_timeout::shouldn't enter this function.");
    }
    domain_merge->merge_req_cnt++;
    if (domain_merge->merge_req_cnt > MAC_MERGE_REQ_MAX_CNT) {
        oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_domain_merge_resp_wait_timeout::exceed cnt[%d],finish send.",
            domain_merge->merge_req_cnt);
        domain_merge->merge_req_cnt = 0;
        hmac_chba_set_sync_domain_merge_flag(domain_merge, OAL_FALSE);
        return OAL_SUCC;
    }
    /* 继续给岛主发送domain merge request帧 */
    oam_warning_log3(0, OAM_SF_CHBA,
        "hmac_chba_domain_merge_resp_wait_timeout::send domain merge request, peer_addr XX:XX:XX:%02X:%02X:%02X.",
        sync_info->his_master_info.master_addr[MAC_ADDR_3], sync_info->his_master_info.master_addr[MAC_ADDR_4],
        sync_info->his_master_info.master_addr[MAC_ADDR_5]);
    hmac_chba_send_sync_action(sync_info->his_master_info.master_addr, hmac_chba_sync_get_domain_bssid(),
        MAC_CHBA_DOMAIN_MERGE, MAC_CHBA_REQUEST_ACTION);
    hmac_chba_create_timer_ms(&sync_info->domain_merge_waiting_timer, MAC_CHBA_SLOT_DURATION,
        hmac_chba_domain_merge_resp_wait_timeout);
    return OAL_SUCC;
}



uint32_t hmac_chba_next_slot_timeout(void *p)
{
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();
    hmac_chba_domain_merge_info *domain_merge = hmac_chba_get_domain_merge_info();
    frw_timeout_stru *p_timer = &sync_info->domain_merge_next_timer;
    hmac_vap_stru *hmac_vap = (hmac_vap_stru *)p;

    /* 注销定时器 */
    if (p_timer->en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(p_timer);
    }

    domain_merge->beacon_cnt--;
    if (domain_merge->beacon_cnt > 0) {
        oam_warning_log3(0, OAM_SF_CHBA,
            "hmac_chba_next_slot_timeout::domain merge, send beacon, bssid XX:XX:XX:%02X:%02X:%02X.",
            domain_merge->his_bssid[3], domain_merge->his_bssid[4], domain_merge->his_bssid[5]);
        hmac_chba_send_sync_beacon(hmac_vap, domain_merge->his_bssid, OAL_MAC_ADDR_LEN);
        hmac_chba_create_timer_ms(&sync_info->domain_merge_next_timer, MAC_CHBA_SLOT_DURATION,
            hmac_chba_next_slot_timeout);
        return OAL_SUCC;
    }

    /* 已经发送完指定次数的beacon，域合并结束 */
    hmac_chba_set_sync_domain_merge_flag(domain_merge, OAL_FALSE);
    oam_warning_log0(0, OAM_SF_CFG, "hmac_chba_next_slot_timeout::domain merge processing is end.");
    return OAL_SUCC;
}

void hmac_chba_domain_merge_sync_handler(hmac_vap_stru *hmac_vap, hmac_chba_sync_info *sync_info)
{
    hmac_chba_domain_merge_info *domain_merge = hmac_chba_get_domain_merge_info();

    /* 注销等待定时器 */
    if (sync_info->domain_merge_waiting_timer.en_is_registerd) {
        frw_timer_immediate_destroy_timer_m(&sync_info->domain_merge_waiting_timer);
    }

    /* 如果是其他设备，向岛主发送domain_merge_request */
    if (domain_merge->his_role == CHBA_OTHER_DEVICE) {
        oam_warning_log3(0, OAM_SF_CHBA,
            "hmac_chba_domain_merge_sync_handler::send domain merge request, peer_addr XX:XX:XX:%02X:%02X:%02X.",
            sync_info->his_master_info.master_addr[MAC_ADDR_3], sync_info->his_master_info.master_addr[MAC_ADDR_4],
            sync_info->his_master_info.master_addr[MAC_ADDR_5]);
        hmac_chba_send_sync_action(sync_info->his_master_info.master_addr, hmac_chba_sync_get_domain_bssid(),
            MAC_CHBA_DOMAIN_MERGE, MAC_CHBA_REQUEST_ACTION);
        domain_merge->merge_req_cnt = 0;
        hmac_chba_create_timer_ms(&sync_info->domain_merge_waiting_timer,
            MAC_CHBA_SLOT_DURATION * MAC_MERGE_REQ_MAX_SLOT_CNT, hmac_chba_domain_merge_resp_wait_timeout);
        return;
    }

    /* 如果是岛主或Master，从现在开始每个slot发送一个广播beacon帧，连续发4个 */
    domain_merge->beacon_cnt = MAC_CHBA_BROADCAST_BEACON_CNT;
    oam_warning_log3(0, OAM_SF_CHBA,
        "hmac_chba_domain_merge_sync_handler::domain merge, send beacon, bssid XX:XX:XX:%02X:%02X:%02X.",
        domain_merge->his_bssid[MAC_ADDR_3], domain_merge->his_bssid[MAC_ADDR_4], domain_merge->his_bssid[MAC_ADDR_5]);
    hmac_chba_send_sync_beacon(hmac_vap, domain_merge->his_bssid, OAL_MAC_ADDR_LEN);
    hmac_chba_create_timer_ms(&sync_info->domain_merge_next_timer, MAC_CHBA_SLOT_DURATION, hmac_chba_next_slot_timeout);
}


void hmac_chba_multiple_domain_detection_after_asoc(uint8_t *sa_addr, chba_req_sync_after_assoc_stru *req_sync)
{
    master_info another_master = { 0 };
    master_info *cur_master = NULL;
    hmac_chba_domain_merge_info *domain_merge = hmac_chba_get_domain_merge_info();
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();

    /* 如果已经在domain merge过程中，则直接返回 */
    if (hmac_chba_get_domain_merge_flag(domain_merge) == TRUE) {
        oam_warning_log0(0, OAM_SF_CHBA,
            "hmac_chba_multiple_domain_detection_after_asoc::already in domain merge processing, return.");
        return;
    }

    /* 获取对端Mater信息 */
    another_master.master_rp = req_sync->peer_master_rp;
    oal_set_mac_addr(another_master.master_addr, req_sync->peer_master_addr);
    another_master.master_work_channel = req_sync->peer_work_channel;
    /* 比较自己master的RP值，与该帧携带的master RP值信息，判断是否需要更新Master */
    cur_master = &(sync_info->cur_master_info);
    if (hmac_chba_sync_dev_rp_compare(cur_master, &another_master) == OAL_TRUE) {
        oam_warning_log0(0, OAM_SF_CHBA, "multiple_domain_detection_after_asoc::don't need to change master.");
        return;
    }

    hmac_chba_domain_merge_start_handler(&another_master, sa_addr, MAC_CHBA_SYNC_REQUEST);
}
uint32_t hmac_chba_sync_multiple_domain_info(mac_vap_stru *mac_vap, uint8_t len, uint8_t *puc_param)
{
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();
    master_info *cur_master = &(sync_info->cur_master_info);
    mac_chba_multiple_domain_stru  *multi_domain = (mac_chba_multiple_domain_stru  *)(puc_param);
    hmac_vap_stru *hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);

    if (hmac_vap == NULL) {
        oam_error_log0(mac_vap->uc_device_id, OAM_SF_PMF, "{hmac_sa_query_del_user::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (hmac_chba_sync_dev_rp_compare(cur_master, &multi_domain->another_master) == OAL_TRUE) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_multiple_domain_info::don't need to change master.");
        /* 对于beacon/PNF帧，给对方发一个beacon帧，增加域合并触发概率 */
        hmac_chba_send_sync_beacon(hmac_vap, sync_info->domain_bssid, OAL_MAC_ADDR_LEN);
        return OAL_SUCC;
    }
    /* 当前master的rp值小于收到的beacon的rp，触发域合并 */
    hmac_chba_domain_merge_start_handler(&multi_domain->another_master, multi_domain->sa_addr, MAC_CHBA_SYNC_REQUEST);
    return OAL_SUCC;
}

static void hmac_chba_domain_merge_update_update_role(uint8_t role)
{
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();

    /* 非Master设备将当前role等信息保存下来 */
    if (role != CHBA_MASTER) {
        hmac_chba_set_role(role);
        return;
    }

    /* master直接变成other_device */
    hmac_chba_set_role(CHBA_OTHER_DEVICE);
    /* 清除岛主地址 */
    self_conn_info->island_owner_valid = FALSE;
    memset_s(self_conn_info->island_owner_addr, OAL_MAC_ADDR_LEN, 0, OAL_MAC_ADDR_LEN);
}


void hmac_chba_domain_merge_start_handler(master_info *another_master, uint8_t *peer_addr, uint8_t mgmt_type)
{
    sync_info_flags *sync_flags = hmac_chba_get_sync_flags();
    hmac_chba_domain_merge_info *domain_merge = hmac_chba_get_domain_merge_info();
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();

    if (another_master == NULL || peer_addr == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_domain_merge_start_handler::input pointer is null.");
        return;
    }

    /* 更新Master信息 */
    memset_s(domain_merge, sizeof(hmac_chba_domain_merge_info), 0, sizeof(hmac_chba_domain_merge_info));
    domain_merge->req_action_type = mgmt_type;
    oal_set_mac_addr(domain_merge->peer_addr, peer_addr);
    hmac_chba_domain_merge_update_update_role(hmac_chba_get_role());
    hmac_chba_sync_update_master_info(sync_info, another_master, FALSE);
    sync_flags->master_update_flag = FALSE; /* master更新过程提前结束 */
    hmac_chba_set_sync_request_flag(sync_flags, OAL_FALSE); /* 结束主动同步请求 */
    hmac_chba_set_sync_domain_merge_flag(domain_merge, OAL_TRUE);
    domain_merge->domain_merge_sync_flag = FALSE;
    domain_merge->merge_req_cnt = 0;
    /* 向该设备发送sync request action帧 */
    if (mgmt_type == MAC_CHBA_SYNC_REQUEST) {
        oam_warning_log3(0, OAM_SF_CHBA,
            "hmac_chba_domain_merge_start_handler::send sync request action, peer_addr is XX:XX:XX:%02X:%02X:%02X.",
            peer_addr[3], peer_addr[4], peer_addr[5]);
        hmac_chba_send_sync_action(peer_addr, hmac_chba_sync_get_domain_bssid(),
            MAC_CHBA_SYNC_REQUEST, MAC_CHBA_REQUEST_ACTION);
    } else if (mgmt_type == MAC_CHBA_DOMAIN_MERGE) {
        oam_warning_log3(0, OAM_SF_CHBA,
            "hmac_chba_domain_merge_start_handler::send domain merge resp action, peer_addr XX:XX:XX:%02X:%02X:%02X.",
            peer_addr[3], peer_addr[4], peer_addr[5]);
        hmac_chba_send_sync_action(peer_addr, hmac_chba_sync_get_domain_bssid(),
            MAC_CHBA_DOMAIN_MERGE, MAC_CHBA_RESPONSE_ACTION);
    }

    /* 启动等待定时器(16 * 3ms) */
    hmac_chba_create_timer_ms(&sync_info->domain_merge_waiting_timer,
        MAC_CHBA_SLOT_DURATION * MAC_MERGE_REQ_MAX_SLOT_CNT, hmac_chba_domain_merge_sync_wait_timeout);
}


void hmac_chba_domain_merge_recovery_handler()
{
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();
    hmac_chba_domain_merge_info *domain_merge = hmac_chba_get_domain_merge_info();
    master_info his_master;
    int32_t err;

    hmac_chba_set_sync_domain_merge_flag(domain_merge, OAL_FALSE);
    /* 如果原来是master恢复成master，其他恢复原来的master */
    if (domain_merge->his_role == CHBA_MASTER) {
        oam_warning_log0(0, 0, "hmac_chba_domain_merge_recovery_handler::become master.");
        hmac_chba_sync_become_master_handler(sync_info, FALSE);
        return;
    }
    err = memcpy_s(&his_master, sizeof(master_info), &sync_info->his_master_info, sizeof(master_info));
    if (err != EOK) {
        oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_domain_merge_recovery_handler::memcpy failed, err[%d].", err);
    }
    /* 更新Master信息 */
    hmac_chba_sync_update_master_info(sync_info, &his_master, TRUE);
}


uint8_t hmac_chba_sync_check_master_change(hmac_chba_sync_info *sync_info, master_info *new_master,
    uint8_t *payload, uint16_t payload_len, uint8_t *sa_addr)
{
    sync_info_flags *sync_flags = hmac_chba_get_sync_flags();
    uint8_t *attr_pos1 = NULL;
    uint8_t *attr_pos2 = NULL;
    uint8_t *own_addr = hmac_chba_get_own_mac_addr();
    int32_t err;

    if (sync_info == NULL || new_master == NULL || payload == NULL || sa_addr == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_check_master_change::input pointer is null.");
        return FALSE;
    }

    /* 如果正处于Master更新过程中，则不处理该beacon帧 */
    if (sync_flags->master_update_flag == TRUE) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_check_master_change::already in master updating, return.");
        return FALSE;
    }

    /* 查找主设备属性 */
    attr_pos1 = hmac_find_chba_attr(MAC_CHBA_ATTR_MASTER_ELECTION, payload, payload_len);
    if (attr_pos1 == NULL || MAC_CHBA_ATTR_HDR_LEN + attr_pos1[1] < MAC_CHBA_MASTER_ELECTION_ATTR_LEN) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_check_master_change::no master election attr, invalid PNF.");
        return FALSE;
    }
    err = memcpy_s((uint8_t *)&(new_master->master_rp), sizeof(ranking_priority),
        attr_pos1 + MAC_CHBA_MASTER_RANKING_LEVEL_POS, sizeof(ranking_priority));
    if (err != EOK) {
        oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_sync_check_master_change::memcpy failed, err[%d].", err);
    }
    oal_set_mac_addr(new_master->master_addr, attr_pos1 + MAC_CHBA_MASTER_ADDR_POS);
    attr_pos2 = hmac_find_chba_attr(MAC_CHBA_ATTR_LINK_INFO, payload, payload_len);
    if (attr_pos2 == NULL || MAC_CHBA_ATTR_HDR_LEN + attr_pos2[1] < MAC_CHBA_LINK_INFO_CHAN_POS + 1) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_check_master_change::can't find link info attr.");
        return FALSE;
    }

    new_master->master_work_channel = *(attr_pos2 + MAC_CHBA_ATTR_HDR_LEN);

    /* 如果该帧的master是自己，则再次将自己设置为Master */
    if (own_addr == NULL) {
        return FALSE;
    }
    if (oal_compare_mac_addr(new_master->master_addr, own_addr) == 0) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_check_master_change::I am the master.");
        hmac_chba_sync_become_master_handler(sync_info, FALSE);
        return FALSE;
    }

    /* 如果该帧的master字段不同，则返回false */
    if (oal_compare_mac_addr(new_master->master_addr, sync_info->cur_master_info.master_addr) != 0) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_check_master_change::master changed.");
        return TRUE;
    }
    /* 如果该帧来自当前master，则更新当前master的RP值、信道等信息 */
    if (oal_compare_mac_addr(sa_addr, sync_info->cur_master_info.master_addr) == 0) {
        sync_info->cur_master_info.master_rp = new_master->master_rp;
        sync_info->cur_master_info.master_work_channel = new_master->master_work_channel;
        /* 同步到dmac */
        hmac_chba_h2d_sync_master_info(sync_info);
    }
    return FALSE;
}
static void hmac_chba_check_need_recovery_channnel(hmac_vap_stru *hmac_vap,
    hmac_chba_vap_stru *chba_vap_info)
{
    if (hmac_vap == NULL || chba_vap_info == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_check_need_recovery_channnel::input pointer is null.");
        return;
    }
    /* 如果需要恢复信道，则恢复信道 */
    if (chba_vap_info->need_recovery_work_channel == OAL_TRUE) {
        chba_vap_info->need_recovery_work_channel = OAL_FALSE;
        oam_warning_log2(0, OAM_SF_CHBA, "hmac_chba_check_need_recovery_work_channnel::current chan is [%d],\
            need recovery chan to [%d]", hmac_vap->st_vap_base_info.st_channel.uc_chan_number,
            chba_vap_info->work_channel.uc_chan_number);
        hmac_config_social_channel(chba_vap_info,
            chba_vap_info->work_channel.uc_chan_number, chba_vap_info->work_channel.en_bandwidth);
        hmac_chba_config_channel(&hmac_vap->st_vap_base_info, chba_vap_info,
            chba_vap_info->work_channel.uc_chan_number, chba_vap_info->work_channel.en_bandwidth);
    }
}
static void hmac_chba_sync_chba_info(hmac_vap_stru *hmac_vap, hmac_chba_vap_stru *chba_vap_info,
    hmac_chba_sync_info *sync_info)
{
    mac_vap_stru *mac_vap = NULL;
    chba_params_config_stru chba_params_sync = { 0 };

    if (hmac_vap == NULL || chba_vap_info == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_chba_info::input pointer is null.");
        return;
    }
    mac_vap = &hmac_vap->st_vap_base_info;
    /* 下发同步信息到dmac */
    chba_params_sync.chba_state = hmac_chba_get_sync_state();
    chba_params_sync.chba_role = hmac_chba_get_role();
    oal_set_mac_addr(chba_params_sync.domain_bssid, mac_vap->auc_bssid);
    chba_params_sync.master_info = sync_info->cur_master_info;
    chba_params_sync.info_bitmap = CHBA_MASTER_INFO | CHBA_STATE | CHBA_ROLE | CHBA_BSSID;

    /* 如果角色成为master，则封装beacon帧；否则封装pnf帧 */
    if (hmac_chba_get_role() == CHBA_MASTER) {
        hmac_chba_encap_sync_beacon_frame(mac_vap, chba_params_sync.sync_beacon, &chba_params_sync.sync_beacon_len,
            mac_vap->auc_bssid, chba_vap_info->beacon_buf, chba_vap_info->beacon_buf_len);
        chba_params_sync.info_bitmap |= CHBA_BEACON_BUF | CHBA_BEACON_LEN;
    } else {
        hmac_chba_encap_pnf_action_frame(mac_vap, chba_params_sync.pnf, &chba_params_sync.pnf_len);
        chba_params_sync.info_bitmap |= CHBA_PNF_BUF | CHBA_PNF_LEN;
    }

    hmac_chba_params_sync(mac_vap, &chba_params_sync);
}

void hmac_chba_sync_update_master_info(hmac_chba_sync_info *sync_info, master_info *new_master,
    uint8_t update_flag)
{
    sync_info_flags *sync_flags = hmac_chba_get_sync_flags();
    uint8_t *own_addr = NULL;
    int32_t err;
    hmac_chba_vap_stru *chba_vap_info = hmac_get_chba_vap();
    hmac_vap_stru *hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(chba_vap_info->mac_vap_id);

    if (sync_info == NULL || new_master == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_update_master_info::input pointer is null.");
        return;
    }

    own_addr = hmac_chba_get_own_mac_addr();
    if (own_addr == NULL) {
        return;
    }

    if (update_flag == TRUE) {
        oam_warning_log0(0, 0, "hmac_chba_sync_update_master_info::masterUpdateFlag set to True.");
        sync_flags->master_update_flag = TRUE;
        sync_flags->sync_with_new_master_flag = FALSE;
    }

    /* 将当前master的信息保存到history_master_info中，将new master的信息保存到cur_master_info中 */
    err = memcpy_s(&(sync_info->his_master_info), sizeof(master_info),
        &(sync_info->cur_master_info), sizeof(master_info));
    err += memcpy_s(&(sync_info->cur_master_info), sizeof(master_info), new_master, sizeof(master_info));
    if (err != EOK) {
        oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_sync_update_master_info::memcpy failed, err[%d].", err);
    }
    /* 设置Master地址信息 */
    oal_set_mac_addr(hmac_chba_get_master_mac_addr(), new_master->master_addr);
    /* 生成域BSSID */
    hmac_chba_generate_domain_bssid(hmac_chba_sync_get_domain_bssid(), new_master->master_addr, OAL_MAC_ADDR_LEN);
    hmac_chba_vap_update_domain_bssid(hmac_vap, hmac_chba_get_master_mac_addr());
    hmac_chba_sync_update_own_rp(hmac_vap);
    hmac_chba_encap_pnf_action(chba_vap_info, chba_vap_info->pnf_buf, &chba_vap_info->pnf_buf_len);
    hmac_chba_check_need_recovery_channnel(hmac_vap, chba_vap_info);
    hmac_chba_sync_chba_info(hmac_vap, chba_vap_info, sync_info);
    oam_warning_log2(0, OAM_SF_CHBA, "hmac_chba_sync_update_master_info::NEW MASTER[0x%x:0x%x].",
        new_master->master_addr[MAC_ADDR_4], new_master->master_addr[MAC_ADDR_5]);
}

static void hmac_chba_become_master_island_proc(hmac_vap_stru *hmac_vap)
{
    hmac_chba_island_list_stru tmp_island_info = { 0 };

    /* 根据连接拓扑图更新生成岛信息 */
    hmac_chba_update_island_info(hmac_vap, &tmp_island_info);
    hmac_chba_print_island_info(&tmp_island_info);

    /* 选举岛主，并设置岛主属性 */
    hmac_chba_island_owner_selection_proc(&tmp_island_info);

    /* 更新本设备连接信息 */
    hmac_chba_update_self_conn_info(hmac_vap, &tmp_island_info, OAL_TRUE);
    hmac_chba_print_self_conn_info();

    /* 清空临时岛信息，释放内存 */
    hmac_chba_clear_island_info(&tmp_island_info, hmac_chba_tmp_island_list_node_free);
}

int32_t hmac_chba_sync_become_master_handler(hmac_chba_sync_info *sync_info,
    uint8_t rrm_list_flag)
{
    uint8_t *own_addr = NULL;
    int32_t err;
    attr_ctrl_info attr_ctrl = { 0 };
    hmac_chba_vap_stru *chba_vap_info = hmac_get_chba_vap();
    hmac_vap_stru *hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(chba_vap_info->mac_vap_id);

    /* 将角色设置为master，sync state设置为work */
    hmac_chba_set_role(CHBA_MASTER);
    hmac_chba_set_sync_state(CHBA_WORK);

    /* 生成域BSSID */
    own_addr = hmac_chba_get_own_mac_addr();
    if (own_addr == NULL) {
        return OAL_FAIL;
    }
    /* 设置Master地址信息 */
    oal_set_mac_addr(hmac_chba_get_master_mac_addr(), own_addr);
    /* 生成域BSSID */
    hmac_chba_generate_domain_bssid(hmac_chba_sync_get_domain_bssid(), own_addr, OAL_MAC_ADDR_LEN);
    hmac_chba_vap_update_domain_bssid(hmac_vap, hmac_chba_get_master_mac_addr());

    /* 变成master之后的岛信息相关处理 */
    hmac_chba_become_master_island_proc(hmac_vap);

    /* 清空同步控制信息 */
    hmac_chba_clear_sync_info(sync_info);
    hmac_chba_set_sync_request_flag(&sync_info->sync_flags, OAL_FALSE);
    sync_info->sync_flags.master_update_flag = FALSE;

    /* 将当前master的信息赋值到history_master_info，再将自己的信息填写到cur_master_info中 */
    err = memcpy_s(&(sync_info->his_master_info), sizeof(master_info), &(sync_info->cur_master_info),
        sizeof(master_info));
    err += memcpy_s(&(sync_info->cur_master_info.master_rp), sizeof(ranking_priority),
        &(sync_info->own_rp_info), sizeof(ranking_priority));
    oal_set_mac_addr(sync_info->cur_master_info.master_addr, own_addr);
    sync_info->cur_master_info.master_work_channel = chba_vap_info->work_channel.uc_chan_number;
    if (err != EOK) {
        oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_sync_become_master_handler::memcpy failed, err[%d].", err);
    }
    hmac_chba_sync_update_own_rp(hmac_vap);
    hmac_chba_encap_sync_beacon(chba_vap_info, chba_vap_info->beacon_buf, &chba_vap_info->beacon_buf_len, &attr_ctrl);
    hmac_chba_check_need_recovery_channnel(hmac_vap, chba_vap_info);
    hmac_chba_sync_chba_info(hmac_vap, chba_vap_info, sync_info);
    return OAL_SUCC;
}


int32_t hmac_chba_sync_become_island_owner_handler(hmac_chba_sync_info *sync_info)
{
    hmac_chba_vap_stru *chba_vap_info = hmac_get_chba_vap();
    hmac_vap_stru *hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(chba_vap_info->mac_vap_id);

    if (sync_info == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_become_island_owner_handler::input pointer is null.");
        return OAL_FAIL;
    }
    oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_become_island_owner_handler::become ISLAND OWNER.");
    /* 将角色设置为岛主 */
    hmac_chba_set_role(CHBA_ISLAND_OWNER);
    hmac_chba_sync_update_own_rp(hmac_vap);
    hmac_chba_encap_pnf_action(chba_vap_info, chba_vap_info->pnf_buf, &chba_vap_info->pnf_buf_len);
    hmac_chba_sync_chba_info(hmac_vap, chba_vap_info, sync_info);
    return OAL_SUCC;
}


int32_t hmac_chba_sync_become_other_dev_handler(hmac_chba_sync_info *sync_info)
{
    hmac_chba_vap_stru *chba_vap_info = hmac_get_chba_vap();
    hmac_vap_stru *hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(chba_vap_info->mac_vap_id);

    if (sync_info == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_become_other_dev_handler::input pointer is null.");
        return OAL_FAIL;
    }
    oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_become_other_dev_handler::IO become OTHER DEVICE.");
    /* 将角色设置为岛主 */
    hmac_chba_set_role(CHBA_OTHER_DEVICE);
    hmac_chba_sync_update_own_rp(hmac_vap);
    hmac_chba_encap_pnf_action(chba_vap_info, chba_vap_info->pnf_buf, &chba_vap_info->pnf_buf_len);
    hmac_chba_sync_chba_info(hmac_vap, chba_vap_info, sync_info);
    return OAL_SUCC;
}


void hmac_chba_update_island_sync_info(void)
{
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();
    uint8_t *own_addr = NULL;
    uint8_t new_owner_valid;

    own_addr = hmac_chba_get_own_mac_addr();
    if (own_addr == NULL) {
        return;
    }

    new_owner_valid = self_conn_info->island_owner_valid;

    /* 如果岛主有效，按照不同的角色进行处理 */
    if (new_owner_valid == TRUE) {
        /* 新岛主不是自己，则变成other device */
        if ((hmac_chba_get_role() == CHBA_ISLAND_OWNER) &&
            (oal_compare_mac_addr(self_conn_info->island_owner_addr, own_addr) != 0)) {
            hmac_chba_sync_become_other_dev_handler(sync_info);
        } else if ((hmac_chba_get_role() == CHBA_OTHER_DEVICE) &&
            (oal_compare_mac_addr(self_conn_info->island_owner_addr, own_addr) == 0)) {
            /* 从非岛主变成岛主 */
            hmac_chba_sync_become_island_owner_handler(sync_info);
        } else if (hmac_chba_get_role() == CHBA_OTHER_DEVICE) {
            /* 非岛主，且角色不变 */
        }
    } else if ((hmac_chba_get_role() == CHBA_ISLAND_OWNER) && (new_owner_valid == FALSE)) {
        hmac_chba_sync_become_other_dev_handler(sync_info);
    }
}


uint8_t hmac_chba_sync_dev_rp_compare(master_info *device_rp1, master_info *device_rp2)
{
    uint8_t result, device_level1, device_level2;
    int32_t value;

    if (device_rp1 == NULL || device_rp2 == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_dev_rp_compare::input pointer is null.");
        return TRUE;
    }

    /* 比较device level */
    device_level1 = (uint8_t)device_rp1->master_rp.device_level.value;
    device_level2 = (uint8_t)device_rp2->master_rp.device_level.value;
    if (device_level1 != device_level2) {
        result = (device_level1 > device_level2) ? TRUE : FALSE;
        oam_warning_log4(0, OAM_SF_CHBA,
            "hmac_chba_sync_dev_rp_compare::addr5[%x] : device level[%d], addr5[%x] : device level[%d].",
            device_rp1->master_addr[MAC_ADDR_5], device_level1, device_rp2->master_addr[MAC_ADDR_5], device_level2);
        return result;
    }

    /* 比较CHBA 版本号 */
    if (device_rp1->master_rp.chba_version != device_rp2->master_rp.chba_version) {
        result = (device_rp1->master_rp.chba_version > device_rp2->master_rp.chba_version) ? TRUE : FALSE;
        oam_warning_log4(0, OAM_SF_CHBA,
            "hmac_chba_sync_dev_rp_compare::addr5[%x] :max chba_version[%d], addr5[%x] :cur chba_version[%d].",
            device_rp1->master_addr[MAC_ADDR_5], device_rp1->master_rp.chba_version,
            device_rp2->master_addr[MAC_ADDR_5], device_rp2->master_rp.chba_version);
        return result;
    }

    /* 比较mac地址 */
    value = oal_memcmp(device_rp1->master_addr, device_rp2->master_addr, OAL_MAC_ADDR_LEN);
    result = (value >= 0) ? TRUE : FALSE; /* mac地址相同返回OAL_TRUE */
    oam_warning_log4(0, OAM_SF_CHBA, "hmac_chba_sync_dev_rp_compare::mac address [%x] vs [%x] value[%d] result[%d].",
        device_rp1->master_addr[MAC_ADDR_5], device_rp2->master_addr[MAC_ADDR_5], value, result);
    return result;
}


void hmac_chba_sync_master_election_compare(ranking_priority *peer_rp, uint8_t *peer_addr,
    uint8_t work_channel)
{
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();
    master_info *max_rp_device = NULL;
    master_info device_rp = { 0 };
    int32_t err;
    uint8_t result;

    if (peer_rp == NULL || peer_addr == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_master_election_compare::input pointer is null.");
        return;
    }
    max_rp_device = &(sync_info->max_rp_device);

    /* 将master election属性中的rp字段赋值到device rp中 */
    err = memcpy_s(&(device_rp.master_rp), sizeof(ranking_priority), peer_rp, sizeof(ranking_priority));
    oal_set_mac_addr(device_rp.master_addr, peer_addr);
    device_rp.master_work_channel = work_channel;
    if (err != EOK) {
        oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_sync_master_election_compare::memcpy failed, err[%d].", err);
        return;
    }

    /* 比较RP值: 第一个入参为当前最大值，第二个入参为现有值，返回true表示不用更新 */
    result = hmac_chba_sync_dev_rp_compare(max_rp_device, &device_rp);
    /* master选举属性中的RP值较大，替换到max_rp_device中 */
    if (result == FALSE) {
        err = memcpy_s(max_rp_device, sizeof(master_info), &device_rp, sizeof(master_info));
        if (err != EOK) {
            oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_sync_master_election_compare::memcpy failed, err[%d].", err);
        }
    }
}

uint32_t hmac_chba_update_master_election_info(mac_vap_stru *mac_vap, uint8_t len, uint8_t *param)
{
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();
    master_info *cur_max_rp_device = NULL;
    master_info *new_max_rp_device = (master_info*)param;
    int32_t err;

    cur_max_rp_device = &(sync_info->max_rp_device);
    /* master选举属性中的RP值较大，替换到max_rp_device中 */
    err = memcpy_s(cur_max_rp_device, sizeof(master_info), new_max_rp_device, sizeof(master_info));
    if (err != EOK) {
        oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_update_master_election_info::memcpy failed, err[%d].", err);
    }
    return OAL_SUCC;
}


static void hmac_chba_update_master_info_after_master_election_proc(hmac_chba_sync_info *sync_info,
    master_info *max_rp_device)
{
    uint8_t *own_addr = NULL;
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();

    /* 本身是非Master，变成Master */
    own_addr = hmac_chba_get_own_mac_addr();
    if (own_addr == NULL) {
        return;
    }
    if (oal_compare_mac_addr(max_rp_device->master_addr, own_addr) == 0) {
        oam_warning_log0(0, OAM_SF_CHBA, "{hmac_chba_update_master_info_after_master_election_proc:become master}");
        hmac_chba_sync_become_master_handler(sync_info, FALSE);
        return;
    }

    /* 本身是Master，则变成OtherDevice */
    if (hmac_chba_get_role() == CHBA_MASTER) {
        hmac_chba_set_role(CHBA_OTHER_DEVICE);
        /* 清除岛主地址 */
        self_conn_info->island_owner_valid = FALSE;
        memset_s(self_conn_info->island_owner_addr, OAL_MAC_ADDR_LEN, 0, OAL_MAC_ADDR_LEN);
        oam_warning_log0(0, OAM_SF_CHBA,
            "{hmac_chba_update_master_info_after_master_election_proc:become other device from master}");
    }

    /* 刷新Master信息 */
    hmac_chba_sync_update_master_info(sync_info, max_rp_device, TRUE);
}


void hmac_chba_sync_master_election_proc(hmac_vap_stru *hmac_vap, hmac_chba_sync_info *sync_info, uint8_t process_type)
{
    master_info *max_rp_device = NULL;
    master_info *cur_master = NULL;
    uint8_t result;

    if (sync_info == NULL) {
        oam_warning_log0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CHBA,
            "hmac_chba_sync_master_election_proc::input pointer is null.");
        return;
    }

    max_rp_device = &(sync_info->max_rp_device);
    cur_master = &(sync_info->cur_master_info);

    /* 比较RP值: 第一个入参为当前Master，第二个入参为本轮最大主设备，返回true表示不用更新 */
    if (process_type == MASTER_ALIVE) {
        result = hmac_chba_sync_dev_rp_compare(cur_master, max_rp_device);
        /* 本轮Master选举，master无需更新 */
        if (result == OAL_TRUE) {
            return;
        }
    }

    /* 根据全域选举结构更新Master */
    hmac_chba_update_master_info_after_master_election_proc(sync_info, max_rp_device);
}


void hmac_chba_get_info_from_frame(uint8_t *payload, uint16_t payload_len, device_info *device_info)
{
    uint8_t *attr_pos = NULL;
    int32_t err;

    if (payload == NULL || device_info == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_get_info_from_frame::input pointer is null.");
        return;
    }

    attr_pos = hmac_find_chba_attr(MAC_CHBA_ATTR_MASTER_ELECTION, payload, payload_len);
    if (attr_pos == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_get_info_from_frame::no master election attr, invalid.");
        return;
    }
    if (MAC_CHBA_ATTR_HDR_LEN + attr_pos[MAC_CHBA_ATTR_ID_LEN] < MAC_CHBA_MASTER_ELECTION_ATTR_LEN) {
        oam_error_log1(0, OAM_SF_CHBA, "hmac_chba_get_info_from_frame::master election attr len[%d] invalid.",
            attr_pos[MAC_CHBA_ATTR_ID_LEN]);
        return;
    }

    oal_set_mac_addr(device_info->master_addr, attr_pos + MAC_CHBA_MASTER_ADDR_POS);

    /* 获取Master的RP值 */
    err = memcpy_s((uint8_t *)&(device_info->master_rp), sizeof(ranking_priority),
        payload + MAC_CHBA_MASTER_RANKING_LEVEL_POS, sizeof(ranking_priority));
    if (err != EOK) {
        oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_get_info_from_frame::memcpy failed, err[%d].", err);
    }
    attr_pos = hmac_find_chba_attr(MAC_CHBA_ATTR_LINK_INFO, payload, payload_len);
    if (attr_pos == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_get_master_info_from_frame::no link info attr, invalid Frame.");
        return;
    }

    if (MAC_CHBA_ATTR_HDR_LEN + attr_pos[MAC_CHBA_ATTR_ID_LEN] < MAC_CHBA_LINK_INFO_CHAN_POS + 1) {
        oam_error_log1(0, OAM_SF_CHBA, "hmac_chba_get_master_info_from_frame::link info attr len[%d] invalid.",
            attr_pos[MAC_CHBA_ATTR_ID_LEN]);
        return;
    }
    device_info->work_channel = *(attr_pos + MAC_CHBA_ATTR_HDR_LEN);
    return;
}

void hmac_chba_get_master_info_from_frame(uint8_t *payload, uint16_t payload_len, uint8_t *sa_addr,
    master_info *master_info)
{
    device_info dev_info = {0};

    if (payload == NULL || sa_addr == NULL || master_info == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_get_master_info_from_frame::input pointer is null.");
        return;
    }

    hmac_chba_get_info_from_frame(payload, payload_len, &dev_info);
    oal_set_mac_addr(master_info->master_addr, dev_info.master_addr);
    master_info->master_rp = dev_info.master_rp;
    /* 如果发送地址等于master地址，则从link info属性获取工作信道 */
    if (oal_compare_mac_addr(sa_addr, dev_info.master_addr) != 0) {
        return;
    }
    if (dev_info.work_channel == 0) {
        return;
    }
    master_info->master_work_channel = dev_info.work_channel;
    oam_warning_log1(0, OAM_SF_CHBA, "hmac_chba_get_master_info_from_frame::get master work channel [%d] from frame.",
        master_info->master_work_channel);
}


void hmac_chba_sync_master_bcn_process(sync_info_flags *sync_flags,
    uint8_t *payload, uint16_t payload_len)
{
    /* role不对不处理 */
    if (hmac_chba_get_role() == CHBA_MASTER) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_master_bcn_process::self master");
        return;
    }

    if (sync_flags->sync_with_new_master_flag == OAL_FALSE) {
        sync_flags->sync_with_new_master_flag = OAL_TRUE;
        hmac_chba_set_sync_request_flag(sync_flags, OAL_FALSE); /* 结束主动同步请求 */
    }

    /* Master更新后的重新同步场景，master更新结束 */
    if (sync_flags->master_update_flag == TRUE && sync_flags->sync_with_new_master_flag == TRUE) {
        oam_warning_log0(0, OAM_SF_CHBA, "hmac_chba_sync_master_bcn_process::sync with master now.");
        sync_flags->master_update_flag = FALSE;
    }
}
#endif
