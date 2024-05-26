/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020. All rights reserved.
 * Description: pedometer implement.
 * Create: 2020/2/17
 */

#include "pedometer.h"
#include <securec.h>

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include <linux/suspend.h>

#include "iomcu_log.h"
#include "device_interface.h"
#include "iomcu_route.h"
#include "device_manager.h"
#include "platform_include/smart/linux/iomcu_dump.h"
#include "common/common.h"
#include "sensor_channel.h"
#include "inputhub_wrapper/inputhub_wrapper.h"
#include "device_common.h"

#define DEFAULT_INTERVAL 0x7FFFFFFF
#define DEFAULT_LATENCY 0
#define WAKEUP 0
#define NON_WAKEUP 1

static struct customized_interface *g_pedometer_interface;

static DEFINE_MUTEX(g_mutex_pedo_status);

struct sensorhub_type_map_pedometer_type {
	int pedometer_shb_type;
	int pedometer_type;
};

static struct sensorhub_type_map_pedometer_type g_shb2pedometer_map[] = {
	{SENSORHUB_TYPE_STEP_COUNTER, PEDO_STEP_COUNTER},
	{SENSORHUB_TYPE_STEP_DETECTOR, PEDO_STEP_DETECTOR},
	{SENSORHUB_TYPE_STEP_COUNTER_WAKEUP, PEDO_STEP_WAKEUP},
};

static int g_enable_cnt;
static struct pedometer_status g_pedometer_status[] = {
	{SENSORHUB_TYPE_STEP_COUNTER, false, DEFAULT_INTERVAL, DEFAULT_LATENCY, NON_WAKEUP,
	 SENSOR_FLAG_ON_CHANGE_MODE, "step_counter"},
	{SENSORHUB_TYPE_STEP_DETECTOR, false, DEFAULT_INTERVAL, DEFAULT_LATENCY, NON_WAKEUP,
	 SENSOR_FLAG_SPECIAL_MODE, "step_detector"},
	{SENSORHUB_TYPE_STEP_COUNTER_WAKEUP, false, DEFAULT_INTERVAL, DEFAULT_LATENCY, WAKEUP,
	 SENSOR_FLAG_SPECIAL_MODE + SENSOR_WAKEUP_FLAG, "step_counter_wakeup"},
};

struct sensorlist_info *get_pedo_sensor_list_info(int shb_type)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(g_pedometer_status); i++) {
		if (g_pedometer_status[i].pedometer_shb_type == shb_type)
			return &g_pedometer_status[i].sensor_info;
	}
	return NULL;
}

static int check_param(int shb_type)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(g_shb2pedometer_map); i++) {
		if (g_shb2pedometer_map[i].pedometer_shb_type == shb_type)
			return RET_SUCC;
	}
	return RET_FAIL;
}

struct pedometer_status *get_pedo_status(int shb_type)
{
	int ret;
	u32 i;

	ret = check_param(shb_type);
	if (ret != RET_SUCC) {
		ctxhub_err("%s, check param failed!input type is %d\n",
			   __func__, shb_type);
		return NULL;
	}

	for (i = 0; i < ARRAY_SIZE(g_pedometer_status); i++) {
		if (g_pedometer_status[i].pedometer_shb_type == shb_type)
			return &g_pedometer_status[i];
	}
	return NULL;
}

static int report_flush_event(struct inputhub_route_table *route_item, int shb_type)
{
	struct sensor_data event;
	int ret;

	event.type = SENSORHUB_TYPE_META_DATA;
	event.length = sizeof(int);
	event.value[0] = shb_type;
	ret = inputhub_route_write_batch(route_item, (char *)&event,
					 event.length + offsetofend(struct sensor_data, length),
					 0);
	if (ret <= 0) {
		ctxhub_err("%s, write flush event failed!\n", __func__);
		return RET_FAIL;
	}
	return RET_SUCC;
}

static uint8_t get_pedometer_type(int shb_type)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(g_shb2pedometer_map); i++) {
		if (g_shb2pedometer_map[i].pedometer_shb_type == shb_type)
			return (uint8_t)g_shb2pedometer_map[i].pedometer_type;
	}
	return 0xFF;
}

