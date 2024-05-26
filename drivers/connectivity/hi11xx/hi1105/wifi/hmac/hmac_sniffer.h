

#ifndef __HMAC_SNIFFER_H__
#define __HMAC_SNIFFER_H__

#include "wal_linux_netdev_ops.h"
#include "wal_linux_ioctl.h"
#include "wal_linux_vendor.h"
#include "mac_vap.h"
#include "hmac_device.h"

#ifdef __cplusplus // windows ut编译不过，后续下线清理
#if __cplusplus
extern "C" {
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_SNIFFER
uint32_t wal_vendor_sniffer_handle(oal_net_device_stru *p_net_dev,
    uint8_t *pc_cmd, uint32_t cmd_len);
extern void hmac_sniffer_save_data(hmac_device_stru *hmac_device,
    oal_netbuf_stru *pst_netbuf, uint16_t us_netbuf_num);
extern uint32_t hmac_monitor_report_frame_to_sdt(mac_rx_ctl_stru *pst_cb_ctrl, oal_netbuf_stru *pst_curr_netbuf);
extern uint32_t hmac_rx_process_data_event_monitor_handle(hmac_device_stru *pst_hmac_device,
    oal_netbuf_stru *pst_netbuf, uint16_t us_netbuf_num);
extern uint32_t hmac_sniffer_rx_info_event(frw_event_mem_stru *frw_mem);
extern void hmac_sniffer_rx_process_handle(hal_host_device_stru *hal_device, oal_netbuf_stru *netbuf,
    mac_rx_ctl_stru *rx_ctl);
extern void hmac_host_rx_sniffer_filter_check(hal_host_device_stru *hal_device, oal_netbuf_stru *netbuf,
    mac_rx_ctl_stru *rx_ctl);
extern bool hmac_host_rx_sniffer_is_on(hal_host_device_stru *hal_device);
#endif
extern void hmac_monitor_init(hmac_device_stru *hmac_device);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif /* end of hmac_sniffer.h */
