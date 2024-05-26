/*
 * 版权所有 (c) 华为技术有限公司 2013-2018
 * 功能说明 : 配置相关实现hmac接口实现源文件
 * 作    者 : zourong
 * 创建日期 : 2013年1月8日
 */

/* 1 头文件包含 */
#include "hmac_config.h"
#include "hmac_cali_mgmt.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CONFIG_MIB_C

/*
 * 函 数 名  : hmac_config_set_bss_type
 * 功能描述  : 设置bss type
 * 1.日    期  : 2013年1月21日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
uint32_t hmac_config_set_bss_type(mac_vap_stru *pst_mac_vap, uint16_t us_len, uint8_t *puc_param)
{
    /* 设置mib值 */
    mac_mib_set_bss_type(pst_mac_vap, (uint8_t)us_len, puc_param);

    return OAL_SUCC;
}

/*
 * 函 数 名  : hmac_config_get_bss_type
 * 功能描述  : 获取bss type
 * 1.日    期  : 2012年12月24日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
uint32_t hmac_config_get_bss_type(mac_vap_stru *pst_mac_vap, uint16_t *pus_len, uint8_t *puc_param)
{
    /* 读取mib值 */
    return mac_mib_get_bss_type(pst_mac_vap, (uint8_t *)pus_len, puc_param);
}

/*
 * 函 数 名  : hmac_config_set_mac_addr
 * 功能描述  : 设置协议模式
 * 1.日    期  : 2012年12月24日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
uint32_t hmac_config_set_mac_addr(mac_vap_stru *pst_mac_vap, uint16_t us_len, uint8_t *puc_param)
{
    mac_cfg_staion_id_param_stru *pst_station_id_param = NULL;
    wlan_p2p_mode_enum_uint8 en_p2p_mode;
    uint32_t ret;

    if (pst_mac_vap->pst_mib_info == NULL) {
        oam_error_log0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_set_mac_addr::vap->mib_info is NULL !}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* P2P 设置MAC 地址mib 值需要区分P2P DEV 或P2P_CL/P2P_GO,P2P_DEV MAC 地址设置到p2p0 MIB 中 */
    pst_station_id_param = (mac_cfg_staion_id_param_stru *)puc_param;
    en_p2p_mode = pst_station_id_param->en_p2p_mode;
    if (en_p2p_mode == WLAN_P2P_DEV_MODE) {
        /* 如果是p2p0 device，则配置MAC 地址到auc_p2p0_dot11StationID 成员中 */
        oal_set_mac_addr(mac_mib_get_p2p0_dot11StationID(pst_mac_vap), pst_station_id_param->auc_station_id);
    } else {
        /* 设置mib值, Station_ID */
        mac_mib_set_station_id(pst_mac_vap, (uint8_t)us_len, puc_param);
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_STATION_ID, us_len, puc_param);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_warning_log1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mac_addr::send_event failed[%d].}", ret);
    }

    return ret;
}

/*
 * 函 数 名  : hmac_config_get_ssid
 * 功能描述  : hmac读SSID
 * 1.日    期  : 2013年1月15日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
uint32_t hmac_config_get_ssid(mac_vap_stru *pst_mac_vap, uint16_t *pus_len, uint8_t *puc_param)
{
    /* 读取mib值 */
    return mac_mib_get_ssid(pst_mac_vap, (uint8_t *)pus_len, puc_param);
}

/*
 * 函 数 名  : hmac_config_set_ssid
 * 功能描述  : hmac设SSID
 * 1.日    期  : 2013年1月15日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
uint32_t hmac_config_set_ssid(mac_vap_stru *pst_mac_vap, uint16_t us_len, uint8_t *puc_param)
{
    /* 设置mib值 */
    mac_mib_set_ssid(pst_mac_vap, (uint8_t)us_len, puc_param);

    return hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SSID, us_len, puc_param);
}

