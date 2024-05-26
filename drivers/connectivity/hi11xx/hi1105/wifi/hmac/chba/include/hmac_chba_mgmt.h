

#ifndef __HMAC_CHBA_MGMT_H__
#define __HMAC_CHBA_MGMT_H__

#include "hmac_chba_common_function.h"
#include "hihash.h"
#include "hilist.h"
#include "hmac_vap.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_MGMT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _PRE_WLAN_CHBA_MGMT
/* 1 宏定义 */
/* 规格相关的宏 */
#define MAC_CHBA_VERSION_2 2 /* CHBA 2.0版本 */
#define MAC_CHBA_SLOT_DURATION 16 /* 时隙长度，单位为ms */
#define MAC_CHBA_SCHEDULE_PERIOD 32 /* 调度周期的时隙个数 */
#define MAC_CHBA_SYNC_SLOT_CNT 1 /* social窗的时隙个数 */
#define MAC_CHBA_INIT_LISTEN_PERIOD 64 /* 初始化监听的最长时隙个数 */
#define MAC_CHBA_VAP_ALIVE_TIMEOUT 1024 /* vap处于无链路状态的最长时隙个数 */
#define MAC_CHBA_LINK_ALIVE_TIMEOUT 1024 /* 链路保活的最长时隙个数 */
#define MAC_CHBA_DEVICE_ALIVE_TIMEOUT 300000 /* 设备不活跃时间超过该值，从设备表中删除，单位为毫秒 */
#define MAC_CHBA_ISLAND_OWNER_PNF_LOSS_TH 6
#define MAC_CHBA_ISLAND_OWNER_CHANGE_TH 20 /* 岛主变更时的RSSI阈值 */

#define REMAIN_POWER_PERCENT_70 70
#define REMAIN_POWER_PERCENT_50 50
#define REMAIN_POWER_PERCENT_30 30

#define PROPERTY_VALUE_MAX 128

#define chba_bit(idx) (0x00000001U << (idx))
#define hmac_chba_get_device_info()         (&g_chba_device_info)
#define hmac_chba_get_mgmt_info()           (&g_chba_mgmt_info)
#define hmac_chba_get_module_state()        (g_chba_mgmt_info.chba_module_state)
#define hmac_chba_set_module_state(val)     (g_chba_mgmt_info.chba_module_state = (val))
#define hmac_chba_get_alive_device_table()  (g_chba_mgmt_info.alive_device_table)
#define hmac_chba_get_island_list()         (&g_chba_mgmt_info.island_list)
#define hmac_chba_get_device_id_info()      (&g_chba_mgmt_info.device_id_info[0])
#define hmac_chba_get_self_conn_info()      (&g_chba_mgmt_info.self_conn_info)
#define hmac_chba_get_network_topo_addr()   (&g_chba_mgmt_info.network_topo[0][0])
#define hmac_chba_get_network_topo_row(row) (&g_chba_mgmt_info.network_topo[(row)][0])
#define hmac_chba_get_role()                (g_chba_mgmt_info.self_conn_info.role)
#define hmac_chba_get_own_device_id()       (g_chba_mgmt_info.self_conn_info.self_device_id)

/* 获取连接拓扑图中[row][col]对应的bit的值 */
#define hmac_chba_get_network_topo_ele(row, col) \
    ((g_chba_mgmt_info.network_topo[(row)][(col) / UINT32_BIT_NUM] \
        >> (UINT32_MAX_BIT_IDX - ((col) % UINT32_BIT_NUM))) & 0x1)
/* 将连接拓扑图中[row][col]对应的bit置为0 */
#define hmac_chba_set_network_topo_ele_false(row, col) \
    (g_chba_mgmt_info.network_topo[(row)][(col) / UINT32_BIT_NUM] \
    &= ~(chba_bit(UINT32_MAX_BIT_IDX - ((col) % UINT32_BIT_NUM))))
/* 将连接拓扑图中[row][col]对应的bit置为1 */
#define hmac_chba_set_network_topo_ele_true(row, col) \
    (g_chba_mgmt_info.network_topo[(row)][(col) / UINT32_BIT_NUM] \
    |= chba_bit(UINT32_MAX_BIT_IDX - ((col) % UINT32_BIT_NUM)))

