/*
 * record the data to rdr. (RDR: kernel run data recorder.)
 * This file wraps the ring buffer.
 *
 * Copyright (c) 2013 ISP Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/printk.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/version.h>
#include <linux/of_irq.h>
#include <securec.h>
#include <hisp_internel.h>
#include "isp_ddr_map.h"
#include "rdr_print.h"
#include "rdr_inner.h"
#include "rdr_field.h"
#ifndef CONFIG_HISP_EXCLUDE_NOC_DEBUG
#include "soc_mid.h"
#ifdef CONFIG_DFX_NOC
#include <platform_include/basicplatform/linux/dfx_noc_modid_para.h>
#endif
#endif

#define SOC_MID_MASK 0x3F

/* Watchdog Regs Offset */
#define ISP_WATCHDOG_WDG_LOCK_ADDR      (0x0C00)
#define ISP_WATCHDOG_WDG_CONTROL_ADDR   (0x0008)
#define ISP_RDR_MAX_SIZE                64
#define ISP_RDR_TMP_SIZE                0x1000

/* SCtrl Regs Offset */
#define SCTRL_SCBAKDATA10_ADDR          (0x434)

/* PCtrl Regs Offset */
#define PCTRL_PERI_CTRL27_ADDR          (0x070)

#define RDR_ISP_MAGIC                   0x66668888
#define RDR_ISP_SYNC                    0x88886666
#define RDR_SYNC_WORD_OFF               0x4

#define ISP_WDT_IRQ                     304
#define MODULE_NAME                     "RDR ISP"
#define SC_WDT_OFFSET                   0x33c
#define SC_ISP_WDT_BIT                  6
#define WDT_LOCK                        0x1
#define WDT_UNLOCK                      0x1acce551

#define ISPCPU_RDR_RESERVE_30           (1 << 30)
#define ISPCPU_RDR_RESERVE_29           (1 << 29)
#define ISPCPU_RDR_RESERVE_28           (1 << 28)
#define ISPCPU_RDR_RESERVE_27           (1 << 27)
#define ISPCPU_RDR_RESERVE_26           (1 << 26)
#define ISPCPU_RDR_RESERVE_25           (1 << 25)
#define ISPCPU_RDR_RESERVE_24           (1 << 24)
#define ISPCPU_RDR_RESERVE_23           (1 << 23)
#define ISPCPU_RDR_RESERVE_22           (1 << 22)
#define ISPCPU_RDR_RESERVE_21           (1 << 21)
#define ISPCPU_RDR_RESERVE_20           (1 << 20)
#define ISPCPU_RDR_RESERVE_19           (1 << 19)
#define ISPCPU_RDR_RESERVE_18           (1 << 18)
#define ISPCPU_RDR_RESERVE_17           (1 << 17)
#define ISPCPU_RDR_RESERVE_16           (1 << 16)
#define ISPCPU_RDR_RESERVE_15           (1 << 15)
#define ISPCPU_RDR_RESERVE_14           (1 << 14)
#define ISPCPU_RDR_RESERVE_13           (1 << 13)
#define ISPCPU_RDR_RESERVE_12           (1 << 12)
#define ISPCPU_RDR_RESERVE_11           (1 << 11)
#define ISPCPU_RDR_RESERVE_10           (1 << 10)
#define ISPCPU_RDR_RESERVE_9            (1 << 9)
#define ISPCPU_RDR_RESERVE_8            (1 << 8)
#define ISPCPU_RDR_RESERVE_7            (1 << 7)
#define ISPCPU_RDR_LEVEL_CPUP           (1 << 6)
#define ISPCPU_RDR_LEVEL_TASK           (1 << 5)
#define ISPCPU_RDR_LEVEL_IRQ            (1 << 4)
#define ISPCPU_RDR_LEVEL_CVDR           (1 << 3)
#define ISPCPU_RDR_LEVEL_ALGO           (1 << 2)
#define ISPCPU_RDR_LEVEL_LAST_WORD      (1 << 1)
#define ISPCPU_RDR_LEVEL_TRACE          (1 << 0)
#define ISPCPU_RDR_LEVEL_MASK           (0x7FFFFFFF)

enum hisp_rdr_syserr_type {
	ISP_MODID_START = DFX_BB_MOD_ISP_START,
	ISP_WDT_TIMEOUT = 0x81fffdfe,
	ISP_SYSTEM_STATES,
	ISP_MODID_END = DFX_BB_MOD_ISP_END,
	ISP_SYSTEM_INVALID,
} rdr_isp_sys_err_t;