/*
 * 函 数 名  : hmac_config_set_shpreamble
 * 功能描述  : 设置短前导码能力位
 * 1.日    期  : 2013年1月21日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
uint32_t hmac_config_set_shpreamble(mac_vap_stru *pst_mac_vap, uint16_t us_len, uint8_t *puc_param)
{
    uint32_t ret;

    /* 设置mib值 */
    mac_mib_set_shpreamble(pst_mac_vap, (uint8_t)us_len, puc_param);

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SHORT_PREAMBLE, us_len, puc_param);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_warning_log1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_config_set_concurrent::hmac_config_send_event failed[%d].}", ret);
    }

    return ret;
}

/*
 * 函 数 名  : hmac_config_get_shpreamble
 * 功能描述  : 读前导码能力位
 * 1.日    期  : 2013年1月15日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
uint32_t hmac_config_get_shpreamble(mac_vap_stru *pst_mac_vap, uint16_t *pus_len, uint8_t *puc_param)
{
    /* 读mib值 */
    return mac_mib_get_shpreamble(pst_mac_vap, (uint8_t *)pus_len, puc_param);
}

/*
 * 函 数 名  : hmac_config_set_shortgi20
 * 功能描述  : 20M short gi能力设置
 * 1.日    期  : 2013年1月15日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
uint32_t hmac_config_set_shortgi20(mac_vap_stru *pst_mac_vap, uint16_t us_len, uint8_t *puc_param)
{
    int32_t l_value;
    uint32_t ret;
    shortgi_cfg_stru shortgi_cfg;

    shortgi_cfg.uc_shortgi_type = SHORTGI_20_CFG_ENUM;
    l_value = *((int32_t *)puc_param);

    if (l_value != 0) {
        shortgi_cfg.uc_enable = OAL_TRUE;
        mac_mib_set_ShortGIOptionInTwentyImplemented(pst_mac_vap, OAL_TRUE);
    } else {
        shortgi_cfg.uc_enable = OAL_FALSE;
        mac_mib_set_ShortGIOptionInTwentyImplemented(pst_mac_vap, OAL_FALSE);
    }

    /* 配置事件的子事件 WLAN_CFGID_SHORTGI 通过新加的接口函数取出关键数据存入skb后通过sdio发出 */
    ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SHORTGI, SHORTGI_CFG_STRU_LEN, (uint8_t *)&shortgi_cfg);
    if (ret != OAL_SUCC) {
        oam_warning_log1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_config_set_shortgi20::hmac_config_send_event failed[%u].}", ret);
    }

    return ret;
}

/*
 * 函 数 名  : hmac_config_set_shortgi40
 * 功能描述  : 设置40M short gi能力
 * 1.日    期  : 2014年5月9日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
uint32_t hmac_config_set_shortgi40(mac_vap_stru *pst_mac_vap, uint16_t us_len, uint8_t *puc_param)
{
    int32_t l_value;
    uint32_t ret;
    shortgi_cfg_stru shortgi_cfg;

    shortgi_cfg.uc_shortgi_type = SHORTGI_40_CFG_ENUM;
    l_value = *((int32_t *)puc_param);

    if (l_value != 0) {
        shortgi_cfg.uc_enable = OAL_TRUE;
        mac_mib_set_ShortGIOptionInFortyImplemented(pst_mac_vap, OAL_TRUE);
    } else {
        shortgi_cfg.uc_enable = OAL_FALSE;
        mac_mib_set_ShortGIOptionInFortyImplemented(pst_mac_vap, OAL_FALSE);
    }

    /* ======================================================================== */
    /* hi1102-cb : Need to send to Dmac via sdio */
    /* 配置事件的子事件 WLAN_CFGID_SHORTGI 通过新加的接口函数取出关键数据存入skb后通过sdio发出 */
    ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SHORTGI, SHORTGI_CFG_STRU_LEN, (uint8_t *)&shortgi_cfg);
    if (ret != OAL_SUCC) {
        oam_warning_log1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_config_set_shortgi40::hmac_config_send_event failed[%u].}", ret);
    }
    /* ======================================================================== */
    return ret;
}