/* 2 枚举定义 */
/* 设备类型枚举，更改该枚举要同步更新Driver中的对应枚举类型 */
typedef enum {
    DEVICE_TYPE_UNKNOWN = 0,                        /* 未知设备类型 */
    DEVICE_TYPE_IOT = 1,                            /* Hilink生态圈设备 */
    DEVICE_TYPE_WEAR = 2,                           /* 穿戴设备 */
    DEVICE_TYPE_CAR = 3,                            /* 车机设备 */
    DEVICE_TYPE_SOUNDBOX = 4,                       /* 音箱 */
    DEVICE_TYPE_TABLET = 5,                         /* 平板 */
    DEVICE_TYPE_HANDSET = 6,                        /* 手机 */
    DEVICE_TYPE_PC = 7,                             /* PC */
    DEVICE_TYPE_TV = 8,                             /* TV */
    DEVICE_TYPE_ROUTER_CPE = 9,                     /* 路由器、CPE */
    DEVICE_TYPE_BUTT
} device_type_enum;

/* 设备剩余电量枚举，更改该枚举要同步更新Driver中的对应枚举类型 */
typedef enum {
    BATTERY_LEVEL_VERY_LOW = 0,                     /* 低电量，电量小于30% */
    BATTERY_LEVEL_LOW = 1,                          /* 中低电量，电量大于等于30%，小于50% */
    BATTERY_LEVEL_MIDDLE = 2,                       /* 中电量，电量大于等于50%，小于70% */
    BATTERY_LEVEL_HIGH = 3,                         /* 高电量，电量大于等于70% */
} battery_level_enum;

/* 硬件能力枚举，更改该枚举要同步更新Driver中的对应枚举类型 */
typedef enum {
    HW_CAP_SINGLE_CHIP = 0,                         /* 单芯片 */
    HW_CAP_DUAL_CHIP_SINGLE_CHBA = 1,              /* 大+小芯片，单芯片支持CHBA */
    HW_CAP_DUAL_CHIP_DUAL_CHBA = 2,                /* 双芯片，双芯片均支持CHBA */
} hardware_capability_enum;

/* 频段能力枚举，更改该枚举要同步更新Driver中的对应枚举类型 */
typedef enum {
    BAND_CAP_2G_ONLY = 0,                           /* 只支持2G频段 */
    BAND_CAP_5G_ONLY = 1,                           /* 只支持5G频段 */
    BAND_CAP_2G_5G = 2,                             /* 2G和5G频段均支持 */
} band_capability_enum;

/* CHBA模块状态枚举 */
typedef enum {
    MAC_CHBA_MODULE_STATE_UNINIT = 0, /* 未初始化态 */
    MAC_CHBA_MODULE_STATE_INIT = 1, /* 初始化后进入该状态，VAP stop后也回归到该状态 */
    MAC_CHBA_MODULE_STATE_UP = 2, /* Vap Start后进入该状态 */
} hmac_chba_module_state_enum;

/* 用户统计信息上报时刻位置枚举 */
typedef enum {
    MAC_CHBA_USER_STAT_RPT_ENTER_DISC = 0,
    MAC_CHBA_USER_STAT_RPT_EXIT_DISC = 1,
    MAC_CHBA_USER_STAT_RPT_ENTER_SCAN = 2,
    MAC_CHBA_USER_STAT_RPT_EXIT_SCAN = 3,
    MAC_CHBA_USER_STAT_RPT_OTHER = 4,

    MAC_CHBA_USER_STAT_RPT_BUTT
} hmac_chba_user_stat_rpt_type_enum;

typedef enum {
    CHBA_CHAN_SWITCH = 0,
    CHBA_PS_SWITCH = 1,

    CHBA_FEATURE_SWITCH_BUTT,
} chba_feature_switch_cmd_enum;

/* 3 结构体定义 */
/* CHBA哈希表node的结构体 */
typedef struct {
    struct hi_node node;
    uint8_t mac_addr[OAL_MAC_ADDR_LEN];                    /* key值，设备的mac地址 */
    uint8_t device_id;                             /* 设备id */
    uint32_t last_alive_time;                      /* 上一次记录的设备活跃时间, s为单位 */
} hmac_chba_hash_node_stru;

/* CHBA 活跃设备哈希表的结构体 */
typedef struct {
    struct hi_hash_table raw;
    size_t node_cnt;
} hmac_chba_alive_device_table_stru;

/* CHBA本设备硬件信息的结构体 */
typedef struct {
    uint8_t device_type;                            /* 设备类型 */
    uint8_t remain_power;                           /* 设备剩余电量的等级 */
    uint8_t hardwire_cap;                           /* 设备的硬件能力, 0:单芯片，1:双芯片单CHBA，2: 双芯片双CHBA */
    uint8_t drv_band_cap;                           /* 设备支持的频段能力，0: 2.4G only, 1: 5G only, 2: 2.4G和5G */
    mac_chba_config_para config_para;                  /* HiD2d特性配置参数 */
} hmac_chba_device_para_stru;

