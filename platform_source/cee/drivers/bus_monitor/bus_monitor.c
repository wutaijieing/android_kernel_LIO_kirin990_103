/*
 * bus_monitor.c
 *
 * for bus_monitor driver.
 *
 * Copyright (c) 2019-2020 Huawei Technologies Co., Ltd.
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
#include <linux/bus_monitor.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <soc_fcm_bus_perf_monitor_interface.h>
#include <soc_fcm_ip_ns_interface.h>
#include <securec.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/spinlock.h>

/* BITn set 1, mask(n+16)=1 */
#define bit_set1_mask(n) (BIT(n) | BIT((n) + 16))
/* BITn set 0, mask(n+16)=1 */
#define bit_set0_mask(n) (BIT((n) + 16))
#define bits_map_mask(_bit_start, _bit_end, _val) \
	(((_val) << (_bit_start)) | \
	(bit_set0_mask(_bit_end)) | \
	(bit_set0_mask(_bit_end) - bit_set0_mask(_bit_start)))

#define INPUT_CFG ((NUM_OF_CFG * 2) + 2)
#define CLEAR_IRQ 0xFFFFFFFF
#define FILT_FLAG 0xF

/*
 * busmon: data of bus monitor
 *
 * @regs: bus monitor register
 * @dev: bus monitor devices
 * @begin: bus monitor begin time
 * @end: bus monitor end time
 * @para: Configuration parameter
 */
struct busmon {
	void __iomem *regs;
	struct device *dev;
	ktime_t begin;
	ktime_t end;
	struct bus_monitor_cfg_para para;
};

/* g_busmonitor: global data of the bus monitor */
static struct busmon *g_busmonitor;
/* g_fcmipns_base: fcmipns devices */
static void __iomem *g_fcmipns_base;

static int get_tokenized_data(const char *buf,
			      struct bus_monitor_cfg_para *para)
{
	const char *cp = NULL;
	int i;
	int ntokens = 1;
	unsigned int *tokenized_data = NULL;

	cp = buf;
	while ((cp = strpbrk(cp + 1, " :")))
		ntokens++;

	if (ntokens != INPUT_CFG) {
		dev_err(g_busmonitor->dev, "[%s] Failed:ntokens is %d\n",
			__func__, ntokens);
		return -EINVAL;
	}

	tokenized_data = kmalloc(ntokens * sizeof(unsigned int), GFP_KERNEL);
	if (tokenized_data == NULL) {
		dev_err(g_busmonitor->dev, "[%s] Failed:No tokenized_data\n",
			__func__);
		return -ENOMEM;
	}
	cp = buf;
	i = 0;
	while (i < ntokens) {
		if (sscanf_s(cp, "%u", &tokenized_data[i++]) != 1) {
			dev_err(g_busmonitor->dev,
				"[%s] Failed:can not get data\n", __func__);
			goto err;
		}

		cp = strpbrk(cp, " :");
		if (!cp)
			break;
		cp++;
	}
	if (i != ntokens) {
		dev_err(g_busmonitor->dev,
			"[%s] Failed:Buf is wrong\n", __func__);
		goto err;
	}
	for (i = 0; i < NUM_OF_CFG; i++) {
		para->latency_range[i] = tokenized_data[i];
		para->osd_range[i] = tokenized_data[i + NUM_OF_CFG];
	}
	para->sample_type = tokenized_data[INPUT_CFG - 2];
	para->count_time = tokenized_data[INPUT_CFG - 1];
	kfree(tokenized_data);
	return 0;

err:
	kfree(tokenized_data);
	return -EINVAL;
}

static void busmonitor_module_unrst_and_open_clk(void)
{
	/* cpu bus monitor unlock reset */
	writel(BIT(SOC_FCM_IP_NS_FCM_LOCAL_CTRL8_rst_dis_bus_monitor_START) |
	       BIT(SOC_FCM_IP_NS_FCM_LOCAL_CTRL8_rst_dis_p2p_fvt_pcr_START),
	       SOC_FCM_IP_NS_FCM_LOCAL_CTRL8_ADDR(g_fcmipns_base));
	/* cpu bus monitor open clock */
	writel(bit_set1_mask(SOC_FCM_IP_NS_FCM_LOCAL_CTRL7_clkoff_fvt_pcr_p2p_START) |
	       bit_set1_mask(SOC_FCM_IP_NS_FCM_LOCAL_CTRL7_clkoff_bus_monitor_START),
	       SOC_FCM_IP_NS_FCM_LOCAL_CTRL7_ADDR(g_fcmipns_base));
}

static void busmonitor_set_mode(u32 ctrl_value)
{
	writel(ctrl_value, SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_ADDR(g_busmonitor->regs));
}

