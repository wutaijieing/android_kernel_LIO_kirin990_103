/*
 * coresight-tmc.c
 *
 * Coresight Traffic Message Channel module
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/spinlock.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/coresight.h>
#include <linux/amba/bus.h>
#include <linux/clk.h>
#include <linux/cpuidle.h>
#include <linux/version.h>
#include <platform_include/basicplatform/linux/rdr_platform.h>
#include "coresight_priv.h"

#include <linux/platform_device.h>
#include "coresight_tmc.h"
#include <platform_include/basicplatform/linux/mntn_dump.h>
#include <securec.h>
#if defined(CONFIG_DFX_KERNELDUMP)
#include "kernel_dump.h"
#endif
#include "common_printk.h"


#define TMC_RSZ			0x004
#define TMC_STS			0x00c
#define TMC_RRD			0x010
#define TMC_RRP			0x014
#define TMC_RWP			0x018
#define TMC_TRG			0x01c
#define TMC_CTL			0x020

#define TMC_MODE		0x028

#define TMC_BUFWM		0x034

#define TMC_AXICTL		0x110
#define TMC_DBALO		0x118
#define TMC_DBAHI		0x11c
#define TMC_FFSR		0x300
#define TMC_FFCR		0x304
#define TMC_PSCR		0x308

#define MASK_L32 (0x00000000ffffffff)
#define MASK_H32 (0xffffffff00000000)

/* register description */
/* TMC_CTL - 0x020 */
#define TMC_CTL_CAPT_EN		BIT(0)
/* TMC_STS - 0x00C */
/* TMC_AXICTL - 0x110 */
#define TMC_AXICTL_PROT_CTL_B0	BIT(0)
#define TMC_AXICTL_PROT_CTL_B1	BIT(1)
#define TMC_AXICTL_SCT_GAT_MODE	BIT(7)
#define TMC_AXICTL_WR_BURST_LEN 0xF00
/* TMC_FFCR - 0x304 */
#define TMC_FFCR_EN_FMT		BIT(0)
#define TMC_FFCR_EN_TI		BIT(1)
#define TMC_FFCR_FON_FLIN	BIT(4)
#define TMC_FFCR_FON_TRIG_EVT	BIT(5)
#define TMC_FFCR_FLUSHMAN	BIT(6)
#define TMC_FFCR_TRIGON_TRIGIN	BIT(8)
#define TMC_FFCR_STOP_ON_FLUSH	BIT(12)

#define TMC_STS_TRIGGERED_BIT	2
#define TMC_FFCR_FLUSHMAN_BIT	6

#define DTS_NOC_TRACE_NAME  "/reserved-memory/noc-trace-mem"

enum tmc_config_type {
	TMC_CONFIG_TYPE_ETB,
	TMC_CONFIG_TYPE_ETR,
	TMC_CONFIG_TYPE_ETF,
};

enum tmc_mode {
	TMC_MODE_CIRCULAR_BUFFER,
	TMC_MODE_SOFTWARE_FIFO,
	TMC_MODE_HARDWARE_FIFO,
};

enum tmc_mem_intf_width {
	TMC_MEM_INTF_WIDTH_32BITS = 0x2,
	TMC_MEM_INTF_WIDTH_64BITS = 0x3,
	TMC_MEM_INTF_WIDTH_128BITS = 0x4,
	TMC_MEM_INTF_WIDTH_256BITS = 0x5,
};

/**
 * struct tmc_drvdata - specifics associated to an TMC component
 * @base:	memory mapped base address for this component.
 * @dev:	the device entity associated to this component.
 * @csdev:	component vitals needed by the framework.
 * @miscdev:	specifics to handle "/dev/xyz.tmc" entry.
 * @spinlock:	only one at a time pls.
 * @read_count:	manages preparation of buffer for reading.
 * @buf:	area of memory where trace data get sent.
 * @paddr:	DMA start location in RAM.
 * @vaddr:	virtual representation of @paddr.
 * @size:	@buf size.
 * @enable:	this TMC is being used.
 * @config_type: TMC variant, must be of type @tmc_config_type.
 * @trigger_cntr: amount of words to store after a trigger.
 * @buf_valid:     true, mean buffer data is valid.
 * @rwp:  store etr's ram write pointer. When re-start etr, write it to etr config.
 * @rd_off:  read pointer offset  of data buf for ETB and ETF
 * @wt_off:  write pointer offset of data buf for ETB and ETF
 * @mode: record tmc mode, which is used for tmc restore
 */
struct tmc_drvdata {
	void __iomem *base;
	struct device *dev;
	struct coresight_device *csdev;
	struct miscdevice miscdev;
	spinlock_t spinlock;
	int read_count;
	bool reading;
	char *buf;
	dma_addr_t paddr;
	void __iomem *vaddr;
	u32 size;
	bool enable;
	enum tmc_config_type config_type;
	u32 trigger_cntr;
	bool buf_valid;
	u64 ts_nsec;
	u32 rwp;
	u32 rd_off;
	u32 wt_off;
	struct clk *atclk;
	struct clk *pclk;
	enum tmc_mode mode;
};
static int tmc_count;
static struct tmc_drvdata *tmcdrvdata[MAX_TMC_DEV];
static unsigned int ap_dump_mem_modu_tmc_size;

#ifdef CONFIG_DFX_CORESIGHT_TRACE
static AP_RECORD_ETR *g_ap_record_etr;
#if defined(CONFIG_DFX_KERNELDUMP)
static KERNELDUMP_ETR g_kerneldump_etr;

static void tmc_kerneldump_etr_trace(u64 paddr, u32 size, u32 rd_offset)
{
	g_kerneldump_etr.paddr = paddr;
	g_kerneldump_etr.vaddr = (unsigned long long)(uintptr_t)phys_to_virt(paddr);
	g_kerneldump_etr.size = size;
	g_kerneldump_etr.rd_offset = rd_offset;
	add_extra_table(g_kerneldump_etr.paddr, size);
}
#else
static inline void tmc_kerneldump_etr_trace(u64 paddr, u32 size, u32 offset)
{
	return;
}
#endif

