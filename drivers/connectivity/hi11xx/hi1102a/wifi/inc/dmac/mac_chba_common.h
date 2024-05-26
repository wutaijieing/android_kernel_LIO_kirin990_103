

#ifndef __MAC_CHBA_COMMON_H__
#define __MAC_CHBA_COMMON_H__

/* 1 其他头文件包含 */
#include "oal_types.h"
#include "wlan_types.h"
#include "wlan_spec.h"
#include "mac_device.h"
#include "mac_vap.h"
#include "mac_user.h"

/* 此处不加extern "C" UT编译不过 */
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_CHBA_COMMON_H

#ifdef _PRE_WLAN_CHBA_MGMT
/* 规格相关宏 */
#define MAC_CHBA_MAX_DEVICE_NUM 32
#define MAC_CHBA_MAX_BITMAP_WORD_NUM 4
#define MAC_CHBA_MAX_ISLAND_NUM 32
#define MAC_CHBA_MAX_ISLAND_DEVICE_NUM 32
#define MAC_CHBA_MAX_LINK_NUM 4
#define MAC_CHBA_MAX_WHITELISI_NUM 16
#define MAC_CHBA_REQUEST_LIST_NUM 8

#define MAC_CHBA_MAX_RPT_BANDWIDTH_CNT 4 /* 表示主20M、主40M、主80M、主160M */
#define MAC_CHBA_MAX_COEX_VAP_NUM 6
#define MAC_CHBA_MAX_VAP_NAME_LEN 16

/* 管理帧相关宏 */
#define MAC_CHBA_MGMT_MAX_PAYLOAD_BYTES 512
#define MAC_CHBA_MGMT_MID_PAYLOAD_BYTES 256
#define MAC_CHBA_MGMT_SHORT_PAYLOAD_BYTES 128
#define MAC_CHBA_PNF_MAX_PAYLOAD_BYTES MAC_CHBA_MGMT_MID_PAYLOAD_BYTES
#define MAC_CHBA_VENDOR_EID 221

#define MAC_CHBA_VENDOR_IE 0x00E0FC
#define MAC_OUI_TYPE_CHBA 0x88 /* 待定 */
#define MAC_CHBA_BEACON_ELEMENT_OFFSET 12 /* Beacon固定字段偏移 */
#define MAC_CHBA_VENDOR_IE_HDR_OFFSET 6 /* IE头偏移 */
#define MAC_CHBA_ACTION_OUI_TYPE_LEN 4
#define MAC_CHBA_ACTION_TYPE_LEN 5
#define MAC_CHBA_ACTION_HDR_LEN 6
#define MAC_CHBA_ATTR_ID_LEN 1
#define MAC_CHBA_ATTR_HDR_LEN 2
#define MAC_CHBA_MASTER_ELECTION_ATTR_LEN 20 /* 包含属性头 */
#define MAC_CHBA_CHAN_SWITCH_ATTR_LEN 7 /* 包含属性头 */
#define MAC_CHBA_BITMAP_UPDATE_REQ_RSP_LEN 8
#define MAC_CHBA_LENGTH_POS 1
#define MAC_CHBA_LOSS_CNT_POS 3
#define MAC_CHBA_LINK_INFO_CHAN_POS 2
#define MAC_CHBA_LINK_INFO_BW_POS 3
#define MAC_CHBA_RANKING_LEVEL_POS 8
#define MAC_CHBA_MASTER_ATTR_LINK_CNT_POS 10
#define MAC_CHBA_MASTER_RANKING_LEVEL_POS 11
#define MAC_CHBA_MASTER_ADDR_POS 14
#define MAC_CHBA_LINK_INFO_RSSI_POS 4
#define MAC_CHBA_COEX_CHAN_CNT_OFFSET 2
#define MAC_CHBA_COEX_CHAN_LIST_OFFSET 3
#define MAC_CHBA_LINK_INFO_LINK_CNT_POS 5 /* LINK INFO字段中link cnt偏移 */
#define MAC_CHBA_POWER_SAVE_BITMAP_FLAG_POS 2 /* power save bitmap字段中req/rsp flag的偏移 */
#define MAC_CHBA_POWER_SAVE_BITMAP_PS_BITMAP 3  /* power save bitmap字段中低功耗bitmap的偏移 */
#define MAC_CHBA_POWER_SAVE_BITMAP_IE_LEN 5 /* chba power_save_bitmap ie信息字段长度 */
#define MAC_CHBA_DEVICE_CAPABILITY_IE_LEN 10 /* chba device capability ie信息字段长度 */
#define MAC_CHBA_DEVICE_CAPABILITY_RES_LEN 8 /* chba device capability ie预留信息字段长度 */
#define MAC_CHBA_SCAN_NOTIFY_BITMAP_LEN 4 /* 扫描通知属性bitmap字段大小 */
#define MAC_CHBA_SCAN_NOTIFY_OFF_SLOT_CNT_LEN 1 /* 扫描通知属性持续周期字段大小 */
#define MAC_CHBA_ISLAND_OWNER_CNT_POS 2
#define MAC_CHBA_MASTER_ELECTION_SYNC_POS 2
#define MAC_CHBA_SYNC_REQ_TYPE_POS 1
#define MAC_CHBA_DEFALUT_PERIODS_MS 512 /* CHBA默认调度周期时长(ms) */

