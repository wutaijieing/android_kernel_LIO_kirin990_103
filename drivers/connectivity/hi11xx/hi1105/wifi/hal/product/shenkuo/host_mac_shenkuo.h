

#ifndef __HOST_MAC_SHENKUO_H__
#define __HOST_MAC_SHENKUO_H__

#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "hal_common.h"
#include "host_hal_device.h"

#if (_PRE_TEST_MODE == _PRE_TEST_MODE_ST)
#define SHENKUO_MAC_CFG_HOST_REG_BUTT       0xBC
#define SHENKUO_MAC_RPT_HOST_REG_BUTT       0xBC
#define SHENKUO_DFX_REG_BANK_BUTT           0xBC
#define SHENKUO_MAC_CFG_DMAC_BANK0_REG_BUTT 0xBC
#define SHENKUO_MAC_RPT_DMAC_REG_BUTT       0xBC
#define SHENKUO_MAC_CFG_DMAC_BANK4_REG_BUTT 0xBC
#define SHENKUO_RX_CHN_EST_REG_BANK_BUTT    0xBC
#define SHENKUO_RF_CTRL_REG_BANK_BUTT       0xBC
#define SHENKUO_CALI_TEST_REG_BANK_BUTT     0xBC
#define SHENKUO_PHY_GLB_REG_BANK_BUTT       0xBC
#define SHENKUO_RX_AGC_REG_BANK_BUTT        0xBC
#define SHENKUO_GLB_CTL_RB_REG_BANK_BUTT    0x200
#define SHENKUO_MAC_RPT_SMAC_REG_BANK_BUTT 0x2A0

uint32_t g_st_host_shenkuo_mac_cfg_host_reg_base[SHENKUO_MAC_CFG_HOST_REG_BUTT];
uint32_t g_st_host_shenkuo_mac_rpt_host_reg_base[SHENKUO_MAC_RPT_HOST_REG_BUTT];
uint32_t g_st_host_shenkuo_dfx_reg_bank_base[SHENKUO_DFX_REG_BANK_BUTT];
uint32_t g_st_host_shenkuo_mac_cfg_dmac_bank0_reg_base[SHENKUO_MAC_CFG_DMAC_BANK0_REG_BUTT];
uint32_t g_st_host_shenkuo_mac_rpt_dmac_reg_base[SHENKUO_MAC_RPT_DMAC_REG_BUTT];
uint32_t g_st_host_shenkuo_mac_cfg_dmac_bank4_reg_base[SHENKUO_MAC_CFG_DMAC_BANK4_REG_BUTT];
uint32_t g_st_host_shenkuo_rx_chn_est_reg_band_base[SHENKUO_RX_CHN_EST_REG_BANK_BUTT];
uint32_t shenkuo_rf_ctrl_reg_bank_0_base[SHENKUO_RF_CTRL_REG_BANK_BUTT];
uint32_t shenkuo_cali_test_reg_bank_0_base[SHENKUO_CALI_TEST_REG_BANK_BUTT];
uint32_t shenkuo_phy_glb_reg_bank_base[SHENKUO_PHY_GLB_REG_BANK_BUTT];
uint32_t shenkuo_rx_agc_reg_bank_base[SHENKUO_RX_AGC_REG_BANK_BUTT];
uint32_t shenkuo_glb_ctl_rb_reg_bank_base[SHENKUO_GLB_CTL_RB_REG_BANK_BUTT];
uint32_t shenkuo_mac_rpt_smac_reg_bank_base[SHENKUO_MAC_RPT_SMAC_REG_BANK_BUTT];

