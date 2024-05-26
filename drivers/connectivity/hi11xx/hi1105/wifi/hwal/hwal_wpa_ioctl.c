

// 1 头文件包含
#include "hwal_wpa_ioctl.h"
#include "hwal_net.h"
#include "wal_linux_cfg80211.h"
#include "wal_linux_ioctl.h"
#include "securec.h"


#ifdef _PRE_WLAN_FEATURE_DFR
#include "wal_dfx.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HWAL_IOCTL_C

// 2 全局变量定义
OAL_CONST hwal_ioctl_handler g_ast_hwal_ioctl_handlers[] = {
    (hwal_ioctl_handler) hwal_ioctl_set_ap,             /* HISI_IOCTL_SET_AP */
    (hwal_ioctl_handler) hwal_ioctl_new_key,            /* HISI_IOCTL_NEW_KEY */
    (hwal_ioctl_handler) hwal_ioctl_del_key,            /* HISI_IOCTL_DEL_KEY */
    (hwal_ioctl_handler) hwal_ioctl_set_key,            /* HISI_IOCTL_SET_KEY */
    (hwal_ioctl_handler) hwal_ioctl_send_mlme,          /* HISI_IOCTL_SEND_MLME */
    (hwal_ioctl_handler) hwal_ioctl_send_eapol,         /* HISI_IOCTL_SEND_EAPOL */
    (hwal_ioctl_handler) hwal_ioctl_receive_eapol,      /* HISI_IOCTL_RECEIVE_EAPOL */
    (hwal_ioctl_handler) hwal_ioctl_enable_eapol,       /* HISI_IOCTL_ENALBE_EAPOL */
    (hwal_ioctl_handler) hwal_ioctl_disable_eapol,      /* HISI_IOCTL_DISABLE_EAPOL */
    (hwal_ioctl_handler) hwal_ioctl_get_addr,           /* HIIS_IOCTL_GET_ADDR */
    (hwal_ioctl_handler) hwal_ioctl_set_power,          /* HISI_IOCTL_SET_POWER */
    (hwal_ioctl_handler) hwal_ioctl_set_mode,           /* HISI_IOCTL_SET_MODE */
    (hwal_ioctl_handler) hwal_ioctl_get_hw_feature,     /* HIIS_IOCTL_GET_HW_FEATURE */
    (hwal_ioctl_handler) hwal_ioctl_stop_ap,            /* HIIS_IOCTL_STOP_AP */
    (hwal_ioctl_handler) hwal_ioctl_del_virtual_intf,   /* HISI_IOCTL_DEL_VIRTUAL_INTF */
    (hwal_ioctl_handler) hwal_ioctl_scan,               /* HISI_IOCTL_SCAN */
    (hwal_ioctl_handler) hwal_ioctl_disconnect,         /* HISI_IOCTL_DISCONNET */
    (hwal_ioctl_handler) hwal_ioctl_assoc,              /* HISI_IOCTL_ASSOC */
    (hwal_ioctl_handler) hwal_ioctl_set_netdev,         /* HISI_IOCTL_SET_NETDEV */
#ifdef _PRE_WLAN_CONFIG_WPS
    (hwal_ioctl_handler) hwal_ioctl_set_ap_wps_p2p_ie,  /* HISI_IOCTL_SET_AP_WPS_P2P_IE */
#endif
    (hwal_ioctl_handler) hwal_ioctl_change_beacon,      /* HISI_IOCTL_CHANGE_BEACON */
    (hwal_ioctl_handler) hwal_ioctl_dhcp_start,         /* HISI_IOCTL_DHCP_START */
    (hwal_ioctl_handler) hwal_ioctl_dhcp_stop,          /* HISI_IOCTL_DHCP_STOP */
    (hwal_ioctl_handler) hwal_ioctl_dhcp_succ_check,    /* HISI_IOCTL_DHCP_SUCC_CHECK */
    (hwal_ioctl_handler) hwal_ioctl_set_pm_on,           /* HISI_IOCTL_SET_PM_ON */
    (hwal_ioctl_handler) hwal_ioctl_ip_notify,           /* HISI_IOCTL_IP_NOTIFY_DRIVER */
    (hwal_ioctl_handler) hwal_ioctl_set_max_sta,         /* HISI_IOCTL_SET_MAX_STA */
    (hwal_ioctl_handler) hwal_ioctl_sta_remove           /* HISI_IOCTL_STA_REMOVE */
};

// 3 函数实现

int32_t hwal_ioctl_set_key(int8_t *ifname, void *buf)
{
    uint8_t key_index;
    oal_bool_enum_uint8 unicast;
    oal_bool_enum_uint8 multicast;
    hisi_key_ext_stru *key_ext = NULL;
    oal_net_device_stru *netdev = NULL;
    oal_wiphy_stru *wiphy = NULL;

    if ((ifname == NULL) || (buf == NULL)) {
        oam_error_log0(0, 0, "hwal_ioctl_set_key parameter NULL.");
        return -OAL_EFAIL;
    }

    unicast = TRUE;
    multicast = FALSE;
    netdev = oal_dev_get_by_name(ifname);
    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_set_key:netdev is NULL.");
        return -HISI_EFAIL;
    }
    wiphy = oal_wiphy_get();

    key_ext = (hisi_key_ext_stru *)buf;
    key_index = (uint8_t)key_ext->key_idx;

    if (key_ext->def == HISI_TRUE) {
        unicast = TRUE;
        multicast = TRUE;
    }

    if (key_ext->defmgmt == HISI_TRUE) {
        multicast = TRUE;
    }

    if (key_ext->default_types == HISI_KEY_DEFAULT_TYPE_UNICAST) {
        unicast = TRUE;
    } else if (key_ext->default_types == HISI_KEY_DEFAULT_TYPE_MULTICAST) {
        multicast = TRUE;
    }

    return wal_cfg80211_set_default_key(wiphy, netdev, key_index, unicast, multicast);
}