/*******************************************************************************
Function:       set_etr_trace_info
Description:    set etr trace data information
Input:      paddr : etr trace buffer's physical addres
		size : etr trace buffer's size
		offset: etr trace buffer's offset
Output:         NA
Return:         NA
others:         Called when etm enable and tmc are initialized
********************************************************************************/
void set_etr_trace_info(u64 paddr, u32 size, u32 offset)
{
	AP_RECORD_ETR *petr = NULL;
	int ret;

	/* check if ap record is ready */
	if (!g_ap_record_etr) {
		ret = register_mntn_dump(MNTN_DUMP_ETR, sizeof(AP_RECORD_ETR) , (void **)&petr);
		if (ret)
			return ;

		if (!petr) {
			printk(KERN_ERR "[%s]:failed to register etr record from mntn dump\n", __func__);
			return ;
		}
		g_ap_record_etr = petr;
	}

	printk(KERN_ERR "[%s]:enter petr=%pK,offset=0x%x\n",
	       __func__, g_ap_record_etr, offset);
	ret = strncpy_s(g_ap_record_etr->magic, ETR_MAGIC_SIZE, ETR_MAGIC_START, (ETR_MAGIC_SIZE - 1));
	if (ret != EOK)
		pr_err("[%s:%d]strncpy_s ret : %d", __func__, __LINE__, ret);
	g_ap_record_etr->paddr = paddr;
	g_ap_record_etr->size = size;
	g_ap_record_etr->rd_offset = (offset == (u32)-1) ? 0 : offset;
	tmc_kerneldump_etr_trace(paddr, size, g_ap_record_etr->rd_offset);

	printk(KERN_INFO "[%s]:exit\n", __func__);
}
#endif

static void tmc_wait_for_ready(struct tmc_drvdata *drvdata)
{
	/* Ensure formatter, unformatter and hardware fifo are empty */
	if (coresight_timeout(drvdata->base, TMC_STS, TMC_STS_TRIGGERED_BIT, 1))
		dev_err(drvdata->dev,
			"timeout observed when probing at offset %#x, value=0x%x,TMC_FFSR = 0x%x\n",
			TMC_STS, readl_relaxed(drvdata->base + TMC_STS),
			readl_relaxed(drvdata->base + TMC_FFSR));
}

static void tmc_flush_and_stop(struct tmc_drvdata *drvdata)
{
	u32 ffcr;

	ffcr = readl_relaxed(drvdata->base + TMC_FFCR);
	ffcr |= TMC_FFCR_STOP_ON_FLUSH;
	writel_relaxed(ffcr, drvdata->base + TMC_FFCR);
	ffcr |= TMC_FFCR_FLUSHMAN;
	writel_relaxed(ffcr, drvdata->base + TMC_FFCR);
	/* Ensure flush completes */
	if (coresight_timeout(drvdata->base,
			      TMC_FFCR, TMC_FFCR_FLUSHMAN_BIT, 0))
		dev_err(drvdata->dev,
			"timeout observed when probing at offset %#x, value=0x%x\n",
			TMC_FFCR, readl_relaxed(drvdata->base + TMC_FFCR));

	tmc_wait_for_ready(drvdata);
}

static void tmc_enable_hw(struct tmc_drvdata *drvdata)
{
	writel_relaxed(TMC_CTL_CAPT_EN, drvdata->base + TMC_CTL);
}

static void tmc_disable_hw(struct tmc_drvdata *drvdata)
{
	writel_relaxed(0x0, drvdata->base + TMC_CTL);
}

static void tmc_etb_enable_hw(struct tmc_drvdata *drvdata)
{
	/* Zero out the memory to help with debug */
	if (EOK != memset_s(drvdata->buf, drvdata->size, 0, drvdata->size))
		dev_err(drvdata->dev,"%s():%d:memset_s fail!\n", __func__, __LINE__);

	CS_UNLOCK(drvdata->base);

	writel_relaxed(TMC_MODE_CIRCULAR_BUFFER, drvdata->base + TMC_MODE);
	writel_relaxed(TMC_FFCR_EN_FMT | TMC_FFCR_EN_TI |
		       TMC_FFCR_FON_FLIN | TMC_FFCR_FON_TRIG_EVT |
		       TMC_FFCR_TRIGON_TRIGIN, drvdata->base + TMC_FFCR);

	writel_relaxed(drvdata->trigger_cntr, drvdata->base + TMC_TRG);
	tmc_enable_hw(drvdata);
	drvdata->mode = TMC_MODE_CIRCULAR_BUFFER;

	CS_LOCK(drvdata->base);
}

static void tmc_etr_enable_hw(struct tmc_drvdata *drvdata)
{
	u32 axictl;
	u32 rwp, lpaddr, hpaddr;

	lpaddr = (u32)(drvdata->paddr & MASK_L32);
	hpaddr = (u32)((drvdata->paddr & MASK_H32) >> 32);
	/* Zero out the memory to help with debug */
	if (EOK != memset_s(drvdata->vaddr, drvdata->size, 0, drvdata->size))
		dev_err(drvdata->dev,"%s():%d:memset_s fail!\n", __func__, __LINE__);

	CS_UNLOCK(drvdata->base);

	writel_relaxed(drvdata->size / 4, drvdata->base + TMC_RSZ);
	writel_relaxed(TMC_MODE_CIRCULAR_BUFFER, drvdata->base + TMC_MODE);

	axictl = readl_relaxed(drvdata->base + TMC_AXICTL);
	axictl |= TMC_AXICTL_WR_BURST_LEN;
	writel_relaxed(axictl, drvdata->base + TMC_AXICTL);
	axictl &= ~TMC_AXICTL_SCT_GAT_MODE;
	writel_relaxed(axictl, drvdata->base + TMC_AXICTL);
	axictl = (axictl &
		  ~(TMC_AXICTL_PROT_CTL_B0 | TMC_AXICTL_PROT_CTL_B1)) |
	    TMC_AXICTL_PROT_CTL_B1;
	writel_relaxed(axictl, drvdata->base + TMC_AXICTL);

	writel_relaxed(lpaddr, drvdata->base + TMC_DBALO);
	writel_relaxed(hpaddr, drvdata->base + TMC_DBAHI);
	writel_relaxed(TMC_FFCR_EN_FMT | TMC_FFCR_EN_TI |
		       TMC_FFCR_FON_FLIN | TMC_FFCR_FON_TRIG_EVT |
		       TMC_FFCR_TRIGON_TRIGIN, drvdata->base + TMC_FFCR);
	writel_relaxed(drvdata->trigger_cntr, drvdata->base + TMC_TRG);
	writel_relaxed(drvdata->rwp, drvdata->base + TMC_RWP);
	rwp = readl_relaxed(drvdata->base + TMC_RWP);
	dev_info(drvdata->dev, "wrp=0x%x : 0x%x\n", drvdata->rwp, rwp);

	tmc_enable_hw(drvdata);
	set_etr_trace_info(drvdata->paddr, drvdata->size, (u32) (-1));
	CS_LOCK(drvdata->base);
}

