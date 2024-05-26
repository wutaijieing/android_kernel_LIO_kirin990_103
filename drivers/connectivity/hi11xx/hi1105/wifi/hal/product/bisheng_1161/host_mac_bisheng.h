

#ifndef __HOST_MAC_BISHENG_H__
#define __HOST_MAC_BISHENG_H__

#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "hal_common.h"
#include "host_hal_device.h"

/* BISHENG MAC�Ĵ�������ַ */
#define MAC_GLB_REG_BASE     0x40020000
#define MAC_RPU_REG_BASE     0x40026000
#define MAC_TMU_REG_BASE     0x40028000
#define MAC_PPU_REG_BASE     0x4002c000

/* BISHENG PHY�Ĵ�������ַ */
#define DFX_REG_BANK_BASE                            0x40064000
#define RX_CHN_EST_REG_BANK_BASE                     0x40072800
#define RF_CTRL_REG_BANK_0_BASE                      0x40064800
#define CALI_TEST_REG_BANK_0_BASE                    0x40061000
#define PHY_GLB_REG_BANK_BASE                        0x40060800
#define BISHENG_GLB_CTL_RB_BASE_BASE                 0x40000000

/* master: 0x4002_0000~0x4002_ffff slave: 0x4004_0000~0x4004_ffff */
#define BISHENG_MAC_BANK_REG_OFFSET 0x20000
/* master: 0x4006_0000~0x          slave: 0x400a_0000~0x */
#define BISHENG_PHY_BANK_REG_OFFSET 0x40000

/* PHY �Ĵ��� */
#define DFX_REG_BANK_PHY_INTR_RPT_REG                   (DFX_REG_BANK_BASE + 0x110) /* PHY�ж�״̬�ϱ��Ĵ��� */
#define DFX_REG_BANK_CFG_INTR_MASK_HOST_REG             (DFX_REG_BANK_BASE + 0x20)  /* PHY��host�ж�MASK�Ĵ��� */
#define DFX_REG_BANK_CFG_INTR_CLR_REG                   (DFX_REG_BANK_BASE + 0x28)  /* PHY�ж�����Ĵ��� */
#define CALI_TEST_REG_BANK_0_FTM_CFG_REG                           (CALI_TEST_REG_BANK_0_BASE + 0x214) /* FTM���üĴ��� */
#define RX_CHN_EST_REG_BANK_CHN_EST_COM_REG                        (RX_CHN_EST_REG_BANK_BASE + 0x1D0) /* �ŵ�����ͨ�üĴ��� */
#define RF_CTRL_REG_BANK_0_CFG_ONLINE_CALI_USE_RF_REG_EN_REG       (RF_CTRL_REG_BANK_0_BASE + 0x34)
#define CALI_TEST_REG_BANK_0_FIFO_FORCE_EN_REG         (CALI_TEST_REG_BANK_0_BASE + 0x28)  /* FIFOʹ�ܶ�Ӧ�ļĴ������õ�ַ */
#define PHY_GLB_REG_BANK_PHY_BW_MODE_REG               (PHY_GLB_REG_BANK_BASE + 0x4)  /* Ƶ��ģʽ���üĴ��� */