static int get_pedometer_shb_type(int pedometer_type)
{
	u32 i;

	for (i = 0; i < ARRAY_SIZE(g_shb2pedometer_map); i++) {
		if (g_shb2pedometer_map[i].pedometer_type == pedometer_type)
			return g_shb2pedometer_map[i].pedometer_shb_type;
	}
	return -1;
}

static bool pedometer_switch_sensor(int shb_type, bool enable)
{
	u32 i;
	int ret;
	bool result = false;

	ret = check_param(shb_type);
	if (ret != RET_SUCC) {
		ctxhub_err("%s, invalid shb type %d\n", __func__, shb_type);
		return false;
	}

	mutex_lock(&g_mutex_pedo_status);
	for (i = 0; i < ARRAY_SIZE(g_pedometer_status); i++) {
		if (g_pedometer_status[i].pedometer_shb_type == shb_type) {
			result = (g_pedometer_status[i].enable != enable);
			break;
		}
	}
	mutex_unlock(&g_mutex_pedo_status);
	return result;
}

static void update_pedo_status(bool enable, int shb_type)
{
	u32 i;
	int delta = enable ? 1 : -1;

	for (i = 0; i < ARRAY_SIZE(g_pedometer_status); i++) {
		if (g_pedometer_status[i].pedometer_shb_type == shb_type) {
			g_enable_cnt += delta;
			g_pedometer_status[i].enable = enable;
			break;
		}
	}
}

