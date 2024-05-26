

#include "oal_net.h"
#include "oal_cfg80211.h"
#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "wal_linux_cfgvendor.h"
#include "wal_linux_ioctl.h"
#include "wal_cfg_ioctl.h"
#include "wal_linux_cfg80211.h"
#include "securec.h"
#include "mac_mib.h"
#include "mac_ftm.h"
#include "wal_linux_netdev_ops.h"
#ifdef _PRE_WLAN_CHBA_MGMT
#include "hmac_chba_mgmt.h"
#include "hmac_chba_interface.h"
#endif

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_CFGVENDOR_C

#define OUI_VENDOR 0x001A11
#define OUI_HISI   0x001018
#define WAL_CFGVENDOR_ALIVE_IP_PKT_LEN_INVALID(ip_pkt_len) \
    (((ip_pkt_len) > MKEEP_ALIVE_IP_PKT_MAXLEN) || ((ip_pkt_len) == 0))
#define WAL_CFGVENDOR_ALIVE_IP_PKT_IS_VALID(ip_pkt_len, ip_pkt) \
    (((ip_pkt_len) > 0) && ((ip_pkt) != NULL))

wal_cfgvendor_radio_stat_stru g_st_wifi_radio_stat;


OAL_STATIC uint32_t wal_cfgvendor_copy_channel_list(mac_vendor_cmd_channel_list_stru *pst_channel_list,
    uint32_t band, int32_t *pl_channel_list, uint32_t *pul_num_channels)
{
    uint8_t *puc_chanel_list = NULL;
    uint32_t loop;
    uint32_t channel_num = 0;

    *pul_num_channels = 0;

    if ((pst_channel_list->uc_channel_num_5g == 0) && (pst_channel_list->uc_channel_num_2g == 0)) {
        oam_warning_log1(0, OAM_SF_ANY,
            "{wal_cfgvendor_copy_channel_list::get 5G or 2G channel list fail,band = %d", band);
        return OAL_FAIL;
    }

    /* 获取2G,信道列表 */
    if ((band & WIFI_BAND_24_GHZ) && (pst_channel_list->uc_channel_num_2g != 0)) {
        puc_chanel_list = pst_channel_list->auc_channel_list_2g;
        for (loop = 0; loop < pst_channel_list->uc_channel_num_2g; loop++) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0))
            pl_channel_list[channel_num++] =
                oal_ieee80211_channel_to_frequency(puc_chanel_list[loop], NL80211_BAND_2GHZ);
#else
            pl_channel_list[channel_num++] =
                oal_ieee80211_channel_to_frequency(puc_chanel_list[loop], IEEE80211_BAND_2GHZ);
#endif
        }
    }

    /* 获取5G Band信道列表，包括非DFS Channels和DFS Channels */
    if ((band & (WIFI_BAND_5_GHZ | WIFI_BAND_5_GHZ_DFS_ONLY)) && (pst_channel_list->uc_channel_num_5g != 0)) {
        oal_bool_enum_uint8 en_dfs;
        puc_chanel_list = pst_channel_list->auc_channel_list_5g;

        for (loop = 0; loop < pst_channel_list->uc_channel_num_5g; loop++) {
            en_dfs = mac_is_dfs_channel(MAC_RC_START_FREQ_5, puc_chanel_list[loop]);
            if (((band & WIFI_BAND_5_GHZ_DFS_ONLY) && (en_dfs == OAL_TRUE)) ||
                ((band & WIFI_BAND_5_GHZ) && (en_dfs == OAL_FALSE))) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0))
                pl_channel_list[channel_num++] =
                    oal_ieee80211_channel_to_frequency(puc_chanel_list[loop], NL80211_BAND_5GHZ);
#else
                pl_channel_list[channel_num++] =
                    oal_ieee80211_channel_to_frequency(puc_chanel_list[loop], IEEE80211_BAND_5GHZ);
#endif
            }
        }
    }

    if (channel_num == 0) {
        oam_warning_log1(0, OAM_SF_ANY,
            "{wal_cfgvendor_copy_channel_list::get 5G or 2G channel list fail,ul_channel_num = %d", channel_num);
        return OAL_FAIL;
    }

    *pul_num_channels = channel_num;

    return OAL_SUCC;
}


OAL_STATIC uint32_t wal_cfgvendor_get_current_channel_list(oal_net_device_stru *pst_netdev,
    uint32_t band, int32_t *pl_channel_list, uint32_t *pul_num_channels)
{
    int32_t l_ret;
    wal_msg_query_stru st_query_msg;
    wal_msg_stru *pst_rsp_msg = NULL;
    wal_msg_rsp_stru *pst_query_rsp_msg = NULL;
    mac_vendor_cmd_channel_list_stru *pst_channel_list = NULL;

    if (oal_any_null_ptr3(pst_netdev, pl_channel_list, pul_num_channels)) {
        oam_error_log0(0, OAM_SF_ANY, "{wal_cfgvendor_get_current_channel_list::channel_list or num_channel is NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 上层在任何时候都可能下发此命令，需要先判断当前netdev的状态并及时返回 */
    if (oal_unlikely(NULL == oal_net_dev_priv(pst_netdev))) {
        oam_warning_log0(0, OAM_SF_ANY, "{wal_cfgvendor_get_current_channel_list::NET_DEV_PRIV is NULL.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 消息参数准备 */
    st_query_msg.en_wid = WLAN_CFGID_VENDOR_CMD_GET_CHANNEL_LIST;

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_netdev, WAL_MSG_TYPE_QUERY, WAL_MSG_WID_LENGTH,
                               (uint8_t *)&st_query_msg, OAL_TRUE, &pst_rsp_msg);
    if (l_ret != OAL_SUCC) {
        oam_warning_log1(0, OAM_SF_ANY,
            "{wal_cfgvendor_get_current_channel_list:: wal_send_cfg_event return err code %d!}\r\n", l_ret);
        if (pst_rsp_msg != NULL) {
            oal_free(pst_rsp_msg);
        }
        return (uint32_t)l_ret;
    }

    pst_query_rsp_msg = (wal_msg_rsp_stru *)(pst_rsp_msg->auc_msg_data);
    pst_channel_list = (mac_vendor_cmd_channel_list_stru *)(pst_query_rsp_msg->auc_value);

    if (wal_cfgvendor_copy_channel_list(pst_channel_list, band, pl_channel_list, pul_num_channels) != OAL_SUCC) {
        oal_free(pst_rsp_msg);
        return OAL_FAIL;
    }

    oal_free(pst_rsp_msg);
    return OAL_SUCC;
}

OAL_STATIC int32_t wal_cfgvendor_get_channel_list(oal_wiphy_stru *wiphy, oal_wireless_dev_stru *wdev,
                                                  const void *data, int32_t len)
{
    int32_t err;
    int32_t type;
    int32_t al_channel_list[VENDOR_CHANNEL_LIST_ALL + 1] = { 0 };
    uint32_t band = 0; /* 上层下发的band值 */
    uint32_t mem_needed, ret;
    uint32_t num_channels = 0;
    uint32_t reply_len;
    oal_netbuf_stru *skb = NULL;

    if (oal_any_null_ptr3(wdev, wdev->netdev, wiphy)) {
        oam_error_log0(0, OAM_SF_ANY, "{wal_cfgvendor_get_channel_list::wdev or netdev is NULL!}");
        return -OAL_EFAIL;
    }
    if ((data == NULL) || (!oal_nla_ok((oal_nlattr_stru *)data, len))) {
        oam_error_log1(0, OAM_SF_ANY, "{wal_cfgvendor_get_channel_list::invalid para l_len=%d.}", len);
        return -OAL_EINVAL;
    }

    type = oal_nla_type(data);
    if (type == GSCAN_ATTRIBUTE_BAND) {
        if (oal_nla_len(data) < (int)sizeof(uint32_t)) {
            oam_error_log2(0, OAM_SF_ANY, "wal_cfgvendor_get_channel_list:len=%d, nla_len=%d", len, oal_nla_len(data));
            return -OAL_EFAIL;
        }
        /* 获取band值 bit0:2G信道列表 bit1:5G信道列表 bit2:DFS channel */
        band = oal_nla_get_u32(data);
    } else {
        oam_warning_log1(0, OAM_SF_ANY, "{wal_cfgvendor_get_channel_list::invalid nla type! type %d.}", type);
        return -OAL_EFAIL;
    }
    ret = wal_cfgvendor_get_current_channel_list(wdev->netdev, band, al_channel_list, &num_channels);
    if (ret != OAL_SUCC) {
        oam_warning_log3(0, OAM_SF_ANY, "{fail. ret %d, band %d, num_channels %d.}", ret, band, num_channels);
        return -OAL_EFAIL;
    }
    reply_len = sizeof(al_channel_list[0]) * (num_channels);
    mem_needed = reply_len + VENDOR_REPLY_OVERHEAD + (ATTRIBUTE_U32_LEN * 2);
    /* 申请SKB 上报信道列表 */
    skb = oal_cfg80211_vendor_cmd_alloc_reply_skb(wiphy, mem_needed);
    if (oal_unlikely(!skb)) {
        oam_warning_log1(0, OAM_SF_ANY, "wal_cfgvendor_get_channel_list::skb alloc failed.len %d\r\n", mem_needed);
        return -OAL_ENOMEM;
    }

    oal_nla_put_u32(skb, GSCAN_ATTRIBUTE_NUM_CHANNELS, num_channels);
    oal_nla_put(skb, GSCAN_ATTRIBUTE_CHANNEL_LIST, reply_len, al_channel_list);

    err = oal_cfg80211_vendor_cmd_reply(skb);
    if (oal_unlikely(err)) {
        oam_warning_log1(0, OAM_SF_ANY, "wal_cfgvendor_get_channel_list::cfg80211_vendor_cmd_reply failed ret:%d", err);
        return err;
    }

    oam_warning_log2(0, OAM_SF_ANY, "{wal_cfgvendor_get_channel_list::band %d, num_channels %d.}", band, num_channels);

    return err;
}

OAL_STATIC int32_t wal_cfgvendor_set_country(oal_wiphy_stru *wiphy,
                                             oal_wireless_dev_stru *wdev, const void *data, int32_t len)
{
    int32_t rem;
    int32_t type;
    int32_t l_ret = OAL_SUCC;
    int8_t auc_country_code[WLAN_COUNTRY_STR_LEN] = { 0 };
    const oal_nlattr_stru *iter = NULL;

    /* 测试阶段可采用定制化99配置 */
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    if (g_cust_country_code_ignore_flag.en_country_code_ingore_ini_flag == OAL_TRUE) {
        oam_warning_log1(0, OAM_SF_ANY, "{wal_cfgvendor_set_country::set_country ignored, flag ini[%d]",
                         g_cust_country_code_ignore_flag.en_country_code_ingore_ini_flag);
        return l_ret;
    }
#endif
    if ((wdev == NULL) || (data == NULL) || (len <= 0)) {
        oam_error_log2(0, OAM_SF_ANY, "{wal_cfgvendor_set_country::invalid para wdev is null[%d] l_len=%d.}",
            (wdev == NULL), len);
        return -OAL_EINVAL;
    }

    oal_nla_for_each_attr(iter, data, len, rem)
    {
        memset_s(auc_country_code, WLAN_COUNTRY_STR_LEN, 0, WLAN_COUNTRY_STR_LEN);
        type = oal_nla_type(iter);
        switch (type) {
            case ANDR_WIFI_ATTRIBUTE_COUNTRY:
#ifdef _PRE_WLAN_FEATURE_11D
                if (EOK != memcpy_s(auc_country_code, sizeof(auc_country_code), oal_nla_data(iter),
                                    oal_min(oal_nla_len(iter), sizeof(auc_country_code)))) {
                    oam_error_log0(0, OAM_SF_ANY, "wal_cfgvendor_set_country::memcpy fail!");
                    return -OAL_EFAIL;
                }
                oam_warning_log1(0, OAM_SF_ANY, "{wal_cfgvendor_set_country:: len = %d!}", oal_nla_len(iter));
                /* 设置国家码到wifi 驱动 */
                l_ret = wal_regdomain_update_country_code(wdev->netdev, auc_country_code);
#else
                oam_warning_log0(0, OAM_SF_ANY,
                    "{wal_cfgvendor_set_country::_PRE_WLAN_FEATURE_11D is not define!}\r\n");
#endif
                break;
            default:
                oam_warning_log1(0, OAM_SF_ANY, "{wal_cfgvendor_set_country::unknow type:%d!}\r\n", type);
                break;
        }
    }

    return l_ret;
}

OAL_STATIC uint32_t wal_cfgvendor_do_get_feature_set(mac_wiphy_priv_stru *pst_wiphy_priv)
{
    uint32_t feature_set = 0;
    mac_device_stru *pst_mac_device = NULL;

    if (pst_wiphy_priv == NULL) {
        oam_warning_log0(0, OAM_SF_ANY, "{wal_cfgvendor_do_get_feature_set::pst_mac_device is null!}");
        return feature_set;
    }

    pst_mac_device = pst_wiphy_priv->pst_mac_device;
    if (pst_mac_device != NULL) {
        feature_set |= ((mac_device_band_is_support(pst_mac_device->uc_device_id, MAC_DEVICE_CAP_5G) == OAL_TRUE) ?
            WIFI_FEATURE_INFRA_5G : 0);
    }
#ifdef _PRE_WLAN_FEATURE_HS20
    feature_set |= WIFI_FEATURE_HOTSPOT;
#endif
    feature_set |= WIFI_FEATURE_LINK_LAYER_STATS; /** 0x10000 Link layer stats collection */
    
    return feature_set;
}

OAL_STATIC int32_t wal_cfgvendor_get_feature_set(oal_wiphy_stru *wiphy,
                                                 oal_wireless_dev_stru *wdev, const void *data, int32_t len)
{
    int32_t l_err;
    uint32_t reply;
    int32_t l_reply_len = sizeof(int32_t);
    oal_netbuf_stru *skb = NULL;
    mac_wiphy_priv_stru *pst_wiphy_priv = NULL;

    if ((wdev == NULL) || (wiphy == NULL)) {
        oam_warning_log1(0, OAM_SF_ANY, "{wal_cfgvendor_get_feature_set::wdev is null[%d]!!}", (wdev == NULL));
        return -OAL_EFAUL;
    }

    pst_wiphy_priv = (mac_wiphy_priv_stru *)oal_wiphy_priv(oal_wireless_dev_wiphy(wdev));

    reply = wal_cfgvendor_do_get_feature_set(pst_wiphy_priv);

    skb = oal_cfg80211_vendor_cmd_alloc_reply_skb(wiphy, l_reply_len);
    if (oal_unlikely(!skb)) {
        oam_warning_log1(0, OAM_SF_ANY, "wal_cfgvendor_get_feature_set::skb alloc failed.len %d\r\n", l_reply_len);
        return -OAL_ENOMEM;
    }

    oal_nla_put_nohdr(skb, l_reply_len, &reply);

    l_err = oal_cfg80211_vendor_cmd_reply(skb);
    if (oal_unlikely(l_err)) {
        oam_error_log1(0, OAM_SF_ANY, "wal_cfgvendor_get_feature_set::Vendor Command reply failed ret:%d.\r\n", l_err);
    }

    oam_warning_log1(0, OAM_SF_ANY, "{wal_cfgvendor_get_feature_set::set flag:0x%x.\r\n}", reply);

    return l_err;
}


OAL_STATIC int32_t wal_send_random_mac_oui(oal_net_device_stru *pst_net_dev,
    uint8_t *auc_random_mac_oui, uint8_t auc_random_mac_oui_len)
{
    wal_msg_write_stru st_write_msg;
    int32_t l_ret;

    if (oal_any_null_ptr2(pst_net_dev, auc_random_mac_oui)) {
        oam_error_log0(0, OAM_SF_ANY, "{wal_send_random_mac_oui:: null point argument}");
        return -OAL_EFAIL;
    }

    /***************************************************************************
        抛事件到wal层处理
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_RANDOM_MAC_OUI, auc_random_mac_oui_len);
    if (EOK != memcpy_s(st_write_msg.auc_value, sizeof(st_write_msg.auc_value),
        auc_random_mac_oui, auc_random_mac_oui_len)) {
        oam_error_log0(0, OAM_SF_ANY, "wal_send_random_mac_oui::memcpy fail!");
        return -OAL_EFAIL;
    }

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + auc_random_mac_oui_len,
                               (uint8_t *)&st_write_msg, OAL_FALSE, NULL);
    if (oal_unlikely(l_ret != OAL_SUCC)) {
        oam_warning_log1(0, OAM_SF_ANY, "{wal_send_random_mac_oui::send rand_mac_oui failed, err code %d!}\r\n", l_ret);
        return l_ret;
    }

    return l_ret;
}


OAL_STATIC int32_t wal_cfgvendor_set_random_mac_oui(oal_wiphy_stru *pst_wiphy, oal_wireless_dev_stru *pst_wdev,
                                                    const void *p_data, int32_t l_len)
{
    int32_t l_type;
    int32_t l_ret;
    uint8_t auc_random_mac_oui[WLAN_RANDOM_MAC_OUI_LEN] = { 0 };

    if ((pst_wdev == NULL) || (p_data == NULL) || (!oal_nla_ok((oal_nlattr_stru*)p_data, l_len))) {
        oam_error_log1(0, OAM_SF_ANY, "{wal_cfgvendor_set_random_mac_oui::invalid para l_len=%d.}", l_len);
        return -OAL_EINVAL;
    }

    l_type = oal_nla_type(p_data);
    if (l_type == ANDR_WIFI_ATTRIBUTE_RANDOM_MAC_OUI) {
        /* 随机mac地址前3字节(mac oui)由系统下发,wps pbc场景和hilink关联场景会将此3字节清0 */
        if (EOK != memcpy_s(auc_random_mac_oui, WLAN_RANDOM_MAC_OUI_LEN,
            oal_nla_data(p_data), oal_nla_len(p_data))) {
            oam_error_log0(0, OAM_SF_ANY, "wal_cfgvendor_set_random_mac_oui::memcpy fail!");
            return -OAL_EFAIL;
        }
        oam_warning_log3(0, OAM_SF_ANY, "{wal_cfgvendor_set_random_mac_oui::mac_ou:0x%.2x:%.2x:%.2x}\r\n",
                         auc_random_mac_oui[0], auc_random_mac_oui[1], auc_random_mac_oui[2]);
    } else {
        oam_warning_log1(0, OAM_SF_ANY, "{wal_cfgvendor_set_random_mac_oui:: unknow type:%x}\r\n", l_type);
        return -OAL_EFAIL;
    }

    l_ret = wal_send_random_mac_oui(pst_wdev->netdev, auc_random_mac_oui, WLAN_RANDOM_MAC_OUI_LEN);
    if (oal_unlikely(l_ret != OAL_SUCC)) {
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}


OAL_STATIC void wal_cfgvendor_blacklist_mode(oal_net_device_stru *pst_net_dev, uint8_t uc_mode)
{
    wal_msg_write_stru st_write_msg;
    int32_t ret;
    uint16_t us_len;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    memset_s((uint8_t *)&st_write_msg, sizeof(st_write_msg), 0, sizeof(st_write_msg));

    *(uint8_t *)(st_write_msg.auc_value) = uc_mode;
    us_len = sizeof(uc_mode);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_BLACKLIST_MODE, us_len);

    ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (uint8_t *)&st_write_msg, OAL_FALSE, NULL);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_warning_log1(0, OAM_SF_CFG, "{wal_cfgvendor_blacklist_mode:wal_send_cfg_event return[%d].}\r\n", ret);
    }

    return;
}


