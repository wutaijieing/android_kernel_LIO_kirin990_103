

#ifdef _PRE_WLAN_CHBA_MGMT
/* 1 头文件包含 */
#include "securec.h"
#include "oal_cfg80211.h"
#include "hmac_chba_function.h"
#include "hmac_chba_interface.h"
#include "hmac_chba_frame.h"
#include "hmac_chba_mgmt.h"
#include "hmac_chba_coex.h"
#include "wlan_types.h"
#include "mac_device.h"
#include "mac_ie.h"
#include "hmac_chan_mgmt.h"
#include "hmac_scan.h"
#include "hmac_config.h"
#include "hmac_mgmt_ap.h"
#include "hmac_mgmt_sta.h"
#include "hmac_sme_sta.h"
#include "hmac_wpa_wpa2.h"
#include "hmac_chba_sync.h"
#include "hmac_chba_frame.h"
#include "hmac_chba_user.h"
#include "hmac_chba_channel_sequence.h"
#include "hmac_chba_fsm.h"
#include "hmac_sme_sta.h"
#include "hmac_chba_ps.h"
#ifdef _PRE_WLAN_FEATURE_STA_PM
#include "plat_pm_wlan.h"
#endif
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "hisi_customize_wifi.h"
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_FUNCTION_C

/* 2 全局变量定义 */
hmac_chba_vap_stru g_chba_vap_info = {0};
hmac_chba_system_params g_chba_system_params;
uint32_t g_discovery_bitmap = 0x01;

/* 3 函数声明 */
uint32_t hmac_chba_init_timeout(void *arg);
uint32_t hmac_chba_sync_waiting_timeout(void *arg);
uint32_t hmac_chba_next_discovery_timeout(void *arg);
uint32_t hmac_chba_domain_merge_end(void *arg);

uint32_t hmac_chba_fill_connect_params(uint8_t *params, hmac_chba_conn_param_stru *chba_conn_params,
    uint8_t connect_role);
static uint32_t hmac_chba_connect_check(mac_vap_stru *mac_vap, hmac_vap_stru *hmac_vap,
    hmac_chba_conn_param_stru *chba_conn_params);
static uint32_t hmac_chba_cfg_security_params(hmac_vap_stru *hmac_vap,
    mac_cfg80211_connect_param_stru *connect_params);
uint32_t hmac_chba_connect_prepare(mac_vap_stru *mac_vap, hmac_chba_conn_param_stru *chba_conn_params,
    uint16_t *user_idx, uint8_t connect_role);
uint32_t hmac_chba_assoc_waiting_timeout_responder(void *arg);
void hmac_chba_ready_to_connect(hmac_vap_stru *hmac_vap, hmac_chba_conn_param_stru *conn_param);
static void hmac_chba_fail_to_connect(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user,
    hmac_chba_conn_param_stru *conn_param, uint8_t connect_role);
static uint32_t hmac_config_social_channel_params(oal_uint8 channel_number, uint8_t bandwidth);


hmac_chba_system_params *hmac_get_chba_system_params(void)
{
    return &g_chba_system_params;
}


oal_bool_enum_uint8 hmac_chba_vap_start_check(hmac_vap_stru *hmac_vap)
{
    mac_vap_stru *mac_vap = NULL;
    hmac_chba_vap_stru *chba_vap_info = NULL;

    if (hmac_vap == NULL) {
        return OAL_FALSE;
    }
    mac_vap = &(hmac_vap->st_vap_base_info);
    if (mac_vap->_rom == NULL) {
        return OAL_FALSE;
    }
    chba_vap_info = hmac_vap->hmac_chba_vap_info;
    if ((((mac_vap_rom_stru *)(mac_vap->_rom))->chba_mode == CHBA_MODE) && (chba_vap_info == &g_chba_vap_info)) {
        if (hmac_chba_get_sync_state() > CHBA_INIT) {
            return OAL_TRUE;
        }
    }
    return OAL_FALSE;
}


hmac_chba_vap_stru *hmac_get_chba_vap(void)
{
    return &g_chba_vap_info;
}


hmac_chba_vap_stru *hmac_get_up_chba_vap_info(void)
{
    uint8_t vap_id;
    hmac_chba_vap_stru *chba_vap_info = NULL;
    hmac_vap_stru *hmac_vap = NULL;

    chba_vap_info = &(g_chba_vap_info);
    vap_id = chba_vap_info->mac_vap_id;

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(vap_id);
    if (hmac_vap == NULL) {
        return NULL;
    }

    if (hmac_vap->st_vap_base_info._rom == NULL) {
        return NULL;
    }

    if (hmac_chba_vap_start_check(hmac_vap)) {
        return chba_vap_info;
    }

    return NULL;
}

uint32_t hmac_chba_is_valid_channel(uint8_t chan_number)
{
    wlan_channel_band_enum en_band = mac_get_band_by_channel_num(chan_number);
    hmac_chba_vap_stru *chba_vap_info = &g_chba_vap_info;

    if (en_band != WLAN_BAND_5G || mac_is_channel_num_valid(en_band, chan_number) != OAL_SUCC ||
        (chba_vap_info->is_support_dfs_channel == OAL_FALSE && mac_is_dfs_channel(en_band, chan_number) == OAL_TRUE)) {
        OAM_WARNING_LOG1(0, OAM_SF_CHBA, "hmac_chba_is_valid_channel::invalid number %d.", chan_number);
        return OAL_FAIL;
    }
    return OAL_SUCC;
}


uint32_t hmac_config_chba_base_params(mac_chba_set_init_para *init_params)
{
    hmac_chba_system_params *system_params = &g_chba_system_params;
    hmac_chba_vap_stru *chba_vap_info = &g_chba_vap_info;

    if (init_params == NULL) {
        return OAL_SUCC;
    }

    /* 先销毁定时器 & 释放资源 */
    memset_s(chba_vap_info, sizeof(hmac_chba_vap_stru), 0, sizeof(hmac_chba_vap_stru));
    memset_s(system_params, sizeof(hmac_chba_system_params), 0, sizeof(hmac_chba_system_params));
    chba_vap_info->mac_vap_id = WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; /* 初始化vap id为无效值 */
    chba_vap_info->is_support_dfs_channel = CHBA_DFS_CHANNEL_SUPPORT_STATE;
    hmac_chba_set_sync_state(CHBA_INIT);
    system_params->schedule_period = init_params->config_para.schdule_period;
    system_params->sync_slot_cnt = init_params->config_para.discovery_slot;
    system_params->schedule_time_ms = MAC_CHBA_SLOT_DURATION * system_params->schedule_period;
    system_params->vap_alive_timeout = init_params->config_para.vap_alive_timeout;
    system_params->link_alive_timeout = init_params->config_para.link_alive_timeout;

    oam_warning_log2(0, 0, "hmac_config_chba_base_params::schedule_period[%d], shedule_period_ms[%d]",
        system_params->schedule_period, system_params->schedule_time_ms);
    return OAL_SUCC;
}


static void hmac_config_chba_vap_bitmap_level_init(hmac_vap_stru *hmac_vap, hmac_chba_vap_stru *chba_vap_info)
{
    /* 设置vap bitmap为最高档位 */
    chba_vap_info->vap_bitmap_level = CHBA_BITMAP_ALL_AWAKE_LEVEL;
    chba_vap_info->auto_bitmap_switch = CHBA_BITMAP_AUTO;

    /* 将初始化vap低功耗等级同步到dmac */
    hmac_config_send_event(&hmac_vap->st_vap_base_info, WLAN_CFGID_CHBA_SET_VAP_BITMAP_LEVEL, sizeof(uint8_t),
        &chba_vap_info->vap_bitmap_level);
}


