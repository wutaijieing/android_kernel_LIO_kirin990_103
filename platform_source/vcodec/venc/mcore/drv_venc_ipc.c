/*
 * drv_venc_ipc.c
 *
 * This is for Operations Related to ipc.
 *
 * Copyright (c) 2021-2021 Huawei Technologies CO., Ltd.
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

#include "drv_venc_ipc.h"
#include <linux/io.h>
#include <linux/wait.h>
#include "soc_venc_reg_interface.h"
#include "soc_venc_reg/soc_ipc_vrce_interface.h"
#include "drv_common.h"
#include "soc_intr_hub_interface.h"
#include "drv_venc_mcore_tcm.h"
#include "venc_regulator.h"

#define IPC_KEY_UNLOCK 0x1acce551
#ifdef PCIE_LINK
#define IPC_TIMEOUT_MS 10000
#else
#define IPC_TIMEOUT_MS 300
#endif

enum MCU_LEVEL2_IRQ {
	INTR_VENC_PROT_TO_GIC = 0, // reserved
	INTR_VENC_IPC_INT_0_TO_GIC,
	INTR_VENC_IPC_MBX_0_TO_GIC,
	INTR_VENC_WATCHDOG_TO_GIC
};

// only for mbx0
void venc_ipc_recive_msg(int32_t mbx_id)
{
	uint32_t data0 = readl(SOC_IPC_VRCE_MBX_DATA0_ADDR(VEDU_IPC_BASE, mbx_id));
	writel(IPC_KEY_UNLOCK, SOC_IPC_VRCE_IPC_LOCK_ADDR(VEDU_IPC_BASE));
	writel(BIT(ACPU_IPC_ID), SOC_IPC_VRCE_MBX_ICLR_ADDR(VEDU_IPC_BASE, mbx_id));
	VCODEC_INFO_VENC("ipc mbx%d reviced data:0x%x", mbx_id, data0);
}

// only for mbx2
void venc_ipc_recive_ack(int32_t mbx_id)
{
	uint32_t data0 = readl(SOC_IPC_VRCE_MBX_DATA0_ADDR(VEDU_IPC_BASE, mbx_id));
	writel(IPC_KEY_UNLOCK, SOC_IPC_VRCE_IPC_LOCK_ADDR(VEDU_IPC_BASE));
	writel(BIT(ACPU_IPC_ID), SOC_IPC_VRCE_MBX_ICLR_ADDR(VEDU_IPC_BASE, mbx_id));
}

int32_t venc_ipc_send_msg(int32_t mbx_id, struct ipc_message *msg)
{
	// step 1
	writel(IPC_KEY_UNLOCK, SOC_IPC_VRCE_IPC_LOCK_ADDR(VEDU_IPC_BASE));
	uint32_t value = readl(SOC_IPC_VRCE_MBX_SOURCE_ADDR(VEDU_IPC_BASE, mbx_id));
	if (value != 0)
		writel(value, SOC_IPC_VRCE_MBX_SOURCE_ADDR(VEDU_IPC_BASE, mbx_id)); // free mbx if in use

	writel(BIT(ACPU_IPC_ID), SOC_IPC_VRCE_MBX_SOURCE_ADDR(VEDU_IPC_BASE, mbx_id));
	// step 2
	writel(BIT(VRCE_IPC_ID), SOC_IPC_VRCE_MBX_DSET_ADDR(VEDU_IPC_BASE, mbx_id));
	// step 3
	writel(0, SOC_IPC_VRCE_MBX_MODE_ADDR(VEDU_IPC_BASE, mbx_id));
	// step 4
	writel(msg->id, SOC_IPC_VRCE_MBX_DATA0_ADDR(VEDU_IPC_BASE, mbx_id));
	writel(msg->data[0], SOC_IPC_VRCE_MBX_DATA1_ADDR(VEDU_IPC_BASE, mbx_id));
	writel(msg->data[1], SOC_IPC_VRCE_MBX_DATA2_ADDR(VEDU_IPC_BASE, mbx_id));
	writel(msg->data[2], SOC_IPC_VRCE_MBX_DATA3_ADDR(VEDU_IPC_BASE, mbx_id));
	writel(msg->data[3], SOC_IPC_VRCE_MBX_DATA4_ADDR(VEDU_IPC_BASE, mbx_id));
	writel(msg->data[4], SOC_IPC_VRCE_MBX_DATA5_ADDR(VEDU_IPC_BASE, mbx_id));
	writel(msg->data[5], SOC_IPC_VRCE_MBX_DATA6_ADDR(VEDU_IPC_BASE, mbx_id));
	writel(msg->data[6], SOC_IPC_VRCE_MBX_DATA7_ADDR(VEDU_IPC_BASE, mbx_id));
	// step 5
	writel(BIT(ACPU_IPC_ID), SOC_IPC_VRCE_MBX_SEND_ADDR(VEDU_IPC_BASE, mbx_id)); // value must be same with MBX_SOURCE
	return 0;
}

static void mcore_ipc_mbx0_int(int32_t irq)
{
	VCODEC_INFO_VENC("ipc_mbx0 irq number:%d", irq);
	venc_ipc_recive_msg(MBX0);
}

static void mcore_ipc_int0_int(int32_t irq)
{
	VCODEC_INFO_VENC("ipc_int0 irq number:%d", irq);
	venc_ipc_recive_ack(MBX2);
}

static void mcore_watchdog_int(int32_t irq)
{
	VCODEC_FATAL_VENC("watchdog irq number:%d", irq);
	// when watchdog used, need add code to deal watchdog event
}

void disable_all_level2_int(uint8_t *intr_hub_reg_base)
{
	int32_t n = 1; // normal:1 secure:2
	writel(0xFFFFFFFF, SOC_INTR_HUB_L2_INTR_MASKCLR_S_n_ADDR(intr_hub_reg_base, n));
}

void clear_all_level2_int(uint8_t *intr_hub_reg_base)
{
	int32_t n = 1; // normal:1 secure:2
	writel(0xFFFFFFFF, SOC_INTR_HUB_L2_INTR_MASKCLR_S_n_ADDR(intr_hub_reg_base, n));
}

// level2 irqs triggerd by mcu
irqreturn_t venc_drv_mcore_irq(int32_t irq, void *dev_id)
{
	int32_t i = 0;
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());

	for (i = 0; i < MAX_SUPPORT_CORE_NUM; i++) {
		if (venc->ctx[i].irq_num.mcore_normal == irq)
			break;
	}

	if (i == MAX_SUPPORT_CORE_NUM) {
		VCODEC_FATAL_VENC("isr not registered");
		return IRQ_HANDLED;
	}

	int32_t n = 1; // normal:1 secure:2
	uint32_t status = readl(SOC_INTR_HUB_L2_INTR_STATUS_S_n_ADDR(venc->ctx[i].intr_hub_reg_base, n));
	VCODEC_INFO_VENC("current cord_id is %d, isr status:0x%x", i, status);
	if (status == BIT(INTR_VENC_IPC_INT_0_TO_GIC))
		mcore_ipc_int0_int(irq);
	else if (status == BIT(INTR_VENC_IPC_MBX_0_TO_GIC))
		mcore_ipc_mbx0_int(irq);
	else if (status == BIT(INTR_VENC_WATCHDOG_TO_GIC))
		mcore_watchdog_int(irq);
	else
		VCODEC_FATAL_VENC("unsupported isr %d", status);

	return IRQ_HANDLED;
}

int32_t venc_ipc_init(struct venc_context *ctx)
{
	unused(ctx);
	return 0;
}

void venc_ipc_deinit(struct venc_context *ctx)
{
	unused(ctx);
}