static void tmc_etf_enable_hw(struct tmc_drvdata *drvdata)
{
	CS_UNLOCK(drvdata->base);

	writel_relaxed(TMC_MODE_HARDWARE_FIFO, drvdata->base + TMC_MODE);
	writel_relaxed(TMC_FFCR_EN_FMT | TMC_FFCR_EN_TI,
		       drvdata->base + TMC_FFCR);
	writel_relaxed(0x0, drvdata->base + TMC_BUFWM);
	tmc_enable_hw(drvdata);
	drvdata->mode = TMC_MODE_HARDWARE_FIFO;

	CS_LOCK(drvdata->base);
}

static int tmc_enable(struct tmc_drvdata *drvdata, enum tmc_mode mode)
{
	unsigned long flags;
	int ret;

	if (!IS_ERR_OR_NULL(drvdata->atclk)) {
		ret = clk_prepare_enable(drvdata->atclk);
		if (ret) {
			dev_err(drvdata->dev, "enable atclk fail \n");
			return ret;
		}
	}

	spin_lock_irqsave(&drvdata->spinlock, flags);
	if (drvdata->reading) {
		spin_unlock_irqrestore(&drvdata->spinlock, flags);
		if (!IS_ERR_OR_NULL(drvdata->atclk))
			clk_disable_unprepare(drvdata->atclk);
		return -EBUSY;
	}

	if (drvdata->config_type == TMC_CONFIG_TYPE_ETB) {
		tmc_etb_enable_hw(drvdata);
	} else if (drvdata->config_type == TMC_CONFIG_TYPE_ETR) {
		tmc_etr_enable_hw(drvdata);
	} else {
		if (mode == TMC_MODE_CIRCULAR_BUFFER)
			tmc_etb_enable_hw(drvdata);
		else
			tmc_etf_enable_hw(drvdata);
	}
	drvdata->enable = true;
	spin_unlock_irqrestore(&drvdata->spinlock, flags);

	dev_info(drvdata->dev, "TMC enabled\n");
	return 0;
}

static int tmc_enable_sink(struct coresight_device *csdev)
{
	struct tmc_drvdata *drvdata = dev_get_drvdata(csdev->dev.parent);

	return tmc_enable(drvdata, TMC_MODE_CIRCULAR_BUFFER);
}

static int tmc_enable_link(struct coresight_device *csdev, int inport,
			   int outport)
{
	struct tmc_drvdata *drvdata = dev_get_drvdata(csdev->dev.parent);

	return tmc_enable(drvdata, TMC_MODE_HARDWARE_FIFO);
}

static void tmc_etb_dump_hw(struct tmc_drvdata *drvdata)
{
	enum tmc_mem_intf_width memwidth;
	u8 memwords;
	char *bufp = NULL;
	u32 read_data;
	int i;

	memwidth = (enum tmc_mem_intf_width)BMVAL((unsigned int)readl_relaxed(drvdata->base + CORESIGHT_DEVID), 8, 10);/*lint !e648 */
	if (memwidth == TMC_MEM_INTF_WIDTH_32BITS)
		memwords = 1;
	else if (memwidth == TMC_MEM_INTF_WIDTH_64BITS)
		memwords = 2;
	else if (memwidth == TMC_MEM_INTF_WIDTH_128BITS)
		memwords = 4;
	else
		memwords = 8;

	bufp = drvdata->buf + drvdata->wt_off;
	while (1) {
		for (i = 0; i < memwords; i++) {
			read_data = readl_relaxed(drvdata->base + TMC_RRD);
			if (read_data == 0xFFFFFFFF) {
				goto out;
			}
			if (EOK != memcpy_s(bufp, BYTES_PER_WORD, &read_data, BYTES_PER_WORD))
				dev_err(drvdata->dev,"%s():%d:memcpy_s fail!\n", __func__, __LINE__);
			bufp += 4;
			drvdata->wt_off += 4;
			/* write pointer is wrapped */
			if (drvdata->wt_off >= drvdata->size) {
				drvdata->wt_off = 0;
				bufp = drvdata->buf + drvdata->wt_off;
			}
		}
	}

out:
	if ((bufp - drvdata->buf) % (memwords * BYTES_PER_WORD)) {
		dev_err(drvdata->dev,
			"ETF-ETB data is not %lx bytes aligned, wt_off=0x%x\n",
			(unsigned long)memwords * BYTES_PER_WORD,
			drvdata->wt_off);
		drvdata->wt_off =
		    (drvdata->wt_off & ~((u32)memwords * BYTES_PER_WORD)) +
		    (u32)memwords * BYTES_PER_WORD;
	}

	drvdata->rd_off = drvdata->wt_off;
	dev_info(drvdata->dev, "etb rd_off=0x%x, memwidth=%d \n",
		 drvdata->wt_off, memwords);
	drvdata->buf_valid = true;
	drvdata->ts_nsec = dfx_getcurtime();
	return;
}

static void tmc_etb_disable_hw(struct tmc_drvdata *drvdata)
{
	CS_UNLOCK(drvdata->base);

	tmc_flush_and_stop(drvdata);
	tmc_etb_dump_hw(drvdata);
	tmc_disable_hw(drvdata);

	CS_LOCK(drvdata->base);
}

static void tmc_etr_dump_hw(struct tmc_drvdata *drvdata)
{
	u32 rwp, val, lpaddr;

	rwp = readl_relaxed(drvdata->base + TMC_RWP);
	val = readl_relaxed(drvdata->base + TMC_STS);
	lpaddr = readl_relaxed(drvdata->base + TMC_DBALO);
	/* How much memory do we still have */
	if (val & BIT(0))
		drvdata->buf = drvdata->vaddr + rwp - lpaddr;
	else
		drvdata->buf = drvdata->vaddr;
	info_print("etr: rwp=0x%x, offset=0x%x,val=0x%x\n", rwp,
		   (u32) (rwp - lpaddr), val);
	drvdata->rwp = rwp;
	drvdata->wt_off = rwp - lpaddr;
	drvdata->rd_off = drvdata->wt_off;
	drvdata->buf_valid = true;
	drvdata->ts_nsec = dfx_getcurtime();
}

static void tmc_etr_disable_hw(struct tmc_drvdata *drvdata)
{
	CS_UNLOCK(drvdata->base);

	tmc_flush_and_stop(drvdata);
	tmc_etr_dump_hw(drvdata);
	tmc_disable_hw(drvdata);

	CS_LOCK(drvdata->base);
}

