

#ifndef __HMAC_DEGRADATION_H_
#define __HMAC_DEGRADATION_H_

/* 1 ����ͷ�ļ����� */
#include "oal_ext_if.h"
#include "mac_vap.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_DEGRADATION_H

/* 2 �궨�� */

/* 3 �������� */
void hmac_degradation_tx_chain_switch(mac_vap_stru *mac_vap, uint8_t degradation_enable);
#endif /* end of __HMAC_MCM_DEGRADATION_H_ */

