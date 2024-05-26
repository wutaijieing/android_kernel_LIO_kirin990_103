

#ifndef __HMAC_CHBA_CHAN_SWITCH_H__
#define __HMAC_CHBA_CHAN_SWITCH_H__

#include "hmac_chba_mgmt.h"
#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_CHAN_SWITCH_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _PRE_WLAN_CHBA_MGMT
/* 1 宏定义 */
/* 干扰检测时延门限，单位ms */
#define CHBA_INTF_DETECT_DELAY_TH 40
#define CHBA_KEEPALIVE_INTF_DETECT_DELAY_TH 1000
/* 干扰检测时延统计窗长 */
#define CHBA_INTF_DETECT_WINDOW_SIZE 3
/* 干扰上报的保护间隔 */
#define CHBA_INTF_RPT_PROTECT_SIZE 20
/* 信道切换最小时间间隔，单位ms */
#define CHBA_CHAN_SWITCH_MIN_INTERVAL 5000
/* 信道切换请求的超时时间，单位ms */
#define CHBA_CHAN_SWITCH_REQ_TIMEOUT 512
/* 信道切换请求的最大连续发送次数 */
#define CHBA_MAX_CONTI_SWITCH_REQ_CNT 2
/* 收集信道切换指示帧的ack的超时时间，单位ms */
#define CHBA_COLLECT_CHAN_SWITCH_ACK_TIMEOUT 16
/* 重传信道切换指示帧后的超时时间，单位ms */
#define CHBA_RETX_CHAN_SWITCH_IND_TIMEOUT 48
/* 给保活设备重传信道切换指示帧后的超时时间，单位ms */
#define CHBA_KEEPALIVE_RETX_CHAN_SWITCH_IND_TIMEOUT 256
/* 信道切换的RSSI门限 */
#define CHBA_CHAN_SWITCH_RSSI_TH (-72)
/* 发送失败包个数的门限 */
#define CHBA_TX_FAIL_MPDU_CNT_TH 5

#define hmac_chba_get_chan_switch_ctrl() (g_chan_switch_enable)
#define hmac_chba_set_chan_switch_ctrl(val) (g_chan_switch_enable = (val))

/* 2 枚举定义 */
/* 信道切换类型 */
typedef enum {
    MAC_CHBA_CHAN_SWITCH_FOR_COEX = 0, /* 上层触发调整 */
    MAC_CHBA_CHAN_SWITCH_FOR_PERFORMANCE = 1, /* driver触发 */
} hmac_chba_chan_switch_type_enum;

/* 信道切换状态 */
typedef enum {
    MAC_CHBA_CHAN_SWITCH_START = 0,
    MAC_CHBA_CHAN_SWITCH_SUCC = 1,
    MAC_CHBA_CHAN_SWITCH_FAIL = 2,
} hmac_chba_chan_switch_status_code_enum;

/* 信道切换请求 接收或者拒绝 */
typedef enum {
    MAC_CHBA_CHAN_SWITCH_ACCEPT = 0,
    MAC_CHBA_CHAN_SWITCH_REJECT = 1,
} hmac_chba_chan_switch_req_status_code_enum;

/* 信道切换状态枚举 */
typedef enum {
    CHAN_SWITCH_NONE = 0,
    CHAN_SWITCH_FIRST_SEL = 1,
    CHAN_SWITCH_SECOND_SEL = 2,
    CHAN_SWITCH_COMMON_SEL = 3,
} hmac_chba_chan_switch_state_enum;

/* 信道切换通知action帧的类型枚举 */
typedef enum {
    MAC_CHBA_CHAN_SWITCH_REQ = 0,
    MAC_CHBA_CHAN_SWITCH_RESP = 1,
    MAC_CHBA_CHAN_SWITCH_INDICATION = 2,
    MAC_CHBA_CHAN_SWITCH_CONFIRM = 3,
} hmac_chba_chan_switch_action_type_enum;

/* 信道切换等待时间枚举 */
typedef enum {
    MAC_CHBA_CHAN_SWITCH_IMMEDIATE = 0,
    MAC_CHBA_CHAN_SWITCH_NEXT_SLOT = 1,
    MAC_CHBA_CHAN_SWITCH_NEXT_NEXT_SLOT = 2,
} hmac_chba_chan_switch_slot_enum;

/* 3 结构体定义 */
/* 信道切换请求/指示的信息结构体 */
typedef struct {
    uint8_t notify_type; /* 信道切换通知类型，请求还是指示 */
    uint8_t chan_number; /* 主20M的信道号 */
    uint8_t bandwidth; /* 带宽模式 */
    uint8_t switch_slot; /* 指示在哪个slot头切换信道 */
    uint8_t status_code; /* CSN resp帧回复，接受还是拒绝 */
} hmac_chba_chan_switch_notify_stru;

/* 4 全局变量声明 */
extern uint8_t g_chan_switch_enable;

/* 5 函数声明 */
void hmac_chba_check_island_owner_chan(hmac_vap_stru *hmac_vap, uint8_t *sa_addr, uint8_t *payload,
    uint16_t payload_len);
void hmac_chba_user_stat_info_report_proc(uint8_t *recv_msg);
uint32_t hmac_chba_chan_query_all_user_stat_info(uint8_t query_type);
void hmac_chba_rx_chan_switch_notify_action(hmac_vap_stru *hmac_vap, uint8_t *sa_addr, uint8_t sa_len, uint8_t *payload,
    uint16_t payload_len);
void hmac_chba_stub_chan_switch_report_proc(uint8_t *recv_msg);
uint32_t hmac_chba_start_switch_chan(hmac_chba_mgmt_info_stru *chba_mgmt_info,
    mac_channel_stru *target_channel, uint8_t remain_slot, uint8_t type);
uint32_t hmac_chba_csa_lost_device_proc(mac_vap_stru *mac_vap, oal_uint8 len, oal_uint8 *param);
uint32_t hmac_chba_send_chan_switch_req(hmac_chba_mgmt_info_stru *chba_mgmt_info);
void hmac_chba_adjust_channel_proc(hmac_vap_stru *hmac_vap, mac_chba_adjust_chan_info *chan_info);
uint32_t hmac_chba_test_chan_switch_cmd(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params);
#endif

#ifdef __cplusplus
}
#endif

#endif
