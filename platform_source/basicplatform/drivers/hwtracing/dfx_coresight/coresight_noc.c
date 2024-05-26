/*
 * coresight-noc.c
 *
 * CoreSight NOC Trace driver
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
#include <linux/bitmap.h>
#include <linux/clk.h>
#include <linux/coresight.h>
#include <linux/device.h>
#include <linux/err.h>
#include <platform_include/basicplatform/linux/util.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <platform_include/basicplatform/linux/nve/nve_ap_kernel_interface.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/perf_event.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/topology.h>

#include <securec.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#include <mntn_public_interface.h>
#include <soc_cfg_sys_noc_bus_interface.h>

#include "coresight_noc.h"
#include "coresight_tsgen.h"

#define PR_LOG_TAG NOC_TRACE_TAG
#define PARAM_NUM_THREE 3
#define PARAM_NUM_TWO 2
#define FILTER_NUM 3

static struct noc_trace_drvdata *noctracedrvdata[NUM_NOC];
static unsigned int noc_trace_number = 0;

static void noc_trace_enable_hw(struct noc_trace_drvdata *drvdata)
{
	u32 i;
	/* close all noc probe */
	writel_relaxed(CFGCTRL_DISABLE,
		(drvdata->base + drvdata->disable_cfg_offset->cfgctrl_offset));
	writel_relaxed(STPV2EN_DISABLE,
		(drvdata->base + drvdata->disable_cfg_offset->stpv2en_offset));
	writel_relaxed(ATBEN_DISABLE,
		(drvdata->base + drvdata->disable_cfg_offset->atben_offset));

	/* init and clear all interupt */
	writel_relaxed(TRACE_ALARMCLR,
		(drvdata->base +
			drvdata->disable_cfg_offset->tracealarmclr_offset));

	/* init probe filter */
	for (i = 0; (i < drvdata->filter_nums) && (i < FILTER_NUM_MAX); i++) {
		writel_relaxed(drvdata->filter_cfg[i]->routeidbase,
			(drvdata->base +
				drvdata->filter_cfg_offset[i]
					->routeidbase_offset));
		writel_relaxed(drvdata->filter_cfg[i]->routeidmask,
			(drvdata->base +
				drvdata->filter_cfg_offset[i]
					->routeidmask_offset));
		writel_relaxed(drvdata->filter_cfg[i]->addrbase_low,
			(drvdata->base +
				drvdata->filter_cfg_offset[i]
					->addrbase_low_offset));
		writel_relaxed(drvdata->filter_cfg[i]->addrbase_high,
			(drvdata->base +
				drvdata->filter_cfg_offset[i]
					->addrbase_high_offset));
		writel_relaxed(drvdata->filter_cfg[i]->windowsize,
			(drvdata->base +
				drvdata->filter_cfg_offset[i]
					->windowsize_offset));
		writel_relaxed(
			SECURITY_BASE, (drvdata->base +
					       drvdata->filter_cfg_offset[i]
						       ->securitybase_offset));
		writel_relaxed(
			SECURITY_MASK, (drvdata->base +
					       drvdata->filter_cfg_offset[i]
						       ->securitymask_offset));
		writel_relaxed(drvdata->filter_cfg[i]->opcode,
			(drvdata->base +
				drvdata->filter_cfg_offset[i]->opcode_offset));
		writel_relaxed(NOC_TRACE_STATUS,
			(drvdata->base +
				drvdata->filter_cfg_offset[i]->status_offset));
		writel_relaxed(NOC_TRACE_LENGTH,
			(drvdata->base +
				drvdata->filter_cfg_offset[i]->length_offset));
		writel_relaxed(URGENCY,
			(drvdata->base +
				drvdata->filter_cfg_offset[i]->urgency_offset));
		writel_relaxed(USER_BASE, (drvdata->base +
						  drvdata->filter_cfg_offset[i]
							  ->userbase_offset));
		writel_relaxed(USER_MASK, (drvdata->base +
						  drvdata->filter_cfg_offset[i]
							  ->usermask_offset));
	}

	writel_relaxed(drvdata->filterlut,
		(drvdata->base + drvdata->enable_cfg_offset->filterlut_offset));

	/* enable stb interface */
	writel_relaxed(drvdata->trace_id,
		(drvdata->base + drvdata->enable_cfg_offset->atbid_offset));
	writel_relaxed(ATBEN_ENABLE,
		(drvdata->base + drvdata->enable_cfg_offset->atben_offset));

	/* enable stpv2 */
	writel_relaxed(ASYNC_PERIOD_ENABLE,
		(drvdata->base +
			drvdata->enable_cfg_offset->asyncperiod_offset));
	writel_relaxed(STPV2EN_ENABLE,
		(drvdata->base + drvdata->enable_cfg_offset->stpv2en_offset));

	/* enable Packet Probe */
	writel_relaxed(MAINCTL_ENABLE,
		(drvdata->base + drvdata->enable_cfg_offset->mainctl_offset));
	writel_relaxed(CFGCTL_ENABLE,
		(drvdata->base + drvdata->enable_cfg_offset->cfgctrl_offset));
	info_print("noc trace enabled!!\n");

	return;
}

