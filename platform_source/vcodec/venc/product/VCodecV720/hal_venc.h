/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: venc register config
 * Create: 2021-12-06
 */
#ifndef __HAL_VENC_H__
#define __HAL_VENC_H__

#include "vcodec_type.h"
#include "drv_venc_ioctl.h"
#include "smmu.h"
#include "hal_cmdlist.h"
#include "hal_common.h"

#ifdef PCIE_UDP
#define VENC_COMPATIBLE       "hisilicon,VCodecV600-venc"
#else
#define VENC_COMPATIBLE       "hisilicon,VCodecV720-venc"
#endif

#define VENC_CS_SUPPORT
#define VENC_FPGAFLAG_CS      "fpga_cs"
#define VENC_PCTRL_PERI_CS    0xFEC3E0BC
#define VENC_EXISTBIT_CS      0x18
#define VENC_EXIST_TRUE       0x8
#define AXIDFX_OFFSET (offsetof(S_HEVC_AVC_REGS_TYPE, AXIDFX_REGS))

void venc_hal_clr_all_int(S_HEVC_AVC_REGS_TYPE *vedu_reg);
void venc_hal_start_encode(S_HEVC_AVC_REGS_TYPE *vedu_reg);
int32_t venc_mcore_start_encode(struct encode_info *info);
int32_t venc_mcore_cmdlist_start_encode(void);
void venc_hal_get_reg_venc(struct stream_info *stream_info, uint32_t *reg_base);
void venc_hal_get_reg_stream_len(struct stream_info *stream_info, uint32_t *reg_base);
void venc_hal_cfg_curld_osd01(struct encode_info *channel_cfg, uint32_t *reg_base);
void venc_hal_disable_all_int(S_HEVC_AVC_REGS_TYPE *vedu_reg);
void venc_hal_get_slice_reg(struct stream_info *stream_info, uint32_t *reg_base);
void venc_hal_cfg_platform_diff(struct encode_info *channel_cfg, uint32_t *reg_base);
bool venc_device_need_bypass(void);

#endif