/*
 * busmonitor_set_filter() bus transaction filter config
 * filter_value: monitor cfg
 * type: 0 is read operations, 1 is write operation
 */
static void busmonitor_set_filter(u32 filter_value, int type)
{
	if (type == 0)
		writel(filter_value,
		       SOC_FCM_BUS_PERF_MONITOR_MONITOR_READ_FILT_CTL_ADDR(g_busmonitor->regs));
	else
		writel(filter_value,
		       SOC_FCM_BUS_PERF_MONITOR_MONITOR_WRITE_FILT_CTL_ADDR(g_busmonitor->regs));
}
/* cfg sample time */
static void busmonitor_set_time(u32 time_value)
{
	writel(time_value, SOC_FCM_BUS_PERF_MONITOR_MONITOR_LOAD_CNT_ADDR(g_busmonitor->regs));
}

static void busmonitor_set_latency_and_outstanding_cfg(void)
{
	int i;

	for (i = 0; i < NUM_OF_CFG; i++) {
		writel(g_busmonitor->para.latency_range[i],
		       SOC_FCM_BUS_PERF_MONITOR_MONITOR_AX_LATENCY_CFG0_ADDR(g_busmonitor->regs) + i * 0x04);
		writel(g_busmonitor->para.osd_range[i],
		       SOC_FCM_BUS_PERF_MONITOR_MONITOR_AX_OSD_CFG0_ADDR(g_busmonitor->regs) + i * 0x04);
	}
}

static void busmonitor_glb_enable(void)
{
	writel(BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_GLB_EN_monitor_global_en_START),
	       SOC_FCM_BUS_PERF_MONITOR_MONITOR_GLB_EN_ADDR(g_busmonitor->regs));
}

static void busmonitor_glb_disable(void)
{
	writel(!BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_GLB_EN_monitor_global_en_START),
	       SOC_FCM_BUS_PERF_MONITOR_MONITOR_GLB_EN_ADDR(g_busmonitor->regs));
}

static void busmonitor_module_rst_and_close_clk(void)
{
	/* cpu bus monitor close clock */
	writel(bit_set0_mask(SOC_FCM_IP_NS_FCM_LOCAL_CTRL7_clkoff_bus_monitor_START),
	       SOC_FCM_IP_NS_FCM_LOCAL_CTRL7_ADDR(g_fcmipns_base));
	writel(BIT(SOC_FCM_IP_NS_FCM_LOCAL_CTRL8_rst_dis_bus_monitor_START),
	       SOC_FCM_IP_NS_FCM_LOCAL_CTRL6_ADDR(g_fcmipns_base));
}

static bool busmonitor_is_unrst(void)
{
	u32 value = readl(SOC_FCM_IP_NS_FCM_LOCAL_CTRL9_ADDR(g_fcmipns_base));

	if ((value & BIT(SOC_FCM_IP_NS_FCM_LOCAL_CTRL9_rst_stat_bus_monitor_START)) == 0)
		return true;
	return false;
}

static DEFINE_SPINLOCK(bus_monitor_lock);
/* Get the bin-counter results of latency/outstanding */
ktime_t bus_monitor_get_result(struct bus_monitor_result *res)
{
	u32 value;
	int i;
	ktime_t begin_to_end_time = -EPERM;
	unsigned long flags = 0;

	if (res == NULL) {
		dev_err(g_busmonitor->dev,
			"[%s] Failed:invalid input\n", __func__);
		return -EINVAL;
	}

	spin_lock_irqsave(&bus_monitor_lock, flags);
	if (!busmonitor_is_unrst()) {
		dev_err(g_busmonitor->dev,
			"[%s] Failed:bus_monitor_not_unrst\n", __func__);
		goto err;
	}
	value = readl(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_ADDR(g_busmonitor->regs));
	/* stop count for free run */
	if ((value & BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_TimerEn_START)) == 0) {
		writel(bit_set0_mask(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_monitor_cnt_en_START),
		       SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_ADDR(g_busmonitor->regs));
		writel(bit_set1_mask(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_monitor_cnt_en_START),
		       SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_ADDR(g_busmonitor->regs));
		g_busmonitor->end = ktime_get();
	}
	for (i = 0; i < NUM_OF_CNT; i++)
		res->latency[i] =
			readl(SOC_FCM_BUS_PERF_MONITOR_MONITOR_LATENCY_BIN0_CNT_ADDR(g_busmonitor->regs) + i * 0x04);
	for (i = 0; i < NUM_OF_CNT; i++)
		res->osd[i] =
			readl(SOC_FCM_BUS_PERF_MONITOR_MONITOR_OUTSTANDING_BIN0_CNT_ADDR(g_busmonitor->regs) + i * 0x04);
	if (g_busmonitor->end == 0) {
		begin_to_end_time = 0;
	} else {
		begin_to_end_time = ktime_sub(g_busmonitor->end,
					      g_busmonitor->begin);
	}