struct hisp_rdr_err_type {
	struct list_head node;
	enum hisp_rdr_syserr_type type;
};

struct hisp_rdr_syserr_dump_info {
	struct list_head node;
	u32 modid;
	u64 coreid;
	pfn_cb_dump_done cb;
};

struct hisp_rdr_device {
	void __iomem *sctrl_addr;
	void __iomem *wdt_addr;
	void __iomem *pctrl_addr;
	void __iomem *rdr_addr;
	struct workqueue_struct *wq;
	struct work_struct err_work;
	struct work_struct dump_work;
	struct list_head err_list;
	struct list_head dump_list;
	spinlock_t err_list_lock;
	spinlock_t dump_list_lock;
	int wdt_irq;
	bool wdt_enable_flag;
	unsigned int offline_rdrlevel;
	unsigned char irq[IRQ_NUM];
	int isprdr_initialized;
	u64 isprdr_addr;
	u64 core_id;
	struct rdr_register_module_result current_info;
} hisp_rdr_dev;

static struct rdr_exception_info_s exc_info[] = {
	[0] = {
		.e_modid = ISP_WDT_TIMEOUT,
		.e_modid_end = ISP_WDT_TIMEOUT,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_AP | RDR_ISP,
		.e_reset_core_mask = RDR_AP,
		.e_reentrant = RDR_REENTRANT_DISALLOW,
		.e_exce_type = ISP_S_ISPWD,
		.e_from_core = RDR_ISP,
		.e_from_module = MODULE_NAME,
		.e_desc = "RDR ISP WDT TIMEOUT",
	},
	[1] = {
		.e_modid = ISP_SYSTEM_STATES,
		.e_modid_end = ISP_SYSTEM_STATES,
		.e_process_priority = RDR_ERR,
		.e_reboot_priority = RDR_REBOOT_NO,
		.e_notify_core_mask = RDR_AP | RDR_ISP,
		.e_reset_core_mask = RDR_AP,
		.e_reentrant = RDR_REENTRANT_DISALLOW,
		.e_exce_type = ISP_S_EXCEPTION,
		.e_from_core = RDR_ISP,
		.e_from_module = MODULE_NAME,
		.e_desc = "RDR ISP NOC",
	},
};

#ifndef CONFIG_HISP_EXCLUDE_NOC_DEBUG
#ifdef CONFIG_DFX_NOC
struct noc_err_para_s ispnoc_err[] = {
	[0] = {
		.masterid = SOC_ISPCORE_MID & SOC_MID_MASK,
		.targetflow = ISP_TARGETFLOW,
		.bus = NOC_ERRBUS_VIVO,
	},
	[1] = {
		.masterid = SOC_ISPCPU_MID & SOC_MID_MASK,
		.targetflow = ISP_TARGETFLOW,
		.bus = NOC_ERRBUS_VIVO,
	},
};
#endif
#endif

static int hisp_rdr_get_wdt_irq(void)
{
	struct device_node *np = NULL;
	char *name = "hisilicon,isp";
	int irq = 0;

	np = of_find_compatible_node(NULL, NULL, name);
	if (!np) {
		pr_err("%s: of_find_compatible_node failed, %s\n",
				__func__, name);
		return -ENXIO;
	}

	irq = (int)irq_of_parse_and_map(np, 0);
	if (!irq) {
		pr_err("%s: irq_of_parse_and_map failed\n", __func__);
		return -ENXIO;
	}

	pr_info("%s: comp.%s, wdt irq.%d.\n", __func__, name, irq);
	return irq;
}

u64 hisp_rdr_addr_get(void)
{
	struct hisp_rdr_device *dev = &hisp_rdr_dev;

	return dev->isprdr_addr;
}

void __iomem *hisp_rdr_va_get(void)
{
	struct hisp_rdr_device *dev = &hisp_rdr_dev;

	return dev->rdr_addr;
}

