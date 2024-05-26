/*
 * smmu_regs.h
 *
 * This is for smmu_regs description.
 *
 * Copyright (c) 2010-2020 Huawei Technologies CO., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __VCODEC_VENC_SMMU_REGS_H__
#define __VCODEC_VENC_SMMU_REGS_H__

/* SMMU TBU regs offset */
#define SMMU_TBU_CR               0x0000
#define SMMU_TBU_CRACK            0x0004
#define SMMU_TBU_IRPT_MASK_NS     0x0010
#define SMMU_TBU_IRPT_CLR_NS      0x001C
#define SMMU_TBU_SWID_CFG_N       0x0100
#define SMMU_TBU_SCR              0x1000
#define SMMU_TBU_IRPT_MASK_S      0x1010
#define SMMU_TBU_IRPT_CLR_S       0x101C
#define SMMU_TBU_PROT_EN_N        0x1100

#endif
