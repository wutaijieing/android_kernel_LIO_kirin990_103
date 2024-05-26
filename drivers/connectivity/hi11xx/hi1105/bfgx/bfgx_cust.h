

#ifndef __BFGX_UART_CUST_H__
#define __BFGX_UART_CUST_H__

/* 其他头文件包含 */
#include "plat_type.h"
#define SKB_ALLOC_RETRY_DEFAULT_COUNT 3
void baudrate_init(void);
uint16_t get_skb_retry_count(void);
#endif