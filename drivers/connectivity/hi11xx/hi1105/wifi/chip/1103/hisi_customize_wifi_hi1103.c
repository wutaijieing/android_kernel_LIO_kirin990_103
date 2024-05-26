

/* 头文件包含 */
#include "hisi_customize_wifi_hi1103.h"
#include "hisi_customize_config_dts_hi1103.h"
#include "hisi_customize_config_priv_hi1103.h"
#include "hisi_customize_config_cmd_hi1103.h"
#include "hisi_customize_nvram_config_hi1103.h"

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

#ifdef _PRE_CONFIG_READ_DYNCALI_E2PROM
#ifdef CONFIG_ARCH_PLATFORM
#define nve_direct_access_interface(...)  0
#else
#define hisi_nve_direct_access(...)  0
#endif /* end for CONFIG_ARCH_PLATFORM */
#else
#include "external/nve_info_interface.h"
#endif /* end for _PRE_CONFIG_READ_DYNCALI_E2PROM */

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HISI_CUSTOMIZE_WIFI_HI1103_C

OAL_STATIC uint8_t g_wifi_mac[WLAN_MAC_ADDR_LEN] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/* 产测定制化参数数组 */
wlan_customize_pwr_fit_para_stru g_pro_line_params[WLAN_RF_CHANNEL_NUMS][DY_CALI_PARAMS_NUM] = {{{0}}};
OAL_STATIC uint8_t g_cust_nvram_info[WLAN_CFG_DTS_NVRAM_END][CUS_PARAMS_LEN_MAX] = {{0}}; /* NVRAM数组 */

static int16_t g_gs_extre_point_vals[WLAN_RF_CHANNEL_NUMS][DY_CALI_NUM_5G_BAND] = {{0}};

oal_bool_enum_uint8 g_en_fact_cali_completed = OAL_FALSE;

/* kunpeng eeprom */
#ifdef _PRE_CONFIG_READ_DYNCALI_E2PROM
OAL_STATIC wlan_cfg_nv_map_handler g_wifi_nvram_cfg_handler[WLAN_CFG_DTS_NVRAM_END] = {
    {"WITXCCK",  "pa2gccka0",   HWIFI_CFG_NV_WITXNVCCK_NUMBER,       WLAN_CFG_DTS_NVRAM_RATIO_PA2GCCKA0, {0}, 0x100},
    {"WINVRAM",  "pa2ga0",      HWIFI_CFG_NV_WINVRAM_NUMBER,         WLAN_CFG_NVRAM_RATIO_PA2GA0,        {0}, 0x0},
    {"WITXLC0", "pa2g40a0",    HWIFI_CFG_NV_WITXL2G5G0_NUMBER,      WLAN_CFG_DTS_NVRAM_RATIO_PA2G40A0,  {0}, 0x400},
    {"WINVRAM",  "pa5ga0",      HWIFI_CFG_NV_WINVRAM_NUMBER,         WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0,    {0}, 0x80},
    {"WITXCCK",  "pa2gccka1",   HWIFI_CFG_NV_WITXNVCCK_NUMBER,       WLAN_CFG_DTS_NVRAM_RATIO_PA2GCCKA1, {0}, 0x180},
    {"WITXRF1",  "pa2ga1",      HWIFI_CFG_NV_WITXNVC1_NUMBER,        WLAN_CFG_DTS_NVRAM_RATIO_PA2GA1,    {0}, 0x200},
    {"WITXLC1",  "pa2g40a1",    HWIFI_CFG_NV_WITXL2G5G1_NUMBER,      WLAN_CFG_DTS_NVRAM_RATIO_PA2G40A1,  {0}, 0x480},
    {"WITXRF1",  "pa5ga1",      HWIFI_CFG_NV_WITXNVC1_NUMBER,        WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1,    {0}, 0x280},
    {"WIC0_5GLOW",  "pa5glowa0",  HWIFI_CFG_NV_WITXNVBWC0_NUMBER,    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_LOW,  {0}, 0x300},
    {"WIC1_5GLOW",  "pa5glowa1",  HWIFI_CFG_NV_WITXNVBWC1_NUMBER,    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_LOW,  {0}, 0x380},
    // DPN
    {"WIC0CCK",  "mf2gccka0",   HWIFI_CFG_NV_MUFREQ_CCK_C0_NUMBER,   WLAN_CFG_DTS_NVRAM_MUFREQ_2GCCK_C0,  {0}, 0x0},
    {"WC0OFDM",  "mf2ga0",      HWIFI_CFG_NV_MUFREQ_2G20_C0_NUMBER,  WLAN_CFG_DTS_NVRAM_MUFREQ_2G20_C0,   {0}, 0x0},
    {"C02G40M",  "mf2g40a0",    HWIFI_CFG_NV_MUFREQ_2G40_C0_NUMBER,  WLAN_CFG_DTS_NVRAM_MUFREQ_2G40_C0,   {0}, 0x0},
    {"WIC1CCK",  "mf2gccka1",   HWIFI_CFG_NV_MUFREQ_CCK_C1_NUMBER,   WLAN_CFG_DTS_NVRAM_MUFREQ_2GCCK_C1,  {0}, 0x0},
    {"WC1OFDM",  "mf2ga1",      HWIFI_CFG_NV_MUFREQ_2G20_C1_NUMBER,  WLAN_CFG_DTS_NVRAM_MUFREQ_2G20_C1,   {0}, 0x0},
    {"C12G40M",  "mf2g40a1",    HWIFI_CFG_NV_MUFREQ_2G40_C1_NUMBER,  WLAN_CFG_DTS_NVRAM_MUFREQ_2G40_C1,   {0}, 0x0},
    {"WIC0_5G160M", "dpn160c0", HWIFI_CFG_NV_MUFREQ_5G160_C0_NUMBER, WLAN_CFG_DTS_NVRAM_MUFREQ_5G160_C0, {0}, 0x500},
    {"WIC1_5G160M", "dpn160c1", HWIFI_CFG_NV_MUFREQ_5G160_C1_NUMBER, WLAN_CFG_DTS_NVRAM_MUFREQ_5G160_C1, {0}, 0x580},
};
#else
OAL_STATIC wlan_cfg_nv_map_handler g_wifi_nvram_cfg_handler[WLAN_CFG_DTS_NVRAM_END] = {
    { "WITXCCK",    "pa2gccka0", HWIFI_CFG_NV_WITXNVCCK_NUMBER,  WLAN_CFG_DTS_NVRAM_RATIO_PA2GCCKA0,  {0}},
    { "WINVRAM",    "pa2ga0",    HWIFI_CFG_NV_WINVRAM_NUMBER,    WLAN_CFG_NVRAM_RATIO_PA2GA0,         {0}},
    { "WITXLC0",    "pa2g40a0",  HWIFI_CFG_NV_WITXL2G5G0_NUMBER, WLAN_CFG_DTS_NVRAM_RATIO_PA2G40A0,   {0}},
    { "WINVRAM",    "pa5ga0",    HWIFI_CFG_NV_WINVRAM_NUMBER,    WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0,     {0}},
    { "WITXCCK",    "pa2gccka1", HWIFI_CFG_NV_WITXNVCCK_NUMBER,  WLAN_CFG_DTS_NVRAM_RATIO_PA2GCCKA1,  {0}},
    { "WITXRF1",    "pa2ga1",    HWIFI_CFG_NV_WITXNVC1_NUMBER,   WLAN_CFG_DTS_NVRAM_RATIO_PA2GA1,     {0}},
    { "WITXLC1",    "pa2g40a1",  HWIFI_CFG_NV_WITXL2G5G1_NUMBER, WLAN_CFG_DTS_NVRAM_RATIO_PA2G40A1,   {0}},
    { "WITXRF1",    "pa5ga1",    HWIFI_CFG_NV_WITXNVC1_NUMBER,   WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1,     {0}},
    { "W5GLOW0",    "pa5glowa0", HWIFI_CFG_NV_WITXNVBWC0_NUMBER, WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_LOW, {0}},
    { "W5GLOW1",    "pa5glowa1", HWIFI_CFG_NV_WITXNVBWC1_NUMBER, WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_LOW, {0}},
    // DPN
    { "WIC0CCK",     "mf2gccka0", HWIFI_CFG_NV_MUFREQ_CCK_C0_NUMBER,   WLAN_CFG_DTS_NVRAM_MUFREQ_2GCCK_C0, {0}},
    { "WC0OFDM",     "mf2ga0",    HWIFI_CFG_NV_MUFREQ_2G20_C0_NUMBER,  WLAN_CFG_DTS_NVRAM_MUFREQ_2G20_C0,  {0}},
    { "C02G40M",     "mf2g40a0",  HWIFI_CFG_NV_MUFREQ_2G40_C0_NUMBER,  WLAN_CFG_DTS_NVRAM_MUFREQ_2G40_C0,  {0}},
    { "WIC1CCK",     "mf2gccka1", HWIFI_CFG_NV_MUFREQ_CCK_C1_NUMBER,   WLAN_CFG_DTS_NVRAM_MUFREQ_2GCCK_C1, {0}},
    { "WC1OFDM",     "mf2ga1",    HWIFI_CFG_NV_MUFREQ_2G20_C1_NUMBER,  WLAN_CFG_DTS_NVRAM_MUFREQ_2G20_C1,  {0}},
    { "C12G40M",     "mf2g40a1",  HWIFI_CFG_NV_MUFREQ_2G40_C1_NUMBER,  WLAN_CFG_DTS_NVRAM_MUFREQ_2G40_C1,  {0}},
    { "W160MC0",     "dpn160c0",  HWIFI_CFG_NV_MUFREQ_5G160_C0_NUMBER, WLAN_CFG_DTS_NVRAM_MUFREQ_5G160_C0, {0}},
    { "W160MC1",     "dpn160c1",  HWIFI_CFG_NV_MUFREQ_5G160_C1_NUMBER, WLAN_CFG_DTS_NVRAM_MUFREQ_5G160_C1, {0}},
};
#endif

OAL_STATIC wlan_cfg_cmd g_nvram_pro_line_config_ini[] = {
    /* 产侧nvram参数 */
    { "nvram_pa2gccka0",  WLAN_CFG_DTS_NVRAM_RATIO_PA2GCCKA0 },
    { "nvram_pa2ga0",     WLAN_CFG_NVRAM_RATIO_PA2GA0 },
    { "nvram_pa2g40a0",   WLAN_CFG_DTS_NVRAM_RATIO_PA2G40A0 },
    { "nvram_pa5ga0",     WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0 },
    { "nvram_pa2gccka1",  WLAN_CFG_DTS_NVRAM_RATIO_PA2GCCKA1 },
    { "nvram_pa2ga1",     WLAN_CFG_DTS_NVRAM_RATIO_PA2GA1 },
    { "nvram_pa2g40a1",   WLAN_CFG_DTS_NVRAM_RATIO_PA2G40A1 },
    { "nvram_pa5ga1",     WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1 },
    { "nvram_pa5ga0_low", WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_LOW },
    { "nvram_pa5ga1_low", WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_LOW },

    { NULL, WLAN_CFG_DTS_NVRAM_MUFREQ_2GCCK_C0 },
    { NULL, WLAN_CFG_DTS_NVRAM_MUFREQ_2G20_C0 },
    { NULL, WLAN_CFG_DTS_NVRAM_MUFREQ_2G40_C0 },
    { NULL, WLAN_CFG_DTS_NVRAM_MUFREQ_2GCCK_C1 },
    { NULL, WLAN_CFG_DTS_NVRAM_MUFREQ_2G20_C1 },
    { NULL, WLAN_CFG_DTS_NVRAM_MUFREQ_2G40_C1 },
    { NULL, WLAN_CFG_DTS_NVRAM_MUFREQ_5G160_C0 },
    { NULL, WLAN_CFG_DTS_NVRAM_MUFREQ_5G160_C1 },

    { "nvram_pa5ga0_band1",     WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_BAND1 },
    { "nvram_pa5ga1_band1",     WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_BAND1 },
    { "nvram_pa2gcwa0",         WLAN_CFG_DTS_NVRAM_RATIO_PA2GCWA0 },
    { "nvram_pa2gcwa1",         WLAN_CFG_DTS_NVRAM_RATIO_PA2GCWA1 },
    { "nvram_pa5ga0_band1_low", WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_BAND1_LOW },
    { "nvram_pa5ga1_band1_low", WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_BAND1_LOW },

    { "nvram_ppa2gcwa0", WLAN_CFG_DTS_NVRAM_RATIO_PPA2GCWA0 },
    { "nvram_ppa2gcwa1", WLAN_CFG_DTS_NVRAM_RATIO_PPA2GCWA1 },

    { NULL, WLAN_CFG_DTS_NVRAM_PARAMS_BUTT },
};

static int32_t hwifi_hcc_custom_get_data(uint16_t syn_id, oal_netbuf_stru *netbuf)
{
    uint32_t data_len = 0;
    uint8_t *netbuf_data = (uint8_t *)oal_netbuf_data(netbuf);

    switch (syn_id) {
        case CUSTOM_CFGID_INI_ID:
            /* INI hmac to dmac 配置项 */
            data_len = (uint32_t)hwifi_custom_adapt_device_ini_param(netbuf_data);
            break;
        case CUSTOM_CFGID_PRIV_INI_ID:
            /* 私有定制化配置项 */
            data_len = (uint32_t)hwifi_custom_adapt_device_priv_ini_param(netbuf_data);
            break;
        default:
            oam_error_log1(0, OAM_SF_CFG, "hwifi_hcc_custom_ini_data_buf::unknown us_syn_id[%d]", syn_id);
            break;
    }

    return data_len;
}

/*
 * 函 数 名  : hwifi_hcc_custom_ini_data_buf
 * 功能描述  : 下发定制化配置命令
 */
