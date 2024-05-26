/*
 * IOMMU API for ARM architected SMMUv3 implementations.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2025. All rights reserved.
 *
 * This driver is powered by bad coffee and bombay mix.
 *
 */

#ifndef _PLATFORM_SMMU_V3_H
#define _PLATFORM_SMMU_V3_H

#include "mm_smmu.h"

#define MM_SMMUV3_SUPPORT
/* MMIO registers */
#define ARM_SMMU_IDR0			0x0
#define IDR0_ST_LVL_SHIFT		27
#define IDR0_ST_LVL_MASK		0x3
#define IDR0_ST_LVL_2LVL		(1 << IDR0_ST_LVL_SHIFT)
#define IDR0_STALL_MODEL_SHIFT		24
#define IDR0_STALL_MODEL_MASK		0x3
#define IDR0_STALL_MODEL_STALL		(0 << IDR0_STALL_MODEL_SHIFT)
#define IDR0_STALL_MODEL_FORCE		(2 << IDR0_STALL_MODEL_SHIFT)
#define IDR0_TTENDIAN_SHIFT		21
#define IDR0_TTENDIAN_MASK		0x3
#define IDR0_TTENDIAN_LE		(2 << IDR0_TTENDIAN_SHIFT)
#define IDR0_TTENDIAN_BE		(3 << IDR0_TTENDIAN_SHIFT)
#define IDR0_TTENDIAN_MIXED		(0 << IDR0_TTENDIAN_SHIFT)
#define IDR0_CD2L			(1 << 19)
#define IDR0_VMID16			(1 << 18)
#define IDR0_PRI			(1 << 16)
#define IDR0_SEV			(1 << 14)
#define IDR0_MSI			(1 << 13)
#define IDR0_ASID16			(1 << 12)
#define IDR0_ATS			(1 << 10)
#define IDR0_HYP			(1 << 9)
#define IDR0_COHACC			(1 << 4)
#define IDR0_TTF_SHIFT			2
#define IDR0_TTF_MASK			0x3
#define IDR0_TTF_AARCH64		(2 << IDR0_TTF_SHIFT)
#define IDR0_TTF_AARCH32_64		(3 << IDR0_TTF_SHIFT)
#define IDR0_S1P			(1 << 1)
#define IDR0_S2P			(1 << 0)

#define ARM_SMMU_IDR1			0x4
#define IDR1_TABLES_PRESET		(1 << 30)
#define IDR1_QUEUES_PRESET		(1 << 29)
#define IDR1_REL			(1 << 28)
#define IDR1_CMDQ_SHIFT			21
#define IDR1_CMDQ_MASK			0x1f
#define IDR1_EVTQ_SHIFT			16
#define IDR1_EVTQ_MASK			0x1f
#define IDR1_PRIQ_SHIFT			11
#define IDR1_PRIQ_MASK			0x1f
#define IDR1_SSID_SHIFT			6
#define IDR1_SSID_MASK			0x1f
#define IDR1_SID_SHIFT			0
#define IDR1_SID_MASK			0x3f

#define ARM_SMMU_IDR5			0x14
#define IDR5_STALL_MAX_SHIFT		16
#define IDR5_STALL_MAX_MASK		0xffff
#define IDR5_GRAN64K			(1 << 6)
#define IDR5_GRAN16K			(1 << 5)
#define IDR5_GRAN4K			(1 << 4)
#define IDR5_OAS_SHIFT			0
#define IDR5_OAS_MASK			0x7
#define IDR5_OAS_32_BIT			(0 << IDR5_OAS_SHIFT)
#define IDR5_OAS_36_BIT			(1 << IDR5_OAS_SHIFT)
#define IDR5_OAS_40_BIT			(2 << IDR5_OAS_SHIFT)
#define IDR5_OAS_42_BIT			(3 << IDR5_OAS_SHIFT)
#define IDR5_OAS_44_BIT			(4 << IDR5_OAS_SHIFT)
#define IDR5_OAS_48_BIT			(5 << IDR5_OAS_SHIFT)
#define IDR5_OAS_52_BIT			(6 << IDR5_OAS_SHIFT)
#define IDR5_VAX			GENMASK(11, 10)
#define IDR5_VAX_52_BIT			1

#define ARM_SMMU_CR0			0x20
#define CR0_CMDQEN			(1 << 3)
#define CR0_EVTQEN			(1 << 2)
#define CR0_PRIQEN			(1 << 1)
#define CR0_SMMUEN			(1 << 0)

#define ARM_SMMU_CR0ACK			0x24

#define ARM_SMMU_CR1			0x28
#define CR1_SH_NSH			0
#define CR1_SH_OSH			2
#define CR1_SH_ISH			3
#define CR1_CACHE_NC			0
#define CR1_CACHE_WB			1
#define CR1_CACHE_WT			2
#define CR1_TABLE_SH_SHIFT		10
#define CR1_TABLE_OC_SHIFT		8
#define CR1_TABLE_IC_SHIFT		6
#define CR1_QUEUE_SH_SHIFT		4
#define CR1_QUEUE_OC_SHIFT		2
#define CR1_QUEUE_IC_SHIFT		0

#define ARM_SMMU_CR2			0x2c
#define CR2_PTM				(1 << 2)
#define CR2_RECINVSID			(1 << 1)
#define CR2_E2H				(1 << 0)

#define ARM_SMMU_GBPA			0x44
#define GBPA_ABORT			(1 << 20)
#define GBPA_UPDATE			(1 << 31)

