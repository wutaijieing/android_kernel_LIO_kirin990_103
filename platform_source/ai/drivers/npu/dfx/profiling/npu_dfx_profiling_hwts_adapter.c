/*
 * npu_dfx_profiling.c
 *
 * about npu dfx profiling
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
#include <linux/err.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/barrier.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/errno.h>

#include "npu_log.h"
#include "npu_dfx.h"
#include <linux/io.h>
#include <securec.h>
#include "npu_platform_resource.h"
#include "npu_dfx_profiling.h"

char *get_prof_channel_data_addr(struct prof_buff_desc *prof_buff, u32 channel)
{
	cond_return_error(prof_buff == NULL, NULL, "prof_buff is null\n");
	cond_return_error(channel >= PROF_TYPE_CNT, NULL, "invalid channel\n");

	return (char *)(uintptr_t)(
		(vir_addr_t)prof_buff->head.manager.ring_buff[channel].base_addr_l +
		((vir_addr_t)prof_buff->head.manager.ring_buff[channel].base_addr_h << 32));
}

u32 get_prof_channel_data_size(struct prof_buff_desc *prof_buff, u32 channel)
{
	cond_return_error(prof_buff == NULL, 0, "prof_buff is null\n");
	switch (channel) {
	case PROF_CHANNEL_TSCPU:
		return PROF_TSCPU_DATA_SIZE;
	case PROF_CHANNEL_HWTS_LOG:
		return PROF_HWTS_LOG_SIZE;
	case PROF_CHANNEL_HWTS_PROFILING:
		return PROF_HWTS_PROFILING_SIZE;
	default:
		npu_drv_err("channel: %u error\n", channel);
	}
	return 0;
}

static int prof_buff_alloc(u32 len, vir_addr_t *virt_addr, unsigned long *iova)
{
	int ret = 0;
	if (virt_addr == NULL) {
		npu_drv_err("virt_addr is null\n");
		return -1;
	}

#ifndef CONFIG_NPU_PHY_MEM
	if (iova == NULL) {
		npu_drv_err("iova is null\n");
		return -1;
	}

	ret = npu_iova_alloc(virt_addr, len);
	if (ret != 0) {
		npu_drv_err("fail, ret=%d\n", ret);
		return ret;
	}
	ret = npu_iova_map(*virt_addr, iova);
	if (ret != 0) {
		npu_drv_err("prof_buff_map fail, ret=%d\n", ret);
		npu_iova_free(*virt_addr);
	}
#else
	*virt_addr = NPU_NS_PROF_BASE_ADDR;
#endif
	return ret;
}

void prof_buff_free(vir_addr_t virt_addr, unsigned long iova)
{
	int ret = 0;

	ret = npu_iova_unmap(virt_addr, iova);
	if (ret != 0) {
		npu_drv_err("npu_iova_unmap fail, ret=%d\n", ret);
		return;
	}
	npu_iova_free(virt_addr);
}

int prof_buffer_init(struct prof_buff_desc *prof_buff)
{
	int ret;
	int channel;
	vir_addr_t addr[PROF_CHANNEL_MAX];

	cond_return_error(prof_buff == NULL, PROF_ERROR, "prof buf nullptr\n");

	/* init tscpu data buffer */
	ret = prof_buff_alloc(PROF_TSCPU_DATA_SIZE, &addr[PROF_CHANNEL_TSCPU],
		(unsigned long *)&(prof_buff->head.manager.ring_buff[PROF_CHANNEL_TSCPU].iova_addr));
	if (ret != 0) {
		npu_drv_err("prof_buff_alloc fail,goal = tscpu_data,size = 0x%x\n",
			PROF_TSCPU_DATA_SIZE);
		return PROF_ERROR;
	}

	/* init hwts log data buffer */
	ret = prof_buff_alloc(PROF_HWTS_LOG_SIZE, &addr[PROF_CHANNEL_HWTS_LOG],
		(unsigned long *)&(prof_buff->head.manager.ring_buff[PROF_CHANNEL_HWTS_LOG].iova_addr));
	if (ret != 0) {
		prof_buff_free(addr[PROF_CHANNEL_TSCPU],
			prof_buff->head.manager.ring_buff[PROF_CHANNEL_TSCPU].iova_addr);
		npu_drv_err("prof_buff_alloc fail, goal = tscpu_data,size = 0x%x\n",
			PROF_HWTS_LOG_SIZE);
		return PROF_ERROR;
	}

	/* init hwts profiling data buffer */
	ret = prof_buff_alloc(PROF_HWTS_PROFILING_SIZE, &addr[PROF_CHANNEL_HWTS_PROFILING],
		(unsigned long *)&(prof_buff->head.manager.ring_buff[PROF_CHANNEL_HWTS_PROFILING].iova_addr));
	if (ret != 0) {
		prof_buff_free(addr[PROF_CHANNEL_TSCPU],
			prof_buff->head.manager.ring_buff[PROF_CHANNEL_TSCPU].iova_addr);
		prof_buff_free(addr[PROF_CHANNEL_HWTS_LOG],
			prof_buff->head.manager.ring_buff[PROF_CHANNEL_HWTS_LOG].iova_addr);
		npu_drv_err("prof_buff_alloc fail,goal = tscpu_data,size = 0x%x\n",
			PROF_HWTS_PROFILING_SIZE);
		return PROF_ERROR;
	}

	prof_buff->head.manager.ring_buff[PROF_CHANNEL_TSCPU].length =
		PROF_TSCPU_DATA_SIZE; /* no used but for consistency with other two buffer */
	prof_buff->head.manager.ring_buff[PROF_CHANNEL_HWTS_LOG].length =
		PROF_HWTS_LOG_SIZE / PROF_HWTS_LOG_FORMAT_SIZE; /* slot num for hwts global register config */
	prof_buff->head.manager.ring_buff[PROF_CHANNEL_HWTS_PROFILING].length =
		PROF_HWTS_PROFILING_SIZE / PROF_HWTS_PROFILING_FORMAT_SIZE; /* slot num for hwts global register config */

	for (channel = PROF_CHANNEL_TSCPU; channel < PROF_CHANNEL_MAX; channel++) {
		prof_buff->head.manager.ring_buff[channel].base_addr_l = addr[channel] & 0xffffffffUL;
		prof_buff->head.manager.ring_buff[channel].base_addr_h = (addr[channel] >> 32) & 0xffffffffUL;
	}

	return PROF_OK;
}

int prof_buffer_release(struct prof_buff_desc *prof_buff)
{
	int i;
	vir_addr_t virt_addr;

	cond_return_error(prof_buff == NULL, PROF_ERROR, "prof buf nullptr\n");

	for (i = PROF_CHANNEL_TSCPU; i < PROF_CHANNEL_MAX; i++) {
		virt_addr = (vir_addr_t)prof_buff->head.manager.ring_buff[i].base_addr_l +
			((vir_addr_t)prof_buff->head.manager.ring_buff[i].base_addr_h << 32);
		prof_buff_free(virt_addr, prof_buff->head.manager.ring_buff[i].iova_addr);
		prof_buff->head.manager.ring_buff[i].base_addr_l = 0;
		prof_buff->head.manager.ring_buff[i].base_addr_h = 0;
		prof_buff->head.manager.ring_buff[i].iova_addr = 0;
	}
	return PROF_OK;
}