static int noc_trace_enable(struct coresight_device *csdev)
{
	struct noc_trace_drvdata *drvdata = NULL;

	if (NULL == csdev) {
		pr_err("[%s]: csdev is NULL!!!\n", __func__);
		return -EINVAL;
	}

	drvdata = dev_get_drvdata(csdev->dev.parent);
	if (NULL == drvdata) {
		pr_err("[%s]: drvdata is NULL!!!\n", __func__);
		return -EINVAL;
	}

	/* enabel timestamp */
	if (coresight_tsgen_enable())
		dev_err(drvdata->dev, "tsgen failed to enable\n");

	spin_lock(&drvdata->spinlock);
	noc_trace_enable_hw(drvdata);
	drvdata->enable = true;
	spin_unlock(&drvdata->spinlock);

	dev_info(drvdata->dev, "noc tracing enabled\n");
	return 0;
}

static void noc_trace_disable_hw(struct noc_trace_drvdata *drvdata)
{
	writel_relaxed(CFGCTRL_DISABLE,
		(drvdata->base + drvdata->disable_cfg_offset->cfgctrl_offset));
	writel_relaxed(STPV2EN_DISABLE,
		(drvdata->base + drvdata->disable_cfg_offset->stpv2en_offset));
	writel_relaxed(ATBEN_DISABLE,
		(drvdata->base + drvdata->disable_cfg_offset->atben_offset));
}

static void noc_trace_disable(struct coresight_device *csdev)
{
	int ret;
	struct noc_trace_drvdata *drvdata = NULL;

	if (NULL == csdev) {
		pr_err("[%s]: csdev is NULL!!!\n", __func__);
		return;
	}

	drvdata = dev_get_drvdata(csdev->dev.parent);
	if (NULL == drvdata) {
		pr_err("[%s]: drvdata is NULL!!!\n", __func__);
		return;
	}

	ret = coresight_tsgen_disable();
	if (ret)
		dev_err(drvdata->dev, "tsgen failed to disable\n");

	spin_lock(&drvdata->spinlock);
	noc_trace_disable_hw(drvdata);
	drvdata->enable = false;
	spin_unlock(&drvdata->spinlock);

	dev_info(drvdata->dev, "noc tracing disabled\n");
}

static int noc_trace_trace_id(struct coresight_device *csdev)
{
	struct noc_trace_drvdata *drvdata = NULL;

	if (NULL == csdev) {
		pr_err("[%s]: csdev is NULL!!!\n", __func__);
		return -EINVAL;
	}

	drvdata = dev_get_drvdata(csdev->dev.parent);
	if (NULL == drvdata) {
		pr_err("[%s]: drvdata is NULL!!!\n", __func__);
		return -EINVAL;
	}

	return drvdata->trace_id;
}

static const struct coresight_ops_source noc_trace_source_ops = {
	.trace_id = noc_trace_trace_id,
	.enable = noc_trace_enable,
	.disable = noc_trace_disable,
};

static const struct coresight_ops noc_trace_cs_ops = {
	.source_ops = &noc_trace_source_ops,
};

/* the funtion show and store blow is used to show and change the value
 * respectly */
static ssize_t routeid_base_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned long val1, val2, val3;
	struct noc_trace_drvdata *drvdata = dev_get_drvdata(dev->parent);

	spin_lock(&drvdata->spinlock);
	if (drvdata->filter_nums == FILTER_NUM) {
		val1 = drvdata->filter_cfg[0]->routeidbase;
		val2 = drvdata->filter_cfg[1]->routeidbase;
		val3 = drvdata->filter_cfg[2]->routeidbase;
		spin_unlock(&drvdata->spinlock);
		return scnprintf(
			buf, PAGE_SIZE, "%#lx %#lx %#lx\n", val1, val2, val3);
	} else {
		val1 = drvdata->filter_cfg[0]->routeidbase;
		val2 = drvdata->filter_cfg[1]->routeidbase;
		spin_unlock(&drvdata->spinlock);
		return scnprintf(buf, PAGE_SIZE, "%#lx %#lx\n", val1, val2);
	}
}

