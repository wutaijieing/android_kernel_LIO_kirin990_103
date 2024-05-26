/*
 * coresight-stm.c
 *
 * CoreSight System Trace Macrocell driver
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
#include <asm/local.h>
#include <securec.h>
#include <linux/amba/bus.h>
#include <linux/bitmap.h>
#include <linux/clk.h>
#include <linux/coresight-stm.h>
#include <linux/coresight.h>
#include <linux/err.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#include <platform_include/basicplatform/linux/util.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <platform_include/basicplatform/linux/nve/nve_ap_kernel_interface.h>
#include <linux/of_address.h>
#include <linux/perf_event.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/stm.h>

#include <mntn_public_interface.h>
#include <soc_cs_stm_interface.h>

#include "coresight_priv.h"
#include "coresight_tsgen.h"

#define PR_LOG_TAG STM_TRACE_TAG
#define STMPRIVMASKR 0xe40
#define STMSPFEAT1R 0xea0
#define STMSPFEAT2R 0xea4
#define STMSPFEAT3R 0xea8

#define STM_32_CHANNEL 32
#define BYTES_PER_CHANNEL 256
#define STM_TRACE_BUF_SIZE 4096
#define STM_SW_MASTER_END 127

#define STM_SPMSCR 0xFFFFC000
#define STM_SPSCR 0x0FFF8000

/* Enable HW event tracing | Error detection on event tracing */
#define STM_HEMCR (0x01 | 0x04)

/* timestamp enable | global STM enable */
#define TIME_AND_GLB_EN (0x02 | 0x01)

/* traceid field is 7bit wide on STM */
#define TRACEID_WIDE 0x7f

#define STM_MEDIA_SET 0x800
/* Register bit definition */
#define STMTCSR_BUSY_BIT 23
/* Reserve the first 10 channels for kernel usage */
#define STM_CHANNEL_OFFSET 0

/* select bank 0 */
#define STM_STMHEBSR0 0

/* select bank 1 */
#define STM_STMHEBSR1 1

/*
 * NUMPS in STMDEVID is 17 bit long and if equal to 0x0,
 * 32 stimulus ports are supported.
 */
#define STM_NUMSP 0x1ffff

#define NUM_STM 2
static struct stm_drvdata *stmtracedrvdata[NUM_STM];
static unsigned int stm_trace_number = 0;
void __iomem *rst;

enum stm_pkt_type {
	STM_PKT_TYPE_DATA = 0x98,
	STM_PKT_TYPE_FLAG = 0xE8,
	STM_PKT_TYPE_TRIG = 0xF8,
};

#define stm_channel_addr(drvdata, ch)                                          \
	(drvdata->chs.base + (ch * BYTES_PER_CHANNEL))
#define stm_channel_off(type, opts) (type & ~opts)

static int boot_nr_channel;

/*
 * Not really modular but using module_param is the easiest way to
 * remain consistent with existing use cases for now.
 */
module_param_named(boot_nr_channel, boot_nr_channel, int, S_IRUGO);

/**
 * struct channel_space - central management entity for extended ports
 * @base:		memory mapped base address where channels start.
 * @guaraneed:		is the channel delivery guaranteed.
 */
struct channel_space {
	void __iomem *base;
	unsigned long *guaranteed;
};

/**
 * struct stm_drvdata - specifics associated to an STM component
 * @base:		memory mapped base address for this component.
 * @dev:		the device entity associated to this component.
 * @atclk:		optional clock for the core parts of the STM.
 * @csdev:		component vitals needed by the framework.
 * @spinlock:		only one at a time pls.
 * @chs:		the channels accociated to this STM.
 * @stm:		structure associated to the generic STM interface.
 * @mode:		this tracer's mode, i.e sysFS, or disabled.
 * @clockdep_supply:	related to power up if stm in media.
 * @useclockdep:		related to power up if stm in media.
 * @in_media:	Is this stm in media.
 * @enable:		   Is this noc trace currently tracing.
 * @boot_enable:   True if we should start tracing at boot time.
 * @traceid:		value of the current ID for this component.
 * @write_bytes:	Maximus bytes this STM can write at a time.
 * @stmsper:		settings for register STMSPER.
 * @stmspscr:		settings for register STMSPSCR.
 * @numsp:		the total number of stimulus port support by this STM.
 * @stmheer:		settings for register STMHEER.
 * @stmheter:		settings for register STMHETER.
 * @stmhebsr:		settings for register STMHEBSR.
 */
struct stm_drvdata {
	void __iomem *base;
	struct device *dev;
	struct clk *atclk;
	struct coresight_device *csdev;
	spinlock_t spinlock;
	struct channel_space chs;
	struct stm_data stm;
	local_t mode;
	struct regulator *clockdep_supply;
	unsigned int useclockdep;
	u32 in_media;
	bool enable;
	bool boot_enable;
	u8 traceid;
	u32 write_bytes;
	u32 stmsper;
	u32 stmspscr;
	u32 numsp;
	u32 stmheer;
	u32 stmheter;
	u32 stmhebsr;
};