/*
 * 函 数 名  : hmac_config_set_shortgi80
 * 功能描述  : 设置80M short gi能力
 * 1.日    期  : 2014年5月9日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
uint32_t hmac_config_set_shortgi80(mac_vap_stru *pst_mac_vap, uint16_t us_len, uint8_t *puc_param)
{
    int32_t l_value;
    uint32_t ret;
    shortgi_cfg_stru shortgi_cfg;

    shortgi_cfg.uc_shortgi_type = SHORTGI_40_CFG_ENUM;

    l_value = *((int32_t *)puc_param);

    if (l_value != 0) {
        shortgi_cfg.uc_enable = OAL_TRUE;
        mac_mib_set_VHTShortGIOptionIn80Implemented(pst_mac_vap, OAL_TRUE);
    } else {
        shortgi_cfg.uc_enable = OAL_FALSE;
        mac_mib_set_VHTShortGIOptionIn80Implemented(pst_mac_vap, OAL_FALSE);
    }

    /* ======================================================================== */
    /* hi1102-cb : Need to send to Dmac via sdio */
    /* 配置事件的子事件 WLAN_CFGID_SHORTGI 通过新加的接口函数取出关键数据存入skb后通过sdio发出 */
    ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SHORTGI, SHORTGI_CFG_STRU_LEN, (uint8_t *)&shortgi_cfg);
    if (ret != OAL_SUCC) {
        oam_warning_log1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_config_set_shortgi80::hmac_config_send_event failed[%u].}", ret);
    }
    /* ======================================================================== */
    return ret;
}

/*
 * 函 数 名  : hmac_config_get_shortgi20
 * 功能描述  : 读取short gi
 * 1.日    期  : 2013年1月15日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
uint32_t hmac_config_get_shortgi20(mac_vap_stru *pst_mac_vap, uint16_t *pus_len, uint8_t *puc_param)
{
    int32_t value;

    value = mac_mib_get_ShortGIOptionInTwentyImplemented(pst_mac_vap);

    *((int32_t *)puc_param) = value;

    *pus_len = sizeof(value);

    return OAL_SUCC;
}

/*
 * 函 数 名  : hmac_config_get_shortgi40
 * 功能描述  : 读取short gi
 * 1.日    期  : 2013年1月15日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
uint32_t hmac_config_get_shortgi40(mac_vap_stru *pst_mac_vap, uint16_t *pus_len, uint8_t *puc_param)
{
    int32_t value;

    value = (int32_t)mac_mib_get_ShortGIOptionInFortyImplemented(pst_mac_vap);

    *((int32_t *)puc_param) = value;

    *pus_len = sizeof(value);

    return OAL_SUCC;
}

/*
 * 函 数 名  : hmac_config_get_shortgi80
 * 功能描述  : 读取short gi
 * 1.日    期  : 2013年1月15日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
uint32_t hmac_config_get_shortgi80(mac_vap_stru *pst_mac_vap, uint16_t *pus_len, uint8_t *puc_param)
{
    int32_t value;

    value = mac_mib_get_VHTShortGIOptionIn80Implemented(pst_mac_vap);

    *((int32_t *)puc_param) = value;

    *pus_len = sizeof(value);

    return OAL_SUCC;
}

/*
 * 函 数 名  : hmac_config_set_bintval
 * 功能描述  : 设置beacon interval
 * 1.日    期  : 2013年1月15日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
uint32_t hmac_config_set_bintval(mac_vap_stru *pst_mac_vap, uint16_t us_len, uint8_t *puc_param)
{
    uint32_t ret;
    mac_device_stru *pst_mac_device;
    uint8_t uc_vap_idx;
    mac_vap_stru *pst_vap = NULL;

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (pst_mac_device == NULL) {
        oam_error_log1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                       "{hmac_config_set_bintval::mac_res_get_dev fail.device_id = %u}",
                       pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置device下的值 */
    mac_device_set_beacon_interval(pst_mac_device, *((uint32_t *)puc_param));

    /* 遍历device下所有vap */
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++) {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (pst_vap == NULL) {
            oam_error_log1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_config_set_bintval::pst_mac_vap(%d) null.}",
                           pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        /* 只有AP VAP需要beacon interval */
        if (pst_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP) {
            /* 设置mib值 */
            mac_mib_set_beacon_period(pst_vap, (uint8_t)us_len, puc_param);
        }
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_BEACON_INTERVAL, us_len, puc_param);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_warning_log1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_bintval::send_event failed[%d].}", ret);
    }

    return ret;
}