#define SHENKUO_MAC_CFG_HOST_REG_BASE             ((uint32_t)&g_st_host_shenkuo_mac_cfg_host_reg_base[0]) /* STʹ�� */
#define SHENKUO_MAC_RPT_HOST_REG_BASE             ((uint32_t)&g_st_host_shenkuo_mac_rpt_host_reg_base[0])
#define SHENKUO_DFX_REG_BANK_BASE                 ((uint32_t)&g_st_host_shenkuo_dfx_reg_bank_base[0])
#define SHENKUO_MAC_CFG_DMAC_BANK0_REG_BASE       ((uint32_t)&g_st_host_shenkuo_mac_cfg_dmac_bank0_reg_base[0])
#define SHENKUO_MAC_RPT_DMAC_REG_BASE             ((uint32_t)&g_st_host_shenkuo_mac_rpt_dmac_reg_base[0])
#define SHENKUO_MAC_CFG_DMAC_BANK4_REG_BASE       ((uint32_t)&g_st_host_shenkuo_mac_cfg_dmac_bank4_reg_base[0])
#define SHENKUO_RX_CHN_EST_REG_BANK_BASE          ((uint32_t)&g_st_host_shenkuo_rx_chn_est_reg_band_base[0])
#define SHENKUO_RF_CTRL_REG_BANK_0_BASE           ((uint32_t)&shenkuo_rf_ctrl_reg_bank_0_base[0])
#define SHENKUO_CALI_TEST_REG_BANK_0_BASE         ((uint32_t)&shenkuo_cali_test_reg_bank_0_base[0])
#define SHENKUO_PHY_GLB_REG_BANK_BASE             ((uint32_t)&shenkuo_phy_glb_reg_bank_base[0])
#define SHENKUO_RX_AGC_REG_BANK_BASE              ((uint32_t)&shenkuo_rx_agc_reg_bank_base[0])
#define SHENKUO_GLB_CTL_RB_BASE_BASE              ((uint32_t)&shenkuo_glb_ctl_rb_reg_bank_base[0])
#define SHENKUO_MAC_RPT_SMAC_REG_BASE        ((uint32_t)&shenkuo_mac_rpt_smac_reg_bank_base[0])
/* _PRE_TEST_MODE_ST */
#else

#define SHENKUO_MAC_CFG_HOST_REG_BASE                        0x40020000
#define SHENKUO_MAC_RPT_HOST_REG_BASE                        0x40030000
#define SHENKUO_DFX_REG_BANK_BASE                            0x40064000
#define SHENKUO_MAC_CFG_DMAC_BANK0_REG_BASE                  0x40022000
#define SHENKUO_MAC_RPT_DMAC_REG_BASE                        0x40032000
#define SHENKUO_MAC_CFG_DMAC_BANK4_REG_BASE                  0x4002A000
#define SHENKUO_RX_CHN_EST_REG_BANK_BASE                     0x40072800
#define SHENKUO_RF_CTRL_REG_BANK_0_BASE                      0x40064800
#define SHENKUO_CALI_TEST_REG_BANK_0_BASE                    0x40061000
#define SHENKUO_PHY_GLB_REG_BANK_BASE                        0x40060800
#define SHENKUO_GLB_CTL_RB_BASE_BASE                         0x40000000
#define SHENKUO_RX_AGC_REG_BANK_BASE                         0x40062800
#define SHENKUO_MAC_RPT_SMAC_REG_BASE                         0x40034000

/* != _PRE_TEST_MODE_ST */
#endif

/* PHY �Ĵ��� */
#define SHENKUO_MAC_HOST_INTR_CLR_REG                        (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x50)
#define SHENKUO_MAC_HOST_INTR_MASK_REG                       (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x54)
#define SHENKUO_DFX_REG_BANK_PHY_INTR_RPT_REG                (SHENKUO_DFX_REG_BANK_BASE + 0xBC)     /* PHY�ж�״̬ */
#define SHENKUO_DFX_REG_BANK_CFG_INTR_MASK_HOST_REG          (SHENKUO_DFX_REG_BANK_BASE + 0xB0)
#define SHENKUO_DFX_REG_BANK_CFG_INTR_CLR_REG                (SHENKUO_DFX_REG_BANK_BASE + 0xB8)
#define SHENKUO_PHY_GLB_REG_BANK_FTM_CFG_REG                 (SHENKUO_PHY_GLB_REG_BANK_BASE + 0x38) /* FTM���üĴ��� */
#define SHENKUO_RX_AGC_REG_BANK_CFG_AGC_WEAK_SIG_EN_REG      (SHENKUO_RX_AGC_REG_BANK_BASE + 0x1F4)
/* �ŵ�����ͨ�üĴ��� */
#define SHENKUO_RX_CHN_EST_REG_BANK_CHN_EST_COM_REG          (SHENKUO_RX_CHN_EST_REG_BANK_BASE + 0x1B0)
#define SHENKUO_RF_CTRL_REG_BANK_0_CFG_ONLINE_CALI_USE_RF_REG_EN_REG     (SHENKUO_RF_CTRL_REG_BANK_0_BASE + 0x34)
/* FIFOʹ�ܶ�Ӧ�ļĴ������õ�ַ */
#define SHENKUO_CALI_TEST_REG_BANK_0_FIFO_FORCE_EN_REG       (SHENKUO_CALI_TEST_REG_BANK_0_BASE + 0x28)
#define SHENKUO_PHY_GLB_REG_BANK_PHY_BW_MODE_REG        (SHENKUO_PHY_GLB_REG_BANK_BASE + 0x4) /* Ƶ��ģʽ���üĴ��� */

