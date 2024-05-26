

#ifndef __MAC_COMMON_H__
#define __MAC_COMMON_H__

/* 1 其他头文件包含 */
#include "oal_types.h"
#include "wlan_types.h"
#include "hal_common.h"
#include "mac_device_common.h"
#include "mac_user_common.h"
#include "mac_vap_common.h"
#include "mac_cb_tx.h"
#include "mac_cb_rx.h"
// 此处不加extern "C" UT编译不过
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define IS_RW_RING_FULL(read, write)                                    \
    (((read)->st_rw_ptr.bit_rw_ptr == (write)->st_rw_ptr.bit_rw_ptr) && \
        ((read)->st_rw_ptr.bit_wrap_flag != (write)->st_rw_ptr.bit_wrap_flag))

/* 抛往dmac侧消息头的长度 */
#define CUSTOM_MSG_DATA_HDR_LEN (sizeof(custom_cfgid_enum_uint32) + sizeof(uint32_t))

#ifdef _PRE_WLAN_FEATURE_NRCOEX
#define DMAC_WLAN_NRCOEX_INTERFERE_RULE_NUM (4) /* 5gnr共存干扰参数表，目前共4组 */
#endif

#define MAC_USER_IS_HE_USER(_user)          (MAC_USER_HE_HDL_STRU(_user)->en_he_capable)
#define MAC_USER_ARP_PROBE_CLOSE_HTC(_user) ((_user)->bit_arp_probe_close_htc)

#define MAC_USER_TX_DATA_INCLUDE_HTC(_user) ((_user)->bit_tx_data_include_htc)
#define MAC_USER_TX_DATA_INCLUDE_OM(_user)  ((_user)->bit_tx_data_include_om)

#define MAC_USER_NOT_SUPPORT_HTC(_user)  \
    (((_user)->st_cap_info.bit_qos == 0) || (MAC_USER_TX_DATA_INCLUDE_HTC(_user) == 0))
/* 3 枚举定义 */
/*****************************************************************************
  枚举名  : dmac_tx_host_drx_subtype_enum
  协议表格:
  枚举说明: HOST DRX事件子类型定义
*****************************************************************************/
/* WLAN_CRX子类型定义 */
typedef enum {
    DMAC_TX_HOST_DRX = 0,
    HMAC_TX_HOST_DRX = 1,
    HMAC_TX_DMAC_SCH = 2, /* 调度子事件 */
    DMAC_TX_HOST_DTX = 3, /* H2D传输netbuf, device ring tx使用 */

    DMAC_TX_HOST_DRX_BUTT
} dmac_tx_host_drx_subtype_enum;

/*****************************************************************************
  枚举名  : dmac_tx_wlan_dtx_subtype_enum
  协议表格:
  枚举说明: WLAN DTX事件子类型定义
*****************************************************************************/
typedef enum {
    DMAC_TX_WLAN_DTX = 0,

    DMAC_TX_WLAN_DTX_BUTT
} dmac_tx_wlan_dtx_subtype_enum;

