/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2015-2020. All rights reserved.
 * Description: jpeg encode
 * Create: 2015-10-09
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 */

#include <linux/of.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/proc_fs.h>
#include <linux/wait.h>
#include <linux/videodev2.h>
#include <linux/iommu.h>
#include <linux/platform_device.h>
#include <linux/of_irq.h>
#include <media/v4l2-fh.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/fs.h>
#include <linux/dma-mapping.h>
#include <platform_include/camera/native/hjpeg_cfg.h>
#include <asm/io.h>
#include <linux/platform_drivers/mm_ion.h>
#include <asm/uaccess.h>
#include <linux/rpmsg.h>
#include <platform_include/isp/linux/hisp_remoteproc.h>
#include <linux/version.h>

#include "hjpgenc150.h"
#include "hjpeg_intf.h"
#include "hjpg150_reg_offset.h"
#include "hjpg150_reg_offset_field.h"
#include "hjpg150_table.h"
#include "cam_log.h"

#define SMMU 1

#ifndef SMMU
#define JPEG_INPUT_PHYADDR 0x40000000
#define JPEG_OUTPUT_PHYADDR 0x50000000
#define PREFETCH_BY_PASS (1 << 31)
#endif

static void hjpeg150_isr_do_tasklet(unsigned long data);
DECLARE_TASKLET(hjpeg150_isr_tasklet,
	hjpeg150_isr_do_tasklet, (unsigned long)0);

typedef struct _tag_hjpeg150_hw_ctl {
	void __iomem *base_viraddr;
	phys_addr_t  phyaddr;
	u32          mem_size;
	u32          irq_no;

	void __iomem *cvdr_viraddr;
	phys_addr_t  cvdr_phyaddr;
	u32          cvdr_mem_size;
} hjpeg150_hw_ctl_t;

typedef struct _tag_hjpeg150 {
	struct platform_device *pdev;
	hjpeg_intf_t           intf;
	char const             *name;

	void __iomem           *viraddr;
	phys_addr_t            phyaddr;
	u32                    mem_size;
	u32                    irq_no;
	struct semaphore       buff_done;
	hjpeg150_hw_ctl_t      hw_ctl;
	struct ion_client      *ion_client;
	bool                   power_on_state;
} hjpeg150_t;

#define i_2_hjpeg_150(i) container_of(i, hjpeg150_t, intf)

static int hjpeg150_encode_process(hjpeg_intf_t *i, void *cfg);
static int hjpeg150_power_on(hjpeg_intf_t *i);
static int hjpeg150_power_off(hjpeg_intf_t *i);
static int hjpeg150_get_reg(hjpeg_intf_t *i, void *cfg);
static int hjpeg150_set_reg(hjpeg_intf_t *i, void *cfg);

static hjpeg_vtbl_t s_vtbl_hjpeg150 = {
	.encode_process = hjpeg150_encode_process,
	.power_on = hjpeg150_power_on,
	.power_down = hjpeg150_power_off,
	.get_reg = hjpeg150_get_reg,
	.set_reg = hjpeg150_set_reg,
};

static hjpeg150_t s_hjpeg150 = {
	.intf = {.vtbl = &s_vtbl_hjpeg150,},
	.name = "hjpeg150",
};

static const struct of_device_id s_hjpeg150_dt_match[] = {
	{
		.compatible = "vendor,hjpeg150",
		.data = &s_hjpeg150.intf,
	},
	{},
};

static struct timeval s_timeval_start;
static struct timeval s_timeval_end;

MODULE_DEVICE_TABLE(of, s_hjpeg150_dt_match);

static void hjpeg150_isr_do_tasklet(unsigned long data)
{
	up(&s_hjpeg150.buff_done);
}

#define ENCODE_FINISH (1 << 4)
static irqreturn_t hjpeg150_irq_handler(int irq, void *dev_id)
{
	void __iomem *base = s_hjpeg150.viraddr;
	u32 value;

	do_gettimeofday(&s_timeval_end);
	value = reg_get(base + JPGENC_JPE_STATUS_RIS_REG);
	if (value & ENCODE_FINISH)
		tasklet_schedule(&hjpeg150_isr_tasklet);
	else
		cam_err("%s %d err irq status 0x%x",
			__func__, __LINE__, value);

	/* clr jpeg irq */
	reg_set(base + JPGENC_JPE_STATUS_ICR_REG, 0x30);
	return IRQ_HANDLED;
}

static int dump_dbg_reg(void)
{
#ifdef DUMP_REGS
	int i;
	const int offsize = 4;
	void __iomem *base = s_hjpeg150.viraddr;

	for (i = 0; i <= JPGENC_FORCE_CLK_ON_CFG_REG;) {
		cam_info("jpeg reg 0x%x = 0x%x", i, reg_get(base + i));
		i += offsize;
	}
	for (i = JPGENC_DBG_0_REG; i <= JPGENC_DBG_13_REG;) {
		cam_info("dbg reg 0x%x = 0x%x", i, reg_get(base + i));
		i += offsize;
	}
#endif
	return 0;
}

static int dump_cvdr_reg(void)
{
#ifdef DUMP_REGS
	void __iomem *base = s_hjpeg150.hw_ctl.cvdr_viraddr;
	void __iomem *smmu_base_addr;

	cam_info("CVDR_SRT_VP_WR_CFG_25 = 0x%x",
		reg_get(base + CVDR_SRT_VP_WR_CFG_25_OFFSET));
	cam_info("CVDR_SRT_VP_WR_AXI_LINE_25 = 0x%x",
		reg_get(base + CVDR_SRT_VP_WR_AXI_LINE_25_OFFSET));
	cam_info("CVDR_SRT_VP_WR_AXI_FS_25 = 0x%x",
		reg_get(base + CVDR_SRT_VP_WR_AXI_FS_25_OFFSET));
	cam_info("CVDR_SRT_LIMITER_VP_WR_25 = 0x%x",
		reg_get(base + CVDR_SRT_LIMITER_VP_WR_25_OFFSET));
	cam_info("CVDR_SRT_NR_RD_CFG_1 = 0x%x",
		reg_get(base + CVDR_SRT_NR_RD_CFG_1_OFFSET));
	cam_info("CVDR_SRT_LIMITER_NR_RD_1 = 0x%x",
		reg_get(base + CVDR_SRT_LIMITER_NR_RD_1_OFFSET));
	cam_info("CVDR_SRT_VP_WR_IF_CFG_25 = 0x%x",
		reg_get(base + CVDR_SRT_VP_WR_IF_CFG_25_OFFSET));

	/* dump SMMU regs */
	/* 8192: source size */
	smmu_base_addr = ioremap_nocache(SMMU_BASE_ADDR, 8192);
	if (IS_ERR_OR_NULL(smmu_base_addr)) {
		cam_err("%s %d remap failed", __func__, __LINE__);
		return -1;
	}
	cam_info("SMMU_GLOBAL_BYPASS = 0x%x",
		reg_get(smmu_base_addr + SMMU_GLOBAL_BYPASS));
	cam_info("SMMU_BYPASS_VPWR = 0x%x",
		reg_get(smmu_base_addr + SMMU_BYPASS_VPWR));
	cam_info("SMMU_BYPASS_NRRD1 = 0x%x",
		reg_get(smmu_base_addr + SMMU_BYPASS_NRRD1));
	cam_info("SMMU_BYPASS_NRRD2 = 0x%x",
		reg_get(smmu_base_addr + SMMU_BYPASS_NRRD2));
	iounmap(smmu_base_addr);
#endif
	return 0;
}

#ifndef SMMU
void hjpeg150_dump_file(char *filename, unsigned long addr, u32 size)
{
#if DUMP_FILE
	struct file *file1 = NULL;
	mm_segment_t old_fs;
	(void)memset_s(&old_fs, sizeof(old_fs), 0x0, sizeof(old_fs));

	if (!filename) {
		cam_err("%s: filename is NULL. %d", __func__, __LINE__);
		return;
	}

	cam_info("dumpfile %s with size %u", filename, size);
	if (!filename) {
		printk("dump file with NULL file name!");
		return;
	}
	file1 = filp_open(filename, O_CREAT | O_RDWR, 0644);
	if (IS_ERR(file1)) {
		cam_err("error occured while opening file %s, exiting...",
			filename);
		return;
	} else {
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		file1->f_op->write(file1, (char*)addr, size, &file1->f_pos);
		set_fs(old_fs);
		filp_close(file1, NULL);
		return;
	}
#endif
}
#endif

static int hjpeg150_read_phyaddr_property(struct device_node *dev_node,
	uint32_t *base_array, uint32_t count)
{
	int ret;
	if (dev_node) {
		ret = of_property_read_u32_array(dev_node,
			"vendor,hjpeg150-base", base_array, count);
		if (ret < 0) {
			cam_err("%s failed line %d", __func__, __LINE__);
			return ret;
		}
	} else {
		cam_err("%s hjpeg150 of_node is NULL.%d",
			__func__, __LINE__);
		ret = -ENXIO;
	}

	return ret;
}