static int stm_media_regulator_enable(struct stm_drvdata *drvdata)
{
	int ret;
	struct device *dev = drvdata->dev;
	struct device_node *np = drvdata->dev->of_node;

	/* power media1 up */
	drvdata->clockdep_supply = devm_regulator_get(dev, "stm-clockdep");
	if (IS_ERR_OR_NULL(drvdata->clockdep_supply)) {
		pr_err("[%s] Failed : stm-clockdep devm_regulator_get.%pK\n",
			__func__, drvdata->clockdep_supply);
		return -EINVAL;
	}

	if ((ret = of_property_read_u32(np, "useclockdep",
		     (unsigned int *)(&drvdata->useclockdep))) < 0) {
		pr_err("[%s] Failed: useclockdep of_property_read_u32.%d\n",
			__func__, ret);
		return -EINVAL;
	}

	dev_dbg(drvdata->dev, " useclockdep is 0x%x\n", drvdata->useclockdep);

	if (drvdata->useclockdep) {
		if ((ret = regulator_enable(drvdata->clockdep_supply)) != 0) {
			pr_err("[%s] Failed: clockdep regulator_enable.%d\n",
				__func__, ret);
			return ret;
		}
	}

	/* un reset */
	writel_relaxed(STM_MEDIA_SET, rst);

	return 0;
}

static void stm_hwevent_enable_hw(struct stm_drvdata *drvdata)
{
	CS_UNLOCK(drvdata->base);
	/* enable hardware event bank0 0~31 */
	writel_relaxed(STM_STMHEBSR0, SOC_CS_STM_STM_HEBSR_ADDR(drvdata->base));
	writel_relaxed(
		drvdata->stmheer, SOC_CS_STM_STM_HEER_ADDR(drvdata->base));
	writel_relaxed(STM_HEMCR, SOC_CS_STM_STM_HEMCR_ADDR(drvdata->base));

	/* enable hardware event bank1 32~63 */
	writel_relaxed(STM_STMHEBSR1, SOC_CS_STM_STM_HEBSR_ADDR(drvdata->base));
	writel_relaxed(
		drvdata->stmheer, SOC_CS_STM_STM_HEER_ADDR(drvdata->base));
	writel_relaxed(STM_HEMCR, SOC_CS_STM_STM_HEMCR_ADDR(drvdata->base));

	CS_LOCK(drvdata->base);
}

static void stm_port_enable_hw(struct stm_drvdata *drvdata)
{
	CS_UNLOCK(drvdata->base);

	writel_relaxed(
		drvdata->stmsper, SOC_CS_STM_STM_SPER_ADDR(drvdata->base));

	writel_relaxed(
		drvdata->stmspscr, SOC_CS_STM_STM_SPSCR_ADDR(drvdata->base));

	writel_relaxed(STM_SPMSCR, SOC_CS_STM_STM_SPMSCR_ADDR(drvdata->base));

	CS_LOCK(drvdata->base);
}

static void stm_enable_hw(struct stm_drvdata *drvdata)
{
	int ret;

	if (drvdata->in_media) {
		ret = stm_media_regulator_enable(drvdata);
		if (ret) {
			pr_err("[%s]: stm_media_regulator_enable fail !\n",
				__func__);
			return;
		}
	}

	if (drvdata->stmheer) {
		stm_hwevent_enable_hw(drvdata);
		pr_err("stm stm_hwevent_enable_hw\n");
	}

	stm_port_enable_hw(drvdata);

	CS_UNLOCK(drvdata->base);

	writel_relaxed(((drvdata->traceid << 16) | /* trace id */
			       TIME_AND_GLB_EN),
		SOC_CS_STM_STM_TCSR_ADDR(drvdata->base));

	CS_LOCK(drvdata->base);
}

static int stm_enable(struct coresight_device *csdev)
{
	struct stm_drvdata *drvdata = dev_get_drvdata(csdev->dev.parent);

	/* enabel timestamp */
	if (coresight_tsgen_enable())
		dev_err(drvdata->dev, "tsgen failed to enable\n");

	spin_lock(&drvdata->spinlock);
	stm_enable_hw(drvdata);
	drvdata->enable = true;
	spin_unlock(&drvdata->spinlock);

	dev_info(drvdata->dev, "STM tracing enabled\n");
	return 0;
}

static void stm_hwevent_disable_hw(struct stm_drvdata *drvdata)
{
	CS_UNLOCK(drvdata->base);

	writel_relaxed(0x0, SOC_CS_STM_STM_HEMCR_ADDR(drvdata->base));
	writel_relaxed(0x0, SOC_CS_STM_STM_HEER_ADDR(drvdata->base));
	writel_relaxed(0x0, SOC_CS_STM_STM_HETER_ADDR(drvdata->base));

	CS_LOCK(drvdata->base);
}

