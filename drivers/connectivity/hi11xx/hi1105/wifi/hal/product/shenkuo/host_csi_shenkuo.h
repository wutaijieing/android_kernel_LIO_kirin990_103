
#ifndef HOST_CSI_SHENKUO_H
#define HOST_CSI_SHENKUO_H

#include "hmac_user.h"
#include "hmac_vap.h"
#include "host_mac_shenkuo.h"
#include "host_dscr_shenkuo.h"
#include "wlan_chip.h"

#define SHENKUO_MAC_CFG_CSI_SAMPLE_PERIOD_LEN      4
#define SHENKUO_MAC_CFG_CSI_SAMPLE_PERIOD_OFFSET   24
#define SHENKUO_MAC_CFG_CSI_SAMPLE_PERIOD_MASK     0xF000000
#define SHENKUO_MAC_CFG_CSI_BAND_WIDTH_MASK_LEN    4
#define SHENKUO_MAC_CFG_CSI_BAND_WIDTH_MASK_OFFSET 20
#define SHENKUO_MAC_CFG_CSI_BAND_WIDTH_MASK_MASK   0xF00000
#define SHENKUO_MAC_CFG_CSI_FRAME_TYPE_MASK_LEN    4
#define SHENKUO_MAC_CFG_CSI_FRAME_TYPE_MASK_OFFSET 16
#define SHENKUO_MAC_CFG_CSI_FRAME_TYPE_MASK_MASK   0xF0000
#define SHENKUO_MAC_CFG_CSI_H_MATRIX_RPT_EN_LEN    1
#define SHENKUO_MAC_CFG_CSI_H_MATRIX_RPT_EN_OFFSET 1
#define SHENKUO_MAC_CFG_CSI_H_MATRIX_RPT_EN_MASK   0x2
#define SHENKUO_MAC_CFG_CSI_EN_LEN                 1
#define SHENKUO_MAC_CFG_CSI_EN_OFFSET              0
#define SHENKUO_MAC_CFG_CSI_EN_MASK                0x1

#define SHENKUO_MAC_CFG_RX_PPDU_DESC_MODE_LEN                2
#define SHENKUO_MAC_CFG_RX_PPDU_DESC_MODE_OFFSET             16
#define SHENKUO_MAC_CFG_RX_PPDU_DESC_MODE_MASK               0x30000

#define SHENKUO_RX_CHN_EST_REG_BANK_CFG_CHN_CONDITION_NUM_RPT_EN_LEN     1
#define SHENKUO_RX_CHN_EST_REG_BANK_CFG_CHN_CONDITION_NUM_RPT_EN_OFFSET  10
#define SHENKUO_RX_CHN_EST_REG_BANK_CFG_CHN_CONDITION_NUM_RPT_EN_MASK    0x400

#define SHENKUO_MAC_RPT_DMAC_INTR_LOCATION_COMPLETE_LEN        1
#define SHENKUO_MAC_RPT_DMAC_INTR_LOCATION_COMPLETE_OFFSET     29
#define SHENKUO_MAC_RPT_DMAC_INTR_LOCATION_COMPLETE_MASK       0x20000000

#define SHENKUO_MAC_RPT_LOCATION_INFO_MASK_LEN                 2
#define SHENKUO_MAC_RPT_LOCATION_INFO_MASK_OFFSET              0
#define SHENKUO_MAC_RPT_LOCATION_INFO_MASK_MASK                0x3

#define HMAC_CSI_BUFF_SIZE (1024 * 15) /* 4x4 最大27104字节，160M抽取减半 */

#ifdef _PRE_WLAN_FEATURE_CSI

uint32_t shenkuo_get_host_ftm_csi_locationinfo(hal_host_device_stru *hal_device);
uint32_t shenkuo_get_csi_info(hmac_csi_info_stru *hmac_csi_info, uint8_t *addr);
uint32_t shenkuo_csi_config(mac_vap_stru *mac_vap, uint16_t len, uint8_t *param);
uint32_t shenkuo_host_ftm_csi_init(hal_host_device_stru *hal_device);
#endif
#endif