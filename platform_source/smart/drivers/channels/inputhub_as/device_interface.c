/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
 * Description: device status manager implement.
 * Create: 2019-11-05
 */

#include <securec.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/pm_wakeup.h>
#include <linux/suspend.h>
#include <linux/reboot.h>
#include <platform_include/smart/linux/base/ap/protocol.h>
#include <platform_include/smart/linux/iomcu_pm.h>
#include <platform_include/smart/linux/inputhub_as/iomcu_route.h>
#include <platform_include/smart/linux/inputhub_as/device_interface.h>
#include <platform_include/smart/linux/inputhub_as/device_manager.h>
#include <platform_include/smart/linux/inputhub_as/plat_func.h>
#include <platform_include/smart/linux/iomcu_dump.h>
#include <platform_include/smart/linux/iomcu_ipc.h>
#include "common/common.h"
#include "inputhub_wrapper/inputhub_wrapper.h"

static DEFINE_MUTEX(g_mutex_update);

#define RET_AP_DEVICE_ERROR (RET_SUCC + 1)
#define RET_ERR_RESP (RET_SUCC + 2)

#define DISABLE_SLEEP_TIME 50

struct device_status g_device_status;

static struct ap_device_ops_record g_all_ap_device_operations[TAG_DEVICE_END];


/*
 * Function    : update_device_enable_status
 * Description : update device status for enable
 * Input       : [tag] key, identify an app of sensor
 *             : [enable] if true, set device status enable true;
 *             :          if false, set false.
 * Output      : none
 * Return      : none
 */
static void update_device_enable_status(int tag, bool enable)
{
	/*
	 * Update device enable status.
	 * The caller ensures that the parameters are valid.
	 * The caller use lock to protect resource.
	 */
	if (enable) {
		g_device_status.opened[tag] = 1;
	} else {
		g_device_status.opened[tag] = 0;
		g_device_status.status[tag] = 0;
		g_device_status.delay[tag] = -1;
	}
}

/*
 * Function    : update_device_delay_status
 * Description : update device status for set delay
 * Input       : [tag] key, identify an app of sensor
 *             : [period] sample rate
 *             : [batch_cnt] batch cnt
 *             : [success] if true(set delay success), use input update device status;
 *             :           if false(set delay failed), use default value update device status
 * Output      : none
 * Return      : none
 */
static void update_device_delay_status(int tag, int period, int batch_cnt, bool success)
{
	/* status is flag that if have set interval or not */
	if (success) {
		g_device_status.status[tag] = 1;
		g_device_status.delay[tag] = period;
		g_device_status.batch_cnt[tag] = batch_cnt;
	} else {
		g_device_status.status[tag] = 0;
		g_device_status.delay[tag] = -1;
	}
}

/*
 * Function    : inputhub_device_setdelay_internal
 * Description : set delay internal interface
 * Input       : [tag] key, identify an app of sensor
 *             : [param] interval param
 * Output      : none
 * Return      : 0 is success, else is error
 */
static int inputhub_device_setdelay_internal(int tag, const interval_param_t *param)
{
	int ret;

	if (work_on_ap(tag)) {
		ret = ap_device_setdelay(tag, param->period);
		if (ret != RET_SUCC)
			return RET_AP_DEVICE_ERROR;
	}

	ret = inputhub_wrapper_send_cmd(tag, CMD_CMN_INTERVAL_REQ,
		(const void *)param, sizeof(interval_param_t), NULL);
	if (ret != RET_SUCC)
		return RET_FAIL;

	return RET_SUCC;
}

/*
 * Function    : inputhub_device_enable_internal
 * Description : enable internal interface
 * Input       : [tag] key, identify an app of sensor
 *             : [enable] true is enable, false is disable
 * Output      : none
 * Return      : 0 is success, else is error
 */
static int inputhub_device_enable_internal(int tag, bool enable)
{
	int ret;
	int cmd = enable ? CMD_CMN_OPEN_REQ : CMD_CMN_CLOSE_REQ;

	if (work_on_ap(tag)){
		ret = ap_device_enable(tag, enable);
		if (ret != RET_SUCC)
			return RET_AP_DEVICE_ERROR;
	}

	ret = inputhub_wrapper_send_cmd(tag, cmd, NULL, 0, NULL);
	if (ret != RET_SUCC)
		return RET_FAIL;

	return RET_SUCC;
}

/*
 * Function    : should_be_processed_when_sr
 * Description : some app will not stop when suspend
 * Input       : [device_tag] key, identify an app of sensor
 * Output      : none
 * Return      : true need to process, false no need to process
 */
