

#ifndef __MAC_DELAY_H__
#define __MAC_DELAY_H__

/* 1 头文件包含 */
#include "oal_types.h"
#include "wlan_types.h"

typedef enum {
    HAL_LOG_TX_LOG = BIT0,
    HAL_LOG_RX_LOG = BIT1,
    HAL_LOG_TX_DELAY_LOG = BIT2,
    HAL_LOG_RX_DELAY_LOG = BIT3,
    HAL_LOG_TX_DSCR_DELAY_LOG = BIT4,
    HAL_LOG_RX_DSCR_DELAY_LOG = BIT5,
    HAL_LOG_TX_SCHED_LOG = BIT6,
    HAL_LOG_TRX_BUT
} hal_debug_log_type_enum;

#ifdef _PRE_DELAY_DEBUG
typedef enum {
    HOST_TX_SKB_XMIT = 0,
    HOST_TX_SKB_HCC = 1,
    HOST_TX_SKB_ETE = 2,
    HAL_TX_SKB_ETE = 3,
    HAL_TX_SKB_HCC = 4,
    HAL_TX_SKB_DSCR = 5,

    HAL_TX_SKB_BUT
} hal_tx_skb_delay_trace_enum;

typedef enum {
    HAL_RX_SKB_START = 0,
    HAL_RX_SKB_REPORT = 1,
    HAL_RX_SKB_ETE = 2,
    HOST_RX_SKB_ETE = 3,
    HOST_RX_SKB_HCC = 4,
    HOST_RX_SKB_NAPI = 5,

    HOST_RX_SKB_BUT
} hal_rx_skb_delay_trace_enum;

typedef struct {
    uint8_t lut_idx      : 4;
    uint8_t tid_no       : 3;
    uint8_t hal_dev_id   : 1;
    uint8_t  reserve;
    uint16_t alloc_alg;
    uint16_t alg_txfifo;
    uint16_t txfifo_isr;
    uint16_t isr_dur;
    uint16_t isr_event;
    uint16_t event_prealg;
    uint16_t alg_free;
} hal_tx_dscr_delay_stru;
typedef struct {
    uint8_t lut_idx      : 4;
    uint8_t tid_no       : 3;
    uint8_t hal_dev_id   : 1;
    uint8_t  reserve;
    uint16_t irq_part;
    uint16_t irq_event_part;
    uint16_t dscr_proc_done;
    uint16_t skb_proc_done;
} hal_rx_dscr_delay_stru;
typedef struct {
    uint8_t lut_idx      : 4;
    uint8_t tid_no       : 3;
    uint8_t hal_dev_id   : 1;
    uint8_t  skb_len;
    uint16_t rxskb_hcc;
    uint16_t hcc_eteslave;
    uint16_t ete_slave_master;
    uint16_t etemaster_hcc;
    uint16_t hcc_netifrx;
} hal_skb_rx_delay_stru;

typedef struct {
    uint8_t lut_idx      : 4;
    uint8_t tid_no       : 3;
    uint8_t hal_dev_id   : 1;
    uint8_t  skb_len; // 8字节
    uint16_t xmit_hcc;
    uint16_t hcc_etemaster;
    uint16_t ete_master_slave;
    uint16_t eteslave_hcc;
    uint16_t hcc_txfree;
} hal_skb_tx_delay_stru;

typedef enum {
    TX_SKB_RPT = 0,
    TX_DSCR_RPT = 1,
    RX_SKB_RPT = 2,
    RX_DSCR_RPT = 3,
    TX_SCHED_RPT = 4,
    RX_E2E_SKB_RPT = 5,
} rpt_type_enum;
typedef struct {
    uint32_t rpt_type : 4;
    uint32_t resv     : 28;
} rpt_ctl_stru;
#endif

#endif /* end of hal_common.h */
