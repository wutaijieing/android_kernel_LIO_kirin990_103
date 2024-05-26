

#ifndef __HMAC_RX_FILTER_H__
#define __HMAC_RX_FILTER_H__

/* 1 ����ͷ�ļ����� */
#include "oal_types.h"
#include "mac_vap.h"
#include "mac_device.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_RX_FILTER_H
/* 2 �궨�� */
/* 3 ö�ٶ��� */
/* 4 ȫ�ֱ������� */
/* 5 ��Ϣͷ���� */
/* 6 ��Ϣ���� */
/* 7 STRUCT���� */
/* 8 UNION���� */
/* 9 OTHERS���� */
/* 10 �������� */
oal_bool_enum_uint8 hmac_find_is_sta_up(mac_device_stru *pst_mac_device);

uint32_t hmac_calc_up_ap_num(mac_device_stru *pst_mac_device);
uint32_t hmac_find_up_vap(mac_device_stru *pst_mac_device, mac_vap_stru **ppst_mac_vap);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of hmac_rx_filter.h */