/*
 * 函 数 名  : hmac_config_get_bintval
 * 功能描述  : 读取beacon interval
 * 1.日    期  : 2013年1月15日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
uint32_t hmac_config_get_bintval(mac_vap_stru *pst_mac_vap, uint16_t *pus_len, uint8_t *puc_param)
{
    /* 读取mib值 */
    return mac_mib_get_beacon_period(pst_mac_vap, (uint8_t *)pus_len, puc_param);
}


uint32_t hmac_config_set_dtimperiod(mac_vap_stru *pst_mac_vap, uint16_t us_len, uint8_t *puc_param)
{
    uint32_t ret;
    /* 设置mib值 */
    mac_mib_set_dtim_period(pst_mac_vap, (uint8_t)us_len, puc_param);

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_DTIM_PERIOD, us_len, puc_param);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_warning_log1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_bintval::send_event failed[%d].}", ret);
    }

    return ret;
}


uint32_t hmac_config_get_dtimperiod(mac_vap_stru *pst_mac_vap, uint16_t *pus_len, uint8_t *puc_param)
{
    /* 读取mib值 */
    return mac_mib_get_dtim_period(pst_mac_vap, (uint8_t *)pus_len, puc_param);
}

/*
 * 函 数 名  : hmac_config_set_freq
 * 功能描述  : 设置频率
 * 1.日    期  : 2013年1月15日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
uint32_t hmac_config_set_freq(mac_vap_stru *mac_vap, uint16_t us_len, uint8_t *puc_param)
{
    uint32_t ret;
    uint8_t channel = *puc_param;
    mac_device_stru *mac_device;
    mac_cfg_channel_param_stru channel_param = {0};

    /* 获取device */
    mac_device = mac_res_get_dev(mac_vap->uc_device_id);
    if (oal_unlikely(mac_device == NULL)) {
        oam_error_log0(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_freq::mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ret = mac_is_channel_num_valid(mac_vap->st_channel.en_band, channel, mac_vap->st_channel.ext6g_band);
    if (ret != OAL_SUCC) {
        oam_error_log1(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_freq::channel num[%d] invalid.}", channel);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

#ifdef _PRE_WLAN_FEATURE_11D
    /* 信道14特殊处理，只在11b协议模式下有效 */
    if ((channel == 14) && (mac_vap->en_protocol != WLAN_LEGACY_11B_MODE)) {
        oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_freq::channel-14 only available in 11b, curr protocol=%d.}", mac_vap->en_protocol);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
#endif

    mac_vap->st_channel.uc_chan_number = channel;
    ret = mac_get_channel_idx_from_num(mac_vap->st_channel.en_band, channel,
        mac_vap->st_channel.ext6g_band, &(mac_vap->st_channel.uc_chan_idx));
    if (ret != OAL_SUCC) {
        oam_warning_log2(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_freq::mac_get_channel_idx_from_num fail. \
            band[%u] channel[%u]!}", mac_vap->st_channel.en_band, mac_vap->st_channel.uc_chan_idx);
        return ret;
    }

    hmac_cali_send_work_channel_cali_data(mac_vap);

    /* 非DBAC时，首次配置信道时设置到硬件 */
    if (mac_device->uc_vap_num == 1 || mac_device->uc_max_channel == 0) {
        mac_device_get_channel(mac_device, &channel_param);
        channel_param.uc_channel = channel;
        mac_device_set_channel(mac_device, &channel_param);

        /***************************************************************************
            抛事件到DMAC层, 同步DMAC数据
        ***************************************************************************/
        ret = hmac_config_send_event(mac_vap, WLAN_CFGID_CURRENT_CHANEL, us_len, puc_param);
        if (oal_unlikely(ret != OAL_SUCC)) {
            oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_freq::send_event failed[%d].}", ret);
            return ret;
        }
    } else if (mac_is_dbac_enabled(mac_device) == OAL_TRUE) {
        /***************************************************************************
            抛事件到DMAC层, 同步DMAC数据
        ***************************************************************************/
        ret = hmac_config_send_event(mac_vap, WLAN_CFGID_CURRENT_CHANEL, us_len, puc_param);
        if (oal_unlikely(ret != OAL_SUCC)) {
            oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_freq::send_event failed[%d].}", ret);
            return ret;
        }
    } else {
        if (mac_device->uc_max_channel != channel) {
            oam_warning_log2(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_freq::previous vap channel number=[%d] \
                mismatch [%d].}", mac_device->uc_max_channel, channel);

            return OAL_FAIL;
        }
    }

    return OAL_SUCC;
}

