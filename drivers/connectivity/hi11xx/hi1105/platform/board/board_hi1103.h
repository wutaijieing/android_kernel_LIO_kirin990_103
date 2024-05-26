

#ifndef __BOARD_HI1103_H__
#define __BOARD_HI1103_H__

/* ����ͷ�ļ����� */
#include "plat_type.h"
#include "hw_bfg_ps.h"

/* �궨�� */
#define GPIO_BASE_ADDR        0X50004000
#define CHECK_DEVICE_RDY_ADDR 0X50000000
#define WLAN_HOST2DEV_GPIO ((unsigned int)(1 << 1))
#define WLAN_DEV2HOST_GPIO ((unsigned int)(1 << 0))

#define GPIO_DIRECTION_OUTPUT    0
#define GPIO_DIRECTION_INPUT     1

#define GPIO_LEVEL_CONFIG_REGADDR       0x0  /* GPIO�ܽŵĵ�ƽֵ���߻����ͼĴ��� */
#define GPIO_INOUT_CONFIG_REGADDR       0x04 /* GPIO�ܽŵ����ݷ������ */
#define GPIO_TYPE_CONFIG_REGADDR        0x30 /* GPIO�ܽŵ�ģʽ�Ĵ���:IO or INT */
#define GPIO_INT_POLARITY_REGADDR       0x3C /* GPIO�жϼ��ԼĴ��� */
#define GPIO_INT_TYPE_REGADDR           0x38 /* GPIO�жϴ������ͼĴ���:��ƽ��������ش��� */
#define GPIO_INT_CLEAR_REGADDR          0x4C /* GPIO����жϼĴ�����ֻ�Ա��ش������ж���Ч */
#define GPIO_LEVEL_GET_REGADDR          0x50 /* GPIO�ܽŵ�ǰ��ƽֵ�Ĵ��� */
#define GPIO_INTERRUPT_DEBOUNCE_REGADDR 0x48 /* GPIO�ܽ��Ƿ�ʹ��ȥ���� */

#define BFGX_SUBSYS_RST_DELAY   100
#define WIFI_SUBSYS_RST_DELAY   10

#define PROC_NAME_GPIO_WLAN_FLOWCTRL  "hi110x_wlan_flowctrl"

/* test ssi write bcpu code */
/* EXTERN VARIABLE */
#ifdef PLATFORM_DEBUG_ENABLE
extern int32_t g_device_monitor_enable;
#endif

/* �������� */
int32_t hi1103_wifi_subsys_reset(void);
void hi1103_bfgx_subsys_reset(void);


#if (defined(CONFIG_PCIE_KIRIN_SLT_HI110X)|| defined(CONFIG_PCIE_KPORT_SLT_DEVICE)) && defined(CONFIG_HISI_DEBUG_FS)
int32_t hi1103_pcie_chip_rc_slt_register(void);
int32_t hi1103_pcie_chip_rc_slt_unregister(void);
#endif

void board_callback_init_hi1103_power(void);
void board_info_init_hi1103(void);
#endif