static ssize_t routeid_base_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned long val1, val2, val3;
	struct noc_trace_drvdata *drvdata = dev_get_drvdata(dev->parent);

	spin_lock(&drvdata->spinlock);
	if (drvdata->filter_nums == FILTER_NUM) {
		if (sscanf_s(buf, "%lx %lx %lx", &val1, &val2, &val3) !=
			PARAM_NUM_THREE) {
			spin_unlock(&drvdata->spinlock);
			return -EINVAL;
		}

		drvdata->filter_cfg[0]->routeidbase = val1;
		drvdata->filter_cfg[1]->routeidbase = val2;
		drvdata->filter_cfg[2]->routeidbase = val3;
	} else {
		if (sscanf_s(buf, "%lx %lx", &val1, &val2) != PARAM_NUM_TWO) {
			spin_unlock(&drvdata->spinlock);
			return -EINVAL;
		}
		drvdata->filter_cfg[0]->routeidbase = val1;
		drvdata->filter_cfg[1]->routeidbase = val2;
	}
	spin_unlock(&drvdata->spinlock);
	return size;
}
static DEVICE_ATTR_RW(routeid_base);

static ssize_t routeid_mask_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned long val1, val2, val3;
	struct noc_trace_drvdata *drvdata = dev_get_drvdata(dev->parent);

	spin_lock(&drvdata->spinlock);
	if (drvdata->filter_nums == FILTER_NUM) {
		val1 = drvdata->filter_cfg[0]->routeidmask;
		val2 = drvdata->filter_cfg[1]->routeidmask;
		val3 = drvdata->filter_cfg[2]->routeidmask;
		spin_unlock(&drvdata->spinlock);
		return scnprintf(
			buf, PAGE_SIZE, "%#lx %#lx %#lx\n", val1, val2, val3);
	} else {
		val1 = drvdata->filter_cfg[0]->routeidmask;
		val2 = drvdata->filter_cfg[1]->routeidmask;
		spin_unlock(&drvdata->spinlock);
		return scnprintf(buf, PAGE_SIZE, "%#lx %#lx\n", val1, val2);
	}
}

static ssize_t routeid_mask_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned long val1, val2, val3;
	struct noc_trace_drvdata *drvdata = dev_get_drvdata(dev->parent);

	spin_lock(&drvdata->spinlock);
	if (drvdata->filter_nums == FILTER_NUM) {
		if (sscanf_s(buf, "%lx %lx %lx", &val1, &val2, &val3) !=
			PARAM_NUM_THREE) {
			spin_unlock(&drvdata->spinlock);
			return -EINVAL;
		}

		drvdata->filter_cfg[0]->routeidmask = val1;
		drvdata->filter_cfg[1]->routeidmask = val2;
		drvdata->filter_cfg[2]->routeidmask = val3;
	} else {
		if (sscanf_s(buf, "%lx %lx", &val1, &val2) != PARAM_NUM_TWO) {
			spin_unlock(&drvdata->spinlock);
			return -EINVAL;
		}
		drvdata->filter_cfg[0]->routeidmask = val1;
		drvdata->filter_cfg[1]->routeidmask = val2;
	}
	spin_unlock(&drvdata->spinlock);
	return size;
}
static DEVICE_ATTR_RW(routeid_mask);

static ssize_t addrbase_low_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned long val1, val2, val3;
	struct noc_trace_drvdata *drvdata = dev_get_drvdata(dev->parent);

	spin_lock(&drvdata->spinlock);
	if (drvdata->filter_nums == FILTER_NUM) {
		val1 = drvdata->filter_cfg[0]->addrbase_low;
		val2 = drvdata->filter_cfg[1]->addrbase_low;
		val3 = drvdata->filter_cfg[2]->addrbase_low;
		spin_unlock(&drvdata->spinlock);
		return scnprintf(
			buf, PAGE_SIZE, "%#lx %#lx %#lx\n", val1, val2, val3);
	} else {
		val1 = drvdata->filter_cfg[0]->addrbase_low;
		val2 = drvdata->filter_cfg[1]->addrbase_low;
		spin_unlock(&drvdata->spinlock);
		return scnprintf(buf, PAGE_SIZE, "%#lx %#lx\n", val1, val2);
	}
}