static void stm_port_disable_hw(struct stm_drvdata *drvdata)
{
	CS_UNLOCK(drvdata->base);

	writel_relaxed(0x0, SOC_CS_STM_STM_SPER_ADDR(drvdata->base));
	writel_relaxed(0x0, SOC_CS_STM_STM_SPTRIGCSR_ADDR(drvdata->base));

	CS_LOCK(drvdata->base);
}

static void stm_disable_hw(struct stm_drvdata *drvdata)
{
	u32 val;
	int ret;
	CS_UNLOCK(drvdata->base);

	val = readl_relaxed(SOC_CS_STM_STM_TCSR_ADDR(drvdata->base));
	val &= ~0x1; /* clear global STM enable [0] */
	writel_relaxed(val, SOC_CS_STM_STM_TCSR_ADDR(drvdata->base));

	CS_LOCK(drvdata->base);

	stm_port_disable_hw(drvdata);
	if (drvdata->stmheer)
		stm_hwevent_disable_hw(drvdata);

	/* power media1 down */
	if (drvdata->in_media) {
		pr_info("stm in media regulator_disable.\n");
		if ((ret = regulator_disable(drvdata->clockdep_supply)) != 0) {
			pr_err("[%s] Failed: clockdep regulator_disable.%d\n",
				__func__, ret);
			return;
		}
	}
}

static void stm_disable(struct coresight_device *csdev)
{
	int ret;
	struct stm_drvdata *drvdata = dev_get_drvdata(csdev->dev.parent);

	ret = coresight_tsgen_disable();
	if (ret)
		dev_err(drvdata->dev, "tsgen failed to disable\n");

	/*
	 * For as long as the tracer isn't disabled another entity can't
	 * change its status.  As such we can read the status here without
	 * fearing it will change under us.
	 */
	spin_lock(&drvdata->spinlock);
	stm_disable_hw(drvdata);
	drvdata->enable = false;
	spin_unlock(&drvdata->spinlock);

	/* Wait until the engine has completely stopped */
	coresight_timeout(
		drvdata, SOC_CS_STM_STM_TCSR_ADDR(0), STMTCSR_BUSY_BIT, 0);

	dev_info(drvdata->dev, "STM tracing disabled\n");
}

static int stm_trace_id(struct coresight_device *csdev)
{
	struct stm_drvdata *drvdata = dev_get_drvdata(csdev->dev.parent);

	return drvdata->traceid;
}

static const struct coresight_ops_source stm_source_ops = {
	.trace_id = stm_trace_id,
	.enable = stm_enable,
	.disable = stm_disable,
};

static const struct coresight_ops stm_cs_ops = {
	.source_ops = &stm_source_ops,
};

static inline bool stm_addr_unaligned(const void *addr, u8 write_bytes)
{
	return ((uintptr_t)addr & (write_bytes - 1));
}

static void stm_send(void *addr, const void *data, u32 size, u8 write_bytes)
{
	u8 paload[8];
	u32 paload_len = 8;
	int ret;

	if (stm_addr_unaligned(data, write_bytes)) {
		ret = memcpy_s(paload, paload_len, data, size);
		if(ret) {
			pr_err("[%s]: memcpy_s fail !\n", __func__);
			return;
		}
		data = paload;
	}

	/* now we are 64bit/32bit aligned */
	switch (size) {
#ifdef CONFIG_64BIT
	case 8:
		writeq_relaxed(*(u64 *)data, addr);
		break;
#endif
	case 4:
		writel_relaxed(*(u32 *)data, addr);
		break;
	case 2:
		writew_relaxed(*(u16 *)data, addr);
		break;
	case 1:
		writeb_relaxed(*(u8 *)data, addr);
		break;
	default:
		break;
	}
}

static int stm_generic_link(
	struct stm_data *stm_data, unsigned int master, unsigned int channel)
{
	struct stm_drvdata *drvdata =
		container_of(stm_data, struct stm_drvdata, stm);
	if (!drvdata || !drvdata->csdev)
		return -EINVAL;

	return coresight_enable(drvdata->csdev);
}

static void stm_generic_unlink(
	struct stm_data *stm_data, unsigned int master, unsigned int channel)
{
	struct stm_drvdata *drvdata =
		container_of(stm_data, struct stm_drvdata, stm);
	if (!drvdata || !drvdata->csdev)
		return;

	stm_disable(drvdata->csdev);
}

