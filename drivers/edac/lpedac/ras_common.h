/*
 * ras_common.h
 *
 * RAS COMMON HEAD
 *
 * Copyright (c) 2019-2021 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#ifndef __RAS_COMMON_H__
#define __RAS_COMMON_H__

#include <platform_include/basicplatform/linux/dfx_bbox_diaginfo.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include <platform_include/basicplatform/linux/rdr_platform.h>
#include <mntn_subtype_exception.h>
#include "edac_module.h"

/* access type */
#define MEM	1
#define MRS	0

/* error type */
#define CE	0
#define UE	1

/* device mode id */
#define SHARE_L2	0x200
#define SHARE_L3	0x300

/* SERR ID */
#define BUS_ERROR	0x12

/* RAS system registers */
#define DISR_EL1                S3_0_C12_C1_1
#define DISR_A_BIT              U(31)

#define ERRIDR_EL1              S3_0_C5_C3_0
#define ERRIDR_MASK             U(0xffff)

#define ERRSELR_EL1             S3_0_C5_C3_1

/* System register access to Standard Error Record registers */
#define ERXFR_EL1               S3_0_C5_C4_0
#define ERXCTLR_EL1             S3_0_C5_C4_1
#define ERXSTATUS_EL1           S3_0_C5_C4_2
#define ERXADDR_EL1             S3_0_C5_C4_3
#define ERXMISC0_EL1            S3_0_C5_C5_0
#define ERXMISC1_EL1            S3_0_C5_C5_1

#define ERXCTLR_ED_BIT          (U(1) << 0)
#define ERXCTLR_UE_BIT          (U(1) << 4)

#define ERXPFGCTL_UC_BIT        (U(1) << 1)
#define ERXPFGCTL_UEU_BIT       (U(1) << 2)
#define ERXPFGCTL_CDEN_BIT      (U(1) << 31)

