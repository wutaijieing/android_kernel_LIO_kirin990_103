

#ifndef __HMAC_HOST_AL_TX_H__
#define __HMAC_HOST_AL_TX_H__

/* 1 其他头文件包含 */
#include "mac_common.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DRIVER_HMAC_TX_MSDU_DSCR_H

extern hmac_msdu_info_ring_stru g_always_tx_ring[WLAN_DEVICE_MAX_NUM_PER_CHIP];
uint32_t hmac_always_tx_proc(uint8_t *param, uint8_t hal_dev_id);


#endif