int32_t hwal_ioctl_new_key(int8_t *ifname, void *buf)
{
    uint8_t key_index;
    oal_bool_enum_uint8 pairwise;
    oal_net_device_stru *netdev = NULL;
    oal_wiphy_stru *wiphy = NULL;
    uint8_t *mac_addr = NULL;
    hisi_key_ext_stru *key_ext = NULL;
    oal_key_params_stru params;

    if ((ifname == NULL) || (buf == NULL)) {
        oam_error_log0(0, 0, "hwal_ioctl_new_key parameter NULL.");
        return -OAL_EFAIL;
    }

    memset_s(&params, sizeof(oal_key_params_stru), 0, sizeof(oal_key_params_stru));

    netdev = oal_dev_get_by_name(ifname);
    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_new_key:netdev is NULL.");
        return -HISI_EFAIL;
    }
    wiphy = oal_wiphy_get();

    key_ext = (hisi_key_ext_stru *)buf;
    key_index = (uint8_t)key_ext->key_idx;
    pairwise = (key_ext->type == HISI_KEYTYPE_PAIRWISE);
    mac_addr = key_ext->addr;

    params.key = (uint8_t *)(key_ext->key);
    params.key_len = (int32_t)key_ext->key_len; // unsigend int -->int
    params.seq_len = (int32_t)key_ext->seq_len; // unsigend int -->int
    params.seq = key_ext->seq;
    params.cipher = key_ext->cipher;

    return wal_cfg80211_add_key(wiphy, netdev, key_index, pairwise, mac_addr, &params);
}


int32_t hwal_ioctl_del_key(int8_t *ifname, void *buf)
{
    uint8_t key_index;
    oal_bool_enum_uint8 pairwise;
    oal_net_device_stru *netdev = NULL;
    oal_wiphy_stru *wiphy = NULL;
    hisi_key_ext_stru *key_ext = NULL;
    uint8_t *mac_addr = NULL;

    if ((ifname == NULL) || (buf == NULL)) {
        oam_error_log0(0, 0, "hwal_ioctl_del_key parameter NULL.");
        return -OAL_EFAIL;
    }

    netdev = oal_dev_get_by_name(ifname);
    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_del_key:netdev is NULL.");
        return -HISI_EFAIL;
    }

    wiphy = oal_wiphy_get();

    key_ext = (hisi_key_ext_stru *)buf;
    pairwise = (key_ext->type == HISI_KEYTYPE_PAIRWISE);
    mac_addr = key_ext->addr;
    key_index = key_ext->key_idx;

    return wal_cfg80211_remove_key(wiphy, netdev, key_index, pairwise, mac_addr);
}
/*
 * 功能描述  : set ap时设置ap参数
 * 修改历史  : 新增函数
 */
static void hwal_set_ap_params(oal_ap_settings_stru *oal_apsettings, hisi_ap_settings_stru *apsettings)
{
    oal_apsettings->ssid_len = apsettings->ssid_len;
    oal_apsettings->beacon_interval = apsettings->beacon_interval;
    oal_apsettings->dtim_period = apsettings->dtim_period;
    oal_apsettings->hidden_ssid = (enum nl80211_hidden_ssid)(apsettings->hidden_ssid);
    oal_apsettings->beacon.head_len = apsettings->beacon_data.head_len;
    oal_apsettings->beacon.tail_len = apsettings->beacon_data.tail_len;

    oal_apsettings->ssid = apsettings->ssid;
    oal_apsettings->beacon.head = apsettings->beacon_data.head;
    oal_apsettings->beacon.tail = apsettings->beacon_data.tail;
    oal_apsettings->auth_type = (enum nl80211_auth_type)(apsettings->auth_type);
}

int32_t hwal_ioctl_set_ap(int8_t *ifname, void *buf)
{
    oal_net_device_stru *netdev = NULL;
    oal_wiphy_stru *wiphy = NULL;
    hisi_ap_settings_stru *apsettings = NULL;
    oal_ap_settings_stru oal_apsettings;

    if ((ifname == NULL) || (buf == NULL)) {
        oam_error_log0(0, 0, "hwal_ioctl_set_ap parameter NULL.");
        return -OAL_EFAIL;
    }

    memset_s(&oal_apsettings, sizeof(oal_ap_settings_stru), 0, sizeof(oal_ap_settings_stru));

    netdev = oal_dev_get_by_name(ifname);
    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_set_ap :netdev is NULL.");
        return -OAL_EFAIL;
    }

    wiphy = oal_wiphy_get();

    apsettings = (hisi_ap_settings_stru *)buf;

    hwal_set_ap_params(&oal_apsettings, apsettings);

    if (netdev->ieee80211_ptr == NULL) {
        netdev->ieee80211_ptr = (oal_wireless_dev_stru*)oal_memalloc(sizeof(struct wireless_dev));
        if (netdev->ieee80211_ptr == NULL) {
            oam_error_log0(0, 0, "ieee80211_ptr parameter NULL.");
            return -OAL_EFAIL;
        }
        memset_s(netdev->ieee80211_ptr, sizeof(struct wireless_dev), 0, sizeof(struct wireless_dev));
    }

    if (netdev->ieee80211_ptr->preset_chandef.chan == NULL) {
        netdev->ieee80211_ptr->preset_chandef.chan =
            (oal_ieee80211_channel*)oal_memalloc(sizeof(oal_ieee80211_channel));
        if (netdev->ieee80211_ptr->preset_chandef.chan == NULL) {
            oal_free(netdev->ieee80211_ptr);
            netdev->ieee80211_ptr = NULL;

            oam_error_log0(0, 0, "chan parameter NULL.");
            return -OAL_EFAIL;
        }
        memset_s(netdev->ieee80211_ptr->preset_chandef.chan, sizeof(oal_ieee80211_channel),
            0, sizeof(oal_ieee80211_channel));
    }
    netdev->ieee80211_ptr->preset_chandef.width = (enum nl80211_channel_type)apsettings->freq_params.bandwidth;
    netdev->ieee80211_ptr->preset_chandef.center_freq1 = apsettings->freq_params.center_freq1;
    netdev->ieee80211_ptr->preset_chandef.chan->hw_value = apsettings->freq_params.channel;
    netdev->ieee80211_ptr->preset_chandef.chan->band = IEEE80211_BAND_2GHZ;

    return wal_cfg80211_start_ap(wiphy, netdev, &oal_apsettings);
}