static void hisp_rdr_syserr_dump_work(struct work_struct *work)
{
	struct hisp_rdr_device *dev = &hisp_rdr_dev;
	struct hisp_rdr_syserr_dump_info *entry = NULL, *tmp = NULL;
	unsigned int sync;
	int timeout = 20;

	list_for_each_entry_safe(entry, tmp, &dev->dump_list, node) {
		if (entry->modid == ISP_WDT_TIMEOUT) {
			/* check sync word */
			do {
				sync = readl(dev->rdr_addr + RDR_SYNC_WORD_OFF);
				msleep(100);
			} while (sync != RDR_ISP_SYNC && timeout-- > 0);
			pr_info("%s: sync_word = 0x%x, timeout = %d.\n",
					__func__, sync, timeout);
		} else if (entry->modid == ISP_SYSTEM_STATES) {
			pr_info("%s: isp noc arrived.\n", __func__);
		}

		entry->cb(entry->modid, entry->coreid);

		spin_lock(&dev->dump_list_lock);
		list_del(&entry->node);
		spin_unlock(&dev->dump_list_lock);

		kfree(entry);
	}
}
/*lint -save -e429*/
static void hisp_rdr_dump(u32 modid, u32 etype, u64 coreid,
		char *pathname, pfn_cb_dump_done pfn_cb)
{
	struct hisp_rdr_device *dev = &hisp_rdr_dev;
	struct hisp_rdr_syserr_dump_info *dump_info = NULL;

	pr_info("%s: enter.\n", __func__);
	dump_info = kzalloc(sizeof(struct hisp_rdr_syserr_dump_info), GFP_KERNEL);
	if (dump_info == NULL) {
		pr_err("%s: kzalloc failed.\n", __func__);
		return;
	}

	dump_info->modid = modid;
	dump_info->coreid = dev->core_id;
	dump_info->cb = pfn_cb;

	spin_lock(&dev->dump_list_lock);
	list_add_tail(&dump_info->node, &dev->dump_list);
	spin_unlock(&dev->dump_list_lock);

	queue_work(dev->wq, &dev->dump_work);
	pr_info("%s: exit.\n", __func__);
}
/*lint -restore */
static void hisp_rdr_reset(u32 modid, u32 etype, u64 coreid)
{
	pr_info("%s: enter.\n", __func__);
}

/*lint -save -e429*/
void hisp_send_fiq2ispcpu(void)
{
	struct hisp_rdr_device *dev = &hisp_rdr_dev;
	u32 exc_flag = 0x0;
	u32 value = 0x0;
	bool wdt_flag = false;
	int status = ISP_NORMAL_MODE;
	int ret = 0;

	exc_flag = hisp_share_get_exc_flag();
	if (exc_flag == ISP_MAX_NUM_MAGIC) {
		pr_err("%s: hisp_share_get_exc_flag failed, exc_flag.0x%x\n",
				__func__, exc_flag);
		return;
	}

	pr_info("%s: exc_flag.0x%x\n", __func__, exc_flag);
	if (hisp_wait_excflag_timeout(ISP_CPU_POWER_DOWN, 10) == 0x0) {
#ifdef DEBUG_HISP
		status = hisp_check_pcie_stat();
#endif
		switch (status) {
		case ISP_NORMAL_MODE:
			if (dev->sctrl_addr == NULL) {
				pr_err("%s: sctrl_addr NULL\n", __func__);
				return;
			}

			value = readl(dev->sctrl_addr + SCTRL_SCBAKDATA10_ADDR);
			if (value & (1 << SC_ISP_WDT_BIT))
				wdt_flag = true;
			break;
#ifdef DEBUG_HISP
		case ISP_PCIE_MODE:
			wdt_flag = true;
			break;
#endif
		default:
			pr_err("unkown PLATFORM: 0x%x\n", status);
			return;
		}

		if (wdt_flag) {
			ret = hisp_smc_send_fiq2ispcpu();
			if (ret < 0) {
				pr_err("[%s] fiq2ispcpu.%d\n", __func__, ret);
				return;
			}
		}
		mdelay(50);
	}
}