OAL_STATIC int32_t hwifi_hcc_custom_ini_data_buf(uint16_t us_syn_id)
{
    oal_netbuf_stru *pst_netbuf = NULL;
    uint32_t data_len;
    int32_t l_ret;
    uint32_t max_data_len = hcc_get_handler(HCC_EP_WIFI_DEV)->tx_max_buf_len;

    struct hcc_transfer_param st_hcc_transfer_param = {0};
    struct hcc_handler *hcc = hcc_get_handler(HCC_EP_WIFI_DEV);
    if (hcc == NULL) {
        oam_error_log0(0, OAM_SF_CFG, "hwifi_hcc_custom_ini_data_buf hcc::is is null");
        return -OAL_EFAIL;
    }
    pst_netbuf = hwifi_hcc_custom_netbuf_alloc();
    if (pst_netbuf == NULL) {
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* 组netbuf */
    data_len = (uint32_t)hwifi_hcc_custom_get_data(us_syn_id, pst_netbuf);
    if (data_len > max_data_len) {
        oam_error_log2(0, OAM_SF_CFG,
            "hwifi_hcc_custom_ini_data_buf::got wrong data_len[%d] max_len[%d]", data_len, max_data_len);
        oal_netbuf_free(pst_netbuf);
        return OAL_FAIL;
    }

    if (data_len == 0) {
        oam_error_log1(0, OAM_SF_CFG, "hwifi_hcc_custom_ini_data_buf::data is null us_syn_id[%d]", us_syn_id);
        oal_netbuf_free(pst_netbuf);
        return OAL_SUCC;
    }

    if ((pst_netbuf->data_len) || (pst_netbuf->data == NULL)) {
        oal_io_print("len:%d\r\n", pst_netbuf->data_len);
        oal_netbuf_free(pst_netbuf);
        return OAL_FAIL;
    }

    oal_netbuf_put(pst_netbuf, data_len);
    hcc_hdr_param_init(&st_hcc_transfer_param, HCC_ACTION_TYPE_CUSTOMIZE,
                       us_syn_id, 0, HCC_FC_WAIT, DATA_HI_QUEUE);

    l_ret = hcc_tx(hcc, pst_netbuf, &st_hcc_transfer_param);
    if (oal_unlikely(l_ret != OAL_SUCC)) {
        oam_error_log1(0, OAM_SF_CFG, "hwifi_hcc_custom_ini_data_buf fail ret[%d]", l_ret);
        oal_netbuf_free(pst_netbuf);
    }

    return l_ret;
}

int32_t hwifi_read_conf_from(uint8_t *puc_buffer_cust_nvram, uint8_t uc_idx)
{
    int32_t l_ret;
#ifdef _PRE_CONFIG_READ_DYNCALI_E2PROM
#ifdef _PRE_SUSPORT_OEMINFO
    l_ret = is_hitv_miniproduct();
    if (l_ret == OAL_SUCC) {
        l_ret = read_conf_from_oeminfo(puc_buffer_cust_nvram, CUS_PARAMS_LEN_MAX,
            g_wifi_nvram_cfg_handler[uc_idx].eeprom_offset);
    } else {
        l_ret = read_conf_from_eeprom(puc_buffer_cust_nvram, CUS_PARAMS_LEN_MAX,
            g_wifi_nvram_cfg_handler[uc_idx].eeprom_offset);
    }
#else
    l_ret = read_conf_from_eeprom(puc_buffer_cust_nvram, CUS_PARAMS_LEN_MAX,
        g_wifi_nvram_cfg_handler[uc_idx].eeprom_offset);
#endif
#else
    l_ret = read_conf_from_nvram(puc_buffer_cust_nvram, CUS_PARAMS_LEN_MAX,
        g_wifi_nvram_cfg_handler[uc_idx].nv_map_idx, g_wifi_nvram_cfg_handler[uc_idx].puc_nv_name);
#endif
    return l_ret;
}

static int32_t hwifi_set_cust_nvram_info(uint8_t *cust_nvram_info, int8_t **pc_token, int32_t *pl_params,
                                         int8_t *pc_ctx, uint8_t idx)
{
    uint8_t *pc_end = ";";
    uint8_t *pc_sep = ",";
    uint8_t param_idx;
    uint8_t times_idx = 0;
    *(cust_nvram_info + (idx * CUS_PARAMS_LEN_MAX * sizeof(uint8_t)) + OAL_STRLEN(*pc_token)) = *pc_end;

     /* 拟合系数获取检查 */
    if (idx <= WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_LOW) {
        /* 二次参数合理性检查 */
        *pc_token = oal_strtok(*pc_token, pc_sep, &pc_ctx);
        param_idx = 0;
        /* 获取定制化系数 */
        while (*pc_token != NULL) {
            /* 将字符串转换成10进制数 */
            *(pl_params + param_idx) = (int32_t)oal_strtol(*pc_token, NULL, 10);
            *pc_token = oal_strtok(NULL, pc_sep, &pc_ctx);
            param_idx++;
        }
        if (param_idx % DY_CALI_PARAMS_TIMES) {
            oam_error_log1(0, OAM_SF_CUSTOM, "hwifi_custom_host_read_dyn_cali_nvram::check NV id[%d]!",
                           g_wifi_nvram_cfg_handler[idx].nv_map_idx);
            memset_s(cust_nvram_info + (idx * CUS_PARAMS_LEN_MAX * sizeof(uint8_t)),
                     CUS_PARAMS_LEN_MAX * sizeof(uint8_t), 0, CUS_PARAMS_LEN_MAX * sizeof(uint8_t));
            return OAL_FALSE;
        }
        times_idx = param_idx / DY_CALI_PARAMS_TIMES;
        /* 二次项系数非0检查 */
        while (times_idx) {
            times_idx--;
            if (pl_params[(times_idx)*DY_CALI_PARAMS_TIMES] == 0) {
                oam_error_log1(0, OAM_SF_CUSTOM, "hwifi_custom_host_read_dyn_cali_nvram::check NV id[%d]!",
                               g_wifi_nvram_cfg_handler[idx].nv_map_idx);
                memset_s(cust_nvram_info + (idx * CUS_PARAMS_LEN_MAX * sizeof(uint8_t)),
                         CUS_PARAMS_LEN_MAX * sizeof(uint8_t), 0, CUS_PARAMS_LEN_MAX * sizeof(uint8_t));
                break;
            }
        }

        return OAL_TRUE;
    }
    return OAL_FALSE;
}

static void hwifi_init_custnvram_and_debug_nvram_cfg(uint8_t *cust_nvram_info, uint8_t idx)
{
    memset_s(cust_nvram_info + (idx * CUS_PARAMS_LEN_MAX * sizeof(uint8_t)),
             CUS_PARAMS_LEN_MAX * sizeof(uint8_t), 0,
             CUS_PARAMS_LEN_MAX * sizeof(uint8_t));
    oal_io_print("hwifi_custom_host_read_dyn_cali_nvram::NVRAM get fail NV id[%d] name[%s] para[%s]!\r\n",
                 g_wifi_nvram_cfg_handler[idx].nv_map_idx,
                 g_wifi_nvram_cfg_handler[idx].puc_nv_name,
                 g_wifi_nvram_cfg_handler[idx].puc_param_name);
}

static uint32_t hwifi_read_conf_and_set_cust_nvram(uint8_t *buffer_cust_nvram_tmp, uint8_t idx,
                                                   uint8_t *cust_nvram_info, int32_t *params,
                                                   oal_bool_enum_uint8 *tmp_fact_cali_completed)
{
    int32_t ret;
    uint8_t *end = ";";
    int8_t *str = NULL;
    int8_t *ctx = NULL;
    int8_t *token = NULL;

    ret = hwifi_read_conf_from(buffer_cust_nvram_tmp, idx);
    if (ret != INI_SUCC) {
        hwifi_init_custnvram_and_debug_nvram_cfg(cust_nvram_info, idx);
        return OAL_TRUE;
    }

    str = OAL_STRSTR(buffer_cust_nvram_tmp, g_wifi_nvram_cfg_handler[idx].puc_param_name);
    if (str == NULL) {
        hwifi_init_custnvram_and_debug_nvram_cfg(cust_nvram_info, idx);
        return OAL_TRUE;
    }

    /* 获取等号后面的实际参数 */
    str += (OAL_STRLEN(g_wifi_nvram_cfg_handler[idx].puc_param_name) + 1);
    token = oal_strtok(str, end, &ctx);
    if (token == NULL) {
        oam_error_log1(0, OAM_SF_CUSTOM, "hwifi_custom_host_read_dyn_cali_nvram::get null value check NV id[%d]!",
                       g_wifi_nvram_cfg_handler[idx].nv_map_idx);
        hwifi_init_custnvram_and_debug_nvram_cfg(cust_nvram_info, idx);
        return OAL_TRUE;
    }

    ret = memcpy_s(cust_nvram_info + (idx * CUS_PARAMS_LEN_MAX * sizeof(uint8_t)),
                   (WLAN_CFG_DTS_NVRAM_END - idx) * CUS_PARAMS_LEN_MAX * sizeof(uint8_t),
                   token, OAL_STRLEN(token));
    if (ret != EOK) {
        oam_error_log1(0, OAM_SF_CUSTOM, "hwifi_custom_host_read_dyn_cali_nvram::memcpy_s fail[%d]!", ret);
        return OAL_FALSE;
    }

    if (hwifi_set_cust_nvram_info(cust_nvram_info, &token, params, ctx, idx) == OAL_TRUE) {
        *tmp_fact_cali_completed = OAL_TRUE;
    }

    return OAL_TRUE;
}

OAL_STATIC int32_t hwifi_custom_check_nvram_data_flag(void)
{
    int32_t val;
    int32_t l_ret;
    oal_bool_enum_uint8 nv_flag = OAL_FALSE;

    l_ret = hwifi_get_init_priv_value(WLAN_CFG_PRIV_CALI_DATA_MASK, &val);
    if (l_ret == OAL_SUCC) {
        nv_flag = !!(CUST_READ_NVRAM_MASK & (uint32_t)val);
        if (nv_flag) {
            oal_io_print("hwifi_custom_host_read_dyn_cali_nvram::nv flag[%d] to abandon nvram data!!\r\n", val);
            memset_s(g_cust_nvram_info, sizeof(g_cust_nvram_info), 0, sizeof(g_cust_nvram_info));
            return INI_FILE_TIMESPEC_UNRECONFIG;
        }
    }
    return OAL_SUCC;
}

OAL_STATIC void hwifi_custom_init_nvram_data(uint8_t *buffer_cust_nvram, int32_t *pl_params, uint8_t *cust_nvram_info)
{
    memset_s(buffer_cust_nvram,
             CUS_PARAMS_LEN_MAX * sizeof(uint8_t),
             0,
             CUS_PARAMS_LEN_MAX * sizeof(uint8_t));
    memset_s(pl_params,
             DY_CALI_PARAMS_NUM * DY_CALI_PARAMS_TIMES * sizeof(int32_t),
             0,
             DY_CALI_PARAMS_NUM * DY_CALI_PARAMS_TIMES * sizeof(int32_t));
    memset_s(cust_nvram_info,
             WLAN_CFG_DTS_NVRAM_END * CUS_PARAMS_LEN_MAX * sizeof(uint8_t),
             0,
             WLAN_CFG_DTS_NVRAM_END * CUS_PARAMS_LEN_MAX * sizeof(uint8_t));
}

OAL_STATIC int32_t hwifi_custom_set_nvram_data(uint8_t *cust_nvram_info)
{
    if (oal_memcmp(cust_nvram_info, g_cust_nvram_info, sizeof(g_cust_nvram_info)) == 0) {
        return INI_FILE_TIMESPEC_UNRECONFIG;
    }

    if (memcpy_s(g_cust_nvram_info, sizeof(g_cust_nvram_info),
                 cust_nvram_info, WLAN_CFG_DTS_NVRAM_END * CUS_PARAMS_LEN_MAX * sizeof(uint8_t)) != EOK) {
        return INI_FAILED;
    }
    return INI_NVRAM_RECONFIG;
}

/*
 * 函 数 名  : hwifi_custom_host_read_dyn_cali_nvram
 * 功能描述  : 包括读取nvram中的dpint和校准系数值
 */
OAL_STATIC int32_t hwifi_custom_host_read_dyn_cali_nvram(void)
{
    int32_t l_ret;
    uint8_t uc_idx;
    uint8_t *buffer_cust_nvram = NULL;
    int32_t *pl_params = NULL;
    uint8_t *cust_nvram_info = NULL; /* NVRAM数组 */
    oal_bool_enum_uint8 tmp_en_fact_cali_completed = OAL_FALSE;

    /* 判断定制化中是否使用nvram中的动态校准参数 */
    l_ret = hwifi_custom_check_nvram_data_flag();
    if (l_ret != OAL_SUCC) {
        return l_ret;
    }

    buffer_cust_nvram = (uint8_t *)os_kzalloc_gfp(CUS_PARAMS_LEN_MAX * sizeof(uint8_t));
    if (buffer_cust_nvram == NULL) {
        oam_error_log0(0, OAM_SF_CUSTOM, "hwifi_custom_host_read_dyn_cali_nvram::cust_nvram_tmp mem alloc fail!");
        memset_s(g_cust_nvram_info, sizeof(g_cust_nvram_info), 0, sizeof(g_cust_nvram_info));
        return INI_FILE_TIMESPEC_UNRECONFIG;
    }

    pl_params = (int32_t *)os_kzalloc_gfp(DY_CALI_PARAMS_NUM * DY_CALI_PARAMS_TIMES * sizeof(int32_t));
    if (pl_params == NULL) {
        oam_error_log0(0, OAM_SF_CUSTOM, "hwifi_custom_host_read_dyn_cali_nvram::ps_params mem alloc fail!");
        os_mem_kfree(buffer_cust_nvram);
        memset_s(g_cust_nvram_info, sizeof(g_cust_nvram_info), 0, sizeof(g_cust_nvram_info));
        return INI_FILE_TIMESPEC_UNRECONFIG;
    }

    cust_nvram_info = (uint8_t *)os_kzalloc_gfp(WLAN_CFG_DTS_NVRAM_END * CUS_PARAMS_LEN_MAX * sizeof(uint8_t));
    if (cust_nvram_info == NULL) {
        oam_error_log0(0, OAM_SF_CUSTOM, "hwifi_custom_host_read_dyn_cali_nvram::cust_nvram_info mem alloc fail!");
        os_mem_kfree(buffer_cust_nvram);
        os_mem_kfree(pl_params);
        memset_s(g_cust_nvram_info, sizeof(g_cust_nvram_info), 0, sizeof(g_cust_nvram_info));
        return INI_FILE_TIMESPEC_UNRECONFIG;
    }

    hwifi_custom_init_nvram_data(buffer_cust_nvram, pl_params, cust_nvram_info);

    /* 拟合系数 */
    for (uc_idx = WLAN_CFG_DTS_NVRAM_RATIO_PA2GCCKA0; uc_idx < WLAN_CFG_DTS_NVRAM_END; uc_idx++) {
        if (hwifi_read_conf_and_set_cust_nvram(buffer_cust_nvram, uc_idx, cust_nvram_info,
                                               pl_params, &tmp_en_fact_cali_completed) != OAL_TRUE) {
            break;
        }
    }

    g_en_fact_cali_completed = tmp_en_fact_cali_completed;

    os_mem_kfree(buffer_cust_nvram);
    os_mem_kfree(pl_params);

    /* 检查NVRAM是否修改 */
    l_ret = hwifi_custom_set_nvram_data(cust_nvram_info);
    os_mem_kfree(cust_nvram_info);
    return l_ret;
}

uint8_t *hwifi_get_nvram_param(uint32_t nvram_param_idx)
{
    return g_cust_nvram_info[nvram_param_idx];
}

/*
 * 函 数 名  : custom_host_read_cfg_init
 * 功能描述  : 首次读取定制化配置文件总入口
 */
uint32_t hwifi_custom_host_read_cfg_init_1103(void)
{
    int32_t l_nv_read_ret;
    int32_t l_ini_read_ret;

    hwifi_get_country_code();
    /* 先获取私有定制化项 */
    if (hwifi_config_init(CUS_TAG_PRIV_INI) != INI_SUCC) {
        oam_error_log0(0, OAM_SF_CFG, "hwifi_custom_host_read_cfg_init::hwifi_config_init fail");
    }

    /* 读取nvram参数是否修改 */
    l_nv_read_ret = hwifi_custom_host_read_dyn_cali_nvram();
    /* 检查定制化文件中的产线配置是否修改 */
    l_ini_read_ret = ini_file_check_conf_update();
    if (l_ini_read_ret || l_nv_read_ret) {
        oam_warning_log0(0, OAM_SF_CFG, "hwifi_custom_host_read_cfg_init config is updated");
        hwifi_config_init(CUS_TAG_PRO_LINE_INI);
    }

    if (l_ini_read_ret == INI_FILE_TIMESPEC_UNRECONFIG) {
        oam_warning_log0(0, OAM_SF_CFG, "hwifi_custom_host_read_cfg_init file is not updated");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    if (hwifi_config_init(CUS_TAG_DTS) != INI_SUCC) {
        oam_error_log0(0, OAM_SF_CFG, "hwifi_custom_host_read_cfg_init::hwifi_config_init fail");
    }
    l_ini_read_ret = hwifi_config_init(CUS_TAG_NV);
    if (oal_unlikely(l_ini_read_ret != OAL_SUCC)) {
        oal_io_print("hwifi_custom_host_read_cfg_init NV fail l_ret[%d].\r\n", l_ini_read_ret);
    }

    if (hwifi_config_init(CUS_TAG_INI) != INI_SUCC) {
        oam_error_log0(0, OAM_SF_CFG, "hwifi_custom_host_read_cfg_init::hwifi_config_init fail");
    }

    /* 启动完成后，输出打印 */
    oal_io_print("hwifi_custom_host_read_cfg_init finish!\r\n");

    return OAL_SUCC;
}

/*
 * 函 数 名  : hwifi_hcc_customize_h2d_data_cfg
 * 功能描述  : 协议栈初始化前定制化配置入口
 */
uint32_t hwifi_hcc_customize_h2d_data_cfg_1103(void)
{
    int32_t l_ret;

    /* wifi上电时重读定制化配置 */
    l_ret = (int32_t)hwifi_custom_host_read_cfg_init_1103();
    if (l_ret != OAL_SUCC) {
        oam_warning_log1(0, OAM_SF_CFG, "hwifi_hcc_customize_h2d_data_cfg data ret[%d]", l_ret);
    }

    // 如果不成功，返回失败
    l_ret = hwifi_hcc_custom_ini_data_buf(CUSTOM_CFGID_PRIV_INI_ID);
    if (oal_unlikely(l_ret != OAL_SUCC)) {
        oam_error_log1(0, OAM_SF_CFG, "hwifi_hcc_customize_h2d_data_cfg priv data fail, ret[%d]", l_ret);
        return INI_FAILED;
    }

    l_ret = hwifi_hcc_custom_ini_data_buf(CUSTOM_CFGID_INI_ID);
    if (oal_unlikely(l_ret != OAL_SUCC)) {
        oam_error_log1(0, OAM_SF_CFG, "hwifi_hcc_customize_h2d_data_cfg ini data fail, ret[%d]", l_ret);
        return INI_FAILED;
    }
    return INI_SUCC;
}


/*
 * 函 数 名  : hwifi_config_sepa_coefficient_from_param
 * 功能描述  : 从字符串中分割二次系数项
 */
uint32_t hwifi_config_sepa_coefficient_from_param(uint8_t *puc_cust_param_info, int32_t *pl_coe_params,
    uint16_t *pus_param_num, uint16_t us_max_idx)
{
    int8_t *pc_token = NULL;
    int8_t *pc_ctx = NULL;
    int8_t *pc_end = ";";
    int8_t *pc_sep = ",";
    uint16_t us_param_num = 0;
    uint8_t auc_cust_param[CUS_PARAMS_LEN_MAX];

    if (memcpy_s(auc_cust_param, CUS_PARAMS_LEN_MAX, puc_cust_param_info, OAL_STRLEN(puc_cust_param_info)) != EOK) {
        return OAL_FAIL;
    }

    pc_token = oal_strtok(auc_cust_param, pc_end, &pc_ctx);
    if (pc_token == NULL) {
        oam_error_log0(0, OAM_SF_CUSTOM, "hwifi_config_sepa_coefficient_from_param read get null value check!");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pc_token = oal_strtok(pc_token, pc_sep, &pc_ctx);
    /* 获取定制化系数 */
    while (pc_token != NULL) {
        if (us_param_num == us_max_idx) {
            oam_error_log2(0, OAM_SF_CUSTOM,
                           "hwifi_config_sepa_coefficient_from_param::nv or ini param is too many idx[%d] Max[%d]",
                           us_param_num, us_max_idx);
            return OAL_FAIL;
        }
        /* 将字符串转换成10进制数 */
        *(pl_coe_params + us_param_num) = (int32_t)oal_strtol(pc_token, NULL, 10);
        pc_token = oal_strtok(NULL, pc_sep, &pc_ctx);
        us_param_num++;
    }

    *pus_param_num = us_param_num;
    return OAL_SUCC;
}


static void hwifi_get_cfg_delt_ru_pow_params(void)
{
    int32_t l_cfg_idx_one = 0;
    int32_t l_cfg_idx_two = 0;
    wlan_cust_nvram_params *pst_g_cust_nv_params = hwifi_get_nvram_params();

    for (l_cfg_idx_one = 0; l_cfg_idx_one < WLAN_BW_CAP_BUTT; l_cfg_idx_one++) {
        for (l_cfg_idx_two = 0; l_cfg_idx_two < WLAN_HE_RU_SIZE_BUTT; l_cfg_idx_two++) {
            oal_io_print("%s[%d][%d]:%d \n", "5g_cfg_tpc_ru_pow", l_cfg_idx_one, l_cfg_idx_two,
                pst_g_cust_nv_params->ac_fullbandwidth_to_ru_power_5g[l_cfg_idx_one][l_cfg_idx_two]);
        }
    }
    for (l_cfg_idx_one = 0; l_cfg_idx_one < WLAN_BW_CAP_80M; l_cfg_idx_one++) {
        for (l_cfg_idx_two = 0; l_cfg_idx_two < WLAN_HE_RU_SIZE_996; l_cfg_idx_two++) {
            oal_io_print("%s[%d][%d]:%d \n", "2g_cfg_tpc_ru_pow", l_cfg_idx_one, l_cfg_idx_two,
                pst_g_cust_nv_params->ac_fullbandwidth_to_ru_power_2g[l_cfg_idx_one][l_cfg_idx_two]);
        }
    }
}

/*
 * 函 数 名  : hwifi_config_get_switch_point_5g
 * 功能描述  : 根据ini文件获取5G二次曲线功率切换点
 */
OAL_STATIC void hwifi_config_get_5g_curv_switch_point(uint8_t *puc_ini_pa_params, uint32_t cfg_id)
{
    int32_t l_ini_params[CUS_NUM_5G_BW * DY_CALI_PARAMS_TIMES] = {0};
    uint16_t us_ini_param_num = 0;
    uint8_t uc_secon_ratio_idx = 0;
    uint8_t uc_param_idx;
    uint8_t uc_chain_idx;
    int16_t *ps_extre_point_val = NULL;

    if ((cfg_id == WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0) || (cfg_id == WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_BAND1)) {
        uc_chain_idx = WLAN_RF_CHANNEL_ZERO;
    } else if ((cfg_id == WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1) || (cfg_id == WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_BAND1)) {
        uc_chain_idx = WLAN_RF_CHANNEL_ONE;
    } else {
        return;
    }

    /* 获取拟合系数项 */
    if (hwifi_config_sepa_coefficient_from_param(puc_ini_pa_params, l_ini_params, &us_ini_param_num,
                                                 sizeof(l_ini_params) / sizeof(int32_t)) != OAL_SUCC ||
        (us_ini_param_num % DY_CALI_PARAMS_TIMES)) {
        oam_error_log2(0, OAM_SF_CUSTOM,
                       "hwifi_config_get_5g_curv_switch_point::ini is unsuitable,num of ini[%d] cfg_id[%d]!",
                       us_ini_param_num, cfg_id);
        return;
    }

    ps_extre_point_val = g_gs_extre_point_vals[uc_chain_idx];
    us_ini_param_num /= DY_CALI_PARAMS_TIMES;
    if (cfg_id <= WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1) {
        if (us_ini_param_num != CUS_NUM_5G_BW) {
            oam_error_log2(0, OAM_SF_CUSTOM,
                "hwifi_config_get_5g_curv_switch_point::ul_cfg_id[%d] us_ini_param_num[%d]", cfg_id, us_ini_param_num);
            return;
        }
        ps_extre_point_val++;
    } else {
        if (us_ini_param_num != 1) {
            oam_error_log2(0, OAM_SF_CUSTOM,
                "hwifi_config_get_5g_curv_switch_point::ul_cfg_id[%d] us_ini_param_num[%d]", cfg_id, us_ini_param_num);
            return;
        }
    }

    /* 计算5g曲线switch point */
    for (uc_param_idx = 0; uc_param_idx < us_ini_param_num; uc_param_idx++) {
        *(ps_extre_point_val + uc_param_idx) = (int16_t)hwifi_dyn_cali_get_extre_point(l_ini_params +
                                               uc_secon_ratio_idx);
        oal_io_print("hwifi_config_get_5g_curv_switch_point::extre power[%d] param_idx[%d] cfg_id[%d]!\r\n",
                     *(ps_extre_point_val + uc_param_idx), uc_param_idx, cfg_id);
        oal_io_print("hwifi_config_get_5g_curv_switch_point::param[%d %d] uc_secon_ratio_idx[%d]!\r\n",
            (l_ini_params + uc_secon_ratio_idx)[0], (l_ini_params + uc_secon_ratio_idx)[1], uc_secon_ratio_idx);
        uc_secon_ratio_idx += DY_CALI_PARAMS_TIMES;
    }

    return;
}

/*
 * 函 数 名  : hwifi_config_nvram_second_coefficient_check
 * 功能描述  : 检查修正nvram中的二次系数是否合理
 */
OAL_STATIC uint32_t hwifi_config_nvram_second_coefficient_check(uint8_t *puc_g_cust_nvram_info,
                                                                uint8_t *puc_ini_pa_params,
                                                                uint32_t cfg_id,
                                                                int16_t *ps_5g_delt_power)
{
    int32_t l_ini_params[CUS_NUM_5G_BW * DY_CALI_PARAMS_TIMES] = {0};
    int32_t l_nv_params[CUS_NUM_5G_BW * DY_CALI_PARAMS_TIMES] = {0};
    uint16_t us_ini_param_num = 0;
    uint16_t us_nv_param_num = 0;
    uint8_t uc_secon_ratio_idx = 0;
    uint8_t uc_param_idx;

    /* 获取拟合系数项 */
    if (hwifi_config_sepa_coefficient_from_param(puc_g_cust_nvram_info, l_nv_params, &us_nv_param_num,
                                                 sizeof(l_nv_params) / (sizeof(int16_t))) != OAL_SUCC ||
        (us_nv_param_num % DY_CALI_PARAMS_TIMES) ||
        hwifi_config_sepa_coefficient_from_param(puc_ini_pa_params, l_ini_params, &us_ini_param_num,
                                                 sizeof(l_ini_params) / (sizeof(int16_t))) != OAL_SUCC ||
        (us_ini_param_num % DY_CALI_PARAMS_TIMES) || (us_nv_param_num != us_ini_param_num)) {
        oam_error_log2(0, OAM_SF_CUSTOM,
            "hwifi_config_nvram_second_coefficient_check::nvram or ini is unsuitable,num of nv and ini[%d %d]!",
            us_nv_param_num, us_ini_param_num);
        return OAL_FAIL;
    }

    us_nv_param_num /= DY_CALI_PARAMS_TIMES;
    /* 检查nv和ini中二次系数是否匹配 */
    for (uc_param_idx = 0; uc_param_idx < us_nv_param_num; uc_param_idx++) {
        if (uc_secon_ratio_idx >= (CUS_NUM_5G_BW * DY_CALI_PARAMS_TIMES)) {
            continue;
        }
        if (l_ini_params[uc_secon_ratio_idx] != l_nv_params[uc_secon_ratio_idx]) {
            oam_warning_log4(0, OAM_SF_CUSTOM, "hwifi_config_nvram_second_coefficient_check::nvram get mismatch value \
                idx[%d %d] val are [%d] and [%d]!", uc_param_idx, uc_secon_ratio_idx, l_ini_params[uc_secon_ratio_idx],
                l_nv_params[uc_secon_ratio_idx]);

            /* 量产后二次系数以nvram中为准，刷新NV中的二次拟合曲线切换点 */
            hwifi_config_get_5g_curv_switch_point(puc_g_cust_nvram_info, cfg_id);
            uc_secon_ratio_idx += DY_CALI_PARAMS_TIMES;
            continue;
        }

        if ((cfg_id == WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0) || (cfg_id == WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1)) {
            /* 计算产线上的delt power */
            *(ps_5g_delt_power + uc_param_idx) = hwifi_get_5g_pro_line_delt_pow_per_band(
                l_nv_params + (int32_t)uc_secon_ratio_idx, l_ini_params + (int32_t)uc_secon_ratio_idx);
            oal_io_print("hwifi_config_nvram_second_coefficient_check::delt power[%d] param_idx[%d] cfg_id[%d]!\r\n",
                         *(ps_5g_delt_power + uc_param_idx), uc_param_idx, cfg_id);
        }
        uc_secon_ratio_idx += DY_CALI_PARAMS_TIMES;
    }

    return OAL_SUCC;
}

static int32_t hwifi_config_get_cust_string(uint32_t cfg_id, uint8_t *nv_pa_params)
{
    uint32_t cfg_id_tmp;
    if (get_cust_conf_string(INI_MODU_WIFI, g_nvram_pro_line_config_ini[cfg_id].name,
                             nv_pa_params, CUS_PARAMS_LEN_MAX - 1) == INI_FAILED) {
        if (oal_value_eq_any4(cfg_id, WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_LOW,
                              WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_LOW,
                              WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_BAND1_LOW,
                              WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_BAND1_LOW)) {
            cfg_id_tmp = ((cfg_id == WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_BAND1_LOW) ?
                              WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_BAND1 :
                              (cfg_id == WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_LOW) ?
                              WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0 :
                              (cfg_id == WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_BAND1_LOW) ?
                              WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_BAND1 :
                              (cfg_id == WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_LOW) ?
                              WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1 : cfg_id);
            get_cust_conf_string(INI_MODU_WIFI, g_nvram_pro_line_config_ini[cfg_id_tmp].name,
                                 nv_pa_params, CUS_PARAMS_LEN_MAX - 1);
        } else {
            oam_error_log1(0, OAM_SF_CUSTOM, "hwifi_config_get_cust_string read, check id[%d] exists!", cfg_id);
            return OAL_FAIL;
        }
    }
    return OAL_SUCC;
}

static uint8_t hwifi_config_set_cali_param_2g_and_5g(int32_t *pl_params, int16_t *s_5g_delt_power)
{
    uint8_t rf_idx;
    uint8_t cali_param_idx;
    uint8_t delt_pwr_idx = 0;
    uint8_t idx = 0;
    for (rf_idx = 0; rf_idx < WLAN_RF_CHANNEL_NUMS; rf_idx++) {
        for (cali_param_idx = 0; cali_param_idx < DY_CALI_PARAMS_BASE_NUM; cali_param_idx++) {
            if (cali_param_idx == (DY_2G_CALI_PARAMS_NUM - 1)) {
                /* band1 & CW */
                cali_param_idx += PRO_LINE_2G_TO_5G_OFFSET;
            }
            g_pro_line_params[rf_idx][cali_param_idx].l_pow_par2 = pl_params[idx++];
            g_pro_line_params[rf_idx][cali_param_idx].l_pow_par1 = pl_params[idx++];
            g_pro_line_params[rf_idx][cali_param_idx].l_pow_par0 = pl_params[idx++];
        }
    }

    /* 5g band2&3 4&5 6 7 low power */
    for (rf_idx = 0; rf_idx < WLAN_RF_CHANNEL_NUMS; rf_idx++) {
        delt_pwr_idx = 0;
        for (cali_param_idx = DY_CALI_PARAMS_BASE_NUM + 1;
             cali_param_idx < DY_CALI_PARAMS_NUM - 1; cali_param_idx++) {
            g_pro_line_params[rf_idx][cali_param_idx].l_pow_par2 = pl_params[idx++];
            g_pro_line_params[rf_idx][cali_param_idx].l_pow_par1 = pl_params[idx++];
            g_pro_line_params[rf_idx][cali_param_idx].l_pow_par0 = pl_params[idx++];

            cus_flush_nv_ratio_by_delt_pow(g_pro_line_params[rf_idx][cali_param_idx].l_pow_par2,
                                           g_pro_line_params[rf_idx][cali_param_idx].l_pow_par1,
                                           g_pro_line_params[rf_idx][cali_param_idx].l_pow_par0,
                                           *(s_5g_delt_power + rf_idx * CUS_NUM_5G_BW + delt_pwr_idx));
            delt_pwr_idx++;
        }
    }
    return idx;
}

static void hwifi_config_set_cali_param_left_num(int32_t *pl_params, uint8_t idx)
{
    uint8_t rf_idx;
    /* band1 & CW */
    for (rf_idx = 0; rf_idx < WLAN_RF_CHANNEL_NUMS; rf_idx++) {
        g_pro_line_params[rf_idx][DY_2G_CALI_PARAMS_NUM].l_pow_par2 = pl_params[idx++];
        g_pro_line_params[rf_idx][DY_2G_CALI_PARAMS_NUM].l_pow_par1 = pl_params[idx++];
        g_pro_line_params[rf_idx][DY_2G_CALI_PARAMS_NUM].l_pow_par0 = pl_params[idx++];
    }
    for (rf_idx = 0; rf_idx < WLAN_RF_CHANNEL_NUMS; rf_idx++) {
        g_pro_line_params[rf_idx][DY_2G_CALI_PARAMS_NUM - 1].l_pow_par2 = pl_params[idx++];
        g_pro_line_params[rf_idx][DY_2G_CALI_PARAMS_NUM - 1].l_pow_par1 = pl_params[idx++];
        g_pro_line_params[rf_idx][DY_2G_CALI_PARAMS_NUM - 1].l_pow_par0 = pl_params[idx++];
    }
    for (rf_idx = 0; rf_idx < WLAN_RF_CHANNEL_NUMS; rf_idx++) {
        /* 5g band1 low power */
        /* band1产线不校准 */
        g_pro_line_params[rf_idx][DY_CALI_PARAMS_BASE_NUM].l_pow_par2 = pl_params[idx++];
        g_pro_line_params[rf_idx][DY_CALI_PARAMS_BASE_NUM].l_pow_par1 = pl_params[idx++];
        g_pro_line_params[rf_idx][DY_CALI_PARAMS_BASE_NUM].l_pow_par0 = pl_params[idx++];
    }

    for (rf_idx = 0; rf_idx < WLAN_RF_CHANNEL_NUMS; rf_idx++) {
        /* 2g cw ppa */
        g_pro_line_params[rf_idx][DY_CALI_PARAMS_NUM - 1].l_pow_par2 = pl_params[idx++];
        g_pro_line_params[rf_idx][DY_CALI_PARAMS_NUM - 1].l_pow_par1 = pl_params[idx++];
        g_pro_line_params[rf_idx][DY_CALI_PARAMS_NUM - 1].l_pow_par0 = pl_params[idx++];
    }
}

static uint32_t hwifi_nvram_param_check_second_coefficient(uint8_t *nv_pa_params, uint8_t *cust_nvram_info,
                                                           int16_t *delt_power_5g, uint32_t cfg_id)
{
    /* 先取nv中的参数值,为空则从ini文件中读取 */
    if (OAL_STRLEN(cust_nvram_info)) {
        /* NVRAM二次系数异常保护 */
        if (hwifi_config_nvram_second_coefficient_check(cust_nvram_info, nv_pa_params, cfg_id, delt_power_5g +
            (cfg_id < WLAN_CFG_DTS_NVRAM_RATIO_PA2GCCKA1 ? WLAN_RF_CHANNEL_ZERO : WLAN_RF_CHANNEL_ONE)) == OAL_SUCC) {
            /* 手机如果low part为空,则取ini中的系数,并根据产测结果修正;否则直接从nvram中取得 */
            if ((cfg_id == WLAN_CFG_DTS_NVRAM_RATIO_PA5GA0_LOW) &&
                (oal_memcmp(cust_nvram_info, nv_pa_params, OAL_STRLEN(cust_nvram_info)))) {
                memset_s(delt_power_5g + WLAN_RF_CHANNEL_ZERO, CUS_NUM_5G_BW * sizeof(int16_t),
                         0, CUS_NUM_5G_BW * sizeof(int16_t));
            }
            if ((cfg_id == WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_LOW) &&
                (oal_memcmp(cust_nvram_info, nv_pa_params, OAL_STRLEN(cust_nvram_info)))) {
                memset_s(delt_power_5g + WLAN_RF_CHANNEL_ONE, CUS_NUM_5G_BW * sizeof(int16_t),
                         0, CUS_NUM_5G_BW * sizeof(int16_t));
            }

            if (memcpy_s(nv_pa_params, CUS_PARAMS_LEN_MAX * sizeof(uint8_t),
                         cust_nvram_info, OAL_STRLEN(cust_nvram_info)) != EOK) {
                return OAL_FAIL;
            }
        } else {
            return OAL_FAIL;
        }
    } else {
        /* 提供产线第一次上电校准初始值 */
        if (memcpy_s(cust_nvram_info, CUS_PARAMS_LEN_MAX,
                     nv_pa_params, OAL_STRLEN(nv_pa_params)) != EOK) {
            return OAL_FAIL;
        }
    }
    return OAL_SUCC;
}

static uint32_t hwifi_coefficient_check_and_set(uint8_t **cust_nvram_info, uint8_t *nv_pa_params,
                                                int32_t *params, int16_t *delt_power_5g,
                                                uint16_t *param_num)
{
    uint32_t cfg_id;
    uint16_t per_param_num = 0;
    uint16_t param_len = WLAN_RF_CHANNEL_NUMS * DY_CALI_PARAMS_TIMES * DY_CALI_PARAMS_NUM * sizeof(int32_t);
    for (cfg_id = WLAN_CFG_DTS_NVRAM_RATIO_PA2GCCKA0; cfg_id < WLAN_CFG_DTS_NVRAM_PARAMS_BUTT; cfg_id++) {
        /* 二次拟合系数 */
        if ((cfg_id >= WLAN_CFG_DTS_NVRAM_MUFREQ_2GCCK_C0) && (cfg_id < WLAN_CFG_DTS_NVRAM_END)) {
            /* DPN */
            continue;
        }

        if (hwifi_config_get_cust_string(cfg_id, nv_pa_params) != OAL_SUCC) {
            return OAL_FAIL;
        }

        /* 获取ini中的二次拟合曲线切换点 */
        hwifi_config_get_5g_curv_switch_point(nv_pa_params, cfg_id);

        if (cfg_id <= WLAN_CFG_DTS_NVRAM_RATIO_PA5GA1_LOW) {
            *cust_nvram_info = hwifi_get_nvram_param(cfg_id);
            if (hwifi_nvram_param_check_second_coefficient(nv_pa_params, *cust_nvram_info,
                                                           delt_power_5g, cfg_id) != OAL_SUCC) {
                return OAL_FAIL;
            }
        }

        if (hwifi_config_sepa_coefficient_from_param(nv_pa_params, params + *param_num,
                                                     &per_param_num, param_len - *param_num) != OAL_SUCC ||
                                                     (per_param_num % DY_CALI_PARAMS_TIMES)) {
            oam_error_log3(0, OAM_SF_CUSTOM,
                "hwifi_coefficient_check_and_set read get wrong value,len[%d] check id[%d] exists per_param_num[%d]!",
                OAL_STRLEN(*cust_nvram_info), cfg_id, per_param_num);
            return OAL_FAIL;
        }
        *param_num += per_param_num;
    }
    return OAL_SUCC;
}

/*
 * 函 数 名  : hwifi_config_init_dy_cali_custom
 * 功能描述  : 获取定制化文件和二次产测系数
 */
OAL_STATIC uint32_t hwifi_config_init_dy_cali_custom(void)
{
    uint32_t ret;
    uint8_t uc_idx = 0;
    uint16_t us_param_num = 0;
    int16_t s_5g_delt_power[WLAN_RF_CHANNEL_NUMS][CUS_NUM_5G_BW] = {{0}};
    uint8_t *puc_g_cust_nvram_info = NULL;
    uint8_t *puc_nv_pa_params = NULL;
    int32_t *pl_params = NULL;
    uint16_t us_param_len = WLAN_RF_CHANNEL_NUMS * DY_CALI_PARAMS_TIMES *
                              DY_CALI_PARAMS_NUM * sizeof(int32_t);

    puc_nv_pa_params = (uint8_t *)os_kzalloc_gfp(CUS_PARAMS_LEN_MAX * sizeof(uint8_t));
    if (puc_nv_pa_params == NULL) {
        oam_error_log0(0, OAM_SF_CUSTOM, "hwifi_config_init_dy_cali_custom::puc_nv_pa_params mem alloc fail!");
        return OAL_FAIL;
    }

    pl_params = (int32_t *)os_kzalloc_gfp(us_param_len);
    if (pl_params == NULL) {
        oam_error_log0(0, OAM_SF_CUSTOM, "hwifi_config_init_dy_cali_custom::ps_params mem alloc fail!");
        os_mem_kfree(puc_nv_pa_params);
        return OAL_FAIL;
    }

    memset_s(puc_nv_pa_params,
             CUS_PARAMS_LEN_MAX * sizeof(uint8_t),
             0,
             CUS_PARAMS_LEN_MAX * sizeof(uint8_t));
    memset_s(pl_params, us_param_len, 0, us_param_len);

    ret = hwifi_coefficient_check_and_set(&puc_g_cust_nvram_info, puc_nv_pa_params, pl_params,
                                          (int16_t *)s_5g_delt_power, &us_param_num);

    os_mem_kfree(puc_nv_pa_params);

    if (ret == OAL_FAIL) {
        /* 置零防止下发到device */
        memset_s(g_pro_line_params, sizeof(g_pro_line_params), 0, sizeof(g_pro_line_params));
    } else {
        if (us_param_num != us_param_len / sizeof(int32_t)) {
            oam_error_log1(0, OAM_SF_CUSTOM,
                           "hwifi_config_init_dy_cali_custom read get wrong ini value num[%d]!", us_param_num);
            memset_s(g_pro_line_params, sizeof(g_pro_line_params), 0, sizeof(g_pro_line_params));
            os_mem_kfree(pl_params);
            return OAL_FAIL;
        }

        uc_idx = hwifi_config_set_cali_param_2g_and_5g(pl_params, (int16_t *)s_5g_delt_power);

        hwifi_config_set_cali_param_left_num(pl_params, uc_idx);
    }

    os_mem_kfree(pl_params);
    return ret;
}

/*
 * 函 数 名  : hwifi_config_init
 * 功能描述  : netdev open 调用的定制化总入口
 *             读取ini文件，更新 g_host_init_params 全局数组
 */
int32_t hwifi_config_init(int32_t cus_tag)
{
    int32_t l_cfg_id;
    int32_t l_ret = INI_FAILED;
    int32_t l_ori_val;
    wlan_cfg_cmd *pgast_wifi_config = NULL;
    int32_t *pgal_params = NULL;
    int32_t l_cfg_value = 0;
    int32_t l_wlan_cfg_butt;

    switch (cus_tag) {
        case CUS_TAG_NV:
            original_value_for_nvram_params();
            return hwifi_config_init_nvram();
        case CUS_TAG_INI:
            host_params_init_first();
            pgast_wifi_config = hwifi_get_g_wifi_config_cmds();
            pgal_params = hwifi_get_g_host_init_params();
            l_wlan_cfg_butt = WLAN_CFG_INIT_BUTT;
            break;
        case CUS_TAG_DTS:
            original_value_for_dts_params();
            pgast_wifi_config = hwifi_get_g_wifi_config_dts();
            pgal_params = hwifi_get_g_dts_params();
            l_wlan_cfg_butt = WLAN_CFG_DTS_BUTT;
            break;
        case CUS_TAG_PRIV_INI:
            return hwifi_config_init_private_custom();
        case CUS_TAG_PRO_LINE_INI:
            return hwifi_config_init_dy_cali_custom();
        default:
            oam_error_log1(0, OAM_SF_CUSTOM, "hwifi_config_init tag number[0x%x] not correct!", cus_tag);
            return INI_FAILED;
    }

    for (l_cfg_id = 0; l_cfg_id < l_wlan_cfg_butt; l_cfg_id++) {
        /* 获取ini的配置值 */
        l_ret = get_cust_conf_int32(INI_MODU_WIFI, pgast_wifi_config[l_cfg_id].name, &l_cfg_value);
        if (l_ret == INI_FAILED) {
            oam_info_log2(0, OAM_SF_CUSTOM, "hwifi_config_init read ini file cfg_id[%d]tag[%d] not exist!",
                          l_cfg_id, cus_tag);
            continue;
        }
        l_ori_val = pgal_params[pgast_wifi_config[l_cfg_id].case_entry];
        pgal_params[pgast_wifi_config[l_cfg_id].case_entry] = l_cfg_value;
    }

    return INI_SUCC;
}

#ifndef _PRE_PRODUCT_HI3751V811
/*
 * 函 数 名  : char2byte
 * 功能描述  : 统计值，判断有无读取到mac地址
 */
OAL_STATIC uint32_t char2byte(const char *strori, char *outbuf)
{
    uint32_t i = 0;
    uint8_t temp = 0;
    uint32_t sum = 0;
    uint8_t *ptr_out = (uint8_t *)outbuf;
    const int l_loop_times = 12; /* 单字节遍历是不是正确的mac地址:xx:xx:xx:xx:xx:xx */

    for (i = 0; i < l_loop_times; i++) {
        if (isdigit(strori[i])) {
            temp = strori[i] - '0';
        } else if (islower(strori[i])) {
            temp = (strori[i] - 'a') + 10; /* 加10为了保证'a'~'f'分别对应10~15 */
        } else if (isupper(strori[i])) {
            temp = (strori[i] - 'A') + 10; /* 加10为了保证'A'~'F'分别对应10~15 */
        }
        sum += temp;
        /* 为了组成正确的mac地址:xx:xx:xx:xx:xx:xx */
        if (i % 2 == 0) { /* 对2取余 */
            ptr_out[i / 2] |= (temp << BIT_OFFSET_4); /* 除以2 */
        } else {
            ptr_out[i / 2] |= temp; /* 除以2 */
        }
    }

    return sum;
}
#endif


#ifdef _PRE_PRODUCT_HI3751V811
#define WIFI_2G_MAC_TYPE    (1)
#define WIFI_5G_MAC_TYPE    (2)
#define WIFI_P2P_MAC_TYPE   (3)
#define BT_MAC_TYPE         (4)
#endif

#if defined(_PRE_CONFIG_READ_DYNCALI_E2PROM) && defined(_PRE_CONFIG_READ_E2PROM_MAC)
/*
 * 函 数 名  : hwifi_get_mac_addr_drveprom
 * 功能描述  : 从e2prom中获取mac地址，用宏_PRE_CONFIG_READ_E2PROM_MACADDR区分PC和大屏
 */
int32_t hwifi_get_mac_addr_drveprom(uint8_t *puc_buf)
{
#define READ_CHECK_MAX_CNT    20
#define READ_CHECK_WAIT_TIME  50

    char original_mac_addr[NUM_12_BYTES] = {0};
    int32_t ret = INI_FAILED;
    int32_t i;
    unsigned int offset = 0;
    unsigned int bit_len = 12;
    uint32_t sum = 0;

    if (g_wifi_mac[MAC_ADDR_0] != 0 || g_wifi_mac[MAC_ADDR_1]  != 0 || g_wifi_mac[MAC_ADDR_2]  != 0 ||
        g_wifi_mac[MAC_ADDR_3] != 0 || g_wifi_mac[MAC_ADDR_4] != 0 || g_wifi_mac[MAC_ADDR_5] != 0) {
        if (memcpy_s(puc_buf, WLAN_MAC_ADDR_LEN, g_wifi_mac, WLAN_MAC_ADDR_LEN) != EOK) {
            return INI_FAILED;
        }

        ini_warning("hwifi_get_mac_addr_drveprom get MAC from g_wifi_mac SUCC\n");
        return INI_SUCC;
    }

    for (i = 0; i < READ_CHECK_MAX_CNT; i++) {
        if (ini_eeprom_read("MACWLAN", offset, original_mac_addr, bit_len) == INI_SUCC) {
            ini_warning("hwifi_get_mac_addr_drveprom get MAC from EEPROM SUCC\n");
            ret = INI_SUCC;
            break;
        }

        msleep(READ_CHECK_WAIT_TIME);
    }

    if (ret == INI_SUCC) {
        oal_io_print("hwifi_get_mac_addr_drveprom ini_eeprom_read return success\n");
        sum = char2byte(original_mac_addr, (char *)puc_buf);
        if (sum != 0) {
            ini_warning("hwifi_get_mac_addr_drveprom get MAC from EEPROM: mac=" MACFMT "\n", ini_mac2str(puc_buf));
            if (memcpy_s(g_wifi_mac, WLAN_MAC_ADDR_LEN, puc_buf, WLAN_MAC_ADDR_LEN) != EOK) {
                ini_warning("hwifi_get_mac_addr_drveprom memcpy_s g_wifi_mac fail\n");
            }
        } else {
            ini_warning("hwifi_get_mac_addr_drveprom get MAC from EEPROM is not char,use random mac\n");
            random_ether_addr(puc_buf);
            puc_buf[1] = 0x11;
            puc_buf[BYTE_OFFSET_2] = 0x03;
        }
    } else {
        oal_io_print("hwifi_get_mac_addr ini_eeprom_read return fail,use random mac\n");
        chr_exception_report(CHR_READ_EEPROM_ERROR_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_INIT, CHR_PLAT_DRV_ERROR_EEPROM_READ_INIT);
        random_ether_addr(puc_buf);
        puc_buf[1] = 0x11;
        puc_buf[BYTE_OFFSET_2] = 0x03;
    }

    return ret;
}
#endif

/*
 * 函 数 名  : hwifi_get_mac_addr
 * 功能描述  : 从e2prom中获取mac地址，用宏_PRE_CONFIG_READ_E2PROM_MACADDR区分PC和大屏
 */
#if defined(_PRE_CONFIG_READ_DYNCALI_E2PROM) && (!defined(_PRE_CONFIG_READ_E2PROM_MAC))
int32_t hwifi_get_mac_addr(uint8_t *puc_buf)
{
    if (puc_buf == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "hwifi_get_mac_addr::buf is NULL!");
        return INI_FAILED;
    }

    if (get_board_mac(puc_buf, WIFI_2G_MAC_TYPE) == 0) {
        ini_warning("hwifi_get_mac_addr get MAC from NV: mac="MACFMT"\n", ini_mac2str(puc_buf));
        if (memcpy_s(g_wifi_mac, WLAN_MAC_ADDR_LEN, puc_buf, WLAN_MAC_ADDR_LEN) != EOK) {
            oam_error_log0(0, OAM_SF_ANY, "hwifi_get_mac_addr::memcpy_s mac to g_wifi_mac failed");
        }
    } else {
        random_ether_addr(puc_buf);
        puc_buf[1] = 0x11;
        puc_buf[BYTE_OFFSET_2] = 0x02;
        ini_warning("hwifi_get_mac_addr get random mac: mac="MACFMT"\n", ini_mac2str(puc_buf));
    }
    return INI_SUCC;
}

#elif defined(_PRE_CONFIG_READ_DYNCALI_E2PROM) && defined(_PRE_CONFIG_READ_E2PROM_MAC)
int32_t hwifi_get_mac_addr(uint8_t *puc_buf)
{
    return hwifi_get_mac_addr_drveprom(puc_buf);
}

#else
int32_t hwifi_get_mac_addr(uint8_t *puc_buf)
{
    struct external_nve_info_user st_info;
    int32_t l_ret;
    uint32_t sum = 0;

    if (puc_buf == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "hwifi_get_mac_addr::buf is NULL!");
        return INI_FAILED;
    }

    memset_s(puc_buf, WLAN_MAC_ADDR_LEN, 0, WLAN_MAC_ADDR_LEN);

    memset_s(&st_info, sizeof(st_info), 0, sizeof(st_info));

    st_info.nv_number = NV_WLAN_NUM;  // nve item

    if (strcpy_s(st_info.nv_name, sizeof(st_info.nv_name), "MACWLAN") != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "hwifi_get_mac_addr:: strcpy_s failed.");
        return INI_FAILED;
    }

    st_info.valid_size = NV_WLAN_VALID_SIZE;
    st_info.nv_operation = NV_READ;

    if (g_wifi_mac[0] != 0 || g_wifi_mac[1]  != 0 || g_wifi_mac[BYTE_OFFSET_2]  != 0 || g_wifi_mac[BYTE_OFFSET_3] != 0
        || g_wifi_mac[BYTE_OFFSET_4] != 0 || g_wifi_mac[BYTE_OFFSET_5] != 0) {
        if (memcpy_s(puc_buf, WLAN_MAC_ADDR_LEN, g_wifi_mac, WLAN_MAC_ADDR_LEN) != EOK) {
            return INI_FAILED;
        }

        return INI_SUCC;
    }

    l_ret = external_nve_direct_access_interface(&st_info);
    if (!l_ret) {
        sum = char2byte(st_info.nv_data, (char *)puc_buf);
        if (sum != 0) {
            ini_warning("hwifi_get_mac_addr get MAC from NV: mac=" MACFMT "\n", ini_mac2str(puc_buf));
            if (memcpy_s(g_wifi_mac, WLAN_MAC_ADDR_LEN, puc_buf, WLAN_MAC_ADDR_LEN) != EOK) {
                return INI_FAILED;
            }
        } else {
            random_ether_addr(puc_buf);
            puc_buf[BYTE_OFFSET_1] = 0x11;
            puc_buf[BYTE_OFFSET_2] = 0x02;
        }
    } else {
        random_ether_addr(puc_buf);
        puc_buf[BYTE_OFFSET_1] = 0x11;
        puc_buf[BYTE_OFFSET_2] = 0x02;
    }

    return INI_SUCC;
}
#endif
/*
 * 函 数 名  : hwifi_get_init_nvram_params
 * 功能描述  : 获取定制化NV数据结构体
 */