static bool should_be_processed_when_sr(int device_tag)
{
	bool ret = true; /* can be closed default */

	switch (device_tag) {
	case TAG_PS:
	case TAG_STEP_COUNTER:
	case TAG_SIGNIFICANT_MOTION:
	case TAG_PHONECALL:
	case TAG_CONNECTIVITY:
	case TAG_FP:
	case TAG_FP_UD:
	case TAG_MAGN_BRACKET:
		ret = false;
		break;

	default:
		break;
	}

	return ret;
}

void disable_devices_when_suspend(void)
{
	int tag;

	mutex_lock(&g_mutex_update);
	for (tag = TAG_DEVICE_BEGIN; tag < TAG_DEVICE_END; ++tag) {
		if (g_device_status.opened[tag] &&
			should_be_processed_when_sr(tag))
			// if close failed when suspend, just failed
			(void)inputhub_device_enable_internal(tag, false);
	}
	mutex_unlock(&g_mutex_update);
}

void enable_devices_when_resume(void)
{
	int tag;
	int ret;
	interval_param_t delay_param = {
		.period = 0,
		.batch_count = 1,
		.mode = AUTO_MODE,
		.reserved[0] = TYPE_STANDARD
	};

	mutex_lock(&g_mutex_update);
	for (tag = TAG_DEVICE_BEGIN; tag < TAG_DEVICE_END; ++tag) {
		if (should_be_processed_when_sr(tag)) {
			if (g_device_status.opened[tag]) {
				ret = inputhub_device_enable_internal(tag, true);
				if (ret != RET_SUCC) {
					update_device_enable_status(tag, false);
					continue;
				}
			}

			if (g_device_status.status[tag]) {
				delay_param.period = g_device_status.delay[tag];
				delay_param.batch_count = g_device_status.batch_cnt[tag];
				ret = inputhub_device_setdelay_internal(tag, &delay_param);
				if (ret != RET_SUCC)
					update_device_delay_status(tag, -1, -1, false);
			}
		}
	}
	mutex_unlock(&g_mutex_update);
}

static int sensorhub_pm_notify(struct notifier_block *nb,
			       unsigned long mode, void *_unused)
{
	switch (mode) {
	case PM_SUSPEND_PREPARE: /* suspend */
		ctxhub_info("suspend in %s\n", __func__);
		disable_devices_when_suspend();
		break;

	case PM_POST_SUSPEND: /* resume */
		ctxhub_info("resume in %s\n", __func__);
		enable_devices_when_resume();
		break;

	case PM_HIBERNATION_PREPARE: /* Going to hibernate */
		ctxhub_info("hibernation prepare in %s +\n", __func__);
		if (sensorhub_pm_s4_entry() != RET_SUCC)
			ctxhub_warn("sensorhub_pm_s4_entry failed in %s\n", __func__);
		ctxhub_info("hibernation prepare in %s -\n", __func__);
		break;

	case PM_POST_HIBERNATION: /* Hibernation finished */
	case PM_RESTORE_PREPARE: /* Going to restore a saved image */
	case PM_POST_RESTORE: /* Restore failed */
	default:
		break;
	}

	return 0;
}

/*
 * Function    : work_on_ap
 * Description : according to tag, retrun device work on ap or not
 * Input       : [tag] key, identify an app of device
 * Output      : none
 * Return      : true is work on ap, false is not
 */
bool work_on_ap(int tag)
{
	if (tag < TAG_DEVICE_BEGIN || tag >= TAG_DEVICE_END)
		return false;

	return g_all_ap_device_operations[tag].work_on_ap;
}

/*
 * Function    : ap_device_enable
 * Description : ap device enable/disable
 * Input       : [tag] key, identify an app of device
 *               [enable] true is enable, false is disable
 * Output      : none
 * Return      : 0 is enable ok, otherwise is not
 */
int ap_device_enable(int tag, bool enable)
{
	int ret;

	if (tag < TAG_DEVICE_BEGIN || tag >= TAG_DEVICE_END) {
		ctxhub_warn("%s, tag %d invalid\n", __func__, tag);
		return RET_FAIL;
	}

	if (work_on_ap(tag)) {
		if (g_all_ap_device_operations[tag].ops.enable) {
			ret = g_all_ap_device_operations[tag].ops.enable(enable);
			if (ret != 0) {
				ctxhub_err("%s, ap device %d enable %d failed!\n",
					__func__, tag, enable);
				return RET_FAIL;
			}
		}
	}
	return RET_SUCC;
}

/*
 * Function    : ap_device_setdelay
 * Description : ap device setdelay
 * Input       : [tag] key, identify an app of device
 *               [ms] delay time, ms
 * Output      : none
 * Return      : 0 is set ok, otherwise is not
 */