#define PEDO_TIMER 100
/*
 * Function    : pedometer_enable_internal
 * Description : Enable pedo activity.If no activity enabled before, you need open TAG_PEDOMETER APP,
 *             : set interval (sensorhub will start a timer) and add cared pedo activty(send config packet).
 *             : When you send the packet of cared pedo activity, your ar activity don't have
 *             : interval and report latency, now we use defalut interval and default latency.
 * Input       : [shb_type] sensorhub type of pedo
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
static int pedometer_enable_internal(int shb_type)
{
	int ret;
	struct pedo_config_packet packet;
	interval_param_t interval_param;
	struct pedometer_status *p_pedo_status = NULL;

	ctxhub_info("%s, enable %d enter!current enable cnt %d\n", __func__, shb_type, g_enable_cnt);
	mutex_lock(&g_mutex_pedo_status);
	if (g_enable_cnt == 0) {
		ret = inputhub_device_enable(TAG_PEDOMETER, true);
		if (ret != RET_SUCC) {
			mutex_unlock(&g_mutex_pedo_status);
			ctxhub_err("%s, enable failed!\n", __func__);
			return ret;
		}

		// 100ms
		interval_param.period = PEDO_TIMER;
		ret = inputhub_device_setdelay(TAG_PEDOMETER, &interval_param);
		if (ret != RET_SUCC) {
			mutex_unlock(&g_mutex_pedo_status);
			ctxhub_err("%s, set interval failed!\n", __func__);
			return ret;
		}
	}

	p_pedo_status = get_pedo_status(shb_type);
	packet.sub_cmd = SUB_CMD_SET_PARAMET_REQ;
	packet.config.oper = PEDO_ENABLE_SENSOR;
	packet.config.sensor = get_pedometer_type(shb_type);
	packet.config.non_wakeup = p_pedo_status->non_wakeup;
	packet.config.interval = DEFAULT_INTERVAL;
	packet.config.latency = DEFAULT_LATENCY;

	ret = inputhub_wrapper_send_cmd(TAG_PEDOMETER, CMD_CMN_CONFIG_REQ, (const void *)&packet,
					sizeof(packet), NULL);
	if (ret != RET_SUCC) {
		mutex_unlock(&g_mutex_pedo_status);
		ctxhub_err("%s, config cmd send failed!ret is %d\n", __func__, ret);
		return RET_FAIL;
	}

	// after send config succ, record interval and latency
	update_pedo_status(true, shb_type);
	p_pedo_status->interval = DEFAULT_INTERVAL;
	p_pedo_status->latency = DEFAULT_LATENCY;
	mutex_unlock(&g_mutex_pedo_status);
	return RET_SUCC;
}

/*
 * Function    : pedomter_disable_internal
 * Description : Disable pedo activity.When func come in here, the status of pedo activity must be active now(status
 *             : has been checked by caller).
 *             : After close this activity success, if cnt of enabled pedo activity become 0,
 *             : you need close TAG_PEDOMETER APP.
 * Input       : [shb_type] sensorhub type of pedo
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
static int pedometer_disable_internal(int shb_type)
{
	int ret;
	struct pedo_config_packet packet;
	struct pedometer_status *p_pedo_status = NULL;

	ctxhub_info("%s, disable %d enter!\n", __func__, shb_type);

	packet.sub_cmd = SUB_CMD_SET_PARAMET_REQ;
	packet.config.oper = PEDO_DISABLE_SENSOR;
	packet.config.sensor = get_pedometer_type(shb_type);

	ret = inputhub_wrapper_send_cmd(TAG_PEDOMETER, CMD_CMN_CONFIG_REQ, (const void *)&packet,
					sizeof(packet), NULL);
	if (ret != RET_SUCC) {
		ctxhub_err("%s, disable %d failed!ret is %d\n", __func__, shb_type, ret);
		return RET_FAIL;
	}

	ctxhub_info("%s, disable %d succ!enable cnt %d\n", __func__, shb_type, g_enable_cnt);
	mutex_lock(&g_mutex_pedo_status);
	p_pedo_status = get_pedo_status(shb_type);
	p_pedo_status->interval = DEFAULT_INTERVAL;
	p_pedo_status->latency = DEFAULT_LATENCY;
	update_pedo_status(false, shb_type);
	if (g_enable_cnt == 0) {
		ret = inputhub_device_enable(TAG_PEDOMETER, false);
		if (ret != RET_SUCC) {
			mutex_unlock(&g_mutex_pedo_status);
			ctxhub_err("%s, disable failed!\n", __func__);
			return ret;
		}
	}
	mutex_unlock(&g_mutex_pedo_status);
	return RET_SUCC;
}

/*
 * Function    : pedometer_enable
 * Description : enable pedo activity.
 * Input       : [shb_type] sensorhub type of pedo
 *             : [enable]] enable or disable
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int pedometer_enable(int shb_type, bool enable)
{
	int ret;

	ret = check_param(shb_type);
	if (ret != RET_SUCC) {
		ctxhub_err("%s, invalid shb type %d\n", __func__, shb_type);
		return RET_FAIL;
	}

	if (pedometer_switch_sensor(shb_type, enable)) {
		if (enable)
			return pedometer_enable_internal(shb_type);
		else
			return pedometer_disable_internal(shb_type);
	}
	return 0;
}

static check_pedo_head(const struct pkt_header *head)
{
	// have check null before
	s32 pkt_len = head->length;
	s32 expect_len = sizeof(struct pedo_report_data);

	/*
	 * pedo_report_pkt is {
	 *    struct pkt_header hd;
	 *    struct pedo_report_data data;
	 * }
	 * hd.length must equal sizeof(struct pedo_report_data), pack is 1.
	 */
	if (expect_len != pkt_len) {
		ctxhub_err("%s, head len %d is not equal %d\n", __func__, pkt_len, expect_len);
		return RET_FAIL;
	}
	return RET_SUCC;
}

