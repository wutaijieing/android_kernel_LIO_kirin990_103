

#include "oal_schedule.h"
#include "hmac_vsp_if.h"
#include "hmac_vsp.h"
#include "hmac_vsp_source.h"
#include "hmac_vsp_sink.h"
#include "hmac_tx_data.h"
#include "hmac_vsp_test.h"
#include "hmac_tid_ring_switch.h"
#include "hmac_tx_ring_alloc.h"
#include "hmac_scan.h"
#include "hmac_config.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_VSP_MAIN_C

#ifdef _PRE_WLAN_FEATURE_VSP
static void hmac_vsp_set_fast_cpus(hmac_vsp_info_stru *vsp_info)
{
#if defined(CONFIG_ARCH_HISI) && defined(CONFIG_NR_CPUS)
#if CONFIG_NR_CPUS > OAL_BUS_HPCPU_NUM
    struct cpumask fast_cpus;
    external_get_fast_cpus(&fast_cpus);
    cpumask_clear_cpu(cpumask_first(&fast_cpus), &fast_cpus);
    set_cpus_allowed_ptr(vsp_info->evt_loop, &fast_cpus);
#endif
#endif
}

typedef struct {
    hmac_scan_state_enum scan_state;
    mac_cfg_ps_open_stru pm_state;
} hmac_vsp_config_stru;

static void hmac_vsp_config_sta(mac_vap_stru *mac_vap, hmac_vsp_config_stru *param)
{
    if (!IS_LEGACY_STA(mac_vap)) {
        return;
    }

    hmac_config_set_sta_pm_on(mac_vap, 0, (uint8_t *)&param->pm_state);
    hmac_bgscan_enable(mac_vap, 0, (uint8_t *)&param->scan_state);
}

static void hmac_vsp_stop_config_sta(hmac_vsp_info_stru *vsp_info)
{
    hmac_vsp_config_stru param = {
        .scan_state = HMAC_BGSCAN_ENABLE,
        .pm_state = {
            .uc_pm_ctrl_type = MAC_STA_PM_CTRL_TYPE_CMD,
            .uc_pm_enable = MAC_STA_PM_SWITCH_ON,
        }
    };

    hmac_vsp_config_sta(&vsp_info->hmac_vap->st_vap_base_info, &param);
}

static void hmac_vsp_start_config_sta(hmac_vsp_info_stru *vsp_info)
{
    hmac_vsp_config_stru param = {
        .scan_state = HMAC_BGSCAN_DISABLE,
        .pm_state = {
            .uc_pm_ctrl_type = MAC_STA_PM_CTRL_TYPE_CMD,
            .uc_pm_enable = MAC_STA_PM_SWITCH_OFF,
        }
    };

    hmac_vsp_config_sta(&vsp_info->hmac_vap->st_vap_base_info, &param);
}

static uint32_t hmac_vsp_sink_start(hmac_vsp_info_stru *vsp_info, vsp_param *param)
{
    uint32_t tbl_size = VSP_FRAME_ID_TOTAL * sizeof(rx_slice_mgmt*) * param->slices_per_frm;

    vsp_info->rx_slice_tbl = (rx_slice_mgmt**)oal_memalloc(tbl_size);
    if (vsp_info->rx_slice_tbl == NULL) {
        oam_error_log1(0, 0, "{hmac_vsp_start::slice table alloc failed, size %d!}", tbl_size);
        hmac_vsp_deinit(vsp_info);
        return OAL_FAIL;
    }
    memset_s(vsp_info->rx_slice_tbl, tbl_size, 0, tbl_size);

    return OAL_SUCC;
}

static uint32_t hmac_vsp_source_start(hmac_vsp_info_stru *vsp_info)
{
    oal_netbuf_list_head_init(&vsp_info->feedback_q);
    oal_wait_queue_init_head(&vsp_info->evt_wq);
    vsp_info->evt_flags = 0;
    vsp_info->evt_loop = oal_thread_create(hmac_vsp_source_evt_loop, NULL, NULL, "vsp_evt_loop", SCHED_FIFO, 98, -1);
    if (vsp_info->evt_loop == NULL) {
        oam_error_log0(0, 0, "{hmac_vsp_start::start event loop failed!}");
        hmac_vsp_deinit(vsp_info);
        return OAL_FAIL;
    }

    hmac_vsp_set_fast_cpus(vsp_info);

    return OAL_SUCC;
}

