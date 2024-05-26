

#ifdef _PRE_WLAN_CHBA_MGMT
#include "hmac_chba_user.h"
#include "hmac_chba_sync.h"
#include "hmac_mgmt_ap.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_USER_C

void hmac_chba_user_set_connect_role(hmac_user_stru *hmac_user,
    chba_connect_role_enum connect_role)
{
    hmac_user->chba_user.connect_info.connect_role = connect_role;
    oam_warning_log2(0, OAM_SF_CHBA, "CHBA:set user [%d] connect_role to [%d]. 0: INITIATOR 1: RESPONDER",
        hmac_user->st_user_base_info.us_assoc_id, connect_role);
}

void hmac_chba_user_set_ssid(hmac_user_stru *hmac_user, uint8_t *ssid, uint8_t ssid_len)
{
    if (ssid_len <= 0 || ssid_len > WLAN_SSID_MAX_LEN) {
        OAM_ERROR_LOG1(0, OAM_SF_CHBA, "hmac_chba_user_set_ssid:invalid ssid_len %d", ssid_len);
        return;
    }

    if (memcpy_s(hmac_user->chba_user.ssid, WLAN_SSID_MAX_LEN, ssid, ssid_len) != EOK) {
        return;
    }

    hmac_user->chba_user.ssid_len = ssid_len;
}


static void hmac_chba_user_bitmap_level_init(mac_vap_stru *mac_vap, hmac_chba_user_stru *chba_user_info,
    uint16_t user_index)
{
    chba_user_ps_cfg_stru user_ps_cfg;

    /* ��ʼ������user bitmap��λΪ��ߵ� */
    chba_user_info->user_bitmap_level = CHBA_BITMAP_ALL_AWAKE_LEVEL;

    /* ͬ����dmac ��ʼ��user�͹������� */
    user_ps_cfg.user_idx = user_index;
    user_ps_cfg.bitmap_level = chba_user_info->user_bitmap_level;
    hmac_config_send_event(mac_vap, WLAN_CFGID_CHBA_SET_USER_BITMAP_LEVEL, sizeof(chba_user_ps_cfg_stru),
        (oal_uint8 *)&user_ps_cfg);
}


uint32_t hmac_chba_add_user(mac_vap_stru *mac_vap, uint8_t *peer_addr, uint16_t *user_idx)
{
    uint32_t ret;
    uint16_t user_index;
    hmac_user_stru *hmac_user = NULL;
    hmac_chba_user_stru *chba_user = NULL;

    /* ����û� */
    ret = hmac_user_add(mac_vap, peer_addr, &user_index);
    if (ret != OAL_SUCC) {
        return ret;
    }
    hmac_user = mac_res_get_hmac_user(user_index);
    if (hmac_user == NULL) {
        return OAL_FAIL;
    }
    *user_idx = user_index;
    /* ��chba_user���г�ʼ�� */
    chba_user = &(hmac_user->chba_user);
    memset_s(chba_user, sizeof(hmac_chba_user_stru), 0, sizeof(hmac_chba_user_stru));
    chba_user->req_sync.need_sync_flag = OAL_FALSE;
    chba_user->chba_link_state = CHBA_USER_STATE_INIT;
    chba_user->connect_info.assoc_waiting_time = CHBA_ASSOC_WAITING_TIMEOUT;
    chba_user->connect_info.max_assoc_cnt = CHBA_MAX_ASSOC_CNT;
    chba_user->connect_info.status_code = MAC_CHBA_INIT_CODE;

    /* ��ʼ��chba user bitmap��λ */
    hmac_chba_user_bitmap_level_init(mac_vap, chba_user, user_index);
    /* ˢ��beacon/pnf/assoc��֡������ֶ� */
    hmac_chba_save_update_beacon_pnf(hmac_chba_sync_get_domain_bssid());
    return OAL_SUCC;
}


void hmac_chba_del_user_proc(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user)
{
    hmac_chba_island_list_stru tmp_island_info = { 0 };
    hmac_chba_vap_stru *chba_vap_info = NULL;
    uint8_t *peer_mac_addr = NULL;
    uint8_t *own_mac_addr = NULL;

    if (hmac_vap == NULL || hmac_user == NULL) {
        return;
    }

    if ((hmac_chba_vap_start_check(hmac_vap) == OAL_FALSE) ||
        (hmac_chba_get_module_state() == MAC_CHBA_MODULE_STATE_UNINIT)) {
        return;
    }
    chba_vap_info = hmac_get_up_chba_vap_info();
    if (chba_vap_info == NULL) {
        return;
    }

    own_mac_addr = mac_mib_get_StationID(&hmac_vap->st_vap_base_info);
    peer_mac_addr = hmac_user->st_user_base_info.auc_user_mac_addr;

    /* ɾ��userʱֹͣkick user��ʱ�� */
    if (hmac_user->chba_user.chba_kick_user_timer.en_is_registerd == OAL_TRUE) {
        frw_immediate_destroy_timer(&hmac_user->chba_user.chba_kick_user_timer);
        return;
    }

    /* ����ɾ����Ϣ��������������ͼ */
    hmac_chba_update_network_topo(own_mac_addr, peer_mac_addr);

    /* ������������ͼ�������ɵ���Ϣ */
    hmac_chba_update_island_info(hmac_vap, &tmp_island_info);
    hmac_chba_print_island_info(&tmp_island_info);

    /* ���±��豸������Ϣ */
    hmac_chba_update_self_conn_info(hmac_vap, &tmp_island_info, FALSE);
    hmac_chba_print_self_conn_info();
    /* �����ʱ����Ϣ���ͷ��ڴ� */
    hmac_chba_clear_island_info(&tmp_island_info, hmac_chba_tmp_island_list_node_free);

    /* �������豸��Ϣ���µ�����ͬ����Ϣ�ṹ���� */
    hmac_chba_update_island_sync_info();

    /* ��������Ϣͬ����DMAC */
    hmac_chba_topo_info_sync();

    oam_warning_log3(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CHBA,
        "{hmac_chba_del_user_proc:peer_addr XX:XX:XX:%02x:%02x:%02x}",
        peer_mac_addr[3], peer_mac_addr[4], peer_mac_addr[5]);
}