static void tmc_etf_dump_hw(struct tmc_drvdata *drvdata)
{
	enum tmc_mem_intf_width memwidth;
	u8 memwords;
	char *bufp = NULL;
	u32 read_data;
	u32 read_len;
	int i;

	memwidth = (enum tmc_mem_intf_width)BMVAL((unsigned int)readl_relaxed(drvdata->base + CORESIGHT_DEVID), 8, 10);/*lint !e648 */
	if (memwidth == TMC_MEM_INTF_WIDTH_32BITS) {
		memwords = 1;
	} else if (memwidth == TMC_MEM_INTF_WIDTH_64BITS) {
		memwords = 2;
	} else if (memwidth == TMC_MEM_INTF_WIDTH_128BITS) {
		memwords = 4;
	} else {
		memwords = 8;
	}

	tmc_disable_hw(drvdata);
	bufp = drvdata->buf + drvdata->wt_off;
	read_len = 0;
	while (read_len < drvdata->size) {
		for (i = 0; i < memwords; i++) {
			read_data = readl_relaxed(drvdata->base + TMC_RRD);
			if (read_data == 0xFFFFFFFF) {
				goto out;
			}
			if (EOK != memcpy_s(bufp, BYTES_PER_WORD, &read_data, BYTES_PER_WORD))
				dev_err(drvdata->dev,"%s():%d:memcpy_s fail!\n", __func__, __LINE__);
			read_len += 4;
			bufp += 4;
			drvdata->wt_off += 4;
			/* write pointer is wrapped */
			if (drvdata->wt_off >= drvdata->size) {
				drvdata->wt_off = 0;
				bufp = drvdata->buf + drvdata->wt_off;
			}
		}
	}

out:
	if ((bufp - drvdata->buf) % (memwords * BYTES_PER_WORD)) {
		dev_err(drvdata->dev,
			"ETF-ETB data is not %lx bytes aligned, wt_off=0x%x\n",
			(unsigned long)memwords * BYTES_PER_WORD,
			drvdata->wt_off);
		drvdata->wt_off =
		    (drvdata->wt_off & ~((u32)memwords * BYTES_PER_WORD)) +
		    (u32)memwords * BYTES_PER_WORD;
	}
	drvdata->rd_off = drvdata->wt_off;

	dev_info(drvdata->dev, "read_len =0x%x, disable memwidth=0x%x\n",
		 read_len, memwords);
	drvdata->buf_valid = true;
	drvdata->ts_nsec = dfx_getcurtime();
}

static void tmc_etf_disable_hw(struct tmc_drvdata *drvdata)
{
	CS_UNLOCK(drvdata->base);

	tmc_flush_and_stop(drvdata);

	tmc_etf_dump_hw(drvdata);
	tmc_disable_hw(drvdata);

	CS_LOCK(drvdata->base);
}

static void tmc_disable(struct tmc_drvdata *drvdata, enum tmc_mode mode)
{
	unsigned long flags;

	spin_lock_irqsave(&drvdata->spinlock, flags);
	if (drvdata->reading)
		goto out;

	if (drvdata->config_type == TMC_CONFIG_TYPE_ETB) {
		tmc_etb_disable_hw(drvdata);
	} else if (drvdata->config_type == TMC_CONFIG_TYPE_ETR) {
		tmc_etr_disable_hw(drvdata);
	} else {
		if (mode == TMC_MODE_CIRCULAR_BUFFER)
			tmc_etb_disable_hw(drvdata);
		else
			tmc_etf_disable_hw(drvdata);
	}
out:
	drvdata->enable = false;
	spin_unlock_irqrestore(&drvdata->spinlock, flags);

	if (!IS_ERR_OR_NULL(drvdata->atclk))
		clk_disable_unprepare(drvdata->atclk);
	dev_info(drvdata->dev, "TMC disabled\n");
}

static void tmc_disable_sink(struct coresight_device *csdev)
{
	struct tmc_drvdata *drvdata = dev_get_drvdata(csdev->dev.parent);

	tmc_disable(drvdata, TMC_MODE_CIRCULAR_BUFFER);
}

static void tmc_disable_link(struct coresight_device *csdev, int inport,
			     int outport)
{
	struct tmc_drvdata *drvdata = dev_get_drvdata(csdev->dev.parent);

	tmc_disable(drvdata, TMC_MODE_HARDWARE_FIFO);
}

static const struct coresight_ops_sink tmc_sink_ops = {
	.enable = tmc_enable_sink,
	.disable = tmc_disable_sink,
};

static const struct coresight_ops_link tmc_link_ops = {
	.enable = tmc_enable_link,
	.disable = tmc_disable_link,
};

static const struct coresight_ops tmc_etb_cs_ops = {
	.sink_ops = &tmc_sink_ops,
};

static const struct coresight_ops tmc_etr_cs_ops = {
	.sink_ops = &tmc_sink_ops,
};

static const struct coresight_ops tmc_etf_cs_ops = {
	.sink_ops = &tmc_sink_ops,
	.link_ops = &tmc_link_ops,
};

static int tmc_read_prepare(struct tmc_drvdata *drvdata)
{
	unsigned long flags;
	enum tmc_mode mode;

	spin_lock_irqsave(&drvdata->spinlock, flags);
	if (!drvdata->enable)
		goto out;

	if (drvdata->config_type == TMC_CONFIG_TYPE_ETB) {
		tmc_etb_disable_hw(drvdata);
	} else if (drvdata->config_type == TMC_CONFIG_TYPE_ETR) {
		tmc_etr_disable_hw(drvdata);
	} else {
		mode = readl_relaxed(drvdata->base + TMC_MODE);
		if (mode == TMC_MODE_CIRCULAR_BUFFER) {
			tmc_etb_disable_hw(drvdata);
		} else {
			tmc_etf_disable_hw(drvdata);
		}
	}
out:
	drvdata->reading = true;
	spin_unlock_irqrestore(&drvdata->spinlock, flags);

	dev_info(drvdata->dev, "TMC read start\n");
	return 0;
}

static void tmc_read_unprepare(struct tmc_drvdata *drvdata)
{
	unsigned long flags;
	enum tmc_mode mode;

	spin_lock_irqsave(&drvdata->spinlock, flags);
	if (!drvdata->enable)
		goto out;

	if (drvdata->config_type == TMC_CONFIG_TYPE_ETB) {
		tmc_etb_enable_hw(drvdata);
	} else if (drvdata->config_type == TMC_CONFIG_TYPE_ETR) {
		tmc_etr_enable_hw(drvdata);
	} else {
		mode = readl_relaxed(drvdata->base + TMC_MODE);
		if (mode == TMC_MODE_CIRCULAR_BUFFER)
			tmc_etb_enable_hw(drvdata);
	}
out:
	drvdata->reading = false;
	spin_unlock_irqrestore(&drvdata->spinlock, flags);

	dev_info(drvdata->dev, "TMC read end\n");
}