static irqreturn_t hisp_rdr_wdt_irq_handler(int irq, void *data)
{
	struct hisp_rdr_device *dev = &hisp_rdr_dev;
	struct hisp_rdr_err_type *err_info = NULL;
	int ret = 0;

	pr_info("%s:enter.\n", __func__);
	/* disable wdt */
	if (dev->wdt_addr == NULL) {
		pr_err("%s: wdt_addr NULL\n", __func__);
		return IRQ_NONE;
	}

	writel(WDT_UNLOCK, dev->wdt_addr + ISP_WATCHDOG_WDG_LOCK_ADDR);
	writel(0, dev->wdt_addr + ISP_WATCHDOG_WDG_CONTROL_ADDR);
	writel(WDT_LOCK, dev->wdt_addr + ISP_WATCHDOG_WDG_LOCK_ADDR);

	/* init sync work */
	writel(0, dev->rdr_addr + RDR_SYNC_WORD_OFF);
	ret = hisp_get_ispcpu_cfginfo();
	if (ret < 0)
		pr_err("%s: hisp_get_ispcpu_cfginfo failed, irq.0x%x\n",
				__func__, irq);

	err_info = kzalloc(sizeof(struct hisp_rdr_err_type), GFP_ATOMIC);
	if (err_info == NULL)
		return IRQ_NONE;

	err_info->type = ISP_WDT_TIMEOUT;

	spin_lock(&dev->err_list_lock);
	list_add_tail(&err_info->node, &dev->err_list);
	spin_unlock(&dev->err_list_lock);

	queue_work(dev->wq, &dev->err_work);
	pr_info("%s:exit.\n", __func__);
	return IRQ_HANDLED;
}
/*lint -restore */
static void hisp_rdr_system_err_work(struct work_struct *work)
{
	struct hisp_rdr_device *dev = &hisp_rdr_dev;
	struct hisp_rdr_err_type *entry = NULL, *tmp = NULL;

	list_for_each_entry_safe(entry, tmp, &dev->err_list, node) {
		if (entry->type == ISP_WDT_TIMEOUT)
			rdr_system_error(entry->type, dev->wdt_irq, 0);
		else
			rdr_system_error(entry->type, 0, 0);

		spin_lock_irq(&dev->err_list_lock);
		list_del(&entry->node);
		spin_unlock_irq(&dev->err_list_lock);

		kfree(entry);
	}
}

static int hisp_rdr_wdt_init(struct hisp_rdr_device *dev)
{
	int ret = 0;
	int irq = 0;

	pr_info("%s: enter.\n", __func__);
	if (dev->wdt_enable_flag == 0) {
		pr_info("%s: isp wdt is disabled.\n", __func__);
		return 0;
	}

	irq = hisp_rdr_get_wdt_irq();
	if (irq <= 0) {
		pr_err("%s: hisp_rdr_get_wdt_irq failed, irq.0x%x\n",
			__func__, irq);
		return -EINVAL;
	}
	dev->wdt_irq = irq;

	ret = request_irq(dev->wdt_irq, hisp_rdr_wdt_irq_handler,
			  0, "isp wtd hanler", NULL);
	if (ret != 0)
		pr_err("%s: request_irq failed, irq.%d, ret.%d.\n",
			__func__, dev->wdt_irq, ret);

	pr_info("%s: exit.\n", __func__);
	return 0;
}

static void hisp_rdr_wdt_exit(struct hisp_rdr_device *dev)
{
	pr_info("%s: enter.\n", __func__);
	free_irq(dev->wdt_irq, NULL);
	pr_info("%s: exit.\n", __func__);
}

static int hisp_rdr_module_register(void)
{
	struct hisp_rdr_device *dev = &hisp_rdr_dev;
	struct rdr_module_ops_pub module_ops;
	struct rdr_register_module_result ret_info;
	int ret = 0;

	pr_info("%s: enter.\n", __func__);
	module_ops.ops_dump = hisp_rdr_dump;
	module_ops.ops_reset = hisp_rdr_reset;

	dev->core_id = RDR_ISP;
	ret = rdr_register_module_ops(dev->core_id, &module_ops, &ret_info);
	if (ret != 0) {
		pr_err("%s: rdr_register_module_ops failed! return %d\n",
				__func__, ret);
		return ret;
	}

	dev->current_info.log_addr = ret_info.log_addr;
	dev->current_info.log_len = ret_info.log_len;
	dev->current_info.nve = ret_info.nve;
	dev->isprdr_addr = ret_info.log_addr;
#ifdef DEBUG_HISP
	pr_info("%s: isprdr_addr. 0x%llx\n", __func__, ret_info.log_addr);
#endif
	dev->rdr_addr = dfx_bbox_map((phys_addr_t)dev->isprdr_addr,
			dev->current_info.log_len);
	if (dev->rdr_addr == NULL) {
		pr_err("%s: isp_bbox_map rdr_addr failed.\n", __func__);
		goto rdr_module_ops_unregister;
	}

	dev->isprdr_initialized = 1;
	pr_info("%s: exit.\n", __func__);
	return 0;

rdr_module_ops_unregister:
	dev->isprdr_initialized = 0;
	ret = rdr_unregister_module_ops(dev->core_id);
	if (ret != 0)
		pr_err("%s: rdr_unregister_module_ops failed! return %d\n",
				__func__, ret);
	return -ENOMEM;
}