/*
 * Function    : pedometer_report_event
 * Description : report pedo event
 * Input       : [route_item] event will write to route item
 *             : [head] report packet from sensorhub
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
int pedometer_report_event(struct inputhub_route_table *route_item, const struct pkt_header *head)
{
	struct pedo_report_pkt *p_head = (struct pedo_report_pkt *)head;
	struct sensor_data event;
	int ret;

	if (!route_item || !head) {
		ctxhub_err("%s, input is null\n", __func__);
		return RET_FAIL;
	}

	if (check_pedo_head(head) != RET_SUCC) {
		ctxhub_err("%s, report head error\n", __func__);
		return RET_FAIL;
	}

	event.type = get_pedometer_shb_type(p_head->data.sensor);
	event.length = sizeof(int);

	switch (event.type) {
	case SENSORHUB_TYPE_STEP_COUNTER:
	case SENSORHUB_TYPE_STEP_COUNTER_WAKEUP:
		event.value[0] = p_head->data.steps;
		break;

	case SENSORHUB_TYPE_STEP_DETECTOR:
		// for step detector, only one step need report
		event.value[0] = 1;
		break;
	default:
		ctxhub_err("%s, unsupport event type %d\n", __func__, event.type);
		return RET_FAIL;
	}

	sensor_get_data(&event);

	// sensorhub timestamp is ok, use directly;
	// else use ap timestamp
	if (p_head->data.flag & DATA_FLAG_VALID_TIMESTAMP)
		ret = inputhub_route_write_batch(route_item, (char *)&event,
						 event.length + offsetofend(struct sensor_data, length),
						 p_head->data.timestamp);
	else
		ret = inputhub_route_write(route_item, (char *)&event,
					   event.length + offsetofend(struct sensor_data, length));
	if (ret <= 0) {
		ctxhub_err("%s, pedometer event write failed!\n", __func__);
		return RET_FAIL;
	}

	if (p_head->data.flag & FLUSH_END) {
		ret = report_flush_event(route_item, event.type);
		if (ret != RET_SUCC) {
			ctxhub_err("%s, report flush event failed\n", __func__);
			return RET_FAIL;
		}
	}

	return RET_SUCC;
}


/*
 * Function    : pedometer_setdelay
 * Description : Set report latency for pedo activity, period for pedo has effect just for step counter wakeup
 *             : which is configed target steps, all of pedo activity will transport timeout to sensorhub.If
 *             : is_batch is true, will use timeout, else use defalut latency.
 * Input       : [shb_type] sensorhub type of pedo
 *             : [period] just for step counter wakeup currently, others will transport to sensorhub, but pedo
 *             :          app will not use.
 *             : [timeout] max report latency, will transport to sensorhub.Whether it is used depends on pedo app.
 *             : [is_batch] true, timeout is valid; false, timeout is invalidi, will use defalut latency.
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
static int pedometer_setdelay(int shb_type, int period, int timeout, bool is_batch)
{
	int ret;
	struct pedo_config_packet packet;
	struct pedometer_status *p_pedo_status = NULL;

	ctxhub_info("%s, pedo %d set delay %d timeout %d is_batch %d enter!\n",
		    __func__, shb_type, period, timeout, is_batch);
	ret = check_param(shb_type);
	if (ret != RET_SUCC) {
		ctxhub_err("%s, invalid shb type %d\n", __func__, shb_type);
		return RET_FAIL;
	}

	p_pedo_status = get_pedo_status(shb_type);
	packet.sub_cmd = SUB_CMD_SET_PARAMET_REQ;
	packet.config.oper = PEDO_ENABLE_SENSOR;
	packet.config.sensor = get_pedometer_type(shb_type);
	/*
	 * period is only use for step counter wakeup, is cfg target steps.
	 * for others event, period only transport to sensorhub, no use.
	 */
	packet.config.interval = period;
	if (is_batch)
		packet.config.latency = timeout;
	else
		packet.config.latency = DEFAULT_LATENCY; // else is set delay
	packet.config.non_wakeup = p_pedo_status->non_wakeup;

	ret = inputhub_wrapper_send_cmd(TAG_PEDOMETER, CMD_CMN_CONFIG_REQ, (const void *)&packet,
					sizeof(packet), NULL);
	if (ret != RET_SUCC) {
		ctxhub_err("%s, set delay send failed!Ret is %d\n", __func__, ret);
		return RET_FAIL;
	}
	mutex_lock(&g_mutex_pedo_status);
	p_pedo_status->interval = period;
	p_pedo_status->latency = packet.config.latency;
	mutex_unlock(&g_mutex_pedo_status);
	ctxhub_info("%s, set delay %d succ!\n", __func__, shb_type);
	return RET_SUCC;
}


