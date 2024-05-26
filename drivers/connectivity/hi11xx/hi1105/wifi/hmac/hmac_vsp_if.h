

#ifndef __HMAC_VSP_IF_H__
#define __HMAC_VSP_IF_H__

#include "hmac_user.h"
#include "hmac_tid.h"
#include "hmac_rx_data.h"
#include "host_hal_device.h"

#ifdef _PRE_WLAN_FEATURE_VSP
enum vsp_debug_cmd_enum {
    VSP_CMD_LINK_EN,
    VSP_CMD_TEST_EN,
    VSP_CMD_BUTT,
};

enum vsp_debug_param_enum {
    VSP_PARAM_MAX_TRANSMIT_DLY,
    VSP_PARAM_VSYNC_INTERVAL,
    VSP_PARAM_MAX_FEEDBACK_DLY,
    VSP_PARAM_SLICES_PER_FRAME,
    VSP_PARAM_LAYER_NUM,
    VSP_PARAM_BUTT,
};

struct hmac_vsp_cmd {
    uint8_t type;
    uint8_t link_en;
    uint8_t mode;
    uint8_t user_mac[OAL_MAC_ADDR_LEN];
    uint32_t param[VSP_PARAM_BUTT];
    uint8_t test_en;
    uint32_t test_frm_cnt;
};

#define VSP_RX_ACCEPT 0
#define VSP_RX_REFUSE 1

uint32_t hmac_vsp_common_timeout(frw_event_mem_stru *event_mem);
uint32_t hmac_set_vsp_info_addr(frw_event_mem_stru *frw_mem);
uint32_t hmac_vsp_rx_proc(hmac_host_rx_context_stru *rx_context, oal_netbuf_stru *netbuf);
void hmac_vsp_tx_complete_process(void);
uint32_t hmac_vsp_process_cmd(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param);
void hmac_vsp_stop(hmac_user_stru *hmac_user);
#endif
#endif
