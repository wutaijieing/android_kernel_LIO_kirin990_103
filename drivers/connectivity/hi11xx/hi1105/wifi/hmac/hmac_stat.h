

#ifndef __HMAC_STAT_H__
#define __HMAC_STAT_H__

#include "hmac_stat_stru.h"
#include "hmac_vap.h"
#include "hmac_user.h"
#ifdef _PRE_WLAN_CHBA_MGMT
#include "hmac_chba_mgmt.h"
#include "hmac_chba_function.h"
#endif

struct hmac_device_tag;

void hmac_stat_init_device_stat(struct hmac_device_tag *hmac_device);

extern hmac_device_stat_stru g_hmac_device_stats;

OAL_STATIC OAL_INLINE hmac_device_stat_stru *hmac_stat_get_device_stats(void)
{
    return &g_hmac_device_stats;
}

OAL_STATIC OAL_INLINE hmac_vap_stat_stru *hmac_stat_get_vap_stat(hmac_vap_stru *hmac_vap)
{
    return &(hmac_vap->vap_stats);
}

OAL_STATIC OAL_INLINE hmac_user_stat_stru *hmac_stat_get_user_stat(hmac_user_stru *hmac_user)
{
    return &(hmac_user->user_stats);
}

OAL_STATIC OAL_INLINE void hmac_stat_device_tx_netbuf(uint32_t netbuf_len)
{
    hmac_device_stat_stru *hmac_device_stats = hmac_stat_get_device_stats();
    oal_atomic_add(&(hmac_device_stats->tx_packets), 1);
    oal_atomic_add(&(hmac_device_stats->tx_bytes), netbuf_len);
}

OAL_STATIC OAL_INLINE void hmac_stat_device_rx_netbuf(uint32_t netbuf_len)
{
    hmac_device_stat_stru *hmac_device_stats = hmac_stat_get_device_stats();
    oal_atomic_add(&(hmac_device_stats->rx_packets), 1);
    oal_atomic_add(&(hmac_device_stats->rx_bytes), netbuf_len);
}

/*
 * 更新接收netbuffer统计信息
 */
OAL_STATIC OAL_INLINE void hmac_stat_vap_rx_netbuf(hmac_vap_stru *hmac_vap, oal_netbuf_stru *netbuf)
{
    hmac_vap_stat_stru *hmac_vap_stat = NULL;

    if ((hmac_vap == NULL) || (netbuf == NULL)) {
        return;
    }

    hmac_vap_stat = hmac_stat_get_vap_stat(hmac_vap);

    /* 数据帧统计 */
    HMAC_VAP_STATS_PKT_INCR(hmac_vap_stat->rx_packets, 1);
    HMAC_VAP_STATS_PKT_INCR(hmac_vap_stat->rx_bytes, oal_netbuf_get_len(netbuf));
}

/*
 * 更新发送netbuffer统计信息
 */
OAL_STATIC OAL_INLINE void hmac_stat_vap_tx_netbuf(hmac_vap_stru *hmac_vap, oal_netbuf_stru *netbuf)
{
    hmac_vap_stat_stru *hmac_vap_stats = NULL;

    if ((hmac_vap == NULL) || (netbuf == NULL)) {
        return;
    }

    hmac_vap_stats = hmac_stat_get_vap_stat(hmac_vap);

    /* 数据帧统计 */
    HMAC_VAP_STATS_PKT_INCR(hmac_vap_stats->tx_packets, 1);
    HMAC_VAP_STATS_PKT_INCR(hmac_vap_stats->tx_bytes, oal_netbuf_get_len(netbuf));
}

#ifdef _PRE_WLAN_CHBA_MGMT
OAL_STATIC OAL_INLINE void hmac_stat_user_rx_netbuf(hmac_user_stru *hmac_user, oal_netbuf_stru *netbuf)
{
    hmac_user_stat_stru *hmac_user_stats = NULL;
    hmac_vap_stru *hmac_vap = NULL;
    if ((hmac_user == NULL) || (netbuf == NULL)) {
        return;
    }
    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(hmac_user->st_user_base_info.uc_vap_id);
    if (!hmac_chba_vap_start_check(hmac_vap)) {
        return;
    }
    hmac_user_stats = hmac_stat_get_user_stat(hmac_user);

    /* 数据帧统计 */
    HMAC_USER_STATS_PKT_INCR(hmac_user_stats->rx_packets, 1);
    HMAC_USER_STATS_PKT_INCR(hmac_user_stats->rx_bytes, oal_netbuf_get_len(netbuf));
}

OAL_STATIC OAL_INLINE void hmac_stat_user_tx_netbuf(hmac_user_stru *hmac_user, oal_netbuf_stru *netbuf)
{
    hmac_user_stat_stru *hmac_user_stats = hmac_stat_get_user_stat(hmac_user);

    /* 数据帧统计 */
    HMAC_USER_STATS_PKT_INCR(hmac_user_stats->tx_packets, 1);
    HMAC_USER_STATS_PKT_INCR(hmac_user_stats->tx_bytes, oal_netbuf_get_len(netbuf));
}
#endif
#endif /* __HMAC_STAT_H__ */

