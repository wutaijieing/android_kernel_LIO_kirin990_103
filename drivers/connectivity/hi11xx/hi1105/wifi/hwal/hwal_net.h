

#ifndef __HWAL_NET_H__
#define __HWAL_NET_H__

// 1 其他头文件包含
#include "driver_hisi_common.h"
#include "arch/oal_net.h"
#include "oam_ext_if.h"
#include "mac_vap.h"
#include "mac_data.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

// 2 宏定义
/* Ask if a drvier is ready to send */
#define NETIF_IS_NOT_READY() (NETIF_FLOW_CTRL_ON == g_en_netif_flow_ctrl)

/* 临时放这里，liteos系统下，wal初始化会起一个tx的任务，这个任务会根据信号量状态调度ndo_start_xmit */
#if (_PRE_OS_VERSION_LITEOS == _PRE_OS_VERSION)
typedef struct {
    oal_semaphore_stru       txdata_sema;
    oal_kthread_stru        *txdata_thread;
    oal_wait_queue_head_stru txdata_wq;
    oal_netbuf_head_stru     txdata_netbuf_head;
    uint32_t                 pkt_loss_cnt;
    oal_bool_enum_uint8      txthread_enable;
}hwal_txdata_thread_stru;

#define HWAL_TXDATA_THERAD_PRIORITY 6

#endif

// 3 函数声明
extern oal_netbuf_stru *hwal_lwip_skb_alloc(oal_net_device_stru *net_dev, uint16_t lwip_buflen);
extern int32_t hwal_netif_rx(oal_net_device_stru *net_dev, oal_netbuf_stru *netbuf);
extern int32_t hwal_lwip_init(oal_net_device_stru *net_dev);
extern void hwal_lwip_deinit(oal_net_device_stru *net_dev);
extern uint32_t hwal_skb_struct_free(oal_netbuf_stru *sk_buf);
extern oal_netbuf_stru *hwal_skb_struct_alloc(void);
extern uint32_t hwal_pbuf_convert_2_skb(oal_lwip_buf *lwip_buf, oal_netbuf_stru *sk_buf);
extern oal_lwip_buf *hwal_skb_convert_2_pbuf(oal_netbuf_stru *sk_buf);
extern void hwal_lwip_receive(oal_lwip_netif *netif, oal_netbuf_stru *drv_buf);
extern int32_t hwal_lwip_init(oal_net_device_stru *net_dev);
extern void hwal_lwip_deinit(oal_net_device_stru *net_dev);
extern int32_t hwal_lwip_notify(oal_lwip_netif *netif, uint32_t notify_type);
extern int32_t hwal_dhcp_start(oal_net_device_stru *netdev);
extern int32_t hwal_dhcp_stop(oal_net_device_stru *netdev);
extern int32_t hwal_dhcp_succ_check(oal_net_device_stru *netdev);
extern oal_lwip_netif *hwal_lwip_get_netif(const int8_t *pc_name);
extern void netif_set_flow_ctrl_status(oal_lwip_netif *netif, netif_flow_ctrl_enum_uint8 status);
extern uint32_t hwal_txdata_thread_init(void);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