/*
 * 函 数 名  : hmac_config_get_freq
 * 功能描述  : 读取频率
 * 1.日    期  : 2013年1月15日
 *   作    者  : zhangheng
 *   修改内容  : 新生成函数
 */
uint32_t hmac_config_get_freq(mac_vap_stru *pst_mac_vap, uint16_t *pus_len, uint8_t *puc_param)
{
    hmac_device_stru *pst_hmac_dev;

    *pus_len = sizeof(uint32_t);
    pst_hmac_dev = hmac_res_get_mac_dev(pst_mac_vap->uc_device_id);
    if (pst_hmac_dev != NULL && pst_hmac_dev->en_in_init_scan) {
        *((uint32_t *)puc_param) = 0;
        oam_warning_log0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_config_get_freq::get channel while init scan, retry please}");
        return OAL_SUCC;
    }

    *((uint32_t *)puc_param) = pst_mac_vap->st_channel.uc_chan_number;

    return OAL_SUCC;
}

static uint32_t hmac_wmm_params_set_EDCATableCWmin(mac_vap_stru *mac_vap, uint32_t ac, uint32_t value)
{
    uint32_t ret = OAL_SUCC;
    /*lint -e685*/ /*lint -e568*/
    if ((value > WLAN_QEDCA_TABLE_CWMIN_MAX) || (value < WLAN_QEDCA_TABLE_CWMIN_MIN)) {
        ret = OAL_FAIL;
    }
    mac_mib_set_EDCATableCWmin(mac_vap, (uint8_t)ac, value);
    return ret;
}

static uint32_t hmac_wmm_params_set_EDCATableCWmax(mac_vap_stru *mac_vap, uint32_t ac, uint32_t value)
{
    uint32_t ret = OAL_SUCC;
    if ((value > WLAN_QEDCA_TABLE_CWMAX_MAX) || (value < WLAN_QEDCA_TABLE_CWMAX_MIN)) {
        ret = OAL_FAIL;
    }

    mac_mib_set_EDCATableCWmax(mac_vap, (uint8_t)ac, value);
    return ret;
}

static uint32_t hmac_wmm_params_set_EDCATableAIFSN(mac_vap_stru *mac_vap, uint32_t ac, uint32_t value)
{
    uint32_t ret = OAL_SUCC;
    if ((value < WLAN_QEDCA_TABLE_AIFSN_MIN) || (value > WLAN_QEDCA_TABLE_AIFSN_MAX)) {
        ret = OAL_FAIL;
    }
    /*lint +e685*/ /*lint +e568*/
    mac_mib_set_EDCATableAIFSN(mac_vap, (uint8_t)ac, value);
    return ret;
}

static uint32_t hmac_wmm_params_set_EDCATableTXOPLimit(mac_vap_stru *mac_vap, uint32_t ac, uint32_t value)
{
    uint32_t ret = OAL_SUCC;
    if (value > WLAN_QEDCA_TABLE_TXOP_LIMIT_MAX) {
        ret = OAL_FAIL;
    }

    mac_mib_set_EDCATableTXOPLimit(mac_vap, (uint8_t)ac, value);
    return ret;
}

