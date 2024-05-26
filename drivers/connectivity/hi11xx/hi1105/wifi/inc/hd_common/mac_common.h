

#ifndef __MAC_COMMON_H__
#define __MAC_COMMON_H__

/* 1 ����ͷ�ļ����� */
#include "oal_types.h"
#include "wlan_types.h"
#include "hal_common.h"
#include "mac_device_common.h"
#include "mac_user_common.h"
#include "mac_vap_common.h"
#include "mac_cb_tx.h"
#include "mac_cb_rx.h"
// �˴�����extern "C" UT���벻��
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define IS_RW_RING_FULL(read, write)                                    \
    (((read)->st_rw_ptr.bit_rw_ptr == (write)->st_rw_ptr.bit_rw_ptr) && \
        ((read)->st_rw_ptr.bit_wrap_flag != (write)->st_rw_ptr.bit_wrap_flag))

/* ����dmac����Ϣͷ�ĳ��� */
#define CUSTOM_MSG_DATA_HDR_LEN (sizeof(custom_cfgid_enum_uint32) + sizeof(uint32_t))

#ifdef _PRE_WLAN_FEATURE_NRCOEX
#define DMAC_WLAN_NRCOEX_INTERFERE_RULE_NUM (4) /* 5gnr������Ų�����Ŀǰ��4�� */
#endif

#define MAC_USER_IS_HE_USER(_user)          (MAC_USER_HE_HDL_STRU(_user)->en_he_capable)
#define MAC_USER_ARP_PROBE_CLOSE_HTC(_user) ((_user)->bit_arp_probe_close_htc)

#define MAC_USER_TX_DATA_INCLUDE_HTC(_user) ((_user)->bit_tx_data_include_htc)
#define MAC_USER_TX_DATA_INCLUDE_OM(_user)  ((_user)->bit_tx_data_include_om)

#define MAC_USER_NOT_SUPPORT_HTC(_user)  \
    (((_user)->st_cap_info.bit_qos == 0) || (MAC_USER_TX_DATA_INCLUDE_HTC(_user) == 0))
/* 3 ö�ٶ��� */
/*****************************************************************************
  ö����  : dmac_tx_host_drx_subtype_enum
  Э����:
  ö��˵��: HOST DRX�¼������Ͷ���
*****************************************************************************/
/* WLAN_CRX�����Ͷ��� */
typedef enum {
    DMAC_TX_HOST_DRX = 0,
    HMAC_TX_HOST_DRX = 1,
    HMAC_TX_DMAC_SCH = 2, /* �������¼� */
    DMAC_TX_HOST_DTX = 3, /* H2D����netbuf, device ring txʹ�� */

    DMAC_TX_HOST_DRX_BUTT
} dmac_tx_host_drx_subtype_enum;

/*****************************************************************************
  ö����  : dmac_tx_wlan_dtx_subtype_enum
  Э����:
  ö��˵��: WLAN DTX�¼������Ͷ���
*****************************************************************************/
typedef enum {
    DMAC_TX_WLAN_DTX = 0,

    DMAC_TX_WLAN_DTX_BUTT
} dmac_tx_wlan_dtx_subtype_enum;

