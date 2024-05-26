

#ifndef __HISI_CUSTOMIZE_WIFI_HI1103_H__
#define __HISI_CUSTOMIZE_WIFI_HI1103_H__

/* 其他头文件包含 */
#include "hisi_customize_wifi.h"
#include "wlan_cali.h"
#include "wlan_customize.h"
#include "mac_vap.h"

/* 宏定义 */
#define NVRAM_PARAMS_ARRAY "nvram_params"

#define STR_COUNTRYCODE_SELFSTUDY "countrycode_selfstudy"

#define NV_WLAN_NUM                 193
#define NV_WLAN_VALID_SIZE          12
#define ini_mac2str(a)                  (a)[0], "**", "**", "**", (a)[4], (a)[5]
#define MACFMT                      "%02x:%s:%s:%s:%02x:%02x"
#define ini_mac2str_all(a)              (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACFMT_ALL                  "%02x:%02x:%02x:%02x:%02x:%02x"
#define CUS_TAG_INI                 0x11
#define CUS_TAG_DTS                 0x12
#define CUS_TAG_NV                  0x13
#define CUS_TAG_PRIV_INI            0x14
#define CUS_TAG_PRO_LINE_INI        0x15
#define CALI_TXPWR_PA_DC_REF_MIN    1000
#define CALI_TXPWR_PA_DC_REF_MAX    0x8000
#define CALI_TXPWR_PA_DC_FRE_MIN    0
#define CALI_TXPWR_PA_DC_FRE_MAX    78
#define CALI_BT_TXPWR_PA_DC_REF_MAX 15000

#define PSD_THRESHOLD_MIN           (-15)
#define PSD_THRESHOLD_MAX           (-10)
#define LNA_GAIN_DB_MIN             (-40)
#define LNA_GAIN_DB_MAX             80

/* NVRAM中存储的参数值的总个数，包括4个基准功率 */
#define NUM_OF_NV_PARAMS (NUM_OF_NV_MAX_TXPOWER + NUM_OF_NV_DPD_MAX_TXPOWER + \
                          NUM_OF_NV_11B_DELTA_TXPOWER + NUM_OF_NV_5G_UPPER_UPC + 4)
#define TX_RATIO_MAX                    2000 /* tx占空比的最大有效值 */
#define TX_PWR_COMP_VAL_MAX             50   /* 发射功率补偿值的最大有效值 */
#define MORE_PWR_MAX                    50   /* 根据温度额外补偿的发射功率的最大有效值 */

#define MAX_COUNTRY_COUNT               300  /* 支持定制的国家的最大个数 */
#define DELTA_CCA_ED_HIGH_TH_RANGE      25
#define CUS_NUM_5G_BW                   4    /* 定制化5g带宽数 */


#define PRO_LINE_2G_TO_5G_OFFSET        2 /* 定制化读取的顺序和底层解析的顺序差异导致 */

/* 这种情况每个level占有3个定义，所以每次偏移3 */
#define WLAN_CFG_ID_OFFSET              3

#define CUS_MIN_OF_SAR_VAL    0x28 /* 定制化降SAR最小值 4dbm */


#define CUS_MAX_BASE_TXPOWER_DELT_VAL_MIN  (-100)       /* MIMO的最大基准发送功率差值的最小有效值 */
#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
#define CUS_MAX_OF_TAS_PWR_CTRL_VAL 40 /* TAS控制抬升功率最大值4db */
#endif
#define CUS_2G_LOW_POW_AMEND_ABS_VAL_MAX 30
#define CUS_RU_DELT_POW_MIN       (-20)
#define CUS_RU_DELT_POW_MAX       20