/*
 * Function    : pedometer_flush
 * Description : Flush specified pedo activity, for disabled pedo activity, flush cmd will
 *             : be intercepted by param check in sensor hal.
 * Input       : [shb_type] sensorhub type of pedo
 * Output      : none
 * Return      : 0 is ok, otherwise failed
 */
static int pedometer_flush(int shb_type)
{
	int ret;
	struct pedo_flush_pkt flush_data;

	ret = check_param(shb_type);
	if (ret != RET_SUCC) {
		ctxhub_err("%s, invalid shb type %d\n", __func__, shb_type);
		return RET_FAIL;
	}

	ctxhub_info("enter pedometer flush\n");
	flush_data.sub_cmd = SUB_CMD_FLUSH_REQ;
	flush_data.data.sensor = get_pedometer_type(shb_type);

	ctxhub_info("flush sensor %u\n", flush_data.data.sensor);
	ret = inputhub_wrapper_send_cmd(TAG_PEDOMETER, CMD_CMN_CONFIG_REQ, (const void *)&flush_data,
					sizeof(flush_data), NULL);
	if (ret != 0) {
		ctxhub_err("%s, pedometer flush failed!ret is %d\n", __func__, ret);
		return RET_FAIL;
	}

	return RET_SUCC;
}

static void set_disable_flag()
{
	u32 i;

	g_enable_cnt = 0;
	for (i = 0; i < ARRAY_SIZE(g_pedometer_status); i++)
		g_pedometer_status[i].enable = false;
}

/*
 * Function    : enable_pedo_when_recovery_iom3
 * Description : recovery pedo app and cared activity.
 * Input       : none
 * Output      : none
 * Return      : none
 */
void enable_pedo_when_recovery_iom3(void)
{
	struct pedo_config_packet packet;
	int ret;
	u32 i;
	interval_param_t interval_param;

	mutex_lock(&g_mutex_pedo_status);
	if (g_enable_cnt == 0) {
		mutex_unlock(&g_mutex_pedo_status);
		ctxhub_warn("%s, don't care any pedo event, don't open pedo\n", __func__);
		return;
	}

	ret = inputhub_device_enable(TAG_PEDOMETER, true);
	if (ret != RET_SUCC) {
		set_disable_flag();
		mutex_unlock(&g_mutex_pedo_status);
		ctxhub_err("%s, enable failed!\n", __func__);
		return;
	}

	interval_param.period = PEDO_TIMER;
	ret = inputhub_device_setdelay(TAG_PEDOMETER, &interval_param);
	if (ret != RET_SUCC) {
		set_disable_flag();
		mutex_unlock(&g_mutex_pedo_status);
		ctxhub_err("%s, recovery pedo set timer failed!\n", __func__);
		return;
	}

	for (i = 0; i < ARRAY_SIZE(g_pedometer_status); i++) {
		if (g_pedometer_status[i].enable) {
			packet.sub_cmd = SUB_CMD_SET_PARAMET_REQ;
			packet.config.oper = PEDO_ENABLE_SENSOR;
			packet.config.sensor = get_pedometer_type(g_pedometer_status[i].pedometer_shb_type);
			packet.config.interval = g_pedometer_status[i].interval;
			packet.config.latency = g_pedometer_status[i].latency;
			packet.config.non_wakeup = g_pedometer_status[i].non_wakeup;
			ret = inputhub_wrapper_send_cmd(TAG_PEDOMETER, CMD_CMN_CONFIG_REQ, (const void *)&packet,
							sizeof(packet), NULL);
			if (ret != RET_SUCC) {
				g_pedometer_status[i].enable = false;
				g_enable_cnt--;
				ctxhub_err("%s, resume %d send failed!ret is %d\n", __func__,
					g_pedometer_status[i].pedometer_shb_type, ret);
			}
		}
	}
	mutex_unlock(&g_mutex_pedo_status);
}

static int pedo_recovery_notifier(struct notifier_block *nb,
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
		enable_pedo_when_recovery_iom3();
		break;
	default:
		ctxhub_err("%s -unknown state %ld\n", __func__, foo);
		break;
	}
	ctxhub_info("%s -\n", __func__);
	return 0;
}

