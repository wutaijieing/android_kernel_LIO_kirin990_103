

#ifndef __HMAC_EDCA_OPT_H__
#define __HMAC_EDCA_OPT_H__

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
#include "hmac_user.h"
#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "mac_device.h"
#include "mac_common.h"
#include "oam_ext_if.h"

#define HMAC_EDCA_OPT_MIN_PKT_LEN 256 /* С�ڸó��ȵ�ip���Ĳ���ͳ�ƣ��ų�chariot���Ʊ��� */

#define HMAC_EDCA_OPT_TIME_MS 30000 /* edca��������Ĭ�϶�ʱ�� */

#define HMAC_EDCA_OPT_PKT_NUM ((HMAC_EDCA_OPT_TIME_MS) >> 3) /* ƽ��ÿ���뱨�ĸ��� */

#define WLAN_EDCA_OPT_MAX_WEIGHT_STA (3)
#define WLAN_EDCA_OPT_WEIGHT_STA     (2)

#define HMAC_IDLE_CYCLE_TH (5) /* ������������������ */

void hmac_edca_opt_rx_pkts_stat(hmac_user_stru *hmac_user, uint8_t tidno, mac_ip_header_stru *ip_hdr);
void hmac_edca_opt_tx_pkts_stat(hmac_user_stru *hmac_user, uint8_t tidno, mac_ip_header_stru *ip_hdr, uint16_t ip_len);
uint32_t hmac_edca_opt_timeout_fn(void *p_arg);
#endif /* end of _PRE_WLAN_FEATURE_EDCA_OPT_AP */

#endif /* end of hmac_edca_opt.h */