#define ARM_SMMU_IRQ_CTRL		0x50
#define IRQ_CTRL_EVTQ_IRQEN		(1 << 2)
#define IRQ_CTRL_PRIQ_IRQEN		(1 << 1)
#define IRQ_CTRL_GERROR_IRQEN		(1 << 0)

#define ARM_SMMU_IRQ_CTRLACK		0x54

#define ARM_SMMU_GERROR			0x60
#define GERROR_SFM_ERR			(1 << 8)
#define GERROR_MSI_GERROR_ABT_ERR	(1 << 7)
#define GERROR_MSI_PRIQ_ABT_ERR		(1 << 6)
#define GERROR_MSI_EVTQ_ABT_ERR		(1 << 5)
#define GERROR_MSI_CMDQ_ABT_ERR		(1 << 4)
#define GERROR_PRIQ_ABT_ERR		(1 << 3)
#define GERROR_EVTQ_ABT_ERR		(1 << 2)
#define GERROR_CMDQ_ERR			(1 << 0)
#define GERROR_ERR_MASK			0xfd

#define ARM_SMMU_GERRORN		0x64

#define ARM_SMMU_GERROR_IRQ_CFG0	0x68
#define ARM_SMMU_GERROR_IRQ_CFG1	0x70
#define ARM_SMMU_GERROR_IRQ_CFG2	0x74

#define ARM_SMMU_STRTAB_BASE		0x80
#define STRTAB_BASE_RA			(1UL << 62)
#define STRTAB_BASE_ADDR_SHIFT		6
#define STRTAB_BASE_ADDR_MASK		0x3fffffffffffUL

#define ARM_SMMU_STRTAB_BASE_CFG	0x88
#define STRTAB_BASE_CFG_LOG2SIZE_SHIFT	0
#define STRTAB_BASE_CFG_LOG2SIZE_MASK	0x3f
#define STRTAB_BASE_CFG_SPLIT_SHIFT	6
#define STRTAB_BASE_CFG_SPLIT_MASK	0x1f
#define STRTAB_BASE_CFG_FMT_SHIFT	16
#define STRTAB_BASE_CFG_FMT_MASK	0x3
#define STRTAB_BASE_CFG_FMT_LINEAR	(0 << STRTAB_BASE_CFG_FMT_SHIFT)
#define STRTAB_BASE_CFG_FMT_2LVL	(1 << STRTAB_BASE_CFG_FMT_SHIFT)

#define ARM_SMMU_CMDQ_BASE		0x90
#define ARM_SMMU_CMDQ_PROD		0x98
#define ARM_SMMU_CMDQ_CONS		0x9c

#define ARM_SMMU_EVTQ_BASE		0xa0
#define ARM_SMMU_EVTQ_PROD		0x100a8
#define ARM_SMMU_EVTQ_CONS		0x100ac
#define ARM_SMMU_EVTQ_IRQ_CFG0		0xb0
#define ARM_SMMU_EVTQ_IRQ_CFG1		0xb8
#define ARM_SMMU_EVTQ_IRQ_CFG2		0xbc

#define ARM_SMMU_PRIQ_BASE		0xc0
#define ARM_SMMU_PRIQ_PROD		0x100c8
#define ARM_SMMU_PRIQ_CONS		0x100cc
#define ARM_SMMU_PRIQ_IRQ_CFG0		0xd0
#define ARM_SMMU_PRIQ_IRQ_CFG1		0xd8
#define ARM_SMMU_PRIQ_IRQ_CFG2		0xdc

/* Common MSI config fields */
#define MSI_CFG0_ADDR_SHIFT		2
#define MSI_CFG0_ADDR_MASK		0x3ffffffffffffUL
#define MSI_CFG2_SH_SHIFT		4
#define MSI_CFG2_SH_NSH			(0UL << MSI_CFG2_SH_SHIFT)
#define MSI_CFG2_SH_OSH			(2UL << MSI_CFG2_SH_SHIFT)
#define MSI_CFG2_SH_ISH			(3UL << MSI_CFG2_SH_SHIFT)
#define MSI_CFG2_MEMATTR_SHIFT		0
#define MSI_CFG2_MEMATTR_DEVICE_NG_NRE	(0x1 << MSI_CFG2_MEMATTR_SHIFT)

#define Q_IDX(q, p)			((p) & ((1 << (q)->max_n_shift) - 1))
#define Q_WRP(q, p)			((p) & (1 << (q)->max_n_shift))
#define Q_OVERFLOW_FLAG			(1 << 31)
#define Q_OVF(q, p)			((p) & Q_OVERFLOW_FLAG)
#define Q_ENT(q, p)			((q)->base +			\
					 Q_IDX(q, p) * (q)->ent_dwords)

#define Q_BASE_RWA			(1UL << 62)
#define Q_BASE_ADDR_SHIFT		5
#define Q_BASE_ADDR_MASK		0xffffffffffffUL
#define Q_BASE_LOG2SIZE_SHIFT		0
#define Q_BASE_LOG2SIZE_MASK		0x1fUL

/*
 * Stream table.
 *
 * Linear: Enough to cover 1 << IDR1.SIDSIZE entries
 * 2lvl: 128k L1 entries,
 *       256 lazy entries per table (each table covers a PCI bus)
 */
#define STRTAB_L1_SZ_SHIFT		20
#define STRTAB_SPLIT			8