/*****************************************************************************
  枚举名  : dmac_wlan_ctx_event_sub_type_enum
  协议表格:
  枚举说明: WLAN CTX事件子类型定义
*****************************************************************************/
typedef enum {
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_MGMT = 0, /* 管理帧处理 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_ADD_USER = 1,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_NOTIFY_ALG_ADD_USER = 2,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_DEL_USER = 3,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_BA_SYNC = 4,  /* 收到wlan的Delba和addba rsp用于到dmac的同步 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_PRIV_REQ = 5, /* 11N自定义的请求的事件类型 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_SCAN_REQ = 6,       /* 扫描请求 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_SCHED_SCAN_REQ = 7, /* PNO调度扫描请求 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_RESET_PSM = 8,      /* 收到认证请求 关联请求，复位用户的节能状态 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_JOIN_SET_REG = 9,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_JOIN_DTIM_TSF_REG = 10,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_CONN_RESULT = 11, /* 关联结果 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_ASOC_WRITE_REG = 12, /* AP侧处理关联时，修改SEQNUM_DUPDET_CTRL寄存器 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_STA_SET_EDCA_REG = 13,       /* STA收到beacon和assoc rsp时，更新EDCA寄存器 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_SWITCH_TO_NEW_CHAN = 14, /* 切换至新信道事件 */
    DMAC_WALN_CTX_EVENT_SUB_TYPR_SELECT_CHAN = 15,        /* 设置信道事件 */
    DMAC_WALN_CTX_EVENT_SUB_TYPR_DISABLE_TX = 16,         /* 禁止硬件发送 */
    DMAC_WALN_CTX_EVENT_SUB_TYPR_ENABLE_TX = 17,          /* 恢复硬件发送 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPR_RESTART_NETWORK = 18,    /* 切换信道后，恢复BSS的运行 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPR_SWITCH_TO_OFF_CHAN = 19,  /* 切换到offchan做off-chan cac检测 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPR_SWITCH_TO_HOME_CHAN = 20, /* 切回home chan */
    DMAC_WLAN_CTX_EVENT_SUB_TYPR_DFS_TEST = 21,
    DMAC_WALN_CTX_EVENT_SUB_TYPR_DFS_CAC_CTRL_TX = 22, /* DFS 1min CAC把vap状态位置为pause或者up,同时禁止或者开启硬件发送 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPR_EDCA_OPT = 23, /* edca优化中业务识别通知事件 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_CALI_HMAC2DMAC = 24,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_DSCR_OPT = 25,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_CALI_MATRIX_HMAC2DMAC = 26,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_APP_IE_H2D = 27,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_IP_FILTER = 28,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_APF_CMD = 29,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_STA_SET_MU_EDCA_REG = 30, /* STA收到beacon和assoc rsp时，更新MU EDCA寄存器 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_STA_SET_SPATIAL_REUSE_REG = 31,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_UPDATE_TWT = 32,                      /* STA收到twt 时，更新寄存器 */
    /* STA收到beacon和assoc rsp时，更新NDP Feedback report寄存器 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_STA_SET_NDP_FEEDBACK_REPORT_REG = 33,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_HIEX_H2D_MSG = 34,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_CUST_HMAC2DMAC = 35,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_CHBA_PARAMS = 36,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_ADD_MULTI_USER = 37,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_CHBA_SELF_CONN_INFO_SYNC = 38,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_CHBA_TOPO_INFO_SYNC = 39,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_BUTT
} dmac_wlan_ctx_event_sub_type_enum;

/* DMAC模块 WLAN_DRX子类型定义 */
typedef enum {
    DMAC_WLAN_DRX_EVENT_SUB_TYPE_RX_DATA,        /* AP模式: DMAC WLAN DRX 流程 */
    DMAC_WLAN_DRX_EVENT_SUB_TYPE_TKIP_MIC_FAILE, /* DMAC tkip mic faile 上报给HMAC */
    DMAC_WLAN_DRX_EVENT_SUB_TYPE_WOW_RX_DATA,    /* WOW DMAC WLAN DRX 流程 */

    DMAC_WLAN_DRX_EVENT_SUB_TYPE_BUTT
} dmac_wlan_drx_event_sub_type_enum;
typedef uint8_t dmac_wlan_drx_event_sub_type_enum_uint8;

/* DMAC模块 WLAN_CRX子类型定义 */
typedef enum {
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_INIT,              /* DMAC 给 HMAC的初始化参数 */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_RX,                /* DMAC WLAN CRX 流程 */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_DELBA,             /* DMAC自身产生的DELBA帧 */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_EVERY_SCAN_RESULT, /* 扫描到一个bss信息，上报结果 */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_SCAN_COMP,         /* DMAC扫描完成上报给HMAC */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_OBSS_SCAN_COMP,    /* DMAC OBSS扫描完成上报给HMAC */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_CHAN_RESULT,       /* 上报一个信道的扫描结果 */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_ACS_RESP,          /* DMAC ACS 回复应用层命令执行结果给HMAC */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_DISASSOC,          /* DMAC上报去关联事件到HMAC, HMAC会删除用户 */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_DEAUTH,            /* DMAC上报去认证事件到HMAC */

    DMAC_WLAN_CRX_EVENT_SUB_TYPR_CH_SWITCH_COMPLETE, /* 信道切换完成事件 */
    DMAC_WLAN_CRX_EVENT_SUB_TYPR_DBAC,               /* DBAC enable/disable事件 */

    DMAC_WLAN_CRX_EVENT_SUB_TYPE_HIEX_D2H_MSG,
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_RX_WOW,
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_EVERY_SCAN_RESULT_RING, /* ring方式接收的扫描结果报文上报 */

    DMAC_WLAN_CRX_EVENT_SUB_TYPE_TX_RING_ADDR,
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_VSP_INFO_ADDR,
    DMAC_WLAN_DRX_EVENT_SUB_TYPE_RX_SNIFFER_INFO,
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_PSD_RPT,           /* DMAC 向HMAC上报PSD信息 */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_CHAN_DET_RPT,       /* DMAC 向HMAC上报信道检测信息 */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_WIFI_DELAY_RPT,
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_BUTT
} dmac_wlan_crx_event_sub_type_enum;
typedef uint8_t dmac_wlan_crx_event_sub_type_enum_uint8;

/* 发向HOST侧的配置事件 */
typedef enum {
    DMAC_TO_HMAC_SYN_UP_REG_VAL = 1,
    DMAC_TO_HMAC_CREATE_BA = 2,
    DMAC_TO_HMAC_DEL_BA = 3,
    DMAC_TO_HMAC_SYN_CFG = 4,

    DMAC_TO_HMAC_ALG_INFO_SYN = 6,
    DMAC_TO_HMAC_VOICE_AGGR = 7,
    DMAC_TO_HMAC_SYN_UP_SAMPLE_DATA = 8,
    DMAC_TO_HMAC_M2S_DATA = 10,
    DMAC_TO_HMAC_BANDWIDTH_INFO_SYN = 11,  /* dmac向hmac同步带宽的信息 */
    DMAC_TO_HMAC_PROTECTION_INFO_SYN = 12, /* dmac向hmac同步保护mib信息 */
    DMAC_TO_HMAC_CH_STATUS_INFO_SYN = 13,  /* dmac向hmac同步可用信道列表 */
    DMAC_TO_HMAC_FTM_CALI = 14,
    /* 事件注册时候需要枚举值列出来，防止出现device侧和host特性宏打开不一致，
       造成出现同步事件未注册问题，后续各子特性人注意宏打开的一致性
    */
    DMAC_TO_HMAC_SYN_BUTT
} dmac_to_hmac_syn_type_enum;

typedef struct {
    uint32_t ftm_cali_time;
    uint8_t vap_id;
} dmac_to_hmac_ftm_ctx_event_stru; /* hd_event */

/* hmac to dmac定制化配置同步消息结构 */
typedef enum {
    CUSTOM_CFGID_NV_ID = 0,
    CUSTOM_CFGID_INI_ID,
    CUSTOM_CFGID_DTS_ID,
    CUSTOM_CFGID_PRIV_INI_ID,
    CUSTOM_CFGID_CAP_ID,

    CUSTOM_CFGID_BUTT,
} custom_cfgid_enum;
typedef unsigned int custom_cfgid_enum_uint32;

typedef enum {
    CUSTOM_CFGID_INI_ENDING_ID = 0,
    CUSTOM_CFGID_INI_FREQ_ID,
    CUSTOM_CFGID_INI_PERF_ID,
    CUSTOM_CFGID_INI_LINKLOSS_ID,
    CUSTOM_CFGID_INI_PM_SWITCH_ID,
    CUSTOM_CFGID_INI_PS_FAST_CHECK_CNT_ID,
    CUSTOM_CFGID_INI_FAST_MODE,

    /* 私有定制 */
    CUSTOM_CFGID_PRIV_INI_RADIO_CAP_ID,
    CUSTOM_CFGID_PRIV_FASTSCAN_SWITCH_ID,
    CUSTOM_CFGID_PRIV_ANT_SWITCH_ID,
    CUSTOM_CFGID_PRIV_LINKLOSS_THRESHOLD_FIXED_ID,
    CUSTOM_CFGID_PRIV_RADAR_ISR_FORBID_ID,
    CUSTOM_CFGID_PRIV_INI_BW_MAX_WITH_ID,
    CUSTOM_CFGID_PRIV_INI_LDPC_CODING_ID,
    CUSTOM_CFGID_PRIV_INI_RX_STBC_ID,
    CUSTOM_CFGID_PRIV_INI_TX_STBC_ID,
    CUSTOM_CFGID_PRIV_INI_SU_BFER_ID,
    CUSTOM_CFGID_PRIV_INI_SU_BFEE_ID,
    CUSTOM_CFGID_PRIV_INI_MU_BFER_ID,
    CUSTOM_CFGID_PRIV_INI_MU_BFEE_ID,
    CUSTOM_CFGID_PRIV_INI_11N_TXBF_ID,
    CUSTOM_CFGID_PRIV_INI_1024_QAM_ID,

    CUSTOM_CFGID_PRIV_INI_CALI_MASK_ID,
    CUSTOM_CFGID_PRIV_CALI_DATA_MASK_ID,
    CUSTOM_CFGID_PRIV_INI_AUTOCALI_MASK_ID,
    CUSTOM_CFGID_PRIV_INI_DOWNLOAD_RATELIMIT_PPS,
    CUSTOM_CFGID_PRIV_INI_TXOPPS_SWITCH_ID,

    CUSTOM_CFGID_PRIV_INI_OVER_TEMPER_PROTECT_THRESHOLD_ID,
    CUSTOM_CFGID_PRIV_INI_TEMP_PRO_ENABLE_ID,
    CUSTOM_CFGID_PRIV_INI_TEMP_PRO_REDUCE_PWR_ENABLE_ID,
    CUSTOM_CFGID_PRIV_INI_TEMP_PRO_SAFE_TH_ID,
    CUSTOM_CFGID_PRIV_INI_TEMP_PRO_OVER_TH_ID,
    CUSTOM_CFGID_PRIV_INI_TEMP_PRO_PA_OFF_TH_ID,

    CUSTOM_CFGID_PRIV_INI_DSSS2OFDM_DBB_PWR_BO_VAL_ID,
    CUSTOM_CFGID_PRIV_INI_EVM_PLL_REG_FIX_ID,
    CUSTOM_CFGID_PRIV_INI_VOE_SWITCH_ID,
    CUSTOM_CFGID_PRIV_COUNTRYCODE_SELFSTUDY_CFG_ID,
    CUSTOM_CFGID_PRIV_M2S_FUNCTION_EXT_MASK_ID,
    CUSTOM_CFGID_PRIV_M2S_FUNCTION_MASK_ID,
    CUSTOM_CFGID_PRIV_MCM_CUSTOM_FUNCTION_MASK_ID,
    CUSTOM_CFGID_PRIV_MCM_FUNCTION_MASK_ID,
    CUSTOM_CFGID_PRIV_INI_11AX_SWITCH_ID,
    CUSTOM_CFGID_PRIV_INI_HTC_SWITCH_ID,
    CUSTOM_CFGID_PRIV_INI_AC_SUSPEND_ID,
    CUSTOM_CFGID_PRIV_INI_MBSSID_SWITCH_ID,
    CUSTOM_CFGID_PRIV_DYN_BYPASS_EXTLNA_ID,
    CUSTOM_CFGID_PRIV_CTRL_FRAME_TX_CHAIN_ID,

    CUSTOM_CFGID_PRIV_CTRL_UPC_FOR_18DBM_C0_ID,
    CUSTOM_CFGID_PRIV_CTRL_UPC_FOR_18DBM_C1_ID,
    CUSTOM_CFGID_PRIV_CTRL_11B_DOUBLE_CHAIN_BO_POW_ID,
    CUSTOM_CFGID_PRIV_INI_LDAC_M2S_TH_ID,
    CUSTOM_CFGID_PRIV_INI_BTCOEX_MCM_TH_ID,
    CUSTOM_CFGID_PRIV_INI_NRCOEX_ID,
    CUSTOM_CFGID_PRIV_INI_HCC_FLOWCTRL_TYPE_ID,
    CUSTOM_CFGID_PRIV_INI_MBO_SWITCH_ID,

    CUSTOM_CFGID_PRIV_INI_DYNAMIC_DBAC_ID,
    CUSTOM_CFGID_PRIV_INI_PHY_CAP_SWITCH_ID,
    CUSTOM_CFGID_PRIV_DC_FLOWCTRL_ID,
    CUSTOM_CFGID_PRIV_INI_HAL_PS_RSSI_ID,
    CUSTOM_CFGID_PRIV_INI_HAL_PS_PPS_ID,
    CUSTOM_CFGID_PRIV_INI_OPTIMIZED_FEATURE_SWITCH_ID,
    CUSTOM_CFGID_PRIV_DDR_SWITCH_ID,
    CUSTOM_CFGID_PRIV_FEM_POW_CTL_ID,
    CUSTOM_CFGID_PRIV_INI_HIEX_CAP_ID,
    CUSTOM_CFGID_PRIV_INI_TX_SWITCH_ID,
    CUSTOM_CFGID_PRIV_INI_FTM_CAP_ID,
    CUSTOM_CFGID_PRIV_INI_MIRACAST_SINK,
    CUSTOM_CFGID_PRIV_INI_MCAST_AMPDU_ENABLE_ID,
    CUSTOM_CFGID_PRIV_INI_PT_MCAST_ENABLE_ID,
    CUSTOM_CFGID_PRIV_CLOSE_FILTER_SWITCH_ID,

    CUSTOM_CFGID_INI_BUTT,
} custom_cfgid_h2d_ini_enum;
typedef unsigned int custom_cfgid_h2d_ini_enum_uint32;

enum custom_optimize_feature {
    CUSTOM_OPTIMIZE_FEATURE_RA_FLT = 0,
    CUSTOM_OPTIMIZE_FEATURE_NAN = 1,
    CUSTOM_OPTIMIZE_FEATURE_CERTIFY_MODE = 2,
    CUSTOM_OPTIMIZE_TXOP_COT = 3,
    CUSTOM_OPTIMIZE_11AX20M_NON_LDPC = 4, /* 认证时ap 20M 不支持LDPC可以关联在ax */
    CUSTOM_OPTIMIZE_CHBA = 5, /* CHBA组网开关 */
    CUSTOM_OPTIMIZE_CE_IDLE = 6, /* CE认证用例场景关闭pk mode，不调整竞争参数 */
    CUSTOM_OPTIMIZE_CSA_EXT = 7, /* CSA扩展 跨频段功能 开关 */
    CUSTOM_OPTIMIZE_FEATURE_BUTT,
};

enum custom_close_filter_switch {
    CUSTOM_APF_CLOSE_SWITCH,
    CUSTOM_ARP_CLOSE_MULITCAST_FILTER,
    CUSTOM_CLOSE_FILTER_SWITCH_BUTT
};

typedef struct {
    custom_cfgid_enum_uint32 en_syn_id; /* 同步配置ID */
    uint32_t len;                  /* DATA payload长度 */
    uint8_t auc_msg_body[NUM_4_BYTES];          /* DATA payload */
} hmac_to_dmac_cfg_custom_data_stru;

/* MISC杂散事件 */
typedef enum {
    DMAC_MISC_SUB_TYPE_RADAR_DETECT,
    DMAC_MISC_SUB_TYPE_DISASOC,
    DMAC_MISC_SUB_TYPE_CALI_TO_HMAC,
    DMAC_MISC_SUB_TYPE_HMAC_TO_CALI,

    DMAC_MISC_SUB_TYPE_ROAM_TRIGGER,
    DMAC_TO_HMAC_DPD,

#ifdef _PRE_WLAN_FEATURE_APF
    DMAC_MISC_SUB_TYPE_APF_REPORT,
#endif
    DMAC_MISC_SUB_TYPE_TX_SWITCH_STATE,
    DMAC_MISC_PSM_GET_HOST_RING_STATE,

    DMAC_MISC_SUB_TYPE_BUTT
} dmac_misc_sub_type_enum;

typedef enum {
    DMAC_DISASOC_MISC_LINKLOSS = 0,
    DMAC_DISASOC_MISC_KEEPALIVE = 1,
    DMAC_DISASOC_MISC_CHANNEL_MISMATCH = 2,
    DMAC_DISASOC_MISC_LOW_RSSI = 3,
    DMAC_DISASOC_MISC_GET_CHANNEL_IDX_FAIL = 5,
    DMAC_DISASOC_MISC_BUTT
} dmac_disasoc_misc_reason_enum;
typedef uint16_t dmac_disasoc_misc_reason_enum_uint16;

/* HMAC to DMAC同步类型 */
typedef enum {
    HMAC_TO_DMAC_SYN_INIT,
    HMAC_TO_DMAC_SYN_CREATE_CFG_VAP,
    HMAC_TO_DMAC_SYN_CFG,
    HMAC_TO_DMAC_SYN_ALG,
    HMAC_TO_DMAC_SYN_REG,
#if defined(_PRE_WLAN_FEATURE_DATA_SAMPLE) || defined(_PRE_WLAN_FEATURE_PSD_ANALYSIS)
    HMAC_TO_DMAC_SYN_SAMPLE,
#endif

    HMAC_TO_DMAC_SYN_BUTT
} hmac_to_dmac_syn_type_enum;
typedef uint8_t hmac_to_dmac_syn_type_enum_uint8;

/* BA会话的状态枚举 */
typedef enum {
    DMAC_BA_INIT = 0,   /* BA会话未建立 */
    DMAC_BA_INPROGRESS, /* BA会话建立过程中 */
    DMAC_BA_COMPLETE,   /* BA会话建立完成 */
    DMAC_BA_HALTED,     /* BA会话节能暂停 */
    DMAC_BA_FAILED,     /* BA会话建立失败 */

    DMAC_BA_BUTT
} dmac_ba_conn_status_enum;
typedef uint8_t dmac_ba_conn_status_enum_uint8;

/* 配置命令带宽能力枚举 */
typedef enum {
    WLAN_BANDWITH_CAP_20M,
    WLAN_BANDWITH_CAP_40M,
    WLAN_BANDWITH_CAP_40M_DUP,
    WLAN_BANDWITH_CAP_80M,
    WLAN_BANDWITH_CAP_80M_DUP,
    WLAN_BANDWITH_CAP_160M,
    WLAN_BANDWITH_CAP_160M_DUP,
    WLAN_BANDWITH_CAP_80PLUS80,
#ifdef  _PRE_WLAN_FEATURE_11AX_ER_SU_DCM
    WLAN_BANDWITH_CAP_ER_242_TONE,
    WLAN_BANDWITH_CAP_ER_106_TONE,
#endif
    WLAN_BANDWITH_CAP_BUTT
} wlan_bandwith_cap_enum;
typedef uint8_t wlan_bandwith_cap_enum_uint8;

typedef enum {
    DMAC_CHANNEL1    = 1,
    DMAC_CHANNEL11   = 11,
    DMAC_CHANNEL12   = 12,
    DMAC_CHANNEL36   = 36,
    DMAC_CHANNEL48   = 48,
    DMAC_CHANNEL149  = 149,
    DMAC_CHANNEL165  = 165,
    DMAC_PC_PASSIVE_CHANNEL_BUTT
} dmac_pc_passive_channel_enum;

typedef struct {
    uint16_t us_baw_start;                          /* 第一个未确认的MPDU的序列号 */
    uint16_t us_last_seq_num;                       /* 最后一个发送的MPDU的序列号 */
    uint16_t us_baw_size;                           /* Block_Ack会话的buffer size大小 */
    uint16_t us_baw_head;                           /* bitmap中记录的第一个未确认的包的位置 */
    uint16_t us_baw_tail;                           /* bitmap中下一个未使用的位置 */
    oal_bool_enum_uint8 en_is_ba;                     /* Session Valid Flag */
    dmac_ba_conn_status_enum_uint8 en_ba_conn_status; /* BA会话的状态 */
    mac_back_variant_enum_uint8 en_back_var;          /* BA会话的变体 */
    uint8_t uc_dialog_token;                        /* ADDBA交互帧的dialog token */
    uint8_t uc_ba_policy;                           /* Immediate=1 Delayed=0 */
    oal_bool_enum_uint8 en_amsdu_supp;                /* BLOCK ACK支持AMSDU的标识 */
    uint8_t *puc_dst_addr;                          /* BA会话接收端地址 */
    uint16_t us_ba_timeout;                         /* BA会话交互超时时间 */
    uint8_t uc_ampdu_max_num;                       /* BA会话下，能够聚合的最大的mpdu的个数 */
    uint8_t uc_tx_ba_lut;                           /* BA会话LUT session index */
    uint16_t us_mac_user_idx;
#ifdef _PRE_WLAN_FEATURE_DFR
    uint16_t us_pre_baw_start;    /* 记录前一次判断ba窗是否卡死时的ssn */
    uint16_t us_pre_last_seq_num; /* 记录前一次判断ba窗是否卡死时的lsn */
    uint16_t us_ba_jamed_cnt;     /* BA窗卡死统计次数 */
#else
    uint8_t auc_resv[NUM_2_BYTES];
#endif
    uint32_t aul_tx_buf_bitmap[DMAC_TX_BUF_BITMAP_WORDS];
} dmac_ba_tx_stru;

typedef enum {
    OAL_QUERY_STATION_INFO_EVENT = 1,
    OAL_ATCMDSRV_DBB_NUM_INFO_EVENT = 2,
    OAL_ATCMDSRV_FEM_PA_INFO_EVENT = 3,
    OAL_ATCMDSRV_GET_RX_PKCG = 4,
    OAL_ATCMDSRV_LTE_GPIO_CHECK = 5,
    OAL_ATCMDSRV_GET_ANT = 6,
    OAL_QUERY_EVNET_BUTT
} oal_query_event_id_enum;

typedef union {
    uint16_t rw_ptr; /* write index */
    struct {
        uint16_t bit_rw_ptr    : 15,
                 bit_wrap_flag : 1;
    } st_rw_ptr;
} un_rw_ptr;

typedef enum {
    TID_CMDID_CREATE                 = 0,
    TID_CMDID_DEL,
    TID_CMDID_ENQUE,
    TID_CMDID_DEQUE,

    TID_CMDID_BUTT,
} tid_cmd_enum;
typedef uint8_t tid_cmd_enum_uint8;

typedef enum {
    RING_PTR_EQUAL = 0,
    RING_PTR_SMALLER,
    RING_PTR_GREATER,
} ring_ptr_compare_enum;
typedef uint8_t ring_ptr_compare_enum_uint8;

typedef enum {
    HOST_RING_TX_MODE = 0,
    DEVICE_RING_TX_MODE = 1,
    DEVICE_TX_MODE = 2,
    H2D_WAIT_SWITCHING_MODE = 3,
    H2D_SWITCHING_MODE = 4,
    D2H_WAIT_SWITCHING_MODE = 5,
    D2H_SWITCHING_MODE = 6,
    TX_MODE_DEBUG_DUMP = 7,

    TX_MODE_BUTT,
} ring_tx_mode_enum;

typedef enum {
    DATA_TYPE_ETH = 0,    /* Eth II以太网报文类型, 无vlan */
    DATA_TYPE_1_VLAN_ETH = 1,    /* Eth II以太网报文类型, 有1个vlan */
    DATA_TYPE_2_VLAN_ETH = 2,    /* Eth II以太网报文类型, 有2个vlan */
    DATA_TYPE_80211 = 3,         /* 802.11报文类型 */
    DATA_TYPE_0_VLAN_8023 = 4,   /* 802.3以太网报文类型, 无vlan */
    DATA_TYPE_1_VLAN_8023 = 5,   /* 802.3以太网报文类型, 有1个vlan */
    DATA_TYPE_2_VLAN_8023 = 6,   /* 802.3以太网报文类型, 有2个vlan */
    DATA_TYPE_MESH = 7,          /* Mesh报文类型, 无extend addresss */
    DATA_TYPE_MESH_EXT_ADDR = 8, /* Mesh报文类型, 有extend addresss */

    DATA_TYPE_BUTT
} data_type_enum;

#define DATA_TYPE_802_3_SNAP (BIT(2))

#define TX_RING_MSDU_INFO_LEN  12

typedef struct {
    uint32_t msdu_addr_lsb;
    uint32_t msdu_addr_msb;
    uint32_t msdu_len      : 12;
    uint32_t data_type     : 4;
    uint32_t frag_num      : 4;
    uint32_t to_ds         : 1;
    uint32_t from_ds       : 1;
    uint32_t more_frag     : 1;
    uint32_t reserved      : 1;
    uint32_t aggr_msdu_num : 4;
    uint32_t first_msdu    : 1;
    uint32_t csum_type     : 3;
} msdu_info_stru;

/* Host Device公用部分 */
typedef struct {
    uint8_t size;
    uint8_t max_amsdu_num;
    uint8_t reserved[2];
    uint32_t base_addr_lsb;
    uint32_t base_addr_msb;
    uint16_t read_index;
    uint16_t write_index;
} msdu_info_ring_stru; /* hal_common.h? */

typedef struct {
    uint16_t user_id;
    uint8_t tid_no;
    uint8_t cmd;
    uint8_t ring_index; /* 受限于内存, device ring和tid无法做到1:1分配,
                         * host侧h2d切换时, 需要在bitmap中找到下一个可分配的ring index,
                         * device根据index从ring内存池中获取可用的ring,
                         * d2h切换时同理, host侧需要将某个已占用的ring index设置为可用
                         */
    uint8_t hal_dev_id;
    uint8_t tid_num; /* 需要切换的tid数量 */
    uint8_t resv;
} tx_state_switch_stru;

typedef struct {
    uint8_t device_loop_sche_enabled;
} tx_sche_switch_stru;

#ifdef _PRE_WLAN_FEATURE_NRCOEX
typedef struct {
    uint32_t freq;
    uint32_t gap0_40m_20m;
    uint32_t gap0_160m_80m;
    uint32_t gap1_40m_20m;
    uint32_t gap1_160m_80m;
    uint32_t gap2_140m_20m;
    uint32_t gap2_160m_80m;
    uint32_t smallgap0_act;
    uint32_t gap01_act;
    uint32_t gap12_act;
    int32_t l_rxslot_rssi;
} nrcoex_rule_stru;

typedef struct {
    uint32_t us_freq_low_limit : 16,
               us_freq_high_limit : 16;
    uint32_t us_20m_w2m_gap0 : 16,
               us_40m_w2m_gap0 : 16;
    uint32_t us_80m_w2m_gap0 : 16,
               us_160m_w2m_gap0 : 16;
    uint32_t us_20m_w2m_gap1 : 16,
               us_40m_w2m_gap1 : 16;
    uint32_t us_80m_w2m_gap1 : 16,
               us_160m_w2m_gap1 : 16;
    uint32_t us_20m_w2m_gap2 : 16,
               us_40m_w2m_gap2 : 16;
    uint32_t us_80m_w2m_gap2 : 16,
               us_160m_w2m_gap2 : 16;
    uint32_t uc_20m_smallgap0_act : 8,
               uc_40m_smallgap0_act : 8,
               uc_80m_smallgap0_act : 8,
               uc_160m_smallgap0_act : 8;
    uint32_t uc_20m_gap01_act : 8,
               uc_40m_gap01_act : 8,
               uc_80m_gap01_act : 8,
               uc_160m_gap01_act : 8;
    uint32_t uc_20m_gap12_act : 8,
               uc_40m_gap12_act : 8,
               uc_80m_gap12_act : 8,
               uc_160m_gap12_act : 8;
    uint32_t uc_20m_rxslot_rssi : 8,
               uc_40m_rxslot_rssi : 8,
               uc_80m_rxslot_rssi : 8,
               uc_160m_rxslot_rssi : 8;
} nrcoex_rule_detail_stru;

typedef union {
    nrcoex_rule_stru st_nrcoex_rule_data;
    nrcoex_rule_detail_stru st_nrcoex_rule_detail_data;
} nrcoex_rule_data_union;

typedef struct {
    uint8_t uc_nrcoex_enable;
    uint8_t rsv[NUM_3_BYTES];  /* 保留字段:3字节 */
    nrcoex_rule_data_union un_nrcoex_rule_data[DMAC_WLAN_NRCOEX_INTERFERE_RULE_NUM];
} nrcoex_cfg_info_stru;
#endif

typedef struct {
    uint8_t cfg_type;
    oal_bool_enum_uint8 need_w4_dev_return;
    uint8_t resv[NUM_2_BYTES]; /* 4 字节对齐 */
    uint32_t dev_ret;
} mac_cfg_cali_hdr_stru;

#ifdef _PRE_WLAN_FEATURE_PHY_EVENT_INFO
typedef struct {
    uint8_t               event_rpt_en;
    uint8_t               rsv[NUM_3_BYTES]; /* 4 字节对齐 */
    uint32_t              wp_mem_num;       /* 事件统计个数 */
    uint32_t              wp_event0_type_sel;  /* event0事件类型选择参数 */
    uint32_t              wp_event1_type_sel;  /* event1事件类型选择参数 */
} mac_cfg_phy_event_rpt_stru;
#endif

enum vsp_tx_sync_stage_enum {
    VSP_TX_SYNC_LOCK_ACQUIRE = 0,
    VSP_TX_SYNC_CLEAR_FAIL,
    VSP_TX_SYNC_CLEAR_DONE,
    VSP_TX_SYNC_LOCK_RELEASE,
    VSP_TX_SYNC_INIT
};

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of mac_common.h */