/*****************************************************************************
  ö����  : dmac_wlan_ctx_event_sub_type_enum
  Э����:
  ö��˵��: WLAN CTX�¼������Ͷ���
*****************************************************************************/
typedef enum {
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_MGMT = 0, /* ����֡���� */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_ADD_USER = 1,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_NOTIFY_ALG_ADD_USER = 2,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_DEL_USER = 3,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_BA_SYNC = 4,  /* �յ�wlan��Delba��addba rsp���ڵ�dmac��ͬ�� */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_PRIV_REQ = 5, /* 11N�Զ����������¼����� */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_SCAN_REQ = 6,       /* ɨ������ */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_SCHED_SCAN_REQ = 7, /* PNO����ɨ������ */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_RESET_PSM = 8,      /* �յ���֤���� �������󣬸�λ�û��Ľ���״̬ */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_JOIN_SET_REG = 9,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_JOIN_DTIM_TSF_REG = 10,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_CONN_RESULT = 11, /* ������� */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_ASOC_WRITE_REG = 12, /* AP�ദ�����ʱ���޸�SEQNUM_DUPDET_CTRL�Ĵ��� */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_STA_SET_EDCA_REG = 13,       /* STA�յ�beacon��assoc rspʱ������EDCA�Ĵ��� */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_SWITCH_TO_NEW_CHAN = 14, /* �л������ŵ��¼� */
    DMAC_WALN_CTX_EVENT_SUB_TYPR_SELECT_CHAN = 15,        /* �����ŵ��¼� */
    DMAC_WALN_CTX_EVENT_SUB_TYPR_DISABLE_TX = 16,         /* ��ֹӲ������ */
    DMAC_WALN_CTX_EVENT_SUB_TYPR_ENABLE_TX = 17,          /* �ָ�Ӳ������ */
    DMAC_WLAN_CTX_EVENT_SUB_TYPR_RESTART_NETWORK = 18,    /* �л��ŵ��󣬻ָ�BSS������ */
    DMAC_WLAN_CTX_EVENT_SUB_TYPR_SWITCH_TO_OFF_CHAN = 19,  /* �л���offchan��off-chan cac��� */
    DMAC_WLAN_CTX_EVENT_SUB_TYPR_SWITCH_TO_HOME_CHAN = 20, /* �л�home chan */
    DMAC_WLAN_CTX_EVENT_SUB_TYPR_DFS_TEST = 21,
    DMAC_WALN_CTX_EVENT_SUB_TYPR_DFS_CAC_CTRL_TX = 22, /* DFS 1min CAC��vap״̬λ��Ϊpause����up,ͬʱ��ֹ���߿���Ӳ������ */
    DMAC_WLAN_CTX_EVENT_SUB_TYPR_EDCA_OPT = 23, /* edca�Ż���ҵ��ʶ��֪ͨ�¼� */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_CALI_HMAC2DMAC = 24,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_DSCR_OPT = 25,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_CALI_MATRIX_HMAC2DMAC = 26,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_APP_IE_H2D = 27,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_IP_FILTER = 28,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_APF_CMD = 29,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_STA_SET_MU_EDCA_REG = 30, /* STA�յ�beacon��assoc rspʱ������MU EDCA�Ĵ��� */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_STA_SET_SPATIAL_REUSE_REG = 31,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_UPDATE_TWT = 32,                      /* STA�յ�twt ʱ�����¼Ĵ��� */
    /* STA�յ�beacon��assoc rspʱ������NDP Feedback report�Ĵ��� */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_STA_SET_NDP_FEEDBACK_REPORT_REG = 33,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_HIEX_H2D_MSG = 34,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_CUST_HMAC2DMAC = 35,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_CHBA_PARAMS = 36,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_ADD_MULTI_USER = 37,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_CHBA_SELF_CONN_INFO_SYNC = 38,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_CHBA_TOPO_INFO_SYNC = 39,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_BUTT
} dmac_wlan_ctx_event_sub_type_enum;

/* DMACģ�� WLAN_DRX�����Ͷ��� */
typedef enum {
    DMAC_WLAN_DRX_EVENT_SUB_TYPE_RX_DATA,        /* APģʽ: DMAC WLAN DRX ���� */
    DMAC_WLAN_DRX_EVENT_SUB_TYPE_TKIP_MIC_FAILE, /* DMAC tkip mic faile �ϱ���HMAC */
    DMAC_WLAN_DRX_EVENT_SUB_TYPE_WOW_RX_DATA,    /* WOW DMAC WLAN DRX ���� */

    DMAC_WLAN_DRX_EVENT_SUB_TYPE_BUTT
} dmac_wlan_drx_event_sub_type_enum;
typedef uint8_t dmac_wlan_drx_event_sub_type_enum_uint8;

/* DMACģ�� WLAN_CRX�����Ͷ��� */
typedef enum {
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_INIT,              /* DMAC �� HMAC�ĳ�ʼ������ */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_RX,                /* DMAC WLAN CRX ���� */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_DELBA,             /* DMAC���������DELBA֡ */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_EVERY_SCAN_RESULT, /* ɨ�赽һ��bss��Ϣ���ϱ���� */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_SCAN_COMP,         /* DMACɨ������ϱ���HMAC */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_OBSS_SCAN_COMP,    /* DMAC OBSSɨ������ϱ���HMAC */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_CHAN_RESULT,       /* �ϱ�һ���ŵ���ɨ���� */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_ACS_RESP,          /* DMAC ACS �ظ�Ӧ�ò�����ִ�н����HMAC */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_DISASSOC,          /* DMAC�ϱ�ȥ�����¼���HMAC, HMAC��ɾ���û� */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_DEAUTH,            /* DMAC�ϱ�ȥ��֤�¼���HMAC */

    DMAC_WLAN_CRX_EVENT_SUB_TYPR_CH_SWITCH_COMPLETE, /* �ŵ��л�����¼� */
    DMAC_WLAN_CRX_EVENT_SUB_TYPR_DBAC,               /* DBAC enable/disable�¼� */

    DMAC_WLAN_CRX_EVENT_SUB_TYPE_HIEX_D2H_MSG,
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_RX_WOW,
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_EVERY_SCAN_RESULT_RING, /* ring��ʽ���յ�ɨ���������ϱ� */

    DMAC_WLAN_CRX_EVENT_SUB_TYPE_TX_RING_ADDR,
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_VSP_INFO_ADDR,
    DMAC_WLAN_DRX_EVENT_SUB_TYPE_RX_SNIFFER_INFO,
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_PSD_RPT,           /* DMAC ��HMAC�ϱ�PSD��Ϣ */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_CHAN_DET_RPT,       /* DMAC ��HMAC�ϱ��ŵ������Ϣ */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_WIFI_DELAY_RPT,
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_BUTT
} dmac_wlan_crx_event_sub_type_enum;
typedef uint8_t dmac_wlan_crx_event_sub_type_enum_uint8;

