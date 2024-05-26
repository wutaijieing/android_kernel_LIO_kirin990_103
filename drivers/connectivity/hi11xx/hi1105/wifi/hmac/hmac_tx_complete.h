

#ifndef __HMAC_TX_COMPLETE_H__
#define __HMAC_TX_COMPLETE_H__

#include "mac_vap.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_TX_COMPLETE_H
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

typedef struct {
    struct semaphore tx_comp_sema;
    oal_atomic hal_dev_tx_comp_triggered[WLAN_DEVICE_MAX_NUM_PER_CHIP];
    oal_task_stru *tx_comp_thread;
} hmac_tx_comp_stru;

extern hmac_tx_comp_stru g_tx_comp_mgmt;

static inline hmac_tx_comp_stru *hmac_get_tx_comp_mgmt(void)
{
    return &g_tx_comp_mgmt;
}

static inline void hmac_set_tx_comp_triggered(uint8_t hal_dev_id)
{
    oal_atomic_set(&g_tx_comp_mgmt.hal_dev_tx_comp_triggered[hal_dev_id], OAL_TRUE);
}

static inline void hmac_clear_tx_comp_triggered(uint8_t hal_dev_id)
{
    oal_atomic_set(&g_tx_comp_mgmt.hal_dev_tx_comp_triggered[hal_dev_id], OAL_FALSE);
}

static inline uint8_t hmac_get_tx_comp_triggered(uint8_t hal_dev_id)
{
    return oal_atomic_read(&g_tx_comp_mgmt.hal_dev_tx_comp_triggered[hal_dev_id]) == OAL_TRUE;
}

static inline void hmac_tx_comp_scheduled(void)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    up(&g_tx_comp_mgmt.tx_comp_sema);
#endif
}

void hmac_tx_comp_init(void);
int32_t hmac_tx_comp_thread(void *p_data);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