#define SHENKUO_MAC_RPT_SMAC_INTR_CNT_REG                   (SHENKUO_MAC_RPT_SMAC_REG_BASE + 0x2A0) /* SMAC�жϼ����� */

#define SHENKUO_MAC_HOST_INTR_STATUS_REG                    (SHENKUO_MAC_RPT_HOST_REG_BASE + 0x0)  /* Host���ж�״̬ */
/* BA INFO BUFFER��дָ���ϱ� */
#define SHENKUO_MAC_BA_INFO_BUF_WPTR_REG                    (SHENKUO_MAC_RPT_HOST_REG_BASE + 0x4)
#define SHENKUO_MAC_RX_NORM_DATA_FREE_RING_RPTR_REG         (SHENKUO_MAC_RPT_HOST_REG_BASE + 0x8)  /* rx-ndata-frrptr */
#define SHENKUO_MAC_RX_SMALL_DATA_FREE_RING_RPTR_REG        (SHENKUO_MAC_RPT_HOST_REG_BASE + 0xC)  /* rx-sdata-frrptr */
#define SHENKUO_MAC_RX_DATA_CMP_RING_WPTR_REG               (SHENKUO_MAC_RPT_HOST_REG_BASE + 0x10)
#define SHENKUO_MAC_RPT_HOST_FREE_RING_STATUS_REG           (SHENKUO_MAC_RPT_HOST_REG_BASE + 0x14) /* F RING�ϱ���Ϣ */
#define SHENKUO_MAC_RX_PPDU_HOST_DESC_FREE_RING_RPTR_REG  (SHENKUO_MAC_RPT_HOST_REG_BASE + 0x18)
#define SHENKUO_MAC_RPT_PPDU_HOST_FREE_RING_STATUS_REG    (SHENKUO_MAC_RPT_HOST_REG_BASE + 0x1C)
/* TX BA INFO BUFFER������üĴ��� */
#define SHENKUO_MAC_TX_BA_INFO_BUF_DEPTH_REG                 (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x0)
/* TX BA INFO BUFFER����ַ���üĴ��� */
#define SHENKUO_MAC_TX_BA_INFO_BUF_ADDR_LSB_REG              (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x4)
/* TX BA INFO BUFFER����ַ���üĴ��� */
#define SHENKUO_MAC_TX_BA_INFO_BUF_ADDR_MSB_REG              (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x8)
/* TX BA INFO WPTR BUFFER����ַ���üĴ��� */
#define SHENKUO_MAC_TX_BA_INFO_WPTR_REG                      (SHENKUO_MAC_CFG_HOST_REG_BASE + 0xC)
/* TX BA INFO RPTR BUFFER����ַ���üĴ��� */
#define SHENKUO_MAC_TX_BA_INFO_RPTR_REG                      (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x10)
#define SHENKUO_MAC_RX_DATA_BUFF_LEN_REG                     (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x1C)
#define SHENKUO_MAC_RX_NORM_DATA_FREE_RING_ADDR_LSB_REG      (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x20)
#define SHENKUO_MAC_RX_NORM_DATA_FREE_RING_ADDR_MSB_REG      (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x24)
#define SHENKUO_MAC_RX_SMALL_DATA_FREE_RING_ADDR_LSB_REG     (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x28)
#define SHENKUO_MAC_RX_SMALL_DATA_FREE_RING_ADDR_MSB_REG     (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x2C)
#define SHENKUO_MAC_RX_DATA_CMP_RING_ADDR_LSB_REG            (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x30)
#define SHENKUO_MAC_RX_DATA_CMP_RING_ADDR_MSB_REG            (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x34)
#define SHENKUO_MAC_RX_NORM_DATA_FREE_RING_SIZE_REG          (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x38)
#define SHENKUO_MAC_RX_SMALL_DATA_FREE_RING_SIZE_REG         (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x3C)
#define SHENKUO_MAC_RX_DATA_CMP_RING_SIZE_REG                (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x40)
#define SHENKUO_MAC_RX_NORM_DATA_FREE_RING_WPTR_REG          (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x44)
#define SHENKUO_MAC_RX_SMALL_DATA_FREE_RING_WPTR_REG         (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x48)
#define SHENKUO_MAC_RX_DATA_CMP_RING_RPTR_REG                (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x4C)
#define SHENKUO_MAC_HOST_INTR_CLR_REG                        (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x50) /* Host�ж���� */
#define SHENKUO_MAC_HOST_INTR_MASK_REG                       (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x54) /* Host�ж����� */
#define SHENKUO_MAC_RX_DATA_CMP_RING_WPTR_CFG_REG            (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x58)
#define SHENKUO_MAC_RX_NORM_DATA_FREE_RING_RPTR_CFG_REG      (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x5C)
#define SHENKUO_MAC_RX_SMALL_DATA_FREE_RING_RPTR_CFG_REG     (SHENKUO_MAC_CFG_HOST_REG_BASE + 0x60)
/* RX���˿��ƼĴ��� */
#define SHENKUO_MAC_RX_FRAMEFILT_REG                         (SHENKUO_MAC_CFG_DMAC_BANK0_REG_BASE + 0x54)
/* RX���˿��ƼĴ���2 */
#define SHENKUO_MAC_RX_FRAMEFILT2_REG                        (SHENKUO_MAC_CFG_DMAC_BANK0_REG_BASE + 0x58)