static ssize_t addrbase_low_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned long val1, val2, val3;
	struct noc_trace_drvdata *drvdata = dev_get_drvdata(dev->parent);

	spin_lock(&drvdata->spinlock);
	if (drvdata->filter_nums == FILTER_NUM) {
		if (sscanf_s(buf, "%lx %lx %lx", &val1, &val2, &val3) !=
			PARAM_NUM_THREE) {
			spin_unlock(&drvdata->spinlock);
			return -EINVAL;
		}
		drvdata->filter_cfg[0]->addrbase_low = val1;
		drvdata->filter_cfg[1]->addrbase_low = val2;
		drvdata->filter_cfg[2]->addrbase_low = val3;
	} else {
		if (sscanf_s(buf, "%lx %lx", &val1, &val2) != PARAM_NUM_TWO) {
			spin_unlock(&drvdata->spinlock);
			return -EINVAL;
		}
		drvdata->filter_cfg[0]->addrbase_low = val1;
		drvdata->filter_cfg[1]->addrbase_low = val2;
	}
	spin_unlock(&drvdata->spinlock);
	return size;
}
static DEVICE_ATTR_RW(addrbase_low);

static ssize_t addrbase_high_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned long val1, val2, val3;
	struct noc_trace_drvdata *drvdata = dev_get_drvdata(dev->parent);

	spin_lock(&drvdata->spinlock);
	if (drvdata->filter_nums == FILTER_NUM) {
		val1 = drvdata->filter_cfg[0]->addrbase_high;
		val2 = drvdata->filter_cfg[1]->addrbase_high;
		val3 = drvdata->filter_cfg[2]->addrbase_high;
		spin_unlock(&drvdata->spinlock);
		return scnprintf(
			buf, PAGE_SIZE, "%#lx %#lx %#lx\n", val1, val2, val3);
	} else {
		val1 = drvdata->filter_cfg[0]->addrbase_high;
		val2 = drvdata->filter_cfg[1]->addrbase_high;
		spin_unlock(&drvdata->spinlock);
		return scnprintf(buf, PAGE_SIZE, "%#lx %#lx\n", val1, val2);
	}
}

static ssize_t addrbase_high_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned long val1, val2, val3;
	struct noc_trace_drvdata *drvdata = dev_get_drvdata(dev->parent);

	spin_lock(&drvdata->spinlock);
	if (drvdata->filter_nums == FILTER_NUM) {
		if (sscanf_s(buf, "%lx %lx %lx", &val1, &val2, &val3) !=
			PARAM_NUM_THREE) {
			spin_unlock(&drvdata->spinlock);
			return -EINVAL;
		}
		drvdata->filter_cfg[0]->addrbase_high = val1;
		drvdata->filter_cfg[1]->addrbase_high = val2;
		drvdata->filter_cfg[2]->addrbase_high = val3;
	} else {
		if (sscanf_s(buf, "%lx %lx", &val1, &val2) != PARAM_NUM_TWO) {
			spin_unlock(&drvdata->spinlock);
			return -EINVAL;
		}
		drvdata->filter_cfg[0]->addrbase_high = val1;
		drvdata->filter_cfg[1]->addrbase_high = val2;
	}
	spin_unlock(&drvdata->spinlock);
	return size;
}
static DEVICE_ATTR_RW(addrbase_high);

/* window size is the range of address to trace */
static ssize_t windowsize_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned long val1, val2, val3;
	struct noc_trace_drvdata *drvdata = dev_get_drvdata(dev->parent);
	spin_lock(&drvdata->spinlock);
	if (drvdata->filter_nums == FILTER_NUM) {
		val1 = drvdata->filter_cfg[0]->windowsize;
		val2 = drvdata->filter_cfg[1]->windowsize;
		val3 = drvdata->filter_cfg[2]->windowsize;
		spin_unlock(&drvdata->spinlock);
		return scnprintf(
			buf, PAGE_SIZE, "%#lx %#lx %#lx\n", val1, val2, val3);
	} else {
		val1 = drvdata->filter_cfg[0]->windowsize;
		val2 = drvdata->filter_cfg[1]->windowsize;
		spin_unlock(&drvdata->spinlock);
		return scnprintf(buf, PAGE_SIZE, "%#lx %#lx\n", val1, val2);
	}
}

