/*
 * smmu_regs.h
 *
 * This is for smmu reg desc.
 *
 * Copyright (c) 2017-2020 Huawei Technologies CO., Ltd.
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

#ifndef VCODEC_VDEC_SMMU_REGS_H
#define VCODEC_VDEC_SMMU_REGS_H

/* SMMU sid regs offset */
#define SMMU_NORM_RSID            0x0
#define SMMU_NORM_RSSID           0x4
#define SMMU_NORM_RSSIDV          0x8

#define SMMU_NORM_WSID            0x10
#define SMMU_NORM_WSSID           0x14
#define SMMU_NORM_WSSIDV          0x18

/* SMMU TBU regs offset */
#define SMMU_TBU_SCR              0x1000
#define SMMU_TBU_CRACK            0x0004
#define SMMU_TBU_CR               0x0000
#define SMMU_TBU_SWID_CFGN        0x0100

#define SMMU_TBU_PROT_EN          0x1100
#define SMMU_TBU_IRPT_CLR_NS      0x001C
#define SMMU_TBU_IRPT_MASK_NS     0x0010
#define SMMU_TBU_IRPT_CLR_S       0x101C
#define SMMU_TBU_IRPT_MASK_S      0x1010

#define SMMU_TBU_PMCG_SMR         0x2A00
#define SMMU_TBU_PMCG_CR          0x2E04
#define SMMU_TBU_PMCG_CAPR        0x2D88
#define SMMU_TBU_PMCG_EVCNTR      0x2000
#define SMMU_TBU_PMCG_CNTENSET0_0 0x2C00
#define SMMU_TBU_SCR              0x1000
#define SMMU_TBU_PMCG_EVTYPERN    0x2404

#endif