static void hisp_rdr_module_unregister(void)
{
	struct hisp_rdr_device *dev = &hisp_rdr_dev;
	int ret = 0;

	pr_info("%s: enter.\n", __func__);
	dev->isprdr_initialized = 0;

	dfx_bbox_unmap(dev->rdr_addr);
	dev->rdr_addr = NULL;

	dev->isprdr_addr = 0;

	ret = rdr_unregister_module_ops(dev->core_id);
	if (ret != 0)
		pr_err("%s: rdr_unregister_module_ops failed! return %d\n",
				__func__, ret);

	pr_info("%s: exit.\n", __func__);
}

#ifndef CONFIG_HISP_EXCLUDE_NOC_DEBUG
#ifdef CONFIG_DFX_NOC
static void hisp_rdr_noc_register(void)
{
	unsigned int i = 0;

	if (ISP_NOC_ENABLE == 0)
		return;

	pr_info("%s: enter.\n", __func__);
	for (i = 0; i < ARRAY_SIZE(ispnoc_err); i++) {
		pr_debug("%s: register rdr exception, i = %d, masterid:%d",
				__func__, i, ispnoc_err[i].masterid);

		noc_modid_register(ispnoc_err[i], ISP_SYSTEM_STATES);
	}

	pr_info("%s: exit.\n", __func__);
}
#endif
#endif

static int hisp_rdr_exception_register(struct hisp_rdr_device *dev)
{
	unsigned int ret;
	unsigned int i;

	pr_info("%s: enter.\n", __func__);
	for (i = 0; i < ARRAY_SIZE(exc_info); i++) {
		pr_info("%s: register rdr exception, i = %d, type:%d",
				__func__, i, exc_info[i].e_exce_type);

		if (exc_info[i].e_modid == ISP_WDT_TIMEOUT)
			if (!dev->wdt_enable_flag)
				continue;

		ret = rdr_register_exception(&exc_info[i]);
		if (ret != exc_info[i].e_modid_end) {
			pr_info("%s: rdr_register_exception failed, ret.%d.\n",
					__func__, ret);
			goto rdr_register_exception_fail;
		}
	}

	pr_info("%s: exit.\n", __func__);
	return 0;

rdr_register_exception_fail:
	for (; i > 0; i--) {
		if (exc_info[i - 1].e_modid == ISP_WDT_TIMEOUT)
			if (!dev->wdt_enable_flag)
				continue;

		if (rdr_unregister_exception(exc_info[i - 1].e_modid) < 0)
			pr_err("%s: rdr_unregister_exception failed.\n",
					__func__);
	}

	return ret;
}

static void hisp_rdr_exception_unregister(struct hisp_rdr_device *dev)
{
	int ret;
	unsigned int i;

	pr_info("%s: enter.\n", __func__);
	for (i = ARRAY_SIZE(exc_info); i > 0; i--) {
		pr_info("%s: register rdr exception, i = %d, type:%d",
				__func__, i - 1, exc_info[i - 1].e_exce_type);

		if (exc_info[i - 1].e_modid == ISP_WDT_TIMEOUT)
			if (!dev->wdt_enable_flag)
				continue;

		ret = rdr_unregister_exception(exc_info[i - 1].e_modid);
		if (ret < 0)
			pr_info("%s: rdr_unregister_exception failed, ret.%d.\n",
					__func__, ret);
	}

	pr_info("%s: exit.\n", __func__);
}

static int hisp_rdr_dev_map(struct hisp_rdr_device *dev)
{
	unsigned int value;
	bool wdt_flag = false;

	pr_info("%s: enter.\n", __func__);

	if (dev == NULL) {
		pr_err("%s: hisp_rdr_device is NULL.\n", __func__);
		return -EINVAL;
	}
	dev->wdt_enable_flag = wdt_flag;

	dev->sctrl_addr = hisp_dev_get_regaddr(ISP_SCTRL);
	if (dev->sctrl_addr == NULL) {
		pr_err("%s: ioremp sctrl failed.\n", __func__);
		return -ENOMEM;
	}

	value = readl(dev->sctrl_addr + SCTRL_SCBAKDATA10_ADDR);

	if (value & (1 << SC_ISP_WDT_BIT))
		wdt_flag = true;

	if (wdt_flag) {
		dev->wdt_addr = hisp_dev_get_regaddr(ISP_WDT);
		if (!dev->wdt_addr) {
			pr_err("%s: ioremp wdt failed.\n", __func__);
			goto err;
		}

		dev->pctrl_addr = hisp_dev_get_regaddr(ISP_PCTRL);
		if (!dev->pctrl_addr) {
			pr_err("%s: ioremp pctrl failed.\n", __func__);
			goto err;
		}
	}

	dev->wdt_enable_flag = wdt_flag;
	pr_info("%s: exit.\n", __func__);
	return 0;

err:
	pr_info("%s: error, exit.\n", __func__);
	return -ENOMEM;
}