static void hmac_vsp_config_param(vsp_param *param, vsp_param *cfg_param)
{
    /* 后续修改为入参，通过命令配置 */
    memcpy_s(param, sizeof(vsp_param), cfg_param, sizeof(vsp_param));
    param->slice_interval_us = param->vsync_interval_us / param->slices_per_frm;
    param->feedback_pkt_len = param->layer_num * 2 + sizeof(vsp_feedback_frame);

    oam_warning_log4(0, 0, "{hmac_vsp_start::max tx dly %dus, vsync intrvl %dus, %d slices/frame, slice intrvl %dus",
        param->max_transmit_dly_us, param->vsync_interval_us, param->slices_per_frm, param->slice_interval_us);
}

static void hmac_vsp_clear_tx_ring(hmac_tid_info_stru *tid_info)
{
    hmac_msdu_info_ring_stru *tx_ring = &tid_info->tx_ring;

    oal_spin_lock(&tx_ring->tx_lock);
    oal_spin_lock(&tx_ring->tx_comp_lock);

    hmac_wait_tx_ring_empty(tid_info);
    hmac_tx_ring_release_all_netbuf(tx_ring);

    oal_spin_unlock(&tx_ring->tx_lock);
    oal_spin_unlock(&tx_ring->tx_comp_lock);
}

uint32_t hmac_vsp_start(hmac_vap_stru *hmac_vap, hmac_user_stru *hmac_user, uint8_t mode, vsp_param *param)
{
    hmac_vsp_info_stru *vsp_info = hmac_vsp_init(hmac_vap, hmac_user, mode);
    uint32_t ret = OAL_SUCC;

    if (vsp_info == NULL) {
        return OAL_FAIL;
    }

    hmac_vsp_config_param(&vsp_info->param, param);

    if (vsp_info->mode == VSP_MODE_SINK) {
        ret = hmac_vsp_sink_start(vsp_info, &vsp_info->param);
    } else if (vsp_info->mode == VSP_MODE_SOURCE) {
        ret = hmac_vsp_source_start(vsp_info);
    } else {
        oam_error_log1(0, 0, "{hmac_vsp_start::mode[%d] invalid}", vsp_info->mode);
        return OAL_FAIL;
    }

    hmac_vsp_start_config_sta(vsp_info);
    hmac_vsp_clear_tx_ring(vsp_info->tid_info);
    hmac_tid_ring_switch(vsp_info->hmac_user, vsp_info->tid_info, HOST_RING_TX_MODE);
    hmac_tx_ba_setup(hmac_vap, hmac_user, WLAN_TIDNO_VSP);
    hmac_user->vsp_hdl = vsp_info;

    return ret;
}

void hmac_vsp_stop(hmac_user_stru *hmac_user)
{
    hmac_vsp_info_stru *vsp_info = NULL;
    if (hmac_user->vsp_hdl == NULL) {
        return;
    }

    vsp_info = hmac_user->vsp_hdl;
    if (vsp_info->mode == VSP_MODE_SOURCE) {
        if (vsp_info->evt_loop != NULL) {
            oal_thread_stop(vsp_info->evt_loop, NULL);
            vsp_info->evt_loop = NULL;
        }
    }

    hmac_vsp_clear_tx_ring(vsp_info->tid_info);
    hmac_vsp_stop_config_sta(vsp_info);
    hmac_vsp_deinit(vsp_info);
    hmac_user->vsp_hdl = NULL;
}


uint32_t hmac_vsp_common_timeout(frw_event_mem_stru *event_mem)
{
    hmac_vsp_info_stru *vsp_info = NULL;
    hal_host_common_timerout_stru *timeout_event;
    timeout_event = (hal_host_common_timerout_stru*)frw_get_event_stru(event_mem)->auc_event_data;
    if (timeout_event->timer == NULL) {
        oam_error_log0(0, 0, "{hmac_vsp_common_timeout::timer ptr is null.}");
        return OAL_FAIL;
    }
    vsp_info = oal_container_of(timeout_event->timer, hmac_vsp_info_stru, mac_common_timer);
    if (vsp_info->mode != VSP_MODE_SINK) {
        return OAL_SUCC;
    }

    /* 若下个slice已开始rx, 此处stop timer, 可能会有问题? */
    vsp_info->timer_running = OAL_FALSE;
    hmac_vsp_sink_rx_slice_timeout(vsp_info);
    /* 处理report帧,实际接口要在rx done时上报 */
    unref_param(event_mem);
    return OAL_SUCC;
}