/* DMAC�ж����μĴ��� */
#define SHENKUO_MAC_DMAC_INTR_MASK_REG                       (SHENKUO_MAC_CFG_DMAC_BANK0_REG_BASE + 0x4)
/* ���տ��ƼĴ��� */
#define SHENKUO_MAC_RX_CTRL_REG                              (SHENKUO_MAC_CFG_DMAC_BANK0_REG_BASE + 0x20)
/* CSI�����ϱ����üĴ��� */
#define SHENKUO_MAC_CSI_PROCESS_REG                          (SHENKUO_MAC_CFG_DMAC_BANK4_REG_BASE + 0x0)
/* CSI�����ϱ�Ƭ��ѭ��BUF����ַLSB */
#define SHENKUO_MAC_CSI_BUF_BASE_ADDR_LSB_REG                (SHENKUO_MAC_CFG_DMAC_BANK4_REG_BASE + 0x4)
/* CSI�����ϱ�Ƭ��ѭ��BUF����ַMSB */
#define SHENKUO_MAC_CSI_BUF_BASE_ADDR_MSB_REG                (SHENKUO_MAC_CFG_DMAC_BANK4_REG_BASE + 0x8)
/* CSI�����ϱ�Ƭ��ѭ��BUF��С */
#define SHENKUO_MAC_CSI_BUF_SIZE_REG                         (SHENKUO_MAC_CFG_DMAC_BANK4_REG_BASE + 0xC)
/* CSI������������32λ */
#define SHENKUO_MAC_CSI_WHITELIST_ADDR_LSB_0_REG             (SHENKUO_MAC_CFG_DMAC_BANK4_REG_BASE + 0x10)
/* CSI������������16λ */
#define SHENKUO_MAC_CSI_WHITELIST_ADDR_MSB_0_REG             (SHENKUO_MAC_CFG_DMAC_BANK4_REG_BASE + 0x14)
/* ��λ��Ϣ�ϱ� */
#define SHENKUO_MAC_LOCATION_INFO_MASK_REG                   (SHENKUO_MAC_RPT_DMAC_REG_BASE + 0x70)
/* CSI�ϱ���ַ��32bit */
#define SHENKUO_MAC_CSI_INFO_ADDR_MSB_REG                    (SHENKUO_MAC_RPT_DMAC_REG_BASE + 0x1B0)
/* CSI�ϱ���ַ��32bit */
#define SHENKUO_MAC_CSI_INFO_ADDR_LSB_REG                    (SHENKUO_MAC_RPT_DMAC_REG_BASE + 0x1B4)
/* FTM�ϱ���ַ��32bit */
#define SHENKUO_MAC_FTM_INFO_ADDR_MSB_REG                    (SHENKUO_MAC_RPT_DMAC_REG_BASE + 0x1B8)
/* FTM�ϱ���ַ��32bit */
#define SHENKUO_MAC_FTM_INFO_ADDR_LSB_REG                    (SHENKUO_MAC_RPT_DMAC_REG_BASE + 0x1BC)

/* VAP��ǰ��Timestamp״ֵ̬����32bit������λus */
#define SHENKUO_MAC_VAP_TSFTIMER_RDVALL_STATUS_12_REG      (SHENKUO_MAC_RPT_DMAC_REG_BASE + 0x140)
/* ͨ�ö�ʱ��״̬�Ĵ��� */
#define SHENKUO_MAC_SMAC_COMMON_TIMER_STATUS_REG         (SHENKUO_MAC_RPT_SMAC_REG_BASE + 0x10)