/* ����HOST��������¼� */
typedef enum {
    DMAC_TO_HMAC_SYN_UP_REG_VAL = 1,
    DMAC_TO_HMAC_CREATE_BA = 2,
    DMAC_TO_HMAC_DEL_BA = 3,
    DMAC_TO_HMAC_SYN_CFG = 4,

    DMAC_TO_HMAC_ALG_INFO_SYN = 6,
    DMAC_TO_HMAC_VOICE_AGGR = 7,
    DMAC_TO_HMAC_SYN_UP_SAMPLE_DATA = 8,
    DMAC_TO_HMAC_M2S_DATA = 10,
    DMAC_TO_HMAC_BANDWIDTH_INFO_SYN = 11,  /* dmac��hmacͬ���������Ϣ */
    DMAC_TO_HMAC_PROTECTION_INFO_SYN = 12, /* dmac��hmacͬ������mib��Ϣ */
    DMAC_TO_HMAC_CH_STATUS_INFO_SYN = 13,  /* dmac��hmacͬ�������ŵ��б� */
    DMAC_TO_HMAC_FTM_CALI = 14,
    /* �¼�ע��ʱ����Ҫö��ֵ�г�������ֹ����device���host���Ժ�򿪲�һ�£�
       ��ɳ���ͬ���¼�δע�����⣬��������������ע���򿪵�һ����
    */
    DMAC_TO_HMAC_SYN_BUTT
} dmac_to_hmac_syn_type_enum;

typedef struct {
    uint32_t ftm_cali_time;
    uint8_t vap_id;
} dmac_to_hmac_ftm_ctx_event_stru; /* hd_event */

/* hmac to dmac���ƻ�����ͬ����Ϣ�ṹ */
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

    /* ˽�ж��� */
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
    CUSTOM_OPTIMIZE_11AX20M_NON_LDPC = 4, /* ��֤ʱap 20M ��֧��LDPC���Թ�����ax */
    CUSTOM_OPTIMIZE_CHBA = 5, /* CHBA�������� */
    CUSTOM_OPTIMIZE_CE_IDLE = 6, /* CE��֤���������ر�pk mode���������������� */
    CUSTOM_OPTIMIZE_CSA_EXT = 7, /* CSA��չ ��Ƶ�ι��� ���� */
    CUSTOM_OPTIMIZE_FEATURE_BUTT,
};

enum custom_close_filter_switch {
    CUSTOM_APF_CLOSE_SWITCH,
    CUSTOM_ARP_CLOSE_MULITCAST_FILTER,
    CUSTOM_CLOSE_FILTER_SWITCH_BUTT
};

typedef struct {
    custom_cfgid_enum_uint32 en_syn_id; /* ͬ������ID */
    uint32_t len;                  /* DATA payload���� */
    uint8_t auc_msg_body[NUM_4_BYTES];          /* DATA payload */
} hmac_to_dmac_cfg_custom_data_stru;

/* MISC��ɢ�¼� */
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

/* HMAC to DMACͬ������ */
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

/* BA�Ự��״̬ö�� */
typedef enum {
    DMAC_BA_INIT = 0,   /* BA�Ựδ���� */
    DMAC_BA_INPROGRESS, /* BA�Ự���������� */
    DMAC_BA_COMPLETE,   /* BA�Ự������� */
    DMAC_BA_HALTED,     /* BA�Ự������ͣ */
    DMAC_BA_FAILED,     /* BA�Ự����ʧ�� */

    DMAC_BA_BUTT
} dmac_ba_conn_status_enum;
typedef uint8_t dmac_ba_conn_status_enum_uint8;