oal_bool_enum_uint8 hwifi_get_g_fact_cali_completed(void)
{
    return g_en_fact_cali_completed;
}

int32_t hwifi_get_init_value(int32_t cus_tag, int32_t cfg_id)
{
    int32_t *pgal_params = NULL;
    int32_t l_wlan_cfg_butt;

    if (cus_tag == CUS_TAG_INI) {
        pgal_params = hwifi_get_g_host_init_params();
        l_wlan_cfg_butt = WLAN_CFG_INIT_BUTT;
    } else if (cus_tag == CUS_TAG_DTS) {
        pgal_params = hwifi_get_g_dts_params();
        l_wlan_cfg_butt = WLAN_CFG_DTS_BUTT;
    } else {
        oam_error_log1(0, OAM_SF_ANY, "hwifi_get_init_value tag number[0x%2x] not correct!", cus_tag);
        return INI_FAILED;
    }

    if (cfg_id < 0 || l_wlan_cfg_butt <= cfg_id) {
        oam_error_log2(0, OAM_SF_ANY, "hwifi_get_init_value cfg id[%d] out of range, max cfg id is:%d",
                       cfg_id, (l_wlan_cfg_butt - 1));
        return INI_FAILED;
    }

    return pgal_params[cfg_id];
}

/*
 * 函 数 名  : hwifi_get_cfg_pow_ctrl_params
 * 功能描述  : host查看ini定制化参数维测命令
 */
