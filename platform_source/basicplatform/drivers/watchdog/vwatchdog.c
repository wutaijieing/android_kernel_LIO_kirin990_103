/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 *
 * vwatchdog functions moudle
 *
 * vwatchdog.c
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
#include <platform_include/basicplatform/linux/dfx_universal_wdt.h>
#include <linux/watchdog.h>
#include <asm/sysreg.h>
#include <linux/../../kernel/sched/sched.h>
#include <linux/../../kernel/workqueue_internal.h>
#include <linux/bitops.h>
#include <linux/device.h>
#include <platform_include/basicplatform/linux/pr_log.h>
#include <platform_include/basicplatform/linux/rdr_platform.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include <platform_include/basicplatform/linux/util.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/math64.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/resource.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/syscore_ops.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <securec.h>
#include <mntn_subtype_exception.h>
#include <vsoc_sctrl_interface.h>

#define VWDT_OVERLOAD_VALUE_MIN         0x00000001
#define VWDT_OVERLOAD_VALUE_MAX         0xFFFFFFFF
#define WDG_TIMEOUT_ADJUST      2
#define WDG_FEED_MOMENT_ADJUST  3
#define MODULE_NAME             "vwdt"
#define PR_LOG_TAG V_WDT_TAG

#define WDCONTROL_ADDR_OFFSET  (VSOC_VWATCHDOG_ENABLE_REG_ADDR - VSOC_VWATCHDOG_ENABLE_REG_ADDR)
#define WDLOAD_ADDR_OFFSET     (VSOC_VWATCHDOG_LOADER_REG_ADDR - VSOC_VWATCHDOG_ENABLE_REG_ADDR)
#define WDVALUE_ADDR_OFFSET    (VSOC_VWATCHDOG_COUNT_REG_ADDR - VSOC_VWATCHDOG_ENABLE_REG_ADDR)

#define WDCONTROL_ENABLE VWATCHDOG_ENABLE_MAGIC
#define WDCONTROL_DISABLE VWATCHDOG_DISABLE_MAGIC

#define WATCHDOG_INT_NUMBER (403 + 32)
#define DEFAULT_TIMEOUT 60
#define SECS_TO_MSECS  1000
#define ENABLE_VWDT 1
#define DISABLE_VWDT 2
#define FEED_VWDT 3
#define VWATCHDOG_MNTN_NAME "hisilicon,mntn_vwatchdog"

struct v_wdt {
	struct watchdog_device wdd;
	spinlock_t lock;
	void __iomem *base;
	unsigned int loader_val;
	unsigned int kick_time;
	struct delayed_work dfx_wdt_delayed_work;
	struct workqueue_struct *dfx_wdt_wq;
	bool active;
	unsigned int timeout;
};

struct rdr_arctimer_s g_rdr_arctimer_record;
static struct v_wdt *g_vwdt = NULL;
static bool vwdt_feed_flag;

static bool nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, bool, 0);
MODULE_PARM_DESC(nowayout,
		"Set to 1 to keep watchdog running after device release");

/* This routine finds load value that will reset system in required timout */
static int vwdt_setload(struct watchdog_device *wdd, unsigned int timeout)
{
	struct v_wdt *v_wdt = NULL;
	u32 load = timeout;

	if (wdd == NULL)
		return -1;

	v_wdt = watchdog_get_drvdata(wdd);
	if (v_wdt == NULL)
		return -1;

	/*
	 * vwatchdog runs counter with given value twice, after the end of first
	 * counter it gives an interrupt and then starts counter again. If
	 * interrupt already occurred then it resets the system. This is why
	 * load is half of what should be required.
	 */
	v_wdt->wdd.timeout = timeout;

	do_div(load, WDG_TIMEOUT_ADJUST);
	load = (load < VWDT_OVERLOAD_VALUE_MIN) ? VWDT_OVERLOAD_VALUE_MIN : load;

	spin_lock(&v_wdt->lock);
	v_wdt->loader_val = load;
	v_wdt->timeout = timeout;
	spin_unlock(&v_wdt->lock);

	pr_info("%s default-timeout=%u, loader_val=0x%x\n", __func__, timeout, v_wdt->loader_val);

	return 0;
}

/* returns number of seconds left for reset to occur */
static unsigned int vwdt_timeleft(struct watchdog_device *wdd)
{
	struct v_wdt *v_wdt = NULL;
	u32 load;

	if (wdd == NULL)
		return DEFAULT_TIMEOUT;

	v_wdt = watchdog_get_drvdata(wdd);
	if ((v_wdt == NULL) || (v_wdt->base == NULL))
		return DEFAULT_TIMEOUT;

	spin_lock(&v_wdt->lock);
	load = readl(v_wdt->base + WDVALUE_ADDR_OFFSET);
	spin_unlock(&v_wdt->lock);

	return load;
}

