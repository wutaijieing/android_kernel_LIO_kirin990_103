

#ifndef __MAC_CHBA_COMMON_H__
#define __MAC_CHBA_COMMON_H__

/* 1 ����ͷ�ļ����� */
#include "oal_types.h"
#include "wlan_types.h"
#include "wlan_spec.h"
#include "mac_device.h"
#include "mac_vap.h"
#include "mac_user.h"

/* �˴�����extern "C" UT���벻�� */
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_CHBA_COMMON_H

#ifdef _PRE_WLAN_CHBA_MGMT
/* �����غ� */
#define MAC_CHBA_MAX_DEVICE_NUM 32
#define MAC_CHBA_MAX_BITMAP_WORD_NUM 4
#define MAC_CHBA_MAX_ISLAND_NUM 32
#define MAC_CHBA_MAX_ISLAND_DEVICE_NUM 32
#define MAC_CHBA_MAX_LINK_NUM 4
#define MAC_CHBA_MAX_WHITELISI_NUM 16
#define MAC_CHBA_REQUEST_LIST_NUM 8

#define MAC_CHBA_MAX_RPT_BANDWIDTH_CNT 4 /* ��ʾ��20M����40M����80M����160M */
#define MAC_CHBA_MAX_COEX_VAP_NUM 6
#define MAC_CHBA_MAX_VAP_NAME_LEN 16

/* ����֡��غ� */
#define MAC_CHBA_MGMT_MAX_PAYLOAD_BYTES 512
#define MAC_CHBA_MGMT_MID_PAYLOAD_BYTES 256
#define MAC_CHBA_MGMT_SHORT_PAYLOAD_BYTES 128
#define MAC_CHBA_PNF_MAX_PAYLOAD_BYTES MAC_CHBA_MGMT_MID_PAYLOAD_BYTES
#define MAC_CHBA_VENDOR_EID 221

#define MAC_CHBA_VENDOR_IE 0x00E0FC
#define MAC_OUI_TYPE_CHBA 0x88 /* ���� */
#define MAC_CHBA_BEACON_ELEMENT_OFFSET 12 /* Beacon�̶��ֶ�ƫ�� */
#define MAC_CHBA_VENDOR_IE_HDR_OFFSET 6 /* IEͷƫ�� */
#define MAC_CHBA_ACTION_OUI_TYPE_LEN 4
#define MAC_CHBA_ACTION_TYPE_LEN 5
#define MAC_CHBA_ACTION_HDR_LEN 6
#define MAC_CHBA_ATTR_ID_LEN 1
#define MAC_CHBA_ATTR_HDR_LEN 2
#define MAC_CHBA_MASTER_ELECTION_ATTR_LEN 20 /* ��������ͷ */
#define MAC_CHBA_CHAN_SWITCH_ATTR_LEN 7 /* ��������ͷ */
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
#define MAC_CHBA_LINK_INFO_LINK_CNT_POS 5 /* LINK INFO�ֶ���link cntƫ�� */
#define MAC_CHBA_POWER_SAVE_BITMAP_FLAG_POS 2 /* power save bitmap�ֶ���req/rsp flag��ƫ�� */
#define MAC_CHBA_POWER_SAVE_BITMAP_PS_BITMAP 3  /* power save bitmap�ֶ��е͹���bitmap��ƫ�� */
#define MAC_CHBA_POWER_SAVE_BITMAP_IE_LEN 5 /* chba power_save_bitmap ie��Ϣ�ֶγ��� */
#define MAC_CHBA_DEVICE_CAPABILITY_IE_LEN 10 /* chba device capability ie��Ϣ�ֶγ��� */
#define MAC_CHBA_DEVICE_CAPABILITY_RES_LEN 8 /* chba device capability ieԤ����Ϣ�ֶγ��� */
#define MAC_CHBA_SCAN_NOTIFY_BITMAP_LEN 4 /* ɨ��֪ͨ����bitmap�ֶδ�С */
#define MAC_CHBA_SCAN_NOTIFY_OFF_SLOT_CNT_LEN 1 /* ɨ��֪ͨ���Գ��������ֶδ�С */
#define MAC_CHBA_ISLAND_OWNER_CNT_POS 2
#define MAC_CHBA_MASTER_ELECTION_SYNC_POS 2
#define MAC_CHBA_SYNC_REQ_TYPE_POS 1
#define MAC_CHBA_DEFALUT_PERIODS_MS 512 /* CHBAĬ�ϵ�������ʱ��(ms) */

