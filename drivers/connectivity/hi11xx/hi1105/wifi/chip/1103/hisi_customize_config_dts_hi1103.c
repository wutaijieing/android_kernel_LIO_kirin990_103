

/* 头文件包含 */
#include "hisi_customize_wifi_hi1103.h"
#include "hisi_customize_config_dts_hi1103.h"

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/etherdevice.h>
#endif

#include "oal_hcc_host_if.h"
#include "oal_main.h"
#include "hisi_ini.h"
#include "plat_cali.h"
#include "plat_pm_wlan.h"
#include "plat_firmware.h"
#include "oam_ext_if.h"

#include "wlan_spec.h"
#include "wlan_chip_i.h"
#include "hisi_customize_wifi.h"

#include "mac_hiex.h"

#include "wal_config.h"
#include "wal_linux_ioctl.h"

#include "hmac_auto_adjust_freq.h"
#include "hmac_tx_data.h"
#include "hmac_cali_mgmt.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HISI_CUSTOMIZE_CONFIG_DTS_HI1103_C

OAL_STATIC int32_t g_dts_params[WLAN_CFG_DTS_BUTT] = {0};        /* dts定制化参数数组 */

OAL_STATIC wlan_cfg_cmd g_wifi_config_dts[] = {
    /* 校准 */
    { "cali_txpwr_pa_dc_ref_2g_val_chan1",  WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN1 },
    { "cali_txpwr_pa_dc_ref_2g_val_chan2",  WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN2 },
    { "cali_txpwr_pa_dc_ref_2g_val_chan3",  WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN3 },
    { "cali_txpwr_pa_dc_ref_2g_val_chan4",  WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN4 },
    { "cali_txpwr_pa_dc_ref_2g_val_chan5",  WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN5 },
    { "cali_txpwr_pa_dc_ref_2g_val_chan6",  WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN6 },
    { "cali_txpwr_pa_dc_ref_2g_val_chan7",  WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN7 },
    { "cali_txpwr_pa_dc_ref_2g_val_chan8",  WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN8 },
    { "cali_txpwr_pa_dc_ref_2g_val_chan9",  WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN9 },
    { "cali_txpwr_pa_dc_ref_2g_val_chan10", WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN10 },
    { "cali_txpwr_pa_dc_ref_2g_val_chan11", WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN11 },
    { "cali_txpwr_pa_dc_ref_2g_val_chan12", WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN12 },
    { "cali_txpwr_pa_dc_ref_2g_val_chan13", WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN13 },

    { "cali_txpwr_pa_dc_ref_5g_val_band1", WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND1 },
    { "cali_txpwr_pa_dc_ref_5g_val_band2", WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND2 },
    { "cali_txpwr_pa_dc_ref_5g_val_band3", WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND3 },
    { "cali_txpwr_pa_dc_ref_5g_val_band4", WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND4 },
    { "cali_txpwr_pa_dc_ref_5g_val_band5", WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND5 },
    { "cali_txpwr_pa_dc_ref_5g_val_band6", WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND6 },
    { "cali_txpwr_pa_dc_ref_5g_val_band7", WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND7 },
    { "cali_tone_amp_grade",               WLAN_CFG_DTS_CALI_TONE_AMP_GRADE },
    /* DPD校准定制化 */
    { "dpd_cali_ch_core0",          WLAN_CFG_DTS_DPD_CALI_CH_CORE0 },
    { "dpd_use_cail_ch_idx0_core0", WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX0_CORE0 },
    { "dpd_use_cail_ch_idx1_core0", WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX1_CORE0 },
    { "dpd_use_cail_ch_idx2_core0", WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX2_CORE0 },
    { "dpd_use_cail_ch_idx3_core0", WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX3_CORE0 },
    { "dpd_cali_20m_del_pow_core0", WLAN_CFG_DTS_DPD_CALI_20M_DEL_POW_CORE0 },
    { "dpd_cali_40m_del_pow_core0", WLAN_CFG_DTS_DPD_CALI_40M_DEL_POW_CORE0 },
    { "dpd_cali_ch_core1",          WLAN_CFG_DTS_DPD_CALI_CH_CORE1 },
    { "dpd_use_cail_ch_idx0_core1", WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX0_CORE1 },
    { "dpd_use_cail_ch_idx1_core1", WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX1_CORE1 },
    { "dpd_use_cail_ch_idx2_core1", WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX2_CORE1 },
    { "dpd_use_cail_ch_idx3_core1", WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX3_CORE1 },
    { "dpd_cali_20m_del_pow_core1", WLAN_CFG_DTS_DPD_CALI_20M_DEL_POW_CORE1 },
    { "dpd_cali_40m_del_pow_core1", WLAN_CFG_DTS_DPD_CALI_40M_DEL_POW_CORE1 },
    /* 动态校准 */
    { "dyn_cali_dscr_interval", WLAN_CFG_DTS_DYN_CALI_DSCR_ITERVL },
    { "dyn_cali_opt_switch",    WLAN_CFG_DTS_DYN_CALI_OPT_SWITCH },
    { "gm0_dB10_amend",         WLAN_CFG_DTS_DYN_CALI_GM0_DB10_AMEND },
    /* DPN 40M 20M 11b */
    { "dpn24g_ch1_core0",  WLAN_CFG_DTS_2G_CORE0_DPN_CH1 },
    { "dpn24g_ch2_core0",  WLAN_CFG_DTS_2G_CORE0_DPN_CH2 },
    { "dpn24g_ch3_core0",  WLAN_CFG_DTS_2G_CORE0_DPN_CH3 },
    { "dpn24g_ch4_core0",  WLAN_CFG_DTS_2G_CORE0_DPN_CH4 },
    { "dpn24g_ch5_core0",  WLAN_CFG_DTS_2G_CORE0_DPN_CH5 },
    { "dpn24g_ch6_core0",  WLAN_CFG_DTS_2G_CORE0_DPN_CH6 },
    { "dpn24g_ch7_core0",  WLAN_CFG_DTS_2G_CORE0_DPN_CH7 },
    { "dpn24g_ch8_core0",  WLAN_CFG_DTS_2G_CORE0_DPN_CH8 },
    { "dpn24g_ch9_core0",  WLAN_CFG_DTS_2G_CORE0_DPN_CH9 },
    { "dpn24g_ch10_core0", WLAN_CFG_DTS_2G_CORE0_DPN_CH10 },
    { "dpn24g_ch11_core0", WLAN_CFG_DTS_2G_CORE0_DPN_CH11 },
    { "dpn24g_ch12_core0", WLAN_CFG_DTS_2G_CORE0_DPN_CH12 },
    { "dpn24g_ch13_core0", WLAN_CFG_DTS_2G_CORE0_DPN_CH13 },
    { "dpn5g_core0_b0",    WLAN_CFG_DTS_5G_CORE0_DPN_B0 },
    { "dpn5g_core0_b1",    WLAN_CFG_DTS_5G_CORE0_DPN_B1 },
    { "dpn5g_core0_b2",    WLAN_CFG_DTS_5G_CORE0_DPN_B2 },
    { "dpn5g_core0_b3",    WLAN_CFG_DTS_5G_CORE0_DPN_B3 },
    { "dpn5g_core0_b4",    WLAN_CFG_DTS_5G_CORE0_DPN_B4 },
    { "dpn5g_core0_b5",    WLAN_CFG_DTS_5G_CORE0_DPN_B5 },
    { "dpn5g_core0_b6",    WLAN_CFG_DTS_5G_CORE0_DPN_B6 },
    { "dpn24g_ch1_core1",  WLAN_CFG_DTS_2G_CORE1_DPN_CH1 },
    { "dpn24g_ch2_core1",  WLAN_CFG_DTS_2G_CORE1_DPN_CH2 },
    { "dpn24g_ch3_core1",  WLAN_CFG_DTS_2G_CORE1_DPN_CH3 },
    { "dpn24g_ch4_core1",  WLAN_CFG_DTS_2G_CORE1_DPN_CH4 },
    { "dpn24g_ch5_core1",  WLAN_CFG_DTS_2G_CORE1_DPN_CH5 },
    { "dpn24g_ch6_core1",  WLAN_CFG_DTS_2G_CORE1_DPN_CH6 },
    { "dpn24g_ch7_core1",  WLAN_CFG_DTS_2G_CORE1_DPN_CH7 },
    { "dpn24g_ch8_core1",  WLAN_CFG_DTS_2G_CORE1_DPN_CH8 },
    { "dpn24g_ch9_core1",  WLAN_CFG_DTS_2G_CORE1_DPN_CH9 },
    { "dpn24g_ch10_core1", WLAN_CFG_DTS_2G_CORE1_DPN_CH10 },
    { "dpn24g_ch11_core1", WLAN_CFG_DTS_2G_CORE1_DPN_CH11 },
    { "dpn24g_ch12_core1", WLAN_CFG_DTS_2G_CORE1_DPN_CH12 },
    { "dpn24g_ch13_core1", WLAN_CFG_DTS_2G_CORE1_DPN_CH13 },
    { "dpn5g_core1_b0",    WLAN_CFG_DTS_5G_CORE1_DPN_B0 },
    { "dpn5g_core1_b1",    WLAN_CFG_DTS_5G_CORE1_DPN_B1 },
    { "dpn5g_core1_b2",    WLAN_CFG_DTS_5G_CORE1_DPN_B2 },
    { "dpn5g_core1_b3",    WLAN_CFG_DTS_5G_CORE1_DPN_B3 },
    { "dpn5g_core1_b4",    WLAN_CFG_DTS_5G_CORE1_DPN_B4 },
    { "dpn5g_core1_b5",    WLAN_CFG_DTS_5G_CORE1_DPN_B5 },
    { "dpn5g_core1_b6",    WLAN_CFG_DTS_5G_CORE1_DPN_B6 },
    { NULL, 0 }
};
int32_t *hwifi_get_g_dts_params(void)
{
    return (int32_t *)g_dts_params;
}