uint32_t hmac_chba_user_mgmt_conn_prepare(mac_vap_stru *mac_vap,
    hmac_chba_conn_param_stru *chba_conn_params, int16_t *user_idx, uint8_t connect_role)
{
    hmac_user_stru *hmac_user = NULL;
    hmac_chba_user_stru *chba_user = NULL;
    uint16_t user_index;
    uint32_t ret;
    /* ������û��Ѿ�����, ��ɾ���û�������ӣ����½��� */
    hmac_user = mac_vap_get_hmac_user_by_addr(mac_vap, chba_conn_params->peer_addr, WLAN_MAC_ADDR_LEN);
    if (hmac_user != NULL) {
        oam_warning_log0(mac_vap->uc_vap_id, OAM_SF_CHBA,
            "{hmac_chba_user_mgmt_conn_prepare: user is already exist, delete it.}");
        hmac_user_del(mac_vap, hmac_user);
    }

    ret = hmac_chba_add_user(mac_vap, chba_conn_params->peer_addr, &user_index);
    if (ret != OAL_SUCC) {
        chba_conn_params->status_code = MAC_CHBA_CREATE_NEW_USER_FAIL;
        return ret;
    }
    *user_idx = user_index;
    hmac_user = mac_res_get_hmac_user(user_index);
    if (hmac_user == NULL) {
        chba_conn_params->status_code = MAC_CHBA_CREATE_NEW_USER_FAIL;
        return OAL_FAIL;
    }
    chba_user = &(hmac_user->chba_user);
    hmac_chba_user_set_connect_role(hmac_user, connect_role);
    chba_user->connect_info.assoc_channel = chba_conn_params->assoc_channel;
    /* ����������Ҫ�޸�uc_assoc_vap_id,�Ա���֡ */
    mac_vap->uc_assoc_vap_id = user_index;
    return OAL_SUCC;
}


static uint32_t hmac_chba_kick_user_timeout(void *arg)
{
    hmac_vap_stru *hmac_vap = NULL;
    hmac_user_stru *hmac_user = (hmac_user_stru *)arg;
    if (hmac_user == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CHBA, "{hmac_chba_kick_user_timeout:hmac_user is NULL!}");
        return OAL_SUCC;
    }

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(hmac_user->st_user_base_info.uc_vap_id);
    if (hmac_vap == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CHBA, "{hmac_chba_kick_user_timeout:hmac_vap is NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_WARNING_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CHBA,
        "{hmac_chba_kick_user_timeout:del user[%d]}", hmac_user->st_user_base_info.us_assoc_id);

    /* ���¼��ϱ��ںˣ��Ѿ�ȥ����ĳ��chba user */
    hmac_handle_disconnect_rsp_ap(hmac_vap, hmac_user);

    /* ɾ���û� */
    hmac_user_del(&hmac_vap->st_vap_base_info, hmac_user);
    return OAL_SUCC;
}


void hmac_chba_kick_user(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user)
{
    hmac_chba_user_stru *chba_user_info = NULL;

    if (oal_any_null_ptr2(hmac_vap, hmac_user)) {
        OAM_ERROR_LOG0(0, OAM_SF_CHBA, "{hmac_chba_kick_user:ptr is NULL!}");
        return;
    }

    chba_user_info = &hmac_user->chba_user;
    /* ȥ������ʱ�����ظ�����, �Ե�һ�ζ�ʱΪ׼ */
    if (chba_user_info->chba_kick_user_timer.en_is_registerd == OAL_TRUE) {
        return;
    }

    /* ����һ��ȥ������ʱ��, �ȴ�ȥ������� */
    frw_create_timer(&(chba_user_info->chba_kick_user_timer), hmac_chba_kick_user_timeout,
        MAC_CHBA_DEFALUT_PERIODS_MS, (void *)hmac_user, OAL_FALSE, OAM_MODULE_ID_HMAC,
        hmac_vap->st_vap_base_info.ul_core_id);
}
#endif