int hisp_rdr_init(void)
{
	struct hisp_rdr_device *dev = &hisp_rdr_dev;
	int ret = 0;
	int i;

	ret = hisp_rdr_dev_map(dev);
	if (ret != 0) {
		pr_err("%s: hisp_rdr_dev_map failed.\n", __func__);
		return ret;
	}

	ret = hisp_rdr_wdt_init(dev);
	if (ret != 0) {
		pr_err("%s: hisp_rdr_wdt_init failed.\n", __func__);
		return ret;
	}

	ret = hisp_rdr_module_register();
	if (ret != 0) {
		pr_err("%s: hisp_rdr_module_register failed.\n", __func__);
		goto hisp_rdr_module_register_fail;
	}
#ifndef CONFIG_HISP_EXCLUDE_NOC_DEBUG
#ifdef CONFIG_DFX_NOC
	hisp_rdr_noc_register();
#endif
#endif
	ret = hisp_rdr_exception_register(dev);
	if (ret != 0) {
		pr_err("%s: hisp_rdr_exception_register failed.\n", __func__);
		goto rdr_register_exception_fail;
	}

	dev->wq = create_singlethread_workqueue(MODULE_NAME);
	if (dev->wq == NULL) {
		pr_err("%s: create_singlethread_workqueue failed.\n", __func__);
		ret = -1;
		goto create_singlethread_workqueue_fail;
	}

	INIT_WORK(&dev->dump_work, hisp_rdr_syserr_dump_work);
	INIT_WORK(&dev->err_work, hisp_rdr_system_err_work);
	INIT_LIST_HEAD(&dev->err_list);
	INIT_LIST_HEAD(&dev->dump_list);

	spin_lock_init(&dev->err_list_lock);
	spin_lock_init(&dev->dump_list_lock);
	dev->offline_rdrlevel = ISPCPU_DEFAULT_RDR_LEVEL;
	for (i = 0; i < IRQ_NUM; i++)
		dev->irq[i] = 0;

	pr_info("[%s] -\n", __func__);

	return 0;

create_singlethread_workqueue_fail:
	hisp_rdr_exception_unregister(dev);

rdr_register_exception_fail:
	hisp_rdr_module_unregister();

hisp_rdr_module_register_fail:
	hisp_rdr_wdt_exit(dev);

	return ret;
}

void hisp_rdr_exit(void)
{
	struct hisp_rdr_device *dev = &hisp_rdr_dev;

	destroy_workqueue(dev->wq);
	hisp_rdr_exception_unregister(dev);
	hisp_rdr_module_unregister();
	hisp_rdr_wdt_exit(dev);
}

MODULE_LICENSE("GPL v2");

#ifdef DEBUG_HISP
static int hisp_rdr_find_irq_num(const char *cmp,
		const char *input)
{
	unsigned long data;
	unsigned int len_cmp;
	int ret;

	len_cmp = strlen(cmp);
	ret = kstrtoul(input + len_cmp, 0, &data);
	if (ret < 0) {
		pr_err("%s: strict_strtoul failed, ret.%d\n", __func__, ret);
		return -EINVAL;
	}

	pr_info("%s: number is %d.\n", __func__, (int)data);
	return (int)data;
}

