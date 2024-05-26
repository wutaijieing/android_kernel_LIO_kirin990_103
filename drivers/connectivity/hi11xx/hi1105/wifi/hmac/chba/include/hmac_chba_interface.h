

#ifndef __HMAC_CHBA_INTERFACE_H__
#define __HMAC_CHBA_INTERFACE_H__
/* 1 其他头文件包含 */
#include "hmac_device.h"
#include "hmac_vap.h"
#include "hmac_user.h"
#include "mac_chba_common.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_INTERFACE_H

#ifdef _PRE_WLAN_CHBA_MGMT
/* 2 宏定义 */
#define HMAC_MAC_CHBA_CHANNEL_NUM_2G                  32     /* 2G上全部的信道 */
#define HMAC_MAC_CHBA_CHANNEL_NUM_5G                  73     /* 5G上全部的信道 */
typedef uint32_t (*hmac_chba_vendor_cmd_func)(hmac_vap_stru *hmac_vap, uint8_t *cmd_buf);

/* 3 枚举定义 */
/* 4 全局变量声明 */
/* 5 消息头定义 */
/* 6 消息定义 */
/* 7 STRUCT定义 */
typedef struct {
    uint8_t idx;                            /* 序号 */
    uint8_t uc_chan_idx;                    /* 主20MHz信道索引号 */
    uint8_t uc_chan_number;                 /* 主20MHz信道号 */
    wlan_channel_bandwidth_enum_uint8 en_bandwidth; /* 带宽模式 */
} hmac_chba_chan_stru;
/* CHBA 命令配置结构体 */
typedef struct {
    uint8_t chba_cmd_type; /* 配置命令类型 mac_chba_cmd_msg_type_enum */
    hmac_chba_vendor_cmd_func func; /* 命令对应处理函数 */
} hmac_chba_cmd_entry_stru;

/* HMAC与SUPPLICANT的接口 */
typedef struct {
    uint8_t op_id; /* 命令ID */
    uint8_t peer_addr[OAL_MAC_ADDR_LEN];
    mac_status_code_enum_uint16 status_code; /* 反馈给上层的建链结果码 */
    uint32_t connect_timeout; /* 连接超时时间 */
    mac_channel_stru assoc_channel;
} hmac_chba_conn_param_stru;

typedef struct {
    uint8_t id; /* connection notfiy id */
    uint8_t peer_mac_addr[WLAN_MAC_ADDR_LEN]; /* 对端MAC地址 */
    uint16_t expire_time; /* 超时时间，ms为单位 */
    mac_kernel_channel_stru channel; /* 建链信道信息 */
} hmac_chba_conn_notify_stru;

/* 8 UNION定义 */
/* 9 OTHERS定义 */
/* 10 函数声明 */
void hmac_chba_rcv_cmd_process(uint8_t cmd_id, uint8_t *cmd_buf, uint16_t buf_size);
void hmac_chba_vap_start(uint8_t *mac_addr, mac_channel_stru *social_channel,
    mac_channel_stru *work_channel);
void hmac_chba_vap_stop(void);
void hmac_chba_conn_info(uint16_t report_type,
    uint8_t *mac_addr, uint8_t *peer_mac_addr, mac_channel_stru *mac_channel);
void hmac_chba_stub_channel_switch_report(hmac_vap_stru *hmac_vap, mac_channel_stru *target_channel);
void hmac_chba_ask_for_sync(uint8_t chan_number, uint8_t *peer_addr, uint8_t *master_addr);

uint32_t hmac_chba_params_sync(mac_vap_stru *mac_vap, chba_params_config_stru *chba_params_sync);
uint32_t hmac_chba_params_sync_after_start(mac_vap_stru *mac_vap, hmac_chba_vap_stru *chba_vap);
uint32_t hmac_config_vap_discovery_channel(mac_vap_stru *mac_vap, hmac_chba_vap_stru *chba_vap_info);
uint32_t hmac_chba_update_work_channel(mac_vap_stru *mac_vap);
uint32_t hmac_chba_update_beacon_pnf(mac_vap_stru *mac_vap, uint8_t *domain_bssid, uint8_t frame_type);
uint32_t hmac_chba_chan_convert_center_freq_to_bw(mac_kernel_channel_stru *kernel_channel,
    mac_channel_stru *mac_channel);
uint32_t hmac_chba_chan_convert_bw_to_center_freq(mac_kernel_channel_stru *kernel_channel,
    mac_channel_stru *mac_channel, mac_vap_stru *mac_vap);
#endif

#endif /* end of this file */

