

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
/* 1 �궨�� */
/* �����صĺ� */
#define MAC_CHBA_VERSION_2 2 /* CHBA 2.0�汾 */
#define MAC_CHBA_SLOT_DURATION 16 /* ʱ϶���ȣ���λΪms */
#define MAC_CHBA_SCHEDULE_PERIOD 32 /* �������ڵ�ʱ϶���� */
#define MAC_CHBA_SYNC_SLOT_CNT 1 /* social����ʱ϶���� */
#define MAC_CHBA_INIT_LISTEN_PERIOD 64 /* ��ʼ���������ʱ϶���� */
#define MAC_CHBA_VAP_ALIVE_TIMEOUT 1024 /* vap��������·״̬���ʱ϶���� */
#define MAC_CHBA_LINK_ALIVE_TIMEOUT 1024 /* ��·������ʱ϶���� */
#define MAC_CHBA_DEVICE_ALIVE_TIMEOUT 300000 /* �豸����Ծʱ�䳬����ֵ�����豸����ɾ������λΪ���� */
#define MAC_CHBA_ISLAND_OWNER_PNF_LOSS_TH 6
#define MAC_CHBA_ISLAND_OWNER_CHANGE_TH 20 /* �������ʱ��RSSI��ֵ */

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

/* ��ȡ��������ͼ��[row][col]��Ӧ��bit��ֵ */
#define hmac_chba_get_network_topo_ele(row, col) \
    ((g_chba_mgmt_info.network_topo[(row)][(col) / UINT32_BIT_NUM] \
        >> (UINT32_MAX_BIT_IDX - ((col) % UINT32_BIT_NUM))) & 0x1)
/* ����������ͼ��[row][col]��Ӧ��bit��Ϊ0 */
#define hmac_chba_set_network_topo_ele_false(row, col) \
    (g_chba_mgmt_info.network_topo[(row)][(col) / UINT32_BIT_NUM] \
    &= ~(chba_bit(UINT32_MAX_BIT_IDX - ((col) % UINT32_BIT_NUM))))
/* ����������ͼ��[row][col]��Ӧ��bit��Ϊ1 */
#define hmac_chba_set_network_topo_ele_true(row, col) \
    (g_chba_mgmt_info.network_topo[(row)][(col) / UINT32_BIT_NUM] \
    |= chba_bit(UINT32_MAX_BIT_IDX - ((col) % UINT32_BIT_NUM)))

/* 2 ö�ٶ��� */
/* �豸����ö�٣����ĸ�ö��Ҫͬ������Driver�еĶ�Ӧö������ */
typedef enum {
    DEVICE_TYPE_UNKNOWN = 0,                        /* δ֪�豸���� */
    DEVICE_TYPE_IOT = 1,                            /* Hilink��̬Ȧ�豸 */
    DEVICE_TYPE_WEAR = 2,                           /* �����豸 */
    DEVICE_TYPE_CAR = 3,                            /* �����豸 */
    DEVICE_TYPE_SOUNDBOX = 4,                       /* ���� */
    DEVICE_TYPE_TABLET = 5,                         /* ƽ�� */
    DEVICE_TYPE_HANDSET = 6,                        /* �ֻ� */
    DEVICE_TYPE_PC = 7,                             /* PC */
    DEVICE_TYPE_TV = 8,                             /* TV */
    DEVICE_TYPE_ROUTER_CPE = 9,                     /* ·������CPE */
    DEVICE_TYPE_BUTT
} device_type_enum;

/* �豸ʣ�����ö�٣����ĸ�ö��Ҫͬ������Driver�еĶ�Ӧö������ */
typedef enum {
    BATTERY_LEVEL_VERY_LOW = 0,                     /* �͵���������С��30% */
    BATTERY_LEVEL_LOW = 1,                          /* �е͵������������ڵ���30%��С��50% */
    BATTERY_LEVEL_MIDDLE = 2,                       /* �е������������ڵ���50%��С��70% */
    BATTERY_LEVEL_HIGH = 3,                         /* �ߵ������������ڵ���70% */
} battery_level_enum;

/* Ӳ������ö�٣����ĸ�ö��Ҫͬ������Driver�еĶ�Ӧö������ */
typedef enum {
    HW_CAP_SINGLE_CHIP = 0,                         /* ��оƬ */
    HW_CAP_DUAL_CHIP_SINGLE_CHBA = 1,              /* ��+СоƬ����оƬ֧��CHBA */
    HW_CAP_DUAL_CHIP_DUAL_CHBA = 2,                /* ˫оƬ��˫оƬ��֧��CHBA */
} hardware_capability_enum;

