/*
* 版权所有 (c) 华为技术有限公司 2020-2020
* 功能说明   : sniffer计算rate功能
* 作者       : wwx5321423
* 创建日期   : 2021年3月24日
*/
#ifdef _PRE_WLAN_FEATURE_SNIFFER
/* 1 头文件包含 */
#include "hmac_rate_calc.h"
#include "hal_common.h"
#include "securec.h"
#include "securectype.h"
#include "oal_types.h"
#include "oam_ext_if.h"
#include "wlan_types.h"
#include "hal_common_descriptor.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_RATE_CALC_C

#define HMAC_HT_MCS_INDEX7 7
#define HMAC_HT_MCS_INDEX15 15
#define HMAC_HT_MCS_INDEX23 23
#define HMAC_HT_MAX_MCS_INDEX 31
#define HMAC_HE_ER_SU_BW_106_RU 1

#define HMAC_HT_BW_INDEX 2
#define HMAC_HT_MCS_INDEX 8
#define HMAC_VHT_BW_INDEX 4
#define HMAC_VHT_MCS_INDEX 12
#define HMAC_HE_BW_INDEX 7
#define HMAC_HE_MCS_INDEX 14


typedef struct {
    uint8_t mcs_index;
    uint32_t rate_kbps;
}hmac_11b_11ag_rate_stru;

/* NSS = 1时的带宽和 Ndbps 的二维数组 */
const uint32_t g_11n_rate_bw_dbps[HMAC_HT_BW_INDEX][HMAC_HT_MCS_INDEX] = {
    /* 0    1    2    3    4    5    6    7(MCS_index ) */
    { 26,  52,  78,  104, 156, 208, 234, 260 }, /* 20MHZ */
    { 54,  108, 162, 216, 324, 432, 486, 540 }  /* 40MHZ */
};

const uint32_t g_11ac_rate_bw_dbps[HMAC_VHT_BW_INDEX][HMAC_VHT_MCS_INDEX] = {
    /* 0    1    2    3    4     5     6     7     8     9     10    11(MCS_index) */
    { 26,  52,  78,  104, 156,  208,  234,  260,  312,  347,  390,  430 },  /* 20MHZ */
    { 54,  108, 162, 216, 324,  432,  486,  540,  648,  720,  810,  900 },  /* 40MHZ */
    { 117, 234, 351, 468, 702,  936,  1053, 1170, 1404, 1560, 1755, 1950 }, /* 80MHZ */
    { 234, 468, 702, 936, 1404, 1872, 2106, 2340, 2808, 3120, 3510, 3900 }  /* 160MHZ */
};

const uint32_t g_11ax_rate_bw_dbps[HMAC_HE_BW_INDEX][HMAC_HE_MCS_INDEX] = {
    /* 0    1     2     3     4     5     6     7     8      9      10     11     12    13(MCS_index) */
    { 117, 234,  351,  468,  702,  936,  1053, 1170, 1404,  1560,  1755,  1950,  2106,  2340 },     /* 20MHZ */
    { 234, 468,  702,  936,  1404, 1872, 2106, 2340, 2808,  3120,  3510,  3900,  4212,  4680 },     /* 40MHZ */
    { 490, 980,  1470, 1960, 2940, 3920, 4410, 4900, 5880,  6533,  7350,  8166,  8820,  9800 },     /* 80MHZ */
    { 980, 1960, 2940, 3920, 5880, 7840, 8820, 9800, 11760, 13066, 14700, 16333, 17640, 19600 },    /* 160MHZ */
    { 12,  24,   36,   48,   72,   96,   108,  120,  144,   160,   180,   200,   216,   240 },      /* 26-tone RU */
    { 24,  48,   72,   96,   144,  192,  216,  240,  288,   320,   360,   400,   432,   480 },      /* 52-tone RU */
    { 51,  102,  153,  204,  306,  408,  459,  510,  612,   680,   765,   850,   918,   1020 }      /* 106-tone RU */
};

/*
 * 函 数 名   : rate_kbps_formula_calc
 * 功能描述   : 通过速率计算公式计算kbps
 */
static uint32_t rate_kbps_formula_calc(uint32_t rate_dbps, uint8_t data_symbol_time, uint8_t gi)
{
    uint32_t rate_kbps;
    /* 计算公式 */
    rate_kbps = (rate_dbps * 10 * 1000) / (data_symbol_time + gi); /* *1000 计算kbps */
    rate_kbps = (uint32_t)(((rate_kbps + 50) / 100) * 100); /* 四舍五入最低两位（十进制）置零 */

    return rate_kbps;
}

/*
 * 函 数 名   : hmac_11b_11g_rate_kbps_calc
 * 功能描述   : 11b_11g的rate计算
 */