/* ���� */
#define MAC_CHBA_COEX_HARDWARE_CAPABILITY 0x24
#define MAC_CHBA_CHANNEL_NUM_5G 73 /* 5G��ȫ�����ŵ� */
#define CHBA_PS_ALL_AWAKE_BITMAP 0xFFFFFFFF /* �͹���bitmap:ȫ�� */
#define CHBA_PS_HALF_AWAKE_BITMAP 0x33333333 /* �͹���bitmap:���� */
#define CHBA_PS_KEEP_ALIVE_BITMAP 0x00010001 /* �͹���bitmap:���� */

#define CHBA_INVALID_MAX_CHANNEL_SWITCH_TIME 0 /* �����õ�max_channel_switch_time���� */
#define WLAN_CHBA_USERS_DEEPSLEEP_AGING_TIME (360 * 1000) /* CHBA ������״̬�ϻ�ʱ�� 360s */
#define WLAN_CHBA_USERS_NORMAL_AGING_TIME (5 * 1000) /* CHBA �ǳ�����״̬�ϻ�ʱ�� 5s */
#define WLAN_CHBA_USERS_NORMAL_SEND_NULL_TIME (1 * 1000) /* CHBA �ǳ�����״̬����null֡����ʱ�� 1s */
#define WLAN_CHBA_KEEPALIVE_TRIGGER_TIME (1 * 1000) /* keepalive��ʱ���������� */
#define CHBA_INIT_SOCIAL_CHANNEL 36 /* chbaģ���ʼ��social channel��36�ŵ� */
#define CHBA_DFS_CHANNEL_SUPPORT_STATE 0 /* chbaģ���ʼ����֧��dfs�ŵ� */
/* Hmac֪ͨ��CHBAģ�����Ϣ����ö�� */
// cfgvendor�����Ի�ʹ�ã��ݲ�ɾ��
typedef enum {
    /* ������Ϣ��HMAC --> CHBAģ�� */
    MAC_CHBA_MSG_START = 5,
    MAC_CHBA_VAP_START_RPT = MAC_CHBA_MSG_START,          /* vapʹ����Ϣ�ϱ� */
    MAC_CHBA_VAP_STOP_RPT = 6,                         /* vapȥʹ����Ϣ�ϱ� */
    MAC_CHBA_CONN_ADD_RPT = 7,                         /* ������Ϣ�ϱ� */
    MAC_CHBA_DELAY_OVER_TH_RPT = 16,                   /* ʱ�ӳ���ֵ�ϱ� */
    MAC_CHBA_STAT_INFO_RPT = 17,                       /* ͳ����Ϣ�ϱ� */
    MAC_CHBA_COEX_VAP_SUPPORTED_CHAN_LIST_RPT = 19,    /* �����vap��֧���ŵ��б��� */
    MAC_CHBA_SUPPORTED_CHAN_LIST_RPT = 20,             /* ���豸֧�ֵ��ŵ��б� */
    MAC_CHBA_COEX_INFO_RPT = 21,                       /* ���豸�Ĺ�����Ϣ���� */
    MAC_CHBA_STUB_CHAN_SWITCH_RPT = 22,                /* ��׮�ŵ��л��ϱ� */

    /* ������Ϣ��Service --> HMAC --> CHBAģ�� */
    MAC_CHBA_MODULE_INIT = 24,                         /* ģ���ʼ�� */
    MAC_CHBA_BATTERY_LEVEL_UPDATE = 25,                /* �������� */
    MAC_CHBA_CHAN_ADJUST = 30,                         /* CHBA�ŵ����� */
    MAC_CHBA_CHAN_ADJUST_NOTIFY = 31,                  /* CHBA�ŵ�����֪ͨ */
    MAC_CHBA_GET_COEX_INFO = 32,                       /* ��ȡ������Ϣ */

    MAC_CHBA_MSG_END,
} mac_chba_msg_notify_type_enum;