int ap_device_setdelay(int tag, int ms)
{
	int ret;

	if (tag < TAG_DEVICE_BEGIN || tag >= TAG_DEVICE_END) {
		ctxhub_err("%s, tag %d invalid\n", __func__, tag);
		return RET_FAIL;
	}

	if (work_on_ap(tag)) {
		if (g_all_ap_device_operations[tag].ops.setdelay) {
			ret = g_all_ap_device_operations[tag].ops.setdelay(ms);
			if (ret != 0) {
				ctxhub_err("%s, ap device %d set delay failed!\n",
					__func__, tag);
				return RET_FAIL;
			}
		}
	}
	return RET_SUCC;
}

/*
 * Function    : inputhub_device_enable
 * Description : enable/disable device
 * Input       : [tag] key, identify an app of device
 *             : [enable] true for enable, false for disable
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int inputhub_device_enable(int tag, bool enable)
{
	int ret;

	if (tag < TAG_BEGIN || tag >= TAG_END) {
		ctxhub_err("NULL pointer param in %s or tag %d is error.\n",
			__func__, tag);
		return -EINVAL;
	}

	if (tag >= TAG_DEVICE_BEGIN && tag < TAG_DEVICE_END) {
		mutex_lock(&g_mutex_update);
		if (g_device_status.opened[tag] == enable) {
			mutex_unlock(&g_mutex_update);
			ctxhub_info("%s, status is same, exit directly\n", __func__);
			return RET_SUCC;
		}

		ret = inputhub_device_enable_internal(tag, enable);
		if (ret != RET_SUCC) {
			mutex_unlock(&g_mutex_update);
			ctxhub_err("%s, device enable failed!Ret is %d\n", __func__, ret);
			return RET_FAIL;
		}
		update_device_enable_status(tag, enable);
		mutex_unlock(&g_mutex_update);
	} else {
		/*
		 * For tag not in [TAG_DEVICE_BEGIN, TAG_DEVICE_END), don't save device status.
		 */
		ret = inputhub_device_enable_internal(tag, enable);
		if (ret != RET_SUCC)
			return RET_FAIL;
	}
	return RET_SUCC;
}

