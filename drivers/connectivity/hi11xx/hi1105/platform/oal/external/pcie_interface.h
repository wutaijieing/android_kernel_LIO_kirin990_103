

#ifndef __PCIE_INTERFACE_H__
#define __PCIE_INTERFACE_H__

#ifdef CONFIG_ARCH_KIRIN_PCIE
#ifndef _PRE_HI375X_PCIE
#ifdef CONFIG_ARCH_PLATFORM
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#include <linux/platform_drivers/pcie-kport-api.h>
#else
#include <linux/hisi/pcie-kport-api.h>
#endif
#else
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#include <linux/platform_drivers/pcie-kirin-api.h>
#else
#include <linux/hisi/pcie-kirin-api.h>
#endif
#endif
#endif
#endif

#ifdef CONFIG_KIRIN_PCIE_L1SS_IDLE_SLEEP
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <linux/platform_drivers/hisi_pcie_idle_sleep.h>
#else
#include <linux/hisi/hisi_pcie_idle_sleep.h>
#endif
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
#include <linux/dma-map-ops.h>
#endif

#ifdef _PRE_HI375X_PCIE
#ifdef _PRE_PRODUCT_HI3751_V900
int pcie_enumerate(u32 rc_idx);
u32 show_link_state(u32 rc_id);
int pcie_pm_control(int resume_flag, u32 rc_idx);
int pcie_lp_ctrl(u32 rc_idx, u32 enable);
int pcie_power_notifiy_register(u32 rc_idx, int (*poweron)(void* data),
                                int (*poweroff)(void* data), void* data);
#define kirin_pcie_enumerate               pcie_enumerate
#define kirin_pcie_pm_control              pcie_pm_control
#define kirin_pcie_lp_ctrl                 pcie_lp_ctrl
#define kirin_pcie_power_notifiy_register  pcie_power_notifiy_register
#else
int hipcie_enumerate(u32 rc_idx);
u32 show_link_state(u32 rc_id);
int hipcie_pm_control(int resume_flag, u32 rc_idx);
int hipcie_lp_ctrl(u32 rc_idx, u32 enable);
int hipcie_power_notifiy_register(u32 rc_idx, int (*poweron)(void* data),
                                  int (*poweroff)(void* data), void* data);
#define kirin_pcie_enumerate               hipcie_enumerate
#define kirin_pcie_pm_control              hipcie_pm_control
#define kirin_pcie_lp_ctrl                 hipcie_lp_ctrl
#define kirin_pcie_power_notifiy_register  hipcie_power_notifiy_register
#endif

#else
#ifdef CONFIG_ARCH_KIRIN_PCIE
#ifdef CONFIG_ARCH_PLATFORM
#define KIRIN_PCIE_EVENT_LINKDOWN PCIE_KPORT_EVENT_LINKDOWN
#define KIRIN_PCIE_TRIGGER_CALLBACK PCIE_KPORT_TRIGGER_CALLBACK
int pcie_kport_enumerate(u32 rc_idx);
int pcie_kport_pm_control(int power_ops, u32 rc_idx);
int pcie_kport_lp_ctrl(u32 rc_idx, u32 enable);
int pcie_kport_register_event(struct pcie_kport_register_event *reg);
int pcie_kport_deregister_event(struct pcie_kport_register_event *reg);
int pcie_kport_power_notifiy_register(u32 rc_id, int (*poweron)(void *data),
                                      int (*poweroff)(void *data), void *data);
#define kirin_pcie_enumerate                pcie_kport_enumerate
#define kirin_pcie_pm_control               pcie_kport_pm_control
#define kirin_pcie_lp_ctrl                  pcie_kport_lp_ctrl
#define kirin_pcie_power_notifiy_register   pcie_kport_power_notifiy_register
#define kirin_pcie_register_event           pcie_kport_register_event
#define kirin_pcie_deregister_event         pcie_kport_deregister_event
#define kirin_pcie_notify                   pcie_kport_notify
#else
/* hisi kirin PCIe functions */
int kirin_pcie_enumerate(u32 rc_idx);
int kirin_pcie_pm_control(int resume_flag, u32 rc_idx);
int kirin_pcie_lp_ctrl(u32 rc_idx, u32 enable);
/* notify WiFi when RC PCIE power on/off */
int kirin_pcie_power_notifiy_register(u32 rc_idx, int (*poweron)(void *data),
                                      int (*poweroff)(void *data), void *data);
#endif // CONFIG_ARCH_PLATFORM
u32 show_link_state(u32 rc_id);
#endif // CONFIG_ARCH_KIRIN_PCIE
#endif // _PRE_HI375X_PCIE

#endif /* end for __PCIE_INTERFACE_H__ */