/* CHBAģ�����õ�Hmac���������� */
// cfgvendor�����Ի�ʹ�ã��ݲ�ɾ��
typedef enum {
    MAC_CHBA_SET_INIT_PARA = 0,                        /* CHBA��ʼ�������·� */
    MAC_CHBA_SET_ISLAND_INFO,                          /* CHBA���豸��������Ϣ�·� */
    MAC_CHBA_SET_SCAN_PARA,                            /* CHBAɨ������·� */
    MAC_CHBA_SET_COEX_VAP_SUPPORTED_CHAN_LIST,         /* ����vap�Ŀ����ŵ��б��·� */
    MAC_CHBA_SET_MGMT_FRAME,                           /* ����֡�·� */
    MAC_CHBA_SET_SYNC_INFO,                            /* �·�ͬ��״̬ */
    MAC_CHBA_SET_WORK_CHAN,                            /* �����ŵ��л� */
    MAC_CHBA_SET_VAP_BITMAP,                           /* �·�VAP�͹���bitmap */
    MAC_CHBA_SET_USER_BITMAP,                          /* �·�USER�͹���bitmap */
    MAC_CHBA_SET_LONG_SLEEP,                           /* �·���������Ϣ */
    MAC_CHBA_SET_SYNC_REQUEST_PARAM,                   /* �·�ͬ��������� */
    MAC_CHBA_SET_DOMAIN_REQUEST_PARAM,                 /* �·���ϲ����� */
    MAC_CHBA_SET_STAT_INFO_QUERY,                      /* �·�ͳ����Ϣ��ѯ */
} mac_chba_cmd_msg_type_enum;

/* ����֡Category�ֶ�ö�� */
typedef enum {
    MAC_CHBA_PNF = 0,
    MAC_CHBA_CHANNEL_SWITCH_NOTIFICATION = 5,
    MAC_CHBA_BITMAP_NOTIFICATION = 6,
    MAC_CHBA_SYNC_REQUEST = 7,
    MAC_CHBA_DOMAIN_MERGE = 8,

    MAC_CHBA_SYNC_BEACON,
    MAC_CHBA_ACTION_CATE_BUTT,
} mac_chba_action_category_enum;

/* Attribute��������ö�� */
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

/* �͹������action֡���� */
typedef enum {
    MAC_CHBA_BITMAP_UPDATE_REQ = 0x00, /* �������bitmap */
    MAC_CHBA_BITMAP_UPDATE_RSP_AGREE = 0x01, /* ͬ�����bitmap */
    MAC_CHBA_BITMAP_UPDATE_RSP_REJECT = 0x02, /* �ܾ�����bitmap */

    MAC_CHBA_BITMAP_TYPE_BUTT,
} mac_chba_bitmap_frame_subtype_enum;

/* chba vap modeö�� */
typedef enum {
    CHBA_LEGACY_MODE = 0, /* ��chbaģʽ */
    CHBA_MODE = 1, /* chba?? */

    CHBA_VAP_MODE_BUTT,
} chba_vap_mode_enum;

/* chba vap��״̬ö�� */
typedef enum {
    CHBA_INIT = 1,
    CHBA_NON_SYNC = 2,
    CHBA_DISCOVERY = 3,
    CHBA_WORK = 4,

    CHBA_STATE_BUTT,
} chba_vap_state_enum;


/* chba��ɫö�� */
typedef enum {
    CHBA_MASTER = 1, /* Master���ڵ��ĵ�������Master */
    CHBA_ISLAND_OWNER = 2,
    CHBA_OTHER_DEVICE = 3,

    CHBA_ROLE_BUTT,
} chba_role_enum;