static int wdt_config(struct watchdog_device *wdd, int conf)
{
	struct v_wdt *v_wdt = NULL;
	int vwdt_conf = conf;

	if (!vwdt_feed_flag)
		return 0;

	if (wdd == NULL)
		return -1;

	v_wdt = watchdog_get_drvdata(wdd);
	if (v_wdt == NULL)
		return -1;

	switch (vwdt_conf) {
	case ENABLE_VWDT:
		spin_lock(&v_wdt->lock);
		writel(v_wdt->loader_val, v_wdt->base + WDLOAD_ADDR_OFFSET);
		writel(WDCONTROL_ENABLE, v_wdt->base + WDCONTROL_ADDR_OFFSET);
		v_wdt->active = true;
		spin_unlock(&v_wdt->lock);
		break;
	case DISABLE_VWDT:
		spin_lock(&v_wdt->lock);
		writel(WDCONTROL_DISABLE, v_wdt->base + WDCONTROL_ADDR_OFFSET);
		v_wdt->active = false;
		spin_unlock(&v_wdt->lock);
		break;
	case FEED_VWDT:
		spin_lock(&v_wdt->lock);
		writel(v_wdt->loader_val, v_wdt->base + WDLOAD_ADDR_OFFSET);
		spin_unlock(&v_wdt->lock);
		break;
	default:
		pr_err("%s conf arg is error\n", __func__);
		break;
	}

	return 0;
}

static int vwdt_ping(struct watchdog_device *wdd)
{
	return wdt_config(wdd, FEED_VWDT);
}

/* enables watchdog timers reset */
static int vwdt_enable(struct watchdog_device *wdd)
{
	return wdt_config(wdd, ENABLE_VWDT);
}

/* disables watchdog timers reset */
static int vwdt_disable(struct watchdog_device *wdd)
{
	return wdt_config(wdd, DISABLE_VWDT);
}

static const struct watchdog_info wdt_info = {
	.options = WDIOF_MAGICCLOSE | WDIOF_SETTIMEOUT | WDIOF_KEEPALIVEPING,
	.identity = MODULE_NAME,
};

static const struct watchdog_ops wdt_ops = {
	.owner          = THIS_MODULE,
	.start          = vwdt_enable,
	.stop           = vwdt_disable,
	.ping           = vwdt_ping,
	.set_timeout    = vwdt_setload,
	.get_timeleft   = vwdt_timeleft,
};

void watchdog_shutdown_oneshot(unsigned int timeout)
{
	struct v_wdt *v_wdt = g_vwdt;
	unsigned int timeout_rst;
	int ret;

	if (v_wdt == NULL) {
		pr_err("%s v_wdt device not init\n", __func__);
		return;
	}
	if ((timeout < v_wdt->kick_time) || (timeout > DEFAULT_TIMEOUT)) {
		pr_err("%s timeout invalid[%u, %u]\n", __func__, timeout, v_wdt->kick_time);
		return;
	}
	cancel_delayed_work_sync(&v_wdt->dfx_wdt_delayed_work);
	vwdt_disable(&v_wdt->wdd);

	timeout_rst = timeout * WDG_TIMEOUT_ADJUST;
	vwdt_setload(&v_wdt->wdd, timeout_rst);
	ret = vwdt_enable(&v_wdt->wdd);
	if (ret < 0) {
		pr_err("%s enable failed\n", __func__);
		return;
	}
	pr_err("%s set %u seconds\n", __func__, timeout);
}

unsigned long get_wdt_expires_time(void)
{
	if (g_vwdt == NULL)
		return 0;

	return g_vwdt->dfx_wdt_delayed_work.timer.expires;
}

unsigned int get_wdt_kick_time(void)
{
	if (g_vwdt == NULL)
		return 0;

	return g_vwdt->kick_time;
}

static int v_wdt_suspend(void)
{
	struct v_wdt *v_wdt = g_vwdt;
	int ret = -1;

	if (v_wdt == NULL) {
		pr_err("%s wdt device not init\n", __func__);
		return ret;
	}
	pr_info("%s+\n", __func__);

	if (watchdog_active(&v_wdt->wdd) || v_wdt->active) {
		cancel_delayed_work(&v_wdt->dfx_wdt_delayed_work);
		ret = vwdt_disable(&v_wdt->wdd);
	}

	pr_info("%s-, ret=%d\n", __func__, ret);

	return ret;
}