/* 其他 */
#define MAC_CHBA_COEX_HARDWARE_CAPABILITY 0x24
#define MAC_CHBA_CHANNEL_NUM_5G 73 /* 5G上全部的信道 */
#define CHBA_PS_ALL_AWAKE_BITMAP 0xFFFFFFFF /* 低功耗bitmap:全醒 */
#define CHBA_PS_HALF_AWAKE_BITMAP 0x33333333 /* 低功耗bitmap:半醒 */
#define CHBA_PS_KEEP_ALIVE_BITMAP 0x00010001 /* 低功耗bitmap:保活 */

#define CHBA_INVALID_MAX_CHANNEL_SWITCH_TIME 0 /* 不可用的max_channel_switch_time配置 */
#define WLAN_CHBA_USERS_DEEPSLEEP_AGING_TIME (360 * 1000) /* CHBA 长休眠状态老化时间 360s */
#define WLAN_CHBA_USERS_NORMAL_AGING_TIME (5 * 1000) /* CHBA 非长休眠状态老化时间 5s */
#define WLAN_CHBA_USERS_NORMAL_SEND_NULL_TIME (1 * 1000) /* CHBA 非长休眠状态发送null帧保活时间 1s */
#define WLAN_CHBA_KEEPALIVE_TRIGGER_TIME (1 * 1000) /* keepalive定时器触发周期 */
#define CHBA_INIT_SOCIAL_CHANNEL 36 /* chba模块初始化social channel在36信道 */
#define CHBA_DFS_CHANNEL_SUPPORT_STATE 0 /* chba模块初始化不支持dfs信道 */
/* Hmac通知到CHBA模块的消息类型枚举 */
// cfgvendor命令仍会使用，暂不删除
typedef enum {
    /* 以下消息从HMAC --> CHBA模块 */
    MAC_CHBA_MSG_START = 5,
    MAC_CHBA_VAP_START_RPT = MAC_CHBA_MSG_START,          /* vap使能信息上报 */
    MAC_CHBA_VAP_STOP_RPT = 6,                         /* vap去使能信息上报 */
    MAC_CHBA_CONN_ADD_RPT = 7,                         /* 建链信息上报 */
    MAC_CHBA_DELAY_OVER_TH_RPT = 16,                   /* 时延超阈值上报 */
    MAC_CHBA_STAT_INFO_RPT = 17,                       /* 统计信息上报 */
    MAC_CHBA_COEX_VAP_SUPPORTED_CHAN_LIST_RPT = 19,    /* 共存的vap可支持信道列表变更 */
    MAC_CHBA_SUPPORTED_CHAN_LIST_RPT = 20,             /* 本设备支持的信道列表 */
    MAC_CHBA_COEX_INFO_RPT = 21,                       /* 本设备的共存信息更新 */
    MAC_CHBA_STUB_CHAN_SWITCH_RPT = 22,                /* 打桩信道切换上报 */

    /* 以下消息从Service --> HMAC --> CHBA模块 */
    MAC_CHBA_MODULE_INIT = 24,                         /* 模块初始化 */
    MAC_CHBA_BATTERY_LEVEL_UPDATE = 25,                /* 电量更新 */
    MAC_CHBA_CHAN_ADJUST = 30,                         /* CHBA信道调整 */
    MAC_CHBA_CHAN_ADJUST_NOTIFY = 31,                  /* CHBA信道调整通知 */
    MAC_CHBA_GET_COEX_INFO = 32,                       /* 获取共存信息 */

    MAC_CHBA_MSG_END,
} mac_chba_msg_notify_type_enum;

