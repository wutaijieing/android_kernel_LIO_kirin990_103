/*
 * bus_monitor_test.c
 *
 * test for bus_monitor.
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

#define TEST_CTRL_FUNC_MAX_NR 21
/*
 * EMU test time 10ms
 * FPGA test time 333ms
 */
#define SAMPLE_TIME 0x32DCD5
#define NUM_OF_CNT 11
#define NUM_OF_CFG 10
#define LATENCY_CFG1_DEFAULT_VALUE 0x60
#define MASK_BIT 0xFFFF0000
#define bit_set1_mask(n) (BIT(n) | BIT((n) + 16))
#define READ_WRAP_RUN_IRQENABLE \
	(MASK_BIT | BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_monitor_cnt_en_START) | \
	BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_TimerEn_START) | \
	BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_IntEnable_START))
#define WRITE_ONE_SHORT_IRQENABLE \
	(MASK_BIT | BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_monitor_cnt_en_START) | \
	BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_monitor_ax_sel_START) | \
	BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_TimerEn_START) | \
	BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_IntEnable_START) | \
	BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_OneShot_START))
#define READ_FREE_RUN_IRQDISABLE \
	(MASK_BIT | BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_monitor_cnt_en_START) | \
	BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_OneShot_START))
#define READ_ONE_SHORT_IRQDISABLE \
	(MASK_BIT | BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_monitor_cnt_en_START) | \
	BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_OneShot_START) | \
	BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_TimerEn_START))
#define WRITE_WRAP_IRQENABLE \
	(MASK_BIT | BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_monitor_cnt_en_START) | \
	BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_monitor_ax_sel_START) | \
	BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_TimerEn_START))
#define WRITE_FREE_RUN_IRQENABLE \
	(MASK_BIT | BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_monitor_cnt_en_START) | \
	BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_monitor_ax_sel_START) | \
	BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_IntEnable_START) | \
	BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_OneShot_START))
#define READ_ONE_SHORT_IRQENABLE \
	(MASK_BIT | BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_monitor_cnt_en_START) | \
	BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_TimerEn_START) | \
	BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_IntEnable_START) | \
	BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_OneShot_START))

struct busmon {
	void __iomem *regs;
	struct device *dev;
	int writenum;
	u32 readnum;
	ktime_t begin;
	ktime_t end;
};

typedef void (*test_ctrl_func)(struct busmon *);

static void set_bus_monitior_mode(struct busmon *busmon, u32 ctrl_value,
				  u32 load_cnt_value)
{
	writel(0x0, SOC_FCM_BUS_PERF_MONITOR_MONITOR_GLB_EN_ADDR(busmon->regs));
	writel(ctrl_value, SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_ADDR(busmon->regs));
	writel(0x0, SOC_FCM_BUS_PERF_MONITOR_MONITOR_WRITE_FILT_CTL_ADDR(busmon->regs));
	writel(0x0, SOC_FCM_BUS_PERF_MONITOR_MONITOR_READ_FILT_CTL_ADDR(busmon->regs));
	writel(load_cnt_value, SOC_FCM_BUS_PERF_MONITOR_MONITOR_LOAD_CNT_ADDR(busmon->regs));
}

static void start_bus_monitior(struct busmon *busmon)
{
	writel(0x1, SOC_FCM_BUS_PERF_MONITOR_MONITOR_GLB_EN_ADDR(busmon->regs));
}

static void set_latency_and_outstanding_cfg(struct busmon *busmon,
					    u32 latency[NUM_OF_CFG],
					    u32 osd[NUM_OF_CFG],
					    u32 statistic_length)
{
	int i;

	if (statistic_length != NUM_OF_CFG)
		return;
	for (i = 0; i < NUM_OF_CFG; i++) {
		writel(latency[i], SOC_FCM_BUS_PERF_MONITOR_MONITOR_AX_LATENCY_CFG0_ADDR(busmon->regs) + i * 0x04);
		writel(osd[i], SOC_FCM_BUS_PERF_MONITOR_MONITOR_AX_OSD_CFG0_ADDR(busmon->regs) + i * 0x04);
	}
}