static int tmc_open(struct inode *inode, struct file *file)
{
	struct tmc_drvdata *drvdata = container_of(file->private_data,
						   struct tmc_drvdata, miscdev);
	int ret;

	if (drvdata->read_count++)
		goto out;

	ret = tmc_read_prepare(drvdata);
	if (ret)
		return ret;
out:
	nonseekable_open(inode, file);

	dev_dbg(drvdata->dev, "%s: successfully opened\n", __func__);
	return 0;
}

static ssize_t tmc_read(struct file *file, char __user *data, size_t len,
			loff_t *ppos)
{
	struct tmc_drvdata *drvdata = container_of(file->private_data,
						   struct tmc_drvdata, miscdev);
	char *bufp = drvdata->buf + *ppos;

	if (!data)
		return -EFAULT;

	if (!len)
		return -EFAULT;

	if (*ppos + len > drvdata->size)
		len = drvdata->size - *ppos;

	if (drvdata->config_type == TMC_CONFIG_TYPE_ETR) {
		if (bufp == (char *)(drvdata->vaddr + drvdata->size))
			bufp = drvdata->vaddr;
		else if (bufp > (char *)(drvdata->vaddr + drvdata->size))
			bufp -= drvdata->size;
		if ((bufp + len) > (char *)(drvdata->vaddr + drvdata->size))
			len = (char *)(drvdata->vaddr + drvdata->size) - bufp;
	}

	if (copy_to_user(data, bufp, len)) {
		dev_dbg(drvdata->dev, "%s: copy_to_user failed\n", __func__);
		return -EFAULT;
	}

	*ppos += len;

	dev_dbg(drvdata->dev, "%s: %zu bytes copied, %d bytes left\n",
		__func__, len, (int)(drvdata->size - *ppos));
	return len;
}

static int tmc_release(struct inode *inode, struct file *file)
{
	struct tmc_drvdata *drvdata = container_of(file->private_data,
						   struct tmc_drvdata, miscdev);

	if (--drvdata->read_count) {
		if (drvdata->read_count < 0) {
			dev_err(drvdata->dev, "mismatched close\n");
			drvdata->read_count = 0;
		}
		goto out;
	}

	tmc_read_unprepare(drvdata);
out:
	dev_dbg(drvdata->dev, "%s: released\n", __func__);
	return 0;
}

static const struct file_operations tmc_fops = {
	.owner = THIS_MODULE,
	.open = tmc_open,
	.read = tmc_read,
	.release = tmc_release,
	.llseek = no_llseek,
};

static ssize_t status_show(struct device *dev,
			   struct device_attribute *attr, char *buf)
{
	unsigned long flags;
	u32 tmc_rsz, tmc_sts, tmc_rrp, tmc_rwp, tmc_trg;
	u32 tmc_ctl, tmc_ffsr, tmc_ffcr, tmc_mode, tmc_pscr;
	u32 devid;
	char buffer[512];
	int len;
	struct tmc_drvdata *drvdata = dev_get_drvdata(dev->parent);

	pm_runtime_get_sync(drvdata->dev);
	spin_lock_irqsave(&drvdata->spinlock, flags);
	cpuidle_pause();
	CS_UNLOCK(drvdata->base);
	tmc_rsz = readl_relaxed(drvdata->base + TMC_RSZ);
	tmc_sts = readl_relaxed(drvdata->base + TMC_STS);
	tmc_rrp = readl_relaxed(drvdata->base + TMC_RRP);
	tmc_rwp = readl_relaxed(drvdata->base + TMC_RWP);
	tmc_trg = readl_relaxed(drvdata->base + TMC_TRG);
	tmc_ctl = readl_relaxed(drvdata->base + TMC_CTL);
	tmc_ffsr = readl_relaxed(drvdata->base + TMC_FFSR);
	tmc_ffcr = readl_relaxed(drvdata->base + TMC_FFCR);
	tmc_mode = readl_relaxed(drvdata->base + TMC_MODE);
	tmc_pscr = readl_relaxed(drvdata->base + TMC_PSCR);
	devid = readl_relaxed(drvdata->base + CORESIGHT_DEVID);
	CS_LOCK(drvdata->base);
	cpuidle_resume();
	spin_unlock_irqrestore(&drvdata->spinlock, flags);
	pm_runtime_put(drvdata->dev);

	/* cppcheck-suppress **/
	len = sprintf_s(buffer, sizeof(buffer),
		       "Depth:\t\t0x%x\n"
		       "Status:\t\t0x%x\n"
		       "RAM read ptr:\t0x%x\n"
		       "RAM wrt ptr:\t0x%x\n"
		       "Trigger cnt:\t0x%x\n"
		       "Control:\t0x%x\n"
		       "Flush status:\t0x%x\n"
		       "Flush ctrl:\t0x%x\n"
		       "Mode:\t\t0x%x\n"
		       "PSRC:\t\t0x%x\n"
		       "DEVID:\t\t0x%x\n",
		       tmc_rsz, tmc_sts, tmc_rrp, tmc_rwp, tmc_trg,
		       tmc_ctl, tmc_ffsr, tmc_ffcr, tmc_mode, tmc_pscr, devid);/*lint !e421*/
	if ( len < 0) {
		err_print("%s:%d sprintf_s\n", __func__, __LINE__);
		return -EFAULT;
	}

	if ( buf == NULL ) {
		err_print("%s:%d buf pointer is NULL\n", __func__, __LINE__);
		return -EFAULT;
	}
	if (copy_to_user(buf, buffer, len)) {
		err_print("%s:%d copy to user error\n", __func__, __LINE__);
		return -EFAULT;
	}

	return len;
}

static DEVICE_ATTR_RO(status);

static ssize_t trigger_cntr_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct tmc_drvdata *drvdata = dev_get_drvdata(dev->parent);
	unsigned long val = drvdata->trigger_cntr;

	return snprintf_s(buf, PAGE_SIZE, sizeof(val), "%#lx\n", val);
}

static ssize_t trigger_cntr_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	int ret;
	unsigned long val;
	struct tmc_drvdata *drvdata = dev_get_drvdata(dev->parent);

	ret = kstrtoul(buf, 16, &val);
	if (ret)
		return ret;

	drvdata->trigger_cntr = val;
	return size;
}

static DEVICE_ATTR_RW(trigger_cntr);

static struct attribute *coresight_etb_attrs[] = {
	&dev_attr_trigger_cntr.attr,
	&dev_attr_status.attr,
	NULL,
};

ATTRIBUTE_GROUPS(coresight_etb);

static struct attribute *coresight_etr_attrs[] = {
	&dev_attr_trigger_cntr.attr,
	&dev_attr_status.attr,
	NULL,
};