static long stm_generic_set_options(struct stm_data *stm_data,
	unsigned int master, unsigned int channel, unsigned int nr_chans,
	unsigned long options)
{
	struct stm_drvdata *drvdata =
		container_of(stm_data, struct stm_drvdata, stm);
	if (!(drvdata && local_read(&drvdata->mode)))
		return -EINVAL;

	if (channel >= drvdata->numsp)
		return -EINVAL;

	switch (options) {
	case STM_OPTION_GUARANTEED:
		set_bit(channel, drvdata->chs.guaranteed);
		break;

	case STM_OPTION_INVARIANT:
		clear_bit(channel, drvdata->chs.guaranteed);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static ssize_t stm_generic_packet(struct stm_data *stm_data,
	unsigned int master, unsigned int channel, unsigned int packet,
	unsigned int flags, unsigned int size, const unsigned char *payload)
{
	uintptr_t ch_addr;
	struct stm_drvdata *drvdata =
		container_of(stm_data, struct stm_drvdata, stm);

	if (!(drvdata && local_read(&drvdata->mode)))
		return 0;

	if (channel >= drvdata->numsp)
		return 0;

	ch_addr = (uintptr_t)stm_channel_addr(
		drvdata, channel); /*lint !e679*/

	flags = (flags == STP_PACKET_TIMESTAMPED) ? STM_FLAG_TIMESTAMPED : 0;
	flags |= test_bit(channel, drvdata->chs.guaranteed)
			 ? STM_FLAG_GUARANTEED
			 : 0;

	if (size > drvdata->write_bytes)
		size = drvdata->write_bytes;
	else
		size = rounddown_pow_of_two(size);

	switch (packet) {
	case STP_PACKET_FLAG:
		ch_addr |= stm_channel_off(STM_PKT_TYPE_FLAG, flags);

		/*
		 * The generic STM core sets a size of '0' on flag packets.
		 * As such send a flag packet of size '1' and tell the
		 * core we did so.
		 */
		stm_send((void *)ch_addr, payload, 1, drvdata->write_bytes);
		size = 1;
		break;

	case STP_PACKET_DATA:
		ch_addr |= stm_channel_off(STM_PKT_TYPE_DATA, flags);
		if (size == 0) {
			pr_err("[%s]: invalid size\n", __func__);
			return size;
		}
		stm_send((void *)ch_addr, payload, size, drvdata->write_bytes);
		break;

	default:
		return -ENOTSUPP;
	}

	return size;
}

static ssize_t hwevent_enable_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned long val;
	struct stm_drvdata *drvdata = dev_get_drvdata(dev->parent);

	spin_lock(&drvdata->spinlock);
	val = drvdata->stmheer;
	spin_unlock(&drvdata->spinlock);

	return scnprintf(buf, PAGE_SIZE, "%#lx\n", val);
}

static ssize_t hwevent_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct stm_drvdata *drvdata = dev_get_drvdata(dev->parent);
	unsigned long val = 0;
	int ret;

	if (NULL == buf) {
		pr_err("[%s]: buf is NULL!!!\n", __func__);
		return -EINVAL;
	}

	ret = kstrtoul(buf, 16, &val);
	if (ret)
		return -EINVAL;

	spin_lock(&drvdata->spinlock);
	drvdata->stmheer = val;
	/* HW event enable and trigger go hand in hand */
	drvdata->stmheter = val;
	spin_unlock(&drvdata->spinlock);
	return size;
}
static DEVICE_ATTR_RW(hwevent_enable);

static ssize_t hwevent_select_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned long val;
	struct stm_drvdata *drvdata = dev_get_drvdata(dev->parent);

	spin_lock(&drvdata->spinlock);
	val = drvdata->stmhebsr;
	spin_unlock(&drvdata->spinlock);

	return scnprintf(buf, PAGE_SIZE, "%#lx\n", val);
}

static ssize_t hwevent_select_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct stm_drvdata *drvdata = dev_get_drvdata(dev->parent);
	unsigned long val = 0;
	int ret;

	if (NULL == buf) {
		pr_err("[%s]: buf is NULL!!!\n", __func__);
		return -EINVAL;
	}

	ret = kstrtoul(buf, 16, &val);
	if (ret)
		return -EINVAL;

	spin_lock(&drvdata->spinlock);
	drvdata->stmhebsr = val;
	spin_unlock(&drvdata->spinlock);

	return size;
}
static DEVICE_ATTR_RW(hwevent_select);

static ssize_t port_select_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct stm_drvdata *drvdata = dev_get_drvdata(dev->parent);
	unsigned long val;

	if (!local_read(&drvdata->mode)) {
		val = drvdata->stmspscr;
	} else {
		spin_lock(&drvdata->spinlock);
		val = readl_relaxed(SOC_CS_STM_STM_SPSCR_ADDR(drvdata->base));
		spin_unlock(&drvdata->spinlock);
	}

	return scnprintf(buf, PAGE_SIZE, "%#lx\n", val);
}

static ssize_t port_select_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct stm_drvdata *drvdata = dev_get_drvdata(dev->parent);
	unsigned long val = 0;
	unsigned long stmsper;
	int ret;

	ret = kstrtoul(buf, 16, &val);
	if (ret)
		return ret;

	spin_lock(&drvdata->spinlock);
	drvdata->stmspscr = val;

	if (local_read(&drvdata->mode)) {
		CS_UNLOCK(drvdata->base);
		/* Process as per ARM's TRM recommendation */
		stmsper =
			readl_relaxed(SOC_CS_STM_STM_SPER_ADDR(drvdata->base));
		writel_relaxed(0x0, SOC_CS_STM_STM_SPER_ADDR(drvdata->base));
		writel_relaxed(drvdata->stmspscr,
			SOC_CS_STM_STM_SPSCR_ADDR(drvdata->base));
		writel_relaxed(
			stmsper, SOC_CS_STM_STM_SPER_ADDR(drvdata->base));
		CS_LOCK(drvdata->base);
	}
	spin_unlock(&drvdata->spinlock);

	return size;
}
static DEVICE_ATTR_RW(port_select);