OAL_STATIC void hwifi_get_cfg_pow_ctrl_params(uint8_t uc_chn_idx)
{
    int32_t l_cfg_idx_one = 0;
    int32_t l_cfg_idx_two = 0;
    wlan_cust_nvram_params *pst_g_cust_nv_params = hwifi_get_nvram_params();
    wlan_cust_cfg_custom_fcc_ce_txpwr_limit_stru *pst_fcc_ce_param;

    pst_fcc_ce_param = &pst_g_cust_nv_params->ast_fcc_ce_param[uc_chn_idx];
    for (l_cfg_idx_one = 0; l_cfg_idx_one < CUS_NUM_5G_20M_SIDE_BAND; l_cfg_idx_one++) {
        oal_io_print("%s[%d] \t [config:%d]\n", "fcc_txpwr_limit_5g:20M side_band", l_cfg_idx_one,
                     pst_fcc_ce_param->auc_5g_fcc_txpwr_limit_params_20m[l_cfg_idx_one]);
    }

    for (l_cfg_idx_one = 0; l_cfg_idx_one < CUS_NUM_5G_40M_SIDE_BAND; l_cfg_idx_one++) {
        oal_io_print("%s[%d] \t [config:%d]\n", "fcc_txpwr_limit_5g:40M side_band", l_cfg_idx_one,
                     pst_fcc_ce_param->auc_5g_fcc_txpwr_limit_params_40m[l_cfg_idx_one]);
    }
    for (l_cfg_idx_one = 0; l_cfg_idx_one < CUS_NUM_5G_80M_SIDE_BAND; l_cfg_idx_one++) {
        oal_io_print("%s[%d] \t [config:%d]\n", "fcc_txpwr_limit_5g:80M side_band", l_cfg_idx_one,
                     pst_fcc_ce_param->auc_5g_fcc_txpwr_limit_params_80m[l_cfg_idx_one]);
    }
    for (l_cfg_idx_one = 0; l_cfg_idx_one < CUS_NUM_5G_160M_SIDE_BAND; l_cfg_idx_one++) {
        oal_io_print("%s[%d] \t [config:%d]\n", "fcc_txpwr_limit_5g:160M side_band", l_cfg_idx_one,
                     pst_fcc_ce_param->auc_5g_fcc_txpwr_limit_params_160m[l_cfg_idx_one]);
    }
    for (l_cfg_idx_one = 0; l_cfg_idx_one < MAC_2G_CHANNEL_NUM; l_cfg_idx_one++) {
        for (l_cfg_idx_two = 0; l_cfg_idx_two < CUS_NUM_FCC_CE_2G_PRO; l_cfg_idx_two++) {
            oal_io_print("%s[%d] [%d] \t [config:%d]\n", "fcc_txpwr_limit_2g: chan", l_cfg_idx_one, l_cfg_idx_two,
                         pst_fcc_ce_param->auc_2g_fcc_txpwr_limit_params[l_cfg_idx_one][l_cfg_idx_two]);
        }
    }
}

/*
 * 函 数 名  : hwifi_get_cfg_iq_lpf_lvl_params
 * 功能描述  : host查看ini定制化参数维测命令
 */
OAL_STATIC void hwifi_get_cfg_iq_lpf_lvl_params(void)
{
    int32_t l_cfg_idx_one = 0;
    wlan_cust_nvram_params *pst_g_cust_nv_params = hwifi_get_nvram_params();

    for (l_cfg_idx_one = 0; l_cfg_idx_one < NUM_OF_NV_5G_LPF_LVL; ++l_cfg_idx_one) {
        oal_io_print("%s%d \t [config:%d]\n", "5g_iq_cali_lpf_lvl", l_cfg_idx_one,
                     pst_g_cust_nv_params->auc_5g_iq_cali_lpf_params[l_cfg_idx_one]);
    }
}

OAL_STATIC void hwifi_get_cfg_ini_params(void)
{
    int32_t cfg_idx_one;
    int32_t *host_init_params = hwifi_get_g_host_init_params();
    wlan_cfg_cmd *wifi_config_cmds = hwifi_get_g_wifi_config_cmds();

    for (cfg_idx_one = 0; cfg_idx_one < WLAN_CFG_INIT_BUTT; ++cfg_idx_one) {
        oal_io_print("%s \t [config:0x%x]\n", wifi_config_cmds[cfg_idx_one].name,
            host_init_params[cfg_idx_one]);
    }
}

OAL_STATIC void hwifi_get_cfg_txpwr_params(void)
{
    int32_t cfg_idx_one;
    int32_t cfg_idx_two;
    wlan_cfg_cmd *nvram_config_ini = hwifi_get_nvram_config();
    wlan_cust_nvram_params *cust_nv_params = hwifi_get_nvram_params(); /* 最大发送功率定制化数组 */

    for (cfg_idx_one = 0; cfg_idx_one < NUM_OF_NV_MAX_TXPOWER; ++cfg_idx_one) {
        oal_io_print("%s%d \t [config:%d]\n", "delt_txpwr_params", cfg_idx_one,
            cust_nv_params->ac_delt_txpwr_params[cfg_idx_one]);
    }

    for (cfg_idx_one = 0; cfg_idx_one < NUM_OF_NV_DPD_MAX_TXPOWER; ++cfg_idx_one) {
        oal_io_print("%s%d \t [config:%d]\n", "delt_dpd_txpwr_params",
            cfg_idx_one, cust_nv_params->ac_dpd_delt_txpwr_params[cfg_idx_one]);
    }

    for (cfg_idx_one = 0; cfg_idx_one < NUM_OF_NV_11B_DELTA_TXPOWER; ++cfg_idx_one) {
        oal_io_print("%s%d \t [config:%d]\n", "delt_11b_txpwr_params", cfg_idx_one,
            cust_nv_params->ac_11b_delt_txpwr_params[cfg_idx_one]);
    }

    for (cfg_idx_one = 0; cfg_idx_one < WLAN_RF_CHANNEL_NUMS; ++cfg_idx_one) {
        oal_io_print("%s%d \t [config:%d]\n", "5g_IQ_cali_pow", cfg_idx_one,
                     cust_nv_params->auc_fem_off_iq_cal_pow_params[cfg_idx_one]);
    }

    for (cfg_idx_one = 0; cfg_idx_one < NUM_OF_NV_5G_UPPER_UPC; ++cfg_idx_one) {
        oal_io_print("%s%d \t [config:%d]\n", "5g_upper_upc_params", cfg_idx_one,
            cust_nv_params->auc_5g_upper_upc_params[cfg_idx_one]);
    }

    for (cfg_idx_one = 0; cfg_idx_one < NUM_OF_NV_2G_LOW_POW_DELTA_VAL; ++cfg_idx_one) {
        oal_io_print("%s%d \t [config:%d]\n", "2g_low_pow_amend_val", cfg_idx_one,
            cust_nv_params->ac_2g_low_pow_amend_params[cfg_idx_one]);
    }

    for (cfg_idx_one = 0; cfg_idx_one < WLAN_RF_CHANNEL_NUMS; cfg_idx_one++) {
        for (cfg_idx_two = 0; cfg_idx_two < CUS_BASE_PWR_NUM_2G; cfg_idx_two++) {
            oal_io_print("%s[%d][%d] \t [config:%d]\n", "2G base_pwr_params", cfg_idx_one,
                cfg_idx_two, cust_nv_params->auc_2g_txpwr_base_params[cfg_idx_one][cfg_idx_two]);
        }
        for (cfg_idx_two = 0; cfg_idx_two < CUS_BASE_PWR_NUM_5G; cfg_idx_two++) {
            oal_io_print("%s[%d][%d] \t [config:%d]\n", "5G base_pwr_params", cfg_idx_one,
                cfg_idx_two, cust_nv_params->auc_5g_txpwr_base_params[cfg_idx_one][cfg_idx_two]);
        }
    }
    oal_io_print("%s \t [config:%d]\n", nvram_config_ini[NVRAM_PARAMS_5G_FCC_CE_HIGH_BAND_MAX_PWR].name,
        cust_nv_params->uc_5g_max_pwr_fcc_ce_for_high_band);
}

OAL_STATIC void hwifi_get_cfg_fcc_ce_params(void)
{
    int32_t cfg_idx_one;

    for (cfg_idx_one = 0; cfg_idx_one < WLAN_RF_CHANNEL_NUMS; cfg_idx_one++) {
        /* FCC/CE */
        oal_io_print("%s \t [RF:%d]\n", "hwifi_get_cfg_pow_ctrl_params", cfg_idx_one);
        hwifi_get_cfg_pow_ctrl_params(cfg_idx_one);
    }
}

OAL_STATIC void hwifi_get_cfg_sar_params(void)
{
    int32_t cfg_idx_one;
    int32_t cfg_idx_two;
    wlan_init_cust_nvram_params *init_g_cust_nv_params = hwifi_get_init_nvram_params();

    for (cfg_idx_one = 0; cfg_idx_one < CUS_NUM_OF_SAR_LVL; cfg_idx_one++) {
        for (cfg_idx_two = 0; cfg_idx_two < CUS_NUM_OF_SAR_PARAMS; cfg_idx_two++) {
            oal_io_print("%s[%d][%d] \t [config:C0 %d C1 %d]\n", "sar_ctrl_params: lvl", cfg_idx_one, cfg_idx_two,
                init_g_cust_nv_params->st_sar_ctrl_params[cfg_idx_one][cfg_idx_two].auc_sar_ctrl_params_c0,
                init_g_cust_nv_params->st_sar_ctrl_params[cfg_idx_one][cfg_idx_two].auc_sar_ctrl_params_c1);
        }
    }
}

#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
OAL_STATIC void hwifi_get_cfg_tas_params(void)
{
    int32_t cfg_idx_one;
    int32_t cfg_idx_two;
    wlan_cust_nvram_params *cust_nv_params = hwifi_get_nvram_params(); /* 最大发送功率定制化数组 */

    for (cfg_idx_one = 0; cfg_idx_one < WLAN_RF_CHANNEL_NUMS; cfg_idx_one++) {
        for (cfg_idx_two = 0; cfg_idx_two < WLAN_BAND_BUTT; cfg_idx_two++) {
            oal_io_print("%s[%d][%d] \t [config:%d]\n", "tas_ctrl_params: lvl", cfg_idx_one, cfg_idx_two,
                cust_nv_params->auc_tas_ctrl_params[cfg_idx_one][cfg_idx_two]);
        }
    }
}
#endif

OAL_STATIC void hwifi_get_cfg_dts_params(void)
{
    int32_t cfg_idx_one;
    wlan_cfg_cmd *wifi_config_dts_func = hwifi_get_g_wifi_config_dts();
    int32_t *dts_params = hwifi_get_g_dts_params();

    for (cfg_idx_one = 0; cfg_idx_one < WLAN_CFG_DTS_BUTT; ++cfg_idx_one) {
        oal_io_print("%s \t [config:0x%x]\n", wifi_config_dts_func[cfg_idx_one].name, dts_params[cfg_idx_one]);
    }
}

OAL_STATIC void hwifi_get_cfg_pro_line_params(void)
{
    int32_t cfg_idx_one;
    int32_t cfg_idx_two;

    for (cfg_idx_one = 0; cfg_idx_one < WLAN_RF_CHANNEL_NUMS; cfg_idx_one++) {
        for (cfg_idx_two = 0; cfg_idx_two < DY_CALI_PARAMS_NUM; cfg_idx_two++) {
            oal_io_print("%s CORE[%d]para_idx[%d]::{%d, %d, %d}\n", "g_pro_line_params: ",
                cfg_idx_one, cfg_idx_two, g_pro_line_params[cfg_idx_one][cfg_idx_two].l_pow_par2,
                g_pro_line_params[cfg_idx_one][cfg_idx_two].l_pow_par1,
                g_pro_line_params[cfg_idx_one][cfg_idx_two].l_pow_par0);
        }
    }

    /* NVRAM */
    oal_io_print("%s : { %d }\n", "en_nv_dp_init_is_null: ", g_en_nv_dp_init_is_null);
    for (cfg_idx_one = 0; cfg_idx_one < WLAN_CFG_DTS_NVRAM_END; cfg_idx_one++) {
        oal_io_print("%s para_idx[%d] name[%s]::DATA{%s}\n", "dp init & ratios nvram_param: ", cfg_idx_one,
            g_wifi_nvram_cfg_handler[cfg_idx_one].puc_param_name,
            hwifi_get_nvram_param(cfg_idx_one));
    }
}