ATTRIBUTE_GROUPS(coresight_etr);

static struct attribute *coresight_etf_attrs[] = {
	&dev_attr_trigger_cntr.attr,
	&dev_attr_status.attr,
	NULL,
};

ATTRIBUTE_GROUPS(coresight_etf);

int get_noctrace_reserve_addr(struct device *dev, dma_addr_t *paddr,
			      unsigned int *size, void __iomem **vaddr)
{
	struct device_node *np = NULL;
	const char *status = NULL;
	int	len;
	const unsigned long *p = NULL;

	np = of_find_node_by_path(DTS_NOC_TRACE_NAME);
	if (!np) {
		dev_err(dev, "could not get noc trace reserve_addr\n");
		return -1;
	}

	/* check if status = ok, okay or status not defined */
	status = of_get_property(np, "status", &len);
	if (status && strncmp(status, "okay", sizeof("okay")) != 0 &&
	    strncmp(status, "ok", sizeof("ok")) != 0) {
		dev_err(dev, "get status %s error\n", DTS_NOC_TRACE_NAME);
		goto err;
	}

	p = (unsigned long *)of_get_property(np, "reg", NULL);
	if (!p) {
		dev_err(dev, "get reg fail, len = %d\n", len);
		goto err;
	}

	*paddr = (u64)be64_to_cpup((const __be64 *)p);
	*size = (unsigned int)be64_to_cpup((const __be64 *) ++p);

	*vaddr = (void *)ioremap_wc((phys_addr_t)*paddr, *size);
	if (*vaddr == NULL) {
		dev_err(dev, "get_noc_trace_dump_reserve_addr fail\n");
		goto err;
	}
	of_node_put(np);
	return 0;
err:
	of_node_put(np);
	return -1;
}

static int tmc_probe(struct amba_device *adev, const struct amba_id *id)
{
	int ret = 0;
	u32 devid;
	void __iomem *base = NULL;
	struct device *dev = &adev->dev;
	struct coresight_platform_data *pdata = NULL;
	struct tmc_drvdata *drvdata = NULL;
	struct resource *res = &adev->res;
	struct coresight_desc *desc = NULL;
	struct device_node *np = adev->dev.of_node;
	bool default_sink = false;
	const char *tmc_type = NULL;

	dev_info(dev, "tmc_probe initialized\n");
	if (!coresight_access_enabled()) {
		dev_err(dev,
			"coresight trace is sec debug mode ,coresight trace disable\n");
		return -EACCES;
	}
	if (np) {
		pdata = of_get_coresight_platform_data(dev, np);
		if (IS_ERR(pdata))
			return PTR_ERR(pdata);
		adev->dev.platform_data = pdata;
		default_sink =
		    of_property_read_bool(np, "coresight-default-sink");
		ret = of_property_read_string(np, "tmc_type", &tmc_type);
		if (ret != 0) {
			dev_err(dev, "failed to get tmc_type\n");
			tmc_type = NULL;
		}
	}
	if (!pdata)
		return -ENOMEM;

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata)
		return -ENOMEM;

	drvdata->dev = &adev->dev;
	dev_set_drvdata(dev, drvdata);

	/* Validity for the resource is already checked by the AMBA core */
	base = devm_ioremap_resource(dev, res);
	if (IS_ERR(base))
		return PTR_ERR(base);/*lint !e429 */

	drvdata->base = base;

	drvdata->atclk = devm_clk_get(&adev->dev, TOPCS_ATCLK_NAME);	 /* optional */
	if (!IS_ERR_OR_NULL(drvdata->atclk)) {
		dev_info(dev, "get atclk OK\n");
		ret = clk_prepare_enable(drvdata->atclk);
		if (ret) {
			dev_err(dev, "enable atclk fail \n");
			return PTR_ERR(drvdata->atclk);/*lint !e429 */
		}
	} else {
		dev_err(dev, "get atclk fail \n");
	}

	drvdata->pclk = devm_clk_get(&adev->dev, "apb_pclk");	 /* optional */
	if (!IS_ERR_OR_NULL(drvdata->pclk)) {
		dev_info(dev, "get pclk OK \n");
		ret = clk_prepare_enable(drvdata->pclk);
		if (ret)
			dev_err(dev, "enable pclk fail \n");
	} else {
		dev_err(dev, "get pclk fail \n");
	}

	spin_lock_init(&drvdata->spinlock);

	cpuidle_pause();
	devid = readl_relaxed(drvdata->base + CORESIGHT_DEVID);
	drvdata->config_type = (enum tmc_config_type)BMVAL(devid, 6, 7);/*lint !e648 */
	dev_err(dev, "devid =%d\n", drvdata->config_type);

	if (drvdata->config_type == TMC_CONFIG_TYPE_ETR) {
		if (np)
			ret = of_property_read_u32(np,
						   "arm,buffer-size",
						   &drvdata->size);
		if (ret)
			drvdata->size = SZ_1M;
	} else {
		drvdata->size = readl_relaxed(drvdata->base + TMC_RSZ) * 4;
	}

	cpuidle_resume();
	dev_err(dev, "devid size =%d\n", drvdata->size);

	pm_runtime_put(&adev->dev);

	if (drvdata->config_type == TMC_CONFIG_TYPE_ETR) {
		ret = get_noctrace_reserve_addr(dev, &drvdata->paddr,
						&drvdata->size,
						&drvdata->vaddr);
		if (ret < 0)
			drvdata->vaddr = dma_alloc_coherent(dev, drvdata->size,
							   &drvdata->paddr,
							   GFP_KERNEL);

		if (!drvdata->vaddr) {
			ret = -ENOMEM;
			goto err_next;
		}

		if (EOK != memset_s(drvdata->vaddr, drvdata->size, 0xff, drvdata->size))
			dev_err(dev,"%s():%d:memset_s fail!\n", __func__, __LINE__);
		drvdata->buf = drvdata->vaddr;

#if defined(CONFIG_DFX_KERNELDUMP)
		add_extra_table(drvdata->paddr, drvdata->size);
#endif
	} else {
		drvdata->buf = devm_kzalloc(dev, drvdata->size, GFP_KERNEL);
		if (!drvdata->buf) {
			ret = -ENOMEM;
			goto err_next;
		}
		if (EOK != memset_s(drvdata->buf, drvdata->size, 0xff, drvdata->size))
			dev_err(dev,"%s():%d:memset_s fail!\n", __func__, __LINE__);
	}
	drvdata->buf_valid = false;
	drvdata->rwp = 0;
	drvdata->rd_off = 0;
	drvdata->wt_off = 0;

	desc = devm_kzalloc(dev, sizeof(*desc), GFP_KERNEL);
	if (!desc) {
		ret = -ENOMEM;
		goto err_devm_kzalloc;
	}

	desc->pdata = pdata;
	desc->dev = dev;
	desc->subtype.sink_subtype = CORESIGHT_DEV_SUBTYPE_SINK_BUFFER;

	if (drvdata->config_type == TMC_CONFIG_TYPE_ETB) {
		desc->type = CORESIGHT_DEV_TYPE_SINK;
		desc->ops = &tmc_etb_cs_ops;
		desc->groups = coresight_etb_groups;
	} else if (drvdata->config_type == TMC_CONFIG_TYPE_ETR) {
		desc->type = CORESIGHT_DEV_TYPE_SINK;
		desc->ops = &tmc_etr_cs_ops;
		desc->groups = coresight_etr_groups;
	} else {
		desc->type = CORESIGHT_DEV_TYPE_LINKSINK;
		desc->subtype.link_subtype = CORESIGHT_DEV_SUBTYPE_LINK_FIFO;
		desc->ops = &tmc_etf_cs_ops;
		desc->groups = coresight_etf_groups;
	}

	drvdata->csdev = coresight_register(desc);
	if (IS_ERR(drvdata->csdev)) {
		dev_err(dev, "drvdata->csdev = 0x%lx", PTR_ERR(drvdata->csdev));
		ret = -EFAULT;
		goto err_coresight_register;
	}

	drvdata->miscdev.name = pdata->name;
	drvdata->miscdev.minor = MISC_DYNAMIC_MINOR;
	drvdata->miscdev.fops = &tmc_fops;
	ret = misc_register(&drvdata->miscdev);
	if (ret)
		goto err_misc_register;

	/* default enable */
	dev_err(dev, "tmc defult sink=%d\n", default_sink);
	drvdata->csdev->activated = default_sink;
	dev_info(dev, "TMC initialized\n");

	if (tmc_count >= MAX_TMC_DEV) {
		dev_err(dev, "ERROR: Too much tmc device \n");
	} else {
		tmcdrvdata[tmc_count++] = drvdata;
	}
	if (!IS_ERR_OR_NULL(drvdata->atclk))
		clk_disable_unprepare(drvdata->atclk);
	return 0;/*lint !e429 !e593*/