#define ERR_STATUS_V_SHIFT	30
#define ERR_STATUS_V_MASK	0x1
#define ERR_STATUS_CE_SHIFT	24
#define ERR_STATUS_CE_MASK	0x2
#define ERR_STATUS_CE_CLE	0x3
#define ERR_STATUS_DE_SHIFT	23
#define ERR_STATUS_DE_MASK	0x1
#define ERR_STATUS_UE_SHIFT	29
#define ERR_STATUS_UE_MASK	0x1
#define ERR_STATUS_SERR_SHIFT	0
#define ERR_STATUS_SERR_MASK	0xff
#define ERR_STATUS_GET_FIELD(_status, _field) \
	(((_status) >> ERR_STATUS_ ##_field ##_SHIFT) & ERR_STATUS_ ##_field ##_MASK)

#define _DEFINE_SYSREG_READ_FUNC(_name, _reg_name)              \
static inline u64 read_ ## _name(void)				\
{								\
	return read_sysreg(_reg_name);				\
}

#define _DEFINE_SYSREG_WRITE_FUNC(_name, _reg_name)		\
static inline void write_ ## _name(u64 v)			\
{								\
	write_sysreg(v, _reg_name);				\
}

/* Define read & write function for renamed system register */
#define DEFINE_RENAME_SYSREG_RW_FUNCS(_name, _reg_name)		\
	_DEFINE_SYSREG_READ_FUNC(_name, _reg_name)		\
	_DEFINE_SYSREG_WRITE_FUNC(_name, _reg_name)

/* Define read function for renamed system register */
#define DEFINE_RENAME_SYSREG_READ_FUNC(_name, _reg_name)	\
	_DEFINE_SYSREG_READ_FUNC(_name, _reg_name)

/* Define write function for renamed system register */
#define DEFINE_RENAME_SYSREG_WRITE_FUNC(_name, _reg_name)	\
	_DEFINE_SYSREG_WRITE_FUNC(_name, _reg_name)

DEFINE_RENAME_SYSREG_READ_FUNC(erridr_el1, ERRIDR_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(errselr_el1, ERRSELR_EL1)

DEFINE_RENAME_SYSREG_READ_FUNC(erxfr_el1, ERXFR_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(erxctlr_el1, ERXCTLR_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(erxstatus_el1, ERXSTATUS_EL1)
DEFINE_RENAME_SYSREG_READ_FUNC(erxaddr_el1, ERXADDR_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(erxmisc0_el1, ERXMISC0_EL1)
DEFINE_RENAME_SYSREG_RW_FUNCS(erxmisc1_el1, ERXMISC1_EL1)

/* L3 RAS regester offset */
#define ERR1FR		0x0
#define ERR1CTRL	0x8
#define ERR1MISC0	0x20
#define ERR1STATUS	0x10

/* Error message for err type */
struct ras_error {
	const char *error_msg;
	/* SERR(bits[7:0]) from ERR<n>STATUS */
	u8 serr_idx;
};

struct err_record {
	u64 errfr;
	u64 errstatus;
	u64 errmisc;
};

struct irq_node {
	int fault_irq;
	int err_irq;
	u32 nr_block;

	cpumask_t support_cpus;
	struct err_record *err_record;
};

struct irq_nodes {
	char *name;
	struct irq_node *irq_data;
	unsigned int mod_id;
	int access_type;
	unsigned poll_msec;
	unsigned long delay;
	void __iomem *base;

	u32 nr_inst;
	u32 *idx_blocks;
	u64 *init_misc_data;

	struct notifier_block nb_pm;
	struct hlist_node node;
	struct delayed_work work;
};

/**
 * ras_device_handle_ue():
 *	perform a common output and handling of an 'edac_dev' UE event
 *
 * @edac_dev: pointer to struct &edac_device_ctl_info
 * @inst_nr: number of the instance where the UE error happened
 * @block_nr: number of the block where the UE error happened
 * @msg: message to be printed
 */
extern void ras_device_handle_ue(struct edac_device_ctl_info *edac_dev,
				int inst_nr, int block_nr, const char *msg);
/**
 * ras_device_handle_ce():
 *	perform a common output and handling of an 'edac_dev' CE event
 *
 * @edac_dev: pointer to struct &edac_device_ctl_info
 * @inst_nr: number of the instance where the CE error happened
 * @block_nr: number of the block where the CE error happened
 * @msg: message to be printed
 */
extern void ras_device_handle_ce(struct edac_device_ctl_info *edac_dev,
				int inst_nr, int block_nr, const char *msg);

/**
 * ras_device_handle_de():
 *	perform a common output and handling of an 'edac_dev' CE event
 *
 * @edac_dev: pointer to struct &edac_device_ctl_info
 * @inst_nr: number of the instance where the CE error happened
 * @block_nr: number of the block where the CE error happened
 * @msg: message to be printed
 */
extern void ras_device_handle_de(struct edac_device_ctl_info *edac_dev,
				 int inst_nr, int block_nr, const char *msg);

/* panic message for log */
#define DEFINE_CPU_EDAC_EINFO(ERR_TYPE)					\
	{								\
		.e_modid            = (u32)MODID_AP_S_##ERR_TYPE,	\
		.e_modid_end        = (u32)MODID_AP_S_##ERR_TYPE,	\
		.e_process_priority = RDR_ERR,				\
		.e_reboot_priority  = RDR_REBOOT_NOW,			\
		.e_notify_core_mask = RDR_AP,				\
		.e_reset_core_mask  = RDR_AP,				\
		.e_from_core        = RDR_AP,				\
		.e_reentrant        = (u32)RDR_REENTRANT_DISALLOW,	\
		.e_exce_type        = (u32)AP_S_PANIC,			\
		.e_exce_subtype     = HI_APPANIC_##ERR_TYPE,		\
		.e_upload_flag      = (u32)RDR_UPLOAD_YES,		\
		.e_from_module      = "ap",				\
		.e_desc             = "ap",				\
	},

static struct rdr_exception_info_s cpu_edac_einfo[] = {
	DEFINE_CPU_EDAC_EINFO(CPU0_CE)
	DEFINE_CPU_EDAC_EINFO(CPU0_UE)
	DEFINE_CPU_EDAC_EINFO(CPU1_CE)
	DEFINE_CPU_EDAC_EINFO(CPU1_UE)
	DEFINE_CPU_EDAC_EINFO(CPU2_CE)
	DEFINE_CPU_EDAC_EINFO(CPU2_UE)
	DEFINE_CPU_EDAC_EINFO(CPU3_CE)
	DEFINE_CPU_EDAC_EINFO(CPU3_UE)
	DEFINE_CPU_EDAC_EINFO(CPU4_CE)
	DEFINE_CPU_EDAC_EINFO(CPU4_UE)
	DEFINE_CPU_EDAC_EINFO(CPU5_CE)
	DEFINE_CPU_EDAC_EINFO(CPU5_UE)
	DEFINE_CPU_EDAC_EINFO(CPU6_CE)
	DEFINE_CPU_EDAC_EINFO(CPU6_UE)
	DEFINE_CPU_EDAC_EINFO(CPU7_CE)
	DEFINE_CPU_EDAC_EINFO(CPU7_UE)
	DEFINE_CPU_EDAC_EINFO(CPU8_CE)
	DEFINE_CPU_EDAC_EINFO(CPU8_UE)
	DEFINE_CPU_EDAC_EINFO(CPU01_L2_CE)
	DEFINE_CPU_EDAC_EINFO(CPU01_L2_UE)
	DEFINE_CPU_EDAC_EINFO(CPU23_L2_CE)
	DEFINE_CPU_EDAC_EINFO(CPU23_L2_UE)
	DEFINE_CPU_EDAC_EINFO(L3_CE)
	DEFINE_CPU_EDAC_EINFO(L3_UE)
};

#endif /* __RAS_COMMON_H__ */