#define WLAN_CALI_TXPWR_REF_2G_CH1_VAL    6250
#define WLAN_CALI_TXPWR_REF_2G_CH2_VAL    5362
#define WLAN_CALI_TXPWR_REF_2G_CH3_VAL    4720
#define WLAN_CALI_TXPWR_REF_2G_CH4_VAL    4480
#define WLAN_CALI_TXPWR_REF_2G_CH5_VAL    4470
#define WLAN_CALI_TXPWR_REF_2G_CH6_VAL    4775
#define WLAN_CALI_TXPWR_REF_2G_CH7_VAL    5200
#define WLAN_CALI_TXPWR_REF_2G_CH8_VAL    5450
#define WLAN_CALI_TXPWR_REF_2G_CH9_VAL    5600
#define WLAN_CALI_TXPWR_REF_2G_CH10_VAL   6100
#define WLAN_CALI_TXPWR_REF_2G_CH11_VAL   6170
#define WLAN_CALI_TXPWR_REF_2G_CH12_VAL   6350
#define WLAN_CALI_TXPWR_REF_2G_CH13_VAL   6530
#define WLAN_CALI_TXPWR_REF_5G_BAND1_VAL  2500
#define WLAN_CALI_TXPWR_REF_5G_BAND2_VAL  2800
#define WLAN_CALI_TXPWR_REF_5G_BAND3_VAL  3100
#define WLAN_CALI_TXPWR_REF_5G_BAND4_VAL  3600
#define WLAN_CALI_TXPWR_REF_5G_BAND5_VAL  3600
#define WLAN_CALI_TXPWR_REF_5G_BAND6_VAL  3700
#define WLAN_CALI_TXPWR_REF_5G_BAND7_VAL  3800
#define WLAN_CALI_TONE_GRADE_AMP          1
#define WLAN_BT_PA_REF_BAND1              11000
#define WLAN_BT_PA_REF_BAND2              10000
#define WLAN_BT_PA_REF_BAND3              7000
#define WLAN_BT_PA_REF_BAND4              8000
#define WLAN_BT_PA_REF_BAND5              7000
#define WLAN_BT_PA_REF_BAND6              7000
#define WLAN_BT_PA_REF_BAND7              12000
#define WLAN_BT_PA_REF_BAND8              12000
#define WLAN_BT_PA_NUM                    7
#define WLAN_BT_PA_FRE1                   0
#define WLAN_BT_PA_FRE2                   10
#define WLAN_BT_PA_FRE3                   28
#define WLAN_BT_PA_FRE4                   45
#define WLAN_BT_PA_FRE5                   53
#define WLAN_BT_PA_FRE6                   63
#define WLAN_BT_PA_FRE7                   76
#define WLAN_BT_PA_FRE8                   78
#define WLAN_BT_AMP_GRADE                 2
#define WLAN_RF_LNA_BYPASS_GAIN_DB_2G     (-12)
#define WLAN_RF_LNA_GAIN_DB_2G            20
#define WLAN_RF_PA_GAIN_DB_B0_2G          (-12)
#define WLAN_EXT_SWITCH_ISEXIST_2G        0
#define WLAN_EXT_PA_ISEXIST_2G            0
#define WLAN_EXT_LNA_ISEXIST_2G           0
#define WLAN_LNA_ON2OFF_TIME_NS_2G        630
#define WLAN_LNA_OFF2ON_TIME_NS_2G        320
#define WLAN_RF_LNA_BYPASS_GAIN_DB_5G     (-12)
#define WLAN_RF_LNA_GAIN_DB_5G            20
#define WLAN_RF_PA_GAIN_DB_B0_5G          (-12)
#define WLAN_EXT_SWITCH_ISEXIST_5G        1
#define WLAN_EXT_PA_ISEXIST_5G            1
#define WLAN_EXT_LNA_ISEXIST_5G           1
#define WLAN_LNA_ON2OFF_TIME_NS_5G        630
#define WLAN_LNA_OFF2ON_TIME_NS_5G        320
#define WLAN_TX_RATIO_LEVEL_0             900
#define WLAN_TX_PWR_COMP_VAL_LEVEL_0      17
#define WLAN_TX_RATIO_LEVEL_1             650
#define WLAN_TX_PWR_COMP_VAL_LEVEL_1      13
#define WLAN_TX_RATIO_LEVEL_2             280
#define WLAN_TX_PWR_COMP_VAL_LEVEL_2      5
#define WLAN_MORE_PWR                     7
#define WLAN_STA_KEEPALIVE_CNT_TH         3
#define WLAN_RF_RX_INSERTION_LOSS_2G_BAND1  (-12)
#define WLAN_RF_RX_INSERTION_LOSS_2G_BAND2  (-12)
#define WLAN_RF_RX_INSERTION_LOSS_2G_BAND3  (-12)
#define WLAN_RF_RX_INSERTION_LOSS_5G_BAND1  (-8)
#define WLAN_RF_RX_INSERTION_LOSS_5G_BAND2  (-8)
#define WLAN_RF_RX_INSERTION_LOSS_5G_BAND3  (-8)
#define WLAN_RF_RX_INSERTION_LOSS_5G_BAND4  (-8)
#define WLAN_RF_RX_INSERTION_LOSS_5G_BAND5  (-8)
#define WLAN_RF_RX_INSERTION_LOSS_5G_BAND6  (-8)
#define WLAN_RF_RX_INSERTION_LOSS_5G_BAND7  (-8)


