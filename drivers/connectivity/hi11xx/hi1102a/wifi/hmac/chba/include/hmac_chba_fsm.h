/*
 * 版权所有 (c) 华为技术有限公司 2021-2021
 * 功能说明 : WIFI CHBA关联发起方状态机头文件
 * 作    者 : duankaiyong
 * 创建日期 : 2021年8月18日
 */

#ifndef __HMAC_CHBA_FSM_H__
#define __HMAC_CHBA_FSM_H__

#include "hmac_fsm.h"
#include "hmac_mgmt_sta.h"
#include "hmac_chba_function.h"
#include "hmac_chba_mgmt.h"

#ifdef _PRE_WLAN_CHBA_MGMT
void hmac_chba_connect_initiator_fsm_init(void);

void hmac_chba_connect_initiator_fsm_change_state(hmac_user_stru *hmac_user, chba_user_state_enum chba_user_state);

uint32_t hmac_fsm_call_func_chba_conn_initiator(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user,
    hmac_fsm_input_type_enum_uint8 input_event, void *param);

void hmac_chba_handle_auth_rsp(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, hmac_auth_rsp_stru *auth_rsp);
#endif
#endif /* __HMAC_CHBA_FSM_H__ */