/*
 * 函 数 名  : hwifi_get_cfg_params
 * 功能描述  : host查看ini定制化参数维测命令
 */
void hwifi_get_cfg_params(void)
{
    oal_io_print("\nhwifi_get_cfg_params\n");

    /* CUS_TAG_INI */
    hwifi_get_cfg_ini_params();

    /* CUS_TAG_TXPWR */
    hwifi_get_cfg_txpwr_params();

    /* FCC CE */
    hwifi_get_cfg_fcc_ce_params();

    /* SAR */
    hwifi_get_cfg_sar_params();

#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
    hwifi_get_cfg_tas_params();
#endif

    /* CUS_TAG_DTS */
    hwifi_get_cfg_dts_params();

    /* iq lvl */
    hwifi_get_cfg_iq_lpf_lvl_params();

    /* pro line */
    hwifi_get_cfg_pro_line_params();

    /* RU DELT POW  */
    hwifi_get_cfg_delt_ru_pow_params();
}


uint32_t hwifi_config_init_nvram_main_1103(oal_net_device_stru *pst_cfg_net_dev)
{
    wal_msg_write_stru st_write_msg;
    int32_t l_ret;
    uint16_t us_offset = sizeof(wlan_cust_nvram_params); /* 包括4个基准功率 */

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_CUS_NVRAM_PARAM, us_offset);
    l_ret = memcpy_s(st_write_msg.auc_value, sizeof(st_write_msg.auc_value), hwifi_get_nvram_params(), us_offset);
    if (l_ret != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "hwifi_config_init_nvram_main_1103::memcpy fail!");
        return OAL_FAIL;
    }

    l_ret = wal_send_cfg_event(pst_cfg_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + us_offset,
        (uint8_t *)&st_write_msg, OAL_FALSE, NULL);
    if (oal_unlikely(l_ret != OAL_SUCC)) {
        oam_error_log1(0, OAM_SF_ANY, "{hwifi_config_init_nvram_main_1103::err [%d]!}", l_ret);
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


void hwifi_config_cpu_freq_ini_param_1103(void)
{
    int32_t l_val;

#ifdef _PRE_FEATURE_PLAT_LOCK_CPUFREQ
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_PRIV_DMA_LATENCY);
    g_freq_lock_control.uc_lock_dma_latency = (l_val > 0) ? OAL_TRUE : OAL_FALSE;
    g_freq_lock_control.dma_latency_value = (uint16_t)l_val;
    oal_io_print("DMA latency[%d]\r\n", g_freq_lock_control.uc_lock_dma_latency);
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_PRIV_LOCK_CPU_TH_HIGH);
    g_freq_lock_control.us_lock_cpu_th_high = (uint16_t)l_val;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_PRIV_LOCK_CPU_TH_LOW);
    g_freq_lock_control.us_lock_cpu_th_low = (uint16_t)l_val;
    oal_io_print("CPU freq high[%d] low[%d]\r\n", g_freq_lock_control.us_lock_cpu_th_high,
        g_freq_lock_control.us_lock_cpu_th_low);
#endif

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_IRQ_AFFINITY);
    g_freq_lock_control.en_irq_affinity = (l_val > 0) ? OAL_TRUE : OAL_FALSE;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_IRQ_TH_HIGH);
    g_freq_lock_control.us_throughput_irq_high = (l_val > 0) ? (uint16_t)l_val : WLAN_IRQ_TH_HIGH;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_IRQ_TH_LOW);
    g_freq_lock_control.us_throughput_irq_low = (l_val > 0) ? (uint16_t)l_val : WLAN_IRQ_TH_LOW;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_IRQ_PPS_TH_HIGH);
    g_freq_lock_control.irq_pps_high = (l_val > 0) ? (uint32_t)l_val : WLAN_IRQ_PPS_TH_HIGH;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_IRQ_PPS_TH_LOW);
    g_freq_lock_control.irq_pps_low = (l_val > 0) ? (uint32_t)l_val : WLAN_IRQ_PPS_TH_LOW;
    g_freq_lock_control.en_userctl_bindcpu = OAL_FALSE; /* 用户指定绑核命令默认不开启 */
    g_freq_lock_control.uc_userctl_irqbind = 0;
    g_freq_lock_control.uc_userctl_threadbind = 0;
    oal_io_print("irq affinity enable[%d]High_th[%u]Low_th[%u]\r\n", g_freq_lock_control.en_irq_affinity,
        g_freq_lock_control.us_throughput_irq_high, g_freq_lock_control.us_throughput_irq_low);
}


OAL_STATIC void hwifi_set_voe_custom_param(void)
{
    uint32_t val;

    val = (uint32_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_VOE_SWITCH);
    g_st_mac_device_custom_cfg.st_voe_custom_cfg.en_11k = (val & BIT0) ? OAL_TRUE : OAL_FALSE;
    g_st_mac_device_custom_cfg.st_voe_custom_cfg.en_11v = (val & BIT1) ? OAL_TRUE : OAL_FALSE;
    g_st_mac_device_custom_cfg.st_voe_custom_cfg.en_11r = (val & BIT2) ? OAL_TRUE : OAL_FALSE;
    g_st_mac_device_custom_cfg.st_voe_custom_cfg.en_11r_ds = (val & BIT3) ? OAL_TRUE : OAL_FALSE;
    g_st_mac_device_custom_cfg.st_voe_custom_cfg.en_adaptive11r = (val & BIT4) ? OAL_TRUE : OAL_FALSE;
    g_st_mac_device_custom_cfg.st_voe_custom_cfg.en_nb_rpt_11k = (val & BIT5) ? OAL_TRUE : OAL_FALSE;

    return;
}


OAL_STATIC void hwifi_config_host_roam_global_ini_param(void)
{
    int32_t l_val;

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_ROAM_SWITCH);
    g_wlan_cust.uc_roam_switch = (0 == l_val || 1 == l_val) ?
        (uint8_t)l_val : g_wlan_cust.uc_roam_switch;

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_SCAN_ORTHOGONAL);
        g_wlan_cust.uc_roam_scan_orthogonal = (1 <= l_val) ?
    (uint8_t)l_val : g_wlan_cust.uc_roam_scan_orthogonal;

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_TRIGGER_B);
    g_wlan_cust.c_roam_trigger_b = (int8_t)l_val;

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_TRIGGER_A);
    g_wlan_cust.c_roam_trigger_a = (int8_t)l_val;

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_DELTA_B);
    g_wlan_cust.c_roam_delta_b = (int8_t)l_val;

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_DELTA_A);
    g_wlan_cust.c_roam_delta_a = (int8_t)l_val;

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_DENSE_ENV_TRIGGER_B);
    g_wlan_cust.c_dense_env_roam_trigger_b = (int8_t)l_val;

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_DENSE_ENV_TRIGGER_A);
    g_wlan_cust.c_dense_env_roam_trigger_a = (int8_t)l_val;

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_SCENARIO_ENABLE);
    g_wlan_cust.uc_scenario_enable = (uint8_t)l_val;

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_CANDIDATE_GOOD_RSSI);
    g_wlan_cust.c_candidate_good_rssi = (int8_t)l_val;

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_CANDIDATE_GOOD_NUM);
    g_wlan_cust.uc_candidate_good_num = (uint8_t)l_val;

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_CANDIDATE_WEAK_NUM);
    g_wlan_cust.uc_candidate_weak_num = (uint8_t)l_val;

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_INTERVAL_VARIABLE);
    g_wlan_cust.us_roam_interval = (uint16_t)l_val;
}

OAL_STATIC void hwifi_config_tcp_ack_buf_ini_param(void)
{
    int32_t l_val;
    mac_tcp_ack_buf_switch_stru *tcp_ack_buf_switch = mac_vap_get_tcp_ack_buf_switch();

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_TX_TCP_ACK_BUF);
    tcp_ack_buf_switch->uc_ini_tcp_ack_buf_en = (l_val > 0) ? OAL_TRUE : OAL_FALSE;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_TCP_ACK_BUF_HIGH);
    tcp_ack_buf_switch->us_tcp_ack_buf_throughput_high = (l_val > 0) ? (uint16_t)l_val : WLAN_TCP_ACK_BUF_HIGH;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_TCP_ACK_BUF_LOW);
    tcp_ack_buf_switch->us_tcp_ack_buf_throughput_low = (l_val > 0) ? (uint16_t)l_val : 30; /* 吞吐量30 */
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_TCP_ACK_BUF_HIGH_40M);
    tcp_ack_buf_switch->us_tcp_ack_buf_throughput_high_40M = (l_val > 0) ? (uint16_t)l_val : WLAN_TCP_ACK_BUF_HIGH_40M;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_TCP_ACK_BUF_LOW_40M);
    tcp_ack_buf_switch->us_tcp_ack_buf_throughput_low_40M = (l_val > 0) ? (uint16_t)l_val : WLAN_TCP_ACK_BUF_LOW_40M;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_TCP_ACK_BUF_HIGH_80M);
    tcp_ack_buf_switch->us_tcp_ack_buf_throughput_high_80M = (l_val > 0) ? (uint16_t)l_val : WLAN_TCP_ACK_BUF_HIGH_80M;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_TCP_ACK_BUF_LOW_80M);
    tcp_ack_buf_switch->us_tcp_ack_buf_throughput_low_80M = (l_val > 0) ? (uint16_t)l_val : WLAN_TCP_ACK_BUF_LOW_80M;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_TCP_ACK_BUF_HIGH_160M);
    tcp_ack_buf_switch->us_tcp_ack_buf_throughput_high_160M = (l_val > 0) ?
                                                              (uint16_t)l_val : WLAN_TCP_ACK_BUF_HIGH_160M;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_TCP_ACK_BUF_LOW_160M);
    tcp_ack_buf_switch->us_tcp_ack_buf_throughput_low_160M = (l_val > 0) ? (uint16_t)l_val : WLAN_TCP_ACK_BUF_LOW_160M;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_TX_TCP_ACK_BUF_USERCTL);
    tcp_ack_buf_switch->uc_ini_tcp_ack_buf_userctl_test_en = (l_val > 0) ? OAL_TRUE : OAL_FALSE;
    /* 上层默认设置TCP ack上门限30M */
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_TCP_ACK_BUF_USERCTL_HIGH);
    tcp_ack_buf_switch->us_tcp_ack_buf_userctl_high = (l_val > 0) ? (uint16_t)l_val : WLAN_TCP_ACK_BUF_USERCTL_HIGH;
     /* 上层默认设置TCP ack上门限20M */
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_TCP_ACK_BUF_USERCTL_LOW);
    tcp_ack_buf_switch->us_tcp_ack_buf_userctl_low = (l_val > 0) ? (uint16_t)l_val : WLAN_TCP_ACK_BUF_USERCTL_LOW;
    oal_io_print("TCP ACK BUF en[%d],high/low:20M[%d]/[%d],40M[%d]/[%d],80M[%d]/[%d],160M[%d]/[%d],\
        TCP ACK BUF USERCTL[%d], userctl[%d]/[%d]",
        tcp_ack_buf_switch->uc_ini_tcp_ack_buf_en,
        tcp_ack_buf_switch->us_tcp_ack_buf_throughput_high,
        tcp_ack_buf_switch->us_tcp_ack_buf_throughput_low,
        tcp_ack_buf_switch->us_tcp_ack_buf_throughput_high_40M,
        tcp_ack_buf_switch->us_tcp_ack_buf_throughput_low_40M,
        tcp_ack_buf_switch->us_tcp_ack_buf_throughput_high_80M,
        tcp_ack_buf_switch->us_tcp_ack_buf_throughput_low_80M,
        tcp_ack_buf_switch->us_tcp_ack_buf_throughput_high_160M,
        tcp_ack_buf_switch->us_tcp_ack_buf_throughput_low_160M,
        tcp_ack_buf_switch->uc_ini_tcp_ack_buf_userctl_test_en,
        tcp_ack_buf_switch->us_tcp_ack_buf_userctl_high,
        tcp_ack_buf_switch->us_tcp_ack_buf_userctl_low);
}

#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU


OAL_STATIC void hwifi_config_host_amsdu_th_ini_param(void)
{
    int32_t l_val;
    mac_small_amsdu_switch_stru *small_amsdu_switch = mac_vap_get_small_amsdu_switch();

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_AMPDU_AMSDU_SKB);
    g_st_tx_large_amsdu.uc_host_large_amsdu_en = (l_val > 0) ? OAL_TRUE : OAL_FALSE;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_AMSDU_AMPDU_TH_HIGH);
    g_st_tx_large_amsdu.us_amsdu_throughput_high = (l_val > 0) ? (uint16_t)l_val : 500;   /* 500是高聚合量 */
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_AMSDU_AMPDU_TH_MIDDLE);
    g_st_tx_large_amsdu.us_amsdu_throughput_middle = (l_val > 0) ? (uint16_t)l_val : 100; /* 100是中等聚合量 */
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_AMSDU_AMPDU_TH_LOW);
    g_st_tx_large_amsdu.us_amsdu_throughput_low = (l_val > 0) ? (uint16_t)l_val : 50;     /* 50是低聚合量 */
    oal_io_print("ampdu+amsdu lareg amsdu en[%d],high[%d],low[%d],middle[%d]\r\n",
        g_st_tx_large_amsdu.uc_host_large_amsdu_en, g_st_tx_large_amsdu.us_amsdu_throughput_high,
        g_st_tx_large_amsdu.us_amsdu_throughput_low, g_st_tx_large_amsdu.us_amsdu_throughput_middle);

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_TX_SMALL_AMSDU);
    small_amsdu_switch->uc_ini_small_amsdu_en = (l_val > 0) ? OAL_TRUE : OAL_FALSE;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_SMALL_AMSDU_HIGH);
    small_amsdu_switch->us_small_amsdu_throughput_high = (l_val > 0) ? (uint16_t)l_val : WLAN_SMALL_AMSDU_HIGH;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_SMALL_AMSDU_LOW);
    small_amsdu_switch->us_small_amsdu_throughput_low = (l_val > 0) ? (uint16_t)l_val : WLAN_SMALL_AMSDU_LOW;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_SMALL_AMSDU_PPS_HIGH);
    small_amsdu_switch->us_small_amsdu_pps_high = (l_val > 0) ? (uint16_t)l_val : WLAN_SMALL_AMSDU_PPS_HIGH;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_SMALL_AMSDU_PPS_LOW);
    small_amsdu_switch->us_small_amsdu_pps_low = (l_val > 0) ? (uint16_t)l_val : WLAN_SMALL_AMSDU_PPS_LOW;
    oal_io_print("SMALL AMSDU SWITCH en[%d],high[%d],low[%d]\r\n", small_amsdu_switch->uc_ini_small_amsdu_en,
        small_amsdu_switch->us_small_amsdu_throughput_high, small_amsdu_switch->us_small_amsdu_throughput_low);
}
#endif


OAL_STATIC void hwifi_config_performance_ini_param(void)
{
    int32_t l_val;
    mac_rx_buffer_size_stru *rx_buffer_size = mac_vap_get_rx_buffer_size();

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_AMPDU_TX_MAX_NUM);
    g_wlan_cust.ampdu_tx_max_num = (g_wlan_spec_cfg->max_tx_ampdu_num >= l_val && 1 <= l_val) ?
        (uint32_t)l_val : g_wlan_cust.ampdu_tx_max_num;
    oal_io_print("hwifi_config_host_global_ini_param::ampdu_tx_max_num:%d", g_wlan_cust.ampdu_tx_max_num);

#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_HW_AMPDU);
    g_st_ampdu_hw.uc_ampdu_hw_en = (l_val > 0) ? OAL_TRUE : OAL_FALSE;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_HW_AMPDU_TH_HIGH);
    g_st_ampdu_hw.us_throughput_high = (l_val > 0) ? (uint16_t)l_val : WLAN_HW_AMPDU_TH_HIGH;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_HW_AMPDU_TH_LOW);
    g_st_ampdu_hw.us_throughput_low = (l_val > 0) ? (uint16_t)l_val : WLAN_HW_AMPDU_TH_LOW;
    oal_io_print("ampdu_hw enable[%d]H[%u]L[%u]\r\n", g_st_ampdu_hw.uc_ampdu_hw_en,
        g_st_ampdu_hw.us_throughput_high, g_st_ampdu_hw.us_throughput_low);
#endif

#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
    hwifi_config_host_amsdu_th_ini_param();
#endif
#ifdef _PRE_WLAN_TCP_OPT
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_TCP_ACK_FILTER);
    g_st_tcp_ack_filter.uc_tcp_ack_filter_en = (l_val > 0) ? OAL_TRUE : OAL_FALSE;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_TCP_ACK_FILTER_TH_HIGH);
    g_st_tcp_ack_filter.us_rx_filter_throughput_high = (l_val > 0) ? (uint16_t)l_val : 50; /* 吞吐量50 */
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_TCP_ACK_FILTER_TH_LOW);
    g_st_tcp_ack_filter.us_rx_filter_throughput_low = (l_val > 0) ? (uint16_t)l_val : WLAN_TCP_ACK_FILTER_TH_LOW;
    oal_io_print("tcp ack filter en[%d],high[%d],low[%d]\r\n", g_st_tcp_ack_filter.uc_tcp_ack_filter_en,
        g_st_tcp_ack_filter.us_rx_filter_throughput_high, g_st_tcp_ack_filter.us_rx_filter_throughput_low);
#endif

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RX_AMPDU_AMSDU_SKB);
    g_uc_host_rx_ampdu_amsdu = (l_val > 0) ? (uint8_t)l_val : OAL_FALSE;
    oal_io_print("Rx:ampdu+amsdu skb en[%d]\r\n", g_uc_host_rx_ampdu_amsdu);

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RX_AMPDU_BITMAP);
    rx_buffer_size->en_rx_ampdu_bitmap_ini = (l_val > 0) ? OAL_TRUE : OAL_FALSE;
    rx_buffer_size->us_rx_buffer_size = (uint16_t)l_val;
    oal_io_print("Rx:ampdu bitmap size[%d]\r\n", l_val);
}

static void hwifi_config_host_global_11ax_ini_param(void)
{
    int32_t l_val;

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_11AX_SWITCH);
    g_pst_mac_device_capability[0].en_11ax_switch = (((uint32_t)l_val & 0x0F) & BIT0) ? OAL_TRUE : OAL_FALSE;
#ifdef _PRE_WLAN_FEATURE_11AX
    g_st_mac_device_custom_cfg.st_11ax_custom_cfg.bit_11ax_aput_switch =
        (((uint32_t)l_val & 0x0F) & BIT1) ? OAL_TRUE : OAL_FALSE;

    g_st_mac_device_custom_cfg.st_11ax_custom_cfg.bit_ignore_non_he_cap_from_beacon =
            (((uint32_t)l_val & 0x0F) & BIT2) ? OAL_TRUE : OAL_FALSE;

    g_st_mac_device_custom_cfg.st_11ax_custom_cfg.bit_11ax_aput_he_cap_switch =
        (((uint32_t)l_val & 0x0F) & BIT3) ? OAL_TRUE : OAL_FALSE;
    g_st_mac_device_custom_cfg.st_11ax_custom_cfg.bit_twt_responder_support =
        (((uint32_t)l_val & 0xFF) & BIT4) ? OAL_TRUE : OAL_FALSE;
    g_st_mac_device_custom_cfg.st_11ax_custom_cfg.bit_twt_requester_support =
        (((uint32_t)l_val & 0xFF) & BIT5) ? OAL_TRUE : OAL_FALSE;
#endif

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_MULTI_BSSID_SWITCH);
    g_pst_mac_device_capability[0].bit_multi_bssid_switch = (((uint32_t)l_val & 0x0F) & BIT0) ? OAL_TRUE : OAL_FALSE;
#ifdef _PRE_WLAN_FEATURE_11AX
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_HTC_SWITCH);
    g_st_mac_device_custom_cfg.st_11ax_custom_cfg.bit_htc_include =
        (((uint32_t)l_val & 0x0F) & BIT0) ? OAL_TRUE : OAL_FALSE;
    g_st_mac_device_custom_cfg.st_11ax_custom_cfg.bit_om_in_data =
        (((uint32_t)l_val & 0x0F) & BIT1) ? OAL_TRUE : OAL_FALSE;
    g_st_mac_device_custom_cfg.st_11ax_custom_cfg.bit_rom_cap_switch =
        (((uint32_t)l_val & 0x0F) & BIT2) ? OAL_TRUE : OAL_FALSE;
