

#ifndef __HMAC_CHBA_FUNCTION_H__
#define __HMAC_CHBA_FUNCTION_H__
/* 1 ����ͷ�ļ����� */
#include "hmac_device.h"
#include "hmac_vap.h"
#include "hmac_user.h"
#include "mac_chba_common.h"
#include "hmac_chba_interface.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_FUNCTION_H

#ifdef _PRE_WLAN_CHBA_MGMT
/* 2 �궨�� */
/* ͬ����غ� */
#define MAC_CHBA_SYNC_REQUEST_CNT 3
#define MAC_CHBA_CHANNEL_SWITCH_OVERHEAD_MS 5 /* ģ���ŵ��л�ʱ�� */
#define MAC_CHBA_SYNC_WAITING_TIMEOUT 48
/* ������غ� */
#define CHBA_WHITELIST_TIMEOUT_MS 2000 * 1000
#define CHBA_ASSOC_WAITING_TIMEOUT 100 /* 100ms */
#define CHBA_MAX_ASSOC_CNT 3
#define CHBA_SWITCH_STA_PSM_PERIOD 60000  /* chba�򿪵͹��ĳ�ʱ��ʱ��60s */

/* 3 ö�ٶ��� */
/* chba���ӽ�ɫö�� */
typedef enum {
    CHBA_CONN_INITIATOR = 0, /* ���������� */
    CHBA_CONN_RESPONDER = 1, /* ������Ӧ�� */

    CHBA_CONN_ROLE_BUTT,
} chba_connect_role_enum;

/* chba user����״̬ö�� */
/* ����״̬ΪUP״̬ʱ������mac_user_stru�е�en_user_asoc_state����ΪMAC_USER_STATE_ASSOC */
typedef enum {
    CHBA_USER_STATE_INIT = 0,
    CHBA_USER_STATE_JOIN_COMPLETE = 1,
    CHBA_USER_STATE_WAIT_AUTH_SEQ2 = 2,
    CHBA_USER_STATE_WAIT_AUTH_SEQ4 = 3,
    CHBA_USER_STATE_AUTH_COMPLETE = 4,
    CHBA_USER_STATE_WAIT_ASSOC = 5,
    CHBA_USER_STATE_LINK_UP = 6,

    CHBA_USER_STATE_BUTT,
} chba_user_state_enum;

/* 4 ȫ�ֱ������� */
/* 5 ��Ϣͷ���� */
/* 6 ��Ϣ���� */
/* 7 STRUCT���� */
/* ������: ����NEARBY�豸��֤����Peer */
typedef struct {
    uint8_t peer_mac_addr[OAL_MAC_ADDR_LEN]; /* �Է�mac��ַ */
} whitelist_stru;

typedef struct {
    uint16_t schedule_period; /* �������ڵ�ʱ϶������Ĭ��Ϊ32 */
    uint16_t sync_slot_cnt; /* ͬ��ʱ϶������Ĭ��Ϊ2 */
    uint16_t schedule_time_ms; /* ��������ʱ�䣬16 * schedule_period */
    uint16_t vap_alive_timeout; /* vap��������·״̬���ʱ϶������Ĭ��Ϊ1024 */
    uint16_t link_alive_timeout; /* ��·������ʱ϶������Ĭ��Ϊ1024 */
} hmac_chba_system_params;

/* ����chba��صĳ�Ա����������hmac_vap_stru�� */
struct hmac_chba_vap_tag {
    uint8_t mac_vap_id;
    uint8_t need_recovery_work_channel;
    uint8_t chba_log_level;
    uint8_t vap_bitmap_level; /* vap�͹���bitmap */

    uint32_t channel_sequence_bitmap;
    mac_channel_stru social_channel;
    mac_channel_stru work_channel; /* ��¼�������ŵ� */
    mac_channel_stru second_work_channel; /* ��¼CHBA�ڶ��������ŵ� */

    uint8_t pnf_buf[MAC_CHBA_MGMT_MID_PAYLOAD_BYTES];
    uint8_t beacon_buf[MAC_CHBA_MGMT_MAX_PAYLOAD_BYTES];
    uint16_t pnf_buf_len;
    uint16_t beacon_buf_len;

