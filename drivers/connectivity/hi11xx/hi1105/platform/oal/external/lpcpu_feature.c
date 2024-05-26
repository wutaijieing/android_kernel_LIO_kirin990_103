
#include <linux/module.h> /* kernel module definitions */
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#if defined(CONFIG_LPCPU_IDLE_SLEEP)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
#include <linux/platform_drivers/lpcpu_idle_sleep.h>
#else
#include <linux/hisi/lpcpu_idle_sleep.h>
#endif
#elif defined(CONFIG_HISI_IDLE_SLEEP) // ���ݾɽӿ�
#include <linux/hisi/hisi_idle_sleep.h>
#endif
#include "plat_pm_wlan.h"
#include "oal_types.h"
#include "oal_hcc_bus.h"
#include "plat_debug.h"
#include "lpcpu_feature.h"

int32_t gps_ilde_sleep_vote(uint32_t val)
{
#if defined(CONFIG_LPCPU_IDLE_SLEEP)
    lpcpu_idle_sleep_vote(ID_GPS, val);
    ps_print_info("lpcpu_idle_sleep_vote 1!\n");
#elif defined(CONFIG_HISI_IDLE_SLEEP)
    hisi_idle_sleep_vote(ID_GPS, val);
    ps_print_info("lpcpu_idle_sleep_vote 1!\n");
#endif

    return OAL_SUCC;
}

/*
 * �� �� ��  : wlan_pm_idle_sleep_vote
 * ��������  : wlanͶƱ�Ƿ�����kirin����32k idleģʽ
 * �������  : TRUE:������FALSE:������
 * �� �� ֵ  : ��ʼ������ֵ���ɹ���ʧ��ԭ��
 */
void wlan_pm_idle_sleep_vote(uint8_t uc_allow)
{
#if defined(CONFIG_HISI_IDLE_SLEEP)
    if (uc_allow == ALLOW_IDLESLEEP) {
        hisi_idle_sleep_vote(ID_WIFI, 0);
    } else {
        hisi_idle_sleep_vote(ID_WIFI, 1);
    }
#if defined(CONFIG_KIRIN_PCIE_L1SS_IDLE_SLEEP) || defined(CONFIG_PCIE_KPORT_IDLE)
    hcc_bus_wlan_pm_vote(hcc_get_bus(HCC_EP_WIFI_DEV), (uint8_t)uc_allow);
#endif
#elif defined(CONFIG_LPCPU_IDLE_SLEEP)
    if (uc_allow == ALLOW_IDLESLEEP) {
        lpcpu_idle_sleep_vote(ID_WIFI, 0);
    } else {
        lpcpu_idle_sleep_vote(ID_WIFI, 1);
    }
#if defined(CONFIG_KIRIN_PCIE_L1SS_IDLE_SLEEP) || defined(CONFIG_PCIE_KPORT_IDLE)
    hcc_bus_wlan_pm_vote(hcc_get_bus(HCC_EP_WIFI_DEV), (uint8_t)uc_allow);
#endif
#endif
}