int32_t hwal_ioctl_change_beacon(int8_t *ifname, void *buf)
{
    oal_net_device_stru *netdev = NULL;
    oal_beacon_data_stru    beacon;
    oal_wiphy_stru *wiphy = NULL;
    hisi_ap_settings_stru *apsettings = NULL;

    if ((ifname == NULL) || (buf == NULL)) {
        oam_error_log0(0, 0, "hwal_ioctl_change_beacon parameter NULL.");
        return -OAL_EFAIL;
    }

    memset_s(&beacon, sizeof(oal_beacon_data_stru), 0, sizeof(oal_beacon_data_stru));

    netdev = oal_dev_get_by_name(ifname);
    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_change_beacon:netdev is NULL.");
        return -HISI_EFAIL;
    }
    wiphy = oal_wiphy_get();
    apsettings = (hisi_ap_settings_stru *)buf;

    /* 获取修改beacon帧参数的结构体 */
    beacon.head = apsettings->beacon_data.head;
    beacon.tail = apsettings->beacon_data.tail;
    beacon.head_len = apsettings->beacon_data.head_len;
    beacon.tail_len = apsettings->beacon_data.tail_len;

    return wal_cfg80211_change_beacon(wiphy, netdev, &beacon);
}
#ifdef _PRE_WLAN_CONFIG_WPS

int32_t hwal_ioctl_set_ap_wps_p2p_ie(int8_t *ifname, void *buf)
{
    oal_net_device_stru *netdev = NULL;
    hisi_app_ie_stru *app_ie = NULL;
    oal_app_ie_stru wps_p2p_ie;

    if ((ifname == NULL) || (buf == NULL)) {
        oam_error_log0(0, 0, "hwal_ioctl_set_ap_wps_p2p_ie parameter NULL.");
        return -OAL_EFAIL;
    }

    memset_s(&wps_p2p_ie, sizeof(oal_app_ie_stru), 0, sizeof(oal_app_ie_stru));

    netdev = oal_dev_get_by_name(ifname);
    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_set_ap_wps_p2p_ie parameter NULL.");
        return -OAL_EFAIL;
    }
    app_ie = (hisi_app_ie_stru *)buf;
    wps_p2p_ie.ie_len = app_ie->ie_len;
    wps_p2p_ie.en_app_ie_type = app_ie->uc_app_ie_type;

    if (wps_p2p_ie.ie_len > WLAN_WPS_IE_MAX_SIZE) {
        oam_error_log0(0, 0, "app ie length is too large!");
        return -OAL_EFAIL;
    }

    if (memcpy_s(wps_p2p_ie.auc_ie, WLAN_WPS_IE_MAX_SIZE, app_ie->ie, app_ie->ie_len) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "hwal_ioctl_set_ap_wps_p2p_ie::memcpy_s failed!");
        return -OAL_EFAIL;
    }

    return wal_ioctl_set_wps_p2p_ie(netdev, wps_p2p_ie.auc_ie,
        wps_p2p_ie.ie_len, wps_p2p_ie.en_app_ie_type);
}
#endif

int32_t hwal_ioctl_send_mlme(int8_t *ifname, void *buf)
{
    oal_net_device_stru *netdev = NULL;
    oal_wiphy_stru *wiphy = NULL;
    hisi_mlme_data_stru *mlme_data = NULL;
    oal_ieee80211_channel       chan;

    if ((ifname == NULL) || (buf == NULL)) {
        oam_error_log0(0, 0, "hwal_ioctl_send_mlme parameter NULL.");
        return -OAL_EFAIL;
    }

    memset_s(&chan, sizeof(oal_ieee80211_channel), 0, sizeof(oal_ieee80211_channel));

    netdev = oal_dev_get_by_name(ifname);
    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_send_mlme:netdev is NULL.");
        return -OAL_EFAIL;
    }
    wiphy = oal_wiphy_get();

    mlme_data = (hisi_mlme_data_stru *)buf;

    chan.center_freq = mlme_data->freq;

    return wal_cfg80211_mgmt_tx(wiphy, netdev->ieee80211_ptr, &chan, 0, 0, mlme_data->data,
        mlme_data->data_len, 0, mlme_data->noack, mlme_data->send_action_cookie);
}


int32_t hwal_ioctl_send_eapol(int8_t *ifname, void *buf)
{
    hisi_tx_eapol_stru *tx_eapol = NULL;
    oal_net_device_stru *netdev = NULL;
    oal_netbuf_stru *netbuf = NULL;

    if ((ifname == NULL) || (buf == NULL)) {
        oam_error_log0(0, 0, "hwal_ioctl_send_eapol parameter NULL.");
        return -OAL_EFAIL;
    }

    netdev = oal_dev_get_by_name(ifname);
    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_send_eapol:netdev is NULL.");
        return -HISI_EFAIL;
    }
    tx_eapol = (hisi_tx_eapol_stru *)buf;

    /* 申请SKB内存内存发送 */
    netbuf = hwal_lwip_skb_alloc(netdev, tx_eapol->len);
    if (netbuf == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_send_eapol skb_alloc NULL.");
        return -OAL_EFAIL;
    }

    oal_netbuf_put(netbuf, tx_eapol->len);
    if (memcpy_s(OAL_NETBUF_DATA(netbuf), OAL_NETBUF_LEN(netbuf), tx_eapol->buf, tx_eapol->len) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "hwal_ioctl_send_eapol::memcpy_s failed!");
        oal_netbuf_free(netbuf);
        return -OAL_EFAIL;
    }

    if ((netdev->netdev_ops == NULL) || (netdev->netdev_ops->ndo_start_xmit == NULL)) {
        oam_error_log0(0, 0, "hwal_ioctl_send_eapol netdev_ops NULL.");
        oal_netbuf_free(netbuf);
        return -OAL_EFAIL;
    }

    return netdev->netdev_ops->ndo_start_xmit(netbuf, netdev);
}


