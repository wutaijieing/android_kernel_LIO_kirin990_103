/** @file
 * Copyright (c) 2020-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef HISI_DISP_SMMU_H
#define HISI_DISP_SMMU_H

#define void_unused(x)    ((void)(x))

#define SMMU_TIMEOUT 1000   /* us */
#define DSS_TBU0_DTI_NUMS 0x17
#define DSS_TBU1_DTI_NUMS 0x0F
#define MDC_TBU_DTI_NUMS 0x0B
#define MDC_SWID_NUMS 33

#define HISI_DSS_SSID_MAX_NUM 3
#define HISI_PRIMARY_DSS_SSID_OFFSET 0
#define HISI_EXTERNAL_DSS_SSID_OFFSET 3
#ifdef CONFIG_DKMD_DPU_V720
#define HISI_OFFLINE_SSID 6
#else
#define HISI_OFFLINE_SSID 16
#endif

typedef union {
	unsigned int value;
	struct {
		unsigned int ch_sid          : 8;  /* bit[7:0]  */
		unsigned int reserved_0      : 3;  /* bit[10:8] */
		unsigned int ch_ssid         : 8;  /* bit[18:11]  */
		unsigned int ch_ssidv        : 1;  /* bit[19] */
		unsigned int ch_sc_hint      : 4;  /* bit[23:20] */
		unsigned int ch_gid          : 4;  /* bit[27:24] */
		unsigned int ch_gid_en       : 1;  /* bit[28] */
		unsigned int ch_ptl_as_ful   : 1;  /* bit[29] */
		unsigned int reserved_2      : 2;  /* bit[31:30] */
	} reg;
} soc_dss_mmu_id_attr;


enum smmu_event {
	TBU_DISCONNECT = 0x0,
	TBU_CONNECT = 0x1,
};

static inline int hisi_dss_get_ssid(int frame_no)
{
	return frame_no % HISI_DSS_SSID_MAX_NUM;
}

void hisi_dss_smmu_ch_set_reg(char __iomem *dpu_base, uint32_t scene_id, uint32_t layer_num);
void hisi_dss_smmu_wch_set_reg(char __iomem *dpu_base, uint32_t scene_id);
void hisi_dss_smmu_tlb_flush(uint32_t scene_id);
void hisi_disp_smmu_deinit(char __iomem *dpu_base, uint32_t scene_id);
void hisi_disp_smmu_init(char __iomem *dss_base, uint32_t scene_id);
#endif

