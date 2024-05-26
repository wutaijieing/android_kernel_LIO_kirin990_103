

#ifndef __HMAC_CHBA_PS_H__
#define __HMAC_CHBA_PS_H__

#include "hmac_chba_mgmt.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_PS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _PRE_WLAN_CHBA_MGMT
/* Ŀǰ�������㷨�Ĵ��ڳ���һ��Ϊ5����ʹ�����5��ͳ�����ڵĽ�����õ�bitmap */
#define CONTINUOUS_WINDOW_LEN 5

/* Ŀǰʹ����ֵ3���ж�����㷨�������Ƿ����㹻��tx rx��Ϊ0��tx rx��һ����Ϊ0����� */
#define ENOUGH_PACKETS_THRESHOLD 3

/* Ŀǰʹ����ֵ2���ж�����㷨�����д����������ֵ��� */
#define LARGE_PACKET_THRESHOLD 2

/* Ŀǰ��Ϊ����2000bytes�İ�Ϊ��� */
#define LAGRE_PACKET_BYTES_THRESHOLD 2000

#define BITMAP_WORK_NOW 1
#define BITMAP_WORK_LATER 0
#define CHBA_MAX_PS_THROUGHT_THRES_LEVEL 3 /* chba�͹��������������ȼ� */
/* ��bitmap����Ҫ������Чʱ��work_chan_update_cnt��Ϊ255 */
#define WORK_CHAN_UPDATE_CNT_INVALID 255

#define hmac_chba_get_ps_switch() (g_ps_feature_enable)
#define hmac_chba_set_ps_switch(val) (g_ps_feature_enable = (val))
/* �ж�chba user�Ƿ���������ģʽ */
#define hmac_chba_user_is_in_zero_throughput(_trx_cnt) \
    ((CHBA_THROUGHPUT_WINDOW_LEN - (_trx_cnt)) >= ENOUGH_PACKETS_THRESHOLD)
/* �ж�chba user�Ƿ������� */
#define hmac_chba_user_is_in_trxing(_trx_cnt) ((_trx_cnt) >= ENOUGH_PACKETS_THRESHOLD)
/* �ж�chba user�Ƿ��ڸ�����ģʽ */
#define hmac_chba_user_is_in_high_throughput(_high_trx_cnt) ((_high_trx_cnt) >= LARGE_PACKET_THRESHOLD)

typedef enum {
    CHBA_PS_THRES_LEVEL_THREE_TO_TWO = 0,
    CHBA_PS_THRES_LEVEL_TWO_TO_ONE = 1,
    CHBA_PS_THRES_LEVEL_ONE_TO_TWO = 2,
    CHBA_PS_THRES_LEVEL_TWO_TO_THREE = 3,

    CHBA_PS_THRES_LEVEL_BUTT
} chba_ps_thres_level_type_enum; /* chba�͹���������������ö�� */

typedef struct {
    uint32_t upgrade_to_lv_two_thres; /* ����͵����ڶ�������������(���޵�λ 100ms) */
    uint32_t upgrade_to_lv_one_thres; /* �����͹��ĵ�һ������������(���޵�λ 100ms) */
    uint32_t downgrade_to_lv_two_thres; /* �����͹��ĵڶ�������������(���޵�λ 100ms) */
    uint32_t downgrade_to_lv_three_thres; /* �����͹�����͵�����������(���޵�λ 100ms) */
} hmac_chba_ps_throught_thres_stru; /* chba�͹����������޽ṹ��ṹ�� */

typedef struct {
    uint8_t thres_type;
    uint8_t resv[3];
    uint32_t thres_val;
} chba_ps_thres_config_stru; /* chba�͹��������������ýṹ�� */
extern uint8_t g_ps_feature_enable;

void hmac_chba_update_throughput_info(hmac_chba_vap_stru *chba_vap_info);
void hmac_chba_buffer_over_th(uint8_t *recv_msg);
/* �ṩ����notify�����޵Ľӿ� */
void hmac_chba_sync_vap_bitmap_info(mac_vap_stru *mac_vap, hmac_chba_vap_stru *chba_vap_info);
void hmac_chba_sync_user_bitmap_info(mac_vap_stru *mac_vap, hmac_chba_vap_stru *chba_vap_info,
    hmac_user_stru *hmac_user, uint8_t tmp_ps_level);
uint32_t hmac_chba_set_user_bitmap_debug(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params);
uint32_t hmac_chba_update_user_bitmap_level(mac_vap_stru *mac_vap, oal_uint8 len, oal_uint8 *param);
uint32_t hmac_chba_update_vap_bitmap_level(mac_vap_stru *mac_vap, oal_uint8 len, oal_uint8 *param);
uint32_t hmac_chba_set_vap_bitmap_debug(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params);
uint32_t hmac_chba_auto_bitmap_debug(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params);
void hmac_chba_user_ps_info_init(hmac_user_stru *hmac_user);
hmac_chba_ps_throught_thres_stru *hmac_chba_get_ps_throught_thres(hmac_vap_stru *hmac_vap);
void hmac_chba_throughput_statis_init(void);
uint32_t hmac_chba_update_topo_bitmap_info_proc(mac_vap_stru *mac_vap, oal_uint8 len, oal_uint8 *param);
void hmac_chba_update_throughput(void);
#endif

#ifdef __cplusplus
}
#endif

#endif