/* d2h chbaͬ���¼� */
typedef enum {
    CHBA_UPDATE_TOPO_INFO = 0, /* ��������topo��Ϣ */
    CHBA_UPDATE_TOPO_BITMAP_INFO = 1, /* ����topo�е�bitmap��Ϣ */
    CHBA_RP_CHANGE_START_MASTER_ELECTION = 2, /* max RP�仯�������豸ѡ�� */
    CHBA_NON_SYNC_START_MASTER_ELECTION = 3, /* ����host�����豸ѡ�� */
    CHBA_NON_SYNC_START_SYNC_REQ = 4, /* ����host��������master����sync beacon */
    CHBA_DOMAIN_SEQARATE_START_MASTER_ELECTION = 5, /* ���ִ������豸ѡ�� */
    CHBA_SYNC_BUTT
} chba_sync_status_enum;

/* ���豸ѡ�����������Ƶ�sync state״̬ö�� */
typedef enum {
    MAC_CHBA_DECLARE_NON_SYNC = 0,
    MAC_CHBA_DECLARE_MASTER = 1,
    MAC_CHBA_DECLARE_SLAVE = 2,
} mac_chba_declare_state_enum;

/* ͬ������ö�� */
typedef enum {
    MAC_CHBA_SYNC_WITH_REQUEST = 1,
    MAC_CHBA_SYNC_ON_SOCIAL_CHANNEL = 2,
} mac_chba_sync_type_enum;

/* ͬ��������bitmap�� */
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

/* bitmap���Ϳ���ö�� */
typedef enum {
    CHBA_BITMAP_AUTO = 0,
    CHBA_BITMAP_FIX = 1,
    CHBA_BITMAP_CLOSE_SCAN = 2,
    CHBA_BITMAP_SWITCH_BUTT
} chba_bitmap_switch_enum;

typedef enum {
    CHBA_DEBUG_NO_LOG = 0, /* ����ӡά����Ϣ */
    CHBA_DEBUG_STAT_INFO_LOG = 1, /* ��ӡCHBA���ͳ����Ϣ */
    CHBA_DEBUG_ALL_LOG = 2, /* ��ӡȫ��CHBA��Ϣ */

    CHBA_DEBUG_BUTT
} chba_debug_log_type_enum; /* CHBAά����־��ӡ����ö�� */

/* �豸��Ϣ��������ö�� */
typedef enum {
    CHBA_DEVICE_ID_ADD = 0,
    CHBA_DEVICE_ID_DEL = 1,
    CHBA_DEVICE_ID_CLEAR = 2,
} chba_device_id_update_enum;

typedef enum {
    CHBA_CHANNEL_CFG_BITMAP_DISCOVER = BIT0, /* discover�ŵ����� */
    CHBA_CHANNEL_CFG_BITMAP_WORK = BIT1, /* work�ŵ����� */

    CHBA_CHANNEL_CFG_BITMAP_BUTT
} chba_channel_cfg_bitmap_enum; /* chba�ŵ�����bitmapö�� */
// �ϲ��·����ù����ŵ���Ϣ��������
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
} chba_bitmap_level_enum; /* CHBA bitmap��λö�� */
/* Ranking Priority��Ϣ�ṹ�� */
typedef struct {
    union {
        uint8_t value;
        struct {
            uint8_t hardware_capability : 2; /* Ӳ����񣬱��絥˫оƬ */
            uint8_t remain_power : 2; /* ʣ����� */
            uint8_t device_type : 4; /* �豸����: ����PC��Pad�� */
        } dl;
    } device_level;
    uint8_t chba_version; /* Э��汾 */
    uint8_t link_cnt; /* ��·���� */
} ranking_priority;

/* d2h chbaͬ���ṹ�� */
typedef struct {
    uint8_t mac_addr[OAL_MAC_ADDR_LEN];
    uint8_t resv[2];
    uint32_t device_bitmap;
} chba_d2h_device_bitmap_info_stru;

/* CHBA����������ͬ������ṹ�� */
typedef struct {
    uint8_t need_sync_flag; /* �Ƿ���Ҫ����ͬ�� */
    uint8_t peer_sync_state; /* �Զ˵�ͬ��״̬ */
    uint8_t peer_master_addr[WLAN_MAC_ADDR_LEN]; /* �Զ�Master��mac��ַ */
    uint8_t peer_work_channel; /* �Զ˵Ĺ����ŵ� */
    ranking_priority peer_rp; /* �Զ˵�RPֵ */
    ranking_priority peer_master_rp; /* �Զ�Master��RPֵ */
} chba_req_sync_after_assoc_stru;

