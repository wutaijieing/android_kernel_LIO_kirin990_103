/*
 * 版权所有 (c) 华为技术有限公司 2021-2021
 * 功能说明 : WIFI CHBA关联发起方状态机文件
 * 作    者 : duankaiyong
 * 创建日期 : 2021年8月18日
 */

#include "hmac_fsm.h"
#include "hmac_chba_mgmt.h"
#include "hmac_chba_function.h"
#include "hmac_sme_sta.h"
#include "hmac_chba_fsm.h"
#include "hmac_sae.h"
#include "hmac_mgmt_join.h"
#include "hmac_encap_frame_sta.h"
#include "hmac_11i.h"
#include "hmac_rx_data.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_FSM_C
#ifdef _PRE_WLAN_CHBA_MGMT
/* CHBA入网状态机函数 */
static uint32_t hmac_chba_wait_auth_seq2_rx(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, void *param);
static uint32_t hmac_chba_wait_auth_seq4_rx(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, void *param);
static uint32_t hmac_chba_wait_assoc_rx(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, void *param);
static uint32_t hmac_chba_wait_auth(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, void *param);
static uint32_t hmac_chba_wait_assoc(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, void *param);
static uint32_t hmac_chba_wait_auth_timeout(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, void *param);
static uint32_t hmac_chba_wait_assoc_timeout(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, void *param);

static void hmac_chba_handle_assoc_rsp(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *hmac_user,
    hmac_asoc_rsp_stru *asoc_rsp);

/* CHBA关联状态机函数 */
typedef uint32_t (*chba_connect_initiator_fsm_func)(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, void *param);
static chba_connect_initiator_fsm_func g_chba_conn_initiator_fsm[CHBA_USER_STATE_BUTT][HMAC_FSM_INPUT_TYPE_BUTT];

/*
 * 功能描述  : CHBA关联发起者状态机空函数
 */
static oal_uint32 hmac_chba_fsm_null_fn(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, oal_void *param)
{
    /* 什么都不做 */
    oam_warning_log0(0, OAM_SF_CHBA, "{hmac_chba_fsm_null_fn::null function}");

    return OAL_SUCC;
}

/*
 * 功能描述  : 初始化CHBA关联发起方状态机
 */
void hmac_chba_connect_initiator_fsm_init(void)
{
    uint32_t state;
    uint32_t input_reqest;

    /* 初始化为空函数 */
    for (state = CHBA_USER_STATE_INIT; state < CHBA_USER_STATE_BUTT; state++) {
        for (input_reqest = 0; input_reqest < HMAC_FSM_INPUT_TYPE_BUTT; input_reqest++) {
            g_chba_conn_initiator_fsm[state][input_reqest] = hmac_chba_fsm_null_fn;
        }
    }

    /* 初始化CHBA关联发起方 AUTH_REQ 状态机函数表 */
    g_chba_conn_initiator_fsm[CHBA_USER_STATE_JOIN_COMPLETE][HMAC_FSM_INPUT_AUTH_REQ] = hmac_chba_wait_auth;

    /* 初始化CHBA关联发起方 ASOC_REQ 状态机函数表 */
    g_chba_conn_initiator_fsm[CHBA_USER_STATE_AUTH_COMPLETE][HMAC_FSM_INPUT_ASOC_REQ] = hmac_chba_wait_assoc;

    /* 初始化CHBA关联发起方 RX_MGMT 状态机函数表 */
    g_chba_conn_initiator_fsm[CHBA_USER_STATE_WAIT_AUTH_SEQ2][HMAC_FSM_INPUT_RX_MGMT] = hmac_chba_wait_auth_seq2_rx;
    g_chba_conn_initiator_fsm[CHBA_USER_STATE_WAIT_AUTH_SEQ4][HMAC_FSM_INPUT_RX_MGMT] = hmac_chba_wait_auth_seq4_rx;
    g_chba_conn_initiator_fsm[CHBA_USER_STATE_WAIT_ASSOC][HMAC_FSM_INPUT_RX_MGMT] = hmac_chba_wait_assoc_rx;

    /* 初始化CHBA关联发起方 定时器超时TIMER_OUT 状态机函数表 */
    g_chba_conn_initiator_fsm[CHBA_USER_STATE_WAIT_AUTH_SEQ2][HMAC_FSM_INPUT_TIMER0_OUT] = hmac_chba_wait_auth_timeout;
    g_chba_conn_initiator_fsm[CHBA_USER_STATE_WAIT_AUTH_SEQ4][HMAC_FSM_INPUT_TIMER0_OUT] = hmac_chba_wait_auth_timeout;
    g_chba_conn_initiator_fsm[CHBA_USER_STATE_WAIT_ASSOC][HMAC_FSM_INPUT_TIMER0_OUT] = hmac_chba_wait_assoc_timeout;
}

/*
 * 功能描述  : CHBA设备关联状态机函数调用
 */
