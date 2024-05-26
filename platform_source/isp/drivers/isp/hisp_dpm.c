#include <asm/compiler.h>
#include <linux/compiler.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_drivers/dpm_hwmon.h>
#include <linux/io.h>
#include <linux/delay.h>
#include "hisp_internel.h"
#include "isp_ddr_map.h"

#define DPM_REG_SIZE 0x1000
#define ISP_DPM_TYPE 5 /* NOC RT SRT VID CAP*/
#define DPM_LOAD_NUM 16
#define NOC_SIGNAL_MODE       0x6BF7
#define RT_SIGNAL_MODE        0x8447
#define SRT_SIGNAL_MODE       0xFA19
#define VID_SIGNAL_MODE       0xA26B
#define CAP_SIGNAL_MODE       0x969C

#define ISP_SUB_DPM_CTRL       0x244
#define DPM_SRT_CLKEN                          (1 << 24)
#define DPM_NOC_CLKEN                          (1 << 23)
#define DPM_RT_CLKEN                           (1 << 22)
#define DPM_VID_CLKEN                          (1 << 21)
#define DPM_CAP_CLKEN                          (1 << 20)
#define SRST_DPM_SRT                           (1 << 19)
#define SRST_DPM_NOC                           (1 << 18)
#define SRST_DPM_RT                            (1 << 17)
#define SRST_DPM_VID                           (1 << 16)
#define SRST_DPM_CAP                           (1 << 15)
#define FREQ_SEL_DPM_SRT                       (12)
#define FREQ_SEL_DPM_NOC                       (9)
#define FREQ_SEL_DPM_RT                        (6)
#define FREQ_SEL_DPM_VID                       (3)
#define FREQ_SEL_DPM_CAP                       (0)


static DEFINE_MUTEX(g_dpm_isp_state_lock);

struct hisp_dpm {
	bool dpm_inited;
};

enum {
	NO_ACCUMULATION = 0,
	ACCUMULATION = 1,
};

static struct hisp_dpm g_dpm_ctrl = {
	.dpm_inited = false
};

void __iomem *noc_dpm_base;
void __iomem *rt_dpm_base;
void __iomem *srt_dpm_base;
void __iomem *vid_dpm_base;
void __iomem *cap_dpm_base;

static int hisp_dpm_update_counter_power(bool only_power, int mode);

typedef int (*call_interface_handle)(bool only_power, int mode);

static int hisp_dpm_get_counter_power(call_interface_handle call_interface,
	bool only_power, int mode)
{
	int len;
	/* if power is off, direct return
	 * if power is on, isp should wait dpm done before power off
	 */
	mutex_lock(&g_dpm_isp_state_lock);
	if (!g_dpm_ctrl.dpm_inited) {
		mutex_unlock(&g_dpm_isp_state_lock);
		return 0;
	}
	pr_info("[%s] +\n", __func__);

	/* write pulse */
	writel(BIT(SOC_DPMONITOR_SOFT_PULSE_soft_pulse_START),
		SOC_DPMONITOR_SOFT_PULSE_ADDR(noc_dpm_base));
	writel(BIT(SOC_DPMONITOR_SOFT_PULSE_soft_pulse_START),
		SOC_DPMONITOR_SOFT_PULSE_ADDR(rt_dpm_base));
	writel(BIT(SOC_DPMONITOR_SOFT_PULSE_soft_pulse_START),
		SOC_DPMONITOR_SOFT_PULSE_ADDR(srt_dpm_base));
	writel(BIT(SOC_DPMONITOR_SOFT_PULSE_soft_pulse_START),
		SOC_DPMONITOR_SOFT_PULSE_ADDR(vid_dpm_base));
	writel(BIT(SOC_DPMONITOR_SOFT_PULSE_soft_pulse_START),
		SOC_DPMONITOR_SOFT_PULSE_ADDR(cap_dpm_base));

	udelay(2); //lint !e1564

	len = call_interface(only_power, mode);
	mutex_unlock(&g_dpm_isp_state_lock);

	pr_info("[%s] -\n", __func__);
	return len;
}


int hisp_dpm_update_counter(void)
{
	return hisp_dpm_get_counter_power(hisp_dpm_update_counter_power,
		false, ACCUMULATION);
}

#ifdef CONFIG_DPM_HWMON_V2
int hisp_dpm_update_power(void)
{
	return hisp_dpm_get_counter_power(hisp_dpm_update_counter_power,
		true, ACCUMULATION);
}
#endif

#ifdef CONFIG_DPM_HWMON_DEBUG
int dpm_isp_debug_get_counter(int mode)
{
	return hisp_dpm_get_counter_power(hisp_dpm_update_counter_power,
		false, mode);
}
#endif