static uint32_t hmac_11b_11g_rate_kbps_calc(wlan_protocol_enum_uint8 protocol, uint8_t mcs_index)
{
    uint32_t rate_kbps = 0;
    hmac_11b_11ag_rate_stru hmac_11b_rate_table[WLAN_11B_BUTT] = {
        { WLAN_11B_1_M_BPS, 1000},
        { WLAN_11B_2_M_BPS, 2000},
        { WLAN_11B_5_HALF_M_BPS, 5500},
        { WLAN_11B_11_M_BPS, 11000}
    };
    hmac_11b_11ag_rate_stru hmac_11ag_rate_table[WLAN_11AG_BUTT] = {
        { WLAN_11AG_6M_BPS, 6000},
        { WLAN_11AG_9M_BPS, 9000},
        { WLAN_11AG_12M_BPS, 12000},
        { WLAN_11AG_18M_BPS, 18000},
        { WLAN_11AG_24M_BPS, 24000},
        { WLAN_11AG_36M_BPS, 36000},
        { WLAN_11AG_48M_BPS, 48000},
        { WLAN_11AG_54M_BPS, 54000}
    };

    if ((protocol == WLAN_PHY_PROTOCOL_MODE_11B) && (mcs_index <= WLAN_11B_11_M_BPS)) {
        rate_kbps = hmac_11b_rate_table[mcs_index].rate_kbps;
    } else if (mcs_index <= WLAN_11AG_54M_BPS) {
        rate_kbps = hmac_11ag_rate_table[mcs_index].rate_kbps;
    } else {
        oam_error_log2(0, OAM_SF_ANY, "hmac_11b_11g_rate_kbps_calc::protocol[%d],mcs_index[%d] wrong!",
            protocol, mcs_index);
    }

    return rate_kbps;
}

/*
 * 函 数 名   : hmac_ht_rate_kbps_calc
 * 功能描述   : ht的rate计算
 */
static uint32_t hmac_ht_rate_kbps_calc(uint8_t bw, wlan_nss_enum_uint8 nss, uint8_t mcs_index, uint8_t gi_type)
{
    uint8_t data_symbol_time = 32; /* 3.2 * 10 */
    uint8_t gi_value[] = { 8, 4 }; /* 0.8us 0.4 us */
    uint32_t rate_bw_dbps;

    if ((bw >= HMAC_HT_BW_INDEX) || (mcs_index > HMAC_HT_MAX_MCS_INDEX)) {
        oam_error_log0(0, OAM_SF_ANY, "hmac_vht_rate_kbps_calc::bw or msc_index fail !");
        return OAL_FALSE;
    }
    if (mcs_index > HMAC_HT_MCS_INDEX7) {
        mcs_index -= ((nss - 1) * 8);
    }

    rate_bw_dbps = g_11n_rate_bw_dbps[bw][mcs_index] * nss;

    return rate_kbps_formula_calc(rate_bw_dbps, data_symbol_time, gi_value[gi_type]);
}

/*
 * 函 数 名   : hmac_vht_rate_kbps_calc
 * 功能描述   : vht的rate计算
 */
static uint32_t hmac_vht_rate_kbps_calc(uint8_t bw, wlan_nss_enum_uint8 nss, uint8_t mcs_index, uint8_t gi_type)
{
    uint8_t data_symbol_time = 32; /* 3.2 * 10 */
    uint8_t gi_value[] = { 8, 4 }; /* 0.8us 0.4 us */
    uint32_t rate_bw_dbps;

    if ((bw >= HMAC_VHT_BW_INDEX) || (mcs_index >= HMAC_VHT_MCS_INDEX)) {
        oam_error_log0(0, OAM_SF_ANY, "hmac_vht_rate_kbps_calc::bw or msc_index fail !");
        return OAL_FALSE;
    }

    rate_bw_dbps = g_11ac_rate_bw_dbps[bw][mcs_index] * nss; /* bw 0:20m 1:40m 2:80m 3:160m */

    return rate_kbps_formula_calc(rate_bw_dbps, data_symbol_time, gi_value[gi_type]);
}

/*
 * 函 数 名   : hmac_he_rate_kbps_calc
 * 功能描述   : he的rate计算
 */
static uint32_t hmac_he_rate_kbps_calc(uint8_t bw, wlan_nss_enum_uint8 nss, uint8_t mcs_index, uint8_t gi_type,
    wlan_protocol_enum_uint8 protocl_type)
{
    uint8_t data_symbol_time = 128; /* 12.8 * 10 */
    uint8_t gi_value[] = { 8, 16, 32, 4 }; /* 0.8us 1.6us 3.2us 0.4 us */
    uint32_t rate_bw_dbps;

    if ((bw >= HMAC_HE_BW_INDEX) || (mcs_index >= HMAC_HE_MCS_INDEX)) {
        oam_error_log0(0, OAM_SF_ANY, "hmac_vht_rate_kbps_calc::bw or msc_index fail !");
        return OAL_FALSE;
    }

    if (protocl_type == WLAN_PHY_PROTOCOL_MODE_HE_ER_SU) { /* he eu su 模式下 */
        if (bw == HMAC_HE_ER_SU_BW_106_RU) {
            bw = HMAC_HE_BW_INDEX - 1; /* 更新到106_RU对应的索引 */
        }
    }

    rate_bw_dbps = g_11ax_rate_bw_dbps[bw][mcs_index] * nss;

    return rate_kbps_formula_calc(rate_bw_dbps, data_symbol_time, gi_value[gi_type]);
}