static int hjpeg150_res_init(struct device *pdev)
{
	struct device_node *dev_node = pdev->of_node;
	uint32_t base_array[2] = {0}; /* 2: phyaddr & mem_size */
	/* property(vendor,isp-base) = <address, size>, so count is 2 */
	uint32_t count = 2;
	int ret;

	ret = hjpeg150_read_phyaddr_property(dev_node, base_array, count);
	if (ret < 0)
		return ret;

	s_hjpeg150.phyaddr = base_array[0];
	s_hjpeg150.mem_size = base_array[1];

	s_hjpeg150.viraddr =
		ioremap_nocache(s_hjpeg150.phyaddr, s_hjpeg150.mem_size);

	if (IS_ERR_OR_NULL(s_hjpeg150.viraddr)) {
		cam_err("%s ioremap fail", __func__);
		return -ENXIO;
	}

	s_hjpeg150.irq_no = irq_of_parse_and_map(dev_node, 0);
	if (s_hjpeg150.irq_no <= 0) {
		cam_err("%s failed line %d", __func__, __LINE__);
		goto fail;
	}

	/* request irq */
	ret = request_irq(s_hjpeg150.irq_no, hjpeg150_irq_handler,
		0, "hjpeg150_irq", 0);
	if (ret != 0) {
		cam_err("fail to request irq [%d], error: %d",
			s_hjpeg150.irq_no, ret);
		ret = -ENXIO;
		goto fail;
	}

	/* 4096: map source size */
	s_hjpeg150.hw_ctl.cvdr_viraddr = ioremap_nocache(CVDR_SRT_BASE, 4096);
	if (IS_ERR_OR_NULL(s_hjpeg150.hw_ctl.cvdr_viraddr)) {
		cam_err("%s %d remap cvdr viraddr failed",
			__func__, __LINE__);
		ret = -ENXIO;
		goto fail;
	}

	return 0;

fail:
	if (s_hjpeg150.viraddr) {
		iounmap((void*)s_hjpeg150.viraddr);
		s_hjpeg150.viraddr = NULL;
	}

	if (s_hjpeg150.irq_no) {
		free_irq(s_hjpeg150.irq_no, 0);
		s_hjpeg150.irq_no = 0;
	}

	return ret;
}

static int hjpeg150_res_deinit(struct device *pdev)
{
	if (s_hjpeg150.viraddr) {
		iounmap((void*)s_hjpeg150.viraddr);
		s_hjpeg150.viraddr = NULL;
	}

	if (s_hjpeg150.irq_no) {
		free_irq(s_hjpeg150.irq_no, 0);
		s_hjpeg150.irq_no = 0;
	}

	if (s_hjpeg150.hw_ctl.cvdr_viraddr)
		iounmap((void*)s_hjpeg150.hw_ctl.cvdr_viraddr);

	return 0;
}

static int hjpeg150_enable_autogating(void)
{
	void __iomem *base_addr = s_hjpeg150.viraddr;

	reg_set(base_addr + JPGENC_FORCE_CLK_ON_CFG_REG, 0x0);

	return 0;
}

static int hjpeg150_disable_autogating(void)
{
	void __iomem *base_addr = s_hjpeg150.viraddr;

	reg_set(base_addr + JPGENC_FORCE_CLK_ON_CFG_REG, 0x1);

	return 0;
}

static void hjpeg150_hufftable_luma_info(void __iomem *base_addr)
{
	int tmp;
	uint32_t i;
	uint32_t length_bit, length_value, length;

	/* DC 0 */
	length_bit = ARRAY_SIZE(luma_dc_bits);
	length_value = ARRAY_SIZE(luma_dc_value);
	length = length_bit + length_value;

	reg_set(base_addr + JPGENC_JPE_TDC0_LEN_REG, length);
	reg_set(base_addr + JPGENC_JPE_TABLE_ID_REG, 4); /* 4: table id */
	for (i = 1; i < length_bit; i = i + 2) { /* 2: offset */
		tmp = 0;
		reg_set_field(tmp, JPGENC_TABLE_WDATA_H, luma_dc_bits[i - 1]);
		reg_set_field(tmp, JPGENC_TABLE_WDATA_L, luma_dc_bits[i]);
		reg_set(base_addr + JPGENC_JPE_TABLE_DATA_REG,tmp);
	}
	for (i = 1; i < length_value; i += 2) { /* 2: offset */
		tmp = 0;
		reg_set_field(tmp, JPGENC_TABLE_WDATA_H, luma_dc_value[i - 1]);
		reg_set_field(tmp, JPGENC_TABLE_WDATA_L, luma_dc_value[i]);
		reg_set(base_addr + JPGENC_JPE_TABLE_DATA_REG, tmp);
	}

	/* AC 0 */
	length_bit = ARRAY_SIZE(luma_ac_bits);
	length_value = ARRAY_SIZE(luma_ac_value);
	length = length_bit + length_value;
	reg_set(base_addr + JPGENC_JPE_TAC0_LEN_REG, length);
	reg_set(base_addr + JPGENC_JPE_TABLE_ID_REG, 5); /* 5: table id */
	for (i = 1; i < length_bit; i = i + 2) { /* 2: offset */
		tmp = 0;
		reg_set_field(tmp, JPGENC_TABLE_WDATA_H, luma_ac_bits[i - 1]);
		reg_set_field(tmp, JPGENC_TABLE_WDATA_L, luma_ac_bits[i]);
		reg_set(base_addr + JPGENC_JPE_TABLE_DATA_REG, tmp);
	}
	for (i = 1; i < length_value; i = i + 2) { /* 2: offset */
		tmp = 0;
		reg_set_field(tmp, JPGENC_TABLE_WDATA_H, luma_ac_value[i - 1]);
		reg_set_field(tmp, JPGENC_TABLE_WDATA_L, luma_ac_value[i]);
		reg_set(base_addr + JPGENC_JPE_TABLE_DATA_REG, tmp);
	}
}

static void hjpeg150_hufftable_chroma_info(void __iomem *base_addr)
{
	int tmp;
	uint32_t i;
	uint32_t length_bit, length_value, length;

	/* DC 1 */
	length_bit = ARRAY_SIZE(chroma_dc_bits);
	length_value = ARRAY_SIZE(chroma_dc_value);
	length = length_bit + length_value;
	reg_set(base_addr + JPGENC_JPE_TDC1_LEN_REG, length);
	reg_set(base_addr + JPGENC_JPE_TABLE_ID_REG, 6); /* 6: table id */
	for (i = 1; i < length_bit; i = i + 2) { /* 2: offset */
		tmp = 0;
		reg_set_field(tmp, JPGENC_TABLE_WDATA_H, chroma_dc_bits[i - 1]);
		reg_set_field(tmp, JPGENC_TABLE_WDATA_L, chroma_dc_bits[i]);
		reg_set(base_addr + JPGENC_JPE_TABLE_DATA_REG, tmp);
	}
	for (i = 1; i < length_value; i = i + 2) { /* 2: offset */
		tmp = 0;
		reg_set_field(tmp, JPGENC_TABLE_WDATA_H, chroma_dc_value[i - 1]);
		reg_set_field(tmp, JPGENC_TABLE_WDATA_L, chroma_dc_value[i]);
		reg_set(base_addr + JPGENC_JPE_TABLE_DATA_REG, tmp);
	}

	/* AC 1 */
	length_bit = ARRAY_SIZE(chroma_ac_bits);
	length_value = ARRAY_SIZE(chroma_ac_value);
	length = length_bit + length_value;
	reg_set(base_addr + JPGENC_JPE_TAC1_LEN_REG, length);
	reg_set(base_addr + JPGENC_JPE_TABLE_ID_REG, 7); /* 7: table id */
	for (i = 1; i < length_bit; i = i + 2) { /* 2: offset */
		tmp = 0;
		reg_set_field(tmp, JPGENC_TABLE_WDATA_H, chroma_ac_bits[i - 1]);
		reg_set_field(tmp, JPGENC_TABLE_WDATA_L, chroma_ac_bits[i]);
		reg_set(base_addr + JPGENC_JPE_TABLE_DATA_REG, tmp);
	}
	for (i = 1; i < length_value; i = i + 2) { /* 2: offset */
		tmp = 0;
		reg_set_field(tmp, JPGENC_TABLE_WDATA_H, chroma_ac_value[i - 1]);
		reg_set_field(tmp, JPGENC_TABLE_WDATA_L, chroma_ac_value[i]);
		reg_set(base_addr + JPGENC_JPE_TABLE_DATA_REG, tmp);
	}
}

static void hjpeg150_hufftable_init(void)
{
	void __iomem *base_addr = s_hjpeg150.viraddr;

	hjpeg150_hufftable_luma_info(base_addr);

	hjpeg150_hufftable_chroma_info(base_addr);
}