err:
	spin_unlock_irqrestore(&bus_monitor_lock, flags);
	return begin_to_end_time;
}

void bus_monitor_cfg(struct bus_monitor_cfg_para *para)
{
	u32 value;
	unsigned long flags = 0;

	if (para == NULL) {
		dev_err(g_busmonitor->dev,
			"[%s] Failed:invalid input\n", __func__);
		return;
	}
	spin_lock_irqsave(&bus_monitor_lock, flags);
	g_busmonitor->para = *para;
	if (busmonitor_is_unrst()) {
		dev_err(g_busmonitor->dev,
			"[%s] Failed:busmonitor is unrst\n", __func__);
		goto err;
	}
	/* 0 is read,1 is write */
	busmonitor_module_unrst_and_open_clk();
	if ((g_busmonitor->para.sample_type & BIT(31)) != 0) {
		g_busmonitor->para.sample_type &= FILT_FLAG;
		busmonitor_set_filter(bits_map_mask(SOC_FCM_BUS_PERF_MONITOR_MONITOR_WRITE_FILT_CTL_aw_op_mode_START,
						    SOC_FCM_BUS_PERF_MONITOR_MONITOR_WRITE_FILT_CTL_aw_op_sel_END,
						    g_busmonitor->para.sample_type), 1);
		value = bit_set1_mask(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_monitor_ax_sel_START);
	} else {
		g_busmonitor->para.sample_type &= FILT_FLAG;
		busmonitor_set_filter(bits_map_mask(SOC_FCM_BUS_PERF_MONITOR_MONITOR_READ_FILT_CTL_ar_op_mode_START,
						    SOC_FCM_BUS_PERF_MONITOR_MONITOR_READ_FILT_CTL_ar_op_sel_END,
						    g_busmonitor->para.sample_type), 0);
		value = bit_set0_mask(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_monitor_ax_sel_START);
	}

	if (g_busmonitor->para.count_time == 0) {
		/* free run model */
		value |= bit_set0_mask(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_TimerEn_START);
		value |= bit_set1_mask(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_monitor_cnt_en_START);
	} else {
		/* period run model */
		value |= bit_set1_mask(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_TimerEn_START);
		value |= bit_set1_mask(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_IntEnable_START);
		value |= bit_set0_mask(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_OneShot_START);
		value |= bit_set1_mask(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_monitor_cnt_en_START);
	}
	busmonitor_set_latency_and_outstanding_cfg();
	g_busmonitor->begin = ktime_get();
	g_busmonitor->end = 0;
	busmonitor_set_mode(value);
	busmonitor_set_time(g_busmonitor->para.count_time);
	busmonitor_glb_enable();
err:
	spin_unlock_irqrestore(&bus_monitor_lock, flags);
}

static ssize_t bus_monitor_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	ssize_t size = 0;
	ssize_t ret;
	int i;
	struct bus_monitor_result *res = NULL;
	ktime_t begin_to_end_time;

	(void)dev;
	(void)attr;

	res = kmalloc(sizeof(*res), GFP_KERNEL);
	if (res == NULL) {
		dev_err(g_busmonitor->dev,
			"[%s] Failed:result_malloc\n", __func__);
		return -ENODEV;
	}
	begin_to_end_time = bus_monitor_get_result(res);
	if (begin_to_end_time < 0) {
		dev_err(g_busmonitor->dev,
			"[%s] Failed:begin_to_time=%lld\n",
			__func__, begin_to_end_time);
		goto err;
	}
	ret = snprintf_s(buf + size, PAGE_SIZE - size, PAGE_SIZE - size - 1,
			 "%lld:", begin_to_end_time);
	if (ret < 0) {
		size = -ENOMEM;
		goto err;
	}
	size += ret;
	for (i = 0; i < NUM_OF_CNT; i++) {
		ret = snprintf_s(buf + size, PAGE_SIZE - size,
				 PAGE_SIZE - size - 1, "%d:", res->latency[i]);
		if (ret < 0) {
			size = -ENOMEM;
			goto err;
		}
		size += ret;
	}
	for (i = 0; i < NUM_OF_CNT; i++) {
		ret = snprintf_s(buf + size, PAGE_SIZE - size,
				 PAGE_SIZE - size - 1, "%d:", res->osd[i]);
		if (ret < 0) {
			size = -ENOMEM;
			goto err;
		}
		size += ret;
	}
	if (size > 0)
		buf[size - 1] = '\n';

err:
	kfree(res);
	return size;
}

