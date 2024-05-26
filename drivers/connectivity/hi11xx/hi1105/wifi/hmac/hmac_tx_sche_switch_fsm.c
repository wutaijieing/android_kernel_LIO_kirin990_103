

#include "hmac_tx_sche_switch_fsm.h"

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)

#include "hmac_pps_fsm.h"
#include "hmac_tx_data.h"
#include "hmac_config.h"
#include "mac_common.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_TX_SCHE_SWITCH_FSM_C

typedef enum {
    TX_SCHE_H2D_SWITCH, /* 高流量事件, 尝试将Host调度切换至Device调度 */
    TX_SCHE_D2H_SWITCH, /* 低流量事件, 尝试将Device调度切换至Device调度 */

    TX_SCHE_SWITCH_EVENT_BUTT,
} hmac_tx_sche_switch_event_enum;

typedef enum {
    TX_SCHE_HOST_TRIGGER_MODE,   /* Host下发事件调度模式 */
    TX_SCHE_DEVICE_LOOP_MODE,    /* Device Main Loop调度模式 */
    TX_SCHE_H2D_SWITCHING_MODE,

    TX_SCHE_MODE_BUTT,
} hmac_tx_sche_switch_fsm_state_enum;

static uint32_t hmac_tx_sche_switch_send_event(uint8_t mode)
{
    uint32_t ret;
    tx_sche_switch_stru param = { .device_loop_sche_enabled = mode == TX_SCHE_DEVICE_LOOP_MODE };

    ret = hmac_config_send_event((mac_vap_stru *)mac_res_get_mac_vap(0), WLAN_CFGID_TX_SCHE_SWITCH,
                                 sizeof(tx_sche_switch_stru), (uint8_t *)&param);
    if (ret != OAL_SUCC) {
        oam_error_log1(0, OAM_SF_TX, "{hmac_tx_sche_switch_send_event::switch to mode[%d] failed}", mode);
        return OAL_FAIL;
    }

    return OAL_SUCC;
}

static uint32_t hmac_tx_sche_d2h_switch(void)
{
    return hmac_tx_sche_switch_send_event(TX_SCHE_HOST_TRIGGER_MODE);
}

static uint32_t hmac_tx_sche_h2d_switch(void)
{
    return hmac_tx_sche_switch_send_event(TX_SCHE_DEVICE_LOOP_MODE);
}

#define HMAC_TX_SCHE_SWITCH_FSM_TABLE_SIZE 4
hmac_pps_fsm_stru g_tx_sche_switch_fsm = {
    .fsm_table = {
        /* device main loop调度模式吞吐量低, 立刻将调度方式切换至host trigger, 避免影响低功耗唤醒 */
        { TX_SCHE_DEVICE_LOOP_MODE, TX_SCHE_HOST_TRIGGER_MODE, TX_SCHE_D2H_SWITCH, hmac_tx_sche_d2h_switch },
        /* host trigger模式下吞吐量高, 继续等待一个统计周期 */
        { TX_SCHE_HOST_TRIGGER_MODE, TX_SCHE_H2D_SWITCHING_MODE, TX_SCHE_H2D_SWITCH, NULL },
        /* 第二个统计周期内吞吐量低, 取消切换 */
        { TX_SCHE_H2D_SWITCHING_MODE, TX_SCHE_HOST_TRIGGER_MODE, TX_SCHE_D2H_SWITCH, NULL },
        /* 第二个统计周期内吞吐量高, 将调度方式切换至device main loop, 以降低高吞吐下的host侧MIPS */
        { TX_SCHE_H2D_SWITCHING_MODE, TX_SCHE_DEVICE_LOOP_MODE, TX_SCHE_H2D_SWITCH, hmac_tx_sche_h2d_switch },
    },
    .entry_num = HMAC_TX_SCHE_SWITCH_FSM_TABLE_SIZE,
    .id = 1,
    .current_state = ATOMIC_INIT(TX_SCHE_HOST_TRIGGER_MODE),
    .enable = ATOMIC_INIT(OAL_TRUE),
};

uint8_t hmac_host_tx_sche_mode(void)
{
    return oal_atomic_read(&g_tx_sche_switch_fsm.current_state) != TX_SCHE_DEVICE_LOOP_MODE;
}

#define HMAC_TX_SCHE_SWITCH_LOW_THROUGHPUT 200
#define HMAC_TX_SCHE_SWITCH_HIGH_THROUGHPUT 300
static uint8_t hmac_tx_sche_switch_throughput_to_event(uint32_t tx_throughput)
{
    if (tx_throughput < HMAC_TX_SCHE_SWITCH_LOW_THROUGHPUT) {
        return TX_SCHE_D2H_SWITCH;
    } else if (tx_throughput > HMAC_TX_SCHE_SWITCH_HIGH_THROUGHPUT) {
        return TX_SCHE_H2D_SWITCH;
    } else {
        return TX_SCHE_SWITCH_EVENT_BUTT;
    }
}

void hmac_tx_sche_switch_process(uint32_t tx_throughput)
{
    if (!hmac_device_loop_sched_enabled() || !oal_atomic_read(&g_tx_sche_switch_fsm.enable)) {
        return;
    }

    hmac_pps_fsm_handle_event(hmac_tx_sche_switch_throughput_to_event(tx_throughput), &g_tx_sche_switch_fsm);
}

#else

uint8_t hmac_host_tx_sche_mode(void)
{
    return OAL_TRUE;
}

#endif
