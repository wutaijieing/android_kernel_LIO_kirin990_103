
#ifndef __HMAC_VSP_TEST_H__
#define __HMAC_VSP_TEST_H__

#include "oal_types.h"

#ifdef _PRE_WLAN_FEATURE_VSP
void hmac_vsp_test_start_source(uint32_t frame_cnt);
void hmac_vsp_test_start_sink(void);
void hmac_vsp_test_stop(void);
#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
void hmac_vsp_init_sysfs(void);
void hmac_vsp_deinit_sysfs(void);
#endif
#endif
#endif
