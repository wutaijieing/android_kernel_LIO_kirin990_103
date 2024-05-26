/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 * Description: ComboPHY Function Module on  platform
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2  of
 * the License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/platform_drivers/usb/tca.h>
#if defined(CONFIG_HUAWEI_DSM) && defined(CONFIG_HW_DP_SOURCE)
#include <huawei_platform/dp_source/dp_dsm.h>
#endif
#ifdef CONFIG_TCPC_CLASS
#include <huawei_platform/usb/hw_pd_dev.h>
#endif
#include <platform_include/display/linux/dpu_dss_dp.h>
#include <linux/platform_drivers/usb/chip_usb.h>
#include <linux/kernel.h>
#include <linux/kfifo.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/version.h>
#include "combophy_core.h"
#include "combophy_common.h"

#undef pr_fmt
#define pr_fmt(fmt) "[COMBOPHY_FUNC]%s: " fmt, __func__

#define MAX_EVENT_COUNT 8
#define INVALID_REG_VALUE 0xFFFFFFFFu
#define COMBOPHY_EVENT_WAIT_TIMEOUT msecs_to_jiffies(10500)

struct combophy_manager {
	enum tcpc_mux_ctrl_type curr_mode;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	enum tcpc_mux_ctrl_type event_type;
#endif
	enum typec_plug_orien_e curr_orien;
	struct timer_list event_timer;
};

static struct combophy_manager *g_combophy_manager;

static void combophy_event_complete(struct chip_usb_event *event)
{
	struct combophy_manager *manager = event->content;
	enum tcpc_mux_ctrl_type mode_type = (enum tcpc_mux_ctrl_type)event->param1;
	enum typec_plug_orien_e typec_orien = (enum typec_plug_orien_e)event->param2;

	pr_info("event %s, mode %s, orien %d\n",
		dev_type_string((enum tca_device_type_e)event->type),
		mode_type_string(mode_type),
		typec_orien);

	/* update mode type and orien after event complete */
	manager->curr_mode = mode_type;
	manager->curr_orien = typec_orien;

	del_timer_sync(&manager->event_timer);
#ifdef CONFIG_TCPC_CLASS
	dp_dfp_u_notify_dp_configuration_done(mode_type, 0);
#endif
}

static void combophy_handle_dp_event(struct chip_usb_event *event)
{
	struct combophy_manager *manager = event->content;
	enum tca_irq_type_e irq_type = (enum tca_irq_type_e)event->param1;
	int ret = -EFAULT;

	pr_info("irq %s\n", irq_type_string(irq_type));

	if (!manager) {
		pr_err("event content is null, invalid parameter\n");
		return;
	}
	if (manager->curr_mode != TCPC_DP &&
	    manager->curr_mode != TCPC_USB31_AND_DP_2LINE) {
		pr_err("mode %s event %d mismatch\n",
		       mode_type_string(manager->curr_mode),
		       event->type);
		goto out;
	}

	ret = dpu_dptx_hpd_trigger(irq_type,
				    manager->curr_mode,
				    manager->curr_orien);
	if (ret)
		pr_err("dptx_hpd_trigger err[%d][%d]\n",
		       __LINE__, ret);

out:
	del_timer_sync(&manager->event_timer);
#ifdef CONFIG_TCPC_CLASS
	dp_dfp_u_notify_dp_configuration_done(manager->curr_mode, ret);
#endif
}

static int combophy_event_handler(struct combophy_manager *manager,
				   enum tca_irq_type_e irq_type,
				   enum tcpc_mux_ctrl_type mode_type,
				   enum tca_device_type_e dev_type,
				   enum typec_plug_orien_e typec_orien)

{
	struct chip_usb_event usb_event = {0};
	int ret;