static ssize_t port_enable_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	struct stm_drvdata *drvdata = dev_get_drvdata(dev->parent);
	unsigned long val;

	if (!local_read(&drvdata->mode)) {
		val = drvdata->stmsper;
	} else {
		spin_lock(&drvdata->spinlock);
		val = readl_relaxed(SOC_CS_STM_STM_SPER_ADDR(drvdata->base));
		spin_unlock(&drvdata->spinlock);
	}

	return scnprintf(buf, PAGE_SIZE, "%#lx\n", val);
}

static ssize_t port_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	struct stm_drvdata *drvdata = dev_get_drvdata(dev->parent);
	unsigned long val = 0;
	int ret;

	ret = kstrtoul(buf, 16, &val);
	if (ret)
		return ret;

	spin_lock(&drvdata->spinlock);
	drvdata->stmsper = val;

	if (local_read(&drvdata->mode)) {
		CS_UNLOCK(drvdata->base);
		writel_relaxed(drvdata->stmsper,
			SOC_CS_STM_STM_SPER_ADDR(drvdata->base));
		CS_LOCK(drvdata->base);
	}
	spin_unlock(&drvdata->spinlock);

	return size;
}
static DEVICE_ATTR_RW(port_enable);

static ssize_t traceid_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned long val;
	struct stm_drvdata *drvdata = dev_get_drvdata(dev->parent);

	spin_lock(&drvdata->spinlock);
	val = drvdata->traceid;
	spin_unlock(&drvdata->spinlock);
	return snprintf_s(buf, PAGE_SIZE, sizeof(unsigned long), "%#lx\n", val);
}

static ssize_t traceid_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t size)
{
	int ret;
	unsigned long val = 0;
	struct stm_drvdata *drvdata = dev_get_drvdata(dev->parent);

	if (NULL == buf) {
		pr_err("[%s]: buf is NULL!!!\n", __func__);
		return -EINVAL;
	}

	ret = kstrtoul(buf, 16, &val);
	if (ret)
		return ret;

	spin_lock(&drvdata->spinlock);
	/* traceid field is 7bit wide on STM32 */
	drvdata->traceid = val & TRACEID_WIDE;
	spin_unlock(&drvdata->spinlock);
	return size;
}
static DEVICE_ATTR_RW(traceid);
#define coresight_simple_func(type, name, offset)                              \
	static ssize_t name##_show(                                            \
		struct device *_dev, struct device_attribute *attr, char *buf) \
	{                                                                      \
		type *drvdata = dev_get_drvdata(_dev->parent);                 \
		return scnprintf(buf, PAGE_SIZE, "0x%x\n",                     \
			readl_relaxed(drvdata->base + offset));                \
	}                                                                      \
	static DEVICE_ATTR_RO(name)

#define coresight_stm_simple_func(name, offset)                                \
	coresight_simple_func(struct stm_drvdata, name, offset)

coresight_stm_simple_func(tcsr, SOC_CS_STM_STM_TCSR_ADDR(0));
coresight_stm_simple_func(tsfreqr, SOC_CS_STM_STM_TSFREQR_ADDR(0));
coresight_stm_simple_func(syncr, SOC_CS_STM_STM_SYNCR_ADDR(0));
coresight_stm_simple_func(sper, SOC_CS_STM_STM_SPER_ADDR(0));
coresight_stm_simple_func(spter, SOC_CS_STM_STM_SPTER_ADDR(0));
coresight_stm_simple_func(privmaskr, STMPRIVMASKR);
coresight_stm_simple_func(spscr, SOC_CS_STM_STM_SPSCR_ADDR(0));
coresight_stm_simple_func(spmscr, SOC_CS_STM_STM_SPMSCR_ADDR(0));
coresight_stm_simple_func(spfeat1r, STMSPFEAT1R);
coresight_stm_simple_func(spfeat2r, STMSPFEAT2R);
coresight_stm_simple_func(spfeat3r, STMSPFEAT3R);
coresight_stm_simple_func(devid, CORESIGHT_DEVID);

static struct attribute *coresight_stm_attrs[] = {
	&dev_attr_hwevent_enable.attr, &dev_attr_hwevent_select.attr,
	&dev_attr_port_enable.attr, &dev_attr_port_select.attr,
	&dev_attr_traceid.attr, NULL,
};

static struct attribute *coresight_stm_mgmt_attrs[] = {
	&dev_attr_tcsr.attr, &dev_attr_tsfreqr.attr, &dev_attr_syncr.attr,
	&dev_attr_sper.attr, &dev_attr_spter.attr, &dev_attr_privmaskr.attr,
	&dev_attr_spscr.attr, &dev_attr_spmscr.attr, &dev_attr_spfeat1r.attr,
	&dev_attr_spfeat2r.attr, &dev_attr_spfeat3r.attr, &dev_attr_devid.attr,
	NULL,
};