static void hjpeg150_qtable_init(void)
{
	int length;
	int i;
	unsigned int tmp;
	void __iomem *base_addr = s_hjpeg150.viraddr;

	/* q-table 0 */
	length = ARRAY_SIZE(luma_qtable1);
	reg_set(base_addr + JPGENC_JPE_TABLE_ID_REG, 0);
	for (i = 1; i < length; i = i + 2) { /* 2: offset bit */
		tmp = 0;
		reg_set_field(tmp, JPGENC_TABLE_WDATA_H, luma_qtable1[i - 1]);
		reg_set_field(tmp, JPGENC_TABLE_WDATA_L, luma_qtable1[i]);
		reg_set(base_addr + JPGENC_JPE_TABLE_DATA_REG, tmp);
	}

	/* q-table 1 */
	length = ARRAY_SIZE(chroma_qtable1);
	reg_set(base_addr + JPGENC_JPE_TABLE_ID_REG, 1);
	for (i = 1; i < length; i = i + 2) { /* 2: offset bit */
		tmp = 0;
		reg_set_field(tmp, JPGENC_TABLE_WDATA_H, chroma_qtable1[i - 1]);
		reg_set_field(tmp, JPGENC_TABLE_WDATA_L, chroma_qtable1[i]);
		reg_set(base_addr + JPGENC_JPE_TABLE_DATA_REG, tmp);
	}

	/* q-table 2 */
	length = ARRAY_SIZE(luma_qtable2);
	reg_set(base_addr + JPGENC_JPE_TABLE_ID_REG, 2); /* 2: table id */
	for (i = 1; i < length; i = i + 2) { /* 2: offset bit */
		tmp = 0;
		reg_set_field(tmp, JPGENC_TABLE_WDATA_H, luma_qtable2[i - 1]);
		reg_set_field(tmp, JPGENC_TABLE_WDATA_L, luma_qtable2[i]);
		reg_set(base_addr + JPGENC_JPE_TABLE_DATA_REG, tmp);
	}

	/* q-table 3 */
	length = ARRAY_SIZE(chroma_qtable2);
	reg_set(base_addr + JPGENC_JPE_TABLE_ID_REG, 3); /* 3: table id */
	for (i = 1; i < length; i = i + 2) { /* 2: offset bit */
		tmp = 0;
		reg_set_field(tmp, JPGENC_TABLE_WDATA_H, chroma_qtable2[i - 1]);
		reg_set_field(tmp, JPGENC_TABLE_WDATA_L, chroma_qtable2[i]);
		reg_set(base_addr + JPGENC_JPE_TABLE_DATA_REG, tmp);
	}
}

static void hjpeg150_prefetch_init(void)
{
	uint32_t tmp = 0;
	void __iomem *base_addr = s_hjpeg150.viraddr;

#ifdef SMMU
	reg_set_field(tmp, JPGENC_PREFETCH_EN, 1);
#else
	reg_set_field(tmp, JPGENC_PREFETCH_EN, 0);
#endif

	reg_set_field(tmp, JPGENC_PREFETCH_DELAY, 1210); /* 1210: delay value */
	reg_set(base_addr + JPGENC_PREFETCH_REG, tmp);

	tmp = 0;
	reg_set_field(tmp, JPGENC_ID0_Y, 49);
	reg_set_field(tmp, JPGENC_ID1_Y, 50);
	reg_set_field(tmp, JPGENC_ID2_Y, 51);
	reg_set_field(tmp, JPGENC_ID3_Y, 52);
	reg_set(base_addr + JPGENC_PREFETCH_IDY0_REG, tmp);

	tmp = 0;
	reg_set_field(tmp, JPGENC_ID4_Y, 53);
	reg_set_field(tmp, JPGENC_ID5_Y, 54);
	reg_set_field(tmp, JPGENC_ID6_Y, 55);
	reg_set_field(tmp, JPGENC_ID7_Y, 56);
	reg_set(base_addr + JPGENC_PREFETCH_IDY1_REG, tmp);

	tmp = 0;
	reg_set_field(tmp, JPGENC_ID0_UV, 57);
	reg_set_field(tmp, JPGENC_ID1_UV, 58);
	reg_set_field(tmp, JPGENC_ID2_UV, 59);
	reg_set_field(tmp, JPGENC_ID3_UV, 60);
	reg_set(base_addr + JPGENC_PREFETCH_IDUV0_REG, tmp);

	tmp = 0;
	reg_set_field(tmp, JPGENC_ID8_Y, 61);
	reg_set_field(tmp, JPGENC_ID4_UV, 62);
	reg_set(base_addr + JPGENC_PREFETCH_IDUVY_REG, tmp);
}

static void hjpeg150_rstmarker_init(void)
{
	void __iomem *base_addr = s_hjpeg150.viraddr;

	reg_set(base_addr + JPGENC_JPE_RESTART_INTERVAL_REG,
		JPGENC_RESTART_INTERVAL);
}

static void hjpeg150_rstmarker(unsigned int rst)
{
	void __iomem *base_addr = s_hjpeg150.viraddr;

	cam_info("%s enter ,rst = %d", __func__, rst);
	reg_set(base_addr + JPGENC_JPE_RESTART_INTERVAL_REG, rst);
}

static void hjpeg150_synccfg_init(void)
{
	void __iomem *base_addr = s_hjpeg150.viraddr;
	uint32_t tmp = 0;

	reg_set_field(tmp, JPGENC_SOURCE, 1);
	reg_set_field(tmp, JPGENC_SRAM_NOOPT, 1);
	reg_set(base_addr + JPGENC_SYNCCFG_REG, tmp);
}

static void add_jpeg_header_soi(unsigned char *jpgenc_addrs, uint32_t *offset)
{
	uint32_t i;
	uint32_t length;
	unsigned char *jpgenc_addr = NULL;

	jpgenc_addr = jpgenc_addrs + (*offset);
	length = ARRAY_SIZE(header_soi);
	for (i = 0; i < length; i++) {
		*jpgenc_addr = header_soi[i];
		jpgenc_addr++;
	}
	(*offset) += length;
}

static void add_jpeg_header_app0(unsigned char *jpgenc_addrs, uint32_t *offset)
{
	uint32_t i;
	uint32_t length;
	unsigned char *jpgenc_addr = NULL;

	jpgenc_addr = jpgenc_addrs + (*offset);
	length = ARRAY_SIZE(header_app0);
	for (i = 0; i < length; i++) {
		*jpgenc_addr = header_app0[i];
		jpgenc_addr++;
	}
	(*offset) += length;
}

static void jpeg_header_add_tbl(unsigned char *jpgenc_addrs,
	const unsigned char *header_qtable, uint32_t tbl_len, uint32_t *offset)
{
	unsigned char *jpgenc_addr = NULL;

	jpgenc_addr = jpgenc_addrs + (*offset);
	for (i = 0; i < tbl_len; i++) {
		*jpgenc_addr = header_qtable[i];
		jpgenc_addr++;
	}
	(*offset) += tbl_len;
}

static void jpeg_header_dqt_add_tbl_from_quality(unsigned char *jpgenc_addrs,
	uint32_t quality, const unsigned int *header_qtable,
	uint32_t tbl_len, uint32_t *offset)
{
	int scaler;
	long temp;
	uint32_t i;
	unsigned char *jpgenc_addr = NULL;

	jpgenc_addr = jpgenc_addrs + (*offset);
	/* 50: quality standard */
	if (quality < 50)
		/* 5000: aver quality ratio */
		scaler = 5000 / quality;
	else
		/* 200: aver max quality */
		scaler = 200 - quality * 2;

	for (i = 0; i < tbl_len; i++) {
		temp = (header_qtable[i] * scaler + 50L) / 100L;
		if (temp <= 0L)
			temp = 1L;
		if (temp > 255L)
			temp = 255L;
		*jpgenc_addr = temp;
		jpgenc_addr++;
	}
	(*offset) += tbl_len;
}

static void add_jpeg_header_dqt(jpgenc_config_t *config,
	unsigned char *jpgenc_addr, uint32_t *offset)
{
	uint32_t i;
	uint32_t length;
	uint32_t quality;

	quality = config->buffer.quality;

	length = ARRAY_SIZE(header_qtable0);
	jpeg_header_add_tbl(jpgenc_addr, header_qtable0, length, offset);

	if (quality == 0) {
		length = ARRAY_SIZE(luma_qtable1);
		jpeg_header_add_tbl(jpgenc_addr, luma_qtable1, length, offset);
	} else {
		length = ARRAY_SIZE(luma_qtable2);
		jpeg_header_dqt_add_tbl_from_quality(jpgenc_addr,
			quality, luma_qtable2, length, offset);
	}

	length = ARRAY_SIZE(header_qtable1);
	jpeg_header_add_tbl(jpgenc_addr, header_qtable1, length, offset);

	if (quality == 0) {
		length = ARRAY_SIZE(chroma_qtable1);
		jpeg_header_add_tbl(jpgenc_addr,
			chroma_qtable1, length, offset);
	} else {
		length = ARRAY_SIZE(chroma_qtable2);
		jpeg_header_dqt_add_tbl_from_quality(jpgenc_addr,
			quality, chroma_qtable2, length, offset);
	}
}

static void add_jpeg_header_sof0(jpgenc_config_t *config,
	unsigned char *jpgenc_addrs, uint32_t *offset)
{
	uint32_t width, height;
	encode_format_e format;
	const uint32_t compress_ratio = 256;
	unsigned char *jpgenc_addr = NULL;

	width = config->buffer.width;
	height = config->buffer.height;
	format = config->buffer.format;
	jpgenc_addr = jpgenc_addrs + (*offset);

	*jpgenc_addr = 0xff;
	jpgenc_addr++;
	*jpgenc_addr = 0xc0;
	jpgenc_addr++;
	*jpgenc_addr = 0x00;
	jpgenc_addr++;
	*jpgenc_addr = 0x11;
	jpgenc_addr++;
	*jpgenc_addr = 0x08;
	jpgenc_addr++;
	*jpgenc_addr = height / compress_ratio;
	jpgenc_addr++;
	*jpgenc_addr = height % compress_ratio;
	jpgenc_addr++;
	*jpgenc_addr = width / compress_ratio;
	jpgenc_addr++;
	*jpgenc_addr = width % compress_ratio;
	jpgenc_addr++;
	*jpgenc_addr = 0x03;
	jpgenc_addr++;
	*jpgenc_addr = 0x01;
	jpgenc_addr++;
	if ((format & JPGENC_FORMAT_BIT) == JPGENC_FORMAT_YUV422)
		*jpgenc_addr = 0x21;
	else
		*jpgenc_addr = 0x22;
	jpgenc_addr++;
	*jpgenc_addr = 0x00;
	jpgenc_addr++;

	*jpgenc_addr = 0x02; /* 0x02:nv12 0x03:nv21 */
	jpgenc_addr++;
	*jpgenc_addr = 0x11;
	jpgenc_addr++;
	*jpgenc_addr = 0x01;
	jpgenc_addr++;
	*jpgenc_addr = 0x03; /* 0x03:nv12 0x02:nv21 */
	jpgenc_addr++;
	*jpgenc_addr = 0x11;
	jpgenc_addr++;
	*jpgenc_addr = 0x01;
	jpgenc_addr++;

	(*offset) += 19; /* 19: offset */
}