int32_t hwal_ioctl_receive_eapol(int8_t *ifname, void *buf)
{
    oal_net_device_stru *netdev = NULL;
    oal_netbuf_stru *skb_buf = NULL;
    hisi_rx_eapol_stru *rx_eapol = NULL;

    if ((ifname == NULL) || (buf == NULL)) {
        oam_error_log0(0, 0, "hwal_ioctl_receive_eapol parameter NULL.");
        return -OAL_EFAIL;
    }

    rx_eapol = (hisi_rx_eapol_stru *)buf;
    netdev = oal_dev_get_by_name(ifname);
    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_receive_eapol:netdev is NULL.");
        return -OAL_EFAIL;
    }

    if (oal_netbuf_list_empty(&netdev->hisi_eapol.st_eapol_skb_head) == TRUE) {
        /* 此处hostapd在取链表数据时，会一直取到链表为空，所以每次都会打印，此时为正常打印，所以设置为info */
        oam_info_log0(0, 0, "hwal_ioctl_receive_eapol eapol pkt Q empty.");
        return -OAL_EINVAL;
    }

    skb_buf = oal_netbuf_delist(&netdev->hisi_eapol.st_eapol_skb_head);
    if (skb_buf == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_receive_eapol:: oal_netbuf_delist is NULL.");
        return -OAL_EINVAL;
    }

    if (skb_buf->len > rx_eapol->len) {
        /* 如果收到EAPOL报文大小超过接收报文内存，返回失败 */
        oam_error_log2(0, 0, "hwal_ioctl_receive_eapol eapol pkt len(%d) > buf size(%d).",
            skb_buf->len, rx_eapol->ul_len);
        oal_netbuf_free(skb_buf);
        return -OAL_EFAIL;
    }

    if (memcpy_s(rx_eapol->buf, rx_eapol->len, skb_buf->data, skb_buf->len) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "hwal_ioctl_receive_eapol::memcpy_s failed!");
        oal_netbuf_free(skb_buf);
        return -OAL_EFAIL;
    }
    rx_eapol->len = skb_buf->len;

    oal_netbuf_free(skb_buf);

    return OAL_SUCC;
}


int32_t hwal_ioctl_enable_eapol(int8_t *ifname, void *buf)
{
    oal_net_device_stru *netdev = NULL;
    hisi_enable_eapol_stru *enable_param = NULL;

    if ((ifname == NULL) || (buf == NULL)) {
        oam_error_log0(0, 0, "hwal_ioctl_enable_eapol parameter NULL.");
        return -OAL_EFAIL;
    }

    enable_param = (hisi_enable_eapol_stru *)buf;
    netdev    = oal_dev_get_by_name(ifname);
    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_enable_eapol:netdev is NULL.");
        return -HISI_EFAIL;
    }

    netdev->hisi_eapol.en_register = TRUE;
    netdev->hisi_eapol.notify_callback = enable_param->callback;
    netdev->hisi_eapol.context = enable_param->contex;

    return OAL_SUCC;
}


int32_t hwal_ioctl_disable_eapol(int8_t *ifname, void *buf)
{
    oal_net_device_stru *netdev = NULL;

    if ((ifname == NULL) || (buf == NULL)) {
        oam_error_log0(0, 0, "hwal_ioctl_disable_eapol parameter NULL.");
        return -OAL_EFAIL;
    }

    netdev = oal_dev_get_by_name(ifname);
    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_disable_eapol:netdev is NULL.");
        return -HISI_EFAIL;
    }

    netdev->hisi_eapol.en_register = FALSE;
    netdev->hisi_eapol.notify_callback = NULL;
    netdev->hisi_eapol.context = NULL;

    return OAL_SUCC;
}


int32_t hwal_ioctl_get_addr(int8_t *ifname, void *buf)
{
    oal_net_device_stru *netdev = NULL;

    if ((ifname == NULL) || (buf == NULL)) {
        oam_error_log0(0, 0, "hwal_ioctl_enable_eapol parameter NULL.");
        return -OAL_EFAIL;
    }

    /* 调用获取MAC地址操作 */
    netdev = oal_dev_get_by_name(ifname);
    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_get_addr:netdev is NULL.");
        return -HISI_EFAIL;
    }
    if (memcpy_s(buf, ETH_ADDR_LEN, netdev->dev_addr, ETH_ADDR_LEN) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "hwal_ioctl_get_addr::memcpy_s failed!");
        return -HISI_EFAIL;
    }

    return OAL_SUCC;
}

static int32_t hwal_get_band(int8_t *ifname, oal_ieee80211_supported_band **band)
{
    oal_net_device_stru *netdev = NULL;
    oal_wiphy_stru *wiphy = NULL;
    oal_wireless_dev_stru *ieee80211 = NULL;

    /* 调用获取HW feature */
    netdev = oal_dev_get_by_name(ifname);
    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_get_hw_band: netdev is NULL.");
        return -OAL_EFAIL;
    }

    ieee80211 = netdev->ieee80211_ptr;
    if (ieee80211 == NULL) {
        oam_error_log0(0, 0, "hwal_get_hw_band ieee80211_ptr NULL.");
        return -OAL_EFAIL;
    }

    wiphy = oal_wiphy_get();
    if (wiphy == NULL) {
        oam_error_log0(0, 0, "hwal_get_hw_band wiphy NULL.");
        return -OAL_EFAIL;
    }

    *band = wiphy->bands[IEEE80211_BAND_2GHZ];
    if (*band == NULL) {
        oam_error_log0(0, 0, "hwal_get_hw_band band NULL.");
        return -OAL_EFAIL;
    }
    return OAL_SUCC;
}

int32_t hwal_ioctl_get_hw_feature(int8_t *ifname, void *buf)
{
    hisi_hw_feature_data_stru *hw_feature_data = NULL;
    oal_ieee80211_supported_band *band = NULL;
    uint32_t loop;
    int32_t ret;

    if ((ifname == NULL) || (buf == NULL)) {
        oam_error_log0(0, 0, "hwal_ioctl_get_hw_feature parameter NULL.");
        return -OAL_EFAIL;
    }

    ret = hwal_get_band(ifname, &band);
    if (ret != OAL_SUCC) {
        return ret;
    }

    hw_feature_data = (hisi_hw_feature_data_stru *)buf;
    hw_feature_data->channel_num = band->n_channels;
    hw_feature_data->ht_capab = band->ht_cap.cap;

    /* 数组越界判断 */
    if (band->n_channels > HISI_CHANNEL_MAX_NUM) {
        oam_error_log1(0, 0, "error: n_channels = %d > HISI_CHANNEL_MAX_NUM.", band->n_channels);
        return -OAL_EFAIL;
    }
    for (loop = 0; loop < (uint32_t)band->n_channels; ++loop) {
        hw_feature_data->ast_iee80211_channel[loop].flags = band->channels[loop].flags;
        hw_feature_data->ast_iee80211_channel[loop].freq = band->channels[loop].center_freq;
        hw_feature_data->ast_iee80211_channel[loop].channel = band->channels[loop].hw_value;
    }

    /* 数组越界判断 */
    if (band->n_bitrates > HISI_BITRATE_MAX_NUM) {
        oam_error_log1(0, 0, "error: n_bitrates = %d > HISI_BITRATE_MAX_NUM.", band->n_bitrates);
        return -OAL_EFAIL;
    }
    for (loop = 0; loop < (uint32_t)band->n_bitrates; ++loop) {
        hw_feature_data->aus_bitrate[loop] = band->bitrates[loop].bitrate;
    }

    return OAL_SUCC;
}
static void hwal_device_power_down(oal_net_device_stru *net_dev)
{
    /* 下电host device_stru去初始化 */
    wal_host_dev_exit(net_dev);

    wal_wake_lock();
    wlan_pm_close();
    wal_wake_unlock();

    return;
}
static int32_t hwal_device_power_on(oal_net_device_stru *net_dev)
{
    int32_t ret;

    wal_wake_lock();
    ret = wlan_pm_open();
    wal_wake_unlock();

    if (ret == OAL_FAIL) {
        return -OAL_EFAIL;
    } else if (ret != OAL_ERR_CODE_ALREADY_OPEN) {
#ifdef _PRE_WLAN_FEATURE_DFR
        wal_dfr_init_param();
#endif
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
        /* 重新上电时调用定制化接口 */
        hwifi_config_init_force();
#endif
        // 重新上电场景，下发配置VAP
        ret = wal_cfg_vap_h2d_event(net_dev);
        if (ret != OAL_SUCC) {
            oam_error_log1(0, 0, "wal_cfg_vap_h2d_event FAIL %d ", ret);
            return -OAL_EFAIL;
        }
    }
    return ret;
}

int32_t hwal_ioctl_set_power(int8_t *ifname, void *buf)
{
    int32_t ret;
    oal_net_device_stru *net_dev = NULL;
    oal_bool_enum_uint8 en_power;

    if ((ifname == NULL) || (buf == NULL)) {
        oam_error_log2(0, 0, "hwal_ioctl_set_power parameter NULL.%x,%x", ifname, buf);
        return -OAL_EFAIL;
    }

    en_power = *(oal_bool_enum_uint8 *)buf;
    net_dev = oal_dev_get_by_name(ifname);
    if (net_dev == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_set_power:net_dev is NULL ");
        return -OAL_EFAIL;
    }

    // ap上下电，配置VAP
    if (en_power == 0) {
        /* device 下电 */
        hwal_device_power_down(net_dev);
        ret = OAL_SUCC;
    } else if (en_power == 1) {
        /* device 上电 */
        en_power = 0;
        ret = hwal_device_power_on(net_dev);
        if (ret != OAL_SUCC) {
            oam_error_log1(0, 0, "hwal_device_power_on FAIL %d ", ret);
            return -OAL_EFAIL;
        }

        /* 上电host device_stru初始化 */
        ret = wal_host_dev_init(net_dev);
        if (ret != OAL_SUCC) {
            oam_error_log1(0, 0, "wal_host_dev_init FAIL %d ", ret);
            return -OAL_EFAIL;
        }
    } else {
        return -OAL_EFAIL;
    }
    return ret;
}


int32_t hwal_ioctl_set_mode(int8_t *ifname, void *buf)
{
    uint32_t flags;
    oal_net_device_stru *netdev = NULL;
    oal_wiphy_stru *wiphy = NULL;
    oal_vif_params_stru params;
    hisi_set_mode_stru *set_mode = NULL;

    if ((ifname == NULL) || (buf == NULL)) {
        oam_error_log2(0, 0, "hwal_ioctl_hapdinit parameter NULL.%x,%x", ifname, buf);
        return -OAL_EFAIL;
    }

    memset_s(&params, sizeof(oal_vif_params_stru), 0, sizeof(oal_vif_params_stru));

    flags = 0;
    set_mode = (hisi_set_mode_stru *)buf;
    netdev = oal_dev_get_by_name(ifname);
    wiphy = oal_wiphy_get();
    if ((netdev == NULL) || (wiphy == NULL)) {
        oam_error_log2(0, 0, "hwal_ioctl_hapdinit netdev or wiphy NULL.%x,%x", netdev, wiphy);
        return -OAL_EFAIL;
    }

    params.use_4addr = 0;
    params.macaddr = set_mode->bssid;

    return wal_cfg80211_change_virtual_intf(wiphy, netdev, set_mode->iftype, &flags, &params);
}


int32_t hwal_ioctl_stop_ap(int8_t *ifname, void *buf)
{
    oal_net_device_stru *netdev = NULL;
    oal_wiphy_stru *wiphy = NULL;

    /* 上层buf入参为NULL，且未被使用, 此处不需做判空处理 */
    if (ifname == NULL) {
        oam_error_log2(0, 0, "hwal_ioctl_hapdinit parameter NULL.%x,%x", ifname, buf);
        return -OAL_EFAIL;
    }

    netdev = oal_dev_get_by_name(ifname);
    wiphy = oal_wiphy_get();
    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_stop_ap:netdev is NULL.");
        return -HISI_EFAIL;
    }

    if (wal_cfg80211_stop_ap(wiphy, netdev) != OAL_SUCC) {
        oam_error_log0(0, 0, "hwal_ioctl_stop_ap::wal_cfg80211_stop_ap failed.");
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}


int32_t hwal_ioctl_del_virtual_intf(int8_t *ifname, void *buf)
{
    oal_net_device_stru *netdev = NULL;
    oal_wiphy_stru *wiphy = NULL;
    oal_wireless_dev_stru *wdev = NULL;

    /* 上层buf入参为NULL，且未被使用, 此处不需做判空处理 */
    if (ifname == NULL) {
        oam_error_log2(0, 0, "hwal_ioctl_hapdinit parameter NULL.%x,%x", ifname, buf);
        return -OAL_EFAIL;
    }

    netdev = oal_dev_get_by_name(ifname);
    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_del_virtual_intf:netdev is NULL.");
        return -OAL_EFAIL;
    }

    wiphy = oal_wiphy_get();

    wdev = oal_memalloc(sizeof(oal_wireless_dev_stru));
    if (wdev == NULL) {
        oam_error_log0(0, 0, "wdev is NULL");
        return -OAL_EFAIL;
    }

    memset_s(wdev, sizeof(oal_wireless_dev_stru), 0, sizeof(oal_wireless_dev_stru));

    wdev->netdev = netdev;

    if (wal_cfg80211_del_virtual_intf(wiphy, wdev) != OAL_SUCC) {
        oam_error_log0(0, 0, "hwal_ioctl_del_virtual_intf::wal_cfg80211_del_virtual_intf failed.");
        oal_free(wdev);
        return -OAL_EFAIL;
    }
    oal_free(wdev);
    return OAL_SUCC;
}