static ssize_t windowsize_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	unsigned long val1, val2, val3;
	struct noc_trace_drvdata *drvdata = dev_get_drvdata(dev->parent);

	spin_lock(&drvdata->spinlock);
	if (drvdata->filter_nums == FILTER_NUM) {
		if (sscanf_s(buf, "%lx %lx %lx", &val1, &val2, &val3) !=
			PARAM_NUM_THREE) {
			spin_unlock(&drvdata->spinlock);
			return -EINVAL;
		}
		drvdata->filter_cfg[0]->windowsize = val1;
		drvdata->filter_cfg[1]->windowsize = val2;
		drvdata->filter_cfg[2]->windowsize = val3;
	} else {
		if (sscanf_s(buf, "%lx %lx", &val1, &val2) != PARAM_NUM_TWO) {
			spin_unlock(&drvdata->spinlock);
			return -EINVAL;
		}
		drvdata->filter_cfg[0]->windowsize = val1;
		drvdata->filter_cfg[1]->windowsize = val2;
	}
	spin_unlock(&drvdata->spinlock);

	return size;
}
static DEVICE_ATTR_RW(windowsize);

/* opcode is read or writer */
static ssize_t opcode_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned long val1, val2, val3;
	struct noc_trace_drvdata *drvdata = dev_get_drvdata(dev->parent);

	spin_lock(&drvdata->spinlock);
	if (drvdata->filter_nums == FILTER_NUM) {
		val1 = drvdata->filter_cfg[0]->opcode;
		val2 = drvdata->filter_cfg[1]->opcode;
		val3 = drvdata->filter_cfg[2]->opcode;
		spin_unlock(&drvdata->spinlock);
		return scnprintf(
			buf, PAGE_SIZE, "%#lx %#lx %#lx\n", val1, val2, val3);
	} else {
		val1 = drvdata->filter_cfg[0]->opcode;
		val2 = drvdata->filter_cfg[1]->opcode;
		spin_unlock(&drvdata->spinlock);
		return scnprintf(buf, PAGE_SIZE, "%#lx %#lx\n", val1, val2);
	}
}

static ssize_t opcode_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t size)
{
	unsigned long val1, val2, val3;
	struct noc_trace_drvdata *drvdata = dev_get_drvdata(dev->parent);

	spin_lock(&drvdata->spinlock);
	if (drvdata->filter_nums == FILTER_NUM) {
		if (sscanf_s(buf, "%lx %lx %lx", &val1, &val2, &val3) !=
			PARAM_NUM_THREE) {
			spin_unlock(&drvdata->spinlock);
			return -EINVAL;
		}
		drvdata->filter_cfg[0]->opcode = val1;
		drvdata->filter_cfg[1]->opcode = val2;
		drvdata->filter_cfg[2]->opcode = val3;
	} else {
		if (sscanf_s(buf, "%lx %lx", &val1, &val2) != PARAM_NUM_TWO) {
			spin_unlock(&drvdata->spinlock);
			return -EINVAL;
		}
		drvdata->filter_cfg[0]->opcode = val1;
		drvdata->filter_cfg[1]->opcode = val2;
	}

	spin_unlock(&drvdata->spinlock);
	return size;
}
static DEVICE_ATTR_RW(opcode);

/* filter lut is the look up table to determine which filter to use */
static ssize_t filter_lut_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned long val;
	struct noc_trace_drvdata *drvdata = dev_get_drvdata(dev->parent);

	spin_lock(&drvdata->spinlock);
	val = drvdata->filterlut;
	spin_unlock(&drvdata->spinlock);
	return scnprintf(buf, PAGE_SIZE, "%#lx\n", val);
}

static ssize_t filter_lut_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	unsigned long val = 0;
	struct noc_trace_drvdata *drvdata = dev_get_drvdata(dev->parent);

	if (NULL == buf) {
		pr_err("[%s]: buf is NULL!!!\n", __func__);
		return -EINVAL;
	}

	ret = kstrtoul(buf, 16, &val);
	if (ret)
		return ret;

	spin_lock(&drvdata->spinlock);
	drvdata->filterlut = val;
	spin_unlock(&drvdata->spinlock);
	return size;
}
static DEVICE_ATTR_RW(filter_lut);

/* traceid shows that which source the data comes from */
static ssize_t traceid_show(
	struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned long val;
	struct noc_trace_drvdata *drvdata = dev_get_drvdata(dev->parent);

	spin_lock(&drvdata->spinlock);
	val = drvdata->trace_id;
	spin_unlock(&drvdata->spinlock);
	return scnprintf(buf, PAGE_SIZE, "%#lx\n", val);
}