static uint32_t hmac_wmm_params_set_EDCATableMandatory(mac_vap_stru *mac_vap, uint32_t ac, uint32_t value)
{
    uint32_t ret = OAL_SUCC;
    if ((value != OAL_TRUE) && (value != OAL_FALSE)) {
        ret = OAL_FAIL;
    }
    mac_mib_set_EDCATableMandatory(mac_vap, (uint8_t)ac, (uint8_t)value);
    return ret;
}

static uint32_t hmac_wmm_params_set_QAPEDCATableCWmin(mac_vap_stru *mac_vap, uint32_t ac, uint32_t value)
{
    uint32_t ret = OAL_SUCC;
    /*lint -e685*/ /*lint -e568*/
    if ((value > WLAN_QEDCA_TABLE_CWMIN_MAX) || (value < WLAN_QEDCA_TABLE_CWMIN_MIN)) {
        ret = OAL_FAIL;
    }
    mac_mib_set_QAPEDCATableCWmin(mac_vap, (uint8_t)ac, value);
    return ret;
}

static uint32_t hmac_wmm_params_set_QAPEDCATableCWmax(mac_vap_stru *mac_vap, uint32_t ac, uint32_t value)
{
    uint32_t ret = OAL_SUCC;
    if ((value > WLAN_QEDCA_TABLE_CWMAX_MAX) || (value < WLAN_QEDCA_TABLE_CWMAX_MIN)) {
        ret = OAL_FAIL;
    }

    mac_mib_set_QAPEDCATableCWmax(mac_vap, (uint8_t)ac, value);
    return ret;
}

static uint32_t hmac_wmm_params_set_QAPEDCATableAIFSN(mac_vap_stru *mac_vap, uint32_t ac, uint32_t value)
{
    uint32_t ret = OAL_SUCC;
    if ((value < WLAN_QEDCA_TABLE_AIFSN_MIN) || (value > WLAN_QEDCA_TABLE_AIFSN_MAX)) {
        ret = OAL_FAIL;
    }
    /*lint +e685*/ /*lint +e568*/

    mac_mib_set_QAPEDCATableAIFSN(mac_vap, (uint8_t)ac, value);
    return ret;
}

static uint32_t hmac_wmm_params_set_QAPEDCATableTXOPLimit(mac_vap_stru *mac_vap, uint32_t ac, uint32_t value)
{
    uint32_t ret = OAL_SUCC;
    if (value > WLAN_QEDCA_TABLE_TXOP_LIMIT_MAX) {
        ret = OAL_FAIL;
    }

    mac_mib_set_QAPEDCATableTXOPLimit(mac_vap, (uint8_t)ac, value);
    return ret;
}

static uint32_t hmac_wmm_params_set_QAPEDCATableMSDULifetime(mac_vap_stru *mac_vap, uint32_t ac, uint32_t value)
{
    uint32_t ret = OAL_SUCC;
    if (value > WLAN_QEDCA_TABLE_MSDU_LIFETIME_MAX) {
        ret = OAL_FAIL;
    }
    mac_mib_set_QAPEDCATableMSDULifetime(mac_vap, (uint8_t)ac, value);
    return ret;
}

static uint32_t hmac_wmm_params_set_QAPEDCATableMandatory(mac_vap_stru *mac_vap, uint32_t ac, uint32_t value)
{
    uint32_t ret = OAL_SUCC;
    if ((value != OAL_TRUE) && (value != OAL_FALSE)) {
        ret = OAL_FAIL;
    }

    mac_mib_set_QAPEDCATableMandatory(mac_vap, (uint8_t)ac, (uint8_t)value);
    return ret;
}

typedef struct {
    uint8_t cfg_id;
    uint32_t (*hmac_set_wmm_params_case)(mac_vap_stru *mac_vap, uint32_t ac, uint32_t value);
} hmac_set_wmm_params_ops;

/*
 * 函 数 名  : hmac_config_set_wmm_params
 * 功能描述  : 设置频率
 * 1.日    期  : 2013年5月8日
 *   作    者  : 康国昌
 *   修改内容  : 新生成函数
 */
