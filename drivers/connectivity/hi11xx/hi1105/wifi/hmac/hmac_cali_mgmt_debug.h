

#ifndef __HMAC_CALI_MGMT_DEBUG_H__
#define __HMAC_CALI_MGMT_DEBUG_H__

/* 1 ����ͷ�ļ����� */
#include "frw_ext_if.h"

#include "hmac_vap.h"
#include "plat_cali.h"
#include "wlan_spec.h"
#include "wlan_cali.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CALI_MGMT_DEBUG_H

void hmac_dump_cali_result_shenkuo(void);
void hmac_dump_cali_result(void);
void hmac_dump_cali_result_1105(void);

#endif
