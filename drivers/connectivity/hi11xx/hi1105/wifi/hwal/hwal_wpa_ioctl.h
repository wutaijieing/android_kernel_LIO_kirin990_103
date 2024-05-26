

#ifndef __HWAL_IOCTL_H__
#define __HWAL_IOCTL_H__

// 1 其他头文件包含
#include "driver_hisi_common.h"
#include "oam_ext_if.h"
#include "oal_net.h"
#include "wal_main.h"
#include "mac_user.h"
#include "mac_vap.h"
#include "wal_config.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

// 3 枚举定义
typedef enum {
    HISI_CHAN_DISABLED      = 1<<0,
    HISI_CHAN_PASSIVE_SCAN  = 1<<1,
    HISI_CHAN_NO_IBSS       = 1<<2,
    HISI_CHAN_RADAR         = 1<<3,
    HISI_CHAN_NO_HT40PLUS   = 1<<4,
    HISI_CHAN_NO_HT40MINUS  = 1<<5,
    HISI_CHAN_NO_OFDM       = 1<<6,
    HISI_CHAN_NO_80MHZ      = 1<<7,
    HISI_CHAN_NO_160MHZ     = 1<<8,
} hisi_channel_flags_enum;

// 7 STRUCT定义
typedef int32_t (*hwal_ioctl_handler)(int8_t *ifname, void *buf);

// 10 函数声明
int32_t hwal_ioctl_set_key(int8_t *ifname, void *buf);
int32_t hwal_ioctl_new_key(int8_t *ifname, void *buf);
int32_t hwal_ioctl_del_key(int8_t *ifname, void *buf);
int32_t hwal_ioctl_set_ap(int8_t *ifname, void *buf);
int32_t hwal_ioctl_change_beacon(int8_t *ifname, void *buf);
int32_t hwal_ioctl_send_mlme(int8_t *ifname, void *buf);
int32_t hwal_ioctl_send_eapol(int8_t *ifname, void *buf);
int32_t hwal_ioctl_set_mode(int8_t *ifname, void *buf);
int32_t hwal_ioctl_set_power(int8_t *ifname, void *buf);
int32_t hwal_ioctl_receive_eapol(int8_t *ifname, void *buf);
int32_t hwal_ioctl_enable_eapol(int8_t *ifname, void *buf);
int32_t hwal_ioctl_disable_eapol(int8_t *ifname, void *buf);
int32_t hwal_ioctl_get_addr(int8_t *ifname, void *buf);
int32_t hwal_ioctl_get_hw_feature(int8_t *ifname, void *buf);
int32_t hwal_ioctl_stop_ap(int8_t *ifname, void *buf);
int32_t hwal_ioctl_del_virtual_intf(int8_t *ifname, void *buf);
#ifdef _PRE_WLAN_CONFIG_WPS
int32_t hwal_ioctl_set_ap_wps_p2p_ie(int8_t *ifname, void *buf);
#endif
int32_t hisi_hwal_wpa_ioctl(int8_t *ifname, hisi_ioctl_command_stru *pst_cmd);
int32_t hwal_ioctl_scan(int8_t *ifname, void *buf);
int32_t hwal_ioctl_disconnect(int8_t *ifname, void *buf);
int32_t hwal_ioctl_assoc(int8_t *ifname, void *buf);
int32_t hwal_ioctl_set_netdev(int8_t *ifname, void *buf);
uint8_t hwal_is_valid_ie_attr(uint8_t *puc_ie, uint32_t ul_ie_len);
int32_t hwal_ioctl_dhcp_start(int8_t *ifname, void *buf);
int32_t hwal_ioctl_dhcp_stop(int8_t *ifname, void *buf);
int32_t hwal_ioctl_dhcp_succ_check(int8_t *ifname, void *buf);
int32_t hwal_ioctl_set_pm_on(int8_t *ifname, void *buf);
int32_t hwal_ioctl_ip_notify(int8_t *ifname, void *buf);
int32_t hwal_ioctl_set_max_sta(int8_t *ifname, void *p_max_sta_num);
int32_t hwal_ioctl_sta_remove(int8_t *ifname, void *buf);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /* end of hwal_ioctl.h */

