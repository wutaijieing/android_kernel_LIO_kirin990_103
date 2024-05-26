

// 1 头文件包含
#include "hwal_event.h"
#include "hwal_app_ioctl.h"
#include "hwal_net.h"
#include "securec.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HWAL_EVENT_C

// 2 全局变量定义
hisi_upload_frame_cb  g_upload_frame_func = NULL;
hisi_send_event_cb    g_send_event_func = NULL;
oal_spin_lock_stru    g_upload_frame_lock;

// 3 函数实现

void hisi_register_send_event_cb(hisi_send_event_cb func)
{
    g_send_event_func = func;
}


void cfg80211_new_sta(oal_net_device_stru *dev, const uint8_t *macaddr,
                      oal_station_info_stru *sta_info, oal_gfp_enum_uint8 gfp)
{
    hisi_new_sta_info_stru new_sta_info;

    memset_s(&new_sta_info, sizeof(hisi_new_sta_info_stru), 0, sizeof(hisi_new_sta_info_stru));

    /* 入参检查 */
    if ((dev == NULL) || (macaddr == NULL) || (sta_info == NULL)) {
        oam_error_log3(0, OAM_SF_ANY,
            "{cfg80211_new_sta::dev or macaddr or sta_info null ptr error %x,%x,%x.}",
            dev, macaddr, sta_info);
        return;
    }

    if ((sta_info->assoc_req_ies == NULL) || (sta_info->assoc_req_ies_len == 0)) {
        oam_error_log2(0, OAM_SF_ANY, "{cfg80211_new_sta::assoc_req_ies or assoc_req_ies_len null error %x,%d.}",
            sta_info->assoc_req_ies, sta_info->assoc_req_ies_len);
        return;
    }

    new_sta_info.ie = NULL;
    new_sta_info.ie_len = sta_info->assoc_req_ies_len;
    new_sta_info.reassoc = 0;
    if (memcpy_s(new_sta_info.macaddr, ETH_ALEN, macaddr, ETH_ALEN) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "{cfg80211_new_sta::memcpy_s failed.}");
    }
    new_sta_info.ie = (uint8_t*)(oal_memalloc(sta_info->assoc_req_ies_len));
    if (new_sta_info.ie == NULL) {
        oam_error_log1(0, OAM_SF_ANY, "{cfg80211_new_sta::pnew_sta_info->ie oal_memalloc error %x.}",
            new_sta_info.ie);
        return;
    }
    if (memcpy_s(new_sta_info.ie, new_sta_info.ie_len, sta_info->assoc_req_ies,
                 sta_info->assoc_req_ies_len) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "{cfg80211_new_sta::memcpy_s failed.}");
    }

    if (g_send_event_func == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "{cfg80211_new_sta::g_send_event_func is null.}");
        oal_free(new_sta_info.ie);
        new_sta_info.ie = NULL;
        return;
    }
    g_send_event_func(HISI_ELOOP_EVENT_NEW_STA, (uint8_t *)&new_sta_info, sizeof(hisi_new_sta_info_stru));
    return;
}


void cfg80211_del_sta(oal_net_device_stru *dev, const uint8_t *mac_addr, oal_gfp_enum_uint8 gfp)
{
    /* 入参检查 */
    if ((dev == NULL) || (mac_addr == NULL)) {
        oam_error_log2(0, OAM_SF_ANY, "{cfg80211_del_sta::dev or mac_addr ptr NULL %x,%x.}",
            dev, mac_addr);
        return;
    }

    if (g_send_event_func == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "{cfg80211_del_sta::g_send_event_func is null.}");
        return;
    }

    g_send_event_func(HISI_ELOOP_EVENT_DEL_STA, (uint8_t *)mac_addr, ETH_ADDR_LEN);
    return;
}