OAL_STATIC void wal_cfgvendor_blacklist_add(oal_net_device_stru *pst_net_dev, uint8_t *puc_mac_addr)
{
    wal_msg_write_stru st_write_msg;
    int32_t ret;
    uint16_t us_len;
    mac_blacklist_stru *pst_blklst = NULL;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    memset_s((uint8_t *)&st_write_msg, sizeof(st_write_msg), 0, sizeof(st_write_msg));
    pst_blklst = (mac_blacklist_stru *)(st_write_msg.auc_value);

    memcpy_s(pst_blklst->auc_mac_addr, OAL_MAC_ADDR_LEN, puc_mac_addr, OAL_MAC_ADDR_LEN);
    us_len = sizeof(mac_blacklist_stru);

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_ADD_BLACK_LIST, us_len);

    ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                             (uint8_t *)&st_write_msg, OAL_FALSE, NULL);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_warning_log1(0, OAM_SF_CFG, "{wal_cfgvendor_blacklist_add:wal_send_cfg_event return[%d].}\r\n", ret);
    }

    return;
}


OAL_STATIC uint32_t wal_cfgvendor_blacklist_del(oal_net_device_stru *pst_net_dev,
    uint8_t *puc_mac_addr, uint8_t puc_mac_addr_len)
{
    wal_msg_write_stru st_write_msg;
    int32_t l_ret;
    uint16_t us_len;

    /***************************************************************************
                                抛事件到wal层处理
    ***************************************************************************/
    memset_s((uint8_t *)&st_write_msg, sizeof(st_write_msg), 0, sizeof(st_write_msg));

    if (EOK != memcpy_s(st_write_msg.auc_value, sizeof(st_write_msg.auc_value), puc_mac_addr, puc_mac_addr_len)) {
        oam_error_log0(0, OAM_SF_CFG, "wal_cfgvendor_blacklist_del::memcpy fail!");
        return -OAL_EINVAL;
    }

    us_len = puc_mac_addr_len;

    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_DEL_BLACK_LIST, us_len);

    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + us_len,
                               (uint8_t *)&st_write_msg, OAL_FALSE, NULL);
    if (oal_unlikely(l_ret != OAL_SUCC)) {
        oam_warning_log1(0, OAM_SF_CFG, "{wal_cfgvendor_blacklist_del:wal_send_cfg_event return[%d].}\r\n", l_ret);
        return (uint32_t)l_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC void wal_cfgvendor_flush_bssid_blacklist(
    oal_nlattr_stru *nla, oal_wireless_dev_stru *wdev, int32_t remaining_len)
{
    uint8_t mac_addr[OAL_MAC_ADDR_LEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    oal_bool_enum_uint8 flush = OAL_FALSE;
    int32_t type;
    if (!oal_nla_ok(nla, remaining_len)) {
        oam_error_log1(0, OAM_SF_ANY, "wal_cfgvendor_flush_bssid_blacklist:remaining_len=%d", remaining_len);
        return;
    }
    type = oal_nla_type(nla);
    if (type == GSCAN_ATTRIBUTE_BSSID_BLACKLIST_FLUSH) {
        if (oal_nla_len(nla) < (int)sizeof(uint32_t)) {
            oam_error_log1(0, OAM_SF_ANY, "wal_cfgvendor_flush_bssid_blacklist:nla_len=%d", oal_nla_len(nla));
            return;
        }
        flush = oal_nla_get_u32(nla);
    }
    oam_warning_log2(0, OAM_SF_ANY,
        "{wal_cfgvendor_flush_bssid_blacklist::l_type=%d en_flush=%d}\r\n", type, flush);

    if (flush == OAL_TRUE) {
        /* broadcast address as delete all bssids */
        wal_cfgvendor_blacklist_del(wdev->netdev, mac_addr, OAL_MAC_ADDR_LEN);
    }
}


OAL_STATIC int32_t wal_cfgvendor_set_bssid_blacklist_info(
    oal_nlattr_stru *nla, oal_wireless_dev_stru *wdev,
    wal_wifi_bssid_params *bssid_info, int32_t remaining_len)
{
    int32_t type;
    uint8_t i;

    if (bssid_info->num_bssid > MAX_BLACKLIST_BSSID) { // 防止后续作为循环边界拷贝发生栈溢出以及写越界
        oam_error_log1(0, OAM_SF_ANY, "{wal_cfgvendor_set_bssid_blacklist_info::num_bssid[%d] overrun!}",
            bssid_info->num_bssid);
        return -OAL_EFAIL;
    }
    for (i = 0; i < bssid_info->num_bssid; i++) {
        if (!oal_nla_ok(nla, remaining_len)) {
            oam_error_log1(0, OAM_SF_ANY, "wal_cfgvendor_set_bssid_blacklist_info:remaining_len=%d", remaining_len);
            return -OAL_EFAIL;
        }
        type = oal_nla_type(nla);
        if (type == GSCAN_ATTRIBUTE_BLACKLIST_BSSID) {
            if (memcpy_s(bssid_info->bssids[i], OAL_MAC_ADDR_LEN, oal_nla_data(nla), oal_nla_len(nla)) != EOK) {
                oam_error_log0(0, OAM_SF_ANY, "{wal_cfgvendor_set_bssid_blacklist_info::memcpy fail !}");
                return -OAL_EFAIL;
            }
        }
        remaining_len -= (int32_t)oal_nla_total_size(nla);
        nla = (oal_nlattr_stru *)((uint8_t *)nla + oal_nla_total_size(nla));
    }
    wal_cfgvendor_blacklist_mode(wdev->netdev, CS_BLACKLIST_MODE_BLACK);
    for (i = 0; i < bssid_info->num_bssid; i++) {
        wal_cfgvendor_blacklist_add(wdev->netdev, bssid_info->bssids[i]);
    }

    oam_warning_log3(0, OAM_SF_ANY, "{wal_cfgvendor_set_bssid_blacklist_info::bssid[0]=xx:xx:xx:%02x:%02x:%02x}\r\n",
        bssid_info->bssids[0][3], bssid_info->bssids[0][4], bssid_info->bssids[0][5]); /* 3 4 5 mac地址位 */
    oam_warning_log3(0, OAM_SF_ANY, "{wal_cfgvendor_set_bssid_blacklist_info::bssid[1]=xx:xx:xx:%02x:%02x:%02x}\r\n",
        bssid_info->bssids[1][3], bssid_info->bssids[1][4], bssid_info->bssids[1][5]); /* 3 4 5 mac地址位 */
    oam_warning_log3(0, OAM_SF_ANY, "{wal_cfgvendor_set_bssid_blacklist_info::bssid[2]=xx:xx:xx:%02x:%02x:%02x}\r\n",
        /* 2是二级指针偏移量，3 4 5 mac地址位 */
        bssid_info->bssids[2][3], bssid_info->bssids[2][4], bssid_info->bssids[2][5]);

    return OAL_SUCC;
}


OAL_STATIC int32_t wal_cfgvendor_set_bssid_blacklist(oal_wiphy_stru *wiphy, oal_wireless_dev_stru *wdev,
                                                     const void *data, int32_t len)
{
    int32_t type;
    int32_t ret;
    oal_nlattr_stru *nla = NULL;
    wal_wifi_bssid_params st_bssid_info;
    int32_t remaining_len = len;

    if (oal_any_null_ptr2(wdev, wdev->netdev)) {
        oam_error_log0(0, OAM_SF_ANY, "{wal_cfgvendor_set_bssid_blacklist::wdev or netdev is NULL!}");
        return -OAL_EFAIL;
    }

    if ((data == NULL) || (!oal_nla_ok((oal_nlattr_stru *)data, remaining_len))) {
        oam_error_log1(0, OAM_SF_ANY, "{wal_cfgvendor_set_bssid_blacklist::invalid para l_len=%d.}", len);
        return -OAL_EINVAL;
    }

    memset_s(&st_bssid_info, sizeof(st_bssid_info), 0, sizeof(st_bssid_info));
    nla = (oal_nlattr_stru *)data;
    type = oal_nla_type(nla);
    if (type == GSCAN_ATTRIBUTE_NUM_BSSID) {
        if (oal_nla_len(nla) < (int)sizeof(uint32_t)) {
            oam_error_log1(0, OAM_SF_ANY, "wal_cfgvendor_set_bssid_blacklist:nla_len = %d", oal_nla_len(nla));
            return -OAL_EFAIL;
        }
        st_bssid_info.num_bssid = oal_nla_get_u32(nla);
        oam_warning_log1(0, OAM_SF_ANY,
            "{wal_cfgvendor_set_bssid_blacklist::num_bssid=%d}\r\n", st_bssid_info.num_bssid);
    } else {
        oam_warning_log1(0, OAM_SF_ANY, "{wal_cfgvendor_set_bssid_blacklist:: unknown bssid type:%d}\r\n", type);
        return -OAL_EFAIL;
    }
    remaining_len = len - (int32_t)oal_nla_total_size(nla);
    nla = (oal_nlattr_stru *)((uint8_t *)nla + oal_nla_total_size(nla));

    /* Flush BSSID Blacklist */
    if (!st_bssid_info.num_bssid) {
        wal_cfgvendor_flush_bssid_blacklist(nla, wdev, remaining_len);
    /* Set BSSID Blacklist */
    } else {
        ret = wal_cfgvendor_set_bssid_blacklist_info(nla, wdev, &st_bssid_info, remaining_len);
        if (ret != OAL_SUCC) {
            return ret;
        }
    }

    return OAL_SUCC;
}


OAL_STATIC uint32_t wal_cfgvendor_roam_enable(oal_net_device_stru *pst_net_dev, wal_wifi_roaming_state roam_state)
{
    uint32_t cmd_id;
    uint32_t params[CMD_PARAMS_MAX_CNT] = { 0 };
    uint32_t offset;
    uint32_t ret;

    if (pst_net_dev == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "{wal_cfgvendor_roam_enable:: netdev is NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ret = wal_get_cmd_id("roam_enable", &cmd_id, &offset);
    if (ret != OAL_SUCC) {
        oam_error_log0(0, OAM_SF_ANY, "wal_cfgvendor_roam_enable:find roam_enable cmd is fail");
        return ret;
    }
    params[0] = roam_state;
    return wal_process_cmd_params(pst_net_dev, cmd_id, params);
}


OAL_STATIC int32_t wal_cfgvendor_set_roam_policy(oal_wiphy_stru *pst_wiphy,
    oal_wireless_dev_stru *pst_wdev, const void *p_data, int32_t l_len)
{
    int32_t l_type;
    wal_wifi_roaming_state roam_state = 0;

    if (oal_any_null_ptr2(pst_wdev, pst_wdev->netdev)) {
        oam_error_log0(0, OAM_SF_ANY, "{wal_cfgvendor_set_roam_policy::wdev or netdev is NULL!}");
        return -OAL_EFAIL;
    }

    if ((p_data == NULL) || (!oal_nla_ok((oal_nlattr_stru *)p_data, l_len))) {
        oam_error_log1(0, OAM_SF_ANY,
            "{wal_cfgvendor_set_roam_policy::invalid para l_len=%d.}", l_len);
        return -OAL_EINVAL;
    }

    l_type = oal_nla_type(p_data);
    if (l_type == GSCAN_ATTRIBUTE_ROAM_STATE_SET) {
        if (oal_nla_len(p_data) < (int)sizeof(uint32_t)) {
            oam_error_log1(0, OAM_SF_ANY, "wal_cfgvendor_set_roam_policy:nla_len=%d", l_len);
            return -OAL_EFAIL;
        }
        roam_state = oal_nla_get_u32(p_data);
        oam_warning_log1(0, OAM_SF_ANY, "{wal_cfgvendor_set_roam_policy::roam_state=%d}\r\n", roam_state);
        /* enable or disable roaming */
        if ((roam_state == VENDOR_WIFI_ROAMING_ENABLE) || (roam_state == VENDOR_WIFI_ROAMING_DISABLE)) {
            wal_cfgvendor_roam_enable(pst_wdev->netdev, roam_state);
        } else {
            oam_warning_log1(0, OAM_SF_ANY, "{wal_cfgvendor_set_roam_policy::invalid roam_state=%d}\r\n", roam_state);
        }
    } else {
        oam_warning_log1(0, OAM_SF_ANY, "{wal_cfgvendor_set_roam_policy:: unknown roaming type:%d}\r\n", l_type);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}

/*
 * 函 数 名  : wal_cfgvendor_dbg_get_feature
 * 功能描述  : 获取特性值
 * 1.日    期  : 2017年8月18日
 *   修改内容  : 新生成函数
 */
OAL_STATIC int32_t wal_cfgvendor_dbg_get_feature(oal_wiphy_stru *wiphy, oal_wireless_dev_stru *wdev,
                                                 const void *data, int32_t len)
{
    int32_t l_ret;
    uint32_t supported_features = VENDOR_DBG_MEMORY_DUMP_SUPPORTED;
    oal_netbuf_stru *skb = NULL;
    if (wiphy == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_feature:wiphy is null");
        return -OAL_EINVAL;
    }
    skb = oal_cfg80211_vendor_cmd_alloc_reply_skb(wiphy, sizeof(uint32_t));
    if (oal_unlikely(!skb)) {
        oam_warning_log1(0, OAM_SF_ANY,
            "wal_cfgvendor_dbg_get_feature::skb alloc failed. len %d\r\n", sizeof(uint32_t));
        return -OAL_ENOMEM;
    }

    oal_nla_put_nohdr(skb, sizeof(uint32_t), &supported_features);

    l_ret = oal_cfg80211_vendor_cmd_reply(skb);
    if (oal_unlikely(l_ret)) {
        oam_error_log1(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_feature::Vendor Command reply failed. ret:%d.\r\n", l_ret);
    }
    oam_warning_log1(0, OAM_SF_ANY,
        "wal_cfgvendor_dbg_get_feature::SUCCESS. supported feature:0x%X.\r\n", supported_features);

    return l_ret;
}

/*
 * 函 数 名  : wal_cfgvendor_dbg_get_version
 * 功能描述  : 获取driver版本号
 * 1.日    期  : 2017年8月18日
 *   修改内容  : 新生成函数
 */
OAL_STATIC int32_t wal_cfgvendor_dbg_get_version(oal_wiphy_stru *wiphy,
                                                 oal_wireless_dev_stru *wdev, const void *data, int32_t len)
{
    int8_t auc_driver_version[] = "Hisi Host Driver, version Hi1102 V100";
    int32_t l_ret;
    int32_t l_buf_len = sizeof(auc_driver_version);
    oal_netbuf_stru *skb = NULL;
    if (wiphy == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_version:wiphy is null");
        return -OAL_EINVAL;
    }
    skb = oal_cfg80211_vendor_cmd_alloc_reply_skb(wiphy, l_buf_len);
    if (oal_unlikely(!skb)) {
        oam_warning_log1(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_version::skb alloc failed.len %d\r\n", l_buf_len);
        return -OAL_ENOMEM;
    }

    oal_nla_put_nohdr(skb, l_buf_len, auc_driver_version);

    l_ret = oal_cfg80211_vendor_cmd_reply(skb);
    if (oal_unlikely(l_ret)) {
        oam_error_log1(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_version::Vendor Command reply failed. ret:%d.\r\n", l_ret);
    }

    oam_warning_log1(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_version::SUCCESS. driver version len %d\r\n", l_buf_len);
    return l_ret;
}

/*
 * 函 数 名  : wal_cfgvendor_dbg_get_ring_status
 * 1.日    期  : 2017年8月18日
 *   修改内容  : 新生成函数
 */
int32_t wal_cfgvendor_dbg_get_ring_status(oal_wiphy_stru *wiphy, oal_wireless_dev_stru *wdev,
                                          const void *data, int32_t len)
{
    int32_t l_ret, l_buf_len;
    debug_ring_status_st debug_ring_status = { { VENDOR_FW_EVENT_RING_NAME }, 0, FW_EVENT_RING_ID,
        VENDOR_FW_EVENT_RING_SIZE, 0, 0, 0, 0
    };
    oal_netbuf_stru *skb = NULL;
    l_buf_len = sizeof(debug_ring_status_st);
    if (wiphy == NULL) {
        oam_error_log0(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_ring_status:wiphy is null");
        return -OAL_EINVAL;
    }
    skb = oal_cfg80211_vendor_cmd_alloc_reply_skb(wiphy, l_buf_len + 100);
    if (oal_unlikely(!skb)) {
        oam_warning_log1(0, OAM_SF_ANY,
            "wal_cfgvendor_dbg_get_ring_status::skb alloc failed.len %d\r\n",
            l_buf_len + 100);
        return -OAL_ENOMEM;
    }

    oal_nla_put_u32(skb, DEBUG_ATTRIBUTE_RING_NUM, 1);
    oal_nla_put(skb, DEBUG_ATTRIBUTE_RING_STATUS, l_buf_len, &debug_ring_status);

    l_ret = oal_cfg80211_vendor_cmd_reply(skb);
    if (oal_unlikely(l_ret)) {
        oam_error_log1(0, OAM_SF_ANY,
            "wal_cfgvendor_dbg_get_ring_status::Vendor Command reply failed. ret:%d.\r\n", l_ret);
    }

    oam_warning_log1(0, OAM_SF_ANY,
        "wal_cfgvendor_dbg_get_ring_status::SUCCESS. driver version len %d\r\n", l_buf_len);
    return l_ret;
}

/*
 * 函 数 名  : wal_cfgvendor_dbg_trigger_mem_dump
 * 1.日    期  : 2017年8月18日
 *   修改内容  : 新生成函数
 */
int32_t wal_cfgvendor_dbg_trigger_mem_dump(oal_wiphy_stru *wiphy, oal_wireless_dev_stru *wdev,
                                           const void *data, int32_t len)
{
    oam_warning_log0(0, OAM_SF_ANY, "wal_cfgvendor_dbg_trigger_mem_dump::ENTER");
    return 0;
}

/*
 * 函 数 名  : wal_cfgvendor_dbg_start_logging
 * 1.日    期  : 2017年8月18日
 *   修改内容  : 新生成函数
 */
OAL_STATIC int32_t wal_cfgvendor_dbg_start_logging(oal_wiphy_stru *wiphy, oal_wireless_dev_stru *wdev,
                                                   const void *data, int32_t len)
{
    oam_warning_log0(0, OAM_SF_ANY, "wal_cfgvendor_dbg_start_logging::ENTER");
    return 0;
}

/*
 * 函 数 名  : wal_cfgvendor_dbg_get_ring_data
 * 1.日    期  : 2017年8月18日
 *   修改内容  : 新生成函数
 */
OAL_STATIC int32_t wal_cfgvendor_dbg_get_ring_data(oal_wiphy_stru *wiphy, oal_wireless_dev_stru *wdev,
                                                   const void *data, int32_t len)
{
    oam_warning_log0(0, OAM_SF_ANY, "wal_cfgvendor_dbg_get_ring_data::ENTER");
    return 0;
}


OAL_STATIC int32_t wal_send_vowifi_nat_keep_alive_params(oal_net_device_stru *pst_net_dev,
                                                         uint8_t *pc_keep_alive_info, uint8_t uc_msg_len)
{
    wal_msg_write_stru st_write_msg;
    int32_t l_ret;

    if (oal_any_null_ptr2(pst_net_dev, pc_keep_alive_info)) {
        oam_error_log0(0, OAM_SF_ANY, "{wal_send_vowifi_nat_keep_alive_params:: null point argument.}");
        return -OAL_EFAIL;
    }

    memset_s(&st_write_msg, sizeof(wal_msg_write_stru), 0, sizeof(wal_msg_write_stru));

    /***************************************************************************
        抛事件到wal层处理  WLAN_CFGID_SET_VOWIFI_KEEP_ALIVE
    ***************************************************************************/
    WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_VOWIFI_KEEP_ALIVE, uc_msg_len);
    if (EOK != memcpy_s(st_write_msg.auc_value, sizeof(st_write_msg.auc_value), pc_keep_alive_info, uc_msg_len)) {
        oam_error_log0(0, OAM_SF_ANY, "wal_send_vowifi_nat_keep_alive_params::memcpy fail!");
        return -OAL_EFAIL;
    }

    /* 发送消息 */
    l_ret = wal_send_cfg_event(pst_net_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + uc_msg_len,
                               (uint8_t *)&st_write_msg, OAL_FALSE, NULL);
    if (oal_unlikely(l_ret != OAL_SUCC)) {
        oam_warning_log1(0, OAM_SF_ANY,
            "{wal_send_vowifi_nat_keep_alive_params::send WLAN_CFGID_SET_VOWIFI_KEEP_ALIVE failed, err code %d!}\r\n",
            l_ret);
        return l_ret;
    }

    return l_ret;
}


OAL_STATIC int32_t wl_cfgvendor_start_vowifi_nat_keep_alive(oal_wiphy_stru *pst_wiphy, oal_wireless_dev_stru *pst_wdev,
                                                            const void *p_data, int32_t l_len)
{
    int32_t l_type, l_rem;
    uint32_t period_msec = 0;
    uint8_t uc_keep_alive_id = 0;
    uint16_t ip_pkt_len = 0;
    uint16_t ether_type = ETHER_TYPE_IP;
    oal_bool_enum_uint8 en_find_keepid_flag = OAL_FALSE;
    uint16_t msg_len;
    uint8_t auc_src_mac[WLAN_MAC_ADDR_LEN], auc_dst_mac[WLAN_MAC_ADDR_LEN];
    uint8_t *puc_ip_pkt = NULL;
    const oal_nlattr_stru *pst_iter = NULL;
    mac_vowifi_nat_keep_alive_start_info_stru *pst_start_info = NULL;
    uint8_t *keepalive_extend = NULL;
    int32_t l_ret = EOK;

    if (oal_any_null_ptr2(pst_wdev, p_data) || (l_len <= 0)) {
        oam_error_log1(0, OAM_SF_ANY, "{wl_cfgvendor_start_vowifi_nat_keep_alive::invalid para l_len=%d.}", l_len);
        return -OAL_EINVAL;
    }

    oal_nla_for_each_attr(pst_iter, p_data, l_len, l_rem) {
        l_type = oal_nla_type(pst_iter);
        switch (l_type) {
            case MKEEP_ALIVE_ATTRIBUTE_ID:
                if (oal_nla_len(pst_iter) < (int)sizeof(uint8_t)) {
                    oam_error_log1(0, OAM_SF_ANY, "wl_cfgvendor_start_vowifi_nat_keep_alive:nla_len=%d",
                        oal_nla_len(pst_iter));
                    return -OAL_EINVAL;
                }
                uc_keep_alive_id = oal_nla_get_u8(pst_iter);
                en_find_keepid_flag = OAL_TRUE;
                break;
            case MKEEP_ALIVE_ATTRIBUTE_IP_PKT_LEN:
                if (oal_nla_len(pst_iter) < (int)sizeof(uint16_t)) {
                    oam_error_log1(0, OAM_SF_ANY, "wl_cfgvendor_start_vowifi_nat_keep_alive:nla_len=%d",
                        oal_nla_len(pst_iter));
                    return -OAL_EINVAL;
                }
                ip_pkt_len = oal_nla_get_u16(pst_iter);
                if (WAL_CFGVENDOR_ALIVE_IP_PKT_LEN_INVALID(ip_pkt_len)) {
                    wal_check_free(puc_ip_pkt);

                    oam_warning_log1(0, OAM_SF_ANY,
                        "{wl_cfgvendor_start_vowifi_nat_keep_alive::ip_pkt_len=[%d]is invalid.}", ip_pkt_len);
                    return -OAL_EFAIL;
                }
                break;
            case MKEEP_ALIVE_ATTRIBUTE_IP_PKT:
                wal_check_free(puc_ip_pkt);
                if (oal_value_in_valid_range(ip_pkt_len, 1, MKEEP_ALIVE_IP_PKT_MAXLEN)) {
                    puc_ip_pkt = oal_mem_alloc_m(OAL_MEM_POOL_ID_LOCAL, ip_pkt_len, OAL_TRUE);
                    if (puc_ip_pkt == NULL) {
                        oam_error_log1(0, OAM_SF_ANY,
                            "{wl_cfgvendor_start_vowifi_nat_keep_alive::alloc %d fail.}", ip_pkt_len);
                        return OAL_EFAIL;
                    }
                    l_ret += memcpy_s(puc_ip_pkt, ip_pkt_len, oal_nla_data(pst_iter), oal_nla_len(pst_iter));
                }
                break;
            case MKEEP_ALIVE_ATTRIBUTE_SRC_MAC_ADDR:
                l_ret += memcpy_s(auc_src_mac, WLAN_MAC_ADDR_LEN, oal_nla_data(pst_iter), oal_nla_len(pst_iter));
                break;
            case MKEEP_ALIVE_ATTRIBUTE_DST_MAC_ADDR:
                l_ret += memcpy_s(auc_dst_mac, WLAN_MAC_ADDR_LEN, oal_nla_data(pst_iter), oal_nla_len(pst_iter));
                break;
            case MKEEP_ALIVE_ATTRIBUTE_PERIOD_MSEC:
                if (oal_nla_len(pst_iter) < (int)sizeof(uint32_t)) {
                    oam_error_log1(0, OAM_SF_ANY, "wl_cfgvendor_start_vowifi_nat_keep_alive:nla_len=%d",
                        oal_nla_len(pst_iter));
                    wal_check_free(puc_ip_pkt);
                    return -OAL_EINVAL;
                }
                period_msec = oal_nla_get_u32(pst_iter);
                break;
            case MKEEP_ALIVE_ATTRIBUTE_ETHER_TYPE:
                if (oal_nla_len(pst_iter) < (int)sizeof(uint16_t)) {
                    oam_error_log1(0, OAM_SF_ANY, "wl_cfgvendor_start_vowifi_nat_keep_alive: ether_type nla_len=%d",
                        oal_nla_len(pst_iter));
                    wal_check_free(puc_ip_pkt);
                    return -OAL_EINVAL;
                }
                ether_type = oal_nla_get_u16(pst_iter);
                break;
            default:
                wal_check_free(puc_ip_pkt);
                return -OAL_EFAIL;
        }
    }

    if (period_msec == 0 || en_find_keepid_flag == OAL_FALSE) {
        wal_check_free(puc_ip_pkt);
        return -OAL_EFAIL;
    }
    if (WAL_CFGVENDOR_ALIVE_IP_PKT_IS_VALID(ip_pkt_len, puc_ip_pkt)) {
        // 后面的sizeof(uint16_t)是ip_data内容后面的ether_type字段的值。
        msg_len = ip_pkt_len + sizeof(mac_vowifi_nat_keep_alive_start_info_stru) + sizeof(uint16_t);
        pst_start_info =
            (mac_vowifi_nat_keep_alive_start_info_stru *)oal_mem_alloc_m(OAL_MEM_POOL_ID_LOCAL, msg_len, OAL_TRUE);
        if (pst_start_info == NULL) {
            wal_check_free(puc_ip_pkt);
            return -OAL_EFAIL;
        }

        pst_start_info->st_basic_info.uc_keep_alive_id = uc_keep_alive_id;
        pst_start_info->st_basic_info.en_type = VOWIFI_MKEEP_ALIVE_TYPE_START;
        pst_start_info->us_ip_pkt_len = ip_pkt_len;
        pst_start_info->period_msec = period_msec;

        l_ret += memcpy_s(pst_start_info->auc_ip_pkt_data, ip_pkt_len, puc_ip_pkt, ip_pkt_len);
        l_ret += memcpy_s(pst_start_info->auc_src_mac, WLAN_MAC_ADDR_LEN, auc_src_mac, WLAN_MAC_ADDR_LEN);
        l_ret += memcpy_s(pst_start_info->auc_dst_mac, WLAN_MAC_ADDR_LEN, auc_dst_mac, WLAN_MAC_ADDR_LEN);

        // keepalive的扩展字段放到ip_data的后面
        keepalive_extend = (uint8_t *)(pst_start_info->auc_ip_pkt_data + ip_pkt_len);
        keepalive_extend[0] = (uint8_t)ether_type;
        keepalive_extend[1] = (uint8_t)(ether_type >> NUM_8_BITS);

        oam_warning_log4(0, OAM_SF_ANY,
                         "{wl_cfgvendor_start_vowifi_nat_keep_alive::en_type=[%d],id=[%d],ip_len=[%d],period=[%d].}",
                         pst_start_info->st_basic_info.en_type, pst_start_info->st_basic_info.uc_keep_alive_id,
                         pst_start_info->us_ip_pkt_len, pst_start_info->period_msec);
        wal_send_vowifi_nat_keep_alive_params(pst_wdev->netdev, (uint8_t *)pst_start_info, msg_len);
        oal_mem_free_m(pst_start_info, OAL_TRUE);
    }

    if (l_ret != EOK) {
        wal_check_free(puc_ip_pkt);
        return -OAL_EFAIL;
    }

    wal_check_free(puc_ip_pkt);

    return OAL_SUCC;
}


OAL_STATIC int32_t wl_cfgvendor_stop_vowifi_nat_keep_alive(oal_wiphy_stru *pst_wiphy, oal_wireless_dev_stru *pst_wdev,
                                                           const void *p_data, int32_t l_len)
{
    int32_t l_type;
    int32_t l_ret;
    int32_t l_rem;
    oal_bool_enum_uint8 en_find_keepid_flag = OAL_FALSE;
    const oal_nlattr_stru *pst_iter = NULL;
    mac_vowifi_nat_keep_alive_basic_info_stru st_stop_info;

    if ((pst_wdev == NULL) || (p_data == NULL) || (l_len <= 0)) {
        oam_error_log1(0, OAM_SF_ANY,
            "{wl_cfgvendor_stop_vowifi_nat_keep_alive::invalid para l_len=%d.}", l_len);
        return -OAL_EINVAL;
    }

    memset_s(&st_stop_info, sizeof(mac_vowifi_nat_keep_alive_basic_info_stru),
             0, sizeof(mac_vowifi_nat_keep_alive_basic_info_stru));

    oal_nla_for_each_attr(pst_iter, p_data, l_len, l_rem)
    {
        l_type = oal_nla_type(pst_iter);
        switch (l_type) {
            case MKEEP_ALIVE_ATTRIBUTE_ID:
                if (oal_nla_len(pst_iter) < (int)sizeof(uint8_t)) {
                    oam_error_log1(0, OAM_SF_ANY, "wl_cfgvendor_stop_vowifi_nat_keep_alive:nla_len=%d",
                        oal_nla_len(pst_iter));
                    return -OAL_EFAIL;
                }
                st_stop_info.uc_keep_alive_id = oal_nla_get_u8(pst_iter);
                en_find_keepid_flag = OAL_TRUE;
                break;
            default:
                return -OAL_EFAIL;
        }
    }

    if (en_find_keepid_flag == OAL_TRUE) {
        st_stop_info.en_type = VOWIFI_MKEEP_ALIVE_TYPE_STOP;
#if 1 /* 待正式测试完成后，删除 */
        oam_error_log2(0, OAM_SF_ANY, "{wl_cfgvendor_stop_vowifi_nat_keep_alive::en_type=[%d],id=[%d].}",
                       st_stop_info.en_type, st_stop_info.uc_keep_alive_id);
#endif
        l_ret = wal_send_vowifi_nat_keep_alive_params(pst_wdev->netdev,
            (uint8_t *)&st_stop_info, sizeof(mac_vowifi_nat_keep_alive_basic_info_stru));
        if (oal_unlikely(l_ret != OAL_SUCC)) {
            return -OAL_EFAIL;
        }
    }

    return OAL_SUCC;
}

OAL_STATIC void wal_cfgvendor_lstats_set_iface_stat(wal_wifi_iface_stat_stru *iface_stat, hmac_vap_stru *hmac_vap)
{
    iface_stat->ac[0].tx_mpdu = hmac_vap->station_info.tx_packets;
    iface_stat->ac[0].rx_mpdu = hmac_vap->station_info.rx_packets;
    iface_stat->ac[0].retries = hmac_vap->station_info.tx_retries;
    iface_stat->ac[0].mpdu_lost = hmac_vap->station_info.tx_failed;
}

OAL_STATIC int32_t wal_cfgvendor_lstats_send_event(oal_net_device_stru *net_dev,
    hmac_vap_stru *hmac_vap, uint8_t *p_mac)
{
    wal_msg_write_stru st_write_msg;
    dmac_query_request_event st_dmac_query_request_event;
    dmac_query_station_info_request_event *pst_query_station_info = NULL;

    pst_query_station_info = (dmac_query_station_info_request_event *)&st_dmac_query_request_event;
    pst_query_station_info->query_event = OAL_QUERY_STATION_INFO_EVENT;
    oal_set_mac_addr(pst_query_station_info->auc_query_sta_addr, p_mac);

    /* 填写 msg 消息头 */
    st_write_msg.en_wid = WLAN_CFGID_QUERY_STATION_STATS;
    st_write_msg.us_len = sizeof(dmac_query_station_info_request_event);

    /* 填写 msg 消息体 */
    if (EOK != memcpy_s(st_write_msg.auc_value, sizeof(st_write_msg.auc_value),
                        pst_query_station_info, sizeof(dmac_query_station_info_request_event))) {
        oam_error_log0(0, OAM_SF_ANY, "wal_cfgvendor_lstats_send_event::memcpy fail!");
        return -OAL_EINVAL;
    }

    return wal_send_cfg_event(net_dev, WAL_MSG_TYPE_WRITE,
        WAL_MSG_WRITE_MSG_HDR_LENGTH + sizeof(dmac_query_station_info_request_event),
        (uint8_t *)&st_write_msg, OAL_FALSE, NULL);
}

#define QUERY_STATION_INFO_TIME_INTER (5 * OAL_TIME_HZ)
OAL_STATIC int32_t wal_cfgvendor_lstats_get_event_timeout_valid(hmac_vap_stru *hmac_vap)
{
    signed long i_leftime;

    i_leftime = oal_wait_event_interruptible_timeout_m(hmac_vap->query_wait_q,
        (OAL_TRUE == hmac_vap->station_info_query_completed_flag), QUERY_STATION_INFO_TIME_INTER);
    if (i_leftime <= 0) {
        /* i_leftime为0超时还没有上报扫描结束, i_leftime小于0定时器内部错误 */
        oam_warning_log2(hmac_vap->hal_vap_id, OAM_SF_ANY,
            "{wal_cfgvendor_lstats_get_event_timeout_valid::query info left_time %d, wait for %ld ms error!}",
            i_leftime, ((QUERY_STATION_INFO_TIME_INTER * WAL_HIPRIV_MS_TO_S) / OAL_TIME_HZ));
        return -OAL_EINVAL;
    } else {
        return OAL_SUCC;
    }
}

OAL_STATIC int32_t wal_cfgvendor_lstats_get_station_info(oal_wiphy_stru *wiphy, oal_wireless_dev_stru *wdev,
    wal_wifi_iface_stat_stru *iface_stat)
{
    mac_vap_stru *mac_vap;
    hmac_vap_stru *hmac_vap = NULL;
    int32_t l_ret;
    uint8_t vap_id;
    mac_user_stru *mac_user = NULL;
    uint8_t *p_mac = NULL;

    mac_vap = oal_net_dev_priv(wdev->netdev);
    if (mac_vap == NULL) {
        oam_warning_log0(0, OAM_SF_ANY, "{wal_cfgvendor_lstats_get_station_info::OAL_NET_DEV_PRIV, return null!}");
        return -OAL_EINVAL;
    }

    vap_id = mac_vap->uc_vap_id;
    if (mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA) {
        oam_warning_log0(vap_id, OAM_SF_ANY, "{wal_cfgvendor_lstats_get_station_info:: not sta mode!}");
        return -OAL_EINVAL;
    }

    mac_user = (mac_user_stru *)mac_res_get_mac_user(mac_vap->us_assoc_vap_id);
    if (mac_user == NULL) {
        return -OAL_EINVAL;
    }

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(vap_id);
    if (hmac_vap == NULL) {
        oam_error_log0(vap_id, OAM_SF_ANY, "{wal_cfgvendor_lstats_get_station_info::mac_res_get_hmac_vap fail.}");
        return -OAL_EINVAL;
    }

    p_mac = mac_user->auc_user_mac_addr;

    /* 固定时间内最多更新一次RSSI */
    if (OAL_FALSE == wal_cfg80211_get_station_filter(&hmac_vap->st_vap_base_info, p_mac)) {
        wal_cfgvendor_lstats_set_iface_stat(iface_stat, hmac_vap);
        return OAL_SUCC;
    }

    hmac_vap->station_info_query_completed_flag = OAL_FALSE;

    /********************************************************************************
        抛事件到wal层处理 ，对于低功耗需要做额外处理，不能像下层抛事件，直接起定时器
        低功耗会在接收beacon帧的时候主动上报信息。
    ********************************************************************************/
    l_ret = wal_cfgvendor_lstats_send_event(wdev->netdev, hmac_vap, p_mac);
    if (oal_unlikely(l_ret != OAL_SUCC)) {
        oam_warning_log1(vap_id, OAM_SF_ANY,
            "{wal_cfgvendor_lstats_get_station_info::wal_cfgvendor_lstats_send_event return err code %d!}", l_ret);
        return l_ret;
    }

    l_ret = wal_cfgvendor_lstats_get_event_timeout_valid(hmac_vap);
    if (oal_unlikely(l_ret != OAL_SUCC)) {
        return l_ret;
    }

    /* 正常结束  */
    wal_cfgvendor_lstats_set_iface_stat(iface_stat, hmac_vap);

    return OAL_SUCC;
}


OAL_STATIC int32_t wal_cfgvendor_lstats_get_info(oal_wiphy_stru *pst_wiphy, oal_wireless_dev_stru *pst_wdev,
                                                 const void *p_data, int32_t l_len)
{
    int32_t l_err;
    uint32_t reply_len;
    void *p_out_data = NULL;
    oal_netbuf_stru *pst_skb = NULL;
    wal_wifi_radio_stat_stru *pst_radio_stat = NULL;
    wal_wifi_iface_stat_stru *pst_iface_stat = NULL;

    if (oal_any_null_ptr2(pst_wiphy, pst_wdev)) {
        oam_error_log0(0, OAM_SF_ANY, "{wal_cfgvendor_lstats_get_info:wiphy or wdev or data is null}");
        return -OAL_EFAUL;
    }

    reply_len = sizeof(wal_wifi_radio_stat_stru) + sizeof(wal_wifi_iface_stat_stru);
    p_out_data = (void *)oal_memalloc(reply_len);
    if (p_out_data == NULL) {
        oam_warning_log1(0, OAM_SF_ANY, "{wal_cfgvendor_lstats_get_info:alloc memory fail.[%d]}", reply_len);
        return -OAL_ENOMEM;
    }
    memset_s(p_out_data, reply_len, 0, reply_len);

    /* ??radio ?? */
    pst_radio_stat = (wal_wifi_radio_stat_stru *)p_out_data;
    pst_radio_stat->num_channels = VENDOR_NUM_CHAN;
    pst_radio_stat->on_time = oal_jiffies_to_msecs(OAL_TIME_JIFFY - g_st_wifi_radio_stat.ull_wifi_on_time_stamp);
    pst_radio_stat->tx_time = 0;
    pst_radio_stat->rx_time = 0;

    /* ??interfac ?? */
    pst_iface_stat = (wal_wifi_iface_stat_stru *)(p_out_data + sizeof(*pst_radio_stat));
    pst_iface_stat->num_peers = VENDOR_NUM_PEER;
    pst_iface_stat->peer_info->num_rate = VENDOR_NUM_RATE;
    l_err = wal_cfgvendor_lstats_get_station_info(pst_wiphy, pst_wdev, pst_iface_stat);
    if (l_err != OAL_SUCC) {
        oal_free(p_out_data);
        return -OAL_EFAUL;
    }

    /* ??link ?? */
    pst_skb = oal_cfg80211_vendor_cmd_alloc_reply_skb(pst_wiphy, reply_len);
    if (oal_unlikely(!pst_skb)) {
        oal_free(p_out_data);
        oam_warning_log1(0, OAM_SF_ANY, "{wal_cfgvendor_lstats_get_info::skb alloc failed.len %d}", reply_len);
        return -OAL_ENOMEM;
    }

    oal_nla_put_nohdr(pst_skb, reply_len, p_out_data);

    l_err = oal_cfg80211_vendor_cmd_reply(pst_skb);
    oam_warning_log4(0, OAM_SF_ANY, "{wal_cfgvendor_lstats_get_info::on_time %d, tx_time %d, rx_time %d, err %d",
                     pst_radio_stat->on_time, pst_radio_stat->tx_time, pst_radio_stat->rx_time, l_err);
    oal_free(p_out_data);
    return l_err;
}

#ifdef _PRE_WLAN_FEATURE_APF

OAL_STATIC int32_t wal_cfgvendor_apf_get_capabilities(oal_wiphy_stru *wiphy, oal_wireless_dev_stru *wdev,
                                                      const void *data, int32_t len)
{
    oal_netbuf_stru *skb = NULL;
    int32_t l_ret, l_mem_needed;
    uint32_t max_len;
    l_mem_needed = VENDOR_REPLY_OVERHEAD + (ATTRIBUTE_U32_LEN * 2);

    if (oal_any_null_ptr3(wdev, wdev->netdev, wiphy)) {
        oam_error_log0(0, OAM_SF_ANY, "{wal_cfgvendor_apf_get_capabilities:input params null.}");
        return -OAL_EFAUL;
    }
    skb = oal_cfg80211_vendor_cmd_alloc_reply_skb(wiphy, l_mem_needed);
    if (oal_unlikely(!skb)) {
        oam_warning_log1(0, OAM_SF_ANY, "wal_cfgvendor_apf_get_capabilities::skb alloc failed, len %d", l_mem_needed);
        return -OAL_ENOMEM;
    }

    if (oal_strcmp(oal_netdevice_name(wdev->netdev), "wlan0") == 0) {
        max_len = APF_PROGRAM_MAX_LEN;
    } else {
        max_len = 0;
    }

    oal_nla_put_u32(skb, APF_ATTRIBUTE_VERSION, APF_VERSION);
    oal_nla_put_u32(skb, APF_ATTRIBUTE_MAX_LEN, max_len);

    l_ret = oal_cfg80211_vendor_cmd_reply(skb);
    if (oal_unlikely(l_ret)) {
        oam_error_log1(0, OAM_SF_ANY,
            "wal_cfgvendor_apf_get_capabilities::Vendor Command reply failed, ret:%d.", l_ret);
    }
    return l_ret;
}


OAL_STATIC int32_t wal_cfgvendor_apf_set_filter(oal_wiphy_stru *wiphy, oal_wireless_dev_stru *wdev,
                                                const void *data, int32_t len)
{
    const oal_nlattr_stru *iter = NULL;
    int32_t l_tmp, l_type;
    mac_apf_filter_cmd_stru st_apf_filter_cmd;
    wal_msg_write_stru st_write_msg;
    wal_msg_stru *pst_rsp_msg = NULL;
    uint32_t program_len;

    if (oal_any_null_ptr2(wdev, wdev->netdev) || (data == NULL) || (len <= 0)) {
        oam_error_log0(0, OAM_SF_ANY, "{wal_cfgvendor_apf_set_filter:input params null.}");
        return -OAL_EFAUL;
    }

    st_apf_filter_cmd.us_program_len = 0;

    /* assumption: length attribute must come first */
    oal_nla_for_each_attr(iter, data, len, l_tmp) {
        l_type = oal_nla_type(iter);
        switch (l_type) {
            case APF_ATTRIBUTE_PROGRAM_LEN:
                if (oal_nla_len(iter) < (int)sizeof(uint32_t)) {
                    oam_error_log1(0, OAM_SF_ANY, "wal_cfgvendor_apf_set_filter:nla_len=%d", oal_nla_len(iter));
                    return -OAL_EINVAL;
                }
                program_len = oal_nla_get_u32(iter);
                if (oal_unlikely(!program_len || program_len > APF_PROGRAM_MAX_LEN)) {
                    oam_error_log1(0, OAM_SF_ANY, "wal_cfgvendor_apf_set_filter::program len[%d] invalid", program_len);
                    return -OAL_EINVAL;
                }
                st_apf_filter_cmd.us_program_len = (uint16_t)program_len;
                break;
            case APF_ATTRIBUTE_PROGRAM:
                if (oal_unlikely(!st_apf_filter_cmd.us_program_len)) {
                    oam_error_log0(0, OAM_SF_ANY, "wal_cfgvendor_apf_set_filter::program len not set");
                    return -OAL_EINVAL;
                }
                if (oal_nla_len(iter) < (int)sizeof(uint8_t)) {
                    oam_error_log1(0, OAM_SF_ANY, "wal_cfgvendor_apf_set_filter:nla_len=%d", oal_nla_len(iter));
                    return -OAL_EINVAL;
                }
                st_apf_filter_cmd.puc_program = (uint8_t *)oal_nla_data(iter);
                st_apf_filter_cmd.en_cmd_type = APF_SET_FILTER_CMD;

                /***************************************************************************
                    抛事件到wal层处理
                ***************************************************************************/
                WAL_WRITE_MSG_HDR_INIT(&st_write_msg, WLAN_CFGID_SET_APF_FILTER, sizeof(st_apf_filter_cmd));
                if (EOK != memcpy_s(st_write_msg.auc_value, sizeof(st_write_msg.auc_value),
                                    &st_apf_filter_cmd, sizeof(st_apf_filter_cmd))) {
                    oam_error_log0(0, OAM_SF_ANY, "wal_cfgvendor_apf_set_filter::memcpy fail!");
                    return -OAL_EFAIL;
                }

                /* 发送消息 */
                /* 需要将发送该函数设置为同步，否则hmac处理时会使用已释放的内存 */
                if (OAL_SUCC != wal_send_cfg_event(wdev->netdev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH +
                    sizeof(st_apf_filter_cmd), (uint8_t *)&st_write_msg, OAL_TRUE, &pst_rsp_msg)) {
                    oam_error_log0(0, OAM_SF_ANY, "{wal_cfgvendor_apf_set_filter::wal_send_cfg_event fail!}");
                    return -OAL_EFAIL;
                }
                if (OAL_SUCC != wal_check_and_release_msg_resp(pst_rsp_msg)) {
                    oam_warning_log0(0, OAM_SF_ANY, "{wal_cfgvendor_apf_set_filter::check_and_release_msg_resp fail!}");
                    return -OAL_EFAIL;
                }
                break;
            default:
                oam_error_log1(0, OAM_SF_ANY, "wal_cfgvendor_apf_set_filter::no such attribute %d", l_type);
                return -OAL_EINVAL;
        }
    }

    oam_warning_log1(0, 0, "wal_cfgvendor_apf_set_filter: program len %d", st_apf_filter_cmd.us_program_len);
    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_NAN

OAL_STATIC oal_net_device_stru *wal_cfgvendor_nan_get_attached_netdev(oal_wiphy_stru *wiphy)
{
    oal_net_device_stru *attached_dev;
    mac_wiphy_priv_stru *wiphy_priv = (mac_wiphy_priv_stru *)(oal_wiphy_priv(wiphy));
    /* 转发到wlan */
    attached_dev = wiphy_priv->pst_mac_device->st_p2p_info.pst_primary_net_device;
    if (attached_dev == NULL) {
        oam_warning_log0(0, OAM_SF_ANY, "{wal_cfgvendor_nan_send_event::primary netdev is null}");
        return NULL;
    }
    /* wlan running标记没有置位时不能下发nan事件 */
    if (!(oal_netdevice_flags(attached_dev) & OAL_IFF_RUNNING)) {
        oam_warning_log0(0, OAM_SF_ANY, "{wal_cfgvendor_nan_send_event::wlan0 not running!}");
        return NULL;
    }
    return attached_dev;
}
OAL_STATIC int32_t wal_cfgvendor_nan_send_event(oal_wiphy_stru *wiphy, wal_msg_write_stru *write_msg,
    uint16_t len, oal_bool_enum_uint8 en_need_rsp, wal_msg_stru **ppst_rsp_msg)
{
    int32_t ret;
    oal_net_device_stru *attached_dev = wal_cfgvendor_nan_get_attached_netdev(wiphy);
    if (attached_dev == NULL) {
        oam_warning_log0(0, OAM_SF_ANY, "{wal_cfgvendor_nan_send_event::get attached netdev failed}");
        return -OAL_EFAIL;
    }

    WAL_WRITE_MSG_HDR_INIT(write_msg, WLAN_CFGID_NAN, len);
    ret = wal_send_cfg_event(attached_dev, WAL_MSG_TYPE_WRITE, WAL_MSG_WRITE_MSG_HDR_LENGTH + len,
        (uint8_t*)write_msg, en_need_rsp, ppst_rsp_msg);
    if (ret != OAL_SUCC) {
        oam_error_log1(0, OAM_SF_ANY, "{wal_cfgvendor_nan_send_event::wal_send_cfg_event fail[%d]!}", ret);
    }
    return ret;
}

OAL_STATIC int32_t wal_cfgvendor_nan_check_netdev(oal_net_device_stru *net_dev)
{
    if (net_dev == NULL || oal_strcmp(oal_netdevice_name(net_dev), "nan0")) {
        oam_warning_log0(0, OAM_SF_ANY, "{wal_cfgvendor_nan_check_netdev::Not nan netdev!}");

        return -OAL_EFAIL;
    }
    return OAL_SUCC;
}
OAL_STATIC oal_bool_enum_uint8 wal_check_nan_set_params(const oal_nlattr_stru *iter, int32_t attribute)
{
    switch (attribute) {
        case NAN_ATTRIBUTE_TYPE_CONFIGURE:
        case NAN_ATTRIBUTE_BAND_CONFIGURE:
        case NAN_ATTRIBUTE_CHANNEL_CONFIGURE:
            if (oal_nla_len(iter) < (int)sizeof(uint8_t)) {
                oam_error_log2(0, OAM_SF_ANY, "wal_check_nan_set_params:attribute=%d, nla_len=%d",
                    attribute, oal_nla_len(iter));
                return OAL_FALSE;
            }
            break;
        case NAN_ATTRIBUTE_TRANSACTION_ID:
            if (oal_nla_len(iter) < (int)sizeof(uint16_t)) {
                oam_error_log2(0, OAM_SF_ANY, "wal_check_nan_set_params:attribute=%d, nla_len=%d",
                    attribute, oal_nla_len(iter));
                return OAL_FALSE;
            }
            break;
        case NAN_ATTRIBUTE_DURATION_CONFIGURE:
        case NAN_ATTRIBUTE_PERIOD_CONFIGURE:
            if (oal_nla_len(iter) < (int)sizeof(uint32_t)) {
                oam_error_log2(0, OAM_SF_ANY, "wal_check_nan_set_params:attribute=%d, nla_len=%d",
                    attribute, oal_nla_len(iter));
                return OAL_FALSE;
            }
            break;
        default:
            break;
    }
    return OAL_TRUE;
}

OAL_STATIC int32_t wal_cfgvendor_nan_set_param(oal_wiphy_stru *wiphy,
    oal_wireless_dev_stru *wdev, const void *data, int32_t len)
{
    wal_msg_write_stru write_msg = {0};
    const oal_nlattr_stru *iter = NULL;
    int32_t tmp, attribute;
    mac_nan_cfg_msg_stru *nan_msg = (mac_nan_cfg_msg_stru*)write_msg.auc_value;
    mac_nan_param_stru *param = &nan_msg->param;
    nan_msg->type = NAN_CFG_TYPE_SET_PARAM;
    nan_msg->transaction_id = 0;
    if ((wiphy == NULL) || (wdev == NULL) || (data == NULL) || (len <= 0)) {
        oam_error_log1(0, OAM_SF_ANY, "{wal_cfgvendor_nan_set_param::invalid para len=%d.}", len);
        return -OAL_EINVAL;
    }
    if (wal_cfgvendor_nan_check_netdev(wdev->netdev) != OAL_SUCC) {
        return -OAL_EFAIL;
    }
    oal_nla_for_each_attr(iter, data, len, tmp) {
        attribute = oal_nla_type(iter);
        if (wal_check_nan_set_params(iter, attribute) == OAL_FALSE) {
            return -OAL_EINVAL;
        }
        switch (attribute) {
            case NAN_ATTRIBUTE_TRANSACTION_ID:
                nan_msg->transaction_id = oal_nla_get_u16(iter);
                break;
            case NAN_ATTRIBUTE_DURATION_CONFIGURE:
                param->duration = oal_nla_get_u32(iter);
                break;
            case NAN_ATTRIBUTE_PERIOD_CONFIGURE:
                param->period = oal_nla_get_u32(iter);
                break;
            case NAN_ATTRIBUTE_TYPE_CONFIGURE:
                param->type = oal_nla_get_u8(iter);
                break;
            case NAN_ATTRIBUTE_BAND_CONFIGURE:
                param->band = oal_nla_get_u8(iter);
                break;
            case NAN_ATTRIBUTE_CHANNEL_CONFIGURE:
                param->channel = oal_nla_get_u8(iter);
                break;
            default:
                oam_error_log1(0, OAM_SF_ANY, "wal_cfgvendor_nan_requset::no such attribute %d", attribute);
                return -OAL_EINVAL;
        }
    }
    wal_nan_correction_param(param);
    oam_warning_log4(0, OAM_SF_ANY, "wal_cfgvendor_nan_requset: duration[%d], period[%d], band[%d], channel[%d]",
        param->duration, param->period, param->band, param->channel);

    return wal_cfgvendor_nan_send_event(wiphy, &write_msg, sizeof(mac_nan_cfg_msg_stru), OAL_FALSE, NULL);
}
OAL_STATIC oal_bool_enum_uint8 wal_check_nan_tx_mgmt_params(const oal_nlattr_stru *iter, int32_t attribute)
{
    switch (attribute) {
        case NAN_ATTRIBUTE_TRANSACTION_ID:
        case NAN_ATTRIBUTE_DATALEN:
            if (oal_nla_len(iter) < (int)sizeof(uint16_t)) {
                oam_error_log2(0, OAM_SF_ANY, "wal_check_nan_tx_mgmt_params:attribute=%d, nla_len=%d",
                    attribute, oal_nla_len(iter));
                return OAL_FALSE;
            }
            break;
        case NAN_ATTRIBUTE_DATATYPE:
        case NAN_ATTRIBUTE_DATA_FRAME:
            if (oal_nla_len(iter) < (int)sizeof(uint8_t)) {
                oam_error_log2(0, OAM_SF_ANY, "wal_check_nan_tx_mgmt_params:attribute=%d, nla_len=%d",
                    attribute, oal_nla_len(iter));
                return OAL_FALSE;
            }
            break;
        default:
            break;
    }
    return OAL_TRUE;
}

OAL_STATIC int32_t wal_cfgvendor_nan_tx_mgmt(oal_wiphy_stru *wiphy, oal_wireless_dev_stru *wdev,
    const void *data, int32_t len, uint8_t action)
{
    const oal_nlattr_stru *iter = NULL;
    int32_t tmp;
    int32_t attribute;
    wal_msg_write_stru write_msg = {0};
    wal_msg_stru *pst_rsp_msg = NULL;
    mac_nan_cfg_msg_stru *nan_msg = NULL;
    mac_nan_mgmt_info_stru *mgmt_info = NULL;

    if ((wiphy == NULL) || (wdev == NULL) || (data == NULL) || (len <= 0)) {
        oam_error_log1(0, OAM_SF_ANY, "{wal_cfgvendor_nan_tx_mgmt::invalid para l_len=%d.}", len);
        return -OAL_EINVAL;
    }
    if (wal_cfgvendor_nan_check_netdev(wdev->netdev) != OAL_SUCC) {
        return -OAL_EFAIL;
    }
    nan_msg = (mac_nan_cfg_msg_stru*)write_msg.auc_value;
    nan_msg->type = NAN_CFG_TYPE_SET_TX_MGMT;
    mgmt_info = &nan_msg->mgmt_info;

    mgmt_info->action = action;
    mgmt_info->len = 0;
    mgmt_info->data = NULL;
    oal_nla_for_each_attr(iter, data, len, tmp) {
        attribute = oal_nla_type(iter);
        if (wal_check_nan_tx_mgmt_params(iter, attribute) == OAL_FALSE) {
            return -OAL_EINVAL;
        }
        switch (attribute) {
            case NAN_ATTRIBUTE_TRANSACTION_ID:
                nan_msg->transaction_id = oal_nla_get_u16(iter);
                break;
            case NAN_ATTRIBUTE_DATATYPE:
                mgmt_info->periodic = oal_nla_get_u8(iter);
                break;
            case NAN_ATTRIBUTE_DATALEN:
                mgmt_info->len = oal_nla_get_u16(iter);
                break;
            case NAN_ATTRIBUTE_DATA_FRAME:
                mgmt_info->data = (uint8_t*)oal_nla_data(iter);
                break;
            default:
                oam_error_log1(0, OAM_SF_ANY, "wal_cfgvendor_nan_tx_mgmt::no such attribute %d", attribute);
                return -OAL_EINVAL;
        }
    }
    if (mgmt_info->len > 400) { /* nan帧最大允许长度400字节 */
        oam_warning_log1(0, OAM_SF_ANY, "wal_cfgvendor_nan_tx_mgmt::frame len[%d] too large", mgmt_info->len);
        return -OAL_EINVAL;
    }
    return wal_cfgvendor_nan_send_event(wiphy, &write_msg, sizeof(mac_nan_cfg_msg_stru), OAL_TRUE, &pst_rsp_msg);
}

OAL_STATIC int32_t wal_cfgvendor_nan_tx_publish(oal_wiphy_stru *wiphy, oal_wireless_dev_stru *wdev,
    const void *data, int32_t len)
{
    return wal_cfgvendor_nan_tx_mgmt(wiphy, wdev, data, len, WLAN_ACTION_NAN_PUBLISH);
}

OAL_STATIC int32_t wal_cfgvendor_nan_tx_followup(oal_wiphy_stru *wiphy, oal_wireless_dev_stru *wdev,
    const void *data, int32_t len)
{
    return wal_cfgvendor_nan_tx_mgmt(wiphy, wdev, data, len, WLAN_ACTION_NAN_FLLOWUP);
}

OAL_STATIC int32_t wal_cfgvendor_nan_tx_subscribe(oal_wiphy_stru *wiphy, oal_wireless_dev_stru *wdev,
    const void *data, int32_t len)
{
    return wal_cfgvendor_nan_tx_mgmt(wiphy, wdev, data, len, WLAN_ACTION_NAN_SUBSCRIBE);
}

OAL_STATIC int32_t wal_cfgvendor_nan_cancel_tx(oal_wiphy_stru *wiphy, oal_wireless_dev_stru *wdev,
    const void *data, int32_t len)
{
    /* 暂不适配 */
    return OAL_SUCC;
}


OAL_STATIC int32_t wal_cfgvendor_nan_enable(oal_wiphy_stru *wiphy, oal_wireless_dev_stru *wdev,
    const void *data, int32_t len)
{
    oal_net_device_stru *attached_dev = NULL;
    oal_netbuf_stru *skb = NULL;
    mac_vap_stru *mac_vap = NULL;
    int32_t mem_needed, tmp, attribute;
    oal_gfp_enum_uint8 kflags = oal_in_atomic() ? GFP_ATOMIC : GFP_KERNEL;
    const oal_nlattr_stru *iter = NULL;
    uint16_t rsp_data[4]; /* 4个short, 2字节transaction id，6字节mac地址 */
    if ((wiphy == NULL) || (wdev == NULL) || (data == NULL) || (len <= 0)) {
        oam_error_log1(0, OAM_SF_ANY, "{wal_cfgvendor_nan_enable::invalid para l_len=%d.}", len);
        return -OAL_EINVAL;
    }
    if (wal_cfgvendor_nan_check_netdev(wdev->netdev) != OAL_SUCC) {
        return -OAL_EFAIL;
    }
    oal_nla_for_each_attr(iter, data, len, tmp) {
        attribute = oal_nla_type(iter);
        switch (attribute) {
            case NAN_ATTRIBUTE_TRANSACTION_ID:
                if (oal_nla_len(iter) < (int)sizeof(uint16_t)) {
                    oam_error_log1(0, OAM_SF_ANY, "wal_cfgvendor_nan_enable:nla_len=%d", oal_nla_len(iter));
                    return -OAL_EINVAL;
                }
                rsp_data[0] = oal_nla_get_u16(iter);
                break;
            default:
                oam_warning_log1(0, OAM_SF_ANY, "wal_cfgvendor_nan_enable::no such attribute %d", attribute);
                return -OAL_EINVAL;
        }
    }

    attached_dev = wal_cfgvendor_nan_get_attached_netdev(wiphy);
    if (attached_dev == NULL) {
        return -OAL_EFAIL;
    }

    mem_needed = VENDOR_DATA_OVERHEAD + oal_nlmsg_length(sizeof(rsp_data));
    skb = oal_cfg80211_vendor_event_alloc(wdev->wiphy, wdev, mem_needed, NAN_EVENT_RX, kflags);
    if (oal_unlikely(skb == NULL)) {
        oam_error_log1(0, OAM_SF_ANY, "wal_cfgvendor_nan_enable::skb alloc failed, len %d", mem_needed);
        return -OAL_EINVAL;
    }
    mac_vap = oal_net_dev_priv(attached_dev);
    if (mac_vap == NULL) {
        oam_warning_log0(0, OAM_SF_ANY, "{wal_cfgvendor_nan_send_event::vap of wlan0 is null}");
        return -OAL_EFAIL;
    }
    oal_set_mac_addr((uint8_t*)&rsp_data[1], mac_mib_get_StationID(mac_vap)); /* 2-7字节放wlan0 mac地址 */
    oal_nla_put(skb, NAN_ATTRIBUTE_ENABLE_RSP, sizeof(rsp_data), (uint8_t*)rsp_data);
    oal_cfg80211_vendor_event(skb, kflags);

    return OAL_SUCC;
}

OAL_STATIC int32_t wal_cfgvendor_nan_disable(oal_wiphy_stru *wiphy, oal_wireless_dev_stru *wdev,
    const void *data, int32_t len)
{
    return OAL_SUCC;
}
#endif
#ifdef _PRE_WLAN_FEATURE_FTM

OAL_STATIC uint32_t wal_cfgvendor_rtt_paras_check(mac_ftm_wifi_rtt_config *wifi_rtt_config)
{
    int32_t channel_num;

    if (mac_addr_is_zero(wifi_rtt_config->addr)) {
        oam_warning_log0(0, OAM_SF_FTM, "{wal_cfgvendor_rtt_paras_check::bssid should not be all zero!}");
        return OAL_FAIL;
    }
    if (wifi_rtt_config->type != RTT_TYPE_2_SIDED) {
        oam_warning_log1(0, OAM_SF_FTM, "wal_cfgvendor_rtt_paras_check::type[%d] not 2-sided!", wifi_rtt_config->type);
        return OAL_FAIL;
    }
    if (wifi_rtt_config->burst_period > RTT_CONFIG_MAX_BURST_PERIOD) {
        oam_warning_log1(0, OAM_SF_FTM, "wal_cfgvendor_rtt_paras_check::burst_period[%d] must be in(0-31)!",
            wifi_rtt_config->burst_period);
        return OAL_FAIL;
    }
    if (wifi_rtt_config->num_burst > RTT_CONFIG_MAX_BURST_NUM) {
        oam_warning_log1(0, OAM_SF_FTM, "wal_cfgvendor_rtt_paras_check::burst num[%u] over 16",
            wifi_rtt_config->num_burst);
        return OAL_FAIL;
    }
    if (wifi_rtt_config->num_frames_per_burst > RTT_CONFIG_MAX_FTMS_PER_BURST) {
        oam_warning_log1(0, OAM_SF_FTM, "{wal_cfgvendor_rtt_paras_check::ftms_per_burst[%u] over 10!}",
            wifi_rtt_config->num_frames_per_burst);
        return OAL_FAIL;
    }
    if ((wifi_rtt_config->lci_request != 0 && wifi_rtt_config->lci_request != 1) ||
        (wifi_rtt_config->lcr_request != 0 && wifi_rtt_config->lcr_request != 1)) {
        oam_warning_log2(0, OAM_SF_FTM, "wal_cfgvendor_rtt_paras_check::lci_request[%u] lcr_request[%u] not \
            valid, should be 1 or 0", wifi_rtt_config->lci_request, wifi_rtt_config->lcr_request);
        return OAL_FAIL;
    }
    if (wifi_rtt_config->burst_duration > RTT_CONFIG_MAX_BURST_DURATION) {
        oam_warning_log1(0, OAM_SF_FTM, "wal_cfgvendor_rtt_paras_check::burst_duration[%d] must be (2-11) or 15!",
            wifi_rtt_config->burst_period);
        return OAL_FAIL;
    }
    if ((wifi_rtt_config->bw < WIFI_RTT_BW_20) || (wifi_rtt_config->bw > WIFI_RTT_BW_80)) {
        oam_warning_log1(0, OAM_SF_FTM, "wal_cfgvendor_rtt_paras_check::bw[%u] not supported!", wifi_rtt_config->bw);
        return OAL_FAIL;
    }
    channel_num = (uint8_t)oal_ieee80211_frequency_to_channel(wifi_rtt_config->channel.center_freq);
    if (mac_is_channel_num_valid(mac_get_band_by_channel_num(channel_num), channel_num,
        (uint8_t)(wifi_rtt_config->channel.center_freq >= 5955)) != OAL_SUCC) { /* 5955是6G的起始中心频点 */
        oam_warning_log1(0, OAM_SF_FTM, "wal_cfgvendor_rtt_paras_check::channel num[%u] invalid!", channel_num);
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


OAL_STATIC void wal_cfgvendor_rtt_params_print(mac_ftm_wifi_rtt_config *wifi_rtt_config)
{
    oam_warning_log4(0, OAM_SF_FTM, "wal_cfgvendor_rtt_params_print:bssid[%X:XX:XX:%X:%X:%X]",
        wifi_rtt_config->addr[MAC_ADDR_0], wifi_rtt_config->addr[MAC_ADDR_3], wifi_rtt_config->addr[MAC_ADDR_4],
        wifi_rtt_config->addr[MAC_ADDR_5]);
    oam_warning_log4(0, OAM_SF_FTM, "wal_cfgvendor_rtt_params_print:: channel info:width:%d center_freq:%d \
        center_freq0:%d center_freq1:%d", wifi_rtt_config->channel.width, wifi_rtt_config->channel.center_freq,
        wifi_rtt_config->channel.center_freq0, wifi_rtt_config->channel.center_freq1);
    oam_warning_log4(0, OAM_SF_FTM, "wal_cfgvendor_rtt_params_print::burst_period:%u burst_duration:%u num_burst:%u \
        num_frames_per_burst:%u ", wifi_rtt_config->burst_period, wifi_rtt_config->burst_duration,
        wifi_rtt_config->num_burst, wifi_rtt_config->num_frames_per_burst);
    oam_warning_log4(0, OAM_SF_FTM, "wal_cfgvendor_rtt_params_print::lci_request:%u lcr_request:%u preamble:%u bw:%u",
        wifi_rtt_config->lci_request, wifi_rtt_config->lcr_request, wifi_rtt_config->preamble, wifi_rtt_config->bw);
}


OAL_STATIC uint32_t wal_cfgvendor_rtt_config_get_mac_and_channel_info(const oal_nlattr_stru *iter,
    int32_t attribute, mac_ftm_wifi_rtt_config *wifi_rtt_config)
{
    if (attribute == RTT_ATTRIBUTE_TARGET_MAC) {
        if (memcpy_s(wifi_rtt_config->addr, WLAN_MAC_ADDR_LEN, oal_nla_data(iter), oal_nla_len(iter)) != EOK) {
            oam_error_log0(0, OAM_SF_FTM, "wal_cfgvendor_rtt_config_get_mac_and_channel_info::mac addr memcpy fail!");
            return OAL_FAIL;
        }
    }
    if (attribute == RTT_ATTRIBUTE_TARGET_CHAN) {
        if (memcpy_s(&wifi_rtt_config->channel, sizeof(mac_ftm_wifi_channel_info), oal_nla_data(iter),
            oal_nla_len(iter)) != EOK) {
            oam_error_log0(0, OAM_SF_FTM, "wal_cfgvendor_rtt_config_get_mac_and_channel_info::wifi channel info \
                memcpy fail!");
            return OAL_FAIL;
        }
    }

    return OAL_SUCC;
}
OAL_STATIC oal_bool_enum_uint8 wal_check_rtt_config_attributes_params(const oal_nlattr_stru *iter, int32_t attribute)
{
    switch (attribute) {
        case RTT_ATTRIBUTE_TARGET_TYPE:
        case RTT_ATTRIBUTE_TARGET_PEER:
        case RTT_ATTRIBUTE_TARGET_PERIOD:
        case RTT_ATTRIBUTE_TARGET_NUM_BURST:
        case RTT_ATTRIBUTE_TARGET_NUM_FTM_BURST:
        case RTT_ATTRIBUTE_TARGET_BURST_DURATION:
        case RTT_ATTRIBUTE_TARGET_PREAMBLE:
        case RTT_ATTRIBUTE_TARGET_BW:
            if (oal_nla_len(iter) < (int)sizeof(uint32_t)) {
                oam_error_log1(0, OAM_SF_ANY, "wal_check_rtt_config_attributes_params:nla_len=%d",
                    oal_nla_len(iter));
                return OAL_FALSE;
            }
            break;
        case RTT_ATTRIBUTE_TARGET_LCI:
        case RTT_ATTRIBUTE_TARGET_LCR:
            if (oal_nla_len(iter) < (int)sizeof(uint8_t)) {
                oam_error_log1(0, OAM_SF_ANY, "wal_check_rtt_config_attributes_params:nla_len=%d", oal_nla_len(iter));
                return OAL_FALSE;
            }
            break;
        default:
            break;
    }
    return OAL_TRUE;
}

OAL_STATIC void wal_cfgvendor_get_rtt_config_attributes(const oal_nlattr_stru *iter, int32_t attribute,
    mac_ftm_wifi_rtt_config *wifi_rtt_config)
{
    if (wal_check_rtt_config_attributes_params(iter, attribute) == OAL_FALSE) {
        return;
    }
    switch (attribute) {
        case RTT_ATTRIBUTE_TARGET_MAC:
        case RTT_ATTRIBUTE_TARGET_CHAN:
            if (wal_cfgvendor_rtt_config_get_mac_and_channel_info(iter, attribute, wifi_rtt_config) != OAL_SUCC) {
                oam_error_log0(0, OAM_SF_FTM, "wal_cfgvendor_get_rtt_config_attributes::get mac addr or channel fail!");
                return;
            }
            break;
        case RTT_ATTRIBUTE_TARGET_TYPE:
            wifi_rtt_config->type = oal_nla_get_u32(iter);
            break;
        case RTT_ATTRIBUTE_TARGET_PEER:
            wifi_rtt_config->peer = oal_nla_get_u32(iter);
            break;
        case RTT_ATTRIBUTE_TARGET_PERIOD:
            wifi_rtt_config->burst_period = oal_nla_get_u32(iter);
            break;
        case RTT_ATTRIBUTE_TARGET_NUM_BURST:
            wifi_rtt_config->num_burst = oal_nla_get_u32(iter);
            break;
        case RTT_ATTRIBUTE_TARGET_NUM_FTM_BURST:
            wifi_rtt_config->num_frames_per_burst = oal_nla_get_u32(iter);
            break;
        case RTT_ATTRIBUTE_TARGET_LCI:
            wifi_rtt_config->lci_request = oal_nla_get_u8(iter);
            break;
        case RTT_ATTRIBUTE_TARGET_LCR:
            wifi_rtt_config->lcr_request = oal_nla_get_u8(iter);
            break;
        case RTT_ATTRIBUTE_TARGET_BURST_DURATION:
            wifi_rtt_config->burst_duration = oal_nla_get_u32(iter);
            break;
        case RTT_ATTRIBUTE_TARGET_PREAMBLE:
            wifi_rtt_config->preamble = oal_nla_get_u32(iter);
            break;
        case RTT_ATTRIBUTE_TARGET_BW:
            wifi_rtt_config->bw = oal_nla_get_u32(iter);
            break;
        default:
            oam_warning_log1(0, OAM_SF_FTM, "wal_cfgvendor_get_rtt_config_attributes::no such attribute %u", attribute);
            break;
    }
}


OAL_STATIC int32_t wal_cfgvendor_rtt_config_get_target_info(mac_ftm_wifi_rtt_config_params *rtt_param,
    const oal_nlattr_stru *iter)
{
    mac_ftm_wifi_rtt_config *rtt_target = NULL;
    const oal_nlattr_stru *iter1 = NULL;
    const oal_nlattr_stru *iter2 = NULL;
    int32_t tmp1 = 0;
    int32_t tmp2 = 0;
    int32_t attribute;

    rtt_target = rtt_param->wifi_rtt_config;
    oal_nla_for_each_nested(iter1, iter, tmp1) {
        if ((uint8_t *)rtt_target >= ((uint8_t *)rtt_param->wifi_rtt_config +
            (rtt_param->rtt_target_cnt * sizeof(mac_ftm_wifi_rtt_config)))) {
            oam_error_log0(0, 0, "wal_cfgvendor_rtt_config_get_target_info::rtt_target over its max size");
            return -OAL_EFAIL;
        }
        oal_nla_for_each_nested(iter2, iter1, tmp2) {
            if (iter2 == NULL) {
                oam_error_log0(0, 0, "wal_cfgvendor_rtt_config_get_target_info::iter2 is NULL");
                return -OAL_EFAIL;
            }
            attribute = oal_nla_type(iter2);
            wal_cfgvendor_get_rtt_config_attributes(iter2, attribute, rtt_target);
        }
        wal_cfgvendor_rtt_params_print(rtt_target);
        if (wal_cfgvendor_rtt_paras_check(rtt_target) != OAL_SUCC) {
            oam_warning_log0(0, OAM_SF_FTM, "wal_cfgvendor_rtt_config_get_target_info::wifi rtt para check fail!");
            return -OAL_EFAIL;
        }
        rtt_target++;
    }

    return OAL_SUCC;
}


OAL_STATIC int32_t wal_cfgvendor_rtt_config_get_attr(const void *data, int32_t len,
    mac_ftm_wifi_rtt_config_params *rtt_param)
{
    const oal_nlattr_stru *iter = NULL;
    int32_t tmp;
    int32_t attribute;

    oal_nla_for_each_attr(iter, data, len, tmp) {
        attribute = oal_nla_type(iter);
        switch (attribute) {
            case RTT_ATTRIBUTE_TARGET_CNT:
                if (oal_nla_len(iter) < (int)sizeof(uint8_t)) {
                    oam_error_log1(0, OAM_SF_ANY, "wal_cfgvendor_rtt_config_get_attr:nla_len=%d", oal_nla_len(iter));
                    return -OAL_EFAIL;
                }
                rtt_param->rtt_target_cnt = (uint8_t)oal_nla_get_u8(iter);
                /* 当前仅支持1个rtt target */
                if (rtt_param->rtt_target_cnt != 1) {
                    return -OAL_EFAIL;
                }
                break;
            case RTT_ATTRIBUTE_TARGET_INFO:
                if (wal_cfgvendor_rtt_config_get_target_info(rtt_param, iter) != OAL_SUCC) {
                    return -OAL_EFAIL;
                }
                break;
            default:
                oam_warning_log1(0, OAM_SF_FTM, "wal_cfgvendor_rtt_config_get_attr::no such attribute %u", attribute);
                break;
        }
    }

    return OAL_SUCC;
}


OAL_STATIC int32_t wal_cfgvendor_rtt_set_config(oal_wiphy_stru *wiphy, oal_wireless_dev_stru *wdev,
    const void *data, int32_t len)
{
    wal_msg_write_stru write_msg = {0};
    mac_ftm_wifi_rtt_config_params rtt_param;
    int32_t ret;
    if ((wdev == NULL) || (data == NULL) || (len <= 0)) {
        oam_error_log1(0, OAM_SF_ANY, "{wal_cfgvendor_rtt_set_config::invalid para l_len=%d.}", len);
        return -OAL_EINVAL;
    }

    memset_s(&rtt_param, sizeof(mac_ftm_wifi_rtt_config_params), 0, sizeof(mac_ftm_wifi_rtt_config_params));
    rtt_param.wifi_rtt_config = (mac_ftm_wifi_rtt_config *)write_msg.auc_value;

    ret = wal_cfgvendor_rtt_config_get_attr(data, len, &rtt_param);
    if (ret != OAL_SUCC) {
        oam_warning_log0(0, 0, "wal_cfgvendor_rtt_set_config::get attributes fail!");
        return -OAL_EFAIL;
    }

    if ((rtt_param.rtt_target_cnt * sizeof(mac_ftm_wifi_rtt_config)) > WAL_MSG_WRITE_MAX_LEN) {
        oam_warning_log2(0, OAM_SF_FTM, "wal_cfgvendor_rtt_set_config::rtt config paras size[%d], over write_msg max \
            size %d", (rtt_param.rtt_target_cnt * sizeof(mac_ftm_wifi_rtt_config)), WAL_MSG_WRITE_MAX_LEN);
        return -OAL_EFAIL;
    }
    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_RTT_CONFIG,
        (rtt_param.rtt_target_cnt * sizeof(mac_ftm_wifi_rtt_config)));
    ret = wal_send_cfg_event(wdev->netdev, WAL_MSG_TYPE_WRITE, (WAL_MSG_WRITE_MSG_HDR_LENGTH +
        rtt_param.rtt_target_cnt * sizeof(mac_ftm_wifi_rtt_config)), (uint8_t*)&write_msg, OAL_FALSE, NULL);
    if (ret != OAL_SUCC) {
        oam_error_log1(0, OAM_SF_FTM, "{wal_cfgvendor_rtt_set_config::wal_send_cfg_event fail[%d]!}", ret);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_CHBA_MGMT
OAL_STATIC oal_bool_enum_uint8 wal_check_chba_conn_notify_params(const oal_nlattr_stru *iter, int32_t attribute)
{
    switch (attribute) {
        case MAC_CHBA_CONN_NTF_ATTR_CENTER_FREQ_20M:
        case MAC_CHBA_CONN_NTF_ATTR_CENTER_FREQ_1:
        case MAC_CHBA_CONN_NTF_ATTR_CENTER_FREQ_2:
            if (oal_nla_len(iter) < (int)sizeof(uint32_t)) {
                oam_error_log2(0, OAM_SF_ANY, "wal_check_chba_conn_notify_params:attribute=%d, nla_len=%d",
                    attribute, oal_nla_len(iter));
                return OAL_FALSE;
            }
            break;
        case MAC_CHBA_CONN_NTF_ATTR_ID:
        case MAC_CHBA_CONN_NTF_ATTR_CENTER_BANDWIDTH:
            if (oal_nla_len(iter) < (int)sizeof(uint8_t)) {
                oam_error_log2(0, OAM_SF_ANY, "wal_check_chba_conn_notify_params:attribute=%d, nla_len=%d",
                    attribute, oal_nla_len(iter));
                return OAL_FALSE;
            }
            break;
        case MAC_CHBA_CONN_NTF_ATTR_EXPIRE_TIME:
            if (oal_nla_len(iter) < (int)sizeof(uint8_t)) {
                oam_error_log1(0, OAM_SF_ANY, "wal_check_chba_conn_notify_params:nla_len=%d", oal_nla_len(iter));
                return OAL_FALSE;
            }
            break;
        default:
            break;
    }
    return OAL_TRUE;
}
OAL_STATIC int32_t wal_cfgvendor_fill_chba_conn_notify_msg(wal_msg_write_stru *msg, const void *data, int32_t len)
{
    hmac_chba_conn_notify_stru *ntf_info = (hmac_chba_conn_notify_stru*)msg->auc_value;
    const oal_nlattr_stru *iter = NULL;
    int32_t tmp;
    int32_t attribute;
    oal_nla_for_each_attr(iter, data, len, tmp) {
        attribute = oal_nla_type(iter);
        if (wal_check_chba_conn_notify_params(iter, attribute) == OAL_FALSE) {
            return -OAL_EFAIL;
        }
        switch (attribute) {
            case MAC_CHBA_CONN_NTF_ATTR_ID:
                ntf_info->id = oal_nla_get_u8(iter);
                break;
            case MAC_CHBA_CONN_NTF_ATTR_MAC_ADDR:
                if (memcpy_s(ntf_info->peer_mac_addr, WLAN_MAC_ADDR_LEN, oal_nla_data(iter),
                    oal_nla_len(iter)) != EOK) {
                    oam_error_log0(0, 0, "wal_cfgvendor_chba_conn_notify::mac addr memcpy fail!");
                    return -OAL_EFAIL;
                }
                break;
            case MAC_CHBA_CONN_NTF_ATTR_CENTER_FREQ_20M:
                ntf_info->channel.center_freq_20m = oal_nla_get_u32(iter);
                break;
            case MAC_CHBA_CONN_NTF_ATTR_CENTER_FREQ_1:
                ntf_info->channel.center_freq1 = oal_nla_get_u32(iter);
                break;
            case MAC_CHBA_CONN_NTF_ATTR_CENTER_FREQ_2:
                ntf_info->channel.center_freq2 = oal_nla_get_u32(iter);
                break;
            case MAC_CHBA_CONN_NTF_ATTR_CENTER_BANDWIDTH:
                ntf_info->channel.bandwidth = oal_nla_get_u8(iter);
                break;
            case MAC_CHBA_CONN_NTF_ATTR_EXPIRE_TIME:
                ntf_info->expire_time = oal_nla_get_u16(iter);
                break;
            default:
                oam_error_log1(0, OAM_SF_ANY, "wal_cfgvendor_chba_conn_notify::no such attribute %d", attribute);
                return -OAL_EINVAL;
        }
    }
    oam_warning_log2(0, 0, "CHBA: rcv conn notify. len %d, ntf id %d", len, ntf_info->id);
    return OAL_SUCC;
}
OAL_STATIC int32_t wal_cfgvendor_chba_conn_notify(oal_wiphy_stru *wiphy,
    oal_wireless_dev_stru *wdev, const void *data, int32_t len)

{
    wal_msg_write_stru write_msg = {0};
    int32_t ret;
    if ((wdev == NULL) || (data == NULL) || (len <= 0)) {
        oam_error_log1(0, OAM_SF_ANY, "{wal_cfgvendor_chba_conn_notify::invalid para l_len=%d.}", len);
        return -OAL_EINVAL;
    }
    if (wal_cfgvendor_fill_chba_conn_notify_msg(&write_msg, data, len) != OAL_SUCC) {
        return -OAL_EINVAL;
    }
    WAL_WRITE_MSG_HDR_INIT(&write_msg, WLAN_CFGID_CHBA_CONN_NOTIFY, sizeof(hmac_chba_conn_notify_stru));
    ret = wal_send_cfg_event(wdev->netdev, WAL_MSG_TYPE_WRITE,
        WAL_MSG_WRITE_MSG_HDR_LENGTH + sizeof(hmac_chba_conn_notify_stru), (uint8_t*)&write_msg, OAL_FALSE, NULL);
    if (ret != OAL_SUCC) {
        oam_error_log1(0, OAM_SF_ANY, "{wal_cfgvendor_chba_conn_notify::wal_send_cfg_event fail[%d]!}", ret);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}
#endif

OAL_STATIC const oal_wiphy_vendor_command_stru g_wal_vendor_cmds[] = {
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = GSCAN_SUBCMD_GET_CHANNEL_LIST
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_get_channel_list,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = wal_andr_wifi_attr_policy,
        .maxattr = ANDR_WIFI_ATTRIBUTE_MAX,
#endif
    },

    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = ANDR_WIFI_SET_COUNTRY
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_set_country,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = wal_andr_wifi_attr_policy,
        .maxattr = ANDR_WIFI_ATTRIBUTE_MAX,
#endif
    },
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = ANDR_WIFI_SUBCMD_GET_FEATURE_SET
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_get_feature_set,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = VENDOR_CMD_RAW_DATA,
#endif
    },
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = ANDR_WIFI_RANDOM_MAC_OUI
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_set_random_mac_oui,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = wal_gscan_attr_policy,
        .maxattr = GSCAN_ATTRIBUTE_MAX - 1,
#endif
    },
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = WIFI_SUBCMD_SET_BSSID_BLACKLIST
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_set_bssid_blacklist,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = wal_gscan_attr_policy,
        .maxattr = GSCAN_ATTRIBUTE_MAX - 1,
#endif
    },
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = WIFI_SUBCMD_FW_ROAM_POLICY
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_set_roam_policy,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = wal_gscan_attr_policy,
        .maxattr = GSCAN_ATTRIBUTE_MAX - 1,
#endif
    },
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = DEBUG_GET_FEATURE
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_dbg_get_feature,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = VENDOR_CMD_RAW_DATA,
#endif
    },
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = DEBUG_GET_VER
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_dbg_get_version,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = VENDOR_CMD_RAW_DATA,
#endif
    },
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = DEBUG_GET_RING_STATUS
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_dbg_get_ring_status,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = VENDOR_CMD_RAW_DATA,
#endif
    },
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = DEBUG_TRIGGER_MEM_DUMP
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_dbg_trigger_mem_dump,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = VENDOR_CMD_RAW_DATA,
#endif
    },
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = DEBUG_START_LOGGING
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_dbg_start_logging,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = VENDOR_CMD_RAW_DATA,
#endif
    },
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = DEBUG_GET_RING_DATA
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_dbg_get_ring_data,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = VENDOR_CMD_RAW_DATA,
#endif
    },
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = WIFI_OFFLOAD_SUBCMD_START_MKEEP_ALIVE
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wl_cfgvendor_start_vowifi_nat_keep_alive,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = wal_mkeep_alive_attr_policy,
        .maxattr = MKEEP_ALIVE_ATTRIBUTE_MAX,
#endif
    },
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = WIFI_OFFLOAD_SUBCMD_STOP_MKEEP_ALIVE
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wl_cfgvendor_stop_vowifi_nat_keep_alive,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = wal_mkeep_alive_attr_policy,
        .maxattr = MKEEP_ALIVE_ATTRIBUTE_MAX,
#endif
    },
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = LSTATS_SUBCMD_GET_INFO
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_lstats_get_info,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = VENDOR_CMD_RAW_DATA,
#endif
    },