static uint32_t hmac_config_chba_vap(mac_vap_stru *mac_vap, hmac_chba_vap_stru *chba_vap_info,
    mac_chba_param_stru *chba_params)
{
    hmac_vap_stru *hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (hmac_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 初始化mac地址，生成域BSSID */
    oal_set_mac_addr(mac_mib_get_StationID(mac_vap), chba_params->mac_addr);
    hmac_chba_vap_update_domain_bssid(hmac_vap, mac_mib_get_StationID(mac_vap));

    /* 初始化vap基本能力 */
    mac_vap->st_cap_flag.bit_wpa2 = OAL_TRUE;

    /* 配置chba_vap相关变量 */
    hmac_chba_set_sync_state(CHBA_NON_SYNC); /* 初始化为non-sync */
    hmac_chba_set_role(CHBA_OTHER_DEVICE);

    /* 设置vap bitmap为全醒态 */
    hmac_config_chba_vap_bitmap_level_init(hmac_vap, chba_vap_info);

    /* social channel根据国家码配置 */
    hmac_config_social_channel(chba_vap_info, chba_params->init_channel.uc_chan_number,
        chba_params->init_channel.en_bandwidth);
    /* 设置工作信道 */
    hmac_chba_config_channel(mac_vap, chba_vap_info, chba_params->init_channel.uc_chan_number,
        chba_params->init_channel.en_bandwidth);
    /* 将信道保存在work channel中 */
    chba_vap_info->work_channel = mac_vap->st_channel;
    /* chba维测日志默认关闭 */
    chba_vap_info->chba_log_level = CHBA_DEBUG_NO_LOG;

    /* 信道序列 bitmap 初始化为全1 */
    hmac_chba_set_channel_seq_bitmap(chba_vap_info, CHBA_CHANNEL_SEQ_BITMAP_100_PERCENT);
    /* 初始化信道序列 第二工作信道0 */
    memset_s(&(chba_vap_info->second_work_channel), sizeof(mac_channel_stru), 0, sizeof(mac_channel_stru));

    return OAL_SUCC;
}


uint32_t hmac_start_chba(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param)
{
    mac_chba_param_stru *chba_params = NULL;
    hmac_vap_stru *hmac_vap = NULL;
    hmac_chba_vap_stru *chba_vap_info = &g_chba_vap_info;

    if (oal_any_null_ptr2(mac_vap, param)) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    chba_params = (mac_chba_param_stru *)param;

    if (hmac_chba_get_sync_state() < CHBA_INIT) {
        oam_warning_log0(0, 0, "hmac_start_chba::fail to start chba vap beacause of uninitializing.");
        return OAL_FAIL;
    }

    if (hmac_chba_is_valid_channel(chba_params->init_channel.uc_chan_number) != OAL_SUCC) {
        OAM_WARNING_LOG1(0, 0, "hmac_start_chba::fail to start chba vap beacause of dfs channel[%d].",
            chba_params->init_channel.uc_chan_number);
        return OAL_FAIL;
    }

    chba_vap_info->mac_vap_id = mac_vap->uc_vap_id;
    /* 获取hmac_vap */
    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (hmac_vap == NULL) {
        oam_warning_log0(mac_vap->uc_vap_id, 0, "{hmac_start_chba::hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    hmac_vap->hmac_chba_vap_info = chba_vap_info;
    /* 初始化白名单 */
    hmac_chba_whitelist_clear(hmac_vap);
    /* 初始化chba vap info */
    hmac_config_chba_vap(mac_vap, chba_vap_info, chba_params);
    /* CHBA1.0 暂只支持单岛 */
    chba_vap_info->is_support_multiple_island = OAL_FALSE;
    hmac_chba_vap_start(mac_mib_get_StationID(mac_vap), &chba_vap_info->social_channel,
        &chba_vap_info->work_channel);

    /* 同步参数到dmac */
    hmac_chba_params_sync_after_start(mac_vap, chba_vap_info);

    return OAL_SUCC;
}


uint32_t hmac_stop_chba(mac_vap_stru *mac_vap)
{
    hmac_chba_vap_stru *chba_vap_info = &g_chba_vap_info;

    if (!mac_is_chba_mode(mac_vap)) {
        return OAL_SUCC;
    }

    /* 全局变量清空 */
    memset_s(chba_vap_info, sizeof(hmac_chba_vap_stru), 0, sizeof(hmac_chba_vap_stru));
    if (hmac_chba_get_sync_state() >= CHBA_INIT) {
        hmac_chba_set_sync_state(CHBA_INIT);
    }

    /* 通知HAL层 */
    hmac_chba_vap_stop();

    OAM_WARNING_LOG1(0, 0, "hmac_stop_chba::chba_state is %d now.", hmac_chba_get_sync_state());

    return OAL_SUCC;
}

static void hmac_chba_reset_device_mgmt_info(hmac_vap_stru *hmac_vap, uint8_t *own_addr)
{
    uint8_t device_id;
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();
    hmac_chba_per_device_id_info_stru *device_id_info = hmac_chba_get_device_id_info();

    /* 清空设备管理信息 */
    hmac_chba_clear_device_mgmt_info();

    /* 获取自身的device_id */
    device_id = hmac_chba_one_dev_update_alive_dev_table(own_addr);
    if (device_id >= MAC_CHBA_MAX_DEVICE_NUM) {
        oam_error_log2(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
            "{hmac_chba_reset_device_mgmt_info:device_id[%d], max_dev_num[%d]!}", device_id, MAC_CHBA_MAX_DEVICE_NUM);
        return;
    }

    /* 在设备管理信息中更新自身的信息 */
    self_conn_info->self_device_id = device_id;
    device_id_info[device_id].use_flag = TRUE;
    if (memcpy_s(device_id_info[device_id].mac_addr, sizeof(uint8_t) * OAL_MAC_ADDR_LEN, own_addr,
        sizeof(uint8_t) * OAL_MAC_ADDR_LEN) != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_chba_reset_device_mgmt_info: memcpy_s fail!}");
    }
}


void hmac_chba_cur_master_info_init(uint8_t *own_mac_addr, mac_channel_stru *work_channel)
{
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();

    if (own_mac_addr == NULL || work_channel == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CHBA, "{hmac_chba_cur_master_info_init:ptr is null!}");
        return;
    }

    sync_info->cur_master_info.master_rp = sync_info->own_rp_info;
    sync_info->cur_master_info.master_work_channel = work_channel->uc_chan_number;
    oal_set_mac_addr(sync_info->cur_master_info.master_addr, own_mac_addr);
}


static void hmac_chba_reset_sync_info(hmac_vap_stru *hmac_vap)
{
    /* 还原CHBA同步模块 */
    hmac_chba_sync_module_deinit();

    /* 设置为非同步状态 */
    hmac_chba_set_sync_state(CHBA_NON_SYNC);

    /* 设置为slave设备 */
    hmac_chba_set_role(CHBA_OTHER_DEVICE);

    /* 将当前Master信息还原为自身 */
    hmac_chba_cur_master_info_init(mac_mib_get_StationID(&hmac_vap->st_vap_base_info),
        &hmac_vap->st_vap_base_info.st_channel);
}


static void hmac_chba_reset_chba_vap_info(hmac_vap_stru *hmac_vap, hmac_chba_vap_stru *chba_vap_info)
{
    uint8_t auto_bitmap_switch_tmp = chba_vap_info->auto_bitmap_switch;
    uint8_t mac_vap_id_tmp = chba_vap_info->mac_vap_id;
    uint8_t is_support_multiple_island = chba_vap_info->is_support_multiple_island;
    mac_channel_stru social_channel_tmp = chba_vap_info->social_channel;

    /* 清空chba_vap_info */
    memset_s(chba_vap_info, sizeof(hmac_chba_vap_stru), 0, sizeof(hmac_chba_vap_stru));

    /* 还原基本配置 */
    chba_vap_info->auto_bitmap_switch = auto_bitmap_switch_tmp;
    chba_vap_info->mac_vap_id = mac_vap_id_tmp;
    chba_vap_info->social_channel = social_channel_tmp;
    chba_vap_info->work_channel = hmac_vap->st_vap_base_info.st_channel;
    chba_vap_info->is_support_multiple_island = is_support_multiple_island;
}


static void hmac_chba_vap_stop_schedule(hmac_vap_stru *hmac_vap)
{
    uint32_t ret;
    uint8_t *own_addr = NULL;
    hmac_chba_vap_stru *chba_vap_info = NULL;
    chba_params_config_stru chba_params_sync;

    chba_vap_info = hmac_get_up_chba_vap_info();
    if (chba_vap_info == NULL) {
        return;
    }

    own_addr = mac_mib_get_StationID(&hmac_vap->st_vap_base_info);
    if (own_addr == NULL) {
        return;
    }

    /* 还原同步相关标记位 */
    hmac_chba_reset_sync_info(hmac_vap);

    /* 还原设备管理信息 */
    hmac_chba_reset_device_mgmt_info(hmac_vap, own_addr);

    /* 将bssid还原为自身的mac addr */
    hmac_chba_generate_domain_bssid(hmac_chba_sync_get_domain_bssid(), own_addr, OAL_MAC_ADDR_LEN);

    /* 刷新beacon/pnf/assoc等帧的相关字段 */
    hmac_chba_save_update_beacon_pnf(hmac_chba_sync_get_domain_bssid());

    /* 重置hmac_chba_vap_stru数据结构 */
    hmac_chba_reset_chba_vap_info(hmac_vap, chba_vap_info);

    /* 将更新后的状态同步给dmac */
    memset_s(&chba_params_sync, sizeof(chba_params_sync), 0, sizeof(chba_params_sync));
    chba_params_sync.chba_role = hmac_chba_get_role();
    chba_params_sync.chba_state = hmac_chba_get_sync_state();
    oal_set_mac_addr(chba_params_sync.domain_bssid, hmac_chba_sync_get_domain_bssid());
    chba_params_sync.info_bitmap = CHBA_STATE | CHBA_ROLE | CHBA_BSSID | CHBA_BEACON_BUF |
        CHBA_BEACON_LEN | CHBA_PNF_BUF | CHBA_PNF_LEN | CHBA_MASTER_INFO;
    ret = hmac_chba_params_sync(&hmac_vap->st_vap_base_info, &chba_params_sync);
    if (ret != OAL_SUCC) {
        OAM_ERROR_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
            "{hmac_chba_vap_stop_schedule:fail to send params to dmac![%d]}", ret);
        return;
    }
    oam_warning_log0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
        "{hmac_chba_vap_stop_schedule:CHBA schedule stop!}");
}