oal_bool_enum_uint8 cfg80211_rx_mgmt(oal_wireless_dev_stru *wdev, int32_t freq, int32_t sig_mbm,
                                     const uint8_t *buf, size_t len, oal_gfp_enum_uint8 gfp)
{
    hisi_rx_mgmt_stru rx_mgmt;

    memset_s(&rx_mgmt, sizeof(hisi_rx_mgmt_stru), 0, sizeof(hisi_rx_mgmt_stru));
    /* 入参检查 */
    if ((wdev == NULL) || (buf == NULL)) {
        oam_error_log2(0, OAM_SF_ANY, "{cfg80211_rx_mgmt::wdev or buf ptr NULL %x,%x.}", wdev, buf);
        return FALSE;
    }

    rx_mgmt.buf = NULL;
    rx_mgmt.len = len;
    rx_mgmt.sig_mbm = sig_mbm;

    if (len != 0) {
        rx_mgmt.buf = oal_memalloc(len);
        if (rx_mgmt.buf == NULL) {
            oam_error_log1(0, OAM_SF_ANY, "{cfg80211_rx_mgmt::prx_mgmt->buf oal_memalloc error %x.}",
                rx_mgmt.buf);
            return FALSE;
        }
        if (memcpy_s(rx_mgmt.buf, rx_mgmt.len, buf, len) != EOK) {
            oam_error_log0(0, OAM_SF_ANY, "cfg80211_rx_mgmt::memcpy_s failed!");
            oal_free(rx_mgmt.buf);
            rx_mgmt.buf = NULL;
            return FALSE;
        }
    }

    if (g_send_event_func == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "{cfg80211_rx_mgmt::g_send_event_func is null.}");
        if (rx_mgmt.buf != NULL) {
            oal_free(rx_mgmt.buf);
            rx_mgmt.buf = NULL;
        }
        return FALSE;
    }

    g_send_event_func(HISI_ELOOP_EVENT_RX_MGMT, (uint8_t *) &rx_mgmt, sizeof(hisi_rx_mgmt_stru));
    return TRUE;
}


oal_bool_enum_uint8 cfg80211_mgmt_tx_status(struct wireless_dev *wdev, uint64_t cookie,
    const uint8_t *buf, size_t len, oal_bool_enum_uint8 ack, oal_gfp_enum_uint8 gfp)
{
    hisi_tx_status_stru tx_status;

    memset_s(&tx_status, sizeof(hisi_tx_status_stru), 0, sizeof(hisi_tx_status_stru));
    /* 入参检查 */
    if ((wdev == NULL) || (buf == NULL)) {
        oam_error_log2(0, OAM_SF_ANY, "{cfg80211_mgmt_tx_status::wdev or buf ptr NULL %x,%x.}",
            wdev, buf);
        return FALSE;
    }

    tx_status.buf = NULL;
    tx_status.len = len;
    tx_status.ack = ack;

    if (len != 0) {
        tx_status.buf = oal_memalloc(len);
        if (tx_status.buf == NULL) {
            oam_error_log1(0, OAM_SF_ANY,
                "{cfg80211_mgmt_tx_status::ptx_status->buf oal_memalloc error %x.}", tx_status.buf);
            return FALSE;
        }
        if (memcpy_s(tx_status.buf, tx_status.len, buf, len) != EOK) {
            oam_error_log0(0, OAM_SF_ANY, "cfg80211_mgmt_tx_status::memcpy_s failed!");
            oal_free(tx_status.buf);
            return FALSE;
        }
    }

    if (g_send_event_func == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "{cfg80211_mgmt_tx_status::g_send_event_func is null.}");
        if (tx_status.buf != NULL) {
            oal_free(tx_status.buf);
        }
        return FALSE;
    }

    g_send_event_func(HISI_ELOOP_EVENT_TX_STATUS, (uint8_t *)&tx_status, sizeof(hisi_tx_status_stru));
    return TRUE;
}


void cfg80211_inform_bss_frame(oal_wiphy_stru *wiphy,
                               oal_ieee80211_channel_stru *ieee80211_channel,
                               oal_ieee80211_mgmt_stru *mgmt,
                               uint32_t len,
                               int32_t signal,
                               oal_gfp_enum_uint8 ftp)
{
    hisi_scan_result_stru  scan_result;
    uint32_t ie_len, beacon_ie_len;

    memset_s(&scan_result, sizeof(hisi_scan_result_stru), 0, sizeof(hisi_scan_result_stru));

    if ((wiphy == NULL) || (ieee80211_channel == NULL) || (mgmt == NULL)) {
        oam_error_log3(0, OAM_SF_ANY,
            "{cfg80211_inform_bss_frame::wiphy or ieee80211_channel or mgmt null ptr error %x,%x,%x.}",
            wiphy, ieee80211_channel, mgmt);
        return;
    }

    ie_len = len - (uint32_t)offsetof(oal_ieee80211_mgmt_stru, u.probe_resp.variable);
    beacon_ie_len = len - (uint32_t)offsetof(oal_ieee80211_mgmt_stru, u.beacon.variable);

    scan_result.variable = oal_memalloc(ie_len + beacon_ie_len);
    if (scan_result.variable == NULL) {
        oam_error_log2(0, OAM_SF_ANY,
            "{cfg80211_inform_bss_frame::os_zalloc error.ie_len = %d beacon_ie_len = %d}", ie_len, beacon_ie_len);
        return;
    }

    memset_s(scan_result.variable, ie_len, 0, ie_len);
    scan_result.ie_len = ie_len;
    scan_result.beacon_ie_len = beacon_ie_len;
    scan_result.beacon_int = (int16_t)mgmt->u.probe_resp.beacon_int;
    scan_result.caps = (int16_t) mgmt->u.probe_resp.capab_info;
    scan_result.level = signal;
    scan_result.freq = ieee80211_channel->center_freq;
    scan_result.flags = (int32_t)ieee80211_channel->flags;
    scan_result.tsf = mgmt->u.probe_resp.timestamp;

    if (memcpy_s(scan_result.bssid, ETH_ADDR_LEN, mgmt->bssid, ETH_ALEN) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "cfg80211_inform_bss_frame::memcpy_s failed!");
    }
    if (memcpy_s(scan_result.variable, (ie_len + beacon_ie_len), mgmt->u.probe_resp.variable, ie_len) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "cfg80211_inform_bss_frame::memcpy_s failed!");
    }
    if (memcpy_s(scan_result.variable + ie_len, beacon_ie_len, mgmt->u.beacon.variable, beacon_ie_len) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "cfg80211_inform_bss_frame::memcpy_s failed!");
    }
    if (g_send_event_func == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "{cfg80211_inform_bss_frame::g_send_event_func is null.}");
        oal_free(scan_result.variable);
        return;
    }

    g_send_event_func(HISI_ELOOP_EVENT_SCAN_RESULT, (uint8_t *) &scan_result, sizeof(hisi_scan_result_stru));

    return;
}
/*
 * 功能描述  : 填写上报关联result结果
 * 修改历史  : 新增函数
 */
