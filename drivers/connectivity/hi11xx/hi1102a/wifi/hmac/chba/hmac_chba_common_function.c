

/* 1 头文件包含 */
#include "hmac_chba_common_function.h"
#include "hmac_chba_mgmt.h"
#include "hmac_chba_frame.h"
#include "hmac_chba_sync.h"
#include "hmac_chba_coex.h"
#include "hmac_chba_ps.h"
#include "hmac_chba_chan_switch.h"
#include "hmac_chba_function.h"
#include "hmac_chba_frame.h"
#include "securec.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHBA_COMMON_FUNCTION_C

#ifdef _PRE_WLAN_CHBA_MGMT

/* 2 全局变量定义 */
/* 3 函数声明 */
/* 4 函数实现 */
/* 功能: CHBA Vendor IE Header填写 */
uint32_t hmac_chba_set_vendor_ie_hdr(uint8_t *buf)
{
    if (buf == NULL) {
        return 0;
    }

    buf[0] = MAC_CHBA_VENDOR_EID; /* vendor ie: 221 */
    buf[2] = 0x00;
    buf[3] = 0xE0;
    buf[4] = 0xFC;
    buf[5] = MAC_OUI_TYPE_CHBA;
    return sizeof(mac_ieee80211_vendor_ie_stru);
}

/* 功能: 根据mac地址生成域bssid */
void hmac_chba_generate_domain_bssid(uint8_t *bssid, uint8_t *mac_addr, uint8_t addr_len)
{
    int32_t err;

    if (bssid == NULL || mac_addr == NULL) {
        oam_warning_log0(0, 0, "hmac_chba_generate_domain_bssid: input pointer is null.");
        return;
    }

    err = memcpy_s(bssid, OAL_MAC_ADDR_LEN, mac_addr, OAL_MAC_ADDR_LEN);
    if (err != EOK) {
        OAM_WARNING_LOG1(0, 0, "hmac_chba_generate_domain_bssid: memcpy failed, err[%d].", err);
        return;
    }

    bssid[0] = 0x00;
    bssid[1] = 0xE0;
    bssid[2] = 0xFC;
}

/* 功能 : 根据sync_state与role填写sync_state字段 */
void hmac_chba_fill_sync_state_field(uint8_t *sync_state)
{
    if (sync_state == NULL) {
        oam_warning_log0(0, 0, "hmac_chba_fill_sync_state_field: input pointer is null.");
        return;
    }

    *sync_state = MAC_CHBA_DECLARE_NON_SYNC;

    if (hmac_chba_get_sync_state() > CHBA_NON_SYNC && hmac_chba_get_role() == CHBA_MASTER) {
        *sync_state = MAC_CHBA_DECLARE_MASTER;
    } else if (hmac_chba_get_sync_state() > CHBA_NON_SYNC && hmac_chba_get_role() != CHBA_MASTER) {
        *sync_state = MAC_CHBA_DECLARE_SLAVE;
    }
}

/* 功能: 判断某个设备id是否在某个岛的设备id列表中 */
uint8_t hmac_chba_is_in_device_list(uint8_t device_id, uint8_t device_cnt, uint8_t *device_list)
{
    uint32_t idx;

    if (device_list == NULL) {
        oam_warning_log0(0, 0, "hmac_chba_is_in_device_list: input pointer is null.");
        return FALSE;
    }

    if (device_cnt == 0) {
        oam_warning_log0(0, 0, "hmac_chba_is_in_device_list: device cnt is 0.");
        return FALSE;
    }

    for (idx = 0; idx < device_cnt; idx++) {
        if (device_id == device_list[idx]) {
            return TRUE;
        }
    }

    return FALSE;
}

/* 功能: 判断某个地址是否属于本岛内设备，如果属于返回TRUE，如果不属于返回FALSE */
uint8_t hmac_chba_island_device_check(uint8_t *peer_addr)
{
    uint8_t idx, device_cnt;
    mac_chba_self_conn_info_stru *self_conn_info = hmac_chba_get_self_conn_info();

    if (peer_addr == NULL) {
        oam_warning_log0(0, 0, "hmac_chba_island_device_check::input pointer is null.");
        return FALSE;
    }

    device_cnt = self_conn_info->island_device_cnt;
    if (device_cnt == 0) {
        return FALSE;
    }

    for (idx = 0; idx < device_cnt; idx++) {
        if (oal_compare_mac_addr(peer_addr, self_conn_info->island_device_list[idx].mac_addr) == 0) {
            return TRUE;
        }
    }

    return FALSE;
}