static const struct attribute_group coresight_stm_group = {
	.attrs = coresight_stm_attrs,
};

static const struct attribute_group coresight_stm_mgmt_group = {
	.attrs = coresight_stm_mgmt_attrs,
	.name = "mgmt",
};

static const struct attribute_group *coresight_stm_groups[] = {
	&coresight_stm_group, &coresight_stm_mgmt_group, NULL,
};

static int stm_get_resource_byname(
	struct device_node *np,const char *ch_base, struct resource *res)
{
	const char *name = NULL;
	int index = 0;
	int found = 0;

	while (!of_property_read_string_index(np, "reg-names", index, &name)) {
		if (strcmp(ch_base, name)) {
			index++;
			continue;
		}

		/* We have a match and @index is where it's at */
		found = 1;
		break;
	}

	if (!found) {
		pr_err("stm do not have stm-stimulus-base\n");
		return 1;
	}

	return of_address_to_resource(np, index, res);
}

static u32 stm_fundamental_data_size(struct stm_drvdata *drvdata)
{
	u32 stmspfeat2r;
	u32 ret;

	if (!IS_ENABLED(CONFIG_64BIT))
		return 4;

	stmspfeat2r = readl_relaxed(drvdata->base + STMSPFEAT2R);

	/*
	 * bit[15:12] represents the fundamental data size
	 * 0 - 32-bit data
	 * 1 - 64-bit data
	 */
	ret = BMVAL(stmspfeat2r, 12, 15) ? 8 : 4; /*lint !e648*/
	return ret;
}

static u32 stm_num_stimulus_port(struct stm_drvdata *drvdata)
{
	u32 numsp;

	numsp = readl_relaxed(drvdata->base + CORESIGHT_DEVID);
	/*
	 * NUMPS in STMDEVID is 17 bit long and if equal to 0x0,
	 * 32 stimulus ports are supported.
	 */
	numsp &= STM_NUMSP;
	if (!numsp)
		numsp = STM_32_CHANNEL;
	return numsp;
}

static void stm_init_default_data(struct stm_drvdata *drvdata)
{
	spin_lock(&drvdata->spinlock);
	/* select all ports 0xe60 */
	drvdata->stmspscr = STM_SPSCR;
	/*
	 * Enable all channel regardless of their number.  When port
	 * selection isn't used (see above) STMSPER applies to all
	 * 32 channel group available, hence setting all 32 bits to 1
	 * 0xe00
	 */
	drvdata->stmsper = ~0x0;

	/* enable all hardware event 0xD00 */
	drvdata->stmheer = 0x0;
	spin_unlock(&drvdata->spinlock);
	/* Set invariant transaction timing on all channels */
	bitmap_clear(drvdata->chs.guaranteed, 0, drvdata->numsp);
}

static void stm_init_generic_data(struct stm_drvdata *drvdata)
{
	drvdata->stm.name = dev_name(drvdata->dev);

	/*
	 * MasterIDs are assigned at HW design phase. As such the core is
	 * using a single master for interaction with this device.
	 */
	drvdata->stm.sw_start = 1;
	drvdata->stm.sw_end = 1;
	drvdata->stm.hw_override = true;
	drvdata->stm.sw_nchannels = drvdata->numsp;
	drvdata->stm.packet = stm_generic_packet;
	drvdata->stm.link = stm_generic_link;
	drvdata->stm.unlink = stm_generic_unlink;
	drvdata->stm.set_options = stm_generic_set_options;
}

/*
 * Disable all  STM_trace.
 * It should be called when fetal error happens or system reset.
 */
void stm_trace_disable_all(void)
{
	unsigned int i = 0;

	struct stm_drvdata *drvdata = NULL;
	for (; i < stm_trace_number; i++) {
		drvdata = stmtracedrvdata[i];
		if (!drvdata)
			continue;
		if (drvdata->enable) {
			drvdata->enable = false;
			spin_lock(&drvdata->spinlock);
			stm_disable_hw(drvdata);
			spin_unlock(&drvdata->spinlock);
			dev_info(drvdata->dev, "disabled\n");
			coresight_refresh_path(drvdata->csdev, 0);
		}
	}
	info_print("stm_trace_disable_all done\n");
}
EXPORT_SYMBOL_GPL(stm_trace_disable_all);

static int get_property_form_dtsi(
	struct stm_drvdata *drvdata, struct device_node *np)
{
	unsigned int stm_set_reg = 0;
	int ret;

	spin_lock(&drvdata->spinlock);
	if ((ret = of_property_read_u32(np, "trace_id",
		     (unsigned int *)(&drvdata->traceid))) < 0) {
		pr_err("[%s] Failed: drvdata->traceid "
		       "of_property_read_u32.%d\n",
			__func__, ret);
		spin_unlock(&drvdata->spinlock);
		return ret;
	}