static void v_wdt_resume(void)
{
	struct v_wdt *v_wdt = g_vwdt;
	int ret = -1;

	if (v_wdt == NULL) {
		pr_err("%s wdt device not init\n", __func__);
		return;
	}
	pr_info("%s+\n", __func__);

	if (watchdog_active(&v_wdt->wdd) || (v_wdt->active == false)) {
		ret = vwdt_enable(&v_wdt->wdd);
		if (cpu_online(0))
			queue_delayed_work_on(0, v_wdt->dfx_wdt_wq, &v_wdt->dfx_wdt_delayed_work, 0);
		else
			queue_delayed_work(v_wdt->dfx_wdt_wq, &v_wdt->dfx_wdt_delayed_work, 0);
	}
	pr_info("%s-, ret=%d\n", __func__, ret);
}

static struct syscore_ops v_wdt_syscore_ops = {
	.suspend = v_wdt_suspend,
	.resume = v_wdt_resume
};

void rdr_arctimer_register_read(struct rdr_arctimer_s *arctimer)
{
	return;
}
void rdr_archtime_register_print(struct rdr_arctimer_s *arctimer, bool after)
{
	return;
}

static int v_wdt_rdr_init(void)
{
	struct rdr_exception_info_s einfo;
	unsigned int ret;
	errno_t errno;

	(void)memset_s(&einfo, sizeof(einfo), 0, sizeof(einfo));
	einfo.e_modid = MODID_AP_S_VWDT;
	einfo.e_modid_end = MODID_AP_S_VWDT;
	einfo.e_process_priority = RDR_ERR;
	einfo.e_reboot_priority = RDR_REBOOT_NOW;
	einfo.e_notify_core_mask = RDR_AP;
	einfo.e_reset_core_mask = RDR_AP;
	einfo.e_from_core = RDR_AP;
	einfo.e_reentrant = (u32)RDR_REENTRANT_DISALLOW;
	einfo.e_exce_type = AP_S_VWDT;
	einfo.e_exce_subtype = 0;
	einfo.e_upload_flag = (u32)RDR_UPLOAD_YES;
	errno = memcpy_s(einfo.e_from_module, sizeof(einfo.e_from_module),
					"ap vwdt", sizeof("ap vwdt"));
	if (errno != EOK) {
		pr_err("%s exception info module memcpy_s failed\n", __func__);
		return -1;
	}
	errno = memcpy_s(einfo.e_desc, sizeof(einfo.e_desc), "ap vwdt", sizeof("ap vwdt"));
	if (errno != EOK) {
		pr_err("%s exception info desc memcpy_s failed\n", __func__);
		return -1;
	}
	ret = rdr_register_exception(&einfo);
	if (ret != MODID_AP_S_VWDT)
		return -1;

	return 0;
}

static BLOCKING_NOTIFIER_HEAD(v_wdt_notifier_list);
void wdt_register_notify(struct notifier_block *nb)
{
	blocking_notifier_chain_register(&v_wdt_notifier_list, nb);
}

void wdt_unregister_notify(struct notifier_block *nb)
{
	blocking_notifier_chain_unregister(&v_wdt_notifier_list, nb);
}

void dfx_wdt_dump(void)
{
	struct v_wdt *v_wdt = g_vwdt;
	struct timer_list *timer = NULL;

	blocking_notifier_call_chain(&v_wdt_notifier_list, 0, NULL);

#ifdef CONFIG_SCHED_INFO
	if (current != NULL) {
		pr_crit("current process last_arrival clock %llu last_queued clock %llu, ",
			current->sched_info.last_arrival, current->sched_info.last_queued);
		pr_crit("printk_time is %llu, 32k_abs_timer_value is %llu\n", dfx_getcurtime(), get_32k_abs_timer_value());
	}
#endif

	if (v_wdt == NULL)
		return;

	pr_crit("work_busy 0x%x, latest kick slice %llu\n",
		work_busy(&(v_wdt->dfx_wdt_delayed_work.work)),
		rdr_get_last_wdt_kick_slice());

	/* check if the watchdog work timer in running state */
	timer = &(v_wdt->dfx_wdt_delayed_work.timer);
	pr_crit("timer 0x%pK: next 0x%pK pprev 0x%pK expires %lu jiffies %lu sec_to_jiffies %lu flags 0x%x\n",
		timer, timer->entry.next, timer->entry.pprev, timer->expires,
		jiffies, msecs_to_jiffies(SECS_TO_MSECS), timer->flags);
}