static void add_jpeg_header_dht(unsigned char *jpgenc_addr, uint32_t *offset)
{
	uint32_t length;

	length = ARRAY_SIZE(header_hufftable_dc0);
	jpeg_header_add_tbl(jpgenc_addr, header_hufftable_dc0, length, offset);

	length = ARRAY_SIZE(luma_dc_bits);
	jpeg_header_add_tbl(jpgenc_addr, luma_dc_bits, length, offset);

	length = ARRAY_SIZE(luma_dc_value);
	jpeg_header_add_tbl(jpgenc_addr, luma_dc_value, length, offset);

	length = ARRAY_SIZE(header_hufftable_ac0);
	jpeg_header_add_tbl(jpgenc_addr, header_hufftable_ac0, length, offset);

	length = ARRAY_SIZE(luma_ac_bits);
	jpeg_header_add_tbl(jpgenc_addr, luma_ac_bits, length, offset);

	length = ARRAY_SIZE(luma_ac_value);
	jpeg_header_add_tbl(jpgenc_addr, luma_ac_value, length, offset);

	length = ARRAY_SIZE(header_hufftable_dc1);
	jpeg_header_add_tbl(jpgenc_addr, header_hufftable_dc1, length, offset);

	length = ARRAY_SIZE(chroma_dc_bits);
	jpeg_header_add_tbl(jpgenc_addr, chroma_dc_bits, length, offset);

	length = ARRAY_SIZE(chroma_dc_value);
	jpeg_header_add_tbl(jpgenc_addr, chroma_dc_value, length, offset);

	length = ARRAY_SIZE(header_hufftable_ac1);
	jpeg_header_add_tbl(jpgenc_addr, header_hufftable_ac1, length, offset);

	length = ARRAY_SIZE(chroma_ac_bits);
	jpeg_header_add_tbl(jpgenc_addr, chroma_ac_bits, length, offset);

	length = ARRAY_SIZE(chroma_ac_value);
	jpeg_header_add_tbl(jpgenc_addr, chroma_ac_value, length, offset);
}

static void add_jpeg_header_dri(jpgenc_config_t *config,
	unsigned char *jpgenc_addrs, uint32_t *offset)
{
	const uint32_t compress_ratio = 256;
	unsigned int rst;
	unsigned char *jpgenc_addr = NULL;

	rst = config->buffer.rst;
	jpgenc_addr = jpgenc_addrs + (*offset);

	/* add define restart interval */
	*jpgenc_addr = 0xff;
	jpgenc_addr++;
	*jpgenc_addr = 0xdd;
	jpgenc_addr++;
	*jpgenc_addr = 0x00;
	jpgenc_addr++;
	*jpgenc_addr = 0x04;
	jpgenc_addr++;
	*jpgenc_addr = rst / compress_ratio;
	jpgenc_addr++;
	*jpgenc_addr = rst % compress_ratio;
	jpgenc_addr++;

	(*offset) += 6; /* 6: offset */
}

static void add_jpeg_header_sos(unsigned char *jpgenc_addrs, uint32_t *offset)
{
	unsigned char *jpgenc_addr = NULL;

	jpgenc_addr = jpgenc_addrs + (*offset);
	*jpgenc_addr = 0xff;
	jpgenc_addr++;
	*jpgenc_addr = 0xda;
	jpgenc_addr++;
	*jpgenc_addr = 0X00;
	jpgenc_addr++;
	*jpgenc_addr = 0X0C;
	jpgenc_addr++;
	*jpgenc_addr = 0x03;
	jpgenc_addr++;
	*jpgenc_addr = 0x01;
	jpgenc_addr++;
	*jpgenc_addr = 0x00;
	jpgenc_addr++;
	*jpgenc_addr = 0x02;
	jpgenc_addr++;
	*jpgenc_addr = 0x11;
	jpgenc_addr++;
	*jpgenc_addr = 0x03;
	jpgenc_addr++;
	*jpgenc_addr = 0x11;
	jpgenc_addr++;
	*jpgenc_addr = 0x00;
	jpgenc_addr++;
	*jpgenc_addr = 0x3F;
	jpgenc_addr++;
	*jpgenc_addr = 0x00;

	(*offset) += 13; /* 13: offset */
}

static void hjpeg150_add_header(jpgenc_config_t *config)
{
	uint32_t offset = 0;
	unsigned char *jpgenc_addr = NULL;
	unsigned char *org_jpeg_addr = NULL;

	if (!config) {
		cam_err("%s: config is NULL. %d[]", __func__, __LINE__);
		return;
	}

#ifdef SMMU
	jpgenc_addr = (unsigned char *)config->buffer.ion_vaddr;
#else
	jpgenc_addr = ioremap_nocache(JPEG_OUTPUT_PHYADDR, JPGENC_HEAD_SIZE);
	if (!jpgenc_addr) {
		cam_err("%s %d remap fail", __func__, __LINE__);
		return;
	}
#endif

	org_jpeg_addr = jpgenc_addr;

	*jpgenc_addr = JPGENC_HEAD_OFFSET;
	jpgenc_addr += JPGENC_HEAD_OFFSET;

	add_jpeg_header_soi(jpgenc_addr, &offset);

	add_jpeg_header_app0(jpgenc_addr, &offset);

	add_jpeg_header_dqt(config, jpgenc_addr, &offset);

	add_jpeg_header_sof0(config, jpgenc_addr, &offset);

	add_jpeg_header_dht(jpgenc_addr, &offset);

	add_jpeg_header_dri(config, jpgenc_addr, &offset);

	add_jpeg_header_sos(jpgenc_addr, &offset);

#ifdef SMMU
	if (org_jpeg_addr)
		org_jpeg_addr = NULL;
#else
	if (org_jpeg_addr)
		iounmap((void*)org_jpeg_addr);
#endif
}

static int check_rst(jpgenc_config_t *config)
{
	int ret = 0;
	cam_debug("enter %s %d", __func__, __LINE__);

	if (!config) {
		cam_err("%s: config is null! %d", __func__, __LINE__);
		return -EINVAL;
	}

	switch (config->buffer.rst) {
	case JPGENC_RESTART_INTERVAL:
		break;
	case JPGENC_RESTART_INTERVAL_ON:
		hjpeg150_rstmarker(config->buffer.rst);
		cam_info("JPEG restart interval is on %s %d",
			__func__, __LINE__);
		break;
	default:
		cam_err("invalid value of \"rst\". %s %d",
			__func__, __LINE__);
		ret = -1;
		break;
	}
	return ret;
}

static int __check_buffer_vaild(int share_fd, unsigned int vaild_addr,
	unsigned int vaild_size)
{
	struct ion_handle *ionhnd = NULL;
	struct iommu_map_format iommu_format;
	int ret = 0;

	if (share_fd < 0) {
		cam_err("invalid ion: fd=%d", share_fd);
		return -1;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
	ionhnd = ion_import_dma_buf_fd(s_hjpeg150.ion_client, share_fd);
#else
	ionhnd = ion_import_dma_buf(s_hjpeg150.ion_client, share_fd);
#endif
	if (IS_ERR(ionhnd)) {
		ionhnd = NULL;
		return -1;
	} else {
		(void)memset_s(&iommu_format, sizeof(struct iommu_map_format),
			0, sizeof(struct iommu_map_format));
		if (ion_map_iommu(s_hjpeg150.ion_client,
			ionhnd, &iommu_format)) {
			ret = -1;
			goto ion_free;
		}
		if (vaild_addr != iommu_format.iova_start)
			ret = -1;
		if (vaild_size > iommu_format.iova_size)
			ret = -1;
	}

	ion_unmap_iommu(s_hjpeg150.ion_client, ionhnd);
ion_free:
	ion_free(s_hjpeg150.ion_client, ionhnd);
	return ret;
}

static int check_jpeg_size(const jpgenc_config_t *config)
{
	if (!check_align(config->buffer.width, 2) || /* 2: alignof ratio */
		config->buffer.width == 0 ||
		(config->buffer.width > 8192)) { /* 8192: resolution */
		cam_err("width[%d] is invalid!", config->buffer.width);
		return -1;
	}

	if (config->buffer.height == 0 ||
		(config->buffer.height > 8192)) { /* 8192: resolution */
		cam_err("height[%d] is invalid!", config->buffer.height);
		return -1;
	}

	return 0;
}

static int check_jpeg_stride(jpgenc_config_t *config)
{
	unsigned int stride_dft;

	stride_dft = (config->buffer.format & JPGENC_FORMAT_BIT) ==
		JPGENC_FORMAT_YUV422 ? 1024 : 512; /* 1024,512: stride */
	/* 16: alignof ratio */
	if ((config->buffer.stride == 0) ||
		!check_align(config->buffer.stride, 16) ||
		(config->buffer.stride / 16 > stride_dft)) {
		cam_err("stride[%d] is invalid!", config->buffer.stride);
		return -1;
	}
	return 0;
}

static int check_jpeg_input_buffer(jpgenc_config_t *config)
{
	encode_format_e format;

	/* 16: alignof ratio */
	if ((config->buffer.input_buffer_y == 0) ||
		!check_align(config->buffer.input_buffer_y, 16)) {
		cam_err("input buffer y[0x%x] is invalid!",
			config->buffer.input_buffer_y);
		return -1;
	}

	format = config->buffer.format & JPGENC_FORMAT_BIT;
	if (format == JPGENC_FORMAT_YUV420 &&
		((config->buffer.input_buffer_uv == 0) ||
		!check_align(config->buffer.input_buffer_uv, 16))) {
		cam_err("input buffer uv[0x%x] is invalid!",
			config->buffer.input_buffer_uv);
		return -1;
	}
	return 0;
}

static int check_jpeg_format(jpgenc_config_t *config)
{
	encode_format_e format;
	unsigned int input_buffer_uv;
	unsigned int input_buffer_y;

	format = config->buffer.format & JPGENC_FORMAT_BIT;
	input_buffer_uv = config->buffer.input_buffer_uv;
	input_buffer_y = config->buffer.input_buffer_y;

	if (format == JPGENC_FORMAT_YUV420 &&
		((input_buffer_uv - input_buffer_y) <
		config->buffer.stride * 8 * 16)) {
			cam_err("buffer format is invalid!");
			return -1;
		}
	}
	return 0;
}

