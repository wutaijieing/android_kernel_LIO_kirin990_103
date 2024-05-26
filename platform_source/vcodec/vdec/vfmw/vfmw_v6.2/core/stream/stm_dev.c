/*
 * stm_dev.c
 *
 * This is for decoder.
 *
 * Copyright (c) 2012-2020 Huawei Technologies CO., Ltd.
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

#include "stm_dev.h"
#include "scd_hal.h"
#include "vfmw_pdt.h"

#define STM_DEV_WAIT_COUNT 20
#define STM_DEV_WAIT_TIME 10

static stm_dev g_stm_dev_entry;
stm_dev *stm_dev_get_dev(void)
{
	return &g_stm_dev_entry;
}

int32_t stm_dev_isr(void)
{
	int32_t irq_handled = 1;

	stm_dev *dev = stm_dev_get_dev();

	scd_hal_read_reg(dev);
	scd_hal_clear_int();
	OS_EVENT_GIVE(dev->event);

	return irq_handled;
}

void stm_dev_init_entry(void)
{
	stm_dev *dev = stm_dev_get_dev();

	(void)memset_s(dev, sizeof(stm_dev), 0, sizeof(stm_dev));
	dev->state = STM_DEV_STATE_NULL;
}

void stm_dev_deinit_entry(void)
{
	stm_dev *dev = stm_dev_get_dev();
	(void)memset_s(dev, sizeof(stm_dev), 0, sizeof(stm_dev));
}

int32_t stm_dev_init(void *args)
{
	int32_t ret;
	stm_dev *dev = stm_dev_get_dev();
	vfmw_module_reg_info *reg_info = (vfmw_module_reg_info *)args;

	OS_EVENT_INIT(&dev->event, 0);
	OS_SEMA_INIT(&dev->sema);
	dev->reg_phy = reg_info->reg_phy_addr;
	dev->reg_size = reg_info->reg_range;
	dev->poll_irq_enable = 0;

	ret = scd_hal_open(dev);
	if (ret == STM_ERR)
		return ret;

	dev->state = STM_DEV_STATE_IDLE;

	return ret;
}

void stm_dev_deinit(void)
{
	stm_dev *dev = stm_dev_get_dev();

	if (dev->state == STM_DEV_STATE_NULL)
		return;

	OS_EVENT_GIVE(dev->event);
	scd_hal_close(dev);

	OS_EVENT_EXIT(dev->event);
	OS_SEMA_EXIT(dev->sema);
	dev->state = STM_DEV_STATE_IDLE;
}

static int32_t stm_dev_poll_irq(stm_dev *dev)
{
	struct vfmw_global_info *glb_info = pdt_get_glb_info();
	int32_t scd_time_out = (glb_info->is_fpga ? SCD_FPGA_TIME_OUT : SCD_TIME_OUT);
	int32_t poll_interval_ms = (glb_info->is_fpga ? 50 : 5);
	int32_t sleep_time = 0;

	while ((scd_hal_isr_state() != STM_OK)) {
		if (sleep_time > scd_time_out) {
			uint32_t cycle = 0;
			rd_vreg(dev->reg_vir, REG_RUN_CYCLE, cycle);
			dprint(PRN_ERROR, "scd timeout, scd cycle is %x\n", cycle);
			return OSAL_ERR;
		}
		OS_MSLEEP(poll_interval_ms);
		sleep_time += poll_interval_ms;
	}
	scd_hal_read_reg(dev);
	scd_hal_clear_int();

	return OSAL_OK;
}

static int32_t stm_dev_block_wait(stm_dev *dev)
{
	struct vfmw_global_info *glb_info = pdt_get_glb_info();
	int32_t scd_time_out = (glb_info->is_fpga ? SCD_FPGA_TIME_OUT : SCD_TIME_OUT);

	if (dev->poll_irq_enable)
		return stm_dev_poll_irq(dev);

	return OS_EVENT_WAIT(dev->event, scd_time_out);
}

int32_t stm_dev_config_reg(void *stm_cfg, void *scd_state_reg)
{
	int32_t ret;
	stm_dev *dev = stm_dev_get_dev();
	scd_reg_ioctl *reg_config = (scd_reg_ioctl *)stm_cfg;
	scd_reg *reg_back = (scd_reg *)scd_state_reg;

	uint64_t start_time = OS_GET_TIME_US();

	OS_SEMA_DOWN(dev->sema);

	if (dev->state != STM_DEV_STATE_IDLE) {
		dprint(PRN_ERROR, "state error\n");
		OS_SEMA_UP(dev->sema);
		return STM_ERR;
	}

	scd_hal_write_reg(reg_config);
	scd_hal_start(dev);
	ret = stm_dev_block_wait(dev);
	if (ret != OSAL_OK) {
		dprint(PRN_ERROR, "scd timeout, reset now\n");
		scd_hal_reset(dev);
		dev->state = STM_DEV_STATE_IDLE;
		OS_SEMA_UP(dev->sema);
		return STM_ERR;
	}

	(void)memcpy_s(reg_back, sizeof(scd_reg), &dev->reg, sizeof(scd_reg));
	dev->state = STM_DEV_STATE_IDLE;

	OS_SEMA_UP(dev->sema);

	if (dev->perf_enable)
		dprint(PRN_ALWS, "scd take time %u us\n",
			OS_GET_TIME_US() - start_time);

	return ret;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static int32_t stm_dev_cancel(void)
{
	stm_dev *dev = stm_dev_get_dev();
	int32_t ret;

	OS_SEMA_DOWN(dev->sema);

	if (dev->state == STM_DEV_STATE_NULL) {
		ret = STM_ERR;
		goto exit;
	}

	ret = scd_hal_cancel();
	if (ret != STM_OK)
		goto exit;
	dev->state = STM_DEV_STATE_IDLE;

exit:
	OS_SEMA_UP(dev->sema);

	return ret;
}
#pragma GCC diagnostic pop


void stm_dev_suspend(void)
{
	stm_dev *dev = stm_dev_get_dev();
	int32_t wait_count = 0;

	OS_SEMA_DOWN(dev->sema);
	do {
		if (dev->state == STM_DEV_STATE_IDLE ||
			dev->state == STM_DEV_STATE_NULL)
			break;

		if (wait_count == STM_DEV_WAIT_COUNT) {
			scd_hal_reset(dev);
			break;
		}

		wait_count++;
		OS_MSLEEP(STM_DEV_WAIT_TIME);
	} while (dev->state != STM_DEV_STATE_IDLE);

	OS_SEMA_UP(dev->sema);
}

void stm_dev_resume(void)
{
}
