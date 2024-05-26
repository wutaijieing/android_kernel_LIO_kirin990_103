

#ifndef __HMAC_VSP_STAT_H__
#define __HMAC_VSP_STAT_H__

#include "wlan_types.h"

#ifdef _PRE_WLAN_FEATURE_VSP

typedef struct {
    uint32_t last_time;
    uint32_t slice_total;
    uint32_t slice_last;
    uint32_t slice_rate;
    uint64_t bytes_total;
    uint64_t bytes_last;
    uint32_t data_rate;
} hmac_vsp_stat_stru;

static inline uint32_t hmac_vsp_stat_get_last_time(hmac_vsp_stat_stru *stat)
{
    return stat->last_time;
}

static inline void hmac_vsp_stat_update_last_time(hmac_vsp_stat_stru *stat, uint32_t ts)
{
    stat->last_time = ts;
}

static inline uint32_t hmac_vsp_stat_get_slice_total(hmac_vsp_stat_stru *stat)
{
    return stat->slice_total;
}

static inline void hmac_vsp_stat_inc_slice_total(hmac_vsp_stat_stru *stat)
{
    stat->slice_total++;
}

static inline uint32_t hmac_vsp_stat_get_slice_last(hmac_vsp_stat_stru *stat)
{
    return stat->slice_last;
}

static inline void hmac_vsp_stat_update_slice_last(hmac_vsp_stat_stru *stat)
{
    stat->slice_last = stat->slice_total;
}

static inline uint32_t hmac_vsp_stat_get_slice_rate(hmac_vsp_stat_stru *stat)
{
    return stat->slice_rate;
}

static inline void hmac_vsp_stat_update_slice_rate(hmac_vsp_stat_stru *stat, uint32_t runtime)
{
    stat->slice_rate = runtime ? (stat->slice_total - stat->slice_last) / runtime : stat->slice_rate;
}

static inline uint64_t hmac_vsp_stat_get_bytes_total(hmac_vsp_stat_stru *stat)
{
    return stat->bytes_total;
}

static inline void hmac_vsp_stat_update_bytes_total(hmac_vsp_stat_stru *stat, uint64_t bytes)
{
    stat->bytes_total += bytes;
}

static inline uint64_t hmac_vsp_stat_get_bytes_last(hmac_vsp_stat_stru *stat)
{
    return stat->bytes_last;
}

static inline void hmac_vsp_stat_update_bytes_last(hmac_vsp_stat_stru *stat)
{
    stat->bytes_last = stat->bytes_total;
}

static inline uint32_t hmac_vsp_stat_get_data_rate(hmac_vsp_stat_stru *stat)
{
    return stat->data_rate;
}

static inline void hmac_vsp_stat_update_data_rate(hmac_vsp_stat_stru *stat, uint32_t runtime)
{
    uint64_t value;

    value = (stat->bytes_total - stat->bytes_last) * 8;
    do_div(value, runtime * 1000);
    stat->data_rate = runtime ? value : stat->data_rate;
}

static inline void hmac_vsp_stat_calc_rate(hmac_vsp_stat_stru *stat)
{
    uint32_t now = oal_time_get_stamp_ms();
    uint32_t runtime = oal_time_get_runtime(hmac_vsp_stat_get_last_time(stat), now) / 1000;
    /* 计算间隔2s以上 */
    if (runtime < 2) {
        return;
    }

    hmac_vsp_stat_update_slice_rate(stat, runtime);
    hmac_vsp_stat_update_data_rate(stat, runtime);
    hmac_vsp_stat_update_slice_last(stat);
    hmac_vsp_stat_update_bytes_last(stat);
    hmac_vsp_stat_update_last_time(stat, now);
}

#endif
#endif
