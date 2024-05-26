

#include "hmac_vsp.h"
#include "host_hal_ext_if.h"
#include "hmac_user.h"
#include "hmac_tx_data.h"
#include "hmac_tx_msdu_dscr.h"
#include "hmac_config.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_VSP_C

#ifdef _PRE_WLAN_FEATURE_VSP
#ifndef CONFIG_VCODEC_VSP_SUPPORT
static void wifi_tx_pkg_done(send_result *send_result);
static void wifi_rx_slice_done(rx_slice_mgmt *rx_slice_mgmt);
static rx_slice_mgmt *alloc_vdec_slice_buffer(uint32_t size);
#endif
void hmac_vsp_timer_intr_callback(void *data);

static const hmac_vsp_vcodec_ops g_default_ops = {
    .alloc_slice_mgmt = alloc_vdec_slice_buffer,
    .rx_slice_done = wifi_rx_slice_done,
    .wifi_tx_pkg_done = wifi_tx_pkg_done,
};

const hmac_vsp_vcodec_ops *g_vsp_vcodec_ops = &g_default_ops;

static hmac_vsp_info_stru g_vsp_info_res[MAX_VSP_INFO_COUNT] = {
    {.mac_common_timer = {.timer_id = 16}},
    {.mac_common_timer = {.timer_id = 17}},
};

/* 记录发送端的指针，方便获取 */
static hmac_vsp_info_stru *g_vsp_source;
hmac_vsp_info_stru *hmac_get_vsp_source(void)
{
    return g_vsp_source;
}


OAL_STATIC uint32_t hmac_vsp_set_status(mac_vap_stru *mac_vap, uint8_t mode)
{
    return hmac_config_send_event(mac_vap, WLAN_CFGID_VSP_STATUS_CHANGE, sizeof(uint8_t), (uint8_t *)&mode);
}

OAL_STATIC hmac_vsp_info_stru *hmac_alloc_vsp_info(void)
{
    int i;
    for (i = 0; i < MAX_VSP_INFO_COUNT; i++) {
        if (!g_vsp_info_res[i].used) {
            g_vsp_info_res[i].used = OAL_TRUE;
            return &g_vsp_info_res[i];
        }
    }
    return NULL;
}

OAL_STATIC void hmac_free_vsp_info(hmac_vsp_info_stru *vsp_info)
{
    vsp_info->used = OAL_FALSE;
}

hmac_vsp_info_stru *hmac_get_vsp_by_idx(uint8_t idx)
{
    if (idx < MAX_VSP_INFO_COUNT) {
        return &g_vsp_info_res[idx];
    }
    return NULL;
}

OAL_STATIC uint32_t hmac_vsp_init_common_timer(hmac_vsp_info_stru *vsp_info)
{
    hal_mac_common_timer *timer = &vsp_info->mac_common_timer;
    timer->timer_en = OAL_TRUE;
    timer->timer_mask_en = OAL_TRUE;
    timer->timer_unit = HAL_COMMON_TIMER_UNIT_1US;
    timer->timer_mode = HAL_COMMON_TIMER_MODE_ONE_SHOT;
    if (vsp_info->mode == VSP_MODE_SOURCE) {
        /* 发送端需要在timer中断上半部与device清ring同步 */
        timer->func = hmac_vsp_timer_intr_callback;
    }
    if (hal_host_init_common_timer(timer) != OAL_SUCC) {
        oam_error_log1(0, OAM_SF_TX, "hmac_vsp_tx_info_init :: init common timer[%d] fail", timer->timer_id);
        return OAL_FAIL;
    }
    return OAL_SUCC;
}

OAL_STATIC void hmac_vsp_deinit_common_timer(hmac_vsp_info_stru *vsp_info)
{
    hal_mac_common_timer *timer = &vsp_info->mac_common_timer;
    /* 关闭MAC定时器 */
    timer->timer_en = OAL_FALSE;
    hal_host_set_mac_common_timer(timer);
}

void hmac_vsp_source_queue_init(hmac_vsp_info_stru *vsp_info)
{
    hmac_vsp_slice_queue_init(&vsp_info->free_queue);
    hmac_vsp_slice_queue_init(&vsp_info->comp_queue);
}