static int hisp_dpm_clk_open(void)
{
	void __iomem *isp_subctrl_base;
	unsigned int value;

	pr_info("[%s] +\n", __func__);

	isp_subctrl_base = hisp_dev_get_regaddr(ISP_SUBCTRL);
	if (isp_subctrl_base == NULL) {
		pr_err("[%s] Failed : isp_subctrl_base\n", __func__);
		return -ENOMEM;
	}

	value = __raw_readl(isp_subctrl_base + ISP_SUB_DPM_CTRL);
	value |= DPM_SRT_CLKEN;
	value |= DPM_NOC_CLKEN;
	value |= DPM_RT_CLKEN;
	value |= DPM_VID_CLKEN;
	value |= DPM_CAP_CLKEN;
	__raw_writel(value, isp_subctrl_base + ISP_SUB_DPM_CTRL);

	value = __raw_readl(isp_subctrl_base + ISP_SUB_DPM_CTRL);
	value &= (~SRST_DPM_SRT);
	value &= (~SRST_DPM_NOC);
	value &= (~SRST_DPM_RT);
	value &= (~SRST_DPM_VID);
	value &= (~SRST_DPM_CAP);
	__raw_writel(value, isp_subctrl_base + ISP_SUB_DPM_CTRL);

	pr_info("[%s] -\n", __func__);
	return 0;
}

static int hisp_dpm_clk_close(void)
{
	void __iomem *isp_subctrl_base;
	unsigned int value;

	pr_info("[%s] +\n", __func__);

	isp_subctrl_base = hisp_dev_get_regaddr(ISP_SUBCTRL);
	if (isp_subctrl_base == NULL) {
		pr_err("[%s] Failed : isp_subctrl_base\n", __func__);
		return -ENOMEM;
	}

	value = __raw_readl(isp_subctrl_base + ISP_SUB_DPM_CTRL);
	value |= SRST_DPM_SRT;
	value |= SRST_DPM_NOC;
	value |= SRST_DPM_RT;
	value |= SRST_DPM_VID;
	value |= SRST_DPM_CAP;
	__raw_writel(value, isp_subctrl_base + ISP_SUB_DPM_CTRL);

	value = __raw_readl(isp_subctrl_base + ISP_SUB_DPM_CTRL);
	value &= (~DPM_SRT_CLKEN);
	value &= (~DPM_NOC_CLKEN);
	value &= (~DPM_RT_CLKEN);
	value &= (~DPM_VID_CLKEN);
	value &= (~DPM_CAP_CLKEN);
	__raw_writel(value, isp_subctrl_base + ISP_SUB_DPM_CTRL);

	pr_info("[%s] -\n", __func__);
	return 0;
}

static void hisp_dpm_noc_on(void)
{
	unsigned int param[] = {
		0x000079D9, 0x00000000, 0x00000000,
		0x00B3FC41, 0x0000063B, 0x00022645,
		0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000000, 0x00000000,
		0x00000000, 0x00000472, 0x0000027D,
		0x0001E967, 0x00000000, 0x00000242,
		0x00000242, 0x00000200, 0x000001C2,
		0x00000188, 0x00000152, 0x00000120,
		0x000000F2, 0x00000200, 0x00000200,
		0x00000200, 0x00000200, 0x00000200,
		0x00000200, 0x00000200, 0x00000200,
	};

	if (noc_dpm_base == NULL)
		return;

	dpm_monitor_enable(noc_dpm_base, NOC_SIGNAL_MODE,
		param, ARRAY_SIZE(param));
}

static void hisp_dpm_rt_on(void)
{
	unsigned int param[] = {
		0x00000D4B, 0x000001F2, 0x000000F1,
		0x00000C21, 0x000001DC, 0x0000A52C,
		0x00003306, 0x000008A6, 0x00008A08,
		0x0000127D, 0x00000930, 0x0000257D,
		0x00003E74, 0x0000319A, 0x000012CE,
		0x00000000, 0x00000000, 0x00000242,
		0x00000242, 0x00000200, 0x000001C2,
		0x00000188, 0x00000152, 0x00000120,
		0x000000F2, 0x00000200, 0x00000200,
		0x00000200, 0x00000200, 0x00000200,
		0x00000200, 0x00000200, 0x00000200,
	};

	if (rt_dpm_base == NULL)
		return;

	dpm_monitor_enable(rt_dpm_base, RT_SIGNAL_MODE,
		param, ARRAY_SIZE(param));
}