static void dfx_wdt_mond(struct work_struct *work)
{
	struct v_wdt *v_wdt = NULL;

	if (work == NULL)
		return;
	v_wdt = container_of(work, struct v_wdt, dfx_wdt_delayed_work.work);

	vwdt_ping(&v_wdt->wdd);

	if (cpu_online(0))
		queue_delayed_work_on(0, v_wdt->dfx_wdt_wq,
					&v_wdt->dfx_wdt_delayed_work,
					msecs_to_jiffies(v_wdt->kick_time * SECS_TO_MSECS));
	else
		queue_delayed_work(v_wdt->dfx_wdt_wq,
					&v_wdt->dfx_wdt_delayed_work,
					msecs_to_jiffies(v_wdt->kick_time * SECS_TO_MSECS));

	pr_info("%s vwatchdog kick now \n", __func__);

	watchdog_check_hardlockup_hiwdt();
}

static irqreturn_t vwatchdog_irq_handle(int irq, void *ptr)
{
	pr_err("vwatchdog irq process start\n");
	dfx_wdt_dump();
	rdr_syserr_process_for_ap((u32)MODID_AP_S_VWDT, 0ULL, 0ULL);
	pr_err("vwatchdog irq process end\n");
	return IRQ_HANDLED;
}

static int vwatchdog_irq_register(struct device_node *dev_node)
{
	u32 irq;

	irq = irq_of_parse_and_map(dev_node, 0);
	if (irq == 0) {
		pr_err("%s get irq failed\n", __func__);
		return -EBUSY;
	}

	return request_irq(irq, vwatchdog_irq_handle, IRQF_TRIGGER_HIGH, "mntn_vwatchdog", NULL);
}