void hmac_update_chba_vap_info(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user)
{
    hmac_chba_vap_stru *chba_vap_info = NULL;

    if (hmac_vap == NULL || hmac_user == NULL) {
        return;
    }

    chba_vap_info = hmac_chba_get_chba_vap_info(hmac_vap);
    if (chba_vap_info == NULL) {
        return;
    }

    /* 更新完拓扑信息之后，同步更新Beacon和PNF */
    // 等待user从链表中摘除后在更新beacon/PNF，否则link info仍携带该删链设备
    hmac_chba_save_update_beacon_pnf(hmac_chba_sync_get_domain_bssid());
    /* 删除链路时删除chba assoc_waiting_timer定时器 */
    if (hmac_user->chba_user.connect_info.assoc_waiting_timer.en_is_registerd == OAL_TRUE) {
        frw_immediate_destroy_timer(&(hmac_user->chba_user.connect_info.assoc_waiting_timer));
    }

    /* 无链路场景下暂停chba业务 */
    if (hmac_vap->st_vap_base_info.us_user_nums == 0) {
        hmac_chba_vap_stop_schedule(hmac_vap);
    }

    OAM_WARNING_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
        "{hmac_update_chba_vap_info:del chab user, link_up_user_cnt is[%d]}", hmac_vap->st_vap_base_info.us_user_nums);
}