static void set_open_clock(struct busmon *busmon)
{
	u32 default_value = readl(SOC_FCM_BUS_PERF_MONITOR_MONITOR_AX_LATENCY_CFG1_ADDR(busmon->regs));

	dev_err(busmon->dev, "get %s\n", __func__);
	if (default_value == LATENCY_CFG1_DEFAULT_VALUE)
		dev_err(busmon->dev, "%s operation is right\n", __func__);
	else
		dev_err(busmon->dev, "%s operation is wrong\n", __func__);
}

static void get_read_ans(struct busmon *busmon)
{
	u32 default_value = readl(SOC_FCM_BUS_PERF_MONITOR_MONITOR_AX_LATENCY_CFG1_ADDR(busmon->regs));

	dev_err(busmon->dev, "get %s\n", __func__);
	dev_err(busmon->dev, "The value is %x bus_monitor\n", default_value);
}

static void get_write_ans(struct busmon *busmon)
{
	u32 default_value;

	dev_err(busmon->dev, "get %s\n", __func__);
	writel(0x1, SOC_FCM_BUS_PERF_MONITOR_MONITOR_GLB_EN_ADDR(busmon->regs));
	default_value = readl(SOC_FCM_BUS_PERF_MONITOR_MONITOR_GLB_EN_ADDR(busmon->regs));
	dev_err(busmon->dev, "get %s\n", __func__);
	dev_err(busmon->dev, "The value of is %x\n", default_value);
}

static void set_wrap(struct busmon *busmon)
{
	dev_err(busmon->dev, "get %s\n", __func__);
	/* set_read_wrap_irq_enabl */
	set_bus_monitior_mode(busmon, READ_WRAP_RUN_IRQENABLE, SAMPLE_TIME);
	start_bus_monitior(busmon);
	dev_err(busmon->dev, "%s end,please go to read_ans\n", __func__);
}

static void set_one_short(struct busmon *busmon)
{
	dev_err(busmon->dev, "get %s\n", __func__);
	/* set_write_one_short_irq_enable */
	set_bus_monitior_mode(busmon, WRITE_ONE_SHORT_IRQENABLE, SAMPLE_TIME);
	start_bus_monitior(busmon);
	dev_err(busmon->dev, "%s end,please go to read_ans\n", __func__);
}

static void set_free_run(struct busmon *busmon)
{
	dev_err(busmon->dev, "get %s\n", __func__);
	/* set_read_free_irq_disable */
	set_bus_monitior_mode(busmon, READ_FREE_RUN_IRQDISABLE, SAMPLE_TIME);
	start_bus_monitior(busmon);
	dev_err(busmon->dev, "%s end,please go to read_ans\n", __func__);
}

static void set_irq_enable(struct busmon *busmon)
{
	dev_err(busmon->dev, "get %s\n", __func__);
	/* set_write_one_short_irq_enable */
	set_bus_monitior_mode(busmon, WRITE_ONE_SHORT_IRQENABLE, SAMPLE_TIME);
	start_bus_monitior(busmon);
	dev_err(busmon->dev, "%s end,please go to read_ans\n", __func__);
}

static void set_irq_disable(struct busmon *busmon)
{
	dev_err(busmon->dev, "get %s\n", __func__);
	/* set_read_one_short_irq_disable */
	set_bus_monitior_mode(busmon, READ_ONE_SHORT_IRQDISABLE, SAMPLE_TIME);
	start_bus_monitior(busmon);
	dev_err(busmon->dev, "%s end,please go to read_ans\n", __func__);
}

static void set_read_wrap_irq_enable(struct busmon *busmon)
{
	dev_err(busmon->dev, "get %s\n", __func__);
	/* set_read_wrap_irq_enabl */
	set_bus_monitior_mode(busmon, READ_WRAP_RUN_IRQENABLE, SAMPLE_TIME);
	start_bus_monitior(busmon);
	dev_err(busmon->dev, "%s end,please go to read_ans\n", __func__);
}

static void set_write_wrap_irq_disable(struct busmon *busmon)
{
	dev_err(busmon->dev, "get %s\n", __func__);
	/* set_write_wrap_irq_disable */
	set_bus_monitior_mode(busmon, WRITE_WRAP_IRQENABLE, SAMPLE_TIME);
	start_bus_monitior(busmon);
	dev_err(busmon->dev, "%s end,please go to read_ans\n", __func__);
}