static void hisp_dpm_srt_on(void)
{
	unsigned int param[] = {
		0x00000D2F, 0x00001004, 0x0002F01A,
		0x00000FDD, 0x00000000, 0x00016158,
		0x00007BCC, 0x00000218, 0x00000000,
		0x00000000, 0x000245FF, 0x00000000,
		0x000004A0, 0x00000505, 0x00000000,
		0x00000000, 0x00000000, 0x00000242,
		0x00000242, 0x00000200, 0x000001C2,
		0x00000188, 0x00000152, 0x00000120,
		0x000000F2, 0x00000200, 0x00000200,
		0x00000200, 0x00000200, 0x00000200,
		0x00000200, 0x00000200, 0x00000200,
	};

	if (srt_dpm_base == NULL)
		return;

	dpm_monitor_enable(srt_dpm_base, SRT_SIGNAL_MODE,
		param, ARRAY_SIZE(param));
}

static void hisp_dpm_vid_on(void)
{
	unsigned int param[] = {
		0x00000000, 0x00001A61, 0x00000139,
		0x00002FFD, 0x00000000, 0x00000000,
		0x00000000, 0x0000F348, 0x00000000,
		0x00000000, 0x0001DDF0, 0x0000D29E,
		0x000018DF, 0x00000000, 0x00000000,
		0x000001E9, 0x00000000, 0x00000242,
		0x00000242, 0x00000200, 0x000001C2,
		0x00000188, 0x00000152, 0x00000120,
		0x000000F2, 0x00000200, 0x00000200,
		0x00000200, 0x00000200, 0x00000200,
		0x00000200, 0x00000200, 0x00000200,
	};

	if (vid_dpm_base == NULL)
		return;

	dpm_monitor_enable(vid_dpm_base, VID_SIGNAL_MODE,
		param, ARRAY_SIZE(param));
}

static void hisp_dpm_cap_on(void)
{
	unsigned int param[] = {
		0x000021D6, 0x0000030D, 0x000004F1,
		0x0000020D, 0x000000D6, 0x00000351,
		0x00002FC7, 0x000004FA, 0x0000021C,
		0x00000016, 0x00000109, 0x00000160,
		0x000002DF, 0x0000016B, 0x000002CC,
		0x0000000C, 0x00000000, 0x00000242,
		0x00000242, 0x00000200, 0x000001C2,
		0x00000188, 0x00000152, 0x00000120,
		0x000000F2, 0x00000200, 0x00000200,
		0x00000200, 0x00000200, 0x00000200,
		0x00000200, 0x00000200, 0x00000200,
	};

	if (cap_dpm_base == NULL)
		return;

	dpm_monitor_enable(cap_dpm_base, CAP_SIGNAL_MODE,
		param, ARRAY_SIZE(param));
}

static int hisp_dpm_on(void)
{
	int ret;

	noc_dpm_base  = hisp_dev_get_regaddr(ISP_NOCDPM);
	rt_dpm_base   = noc_dpm_base + DPM_REG_SIZE;
	vid_dpm_base  = noc_dpm_base + 2*DPM_REG_SIZE;
	cap_dpm_base  = noc_dpm_base + 3*DPM_REG_SIZE;
	srt_dpm_base  = noc_dpm_base + 4*DPM_REG_SIZE;
	if (noc_dpm_base == NULL ||
		rt_dpm_base == NULL ||
		srt_dpm_base == NULL ||
		vid_dpm_base == NULL ||
		cap_dpm_base == NULL) {
		pr_err("[%s] Failed : ioremap\n", __func__);
		return -EINVAL;
	}

	ret = hisp_dpm_clk_open();
	if (ret < 0) {
		pr_err("[%s] hisp_dpm_clk_open.%d\n", __func__, ret);
		return -ENOMEM;
	}

	hisp_dpm_noc_on();

	hisp_dpm_rt_on();

	hisp_dpm_srt_on();

	hisp_dpm_vid_on();

	hisp_dpm_cap_on();

	g_dpm_ctrl.dpm_inited = 1;
	return 0;
}

static void hisp_dpm_off(void)
{
	int ret;

	dpm_monitor_disable(noc_dpm_base);
	dpm_monitor_disable(rt_dpm_base);
	dpm_monitor_disable(srt_dpm_base);
	dpm_monitor_disable(vid_dpm_base);
	dpm_monitor_disable(cap_dpm_base);

	ret = hisp_dpm_clk_close();
	if (ret < 0) {
		pr_err("[%s] dpm_isp_clk_close.%d\n", __func__, ret);
		return;
	}
	g_dpm_ctrl.dpm_inited = 0;
}

static bool hisp_dpm_is_on(void)
{
	bool state = false;

	mutex_lock(&g_dpm_isp_state_lock);
	state = g_dpm_ctrl.dpm_inited;
	mutex_unlock(&g_dpm_isp_state_lock);
	return state;
}

