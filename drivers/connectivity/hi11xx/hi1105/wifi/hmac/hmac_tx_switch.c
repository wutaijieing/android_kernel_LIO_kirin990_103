

#include "hmac_tx_switch.h"
#include "hmac_tx_switch_fsm.h"
#include "hmac_config.h"
#include "hmac_tx_msdu_dscr.h"
#include "host_hal_ext_if.h"
#include "hmac_tid_ring_switch.h"
#include "hmac_tid_sche.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_TX_SWITCH_C

OAL_STATIC uint32_t hmac_tx_switch_send_event(mac_vap_stru *mac_vap, hmac_tid_info_stru *tid_info, uint8_t mode)
{
    uint32_t ret;
    tx_state_switch_stru param = {
        .user_id = tid_info->user_index,
        .tid_no = tid_info->tid_no,
        .cmd = mode,
    };
    param.tid_num = g_tid_switch_list.tid_list_count;

    ret = hmac_config_send_event(mac_vap, WLAN_CFGID_TX_SWITCH_START, sizeof(tx_state_switch_stru), (uint8_t *)&param);
    if (ret != OAL_SUCC) {
        oam_warning_log3(0, OAM_SF_TX, "{hmac_tx_switch_send_event::user[%d] tid[%d] switch to [%d] failed}",
            tid_info->user_index, tid_info->tid_no, mode);
    }

    return ret;
}

uint32_t hmac_tx_mode_d2h_switch(hmac_tid_info_stru *tid_info)
{
    if (oal_atomic_read(&tid_info->ring_tx_mode) != DEVICE_RING_TX_MODE) {
        return OAL_SUCC;
    }

    oal_atomic_set(&tid_info->ring_tx_mode, D2H_SWITCHING_MODE);

    if (hmac_tx_switch_send_event(mac_res_get_mac_vap(0), tid_info, HOST_RING_TX_MODE) != OAL_SUCC) {
        oal_atomic_set(&tid_info->ring_tx_mode, DEVICE_RING_TX_MODE);
        return OAL_FAIL;
    }

    oam_warning_log2(0, 0, "{hmac_tx_mode_d2h_switch::user[%d] tid[%d] switching...}",
        tid_info->user_index, tid_info->tid_no);

    return OAL_SUCC;
}

static inline uint8_t hmac_tx_mode_h2d_switch_allowed(hmac_msdu_info_ring_stru *tx_ring)
{
    un_rw_ptr release_ptr = { .rw_ptr = tx_ring->release_index};
    un_rw_ptr write_ptr = { .rw_ptr = tx_ring->base_ring_info.write_index };

    return !oal_atomic_read(&tx_ring->msdu_cnt) && hmac_tx_rw_ptr_compare(write_ptr, release_ptr) == RING_PTR_EQUAL;
}

void hmac_tx_mode_h2d_switch_process(hmac_tid_info_stru *tid_info)
{
    if (oal_atomic_read(&tid_info->ring_tx_mode) != H2D_SWITCHING_MODE) {
        return;
    }

    if (!hmac_tx_mode_h2d_switch_allowed(&tid_info->tx_ring)) {
        return;
    }

    if (hmac_tx_switch_send_event(mac_res_get_mac_vap(0), tid_info, DEVICE_RING_TX_MODE) != OAL_SUCC) {
        oal_atomic_set(&tid_info->ring_tx_mode, HOST_RING_TX_MODE);
        return;
    }

    oam_warning_log2(0, 0, "{hmac_tx_mode_h2d_switch::user[%d] tid[%d] switching...}",
        tid_info->user_index, tid_info->tid_no);
}

uint32_t hmac_tx_mode_h2d_switch(hmac_tid_info_stru *tid_info)
{
    hmac_msdu_info_ring_stru *tx_ring = &tid_info->tx_ring;

    if (oal_atomic_read(&tid_info->ring_tx_mode) != HOST_RING_TX_MODE) {
        return OAL_SUCC;
    }

    oal_spin_lock(&tx_ring->tx_lock);
    oal_spin_lock(&tx_ring->tx_comp_lock);

    oal_atomic_set(&tid_info->ring_tx_mode, H2D_SWITCHING_MODE);
    hmac_tx_mode_h2d_switch_process(tid_info);

    oal_spin_unlock(&tx_ring->tx_lock);
    oal_spin_unlock(&tx_ring->tx_comp_lock);

    return OAL_SUCC;
}