static uint32_t cfg80211_fill_result(const uint8_t *req_ie, size_t req_ie_len,
    const uint8_t *resp_ie, size_t resp_ie_len, hisi_connect_result_stru *result)
{
    if ((req_ie != NULL) && (req_ie_len != 0)) {
        result->req_ie = oal_memalloc(req_ie_len);
        result->req_ie_len = req_ie_len;
        if (result->req_ie == NULL) {
            oam_error_log0(0, OAM_SF_ANY, "{cfg80211_connect_result::connect_result->req_ie os_zalloc error.}");
            return OAL_FAIL;
        }
        if (memcpy_s(result->req_ie, result->req_ie_len, req_ie, req_ie_len) != EOK) {
            oam_error_log0(0, OAM_SF_ANY, "{cfg80211_connect_result::memcpy_s failed}");
        }
    }

    if ((resp_ie != NULL) && (resp_ie_len != 0)) {
        result->resp_ie = oal_memalloc(resp_ie_len);
        if (result->resp_ie == NULL) {
            oam_error_log0(0, OAM_SF_ANY, "{cfg80211_connect_result::connect_result->resp_ie os_zalloc error.}");
            if (result->req_ie != NULL) {
                oal_free(result->req_ie);
                result->req_ie = NULL;
            }
            return OAL_FAIL;
        }
        result->resp_ie_len = resp_ie_len;
        if (memcpy_s(result->resp_ie, result->resp_ie_len, resp_ie, resp_ie_len) != EOK) {
            oam_error_log0(0, OAM_SF_ANY, "{cfg80211_connect_result::memcpy_s failed}");
        }
    }
    return OAL_SUCC;
}

void cfg80211_connect_result(oal_net_device_stru *dev, const uint8_t *bssid,
                             const uint8_t *req_ie, size_t req_ie_len,
                             const uint8_t *resp_ie, size_t resp_ie_len,
                             uint16_t status, oal_gfp_enum_uint8 gfp)
{
    hisi_connect_result_stru connect_result;
    uint32_t pm_flag;
    uint32_t ret;

    memset_s(&connect_result, sizeof(hisi_connect_result_stru), 0, sizeof(hisi_connect_result_stru));

    if ((dev == NULL) || (bssid == NULL)) {
        oam_error_log2(0, OAM_SF_ANY, "{cfg80211_connect_result::parameter null %x. %x}", dev, bssid);
        return;
    }

    if (memcpy_s(connect_result.bssid, ETH_ADDR_LEN, bssid, ETH_ALEN) != EOK) {
        oam_error_log0(0, OAM_SF_ANY, "{cfg80211_connect_result::memcpy_s failed}");
    }

    ret = cfg80211_fill_result(req_ie, req_ie_len, resp_ie, resp_ie_len, &connect_result);
    if (ret == OAL_FAIL) {
        return;
    }

    connect_result.status = status;

    if (status == 0) {
        /* 如果上报关联成功则关闭PM */
        pm_flag = 0;
        hwal_ioctl_set_pm_on("wlan0", &pm_flag);
    }
    if (g_send_event_func == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "{cfg80211_connect_result::g_send_event_func is null.}");
        if (connect_result.req_ie != NULL) {
            oal_free(connect_result.req_ie);
        }
        if (connect_result.resp_ie != NULL) {
            oal_free(connect_result.resp_ie);
        }
        return;
    }

    g_send_event_func(HISI_ELOOP_EVENT_CONNECT_RESULT, (uint8_t *) &connect_result,
                      sizeof(hisi_connect_result_stru));
    return;
}