/* ���������������ö�� */
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
    uint16_t us_baw_start;                          /* ��һ��δȷ�ϵ�MPDU�����к� */
    uint16_t us_last_seq_num;                       /* ���һ�����͵�MPDU�����к� */
    uint16_t us_baw_size;                           /* Block_Ack�Ự��buffer size��С */
    uint16_t us_baw_head;                           /* bitmap�м�¼�ĵ�һ��δȷ�ϵİ���λ�� */
    uint16_t us_baw_tail;                           /* bitmap����һ��δʹ�õ�λ�� */
    oal_bool_enum_uint8 en_is_ba;                     /* Session Valid Flag */
    dmac_ba_conn_status_enum_uint8 en_ba_conn_status; /* BA�Ự��״̬ */
    mac_back_variant_enum_uint8 en_back_var;          /* BA�Ự�ı��� */
    uint8_t uc_dialog_token;                        /* ADDBA����֡��dialog token */
    uint8_t uc_ba_policy;                           /* Immediate=1 Delayed=0 */
    oal_bool_enum_uint8 en_amsdu_supp;                /* BLOCK ACK֧��AMSDU�ı�ʶ */
    uint8_t *puc_dst_addr;                          /* BA�Ự���ն˵�ַ */
    uint16_t us_ba_timeout;                         /* BA�Ự������ʱʱ�� */
    uint8_t uc_ampdu_max_num;                       /* BA�Ự�£��ܹ��ۺϵ�����mpdu�ĸ��� */
    uint8_t uc_tx_ba_lut;                           /* BA�ỰLUT session index */
    uint16_t us_mac_user_idx;
#ifdef _PRE_WLAN_FEATURE_DFR
    uint16_t us_pre_baw_start;    /* ��¼ǰһ���ж�ba���Ƿ���ʱ��ssn */
    uint16_t us_pre_last_seq_num; /* ��¼ǰһ���ж�ba���Ƿ���ʱ��lsn */
    uint16_t us_ba_jamed_cnt;     /* BA������ͳ�ƴ��� */
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
    DATA_TYPE_ETH = 0,    /* Eth II��̫����������, ��vlan */
    DATA_TYPE_1_VLAN_ETH = 1,    /* Eth II��̫����������, ��1��vlan */
    DATA_TYPE_2_VLAN_ETH = 2,    /* Eth II��̫����������, ��2��vlan */
    DATA_TYPE_80211 = 3,         /* 802.11�������� */
    DATA_TYPE_0_VLAN_8023 = 4,   /* 802.3��̫����������, ��vlan */
    DATA_TYPE_1_VLAN_8023 = 5,   /* 802.3��̫����������, ��1��vlan */
    DATA_TYPE_2_VLAN_8023 = 6,   /* 802.3��̫����������, ��2��vlan */
    DATA_TYPE_MESH = 7,          /* Mesh��������, ��extend addresss */
    DATA_TYPE_MESH_EXT_ADDR = 8, /* Mesh��������, ��extend addresss */

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

/* Host Device���ò��� */
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
    uint8_t ring_index; /* �������ڴ�, device ring��tid�޷�����1:1����,
                         * host��h2d�л�ʱ, ��Ҫ��bitmap���ҵ���һ���ɷ����ring index,
                         * device����index��ring�ڴ���л�ȡ���õ�ring,
                         * d2h�л�ʱͬ��, host����Ҫ��ĳ����ռ�õ�ring index����Ϊ����
                         */
    uint8_t hal_dev_id;
    uint8_t tid_num; /* ��Ҫ�л���tid���� */
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
    uint8_t rsv[NUM_3_BYTES];  /* �����ֶ�:3�ֽ� */
    nrcoex_rule_data_union un_nrcoex_rule_data[DMAC_WLAN_NRCOEX_INTERFERE_RULE_NUM];
} nrcoex_cfg_info_stru;
#endif

typedef struct {
    uint8_t cfg_type;
    oal_bool_enum_uint8 need_w4_dev_return;
    uint8_t resv[NUM_2_BYTES]; /* 4 �ֽڶ��� */
    uint32_t dev_ret;
} mac_cfg_cali_hdr_stru;

#ifdef _PRE_WLAN_FEATURE_PHY_EVENT_INFO
typedef struct {
    uint8_t               event_rpt_en;
    uint8_t               rsv[NUM_3_BYTES]; /* 4 �ֽڶ��� */
    uint32_t              wp_mem_num;       /* �¼�ͳ�Ƹ��� */
    uint32_t              wp_event0_type_sel;  /* event0�¼�����ѡ����� */
    uint32_t              wp_event1_type_sel;  /* event1�¼�����ѡ����� */
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