#define STRTAB_L1_DESC_DWORDS		1
#define STRTAB_L1_DESC_SPAN_SHIFT	0
#define STRTAB_L1_DESC_SPAN_MASK	0x1fUL
#define STRTAB_L1_DESC_L2PTR_SHIFT	6
#define STRTAB_L1_DESC_L2PTR_MASK	0x3fffffffffffUL

#define STRTAB_STE_DWORDS		8
#define STRTAB_STE_0_V			(1UL << 0)
#define STRTAB_STE_0_CFG_SHIFT		1
#define STRTAB_STE_0_CFG_MASK		0x7UL
#define STRTAB_STE_0_CFG_ABORT		(0UL << STRTAB_STE_0_CFG_SHIFT)
#define STRTAB_STE_0_CFG_BYPASS		(4UL << STRTAB_STE_0_CFG_SHIFT)
#define STRTAB_STE_0_CFG_S1_TRANS	(5UL << STRTAB_STE_0_CFG_SHIFT)
#define STRTAB_STE_0_CFG_S2_TRANS	(6UL << STRTAB_STE_0_CFG_SHIFT)

#define STRTAB_STE_0_S1FMT_SHIFT	4
#define STRTAB_STE_0_S1FMT_LINEAR	(0UL << STRTAB_STE_0_S1FMT_SHIFT)
#define STRTAB_STE_0_S1CTXPTR_SHIFT	6
#define STRTAB_STE_0_S1CTXPTR_MASK	0x3fffffffffffUL
#define STRTAB_STE_0_S1CDMAX_SHIFT	59
#define STRTAB_STE_0_S1CDMAX_MASK	0x1fUL

#define STRTAB_STE_1_S1C_CACHE_NC	0UL
#define STRTAB_STE_1_S1C_CACHE_WBRA	1UL
#define STRTAB_STE_1_S1C_CACHE_WT	2UL
#define STRTAB_STE_1_S1C_CACHE_WB	3UL
#define STRTAB_STE_1_S1C_SH_NSH		0UL
#define STRTAB_STE_1_S1C_SH_OSH		2UL
#define STRTAB_STE_1_S1C_SH_ISH		3UL
#define STRTAB_STE_1_S1CIR_SHIFT	2
#define STRTAB_STE_1_S1COR_SHIFT	4
#define STRTAB_STE_1_S1CSH_SHIFT	6

#define STRTAB_STE_1_S1STALLD		(1UL << 27)

#define STRTAB_STE_1_EATS_ABT		0UL
#define STRTAB_STE_1_EATS_TRANS		1UL
#define STRTAB_STE_1_EATS_S1CHK		2UL
#define STRTAB_STE_1_EATS_SHIFT		28

#define STRTAB_STE_1_STRW_NSEL1		0UL
#define STRTAB_STE_1_STRW_EL2		2UL
#define STRTAB_STE_1_STRW_SHIFT		30

#define STRTAB_STE_1_SHCFG_INCOMING	1UL
#define STRTAB_STE_1_SHCFG_SHIFT	44
#define STRTAB_STE_1_SHCFG_OFFSET	1

#define STRTAB_STE_2_S2VMID_SHIFT	0
#define STRTAB_STE_2_S2VMID_MASK	0xffffUL
#define STRTAB_STE_2_S2VMID_OFFSET	2
#define STRTAB_STE_2_VTCR_SHIFT		32
#define STRTAB_STE_2_VTCR_MASK		0x7ffffUL
#define STRTAB_STE_2_S2AA64		(1UL << 51)
#define STRTAB_STE_2_S2ENDI		(1UL << 52)
#define STRTAB_STE_2_S2PTW		(1UL << 54)
#define STRTAB_STE_2_S2R		(1UL << 58)

#define STRTAB_STE_3_S2TTB_SHIFT	4
#define STRTAB_STE_3_S2TTB_MASK		0xffffffffffffUL
#define STRTAB_STE_3_S2TTB_OFFSET		3

#define STRTAB_SURPPOT_SPLIT		6

/* Context descriptor (stage-1 only) */
#define CTXDESC_CD_DWORDS		8
#define CTXDESC_CD_MAX_SSIDS		(1 << 6)
#define CTXDESC_CD_0_TCR_T0SZ_SHIFT	0
#define ARM64_TCR_T0SZ_SHIFT		0
#define ARM64_TCR_T0SZ_MASK		0x1fUL
#define CTXDESC_CD_0_TCR_TG0_SHIFT	6
#define ARM64_TCR_TG0_SHIFT		14
#define ARM64_TCR_TG0_MASK		0x3UL
#define CTXDESC_CD_0_TCR_IRGN0_SHIFT	8
#define ARM64_TCR_IRGN0_SHIFT		8
#define ARM64_TCR_IRGN0_MASK		0x3UL
#define CTXDESC_CD_0_TCR_ORGN0_SHIFT	10
#define ARM64_TCR_ORGN0_SHIFT		10
#define ARM64_TCR_ORGN0_MASK		0x3UL
#define CTXDESC_CD_0_TCR_SH0_SHIFT	12
#define ARM64_TCR_SH0_SHIFT		12
#define ARM64_TCR_SH0_MASK		0x3UL
#define CTXDESC_CD_0_TCR_EPD0_SHIFT	14
#define ARM64_TCR_EPD0_SHIFT		7
#define ARM64_TCR_EPD0_MASK		0x1UL
#define CTXDESC_CD_0_TCR_EPD1_SHIFT	30
#define ARM64_TCR_EPD1_SHIFT		23
#define ARM64_TCR_EPD1_MASK		0x1UL

