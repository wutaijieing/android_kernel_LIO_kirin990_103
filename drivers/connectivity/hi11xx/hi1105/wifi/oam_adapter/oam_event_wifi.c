

#include "oam_event_wifi.h"
#include "oam_event.h"
#include "wlan_types.h"
#include "oam_main.h"

uint8_t g_bcast_addr[WLAN_MAC_ADDR_LEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

OAL_STATIC void oam_hide_mac_addr(uint8_t *puc_mac_hdr, uint8_t uc_beacon_hdr_len)
{
    if (puc_mac_hdr == NULL || uc_beacon_hdr_len < WLAN_MGMT_FRAME_HEADER_LEN) {
        return;
    }
    /* addr1 */
    puc_mac_hdr[5] = 0xff; /* 第1个地址的存放位置在数组的第5个 */
    puc_mac_hdr[6] = 0xff; /* 第1个地址的存放位置在数组的第6个 */
    puc_mac_hdr[7] = 0xff; /* 第1个地址的存放位置在数组的第7个 */

    /* addr2 */
    puc_mac_hdr[11] = 0xff; /* 第2个地址的存放位置在数组的第11个 */
    puc_mac_hdr[12] = 0xff; /* 第2个地址的存放位置在数组的第12个 */
    puc_mac_hdr[13] = 0xff; /* 第2个地址的存放位置在数组的第13个 */

    /* addr3 */
    puc_mac_hdr[17] = 0xff; /* 第3个地址的存放位置在数组的第17个 */
    puc_mac_hdr[18] = 0xff; /* 第3个地址的存放位置在数组的第18个 */
    puc_mac_hdr[19] = 0xff; /* 第3个地址的存放位置在数组的第19个 */
}

OAL_STATIC uint32_t oam_report_80211_frame_to_sdt(uint8_t *puc_user_macaddr,
                                                  uint8_t *puc_mac_hdr_addr,
                                                  uint8_t uc_mac_hdr_len,
                                                  uint8_t *puc_mac_body_addr,
                                                  uint16_t us_mac_frame_len,
                                                  oam_ota_frame_direction_type_enum_uint8 en_frame_direction)
{
    uint32_t ul_tick;
    uint16_t us_skb_len;
    oal_netbuf_stru *pst_netbuf = NULL;
    oam_ota_stru *pst_ota_data = NULL;
    uint32_t ul_ret;

    if (oal_unlikely(g_oam_sdt_func_hook.p_sdt_report_data_func == NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 为上报80211帧申请空间 */
    us_skb_len = us_mac_frame_len + sizeof(oam_ota_hdr_stru);
    if (us_skb_len > WLAN_SDT_NETBUF_MAX_PAYLOAD) {
        us_skb_len = WLAN_SDT_NETBUF_MAX_PAYLOAD;
        us_mac_frame_len = WLAN_SDT_NETBUF_MAX_PAYLOAD - sizeof(oam_ota_hdr_stru);
    }

    pst_netbuf = oam_alloc_data2sdt(us_skb_len);
    if (pst_netbuf == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_ota_data = (oam_ota_stru *)oal_netbuf_header(pst_netbuf);

    /* 获取系统TICK值 */
    ul_tick = (uint32_t)oal_time_get_stamp_ms();

    /* 填写ota消息头结构体 */
    pst_ota_data->st_ota_hdr.ul_tick = ul_tick;
    pst_ota_data->st_ota_hdr.en_ota_type = OAM_OTA_TYPE_80211_FRAME;
    pst_ota_data->st_ota_hdr.uc_frame_hdr_len = uc_mac_hdr_len;
    pst_ota_data->st_ota_hdr.us_ota_data_len = us_mac_frame_len;
    pst_ota_data->st_ota_hdr.en_frame_direction = en_frame_direction;
    oal_set_mac_addr(pst_ota_data->st_ota_hdr.auc_user_macaddr, puc_user_macaddr);
    oam_set_ota_board_type(pst_ota_data);

    /* 复制帧头 */
    ul_ret = (uint32_t)memcpy_s((void *)pst_ota_data->auc_ota_data,
        (uint32_t)us_mac_frame_len, (const void *)puc_mac_hdr_addr, (uint32_t)uc_mac_hdr_len);
    /* 复制帧体 */
    ul_ret += (uint32_t)memcpy_s((void *)(pst_ota_data->auc_ota_data + uc_mac_hdr_len),
        (uint32_t)(us_mac_frame_len - uc_mac_hdr_len), (const void *)puc_mac_body_addr,
        (uint32_t)(us_mac_frame_len - uc_mac_hdr_len));
    if (ul_ret != EOK) {
        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
        oal_io_print("oam_report_80211_frame_to_sdt:: memcpy_s failed\r\n");
        return OAL_FAIL;
    }
    oam_hide_mac_addr(pst_ota_data->auc_ota_data, uc_mac_hdr_len);

    /* 判断sdt发送消息队列是否已满，若满输出至串口 */
    ul_ret = oam_report_data2sdt(pst_netbuf, OAM_DATA_TYPE_OTA, OAM_PRIMID_TYPE_OUTPUT_CONTENT);
    return ul_ret;
}

OAL_STATIC uint32_t oam_report_80211_frame_to_console(uint8_t *puc_mac_hdr_addr,
                                                      uint8_t uc_mac_hdr_len,
                                                      uint8_t *puc_mac_body_addr,
                                                      uint16_t us_mac_frame_len,
                                                      oam_ota_frame_direction_type_enum_uint8 en_frame_direction)
{
    uint16_t us_80211_frame_body_len;

    if (en_frame_direction == OAM_OTA_FRAME_DIRECTION_TYPE_TX) {
        oal_io_print("oam_report_80211_frame_to_console::tx_80211_frame header:\n");
    } else {
        oal_io_print("oam_report_80211_frame_to_console::rx_80211_frame header:\n");
    }

    oam_dump_buff_by_hex(puc_mac_hdr_addr, uc_mac_hdr_len, OAM_PRINT_CRLF_NUM);

    if (uc_mac_hdr_len > us_mac_frame_len) {
        oal_io_print("oam_report_80211_frame_to_console::rx_80211_frame invalid frame\n");
        return OAL_FAIL;
    }

    us_80211_frame_body_len = us_mac_frame_len - uc_mac_hdr_len;
    oal_io_print("oam_report_80211_frame_to_console::80211_frame body:\n");
    oam_dump_buff_by_hex(puc_mac_body_addr, us_80211_frame_body_len, OAM_PRINT_CRLF_NUM);
    return OAL_SUCC;
}

uint32_t oam_report_80211_frame(uint8_t *puc_user_macaddr,
                                uint8_t *puc_mac_hdr_addr,
                                uint8_t uc_mac_hdr_len,
                                uint8_t *puc_mac_body_addr,
                                uint16_t us_mac_frame_len,
                                oam_ota_frame_direction_type_enum_uint8 en_frame_direction)
{
    uint32_t ul_ret = OAL_SUCC;
    uint32_t ul_oam_ret = OAL_SUCC;
    uint32_t ul_return_addr = 0;

    /* Less IO_print, less oam_report_80211_frame_to_console, use oam_report_80211_frame_to_sdt for log.
       Suggested by gongxiangling & wangzhenzhong */
#if (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)
    ul_return_addr = __return_address();
#endif

    if (oal_unlikely(oal_any_null_ptr3(puc_mac_hdr_addr, puc_mac_body_addr, puc_user_macaddr))) {
        oam_error_log4(0, OAM_SF_ANY, "{oam_report_80211_frame:[device] puc_mac_hdr_addr = 0x%X, "
            "puc_mac_body_addr = 0x%X, puc_user_macaddr = 0x%X, __return_address = 0x%X}",
            (uintptr_t)puc_mac_hdr_addr, (uintptr_t)puc_mac_body_addr, (uintptr_t)puc_user_macaddr, ul_return_addr);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 检查帧头长度合法性 */
    if ((uc_mac_hdr_len > WLAN_MAX_FRAME_HEADER_LEN) || (uc_mac_hdr_len < WLAN_MIN_FRAME_HEADER_LEN)) {
        oam_warning_log4(0, OAM_SF_ANY, "{oam_report_80211_frame:HEAD CHECK! HDR_LEN_INVALID!!hearder_len = %d, "
            "frame_len = %d, en_frame_direction = %d, return_addres = 0x%X}",
            uc_mac_hdr_len, us_mac_frame_len, en_frame_direction, ul_return_addr);
    }

    /* 检查mac帧总长度合法性 */
    if (uc_mac_hdr_len > us_mac_frame_len) {
        oam_report_dft_params(BROADCAST_MACADDR, puc_mac_hdr_addr, uc_mac_hdr_len, OAM_OTA_TYPE_80211_FRAME);
        oam_warning_log4(0, OAM_SF_ANY,
                         "{oam_report_80211_frame:HEAD/FRAME CHECK! hearder_len = %d, frame_len = %d, "
                         "en_frame_direction = %d, return_addres = 0x%X}",
                         uc_mac_hdr_len, us_mac_frame_len, en_frame_direction, ul_return_addr);
        return OAL_ERR_CODE_OAM_EVT_FR_LEN_INVALID;
    }

    us_mac_frame_len = (us_mac_frame_len > WLAN_MAX_FRAME_LEN) ? WLAN_MAX_FRAME_LEN : us_mac_frame_len;
    if (oal_unlikely(en_frame_direction >= OAM_OTA_FRAME_DIRECTION_TYPE_BUTT)) {
        return OAL_ERR_CODE_OAM_EVT_FRAME_DIR_INVALID;
    }

    switch (g_oam_mng_ctx.en_output_type) {
        /* 输出至控制台 */
        case OAM_OUTPUT_TYPE_CONSOLE:
            ul_ret = oam_report_80211_frame_to_console(puc_mac_hdr_addr, uc_mac_hdr_len,
                puc_mac_body_addr, us_mac_frame_len, en_frame_direction);
            break;
        /* 输出至SDT工具 */
        case OAM_OUTPUT_TYPE_SDT:
            ul_oam_ret = oam_report_80211_frame_to_sdt(puc_user_macaddr, puc_mac_hdr_addr,
                uc_mac_hdr_len, puc_mac_body_addr, us_mac_frame_len, en_frame_direction);
            break;
        default:
            ul_oam_ret = OAL_ERR_CODE_INVALID_CONFIG;
            break;
    }

    if ((ul_oam_ret != OAL_SUCC) || (ul_ret != OAL_SUCC)) {
        oam_warning_log4(0, OAM_SF_ANY, "{oam_report_80211_frame:[device] en_frame_direction = %d, ul_ret = %d, "
        "ul_oam_ret = %d, return_addres = 0x%X}", en_frame_direction, ul_ret, ul_oam_ret, ul_return_addr);
    }

    return ((ul_ret != OAL_SUCC) ? (ul_ret) : (ul_oam_ret));
}

OAL_STATIC uint32_t oam_report_beacon_to_sdt(uint8_t *puc_beacon_hdr_addr,
                                             uint8_t uc_beacon_hdr_len,
                                             uint8_t *puc_beacon_body_addr,
                                             uint16_t us_beacon_len,
                                             oam_ota_frame_direction_type_enum_uint8 en_beacon_direction)
{
    uint32_t ul_tick;
    uint16_t us_skb_len;
    oal_netbuf_stru *pst_netbuf = NULL;
    oam_ota_stru *pst_ota_data = NULL;
    int32_t ret;

    if (oal_unlikely(g_oam_sdt_func_hook.p_sdt_report_data_func == NULL)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 为上报beacon帧申请空间 */
    us_skb_len = us_beacon_len + sizeof(oam_ota_hdr_stru);
    if (us_skb_len > WLAN_SDT_NETBUF_MAX_PAYLOAD) {
        us_skb_len = WLAN_SDT_NETBUF_MAX_PAYLOAD;
        us_beacon_len = WLAN_SDT_NETBUF_MAX_PAYLOAD - sizeof(oam_ota_hdr_stru);
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
    pst_ota_data->st_ota_hdr.en_ota_type = OAM_OTA_TYPE_BEACON;
    pst_ota_data->st_ota_hdr.uc_frame_hdr_len = OAM_BEACON_HDR_LEN;
    pst_ota_data->st_ota_hdr.us_ota_data_len = us_beacon_len;
    pst_ota_data->st_ota_hdr.en_frame_direction = en_beacon_direction;
    oam_set_ota_board_type(pst_ota_data);

    /* 复制数据,填写ota数据 */
    ret = memcpy_s((void *)pst_ota_data->auc_ota_data, (uint32_t)us_beacon_len,
        (const void *)puc_beacon_hdr_addr, (uint32_t)uc_beacon_hdr_len);

    ret += memcpy_s((void *)(pst_ota_data->auc_ota_data + uc_beacon_hdr_len),
        (uint32_t)(us_beacon_len - uc_beacon_hdr_len), (const void *)puc_beacon_body_addr,
        (uint32_t)(us_beacon_len - uc_beacon_hdr_len));
    if (ret != EOK) {
        oal_mem_sdt_netbuf_free(pst_netbuf, OAL_TRUE);
        oal_io_print("oam_report_beacon_to_sdt:: memcpy_s failed\r\n");
        return OAL_FAIL;
    }

    oam_hide_mac_addr(pst_ota_data->auc_ota_data, uc_beacon_hdr_len);

    /* 下发至sdt接收队列，若队列满则串口输出 */
    return oam_report_data2sdt(pst_netbuf, OAM_DATA_TYPE_OTA, OAM_PRIMID_TYPE_OUTPUT_CONTENT);
}

OAL_STATIC uint32_t oam_report_beacon_to_console(uint8_t *puc_beacon_hdr_addr,
                                                 uint16_t us_beacon_len,
                                                 oam_ota_frame_direction_type_enum_uint8 en_beacon_direction)
{
    uint8_t *puc_beacon_body_addr = NULL;
    uint16_t us_beacon_body_len;

    if (en_beacon_direction == OAM_OTA_FRAME_DIRECTION_TYPE_TX) {
        oal_io_print("oam_report_beacon_to_console::tx_beacon info:\n");
    } else {
        oal_io_print("oam_report_beacon_to_console::rx_beacon info:\n");
    }
    oal_io_print("oam_report_beacon_to_console::beacon_header:\n");
    oam_dump_buff_by_hex(puc_beacon_hdr_addr, OAM_BEACON_HDR_LEN, OAM_PRINT_CRLF_NUM);
    puc_beacon_body_addr = puc_beacon_hdr_addr + OAM_BEACON_HDR_LEN;
    us_beacon_body_len = us_beacon_len - OAM_BEACON_HDR_LEN;

    oal_io_print("oam_report_beacon_to_console::beacon_body:\n");
    oam_dump_buff_by_hex(puc_beacon_body_addr, us_beacon_body_len, OAM_PRINT_CRLF_NUM);

    return OAL_SUCC;
}

uint32_t oam_report_beacon(uint8_t *puc_beacon_hdr_addr,
                           uint8_t uc_beacon_hdr_len,
                           uint8_t *puc_beacon_body_addr,
                           uint16_t us_beacon_len,
                           oam_ota_frame_direction_type_enum_uint8 en_beacon_direction)
{
    uint32_t ul_ret;

    if (en_beacon_direction >= OAM_OTA_FRAME_DIRECTION_TYPE_BUTT) {
        return OAL_ERR_CODE_OAM_EVT_FRAME_DIR_INVALID;
    }

    if ((oam_ota_get_switch(OAM_OTA_SWITCH_BEACON) != OAM_SDT_PRINT_BEACON_RXDSCR_TYPE_BEACON) &&
        (oam_ota_get_switch(OAM_OTA_SWITCH_BEACON) != OAM_SDT_PRINT_BEACON_RXDSCR_TYPE_BOTH)) {
        return OAL_SUCC;
    }

    if (oal_any_null_ptr2(puc_beacon_hdr_addr, puc_beacon_body_addr)) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if ((us_beacon_len > WLAN_MAX_FRAME_LEN) ||
        (us_beacon_len <= WLAN_MGMT_FRAME_HEADER_LEN)) {
        oam_dump_buff_by_hex(puc_beacon_hdr_addr, us_beacon_len, OAM_PRINT_CRLF_NUM);
        return OAL_ERR_CODE_OAM_EVT_FR_LEN_INVALID;
    }

    switch (g_oam_mng_ctx.en_output_type) {
        /* 输出至控制台 */
        case OAM_OUTPUT_TYPE_CONSOLE:
            ul_ret = oam_report_beacon_to_console(puc_beacon_hdr_addr,
                                                  us_beacon_len,
                                                  en_beacon_direction);
            break;

        /* 输出至SDT工具 */
        case OAM_OUTPUT_TYPE_SDT:
            ul_ret = oam_report_beacon_to_sdt(puc_beacon_hdr_addr,
                                              uc_beacon_hdr_len,
                                              puc_beacon_body_addr,
                                              us_beacon_len,
                                              en_beacon_direction);
            break;
        default:
            ul_ret = OAL_ERR_CODE_INVALID_CONFIG;
            break;
    }
    return ul_ret;
}