uint32_t cfg80211_disconnected(oal_net_device_stru *net_device, uint16_t reason,
                               uint8_t *ie, uint32_t ie_len, oal_gfp_enum_uint8 gfp)
{
    hisi_disconnect_stru disconnect;
    uint32_t pm_flag;

    memset_s(&disconnect, sizeof(hisi_disconnect_stru), 0, sizeof(hisi_disconnect_stru));

    if ((net_device == NULL)) {
        oam_error_log1(0, OAM_SF_ANY, "{cfg80211_disconnected::net_device %x.}", net_device);
        return OAL_FAIL;
    }

    if ((ie != NULL) && (ie_len != 0)) {
        disconnect.ie = oal_memalloc(ie_len);
        disconnect.ie_len = ie_len;
        if (disconnect.ie == NULL) {
            oam_error_log0(0, OAM_SF_ANY, "{cfg80211_disconnected::pconnect_result->req_ie os_zalloc error.}");
            return OAL_FAIL;
        }
        if (memcpy_s(disconnect.ie, disconnect.ie_len, ie, ie_len) != EOK) {
            oam_error_log0(0, OAM_SF_ANY, "{cfg80211_disconnected::memcpy_s failed .}");
            oal_free(disconnect.ie);
            return OAL_FAIL;
        }
    }

    disconnect.reason = reason;

    if (net_device->lwip_netif != NULL) {
        pm_flag = 1;
        hwal_ioctl_set_pm_on("wlan0", &pm_flag);
        hwal_lwip_notify(net_device->lwip_netif, NETDEV_DOWN);
    }

    if (g_send_event_func == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "{cfg80211_disconnected::g_send_event_func is null.}");
        oal_free(disconnect.ie);
        return OAL_FAIL;
    }

    g_send_event_func(HISI_ELOOP_EVENT_DISCONNECT, (uint8_t *)&disconnect, sizeof(hisi_disconnect_stru));
    return OAL_SUCC;
}


void cfg80211_scan_done(oal_cfg80211_scan_request_stru *pst_request, uint8_t uc_aborted)
{
    if (g_send_event_func == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "{cfg80211_scan_done::g_send_event_func is null.}");
        return;
    }

    g_send_event_func(HISI_ELOOP_EVENT_SCAN_DONE, NULL, 0);
}


void cfg80211_rx_exception(oal_net_device_stru *netdev, uint8_t *data, uint32_t data_len)
{
    if (g_send_event_func == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "{cfg80211_rx_exception::g_send_event_func is null.}");
        return;
    }
    if ((data == NULL) || (data_len > 32)) {
        oam_error_log1(0, OAM_SF_ANY, "{cfg80211_rx_exception::data is null or data_len[%d] too long.}", data_len);
        return;
    }
    g_send_event_func(HISI_ELOOP_EVENT_RX_DFX, data, data_len);
}

#ifdef _PRE_WLAN_FEATURE_HILINK

uint32_t hwal_send_others_bss_data(oal_netbuf_stru *netbuf)
{
    uint8_t *data = NULL;
    uint32_t ret;
    uint8_t *payload = NULL;
    uint32_t len;
    if (netbuf == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "hwal_send_others_bss_data::netbuf is null");
        return OAL_ERR_CODE_PTR_NULL;
    }
    len = OAL_NETBUF_LEN(netbuf);
    if (len != 0) {
        payload = OAL_NETBUF_PAYLOAD(netbuf);

        oal_spin_lock(&g_upload_frame_lock);
        if (g_upload_frame_func != NULL) {
            g_upload_frame_func(payload, len);
        }
        oal_spin_unlock(&g_upload_frame_lock);
    }
    return OAL_SUCC;
}


uint32_t hisi_wlan_register_upload_frame_cb(hisi_upload_frame_cb func)
{
    oal_spin_lock_init(&g_upload_frame_lock);
    oal_spin_lock(&g_upload_frame_lock);
    g_upload_frame_func = func;
    oal_spin_unlock(&g_upload_frame_lock);
    return OAL_SUCC;
}
#endif // #ifdef _PRE_WLAN_FEATURE_HILINK
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