/* Ƶ������ö�٣����ĸ�ö��Ҫͬ������Driver�еĶ�Ӧö������ */
typedef enum {
    BAND_CAP_2G_ONLY = 0,                           /* ֻ֧��2GƵ�� */
    BAND_CAP_5G_ONLY = 1,                           /* ֻ֧��5GƵ�� */
    BAND_CAP_2G_5G = 2,                             /* 2G��5GƵ�ξ�֧�� */
} band_capability_enum;

/* CHBAģ��״̬ö�� */
typedef enum {
    MAC_CHBA_MODULE_STATE_UNINIT = 0, /* δ��ʼ��̬ */
    MAC_CHBA_MODULE_STATE_INIT = 1, /* ��ʼ��������״̬��VAP stop��Ҳ�ع鵽��״̬ */
    MAC_CHBA_MODULE_STATE_UP = 2, /* Vap Start������״̬ */
} hmac_chba_module_state_enum;

/* �û�ͳ����Ϣ�ϱ�ʱ��λ��ö�� */
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

/* 3 �ṹ�嶨�� */
/* CHBA��ϣ��node�Ľṹ�� */
typedef struct {
    struct hi_node node;
    uint8_t mac_addr[OAL_MAC_ADDR_LEN];                    /* keyֵ���豸��mac��ַ */
    uint8_t device_id;                             /* �豸id */
    uint32_t last_alive_time;                      /* ��һ�μ�¼���豸��Ծʱ��, sΪ��λ */
} hmac_chba_hash_node_stru;

/* CHBA ��Ծ�豸��ϣ��Ľṹ�� */
typedef struct {
    struct hi_hash_table raw;
    size_t node_cnt;
} hmac_chba_alive_device_table_stru;

/* CHBA���豸Ӳ����Ϣ�Ľṹ�� */
typedef struct {
    uint8_t device_type;                            /* �豸���� */
    uint8_t remain_power;                           /* �豸ʣ������ĵȼ� */
    uint8_t hardwire_cap;                           /* �豸��Ӳ������, 0:��оƬ��1:˫оƬ��CHBA��2: ˫оƬ˫CHBA */
    uint8_t drv_band_cap;                           /* �豸֧�ֵ�Ƶ��������0: 2.4G only, 1: 5G only, 2: 2.4G��5G */
    mac_chba_config_para config_para;                  /* HiD2d�������ò��� */
} hmac_chba_device_para_stru;

/* HiD2D��������ʱ��Ϣ�Ľṹ�� */
typedef struct {
    uint8_t island_id;                              /* ��ID */
    uint8_t owner_valid;                            /* �õ��Ƿ��ѻ�ȡ���� */
    uint8_t owner_device_id;                        /* ������device id */
    uint8_t pnf_loss_cnt;                           /* ����δ�յ�����pnf�Ĵ��� */
    uint8_t change_flag;                            /* �����豸�б��Ƿ�仯 */
    uint8_t device_cnt;                             /* �����豸�� */
    uint8_t device_list[MAC_CHBA_MAX_DEVICE_NUM];   /* �����豸device id�б� */
} hmac_chba_per_island_tmp_info_stru;

/* HiD2D��ʱ����Ϣ����ڵ�Ľṹ�� */
typedef struct {
    struct hi_node node;
    hmac_chba_per_island_tmp_info_stru data;
} hmac_chba_tmp_island_list_node_stru;

/* HiD2D��������Ϣ�Ľṹ�� */
typedef struct {
    uint8_t owner_device_id;                        /* ������device id */
    uint8_t pnf_loss_cnt;                           /* ����δ�յ�����pnf�Ĵ��� */
    uint8_t device_cnt;                             /* �����豸�� */
    uint8_t device_list[MAC_CHBA_MAX_DEVICE_NUM];   /* �����豸device id�б� */
} hmac_chba_per_island_info_stru;

/* CHBA����Ϣ����ڵ�Ľṹ�� */
typedef struct {
    struct hi_node node;
    hmac_chba_per_island_info_stru data;
} hmac_chba_island_list_node_stru;

/* CHBA���豸ά�������е���Ϣ�Ľṹ�� */
typedef struct {
    uint8_t island_cnt;                                  /* ������ */
    struct hi_list island_list;                          /* ����Ϣ���� */
} hmac_chba_island_list_stru;
typedef struct {
    hmac_info_report_type_enum report_type;    /* �ϱ���Ϣ���� */
    uint8_t role;                          /* ��ɫ��Ϣ */
} hmac_chba_role_report_stru;

/* CHBA�豸device id����Ϣ�ṹ�� */
typedef struct {
    uint8_t use_flag;                               /* ��device id�Ƿ��ѱ�����ʹ�� */
    uint8_t mac_addr[OAL_MAC_ADDR_LEN];             /* ��device id��Ӧ���豸MAC��ַ */
    uint8_t chan_number;                            /* ��device id�Ĺ����ŵ���20M���ŵ��� */
    uint8_t bandwidth;                              /* ��device id�Ĺ����ŵ��Ĵ���ģʽ */
    uint8_t pnf_rcv_flag;                           /* �������Ƿ���յ���device id��PNF֡ */
    int8_t pnf_rssi;                                /* ��device id��PNF֡�Ľ���rssi����λdBm */
    uint32_t ps_bitmap;                             /* ��device id�ĵ͹���bitmap */
} hmac_chba_per_device_id_info_stru;