static void hisp_dpm_set_on(bool state)
{
	mutex_lock(&g_dpm_isp_state_lock);
	g_dpm_ctrl.dpm_inited = state;
	mutex_unlock(&g_dpm_isp_state_lock);
}

static struct dpm_hwmon_ops hisp_dpm_ops = {
	.dpm_module_id = DPM_ISP_ID,
	.dpm_type = DPM_PERI_MODULE,
	.hi_dpm_update_counter = hisp_dpm_update_counter,
	.dpm_cnt_len = DPM_LOAD_NUM * ISP_DPM_TYPE,
#ifdef CONFIG_DPM_HWMON_V2
	.dpm_power_len = ISP_DPM_TYPE,
	.hi_dpm_update_power = hisp_dpm_update_power,
#endif

#ifdef CONFIG_DPM_HWMON_DEBUG
	.hi_dpm_get_counter_for_fitting = dpm_isp_debug_get_counter,
#endif
};

static int hisp_dpm_update_counter_power(bool only_power, int mode)
{
	int len;
	unsigned long long *dpm_ctable = hisp_dpm_ops.dpm_counter_table;
	unsigned long long *dpm_ptable = hisp_dpm_ops.dpm_power_table;

	pr_info("[%s] +\n", __func__);

	if (only_power) {
		len = dpm_hwmon_update_power(noc_dpm_base,
					dpm_ptable, mode);
		len += dpm_hwmon_update_power(rt_dpm_base,
					dpm_ptable + 1, mode);
		len += dpm_hwmon_update_power(srt_dpm_base,
					dpm_ptable + 2, mode);
		len += dpm_hwmon_update_power(vid_dpm_base,
					dpm_ptable + 3, mode);
		len += dpm_hwmon_update_power(cap_dpm_base,
					dpm_ptable + 4, mode);
	} else {
		len = dpm_hwmon_update_counter_power(noc_dpm_base,
			dpm_ctable, DPM_LOAD_NUM, dpm_ptable, mode);
		len += dpm_hwmon_update_counter_power(rt_dpm_base,
			dpm_ctable + 1*DPM_LOAD_NUM, DPM_LOAD_NUM,
			dpm_ptable + 1, mode);
		len += dpm_hwmon_update_counter_power(srt_dpm_base,
			dpm_ctable + 2*DPM_LOAD_NUM, DPM_LOAD_NUM,
			dpm_ptable + 2, mode);
		len += dpm_hwmon_update_counter_power(vid_dpm_base,
			dpm_ctable + 3*DPM_LOAD_NUM, DPM_LOAD_NUM,
			dpm_ptable + 3, mode);
		len += dpm_hwmon_update_counter_power(cap_dpm_base,
			dpm_ctable + 4*DPM_LOAD_NUM, DPM_LOAD_NUM,
			dpm_ptable + 4, mode);
	}
	pr_info("[%s] -\n", __func__);

	return len;
}

void hisp_dpm_probe(void)
{
#ifdef DEBUG_HISP
	int stat;

	stat = hisp_check_pcie_stat();
	if (stat != ISP_NORMAL_MODE)
		return;
#endif

	if (!hisp_dpm_ops.module_enabled) {
		pr_err("[%s] hisp_dpm_ops.module_enabled = %d\n",
			__func__, hisp_dpm_ops.module_enabled);
		return;
	}

	if (hisp_dpm_is_on()) {
		pr_err("[%s] hisp_dpm_is_on already open.\n", __func__);
		return;
	}

	/* enable dpm */
	if (hisp_dpm_on()) {
		pr_err("[%s] hisp_dpm_on error.\n", __func__);
		return;
	}

	hisp_dpm_set_on(true);
	pr_info("%s dpm_isp is successfully init\n", __func__);
}

void hisp_dpm_release(void)
{
#ifdef DEBUG_HISP
	int stat;

	stat = hisp_check_pcie_stat();
	if (stat != ISP_NORMAL_MODE)
		return;
#endif

	int ret;

	if (!hisp_dpm_is_on()) {
		pr_err("%s dpm_isp is not inited\n", __func__);
		return;
	}
	ret = hisp_dpm_update_power();
	if (!ret) {
		pr_err("%s hisp_dpm_update_power error\n", __func__);
		return;
	}

	/* ready to disable dpm, set flag to be off to avoid read reg */
	hisp_dpm_set_on(false);
	hisp_dpm_off();

	pr_info("%s dpm_isp is successfully deinit\n", __func__);
}

static int __init hisp_dpm_init(void)
{
	dpm_hwmon_register(&hisp_dpm_ops);
	return 0;
}

static void __exit hisp_dpm_exit(void)
{
	dpm_hwmon_unregister(&hisp_dpm_ops);
}

module_init(hisp_dpm_init);
module_exit(hisp_dpm_exit);
