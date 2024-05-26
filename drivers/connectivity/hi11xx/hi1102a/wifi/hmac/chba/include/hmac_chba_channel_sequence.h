/*
 * 版权所有 (c) 华为技术有限公司 2022-2022
 * 功能说明 : CHBA DBAC头文件
 * 作    者 : duankaiyong
 * 创建日期 : 2022年3月24日
 */

#ifndef __HMAC_CHBA_CHANNEL_SEQUENCE_H__
#define __HMAC_CHBA_CHANNEL_SEQUENCE_H__

#ifdef _PRE_WLAN_CHBA_MGMT
#include "hmac_vap.h"

void hmac_chba_set_channel_seq_bitmap(hmac_chba_vap_stru *chba_vap_info, uint32_t channel_seq_bitmap);
uint32_t hmac_chba_set_channel_seq_params(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params);

void hmac_chba_update_channel_seq(mac_vap_stru *mac_vap);
#endif

#endif /* __HMAC_CHBA_CHANNEL_SEQUENCE_H__ */