/*
 * 函 数 名   : hmac_get_ht_nss_by_mcs_index
 * 功能描述   : 根据ht rate_index获取nss
 */
static wlan_nss_enum_uint8 hmac_get_ht_nss_by_mcs_index(uint8_t rate_index)
{
    wlan_nss_enum_uint8 nss = WLAN_SINGLE_NSS;
    if (rate_index > HMAC_HT_MAX_MCS_INDEX && rate_index != WLAN_HT_MCS32) {
        oam_error_log1(0, OAM_SF_ANY, "{hmac_get_ht_nss_by_mcs_index:: HT rate_value[%d] invalid!}", rate_index);
        return nss;
    }

    if (rate_index <= HMAC_HT_MCS_INDEX7 || rate_index == WLAN_HT_MCS32) {
        nss = WLAN_SINGLE_NSS + 1;
    } else if (rate_index <= HMAC_HT_MCS_INDEX15 && rate_index > HMAC_HT_MCS_INDEX7) {
        nss = WLAN_DOUBLE_NSS + 1;
    } else if (rate_index <= HMAC_HT_MCS_INDEX23 && rate_index > HMAC_HT_MCS_INDEX15) {
        nss = WLAN_TRIPLE_NSS + 1;
    } else if (rate_index <= HMAC_HT_MAX_MCS_INDEX && rate_index > HMAC_HT_MCS_INDEX23) {
        nss = WLAN_FOUR_NSS + 1;
    }

    return nss;
}

/*
 * 函 数 名   : hmac_rate_kbps_calc
 * 功能描述   : 根据协议不同计算速率
 */
static uint32_t hmac_rate_kbps_calc(wlan_protocol_enum_uint8 protocol, uint8_t bw, wlan_nss_enum_uint8 nss,
    uint8_t gi_type, uint8_t mcs_index)
{
    /* 判断协议模式protocol，取不同模式的数组 */
    /* 根据bw，mcs_index获取数组里面对应的DBPS */
    /* 根据nss计算当前的DBPS */
    /* 根据计算公式得到最后的速率值 */
    switch (protocol) {
        case WLAN_PHY_PROTOCOL_MODE_11B:
        case WLAN_PHY_PROTOCOL_MODE_LEGACY_OFDM:
            return hmac_11b_11g_rate_kbps_calc(protocol, mcs_index);
        case WLAN_PHY_PROTOCOL_MODE_HT_MIXED_MODE:
        case WLAN_PHY_PROTOCOL_MODE_HT_GREEN_FIELD:
            return hmac_ht_rate_kbps_calc(bw, nss, mcs_index, gi_type);
        case WLAN_PHY_PROTOCOL_MODE_VHT:
            return hmac_vht_rate_kbps_calc(bw, nss, mcs_index, gi_type);
        case WLAN_PHY_PROTOCOL_MODE_HE_SU:
        case WLAN_PHY_PROTOCOL_MODE_HE_ER_SU:
            return hmac_he_rate_kbps_calc(bw, nss, mcs_index, gi_type, protocol);
        default:
            oam_error_log1(0, OAM_SF_ANY, "{hmac_rate_kbps_calc::protocolanalysis fail! protocol_mode[%d]}", protocol);
            return OAL_FALSE;
    }
}

/*
 * 函 数 名   : hmac_get_rate_kbps
 * 功能描述   : 通过速率参数获取速率表中的rate信息
 */
uint32_t hmac_get_rate_kbps(hal_statistic_stru *rate_info, uint32_t *rate_kbps)
{
    wlan_protocol_enum_uint8 protocol;
    uint8_t bw;
    wlan_nss_enum_uint8 nss;
    uint8_t rate_index;

    if (oal_any_null_ptr2(rate_info, rate_kbps)) {
        oam_error_log0(0, OAM_SF_AUTORATE, "{hmac_get_rate_kbps::input pointer null!}");
        return OAL_FALSE;
    }

    protocol = rate_info->un_nss_rate.rate_stru.bit_protocol_mode;
    bw = rate_info->uc_bandwidth;
    if (protocol == WLAN_PHY_PROTOCOL_MODE_HT_MIXED_MODE || protocol == WLAN_PHY_PROTOCOL_MODE_HT_GREEN_FIELD) {
        rate_index = rate_info->un_nss_rate.st_ht_rate.bit_ht_mcs;
        nss = hmac_get_ht_nss_by_mcs_index(rate_index);
    } else {
        rate_index = rate_info->un_nss_rate.rate_stru.bit_rate_mcs;
        nss = rate_info->un_nss_rate.rate_stru.bit_nss_mode;
    }

    *rate_kbps = hmac_rate_kbps_calc(protocol, bw, nss, rate_info->uc_short_gi, rate_index);

    return OAL_TRUE;
}

#endif