#endif
}


OAL_STATIC void hwifi_config_dmac_freq_ini_param(void)
{
    uint32_t cfg_id;
    uint32_t val;
    int32_t l_cfg_value;
    int8_t *pc_tmp;
    host_speed_freq_level_stru ast_host_speed_freq_level_tmp[NUM_4_BITS];
    device_speed_freq_level_stru ast_device_speed_freq_level_tmp[NUM_4_BITS];
    uint8_t uc_flag = OAL_FALSE;
    uint8_t uc_index;
    int32_t l_ret = EOK;

    /******************************************** 自动调频 ********************************************/
    /* config g_host_speed_freq_level */
    pc_tmp = (int8_t *)&ast_host_speed_freq_level_tmp;

    for (cfg_id = WLAN_CFG_INIT_PSS_THRESHOLD_LEVEL_0; cfg_id <= WLAN_CFG_INIT_DDR_FREQ_LIMIT_LEVEL_3; ++cfg_id) {
        val = (uint32_t)hwifi_get_init_value(CUS_TAG_INI, cfg_id);
        *(uint32_t *)pc_tmp = val;
        pc_tmp += BYTE_OFFSET_4;
    }

    /* config g_device_speed_freq_level */
    pc_tmp = (int8_t *)&ast_device_speed_freq_level_tmp;
    for (cfg_id = WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_0; cfg_id <= WLAN_CFG_INIT_DEVICE_TYPE_LEVEL_3; ++cfg_id) {
        l_cfg_value = hwifi_get_init_value(CUS_TAG_INI, cfg_id);
        if (oal_value_in_valid_range(l_cfg_value, FREQ_IDLE, FREQ_HIGHEST)) {
            *pc_tmp = l_cfg_value;
            pc_tmp += BIT_OFFSET_4;
        } else {
            uc_flag = OAL_TRUE;
            break;
        }
    }

    if (!uc_flag) {
        l_ret += memcpy_s(&g_host_speed_freq_level, sizeof(g_host_speed_freq_level),
                          &ast_host_speed_freq_level_tmp, sizeof(ast_host_speed_freq_level_tmp));
        l_ret += memcpy_s(&g_device_speed_freq_level, sizeof(g_device_speed_freq_level),
                          &ast_device_speed_freq_level_tmp, sizeof(ast_device_speed_freq_level_tmp));
        if (l_ret != EOK) {
            oam_error_log0(0, OAM_SF_ANY, "hwifi_config_host_global_ini_param::memcpy fail!");
            return;
        }

        for (uc_index = 0; uc_index < HOST_SPEED_FREQ_LEVEL_BUTT; uc_index++) {
            oam_warning_log4(0, OAM_SF_ANY, "{hwifi_config_host_global_ini_param::ul_speed_level = %d, \
                min_cpu_freq = %d, min_ddr_freq = %d, uc_device_type = %d}\r\n",
                g_host_speed_freq_level[uc_index].speed_level,
                g_host_speed_freq_level[uc_index].min_cpu_freq,
                g_host_speed_freq_level[uc_index].min_ddr_freq,
                g_device_speed_freq_level[uc_index].uc_device_type);
        }
    }
}

OAL_STATIC void hwifi_config_bypass_extlna_ini_param(void)
{
    int32_t l_val;
    mac_rx_dyn_bypass_extlna_stru *rx_dyn_bypass_extlna_switch = NULL;
    rx_dyn_bypass_extlna_switch = mac_vap_get_rx_dyn_bypass_extlna_switch();

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RX_DYN_BYPASS_EXTLNA);
    rx_dyn_bypass_extlna_switch->uc_ini_en = (l_val > 0) ? OAL_TRUE : OAL_FALSE;
    rx_dyn_bypass_extlna_switch->uc_cur_status = OAL_TRUE; /* 默认低功耗场景 */
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RX_DYN_BYPASS_EXTLNA_HIGH);
    rx_dyn_bypass_extlna_switch->us_throughput_high = (l_val > 0) ? (uint16_t)l_val : WLAN_RX_DYN_BYPASS_EXTLNA_HIGH;
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RX_DYN_BYPASS_EXTLNA_LOW);
    rx_dyn_bypass_extlna_switch->us_throughput_low = (l_val > 0) ? (uint16_t)l_val : WLAN_RX_DYN_BYPASS_EXTLNA_LOW;

    oal_io_print("DYN_BYPASS_EXTLNA SWITCH en[%d],high[%d],low[%d]\r\n", rx_dyn_bypass_extlna_switch->uc_ini_en,
        rx_dyn_bypass_extlna_switch->us_throughput_high, rx_dyn_bypass_extlna_switch->us_throughput_low);
}


static void hwifi_config_factory_lte_gpio_ini_param(void)
{
    int32_t val;

    val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_LTE_GPIO_CHECK_SWITCH);
    g_wlan_cust.lte_gpio_check_switch = (uint32_t) !!val;
    val = hwifi_get_init_value(CUS_TAG_INI, WLAN_ATCMDSRV_ISM_PRIORITY);
    g_wlan_cust.ism_priority = (uint32_t)val;
    val = hwifi_get_init_value(CUS_TAG_INI, WLAN_ATCMDSRV_LTE_RX);
    g_wlan_cust.lte_rx = (uint32_t)val;
    val = hwifi_get_init_value(CUS_TAG_INI, WLAN_ATCMDSRV_LTE_TX);
    g_wlan_cust.lte_tx = (uint32_t)val;
    val = hwifi_get_init_value(CUS_TAG_INI, WLAN_ATCMDSRV_LTE_INACT);
    g_wlan_cust.lte_inact = (uint32_t)val;
    val = hwifi_get_init_value(CUS_TAG_INI, WLAN_ATCMDSRV_ISM_RX_ACT);
    g_wlan_cust.ism_rx_act = (uint32_t)val;
    val = hwifi_get_init_value(CUS_TAG_INI, WLAN_ATCMDSRV_BANT_PRI);
    g_wlan_cust.bant_pri = (uint32_t)val;
    val = hwifi_get_init_value(CUS_TAG_INI, WLAN_ATCMDSRV_BANT_STATUS);
    g_wlan_cust.bant_status = (uint32_t)val;
    val = hwifi_get_init_value(CUS_TAG_INI, WLAN_ATCMDSRV_WANT_PRI);
    g_wlan_cust.want_pri = (uint32_t)val;
    val = hwifi_get_init_value(CUS_TAG_INI, WLAN_ATCMDSRV_WANT_STATUS);
    g_wlan_cust.want_status = (uint32_t)val;
}


OAL_STATIC void hwifi_config_host_global_ini_param_extend(void)
{
    int32_t l_val;

#ifdef _PRE_WLAN_FEATURE_MBO
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_MBO_SWITCH);
    g_uc_mbo_switch = !!l_val;
#endif

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_DYNAMIC_DBAC_SWITCH);
    g_uc_dbac_dynamic_switch = (uint8_t)l_val;

    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_DDR_FREQ);
    g_ddr_freq = (uint32_t)l_val;

#ifdef _PRE_WLAN_FEATURE_HIEX
    if (g_wlan_spec_cfg->feature_11ax_is_open) {
        l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_HIEX_CAP);
        if (memcpy_s(&g_st_default_hiex_cap, sizeof(mac_hiex_cap_stru), &l_val,
            sizeof(l_val)) != EOK) {
            oam_error_log0(0, OAM_SF_ANY, "hwifi_config_host_global_ini_param::hiex cap memcpy fail!");
        }
    }
#endif
#ifdef _PRE_WLAN_FEATURE_FTM
    if (g_wlan_spec_cfg->feature_ftm_is_open) {
        l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_FTM_CAP);
        g_mac_ftm_cap = (uint8_t)l_val;
    }
#endif
}



void hwifi_config_host_global_ini_param_1103(void)
{
    int32_t l_val;

    /******************************************** 漫游 ********************************************/
    hwifi_config_host_roam_global_ini_param();

    /******************************************** 性能 ********************************************/
    wlan_chip_cpu_freq_ini_param_init();

    hwifi_config_tcp_ack_buf_ini_param();

    hwifi_config_dmac_freq_ini_param();

    hwifi_config_bypass_extlna_ini_param();

    hwifi_config_performance_ini_param();

#ifdef _PRE_WLAN_FEATURE_MCAST_AMPDU
    /******************************************** 组播聚合 ********************************************/
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_MCAST_AMPDU_ENABLE);
    g_mcast_ampdu_cfg.mcast_ampdu_enable = (l_val > 0) ? OAL_TRUE : OAL_FALSE;
#endif
    /******************************************** 组播图传 ********************************************/
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_PT_MCAST_ENABLE);
    g_pt_mcast_enable = (l_val > 0) ? OAL_TRUE : OAL_FALSE;
    /******************************************** 随机MAC地址扫描 ********************************************/
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RANDOM_MAC_ADDR_SCAN);
    g_wlan_cust.uc_random_mac_addr_scan = !!l_val;

    /******************************************** CAPABILITY ********************************************/
    l_val = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_DISABLE_CAPAB_2GHT40);
    g_wlan_cust.uc_disable_capab_2ght40 = (uint8_t) !!l_val;
    /********************************************factory_lte_gpio_check ********************************************/
    hwifi_config_factory_lte_gpio_ini_param();

    hwifi_config_host_global_ini_param_extend();

    hwifi_set_voe_custom_param();
#ifdef _PRE_WLAN_FEATURE_11AX
    if (g_wlan_spec_cfg->feature_11ax_is_open) {
        hwifi_config_host_global_11ax_ini_param();
    }
#endif
    return;
}

OAL_STATIC uint32_t hwifi_cfg_front_end_set_2g_rf(mac_cfg_customize_rf *pst_customize_rf)
{
    uint8_t uc_idx; /* 结构体数组下标 */
    int32_t l_mult4;
    int8_t c_mult4_rf[NUM_2_BYTES];
    /* 配置: 2g rf */
    for (uc_idx = 0; uc_idx < MAC_NUM_2G_BAND; ++uc_idx) {
        /* 获取各2p4g 各band 0.25db及0.1db精度的线损值 */
        l_mult4 = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND_START + uc_idx);
        /* rf0 */
        c_mult4_rf[0] = (int8_t)cus_get_first_byte(l_mult4);
        /* rf1 */
        c_mult4_rf[1] = (int8_t)cus_get_second_byte(l_mult4);
        if (cus_val_valid(c_mult4_rf[0], RF_LINE_TXRX_GAIN_DB_MAX, RF_LINE_TXRX_GAIN_DB_2G_MIN) &&
            cus_val_valid(c_mult4_rf[1], RF_LINE_TXRX_GAIN_DB_MAX, RF_LINE_TXRX_GAIN_DB_2G_MIN)) {
            pst_customize_rf->ast_rf_gain_db_rf[0].ac_gain_db_2g[uc_idx].c_rf_gain_db_mult4 = c_mult4_rf[0];
            pst_customize_rf->ast_rf_gain_db_rf[1].ac_gain_db_2g[uc_idx].c_rf_gain_db_mult4 = c_mult4_rf[1];
        } else {
            /* 值超出有效范围 */
            oam_error_log2(0, OAM_SF_CFG,
                "{hwifi_cfg_front_end_set_2g_rf::ini_id[%d]value out of range, 2g mult4[0x%0x}!}",
                WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_2G_BAND_START + uc_idx, l_mult4);
            return OAL_FAIL;
        }
    }
    return OAL_SUCC;
}

OAL_STATIC uint32_t hwifi_cfg_front_end_set_5g_rf(mac_cfg_customize_rf *pst_customize_rf)
{
    uint8_t uc_idx; /* 结构体数组下标 */
    int32_t l_mult4;
    int8_t c_mult4_rf[NUM_2_BYTES];
    /* 配置: 5g rf */
    /* 配置: fem口到天线口的负增益 */
    for (uc_idx = 0; uc_idx < MAC_NUM_5G_BAND; ++uc_idx) {
        /* 获取各5g 各band 0.25db及0.1db精度的线损值 */
        l_mult4 = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND_START + uc_idx);
        c_mult4_rf[0] = (int8_t)cus_get_first_byte(l_mult4);
        c_mult4_rf[1] = (int8_t)cus_get_second_byte(l_mult4);
        if (c_mult4_rf[0] <= RF_LINE_TXRX_GAIN_DB_MAX && c_mult4_rf[1] <= RF_LINE_TXRX_GAIN_DB_MAX) {
            pst_customize_rf->ast_rf_gain_db_rf[0].ac_gain_db_5g[uc_idx].c_rf_gain_db_mult4 = c_mult4_rf[0];
            pst_customize_rf->ast_rf_gain_db_rf[1].ac_gain_db_5g[uc_idx].c_rf_gain_db_mult4 = c_mult4_rf[1];
        } else {
            /* 值超出有效范围 */
            oam_error_log2(0, OAM_SF_CFG,
                "{hwifi_cfg_front_end_set_5g_rf::ini_id[%d]value out of range, 5g mult4[0x%0x}}",
                WLAN_CFG_INIT_RF_RX_INSERTION_LOSS_5G_BAND_START + uc_idx, l_mult4);
            return OAL_FAIL;
        }
    }
    return OAL_SUCC;
}



OAL_STATIC oal_bool_enum_uint8 hwifi_cfg_front_end_value_range_check(mac_cfg_customize_rf *customize_rf,
    int32_t band, int32_t l_rf_db_min)
{
    return ((customize_rf->ast_ext_rf[band][WLAN_RF_CHANNEL_ZERO].c_lna_bypass_gain_db < l_rf_db_min ||
        customize_rf->ast_ext_rf[band][WLAN_RF_CHANNEL_ZERO].c_lna_bypass_gain_db > 0 ||
        customize_rf->ast_ext_rf[band][WLAN_RF_CHANNEL_ZERO].c_pa_gain_b0_db < l_rf_db_min ||
        customize_rf->ast_ext_rf[band][WLAN_RF_CHANNEL_ZERO].c_pa_gain_b0_db > 0 ||
        customize_rf->ast_ext_rf[band][WLAN_RF_CHANNEL_ZERO].c_pa_gain_b1_db < l_rf_db_min ||
        customize_rf->ast_ext_rf[band][WLAN_RF_CHANNEL_ZERO].c_pa_gain_b1_db > 0 ||
        customize_rf->ast_ext_rf[band][WLAN_RF_CHANNEL_ZERO].uc_pa_gain_lvl_num == 0 ||
        customize_rf->ast_ext_rf[band][WLAN_RF_CHANNEL_ZERO].uc_pa_gain_lvl_num > MAC_EXT_PA_GAIN_MAX_LVL ||
        customize_rf->ast_ext_rf[band][WLAN_RF_CHANNEL_ZERO].c_lna_gain_db < LNA_GAIN_DB_MIN ||
        customize_rf->ast_ext_rf[band][WLAN_RF_CHANNEL_ZERO].c_lna_gain_db > LNA_GAIN_DB_MAX) ||
        (customize_rf->ast_ext_rf[band][WLAN_RF_CHANNEL_ONE].c_lna_bypass_gain_db < l_rf_db_min ||
        customize_rf->ast_ext_rf[band][WLAN_RF_CHANNEL_ONE].c_lna_bypass_gain_db > 0 ||
        customize_rf->ast_ext_rf[band][WLAN_RF_CHANNEL_ONE].c_pa_gain_b0_db < l_rf_db_min ||
        customize_rf->ast_ext_rf[band][WLAN_RF_CHANNEL_ONE].c_pa_gain_b0_db > 0 ||
        customize_rf->ast_ext_rf[band][WLAN_RF_CHANNEL_ONE].c_pa_gain_b1_db < l_rf_db_min ||
        customize_rf->ast_ext_rf[band][WLAN_RF_CHANNEL_ONE].c_pa_gain_b1_db > 0 ||
        customize_rf->ast_ext_rf[band][WLAN_RF_CHANNEL_ONE].uc_pa_gain_lvl_num == 0 ||
        customize_rf->ast_ext_rf[band][WLAN_RF_CHANNEL_ONE].uc_pa_gain_lvl_num > MAC_EXT_PA_GAIN_MAX_LVL ||
        customize_rf->ast_ext_rf[band][WLAN_RF_CHANNEL_ONE].c_lna_gain_db < LNA_GAIN_DB_MIN ||
        customize_rf->ast_ext_rf[band][WLAN_RF_CHANNEL_ONE].c_lna_gain_db > LNA_GAIN_DB_MAX)) ? OAL_TRUE : OAL_FALSE;
}

OAL_STATIC void hwifi_cfg_front_end_2g_rf0_fem(mac_cfg_customize_rf *pst_customize_rf)
{
    /* 2g 外部fem */
    /* RF0 */
    pst_customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ZERO].c_lna_bypass_gain_db =
        (int8_t)cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_LNA_BYPASS_GAIN_DB_2G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ZERO].c_lna_gain_db =
        (int8_t)cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_LNA_GAIN_DB_2G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ZERO].c_pa_gain_b0_db =
        (int8_t)cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_PA_GAIN_DB_B0_2G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ZERO].c_pa_gain_b1_db =
        (int8_t)cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_PA_GAIN_DB_B1_2G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ZERO].uc_pa_gain_lvl_num =
        (uint8_t)cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_PA_GAIN_LVL_2G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ZERO].uc_ext_switch_isexist =
        (uint8_t) !!cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_2G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ZERO].uc_ext_pa_isexist =
        (uint8_t) !!cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_EXT_PA_ISEXIST_2G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ZERO].uc_ext_lna_isexist =
        (uint8_t)cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_EXT_LNA_ISEXIST_2G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ZERO].us_lna_on2off_time_ns =
        (uint16_t)cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_2G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ZERO].us_lna_off2on_time_ns =
        (uint16_t)cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_2G));
}

OAL_STATIC void hwifi_cfg_front_end_2g_rf1_fem(mac_cfg_customize_rf *pst_customize_rf)
{
    /* RF1 */
    pst_customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ONE].c_lna_bypass_gain_db =
        (int8_t)cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_LNA_BYPASS_GAIN_DB_2G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ONE].c_lna_gain_db =
        (int8_t)cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_LNA_GAIN_DB_2G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ONE].c_pa_gain_b0_db =
        (int8_t)cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_PA_GAIN_DB_B0_2G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ONE].c_pa_gain_b1_db =
        (int8_t)cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_PA_GAIN_DB_B1_2G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ONE].uc_pa_gain_lvl_num =
        (uint8_t)cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_PA_GAIN_LVL_2G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ONE].uc_ext_switch_isexist =
        (uint8_t) !!cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_2G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ONE].uc_ext_pa_isexist =
        (uint8_t) !!cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_EXT_PA_ISEXIST_2G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ONE].uc_ext_lna_isexist =
        (uint8_t)cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_EXT_LNA_ISEXIST_2G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ONE].us_lna_on2off_time_ns =
        (uint16_t)cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_2G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ONE].us_lna_off2on_time_ns =
        (uint16_t)cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_2G));
}

OAL_STATIC uint32_t hwifi_cfg_front_end_2g_fem(mac_cfg_customize_rf *customize_rf)
{
    uint8_t uc_idx;

    /* RF0 */
    hwifi_cfg_front_end_2g_rf0_fem(customize_rf);
    /* RF1 */
    hwifi_cfg_front_end_2g_rf1_fem(customize_rf);

    if (hwifi_cfg_front_end_value_range_check(customize_rf, WLAN_BAND_2G, RF_LINE_TXRX_GAIN_DB_2G_MIN) == OAL_TRUE) {
        /* 值超出有效范围 */
        oam_error_log4(0, OAM_SF_CFG,
            "{hwifi_cfg_front_end_2g_fem:2g gain db out of range! rf0 lna_bypass[%d] pa_b0[%d] lna gain[%d] pa_b1[%d]}",
            customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ONE].c_lna_bypass_gain_db,
            customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ONE].c_pa_gain_b0_db,
            customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ONE].c_lna_gain_db,
            customize_rf->ast_ext_rf[WLAN_BAND_2G][WLAN_RF_CHANNEL_ONE].c_pa_gain_b1_db);
        return OAL_FAIL;
    }
    /* 2g定制化RF部分PA偏置寄存器  */
    for (uc_idx = 0; uc_idx < CUS_RF_PA_BIAS_REG_NUM; uc_idx++) {
        customize_rf->aul_2g_pa_bias_rf_reg[uc_idx] =
        (uint32_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_TX2G_PA_GATE_VCTL_REG236 + uc_idx);
    }
    return OAL_SUCC;
}