static void check_jpeg_quality(jpgenc_config_t *config)
{
	/* 100: quality default */
	if (config->buffer.quality > 100) {
		cam_err("quality[%d] is invalid, adjust to 100",
			config->buffer.quality);
		config->buffer.quality = 100;
	}
}

static int check_buffer_vaild(jpgenc_config_t *config)
{
	unsigned int vaild_input_size;

	if (IS_ERR_OR_NULL(s_hjpeg150.ion_client)) {
		cam_err("invalid ion_client: s_hjpeg150.ion_client is error");
		return -1;
	}

	if (__check_buffer_vaild(config->buffer.ion_fd,
		config->buffer.output_buffer, config->buffer.output_size))
		return -1;

	if ((config->buffer.format & JPGENC_FORMAT_BIT) ==
		JPGENC_FORMAT_YUV422)
		/* 2: size = width * height * 2 */
		vaild_input_size =
			config->buffer.width * config->buffer.height * 2;
	else
		/* 3,2: size = width * height * 1.5 */
		vaild_input_size =
			config->buffer.width * config->buffer.height * 3 / 2;

	/* uv addr has been checked */
	if (__check_buffer_vaild(config->buffer.input_ion_fd,
		config->buffer.input_buffer_y, vaild_input_size))
		return -1;

	return 0;
}

static int check_jpeg_output_size(jpgenc_config_t *config)
{
	if (config->buffer.output_size <= JPGENC_HEAD_SIZE ||
		config->buffer.output_size > MAX_JPEG_BUFFER_SIZE) {
		cam_err("output size[%u] is invalid!",
			config->buffer.output_size);
		return -1;
	}

	return 0;
}

static int check_config(jpgenc_config_t *config)
{
	cam_info("%s enter", __func__);
	if (!config) {
		cam_err("%s: config is null! %d", __func__, __LINE__);
		return -EINVAL;
	}

	if (config->buffer.format != JPGENC_FORMAT_UYVY &&
		config->buffer.format != JPGENC_FORMAT_VYUY &&
		config->buffer.format != JPGENC_FORMAT_YVYU &&
		config->buffer.format != JPGENC_FORMAT_YUYV &&
		config->buffer.format != JPGENC_FORMAT_NV12 &&
		config->buffer.format != JPGENC_FORMAT_NV21) {
		cam_err("%s: check buffer format fail! %d",
			__func__, __LINE__);
		return -1;
	}

	if (check_jpeg_size(config) != 0)
		return -1;

	if (check_jpeg_stride(config) != 0)
		return -1;

	if (check_jpeg_input_buffer(config) != 0)
		return -1;

	if (check_jpeg_format(config) != 0)
		return -1;

	check_jpeg_quality(config);

	if(check_jpeg_output_size(config) != 0)
		return -1;

	if(check_buffer_vaild(config) != 0)
		return -1;

	return 0;
}

static int hjpeg150_cvdr_fmt_desc_vp_wr(uint32_t width, uint32_t height,
	uint32_t axi_address_start, uint32_t buf_size, cvdr_wr_fmt_desc_t *desc)
{
	uint32_t total_num_bytes = buf_size;
	if (width == 0) {
	   cam_err("width cannot be zero");
	   return -1;
	}

	desc->pix_fmt = DF_D64;
	desc->pix_expan = EXP_PIX;
	desc->access_limiter = 16; /* 16: access limiter */
	/* 15: page size 32768 */
	desc->last_page = (axi_address_start + total_num_bytes) >> 15;
	/* 2,16: width to stride ratio */
	desc->line_stride = (width * 2) / 16 - 1;
	desc->line_wrap = height;

	cam_info("%s acess_limiter = %d, last_page = %d, line_stride = %d,"
		" width = %d,  height = %d",
		__func__, desc->access_limiter, desc->last_page,
		desc->line_stride, width, height);

	return 0;
}

static int hjpeg150_set_vp_wr_ready(cvdr_wr_fmt_desc_t *desc, uint32_t buf_addr)
{
	void __iomem *cvdr_srt_base = s_hjpeg150.hw_ctl.cvdr_viraddr;
	int ret = 0;
	u_vp_wr_cfg tmp_cfg;
	u_vp_wr_axi_line tmp_line;
	u_vp_wr_axi_fs tmp_fs;

	tmp_cfg.reg32 = reg_get(cvdr_srt_base + CVDR_SRT_VP_WR_CFG_25_OFFSET);
	tmp_cfg.bits.vpwr_pixel_format = desc->pix_fmt;
	tmp_cfg.bits.vpwr_pixel_expansion = desc->pix_expan;
	tmp_cfg.bits.vpwr_access_limiter = desc->access_limiter;
	tmp_cfg.bits.vpwr_last_page = desc->last_page;
	reg_set(cvdr_srt_base + CVDR_SRT_VP_WR_CFG_25_OFFSET, tmp_cfg.reg32);

	tmp_line.reg32 = reg_get(cvdr_srt_base +
		CVDR_SRT_VP_WR_AXI_LINE_25_OFFSET);
	tmp_line.bits.vpwr_line_stride = desc->line_stride;
	tmp_line.bits.vpwr_line_wrap = desc->line_wrap;
	reg_set(cvdr_srt_base +
		CVDR_SRT_VP_WR_AXI_LINE_25_OFFSET, tmp_line.reg32);

#ifndef SMMU
	reg_set(cvdr_srt_base + CVDR_SRT_VP_WR_IF_CFG_25_OFFSET,
		(reg_get(cvdr_srt_base + CVDR_SRT_VP_WR_IF_CFG_25_OFFSET) |
		(PREFETCH_BY_PASS)));
#endif

	tmp_fs.reg32 = reg_get(cvdr_srt_base + CVDR_SRT_VP_WR_AXI_FS_25_OFFSET);
	tmp_fs.bits.vpwr_address_frame_start = buf_addr >> 4; /* 4: offset 64 */
	reg_set(cvdr_srt_base + CVDR_SRT_VP_WR_AXI_FS_25_OFFSET, tmp_fs.reg32);
	return ret;
}

static int hjpeg150_set_nr_rd_config(unsigned char du, unsigned char limiter)
{
	void __iomem *cvdr_srt_base = s_hjpeg150.hw_ctl.cvdr_viraddr;
	u_cvdr_srt_nr_rd_cfg_1 tmp;
	u_cvdr_srt_limiter_nr_rd_1 lmt;

	tmp.reg32 = reg_get(cvdr_srt_base + CVDR_SRT_NR_RD_CFG_1_OFFSET);
	tmp.bits.nrrd_allocated_du_1 = du;
	tmp.bits.nrrd_enable_1 = 1;
	reg_set(cvdr_srt_base + CVDR_SRT_NR_RD_CFG_1_OFFSET, tmp.reg32);

	lmt.reg32 = reg_get(cvdr_srt_base + CVDR_SRT_LIMITER_NR_RD_1_OFFSET);
	lmt.bits.nrrd_access_limiter_0_1 = limiter;
	lmt.bits.nrrd_access_limiter_1_1 = limiter;
	lmt.bits.nrrd_access_limiter_2_1 = limiter;
	lmt.bits.nrrd_access_limiter_3_1 = limiter;
	lmt.bits.nrrd_access_limiter_reload_1 = 0xF;
	reg_set(cvdr_srt_base + CVDR_SRT_LIMITER_NR_RD_1_OFFSET, lmt.reg32);

	return 0;
}