struct level_switch_s rdrlevel[] = {
	{ISPCPU_RDR_USE_APCTRL, "yes", "no", "RDR Controlled by AP"},
	{ISPCPU_RDR_RESERVE_30, "enable", "disable", "reserved 30"},
	{ISPCPU_RDR_RESERVE_29, "enable", "disable", "reserved 29"},
	{ISPCPU_RDR_RESERVE_28, "enable", "disable", "reserved 28"},
	{ISPCPU_RDR_RESERVE_27, "enable", "disable", "reserved 27"},
	{ISPCPU_RDR_RESERVE_26, "enable", "disable", "reserved 26"},
	{ISPCPU_RDR_RESERVE_25, "enable", "disable", "reserved 25"},
	{ISPCPU_RDR_RESERVE_24, "enable", "disable", "reserved 24"},
	{ISPCPU_RDR_RESERVE_23, "enable", "disable", "reserved 23"},
	{ISPCPU_RDR_RESERVE_22, "enable", "disable", "reserved 22"},
	{ISPCPU_RDR_RESERVE_21, "enable", "disable", "reserved 21"},
	{ISPCPU_RDR_RESERVE_20, "enable", "disable", "reserved 20"},
	{ISPCPU_RDR_RESERVE_19, "enable", "disable", "reserved 19"},
	{ISPCPU_RDR_RESERVE_18, "enable", "disable", "reserved 18"},
	{ISPCPU_RDR_RESERVE_17, "enable", "disable", "reserved 17"},
	{ISPCPU_RDR_RESERVE_16, "enable", "disable", "reserved 16"},
	{ISPCPU_RDR_RESERVE_15, "enable", "disable", "reserved 15"},
	{ISPCPU_RDR_RESERVE_14, "enable", "disable", "reserved 14"},
	{ISPCPU_RDR_RESERVE_13, "enable", "disable", "reserved 13"},
	{ISPCPU_RDR_RESERVE_12, "enable", "disable", "reserved 12"},
	{ISPCPU_RDR_RESERVE_11, "enable", "disable", "reserved 11"},
	{ISPCPU_RDR_RESERVE_10, "enable", "disable", "reserved 10"},
	{ISPCPU_RDR_RESERVE_9, "enable", "disable", "reserved 9"},
	{ISPCPU_RDR_RESERVE_8, "enable", "disable", "reserved 8"},
	{ISPCPU_RDR_RESERVE_7, "enable", "disable", "reserved 7"},
	{ISPCPU_RDR_LEVEL_CPUP, "enable", "disable", "task cpup"},
	{ISPCPU_RDR_LEVEL_TASK, "enable", "disable", "task switch"},
	{ISPCPU_RDR_LEVEL_IRQ, "enable", "disable", "irq"},
	{ISPCPU_RDR_LEVEL_CVDR, "enable", "disable", "cvdr"},
	{ISPCPU_RDR_LEVEL_ALGO, "enable", "disable", "algo"},
	{ISPCPU_RDR_LEVEL_LAST_WORD, "enable", "disable", "last word"},
	{ISPCPU_RDR_LEVEL_TRACE, "enable", "disable", "trace"},
};
static void hisp_rdr_usage_ctrl(void)
{
	unsigned int i = 0;

	pr_info("<Usage: >\n");
	for (i = 0; i < ARRAY_SIZE(rdrlevel); i++) {
		if (rdrlevel[i].level == ISPCPU_RDR_USE_APCTRL)
			continue;
		pr_info("echo <%s>:<%s/%s> > rdr_isp\n", rdrlevel[i].info,
			rdrlevel[i].enable_cmd, rdrlevel[i].disable_cmd);
	}
	/* SIRQ Handle */
	pr_info("echo <%s>:<%s/%s> > rdr_isp\n", "sirqX",
			"enable", "disable");
	pr_info("sirqX,X[0-%d]\n", IRQ_NUM);
}

static int hisp_rdr_get_offline_rdrlevel(const char *buf,
		char *p, int len)
{
	int flag = 0;
	unsigned int i = 0;
	struct hisp_rdr_device *dev = &hisp_rdr_dev;

	for (i = 0; i < ARRAY_SIZE(rdrlevel); i++) {
		if (rdrlevel[i].level == ISPCPU_RDR_USE_APCTRL)
			continue;

		if (!strncmp(buf, rdrlevel[i].info, len)) {
			flag = 1;
			p += 1;
			if (!strncmp(p, rdrlevel[i].enable_cmd,
					(int)strlen(rdrlevel[i].enable_cmd)))
				dev->offline_rdrlevel |= rdrlevel[i].level;
			else if (!strncmp(p, rdrlevel[i].disable_cmd,
					(int)strlen(rdrlevel[i].disable_cmd)))
				dev->offline_rdrlevel &= ~(rdrlevel[i].level);
			else
				flag = 0;
			break;
		}
	}

	return flag;
}