	if ((ret = of_property_read_u32(np, "in_media",
		     (unsigned int *)(&drvdata->in_media))) < 0) {
		pr_err("[%s] Failed: in_media of_property_read_u32.%d\n",
			__func__, ret);
		spin_unlock(&drvdata->spinlock);
		return ret;
	}

	if (drvdata->in_media) {
		if ((ret = of_property_read_u32(np, "set_reg",
			     (unsigned int *)(&stm_set_reg))) < 0) {
			pr_err("[%s] Failed: set_reg of_property_read_u32 %d\n",
				__func__, ret);
			spin_unlock(&drvdata->spinlock);
			return ret;
		}
		dev_dbg(drvdata->dev, " set_reg is 0x%x\n", stm_set_reg);

		rst = ioremap(stm_set_reg, 0x4);

		ret = stm_media_regulator_enable(drvdata);
		if (ret) {
			pr_err("[%s] Failed: stm_media_regulator_enable %d\n",
				__func__, ret);
			spin_unlock(&drvdata->spinlock);
			return ret;
		}
	}
	spin_unlock(&drvdata->spinlock);
	return 0;
}

/* if the stm not in media, get stimulate port property */
static int get_stimulate_port_property(struct stm_drvdata *drvdata,
	struct device_node *np, struct resource *res, unsigned long *guaranteed)
{
	int ret;
	void __iomem *base = NULL;
	struct resource ch_res;
	size_t bitmap_size;

	spin_lock(&drvdata->spinlock);

	if (!drvdata->in_media) {
		ret = stm_get_resource_byname(np, "stm-stimulus-base", &ch_res);
		if (ret) {
			pr_err("[%s]: stm_get_resource_byname fail !\n",
				__func__);
			spin_unlock(&drvdata->spinlock);
			return ret;
		}

		if (!ret) {
			base = devm_ioremap_resource(drvdata->dev, &ch_res);
			if (IS_ERR_OR_NULL(base)) {
				pr_err("[%s]: devm_ioremap_resource fail !\n",
					__func__);
				ret = -EINVAL;
				spin_unlock(&drvdata->spinlock);
				return ret;
			}
			drvdata->chs.base = base;
		}

		drvdata->write_bytes = stm_fundamental_data_size(drvdata);

		if (boot_nr_channel)
			drvdata->numsp = (unsigned int)boot_nr_channel;
		else
			drvdata->numsp = stm_num_stimulus_port(drvdata);
		bitmap_size = BITS_TO_LONGS(drvdata->numsp) * sizeof(long);

		guaranteed =
			devm_kzalloc(drvdata->dev, bitmap_size, GFP_KERNEL);
		if (IS_ERR_OR_NULL(guaranteed)) {
			pr_err("[%s]: guaranteed get memory fail !\n",
				__func__);
			ret = -ENOMEM;
			spin_unlock(&drvdata->spinlock);
			return ret;
		}
		drvdata->chs.guaranteed = guaranteed;
	}

	spin_unlock(&drvdata->spinlock);
	return 0;
}

static int stm_get_property(struct stm_drvdata *drvdata, struct device_node *np,
	struct resource *res, unsigned long *guaranteed)
{
	int ret;

	if (stm_trace_number < NUM_STM)
		stmtracedrvdata[stm_trace_number++] = drvdata;

	ret = get_property_form_dtsi(drvdata, np);
	if (ret) {
		pr_err("[%s] get_property_form_dtsi fail !\n", __func__);
		return ret;
	}

	/* if the stm not in media, get stimulate port property */
	ret = get_stimulate_port_property(drvdata, np, res, guaranteed);
	if (ret) {
		pr_err("[%s] get_stimulate_port_property fail !\n", __func__);
		return ret;
	}
	return 0;
}