#ifdef _PRE_WLAN_FEATURE_APF
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = APF_SUBCMD_GET_CAPABILITIES
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_apf_get_capabilities,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = wal_apf_attr_policy,
        .maxattr = APF_ATTRIBUTE_MAX,
#endif
    },

    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = APF_SUBCMD_SET_FILTER
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_apf_set_filter,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = wal_apf_attr_policy,
        .maxattr = APF_ATTRIBUTE_MAX,
#endif
    },
#endif

#ifdef _PRE_WLAN_FEATURE_NAN
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = WIFI_SUBCMD_NAN_SET_PARAM
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_nan_set_param,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = wal_nan_attr_policy,
        .maxattr = NAN_ATTRIBUTE_MAX,
#endif
    },
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = WIFI_SUBCMD_NAN_ENABLE
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_nan_enable,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = wal_nan_attr_policy,
        .maxattr = NAN_ATTRIBUTE_MAX,
#endif
    },
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = WIFI_SUBCMD_NAN_DISABLE
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_nan_disable,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = wal_nan_attr_policy,
        .maxattr = NAN_ATTRIBUTE_MAX,
#endif
    },
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = WIFI_SUBCMD_NAN_PUBLISH_REQUEST
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_nan_tx_publish,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = wal_nan_attr_policy,
        .maxattr = NAN_ATTRIBUTE_MAX,