void hmac_tx_mode_switch_dump(hmac_tid_info_stru *tid_info)
{
    oam_warning_log4(0, 0, "{hmac_tx_mode_switch_dump::tx_mode[%d] sync_th[%d] sync_cnt[%d] sche_th}",
        oal_atomic_read(&tid_info->ring_tx_mode), oal_atomic_read(&tid_info->ring_sync_th),
        tid_info->ring_sync_cnt, oal_atomic_read(&tid_info->tid_sche_th));

    oam_warning_log3(0, 0, "{hmac_tx_mode_switch_dump::tid_len[%d] ring_msdu_cnt[%d] last_period_tx_msdu[%d]}",
        oal_netbuf_list_len(&tid_info->tid_queue), oal_atomic_read(&tid_info->tx_ring.msdu_cnt),
        oal_atomic_read(&tid_info->tx_ring.last_period_tx_msdu));

    oam_warning_log3(0, 0, "{hmac_tx_mode_switch_dump::release_ptr[%d] rptr[%d] wptr[%d]}",
        tid_info->tx_ring.release_index, tid_info->tx_ring.base_ring_info.read_index,
        tid_info->tx_ring.base_ring_info.write_index);
}

void hmac_tx_mode_switch_dump_process(hmac_tid_info_stru *tid_info)
{
    hmac_tx_mode_switch_dump(tid_info);
    hmac_tx_switch_send_event(mac_res_get_mac_vap(0), tid_info, TX_MODE_DEBUG_DUMP);
}

static inline oal_bool_enum_uint8 hmac_tx_mode_switch_ring_ptr_abnormal(hmac_tid_info_stru *tid_info)
{
    return ((tid_info->tx_ring.base_ring_info.read_index != tid_info->tx_ring.base_ring_info.write_index) ||
           (tid_info->tx_ring.base_ring_info.read_index != tid_info->tx_ring.release_index)) ? OAL_TRUE : OAL_FALSE;
}

static inline void hmac_tx_mode_switch_ring_ptr_reset(hmac_tid_info_stru *tid_info)
{
    tid_info->tx_ring.base_ring_info.read_index = 0;
    tid_info->tx_ring.base_ring_info.write_index = 0;
    tid_info->tx_ring.release_index = 0;
    tid_info->tx_ring_device_info->msdu_info_word2.word2_bit.write_ptr = 0;
    tid_info->tx_ring_device_info->msdu_info_word3.word3_bit.read_ptr = 0;
}

OAL_STATIC void hmac_tx_mode_d2h_switch_process(hmac_tid_info_stru *tid_info)
{
    if (hmac_tx_mode_switch_ring_ptr_abnormal(tid_info) == OAL_TRUE) {
        hmac_tx_mode_switch_dump(tid_info);
        hmac_tx_mode_switch_ring_ptr_reset(tid_info);
    }

    hmac_tx_reg_write_ring_info(tid_info, TID_CMDID_CREATE);
}