int32_t hwal_ioctl_set_netdev(int8_t *ifname, void *buf)
{
    int32_t ret;
    oal_net_device_stru *net_dev = NULL;
    oal_bool_enum_uint8 netdev;

    if ((ifname == NULL) || (buf == NULL)) {
        oam_error_log2(0, 0, "hwal_ioctl_set_netdev parameter NULL.%x,%x", ifname, buf);
        return -OAL_EFAIL;
    }

    netdev = *(oal_bool_enum_uint8 *)buf;
    net_dev = oal_dev_get_by_name(ifname);

    if (netdev == HISI_FALSE) {
        ret = wal_netdev_stop(net_dev);
    } else if (netdev == HISI_TRUE) {
        ret = wal_netdev_open(net_dev);
    } else {
        oam_error_log1(0, 0, "hwal_ioctl_set_netdev netdev ERROR: %d", netdev);
        ret = -OAL_EFAIL;
    }

    return ret;
}


static oal_ieee80211_channel *hwal_get_channel_by_freq(oal_wiphy_stru *wiphy, int32_t freq)
{
    enum ieee80211_band band;
    int32_t n_channels;
    oal_ieee80211_supported_band *supported_band = NULL;
    int32_t loop;

    for (band = (enum ieee80211_band)0; band < IEEE80211_NUM_BANDS; band++) {
        supported_band = wiphy->bands[band];
        if (supported_band == NULL) {
            continue;
        }
        n_channels = supported_band->n_channels;
        if (n_channels < 0) {
            oam_error_log1(0, 0, "get_channel_by_freq: n_channels = %d invaild.", n_channels);
            return -HISI_EFAIL;
        }
        for (loop = 0; loop < n_channels; loop++) {
            if (supported_band->channels[loop].center_freq == freq) {
                return &supported_band->channels[loop];
            }
        }
    }

    return NULL;
}

static int32_t hwal_get_all_enable_channel(oal_wiphy_stru *wiphy,
                                           oal_ieee80211_channel_stru **channels,
                                           uint32_t *chan_num)
{
    uint32_t count = 0;
    uint32_t loop;
    enum ieee80211_band band;
    int32_t n_channels;
    oal_ieee80211_channel_stru *channel = NULL;

    for (band = (enum ieee80211_band)0; band < IEEE80211_BAND_5GHZ; band++) {
        if (wiphy->bands[band] == NULL) {
            continue;
        }
        n_channels = (uint32_t)wiphy->bands[band]->n_channels;
        if (n_channels < 0) {
            oam_error_log1(0, 0, "hwal_get_all_enable_channel: n_channels = %d invaild.", n_channels);
            return -HISI_EFAIL;
        }
        for (loop = 0; loop < n_channels; loop++) {
            channel = &(wiphy->bands[band]->channels[loop]);
            if ((channel->flags & HISI_CHAN_DISABLED) != 0) {
                continue;
            }

            *(channels + count) = channel;
            count++;
            if (count == OAL_MAX_SCAN_CHANNELS) {
                *chan_num = count;
                return HISI_SUCC;
            }
        }
    }
    *chan_num = count;
    return HISI_SUCC;
}

static int32_t hwal_get_channel(oal_wiphy_stru *wiphy, hisi_scan_stru *scan_params,
                                oal_cfg80211_scan_request_stru *request)
{
    uint32_t loop;
    uint32_t count = 0;
    int32_t ret;
    oal_ieee80211_channel_stru *channel = NULL;

    if (scan_params->freqs != NULL) {
        for (loop = 0; loop < scan_params->num_freqs; loop++) {
            channel = hwal_get_channel_by_freq(wiphy, scan_params->freqs[loop]);
            if (channel == NULL) {
                continue;
            }

            request->channels[count] = channel;
            count++;
        }
    } else {
        ret = hwal_get_all_enable_channel(wiphy, &(request->channels), &count);
        if (ret != HISI_SUCC) {
            return -HISI_EFAIL;
        }
    }

    if (count == 0) {
        oam_error_log0(0, 0, "hwal_ioctl_scan: count = 0.");
        return -HISI_EFAIL;
    } else {
        request->n_channels = count;
        return HISI_SUCC;
    }
}

static int32_t hwal_get_ssid(hisi_scan_stru *scan_params, oal_cfg80211_scan_request_stru *request)
{
    uint32_t loop;

    request->n_ssids = scan_params->num_ssids;
    for (loop = 0; (loop < scan_params->num_ssids) && (loop < HISI_WPAS_MAX_SCAN_SSIDS); loop++) {
        if (scan_params->ssids[loop].ssid_len > IEEE80211_MAX_SSID_LEN) {
            oam_error_log2(0, 0, "hwal_ioctl_scan: loop = %d, len = %d.", loop, scan_params->ssids[loop].ssid_len);
            return -HISI_EFAIL;
        }

        request->ssids[loop].ssid_len = scan_params->ssids[loop].ssid_len;
        if (memcpy_s(request->ssids[loop].ssid, request->ssids[loop].ssid_len,
            scan_params->ssids[loop].ssid, scan_params->ssids[loop].ssid_len) != EOK) {
            oam_warning_log0(0, OAM_SF_ANY, "hwal_ioctl_scan::memcpy_s failed!");
        }
    }
    return HISI_SUCC;
}

static int32_t hwal_get_scan_param(oal_wiphy_stru *wiphy, oal_net_device_stru *netdev,
                                   hisi_scan_stru *scan_params, oal_cfg80211_scan_request_stru *request)
{
    int32_t ret;
    request->wiphy = wiphy;
    request->dev = netdev;
    request->wdev = netdev->ieee80211_ptr;

    ret = hwal_get_channel(wiphy, scan_params, request);
    if (ret != HISI_SUCC) {
        oam_error_log1(0, 0, "hwal_ioctl_scan: hwal_get_channel fail ret[%d].", ret);
        return -HISI_EFAIL;
    }

    ret = hwal_get_ssid(scan_params, request);
    if (ret != HISI_SUCC) {
        oam_error_log1(0, 0, "hwal_ioctl_scan: hwal_get_ssid fail ret[%d].", ret);
        return -HISI_EFAIL;
    }

    if (scan_params->extra_ies != NULL) {
        request->ie_len = scan_params->extra_ies_len;
        request->ie = scan_params->extra_ies;
    }
    return HISI_SUCC;
}