/* 根据产线delt power更新系数 */
#define cus_flush_nv_ratio_by_delt_pow(_s_pow_par2, _s_pow_par1, _s_pow_par0, _s_delt_power)             \
    do {                                                                                                 \
        (_s_pow_par0) = (((int32_t)(_s_delt_power) * (_s_delt_power) * (_s_pow_par2)) +                \
                            (((int32_t)(_s_pow_par1) << DY_CALI_FIT_PRECISION_A1) * (_s_delt_power)) + \
                            ((int32_t)(_s_pow_par0) << DY_CALI_FIT_PRECISION_A0)) >>                   \
                        DY_CALI_FIT_PRECISION_A0;                                                        \
        (_s_pow_par1) = (((int32_t)(_s_delt_power) * (_s_pow_par2)*2) +                                \
                            ((int32_t)(_s_pow_par1) << DY_CALI_FIT_PRECISION_A1)) >> DY_CALI_FIT_PRECISION_A1;  \
    } while (0)


/* 计算绝对值 */
#define cus_abs(val) ((val) > 0 ? (val) : -(val))

/* 判断CCA能量门限调整值是否超出范围, 最大调整幅度:DELTA_CCA_ED_HIGH_TH_RANGE */
#define cus_delta_cca_ed_high_th_out_of_range(val) (cus_abs(val) > DELTA_CCA_ED_HIGH_TH_RANGE ? 1 : 0)

#define hwifi_get_5g_pro_line_delt_pow_per_band(_ps_nv_params, _ps_ini_params) \
    (int16_t)(((int32_t)(((_ps_nv_params)[1]) - ((_ps_ini_params)[1])) \
                    << DY_CALI_FIT_PRECISION_A1) /                             \
                (2 * ((_ps_nv_params)[0])))

#define hwifi_dyn_cali_get_extre_point(_ps_ini_params) \
    (-(((_ps_ini_params)[1]) << DY_CALI_FIT_PRECISION_A1) / (2 * ((_ps_ini_params)[0])))

/* 枚举定义 */
typedef enum {
    HWIFI_CFG_DYN_PWR_CALI_2G_SNGL_MODE_11B = 0,
    HWIFI_CFG_DYN_PWR_CALI_2G_SNGL_MODE_OFDM20,
    HWIFI_CFG_DYN_PWR_CALI_2G_SNGL_MODE_OFDM40,
    HWIFI_CFG_DYN_PWR_CALI_2G_SNGL_MODE_CW,
    HWIFI_CFG_DYN_PWR_CALI_2G_SNGL_MODE_BUTT,
} hwifi_dyn_2g_pwr_sngl_mode_enum;

/* NV map idx */
typedef enum {
    HWIFI_CFG_NV_WINVRAM_NUMBER = 340,
    HWIFI_CFG_NV_WITXNVCCK_NUMBER = 367,
    HWIFI_CFG_NV_WITXNVC1_NUMBER = 368,
    HWIFI_CFG_NV_WITXNVBWC0_NUMBER = 369,
    HWIFI_CFG_NV_WITXNVBWC1_NUMBER = 370,
    HWIFI_CFG_NV_WITXL2G5G0_NUMBER = 384,
    HWIFI_CFG_NV_WITXL2G5G1_NUMBER = 385,
    HWIFI_CFG_NV_MUFREQ_5G160_C0_NUMBER,
    HWIFI_CFG_NV_MUFREQ_5G160_C1_NUMBER,
    HWIFI_CFG_NV_MUFREQ_2G20_C0_NUMBER = 396,
    HWIFI_CFG_NV_MUFREQ_2G20_C1_NUMBER,
    HWIFI_CFG_NV_MUFREQ_2G40_C0_NUMBER,
    HWIFI_CFG_NV_MUFREQ_2G40_C1_NUMBER,
    HWIFI_CFG_NV_MUFREQ_CCK_C0_NUMBER,
    HWIFI_CFG_NV_MUFREQ_CCK_C1_NUMBER,
} wlan_nvram_idx;