wlan_cfg_cmd *hwifi_get_g_wifi_config_dts(void)
{
    return (wlan_cfg_cmd *)g_wifi_config_dts;
}
/*
 * 函 数 名  : original_value_for_dts_params
 * 功能描述  : dts定制化参数初值处理
 */
void original_value_for_dts_params(void)
{
    /* 校准 */
    g_dts_params[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN1] = WLAN_CALI_TXPWR_REF_2G_CH1_VAL;
    g_dts_params[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN2] = WLAN_CALI_TXPWR_REF_2G_CH2_VAL;
    g_dts_params[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN3] = WLAN_CALI_TXPWR_REF_2G_CH3_VAL;
    g_dts_params[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN4] = WLAN_CALI_TXPWR_REF_2G_CH4_VAL;
    g_dts_params[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN5] = WLAN_CALI_TXPWR_REF_2G_CH5_VAL;
    g_dts_params[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN6] = WLAN_CALI_TXPWR_REF_2G_CH6_VAL;
    g_dts_params[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN7] = WLAN_CALI_TXPWR_REF_2G_CH7_VAL;
    g_dts_params[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN8] = WLAN_CALI_TXPWR_REF_2G_CH8_VAL;
    g_dts_params[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN9] = WLAN_CALI_TXPWR_REF_2G_CH9_VAL;
    g_dts_params[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN10] = WLAN_CALI_TXPWR_REF_2G_CH10_VAL;
    g_dts_params[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN11] = WLAN_CALI_TXPWR_REF_2G_CH11_VAL;
    g_dts_params[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN12] = WLAN_CALI_TXPWR_REF_2G_CH12_VAL;
    g_dts_params[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_VAL_CHAN13] = WLAN_CALI_TXPWR_REF_2G_CH13_VAL;
    g_dts_params[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND1] = WLAN_CALI_TXPWR_REF_5G_BAND1_VAL;
    g_dts_params[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND2] = WLAN_CALI_TXPWR_REF_5G_BAND2_VAL;
    g_dts_params[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND3] = WLAN_CALI_TXPWR_REF_5G_BAND3_VAL;
    g_dts_params[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND4] = WLAN_CALI_TXPWR_REF_5G_BAND4_VAL;
    g_dts_params[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND5] = WLAN_CALI_TXPWR_REF_5G_BAND5_VAL;
    g_dts_params[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND6] = WLAN_CALI_TXPWR_REF_5G_BAND6_VAL;
    g_dts_params[WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_VAL_BAND7] = WLAN_CALI_TXPWR_REF_5G_BAND7_VAL;
    g_dts_params[WLAN_CFG_DTS_CALI_TONE_AMP_GRADE] = WLAN_CALI_TONE_GRADE_AMP;
    /* DPD校准定制化 */
    g_dts_params[WLAN_CFG_DTS_DPD_CALI_CH_CORE0] = 0x641DA71,
    g_dts_params[WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX0_CORE0] = 0x11110000,
    g_dts_params[WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX1_CORE0] = 0x33222,
    g_dts_params[WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX2_CORE0] = 0x22211000,
    g_dts_params[WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX3_CORE0] = 0x2,
    g_dts_params[WLAN_CFG_DTS_DPD_CALI_20M_DEL_POW_CORE0] = 0x00000000,
    g_dts_params[WLAN_CFG_DTS_DPD_CALI_40M_DEL_POW_CORE0] = 0x00000000,
    g_dts_params[WLAN_CFG_DTS_DPD_CALI_CH_CORE1] = 0x641DA71,
    g_dts_params[WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX0_CORE1] = 0x11110000,
    g_dts_params[WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX1_CORE1] = 0x33222,
    g_dts_params[WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX2_CORE1] = 0x22211000,
    g_dts_params[WLAN_CFG_DTS_DPD_USE_CALI_CH_IDX3_CORE1] = 0x2,
    g_dts_params[WLAN_CFG_DTS_DPD_CALI_20M_DEL_POW_CORE1] = 0x00000000,
    g_dts_params[WLAN_CFG_DTS_DPD_CALI_40M_DEL_POW_CORE1] = 0x00000000,
    g_dts_params[WLAN_CFG_DTS_DYN_CALI_DSCR_ITERVL] = 0x0;
}

