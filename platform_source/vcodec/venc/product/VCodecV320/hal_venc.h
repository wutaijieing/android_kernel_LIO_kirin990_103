/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2022. All rights reserved.
 * Description: venc register config
 * Create: 2022-01-28
 */
#ifndef __HAL_VENC_H__
#define __HAL_VENC_H__

#include "vcodec_type.h"
#include "drv_venc_ioctl.h"
#include "smmu.h"

#define VENC_COMPATIBLE       "hisilicon,VCodecV320-venc"
#define VENC_CS_SUPPORT
#define VENC_FPGAFLAG_CS      "venc_fpga"
#define VENC_PCTRL_PERI_CS    0xEE58F018
#define VENC_PCTRL_PERI_MSK   0xEE58F804
#define VENC_PCTRL_PERI_MSK_DATA   0x3
#define VENC_EXISTBIT_CS      0x20

void venc_hal_clr_all_int(S_HEVC_AVC_REGS_TYPE *vedu_reg);
void venc_hal_disable_all_int(S_HEVC_AVC_REGS_TYPE *vedu_reg);
void venc_hal_start_encode(S_HEVC_AVC_REGS_TYPE *vedu_reg);
void venc_hal_get_reg_venc(struct stream_info *stream_info, uint32_t *reg_base);
void vedu_hal_cfg_reg_intraset(const struct encode_info *channel_cfg, uint32_t *reg_base);
void vedu_hal_cfg_reg_lambda_set(const struct encode_info *channel_cfg, uint32_t *reg_base);
void vedu_hal_cfg_reg_qpg_map_set(const struct encode_info *channel_cfg, uint32_t *reg_base);
void vedu_hal_cfg_reg_addr_set(const struct encode_info *channel_cfg, uint32_t *reg_base);
void vedu_hal_cfg_reg_slc_head_set(const struct encode_info *channel_cfg, uint32_t *reg_base);
void vedu_hal_cfg_reg_simple(struct encode_info *channel_cfg, int32_t core_id);
void vedu_hal_cfg_reg(struct encode_info *regcfginfo, int32_t core_id);
bool venc_device_need_bypass(void);

#endif