#define CTXDESC_CD_0_ENDI		(1UL << 15)
#define CTXDESC_CD_0_V			(1UL << 31)

#define CTXDESC_CD_0_TCR_IPS_SHIFT	32
#define ARM64_TCR_IPS_SHIFT		32
#define ARM64_TCR_IPS_MASK		0x7UL
#define CTXDESC_CD_0_TCR_TBI0_SHIFT	38
#define ARM64_TCR_TBI0_SHIFT		37
#define ARM64_TCR_TBI0_MASK		0x1UL

#define CTXDESC_CD_0_AA64		(1UL << 41)
#define CTXDESC_CD_0_R			(1UL << 45)
#define CTXDESC_CD_0_A			(1UL << 46)
#define CTXDESC_CD_0_ASET_SHIFT		47
#define CTXDESC_CD_0_ASET_SHARED	(0UL << CTXDESC_CD_0_ASET_SHIFT)
#define CTXDESC_CD_0_ASET_PRIVATE	(1UL << CTXDESC_CD_0_ASET_SHIFT)
#define CTXDESC_CD_0_ASID_SHIFT		48
#define CTXDESC_CD_0_ASID_MASK		0xffffUL

#define CTXDESC_CD_1_TTB0_SHIFT		4
#define CTXDESC_CD_1_TTB0_MASK		0xffffffffffffUL
#define CTXDESC_CD_1_TTB0_OFFSET		1

#define CTXDESC_CD_3_MAIR_SHIFT		0
#define CTXDESC_CD_3_MAIR_OFFSET		3

#define CTXDESC_CD_0_TCR_T0SZ		GENMASK_ULL(5, 0)
#define CTXDESC_CD_0_TCR_TG0		GENMASK_ULL(7, 6)
#define CTXDESC_CD_0_TCR_IRGN0		GENMASK_ULL(9, 8)
#define CTXDESC_CD_0_TCR_ORGN0		GENMASK_ULL(11, 10)
#define CTXDESC_CD_0_TCR_SH0		GENMASK_ULL(13, 12)
#define CTXDESC_CD_0_TCR_EPD0		(1ULL << 14)
#define CTXDESC_CD_0_TCR_EPD1		(1ULL << 30)
#define CTXDESC_CD_0_TCR_IPS		GENMASK_ULL(34, 32)

#define STRTAB_STE_2_VTCR_S2T0SZ	GENMASK_ULL(5, 0)
#define STRTAB_STE_2_VTCR_S2SL0		GENMASK_ULL(7, 6)
#define STRTAB_STE_2_VTCR_S2IR0		GENMASK_ULL(9, 8)
#define STRTAB_STE_2_VTCR_S2OR0		GENMASK_ULL(11, 10)
#define STRTAB_STE_2_VTCR_S2SH0		GENMASK_ULL(13, 12)
#define STRTAB_STE_2_VTCR_S2TG		GENMASK_ULL(15, 14)
#define STRTAB_STE_2_VTCR_S2PS		GENMASK_ULL(18, 16)