/*lint -e429*/
static int stm_probe(struct amba_device *adev, const struct amba_id *id)
{
	int ret;
	void __iomem *base = NULL;
	unsigned long *guaranteed = NULL;
	struct device *dev = &adev->dev;
	struct coresight_platform_data *pdata = NULL;
	struct stm_drvdata *drvdata = NULL;
	struct resource *res = &adev->res;

	struct coresight_desc *desc = NULL;
	struct device_node *np = adev->dev.of_node;

	if (!check_mntn_switch(MNTN_STM_TRACE)) {
		pr_err("[%s] MNTN_STM_TRACE is closed!\n", __func__);
		ret = -EACCES;
		goto err;
	}

	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (IS_ERR_OR_NULL(drvdata)) {
		dev_err(dev, "cannot get memory\n");
		ret = -ENOMEM;
		goto err;
	}

	if (np) {
		pdata = of_get_coresight_platform_data(dev, np);
		if (IS_ERR_OR_NULL(pdata)) {
			dev_err(dev, "cannot get memory\n");
			ret = -ENOMEM;
			goto err_next;
		}
		adev->dev.platform_data = pdata;
		drvdata->boot_enable =
			of_property_read_bool(np, "default_enable");
		dev_info(dev, "STM boot_init =%d\n", drvdata->boot_enable);
	}

	drvdata->dev = &adev->dev;
	drvdata->atclk = devm_clk_get(&adev->dev, "atclk"); /* optional */
	if (!IS_ERR(drvdata->atclk)) {
		ret = clk_prepare_enable(drvdata->atclk);
		if (ret) {
			dev_err(drvdata->dev, "enable atclk fail \n");
			goto err_next;
		}
	}

	dev_set_drvdata(dev, drvdata);
	spin_lock_init(&drvdata->spinlock);

	base = devm_ioremap_resource(dev, res);
	if (IS_ERR_OR_NULL(base)) {
		ret = -EINVAL;
		goto err_next;
	}
	drvdata->base = base;

	ret = stm_get_property(drvdata, np, res, guaranteed);
	if (ret) {
		pr_err("[%s] get_stimulate_port_property fail !\n", __func__);
		goto err_next;
	}

	stm_init_default_data(drvdata);
	stm_init_generic_data(drvdata);

	desc = devm_kzalloc(dev, sizeof(*desc), GFP_KERNEL);
	if (IS_ERR_OR_NULL(desc)) {
		ret = -ENOMEM;
		pr_err("[%s] coresight_desc get memory failed %d\n", __func__,
			ret);
		goto err_alloc_guaranteed;
	}

	if (!pdata) {
		ret = -ENODEV;
		goto err_alloc_guaranteed;
	}

	desc->type = CORESIGHT_DEV_TYPE_SOURCE;
	desc->subtype.source_subtype = CORESIGHT_DEV_SUBTYPE_SOURCE_SOFTWARE;
	desc->ops = &stm_cs_ops;
	desc->pdata = pdata;
	desc->dev = dev;
	desc->groups = coresight_stm_groups;
	drvdata->csdev = coresight_register(desc);
	if (IS_ERR_OR_NULL(drvdata->csdev)) {
		pr_err("[%s] coresight_register failed %d\n", __func__, ret);
		ret = PTR_ERR(drvdata->csdev);
		goto err_coresight_register;
	}

	/* if the stm in media, the regulator_disable need to be used after
	 * regulator_enable */
	if (drvdata->in_media) {
		pr_info("stm in media regulator_disable.\n");
		if ((ret = regulator_disable(drvdata->clockdep_supply)) != 0) {
			pr_err("[%s] Failed: clockdep regulator_disable.%d\n",
				__func__, ret);
			goto err_coresight_register;
		}
	}

	if (drvdata->boot_enable)
		coresight_enable(drvdata->csdev);

	dev_info(dev, "%s initialized\n", (char *)id->data);

	return 0; /*lint !e593 !e429*/

err_coresight_register:
	devm_kfree(dev, desc);

err_alloc_guaranteed:
	if (!drvdata->in_media)
		devm_kfree(dev, guaranteed); /*lint !e644 !e668*/

err_next:
	devm_kfree(dev, drvdata);
err:
	return ret; /*lint !e593*/
}
/*lint +e429*/

#ifdef CONFIG_PM
static int stm_runtime_suspend(struct device *dev)
{
	int ret = 0;
	struct stm_drvdata *drvdata = dev_get_drvdata(dev);

	dev_info(drvdata->dev, "%s: +\n", __func__);
	if (drvdata->enable)
		coresight_disable(drvdata->csdev);

	dev_info(drvdata->dev, "%s: -\n", __func__);

	return ret;
}

static int stm_runtime_resume(struct device *dev)
{
	int ret = 0;
	struct stm_drvdata *drvdata = dev_get_drvdata(dev);

	dev_info(drvdata->dev, "%s: +\n", __func__);
	if (!drvdata->enable)
		coresight_enable(drvdata->csdev);

	dev_info(drvdata->dev, "%s: -\n", __func__);

	return ret;
}

static SIMPLE_DEV_PM_OPS(
	stm_dev_pm_ops, stm_runtime_suspend, stm_runtime_resume);
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static void stm_remove(struct amba_device *adev)
{
	struct stm_drvdata *drvdata = amba_get_drvdata(adev);

	coresight_unregister(drvdata->csdev);

	dev_info(drvdata->dev, "%s: coresight unregister done. \n", __func__);
}
#else
static int stm_remove(struct amba_device *adev)
{
	struct stm_drvdata *drvdata = amba_get_drvdata(adev);

	coresight_unregister(drvdata->csdev);

	dev_info(drvdata->dev, "%s: coresight unregister done. \n", __func__);

	return 0;
}
#endif

static struct amba_id stm_ids[] = {
	{
		.id = 0x000bb963,
		.mask = 0x000fffff,
		.data = "STM500",
	},
	{0, 0},
};

static struct amba_driver stm_driver = {
	.drv =
		{
			.name = "coresight-stm",
			.owner = THIS_MODULE,
#ifdef CONFIG_PM
			.pm = &stm_dev_pm_ops,
#endif
			.suppress_bind_attrs = true,
		},
	.probe = stm_probe,
	.remove = stm_remove,
	.id_table = stm_ids,
};

builtin_amba_driver(stm_driver);
