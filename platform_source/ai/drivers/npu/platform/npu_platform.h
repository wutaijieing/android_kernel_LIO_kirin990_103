/*
 * npu_platform.h
 *
 * about npu platform
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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
#ifndef __NPU_PLATFORM_H
#define __NPU_PLATFORM_H
#include <linux/interrupt.h>
#include "npu_common.h"
#include "npu_platform_resource.h"
#include "npu_ddr_map.h"
#include "npu_platform_register.h"

#ifdef CONFIG_NPU_INTR_HUB
#include "npu_intr_hub.h"
#endif

#define NPU_SC_TESTREG_INIT          0
#define NPU_SC_TESTREG_TS_READY      0x06

#define NPU_PLAT_TYPE_FPGA   0x0
#define NPU_PLAT_TYPE_EMU    0x1
#define NPU_PLAT_TYPE_ESL    0x2
#define NPU_PLAT_TYPE_ASIC   0x3
#define NPU_PLAT_VERSION_MASK       0x00000FFF
#define NPU_PLAT_AI_CORE_NUM_MASK   0x0000F000
#define NPU_PLAT_MASK               0x000F0000
#define NPU_PLAT_AI_CORE_NUM_2      0x00002000
#define NPU_PLAT_AI_CORE_NUM_1      0x00000000

#define NPU_PLAT_DEVICE      0
#define NPU_PLAT_HOST        1

#define NPU_DFX_CHANNEL_MAX_RESOURCE 4

enum npu_register_index {
	NPU_REG_TS_SUBSYSCTL,
	NPU_REG_TS_DOORBELL,
	NPU_REG_TS_SRAM,
	NPU_REG_SYSCTL,
	NPU_REG_L2BUF_BASE,
	NPU_REG_PERICRG_BASE,
	NPU_REG_HWTS_BASE,
	NPU_REG_AIC0_BASE,
	NPU_REG_AIC1_BASE,
	NPU_REG_HW_EXP_IRQ_NS_BASE,
	NPU_REG_SDMA_BASE,
	NPU_REG_NOC_BUS_BASE,
	NPU_REG_MAX_RESOURCE,
};

enum npu_feature_index {
	NPU_FEATURE_AUTO_POWER_DOWN,
	NPU_FEATURE_NPU_PM_INTF,
	NPU_FEATURE_SVM_MEM,
	NPU_FEATURE_GIC_MULTICHIP,
	NPU_FEATURE_KMALLOC,
	NPU_FEATURE_TASK_DOWN,
	NPU_FEATURE_HWTS,
	NPU_FEATURE_SEC_PLAT,
	NPU_FEATURE_KERNEL_LOAD_IMG,
	NPU_FEATURE_GIC_SUPPORT,
	NPU_FEATURE_INTERFRAME_IDLE_POWER_DOWN,
	NPU_FEATURE_NPU_AUTOFS,
	NPU_FEATURE_L2_BUFF,
	NPU_FEATURE_EASC,
	NPU_FEATURE_SEC_NONSEC_CONCURRENCY,
	NPU_FEATURE_MAX_RESOURCE
};

enum npu_feature_switch {
	NPU_FEATURE_OFF,
	NPU_FEATURE_ON
};

enum npu_irq_index {
	NPU_IRQ_CALC_CQ_UPDATE0,
	NPU_IRQ_DFX_CQ_UPDATE,
	NPU_IRQ_MAILBOX_ACK,
#ifdef CONFIG_NPU_INTR_HUB
	NPU_IRQ_INTR_HUB,
#endif
#ifdef CONFIG_NPU_SWTS
	NPU_IRQ_HWTS_NORMAL_NS,
	NPU_IRQ_HWTS_ERROR_NS,
	NPU_IRQ_DMMU,
#endif
	NPU_IRQ_EASC,
};


enum npu_dfx_dev_index {
	NPU_DFX_DEV_LOG,
	NPU_DFX_DEV_PROFILE,
	NPU_DFX_DEV_BLACKBOX,
	NPU_DFX_DEV_MAX_RESOURCE
};

enum npu_binary_idx_s {
	TSCPU_EL3_S,
	TSCPU_EL1_S,
	TSCPU_RESV_S,
	BINARY_IDX_S_CNT
};

struct npu_pm_adapter {
	int (*npu_power_up)(struct npu_dev_ctx *dev_ctx, u32 work_mode);
	int (*npu_open)(uint32_t devid);
	int (*npu_release)(uint32_t devid);
	int (*npu_power_down)(uint32_t devid, u32 work_mode, u32 *stage);
};

struct npu_resource_adapter {
	int (*npu_irq_change_route)(u32 irq_num, u32 cpuid);
	int (*npu_cqsq_share_alloc)(u32 stream_id, void *alloc_func, u32 dev_id);
	int (*npu_trigger_irq)(u32 irq_num);
	int (*npu_mailbox_send)(void *mailbox, int mailbox_len,
		const void *message, int message_len);
	int (*npu_ctrl_core)(struct npu_dev_ctx *dev_ctx, u32 core_num);
};

struct npu_platform_specification {
	u32 stream_max;
	u32 event_max;
	u32 notify_max;
	u32 model_max;
	u32 aicore_max;
	u32 calc_sq_max;
	u32 calc_sq_depth;
	u32 calc_cq_max;
	u32 calc_cq_depth;
	u32 dfx_sq_max;
	u32 dfx_cq_max;
	u32 dfx_sq_depth;
	u32 dfx_cq_depth;
	u32 doorbell_stride;
};

struct npu_mem_desc {
	u64 base;
	u64 len;
};

struct npu_platform_adapter {
	struct npu_pm_adapter pm_ops;
	struct npu_resource_adapter res_ops;
};

struct npu_dfx_desc {
	u32 channels[NPU_DFX_CHANNEL_MAX_RESOURCE];
	struct npu_mem_desc *bufs;
	u8 channel_num;
};

struct npu_dts_info {
	void __iomem *reg_vaddr[NPU_REG_MAX_RESOURCE];
	struct npu_mem_desc reg_desc[NPU_REG_MAX_RESOURCE];
	u32 feature_switch[NPU_FEATURE_MAX_RESOURCE];
	struct npu_mem_desc dump_region_desc[NPU_DUMP_REGION_MAX];
#ifdef CONFIG_NPU_SWTS
	int irq_swts_hwts_normal;
	int irq_swts_hwts_error;
	int irq_dmmu;
#endif
	int irq_cq_update;
	int irq_mailbox_ack;
	int irq_dfx_cq;
#ifdef CONFIG_NPU_INTR_HUB
	int irq_intr_hub;
#endif
	int irq_easc;
	u32 tscpu_cluster;
	u32 tscpu_core;
	u32 gic0_spi_blk_num;
};

struct npu_resmem_info {
	struct npu_dfx_desc dfx_desc[NPU_DFX_DEV_MAX_RESOURCE];
	struct npu_mem_desc resmem_desc[NPU_MAX_RESMEM_IDX];
	struct npu_mem_desc resmem_sec_desc[BINARY_IDX_S_CNT];
	struct npu_mem_desc *info_buf;
	struct npu_mem_desc *sqcq_buf;
#ifndef CONFIG_NPU_SWTS
	struct npu_mem_desc *tsfw_buf;
	struct npu_mem_desc *aifw_buf;
	struct npu_mem_desc *sdma_sq_buf;
	struct npu_mem_desc *persistent_task_buf;
	struct npu_mem_desc *npu_cfg_buf;
#else
	struct npu_mem_desc *hwts_swap_buf;
#endif
};

struct npu_platform_info {
	struct npu_platform_adapter adapter;
	struct npu_platform_specification spec;
	struct npu_dts_info dts_info;
	struct npu_resmem_info resmem_info;
#ifdef CONFIG_NPU_INTR_HUB
	struct npu_intr_hub intr_hub;
#endif
	struct device *pdev;
	struct device *p_ts_subsys_dev;
	const char *chiptype;
	u8 env_type;
	u8 plat_type;
	u8 reserved[2];
};

struct npu_platform_info *npu_plat_get_info(void);
u32 *npu_plat_get_reg_vaddr_offset(u32 reg_idx, u32 offset);
int npu_debug_init(void);
int npu_plat_set_resmem(struct npu_platform_info *plat_info);

#endif