	usb_event.type = (enum otg_dev_event_type)dev_type;
	usb_event.content = manager;
	switch (dev_type) {
	case TCA_DP_IN:
	case TCA_DP_OUT:
		usb_event.param1 = (uint32_t)irq_type;
		usb_event.flags = EVENT_CB_AT_HANDLE;
		usb_event.callback = combophy_handle_dp_event;
		break;
	case TCA_ID_RISE_EVENT:
	case TCA_CHARGER_DISCONNECT_EVENT:
	case TCA_ID_FALL_EVENT:
	case TCA_CHARGER_CONNECT_EVENT:
		usb_event.param1 = (uint32_t)mode_type;
		usb_event.param2 = (uint32_t)typec_orien;
		usb_event.flags = PD_EVENT | EVENT_CB_AT_COMPLETE;
		usb_event.callback = combophy_event_complete;
		break;
	default:
		return -EINVAL;
	}

	ret = chip_usb_queue_event(&usb_event);
	if (ret) {
		pr_err("queue event failed %d\n", ret);
		return ret;
	}

	manager->event_timer.expires = jiffies + COMBOPHY_EVENT_WAIT_TIMEOUT;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	manager->event_type =  mode_type;
#else
	manager->event_timer.data = (TIMER_DATA_TYPE)mode_type;
#endif
	mod_timer(&manager->event_timer, manager->event_timer.expires);

	return 0;
}

int pd_event_notify(enum tca_irq_type_e irq_type, enum tcpc_mux_ctrl_type mode_type,
		    enum tca_device_type_e dev_type, enum typec_plug_orien_e typec_orien)
{
	struct combophy_manager *manager = NULL;
	int ret;

	manager = g_combophy_manager;
	if (!manager) {
		pr_err("combophy func not probed\n");
		ret = chip_usb_otg_event((enum otg_dev_event_type)dev_type);
		if (ret)
			pr_err("chip_usb_otg_event ret %d\n", ret);
#ifdef CONFIG_TCPC_CLASS
		dp_dfp_u_notify_dp_configuration_done(mode_type, ret);
#endif
		return ret;
	}

	pr_info("IRQ[%s]MODEcur[%s]new[%s]DEV[%s]ORIEN[%d]\n",
		irq_type_string(irq_type),
		mode_type_string(manager->curr_mode),
		mode_type_string(mode_type),
		dev_type_string(dev_type),
		typec_orien);
#if defined(CONFIG_HUAWEI_DSM) && defined(CONFIG_HW_DP_SOURCE)
	dp_imonitor_set_pd_event(irq_type, manager->curr_mode, mode_type,
				 dev_type, typec_orien);
#endif

	if (irq_type >= TCA_IRQ_MAX_NUM || mode_type >= TCPC_MUX_MODE_MAX ||
	    dev_type >= TCA_DEV_MAX || typec_orien >= TYPEC_ORIEN_MAX) {
		ret = -EINVAL;
		pr_err("invalid pd event\n");
		goto err_out;
	}

	ret = combophy_event_handler(manager, irq_type, mode_type,
				     dev_type, typec_orien);
	if (ret)
		goto err_out;