static ssize_t bus_monitor_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t size)
{
	int ret;
	struct bus_monitor_cfg_para *para = NULL;

	(void)dev;
	(void)attr;

	para = kmalloc(sizeof(*para), GFP_KERNEL);
	if (para == NULL)
		return size;
	ret = get_tokenized_data(buf, para);
	if (ret != 0) {
		dev_err(g_busmonitor->dev,
			"[%s] Failed:get range is fail\n", __func__);
		goto err;
	}
	bus_monitor_cfg(para);
err:
	kfree(para);
	return size;
}

static DEVICE_ATTR_RW(bus_monitor);

void bus_monitor_stop(void)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&bus_monitor_lock, flags);
	if (busmonitor_is_unrst()) {
		g_busmonitor->begin = 0;
		g_busmonitor->end = 0;
		busmonitor_glb_disable();
		busmonitor_module_rst_and_close_clk();
	}
	spin_unlock_irqrestore(&bus_monitor_lock, flags);
}
EXPORT_SYMBOL_GPL(bus_monitor_stop);

static irqreturn_t busmon_irq(int irq, void *data)
{
	(void)data;
	if (g_busmonitor->begin != 0)
		g_busmonitor->end = ktime_get();
	spin_lock(&bus_monitor_lock);
	if (busmonitor_is_unrst())
		writel(CLEAR_IRQ, SOC_FCM_BUS_PERF_MONITOR_MONITOR_CLR_INT_ADDR(g_busmonitor->regs));
	spin_unlock(&bus_monitor_lock);
	return IRQ_HANDLED;
}

static int bus_monitor_probe(struct platform_device *pdev)
{
	int ret;
	int irq;
	struct resource *addr = NULL;

	dev_info(&pdev->dev, " [%s]: - entry.\n", __func__);
	addr = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (addr == NULL) {
		dev_err(&pdev->dev,
			"[%s] Failed:No get fcmipns_addr\n", __func__);
		return -EFAULT;
	}
	g_fcmipns_base = devm_ioremap(&pdev->dev, addr->start,
				      resource_size(addr));
	if (g_fcmipns_base == NULL) {
		dev_err(&pdev->dev,
			"[%s] Failed:No get fcmipns_base\n", __func__);
		return -EFAULT;
	}

	g_busmonitor = devm_kzalloc(&pdev->dev, sizeof(*g_busmonitor),
				    GFP_KERNEL);
	if (g_busmonitor == NULL)
		return -EFAULT;
	addr = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (addr == NULL) {
		dev_err(&pdev->dev,
			"[%s] Failed:No get busmonitor_addr\n", __func__);
		ret = -EFAULT;
		goto err;
	}
	g_busmonitor->regs = devm_ioremap(&pdev->dev, addr->start,
					  resource_size(addr));
	if (g_busmonitor->regs == NULL) {
		dev_err(&pdev->dev,
			"[%s] Failed:No get busmonreg1\n", __func__);
		ret = -EFAULT;
		goto err;
	}
	g_busmonitor->dev = &pdev->dev;
	platform_set_drvdata(pdev, g_busmonitor);

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(g_busmonitor->dev,
			"[%s] Failed: No get irq resourse\n", __func__);
		return irq;
	}
	ret = devm_request_irq(&pdev->dev, irq, busmon_irq, 0,
			       dev_name(&pdev->dev), g_busmonitor);
	if (ret != 0) {
		dev_err(g_busmonitor->dev,
			"[%s] Failed: No get request irq\n", __func__);
		return ret;
	}
	ret = device_create_file(&pdev->dev, &dev_attr_bus_monitor);
	if (ret != 0) {
		dev_err(&pdev->dev,
			"[%s] Failed: sys file creation failed\n", __func__);
		return -ENODEV;
	}
	return 0;
err:
	devm_kfree(&pdev->dev, g_busmonitor);
	return ret;
}

static int bus_monitor_remove(struct platform_device *pdev)
{
	device_remove_file(&pdev->dev, &dev_attr_bus_monitor);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id bus_monitor[] = {
	{.compatible = "lpm,bus_monitor",},
	{},
};

MODULE_DEVICE_TABLE(of, bus_monitor);
#endif

static struct platform_driver bus_monitor_driver = {
	.probe  = bus_monitor_probe,
	.remove = bus_monitor_remove,
	.driver = {
		.name = "BUS_MONITOR",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(bus_monitor),
	},
};

static int __init bus_monitor_init(void)
{
	return platform_driver_register(&bus_monitor_driver);
}
module_init(bus_monitor_init);

static void __exit bus_monitor_exit(void)
{
	platform_driver_unregister(&bus_monitor_driver);
}
module_exit(bus_monitor_exit);
