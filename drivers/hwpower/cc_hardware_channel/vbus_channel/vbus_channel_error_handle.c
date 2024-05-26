// SPDX-License-Identifier: GPL-2.0
/*
 * vbus_channel_error_handle.c
 *
 * error handle for vbus channel driver
 *
 * Copyright (c) 2020-2020 Huawei Technologies Co., Ltd.
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

#include <chipset_common/hwpower/hardware_channel/vbus_channel_error_handle.h>
#include <chipset_common/hwpower/hardware_channel/vbus_channel.h>
#include <chipset_common/hwpower/common_module/power_dts.h>
#include <chipset_common/hwpower/common_module/power_dsm.h>
#include <chipset_common/hwpower/common_module/power_event_ne.h>
#include <huawei_platform/hwpower/common_module/power_platform.h>
#include <chipset_common/hwpower/common_module/power_printk.h>

#define HWLOG_TAG vbus_channel_eh
HWLOG_REGIST();

#define MAX_USB_STATE_CNT         5
#define VBUS_CH_EH_DMD_BUF_SIZE   128

static struct vbus_ch_eh_dev *g_vbus_ch_eh_dev;
static bool g_otg_scp_flag;

static struct vbus_ch_eh_dev *vbus_ch_eh_get_dev(void)
{
	if (!g_vbus_ch_eh_dev) {
		hwlog_err("g_vbus_ch_eh_dev is null\n");
		return NULL;
	}

	return g_vbus_ch_eh_dev;
}

static int vbus_ch_eh_check_usb_state(struct vbus_ch_eh_dev *l_dev)
{
	if (power_platform_usb_state_is_host())
		l_dev->usb_state_count = 0;
	else
		l_dev->usb_state_count++;

	if (l_dev->usb_state_count >= MAX_USB_STATE_CNT) {
		hwlog_info("not in usb_state_host, close check work\n");
		return -EPERM;
	}

	return 0;
}

static void vbus_ch_eh_dmd_report(enum vbus_ch_eh_dmd_type type,
	const void *data)
{
	char buff[VBUS_CH_EH_DMD_BUF_SIZE] = {0};

	if (!data)
		return;

	switch (type) {
	case OTG_SCP_DMD_REPORT:
		snprintf(buff, sizeof(buff), "otg scp: data=%d\n",
			*(int *)data);
		power_dsm_report_dmd(POWER_DSM_BATTERY,
			POWER_DSM_OTG_VBUS_SHORT, buff);
		break;
	case OTG_OCP_DMD_REPORT:
		snprintf(buff, sizeof(buff), "otg ocp: data=%d\n",
			*(int *)data);
		power_dsm_report_dmd(POWER_DSM_BATTERY,
			POWER_DSM_OTG_VBUS_SHORT, buff);
		break;
	default:
		hwlog_err("type is invaild\n");
		break;
	}
}

bool vbus_ch_eh_get_otg_scp_flag(void)
{
	return g_otg_scp_flag;
}

static void vbus_ch_eh_set_otg_scp_flag(bool flag)
{
	g_otg_scp_flag = flag;
	hwlog_info("set_otg_scp_flag: flag=%d\n", flag);
}

static void otg_scp_check_work(struct work_struct *work)
{
	struct vbus_ch_eh_dev *l_dev = vbus_ch_eh_get_dev();
	struct otg_scp_info *info = NULL;
	int value;

	if (!l_dev)
		return;

	info = &l_dev->otg_scp;
	if (!info)
		return;

	value = charge_get_vusb();
	if (value < 0) {
		hwlog_err("get vusb value failed\n");
		return;
	}

	if (value < (int)info->vol_mv) {
		info->fault_count++;
		hwlog_info("sc: value=%d, fault_count=%u\n", value, info->fault_count);
	} else {
		info->fault_count = 0;
	}

	if (info->fault_count > info->check_times) {
		info->fault_count = 0;
		if (info->work_flag) {
			vbus_ch_eh_set_otg_scp_flag(true);
			if (power_platform_pogopin_is_support() &&
				power_platform_pogopin_otg_from_buckboost())
				vbus_ch_close(VBUS_CH_USER_PD,
					VBUS_CH_TYPE_POGOPIN_BOOST, true, true);
			else
				vbus_ch_close(VBUS_CH_USER_PD,
					VBUS_CH_TYPE_BOOST_GPIO, true, true);
			vbus_ch_eh_dmd_report(OTG_SCP_DMD_REPORT, &value);
		}

		hwlog_err("otg short circuit happen\n");
		return;
	}

	if (vbus_ch_eh_check_usb_state(l_dev))
		return;

	if (info->work_flag)
		schedule_delayed_work(&info->work, msecs_to_jiffies(info->delayed_time));
}

static void otg_scp_check_start(struct vbus_ch_eh_dev *l_dev, void *data)
{
	struct otg_scp_para *para = NULL;

	if (!data)
		return;

	para = (struct otg_scp_para *)data;
	l_dev->otg_scp.vol_mv = para->vol_mv;
	l_dev->otg_scp.check_times = para->check_times;
	l_dev->otg_scp.delayed_time = para->delayed_time;
	l_dev->otg_scp.work_flag = true;
	l_dev->usb_state_count = 0;
	cancel_delayed_work(&l_dev->otg_scp.work);
	schedule_delayed_work(&l_dev->otg_scp.work,
		msecs_to_jiffies(l_dev->otg_scp.delayed_time));
}

static void otg_scp_check_stop(struct vbus_ch_eh_dev *l_dev)
{
	l_dev->otg_scp.fault_count = 0;
	l_dev->otg_scp.work_flag = false;
	l_dev->usb_state_count = 0;
	cancel_delayed_work(&l_dev->otg_scp.work);
	vbus_ch_eh_set_otg_scp_flag(false);
}

static void otg_ocp_handle(struct vbus_ch_eh_dev *l_dev, void *data)
{
	if (power_platform_pogopin_is_support() &&
		power_platform_pogopin_otg_from_buckboost())
		vbus_ch_close(VBUS_CH_USER_PD,
			VBUS_CH_TYPE_POGOPIN_BOOST, true, true);
	else
		vbus_ch_close(VBUS_CH_USER_PD,
			VBUS_CH_TYPE_BOOST_GPIO, true, true);
	vbus_ch_eh_dmd_report(OTG_OCP_DMD_REPORT, data);
}

static void otg_ocp_check_work(struct work_struct *work)
{
	struct vbus_ch_eh_dev *l_dev = vbus_ch_eh_get_dev();
	struct otg_ocp_info *info = NULL;
	int value;

	if (!l_dev)
		return;

	info = &l_dev->otg_ocp;
	if (!info)
		return;

	value = charge_get_vbus();
	if (value < 0) {
		hwlog_err("get vbus value failed\n");
		return;
	}

	if (value < (int)info->vol_mv) {
		info->fault_count++;
		hwlog_info("ocp: value=%d, fault_count=%u\n", value, info->fault_count);
	} else {
		info->fault_count = 0;
	}

	if (info->fault_count > info->check_times) {
		info->fault_count = 0;
		if (info->work_flag)
			vbus_ch_eh_dmd_report(OTG_OCP_DMD_REPORT, &value);
		hwlog_err("otg ocp happen\n");
		return;
	}

	if (info->work_flag)
		schedule_delayed_work(&info->work, msecs_to_jiffies(info->delayed_time));
}

static void otg_ocp_check_start(struct vbus_ch_eh_dev *l_dev, void *data)
{
	struct otg_ocp_para *para = NULL;

	if (!data)
		return;

	para = (struct otg_ocp_para *)data;
	l_dev->otg_ocp.vol_mv = para->vol_mv;
	l_dev->otg_ocp.check_times = para->check_times;
	l_dev->otg_ocp.delayed_time = para->delayed_time;
	l_dev->otg_ocp.work_flag = true;
	cancel_delayed_work(&l_dev->otg_ocp.work);
	schedule_delayed_work(&l_dev->otg_ocp.work,
		msecs_to_jiffies(l_dev->otg_ocp.delayed_time));
}

static void otg_ocp_check_stop(struct vbus_ch_eh_dev *l_dev)
{
	l_dev->otg_ocp.fault_count = 0;
	l_dev->otg_ocp.work_flag = false;
	cancel_delayed_work(&l_dev->otg_ocp.work);
}

static int vbus_ch_eh_notifier_call(struct notifier_block *nb,
	unsigned long event, void *data)
{
	struct vbus_ch_eh_dev *l_dev = vbus_ch_eh_get_dev();

	if (!l_dev)
		return NOTIFY_OK;

	switch (event) {
	case POWER_NE_OTG_SCP_CHECK_STOP:
		otg_scp_check_stop(l_dev);
		break;
	case POWER_NE_OTG_SCP_CHECK_START:
		otg_scp_check_start(l_dev, data);
		break;
	case POWER_NE_OTG_OCP_HANDLE:
		otg_ocp_handle(l_dev, data);
		break;
	case POWER_NE_OTG_OCP_CHECK_STOP:
		otg_ocp_check_stop(l_dev);
		break;
	case POWER_NE_OTG_OCP_CHECK_START:
		otg_ocp_check_start(l_dev, data);
		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static int __init vbus_ch_eh_init(void)
{
	int ret;
	struct vbus_ch_eh_dev *l_dev = NULL;

	l_dev = kzalloc(sizeof(*l_dev), GFP_KERNEL);
	if (!l_dev)
		return -ENOMEM;

	g_vbus_ch_eh_dev = l_dev;
	l_dev->nb.notifier_call = vbus_ch_eh_notifier_call;
	ret = power_event_bnc_register(POWER_BNT_OTG, &l_dev->nb);
	if (ret)
		goto fail_free_mem;

	INIT_DELAYED_WORK(&l_dev->otg_scp.work, otg_scp_check_work);
	INIT_DELAYED_WORK(&l_dev->otg_ocp.work, otg_ocp_check_work);
	return 0;

fail_free_mem:
	kfree(l_dev);
	g_vbus_ch_eh_dev = NULL;
	return ret;
}

static void __exit vbus_ch_eh_exit(void)
{
	struct vbus_ch_eh_dev *l_dev = vbus_ch_eh_get_dev();

	if (!l_dev)
		return;

	cancel_delayed_work(&l_dev->otg_scp.work);
	cancel_delayed_work(&l_dev->otg_ocp.work);
	power_event_bnc_unregister(POWER_BNT_OTG, &l_dev->nb);
	kfree(l_dev);
	g_vbus_ch_eh_dev = NULL;
}

subsys_initcall_sync(vbus_ch_eh_init);
module_exit(vbus_ch_eh_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("vbus channel error check driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