static void set_read_one_short_irq_disable(struct busmon *busmon)
{
	dev_err(busmon->dev, "get %s\n", __func__);
	/* set_read_one_short_irq_disable */
	set_bus_monitior_mode(busmon, READ_ONE_SHORT_IRQDISABLE, SAMPLE_TIME);
	start_bus_monitior(busmon);
	dev_err(busmon->dev, "%s ,please go to read_ans\n", __func__);
}

static void set_write_one_short_irq_enable(struct busmon *busmon)
{
	dev_err(busmon->dev, "get %s\n", __func__);
	/* set_write_one_short_irq_enable */
	set_bus_monitior_mode(busmon, WRITE_ONE_SHORT_IRQENABLE, SAMPLE_TIME);
	start_bus_monitior(busmon);
	dev_err(busmon->dev, "%s end,please go to read_ans\n", __func__);
}

static void set_read_free_irq_disable(struct busmon *busmon)
{
	dev_err(busmon->dev, "get %s\n", __func__);
	/* set_read_free_irq_disable */
	set_bus_monitior_mode(busmon, READ_FREE_RUN_IRQDISABLE, SAMPLE_TIME);
	start_bus_monitior(busmon);
	dev_err(busmon->dev, "%s end,please go to read_ans\n", __func__);
}

static void set_write_free_irq_enable(struct busmon *busmon)
{
	dev_err(busmon->dev, "get %s\n", __func__);
	/* set_write_free_irq_enable */
	set_bus_monitior_mode(busmon, WRITE_FREE_RUN_IRQENABLE, SAMPLE_TIME);
	start_bus_monitior(busmon);
	dev_err(busmon->dev, "%s end,please go to read_ans\n", __func__);
}

static void set_running_time(struct busmon *busmon)
{
	busmon->begin = ktime_get();
	set_bus_monitior_mode(busmon, WRITE_ONE_SHORT_IRQENABLE, SAMPLE_TIME);
	start_bus_monitior(busmon);
	dev_err(busmon->dev, "The begin time is %lld\n", busmon->begin);
	dev_err(busmon->dev, "%s end\n", __func__);
}

static void set_first_cfg_latency_and_outstanding(struct busmon *busmon)
{
	/* latency_cfg */
	u32 latency[NUM_OF_CFG] = {
		0x0, 0x10, 0x20, 0x30, 0x40,
		0x50, 0x60, 0x70, 0x80, 0xA0
	};
	/* outstandint_cfg */
	u32 osd[NUM_OF_CFG] = {
		0x0, 0x2, 0x8, 0xc, 0x10,
		0x14, 0x18, 0x1C, 0x20, 0x24
	};

	dev_err(busmon->dev, "get %s\n", __func__);
	/* set_read_one_short_irq_enable */
	set_bus_monitior_mode(busmon, READ_ONE_SHORT_IRQENABLE, SAMPLE_TIME);
	set_latency_and_outstanding_cfg(busmon, latency, osd, NUM_OF_CFG);
	start_bus_monitior(busmon);
	dev_err(busmon->dev, "%s end,please go to read_ans\n", __func__);
}

static void set_second_cfg_latency_and_outstanding(struct busmon *busmon)
{
	/* latency_cfg */
	u32 latency[NUM_OF_CFG] = {
		0x0, 0x20, 0x40, 0x60, 0x80,
		0xA0, 0xC0, 0xE0, 0x100, 0x120
	};
	/* outstandint_cfg */
	u32 osd[NUM_OF_CFG] = {
		0x0, 0x8, 0x10, 0x18, 0x20,
		0x24, 0x30, 0x37, 0x40, 0x47
	};

	dev_err(busmon->dev, "get %s\n", __func__);
	/* set_read_one_short_irq_enable */
	set_bus_monitior_mode(busmon, READ_ONE_SHORT_IRQENABLE, SAMPLE_TIME);
	set_latency_and_outstanding_cfg(busmon, latency, osd, NUM_OF_CFG);
	start_bus_monitior(busmon);
	dev_err(busmon->dev, "%s end,please go to read_ans\n", __func__);
}

static void clear_irq(struct busmon *busmon)
{
	dev_err(busmon->dev, "get %s\n", __func__);
	writel(0xffffffff, SOC_FCM_BUS_PERF_MONITOR_MONITOR_CLR_INT_ADDR(busmon->regs));
	dev_err(busmon->dev, "%s end,please go to read_ans\n", __func__);
}