static void hjpeg150_do_cvdr_config(jpgenc_config_t *config)
{
	int ret;
	uint32_t width;
	uint32_t height;
	uint32_t buf_addr;
	uint32_t buf_size;
	cvdr_wr_fmt_desc_t cvdr_wr_fmt;
	unsigned char access_limiter = ACCESS_LIMITER;
	unsigned char allocated_du = ALLOCATED_DU;

	cam_info("%s enter", __func__);
	if (!config) {
		cam_err("%s: config is null! %d", __func__, __LINE__);
		return;
	}
	width = config->buffer.width;
	height = config->buffer.height;
	buf_addr = config->buffer.output_buffer + JPGENC_HEAD_SIZE;
	buf_size = config->buffer.output_size - JPGENC_HEAD_SIZE;

	ret = hjpeg150_cvdr_fmt_desc_vp_wr(width, height, buf_addr,
		buf_size, &cvdr_wr_fmt);
	if (ret != 0) {
		cam_err("%s %d config cvdr failed", __func__, __LINE__);
		return;
	}

	ret = hjpeg150_set_vp_wr_ready(&cvdr_wr_fmt, buf_addr);
	if (ret != 0)
		cam_err("%s %d set vp wr ready fail", __func__, __LINE__);

	hjpeg150_set_nr_rd_config(allocated_du, access_limiter);
}

static void jpeg_pic_format_config(jpgenc_config_t *config,
	void __iomem *base_addr)
{
	unsigned int tmp = 0;

	if ((config->buffer.format & JPGENC_FORMAT_BIT) == JPGENC_FORMAT_YUV422) {
		set_field_to_reg(base_addr +
			JPGENC_JPE_PIC_FORMAT_REG, JPGENC_ENC_PIC_FORMAT, 0);
		if (config->buffer.format == JPGENC_FORMAT_UYVY) {
			/* deafult format */
			reg_set_field(tmp, JPGENC_SWAPIN_Y_UV, 0);
			reg_set_field(tmp, JPGENC_SWAPIN_U_V, 0);
		} else if (config->buffer.format == JPGENC_FORMAT_VYUY) {
			reg_set_field(tmp, JPGENC_SWAPIN_Y_UV, 0);
			reg_set_field(tmp, JPGENC_SWAPIN_U_V, 1);
		} else if (config->buffer.format == JPGENC_FORMAT_YVYU) {
			reg_set_field(tmp, JPGENC_SWAPIN_Y_UV, 1);
			reg_set_field(tmp, JPGENC_SWAPIN_U_V, 1);
		} else {
			/* JPGENC_FORMAT_YUYV */
			reg_set_field(tmp, JPGENC_SWAPIN_Y_UV, 1);
			reg_set_field(tmp, JPGENC_SWAPIN_U_V, 0);
		}
		reg_set(base_addr + JPGENC_INPUT_SWAP_REG, tmp);
	} else if (config->buffer.format == JPGENC_FORMAT_NV12) {
		set_field_to_reg(base_addr + JPGENC_JPE_PIC_FORMAT_REG,
			JPGENC_ENC_PIC_FORMAT, 1);
		reg_set_field(tmp, JPGENC_SWAPIN_Y_UV, 0);
		reg_set_field(tmp, JPGENC_SWAPIN_U_V, 0);
		reg_set(base_addr + JPGENC_INPUT_SWAP_REG, tmp);
	} else {
		/* JPGENC_FORMAT_NV21 */
		set_field_to_reg(base_addr +
			JPGENC_JPE_PIC_FORMAT_REG, JPGENC_ENC_PIC_FORMAT, 1);
		reg_set_field(tmp, JPGENC_SWAPIN_Y_UV, 0);
		reg_set_field(tmp, JPGENC_SWAPIN_U_V, 1); /* swap U and V */
		reg_set(base_addr + JPGENC_INPUT_SWAP_REG, tmp);
	}
}

static void jpeg_pic_size_config(jpgenc_config_t *config,
	void __iomem *base_addr)
{
	uint32_t width_left;
	uint32_t width_right;

	width_left = config->buffer.width;

	/* 64: left standard width */
	if (width_left >= 64)
		/* 2,16: get width lower bit */
		width_left = check_down((width_left / 2), 16);
	width_right = config->buffer.width - width_left;

	set_field_to_reg(base_addr + JPGENC_JPE_ENC_HRIGHT1_REG,
		JPGENC_ENC_HRIGHT1, width_left - 1);
	set_field_to_reg(base_addr + JPGENC_JPE_ENC_VBOTTOM_REG,
		JPGENC_ENC_VBOTTOM, config->buffer.height - 1);
	set_field_to_reg(base_addr + JPGENC_JPE_ENC_HRIGHT2_REG,
		JPGENC_ENC_HRIGHT2, width_right != 0 ? width_right - 1 : 0);
}

static void jpeg_q_table_config(void __iomem *base_addr,
	const unsigned int *qtable, int length,
	const unsigned int tbl_id,  scaler)
{
	unsigned int tmp;
	uint32_t temp;

	/* q-table 2 */
	set_field_to_reg(base_addr + JPGENC_JPE_TABLE_ID_REG,
		JPGENC_TABLE_ID, tbl_id);
	for (i = 1; i < length; i = i + 2) { /* 2: offset bits */
		temp = (qtable[i - 1] * scaler + 50L) / 100L;
		if (temp <= 0L)
			temp = 1L;
		if (temp > 255L)
			temp = 255L;
		tmp = 0;
		reg_set_field(tmp, JPGENC_TABLE_WDATA_H, temp);
		temp = (qtable[i] * scaler + 50L) / 100L;
		if (temp <= 0L)
			temp = 1L;
		if (temp > 255L)
			temp = 255L;
		reg_set_field(tmp, JPGENC_TABLE_WDATA_L, temp);
		reg_set(base_addr + JPGENC_JPE_TABLE_DATA_REG, tmp);
	}
}

static void jpeg_yuv_com_quant_config(jpgenc_config_t *config,
	void __iomem *base_addr)
{
	int scaler;

	if (config->buffer.quality == 0) {
		set_field_to_reg(base_addr + JPGENC_JPE_TQ_Y_SELECT_REG,
			JPGENC_TQ0_SELECT, 0);
		set_field_to_reg(base_addr + JPGENC_JPE_TQ_U_SELECT_REG,
			JPGENC_TQ1_SELECT, 1);
		set_field_to_reg(base_addr + JPGENC_JPE_TQ_V_SELECT_REG,
			JPGENC_TQ2_SELECT, 1);
	} else {
		/* 50: quality standard */
		if (config->buffer.quality < 50)
			/* 5000: aver quality ratio */
			scaler = 5000 / config->buffer.quality;
		else
			/* 200,2: aver max quality */
			scaler = 200 - config->buffer.quality * 2;

		/* 2: q-table 3 */
		jpeg_q_table_config(base_addr, luma_qtable2,
			ARRAY_SIZE(luma_qtable2), 2, scaler);

		/* 3: q-table 3 */
		jpeg_q_table_config(base_addr, chroma_qtable2,
			ARRAY_SIZE(chroma_qtable2), 3, scaler);

		set_field_to_reg(base_addr + JPGENC_JPE_TQ_Y_SELECT_REG,
			JPGENC_TQ0_SELECT, 2); /* 2: tq y value */
		set_field_to_reg(base_addr + JPGENC_JPE_TQ_U_SELECT_REG,
			JPGENC_TQ1_SELECT, 3); /* 3: tq u value */
		set_field_to_reg(base_addr + JPGENC_JPE_TQ_V_SELECT_REG,
			JPGENC_TQ2_SELECT, 3); /* 3: tq v value */
	}
}

static void hjpeg150_do_config(jpgenc_config_t *config)
{
	void __iomem *base_addr = NULL;

	base_addr = s_hjpeg150.viraddr;
	cam_info("%s enter", __func__);
	if (!config || !base_addr) {
		cam_err("%s: config or viraddr is null! line: %d",
			__func__, __LINE__);
		return;
	}

	jpeg_pic_format_config(config, base_addr);

	jpeg_pic_size_config(config, base_addr);

	jpeg_yuv_com_quant_config(config, base_addr);

	/* 4: YUV compress ratio */
	set_field_to_reg(base_addr + JPGENC_ADDRESS_Y_REG,
		JPGENC_ADDRESS_Y, config->buffer.input_buffer_y >> 4);

	if ((config->buffer.format & JPGENC_FORMAT_BIT) == JPGENC_FORMAT_YUV420)
		/* 4: YUV compress ratio */
		set_field_to_reg(base_addr + JPGENC_ADDRESS_UV_REG,
			JPGENC_ADDRESS_UV, config->buffer.input_buffer_uv >> 4);

	if ((config->buffer.format & JPGENC_FORMAT_BIT) == JPGENC_FORMAT_YUV420)
		set_field_to_reg(base_addr + JPGENC_PREREAD_REG,
			JPGENC_PREREAD, 4); /* preread value */
	else
		set_field_to_reg(base_addr + JPGENC_PREREAD_REG,
			JPGENC_PREREAD, 0);

	/* 4: stride compress ratio */
	set_field_to_reg(base_addr + JPGENC_STRIDE_REG,
		JPGENC_STRIDE, config->buffer.stride >> 4);
	set_field_to_reg(base_addr + JPGENC_JPE_ENCODE_REG,
		JPGENC_ENCODE, 1);

	cam_info("%s activate JPGENC", __func__);
	do_gettimeofday(&s_timeval_start);
	set_field_to_reg(base_addr + JPGENC_JPE_INIT_REG, JPGENC_JP_INIT, 1);
}