/* CHBA模块配置到Hmac的命令类型 */
// cfgvendor命令仍会使用，暂不删除
typedef enum {
    MAC_CHBA_SET_INIT_PARA = 0,                        /* CHBA初始化参数下发 */
    MAC_CHBA_SET_ISLAND_INFO,                          /* CHBA本设备所属岛信息下发 */
    MAC_CHBA_SET_SCAN_PARA,                            /* CHBA扫描参数下发 */
    MAC_CHBA_SET_COEX_VAP_SUPPORTED_CHAN_LIST,         /* 共存vap的可用信道列表下发 */
    MAC_CHBA_SET_MGMT_FRAME,                           /* 管理帧下发 */
    MAC_CHBA_SET_SYNC_INFO,                            /* 下发同步状态 */
    MAC_CHBA_SET_WORK_CHAN,                            /* 工作信道切换 */
    MAC_CHBA_SET_VAP_BITMAP,                           /* 下发VAP低功耗bitmap */
    MAC_CHBA_SET_USER_BITMAP,                          /* 下发USER低功耗bitmap */
    MAC_CHBA_SET_LONG_SLEEP,                           /* 下发长休眠信息 */
    MAC_CHBA_SET_SYNC_REQUEST_PARAM,                   /* 下发同步请求参数 */
    MAC_CHBA_SET_DOMAIN_REQUEST_PARAM,                 /* 下发域合并参数 */
    MAC_CHBA_SET_STAT_INFO_QUERY,                      /* 下发统计信息查询 */
} mac_chba_cmd_msg_type_enum;

/* 管理帧Category字段枚举 */
typedef enum {
    MAC_CHBA_PNF = 0,
    MAC_CHBA_CHANNEL_SWITCH_NOTIFICATION = 5,
    MAC_CHBA_BITMAP_NOTIFICATION = 6,
    MAC_CHBA_SYNC_REQUEST = 7,
    MAC_CHBA_DOMAIN_MERGE = 8,

    MAC_CHBA_SYNC_BEACON,
    MAC_CHBA_ACTION_CATE_BUTT,
} mac_chba_action_category_enum;

/* Attribute属性类型枚举 */
typedef enum {
    MAC_CHBA_ATTR_MASTER_ELECTION = 0x00,
    MAC_CHBA_ATTR_LINK_INFO = 0x01,
    MAC_CHBA_ATTR_DEVICE_CAPABILITY = 0x02,
    MAC_CHBA_ATTR_CHAN_SWITCH = 0x05,
    MAC_CHBA_ATTR_POWER_SAVE_BITMAP = 0x06,
    MAC_CHBA_ATTR_COEX_STATUS = 0x07,
    MAC_CHBA_ATTR_SCAN_NOTIFY = 0x08,
    MAC_CHBA_ATTR_ISLAND_OWNER = 0x09,
    MAC_CHBA_ATTR_CHANNEL_SEQ = 0x0A,
    MAC_CHBA_ATTR_IE_CONTAINER = 0x20,

    MAC_CHBA_ATTR_BUTT,
} chba_attribute_type_enum;

/* 低功耗相关action帧类型 */
typedef enum {
    MAC_CHBA_BITMAP_UPDATE_REQ = 0x00, /* 请求更新bitmap */
    MAC_CHBA_BITMAP_UPDATE_RSP_AGREE = 0x01, /* 同意更新bitmap */
    MAC_CHBA_BITMAP_UPDATE_RSP_REJECT = 0x02, /* 拒绝更新bitmap */

    MAC_CHBA_BITMAP_TYPE_BUTT,
} mac_chba_bitmap_frame_subtype_enum;

/* chba vap mode枚举 */
typedef enum {
    CHBA_LEGACY_MODE = 0, /* 非chba模式 */
    CHBA_MODE = 1, /* chba?? */

    CHBA_VAP_MODE_BUTT,
} chba_vap_mode_enum;

/* chba vap的状态枚举 */
typedef enum {
    CHBA_INIT = 1,
    CHBA_NON_SYNC = 2,
    CHBA_DISCOVERY = 3,
    CHBA_WORK = 4,

    CHBA_STATE_BUTT,
} chba_vap_state_enum;


/* chba角色枚举 */
typedef enum {
    CHBA_MASTER = 1, /* Master所在岛的岛主就是Master */
    CHBA_ISLAND_OWNER = 2,
    CHBA_OTHER_DEVICE = 3,

    CHBA_ROLE_BUTT,
} chba_role_enum;