/* MAC �Ĵ��� */
#define MAC_GLB_REG_HOST_INTR_STATUS_REG                     (MAC_GLB_REG_BASE + 0x1004) /* �ϱ���Host���ж�״̬�Ĵ��� */
#define MAC_GLB_REG_BA_INFO_BUF_WPTR_REG                     (MAC_GLB_REG_BASE + 0x105C) /* BA INFO BUFFER��дָ���ϱ� */
#define MAC_RPU_REG_RX_NORM_DATA_FREE_RING_RPTR_REG          (MAC_RPU_REG_BASE + 0x1000) /* ������ͨ����֡Free Ring�Ķ�ָ���ϱ� */
#define MAC_RPU_REG_RX_SMALL_DATA_FREE_RING_RPTR_REG         (MAC_RPU_REG_BASE + 0x1004) /* ����С������֡Free Ring�Ķ�ָ���ϱ� */
#define MAC_RPU_REG_RX_DATA_CMP_RING_WPTR_REG                (MAC_RPU_REG_BASE + 0x1008) /* ��������֡Complete Ring��дָ���ϱ� */
#define MAC_RPU_REG_RPT_HOST_FREE_RING_STATUS_REG            (MAC_RPU_REG_BASE + 0x100C) /* FREE RING�ϱ���Ϣ */
#define MAC_RPU_REG_RX_PPDU_HOST_DESC_FREE_RING_RPTR_REG  (MAC_RPU_REG_BASE + 0x1010) /* RX PPDU HOST������Free Ring�Ķ�ָ���ϱ� */
#define MAC_RPU_REG_RPT_PPDU_HOST_FREE_RING_STATUS_REG       (MAC_RPU_REG_BASE + 0x1014) /* PPDU HOST FREE RING�ϱ���Ϣ */
#define MAC_GLB_REG_TX_BA_INFO_BUF_DEPTH_REG                 (MAC_GLB_REG_BASE + 0x3A4)  /* TX BA INFO BUFFER������üĴ��� */
#define MAC_GLB_REG_TX_BA_INFO_BUF_ADDR_LSB_REG              (MAC_GLB_REG_BASE + 0x3A8)  /* TX BA INFO BUFFER����ַ���üĴ��� */
#define MAC_GLB_REG_TX_BA_INFO_BUF_ADDR_MSB_REG              (MAC_GLB_REG_BASE + 0x3AC)  /* TX BA INFO BUFFER����ַ���üĴ��� */
#define MAC_GLB_REG_TX_BA_INFO_RPTR_REG                   (MAC_GLB_REG_BASE + 0x3B0)  /* TX BA INFO RPTR BUFFER����ַ���üĴ��� */
#define MAC_GLB_REG_TX_BA_INFO_WPTR_REG                   (MAC_GLB_REG_BASE + 0x3B4)  /* TX BA INFO WPTR BUFFER����ַ���üĴ��� */
#define MAC_RPU_REG_RX_DATA_BUFF_LEN_REG                     (MAC_RPU_REG_BASE + 0x58)   /* RX����֡Buffer��С���üĴ��� */
#define MAC_RPU_REG_RX_NORM_DATA_FREE_RING_ADDR_LSB_REG   (MAC_RPU_REG_BASE + 0x5C)   /* ������ͨ����֡Free Ring����ַ���üĴ���LSB */
#define MAC_RPU_REG_RX_NORM_DATA_FREE_RING_ADDR_MSB_REG   (MAC_RPU_REG_BASE + 0x60)   /* ������ͨ����֡Free Ring����ַ���üĴ���MSB */
#define MAC_RPU_REG_RX_SMALL_DATA_FREE_RING_ADDR_LSB_REG  (MAC_RPU_REG_BASE + 0x64)   /* ����С������֡Free Ring����ַ���üĴ���_LSB */
#define MAC_RPU_REG_RX_SMALL_DATA_FREE_RING_ADDR_MSB_REG  (MAC_RPU_REG_BASE + 0x68)   /* ����С������֡Free Ring����ַ���üĴ���_MSB */
#define MAC_RPU_REG_RX_DATA_CMP_RING_ADDR_LSB_REG         (MAC_RPU_REG_BASE + 0x6C)   /* ��������֡Complete Ring����ַ���üĴ���LSB */
#define MAC_RPU_REG_RX_DATA_CMP_RING_ADDR_MSB_REG         (MAC_RPU_REG_BASE + 0x70)   /* ��������֡Complete Ring����ַ���üĴ���MSB */
#define MAC_RPU_REG_RX_NORM_DATA_FREE_RING_SIZE_REG          (MAC_RPU_REG_BASE + 0x74)   /* ������ͨ����֡Free Ring�Ĵ�С���üĴ��� */
#define MAC_RPU_REG_RX_SMALL_DATA_FREE_RING_SIZE_REG         (MAC_RPU_REG_BASE + 0x78)   /* ����С������֡Free Ring�Ĵ�С���üĴ��� */
#define MAC_RPU_REG_RX_DATA_CMP_RING_SIZE_REG             (MAC_RPU_REG_BASE + 0x7C)   /* ��������֡Complete Ring�Ĵ�С���üĴ��� */
#define MAC_RPU_REG_RX_NORM_DATA_FREE_RING_WPTR_REG         (MAC_RPU_REG_BASE + 0x80)   /* ������ͨ����֡Free Ring��дָ�����üĴ��� */
#define MAC_RPU_REG_RX_SMALL_DATA_FREE_RING_WPTR_REG        (MAC_RPU_REG_BASE + 0x84)   /* ����С������֡Free Ring��дָ�����üĴ��� */
#define MAC_RPU_REG_RX_DATA_CMP_RING_RPTR_REG             (MAC_RPU_REG_BASE + 0x88)   /* ��������֡Complete Ring�Ķ�ָ�����üĴ��� */
#define MAC_RPU_REG_RX_PPDU_HOST_DESC_FREE_RING_ADDR_LSB_REG (MAC_RPU_REG_BASE + 0x98)   /* RX PPDU HOST������Free Ring����ַLSB */
#define MAC_RPU_REG_RX_PPDU_HOST_DESC_FREE_RING_ADDR_MSB_REG (MAC_RPU_REG_BASE + 0x9C)   /* RX PPDU HOST������Free Ring����ַMSB */
#define MAC_RPU_REG_RX_PPDU_HOST_DESC_FREE_RING_SIZE_REG     (MAC_RPU_REG_BASE + 0xA0)   /* RX PPDU HOST������Free Ring�Ĵ�С */
#define MAC_RPU_REG_RX_PPDU_HOST_DESC_FREE_RING_WPTR_REG     (MAC_RPU_REG_BASE + 0xA4)   /* RX PPDU HOST������Free Ring��дָ�� */
#define MAC_RPU_REG_RX_PPDU_HOST_DESC_FREE_RING_RPTR_CFG_REG (MAC_RPU_REG_BASE + 0xA8)   /* RX PPDU HOST��������Free Ring�Ķ�ָ�� */
#define MAC_GLB_REG_HOST_INTR_CLR_REG                        (MAC_GLB_REG_BASE + 0x35C)  /* Host�ж�����Ĵ��� */
#define MAC_GLB_REG_HOST_INTR_MASK_REG                       (MAC_GLB_REG_BASE + 0x360)  /* Host�ж����μĴ��� */
#define MAC_RPU_REG_RX_DATA_CMP_RING_WPTR_CFG_REG          (MAC_RPU_REG_BASE + 0x8C)   /* ��������֡Complete Ring��дָ�����üĴ��� */
#define MAC_RPU_REG_RX_NORM_DATA_FREE_RING_RPTR_CFG_REG    (MAC_RPU_REG_BASE + 0x90)   /* ������ͨ����֡Free Ring�Ķ�ָ�����üĴ��� */
#define MAC_RPU_REG_RX_SMALL_DATA_FREE_RING_RPTR_CFG_REG     (MAC_RPU_REG_BASE + 0x94)   /* ����С������֡��Free Ring�Ķ�ָ�� */
#define MAC_RPU_REG_RX_FRAMEFILT_REG                         (MAC_RPU_REG_BASE + 0x4)    /* RX���˿��ƼĴ��� */
#define MAC_RPU_REG_RX_FRAMEFILT2_REG                        (MAC_RPU_REG_BASE + 0x8)    /* RX���˿��ƼĴ���2 */