static void search_irq(struct busmon *busmon)
{
	u32 irq_status;

	set_read_one_short_irq_disable(busmon);
	dev_err(busmon->dev, "get %s\n", __func__);
	irq_status = readl(SOC_FCM_BUS_PERF_MONITOR_MONITOR_RIS_INT_ADDR(busmon->regs));
	dev_err(busmon->dev, "%s irq_status is %x\n", __func__, irq_status);
}

static void set_scare(struct busmon *busmon)
{
	/* latency_cfg */
	u32 latency[NUM_OF_CFG] = {
		0x0, 0x8, 0x10, 0x18, 0x20,
		0x28, 0x30, 0x38, 0x40, 0x50
	};
	/* outstandint_cfg */
	u32 osd[NUM_OF_CFG] = {
		0x0, 0x1, 0x4, 0x6, 0x8,
		0xA, 0xC, 0xE, 0x10, 0x12
	};

	dev_err(busmon->dev, "get %s\n", __func__);
	/* set_read_one_short_irq_enable */
	set_bus_monitior_mode(busmon, READ_ONE_SHORT_IRQENABLE |
		BIT(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_monitor_scaler_cnt_START), SAMPLE_TIME);
	set_latency_and_outstanding_cfg(busmon, latency, osd, NUM_OF_CFG);
	start_bus_monitior(busmon);
	dev_err(busmon->dev, "%s end,please go to read_ans\n", __func__);
}

static void read_ans(struct busmon *busmon)
{
	u32 latency_cnt;
	u32 osd_cnt;
	int i;
	u32 ctrl_addr = readl(SOC_FCM_BUS_PERF_MONITOR_MONITOR_CTRL_ADDR(busmon->regs));

	dev_err(busmon->dev, "Read_ans operation is beginning,%s\n", __func__);
	for (i = 0; i < NUM_OF_CNT; i++) {
		latency_cnt = readl(SOC_FCM_BUS_PERF_MONITOR_MONITOR_LATENCY_BIN0_CNT_ADDR(busmon->regs) + i * 0x04);
		dev_err(busmon->dev, "The part of latency %d is %x\n",
			i, latency_cnt);
	}
	for (i = 0; i < NUM_OF_CNT; i++) {
		osd_cnt = readl(SOC_FCM_BUS_PERF_MONITOR_MONITOR_OUTSTANDING_BIN0_CNT_ADDR(busmon->regs) + i * 0x04);
		dev_err(busmon->dev, "The part of osd %d is %x\n", i, osd_cnt);
	}
	dev_err(busmon->dev, "Read_ans operation is end,%s", __func__);
}

test_ctrl_func test_array[TEST_CTRL_FUNC_MAX_NR] = {
	read_ans,
	set_open_clock,
	get_read_ans,
	get_write_ans,
	set_wrap,
	set_one_short,
	set_free_run,
	set_irq_enable,
	set_irq_disable,
	set_read_wrap_irq_enable,
	set_write_wrap_irq_disable,
	set_read_one_short_irq_disable,
	set_write_one_short_irq_enable,
	set_read_free_irq_disable,
	set_write_free_irq_enable,
	set_running_time,
	set_first_cfg_latency_and_outstanding,
	set_second_cfg_latency_and_outstanding,
	clear_irq,
	search_irq,
	set_scare
};

static ssize_t bus_monitor_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = container_of(dev, struct platform_device,
						    dev);
	struct busmon *busmon = platform_get_drvdata(pdev);

	read_ans(busmon);

	return snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1,
			  "read_bus_monitor is OK\n");
}

static ssize_t bus_monitor_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t size)
{
	struct platform_device *pdev =
		container_of(dev, struct platform_device, dev);
	struct busmon *busmon = platform_get_drvdata(pdev);
	ssize_t ret = 0;

	ret = sscanf_s(buf, "%d", &busmon->writenum);
	if (ret == -1)
		return ret;

	if (busmon->writenum < 0 || busmon->writenum >= TEST_CTRL_FUNC_MAX_NR) {
		dev_err(busmon->dev, "Writenum is wrong,%s", __func__);
		return size;
	}
	if (test_array[busmon->writenum] == NULL) {
		dev_err(busmon->dev, "test_array is NULL,%s", __func__);
		return size;
	}
	(*test_array[busmon->writenum])(busmon);
	dev_err(busmon->dev, "test is %d\n", busmon->writenum);
	return size;
}