hmac_vsp_info_stru *hmac_vsp_init(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, uint8_t mode)
{
    hmac_vsp_info_stru *vsp_info = NULL;
    mac_vap_stru *mac_vap = NULL;
    hal_host_device_stru *host_device = NULL;

    if (mode == VSP_MODE_SOURCE && !hmac_host_ring_tx_enabled()) {
        oam_warning_log0(0, 0, "{hmac_vsp_info_init::vsp init fail due to tx_switch != HOST_TX}");
        return NULL;
    }

    mac_vap = &hmac_vap->st_vap_base_info;
    host_device = hal_get_host_device(mac_vap->uc_device_id);
    if (host_device == NULL) {
        oam_warning_log0(0, OAM_SF_TX, "{hmac_vsp_init::host_device nullptr}");
        return NULL;
    }

    vsp_info = hmac_alloc_vsp_info();
    if (vsp_info == NULL) {
        oam_error_log0(0, 0, "{hmac_vsp_init::vsp info alloc failed}");
        return NULL;
    }

    vsp_info->hmac_user = hmac_user;
    vsp_info->hmac_vap = hmac_vap;
    vsp_info->tid_info = &hmac_user->tx_tid_info[WLAN_TIDNO_VSP];
    vsp_info->host_hal_device = host_device;
    vsp_info->mode = mode;

    hmac_vsp_source_queue_init(vsp_info);
    oal_spin_lock_init(&vsp_info->tx_lock);

    if (hmac_vsp_init_common_timer(vsp_info) != OAL_SUCC) {
        hmac_free_vsp_info(vsp_info);
        return NULL;
    }
    vsp_info->timer_running = OAL_FALSE;

    hmac_vsp_set_status(mac_vap, mode);
    vsp_info->next_timeout_frm = VSP_INVALID_FRAME_ID;
    vsp_info->next_timeout_slice = 0;
    vsp_info->last_rx_comp_frm = VSP_INVALID_FRAME_ID;
    vsp_info->last_rx_comp_slice = 0;
    vsp_info->last_vsync = hmac_vsp_get_timestamp(vsp_info);
    hmac_vsp_set_vcodec_ops(&g_default_ops);

    memset_s(&vsp_info->sink_stat, sizeof(struct vsp_sink_stat), 0, sizeof(struct vsp_sink_stat));
    memset_s(&vsp_info->source_stat, sizeof(struct vsp_source_stat), 0, sizeof(struct vsp_source_stat));
    vsp_info->enable = OAL_TRUE;
    if (mode == VSP_MODE_SOURCE) {
        g_vsp_source = vsp_info;
    }
    oam_warning_log2(0, 0, "hmac_vsp_init::VSP init SUCC, vap idx[%d], user idx[%d]!",
        mac_vap->uc_vap_id, hmac_user->st_user_base_info.us_assoc_id);
    return vsp_info;
}


void hmac_vsp_deinit(hmac_vsp_info_stru *vsp_info)
{
    if (vsp_info->mode == VSP_MODE_SOURCE) {
        if (g_vsp_source != vsp_info) {
            oam_error_log0(0, 0, "{hmac_vsp_deinit::not the last create vsp source}");
        } else {
            g_vsp_source = NULL;
        }
    }
    vsp_info->enable = OAL_FALSE;
    hmac_vsp_deinit_common_timer(vsp_info);
    hmac_vsp_set_status(&vsp_info->hmac_vap->st_vap_base_info, VSP_MODE_DISABLED);
    if (vsp_info->rx_slice_tbl) {
        oal_free(vsp_info->rx_slice_tbl);
        vsp_info->rx_slice_tbl = NULL;
    }
    vsp_info->hmac_vap = NULL;
    vsp_info->hmac_user = NULL;
    vsp_info->host_hal_device = NULL;
    hmac_free_vsp_info(vsp_info);
}