/* d2h chba同步事件 */
typedef enum {
    CHBA_UPDATE_TOPO_INFO = 0, /* 更新整个topo信息 */
    CHBA_UPDATE_TOPO_BITMAP_INFO = 1, /* 更新topo中的bitmap信息 */
    CHBA_RP_CHANGE_START_MASTER_ELECTION = 2, /* max RP变化触发主设备选举 */
    CHBA_NON_SYNC_START_MASTER_ELECTION = 3, /* 触发host侧主设备选举 */
    CHBA_NON_SYNC_START_SYNC_REQ = 4, /* 触发host侧主动向master申请sync beacon */
    CHBA_DOMAIN_SEQARATE_START_MASTER_ELECTION = 5, /* 域拆分触发主设备选举 */
    CHBA_SYNC_BUTT
} chba_sync_status_enum;

/* 主设备选举属性中宣称的sync state状态枚举 */
typedef enum {
    MAC_CHBA_DECLARE_NON_SYNC = 0,
    MAC_CHBA_DECLARE_MASTER = 1,
    MAC_CHBA_DECLARE_SLAVE = 2,
} mac_chba_declare_state_enum;

/* 同步类型枚举 */
typedef enum {
    MAC_CHBA_SYNC_WITH_REQUEST = 1,
    MAC_CHBA_SYNC_ON_SOCIAL_CHANNEL = 2,
} mac_chba_sync_type_enum;

/* 同步参数的bitmap表 */
typedef enum {
    CHBA_SCHD = BIT0,
    CHBA_ISLAND_BEACON = BIT1,
    CHBA_DISCOVER_BITMAP = BIT2,
    CHBA_WORK_CHAN = BIT3,
    CHBA_SOCIAL_CHAN = BIT4,
    CHBA_STATE = BIT5,
    CHBA_ROLE = BIT6,
    CHBA_BSSID = BIT7,
    CHBA_BEACON_BUF = BIT8,
    CHBA_BEACON_LEN = BIT9,
    CHBA_PNF_BUF = BIT10,
    CHBA_PNF_LEN = BIT11,

    CHBA_PS_BITMAP = BIT12,
    CHBA_MASTER_INFO = BIT13,
} params_update_bitmap_enum;

/* bitmap类型开关枚举 */
typedef enum {
    CHBA_BITMAP_AUTO = 0,
    CHBA_BITMAP_FIX = 1,
    CHBA_BITMAP_CLOSE_SCAN = 2,
    CHBA_BITMAP_SWITCH_BUTT
} chba_bitmap_switch_enum;

typedef enum {
    CHBA_DEBUG_NO_LOG = 0, /* 不打印维测信息 */
    CHBA_DEBUG_STAT_INFO_LOG = 1, /* 打印CHBA相关统计信息 */
    CHBA_DEBUG_ALL_LOG = 2, /* 打印全部CHBA信息 */

    CHBA_DEBUG_BUTT
} chba_debug_log_type_enum; /* CHBA维测日志打印类型枚举 */

/* 设备信息更新类型枚举 */
typedef enum {
    CHBA_DEVICE_ID_ADD = 0,
    CHBA_DEVICE_ID_DEL = 1,
    CHBA_DEVICE_ID_CLEAR = 2,
} chba_device_id_update_enum;

typedef enum {
    CHBA_CHANNEL_CFG_BITMAP_DISCOVER = BIT0, /* discover信道配置 */
    CHBA_CHANNEL_CFG_BITMAP_WORK = BIT1, /* work信道配置 */

    CHBA_CHANNEL_CFG_BITMAP_BUTT
} chba_channel_cfg_bitmap_enum; /* chba信道配置bitmap枚举 */
// 上层下发配置共存信道信息命令类型
typedef enum {
    HMAC_CHBA_SELF_DEV_COEX_CFG_TYPE,
    HMAC_CHBA_ISLAND_COEX_CFG_TYPE,
    HMAC_CHBA_COEX_CFG_TYPE_BUTT,
} hmac_chba_coex_cmd_type_enum;

