


#ifndef __HWAL_APP_IOCTL_H__
#define __HWAL_APP_IOCTL_H__


// 其他头文件包含
#include "oal_types.h"
#include "driver_hisi_common.h"
#include "driver_hisi_lib_api.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


// 数据结构类型和联合体类型定义
typedef int32_t (*hwal_app_ioctl_handler)(int8_t *ifname, void *buf);

// 函数声明
int hisi_wlan_set_monitor(unsigned char monitor_switch, char rssi_level);
int hisi_wlan_set_channel(hisi_channel_stru *channel_info);
int hisi_wlan_set_country(unsigned char *country);
char *hisi_wlan_get_country(void);
int hisi_wlan_rx_fcs_info(int *rx_succ_num);
int hisi_wlan_set_always_tx(char *param);
int hisi_wlan_set_always_rx(char *param);
int hisi_wlan_get_channel(hisi_channel_stru *channel_info);
int hisi_wlan_set_pm_switch(unsigned int pm_switch);
int hisi_wlan_enable_channel_14(void);
int hisi_wlan_disable_channel_14(void);

extern int32_t hwal_ioctl_ip_notify(int8_t *puc_ifname, void *p_buf);
extern int32_t hwal_ioctl_set_pm_on(int8_t *puc_ifname, void *p_buf);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif
