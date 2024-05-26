

#include "oam_stat_wifi.h"
#include "oam_ext_if.h"

oam_stat_info_stru g_oam_stat_info;

uint32_t oam_stats_report_info_to_sdt(oam_ota_type_enum_uint8 en_ota_type)
{
    int32_t ret;
    uint16_t us_skb_len, us_stat_info_len;
    oal_netbuf_stru *pst_netbuf = NULL;
    oam_ota_stru *pst_ota_data = NULL;

    if (oal_unlikely(g_oam_sdt_func_hook.p_sdt_report_data_func == NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (en_ota_type == OAM_OTA_TYPE_DEV_STAT_INFO) {
        us_stat_info_len = sizeof(oam_device_stat_info_stru) * WLAN_DEVICE_MAX_NUM_PER_CHIP;
    } else if (en_ota_type == OAM_OTA_TYPE_VAP_STAT_INFO) {
        us_stat_info_len = (uint16_t)(sizeof(oam_vap_stat_info_stru) * WLAN_VAP_SUPPORT_MAX_NUM_LIMIT);
    } else {
        oal_io_print("oam_stats_report_info_to_sdt::ota_type invalid-->%d!\n", en_ota_type);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* 为上报统计信息申请空间,头部预留8字节，尾部预留1字节，给sdt_drv用 */
    us_skb_len = us_stat_info_len + sizeof(oam_ota_hdr_stru);
    if (us_skb_len > WLAN_SDT_NETBUF_MAX_PAYLOAD) {
        us_skb_len = WLAN_SDT_NETBUF_MAX_PAYLOAD;
        us_stat_info_len = WLAN_SDT_NETBUF_MAX_PAYLOAD - sizeof(oam_ota_hdr_stru);
    }

    pst_netbuf = oam_alloc_data2sdt(us_skb_len);
    if (pst_netbuf == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_ota_data = (oam_ota_stru *)oal_netbuf_data(pst_netbuf);

    /* 填写ota消息头结构体 */
    pst_ota_data->st_ota_hdr.ul_tick = (uint32_t)oal_time_get_stamp_ms();
    pst_ota_data->st_ota_hdr.en_ota_type = en_ota_type;
    pst_ota_data->st_ota_hdr.uc_frame_hdr_len = 0;
    pst_ota_data->st_ota_hdr.us_ota_data_len = us_stat_info_len;

    /* 复制数据,填写ota数据 */
    if (en_ota_type == OAM_OTA_TYPE_DEV_STAT_INFO) {
        ret = memcpy_s((void *)pst_ota_data->auc_ota_data, (uint32_t)pst_ota_data->st_ota_hdr.us_ota_data_len,
            (const void *)g_oam_stat_info.ast_dev_stat_info, (uint32_t)us_stat_info_len);
    } else {
        ret = memcpy_s((void *)pst_ota_data->auc_ota_data, (uint32_t)pst_ota_data->st_ota_hdr.us_ota_data_len,
            (const void *)g_oam_stat_info.ast_vap_stat_info, (uint32_t)us_stat_info_len);
    }
    if (ret != EOK) {
        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
        oal_io_print("oam_stats_report_info_to_sdt:: memcpy_s failed\r\n");
        return OAL_FAIL;
    }

    /* 下发至sdt接收队列，若队列满则串口输出 */
    return oam_report_data2sdt(pst_netbuf, OAM_DATA_TYPE_OTA, OAM_PRIMID_TYPE_OUTPUT_CONTENT);
}

uint32_t oam_stats_clear_vap_stat_info(uint8_t uc_vap_id)
{
    if (uc_vap_id >= WLAN_VAP_SUPPORT_MAX_NUM_LIMIT) {
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    memset_s(&g_oam_stat_info.ast_vap_stat_info[uc_vap_id], sizeof(oam_vap_stat_info_stru),
             0, sizeof(oam_vap_stat_info_stru));

    return OAL_SUCC;
}

void oam_stats_clear_stat_info(void)
{
    memset_s(&g_oam_stat_info, sizeof(oam_stat_info_stru), 0, sizeof(oam_stat_info_stru));
}

uint32_t oam_stats_clear_user_stat_info(uint16_t us_usr_id)
{
    if (us_usr_id >= WLAN_USER_MAX_USER_LIMIT) {
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
    memset_s(&g_oam_stat_info.ast_user_stat_info[us_usr_id], sizeof(oam_user_stat_info_stru),
             0, sizeof(oam_user_stat_info_stru));
    return OAL_SUCC;
}

uint32_t oam_stats_report_usr_info(uint16_t us_usr_id)
{
    uint32_t ul_tick;
    uint16_t us_skb_len; /* skb总长度 */
    oal_netbuf_stru *pst_netbuf = NULL;
    oam_ota_stru *pst_ota_data = NULL;
    uint32_t ul_ret;
    uint16_t us_stat_info_len;

    if (oal_unlikely(g_oam_sdt_func_hook.p_sdt_report_data_func == NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (us_usr_id >= WLAN_USER_MAX_USER_LIMIT) {
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
    us_stat_info_len = sizeof(oam_user_stat_info_stru);

    /* 为上报统计信息申请空间,头部预留8字节，尾部预留1字节，给sdt_drv用 */
    us_skb_len = us_stat_info_len + sizeof(oam_ota_hdr_stru);
    if (us_skb_len > WLAN_SDT_NETBUF_MAX_PAYLOAD) {
        us_skb_len = WLAN_SDT_NETBUF_MAX_PAYLOAD;
        us_stat_info_len = WLAN_SDT_NETBUF_MAX_PAYLOAD - sizeof(oam_ota_hdr_stru);
    }

    pst_netbuf = oam_alloc_data2sdt(us_skb_len);
    if (pst_netbuf == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_ota_data = (oam_ota_stru *)oal_netbuf_data(pst_netbuf);

    /* 获取系统TICK值 */
    ul_tick = (uint32_t)oal_time_get_stamp_ms();

    /* 填写ota消息头结构体 */
    pst_ota_data->st_ota_hdr.ul_tick = ul_tick;
    pst_ota_data->st_ota_hdr.en_ota_type = OAM_OTA_TYPE_USER_STAT_INFO;
    pst_ota_data->st_ota_hdr.uc_frame_hdr_len = 0;
    pst_ota_data->st_ota_hdr.us_ota_data_len = us_stat_info_len;

    if (memcpy_s((void *)pst_ota_data->auc_ota_data, (uint32_t)pst_ota_data->st_ota_hdr.us_ota_data_len,
        (const void *)&g_oam_stat_info.ast_user_stat_info[us_usr_id], (uint32_t)us_stat_info_len) != EOK) {
        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
        oal_io_print("oam_stats_report_usr_info:: memcpy_s failed\r\n");
        return OAL_FAIL;
    }

    /* 下发至sdt接收队列，若队列满则串口输出 */
    ul_ret = oam_report_data2sdt(pst_netbuf, OAM_DATA_TYPE_OTA, OAM_PRIMID_TYPE_OUTPUT_CONTENT);
    return ul_ret;
}