err_misc_register:
	coresight_unregister(drvdata->csdev);
err_coresight_register:
	devm_kfree(dev, desc);
err_devm_kzalloc:
	if (drvdata->config_type == TMC_CONFIG_TYPE_ETR) {
		dma_free_coherent(dev, drvdata->size,
				  &drvdata->paddr, GFP_KERNEL);
	} else {
		devm_kfree(dev, drvdata->buf);
	}
err_next:
	if (!IS_ERR_OR_NULL(drvdata->atclk))
		clk_disable_unprepare(drvdata->atclk);
	devm_kfree(dev, drvdata);
	return ret;/*lint !e429 */
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
static void tmc_remove(struct amba_device *adev)
{
	struct tmc_drvdata *drvdata = amba_get_drvdata(adev);

	misc_deregister(&drvdata->miscdev);
	coresight_unregister(drvdata->csdev);
	if (drvdata->config_type == TMC_CONFIG_TYPE_ETR)
		dma_free_coherent(drvdata->dev, drvdata->size,
				  &drvdata->paddr, GFP_KERNEL);

	tmcdrvdata[--tmc_count] = 0;
}
#else
static int tmc_remove(struct amba_device *adev)
{
	struct tmc_drvdata *drvdata = amba_get_drvdata(adev);

	misc_deregister(&drvdata->miscdev);
	coresight_unregister(drvdata->csdev);
	if (drvdata->config_type == TMC_CONFIG_TYPE_ETR)
		dma_free_coherent(drvdata->dev, drvdata->size,
				  &drvdata->paddr, GFP_KERNEL);

	tmcdrvdata[--tmc_count] = 0;
	return 0;
}
#endif

static struct amba_id tmc_ids[] = {
	{
	 .id = 0x0003b961,
	 .mask = 0x0003ffff,
	 },
	{0, 0},
};

static struct amba_driver tmc_driver = {
	.drv = {
		.name = "coresight-tmc",
		.owner = THIS_MODULE,
		},
	.probe = tmc_probe,
	.remove = tmc_remove,
	.id_table = tmc_ids,
};

module_amba_driver(tmc_driver);