void hmac_vsp_dump_packet(vsp_msdu_hdr_stru *hdr, uint16_t len)
{
    oam_warning_log4(0, 0, "VSP DUMP PKT: frame id[%d] slice[%d], layer[%d], pkt[%d]",
        hdr->frame_id, hdr->slice_num, hdr->layer_num, hdr->number);
    if (len) {
        oal_print_hex_dump((uint8_t*)hdr, len, 32, "VSP DUMP PKT: ");
    }
}

uint32_t hmac_vsp_get_timestamp(hmac_vsp_info_stru *vsp_info)
{
    uint32_t curr_tsf = 0;
    if (oal_unlikely(hal_host_get_tsf_lo(vsp_info->host_hal_device,
        vsp_info->hmac_vap->hal_vap_id, &curr_tsf) != OAL_SUCC)) {
        oam_error_log0(0, 0, "{hmac_vsp_get_timestamp::get tsf failed}");
        curr_tsf = 0;
    }
    return curr_tsf;
}

void hmac_vsp_start_timer(hmac_vsp_info_stru *vsp_info, uint32_t timeout_us)
{
    oal_warn_on(vsp_info->timer_running);
    vsp_info->mac_common_timer.common_timer_val = timeout_us;
    vsp_info->mac_common_timer.timer_en = OAL_TRUE;
    hal_host_set_mac_common_timer(&vsp_info->mac_common_timer);
    vsp_info->timer_running = OAL_TRUE;
    oam_warning_log2(0, 0, "{hmac_vsp_start_timer::curr %u, %u us timeout}",
        hmac_vsp_get_timestamp(vsp_info), timeout_us);
}

void hmac_vsp_stop_timer(hmac_vsp_info_stru *vsp_info)
{
    vsp_info->mac_common_timer.timer_en = OAL_FALSE;
    hal_host_set_mac_common_timer(&vsp_info->mac_common_timer);
    vsp_info->timer_running = OAL_FALSE;
}

/* 设置slice的超时时间，需要保证timer_ref_vsync已经刷新到当前slice对应的vysnc */
void hmac_vsp_set_timeout_for_slice(uint8_t slice_id, hmac_vsp_info_stru *vsp_info)
{
    uint32_t now, timeout, consume, target;
    now = hmac_vsp_get_timestamp(vsp_info);
    consume = hmac_vsp_calc_runtime(vsp_info->timer_ref_vsync, now);
    target = vsp_info->param.max_transmit_dly_us + slice_id * vsp_info->param.slice_interval_us;
    if (consume < target) {
        timeout = target - consume;
        if (oal_warn_on(timeout < 100)) {
            timeout = vsp_info->param.slice_interval_us;
        }
    } else {
        oam_error_log2(0, 0, "{hmac_vsp_set_tx_timeout::slice[%d] consume[%d] is lager than target transmit dly}",
            slice_id, consume);
        timeout = vsp_info->param.slice_interval_us;
    }
    hmac_vsp_start_timer(vsp_info, timeout);
}

void hmac_vsp_record_vsync_ts(hmac_vsp_info_stru *vsp_info)
{
    vsp_info->last_vsync = hmac_vsp_get_timestamp(vsp_info);
}

/* 发送端注册给显示模块，在vsync中断调用, 记录tsf，用于与接收端时间同步 */
void hmac_vsp_vsync_intr_callback(void)
{
    hmac_vsp_info_stru *vsp_info = hmac_get_vsp_source();
    if (vsp_info->mode != VSP_MODE_SOURCE) {
        return;
    }
    hmac_vsp_record_vsync_ts(vsp_info);
}

/* 打桩测试，切换为打桩接口 */
void hmac_vsp_set_vcodec_ops(const hmac_vsp_vcodec_ops *ops)
{
    g_vsp_vcodec_ops = ops;
}

#ifndef CONFIG_VCODEC_VSP_SUPPORT
/* wifi call venc to send the result */
static void wifi_tx_pkg_done(send_result *send_result)
{
}

/* wifi call vdec to send slice buffer */
static void wifi_rx_slice_done(rx_slice_mgmt *rx_slice_mgmt)
{
}

/* wifi call vdec alloc buffer for wifi */
static rx_slice_mgmt *alloc_vdec_slice_buffer(uint32_t size)
{
    return NULL;
}
#endif
#endif