#endif
    },
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = WIFI_SUBCMD_NAN_CANCEL_PUBLISH_REQUEST
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_nan_cancel_tx,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = wal_nan_attr_policy,
        .maxattr = NAN_ATTRIBUTE_MAX,
#endif
    },
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = WIFI_SUBCMD_NAN_SUBSCRIBE_REQUEST
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_nan_tx_subscribe,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = wal_nan_attr_policy,
        .maxattr = NAN_ATTRIBUTE_MAX,
#endif
    },
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = WIFI_SUBCMD_NAN_SUBSCRIBE_CANCEL_REQUEST
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_nan_cancel_tx,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = wal_nan_attr_policy,
        .maxattr = NAN_ATTRIBUTE_MAX,
#endif
    },
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = WIFI_SUBCMD_NAN_FOLLOWUP_REQUEST
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_nan_tx_followup,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = wal_nan_attr_policy,
        .maxattr = NAN_ATTRIBUTE_MAX,
#endif
    },
#endif
#ifdef _PRE_WLAN_FEATURE_FTM
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = RTT_SUBCMD_SET_CONFIG
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_rtt_set_config,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = wal_rtt_attr_policy,
        .maxattr = RTT_ATTRIBUTE_MAX,
#endif
    },
#endif
#ifdef _PRE_WLAN_CHBA_MGMT
    {   {   .vendor_id = OUI_VENDOR,
            .subcmd = WIFI_SUBCMD_HID2D_CONN_NOTIFY
        },
        .flags = WIPHY_VENDOR_CMD_NEED_WDEV | WIPHY_VENDOR_CMD_NEED_NETDEV,
        .doit = wal_cfgvendor_chba_conn_notify,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
        .policy = wal_chba_conn_ntf_attr_policy,
        .maxattr = MAC_CHBA_CONN_NTF_ATTR_MAX,
#endif
    },