/* HiD2D单个岛临时信息的结构体 */
typedef struct {
    uint8_t island_id;                              /* 岛ID */
    uint8_t owner_valid;                            /* 该岛是否已获取岛主 */
    uint8_t owner_device_id;                        /* 岛主的device id */
    uint8_t pnf_loss_cnt;                           /* 连续未收到岛主pnf的次数 */
    uint8_t change_flag;                            /* 岛内设备列表是否变化 */
    uint8_t device_cnt;                             /* 岛内设备数 */
    uint8_t device_list[MAC_CHBA_MAX_DEVICE_NUM];   /* 岛内设备device id列表 */
} hmac_chba_per_island_tmp_info_stru;

/* HiD2D临时岛信息链表节点的结构体 */
typedef struct {
    struct hi_node node;
    hmac_chba_per_island_tmp_info_stru data;
} hmac_chba_tmp_island_list_node_stru;

/* HiD2D单个岛信息的结构体 */
typedef struct {
    uint8_t owner_device_id;                        /* 岛主的device id */
    uint8_t pnf_loss_cnt;                           /* 连续未收到岛主pnf的次数 */
    uint8_t device_cnt;                             /* 岛内设备数 */
    uint8_t device_list[MAC_CHBA_MAX_DEVICE_NUM];   /* 岛内设备device id列表 */
} hmac_chba_per_island_info_stru;

/* CHBA岛信息链表节点的结构体 */
typedef struct {
    struct hi_node node;
    hmac_chba_per_island_info_stru data;
} hmac_chba_island_list_node_stru;

/* CHBA本设备维护的所有岛信息的结构体 */
typedef struct {
    uint8_t island_cnt;                                  /* 岛个数 */
    struct hi_list island_list;                          /* 岛信息链表 */
} hmac_chba_island_list_stru;
typedef struct {
    hmac_info_report_type_enum report_type;    /* 上报消息类型 */
    uint8_t role;                          /* 角色信息 */
} hmac_chba_role_report_stru;

/* CHBA设备device id的信息结构体 */
typedef struct {
    uint8_t use_flag;                               /* 该device id是否已被分配使用 */
    uint8_t mac_addr[OAL_MAC_ADDR_LEN];             /* 该device id对应的设备MAC地址 */
    uint8_t chan_number;                            /* 该device id的工作信道主20M的信道号 */
    uint8_t bandwidth;                              /* 该device id的工作信道的带宽模式 */
    uint8_t pnf_rcv_flag;                           /* 本周期是否接收到该device id的PNF帧 */
    int8_t pnf_rssi;                                /* 该device id的PNF帧的接收rssi，单位dBm */
    uint32_t ps_bitmap;                             /* 该device id的低功耗bitmap */
} hmac_chba_per_device_id_info_stru;

/* 优选/备选信道信息结构体 */
typedef struct {
    uint8_t first_sel_chan_num; /* 优选信道号 */
    uint8_t first_sel_chan_bw; /* 优选信道带宽 */
    uint8_t second_sel_chan_num; /* 备选信道号 */
    uint8_t second_sel_chan_bw; /* 备选信道带宽 */
    uint8_t common_sel_chan_num; /* 公共备选信道号 */
    uint8_t common_sel_chan_bw; /* 公共备选信道带宽 */
} hmac_chba_select_chan_info_stru;

typedef struct {
    uint8_t csa_ack;
    uint8_t mac_addr[WLAN_MAC_ADDR_LEN];
} hmac_chba_lost_device_info_stru;
/* 信道切换信息结构体 */
typedef struct {
    uint32_t last_send_req_time; /* 上一次发送信道切换请求的时间 */
    uint8_t switch_type;
    uint8_t req_timeout_cnt; /* 连续请求超时的次数 */
    uint8_t in_island_chan_switch; /* 是否处于岛信道切换流程中 */
    uint8_t last_first_sel_chan; /* 上一次信道切换决策时的优选信道号 */
    uint8_t last_first_sel_chan_bw; /* 上一次信道切换决策时的优选信道带宽 */
    uint8_t last_target_channel; /* 上一次决策信道切换的目标信道号 */
    uint8_t last_target_channel_bw; /* 上一次决策信道切换的目标信道带宽 */
    uint8_t last_chan_switch_state; /* 上一次决策信道切换的状态 */
    uint32_t last_chan_switch_time; /* 上一次决策信道需要切换的时间 */
    uint8_t csa_lost_device_num;   /* 需要补救的未切信道的设备的数量 */
    uint8_t csa_save_lost_device_flag; /* 补救开始标志位 */
    mac_channel_stru old_channel;  /* 补救未切信道的设备，记录原信道信息 */
    hmac_chba_lost_device_info_stru csa_lost_device[MAC_CHBA_MAX_ISLAND_DEVICE_NUM]; /* 需要补救的设备信息 */

    frw_timeout_stru req_timer;
    frw_timeout_stru ack_timer;
    frw_timeout_stru chan_switch_timer;
    frw_timeout_stru chan_switch_end_timer;
} hmac_chba_chan_switch_info_stru;

