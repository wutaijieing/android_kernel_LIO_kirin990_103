

#ifndef __WAL_CONFIG_BASE_H__
#define __WAL_CONFIG_BASE_H__

#include "hmac_vap.h"

#ifdef __cplusplus // windows ut���벻����������������
#if __cplusplus
extern "C" {
#endif
#endif
#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_CONFIG_BASE_H

int32_t wal_recv_config_cmd(uint8_t *puc_buf, uint16_t us_len);
#if defined(_PRE_WLAN_FEATURE_DATA_SAMPLE) || defined(_PRE_WLAN_FEATURE_PSD_ANALYSIS)
uint32_t wal_sample_report2sdt(frw_event_mem_stru *pst_event_mem);
#endif
uint32_t wal_dpd_report2sdt(frw_event_mem_stru *pst_event_mem);
void wal_drv_cfg_func_hook_init(void);
oal_bool_enum_uint8 wal_should_down_protocol_bw(mac_device_stru *mac_dev, mac_vap_stru *mac_vap);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif
