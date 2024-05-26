

#ifndef __HMAC_TID_H__
#define __HMAC_TID_H__

#include "mac_common.h"
#include "oal_ext_if.h"

typedef struct {
    msdu_info_ring_stru base_ring_info;
    uint8_t *host_ring_buf;
    uint64_t host_ring_dma_addr;
    uint32_t host_ring_buf_size;
    uint16_t release_index;
    oal_atomic last_period_tx_msdu;
    oal_atomic msdu_cnt;
    oal_atomic enabled;
    uint8_t host_tx_flag;
    oal_netbuf_stru **netbuf_list;
    oal_spin_lock_stru tx_lock;
    oal_spin_lock_stru tx_comp_lock;
} hmac_msdu_info_ring_stru;

typedef union {
    uint32_t word2;
    struct {
        uint16_t write_ptr;
        uint16_t tx_msdu_ring_depth : 4;
        uint16_t max_aggr_amsdu_num : 4;
        uint16_t reserved : 8;
    } word2_bit;
} msdu_info_word2;

typedef union {
    uint32_t word3;
    struct {
        uint16_t read_ptr;
        uint16_t reserved;
    } word3_bit;
} msdu_info_word3;

#define TX_RING_INFO_LEN      16
#define TX_RING_INFO_WORD_NUM (TX_RING_INFO_LEN / sizeof(uint32_t))

typedef struct {
    /* 保存tx ring table的4个word地址,经devca_to_hostva转换为64位地址, 结构与hal_tx_msdu_info_ring_stru一致 */
    uint64_t word_addr[TX_RING_INFO_WORD_NUM];
    msdu_info_word2 msdu_info_word2;  /* wptr所属word2 */
    msdu_info_word3 msdu_info_word3;  /* rptr所属word3 */
} hmac_tx_ring_device_info_stru;

typedef struct {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    struct list_head tid_sche_entry;
    struct list_head tid_update_entry;
#else
    oal_dlist_head_stru tid_sche_entry;
    oal_dlist_head_stru tid_update_entry;
#endif
    oal_dlist_head_stru tid_tx_entry;
    oal_dlist_head_stru tid_ring_switch_entry;
    uint8_t tid_no;
    uint8_t is_in_ring_list; /* 该tid是否已存在tx ring切换链表 */
    uint16_t user_index;
    oal_atomic ring_tx_mode;         /* 发送模式host ring tx/device ring tx/device tx */
    oal_atomic ring_sync_th;
    oal_atomic tid_sche_th;
    oal_atomic ring_tx_opt;
    oal_atomic tid_scheduled;
    oal_atomic tid_updated;
    uint32_t ring_sync_cnt;
    hmac_tx_ring_device_info_stru *tx_ring_device_info;
    hmac_msdu_info_ring_stru tx_ring;
    oal_netbuf_head_stru tid_queue;
} hmac_tid_info_stru;

typedef uint8_t (* hmac_tid_list_func)(hmac_tid_info_stru *, void *);

typedef struct {
    hmac_tid_info_stru* (*tid_info_getter)(void *);
    void* (*entry_getter)(hmac_tid_info_stru *);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    uint8_t (*for_each_rcu)(hmac_tid_list_func, void *);
#endif
} hmac_tid_list_ops;

static inline uint8_t hmac_tx_ring_switching(hmac_tid_info_stru *tid_info)
{
    return oal_atomic_read(&tid_info->ring_tx_mode) != HOST_RING_TX_MODE &&
           oal_atomic_read(&tid_info->ring_tx_mode) != DEVICE_RING_TX_MODE;
}

static inline bool hmac_tid_ring_tx_opt(hmac_tid_info_stru *tid_info)
{
    return oal_atomic_read(&tid_info->ring_tx_opt);
}

static inline void hmac_tid_netbuf_enqueue(oal_netbuf_head_stru *tid_queue, oal_netbuf_stru *netbuf)
{
    oal_netbuf_list_tail(tid_queue, netbuf);
}

static inline oal_netbuf_stru *hmac_tid_netbuf_dequeue(oal_netbuf_head_stru *tid_queue)
{
    if (oal_netbuf_list_empty(tid_queue)) {
        return NULL;
    }

    return oal_netbuf_delist(tid_queue);
}

#endif