/* Master��Ϣ�ṹ�� */
typedef struct {
    uint8_t master_work_channel; /* ����master�����ŵ�����20M�ŵ��� */
    uint8_t master_addr[WLAN_MAC_ADDR_LEN]; /* master��mac��ַ��Ϣ */
    ranking_priority master_rp; /* ����Master��RPֵ��Ϣ */
} master_info;

/* ���õ�DMAC�Ĳ��� */
typedef struct {
    uint32_t info_bitmap; /* ÿ��bit����һ����Ϣ�������bit��1����ʾ��Ӧ��Ϣ�и��£������������ */

    /* ���²���add vapʱ�·��������ϲ��� */
    uint8_t schedule_period; /* �������ڰ�����ʱ϶���� */
    uint8_t island_beacon_slot; /* �����㲥beacon֡��ʱ϶ */
    uint8_t resv1[2];
    uint32_t discover_bitmap; /* ����discover bitmap */

    /* ���²����ڽ�ɫ��״̬���ʱ�·� */
    uint8_t chba_state; /* vapͬ��״̬(chba_vap_state_enum) */
    uint8_t chba_role; /* �Ƿ��ǵ���(chba_role_enum) */
    uint8_t domain_bssid[WLAN_MAC_ADDR_LEN]; /* ��BSSID */

    /* ���²���ÿ�����������·� */
    uint8_t sync_beacon[MAC_CHBA_MGMT_MAX_PAYLOAD_BYTES];
    uint16_t sync_beacon_len;
    uint8_t resv2[2];
    uint8_t pnf[MAC_CHBA_MGMT_MID_PAYLOAD_BYTES];
    uint16_t pnf_len;
    uint8_t resv3[2];
    master_info master_info; /* master��Ϣ */
    uint8_t resv4;
} chba_params_config_stru;

typedef struct {
    uint8_t chan_cfg_type_bitmap; /* �ŵ���������(bit0��1��ʾ����discover slot�ŵ�, bit1��1��ʾ����work slot�ŵ�) */
    uint8_t chan_switch_slot_idx; /* ָ����ĳ��slot�ٸ���bitmap ��չ��, ��δ���� */
    uint8_t resv[2];
    mac_channel_stru channel; /* ��Ҫ���µ��ŵ� */
} chba_h2d_channel_cfg_stru; /* hmac->dmac �����ŵ��Ľṹ�� */

typedef struct {
    uint16_t user_idx; /* ָ��user��user_idx */
    uint8_t bitmap_level; /* �͹��ĵȼ� ȡchba_ps_level_enumö��ֵ */
    uint8_t resv;
} chba_user_ps_cfg_stru; /* chba user�͹������ýṹ�� */

/* ��DMAC����user�������Ľṹ�� */
typedef struct {
    uint16_t user_idx;
    uint32_t info_bitmap; /* ÿ��bit����һ����Ϣ�������bit��1����ʾ��Ӧ��Ϣ�и��£������������ */
    uint16_t delay_th; /* ��bit0���ƣ�CHBA VAP�Ŀտ�ʱ�����ޣ���λms */
    uint8_t window_size; /* ��bit0���ƣ�ʱ��ͳ�ƴ��������ٸ�����������8�� */
    uint16_t protected_packets_cnt; /* ��bit0���ƣ����ϱ�1�θ��ź��ӳٸ�cnt�����ٽ��м�� */
    uint16_t buffer_threshold; /* ��bit1���ƣ�buffer���� */
    uint8_t overbuffer_notify_flag;
} chba_h2d_cfg_user_param_stru;

/* ��DMAC��������������Ϣ */
typedef struct {
    uint32_t network_topo[MAC_CHBA_MAX_DEVICE_NUM][MAC_CHBA_MAX_BITMAP_WORD_NUM];
} chba_h2d_topo_info_stru;