OAL_STATIC uint32_t hwifi_cfg_init_dts_cus_cali_2g_txpwr_pa_dc_ref(mac_cus_dts_cali_stru *pst_cus_cali)
{
    uint32_t uc_idx;
    int32_t l_val;
    int16_t s_ref_val_ch1, s_ref_val_ch0;

    /* 2G REF: 分13个信道 */
    for (uc_idx = 0; uc_idx < 13; uc_idx++) {
        l_val = hwifi_get_init_value(CUS_TAG_DTS, WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_START + uc_idx);
        s_ref_val_ch1 = (int16_t)cus_get_high_16bits(l_val);
        s_ref_val_ch0 = (int16_t)cus_get_low_16bits(l_val);
        /* 2G判断参考值先不判断<0, 待与RF同事确认 */
        if (s_ref_val_ch0 <= CALI_TXPWR_PA_DC_REF_MAX) {
            pst_cus_cali->ast_cali[0].aus_cali_txpwr_pa_dc_ref_2g_val_chan[uc_idx] = s_ref_val_ch0;
        } else {
            /* 值超出有效范围 */
            oam_error_log2(0, OAM_SF_ANY, "{hwifi_cfg_init_dts_cus_cali::dts 2g ref id[%d]value[%d] out of range!}",
                           WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_START + uc_idx, s_ref_val_ch0);  //lint !e571
            return OAL_FAIL;
        }
        /* 02不需要适配双通道 */
        if (s_ref_val_ch1 <= CALI_TXPWR_PA_DC_REF_MAX) {
            pst_cus_cali->ast_cali[1].aus_cali_txpwr_pa_dc_ref_2g_val_chan[uc_idx] = s_ref_val_ch1;
        } else {
            /* 值超出有效范围 */
            oam_error_log2(0, OAM_SF_ANY, "{hwifi_cfg_init_dts_cus_cali::dts ch1 2g ref id[%d]value[%d] invalid!}",
                           WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_2G_START + uc_idx, s_ref_val_ch1);  //lint !e571
            return OAL_FAIL;
        }
    }
    return OAL_SUCC;
}