typedef enum {
    CHBA_BITMAP_ALL_AWAKE_LEVEL = 0,
    CHBA_BITMAP_HALF_AWAKE_LEVEL = 1,
    CHBA_BITMAP_KEEP_ALIVE_LEVEL = 2,

    CHBA_BITMAP_LEVEL_BUTT
} chba_bitmap_level_enum; /* CHBA bitmap档位枚举 */
/* Ranking Priority信息结构体 */
typedef struct {
    union {
        uint8_t value;
        struct {
            uint8_t hardware_capability : 2; /* 硬件规格，比如单双芯片 */
            uint8_t remain_power : 2; /* 剩余电量 */
            uint8_t device_type : 4; /* 设备类型: 例如PC、Pad等 */
        } dl;
    } device_level;
    uint8_t chba_version; /* 协议版本 */
    uint8_t link_cnt; /* 链路总数 */
} ranking_priority;

/* d2h chba同步结构体 */
typedef struct {
    uint8_t mac_addr[OAL_MAC_ADDR_LEN];
    uint8_t resv[2];
    uint32_t device_bitmap;
} chba_d2h_device_bitmap_info_stru;

/* CHBA建链后请求同步所需结构体 */
typedef struct {
    uint8_t need_sync_flag; /* 是否需要进行同步 */
    uint8_t peer_sync_state; /* 对端的同步状态 */
    uint8_t peer_master_addr[WLAN_MAC_ADDR_LEN]; /* 对端Master的mac地址 */
    uint8_t peer_work_channel; /* 对端的工作信道 */
    ranking_priority peer_rp; /* 对端的RP值 */
    ranking_priority peer_master_rp; /* 对端Master的RP值 */
} chba_req_sync_after_assoc_stru;

/* Master信息结构体 */
typedef struct {
    uint8_t master_work_channel; /* 保存master工作信道的主20M信道号 */
    uint8_t master_addr[WLAN_MAC_ADDR_LEN]; /* master的mac地址信息 */
    ranking_priority master_rp; /* 保存Master的RP值信息 */
} master_info;

/* 配置到DMAC的参数 */
typedef struct {
    uint32_t info_bitmap; /* 每个bit代表一个信息，如果该bit置1，表示对应信息有更新，否则无需更新 */

    /* 以下参数add vap时下发，基本上不变 */
    uint8_t schedule_period; /* 调度周期包含的时隙个数 */
    uint8_t island_beacon_slot; /* 岛主广播beacon帧的时隙 */
    uint8_t resv1[2];
    uint32_t discover_bitmap; /* 设置discover bitmap */

    /* 以下参数在角色或状态变更时下发 */
    uint8_t chba_state; /* vap同步状态(chba_vap_state_enum) */
    uint8_t chba_role; /* 是否是岛主(chba_role_enum) */
    uint8_t domain_bssid[WLAN_MAC_ADDR_LEN]; /* 域BSSID */

    /* 以下参数每个调度周期下发 */
    uint8_t sync_beacon[MAC_CHBA_MGMT_MAX_PAYLOAD_BYTES];
    uint16_t sync_beacon_len;
    uint8_t resv2[2];
    uint8_t pnf[MAC_CHBA_MGMT_MID_PAYLOAD_BYTES];
    uint16_t pnf_len;
    uint8_t resv3[2];
    master_info master_info; /* master信息 */
    uint8_t resv4;
} chba_params_config_stru;

typedef struct {
    uint8_t chan_cfg_type_bitmap; /* 信道配置类型(bit0置1表示配置discover slot信道, bit1置1表示配置work slot信道) */
    uint8_t chan_switch_slot_idx; /* 指定在某个slot再更新bitmap 扩展项, 暂未开发 */
    uint8_t resv[2];
    mac_channel_stru channel; /* 需要更新的信道 */
} chba_h2d_channel_cfg_stru; /* hmac->dmac 配置信道的结构体 */

typedef struct {
    uint16_t user_idx; /* 指定user的user_idx */
    uint8_t bitmap_level; /* 低功耗等级 取chba_ps_level_enum枚举值 */
    uint8_t resv;
} chba_user_ps_cfg_stru; /* chba user低功耗配置结构体 */

/* 给DMAC配置user级参数的结构体 */
typedef struct {
    uint16_t user_idx;
    uint32_t info_bitmap; /* 每个bit代表一个信息，如果该bit置1，表示对应信息有更新，否则无需更新 */
    uint16_t delay_th; /* 由bit0控制，CHBA VAP的空口时延门限，单位ms */
    uint8_t window_size; /* 由bit0控制，时延统计窗长，多少个包（不超过8） */
    uint16_t protected_packets_cnt; /* 由bit0控制，当上报1次干扰后，延迟该cnt个包再进行检测 */
    uint16_t buffer_threshold; /* 由bit1控制，buffer门限 */
    uint8_t overbuffer_notify_flag;
} chba_h2d_cfg_user_param_stru;