uint32_t hmac_chba_whitelist_clear(hmac_vap_stru *hmac_vap)
{
    hmac_chba_vap_stru *chba_vap = NULL;

    if (hmac_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    chba_vap = (hmac_chba_vap_stru *)hmac_vap->hmac_chba_vap_info;
    if (chba_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    oal_set_mac_addr_zero(chba_vap->whitelist.peer_mac_addr);
    return OAL_SUCC;
}


uint32_t hmac_chba_whitelist_add_user(hmac_vap_stru *hmac_vap, uint8_t *peer_addr)
{
    hmac_chba_vap_stru *chba_vap = NULL;

    if (oal_any_null_ptr2(hmac_vap, peer_addr)) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    chba_vap = (hmac_chba_vap_stru *)hmac_vap->hmac_chba_vap_info;
    if (chba_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_set_mac_addr(chba_vap->whitelist.peer_mac_addr, peer_addr);
    oam_warning_log0(0, 0, "hmac_chba_whitelist_add_user::add peer into whitelist.");

    return OAL_SUCC;
}


uint32_t hmac_chba_whitelist_check(hmac_vap_stru *hmac_vap, uint8_t *peer_addr, uint8_t addr_len)
{
    hmac_chba_vap_stru *chba_vap = NULL;

    if (oal_any_null_ptr2(hmac_vap, peer_addr)) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    chba_vap = (hmac_chba_vap_stru *)hmac_vap->hmac_chba_vap_info;
    if (chba_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (oal_memcmp(chba_vap->whitelist.peer_mac_addr, peer_addr, OAL_MAC_ADDR_LEN) == 0) {
        oam_warning_log0(0, 0, "hmac_chba_whitelist_check::found peer in whitelist.");
        return OAL_SUCC;
    }
    oam_warning_log0(0, 0, "hmac_chba_whitelist_check::not found peer in whitelist.");
    return OAL_FAIL;
}


void hmac_chba_h2d_update_bssid(mac_vap_stru *mac_vap, uint8_t *domain_bssid, uint8_t domain_bssid_len)
{
    chba_params_config_stru chba_params_sync = { 0 };
    uint32_t ret;

    if (oal_any_null_ptr2(mac_vap, domain_bssid)) {
        return;
    }

    oal_set_mac_addr(chba_params_sync.domain_bssid, domain_bssid);
    chba_params_sync.info_bitmap = CHBA_BSSID;
    oam_warning_log4(0, 0, "hmac_chba_h2d_update_bssid::send bssid(XX:XX:XX:%02X:%02X:%02X) to dmac, bitmap 0x%X.",
        chba_params_sync.domain_bssid[3], chba_params_sync.domain_bssid[4],
        chba_params_sync.domain_bssid[5], chba_params_sync.info_bitmap);
    ret = hmac_chba_params_sync(mac_vap, &chba_params_sync);
    if (ret != OAL_SUCC) {
        return;
    }
}


uint32_t hmac_chba_request_sync(mac_vap_stru *mac_vap, hmac_chba_vap_stru *chba_vap_info,
    mac_chba_set_sync_request *sync_req_params)
{
    request_sync_info_stru *request_sync_info = NULL;
    sync_peer_info *peer_info = NULL;
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();
    uint8_t domain_bssid[WLAN_MAC_ADDR_LEN];
    int32_t err;

    if (oal_any_null_ptr3(mac_vap, chba_vap_info, sync_req_params)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    request_sync_info = &(sync_info->request_sync_info);
    peer_info = &sync_req_params->request_peer;
    err = (int32_t)mac_is_channel_num_valid(chba_vap_info->work_channel.en_band, peer_info->chan_num);
    /* 判断下发的参数是否有效 */
    if ((sync_req_params->mgmt_type != MAC_CHBA_SYNC_REQUEST) ||
        (sync_req_params->mgmt_buf_len == 0) || (err != OAL_SUCC)) {
        oam_warning_log3(0, 0,
            "hmac_chba_request_sync::invalid sync request, mgmt type[%d], buf len [%d], peer channel number [%d].",
            sync_req_params->mgmt_type, sync_req_params->mgmt_buf_len, peer_info->chan_num);
        return OAL_FAIL;
    }

    /* 停掉之前的sync request */
    if (sync_info->sync_request_timer.en_is_registerd == OAL_TRUE) {
        frw_immediate_destroy_timer(&(sync_info->sync_request_timer));
    }
    request_sync_info->request_cnt = 0;
    if (memcpy_s(request_sync_info->sync_req_buf, MAC_CHBA_MGMT_SHORT_PAYLOAD_BYTES,
        sync_req_params->mgmt_buf, sync_req_params->mgmt_buf_len) != EOK) {
        return OAL_FAIL;
    }
    request_sync_info->sync_req_buf_len = sync_req_params->mgmt_buf_len;
    oal_set_mac_addr(request_sync_info->peer_addr, peer_info->peer_addr);

    /* 生成新的域BSSID */
    hmac_generate_chba_domain_bssid(domain_bssid, WLAN_MAC_ADDR_LEN * sizeof(uint8_t),
        peer_info->master_addr, WLAN_MAC_ADDR_LEN * sizeof(uint8_t));
    
    mac_vap_set_bssid(mac_vap, domain_bssid, WLAN_MAC_ADDR_LEN);
    hmac_chba_h2d_update_bssid(mac_vap, domain_bssid, sizeof(domain_bssid));

    /* 切换信道到该peer所在的信道 */
    if (peer_info->chan_num != mac_vap->st_channel.uc_chan_number) {
        chba_vap_info->need_recovery_work_channel = OAL_TRUE;
        hmac_chba_config_channel(mac_vap, chba_vap_info, peer_info->chan_num, WLAN_BAND_WIDTH_20M);
        hmac_config_social_channel(chba_vap_info, peer_info->chan_num, WLAN_BAND_WIDTH_20M);
    }

    /* 封装并发送sync request action帧 */
    oam_warning_log3(0, 0, "hmac_chba_request_sync::send sync request action, peer addr is XX:XX:XXX:%02X:%02X:%02X.",
        peer_info->peer_addr[3], peer_info->peer_addr[4], peer_info->peer_addr[5]);
    hmac_send_chba_sync_request_action(mac_vap, chba_vap_info, peer_info->peer_addr,
        request_sync_info->sync_req_buf, request_sync_info->sync_req_buf_len);

    /* 启动Sync Waiting定时器 */
    frw_create_timer(&(sync_info->sync_request_timer),
        hmac_chba_sync_waiting_timeout, MAC_CHBA_SYNC_WAITING_TIMEOUT,
        chba_vap_info, OAL_FALSE, OAM_MODULE_ID_HMAC, mac_vap->ul_core_id);

    return OAL_SUCC;
}


uint32_t hmac_chba_sync_waiting_timeout(void *arg)
{
    hmac_chba_vap_stru *chba_vap_info = NULL;
    mac_vap_stru *mac_vap = NULL;
    request_sync_info_stru *request_sync_info = NULL;
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();
    uint32_t ret;

    chba_vap_info = (hmac_chba_vap_stru *)arg;
    if (chba_vap_info == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(chba_vap_info->mac_vap_id);
    if (mac_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    request_sync_info = &(sync_info->request_sync_info);

    request_sync_info->request_cnt++;
    /* 如果已经发送完三次，则退出 */
    if (request_sync_info->request_cnt >= MAC_CHBA_SYNC_REQUEST_CNT) {
        return OAL_SUCC;
    }
    /* 如果已经同步，则退出 */
    if ((hmac_chba_get_sync_state() > CHBA_NON_SYNC) &&
        (hmac_chba_get_sync_request_flag(&sync_info->sync_flags) == FALSE)) {
        oam_warning_log0(0, 0, "hmac_chba_sync_waiting_timeout::sync waitint timer timeout, but already sync.");
        return OAL_SUCC;
    }

    /* 再次发送sync request action帧 */
    oam_warning_log3(0, 0, "hmac_chba_sync_waiting_timeout::send sync req action, peer addr XX:XX:XX:%02X:%02X:%02X.",
        request_sync_info->peer_addr[3], request_sync_info->peer_addr[4], request_sync_info->peer_addr[5]);
    ret = hmac_send_chba_sync_request_action(mac_vap, chba_vap_info, request_sync_info->peer_addr,
        request_sync_info->sync_req_buf, request_sync_info->sync_req_buf_len);
    if (ret != OAL_SUCC) {
        return ret;
    }

    /* 启动Sync Waiting定时器 */
    frw_create_timer(&(sync_info->sync_request_timer),
        hmac_chba_sync_waiting_timeout, MAC_CHBA_SYNC_WAITING_TIMEOUT,
        chba_vap_info, OAL_FALSE, OAM_MODULE_ID_HMAC, mac_vap->ul_core_id);

    return OAL_SUCC;
}

uint32_t hmac_chba_d2h_sync_event(mac_vap_stru *mac_vap, uint8_t len, uint8_t *param)
{
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();
    hmac_vap_stru *hmac_vap = NULL;

    if (mac_vap == NULL || param == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (hmac_chba_vap_start_check(hmac_vap) == OAL_FALSE) {
        oam_warning_log0(mac_vap->uc_vap_id, OAM_SF_CHBA, "{hmac_chba_d2h_sync_event: CHBA vap is not start, return}");
        return OAL_FAIL;
    }

    /* 调度已停止, 不再处理上报 */
    if (mac_vap->us_user_nums == 0) {
        return OAL_SUCC;
    }

    switch (*param) {
        case CHBA_UPDATE_TOPO_INFO:
            hmac_chba_update_topo_info_proc(hmac_vap);
            break;
        case CHBA_NON_SYNC_START_MASTER_ELECTION:
            hmac_chba_sync_master_election_proc(hmac_vap, sync_info, MASTER_LOST);
            break;
        case CHBA_NON_SYNC_START_SYNC_REQ:
            hmac_chba_start_sync_request(sync_info);
            break;
        case CHBA_RP_CHANGE_START_MASTER_ELECTION:
            /* master选举信息发生变化直接进行master选举 */
            hmac_chba_sync_master_election_proc(hmac_vap, sync_info, MASTER_ALIVE);
            /* 更新最新的Beacon和RNF到Driver */
            hmac_chba_save_update_beacon_pnf(hmac_chba_sync_get_domain_bssid());
            break;
        case CHBA_DOMAIN_SEQARATE_START_MASTER_ELECTION:
            /* 域拆分引起的主设备选举 */
            hmac_chba_sync_master_election_proc(hmac_vap, sync_info, MASTER_LOST);
            /* 更新最新的Beacon和RNF到Driver */
            hmac_chba_save_update_beacon_pnf(hmac_chba_sync_get_domain_bssid());
            break;
        default:
            break;
    }

    return OAL_SUCC;
}

uint32_t hmac_chba_initiate_connect(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params)
{
    mac_cfg80211_connect_param_stru *connect_params = NULL;
    hmac_chba_conn_param_stru chba_conn_params;
    hmac_vap_stru *hmac_vap = NULL;
    hmac_user_stru *hmac_user = NULL;
    hmac_auth_req_stru auth_req;
    uint16_t user_idx;
    uint32_t ret;

    oam_warning_log0(0, 0, "hmac_chba_initiate_connect::initiate connect.");

    connect_params = (mac_cfg80211_connect_param_stru *)params;
    if (connect_params->ul_ie_len > WLAN_WPS_IE_MAX_SIZE) {
        OAM_ERROR_LOG1(0, 0, "hmac_chba_initiate_connect::invalid config, ie_len[%x] err!", connect_params->ul_ie_len);
        hmac_free_connect_param_resource(connect_params);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (hmac_vap == NULL) {
        hmac_free_connect_param_resource(connect_params);
        return OAL_FAIL;
    }

    hmac_config_connect_update_app_ie_to_vap(mac_vap, connect_params);

    /* 根据下发参数填写chba_conn_params */
    hmac_chba_fill_connect_params(params, &chba_conn_params, CHBA_CONN_INITIATOR);

    /* 根据下发的安全参数，配置vap */
    ret = hmac_chba_cfg_security_params(hmac_vap, connect_params);
    if (ret != OAL_SUCC) {
        hmac_free_connect_param_resource(connect_params);
        hmac_chba_fail_to_connect(hmac_vap, hmac_user, &chba_conn_params, CHBA_CONN_INITIATOR);
        OAM_WARNING_LOG1(0, 0, "hmac_chba_initiate_connect::fail to config security params[%d].", ret);
        return ret;
    }
    hmac_free_connect_param_resource(connect_params);

    /* 检查是否满足建链条件 */
    if (hmac_chba_connect_check(mac_vap, hmac_vap, &chba_conn_params) != OAL_SUCC ||
        hmac_chba_connect_prepare(mac_vap, &chba_conn_params, &user_idx, CHBA_CONN_INITIATOR) != OAL_SUCC) {
        hmac_chba_fail_to_connect(hmac_vap, hmac_user, &chba_conn_params, CHBA_CONN_INITIATOR);
        oam_warning_log0(0, 0, "hmac_chba_initiate_connect::fail to connect");
        return OAL_FAIL;
    }

    /* 在建链准备完成之后，不允许再使用hmac_chba_fail_to_connect来上报失败 */
    hmac_user = mac_res_get_hmac_user(user_idx);
    if (hmac_user == NULL) {
        return OAL_FAIL;
    }

    /* 设置ssid 到chba user. external_auth上报到wpa_s和external_auth下发时使用 */
    hmac_chba_user_set_ssid(hmac_user, connect_params->auc_ssid, connect_params->uc_ssid_len);

    hmac_chba_connect_initiator_fsm_change_state(hmac_user, CHBA_USER_STATE_JOIN_COMPLETE);
    hmac_prepare_auth_req(hmac_vap, &auth_req);
    /* 调用函数 hmac_chba_wait_auth */
    hmac_fsm_call_func_chba_conn_initiator(hmac_vap, hmac_user, HMAC_FSM_INPUT_AUTH_REQ, (void *)&auth_req);

    return OAL_SUCC;
}


uint32_t hmac_chba_response_connect(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params)
{
    hmac_chba_conn_param_stru chba_conn_params;
    hmac_vap_stru *hmac_vap = NULL;
    hmac_user_stru *hmac_user = NULL;
    uint16_t user_idx;
    uint32_t ret;

    if (oal_any_null_ptr2(mac_vap, params)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (hmac_vap == NULL) {
        return OAL_FAIL;
    }

    /* 根据下发参数填写chba_conn_params */
    ret = hmac_chba_fill_connect_params(params, &chba_conn_params, CHBA_CONN_RESPONDER);
    if (ret != OAL_SUCC) {
        hmac_chba_fail_to_connect(hmac_vap, hmac_user, &chba_conn_params, CHBA_CONN_RESPONDER);
        return OAL_FAIL;
    }

    /* 检查是否满足建链条件 */
    ret = hmac_chba_connect_check(mac_vap, hmac_vap, &chba_conn_params);
    if (ret != OAL_SUCC) {
        hmac_chba_fail_to_connect(hmac_vap, hmac_user, &chba_conn_params, CHBA_CONN_RESPONDER);
        return OAL_FAIL;
    }

    /* 建链准备 */
    ret = hmac_chba_connect_prepare(mac_vap, &chba_conn_params, &user_idx, CHBA_CONN_RESPONDER);
    if (ret != OAL_SUCC) {
        hmac_chba_fail_to_connect(hmac_vap, hmac_user, &chba_conn_params, CHBA_CONN_RESPONDER);
        return ret;
    }

    /* 此时user应该已经创建好了 */
    hmac_user = mac_res_get_hmac_user(user_idx);
    if (hmac_user == NULL) {
        OAM_WARNING_LOG1(0, 0, "hmac_chba_response_connect::hmac user is null, user_idx[%d]", user_idx);
        return OAL_FAIL;
    }
    hmac_user->chba_user.connect_info.assoc_waiting_time = chba_conn_params.connect_timeout * 1000;

    /* 通知上层ready to connect */
    chba_conn_params.status_code = MAC_SUCCESSFUL_STATUSCODE;
    hmac_chba_ready_to_connect(hmac_vap, &chba_conn_params);

    /* 启动assoc waiting定时器 */
    frw_create_timer(&(hmac_user->chba_user.connect_info.assoc_waiting_timer),
        hmac_chba_assoc_waiting_timeout_responder, hmac_user->chba_user.connect_info.assoc_waiting_time,
        hmac_user, OAL_FALSE, OAM_MODULE_ID_HMAC, hmac_vap->st_vap_base_info.ul_core_id);

    return OAL_SUCC;
}


uint32_t hmac_chba_fill_connect_params(uint8_t *params, hmac_chba_conn_param_stru *chba_conn_params,
    uint8_t connect_role)
{
    mac_cfg80211_connect_param_stru *connect_params = NULL;
    hmac_chba_conn_notify_stru *notify_params = NULL;
    uint32_t ret;

    if (oal_any_null_ptr2(params, chba_conn_params)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (connect_role == CHBA_CONN_INITIATOR) {
        connect_params = (mac_cfg80211_connect_param_stru *)params;
        chba_conn_params->status_code = MAC_CHBA_INIT_CODE;

        oal_set_mac_addr(chba_conn_params->peer_addr, connect_params->auc_bssid);
        return OAL_SUCC;
    }

    notify_params = (hmac_chba_conn_notify_stru *)params;
    chba_conn_params->op_id = notify_params->id;
    chba_conn_params->status_code = MAC_CHBA_INIT_CODE;

    oal_set_mac_addr(chba_conn_params->peer_addr, notify_params->peer_mac_addr);
    ret = hmac_chba_chan_convert_center_freq_to_bw(&(notify_params->channel),
        &(chba_conn_params->assoc_channel));
    if (ret != OAL_SUCC) {
        chba_conn_params->status_code = MAC_CHBA_INVAILD_CONNECT_CMD;
        return OAL_FAIL;
    }
    chba_conn_params->connect_timeout = notify_params->expire_time;
    oam_warning_log3(0, 0, "hmac_chba_fill_connect_params::responder, peer_addr XX:XX:XX:%02X:%02X:%02X",
        chba_conn_params->peer_addr[MAC_ADDR_3], chba_conn_params->peer_addr[MAC_ADDR_4],
        chba_conn_params->peer_addr[MAC_ADDR_5]);

    return OAL_SUCC;
}


static uint32_t hmac_chba_connect_check(mac_vap_stru *mac_vap, hmac_vap_stru *hmac_vap,
    hmac_chba_conn_param_stru *chba_conn_params)
{
    hmac_chba_vap_stru *chba_vap_info = NULL;

    if (oal_any_null_ptr3(mac_vap, hmac_vap, chba_conn_params)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    chba_vap_info = hmac_vap->hmac_chba_vap_info;
    if (chba_vap_info == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 检查chba vap链路数是否达到上限 */
    if (mac_vap->us_user_nums >= MAC_CHBA_MAX_LINK_NUM) {
        chba_conn_params->status_code = MAC_CHBA_CREATE_NEW_USER_FAIL;
        return OAL_FAIL;
    }

    /* 并行建链存在较多问题，暂不接受并行建链 */
    if (hmac_vap_is_connecting(mac_vap)) {
        chba_conn_params->status_code = MAC_CHBA_UNSUP_PARALLEL_CONNECT;
        return OAL_FAIL;
    }

    return OAL_SUCC;
}

static uint32_t hmac_chba_cfg_security_params(hmac_vap_stru *hmac_vap, mac_cfg80211_connect_param_stru *connect_params)
{
    mac_cfg80211_connect_security_stru conn_sec;
    uint32_t ret;

    memset_s(&conn_sec, sizeof(mac_cfg80211_connect_security_stru), 0, sizeof(mac_cfg80211_connect_security_stru));
    conn_sec.uc_wep_key_len = connect_params->uc_wep_key_len;
    conn_sec.en_privacy = connect_params->en_privacy;
    conn_sec.st_crypto = connect_params->st_crypto;

    conn_sec.en_mgmt_proteced = connect_params->en_mfp;
#ifdef _PRE_WLAN_FEATURE_PMF
    conn_sec.en_pmf_cap = mac_get_pmf_cap(connect_params->puc_ie, connect_params->ul_ie_len);
#endif
    conn_sec.en_wps_enable = OAL_FALSE;

    hmac_vap->en_auth_mode = connect_params->en_auth_type;
    OAM_WARNING_LOG1(0, OAM_SF_CHBA, "hmac_chba_cfg_security_params: connect auth_type %d",
        connect_params->en_auth_type);

    ret = mac_vap_init_privacy(&(hmac_vap->st_vap_base_info), &conn_sec);
    if (ret != OAL_SUCC) {
        OAM_ERROR_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CHBA,
            "hmac_chba_cfg_security_params::mac_vap_init_privacy fail[%d]!", ret);
        return ret;
    }
    /* 同步加密参数到dmac */
    ret = hmac_config_send_event(&(hmac_vap->st_vap_base_info), WLAN_CFGID_CONNECT_REQ,
        sizeof(conn_sec), (uint8_t *)&conn_sec);
    if (oal_unlikely(ret != OAL_SUCC)) {
        OAM_ERROR_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CHBA,
            "hmac_chba_cfg_security_params: hmac_config_send_event failed[%d].", ret);
        return ret;
    }
    return OAL_SUCC;
}


uint32_t hmac_chba_connect_prepare(mac_vap_stru *mac_vap, hmac_chba_conn_param_stru *chba_conn_params,
    uint16_t *user_idx, uint8_t connect_role)
{
    uint32_t ret;
    uint8_t connect_prepare_params = 0;
    hmac_vap_stru *hmac_vap = NULL;
    hmac_chba_vap_stru *chba_vap_info = NULL;
    /* 创建user，并将状态设置为wait assoc */
    ret = hmac_chba_user_mgmt_conn_prepare(mac_vap, chba_conn_params, user_idx, connect_role);
    if (ret != OAL_SUCC) {
        return ret;
    }
    /* 如果是建链响应者，则将peer addr添加到白名单 */
    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    if (hmac_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    chba_vap_info = hmac_vap->hmac_chba_vap_info;
    if (chba_vap_info == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 告知dmac准备建链 唤醒device开前端 */
    hmac_config_send_event(mac_vap, WLAN_CFGID_CHBA_CONNECT_PREPARE, sizeof(uint8_t), &connect_prepare_params);

    hmac_chba_whitelist_add_user(hmac_vap, chba_conn_params->peer_addr);

    /* 设置vap低功耗为最高档 */
    chba_vap_info->vap_bitmap_level = CHBA_BITMAP_ALL_AWAKE_LEVEL;
    hmac_chba_sync_vap_bitmap_info(&(hmac_vap->st_vap_base_info), chba_vap_info);
    /* 切换到建链信道 */
    /* 如果未同步，直接切换，如果已经同步，调用vap bitmap配置函数 */
    oam_warning_log3(mac_vap->uc_vap_id, OAM_SF_CHBA,
        "hmac_chba_connect_prepare::connect channel is [%d], bandwidth is [%d], connect_role[%d].",
        chba_conn_params->assoc_channel.uc_chan_number, chba_conn_params->assoc_channel.en_bandwidth, connect_role);

    /* 入网阶段, sleep work超时延长至400ms */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    wlan_pm_set_timeout(WLAN_SLEEP_LONG_CHECK_CNT);
#endif
    return OAL_SUCC;
}


uint32_t hmac_chba_connect_chan_switch_proc(mac_vap_stru *mac_vap, hmac_chba_conn_param_stru *chba_conn_params)
{
    return OAL_SUCC;
}


uint32_t hmac_chba_config_channel(mac_vap_stru *mac_vap, hmac_chba_vap_stru *chba_vap_info,
    uint8_t channel_number, uint8_t bandwidth)
{
    chba_h2d_channel_cfg_stru channel_cfg = { 0 };
    mac_channel_stru mac_channel = { 0 };
    uint32_t ret;

    if (mac_vap == NULL || chba_vap_info == NULL) {
        return OAL_FAIL;
    }

    mac_channel.uc_chan_number = channel_number;
    mac_channel.en_bandwidth = bandwidth;
    mac_channel.en_band = mac_get_band_by_channel_num(mac_channel.uc_chan_number);
    ret = mac_get_channel_idx_from_num(mac_channel.en_band, mac_channel.uc_chan_number, &(mac_channel.uc_idx));
    if (ret != OAL_SUCC) {
        return ret;
    }

    /* 使用vap信道配置接口 */
    channel_cfg.chan_cfg_type_bitmap = CHBA_CHANNEL_CFG_BITMAP_WORK;
    channel_cfg.channel = mac_channel;
    ret = hmac_config_send_event(mac_vap, WLAN_CFGID_CHBA_SET_CHANNEL, sizeof(chba_h2d_channel_cfg_stru),
        (uint8_t *)&channel_cfg);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, 0, "hmac_chba_config_channel::config vap work channel err[%u]", ret);
        return ret;
    }

    oam_warning_log2(0, 0, "{hmac_chba_config_channel::cur channel %d, switch to channel %d}",
        mac_vap->st_channel.uc_chan_number, channel_number);
    return OAL_SUCC;
}


void hmac_chba_ready_to_connect(hmac_vap_stru *hmac_vap, hmac_chba_conn_param_stru *conn_param)
{
    if (oal_any_null_ptr2(hmac_vap, conn_param)) {
        return;
    }

    oal_cfg80211_connect_ready_report(hmac_vap->pst_net_device, conn_param->peer_addr,
        conn_param->op_id, conn_param->status_code, GFP_ATOMIC);
}


static void hmac_chba_fail_to_connect(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user,
    hmac_chba_conn_param_stru *conn_param, uint8_t connect_role)
{
    mac_status_code_enum_uint16 reason_code;
    hmac_chba_vap_stru *chba_vap_info = NULL;

    /* hmac_user有可能为空，在后续逻辑中判断处理 */
    if (oal_any_null_ptr2(hmac_vap, conn_param)) {
        return;
    }

    reason_code = conn_param->status_code;
    OAM_WARNING_LOG1(0, 0, "hmac_chba_fail_to_connect::fail to connect, reason code[%d].", reason_code);

    chba_vap_info = hmac_vap->hmac_chba_vap_info;
    if (chba_vap_info == NULL) {
        return;
    }

    /* 如果角色是CHBA_CONN_INITIATOR，延用原接口上报建链失败 */
    if (connect_role == CHBA_CONN_INITIATOR) {
        hmac_report_connect_failed_result(hmac_vap, reason_code, conn_param->peer_addr);
    } else {
        /* 填写上报结果，并调用接口上报建链准备失败 */
        oal_cfg80211_connect_ready_report(hmac_vap->pst_net_device, conn_param->peer_addr,
            conn_param->op_id, conn_param->status_code, GFP_ATOMIC);
    }

    /* 如果用户已经创建，删除用户 */
    if (hmac_user != NULL) {
        hmac_user_del(&hmac_vap->st_vap_base_info, hmac_user);
    }
}


static void hmac_chba_master_election_after_asoc(hmac_vap_stru *hmac_vap, uint8_t *sa_addr,
    chba_req_sync_after_assoc_stru *req_sync)
{
    hmac_chba_sync_info *sync_info = hmac_chba_get_sync_info();
    hmac_chba_vap_stru *chba_vap_info = hmac_vap->hmac_chba_vap_info;
    master_info own_rp_info;
    master_info peer_rp_info;

    /* 1.获取对端RP值信息 */
    peer_rp_info.master_rp = req_sync->peer_rp;
    oal_set_mac_addr(peer_rp_info.master_addr, sa_addr);
    peer_rp_info.master_work_channel = chba_vap_info->work_channel.uc_chan_number;

    /* 2.获取自身RP值信息 */
    own_rp_info.master_rp = sync_info->own_rp_info;
    oal_set_mac_addr(own_rp_info.master_addr, mac_mib_get_StationID(&hmac_vap->st_vap_base_info));
    own_rp_info.master_work_channel = chba_vap_info->work_channel.uc_chan_number;

    /* 3.RP值比较，自身RP值较大, 成为Master */
    if (hmac_chba_sync_dev_rp_compare(&own_rp_info, &peer_rp_info) == OAL_TRUE) {
        hmac_chba_sync_become_master_handler(sync_info, FALSE);
        oam_warning_log0(0, OAM_SF_CHBA, "{hmac_chba_master_election_after_asoc:become master!}");
        return;
    }

    /* 4.自身RP值较小, 以建链对端为Master，申请向建链对端同步 */
    /* 更新自身Master信息 */
    hmac_chba_set_role(CHBA_OTHER_DEVICE);
    sync_info->cur_master_info = peer_rp_info;
    /* 准备向对端同步 */
    hmac_chba_ask_for_sync(peer_rp_info.master_work_channel, sa_addr, peer_rp_info.master_addr);

    oam_warning_log3(0, OAM_SF_CHBA,
        "{hmac_chba_master_election_after_asoc:become slave! Master is XX:XX:XX:%02x:%02x:%02x}",
        peer_rp_info.master_addr[3], peer_rp_info.master_addr[4], peer_rp_info.master_addr[5]);
}

#ifdef _PRE_WLAN_FEATURE_STA_PM

oal_uint32 hmac_set_chba_ipaddr_timeout(void *para)
{
    if (hmac_config_get_ps_mode() == MAX_FAST_PS) {
        wlan_pm_set_timeout(wlan_pm_get_fast_check_cnt());
    } else {
        wlan_pm_set_timeout(WLAN_SLEEP_DEFAULT_CHECK_CNT);
    }
    return OAL_SUCC;
}
#endif


void hmac_chba_sync_process_after_asoc(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user)
{
    chba_req_sync_after_assoc_stru *req_sync = NULL;
    hmac_chba_vap_stru *chba_vap_info = NULL;

    if (hmac_vap == NULL || hmac_user == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CHBA, "{hmac_chba_sync_process_after_asoc:ptr is NULL!}");
        return;
    }

    /* 非CHBA vap无需进行处理 */
    if (hmac_chba_vap_start_check(hmac_vap) != OAL_TRUE) {
        return;
    }

    req_sync = &hmac_user->chba_user.req_sync;
    chba_vap_info = hmac_vap->hmac_chba_vap_info;

    /* 不需要进行同步 */
    if (req_sync->need_sync_flag != OAL_TRUE) {
        return;
    }

    /* 同步标志位还原 */
    req_sync->need_sync_flag = OAL_FALSE;

    
    hmac_chba_add_conn_proc(hmac_vap, hmac_user);

    /* 1、如果自己已经同步，但对方尚未同步，直接返回 */
    if (hmac_chba_get_sync_state() > CHBA_NON_SYNC && req_sync->peer_sync_state == MAC_CHBA_DECLARE_NON_SYNC) {
        oam_warning_log0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CHBA,
            "hmac_chba_sync_process_after_asoc::already sync, return.");
        return;
    }

    /* 2、如果自己尚未同步，而对方已经同步，将对方信息填入Request List，并调用Request List函数 */
    if (hmac_chba_get_sync_state() == CHBA_NON_SYNC && req_sync->peer_sync_state > MAC_CHBA_DECLARE_NON_SYNC) {
        oam_warning_log0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CHBA,
            "hmac_chba_sync_process_after_asoc::peer device is already sync, try to sync with it.");
        hmac_chba_ask_for_sync(chba_vap_info->work_channel.uc_chan_number,
            hmac_user->st_user_base_info.auc_user_mac_addr, req_sync->peer_master_addr);
        return;
    }

    /* 3、如果都未同步，则进行Master选举 */
    if (hmac_chba_get_sync_state() == CHBA_NON_SYNC && req_sync->peer_sync_state == MAC_CHBA_DECLARE_NON_SYNC) {
        oam_warning_log0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CHBA,
            "hmac_chba_sync_process_after_asoc::nobody sync, start Master elec!");
        hmac_chba_master_election_after_asoc(hmac_vap, hmac_user->st_user_base_info.auc_user_mac_addr, req_sync);
        return;
    }

    /* 4、如果都同步，且master一致,无需处理 */
    if (oal_compare_mac_addr(req_sync->peer_master_addr, hmac_chba_get_master_mac_addr()) == 0) {
        oam_warning_log0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CHBA,
            "hmac_chba_sync_process_after_asoc::both of us are sync with the same master.");
        return;
    }
    /* 5、如果都已经同步但不是同一个域,则进行域合并 */
    oam_warning_log0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CHBA,
        "hmac_chba_sync_process_after_asoc::sync with the different master.");
    hmac_chba_multiple_domain_detection_after_asoc(hmac_user->st_user_base_info.auc_user_mac_addr, req_sync);
}


static void hmac_chba_save_sync_info_after_assoc(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user,
    hmac_chba_vap_stru *chba_vap_info, uint8_t *master_attr)
{
    int32_t ret;
    chba_req_sync_after_assoc_stru *req_sync = &hmac_user->chba_user.req_sync;

    /* 获取对端同步状态 */
    req_sync->peer_sync_state = master_attr[MAC_CHBA_ATTR_HDR_LEN];
    /* 获取对端Master RP */
    ret = memcpy_s((uint8_t *)&req_sync->peer_master_rp, sizeof(ranking_priority),
        master_attr + MAC_CHBA_MASTER_RANKING_LEVEL_POS, sizeof(ranking_priority));
    if (ret != EOK) {
        OAM_ERROR_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CHBA,
            "{hmac_chba_save_sync_info_after_assoc:memcpy failed, ret[%d]}", ret);
        return;
    }
    /* 获取对端自身 RP */
    ret = memcpy_s((uint8_t *)&req_sync->peer_rp, sizeof(ranking_priority),
        master_attr + MAC_CHBA_RANKING_LEVEL_POS, sizeof(ranking_priority));
    if (ret != EOK) {
        OAM_ERROR_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CHBA,
            "{hmac_chba_save_sync_info_after_assoc:memcpy failed, ret[%d]}", ret);
        return;
    }
    /* 获取对端Master mac地址 */
    oal_set_mac_addr(req_sync->peer_master_addr, master_attr + MAC_CHBA_MASTER_ADDR_POS);
    /* 获取对端工作信道 */
    req_sync->peer_work_channel = chba_vap_info->work_channel.uc_chan_number;

    /* need_sync_flag置位, 在eapol结束后启动sync流程 */
    req_sync->need_sync_flag = OAL_TRUE;
}


void hmac_chba_connect_succ_handler(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user,
    uint8_t *payload, uint16_t payload_len)
{
    uint8_t *master_attr = NULL;
    uint8_t *chba_ie = NULL;
    hmac_chba_vap_stru *chba_vap_info = hmac_vap->hmac_chba_vap_info;
    if ((mac_is_chba_mode(&hmac_vap->st_vap_base_info) == OAL_FALSE) || (chba_vap_info == NULL)) {
        return;
    }

    /* 关联成功后，停止关联超时定时器 */
    if (hmac_user->chba_user.connect_info.assoc_waiting_timer.en_is_registerd == OAL_TRUE) {
        frw_immediate_destroy_timer(&(hmac_user->chba_user.connect_info.assoc_waiting_timer));
    }

    /* 更新CHBA用户状态为关联UP */
    hmac_chba_user_set_connect_role(hmac_user, CHBA_CONN_ROLE_BUTT);
    hmac_chba_connect_initiator_fsm_change_state(hmac_user, CHBA_USER_STATE_LINK_UP);

    /* 配置该user的bitmap */
    hmac_chba_update_work_channel(&hmac_vap->st_vap_base_info);

    /* 从白名单中删除 */
    hmac_chba_whitelist_clear(hmac_vap);
    /* 查找Master选举属性 */
    /* assoc req/rsp 帧体中查找CHBA IE */
    chba_ie = mac_find_vendor_ie(MAC_CHBA_VENDOR_IE, MAC_OUI_TYPE_CHBA, payload, payload_len);
    if (chba_ie == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CHBA, "hmac_chba_connect_succ_handler: can't find chba ie in assoc req frame.");
        return;
    }
    payload = chba_ie + sizeof(mac_ieee80211_vendor_ie_stru);
    payload_len = payload_len - sizeof(mac_ieee80211_vendor_ie_stru);
    master_attr = hmac_find_chba_attr(MAC_CHBA_ATTR_MASTER_ELECTION, payload, payload_len);
    if (master_attr == NULL) {
        oam_warning_log0(0, OAM_SF_CHBA,
            "hmac_chba_connect_succ_handler::can't find master election attr in assoc req frame.");
        return;
    }

    if (MAC_CHBA_ATTR_HDR_LEN + master_attr[MAC_CHBA_ATTR_ID_LEN] < MAC_CHBA_MASTER_ELECTION_ATTR_LEN) {
        OAM_ERROR_LOG1(0, OAM_SF_CHBA, "hmac_chba_connect_succ_handler::master election attr len[%d] invalid.",
            master_attr[MAC_CHBA_ATTR_ID_LEN]);
        return;
    }
    hmac_chba_save_sync_info_after_assoc(hmac_vap, hmac_user, chba_vap_info, master_attr);

#ifdef _PRE_WLAN_FEATURE_STA_PM
    /* 获取IP超时时, 配置低功耗开关 */
    frw_create_timer(&(hmac_vap->st_ps_sw_timer), hmac_set_chba_ipaddr_timeout, CHBA_SWITCH_STA_PSM_PERIOD,
        (void *)hmac_vap, OAL_FALSE, OAM_MODULE_ID_HMAC, hmac_vap->st_vap_base_info.ul_core_id);
    hmac_vap->ul_check_timer_pause_cnt = 0;
#endif

    oam_warning_log0(0, OAM_SF_CHBA, "{hmac_chba_connect_succ_handler:connect succ}");
}


static void hmac_chba_connect_fail_handler(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user,
    uint8_t connect_role)
{
    hmac_chba_user_stru *hmac_chba_user = NULL;
    chba_connect_status_stru *connect_info = NULL;
    hmac_chba_vap_stru *chba_vap_info = hmac_vap->hmac_chba_vap_info;
    if (chba_vap_info == NULL) {
        return;
    }

    hmac_chba_user = &(hmac_user->chba_user);
    connect_info = &(hmac_chba_user->connect_info);

    /* 如果connect_role为CHBA_CONN_INITIATOR，上报WAPS建链失败; connect_role为CHBA_CONN_RESPONDER，无需上报建链失败 */
    if (connect_role == CHBA_CONN_INITIATOR) {
        hmac_report_connect_failed_result(hmac_vap, connect_info->status_code,
            hmac_user->st_user_base_info.auc_user_mac_addr);
    }

    /* 删除用户 */
    hmac_user_del(&hmac_vap->st_vap_base_info, hmac_user);
}

uint32_t hmac_chba_assoc_waiting_timeout_responder(void *arg)
{
    hmac_vap_stru *hmac_vap = NULL;
    hmac_user_stru *hmac_user = NULL;

    hmac_user = (hmac_user_stru *)arg;
    if (hmac_user == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(hmac_user->st_user_base_info.uc_vap_id);
    if (hmac_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* responder不上报建链失败，只恢复建链前的状态 */
    oam_warning_log0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CHBA,
        "hmac_chba_assoc_waiting_timeout_responder::responder assoc failed, restore.");
    hmac_chba_connect_fail_handler(hmac_vap, hmac_user, CHBA_CONN_RESPONDER);
    return OAL_SUCC;
}

static uint32_t hmac_config_social_channel_params(oal_uint8 channel_number, uint8_t bandwidth)
{
    uint8_t chan_idx;
    uint32_t ret;
    wlan_channel_band_enum_uint8 band;
    wlan_bw_cap_enum_uint8 bw_cap = WLAN_BW_CAP_20M;

    hmac_chba_vap_stru *chba_vap_info = &g_chba_vap_info;
    mac_channel_stru *social_channel = &chba_vap_info->social_channel;

    band = mac_get_band_by_channel_num(channel_number);
    if (band == WLAN_BAND_2G) {
        OAM_ERROR_LOG1(0, 0, "hmac_config_social_channel::social chan[%d]", channel_number);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    ret = mac_get_channel_idx_from_num(band, channel_number, &chan_idx);
    if (ret != OAL_SUCC) {
        OAM_ERROR_LOG1(0, 0, "hmac_config_social_channel::config social channel err[%u]", ret);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    if (bandwidth >= WLAN_BAND_WIDTH_80PLUSPLUS) {
        bw_cap = WLAN_BW_CAP_80M;
    } else if (bandwidth >= WLAN_BAND_WIDTH_40PLUS) {
        bw_cap = WLAN_BW_CAP_40M;
    } else if (bandwidth == WLAN_BAND_WIDTH_20M) {
        bw_cap = WLAN_BW_CAP_20M;
    }

    bandwidth = mac_regdomain_get_bw_by_channel_bw_cap(channel_number, bw_cap);
    social_channel->uc_chan_number = channel_number;
    social_channel->en_band = band;
    social_channel->uc_idx = chan_idx;
    social_channel->en_bandwidth = bandwidth;

    return OAL_SUCC;
}


void hmac_config_social_channel(hmac_chba_vap_stru *chba_vap_info, uint8_t channel_number, uint8_t bandwidth)
{
    hmac_vap_stru *hmac_vap = NULL;
    uint8_t chan_temp;

    if (chba_vap_info == NULL) {
        return;
    }

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(chba_vap_info->mac_vap_id);
    if (hmac_vap == NULL) {
        return;
    }

    /* 根据定制化使用固定的social channel */
    if (hwifi_get_chba_social_chan_cap()) {
        chan_temp = hwifi_get_chba_social_channel();
        if (chan_temp == 0) {
            hmac_config_social_channel_params(CHBA_INIT_SOCIAL_CHANNEL, WLAN_BAND_WIDTH_80PLUSPLUS);
        } else if (hmac_config_social_channel_params(chan_temp, bandwidth) != OAL_SUCC) {
            hmac_config_social_channel_params(CHBA_INIT_SOCIAL_CHANNEL, WLAN_BAND_WIDTH_80PLUSPLUS);
        }
    } else {
        /* social channel跟随work channel */
        if (hmac_config_social_channel_params(channel_number, bandwidth) != OAL_SUCC) {
            return;
        }
    }

    OAM_WARNING_LOG1(0, 0, "hmac_config_social_channel::social channel switch to [%d]",
        chba_vap_info->social_channel.uc_chan_number);
    hmac_config_vap_discovery_channel(&hmac_vap->st_vap_base_info, chba_vap_info);
}


void hmac_generate_chba_domain_bssid(uint8_t *bssid, uint8_t bssid_len,
    uint8_t *mac_addr, uint8_t mac_addr_len)
{
    if (memcpy_s(bssid, bssid_len, mac_addr, mac_addr_len) != EOK) {
        oam_warning_log0(0, 0, "hmac_generate_chba_domain_bssid: memcpy_s failed.");
        return;
    }
    bssid[0] = 0x00;
    bssid[1] = 0xE0;
    bssid[2] = 0xFC;
}


void hmac_chba_vap_update_domain_bssid(hmac_vap_stru *hmac_vap, uint8_t *mac_addr)
{
    mac_vap_stru *mac_vap = NULL;

    if ((hmac_vap == NULL) || (mac_addr == NULL)) {
        oam_warning_log0(0, OAM_SF_ANY, "{hmac_chba_vap_update_domain_bssid: ptr is NULL}");
        return;
    }
    mac_vap = &hmac_vap->st_vap_base_info;

    /* 更新mac_vap下bssid */
    hmac_generate_chba_domain_bssid(mac_vap->auc_bssid, WLAN_MAC_ADDR_LEN * sizeof(uint8_t),
        mac_addr, WLAN_MAC_ADDR_LEN * sizeof(uint8_t));
}
static uint32_t hmac_chba_send_chba_topo_info(mac_vap_stru *mac_vap, chba_h2d_topo_info_stru *topo_info)
{
    dmac_tx_event_stru *tx_event = NULL;
    frw_event_mem_stru *event_mem = NULL;
    oal_netbuf_stru *cmd_netbuf = NULL;
    frw_event_stru *hmac_to_dmac_ctx_event = NULL;
    uint32_t ret;
    int32_t err;

    cmd_netbuf = oal_mem_netbuf_alloc(OAL_NORMAL_NETBUF, sizeof(chba_h2d_topo_info_stru), OAL_NETBUF_PRIORITY_MID);
    if (cmd_netbuf == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CHBA, "hmac_chba_send_chba_topo_info::netbuf alloc null!");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* 拷贝命令结构体到netbuf */
    err = memcpy_s(oal_netbuf_data(cmd_netbuf), sizeof(chba_h2d_topo_info_stru),
        topo_info, sizeof(chba_h2d_topo_info_stru));
    oal_netbuf_put(cmd_netbuf, sizeof(chba_h2d_topo_info_stru));
    if (err != EOK) {
        OAM_ERROR_LOG0(0, OAM_SF_CHBA, "hmac_chba_send_chba_topo_info::memcpy fail!");
        oal_netbuf_free(cmd_netbuf);
        return OAL_FAIL;
    }
    /* 抛事件到DMAC */
    event_mem = frw_event_alloc_m(sizeof(dmac_tx_event_stru));
    if (event_mem == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CHBA, "hmac_chba_send_chba_topo_info::event_mem null.");
        oal_netbuf_free(cmd_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }
    hmac_to_dmac_ctx_event = (frw_event_stru *)event_mem->puc_data;
    frw_event_hdr_init(&(hmac_to_dmac_ctx_event->st_event_hdr), FRW_EVENT_TYPE_WLAN_CTX,
        DMAC_WLAN_CTX_EVENT_SUB_TYPE_CHBA_TOPO_INFO_SYNC, sizeof(dmac_tx_event_stru), FRW_EVENT_PIPELINE_STAGE_1,
        mac_vap->uc_chip_id, mac_vap->uc_device_id, mac_vap->uc_vap_id);

    tx_event = (dmac_tx_event_stru *)(hmac_to_dmac_ctx_event->auc_event_data);
    tx_event->pst_netbuf = cmd_netbuf;
    tx_event->us_frame_len = oal_netbuf_len(cmd_netbuf);

    ret = frw_event_dispatch_event(event_mem);
    if (ret != OAL_SUCC) {
        OAM_ERROR_LOG1(0, OAM_SF_CHBA, "hmac_chba_params_sync::dispatch_event fail[%d]!", ret);
    }

    oal_netbuf_free(cmd_netbuf);
    frw_event_free_m(event_mem);
    return OAL_SUCC;
}

uint32_t hmac_chba_topo_info_sync(void)
{
    uint32_t ret;
    int32_t err;
    hmac_chba_vap_stru *chba_vap_info = NULL;
    hmac_vap_stru *hmac_vap = NULL;
    chba_h2d_topo_info_stru topo_info;
    uint32_t *network_topo_addr = hmac_chba_get_network_topo_addr();

    chba_vap_info = hmac_get_up_chba_vap_info();
    if (chba_vap_info == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(chba_vap_info->mac_vap_id);
    if (hmac_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    memset_s(&topo_info, sizeof(chba_h2d_topo_info_stru), 0, sizeof(chba_h2d_topo_info_stru));

    /* 填写连接拓扑 */
    err = memcpy_s(topo_info.network_topo, sizeof(uint32_t) * (MAC_CHBA_MAX_DEVICE_NUM * MAC_CHBA_MAX_BITMAP_WORD_NUM),
        network_topo_addr, sizeof(uint32_t) * (MAC_CHBA_MAX_DEVICE_NUM * MAC_CHBA_MAX_BITMAP_WORD_NUM));
    if (err != EOK) {
        OAM_WARNING_LOG1(0, OAM_SF_CHBA, "hmac_chba_topo_info_sync::memcpy failed, err[%d].", err);
        return OAL_FAIL;
    }

    /* 同步给DMAC */
    ret = hmac_chba_send_chba_topo_info(&hmac_vap->st_vap_base_info, &topo_info);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_CHBA, "hmac_chba_topo_info_sync::sync topo info err[%u]", ret);
    }
    return ret;
}


uint32_t hmac_chba_device_info_sync(uint8_t update_type, uint8_t device_id, const uint8_t *mac_addr)
{
    uint32_t ret;
    hmac_chba_vap_stru *chba_vap_info = NULL;
    hmac_vap_stru *hmac_vap = NULL;
    chba_h2d_device_update_stru device_update_info = { 0 };

    chba_vap_info = hmac_get_up_chba_vap_info();
    if (chba_vap_info == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(chba_vap_info->mac_vap_id);
    if (hmac_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 填写接口结构体 */
    device_update_info.update_type = update_type;
    if (update_type != CHBA_DEVICE_ID_CLEAR) {
        device_update_info.device_id = device_id;
        oal_set_mac_addr(device_update_info.mac_addr, mac_addr);
    }

    /* 同步给DMAC */
    ret = hmac_config_send_event(&hmac_vap->st_vap_base_info, WLAN_CFGID_CHBA_DEVICE_INFO_UPDATE,
        sizeof(chba_h2d_device_update_stru), (uint8_t *)&device_update_info);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG1(0, OAM_SF_CHBA, "hmac_chba_device_info_sync::sync device info err[%u]", ret);
    }
    return ret;
}


hmac_chba_vap_stru *hmac_chba_get_chba_vap_info(hmac_vap_stru *hmac_vap)
{
    /* chba vap未使能 */
    if (hmac_chba_vap_start_check(hmac_vap) == OAL_FALSE) {
        return NULL;
    }

    return ((hmac_chba_vap_stru *)hmac_vap->hmac_chba_vap_info);
}


hmac_vap_stru *hmac_chba_find_chba_vap(mac_device_stru *mac_device)
{
    uint8_t vap_idx;
    mac_vap_stru *mac_vap = NULL;
    hmac_vap_stru *hmac_vap = NULL;

    /* chba定制化开关未开启, 无需进行查找 */
    if (hwifi_get_chba_en() == OAL_FALSE) {
        return NULL;
    }

    for (vap_idx = 0; vap_idx < mac_device->uc_vap_num; vap_idx++) {
        mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(mac_device->auc_vap_id[vap_idx]);
        if (mac_vap == NULL) {
            continue;
        }

        hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
        if (hmac_chba_vap_start_check(hmac_vap) == OAL_TRUE) {
            return hmac_vap;
        }
    }

    return NULL;
}
#endif /* end of this file */
