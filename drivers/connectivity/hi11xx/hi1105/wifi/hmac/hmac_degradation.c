

#ifndef __HMAC_DEGRADATION_C_
#define __HMAC_DEGRADATION_C_

#include "hmac_degradation.h"
#include "hmac_vap.h"
#include "hmac_user.h"
#include "hmac_config.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_DEGRADATION_C

#ifdef _PRE_WLAN_FEATURE_DEGRADE_SWITCH

void hmac_degradation_tx_chain_switch(mac_vap_stru *mac_vap, uint8_t degradation_enable)
{
    uint32_t ret;

    /***************************************************************************
        抛事件到DMAC层, 同步VAP最新状态到DMAC
    ***************************************************************************/
    ret = hmac_config_send_event(mac_vap, WLAN_CFGID_DEGRADATION_SWITCH,
        sizeof(uint8_t), (uint8_t *)&degradation_enable);
    if (oal_unlikely(ret != OAL_SUCC)) {
        oam_warning_log1(mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_degradation_tx_chain_disable:: failed[%d].}", ret);
    }
}
#endif /* end of _PRE_WLAN_FEATURE_DEGRADE_SWITCH */
#endif /* end of __HMAC_MCM_DEGRADATION_C_ */

