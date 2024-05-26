

#ifndef __HMAC_PPS_FSM_H__
#define __HMAC_PPS_FSM_H__

#include "oal_ext_if.h"

typedef uint32_t (*hmac_pps_fsm_state_trans_action)(void);

typedef struct {
    uint8_t current_state;                  /* 状态机当前状态 */
    uint8_t next_state;                     /* 状态机下一状态 */
    uint8_t event;                          /* 触发状态转换的事件类型 */
    hmac_pps_fsm_state_trans_action action; /* 状态转换时执行的操作 */
} hmac_pps_fsm_state_trans_stru;

#define HMAC_PPS_FSM_MAX_SIZE 8

typedef struct {
    hmac_pps_fsm_state_trans_stru fsm_table[HMAC_PPS_FSM_MAX_SIZE];
    uint16_t entry_num;
    uint16_t id;
    uint32_t host_keep_cnt;
    oal_atomic current_state;
    oal_atomic enable;
} hmac_pps_fsm_stru;

void hmac_pps_fsm_handle_event(uint8_t event, hmac_pps_fsm_stru *fsm);

#endif