OAL_STATIC oal_bool_enum_uint8 hwifi_cfg_front_end_adjustment_range_check(int8_t c_delta_cca_ed_high_20th_2g,
    int8_t c_delta_cca_ed_high_40th_2g, int8_t c_delta_cca_ed_high_20th_5g,
    int8_t c_delta_cca_ed_high_40th_5g, int8_t c_delta_cca_ed_high_80th_5g)
{
    return (cus_delta_cca_ed_high_th_out_of_range(c_delta_cca_ed_high_20th_2g) ||
            cus_delta_cca_ed_high_th_out_of_range(c_delta_cca_ed_high_40th_2g) ||
            cus_delta_cca_ed_high_th_out_of_range(c_delta_cca_ed_high_20th_5g) ||
            cus_delta_cca_ed_high_th_out_of_range(c_delta_cca_ed_high_40th_5g) ||
            cus_delta_cca_ed_high_th_out_of_range(c_delta_cca_ed_high_80th_5g)) ? OAL_TRUE : OAL_FALSE;
}

OAL_STATIC void hwifi_cfg_front_end_5g_rf0_fem(mac_cfg_customize_rf *pst_customize_rf)
{
    /* 5g 外部fem */
    /* RF0 */
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ZERO].c_lna_bypass_gain_db =
        (int8_t)cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_LNA_BYPASS_GAIN_DB_5G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ZERO].c_lna_gain_db =
        (int8_t)cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_LNA_GAIN_DB_5G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ZERO].c_pa_gain_b0_db =
        (int8_t)cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_PA_GAIN_DB_B0_5G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ZERO].c_pa_gain_b1_db =
        (int8_t)cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_PA_GAIN_DB_B1_5G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ZERO].uc_pa_gain_lvl_num =
        (uint8_t)cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_PA_GAIN_LVL_5G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ZERO].uc_ext_switch_isexist =
        (uint8_t) !!cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_5G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ZERO].uc_ext_pa_isexist =
        (uint8_t) !!(cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_EXT_PA_ISEXIST_5G)) &
        EXT_PA_ISEXIST_5G_MASK);
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ZERO].en_fem_lp_enable =
        (oal_fem_lp_state_enum_uint8)((cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI,
            WLAN_CFG_INIT_EXT_PA_ISEXIST_5G)) & EXT_FEM_LP_STATUS_MASK) >> EXT_FEM_LP_STATUS_OFFSET);
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ZERO].c_fem_spec_value =
        (int8_t)((cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI,
            WLAN_CFG_INIT_EXT_PA_ISEXIST_5G)) & EXT_FEM_FEM_SPEC_MASK) >> EXT_FEM_FEM_SPEC_OFFSET);
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ZERO].uc_ext_lna_isexist =
        (uint8_t)cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_EXT_LNA_ISEXIST_5G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ZERO].us_lna_on2off_time_ns =
        (uint16_t)cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_5G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ZERO].us_lna_off2on_time_ns =
        (uint16_t)cus_get_low_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_5G));
}
OAL_STATIC void hwifi_cfg_front_end_5g_rf1_fem(mac_cfg_customize_rf *pst_customize_rf)
{
    /* 5g 外部fem */
    /* RF1 */
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ONE].c_lna_bypass_gain_db =
        (int8_t)cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_LNA_BYPASS_GAIN_DB_5G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ONE].c_lna_gain_db =
        (int8_t)cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_LNA_GAIN_DB_5G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ONE].c_pa_gain_b0_db =
        (int8_t)cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_PA_GAIN_DB_B0_5G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ONE].c_pa_gain_b1_db =
        (int8_t)cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_PA_GAIN_DB_B1_5G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ONE].uc_pa_gain_lvl_num =
        (uint8_t)cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_RF_PA_GAIN_LVL_5G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ONE].uc_ext_switch_isexist =
        (uint8_t) !!cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_EXT_SWITCH_ISEXIST_5G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ONE].uc_ext_pa_isexist =
        (uint8_t) !!(cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_EXT_PA_ISEXIST_5G)) &
        EXT_PA_ISEXIST_5G_MASK);
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ONE].en_fem_lp_enable =
        (oal_fem_lp_state_enum_uint8)((cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI,
            WLAN_CFG_INIT_EXT_PA_ISEXIST_5G)) & EXT_FEM_LP_STATUS_MASK) >> EXT_FEM_LP_STATUS_OFFSET);
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ONE].c_fem_spec_value =
        (int8_t)((cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_EXT_PA_ISEXIST_5G)) &
        EXT_FEM_FEM_SPEC_MASK) >> EXT_FEM_FEM_SPEC_OFFSET);
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ONE].uc_ext_lna_isexist =
        (uint8_t)cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_EXT_LNA_ISEXIST_5G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ONE].us_lna_on2off_time_ns =
        (uint16_t)cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LNA_ON2OFF_TIME_NS_5G));
    pst_customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ONE].us_lna_off2on_time_ns =
        (uint16_t)cus_get_high_16bits(hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LNA_OFF2ON_TIME_NS_5G));
}

OAL_STATIC uint32_t hwifi_cfg_front_end_5g_fem(mac_cfg_customize_rf *customize_rf)
{
    uint8_t uc_idx;
    /* RF0 */
    hwifi_cfg_front_end_5g_rf0_fem(customize_rf);
    /* RF1 */
    hwifi_cfg_front_end_5g_rf1_fem(customize_rf);

    /* 5g upc mix_bf_gain_ctl for P10 */
    for (uc_idx = 0; uc_idx < MAC_NUM_5G_BAND; uc_idx++) {
        customize_rf->aul_5g_upc_mix_gain_rf_reg[uc_idx] =
            (uint32_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_TX5G_UPC_MIX_GAIN_CTRL_1 + uc_idx);
    }

    if (hwifi_cfg_front_end_value_range_check(customize_rf, WLAN_BAND_5G, RF_LINE_TXRX_GAIN_DB_5G_MIN) == OAL_TRUE) {
        /* 值超出有效范围 */
        oam_error_log4(0, OAM_SF_CFG,
            "{hwifi_cfg_front_end_5g_fem:2g gain db out of range! rf0 lna_bypass[%d] pa_b0[%d] lna gain[%d] pa_b1[%d]}",
            customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ONE].c_lna_bypass_gain_db,
            customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ONE].c_pa_gain_b0_db,
            customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ONE].c_lna_gain_db,
            customize_rf->ast_ext_rf[WLAN_BAND_5G][WLAN_RF_CHANNEL_ONE].c_pa_gain_b1_db);
        return OAL_FAIL;
    }
    return OAL_SUCC;
}

/* 配置: cca能量门限调整值 */
OAL_STATIC void hwifi_set_cca_energy_thrsehold(mac_cfg_customize_rf *pst_customize_rf)
{
    int8_t c_delta_cca_ed_high_20th_2g =
        (int8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_20TH_2G);
    int8_t c_delta_cca_ed_high_40th_2g =
        (int8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_40TH_2G);
    int8_t c_delta_cca_ed_high_20th_5g =
        (int8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_20TH_5G);
    int8_t c_delta_cca_ed_high_40th_5g =
        (int8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_40TH_5G);
    int8_t c_delta_cca_ed_high_80th_5g =
        (int8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_DELTA_CCA_ED_HIGH_80TH_5G);
    /* 检查每一项的调整幅度是否超出最大限制 */
    if (hwifi_cfg_front_end_adjustment_range_check(c_delta_cca_ed_high_20th_2g, c_delta_cca_ed_high_40th_2g,
                                                   c_delta_cca_ed_high_20th_5g, c_delta_cca_ed_high_40th_5g,
                                                   c_delta_cca_ed_high_80th_5g) == OAL_TRUE) {
        oam_error_log4(0, OAM_SF_ANY,
            "{hwifi_set_cca_energy_thrsehold::one or more delta cca ed high threshold out of range \
            [delta_20th_2g=%d, delta_40th_2g=%d, delta_20th_5g=%d, delta_40th_5g=%d], please check the value!}",
            c_delta_cca_ed_high_20th_2g, c_delta_cca_ed_high_40th_2g,
            c_delta_cca_ed_high_20th_5g, c_delta_cca_ed_high_40th_5g);
        /* set 0 */
        pst_customize_rf->c_delta_cca_ed_high_20th_2g = 0;
        pst_customize_rf->c_delta_cca_ed_high_40th_2g = 0;
        pst_customize_rf->c_delta_cca_ed_high_20th_5g = 0;
        pst_customize_rf->c_delta_cca_ed_high_40th_5g = 0;
        pst_customize_rf->c_delta_cca_ed_high_80th_5g = 0;
    } else {
        pst_customize_rf->c_delta_cca_ed_high_20th_2g = c_delta_cca_ed_high_20th_2g;
        pst_customize_rf->c_delta_cca_ed_high_40th_2g = c_delta_cca_ed_high_40th_2g;
        pst_customize_rf->c_delta_cca_ed_high_20th_5g = c_delta_cca_ed_high_20th_5g;
        pst_customize_rf->c_delta_cca_ed_high_40th_5g = c_delta_cca_ed_high_40th_5g;
        pst_customize_rf->c_delta_cca_ed_high_80th_5g = c_delta_cca_ed_high_80th_5g;
    }
}


static int8_t hwifi_check_pwr_ref_delta(int8_t c_pwr_ref_delta)
{
    int8_t c_ret = 0;
    if (c_pwr_ref_delta > WAL_HIPRIV_PWR_REF_DELTA_HI) {
        c_ret = WAL_HIPRIV_PWR_REF_DELTA_HI;
    } else if (c_pwr_ref_delta < WAL_HIPRIV_PWR_REF_DELTA_LO) {
        c_ret = WAL_HIPRIV_PWR_REF_DELTA_LO;
    } else {
        c_ret = c_pwr_ref_delta;
    }

    return c_ret;
}


static void hwifi_cfg_pwr_ref_delta(mac_cfg_customize_rf *pst_customize_rf)
{
    uint8_t uc_rf_idx;
    wlan_cfg_init cfg_id;
    int32_t l_pwr_ref_delta;
    mac_cfg_custom_delta_pwr_ref_stru *pst_delta_pwr_ref = NULL;
    mac_cfg_custom_amend_rssi_stru *pst_rssi_amend_ref = NULL;
    int8_t *rssi = NULL;

    for (uc_rf_idx = 0; uc_rf_idx < WLAN_RF_CHANNEL_NUMS; uc_rf_idx++) {
        pst_delta_pwr_ref = &pst_customize_rf->ast_delta_pwr_ref_cfg[uc_rf_idx];
        /* 2G 20M/40M */
        cfg_id = (uc_rf_idx == WLAN_RF_CHANNEL_ZERO) ?
            WLAN_CFG_INIT_RF_PWR_REF_RSSI_2G_C0_MULT4 : WLAN_CFG_INIT_RF_PWR_REF_RSSI_2G_C1_MULT4;
        l_pwr_ref_delta = hwifi_get_init_value(CUS_TAG_INI, cfg_id);
        rssi = pst_delta_pwr_ref->c_cfg_delta_pwr_ref_rssi_2g;
        rssi[0] = hwifi_check_pwr_ref_delta((int8_t)cus_get_first_byte(l_pwr_ref_delta));
        rssi[1] = hwifi_check_pwr_ref_delta((int8_t)cus_get_second_byte(l_pwr_ref_delta));
        /* 5G 20M/40M/80M/160M */
        cfg_id = (uc_rf_idx == WLAN_RF_CHANNEL_ZERO) ?
            WLAN_CFG_INIT_RF_PWR_REF_RSSI_5G_C0_MULT4 : WLAN_CFG_INIT_RF_PWR_REF_RSSI_5G_C1_MULT4;
        l_pwr_ref_delta = hwifi_get_init_value(CUS_TAG_INI, cfg_id);
        rssi = pst_delta_pwr_ref->c_cfg_delta_pwr_ref_rssi_5g;
        rssi[0] = hwifi_check_pwr_ref_delta((int8_t)cus_get_first_byte(l_pwr_ref_delta));
        rssi[1] = hwifi_check_pwr_ref_delta((int8_t)cus_get_second_byte(l_pwr_ref_delta));
        rssi[BYTE_OFFSET_2] = hwifi_check_pwr_ref_delta((int8_t)cus_get_third_byte(l_pwr_ref_delta));
        rssi[BYTE_OFFSET_3] = hwifi_check_pwr_ref_delta((int8_t)cus_get_fourth_byte(l_pwr_ref_delta));

        /* RSSI amend */
        pst_rssi_amend_ref = &pst_customize_rf->ast_rssi_amend_cfg[uc_rf_idx];
        cfg_id = (uc_rf_idx == WLAN_RF_CHANNEL_ZERO) ?
            WLAN_CFG_INIT_RF_AMEND_RSSI_2G_C0 : WLAN_CFG_INIT_RF_AMEND_RSSI_2G_C1;
        l_pwr_ref_delta = hwifi_get_init_value(CUS_TAG_INI, cfg_id);
        rssi = pst_rssi_amend_ref->ac_cfg_delta_amend_rssi_2g;
        rssi[0] = cus_val_valid((int8_t)cus_get_first_byte(l_pwr_ref_delta), WLAN_RF_RSSI_AMEND_TH_HIGH,
            WLAN_RF_RSSI_AMEND_TH_LOW) ? (int8_t)cus_get_first_byte(l_pwr_ref_delta) : 0;
        rssi[1] = cus_val_valid((int8_t)cus_get_second_byte(l_pwr_ref_delta), WLAN_RF_RSSI_AMEND_TH_HIGH,
            WLAN_RF_RSSI_AMEND_TH_LOW) ? (int8_t)cus_get_second_byte(l_pwr_ref_delta) : 0;
        rssi[BYTE_OFFSET_2] = cus_val_valid((int8_t)cus_get_third_byte(l_pwr_ref_delta), WLAN_RF_RSSI_AMEND_TH_HIGH,
            WLAN_RF_RSSI_AMEND_TH_LOW) ? (int8_t)cus_get_third_byte(l_pwr_ref_delta) : 0;
        cfg_id = (uc_rf_idx == WLAN_RF_CHANNEL_ZERO) ?
            WLAN_CFG_INIT_RF_AMEND_RSSI_5G_C0 : WLAN_CFG_INIT_RF_AMEND_RSSI_5G_C1;
        l_pwr_ref_delta = hwifi_get_init_value(CUS_TAG_INI, cfg_id);
        rssi = pst_rssi_amend_ref->ac_cfg_delta_amend_rssi_5g;
        rssi[0] = cus_val_valid((int8_t)cus_get_first_byte(l_pwr_ref_delta), WLAN_RF_RSSI_AMEND_TH_HIGH,
            WLAN_RF_RSSI_AMEND_TH_LOW) ? (int8_t)cus_get_first_byte(l_pwr_ref_delta) : 0;
        rssi[1] = cus_val_valid((int8_t)cus_get_second_byte(l_pwr_ref_delta), WLAN_RF_RSSI_AMEND_TH_HIGH,
            WLAN_RF_RSSI_AMEND_TH_LOW) ? (int8_t)cus_get_second_byte(l_pwr_ref_delta) : 0;
        rssi[BYTE_OFFSET_2] = cus_val_valid((int8_t)cus_get_third_byte(l_pwr_ref_delta), WLAN_RF_RSSI_AMEND_TH_HIGH,
            WLAN_RF_RSSI_AMEND_TH_LOW) ? (int8_t)cus_get_third_byte(l_pwr_ref_delta) : 0;
        rssi[BYTE_OFFSET_3] = cus_val_valid((int8_t)cus_get_fourth_byte(l_pwr_ref_delta), WLAN_RF_RSSI_AMEND_TH_HIGH,
            WLAN_RF_RSSI_AMEND_TH_LOW) ? (int8_t)cus_get_fourth_byte(l_pwr_ref_delta) : 0;
    }
}


static int8_t hwifi_get_valid_amend_rssi_val(int8_t pwr_ref_delta)
{
    int8_t rssi_amend_val;
    rssi_amend_val = cus_val_valid(pwr_ref_delta, WLAN_RF_RSSI_AMEND_TH_HIGH, WLAN_RF_RSSI_AMEND_TH_LOW) ?
        pwr_ref_delta : 0;
    return rssi_amend_val;
}


static void hwifi_cfg_filter_narrow_ref_delta(mac_cfg_customize_rf *pst_customize_rf)
{
    uint8_t uc_rf_idx;
    wlan_cfg_init cfg_id;
    int32_t l_pwr_ref_delta;
    mac_cfg_custom_filter_narrow_amend_rssi_stru *filter_narrow_rssi_amend = NULL;

    for (uc_rf_idx = 0; uc_rf_idx < WLAN_RF_CHANNEL_NUMS; uc_rf_idx++) {
        filter_narrow_rssi_amend = &pst_customize_rf->filter_narrow_rssi_amend[uc_rf_idx];
        cfg_id = (uc_rf_idx == WLAN_RF_CHANNEL_ZERO) ? WLAN_CFG_INIT_RF_FILTER_NARROW_RSSI_AMEND_2G_C0 :
            WLAN_CFG_INIT_RF_FILTER_NARROW_RSSI_AMEND_2G_C1;
        l_pwr_ref_delta = hwifi_get_init_value(CUS_TAG_INI, cfg_id);
        filter_narrow_rssi_amend->filter_narrowing_amend_rssi_2g[0] =
            hwifi_get_valid_amend_rssi_val((int8_t)cus_get_first_byte(l_pwr_ref_delta));
        filter_narrow_rssi_amend->filter_narrowing_amend_rssi_2g[1] =
            hwifi_get_valid_amend_rssi_val((int8_t)cus_get_second_byte(l_pwr_ref_delta));
        cfg_id = (uc_rf_idx == WLAN_RF_CHANNEL_ZERO) ? WLAN_CFG_INIT_RF_FILTER_NARROW_RSSI_AMEND_5G_C0 :
            WLAN_CFG_INIT_RF_FILTER_NARROW_RSSI_AMEND_5G_C1;
        l_pwr_ref_delta = hwifi_get_init_value(CUS_TAG_INI, cfg_id);
        filter_narrow_rssi_amend->filter_narrowing_amend_rssi_5g[0] =
            hwifi_get_valid_amend_rssi_val((int8_t)cus_get_first_byte(l_pwr_ref_delta));
        filter_narrow_rssi_amend->filter_narrowing_amend_rssi_5g[1] =
            hwifi_get_valid_amend_rssi_val((int8_t)cus_get_second_byte(l_pwr_ref_delta));
        filter_narrow_rssi_amend->filter_narrowing_amend_rssi_5g[2] = /* rssi修正5G phy mode160M 数组下标2 */
            hwifi_get_valid_amend_rssi_val((int8_t)cus_get_third_byte(l_pwr_ref_delta));
    }
}

OAL_STATIC uint32_t hwifi_cfg_front_end(uint8_t *puc_param)
{
    mac_cfg_customize_rf *pst_customize_rf;

    pst_customize_rf = (mac_cfg_customize_rf *)puc_param;
    memset_s(pst_customize_rf, sizeof(mac_cfg_customize_rf), 0, sizeof(mac_cfg_customize_rf));

    /* 配置: 2g rf */
    if (hwifi_cfg_front_end_set_2g_rf(pst_customize_rf) != OAL_SUCC) {
        return OAL_FAIL;
    }

    hwifi_cfg_pwr_ref_delta(pst_customize_rf);
    hwifi_cfg_filter_narrow_ref_delta(pst_customize_rf);

    /* 通道radio cap */
    pst_customize_rf->uc_chn_radio_cap = (uint8_t)hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_CHANN_RADIO_CAP);

    /* 2g 外部fem */
    if (hwifi_cfg_front_end_2g_fem(pst_customize_rf) != OAL_SUCC) {
        return OAL_FAIL;
    }

    if (OAL_TRUE == mac_device_check_5g_enable_per_chip()) {
        /* 配置: 5g rf */
        /* 配置: fem口到天线口的负增益 */
        if (hwifi_cfg_front_end_set_5g_rf(pst_customize_rf) != OAL_SUCC) {
            return OAL_FAIL;
        }
        /* 5g 外部fem */
        /* RF0 */
        if (hwifi_cfg_front_end_5g_fem(pst_customize_rf) != OAL_SUCC) {
            return OAL_FAIL;
        }
    }

    pst_customize_rf->uc_far_dist_pow_gain_switch =
        (uint8_t) !!hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_FAR_DIST_POW_GAIN_SWITCH);
    pst_customize_rf->uc_far_dist_dsss_scale_promote_switch =
        (uint8_t) !!hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_FAR_DIST_DSSS_SCALE_PROMOTE_SWITCH);

    /* 配置: cca能量门限调整值 */
    hwifi_set_cca_energy_thrsehold(pst_customize_rf);

    return OAL_SUCC;
}