/* 03产侧nvram参数 */
typedef enum {
    /* pa */
    WLAN_CFG_DTS_NVRAM_RATIO_PA2GCCKA0,
    WLAN_CFG_NVRAM_RATIO_PA2GA0,
    WLAN_CFG_DTS_NVRAM_RATIO_PA2G40A0,
    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0,
    WLAN_CFG_DTS_NVRAM_RATIO_PA2GCCKA1,
    WLAN_CFG_DTS_NVRAM_RATIO_PA2GA1,
    WLAN_CFG_DTS_NVRAM_RATIO_PA2G40A1,
    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1,
    /* add 5g low power part */
    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_LOW,
    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_LOW,

    /* DPN */
    WLAN_CFG_DTS_NVRAM_MUFREQ_2GCCK_C0,
    WLAN_CFG_DTS_NVRAM_MUFREQ_2G20_C0,
    WLAN_CFG_DTS_NVRAM_MUFREQ_2G40_C0,
    WLAN_CFG_DTS_NVRAM_MUFREQ_2GCCK_C1,
    WLAN_CFG_DTS_NVRAM_MUFREQ_2G20_C1,
    WLAN_CFG_DTS_NVRAM_MUFREQ_2G40_C1,
    WLAN_CFG_DTS_NVRAM_MUFREQ_5G160_C0,
    WLAN_CFG_DTS_NVRAM_MUFREQ_5G160_C1,
    WLAN_CFG_DTS_NVRAM_END,

    /* just for ini file */
    /* 5g Band1& 2g CW */
    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_BAND1 = WLAN_CFG_DTS_NVRAM_END,
    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_BAND1,
    WLAN_CFG_DTS_NVRAM_RATIO_PA2GCWA0,
    WLAN_CFG_DTS_NVRAM_RATIO_PA2GCWA1,
    /* add 5g low power part */
    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_BAND1_LOW,
    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_BAND1_LOW,

    WLAN_CFG_DTS_NVRAM_RATIO_PPA2GCWA0,
    WLAN_CFG_DTS_NVRAM_RATIO_PPA2GCWA1,

    WLAN_CFG_DTS_NVRAM_PARAMS_BUTT,
    /* 新增系数依次往后添加 */
} pro_line_nvram_params_index;

/* regdomain <-> plat_tag map structure */
typedef struct regdomain_plat_tag_map {
    regdomain_enum_uint8 en_regdomain;
    int plat_tag;
} regdomain_plat_tag_map_stru;

typedef struct {
    char *name;
    int case_entry;
} wlan_cfg_cmd;

typedef struct {
    int32_t l_val;
    oal_bool_enum_uint8 en_value_state;
} wlan_customize_private_stru;

typedef struct wlan_cus_pwr_fit_para_stru {
    int32_t l_pow_par2; /* 二次项系数 */
    int32_t l_pow_par1; /* 一次 */
    int32_t l_pow_par0; /* 常数项 */
} wlan_customize_pwr_fit_para_stru;

typedef struct {
    uint8_t *puc_nv_name;
    uint8_t *puc_param_name;
    uint32_t nv_map_idx;
    uint8_t uc_param_idx;
    uint8_t auc_rsv[BYTE_OFFSET_3];
#ifdef _PRE_CONFIG_READ_DYNCALI_E2PROM
    uint32_t   eeprom_offset;
#endif
} wlan_cfg_nv_map_handler;

/* perf */
typedef struct {
    signed char ac_used_mem_param[NUM_16_BYTES];
    unsigned char uc_sdio_assem_d2h;
    unsigned char auc_resv[NUM_3_BYTES];
} config_device_perf_h2d_stru;

extern wlan_customize_pwr_fit_para_stru g_pro_line_params[WLAN_RF_CHANNEL_NUMS][DY_CALI_PARAMS_NUM];


int hwifi_config_init(int);
int hwifi_get_init_value(int, int);


uint32_t hwifi_hcc_customize_h2d_data_cfg_1103(void);
uint32_t hwifi_custom_host_read_cfg_init_1103(void);
int32_t hwifi_get_init_priv_value(int32_t l_cfg_id, int32_t *pl_priv_value);

uint32_t hwifi_config_init_nvram_main_1103(oal_net_device_stru *pst_cfg_net_dev);
void hwifi_config_cpu_freq_ini_param_1103(void);
void hwifi_config_host_global_ini_param_1103(void);
uint32_t wal_custom_cali_1103(void);
void wal_send_cali_data_1103(oal_net_device_stru *cfg_net_dev);
void wal_send_cali_data_1105(oal_net_device_stru *cfg_net_dev);
void hwifi_get_cfg_params(void);
uint32_t hwifi_get_sar_ctrl_params_1103(uint8_t lvl_num, uint8_t *data_addr, uint16_t *data_len, uint16_t dest_len);
void hwifi_config_init_cus_dyn_cali(oal_net_device_stru *pst_cfg_net_dev);
oal_bool_enum_uint8 hwifi_get_g_fact_cali_completed(void);
uint32_t hwifi_config_sepa_coefficient_from_param(uint8_t *puc_cust_param_info, int32_t *pl_coe_params,
    uint16_t *pus_param_num, uint16_t us_max_idx);
int32_t get_board_mac(uint8_t *puc_buf, uint8_t type);
#endif  // hisi_customize_wifi_hi1103.h