/* Convert between AArch64 (CPU) TCR format and SMMU CD format */
#define ARM_SMMU_TCR2CD(tcr, fld)					\
	((((tcr) >> ARM64_TCR_##fld##_SHIFT) & ARM64_TCR_##fld##_MASK)	\
	 << CTXDESC_CD_0_TCR_##fld##_SHIFT)

/* Command queue */
#define CMDQ_ENT_DWORDS			2
#define CMDQ_MAX_SZ_SHIFT		8

#define CMDQ_CONS_ERR			GENMASK(30, 24)
#define CMDQ_ERR_CERROR_NONE_IDX	0
#define CMDQ_ERR_CERROR_ILL_IDX		1
#define CMDQ_ERR_CERROR_ABT_IDX		2
#define CMDQ_ERR_CERROR_ATC_INV_IDX	3

#define CMDQ_PROD_OWNED_FLAG		Q_OVERFLOW_FLAG

/*
 * This is used to size the command queue and therefore must be at least
 * BITS_PER_LONG so that the valid_map works correctly (it relies on the
 * total number of queue entries being a multiple of BITS_PER_LONG).
 */
#define CMDQ_BATCH_ENTRIES		BITS_PER_LONG

#define CMDQ_ERR_SHIFT			24
#define CMDQ_ERR_MASK			0x7f
#define CMDQ_ERR_CERROR_NONE_IDX	0
#define CMDQ_ERR_CERROR_ILL_IDX		1
#define CMDQ_ERR_CERROR_ABT_IDX		2

#define CMDQ_0_OP_SHIFT			0
#define CMDQ_0_OP_MASK			0xffUL
#define CMDQ_0_SSV			(1UL << 11)

#define CMDQ_PREFETCH_0_SID_SHIFT	32
#define CMDQ_PREFETCH_1_SIZE_SHIFT	0
#define CMDQ_PREFETCH_1_ADDR_MASK	~0xfffUL

#define CMDQ_CFGI_0_CD_SHIFT		12
#define CMDQ_CFGI_0_CD_MASK		0xfffff000UL
#define CMDQ_CFGI_0_SID_SHIFT		32
#define CMDQ_CFGI_0_SID_MASK		0xffffffffUL
#define CMDQ_CFGI_1_LEAF		(1UL << 0)
#define CMDQ_CFGI_1_RANGE_SHIFT		0
#define CMDQ_CFGI_1_RANGE_MASK		0x1fUL

#define CMDQ_TLBI_0_VMID_SHIFT		32
#define CMDQ_TLBI_0_ASID_SHIFT		48
#define CMDQ_TLBI_1_LEAF		(1UL << 0)
#define CMDQ_TLBI_1_VA_MASK		~0xfffUL
#define CMDQ_TLBI_1_IPA_MASK		0xfffffffff000UL

#define CMDQ_PRI_0_SSID_SHIFT		12
#define CMDQ_PRI_0_SSID_MASK		0xfffffUL
#define CMDQ_PRI_0_SID_SHIFT		32
#define CMDQ_PRI_0_SID_MASK		0xffffffffUL
#define CMDQ_PRI_1_GRPID_SHIFT		0
#define CMDQ_PRI_1_GRPID_MASK		0x1ffUL
#define CMDQ_PRI_1_RESP_SHIFT		12
#define CMDQ_PRI_1_RESP_DENY		(0UL << CMDQ_PRI_1_RESP_SHIFT)
#define CMDQ_PRI_1_RESP_FAIL		(1UL << CMDQ_PRI_1_RESP_SHIFT)
#define CMDQ_PRI_1_RESP_SUCC		(2UL << CMDQ_PRI_1_RESP_SHIFT)

#define CMDQ_SYNC_0_CS_SHIFT		12
#define CMDQ_SYNC_0_CS_NONE		(0UL << CMDQ_SYNC_0_CS_SHIFT)
#define CMDQ_SYNC_0_CS_SEV		(2UL << CMDQ_SYNC_0_CS_SHIFT)

/* Event queue */
#define EVTQ_ENT_DWORDS			4
#define EVTQ_MAX_SZ_SHIFT		7

#define EVTQ_0_ID_SHIFT			0
#define EVTQ_0_ID_MASK			0xffUL

#define EVTQ_0_STREAM_ID_SHIFT	32
#define EVTQ_0_STREAM_ID_MASK		0xffffffffUL

#define EVTQ_0_SSID_SHIFT  12
#define EVTQ_0_SSID_MASK   0xffff

#define EVTQ_TYPE_STREAM_DISABLE           0x06
#define EVTQ_TYPE_TRANSL_FORBIDDEN         0x07
#define EVTQ_TYPE_WALK_EABT                0x0b
#define EVTQ_TYPE_TRANSLATION              0x10
#define EVTQ_TYPE_ADDR_SIZE                0x11
#define EVTQ_TYPE_ACCESS                   0x12
#define EVTQ_TYPE_PERMISSION               0x13

#define EVTQ_TYPE_WITH_ADDR(id)     ((EVTQ_TYPE_TRANSLATION == (id))   \
				|| (EVTQ_TYPE_WALK_EABT == (id))   \
				|| (EVTQ_TYPE_TRANSL_FORBIDDEN == (id)) \
				|| (EVTQ_TYPE_ADDR_SIZE == (id))   \
				|| (EVTQ_TYPE_ACCESS == (id))      \
				|| (EVTQ_TYPE_PERMISSION == (id)))

#define EVTQ_TYPE_WITHOUT_SSID(id)  ((EVTQ_TYPE_STREAM_DISABLE == (id))   \
				|| (EVTQ_TYPE_TRANSL_FORBIDDEN == (id)))
/* PRI queue */
#define PRIQ_ENT_DWORDS			2
#define PRIQ_MAX_SZ_SHIFT		8

#define PRIQ_0_SID_SHIFT		32
#define PRIQ_0_SID_MASK			0xffffffffUL
#define PRIQ_0_SSID_SHIFT		32
#define PRIQ_0_SSID_MASK		0xfffffUL
#define PRIQ_0_PERM_PRIV		(1UL << 58)
#define PRIQ_0_PERM_EXEC		(1UL << 59)
#define PRIQ_0_PERM_READ		(1UL << 60)
#define PRIQ_0_PERM_WRITE		(1UL << 61)
#define PRIQ_0_PRG_LAST			(1UL << 62)
#define PRIQ_0_SSID_V			(1UL << 63)

#define PRIQ_1_PRG_IDX_SHIFT		0
#define PRIQ_1_PRG_IDX_MASK		0x1ffUL
#define PRIQ_1_ADDR_SHIFT		12
#define PRIQ_1_ADDR_MASK		0xfffffffffffffUL

/* High-level queue structures */
#define ARM_SMMU_POLL_TIMEOUT_US	100000 /* 100ms */
#define ARM_SMMU_CMDQ_DRAIN_TIMEOUT_US	1000000 /* 1s! */

#define MSI_IOVA_BASE			0x8000000
#define MSI_IOVA_LENGTH			0x100000

/* Until ACPICA headers cover IORT rev. C */
#ifndef ACPI_IORT_SMMU_HISILICON_HI161X
#define ACPI_IORT_SMMU_HISILICON_HI161X		0x1
#endif

#ifndef ACPI_IORT_SMMU_V3_CAVIUM_CN99XX
#define ACPI_IORT_SMMU_V3_CAVIUM_CN99XX		0x2
#endif

#ifdef MM_SMMUV3_SUPPORT
/* TCU CACHE */
#define MM_TCU_CACHE                     0x31000
#define TCU_CACHE_BYPASS                  (MM_TCU_CACHE + 0x0)
#define TCU_CACHELINE_INV_ALL             (MM_TCU_CACHE + 0x4)
/* TCU CTRL */
#define MM_TOP_CTL_BASE                  0x30000
#define SMMU_LP_REQ                       (MM_TOP_CTL_BASE + 0)
#define TCU_QREQN_CG                      (1 << 0)
#define TCU_QREQN_PD                      (1 << 1)
#define SMMU_LP_ACK                       (MM_TOP_CTL_BASE + 0x4)
#define TCU_QACCEPTN_CG                   (1 << 0)
#define TCU_QACCEPTN_PD                   (1 << 4)
#define STREAM_ID_BYPASS                   63
#define INVAL_TTWC_ASID			0x3c
#define ARM_SMMU_IRPT_MASK_NS	(MM_TOP_CTL_BASE + 0x70)
#define ARM_SMMU_IRPT_RAW_NS	(MM_TOP_CTL_BASE + 0x74)
#define TCU_EVENT_TO_MASK		0x38
#define TCU_EVENT_MASK_ALL		0xFFFFFFFF
#define TCU_IRPT_CLR_NS_VAL_MASK	0xffffffff
#define ARM_SMMU_IRPT_CLR_NS		(MM_TOP_CTL_BASE + 0x7c)
#define TCU_EVENT_Q_IRQ_CLR		BIT(0)
#define TCU_CMD_SYNC_IRQ_CLR		BIT(1)
#define TCU_GERROR_IRQ_CLR		BIT(2)
#define TCU_EVENTTO_CLR		BIT(5)
#define INVALID_TCU_PW_CFG	0xFFFFFFFF
#define ARM_SMMU_STRTAB_BASE1		0x84
#define ARM_SMMU_CMDQ_BASE1		0x94
#define ARM_SMMU_EVTQ_BASE1		0xA4
#endif

#define IOVA_DEFAULT_ADDS		0x100000
#define IOVA_DEFAULT_SIZE		0xc0000000
#define DELAY_COEF                         2
#define CMDQ_MAX_TIMEOUT_TIMES	3
#define IOMMU_CELLS_VALUE		2
#define DWORD_BYTES_NUM		3
#define ARM_SMMU_ADDR_SIZE_32	32
#define ARM_SMMU_ADDR_SIZE_36	36
#define ARM_SMMU_ADDR_SIZE_40	40
#define ARM_SMMU_ADDR_SIZE_42	42
#define ARM_SMMU_ADDR_SIZE_44	44
#define ARM_SMMU_ADDR_SIZE_48	48
#define ARM_SMMU_ADDR_SIZE_52	52
#define ARM_SMMU_ID_SIZE_8		8
#define ARM_SMMU_ID_SIZE_16		16
#define MAX_CHECK_TIMES	1000

#define ARM_LPAE_TCR_EPD1 BIT(23)
#define ARM_LPAE_TCR_T0SZ_SHIFT 0
#define ARM_LPAE_TCR_IPS_SHIFT 32
#define ARM_LPAE_TCR_IPS_MASK 0x7

#define ARM_LPAE_TCR_PS_32_BIT 0x0ULL
#define ARM_LPAE_TCR_PS_36_BIT 0x1ULL
#define ARM_LPAE_TCR_PS_40_BIT 0x2ULL
#define ARM_LPAE_TCR_PS_42_BIT 0x3ULL
#define ARM_LPAE_TCR_PS_44_BIT 0x4ULL
#define ARM_LPAE_TCR_PS_48_BIT 0x5ULL

#define ARM_LPAE_MAIR_ATTR_DEVICE 0x04
#define ARM_LPAE_MAIR_ATTR_NC 0x44
#define ARM_LPAE_MAIR_ATTR_WBRWA 0xff
#define ARM_LPAE_MAIR_ATTR_IDX_NC 0
#define ARM_LPAE_MAIR_ATTR_IDX_CACHE 1
#define ARM_LPAE_MAIR_ATTR_IDX_DEV 2
#define arm_lpae_mair_attr_shift(n) ((n) << 3)
#define TCU_CACHE_SOFT_INV  0x3
#define ARM_SMMU_SVM_S1CFG_VMID 0
#define LAZY_FREE_WATERLINE	10
#define WATCHDOG_CPU	0
#define WIFI_INTX_CPU	4
#define LAZY_FREE_SCHED_PRI	97
#define MAX_MEDIA_REG_NAME_LEN  32
#define MAX_MEDIA_REG_NUM 30

enum pri_resp {
	PRI_RESP_DENY,
	PRI_RESP_FAIL,
	PRI_RESP_SUCC,
};

enum arm_smmu_msi_index {
	EVTQ_MSI_INDEX,
	GERROR_MSI_INDEX,
	PRIQ_MSI_INDEX,
	ARM_SMMU_MAX_MSIS,
};

enum tlb_flush_ops {
	ASID_FLUSH, /* arm smmu driver flush tlb by asid */
	SSID_FLUSH, /* arm smmu driver flush tlb by ssid */
	DEV_FLUSH,  /* master dev driver flush tlb by ssid */
};

#ifdef MM_SMMUV3_SUPPORT
/* smmuid must match with dtsi smmuid definition */
enum {
	SMMU_MEDIA1,
	SMMU_MEDIA2,
	SMMU_NPU,
	SMMU_HSDT0,
	SMMU_HSDT1,
	SMMU_MEDIA1_EX1 = 20,
	SMMU_MEDIA2_EX1 = 21,
	SMMU_MEDIA1_EX2 = 22,
	SMMU_MEDIA2_EX2 = 23,
	SMMU_ERR_ID = 0xFF,
};
#endif

struct arm_smmu_cmdq_ent {
	/* Common fields */
	u8				opcode;
	bool				substream_valid;

	/* Command-specific fields */
	union {
#define CMDQ_OP_PREFETCH_CFG	0x1
		struct {
			u32			sid;
			u8			size;
			u64			addr;
		} prefetch;

#define CMDQ_OP_CFGI_STE	0x3
#define CMDQ_OP_CFGI_ALL	0x4
#define CMDQ_OP_CFGI_CD		0x5
		struct {
			u32			sid;
			u32			ssid;
			union {
				bool		leaf;
				u8		span;
			};
		} cfgi;

#define CMDQ_OP_TLBI_NH_ASID	0x11
#define CMDQ_OP_TLBI_NH_VA	0x12
#define CMDQ_OP_TLBI_EL2_ALL	0x20
#define CMDQ_OP_TLBI_S12_VMALL	0x28
#define CMDQ_OP_TLBI_S2_IPA	0x2a
#define CMDQ_OP_TLBI_NSNH_ALL	0x30
		struct {
			u16			asid;
			u16			vmid;
			bool			leaf;
			u64			addr;
		} tlbi;

#define CMDQ_OP_PRI_RESP	0x41
		struct {
			u32			sid;
			u32			ssid;
			u16			grpid;
			enum pri_resp		resp;
		} pri;

#define CMDQ_OP_CMD_SYNC	0x46
	};
};

struct arm_smmu_queue {
	int				irq; /* Wired interrupt */

	__le64				*base;
	dma_addr_t			base_dma;
	u64				q_base;

	size_t				ent_dwords;
	u32				max_n_shift;
	u32				prod;
	u32				cons;

	u32 __iomem			*prod_reg;
	u32 __iomem			*cons_reg;
};

struct arm_smmu_cmdq {
	struct arm_smmu_queue		q;
	spinlock_t			lock;
};

struct arm_smmu_evtq {
	struct arm_smmu_queue		q;
	u32				max_stalls;
};

struct arm_smmu_priq {
	struct arm_smmu_queue		q;
};

/* High-level stream table and context descriptor structures */
struct arm_smmu_strtab_l1_desc {
	u8				span;

	__le64				*l2ptr;
	dma_addr_t			l2ptr_dma;
};

struct arm_smmu_s1_cfg {
	__le64				*cdptr;
	dma_addr_t			cdptr_dma;
	u8				tlb_flush;
	u16				vmid;
	struct arm_smmu_ctx_desc {
		u16	ssid;
		u16	asid;
		u32	ssid_nums;
		u64	ttbr;
		u64	tcr;
		u64	mair;
	}				cd;
};

struct arm_smmu_s2_cfg {
	u16				vmid;
	u64				vttbr;
	u64				vtcr;
};

struct arm_smmu_strtab_ent {
	/*
	 * An STE is "assigned" if the master emitting the corresponding SID
	 * is attached to a domain. The behaviour of an unassigned STE is
	 * determined by the disable_bypass parameter, whereas an assigned
	 * STE behaves according to s1_cfg/s2_cfg, which themselves are
	 * configured according to the domain type.
	 */
	bool				assigned;
	struct arm_smmu_s1_cfg		*s1_cfg;
	struct arm_smmu_s2_cfg		*s2_cfg;
};

struct arm_smmu_strtab_cfg {
	__le64				*strtab;
	dma_addr_t			strtab_dma;
	struct arm_smmu_strtab_l1_desc	*l1_desc;
	unsigned int			num_l1_ents;

	u64				strtab_base;
	u32				strtab_base_cfg;
};

struct arm_smmu_iommu_group {
	u32				sid;
	struct list_head			list;
	struct iommu_group		*group;
	struct blocking_notifier_head	notifier;
};

struct arm_smmu_tcu_pw {
	u32	lp_req_reg;
	u32	lp_ack_reg;
	u32	clk_gate_req;
	u32	clk_gate_ack;
	u32	pw_stat_req;
	u32	pw_stat_ack;
};

enum tcu_cache_type {
	OUTER_CACHE,
	INNER_CACHE,
};

enum tcu_cache_status {
	TCU_CACHE_DISABLE,
	TCU_CACHE_ENABLE,
};

struct arm_smmu_tcu_cache {
	enum tcu_cache_type type;
	enum tcu_cache_status status;
};

struct arm_smmu_media_dump {
	u32 reg_nums;
	const char *reg_name[MAX_MEDIA_REG_NUM];
	u32 *reg_offset; /* need dump reg offset */
};

/* An SMMUv3 instance */
struct arm_smmu_device {
	struct device			*dev;
	void __iomem			*base;

#define ARM_SMMU_FEAT_2_LVL_STRTAB	(1 << 0)
#define ARM_SMMU_FEAT_2_LVL_CDTAB	(1 << 1)
#define ARM_SMMU_FEAT_TT_LE		(1 << 2)
#define ARM_SMMU_FEAT_TT_BE		(1 << 3)
#define ARM_SMMU_FEAT_PRI		(1 << 4)
#define ARM_SMMU_FEAT_ATS		(1 << 5)
#define ARM_SMMU_FEAT_SEV		(1 << 6)
#define ARM_SMMU_FEAT_MSI		(1 << 7)
#define ARM_SMMU_FEAT_COHERENCY		(1 << 8)
#define ARM_SMMU_FEAT_TRANS_S1		(1 << 9)
#define ARM_SMMU_FEAT_TRANS_S2		(1 << 10)
#define ARM_SMMU_FEAT_STALLS		(1 << 11)
#define ARM_SMMU_FEAT_HYP		(1 << 12)
#define ARM_SMMU_FEAT_VAX		(1 << 14)
	u32				features;

#define ARM_SMMU_OPT_SKIP_PREFETCH	(1 << 0)
#define ARM_SMMU_OPT_PAGE0_REGS_ONLY	(1 << 1)
	u32				options;

	struct arm_smmu_tcu_pw		pw_cfg;
	struct arm_smmu_media_dump	reg_dump;
	struct arm_smmu_cmdq		cmdq;
	struct arm_smmu_evtq		evtq;
	struct arm_smmu_priq		priq;

	int				gerr_irq;
	int				combined_irq;

	unsigned long			ias; /* IPA */
	unsigned long			oas; /* PA */
	unsigned long			pgsize_bitmap;

#define ARM_SMMU_MAX_ASIDS		(1 << 16)
	unsigned int			asid_bits;
	DECLARE_BITMAP(asid_map, ARM_SMMU_MAX_ASIDS);

#define ARM_SMMU_MAX_VMIDS		(1 << 16)
	unsigned int			vmid_bits;
	DECLARE_BITMAP(vmid_map, ARM_SMMU_MAX_VMIDS);

	unsigned int			ssid_bits;
	unsigned int			sid_bits;

	struct arm_smmu_strtab_cfg	strtab_cfg;

	/* IOMMU core code handle */
	struct iommu_device		iommu;
	int				status;
	int                             smmuid;

	spinlock_t			iommu_groups_lock;
	struct list_head		iommu_groups;
	bool				bypass;
	/*
	 * Sometimes self-developed components and purchased components may coexist,
	 * is_arm_smmu is used to identify the driver type
	 */
	bool				is_arm_smmu;
	/*
	 * TCU cache management: type and status
	 * type 0: outer_cache
	 * type 1: inner_cache
	 * status 0: disable
	 * status 1: enable
	 */
	struct arm_smmu_tcu_cache	tcu_cache;
	spinlock_t			tcu_lock;
	struct notifier_block		tcu_pm_nb;
	unsigned long                   cnt;
	void                            *crg_vbase;
	u64                             evt[EVTQ_ENT_DWORDS];
};

/* SMMU private data for each master */
struct arm_smmu_master_data {
	struct arm_smmu_device		*smmu;
	struct arm_smmu_domain		*smmu_domain;
	struct arm_smmu_strtab_ent	ste;
	struct iommu_domain_data	iova;
	u32				*sids;
	unsigned int	num_sids;
	u8				tlb_flush;
};

/* SMMU private data for an IOMMU domain */
enum arm_smmu_domain_stage {
	ARM_SMMU_DOMAIN_S1 = 0,
	ARM_SMMU_DOMAIN_S2,
	ARM_SMMU_DOMAIN_NESTED,
	ARM_SMMU_DOMAIN_BYPASS,
};

enum arm_smmu_status {
	ARM_SMMU_DISABLE,
	ARM_SMMU_ENABLE,
};

enum arm_smmu_domain_type {
	ARM_SMMU_DOMAIN_DMA,
	ARM_SMMU_DOMAIN_SVM,
};

struct arm_smmu_domain {
	struct arm_smmu_device		*smmu;
	struct mutex			init_mutex; /* Protects smmu pointer */

	struct io_pgtable_ops		*pgtbl_ops;

	enum arm_smmu_domain_stage	stage;
	union {
		struct arm_smmu_s1_cfg	s1_cfg;
		struct arm_smmu_s2_cfg	s2_cfg;
	};

	struct iommu_domain		domain;
	struct device			*dev; /* svm domain use */
	u32				type; /* domain type:dma or svm */
	u8				tlb_flush;

	DECLARE_BITMAP(ssid_map, CTXDESC_CD_MAX_SSIDS); /* svm domain use */
};

struct arm_v3_domain {
	struct arm_smmu_domain smmu_domain;
	struct list_head list;
};

struct arm_smmu_option_prop {
	u32 opt;
	const char *prop;
};

struct finalise_stage_ops {
	int (*finalise_stage_fn)(struct arm_smmu_domain *smmu_domain,
				struct io_pgtable_cfg *pgtbl_cfg);
};

static struct arm_smmu_option_prop arm_smmu_options[] = {
	{ ARM_SMMU_OPT_SKIP_PREFETCH, "mm,broken-prefetch-cmd" },
	{ ARM_SMMU_OPT_PAGE0_REGS_ONLY, "cavium,cn9900-broken-page1-regspace"},
	{ 0, NULL},
};

#define REG_NAME_LEN  30
struct reg_info {
	char name[REG_NAME_LEN];
	u32 offset;
};

/* svm_private Interface */
int arm_smmu_svm_get_ssid(struct iommu_domain *domain, u16 *ssid, u64 *ttbr, u64 *tcr);
void arm_smmu_svm_tlb_inv_context(struct mm_struct *mm);
int arm_smmu_poweron(struct device *dev);
int arm_smmu_poweroff(struct device *dev);
int arm_smmu_evt_register_notify(struct device *dev, struct notifier_block *n);
int arm_smmu_evt_unregister_notify(struct device *dev, struct notifier_block *n);
void arm_smmu_tbu_status_print(struct device *dev);
int arm_smmu_get_reg_info(struct device *dev, char *buffer, int length);

#endif /* _PLATFORM_SMMU_V3_H */