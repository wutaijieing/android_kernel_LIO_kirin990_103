
#ifndef __HMAC_GNSS_RSMC_H__
#define __HMAC_GNSS_RSMC_H__

#ifdef _PRE_WLAN_FEATURE_GNSS_RSMC
#include "mac_vap.h"
uint32_t hmac_process_gnss_rsmc_status_sync(mac_vap_stru *mac_vap, uint8_t len, uint8_t *param);
uint32_t hmac_process_gnss_rsmc_status_cmd(mac_vap_stru *mac_vap, uint32_t *params);
#endif

#endif
