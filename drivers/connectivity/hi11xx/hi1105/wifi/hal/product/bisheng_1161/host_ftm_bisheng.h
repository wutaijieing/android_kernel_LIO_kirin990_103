
#ifndef HOST_FTM_BISHENG_H
#define HOST_FTM_BISHENG_H

#ifdef _PRE_WLAN_FEATURE_FTM

#include "hmac_user.h"
#include "hmac_vap.h"
#include "host_mac_bisheng.h"
#include "host_dscr_bisheng.h"
#include "wlan_chip.h"

#define MAC_PPU_REG_CFG_FTM_EN_LEN	1
#define MAC_PPU_REG_CFG_FTM_EN_OFFSET	0
#define MAC_PPU_REG_CFG_FTM_EN_MASK  0x1

#define CALI_TEST_REG_BANK_0_CFG_FTM_EN_LEN	1
#define CALI_TEST_REG_BANK_0_CFG_FTM_EN_OFFSET	0
#define CALI_TEST_REG_BANK_0_CFG_FTM_EN_MASK  0x1

#define RF_CTRL_REG_BANK_0_CFG_FTM_CALI_WR_RF_REG_EN_LEN	1
#define RF_CTRL_REG_BANK_0_CFG_FTM_CALI_WR_RF_REG_EN_OFFSET	1
#define RF_CTRL_REG_BANK_0_CFG_FTM_CALI_WR_RF_REG_EN_MASK  0x2

#define CALI_TEST_REG_BANK_0_CFG_FTM_CALI_CH_SEL_LEN	2
#define CALI_TEST_REG_BANK_0_CFG_FTM_CALI_CH_SEL_OFFSET	16
#define CALI_TEST_REG_BANK_0_CFG_FTM_CALI_CH_SEL_MASK  0x30000

#define CALI_TEST_REG_BANK_0_CFG_REG_TX_FIFO_EN_LEN	4
#define CALI_TEST_REG_BANK_0_CFG_REG_TX_FIFO_EN_OFFSET	0
#define CALI_TEST_REG_BANK_0_CFG_REG_TX_FIFO_EN_MASK  0xF

#define MAC_PPU_REG_CFG_FTM_BUF_SIZE_LEN	16
#define MAC_PPU_REG_CFG_FTM_BUF_SIZE_OFFSET	4
#define MAC_PPU_REG_CFG_FTM_BUF_SIZE_MASK  0xFFFF0

#define MAC_GLB_REG_RPT_DMAC_INTR_LOCATION_COMPLETE_LEN	1
#define MAC_GLB_REG_RPT_DMAC_INTR_LOCATION_COMPLETE_OFFSET	29
#define MAC_GLB_REG_RPT_DMAC_INTR_LOCATION_COMPLETE_MASK  0x20000000

#define FTM_WHITELIST_ADDR_SIZE    32
#define DIVIDER_1CLK    1
#define DIVIDER_2CLK    2
#define DIVIDER_4CLK    4

typedef enum {
    FTM_INITIATOR_CAP = BIT0,
    FTM_RESPONDER_CAP = BIT1,
    FTM_RANGE_CAP = BIT2,
    FTM_SWITCH_CAP = BIT3,
    HAL_FTM_CAP_BUTT
} hal_device_ftm_cap_enum;

uint32_t bisheng_host_ftm_reg_init(uint8_t hal_dev_id);

hmac_vap_stru *bisheng_host_vap_get_by_hal_vap_id(uint8_t hal_device_id, uint8_t hal_vap_id);

uint32_t bisheng_host_ftm_get_info(hal_ftm_info_stru *hal_ftm_info, uint8_t *addr);

void bisheng_host_ftm_get_divider(hal_host_device_stru *hal_device, uint8_t *divider);

void bisheng_host_ftm_set_sample(hal_host_device_stru *hal_device, oal_bool_enum_uint8 ftm_status);

void bisheng_host_ftm_set_enable(hal_host_device_stru *hal_device, oal_bool_enum_uint8 ftm_status);

void bisheng_host_ftm_set_m2s_phy(hal_host_device_stru *hal_device, uint8_t tx_chain_selection,
    uint8_t tx_rssi_selection);

void bisheng_host_ftm_set_buf_base_addr(hal_host_device_stru *hal_device, uint64_t devva);

void bisheng_host_ftm_set_buf_size(hal_host_device_stru *hal_device, uint16_t cfg_ftm_buf_size);

uint32_t bisheng_host_ftm_set_white_list(hal_host_device_stru *hal_device, uint8_t idx, uint8_t *mac_addr);

#endif

#endif