static int hjpeg150_prepare_buf(jpgenc_config_t *cfg)
{
	struct ion_handle *hdl = NULL;
	void *vaddr = NULL;

	if (!cfg) {
		cam_err("%s: cfg is null! %d", __func__, __LINE__);
		return -EINVAL;
	}

	/* check arg */
	if ((cfg->buffer.ion_fd < 0) || IS_ERR_OR_NULL(s_hjpeg150.ion_client)) {
		cam_err("invalid ion: fd=%d", cfg->buffer.ion_fd);
		return -EINVAL;
	}
	cam_info("%s: cfg->buffer.ion_fd is %d",
		__func__, cfg->buffer.ion_fd);

	/* get hdl */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)) /* 4,9: version */
	hdl = ion_import_dma_buf_fd(s_hjpeg150.ion_client, cfg->buffer.ion_fd);
#else
	hdl = ion_import_dma_buf(s_hjpeg150.ion_client, cfg->buffer.ion_fd);
#endif
	if (IS_ERR_OR_NULL(hdl)) {
		cam_err("failed to create ion handle!");
		return PTR_ERR(hdl);
	}
	cfg->buffer.ion_hdl = hdl;

	/* map kernel addr */
	vaddr = ion_map_kernel(s_hjpeg150.ion_client, hdl);
	if (IS_ERR_OR_NULL(vaddr)) {
		cam_err("failed to map ion buffer(fd=%lx)!", (unsigned long)hdl);
		cfg->buffer.ion_hdl = NULL;
		cfg->buffer.ion_vaddr = NULL;
		ion_free(s_hjpeg150.ion_client, hdl);
		return PTR_ERR(vaddr);
	}

	cfg->buffer.ion_vaddr = vaddr;

	return 0;
}

#ifndef SMMU
static int hjpeg150_cfg_smmu(bool bypass)
{
	void __iomem *smmu_base_addr = NULL;
	cam_info("%s enter", __func__);
	if (bypass == true) {
		/* 8192: source size */
		smmu_base_addr = ioremap_nocache(SMMU_BASE_ADDR, 8192);
		if (IS_ERR_OR_NULL(smmu_base_addr)) {
			cam_err("%s %d remap failed", __func__, __LINE__);
			return -1;
		}
		/* disable SMMU only for JPGENC */
		reg_set(smmu_base_addr + SMMU_BYPASS_VPWR,
			reg_get(smmu_base_addr + SMMU_BYPASS_VPWR) | 0x1);
		reg_set(smmu_base_addr + SMMU_BYPASS_NRRD1,
			reg_get(smmu_base_addr + SMMU_BYPASS_NRRD1) | 0x1);
		reg_set(smmu_base_addr + SMMU_BYPASS_NRRD2,
		reg_get(smmu_base_addr + SMMU_BYPASS_NRRD2) | 0x1);
		iounmap(smmu_base_addr);
	}

	return 0;
}

char *image_file_addr;
u32 image_file_size;
static int load_image_file(char *filename)
{
	struct kstat stat;
	mm_segment_t fs;
	struct file *fp = NULL;
	int file_flag = O_RDONLY;
	int ret = 0;

	cam_info("%s enter", __func__);

	if (!filename) {
		cam_err("%s param error", __func__);
		return -EINVAL;
	}

	/* must have the following 2 statement */
	fs = get_fs();
	set_fs(KERNEL_DS);
	cam_info("%s %d opening file %s", __func__, __LINE__, filename);
	fp = filp_open(filename, file_flag, 0666);
	if (IS_ERR_OR_NULL(fp)) {
		cam_err("open file error!");
		set_fs(fs);
		return -ENOENT;
	}

	if (vfs_stat(filename, &stat) != 0) {
		cam_err("failed to get file stat!");
		ret = -EIO;
		goto ERROR;
	}

	image_file_size = stat.size;
	cam_notice("image file %s, file size : %d", filename, (u32)stat.size);
	ret = vfs_read(fp,
		(char*)image_file_addr, (u32)image_file_size, &fp->f_pos);
	if (ret != stat.size) {
		cam_err("read file error!, %s , ret=%d", filename, ret);
		ret = -EIO;
	} else {
		ret = 0;
	}

ERROR:
	/* must have the following 1 statement */
	set_fs(fs);
	filp_close(fp, 0);
	return ret;
}
#endif

static void hjpeg150_disabe_irq(void)
{
	void __iomem *base = s_hjpeg150.viraddr;
	reg_set(base + JPGENC_JPE_STATUS_IMR_REG, 0x00);
}

static void hjpeg150_enable_irq(void)
{
	void __iomem *base = s_hjpeg150.viraddr;
	reg_set(base + JPGENC_JPE_STATUS_IMR_REG, 0x30);
}

static void hjpeg150_calculate_encoding_time(void)
{
	u64 tm_used;

	tm_used = (s_timeval_end.tv_sec - s_timeval_start.tv_sec)
		* MICROSECOND_PER_SECOND +
		(s_timeval_end.tv_usec - s_timeval_start.tv_usec);

	cam_info("%s JPGENC encoding elapse %llu us", __func__, tm_used);
}

static int check_jpeg_encode_param(jpgenc_config_t *pcfg)
{
	int ret;

	cam_info("width:%d, height:%d, stride:%d, format:%#x, quality:%d, "
		"rst:%d, ion_fd:%d, input_ion_fd:%d",
		pcfg->buffer.width,
		pcfg->buffer.height,
		pcfg->buffer.stride,
		pcfg->buffer.format,
		pcfg->buffer.quality,
		pcfg->buffer.rst,
		pcfg->buffer.ion_fd,
		pcfg->buffer.input_ion_fd);
	cam_info("input_buffer_y:%#x, input_buffer_uv:%#x, "
		"output_buffer:%#x, output_size:%u",
		pcfg->buffer.input_buffer_y,
		pcfg->buffer.input_buffer_uv,
		pcfg->buffer.output_buffer,
		pcfg->buffer.output_size);

	if (s_hjpeg150.power_on_state == false) {
		cam_err("%s %djpegenc is not powered on, "
			"encode processing terminated", __func__, __LINE__);
		return -1;
	}

	if (check_rst(pcfg) != 0) {
		cam_err("%s %dchecking rst failed, adjust to 0",
			__func__, __LINE__);
		return -1;
	}

#ifndef SAMPLEBACK
	ret = check_config(pcfg);
	if (ret != 0) {
		cam_err("%s %dcheck_config failed, "
			"encode processing terminated", __func__, __LINE__);
		return ret;
	}
#endif
	return 0;
}

static int hjpeg150_map_phyaddr(jpgenc_config_t *pcfg)
{
#ifdef SMMU
	int ret;
	ret = hjpeg150_prepare_buf(pcfg);
	if (ret != 0) {
		cam_err("%s %dprepare buffer failed, "
			"encode processing terminated", __func__, __LINE__);
		return ret;
	}
#else
	/* 2: map source size = width * height * 2 */
	image_file_addr = ioremap_nocache(JPEG_INPUT_PHYADDR,
		(pcfg->buffer.width * pcfg->buffer.height) * 2);
	if (IS_ERR_OR_NULL(image_file_addr)) {
		cam_err("%s %d remap failed", __func__, __LINE__);
		return -1;
	}

	load_image_file(&(pcfg->filename));

	hjpeg150_cfg_smmu(true);
#endif
	return 0;
}

static int hjpeg150_computes_size(jpgenc_config_t *pcfg, u32 *byte_cnt)
{
	*byte_cnt = reg_get(s_hjpeg150.viraddr + JPGENC_JPG_BYTE_CNT_REG);
	if (*byte_cnt == 0) {
		cam_err("%s encode fail,byte cnt [%u]", __func__, byte_cnt);
		pcfg->jpegSize = 0;
		return -ENOMEM;
	} else {
		*byte_cnt += JPGENC_HEAD_SIZE;
	}

	if (*byte_cnt > pcfg->buffer.output_size) {
		cam_err("%s encode fail, output buffer overflowed!"
			"byte cnt [%u] > buffer size [%u]",
			__func__, *byte_cnt, pcfg->buffer.output_size);
		return -ENOMEM;
	}

	pcfg->jpegSize = *byte_cnt - JPGENC_HEAD_OFFSET;

	cam_info("%s user jpeg size is %u", __func__, pcfg->jpegSize);
	cam_info("%s jpeg encode process success", __func__);

	return 0;
}

static void hjpeg150_unmap_phyaddr(jpgenc_config_t *pcfg, u32 byte_cnt)
{
#ifdef SMMU
	if (s_hjpeg150.ion_client && pcfg->buffer.ion_hdl) {
		ion_unmap_kernel(s_hjpeg150.ion_client, pcfg->buffer.ion_hdl);
		ion_free(s_hjpeg150.ion_client, pcfg->buffer.ion_hdl);
	}
#else
	/* 2: map source = width * height * 2 */
	unsigned long addr = ioremap_nocache(JPEG_OUTPUT_PHYADDR,
		pcfg->buffer.width * pcfg->buffer.height * 2);
	hjpeg150_dump_file("/data/vendor/camera/img/out.jpg", addr, byte_cnt);
	mb();

	iounmap((void*)addr);
	iounmap(image_file_addr);
#endif
}