static int vwatchdog_reg_remap(struct device_node *dev_node, struct v_wdt *v_wdt)
{
	int ret;
	unsigned int vwatchdog_reg_base;
	unsigned int vwatchdog_reg_size;

	ret = of_property_read_u32_index(dev_node, "reg", 1, &vwatchdog_reg_base);
	if (ret) {
		pr_err("%s failed to get vwatchdog_reg_base resource! ret=%d\n", __func__, ret);
		return -ENXIO;
	}
	ret = of_property_read_u32_index(dev_node, "reg", 3, &vwatchdog_reg_size);
	if (ret) {
		pr_err("%s failed to get vwatchdog_reg_size resource! ret=%d\n", __func__, ret);
		return -ENXIO;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	v_wdt->base = ioremap(vwatchdog_reg_base, vwatchdog_reg_size);
#else
	v_wdt->base = ioremap_nocache(vwatchdog_reg_base, vwatchdog_reg_size);
#endif
	if (IS_ERR(v_wdt->base)) {
		pr_err("%s base is error\n", __func__);
		return -ENOMEM;
	}

	return 0;
}

static unsigned int vwatchdog_get_timeout(struct device_node *dev_node)
{
	int ret;
	unsigned int default_timeout;

	ret = of_property_read_u32_index(dev_node, "default-timeout", 0, &default_timeout);
	if (ret) {
		pr_err("find default-timeout property fail, Use the default value: %us\n", DEFAULT_TIMEOUT);
		return  DEFAULT_TIMEOUT;
	}

	return default_timeout;
}
/* for ecall test vwatchdog interrupter */
void stop_vwdt_feed(void)
{
	vwdt_feed_flag = false;
}

static int v_wdt_dev_init(struct v_wdt **v_wdt, unsigned int *default_timeout)
{
	int ret = 0;
	struct device_node *dev_node = NULL;

	pr_info("%s enter\n", __func__);
	dev_node = of_find_compatible_node(NULL, NULL, VWATCHDOG_MNTN_NAME);
	if (!dev_node) {
		pr_err("%s find device node %s by compatible failed\n", __func__, VWATCHDOG_MNTN_NAME);
		return -EBUSY;
	}

	*v_wdt = kzalloc(sizeof(struct v_wdt), GFP_KERNEL);
	if (*v_wdt == NULL) {
		pr_err("%s Kzalloc failed\n", __func__);
		return -ENOMEM;
	}

	ret = vwatchdog_irq_register(dev_node);
	if (ret) {
		pr_err("%s request irq failed, return %d\n", __func__, ret);
		goto v_wdt_free;
	}

	ret = vwatchdog_reg_remap(dev_node, *v_wdt);
	if (ret) {
		pr_err("%s remap vsoc registers failed\n", __func__);
		goto v_wdt_free;
	}

	*default_timeout = vwatchdog_get_timeout(dev_node);

	(*v_wdt)->wdd.info = &wdt_info;
	(*v_wdt)->wdd.ops = &wdt_ops;

	spin_lock_init(&((*v_wdt)->lock));
	watchdog_set_nowayout(&((*v_wdt)->wdd), nowayout);
	watchdog_set_drvdata(&((*v_wdt)->wdd), *v_wdt);

	pr_info("%s device init successful\n", __func__);
	return 0;
v_wdt_free:
	kfree(*v_wdt);
	*v_wdt = NULL;
	pr_err("%s device init Failed!!!\n", __func__);
	return ret;
}

static int __init vwatchdog_init(void)
{
	struct v_wdt *v_wdt = NULL;
	int ret;
	unsigned int default_timeout = DEFAULT_TIMEOUT;
	vwdt_feed_flag = true;

	if (check_mntn_switch(MNTN_NON_RDA_WDT) == 0) {
		pr_err("%s vwatchdog is closed in nv!!!\n", __func__);
		return 0;
	}

	ret = v_wdt_dev_init(&v_wdt, &default_timeout);
	if (ret)
		goto init_fail;

	vwdt_setload(&v_wdt->wdd, default_timeout);

	if ((default_timeout >> 1) < WDG_FEED_MOMENT_ADJUST)
		v_wdt->kick_time = (default_timeout >> 1) - 1;
	else
		v_wdt->kick_time = ((default_timeout >> 1) - 1) / WDG_FEED_MOMENT_ADJUST; /* minus 1 from the total */

	vwdt_ping(&v_wdt->wdd);

	INIT_DELAYED_WORK(&v_wdt->dfx_wdt_delayed_work, dfx_wdt_mond);

	v_wdt->dfx_wdt_wq = alloc_workqueue("v_wdt_wq", WQ_MEM_RECLAIM | WQ_HIGHPRI, 1);
	if (v_wdt->dfx_wdt_wq == NULL) {
		pr_err("%s alloc workqueue failed\n", __func__);
		ret = -ENOMEM;
		goto dev_destory;
	}

	ret = watchdog_register_device(&v_wdt->wdd);
	if (ret) {
		pr_err("%s watchdog_register_device() failed: %d\n", __func__, ret);
		goto workqueue_destroy;
	}

	vwdt_enable(&v_wdt->wdd);
	if (cpu_online(0))
		queue_delayed_work_on(0, v_wdt->dfx_wdt_wq, &v_wdt->dfx_wdt_delayed_work, 0);
	else
		queue_delayed_work(v_wdt->dfx_wdt_wq, &v_wdt->dfx_wdt_delayed_work, 0);

	register_syscore_ops(&v_wdt_syscore_ops);

	if (v_wdt_rdr_init())
		pr_err("%s register v_wdt_rdr_init failed\n", __func__);

	watchdog_set_thresh((int)(default_timeout >> 1));

	g_vwdt = v_wdt;
	pr_info("%s registration successful\n", __func__);

	return 0;

workqueue_destroy:
	if (v_wdt->dfx_wdt_wq != NULL) {
		destroy_workqueue(v_wdt->dfx_wdt_wq);
		v_wdt->dfx_wdt_wq = NULL;
	}
dev_destory:
	if (v_wdt != NULL) {
		kfree(v_wdt);
		v_wdt = NULL;
	}
init_fail:
	pr_err("%s Init Failed!!!\n", __func__);
	return ret;
}

static void __exit vwatchdog_exit(void)
{
	if (g_vwdt == NULL)
		return;

	vwdt_disable(&g_vwdt->wdd);
	rdr_unregister_exception(MODID_AP_S_VWDT);
	unregister_syscore_ops(&v_wdt_syscore_ops);
	cancel_delayed_work(&g_vwdt->dfx_wdt_delayed_work);
	destroy_workqueue(g_vwdt->dfx_wdt_wq);
	g_vwdt->dfx_wdt_wq = NULL;
	watchdog_unregister_device(&g_vwdt->wdd);
	watchdog_set_drvdata(&g_vwdt->wdd, NULL);
	iounmap(g_vwdt->base);
	kfree(g_vwdt);
	g_vwdt = NULL;

	return;
}
module_init(vwatchdog_init);
module_exit(vwatchdog_exit);
MODULE_LICENSE("GPL v2");