#define PEDO_RECOVERY_PRIORITY (-2)
static struct notifier_block g_pedo_recovery_notify = {
	.notifier_call = pedo_recovery_notifier,
	.priority = PEDO_RECOVERY_PRIORITY, // smaller than true sensor is better
};

static void get_pedo_sensor_info(void)
{
	u32 i;
	struct device_node *node = NULL;
	bool wakeup = false;

	for (i = 0; i < ARRAY_SIZE(g_pedometer_status); i++) {
		node = of_find_compatible_node(NULL, NULL, g_pedometer_status[i].dts_name);
		if (node) {
			read_sensorlist_info(node, &g_pedometer_status[i].sensor_info, TAG_PEDOMETER);
			if (g_pedometer_status[i].sensor_info.flags != INVALID_VALUE) {
				wakeup = SENSOR_WAKEUP_FLAG & g_pedometer_status[i].sensor_info.flags;
				g_pedometer_status[i].non_wakeup = !wakeup ? NON_WAKEUP : WAKEUP;
				g_pedometer_status[i].flags = g_pedometer_status[i].sensor_info.flags;
			}
		}
	}
}

static int __init pedometer_init(void)
{
	int ret;

	ctxhub_info("%s, enter\n", __func__);
	if (get_contexthub_dts_status())
		return RET_FAIL;

	get_pedo_sensor_info();

	g_pedometer_interface = (struct customized_interface *)vzalloc(sizeof(struct customized_interface));
	if (!g_pedometer_interface) {
		ctxhub_err("%s, alloc failed", __func__);
		return RET_FAIL;
	}

	ret = register_iom3_recovery_notifier(&g_pedo_recovery_notify);
	if (ret != 0) {
		ctxhub_err("%s, register pedo recovery notifer failed!\n", __func__);
		ret = RET_FAIL;
		goto free;
	}
	g_pedometer_interface->enable = pedometer_enable;
	g_pedometer_interface->report_event = pedometer_report_event;
	g_pedometer_interface->setdelay = pedometer_setdelay;
	g_pedometer_interface->flush = pedometer_flush;

	ret = register_customized_interface(SENSORHUB_TYPE_STEP_COUNTER, TAG_PEDOMETER, g_pedometer_interface);
	if (ret != RET_SUCC)
		ctxhub_warn("%s, register interface for step counter failed!\n", __func__);

	ret = register_customized_interface(SENSORHUB_TYPE_STEP_COUNTER_WAKEUP, TAG_PEDOMETER, g_pedometer_interface);
	if (ret != RET_SUCC)
		ctxhub_warn("%s, register interface for step counter wakeup failed!\n", __func__);

	ret = register_customized_interface(SENSORHUB_TYPE_STEP_DETECTOR, TAG_PEDOMETER, g_pedometer_interface);
	if (ret != RET_SUCC)
		ctxhub_warn("%s, register interface for step detector failed!\n", __func__);

	ctxhub_info("%s, exit\n", __func__);
	return ret;

free:
	vfree(g_pedometer_interface);
	g_pedometer_interface = NULL;
	return ret;
}

static void __exit pedometer_exit(void)
{
	int ret;

	if (get_contexthub_dts_status())
		return;

	ret = unregister_customized_interface(SENSORHUB_TYPE_STEP_COUNTER);
	if (ret != RET_SUCC)
		ctxhub_warn("%s, unregister interface for step counter failed!\n", __func__);

	ret = unregister_customized_interface(SENSORHUB_TYPE_STEP_COUNTER_WAKEUP);
	if (ret != RET_SUCC)
		ctxhub_warn("%s, unregister interface for step counter failed!\n", __func__);

	ret = unregister_customized_interface(SENSORHUB_TYPE_STEP_DETECTOR);
	if (ret != RET_SUCC)
		ctxhub_warn("%s, unregister interface for step detector failed!\n", __func__);

	if (g_pedometer_interface)
		vfree(g_pedometer_interface);
	g_pedometer_interface = NULL;
}

late_initcall(pedometer_init);
module_exit(pedometer_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("pedometer kernel impl");
MODULE_AUTHOR("Smart");