    whitelist_stru whitelist; /* ���澭��Nearby�豸��֤����peer�豸��Ϣ */
    /* chba debug��� */
    chba_bitmap_switch_enum auto_bitmap_switch; /* ��̬����bitmap���� */
    uint8_t is_support_dfs_channel;
    /* ���豸�����ŵ���Ϣ */
    uint8_t self_coex_chan_cnt;
    uint8_t is_support_multiple_island; /* �Ƿ�֧��һ��ൺ */
    uint8_t resv[1];
    uint8_t self_coex_channels_list[MAX_CHANNEL_NUM_FREQ_5G];
};

typedef struct {
    uint8_t intf_rpt_slot;
    uint8_t switch_chan_num;
    uint8_t switch_chan_bw;
} mac_set_chan_switch_test_params;
typedef struct {
    uint8_t auc_mac_addr[WLAN_MAC_ADDR_LEN]; /* MAC��ַ */
    uint8_t ps_bitmap; /* bitmap��Ϣ */
} mac_chba_set_ps_bitmap_params;

extern uint32_t g_discovery_bitmap;
/* 8 UNION���� */
/* 9 OTHERS���� */
/* 10 �������� */
hmac_chba_system_params *hmac_get_chba_system_params(void);
hmac_chba_vap_stru *hmac_get_chba_vap(void);
hmac_chba_vap_stru *hmac_get_up_chba_vap_info(void);
oal_bool_enum_uint8 hmac_chba_vap_start_check(hmac_vap_stru *hmac_vap);
uint32_t hmac_config_chba_base_params(mac_chba_set_init_para *init_params);
uint32_t hmac_start_chba(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param);
uint32_t hmac_stop_chba(mac_vap_stru *mac_vap);
void hmac_update_chba_info_aftet_vap_del_user(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user);
uint32_t hmac_chba_request_sync(mac_vap_stru *mac_vap, hmac_chba_vap_stru *chba_vap_info,
    mac_chba_set_sync_request *sync_req_params);
uint32_t hmac_chba_d2h_sync_event(mac_vap_stru *mac_vap, uint8_t len, uint8_t *param);
uint32_t hmac_chba_initiate_connect(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params);
uint32_t hmac_chba_response_connect(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params);
void hmac_rx_assoc_req_action(hmac_vap_stru *hmac_vap, mac_ieee80211_frame_stru *mac_hdr,
    uint16_t mac_hdr_len, uint8_t *payload, uint16_t payload_len);

void hmac_generate_chba_domain_bssid(uint8_t *bssid, uint8_t bssid_len, uint8_t *mac_addr, uint8_t mac_addr_len);
void hmac_chba_vap_update_domain_bssid(hmac_vap_stru *hmac_vap, uint8_t *mac_addr);
uint32_t hmac_chba_config_channel(mac_vap_stru *mac_vap, hmac_chba_vap_stru *chba_vap_info,
    uint8_t channel_number, uint8_t bandwidth);
uint32_t hmac_chba_vap_coex_check(hmac_chba_vap_stru *chba_vap_info, mac_channel_stru *new_chba_channel);

uint32_t hmac_chba_rcv_domain_merge_params(hmac_vap_stru *hmac_vap, uint8_t *buf);
uint32_t hmac_user_set_asoc_req_ie(hmac_user_stru *hmac_user,
                                   uint8_t *payload, uint32_t payload_len, uint8_t reass_flag);
uint32_t hmac_user_free_asoc_req_ie(uint16_t us_idx);

void hmac_chba_connect_succ_handler(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user,
    uint8_t *payload, uint16_t payload_len);
uint32_t hmac_chba_whitelist_check(hmac_vap_stru *hmac_vap, uint8_t *peer_addr, uint8_t addr_len);
uint32_t hmac_chba_whitelist_clear(hmac_vap_stru *hmac_vap);
void hmac_update_chba_vap_info(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user);
uint32_t hmac_chba_topo_info_sync(void);
uint32_t hmac_chba_device_info_sync(uint8_t update_type, uint8_t device_id, const uint8_t *mac_addr);
void hmac_chba_cur_master_info_init(uint8_t *own_mac_addr, mac_channel_stru *work_channel);
void hmac_chba_sync_process_after_asoc(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user);
void hmac_config_social_channel(hmac_chba_vap_stru *chba_vap_info, uint8_t channel_number, uint8_t bandwidth);
hmac_chba_vap_stru *hmac_chba_get_chba_vap_info(hmac_vap_stru *hmac_vap);
hmac_vap_stru *hmac_chba_find_chba_vap(mac_device_stru *mac_device);
uint32_t hmac_chba_is_valid_channel(uint8_t chan_number);
#endif
#endif /* end of this file */
