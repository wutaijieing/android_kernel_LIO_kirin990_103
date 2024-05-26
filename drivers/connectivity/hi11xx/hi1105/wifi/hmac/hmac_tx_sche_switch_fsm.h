

#ifndef __HMAC_TX_SCHE_SWITCH_FSM_H__
#define __HMAC_TX_SCHE_SWITCH_FSM_H__

#include "oal_ext_if.h"

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
void hmac_tx_sche_switch_process(uint32_t tx_throughput);
#endif
uint8_t hmac_host_tx_sche_mode(void);

#endif
