

#ifndef __HMAC_TX_SWITCH_FSM_H__
#define __HMAC_TX_SWITCH_FSM_H__

#include "oal_ext_if.h"

#define HMAC_PSM_TX_SWITCH_LOW_PPS 4500
#define HMAC_PSM_TX_SWITCH_HIGH_PPS 5500

typedef enum {
    PSM_TX_H2D_SWITCH,
    PSM_TX_D2H_SWITCH,

    PSM_TX_SWITCH_BUTT,
} hmac_psm_tx_switch_event_enum;

uint32_t hmac_get_tx_switch_fsm_state(void);
void hmac_tx_switch_fsm_init(void);
void hmac_tx_switch_fsm_handle_event(hmac_psm_tx_switch_event_enum ring_state);
void hmac_tx_fsm_quick_switch_to_device(void);
#endif