/* ��DMAC�������ӻ�ɾ��ĳ��device id */
typedef struct {
    uint8_t update_type; /* ����: 0��ʾ���ӣ�1��ʾɾ��, 2��ʾ������� */
    uint8_t device_id;
    uint8_t mac_addr[WLAN_MAC_ADDR_LEN]; /* ��device id��Ӧ���豸MAC��ַ */
} chba_h2d_device_update_stru;

typedef struct {
    uint8_t device_id;
    uint8_t mac_addr[WLAN_MAC_ADDR_LEN]; /* ��device id��Ӧ���豸MAC��ַ */
} chba_device_id_stru;

typedef struct {
    uint8_t mac_addr[WLAN_MAC_ADDR_LEN];
} chba_lost_device_info_stru;
typedef struct {
    uint8_t csa_lost_device_num;   /* ��Ҫ���ȵ�δ���ŵ����豸������ */
    mac_channel_stru old_channel;  /* ����δ���ŵ����豸����¼ԭ�ŵ���Ϣ */
    chba_lost_device_info_stru csa_lost_device[MAC_CHBA_MAX_ISLAND_DEVICE_NUM]; /* ��Ҫ���ȵ��豸��Ϣ */
} chba_h2d_lost_device_info_stru;

/* dmac�ϱ���hmac����Ϣ */
/* DMAC�ϱ���⵽�����¼��Ľṹ�� */
typedef struct {
    uint16_t user_idx;
    uint16_t delay; /* ��λ��ms */
} chba_d2h_delay_exceed_stru;

/* DMAC�ϱ�user����mpdu��������ֵ��Ϣ */
typedef struct {
    uint16_t user_idx;
    uint16_t all_mpdu_num; /* ��ǰuser��mpdu���� */
} chba_d2h_user_buffer_over_threshlod_report_stru;

/* chbaģ�鷢�͵�Hmac������ṹ�� */
/* �·�CHBA��ʼ�������ṹ�� */
typedef struct {
    uint8_t chba_version;                          /* ֧�ֵ�CHBA�汾 */
    uint8_t slot_duration;                          /* CHBA slot���ȣ�msΪ��λ */
    uint8_t schdule_period;                         /* CHBA�������ڣ�slotΪ��λ */
    uint8_t discovery_slot;                         /* CHBA social�����ȣ�slotΪ��λ */
    uint8_t init_listen_period;                     /* ��ʼ���������ʱ϶������slotΪ��λ */
    uint16_t vap_alive_timeout;                     /* vap��������·״̬���ʱ϶������slotΪ��λ */
    uint16_t link_alive_timeout;                    /* ��·������ʱ϶������slotΪ��λ */
} mac_chba_config_para;
typedef struct {
    uint8_t master_election_attr[MAC_CHBA_MASTER_ELECTION_ATTR_LEN]; /* ���豸ѡ���ֶ� */
    mac_chba_config_para config_para;                  /* CHBA�������ò��� */
} mac_chba_set_init_para;

/* �·�����֡��Ϣ�ṹ�� */
typedef struct {
    uint8_t mgmt_frame_type;                          /* ����֡���� */
    uint8_t immediate_flag;                           /* �Ƿ��������� */
    uint8_t peer_mac_addr[WLAN_MAC_ADDR_LEN];         /* ����֡Ŀ�귽��MAC��ַ */
    uint8_t bssid[WLAN_MAC_ADDR_LEN];                 /* ����֡��BSSID */
    /* ����֡��payload����дʱbeacon��vendor IE��ʼ��action֡��action header��ʼ */
    uint8_t buf[MAC_CHBA_MGMT_MAX_PAYLOAD_BYTES];
    uint16_t buf_size;                                /* ����֡��payload���� */
} mac_chba_send_mgmt_frame;

/* �·��ŵ��л���Ϣ�ṹ�� */
typedef struct {
    uint8_t switch_slot;                             /* �л�slot */
    mac_channel_stru channel;                        /* Ŀ���ŵ���Ϣ */
} mac_chba_set_work_chan;