#define MAC_GLB_REG_DMAC_INTR_MASK_REG                       (MAC_GLB_REG_BASE + 0x368)  /* DMAC�ж����μĴ��� */
#define MAC_RPU_REG_RX_CTRL_REG                              (MAC_RPU_REG_BASE + 0x0)    /* ���տ��ƼĴ��� */
#define MAC_PPU_REG_CSI_PROCESS_REG               (MAC_PPU_REG_BASE + 0x4)    /* CSI�����ϱ����üĴ��� */
#define MAC_PPU_REG_CSI_BUF_BASE_ADDR_LSB_REG     (MAC_PPU_REG_BASE + 0x14)   /* CSI�����ϱ�Ƭ��ѭ��BUF����ַLSB */
#define MAC_PPU_REG_CSI_BUF_BASE_ADDR_MSB_REG     (MAC_PPU_REG_BASE + 0x18)   /* CSI�����ϱ�Ƭ��ѭ��BUF����ַMSB */
#define MAC_PPU_REG_CSI_BUF_SIZE_REG              (MAC_PPU_REG_BASE + 0x1C)   /* CSI�����ϱ�Ƭ��ѭ��BUF��С */
#define MAC_PPU_REG_CSI_WHITELIST_ADDR_LSB_0_REG  (MAC_PPU_REG_BASE + 0x128)  /* CSI������������32λ */
#define MAC_PPU_REG_CSI_WHITELIST_ADDR_MSB_0_REG  (MAC_PPU_REG_BASE + 0x1A8)  /* CSI������������16λ */