OAL_STATIC uint32_t hwifi_cfg_init_dts_cus_cali_5g_txpwr_pa_dc_ref(mac_cus_dts_cali_stru *pst_cus_cali)
{
    uint32_t uc_idx;
    int32_t l_val;
    int16_t s_ref_val_ch1, s_ref_val_ch0;

    /* 5G REF: 分7个band */
    for (uc_idx = 0; uc_idx < 7; ++uc_idx) {
        l_val = hwifi_get_init_value(CUS_TAG_DTS, WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_START + uc_idx);
        s_ref_val_ch1 = (int16_t)cus_get_high_16bits(l_val);
        s_ref_val_ch0 = (int16_t)cus_get_low_16bits(l_val);
        if (s_ref_val_ch0 >= 0 && s_ref_val_ch0 <= CALI_TXPWR_PA_DC_REF_MAX) {
            pst_cus_cali->ast_cali[0].aus_cali_txpwr_pa_dc_ref_5g_val_band[uc_idx] = s_ref_val_ch0;
        } else {
            /* 值超出有效范围 */
            oam_error_log2(0, OAM_SF_ANY, "{hwifi_cfg_init_dts_cus_cali::dts 5g ref id[%d]val[%d] invalid}",
                WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_START + uc_idx, s_ref_val_ch0);  //lint !e571
            return OAL_FAIL;
        }
        if (s_ref_val_ch1 >= 0 && s_ref_val_ch1 <= CALI_TXPWR_PA_DC_REF_MAX) {
            pst_cus_cali->ast_cali[1].aus_cali_txpwr_pa_dc_ref_5g_val_band[uc_idx] = s_ref_val_ch1;
        } else {
            /* 值超出有效范围 */
            oam_error_log2(0, OAM_SF_ANY, "{hwifi_cfg_init_dts_cus_cali::dts ch1 5g ref id[%d]val[%d] invalid!}",
                WLAN_CFG_DTS_CALI_TXPWR_PA_DC_REF_5G_START + uc_idx, s_ref_val_ch1);  //lint !e571
            return OAL_FAIL;
        }
    }
    return OAL_SUCC;
}