static int hjpeg150_encode_process(hjpeg_intf_t *i, void *cfg)
{
	jpgenc_config_t *pcfg = NULL;
	long jiffies_time;
	int ret;
	u32 byte_cnt;

	cam_info("%s enter", __func__);

	if (!cfg) {
		cam_err("%s: cfg is null! %d", __func__, __LINE__);
		return -EINVAL;
	}
	pcfg = (jpgenc_config_t *)cfg;

	ret = check_jpeg_encode_param(pcfg);
	if (ret != 0)
		return ret;

	ret = hjpeg150_map_phyaddr(pcfg);
	if (ret != 0)
		return ret;

	hjpeg150_disable_autogating();
	hjpeg150_do_cvdr_config(pcfg);
	hjpeg150_enable_irq();

	hjpeg150_do_config(pcfg);

	jiffies_time = msecs_to_jiffies(WAIT_ENCODE_TIMEOUT);
	if (down_timeout(&s_hjpeg150.buff_done, jiffies_time)) {
		cam_err("time out wait for jpeg encode");
		ret = -1;
	}

	hjpeg150_calculate_encoding_time();

	dump_cvdr_reg();
	dump_dbg_reg();

	hjpeg150_disabe_irq();
	hjpeg150_enable_autogating();

	hjpeg150_add_header(pcfg);

	ret = hjpeg150_computes_size(pcfg, &byte_cnt);
	if (ret != 0)
		ret = -ENOMEM;

	hjpeg150_unmap_phyaddr(pcfg, byte_cnt);

	return ret;
}

void hjpeg150_init_hw_param(void)
{
	hjpeg150_hufftable_init();

	hjpeg150_qtable_init();

	hjpeg150_prefetch_init();

	hjpeg150_rstmarker_init();

	hjpeg150_synccfg_init();
}

static int hjpeg150_clk_ctrl(bool flag)
{
	void __iomem *subctrl1 = NULL;
	uint32_t set_clk;
	uint32_t cur_clk;
	int ret = 0;

	cam_info("%s enter", __func__);

	subctrl1 = ioremap_nocache(ISP_CORE_CTRL_1_REG, 0x4);

	if (flag == true)
		reg_set(subctrl1, reg_get(subctrl1) | 0x1);
	else
		reg_set(subctrl1, reg_get(subctrl1) & 0x0);

	set_clk = (flag == true ? 0x1 : 0x0);
	cur_clk = reg_get(subctrl1);
	if (set_clk != cur_clk) {
		cam_err("%s %d isp jpeg clk status %d, clk write failed",
			__func__, __LINE__, cur_clk);
		ret = -EIO;
	}

	cam_info("%s isp jpeg clk status %d", __func__, cur_clk);
	iounmap(subctrl1);
	return ret;
}

static int hjpeg150_power_on(hjpeg_intf_t *i)
{
	int ret = 0;
	hjpeg150_t *phjpeg150 = NULL;

	cam_info("%s enter", __func__);

	phjpeg150 = i_2_hjpeg_150(i);
	if (!phjpeg150) {
		cam_err("%s: phjpeg150 is NULL. %d", __func__, __LINE__);
		return -EINVAL;
	}

	if (phjpeg150->power_on_state == false) {
		ret = hjpeg150_res_init(&phjpeg150->pdev->dev);
		if (ret != 0) {
			cam_err("%s %d res init fail", __func__, __LINE__);
			return ret;
		}

		ret = hisp_jpeg_powerup();
		if (ret != 0) {
			cam_err("%s %d jpeg power up fail",
				__func__, __LINE__);
			goto powerup_error;
		}

		ret = hjpeg150_clk_ctrl(true);
		if (ret != 0) {
			cam_err("%s %d failed to enable jpeg clock, "
				"prepare to power down!",
				__func__, __LINE__);
			goto failed_ret;
		}

		hjpeg150_init_hw_param();
		sema_init(&(phjpeg150->buff_done), 0);

		s_hjpeg150.ion_client = hisi_ion_client_create("hwcam-hjpeg");
		if (IS_ERR_OR_NULL(s_hjpeg150.ion_client)) {
			cam_err("failed to create ion client!");
			ret = -ENOMEM;
			goto failed_ret;
		}
		phjpeg150->power_on_state = true;
		cam_info("%s jpeg power on success", __func__);
	}
	return ret;

failed_ret:
	if (hisp_jpeg_powerdn() != 0)
		cam_err("%s %d jpeg power down fail", __func__, __LINE__);

powerup_error:
	if (hjpeg150_res_deinit(&phjpeg150->pdev->dev) != 0)
		cam_err("%s %d res deinit fail", __func__, __LINE__);

	return ret;
}

static int hjpeg150_power_off(hjpeg_intf_t *i)
{
	int ret = 0;
	hjpeg150_t *phjpeg150 = NULL;
	struct ion_client *ion = NULL;

	phjpeg150 = i_2_hjpeg_150(i);
	if (!phjpeg150) {
		cam_err("%s: phjpeg150 is NULL. %d", __func__, __LINE__);
		return -EINVAL;
	}

	if (phjpeg150->power_on_state == true) {
		swap(s_hjpeg150.ion_client, ion);
		if (ion)
			ion_client_destroy(ion);

		ret = hisp_jpeg_powerdn();
		if (ret != 0)
			cam_err("%s jpeg power down fail", __func__);

		if (hjpeg150_res_deinit(&phjpeg150->pdev->dev) != 0)
			cam_err("%s %d res deinit fail",
				__func__, __LINE__);

		phjpeg150->power_on_state = false;
		if (ret == 0)
			cam_info("%s jpeg power off success", __func__);
	}
	return ret;
}

static int hjpeg150_set_reg(hjpeg_intf_t *i, void *cfg)
{
	int ret = 0;

#ifdef SAMPLEBACK
	hjpeg150_t *phjpeg150 = NULL;
	void __iomem *base_addr = 0;
	jpgenc_config_t *pcfg = NULL;
	uint32_t addr;
	uint32_t value;

	cam_info("%s enter", __func__);

	if (s_hjpeg150.power_on_state == false) {
		cam_err("%s %djpgenc is not powered on",
			__func__, __LINE__);
		return -1;
	}

	phjpeg150 = i_2_hjpeg_150(i);
	if (!phjpeg150) {
		cam_err("%s: phjpeg150 is NULL. %d", __func__, __LINE__);
		return -EINVAL;
	}
	base_addr = phjpeg150->viraddr;

	if (!cfg) {
		cam_err("%s: cfg is null! %d", __func__, __LINE__);
		return -EINVAL;
	}
	pcfg = (jpgenc_config_t*)cfg;
	addr = pcfg->reg.addr;
	value = pcfg->reg.value;

	if ((addr < JPGENC_JPE_ENCODE_REG) ||
		(addr > JPGENC_FORCE_CLK_ON_CFG_REG)) {
		cam_info("%s input addr is invaild 0x%x", __func__, addr);
		ret = -1;
		return ret;
	}

	reg_set(base_addr + addr, value);

	cam_info("%s input addr is  0x%x input value is %d",
		__func__, addr, value);
#endif

	return ret;
}

static int hjpeg150_get_reg(hjpeg_intf_t *i, void *cfg)
{
	int ret = 0;

#ifdef SAMPLEBACK
	hjpeg150_t *phjpeg150 = NULL;
	void __iomem *base_addr = 0;
	jpgenc_config_t *pcfg = NULL;
	uint32_t addr;

	cam_info("%s enter", __func__);
	if (s_hjpeg150.power_on_state == false) {
		cam_err("%s %djpgenc is not powered on",
			__func__, __LINE__);
		return -1;
	}

	phjpeg150 = i_2_hjpeg_150(i);
	if (!phjpeg150) {
		cam_err("%s: phjpeg150 is NULL. %d", __func__, __LINE__);
		return -EINVAL;
	}
	base_addr = phjpeg150->viraddr;

	if (!cfg) {
		cam_err("%s: cfg is null! %d", __func__, __LINE__);
		return -EINVAL;
	}
	pcfg = (jpgenc_config_t*)cfg;
	addr = pcfg->reg.addr;

	if ((addr < JPGENC_JPE_ENCODE_REG) ||
		(addr > JPGENC_FORCE_CLK_ON_CFG_REG)) {
		cam_info("%s input addr is invaild 0x%x", __func__, addr);
		ret = -1;
		return ret;
	}

	pcfg->reg.value = reg_get(base_addr + addr);

	cam_info("%s input addr is 0x%x input value is %d",
		__func__, addr, pcfg->reg.value);
#endif

	return ret;
}

static struct platform_driver
s_hjpeg150_driver = {
	.driver = {
		.name = "vendor,hjpeg150",
		.owner = THIS_MODULE,
		.of_match_table = s_hjpeg150_dt_match,
	},
};

static int32_t hjpeg150_platform_probe(struct platform_device *pdev)
{
	int32_t ret;
	cam_info("%s enter", __func__);

	ret = hjpeg_register(pdev, &s_hjpeg150.intf);
	s_hjpeg150.pdev = pdev;
	s_hjpeg150.power_on_state = false;
	return ret;
}

static int __init hjpeg150_init_module(void)
{
	cam_info("%s enter", __func__);
	return platform_driver_probe(&s_hjpeg150_driver,
		hjpeg150_platform_probe);
}

static void __exit hjpeg150_exit_module(void)
{
	cam_info("%s enter", __func__);
	hjpeg_unregister(s_hjpeg150.pdev);
	platform_driver_unregister(&s_hjpeg150_driver);
}

module_init(hjpeg150_init_module);
module_exit(hjpeg150_exit_module);
MODULE_DESCRIPTION("hjpeg150 driver");
MODULE_LICENSE("GPL v2");