#define MAC_PPU_REG_LOCATION_INFO_MASK_REG        (MAC_PPU_REG_BASE + 0x1004) /* ��λ��Ϣ�ϱ� */
#define MAC_PPU_REG_CSI_INFO_ADDR_MSB_REG         (MAC_PPU_REG_BASE + 0x1008) /* CSI�ϱ���ַ��32bit */
#define MAC_PPU_REG_CSI_INFO_ADDR_LSB_REG         (MAC_PPU_REG_BASE + 0x100C) /* CSI�ϱ���ַ��32bit */
#define MAC_PPU_REG_FTM_INFO_ADDR_MSB_REG         (MAC_PPU_REG_BASE + 0x1010) /* FTM�ϱ���ַ��32bit */
#define MAC_PPU_REG_FTM_INFO_ADDR_LSB_REG         (MAC_PPU_REG_BASE + 0x1014) /* FTM�ϱ���ַ��32bit */


#define MAC_PPU_REG_FTM_PROCESS_REG               (MAC_PPU_REG_BASE + 0x8)    /* FTM�����ϱ����üĴ��� */
#define MAC_PPU_REG_FTM_BUF_BASE_ADDR_LSB_REG     (MAC_PPU_REG_BASE + 0xC)    /* FTM�����ϱ�Ƭ��ѭ��BUF����ַLSB */
#define MAC_PPU_REG_FTM_BUF_BASE_ADDR_MSB_REG     (MAC_PPU_REG_BASE + 0x10)   /* FTM�����ϱ�Ƭ��ѭ��BUF����ַMSB */
#define MAC_PPU_REG_FTM_WHITELIST_ADDR_LSB_0_REG  (MAC_PPU_REG_BASE + 0x28)   /* FTM������������32λ */
#define MAC_PPU_REG_FTM_WHITELIST_ADDR_MSB_0_REG  (MAC_PPU_REG_BASE + 0xA8)   /* FTM������������16λ */

#define MAC_GLB_REG_DMAC_COMMON_TIMER_MASK_REG               (MAC_GLB_REG_BASE + 0x384)  /* DMACͨ�ü������ж����μĴ��� */
#define MAC_TMU_REG_COMMON_TIMER_CTRL_0_REG                (MAC_TMU_REG_BASE + 0xE8)   /* ͨ�ö�ʱ����ز������� */
#define MAC_TMU_REG_COMMON_TIMER_VAL_0_REG                 (MAC_TMU_REG_BASE + 0x168)  /* ͨ�ö�ʱ����ʱֵ���� */

#define BISHENG_W_SOFT_INT_SET_AON_REG            (BISHENG_GLB_CTL_RB_BASE_BASE + 0x134)

/* MAC ͨ�ö�ʱ�� */
#define BISHENG_MAC_COMMON_TIMER_OFFSET         0x4
#define BISHENG_MAC_MAX_COMMON_TIMER 31 /* ͨ�ö�ʱ������ */

void bisheng_host_mac_irq_mask(hal_host_device_stru *hal_device, uint32_t irq);
void bisheng_host_mac_irq_unmask(hal_host_device_stru *hal_device, uint32_t irq);
void bisheng_clear_host_mac_int_status(hal_host_device_stru *hal_device, uint32_t status);
void bisheng_get_host_mac_int_mask(hal_host_device_stru *hal_device, uint32_t *p_mask);
void bisheng_get_host_mac_int_status(hal_host_device_stru *hal_device, uint32_t *p_status);
uint32_t bisheng_regs_addr_transfer(hal_host_device_stru *hal_device, uint32_t reg_addr);
void bisheng_clear_host_phy_int_status(hal_host_device_stru *hal_device, uint32_t status);
void bisheng_get_host_phy_int_mask(hal_host_device_stru *hal_device, uint32_t *p_mask);
void bisheng_get_host_phy_int_status(hal_host_device_stru *hal_device, uint32_t *p_status);
int32_t bisheng_host_regs_addr_init(hal_host_device_stru *hal_device);
uint32_t bisheng_get_dev_rx_filt_mac_status(uint64_t *reg_status);
uint32_t bisheng_regs_addr_get_offset(uint8_t device_id, uint32_t reg_addr);


void bisheng_host_al_rx_fcs_info(hmac_vap_stru *hmac_vap);
void bisheng_host_get_rx_pckt_info(hmac_vap_stru *hmac_vap,
    dmac_atcmdsrv_atcmd_response_event *rx_pkcg_event_info);
int32_t bisheng_host_init_common_timer(hal_mac_common_timer *mac_timer);
void bisheng_host_set_mac_common_timer(hal_mac_common_timer *mac_common_timer);
#endif