uint32_t hmac_tx_mode_switch_process(frw_event_mem_stru *event_mem)
{
    tx_state_switch_stru *tx_switch_param = NULL;
    hmac_user_stru *hmac_user = NULL;
    hmac_tid_info_stru *tid_info = NULL;

    if (event_mem == NULL) {
        oam_error_log0(0, OAM_SF_RX, "{hmac_tx_mode_switch_process::event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    tx_switch_param = (tx_state_switch_stru *)(frw_get_event_stru(event_mem)->auc_event_data);
    hmac_user = mac_res_get_hmac_user(tx_switch_param->user_id);
    if (hmac_user == NULL) {
        oam_warning_log1(0, OAM_SF_RX, "{hmac_tx_mode_switch_process::hmac user[%d] null.}", tx_switch_param->user_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    tid_info = &hmac_user->tx_tid_info[tx_switch_param->tid_no];

    if (tx_switch_param->cmd == oal_atomic_read(&tid_info->ring_tx_mode)) {
        oam_warning_log3(0, 0, "{hmac_tx_mode_switch_process::user[%d] tid[%d] target[%d] is the same as current mode",
            tx_switch_param->user_id, tx_switch_param->tid_no, tx_switch_param->cmd);
        return OAL_SUCC;
    }

    oam_warning_log3(0, 0, "{hmac_tx_mode_switch_process::user[%d] tid[%d] mode[%d]}",
        tx_switch_param->user_id, tx_switch_param->tid_no, tx_switch_param->cmd);

    if (tx_switch_param->cmd == HOST_RING_TX_MODE) {
        hmac_tx_mode_d2h_switch_process(tid_info);
    }

    oal_smp_mb();

    /* update tx mode & reschedule tid thread */
    oal_atomic_set(&tid_info->ring_tx_mode, tx_switch_param->cmd);

    hmac_tid_schedule();

    return OAL_SUCC;
}

hmac_psm_tx_switch_event_enum hmac_psm_tx_switch_throughput_to_event(uint32_t tx_pps)
{
    if (tx_pps <= HMAC_PSM_TX_SWITCH_LOW_PPS) {
        return PSM_TX_H2D_SWITCH;
    } else if (tx_pps >= HMAC_PSM_TX_SWITCH_HIGH_PPS) {
        return PSM_TX_D2H_SWITCH;
    } else {
        return PSM_TX_SWITCH_BUTT;
    }
}

/*
功能：配置命令切换入口
*/
uint32_t hmac_tx_mode_switch(mac_vap_stru *mac_vap, uint32_t *params)
{
    uint32_t tx_pps_thre;
    uint8_t mode = (uint8_t)params[0];

    if ((mode == HOST_RING_TX_MODE) || (mode == DEVICE_RING_TX_MODE)) {
        g_tid_switch_list.tid_fix_switch = OAL_TRUE;
    } else {
        g_tid_switch_list.tid_fix_switch = OAL_FALSE;
    }
    oam_warning_log1(0, OAM_SF_CFG, "{hmac_tx_mode_switch::switch mode[%d]}", mode);
    hmac_tx_fsm_quick_switch_to_device();
    tx_pps_thre = (mode == HOST_RING_TX_MODE) ? HMAC_PSM_TX_SWITCH_HIGH_PPS : HMAC_PSM_TX_SWITCH_LOW_PPS;
    hmac_tid_ring_switch_process(tx_pps_thre);
    hmac_tid_ring_switch_process(tx_pps_thre);
    return OAL_SUCC;
}


oal_bool_enum_uint8 hmac_tx_keep_host_switch(void)
{
    uint8_t tid_entry = hmac_tid_list_entry_count(&g_tid_switch_list.tid_list);
    /* Device ring资源足够，根据pps动态切换；否则全部部署于Host */
    if (tid_entry <= hal_host_get_device_tx_ring_num()) {
        /* 允许根据pps状态机切换 */
        return OAL_FALSE;
    }
    if (g_tid_switch_list.tid_fix_switch == OAL_TRUE) {
        return OAL_TRUE;
    }
    if (hmac_get_tx_switch_fsm_state() == HOST_RING_TX_MODE) {
        return OAL_TRUE;
    }
    /* 保持Host ring state */
    hmac_tid_ring_switch_process(HMAC_PSM_TX_SWITCH_HIGH_PPS);
    return OAL_TRUE;
}

/*
功能：发送流程H2D/D2H PPS门限切换入口
 */
void hmac_tx_pps_switch(uint32_t tx_pps)
{
    /* TX跟随RX切换, PPS不处理 */
    if (!hmac_tx_ring_switch_sync_mode()) {
        return;
    }
    if (g_tid_switch_list.tid_fix_switch == OAL_TRUE) {
        return;
    }
    hmac_tid_ring_switch_process(tx_pps);
}
/*
功能：发送流程H2D/D2H通道统一切换入口
 */
void hmac_tx_switch(uint8_t new_rx_mode)
{
    uint32_t tx_pps_thre;

    /* TRX独立切换, 不处理 */
    if (hmac_tx_ring_switch_sync_mode() == OAL_TRUE) {
        return;
    }
    if (g_tid_switch_list.tid_fix_switch == OAL_TRUE) {
        return;
    }
    /* 根据rx ring模式判定tx ring模型, 两者保持一致: DDR TRX or WRAM TRX */
    tx_pps_thre = (new_rx_mode == HAL_DDR_RX) ? HMAC_PSM_TX_SWITCH_HIGH_PPS : HMAC_PSM_TX_SWITCH_LOW_PPS;
    oam_warning_log1(0, 0, "{hmac_tx_switch::tx ring switch by rx mode, pps[%d]!}", tx_pps_thre);
    hmac_tid_ring_switch_process(tx_pps_thre);
    hmac_tid_ring_switch_process(tx_pps_thre);
}
