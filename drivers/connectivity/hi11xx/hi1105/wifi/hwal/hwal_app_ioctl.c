


/* 平台适配层 */
#include "oal_types.h"
#include "oal_util.h"

/* 平台差异层 */
#include "arch/oal_util.h"
#include "arch/oal_net.h"

/* wifi */
#include "hd_event.h"
#include "wal_config.h"
#include "mac_regdomain.h"

#include "securec.h"

#include "hwal_app_ioctl.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

// 2 全局变量定义
#ifdef _PRE_WLAN_FEATURE_HILINK

int hisi_wlan_set_monitor(unsigned char monitor_switch, char rssi_level)
{
    oal_net_device_stru *netdev = NULL;
    uint8_t monitor_mode = HISI_MONITOR_SWITCH_OFF;
    wal_msg_write_stru              st_write_msg;
    uint8_t auc_info[2];
    int32_t ret;

    netdev = oal_dev_get_by_name("wlan0");
    if (netdev == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "hisi_wlan_set_monitor::netdev is null");
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (monitor_switch < HISI_MONITOR_SWITCH_BUTT) {
        monitor_mode = monitor_switch;
    } else {
        oam_error_log1(0, OAM_SF_ANY, "hisi_wlan_set_monitor:monitor_switch=%d", monitor_switch);
    }

    auc_info[0] = monitor_mode;
    auc_info[1] = (uint8_t)rssi_level;
    memset_s(&st_write_msg, sizeof(wal_msg_write_stru), 0, sizeof(wal_msg_write_stru));
    // 抛事件到wal层处理
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_MONITOR_EN, sizeof(uint8_t) * 2);
    if (memcpy_s(st_write_msg.auc_value, WAL_MSG_WRITE_MAX_LEN, auc_info, sizeof(uint8_t) * 2) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "hisi_wlan_set_monitor::memcpy_s failed");
        return OAL_FAIL;
    }
    ret = wal_send_cfg_event(netdev,
                             WAL_MSG_TYPE_WRITE,
                             WAL_MSG_WRITE_MSG_HDR_LENGTH + st_write_msg.us_len,
                             (uint8_t *)&st_write_msg,
                             FALSE,
                             NULL);
    if (OAL_UNLIKELY(ret != OAL_SUCC)) {
        oam_warning_log1(0, OAM_SF_ANY, "{hisi_wlan_set_monitor::return err code %d!}", ret);
        return ret;
    }

    return OAL_SUCC;
}
#endif


