

#ifndef __HMAC_CHBA_INTERFACE_H__
#define __HMAC_CHBA_INTERFACE_H__
/* 1 ����ͷ�ļ����� */
#include "hmac_device.h"
#include "hmac_vap.h"
#include "hmac_user.h"
#include "mac_chba_common.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_INTERFACE_H

#ifdef _PRE_WLAN_CHBA_MGMT
/* 2 �궨�� */
#define HMAC_MAC_CHBA_CHANNEL_NUM_2G                  32     /* 2G��ȫ�����ŵ� */
#define HMAC_MAC_CHBA_CHANNEL_NUM_5G                  73     /* 5G��ȫ�����ŵ� */
typedef uint32_t (*hmac_chba_vendor_cmd_func)(hmac_vap_stru *hmac_vap, uint8_t *cmd_buf);

/* 3 ö�ٶ��� */
/* 4 ȫ�ֱ������� */
/* 5 ��Ϣͷ���� */
/* 6 ��Ϣ���� */
/* 7 STRUCT���� */
typedef struct {
    uint8_t idx;                            /* ��� */
    uint8_t uc_chan_idx;                    /* ��20MHz�ŵ������� */
    uint8_t uc_chan_number;                 /* ��20MHz�ŵ��� */
    wlan_channel_bandwidth_enum_uint8 en_bandwidth; /* ����ģʽ */
} hmac_chba_chan_stru;
/* CHBA �������ýṹ�� */
typedef struct {
    uint8_t chba_cmd_type; /* ������������ mac_chba_cmd_msg_type_enum */
    hmac_chba_vendor_cmd_func func; /* �����Ӧ������ */
} hmac_chba_cmd_entry_stru;

/* HMAC��SUPPLICANT�Ľӿ� */
typedef struct {
    uint8_t op_id; /* ����ID */
    uint8_t peer_addr[OAL_MAC_ADDR_LEN];
    mac_status_code_enum_uint16 status_code; /* �������ϲ�Ľ�������� */
    uint32_t connect_timeout; /* ���ӳ�ʱʱ�� */
    mac_channel_stru assoc_channel;
} hmac_chba_conn_param_stru;

typedef struct {
    uint8_t id; /* connection notfiy id */
    uint8_t peer_mac_addr[WLAN_MAC_ADDR_LEN]; /* �Զ�MAC��ַ */
    uint16_t expire_time; /* ��ʱʱ�䣬msΪ��λ */
    mac_kernel_channel_stru channel; /* �����ŵ���Ϣ */
} hmac_chba_conn_notify_stru;

/* 8 UNION���� */
/* 9 OTHERS���� */
/* 10 �������� */
void hmac_chba_rcv_cmd_process(uint8_t cmd_id, uint8_t *cmd_buf, uint16_t buf_size);
void hmac_chba_vap_start(uint8_t *mac_addr, mac_channel_stru *social_channel,
    mac_channel_stru *work_channel);
void hmac_chba_vap_stop(void);
void hmac_chba_conn_info(uint16_t report_type,
    uint8_t *mac_addr, uint8_t *peer_mac_addr, mac_channel_stru *mac_channel);
void hmac_chba_stub_channel_switch_report(hmac_vap_stru *hmac_vap, mac_channel_stru *target_channel);
void hmac_chba_ask_for_sync(uint8_t chan_number, uint8_t *peer_addr, uint8_t *master_addr);

uint32_t hmac_chba_params_sync(mac_vap_stru *mac_vap, chba_params_config_stru *chba_params_sync);
uint32_t hmac_chba_params_sync_after_start(mac_vap_stru *mac_vap, hmac_chba_vap_stru *chba_vap);
uint32_t hmac_config_vap_discovery_channel(mac_vap_stru *mac_vap, hmac_chba_vap_stru *chba_vap_info);
uint32_t hmac_chba_update_work_channel(mac_vap_stru *mac_vap);
uint32_t hmac_chba_update_beacon_pnf(mac_vap_stru *mac_vap, uint8_t *domain_bssid, uint8_t frame_type);
uint32_t hmac_chba_chan_convert_center_freq_to_bw(mac_kernel_channel_stru *kernel_channel,
    mac_channel_stru *mac_channel);
uint32_t hmac_chba_chan_convert_bw_to_center_freq(mac_kernel_channel_stru *kernel_channel,
    mac_channel_stru *mac_channel, mac_vap_stru *mac_vap);
#endif

#endif /* end of this file */

