/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2019. All rights reserved.
 * Description: HSDT Trace driver for Kirin SoCs
 * Author: Hisilicon
 * Create: 2019-10-14
 */

#ifndef __HSDT_TRACE_H
#define __HSDT_TRACE_H

#include <linux/err.h>
#include <linux/atomic.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <securec.h>
#include <linux/platform_drivers/hsdt_trace_api.h>

/* hsdt trace register offset */
#define TRG_GROUP0_CMD_OFFSET		0x0
#define TRG_GROUP0_EVENT_OFFSET		0x4
#define TRG_GROUP0_DATA1_LOW_OFFSET	0x8
#define TRG_GROUP0_DATA1_HIGH_OFFSET	0xc
#define TRG_GROUP0_DATA2_LOW_OFFSET	0x10
#define TRG_GROUP0_DATA2_HIGH_OFFSET	0x14
#define TRG_GROUP0_DATA1_MASK_L_OFFSET	0x18
#define TRG_GROUP0_DATA1_MASK_H_OFFSET	0x1c
#define TRG_GROUP0_DATA2_MASK_L_OFFSET	0x20
#define TRG_GROUP0_DATA2_MASK_H_OFFSET	0x24
#define TRG_TIMEOUT_OFFSET		0x28
#define TRG_WAIT_NUM_OFFSET		0x2c
#define TRG_MANUAL_OFFSET		0x30
#define EOF_INT_OFFSET			0x34
#define CFG_AXI_BURST_LENGTH_OFFSET	0x38
#define CFG_AXI_TIMEOUT_L_OFFSET	0x3c
#define CFG_AXI_TIMEOUT_H_OFFSET	0x40
#define CFG_AXI_INIT_ADDR_L_OFFSET	0x44
#define CFG_AXI_INIT_ADDR_H_OFFSET	0x48
#define CFG_TRACE_EN_OFFSET		0x54
#define CFG_EVENT_PRI_OFFSET		0x58

/* hsdt trace enable/disable */
#define HSDT_TRACE_ENABLE		0x1
#define HSDT_TRACE_DISABLE		0x0

/* reg trace default value */
#define DEFAULT_TRACE_EVENT		0x03020100
#define DEFAULT_TRACE_TIMEOUT		0x2000
#define DEFAULT_TRACE_OUTSTANDING	0x23
#define DEFAULT_TRACE_EOF_EN		0xf0
#define DEFAULT_TRACE_TIMESTAMP		0x4
#define DEFAULT_CS_LTIMESTAMP_ENABLE	0x1

/* bit validation check */
#define TRACE_PARAM_VALID		1
#define TRACE_PARAM_INVALID		0
#define TRG_GROUP0_CMD_VALIDBIT		0xFF
#define TRG_GROUP0_EVENT_VALIDBIT	0x3F003F
#define TRG_MANUAL_VALIDBIT		0x3FF

/* other */
#define BIT_NUM_PER_BYTE		8
#define BYTE_NUM_PER_WORD		4
#define FILE_CHMOD_PRIORITY		644
#define TRACE_PRIORITY_UPPER_BOUND	0x3F
#define TRACE_PRIORITY_LOWER_BOUND	0
#define TRACE_BUFF_SIZE			0x400000 /* bytes */
#define TRACE_COUNT_MAX			(TRACE_BUFF_SIZE / BYTE_NUM_PER_WORD)
#define TRACE_SIZE_PER			128	 /* bit */
#define TRACE_SMC_INVALID_PARAM		0
#define REG_INDEX_IN_DTSI		5
#define TRACE_DMA_MASK  64

#define is_valid_trace_bus_type(bus_type) ((bus_type) < HSDT_BUS_BUTT)
#define hsdt_trace_err(fmt, args ...)	pr_err("[Kirin_hsdt_trace][err]%s:" fmt, __func__, ##args)
#define hsdt_trace_inf(fmt, args ...)	pr_info("[Kirin_hsdt_trace][info]%s:" fmt, __func__, ##args)
#define hsdt_trace_debug(fmt, args ...) pr_debug("[Kirin_hsdt_trace][debug]%s:" fmt, __func__, ##args)

enum hsdt_trace_init {
	TRACE_NO_INIT = 0,
	TRACE_INIT = 1,
};
enum hsdt_trace_bustype {
	HSDT_BUS_UFS   = 0,
	HSDT_BUS_PCIE0 = 1,
	HSDT_BUS_PCIE1 = 2,
	HSDT_BUS_USB   = 3,
	HSDT_BUS_DP    = 4,
	HSDT_BUS_BUTT,
};

enum hsdt_trace_register_type {
	HSDT_TRACE_REGISTER_USF 	= 0,
	HSDT_TRACE_REGISTER_PCIE0 	= 1,
	HSDT_TRACE_REGISTER_HSDT1	= 2,
	HSDT_TRACE_REGISTER_BUTT,
};

enum hsdt_trace_event_num {
	HSDT_EVENT_NUM_UFS 	= 92,
	HSDT_EVENT_NUM_PCIE0	= 12,
	HSDT_EVENT_NUM_PCIE1	= 12,
	HSDT_EVENT_NUM_USB	= 42,
	HSDT_EVENT_NUM_DP	= 28,
};

enum hsdt_trace_smc_ops_type {
	TRACE_SMC_ENABLE = 0,
	TRACE_SMC_CLK_ENABLE = 1,
	TRACE_OPS_BUTT,
};

struct hsdt_trace_info {
	void __iomem *trace_cfg_base_addr;
	void __iomem *cs_tsgen_base_addr;
	u32 enable_trace;
	u32 trace_buffer_order;
	u32 trace_buffer_size;
	u32 trace_trigger_type;
	u32 trace_register_type;
	u32 *trace_buffer_addr;
	u32 init;
	dma_addr_t *buff_virt;
	phys_addr_t buff_phy;
};

struct hsdt_trace_data_info {
	u64 trace_data : 55;
	u64 ddr_loop_flag : 1;
	u64 short_timestamp_info : 8;

	u64 event_priority_remark : 63;
	u64 long_timestamp_flag : 1;
};

#endif