uint32_t hmac_fsm_call_func_chba_conn_initiator(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user,
    hmac_fsm_input_type_enum_uint8 input_event, void *param)
{
    uint32_t chba_user_state;
    if (hmac_vap == NULL || hmac_user == NULL || param == NULL) {
        OAM_ERROR_LOG0(0, OAM_SF_CHBA, "hmac_fsm_call_func_chba_conn_initiator:hmac_vap or hmac_user or param is NULL");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (input_event < 0 || input_event >= HMAC_FSM_INPUT_TYPE_BUTT) {
        OAM_ERROR_LOG1(0, OAM_SF_CHBA, "hmac_fsm_call_func_chba_conn_initiator:input event [%d] invalid", input_event);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    chba_user_state = hmac_user->chba_user.chba_link_state;
    if (chba_user_state < CHBA_USER_STATE_INIT || chba_user_state >= CHBA_USER_STATE_BUTT) {
        OAM_ERROR_LOG1(0, OAM_SF_CHBA, "hmac_fsm_call_func_chba_conn_initiator:chba user state [%d] invalid",
            chba_user_state);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    return g_chba_conn_initiator_fsm[chba_user_state][input_event](hmac_vap, hmac_user, param);
}


void hmac_chba_connect_initiator_fsm_change_state(hmac_user_stru *hmac_user, chba_user_state_enum chba_user_state)
{
    oam_warning_log2(0, OAM_SF_CHBA, "CHBA:user_state_change from [%d] to [%d]",
        hmac_user->chba_user.chba_link_state, chba_user_state);
    hmac_user->chba_user.chba_link_state = chba_user_state;
}


static void hmac_chba_set_tx_ctl(oal_netbuf_stru *netbuf, uint16_t mpdu_len, uint16_t tx_user_idx)
{
    mac_tx_ctl_stru *tx_ctl = NULL;
    tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(netbuf); /* 获取cb结构体 */
    tx_ctl->us_mpdu_len = mpdu_len; /* dmac发送需要的mpdu长度 */
    tx_ctl->us_tx_user_idx = tx_user_idx; /* 发送完成需要获取user结构体 */
}

/*
 * 功能描述  : 发起认证
 */
static uint32_t  hmac_chba_initiate_auth(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user)
{
    hmac_auth_req_stru  auth_req;
    uint32_t ret;

    if (oal_unlikely(hmac_vap == NULL)) {
        OAM_ERROR_LOG0(0, OAM_SF_AUTH, "{hmac_chba_initiate_auth: hmac_vap is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_prepare_auth_req(hmac_vap, &auth_req);

    /* 状态机调用 hmac_chba_wait_auth */
    ret = hmac_fsm_call_func_chba_conn_initiator(hmac_vap, hmac_user, HMAC_FSM_INPUT_AUTH_REQ, (void *)(&auth_req));
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
            "{hmac_chba_initiate_auth::fsm_call_func fail[%d].}", ret);
        return ret;
    }

    return OAL_SUCC;
}

/*
 * 功能描述  : 发起关联
 */
static uint32_t  hmac_chba_initiate_assoc(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user)
{
    hmac_asoc_req_stru assoc_req;
    uint32_t ret;

    if (oal_any_null_ptr2(hmac_vap, hmac_user)) {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{hmac_chba_initiate_assoc::null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    assoc_req.us_assoc_timeout = (uint16_t)
        hmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.ul_dot11AssociationResponseTimeOut >> 1;

    /* 状态机调用 hmac_chba_wait_assoc */
    ret = hmac_fsm_call_func_chba_conn_initiator(hmac_vap, hmac_user, HMAC_FSM_INPUT_ASOC_REQ, (void *)(&assoc_req));
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                         "hmac_chba_initiate_assoc::fsm_fun fail[%d]", ret);
        return ret;
    }

    return OAL_SUCC;
}

/*
 * 功能描述  : CHBA设备认证超时处理
 */
static uint32_t hmac_chba_wait_auth_timeout(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, void *param)
{
    hmac_auth_rsp_stru auth_rsp;

    if (oal_any_null_ptr3(hmac_vap, hmac_user, param)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 根据用户认证状态，准备认证超时参数 */
    if (hmac_user->chba_user.chba_link_state == CHBA_USER_STATE_WAIT_AUTH_SEQ2) {
        auth_rsp.us_status_code = MAC_AUTH_RSP2_TIMEOUT;
    } else if (hmac_user->chba_user.chba_link_state == CHBA_USER_STATE_WAIT_AUTH_SEQ4) {
        auth_rsp.us_status_code = MAC_AUTH_RSP4_TIMEOUT;
    } else {
        auth_rsp.us_status_code = HMAC_MGMT_TIMEOUT;
    }

    hmac_chba_handle_auth_rsp(hmac_vap, hmac_user, &auth_rsp);

    return OAL_SUCC;
}

/*
 * 功能描述  : 关联超时处理函数
 */
static uint32_t hmac_chba_wait_assoc_timeout(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, void *param)
{
    hmac_asoc_rsp_stru assoc_rsp = { 0 };

    if (oal_any_null_ptr3(hmac_vap, hmac_user, param)) {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{hmac_chba_wait_assoc_timeout::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 填写关联结果 */
    assoc_rsp.en_result_code = HMAC_MGMT_TIMEOUT;

    /* 关联超时失败,原因码上报wpa_supplicant */
    assoc_rsp.en_status_code = hmac_vap->st_mgmt_timetout_param.en_status_code;

    /* 发送关联结果给SME */
    hmac_chba_handle_assoc_rsp(hmac_vap, hmac_user, &assoc_rsp);

    return OAL_SUCC;
}

/*
 * 功能描述  : CHBA处理认证结果
 */
void hmac_chba_handle_auth_rsp(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, hmac_auth_rsp_stru *auth_rsp)
{
    uint8_t reg_info_all[] = "all"; /* 认证失败，寄存器上报使用 */

    if (auth_rsp->us_status_code == HMAC_MGMT_SUCCESS) {
        /* 认证成功，发起关联 */
        hmac_user->chba_user.connect_info.assoc_cnt = 1;
        hmac_chba_initiate_assoc(hmac_vap, hmac_user);
        return;
    }

    oam_warning_log2(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
        "hmac_chba_handle_auth_rsp::auth fail[%d],cnt[%d]",
        auth_rsp->us_status_code, hmac_user->chba_user.connect_info.auth_cnt);

    if (hmac_user->chba_user.connect_info.auth_cnt < MAX_AUTH_CNT) {
        /* 认证次数未达到最大认证次数，继续发起认证 */
        hmac_chba_connect_initiator_fsm_change_state(hmac_user, CHBA_USER_STATE_JOIN_COMPLETE);

        hmac_user->chba_user.connect_info.auth_cnt++;

        /* 重新发起认证动作 */
        hmac_chba_initiate_auth(hmac_vap, hmac_user);
        return;
    }

    /* 本次关联认证次数超过最大认证次数，关联失败 */
    hmac_config_reg_info(&(hmac_vap->st_vap_base_info), sizeof(reg_info_all), reg_info_all);

    hmac_send_connect_result_to_dmac_sta(hmac_vap, OAL_FAIL);

    /* 关联失败，配置用户状态为无效值 */
    hmac_chba_connect_initiator_fsm_change_state(hmac_user, CHBA_USER_STATE_BUTT);
    /* 从白名单中删除 */
    hmac_chba_whitelist_clear(hmac_vap);

#ifdef _PRE_WLAN_FEATURE_SAE
    if (hmac_vap->en_auth_mode == WLAN_WITP_AUTH_SAE && auth_rsp->us_status_code != MAC_AP_FULL) {
        /* SAE关联失败，上报停止external auth到wpa_s */
        hmac_report_external_auth_req(hmac_vap, NL80211_EXTERNAL_AUTH_ABORT);
    }
#endif
    hmac_report_connect_failed_result(hmac_vap, auth_rsp->us_status_code,
        hmac_user->st_user_base_info.auc_user_mac_addr);

    hmac_user_del(&(hmac_vap->st_vap_base_info), hmac_user);
    /* 上报关联失败到wpa_supplicant */
}

/*
 * 功能描述  : 处理关联结果
 */
static void hmac_chba_handle_assoc_rsp(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user,
    hmac_asoc_rsp_stru *assoc_rsp)
{
    mac_vap_stru            *pst_mac_vap = &(hmac_vap->st_vap_base_info);

    if (assoc_rsp->en_result_code == HMAC_MGMT_SUCCESS) {
        hmac_handle_assoc_rsp_succ_sta(hmac_vap, assoc_rsp);
    } else {
        oam_warning_log2(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
            "{hmac_chba_handle_assoc_rsp::asoc fail[%d],assoc_cnt[%d]}",
            assoc_rsp->en_result_code, hmac_user->chba_user.connect_info.assoc_cnt);

        if (hmac_user->chba_user.connect_info.assoc_cnt >= MAX_ASOC_CNT) {
            /* 发送去认证帧到AP */
            hmac_mgmt_send_deauth_frame(&hmac_vap->st_vap_base_info,
                hmac_user->st_user_base_info.auc_user_mac_addr, WLAN_MAC_ADDR_LEN, MAC_AUTH_NOT_VALID, OAL_FALSE);

            /* 需要将状态机设置为无效状态 */
            hmac_chba_connect_initiator_fsm_change_state(hmac_user, CHBA_USER_STATE_BUTT);
            /* 从白名单中删除 */
            hmac_chba_whitelist_clear(hmac_vap);
            /* 上报关联失败到wpa_supplicant */
            hmac_report_connect_failed_result(hmac_vap, assoc_rsp->en_status_code,
                hmac_user->st_user_base_info.auc_user_mac_addr);
            /* 删除对应用户 */
            hmac_user_del(&hmac_vap->st_vap_base_info, hmac_user);

            /* 同步关联失败状态到DMAC */
            hmac_send_connect_result_to_dmac_sta(hmac_vap, OAL_FAIL);
        } else {
            /* 需要将状态机设置为AUTH_COMPLETE */
            hmac_chba_connect_initiator_fsm_change_state(hmac_user, CHBA_USER_STATE_AUTH_COMPLETE);

            /* 发起ASOC的次数 */
            hmac_user->chba_user.connect_info.assoc_cnt++;

            /* 重新发起关联动作 */
            hmac_chba_initiate_assoc(hmac_vap, hmac_user);
        }
    }
}

/*
 * 功能描述  : CHBA关联发起方入网超时定时器处理函数
 */
static uint32_t hmac_chba_mgmt_timeout(oal_void *arg)
{
    hmac_vap_stru *hmac_vap = NULL;
    hmac_user_stru *hmac_user = NULL;
    hmac_mgmt_timeout_param_stru *timeout_param = NULL;

    timeout_param = (hmac_mgmt_timeout_param_stru *)arg;
    if (timeout_param == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(timeout_param->uc_vap_id);
    if (hmac_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_user = mac_res_get_hmac_user(timeout_param->us_user_index);
    if (hmac_user == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    return hmac_fsm_call_func_chba_conn_initiator(hmac_vap, hmac_user, HMAC_FSM_INPUT_TIMER0_OUT, arg);
}

static void hmac_chba_start_mgmt_timer(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, uint32_t timeout)
{
    hmac_vap->st_mgmt_timetout_param.us_user_index = hmac_user->st_user_base_info.us_assoc_id;
    hmac_vap->st_mgmt_timetout_param.uc_vap_id = hmac_vap->st_vap_base_info.uc_vap_id;
    hmac_vap->st_mgmt_timetout_param.en_status_code = HMAC_MGMT_TIMEOUT;
    frw_create_timer(&(hmac_vap->st_mgmt_timer), hmac_chba_mgmt_timeout,
                     timeout, &(hmac_vap->st_mgmt_timetout_param),
                     OAL_FALSE, OAM_MODULE_ID_HMAC, hmac_vap->st_vap_base_info.ul_core_id);
}

/*
 * 功能描述  : CHBA入网发起方发出AUTH_REQ帧
 */
static uint32_t hmac_chba_wait_auth(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, void *param)
{
    hmac_auth_req_stru *auth_req = NULL;

    if (oal_any_null_ptr3(hmac_vap, hmac_user, param)) {
        OAM_ERROR_LOG0(0, OAM_SF_AUTH, "{hmac_chba_wait_auth::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (hmac_vap->en_auth_mode != WLAN_WITP_AUTH_SAE) {
        OAM_ERROR_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CHBA,
            "{hmac_chba_wait_auth:: auth mode [%d] not SAE.}", hmac_vap->en_auth_mode);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* CHBA SAE关联,上报external auth事件到wpa_s */
    oam_warning_log0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CHBA,
        "{hmac_chba_wait_auth:: report external auth to wpa_s.}");
#ifdef _PRE_WLAN_FEATURE_SAE
    /* 上报启动external auth到wpa_s(hmac_report_ext_auth_worker) */
    oal_workqueue_delay_schedule(&(hmac_vap->st_sae_report_ext_auth_worker), oal_msecs_to_jiffies(1));
#endif
    /* CHBA用户切换到 WAIT_AUTH_SEQ2 */
    hmac_chba_connect_initiator_fsm_change_state(hmac_user, CHBA_USER_STATE_WAIT_AUTH_SEQ2);

    /* 启动认证超时定时器 */
    auth_req = (hmac_auth_req_stru *)param;
    hmac_chba_start_mgmt_timer(hmac_vap, hmac_user, auth_req->us_timeout);

    return OAL_SUCC;
}

static uint32_t hmac_chba_is_auth_rsp_ok(hmac_vap_stru *hmac_vap,
    uint8_t *mac_hdr, uint32_t mac_hdr_len, uint8_t *payload, uint32_t payload_len,
    uint16_t expect_auth_seq, uint16_t expect_auth_alg)
{
#define MIN_AUTH_BODY_LEN 6
    uint16_t auth_alg;
    uint16_t auth_seq;
    mac_ieee80211_frame_stru *ieee80211_hdr = (mac_ieee80211_frame_stru *)mac_hdr;

    if (mac_hdr_len < WLAN_MGMT_FRAME_HEADER_LEN || payload_len < MIN_AUTH_BODY_LEN) {
        return OAL_FAIL;
    }

    if (mac_get_frame_sub_type(mac_hdr) != WLAN_FC0_SUBTYPE_AUTH) {
        return OAL_FAIL;
    }

    if (hmac_chba_whitelist_check(hmac_vap, ieee80211_hdr->auc_address2, WLAN_MAC_ADDR_LEN) != OAL_SUCC) {
        return OAL_FAIL;
    }

    auth_seq = mac_get_auth_seq_num(mac_hdr);
    if (auth_seq != expect_auth_seq) {
        oam_warning_log2(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
            "hmac_chba_is_auth_rsp_ok::rcv unexpected auth seq[%d].", auth_seq, expect_auth_seq);
        return OAL_FAIL;
    }

    /* AUTH alg CHECK */
    auth_alg = mac_get_auth_alg(mac_hdr);
    if (auth_alg != expect_auth_alg) {
        oam_warning_log2(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
            "hmac_chba_is_auth_rsp_ok::rcv unexpected auth alg[%d].", auth_alg, expect_auth_alg);
        return OAL_FAIL;
    }
    return OAL_SUCC;
}

/*
 * 功能描述  : 处理接收到WPA3 auth commit认证帧
 */
static uint32_t hmac_chba_wait_auth_seq2_rx(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, void *param)
{
    oal_netbuf_stru *netbuf = NULL;
    hmac_rx_ctl_stru *rx_ctl = NULL;
    uint8_t *mac_hdr = NULL;
    uint8_t *payload = NULL;
    uint32_t mac_hdr_len, payload_len;
    uint16_t status_code;
    hmac_auth_req_stru auth_req;

    if (oal_any_null_ptr3(hmac_vap, hmac_user, param)) {
        OAM_ERROR_LOG0(0, OAM_SF_AUTH, "{hmac_chba_wait_auth_seq2_rx::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    netbuf = (oal_netbuf_stru *)param;
    rx_ctl = (hmac_rx_ctl_stru *)oal_netbuf_cb(netbuf);
    mac_hdr = (uint8_t *)(rx_ctl->st_rx_info.pul_mac_hdr_start_addr);
    payload = mac_hdr + rx_ctl->st_rx_info.uc_mac_header_len;
    mac_hdr_len = rx_ctl->st_rx_info.uc_mac_header_len;
    payload_len = hmac_get_frame_body_len(netbuf);
    if (hmac_chba_is_auth_rsp_ok(hmac_vap, mac_hdr, mac_hdr_len, payload, payload_len,
        WLAN_AUTH_TRASACTION_NUM_ONE, WLAN_MIB_AUTH_ALG_SAE) != OAL_SUCC) {
        return OAL_FAIL;
    }

    status_code = mac_get_auth_status(mac_hdr);
    if (status_code != MAC_SUCCESSFUL_STATUSCODE && status_code != MAC_ANTI_CLOGGING) {
        hmac_vap->st_mgmt_timetout_param.en_status_code = status_code;

        OAM_WARNING_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
            "{hmac_chba_wait_auth_seq2_rx::AP refuse STA auth reason[%d]!}", status_code);

        return OAL_FAIL;
    }

    oam_warning_log0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
        "hmac_chba_wait_auth_seq2_rx::receive auth.");

    if (status_code != MAC_ANTI_CLOGGING) {
        /* statuc是SUCC将状态更改为WAIT_AUTH_SEQ4 */
        hmac_chba_connect_initiator_fsm_change_state(hmac_user, CHBA_USER_STATE_WAIT_AUTH_SEQ4);
    }
#ifdef _PRE_WLAN_FEATURE_SAE
    hmac_vap->duplicate_auth_seq4_flag = OAL_FALSE; // seq4置位
#endif

    frw_immediate_destroy_timer(&hmac_vap->st_mgmt_timer);

    /* 上报auth 到wpa_s */
    hmac_rx_mgmt_send_to_host(hmac_vap, netbuf);

    /* 启动认证超时定时器 */
    hmac_prepare_auth_req(hmac_vap, &auth_req);
    hmac_chba_start_mgmt_timer(hmac_vap, hmac_user, auth_req.us_timeout);
    return OAL_SUCC;
}

/*
 * 功能描述  : 处理收到WPA3 SAE CONFIRM认证帧
 */
static uint32_t hmac_chba_wait_auth_seq4_rx(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, oal_void *param)
{
    oal_netbuf_stru *netbuf = NULL;
    hmac_rx_ctl_stru *rx_ctl = NULL;
    uint8_t *mac_hdr = NULL;
    uint8_t *payload = NULL;
    uint32_t mac_hdr_len, payload_len;
    hmac_auth_req_stru auth_req;

    if (oal_any_null_ptr3(hmac_vap, hmac_user, param)) {
        OAM_ERROR_LOG0(0, OAM_SF_AUTH, "{hmac_chba_wait_auth_seq4_rx::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    netbuf = (oal_netbuf_stru *)param;
    rx_ctl = (hmac_rx_ctl_stru *)oal_netbuf_cb(netbuf);
    mac_hdr = (uint8_t *)(rx_ctl->st_rx_info.pul_mac_hdr_start_addr);
    payload = mac_hdr + rx_ctl->st_rx_info.uc_mac_header_len;
    mac_hdr_len = rx_ctl->st_rx_info.uc_mac_header_len;
    payload_len = hmac_get_frame_body_len(netbuf);
    if (hmac_chba_is_auth_rsp_ok(hmac_vap, mac_hdr, mac_hdr_len, payload, payload_len,
        WLAN_AUTH_TRASACTION_NUM_TWO, WLAN_MIB_AUTH_ALG_SAE) != OAL_SUCC) {
        return OAL_FAIL;
    }

#ifdef _PRE_WLAN_FEATURE_SAE
    if (hmac_vap->duplicate_auth_seq4_flag == OAL_TRUE) { // wpa3 auth seq4重复帧过滤，禁止上报waps
        uint16_t status_code = mac_get_auth_status(mac_hdr);
        oam_warning_log2(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CHBA,
            "{hmac_chba_wait_auth_seq4_rx::drop sae auth frame, status_code %d, seq_num %d.}",
            status_code, mac_get_auth_seq_num(mac_hdr));
        return OAL_SUCC;
    }
#endif
    oam_warning_log0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
        "hmac_chba_wait_auth_seq4_rx::receive auth.");

    frw_immediate_destroy_timer(&hmac_vap->st_mgmt_timer);

    /* 上报auth 到wpa_s */
    hmac_rx_mgmt_send_to_host(hmac_vap, netbuf);

    /* 启动认证超时定时器 */
    hmac_prepare_auth_req(hmac_vap, &auth_req);
    hmac_chba_start_mgmt_timer(hmac_vap, hmac_user, auth_req.us_timeout);
#ifdef _PRE_WLAN_FEATURE_SAE
    hmac_vap->duplicate_auth_seq4_flag = OAL_TRUE;
#endif
    return OAL_SUCC;
}

static uint32_t hmac_chba_is_assoc_rsp_ok(hmac_vap_stru *hmac_vap,
    uint8_t *mac_hdr, uint32_t mac_hdr_len, uint8_t *payload, uint32_t payload_len)
{
    mac_status_code_enum_uint16 assoc_status;
    uint8_t frame_sub_type;
    mac_ieee80211_frame_stru *ieee80211_hdr = NULL;

    if (payload_len < OAL_ASSOC_RSP_FIXED_OFFSET || mac_hdr_len < WLAN_MGMT_FRAME_HEADER_LEN) {
        oam_warning_log2(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
            "{hmac_chba_wait_assoc_rx::asoc_rsp_body [%d] or header [%d] is too short to going on!}",
            payload_len, mac_hdr_len);
        return OAL_FAIL;
    }

    frame_sub_type = mac_get_frame_sub_type(mac_hdr);
    if (frame_sub_type != WLAN_FC0_SUBTYPE_ASSOC_RSP) {
        return OAL_FAIL;
    }

    /* 获取SA 地址 */
    ieee80211_hdr = (mac_ieee80211_frame_stru *)mac_hdr;
    if (hmac_chba_whitelist_check(hmac_vap, ieee80211_hdr->auc_address2, WLAN_MAC_ADDR_LEN) != OAL_SUCC) {
        return OAL_FAIL;
    }

    assoc_status = mac_get_asoc_status(payload);
    if (assoc_status != MAC_SUCCESSFUL_STATUSCODE) {
        OAM_WARNING_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
            "{hmac_chba_wait_assoc_rx:: AP refuse STA assoc reason=%d.}", assoc_status);
        hmac_vap->st_mgmt_timetout_param.en_status_code = assoc_status;
        return OAL_FAIL;
    }

    return OAL_SUCC;
}
/* 功能描述：关联发起方关联成功，准备关联完成结构体上报 */
static void hmac_chba_prepare_assoc_rsp(hmac_vap_stru *hmac_vap, hmac_asoc_rsp_stru *assoc_rsp,
    uint8_t *mac_hdr, uint8_t *payload, uint32_t payload_len)
{
    assoc_rsp->en_result_code = HMAC_MGMT_SUCCESS;
    assoc_rsp->en_status_code = MAC_SUCCESSFUL_STATUSCODE;
    /* 记录关联响应帧的部分内容，用于上报给内核 */
    assoc_rsp->ul_asoc_rsp_ie_len = payload_len - OAL_ASSOC_RSP_FIXED_OFFSET; /* 除去assoc rsp 固定部分6字节 */
    assoc_rsp->puc_asoc_rsp_ie_buff = payload + OAL_ASSOC_RSP_FIXED_OFFSET;
    /* 获取源的mac地址 */
    mac_get_address2(mac_hdr, assoc_rsp->auc_addr_ap, WLAN_MAC_ADDR_LEN); /* chba IBSS帧结构,未使用基站,bssid随机产生 */
    /* 获取关联请求帧信息 */
    assoc_rsp->puc_asoc_req_ie_buff = hmac_vap->puc_asoc_req_ie_buff;
    assoc_rsp->ul_asoc_req_ie_len = hmac_vap->ul_asoc_req_ie_len;
}

/*
 * 功能描述  : 处理收到assoc rsp帧
 */
static uint32_t hmac_chba_wait_assoc_rx(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, void *param)
{
    oal_netbuf_stru *netbuf = NULL;
    hmac_rx_ctl_stru *rx_ctl = NULL;
    mac_rx_ctl_stru *rx_info = NULL;
    uint8_t *mac_hdr = NULL;
    uint8_t *payload = NULL;
    uint16_t mac_hdr_len;
    uint16_t payload_len;
    hmac_asoc_rsp_stru assoc_rsp;
    uint32_t ret;

    if (oal_any_null_ptr3(hmac_vap, hmac_user, param)) {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{hmac_chba_wait_assoc_rx::param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    netbuf = (oal_netbuf_stru *)param;
    rx_ctl = (hmac_rx_ctl_stru *)oal_netbuf_cb(netbuf);
    rx_info = (mac_rx_ctl_stru *)(&(rx_ctl->st_rx_info));
    mac_hdr = (uint8_t *)(rx_info->pul_mac_hdr_start_addr);
    payload = (uint8_t *)(mac_hdr) + rx_info->uc_mac_header_len;
    mac_hdr_len = rx_info->uc_mac_header_len;
    payload_len = hmac_get_frame_body_len(netbuf);
    if (hmac_chba_is_assoc_rsp_ok(hmac_vap, mac_hdr, mac_hdr_len, payload, payload_len) != OAL_SUCC) {
        return OAL_FAIL;
    }

    /* 取消定时器 */
    frw_immediate_destroy_timer(&(hmac_vap->st_mgmt_timer));
    oam_warning_log0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_chba_wait_assoc_rx::rx assoc rsp.}");
    ret = hmac_process_assoc_rsp(hmac_vap, hmac_user, mac_hdr, payload, payload_len);
    if (ret != OAL_SUCC) {
        OAM_WARNING_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
            "{hmac_chba_wait_assoc_rx::hmac_process_assoc_rsp failed[%d].}", ret);
        return ret;
    }

    /* 关联成功，更新CHBA信息 */
    hmac_chba_connect_succ_handler(hmac_vap, hmac_user,
        payload + OAL_ASSOC_RSP_FIXED_OFFSET, payload_len - OAL_ASSOC_RSP_FIXED_OFFSET);

    /* CHBA用户切换到UP状态 */
    hmac_chba_connect_initiator_fsm_change_state(hmac_user, CHBA_USER_STATE_LINK_UP);

    /* user已经关联上，抛事件给DMAC，在DMAC层挂用户算法钩子 */
    hmac_user_add_notify_alg(&(hmac_vap->st_vap_base_info), hmac_user->st_user_base_info.us_assoc_id);

    /* 准备消息，上报给APP */
    hmac_chba_prepare_assoc_rsp(hmac_vap, &assoc_rsp, mac_hdr, payload, payload_len);
    hmac_chba_handle_assoc_rsp(hmac_vap, hmac_user, &assoc_rsp);

    /* CHBA关联成功，输出维测信息 */
    hmac_config_vap_info(&(hmac_vap->st_vap_base_info), OAL_SIZEOF(uint32_t), (uint8_t *)&ret);

    return OAL_SUCC;
}

static void hmac_chba_free_assoc_req_by_vap(hmac_vap_stru *hmac_vap)
{
    if (hmac_vap->puc_asoc_req_ie_buff != NULL) {
        oal_mem_free_m(hmac_vap->puc_asoc_req_ie_buff, OAL_TRUE);
        hmac_vap->puc_asoc_req_ie_buff = NULL;
        hmac_vap->ul_asoc_req_ie_len = 0;
    }
}

static uint32_t hmac_chba_save_assoc_req_frame_by_vap(hmac_vap_stru *hmac_vap, uint8_t *assoc_req_frame, uint32_t len)
{
    hmac_vap->ul_asoc_req_ie_len = 0;
    hmac_vap->puc_asoc_req_ie_buff = oal_mem_alloc_m(OAL_MEM_POOL_ID_LOCAL, (uint16_t)len, OAL_TRUE);
    if (hmac_vap->puc_asoc_req_ie_buff == NULL) {
        OAM_ERROR_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
            "{hmac_chba_wait_assoc::puc_asoc_req_ie_buff null,alloc %u bytes failed}",
            len);
        return OAL_FAIL;
    }

    if (memcpy_s(hmac_vap->puc_asoc_req_ie_buff, len, assoc_req_frame, len) != EOK) {
        OAM_ERROR_LOG0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_chba_wait_assoc::memcpy failed}");
    }
    hmac_vap->ul_asoc_req_ie_len = len;
    return OAL_SUCC;
}

static void hmac_chba_init_assoc_req_info(hmac_vap_stru *hmac_vap)
{
    uint8_t support_rate_ie[WLAN_MAX_SUPP_RATES + MAC_IE_HDR_LEN] = {0};
    uint8_t support_rate_ie_len;

    /* 配置关联请求 capabilities */
    mac_set_cap_info_ap(&(hmac_vap->st_vap_base_info), (uint8_t *)&(hmac_vap->st_vap_base_info.us_assoc_user_cap_info));
    /* 配置支持速率集 */
    mac_set_supported_rates_ie(&hmac_vap->st_vap_base_info, support_rate_ie, &support_rate_ie_len);
    hmac_vap->uc_rs_nrates = support_rate_ie[1];
    if (memcpy_s(hmac_vap->auc_supp_rates, WLAN_MAX_SUPP_RATES,
        support_rate_ie + MAC_IE_HDR_LEN, hmac_vap->uc_rs_nrates) != EOK) {
        oam_error_log2(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CHBA,
            "hmac_chba_wait_assoc:: support rate ie len %d, vap rate num %d",
            support_rate_ie_len, hmac_vap->uc_rs_nrates);
    }

    /* 使能WMM */
    hmac_vap->uc_wmm_cap = OAL_TRUE;
    /* 使能聚合 */
    hmac_vap->en_tx_aggr_on = OAL_TRUE;
    hmac_vap->en_amsdu_active = OAL_TRUE;
}

/*
 * 功能描述  : 在AUTH_COMP状态接收到SME发过来的ASOC_REQ请求，将CHBA用户状态设置为WAIT_ASSOC,
 */
static uint32_t hmac_chba_wait_assoc(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, void *param)
{
    hmac_asoc_req_stru *hmac_assoc_req = NULL;
    oal_netbuf_stru *assoc_req_frame = NULL;
    uint32_t assoc_frame_len;
    uint32_t ret;

    if (oal_any_null_ptr3(hmac_vap, hmac_user, param)) {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{hmac_chba_wait_assoc::null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    assoc_req_frame = oal_mem_netbuf_alloc(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if (assoc_req_frame == NULL) {
        OAM_ERROR_LOG0(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                       "{hmac_chba_wait_assoc::assoc_req_frame null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_chba_free_assoc_req_by_vap(hmac_vap);

    memset_s(oal_netbuf_cb(assoc_req_frame), oal_netbuf_cb_size(), 0, oal_netbuf_cb_size());

    /* 将mac header清零 */
    memset_s(oal_netbuf_header(assoc_req_frame), MAC_80211_FRAME_LEN, 0, MAC_80211_FRAME_LEN);

    /* 初始化chba 关联发起方组帧assoc req 需要的资源 */
    hmac_chba_init_assoc_req_info(hmac_vap);

    assoc_frame_len = hmac_mgmt_encap_asoc_req_sta(hmac_vap, oal_netbuf_header(assoc_req_frame), NULL,
        hmac_user->st_user_base_info.auc_user_mac_addr);
    if (assoc_frame_len < OAL_ASSOC_REQ_IE_OFFSET) {
        OAM_ERROR_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
            "{hmac_chba_wait_assoc::hmac_mgmt_encap_asoc_req_sta len [%d] < assoc req min len.}", assoc_frame_len);
        oal_netbuf_free(assoc_req_frame);
        return OAL_FAIL;
    }
    oal_netbuf_put(assoc_req_frame, assoc_frame_len);

    /* 此处申请的内存，只在上报给内核后释放 */
    if (hmac_chba_save_assoc_req_frame_by_vap(hmac_vap,
        oal_netbuf_header(assoc_req_frame) + OAL_ASSOC_REQ_IE_OFFSET,
        assoc_frame_len - OAL_ASSOC_REQ_IE_OFFSET) != OAL_SUCC) {
        oal_netbuf_free(assoc_req_frame);
        return OAL_FAIL;
    }

    hmac_chba_set_tx_ctl(assoc_req_frame, assoc_frame_len, hmac_user->st_user_base_info.us_assoc_id);

    ret = hmac_tx_mgmt_send_event(&(hmac_vap->st_vap_base_info), assoc_req_frame, (uint16_t)assoc_frame_len);
    if (ret != OAL_SUCC) {
        oal_netbuf_free(assoc_req_frame);
        hmac_chba_free_assoc_req_by_vap(hmac_vap);

        OAM_WARNING_LOG1(hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
            "{hmac_chba_wait_assoc:tx_mgmt_send_event fail[%d].}", ret);
        return ret;
    }

    /* 更改CHBA用户状态为WAIT_ASSOC */
    hmac_chba_connect_initiator_fsm_change_state(hmac_user, CHBA_USER_STATE_WAIT_ASSOC);

    /* 启动关联超时定时器, 为对端ap分配一个定时器，如果超时ap没回asoc rsp则启动超时处理 */
    hmac_assoc_req = (hmac_asoc_req_stru *)param;
    hmac_chba_start_mgmt_timer(hmac_vap, hmac_user, hmac_assoc_req->us_assoc_timeout);

    return OAL_SUCC;
}
#endif