	pr_info("-\n");
	return 0;
err_out:
	pr_err("err out %d\n", ret);
#ifdef CONFIG_TCPC_CLASS
	dp_dfp_u_notify_dp_configuration_done(mode_type, ret);
#endif
	return ret;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
static void combophy_event_watchdog(struct timer_list *t)
{
#ifdef CONFIG_TCPC_CLASS
	struct combophy_manager *manager = from_timer(manager, t, event_timer);
	enum tcpc_mux_ctrl_type mode_type = (enum tcpc_mux_ctrl_type)manager->event_type;
#endif

	pr_err("timeout for handle event\n");
#ifdef CONFIG_TCPC_CLASS
	dp_dfp_u_notify_dp_configuration_done(mode_type, -ETIMEDOUT);
#endif
}
#else
static void combophy_event_watchdog(TIMER_DATA_TYPE arg)
{
#ifdef CONFIG_TCPC_CLASS
	enum tcpc_mux_ctrl_type mode_type = (enum tcpc_mux_ctrl_type)arg;
#endif

	pr_err("timeout for handle event\n");
#ifdef CONFIG_TCPC_CLASS
	dp_dfp_u_notify_dp_configuration_done(mode_type, -ETIMEDOUT);
#endif
}
#endif

#if defined(CONFIG_PM_SLEEP) && defined(CONFIG_PRODUCT_ARMPC)
static int combophy_func_suspend(struct device *dev)
{
	struct combophy_manager *manager = NULL;
	int ret;

	pr_info("+\n");
	manager = g_combophy_manager;
	if (manager == NULL)
		return -EINVAL;

	pr_info("combophy suspend process, cur_mode:%u typec_orien: %u.\n",
		manager->curr_mode, manager->curr_orien);

	if (manager->curr_mode == TCPC_DP ||
		manager->curr_mode == TCPC_USB31_AND_DP_2LINE) {
		ret = dpu_dptx_hpd_trigger(TCA_IRQ_HPD_OUT,
			manager->curr_mode, manager->curr_orien);
		if (ret)
			pr_err("dptx_hpd_trigger failed, [%d].\n", ret);
	}

	pr_info("-\n");
	return 0;
}

static int combophy_func_resume(struct device *dev)
{
	struct combophy_manager *manager = NULL;
	int ret;

	pr_info("+\n");
	manager = g_combophy_manager;
	if (manager == NULL)
		return -EINVAL;

	pr_info("combophy resume process, cur_mode:%u typec_orien: %u.\n",
		manager->curr_mode, manager->curr_orien);

	if (manager->curr_mode == TCPC_DP ||
		manager->curr_mode == TCPC_USB31_AND_DP_2LINE) {
		ret = dpu_dptx_hpd_trigger(TCA_IRQ_HPD_IN,
			 manager->curr_mode, manager->curr_orien);
		if (ret)
			pr_err("dptx_hpd_trigger failed, [%d].\n", ret);
	}

	pr_info("-\n");
	return 0;
}

const struct dev_pm_ops chip_combophy_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(combophy_func_suspend, combophy_func_resume)
};

#endif

static int combophy_func_probe(struct platform_device *pdev)
{
	struct combophy_manager *manager = NULL;
	struct device *dev = &pdev->dev;
	int ret;

	pr_info("+\n");
	manager = devm_kzalloc(dev, sizeof(*manager), GFP_KERNEL);
	if (!manager)
		return -ENOMEM;

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	pm_runtime_no_callbacks(dev);
	ret = pm_runtime_get_sync(dev);
	if (ret < 0) {
		pr_err("pm_runtime_get_sync failed\n");
		goto err_pm_put;
	}

	pm_runtime_forbid(dev);

	manager->curr_mode = TCPC_NC;
	manager->curr_orien = TYPEC_ORIEN_POSITIVE;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	timer_setup(&manager->event_timer, combophy_event_watchdog, 0);
#else
	setup_timer(&manager->event_timer, combophy_event_watchdog, 0);
#endif
	g_combophy_manager = manager;

	pr_info("-\n");

	return 0;
err_pm_put:
	pm_runtime_put_sync(dev);
	pm_runtime_disable(dev);

	pr_err("ret %d\n", ret);
	return ret;
}

static int combophy_func_remove(struct platform_device *pdev)
{
	struct combophy_manager *manager = g_combophy_manager;

	del_timer_sync(&manager->event_timer);
	pm_runtime_allow(&pdev->dev);
	pm_runtime_put_sync(&pdev->dev);
	pm_runtime_disable(&pdev->dev);

	g_combophy_manager = NULL;

	return 0;
}

static const struct of_device_id combophy_func_of_match[] = {
	{ .compatible = "hisilicon,combophy-func", },
	{ }
};
MODULE_DEVICE_TABLE(of, combophy_func_of_match);

static struct platform_driver combophy_func_driver = {
	.probe	= combophy_func_probe,
	.remove = combophy_func_remove,
	.driver = {
		.name	= "chip-combophy-func",
		.of_match_table	= combophy_func_of_match,
#if defined(CONFIG_PM_SLEEP) && defined(CONFIG_PRODUCT_ARMPC)
		.pm = &chip_combophy_pm_ops,
#endif
	}
};
module_platform_driver(combophy_func_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("ComboPHY Function Driver");
