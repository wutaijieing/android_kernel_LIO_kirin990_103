

#ifndef __HMAC_CHBA_CHAN_SWITCH_H__
#define __HMAC_CHBA_CHAN_SWITCH_H__

#include "hmac_chba_mgmt.h"
#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_CHAN_SWITCH_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _PRE_WLAN_CHBA_MGMT
/* 1 �궨�� */
/* ���ż��ʱ�����ޣ���λms */
#define CHBA_INTF_DETECT_DELAY_TH 40
#define CHBA_KEEPALIVE_INTF_DETECT_DELAY_TH 1000
/* ���ż��ʱ��ͳ�ƴ��� */
#define CHBA_INTF_DETECT_WINDOW_SIZE 3
/* �����ϱ��ı������ */
#define CHBA_INTF_RPT_PROTECT_SIZE 20
/* �ŵ��л���Сʱ��������λms */
#define CHBA_CHAN_SWITCH_MIN_INTERVAL 5000
/* �ŵ��л�����ĳ�ʱʱ�䣬��λms */
#define CHBA_CHAN_SWITCH_REQ_TIMEOUT 512
/* �ŵ��л����������������ʹ��� */
#define CHBA_MAX_CONTI_SWITCH_REQ_CNT 2
/* �ռ��ŵ��л�ָʾ֡��ack�ĳ�ʱʱ�䣬��λms */
#define CHBA_COLLECT_CHAN_SWITCH_ACK_TIMEOUT 16
/* �ش��ŵ��л�ָʾ֡��ĳ�ʱʱ�䣬��λms */
#define CHBA_RETX_CHAN_SWITCH_IND_TIMEOUT 48
/* �������豸�ش��ŵ��л�ָʾ֡��ĳ�ʱʱ�䣬��λms */
#define CHBA_KEEPALIVE_RETX_CHAN_SWITCH_IND_TIMEOUT 256
/* �ŵ��л���RSSI���� */
#define CHBA_CHAN_SWITCH_RSSI_TH (-72)
/* ����ʧ�ܰ����������� */
#define CHBA_TX_FAIL_MPDU_CNT_TH 5

#define hmac_chba_get_chan_switch_ctrl() (g_chan_switch_enable)
#define hmac_chba_set_chan_switch_ctrl(val) (g_chan_switch_enable = (val))

/* 2 ö�ٶ��� */
/* �ŵ��л����� */
typedef enum {
    MAC_CHBA_CHAN_SWITCH_FOR_COEX = 0, /* �ϲ㴥������ */
    MAC_CHBA_CHAN_SWITCH_FOR_PERFORMANCE = 1, /* driver���� */
} hmac_chba_chan_switch_type_enum;

/* �ŵ��л�״̬ */
typedef enum {
    MAC_CHBA_CHAN_SWITCH_START = 0,
    MAC_CHBA_CHAN_SWITCH_SUCC = 1,
    MAC_CHBA_CHAN_SWITCH_FAIL = 2,
} hmac_chba_chan_switch_status_code_enum;

/* �ŵ��л����� ���ջ��߾ܾ� */
typedef enum {
    MAC_CHBA_CHAN_SWITCH_ACCEPT = 0,
    MAC_CHBA_CHAN_SWITCH_REJECT = 1,
} hmac_chba_chan_switch_req_status_code_enum;

/* �ŵ��л�״̬ö�� */
typedef enum {
    CHAN_SWITCH_NONE = 0,
    CHAN_SWITCH_FIRST_SEL = 1,
    CHAN_SWITCH_SECOND_SEL = 2,
    CHAN_SWITCH_COMMON_SEL = 3,
} hmac_chba_chan_switch_state_enum;

/* �ŵ��л�֪ͨaction֡������ö�� */
typedef enum {
    MAC_CHBA_CHAN_SWITCH_REQ = 0,
    MAC_CHBA_CHAN_SWITCH_RESP = 1,
    MAC_CHBA_CHAN_SWITCH_INDICATION = 2,
    MAC_CHBA_CHAN_SWITCH_CONFIRM = 3,
} hmac_chba_chan_switch_action_type_enum;

/* �ŵ��л��ȴ�ʱ��ö�� */
typedef enum {
    MAC_CHBA_CHAN_SWITCH_IMMEDIATE = 0,
    MAC_CHBA_CHAN_SWITCH_NEXT_SLOT = 1,
    MAC_CHBA_CHAN_SWITCH_NEXT_NEXT_SLOT = 2,
} hmac_chba_chan_switch_slot_enum;

/* 3 �ṹ�嶨�� */
/* �ŵ��л�����/ָʾ����Ϣ�ṹ�� */
typedef struct {
    uint8_t notify_type; /* �ŵ��л�֪ͨ���ͣ�������ָʾ */
    uint8_t chan_number; /* ��20M���ŵ��� */
    uint8_t bandwidth; /* ����ģʽ */
    uint8_t switch_slot; /* ָʾ���ĸ�slotͷ�л��ŵ� */
    uint8_t status_code; /* CSN resp֡�ظ������ܻ��Ǿܾ� */
} hmac_chba_chan_switch_notify_stru;

/* 4 ȫ�ֱ������� */
extern uint8_t g_chan_switch_enable;

/* 5 �������� */
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