#endif

};

OAL_STATIC const oal_nl80211_vendor_cmd_info_stru g_wal_vendor_events[] = {
    { OUI_HISI, HISI_VENDOR_EVENT_UNSPEC },
    { OUI_HISI, HISI_VENDOR_EVENT_PRIV_STR },
    { OUI_VENDOR, WIFI_SUBCMD_CHBA_READY_TO_CONN },
    [NAN_EVENT_RX] = { OUI_HISI, NAN_EVENT_RX },
    [VENDOR_RTT_COMPLETE_EVENT] = { OUI_HISI, VENDOR_RTT_COMPLETE_EVENT },
    [VENDOR_FT_CONNECT_FAIL_EVENT] = { OUI_HISI, VENDOR_FT_CONNECT_FAIL_EVENT },
};

void wal_cfgvendor_init(oal_wiphy_stru *wiphy)
{
    wiphy->vendor_commands = g_wal_vendor_cmds;
    wiphy->n_vendor_commands = oal_array_size(g_wal_vendor_cmds);
    wiphy->vendor_events = g_wal_vendor_events;
    wiphy->n_vendor_events = oal_array_size(g_wal_vendor_events);
}

void wal_cfgvendor_deinit(oal_wiphy_stru *wiphy)
{
    wiphy->vendor_commands = NULL;
    wiphy->vendor_events = NULL;
    wiphy->n_vendor_commands = 0;
    wiphy->n_vendor_events = 0;
}