/* 功能: 转化信道信息为driver_channel_stru(屏蔽差异) */
void hmac_chba_set_channel(uint8_t *channel_dst, uint8_t *channel_src, uint16_t size)
{
    int32_t err;

    if (channel_dst == NULL || channel_src == NULL) {
        return;
    }

    err = memcpy_s(channel_dst, size, channel_src, size);
    if (err != EOK) {
        OAM_WARNING_LOG1(0, 0, "hmac_chba_set_channel: memcpy failed, err[%d].", err);
    }
}

/* 功能: 启动定时器(ms级别) */
int32_t hmac_chba_create_timer_ms(frw_timeout_stru *timer, uint32_t timeout, frw_timeout_func handle)
{
    hmac_chba_vap_stru *chba_vap_info = NULL;
    hmac_vap_stru *hmac_vap = NULL;

    chba_vap_info = hmac_get_up_chba_vap_info();
    if (chba_vap_info == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(chba_vap_info->mac_vap_id);
    if (hmac_vap == NULL) {
        return OAL_ERR_CODE_PTR_NULL;
    }

    frw_create_timer(timer,
        handle, timeout, (void *)hmac_vap,
        OAL_FALSE, OAM_MODULE_ID_HMAC, hmac_vap->st_vap_base_info.ul_core_id);
    return OAL_SUCC;
}

/* 功能: 判断元素element是否属于集合set_s */
uint8_t hmac_chba_find_element(uint8_t *set_s, uint8_t cnt, uint8_t element)
{
    uint8_t s_idx;

    if (set_s == NULL) {
        return OAL_FALSE;
    }

    for (s_idx = 0; s_idx < cnt; s_idx++) {
        if (set_s[s_idx] == element) {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}

/* 特性开关设置 */
uint32_t hmac_chba_set_feature_switch(mac_vap_stru *mac_vap, uint16_t len, uint8_t *params)
{
    hmac_vap_stru *hmac_vap = NULL;
    hmac_chba_vap_stru *chba_vap_info = NULL;
    mac_chba_feature_switch_stru *cfg_params = NULL;

    if (oal_any_null_ptr2(mac_vap, params)) {
        OAM_ERROR_LOG0(0, OAM_SF_CHBA, "{hmac_chba_set_feature_switch:ptr is NULl!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
    chba_vap_info = hmac_chba_get_chba_vap_info(hmac_vap);
    if (chba_vap_info == NULL) {
        OAM_ERROR_LOG0(mac_vap->uc_vap_id, OAM_SF_CHBA, "{hmac_chba_set_feature_switch:chba_vap_info is NULl!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    cfg_params = (mac_chba_feature_switch_stru *)params;
    oam_warning_log3(0, 0, "CHBA: set feature switch, cmd_bitmap %X, chan_switch %d, ps %d",
        cfg_params->cmd_bitmap, cfg_params->chan_switch_enable, cfg_params->ps_enable);
    if (cfg_params->cmd_bitmap & BIT(CHBA_CHAN_SWITCH)) {
        hmac_chba_set_chan_switch_ctrl(cfg_params->chan_switch_enable);
    }

    if (cfg_params->cmd_bitmap & BIT(CHBA_PS_SWITCH)) {
        hmac_chba_set_ps_switch(cfg_params->ps_enable);
        /* 如果低功耗关闭，将bitmap设为全1 */
        if (cfg_params->ps_enable == FALSE) {
            chba_vap_info->vap_bitmap_level = CHBA_BITMAP_ALL_AWAKE_LEVEL;
            hmac_chba_sync_vap_bitmap_info(&(hmac_vap->st_vap_base_info), chba_vap_info);
            hmac_chba_save_update_beacon_pnf(hmac_chba_sync_get_domain_bssid());
        }
    }

    return OAL_SUCC;
}
#endif