/*
 * 函 数 名  : hwifi_cfg_init_dts_cus_cali
 * 功能描述  : 定制化配置dts校准参数
 * 1.日    期  : 2016年12月8日
 *   作    者  : wulei
 *   修改内容  : 新生成函数
 */
OAL_STATIC uint32_t hwifi_cfg_init_dts_cus_cali(mac_cus_dts_cali_stru *pst_cus_cali, uint8_t uc_5g_band_enable)
{
    int32_t l_val;
    uint8_t uc_idx; /* 结构体数组下标 */
    uint8_t uc_gm_opt;

    /** 配置: TXPWR_PA_DC_REF **/
    if (hwifi_cfg_init_dts_cus_cali_2g_txpwr_pa_dc_ref(pst_cus_cali) != OAL_SUCC) {
        return OAL_FAIL;
    }

    if (uc_5g_band_enable) {
        if (hwifi_cfg_init_dts_cus_cali_5g_txpwr_pa_dc_ref(pst_cus_cali) != OAL_SUCC) {
            return OAL_FAIL;
        }
    }

    /* 配置BAND 5G ENABLE */
    pst_cus_cali->uc_band_5g_enable = !!uc_5g_band_enable;

    /* 配置单音幅度档位 */
    pst_cus_cali->uc_tone_amp_grade =
        (uint8_t)hwifi_get_init_value(CUS_TAG_DTS, WLAN_CFG_DTS_CALI_TONE_AMP_GRADE);

    /* 配置DPD校准参数 */
    for (uc_idx = 0; uc_idx < MAC_DPD_CALI_CUS_PARAMS_NUM; uc_idx++) {
        /* 通道0 */
        l_val = hwifi_get_init_value(CUS_TAG_DTS, WLAN_CFG_DTS_DPD_CALI_START + uc_idx);
        pst_cus_cali->ast_dpd_cali_para[0].aul_dpd_cali_cus_dts[uc_idx] = (uint32_t)l_val;
        /* 通道1 */
        l_val = hwifi_get_init_value(CUS_TAG_DTS, WLAN_CFG_DTS_DPD_CALI_START +
            uc_idx + MAC_DPD_CALI_CUS_PARAMS_NUM);
        pst_cus_cali->ast_dpd_cali_para[1].aul_dpd_cali_cus_dts[uc_idx] = (uint32_t)l_val;
    }

    /* 配置动态校准参数 */
    l_val = hwifi_get_init_value(CUS_TAG_DTS, WLAN_CFG_DTS_DYN_CALI_DSCR_ITERVL);
    pst_cus_cali->aus_dyn_cali_dscr_interval[WLAN_BAND_2G] = (uint16_t)((uint32_t)l_val & 0x0000FFFF);

    if (uc_5g_band_enable) {
        pst_cus_cali->aus_dyn_cali_dscr_interval[WLAN_BAND_5G] =
            (uint16_t)(((uint32_t)l_val & 0xFFFF0000) >> BIT_OFFSET_16);
    }

    l_val = hwifi_get_init_value(CUS_TAG_DTS, WLAN_CFG_DTS_DYN_CALI_OPT_SWITCH);
    uc_gm_opt = ((uint32_t)l_val & BIT2) >> NUM_1_BITS;

    if (((uint32_t)l_val & 0x3) >> 1) {
        /* 自适应选择 */
        l_val = !hwifi_get_g_fact_cali_completed();
    } else {
        l_val = (int32_t)((uint32_t)l_val & BIT0);
    }

    pst_cus_cali->en_dyn_cali_opt_switch = (uint32_t)l_val | uc_gm_opt;

    l_val = hwifi_get_init_value(CUS_TAG_DTS, WLAN_CFG_DTS_DYN_CALI_GM0_DB10_AMEND);
    pst_cus_cali->gm0_db10_amend[WLAN_RF_CHANNEL_ZERO] = (int16_t)cus_get_low_16bits(l_val);
    pst_cus_cali->gm0_db10_amend[WLAN_RF_CHANNEL_ONE] = (int16_t)cus_get_high_16bits(l_val);

    return OAL_SUCC;
}


