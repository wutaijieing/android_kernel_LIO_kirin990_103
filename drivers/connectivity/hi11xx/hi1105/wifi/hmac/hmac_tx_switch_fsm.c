

#include "hmac_tx_switch_fsm.h"
#include "hmac_tid_ring_switch.h"
#include "hmac_pps_fsm.h"
#include "mac_common.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_TX_SWITCH_FSM_C

uint32_t hmac_tx_fsm_wait_switch_to_device(void);
uint32_t hmac_tx_fsm_wait_resume_to_host(void);

#define HMAC_PSM_TX_SWITCH_KEEP_TH 10
#define HMAC_PSM_TX_SWITCH_FSM_TABLE_SIZE 4

hmac_pps_fsm_stru g_tx_switch_fsm = {
    .fsm_table = {
        /* host ring tx状态下吞吐量低, 继续等一个统计周期 */
        { HOST_RING_TX_MODE, H2D_WAIT_SWITCHING_MODE, PSM_TX_H2D_SWITCH, NULL },
        /* 连续两个统计周期内吞吐量低, 开始将所有sta的特定tid切换至device ring tx */
        { H2D_WAIT_SWITCHING_MODE, DEVICE_RING_TX_MODE, PSM_TX_H2D_SWITCH, hmac_tx_fsm_wait_switch_to_device},
        /* 第二个统计周期内吞吐量高, 取消切换, 并将所有已切换到device ring tx的tid切回host ring tx */
        { H2D_WAIT_SWITCHING_MODE, HOST_RING_TX_MODE, PSM_TX_D2H_SWITCH, hmac_tx_fsm_wait_resume_to_host },
        /* device ring tx状态下吞吐量高, 直接将所有sta的特定tid切换回host ring tx */
        { DEVICE_RING_TX_MODE, HOST_RING_TX_MODE, PSM_TX_D2H_SWITCH, NULL },
    },
    .entry_num = HMAC_PSM_TX_SWITCH_FSM_TABLE_SIZE,
    .id = 0,
    .host_keep_cnt = 0,
};


uint32_t hmac_tx_fsm_wait_switch_to_device(void)
{
    g_tx_switch_fsm.host_keep_cnt++;
    if (g_tx_switch_fsm.host_keep_cnt < HMAC_PSM_TX_SWITCH_KEEP_TH) {
        return OAL_FAIL;
    }

    /* 达到累加切换门限, 启动H2D切换 */
    g_tx_switch_fsm.host_keep_cnt = 0;
    return OAL_SUCC;
}


uint32_t hmac_tx_fsm_wait_resume_to_host(void)
{
    /* WAITING状态下,流量增大, 恢复Host ring tx */
    g_tx_switch_fsm.host_keep_cnt = 0;
    return OAL_SUCC;
}

void hmac_tx_fsm_quick_switch_to_device(void)
{
    /* 立即满足向device 切换条件 */
    g_tx_switch_fsm.host_keep_cnt = HMAC_PSM_TX_SWITCH_KEEP_TH;
}

void hmac_tx_switch_fsm_handle_event(hmac_psm_tx_switch_event_enum ring_state)
{
    hmac_pps_fsm_handle_event(ring_state, &g_tx_switch_fsm);
}

uint32_t hmac_get_tx_switch_fsm_state(void)
{
    return oal_atomic_read(&g_tx_switch_fsm.current_state);
}

void hmac_tx_switch_fsm_init(void)
{
    oal_atomic_set(&g_tx_switch_fsm.current_state, HOST_RING_TX_MODE);
    oal_atomic_set(&g_tx_switch_fsm.enable, OAL_TRUE);
}