static void hmac_vsp_rx_amsdu_proc(hmac_vsp_info_stru *vsp_info, oal_netbuf_stru *netbuf)
{
    switch (vsp_info->mode) {
        case VSP_MODE_SINK:
            hmac_vsp_sink_rx_amsdu_proc(vsp_info, netbuf);
            break;
        case VSP_MODE_SOURCE:
            hmac_vsp_source_rx_amsdu_proc(vsp_info, netbuf);
            break;
        default:
            hmac_rx_netbuf_list_free(netbuf);
    }
}

uint32_t hmac_vsp_rx_proc(hmac_host_rx_context_stru *rx_context, oal_netbuf_stru *netbuf)
{
    hmac_vsp_info_stru *vsp_info = NULL;

    if (oal_unlikely(rx_context->hmac_user == NULL)) {
        return VSP_RX_REFUSE;
    }

    vsp_info = rx_context->hmac_user->vsp_hdl;
    if (vsp_info == NULL || !vsp_info->enable) {
        return VSP_RX_REFUSE;
    }

    if (MAC_GET_RX_CB_TID((mac_rx_ctl_stru *)oal_netbuf_cb(netbuf)) != WLAN_TIDNO_VSP) {
        return VSP_RX_REFUSE;
    }

    hmac_vsp_rx_amsdu_proc(vsp_info, netbuf);

    return VSP_RX_ACCEPT;
}

void hmac_vsp_start_cmd_proc(mac_vap_stru *mac_vap, struct hmac_vsp_cmd *cmd)
{
#define VSP_USEC_PER_MSEC 1000
    hmac_vap_stru *hmac_vap = NULL;
    hmac_user_stru *hmac_user = NULL;
    vsp_param param;
    uint16_t user_idx;
    if (mac_vap_find_user_by_macaddr(mac_vap, cmd->user_mac, &user_idx) != OAL_SUCC) {
        oam_warning_log0(0, 0, "{hmac_vsp_start_cmd_proc::find user by mac failed!}");
        return;
    }
    hmac_user = mac_res_get_hmac_user(user_idx);
    if (hmac_user == NULL) {
        oam_warning_log1(0, 0, "{hmac_vsp_start_cmd_proc::get user[%d] failed!}", user_idx);
        return;
    }
    if (cmd->link_en) {
        hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(mac_vap->uc_vap_id);
        if (hmac_vap == NULL) {
            return;
        }
        param.max_transmit_dly_us = cmd->param[VSP_PARAM_MAX_TRANSMIT_DLY] * VSP_USEC_PER_MSEC;
        param.vsync_interval_us = cmd->param[VSP_PARAM_VSYNC_INTERVAL] * VSP_USEC_PER_MSEC;
        param.feedback_dly_us = cmd->param[VSP_PARAM_MAX_FEEDBACK_DLY] * VSP_USEC_PER_MSEC;
        param.slices_per_frm = cmd->param[VSP_PARAM_SLICES_PER_FRAME];
        param.layer_num = cmd->param[VSP_PARAM_LAYER_NUM];
        hmac_vsp_start(hmac_vap, hmac_user, cmd->mode, &param);
    } else {
        hmac_vsp_stop(hmac_user);
    }
}

void hmac_vsp_test_cmd_proc(mac_vap_stru *mac_vap, struct hmac_vsp_cmd *cmd)
{
    hmac_vsp_info_stru *vsp_info = NULL;
    if (cmd->test_en) {
        vsp_info = hmac_get_vsp_source();
        if (vsp_info == NULL) {
            hmac_vsp_test_start_sink();
        } else {
            hmac_vsp_test_start_source(cmd->test_frm_cnt);
        }
    } else {
        hmac_vsp_test_stop();
    }
}

uint32_t hmac_vsp_process_cmd(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param)
{
    struct hmac_vsp_cmd *cmd = (struct hmac_vsp_cmd *)param;

    if (cmd->type == VSP_CMD_LINK_EN) {
        hmac_vsp_start_cmd_proc(mac_vap, cmd);
    } else if (cmd->type == VSP_CMD_TEST_EN) {
        hmac_vsp_test_cmd_proc(mac_vap, cmd);
    } else {
        return OAL_FAIL;
    }
    return OAL_SUCC;
}
#endif
