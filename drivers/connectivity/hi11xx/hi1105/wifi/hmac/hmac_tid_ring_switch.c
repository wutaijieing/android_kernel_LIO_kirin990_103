

#include "hmac_tid_ring_switch.h"
#include "hmac_tx_switch_fsm.h"
#include "hmac_tx_switch.h"
#include "hmac_tx_data.h"
#include "hmac_vsp.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_TID_RING_SWITCH_C

hmac_tid_ring_switch_stru g_tid_switch_list;

static inline hmac_tid_info_stru *hmac_ring_switch_tid_info_getter(void *entry)
{
    return oal_container_of(entry, hmac_tid_info_stru, tid_ring_switch_entry);
}

static inline void *hmac_ring_switch_entry_getter(hmac_tid_info_stru *tid_info)
{
    return &tid_info->tid_ring_switch_entry;
}

const hmac_tid_list_ops g_tid_switch_list_ops = {
    .tid_info_getter = hmac_ring_switch_tid_info_getter,
    .entry_getter = hmac_ring_switch_entry_getter,
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    .for_each_rcu = NULL,
#endif
};

void hmac_tid_ring_switch_init(void)
{
    hmac_tid_list_init(&g_tid_switch_list.tid_list, &g_tid_switch_list_ops);
    g_tid_switch_list.tid_fix_switch = OAL_FALSE;
    hmac_tx_switch_fsm_init();
}

void hmac_tid_ring_switch_deinit(void)
{
    hmac_tid_switch_list_clear();
}


void hmac_tid_ring_switch(hmac_user_stru *hmac_user, hmac_tid_info_stru *tid_info, uint32_t mode)
{
    hmac_vap_stru *hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(hmac_user->st_user_base_info.uc_vap_id);
    if (hmac_vap == NULL) {
        oam_warning_log1(0, 0, "{hmac_tx_ring_switch::NULL vap id[%d]!}", hmac_user->st_user_base_info.uc_vap_id);
        return;
    }

    if (hmac_vap->st_vap_base_info.en_vap_state != MAC_VAP_STATE_UP ||
        hmac_user->st_user_base_info.en_user_asoc_state != MAC_USER_STATE_ASSOC) {
        oam_warning_log2(hmac_vap->st_vap_base_info.uc_vap_id, 0,
            "{hmac_tid_ring_switch::user_state[%d],vap_state[%d] can't switch}",
            hmac_vap->st_vap_base_info.en_vap_state, hmac_user->st_user_base_info.en_user_asoc_state);
        return;
    }

    switch (mode) {
        case HOST_RING_TX_MODE:
            hmac_tx_mode_d2h_switch(tid_info);
            break;
        case DEVICE_RING_TX_MODE:
            hmac_tx_mode_h2d_switch(tid_info);
            break;
        case TX_MODE_DEBUG_DUMP:
            hmac_tx_mode_switch_dump_process(tid_info);
            break;
        default:
            break;
    }
}

static inline uint8_t hmac_tx_ring_switch_allowed(hmac_user_stru *hmac_user, uint8_t tidno)
{
#ifdef _PRE_WLAN_FEATURE_VSP
    return !hmac_is_vsp_tid(hmac_user, tidno);
#else
    return OAL_TRUE;
#endif
}

/*
功能：程序动态切换入口
 */
uint8_t hmac_tx_ring_switch(hmac_tid_info_stru *tid_info, void *param)
{
    uint16_t user_id = tid_info->user_index;
    hmac_user_stru *hmac_user = mac_res_get_hmac_user(user_id);

    if (hmac_user == NULL) {
        oam_warning_log1(0, 0, "{hmac_tx_ring_switch::NULL user id[%d]!}", user_id);
        return OAL_CONTINUE;
    }

    if (hmac_tx_ring_switch_allowed(hmac_user, tid_info->tid_no) == OAL_TRUE) {
        hmac_tid_ring_switch(hmac_user, tid_info, *(uint32_t*)param);
    }

    return OAL_CONTINUE;
}

void hmac_tid_ring_switch_process(uint32_t tx_pps)
{
    uint32_t mode;
    if (!hmac_tx_ring_switch_enabled()) {
        return;
    }

    /* pps状态机状态切换 */
    hmac_tx_switch_fsm_handle_event(hmac_psm_tx_switch_throughput_to_event(tx_pps));

    /* 非Ring稳态不切换Ring实体 */
    mode = hmac_get_tx_switch_fsm_state();
    if ((mode != HOST_RING_TX_MODE) && (mode != DEVICE_RING_TX_MODE)) {
        return;
    }

    g_tid_switch_list.tid_list_count = hmac_tid_list_entry_count(&g_tid_switch_list.tid_list);
    hmac_tid_list_for_each(&g_tid_switch_list.tid_list, hmac_tx_ring_switch, &mode);
}

void hmac_tid_ring_switch_disable(hmac_user_stru *hmac_user)
{
    uint8_t tid_no;

    if (!hmac_tx_ring_switch_enabled()) {
        return;
    }

    for (tid_no = WLAN_TIDNO_BEST_EFFORT; tid_no < WLAN_TIDNO_BCAST; tid_no++) {
        hmac_tid_ring_switch_list_clear(&hmac_user->tx_tid_info[tid_no]);
        oam_warning_log2(0, 0, "{hmac_tid_ring_switch_disable::user[%d] tid[%d]}",
            hmac_user->st_user_base_info.us_assoc_id, tid_no);
    }
}