static int hisp_rdr_get_irq_num(const char *buf, char *p,
		int len, struct hisp_shared_para *param)
{
	char *q = NULL;
	int irq_num = -1;
	int ret = 0;
	int flag = 0;
	struct hisp_rdr_device *dev = &hisp_rdr_dev;

	if (!strncmp(buf, "sirq", (int)strlen("sirq"))) {
		p += 1;

		q = kzalloc(strlen(buf) + 1, GFP_KERNEL);
		if (q == NULL)
			return flag;

		ret = memcpy_s(q, ISP_RDR_MAX_SIZE, buf, len);
		if (ret != 0) {
			pr_err("%s:memcpy_s q from buf failed.\n", __func__);
			kfree_sensitive(q);
			return flag;
		}
		q[len] = '\0';

		irq_num = hisp_rdr_find_irq_num("sirq", q);
		kfree_sensitive(q);

		if (irq_num < 0 || irq_num > (IRQ_NUM - 1)) {
			pr_err("%s: SIRQ irq number overflow.\n", __func__);
			return flag;
		}
		pr_info("%s: SIRQ .%d\n", __func__, irq_num);

		if (!strncmp(p, "enable", (int)strlen("enable"))) {
			flag = 1;
			param->irq[irq_num] = 1;
			dev->irq[irq_num] = 1;
		} else if (!strncmp(p, "disable", (int)strlen("disable"))) {
			flag = 1;
			param->irq[irq_num] = 0;
			dev->irq[irq_num] = 0;
		} else {
			flag = 0;
		}
	}

	return flag;
}

ssize_t hisp_rdr_store(struct device *pdev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int len = 0, flag = 0;
	char *p = NULL;
	struct hisp_rdr_device *dev = &hisp_rdr_dev;
	struct hisp_shared_para *param = NULL;

	if (buf == NULL)
		return -EINVAL;

	p = memchr(buf, ':', count);
	if (p == NULL)
		return -EINVAL;

	len = p - buf;

	flag = hisp_rdr_get_offline_rdrlevel(buf, p, len);

	hisp_lock_sharedbuf();
	param = hisp_share_get_para();
	if (param == NULL) {
		pr_err("%s:hisp_share_get_para failed.\n", __func__);
		goto EXIT;
	}
	/*sirq should special handle  example:sirq32:disable or sirq32:enable*/
	flag += hisp_rdr_get_irq_num(buf, p, len, param);

EXIT:
	if (!flag)
		hisp_rdr_usage_ctrl();

	if (param != NULL) {
		param->rdr_enable_type =
			(param->rdr_enable_type & ~ISPCPU_RDR_LEVEL_MASK) |
			(dev->offline_rdrlevel & ISPCPU_RDR_LEVEL_MASK);
		pr_info("[%s] rdrlevel.0x%x >> online.0x%x\n", __func__,
				dev->offline_rdrlevel, param->rdr_enable_type);
	}
	hisp_unlock_sharedbuf();

	return (ssize_t)count;

}
ssize_t hisp_rdr_show(struct device *pdev,
		struct device_attribute *attr, char *buf)
{
	struct hisp_rdr_device *dev = &hisp_rdr_dev;
	struct hisp_shared_para *param = NULL;
	unsigned int rdr_enable_type = 0;
	char *tmp = NULL;
	ssize_t size = 0;
	unsigned int i;
	int ret = 0;

	tmp = kzalloc(ISP_RDR_TMP_SIZE, GFP_KERNEL);
	if (tmp == NULL)
		return 0;

	hisp_lock_sharedbuf();
	param = hisp_share_get_para();
	if (param != NULL)
		rdr_enable_type = param->rdr_enable_type;

	for (i = 0; i < ARRAY_SIZE(rdrlevel); i++)
		// cppcheck-suppress *
		size += snprintf_s(tmp + size, ISP_RDR_MAX_SIZE,
			ISP_RDR_MAX_SIZE - 1, "[%s.%s] : %s\n",
			(param ? ((rdr_enable_type & rdrlevel[i].level) ?
			rdrlevel[i].enable_cmd : rdrlevel[i].disable_cmd) :
			"ispoffline"),
			((dev->offline_rdrlevel & rdrlevel[i].level) ?
			rdrlevel[i].enable_cmd : rdrlevel[i].disable_cmd),
			rdrlevel[i].info);/*lint !e421 */

	/*handle  sirq */
	for (i = 1; i < IRQ_NUM; i++)
		size += snprintf_s(tmp + size, ISP_RDR_MAX_SIZE,
				   ISP_RDR_MAX_SIZE - 1, "[%s.%s] : sirq%u\n",
				   (param ? ((param->irq[i]) ?
				   "enable":"disable"):"ispoffline"),
				   ((dev->irq[i])?"enable":"disable"), i);


	hisp_unlock_sharedbuf();
	ret = memcpy_s((void *)buf, ISP_RDR_TMP_SIZE,
			   (void *)tmp, ISP_RDR_TMP_SIZE);
	if (ret != 0)
		pr_err("%s:memcpy_s q from buf failed.\n", __func__);

	kfree((void *)tmp);
	return size;
}
#endif