int32_t hwal_ioctl_scan(int8_t *ifname, void *buf)
{
    oal_wiphy_stru *wiphy = NULL;
    hisi_scan_stru *scan_params = NULL;
    oal_net_device_stru *netdev = NULL;
    oal_cfg80211_scan_request_stru request;
    uint32_t ssids_size;
    int32_t ret;

    if ((ifname == NULL) || (buf == NULL)) {
        oam_error_log0(0, 0, "hwal_ioctl_scan parameter NULL.");
        return -HISI_EFAIL;
    }

    scan_params = (hisi_scan_stru *)buf;
    netdev = oal_dev_get_by_name(ifname);
    wiphy = oal_wiphy_get();

    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_scan :netdev is NULL.");
        return -HISI_EFAIL;
    }

    memset_s(&request, sizeof(oal_cfg80211_scan_request_stru), 0, sizeof(oal_cfg80211_scan_request_stru));
    if (scan_params->num_ssids != 0) {
        ssids_size = scan_params->num_ssids * sizeof(oal_cfg80211_ssid_stru);
        request.ssids = oal_memalloc(ssids_size);
        if (request.ssids == NULL) {
            oam_error_log0(0, 0, "hwal_ioctl_scan: request.ssids malloc failed.");
            return -HISI_EFAIL;
        }
        memset_s(request.ssids, ssids_size, 0, ssids_size);
    }

    ret = hwal_get_scan_param(wiphy, netdev, scan_params, &request);
    if (ret != HISI_SUCC) {
        oam_error_log0(0, 0, "hwal_ioctl_scan: hwal_get_scan_param fail.");
        if (request.ssids != NULL) {
            oal_free(request.ssids);
        }
        return -HISI_EFAIL;
    }

    if (wal_cfg80211_scan(wiphy, &request) != OAL_SUCC) {
        oam_error_log0(0, 0, "hwal_ioctl_scan:wal_cfg80211_scan failed");
        if (request.ssids != NULL) {
            oal_free(request.ssids);
        }
        return -HISI_EFAIL;
    }

    if (request.ssids != NULL) {
        oal_free(request.ssids);
    }
    return OAL_SUCC;
}


int32_t hwal_ioctl_disconnect(int8_t *ifname, void *buf)
{
    oal_wiphy_stru *wiphy = NULL;
    oal_net_device_stru *netdev = NULL;
    uint16_t reason_code;
    uint32_t pm_flag;

    if ((ifname == NULL) || (buf == NULL)) {
        oam_error_log0(0, 0, "hwal_ioctl_disconnect parameter NULL.");
        return -HISI_EFAIL;
    }

    netdev = oal_dev_get_by_name(ifname);
    wiphy = oal_wiphy_get();
    reason_code = *(uint16_t *)buf;

    if ((netdev != NULL) && (netdev->lwip_netif != NULL)) {
        pm_flag = 1;
        hwal_ioctl_set_pm_on(ifname, &pm_flag);
        hwal_lwip_notify(netdev->lwip_netif, NETDEV_DOWN);
    }

    return wal_cfg80211_disconnect(wiphy, netdev, reason_code);
}


uint8_t hwal_is_valid_ie_attr(uint8_t *ie, uint32_t ie_len)
{
    uint8_t uc_elemlen;

    /* ie可以为空 */
    if (ie == NULL) {
        return TRUE;
    }

    while (ie_len != 0) {
        if (ie_len < MAC_IE_HDR_LEN) {
            return FALSE;
        }
        ie_len -= MAC_IE_HDR_LEN;

        uc_elemlen = ie[1]; // ie偏移一位为该ie长度
        if (uc_elemlen > ie_len) {
            return FALSE;
        }
        ie_len -= uc_elemlen;
        ie += MAC_IE_HDR_LEN + uc_elemlen;
    }

    return TRUE;
}

static int32_t hwal_check_assoc_param(oal_wiphy_stru *wiphy, hisi_associate_params_stru *params)
{
    oal_ieee80211_channel_stru *channel = NULL;

    if ((params->ssid == NULL) || (params->ssid_len == 0)) {
        oam_error_log0(0, 0, "assoc_params parameter NULL.");
        return -HISI_EFAIL;
    }

    if (hwal_is_valid_ie_attr(params->ie, params->ie_len) == FALSE) {
        oam_error_log0(0, 0, "assoc_params->ie parameter FALSE.");
        return -HISI_EFAIL;
    }

    if ((params->auth_type > NL80211_AUTHTYPE_AUTOMATIC) ||
        (params->auth_type == NL80211_AUTHTYPE_SAE)) {
        oam_error_log0(0, 0, "assoc_params->auth_type ERROR.");
        return -HISI_EFAIL;
    }

    channel = hwal_get_channel_by_freq(wiphy, (int32_t)params->freq);
    if ((channel == NULL) || (channel->flags & HISI_CHAN_DISABLED)) {
        oam_error_log0(0, 0, "sme.channel ERROR.");
        return -HISI_EFAIL;
    }

    if ((params->mfp != HISI_MFP_REQUIRED) && (params->mfp != HISI_MFP_NO)) {
        oam_error_log1(0, 0, "assoc_params->mfp ERROR. mfp = %d", params->mfp);
        return -HISI_EFAIL;
    }
}