static ssize_t traceid_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t size)
{
	int ret;
	unsigned long val = 0;
	struct noc_trace_drvdata *drvdata = dev_get_drvdata(dev->parent);

	if (NULL == buf) {
		pr_err("[%s]: buf is NULL!!!\n", __func__);
		return -EINVAL;
	}

	ret = kstrtoul(buf, 16, &val);
	if (ret)
		return ret;

	spin_lock(&drvdata->spinlock);
	/* traceid field is 7bit width */
	drvdata->trace_id = val & TRACEID_WIDE;
	spin_unlock(&drvdata->spinlock);
	return size;
}
static DEVICE_ATTR_RW(traceid);

static struct attribute *coresight_noc_trace_attrs[] = {
	&dev_attr_routeid_base.attr, &dev_attr_routeid_mask.attr,
	&dev_attr_addrbase_low.attr, &dev_attr_addrbase_high.attr,
	&dev_attr_windowsize.attr, &dev_attr_opcode.attr,
	&dev_attr_filter_lut.attr, &dev_attr_traceid.attr, NULL,
};

static const struct attribute_group coresight_noc_trace_group = {
	.attrs = coresight_noc_trace_attrs,
};

static const struct attribute_group *coresight_noc_trace_groups[] = {
	&coresight_noc_trace_group, NULL,
};

/*
 * Disable all  noc_trace.
 * It should be called when fetal error happens or system reset.
 */
void noc_trace_disable_all(void)
{
	unsigned int i = 0;

	struct noc_trace_drvdata *drvdata = NULL;
	for (; i < noc_trace_number; i++) {
		drvdata = noctracedrvdata[i];
		if (!drvdata)
			continue;
		if (drvdata->enable) {
			drvdata->enable = false;
			spin_lock(&drvdata->spinlock);
			noc_trace_disable_hw(drvdata);
			spin_unlock(&drvdata->spinlock);
			dev_info(drvdata->dev, "disabled\n");
			coresight_refresh_path(drvdata->csdev, 0);
		}
	}
	info_print("noc_trace_disable_all done\n");
}
EXPORT_SYMBOL_GPL(noc_trace_disable_all);

static int noc_trace_init_default_data(
	struct platform_device *pdev, struct device_node *np)
{
	int ret;
	u32 i;
	char filter_cfg_name[20] = {};
	struct noc_trace_drvdata *drvdata = NULL;

	if (NULL == pdev) {
		pr_err("[%s]: pdev is NULL!!!\n", __func__);
		return -EINVAL;
	}

	drvdata = platform_get_drvdata(pdev);
	if (NULL == drvdata) {
		pr_err("[%s]: drvdata is NULL!!!\n", __func__);
		return -EINVAL;
	}

	spin_lock(&drvdata->spinlock);

	if ((ret = of_property_read_u32(np, "trace_id",
		     (unsigned int *)(&drvdata->trace_id))) < 0) {
		pr_err("[%s] Failed: trace_id of_property_read_u32.%d\n",
			__func__, ret);
		spin_unlock(&drvdata->spinlock);
		return -EINVAL;
	}

	if ((ret = of_property_read_u32(np, "filter_lut",
		     (unsigned int *)(&drvdata->filterlut))) < 0) {
		pr_err("[%s] Failed: filter_lut of_property_read_u32.%d\n",
			__func__, ret);
		spin_unlock(&drvdata->spinlock);
		return -EINVAL;
	}

	/* Get filter configuretion Form DTS. */

	for (i = 0; (i < drvdata->filter_nums) && (i < FILTER_NUM_MAX); i++) {
		drvdata->filter_cfg[i] =
			(struct filter_configuration *)devm_kzalloc(&pdev->dev,
				sizeof(struct filter_configuration),
				GFP_KERNEL);
		if (!drvdata->filter_cfg[i]) {
			dev_err(&pdev->dev,
				"get drvdata->filter_cfg memory failed.\n");
			spin_unlock(&drvdata->spinlock);
			return -1;
		}

		scnprintf(filter_cfg_name, sizeof(filter_cfg_name),
			"cfg_filter%d", i);
		if ((ret = of_property_read_u32_array(np, filter_cfg_name,
			     (u32 *)drvdata->filter_cfg[i], NUM_FLITER_CFG)) <
			0) {
			pr_err("[%s] Get filter_cfg from DTS error.\n",
				__func__);
			spin_unlock(&drvdata->spinlock);
			return -EINVAL;
		}
	}

	spin_unlock(&drvdata->spinlock);
	return 0;
}

static int noc_get_reg_offset_from_dtsi(
	struct platform_device *pdev, struct device_node *np)
{
	u32 i;
	int ret;
	char filter_name[20] = {};
	struct noc_trace_drvdata *drvdata = NULL;

