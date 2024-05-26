

#ifndef __HMAC_CHBA_COMMON_FUNCTION_H__
#define __HMAC_CHBA_COMMON_FUNCTION_H__

#include "securec.h"
#include "hmac_device.h"
#include "hmac_vap.h"
#include "hmac_user.h"
#include "mac_chba_common.h"
#include "hmac_chba_interface.h"
#include "oal_cfg80211.h"
#include "hihash.h"
#include "hilist.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_COMMON_FUNCTION_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _PRE_WLAN_CHBA_MGMT
/* 1 宏定义 */
#define MIN_INT8 0x80
#define UINT8_INVALID_VALUE 0xFF /* uint8_t的最大值，作为一些变量的无效值 */
#define UINT32_BIT_NUM 32
#define UINT32_MAX_BIT_IDX 31

#define HMAC_CHBA_BYTE_LEN_1 1

#define HMAC_CHBA_TINY_BUFF_SIZE 64
#define HMAC_CHBA_BUFF_SIZE 128
#define HMAC_CHBA_LARGE_BUFF_SIZE 256
#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif
/* 2 枚举定义 */
/* 3 结构体定义 */
/* kernel信道表示 */
typedef struct {
    uint32_t center_freq20m; /* 主20M的频点 */
    uint32_t center_freq1; /* 整个带宽的频点 */
    uint32_t center_freq2; /* 80+80下使用 */
    uint8_t bandwidth; /* 带宽 */
} kernel_channel_stru;

/* 4 全局变量声明 */
/* 5 函数声明 */
uint32_t hmac_chba_set_vendor_ie_hdr(uint8_t *buf);
void hmac_chba_generate_domain_bssid(uint8_t *bssid, uint8_t *mac_addr, uint8_t addr_len);
uint8_t hmac_chba_island_device_check(uint8_t *peer_addr);

uint8_t hmac_chba_find_element(uint8_t *set_s, uint8_t cnt, uint8_t element);
void hmac_chba_fill_sync_state_field(uint8_t *sync_state);

int32_t hmac_chba_create_timer_ms(frw_timeout_stru *timer, uint32_t timeout, frw_timeout_func handle);

void hmac_chba_set_channel(uint8_t *channel_dst, uint8_t *channel_src, uint16_t size);
uint8_t hmac_chba_is_in_device_list(uint8_t device_id, uint8_t device_cnt, uint8_t *device_list);
uint32_t hmac_chba_set_feature_switch(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params);
#endif

#ifdef __cplusplus
}
#endif

#endif

