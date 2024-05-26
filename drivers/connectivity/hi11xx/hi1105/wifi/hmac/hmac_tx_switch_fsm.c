

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
        /* host ring tx״̬����������, ������һ��ͳ������ */
        { HOST_RING_TX_MODE, H2D_WAIT_SWITCHING_MODE, PSM_TX_H2D_SWITCH, NULL },
        /* ��������ͳ����������������, ��ʼ������sta���ض�tid�л���device ring tx */
        { H2D_WAIT_SWITCHING_MODE, DEVICE_RING_TX_MODE, PSM_TX_H2D_SWITCH, hmac_tx_fsm_wait_switch_to_device},
        /* �ڶ���ͳ����������������, ȡ���л�, �����������л���device ring tx��tid�л�host ring tx */
        { H2D_WAIT_SWITCHING_MODE, HOST_RING_TX_MODE, PSM_TX_D2H_SWITCH, hmac_tx_fsm_wait_resume_to_host },
        /* device ring tx״̬����������, ֱ�ӽ�����sta���ض�tid�л���host ring tx */
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

    /* �ﵽ�ۼ��л�����, ����H2D�л� */
    g_tx_switch_fsm.host_keep_cnt = 0;
    return OAL_SUCC;
}


uint32_t hmac_tx_fsm_wait_resume_to_host(void)
{
    /* WAITING״̬��,��������, �ָ�Host ring tx */
    g_tx_switch_fsm.host_keep_cnt = 0;
    return OAL_SUCC;
}

void hmac_tx_fsm_quick_switch_to_device(void)
{
    /* ����������device �л����� */
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