#if defined(CONFIG_DFX_CORESIGHT_TRACE)
/*
Function: tcm_dump
return: 0 is fail. Non-zero is success
*/
static int tmc_dump(void *dump_addr, unsigned int size)
{
	int i;
	int ret;
	char *ptr = NULL;
	struct tmc_drvdata *drvdata = NULL;
	unsigned int dump_len = 0;
	unsigned int buflen;
	struct tcm_dump_head *pdump_hd = NULL;

	if (NULL == dump_addr || (size > ap_dump_mem_modu_tmc_size))
		return 0;
	ptr = (char *)dump_addr;
	info_print("max buffer_len=0x%d\n", size);

	for (i = 0; i < tmc_count; i++) {
		drvdata = tmcdrvdata[i];
		if (!drvdata) {
			break;
		}

		if (!drvdata->buf_valid) {
			continue;
		}

		if ((dump_len + HEADER_SIZE + drvdata->size) > size) {
			/* dump etr information */
			if (drvdata->config_type == TMC_CONFIG_TYPE_ETR) {
				set_etr_trace_info(drvdata->paddr,
						   drvdata->size,
						   (u32) (drvdata->buf -
							  (char *)drvdata->
							  vaddr));
				info_print("tcm_dump buf=%pK,vaddr=%pK\n",
					   drvdata->buf,
					   (char *)drvdata->vaddr);
			}

			if ((dump_len + HEADER_SIZE + BUFF_LEN) <= size) {
				info_print
				    ("small dump size. tmc dev=%s, only save tmc information\n",
				     drvdata->miscdev.name);

				/* reserve tmc header */
				pdump_hd = (struct tcm_dump_head *)ptr;
				ret = strncpy_s(pdump_hd->magic, TMC_MAGIC_SIZE, TMC_MAGIC_START, (TMC_MAGIC_SIZE - 1));
				if (ret != EOK)
					pr_err("[%s:%d]strncpy_s ret : %d", __func__, __LINE__, ret);
				pdump_hd->data_size = BUFF_LEN;
				pdump_hd->ts_nsec = drvdata->ts_nsec;

				ret = strncpy_s(pdump_hd->name, TMC_NAME_LEN, drvdata->miscdev.name, (TMC_NAME_LEN - 1));
				if (ret != EOK)
					pr_err("[%s:%d]strncpy_s ret : %d", __func__, __LINE__, ret);
				pdump_hd->name[TMC_NAME_LEN - 1] = '\0';

				dump_len += HEADER_SIZE;
				ptr += HEADER_SIZE;

				ret = snprintf_s(ptr, (size - dump_len), BUFF_LEN,
					 "TMC data in ramdump \n vaddr:%pK\n paddr:%pK\n size:0x%x\n rd_ptr_offset:0x%x\n",
					 drvdata->vaddr, (void *)(uintptr_t)drvdata->paddr,
					 drvdata->size,
					 (u32) (drvdata->buf -
						(char *)drvdata->vaddr));
				if (ret < 0)
					err_print("[%s:%d]snprintf_s error\n", __func__, __LINE__);
				*(ptr + BUFF_LEN - 1) = '\0';

				dump_len += BUFF_LEN;
				ptr += BUFF_LEN;
			} else {
				err_print
				    ("small dump size. tmc dev=%s, no tmc data saved\n",
				     drvdata->miscdev.name);
			}
			continue;
		}

		/* reserve tmc header */
		pdump_hd = (struct tcm_dump_head *)ptr;
		ret = strncpy_s(pdump_hd->magic, TMC_MAGIC_SIZE, TMC_MAGIC_START, (TMC_MAGIC_SIZE - 1));
		if (ret != EOK)
			err_print("[%s:%d]strncpy_s ret : %d", __func__, __LINE__, ret);
		pdump_hd->magic[TMC_MAGIC_SIZE - 1] = '\0';
		pdump_hd->data_size = drvdata->size;
		pdump_hd->ts_nsec = drvdata->ts_nsec;

		ret = strncpy_s(pdump_hd->name, TMC_NAME_LEN, drvdata->miscdev.name, (TMC_NAME_LEN - 1));
		if (ret != EOK)
			err_print("[%s:%d]strncpy_s ret : %d", __func__, __LINE__, ret);

		dump_len += HEADER_SIZE;
		ptr += HEADER_SIZE;

		/* copy tmc data */
		if (drvdata->config_type == TMC_CONFIG_TYPE_ETR) {
			buflen = 0;
			if ((void *)(drvdata->buf) > (void *)(drvdata->vaddr)) {
				buflen =
				    (char *)drvdata->vaddr + drvdata->size -
				    drvdata->buf;
				memcpy(ptr, drvdata->buf, buflen);
				ptr += buflen;
			}

			buflen = drvdata->size - buflen;
			memcpy(ptr, drvdata->vaddr, buflen);
			ptr += buflen;
		} else {
			memcpy(ptr, drvdata->buf, drvdata->size);
			ptr += drvdata->size;
		}

		dump_len += drvdata->size;
	}

	info_print("end \n");
	return 1;
}

static int __init tmc_dump_init(void)
{
	int ret = -1;
	struct device_node *np = NULL;
	np = of_find_compatible_node(NULL, NULL,
				     "hisilicon,rdr_ap_adapter");
	if (!np) {
		err_print("[%s], cannot find rdr_ap_adapter node in dts!\n",
			  __func__);
		return ret;
	}

	ret = of_property_read_u32(np, "ap_dump_mem_modu_tmc_size",
				   &ap_dump_mem_modu_tmc_size);
	if (ret) {
		err_print("[%s], cannot find ap_dump_mem_modu_tmc_size in dts!\n",
			  __func__);
		return ret;
	}

	if (rdr_get_ap_init_done()) {
		ret = register_module_dump_mem_func(tmc_dump, "TMC", MODU_TMC);
		if (ret) {
			err_print("%s: register tmc_dump failed \n", __func__);
		}
	}
	return ret;
}

late_initcall(tmc_dump_init);

#endif

/* get tmc drive data by device node's phandle */
void *get_etb_drvdata_bydevnode(struct device_node *np)
{
	int i;
	struct tmc_drvdata *drvdata = NULL;

	for (i = 0; i < tmc_count; i++) {
		drvdata = tmcdrvdata[i];
		if (!drvdata) {
			break;
		}
		if (np == drvdata->dev->of_node)
			return (void *)drvdata;
	}

	return NULL;
}

/* if the cluster already be powered down,
need restore etbetf config  when exit cpu idle */
int etbetf_restore(void *drv)
{
	struct tmc_drvdata *drvdata = NULL;
	u32 val;
	int ret = 0;

	drvdata = (struct tmc_drvdata *)drv;
	if (!drvdata) {
		printk("[%s]:NULL pointer \n", __func__);
		return -1;
	}
	spin_lock(&drvdata->spinlock);
	CS_UNLOCK(drvdata->base);

	val = readl_relaxed(drvdata->base + TMC_CTL);
	if (TMC_CTL_CAPT_EN == (val & TMC_CTL_CAPT_EN)) {
		ret = -1;
		goto out; /* etbetf is already running, so no need to restore */
	}

	if ((drvdata->config_type == TMC_CONFIG_TYPE_ETB) ||
	    (drvdata->config_type == TMC_CONFIG_TYPE_ETF &&
	     drvdata->mode == TMC_MODE_CIRCULAR_BUFFER)){
		/* etb */
		writel_relaxed(TMC_MODE_CIRCULAR_BUFFER, drvdata->base + TMC_MODE);
		writel_relaxed(TMC_FFCR_EN_FMT | TMC_FFCR_EN_TI |
			       TMC_FFCR_FON_FLIN | TMC_FFCR_FON_TRIG_EVT |
			       TMC_FFCR_TRIGON_TRIGIN, drvdata->base + TMC_FFCR);

		writel_relaxed(drvdata->trigger_cntr, drvdata->base + TMC_TRG);
		tmc_enable_hw(drvdata);
	} else if (drvdata->config_type == TMC_CONFIG_TYPE_ETF &&
	     drvdata->mode == TMC_MODE_HARDWARE_FIFO){
		/* etf */
		writel_relaxed(TMC_MODE_HARDWARE_FIFO, drvdata->base + TMC_MODE);
		writel_relaxed(TMC_FFCR_EN_FMT | TMC_FFCR_EN_TI,
			       drvdata->base + TMC_FFCR);
		writel_relaxed(0x0, drvdata->base + TMC_BUFWM);
		tmc_enable_hw(drvdata);
	} else {
		dev_err(drvdata->dev, "[%s]:Error TMC type=%d,mode=%d \n", __func__, (int)drvdata->config_type, drvdata->mode);
		ret = -1;
	}
out:
	CS_LOCK(drvdata->base);
	spin_unlock(&drvdata->spinlock);
	return ret;
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("CoreSight Trace Memory Controller driver");