uint32_t hmac_config_set_wmm_params(mac_vap_stru *mac_vap, uint16_t us_len, uint8_t *puc_param)
{
    uint32_t ret;
    uint32_t ac;
    uint32_t value;
    wlan_cfgid_enum_uint16 en_cfg_id;
    hmac_config_wmm_para_stru *pst_cfg_stru = NULL;
    uint8_t idx;
    const hmac_set_wmm_params_ops hmac_set_wmm_params_ops_table[] = {
        { WLAN_CFGID_EDCA_TABLE_CWMIN,          hmac_wmm_params_set_EDCATableCWmin },
        { WLAN_CFGID_EDCA_TABLE_CWMAX,          hmac_wmm_params_set_EDCATableCWmax },
        { WLAN_CFGID_EDCA_TABLE_AIFSN,          hmac_wmm_params_set_EDCATableAIFSN },
        { WLAN_CFGID_EDCA_TABLE_TXOP_LIMIT,     hmac_wmm_params_set_EDCATableTXOPLimit },
        { WLAN_CFGID_EDCA_TABLE_MANDATORY,      hmac_wmm_params_set_EDCATableMandatory },
        { WLAN_CFGID_QEDCA_TABLE_CWMIN,         hmac_wmm_params_set_QAPEDCATableCWmin },
        { WLAN_CFGID_QEDCA_TABLE_CWMAX,         hmac_wmm_params_set_QAPEDCATableCWmax },
        { WLAN_CFGID_QEDCA_TABLE_AIFSN,         hmac_wmm_params_set_QAPEDCATableAIFSN },
        { WLAN_CFGID_QEDCA_TABLE_TXOP_LIMIT,    hmac_wmm_params_set_QAPEDCATableTXOPLimit },
        { WLAN_CFGID_QEDCA_TABLE_MSDU_LIFETIME, hmac_wmm_params_set_QAPEDCATableMSDULifetime },
        { WLAN_CFGID_QEDCA_TABLE_MANDATORY,     hmac_wmm_params_set_QAPEDCATableMandatory },
    };

    pst_cfg_stru = (hmac_config_wmm_para_stru *)puc_param;
    en_cfg_id = (uint16_t)pst_cfg_stru->cfg_id;
    ac = pst_cfg_stru->ac;
    value = pst_cfg_stru->value;

    ret = OAL_SUCC;
    if (ac >= WLAN_WME_AC_BUTT) {
        oam_warning_log3(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_wmm_params::invalid param, \
                         en_cfg_id=%d, ac=%d, value=%d.}", en_cfg_id, ac, value);
        return OAL_FAIL;
    }

    /* 根据sub-ioctl id填写WID */
    for (idx = 0; idx < (sizeof(hmac_set_wmm_params_ops_table) / sizeof(hmac_set_wmm_params_ops)); ++idx) {
        if (hmac_set_wmm_params_ops_table[idx].cfg_id == en_cfg_id) {
            ret = hmac_set_wmm_params_ops_table[idx].hmac_set_wmm_params_case(mac_vap, ac, value);
            break;
        }
    }
    if (idx == (sizeof(hmac_set_wmm_params_ops_table) / sizeof(hmac_set_wmm_params_ops))) {
        ret = OAL_FAIL;
    }
    if (ret == OAL_FAIL) {
        return ret;
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ret = hmac_config_send_event(mac_vap, en_cfg_id, us_len, puc_param);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_wmm_params::send event fail[%d].}", ret);
    }

    return ret;
}

/*
 * 函 数 名  : hmac_config_get_uapsden
 * 功能描述  : UAPSD使能
 * 1.日    期  : 2013年9月18日
 *   作    者  : zourong
 *   修改内容  : 新生成函数
 */
uint32_t hmac_config_get_uapsden(mac_vap_stru *pst_mac_vap, uint16_t *pus_len, uint8_t *puc_param)
{
    *puc_param = mac_vap_get_uapsd_en(pst_mac_vap);
    *pus_len = sizeof(uint8_t);

    return OAL_SUCC;
}
