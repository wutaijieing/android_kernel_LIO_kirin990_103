

#ifndef __OAL_SOFTWDT_H__
#define __OAL_SOFTWDT_H__
#include "oal_types.h"

typedef struct softwdt_check {
    /* thread stat */
    unsigned long xmit_cnt;
    unsigned long hcc_cnt;
    unsigned long rxdata_cnt;
    unsigned long rxtask_cnt;
    unsigned long frw_cnt;
    // 入队skb、出队
    unsigned long napi_enq_cnt;
    unsigned long napi_sched_cnt;
    unsigned long netif_rx_cnt;
    unsigned long napi_poll_cnt;
    /* rxdata schd stat */
    unsigned long rxshed_napi;
    unsigned long rxshed_ap;
    unsigned long rxshed_sta;
    unsigned long rxshed_lan;
    unsigned long rxshed_msdu;
    unsigned long rxshed_rpt;
} softwdt_check_stru;
extern softwdt_check_stru g_hisi_softwdt_check;

#ifdef _PRE_CONFIG_HISI_CONN_SOFTWDFT
int32_t oal_softwdt_init(void);
void oal_softwdt_exit(void);
#else
OAL_STATIC OAL_INLINE int32_t oal_softwdt_init(void)
{
    return OAL_SUCC;
}
OAL_STATIC OAL_INLINE void oal_softwdt_exit(void)
{
    return;
}
#endif

#endif /* end of oal_softwdt.h */