static DEVICE_ATTR(bus_monitor_sys, 0660, bus_monitor_show, bus_monitor_store);

static irqreturn_t busmon_irq(int irq, void *data)
{
	struct busmon *busmon = (struct busmon *)data;
	ktime_t begin_to_end_time;

	if (busmon->begin != 0) {
		busmon->end = ktime_get();
		begin_to_end_time = ktime_sub(busmon->end, busmon->begin);
		dev_err(busmon->dev, "The time is %lld\n", begin_to_end_time);
		busmon->begin = 0;
	}
	dev_err(busmon->dev, "The irq bus_monitor is done,%s\n", __func__);
	writel(0xffffffff, SOC_FCM_BUS_PERF_MONITOR_MONITOR_CLR_INT_ADDR(busmon->regs));
	read_ans(busmon);
	dev_err(busmon->dev, "The irq bus_monitor is clear\n,%s", __func__);
	return IRQ_HANDLED;
}

static int bus_monitor_probe(struct platform_device *pdev)
{
	int ret;
	int irq;
	struct resource *addr = NULL;
	void __iomem *fcmipns_base = NULL;
	struct busmon *busmon = NULL;

	dev_err(&pdev->dev, "%s : %s : %d - entry.\n",
		__FILE__, __func__, __LINE__);
	if (pdev == NULL)
		return -EINVAL;
	addr = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (addr == NULL) {
		dev_err(&pdev->dev, "No get resource0,%s\n", __func__);
		return -EFAULT;
	}
	fcmipns_base = devm_ioremap(&pdev->dev, addr->start,
				    resource_size(addr));
	if (fcmipns_base == NULL) {
		dev_err(&pdev->dev,
			"[%s] Failed:No get fcmipns_base\n", __func__);
		return -EFAULT;
	}

	/* cpu bus monitor unlock reset */
	writel(BIT(SOC_FCM_IP_NS_FCM_LOCAL_CTRL8_rst_dis_bus_monitor_START) |
	       BIT(SOC_FCM_IP_NS_FCM_LOCAL_CTRL8_rst_dis_p2p_fvt_pcr_START),
	       SOC_FCM_IP_NS_FCM_LOCAL_CTRL8_ADDR(fcmipns_base));
	/* cpu bus monitor open clock */
	writel(bit_set1_mask(SOC_FCM_IP_NS_FCM_LOCAL_CTRL7_clkoff_fvt_pcr_p2p_START) |
	       bit_set1_mask(SOC_FCM_IP_NS_FCM_LOCAL_CTRL7_clkoff_bus_monitor_START),
	       SOC_FCM_IP_NS_FCM_LOCAL_CTRL7_ADDR(fcmipns_base));

	busmon = devm_kzalloc(&pdev->dev, sizeof(*busmon), GFP_KERNEL);
	if (busmon == NULL) {
		dev_err(&pdev->dev, "error get busmon%s\n", __func__);
		return -ENOMEM;
	}

	addr = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (addr == NULL) {
		dev_err(&pdev->dev, "No get resource1%s\n", __func__);
		ret = -EFAULT;
		goto err;
	}

	busmon->regs = devm_ioremap(&pdev->dev, addr->start,
				    resource_size(addr));
	if (busmon->regs == NULL) {
		dev_err(&pdev->dev, "cannot get busmonreg0%s\n", __func__);
		ret = -EFAULT;
		goto err;
	}

	busmon->dev = &pdev->dev;
	platform_set_drvdata(pdev, busmon);
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(busmon->dev, "failed to get irq%s\n", __func__);
		return irq;
	}

	ret = devm_request_irq(&pdev->dev, irq, busmon_irq, 0,
			       dev_name(&pdev->dev), busmon);
	if (ret != 0) {
		dev_err(busmon->dev, "failed to request irq%s\n", __func__);
		return ret;
	}
	ret = device_create_file(&pdev->dev, &dev_attr_bus_monitor_sys);
	if (ret != 0) {
		dev_err(&pdev->dev, "sys file creation failed\n");
		return -ENODEV;
	}
	return 0;
err:
	devm_kfree(&pdev->dev, busmon);
	return ret;
}

static int bus_monitor_remove(struct platform_device *pdev)
{
	device_remove_file(&pdev->dev, &dev_attr_bus_monitor_sys);
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
		.name = "BUS_MONITOR_TEST",
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