/* 给DMAC配置连接拓扑信息 */
typedef struct {
    uint32_t network_topo[MAC_CHBA_MAX_DEVICE_NUM][MAC_CHBA_MAX_BITMAP_WORD_NUM];
} chba_h2d_topo_info_stru;

/* 给DMAC配置增加或删除某个device id */
typedef struct {
    uint8_t update_type; /* 类型: 0表示增加，1表示删除, 2表示清空所有 */
    uint8_t device_id;
    uint8_t mac_addr[WLAN_MAC_ADDR_LEN]; /* 该device id对应的设备MAC地址 */
} chba_h2d_device_update_stru;

typedef struct {
    uint8_t device_id;
    uint8_t mac_addr[WLAN_MAC_ADDR_LEN]; /* 该device id对应的设备MAC地址 */
} chba_device_id_stru;

typedef struct {
    uint8_t mac_addr[WLAN_MAC_ADDR_LEN];
} chba_lost_device_info_stru;
typedef struct {
    uint8_t csa_lost_device_num;   /* 需要补救的未切信道的设备的数量 */
    mac_channel_stru old_channel;  /* 补救未切信道的设备，记录原信道信息 */
    chba_lost_device_info_stru csa_lost_device[MAC_CHBA_MAX_ISLAND_DEVICE_NUM]; /* 需要补救的设备信息 */
} chba_h2d_lost_device_info_stru;

/* dmac上报到hmac的信息 */
/* DMAC上报检测到卡顿事件的结构体 */
typedef struct {
    uint16_t user_idx;
    uint16_t delay; /* 单位：ms */
} chba_d2h_delay_exceed_stru;

/* DMAC上报user缓存mpdu数超过阈值信息 */
typedef struct {
    uint16_t user_idx;
    uint16_t all_mpdu_num; /* 当前user下mpdu总数 */
} chba_d2h_user_buffer_over_threshlod_report_stru;

/* chba模块发送到Hmac的命令结构体 */
/* 下发CHBA初始化参数结构体 */
typedef struct {
    uint8_t chba_version;                          /* 支持的CHBA版本 */
    uint8_t slot_duration;                          /* CHBA slot长度，ms为单位 */
    uint8_t schdule_period;                         /* CHBA调度周期，slot为单位 */
    uint8_t discovery_slot;                         /* CHBA social窗长度，slot为单位 */
    uint8_t init_listen_period;                     /* 初始化监听的最长时隙个数，slot为单位 */
    uint16_t vap_alive_timeout;                     /* vap处于无链路状态的最长时隙个数，slot为单位 */
    uint16_t link_alive_timeout;                    /* 链路保活的最长时隙个数，slot为单位 */
} mac_chba_config_para;
typedef struct {
    uint8_t master_election_attr[MAC_CHBA_MASTER_ELECTION_ATTR_LEN]; /* 主设备选举字段 */
    mac_chba_config_para config_para;                  /* CHBA特性配置参数 */
} mac_chba_set_init_para;

/* 下发管理帧信息结构体 */
typedef struct {
    uint8_t mgmt_frame_type;                          /* 管理帧类型 */
    uint8_t immediate_flag;                           /* 是否立即发送 */
    uint8_t peer_mac_addr[WLAN_MAC_ADDR_LEN];         /* 管理帧目标方的MAC地址 */
    uint8_t bssid[WLAN_MAC_ADDR_LEN];                 /* 管理帧域BSSID */
    /* 管理帧的payload，填写时beacon从vendor IE开始，action帧从action header开始 */
    uint8_t buf[MAC_CHBA_MGMT_MAX_PAYLOAD_BYTES];
    uint16_t buf_size;                                /* 管理帧的payload长度 */
} mac_chba_send_mgmt_frame;

/* 下发信道切换信息结构体 */
typedef struct {
    uint8_t switch_slot;                             /* 切换slot */
    mac_channel_stru channel;                        /* 目标信道信息 */
} mac_chba_set_work_chan;