OAL_STATIC void hwifi_config_init_ini_rf(oal_net_device_stru *pst_cfg_net_dev)
{
    wal_msg_write_stru st_write_msg;
    uint32_t ret;
    uint16_t us_event_len = sizeof(mac_cfg_customize_rf);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_CUS_RF, us_event_len);

    /*lint -e774*/
    /* 定制化下发不能超过事件内存长 */
    if (us_event_len > WAL_MSG_WRITE_MAX_LEN) {
        oam_error_log2(0, OAM_SF_ANY, "{hwifi_config_init_ini_rf::event size[%d] larger than msg size[%d]!}",
                       us_event_len, WAL_MSG_WRITE_MAX_LEN);
        return;
    }
    /*lint +e774*/
    /*  */
    ret = hwifi_cfg_front_end(st_write_msg.auc_value);
    if (ret != OAL_SUCC) {
        oam_error_log0(0, OAM_SF_ANY, "{hwifi_config_init_ini_rf::front end rf wrong value, not send cfg!}");
        return;
    }

    /* 如果所有参数都在有效范围内，则下发配置值 */
    ret = (uint32_t)wal_send_cfg_event(pst_cfg_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + \
        us_event_len, (uint8_t *)&st_write_msg,  OAL_FALSE, NULL);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_error_log1(0, OAM_SF_ANY, "{hwifi_config_init_ini_rf::EVENT[wal_send_cfg_event] fail[%d]!}", ret);
    }
}



OAL_STATIC void hwifi_config_init_ini_log(oal_net_device_stru *pst_cfg_net_dev)
{
    wal_msg_write_stru st_write_msg;
    int32_t l_ret;
    int32_t l_loglevel;

    /* log_level */
    l_loglevel = hwifi_get_init_value(CUS_TAG_INI, WLAN_CFG_INIT_LOGLEVEL);
    if (l_loglevel < OAM_LOG_LEVEL_ERROR ||
        l_loglevel > OAM_LOG_LEVEL_INFO) {
        oam_error_log3(0, OAM_SF_ANY, "{hwifi_config_init_ini_clock::loglevel[%d] out of range[%d,%d], check ini file}",
                       l_loglevel, OAM_LOG_LEVEL_ERROR, OAM_LOG_LEVEL_INFO);
        return;
    }

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_ALL_LOG_LEVEL, sizeof(int32_t));
    *((int32_t *)(st_write_msg.auc_value)) = l_loglevel;
    l_ret = wal_send_cfg_event(pst_cfg_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + sizeof(int32_t),
                               (uint8_t *)&st_write_msg, OAL_FALSE, NULL);
    if (oal_unlikely(l_ret != OAL_SUCC)) {
        oam_error_log1(0, OAM_SF_ANY, "{hwifi_config_init_ini_log::return err code[%d]!}\r\n", l_ret);
    }
}


OAL_STATIC void hwifi_config_init_ini_main_1103(oal_net_device_stru *pst_cfg_net_dev)
{
    /* 国家码 */
    hwifi_config_init_ini_country(pst_cfg_net_dev);
#ifdef _PRE_WLAN_COUNTRYCODE_SELFSTUDY
    /* 自学习国家码初始化 */
    hwifi_config_selfstudy_init_country(pst_cfg_net_dev);
#endif
    /* 可维可测 */
    hwifi_config_init_ini_log(pst_cfg_net_dev);
    /* RF */
    hwifi_config_init_ini_rf(pst_cfg_net_dev);
}

void wal_send_cali_data_1103(oal_net_device_stru *cfg_net_dev)
{
    hmac_send_cali_data_1103(oal_net_dev_priv(cfg_net_dev), WLAN_SCAN_ALL_CHN);
}

void wal_send_cali_data_1105(oal_net_device_stru *cfg_net_dev)
{
    hmac_send_cali_data_1105(oal_net_dev_priv(cfg_net_dev), WLAN_SCAN_ALL_CHN);
}


uint32_t wal_custom_cali_1103(void)
{
    oal_net_device_stru *pst_net_dev;
    uint32_t ret;

    pst_net_dev = wal_config_get_netdev("Hisilicon0", OAL_STRLEN("Hisilicon0"));  // 通过cfg vap0来下c0 c1校准
    if (oal_warn_on(pst_net_dev == NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    } else {
        /* 调用oal_dev_get_by_name后，必须调用oal_dev_put使net_dev的引用计数减一 */
        oal_dev_put(pst_net_dev);
        oam_warning_log0(0, OAM_SF_ANY, "{wal_custom_cali::the net_device is already exist!}");
    }

    if (hwifi_config_init_nvram_main_1103(pst_net_dev) != OAL_SUCC) {
        oam_warning_log0(0, OAM_SF_ANY, "{wal_custom_cali::init_nvram fail!}");
    }

    hwifi_config_init_ini_main_1103(pst_net_dev);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if (g_custom_cali_done == OAL_TRUE) {
        /* 校准数据下发 */
        wlan_chip_send_cali_data(pst_net_dev);
    } else {
        g_custom_cali_done = OAL_TRUE;
    }

    wal_send_cali_matrix_data(pst_net_dev);
#endif

    /* 下发参数 */
    ret = hwifi_config_init_dts_main(pst_net_dev);
    if (ret != OAL_SUCC) {
        oam_error_log0(0, OAM_SF_ANY, "{wal_custom_cali:init_dts_main fail!}");
    }

    return ret;
}

OAL_STATIC uint32_t hwifi_cfg_init_cus_5g_160m_dpn_cali(mac_cus_dy_cali_param_stru *dyn_cali_param,
    int8_t *pc_ctx, uint8_t rf_idx)
{
    uint8_t  uc_dpn_5g_nv_id = WLAN_CFG_DTS_NVRAM_MUFREQ_5G160_C0;
    uint8_t  nv_pa_params[CUS_PARAMS_LEN_MAX] = { 0 };
    int8_t   dpn_5g_nv[OAL_5G_160M_CHANNEL_NUM];
    uint8_t *pc_end = ";";
    uint8_t *pc_sep = ",";
    int8_t  *pc_token = NULL;
    uint8_t *cust_nvram_info = NULL;
    uint8_t  num_idx;
    int32_t  val;

    cust_nvram_info = hwifi_get_nvram_param(uc_dpn_5g_nv_id);
    uc_dpn_5g_nv_id++;
    if (OAL_STRLEN(cust_nvram_info)) {
        memset_s(nv_pa_params, sizeof(nv_pa_params), 0, sizeof(nv_pa_params));
        if (memcpy_s(nv_pa_params, sizeof(nv_pa_params),
                     cust_nvram_info, OAL_STRLEN(cust_nvram_info)) != EOK) {
            oam_error_log0(0, OAM_SF_CUSTOM, "hwifi_cfg_init_cus_5g_160m_dpn_cali::memcpy fail!");
        }
        pc_token = oal_strtok(nv_pa_params, pc_end, &pc_ctx);
        pc_token = oal_strtok(pc_token, pc_sep, &pc_ctx);
        num_idx = 0;
        while ((pc_token != NULL)) {
            if (num_idx >= OAL_5G_160M_CHANNEL_NUM) {
                num_idx++;
                break;
            }
            val = oal_strtol(pc_token, NULL, 10) / 10; /* 10表示十进制 */
            pc_token = oal_strtok(NULL, pc_sep, &pc_ctx);
            if (oal_value_not_in_valid_range(val, CUS_DY_CALI_5G_VAL_DPN_MIN, CUS_DY_CALI_5G_VAL_DPN_MAX)) {
                oam_error_log3(0, OAM_SF_CUSTOM, "{hwifi_cfg_init_cus_dyn_cali::nvram 2g dpn val[%d]\
                    unexpect:idx[%d] num_idx[%d}}", val, MAC_NUM_5G_BAND, num_idx);
                val = 0;
            }
            dpn_5g_nv[num_idx] = (int8_t)val;
            num_idx++;
        }

        if (num_idx != OAL_5G_160M_CHANNEL_NUM) {
            oam_error_log2(0, OAM_SF_CUSTOM,
                "{hwifi_cfg_init_cus_dyn_cali::nvram 2g dpn num unexpected id[%d] rf[%d}}", MAC_NUM_5G_BAND, rf_idx);
            return OAL_FAIL;
        }
        /* 5250  5570 */
        for (num_idx = 0; num_idx < OAL_5G_160M_CHANNEL_NUM; num_idx++) {
            dyn_cali_param->ac_dy_cali_5g_dpn_params[num_idx + 1][WLAN_BW_CAP_160M] += dpn_5g_nv[num_idx];
        }
    }
    return OAL_SUCC;
}

OAL_STATIC void hwifi_cfg_init_cus_dpn_cali(mac_cus_dy_cali_param_stru *dyn_cali_param, int8_t *pc_ctx, uint8_t rf_idx)
{
    uint8_t  idx, num_idx;
    int32_t  val;
    uint8_t *cust_nvram_info = NULL;
    uint8_t  dpn_2g_nv_id = WLAN_CFG_DTS_NVRAM_MUFREQ_2GCCK_C0;
    uint8_t  nv_pa_params[CUS_PARAMS_LEN_MAX] = { 0 };
    int8_t   ac_dpn_nv[HWIFI_CFG_DYN_PWR_CALI_2G_SNGL_MODE_CW][MAC_2G_CHANNEL_NUM];
    uint8_t *pc_end = ";";
    uint8_t *pc_sep = ",";
    int8_t  *pc_token = NULL;

    for (idx = HWIFI_CFG_DYN_PWR_CALI_2G_SNGL_MODE_11B;
        idx <= HWIFI_CFG_DYN_PWR_CALI_2G_SNGL_MODE_OFDM40; idx++) {
        /* 获取产线计算DPN值修正 */
        cust_nvram_info = hwifi_get_nvram_param(dpn_2g_nv_id);
        dpn_2g_nv_id++;

        if (0 == OAL_STRLEN(cust_nvram_info)) {
            continue;
        }

        memset_s(nv_pa_params, sizeof(nv_pa_params), 0, sizeof(nv_pa_params));
        if (memcpy_s(nv_pa_params, sizeof(nv_pa_params), cust_nvram_info, OAL_STRLEN(cust_nvram_info)) != EOK) {
            oam_error_log0(0, OAM_SF_CUSTOM, "hwifi_cfg_init_cus_dpn_cali::memcpy fail!");
        }
        pc_token = oal_strtok(nv_pa_params, pc_end, &pc_ctx);
        pc_token = oal_strtok(pc_token, pc_sep, &pc_ctx);
        num_idx = 0;
        while ((pc_token != NULL)) {
            if (num_idx >= MAC_2G_CHANNEL_NUM) {
                num_idx++;
                break;
            }
            val = oal_strtol(pc_token, NULL, 10) / 10; /* 10表示十进制 */
            pc_token = oal_strtok(NULL, pc_sep, &pc_ctx);
            if (oal_value_not_in_valid_range(val, CUS_DY_CALI_2G_VAL_DPN_MIN, CUS_DY_CALI_2G_VAL_DPN_MAX)) {
                oam_error_log3(0, OAM_SF_CUSTOM, "{hwifi_cfg_init_cus_dyn_cali::nvram 2g dpn val[%d]\
                    unexpected idx[%d] num_idx[%d}!}", val, idx, num_idx);
                val = 0;
            }
            ac_dpn_nv[idx][num_idx] = (int8_t)val;
            num_idx++;
        }

        if (num_idx != MAC_2G_CHANNEL_NUM) {
            oam_error_log2(0, OAM_SF_CUSTOM,
                "{hwifi_cfg_init_cus_dyn_cali::nvram 2g dpn num is unexpect uc_id[%d] rf[%d}}", idx, rf_idx);
            continue;
        }

        for (num_idx = 0; num_idx < MAC_2G_CHANNEL_NUM; num_idx++) {
            dyn_cali_param->ac_dy_cali_2g_dpn_params[num_idx][idx] += ac_dpn_nv[idx][num_idx];
        }
    }
}


OAL_STATIC uint32_t hwifi_cfg_init_cus_dyn_cali(mac_cus_dy_cali_param_stru *puc_dyn_cali_param, int num)
{
    int32_t  val, ret;
    uint8_t  uc_idx = 0;
    uint8_t  uc_rf_idx, uc_dy_cal_param_idx;
    uint8_t  uc_cfg_id = WLAN_CFG_DTS_2G_CORE0_DPN_CH1;
    int8_t  *pc_ctx = NULL;

    for (uc_rf_idx = 0; uc_rf_idx < num; uc_rf_idx++) {
        puc_dyn_cali_param->uc_rf_id = uc_rf_idx;

        /* 动态校准二次项系数入参检查 */
        for (uc_dy_cal_param_idx = 0; uc_dy_cal_param_idx < DY_CALI_PARAMS_NUM; uc_dy_cal_param_idx++) {
            if (!g_pro_line_params[uc_rf_idx][uc_dy_cal_param_idx].l_pow_par2) {
                oam_error_log1(0, OAM_SF_CUSTOM, "{hwifi_cfg_init_cus_dyn_cali::unexpect val[%d] s_pow_par2[0]!}",
                    uc_dy_cal_param_idx);
                return OAL_FAIL;
            }
        }
        ret = memcpy_s(puc_dyn_cali_param->al_dy_cali_base_ratio_params,
                       sizeof(puc_dyn_cali_param->al_dy_cali_base_ratio_params),
                       g_pro_line_params[uc_rf_idx], sizeof(puc_dyn_cali_param->al_dy_cali_base_ratio_params));

        ret += memcpy_s(puc_dyn_cali_param->al_dy_cali_base_ratio_ppa_params,
                        sizeof(puc_dyn_cali_param->al_dy_cali_base_ratio_ppa_params),
                        &g_pro_line_params[uc_rf_idx][CUS_DY_CALI_PARAMS_NUM],
                        sizeof(puc_dyn_cali_param->al_dy_cali_base_ratio_ppa_params));

        ret += memcpy_s(puc_dyn_cali_param->as_extre_point_val, sizeof(puc_dyn_cali_param->as_extre_point_val),
                        g_gs_extre_point_vals[uc_rf_idx], sizeof(puc_dyn_cali_param->as_extre_point_val));

        /* DPN */
        for (uc_idx = 0; uc_idx < MAC_2G_CHANNEL_NUM; uc_idx++) {
            val = hwifi_get_init_value(CUS_TAG_DTS, uc_cfg_id + uc_idx);
            ret += memcpy_s(puc_dyn_cali_param->ac_dy_cali_2g_dpn_params[uc_idx],
                CUS_DY_CALI_DPN_PARAMS_NUM * sizeof(int8_t), &val, CUS_DY_CALI_DPN_PARAMS_NUM * sizeof(int8_t));
        }
        uc_cfg_id += MAC_2G_CHANNEL_NUM;
        hwifi_cfg_init_cus_dpn_cali(puc_dyn_cali_param, pc_ctx, uc_rf_idx);

        for (uc_idx = 0; uc_idx < MAC_NUM_5G_BAND; uc_idx++) {
            val = hwifi_get_init_value(CUS_TAG_DTS, uc_cfg_id + uc_idx);
            ret += memcpy_s(puc_dyn_cali_param->ac_dy_cali_5g_dpn_params[uc_idx],
                CUS_DY_CALI_DPN_PARAMS_NUM * sizeof(int8_t), &val, CUS_DY_CALI_DPN_PARAMS_NUM * sizeof(int8_t));
        }
        uc_cfg_id += MAC_NUM_5G_BAND;

        /* 5G 160M DPN */
        if (hwifi_cfg_init_cus_5g_160m_dpn_cali(puc_dyn_cali_param, pc_ctx, uc_rf_idx) != OAL_SUCC) {
            continue;
        }
        puc_dyn_cali_param++;
    }
    if (ret != EOK) {
        oam_error_log0(0, OAM_SF_CUSTOM, "hwifi_cfg_init_cus_dyn_cali::memcpy fail!");
        return OAL_FAIL;
    }
    return OAL_SUCC;
}


void hwifi_config_init_cus_dyn_cali(oal_net_device_stru *pst_cfg_net_dev)
{
    wal_msg_write_stru st_write_msg;
    uint32_t ret;
    uint32_t offset = 0;
    mac_cus_dy_cali_param_stru st_dy_cus_cali[WLAN_RF_CHANNEL_NUMS];
    uint8_t uc_rf_id;
    mac_cus_dy_cali_param_stru *pst_dy_cus_cali = NULL;
    wal_msg_stru *pst_rsp_msg = NULL;
    wal_msg_write_rsp_stru *pst_write_rsp_msg = NULL;
    int32_t l_ret;

    if (oal_warn_on(pst_cfg_net_dev == NULL)) {
        return;
    }

    /* 配置动态校准参数TXPWR_PA_DC_REF */
    memset_s(st_dy_cus_cali, sizeof(mac_cus_dy_cali_param_stru) * WLAN_RF_CHANNEL_NUMS,
             0, sizeof(mac_cus_dy_cali_param_stru) * WLAN_RF_CHANNEL_NUMS);

    ret = hwifi_cfg_init_cus_dyn_cali(st_dy_cus_cali, WLAN_RF_CHANNEL_NUMS);
    if (oal_unlikely(ret != OAL_SUCC)) {
        return;
    }

    for (uc_rf_id = 0; uc_rf_id < WLAN_RF_CHANNEL_NUMS; uc_rf_id++) {
        pst_dy_cus_cali = &st_dy_cus_cali[uc_rf_id];
        pst_rsp_msg = NULL;

        /* 如果所有参数都在有效范围内，则下发配置值 */
        l_ret = memcpy_s(st_write_msg.auc_value, sizeof(st_write_msg.auc_value),
                         (int8_t *)pst_dy_cus_cali, sizeof(mac_cus_dy_cali_param_stru));
        if (l_ret != EOK) {
            oam_error_log0(0, OAM_SF_CFG, "hwifi_config_init_cus_dyn_cali::memcpy fail!");
            return;
        }

        offset = sizeof(mac_cus_dy_cali_param_stru);
        WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_CUS_DYN_CALI_PARAM, offset);

        l_ret = wal_send_cfg_event(pst_cfg_net_dev, WAL_MSG_TYPE_WRITE,
            WAL_MSG_WRITE_MSG_HDR_LENGTH + offset, (uint8_t *)&st_write_msg, OAL_TRUE, &pst_rsp_msg);
        if (oal_unlikely(ret != OAL_SUCC)) {
            return;
        }

        if (pst_rsp_msg != NULL) {
            pst_write_rsp_msg = (wal_msg_write_rsp_stru *)(pst_rsp_msg->auc_msg_data);
            if (pst_write_rsp_msg->err_code != OAL_SUCC) {
                oam_error_log2(0, OAM_SF_SCAN, "{wal_check_and_release_msg_resp::detect err code:[%u],wid:[%u]}",
                               pst_write_rsp_msg->err_code, pst_write_rsp_msg->en_wid);
                oal_free(pst_rsp_msg);
                return;
            }

            oal_free(pst_rsp_msg);
        }
    }

    return;
}


uint32_t hwifi_get_sar_ctrl_params_1103(uint8_t lvl_num, uint8_t *data_addr,
    uint16_t *data_len, uint16_t dest_len)
{
    wlan_init_cust_nvram_params *cust_nv_params = (wlan_init_cust_nvram_params *)hwifi_get_init_nvram_params();
    *data_len = sizeof(wlan_cust_sar_ctrl_stru) * CUS_NUM_OF_SAR_PARAMS;
    if ((lvl_num <= CUS_NUM_OF_SAR_LVL) && (lvl_num > 0)) {
        lvl_num--;
        if (EOK != memcpy_s(data_addr, dest_len, cust_nv_params->st_sar_ctrl_params[lvl_num], *data_len)) {
            oam_error_log0(0, OAM_SF_CFG, "hwifi_get_sar_ctrl_params_1103::memcpy fail!");
            return OAL_FAIL;
        }
    } else {
        memset_s(data_addr, dest_len, 0xFF, dest_len);
    }
    return OAL_SUCC;
}