/* ��ѡ/��ѡ�ŵ���Ϣ�ṹ�� */
typedef struct {
    uint8_t first_sel_chan_num; /* ��ѡ�ŵ��� */
    uint8_t first_sel_chan_bw; /* ��ѡ�ŵ����� */
    uint8_t second_sel_chan_num; /* ��ѡ�ŵ��� */
    uint8_t second_sel_chan_bw; /* ��ѡ�ŵ����� */
    uint8_t common_sel_chan_num; /* ������ѡ�ŵ��� */
    uint8_t common_sel_chan_bw; /* ������ѡ�ŵ����� */
} hmac_chba_select_chan_info_stru;

typedef struct {
    uint8_t csa_ack;
    uint8_t mac_addr[WLAN_MAC_ADDR_LEN];
} hmac_chba_lost_device_info_stru;
/* �ŵ��л���Ϣ�ṹ�� */
typedef struct {
    uint32_t last_send_req_time; /* ��һ�η����ŵ��л������ʱ�� */
    uint8_t switch_type;
    uint8_t req_timeout_cnt; /* ��������ʱ�Ĵ��� */
    uint8_t in_island_chan_switch; /* �Ƿ��ڵ��ŵ��л������� */
    uint8_t last_first_sel_chan; /* ��һ���ŵ��л�����ʱ����ѡ�ŵ��� */
    uint8_t last_first_sel_chan_bw; /* ��һ���ŵ��л�����ʱ����ѡ�ŵ����� */
    uint8_t last_target_channel; /* ��һ�ξ����ŵ��л���Ŀ���ŵ��� */
    uint8_t last_target_channel_bw; /* ��һ�ξ����ŵ��л���Ŀ���ŵ����� */
    uint8_t last_chan_switch_state; /* ��һ�ξ����ŵ��л���״̬ */
    uint32_t last_chan_switch_time; /* ��һ�ξ����ŵ���Ҫ�л���ʱ�� */
    uint8_t csa_lost_device_num;   /* ��Ҫ���ȵ�δ���ŵ����豸������ */
    uint8_t csa_save_lost_device_flag; /* ���ȿ�ʼ��־λ */
    mac_channel_stru old_channel;  /* ����δ���ŵ����豸����¼ԭ�ŵ���Ϣ */
    hmac_chba_lost_device_info_stru csa_lost_device[MAC_CHBA_MAX_ISLAND_DEVICE_NUM]; /* ��Ҫ���ȵ��豸��Ϣ */

    frw_timeout_stru req_timer;
    frw_timeout_stru ack_timer;
    frw_timeout_stru chan_switch_timer;
    frw_timeout_stru chan_switch_end_timer;
} hmac_chba_chan_switch_info_stru;

/* CHBA������Ϣ�Ľṹ�� */
typedef struct {
    uint8_t chba_module_state; /* CHBAģ��״̬ */

    hmac_chba_alive_device_table_stru *alive_device_table; /* CHBA��Ծ�豸����ϣ�� */
    /* CHBA��������ͼ, ÿһ��4��uint32��Ӧ128bit, bit˳��0~31|32~63|64~95|96~127 */
    uint32_t network_topo[MAC_CHBA_MAX_DEVICE_NUM][MAC_CHBA_MAX_BITMAP_WORD_NUM];
    hmac_chba_island_list_stru island_list; /* ����Ϣ���� */
    hmac_chba_per_device_id_info_stru device_id_info[MAC_CHBA_MAX_DEVICE_NUM]; /* CHBA device id�ķ�����Ϣ */
    mac_chba_self_conn_info_stru self_conn_info; /* CHBA���豸��������Ϣ */
    uint8_t island_owner_attr_len; /* �������Եĳ��� */
    uint8_t island_owner_attr_buf[MAC_CHBA_MGMT_MID_PAYLOAD_BYTES]; /* �������Ե�buffer */

    hmac_chba_chan_switch_info_stru chan_switch_info; /* �ŵ��л���Ϣ */
} hmac_chba_mgmt_info_stru;

typedef struct {
    uint32_t cmd_bitmap;
    uint8_t chan_switch_enable;
    uint8_t ps_enable;
} mac_chba_feature_switch_stru;

/* 4 ȫ�ֱ������� */
extern hmac_chba_device_para_stru g_chba_device_info;
extern hmac_chba_mgmt_info_stru g_chba_mgmt_info;

/* 5 �������� */
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

/* ����hmac�ϱ����¼� */
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
