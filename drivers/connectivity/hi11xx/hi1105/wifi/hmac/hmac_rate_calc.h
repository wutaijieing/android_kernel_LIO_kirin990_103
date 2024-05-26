


#ifndef __HMAC_RATE_CALC_H__
#define __HMAC_RATE_CALC_H__

#include "wlan_spec.h"
#include "wlan_types.h"
#include "hal_common.h"

#ifdef _PRE_WLAN_FEATURE_SNIFFER

uint32_t hmac_get_rate_kbps(hal_statistic_stru *rate_info, uint32_t *rate_kbps);

#endif
#endif /* end of alg_rate_table.h */