/* CHBA管理信息的结构体 */
typedef struct {
    uint8_t chba_module_state; /* CHBA模块状态 */

    hmac_chba_alive_device_table_stru *alive_device_table; /* CHBA活跃设备表，哈希表 */
    /* CHBA连接拓扑图, 每一行4个uint32对应128bit, bit顺序0~31|32~63|64~95|96~127 */
    uint32_t network_topo[MAC_CHBA_MAX_DEVICE_NUM][MAC_CHBA_MAX_BITMAP_WORD_NUM];
    hmac_chba_island_list_stru island_list; /* 岛信息链表 */
    hmac_chba_per_device_id_info_stru device_id_info[MAC_CHBA_MAX_DEVICE_NUM]; /* CHBA device id的分配信息 */
    mac_chba_self_conn_info_stru self_conn_info; /* CHBA本设备的连接信息 */
    uint8_t island_owner_attr_len; /* 岛主属性的长度 */
    uint8_t island_owner_attr_buf[MAC_CHBA_MGMT_MID_PAYLOAD_BYTES]; /* 岛主属性的buffer */

    hmac_chba_chan_switch_info_stru chan_switch_info; /* 信道切换信息 */
} hmac_chba_mgmt_info_stru;

typedef struct {
    uint32_t cmd_bitmap;
    uint8_t chan_switch_enable;
    uint8_t ps_enable;
} mac_chba_feature_switch_stru;

/* 4 全局变量声明 */
extern hmac_chba_device_para_stru g_chba_device_info;
extern hmac_chba_mgmt_info_stru g_chba_mgmt_info;

/* 5 函数声明 */
uint8_t *hmac_chba_get_own_mac_addr(void);
void hmac_chba_set_role(uint8_t role);
void hmac_chba_clear_device_mgmt_info(void);
uint8_t hmac_chba_one_dev_update_alive_dev_table(const uint8_t* mac_addr);
void hmac_chba_beacon_update_device_mgmt_info(uint8_t *sa_addr, uint8_t *payload,
    uint16_t payload_len, uint8_t is_my_user);
uint8_t hmac_chba_update_dev_mgmt_info_by_frame(uint8_t *payload, int32_t len,
    uint8_t *peer_addr, int8_t *rssi, uint8_t *linkcnt, uint8_t is_my_user);
void hmac_chba_update_island_info(hmac_vap_stru *hmac_vap, hmac_chba_island_list_stru *island_info);
void hmac_chba_print_island_info(hmac_chba_island_list_stru *island_info);
void hmac_chba_update_self_conn_info(hmac_vap_stru *hmac_vap, hmac_chba_island_list_stru *tmp_island_info,
    uint8_t owner_info_flag);
void hmac_chba_print_self_conn_info(void);
uint8_t hmac_chba_rx_beacon_island_owner_attr_handler(uint8_t *sa_addr, uint8_t *payload,
    uint16_t payload_len);
void hmac_chba_clear_island_info(hmac_chba_island_list_stru *island_info, hi_node_func node_deinit);
void hmac_chba_tmp_island_list_node_free(struct hi_node *node);

/* 处理hmac上报的事件 */
void hmac_chba_module_init(uint8_t device_type);
void hmac_chba_battery_update_proc(uint8_t percent);
void hmac_chba_vap_start_proc(mac_chba_vap_start_rpt *info_report);
void hmac_chba_vap_stop_proc(uint8_t *recv_msg);
void hmac_chba_add_conn_proc(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user);
void hmac_chba_update_topo_info_proc(hmac_vap_stru *hmac_vap);
void hmac_chba_island_owner_selection_proc(hmac_chba_island_list_stru *tmp_island_info);
uint32_t hmac_chba_module_init_cmd(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params);
void hmac_chba_update_network_topo(uint8_t *own_mac_addr, uint8_t *peer_mac_addr);
uint32_t hmac_config_chba_set_battery(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params);
uint32_t hmac_chba_del_non_alive_device(mac_vap_stru *mac_vap, uint8_t len, uint8_t *param);
mac_chba_per_device_info_stru *hmac_chba_find_island_device_info(const uint8_t* mac_addr);
#endif

#ifdef __cplusplus
}
#endif

#endif