/* 下发同步请求的参数 */
typedef struct {
    uint8_t chan_num; /* 工作信道的主20M信道号 */
    uint8_t peer_addr[WLAN_MAC_ADDR_LEN];
    uint8_t master_addr[WLAN_MAC_ADDR_LEN]; /* 该设备对应的master地址，用于配置域BSSID */
} sync_peer_info;
typedef struct {
    uint8_t sync_type; /* 同步请求类型 */
    sync_peer_info request_peer; /* 填写请求同步的设备信息 */
    uint8_t mgmt_type; /* sync request或PNF帧 */
    uint8_t mgmt_buf[MAC_CHBA_MGMT_SHORT_PAYLOAD_BYTES]; /* 发送管理帧 */
    uint16_t mgmt_buf_len; /* 管理帧payload长度 */
} mac_chba_set_sync_request;

/* 下发同步状态信息 */
typedef struct {
    uint8_t sync_state;
    uint8_t role;
    uint8_t master_addr[WLAN_MAC_ADDR_LEN];
    uint8_t mgmt_type; /* Beacon或PNF帧 */
    uint8_t mgmt_buf[MAC_CHBA_MGMT_MAX_PAYLOAD_BYTES]; /* 发送管理帧 */
    uint16_t mgmt_buf_len; /* 管理帧payload长度 */
} mac_chba_set_sync_status;
/* 下发扫描参数结构体 */
typedef struct {
    mac_channel_stru work_scan_chan;
    mac_channel_stru nonwork_scan_chan;
    uint32_t work_chan_scan_bitmap;
    uint32_t nonwork_chan_scan_bitmap;
} mac_chba_set_chan_scan_param;

/* Hmac通知事件给CHBA模块的消息结构体 */
/* CHBA vap start通知结构体 */
typedef struct {
    uint8_t mac_addr[WLAN_MAC_ADDR_LEN];            /* 本设备MAC地址 */
    mac_channel_stru social_channel;
    mac_channel_stru work_channel;
} mac_chba_vap_start_rpt;

/* 建链/删链信息上报结构体 */
typedef struct {
    uint8_t self_mac_addr[WLAN_MAC_ADDR_LEN];       /* 本设备MAC地址 */
    uint8_t peer_mac_addr[WLAN_MAC_ADDR_LEN];       /* 该连接的对端MAC地址 */
    mac_channel_stru work_channel;
} mac_chba_conn_info_rpt;

/* 打桩信道切换上报结构体 */
typedef struct {
    mac_channel_stru target_channel;
} mac_chba_stub_chan_switch_rpt;

/* 信道调整命令与通知结构体 */
typedef struct {
    hmac_info_report_type_enum report_type;
    uint8_t chan_number;
    uint8_t bandwidth;
    uint8_t switch_type;
    uint8_t status_code;
} mac_chba_adjust_chan_info;

/* 每个device的信息结构体 */
typedef struct {
    uint8_t mac_addr[WLAN_MAC_ADDR_LEN];                     /* 该设备的mac地址 */
    uint8_t chan_switch_ack;                        /* 是否收到了信道切换指示帧的confirm */
    uint32_t ps_bitmap;                             /* 该设备的低功耗bitmap */
    uint8_t is_conn_flag; /* 如果本设备与该节点有连接，则该flag置为1 */
} mac_chba_per_device_info_stru;

/* CHBA本设备连接信息的结构体 */
typedef struct {
    uint8_t self_device_id;                         /* 本设备的device id */
    uint8_t role;                                   /* 本设备的角色，master、岛主、其它设备 */
    uint8_t island_owner_valid;                     /* 本设备是否已获取岛主 */
    uint8_t island_owner_addr[WLAN_MAC_ADDR_LEN];            /* 本设备所属的岛主的mac地址 */
    uint8_t island_device_cnt;                      /* 本设备所在岛的设备数 */
    mac_chba_per_device_info_stru island_device_list[MAC_CHBA_MAX_ISLAND_DEVICE_NUM]; /* 本设备所在岛的设备信息列表 */
} mac_chba_self_conn_info_stru;

/* 信息结构体 */
typedef struct {
    uint8_t work_channel; /* 保存当前设备工作信道的主20M信道号 */
    uint8_t master_addr[WLAN_MAC_ADDR_LEN]; /* master的mac地址信息 */
    ranking_priority master_rp; /* 保存Master的RP值信息 */
    ranking_priority own_rp; /* 保存自身的RP值信息 */
} device_info;

/* CHBA域合并同步结构体 */
typedef struct {
    uint8_t sa_addr[WLAN_MAC_ADDR_LEN];
    master_info  another_master;
} mac_chba_multiple_domain_stru;

typedef struct {
    uint8_t dev_mac_addr[WLAN_MAC_ADDR_LEN];
    uint8_t coex_chan_cnt;
    uint8_t coex_chan_lists[MAX_CHANNEL_NUM_FREQ_5G];
} mac_chba_rpt_coex_info;