	if (NULL == pdev) {
		pr_err("[%s]: pdev is NULL!!!\n", __func__);
		return -1;
	}

	drvdata = platform_get_drvdata(pdev);
	if (NULL == drvdata) {
		pr_err("[%s]: drvdata is NULL!!!\n", __func__);
		return -1;
	}

	spin_lock(&drvdata->spinlock);
	/* Get disable reg offset Form DTS. */
	drvdata->disable_cfg_offset =
		(struct disable_configuration_offset *)devm_kzalloc(&pdev->dev,
			sizeof(struct disable_configuration_offset),
			GFP_KERNEL);
	if (!drvdata->disable_cfg_offset) {
		dev_err(&pdev->dev,
			"get drvdata->disable_cfg_offset memory failed.\n");
		spin_unlock(&drvdata->spinlock);
		return -1;
	}

	if ((ret = of_property_read_u32_array(np, "offset_disable",
		     (u32 *)drvdata->disable_cfg_offset,
		     NUM_DISABLE_CFG_OFFSET)) < 0) {
		pr_err("[%s] Get reg_offset from DTS error.\n", __func__);
		spin_unlock(&drvdata->spinlock);
		return -EINVAL;
	}

	/* Get enable reg offset Form DTS. */

	drvdata->enable_cfg_offset =
		(struct enable_configuration_offset *)devm_kzalloc(&pdev->dev,
			sizeof(struct enable_configuration_offset), GFP_KERNEL);
	if (!drvdata->enable_cfg_offset) {
		dev_err(&pdev->dev,
			"get drvdata->enable_cfg_offset memory failed.\n");
		spin_unlock(&drvdata->spinlock);
		return -1;
	}

	if ((ret = of_property_read_u32_array(np, "offset_enable",
		     (u32 *)drvdata->enable_cfg_offset,
		     NUM_ENABLE_CFG_OFFSET)) < 0) {
		pr_err("[%s] Get reg_offset from DTS error.\n", __func__);
		spin_unlock(&drvdata->spinlock);
		return -EINVAL;
	}

	/* Get filter numbers Form DTS. */
	if ((ret = of_property_read_u32(
		     np, "filter_nums", &(drvdata->filter_nums))) < 0) {
		pr_err("[%s] Get filter_nums from DTS error.\n", __func__);
		spin_unlock(&drvdata->spinlock);
		return -EINVAL;
	}

	/* Get filter configuretion reg offset Form DTS. */

	for (i = 0; (i < drvdata->filter_nums) && (i < FILTER_NUM_MAX); i++) {
		drvdata->filter_cfg_offset[i] =
			(struct filter_configuration_offset *)devm_kzalloc(
				&pdev->dev,
				sizeof(struct filter_configuration_offset),
				GFP_KERNEL);
		if (!drvdata->filter_cfg_offset[i]) {
			dev_err(&pdev->dev, "get drvdata->filter_cfg_offset "
					    "memory failed.\n");
			spin_unlock(&drvdata->spinlock);
			return -1;
		}

		scnprintf(
			filter_name, sizeof(filter_name), "offset_filter%d", i);
		if ((ret = of_property_read_u32_array(np, filter_name,
			     (u32 *)drvdata->filter_cfg_offset[i],
			     NUM_FLITER_CFG_OFFSET)) < 0) {
			pr_err("[%s] Get offset_filter from DTS error.\n",
				__func__);
			spin_unlock(&drvdata->spinlock);
			return -EINVAL;
		}
	}

	spin_unlock(&drvdata->spinlock);
	return 0;
}