/* FTM�����ϱ����üĴ��� */
#define SHENKUO_MAC_FTM_PROCESS_REG                          (SHENKUO_MAC_CFG_DMAC_BANK4_REG_BASE + 0x110)
/* FTM�����ϱ�Ƭ��ѭ��BUF����ַLSB */
#define SHENKUO_MAC_FTM_BUF_BASE_ADDR_LSB_REG                (SHENKUO_MAC_CFG_DMAC_BANK4_REG_BASE + 0x114)
/* FTM�����ϱ�Ƭ��ѭ��BUF����ַMSB */
#define SHENKUO_MAC_FTM_BUF_BASE_ADDR_MSB_REG                (SHENKUO_MAC_CFG_DMAC_BANK4_REG_BASE + 0x118)
/* FTM������������32λ */
#define SHENKUO_MAC_FTM_WHITELIST_ADDR_LSB_0_REG             (SHENKUO_MAC_CFG_DMAC_BANK4_REG_BASE + 0x12C)
/* FTM������������16λ */
#define SHENKUO_MAC_FTM_WHITELIST_ADDR_MSB_0_REG             (SHENKUO_MAC_CFG_DMAC_BANK4_REG_BASE + 0x130)

/* master: 0x4002_0000~0x4003_ffff slave: 0x4004_0000~0x4005_ffff */
#define SHENKUO_MAC_BANK_REG_OFFSET 0x20000
/* fpga �׶���δ������ƫ�� �����޸����� */
#define SHENKUO_PHY_BANK_REG_OFFSET 0x40000

/* DMACͨ�ü������ж����μĴ��� */
#define SHENKUO_MAC_DMAC_COMMON_TIMER_MASK_REG       (SHENKUO_MAC_CFG_DMAC_BANK0_REG_BASE + 0x124)
/* ͨ�ö�ʱ����ز������� */
#define SHENKUO_MAC_COMMON_TIMER_CTRL_0_REG          (SHENKUO_MAC_CFG_DMAC_BANK0_REG_BASE + 0x128)
/* ͨ�ö�ʱ����ʱֵ���� */
#define SHENKUO_MAC_COMMON_TIMER_VAL_0_REG           (SHENKUO_MAC_CFG_DMAC_BANK0_REG_BASE + 0x1A8)
#define SHENKUO_W_SOFT_INT_SET_AON_REG               (SHENKUO_GLB_CTL_RB_BASE_BASE + 0x104)

#define SHENKUO_MAC_CFG_SMAC_REG_BASE          0x4002E000
#define SHENKUO_MAC_CFG_DMAC_BANK2_REG_BASE    0x40026000
#define SHENKUO_MAC_HWMAC_STARTUP_REG          (SHENKUO_MAC_CFG_SMAC_REG_BASE + 0x140) /* HWMAC�������üĴ��� */
#define SHENKUO_MAC_COEX_ABORT_CTRL_REG        (SHENKUO_MAC_CFG_DMAC_BANK2_REG_BASE + 0x300) /* ������ƼĴ��� */
#define SHENKUO_MAC_CFG_COEX_ABORT_EN_MASK      0x1
#define SHENKUO_MAC_CFG_HWMAC_STARTUP_EN_MASK   0x1
#define SHENKUO_MAC_COMMON_TIMER_OFFSET         0x4

#define SHENKUO_MAC_MAX_COMMON_TIMER 31 /* ͨ�ö�ʱ������ */

void shenkuo_host_mac_irq_mask(hal_host_device_stru *hal_device, uint32_t irq);
void shenkuo_host_mac_irq_unmask(hal_host_device_stru *hal_device, uint32_t irq);
void shenkuo_clear_host_mac_int_status(hal_host_device_stru *hal_device, uint32_t status);
void shenkuo_get_host_mac_int_mask(hal_host_device_stru *hal_device, uint32_t *p_mask);
void shenkuo_get_host_mac_int_status(hal_host_device_stru *hal_device, uint32_t *p_status);
uint32_t shenkuo_regs_addr_transfer(hal_host_device_stru *hal_device, uint32_t reg_addr);
void shenkuo_clear_host_phy_int_status(hal_host_device_stru *hal_device, uint32_t status);
void shenkuo_get_host_phy_int_mask(hal_host_device_stru *hal_device, uint32_t *p_mask);
void shenkuo_get_host_phy_int_status(hal_host_device_stru *hal_device, uint32_t *p_status);
int32_t shenkuo_host_regs_addr_init(hal_host_device_stru *hal_device);
uint32_t shenkuo_get_dev_rx_filt_mac_status(uint64_t *reg_status);
uint32_t shenkuo_regs_addr_get_offset(uint8_t device_id, uint32_t reg_addr);
uint32_t shenkuo_get_host_mac_common_timer_status(hal_host_device_stru *hal_device);
hal_mac_common_timer *shenkuo_get_host_mac_common_timer_ptr(uint8_t id);
#endif