int hisi_wlan_set_channel(hisi_channel_stru *channel_info)
{
    oal_net_device_stru *netdev = NULL;
    mac_cfg_channel_param_stru channel_param;
    uint8_t channel;
    uint8_t channel_width;
    int32_t ret;
    wlan_channel_bandwidth_enum_uint8 bandwidth;
    mac_vap_stru *pst_mac_vap = NULL;
    uint32_t retValue;

    if (channel_info == NULL) {
        oam_error_log1(0, OAM_SF_ANY, "hwal_ioctl_set_freq::p_buf = %x", channel_info);
        return OAL_ERR_CODE_PTR_NULL;
    }

    netdev = oal_dev_get_by_name("wlan0");
    if (netdev == NULL) {
        oam_error_log1(0, OAM_SF_ANY, "hwal_ioctl_set_freq::netdev = %x", netdev);
        return OAL_ERR_CODE_PTR_NULL;
    }
    channel   = channel_info->uc_channel_num;
    channel_width = channel_info->uc_channel_bandwidth;
    pst_mac_vap = OAL_NET_DEV_PRIV(netdev);
    if (pst_mac_vap == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_set_freq_40m::pst_mac_vap is null");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ret = mac_is_channel_num_valid(WLAN_BAND_2G, channel, FALSE);
    if (ret != OAL_SUCC) {
        oam_warning_log1(0, 0, "channel %d is invable", channel);
        return -OAL_EFAIL;
    }
    if (((channel_width == HISI_BAND_WIDTH_40PLUS) && (channel > 9)) ||
        (channel_width == HISI_BAND_WIDTH_40MINUS) && (channel < 5)) {
        oam_error_log2(0, OAM_SF_ANY, "hwal_ioctl_set_freq::channel = %d, channel_width = %d", channel, channel_width);
        return -OAL_EFAIL;
    }

    bandwidth = (channel_width == HISI_BAND_WIDTH_40PLUS) ? WLAN_BAND_WIDTH_40PLUS : WLAN_BAND_WIDTH_40MINUS;
    channel_param.uc_channel = channel;
    channel_param.en_band = 0;
    channel_param.en_bandwidth = bandwidth;

    retValue = wal_config_set_channel(pst_mac_vap, sizeof(mac_cfg_channel_param_stru), (uint8_t*)(&channel_param));
    if (retValue != OAL_SUCC) {
        oam_error_log1(0, 0, "hwal_ioctl_set_freq_40m::ret = %d", retValue);
        return -OAL_EFAIL;
    }
    return OAL_SUCC;
}


int hisi_wlan_get_channel(hisi_channel_stru *channel_info)
{
    oal_net_device_stru *netdev = NULL;
    mac_vap_stru *pst_mac_vap = NULL;

    if (channel_info == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "wal_get_channel::ptr is NULL");
        return OAL_ERR_CODE_PTR_NULL;
    }

    netdev = oal_dev_get_by_name("wlan0");
    if (netdev == NULL) {
        oam_error_log1(0, OAM_SF_ANY, "wal_get_channel::netdev = %x", netdev);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap = OAL_NET_DEV_PRIV(netdev);
    if (pst_mac_vap == NULL) {
        oam_error_log0(0, 0, "wal_get_channel::pst_mac_vap is null");
        return OAL_ERR_CODE_PTR_NULL;
    }

    channel_info->uc_channel_num = pst_mac_vap->st_channel.uc_chan_number;

    /* 应用层与底层带宽枚举不相同，这里需要做转换 */
    if (pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_20M) {
        channel_info->uc_channel_bandwidth = HISI_BAND_WIDTH_20M;
    } else if (pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40PLUS) {
        channel_info->uc_channel_bandwidth = HISI_BAND_WIDTH_40PLUS;
    } else if (pst_mac_vap->st_channel.en_bandwidth == WLAN_BAND_WIDTH_40MINUS) {
        channel_info->uc_channel_bandwidth = HISI_BAND_WIDTH_40MINUS;
    } else {
        oam_error_log1(0, 0, "hisi_wlan_get_channel:: error bandwith [%d]", pst_mac_vap->st_channel.uc_chan_number);
    }

    return OAL_SUCC;
}


int hisi_wlan_set_country(uint8_t *country)
{
    uint32_t ret;
    if (country == NULL) {
        oam_error_log0(0, 0, "hisi_wlan_set_country:: ptr is NULL");
        return OAL_FAIL;
    }

    ret = wal_set_country(country);
    if (ret != OAL_SUCC) {
        oam_error_log1(0, 0, "hisi_wlan_set_country:: set country fail, error code[%d]", ret);
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


char *hisi_wlan_get_country(void)
{
    return mac_regdomain_get_country();
}


int hisi_wlan_set_always_tx(char *param)
{
    int32_t ret;
    if (param == NULL) {
        oam_error_log0(0, 0, "hisi_wlan_set_always_tx:: ptr is NULL");
        return OAL_FAIL;
    }

    ret = wal_set_always_tx(param);
    if (ret != OAL_SUCC) {
        oam_error_log1(0, 0, "hisi_wlan_set_always_tx:: set always tx fail, error code[%d]", ret);
        return ret;
    }

    return OAL_SUCC;
}


int hisi_wlan_set_always_rx(char *param)
{
    int32_t ret;
    if (param == NULL) {
        oam_error_log0(0, 0, "hisi_wlan_set_always_tx:: ptr is NULL");
        return OAL_FAIL;
    }

    ret = wal_set_always_rx(param);
    if (ret != OAL_SUCC) {
        oam_error_log1(0, 0, "hisi_wlan_set_always_tx:: set always tx fail, error code[%d]", ret);
        return OAL_FAIL;
    }

    return OAL_SUCC;
}



int hisi_wlan_rx_fcs_info(int32_t *rx_succ_num)
{
    if (rx_succ_num == NULL) {
        oam_error_log0(0, 0, "hisi_wlan_rx_fcs_info:: ptr is NULL");
        return OAL_FAIL;
    }

    *rx_succ_num = wal_rx_fcs_info();

    return OAL_SUCC;
}


int hisi_wlan_set_pm_switch(uint32_t pm_switch_value)
{
    uint32_t pm_switch = pm_switch_value;
    /* 获取低功耗开关状态 */
    if ((pm_switch != 0) && (pm_switch != 1)) {
        oam_warning_log1(0, 0, "hisi_wlan_set_pm_switch::parama is error [%d]", pm_switch);
        return OAL_FAIL;
    }
    if (pm_switch == 0) {
        /* 下发低功耗状态 */
        if (hwal_ioctl_set_pm_on("wlan0", &pm_switch) != HISI_SUCC) {
            oam_error_log0(0, 0, "pm state set faile");
            return OAL_FAIL;
        }

        /* 设置通道状态 */
        if (wal_set_wlan_pm_state(pm_switch) != HISI_SUCC) {
            oam_error_log0(0, 0, "pm state set faile");
            return OAL_FAIL;
        }
    } else {
        /* 设置通道状态 */
        if (wal_set_wlan_pm_state(pm_switch) != HISI_SUCC) {
            oam_error_log0(0, 0, "pm state set faile");
            return OAL_FAIL;
        }

        /* 下发低功耗状态 */
        if (hwal_ioctl_set_pm_on("wlan0", &pm_switch) != HISI_SUCC) {
            oam_error_log0(0, 0, "pm state set faile");
            return OAL_FAIL;
        }
    }

    return OAL_SUCC;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