int32_t hwal_ioctl_assoc(int8_t *ifname, void *buf)
{
    oal_wiphy_stru *wiphy = NULL;
    oal_net_device_stru *netdev = NULL;
    oal_cfg80211_connect_params_stru sme;
    hisi_associate_params_stru *assoc_params = NULL;
    int32_t ret;

    if ((ifname == NULL) || (buf == NULL)) {
        oam_error_log0(0, 0, "hwal_ioctl_assoc parameter NULL.");
        return -HISI_EFAIL;
    }

    memset_s(&sme, sizeof(oal_cfg80211_connect_params_stru), 0, sizeof(oal_cfg80211_connect_params_stru));
    netdev = oal_dev_get_by_name(ifname);
    wiphy = oal_wiphy_get();
    assoc_params = (hisi_associate_params_stru *)buf;
    ret = hwal_check_assoc_param(wiphy, assoc_params);
    if (ret != HISI_SUCC) {
        oam_error_log0(0, 0, "hwal_ioctl_assoc assoc_params parameter err.");
        return -HISI_EFAIL;
    }

    sme.ssid = assoc_params->ssid;
    sme.ssid_len = assoc_params->ssid_len;
    sme.ie = assoc_params->ie;
    sme.ie_len = assoc_params->ie_len;
    sme.auth_type = assoc_params->auth_type;
    sme.channel = hwal_get_channel_by_freq(wiphy, (int32_t)assoc_params->freq);

    if (assoc_params->bssid != NULL) {
        sme.bssid = assoc_params->bssid;
    }

    sme.privacy = assoc_params->privacy;
    sme.mfp = (enum nl80211_mfp)assoc_params->mfp;

    if (assoc_params->key != NULL) {
        sme.key = assoc_params->key;
        sme.key_len = assoc_params->key_len;
        sme.key_idx = assoc_params->key_idx;
    }

    if (memcpy_s(&sme.crypto, sizeof(hisi_crypto_settings_stru), &assoc_params->crypto,
                 sizeof(hisi_crypto_settings_stru)) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "hwal_ioctl_assoc::memcpy_s failed!");
        return -HISI_EFAIL;
    }

    return wal_cfg80211_connect(wiphy, netdev, &sme);
}


int32_t hwal_ioctl_dhcp_start(int8_t *ifname, void *buf)
{
    oal_net_device_stru *netdev = NULL;

    if (ifname == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_scan parameter NULL.");
        return -HISI_EFAIL;
    }

    netdev = oal_dev_get_by_name(ifname);

    return hwal_dhcp_start(netdev);
}


int32_t hwal_ioctl_dhcp_stop(int8_t *ifname, void *buf)
{
    oal_net_device_stru *netdev = NULL;

    if (ifname == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_scan parameter NULL.");
        return -HISI_EFAIL;
    }

    netdev = oal_dev_get_by_name(ifname);

    return hwal_dhcp_stop(netdev);
}


int32_t hwal_ioctl_dhcp_succ_check(int8_t *ifname, void *buf)
{
    oal_net_device_stru *netdev = NULL;

    if (ifname == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_dhcp_succ_check parameter NULL.");
        return -HISI_EFAIL;
    }

    netdev = oal_dev_get_by_name(ifname);

    return hwal_dhcp_succ_check(netdev);
}


int32_t hwal_ioctl_set_pm_on(int8_t *ifname, void *buf)
{
    oal_net_device_stru *cfg_net_dev = NULL;

    if (ifname == NULL || buf == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_set_pm_on:ifname or buf NULL.");
        return -OAL_EFAIL;
    }

    cfg_net_dev = oal_dev_get_by_name(ifname);
    if (cfg_net_dev == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_set_pm_on:cfg_net_dev NULL.");
        return -OAL_EFAIL;
    }

    return wal_set_pm_on(cfg_net_dev, buf);
}


int32_t hwal_ioctl_ip_notify(int8_t *ifname, void *buf)
{
    oal_net_device_stru *netdev = NULL;
    uint32_t ipflg;

    if (ifname == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_ip_notify parameter NULL.");
        return -HISI_EFAIL;
    }

    netdev = oal_dev_get_by_name(ifname);
    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_ip_notify:netdev is NULL.");
        return -HISI_EFAIL;
    }

    ipflg = *(uint32_t *)buf;

    return hwal_lwip_notify(netdev->lwip_netif, ipflg);
}


int32_t hwal_ioctl_set_max_sta(int8_t *ifname, void *p_max_sta_num)
{
    oal_net_device_stru *netdev = NULL;
    uint32_t max_sta_num;

    if (ifname == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_set_max_sta parameter NULL.");
        return -HISI_EFAIL;
    }

    netdev = oal_dev_get_by_name(ifname);
    if (netdev == NULL) {
        oam_error_log0(0, 0, "hwal_ioctl_set_max_sta:netdev is NULL.");
        return -HISI_EFAIL;
    }

    max_sta_num = *(uint32_t *)p_max_sta_num;

#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
    return wal_init_max_sta_num(netdev, max_sta_num);
#else
    return HISI_SUCC;
#endif
}


int32_t hwal_ioctl_sta_remove(int8_t *ifname, void *buf)
{
    oal_wiphy_stru *wiphy = NULL;
    oal_net_device_stru *netdev = NULL;
    uint8_t *mac = NULL;

    if ((ifname == NULL) || (buf == NULL)) {
        oam_error_log0(0, 0, "hwal_ioctl_sta_remove parameter NULL.");
        return -OAL_EFAIL;
    }

    netdev = oal_dev_get_by_name(ifname);
    if (netdev == NULL) {
        oam_error_log0(0, 0, "{hwal_ioctl_sta_remove:netdev is NULL.}");
        return -OAL_EFAIL;
    }

    wiphy = oal_wiphy_get();
    if (wiphy == NULL) {
        oam_error_log0(0, 0, "{hwal_ioctl_sta_remove wiphy NULL.}");
        return -OAL_EFAIL;
    }
    mac = (uint8_t*)buf;

    return wal_cfg80211_del_station(wiphy, netdev, mac);
}


int32_t hisi_hwal_wpa_ioctl(int8_t *ifname, hisi_ioctl_command_stru *cmd)
{
    if ((ifname == NULL) || (cmd == NULL)) {
        oam_error_log2(0, OAM_SF_ANY, "hwal_wpa_ioctl::ifname = %x,buf = %x", ifname, cmd);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if ((cmd->cmd < HWAL_EVENT_BUTT) && (g_ast_hwal_ioctl_handlers[cmd->cmd] != NULL)) {
        return g_ast_hwal_ioctl_handlers[cmd->cmd](ifname, cmd->buf);
    }
    oam_error_log1(0, 0, "hwal_wpa_ioctl ::The CMD[%d] handlers is NULL.", cmd->cmd);

    return -OAL_EFAIL;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