/* �·�ͬ������Ĳ��� */
typedef struct {
    uint8_t chan_num; /* �����ŵ�����20M�ŵ��� */
    uint8_t peer_addr[WLAN_MAC_ADDR_LEN];
    uint8_t master_addr[WLAN_MAC_ADDR_LEN]; /* ���豸��Ӧ��master��ַ������������BSSID */
} sync_peer_info;
typedef struct {
    uint8_t sync_type; /* ͬ���������� */
    sync_peer_info request_peer; /* ��д����ͬ�����豸��Ϣ */
    uint8_t mgmt_type; /* sync request��PNF֡ */
    uint8_t mgmt_buf[MAC_CHBA_MGMT_SHORT_PAYLOAD_BYTES]; /* ���͹���֡ */
    uint16_t mgmt_buf_len; /* ����֡payload���� */
} mac_chba_set_sync_request;

/* �·�ͬ��״̬��Ϣ */
typedef struct {
    uint8_t sync_state;
    uint8_t role;
    uint8_t master_addr[WLAN_MAC_ADDR_LEN];
    uint8_t mgmt_type; /* Beacon��PNF֡ */
    uint8_t mgmt_buf[MAC_CHBA_MGMT_MAX_PAYLOAD_BYTES]; /* ���͹���֡ */
    uint16_t mgmt_buf_len; /* ����֡payload���� */
} mac_chba_set_sync_status;
/* �·�ɨ������ṹ�� */
typedef struct {
    mac_channel_stru work_scan_chan;
    mac_channel_stru nonwork_scan_chan;
    uint32_t work_chan_scan_bitmap;
    uint32_t nonwork_chan_scan_bitmap;
} mac_chba_set_chan_scan_param;

/* Hmac֪ͨ�¼���CHBAģ�����Ϣ�ṹ�� */
/* CHBA vap start֪ͨ�ṹ�� */
typedef struct {
    uint8_t mac_addr[WLAN_MAC_ADDR_LEN];            /* ���豸MAC��ַ */
    mac_channel_stru social_channel;
    mac_channel_stru work_channel;
} mac_chba_vap_start_rpt;

/* ����/ɾ����Ϣ�ϱ��ṹ�� */
typedef struct {
    uint8_t self_mac_addr[WLAN_MAC_ADDR_LEN];       /* ���豸MAC��ַ */
    uint8_t peer_mac_addr[WLAN_MAC_ADDR_LEN];       /* �����ӵĶԶ�MAC��ַ */
    mac_channel_stru work_channel;
} mac_chba_conn_info_rpt;

/* ��׮�ŵ��л��ϱ��ṹ�� */
typedef struct {
    mac_channel_stru target_channel;
} mac_chba_stub_chan_switch_rpt;

/* �ŵ�����������֪ͨ�ṹ�� */
typedef struct {
    hmac_info_report_type_enum report_type;
    uint8_t chan_number;
    uint8_t bandwidth;
    uint8_t switch_type;
    uint8_t status_code;
} mac_chba_adjust_chan_info;

/* ÿ��device����Ϣ�ṹ�� */
typedef struct {
    uint8_t mac_addr[WLAN_MAC_ADDR_LEN];                     /* ���豸��mac��ַ */
    uint8_t chan_switch_ack;                        /* �Ƿ��յ����ŵ��л�ָʾ֡��confirm */
    uint32_t ps_bitmap;                             /* ���豸�ĵ͹���bitmap */
    uint8_t is_conn_flag; /* ������豸��ýڵ������ӣ����flag��Ϊ1 */
} mac_chba_per_device_info_stru;

/* CHBA���豸������Ϣ�Ľṹ�� */
typedef struct {
    uint8_t self_device_id;                         /* ���豸��device id */
    uint8_t role;                                   /* ���豸�Ľ�ɫ��master�������������豸 */
    uint8_t island_owner_valid;                     /* ���豸�Ƿ��ѻ�ȡ���� */
    uint8_t island_owner_addr[WLAN_MAC_ADDR_LEN];            /* ���豸�����ĵ�����mac��ַ */
    uint8_t island_device_cnt;                      /* ���豸���ڵ����豸�� */
    mac_chba_per_device_info_stru island_device_list[MAC_CHBA_MAX_ISLAND_DEVICE_NUM]; /* ���豸���ڵ����豸��Ϣ�б� */
} mac_chba_self_conn_info_stru;

