/*
 * drv_venc_mcore.c
 *
 * This is for Operations Related to mcore.
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

#include "drv_venc_mcore.h"
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/io.h>
#include "soc_venc_reg_interface.h"
#include "drv_common.h"
#include "drv_venc_osal.h"
#include "drv_mem.h"
#include "venc_regulator.h"
#include "drv_venc_mcore_tcm.h"
#include "drv_venc_ipc.h"

#define REG_VEDU_VCPI_SWPTRADDR_H  (VEDU_REG_BASE + 0x09d4)
#define MCORE_IMG_SIZE (32 * 1024)
#define MCORE_VERSION 10
#define MCORE_DDR_DATA_SIZE (9*1024*1024) // for rate control algrithom
#define MCORE_DDR_LOG_SIZE (7*1024*1024) // for mcore log recording
#define MCORE_DDR_SIZE (MCORE_DDR_DATA_SIZE + MCORE_DDR_LOG_SIZE)
#define VENC_MCORE_IMG  "/data/venc/sec_venc_mcore.img"

int32_t venc_mcore_load_image(void)
{
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());

	venc->mcore_image = vcodec_mem_valloc(MCORE_IMG_SIZE);
	if (!venc->mcore_image) {
		VCODEC_ERR_VENC("alloc memory failed");
		return VCODEC_FAILURE;
	}
	// verify and copy image from rom

	return 0;
}

static int32_t copy_img_to_tcm(void)
{
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());
	uint32_t *vir = venc->mcore_image;
	if (!vir) {
		VCODEC_ERR_VENC("image memory not init");
		return VCODEC_FAILURE;
	}

	uint32_t size = vir[0];
	uint32_t *fp = vir + 1;
	uint32_t *dest = (uint32_t *)VRCE_SRAM_BASE;

	if (size > MCORE_IMG_SIZE - sizeof(uint32_t)) {
		VCODEC_ERR_VENC("image size error %d", size);
		return VCODEC_FAILURE;
	}

	for (int32_t i = 0; i < size / sizeof(uint32_t); ++i)
		dest[i] = fp[i];

	VCODEC_INFO_VENC("Load mcu image size %d...End", size);
	return 0;
}

static struct venc_dma_mem g_mcore_databuf;
static int32_t setup_mcore_memory(void)
{
	int32_t ret;
	if (!g_mcore_databuf.virt_addr) {
		ret = drv_mem_alloc_dma(&(venc_get_device()->dev), MCORE_DDR_SIZE, &g_mcore_databuf);
		if (ret != 0)
			return ret;
	}

	(void)memset_s(g_mcore_databuf.virt_addr, MCORE_DDR_SIZE, 0, MCORE_DDR_SIZE);
	uint32_t *addr = (uint32_t *)(REG_VEDU_VCPI_SWPTRADDR_H);
	*addr = g_mcore_databuf.iova_addr;
	return 0;
}

int32_t venc_mcore_open(void)
{
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());
#ifdef VENC_DEBUG_ENABLE
	if (!(venc->debug_flag & (1LL << MCORE_ENABLE)))
		return 0;
#endif
	VCODEC_INFO_VENC("mcore start version:%d", MCORE_VERSION);
	if (venc_ipc_init(&venc->ctx[0]) != 0)
		return VCODEC_FAILURE;

	S_VE_SUB_CTRL_REGS_TYPE *ctl = (S_VE_SUB_CTRL_REGS_TYPE *)VEDU_SUB_CTRL_BASE;
	{
		U_SUBCTRL_CRG0 reg;
		reg.bits.mcu_por_rst = 1;
		reg.bits.mcu_wdt_rst = 1;
		ctl->SUBCTRL_CRG0.u32 = reg.u32;
	}

	{
		U_SUBCTRL_CRG3 reg;
		reg.bits.tim_clk_en = 1;
		reg.bits.ipc_clk_en = 1;
		ctl->SUBCTRL_CRG3.u32 = reg.u32;
	}

	// start address
	ctl->SUBCTRL_MCU_POR_PC = venc_get_mcore_code_base();

	// 1 set mcu_core_wait = 1, then mcu enter wait state
	{
		U_SUBCTRL_CRG0 reg;
		reg.bits.mcu_core_wait = 1;
		ctl->SUBCTRL_CRG0.u32 = reg.u32;
	}

	// 2 set mcu_por_rst = 0, then mcu exit reset state
	ctl->SUBCTRL_CRG0.bits.mcu_por_rst = 0; // mcu_core_wait should be 1 in the mean time

	// 3 load mcu image to ITCM by AP
	if (copy_img_to_tcm() != 0)
		goto ERROR;

	if (setup_mcore_memory() != 0)
		goto ERROR;

	// 4 set mcu_core_wait = 0, then start mcu core
	{
		U_SUBCTRL_CRG0 reg;
		reg.bits.mcu_core_wait = 0;
		ctl->SUBCTRL_CRG0.u32 = reg.u32;
	}
	VCODEC_INFO_VENC("end");
	udelay(1000);
	return 0;
ERROR:
	venc_ipc_deinit(&venc->ctx[0]);
	return VCODEC_FAILURE;
}

void venc_mcore_close(void)
{
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());
#ifdef VENC_DEBUG_ENABLE
	if (!(venc->debug_flag & (1LL << MCORE_ENABLE)))
		return;
#endif
	S_VE_SUB_CTRL_REGS_TYPE *ctl = (S_VE_SUB_CTRL_REGS_TYPE *)VEDU_SUB_CTRL_BASE;

	// stop mcore
	U_SUBCTRL_CRG1 reg;
	reg.bits.mcu_clk_en = 0;
	ctl->SUBCTRL_CRG1.u32 = reg.u32;

	// release memory
	if (g_mcore_databuf.virt_addr) {
		drv_mem_free_dma(&(venc_get_device()->dev), &g_mcore_databuf);
		(void)memset_s(&g_mcore_databuf, sizeof(g_mcore_databuf), 0, sizeof(g_mcore_databuf));
	}

	venc_ipc_deinit(&venc->ctx[0]);
	VCODEC_INFO_VENC("mcu closed");
}

void *venc_regbase(void)
{
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());
	return venc->ctx[0].reg_base;
}

int32_t venc_mcore_load_image_to_mem(void)
{
#ifdef VENC_DEBUG_ENABLE
	char *fp = NULL;
	loff_t size;
	int err = kernel_read_file_from_path(VENC_MCORE_IMG, (void **)&fp, &size, MCORE_IMG_SIZE, READING_FIRMWARE);
	if (err < 0) {
		VCODEC_ERR_VENC("read file error %d", err);
		return VCODEC_FAILURE;
	}
	VCODEC_INFO_VENC("Load mcu image, size=%d 0x%x-0x%x", size, fp[0], fp[size - 1]);

	if (size % 4 != 0 || size > MCORE_IMG_SIZE - sizeof(uint32_t)) {
		VCODEC_ERR_VENC("mcore img size error %d", size);
		return VCODEC_FAILURE;
	}

	venc_entry_t *venc = platform_get_drvdata(venc_get_device());
	uint32_t *vir = venc->mcore_image;
	if (!vir) {
		VCODEC_ERR_VENC("image memory not init");
		vfree(fp);
		return VCODEC_FAILURE;
	}

	vir[0] = size;
	(void)memcpy_s(vir + 1, MCORE_IMG_SIZE - sizeof(uint32_t), fp, size);

	VCODEC_INFO_VENC("Load mcu image...End,0x%x-0x%x", vir[0], vir[size / sizeof(uint32_t) - 1]);
	vfree(fp);
#endif
	return 0;
}

void venc_mcore_log_dump()
{
#ifdef VENC_DEBUG_ENABLE
	venc_entry_t *venc = platform_get_drvdata(venc_get_device());
	if (!(venc->debug_flag & (1LL << MCORE_ENABLE)))
		return;
	char log[2048];
	int32_t *src = g_mcore_databuf.virt_addr;

	if (!src) {
		VCODEC_FATAL_VENC("mcore is not init");
		return;
	}

	(void)memcpy_s(log, sizeof(log), src + 2, sizeof(log));
	log[sizeof(log) - 1] = 0;

	int32_t start = src[0];
	int32_t len = src[1];
	VCODEC_INFO_VENC("mcore log start %d, size %d: %s", start, len, log);
	if (start != 0 || len > MCORE_DDR_LOG_SIZE) {
		VCODEC_WARN_VENC("mcore log buffer maybe overwrite");
		len = MCORE_DDR_LOG_SIZE;
	}

	struct file *f = osal_fopen("/data/venc/mcu.log", O_CREAT | O_WRONLY | O_TRUNC, 0600);
	if (!f) {
		VCODEC_FATAL_VENC("mcore dump log failed");
		return;
	}
	osal_fwrite(f, (char *)(src + 2), len);
	osal_fclose(f);
#endif
}