/*lint -e429*/
static int noc_trace_probe(struct platform_device *pdev)
{
	int ret;
	void __iomem *base = NULL;
	struct device *dev = &pdev->dev;
	struct coresight_platform_data *pdata = NULL;
	struct noc_trace_drvdata *drvdata = NULL;
	struct resource *res = NULL;
	struct coresight_desc *desc = NULL;
	struct device_node *np = pdev->dev.of_node;

	if (!check_mntn_switch(MNTN_NOC_TRACE)) {
		pr_err("[%s] MNTN_NOC_TRACE is closed!\n", __func__);
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
			dev_err(dev, "of_get_coresight_platform_data fail \n");
			ret = -EINVAL;
			goto err_next;
		}
		pdev->dev.platform_data = pdata;
		drvdata->boot_enable =
			of_property_read_bool(np, "default_enable");
		dev_info(dev, "NOC boot_init =%d\n", drvdata->boot_enable);
	}

	drvdata->dev = &pdev->dev;
	platform_set_drvdata(pdev, drvdata);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "platform_get_resource fail \n");
		ret = -ENODEV;
		goto err_next;
	}

	if (noc_trace_number < NUM_NOC)
		noctracedrvdata[noc_trace_number++] = drvdata;

	base = devm_ioremap_resource(dev, res);
	if (IS_ERR_OR_NULL(base)) {
		dev_err(dev, "devm_ioremap_resource fail \n");
		ret = -EINVAL;
		goto err_next;
	}

	drvdata->base = base;

	spin_lock_init(&drvdata->spinlock);

	ret = noc_get_reg_offset_from_dtsi(pdev, np);
	if (ret) {
		pr_err("[%s] Failed: get_reg_offset_from_dtsi %d\n", __func__,
			ret);
		goto err_next;
	}

	ret = noc_trace_init_default_data(pdev, np);
	if (ret) {
		pr_err("[%s] Failed: noc_trace_init_default_data %d\n",
			__func__, ret);
		goto err_next;
	}

	desc = devm_kzalloc(dev, sizeof(*desc), GFP_KERNEL);
	if (IS_ERR_OR_NULL(desc)) {
		pr_err("[%s] coresight_desc get memory failed %d\n", __func__,
			ret);
		ret = -ENOMEM;
		goto err_next;
	}

	if (!pdata) {
		ret = -ENODEV;
		goto err_next;
	}

	desc->type = CORESIGHT_DEV_TYPE_SOURCE;
	desc->subtype.source_subtype = CORESIGHT_DEV_SUBTYPE_SOURCE_SOFTWARE;
	desc->ops = &noc_trace_cs_ops;
	desc->pdata = pdata;
	desc->dev = dev;
	desc->groups = coresight_noc_trace_groups;
	drvdata->csdev = coresight_register(desc);
	if (IS_ERR_OR_NULL(drvdata->csdev)) {
		pr_err("[%s] coresight_register failed %d\n", __func__, ret);
		ret = PTR_ERR(drvdata->csdev);
		goto err_coresight_register;
	}

	if (drvdata->boot_enable)
		coresight_enable(drvdata->csdev);

	dev_info(dev, "%s initialized\n", dev->kobj.name);

	return 0; /*lint !e593 !e429*/

err_coresight_register:
	devm_kfree(dev, desc);

err_next:
	devm_kfree(dev, drvdata);

err:
	return ret; /*lint !e593*/
}
/*lint +e429*/

#ifdef CONFIG_PM
static int noc_trace_runtime_suspend(
	struct platform_device *pdev, pm_message_t state)
{
	struct noc_trace_drvdata *drvdata = platform_get_drvdata(pdev);

	dev_info(drvdata->dev, "%s: +\n", __func__);
	if (drvdata->enable)
		coresight_disable(drvdata->csdev);

	dev_info(drvdata->dev, "%s: -\n", __func__);

	return 0;
}

static int noc_trace_runtime_resume(struct platform_device *pdev)
{
	struct noc_trace_drvdata *drvdata = platform_get_drvdata(pdev);

	dev_info(drvdata->dev, "%s: +\n", __func__);
	if (!drvdata->enable)
		coresight_enable(drvdata->csdev);

	dev_info(drvdata->dev, "%s: -\n", __func__);

	return 0;
}

#endif

static int noc_trace_remove(struct platform_device *pdev)
{
	struct noc_trace_drvdata *drvdata = platform_get_drvdata(pdev);

	coresight_unregister(drvdata->csdev);
	dev_info(drvdata->dev, "%s: coresight unregister done. \n", __func__);
	return 0;
}

static struct of_device_id noc_trace_match[] = {
	{.compatible = "hisilicon,coresight-noc"}, {}};

static struct platform_driver noc_trace_driver = {
	.probe = noc_trace_probe,
	.remove = noc_trace_remove,
#ifdef CONFIG_PM
	.suspend = noc_trace_runtime_suspend,
	.resume = noc_trace_runtime_resume,
#endif
	.driver =
		{
			.name = "coresight-noc",
			.owner = THIS_MODULE,
			.of_match_table = noc_trace_match,
			.suppress_bind_attrs = true,
		},
};

static int __init noc_trace_init(void)
{
	return platform_driver_register(&noc_trace_driver);
}

module_init(noc_trace_init);

static void __exit noc_trace_exit(void)
{
	platform_driver_unregister(&noc_trace_driver);
}

module_exit(noc_trace_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("CoreSight Noc Trace driver");