/* ��Ϣ�ṹ�� */
typedef struct {
    uint8_t work_channel; /* ���浱ǰ�豸�����ŵ�����20M�ŵ��� */
    uint8_t master_addr[WLAN_MAC_ADDR_LEN]; /* master��mac��ַ��Ϣ */
    ranking_priority master_rp; /* ����Master��RPֵ��Ϣ */
    ranking_priority own_rp; /* ���������RPֵ��Ϣ */
} device_info;

/* CHBA��ϲ�ͬ���ṹ�� */
typedef struct {
    uint8_t sa_addr[WLAN_MAC_ADDR_LEN];
    master_info  another_master;
} mac_chba_multiple_domain_stru;

typedef struct {
    uint8_t dev_mac_addr[WLAN_MAC_ADDR_LEN];
    uint8_t coex_chan_cnt;
    uint8_t coex_chan_lists[MAX_CHANNEL_NUM_FREQ_5G];
} mac_chba_rpt_coex_info;

/* �ϲ��·�����vap�Ŀ�֧���ŵ��б� */
typedef struct {
    uint8_t cfg_cmd_type; // ��ʶ��ǰ�����ŵ��������ڱ��豸���Ǳ���
    uint8_t supported_channel_cnt; /* ֧�ֵ��ŵ����� */
    uint8_t supported_channel_list[MAX_CHANNEL_NUM_FREQ_5G]; /* ֧�ֵ��ŵ����б� */
} mac_chba_set_coex_chan_info;

#define chba_bitmap_to_level(_bitmap) (       \
        ((_bitmap) == CHBA_PS_ALL_AWAKE_BITMAP) ? CHBA_BITMAP_ALL_AWAKE_LEVEL : \
        ((_bitmap) == CHBA_PS_HALF_AWAKE_BITMAP) ? CHBA_BITMAP_HALF_AWAKE_LEVEL : \
        ((_bitmap) == CHBA_PS_KEEP_ALIVE_BITMAP) ? CHBA_BITMAP_KEEP_ALIVE_LEVEL : CHBA_BITMAP_LEVEL_BUTT)

#define chba_level_to_bitmap(_level) (       \
        ((_level) == CHBA_BITMAP_ALL_AWAKE_LEVEL) ? CHBA_PS_ALL_AWAKE_BITMAP : \
        ((_level) == CHBA_BITMAP_HALF_AWAKE_LEVEL) ? CHBA_PS_HALF_AWAKE_BITMAP : \
        ((_level) == CHBA_BITMAP_KEEP_ALIVE_LEVEL) ? CHBA_PS_KEEP_ALIVE_BITMAP : 0)

/* CHBA FRAME��ؽṹ�� */
#if (defined(_PRE_PRODUCT_ID_HI110X_DEV))
/* 02 dev����#pragma pack(1)/#pragma pack()��ʽ�ﵽһ�ֽڶ��� */
#pragma pack(1)
#endif
/* linkinfo��Ϣ */
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
/* 02 dev����#pragma pack(1)/#pragma pack()��ʽ�ﵽһ�ֽڶ��� */
#pragma pack()
#endif

/* CHBA �ŵ����� bitmap �궨�� */
#define CHBA_CHANNEL_SEQ_BITMAP_25_PERCENT (uint32_t)0x03030303
#define CHBA_CHANNEL_SEQ_BITMAP_50_PERCENT (uint32_t)0x33333333
#define CHBA_CHANNEL_SEQ_BITMAP_75_PERCENT (uint32_t)0x3F3F3F3F
#define CHBA_CHANNEL_SEQ_BITMAP_100_PERCENT (uint32_t)0xFFFFFFFF

/* CHBA �ŵ����� bitmap��λö�� */
enum chba_dbac_level {
    CHBA_CHANNEL_SEQ_LEVEL0 = 0,
    CHBA_CHANNEL_SEQ_LEVEL1 = 1,
    CHBA_CHANNEL_SEQ_LEVEL2 = 2,

    CHBA_CHANNEL_SEQ_LEVEL_BUTT
};

/* CHBA �ŵ��������ò��� */
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