/*
 * Function    : inputhub_device_setdelay
 * Description : set delay for device
 * Input       : [tag] key, identify an app of device
 *             : [interval_param] interval param info
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int inputhub_device_setdelay(int tag, const interval_param_t *interval_param)
{
	int ret;

	if (!interval_param || (tag < TAG_BEGIN || tag >= TAG_END)) {
		ctxhub_err("NULL pointer param in %s or tag %d is error.\n",
			__func__, tag);
		return -EINVAL;
	}

	if (tag >= TAG_DEVICE_BEGIN && tag < TAG_DEVICE_END) {
		mutex_lock(&g_mutex_update);
		ret = inputhub_device_setdelay_internal(tag, interval_param);
		if (ret != RET_SUCC) {
			mutex_unlock(&g_mutex_update);
			ctxhub_err("%s, set device delay failed!Ret is %d\n", __func__, ret);
			return RET_FAIL;
		}
		update_device_delay_status(tag, interval_param->period,
					interval_param->batch_count, true);
		mutex_unlock(&g_mutex_update);
	} else {
		ret = inputhub_device_setdelay_internal(tag, interval_param);
		if (ret != RET_SUCC)
			return RET_FAIL;
	}

	return RET_SUCC;
}

/*
 * Function    : register_ap_device_operations
 * Description : register ap device operation
 * Input       : [tag] key, identify an app of device
 *             : [ops] callback for ap device
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int register_ap_device_operations(int tag, struct device_operation *ops)
{
	if (!(tag >= TAG_DEVICE_BEGIN && tag < TAG_DEVICE_END)) {
		ctxhub_err("tag %d range error in %s\n", tag, __func__);
		return -EINVAL;
	}

	if (!g_all_ap_device_operations[tag].work_on_ap) {
		(void)memcpy_s(&g_all_ap_device_operations[tag].ops,
			sizeof(struct device_operation), ops,
			sizeof(struct device_operation));
		g_all_ap_device_operations[tag].work_on_ap = true;
	} else {
		ctxhub_warn("tag %d has registered already in %s\n", tag, __func__);
	}

	return 0;
}

/*
 * Function    : unregister_ap_device_operations
 * Description : unregister ap device operation
 * Input       : [tag] key, identify an app of device
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int unregister_ap_device_operations(int tag)
{
	if (!(tag >= TAG_DEVICE_BEGIN && tag < TAG_DEVICE_END)) {
		ctxhub_err("tag %d range error in %s\n", tag, __func__);
		return -EINVAL;
	}
	(void)memset_s(&g_all_ap_device_operations[tag],
		sizeof(g_all_ap_device_operations[tag]), 0,
		sizeof(g_all_ap_device_operations[tag]));
	return 0;
}

static void enable_devices_when_recovery_iom3(void)
{
	int tag;
	interval_param_t interval_param;
	int ret;

	mutex_lock(&g_mutex_update);
	for (tag = TAG_DEVICE_BEGIN; tag < TAG_DEVICE_END; ++tag) {
		if (g_device_status.opened[tag]) {
			ret = inputhub_device_enable_internal(tag, true);
			if (ret != RET_SUCC) {
				update_device_enable_status(tag, false);
				continue;
			}
		}

		if (g_device_status.status[tag]) {
			interval_param.period = g_device_status.delay[tag];
			interval_param.batch_count = g_device_status.batch_cnt[tag];
			interval_param.mode = AUTO_MODE;
			interval_param.reserved[0] = TYPE_STANDARD;
			ret = inputhub_device_setdelay_internal(tag, &interval_param);
			if (ret != RET_SUCC)
				update_device_delay_status(tag, -1, -1, false);
		}
	}
	mutex_unlock(&g_mutex_update);
}

static int device_recovery_notifier(struct notifier_block *nb,
				    unsigned long foo, void *bar)
{
	ctxhub_info("%s %lu +\n", __func__, foo);
	switch (foo) {
	case IOM3_RECOVERY_IDLE:
	case IOM3_RECOVERY_START:
	case IOM3_RECOVERY_MINISYS:
	case IOM3_RECOVERY_3RD_DOING:
	case IOM3_RECOVERY_FAILED:
		break;
	case IOM3_RECOVERY_DOING:
		reset_calibrate_data_to_mcu();
		enable_devices_when_recovery_iom3();
		break;
	default:
		ctxhub_err("%s -unknown state %lu\n", __func__, foo);
		break;
	}
	ctxhub_info("%s -\n", __func__);
	return 0;
}

static struct notifier_block g_device_recovery_notify = {
	.notifier_call = device_recovery_notifier,
	.priority = -1,
};

static void disable_devices_when_sysreboot(void)
{
	int tag;
	int ret;

	for (tag = TAG_DEVICE_BEGIN; tag < TAG_DEVICE_END; ++tag) {
		if (g_device_status.opened[tag]) {
			ret = inputhub_device_enable_internal(tag, false);
			if (ret != RET_SUCC)
				ctxhub_warn("%s, %d disabled failed!\n", __func__, tag);
			msleep(DISABLE_SLEEP_TIME);
			ctxhub_info("disable device - %d before reboot\n", tag);
		}
	}
}

static int shb_reboot_notifier(struct notifier_block *nb, unsigned long foo,
			       void *bar)
{
	/* prevent access the emmc now: */
	ctxhub_info("shb:%s: %lu +\n", __func__, foo);
	if (foo == SYS_RESTART)
		disable_devices_when_sysreboot();

	ctxhub_info("shb:%s: -\n", __func__);
	return 0;
}

static struct notifier_block g_shb_reboot_notify = {
	.notifier_call = shb_reboot_notifier,
	.priority = -1,
};

static struct notifier_block g_sensorhub_pm_notify = {
	.notifier_call = sensorhub_pm_notify,
	.priority = 0
};

static int device_interface_init(void)
{
	int ret;

	ctxhub_info("%s, enter\n", __func__);

	if (get_contexthub_dts_status())
		return -EINVAL;

	ret = register_pm_notifier(&g_sensorhub_pm_notify);
	if (ret != 0) {
		ctxhub_err("register pm notifier failed, ret = %d\n", ret);
		return ret;
	}

	ret = register_reboot_notifier(&g_shb_reboot_notify);
	if (ret != 0) {
		ctxhub_err("register reboot notifier failed, ret = %d\n", ret);
		goto out_pm_nofifier;
	}

	ret = register_iom3_recovery_notifier(&g_device_recovery_notify);
	if (ret != 0) {
		ctxhub_err("register iom3 recovery notifier failed, ret = %d\n",
			ret);
		goto out_reboot_notifier;
	}

	ctxhub_info("%s, ok\n", __func__);
	return 0;

out_reboot_notifier:
	unregister_reboot_notifier(&g_shb_reboot_notify);
out_pm_nofifier:
	unregister_pm_notifier(&g_sensorhub_pm_notify);

	return ret;
}

static void __exit device_interface_exit(void)
{
	unregister_reboot_notifier(&g_shb_reboot_notify);
	unregister_pm_notifier(&g_sensorhub_pm_notify);
	ctxhub_info("exit %s\n", __func__);
}


device_initcall(device_interface_init);
module_exit(device_interface_exit);

MODULE_AUTHOR("Smart");
MODULE_DESCRIPTION("device interface driver");
MODULE_LICENSE("GPL");