/* 上层下发共存vap的可支持信道列表 */
typedef struct {
    uint8_t cfg_cmd_type; // 标识当前共存信道集合属于本设备还是本岛
    uint8_t supported_channel_cnt; /* 支持的信道个数 */
    uint8_t supported_channel_list[MAX_CHANNEL_NUM_FREQ_5G]; /* 支持的信道号列表 */
} mac_chba_set_coex_chan_info;

#define chba_bitmap_to_level(_bitmap) (       \
        ((_bitmap) == CHBA_PS_ALL_AWAKE_BITMAP) ? CHBA_BITMAP_ALL_AWAKE_LEVEL : \
        ((_bitmap) == CHBA_PS_HALF_AWAKE_BITMAP) ? CHBA_BITMAP_HALF_AWAKE_LEVEL : \
        ((_bitmap) == CHBA_PS_KEEP_ALIVE_BITMAP) ? CHBA_BITMAP_KEEP_ALIVE_LEVEL : CHBA_BITMAP_LEVEL_BUTT)

#define chba_level_to_bitmap(_level) (       \
        ((_level) == CHBA_BITMAP_ALL_AWAKE_LEVEL) ? CHBA_PS_ALL_AWAKE_BITMAP : \
        ((_level) == CHBA_BITMAP_HALF_AWAKE_LEVEL) ? CHBA_PS_HALF_AWAKE_BITMAP : \
        ((_level) == CHBA_BITMAP_KEEP_ALIVE_LEVEL) ? CHBA_PS_KEEP_ALIVE_BITMAP : 0)

/* CHBA FRAME相关结构体 */
#if (defined(_PRE_PRODUCT_ID_HI110X_DEV))
/* 02 dev侧用#pragma pack(1)/#pragma pack()方式达到一字节对齐 */
#pragma pack(1)
#endif
/* linkinfo信息 */
struct chba_link_info {
    uint8_t  eid;
    uint8_t  len;
    uint8_t channel_num;
    uint8_t bw;
    uint8_t rssi;
    uint8_t linkcnt;
} __OAL_DECLARE_PACKED;
typedef struct chba_link_info chba_link_info_stru;

struct chba_ps_bitmap {
    uint8_t eid;
    uint8_t len;
    uint8_t flag;
    uint32_t ps_bitmap;
} __OAL_DECLARE_PACKED;
typedef struct chba_ps_bitmap chba_ps_bitmap_stru; /* chba powe_save_ie */

struct chba_device_capability {
    uint8_t eid;
    uint8_t len;
    uint8_t max_channel_switch_time;
    uint8_t social_channel_support;
} __OAL_DECLARE_PACKED;
typedef struct chba_device_capability chba_dev_cap_stru; /* chba device_capability_ie */
#if (defined(_PRE_PRODUCT_ID_HI110X_DEV))
/* 02 dev侧用#pragma pack(1)/#pragma pack()方式达到一字节对齐 */
#pragma pack()
#endif

/* CHBA 信道序列 bitmap 宏定义 */
#define CHBA_CHANNEL_SEQ_BITMAP_25_PERCENT (uint32_t)0x03030303
#define CHBA_CHANNEL_SEQ_BITMAP_50_PERCENT (uint32_t)0x33333333
#define CHBA_CHANNEL_SEQ_BITMAP_75_PERCENT (uint32_t)0x3F3F3F3F
#define CHBA_CHANNEL_SEQ_BITMAP_100_PERCENT (uint32_t)0xFFFFFFFF

/* CHBA 信道序列 bitmap档位枚举 */
enum chba_dbac_level {
    CHBA_CHANNEL_SEQ_LEVEL0 = 0,
    CHBA_CHANNEL_SEQ_LEVEL1 = 1,
    CHBA_CHANNEL_SEQ_LEVEL2 = 2,

    CHBA_CHANNEL_SEQ_LEVEL_BUTT
};

/* CHBA 信道序列配置参数 */
typedef struct chba_channel_sequence_params {
    uint32_t channel_seq_level;
    uint32_t first_work_channel;
    uint32_t sec_work_channel;
} chba_channel_sequence_params_stru;

struct chba_channel_sequence_member {
    uint8_t channel_number;
    wlan_channel_bandwidth_enum_uint8 bandwidth;
} __OAL_DECLARE_PACKED;

#endif
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of mac_common.h */