OAL_STATIC uint32_t hwifi_config_init_dts_cali(oal_net_device_stru *pst_cfg_net_dev)
{
    wal_msg_write_stru st_write_msg;
    uint32_t ret;
    mac_cus_dts_cali_stru st_cus_cali;
    uint32_t offset = 0;
    oal_bool_enum en_5g_band_enable; /* mac device是否支持5g能力 */

    if (oal_warn_on(pst_cfg_net_dev == NULL)) {
        oam_error_log0(0, OAM_SF_CFG, "{hwifi_config_init_dts_cali::pst_cfg_net_dev is null}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    memset_s(&st_cus_cali, sizeof(mac_cus_dts_cali_stru), 0, sizeof(mac_cus_dts_cali_stru));
    /* 检查硬件是否需要使能5g */
    en_5g_band_enable = mac_device_check_5g_enable_per_chip();

    /* 配置校准参数TXPWR_PA_DC_REF */
    ret = hwifi_cfg_init_dts_cus_cali(&st_cus_cali, en_5g_band_enable);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_error_log1(0, OAM_SF_CFG, "{hwifi_config_init_dts_cali::init dts cus cali failed ret[%d]!}", ret);
        return ret;
    }

    /* 如果所有参数都在有效范围内，则下发配置值 */
    if (EOK != memcpy_s(st_write_msg.auc_value, sizeof(st_write_msg.auc_value),
                        (int8_t *)&st_cus_cali, sizeof(mac_cus_dts_cali_stru))) {
        oam_error_log0(0, OAM_SF_CFG, "hwifi_config_init_dts_cali::memcpy fail!");
        return OAL_FAIL;
    }
    offset += sizeof(mac_cus_dts_cali_stru);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_CUS_DTS_CALI, offset);
    ret = (uint32_t)wal_send_cfg_event(pst_cfg_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + offset,
        (uint8_t *)&st_write_msg, OAL_FALSE, NULL);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_error_log1(0, OAM_SF_ANY, "{hwifi_config_init_dts_cali::wal_send_cfg_event failed, ret[%d]!}", ret);
        return ret;
    }

    oam_warning_log0(0, OAM_SF_CFG, "{hwifi_config_init_dts_cali::wal_send_cfg_event send succ}");

    return OAL_SUCC;
}

uint32_t hwifi_config_init_dts_main(oal_net_device_stru *cfg_net_dev)
{
    uint32_t ret = OAL_SUCC;

    /* 下发动态校准参数 */
    hwifi_config_init_cus_dyn_cali(cfg_net_dev);

    /* 校准 */
    if (OAL_SUCC != hwifi_config_init_dts_cali(cfg_net_dev)) {
        return OAL_FAIL;
    }
    /* 校准放到第一个进行 */
    return ret;
}
